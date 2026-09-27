#!/bin/bash
# #144: letra estilizada -> letra comum. Ver tests/dobra.c.
#   bash tests/dobra.sh
set -eu
cd "$(dirname "$0")/.."
cc src/dobra.c tests/dobra.c -Isrc -o /tmp/nuvio-dobra-tests -O1 -g -std=c11 -Wall -Wextra
/tmp/nuvio-dobra-tests
