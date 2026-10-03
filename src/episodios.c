#include "logotitulo.h"
#include "episodios.h"
#include "ajustes.h"   /* ajustes_acento: a cor do check da confirmacao */
#include "idioma.h"
#include "catalogo.h"
#include "descoberta.h"
#include "extras.h"
#include "gfx.h"
#include "tex_cache.h"
#include "text.h"
#include "layout.h"
#include "anim.h"
#include "vistoep.h"
#include "visto.h"
#include "botoes.h"
#include <stdio.h>
#include <string.h>
#include "ponteiro.h"
#include "plrui.h"
#include "progresso.h"

// A FOLHA DAS FONTES NO PLAYER (Glass UI, mockup de 03/10): 820 de largura,
// flutuando a 40 das bordas, raio 36. A lista comeca em EP_TOP e termina no
// rodape (EP_PE); cada linha tem a still 240x135 com recuo de 14.
#define EP_W 820.0f
#define EP_MARGEM 40.0f
#define EP_RAIO 36.0f
#define EP_ROW 167.0f
#define EP_TOP 248.0f
#define EP_PE  (NV_TELA_H - EP_MARGEM - 62.0f)
static int aberto, titulo, atualT, atualE, temporada, foco, grupo;
static int pedidoT, pedidoE;
static float anim, scroll;
// Velocidade da rolagem de 2a ordem (anim_mola2): partida macia, como na home.
static float velScroll;
static int localizarAtual;
// ONDE O FOCO TEM DE ESTAR, POR NUMERO DE EPISODIO — issue #102.
//
// `foco` e uma LINHA, e linha depende da lista existir. Quando o catalogo
// remonta com a folha aberta (cat_definir_tudo zera nEps de proposito, e a
// descoberta republica sozinha de tempos em tempos), a lista fica vazia por
// alguns quadros: o clamp la embaixo puxava `foco` para 0 e, quando os
// episodios voltavam, a folha estava no primeiro da temporada. E exatamente o
// "volta sozinho para o E1 se a pessoa demorar" do relato — o que segura a
// pessoa parada e o menu de visto por cima, nao ele que causa o salto.
//
// Guardando o NUMERO do episodio o foco sobrevive ao desaparecimento da lista:
// quando ela volta, a linha e reencontrada.
static int alvoE;
// A ROLAGEM DO PRIMEIRO QUADRO E A FINAL, sem mola — a outra metade do #102.
// A folha nascia com scroll=0 e a mola levava ate o episodio atual: para quem
// olha, ela "carrega no E1 e depois salta". Abrir ja no lugar nao e animacao
// mais rapida, e animacao nenhuma.
static int semMolaScroll;

// --- MENU DE VISTO -----------------------------------------------------------
//
// Segurar OK numa linha abre. Os tres gestos que o dono pediu — este episodio,
// ate aqui, a temporada inteira — sao O MESMO LOTE em tamanhos diferentes
// (vistoep.h), e o sentido (marcar ou desmarcar) sai do estado do episodio em
// foco: quem esta olhando um episodio visto quer desmarcar.
// VM_FONTES so aparece quando o menu e aberto DE FORA (da pagina de detalhe):
// la o toque curto no card ja abre as fontes e a pressao longa passou a abrir
// este menu, entao ele precisa carregar a porta que tomou. Dentro da folha de
// episodios a opcao nao existe — quem abriu a folha veio do player e ja tem
// fonte tocando; oferece-la ali seria uma linha que nao leva a lugar nenhum.
enum { VM_ESTE = 0, VM_ATE, VM_TEMP, VM_FONTES, VM_N };
// O MENU DA TEMPORADA (issue #108, "Pressing 'Season' brings up option to mark
// all as watched"): o mesmo cartao, aberto pela ABA e nao por um episodio, com
// so as duas linhas que o dono pediu. vmModoTemp escolhe qual dos dois; as
// opcoes sao indices proprios porque nao ha "este" nem "ate aqui" numa aba.
enum { VT_MARCAR = 0, VT_DESMARCAR, VT_N };
static int vmModoTemp;
static int vmQuantos[VM_N];   // tamanho do lote de cada opcao, da abertura
static int montarLote(int idx, int modo, int t, int e, VistoPar *saida, int max);
// Teto do lote de um gesto. 64 era o antigo e cortava temporada de anime.
#define VM_LOTE 256
static int vmAberto, vmFoco, vmVisto;      // vmVisto: o sentido do gesto
static Uint32 vmDesde;                     // relogio da pressao longa
static int vmSegurando, vmConsumir;
// CONFIRMACAO NA TELA DEPOIS DE APLICAR: o menu nao some no mesmo quadro; por
// FEITO_MS ele mostra um check e "N episodios marcados" e so entao fecha. Sem
// isto a unica prova de que o OK entrou era procurar o "Visto" na lista, e o
// #70 mostrou que a pessoa apertava de novo.
#define FEITO_MS 900
static int vmFeito, vmFeitoN, vmFeitoVisto; static Uint32 vmFeitoAte;

// O ALVO DO MENU E EXPLICITO, e nao lido do estado da folha.
//
// Ele dependia de `titulo`, `foco`, `temporada` e epLinha(foco) — tudo interno
// desta tela —, e por isso so podia existir dentro dela. Guardando o alvo o
// menu passa a servir tambem a pagina de detalhe, onde a folha nem esta aberta.
static int  vmIdx = -1, vmT, vmE;
static char vmNome[96];
// O TITULO, e nao so a posicao (#190). `titulo` e `vmIdx` sao indices no
// catalogo, e o catalogo e refeito com a folha aberta — a refacao de
// "Continuar assistindo" roda com o player tocando. A mesma posicao passava a
// ser de outra serie, e a folha listava os episodios dela ("Attack on Titan
// trocado por Knights of Guinevere no menu de episodios"). Guardado na
// abertura e conferido por revalidar() antes de cada uso.
static char idTitulo[64], vmId[64];
static void guardarId(char *dst, size_t tam, int idx) {
  const CatItem *ci = cat_item(idx);
  snprintf(dst, tam, "%s", ci ? ci->imdb : "");
}
static void revalidar(void) {
  int i;
  if (idTitulo[0] && (i = cat_indice_vivo(titulo, idTitulo)) >= 0) titulo = i;
  if (vmId[0] && (i = cat_indice_vivo(vmIdx, vmId)) >= 0) vmIdx = i;
}
// A MINIATURA DO EPISODIO, resolvida UMA vez na abertura. O cartao mostra a
// arte do episodio de que ele fala — sem ela o menu e quatro linhas de texto
// que poderiam ser de qualquer titulo. Procurar no catalogo a cada quadro
// custaria uma varredura por episodio 60 vezes por segundo para desenhar uma
// imagem que nao muda enquanto o menu estiver aberto.
static char vmThumb[512];   // = CatEp.thumb e CatItem.backdrop; 400 cortava URL longa
static int  vmSo;        // 1 = aberto sozinho, sobre outra tela
static int  vmFontesPed; // consumido por episodios_menu_pediu_fontes()

