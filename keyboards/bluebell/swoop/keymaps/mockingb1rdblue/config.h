#pragma once

// Split sides identify themselves by EE_HANDS (write side flag to EEPROM with
// `qmk flash ... -bl uf2-split-{left,right}`). Matches upstream bluebell/swoop.
#define EE_HANDS

// ===========================================================================
// Tap/hold: HOLD_ON_OTHER_KEY_PRESS (operator request 2026-06-10). Holding a
// tap-hold key and pressing ANY other key settles the hold IMMEDIATELY, no
// waiting out TAPPING_TERM. z(hold)+a => Shift+A the instant `a` goes down.
// The previous roll-protection stack (FLOW_TAP_TERM, CHORDAL_HOLD,
// PERMISSIVE_HOLD) is removed: all three force/await TAP on overlap and
// directly defeat this behavior. Trade-off: fast same-hand rolls across a
// mod-tap (e.g. z->a in one motion) will now fire the mod.
// Docs: docs.qmk.fm/tap_hold#hold-on-other-key-press
// ===========================================================================
#define TAPPING_TERM 175
#define HOLD_ON_OTHER_KEY_PRESS

// Quick Tap: a fast re-press of the same key repeats the TAP instead of holding
// (stops a held "ZZ"/"BB"). Shorter than TAPPING_TERM so it only catches genuine
// double-taps.
#define QUICK_TAP_TERM 120

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
// Visible default so first flash obviously lights.
#define ENABLE_RGB_MATRIX_CYCLE_LEFT_RIGHT
#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_CYCLE_LEFT_RIGHT
#define RGB_MATRIX_DEFAULT_VAL 150
#define RGB_MATRIX_DEFAULT_SPD 100

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

// Sync WPM master->slave so the bongo cat animates on BOTH OLEDs (the slave
// half has no keypress data of its own; without this its cat never taps).
#define SPLIT_WPM_ENABLE
