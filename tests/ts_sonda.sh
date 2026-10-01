#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
cc -Wall -Wextra tests/ts_sonda.c -o /tmp/nuvio-ts-sonda && /tmp/nuvio-ts-sonda
