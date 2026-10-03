// Rail lateral fixa do Nuvio 1.0.1 legacy, com overlay expansível para
// navegação por D-pad.
//
// Duas animacoes independentes, e a separacao e o que da o movimento certo:
//   - `desliza` tira a barra da borda esquerda (posicao);
//   - `expande` troca a largura de "so icone" para "icone + rotulo".
// No tvOS a barra recolhida mostra apenas os icones e so alarga quando ganha o
// foco. Aqui ela nasce fora da tela, entao os dois acontecem quase juntos — mas
// com molas de rigidez diferente, de forma que a largura ATRASA em relacao a
// entrada. E esse atraso que produz a leitura "entrou e entao se abriu"; com uma
// mola so, a barra aparece ja no tamanho final e o efeito some.
//
// Icones derivados dos SVGs originais do sidebar, com alpha e recortes reais.
#include "menu.h"
#include "perfis.h"
#include "tex_cache.h"
#include "gfx.h"
#include "text.h"
#include "anim.h"
#include "layout.h"
#include "ajustes.h"
#include "botoes.h"
#include "ponteiro.h"
#include "home.h"
#include "colecoes.h"
#define NV_ESCALA_TELA   // o arquivo inteiro mede pela tela virtual (escala.h)
#include "escala.h"

// O MENU E UMA BARRA DE ALTURA INTEIRA (dono, 02/10, revisao dos mockups
// "Glass UI — ilha", telas 1 e 2). A primeira versao do Glass UI fez dele uma
// ilha flutuante debaixo do relogio, como no mockup; o dono decidiu depois que,
// nos layouts Moderna e Padrao, o menu aberto e uma BARRA colada na esquerda,
// de cima a baixo, sem margem nenhuma — e a rail fixa e a mesma barra, fina, so
// com os icones. Todo o RESTO continua o do mockup, medida por medida (o CSS
// e o de glass-ilha.html, da branch do Glass UI, classes .ilha e .it):
//   - item: 60 de altura, pilula (raio 30), 4 de vao, recuo de 20 ate o icone;
//   - icone de TRACO (mt_*.png, tools/icones-menu.sh) de 26, 18 ate o rotulo;
//   - rotulo Inter 22/500 em (243,242,239) a 72 %; em foco branco cheio;
//   - foco = superficie clara (vidro: branco a 14 %; solido: #2b2d34 opaco);
//   - pagina ativa = ponto de 5 px no acento, a 6 px da ponta esquerda da
//     linha (na rail, embaixo do icone, como no mockup 2);
//   - fio de 1 px a 8 % antes do perfil, e o perfil com avatar de 40, nome e
//     "Trocar de usuario" em cinza.
// O layout Dinamica (barra da Apple TV, mais abaixo) NAO usa nada disto.
//
// O CENTRO DO ICONE E O MESMO NAS DUAS LARGURAS (48 px da borda): o icone NAO
// anda quando a barra abre, so o rotulo entra ao lado dele. Se o icone
// deslizasse junto, a abertura viraria um empurrao lateral em vez de uma
// revelacao. Por isso o recuo da linha (16) + o recuo do icone (19) + meio
// icone (13) = 48 = meia largura da rail.
//
// A rail fixa (collapseSidebar desligado) continua reservando os 144 px do
// conteudo (ajustes_rail_largura_fixa): a barra de 96 cabe neles com folga, e
// o conteudo nao muda de lugar por causa desta revisao.
#define NV_MENU_W_RAIL     96.0f
// 16 + 292 (a largura da linha no mockup: ilha de 320 menos 2 x 14) + 16.
#define NV_MENU_W_ABERTO  324.0f
#define NV_MENU_LINHA_X    16.0f   // recuo da pilula da linha, barra aberta
#define NV_MENU_ITEM_H     60.0f
#define NV_MENU_ITEM_VAO    4.0f
// Topo da primeira linha: o mesmo do mockup (ilha em 112 + 14 de recuo). A
// coluna de destinos fica na altura em que o olho ja a conhecia; a barra de
// altura inteira so muda o material em volta dela.
#define NV_MENU_TOPO      126.0f
#define NV_MENU_ICONE      26.0f
#define NV_MENU_ICONE_CX  (NV_MENU_W_RAIL * 0.5f)                       // 48
#define NV_MENU_ROTULO_X  (NV_MENU_ICONE_CX + NV_MENU_ICONE * 0.5f + 18.0f)   // 79
// Fio entre os destinos e o rodape do perfil, como no mockup: 4 de vao da
// lista, 10 de margem, 1 de fio, 10 de margem e mais 4 de vao.
#define NV_MENU_SEP_H      29.0f
// Cor do texto e dos icones em repouso: o --fg do mockup a 72 %.
#define NV_MENU_FG_R      (243.0f / 255.0f)
#define NV_MENU_FG_G      (242.0f / 255.0f)
#define NV_MENU_FG_B      (239.0f / 255.0f)
#define NV_MENU_INATIVO    0.72f
// O RESTO DA TELA ESCURECE POUCO. O veu de 58 % de tela cheia que ficava aqui
// apagava a arte inteira ao abrir a barra — no mockup ela continua viva, e o
// dono apontou isso como o maior sinal de "menos polido". Agora e uma luz
// ESCURA (GFX_LUZ, com nv_dither) nascendo fora da tela a esquerda e morrendo
// antes do meio: encosta na barra o bastante para ela se destacar da arte e
// some sem degrau. Nada de faixas de gfx_cor empilhadas: o painel de 8 bits da
// OLED mostra cada uma como um degrau.
#define NV_MENU_VEU        0.70f
#define NV_MENU_VEU_ALC  1700.0f
// TEMPO DE ABRIR E DE FECHAR, com relogio proprio.
//
// A referencia nao tem barra lateral nenhuma na home — LEFT e UP a partir do
// primeiro card sobem para os botoes do hero e param ali; nao ha rail para
// medir. O unico overlay comparavel que consegui abrir la foi a folha de
// contexto da tecla MENU, e ela da o tempo e a FORMA do veu:
//
//   fechar (medida limpa, 10 quadros seguidos, sem perda):
//     16ms 0,00 | 33 0,09 | 50 0,19 | 66 0,29 | 83 0,38 | 100 0,49
//     117 0,60 | 133 0,73 | 151 0,84 | 166 0,98
//   ou seja RAMPA RETA, ~0,10 a cada 17 ms, terminando em ~150 ms de percurso.
//   abrir: mesma rampa reta, ~0,0044/ms, o que da ~230 ms de percurso.
//
// Dois achados que a mola nao reproduzia: o veu e LINEAR (mola nenhuma e), e
// FECHAR e bem mais rapido que ABRIR. NV_MOLA_TELA (9,0) dava 333 ms simetricos
// e com a partida mais veloz do percurso, que e o oposto de uma rampa.
#define NV_MOLA_MENU_DESFOCO 60.0f
#define NV_MENU_ABRIR_MS  230.0f
#define NV_MENU_FECHAR_MS 150.0f
// A largura continua ATRASADA em relacao a entrada — e o efeito "entrou e
// entao se abriu" descrito no topo do arquivo. Nao ha medida da referencia para
// ele (la nao existe esta barra); o que mudou foi so o tempo total, agora
// amarrado ao mesmo relogio em vez de uma mola de rigidez solta.
#define NV_MENU_EXP_LENTO  1.6f

// "Inicio" sem acento era erro de portugues NA TELA. E "Busca", nao "Buscar":
// os outros tres sao substantivos (Biblioteca, Ajustes) e o verbo destoava.
// Rotulos e ordem conferidos na referencia.
static const char *ROTULOS[MENU_N] = { "Início", "Explorar", "Guia TV", "Busca", "Biblioteca", "Agenda", "Perfil e Stats", "Ajustes" };

