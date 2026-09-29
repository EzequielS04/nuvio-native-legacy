#!/bin/bash
# Posteres personalizados (src/posterprov.c): URLs de cada provedor, token do
# manifest, redacao, falhas por item, disjuntor e portao. Sem rede.
#
#   bash tests/posterprov.sh
set -eu
cd "$(dirname "$0")/.."
cc -O1 -g -Wall -Wextra -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -Wno-deprecated-declarations -Wno-macro-redefined \
  src/posterprov.c src/redeurl.c tests/posterprov.c -lpthread -o /tmp/nuvio-posterprov-tests
/tmp/nuvio-posterprov-tests
