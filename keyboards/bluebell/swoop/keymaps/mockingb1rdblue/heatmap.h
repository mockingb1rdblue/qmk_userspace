// Copyright 2026 mockingb1rdblue
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Per-layer neon keypress heatmap for RGB Matrix. Public surface only; all
// state and the color/idle math live in heatmap.c. See that file's header for
// the behavior contract (rolling window, color ramp, maturity gate, latch,
// breathing + pre-sleep animation).
#pragma once

#include <stdint.h>

// Fold this scan's key-DOWN edges into the rolling per-layer counters. Call
// once per matrix scan on BOTH halves (the mirrored matrix + synced layer
// state keep the two sides' counters identical). Cheap: O(MATRIX_ROWS).
void heatmap_record_scan(void);

// Paint LEDs [led_min, led_max) for the active layer's heatmap. Call from the
// custom RGB Matrix effect each render chunk.
void heatmap_render(uint8_t led_min, uint8_t led_max);

// Milliseconds since the last key-DOWN edge (0->1) seen by heatmap_record_scan,
// on THIS half. Unlike QMK's last_input_activity_elapsed() (which also resets on
// key-UP / any matrix change), this counts presses ONLY -- releases never reset
// it. Both the heatmap idle wave and the bongo pacing run off this so neither
// "wakes" on a key release. Both halves track it independently off the mirrored
// matrix, so they stay aligned without an explicit split sync.
uint32_t heatmap_last_keydown_elapsed(void);

// Persistence (counts survive power-down/sleep via the wear-leveled user EEPROM
// datablock; see heatmap.c). Call heatmap_init() once from keyboard_post_init_user
// (restores the saved map), heatmap_persist_task() from housekeeping_task_user
// (throttled dirty-save), and heatmap_persist_save_now() from
// suspend_power_down_user (flush before the host cuts power).
void heatmap_init(void);
void heatmap_persist_task(void);
void heatmap_persist_save_now(void);
