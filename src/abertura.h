// ABERTURA DO APP (#213): a marca do Nuvio por cima dos primeiros quadros.
//
// O relato da G5 (webOS 10.3): "abre uma tela cinza com bolinhas paradas, cara
// de app demo". O cinza e o splash PADRAO do sistema (splashColor "gray" na
// referencia do appinfo.json) e as bolinhas o spinner dele, parado enquanto o
// app compila os shaders — no registro, 1,3 s entre gfx_iniciar e as fontes.
//
// Duas pecas, uma imagem so:
//   1. ANTES do app: o sistema mostra deploy/app/splash.png (appinfo
//      splashBackground). Android: windowBackground (res/drawable/abertura.xml).
//      Tizen: o #abertura do tools/tizen-shell.html.
//   2. NO APP: o primeiro quadro ja nasce com a MESMA marca no MESMO lugar
//      (780 px no centro de 1920x1080, fundo #0E0F12) — sem salto da imagem do
//      sistema para o app —, respira de leve e abre para a home com uma mola.
//      Um quad de cor e um de textura por quadro, e so ate a saida acabar.
//
// O app nao espera por ela: a home monta por baixo desde o primeiro quadro, e a
// saida comeca quando as artes pedidas chegam (ou no teto, ABERTURA_TETO_MS).
// Uma tecla durante a abertura antecipa a saida.
#ifndef NV_ABERTURA_H
#define NV_ABERTURA_H
#include <SDL2/SDL.h>

// Carrega a marca (art/marcas/nuvio_wordmark.png). Sem o arquivo, a abertura
// vira so o fundo esvanecendo. Depois de gfx_iniciar (precisa de contexto GL).
void abertura_iniciar(const char *dirArte);
// Desenha por cima do quadro; 0 quando ja acabou (e a textura foi liberada).
// `pendentes` = artes em voo no cache (tex_estatisticas).
int  abertura_desenhar(Uint32 agora, float dt, int pendentes);
// Uma tecla chegou: a saida comeca ja.
void abertura_tecla(void);
// 1 enquanto ainda cobre a tela.
int  abertura_ativa(void);

#endif
