// ILHA — o relogio no topo da tela, numa pilula, que se ABRE para os avisos.
//
// Pedido do dono (01/10/2026): "ilha dinamica estilo iOS". Em repouso a
// pilula mostra a hora; quando o app tem algo a dizer (aviso novo na central,
// erro, lembrete, versao nova, atualizacao baixando) a MESMA pilula cresce com
// mola, troca o conteudo por icone + frase curta e, vencido o prazo, encolhe
// de volta para o relogio. Um lugar so para o app falar, em vez de cada coisa
// abrir o proprio cartao num canto diferente.
//
// O QUE ELA NAO FAZ: dentro do player quem desenha a pilula e plrilha.c (a
// MESMA pilula, no mesmo canto, com a hora e o "termina as" do titulo, e os
// componentes pequenos do player nascendo dela — Glass UI, 03/10); app.c so
// desenha ESTA com o player fechado. Nao pega teclado fora do modal (o
// AZUL/CH+ da central continua em avisos_evento; o do modal, ver abaixo) e
// nao e camada de tela cheia — e uma sombra pequena, uma pilula e o texto,
// mesmo durante a mola, e o modal e a mesma pilula crescida.
//
// QUATRO CONTEUDOS, por prioridade: aviso (fila curta, um de cada vez) >
// atividade (algo em andamento, renovado a cada quadro por quem a tem) >
// cartao (atividade ao vivo / estreia, ao lado do relogio, so com ele) >
// relogio (so onde app.c disse que cabe: ilha_relogio_visivel).
//
// FIO PRINCIPAL em tudo.
#ifndef NV_ILHA_MODULO_H
#define NV_ILHA_MODULO_H
#include <SDL2/SDL.h>

enum { ILHA_INFO = 0, ILHA_OK, ILHA_ERRO, ILHA_ACENTO };

// Aviso curto. `chave` identifica o aviso: repetir a chave do que esta na
// tela troca o texto NO LUGAR (a pilula remorfa) e renova o prazo, em vez de
// entrar na fila de novo; NULL/"" = um aviso avulso. `icone` e um nome de
// art/icones (NULL = o do tipo). `ms` conta a partir do primeiro quadro em que
// ele aparece. `tecla` = 1 desenha a tecla da central (disco AZUL / CH+) com
// "abre" na ponta direita.
void ilha_avisar(const char *chave, int tipo, const char *icone,
                 const char *texto, unsigned ms, int tecla);
// Tira o aviso desta chave da tela e da fila (ex.: a central foi aberta).
void ilha_retirar(const char *chave);
// 1 enquanto o aviso desta chave esta na tela ou na fila.
int  ilha_tem(const char *chave);

// --- PRIORIDADE, CONTADOR E A CENTRAL (mockup aprovado em 02/10) ------------------
//
// A FILA ERA FIFO e um erro esperava atras de tres avisos informativos — ate
// 4 x 20 s se fossem toasts da central (mapa.md, secao 10). Agora:
//   - P1 (erro que trava a pessoa) FURA A FILA: entra ja, e o que estava na
//     tela volta para a frente da fila e reaparece depois, com o prazo inteiro
//     (o da central nao: ele continua na lista de avisos);
//   - dentro da mesma prioridade, ordem de chegada;
//   - com dois ou mais esperando, a pilula mostra "+N" na ponta;
//   - os avisos DA CENTRAL (grupo = 1) nao passam um por um: o primeiro diz o
//     assunto ("Ana recomendou Fallout"); chegando outro enquanto ele ainda
//     espera, os que esperam viram UM so, "N avisos novos", que abre a central.
// Prioridade 0 = pelo tipo: ERRO e P1, ACENTO e P2, OK e INFO sao P3.
enum { ILHA_P1 = 1, ILHA_P2, ILHA_P3 };