// ITEM ESCONDIDO (#162): Explorar, Guia, Agenda e Perfil somem da barra quando
// desligados nos Ajustes. Inicio, Busca, Biblioteca e Ajustes ficam sempre —
// sem os Ajustes nao haveria como trazer os outros de volta. Esconder so tira
// a linha: desenho, alvos do ponteiro e setas pulam o item, e as linhas
// visiveis continuam centralizadas na altura da tela.
static int mostra(int i) {
  switch (i) {
    case MENU_EXPLORAR: return ajustes_menu_explorar();
    case MENU_GUIA:     return ajustes_menu_guia();
    case MENU_AGENDA:   return ajustes_menu_agenda();
    case MENU_PERFIL:   return ajustes_menu_perfil();
    default:            return 1;
  }
}
static int visiveis(void) {
  int i, n = 0;
  for (i = 0; i < MENU_N; i++) n += mostra(i);
  return n;
}
// Topo da k-esima linha VISIVEL (k conta so os itens mostrados).
static float linhaY(int k) {
  return NV_MENU_TOPO + k * (NV_MENU_ITEM_H + NV_MENU_ITEM_VAO);
}
// RODAPE: quem esta usando o app, e a porta para trocar. Ele e um item de
// FOCO a mais, no indice MENU_N — nao entrou no enum de proposito, porque
// trocar de perfil nao e uma aba do app e ninguem deve poder "navegar" para
// ela como destino.
// Medidas do mockup: linha de 72, avatar de 40 aberta e 44 na rail (mockup 2).
#define NV_MENU_RODAPE_H    72.0f
#define NV_MENU_AVATAR      40.0f
#define NV_MENU_AVATAR_RAIL 44.0f
#define NV_MENU_FOCOS      (MENU_N + 1)
#define MENU_RODAPE         MENU_N
// Layout Dinamica: as pastas de Streaming entram como focos depois do rodape
// (MENU_ST0 + k). So a barra da Apple TV usa; a rail classica nao as conhece.
#define NV_MENU_ST_MAX      24
#define MENU_ST0            NV_MENU_FOCOS
#define NV_MENU_FOCOS_TV   (NV_MENU_FOCOS + NV_MENU_ST_MAX)

static int   pediuTrocar = 0;
static int   aberto  = 0;
static int   destino = MENU_INICIO;
static int   linha   = MENU_INICIO;   // destaque; so vira destino ao escolher
static int   mudou   = 0;
static float desliza = 0.0f;
static float expande = 0.0f;
static float animFoco[NV_MENU_FOCOS_TV];
static int   pediuColecao = -1;   // col_folder da pasta escolhida na barra
// SEGURAR OK EM "BUSCAR" abre o Spotlight (spotlight.h). E o caminho da LG,
// onde o microfone do Magic Remote e do sistema e nem todo controle tem a
// amarela. Por isso o OK em Buscar decide na SOLTURA: toque curto = a tela de
// Busca, como sempre; segurado NV_HOLD_MS = o Spotlight, com o dedo ainda no
// botao (menu_atualizar), como o menu do cartaz.
static int    buscaOk, buscaLongo, pediuSpot;
static Uint32 buscaDesde;
static void icone(int d, float cx, float cy, float s, float r, float g, float b, float a);
static void corAvatar(const char *hex, float *r, float *g, float *b);
static int  tvAtivo(void);
static int  tvOrdem(int *lista);
static void tvAtualizar(float dt);
static void tvDesenhar(void);
static void desenhaRodape(GfxRect barra, float e, float alpha, float foco, float aTexto);
static void iconeTraco(int d, float cx, float cy, float s, float r, float g, float b, float a);

static float rodapeY(void) {
  return linhaY(visiveis()) - NV_MENU_ITEM_VAO + NV_MENU_SEP_H;
}
// A BARRA INTEIRA: da borda de cima a de baixo, colada na esquerda. So a
// largura anda (rail <-> aberta).
static GfxRect barraRect(float w) {
  return (GfxRect){ 0.0f, 0.0f, w, NV_TELA_H };
}
// A PILULA DA LINHA nas duas larguras: na rail e o circulo de 60 centrado no
// icone (mockup 2); aberta, a pilula de ponta a ponta com 16 de recuo. `e`
// mistura as duas, e o foco cresce do circulo para a pilula junto com a barra.
static GfxRect linhaRect(float y, float h, float w, float e) {
  float x0 = NV_MENU_ICONE_CX - h * 0.5f;
  float x = anim_mistura(x0, NV_MENU_LINHA_X, e);
  float lw = anim_mistura(h, w - NV_MENU_LINHA_X * 2.0f, e);
  return (GfxRect){ x, y, lw, h };
}

// FOCO DE LINHA = SUPERFICIE UM DEGRAU MAIS CLARA (regra 2 do Glass UI):
// branco a 14 % sobre o vidro (o .it.foco do mockup), #2b2d34 opaco sobre o
// solido. Sem anel, sem bloco cheio no acento — o acento fica para o ESTADO (o
// ponto da pagina ativa). Some por OPACIDADE (a mola `f`), nunca por cor:
// misturar duas cores com alpha cheio deixava a pilula que perdeu o foco como
// placa arrastada (25/09, C9).
static void focoLinha(GfxRect r, float f, float alpha) {
  if (f <= 0.01f || alpha <= 0.01f) return;
  if (ajustes_vidro()) gfx_cor(r, 0.5f, 1, 1, 1, .14f * f * alpha);
  else gfx_cor(r, 0.5f, .169f, .176f, .204f, f * alpha);
}

// PONTEIRO (#99). Passar por cima da rail ABRE a barra ja com o destaque na
// linha sob o cursor — o mesmo que ESQUERDA e depois cima/baixo. O clique e o
// OK de sempre (escolher). Com a barra aberta, clicar fora dela fecha, como o
// Voltar.
static void ponteiroLinha(int i, int b) {
  (void)b;
  if (i < 0 || i >= NV_MENU_FOCOS_TV) return;
  if (!aberto) menu_abrir();
  linha = i;
}
static void ponteiroFora(int a, int b) { (void)a; (void)b; menu_fechar(); }
static void alvosDasLinhas(GfxRect barra) {
  int k = 0;
  if (!ponteiro_ativo()) return;
  for (int i = 0; i < MENU_N; i++) {
    if (!mostra(i)) continue;
    ponteiro_alvo(barra.x, linhaY(k++), barra.w, NV_MENU_ITEM_H, ponteiroLinha, NULL, i, 0);
  }
  ponteiro_alvo(barra.x, rodapeY(), barra.w, NV_MENU_RODAPE_H, ponteiroLinha, NULL, MENU_RODAPE, 0);
}

// O MATERIAL DA BARRA. Vidro: o miolo do .ilha do mockup, rgba(14,15,18,.80)
// — a arte aparece por ele, nitida — e uma luz larga e fraca entrando pelo
// canto de cima, que e o radial-gradient do .ilha::before (branco a 10 % a
// 22 % da largura, acima do topo). NAO e gfx_vidro_folha: o miolo dela e um
// cinza mais claro (.085/.088/.10) com um brilho de topo por cima, e na
// barra de altura inteira o comparativo com o mockup saiu acinzentado,
// lavado no alto; o mockup e mais escuro e frio. O miolo sobe de 80 para
// 86 %: no mockup a barra so tinha arte atras, mas na home de verdade o
// titulo, a sinopse e o botao do destaque moram em x = 104..400, EMBAIXO da
// barra aberta, e a 80 % eles atravessavam legiveis entre os rotulos (captura
// de tests/homelayouts_shot.sh, Moderna). A 86 % a arte ainda aparece, o texto
// de tras nao compete. Solido (Interface de vidro
// desligada): cinza opaco, sem a luz — no mockup o ::before some no solido.
//
// Sem sombra: a barra encosta nas tres bordas da tela, e a sombra grossa da
// ilha flutuante (que existia para descola-la da arte) aqui so sujava a arte
// ao lado. O que separa a barra do conteudo e UM FIO de 1,5 px a 8 % na borda
// direita — reto, porque a barra nao tem canto arredondado.
static void desenhaMaterial(GfxRect r, float a) {
  if (a <= 0.01f) return;
  if (ajustes_vidro()) {
    gfx_cor(r, 0.0f, 14.0f / 255.0f, 15.0f / 255.0f, 18.0f / 255.0f, .86f * a);
    gfx_luz_canto(r, 0.0f, r.w * .22f, -160.0f, 560.0f, 1, 1, 1, .10f * a);
  } else {
    gfx_cor(r, 0.0f, .071f, .075f, .086f, a);
  }
  gfx_cor((GfxRect){ r.x + r.w - 1.5f, r.y, 1.5f, r.h }, 0.0f, 1, 1, 1, .08f * a);
}