static int vmOpcoes(void) { return vmModoTemp ? VT_N : vmSo ? VM_N : VM_N - 1; }

// Abre o menu para um episodio qualquer. `t` e o NUMERO da temporada, nao o
// indice da aba: quem chama de fora nao tem abas.
static void menuAbrir(int idx, int t, int e, const char *nome, int so) {
  const CatItem *ci = cat_item(idx);
  if (!ci || !ci->imdb[0]) return;
  vmIdx = idx; vmT = t; vmE = e; vmSo = so; vmModoTemp = 0;
  guardarId(vmId, sizeof vmId, idx);
  snprintf(vmNome, sizeof vmNome, "%s", nome ? nome : "");
  vmThumb[0] = 0;
  { int i;
    for (i = 0; i < cat_n_episodios(idx); i++) {
      const CatEp *ce = cat_episodio(idx, i);
      if (ce && ce->temporada == t && ce->episodio == e) {
        snprintf(vmThumb, sizeof vmThumb, "%s", ce->thumb);
        break;
      }
    } }
  // O SENTIDO SAI DO ESTADO: quem esta olhando um episodio visto quer
  // desmarcar. Desconhecido (-1) conta como nao visto.
  vmVisto = vistoep_estado(ci->imdb, t, e) == 1 ? 0 : 1;
  // vmConsumir FICA EM ZERO. O menu abre no KEYUP da pressao longa — o OK que
  // o abriu ja foi SOLTO quando se chega aqui — e com 1 o proximo KEYDOWN, que
  // e a escolha de verdade, era engolido como se fosse esse soltar. Era o
  // "precisa apertar duas vezes em toda opcao" do #70, desde o dia em que o
  // menu nasceu; a regra de ctxmenu.c que isto copiava abre no KEYDOWN, e la
  // consumir o soltar faz sentido.
  vmAberto = 1; vmFoco = 0; vmConsumir = 0; vmFontesPed = 0;
  vmFeito = 0;
  vmQuantos[VM_ESTE] = 1;
  vmQuantos[VM_ATE]  = montarLote(idx, VM_ATE, t, e, NULL, 0);
  vmQuantos[VM_TEMP] = montarLote(idx, VM_TEMP, t, e, NULL, 0);
  vmQuantos[VM_FONTES] = 0;
}

static int nTemporadas(void) {
  const CatItem *c = cat_item(titulo);
  return c && c->nTemporadas > 0 ? c->nTemporadas : 1;
}
static int numTemporada(int i) {
  const CatItem *c = cat_item(titulo);
  return c && c->nTemporadas > 0 ? c->temporadas[i] : atualT;
}
static const CatEp *epLinha(int linha) {
  int n = cat_n_episodios(titulo);
  for (int i = 0, j = 0; i < n; i++) {
    const CatEp *ep = cat_episodio(titulo, i);
    if (ep && ep->temporada == numTemporada(temporada) && j++ == linha) return ep;
  }
  return NULL;
}
static int nLinhas(void) {
  int n = 0;
  for (int i=0;i<cat_n_episodios(titulo);i++) {
    const CatEp *e=cat_episodio(titulo,i);
    if(e && e->temporada==numTemporada(temporada)) n++;
  }
  return n;
}
// O LOTE SAI DO CATALOGO E DO MAPA, JUNTOS.
//
// Antes saia so do mapa (vistoep_temporada / vistoep_ate_aqui), e o mapa so
// ENUMERA a serie quando o Trakt respondeu /shows/<id>/progress/watched. Sem
// Trakt ele so conhece o que a conta disse que foi visto — entao "Temporada
// inteira" mostrava "(0 episodios)" e o OK nao fazia nada. Era a metade
// episodica do "sem o traktv nao ta dando o watched".
//
// O catalogo (Cinemeta `videos`) lista os episodios que existem; o mapa
// acrescenta o que o Trakt conhece e o catalogo nao (especiais). Episodio que
// AINDA NAO FOI AO AR fica fora (a agenda do TMDB da visita, a mesma regra de
// epNaoExibido em detail.c): marcar como visto o que nao saiu e o erro que o
// Trakt aceita calado. Temporada 0 do catalogo fica fora do "ate aqui" — o
// especial so entra se o mapa o trouxer, como ja era.
//
// `saida` NULA = so contar, como em vistoep.h; o rotulo precisa do numero.
// A regra mora em vistoep_lote (pura, com teste); aqui so se junta o catalogo.
static int montarLote(int idx, int modo, int t, int e, VistoPar *saida, int max) {
  static VistoPar cat[VM_LOTE * 4];
  const CatItem *ci = cat_item(idx);
  int i, nc = 0;
  if (!ci || !ci->imdb[0]) return 0;
  if (modo == VM_ESTE) {
    if (saida && max > 0) { saida[0].temporada = (short)t; saida[0].episodio = (short)e; }
    return 1;
  }
  for (i = 0; i < cat_n_episodios(idx) && nc < VM_LOTE * 4; i++) {
    const CatEp *ce = cat_episodio(idx, i);
    if (!ce) continue;
    cat[nc].temporada = (short)ce->temporada;
    cat[nc].episodio = (short)ce->episodio;
    nc++;
  }
  return vistoep_lote(ci->imdb, modo == VM_ATE, t, e, cat, nc,
                      extras_agenda_temporada(), extras_agenda_episodio(),
                      saida, max);
}

