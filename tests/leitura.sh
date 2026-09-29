#!/bin/bash
# O extrator de noticia (src/leitura.c): paginas reais em tests/fixtures,
# JSON-LD, Latin-1 e as pecas do Google News. Sem rede, sem SDL.
#
#   bash tests/leitura.sh
set -eu
cd "$(dirname "$0")/.."
cc src/leitura.c tests/leitura.c -Isrc -o /tmp/nuvio-leitura -O1 -g \
  -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra
/tmp/nuvio-leitura "$@"
