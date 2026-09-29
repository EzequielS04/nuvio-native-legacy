#!/bin/bash
# Grade curta do Xtream por canal (src/xtepg.c, #158). Ver tests/xtepg.c.
set -eu
cd "$(dirname "$0")/.."
flags=()
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc ${flags[@]+"${flags[@]}"} ${NUVIO_CFLAGS:-} src/xtepg.c tests/xtepg.c \
  -Isrc -o /tmp/nuvio-xtepg-tests -O1 -g -Wall -Wextra -lpthread
/tmp/nuvio-xtepg-tests
