#!/bin/bash

BUILD_DIR=".build-tests"


if [ ! -d "$BUILD_DIR" ]; then
    meson setup "$BUILD_DIR" -Dunit-tests=true -Db_coverage=true
fi

# only run the unit tests - we don't want to start the application here
meson test unit-tests -C "$BUILD_DIR" --print-errorlogs --timeout=10

ln -sf "$BUILD_DIR"/compile_commands.json .

