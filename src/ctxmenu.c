#include "ctxmenu.h"
#include "catalogo.h"
#include "logotitulo.h"
#include "descoberta.h"
#include "syncprog.h"
#include "visto.h"
#include "trakt.h"
#include "simkl.h"
#include "extras.h"
#include "gfx.h"
#include "text.h"
#include "tex_cache.h"
#include "layout.h"
#include "anim.h"
#include "ajustes.h"
#include "progresso.h"
#include "salvos.h"
#include "recomenda.h"
#include "recenviar.h"
#include "botoes.h"
#include "badges.h"
#include "idioma.h"
#include "fileiras.h"
#include "home.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ponteiro.h"

// Mantem o header publico de Trakt estavel: estas leituras sao o contrato
// interno entre a modal e as escritas assincronas do proprio port.
extern int trakt_operacao_estado(int tipo);
extern int trakt_watchlist_tipo(const char *imdb, const char *tipo, int adicionar);
extern int trakt_assistido_tipo(const char *imdb, const char *tipo, int marcar);
extern int cat_historico_estado_item(int indice);
extern int cat_historico_estado_id(const char *imdb, const char *tipo);
extern void cat_historico_definir_id(const char *imdb, const char *tipo, int visto);

enum { CTX_OP_NENHUMA, CTX_OP_LISTA = 1, CTX_OP_HISTORICO = 2 };
enum { CTX_PENDENTE = 1, CTX_CONFIRMADA = 2, CTX_FALHA = 3 };

// MEDIDO no bundle 1.0.4: o dialogo tem 37,5vw de largura (720 px em 1920).
#define CTX_W      720.0f
#define CTX_PAD     44.0f
#define CTX_LINHA   BOTAO_H_PRIMARIO // mesma altura do botao primario do detalhe
#define CTX_GAP     BOTAO_GAP         // o mesmo ritmo entre acoes do app
// O NOME VIROU A CAIXA DO LOGO (logotitulo.h): 76 px de altura, ~2 linhas do
// TXT_HEADLINE, reservados com ou sem logo para o cartao nao pular quando o
// arquivo chega. O cabecalho cresceu 38 px (era 158) por isso.
#define CTX_LOGO_Y  32.0f
#define CTX_LOGO_H  76.0f
#define CTX_LOGO_W 420.0f
#define CTX_CAB    196.0f     // titulo, estados e rotulo do grupo
#define CTX_RODAPE  70.0f

// SALVO E UM FATO DO TITULO, NAO DO CARTAO. Segurar OK num cartao do
// "Trending" de um titulo que esta nos Salvos oferecia "Salvar", porque aquela
// copia do CatItem nao tem a marca — so a da watchlist tem. Pergunta a todas as
// fontes que sabem: a copia do cartao, a lista local, qualquer outra copia do
// catalogo (watchlist do Trakt) e o Plan to Watch do Simkl.
static int tituloSalvo(const CatItem *ci) {
  if (!ci) return 0;
  if (ci->naLista) return 1;
  if (!ci->imdb[0]) return 0;
  return salvos_tem(ci->imdb) || cat_imdb_na_lista(ci->imdb) ||
         (simkl_ativo() && simkl_na_plantowatch(ci->imdb));
}

static int   aberto, idx = -1, foco, pedDetalhes = -1;
static float anim;
static int   operacao, intencao, estadoOperacao;
static int   espelhoAplicado;
// A ESCRITA EM ANDAMENTO E DO SIMKL, e nao do Trakt (issue #110): diz a
// ctx_atualizar qual estado consultar. Os numeros dos dois estados sao os
// mesmos (SMK_OP_* = CTX_*), entao o resto do jogo de estados nao muda.
static int   opSimkl;
// Frase no lugar de "Biblioteca atualizada" quando o destino e o Simkl e nao
// ha vinculo: a lista desta TV foi escrita, o Simkl nao — e a tela diz.
static const char *avisoOp;
static char  operacaoImdb[16];
static volatile int holdAtivo, holdCancelado, holdPronto;
// O OK QUE ABRIU O MODAL AINDA ESTA AFUNDADO.
//
// O menu do cartaz abre NO LIMIAR, com o dedo ainda no botao (home.c dispara
// em home_atualizar, nao no KEYUP) — e isso e de proposito: esperar a soltura
// faria a barra encher na tela sem nada acontecer. O preco e que a repeticao
// automatica do controle continua mandando KEYDOWN de OK, e o modal recem-
// aberto os tratava como escolha: "quando abre o modal e eu ainda estou
// segurando, ele ja clica sozinho".
//
// Enquanto esta marca vale, OK nao escolhe nada aqui. Ela cai no primeiro
// KEYUP de OK — ou seja, exige um toque NOVO, que e o que o dono espera.
static volatile int esperandoSoltura;
static Uint32 holdDesde;

// MODO PAINEL: o mesmo menu, aberto SEGURANDO OK numa linha do painel de
// Salvos (salvospainel.c, dono 25/09/2026: "segurar e remover, ou more
// infos").
//
// POR QUE UMA COPIA E NAO O INDICE. O painel mostra titulos que NAO estao no
// catalogo — a lista local se desenha do proprio arquivo, antes de a descoberta
// responder (ver salvospainel.c) — e para esses nao existe indice nenhum. A
// copia e o titulo; o catalogo, quando tem o mesmo IMDb, continua sendo quem
// responde (itemAtual), porque e ele que tem progresso, temporadas e a marca.
//
// O RESTO E O MESMO MENU: mesmo cartao, mesmas pilulas, mesma espera pelo 2xx
// do Trakt e o mesmo caminho de escrita de OP_LISTA (salvos_definir + Trakt ou
// Simkl + espelho). Um "Remover" proprio do painel seria a quinta porta
// escrevendo a mesma marca, e o defeito dos quatro "+" do app irmao (salvos.h)
// comecou exatamente assim.
static int     doPainel;
static CatItem copiaPainel;
// Removeu pelo painel: o menu sai sozinho quando a remocao CONFIRMA — a linha
// ja nao existe mais atras dele, e o foco do painel foi para a seguinte.
static int     fecharAoConfirmar;
static char    pedDetalhesImdb[24];
static float   dicaCx = -1.0f;   // centro da barra de "Segure OK"; <0 = tela

static int teclaOk(SDL_Keycode k) {
  return k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE;
}

// A home e quem conhece o item focado, por isso ela continua decidindo qual
// indice entregar a ctx_abrir no KEYUP. Este observador fornece o feedback
// durante a retenção e arma a janela longa; setas/Voltar invalidam o gesto
// antes que a home possa transformá-lo em ação.
static int observarHold(void *u, SDL_Event *e) {
  (void)u;
  if (e->type == SDL_KEYDOWN) {
    SDL_Keycode k = e->key.keysym.sym;
    if (teclaOk(k) && !e->key.repeat) {
      holdAtivo = 1;
      holdCancelado = 0;
      holdPronto = 0;
      holdDesde = SDL_GetTicks();
    } else if (holdAtivo &&
               (k == SDLK_UP || k == SDLK_DOWN || k == SDLK_LEFT ||
                k == SDLK_RIGHT || k == SDLK_AC_BACK || k == SDLK_ESCAPE ||
                k == SDLK_BACKSPACE || e->key.keysym.scancode == NV_SCANCODE_BACK)) {
      holdCancelado = 1;
    }
  } else if (e->type == SDL_KEYUP && teclaOk(e->key.keysym.sym)) {
    if (holdAtivo && !holdCancelado && SDL_GetTicks() - holdDesde >= NV_HOLD_MS)
      holdPronto = 1;
    holdAtivo = 0;
    esperandoSoltura = 0;
  }
  return 0;
}