// Aplica o gesto: monta o lote, muda o local na hora, e manda o resto para um
// fio (visto.c — Trakt, Simkl e conta, o que estiver vinculado). Devolve 0
// quando nao ha nada a fazer — e o caso de marcar o que ja esta marcado, que
// nao deve gastar uma requisicao.
//
// QUANDO ALGO MUDOU, O LOTE INTEIRO VAI, e nao so o que mudou localmente: o
// mapa pode dizer "visto" por um destino (Trakt) e o outro (Simkl) nao saber.
// Um pedido por destino, com todos os episodios — nunca um por episodio. Se
// nada mudou no local, nada sai (a regra de antes; o menu diz "ja estava").
static int aplicarVisto(int modo, int visto) {
  const CatItem *ci = cat_item(vmIdx);
  VistoPar lote[VM_LOTE];
  int n, mudou;
  if (!ci || !ci->imdb[0]) return 0;
  // A TEMPORADA DO EPISODIO (vmT), e nao a da aba selecionada. Sao a mesma
  // coisa dentro da folha, e fora dela nao ha aba nenhuma.
  n = montarLote(vmIdx, modo, vmT, vmE, lote, VM_LOTE);
  if (n < 1) return 0;
  mudou = vistoep_marcar_lote(ci->imdb, lote, n, visto);
  if (!mudou) return 0;
  visto_episodios(ci->imdb, ci->tipo[0] ? ci->tipo : "series", lote, n, visto,
                  visto_destinos());
  return mudou;
}

// Abre o menu da TEMPORADA `t` (numero, nao indice de aba). O foco nasce em
// "Desmarcar" so quando a temporada inteira ja esta vista: e o unico caso em
// que marcar nao mudaria nada.
static void menuAbrirTemporada(int idx, int t, int so) {
  const CatItem *ci = cat_item(idx);
  VistoPar lote[VM_LOTE];
  int n, i, vistos = 0;
  if (!ci || !ci->imdb[0] || t < 0) return;
  menuAbrir(idx, t, 0, ci->titulo, so);
  if (!vmAberto) return;
  vmModoTemp = 1;
  snprintf(vmThumb, sizeof vmThumb, "%s", ci->backdrop);
  n = montarLote(idx, VM_TEMP, t, 0, lote, VM_LOTE);
  for (i = 0; i < n; i++)
    if (vistoep_estado(ci->imdb, lote[i].temporada, lote[i].episodio) == 1) vistos++;
  vmFoco = (n > 0 && vistos == n) ? VT_DESMARCAR : VT_MARCAR;
}

void episodios_abrir(int idx, int t, int e) {
  titulo = idx; atualT = t; atualE = e; aberto = 1;
  guardarId(idTitulo, sizeof idTitulo, idx);
  temporada = foco = 0; grupo = 1; pedidoE = 0; scroll = 0;
  vmAberto = 0; vmSegurando = 0; vmConsumir = 0;
  localizarAtual = 1; alvoE = e; semMolaScroll = 1;
  for (int i = 0; i < nTemporadas(); i++) if (numTemporada(i) == t) temporada = i;
  for (int i = 0; i < nLinhas(); i++) if (epLinha(i)->episodio == e) foco = i;
  desc_episodios(titulo, t);
}
int episodios_aberto(void) { return aberto; }
float episodios_anim(void) { return anim; }
int   episodios_foco_linha(void) { return foco; }
float episodios_rolagem(void) { return scroll; }
void episodios_fechar(void) { aberto = 0; }
int  episodios_titulo(void) { revalidar(); return titulo; }
int episodios_escolheu(int *t, int *e) {
  if (!pedidoE) return 0;
  *t = pedidoT; *e = pedidoE; pedidoE = 0; return 1;
}
// O menu em si, separado de quem o hospeda: a folha chama daqui, e a pagina de
// detalhe chama por episodios_menu_evento(). As duas regras abaixo o menu do
// cartaz aprendeu do jeito dificil (ver ctxmenu.c): repeticao automatica nunca
// e uma segunda escolha, e o OK que ABRIU o menu nao escolhe nada.
// O MENU, DESENHADO ONDE MANDAREM. Dentro da folha ele e centrado sobre a
// COLUNA dela (centrar em NV_TELA_W poria o menu sobre o vazio da esquerda,
// longe do episodio a que se refere); aberto sobre a pagina de detalhe, a
// caixa e a tela inteira. A altura sai do numero de opcoes, que muda com isso.
// PONTEIRO (#99) no menu de visto: opcao sob o cursor ganha o foco (vmFoco,
// o mesmo de cima/baixo), clique e o OK, clique fora fecha o menu.
static void ponteiroVmOpcao(int i, int b) { (void)b; if (i >= 0 && i < vmOpcoes()) vmFoco = i; }
static void ponteiroVmFora(int a, int b) { (void)a; (void)b; vmAberto = 0; }

