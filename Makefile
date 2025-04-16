# THIS MAKEFILE SHOULD ONLY BE USED FOR OPENBENCH
# PLEASE CHECK THE README FOR HOW TO BUILD MELTDOWN

ifndef EXE
$(error Makefile is only for OpenBench support - please check the README for how to build Meltdown)
endif

openbench:
	meson setup .openbench --cross-file targets/linux-native.txt --buildtype=release
	meson compile -C .openbench
	cp .openbench/meltdown-chess-engine $(EXE)
