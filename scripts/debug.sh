#!/bin/bash

BUILD_DIR="debug"

meson setup "$BUILD_DIR"

meson test --gdb -C "$BUILD_DIR"