// QUATRO: detalhes, salvar, assistido (so em filme/serie) e tirar de
// Continuar assistindo (so em item com progresso).
//
// ERA TRES, E O QUARTO EXISTIA MESMO ASSIM. Em filme com progresso as quatro
// condicoes valem ao mesmo tempo e montar() escrevia em ops[3] — fora do
// vetor. O sintoma que chegou (issue #36) foi o mais brando dos possiveis:
// focoAnim so era animado ate CTX_MAX, entao a ultima linha do menu NUNCA
// acendia ("doesn't go white to show it is selected"). O desenho ia ate nOps e
// lia focoAnim[3], que ninguem escrevia.
//
// A opcao de Continuar assistindo entrou depois das outras tres, e o teto
// ficou onde estava. Por isso o append agora passa por juntar(), que confere o
// teto num lugar so: uma quinta opcao deixa de aparecer, em vez de corromper
// memoria.
// CINCO desde que "Recomendar a um amigo" entrou. O teto tem de subir JUNTO
// com a opcao nova: a quarta opcao existiu antes de CTX_MAX virar 4 e o
// sintoma foi escrita fora do vetor (issue #36). juntar() confere o teto num
// lugar so, entao uma sexta opcao deixa de aparecer em vez de corromper
// memoria — mas "deixa de aparecer" tambem e defeito, e por isso o numero sobe
// aqui e nao em silencio.
// SETE desde "Estilo da fileira": seis no cartaz (as cinco + a entrada do
// estilo) e, na pagina de estilos, ate cinco formas. O contrato conta as linhas
// de juntar(), e a pagina de estilos e a sexta e setima delas.
// SEIS desde o modal de estilo: as formas sairam de ops[]. O laco que as
// juntava contava UMA linha para o contrato e escrevia sete — e o teto de sete
// era exatamente o que deixava o Destaque 4:3 e a faixa com titulo de fora do
// menu. Agora elas moram em estLin[FIL_TIPO_N], do tamanho do enum.
#define CTX_MAX 6
static struct { const char *rot; int acao; } ops[CTX_MAX];
static int nOps;
static float focoAnim[CTX_MAX];
static int holdObservador;
enum { OP_DETALHES, OP_LISTA, OP_ASSISTIDO, OP_TIRAR_CONTINUAR, OP_RECOMENDAR,
       OP_ESTILO, OP_CATEGORIA };
// "Mover para categoria" pedido no modo painel: o IMDb do titulo, consumido
// uma vez pelo painel (ctx_pediu_categoria), que abre a escolha dele.
static char pedCategoriaImdb[24];

// --- ESTILO DA FILEIRA -------------------------------------------------------
//
// A home diz de que FILEIRA e o cartao (ctx_fileira) antes de abrir o menu, e
// o menu oferece a forma dela. Quem grava e fileiras.c, no mesmo campo da tela
// Ajustes > Fileiras da Home: as duas telas mostram sempre a mesma escolha.
//
// Duas paginas: 0 e o menu do titulo (com a entrada "Estilo da fileira"), 1 e
// a lista de formas. Um grupo de colecao nao tem titulo por tras — o cartao e
// uma pasta — e abre direto na pagina 1 (soFileira).
static char filChave[192], filTitulo[96];
static int  pagina, soFileira;
// A PAGINA DE ESTILOS E UM MODAL PROPRIO (dono, 01/10/2026: "tem todos os
// estilos? ... coloque ao lado o preview de como seria os cards"): a lista de
// formas a esquerda, com a atual marcada, e a direita a fileira desenhada na
// forma em foco com as artes dela (home_previa_fileira). As formas nao passam
// por ops[]: o vetor e do tamanho do enum, entao toda forma que fil_estilos
// devolve aparece.
//
// UMA LINHA POR FORMA (dono, 01/10): as que so diferem em tamanho — as tres
// paisagens, os tres 4:3 — dividem a linha, e o tamanho e um segmentado P M G
// DENTRO dela, que esquerda/direita trocam. Cima/baixo continuam sendo "outra
// forma"; a previa mostra sempre o tamanho em foco. Sub-lista foi descartada:
// um segundo nivel com Voltar proprio no D-pad esconde os tamanhos atras de um
// OK, e o segmentado os mostra o tempo todo.
static FilEstiloLinha estLin[FIL_TIPO_N];
static int   nEstilos, estFoco, estTam[FIL_TIPO_N];
static float estAnim[FIL_TIPO_N];
// A previa troca com a mola: a forma de antes sai enquanto a nova entra.
// `prevRef*` e a forma que decide a reducao de cada uma (home_previa_fileira).
static int   prevAtual = -1, prevAnt = -1, prevRefAtual = -1, prevRefAnt = -1;
static float prevT = 1.0f;

// --- RECOMENDAR: O FLUXO NAO MORA MAIS AQUI ---------------------------------
//
// "Para quem" e "o que dizer" foram DUAS PAGINAS DENTRO DESTE MODAL ate a tela
// de detalhe pedir o mesmo fluxo pelo botao circular. Duas copias das mesmas
// listas divergiriam na primeira mudanca — e a primeira mudanca ja estava
// pedida: o pareamento por codigo e o estado vazio honesto. As telas viraram
// recenviar.c, e este arquivo faz o que a tela de detalhe tambem faz: abre a
// modal compartilhada e sai da frente.

// Os DELETE remotos de "Tirar de Continuar assistindo", fora do fio de
// desenho. Nenhum dos dois e obrigatorio: sem Trakt nao ha id de playback, sem
// conta nao ha RPC. Os dois dizem no log o que fizeram.
typedef struct { char imdb[64]; char chave[192]; } TirarRemoto;
static void *fioTirarRemoto(void *u) {
  TirarRemoto *tr = (TirarRemoto *)u;
  trakt_playback_remover(tr->imdb);
  syncprog_remover(tr->chave);
  free(tr);
  return NULL;
}

static int indiceAtual(void) {
  int n = cat_n();
  int achado;
  // No modo painel o indice e so "o catalogo tem este titulo?" — pode nao ter,
  // e ai quem responde e a copia (itemAtual).
  if (doPainel) return copiaPainel.imdb[0] ? cat_indice_por_imdb(copiaPainel.imdb) : -1;
  if (n < 1 || idx < 0 || idx >= n) return -1;
  if (operacaoImdb[0]) {
    achado = cat_indice_por_imdb(operacaoImdb);
    // A resposta pode chegar depois de a descoberta trocar o bloco. Nunca
    // reutilizar `idx` nesse caso, pois ele pode ser outro titulo.
    return achado;
  }
  return idx;
}

// O titulo do menu: a copia do catalogo quando ela existe, senao (so no modo
// painel) a copia que o painel entregou. NULL quando nao ha mais titulo.
static const CatItem *itemAtual(void) {
  int i = indiceAtual();
  const CatItem *ci = i >= 0 ? cat_item(i) : NULL;
  if (!ci && doPainel && copiaPainel.imdb[0]) ci = &copiaPainel;
  return ci;
}

// -1 nao consultado, 0 nao visto, 1 visto. Por id, para valer tambem na copia.
static int historicoDe(const CatItem *ci) {
  return ci ? cat_historico_estado_id(ci->imdb, ci->tipo) : -1;
}

// O UNICO CAMINHO PARA DENTRO DE ops[]. Ver a nota em CTX_MAX.
static void juntar(const char *rot, int acao) {
  if (nOps >= CTX_MAX) return;
  ops[nOps].rot = rot; ops[nOps].acao = acao; nOps++;
}

