// SPOTLIGHT: a caixa de busca que o botao de MICROFONE do controle abre por
// cima de qualquer tela (pedido do dono, 01/10/2026), no lugar de levar a
// pessoa para a tela de Busca inteira.
//
// O QUE E: um painel de vidro centrado, com o campo no topo, o teclado de tela
// a esquerda (o D-pad continua precisando dele) e a lista de resultados a
// direita, que muda A CADA LETRA. Os resultados vem agrupados — melhor
// resultado em destaque, Titulos, Pessoas, Colecoes, Canais, Catalogos e
// Addons — e com o campo vazio a lista mostra as PESQUISAS RECENTES do perfil
// (as mesmas de buscasrec.h, que a tela de Busca usa) e "Em alta".
//
// QUEM ABRE (pesquisa de 01/10/2026):
//   - Android TV: KEYCODE_SEARCH (84), que e o que o botao de microfone da
//     maioria dos controles manda e que o Android ENTREGA ao app. O
//     NuvioActivity o troca por F6. KEYCODE_ASSIST (219) e
//     KEYCODE_VOICE_ASSIST (231) o sistema NAO entrega (documentado em
//     KeyEvent): controle cujo botao manda esses abre o Google Assistente e o
//     app nao fica sabendo. Amarela (PROG_YELLOW) vira F5.
//   - Samsung .tpk: "XF86BTVoice" CHEGA ao app (MEDIDO no D1: linhas
//     "[tecla] tpk sem mapa: XF86BTVoice" de um 6+ na 1.6.0). tpk.c o troca
//     por F6; "XF86Yellow" vira F5.
//   - Samsung .wgt: a tecla de microfone nao esta na lista registravel do
//     tizen.tvinputdevice. A casca registra "Search" (10225, controles antigos)
//     -> F6 e repassa a amarela (405) -> F5.
//   - LG webOS: o microfone do Magic Remote e do sistema (LG: "no APIs are
//     provided for system-level voice control"). Nenhuma tecla desconhecida do
//     D1 tem cara de microfone. Ficam a AMARELA (scancode 488, fora do Guia,
//     que ja a usa) e SEGURAR OK no item Buscar da barra lateral.
//   - Mac/teste: F6, F5 e "spotlight"/"voz" em /tmp/nuvio-key.
// F6 e "abrir pela voz": onde ha ditado (Android), o ditado ja comeca. F5 e
// "abrir": so a caixa.
//
// DITADO: so no Android, pelo RecognizerIntent (a tela de voz do sistema; o
// app nao pede permissao de microfone, quem grava e o reconhecedor). O texto
// reconhecido entra no campo como se tivesse sido digitado. Tambem no Android
// a tecla "Teclado" chama o IME do sistema (SDL_StartTextInput), que nas TVs
// com Gboard tem o proprio microfone.
#ifndef NV_SPOTLIGHT_H
#define NV_SPOTLIGHT_H
#include <SDL2/SDL.h>

// Teclas sinteticas (ver o topo): quem traduz o controle manda estas.
#define SPOT_TECLA_VOZ   SDLK_F6
#define SPOT_TECLA_ABRIR SDLK_F5

// `voz` = 1 quando quem abriu foi o botao de microfone: com ditado disponivel
// ele comeca na hora.
void spot_abrir(int voz);
void spot_fechar(void);
int  spot_aberto(void);
// Ainda na tela (aberto ou saindo na animacao).
int  spot_visivel(void);
// Entrada assentada: o fundo de tras pode ser congelado (ver app.c).
int  spot_cheio(void);
void spot_evento(const SDL_Event *e);
void spot_atualizar(float dt, Uint32 agora);
// `veuPronto` = 1 quando o veu ja foi pintado na copia congelada do fundo.
void spot_desenhar(Uint32 agora, int veuPronto);
// O veu de tela inteira, para quem congela o fundo pinta-lo dentro da copia.
void spot_veu(void);

// O que a pessoa escolheu. Consumido na leitura, como busca_pediu_abrir.
enum {
  SPOT_NADA = 0,
  SPOT_TITULO,      // indice = indice no catalogo
  SPOT_PESSOA,      // indice = titulo de onde a pessoa veio; tmdb/nome/arte
  SPOT_COLECAO,     // indice = col_folder(indice)
  SPOT_CANAL,       // id/nome/base do canal (guia_item_do_canal)
  SPOT_CATALOGO,    // indice = cat_fileira(indice)
  SPOT_ADDONS       // abre a tela de Addons
};
typedef struct {
  int  tipo;
  int  indice;
  long tmdb;
  char id[80];
  char nome[140];
  char arte[512];
  char base[600];
} SpotPedido;
int  spot_pediu(SpotPedido *p);

// Texto que veio de fora (ditado, testes): substitui o campo.
void spot_texto_externo(const char *t);
const char *spot_consulta(void);
// Para testes: quantas linhas a lista tem, o tipo e o texto da linha i e a
// linha focada (-1 com o foco no teclado).
int  spot_n_linhas(void);
int  spot_linha_tipo(int i);
const char *spot_linha_texto(int i);
int  spot_linha_focada(void);
#endif
