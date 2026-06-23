#pragma once

// Split sides identify themselves by EE_HANDS (write side flag to EEPROM with
// `qmk flash ... -bl uf2-split-{left,right}`). Matches upstream bluebell/swoop.
#define EE_HANDS

// Heatmap persistence (heatmap.c): the per-(layer,LED) counts for both windows
// are saved to the user EEPROM datablock so the map survives sleep/power-down.
// Must equal sizeof(hm_persist_t): 4 (magic) + 2 * (HM_LAYERS=4 * HM_LEDS=36 *
// 2 bytes) = 580. A _Static_assert in heatmap.c enforces the match. Fits well
// inside the RP2040 wear-leveled logical EEPROM (4096 B).
#define EECONFIG_USER_DATA_SIZE 580

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
// Encoder: reversal-swallowed-detent fix (2026-06-12).
//
// ROOT CAUSE (encoder_quadrature.c lines 172-195):
//   encoder_pulses[i] += encoder_LUT[state & 0xF]  -- accumulates quadrature steps.
//   On fire the accumulator is reset via `encoder_pulses[i] %= resolution` (line 191).
//   With default ENCODER_RESOLUTION=4 this always yields 0 after a clean fire.
//   On direction reversal the accumulator starts from 0 and must accumulate a full
//   resolution (4 pulses) in the new direction before the next event fires.  A typical
//   mechanical detent produces ~2 LUT pulses, so one full detent is silently consumed
//   building the accumulator back up -- the "first notch after reversal does nothing".
//
// FIX: ENCODER_DEFAULT_POS selects the alternate branch (lines 174-195).
//   In this mode the driver fires as soon as pulses >= 1 or <= -1 AND resets to 0
//   (not %= resolution), which means a direction reversal fires on the very first
//   LUT step in the new direction.  The detent-position check `(state & 0x3) ==
//   ENCODER_DEFAULT_POS` also re-fires any accumulated partial count whenever the
//   shaft returns to its resting position, eliminating residual accumulator drift.
//   Value 0x3 = both pins HIGH = the standard at-rest state for most EC11 encoders.
//
// WHY THIS WORKS WITH ENCODER_MAP_ENABLE:
//   encoder_map is driven by encoder_handle_queue() in encoder.c, which calls
//   action_exec() on queued events.  ENCODER_DEFAULT_POS still calls
//   encoder_queue_event() at lines 181/189 -- the identical path -- so the map
//   fires with exactly the same mechanism as without the flag.
//
// Config-only fix; no core patch, no keymap shim, no submodule edit.
// ===========================================================================
#define ENCODER_DEFAULT_POS 0x3

// ===========================================================================
// RGB matrix: per-key ws2812 on D3/GP0, 36 LEDs (split 18/18).
// ===========================================================================
// ws2812.pin (D3/GP0) comes from keyboard.json; the vendor (PIO) driver needs a
// free PIO block on RP2040.
#define WS2812_PIO_USE_PIO1
#define RGB_MATRIX_LED_COUNT 36
#define RGB_MATRIX_SPLIT { 18, 18 }
#define RGB_MATRIX_MAXIMUM_BRIGHTNESS 150
// ONE AND ONLY RGB PATTERN (operator lock-in 2026-06-12): the per-layer neon
// keypress heatmap (heatmap_neon / heatmap.c) is the sole RGB Matrix effect.
// The press-to-light LED<->key MAPPING diagnostic (led_map / ledmap.c) did its
// job -- its result is baked into g_led_config (keymap.c) -- and is now RETIRED
// from the build (rgb_matrix_user.inc, rules.mk, keymap.c). No cycle fallback is
// compiled, so the effect enum holds exactly one custom effect: heatmap_neon.
#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_CUSTOM_heatmap_neon
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
