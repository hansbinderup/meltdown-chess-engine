#!/bin/bash

set -e

# This script expects these env variables to be set:
#   VERSION (release prefix)

echo "New commits in last 24 hours: $NEW_COMMIT_COUNT"
if [[ "$NEW_COMMIT_COUNT" -eq 0 ]]; then
    # skip the build if no new commits and if the build was triggered by the scheduler
    # NOTE: we should always allow builds to be triggered manually, so only skip automated ones
    if [[ "$GITHUB_EVENT_NAME" == "schedule" ]]; then
        echo "No new commits. Skipping workflow."
        exit 0 # exit quietly - we don't want the job to look like it's "failed"
    fi
fi

BUILD_DIR=".build-release"
RELEASE_DIR=".release"

# ensure that we always have an empty release folder
rm -rf $RELEASE_DIR || true
mkdir $RELEASE_DIR

# NOTE: we always want a clean build when building a release
meson setup "$BUILD_DIR" --wipe --buildtype=release -Dmeltdown-version=$VERSION
meson compile -C "$BUILD_DIR"

# no need to store "chess engine" in our release name - rename and append version
cp "$BUILD_DIR/meltdown-chess-engine" "$RELEASE_DIR/meltdown-$VERSION"

# add all release files to a tar ball - will be useful when we have multiple executables
pushd $RELEASE_DIR
tar -czvf meltdown-$VERSION.tar.gz *
popd
