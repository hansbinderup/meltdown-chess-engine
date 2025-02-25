#!/bin/bash

BUILD_DIR="debug"

meson setup "$BUILD_DIR"

# only run the actual executable - we don't care about "tests" here
meson test --gdb meltdown-chess-engine -C "$BUILD_DIR"

ln -sf "$BUILD_DIR"/compile_commands.json .
