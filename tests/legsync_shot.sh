#!/bin/bash
# Capturas da linha de AutoSync na folha de legendas. Nao entra na suite
# (*_shot.sh): janela GL. HTTP real contra servidor Range local.
#   bash tests/legsync_shot.sh /Volumes/ExternalSSD/nv-f05-shots/legsync
set -eu
cd "$(dirname "$0")/.."
T=${TMPDIR:-/tmp}; FX="$T/nv-legref-fx"
bash tests/legref_fixtures.sh "$FX"
sources=()
for source in src/*.c; do [ "$source" != src/main.c ] && sources+=("$source"); done
# LS_RITMO=0: sem o teto de 8 Ranges/s (servidor local).
cc "${sources[@]}" tests/legsync_shot.c -Isrc -DLS_RITMO=0 -o /tmp/nuvio-legsync-shot \
  -O1 -g -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wno-deprecated-declarations -Wno-macro-redefined
P=$((20000 + RANDOM % 20000))
D=$(mktemp -d)
python3 tests/legref_rangesrv.py "$FX" $P & S=$!
trap 'kill $S 2>/dev/null || true; rm -rf "$D"' EXIT
for i in $(seq 50); do nc -z 127.0.0.1 $P 2>/dev/null && break; sleep 0.1; done
mkdir -p "$(dirname "${1:-/tmp/nv-legsync}")"
NV_LEGSYNC_BASE="http://127.0.0.1:$P/f" NUVIO_DADOS="$D" NUVIO_TESTE_DIR="$D" /tmp/nuvio-legsync-shot "$@"
