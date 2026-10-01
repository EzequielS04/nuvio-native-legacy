#!/bin/bash
# Capturas do Spotlight (src/spotlight.c) para revisao visual, com as
# conferencias de comportamento do .c (filtro, pedido, recentes, Voltar).
#
#   bash tests/spotlight_shot.sh /tmp/nuvio-spotlight
#
# Nao entra na suite (tools/testa-tudo.sh pula *_shot.sh): precisa de janela GL.
set -eu
cd "$(dirname "$0")/.."
saida="${1:-/tmp/nuvio-spotlight}"
tmp="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-spot-shot-XXXXXX")"
dados="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-spot-dados-XXXXXX")"
trap 'rm -rf "$tmp" "$dados"' EXIT
sources=()
for source in src/*.c; do
  [ "$source" = "src/main.c" ] && continue
  sources+=("$source")
done
cc "${sources[@]}" tests/spotlight_shot.c -Isrc -o "$tmp/shot" -O1 -g \
  -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -L/opt/homebrew/lib \
  -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wno-deprecated-declarations -Wno-macro-redefined
NUVIO_DADOS="$dados" "$tmp/shot" "$saida"
if command -v sips >/dev/null 2>&1; then
  for b in "$saida"-*.bmp; do
    sips -s format jpeg -Z 1400 "$b" --out "${b%.bmp}.jpg" >/dev/null
  done
fi
