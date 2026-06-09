// Copyright 2026 mockingb1rdblue
// SPDX-License-Identifier: GPL-2.0-or-later
//
// TurkeyBoards Swoop / Sea-Picro (RP2040): DAILY-DRIVER keymap.
// Compiles via upstream bluebell/swoop + CONVERT_TO=promicro_rp2040 (see rules.mk).
//
// Layout/layers follow the operator's OWN reviung34 keymap
// (qmk_userspace/keyboards/reviung/reviung34/keymaps/mockingb1rdblue), NOT the
// gtips upstream default. ENTER is a gboards COMBO (jk), not a pinky home key; the
// right pinky home key is KC_QUOT. Layers below are the operator's 4 layers ported
// 1:1 onto the 36-key LAYOUT_split_3x5_3 (30 alphas map straight across; reviung34
// has 4 thumbs, swoop has 4 real thumb switches + 2 encoder pushes).
//
// Layer mapping (enum alias -> operator reviung34 layer index):
//   _BASE   -> [0] alpha/base
//   _LOWER  -> [1] nav + numpad + redo  (edit-oriented; left encoder = edit)
//   _RAISE  -> [2] symbols + page-nav   (right encoder = page nav)
//   _ADJUST -> [3] function row
//
// Hardware ground truth (probe run 2026-06-09, see README "CONFIRMED HARDWARE FACTS"):
//   * Encoders are ALIVE on B4/B5 (GP8/GP9). Driver re-armed; encoder_map below.
//   * Each hand has 2 real thumb switches + 1 ENCODER PUSH (the OUTER thumb cell).
//       LEFT : push [3,2], real [3,3] [3,4]      RIGHT: real [7,3] [7,4], push [7,2]
//     The 4 real switches carry the operator's reviung34 thumb functions; the 2
//     encoder pushes carry the browser/edit binds (WBAK/WFWD on base, redo on lower).
//
// Three hardware-bring-up subsystems are enabled here for first-flash confirmation
// (tracked as WyrdWeaver operator tasks, group mock1ngbboards):
//   * RGB_MATRIX  - per-key ws2812 (PIO) on D3/GP0, 36 LEDs split 18/18. g_led_config
//                   below is derived from keyboard.json layout coords; the vendor
//                   PCB's actual LED wiring order is UNKNOWN and may need a remap.
//   * OLED        - 128x32 SSD1306 over I2C1. PINS ARE A BEST-GUESS (SDA=GP2/D1,
//                   SCL=GP3/D0); confirm against the physical board.

#include QMK_KEYBOARD_H
#include "keymap_combo.h" // gboards combos, http://combos.gboards.ca/docs/install/

enum layers {
    _BASE,
    _LOWER,
    _RAISE,
    _ADJUST,
};

// Thumb-cluster arg order in LAYOUT_split_3x5_3 is: k32 k33 k34  k74 k73 k72.
//   LEFT : [3,2]=encoder push (outer)  [3,3]=real (outer)  [3,4]=real (inner)
//   RIGHT: [7,4]=real (inner)  [7,3]=real (outer)  [7,2]=encoder push (outer)
// Operator reviung34 thumbs: left inner=KC_SPC, left outer=KC_LCTL;
//   right inner=LT(2,KC_HOME), right outer=KC_LGUI.
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    // [0] base. Right pinky home = KC_QUOT (ENTER is the jk combo). Left inner
    // bottom alpha is LT(1,KC_B) for the lower layer.
    [_BASE] = LAYOUT_split_3x5_3(
        KC_Q,         KC_W, KC_E, KC_R, KC_T,                  KC_Y, KC_U, KC_I,    KC_O,           KC_P,
        KC_A,         KC_S, KC_D, KC_F, KC_G,                  KC_H, KC_J, KC_K,    KC_L,           KC_QUOT,
        LSFT_T(KC_Z), KC_X, KC_C, KC_V, LT(_LOWER, KC_B),      KC_N, KC_M, KC_COMM, RALT_T(KC_DOT), KC_SLSH,
                 KC_WBAK, KC_LCTL, KC_SPC,             LT(_RAISE, KC_HOME), KC_LGUI, KC_WFWD
    ),

    // [1] nav + numpad + redo. MO(3) on left ring (was reviung KC_F position).
    [_LOWER] = LAYOUT_split_3x5_3(
        KC_1,       KC_2,    KC_3,    KC_4,        KC_5,           KC_6,    KC_7, KC_8, KC_9, KC_0,
        KC_UP,      KC_LEFT, KC_DOWN, KC_RGHT,     LCTL(KC_Y),     KC_EQL,  KC_4, KC_5, KC_6, KC_PPLS,
        LSFT(KC_Z), KC_NO,   KC_NO,   MO(_ADJUST), KC_TRNS,        KC_UNDS, KC_1, KC_2, KC_3, KC_PMNS,
                 C(KC_Y), KC_TRNS, KC_TRNS,                KC_END, KC_0, KC_TRNS
    ),

    // [2] symbols + page-nav. MO(3) on the right inner thumb (was reviung 2u-right).
    [_RAISE] = LAYOUT_split_3x5_3(
        KC_EXLM, KC_AT, KC_HASH, KC_DLR,      KC_PERC,           KC_CIRC, KC_AMPR, KC_ASTR, KC_LPRN, KC_RPRN,
        KC_PGUP, KC_NO, KC_PGDN, KC_NO,       KC_NO,             KC_EQL,  KC_MINS, KC_NO,   KC_LBRC, KC_RBRC,
        KC_LSFT, KC_NO, KC_NO,   TG(_ADJUST), KC_TRNS,           KC_UNDS, KC_PLUS, KC_NO,   KC_LCBR, KC_RCBR,
                 KC_TRNS, KC_TRNS, MO(_ADJUST),          KC_TRNS, KC_TRNS, KC_TRNS
    ),

    // [3] function row.
    [_ADJUST] = LAYOUT_split_3x5_3(
        KC_F1, KC_F2, KC_F3, KC_F4, KC_F5,                      KC_F6,  KC_F7,  KC_F8, KC_F9,   KC_F10,
        KC_NO, KC_NO, KC_NO, KC_F9, KC_F10,                     KC_F11, KC_F12, KC_NO, KC_NO,   KC_NUM,
        KC_NO, KC_NO, KC_NO, KC_NO, KC_TRNS,                    KC_NO,  KC_NO,  KC_NO, KC_TRNS, KC_CAPS,
                 KC_TRNS, KC_TRNS, KC_TRNS,                KC_TRNS, KC_TRNS, KC_TRNS
    ),
};

