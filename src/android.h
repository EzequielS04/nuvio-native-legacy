// Ponte do nucleo com o Android (so existe com -DNV_ANDROID). Chamada cedo no
// main(), DEPOIS de o log da sessao ja estar redirecionado para o arquivo.
#ifndef NV_ANDROID_H
#define NV_ANDROID_H
#ifdef NV_ANDROID
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
#endif
#endif
