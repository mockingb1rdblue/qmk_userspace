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
//   * IDLE (drives the per-key SHORT<->LONG mix + a global fade; never distorts hue).
//     Time is measured from the last key-DOWN only (heatmap_last_keydown_elapsed),
//     so a key RELEASE never restarts the idle clock (operator 2026-06-23):
//       < HM_ACTIVE_MS (5s)             : ACTIVE -> render the SHORT (rolling-100) map.
//       5s .. (TIMEOUT - 7s)            : STAGGERED WAVE -- each LED crossfades
//                                         SHORT<->LONG on the trapezoid, phase-
//                                         delayed by rank*HM_STAGGER_MS (rank by
//                                         SHORT usage, hottest first). 11s loop.
//       last 7s before TIMEOUT          : hold the LONG map and FADE to zero,
//                                         hitting 0 as RGB_MATRIX_SLEEP cuts LEDs.
//   * SMOOTHNESS: value goes through a gamma-2.2 LUT (perceptual) plus per-LED
//     temporal dithering (fractional carry across frames) so low-brightness fades
//     don't band on the 8-bit ws2812. Target framerate ~38Hz (RGB_MATRIX_LED_FLUSH_LIMIT).
//
// rev-15: hardened only (behavior identical) -- defensive float->uint8 clamps in
//     hm_color so a boundary rounding error can't wrap a 256 to a small value;
//     explicit overflow-bound comments in the gamma/dither path and ring packing.

#include QMK_KEYBOARD_H
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "eeconfig.h"

#define HM_LAYERS    4              // _BASE.._ADJUST
#define HM_LEDS      RGB_MATRIX_LED_COUNT
#define HM_SHORT     100            // recent-use window depth
#define HM_LONG      2000           // long-term window depth
#define HM_DENOM_MIN 35             // denom floor -> ~3 presses == yellow cold
#define HM_CAP_SHORT 100            // short denom cap (goes solid by ~100)
#define HM_CAP_LONG  400            // long denom cap
#define HM_K         15.0f          // log-curve steepness (Sonar)

// Idle-animation timing (ms). Anchored to the RGB sleep timeout (60s).
#define HM_ACTIVE_MS   5000         // static SHORT map this long after last press,
                                    // then the idle wave begins (operator 2026-06-23)
// SHORT<->LONG idle oscillation is a TRAPEZOID, not a triangle (operator
// 2026-06-22): ramp one way over HM_OSC_RAMP_MS, then DWELL at the fully
// resolved map for HM_OSC_HOLD_MS before reversing -- so each map sits still
// and readable instead of instantly bouncing back. Full loop period is
// 2*ramp + 2*hold.
//
// STAGGERED WAVE (operator 2026-06-23): the crossfade is no longer global. Each
// LED runs the SAME trapezoid but with its phase delayed by (rank * HM_STAGGER_MS),
// where rank is the LED's position in the SHORT-window usage order (hottest =
// rank 0). So the SHORT<->LONG crossfade sweeps across the keys as a wave led by
// the most-recently-used keys -- "a rainbow wave, but ordered by heat not
// position." Each key still fades to its LONG (2000-press) appearance and back,
// NOT to off. 36 keys * 20ms = 700ms of spread inside the 11s loop.
#define HM_OSC_RAMP_MS 5000         // crossfade ramp, each direction (5s cos/sin fade)
#define HM_OSC_HOLD_MS 500          // dwell at each fully-resolved map
#define HM_OSC_MS      (2 * HM_OSC_RAMP_MS + 2 * HM_OSC_HOLD_MS)
#define HM_STAGGER_MS  20           // per-rank phase delay of the wave
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

// Timestamp of the last key-DOWN edge (key presses ONLY; releases never bump it).
// Both the idle wave (hm_idle) and bongo read this via heatmap_last_keydown_elapsed
// instead of QMK's last_input_activity_elapsed, which also resets on key-up.
// At boot it reads ~0ms-elapsed, so the restored map shows bright (ACTIVE) for
// up to HM_ACTIVE_MS before the idle wave starts -- harmless.
static uint32_t hm_last_keydown = 0;

