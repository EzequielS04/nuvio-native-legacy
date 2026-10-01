#!/bin/bash
# Capturas do cartao da 1.6.6 em BMP: as tres cenas da previa (barra nova,
# carrossel, Live TV) em pt e en, momentos de cada uma, a Live TV em ru e as
# animacoes reduzidas. Antes, as regras: OK abre Ajustes no layout, Esquerda +
# OK o guia, Voltar = Agora nao, cima poe o foco na previa (direita/OK trocam
# a cena sem fechar), e todos gravam a marca. Janela GL ESCONDIDA e desenho
# num FBO: nada aparece na tela de quem roda. Fica fora da suite (*_shot.sh):
# precisa de GL e de olho humano. Sem rede: as artes sao as do pacote.
#
#   bash tests/novidades166_shot.sh /tmp/nuvio-novidades166
#   N166_SO=ja bash tests/novidades166_shot.sh      # so um grupo de capturas
set -eu
cd "$(dirname "$0")/.."
NUVIO_DADOS=$(mktemp -d /tmp/nuvio-n166-shot.XXXXXX)
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS"' EXIT
sources=()
for source in src/*.c; do
  case "$source" in */main.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/novidades166_shot.c -Isrc \
  -o /tmp/nuvio-n166-shot -O1 -g \
  -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wall -Wextra -Wno-deprecated-declarations -Wno-macro-redefined
/tmp/nuvio-n166-shot "${1:-/tmp/nuvio-novidades166}"