// O MENU DE VISTO (segurar OK) vira uma ILHA MODAL: still + kicker +
// "T1E4 · nome", e as acoes em linhas de folha (foco = superficie), nao
// pilulas empilhadas. Sobre a folha ele se alinha a ela; sozinho (detalhe),
// fica no centro da tela.
static void menuDesenhar(float x, float larg, float anim) {
  const float PAD = 44.0f, MW = 800.0f, TH_W = 224.0f, TH_H = 126.0f, OPT_H = 62.0f, OPT_PASSO = 66.0f;
  float mh, optTop;
  char cab[160];
  int i;
  if (!vmAberto) return;
  optTop = PAD + TH_H + 26.0f;
  mh = optTop + (float)vmOpcoes() * OPT_PASSO - 4.0f + 22.0f + 30.0f + PAD;
  if (vmFeito) mh = optTop + 60.0f + PAD;
  { GfxRect m = { x + (larg - MW) * 0.5f, 230.0f, MW, mh };
    if (vmSo) m.y = (NV_TELA_H - mh) * 0.5f;
    if (ponteiro_ativo() && anim > .5f) {
      ponteiro_camada();
      if (!vmFeito) ponteiro_alvo(0, 0, NV_TELA_W, NV_TELA_H, NULL, ponteiroVmFora, 0, 0);
      ponteiro_alvo(m.x, m.y, m.w, m.h, NULL, NULL, 0, 0);
    }
    gfx_cor((GfxRect){ 0, 0, NV_TELA_W, NV_TELA_H }, 0, 0, 0, 0, (ajustes_vidro() ? .40f : .42f) * anim);
    plrui_material(m, 36.0f, 1, anim);
    if (ajustes_vidro()) gfx_cor(m, 36.0f / m.h, 0.055f, 0.059f, 0.071f, 0.55f * anim);   // .94 no mockup
    { float tx = m.x + PAD, tw = MW - PAD * 2.0f;
      GLuint th = vmThumb[0] ? tex_obter_larg(vmThumb, TH_W) : 0;
      // O cabecalho mostra o still do MESMO episodio: desfocado pela mesma
      // regra da lista (#133), senao o menu entregava o que ela esconde.
      if (th && !vmModoTemp && ajustes_desfocar_nao_assistidos()) {
        const CatItem *cv = cat_item(vmIdx);
        if (!(cv && vistoep_estado(cv->imdb, vmT, vmE) == 1)) th = gfx_desfocado(th, vmThumb);
      }
      if (th) {
        gfx_tex_aspect_atual = tex_aspecto(vmThumb);
        gfx_rect((GfxRect){ m.x + PAD, m.y + PAD, TH_W, TH_H }, th, GFX_CARD, 0, 0, 0, 16.0f / TH_H, 0, 0, 0, anim);
        gfx_tex_aspect_atual = 0.0f;
        tx += TH_W + 26.0f; tw -= TH_W + 26.0f;
      }
      { float yc = m.y + PAD + TH_H * 0.5f;
        if (vmModoTemp) {
          char sob[48];
          snprintf(sob, sizeof sob, i18n("Temporada %d"), vmT);
          plrui_kicker(sob, tx, yc - 32.0f, 243, 242, 239, anim * 0.45f);
          logotitulo_desenhar(cat_item(vmIdx), vmNome, TXT_HEADLINE, tx, yc - 6.0f,
                              tw < 360.0f ? tw : 360.0f, 50.0f, tw, anim);
        } else {
          plrui_kicker(vmVisto ? "Marcar como assistido" : "Desmarcar como assistido",
                       tx, yc - 32.0f, 243, 242, 239, anim * 0.45f);
          snprintf(cab, sizeof cab, i18n("T%dE%d · %s"), vmT, vmE, vmNome);
          txt_desenhar_alpha(txt_linha_corta(TXT_G30B, cab, 243, 242, 239, 255, tw), tx, yc - 6.0f, anim);
        } } }
    if (vmFeito) {
      float ar, ag, ab;
      char fr[120];
      int tinta = plrui_tinta();
      GfxRect ck = { m.x + PAD, m.y + optTop + 8.0f, 44.0f, 44.0f };
      ajustes_acento(&ar, &ag, &ab);
      gfx_cor((GfxRect){ ck.x - 8.0f, ck.y - 8.0f, 60.0f, 60.0f }, 0.5f, ar, ag, ab, anim);
      gfx_icone(ck, "pl_check", tinta / 255.0f, tinta / 255.0f, tinta / 255.0f, anim);
      if (vmFeitoN < 1) snprintf(fr, sizeof fr, "%s", i18n("Nada a mudar: já estava assim"));
      else if (vmFeitoVisto) snprintf(fr, sizeof fr, vmFeitoN == 1 ? i18n("%d episódio marcado como assistido") : i18n("%d episódios marcados como assistidos"), vmFeitoN);
      else snprintf(fr, sizeof fr, vmFeitoN == 1 ? i18n("%d episódio desmarcado") : i18n("%d episódios desmarcados"), vmFeitoN);
      txt_desenhar_alpha(txt_linha_corta(TXT_G23B, fr, 243, 242, 239, 255, MW - PAD * 2.0f - 72.0f),
                         m.x + PAD + 72.0f, m.y + optTop + 16.0f, anim);
      if (SDL_GetTicks() >= vmFeitoAte) { vmAberto = 0; vmFeito = 0; }
      return;
    }
    for (i = 0; i < vmOpcoes(); i++) {
      GfxRect r = { m.x + PAD, m.y + optTop + (float)i * OPT_PASSO, MW - 2.0f * PAD, OPT_H };
      int f = i == vmFoco, quantos;
      const char *ic;
      char rot[120];
      if (anim > .5f) ponteiro_alvo(r.x, r.y, r.w, r.h, ponteiroVmOpcao, NULL, i, 0);
      // O NUMERO NO ROTULO: "marcar 7 episodios" e outra decisao que "marcar
      // 1". Sai do MESMO montarLote da acao, contado uma vez na abertura.
      quantos = vmModoTemp ? vmQuantos[VM_TEMP] : (i < VM_FONTES ? vmQuantos[i] : 0);
      if (vmModoTemp) {
        snprintf(rot, sizeof rot, i == VT_MARCAR ? i18n("Marcar temporada como assistida (%d)")
                                                 : i18n("Desmarcar temporada (%d)"), quantos);
        ic = i == VT_MARCAR ? "pl_eye" : "pl_eye-off";
      } else if (i == VM_ESTE) {
        snprintf(rot, sizeof rot, "%s", vmVisto ? i18n("Marcar este episódio") : i18n("Desmarcar este episódio"));
        ic = vmVisto ? "pl_eye" : "pl_eye-off";
      } else if (i == VM_ATE) {
        snprintf(rot, sizeof rot, quantos == 1 ? i18n("Até aqui (%d episódio)") : i18n("Até aqui (%d episódios)"), quantos);
        ic = "pl_skip-forward";
      } else if (i == VM_TEMP) {
        snprintf(rot, sizeof rot, quantos == 1 ? i18n("Temporada inteira (%d episódio)") : i18n("Temporada inteira (%d episódios)"), quantos);
        ic = "pl_list-video";
      } else {
        snprintf(rot, sizeof rot, "%s", i18n("Fontes deste episódio"));
        ic = "pl_layers";
      }
      if (f) plrui_linha_foco(r, 22.0f, anim);
      gfx_icone((GfxRect){ r.x + 20.0f, r.y + (OPT_H - 26.0f) * 0.5f, 26.0f, 26.0f }, ic, 1, 1, 1, (f ? 0.95f : 0.6f) * anim);
      { TxtLinha l = txt_linha_corta(TXT_G23B, rot, 243, 242, 239, f ? 255 : 219, r.w - 84.0f);
        txt_desenhar_alpha(l, r.x + 20.0f + 26.0f + 18.0f, r.y + (OPT_H - (float)l.h) * 0.5f, anim); }
    }
    { const char *k[3] = { "\xe2\x86\x91 \xe2\x86\x93", "OK", "Voltar" };
      const char *rt[3] = { "Escolher", "Aplicar", "Fechar" };
      plrui_dicas(k, rt, 3, m.x + PAD, m.y + optTop + (float)vmOpcoes() * OPT_PASSO - 4.0f + 22.0f + 15.0f, 0, anim); } }
}

