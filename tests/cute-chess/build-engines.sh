#!/bin/bash

set -e  # Exit on any error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CURRENT_DIR="$(pwd)"
SCRIPT_PARENT="$(dirname "$SCRIPT_DIR")"
ROOT="$(dirname "$SCRIPT_PARENT")"
BUILD_DIR="$SCRIPT_DIR/.build-tournament"
NATIVE_TARGET=$($ROOT/scripts/get-native-target.sh)

# Check for input
if [ -z "$1" ]; then
    echo "Usage: $0 <target-branch>"
    exit 1
fi


if ! git diff --quiet || ! git diff --cached --quiet; then
    echo "Stash or commit local changes.." > /dev/tty
    exit 1
fi

# current branch is gonna be dev
current_branch=$(git rev-parse --abbrev-ref HEAD)

# ensure that we always return to current branch if something goes wrong
cleanup() {
    git checkout "$current_branch"
}

trap cleanup EXIT

# base branch is gonna be target
target_branch="$1"

# Switch to target branch
git checkout "$target_branch"

# Run base build
echo "Building base [$target_branch]" > /dev/tty
meson setup "$BUILD_DIR" --wipe --cross-file "$ROOT/targets/$NATIVE_TARGET" --buildtype=release "${ARGS[@]}"
meson compile -C "$BUILD_DIR"
mv "$BUILD_DIR/meltdown-chess-engine" "$SCRIPT_DIR/meltdown-base"
rm -rf "$BUILD_DIR"

git checkout "$current_branch"

# Run dev build
echo "Building dev [$current_branch]" > /dev/tty
meson setup "$BUILD_DIR" --wipe --cross-file "$ROOT/targets/$NATIVE_TARGET" --buildtype=release
meson compile -C "$BUILD_DIR"
mv "$BUILD_DIR/meltdown-chess-engine" "$SCRIPT_DIR/meltdown-dev"
rm -rf "$BUILD_DIR"

echo "Done"

