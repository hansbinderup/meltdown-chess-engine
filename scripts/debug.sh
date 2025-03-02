#!/bin/bash

set -e

BUILD_DIR=".debug"

if [ ! -d "$BUILD_DIR" ]; then
    meson setup "$BUILD_DIR"
fi

ln -sf "$BUILD_DIR"/compile_commands.json .

# only run the actual executable - we don't care about "tests" here
meson test --gdb meltdown-chess-engine -C "$BUILD_DIR"
