// Ver legsync.h. Nucleo sem SDL/GL: o desenho e os textos ficam em legsyncui.c.
#include "legsync.h"
#include "autosync.h"
#include "legenda.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LS_SESSAO_BYTES  (24LL * 1024 * 1024)  // todas as referencias de uma midia
#define LS_LEITURA_BYTES (12LL * 1024 * 1024)  // uma faixa
#define LS_PEDIDOS       6000
#ifndef LS_RITMO
#define LS_RITMO         8                     // Ranges por segundo, como o mkvass
#endif
#define LS_CALMO_MS      2000u
#define LS_FOLGA_MIN     20.0                  // buffer de video abaixo disto pausa a leitura

static pthread_mutex_t M = PTHREAD_MUTEX_INITIALIZER;
static struct {
  int criado;
  AutoSync *sync;
  LegRef *ref;
  LegRefLer lerTeste; void *lerTesteU;
  uint64_t sessao, urlHash;
  char url[4096];                 // privado: nunca vai a log
  // principal
  int primTipo;                   // 0 nenhuma, 1 externa, 2 embutida/nativa
  uint64_t primToken;
  unsigned primGer;
  int primFase;                   // 0 baixando, 1 pronta e completa, 2 falhou/incompleta
  LegendaDocumento *primDoc;
  char primIdioma[24];
  // referencia
  uint64_t refPedido;
  LegendaDocumento *refDoc;
  int refFaixa, ultimaFaixa;
  int excl[16], nExcl;
  long long bytesSessao;
  LegRefMotivo refMotivo;
  char idiomaRef[24];
  // analise
  int querModo;                   // -1 nada; senao AutoSyncModo a pedir quando houver referencia
  int ultimoModo, solicitou, desfeita, reenviar, semOutra;
  unsigned calmoDesde;
} L = { .querModo = -1 };

static uint64_t fnv(const char *s) {
  uint64_t h = UINT64_C(14695981039346656037);
  for (; s && *s; s++) { h ^= (unsigned char)*s; h *= UINT64_C(1099511628211); }
  return h;
}

static void soltarRef(void) {
  legenda_documento_liberar(L.refDoc); L.refDoc = NULL; L.refFaixa = 0;
}

static void zerarAnalise(void) {
  L.querModo = -1; L.solicitou = L.desfeita = L.reenviar = L.semOutra = 0;
}

// Geracao nova para a MESMA escolha de legenda (troca de fonte no meio da
// sessao) ou para um titulo novo (manterPrimaria = 0).
static void novaSessao(const char *url, int manterPrimaria) {
  L.sessao++;
  autosync_iniciar(L.sync, L.sessao);
  legref_cancelar(L.ref);
  L.refPedido = 0; soltarRef();
  L.nExcl = 0; L.bytesSessao = 0; L.refMotivo = LEGREF_OK; L.ultimaFaixa = 0;
  L.idiomaRef[0] = 0;
  zerarAnalise();
  snprintf(L.url, sizeof L.url, "%s", url ? url : "");
  L.urlHash = fnv(L.url);
  if (!manterPrimaria) {
    L.primToken++; L.primTipo = 0; L.primFase = 0; L.primGer = 0;
    legenda_documento_liberar(L.primDoc); L.primDoc = NULL; L.primIdioma[0] = 0;
  }
}

void legsync_teste_leitor(LegRefLer ler, void *u) {
  pthread_mutex_lock(&M); L.lerTeste = ler; L.lerTesteU = u; pthread_mutex_unlock(&M);
}

void legsync_iniciar(const char *url) {
  pthread_mutex_lock(&M);
  if (!L.criado) {
    // Um fio da engine e um do coletor, ociosos ate haver pedido. Criar nao
    // faz rede nem espera nada: pode rodar no player_abrir.
    L.sync = autosync_criar();
    L.ref = (L.lerTeste || legref_disponivel()) ? legref_criar(L.lerTeste, L.lerTesteU) : NULL;
    L.criado = L.sync != NULL;
    if (!L.criado) { legref_destruir(L.ref); L.ref = NULL; pthread_mutex_unlock(&M); return; }
  }
  novaSessao(url, 0);
  pthread_mutex_unlock(&M);
}

