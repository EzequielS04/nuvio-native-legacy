#include "idioma.h"
#include "idiomacod.h"
#include "ajustes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------- tabela
//
// ORDENADA POR CHAVE, e a ordem e conferida em tempo de execucao no primeiro
// uso (ver conferirOrdem). Uma tabela desordenada faria a busca binaria falhar
// em SILENCIO: alguns textos traduziriam, outros nao, e o padrao pareceria
// aleatorio. Nao ha teste que pegue isso melhor que a propria busca.
typedef struct { const char *pt, *en; } Par;

static const Par TAB[] = {
#include "idioma_tab.h"
};
#define TAB_N ((int)(sizeof TAB / sizeof *TAB))

// OS OUTROS IDIOMAS moram em tabelas PARALELAS, uma por idioma, com uma entrada
// por linha de idioma_tab.h e na MESMA ordem. A busca binaria roda UMA vez, na
// chave portuguesa, e devolve o indice; o indice serve a qualquer idioma. Por
// isso o custo por linha desenhada e o de antes de haver 5 idiomas, e o cache
// abaixo nao precisa saber qual idioma esta ligado (trocar de idioma nao o
// invalida). Nos arquivos idioma_XX.h cada linha e T("chave pt", "traducao"):
// a macro joga a chave fora — ela so existe para o revisor ler a linha inteira
// e para tools/idiomas.py conferir que o alinhamento nao escorregou.
#define T(chave, traducao) traducao
static const char *const TAB_RO[] = {
#include "idioma_ro.h"
};
static const char *const TAB_UK[] = {
#include "idioma_uk.h"
};
static const char *const TAB_RU[] = {
#include "idioma_ru.h"
};
#undef T
// Uma tabela com o numero errado de linhas desalinharia TODAS as traducoes
// depois dela — falha na compilacao, nao em silencio na tela.
_Static_assert(sizeof TAB_RO / sizeof *TAB_RO == sizeof TAB / sizeof *TAB,
               "idioma_ro.h: uma linha por entrada de idioma_tab.h (tools/idiomas.py --sincronizar)");
_Static_assert(sizeof TAB_UK / sizeof *TAB_UK == sizeof TAB / sizeof *TAB,
               "idioma_uk.h: uma linha por entrada de idioma_tab.h (tools/idiomas.py --sincronizar)");
_Static_assert(sizeof TAB_RU / sizeof *TAB_RU == sizeof TAB / sizeof *TAB,
               "idioma_ru.h: uma linha por entrada de idioma_tab.h (tools/idiomas.py --sincronizar)");

// A traducao da entrada `i` no idioma `lg` (nunca IDIOMA_PT). Valor vazio cai no
// ingles: uma entrada nova ainda sem traducao aparece em ingles, nao em branco.
static const char *traduzida(int i, int lg) {
  const char *r = NULL;
  switch (lg) {
    case IDIOMA_RO: r = TAB_RO[i]; break;
    case IDIOMA_UK: r = TAB_UK[i]; break;
    case IDIOMA_RU: r = TAB_RU[i]; break;
    default: break;
  }
  return r && *r ? r : TAB[i].en;
}

// ---------------------------------------------------------------- registro
//
// Levantamento das strings vivas. Guarda o que ja viu para nao reescrever a
// mesma linha a cada quadro — sao ~200 linhas por quadro a 60fps.
#define REG_MAX 1024
static char *vistos[REG_MAX];
static int   nVistos;
static FILE *arqReg;
static int   regTentado;

void idioma_registrar(const char *s) {
  int i;
  if (!s || !*s) return;
  if (!regTentado) {
    const char *caminho = getenv("NUVIO_TEXTO_DUMP");
    regTentado = 1;
    if (caminho && *caminho) arqReg = fopen(caminho, "w");
  }
  if (!arqReg || nVistos >= REG_MAX) return;
  for (i = 0; i < nVistos; i++) if (!strcmp(vistos[i], s)) return;
  vistos[nVistos] = strdup(s);
  if (!vistos[nVistos]) return;
  nVistos++;
  fprintf(arqReg, "%s\n", s);
  fflush(arqReg);
}

// ---------------------------------------------------------------- traducao

