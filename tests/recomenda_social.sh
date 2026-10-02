#!/bin/bash
# Redesenho do Social (cliente): nome, feed, perfil do amigo, merge, fila.
#   bash tests/recomenda_social.sh
set -eu
cd "$(dirname "$0")/.."
NUVIO_DADOS=$(mktemp -d /tmp/nuvio-recsoc-dados.XXXXXX)
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS" /tmp/nuvio-recomenda-social' EXIT
sources=()
for source in src/*.c; do
  case "$source" in src/main.c|src/recomenda.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/recomenda_social.c -Isrc -o /tmp/nuvio-recomenda-social \
  -O1 -g -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wall -Wextra -Wno-deprecated-declarations -Wno-macro-redefined 2>&1 | grep -E "recomenda(_social)?\.c.*(error|warning)" || true
/tmp/nuvio-recomenda-social
