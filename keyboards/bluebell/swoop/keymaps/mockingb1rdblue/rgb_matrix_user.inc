// Copyright 2026 mockingb1rdblue
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Custom RGB Matrix effect registration. ONE AND ONLY effect (operator lock-in
// 2026-06-12):
//   * heatmap_neon -- the per-layer neon keypress heatmap (logic in heatmap.c).
// The press-to-light LED<->key mapping diagnostic (led_map / ledmap.c) was a
// bring-up tool; its output is now baked into g_led_config (keymap.c) and the
// effect has been RETIRED so the heatmap is the sole RGB pattern. ledmap.c/.h
// remain on disk as the documented chain-mapping record but are NOT compiled.
// QMK includes this .inc twice -- once to build the effect enum, once (under
// RGB_MATRIX_CUSTOM_EFFECT_IMPLS) for the implementations.
RGB_MATRIX_EFFECT(heatmap_neon)

#ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS
#    include "heatmap.h"

static bool heatmap_neon(effect_params_t* params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    heatmap_render(led_min, led_max);
    return rgb_matrix_check_finished_leds(led_max);
}
#endif // RGB_MATRIX_CUSTOM_EFFECT_IMPLS
