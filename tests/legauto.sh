#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-legauto.XXXXXX")
trap 'rm -rf "$work"' EXIT
cc tests/legauto.c -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -std=gnu11 -g -Wall -Wno-unused-function -Wno-macro-redefined -o "$work/t"
"$work/t"
