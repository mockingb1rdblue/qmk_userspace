make keebio/nyquist/rev3:mockingb1rdblue

# Half is declared in rev3 SPLIT_KEYBOARD pin on MCU

qmk flash -kb keebio/nyquist/rev3 -km mockingb1rdblue -bl dfu
# OR
make keebio/nyquist/rev3:default:flash
