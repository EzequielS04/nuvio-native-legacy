#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
DIR=$(mktemp -d /tmp/nuvio-redeurl.XXXXXX)
trap 'rm -rf "$DIR"' EXIT
cc -O1 -g -Wall -Wextra -fsanitize=address,undefined -Isrc \
  src/redeurl.c tests/redeurl.c -o "$DIR/teste"
"$DIR/teste"
