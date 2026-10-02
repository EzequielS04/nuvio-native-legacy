#!/bin/bash
# Seekr contra a API real (precisa de rede). Ver tests/seekr_vivo.c.
set -eu
cd "$(dirname "$0")/.."
cc src/seekr.c src/seekrvtt.c src/js.c src/rede.c src/redeurl.c src/jpegrapido.c tests/seekr_vivo.c \
  -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -L/opt/homebrew/lib \
  -lSDL2 -lSDL2_image -framework OpenGL -o /tmp/nuvio-seekr-vivo -O1 -g \
  -Wno-deprecated-declarations -Wno-macro-redefined
/tmp/nuvio-seekr-vivo
