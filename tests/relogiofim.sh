#!/bin/bash
# "Termina as HH:MM" do player cabe em todos os idiomas (issue #213).
set -eu
cd "$(dirname "$0")/.."
cc tests/relogiofim.c src/idioma.c -Isrc -I/opt/homebrew/include \
   -I/opt/homebrew/include/SDL2 -o /tmp/nuvio-relogiofim-tests \
   -Wall -Wno-macro-redefined -Wno-deprecated-declarations
/tmp/nuvio-relogiofim-tests
# Os dois lugares que desenham a frase usam o helper (nada de buffer proprio).
test "$(grep -c 'relogio_fim(fim, sizeof fim' src/player.c src/pausao.c | awk -F: '{s+=$2} END {print s}')" = 2
grep -q 'char hora\[8\], fim\[RELOGIO_FIM_MAX\]' src/player.c
grep -q 'fim\[RELOGIO_FIM_MAX\]' src/pausao.c
echo "relogiofim: player e pausa usam o helper"
