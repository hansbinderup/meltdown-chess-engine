#!/bin/bash

args="--privileged \
    -u user \
    -v $(pwd):/workspaces/$(basename "$(pwd)") \
    -w /workspaces/$(basename "$(pwd)")"


docker run -it $args hansbinderup/meson-gcc:1.2