// MODAL GENERICO: a mesma pilula crescida (o "a pilula cresce" dos cartoes),
// com 1 a 3 botoes e, no lugar da arte 16:9, o ROSTO de alguem (pedido de
// amizade) ou um ICONE num ladrilho (versao nova, Trakt, aviso do dono). Todos
// os campos sao opcionais menos `titulo` e um botao; vazio = a linha some.
#define ILHA_MODAL_BOTOES 3
typedef struct {
  char kicker[80];          // linha pequena em maiusculas acima do titulo
  char titulo[160];         // grande (a "logo" do modal)
  char linha[160];          // logo abaixo do titulo ("Série · 2022 · IMDb 8,7")
  char nota[120];           // apagada, logo abaixo ("Você está na 1.7.1")
  char texto[420];          // corrido, ate 3 linhas (bio, explicacao)
  char fala[240];           // citacao com fio a esquerda (o recado do amigo)
  char lista[3][96];        // itens com bolinha (notas da versao)
  char chips[3][32];        // generos, em pilulas
  char estado[80];          // na base da coluna ("há 12 min")
  char rodape[120];         // a direita dos botoes ("Recusar não avisa a pessoa.")
  char arte[1024];          // 480x270; com `rosto`, ele vai no canto da arte
  char rosto[256];          // url do avatar (vazio = so a inicial de rostoNome)
  char rostoNome[64];
  char icone[32];           // sem arte nem rosto: ladrilho 150 com este icone
  int  tipo;                // a luz do canto (ILHA_ERRO no Trakt), como na pilula
  int  salvos;              // 1 = "Salvos ›" na ponta, como nos cartoes
  int  nBotoes;
  char botao[ILHA_MODAL_BOTOES][40];       // rotulo JA traduzido
  char botaoIcone[ILHA_MODAL_BOTOES][32];  // "" = sem icone
  // A ILHA DO RELOGIO CRESCIDA (mockup do registro, quadro 14): 880 de largura,
  // cabecalho de 64 com o icone, o `kicker` e a hora, e embaixo titulo, texto
  // e botoes, sem ladrilho. A queda da sessao anterior usa.
  int  cabecalho;
} IlhaModal;

// O aviso completo. `texto` aceita ENFASE: o trecho entre dois ILHA_FORTE sai
// em negrito (o nome da pessoa, o titulo) — use ilha_forte para embrulhar o
// argumento, nunca o formato traduzido.
#define ILHA_FORTE "\x02"
typedef struct {
  const char *chave;
  int tipo;
  const char *icone;        // NULL = o do tipo; ignorado com rosto/capa
  const char *texto;
  unsigned ms;
  int tecla;                // 1 = a tecla AZUL com "abre" (vira 1 sozinho com modal)
  int prior;                // ILHA_P1..P3; 0 = pelo tipo
  int grupo;                // 1 = item da central (ver acima)
  const char *rosto;        // avatar 36 (url; "" = a inicial de rostoNome)
  const char *rostoNome;
  const char *capa;         // mini capa 30x44; com rosto vira o "duo"
  const char *meta;         // sufixo apagado depois da frase ("T2E4")
  int vivo;                 // ponto vermelho de "agora"
  int cartao;               // AZUL abre o modal deste cartao (ILHA_VIVO + 1 ...); 0 = nao
  const IlhaModal *modal;   // AZUL abre este modal; NULL = nao
} IlhaAvisoEx;
void ilha_avisar_ex(const IlhaAvisoEx *a);
// Embrulha `s` em ILHA_FORTE em `dst` (devolve dst), para passar como %s.
const char *ilha_forte(char *dst, size_t tam, const char *s);
// A central abriu: os avisos dela (grupo = 1) saem da tela e da fila.
void ilha_retirar_grupo(void);
// 1 = o aviso na tela leva a tecla e ela e da CENTRAL (sem modal nem cartao):
// e quando AZUL/CH+ abre a lista de avisos (avisos_evento).
int  ilha_tecla_central(void);
// Botao escolhido num modal de AVISO, entregue uma vez: devolve 1..3 (o
// indice + 1) e copia a chave do aviso em `chave`; 0 = nada. Voltar nao conta.
int  ilha_aviso_pediu(char *chave, size_t tam);
// Para o teste da fila (tests/ilhafila.c) e para o log: a chave do aviso da
// vez ("" sem aviso) e quantos esperam atras dele (o numero do "+N").
const char *ilha_aviso_vez(void);
int  ilha_esperando(void);

// ATIVIDADE em andamento: chame A CADA QUADRO enquanto durar; sem renovacao
// por ~0,4 s ela sai sozinha. `progresso` de 0 a 1, ou < 0 quando nao ha numero.
void ilha_atividade(const char *texto, float progresso);

// Onde o relogio pode ficar, decidido por quadro por app.c (a home tem o topo
// esquerdo livre; Ajustes e Explorar tem titulo ali).
void ilha_relogio_visivel(int visivel);

// PONTO UNICO DA POSICAO. Sem chamada, a ilha fica no topo esquerdo, na coluna
// do conteudo — ou no topo direito no layout Dinamica, onde a pilula fechada
// da barra lateral ocupa o canto esquerdo. Quem expuser uma area (ex. a pilula
// do menu) chama isto a cada quadro antes de ilha_desenhar: (x, y) e o canto
// de cima da ilha; `daDireita` = 1 faz x ser a borda DIREITA e a ilha crescer
// para a esquerda. Vale para um quadro.
void ilha_ancorar(float x, float y, int daDireita);

// Aplica a escolha de Ajustes (Posicao do relogio) ao quadro: chamar antes de
// ilha_desenhar. `guia` = 1 na tela do Guia (titulo a esquerda: vai a direita).
void ilha_posicionar(int guia);

void ilha_desenhar(Uint32 agora);
// 1 quando ha aviso ou atividade aberta (o relogio sozinho nao conta).
int  ilha_ocupada(void);

