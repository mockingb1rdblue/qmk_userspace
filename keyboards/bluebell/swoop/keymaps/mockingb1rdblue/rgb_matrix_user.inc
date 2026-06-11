// Copyright 2026 mockingb1rdblue
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Custom RGB Matrix effect registration. The neon per-layer keypress heatmap
// is the keymap's default mode (RGB_MATRIX_DEFAULT_MODE in config.h). All logic
// lives in heatmap.c; this file just wires it to the effect dispatcher. QMK
// includes this .inc twice -- once to build the effect enum, once (under
// RGB_MATRIX_CUSTOM_EFFECT_IMPLS) for the implementation.
RGB_MATRIX_EFFECT(heatmap_neon)

#ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS
#    include "heatmap.h"

static bool heatmap_neon(effect_params_t* params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    heatmap_render(led_min, led_max);
    return rgb_matrix_check_finished_leds(led_max);
}
#endif // RGB_MATRIX_CUSTOM_EFFECT_IMPLS
