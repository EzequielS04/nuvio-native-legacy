// Spotlight: a caixa de busca por cima da tela. O porque e quem abre estao em
// spotlight.h; aqui so as decisoes de forma e de custo.
//
// FORMA (pedido do dono, 01/10/2026: "deixar a barra do spotlight so a barra e
// so crescer com os resultados aparecendo"). Como o Spotlight do macOS: abre SO
// a barra — vidro, centrada no alto, lupa, campo e microfone a direita. Quando
// o texto chega, a mesma superficie CRESCE para baixo com a mola lenta da ilha
// (ilha.c, ILHA_MOLA_*) e mostra a lista; sem resultado, encolhe de volta. A
// lista e VERTICAL e agrupada (o Spotlight do Mac, o Google TV): o melhor
// resultado no alto fica a UM toque (baixo + OK).
//
// QUEM DIGITA. No Android, o teclado do SISTEMA (sistexto.h): abrir ja foca o
// campo e o chama; a tecla de voz ja comeca o ditado. Onde nao ha IME
// confiavel (LG, Samsung) o teclado do app — o mesmo alfabeto de
// teclado_alfabeto() — so aparece com OK no campo, a esquerda dentro do corpo,
// e some quando o foco desce para os resultados. Voltar desfaz na ordem
// inversa: teclado/lista -> campo; campo com texto -> limpa (a barra encolhe);
// campo vazio -> fecha.
//
// AS FONTES SAO AS QUE O APP JA TEM, nenhuma rede nova:
//   - titulos: o catalogo em memoria (as fileiras da home) + desc_buscar, a
//     mesma busca nos addons da tela de Busca;
//   - pessoas: o ELENCO dos titulos ja carregados (CatItem.elenco), com o id do
//     TMDB que abre a filmografia, e depois o /search/person do TMDB
//     (spotpessoa.h) — com debounce, um pedido quando o texto para, nunca um
//     por letra. A do elenco vem antes: e da biblioteca do dono;
//   - colecoes (pastas), canais da Live TV (lista publicada do guia),
//     catalogos (titulos das fileiras) e addons instalados.
//   Fora isso so a busca nos addons (desc_buscar) e a de pessoas vao a rede.
//
// CUSTO (60 fps na C9): remontar so roda quando o texto muda ou a resposta da
// rede chega, nunca por quadro. O desenho e um veu, um painel e no maximo
// ~12 linhas visiveis; com a entrada assentada app.c congela o fundo numa
// copia (como o painel de Salvos), e o quadro passa a ser copia + painel.
#include "spotlight.h"
#include "buscasrec.h"
#include "buscanorm.h"
#include "catalogo.h"
#include "descoberta.h"
#include "colecoes.h"
#include "guia.h"
#include "spotpessoa.h"
#include "addons.h"
#include "teclado.h"
#include "gfx.h"
#include "text.h"
#include "tex_cache.h"
#include "anim.h"
#include "layout.h"
#include "ajustes.h"
#include "idioma.h"
#include "posterprov.h"
#include "ponteiro.h"
#include "sistexto.h"
#include "celbotao.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Geometria (1920x1080) ----------------------------------------------------
#define SP_BW        1240.0f
#define SP_BX        ((NV_TELA_W - SP_BW) * 0.5f)
#define SP_BY        116.0f
#define SP_BH        104.0f
#define SP_RAIO      34.0f          // px do corpo aberto; a barra sozinha e pilula
#define SP_CORPO_Y   (SP_BY + SP_BH)
#define SP_CORPO_MAX (1024.0f - SP_CORPO_Y)
#define SP_CPAD_T    16.0f
#define SP_CPAD_B    10.0f
#define SP_RODAPE_H  56.0f
#define SP_MIC_D     64.0f
#define SP_TECLA     62.0f
#define SP_TECLA_GAP 10.0f
#define SP_KB_COLS   6
#define SP_KB_PASSO  (SP_TECLA + SP_TECLA_GAP)
#define SP_KB_X      (SP_BX + 36.0f)
#define SP_KB_Y      (SP_CORPO_Y + SP_CPAD_T + 8.0f)
#define SP_KB_W      (SP_KB_COLS * SP_TECLA + (SP_KB_COLS - 1) * SP_TECLA_GAP)
#define SP_LISTA_X0  (SP_BX + 26.0f)                 // sem o teclado do app
#define SP_LISTA_X1  (SP_KB_X + SP_KB_W + 44.0f)     // com ele
#define SP_LISTA_XF  (SP_BX + SP_BW - 26.0f)
#define SP_MAX_TXT   48             // = BUSCASREC_TERMO
#define SP_MAX_LIN   48
#define SP_KB_MAX_FIL 8
#define SP_MAX_RECENTES 4
// A mola da ilha (ilha.c): subamortecida, o corpo passa um pouco e volta.
#define SP_MOLA_W    10.0f
#define SP_MOLA_Z    0.72f

// --- Linhas da lista -----------------------------------------------------------
enum {
  L_CAB = 0,      // cabecalho de grupo (nao focavel)
  L_AVISO,        // "Buscando...", "Nada encontrado" (nao focavel)
  L_TOPO,         // melhor resultado
  L_TITULO,
  L_PESSOA,
  L_COLECAO,
  L_CANAL,
  L_CATALOGO,
  L_ADDON,
  L_RECENTE,
  L_LIMPAR,
};
static const float ALTURA[] = { 50, 64, 210, 92, 92, 92, 92, 80, 80, 72, 72 };

typedef struct {
  int  tipo;
  int  ref;          // indice de catalogo / pasta / canal / fileira / addon / termo
  int  ref2;         // pessoa: indice do titulo de onde ela veio
  long tmdb;
  long tituloTmdb;   // pessoa do TMDB: o titulo pelo qual a filmografia abre
  char tituloTipo[8];
  char t1[160];
  char t2[200];
  char arte[1024];
  char icone[32];
  char id[80];
  char base[600];
  char chave[96];    // identidade, para foco e entrada sobreviverem a remontagem
  float y, h;
} Linha;

static Linha lin[SP_MAX_LIN];
static int   nLin;
static float animLin[SP_MAX_LIN], entraLin[SP_MAX_LIN];

// --- Estado ----------------------------------------------------------------------
static int   aberto;
static float entrada;              // 0..1 (mola)
// Onde esta o foco. O teclado do app so existe com kbAberto.
enum { P_CAMPO = 0, P_MIC, P_TECLADO, P_LISTA, P_CEL };
static int   painel;
static int   kbAberto;
static float kbAnim;               // 0..1, o teclado do app entrando
static float corpoH, corpoV;       // altura do corpo (mola da ilha) e velocidade
static float animMic;
static int   focoL = -1;           // linha focada (indice em lin)
static int   kbF, kbC;             // tecla focada
static char  consulta[SP_MAX_TXT];
static int   nConsulta;
static char  montada[SP_MAX_TXT];  // consulta da ultima remontagem
static int   ultimoRemoto = -1, ultimoBuscando = -1, ultimaGeracao = -1;
static unsigned ultimaGerPessoa;
static float scrollY, scrollAlvo, velY;
static float animTecla[SP_KB_MAX_FIL + 1][SP_KB_COLS];
static float animCampo;
static SpotPedido pedido;
static int   temPedido;
static int   okPress, okLongo;
static Uint32 okDesde;
static float nivelVoz;             // nivel do som suavizado (st_nivel)

// Soma do que cada alvo de busca ja devolveu para o termo corrente: a resposta
// de um addon lento chega depois da tecla e tem de aparecer sozinha.
static int remotoTotal(void) {
  int a, n = 0, nA = desc_busca_n_alvos();
  for (a = 0; a < nA; a++) n += desc_busca_alvo_n(a, consulta);
  return n + desc_busca_n(consulta) * 1000;
}

// Teclado: 36 caracteres em 6 fileiras e a fileira de baixo de comandos.
static char  kbTeclas[64][5];
static int   kbN, kbFil;
enum { K_ESPACO, K_APAGAR, K_LIMPAR, K_FALAR, K_TECLADO };
static int   kbCmd[6], kbNCmd;

static int ditadoDisponivel(void) { return st_voz_disponivel(); }
// DIGITAR PELO CELULAR (celbotao.h): o disco no fim da barra, depois do Falar.
static int celDisponivel(void) { return celb_disponivel(); }
// Largura que os botoes da direita (Falar, Celular) tiram da barra.
static float botoesW(void) {
  return (ditadoDisponivel() ? SP_MIC_D + 14.0f : 0.0f) + (celDisponivel() ? SP_MIC_D + 14.0f : 0.0f) +
         (ditadoDisponivel() || celDisponivel() ? 12.0f : 0.0f);
}
static int imeDisponivel(void) { return st_ime_disponivel(); }
static int ouvindo(void) {
  return st_dono() == ST_SPOT && (st_estado() == ST_OUVINDO || st_estado() == ST_PERMISSAO ||
                                  st_estado() == ST_VOZ_SISTEMA);
}
static int digitandoSis(void) { return st_dono() == ST_SPOT && st_estado() == ST_DIGITANDO; }

static void kbMontar(void) {
  const unsigned char *p = (const unsigned char *)teclado_alfabeto();
  kbN = 0;
  while (*p && kbN < 48) {
    int len = *p < 0x80 ? 1 : (*p >= 0xF0 ? 4 : (*p >= 0xE0 ? 3 : 2)), i;
    for (i = 0; i < len; i++) kbTeclas[kbN][i] = (char)p[i];
    kbTeclas[kbN][len] = 0;
    kbN++; p += len;
  }
  kbFil = (kbN + SP_KB_COLS - 1) / SP_KB_COLS;
  kbNCmd = 0;
  kbCmd[kbNCmd++] = K_ESPACO;
  kbCmd[kbNCmd++] = K_APAGAR;
  kbCmd[kbNCmd++] = K_LIMPAR;
  if (ditadoDisponivel()) kbCmd[kbNCmd++] = K_FALAR;
  if (imeDisponivel()) kbCmd[kbNCmd++] = K_TECLADO;
}