// A BARRA nas duas larguras. `e` vai de 0 (rail: so icones) a 1 (aberta, com
// rotulos); `a` e a opacidade da barra toda; `focos` diz se as molas de foco
// valem (a rail parada nao tem foco: a primeira tecla abre a barra).
static void desenhaBarra(float e, float a, int focos) {
  float ar, ag, ab;
  float w = anim_mistura(NV_MENU_W_RAIL, NV_MENU_W_ABERTO, e);
  GfxRect barra = barraRect(w);
  // O rotulo entra com a largura, nao antes dela: `e` ao quadrado segura a
  // palavra ate a barra ter espaco de verdade, senao ela nasce espremida
  // contra o icone.
  float aRot = e * e * a;
  int k = 0;
  ajustes_acento(&ar, &ag, &ab);
  desenhaMaterial(barra, a);

  // Tudo daqui para baixo fica preso a barra. Sem o recorte, o rotulo — que e
  // desenhado no x fixo do texto — vaza para o conteudo enquanto a barra ainda
  // esta estreita, e ve-se a palavra aparecendo fora dela.
  gfx_recorte(barra.x, barra.y, barra.w - 1.5f, barra.h);
  for (int i = 0; i < MENU_N; i++) {
    if (!mostra(i)) continue;
    float y = linhaY(k++);
    float f = focos ? animFoco[i] : 0.0f;
    float cy = y + NV_MENU_ITEM_H * 0.5f;
    GfxRect linhaR = linhaRect(y, NV_MENU_ITEM_H, w, e);
    int atual = (i == destino);
    focoLinha(linhaR, f, a);
    // ONDE VOCE ESTA: um ponto de 5 px no acento (regra 4: acento so para
    // estado). Aberta, a 6 px da ponta esquerda da linha, na altura do centro
    // (.it.ativo::after); na rail, 8 px acima da base do circulo, centrado
    // (mockup 2). O ponto anda entre os dois lugares junto com a largura.
    if (atual) {
      float px = anim_mistura(NV_MENU_ICONE_CX - 2.5f, NV_MENU_LINHA_X + 6.0f, e);
      float py = anim_mistura(y + NV_MENU_ITEM_H - 8.0f - 5.0f, cy - 2.5f, e);
      gfx_cor((GfxRect){ px, py, 5.0f, 5.0f }, 0.5f, ar, ag, ab, a);
    }
    // DOIS TONS, como no mockup: em foco branco cheio, o resto no --fg a 72 %.
    // A pagina ativa NAO ganha tom proprio na barra aberta — quem diz "voce
    // esta aqui" e o ponto; um terceiro cinza competia com ele. Na rail, onde
    // nao ha rotulo e o ponto e miudo, o icone ativo fica branco (mockup 2).
    { float base = atual ? anim_mistura(1.0f, NV_MENU_INATIVO, e) : NV_MENU_INATIVO;
      float op = anim_mistura(base, 1.0f, f);
      float tr = anim_mistura(NV_MENU_FG_R, 1.0f, f), tg = anim_mistura(NV_MENU_FG_G, 1.0f, f),
            tb = anim_mistura(NV_MENU_FG_B, 1.0f, f);
      // Na rail o icone sobe 3 px no circulo do ativo para o ponto caber
      // embaixo dele sem encostar.
      float dy = atual ? anim_mistura(-3.0f, 0.0f, e) : 0.0f;
      iconeTraco(i, NV_MENU_ICONE_CX, cy + dy, NV_MENU_ICONE, tr, tg, tb, op * a);
      if (aRot > 0.01f) {
        // Texto ja rasterizado nao muda de cor: troca no meio da mola, e a
        // opacidade (72 % -> 100 %) anda com ela.
        int emFoco = f > 0.5f;
        TxtLinha l = emFoco
          ? txt_linha_corta(TXT_PG_ROTULO, ROTULOS[i], 255, 255, 255, 255,
                            NV_MENU_W_ABERTO - NV_MENU_ROTULO_X - 24.0f)
          : txt_linha_corta(TXT_PG_ROTULO, ROTULOS[i], 243, 242, 239, 255,
                            NV_MENU_W_ABERTO - NV_MENU_ROTULO_X - 24.0f);
        txt_desenhar_alpha(l, NV_MENU_ROTULO_X, cy - l.h * 0.5f, aRot * op);
      } }
  }
  // O fio do rodape encolhe com a barra: na rail e um traco de 40 centrado no
  // icone (mockup 2, a 10 %); aberta vai de 14 a 14 da pilula (mockup 1, 8 %).
  { float fx = anim_mistura(NV_MENU_ICONE_CX - 20.0f, NV_MENU_LINHA_X + 14.0f, e);
    float fw = anim_mistura(40.0f, w - (NV_MENU_LINHA_X + 14.0f) * 2.0f, e);
    gfx_cor((GfxRect){ fx, rodapeY() - NV_MENU_ITEM_VAO - 11.0f, fw, 1.0f },
            0, 1, 1, 1, anim_mistura(.10f, .08f, e) * a); }
  desenhaRodape(barra, e, a, focos ? animFoco[MENU_RODAPE] : 0.0f, aRot);
  gfx_sem_recorte();
}

int menu_iniciar(void) {
  aberto = 0; destino = MENU_INICIO; linha = MENU_INICIO; mudou = 0;
  desliza = 0.0f; expande = 0.0f;
  for (int i = 0; i < MENU_N; i++) animFoco[i] = 0.0f;
  return 1;
}

void menu_abrir(void) {
  if (aberto) return;
  // O destaque comeca sempre no destino em vigor, nunca onde ficou da ultima
  // vez: a barra e um mapa de onde voce esta, e abrir com o destaque em outro
  // item faria o usuario ler que ja mudou de tela.
  linha = mostra(destino) ? destino : MENU_INICIO;
  aberto = 1;
  buscaOk = buscaLongo = 0;
}
void menu_fechar(void) { aberto = 0; linha = destino; }

int menu_aberto(void)  { return aberto; }
int menu_visivel(void) { return 1; }
int menu_destino(void) { return destino; }
void menu_definir_destino(int d) {
  if (d < 0 || d >= MENU_N) return;
  destino = d;
  if (!aberto) linha = d;
}
int menu_mudou_destino(void) { int m = mudou; mudou = 0; return m; }
const char *menu_rotulo(int d) {
  return (d >= 0 && d < MENU_N) ? ROTULOS[d] : "";
}

// Confirma o destaque e recolhe. DIREITA tambem passa por aqui: no aparelho a
// barra nao "cancela" ao sair pela direita — o item destacado e o que o usuario
// esta olhando, e desfazer a escolha no caminho de volta seria surpresa.
static int tvPastaDoFoco(int foco);
static void escolher(void) {
  if (linha >= MENU_ST0) {
    // Pasta de Streaming (layout Dinamica): abre a colecao, sem trocar de aba.
    pediuColecao = tvPastaDoFoco(linha);
    aberto = 0;
    linha = destino;
    return;
  }
  if (linha == MENU_RODAPE) {
    // O rodape nao troca de destino: ele pede a tela de escolha de perfil.
    pediuTrocar = 1;
    aberto = 0;
    linha = destino;
    return;
  }
  if (linha != destino) { destino = linha; mudou = 1; }
  aberto = 0;
}

int menu_pediu_trocar(void) { int p = pediuTrocar; pediuTrocar = 0; return p; }
int menu_pediu_colecao(void) { int c = pediuColecao; pediuColecao = -1; return c; }
int menu_pediu_spotlight(void) { int p = pediuSpot; pediuSpot = 0; return p; }

