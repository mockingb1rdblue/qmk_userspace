// Copyright 2026 mockingb1rdblue
// SPDX-License-Identifier: GPL-2.0-or-later
//
// TurkeyBoards Swoop / Sea-Picro (RP2040): COMBINED BRING-UP PROBE.
// Compiles via upstream bluebell/swoop + CONVERT_TO=promicro_rp2040 (see rules.mk).
//
// ROTARY-ENCODER PIN-DISCOVERY PROBE BUILD (2026-06-09)
// ----------------------------------------------------
// Symptom from last session: the rotary encoders register NOTHING. Upstream
// bluebell/swoop hard-wires the encoder rotation to AVR pads B4/B5 (-> RP2040
// GP8/GP9). If the actual TurkeyBoards Sea-Picro PCB routes the encoder to
// different GPIOs, QMK's encoder driver polls the wrong two pins and sees no
// rotation, indistinguishable from dead hardware. We don't have the vendor
// schematic, so we DISCOVER the wiring empirically.
//
// This build DISABLES the encoder driver (rules.mk: ENCODER_ENABLE = no) so
// nothing claims GP8/GP9, then sweeps every Pro-Micro GPIO that is NOT a matrix
// row/col or the split-serial pin: the only physical pins an encoder leg could
// be on. Each candidate pin is held input-with-pull-up and assigned a unique
// letter; on a FALLING edge (pin pulled to GND, i.e. encoder contact closes) the
// firmware types that letter. Turn an encoder and two letters stream (the A/B
// quadrature pair), and that pair IS the real encoder wiring.
//
//   LETTER  AVR  RP2040   note
//     a     D3   GP0      WS2812 pad (RGB disabled => free)
//     b     D1   GP2      TX
//     c     D0   GP3      RX
//     d     B4   GP8      <- upstream encoder pin_a(L)/pin_b(R) guess
//     e     B5   GP9      <- upstream encoder pin_b(L)/pin_a(R) guess
//     f     B0   GP16     QT/LED pad
//     g     D5   GP17     QT/LED pad
//     h     B3   GP20     free
//     i     B6   GP21     free
//     j     B2   GP23     free
//
// SPLIT NOTE: only the USB-master half runs this loop and reads its own GPIOs;
// slave-half pins are NOT transported. TEST ONE HALF AT A TIME, each plugged
// DIRECTLY into USB (no TRRS). The plugged-in half is the side under test.
//   * Left half alone  -> turn left encoder  -> record the two letters = LEFT pair
//   * Right half alone -> turn right encoder -> record the two letters = RIGHT pair
//   * No letters at all -> encoder not on any free GPIO (dead joint / dead part).
//
// PUSH-SWITCH PROBE (retained from the prior build): the four spare matrix cells
// [3,0][3,1][7,0][7,1] still type unique <EncPush ...> tags on PRESS, so a single
// flash reveals BOTH the rotary pin pair (letters) AND the push wiring (tags).
//
// To return to the daily-driver keymap: rules.mk ENCODER_ENABLE = yes +
// ENCODER_MAP_ENABLE = yes (the encoder_map below is preserved, guarded), and
// delete the probe pin sweep + the four ENC_* push cells.

#include QMK_KEYBOARD_H

enum layers {
    _BASE,
    _RAISE,  // L1: numbers + arrows (left thumb hold on LT(1, BSPC) -> left encoder dead by design)
    _LOWER,  // L2: symbols + edit (right thumb hold on LT(2, HOME) -> right encoder dead by design)
};

