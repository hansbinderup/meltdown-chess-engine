#!/bin/bash

args="--privileged \
    -u user \
    -v $(pwd):/workspaces/$(basename "$(pwd)") \
    -w /workspaces/$(basename "$(pwd)")"


docker run -it $args meson-gcc:1.0
