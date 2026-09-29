#!/bin/bash
# Normalizacao da busca: latino, romeno, cirilico, UTF-8 (tests/buscanorm.c).
#
#   bash tests/buscanorm.sh
set -eu
cd "$(dirname "$0")/.."
cc tests/buscanorm.c src/buscanorm.c -Isrc -o /tmp/nuvio-buscanorm-tests -O1 -g -Wall -Wextra
/tmp/nuvio-buscanorm-tests
