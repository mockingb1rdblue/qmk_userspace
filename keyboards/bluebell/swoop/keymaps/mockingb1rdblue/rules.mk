# TurkeyBoards Swoop variant — Sea-Picro controller (RP2040, SparkFun Pro Micro
# RP2040 pin-compatible per joshajohnson.com/sea-picro). Compile upstream
# bluebell/swoop with QMK's stock Pro-Micro -> Pro-Micro-RP2040 converter; the
# converter rewrites every AVR pad name (D3, B1, F4...) to its RP2040 GP equivalent
# at build time (see qmk_firmware/platforms/chibios/converters/promicro_to_promicro_rp2040/_pin_defs.h).
CONVERT_TO = promicro_rp2040

# PIN-DISCOVERY PROBE BUILD (2026-06-09): the rotary encoders register nothing on
# the upstream B4/B5 wiring. Disable the encoder driver so it stops owning GP8/GP9,
# letting keymap.c sweep those (and the other free GPIOs) as raw inputs to find the
# real encoder pins empirically. Restore both lines to re-arm the daily-driver
# encoders once the probe reports the wiring:
#   ENCODER_ENABLE     = yes
#   ENCODER_MAP_ENABLE = yes
ENCODER_ENABLE     = no
ENCODER_MAP_ENABLE = no

# RGB: vendor unit is per-key RGB matrix; target = 18/18 split, no underglow.
# Disabled in this scaffold — pin map and LED-position table require the vendor PCB.
# When PCB lands: flip RGB_MATRIX_ENABLE to yes and add the config.h block for
# RGB_MATRIX_LED_COUNT 36 + driver/data-pin overrides.
RGBLIGHT_ENABLE   = no
RGB_MATRIX_ENABLE = no

# Userspace defaults
LTO_ENABLE      = yes
NKRO_ENABLE     = yes
MOUSEKEY_ENABLE = yes  # required for KC_WH_U / KC_WH_D encoder scroll on L0 right
EXTRAKEY_ENABLE = yes  # browser/system keys (KC_WBAK / KC_WFWD) for future push binds
CONSOLE_ENABLE  = no
COMMAND_ENABLE  = no
