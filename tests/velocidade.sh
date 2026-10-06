#!/bin/bash
# #202: velocidade de reproducao (lista, passo, rotulo, tempo de relogio).
#   bash tests/velocidade.sh
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-velocidade.XXXXXX")
trap 'rm -rf "$work"' EXIT
cc -std=gnu11 -O1 -g -Wall -Wextra -Werror -Isrc src/velocidade.c tests/velocidade.c -lm -o "$work/t"
"$work/t"
# A ilha e a contagem do proximo episodio contam em tempo de relogio.
grep -q 'vel_tempo_real((double)(duracaoSeg - posSeg), video_velocidade_atual())' src/player.c
grep -q 'vel_tempo_real(durSeg - posSeg, video_velocidade_atual())' src/posplay.c
# O .wgt nao oferece a linha (AVPlay setSpeed e trick play inteiro).
grep -q 'int  video_velocidade_suportada(void) { return 0; }' src/video_tizen.c
echo "velocidade: termina as, proximo episodio e .wgt conferidos"
