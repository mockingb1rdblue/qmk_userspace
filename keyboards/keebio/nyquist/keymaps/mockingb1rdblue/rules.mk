CONSOLE_ENABLE = no         # Console for debug
COMMAND_ENABLE = no         # Commands for debug and configuration
NKRO_ENABLE = yes           # Enable N-Key Rollover
MOUSEKEY_ENABLE = no
EXTRAKEY_ENABLE = no
VIA_ENABLE = yes

# http://combos.gboards.ca/docs/install/
COMBO_ENABLE=yes
LTO_ENABLE = yes # Enables Link Time Optimization (LTO) when compiling the keyboard. This makes the process take longer, but it can significantly reduce the compiled size (and since the firmware is small, the added time is not noticeable

# Nyquist rev3 has WS2812 RGB underglow; required for RGB_TOG/RGB_MOD/RGB_HUI/RGB_SAI/RGB_VAI keycodes used in keymap
RGBLIGHT_ENABLE = yes
