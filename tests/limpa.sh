#!/bin/bash
# #144: texto de addon (emoji, bandeiras, versalete, matematicas...) sai so com
# glifos da Inter embarcada. Ver tests/limpa.c.
#   bash tests/limpa.sh
set -eu
cd "$(dirname "$0")/.."
python3 tools/glifos.py > /tmp/nuvio-limpa-inter.txt
cc src/limpa.c src/dobra.c tests/limpa.c -Isrc -o /tmp/nuvio-limpa-tests -O1 -g -std=c11 -Wall -Wextra -fsanitize=address,undefined
/tmp/nuvio-limpa-tests /tmp/nuvio-limpa-inter.txt tests/limpa_corpus.txt
