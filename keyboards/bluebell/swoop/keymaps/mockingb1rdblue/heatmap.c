// Copyright 2026 mockingb1rdblue
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Per-layer neon keypress heatmap (RGB Matrix custom effect backend).
//
// BEHAVIOR CONTRACT (operator spec 2026-06-11):
//   * Rolling window of the last HM_WINDOW (2000) key-DOWNs across the whole
//     keyboard, bucketed per (layer, LED). On layer change the effect simply
//     reads that layer's buckets, so each layer shows ITS OWN hot keys.
//   * Counting runs on BOTH halves from the mirrored matrix (0->1 edges, same
//     as bongo). SPLIT_TRANSPORT_MIRROR gives both sides the full matrix and
//     SPLIT_LAYER_STATE_ENABLE keeps the active layer in sync, so both halves
//     build IDENTICAL counters and each renders its own 18 LEDs correctly.
//     A process_record hook would only run on the master and leave the slave
//     half's heatmap blank -- hence matrix-edge counting instead.
//   * Color ramp by normalized hotness t in [0,1]:
//       t=0  -> brightness ZERO (off)         (least pressed / unpressed)
//       low  -> WHITE   (sat ramps up from 0)
//             -> YELLOW
//             -> CYAN
//       t=1  -> PURPLE-PINK at MAX brightness  (most pressed)
//     Brightness scales linearly with t; hue/sat walk the 4 stops above.
//   * MATURITY gate: no key reaches max until it has been pressed >= HM_MATURITY
//     (10) times. Implemented by flooring the normalize denominator at 10, so
//     nothing saturates until the hottest key crosses 10, then everything
//     normalizes against the real max.
//   * LATCH: once a key reaches full brightness it STAYS full (it does not fade
//     back as the rolling window evicts its presses). Only the idle animation
//     dims it. Latch state is per (layer, LED) and persists until power-cycle.
//   * IDLE animation (drives a global brightness multiplier; never touches hue):
//       < 500ms since last key      : ACTIVE, full brightness.
//       500ms .. (TIMEOUT-6s)       : BREATHING, 2s loop, 100% -> 50% -> 100%.
//       last 6s before TIMEOUT      : PRE-SLEEP -- 2s held at MAX, then a 4s
//                                     fade to ZERO, hitting 0 exactly as
//                                     RGB_MATRIX_SLEEP cuts the LEDs at TIMEOUT.
//     (TIMEOUT = RGB_MATRIX_TIMEOUT, 60s; activity ts synced via SPLIT_ACTIVITY_ENABLE.)

#include QMK_KEYBOARD_H
#include <stdbool.h>

#define HM_LAYERS   4              // _BASE.._ADJUST
#define HM_LEDS     RGB_MATRIX_LED_COUNT
#define HM_WINDOW   2000           // rolling key-DOWN history depth
#define HM_MATURITY 10             // presses before a key may hit max brightness

// Idle-animation timing (ms). Anchored to the RGB sleep timeout.
#define HM_ACTIVE_MS   500
#define HM_BREATHE_MS  2000        // breathing loop period
#define HM_PRESLEEP_MS 6000        // window before TIMEOUT that the send-off runs
#define HM_PS_HOLD_MS  2000        // ...held at max...
#define HM_PS_FADE_MS  4000        // ...then faded to zero (2000 + 4000 = 6000)

// Per-(layer, LED) rolling counts and the "reached full" latch.
static uint16_t hm_count[HM_LAYERS][HM_LEDS];
static bool     hm_latched[HM_LAYERS][HM_LEDS];

// Circular history of the last HM_WINDOW presses, each packed (layer<<6 | led).
// 0xFF == empty slot. led is 0..HM_LEDS-1 (<64) and layer 0..3, so 0xFF never
// collides with a real entry. Evicting the oldest decrements its bucket.
static uint8_t  hm_ring[HM_WINDOW];
static uint16_t hm_ring_head = 0;
static uint16_t hm_ring_fill = 0;
static bool     hm_ring_init = false;

// Previous matrix snapshot for 0->1 edge detection (independent of bongo's).
static matrix_row_t hm_prev[MATRIX_ROWS];

static void hm_push(uint8_t layer, uint8_t led) {
    if (hm_ring_fill == HM_WINDOW) {
        uint8_t old = hm_ring[hm_ring_head];
        if (old != 0xFF) {
            uint8_t ol = old >> 6, oe = old & 0x3F;
            if (ol < HM_LAYERS && oe < HM_LEDS && hm_count[ol][oe]) hm_count[ol][oe]--;
        }
    } else {
        hm_ring_fill++;
    }
    hm_ring[hm_ring_head] = (uint8_t)((layer << 6) | led);
    hm_ring_head          = (hm_ring_head + 1) % HM_WINDOW;
    if (hm_count[layer][led] < HM_WINDOW) hm_count[layer][led]++;
}