static void montar(void) {
  int i = indiceAtual();
  const CatItem *ci = itemAtual();
  nOps = 0;
  if (pagina == 1) {
    int k, j, atual = fil_tipo(filChave);
    nEstilos = fil_estilo_linhas(filChave, estLin, FIL_TIPO_N);
    // O foco nasce na forma que vale agora, no TAMANHO que vale agora; as
    // outras linhas de tamanho nascem no menor (o de sempre).
    if (estFoco < 0 || estFoco >= nEstilos)
      for (estFoco = 0, k = 0; k < nEstilos; k++) {
        estTam[k] = 0;
        for (j = 0; j < estLin[k].n; j++)
          if (estLin[k].tipos[j] == atual) { estFoco = k; estTam[k] = j; }
      }
    return;
  }
  if (!ci) return;
  // "Mais informações" no painel, que e o nome que o dono deu ao pedir; o
  // efeito e o mesmo "Ver detalhes" do cartaz (a pagina do titulo).
  juntar(doPainel ? "Mais informações" : "Ver detalhes", OP_DETALHES);
  // Sem IMDb nao ha endpoint remoto suportado para esta acao. Nao oferecer
  // um botao que so aparentaria funcionar e inventaria estado local.
  if (ci->imdb[0]) {
    // O MESMO VERBO DO PAINEL E DO BOTAO "+". Estava "Adicionar à biblioteca",
    // e "Biblioteca" e o nome de uma TELA — a pessoa lia o rotulo, ia ate a
    // tela Biblioteca e nao encontrava relacao com o "+" que tinha apertado no
    // detalhe. Agora as tres portas da mesma acao (o "+", esta linha e o painel
    // da tecla AZUL) usam a palavra "salvar", e todas escrevem no mesmo lugar.
    juntar(estadoOperacao == CTX_PENDENTE && operacao == CTX_OP_LISTA
             ? (intencao ? "Salvando..." : "Removendo dos Salvos...")
             : (tituloSalvo(ci) ? "Remover dos Salvos" : "Salvar"),
           OP_LISTA);
  }
  // AS CATEGORIAS DA PESSOA (salvosorg.h) so existem no painel de Salvos: e
  // la que o titulo esta salvo e e la que a escolha abre.
  if (doPainel && ci->imdb[0]) juntar("Mover para categoria", OP_CATEGORIA);
  // O web so oferece "assistido" em filme e serie — nao em canal nem evento,
  // que sao tipos que os addons do dono tambem declaram.
  //
  // NO PAINEL, SERIE SO COM O CATALOGO. A copia da lista local nao tem as
  // temporadas, e desmarcar uma serie no Simkl sem elas apaga a serie da
  // biblioteca de la (ver visto.c/simkl.c). Filme nao tem esse risco.
  if (ci->imdb[0] && (!strcmp(ci->tipo, "movie") || !strcmp(ci->tipo, "series")) &&
      (!doPainel || i >= 0 || !strcmp(ci->tipo, "movie"))) {
    juntar(estadoOperacao == CTX_PENDENTE && operacao == CTX_OP_HISTORICO
             ? (intencao ? "Marcando como assistido..."
                         : "Desmarcando como assistido...")
             : (historicoDe(ci) == 1 ? "Desmarcar como assistido"
                                     : "Marcar como assistido"),
           OP_ASSISTIDO);
  }
  // TIRAR DE "CONTINUAR ASSISTINDO".
  //
  // So aparece em item que TEM progresso — e o unico caso em que a acao quer
  // dizer alguma coisa, e oferecer em todo card poluiria o menu com um botao
  // que nao faz nada. `progresso` e o campo que a home usa para decidir se
  // desenha a barra, entao a condicao aqui e a mesma que poe o item na fileira.
  //
  // Distinta de "marcar como assistido": aquela e historico no Trakt e vale
  // para o titulo; esta apaga a POSICAO DE RETOMADA local, que e o que faz o
  // card aparecer na fileira. Quem terminou um filme quer as duas; quem
  // desistiu no meio quer so esta.
  // O menu do painel e o curto que o dono pediu (remover, mais informacoes,
  // assistido): esta e a de baixo sao acoes do cartaz da home.
  if (!doPainel && ci->progresso > 0 && ci->imdb[0]) {
    juntar("Tirar de Continuar assistindo", OP_TIRAR_CONTINUAR);
  }
  // SO EXISTE SE O PACOTE TEM O SERVICO. Sem NUVIO_REC_URL compilada,
  // recomenda_ativo() e 0 e esta linha nunca aparece — o dono publica builds
  // assim, e um item de menu que so da erro e pior que item nenhum.
  if (!doPainel && recomenda_ativo() && ci->imdb[0] &&
      (!strcmp(ci->tipo, "movie") || !strcmp(ci->tipo, "series"))) {
    juntar("Recomendar a um amigo", OP_RECOMENDAR);
  }
  // Por ULTIMO: e da fileira, nao do titulo. So quando a home disse qual e a
  // fileira e ela tem forma para escolher (Continuar assistindo, Top 10 e o
  // destaque ficam com o visual deles — a home nem passa a chave).
  if (!doPainel && filChave[0] && fil_estilos(filChave, NULL, NULL, FIL_TIPO_N) > 0)
    juntar("Estilo da fileira", OP_ESTILO);
  // O FOCO TEM DE CABER NA LISTA QUE ACABOU DE SER MONTADA.
  //
  // montar() roda de novo a cada confirmacao, e a lista ENCOLHE em casos
  // reais: marcar como assistido apaga a posicao de retomada, e com isso
  // "Tirar de Continuar assistindo" deixa de existir. Se o foco estava nela,
  // `foco` passa a apontar para fora — e ai o desenho nao pinta linha nenhuma
  // ali (o laco vai ate nOps) e aplicar() sai cedo em `foco >= nOps`. Na TV
  // isso e exatamente "ele pula e nao faz nada".
  if (foco >= nOps) foco = nOps > 0 ? nOps - 1 : 0;
  if (foco < 0) foco = 0;
}

static void abrirComum(int indice);
void ctx_abrir(int indice) {
  if (holdCancelado) {
    holdCancelado = 0;
    holdPronto = 0;
    return;
  }
  if (indice < 0 || indice >= cat_n() || !cat_item(indice)) { filChave[0] = 0; return; }
  doPainel = 0;
  soFileira = 0;
  abrirComum(indice);
}

void ctx_fileira(const char *chave, const char *titulo) {
  snprintf(filChave, sizeof filChave, "%s", chave ? chave : "");
  snprintf(filTitulo, sizeof filTitulo, "%s", titulo ? titulo : "");
}

void ctx_abrir_fileira(const char *chave, const char *titulo) {
  if (holdCancelado) {
    holdCancelado = 0;
    holdPronto = 0;
    return;
  }
  ctx_fileira(chave, titulo);
  if (fil_estilos(filChave, NULL, NULL, FIL_TIPO_N) < 1) { filChave[0] = 0; return; }
  doPainel = 0;
  soFileira = 1;
  abrirComum(-1);
}

// O que as duas portas fazem igual. Separado para o modo painel nao ser uma
// segunda copia da inicializacao que diverge na primeira correcao.
static void abrirComum(int indice) {
  // A longa ja consumiu o gesto na home. Limpar a sentinela aqui evita que o
  // KEYUP seguinte seja reaproveitado como uma selecao dentro da modal.
  holdPronto = 0;
  esperandoSoltura = 1;   // o OK que abriu ainda esta afundado; ver a nota acima
  idx = indice; foco = 0; aberto = 1; pedDetalhes = -1;
  pagina = soFileira ? 1 : 0;
  estFoco = -1;               // montar() poe o foco na forma atual
  prevAtual = prevAnt = prevRefAtual = prevRefAnt = -1; prevT = 1.0f;
  memset(estAnim, 0, sizeof estAnim);
  pedDetalhesImdb[0] = 0;
  fecharAoConfirmar = 0;
  operacao = CTX_OP_NENHUMA; intencao = 0; estadoOperacao = 0;
  espelhoAplicado = 0;
  operacaoImdb[0] = 0;
  memset(focoAnim, 0, sizeof focoAnim);
  montar();
}

void ctx_abrir_salvo(const CatItem *titulo) {
  if (holdCancelado) {
    holdCancelado = 0;
    holdPronto = 0;
    return;
  }
  if (!titulo || !titulo->imdb[0]) return;
  filChave[0] = 0;   // o painel de Salvos nao e fileira da home
  soFileira = 0;
  copiaPainel = *titulo;
  // O ID DO TITULO, nunca o do episodio: e o que a lista local, a watchlist e
  // a conta guardam (salvos.h), e o que o Trakt recebe no DELETE.
  salvos_id_titulo(titulo->imdb, copiaPainel.imdb, sizeof copiaPainel.imdb);
  if (!copiaPainel.tipo[0]) snprintf(copiaPainel.tipo, sizeof copiaPainel.tipo, "movie");
  doPainel = 1;
  abrirComum(-1);
}

int ctx_do_painel(void) { return aberto && doPainel; }
void ctx_centro_dica(float cx) { dicaCx = cx; }

const char *ctx_pediu_detalhes_imdb(void) {
  static char s[24];
  if (!pedDetalhesImdb[0]) return NULL;
  snprintf(s, sizeof s, "%s", pedDetalhesImdb);
  pedDetalhesImdb[0] = 0;
  return s;
}

int ctx_aberto(void) { return aberto; }
const char *ctx_pediu_categoria(void) {
  static char s[24];
  if (!pedCategoriaImdb[0]) return NULL;
  snprintf(s, sizeof s, "%s", pedCategoriaImdb);
  pedCategoriaImdb[0] = 0;
  return s;
}
int ctx_pediu_detalhes(void) { int v = pedDetalhes; pedDetalhes = -1; return v; }