void legsync_encerrar(void) {
  pthread_mutex_lock(&M);
  if (L.criado) novaSessao("", 0);
  pthread_mutex_unlock(&M);
}

void legsync_destruir(void) {
  AutoSync *s; LegRef *r; LegendaDocumento *a, *b;
  pthread_mutex_lock(&M);
  s = L.sync; r = L.ref; a = L.primDoc; b = L.refDoc;
  L.sync = NULL; L.ref = NULL; L.primDoc = L.refDoc = NULL; L.criado = 0;
  L.primToken++; L.refPedido = 0; L.primTipo = 0;
  pthread_mutex_unlock(&M);
  // Join fora do lock: o fio do download pode estar esperando M em aoBaixar.
  autosync_destruir(s);
  legref_destruir(r);
  legenda_documento_liberar(a); legenda_documento_liberar(b);
}

// --- PRINCIPAL ------------------------------------------------------------------
typedef struct {
  uint64_t token, sessao;
  LegendaDocumentoInfo info;
} Meta;

// Fio do download (legenda.c). Monta o documento FORA do lock e publica so se
// esta escolha ainda e a vigente.
static void aoBaixar(const char *corpo, unsigned g, int vigente, void *u) {
  Meta *m = u; LegendaDocumento *doc = NULL, *velho = NULL;
  if (corpo && vigente) doc = legenda_documento_criar(corpo, &m->info);
  pthread_mutex_lock(&M);
  if (L.criado && m->token == L.primToken) {
    velho = L.primDoc; L.primDoc = NULL;
    if (doc && (legenda_documento_info(doc)->flags & LEGENDA_DOC_COMPLETO)) {
      L.primDoc = legenda_documento_reter(doc); L.primGer = g; L.primFase = 1;
      if (m->sessao == L.sessao) autosync_selecionar(L.sync, 0, doc);
    } else L.primFase = 2;
  }
  pthread_mutex_unlock(&M);
  legenda_documento_liberar(velho);
  legenda_documento_liberar(doc);
  free(m);
}

void legsync_primaria_externa(const char *url, const char *idioma, const char *origem) {
  Meta *m = calloc(1, sizeof *m);
  LegendaDocumento *velho = NULL;
  if (!url || !*url) { free(m); return; }
  pthread_mutex_lock(&M);
  if (m && L.criado) {
    uint64_t h = fnv(url);
    m->token = ++L.primToken; m->sessao = L.sessao;
    m->info.sessao = L.sessao; m->info.flags = LEGENDA_DOC_COMPLETO;
    snprintf(m->info.idioma, sizeof m->info.idioma, "%s", idioma ? idioma : "");
    snprintf(m->info.origem, sizeof m->info.origem, "%s", origem && *origem ? origem : "Addon");
    // Identidade opaca: hash da URL, nunca a URL (pode ser assinada).
    snprintf(m->info.identidade, sizeof m->info.identidade, "addon:%016llx", (unsigned long long)h);
    L.primTipo = 1; L.primFase = 0; L.primGer = 0;
    velho = L.primDoc; L.primDoc = NULL;
    snprintf(L.primIdioma, sizeof L.primIdioma, "%s", m->info.idioma);
    autosync_selecionar(L.sync, 0, NULL);
    // A referencia embutida continua valendo para outra externa da mesma
    // midia; as exclusoes eram do par anterior.
    L.nExcl = 0; zerarAnalise();
    if (L.refPedido) { legref_cancelar(L.ref); L.refPedido = 0; }   // pedido com exclusoes do par antigo
    if (!L.refDoc) L.refMotivo = LEGREF_OK;
  } else { free(m); m = NULL; }
  pthread_mutex_unlock(&M);
  legenda_documento_liberar(velho);
  if (m) legenda_carregar_com(url, aoBaixar, m);
  else legenda_carregar(url);
}

void legsync_primaria_outra(int embutida) {
  LegendaDocumento *velho;
  pthread_mutex_lock(&M);
  L.primToken++; L.primTipo = embutida ? 2 : 0; L.primFase = 0; L.primGer = 0;
  velho = L.primDoc; L.primDoc = NULL;
  if (L.criado) {
    autosync_selecionar(L.sync, 0, NULL);
    legref_cancelar(L.ref); L.refPedido = 0;
  }
  zerarAnalise();
  pthread_mutex_unlock(&M);
  legenda_documento_liberar(velho);
}