// Perceptual gamma LUT + per-LED temporal-dither carry (1/256ths of a level).
static uint8_t hm_gamma[256];
static uint8_t hm_carry[HM_LEDS];

// Baked sine (raised-cosine) ease-in-out curve for the SHORT<->LONG ramp:
// hm_ease[i] = round(255 * (1 - cos(pi * i/255)) / 2), i in 0..255. Velocity is
// zero at both ends (elongated curve at the extremes) and steepest/most-linear
// through the middle. Built once at init so the per-frame path stays integer
// (no cosf on the FPU-less M0+).
static uint8_t hm_ease[256];

// ---------------------------------------------------------------------------
// PERSISTENCE (operator 2026-06-22). The rolling counts above are pure RAM, so
// a power-down wiped the whole heatmap -- and a host SLEEP is exactly that on
// this bus-powered split: VBUS drops, the MCU loses power, RAM clears. (The RGB
// *mode* survives sleep only because RGB_MATRIX_SLEEP merely blanks the LEDs
// while still powered; that is a different, non-power-down case.)
//
// Fix: persist the per-(layer,LED) COUNTS for both windows to the wear-leveled
// user EEPROM datablock and restore on boot. We store counts only (580 B), NOT
// the 2000-entry ring -- the ring would need an EEPROM resize and far more flash
// wear. On restore the rings are rebuilt "full" from the counts (sum of counts
// == live presses <= depth, so it always fits), so the rolling window keeps
// evicting normally. The rendered map is EXACT at restore and decays naturally
// afterward; only the precise eviction order of old presses is approximated.
//
// Save cadence: on host suspend (suspend_power_down_user -- the sleep path that
// caused the loss) AND a throttled dirty-save every HM_SAVE_INTERVAL_MS, so an
// abrupt unplug (no clean suspend, and the slave half has no USB to detect one)
// loses at most that window. eeconfig_update_user_datablock compares before
// writing, so an unchanged map costs zero flash cycles.
#define HM_PERSIST_MAGIC    0x484D0001u  // 'HM' + schema rev 1
#define HM_SAVE_INTERVAL_MS 300000u      // 5 min throttled dirty-save (unplug guard)

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t s[HM_LAYERS][HM_LEDS];
    uint16_t l[HM_LAYERS][HM_LEDS];
} hm_persist_t;

_Static_assert(sizeof(hm_persist_t) == EECONFIG_USER_DATA_SIZE,
               "EECONFIG_USER_DATA_SIZE (config.h) must equal sizeof(hm_persist_t)");

static bool     hm_dirty     = false;  // counts changed since last save
static uint32_t hm_last_save = 0;

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
    // Pack (layer<<6 | led): layer 0..3 (2 bits), led 0..HM_LEDS-1 (<=35, 6 bits).
    // Max packed value is (3<<6)|35 == 0xE3, so a live entry can NEVER equal the
    // 0xFF empty sentinel (that would need led==63). The underflow guard above
    // (cnt[ol][oe] only decremented when nonzero) keeps counts >= 0.
    ring[*head] = (uint8_t)((layer << 6) | led);
    *head       = (uint16_t)((*head + 1) % depth);
    if (cnt[layer][led] < depth) cnt[layer][led]++;
}

// Zero the rings (0xFF = empty) and build the perceptual gamma LUT. Idempotent;
// runs once, either from heatmap_init() at boot or lazily on the first scan.
static void hm_lazy_init(void) {
    for (uint16_t i = 0; i < HM_SHORT; i++) hm_ring_s[i] = 0xFF;
    for (uint16_t i = 0; i < HM_LONG; i++)  hm_ring_l[i] = 0xFF;
    for (uint16_t i = 0; i < 256; i++) {
        float x = (float)i / 255.0f;
        hm_gamma[i] = (uint8_t)lroundf(powf(x, 1.0f / 2.2f) * 255.0f);
        // Sine ease-in-out, baked: flat at the ends, linear-ish in the middle.
        hm_ease[i]  = (uint8_t)lroundf((1.0f - cosf(3.14159265f * x)) * 0.5f * 255.0f);
    }
    hm_init = true;
}

