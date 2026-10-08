#!/bin/bash
# O SIGALRM de seguranca do SIGTERM (main.c, aoAlarmeTerminar) so pode fazer o
# que e seguro dentro de um handler e depois dar _exit. Antes ele chamava
# trailer_fechar/video_encerrar DE NOVO: se a saida normal estava no meio de
# video_encerrar (trava tomada) ou de um pthread_join, o handler travava na
# mesma trava e o processo so morria por SIGKILL. E o alarm(4) nunca era
# cancelado depois da saida normal.
#
#   bash tests/sigalarm_saida.sh
#
# Extrai o bloco NV_SINAL_TERMINAR do main.c de verdade (nao copia), compila
# com stubs e simula uma saida lenta: a "saida normal" segura a trava do
# video_encerrar, chega o SIGTERM, e o processo tem de sumir em ate 7 s sem KILL.
set -euo pipefail
cd "$(dirname "$0")/.."
export TMPDIR="${TMPDIR:-/Volumes/ExternalSSD/tmp}"
W="$(mktemp -d "$TMPDIR/nv-sigalarm.XXXXXX")"
trap 'rm -rf "$W"' EXIT
awk '/^#ifdef NV_SINAL_TERMINAR/ && !f {g=1}
     g && /SIGTERM \(deploy/ {f=1}
     f && /^#endif/ {exit}
     f {print}' src/main.c >"$W/bloco.h"
[ -s "$W/bloco.h" ] || { echo "FALHOU: bloco NV_SINAL_TERMINAR nao encontrado"; exit 2; }
cat >"$W/t.c" <<'C'
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
static pthread_mutex_t travaVideo = PTHREAD_MUTEX_INITIALIZER;
void trailer_fechar(void) {}
void video_encerrar(void) { pthread_mutex_lock(&travaVideo); pthread_mutex_unlock(&travaVideo); }
void ajustes_log_vazar_tudo(void) {}
#define NV_SINAL_TERMINAR 1
#include "bloco.h"
int main(void) {
  signal(SIGTERM, aoSinalTerminar);
  signal(SIGALRM, aoAlarmeTerminar);
  pthread_mutex_lock(&travaVideo);   // a saida normal esta DENTRO de video_encerrar
  raise(SIGTERM);
  sleep(60);                         // saida lenta: nunca termina sozinha
  return 0;
}
C
cc -O1 -g -I"$W" "$W/t.c" -o "$W/t" -lpthread -Wno-deprecated-declarations
"$W/t" & P=$!
for i in $(seq 1 28); do kill -0 "$P" 2>/dev/null || break; sleep 0.25; done
fail=0
if kill -0 "$P" 2>/dev/null; then
  echo "FALHOU: o handler do SIGALRM travou (reentrou em video_encerrar); so o KILL tira"
  kill -KILL "$P" 2>/dev/null || true; fail=1
fi
wait "$P" 2>/dev/null || true
# o alarm pendente tem de ser cancelado quando a saida normal termina
grep -q 'alarm(0)' src/main.c || { echo "FALHOU: main.c nao cancela o alarm (alarm(0)) ao fim da saida normal"; fail=1; }
[ $fail = 0 ] && echo "OK: SIGALRM sai sem reentrar na limpeza; alarm cancelado"
exit $fail