// O ESPELHO LOCAL DE "ASSISTIDO", separado de quem confirma: com Trakt ele
// roda depois do 2xx (ctx_atualizar); sem Trakt roda na hora (aplicar), porque
// nao ha resposta nenhuma a esperar.
static void espelharAssistido(int atual, const CatItem *ci, int intencao) {
  cat_historico_definir_id(ci->imdb, ci->tipo, intencao);
  // MARCAR COMO ASSISTIDO APAGA A POSICAO DE RETOMADA.
  //
  // cat_historico_definir_id so escreve numa tabela lateral de
  // historico, e a fileira "Continuar assistindo" nao le dela: ela
  // le progresso/restanteMin/temporada/episodio do proprio item. Sem
  // isto o card continuava ali com a barra cheia depois de o titulo
  // ter sido marcado como visto — o "removo do watch e o card nao
  // sai" do relato.
  //
  // E o MESMO par que "Tirar de Continuar assistindo" faz logo
  // abaixo, e pelo mesmo motivo: quem terminou nao tem o que
  // retomar. So na direcao "assistido"; desmarcar nao inventa uma
  // posicao que ninguem gravou.
  if (intencao) {
    char chave[192];
    prog_chave(chave, sizeof chave, ci->imdb, ci->temporada, ci->episodio);
    prog_remover(chave);
    // As mesmas tres fontes de "Tirar de Continuar assistindo": quem
    // marcou como visto tambem nao quer o card de retomada de volta
    // no proximo ciclo.
    trakt_playback_remover(ci->imdb);
    simkl_playback_remover(ci->imdb);
    syncprog_remover(chave);
    cat_zerar_progresso(atual);
  }
}

// OK na pagina de estilos. Grava e sai: a home remonta pela revisao de
// fileiras.c e a pessoa ve a forma nova no lugar, sem o veu do menu por cima.
// A forma da linha `i` no tamanho escolhido nela, e a maior da linha (a que
// decide a reducao da previa).
static int estTipo(int i) {
  int j;
  if (i < 0 || i >= nEstilos) return -1;
  j = estTam[i];
  if (j < 0 || j >= estLin[i].n) j = 0;
  return estLin[i].tipos[j];
}
static int estRef(int i) {
  return (i >= 0 && i < nEstilos) ? estLin[i].tipos[estLin[i].n - 1] : -1;
}

static void aplicarEstilo(void) {
  if (estFoco >= 0 && estFoco < nEstilos) fil_definir_tipo(filChave, estTipo(estFoco));
  aberto = 0;
}

static void aplicar(void) {
  int atual = indiceAtual();
  const CatItem *ci = itemAtual();
  int acao;
  if (foco < 0 || foco >= nOps) return;
  acao = ops[foco].acao;
  if (acao == OP_ESTILO) {
    pagina = 1; estFoco = -1; prevAtual = prevAnt = prevRefAtual = prevRefAnt = -1; prevT = 1.0f;
    memset(estAnim, 0, sizeof estAnim);
    montar(); return;
  }
  if (!ci) return;
  // SO A ESPERA BLOQUEIA, e nao "ja houve uma operacao".
  //
  // A guarda antiga era `operacao != CTX_OP_NENHUMA && estado != FALHA`, e
  // como `operacao` nunca volta a NENHUMA enquanto o modal esta aberto, a
  // PRIMEIRA acao confirmada trancava todas as outras: depois de adicionar a
  // biblioteca, "Desmarcar como assistido" no mesmo modal simplesmente nao
  // fazia nada. Era preciso fechar e reabrir, e ninguem adivinha isso.
  //
  // Enquanto a requisicao esta no ar continua valendo esperar: duas escritas
  // simultaneas na mesma superficie e que nao podem acontecer.
  if (acao != OP_DETALHES && estadoOperacao == CTX_PENDENTE) return;
  switch (acao) {
    case OP_DETALHES:
      // O painel resolve pelo IMDb (spainel_pediu_abrir -> app.c), porque o
      // titulo dele pode nao ter indice; a home continua pelo indice.
      if (doPainel) snprintf(pedDetalhesImdb, sizeof pedDetalhesImdb, "%s", ci->imdb);
      else pedDetalhes = idx;
      break;
    case OP_LISTA:
      // Captura a intencao ANTES de qualquer escrita. O mesmo valor segue para
      // o POST e so chega ao espelho local depois de uma resposta 2xx.
      intencao = !tituloSalvo(ci);
      snprintf(operacaoImdb, sizeof operacaoImdb, "%s", ci->imdb);
      operacao = CTX_OP_LISTA;
      fecharAoConfirmar = doPainel && !intencao;
      opSimkl = 0;
      avisoOp = NULL;
      espelhoAplicado = 0;
      estadoOperacao = CTX_PENDENTE;
      // LOCAL PRIMEIRO E SEMPRE. E sincrono e nao pode falhar por rede, entao
      // acontece fora do jogo de estados abaixo; ver salvos.h para por que ele
      // e o unico destino que sobrevive ao fechamento do app.
      salvos_definir(ci, intencao);
      // TRAKT ESCOLHIDO E SEM VINCULO cai no ramo local, como o Simkl sem
      // vinculo ja caia. Antes era CTX_FALHA com a lista local JA escrita: o
      // titulo saia do arquivo, a marca do catalogo ficava, e o "Remover" do
      // painel de Salvos (o padrao de "Onde o + salva" e o Trakt) deixava a
      // linha na tela com "Nao foi possivel". O "+" do detalhe (app.c) sempre
      // marcou o local nesse caso; agora as duas portas concordam.
      if (ajustes_salvos_no_trakt() && trakt_ativo()) {
        if (!trakt_watchlist_tipo(ci->imdb, ci->tipo, intencao))
          estadoOperacao = CTX_FALHA;
      } else if (ajustes_salvos_no_simkl() && simkl_ativo() &&
                 (intencao || simkl_na_plantowatch(ci->imdb))) {
        // PLAN TO WATCH DO SIMKL. O "-" so vai ao Simkl quando o titulo esta
        // no Plan to Watch conhecido: /sync/history/remove apaga o historico
        // do titulo inteiro (ver simkl.h). Fora dele o "-" e so local, pelo
        // ramo de baixo — o mesmo de quem salva na lista do Nuvio.
        opSimkl = 1;
        if (!simkl_lista_tipo(ci->imdb, ci->tipo, intencao))
          estadoOperacao = CTX_FALHA;
      } else {
        // Simkl escolhido e sem vinculo: a lista local ja foi escrita, e o
        // modal diz por que o Simkl nao recebeu.
        avisoOp = simkl_aviso_sem_vinculo(ajustes_salvos_no_simkl());
        // SEM TRAKT NAO HA O QUE ESPERAR, e deixar CTX_PENDENTE aqui seria um
        // modal travado para sempre: ctx_atualizar so sai da espera consultando
        // trakt_operacao_estado, e nenhuma operacao foi aberta la. A escrita
        // local ja terminou, entao o estado correto e "confirmada" — e e ele
        // que faz o espelho (cat_definir_na_lista) rodar no proximo quadro.
        estadoOperacao = CTX_CONFIRMADA;
        espelhoAplicado = 1;
        cat_definir_na_lista(atual, intencao);
        cat_definir_na_lista_imdb(operacaoImdb, intencao);
        desc_remontar_fileiras();
        if (fecharAoConfirmar) { aberto = 0; fecharAoConfirmar = 0; }
      }
      montar();
      break;
    case OP_ASSISTIDO:
      // Progresso e posicao de retomada, nao historico. So um retrato de
      // historico confirmado pode inverter a acao para "desmarcar".
      intencao = historicoDe(ci) == 1 ? 0 : 1;
      snprintf(operacaoImdb, sizeof operacaoImdb, "%s", ci->imdb);
      operacao = CTX_OP_HISTORICO;
      opSimkl = 0;
      avisoOp = NULL;
      espelhoAplicado = 0;
      estadoOperacao = CTX_PENDENTE;
      // SIMKL E CONTA NUVIO, em fio (visto.c), com ou sem Trakt. Antes daqui
      // so havia o Trakt, e sem ele esta acao era "[trakt] historico recusado:
      // Trakt desligado" e CTX_FALHA — o "sem o traktv nao ta dando o watched"
      // do dono. As temporadas vao junto porque desmarcar serie no Simkl sem
      // elas apagaria a serie da biblioteca de la (ver simkl.c).
      visto_titulo(ci->imdb, ci->tipo, ci->temporadas, ci->nTemporadas, intencao,
                   visto_destinos());
      if (trakt_ativo()) {
        // O caminho do Trakt NAO MUDOU: mesmo pedido, mesma espera, espelho so
        // depois do 2xx em ctx_atualizar.
        if (!trakt_assistido_tipo(ci->imdb, ci->tipo, intencao))
          estadoOperacao = CTX_FALHA;
      } else {
        // SEM TRAKT NAO HA RESPOSTA A ESPERAR (o mesmo raciocinio do "+" na
        // Lista do Nuvio, acima): o local e a verdade desta TV, aplicado agora,
        // e a conta o devolve no proximo pull (sync.c aplica os vistos da
        // conta justamente quando o Trakt esta desligado).
        espelharAssistido(atual, ci, intencao);
        estadoOperacao = CTX_CONFIRMADA;
        espelhoAplicado = 1;
        desc_remontar_fileiras();
      }
      montar();
      break;
    case OP_CATEGORIA:
      snprintf(pedCategoriaImdb, sizeof pedCategoriaImdb, "%s", ci->imdb);
      aberto = 0;
      break;
    case OP_RECOMENDAR:
      // FECHA ESTE MODAL E ABRE O COMPARTILHADO. O menu do cartaz falou de um
      // INDICE da home; dali em diante quem manda e uma copia do CatItem, pela
      // mesma razao que salvospainel.c copia: o vetor do catalogo troca de
      // bloco a cada republicacao da descoberta.
      if (recenviar_abrir(ci)) aberto = 0;
      break;
    case OP_TIRAR_CONTINUAR: {
      // COPIA ANTES: `ci` aponta para dentro do bloco do catalogo, e
      // desc_tirar_continuar desloca esse bloco (o item seguinte ocupa o
      // lugar). Depois dela `ci->imdb` ja e OUTRO titulo.
      TirarRemoto *tr = (TirarRemoto *)calloc(1, sizeof *tr);
      char imdb[sizeof ci->imdb];
      int temp = ci->temporada, ep = ci->episodio;
      pthread_t t;
      snprintf(imdb, sizeof imdb, "%s", ci->imdb);
      // Efeito local e imediato, ANTES da rede: zera o que a legenda desenha
      // neste indice (o card pode estar numa fileira de catalogo com barra) e
      // desc_tirar_continuar apaga o registro, carimba a remocao e tira o card
      // de "Continuar assistindo" por identidade, subindo a revisao — a home
      // remonta neste mesmo quadro. Antes a revisao nao subia e a home (guarda
      // curto da 1.4) seguia com a contagem velha: o card era coberto pelo
      // vizinho e o ultimo aparecia repetido ate a proxima republicacao
      // (medido em tests/cwremover.sh; ver tirarDaJanela em catalogo.c).
      cat_zerar_progresso(atual);
      desc_tirar_continuar(imdb, temp, ep);
      // AS TRES FONTES, e nao so a local — issue #22.
      //
      // A fileira de retomada e a fusao do registro local, do /sync/playback
      // do Trakt (e do Simkl, #110) e do progresso da conta Nuvio. Apagar so a
      // local fazia a entrada voltar no ciclo seguinte, vinda de qualquer uma
      // das outras: "seleciono remover, o prompt some e nada e removido".
      //
      // EM FIO, e nao aqui: os dois pedidos eram sincronos no fio de DESENHO.
      // Na Samsung isso e XHR sincrono no fio principal do navegador — a tela
      // congela ate os dois servidores responderem, e o menu so fechava
      // depois. Com o carimbo de desc_tirar_continuar a ordem deixou de
      // importar: uma refacao que leia o Trakt antes do DELETE chegar recebe o
      // paused_at velho, e a remocao vence (prog_removido_vence).
      if (tr) {
        snprintf(tr->imdb, sizeof tr->imdb, "%s", imdb);
        prog_chave(tr->chave, sizeof tr->chave, imdb, temp, ep);
        if (pthread_create(&t, NULL, fioTirarRemoto, tr) == 0) pthread_detach(t);
        else fioTirarRemoto(tr);   // sem fio: faz aqui, como antes
      }
      // O Simkl tambem guarda o pausado (issue #110). Ja sai em fio proprio;
      // sem id conhecido (item que nao veio do Simkl) nao faz nada.
      simkl_playback_remover(imdb);
      aberto = 0;
      break;
    }
  }
  if (acao == OP_DETALHES) aberto = 0;
}

