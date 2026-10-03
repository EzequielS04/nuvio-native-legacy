#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-addon-subtitles.XXXXXX")
trap 'rm -rf "$work"' EXIT
cc tests/addons_legendas.c src/linguas.c src/js.c -Isrc \
  -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -std=gnu11 -pthread -ffunction-sections -fdata-sections -Wl,-dead_strip -Wall -Wextra -Wno-unused-function -o "$work/test"
"$work/test"