void menu_evento(const SDL_Event *e) {
  if (!aberto) return;
  if (e->type == SDL_KEYUP && buscaOk &&
      (e->key.keysym.sym == SDLK_RETURN || e->key.keysym.sym == SDLK_KP_ENTER)) {
    int longo = buscaLongo;
    buscaOk = buscaLongo = 0;
    if (!longo && linha == MENU_BUSCAR) escolher();
    return;
  }
  if (e->type != SDL_KEYDOWN) return;
  SDL_Keycode k = e->key.keysym.sym;
  // OK em Buscar so arma; a repeticao do firmware (OK segurado manda KEYDOWNs
  // separados) nao rearma.
  if ((k == SDLK_RETURN || k == SDLK_KP_ENTER) && linha == MENU_BUSCAR) {
    if (!buscaOk) { buscaOk = 1; buscaLongo = 0; buscaDesde = SDL_GetTicks(); }
    return;
  }
  buscaOk = 0;

  // Mesmo conjunto de teclas de "voltar" que o detalhe aceita: no controle e o
  // Back, no teclado cada pessoa alcanca uma diferente.
  if (k == SDLK_AC_BACK || k == SDLK_ESCAPE || k == SDLK_BACKSPACE ||
      k == SDLK_DELETE) { menu_fechar(); return; }

  if (k == SDLK_RIGHT || k == SDLK_RETURN || k == SDLK_KP_ENTER) { escolher(); return; }
  if (tvAtivo()) {
    // Ordem da barra da Apple TV: cabecalho (perfil), Buscar, Inicio, ...
    int lista[NV_MENU_FOCOS_TV], n = tvOrdem(lista), p = 0, i;
    for (i = 0; i < n; i++) if (lista[i] == linha) p = i;
    if (k == SDLK_DOWN && p + 1 < n) linha = lista[p + 1];
    else if (k == SDLK_UP && p > 0) linha = lista[p - 1];
    return;
  }
  // Sem rotacao nas pontas: a barra e curta e o usuario ve as quatro linhas de
  // uma vez, entao dar a volta no fim da lista le como falha, nao como atalho.
  if (k == SDLK_DOWN) {
    int j = linha + 1;
    while (j < MENU_N && !mostra(j)) j++;
    if (j < NV_MENU_FOCOS) linha = j;
  } else if (k == SDLK_UP) {
    int j = linha - 1;
    while (j >= 0 && !mostra(j)) j--;
    if (j >= 0) linha = j;
  }
  // ESQUERDA morre aqui de proposito: a barra ja e a borda da tela.
}

void menu_atualizar(float dt, Uint32 agora) {
  if (buscaOk && !buscaLongo && aberto && agora - buscaDesde >= NV_HOLD_MS) {
    buscaLongo = 1;
    pediuSpot = 1;
    aberto = 0;
    linha = destino;
  }
  if (!aberto) buscaOk = buscaLongo = 0;
  if (tvAtivo()) { tvAtualizar(dt); return; }
  // Recolhido e assentado nao custa nada: nem mola, nem laco pelos destinos.
  // `expande` anda 1.6x mais devagar que `desliza`: quando `desliza` chega a
  // 0 ele ainda vale ~0,37. Zerar so "se desliza != 0" falhava sempre que a
  // rampa passava do alvo e cravava 0 exato (FPS variando na TV), e o resto
  // de `expande` deixava o nome do perfil e "Trocar de usuário" apagados ao
  // lado do avatar da rail fixa (#210). Zera os dois, sempre.
  if (!aberto && desliza < 0.002f) {
    desliza = 0.0f; expande = 0.0f;
    return;
  }
  float alvo = aberto ? 1.0f : 0.0f;
  float ms   = aberto ? NV_MENU_ABRIR_MS : NV_MENU_FECHAR_MS;
  desliza = anim_rampa(desliza, alvo, dt, ms);
  expande = anim_rampa(expande, alvo, dt, ms * NV_MENU_EXP_LENTO);
  for (int i = 0; i < NV_MENU_FOCOS; i++) {
    float a = (aberto && i == linha) ? 1.0f : 0.0f;
    // O ITEM QUE SAI APAGA EM ~50 ms, e nao nos 120 ms do NV_MOLA_DESFOCO.
    // A pilula aqui e SOLIDA na cor de realce, com luz em volta: descendo o
    // menu com o controle, os 120 ms deixavam duas ou tres pilulas acesas
    // atras do foco — o "rastro" que o dono viu (25/09, C9), com o FPS em 60.
    // Mesmo valor do painel de Salvos (SP_MOLA_DESFOCO).
    animFoco[i] = anim_mola(animFoco[i], a, dt,
                            a > animFoco[i] ? NV_MOLA_FOCO : NV_MOLA_MENU_DESFOCO);
  }
}

// Mesmos vetores do sidebar oficial, rasterizados no build e tintados pelo shader.
static void icone(int d, float cx, float cy, float s, float r, float g, float b, float a) {
  // `portal` ja e um SVG embarcado e le como entrada para uma descoberta;
  // manter o icone real evita inventar um glifo SDF e duplicar o de Busca.
  static const char *nomes[MENU_N] = {"menu_home", "portal", "menu_guide", "menu_search", "menu_library", "menu_agenda", "menu_profile", "menu_settings"};
  if (d < 0 || d >= MENU_N) return;
  gfx_icone((GfxRect){cx-s*.5f, cy-s*.5f, s, s}, nomes[d], r, g, b, a);
}


// Os icones da barra CLASSICA: os de traco do mockup (tools/icones-menu.sh).
// A Dinamica continua chamando icone(), com os menu_*.png de sempre.
static void iconeTraco(int d, float cx, float cy, float s, float r, float g, float b, float a) {
  static const char *nomes[MENU_N] = {"mt_inicio", "mt_explorar", "mt_guia", "mt_busca", "mt_biblioteca", "mt_agenda", "mt_perfil", "mt_ajustes"};
  if (d < 0 || d >= MENU_N) return;
  gfx_icone((GfxRect){cx-s*.5f, cy-s*.5f, s, s}, nomes[d], r, g, b, a);
}

// Cor do avatar a partir do "#RRGGBB" que a conta guarda. Sem cor legivel, o
// azul do padrao do web.
static void corAvatar(const char *hex, float *r, float *g, float *b) {
  unsigned v = 0;
  *r = 0.12f; *g = 0.53f; *b = 0.90f;
  if (!hex || hex[0] != '#' || strlen(hex) < 7) return;
  if (sscanf(hex + 1, "%6x", &v) != 1) return;
  *r = ((v >> 16) & 255) / 255.0f;
  *g = ((v >> 8) & 255) / 255.0f;
  *b = (v & 255) / 255.0f;
}

// A INICIAL do nome, respeitando UTF-8: um nome comecado por acento tem dois
// bytes, e cortar no primeiro desenha lixo.
static void inicialDe(const char *nome, char *dst, size_t tam) {
  if (tam < 3) { if (tam) dst[0] = 0; return; }
  dst[0] = (nome && nome[0]) ? nome[0] : '?';
  dst[1] = 0;
  if (nome && (unsigned char)nome[0] >= 0xC0 && nome[1]) { dst[1] = nome[1]; dst[2] = 0; }
}