static void menuEvento(const SDL_Event *ev) {
  SDL_Keycode ko = ev->key.keysym.sym;
  int ehOk = (ko == SDLK_RETURN || ko == SDLK_KP_ENTER || ko == SDLK_SPACE);
  if (ev->type == SDL_KEYUP && ehOk) { vmConsumir = 0; return; }
  if (ev->type != SDL_KEYDOWN) return;
  if (ehOk && (ev->key.repeat || vmConsumir)) return;
  if (ko == SDLK_UP)   { if (vmFoco > 0) vmFoco--; return; }
  if (ko == SDLK_DOWN) { if (vmFoco < vmOpcoes() - 1) vmFoco++; return; }
  if (vmFeito) { vmAberto = 0; vmFeito = 0; return; }   // qualquer tecla encerra a confirmacao
  if (ehOk) {
    if (vmModoTemp) {
      int v = vmFoco == VT_MARCAR;
      vmFeitoN = aplicarVisto(VM_TEMP, v);
      vmFeitoVisto = v;
      vmFeito = 1; vmFeitoAte = SDL_GetTicks() + FEITO_MS;
      return;
    }
    if (vmFoco == VM_FONTES) { vmFontesPed = 1; vmAberto = 0; return; }
    vmFeitoN = aplicarVisto(vmFoco, vmVisto);
    vmFeitoVisto = vmVisto;
    vmFeito = 1; vmFeitoAte = SDL_GetTicks() + FEITO_MS;
    return;
  }
  vmAberto = 0;   // qualquer outra tecla fecha
}

