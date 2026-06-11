// Copyright 2026 mockingb1rdblue
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Custom RGB Matrix effect registration. Two custom effects live here:
//   * led_map     -- the boot mode: a press-to-light LED<->key mapping diagnostic
//                    (RGB_MATRIX_DEFAULT_MODE in config.h). Logic in ledmap.c.
//   * heatmap_neon -- the per-layer neon keypress heatmap (logic in heatmap.c),
//                    kept registered + compiled as a selectable mode.
// QMK includes this .inc twice -- once to build the effect enum, once (under
// RGB_MATRIX_CUSTOM_EFFECT_IMPLS) for the implementations.
RGB_MATRIX_EFFECT(led_map)
RGB_MATRIX_EFFECT(heatmap_neon)

#ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS
#    include "ledmap.h"
#    include "heatmap.h"

static bool led_map(effect_params_t* params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    ledmap_render(led_min, led_max);
    return rgb_matrix_check_finished_leds(led_max);
}

static bool heatmap_neon(effect_params_t* params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    heatmap_render(led_min, led_max);
    return rgb_matrix_check_finished_leds(led_max);
}
#endif // RGB_MATRIX_CUSTOM_EFFECT_IMPLS
