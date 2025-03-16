#!/bin/bash

set -e

# This script expects these env variables to be set:
#   VERSION (release prefix)

BUILD_DIR=".build-release"
RELEASE_DIR=".release"

# List of elements that we cross compile.
# They refer to the .txt files in our 'targets' folder
ARCHS=(
    "linux-gcc-x86-64-v2" 
    "linux-gcc-x86-64-v3"
)

# Ensure that we always have an empty release folder
rm -rf "$RELEASE_DIR" || true
mkdir -p "$RELEASE_DIR"

for ARCH in "${ARCHS[@]}"; do
    echo "Building for $ARCH..."

    # Clean build directory for each architecture
    rm -rf "$BUILD_DIR"
    meson setup "$BUILD_DIR" --cross-file targets/$ARCH.txt --wipe --buildtype=release -Dmeltdown-version="$VERSION"

    meson compile -C "$BUILD_DIR"

    # Rename the binary with architecture and version info 
    cp "$BUILD_DIR/meltdown-chess-engine" "$RELEASE_DIR/meltdown-$VERSION-$ARCH"

    echo "Finished building for $ARCH."
done

# Create a tarball with all builds
pushd "$RELEASE_DIR"
tar -czvf "meltdown-$VERSION.tar.gz" *
popd

echo "All builds completed successfully!"