static int kbColunas(int f) {
  if (f < kbFil) {
    int resto = kbN - f * SP_KB_COLS;
    return resto > SP_KB_COLS ? SP_KB_COLS : resto;
  }
  return kbNCmd;
}

static GfxRect teclaRect(int f, int c) {
  GfxRect r;
  r.y = SP_KB_Y + f * SP_KB_PASSO;
  r.h = SP_TECLA;
  if (f < kbFil) { r.x = SP_KB_X + c * SP_KB_PASSO; r.w = SP_TECLA; }
  else {
    r.w = (SP_KB_W - (kbNCmd - 1) * SP_TECLA_GAP) / (float)kbNCmd;
    r.x = SP_KB_X + c * (r.w + SP_TECLA_GAP);
  }
  return r;
}

// --- Montagem da lista --------------------------------------------------------------
static int focavel(int tipo) { return tipo != L_CAB && tipo != L_AVISO; }

static Linha *nova(int tipo) {
  Linha *l;
  if (nLin >= SP_MAX_LIN) return NULL;
  l = &lin[nLin++];
  memset(l, 0, sizeof *l);
  l->tipo = tipo;
  l->ref = l->ref2 = -1;
  return l;
}

static void cabecalho(const char *t) {
  Linha *l = nova(L_CAB);
  if (!l) return;
  snprintf(l->t1, sizeof l->t1, "%s", t);
  snprintf(l->chave, sizeof l->chave, "cab|%s", t);
}

// Quanto o titulo `t` (normalizado) casa com o alvo: igual > comeca com >
// uma palavra comeca com > contem. 0 = nao casa.
static int pontuar(const char *t, const char *alvo) {
  size_t n = strlen(alvo);
  const char *p;
  if (!strcmp(t, alvo)) return 100;
  if (!strncmp(t, alvo, n)) return 80;
  for (p = t; (p = strstr(p, alvo)) != NULL; p++)
    if (p > t && (p[-1] == ' ' || p[-1] == ':' || p[-1] == '-')) return 60;
  return strstr(t, alvo) ? 40 : 0;
}

static const char *rotuloTipo(const char *tipo) {
  if (!strcmp(tipo, "movie")) return i18n("Filme");
  if (!strcmp(tipo, "series")) return i18n("Série");
  if (!strcmp(tipo, "channel") || !strcmp(tipo, "tv")) return i18n("Canal");
  return "";
}

// "2022 · 3 temporadas · Série · ★ 8.1": o que a lista mostra sob o nome.
static void metaTitulo(const CatItem *ci, char *dst, size_t n) {
  const char *tp = rotuloTipo(ci->tipo);
  char nota[24] = "";
  if (ci->nota > 0) snprintf(nota, sizeof nota, "\xe2\x98\x85 %d.%d", ci->nota / 10, ci->nota % 10);
  snprintf(dst, n, "%s%s%s%s%s", ci->meta,
           ci->meta[0] && tp[0] ? "  \xc2\xb7  " : "", tp,
           (ci->meta[0] || tp[0]) && nota[0] ? "  \xc2\xb7  " : "", nota);
}

static const char *arteDe(const CatItem *ci, int paisagem) {
  const char *a;
  if (paisagem && ci->backdrop[0]) return ci->backdrop;
  a = posterprov_card_addon(ci->origem, ci->imdb, ci->tmdb, ci->tipo, ci->poster);
  if (a && a[0]) return a;
  return ci->backdrop[0] ? ci->backdrop : "";
}

static void linhaTitulo(int tipo, int idx) {
  const CatItem *ci = cat_item(idx);
  Linha *l;
  if (!ci || !(l = nova(tipo))) return;
  l->ref = idx;
  snprintf(l->t1, sizeof l->t1, "%s", ci->titulo);
  metaTitulo(ci, l->t2, sizeof l->t2);
  snprintf(l->arte, sizeof l->arte, "%s", arteDe(ci, tipo == L_TOPO));
  snprintf(l->chave, sizeof l->chave, "t|%s|%s", ci->imdb, ci->titulo);
}

typedef struct { int idx, pont, ordem; } Cand;
static int candCmp(const void *a, const void *b) {
  const Cand *x = a, *y = b;
  if (x->pont != y->pont) return y->pont - x->pont;
  return x->ordem - y->ordem;
}

#define SP_MAX_TIT 9
static void montarTitulos(const char *alvo) {
  Cand c[64];
  int nc = 0, r, i, ordem = 0;
  char nome[320];
  // REDE primeiro na ORDEM (o addon ja ranqueou), local depois; o placar
  // decide o resto. Remoto que nao contem o texto (o addon casou por outro
  // campo) ainda entra, abaixo de qualquer casamento de nome.
  { int a, nAlvos = desc_busca_n_alvos();
    for (a = 0; a < nAlvos && nc < 40; a++) {
      int nRem = desc_busca_alvo_n(a, consulta);
      CatItem novos[8];
      int posNovo[8], idxNovos[8], nNovos = 0;
      for (i = 0; i < nRem && i < 8 && nc < 40; i++) {
        CatItem it;
        int idx, k, dup = 0;
        if (!desc_busca_alvo_item(a, i, &it)) continue;
        idx = it.imdb[0] ? cat_indice_por_imdb(it.imdb) : -1;
        if (idx >= 0) for (k = 0; k < nc; k++) if (c[k].idx == idx) { dup = 1; break; }
        if (dup) continue;
        busca_normalizar(it.titulo, nome, sizeof nome);
        c[nc].pont = pontuar(nome, alvo);
        if (c[nc].pont < 20) c[nc].pont = 20;
        c[nc].pont += 6 - (a < 6 ? a : 6);   // primeiro addon desempata
        c[nc].ordem = ordem++;
        if (idx >= 0) c[nc++].idx = idx;
        else if (nNovos < 8) { novos[nNovos] = it; posNovo[nNovos++] = nc; c[nc++].idx = -1; }
      }
      // Os que nao estao no catalogo entram numa troca de bloco so (o porque
      // esta em refiltrar, busca.c: cat_acrescentar por item copia tudo).
      if (nNovos > 0) {
        int entraram = cat_acrescentar_lote(novos, nNovos, idxNovos);
        for (i = 0; i < nNovos; i++) c[posNovo[i]].idx = i < entraram ? idxNovos[i] : -1;
      }
    } }
  for (r = 0; r < cat_n_fileiras() && nc < 64; r++) {
    const CatFileira *cf = cat_fileira(r);
    if (!cf) break;
    for (i = 0; i < cf->n && nc < 64; i++) {
      const CatItem *ci = cat_item(cf->ini + i);
      int p, k, dup = 0;
      if (!ci || !strcmp(ci->tipo, "channel")) continue;
      busca_normalizar(ci->titulo, nome, sizeof nome);
      if (!(p = pontuar(nome, alvo))) continue;
      for (k = 0; k < nc; k++) {
        const CatItem *o = c[k].idx >= 0 ? cat_item(c[k].idx) : NULL;
        if (c[k].idx == cf->ini + i || (o && ci->imdb[0] && !strcmp(o->imdb, ci->imdb))) {
          if (p + 5 > c[k].pont) c[k].pont = p + 5;   // esta nas fileiras do dono
          dup = 1; break;
        }
      }
      if (dup) continue;
      c[nc].idx = cf->ini + i; c[nc].pont = p + 5; c[nc].ordem = ordem++; nc++;
    }
  }
  // tira os -1 (catalogo no teto) antes de ordenar
  { int w = 0; for (i = 0; i < nc; i++) if (c[i].idx >= 0) c[w++] = c[i]; nc = w; }
  qsort(c, (size_t)nc, sizeof *c, candCmp);
  // O mesmo titulo vindo de dois addons sem imdb conhecido so ganha o indice
  // no lote: fica o de placar maior (o primeiro depois da ordenacao).
  { int w = 0, k;
    for (i = 0; i < nc; i++) {
      for (k = 0; k < w; k++) if (c[k].idx == c[i].idx) break;
      if (k == w) c[w++] = c[i];
    }
    nc = w; }
  if (nc > 0) {
    cabecalho(i18n("Melhor resultado"));
    linhaTitulo(L_TOPO, c[0].idx);
  }
  if (nc > 1) {
    cabecalho(i18n("Títulos"));
    for (i = 1; i < nc && i < SP_MAX_TIT; i++) linhaTitulo(L_TITULO, c[i].idx);
  }
}

