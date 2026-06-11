#pragma once

// Split sides identify themselves by EE_HANDS (write side flag to EEPROM with
// `qmk flash ... -bl uf2-split-{left,right}`). Matches upstream bluebell/swoop.
#define EE_HANDS

// ===========================================================================
// Tap/hold: EXACT port of the operator's reviung34 originals (operator request
// 2026-06-10: "my originals were set perfectly"). The reviung config is a bare
// TAPPING_TERM 150 with NO extra modes (PERMISSIVE_HOLD commented out there).
// Modern QMK's default overlap rule already matches the old default the
// reviung ran (IGNORE_MOD_TAP_INTERRUPT became the default in 2022 and was
// removed as a flag): keys overlapping inside the term settle as TAP; a hold
// fires only by outlasting the 150ms term. HOLD_ON_OTHER_KEY_PRESS,
// FLOW_TAP_TERM, CHORDAL_HOLD, PERMISSIVE_HOLD, QUICK_TAP_TERM: all
// intentionally absent.
// ===========================================================================
#define TAPPING_TERM 150

// gboards combos (combos.def -> keymap_combo.h). Matches the operator's reviung34
// keymap config: variable-length combo array + 40ms chord window.
#define COMBO_VARIABLE_LEN
#define COMBO_TERM 40

// ===========================================================================
// RGB matrix: per-key ws2812 on D3/GP0, 36 LEDs (split 18/18).
// ===========================================================================
// ws2812.pin (D3/GP0) comes from keyboard.json; the vendor (PIO) driver needs a
// free PIO block on RP2040.
#define WS2812_PIO_USE_PIO1
#define RGB_MATRIX_LED_COUNT 36
#define RGB_MATRIX_SPLIT { 18, 18 }
#define RGB_MATRIX_MAXIMUM_BRIGHTNESS 150
// Default = the press-to-light LED<->key MAPPING diagnostic (custom effect
// led_map in rgb_matrix_user.inc / ledmap.c): press a key, its LED glows for ~1s
// (left-half hue vs right-half hue) so the operator can confirm the LED wiring.
// The per-layer neon keypress heatmap (heatmap_neon / heatmap.c) stays compiled
// and selectable. Cycle-left-right also kept as a selectable fallback mode.
#define ENABLE_RGB_MATRIX_CYCLE_LEFT_RIGHT
#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_CUSTOM_led_map
#define RGB_MATRIX_DEFAULT_VAL 150
#define RGB_MATRIX_DEFAULT_SPD 100
// Sleep with the OLEDs: LEDs off after 1 min of no input (and on host suspend).
#define RGB_MATRIX_TIMEOUT 60000
#define RGB_MATRIX_SLEEP
// Drive the matrix at ~38Hz (26ms flush floor) so the heatmap's gamma + temporal
// -dither fades (heatmap.c) have enough frames to look smooth, not steppy.
#define RGB_MATRIX_LED_FLUSH_LIMIT 26

// ===========================================================================
// OLED: 128x32 SSD1306 over I2C1.  *** PINS ARE A BEST-GUESS, NOT VERIFIED ***
// ===========================================================================
// GP2 (=D1) / GP3 (=D0) are the standard Pro-Micro I2C pads and the RP2040 I2C1
// SDA/SCL pair; they are the only free non-matrix / non-split-serial pins the
// encoder probe left open. The vendor PCB may not route an OLED header here.
// If the OLED stays blank: try swapping SDA/SCL, then confirm the header pinout
// on the physical board. I2C1 (not I2C0) -> I2CD1 + RP_I2C_USE_I2C1 (mcuconf.h).
#define I2C_DRIVER I2CD1
#define I2C1_SDA_PIN D1
#define I2C1_SCL_PIN D0
#define OLED_TIMEOUT 60000

// Sync input-activity timestamps master<->slave so the bongo cat idle/prep/
// sleep pacing stays aligned on BOTH OLEDs.
#define SPLIT_ACTIVITY_ENABLE

// Sync the active layer to the slave so the per-layer keypress heatmap
// (heatmap.c) renders the SAME layer's hot keys on both halves.
#define SPLIT_LAYER_STATE_ENABLE

// Mirror the master's matrix to the slave so BOTH halves see the FULL matrix
// (by default the slave only sees its own rows). bongo.h needs this: each OLED
// must tell which physical half a key-down landed on to pick the paw (left
// half -> left paw, right half -> right paw, identically on both displays).
// QMK copies PUT_MASTER_MATRIX into the slave's matrix every split scan, so
// matrix_get_row() returns all rows on both sides.
#define SPLIT_TRANSPORT_MIRROR
