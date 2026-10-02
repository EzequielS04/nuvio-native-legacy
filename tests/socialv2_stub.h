// A API DE ALCANCE E NOME do socialsrv (branch agente/socialsrv, recomenda.h),
// so as DECLARACOES, para a captura desenhar as linhas novas do painel antes do
// merge (-DNV_SOCIAL_V2_UI -include tests/socialv2_stub.h). As implementacoes
// de mentira estao em tests/socialui_shot.c. No merge este arquivo sai.
#ifndef NV_SOCIALV2_STUB_H
#define NV_SOCIALV2_STUB_H
enum { REC_ALCANCE_NAO_PERGUNTADO = -1, REC_ALCANCE_NINGUEM = 0,
       REC_ALCANCE_AMIGOS = 1, REC_ALCANCE_AMIGOS2 = 2 };
int  recomenda_alcance(void);
void recomenda_responder_alcance(int nivel);
const char *recomenda_meu_nome(void);
const char *recomenda_minha_exibicao(void);
int  recomenda_definir_nome(const char *nome);
#endif
