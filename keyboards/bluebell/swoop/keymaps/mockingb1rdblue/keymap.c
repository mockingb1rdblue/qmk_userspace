// Copyright 2026 mockingb1rdblue
// SPDX-License-Identifier: GPL-2.0-or-later
//
// TurkeyBoards Swoop / Sea-Picro (RP2040) — full keymap, Phase 3b.
// Compiles via upstream bluebell/swoop + CONVERT_TO=promicro_rp2040 (see rules.mk).
//
// ENCODER-PUSH PROBE BUILD (2026-06-08)
// ------------------------------------
// Upstream bluebell/swoop info.json wires the encoders' ROTATION only (left B4/B5,
// right B5/B4) and defines NO push switch anywhere. The 36-key LAYOUT_split_3x5_3
// scans a 4x5-per-half matrix but only populates 18 cells/side, leaving four
// already-scanned-but-unmapped cells: [3,0] [3,1] (left) and [7,0] [7,1] (right)
// — the conventional landing spot for a Swoop encoder push switch.
//
// This build binds those four cells to unique SEND_STRING tags so a flash + press
// in any text field reveals empirically which cell (if any) each encoder push is
// wired to — resolving blocker #3 without the vendor schematic. Pressing an
// unwired cell is harmless (no matrix contact = nothing fires).
//
// Once hardware tells us the mapping, swap the probe tags for the real binds
// (target: L0 LPush=KC_WBAK, RPush=KC_WFWD; L2 LPush=C(KC_Y) redo) — see bottom.

#include QMK_KEYBOARD_H

enum layers {
    _BASE,
    _RAISE,  // L1 — numbers + arrows (left thumb hold on LT(1, BSPC) → left encoder dead by design)
    _LOWER,  // L2 — symbols + edit (right thumb hold on LT(2, HOME) → right encoder dead by design)
};

// Encoder-push probe keycodes. Each types a unique tag visible in any text field.
enum custom_keycodes {
    ENC_L0 = SAFE_RANGE,  // matrix [3,0] — left half, col 0 spare
    ENC_L1,               // matrix [3,1] — left half, col 1 spare
    ENC_R0,               // matrix [7,0] — right half, col 0 spare
    ENC_R1,               // matrix [7,1] — right half, col 1 spare
};

// Local full-matrix macro. Identical key-for-key to upstream LAYOUT_split_3x5_3
// for the first 36 args (so the existing layout is byte-for-byte preserved),
// then appends the four spare encoder-switch cells: k30 k31 (left) k70 k71 (right).
// Argument names are matrix coordinates; the right half is entered in visual
// (reversed-column) order exactly as upstream, so this drops in unchanged.
#define LAYOUT_split_3x5_3_enc( \
    k00, k01, k02, k03, k04,        k44, k43, k42, k41, k40, \
    k10, k11, k12, k13, k14,        k54, k53, k52, k51, k50, \
    k20, k21, k22, k23, k24,        k64, k63, k62, k61, k60, \
              k32, k33, k34,        k74, k73, k72,           \
    /* appended encoder-switch probe cells */ \
    k30, k31,                       k70, k71                 \
) { \
    { k00, k01, k02, k03, k04 }, \
    { k10, k11, k12, k13, k14 }, \
    { k20, k21, k22, k23, k24 }, \
    { k30, k31, k32, k33, k34 }, \
    { k40, k41, k42, k43, k44 }, \
    { k50, k51, k52, k53, k54 }, \
    { k60, k61, k62, k63, k64 }, \
    { k70, k71, k72, k73, k74 }  \
}

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_BASE] = LAYOUT_split_3x5_3_enc(
        KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,               KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,
        KC_A,    KC_S,    KC_D,    KC_F,    KC_G,               KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN,
        KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,               KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH,
                       KC_LCTL,  LT(_RAISE, KC_BSPC),  KC_ESC,        KC_ENT,  KC_SPC,  LT(_LOWER, KC_HOME),
        /* enc-switch probes: */  ENC_L0,  ENC_L1,                    ENC_R0,  ENC_R1
    ),

    [_RAISE] = LAYOUT_split_3x5_3_enc(
        KC_1,    KC_2,    KC_3,    KC_4,    KC_5,               KC_6,    KC_7,    KC_8,    KC_9,    KC_0,
        KC_TAB,  KC_LEFT, KC_DOWN, KC_UP,   KC_RGHT,            KC_MINS, KC_EQL,  KC_LBRC, KC_RBRC, KC_QUOT,
        KC_LSFT, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,            KC_BSLS, KC_GRV,  KC_COMM, KC_DOT,  KC_SLSH,
                       KC_TRNS, KC_TRNS, KC_TRNS,                       KC_TRNS, KC_TRNS, KC_TRNS,
        /* probes fall through to base: */ KC_TRNS, KC_TRNS,           KC_TRNS, KC_TRNS
    ),

    [_LOWER] = LAYOUT_split_3x5_3_enc(
        KC_EXLM, KC_AT,   KC_HASH, KC_DLR,  KC_PERC,            KC_CIRC, KC_AMPR, KC_ASTR, KC_LPRN, KC_RPRN,
        KC_TRNS, KC_NO,   KC_NO,   KC_HOME, KC_PGUP,            KC_MINS, KC_UNDS, KC_LBRC, KC_RBRC, KC_PIPE,
        KC_TRNS, C(KC_Z), C(KC_X), C(KC_C), C(KC_V),            KC_PLUS, KC_EQL,  KC_LCBR, KC_RCBR, KC_BSLS,
                       KC_TRNS, KC_TRNS, KC_TRNS,                       KC_TRNS, KC_TRNS, KC_TRNS,
        /* probes fall through to base: */ KC_TRNS, KC_TRNS,           KC_TRNS, KC_TRNS
    ),
};

// Encoder-push probe handler: each spare cell types a unique tag on key-down.
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        switch (keycode) {
            case ENC_L0: SEND_STRING("<EncPush LEFT  matrix[3,0]>"); return false;
            case ENC_L1: SEND_STRING("<EncPush LEFT  matrix[3,1]>"); return false;
            case ENC_R0: SEND_STRING("<EncPush RIGHT matrix[7,0]>"); return false;
            case ENC_R1: SEND_STRING("<EncPush RIGHT matrix[7,1]>"); return false;
        }
    }
    return true;
}

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

// TODO(vendor-pcb): encoder push switches — PROBE ACTIVE (this build).
// The four SEND_STRING probes above map every plausible spare matrix cell. After
// flashing, press each encoder's push in a text editor and record which tag (if
// any) appears. Outcomes:
//   • A tag appears  → that cell is the push. Replace its SEND_STRING with the
//     real bind and (optionally) give it per-layer behavior via the layout.
//   • Nothing on either col for a half → that half's push is NOT on the matrix
//     (direct GPIO or unwired); resolve via direct-pin read once the schematic
//     lands, or drop the bind.
// Target binds once located:
//   L0:  LPush = KC_WBAK (browser back)    RPush = KC_WFWD (browser forward)
//   L2:  LPush = C(KC_Y) (redo)            RPush = TBD
