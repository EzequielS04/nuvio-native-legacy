// SUBTITLE SELECTOR MODEL (src/legendasui.c): languages + origins filtered to
// the configured main/second languages, slot markers, None per slot, More
// options with real details only, focus by opaque identity across late
// arrivals (embedded discovery shifts indices, addon list replaced), second
// slot refusing embedded/ASS without touching the primary, independent
// offsets, and the AutoSync provider row (hidden when NULL).
#include "../src/legendasui.c"
#include "linguas.h"
#include <assert.h>

// --- the world around the selector ------------------------------------------------
static VideoFaixa emb[8];
static int nEmb;
static Legenda add[LEG_MAX];
static int nAdd;
static char extId[24];
static int ativaEmb = -1;
static int escolhidaEmb = -99, escolhidasExt;
static char escolhidaExtUrl[600];
static char sec[128];
static int secEscolhas, secDesligar, secOffset;
static VideoLegendaEstilo estilo = { 120, 0, 0, 3, 1, 0, 0, 0, 0 };
static int caps = REDE_CAP_CORPO;

int video_n_legenda(void) { return nEmb; }
const VideoFaixa *video_legenda(int i) { return i >= 0 && i < nEmb ? &emb[i] : NULL; }
const char *player_id_canal(void) { return ""; }
int addons_legendas_copiar(Legenda *d, int max, unsigned *g, int *p) {
  int i, n = nAdd < max ? nAdd : max;
  for (i = 0; i < n; i++) d[i] = add[i];
  if (g) *g = 1;
  if (p) *p = 1;
  return n;
}
const char *faixas_legenda_externa_id(void) { return extId; }
int faixas_legenda_ativa(void) { return ativaEmb; }
void faixas_escolher_embutida(int i) {
  escolhidaEmb = i; extId[0] = 0; ativaEmb = i;
}
void faixas_escolher_externa(const Legenda *l) {
  escolhidasExt++; snprintf(escolhidaExtUrl, sizeof escolhidaExtUrl, "%s", l->url);
  legendasui_id_addon(l, extId); ativaEmb = -1;
}
const char *faixas_legenda_marca(int i) { (void)i; return NULL; }
const char *i18n(const char *s) { return s; }
unsigned rede_pedido_capacidades(void) { return (unsigned)caps; }
VideoLegendaEstilo *player_leg_estilo(void) { return &estilo; }
void player_leg_estilo_mudou(void) {}
Uint32 SDL_GetTicks(void) { return 1000; }
void plrui_decimal(char *s) { (void)s; }
// legenda2: record what the selector asks for (the loader has its own test).
void legenda2_reiniciar(void) { sec[0] = 0; secOffset = 0; }
unsigned legenda2_escolher(const char *id, const char *url, const char *i, const char *o) {
  (void)url; (void)i; (void)o; secEscolhas++; snprintf(sec, sizeof sec, "%s", id); return 1;
}
void legenda2_desligar(void) { secDesligar++; sec[0] = 0; }
Leg2Estado legenda2_estado(void) { return sec[0] ? LEG2_ATIVA : LEG2_NENHUMA; }
const char *legenda2_identidade(void) { return sec; }
int legenda2_offset_manual(void) { return secOffset; }
void legenda2_definir_offset_manual(int ms) { secOffset = ms; }

static void faixa(int i, int numero, const char *idioma, const char *rot, const char *codec, int letreiro) {
  memset(&emb[i], 0, sizeof emb[i]);
  emb[i].numero = numero; emb[i].letreiro = letreiro; emb[i].ordinalMkv = -1;
  snprintf(emb[i].idioma, sizeof emb[i].idioma, "%s", idioma);
  snprintf(emb[i].rotulo, sizeof emb[i].rotulo, "%s", rot);
  snprintf(emb[i].codec, sizeof emb[i].codec, "%s", codec);
}
static void addon(int i, const char *idioma, const char *prov, const char *arquivo, const char *url) {
  memset(&add[i], 0, sizeof add[i]);
  snprintf(add[i].idioma, sizeof add[i].idioma, "%s", idioma);
  snprintf(add[i].provedor, sizeof add[i].provedor, "%s", prov);
  snprintf(add[i].arquivo, sizeof add[i].arquivo, "%s", arquivo);
  snprintf(add[i].url, sizeof add[i].url, "%s", url);
  snprintf(add[i].rotulo, sizeof add[i].rotulo, "%s", idioma);
}

