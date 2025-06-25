# THIS MAKEFILE SHOULD ONLY BE USED FOR OPENBENCH
# PLEASE CHECK THE README FOR HOW TO BUILD MELTDOWN

ifndef EXE
$(error Makefile is only for OpenBench support - please check the README for how to build Meltdown)
endif

# *************************************************************
#                         SPSA TUNING
#
# For SPSA tuning add the flag "-Dspsa=true" to the meson setup
# More information can be found in the src/spsa/README.md
#
# FIXME: add compile argument to openbench
# *************************************************************

build_dir := .build-openbench

openbench:
	meson setup $(build_dir) --cross-file targets/linux-native.txt --wipe --buildtype=release
	meson compile -C $(build_dir)
	cp $(build_dir)/meltdown-chess-engine $(EXE)
