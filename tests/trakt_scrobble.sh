#!/bin/bash
# Decisao e corpo do scrobble do Trakt (#179). Sem SDL e sem rede.
set -eu
cd "$(dirname "$0")/.."
cc src/traktscrobble.c tests/trakt_scrobble.c -Isrc -o /tmp/nuvio-trakt-scrobble-tests -O1 -g -Wall
/tmp/nuvio-trakt-scrobble-tests
