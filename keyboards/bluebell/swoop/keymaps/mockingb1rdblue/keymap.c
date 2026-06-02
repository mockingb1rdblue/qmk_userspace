// Copyright 2026 mockingb1rdblue
// SPDX-License-Identifier: GPL-2.0-or-later
//
// TurkeyBoards Swoop / Sea-Picro (RP2040) — full keymap, Phase 3b.
// Compiles via upstream bluebell/swoop + CONVERT_TO=promicro_rp2040 (see rules.mk).
//
// Encoder push switches are matrix keys whose positions depend on the vendor PCB
// (PR pending with vendor as of 2026-06-01). LAYOUT_split_3x5_3 from upstream
// info.json does NOT expose encoder-push positions, so push binds are deferred
// until the PCB schematic lands — see TODO block below encoder_map.

#include QMK_KEYBOARD_H

enum layers {
    _BASE,
    _RAISE,  // L1 — numbers + arrows (left thumb hold on LT(1, BSPC) → left encoder dead by design)
    _LOWER,  // L2 — symbols + edit (right thumb hold on LT(2, HOME) → right encoder dead by design)
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_BASE] = LAYOUT_split_3x5_3(
        KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,               KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,
        KC_A,    KC_S,    KC_D,    KC_F,    KC_G,               KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN,
        KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,               KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH,
                       KC_LCTL,  LT(_RAISE, KC_BSPC),  KC_ESC,        KC_ENT,  KC_SPC,  LT(_LOWER, KC_HOME)
    ),

    [_RAISE] = LAYOUT_split_3x5_3(
        KC_1,    KC_2,    KC_3,    KC_4,    KC_5,               KC_6,    KC_7,    KC_8,    KC_9,    KC_0,
        KC_TAB,  KC_LEFT, KC_DOWN, KC_UP,   KC_RGHT,            KC_MINS, KC_EQL,  KC_LBRC, KC_RBRC, KC_QUOT,
        KC_LSFT, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,            KC_BSLS, KC_GRV,  KC_COMM, KC_DOT,  KC_SLSH,
                       KC_TRNS, KC_TRNS, KC_TRNS,                       KC_TRNS, KC_TRNS, KC_TRNS
    ),

    [_LOWER] = LAYOUT_split_3x5_3(
        KC_EXLM, KC_AT,   KC_HASH, KC_DLR,  KC_PERC,            KC_CIRC, KC_AMPR, KC_ASTR, KC_LPRN, KC_RPRN,
        KC_TRNS, KC_NO,   KC_NO,   KC_HOME, KC_PGUP,            KC_MINS, KC_UNDS, KC_LBRC, KC_RBRC, KC_PIPE,
        KC_TRNS, C(KC_Z), C(KC_X), C(KC_C), C(KC_V),            KC_PLUS, KC_EQL,  KC_LCBR, KC_RCBR, KC_BSLS,
                       KC_TRNS, KC_TRNS, KC_TRNS,                       KC_TRNS, KC_TRNS, KC_TRNS
    ),
};

#if defined(ENCODER_MAP_ENABLE)
// encoder_map signature: [layer][NUM_ENCODERS][NUM_DIRECTIONS]
// NUM_ENCODERS = 2 (1 left + 1 right per upstream bluebell/swoop info.json).
// ENCODER_CCW_CW(ccw, cw) — verified against qmk_firmware/quantum/encoder.h:105.
// Index 0 = left encoder, Index 1 = right encoder.
//
// Per spitball (2026-06-02):
//   L0   left: word-jump (Ctrl+Left / Ctrl+Right)   right: scroll
//   L1   left: dead (thumb on LT raise)             right: page nav
//   L2   left: edit (Ctrl+Bspc / Undo)              right: dead (thumb on LT lower)
//   L3+: transparent → falls through to L_/L0 per QMK layer stack
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [_BASE]  = { ENCODER_CCW_CW(C(KC_LEFT), C(KC_RIGHT)), ENCODER_CCW_CW(KC_WH_D, KC_WH_U)   },
    [_RAISE] = { ENCODER_CCW_CW(KC_NO,      KC_NO),       ENCODER_CCW_CW(KC_PGDN, KC_PGUP)   },
    [_LOWER] = { ENCODER_CCW_CW(C(KC_BSPC), C(KC_Z)),     ENCODER_CCW_CW(KC_NO,   KC_NO)     },
};
#endif

// TODO(vendor-pcb): encoder push switches.
// LAYOUT_split_3x5_3 (upstream) is 36 keys = 5+5+5+3 thumbs per side = no push slots.
// Vial UF2 extraction confirmed 2 encoders but did not reveal which matrix
// position (if any) the push switch is wired to. When the vendor PCB schematic
// arrives, decide one of:
//   (a) Push wired to an unused matrix cell → declare a new LAYOUT_split_3x5_3_2e
//       macro in a fork of bluebell/swoop info.json and bind here.
//   (b) Push wired in series with an existing thumb → handle via process_record_user.
//   (c) Push not wired → drop the L0 Back/Forward + L2 Redo binds from the spitball.
// Target binds (when (a) or (b)):
//   L0:  LPush = KC_WBAK (browser back)    RPush = KC_WFWD (browser forward)
//   L1:  LPush = TBD                       RPush = TBD
//   L2:  LPush = C(KC_Y)  (redo)           RPush = TBD
