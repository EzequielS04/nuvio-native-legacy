#!/bin/bash
# Reconexao quando a rede cai no meio do video (src/video_reconexao.h).
set -eu
cd "$(dirname "$0")/.."
cc -O1 -g -Wall -Wextra -Werror tests/reconexao.c -o /tmp/nuvio-reconexao-tests
/tmp/nuvio-reconexao-tests