// Rodape: quem esta usando, e a porta para trocar. Desenha nas DUAS larguras —
// na rail so o avatar (44, centrado no icone, mockup 2), aberta o avatar de 40
// na linha de 72 com o nome e a acao (mockup 1). aTexto e a mesma rampa dos
// rotulos. Foco = a mesma superficie clara das linhas.
static void desenhaRodape(GfxRect barra, float e, float alpha, float foco, float aTexto) {
  const ContaPerfil *p = perfis_item_ativo();
  float y = rodapeY();
  float cy = y + NV_MENU_RODAPE_H * 0.5f;
  float cr, cg, cb;
  float tam = anim_mistura(NV_MENU_AVATAR_RAIL, NV_MENU_AVATAR, e);
  // Aberta, o avatar comeca no mesmo recuo de 20 do icone (.it padding), e
  // por ser mais largo que o icone o centro dele fica 7 px a direita.
  float cx = anim_mistura(NV_MENU_ICONE_CX, NV_MENU_LINHA_X + 19.0f + NV_MENU_AVATAR * 0.5f, e);
  float tx = NV_MENU_LINHA_X + 19.0f + NV_MENU_AVATAR + 18.0f;
  char ini[4];
  GfxRect av;

  if (alpha <= 0.01f) return;
  focoLinha(linhaRect(y, NV_MENU_RODAPE_H, barra.w, e), foco, alpha);

  av.x = cx - tam * 0.5f;
  av.y = cy - tam * 0.5f;
  av.w = av.h = tam;

  // FOTO quando a conta tem uma; senao o circulo com a inicial, que e o mesmo
  // que o app web mostra quando `avatar_url` e nulo — e nesta conta ele e.
  // A inicial e 15 Bold (TXT_MINI): o mockup usa 16/700 num avatar de 40, e
  // o TXT_CALLOUT de antes (28) enchia o circulo.
  { GLuint tex = (p && p->avatarUrl[0]) ? tex_obter(p->avatarUrl) : 0;
    if (tex) {
      gfx_tex_aspect_atual = 1.0f;
      gfx_rect(av, tex, GFX_CARD, 0, 0, 0, 0.5f, 0, 0, 0, alpha);
    } else {
      corAvatar(p ? p->corHex : NULL, &cr, &cg, &cb);
      gfx_cor(av, 0.5f, cr, cg, cb, alpha);
      inicialDe(p ? p->nome : NULL, ini, sizeof ini);
      { TxtLinha l = txt_linha(TXT_MINI, ini, 255, 255, 255, 255);
        txt_desenhar_alpha(l, av.x + (av.w - l.w) * 0.5f,
                           av.y + (av.h - l.h) * 0.5f, alpha); } } }

  if (aTexto > 0.01f) {
    // Nome no corpo dos rotulos (22/500) em --fg cheio; a acao em cinza a
    // 45 %. O mockup pede 16 px para a acao, mas o piso de leitura do app a
    // 3 m e 21 (TXT_CAPTION2) — abaixo disso so selo, nunca frase.
    int emFoco = foco > 0.5f;
    int c = emFoco ? 255 : 243;
    TxtLinha nome = txt_linha_corta(TXT_PG_ROTULO, p ? p->nome : "Sua conta", c, c - (emFoco ? 0 : 1),
                                    c - (emFoco ? 0 : 4), 255, NV_MENU_W_ABERTO - tx - 24.0f);
    TxtLinha acao = txt_linha_corta(TXT_CAPTION2, "Trocar de usuário", 243, 242, 239, 255,
                                    NV_MENU_W_ABERTO - tx - 24.0f);
    float alto = nome.h + 2.0f + acao.h, y0 = cy - alto * 0.5f;
    txt_desenhar_alpha(nome, tx, y0, aTexto);
    txt_desenhar_alpha(acao, tx, y0 + nome.h + 2.0f, aTexto * (emFoco ? .62f : .45f));
  }
}

// Borda direita da barra classica, para a ilha do relogio (ilha.c) ficar FORA
// dela. Abrindo, devolve logo a largura aberta (o relogio sai da frente antes
// da barra chegar); fechando, a largura que ainda esta na tela, para o relogio
// nao voltar por cima da barra que recolhe. 0 sem barra ou no layout Dinamica.
float menu_barra_borda(void) {
  float atual;
  if (tvAtivo()) return 0.0f;
  atual = (aberto || desliza >= 0.002f)
        ? anim_mistura(NV_MENU_W_RAIL, NV_MENU_W_ABERTO, anim_suave(expande)) : 0.0f;
  if (aberto) return NV_MENU_W_ABERTO * gfx_escala_ui();
  if (!ajustes_rail_recolhida() && atual < NV_MENU_W_RAIL) atual = NV_MENU_W_RAIL;
  // Em pixels da tela REAL: a barra e camada ampliada (escala.h).
  return atual * gfx_escala_ui();
}

static void menu_desenharCorpo_(Uint32 agora);
// Camada ampliada (escala.h): o corpo desenha na tela virtual.
void menu_desenhar(Uint32 agora) {
  ESCALA_INI();
  menu_desenharCorpo_(agora);
  ESCALA_FIM();
}
static void menu_desenharCorpo_(Uint32 agora) {
  (void)agora;
  if (tvAtivo()) { tvDesenhar(); return; }
  // Rail fixa sempre presente, como no shell legacy: a barra estreita, so
  // icones. `collapseSidebar`: com a barra RECOLHIDA o web nao desenha rail
  // nenhuma — `.home-nav-list` fica com largura 0 e nao ocupa fluxo; ela so
  // aparece como camada quando ganha foco.
  int fixa = !ajustes_rail_recolhida();
  if (!aberto && desliza < 0.002f) {
    if (fixa) desenhaBarra(0.0f, 1.0f, 0);
    // Recolhida, a rail nao existe na tela; uma faixa na borda faz o papel
    // dela para o ponteiro, como o ESQUERDA na primeira coluna.
    if (fixa) alvosDasLinhas(barraRect(NV_MENU_W_RAIL));
    else alvosDasLinhas((GfxRect){ 0, 0, 28.0f, 0 });
    return;
  }

  // O VEU usa a rampa CRUA: a medida da referencia e uma reta (ver
  // NV_MENU_ABRIR_MS). A LARGURA usa a rampa suavizada e atrasada (`expande`):
  // um bloco parando de vez no fim do percurso le como corte.
  float entrada = anim_suave(desliza);
  float e = anim_suave(expande);
  // Luz ESCURA nascendo 600 px fora da tela, na altura do meio: e quase uma
  // rampa horizontal (o centro esta longe), ~12 % junto a barra aberta e
  // zerada por volta de x = 1100. Um quad do tamanho da area que ela alcanca,
  // nao da tela inteira.
  gfx_luz_canto((GfxRect){ 0, 0, NV_MENU_VEU_ALC - 600.0f, NV_TELA_H }, 0.0f,
                -600.0f, NV_TELA_H * 0.5f, NV_MENU_VEU_ALC, 0, 0, 0, NV_MENU_VEU * desliza);
  if (aberto) {
    GfxRect barra = barraRect(anim_mistura(NV_MENU_W_RAIL, NV_MENU_W_ABERTO, e));
    ponteiro_alvo(0, 0, NV_TELA_W, NV_TELA_H, NULL, ponteiroFora, 0, 0);
    ponteiro_alvo(barra.x, barra.y, barra.w, barra.h, NULL, NULL, 0, 0);
    alvosDasLinhas(barra);
  }
  // Com a rail fixa a barra ja esta na tela e so alarga; recolhida ela nasce
  // (e some) pela opacidade, no mesmo lugar.
  desenhaBarra(e, fixa ? 1.0f : entrada, 1);
}

// ===========================================================================
// BARRA DA APPLE TV (so no layout Dinamica da home; os outros layouts usam a
// rail de cima, sem mudanca nenhuma).
//
// Referencia: fotos do app Apple TV (tvOS 26) na TV do dono, 01/10/2026.
//   FECHADA: nao ha barra lateral. No topo esquerdo, por cima do conteudo, so
//   uma pilula de vidro "‹ (icone) Inicio" com a secao atual.
//   ABERTA: a pilula CRESCE ate virar um painel flutuante arredondado, com
//   margem da borda e altura so ate o ultimo item. Cabecalho com o avatar, o
//   nome e o relogio; itens com o icone num circulo; o item ATUAL tem pilula
//   cinza translucida e o item EM FOCO pilula clara com texto escuro (aqui: a
//   cor de realce, com a tinta por contraste dela — branca no padrao).
//   O conteudo atras escurece so do lado esquerdo, sem desfoque.
//
// Custo: fechada, 3 quads SDF e 2 glifos; aberta, 1 quad de sombra radial
// (meia tela, nenhum veu de tela cheia), 1 painel e ~2 quads por item.
// ===========================================================================
#include <time.h>
#include <ctype.h>

