#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
cc tests/menu_geometry.c -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -Wno-deprecated-declarations -Wno-macro-redefined \
  -Wl,-undefined,dynamic_lookup -o /tmp/nuvio-menu-geometry
/tmp/nuvio-menu-geometry
