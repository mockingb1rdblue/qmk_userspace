#pragma once

// Split sides identify themselves by EE_HANDS (write side flag to EEPROM with
// `qmk flash ... -bl uf2-split-{left,right}`). Matches upstream bluebell/swoop
// convention and the mockingb1rdblue_min bring-up keymap.
#define EE_HANDS

// Tap/hold tuning — carried over from crkbd/mockingb1rdblue (TAPPING_TERM 150).
#define TAPPING_TERM 150
