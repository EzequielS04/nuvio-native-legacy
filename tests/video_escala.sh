#!/bin/bash
# Escala do retangulo de video (layout 1920x1080 -> drawable), issue #176.
set -eu
cd "$(dirname "$0")/.."
cc -O1 -g -Wall -Wextra tests/video_escala.c -o /tmp/nuvio-video-escala-tests
/tmp/nuvio-video-escala-tests
