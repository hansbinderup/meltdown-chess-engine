#!/bin/bash

set -e

BUILD_DIR=".build-tests"


if [ ! -d "$BUILD_DIR" ]; then
    meson setup "$BUILD_DIR" -Dunit-tests=true
fi

ln -sf "$BUILD_DIR"/compile_commands.json .

# only run the meltdown unit tests - we don't want to start the application here
meson test meltdown_test* -C "$BUILD_DIR" --print-errorlogs --timeout=10

# example: run gdb for a given test
# meson test meltdown_test_perft --gdb -C "$BUILD_DIR" --print-errorlogs --timeout=10
