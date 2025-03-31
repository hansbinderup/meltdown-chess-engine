#!/bin/bash

set -e

if [[ "$(uname -s)" == "Linux" ]]; then
    echo "linux-native.txt"
elif [[ "$(uname -s)" =~ MINGW|MSYS|CYGWIN ]]; then
    echo "windows-native.txt"
else
    echo "Error: native build was not detected" >&2
    exit 1
fi

exit 0