void ctx_evento(const SDL_Event *e) {
  int k;
  if (!aberto) return;
  // A SOLTURA VEM POR AQUI TAMBEM, e nao so pelo SDL_AddEventWatch de
  // observarHold: com o modal aberto, app.c entrega o evento a esta funcao e
  // nao ha garantia de que o watch tenha visto o mesmo KEYUP (as teclas
  // injetadas de /tmp/nuvio-key, por exemplo, nao passam pela fila do SDL).
  // Sem esta linha a marca nunca cairia por esse caminho e o modal ficaria
  // surdo ao OK.
  if (e->type == SDL_KEYUP && teclaOk(e->key.keysym.sym)) esperandoSoltura = 0;
  if (e->type != SDL_KEYDOWN) return;
  k = e->key.keysym.sym;
  // Repeticao automatica NUNCA e uma segunda escolha: quem quer clicar duas
  // vezes solta e aperta de novo.
  if (e->key.repeat && teclaOk(k)) return;
  if (esperandoSoltura && teclaOk(k)) return;
  if (k == SDLK_AC_BACK || k == SDLK_ESCAPE || k == SDLK_BACKSPACE ||
      e->key.keysym.scancode == NV_SCANCODE_BACK) {
    // Da pagina de estilos, Voltar volta ao menu do titulo — com o foco na
    // entrada de onde a pessoa veio. Sem titulo por tras (colecao), fecha.
    // Voltar CANCELA: nada foi gravado ao mover o foco, so a previa mudou.
    if (pagina == 1 && !soFileira) {
      pagina = 0; montar();
      for (foco = 0; foco < nOps && ops[foco].acao != OP_ESTILO; foco++) {}
      if (foco >= nOps) foco = 0;
      return;
    }
    aberto = 0; return;
  }
  // Pagina de estilos: cima/baixo trocam a forma, esquerda/direita o tamanho
  // (so nas linhas que tem tamanhos); nada disso grava. OK grava.
  if (pagina == 1) {
    if (k == SDLK_UP)   { if (estFoco > 0) estFoco--; return; }
    if (k == SDLK_DOWN) { if (estFoco + 1 < nEstilos) estFoco++; return; }
    if (estFoco >= 0 && estFoco < nEstilos && estLin[estFoco].n > 1) {
      if (k == SDLK_LEFT)  { if (estTam[estFoco] > 0) estTam[estFoco]--; return; }
      if (k == SDLK_RIGHT) { if (estTam[estFoco] + 1 < estLin[estFoco].n) estTam[estFoco]++; return; }
    }
    if (teclaOk(k)) aplicarEstilo();
    return;
  }
  // Enquanto a requisicao esta no ar, OK nao repete a escrita. O foco continua
  // sendo o do modal e Voltar sempre pode cancelar a espera visual.
  //
  // DEPOIS que ela termina, o OK volta a ser OK. Antes ele virava "fechar" —
  // o modal ficava com os botoes na tela, respondendo ao foco, e o unico
  // efeito de aperta-los era sumir. Somado a guarda de aplicar() logo acima,
  // era a metade visivel do "nao faz nada" no botao de desmarcar.
  if (operacao != CTX_OP_NENHUMA && estadoOperacao == CTX_PENDENTE) return;
  if (k == SDLK_UP)   { if (foco > 0) foco--; return; }
  if (k == SDLK_DOWN) { if (foco + 1 < nOps) foco++; return; }
  if (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE) { aplicar(); return; }
}

