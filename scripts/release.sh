#!/bin/bash

BUILD_DIR="release"
SETUP=false
RUN=false

# Parse flags
while [[ $# -gt 0 ]]; do
    case "$1" in
        -s) SETUP=true ;;
        -r) RUN=true ;;
        *) echo "Unknown option: $1" && exit 1 ;;
    esac
    shift
done

# Meson setup (if -s is passed)
if $SETUP; then
    echo "Setting up Meson..."
    meson setup "$BUILD_DIR" --buildtype=release -Doptimization=3
fi

# Compile
meson compile -C "$BUILD_DIR"

# Run the executable (if -r is passed)
if $RUN; then
    echo "Running release build"
    "$BUILD_DIR/meltdown-chess-engine"
fi
