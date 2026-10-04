// SINCRONIZACAO AUTOMATICA DA LEGENDA NA SESSAO DO PLAYER (F05, 1.8).
//
// Cola entre o player, a legenda externa (legenda.c), a engine temporal
// (autosync.c) e a referencia independente (legref.c). Um idioma: o slot
// PRINCIPAL. O slot 1 (segundo idioma) so expoe estado "disponivel depois".
//
// O QUE FAZ, nesta ordem:
//   1. legsync_iniciar no player_abrir: contexto criado uma vez (nao segura o
//      primeiro quadro), geracao de sessao nova por midia. legsync_passo
//      detecta troca de URL no meio da sessao e abre outra geracao.
//   2. A legenda EXTERNA escolhida vira LegendaDocumento no proprio fio que a
//      baixou (legenda_carregar_com), com idioma/origem/identidade opaca.
//      Embutida, nativa ou nenhuma: AutoSync indisponivel (a embutida ja foi
//      multiplexada com o video).
//   3. So quando a pessoa pede (Rapida/Completa) o legref le a faixa de texto
//      embutida do MKV por Range, em segundo plano, com orcamento. Sem
//      referencia COMPLETA: indisponivel, offset automatico zero.
//   4. legsync_offset_ms(manual) devolve manual + automatico ACEITO, para ser
//      aplicado UMA vez no overlay principal (positivo adianta, o mesmo sinal
//      de legenda_cues/assrender_desenhar). Sem documento dono do overlay,
//      devolve o manual intacto.
//   5. Seek/buffer: cancela a analise e pausa a leitura; retoma sozinha 2 s
//      depois de calmo. legsync_encerrar no fim da sessao (sem join);
//      legsync_destruir no encerramento do app (join).
//
// NADA AQUI BLOQUEIA o fio de desenho por rede ou analise.
#ifndef NV_LEGSYNC_H
#define NV_LEGSYNC_H
#include "legref.h"

#define LEGSYNC_ACAO_RAPIDA   1
#define LEGSYNC_ACAO_COMPLETA 2
#define LEGSYNC_ACAO_DESFAZER 4
#define LEGSYNC_ACAO_OUTRA    8
#define LEGSYNC_ACAO_PARAR    16

typedef enum {
  LEGSYNC_INDISPONIVEL = 0, LEGSYNC_AGUARDANDO, LEGSYNC_PRONTA, LEGSYNC_LENDO,
  LEGSYNC_ANALISANDO, LEGSYNC_ACEITA, LEGSYNC_RECUSADA, LEGSYNC_PAUSADA,
  LEGSYNC_DESFEITA, LEGSYNC_DEPOIS
} LegSyncFase;

// Por que (para INDISPONIVEL/RECUSADA). Mapeado a texto traduzido por
// legsync_texto (legsyncui.c).
typedef enum {
  LEGSYNC_M_NENHUM = 0, LEGSYNC_M_SEM_EXTERNA, LEGSYNC_M_EMBUTIDA, LEGSYNC_M_PLATAFORMA,
  LEGSYNC_M_EXTERNA_INCOMPLETA, LEGSYNC_M_SEM_REFERENCIA, LEGSYNC_M_SEM_RANGE,
  LEGSYNC_M_REDE, LEGSYNC_M_ORCAMENTO, LEGSYNC_M_SEM_OUTRA, LEGSYNC_M_CONFIANCA
} LegSyncMotivo;

typedef struct {
  LegSyncFase fase;
  LegSyncMotivo motivo;
  int acoes;             // LEGSYNC_ACAO_* possiveis agora
  int offsetAutoMs;      // so o automatico aceito (0 sem aceite)
  int offsetTotalMs;     // manual + automatico, o que o overlay usa
  int progresso;         // 0..100 lendo a referencia
  char idiomaRef[24];    // idioma da faixa embutida usada/lida
} LegSyncVisao;

void legsync_iniciar(const char *urlMidia);
void legsync_encerrar(void);
void legsync_destruir(void);
void legsync_passo(const char *urlMidia, double posSeg, double folgaSeg, int sensivel,
                   unsigned agoraMs);
int  legsync_offset_ms(int manualMs);

// Escolha da legenda PRINCIPAL (faixas.c). externa: no lugar de
// legenda_carregar. outra: embutida (1) ou nenhuma (0).
void legsync_primaria_externa(const char *url, const char *idioma, const char *origem);
void legsync_primaria_outra(int embutida);

int  legsync_acao(int acao);            // 1 = aceita no estado atual
LegSyncVisao legsync_visao(int slot);   // slot 1: LEGSYNC_DEPOIS

// --- API DE APRESENTACAO (legsyncui.c) -------------------------------------
void legsync_texto(const LegSyncVisao *v, char *dst, unsigned tam);  // i18n
const char *legsync_acao_rotulo(int acao);                            // i18n
// Liga o AutoSync como provedor da linha de sincronizacao do seletor de
// legendas do F04 (legendasui_definir_sync). Idempotente; thread da UI.
void legsync_ui_ligar(void);

// So para testes: leitor do legref antes do primeiro legsync_iniciar.
void legsync_teste_leitor(LegRefLer ler, void *u);

#endif
