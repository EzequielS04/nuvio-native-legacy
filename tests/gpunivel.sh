#!/bin/bash
# A regra do nivel de GPU adaptativo do .tpk (src/gpunivel.h), com dubles de
# gfx e dados: sem TV, sem janela. Ver tests/gpunivel.c.
set -eu
cd "$(dirname "$0")/.."
cc src/gpunivel.c src/perfiltv.c tests/gpunivel.c -Isrc -o /tmp/nuvio-gpunivel-teste \
  -DNV_GPUN_TESTE -O1 -g -Wall -Wextra -framework OpenGL -Wno-deprecated-declarations \
  -I/opt/homebrew/include -I/opt/homebrew/include/SDL2
/tmp/nuvio-gpunivel-teste
