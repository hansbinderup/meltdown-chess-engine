#!/bin/bash

set -e

BUILD_DIR=".build-debug"

if [ ! -d "$BUILD_DIR" ]; then
    NATIVE_TARGET=$(scripts/get-native-target.sh)
    echo "Setting up Meson for $NATIVE_TARGET"
    meson setup "$BUILD_DIR" --cross-file "targets/$NATIVE_TARGET"
fi

ln -sf "$BUILD_DIR"/compile_commands.json .

# only run the actual executable - we don't care about "tests" here
meson test --gdb meltdown-chess-engine -C "$BUILD_DIR"