#ifdef ENCODER_MAP_ENABLE
// Rotary-encoder bindings, preserved verbatim from the prior build (index 0 = left,
// 1 = right; ENCODER_CCW_CW(ccw, cw)). Designated initializers place each layer's
// row at its enum index.
//   _BASE  L: word-jump (Ctrl+Left/Right)   R: mousewheel scroll
//   _LOWER L: edit (Ctrl+Bspc / Undo)       R: dead
//   _RAISE L: dead                          R: page nav
//   _ADJUST: fall through (TRNS)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [_BASE]   = { ENCODER_CCW_CW(C(KC_LEFT), C(KC_RIGHT)), ENCODER_CCW_CW(MS_WHLD, MS_WHLU) },
    [_LOWER]  = { ENCODER_CCW_CW(C(KC_BSPC), C(KC_Z)),     ENCODER_CCW_CW(KC_NO,   KC_NO)   },
    [_RAISE]  = { ENCODER_CCW_CW(KC_NO,      KC_NO),       ENCODER_CCW_CW(KC_PGDN, KC_PGUP) },
    [_ADJUST] = { ENCODER_CCW_CW(KC_TRNS,    KC_TRNS),     ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },
};
#endif

#ifdef RGB_MATRIX_ENABLE
// Per-key LED map. One ws2812 LED per key, 18 per half (indices 0-17 left,
// 18-35 right). matrix_co maps each [row][col] to its LED index (NO_LED for the
// two unused thumb cols 0,1 on rows 3 and 7). Physical x/y are scaled from the
// keyboard.json layout coords into rgb_matrix space (x 0-224, y 0-64).
// CAVEAT: the vendor PCB's true LED chain order is unknown; if effects look
// scrambled on hardware, the index assignment here is what to remap.
led_config_t g_led_config = {
    {
        // left half
        {  0,  1,  2,  3,  4 },
        {  5,  6,  7,  8,  9 },
        { 10, 11, 12, 13, 14 },
        { NO_LED, NO_LED, 15, 16, 17 },
        // right half
        { 18, 19, 20, 21, 22 },
        { 23, 24, 25, 26, 27 },
        { 28, 29, 30, 31, 32 },
        { NO_LED, NO_LED, 33, 34, 35 },
    },
    {
        // left half positions
        {   0,   7 }, {  19,   2 }, {  37,   0 }, {  56,   2 }, {  75,   4 },
        {   0,  24 }, {  19,  20 }, {  37,  18 }, {  56,  20 }, {  75,  22 },
        {   0,  42 }, {  19,  38 }, {  37,  35 }, {  56,  38 }, {  75,  40 },
        {  54,  57 }, {  75,  60 }, {  96,  64 },
        // right half positions
        { 224,   7 }, { 205,   2 }, { 187,   0 }, { 168,   2 }, { 149,   4 },
        { 224,  24 }, { 205,  20 }, { 187,  18 }, { 168,  20 }, { 149,  22 },
        { 224,  42 }, { 205,  38 }, { 187,  35 }, { 168,  38 }, { 149,  40 },
        { 170,  57 }, { 149,  60 }, { 128,  64 },
    },
    {
        // flags: all per-key
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    },
};
#endif

#ifdef OLED_ENABLE
// Minimal layer + modifier readout. 128x32 default geometry = 21 cols x 4 rows.
oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    return OLED_ROTATION_0;
}

bool oled_task_user(void) {
    oled_write_P(PSTR("Swoop\n"), false);

    oled_write_P(PSTR("Lyr: "), false);
    switch (get_highest_layer(layer_state)) {
        case _BASE:   oled_write_ln_P(PSTR("Base"),   false); break;
        case _LOWER:  oled_write_ln_P(PSTR("Lower"),  false); break;
        case _RAISE:  oled_write_ln_P(PSTR("Raise"),  false); break;
        case _ADJUST: oled_write_ln_P(PSTR("Adjust"), false); break;
        default:      oled_write_ln_P(PSTR("?"),      false); break;
    }

    uint8_t mods = get_mods() | get_oneshot_mods();
    oled_write_P(PSTR("Mod: "), false);
    oled_write_P((mods & MOD_MASK_SHIFT) ? PSTR("S") : PSTR("_"), false);
    oled_write_P((mods & MOD_MASK_CTRL)  ? PSTR("C") : PSTR("_"), false);
    oled_write_P((mods & MOD_MASK_ALT)   ? PSTR("A") : PSTR("_"), false);
    oled_write_ln_P((mods & MOD_MASK_GUI) ? PSTR("G") : PSTR("_"), false);

    return false;
}
#endif
