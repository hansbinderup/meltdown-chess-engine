#!/bin/bash

# Get the absolute path of the current script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCRIPT_PARENT="$(dirname "$SCRIPT_DIR")"

# Create the training-data directory if it doesn't exist
TARGET_DIR="$SCRIPT_PARENT/training-data"
mkdir -p "$TARGET_DIR"

# URL of the file to download
URL="https://archive.org/download/lichess-big3-resolved.7z/lichess-big3-resolved.7z"

# Download the file using wget
echo "Downloading to $TARGET_DIR..."
wget --continue -P "$TARGET_DIR" "$URL"

echo "Download complete."

pushd "$TARGET_DIR"
echo "Unpacking training data.."
7z x lichess-big3-resolved.7z 
popd
