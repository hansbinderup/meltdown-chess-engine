#!/bin/bash

set -e

BUILD_DIR=".build"
RUN=false
OPTIMIZATION="dev"
SPSA=false
ARGS=()

# Parse flags
while [[ $# -gt 0 ]]; do
    case "$1" in
        -r) RUN=true ;;
        --release) OPTIMIZATION="release" ;;
        --spsa) SPSA=true ;;
        *) echo "Unknown option: $1" && exit 1 ;;
    esac
    shift
done

if [[ "$OPTIMIZATION" == "dev" ]]; then
    ARGS+=("-Ddeveloper-build=true")
fi

# custom build for SPSA to ensure proper meson setup
if $SPSA; then
    ARGS+=("-Dspsa=true")
    BUILD_DIR=".spsa-build"
fi

if [ ! -d "$BUILD_DIR" ]; then
    NATIVE_TARGET=$(scripts/get-native-target.sh)
    echo "Setting up Meson for $NATIVE_TARGET with ARGS: ${ARGS[*]}"
    meson setup "$BUILD_DIR" --cross-file "targets/$NATIVE_TARGET" --buildtype=release "${ARGS[@]}"
fi

ln -sf "$BUILD_DIR"/compile_commands.json .

# Compile
meson compile -C "$BUILD_DIR"

# Run the executable (if -r is passed or for SPSA builds)
if $RUN; then
    "$BUILD_DIR/meltdown-chess-engine"
fi