// Rebuild a ring "full" from per-(layer,LED) counts so eviction keeps working
// after a restore. sum(counts) == live presses <= depth, so it always fits.
static void hm_rebuild_ring(uint8_t *ring, uint16_t depth, uint16_t *head,
                            uint16_t *fill, uint16_t (*cnt)[HM_LEDS]) {
    uint16_t idx = 0;
    for (uint8_t ly = 0; ly < HM_LAYERS && idx < depth; ly++) {
        for (uint8_t e = 0; e < HM_LEDS && idx < depth; e++) {
            for (uint16_t n = 0; n < cnt[ly][e] && idx < depth; n++) {
                ring[idx++] = (uint8_t)((ly << 6) | e);
            }
        }
    }
    for (uint16_t i = idx; i < depth; i++) ring[i] = 0xFF;
    *fill = idx;
    *head = (uint16_t)(idx % depth);
}

static void hm_persist_save(void) {
    hm_persist_t blk;
    blk.magic = HM_PERSIST_MAGIC;
    memcpy(blk.s, hm_short, sizeof(blk.s));
    memcpy(blk.l, hm_long,  sizeof(blk.l));
    eeconfig_update_user_datablock(&blk, 0, sizeof(blk)); // no-op when unchanged
    hm_dirty     = false;
    hm_last_save = timer_read32();
}

static void hm_persist_restore(void) {
    if (!eeconfig_is_user_datablock_valid()) return;      // fresh/blank EEPROM
    hm_persist_t blk;
    eeconfig_read_user_datablock(&blk, 0, sizeof(blk));
    if (blk.magic != HM_PERSIST_MAGIC) return;            // schema mismatch -> ignore
    memcpy(hm_short, blk.s, sizeof(blk.s));
    memcpy(hm_long,  blk.l, sizeof(blk.l));
    hm_rebuild_ring(hm_ring_s, HM_SHORT, &hm_head_s, &hm_fill_s, hm_short);
    hm_rebuild_ring(hm_ring_l, HM_LONG,  &hm_head_l, &hm_fill_l, hm_long);
}

void heatmap_init(void) {
    if (!hm_init) hm_lazy_init();
    hm_persist_restore();
    hm_last_save = timer_read32();
}

void heatmap_persist_task(void) {
    if (hm_dirty && timer_elapsed32(hm_last_save) >= HM_SAVE_INTERVAL_MS) {
        hm_persist_save();
    }
}

void heatmap_persist_save_now(void) {
    if (hm_dirty) hm_persist_save();
}

void heatmap_record_scan(void) {
    if (!hm_init) hm_lazy_init();
    uint8_t layer = get_highest_layer(layer_state);
    if (layer >= HM_LAYERS) layer = HM_LAYERS - 1;

    for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
        matrix_row_t cur   = matrix_get_row(r);
        matrix_row_t newly = cur & ~hm_prev[r]; // bits that turned ON this scan
        hm_prev[r]         = cur;
        if (!newly) continue;
        hm_last_keydown = timer_read32();        // key-DOWN only -> idle/wake clock
        for (uint8_t c = 0; c < MATRIX_COLS; c++) {
            if (!(newly & ((matrix_row_t)1 << c))) continue;
            uint8_t led = g_led_config.matrix_co[r][c];
            if (led == NO_LED || led >= HM_LEDS) continue;
            hm_push_ring(hm_ring_s, HM_SHORT, &hm_head_s, &hm_fill_s, hm_short, layer, led);
            hm_push_ring(hm_ring_l, HM_LONG,  &hm_head_l, &hm_fill_l, hm_long,  layer, led);
            hm_dirty = true;  // counts changed -> schedule a persist
        }
    }
}