// MEDIDAS DO ORIGINAL (print do app oficial no layout Apple TV, medido pelo
// coordenador em 2000 px e convertido x0,96 para 1920; dono, 01/10: "muito
// pesada", "a letra ta grande demais"):
//   painel x 38, topo 38, ~355 de largura, raio ~40, vidro escuro translucido
//   com aro fino; avatar 44, nome 26 Medium, relogio ~24 Regular; itens sem
//   circulo atras do icone (icone de linha 28 a 85%), rotulo 25 Regular,
//   passo 79, foco = pilula BRANCA cheia de 74; "Streaming" 22 cinza medio;
//   logo da pasta em circulo de 48.
#define TV_PAINEL_X     38.0f
#define TV_PAINEL_Y     38.0f
#define TV_PAINEL_W    355.0f
#define TV_RAIO         40.0f
#define TV_CAB_H        96.0f    // cabecalho: avatar, nome, relogio
#define TV_LINHA_H      79.0f
#define TV_PILULA_H     74.0f
#define TV_PAD_X        10.0f    // pilula da linha por dentro do painel
#define TV_PAD_BASE     14.0f
#define TV_ICONE        28.0f
#define TV_COL_CX       32.0f    // centro da coluna de icones, a partir da pilula
#define TV_ROT_X        66.0f    // x do rotulo, a partir da pilula
#define TV_LOGO         48.0f    // circulo da pasta de Streaming
#define TV_AVATAR       44.0f
#define TV_ROTULO_H     48.0f    // rotulo da secao "Streaming"
// Pilula fechada: mais baixa e com a letra do item (25), nao a de titulo.
#define TV_PIL_CIRC     46.0f
#define TV_ALTURA_MAX  (NV_TELA_H - 2.0f * TV_PAINEL_Y)
#define TV_MOLA_ROLAR   14.0f
// Molas (anim_mola2, rad/s): abrir um pouco mais lento que fechar, como a
// barra do aparelho; com Animacoes reduzidas vai direto ao alvo.
#define TV_MOLA_ABRE    13.0f
#define TV_MOLA_FECHA   17.0f
#define TV_MOLA_PILULA  10.0f

static float tvAbre = 0.0f, tvAbreV = 0.0f;
static float tvRolar = 0.0f, tvRolarV = 0.0f;
static float tvPilAlfa = 1.0f, tvPilAlvo = 1.0f;

static int tvAtivo(void) { return ajustes_home_layout() == HOME_LAYOUT_DINAMICA; }

// Ordem do original: Inicio, Busca, Explorar... (o resto segue a ordem do app).
static const int TV_ORDEM[MENU_N] = {
  MENU_INICIO, MENU_BUSCAR, MENU_EXPLORAR, MENU_GUIA, MENU_AGENDA,
  MENU_BIBLIOTECA, MENU_PERFIL, MENU_AJUSTES
};
static const char *tvRotulo(int d) { return menu_rotulo(d); }
// Focos na ordem de navegacao: cabecalho (trocar de usuario) e os visiveis.
// PASTAS DE STREAMING (home_streaming_barra): a fileira "Streaming" que o
// layout Dinamica tira da home. Indices de col_folder, na ordem da fileira.
static int tvPastas(const int **v) {
  int n = home_streaming_barra(v);
  return n > NV_MENU_ST_MAX ? NV_MENU_ST_MAX : (n < 0 ? 0 : n);
}
static int tvPastaDoFoco(int foco) {
  const int *v;
  int n = tvPastas(&v), k = foco - MENU_ST0;
  return (k >= 0 && k < n) ? v[k] : -1;
}
static int tvOrdem(int *lista) {
  const int *v;
  int n = 0, i, np = tvPastas(&v);
  lista[n++] = MENU_RODAPE;
  for (i = 0; i < MENU_N; i++) if (mostra(TV_ORDEM[i])) lista[n++] = TV_ORDEM[i];
  for (i = 0; i < np; i++) lista[n++] = MENU_ST0 + i;
  return n;
}
// O conteudo do painel em coordenadas PROPRIAS (0 = topo do painel, antes da
// rolagem): `ys[foco]` e o topo de cada linha (-1 = nao esta na barra).
// Devolve a altura total; `yRotulo` recebe o topo do rotulo "Streaming".
static float tvLayout(float *ys, float *yRotulo) {
  const int *v;
  int i, np = tvPastas(&v);
  float y = TV_CAB_H;
  for (i = 0; i < NV_MENU_FOCOS_TV; i++) ys[i] = -1.0f;
  ys[MENU_RODAPE] = 0.0f;
  for (i = 0; i < MENU_N; i++)
    if (mostra(TV_ORDEM[i])) { ys[TV_ORDEM[i]] = y; y += TV_LINHA_H; }
  if (yRotulo) *yRotulo = -1.0f;
  if (np) {
    if (yRotulo) *yRotulo = y;
    y += TV_ROTULO_H;
    for (i = 0; i < np; i++) { ys[MENU_ST0 + i] = y; y += TV_LINHA_H; }
  }
  return y + TV_PAD_BASE;
}
// Altura na tela: ate o ultimo item, no maximo a tela menos as margens (dai
// para baixo a barra ROLA, com o foco sempre visivel).
static GfxRect tvPainel(void) {
  float ys[NV_MENU_FOCOS_TV], h = tvLayout(ys, NULL);
  GfxRect r = { TV_PAINEL_X, TV_PAINEL_Y, TV_PAINEL_W, h < TV_ALTURA_MAX ? h : TV_ALTURA_MAX };
  return r;
}

// A PILULA FECHADA: "‹" + circulo + rotulo da secao atual.
static float tvPilulaLargura(void) {
  return 7.0f + TV_PIL_CIRC + 14.0f + (float)txt_largura(TXT_BODY, tvRotulo(destino)) + 24.0f;
}
static GfxRect tvPilula(void) {
  GfxRect r = { NV_MENU_PILULA_X + NV_MENU_PILULA_SETA, NV_MENU_PILULA_Y,
                tvPilulaLargura(), NV_MENU_PILULA_H };
  return r;
}

int menu_pilula_rect(float *x, float *y, float *w, float *h) {
  GfxRect r;
  if (!tvAtivo()) { if (x) *x = 0; if (y) *y = 0; if (w) *w = 0; if (h) *h = 0; return 0; }
  r = tvPilula();
  // O retangulo devolvido INCLUI a seta, que fica a esquerda da pilula. Em
  // pixels da tela REAL: a pilula e camada ampliada (escala.h), e quem pergunta
  // (Busca, Biblioteca, a ilha) converte para a tela dele.
  { float e = gfx_escala_ui();
    if (x) *x = NV_MENU_PILULA_X * e;
    if (y) *y = r.y * e;
    if (w) *w = (r.x + r.w - NV_MENU_PILULA_X) * e;
    if (h) *h = r.h * e; }
  return 1;
}
int menu_pilula_titulo(void) { return tvAtivo(); }
float menu_pilula_alfa(void) {
  if (!tvAtivo()) return 0.0f;
  { float a = tvPilAlfa * (1.0f - tvAbre); return a < 0.0f ? 0.0f : a; }
}
void menu_pilula_mostrar(float alvo) {
  // Negativo = some JA, sem mola: ao trocar para uma tela com titulo no canto
  // a pilula nao pode ficar 300 ms por cima dele.
  if (alvo < 0.0f) { tvPilAlvo = tvPilAlfa = 0.0f; return; }
  tvPilAlvo = alvo > 1.0f ? 1.0f : alvo;
}

static void tvAtualizar(float dt) {
  int i;
  float alvo = aberto ? 1.0f : 0.0f;
  desliza = 0.0f; expande = 0.0f;
  tvPilAlfa = anim_mola(tvPilAlfa, tvPilAlvo, dt, TV_MOLA_PILULA);
  if (!aberto && tvAbre < 0.002f && tvAbreV == 0.0f) {
    tvAbre = 0.0f; tvRolar = 0.0f; tvRolarV = 0.0f; return;
  }
  tvAbre = anim_mola2(&tvAbreV, tvAbre, alvo, dt, aberto ? TV_MOLA_ABRE : TV_MOLA_FECHA);
  if (!aberto && tvAbre < 0.002f) { tvAbre = 0.0f; tvAbreV = 0.0f; }
  // ROLAGEM: a linha em foco fica inteira dentro do painel, com folga de meia
  // linha (a seguinte aparece cortada, que e o aviso de que ha mais).
  { float ys[NV_MENU_FOCOS_TV], total = tvLayout(ys, NULL), alvoR = tvRolar;
    float vis = total < TV_ALTURA_MAX ? total : TV_ALTURA_MAX;
    float y0 = (linha >= 0 && linha < NV_MENU_FOCOS_TV) ? ys[linha] : 0.0f;
    float h0 = linha == MENU_RODAPE ? TV_CAB_H : TV_LINHA_H, folga = TV_LINHA_H * 0.5f;
    if (y0 >= 0.0f) {
      if (y0 - folga < alvoR) alvoR = y0 - folga;
      if (y0 + h0 + folga > alvoR + vis) alvoR = y0 + h0 + folga - vis;
    }
    if (alvoR > total - vis) alvoR = total - vis;
    if (alvoR < 0.0f) alvoR = 0.0f;
    if (!aberto && tvAbre <= 0.002f) { tvRolar = 0.0f; tvRolarV = 0.0f; }
    else tvRolar = anim_mola2(&tvRolarV, tvRolar, alvoR, dt, TV_MOLA_ROLAR); }
  for (i = 0; i < NV_MENU_FOCOS_TV; i++) {
    float a = (aberto && i == linha) ? 1.0f : 0.0f;
    animFoco[i] = anim_mola(animFoco[i], a, dt,
                            a > animFoco[i] ? NV_MOLA_FOCO : NV_MOLA_MENU_DESFOCO);
  }
}

