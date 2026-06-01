make crkbd/rev1/legacy:mocking:dfu-split-right
:avrdude-split-left

qmk flash -kb crkbd/rev1/legacy -km mocking -bl dfu-split-right
qmk flash -kb crkbd/rev1/legacy -km mocking -bl avrdude-split-left
