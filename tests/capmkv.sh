#!/bin/bash
# 203-capitulos: capitulos do MKV no Android/.tpk (ver tests/capmkv.c).
#   bash tests/capmkv.sh
set -eu
cd "$(dirname "$0")/.."
DIR=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-capmkv.XXXXXX")
trap 'rm -rf "$DIR"' EXIT
cc -Isrc -O1 -g -Wall -Wextra -Wno-unused-function -Wno-deprecated-declarations tests/capmkv.c src/capmkv.c src/mkv.c -lpthread -o "$DIR/test"
"$DIR/test"