void episodios_evento(const SDL_Event *ev) {
  if (!aberto) return;
  revalidar();
  { SDL_Keycode ko = ev->key.keysym.sym;
    int ehOk = (ko == SDLK_RETURN || ko == SDLK_KP_ENTER || ko == SDLK_SPACE);

    // O MENU COME A TECLA ENQUANTO ESTA NO AR. Mesma regra do menu do cartaz:
    // ele e a coisa mais recente na tela e e para ela que a pessoa esta
    // olhando.
    if (vmAberto) { menuEvento(ev); return; }

    // PRESSAO LONGA NA ABA DA TEMPORADA (issue #108): abre o menu da temporada.
    // O toque curto continua descendo para a lista, e passou para o KEYUP pelo
    // mesmo motivo da linha de episodio abaixo — so o soltar conhece a duracao.
    if (ehOk && grupo == 0) {
      if (ev->type == SDL_KEYDOWN && !ev->key.repeat) {
        vmSegurando = 1; vmDesde = SDL_GetTicks(); return;
      }
      if (ev->type == SDL_KEYUP) {
        int foiAqui = vmSegurando;
        Uint32 dur = foiAqui ? SDL_GetTicks() - vmDesde : 0;
        vmSegurando = 0;
        if (!foiAqui) return;
        if (dur >= NV_HOLD_MS) { menuAbrirTemporada(titulo, numTemporada(temporada), 0); return; }
        grupo = 1;
        if (!nLinhas()) desc_episodios(titulo, numTemporada(temporada));
        return;
      }
    }
    // PRESSAO LONGA SOBRE UMA LINHA DE EPISODIO. So no grupo da lista: em cima
    // do cabecalho nao ha episodio para marcar.
    if (ehOk && grupo == 1) {
      if (ev->type == SDL_KEYDOWN && !ev->key.repeat) {
        vmSegurando = 1; vmDesde = SDL_GetTicks(); return;
      }
      if (ev->type == SDL_KEYUP) {
        // A GUARDA OLHA SE FOI PRESSIONADO AQUI, e nao a duracao. Soltar sem
        // ter pressionado nesta camada nao e clique — a de cima pode ter
        // fechado no KEYDOWN e o KEYUP vazar para ca; e a mesma guarda que
        // home.c, detail.c e ctxmenu.c ja tem, pelo mesmo defeito.
        //
        // TESTAR `dur != 0` NO LUGAR DISSO ENGOLE O TOQUE RAPIDO: descida e
        // subida no mesmo milissegundo dao dur=0, que e legitimo. Foi assim na
        // primeira versao, e tests/player.sh pegou na primeira rodada.
        int foiAqui = vmSegurando;
        Uint32 dur = foiAqui ? SDL_GetTicks() - vmDesde : 0;
        vmSegurando = 0;
        if (!foiAqui) return;
        if (dur >= NV_HOLD_MS) {
          const CatEp *e2 = epLinha(foco);
          if (e2) menuAbrir(titulo, e2->temporada, e2->episodio, e2->nome, 0);
          return;
        }
        // TOQUE CURTO ABRE O EPISODIO, e a acao acontece AQUI e nao no ramo
        // antigo la embaixo: o KEYUP e o unico momento que conhece a DURACAO, e
        // e ela que separa abrir de segurar. Deixar isso para o KEYDOWN faria a
        // pressao longa nunca existir, porque o episodio ja teria aberto.
        { const CatEp *e2 = epLinha(foco);
          if (e2) {
            if (e2->temporada != atualT || e2->episodio != atualE) {
              pedidoT = e2->temporada; pedidoE = e2->episodio;
            }
            aberto = 0;
          } }
        return;
      }
    }
    if (ev->type == SDL_KEYUP) { vmSegurando = 0; return; }
  }
  if (ev->type != SDL_KEYDOWN) return;
  SDL_Keycode k = ev->key.keysym.sym;
  // O OK JA FOI DECIDIDO NO KEYUP acima (e o unico jeito de conhecer a
  // DURACAO). Deixar o KEYDOWN cair no ramo antigo abriria o episodio antes de
  // a pressao longa poder existir.
  if ((k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE) && grupo >= 0)
    return;
  if(k==SDLK_r) { desc_episodios(titulo,numTemporada(temporada)); return; }
  if (k == SDLK_ESCAPE || k == SDLK_BACKSPACE || k == SDLK_DELETE || k == SDLK_AC_BACK) {
    aberto = 0; return;
  }
  int nt = nTemporadas(), n = nLinhas();
  if (k == SDLK_UP) { if (grupo == 1 && foco > 0) foco--; else grupo--; }
  if (k == SDLK_DOWN) { if (grupo < 1) grupo++; else if (foco < n - 1) foco++; }
  if (grupo < -1) grupo = -1;
  if (grupo == 0 && (k == SDLK_LEFT || k == SDLK_RIGHT)) {
    int nova = temporada + (k == SDLK_RIGHT ? 1 : -1);
    if (nova >= 0 && nova < nt) {
      temporada = nova; foco = 0; scroll = 0;
      // TROCAR DE ABA E UM PEDIDO EXPLICITO de recomecar a lista: aqui o topo
      // e o lugar certo, e o alvo passa a ser o primeiro episodio da temporada
      // nova (o sincronizador em episodios_atualizar o escreve no proximo
      // quadro, quando a lista ja e a dela).
      localizarAtual = 0; semMolaScroll = 1;
      desc_episodios(titulo, numTemporada(temporada));
    }
  }
  if (k == SDLK_RETURN || k == SDLK_KP_ENTER) {
    if (grupo == -1) aberto = 0;
    else if (grupo == 0) {
      grupo = 1;
      if (!n) desc_episodios(titulo,numTemporada(temporada));
    }
    else {
      const CatEp *ep = epLinha(foco);
      if (ep) {
        if (ep->temporada != atualT || ep->episodio != atualE) {
          pedidoT = ep->temporada; pedidoE = ep->episodio;
        }
        aberto = 0;
      }
    }
  }
}
void episodios_atualizar(float dt) {
  anim = anim_mola(anim, aberto ? 1 : 0, dt, NV_MOLA_TELA);
  if(!aberto && anim<.005f) return;
  revalidar();
  desc_episodios_pendente();
  int n = nLinhas();
  // A LISTA SUMIU: NAO E "VOLTE PARA O COMECO", E "ESPERE" — issue #102.
  // Rearma a localizacao em vez de deixar o clamp abaixo zerar o foco; quem
  // sabe onde a pessoa estava e alvoE, e ele nao depende da lista.
  if (!n) localizarAtual = 1;
  else if (localizarAtual) {
    for (int i = 0; i < n; i++) {
      const CatEp *l = epLinha(i);
      if (l && l->episodio == alvoE) { foco = i; break; }
    }
    // SEM MOLA no quadro em que a linha e encontrada: a lista chegou depois da
    // folha abrir, e animar daqui e o salto que o relato descreve.
    // A PRIMEIRA LINHA E A ANTERIOR A QUE TOCA (mockup do Glass UI): a pessoa
    // ve de onde veio e o que vem, e o foco fica na segunda linha.
    localizarAtual = 0; semMolaScroll = 1;
    scroll = foco > 0 ? (float)(foco - 1) * EP_ROW : 0.0f;
  }
  if (foco >= n) foco = n > 0 ? n - 1 : 0;
  // O ALVO SEGUE O FOCO enquanto a lista existe — e assim que andar com o
  // direcional (ou trocar de aba) atualiza o que sera reencontrado depois.
  if (n) { const CatEp *l = epLinha(foco); if (l) alvoE = l->episodio; }
  float area = EP_PE - EP_TOP;
  float max = n * EP_ROW - area;
  float alvo = scroll;
  if(foco*EP_ROW<scroll) alvo=foco*EP_ROW;
  if((foco+1)*EP_ROW>scroll+area) alvo=(foco+1)*EP_ROW-area;
  if (alvo > max) alvo = max;
  if (alvo < 0) alvo = 0;
  if (semMolaScroll) { scroll = alvo; velScroll = 0.0f; }
  else scroll = anim_mola2(&velScroll, scroll, alvo, dt, NV_MOLA2_SCROLL);
  semMolaScroll = 0;
}
// PONTEIRO (#99) na folha: linha e "Fechar" focam como as setas; a aba de
// temporada troca so no CLIQUE (trocar ao passar por cima recarregaria a lista
// debaixo da mao). Clicar fora do painel fecha.
static void ponteiroEpLinha(int i, int b) { (void)b; if (i >= 0 && i < nLinhas()) { grupo = 1; foco = i; } }
static void ponteiroEpFechar(int a, int b) { (void)a; (void)b; grupo = -1; }
static void ponteiroEpFora(int a, int b) { (void)a; (void)b; aberto = 0; }
static void ponteiroEpTemporada(int i, int b) {
  (void)b;
  if (i < 0 || i >= nTemporadas()) return;
  grupo = 0;
  if (i == temporada) return;
  temporada = i; foco = 0; scroll = 0;
  localizarAtual = 0; semMolaScroll = 1;
  desc_episodios(titulo, numTemporada(temporada));
}

// A FOLHA: a mesma ilha das Fontes (streams.c) — veu de 30/42%, sombra curta,
// miolo de vidro ou solido, luz no canto de cima, SEM contorno. Cabecalho com
// kicker (a serie) + "Episodios" e o disco de fechar; as temporadas no
// segmentado com a contagem; cada linha com a still (barra de progresso e o
// disco de assistido sobre ela), "EPISODIO n", o nome e a linha de estado; so
// a focada abre a sinopse. O que esta tocando leva o equalizador no acento.
static void equalizador(float x, float yb, float a) {
  float ar, ag, ab;
  static const float H[3] = { 6.0f, 12.0f, 8.0f };
  int i;
  ajustes_acento(&ar, &ag, &ab);
  for (i = 0; i < 3; i++) gfx_cor((GfxRect){ x + i * 5.0f, yb - H[i], 3.0f, H[i] }, 0.0f, ar, ag, ab, a);
}