void ctx_atualizar(float dt, Uint32 agora) {
  int i;
  int atual;
  if (!holdObservador) {
    SDL_AddEventWatch(observarHold, NULL);
    holdObservador = 1;
  }
  if (holdAtivo && agora - holdDesde >= NV_HOLD_MS) holdPronto = 1;
  if (ajustes_animacoes_reduzidas())
    anim = aberto ? 1.0f : 0.0f;
  else
    anim = anim_mola(anim, aberto ? 1.0f : 0.0f, dt, NV_MOLA_TELA);
  for (i = 0; i < CTX_MAX; i++)
    focoAnim[i] = ajustes_animacoes_reduzidas()
      ? (aberto && foco == i ? 1.0f : 0.0f)
      : anim_mola(focoAnim[i], aberto && foco == i ? 1.0f : 0.0f,
                  dt, NV_MOLA_FOCO);
  // Pagina de estilos: o foco da lista e a troca da previa. So aqui, e so com
  // a pagina aberta — fechada, nada disto anda nem desenha.
  if (aberto && pagina == 1) {
    for (i = 0; i < FIL_TIPO_N; i++)
      estAnim[i] = ajustes_animacoes_reduzidas()
        ? (estFoco == i ? 1.0f : 0.0f)
        : anim_mola(estAnim[i], estFoco == i ? 1.0f : 0.0f, dt, NV_MOLA_FOCO);
    if (estFoco >= 0 && estFoco < nEstilos && estTipo(estFoco) != prevAtual) {
      // A primeira forma entra com o proprio modal; as seguintes, com a mola.
      prevAnt = prevAtual; prevRefAnt = prevRefAtual;
      prevAtual = estTipo(estFoco); prevRefAtual = estRef(estFoco);
      prevT = prevAnt < 0 ? 1.0f : 0.0f;
    }
    prevT = ajustes_animacoes_reduzidas() ? 1.0f : anim_mola(prevT, 1.0f, dt, NV_MOLA_FOCO);
  }

  atual = indiceAtual();
  if (aberto && !soFileira && !itemAtual()) { aberto = 0; return; }

  if (operacao != CTX_OP_NENHUMA && estadoOperacao == CTX_PENDENTE) {
    int novo = opSimkl ? simkl_lista_estado() : trakt_operacao_estado(operacao);
    if (novo == CTX_CONFIRMADA || novo == CTX_FALHA) {
      estadoOperacao = novo;
      if (!espelhoAplicado && itemAtual()) {
        const CatItem *ci = itemAtual();
        if (ci && novo == CTX_CONFIRMADA) {
          if (operacao == CTX_OP_LISTA) {
            cat_definir_na_lista(atual, intencao);
            cat_definir_na_lista_imdb(operacaoImdb, intencao);
          } else {
            espelharAssistido(atual, ci, intencao);
          }
        }
        espelhoAplicado = 1;
        // A HOME TEM DE MUDAR NA HORA.
        //
        // cat_historico_definir_id so mexe na tabela lateral de historico, e
        // nenhuma fileira le dela: quem monta as fileiras e a descoberta, a
        // partir do que o Trakt respondeu. Sem este pedido a mudanca so
        // aparecia no ciclo seguinte — o "tiro de assistido e a home nao da
        // refresh, tenho que sair e voltar" do relato.
        //
        // desc_remontar_fileiras remonta SEM REDE, a partir do que ja esta em
        // memoria; e a mesma porta que a mudanca de limite de fileiras usa.
        desc_remontar_fileiras();
        montar();
      }
      // Remocao pelo painel confirmada: o menu sai e a lista, que ja se
      // remontou pela revisao, fica com o foco na linha seguinte. Falhou: o
      // menu fica, com o "Nao foi possivel" do subtitulo.
      if (fecharAoConfirmar) {
        if (novo == CTX_CONFIRMADA) aberto = 0;
        fecharAoConfirmar = 0;
      }
    }
  }
}

// PONTEIRO (#99): passar por cima de uma opcao a foca (a mesma variavel de
// cima/baixo); o clique e o OK. Clicar fora do cartao fecha, como o Voltar.
static void ponteiroCtxOpcao(int i, int b) { (void)b; if (i >= 0 && i < nOps) foco = i; }
static void ponteiroCtxFora(int a, int b) { (void)a; (void)b; aberto = 0; }
// `b` > 0 e o segmento de tamanho b-1 da linha (o alvo menor, por cima dela).
static void ponteiroEstFoco(int i, int b) {
  if (i < 0 || i >= nEstilos) return;
  estFoco = i;
  if (b > 0 && b - 1 < estLin[i].n) estTam[i] = b - 1;
}
static void ponteiroEstOk(int i, int b) {
  if (i < 0 || i >= nEstilos) return;
  ponteiroEstFoco(i, b);
  aplicarEstilo();
}

// --- O MODAL DE ESTILO ------------------------------------------------------
//
// O mesmo cartao flutuante do menu (vidro ou folha com a luz do realce), mais
// largo: a lista de formas a esquerda e, a direita, um PALCO na cor do fundo da
// Home com o nome da fileira e a fileira desenhada na forma em foco. O palco e
// a Home em miniatura de proposito — a pergunta da pessoa e "como fica la".
#define EST_W       1680.0f
#define EST_LISTA_W  500.0f
#define EST_LINHA     60.0f
#define EST_GAP       10.0f
#define EST_COL_GAP   48.0f
#define EST_PALCO_H  520.0f   // altura minima do palco da previa
#define EST_PALCO_Y  140.0f   // do topo da coluna ao palco: nome + frase

// O SEGMENTADO DE TAMANHO, na ponta direita da pilula da linha `i`: uma letra
// por tamanho (a primeira da palavra traduzida — P M G, S M L), o escolhido
// cheio na cor do texto da pilula e o gravado com um anel. Na tinta da pilula:
// escura quando ela esta acesa (branca), clara em repouso.
#define EST_SEG_W   46.0f
#define EST_SEG_H   40.0f
#define EST_SEG_GAP  6.0f
static void desenhaTamanhos(GfxRect r, int i, int atual, float a) {
  float fr, fg, fb, ti = botao_cor_foco(&fr, &fg, &fb);
  int aceso = estAnim[i] >= 0.5f, j, n = estLin[i].n;
  // A tinta e a do rotulo da pilula (botao_pilula): a de botao_cor_foco acesa,
  // clara em repouso. A letra escolhida vai no fundo da pilula — o realce
  // aceso, o escuro do cartao em repouso — sobre um disco na tinta.
  float tinta = aceso ? ti : 0.93f;
  float lr = aceso ? fr : 0.08f, lg = aceso ? fg : 0.085f, lb = aceso ? fb : 0.1f;
  float x0 = r.x + r.w - 10.0f - (float)n * EST_SEG_W - (float)(n - 1) * EST_SEG_GAP;
  float y0 = r.y + (r.h - EST_SEG_H) * 0.5f;
  for (j = 0; j < n; j++) {
    GfxRect c = { x0 + (float)j * (EST_SEG_W + EST_SEG_GAP), y0, EST_SEG_W, EST_SEG_H };
    const char *pal = i18n(fil_estilo_tam_palavra(j));
    char letra[8];
    int k = 0, esc = j == estTam[i], t8 = (int)(tinta * 255.0f + 0.5f);
    TxtLinha l;
    // A primeira letra, inteira: um caractere UTF-8 e o lider mais as
    // continuacoes (10xxxxxx).
    if (pal[0]) { letra[k++] = pal[0];
      while (k < 6 && ((unsigned char)pal[k] & 0xC0) == 0x80) { letra[k] = pal[k]; k++; } }
    letra[k] = 0;
    if (aberto && a > 0.5f)
      ponteiro_alvo(c.x, c.y, c.w, c.h, ponteiroEstFoco, ponteiroEstOk, i, j + 1);
    if (esc) gfx_cor(c, 0.5f, tinta, tinta, tinta, 0.95f * a);
    else if (estLin[i].tipos[j] == atual)
      gfx_anel(c, 0.5f, 1.5f, tinta, tinta, tinta, 0.6f * a);
    l = esc ? txt_linha(TXT_DET_BOTAO, letra, (int)(lr * 255.0f + 0.5f),
                        (int)(lg * 255.0f + 0.5f), (int)(lb * 255.0f + 0.5f), 255)
            : txt_linha(TXT_DET_BOTAO, letra, t8, t8, t8, 255);
    txt_desenhar_alpha(l, c.x + (c.w - (float)l.w) * 0.5f, c.y + (c.h - (float)l.h) * 0.5f,
                       (esc ? 1.0f : 0.72f) * a);
  }
}

