// Copyright 2026 mockingb1rdblue
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Per-layer neon keypress heatmap (RGB Matrix custom effect backend).
//
// BEHAVIOR CONTRACT (operator spec 2026-06-11 rev-14; Sonar Pro consult on the
// normalization curve, gamma/temporal-dither smoothness, and split counting):
//   * TWO rolling windows of key-DOWN edges, bucketed per (layer, LED):
//       SHORT = last HM_SHORT (100) presses  -> the "recently used" map.
//       LONG  = last HM_LONG  (2000) presses -> the long-term map.
//     Both updated on every 0->1 matrix edge (same mechanism bongo uses; the
//     mirrored matrix makes each half see the full matrix, so both build
//     identical counters and each renders its own 18 LEDs).
//   * NORMALIZATION (Sonar): t = log(1 + k*u) / log(1 + k), u = c/denom, k=15.
//     This compresses the low end so a key pressed only a FEW times already
//     reads clearly while the hottest sits at purple-pink -- giving the bulk of
//     pressed keys a visible, distinguishable shade (operator: "~80% lit, obvious
//     which keys get neglected"). denom is the per-layer hottest count CLAMPED to
//     [HM_DENOM_MIN(35), cap]: the 35 floor makes ~3 presses land in YELLOW from
//     a cold start, and the cap (100 short / 400 long) lets it "go solid by
//     50-100 and keep normalizing" as traffic grows.
//   * COLOR ramp by t in [0,1]: pressed-but-cold -> WHITE, then YELLOW (~t .30),
//     CYAN (~t .63), PURPLE-PINK at t=1 (most used). Unpressed (count 0) -> OFF,
//     so neglected keys stay obviously dark. NO permanent latch: the map is live
//     and rolls (a key cools as it leaves the window).
//   * IDLE (drives WHICH window + a global fade; never distorts hue):
//       < HM_ACTIVE_MS (500ms)          : ACTIVE -> render the SHORT (rolling-100) map.
//       500ms .. (TIMEOUT - 7s)         : oscillate (crossfade) SHORT<->LONG, 4s loop.
//       last 7s before TIMEOUT          : hold the LONG map and FADE to zero,
//                                         hitting 0 as RGB_MATRIX_SLEEP cuts LEDs.
//   * SMOOTHNESS: value goes through a gamma-2.2 LUT (perceptual) plus per-LED
//     temporal dithering (fractional carry across frames) so low-brightness fades
//     don't band on the 8-bit ws2812. Target framerate ~38Hz (RGB_MATRIX_LED_FLUSH_LIMIT).

#include QMK_KEYBOARD_H
#include <stdbool.h>
#include <math.h>

#define HM_LAYERS    4              // _BASE.._ADJUST
#define HM_LEDS      RGB_MATRIX_LED_COUNT
#define HM_SHORT     100            // recent-use window depth
#define HM_LONG      2000           // long-term window depth
#define HM_DENOM_MIN 35             // denom floor -> ~3 presses == yellow cold
#define HM_CAP_SHORT 100            // short denom cap (goes solid by ~100)
#define HM_CAP_LONG  400            // long denom cap
#define HM_K         15.0f          // log-curve steepness (Sonar)

// Idle-animation timing (ms). Anchored to the RGB sleep timeout (60s).
#define HM_ACTIVE_MS   500
#define HM_OSC_MS      4000         // SHORT<->LONG oscillation loop period
#define HM_FADEOUT_MS  7000         // pre-sleep fade-to-black window

// Per-(layer, LED) rolling counts for each window.
static uint16_t hm_short[HM_LAYERS][HM_LEDS];
static uint16_t hm_long[HM_LAYERS][HM_LEDS];

// Circular histories, each packed (layer<<6 | led); 0xFF == empty.
static uint8_t  hm_ring_s[HM_SHORT];
static uint8_t  hm_ring_l[HM_LONG];
static uint16_t hm_head_s = 0, hm_fill_s = 0;
static uint16_t hm_head_l = 0, hm_fill_l = 0;
static bool     hm_init = false;