void episodios_desenhar(void) {
  if (anim < .005f) return;
  revalidar();
  float x = NV_TELA_W - EP_W - EP_MARGEM + (1 - anim) * (EP_W + EP_MARGEM);
  float lx = x + 26.0f, lw = EP_W - 52.0f;   // a coluna das linhas
  const int vid = ajustes_vidro();
  const CatItem *ci = cat_item(titulo);
  int ptr = aberto && anim > .5f && !vmAberto && ponteiro_ativo();
  GfxRect corpo = { x, EP_MARGEM, EP_W, NV_TELA_H - 2.0f * EP_MARGEM };
  gfx_cor((GfxRect){ 0, 0, NV_TELA_W, NV_TELA_H }, 0, 0, 0, 0, (vid ? .30f : .42f) * anim);
  plrui_material(corpo, EP_RAIO, 0, anim);
  if (ptr) {
    ponteiro_alvo(0, 0, x, NV_TELA_H, NULL, ponteiroEpFora, 0, 0);
    ponteiro_alvo(x, 0, EP_W, NV_TELA_H, NULL, NULL, 0, 0);
    ponteiro_alvo(x + EP_W - 48.0f - 56.0f, 92.0f, 56.0f, 56.0f, ponteiroEpFechar, NULL, 0, 0);
  }
  // CABECALHO: kicker com a serie, "Episodios" 40/700; o fechar e um disco.
  plrui_kicker(ci ? ci->titulo : "", lx + 22.0f, 80.0f, 243, 242, 239, anim * 0.45f);
  txt_desenhar_alpha(txt_linha(TXT_ILHA_TITULO, "Episódios", 243, 242, 239, 255), lx + 22.0f, 100.0f, anim);
  { GfxRect d = { x + EP_W - 48.0f - 56.0f, 92.0f, 56.0f, 56.0f };
    int f = grupo == -1;
    float k = f ? plrui_tinta() / 255.0f : 0.85f;
    if (f) plrui_pilula_foco(d, anim); else plrui_botao_repouso(d, anim);
    gfx_icone((GfxRect){ d.x + 16.0f, d.y + 16.0f, 24.0f, 24.0f }, "pl_x", k, k, k, anim); }
  // TEMPORADAS no segmentado, com a contagem de episodios das que a lista ja
  // tem; uma janela que cabe na folha, comecando antes da selecionada.
  { const char *rot[8];
    char nomes[8][40];
    int cont[8], n = 0, ini = temporada > 1 ? temporada - 1 : 0, i;
    for (i = ini; i < nTemporadas() && n < 4; i++) {
      int t = numTemporada(i), k, c = 0;
      snprintf(nomes[n], sizeof nomes[n], i18n("Temporada %d"), t);
      for (k = 0; k < cat_n_episodios(titulo); k++) { const CatEp *e = cat_episodio(titulo, k); if (e && e->temporada == t) c++; }
      rot[n] = nomes[n]; cont[n] = c > 0 ? c : -1;
      if (plrui_seg(rot, cont, n + 1, -1, 0, -1.0f, 0, anim) > lw - 32.0f) break;
      n++;
    }
    { float sx = lx + 16.0f, xx = sx + 5.0f;
      plrui_seg(rot, cont, n, temporada - ini, grupo == 0, sx, 170.0f, anim);
      if (ptr) for (i = 0; i < n; i++) {
        float w = plrui_seg(&rot[i], &cont[i], 1, -1, 0, -1.0f, 0, anim) - 10.0f;
        ponteiro_alvo(xx, 175.0f, w, 44.0f, NULL, ponteiroEpTemporada, ini + i, 0);
        xx += w + 4.0f;
      } } }
  gfx_recorte(x, EP_TOP - 4.0f, EP_W, EP_PE - EP_TOP + 4.0f);
  int n = nLinhas();
  for (int i = 0; i < n; i++) {
    float y = EP_TOP + i * EP_ROW - scroll;
    if (y + EP_ROW < EP_TOP || y > EP_PE) continue;
    const CatEp *ep = epLinha(i);
    int sel = grupo == 1 && i == foco;
    GfxRect row = { lx, y, lw, EP_ROW - 4.0f };
    // A LISTA SOME NA BASE (mask-image do mockup): as linhas que entram no
    // rodape apagam nos ultimos 70 px em vez de cortar seco.
    float fade = anim_clamp((EP_PE - (y + 30.0f)) / 70.0f, 0.0f, 1.0f), ra = anim * fade;
    if (ptr) {
      float t = y < EP_TOP ? EP_TOP : y;
      float b = y + row.h > EP_PE ? EP_PE : y + row.h;
      if (b > t) ponteiro_alvo(row.x, t, row.w, b - t, ponteiroEpLinha, NULL, i, 0);
    }
    if (sel) plrui_linha_foco(row, 24.0f, ra);
    const char *arte = ep->thumb[0] ? ep->thumb : (ci ? ci->backdrop : "");
    GLuint tex = tex_obter_larg(arte, 240);
    // Still que nao existe (nem metahub nem TMDB): o fundo da serie.
    if (!tex && ep->thumb[0] && tex_falhou(ep->thumb) && ci) {
      const char *res = ci->backdrop[0] ? ci->backdrop : ci->poster;
      GLuint t3 = res[0] ? tex_obter_larg(res, 240) : 0;
      if (t3) { arte = res; tex = t3; }
    }
    GfxRect tr = { lx + 14.0f, y + 14.0f, 240.0f, 135.0f };
    int atual = ep->temporada == atualT && ep->episodio == atualE;
    int visto = ci ? vistoep_estado(ci->imdb, ep->temporada, ep->episodio) : -1;
    if (vid) gfx_cor(tr, 16.0f / 135.0f, 0.102f, 0.106f, 0.125f, ra);
    else gfx_cor(tr, 16.0f / 135.0f, 0.102f, 0.106f, 0.125f, ra);
    // DESFOCAR NAO ASSISTIDOS (#133): so o que o mapa afirma como visto fica
    // nitido — e o que esta tocando agora, que a pessoa ja esta vendo.
    if (tex && ajustes_desfocar_nao_assistidos() && !atual && visto != 1) tex = gfx_desfocado(tex, arte);
    if (tex) { gfx_tex_aspect_atual = tex_aspecto(arte); gfx_rect(tr, tex, GFX_CARD, 0, 0, 0, 16.0f / 135.0f, 0, 0, 0, ra); gfx_tex_aspect_atual = 0; }
    // O PROGRESSO NA STILL (nao existia na folha): o ponto salvo deste
    // episodio, branco; no que esta tocando, no acento.
    { char chave[96]; ProgRegistro pr;
      if (ci && visto != 1) {
        prog_chave(chave, sizeof chave, ci->imdb, ep->temporada, ep->episodio);
        if (prog_por_chave(chave, &pr) && pr.durSeg > 1.0 && pr.posSeg > 1.0) {
          float f = (float)(pr.posSeg / pr.durSeg), ar, ag, ab;
          if (f > 1.0f) f = 1.0f;
          if (atual) ajustes_acento(&ar, &ag, &ab); else ar = ag = ab = 1.0f;
          plrui_trilho((GfxRect){ tr.x + 10.0f, tr.y + tr.h - 14.0f, tr.w - 20.0f, 4.0f }, f, ar, ag, ab, ra);
        } } }
    // O ASSISTIDO e um disco com o check sobre a still (era "✓ Assistido").
    if (visto == 1) {
      GfxRect d = { tr.x + tr.w - 44.0f, tr.y + 10.0f, 34.0f, 34.0f };
      if (vid) gfx_cor(d, 0.5f, 0.055f, 0.059f, 0.071f, 0.78f * ra);
      else gfx_cor(d, 0.5f, 0.082f, 0.086f, 0.102f, ra);
      gfx_icone((GfxRect){ d.x + 8.0f, d.y + 8.0f, 18.0f, 18.0f }, "pl_check", 1, 1, 1, ra);
    }
    { float tx = tr.x + tr.w + 20.0f, w = row.x + row.w - 14.0f - tx, ty = y + 18.0f;
      char num[48], estado[96], dur[32];
      snprintf(num, sizeof num, i18n("Episódio %d"), ep->episodio);
      plrui_kicker(num, tx, ty, 243, 242, 239, ra * 0.42f);
      ty += 22.0f;
      { TxtLinha l = txt_linha_corta(TXT_ILHA_NOME, ep->nome[0] ? ep->nome : num, 243, 242, 239, sel ? 255 : 224, w);
        txt_desenhar_alpha(l, tx, ty, ra); ty += (float)l.h + 6.0f; }
      desc_duracao_txt(ep->duracao, dur, sizeof dur);   // "45 min" -> forma do idioma da UI
      if (atual) {
        float ar, ag, ab;
        ajustes_acento(&ar, &ag, &ab);
        equalizador(tx, ty + 15.0f, ra);
        plrui_kicker("Reproduzindo agora", tx + 22.0f, ty + 1.0f, (int)(ar * 255), (int)(ag * 255), (int)(ab * 255), ra);
      } else {
        if (visto == 1) snprintf(estado, sizeof estado, "%s%s%s", i18n("Assistido"), dur[0] ? " \xc2\xb7 " : "", dur);
        else snprintf(estado, sizeof estado, "%s%s%s", ep->data, ep->data[0] && dur[0] ? " \xc2\xb7 " : "", dur);
        txt_desenhar_alpha(txt_linha_corta(TXT_ILHA_APOIO, estado, 243, 242, 239, 128, w), tx, ty, ra);
      }
      ty += 20.0f + 8.0f;
      if (sel && ep->sinopse[0])
        txt_bloco_corta(TXT_ILHA_APOIO, ep->sinopse, 243, 242, 239, tx, ty, w, 23.0f, ra * 0.5f, 2);
    }
  }
  if (!n) txt_bloco(TXT_ILHA_TEXTO, desc_episodios_carregando(titulo) ?
    "Carregando episódios…" : "Episódios indisponíveis. Selecione a temporada e pressione OK para tentar novamente.",
    196, 198, 204, lx + 22.0f, EP_TOP + 30.0f, lw - 44.0f, 30, anim, 4);
  gfx_sem_recorte();
  // RODAPE sob um fio: "4 de 8 episodios" e a dica do gesto (pressao longa
  // nao se descobre sozinha num D-pad).
  gfx_cor((GfxRect){ lx, EP_PE, lw, 1.0f }, 0.0f, 1, 1, 1, 0.07f * anim);
  { float yc = EP_PE + 14.0f + 11.0f;
    if (n) {
      char contador[48];
      snprintf(contador, sizeof contador, i18n("%d de %d episódios"), foco + 1, n);
      { TxtLinha l = txt_linha(TXT_ILHA_APOIO, contador, 243, 242, 239, 115);
        txt_desenhar_alpha(l, lx + 22.0f, yc - (float)l.h * 0.5f, anim); }
    }
    if (!vmAberto && (grupo == 0 || (n && grupo == 1))) {
      const char *k[1] = { "Segure OK" };
      const char *rt[1] = { grupo == 0 ? "Marcar a temporada" : "Marcar como assistido" };
      plrui_dicas(k, rt, 1, lx + lw - 22.0f, yc, 1, anim);
    } }
  menuDesenhar(x, EP_W, anim);
}

