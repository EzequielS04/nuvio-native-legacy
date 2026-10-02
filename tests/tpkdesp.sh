#!/bin/bash
# Saida pela TV (Exit, desligar, janela escondida) nao conta como queda no .tpk.
set -eu
cd "$(dirname "$0")/.."
cc -fsanitize=address,undefined src/tpkdesp.c tests/tpkdesp.c -Isrc -o /tmp/nuvio-tpkdesp-tests -O1 -g -Wall
/tmp/nuvio-tpkdesp-tests
