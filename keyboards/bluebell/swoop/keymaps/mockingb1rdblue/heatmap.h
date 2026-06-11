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
