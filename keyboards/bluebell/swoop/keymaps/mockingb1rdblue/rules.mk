# TurkeyBoards Swoop variant: Sea-Picro controller (RP2040, SparkFun Pro Micro
# RP2040 pin-compatible). Compile upstream bluebell/swoop with QMK's stock
# Pro-Micro -> Pro-Micro-RP2040 converter; it rewrites every AVR pad name (D3,
# B4...) to its RP2040 GP equivalent at build time.
CONVERT_TO = promicro_rp2040

# DAILY DRIVER (2026-06-09): encoders confirmed alive on B4/B5 (GP8/GP9) by the
# pin-discovery probe + stock vendor Vial. Driver re-armed; encoder_map carries the
# preserved per-layer rotate bindings.
ENCODER_ENABLE     = yes
ENCODER_MAP_ENABLE = yes

# RGB: per-key matrix, ws2812 on D3/GP0, 36 LEDs split 18/18. ws2812 vendor driver
# = RP2040 PIO (bitbang is unreliable on RP2040). rgblight (underglow) stays OFF;
# the keyboard.json still declares an rgblight block but RGBLIGHT_ENABLE=no compiles
# it out, leaving the ws2812 data pin to RGB_MATRIX.
RGBLIGHT_ENABLE   = no
RGB_MATRIX_ENABLE = yes
RGB_MATRIX_DRIVER = ws2812
WS2812_DRIVER     = vendor
# Custom RGB Matrix effect (rgb_matrix_user.inc declares ONE -- operator lock-in
# 2026-06-12):
#   heatmap.c = per-layer neon keypress heatmap (the one and only RGB pattern).
# ledmap.c (press-to-light mapping diagnostic) is RETIRED from the build; its
# result is baked into g_led_config (keymap.c). File stays on disk, uncompiled.
RGB_MATRIX_CUSTOM_USER = yes
SRC += heatmap.c

# OLED: 128x32 SSD1306 over I2C (SSD1306 is the default driver). I2C pins are set
# in config.h (BEST-GUESS GP2/GP3, verify on hardware).
OLED_ENABLE = yes
OLED_DRIVER = ssd1306

# Combos: gboards combos.def -> keymap_combo.h (committed, no build-time tooling).
# http://combos.gboards.ca/docs/install/  ENTER and other keys are combos here.
COMBO_ENABLE = yes

# Userspace defaults
LTO_ENABLE      = yes
NKRO_ENABLE     = yes
MOUSEKEY_ENABLE = yes  # KC_WH_U / KC_WH_D encoder scroll on L0 right
EXTRAKEY_ENABLE = yes  # KC_WBAK / KC_WFWD browser keys on the encoder pushes
CONSOLE_ENABLE  = no
COMMAND_ENABLE  = no
