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
//   _LOWER  -> [1] nav + numpad         (right encoder = page nav)
//   _RAISE  -> [2] symbols + edit       (left encoder = edit)
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
                 KC_TRNS, KC_TRNS, KC_TRNS,                KC_END, KC_0, KC_TRNS
    ),

    // [2] symbols + page-nav. MO(3) on the right inner thumb (was reviung 2u-right).
    [_RAISE] = LAYOUT_split_3x5_3(
        KC_EXLM, KC_AT, KC_HASH, KC_DLR,      KC_PERC,           KC_CIRC, KC_AMPR, KC_ASTR, KC_LPRN, KC_RPRN,
        KC_PGUP, KC_NO, KC_PGDN, KC_NO,       KC_NO,             KC_EQL,  KC_MINS, KC_NO,   KC_LBRC, KC_RBRC,
        KC_LSFT, KC_NO, KC_NO,   TG(_ADJUST), KC_TRNS,           KC_UNDS, KC_PLUS, KC_NO,   KC_LCBR, KC_RCBR,
                 C(KC_Y), KC_TRNS, MO(_ADJUST),          KC_TRNS, KC_TRNS, KC_TRNS
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
//   _LOWER L: dead                          R: page nav
//   _RAISE L: edit (Ctrl+Bspc / Undo)       R: dead
//   _ADJUST: fall through (TRNS)
// Directions flipped 2026-06-10: hardware confirmed both encoders rotated
// backwards, so the two args of every ENCODER_CCW_CW(ccw, cw) are swapped.
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [_BASE]   = { ENCODER_CCW_CW(C(KC_RIGHT), C(KC_LEFT)), ENCODER_CCW_CW(MS_WHLU, MS_WHLD) },
    [_LOWER]  = { ENCODER_CCW_CW(KC_NO,        KC_NO),      ENCODER_CCW_CW(KC_PGUP, KC_PGDN) },
    [_RAISE]  = { ENCODER_CCW_CW(C(KC_Z),     C(KC_BSPC)), ENCODER_CCW_CW(KC_NO,   KC_NO)   },
    [_ADJUST] = { ENCODER_CCW_CW(KC_TRNS,     KC_TRNS),    ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },
};
#endif

