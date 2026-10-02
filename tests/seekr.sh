#!/bin/bash
# Seekr: leitura do /sprites, do VTT de miniaturas e escolha da cue.
#
#   bash tests/seekr.sh
#
# So src/seekrvtt.c e src/js.c — sem rede nem SDL.
set -eu
cd "$(dirname "$0")/.."
flags=()
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc ${flags[@]+"${flags[@]}"} src/seekrvtt.c src/js.c tests/seekr.c \
  -Isrc -o /tmp/nuvio-seekr-tests -O1 -g \
  -Wall -Wextra -Wno-unused-parameter -Wno-deprecated-declarations
/tmp/nuvio-seekr-tests
