#!/bin/bash
# Conferir varias fontes ao mesmo tempo (2.0.2, opcional): a ordem da fila vale
# sempre. Ver tests/fonteparalela.c.
#
#   bash tests/fonteparalela.sh
set -eu
cd "$(dirname "$0")/.."
bin="${TMPDIR:-/tmp}/nuvio-fonteparalela-tests"
cc -O1 -g -Wall -Wextra -Isrc src/fonteparalela.c src/fonteauto.c tests/fonteparalela.c -o "$bin" -lpthread
"$bin"

# CONTRATO: o paralelismo so existe atras do ajuste (desligado de fabrica), so
# nas primeiras 3 e so em "Melhor fonte" (issue #130: uma por vez e o padrao).
corpo=$(awk '/^int stream_primeira_boa\(/,/^}/' src/streams.c)
if ! printf '%s' "$corpo" | grep -q 'ajustes_fonte_conferir_varias()' ||
   ! printf '%s' "$corpo" | grep -q 'fonteparalela('; then
  echo "fonteparalela: stream_primeira_boa nao usa fonteparalela atras do ajuste"; exit 1
fi
if printf '%s' "$corpo" | grep -v 'ajustes_fonte_conferir_varias' | grep -q 'pthread_create'; then
  echo "fonteparalela: stream_primeira_boa abre fios por fora do ajuste"; exit 1
fi
if ! grep -q 'valorPadrao\|^  1, /\* check several sources at once' src/ajustes_ux_padrao.inc; then
  echo "fonteparalela: padrao do ajuste sumiu"; exit 1
fi
echo "fonteparalela: contrato de streams.c ok"