// Iniciais como na referencia ("HR"): a primeira letra das duas primeiras
// palavras do nome, respeitando UTF-8. Nome de uma palavra so: uma letra.
static void tvIniciais(const char *nome, char *dst, size_t tam) {
  size_t n = 0;
  int palavras = 0;
  const char *p = nome && nome[0] ? nome : "?";
  while (*p && palavras < 2 && n + 5 < tam) {
    while (*p == ' ') p++;
    if (!*p) break;
    { size_t len = 1;
      unsigned char c = (unsigned char)*p;
      if (c >= 0xF0) len = 4; else if (c >= 0xE0) len = 3; else if (c >= 0xC0) len = 2;
      if (len == 1) dst[n++] = (char)toupper(c);
      else { size_t k; for (k = 0; k < len && p[k]; k++) dst[n++] = p[k]; } }
    palavras++;
    while (*p && *p != ' ') p++;
  }
  dst[n] = 0;
}

static void tvRelogio(char *buf, size_t tam) {
  time_t t = time(NULL);
  struct tm tmv;
#ifdef _WIN32
  localtime_s(&tmv, &t);
#else
  localtime_r(&t, &tmv);
#endif
  snprintf(buf, tam, "%02d:%02d", tmv.tm_hour, tmv.tm_min);
}

// Icone de linha, sem bolha atras: branco a 85% em repouso, a tinta escura
// sobre a pilula branca do foco.
static void tvIcone(int d, float cx, float cy, float foco, float alfa) {
  float lum = foco > 0.5f ? 0.08f : 1.0f;
  icone(d, cx, cy, TV_ICONE, lum, lum, lum, alfa * (foco > 0.5f ? 1.0f : 0.85f));
}

// Circulo da PASTA de Streaming: a capa dela recortada em circulo (cover).
// Sem capa ainda (rede), a cor da pasta com a inicial do nome.
static void tvCirculoPasta(const ColFolder *pf, float cx, float cy, float alfa) {
  GfxRect c = { cx - TV_LOGO * 0.5f, cy - TV_LOGO * 0.5f, TV_LOGO, TV_LOGO };
  const char *capa = pf ? col_capa(pf) : NULL;
  GLuint tex = (capa && capa[0]) ? tex_obter(capa) : 0;
  if (tex) {
    float asp = tex_aspecto(capa);
    gfx_tex_aspect_atual = asp > 0.0f ? asp : 1.0f;
    gfx_card_forcar_cover_atual = 1.0f;
    gfx_rect(c, tex, GFX_CARD, 0, 0, 0, 0.5f, 0, 0, 0, alfa);
    gfx_card_forcar_cover_atual = 0.0f;
  } else {
    float r = 0.3f, g = 0.3f, b = 0.34f;
    char ini[8];
    if (pf) col_cor(pf, &r, &g, &b);
    gfx_cor(c, 0.5f, r, g, b, alfa);
    inicialDe(pf ? pf->title : NULL, ini, sizeof ini);
    { TxtLinha l = txt_linha(TXT_CAPTION, ini, 255, 255, 255, 255);
      txt_desenhar_alpha(l, c.x + (c.w - l.w) * 0.5f, c.y + (c.h - l.h) * 0.5f, alfa); }
  }
}

static void tvAvatar(GfxRect av, float alfa) {
  const ContaPerfil *p = perfis_item_ativo();
  GLuint tex = (p && p->avatarUrl[0]) ? tex_obter(p->avatarUrl) : 0;
  if (tex) {
    gfx_tex_aspect_atual = 1.0f;
    gfx_rect(av, tex, GFX_CARD, 0, 0, 0, 0.5f, 0, 0, 0, alfa);
  } else {
    float cr, cg, cb;
    char ini[16];
    corAvatar(p ? p->corHex : NULL, &cr, &cg, &cb);
    gfx_cor(av, 0.5f, cr, cg, cb, alfa);
    tvIniciais(p ? p->nome : NULL, ini, sizeof ini);
    { TxtLinha l = txt_linha(TXT_CAPTION2, ini, 255, 255, 255, 255);
      txt_desenhar_alpha(l, av.x + (av.w - l.w) * 0.5f, av.y + (av.h - l.h) * 0.5f, alfa); }
  }
}

// Escurece SO o lado esquerdo: faixas verticais de cor chapada com o alfa
// caindo em degraus pequenos ate sumir em x ~ 900. Cada pixel e pintado UMA
// vez e pelo shader mais barato (GFX_COR). MEDIDO na C9 (01/10): com uma
// sombra radial (GFX_LUZ, 1000x1080) + aro no painel a barra aberta ficava em
// 47-49 fps contra 60 fechada.
static void tvSombra(float a) {
  int i;
  float x = 0.0f;
  if (a <= 0.01f) return;
  // Veu SUAVE atras do painel (o original nao escurece a tela): 0,22 ate o
  // fim do painel e caindo a zero em ~300 px.
  // Degraus de 20 px e ~0,014 de alfa: com 50 px os degraus apareciam como
  // faixas verticais sobre ceu claro (captura no Mac).
  gfx_cor((GfxRect){ 0, 0, 400, NV_TELA_H }, 0.0f, 0, 0, 0, 0.22f * a);
  x = 400.0f;
  for (i = 1; i <= 15; i++, x += 20.0f)
    gfx_cor((GfxRect){ x, 0, 20, NV_TELA_H }, 0.0f, 0, 0, 0, 0.22f * a * (1.0f - i / 16.0f));
}

static void tvPonteiroPilula(int a, int b) { (void)a; (void)b; menu_abrir(); }