static void montarPessoas(const char *alvo) {
  long vistos[SPP_MAX + 4];
  int nv = 0, i, j, n = cat_n(), cab = 0;
  char nome[160];
  for (i = 0; i < n && nv < 4; i++) {
    const CatItem *ci = cat_item(i);
    if (!ci || ci->nElenco <= 0) continue;
    for (j = 0; j < ci->nElenco && nv < 4; j++) {
      int k, dup = 0;
      Linha *l;
      if (ci->elenco[j].tmdb <= 0 || !ci->elenco[j].nome[0]) continue;
      for (k = 0; k < nv; k++) if (vistos[k] == ci->elenco[j].tmdb) { dup = 1; break; }
      if (dup) continue;
      busca_normalizar(ci->elenco[j].nome, nome, sizeof nome);
      if (pontuar(nome, alvo) < 60) continue;   // pessoa: so inicio de nome/sobrenome
      if (!cab) { cabecalho(i18n("Pessoas")); cab = 1; }
      if (!(l = nova(L_PESSOA))) return;
      vistos[nv++] = ci->elenco[j].tmdb;
      l->ref2 = i;
      l->tmdb = ci->elenco[j].tmdb;
      snprintf(l->t1, sizeof l->t1, "%s", ci->elenco[j].nome);
      snprintf(l->t2, sizeof l->t2, i18n("Em %s"), ci->titulo);
      snprintf(l->arte, sizeof l->arte, "%s", ci->elenco[j].foto);
      snprintf(l->chave, sizeof l->chave, "p|%ld", l->tmdb);
    }
  }
  // O TMDB completa ate SPP_MAX, na ordem de popularidade dele. Sem resposta
  // ainda (debounce, rede) a lista fica com as do elenco e remonta quando a
  // resposta chega (spot_atualizar).
  { int nt = spotpessoa_n(consulta);
    for (i = 0; i < nt && nv < SPP_MAX; i++) {
      SpotPessoa sp;
      int k, dup = 0;
      Linha *l;
      if (!spotpessoa_item(consulta, i, &sp)) break;
      for (k = 0; k < nv; k++) if (vistos[k] == sp.tmdb) { dup = 1; break; }
      if (dup) continue;
      if (!cab) { cabecalho(i18n("Pessoas")); cab = 1; }
      if (!(l = nova(L_PESSOA))) return;
      vistos[nv++] = sp.tmdb;
      l->tmdb = sp.tmdb;
      l->tituloTmdb = sp.tituloTmdb;
      snprintf(l->tituloTipo, sizeof l->tituloTipo, "%s", sp.tituloTipo);
      snprintf(l->t1, sizeof l->t1, "%s", sp.nome);
      if (sp.conhecido[0]) snprintf(l->t2, sizeof l->t2, i18n("Conhecido por  %s"), sp.conhecido);
      snprintf(l->arte, sizeof l->arte, "%s", sp.foto);
      snprintf(l->chave, sizeof l->chave, "p|%ld", l->tmdb);
    } }
}

static void montarColecoes(const char *alvo) {
  int i, n = col_n(), achou = 0;
  char t[300];
  for (i = 0; i < n && achou < 3; i++) {
    const ColFolder *f = col_folder(i);
    Linha *l;
    if (!f || !f->title[0]) continue;
    busca_normalizar(f->title, t, sizeof t);
    if (!pontuar(t, alvo)) {
      busca_normalizar(f->group, t, sizeof t);
      if (!f->group[0] || pontuar(t, alvo) < 60) continue;
    }
    if (!achou) cabecalho(i18n("Coleções"));
    if (!(l = nova(L_COLECAO))) return;
    achou++;
    l->ref = i;
    snprintf(l->t1, sizeof l->t1, "%s", f->title);
    snprintf(l->t2, sizeof l->t2, "%s%s%s", i18n("Coleção"), f->group[0] ? "  \xc2\xb7  " : "", f->group);
    snprintf(l->arte, sizeof l->arte, "%s", col_capa(f) ? col_capa(f) : "");
    snprintf(l->chave, sizeof l->chave, "c|%s", f->id);
  }
}

static void montarCanais(const char *alvo) {
  int idx[4], n = guia_buscar_canais(alvo, idx, 4), i;
  if (n > 0) cabecalho(i18n("Canais ao vivo"));
  for (i = 0; i < n; i++) {
    const char *id, *nome, *logo, *cat, *base;
    Linha *l;
    if (!guia_canal_campos(idx[i], &id, &nome, &logo, &cat, &base) || !(l = nova(L_CANAL))) continue;
    l->ref = idx[i];
    snprintf(l->id, sizeof l->id, "%s", id);
    snprintf(l->t1, sizeof l->t1, "%s", nome);
    // A categoria passa por i18n como no guia (linhaNome): as secoes de
    // categoriaPorNome sao chaves da tabela; genero do addon volta como veio.
    snprintf(l->t2, sizeof l->t2, "%s%s%s", i18n("Canal"), cat[0] ? "  \xc2\xb7  " : "",
             cat[0] ? i18n(cat) : "");
    snprintf(l->arte, sizeof l->arte, "%s", logo);
    snprintf(l->base, sizeof l->base, "%s", base);
    snprintf(l->chave, sizeof l->chave, "k|%.90s", id);
  }
}

static void montarCatalogos(const char *alvo) {
  int r, achou = 0, i;
  char t[300];
  for (r = 0; r < cat_n_fileiras() && achou < 3; r++) {
    const CatFileira *cf = cat_fileira(r);
    Linha *l;
    if (!cf || !cf->base[0] || !cf->catId[0]) continue;
    busca_normalizar(cf->titulo, t, sizeof t);
    if (pontuar(t, alvo) < 60) continue;
    if (!achou) cabecalho(i18n("Catálogos"));
    if (!(l = nova(L_CATALOGO))) return;
    achou++;
    l->ref = r;
    snprintf(l->t1, sizeof l->t1, "%s", cf->titulo);
    snprintf(l->t2, sizeof l->t2, "%s  \xc2\xb7  %d %s", i18n("Catálogo"), cf->n,
             i18n(cf->n == 1 ? "título" : "títulos"));
    snprintf(l->icone, sizeof l->icone, "aj_rows-3");
    snprintf(l->chave, sizeof l->chave, "f|%.90s", cf->chave);
  }
  for (i = 0, achou = 0; i < addons_n() && achou < 2; i++) {
    const char *nm = addons_nome(i);
    Linha *l;
    if (!nm || !nm[0]) continue;
    busca_normalizar(nm, t, sizeof t);
    if (pontuar(t, alvo) < 60) continue;
    if (!achou) cabecalho(i18n("Addons"));
    if (!(l = nova(L_ADDON))) return;
    achou++;
    l->ref = i;
    snprintf(l->t1, sizeof l->t1, "%s", nm);
    snprintf(l->t2, sizeof l->t2, "%s", i18n(addons_ativo(i) ? "Addon ativo" : "Addon desligado"));
    snprintf(l->icone, sizeof l->icone, "addon");
    snprintf(l->chave, sizeof l->chave, "a|%.90s", nm);
  }
}

// Campo vazio: pesquisas recentes e "Em alta" (os primeiros da primeira
// fileira de catalogo de addon — o que o dono ja ve no topo da home, e o unico
// "em alta" que existe sem uma viagem de rede).
// CAMPO VAZIO: so as pesquisas recentes, curtas (o dono: "mantenha
// discreto"). Sem recentes, a barra fica sozinha — nada de "Em alta" nem de
// frase de ajuda abrindo o corpo antes de a pessoa digitar.
static void montarVazio(void) {
  int i, n = buscasrec_n();
  if (n <= 0) return;
  cabecalho(i18n("Pesquisas recentes"));
  for (i = 0; i < n && i < SP_MAX_RECENTES; i++) {
    Linha *l = nova(L_RECENTE);
    if (!l) return;
    l->ref = i;
    snprintf(l->t1, sizeof l->t1, "%s", buscasrec_termo(i));
    snprintf(l->icone, sizeof l->icone, "aj_rotate-ccw-clock");
    snprintf(l->chave, sizeof l->chave, "r|%s", l->t1);
  }
  { Linha *l = nova(L_LIMPAR);
    if (l) { snprintf(l->t1, sizeof l->t1, "%s", i18n("Limpar pesquisas recentes"));
             snprintf(l->chave, sizeof l->chave, "limpar"); } }
}

static void remontar(void) {
  char alvo[SP_MAX_TXT * 2];
  char chaveFoco[96] = "";
  static char chavesAntes[SP_MAX_LIN][96];
  float entraAntes[SP_MAX_LIN];
  int nAntes = nLin, i, j;
  if (focoL >= 0 && focoL < nLin) snprintf(chaveFoco, sizeof chaveFoco, "%s", lin[focoL].chave);
  for (i = 0; i < nLin; i++) { memcpy(chavesAntes[i], lin[i].chave, 96); entraAntes[i] = entraLin[i]; }
  snprintf(montada, sizeof montada, "%s", consulta);
  nLin = 0;
  busca_normalizar(consulta, alvo, sizeof alvo);
  // Sempre, inclusive com o campo vazio: o debounce precisa saber que o
  // texto mudou para nao disparar um termo que ja nao esta no campo.
  spotpessoa_pedir(consulta, SDL_GetTicks());
  if (busca_codepoints(alvo) < 2) montarVazio();
  else {
    desc_buscar(consulta);
    montarTitulos(alvo);
    montarPessoas(alvo);
    montarColecoes(alvo);
    montarCanais(alvo);
    montarCatalogos(alvo);
    if (desc_buscando()) {
      Linha *l = nova(L_AVISO);
      if (l) { snprintf(l->t1, sizeof l->t1, "%s", i18n("Buscando nos seus addons…"));
               snprintf(l->chave, sizeof l->chave, "aviso"); }
    } else if (nLin == 0) {
      Linha *l = nova(L_AVISO);
      if (l) { snprintf(l->t1, sizeof l->t1, i18n("Nada encontrado para “%s”."), consulta);
               snprintf(l->t2, sizeof l->t2, "%s", i18n("Confira a grafia ou tente o nome original."));
               snprintf(l->chave, sizeof l->chave, "aviso"); }
    }
  }
  // Posicoes, e quem ja estava herda foco e entrada.
  { float y = 0.0f;
    for (i = 0; i < nLin; i++) {
      lin[i].h = ALTURA[lin[i].tipo];
      if (lin[i].tipo == L_AVISO && lin[i].t2[0]) lin[i].h += 30.0f;
      if (lin[i].tipo == L_CAB && i > 0) y += 14.0f;
      lin[i].y = y; y += lin[i].h;
      entraLin[i] = 0.0f;
      for (j = 0; j < nAntes; j++)
        if (lin[i].chave[0] && !strcmp(chavesAntes[j], lin[i].chave)) { entraLin[i] = entraAntes[j]; break; }
    } }
  memset(animLin, 0, sizeof animLin);
  focoL = -1;
  if (chaveFoco[0])
    for (i = 0; i < nLin; i++) if (!strcmp(lin[i].chave, chaveFoco)) { focoL = i; break; }
  if (focoL < 0) for (i = 0; i < nLin; i++) if (focavel(lin[i].tipo)) { focoL = i; break; }
  if (focoL < 0 && painel == P_LISTA) painel = P_CAMPO;
  ultimoRemoto = busca_codepoints(alvo) >= 2 ? remotoTotal() : -1;
  ultimoBuscando = desc_buscando();
  ultimaGeracao = desc_busca_geracao();
  ultimaGerPessoa = spotpessoa_geracao();
}