// Previous matrix snapshot for 0->1 edge detection (independent of bongo's).
static matrix_row_t hm_prev[MATRIX_ROWS];

// Perceptual gamma LUT + per-LED temporal-dither carry (1/256ths of a level).
static uint8_t hm_gamma[256];
static uint8_t hm_carry[HM_LEDS];

static void hm_push_ring(uint8_t *ring, uint16_t depth, uint16_t *head,
                         uint16_t *fill, uint16_t (*cnt)[HM_LEDS],
                         uint8_t layer, uint8_t led) {
    if (*fill == depth) {
        uint8_t old = ring[*head];
        if (old != 0xFF) {
            uint8_t ol = old >> 6, oe = old & 0x3F;
            if (ol < HM_LAYERS && oe < HM_LEDS && cnt[ol][oe]) cnt[ol][oe]--;
        }
    } else {
        (*fill)++;
    }
    ring[*head] = (uint8_t)((layer << 6) | led);
    *head       = (uint16_t)((*head + 1) % depth);
    if (cnt[layer][led] < depth) cnt[layer][led]++;
}

void heatmap_record_scan(void) {
    if (!hm_init) {
        for (uint16_t i = 0; i < HM_SHORT; i++) hm_ring_s[i] = 0xFF;
        for (uint16_t i = 0; i < HM_LONG; i++)  hm_ring_l[i] = 0xFF;
        for (uint16_t i = 0; i < 256; i++) {
            float x = (float)i / 255.0f;
            hm_gamma[i] = (uint8_t)lroundf(powf(x, 1.0f / 2.2f) * 255.0f);
        }
        hm_init = true;
    }
    uint8_t layer = get_highest_layer(layer_state);
    if (layer >= HM_LAYERS) layer = HM_LAYERS - 1;

    for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
        matrix_row_t cur   = matrix_get_row(r);
        matrix_row_t newly = cur & ~hm_prev[r]; // bits that turned ON this scan
        hm_prev[r]         = cur;
        if (!newly) continue;
        for (uint8_t c = 0; c < MATRIX_COLS; c++) {
            if (!(newly & ((matrix_row_t)1 << c))) continue;
            uint8_t led = g_led_config.matrix_co[r][c];
            if (led == NO_LED || led >= HM_LEDS) continue;
            hm_push_ring(hm_ring_s, HM_SHORT, &hm_head_s, &hm_fill_s, hm_short, layer, led);
            hm_push_ring(hm_ring_l, HM_LONG,  &hm_head_l, &hm_fill_l, hm_long,  layer, led);
        }
    }
}

static uint8_t hm_lerp(uint8_t a, uint8_t b, uint8_t frac /*0..255*/) {
    return (uint8_t)(a + (((int16_t)b - (int16_t)a) * frac) / 255);
}

// Hottest count on a layer, clamped to [HM_DENOM_MIN, cap].
static uint16_t hm_denom(uint16_t (*cnt)[HM_LEDS], uint8_t layer, uint16_t cap) {
    uint16_t mx = 0;
    for (uint8_t i = 0; i < HM_LEDS; i++) if (cnt[layer][i] > mx) mx = cnt[layer][i];
    if (mx < HM_DENOM_MIN) mx = HM_DENOM_MIN;
    if (mx > cap) mx = cap;
    return mx;
}

// count c against denom -> hotness t in [0,1] via the log low-end ramp.
static float hm_norm(uint16_t c, uint16_t denom) {
    if (c == 0) return 0.0f;
    float u = (float)c / (float)denom;
    if (u > 1.0f) u = 1.0f;
    return logf(1.0f + HM_K * u) / logf(1.0f + HM_K);
}