uint32_t heatmap_last_keydown_elapsed(void) {
    return timer_elapsed32(hm_last_keydown);
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

// Clamp a [0,255]-intended float to 0..255 *before* the uint8 cast, so a
// boundary rounding error (e.g. t/0.30f landing at 255.0000x or 256.0 from float
// representation) can never wrap a near-256 value down to a small one.
static uint8_t hm_u8(float x) {
    if (x < 0.0f) return 0;
    if (x > 255.0f) return 255;
    return (uint8_t)x;
}

// Hotness t (0..1) -> HSV. white->yellow->cyan->purple-pink; value tracks t.
static hsv_t hm_color(float t, uint8_t maxv) {
    if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f;
    uint8_t h, s;
    if (t <= 0.30f) {                         // white -> yellow
        h = 43;
        s = hm_u8(t / 0.30f * 255.0f);        // sat 0 (white) -> 255 (yellow)
    } else if (t <= 0.63f) {                  // yellow -> cyan
        uint8_t f = hm_u8((t - 0.30f) / 0.33f * 255.0f);
        h = hm_lerp(43, 128, f);
        s = 255;
    } else {                                  // cyan -> purple-pink
        uint8_t f = hm_u8((t - 0.63f) / 0.37f * 255.0f);
        h = hm_lerp(128, 222, f);
        s = 255;
    }
    uint8_t v = hm_u8(t * (float)maxv);       // t<=1, maxv<=255 -> already in range
    return (hsv_t){h, s, v};
}

// Idle regimes (return value), with the window-mix base phase + a global
// brightness fade as out-params:
#define HM_REGIME_ACTIVE  0   // pure SHORT, full bright (just typed)
#define HM_REGIME_WAVE    1   // staggered SHORT<->LONG crossfade (*phase valid)
#define HM_REGIME_FADE    2   // hold LONG, fading to black before sleep
#define HM_REGIME_SLEEP   3   // fully faded out

// Trapezoid SHORT<->LONG mix (0 = SHORT, 255 = LONG) for one phase in
// [0, HM_OSC_MS). Both ramps are sine ease-in-out (baked hm_ease LUT): velocity
// zero at the ends, near-linear through the middle.
//   [0, ramp)                ramp SHORT -> LONG  (eased)
//   [ramp, ramp+hold)        DWELL on LONG  (still & readable)
//   [ramp+hold, 2ramp+hold)  ramp LONG -> SHORT  (eased)
//   [2ramp+hold, period)     DWELL on SHORT (still & readable)
static uint8_t hm_mix_at(uint32_t phase) {
    if (phase < HM_OSC_RAMP_MS) {
        return hm_ease[phase * 255 / HM_OSC_RAMP_MS];
    } else if (phase < HM_OSC_RAMP_MS + HM_OSC_HOLD_MS) {
        return 255;
    } else if (phase < 2 * HM_OSC_RAMP_MS + HM_OSC_HOLD_MS) {
        uint32_t into = phase - (HM_OSC_RAMP_MS + HM_OSC_HOLD_MS);
        return 255 - hm_ease[into * 255 / HM_OSC_RAMP_MS];
    }
    return 0;
}

// Decide the idle regime from key-DOWN-only elapsed time. For HM_REGIME_WAVE the
// caller offsets *phase per-LED by rank*HM_STAGGER_MS (see heatmap_render); for
// the other regimes the mix is uniform (handled by the caller). *fade is the
// global brightness multiplier (0..255), only < 255 in the pre-sleep fade.
static uint8_t hm_idle(uint32_t *phase, uint8_t *fade) {
    uint32_t since = heatmap_last_keydown_elapsed();
    *fade = 255;
    if (since < HM_ACTIVE_MS) return HM_REGIME_ACTIVE;

    uint32_t fade_start = (uint32_t)RGB_MATRIX_TIMEOUT - HM_FADEOUT_MS;
    if (since < fade_start) {
        *phase = timer_read32() % HM_OSC_MS;
        return HM_REGIME_WAVE;
    }
    if (since < (uint32_t)RGB_MATRIX_TIMEOUT) {       // 7s fade to black on LONG map
        // Branch guard + fade_start def => into in [0, HM_FADEOUT_MS), so
        // into*255/HM_FADEOUT_MS is in [0,254] and fade stays in [1,255]
        // (no unsigned wrap). into*255 max ~1.78e6 fits uint32_t.
        uint32_t into = since - fade_start;
        *fade = (uint8_t)(255 - (into * 255 / HM_FADEOUT_MS));
        return HM_REGIME_FADE;
    }
    *fade = 0;
    return HM_REGIME_SLEEP;
}

// Per-LED wave-phase offset by SHORT-usage rank, recomputed once per frame.
// rank 0 = hottest recent key (leads the wave); each colder key lags
// HM_STAGGER_MS more. Indexed by LED.
static uint16_t hm_rank_off[HM_LEDS];

// Build hm_rank_off for `layer`: an LED's rank = how many LEDs have a STRICTLY
// greater SHORT count (ties share a rank). Offset = rank * HM_STAGGER_MS. O(n^2)
// over <=36 LEDs -- trivial at the ~38Hz render rate.
static void hm_build_ranks(uint8_t layer) {
    for (uint8_t i = 0; i < HM_LEDS; i++) {
        uint16_t ci = hm_short[layer][i];
        uint16_t rank = 0;
        for (uint8_t j = 0; j < HM_LEDS; j++) {
            if (hm_short[layer][j] > ci) rank++;
        }
        hm_rank_off[i] = (uint16_t)(rank * HM_STAGGER_MS);
    }
}

void heatmap_render(uint8_t led_min, uint8_t led_max) {
    uint8_t layer = get_highest_layer(layer_state);
    if (layer >= HM_LAYERS) layer = HM_LAYERS - 1;

    uint16_t ds = hm_denom(hm_short, layer, HM_CAP_SHORT);
    uint16_t dl = hm_denom(hm_long,  layer, HM_CAP_LONG);
    uint8_t  maxv = rgb_matrix_get_val();     // respect the brightness knob
    uint32_t phase = 0;
    uint8_t  fade;
    uint8_t  regime = hm_idle(&phase, &fade);

    // The wave needs each LED's SHORT-usage rank; rebuild it once per frame
    // (the effect renders in led_min..led_max chunks -- only the i==led_min<=0
    // chunk, i.e. the frame's first, recomputes).
    if (regime == HM_REGIME_WAVE && led_min == 0) hm_build_ranks(layer);

    for (uint8_t i = led_min; i < led_max; i++) {
        if (i >= HM_LEDS) continue;
        uint16_t cs = hm_short[layer][i], cl = hm_long[layer][i];
        if (cs == 0 && cl == 0) {             // never pressed in either window -> off
            rgb_matrix_set_color(i, 0, 0, 0);
            continue;
        }
        // Per-LED SHORT<->LONG mix (0=SHORT, 255=LONG): pure SHORT while active,
        // the staggered trapezoid wave mid-idle, full LONG during the pre-sleep
        // fade. The wave delays each LED's phase by its rank offset so the
        // crossfade sweeps the keys hottest-first.
        uint8_t mix;
        if (regime == HM_REGIME_WAVE) {
            uint32_t kp = (phase + HM_OSC_MS - hm_rank_off[i]) % HM_OSC_MS;
            mix = hm_mix_at(kp);
        } else {
            mix = (regime == HM_REGIME_ACTIVE) ? 0 : 255;
        }
        // Blend the two windows' hotness by the idle mix factor.
        float ts = hm_norm(cs, ds), tl = hm_norm(cl, dl);
        float t  = ts + (tl - ts) * ((float)mix / 255.0f);

        hsv_t hsv = hm_color(t, maxv);
        // Perceptual gamma + global idle fade, then temporal dither the residual
        // so sub-LSB brightness still shows across frames (smooth low-end fades).
        // Bounds (all fit uint16_t, max 65535): hm_gamma[hsv.v] in 0..255, fade in
        // 0..255 -> target in 0..65025; +hm_carry[i] (0..255) -> acc in 0..65280.
        // acc>>8 in 0..255 (high byte), acc&0xFF in 0..255 (fractional carry).
        uint16_t target = (uint16_t)hm_gamma[hsv.v] * fade;   // 0..65025
        uint16_t acc    = target + hm_carry[i];               // 0..65280
        hsv.v           = (uint8_t)(acc >> 8);                // 0..255
        hm_carry[i]     = (uint8_t)(acc & 0xFF);              // 0..255

        rgb_t rgb = hsv_to_rgb(hsv);
        rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
    }
}