static void desenhaEstilos(float a) {
  float lista = (float)nEstilos * (EST_LINHA + EST_GAP) - EST_GAP;
  float corpo = lista > EST_PALCO_H + EST_PALCO_Y ? lista : EST_PALCO_H + EST_PALCO_Y;
  float alt = CTX_PAD * 2.0f + 116.0f + corpo + CTX_RODAPE;
  float x = (NV_TELA_W - EST_W) * 0.5f, y = (NV_TELA_H - alt) * 0.5f;
  float topo, dx, dw;
  int i, atual = fil_tipo(filChave);
  float ar_, ag_, ab_;
  { GfxRect tela = { 0, 0, NV_TELA_W, NV_TELA_H };
    gfx_cor(tela, 0.0f, 0, 0, 0, 0.72f * a); }
  if (aberto && a > 0.5f && ponteiro_ativo()) {
    ponteiro_alvo(0, 0, NV_TELA_W, NV_TELA_H, NULL, ponteiroCtxFora, 0, 0);
    ponteiro_alvo(x, y, EST_W, alt, NULL, NULL, 0, 0);
  }
  y += (1.0f - a) * 40.0f;
  ajustes_acento(&ar_, &ag_, &ab_);
  { GfxRect p = { x, y, EST_W, alt };
    float raio = 28.0f / alt;
    if (ajustes_vidro()) gfx_vidro_folha(p, raio, a);
    else {
      gfx_cor(p, raio, 0.055f, 0.058f, 0.068f, 0.94f * a);
      gfx_luz_canto(p, raio, EST_W * 0.1f, -EST_W * 0.1f, EST_W * 0.4f, ar_, ag_, ab_, 0.22f * a);
    } }
  { TxtLinha t = txt_linha(TXT_CAPTION2, "FILEIRA SELECIONADA", 174, 178, 188, 255);
    txt_desenhar_alpha(t, x + CTX_PAD, y + CTX_PAD, a * 0.95f); }
  { TxtLinha t = txt_linha_corta(TXT_HEADLINE, filTitulo, 245, 248, 255, 255,
                                 EST_W - CTX_PAD * 2.0f);
    txt_desenhar_alpha(t, x + CTX_PAD, y + CTX_PAD + 28.0f, a); }
  { TxtLinha t = txt_linha(TXT_DET_META2, "Estilo da fileira", 150, 154, 163, 255);
    txt_desenhar_alpha(t, x + CTX_PAD, y + CTX_PAD + 70.0f, a * 0.9f); }
  topo = y + CTX_PAD + 116.0f;

  // A LISTA. O visto marca a forma que vale agora; o foco e so a previa.
  for (i = 0; i < nEstilos; i++) {
    GfxRect r = { x + CTX_PAD, topo + (float)i * (EST_LINHA + EST_GAP), EST_LISTA_W, EST_LINHA };
    int j, temAtual = 0;
    for (j = 0; j < estLin[i].n; j++) if (estLin[i].tipos[j] == atual) temAtual = 1;
    if (aberto && a > 0.5f)
      ponteiro_alvo(r.x, r.y, r.w, r.h, ponteiroEstFoco, ponteiroEstOk, i, 0);
    botao_pilula(r, estLin[i].rotulo, temAtual ? "check" : "", estAnim[i], 1, 1, a);
    if (estLin[i].n > 1) desenhaTamanhos(r, i, atual, a);
  }

  // A DIREITA: o nome da forma em foco, a frase dela e o palco.
  dx = x + CTX_PAD + EST_LISTA_W + EST_COL_GAP;
  dw = x + EST_W - CTX_PAD - dx;
  if (estFoco >= 0 && estFoco < nEstilos) {
    float ty = topo;
    int jt = estTam[estFoco] >= 0 && estTam[estFoco] < estLin[estFoco].n ? estTam[estFoco] : 0;
    TxtLinha nome = txt_linha(TXT_TITULO3, estLin[estFoco].nomes[jt], 245, 248, 255, 255);
    txt_desenhar_alpha(nome, dx, ty, a);
    if (estTipo(estFoco) == atual)
      badge_desenhar(dx + nome.w + 18.0f, ty + (nome.h - BADGE_H) * 0.5f, "Atual",
                     BADGE_REALCE, a);
    ty += nome.h + 8.0f;
    txt_bloco_corta(TXT_CAPTION2, i18n(fil_estilo_ajuda(filChave, estTipo(estFoco))),
                    170, 174, 184, dx, ty, dw, 30.0f, a * 0.92f, 2);
  }
  // O palco comeca abaixo das duas linhas da frase e vai ate a base da lista.
  { GfxRect palco = { dx, topo + EST_PALCO_Y, dw, corpo - EST_PALCO_Y };
    TxtLinha tl;
    GfxRect area;
    gfx_cor(palco, 22.0f / palco.h, NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 0.96f * a);
    gfx_anel(palco, 22.0f / palco.h, 1.5f, 1, 1, 1, 0.08f * a);
    tl = txt_linha_corta(TXT_ROW_TITULO, filTitulo, 245, 246, 249, 255, palco.w - 64.0f);
    txt_desenhar_alpha(tl, palco.x + 32.0f, palco.y + 28.0f, a);
    area = (GfxRect){ palco.x + 32.0f, palco.y + 28.0f + tl.h + 20.0f,
                      palco.w - 32.0f, palco.h - (28.0f + tl.h + 20.0f) - 28.0f };
    // A forma que sai desliza para a esquerda e apaga; a nova entra pela
    // direita. 24 px: o bastante para ler "trocou", pouco para competir.
    if (prevAnt >= 0 && prevT < 0.995f) {
      GfxRect s = area; s.x -= 24.0f * prevT;
      home_previa_fileira(filChave, prevAnt, prevRefAnt, s, a * (1.0f - prevT));
    }
    if (prevAtual >= 0) {
      GfxRect e = area; e.x += 24.0f * (1.0f - prevT);
      // O recorte acompanha a area deslocada; sem o corte de volta a direita
      // a arte que entra passaria da borda do palco.
      if (e.x + e.w > palco.x + palco.w) e.w = palco.x + palco.w - e.x;
      home_previa_fileira(filChave, prevAtual, prevRefAtual, e, a * prevT);
    } }

  // O gesto de tamanho so aparece no rodape quando a linha em foco tem um.
  { int comTam = estFoco >= 0 && estFoco < nEstilos && estLin[estFoco].n > 1;
    TxtLinha t = txt_linha(TXT_CAPTION2, comTam
                   ? "↑ ↓ Escolher   ·   ← → Tamanho   ·   OK Aplicar   ·   Voltar Cancelar"
                   : "↑ ↓ Escolher   ·   OK Aplicar   ·   Voltar Cancelar",
                           155, 159, 169, 255);
    txt_desenhar_alpha(t, x + CTX_PAD, y + alt - CTX_PAD - t.h, a * 0.86f); }
}