// Documento da principal na geracao ATUAL (a fonte pode ter trocado depois
// do download). Com M.
static int garantirPrimaria(void) {
  const LegendaDocumentoInfo *i = legenda_documento_info(L.primDoc);
  if (!L.primDoc || L.primFase != 1) return 0;
  if (i->sessao != L.sessao) {
    int n = 0; const LegendaCue *v = legenda_documento_dados(L.primDoc, &n);
    LegendaDocumentoInfo ni = *i; LegendaDocumento *d;
    ni.sessao = L.sessao;
    d = legenda_documento_de_cues(v, n, &ni);
    if (!d) return 0;
    legenda_documento_liberar(L.primDoc); L.primDoc = d;
  }
  return autosync_selecionar(L.sync, 0, L.primDoc);
}

static int dona(void) {
  // O overlay mudou de dono por outro caminho (mkvass, desligar): o
  // documento antigo nao descreve o que esta na tela.
  return L.primTipo == 1 && L.primFase == 1 && L.primDoc && legenda_geracao() == L.primGer;
}

int legsync_offset_ms(int manual) {
  int total = manual;
  unsigned g = legenda_geracao();      // fora de M: legenda.c tem lock proprio
  pthread_mutex_lock(&M);
  if (L.criado && L.primTipo == 1 && L.primFase == 1 && L.primDoc && g == L.primGer &&
      legenda_documento_info(L.primDoc)->sessao == L.sessao) {
    autosync_manual(L.sync, 0, manual);
    total = autosync_offset_ms(L.sync, 0);
  }
  pthread_mutex_unlock(&M);
  return total;
}

// --- REFERENCIA ------------------------------------------------------------------
static void iniciarColeta(void) {
  LegRefOrcamento o;
  long long resta = LS_SESSAO_BYTES - L.bytesSessao;
  if (!L.ref) { L.refMotivo = LEGREF_PLATAFORMA; L.querModo = -1; return; }
  if (resta < 512 * 1024) { L.refMotivo = LEGREF_ORCAMENTO; L.querModo = -1; return; }
  o.maxBytes = resta < LS_LEITURA_BYTES ? resta : LS_LEITURA_BYTES;
  o.maxPedidos = LS_PEDIDOS; o.pedidosPorSeg = LS_RITMO;
  L.refMotivo = LEGREF_OK;
  L.refPedido = legref_pedir(L.ref, L.url, L.sessao, L.primIdioma, L.excl, L.nExcl, &o);
  if (!L.refPedido) { L.refMotivo = LEGREF_SEM_FAIXA; L.querModo = -1; }
}

static int solicitar(int modo) {
  AutoSyncConfig c = autosync_config(modo == AUTOSYNC_THOROUGH ? AUTOSYNC_THOROUGH : AUTOSYNC_QUICK);
  // Tolerancia: nao ha ajuste em Ajustes; vale o padrao da engine (250 ms).
  if (!L.refDoc || !garantirPrimaria()) return 0;
  if (!autosync_referencia_permitida(L.sync, 0, L.refDoc)) { L.semOutra = 1; return 0; }
  if (!autosync_solicitar(L.sync, 0, L.refDoc, &c)) return 0;
  L.solicitou = 1; L.querModo = -1; L.reenviar = 0; L.desfeita = 0;
  return 1;
}

