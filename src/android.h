// Ponte do nucleo com o Android (so existe com -DNV_ANDROID). Chamada cedo no
// main(), DEPOIS de o log da sessao ja estar redirecionado para o arquivo.
#ifndef NV_ANDROID_H
#define NV_ANDROID_H
#ifdef NV_ANDROID
#include <stddef.h>
// Escreve a linha "[tv] ..." no log (mesmo formato do host .tpk, LogaTv),
// espelha stdout/stderr no logcat (tag "nuvio") sem tirar nada do arquivo, e
// pede ao SDL que o Voltar chegue ao app em vez de fechar a Activity.
void android_iniciar(void);
// Pede ao Android uma superficie de w x h pixels (SurfaceHolder.setFixedSize)
// e espera ela chegar, ANTES do SDL_CreateWindow: no Android a janela do SDL
// tem o tamanho da superficie, nao o pedido. Usado pelo ajuste 4K. Devolve 1
// se a superficie veio nesse tamanho.
int android_pedir_superficie(int w, int h);
// Entrega o APK em `caminho` ao instalador do sistema (NuvioActivity.instalarApk,
// FileProvider). 1 = instalador aberto, 2 = falta a permissao de instalar apps
// desta fonte (a tela dela foi aberta), 0 = falhou. Chamar do fio do SDL.
int android_instalar_apk(const char *caminho);
// Texto do sistema (sistexto.h). Teclado do sistema com `inicial` no campo;
// voz no idioma dado ("pt-BR"); fechar os dois; e o proximo evento da fila do
// NuvioActivity (1 = veio um em dst). O formato dos eventos esta em sistexto.c.
int  android_st_teclado(const char *inicial, int max);
int  android_st_ditar(const char *idioma);
void android_st_fechar(void);
int  android_st_evento(char *dst, size_t n);
// ONDE ASSISTIR (ondever.c). Lista: "pacote\tnome" por linha, malloc (free
// pelo chamador), NULL se falhou. Abrir/loja: 1 = abriu.
char *android_listar_apps(void);
int android_abrir_app(const char *pacote);
int android_abrir_loja(const char *pacote, const char *nome);
// Vigia do arranque (#266, ArranqueVigia.kt): a etapa do main() em que o fio do
// SDL esta (texto estatico) e um contador de quadros apresentados. So o fio
// do SDL chama.
void android_etapa(const char *nome);
void android_quadro(void);
// #318 (Xiaomi MiTV-AFKR0 e outros: tela pisca e congela ao trocar de tela,
// desde a 2.0.1). Tres mudancas de GL so do Android entraram na 2.0.1: o
// glDiscardFramebufferEXT (descarte), a consulta de tempo de GPU (gputempo) e o
// fundo da Dinamica adiado (din). Ficam DESLIGADAS, como na 2.0.0; cada uma
// volta com `adb shell setprop debug.nuvio.318 "descarte gputempo din"`.
// 1 = religar `nome`.
int android_318_religar(const char *nome);
#endif
#endif
