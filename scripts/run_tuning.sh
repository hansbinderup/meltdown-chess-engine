#!/bin/bash

set -e

BUILD_DIR=".build-tuning"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Setting up Meson..."
    meson setup "$BUILD_DIR" --cross-file targets/linux-native.txt --buildtype=release -Dtuning=true
    # meson setup "$BUILD_DIR" --cross-file targets/linux-native.txt --buildtype=debug -Dtuning=true
fi

ln -sf "$BUILD_DIR"/compile_commands.json .

# Compile
meson compile -C "$BUILD_DIR"

# xDDD
ulimit -s unlimited

"$BUILD_DIR/tuner/meltdown-tuner"