#ifdef RGB_MATRIX_ENABLE
// Per-key LED map. One ws2812 LED per key, 18 per half (indices 0-17 left,
// 18-35 right). matrix_co maps each [row][col] to its LED index (NO_LED for the
// two unused thumb cols 0,1 on rows 3 and 7). Physical x/y are scaled from the
// keyboard.json layout coords into rgb_matrix space (x 0-224, y 0-64).
// CAVEAT: the vendor PCB's true LED chain order is unknown; if effects look
// scrambled on hardware, the index assignment here is what to remap.
// FIX A (2026-06-12): corrected per-key LED-chain index mapping derived from
// hardware press-to-light test (custom/led-map-test-2026-06-11.md).
//
// The physical WS2812 strip was wired in a chain order that does NOT match
// the row-major matrix order assumed before.  The test revealed the full
// mis-permutation; the matrix_co below is the deterministic inversion of that
// permutation (rule: new_index(key) = old_index(key_that_previously_lit_key)).
//
// FLAGS / AMBIGUOUS CELLS:
//   L-R/T : r/t are a 2-element swap; both corrected (R=4, T=3).
//   L-A   : 'a' never appeared as a "lit under" target in the test so no chain
//            position physically under A; assigned NO_LED (phantom).
//   L-SPC : was phantom (old index 17, lit nothing).  After inversion,
//            SPC=0 (chain 0 physically under space, confirmed by Q->space).
//   R-Y/U : y/u are a 2-element swap; both corrected (Y=19, U=18).
//   R-M   : the test report flagged "m->comma AMBIGUOUS".  Most consistent
//            chain reading assigns COMM=24 (from unambiguous j->comma) and
//            M=26 (from unambiguous l->m).  Re-test if M still looks wrong.
//   R-QUOT: FIX F -- QUOT=33 (lit when R-encoder pushed; LED sits under ').
//   R-DOT : DOT never appeared as "lit under"; assigned chain 29 by
//            elimination (last unaccounted right-half chain position).
//   WBAK/WFWD (encoder pushes): FIX F (2026-06-12) -- operator pressed both
//            encoders; index 15 lit under A, index 33 lit under '. Encoders have
//            NO LED, so the pushes are NO_LED and those indices belong to the
//            outer pinky-home keys: A=15, QUOT=33. This corrects FIX E (which
//            wrongly called A/' "dead hardware"; they are 17 real LEDs/half,
//            verified on rainbow -- the indices were just on the wrong cells).
//
// position[] is reordered to match the physical chain order so that
// distance-based effects compute correct spatial distances.
led_config_t g_led_config = {
    {
        // left half  (row x col -> LED chain index; NO_LED = 255)
        // Row 0: Q   W   E   R   T
        {  16,  11,  10,   4,   3 },
        // Row 1: A   S    D    F    G   (FIX F: A=15, the LED that lit on L-encoder push)
        {  15,  12,   9,   5,   2 },
        // Row 2: Z   X   C   V   B
        {  14,  13,   8,   6,   1 },
        // Row 3: (NO_LED x2)  WBAK CTRL SPC  (FIX F: WBAK=encoder push, no LED)
        { NO_LED, NO_LED, NO_LED,   7,   0 },
        // right half
        // FIX D (2026-06-12): direct from hardware press-to-light, full right
        // capture (operator). The mirror guess (FIX C) had the 3x5 columns
        // reversed and a wrong phantom. Ground truth: pressing each key lit the
        // index whose PHYSICAL position is recorded below; new(key)=index under
        // key. Thumbs HOME(18)/LGUI(25) confirmed correct. The right half drives
        // ALL 18 LEDs (no phantom): index 35 sits under H (it was the only index
        // not covering any observed 3x5 position), and QUOT has a real LED (20).
        // Row 4: Y   U   I   O   P
        {  34,  29,  28,  22,  21 },
        // Row 5 cols 0..4 are PHYSICALLY  QUOT L K J H  (right half matrix cols are
        // reversed vs physical; keyboard.json: [5,0]=' outer pinky x12, [5,4]=H inner x8).
        // FIX G (2026-06-12): QUOT(['],[5,0])=33 (LED under '); H([5,4])=35. FIX F had
        // these two transposed because the old comment mislabeled the columns H..QUOT.
        // FIX E (2026-06-12): outer pinky-home (' right / A left) has NO working
        // LED on either half -- both halves drive 17 LEDs, not 18. Same physical
        // position is dark on both, the signature of a hardware gap (not a map
        // slip). ' -> NO_LED mirrors left A=NO_LED; index 20 (the dead/absent LED
        // at that spot) is now unused. H stays 35 (a real LED, confirmed lit).
        {  33,  30,  27,  23,  35 },
        // Row 6: N   M   ,   .   /
        {  32,  31,  26,  24,  19 },
        // Row 7: (NO_LED x2)  WFWD LGUI HOME  (FIX F: WFWD=encoder push, no LED)
        { NO_LED, NO_LED, NO_LED,  25,  18 },
    },
    {
        // Physical (x,y) of each chain position (index order = WS2812 chain order).
        // Left chain 0-17:
        //  0=SPC  1=B    2=G    3=T    4=R    5=F    6=V    7=CTRL
        {  96,  64 }, {  75,  40 }, {  75,  22 }, {  75,   4 },
        {  56,   2 }, {  56,  20 }, {  56,  38 }, {  75,  60 },
        //  8=C    9=D   10=E   11=W   12=S   13=X   14=Z   15=A
        {  37,  35 }, {  37,  18 }, {  37,   0 }, {  19,   2 },
        {  19,  20 }, {  19,  38 }, {   0,  42 }, {   0,  24 },
        // 16=Q   17=phantom (declared 18th LED, no physical LED)
        {   0,   7 }, {   0,  24 },
        // Right chain 18-35 (true physical position per index, from hardware):
        // 18=HOME 19=SLSH 20=phantom 21=P  22=O    23=L    24=DOT  25=LGUI
        { 128,  64 }, { 224,  42 }, { 224,  24 }, { 224,   7 },
        { 205,   2 }, { 205,  20 }, { 205,  38 }, { 149,  60 },
        // 26=COMM 27=K    28=I    29=U    30=J    31=M    32=N    33=QUOT
        { 187,  35 }, { 187,  18 }, { 187,   0 }, { 168,   2 },
        { 168,  20 }, { 168,  38 }, { 149,  40 }, { 224,  24 },
        // 34=Y   35=H
        { 149,   4 }, { 149,  22 },
    },
    {
        // flags: all per-key
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    },
};
#endif

