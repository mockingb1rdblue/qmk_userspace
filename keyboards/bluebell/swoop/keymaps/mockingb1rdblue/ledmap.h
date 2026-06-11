// Copyright 2026 mockingb1rdblue
// SPDX-License-Identifier: GPL-2.0-or-later
//
// "press-to-light" LED<->key mapping diagnostic (RGB Matrix custom effect
// backend). Public surface only; all state and the decay/color math live in
// ledmap.c. See that file's header for the behavior contract.
//
// PURPOSE: confirm the g_led_config.matrix_co[][] wiring against the physical
// board. Press a key -> THAT key's LED lights bright for ~1s with decay; left-
// half presses render in one hue, right-half presses in a distinct hue. All
// other LEDs OFF. The OLED echoes the last press as "rRcC -> LEDnn H".
#pragma once

#include <stdint.h>
#include <stdbool.h>

// Fold this scan's key-DOWN edges into the per-LED decay timers. Call once per
// matrix scan on BOTH halves (the mirrored matrix + synced layer state keep the
// two sides identical so each renders its own 18 LEDs). Cheap: O(MATRIX_ROWS).
void ledmap_record_scan(void);

// Paint LEDs [led_min, led_max): the most recently pressed keys glow + decay in
// their per-half hue, everything else off. Call from the custom RGB Matrix
// effect each render chunk.
void ledmap_render(uint8_t led_min, uint8_t led_max);

// Last key-DOWN seen, for the OLED readout. Returns false if nothing has been
// pressed yet (OLED should then show a placeholder). On true, fills the out
// params: matrix (row,col), resolved LED index, and is_left = true for a left-
// half press (LED index < RGB_MATRIX_LED_COUNT/2), false for right.
bool ledmap_last_press(uint8_t *row, uint8_t *col, uint8_t *led, bool *is_left);