static void tvDesenhar(void) {
  float s = tvAbre < 0.0f ? 0.0f : (tvAbre > 1.0f ? 1.0f : tvAbre);
  GfxRect P = tvPilula(), Q = tvPainel(), R;
  float pa = tvPilAlfa;
  float A, raioPx;
  float ar, ag, ab;
  ajustes_acento(&ar, &ag, &ab);

  // Fechada e com a pilula escondida (pagina rolada): nada na tela. O clique
  // na area da pilula continua abrindo, como o ESQUERDA na primeira coluna.
  if (s <= 0.001f) {
    // Pilula visivel: clicar nela abre. Escondida, o alvo dela cobriria o que
    // a tela tem no canto (o campo da Busca); fica so a faixa da borda, que
    // abre ao passar, como a rail recolhida.
    if (ponteiro_ativo()) {
      if (pa > 0.5f)
        ponteiro_alvo(NV_MENU_PILULA_X, P.y, P.x + P.w - NV_MENU_PILULA_X, P.h,
                      NULL, tvPonteiroPilula, 0, 0);
      else
        ponteiro_alvo(0, 0, 28.0f, NV_TELA_H, tvPonteiroPilula, NULL, 0, 0);
    }
    if (pa <= 0.01f) return;
  }

  tvSombra(s);

  // O painel NASCE da pilula: o retangulo e o raio vao de um ao outro na mola.
  R.x = anim_mistura(P.x, Q.x, s);
  R.y = anim_mistura(P.y, Q.y, s);
  R.w = anim_mistura(P.w, Q.w, s);
  R.h = anim_mistura(P.h, Q.h, s);
  raioPx = anim_mistura(P.h * 0.5f, TV_RAIO, s);
  A = pa + (1.0f - pa) * (s * 3.0f > 1.0f ? 1.0f : s * 3.0f);

  if (aberto) {
    ponteiro_alvo(0, 0, NV_TELA_W, NV_TELA_H, NULL, ponteiroFora, 0, 0);
    ponteiro_alvo(Q.x, Q.y, Q.w, Q.h, NULL, NULL, 0, 0);
  }

  // Superficie de vidro: escuro translucido, tingido de leve pela cor de
  // realce (a arte aparece por tras), com aro fino e claro bem sutil.
  // Alfa 0,82: a 0,72 os titulos das fileiras da home atravessavam o painel e
  // disputavam com os rotulos (captura da C9, 01/10).
  { float vr = 0.100f + ar * 0.05f, vg = 0.104f + ag * 0.05f, vb = 0.118f + ab * 0.05f;
    gfx_cor(R, raioPx / R.h, vr, vg, vb, 0.82f * A);
    gfx_anel(R, raioPx / R.h, 1.2f, 1.0f, 1.0f, 1.0f, 0.12f * A); }

  // Conteudo da pilula fechada: some no comeco da abertura.
  { float ap = A * (1.0f - s * 3.0f);
    if (ap > 0.01f) {
      // A seta "‹" fica solta sobre a arte: uma sombra escura de 1 px a
      // mantem legivel quando o fundo e claro (ceu, neve).
      TxtLinha seta = txt_linha(TXT_HEADLINE, "\xE2\x80\xB9", 245, 245, 248, 255);
      TxtLinha sombra = txt_linha(TXT_HEADLINE, "\xE2\x80\xB9", 0, 0, 0, 255);
      float cy = P.y + P.h * 0.5f;
      float sx = NV_MENU_PILULA_X + (NV_MENU_PILULA_SETA - seta.w) * 0.5f - 3.0f;
      float sy = cy - seta.h * 0.5f - 2.0f;
      txt_desenhar_alpha(sombra, sx + 1.0f, sy + 2.0f, ap * 0.35f);
      txt_desenhar_alpha(seta, sx, sy, ap * 0.90f);
      { GfxRect c = { P.x + 7.0f, cy - TV_PIL_CIRC * 0.5f, TV_PIL_CIRC, TV_PIL_CIRC };
        gfx_cor(c, 0.5f, 1.0f, 1.0f, 1.0f, 0.22f * ap);
        icone(destino, c.x + TV_PIL_CIRC * 0.5f, cy, 24.0f, 0.97f, 0.97f, 0.98f, ap); }
      { TxtLinha l = txt_linha(TXT_BODY, tvRotulo(destino), 245, 245, 248, 255);
        txt_desenhar_alpha(l, P.x + 7.0f + TV_PIL_CIRC + 14.0f, cy - l.h * 0.5f, ap); }
    } }

  // Conteudo do painel aberto, preso ao retangulo que cresce e deslocado pela
  // rolagem (tvRolar). Linha fora do painel nao desenha nem vira alvo.
  { float ac = (s - 0.22f) / 0.70f;
    float ys[NV_MENU_FOCOS_TV], yRot, f, topo;
    const int *pastas;
    int i, np = tvPastas(&pastas);
    // Foco: pilula BRANCA cheia, texto e icone escuros (como o original).
    const float FR = 0.95f, FG = 0.95f, FB = 0.96f;
    const int TINTA_FOCO = 22;
    if (ac <= 0.01f) return;
    if (ac > 1.0f) ac = 1.0f;
    tvLayout(ys, &yRot);
    topo = Q.y - tvRolar;
    gfx_recorte(R.x, R.y, R.w, R.h);

    // Cabecalho: avatar 44, nome 26 Medium, relogio 23 Regular cinza claro.
    // Focavel: e o "trocar de usuario".
    { float cyCab = topo + 50.0f;
      int emFoco, c, cRel;
      const ContaPerfil *p = perfis_item_ativo();
      char hora[8];
      GfxRect av = { Q.x + TV_PAD_X + TV_COL_CX - TV_AVATAR * 0.5f, cyCab - TV_AVATAR * 0.5f,
                     TV_AVATAR, TV_AVATAR };
      TxtLinha rel, nome;
      f = animFoco[MENU_RODAPE];
      if (f > 0.01f) {
        GfxRect pill = { Q.x + TV_PAD_X, cyCab - TV_PILULA_H * 0.5f, Q.w - TV_PAD_X * 2.0f, TV_PILULA_H };
        gfx_cor(pill, 0.5f, FR, FG, FB, f * ac);
      }
      emFoco = f > 0.5f;
      c = emFoco ? TINTA_FOCO : 240;
      cRel = emFoco ? 70 : 200;
      tvAvatar(av, ac);
      tvRelogio(hora, sizeof hora);
      rel = txt_linha(TXT_DET_META2, hora, cRel, cRel, cRel, 255);
      nome = txt_linha_corta(TXT_PG_RELOGIO, p ? p->nome : "Sua conta", c, c, c, 255,
                             Q.x + Q.w - 24.0f - rel.w - 16.0f - (Q.x + TV_PAD_X + TV_ROT_X));
      txt_desenhar_alpha(nome, Q.x + TV_PAD_X + TV_ROT_X, cyCab - nome.h * 0.5f, ac);
      txt_desenhar_alpha(rel, Q.x + Q.w - 24.0f - rel.w, cyCab - rel.h * 0.5f, ac);
      if (aberto && ponteiro_ativo() && cyCab - TV_PILULA_H * 0.5f >= Q.y - 1.0f)
        ponteiro_alvo(Q.x, cyCab - TV_PILULA_H * 0.5f, Q.w, TV_PILULA_H, ponteiroLinha, NULL, MENU_RODAPE, 0); }

    // Rotulo da secao de Streaming: 22 Regular, cinza medio, no recuo do icone.
    if (np && yRot >= 0.0f) {
      TxtLinha l = txt_linha(TXT_CAPTION, "Streaming", 150, 152, 160, 255);
      txt_desenhar_alpha(l, Q.x + TV_PAD_X + TV_COL_CX - TV_ICONE * 0.5f,
                         topo + yRot + TV_ROTULO_H - l.h - 6.0f, ac);
    }

    for (i = 0; i < MENU_N + np; i++) {
      int d = i < MENU_N ? TV_ORDEM[i] : MENU_ST0 + (i - MENU_N);
      int atual = (d == destino);
      float y, cy;
      GfxRect pill;
      if (ys[d] < 0.0f) continue;
      y = topo + ys[d];
      if (y + TV_LINHA_H < Q.y || y > Q.y + Q.h) continue;
      cy = y + TV_LINHA_H * 0.5f;
      pill = (GfxRect){ Q.x + TV_PAD_X, cy - TV_PILULA_H * 0.5f, Q.w - TV_PAD_X * 2.0f, TV_PILULA_H };
      f = animFoco[d];
      // ATUAL sem foco: so um veu claro bem leve, nada de pilula pesada.
      if (atual && f < 0.99f) gfx_cor(pill, 0.5f, 1.0f, 1.0f, 1.0f, 0.09f * (1.0f - f) * ac);
      if (f > 0.01f) gfx_cor(pill, 0.5f, FR, FG, FB, f * ac);
      { int emFoco = f > 0.5f;
        int c = emFoco ? TINTA_FOCO : 235;
        const char *rot;
        float ccx = pill.x + TV_COL_CX;
        if (d < MENU_ST0) {
          tvIcone(d, ccx, cy, f, ac);
          rot = tvRotulo(d);
        } else {
          const ColFolder *pf = col_folder(pastas[d - MENU_ST0]);
          tvCirculoPasta(pf, ccx, cy, ac);
          rot = pf ? pf->title : "";
        }
        { TxtLinha l = txt_linha_corta(TXT_DET_META, rot, c, c, c, 255,
                                       pill.w - TV_ROT_X - 16.0f);
          txt_desenhar_alpha(l, pill.x + TV_ROT_X, cy - l.h * 0.5f, ac); } }
      if (aberto && ponteiro_ativo() && y >= Q.y - 1.0f && y + TV_LINHA_H <= Q.y + Q.h + 1.0f)
        ponteiro_alvo(Q.x, y, Q.w, TV_LINHA_H, ponteiroLinha, NULL, d, 0);
    }
    gfx_sem_recorte();
  }
}
