/* Copyright 2020 Danny Nguyen <danny@keeb.io>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include QMK_KEYBOARD_H
#include "keymap_combo.h" // following http://combos.gboards.ca/docs/install/ for combos
// !remind me to buy gergo a beer

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT_ortho_5x12(
        KC_GRV,  KC_1,          KC_2,   KC_3,   KC_4,   KC_5,               KC_6,           KC_7,       KC_8,   KC_9,           KC_0,       KC_NO,
        KC_TAB,  KC_Q,          KC_W,   KC_E,   KC_R,   KC_T,               KC_Y,           KC_U,       KC_I,   KC_O,           KC_P,       KC_NO,
        KC_ESC,  KC_A,          KC_S,   KC_D,   KC_F,   KC_G,               KC_H,           KC_J,       KC_K,   KC_L,           KC_QUOT,    KC_NO,
        KC_LSFT, LSFT_T(KC_Z),  KC_X,   KC_C,   KC_V,   LT(1,KC_B),         KC_N,           KC_M,       KC_COMM,RALT_T(KC_DOT), KC_SLSH,    KC_NO,
        KC_LALT, MO(3),         KC_LALT,MO(1),  KC_LCTL,KC_SPC,             LT(2,KC_HOME),  KC_LGUI,    KC_UP,  KC_LEFT,        KC_DOWN,    KC_RGHT
    ),
    [1] = LAYOUT_ortho_5x12(
        KC_TILD, KC_EXLM,       KC_AT,  KC_HASH,KC_DLR,KC_PERC,             KC_CIRC,        KC_AMPR,    KC_ASTR,KC_LPRN,        KC_RPRN,    KC_NO,
        KC_TILD, KC_1,          KC_2,   KC_3,   KC_4,   KC_5,               KC_6,           KC_7,       KC_8,   KC_9,           KC_0,       KC_NO,
        KC_DEL,  KC_UP,         KC_LEFT,KC_DOWN,KC_RGHT,LCTL(KC_Y),         KC_EQL,         KC_4,       KC_5,   KC_6,           KC_PPLS,    KC_NO,
        BL_STEP, LSFT(KC_Z),    KC_NO,  KC_NO,  KC_NO,  KC_TRNS,            KC_UNDS,        KC_1,       KC_2,   KC_3,           KC_PMNS,    KC_NO,
        KC_TRNS, KC_TRNS,       KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,            KC_END,         KC_0,       KC_MNXT,KC_VOLD,        KC_VOLU,    KC_MPLY
    ),
    [2] = LAYOUT_ortho_5x12(
        KC_GRV,  KC_1,          KC_2,   KC_3,   KC_4,   KC_5,               KC_6,           KC_7,       KC_8,   KC_9,           KC_0,       KC_NO,
        KC_GRV,  KC_EXLM,       KC_AT,  KC_HASH,KC_DLR, KC_PERC,            KC_CIRC,        KC_AMPR,    KC_ASTR,KC_LPRN,        KC_RPRN,    KC_NO,
        KC_DEL,  KC_PGUP,       KC_NO,  KC_PGDN,KC_NO,  KC_NO,              KC_EQL,         KC_MINS,    KC_NO,  KC_LBRC,        KC_RBRC,    KC_NO,
        KC_TRNS, KC_LSFT,       KC_NO,  KC_NO,  KC_NO,  KC_TRNS,            KC_UNDS,        KC_PLUS,    KC_NO,  KC_LCBR,        KC_RCBR,    KC_NO,
        KC_TRNS, KC_TRNS,       KC_TRNS,KC_TRNS,KC_TRNS,MO(3),              KC_TRNS,        KC_TRNS,    KC_MNXT,KC_VOLD,        KC_VOLU,    KC_MPLY
    ),
    [3] = LAYOUT_ortho_5x12(
        KC_TRNS, QK_BOOT,       DB_TOGG,RGB_TOG,RGB_MOD,RGB_HUI,            RGB_HUD,        RGB_SAI,    RGB_SAD,RGB_VAI,        RGB_VAD,    KC_TRNS,
        KC_F12,  KC_F1,         KC_F2,  KC_F3,  KC_F4,  KC_F5,              KC_F6,          KC_F7,      KC_F8,  KC_F9,          KC_F10,     KC_F11,
        KC_TRNS, KC_TRNS,       KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,            KC_TRNS,        KC_TRNS,    KC_TRNS,KC_TRNS,        KC_NUM,     KC_TRNS,
        KC_TRNS, KC_TRNS,       KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,            KC_TRNS,        KC_TRNS,    KC_TRNS,KC_TRNS,        KC_CAPS,    KC_TRNS,
        KC_TRNS, KC_TRNS,       KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,            KC_TRNS,        KC_TRNS,    KC_TRNS,KC_TRNS,        KC_TRNS,    KC_TRNS
    )
};
