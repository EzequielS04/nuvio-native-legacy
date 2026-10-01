#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
cc -Wall -Wextra tests/livetv_regras.c -o /tmp/nuvio-livetv-regras && /tmp/nuvio-livetv-regras
