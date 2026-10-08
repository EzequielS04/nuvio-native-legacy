// A tela do Dolby Vision em MKV. Ver dvtela.h.
#include "dvtela.h"
#include "video.h"
#include <string.h>

// ESQUELETO: a tela ainda nao existe. A maquina nunca entra e o player segue
// como antes; os testes (tests/dvtela.c, tests/dvtela_shot.c) cobram o resto.
void dvt_entrar(DvtelaEstado *e, Uint32 agora) { (void)e; (void)agora; }
int  dvt_passo(DvtelaEstado *e, const DvtelaSinais *s, Uint32 agora) { (void)e; (void)s; (void)agora; return 0; }
void dvt_sair(DvtelaEstado *e, int saida, Uint32 agora) { (void)e; (void)saida; (void)agora; }

static DvtelaEstado E;
void dvtela_entrar(Uint32 agora) { (void)agora; }
int  dvtela_ativa(void) { return 0; }
int  dvtela_visivel(void) { return 0; }
float dvtela_alfa(void) { return 0.0f; }
int  dvtela_atualizar(const DvtelaSinais *s, float dt, Uint32 agora) { (void)s; (void)dt; (void)agora; return 0; }
int  dvtela_evento(const SDL_Event *e, Uint32 agora) { (void)e; (void)agora; return DVT_EV_NADA; }
void dvtela_sair(int saida, Uint32 agora) { (void)saida; (void)agora; }
const DvtelaEstado *dvtela_estado(void) { return &E; }
int  dvtela_foco_botao(void) { return 0; }
const char *dvtela_nome_passo(int passo) { (void)passo; return ""; }
const char *dvtela_nome_saida(int saida) { (void)saida; return ""; }
void dvtela_desenhar(Uint32 agora) { (void)agora; }

// O BACKEND DOS ALVOS SEM O CAMINHO DO DV (Tizen, .tpk, Android). So o ramo
// webOS e o coto do Mac de video.c tem a implementacao; nos outros a tela
// nunca entra e o preroll nao existe. `weak`: a definicao de video.c vence.
__attribute__((weak)) void video_dv_fase(VideoDvFase *f) { if (f) memset(f, 0, sizeof *f); }
__attribute__((weak)) int  video_dv_candidato(const char *url) { (void)url; return 0; }
__attribute__((weak)) void video_dv_tela(int cobrindo) { (void)cobrindo; }
__attribute__((weak)) void video_dv_recusar(void) {}
__attribute__((weak)) void video_dv_segurar(int segurar) { (void)segurar; }
__attribute__((weak)) int  video_iniciando(void) { return 0; }
