// Keymap-level ChibiOS MCU config override. Enables RP2040 I2C1 (GP2/GP3 = the
// SSD1306 OLED bus). I2C1, not I2C0 -> RP_I2C_USE_I2C1. #include_next stacks this
// on the board defaults.
#pragma once

#include_next <mcuconf.h>

#undef RP_I2C_USE_I2C1
#define RP_I2C_USE_I2C1 TRUE
