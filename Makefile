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

openbench:
	meson setup .openbench --cross-file targets/linux-native.txt --wipe --buildtype=release -Dspsa=true
	meson compile -C .openbench
	cp .openbench/meltdown-chess-engine $(EXE)
