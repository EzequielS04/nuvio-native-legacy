#include "streamfitdiag.h"
#include "streamfit.h"
#include "redemarca.h"
#include "vazao.h"
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

typedef struct {
  const StreamfitDiagControle *controle;
  int n, invalida;
  int kbps[STREAMFIT_AMOSTRAS_MAX];
} Medida;
static int parar(void *arg) {
  Medida *m = arg;
  return !m->controle->rede || redemarca_atual() != m->controle->rede ||
      (m->controle->cancelado && m->controle->cancelado(m->controle->usuario));
}
static void intervalo(const RedeIntervalo *v, void *arg) {
  Medida *m = arg;
  uint64_t taxa;
  if (!v->completa || v->ms < 1000) return; /* final partial interval */
  if (m->n >= STREAMFIT_AMOSTRAS_MAX || v->bytes > UINT64_MAX / 8) { m->invalida = 1; return; }
  taxa = v->bytes * 8 / v->ms; /* bits/ms == kbps; actual interval duration */
  if (taxa > 10000000) { m->invalida = 1; return; }
  m->kbps[m->n++] = (int)taxa; /* an actual zero-byte stall is evidence */
}
static int midia(const RedeResposta *r) {
  char prefixo[513];
  unsigned i = 0, n = r->n_prefixo;
  if (r->status != 200 && r->status != 206) return 0;
  /* A playlist/text/unknown octet stream cannot prove transferred video.
   * Missing MIME stays unknown, including older CORS/browser measurements. */
  if (strncasecmp(r->mime, "video/", 6) || strstr(r->mime, "mpegurl")) return 0;
  if (!n) return 0;
  for (; i < n; i++) prefixo[i] = (char)tolower(r->prefixo[i]);
  prefixo[n] = 0;
  i = 0;
  while (i < n && isspace((unsigned char)prefixo[i])) i++;
  if (prefixo[i] == '<' || prefixo[i] == '{' || prefixo[i] == '[' ||
      strstr(prefixo, "<!doctype") || strstr(prefixo, "<html")) return 0;
  return !vazao_url_aviso(r->final);
}
int streamfitdiag_medir(const char *url, const char *const *cab, int segundos,
    long inicio, uint64_t maxBytes, const StreamfitDiagControle *ctl,
    int *kbps, int nMax, RedeVazao *res, char *final, unsigned tamFinal) {
  Medida m = {0}; RedePedido p = {0}; RedeResposta r;
  const char *vet[66]; char faixa[64];
  int i, nc = 0, ok, n = 0, usada = 0;
  if (res) memset(res, 0, sizeof *res);
  if (final && tamFinal) final[0] = 0;
  if (!ctl || !ctl->rede || segundos < 1 || segundos > 60 || !maxBytes || !kbps || nMax < 1)
    return 0;
  m.controle = ctl;
  if (cab) for (; nc < 64 && cab[nc]; nc++) vet[nc] = cab[nc];
  if (cab && nc == 64 && cab[nc]) return 0;
  if (inicio > 0) { snprintf(faixa, sizeof faixa, "Range: bytes=%ld-", inicio); vet[nc++] = faixa; }
  vet[nc] = NULL;
  p.url = url; p.cabecalhos = vet; p.seguir = 1;
  p.prazo_ms = ctl->prazo_ms ? ctl->prazo_ms : (unsigned)segundos * 1000 + 8000;
  p.janela_corpo_ms = (unsigned)segundos * 1000; p.max_descartado = maxBytes;
  p.intervalo = intervalo; p.intervalo_usuario = &m;
  p.parar = parar; p.parar_usuario = &m; p.ca_arquivo = ctl->ca_arquivo;
  ok = rede_pedir(&p, &r);
  if (final && tamFinal) snprintf(final, tamFinal, "%s", r.final);
  if (res) {
    res->status = r.status;
    res->erro = ok ? 0 : r.curl_erro ? r.curl_erro : (int)r.erro;
    res->bytes = (long long)r.bytes_fio;
    res->ms = r.corpo_ms; res->esperaMs = r.primeiro_byte_ms;
    res->cancelado = r.erro == REDE_CANCELADO || r.erro == REDE_GERACAO;
  }
  if (ok && !r.fim_teto && !m.invalida && midia(&r) && !parar(&m)) {
    n = m.n < nMax ? m.n : nMax;
    for (i = 0; i < n; i++) kbps[i] = m.kbps[i];
    usada = streamfit_diagnostico(ctl->rede, r.final, m.kbps, m.n, streamfit_agora_ms());
  }
  /* No URL, title, host path, network identifier or credentials in the log. */
  printf("[stream_fit] diagnostic accepted=%d intervals=%d http=%d error=%d\n",
         usada, m.n, r.status, (int)r.erro);
  rede_resposta_limpar(&r);
  return n;
}
