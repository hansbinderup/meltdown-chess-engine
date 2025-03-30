#!/bin/bash

set -e  # Exit on any error

# Check for input
if [ -z "$1" ]; then
    echo "Usage: $0 <target-branch>"
    exit 1
fi

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
echo "Checking out $target_branch..."
git checkout "$target_branch"

# Run build
echo "Running build..."
scripts/build.sh
cp .build/meltdown-chess-engine tests/cute-chess/meltdown-baseline

# Return to original branch
echo "Returning to $current_branch..."
git checkout "$current_branch"

# Re-apply stashed changes if any
if $stash_applied; then
    echo "Reapplying stashed changes..."
    git stash pop
fi

echo "Done"

