#!/bin/bash
# Idioma automatico: mapa de codigos e precedencia (funcao pura). Ver o .c.
#
#   bash tests/idiomaauto.sh
set -eu
cd "$(dirname "$0")/.."
cc -Wall -Wextra tests/idiomaauto.c -Isrc -o /tmp/nuvio-idiomaauto
/tmp/nuvio-idiomaauto