// Encoder-push probe keycodes. Each types a unique tag visible in any text field.
enum custom_keycodes {
    ENC_L0 = SAFE_RANGE,  // matrix [3,0]: left half, col 0 spare
    ENC_L1,               // matrix [3,1]: left half, col 1 spare
    ENC_R0,               // matrix [7,0]: right half, col 0 spare
    ENC_R1,               // matrix [7,1]: right half, col 1 spare
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

// ===========================================================================
// Rotary-encoder pin-discovery sweep
// ===========================================================================
// Each entry: a candidate free GPIO, the letter it types on a falling edge, and
// a human label (AVR pad / RP2040 GP) for the legend. The pin macros (B4, D3 ...)
// are defined to their RP2040 GP numbers by the active sparkfun_pm2040 converter
// (CONVERT_TO=promicro_rp2040 -> promicro_to_sparkfun_pm2040/_pin_defs.h).
typedef struct {
    pin_t       pin;
    char        letter;
    const char *label;
} probe_pin_t;

static const probe_pin_t probe_pins[] = {
    { D3, 'a', "D3/GP0"  },  // WS2812 pad (RGB off => free)
    { D1, 'b', "D1/GP2"  },  // TX
    { D0, 'c', "D0/GP3"  },  // RX
    { B4, 'd', "B4/GP8"  },  // upstream encoder pin guess
    { B5, 'e', "B5/GP9"  },  // upstream encoder pin guess
    { B0, 'f', "B0/GP16" },  // QT/LED pad
    { D5, 'g', "D5/GP17" },  // QT/LED pad
    { B3, 'h', "B3/GP20" },
    { B6, 'i', "B6/GP21" },
    { B2, 'j', "B2/GP23" },
};
#define NUM_PROBE_PINS (sizeof(probe_pins) / sizeof(probe_pins[0]))
#define PROBE_DEBOUNCE_MS 4  // ignore re-triggers within this window per pin

static bool     probe_last[NUM_PROBE_PINS];  // last sampled level (true = high)
static uint16_t probe_stamp[NUM_PROBE_PINS]; // timer of last accepted change (0 = none yet)

// Configure every candidate pin as input-with-pull-up (encoder common = GND, so a
// closed contact reads LOW) and snapshot the resting level so we don't emit at boot.
void keyboard_post_init_user(void) {
    for (uint8_t i = 0; i < NUM_PROBE_PINS; i++) {
        gpio_set_pin_input_high(probe_pins[i].pin);
        probe_last[i]  = gpio_read_pin(probe_pins[i].pin);
        probe_stamp[i] = 0;
    }
}

// Poll the candidate pins every main loop; type the pin's letter on each FALLING
// edge (contact closed to GND), debounced per pin. Falling-edge-only keeps a slow
// encoder turn to a clean alternating pair (e.g. "dedede" => pins d and e).
void housekeeping_task_user(void) {
    for (uint8_t i = 0; i < NUM_PROBE_PINS; i++) {
        bool now = gpio_read_pin(probe_pins[i].pin);
        if (now == probe_last[i]) continue;
        if (probe_stamp[i] != 0 && timer_elapsed(probe_stamp[i]) < PROBE_DEBOUNCE_MS) continue;
        probe_last[i]  = now;
        probe_stamp[i] = timer_read() | 1;          // never 0 (0 means "no change yet")
        if (!now) {                                  // HIGH -> LOW: contact to GND
            tap_code(KC_A + (uint8_t)(probe_pins[i].letter - 'a'));
        }
    }
}

// Encoder-push probe handler: each spare matrix cell types a unique tag on key-down.
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
// PRESERVED for the daily-driver build (currently compiled OUT: ENCODER_ENABLE=no
// in rules.mk for the pin-discovery probe). Restore both encoder rules to re-arm.
// encoder_map signature: [layer][NUM_ENCODERS][NUM_DIRECTIONS]; index 0 = left,
// 1 = right. ENCODER_CCW_CW(ccw, cw) per qmk_firmware/quantum/encoder.h.
//   L0   left: word-jump (Ctrl+Left / Ctrl+Right)   right: scroll
//   L1   left: dead (thumb on LT raise)             right: page nav
//   L2   left: edit (Ctrl+Bspc / Undo)              right: dead (thumb on LT lower)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [_BASE]  = { ENCODER_CCW_CW(C(KC_LEFT), C(KC_RIGHT)), ENCODER_CCW_CW(KC_WH_D, KC_WH_U)   },
    [_RAISE] = { ENCODER_CCW_CW(KC_NO,      KC_NO),       ENCODER_CCW_CW(KC_PGDN, KC_PGUP)   },
    [_LOWER] = { ENCODER_CCW_CW(C(KC_BSPC), C(KC_Z)),     ENCODER_CCW_CW(KC_NO,   KC_NO)     },
};
#endif

// TODO(vendor-pcb): once the probe reports the real pin pair per side, restore the
// encoder driver and set the discovered pins (a keymap-level config.h override if
// they differ from B4/B5), then re-arm encoder_map.
// Push binds, once located via the <EncPush ...> tags:
//   L0:  LPush = KC_WBAK (browser back)    RPush = KC_WFWD (browser forward)
//   L2:  LPush = C(KC_Y) (redo)            RPush = TBD