#ifdef RGB_MATRIX_ENABLE
// Feed the heatmap's key-DOWN tracker (heatmap.c = the one and only RGB pattern).
// The retired ledmap diagnostic's tracker is no longer called.
//
// FIX B (2026-06-12): slave-half dark regression.
// Root cause: in QMK split, matrix_post_scan() calls matrix_scan_kb() (which
// reaches matrix_scan_user) ONLY on the master.  On the slave it calls
// matrix_slave_scan_kb() -> matrix_slave_scan_user() instead.  Without an
// override here, the slave's scan functions were never called, so hm_short /
// hm_long / lm_hit stayed all-zero on the slave side.  The render functions
// (heatmap_render / ledmap_render) DO run on both halves (rgb_matrix_task is
// unconditional) and RGB_MATRIX_SPLIT + USE_LIMITS correctly bounds them to
// each half's LED range, but they rendered nothing because the counters/timers
// were always zero.  Built-in effects were unaffected because they don't depend
// on these user-managed arrays.
// Fix: mirror matrix_scan_user in matrix_slave_scan_user so both codepaths
// populate the scan state.  SPLIT_TRANSPORT_MIRROR ensures the slave's
// matrix_get_row() returns the full mirrored matrix by the time the slave
// scan hook fires, so both halves build identical counters.
#    include "heatmap.h"
void matrix_scan_user(void) {
    heatmap_record_scan();
}
void matrix_slave_scan_user(void) {
    heatmap_record_scan();
}

// Pin the heatmap as the boot RGB mode -- the one and only RGB pattern.
// RGB_MATRIX_DEFAULT_MODE (config.h) only seeds the EEPROM when it is blank/
// reset, so a board previously flashed with a different effect (e.g. the retired
// led_map diagnostic) keeps that STALE mode forever. Force heatmap_neon into the
// live (RAM) state on every boot with the _noeeprom setters: this overrides
// stale EEPROM without burning a write cycle, and because the mode is RAM-
// resident it survives RGB_MATRIX_SLEEP (which only blanks the LEDs) -> resumes
// on wake.
void keyboard_post_init_user(void) {
    rgb_matrix_enable_noeeprom();
    rgb_matrix_mode_noeeprom(RGB_MATRIX_CUSTOM_heatmap_neon);
}
#endif

#ifdef OLED_ENABLE
// Bongo cat on BOTH OLEDs (no is_keyboard_master() gate; the slave reacts via
// SPLIT_ACTIVITY_ENABLE in config.h). Frames + render loop live in bongo.h.
#include "bongo.h"

// RIGHT half reads upright at OLED_ROTATION_180; the LEFT half is mounted 180deg
// from it and reads upright at OLED_ROTATION_0 (HARDWARE-confirmed by operator).
// The LEFT art is also mirrored horizontally in bongo_draw() so the two cats
// face each other. Orientation does NOT change which PAW a tap frame depicts --
// the leading paw is chosen per half by frame index in bongo.h, not here.
oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    return is_keyboard_left() ? OLED_ROTATION_0 : OLED_ROTATION_180;
}

// OLED is the bongo cat, always (operator lock-in 2026-06-12). The led_map
// diagnostic and its per-mode text readout are retired now that the heatmap is
// the one and only RGB pattern, so there is no longer a mode switch here -- both
// halves render the keystroke-reactive bongo cat every tick.
bool oled_task_user(void) {
    render_bongo();
    return false;
}
#endif
