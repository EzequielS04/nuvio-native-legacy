// TEXTO DO SISTEMA: o teclado (IME) e a voz do aparelho para os campos de texto
// do app (Spotlight, Busca e a modal de teclado.c).
//
// POR QUE EXISTE (relato do dono na TCL, Android 14, 01/10/2026): no Spotlight
// ele "nao consegue clicar no campo de texto, so no teclado", e queria DITADO.
// O microfone do controle da TCL manda ASSIST, que o Android entrega ao Gemini e
// nunca ao app (KeyEvent: "not delivered to applications"); o ditado tem de vir
// de dentro do app.
//
// O QUE O ANDROID FAZ (NuvioActivity.kt):
//   - TECLADO: um EditText invisivel recebe o foco e chama o IME do sistema
//     (Gboard e o teclado da TCL tem o proprio microfone: o ditado dele chega
//     como texto normal). A cada mudanca o texto INTEIRO vem para ca, com a
//     composicao em andamento: o campo do app espelha o do IME, nunca soma
//     letra por letra — por isso ditado, autocorrecao e apagar funcionam sem
//     contar teclas. Concluir/Voltar fecham o IME e devolvem o foco ao SDL.
//     O SDL_StartTextInput do SDL 2.30 NAO foi usado: ele gera KEYDOWN falso
//     para cada letra ASCII (SDLInputConnection.updateText), e o espaco e o
//     backspace falsos viravam "OK" e "fechar" nas telas do app.
//   - VOZ: android.speech.SpeechRecognizer DENTRO do app, com a permissao
//     RECORD_AUDIO pedida no primeiro uso (o dono: "o aplicativo nem pediu
//     permissao do mic igual outros pedem"); parciais aparecem no campo
//     enquanto a pessoa fala, e o nivel do som (onRmsChanged) anima o
//     microfone. Sem reconhecedor, ou com a permissao negada, cai na tela de
//     voz do sistema (RecognizerIntent); sem ela tambem, abre o teclado do
//     sistema e avisa que o microfone dele dita.
//
// FORA DO ANDROID (LG, Samsung, Mac) nada disto existe: st_*_disponivel() = 0 e
// as telas ficam como eram. Para captura e teste no Mac, NUVIO_SISTEXTO_TESTE=1
// finge que existe (abrir so loga) e st_teste_evento injeta o que o Android
// mandaria.
#ifndef NV_SISTEXTO_H
#define NV_SISTEXTO_H
#include <stddef.h>

// Quem pediu: o texto que volta e so dele.
enum { ST_DONO_NENHUM = 0, ST_SPOT, ST_BUSCA, ST_TECLADO };
// O que esta aberto agora.
enum { ST_PARADO = 0, ST_DIGITANDO, ST_PERMISSAO, ST_OUVINDO, ST_VOZ_SISTEMA };
// Retorno de st_ler.
enum { ST_NADA = 0, ST_TEXTO, ST_FIM, ST_CANCELOU, ST_PEDE_TECLADO };

int   st_ime_disponivel(void);
int   st_voz_disponivel(void);
// Abre o teclado do sistema com `inicial` no campo (max em bytes). 1 = abriu.
int   st_ime_abrir(int dono, const char *inicial, int max);
// Comeca o ditado (permissao, reconhecedor do app, ou os degraus de reserva).
int   st_voz_iniciar(int dono);
// Fecha o teclado e para a voz, se forem desse dono.
void  st_fechar(int dono);
int   st_estado(void);
int   st_dono(void);
// Nivel do som enquanto ouve, 0..1.
float st_nivel(void);
// Ultimo aviso para a pessoa (texto pt, passar por i18n), "" se nenhum. Vale
// ate a proxima abertura.
const char *st_aviso(void);
// Uma vez por quadro pelo dono. ST_TEXTO: `dst` e o texto INTEIRO do campo
// agora (substitui, nao soma). ST_FIM: o mesmo e a entrada terminou (Concluir
// ou fim da fala). ST_CANCELOU: fechou sem concluir (o texto que ja veio fica).
// ST_PEDE_TECLADO: a voz nao existe aqui; o dono abre st_ime_abrir com o texto
// dele (o aviso ja diz que o microfone do teclado do sistema dita).
int   st_ler(int dono, char *dst, size_t n);

// --- testes ---
// Liga/desliga o modo de teste (o mesmo de NUVIO_SISTEXTO_TESTE=1).
void  st_teste_ligar(int on);
// Evento cru, no formato do Android (ver sistexto.c).
void  st_teste_evento(const char *ev);
#endif