static int temResultados(void) {
  int i;
  for (i = 0; i < nLin; i++)
    if (focavel(lin[i].tipo) && lin[i].tipo != L_RECENTE && lin[i].tipo != L_LIMPAR) return 1;
  return 0;
}
// QUANDO A BUSCA CONTA COMO FEITA: a mesma regra da tela de Busca (busca.c,
// registrarConsulta) — abriu um resultado, fechou com resultado na tela, ou
// o ditado trouxe o texto. Nunca por letra.
static void registrar(void) {
  if (nConsulta >= 2 && temResultados()) buscasrec_registrar(consulta);
}

// --- Campo -------------------------------------------------------------------------
static void acrescentar(const char *t) {
  size_t n = strlen(t);
  if ((size_t)nConsulta + n + 1 > SP_MAX_TXT) return;
  memcpy(consulta + nConsulta, t, n);
  nConsulta += (int)n;
  consulta[nConsulta] = 0;
}
static void apagar(void) { nConsulta = (int)busca_apagar_ultimo(consulta, (size_t)nConsulta); }

// O campo inteiro de uma vez (ditado, teclado do sistema, testes). `aparar` tira
// os espacos das pontas — o reconhecedor as vezes devolve; o IME NAO apara: o
// espaco que a pessoa acabou de digitar tem de aparecer antes da proxima letra.
static void campoDefinir(const char *t, int aparar) {
  size_t i, w = 0;
  if (!t) return;
  if (aparar) while (*t == ' ' || *t == '\n') t++;
  for (i = 0; t[i] && w + 1 < sizeof consulta; i++)
    consulta[w++] = (t[i] == '\n' || t[i] == '\t') ? ' ' : t[i];
  if (aparar) while (w > 0 && consulta[w - 1] == ' ') w--;
  // nao deixa meia sequencia UTF-8 no fim (o corte em 47 bytes pode cair nela)
  if (w > 0) {
    size_t k = w;
    while (k > 0 && ((unsigned char)consulta[k - 1] & 0xC0) == 0x80) k--;
    if (k > 0) {
      unsigned char c0 = (unsigned char)consulta[k - 1];
      size_t len = c0 < 0x80 ? 1 : (c0 >= 0xF0 ? 4 : (c0 >= 0xE0 ? 3 : 2));
      if (k - 1 + len > w) w = k - 1;
    }
  }
  consulta[w] = 0;
  nConsulta = (int)w;
  if (strcmp(consulta, montada)) remontar();
}
void spot_texto_externo(const char *t) { campoDefinir(t, 1); }


static void entrarLista(void);

// --- Teclado e voz do sistema (sistexto.h) ----------------------------------------
static void abrirTecladoSis(void) { st_ime_abrir(ST_SPOT, consulta, SP_MAX_TXT - 1); }
static void ditar(void) { if (ditadoDisponivel()) st_voz_iniciar(ST_SPOT); }

// OK no campo: o teclado do sistema onde ha; senao o do app, com o foco nele.
static void okCampo(void) {
  if (imeDisponivel()) { abrirTecladoSis(); return; }
  kbAberto = 1;
  painel = P_TECLADO;
  if (kbF > kbFil) kbF = 0;
}

static void lerSistema(void) {
  char t[SP_MAX_TXT * 2];
  // DO CELULAR: o campo inteiro, como se a pessoa tivesse digitado, e o foco
  // vai para os resultados (o mesmo que o "Concluir" do teclado da TV).
  if (celb_pegar(CELB_SPOT, t, sizeof t)) {
    st_fechar(ST_SPOT);
    campoDefinir(t, 1);
    memset(t, 0, sizeof t);
    registrar();
    if (temResultados()) entrarLista(); else painel = P_CAMPO;
    return;
  }
  int r = st_ler(ST_SPOT, t, sizeof t);
  int voz = ouvindo();
  if (r == ST_NADA) return;
  if (r == ST_PEDE_TECLADO) { painel = P_CAMPO; abrirTecladoSis(); return; }
  if (r == ST_TEXTO || r == ST_FIM) campoDefinir(t, voz || r == ST_FIM);
  if (r == ST_FIM) {
    registrar();
    // Fim da fala ou "Concluir": o proximo passo provavel e escolher.
    if (temResultados()) entrarLista(); else painel = P_CAMPO;
  }
  if (r == ST_CANCELOU && painel != P_LISTA) painel = P_CAMPO;
}

// --- Ciclo de vida -------------------------------------------------------------------
void spot_abrir(int voz) {
  kbMontar();
  guia_preparar_busca();
  aberto = 1;
  painel = P_CAMPO; kbF = 0; kbC = 0;
  kbAberto = 0; kbAnim = 0.0f;
  corpoH = corpoV = 0.0f; animMic = 0.0f; nivelVoz = 0.0f;
  nConsulta = 0; consulta[0] = 0; montada[0] = 0;
  scrollY = scrollAlvo = velY = 0.0f;
  temPedido = 0; okPress = okLongo = 0;
  memset(animTecla, 0, sizeof animTecla);
  nLin = 0; focoL = -1;
  memset(entraLin, 0, sizeof entraLin);
  remontar();
  // O corpo das recentes ja nasce aberto: crescer de zero a cada abertura
  // seria a barra "pulando" antes de a pessoa fazer qualquer coisa.
  printf("[spotlight] aberto (%s)\n", voz ? "voz" : "tecla");
#if defined(__linux__) && !defined(NV_TPK) && !defined(NV_ANDROID) && !defined(__EMSCRIPTEN__)
  // LG: o teclado do SISTEMA por SDL_StartTextInput nao esta ligado (ver
  // spotlight.h, "LG E O TECLADO DO SISTEMA"). Uma linha por sessao para o
  // D1 dizer o que o SDL do aparelho responde, sem chamar nada que mude a tela.
  { static int dito;
    if (!dito) {
      SDL_version v;
      dito = 1;
      SDL_GetVersion(&v);
      printf("[spotlight] lg sdl %d.%d.%d osk=%d textinput=%d\n", v.major, v.minor, v.patch,
             (int)SDL_HasScreenKeyboardSupport(), (int)SDL_IsTextInputActive());
    } }
#endif
  fflush(stdout);
  if (voz && ditadoDisponivel()) { painel = P_MIC; ditar(); }
  else if (imeDisponivel() && st_abre_sozinho()) abrirTecladoSis();
}

void spot_fechar(void) {
  if (!aberto) return;
  aberto = 0;
  okPress = okLongo = 0;
  kbAberto = 0;
  st_fechar(ST_SPOT);
  celb_fechar_dono(CELB_SPOT);
}

int spot_aberto(void)  { return aberto; }
int spot_visivel(void) { return aberto || entrada > 0.004f; }
int spot_cheio(void)   { return aberto && entrada >= 0.999f; }
const char *spot_consulta(void) { return consulta; }
int spot_n_linhas(void) { return nLin; }
int spot_linha_tipo(int i) { return i >= 0 && i < nLin ? lin[i].tipo : -1; }
const char *spot_linha_texto(int i) { return i >= 0 && i < nLin ? lin[i].t1 : ""; }
int spot_linha_focada(void) { return painel == P_LISTA ? focoL : -1; }
int spot_teclado_app_aberto(void) { return kbAberto; }
int spot_foco_campo(void) { return painel == P_CAMPO ? 1 : painel == P_MIC ? 2 : painel == P_CEL ? 3 : 0; }
float spot_altura_corpo(void) { return corpoH; }

int spot_pediu(SpotPedido *p) {
  if (!temPedido) return 0;
  temPedido = 0;
  if (p) *p = pedido;
  return 1;
}

static void acionar(int i) {
  Linha *l;
  if (i < 0 || i >= nLin) return;
  l = &lin[i];
  memset(&pedido, 0, sizeof pedido);
  switch (l->tipo) {
    case L_RECENTE:
      snprintf(consulta, sizeof consulta, "%s", buscasrec_termo(l->ref));
      nConsulta = (int)strlen(consulta);
      buscasrec_registrar(consulta);
      remontar();
      return;
    case L_LIMPAR:
      buscasrec_limpar();
      painel = P_CAMPO;
      remontar();
      return;
    case L_TOPO: case L_TITULO:
      pedido.tipo = SPOT_TITULO; pedido.indice = l->ref; break;
    case L_PESSOA:
      pedido.tipo = SPOT_PESSOA; pedido.indice = l->ref2; pedido.tmdb = l->tmdb;
      pedido.tituloTmdb = l->tituloTmdb;
      snprintf(pedido.tituloTipo, sizeof pedido.tituloTipo, "%s", l->tituloTipo);
      snprintf(pedido.nome, sizeof pedido.nome, "%s", l->t1);
      snprintf(pedido.arte, sizeof pedido.arte, "%s", l->arte);
      break;
    case L_COLECAO:  pedido.tipo = SPOT_COLECAO;  pedido.indice = l->ref; break;
    case L_CATALOGO: pedido.tipo = SPOT_CATALOGO; pedido.indice = l->ref; break;
    case L_ADDON:    pedido.tipo = SPOT_ADDONS;   pedido.indice = l->ref; break;
    case L_CANAL:
      pedido.tipo = SPOT_CANAL;
      snprintf(pedido.id, sizeof pedido.id, "%s", l->id);
      snprintf(pedido.nome, sizeof pedido.nome, "%s", l->t1);
      snprintf(pedido.base, sizeof pedido.base, "%s", l->base);
      break;
    default: return;
  }
  registrar();
  temPedido = 1;
  spot_fechar();
}

