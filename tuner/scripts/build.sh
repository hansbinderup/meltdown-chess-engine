#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CURRENT_DIR="$(pwd)"
SCRIPT_PARENT="$(dirname "$SCRIPT_DIR")"
SCRIPT_GRANDPARENT="$(dirname "$SCRIPT_PARENT")"

BUILD_DIR="$SCRIPT_GRANDPARENT/.build-tuning"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Setting up Meson..."
    meson setup "$BUILD_DIR" --cross-file "$SCRIPT_GRANDPARENT/targets/linux-native.txt" --buildtype=release -Dtuning=true
fi

ln -sf "$BUILD_DIR"/compile_commands.json "$SCRIPT_GRANDPARENT"

meson compile -C "$BUILD_DIR"
