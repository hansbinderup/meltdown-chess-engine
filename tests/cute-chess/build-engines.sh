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

# Run dev build
echo "Building dev..."
meson setup "$BUILD_DIR" --wipe --cross-file "$ROOT/targets/$NATIVE_TARGET" --buildtype=release
meson compile -C "$BUILD_DIR"
mv "$BUILD_DIR/meltdown-chess-engine" "$SCRIPT_DIR/meltdown-dev"
rm -rf "$BUILD_DIR"

target_branch="$1"

# Save current branch name
current_branch=$(git rev-parse --abbrev-ref HEAD)

# Stash changes if needed
stash_applied=false
if ! git diff --quiet || ! git diff --cached --quiet; then
    echo "Stashing local changes..."
    git stash push -u -m "temp-stash-for-branch-switch"
    stash_applied=true
fi

# Switch to target branch
git checkout "$target_branch"

# Run base build
echo "Building base..."
meson setup "$BUILD_DIR" --wipe --cross-file "$ROOT/targets/$NATIVE_TARGET" --buildtype=release "${ARGS[@]}"
meson compile -C "$BUILD_DIR"
mv "$BUILD_DIR/meltdown-chess-engine" "$SCRIPT_DIR/meltdown-base"
rm -rf "$BUILD_DIR"

git checkout "$current_branch"

# Re-apply stashed changes if any
if $stash_applied; then
    git stash pop
fi


echo "Done"