static void removerRecente(int i) {
  if (i < 0 || i >= nLin) return;
  if (lin[i].tipo == L_RECENTE) buscasrec_remover(lin[i].ref);
  else if (lin[i].tipo == L_LIMPAR) buscasrec_limpar();
  else return;
  remontar();
}

static int primeiraFocavel(void) {
  int i;
  for (i = 0; i < nLin; i++) if (focavel(lin[i].tipo)) return i;
  return -1;
}

// Cima da primeira linha volta ao campo (o unico lugar acima dela).
static void moverLista(int d) {
  int i = focoL;
  if (nLin == 0) return;
  for (;;) {
    i += d;
    if (i < 0) { if (d < 0) painel = P_CAMPO; return; }
    if (i >= nLin) return;
    if (focavel(lin[i].tipo)) { focoL = i; return; }
  }
}

// Descer aos resultados FECHA o teclado do app: ele so serve ao campo.
static void entrarLista(void) {
  if (focoL < 0 || focoL >= nLin || !focavel(lin[focoL].tipo)) focoL = primeiraFocavel();
  if (focoL >= 0) { painel = P_LISTA; kbAberto = 0; }
}

static void aplicarTecla(void) {
  if (kbF < kbFil) {
    int k = kbF * SP_KB_COLS + kbC;
    if (k < kbN) acrescentar(kbTeclas[k]);
  } else {
    switch (kbCmd[kbC]) {
      case K_ESPACO: if (nConsulta > 0 && consulta[nConsulta - 1] != ' ') acrescentar(" "); break;
      case K_APAGAR: apagar(); break;
      case K_LIMPAR: registrar(); nConsulta = 0; consulta[0] = 0; break;
      case K_FALAR:  ditar(); return;
      case K_TECLADO: abrirTecladoSis(); return;
      default: break;
    }
  }
  remontar();
}

static void kbMover(int dx, int dy) {
  if (dy) {
    int nf = kbF + dy;
    if (nf < 0) { painel = P_CAMPO; return; }   // cima da primeira fileira: o campo
    if (nf > kbFil) return;
    // Entre a grade e a fileira de comandos o x e o que conta: a coluna da
    // tecla mais perto do centro da tecla de onde se saiu.
    { GfxRect a = teclaRect(kbF, kbC);
      float cx = a.x + a.w * 0.5f, melhor = 1e9f;
      int c, nc = kbColunas(nf), alvo = 0;
      for (c = 0; c < nc; c++) {
        GfxRect b = teclaRect(nf, c);
        float d = b.x + b.w * 0.5f - cx;
        if (d < 0) d = -d;
        if (d < melhor) { melhor = d; alvo = c; }
      }
      kbF = nf; kbC = alvo; }
    return;
  }
  if (dx < 0) { if (kbC > 0) kbC--; return; }
  if (kbC + 1 < kbColunas(kbF)) kbC++;
  else entrarLista();
}

static void focarTecla(int f, int c) { painel = P_TECLADO; kbF = f; kbC = c; }
static void focarLinha(int i, int b) {
  (void)b;
  if (i >= 0 && i < nLin && focavel(lin[i].tipo)) { painel = P_LISTA; focoL = i; }
}
static void focarCampo(int a, int b) { (void)b; painel = a == 2 ? P_CEL : a ? P_MIC : P_CAMPO; }

// Baixo a partir da barra: o teclado do app (se aberto) ou a lista.
static void descerDaBarra(void) {
  if (kbAberto) painel = P_TECLADO;
  else entrarLista();
}

void spot_evento(const SDL_Event *e) {
  SDL_Keycode k;
  if (!aberto) return;
  // Teclado da TV aberto: o texto (e o Backspace) ja entraram no valor inteiro.
  if (st_evento(e)) return;
  if (e->type == SDL_TEXTINPUT) {
    // Teclado FISICO: ASCII alfanumerico chega TAMBEM como KEYDOWN (tratado
    // abaixo); o resto (acentos, cirilico, pontuacao) so por aqui.
    unsigned char c = (unsigned char)e->text.text[0];
    if (c >= 0x80 || (c > ' ' && c < 0x7f && !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                                              (c >= '0' && c <= '9')))) {
      acrescentar(e->text.text);
      remontar();
    }
    return;
  }
  if (e->type != SDL_KEYDOWN && e->type != SDL_KEYUP) return;
  k = e->key.keysym.sym;

  // OK numa pesquisa recente decide na SOLTURA (toque = buscar, segurar =
  // remover), como as pilulas da tela de Busca.
  if ((k == SDLK_RETURN || k == SDLK_KP_ENTER) && painel == P_LISTA && focoL >= 0 && focoL < nLin &&
      (lin[focoL].tipo == L_RECENTE || lin[focoL].tipo == L_LIMPAR)) {
    if (e->type == SDL_KEYDOWN) {
      if (!okPress) { okPress = 1; okLongo = 0; okDesde = SDL_GetTicks(); }
    } else if (okPress) {
      okPress = 0;
      if (okLongo) okLongo = 0;
      else acionar(focoL);
    }
    return;
  }
  if (e->type != SDL_KEYDOWN) { okPress = 0; return; }

  if (k == SPOT_TECLA_ABRIR || e->key.keysym.scancode == NV_SCANCODE_YELLOW) {
    registrar(); spot_fechar(); return;
  }
  // VOLTAR desfaz na ordem inversa (ver o topo).
  if (k == SDLK_AC_BACK || k == SDLK_ESCAPE || e->key.keysym.scancode == NV_SCANCODE_BACK) {
    if (ouvindo()) { st_fechar(ST_SPOT); painel = P_CAMPO; return; }
    if (kbAberto || painel == P_LISTA || painel == P_TECLADO || painel == P_CEL) {
      kbAberto = 0; painel = P_CAMPO; return;
    }
    if (nConsulta > 0) { registrar(); nConsulta = 0; consulta[0] = 0; remontar(); return; }
    spot_fechar(); return;
  }
  if (k == SPOT_TECLA_VOZ) {
    if (ditadoDisponivel()) { painel = P_MIC; ditar(); }
    else { registrar(); spot_fechar(); }
    return;
  }
  if (k == SDLK_BACKSPACE || k == SDLK_DELETE) {
    if (nConsulta > 0) { apagar(); remontar(); if (painel == P_LISTA) painel = P_CAMPO; }
    else spot_fechar();
    return;
  }
  if (!(e->key.keysym.mod & (KMOD_CTRL | KMOD_ALT | KMOD_GUI)) &&
      ((k >= SDLK_a && k <= SDLK_z) || (k >= SDLK_0 && k <= SDLK_9) || k == SDLK_SPACE)) {
    if (k != SDLK_SPACE || (nConsulta && consulta[nConsulta - 1] != ' ')) {
      char um[2] = { (char)k, 0 };
      acrescentar(um);
      remontar();
    }
    if (painel == P_LISTA || painel == P_MIC || painel == P_CEL) painel = P_CAMPO;
    return;
  }
  switch (painel) {
    case P_CAMPO: case P_MIC: case P_CEL:
      switch (k) {
        // DIREITA anda campo -> Falar -> Celular, o que existir aqui.
        case SDLK_RIGHT:
          if (painel == P_CAMPO && ditadoDisponivel()) painel = P_MIC;
          else if (painel != P_CEL && celDisponivel()) painel = P_CEL;
          break;
        case SDLK_LEFT:
          if (painel == P_CEL && ditadoDisponivel()) painel = P_MIC;
          else if (painel != P_CAMPO) painel = P_CAMPO;
          break;
        case SDLK_DOWN: case SDLK_TAB: descerDaBarra(); break;
        case SDLK_RETURN: case SDLK_KP_ENTER:
          if (e->key.repeat) break;
          if (painel == P_MIC) ditar();
          else if (painel == P_CEL) { st_fechar(ST_SPOT); celb_abrir(CELB_SPOT, "Buscar"); }
          else okCampo();
          break;
        default: break;
      }
      return;
    case P_TECLADO:
      switch (k) {
        case SDLK_LEFT:  kbMover(-1, 0); break;
        case SDLK_RIGHT: kbMover(1, 0);  break;
        case SDLK_UP:    kbMover(0, -1); break;
        case SDLK_DOWN:  kbMover(0, 1);  break;
        case SDLK_TAB:   entrarLista();  break;
        case SDLK_RETURN: case SDLK_KP_ENTER: aplicarTecla(); break;
        default: break;
      }
      return;
    default: break;
  }
  switch (k) {
    case SDLK_LEFT: case SDLK_TAB: painel = P_CAMPO; break;
    case SDLK_UP:   moverLista(-1); break;
    case SDLK_DOWN: moverLista(1);  break;
    case SDLK_RETURN: case SDLK_KP_ENTER: acionar(focoL); break;
    default: break;
  }
}

// Altura que o corpo quer: a lista inteira (ate o teto) ou o teclado do app.
static float alturaLista(void) { return nLin ? lin[nLin - 1].y + lin[nLin - 1].h : 0.0f; }
static float alturaTeclado(void) { return (kbFil + 1) * SP_KB_PASSO - SP_TECLA_GAP + 52.0f; }
static float corpoAlvo(void) {
  float h = alturaLista();
  if (kbAberto && alturaTeclado() > h) h = alturaTeclado();
  if (h <= 0.0f) return 0.0f;
  h += SP_CPAD_T + SP_CPAD_B + SP_RODAPE_H;
  return h > SP_CORPO_MAX ? SP_CORPO_MAX : h;
}
// A janela da lista dentro do corpo (para a rolagem): pelo alvo, nao pela mola.
static float listaVisivel(void) {
  float h = corpoAlvo() - SP_CPAD_T - SP_CPAD_B - SP_RODAPE_H;
  return h > 0.0f ? h : 0.0f;
}

