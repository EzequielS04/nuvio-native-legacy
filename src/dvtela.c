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
