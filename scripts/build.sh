#!/bin/bash

set -e

BUILD_DIR=".build"
RUN=false

# Parse flags
while [[ $# -gt 0 ]]; do
    case "$1" in
        -r) RUN=true ;;
        *) echo "Unknown option: $1" && exit 1 ;;
    esac
    shift
done

if [ ! -d "$BUILD_DIR" ]; then
    echo "Setting up Meson..."
    meson setup "$BUILD_DIR" --buildtype=release
fi

# Compile
meson compile -C "$BUILD_DIR"

# Run the executable (if -r is passed)
if $RUN; then
    echo "Running release build"
    "$BUILD_DIR/meltdown-chess-engine"
fi

ln -sf "$BUILD_DIR"/compile_commands.json .
