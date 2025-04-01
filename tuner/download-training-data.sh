#!/bin/bash

# Get the absolute path of the current script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Ensure the current directory is named "tuner"
CURRENT_DIR="$(basename "$SCRIPT_DIR")"
if [[ "$CURRENT_DIR" != "tuner" ]]; then
    echo "Error: This script must be run inside the 'tuner' directory."
    exit 1
fi

# Create the training-data directory if it doesn't exist
TARGET_DIR="$SCRIPT_DIR/training-data"
mkdir -p "$TARGET_DIR"

# URL of the file to download
URL="https://archive.org/download/lichess-big3-resolved.7z/lichess-big3-resolved.7z"

# Download the file using wget
echo "Downloading to $TARGET_DIR..."
# wget --continue -P "$TARGET_DIR" "$URL"

echo "Download complete."

pushd "$TARGET_DIR"
echo "Unpacking training data.."
7z x lichess-big3-resolved.7z 
popd
