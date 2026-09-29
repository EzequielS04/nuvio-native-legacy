#!/bin/bash
# Regras puras das fontes de nota (src/notasfontes.c): normalizacao, texto,
# ordem/prioridade, cores, resumo, encaixe da linha e grade de episodios.
set -eu
cd "$(dirname "$0")/.."
cc tests/notasfontes.c -Isrc -o /tmp/nuvio-notasfontes -Wall -Wextra
/tmp/nuvio-notasfontes
