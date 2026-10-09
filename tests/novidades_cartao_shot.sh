#!/bin/bash
# Capturas do cartao de novidades da versao atual (novidades_cartao.h) em PNG
# 1920x1080: cada pagina, a tela do Dolby Vision e os Ajustes na previa, a
# Samsung sem Dolby Vision, ingles, alemao, japones, russo e animacoes
# reduzidas; no fim, a lista cabe nos 30 idiomas. Janela GL ESCONDIDA e
# desenho num FBO. Fica fora da suite (*_shot.sh): precisa de GL e de olho
# humano. Sem rede: as artes sao as do pacote (deploy/app/art).
#
#   NUVIO_SHOT_FONTE=3 bash tests/novidades_cartao_shot.sh /tmp/nuvio-novcartao
#   (NUVIO_SHOT_FONTE=3 = Montserrat, a fonte da TV; NUVIO_SHOT_SO=medir pula as capturas)
set -euo pipefail
cd "$(dirname "$0")/.."
NUVIO_DADOS=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-novcartao-shot.XXXXXX")
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS"' EXIT
saida="${1:-/tmp/nuvio-novcartao}"
mkdir -p "$saida"
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in */main.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/novidades_cartao_shot.c -Isrc \
  -o "$NUVIO_DADOS/shot" -O1 -g \
  -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wno-deprecated-declarations -Wno-macro-redefined
"$NUVIO_DADOS/shot" "$saida"
