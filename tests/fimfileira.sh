#!/bin/bash
# Fim de fileira (issue #65): o foco chega a ultima coluna e continua recebendo
# seta, com republicacao concorrente do catalogo. Ver tests/fimfileira.c.
#
# Sem janela nem inicializacao SDL: o relogio e um duble deterministico do
# teste. SANITIZE=1 verifica a logica sem carregar SDL/AppKit no macOS.
set -eu
cd "$(dirname "$0")/.."
flags=()
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc ${flags[@]+"${flags[@]}"} src/catalogo.c src/progresso.c src/focus.c src/ajustes.c src/posterprov.c src/redeurl.c src/colecoes.c src/js.c src/catordem.c src/fileiras.c src/artehero.c src/cwordem.c tests/fimfileira.c \
  -Isrc -o /tmp/nuvio-fimfileira-tests -O1 -g -ffunction-sections -fdata-sections \
  -Wl,-dead_strip -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -Wno-deprecated-declarations -Wno-macro-redefined
/tmp/nuvio-fimfileira-tests "$@"