void ctx_desenhar(Uint32 agora) {
  const CatItem *ci;
  const char *estados[2];
  const char *mensagem = NULL;
  float a = anim, alt, x, y;
  int i, nEstados = 1;
  (void)agora;
  if (!aberto && holdAtivo) {
    float p = (float)(SDL_GetTicks() - holdDesde) / (float)NV_HOLD_MS;
    TxtLinha t;
    // O centro e o da tela, ou o do painel de Salvos quando e nele que o dedo
    // esta (ctx_centro_dica): centrada na tela a barra ficava metade no veu,
    // metade sob o painel.
    float cx = dicaCx >= 0.0f ? dicaCx : NV_TELA_W * 0.5f;
    if (p > 1.0f) p = 1.0f;
    t = txt_linha(TXT_CAPTION2,
                  p >= 1.0f ? "Solte para abrir opções" : "Segure OK para opções",
                  220, 224, 232, 255);
    // No painel a dica cai EM CIMA da ultima linha da lista (ela vai ate a
    // borda de baixo), e o texto dos dois se misturava: "Salvo agoraSegure
    // OK". Uma placa na cor do cartao por baixo separa as duas leituras.
    if (dicaCx >= 0.0f)
      gfx_cor((GfxRect){ cx - 250.0f, NV_TELA_H - 140.0f, 500.0f, 84.0f },
              0.3f, 0.055f, 0.058f, 0.068f, 0.94f);
    txt_desenhar_alpha(t, cx - t.w * 0.5f, NV_TELA_H - 124.0f, 0.94f);
    gfx_cor((GfxRect){ cx - 210.0f, NV_TELA_H - 82.0f,
                       420.0f, 8.0f }, 4.0f, 0.18f, 0.2f, 0.23f, 0.96f);
    gfx_cor((GfxRect){ cx - 210.0f, NV_TELA_H - 82.0f,
                       420.0f * p, 8.0f }, 4.0f, 0.78f, 0.84f, 0.96f, 0.98f);
  }
  if (a < 0.01f) return;
  if (pagina == 1) { desenhaEstilos(a); return; }
  ci = itemAtual();
  if (!ci) return;
  if (estadoOperacao == CTX_PENDENTE)
    mensagem = operacao == CTX_OP_LISTA ? "Atualizando biblioteca..."
                                        : (intencao ? "Marcando como assistido..."
                                                    : "Desmarcando como assistido...");
  else if (estadoOperacao == CTX_CONFIRMADA)
    mensagem = operacao == CTX_OP_LISTA ? "Biblioteca atualizada"
                                        : (intencao ? "Marcado como assistido"
                                                    : "Desmarcado como assistido");
  else if (estadoOperacao == CTX_FALHA)
    mensagem = "Não foi possível atualizar. Tente novamente.";
  if (estadoOperacao == CTX_CONFIRMADA && operacao == CTX_OP_LISTA && avisoOp)
    mensagem = avisoOp;

  estados[0] = ci && tituloSalvo(ci) ? "Na biblioteca" : "Fora da biblioteca";
  if (!strcmp(ci->tipo, "movie") || !strcmp(ci->tipo, "series")) {
    { int historico = historicoDe(ci);
      estados[1] = historico == 1 ? "Assistido"
                   : historico == 0 ? "Não assistido"
                   : ci->progresso > 0 ? "Progresso salvo"
                   : "Histórico não consultado"; }
    nEstados = 2;
  }

  { GfxRect tela = { 0, 0, NV_TELA_W, NV_TELA_H };
    gfx_cor(tela, 0.0f, 0, 0, 0, 0.72f * a); }

  alt = CTX_PAD * 2.0f + CTX_CAB +
        (float)nOps * (CTX_LINHA + CTX_GAP) - CTX_GAP + CTX_RODAPE;
  x = (NV_TELA_W - CTX_W) * 0.5f;
  y = (NV_TELA_H - alt) * 0.5f;
  if (aberto && a > 0.5f && ponteiro_ativo()) {
    ponteiro_alvo(0, 0, NV_TELA_W, NV_TELA_H, NULL, ponteiroCtxFora, 0, 0);
    ponteiro_alvo(x, y, CTX_W, alt, NULL, NULL, 0, 0);
  }
  // Sobe do fundo enquanto aparece, como as outras folhas do app.
  y += (1.0f - a) * 40.0f;

  // CARTAO FLUTUANTE (a "cara nova" da barra lateral, dono, 21/09/2026):
  // cantos de 28 px de verdade (raio normalizado pelo menor lado, senao o
  // canto muda com o numero de opcoes), fundo translucido e UMA luz difusa na
  // cor de realce entrando pelo canto superior esquerdo, presa aos cantos do
  // cartao (GFX_LUZ). Com o veu de tela cheia ja pago, e a ultima camada
  // grande daqui — e mede o cartao, nao a tela.
  float ar_, ag_, ab_; ajustes_acento(&ar_, &ag_, &ab_);
  { GfxRect p = { x, y, CTX_W, alt };
    float menor = alt < CTX_W ? alt : CTX_W, raio = 28.0f / menor;
    if (ajustes_vidro()) {
      // Folha de vidro sem contorno (gfx_vidro_folha), sem luz colorida.
      gfx_vidro_folha(p, raio, a);
    } else {
    gfx_cor(p, raio, 0.055f, 0.058f, 0.068f, 0.94f * a);
    gfx_luz_canto(p, raio, CTX_W * 0.1f, -CTX_W * 0.1f, CTX_W * 0.65f, ar_, ag_, ab_, 0.22f * a); } }

  { TxtLinha t = txt_linha(TXT_CAPTION2, "TÍTULO SELECIONADO",
                           174, 178, 188, 255);
    txt_desenhar_alpha(t, x + CTX_PAD, y + CTX_PAD, a * 0.95f); }
  // O LOGO DO TITULO no lugar do nome (pedido do dono, 01/10): o mesmo do
  // destaque e do detalhe. Sem logo, o nome escrito na mesma caixa.
  logotitulo_desenhar(ci, ci->titulo, TXT_HEADLINE, x + CTX_PAD,
                      y + CTX_PAD + CTX_LOGO_Y, CTX_LOGO_W, CTX_LOGO_H,
                      CTX_W - CTX_PAD * 2.0f, a);
  { const char *subtitulo = mensagem ? mensagem : "Opções do título";
    TxtLinha t = txt_linha(TXT_DET_META2, subtitulo, 150, 154, 163, 255);
    txt_desenhar_alpha(t, x + CTX_PAD, y + CTX_PAD + CTX_LOGO_Y + CTX_LOGO_H, a * 0.9f); }

  // OS SELOS DE ESTADO SAO DA TABELA (badges.h) e TEM HIERARQUIA: o estado
  // POSITIVO ("Na biblioteca", "Assistido") acende em realce a 18 %; o
  // negativo ou desconhecido ("Fora da biblioteca", "Historico nao
  // consultado") e cinza com texto apagado. Antes eram duas pilulas cinza
  // iguais e a pessoa tinha de LER para saber se o titulo ja era dela.
  { float sx = x + CTX_PAD;
    float sy = y + CTX_PAD + CTX_LOGO_Y + CTX_LOGO_H + 38.0f;
    int historico = ci ? historicoDe(ci) : -1;
    for (i = 0; i < nEstados; i++) {
      int positivo = i == 0 ? tituloSalvo(ci) : historico == 1;
      // "Progresso salvo" e o unico estado nem positivo nem negativo: neutro.
      int fraco = i == 0 ? !tituloSalvo(ci) : !(historico < 0 && ci->progresso > 0);
      sx += badge_desenhar(sx, sy, estados[i],
                           positivo ? BADGE_REALCE : fraco ? BADGE_APAGADO : BADGE_NEUTRO,
                           a) + BADGE_GAP;
    } }

  for (i = 0; i < nOps; i++) {
    float by = y + CTX_PAD + CTX_CAB + (float)i * (CTX_LINHA + CTX_GAP);
    GfxRect r = { x + CTX_PAD, by, CTX_W - CTX_PAD * 2.0f, CTX_LINHA };
    float f = focoAnim[i];
    if (aberto && a > 0.5f)
      ponteiro_alvo(r.x, r.y, r.w, r.h, ponteiroCtxOpcao, NULL, i, 0);
    // O menu usava uma pilula propria: 86px, cinza fixo, TXT_PLR_CORPO e uma
    // seta desenhada a mao. Isso fazia as acoes parecerem de outra tela. O
    // componente comum concentra altura, raio, luz, acento e tinta legivel;
    // aqui ele so recebe a opcao como uma acao primaria alinhada a esquerda.
    // O ICONE DIZ O QUE A OPCAO FAZ antes de a pessoa ler — os mesmos PNG
    // dos botoes redondos da tela de titulo (gfx.h), para o menu e o detalhe
    // falarem o mesmo vocabulario: seta = abrir, "+"/olho = biblioteca,
    // olho riscado/aberto = historico, oculto = tirar da fileira, aviao =
    // recomendar.
    const char *icone = "avancar";
    switch (ops[i].acao) {
      case OP_LISTA:     icone = tituloSalvo(ci) ? "visto" : "mais"; break;
      case OP_ASSISTIDO: icone = historicoDe(ci) == 1
                                 ? "naovisto" : "visto"; break;
      case OP_TIRAR_CONTINUAR: icone = "oculto"; break;
      case OP_RECOMENDAR: icone = "recomendar"; break;
      case OP_ESTILO:     icone = "aspecto"; break;
      default: break;
    }
    botao_pilula(r, ops[i].rot, icone, f, 1, 1, a);
  }

  { const char *rodape = estadoOperacao == CTX_PENDENTE
                           ? "Voltar Fechar   Aguarde..."
                           : operacao != CTX_OP_NENHUMA
                           ? "↑ ↓ Navegar   OK Fechar   Voltar Fechar"
                           : "↑ ↓ Navegar   OK Selecionar   Voltar Fechar";
    TxtLinha t = txt_linha(TXT_CAPTION2, rodape,
                           155, 159, 169, 255);
    txt_desenhar_alpha(t, x + CTX_PAD,
                       y + alt - CTX_PAD - t.h, a * 0.86f); }
}