static int linhaDe(const char *chave) {
  int i;
  for (i = 0; i < nLinhasV; i++) if (!strcmp(linhas[i].chave, chave)) return i;
  return -1;
}
static int tecla(SDL_Keycode k) {
  SDL_Event e; memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN; e.key.keysym.sym = k;
  return legendasui_evento(&e);
}
static void focarChave(const char *k) {
  montarLinhas();
  assert(linhaDe(k) >= 0);
  focar(linhaDe(k));
}
static void subDe(const char *chave, char *sub, size_t n) {
  char nome[96]; const char *id, *ic; int at, ap;
  int i = linhaDe(chave);
  assert(i >= 0);
  textoLinha(&linhas[i], nome, sizeof nome, sub, n, &id, &ic, &at, &ap);
}

// AutoSync stand-in.
static int syncExec[2] = { -1, -1 };
static const char *syncEstado(int slot, void *u) { (void)u; return slot == 1 ? "Sincronizada (+1,2 s)" : "Pronta"; }
static int syncAcoes(int slot, const char **r, int max, void *u) {
  (void)slot; (void)u; (void)max;
  r[0] = "Sincronizar"; r[1] = "Desfazer"; r[2] = "Tentar outra referência";
  return 3;
}
static void syncRodar(int slot, int acao, void *u) { (void)u; syncExec[slot] = acao; }

