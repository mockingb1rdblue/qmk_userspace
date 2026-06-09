// Keymap-level ChibiOS HAL config override (QMK searches KEYBOARD_PATH_5 = the
// keymap dir first; see platforms/chibios/platform.mk). Enables the I2C driver
// for the SSD1306 OLED. #include_next stacks this on the board defaults.
#pragma once

#define HAL_USE_I2C TRUE

#include_next <halconf.h>