// --- O MENU SOZINHO, SOBRE OUTRA TELA ----------------------------------------
//
// A pagina de detalhe usa isto. Ver o comentario de VM_FONTES: la o toque curto
// no card abre as fontes e a pressao longa passou a abrir este menu, entao ele
// carrega a porta que tomou.
void episodios_menu_visto(int idxCat, int temporada, int episodio,
                          const char *nome) {
  menuAbrir(idxCat, temporada, episodio, nome, 1);
}
void episodios_menu_temporada(int idxCat, int temporada) {
  menuAbrirTemporada(idxCat, temporada, 1);
}
int  episodios_lote(int idxCat, int temporada, VistoPar *saida, int max) {
  return montarLote(idxCat, VM_TEMP, temporada, 0, saida, max);
}
int  episodios_menu_modo_temporada(void) { return vmAberto && vmModoTemp; }
int  episodios_menu_aberto(void) { return vmAberto && vmSo; }
int  episodios_menu_aberto_qualquer(void) { return vmAberto; }
void episodios_menu_evento(const SDL_Event *e) {
  if (vmAberto && vmSo) { revalidar(); menuEvento(e); }
}
void episodios_menu_desenhar(void) {
  if (vmAberto && vmSo) revalidar();
  if (vmAberto && vmSo) menuDesenhar(0.0f, (float)NV_TELA_W, 1.0f);
}
int  episodios_menu_pediu_fontes(void) { int v = vmFontesPed; vmFontesPed = 0; return v; }

#ifdef NV_SHOT_HOOKS
// Capturas: o menu de visto do episodio em foco, como a pressao longa abre.
void episodios_shot_menu(void) {
  const CatEp *e = epLinha(foco);
  if (e) menuAbrir(titulo, e->temporada, e->episodio, e->nome, 0);
}
// Capturas: o foco numa linha da lista.
void episodios_shot_foco(int linha) { grupo = 1; foco = linha; }
#endif