static float molaIlha(float *v, float x, float alvo, float dt) {
  int k;
  if (anim_politica_reduzida || ajustes_animacoes_reduzidas()) { *v = 0.0f; return alvo; }
  if (dt > 0.05f) dt = 0.05f;
  for (k = 0; k < 4; k++) {
    float h = dt * 0.25f, ac = SP_MOLA_W * SP_MOLA_W * (alvo - x) - 2.0f * SP_MOLA_Z * SP_MOLA_W * (*v);
    *v += ac * h;
    x += *v * h;
  }
  return x;
}

void spot_atualizar(float dt, Uint32 agora) {
  int i, f, c;
  entrada = anim_mola(entrada, aberto ? 1.0f : 0.0f, dt, aberto ? 16.0f : 22.0f);
  if (!aberto) { if (entrada < 0.004f) entrada = 0.0f; return; }

  lerSistema();
  // A RESPOSTA DA REDE CHEGA DEPOIS DA TECLA: remonta quando a contagem do termo
  // corrente muda ou quando a busca termina (o aviso "Buscando..." sai).
  spotpessoa_atualizar(agora);
  if (strcmp(montada, consulta)) remontar();
  else if (nConsulta >= 2) {
    int n = remotoTotal(), b = desc_buscando(), g = desc_busca_geracao();
    if (n != ultimoRemoto || b != ultimoBuscando || g != ultimaGeracao ||
        spotpessoa_geracao() != ultimaGerPessoa) remontar();
  }
  corpoH = molaIlha(&corpoV, corpoH, corpoAlvo(), dt);
  if (corpoH < 0.0f) { corpoH = 0.0f; if (corpoV < 0.0f) corpoV = 0.0f; }
  kbAnim = anim_mola(kbAnim, kbAberto ? 1.0f : 0.0f, dt, NV_MOLA_TELA);
  nivelVoz = anim_mola(nivelVoz, st_nivel(), dt, 18.0f);
  animMic = anim_mola(animMic, painel == P_MIC ? 1.0f : 0.0f, dt, NV_MOLA_FOCO);
  for (f = 0; f <= kbFil && f <= SP_KB_MAX_FIL; f++)
    for (c = 0; c < SP_KB_COLS; c++) {
      float alvo = (painel == P_TECLADO && f == kbF && c == kbC) ? 1.0f : 0.0f;
      animTecla[f][c] = anim_mola(animTecla[f][c], alvo, dt, NV_MOLA_FOCO);
    }
  animCampo = anim_mola(animCampo, painel == P_CAMPO ? 1.0f : 0.0f, dt, NV_MOLA_FOCO);
  for (i = 0; i < nLin; i++) {
    float alvo = (painel == P_LISTA && i == focoL) ? 1.0f : 0.0f;
    animLin[i] = anim_mola(animLin[i], alvo, dt, NV_MOLA_FOCO);
    entraLin[i] = anim_mola(entraLin[i], 1.0f, dt, 14.0f);
  }
  if (painel == P_LISTA && okPress && !okLongo && agora - okDesde >= NV_HOLD_MS) {
    okLongo = 1;
    removerRecente(focoL);
  }
  if (painel != P_LISTA) okPress = okLongo = 0;
  // Rolagem: so o necessario para a linha focada caber (com o cabecalho do
  // grupo dela visivel, quando ele e a linha de cima).
  if (painel == P_LISTA && focoL >= 0) {
    float topo = lin[focoL].y, base = topo + lin[focoL].h, vis = listaVisivel();
    if (focoL > 0 && lin[focoL - 1].tipo == L_CAB) topo = lin[focoL - 1].y;
    if (topo - scrollAlvo < 0.0f) scrollAlvo = topo;
    if (base - scrollAlvo > vis) scrollAlvo = base - vis;
  } else if (painel != P_LISTA) scrollAlvo = 0.0f;
  if (scrollAlvo < 0.0f) scrollAlvo = 0.0f;
  scrollY = anim_mola2(&velY, scrollY, scrollAlvo, dt, NV_MOLA2_SCROLL);
}

// --- Desenho -----------------------------------------------------------------------
static float listaX = SP_LISTA_X0, listaW = SP_LISTA_XF - SP_LISTA_X0;

void spot_veu(void) {
  gfx_cor((GfxRect){ 0, 0, NV_TELA_W, NV_TELA_H }, 0.0f, 0.0f, 0.0f, 0.01f, 0.62f);
}

// A superficie unica: barra sozinha = pilula; crescendo, o raio vai para o de
// painel e a parte de baixo vira o corpo.
static void desenhaSuperficie(GfxRect p, float a) {
  float ar, ag, ab, menor = p.w < p.h ? p.w : p.h;
  float abre = anim_clamp(corpoH / 120.0f, 0.0f, 1.0f);
  float raio = anim_mistura(SP_BH * 0.5f, SP_RAIO, abre) / menor;
  ajustes_acento(&ar, &ag, &ab);
  // Uma luz curta da cor do tema atras, no alto: tira a barra do plano de tras
  // sem uma sombra de tela inteira.
  gfx_rect((GfxRect){ p.x - 60.0f, p.y - 46.0f, p.w + 120.0f, SP_BH + 120.0f }, 0, GFX_SOMBRA,
           1.0f, 0, 0, 0.5f, ar, ag, ab, 0.12f * a);
  gfx_rect((GfxRect){ p.x - 40.0f, p.y - 10.0f, p.w + 80.0f, p.h + 70.0f }, 0, GFX_SOMBRA,
           1.0f, 0, 0, 0.5f, 0, 0, 0, 0.45f * a);
  // No vidro a folha sozinha deixa os cartazes de tras competirem com o texto
  // (captura de 01/10): um miolo escuro por baixo dela.
  if (ajustes_vidro()) { gfx_cor(p, raio, 0.03f, 0.032f, 0.04f, 0.55f * a); gfx_vidro_folha(p, raio, a); }
  else {
    gfx_cor(p, raio, 0.066f, 0.070f, 0.084f, 0.95f * a);
    gfx_luz_canto(p, raio, p.w * 0.18f, -40.0f, 620.0f, ar, ag, ab, 0.06f * a);
  }
  gfx_vidro_aro(p, raio, 1.5f, 1.0f, 1.0f, 1.0f, (0.10f + 0.10f * animCampo) * a);
  // Separador entre a barra e o corpo, so com o corpo aberto.
  if (corpoH > 6.0f)
    gfx_cor((GfxRect){ p.x + 30.0f, SP_CORPO_Y - 1.0f + (p.y - SP_BY), p.w - 60.0f, 1.5f }, 0.0f,
            1.0f, 1.0f, 1.0f, 0.08f * abre * a);
}