int legsync_acao(int acao) {
  int ok = 0;
  pthread_mutex_lock(&M);
  if (!L.criado) { pthread_mutex_unlock(&M); return 0; }
  switch (acao) {
    case LEGSYNC_ACAO_RAPIDA: case LEGSYNC_ACAO_COMPLETA: {
      int modo = acao == LEGSYNC_ACAO_COMPLETA ? AUTOSYNC_THOROUGH : AUTOSYNC_QUICK;
      if (!dona() || !L.ref) break;
      L.ultimoModo = modo; L.desfeita = 0;
      if (L.refDoc) ok = solicitar(modo);
      else {
        L.querModo = modo; ok = 1;
        if (!L.refPedido) iniciarColeta();
        if (!L.refPedido) ok = 0;
      }
      break; }
    case LEGSYNC_ACAO_DESFAZER:
      autosync_desfazer(L.sync, 0);       // mantem o manual
      L.desfeita = 1; L.querModo = -1; L.reenviar = 0; ok = 1;
      break;
    case LEGSYNC_ACAO_OUTRA: {
      int f = L.refFaixa ? L.refFaixa : L.ultimaFaixa, k, ja = 0;
      if (!dona() || !L.ref || L.semOutra) break;
      if (L.refDoc && L.solicitou) autosync_tentar_outra(L.sync, 0);
      for (k = 0; k < L.nExcl; k++) if (L.excl[k] == f) ja = 1;
      if (f > 0 && !ja && L.nExcl < 16) L.excl[L.nExcl++] = f;
      soltarRef();
      L.solicitou = 0; L.desfeita = 0;
      L.querModo = L.ultimoModo; iniciarColeta();
      ok = L.refPedido != 0;
      break; }
    case LEGSYNC_ACAO_PARAR:
      legref_cancelar(L.ref); L.refPedido = 0;
      autosync_cancelar(L.sync, 0);
      L.querModo = -1; L.reenviar = 0; ok = 1;
      break;
  }
  pthread_mutex_unlock(&M);
  return ok;
}

void legsync_passo(const char *url, double pos, double folga, int sensivel, unsigned agora) {
  (void)pos;
  pthread_mutex_lock(&M);
  if (!L.criado) { pthread_mutex_unlock(&M); return; }
  // Troca de fonte no meio da sessao: outra midia, outra geracao. A escolha
  // de legenda da pessoa continua; a referencia e o offset automatico nao.
  if (url && *url && fnv(url) != L.urlHash) novaSessao(url, 1);
  if (L.refPedido) {
    LegRefStatus st = legref_status(L.ref);
    if (st.pedido == L.refPedido) {
      if (st.faixa) { L.ultimaFaixa = st.faixa; snprintf(L.idiomaRef, sizeof L.idiomaRef, "%s", st.idioma); }
      if (st.fase == LEGREF_PRONTO) {
        L.refDoc = legref_tomar(L.ref, L.refPedido);
        L.refFaixa = st.faixa; L.bytesSessao += st.bytes; L.refPedido = 0;
        if (!L.refDoc) { L.refMotivo = LEGREF_INCOMPLETO; L.querModo = -1; }
      } else if (st.fase == LEGREF_INDISPONIVEL || st.fase == LEGREF_CANCELADO) {
        L.refMotivo = st.motivo; L.bytesSessao += st.bytes; L.refPedido = 0; L.querModo = -1;
        if (st.motivo == LEGREF_SEM_FAIXA && L.nExcl > 0) L.semOutra = 1;
      }
    }
  }
  // Competicao: seek/buffer pausa a leitura; buffer de video curto tambem.
  legref_pausar(L.ref, sensivel || (folga >= 0.0 && folga < LS_FOLGA_MIN));
  if (sensivel) {
    AutoSyncResultado e = autosync_estado(L.sync, 0);
    L.calmoDesde = 0;
    if (e.estado == AUTOSYNC_ANALYSING) { autosync_cancelar(L.sync, 0); L.reenviar = 1; }
  } else {
    if (!L.calmoDesde) L.calmoDesde = agora | 1u;
    if ((L.reenviar || L.querModo >= 0) && L.refDoc && agora - L.calmoDesde >= LS_CALMO_MS && dona())
      solicitar(L.querModo >= 0 ? L.querModo : L.ultimoModo);
  }
  pthread_mutex_unlock(&M);
}

static LegSyncMotivo motivoRef(LegRefMotivo m) {
  switch (m) {
    case LEGREF_PLATAFORMA: return LEGSYNC_M_PLATAFORMA;
    case LEGREF_SEM_RANGE: return LEGSYNC_M_SEM_RANGE;
    case LEGREF_REDE: return LEGSYNC_M_REDE;
    case LEGREF_ORCAMENTO: return LEGSYNC_M_ORCAMENTO;
    default: return LEGSYNC_M_SEM_REFERENCIA;
  }
}

