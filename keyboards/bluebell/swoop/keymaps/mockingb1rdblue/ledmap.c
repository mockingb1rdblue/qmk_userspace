// Copyright 2026 mockingb1rdblue
// SPDX-License-Identifier: GPL-2.0-or-later
//
// "press-to-light" LED<->key mapping diagnostic (RGB Matrix custom effect).
//
// BEHAVIOR CONTRACT (operator spec 2026-06-11):
//   * PURPOSE: a ground-truth probe of g_led_config.matrix_co[r][c]. The vendor
//     PCB's true ws2812 chain order is UNKNOWN; this mode makes the wiring
//     directly observable -- press a key, watch which LED lights. If the lit LED
//     is NOT under the pressed key, the matrix_co mapping in keymap.c needs a
//     remap, and this mode tells you exactly which way.
//   * DETECTION: key-DOWN matrix edges, detected the SAME way heatmap.c does --
//     0->1 bit transitions vs a private previous-scan snapshot (cur & ~prev),
//     then g_led_config.matrix_co[r][c] -> LED index. Runs on BOTH halves; the
//     mirrored matrix (SPLIT_TRANSPORT_MIRROR) lets each half see every key, so
//     both sides build identical decay state and each paints its own 18 LEDs.
//   * RENDER: on each key-down the struck LED is set to full and a per-LED decay
//     timer is (re)armed for LM_HOLD_MS (~1s). Brightness decays linearly to
//     zero over that window. ALL non-active LEDs are OFF (this is a diagnostic,
//     not an ambient effect -- a dark field makes the single lit LED obvious).
//   * HUE BY HALF: an LED in the left bank (index < LEDS/2) glows LM_HUE_LEFT;
//     the right bank glows LM_HUE_RIGHT. So the operator confirms per-half: a
//     left-key press should light a left-hue LED on the LEFT board's own LEDs.
//   * BRIGHTNESS: peak value tracks rgb_matrix_get_val() (the brightness knob).
//   * SLEEP/TIMEOUT: handled by the RGB Matrix core (RGB_MATRIX_SLEEP +
//     RGB_MATRIX_TIMEOUT in config.h) -- when idle the core stops calling the
//     effect and blanks the LEDs, same as every other mode. The decay timers
//     simply expire; nothing here fights the core's sleep.
//   * OLED: ledmap_last_press() exposes the last key-down (row, col, LED, half)
//     so keymap.c's oled_task_user can print "rRcC -> LEDnn H" when this mode is
//     active. Best-effort readout; the LED behavior above is the must-have.

#include QMK_KEYBOARD_H
#include "ledmap.h"

#define LM_LEDS    RGB_MATRIX_LED_COUNT
#define LM_HALF    (RGB_MATRIX_LED_COUNT / 2)   // LEDs [0,HALF) left, [HALF,..) right
#define LM_HOLD_MS 1000                         // glow-then-decay window per press

// Distinct hues per half so the operator can tell which bank lit. QMK HSV hue is
// 0..255: 0 = red-ish (LEFT), 170 = blue (RIGHT). Both are vivid and unmistakably
// different so a left-vs-right wiring swap is obvious at a glance.
#define LM_HUE_LEFT  0
#define LM_HUE_RIGHT 170
#define LM_SAT       255

// Per-LED time-of-last-press (timer_read32 ticks). 0 == never / expired.
static uint32_t lm_hit[LM_LEDS];

// Private previous-matrix snapshot for 0->1 edge detection (independent of
// heatmap's and bongo's snapshots; each subsystem keeps its own).
static matrix_row_t lm_prev[MATRIX_ROWS];

// Last key-DOWN, for the OLED readout. lm_seen stays false until the first press.
static bool    lm_seen = false;
static uint8_t lm_row  = 0, lm_col = 0, lm_led = 0;
static bool    lm_left = false;

void ledmap_record_scan(void) {
    for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
        matrix_row_t cur   = matrix_get_row(r);
        matrix_row_t newly = cur & ~lm_prev[r]; // bits that turned ON this scan
        lm_prev[r]         = cur;
        if (!newly) continue;
        for (uint8_t c = 0; c < MATRIX_COLS; c++) {
            if (!(newly & ((matrix_row_t)1 << c))) continue;
            uint8_t led = g_led_config.matrix_co[r][c];
            if (led == NO_LED || led >= LM_LEDS) continue;
            lm_hit[led] = timer_read32();
            // Latch this press for the OLED. timer 0 is a valid "just now" value,
            // so force a tiny nonzero floor to keep 0 strictly meaning "expired".
            if (lm_hit[led] == 0) lm_hit[led] = 1;
            lm_seen = true;
            lm_row  = r;
            lm_col  = c;
            lm_led  = led;
            lm_left = (led < LM_HALF);
        }
    }
}

bool ledmap_last_press(uint8_t *row, uint8_t *col, uint8_t *led, bool *is_left) {
    if (!lm_seen) return false;
    if (row)     *row     = lm_row;
    if (col)     *col     = lm_col;
    if (led)     *led     = lm_led;
    if (is_left) *is_left = lm_left;
    return true;
}

void ledmap_render(uint8_t led_min, uint8_t led_max) {
    uint8_t maxv = rgb_matrix_get_val();   // respect the brightness knob

    for (uint8_t i = led_min; i < led_max; i++) {
        if (i >= LM_LEDS) continue;
        uint32_t hit = lm_hit[i];
        if (hit == 0) {                    // never pressed / already expired -> off
            rgb_matrix_set_color(i, 0, 0, 0);
            continue;
        }
        uint32_t age = timer_elapsed32(hit);
        if (age >= LM_HOLD_MS) {           // window elapsed -> expire and go dark
            lm_hit[i] = 0;
            rgb_matrix_set_color(i, 0, 0, 0);
            continue;
        }
        // Linear decay full -> 0 across LM_HOLD_MS. age < LM_HOLD_MS here, so the
        // product stays in [0, 255*maxv] and v lands in [0, maxv].
        uint8_t v = (uint8_t)(((uint32_t)(LM_HOLD_MS - age) * maxv) / LM_HOLD_MS);
        uint8_t h = (i < LM_HALF) ? LM_HUE_LEFT : LM_HUE_RIGHT;

        hsv_t hsv = {h, LM_SAT, v};
        rgb_t rgb = hsv_to_rgb(hsv);
        rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
    }
}