// Hotness t (0..1) -> HSV. white->yellow->cyan->purple-pink; value tracks t.
static hsv_t hm_color(float t, uint8_t maxv) {
    if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f;
    uint8_t h, s;
    if (t <= 0.30f) {                         // white -> yellow
        h = 43;
        s = (uint8_t)(t / 0.30f * 255.0f);    // sat 0 (white) -> 255 (yellow)
    } else if (t <= 0.63f) {                  // yellow -> cyan
        uint8_t f = (uint8_t)((t - 0.30f) / 0.33f * 255.0f);
        h = hm_lerp(43, 128, f);
        s = 255;
    } else {                                  // cyan -> purple-pink
        uint8_t f = (uint8_t)((t - 0.63f) / 0.37f * 255.0f);
        h = hm_lerp(128, 222, f);
        s = 255;
    }
    uint8_t v = (uint8_t)(t * (float)maxv);
    return (hsv_t){h, s, v};
}

// Idle state -> (window-mix alpha 0..255 where 0=SHORT,255=LONG) and a global
// brightness fade 0..255. Active = pure SHORT at full; mid-idle oscillates
// SHORT<->LONG; last 7s holds LONG and fades to zero.
static void hm_idle(uint8_t *mix, uint8_t *fade) {
    uint32_t since = last_input_activity_elapsed();
    if (since < HM_ACTIVE_MS) { *mix = 0; *fade = 255; return; }

    uint32_t fade_start = (uint32_t)RGB_MATRIX_TIMEOUT - HM_FADEOUT_MS;
    if (since < fade_start) {
        // Oscillate between the two maps: triangle 0..255..0 over HM_OSC_MS.
        uint32_t phase = timer_read32() % HM_OSC_MS;
        uint32_t half  = HM_OSC_MS / 2;
        *mix  = phase < half ? (uint8_t)(phase * 255 / half)
                             : (uint8_t)((HM_OSC_MS - phase) * 255 / half);
        *fade = 255;
        return;
    }
    if (since < (uint32_t)RGB_MATRIX_TIMEOUT) {       // 7s fade to black on LONG map
        uint32_t into = since - fade_start;
        *mix  = 255;
        *fade = (uint8_t)(255 - (into * 255 / HM_FADEOUT_MS));
        return;
    }
    *mix = 255; *fade = 0;
}

void heatmap_render(uint8_t led_min, uint8_t led_max) {
    uint8_t layer = get_highest_layer(layer_state);
    if (layer >= HM_LAYERS) layer = HM_LAYERS - 1;

    uint16_t ds = hm_denom(hm_short, layer, HM_CAP_SHORT);
    uint16_t dl = hm_denom(hm_long,  layer, HM_CAP_LONG);
    uint8_t  maxv = rgb_matrix_get_val();     // respect the brightness knob
    uint8_t  mix, fade;
    hm_idle(&mix, &fade);

    for (uint8_t i = led_min; i < led_max; i++) {
        if (i >= HM_LEDS) continue;
        uint16_t cs = hm_short[layer][i], cl = hm_long[layer][i];
        if (cs == 0 && cl == 0) {             // never pressed in either window -> off
            rgb_matrix_set_color(i, 0, 0, 0);
            continue;
        }
        // Blend the two windows' hotness by the idle mix factor.
        float ts = hm_norm(cs, ds), tl = hm_norm(cl, dl);
        float t  = ts + (tl - ts) * ((float)mix / 255.0f);

        hsv_t hsv = hm_color(t, maxv);
        // Perceptual gamma + global idle fade, then temporal dither the residual
        // so sub-LSB brightness still shows across frames (smooth low-end fades).
        uint16_t target = (uint16_t)hm_gamma[hsv.v] * fade;   // 0..255*255
        uint16_t acc    = target + hm_carry[i];
        hsv.v           = (uint8_t)(acc >> 8);
        hm_carry[i]     = (uint8_t)(acc & 0xFF);

        rgb_t rgb = hsv_to_rgb(hsv);
        rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
    }
}