LegSyncVisao legsync_visao(int slot) {
  LegSyncVisao v; AutoSyncResultado e;
  memset(&v, 0, sizeof v);
  if (slot != 0) { v.fase = LEGSYNC_DEPOIS; return v; }
  pthread_mutex_lock(&M);
  if (!L.criado) { v.motivo = LEGSYNC_M_SEM_EXTERNA; goto fim; }
  snprintf(v.idiomaRef, sizeof v.idiomaRef, "%s", L.idiomaRef);
  if (L.primTipo == 2) { v.motivo = LEGSYNC_M_EMBUTIDA; goto fim; }
  if (L.primTipo != 1) { v.motivo = LEGSYNC_M_SEM_EXTERNA; goto fim; }
  if (!L.ref) { v.motivo = LEGSYNC_M_PLATAFORMA; goto fim; }
  if (L.primFase == 0) { v.fase = LEGSYNC_AGUARDANDO; goto fim; }
  if (L.primFase == 2) { v.motivo = LEGSYNC_M_EXTERNA_INCOMPLETA; goto fim; }
  if (!dona()) { v.motivo = LEGSYNC_M_SEM_EXTERNA; goto fim; }
  e = autosync_estado(L.sync, 0);
  v.offsetTotalMs = autosync_offset_ms(L.sync, 0);
  if (L.reenviar) { v.fase = LEGSYNC_PAUSADA; v.acoes = LEGSYNC_ACAO_PARAR; goto fim; }
  if (L.refPedido) {
    LegRefStatus st = legref_status(L.ref);
    v.fase = LEGSYNC_LENDO; v.acoes = LEGSYNC_ACAO_PARAR;
    v.progresso = st.total > 0 ? st.feitos * 100 / st.total : 0;
    goto fim;
  }
  if (e.estado == AUTOSYNC_ANALYSING || (L.querModo >= 0 && L.refDoc)) {
    v.fase = LEGSYNC_ANALISANDO; v.acoes = LEGSYNC_ACAO_PARAR; goto fim;
  }
  if (e.estado == AUTOSYNC_ACCEPTED && !L.desfeita) {
    v.fase = LEGSYNC_ACEITA; v.offsetAutoMs = e.offsetMs;
    v.acoes = LEGSYNC_ACAO_DESFAZER | (L.semOutra ? 0 : LEGSYNC_ACAO_OUTRA);
    goto fim;
  }
  v.acoes = LEGSYNC_ACAO_RAPIDA | LEGSYNC_ACAO_COMPLETA;
  if (L.desfeita) { v.fase = LEGSYNC_DESFEITA; v.acoes |= L.semOutra ? 0 : LEGSYNC_ACAO_OUTRA; goto fim; }
  if (L.semOutra) { v.fase = LEGSYNC_INDISPONIVEL; v.motivo = LEGSYNC_M_SEM_OUTRA; v.acoes = 0; goto fim; }
  if (L.solicitou && e.estado == AUTOSYNC_REJECTED) {
    v.fase = LEGSYNC_RECUSADA; v.motivo = LEGSYNC_M_CONFIANCA; v.acoes |= LEGSYNC_ACAO_OUTRA; goto fim;
  }
  if (!L.refDoc && L.refMotivo != LEGREF_OK && L.refMotivo != LEGREF_PARADO) {
    v.fase = LEGSYNC_INDISPONIVEL; v.motivo = motivoRef(L.refMotivo);
    // Falha da FAIXA (sem duracao, incompleta): outra faixa pode servir. Falha
    // do ARQUIVO/servidor: so tentar de novo (rede) ou nada.
    v.acoes = (L.refMotivo == LEGREF_SEM_DURACAO || L.refMotivo == LEGREF_INCOMPLETO) ? LEGSYNC_ACAO_OUTRA
            : L.refMotivo == LEGREF_REDE ? LEGSYNC_ACAO_RAPIDA : 0;
    goto fim;
  }
  v.fase = LEGSYNC_PRONTA;
fim:
  pthread_mutex_unlock(&M);
  return v;
}
