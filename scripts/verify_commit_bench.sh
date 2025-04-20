#!/bin/bash

set -e

# build for native (fastest)
scripts/build.sh

# run bench and extract nodes
echo "running bench..."
ENGINE_OUTPUT=$(./.build/meltdown-chess-engine bench)

# openbench expects: 'xxxx nodes xxxx nps' as last line
# extract the number in front of 'nodes' for last line
ENGINE_NODES=$(echo $ENGINE_OUTPUT | grep -oP '\d+(?=\s+nodes)' | tail -n1 || true)
if [ -z "$ENGINE_NODES" ]; then
    echo "::error title=verify-bench::Failed to extract nodes from engine's bench result"
    exit 1
fi

# Extract bench from commit message
# Format is: Bench XXXXXXX
# Prioritize last one if multiple are provided
# NOTE: --no-merges is added to support 'checkout action' from github actions
#       it adds a merge commit on top of the branch
COMMIT_MSG=$(git log --no-merges -1 --pretty=%B)
COMMIT_NODES=$(echo "$COMMIT_MSG" | grep -oP 'Bench \K[0-9]+' | tail -n1 || true)

if [ -z "$COMMIT_NODES" ]; then
    echo "Commit does not contain a bench"
    echo "::error title=verify-bench::Missing 'Bench $ENGINE_NODES' in commit message"
    exit 1
fi

# Compare bench from engine and commit message
if [ "$ENGINE_NODES" -eq "$COMMIT_NODES" ]; then
    echo "::notice title=verify-bench::Engine and commit bench is a match: $COMMIT_NODES"
    exit 0
else
    echo "Engine and commit nodes does not match: [$ENGINE_NODES != $COMMIT_NODES]"
    echo "::error title=verify-bench::Update 'Bench $ENGINE_NODES' in the commit message"
    exit 1
fi