int main(void) {
  char sub[256];
  int i, n0;

  // --- identities and format detection ---
  { char a[24], b[24];
    faixa(0, 3, "pt", "Português", "S_TEXT/UTF8", 0);
    legendasui_id_embutida(&emb[0], a);
    snprintf(emb[0].idioma, sizeof emb[0].idioma, "por");     // the probe fills the language later
    snprintf(emb[0].codec, sizeof emb[0].codec, "S_TEXT/ASS");
    legendasui_id_embutida(&emb[0], b);
    assert(!strcmp(a, b) && a[0] == 'e' && strlen(a) == 17);  // same track, same identity
    addon(0, "pt", "OpenSubtitles", "", "https://secret.invalid/file?token=PRIVATE");
    legendasui_id_addon(&add[0], a);
    assert(a[0] == 'a' && !strstr(a, "secret") && !strstr(a, "PRIVATE"));
    addon(1, "pt", "OpenSubtitles", "", "https://secret.invalid/other");
    legendasui_id_addon(&add[1], b);
    assert(strcmp(a, b)); }
  { char f[8];
    formatoDaExtensao("Movie.2024.WEB.srt", f); assert(!strcmp(f, "SRT"));
    formatoDaExtensao("https://h.invalid/x/file.VTT?sig=1", f); assert(!strcmp(f, "VTT"));
    formatoDaExtensao("Fansub.ssa", f); assert(!strcmp(f, "ASS"));
    formatoDaExtensao("https://h.invalid/download/123", f); assert(!f[0]);   // unknown stays unknown
    formatoDaExtensao("Movie.2024.WEB", f); assert(!f[0]); }

  // --- languages / origins / slots ---
  nEmb = 3;
  faixa(0, 3, "pt", "Português", "S_TEXT/UTF8", 0);
  faixa(1, 4, "pt", "Português — Letreiros", "S_TEXT/ASS", 1);
  faixa(2, 5, "en", "English", "S_TEXT/UTF8", 0);
  nAdd = 6;
  addon(0, "pt", "OpenSubtitles", "Show.S01E01.WEB.srt", "https://h.invalid/1");
  addon(1, "pt", "OpenSubtitles", "", "https://h.invalid/2");
  addon(2, "pt", "SubDL", "Show.S01E01.ass", "https://h.invalid/3");
  addon(3, "en", "OpenSubtitles", "Show.S01E01.srt", "https://h.invalid/4");
  addon(4, "es", "OpenSubtitles", "", "https://h.invalid/5");
  addon(5, "en", "Subs.ro", "", "https://h.invalid/6");
  ling_local_legenda("pt");
  ling_local_legenda2("en");
  ativaEmb = 0;
  legendasui_abrir();
  assert(aberto && !mais);
  assert(linhaDe("p|Português|E") >= 0 && linhaDe("p|Português|E*") >= 0);
  assert(linhaDe("p|Português|A:OpenSubtitles") >= 0 && linhaDe("p|Português|A:SubDL") >= 0);
  assert(linhas[linhaDe("p|Português|A:OpenSubtitles")].nVar == 2);   // variants folded
  assert(linhaDe("p|Espanhol|A:OpenSubtitles") < 0);                     // filtered out
  assert(linhaDe("p|Inglês|E") < 0);
  // The second slot never offers what would not draw: no embedded row.
  assert(linhaDe("s|Inglês|E") < 0 && linhaDe("s|Inglês|A:OpenSubtitles") >= 0 && linhaDe("s|Inglês|A:Subs.ro") >= 0);
  assert(linhaDe("s|Português|E") < 0);
  assert(linhaDe("p|-") >= 0 && linhaDe("s|-") >= 0 && linhaDe("mais") == nLinhasV - 1);
  assert(linhaDe("sync|p") < 0 && linhaDe("sync|s") < 0);              // NULL provider: no sync row
  assert(!strcmp(linhas[foco].chave, "p|Português|E"));                // opens on the active primary
  subDe("p|Português|A:OpenSubtitles", sub, sizeof sub);
  assert(strstr(sub, "OpenSubtitles") && strstr(sub, "2 versões"));
  subDe("s|-", sub, sizeof sub);                                          // ... and says why
  assert(strstr(sub, "Embutida como secundária: indisponível"));

  // --- no configured language: unfiltered primary, second slot only None ---
  ling_local_legenda("*"); ling_local_legenda2("");
  montarLinhas();
  assert(linhaDe("p|Espanhol|A:OpenSubtitles") >= 0 && linhaDe("p|Inglês|E") >= 0);
  for (i = 0; i < nLinhasV; i++) assert(linhas[i].slot == 0 || linhas[i].tipo != LR_CAND);
  ling_local_legenda("pt"); ling_local_legenda2("en");

  // --- focus by identity while data arrives late ---
  focarChave("s|Inglês|A:OpenSubtitles");
  n0 = foco;
  // Embedded tracks discovered late: two NEW tracks listed before the old
  // ones, so every old index shifts by two. The addon list is replaced too.
  memmove(&emb[2], &emb[0], 3 * sizeof emb[0]);
  faixa(0, 1, "pt", "Português (Forçada)", "S_TEXT/UTF8", 0);
  faixa(1, 2, "en", "English SDH", "S_TEXT/UTF8", 0);
  nEmb = 5;
  ativaEmb = 2;                                   // same track (numero 3), new index
  memmove(&add[1], &add[0], 6 * sizeof add[0]);
  addon(0, "en", "Podnapisi", "", "https://h.invalid/7");
  nAdd = 7;
  montarLinhas();
  assert(aberto);                                 // never closed by late data
  assert(!strcmp(linhas[foco].chave, "s|Inglês|A:OpenSubtitles") && foco != n0);
  assert(linhaDe("p|Português|E") >= 0 && linhas[linhaDe("p|Português|E")].nVar == 2);
  // The check of the primary follows the identity, not the index.
  { char nome[96]; const char *id, *ic; int at, ap, k = linhaDe("p|Português|E");
    textoLinha(&linhas[k], nome, sizeof nome, sub, sizeof sub, &id, &ic, &at, &ap);
    assert(at && cand[linhas[k].cand].indice == 2); }
  // A row that disappears: focus stays at the same position, panel open.
  focarChave("s|Inglês|A:Podnapisi");
  n0 = foco;
  nAdd = 0;
  montarLinhas();
  assert(aberto && foco == (n0 < nLinhasV ? n0 : nLinhasV - 1) && !strcmp(linhas[foco].chave, focoChave));

  // --- choosing the primary by identity after the shift ---
  nAdd = 7;
  focarChave("p|Português|E*");
  assert(tecla(SDLK_RETURN) == LEGUI_TRATADO);
  assert(escolhidaEmb == 3);                      // letreiro track: index 1 -> 3
  assert(aberto);                                 // stays open: both slots can be set

  // --- second slot: embedded and ASS refused, primary untouched ---
  escolhidaEmb = -99; escolhidasExt = 0; secEscolhas = 0;
  // Simple view: not offered at all (see above). More options with the
  // second slot aimed: listed dimmed with the reason, OK refused.
  focarChave("mais");
  tecla(SDLK_RETURN);
  focarChave("m|alvo");
  tecla(SDLK_RIGHT);
  assert(mais && alvo == 1);
  { char k[96]; int j, e = -1, a = -1;
    for (j = 0; j < nCand; j++) {
      if (cand[j].embutida && !strcmp(cand[j].idioma, "en") && e < 0) e = j;
      if (!cand[j].embutida && !strcmp(cand[j].formato, "ASS")) a = j;
    }
    assert(e >= 0 && a >= 0);
    snprintf(k, sizeof k, "m|%s", cand[e].id);
    focarChave(k);
    tecla(SDLK_RETURN);
    assert(secEscolhas == 0 && escolhidaEmb == -99 && escolhidasExt == 0);
    assert(strstr(aviso, "Embutida como secundária"));
    snprintf(k, sizeof k, "m|%s", cand[a].id);   // .ass file
    focarChave(k);
    tecla(SDLK_RETURN);
    assert(secEscolhas == 0 && strstr(aviso, "ASS como secundária"));
    { const char *id, *ic; char nome[96]; int at, ap, i2 = linhaDe(k);
      textoLinha(&linhas[i2], nome, sizeof nome, sub, sizeof sub, &id, &ic, &at, &ap);
      assert(ap && !at); } }                      // dimmed, never checked
  tecla(SDLK_ESCAPE);
  assert(!mais);
  alvo = 0;
  // Portuguese as the second language: its embedded tracks and the SubDL .ass
  // file are hidden from the simple view; the OpenSubtitles SRTs stay.
  ling_local_legenda2("pt");
  montarLinhas();
  assert(linhaDe("s|Português|A:SubDL") < 0 && linhaDe("s|Português|E") < 0);
  assert(linhaDe("s|Português|A:OpenSubtitles") >= 0);
  // Unsupported platform: honest refusal, nothing changes.
  caps = 0;
  focarChave("s|Português|A:OpenSubtitles");
  tecla(SDLK_RETURN);
  assert(secEscolhas == 0 && strstr(aviso, "nesta plataforma"));
  caps = REDE_CAP_CORPO;
  // An external SRT as the second subtitle.
  ling_local_legenda2("en");
  montarLinhas();
  focarChave("s|Inglês|A:OpenSubtitles");
  tecla(SDLK_RETURN);
  assert(secEscolhas == 1 && sec[0] == 'a' && escolhidaEmb == -99 && escolhidasExt == 0);
  { char nome[96]; const char *id, *ic; int at, ap, k = linhaDe("s|Inglês|A:OpenSubtitles");
    textoLinha(&linhas[k], nome, sizeof nome, sub, sizeof sub, &id, &ic, &at, &ap);
    assert(at); }
  // The same file cannot be both.
  focarChave("p|Português|A:OpenSubtitles");
  tecla(SDLK_RETURN);
  assert(escolhidasExt == 1 && strstr(escolhidaExtUrl, "h.invalid/1"));
  ling_local_legenda2("pt");
  montarLinhas();
  secEscolhas = 0;
  focarChave("s|Português|A:OpenSubtitles");
  tecla(SDLK_RETURN);
  assert(secEscolhas == 0 && strstr(aviso, "Já é a principal"));
  ling_local_legenda2("en");
  // None per slot.
  focarChave("s|-");
  tecla(SDLK_RETURN);
  assert(secDesligar == 1 && !sec[0] && escolhidasExt == 1);
  focarChave("p|-");
  tecla(SDLK_RETURN);
  assert(escolhidaEmb == -1);

  // --- More options: every candidate, real details, aim at either slot ---
  focarChave("mais");
  tecla(SDLK_RETURN);
  assert(mais && linhaDe("m|alvo") == 0 && linhaDe("m|atraso") == 1 && linhaDe("m|-") == 2);
  assert(nLinhasV == 3 + nEmb + nAdd);            // nothing filtered here
  assert(linhas[foco].tipo == LR_CAND);
  { char k[96];
    snprintf(k, sizeof k, "m|%s", cand[nEmb + 1].id);           // pt OpenSubtitles with release name
    subDe(k, sub, sizeof sub);
    assert(strstr(sub, "OpenSubtitles") && strstr(sub, "SRT") && strstr(sub, "Show.S01E01.WEB.srt"));
    snprintf(k, sizeof k, "m|%s", cand[nEmb + 2].id);           // no name, no extension: provider only
    subDe(k, sub, sizeof sub);
    assert(!strcmp(sub, "OpenSubtitles"));
    assert(!strstr(sub, "h.invalid")); }
  // Offsets are per slot: the second slot's offset never moves the primary's.
  focarChave("m|atraso");
  tecla(SDLK_RIGHT); tecla(SDLK_RIGHT);
  assert(estilo.atrasoMs == 500 && secOffset == 0);
  focarChave("m|alvo");
  tecla(SDLK_RIGHT);
  assert(alvo == 1);
  focarChave("m|atraso");
  tecla(SDLK_LEFT);
  assert(secOffset == -250 && estilo.atrasoMs == 500);
  // Estilo is still one RIGHT away from a candidate row; Back returns to the
  // simple view on the More options row, a second Back closes.
  focarChave("m|-");
  assert(tecla(SDLK_RIGHT) == LEGUI_ESTILO);
  assert(tecla(SDLK_ESCAPE) == LEGUI_TRATADO && !mais && !strcmp(linhas[foco].chave, "mais"));

  // --- AutoSync provider plugs in per slot ---
  { LegendasSyncProvider p = { syncEstado, syncAcoes, syncRodar, NULL };
    legendasui_definir_sync(&p);
    montarLinhas();
    assert(linhaDe("sync|p") >= 0 && linhaDe("sync|s") >= 0);
    focarChave("sync|s");
    tecla(SDLK_RIGHT); tecla(SDLK_RIGHT);             // to "Tentar outra referência"
    tecla(SDLK_RETURN);
    assert(syncExec[1] == 2 && syncExec[0] == -1);
    subDe("sync|s", sub, sizeof sub);
    // F05 row: title is fixed, the provider's state is the line below; the
    // chosen action is drawn between < > (not in the text).
    assert(strstr(sub, "Sincronizada (+1,2 s)") && !strstr(sub, "["));
    legendasui_definir_sync(NULL);
    montarLinhas();
    assert(linhaDe("sync|p") < 0 && linhaDe("sync|s") < 0); }

  // --- foto da TV (04/10): a lingua preferida vem primeiro, "PORTUGUESE" tem nome
  //     e selo inteiros, e as duas "Nenhuma" nao se parecem ---
  { char nm[96], sb[160]; const char *id, *ic; int at, ap, a, b;
    nAdd = 3; ativaEmb = -1; nEmb = 0;
    addon(0, "eng", "AIOStreams", "", "https://h.invalid/a");
    ling_normalizar("PORTUGUESE", add[1].idioma, sizeof add[1].idioma);
    snprintf(add[1].provedor, sizeof add[1].provedor, "AIOStreams");
    snprintf(add[1].url, sizeof add[1].url, "https://h.invalid/b");
    addon(2, "pob", "OpenSubtitles", "", "https://h.invalid/c");
    ling_local_legenda("pt"); ling_local_legenda2("");
    legendasui_abrir();
    assert(linhas[0].tipo == LR_CAND && ling_casa(cand[linhas[0].cand].idioma, "pt"));
    a = linhaDe("p|-"); assert(a >= 0);
    textoLinha(&linhas[a], nm, sizeof nm, sb, sizeof sb, &id, &ic, &at, &ap);
    assert(!strcmp(nm, "Nenhuma"));
    b = linhaDe("s|-");
    if (b >= 0) { textoLinha(&linhas[b], nm, sizeof nm, sb, sizeof sb, &id, &ic, &at, &ap); assert(!strcmp(nm, "Sem segunda legenda")); }
    assert(!strcmp(ling_selo(add[1].idioma), "PT")); }

  assert(tecla(SDLK_ESCAPE) == LEGUI_FECHAR && !aberto);
  // Media change drops the second slot.
  snprintf(sec, sizeof sec, "a1"); secOffset = 900;
  legendasui_reiniciar();
  assert(!sec[0] && secOffset == 0);
  puts("legendasui: languages/origins, slots, None, More options details, focus identity, index shift, second-slot refusals, offsets, sync row ok");
  return 0;
}
