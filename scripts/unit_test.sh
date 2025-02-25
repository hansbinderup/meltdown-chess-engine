#!/bin/bash

BUILD_DIR=".test"

meson setup "$BUILD_DIR" -Dunit-tests=true

# only run the unit tests - we don't want to start the application here
meson test unit-tests -C "$BUILD_DIR" --print-errorlogs

ln -sf "$BUILD_DIR"/compile_commands.json .