static void desenhaCampo(float dy, float a, Uint32 agora) {
  GfxRect barra = { SP_BX, SP_BY + dy, SP_BW, SP_BH };
  float ar, ag, ab, tx, xMax;
  int ouve = ouvindo(), comMic = ditadoDisponivel();
  const char *av = st_dono() == ST_SPOT || !st_dono() ? st_aviso() : "";
  ajustes_acento(&ar, &ag, &ab);
  // Foco no campo: um aro na cor do tema por dentro da barra.
  if (animCampo > 0.01f) {
    GfxRect r = { barra.x + 8.0f, barra.y + 8.0f, barra.w - botoesW() - 16.0f,
                  barra.h - 16.0f };
    gfx_vidro_aro(r, 0.5f, 2.5f, ar, ag, ab, 0.9f * animCampo * a);
    gfx_cor(r, 0.5f, ar, ag, ab, 0.10f * animCampo * a);
  }
  if (ponteiro_ativo()) {
    ponteiro_alvo(barra.x, barra.y, barra.w - botoesW(), barra.h,
                  focarCampo, NULL, 0, 0);
  }
  gfx_icone((GfxRect){ barra.x + 40.0f, barra.y + (barra.h - 40.0f) * 0.5f, 40.0f, 40.0f },
            "menu_search", 0.78f, 0.79f, 0.83f, a);
  tx = barra.x + 40.0f + 40.0f + 24.0f;
  xMax = barra.x + barra.w - (botoesW() > 0.0f ? botoesW() + 16.0f : 40.0f) - (ouve ? 170.0f : 0.0f);
  if (nConsulta) {
    TxtLinha l = txt_linha_corta(TXT_HEADLINE, consulta, 246, 247, 251, 255, xMax - tx - 10.0f);
    txt_desenhar_alpha(l, tx, barra.y + (barra.h - l.h) * 0.5f, a);
    tx += l.w + 6.0f;
  } else {
    const char *ph = ouve ? i18n(st_estado() == ST_PERMISSAO ? "Permita o microfone para falar…" : "Ouvindo…")
                   : av[0] ? i18n(av)
                   : i18n(comMic ? "Buscar ou falar: filmes, séries, pessoas, canais"
                                 : "Buscar filmes, séries, pessoas e canais");
    int amb = !ouve && av[0];
    TxtLinha l = txt_linha_corta(amb ? TXT_BODY : TXT_HEADLINE, ph, amb ? 240 : 255, amb ? 190 : 255,
                                 amb ? 130 : 255, 255, xMax - tx - 10.0f);
    txt_desenhar_alpha(l, tx, barra.y + (barra.h - l.h) * 0.5f, (amb ? 0.95f : 0.45f) * a);
  }
  // Cursor: com o foco no campo ou com o teclado do sistema aberto para ele.
  if ((painel == P_CAMPO || painel == P_TECLADO || digitandoSis()) && (agora / 500) % 2 == 0 &&
      (nConsulta > 0 || digitandoSis()))
    gfx_cor((GfxRect){ tx, barra.y + 28.0f, 3.0f, barra.h - 56.0f }, 0.5f, ar, ag, ab, 0.95f * a);
  // MICROFONE: botao focavel a direita, so onde ha voz (nao se promete o que a
  // TV nao faz). Ouvindo, acende na cor do tema e um anel cresce com o som.
  if (comMic) {
    float d = SP_MIC_D, cx = barra.x + barra.w - 26.0f - d - (celDisponivel() ? d + 14.0f : 0.0f);
    float cy = barra.y + (barra.h - d) * 0.5f;
    float k = animMic;
    if (ponteiro_ativo()) ponteiro_alvo(cx - 8, barra.y, d + 16, barra.h, focarCampo, NULL, 1, 0);
    if (ouve) {
      float anel = 10.0f + 26.0f * nivelVoz + 4.0f * SDL_sinf(agora * 0.006f);
      float aro = 6.0f + 20.0f * nivelVoz;
      // Com texto no campo o placeholder "Ouvindo…" some: a palavra vai ao lado.
      if (nConsulta) {
        TxtLinha o = txt_linha(TXT_CAPTION, i18n("Ouvindo…"), 200, 204, 212, 255);
        txt_desenhar_alpha(o, cx - 22.0f - o.w, barra.y + (barra.h - o.h) * 0.5f, 0.9f * a);
      }
      // O anel que respira com a voz: e o que diz "estou ouvindo VOCE".
      gfx_vidro_aro((GfxRect){ cx - aro, cy - aro, d + 2 * aro, d + 2 * aro }, 0.5f, 2.0f,
                    ar, ag, ab, (0.25f + 0.45f * nivelVoz) * a);
      gfx_rect((GfxRect){ cx - anel, cy - anel, d + 2 * anel, d + 2 * anel }, 0, GFX_SOMBRA, 1.0f, 0, 0,
               0.5f, ar, ag, ab, (0.30f + 0.35f * nivelVoz) * a);
      gfx_cor((GfxRect){ cx - 6.0f * nivelVoz, cy - 6.0f * nivelVoz, d + 12.0f * nivelVoz, d + 12.0f * nivelVoz },
              0.5f, ar, ag, ab, a);
    } else {
      if (k > 0.01f)
        gfx_rect((GfxRect){ cx - 14, cy - 14, d + 28, d + 28 }, 0, GFX_SOMBRA, 1.0f, 0, 0, 0.5f,
                 ar, ag, ab, 0.30f * k * a);
      gfx_cor((GfxRect){ cx, cy, d, d }, 0.5f, anim_mistura(0.2f, ar, k), anim_mistura(0.205f, ag, k),
              anim_mistura(0.225f, ab, k), a);
    }
    { int t = (ouve || k > 0.5f) ? ajustes_tinta_foco() : 224;
      gfx_icone((GfxRect){ cx + 17, cy + 17, d - 34, d - 34 }, "aj_mic",
                t / 255.0f, t / 255.0f, t / 255.0f, a); }
  }
  // CELULAR: o ultimo da barra, sempre na mesma ponta.
  if (celDisponivel()) {
    float d = SP_MIC_D;
    celb_botao(CELB_SPOT, (GfxRect){ barra.x + barra.w - 26.0f - d, barra.y + (barra.h - d) * 0.5f, d, d },
               painel == P_CEL, focarCampo, 2, 0, a);
  }
}

static void desenhaTeclado(float dy, float a) {
  int f, c;
  float ar, ag, ab, ka = anim_suave(kbAnim), sx = (1.0f - ka) * -18.0f;
  if (kbAnim < 0.01f) return;
  a *= ka;
  ajustes_acento(&ar, &ag, &ab);
  for (f = 0; f <= kbFil; f++)
    for (c = 0; c < kbColunas(f); c++) {
      float k = animTecla[f][c], esc = 1.0f + 0.08f * k;
      GfxRect b = teclaRect(f, c), t;
      const char *s = "", *ic = NULL;
      int tom;
      b.y += dy; b.x += sx;
      t = (GfxRect){ b.x - b.w * (esc - 1) * 0.5f, b.y - b.h * (esc - 1) * 0.5f, b.w * esc, b.h * esc };
      gfx_cor(t, 0.16f, 0.13f, 0.138f, 0.158f, 0.95f * a);
      if (k > 0.01f) {
        gfx_rect((GfxRect){ t.x - 12, t.y - 12, t.w + 24, t.h + 24 }, 0, GFX_SOMBRA, 1.0f, 0, 0, 0.5f,
                 ar, ag, ab, 0.28f * k * a);
        gfx_cor(t, 0.16f, ar, ag, ab, k * a);
      }
      if (ponteiro_ativo() && kbAberto) ponteiro_alvo(b.x, b.y, b.w, b.h, focarTecla, NULL, f, c);
      if (f < kbFil) s = kbTeclas[f * SP_KB_COLS + c];
      else switch (kbCmd[c]) {
        case K_ESPACO:  s = i18n("espaço"); break;
        case K_APAGAR:  s = i18n("apagar"); break;
        case K_LIMPAR:  s = i18n("limpar"); break;
        case K_FALAR:   ic = "aj_mic"; break;
        case K_TECLADO: ic = "aj_keyboard"; break;
      }
      tom = (int)anim_mistura(226.0f, (float)ajustes_tinta_foco(), k);
      if (ic) gfx_icone((GfxRect){ t.x + (t.w - 30) * 0.5f, t.y + (t.h - 30) * 0.5f, 30, 30 }, ic,
                        tom / 255.0f, tom / 255.0f, tom / 255.0f, a);
      else {
        TxtLinha l = txt_linha(f < kbFil ? TXT_PAINEL_ITEM : TXT_CAPTION, s, tom, tom, tom, 255);
        txt_desenhar_alpha(l, t.x + (t.w - l.w) * 0.5f, t.y + (t.h - l.h) * 0.5f, a);
      }
    }
}

// Arte de uma linha no retangulo `r`, com esqueleto enquanto nao chega.
static void arte(GfxRect r, const char *url, float raio, int circulo, float a) {
  GLuint tex = (url && url[0]) ? tex_obter_larg(url, r.w) : 0;
  if (tex) {
    if (circulo) gfx_rect(r, tex, GFX_AVATAR, 0, 0, 0, 0.0f, 0, 0, 0, a);
    else {
      gfx_tex_aspect_atual = tex_aspecto(url);
      gfx_rect(r, tex, GFX_CARD, 0.0f, 0.0f, 0.0f, raio, 0, 0, 0, a);
      gfx_tex_aspect_atual = 0.0f;
    }
  } else if (url && url[0] && !tex_falhou(url))
    gfx_esqueleto(r, circulo ? 0.5f : raio, NV_COR_ESQUELETO_R, NV_COR_ESQUELETO_G, NV_COR_ESQUELETO_B, a);
  else gfx_cor(r, circulo ? 0.5f : raio, NV_COR_ESQUELETO_R, NV_COR_ESQUELETO_G, NV_COR_ESQUELETO_B, a);
}