const char *idioma_mes_data(int mes, const char *nomePt) {
  static const char *UK[12] = {
    "січня", "лютого", "березня", "квітня", "травня", "червня",
    "липня", "серпня", "вересня", "жовтня", "листопада", "грудня"
  };
  static const char *RU[12] = {
    "января", "февраля", "марта", "апреля", "мая", "июня",
    "июля", "августа", "сентября", "октября", "ноября", "декабря"
  };
  int lg = ajustes_idioma();
  if (mes >= 1 && mes <= 12) {
    if (lg == IDIOMA_UK) return UK[mes - 1];
    if (lg == IDIOMA_RU) return RU[mes - 1];
  }
  return i18n(nomePt);
}

static int ordemOk = -1;
static void conferirOrdem(void) {
  int i;
  ordemOk = 1;
  for (i = 1; i < TAB_N; i++) {
    if (strcmp(TAB[i - 1].pt, TAB[i].pt) >= 0) {
      // Falar alto uma vez. Uma tabela fora de ordem nao quebra o app, ela
      // traduz PELA METADE — o pior defeito possivel, porque parece escolha.
      printf("[idioma] TABELA FORA DE ORDEM em %d: \"%s\" antes de \"%s\"\n",
             i, TAB[i - 1].pt, TAB[i].pt);
      fflush(stdout);
      ordemOk = 0;
      return;
    }
  }
}

// CACHE DA BUSCA. text.c chama i18n em TODA linha desenhada, todo quadro;
// com o ingles ligado cada chamada era uma busca binaria de ~11 strcmp em
// 1500 entradas. Medido no Mac (perfil CDP, 35 s de home): 268 ms dentro de
// i18n, a segunda funcao mais cara do fio principal depois de main — e na
// Samsung o mesmo trabalho custa varias vezes mais. Tabela direta de 1024
// posicoes chaveada pelo hash FNV-1a do texto: acerto = 1 passada pelo texto
// + 1 strcmp (positivo) ou 0 strcmp (negativo, confiado pelo hash de 64 bits
// mais o tamanho). Titulo de filme e fragmento de sinopse tambem entram — sao
// justamente os negativos que antes pagavam a busca inteira.
#define I18N_CACHE 1024
static struct { unsigned long long h; unsigned n; int idx; } cache[I18N_CACHE];

const char *i18n(const char *s) {
  int lo = 0, hi = TAB_N - 1;
  unsigned long long h = 1469598103934665603ull;
  unsigned n = 0, slot;
  const unsigned char *p;
  int lg;
  idioma_registrar(s);
  if (!s || !*s) return s;
  lg = ajustes_idioma();
  if (lg == IDIOMA_PT) return s;
  if (ordemOk < 0) conferirOrdem();
  if (!ordemOk) return s;
  for (p = (const unsigned char *)s; *p; p++, n++) { h ^= *p; h *= 1099511628211ull; }
  slot = (unsigned)(h % I18N_CACHE);
  if (cache[slot].h == h && cache[slot].n == n && (cache[slot].idx < 0 || !strcmp(s, TAB[cache[slot].idx].pt)))
    return cache[slot].idx < 0 ? s : traduzida(cache[slot].idx, lg);
  while (lo <= hi) {
    int m = (lo + hi) / 2;
    int c = strcmp(s, TAB[m].pt);
    if (c == 0) { cache[slot].h = h; cache[slot].n = n; cache[slot].idx = m; return traduzida(m, lg); }
    if (c < 0) hi = m - 1; else lo = m + 1;
  }
  cache[slot].h = h; cache[slot].n = n; cache[slot].idx = -1;
  // NAO ADIANTA RECLAMAR AQUI, e eu tentei: text.c chama i18n em cada linha
  // DESENHADA, entao esta funcao ve tambem titulo de filme, sinopse e cada
  // fragmento de quebra de linha. Uma medicao de 25 s no Mac produziu 96
  // avisos — sinopse do Fallout palavra a palavra, letras soltas do relogio —
  // e encheu o teto antes de qualquer texto de interface aparecer. O sinal
  // real ficaria enterrado no log de quem relata. Separar interface de
  // conteudo aqui exigiria a mesma heuristica de portugues que ja falhou tres
  // vezes na varredura estatica, e ela erra igual em "Detalhes" e "Cartazes".
  return s;
}
