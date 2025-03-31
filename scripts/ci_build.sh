#!/bin/bash
#!/bin/bash

set -e

BUILD_DIR=".build-ci"

# List of elements that we cross compile.
# They refer to the .txt files in our 'targets' folder
ARCHS=(
    "linux-native"
    "linux-gcc-x86-64-v2"
    "linux-gcc-x86-64-v3"
    "windows-gcc-x86-64-v2"
    "windows-gcc-x86-64-v3"
)

for ARCH in "${ARCHS[@]}"; do
    echo "Building for $ARCH..."

    # Clean build directory for each architecture
    rm -rf "$BUILD_DIR"
    meson setup "$BUILD_DIR" --cross-file targets/$ARCH.txt --wipe --buildtype=release
    meson compile -C "$BUILD_DIR"
done

