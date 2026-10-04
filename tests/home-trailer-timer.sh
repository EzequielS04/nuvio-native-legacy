#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
EMSDK="/Users/hrocha/emsdk/upstream/emscripten"
PATH="$EMSDK:$PATH" emcc -O0 -std=gnu99 -D__EMSCRIPTEN__ \
  -I/opt/homebrew/include -Isrc \
  -ffunction-sections -fdata-sections \
  src/cwordem.c tests/home-trailer-timer.c tests/amigosfil_stub.c src/trailerfonte.c -Wl,--gc-sections \
  -o /tmp/nuvio-home-trailer-timer.js
node /tmp/nuvio-home-trailer-timer.js
# Native TPK scheduling is a different production branch: no WGT iframe,
# and the real Home source order can be IMDb -> Apple. Only these four units
# are linked; unused UI edges stay uncalled rather than compiling the core.
cc -DNV_TPK -O1 -g -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -Wno-return-stack-address -ffunction-sections -fdata-sections \
  src/cwordem.c tests/home-trailer-timer.c tests/amigosfil_stub.c src/trailerfonte.c \
  -Wl,-dead_strip,-undefined,dynamic_lookup -o /tmp/nuvio-home-trailer-tpk
/tmp/nuvio-home-trailer-tpk
