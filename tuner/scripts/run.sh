#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CURRENT_DIR="$(pwd)"
SCRIPT_PARENT="$(dirname "$SCRIPT_DIR")"
SCRIPT_GRANDPARENT="$(dirname "$SCRIPT_PARENT")"

BUILD_DIR="$SCRIPT_GRANDPARENT/.build-tuning"
GENERATED_FILE="$SCRIPT_GRANDPARENT/src/evaluation/generated/tuned_terms.h"

source "$SCRIPT_DIR/build.sh"

cleanup() {
    clang-format -i "$GENERATED_FILE"
}

trap cleanup EXIT

# tuner consumes a fair amount of stack memory..
ulimit -s unlimited

"$BUILD_DIR/tuner/meltdown-tuner"
