#!/bin/bash
# Fila da grade curta do Xtream (src/xtepg.c) contra um painel falso com
# limite de pedidos por segundo. Ver tests/xtepg.c.
set -eu
cd "$(dirname "$0")/.."
cc -Wall tests/xtepg.c -Isrc -o /tmp/nuvio-xtepg -lpthread && /tmp/nuvio-xtepg
