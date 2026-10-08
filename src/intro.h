// MARCADORES DE ABERTURA, RESUMO E CREDITOS.
//
// A fonte e o TheIntroDB (api.theintrodb.org/v3/media), e a troca foi feita por
// uma razao concreta: o servico anterior (api.introdb.app) e indexado POR
// EPISODIO — sem `season` e `episode` ele nao responde. Filme, portanto, nao
// tinha marcador nenhum e caia na estimativa proporcional do posplay.c. O app
// web tem a mesma limitacao, pelo mesmo motivo (skipIntroRepository.js exige os
// dois numeros antes de chamar).
//
// O TheIntroDB aceita `imdb_id` sozinho e devolve marcador de FILME. Conferido
// ao vivo antes de escrever este arquivo:
//
//   GET /v3/media?imdb_id=tt0111161
//   {"tmdb_id":278,"type":"movie","credits":[{"start_ms":8300000,"end_ms":null}]}
//
// (Shawshank: creditos aos 8300 s de um filme de 8520 s.) Para serie, com
// season/episode, vem tambem o `intro`:
//
//   {"type":"tv","intro":[{"start_ms":272500,"end_ms":366000}],
//    "credits":[{"start_ms":3503000,"end_ms":null}]}
//
// SEM CHAVE e sem cadastro. Titulo desconhecido responde 404 com
// {"error":"media not found"}, que aqui vira "zero marcadores".
#ifndef NV_INTRO_H
#define NV_INTRO_H
// `fim` ZERO QUER DIZER "ATE O FIM DA MIDIA", que e como a API representa o
// `end_ms: null` dos creditos. Quem consome tem de tratar esse caso — ver
// intro_ativo.
typedef struct { double inicio,fim; int tipo; } IntroTrecho;
enum { INTRO_ABERTURA=1, INTRO_RESUMO=2, INTRO_CREDITOS=3 };
// `temporada` e `episodio` ZERO = filme: a consulta sai so com o imdb.
void intro_pedir(const char *imdb,int temporada,int episodio);
// Igual a intro_pedir, mas diz a duracao (s) dos episodios E-1 e E+1 quando o
// catalogo sabe (0 = desconhecida): sem marcador deste episodio, o marcador de
// um vizinho vira "quanto falta para o fim". Ver credfonte.h.
void intro_pedir_vizinhos(const char *imdb,int temporada,int episodio,double durAnt,double durProx);
void intro_desligar(void);
int  intro_ativo(double posSeg,double *fim,int *tipo);
// Segundo em que os creditos comecam, ou 0 quando nao ha marcador. Serve ao
// posplay.c, que precisa do INSTANTE e nao de "estou dentro".
double intro_creditos_seg(void);
// Duracao real da midia (0 = desconhecida) e se e filme: base da guarda de janela.
void intro_definir_duracao(double dur,int filme);
// Janela aceitavel? Creditos <= 900 s, abertura/resumo <= 180 s, creditos de
// filme so a partir de 50% da duracao, creditos de SERIE so na parte final
// (intro_creditos_janela), abertura/resumo so na primeira metade, e nenhum
// trecho com fim explicito alem da duracao (marcador de outro corte). `motivo`
// (opcional) diz o porque.
int  intro_janela_ok(int tipo,double ini,double fim,double dur,int filme,const char **motivo);

// ONDE OS CREDITOS DE UM EPISODIO PODEM COMECAR (2.0.3). Um marcador de creditos
// so vale se sobrar ate esta janela de episodio depois dele: 15% da duracao,
// com piso de 2 min (episodio curto, anime com ED + previa ~110 s) e teto de
// 5 min (o #34: "dez minutos antes do fim nao e credito"). Nunca mais que
// metade do episodio. 22 min -> 198 s, 25 min -> 225 s, 45 min+ -> 300 s.
#define INTRO_CRED_FRAC   0.15
#define INTRO_CRED_MIN_S  120.0
#define INTRO_CRED_MAX_S  300.0
double intro_creditos_janela(double dur);

// SEM MARCADOR NENHUM, quanto antes do fim o cartao do proximo episodio sobe
// (2.0.3, pedido do dono: NADA de porcentagem). Tempo FIXO de creditos finais
// tipicos: medidos no TheIntroDB, Breaking Bad T1E1/T1E2 tem 47 s/45 s de
// creditos, Silo T1E1 ~100 s, anime ~110 s (ED + previa). 40 s fica dentro dos
// creditos de quase toda serie sem roubar a ultima cena. Episodio curto
// (< 10 min, desenho/web serie) tem creditos de ~10-20 s: 15 s. Abaixo de 2 min
// nao e episodio (clipe de erro, duracao provisoria): nao ha estimativa.
#define INTRO_FIM_SERIE_S   40.0
#define INTRO_FIM_CURTO_S   15.0
#define INTRO_CURTO_ATE_S  600.0
#define INTRO_DUR_MIN_S    120.0
// Segundos antes do fim em que a estimativa vale; 0 = nao estimar.
double intro_fim_estimado(double dur);

// A RESPOSTA E DESTE EPISODIO? O TheIntroDB remapeia a numeracao do IMDb para a
// do TMDB e ecoa `season`/`episode` do que achou. Medido em 07/10/2026: One
// Piece imdb T4E1 e T4E2 voltam AMBOS como T1E48 — o marcador seria de outro
// episodio. So vale a resposta que ecoa o mesmo par pedido (filme: sempre).
int  intro_resposta_confere(const char *json,int temporada,int episodio);
// Botao de pular com tempo: some em INTRO_BOTAO_SEG sem foco, uma aparicao
// automatica por trecho, volta so com os controles (osd) de pe.
#define INTRO_BOTAO_SEG 10.0
int  intro_botao(double posSeg,double agora,int osd,int focado,double *fim,int *tipo);
int  intro_botao_visivel(double *fim,int *tipo);
int  intro_extrair(const char *json,IntroTrecho *saida,int max);
// Copia ate `max` trechos conhecidos; devolve quantos.
int  intro_trechos(IntroTrecho *saida,int max);
#ifdef NV_SHOT_HOOKS
void intro_shot_definir(const IntroTrecho *v,int n);   // capturas: trechos fixos
#endif
#endif