static void desenhaLinha(int i, float x, float y, float a) {
  Linha *l = &lin[i];
  float f = animLin[i], w = listaW, ar, ag, ab;
  int vidro = ajustes_vidro();
  int t1 = 246, t2 = 168;
  GfxRect r = { x, y, w, l->h - 8.0f };
  ajustes_acento(&ar, &ag, &ab);
  if (l->tipo == L_CAB) {
    TxtLinha t = txt_linha(TXT_CW_BADGE, l->t1, 150, 156, 168, 255);
    txt_desenhar_alpha(t, x + 18.0f, y + l->h - t.h - 10.0f, a);
    return;
  }
  if (l->tipo == L_AVISO) {
    TxtLinha t = txt_linha_corta(TXT_PAINEL_ITEM, l->t1, 220, 222, 228, 255, w - 36.0f);
    txt_desenhar_alpha(t, x + 18.0f, y + 16.0f, a);
    if (l->t2[0]) {
      TxtLinha s = txt_linha_corta(TXT_CAPTION, l->t2, 160, 164, 175, 255, w - 36.0f);
      txt_desenhar_alpha(s, x + 18.0f, y + 16.0f + t.h + 8.0f, a);
    }
    return;
  }
  if (ponteiro_ativo()) ponteiro_alvo(r.x, r.y, r.w, r.h, focarLinha, NULL, i, 0);
  // Foco: a linha inteira acende na cor do tema (o realce do Spotlight); no
  // vidro, a superficie clareia e ganha o contorno branco.
  if (f > 0.01f) {
    float rr = 20.0f / r.h;
    if (vidro) { gfx_vidro_painel(r, rr, 0.55f, f * a); gfx_vidro_foco(r, rr, f, a); }
    else {
      gfx_rect((GfxRect){ r.x - 14, r.y - 14, r.w + 28, r.h + 28 }, 0, GFX_SOMBRA, 1.0f, 0, 0, 0.5f,
               ar, ag, ab, 0.22f * f * a);
      gfx_cor(r, rr, ar, ag, ab, f * a);
      t1 = (int)anim_mistura(246.0f, (float)ajustes_tinta_foco(), f);
      t2 = (int)anim_mistura(168.0f, (float)ajustes_tinta_foco(), f * 0.85f);
    }
  }
  if (l->tipo == L_TOPO) {
    const CatItem *ci = cat_item(l->ref);
    GfxRect art = { r.x + 14.0f, r.y + 14.0f, 0, r.h - 28.0f };
    float tx;
    art.w = art.h * 16.0f / 9.0f;
    // Sem paisagem, o cartaz em pe ocupa o mesmo lugar (e a caixa encolhe).
    if (!(ci && ci->backdrop[0])) art.w = art.h * 2.0f / 3.0f;
    arte(art, l->arte, 16.0f / art.h, 0, a);
    tx = art.x + art.w + 28.0f;
    { TxtLinha t = txt_linha_corta(TXT_ROW_TITULO, l->t1, t1, t1, t1, 255, r.x + r.w - tx - 24.0f);
      TxtLinha m = txt_linha_corta(TXT_CAPTION, l->t2, t2, t2, t2, 255, r.x + r.w - tx - 24.0f);
      float ty = r.y + 34.0f;
      txt_desenhar_alpha(t, tx, ty, a);
      txt_desenhar_alpha(m, tx, ty + t.h + 10.0f, a);
      if (ci && ci->genero[0]) {
        TxtLinha g = txt_linha_corta(TXT_CAPTION2, ci->genero, t2, t2, t2, 255, r.x + r.w - tx - 24.0f);
        txt_desenhar_alpha(g, tx, ty + t.h + 10.0f + m.h + 8.0f, 0.85f * a);
      }
      if (f > 0.5f) {
        TxtLinha o = txt_linha(TXT_CAPTION2, i18n("OK   Abrir"), t2, t2, t2, 255);
        txt_desenhar_alpha(o, tx, r.y + r.h - o.h - 22.0f, (f - 0.5f) * 2.0f * a);
      } }
    return;
  }
  { GfxRect ic = { r.x + 14.0f, r.y + 8.0f, 0, r.h - 16.0f };
    float tx;
    switch (l->tipo) {
      case L_TITULO:  ic.w = ic.h * 2.0f / 3.0f; arte(ic, l->arte, 8.0f / ic.h, 0, a); break;
      case L_PESSOA:
        ic.w = ic.h;
        if (l->arte[0]) arte(ic, l->arte, 0.5f, 1, a);
        else {
          // Sem foto no TMDB: o disco com o icone de pessoa, nao um buraco.
          gfx_cor(ic, 0.5f, 0.20f, 0.205f, 0.225f, a);
          gfx_icone((GfxRect){ ic.x + ic.w * 0.25f, ic.y + ic.h * 0.25f, ic.w * 0.5f, ic.h * 0.5f },
                    "aj_user-round", 0.78f, 0.79f, 0.82f, a);
        }
        break;
      case L_COLECAO: ic.w = ic.h * 16.0f / 9.0f; arte(ic, l->arte, 10.0f / ic.h, 0, a); break;
      case L_CANAL:
        ic.w = ic.h * 16.0f / 9.0f;
        gfx_cor(ic, 10.0f / ic.h, 0.16f, 0.165f, 0.185f, a);
        guia_logo_desenhar(l->arte, l->t1, ic, ic.w - 16.0f, ic.h - 16.0f, 0.965f, a);
        break;
      default: {
        // Icone num disco: recente, limpar, catalogo, addon.
        float d = 46.0f;
        GfxRect dc = { r.x + 18.0f, r.y + (r.h - d) * 0.5f, d, d };
        int tt = (f > 0.5f && !vidro) ? ajustes_tinta_foco() : 210;
        gfx_cor(dc, 0.5f, 1.0f, 1.0f, 1.0f, (0.08f + 0.06f * (1.0f - f)) * a);
        gfx_icone((GfxRect){ dc.x + 11, dc.y + 11, d - 22, d - 22 },
                  l->icone[0] ? l->icone : "aj_rotate-ccw-clock",
                  tt / 255.0f, tt / 255.0f, tt / 255.0f, l->icone[0] ? a : 0.6f * a);
        ic.w = d + 4.0f;
        break; }
    }
    tx = ic.x + ic.w + 24.0f;
    if (l->tipo == L_RECENTE || l->tipo == L_LIMPAR) {
      TxtLinha t = txt_linha_corta(l->tipo == L_LIMPAR ? TXT_CAPTION : TXT_PAINEL_ITEM, l->t1,
                                   t1, t1, t1, 255, r.x + r.w - tx - 24.0f);
      txt_desenhar_alpha(t, tx, r.y + (r.h - t.h) * 0.5f, l->tipo == L_LIMPAR ? 0.8f * a : a);
      // A barra da pressao longa: solte antes de encher e nao apaga.
      if (painel == P_LISTA && i == focoL && okPress && !okLongo) {
        float p = anim_clamp((SDL_GetTicks() - okDesde) / (float)NV_HOLD_MS, 0.0f, 1.0f);
        if (p > 0.02f) gfx_cor((GfxRect){ r.x + 18.0f, r.y + r.h - 8.0f, (r.w - 36.0f) * p, 4.0f },
                               0.5f, t1 / 255.0f, t1 / 255.0f, t1 / 255.0f, 0.9f * a);
      }
      return;
    }
    { TxtLinha t = txt_linha_corta(TXT_PAINEL_ITEM, l->t1, t1, t1, t1, 255, r.x + r.w - tx - 24.0f);
      TxtLinha m = txt_linha_corta(TXT_CAPTION, l->t2, t2, t2, t2, 255, r.x + r.w - tx - 24.0f);
      float bloco = t.h + 6.0f + (l->t2[0] ? m.h : 0);
      float ty = r.y + (r.h - bloco) * 0.5f;
      txt_desenhar_alpha(t, tx, ty, a);
      if (l->t2[0]) txt_desenhar_alpha(m, tx, ty + t.h + 6.0f, a); }
  }
}

static void desenhaLista(float dy, float a) {
  int i;
  float topo = SP_CORPO_Y + SP_CPAD_T + dy;
  float vis = corpoH - SP_CPAD_T - SP_CPAD_B - SP_RODAPE_H;
  if (vis <= 2.0f) return;
  gfx_recorte(listaX - 30.0f, topo - 8.0f, listaW + 60.0f, vis + 12.0f);
  for (i = 0; i < nLin; i++) {
    float y = topo + lin[i].y - scrollY;
    float e = entraLin[i];
    if (y > topo + vis + 20.0f || y + lin[i].h < topo - 20.0f) continue;
    // Linha nova sobe 10 px e acende; a que ja estava fica parada.
    desenhaLinha(i, listaX, y + (1.0f - e) * 10.0f, a * e);
  }
  gfx_sem_recorte();
}

// Dicas do controle em pares tecla + acao, cada uma desenhada separada: a
// frase inteira numa linha so perdia os espacos entre os pares.
static float dica(float x, float y, const char *tecla, const char *acao, float a) {
  TxtLinha t = txt_linha(TXT_CAPTION2, tecla, 222, 224, 230, 255);
  TxtLinha r = txt_linha(TXT_CAPTION2, acao, 150, 154, 163, 255);
  txt_desenhar_alpha(t, x, y, a);
  txt_desenhar_alpha(r, x + t.w + 12.0f, y, a);
  return x + t.w + 12.0f + r.w + 44.0f;
}

static void desenhaRodape(float dy, float a) {
  float x = SP_BX + 40.0f, y = SP_CORPO_Y + corpoH - SP_RODAPE_H + 14.0f + dy;
  int recente = painel == P_LISTA && focoL >= 0 && focoL < nLin && lin[focoL].tipo == L_RECENTE;
  a *= anim_clamp((corpoH - 80.0f) / 80.0f, 0.0f, 1.0f);
  if (a < 0.01f) return;
  if (painel == P_TECLADO) {
    x = dica(x, y, "OK", i18n("Digitar"), a);
    x = dica(x, y, "\xe2\x86\x92", i18n("Resultados"), a);
    dica(x, y, i18n("Voltar"), i18n("Campo"), a);
  } else if (painel == P_LISTA) {
    x = dica(x, y, "OK", i18n(recente ? "Buscar de novo" : "Abrir"), a);
    if (recente) x = dica(x, y, i18n("Segure OK"), i18n("Remover"), a);
    dica(x, y, i18n("Voltar"), i18n("Campo"), a);
  } else {
    x = dica(x, y, "OK", i18n(painel == P_MIC ? "Falar" : painel == P_CEL ? "Digitar pelo celular" : "Digitar"), a);
    x = dica(x, y, "\xe2\x86\x93", i18n("Resultados"), a);
    dica(x, y, i18n("Voltar"), i18n(nConsulta > 0 ? "Limpar" : "Fechar"), a);
  }
}

void spot_desenhar(Uint32 agora, int veuPronto) {
  float a, dy;
  GfxRect sup;
  if (entrada < 0.004f) return;
  a = anim_suave(entrada);
  dy = (1.0f - a) * -24.0f;
  ponteiro_camada();
  if (!veuPronto) {
    float ga = gfx_opacidade_grupo;
    gfx_opacidade_grupo = ga * a;
    spot_veu();
    gfx_opacidade_grupo = ga;
  }
  { float ka = anim_suave(kbAnim);
    listaX = anim_mistura(SP_LISTA_X0, SP_LISTA_X1, ka);
    listaW = SP_LISTA_XF - listaX; }
  sup = (GfxRect){ SP_BX, SP_BY + dy, SP_BW, SP_BH + (corpoH > 0.0f ? corpoH : 0.0f) };
  // A SUPERFICIE TODA e um anteparo para o ponteiro: clique fora das teclas e
  // das linhas nao vaza para a tela de tras.
  if (ponteiro_ativo()) ponteiro_alvo(sup.x, sup.y, sup.w, sup.h, NULL, NULL, 0, 0);
  desenhaSuperficie(sup, a);
  desenhaCampo(dy, a, agora);
  if (corpoH > 2.0f) {
    gfx_recorte(SP_BX, SP_CORPO_Y + dy, SP_BW, corpoH);
    desenhaTeclado(dy, a);
    gfx_sem_recorte();
    desenhaLista(dy, a);
    desenhaRodape(dy, a);
  }
}