// --- CARTOES PERSISTENTES: atividade ao vivo e estreia (pedido do dono, 01/10) ---
//
// SO COM O RELOGIO NA TELA (Ajustes > Relogio na tela e onde app.c diz que ele
// cabe). Um cartao fica AO LADO do relogio, na mesma pilula, ate quem o poe
// tira-lo (ilhacart.c): a sessao interrompida do player (ILHA_VIVO) e o
// episodio novo de uma serie com lembrete ainda nao lido (ILHA_ESTREIA). Os
// avisos e a atividade continuam passando na frente; dois cartoes alternam a
// cada ILHA_ALTERNA_MS. AZUL/CH+ com um cartao na pilula abre o MODAL — a
// pilula cresce ate um cartao maior com a arte e os botoes (ilha_evento).
// ILHA_AMIGO (02/10, mockup aprovado): o terceiro cartao, "Ana · Severance ·
// agora", com o rosto dela e a capa — o evento de inicio mais novo do feed
// enquanto ele tiver menos de 15 min (a regra de RecAmigo.temAgora).
enum { ILHA_VIVO = 0, ILHA_ESTREIA, ILHA_AMIGO, ILHA_N_CARTOES };
#define ILHA_ALTERNA_MS 6000u
typedef struct {
  char chave[80];        // muda = outro conteudo (a pilula remorfa)
  char imdb[64];
  int  serie, t, e;      // t/e = 0 em filme
  char titulo[160], epNome[120], sinopse[420];
  char poster[1024], logo[512], arte[512];   // arte: still do episodio ou fundo
  float progresso;       // 0..1 na ILHA_VIVO; < 0 sem barra
  int  restanteMin;      // ILHA_VIVO
  char quando[24];       // ILHA_ESTREIA: "hoje" (ja traduzido) ou ""
  char avisoId[72];      // ILHA_ESTREIA: id do aviso em avisos.c
  char pessoa[64], rosto[256];   // ILHA_AMIGO: nome e avatar de quem esta vendo
} IlhaCartao;
// NULL tira o cartao. A copia e da ilha; quem chama pode descartar o seu.
void ilha_cartao(int qual, const IlhaCartao *c);
// Troca de identidade: apaga este cartao e suas copias no modal, no pedido e
// na animacao. Os outros cartoes, avisos e atividade nao sao alterados.
void ilha_cartao_invalidar(int qual);
// 1 quando ha cartao e o relogio esta na tela: e quando AZUL/CH+ abre o modal.
int  ilha_cartao_na_tela(void);

// O MODAL. Abrir pega o cartao que a pilula mostra; Voltar recolhe para ela.
int  ilha_modal_abrir(void);
void ilha_modal_fechar(int seco);   // seco = 1: some sem recolher (virou o painel)
int  ilha_modal_aberto(void);
int  ilha_modal_visivel(void);
// Teclado do modal (come tudo enquanto aberto). Fora dele devolve 0.
int  ilha_evento(const SDL_Event *e);
// Acao pedida no modal, entregue uma vez (o contrato de avisos_pediu). `c`
// recebe o cartao em que ela foi pedida.
enum { ILHA_PEDIU_NADA = 0, ILHA_PEDIU_TOCAR, ILHA_PEDIU_DETALHES,
       ILHA_PEDIU_DISPENSAR, ILHA_PEDIU_SALVOS };
int  ilha_pediu(IlhaCartao *c, int *qual);

// MINIMIZAR NA ILHA: o player saiu no meio e a sessao virou o cartao
// ILHA_VIVO. A arte do cartao (still do episodio; `fundoReserva` = o fundo do
// titulo, quando o still nao esta decodificado) nasce em tela cheia e encolhe
// em 400 ms ate a mini capa da pilula, com a tela de baixo aparecendo por tras.
// Chamar com o relogio na tela (quem chama ja trocou para a home) e o cartao
// posto. Animacoes reduzidas: nada voa, a pilula ja aparece com o cartao.
// 0 = sem cartao ou sem relogio: nada a fazer.
int  ilha_minimizar(const char *fundoReserva);
// Logo depois do ilha_minimizar: 1 = o video pausado segue no plano de baixo
// (sessao retida, Android) e o voo nasce dissolvendo a partir dele.
void ilha_minimizar_dissolver(int sim);
int  ilha_minimizando(void);

// Retangulo da pilula (ou do modal, enquanto ele esta na tela) no ultimo
// quadro: e de onde o painel de Salvos nasce e para onde ele recolhe. 0 quando
// a ilha nao esta na tela.
int  ilha_rect(float *x, float *y, float *w, float *h);
// 1 = outra superficie nasceu da pilula e esta no lugar dela (o painel de
// Salvos): a ilha continua medindo, mas nao se desenha. Vale um quadro.
void ilha_coberta(int coberta);

#endif
