#!/bin/bash
# F07 seek cache / volume boost state. NV_SANITIZERS=1 (ASan+UBSan) or
# NV_TSAN=1 for the concurrent reporter.
#   bash tests/cacheboost.sh
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-cacheboost.XXXXXX")
trap 'rm -rf "$work"' EXIT
flags=(-std=gnu11 -O1 -g -Wall -Wextra -Werror -Isrc)
if [ "${NV_SANITIZERS:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
if [ "${NV_TSAN:-0}" = 1 ]; then flags+=(-fsanitize=thread); fi
cc "${flags[@]}" src/cacheboost.c tests/cacheboost.c -lpthread -lm -o "$work/t"
"$work/t"