void heatmap_record_scan(void) {
    if (!hm_ring_init) { // one-time: mark all ring slots empty
        for (uint16_t i = 0; i < HM_WINDOW; i++) hm_ring[i] = 0xFF;
        hm_ring_init = true;
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
            hm_push(layer, led);
        }
    }
}

static uint8_t hm_lerp(uint8_t a, uint8_t b, uint8_t frac /*0..255*/) {
    return (uint8_t)(a + (((int16_t)b - (int16_t)a) * frac) / 255);
}

// Map hotness t (0..255) -> HSV. Hue/sat walk white->yellow->cyan->purple-pink
// over thirds of t; value rises linearly with t up to `maxv`.
static hsv_t hm_color(uint8_t t, uint8_t maxv) {
    // QMK hue space (0..255 == 0..360deg): yellow~43, cyan~128, purple-pink~222.
    uint8_t h, s;
    if (t <= 85) {                                   // white -> yellow
        uint8_t f = (uint8_t)((uint16_t)t * 255 / 85);
        h = 43;
        s = f;                                        // saturation 0 (white) -> 255
    } else if (t <= 170) {                            // yellow -> cyan
        uint8_t f = (uint8_t)((uint16_t)(t - 85) * 255 / 85);
        h = hm_lerp(43, 128, f);
        s = 255;
    } else {                                          // cyan -> purple-pink
        uint8_t f = (uint8_t)((uint16_t)(t - 170) * 255 / 85);
        h = hm_lerp(128, 222, f);
        s = 255;
    }
    uint8_t v = (uint8_t)((uint16_t)t * maxv / 255);  // brightness tracks t
    return (hsv_t){h, s, v};
}

// Global brightness multiplier (0..255) for the idle animation. Hue untouched.
static uint8_t hm_idle_mul(void) {
    uint32_t since = last_input_activity_elapsed();
    if (since < HM_ACTIVE_MS) return 255; // actively typing -> full

    if (since < (uint32_t)RGB_MATRIX_TIMEOUT - HM_PRESLEEP_MS) {
        // Breathing: triangle 0..255..0 over the loop, applied as 100%..50%..100%.
        uint32_t phase = timer_read32() % HM_BREATHE_MS;
        uint32_t half  = HM_BREATHE_MS / 2;
        uint32_t tri   = phase < half ? (phase * 255 / half)
                                      : ((HM_BREATHE_MS - phase) * 255 / half);
        return (uint8_t)(255 - tri / 2); // dip to 50%, never lower; no color change
    }

    if (since < (uint32_t)RGB_MATRIX_TIMEOUT) {
        // Pre-sleep send-off: hold max, then fade to zero by the timeout.
        uint32_t into = since - ((uint32_t)RGB_MATRIX_TIMEOUT - HM_PRESLEEP_MS);
        if (into < HM_PS_HOLD_MS) return 255;
        uint32_t f = into - HM_PS_HOLD_MS;
        if (f >= HM_PS_FADE_MS) return 0;
        return (uint8_t)(255 - (f * 255 / HM_PS_FADE_MS));
    }
    return 0; // at/after timeout RGB_MATRIX_SLEEP cuts the LEDs anyway
}

void heatmap_render(uint8_t led_min, uint8_t led_max) {
    uint8_t layer = get_highest_layer(layer_state);
    if (layer >= HM_LAYERS) layer = HM_LAYERS - 1;

    // Hottest count on this layer; floored at HM_MATURITY so nothing saturates
    // until some key has been pressed >= 10 times ("normalize from there").
    uint16_t mx = 0;
    for (uint8_t i = 0; i < HM_LEDS; i++) {
        if (hm_count[layer][i] > mx) mx = hm_count[layer][i];
    }
    uint16_t denom = mx < HM_MATURITY ? HM_MATURITY : mx;

    uint8_t maxv = rgb_matrix_get_val();      // respect the brightness knob
    uint8_t gmul = hm_idle_mul();

    for (uint8_t i = led_min; i < led_max; i++) {
        if (i >= HM_LEDS) continue;
        uint16_t c = hm_count[layer][i];

        // Unpressed AND never-latched -> fully off.
        if (c == 0 && !hm_latched[layer][i]) {
            rgb_matrix_set_color(i, 0, 0, 0);
            continue;
        }
        // Latch on reaching the (mature) top; latched keys render full forever.
        if (mx >= HM_MATURITY && c >= mx) hm_latched[layer][i] = true;

        uint8_t t = hm_latched[layer][i] ? 255
                                         : (uint8_t)((uint32_t)c * 255 / denom);
        hsv_t hsv = hm_color(t, maxv);
        hsv.v     = (uint8_t)((uint16_t)hsv.v * gmul / 255); // idle animation
        rgb_t rgb = hsv_to_rgb(hsv);
        rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
    }
}
