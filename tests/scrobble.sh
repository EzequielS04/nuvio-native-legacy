#!/bin/bash
# Scrobble do Trakt (#179): sem rede e sem SDL; rede_postar_st e trakt_* sao
# dubles. Confere tambem que o token nao vai para o log.
set -eu
cd "$(dirname "$0")/.."
flags=(-O1 -g -Wall -Wextra -Isrc -Wno-macro-redefined -Wno-deprecated-declarations)
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc ${flags[@]+"${flags[@]}"} src/scrobble.c tests/scrobble.c -lpthread -o /tmp/nuvio-scrobble-tests
saida=$(/tmp/nuvio-scrobble-tests)
printf '%s\n' "$saida"
if printf '%s' "$saida" | grep -q SEGREDO; then echo "FALHA: token/chave no log"; exit 1; fi
printf '%s' "$saida" | grep -q "scrobble stop tt222 S1E2 83.3% -> 409" || { echo "FALHA: linha de log do stop"; exit 1; }
