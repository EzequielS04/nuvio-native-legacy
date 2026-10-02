// Painel "Salvos" — a camada da direita que a tecla AZUL abre. Ver salvospainel.h
// para o que ele substituiu e por que.
//
// O QUE ELE MOSTRA, e a decisao nao e obvia: a UNIAO das tres fontes de "quero
// ver", nao so a lista local. As tres caem na mesma marca (`CatItem.naLista`):
// a watchlist do Trakt (descoberta.c), a biblioteca da conta (contalib.c) e a
// lista local (salvos.c). A aba "Salvos" da tela de Biblioteca ja mostra essa
// uniao, e duas telas chamadas "Salvos" mostrando conjuntos diferentes seria
// exatamente o defeito que o app irmao teve com quatro botoes "+".
//
// A lista local entra por fora do catalogo de proposito. Ela guarda titulo,
// poster e meta no proprio arquivo (ver salvos.h), entao o painel se desenha no
// primeiro quadro do arranque — antes de a descoberta responder. Sem isso o
// atalho mais rapido do controle abriria vazio por ~20 s toda vez que a TV
// liga, que e justamente quando alguem aperta.
#include "salvospainel.h"
#include "salvos.h"
#include "salvosorg.h"
#include "teclado.h"
#include "simkl.h"
#include "recomenda.h"
#include "avisos.h"
#include "recenviar.h"
#include "pessoas.h"
#include "catalogo.h"
#include "ctxmenu.h"
#include "gfx.h"
#include "text.h"
#include "tex_cache.h"
#include "anim.h"
#include "layout.h"
#include "ajustes.h"
#include "idioma.h"
#include "badges.h"
#include "botoes.h"
#include "socialvis.h"
#include "svdesenho.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

// Mesma pegada do painel "Sua atividade" que ele substitui (perfil.c desenhava
// em x=1120, 776x1032): quem ja tinha o gesto na memoria muscular encontra a
// camada no mesmo lugar, so com outro conteudo.
#define SP_X          1120.0f
#define SP_W           776.0f
#define SP_Y            24.0f
#define SP_H          1032.0f
#define SP_PAD          NV_FOLHA_PAD
#define SP_INTERNO    (SP_W - SP_PAD * 2.0f)
// ABAS. Elas so existem quando o servico de recomendacoes foi compilado
// (recomenda_ativo); sem ele o painel e exatamente o que era, sem uma linha a
// mais de cromo para uma funcao que nao existe naquele pacote.
// AS ABAS OCUPAM O LUGAR DO TITULO GRANDE, e nao uma faixa a mais. A pilula
// acesa ja diz em que secao a pessoa esta — repetir isso num "Salvos" de 40px
// logo acima gastaria 76px de altura para dizer duas vezes a mesma coisa, e a
// lista comecaria mais embaixo em todo pacote com o servico ligado.
#define SP_ABAS_Y      (SP_Y + 56.0f)
#define SP_ABAS_H        NV_CTRL_H
#define SP_ABA_GAP       NV_CTRL_VAO
// 176, e nao 200: a secao ganhou 24 px de ar proprio acima do rotulo (ver
// SP_SECAO_H), entao a lista sobe para o primeiro rotulo nao ficar a 90 px
// das abas.
#define SP_LISTA_Y     176.0f
#define SP_LISTA_BASE (SP_Y + SP_H - 24.0f)
// AR DO FOCO entre o recorte da lista e a primeira linha. A superficie da linha
// focada nasce SP_FOCO_PADY ACIMA do `y` da linha (e o vidro poe o aro por
// cima disso), e a lista comecava exatamente no topo do recorte: a primeira
// linha em foco — e qualquer linha que a rolagem alinhava ao topo — saia com
// a borda de cima reta, sem os cantos (foto do dono, 01/10, aba Social com
// vidro; medido no tests/spainel_foco_shot: superficie cortada em y=176, o
// topo do recorte). O conteudo comeca SP_FOCO_AR abaixo do recorte e a
// rolagem guarda o mesmo ar em cima e embaixo da linha focada.
#define SP_FOCO_AR      14.0f
#define SP_POSTER_W     92.0f
#define SP_POSTER_H    138.0f
// 12 px entre linhas (dono, 21/09/2026); a pilula de foco tem 6 px de folga
// vertical, entao duas pilulas vizinhas se tocariam sem se sobrepor.
// 162 = cartao de 150 (cartaz 138 + 6 de cada lado) + NV_LINHA_VAO. Era 150:
// o cartao tinha a altura do passo e dois focos vizinhos encostavam.
#define SP_PASSO       162.0f
#define SP_FOCO_PADY     6.0f
// ROTULO DE SECAO: 24 px de ar acima, o texto (26), 8 px, uma linha de 1 px a
// 8 % de branco e 13 px ate a primeira linha. O ar maior em cima e o que
// separa "Continuar" da linha anterior sem precisar de caixa nenhuma.
#define SP_SECAO_AR     24.0f
#define SP_SECAO_H      72.0f
// 28, e nao 20: o ponto de "nao lida" mora neste vao, e com 20 ele encostava
// na primeira letra do titulo — foi o que o dono viu na foto ampliada.
#define SP_TEXTO_X    (SP_PAD + SP_POSTER_W + 28.0f)
#define SP_TEXTO_W    (SP_INTERNO - SP_POSTER_W - 28.0f)
// Barra de progresso do card de retomada: a mesma altura da que a home usa nos
// cards de "Continuar assistindo", para as duas lerem como a mesma coisa.
// Trilho de 300 e o resto da coluna para "T3E4 · 43 min restantes", que e o
// dado que importa e vai em tinta principal ao lado do trilho; o "≈35 min
// assistidos" e secundario e fica embaixo.
#define SP_BARRA_W     300.0f
#define SP_BARRA_LABEL_GAP 16.0f
#define SP_BARRA_H       6.0f
// Entrada e saida com o MESMO relogio do menu lateral (menu.c): as duas camadas
// aparecem no mesmo app e tempos diferentes se leem como bug, nao como estilo.
#define SP_ABRIR_MS    230.0f
#define SP_FECHAR_MS   150.0f
#define SP_VEU           0.58f
// O RASTRO. A linha focada aqui e um bloco inteiro na cor de realce, e nao um
// anel: com a mesma mola nos dois sentidos (95 % em 120 ms) e a tecla presa
// descendo a ~10 linhas/s, duas ou tres linhas acima da focada ainda estavam
// acesas pela metade — uma cauda de pilulas atras do foco. E a explicacao
// mais provavel para o "deixando rastro" do dono (24/09/2026), somada ao
// ritmo irregular de 52 fps; nao foi conferida na TV. A que perde o foco
// apaga em ~50 ms (95 %);
// a que ganha continua na mola do app. A mola e exp(-k*dt): o tempo e o mesmo
// a 60 ou a 30 quadros por segundo.
#define SP_MOLA_DESFOCO 60.0f

// Teto de linhas do painel. ERA 200, com a lista local podendo ter 300 e a
// conta mais 200: o que passasse de 200 sumia do painel sem aviso, e como a
// lista local vem primeiro na ordem de insercao, sumia justamente o mais novo.
// Agora cabe tudo que pode existir (lista local + catalogo inteiro); o vetor de
// linhas mora no heap e cresce so ate o que a lista de verdade tem.
#define SP_MAX (SALVOS_MAX + CAT_MAX)

// Linha ja resolvida: o desenho nao volta ao catalogo nem a lista local por
// quadro.
//
// OS TEXTOS SAO COPIADOS, E NAO APONTADOS — e isto derrubou o app na TV.
//
// A primeira versao guardava `const char *titulo` apontando para dentro do
// CatItem, com um comentario afirmando que a memoria era estavel. Nao e: o
// vetor `itens` de catalogo.c e do heap e TROCA DE BLOCO a cada republicacao
// (cat_definir_tudo/cat_acrescentar_lote fazem malloc do bloco novo e liberam o
// antigo). O proprio catalogo.c documenta isso na linha da troca e segura UM
// bloco velho em `lixo` justamente porque alguem ja leu memoria liberada ali —
// mas uma folga de um bloco nao salva quem guarda o ponteiro por varios ciclos.
//
// Na TV o resultado foi core dump de 218 MB alguns segundos depois do arranque,
// quando o segundo ciclo de sync republicou o catalogo (o log parava logo apos
// "[contalib] biblioteca da conta aplicada"). No Mac, sem conta, o catalogo
// nunca era republicado e nada acontecia — o defeito so existia com dados reais.
//
// Copiar custa ~800 bytes por linha, no heap e do tamanho da lista real. E o
// preco de nao depender do tempo de vida de um bloco que outro modulo troca sem
// avisar.
typedef struct {
  char  titulo[160], poster[512], meta[96];
  char  id[24];
  int   serie;
  int   nota;
  int   progresso, temporada, episodio, restanteMin;
  long long quandoS;      // 0 = veio do Trakt/conta, nao sabemos quando entrou
  // As duas legendas da barra de retomada, montadas na reconstrucao e nao a
  // cada quadro: dependem so do progresso, que so muda com o catalogo (e ai
  // a lista e reconstruida). A rasterizacao ja era cacheada em text.c — o log
  // da C9 dizia "texto 0.0ms em 0 linhas" com o painel aberto —; isto poupa
  // os snprintf e as buscas de i18n de cada linha visivel.
  char  txtRestante[96], txtVisto[96];
  // ORGANIZACAO (salvosorg.h). `fundo` e a arte 16:9 do catalogo, para o
  // estilo paisagem; vazio cai no cartaz. `tipoG` e o grupo de tipo (SPT_*),
  // `cat` a categoria da pessoa (0 = nenhuma), `orig` a posicao na ordem de
  // sempre — o desempate de toda ordenacao, para a lista nunca dancar.
  char  fundo[512];
  int   ano, tipoG, cat, orig;
  // ONDE A CELULA MORA, relativo ao topo do conteudo e a esquerda da coluna.
  // Calculado na reconstrucao (montarLayout), nunca por quadro: o desenho, a
  // rolagem e o D-pad perguntam ao mesmo lugar. `fila` e a linha visual —
  // na grade varias celulas dividem a mesma.
  float lx, ly, lw, lh;
  int   fila;
} SPLinha;
// Tipos para "Agrupar por tipo", na ordem das secoes.
enum { SPT_FILME = 0, SPT_SERIE, SPT_COLECAO, SPT_CANAL, SPT_N };

static SPLinha *linhas;
static int nLinhas, capLinhas;

// Vaga para `n` linhas. Cresce dobrando ate SP_MAX; 0 quando nao ha memoria, e
// quem chama para de acrescentar (a lista fica curta, mas nao corrompe).
static int garantirLinhas(int n) {
  SPLinha *novo;
  int cap;
  if (n <= capLinhas) return 1;
  if (n > SP_MAX) return 0;
  cap = capLinhas ? capLinhas * 2 : 64;
  while (cap < n) cap *= 2;
  if (cap > SP_MAX) cap = SP_MAX;
  novo = (SPLinha *)realloc(linhas, sizeof *linhas * (size_t)cap);
  if (!novo) return 0;
  linhas = novo;
  capLinhas = cap;
  return 1;
}
static int nCont;            // quantas das primeiras linhas sao "Continuar"

// A ABA SOCIAL. `foco == SP_FOCO_ABAS` e a linha de cima, onde esquerda e
// direita trocam de aba; do zero para baixo o D-pad e o de sempre. Uma linha
// de foco "fora da lista" em vez de um modo separado porque o resto do painel
// (rolagem, animacao de foco, recorte) continua valendo sem mudanca nenhuma.
// A ABA AVISOS e a central de avisos (avisos.h) dentro deste painel, para
// abrir quando se quiser e nao so no toast. Existe SEMPRE; a Social so com o
// servico de recomendacoes.
// ATIVIDADE (02/10/2026, tela B do desenho aprovado): o feed dos amigos
// agrupado por dia, entre Salvos e a antiga Social — que passou a se chamar
// AMIGOS, porque e isso que ela lista (recomendacoes recebidas, sugestoes e a
// lista de amigos). O nome interno SP_ABA_SOCIAL ficou: e o mesmo conteudo.
enum { SP_ABA_SALVOS = 0, SP_ABA_ATIVIDADE = 1, SP_ABA_SOCIAL = 2, SP_ABA_AVISOS = 3,
       SP_ABA_N = 4 };
#define SP_FOCO_ABAS (-1)
static int aba;
static RecItem recs[REC_MAX];
static int nRecs;

// A ABA SOCIAL DEIXOU DE SER UMA LISTA SO. Ela tem agora quatro tipos de linha
// com ALTURAS DIFERENTES, e por isso existe este vetor em vez de um indice
// direto na lista de recomendacoes: a rolagem, o foco e o desenho tem de
// concordar sobre onde comeca a linha `i`, e a unica forma de garantir isso e
// as tres perguntarem ao MESMO lugar.
//
//   SPS_CONSENT_NAO / _SIM  as duas respostas da pergunta de primeira entrada
//   SPS_REC                 uma recomendacao recebida
//   SPS_SUG                 alguem que a pessoa talvez conheca
//   SPS_ADICIONAR           "Adicionar um amigo"
//   SPS_APARECER            o interruptor de "apareco para os outros?"
//
// COM A PERGUNTA NA TELA A LISTA TEM SO DUAS LINHAS, as duas respostas. Nao e
// uma tela separada com laco proprio: o D-pad, a rolagem, a animacao de foco e
// o recorte do painel ja funcionam para linhas, e uma segunda maquina de estado
// para duas pilulas divergiria da primeira na primeira correcao.
//   SPS_ENCONTRAR           "Encontrar pessoas" (busca, perfil, pedidos; pessoas.c)
//   SPS_AMIGO               um contato ja adicionado (foto + nome), sob o
//                           cabecalho "Seus amigos" (dono, 20/09/2026)
enum { SPS_CONSENT_NAO = 0, SPS_CONSENT_SIM, SPS_REC, SPS_SUG,
       SPS_ADICIONAR, SPS_APARECER, SPS_AMIGO, SPS_ENCONTRAR,
       // QUEM VE O QUE EU ASSISTO (socialsrv, recomenda_alcance): as tres
       // respostas da pergunta, a linha que reabre a pergunta e o nome.
       SPS_ALC_0, SPS_ALC_1, SPS_ALC_2, SPS_ALCANCE, SPS_NOME };
// A API do alcance e do nome so existe no socialsrv (branch agente/socialsrv).
// Ate o merge as linhas ficam desligadas; NV_SOCIAL_V2_UI liga so a tela (o
// teste de captura o usa com a API de mentira).
#if defined(NV_SOCIAL_V2) || defined(NV_SOCIAL_V2_UI)
#define SP_V2 1
#else
#define SP_V2 0
#endif
typedef struct { unsigned char tipo; short idx; } SPSocial;
#define SP_SOCIAL_MAX (REC_MAX + REC_SUGESTOES_MAX + REC_CONTATOS_MAX + 8)
static SPSocial social[SP_SOCIAL_MAX];
static int nSocial;
static RecSugestao sugs[REC_SUGESTOES_MAX];
static int nSugs;
static RecContato ctts[REC_CONTATOS_MAX];
static int nCtts;
// Retrato do estado do consentimento na ultima reconstrucao. O fio de rede pode
// adotar um "sim" respondido em OUTRA TV no meio de um ciclo (ver a
// reconciliacao em recomenda.c), e sem esta marca a pergunta continuaria na
// tela depois de ja ter sido respondida.
static int consentEstado = -1;
// O nivel na ultima reconstrucao, e 1 enquanto a pessoa reabriu a pergunta.
#if SP_V2
static int alcEstado = -2, escolhendoAlcance;
#endif
#define SPS_ALC_TOPO 262.0f

// A LINHA DO AMIGO SABE O QUE ELE ESTA FAZENDO (tela A, 02/10/2026): o indice
// dele no modelo do social (socialvis.h; -1 = sem atividade) e se ha uma
// recomendacao MINHA para ele, que ganha a cadeia "Voce mandou X › viu ›
// gostou" e por isso uma linha mais alta. Refeito com a lista (reconstruirSocial)
// e quando o modelo muda.
static short cttSv[REC_CONTATOS_MAX];
static unsigned char cttCadeia[REC_CONTATOS_MAX];
static unsigned svRevSocial = ~0u;

// A ABA ATIVIDADE (tela B): o feed de socialvis, uma linha por evento, com o
// rotulo do dia ("Agora", "Hoje", "Ontem", a data, "Recentes") antes da
// primeira linha de cada dia. Os rotulos sao montados aqui, uma vez por
// mudanca do modelo, e nao por quadro.
static int nAtv;
static unsigned char atvDia[SV_EVENTOS_MAX];
static char atvRot[SV_EVENTOS_MAX][32];
static unsigned atvRev = ~0u;
// O perfil pedido pela linha do amigo (spainel_pediu_perfil).
static char pedidoPerfil[96];
static int temPedidoPerfil;

// Alturas das linhas novas. A recomendacao mantem SP_POSTER_H + SPS_GAP, que e
// exatamente o SP_PASSO de antes — a aba nao mudou de ritmo, so ganhou vizinhos.
// AMIGO E MAIS COMPACTO: ele nao tem poster, selo nem botao, so identidade e a
// ultima atividade. Dar a ele os mesmos 112px da sugestao fazia uma linha
// simples parecer um cartao de destaque.
#define SPS_GAP         18.0f
#define SPS_H_CONSENT   84.0f
#define SPS_H_SUG      112.0f
#define SPS_H_AMIGO     88.0f
#define SPS_H_ACAO      60.0f
#define SPS_H_APARECER  88.0f
// Alturas dos dois blocos de texto que NAO sao linha e por isso nao recebem
// foco: o enunciado da pergunta e a explicacao do estado vazio. Sao constantes
// e nao medidas porque a rolagem precisa delas ANTES do desenho — e as duas
// foram conferidas na captura, que e o unico juiz util aqui.
#define SPS_CONSENT_TOPO 400.0f
#define SPS_VAZIO_TOPO   320.0f

// O INTERRUPTOR DE "APARECER". As medidas sao para TRES METROS, e nao copiadas
// de um telefone.
//
// A conta: numa TV de 55" a 3 m, 80 px ainda deixam a trilha claramente
// distinguivel, mas tiram o peso de um controle de telefone ampliado. Como o
// foco da TV transforma a linha inteira em alvo, o switch pode ser visualmente
// compacto sem reduzir a area de acao.
//
// A bola e 30 px com 5 px de folga de cada lado: continua legivel e deixa a
// capsula respirar sem competir com o titulo.
#define SPS_SW_W        80.0f
#define SPS_SW_H        40.0f
#define SPS_SW_PAD       5.0f
#define SPS_SW_BOLA    (SPS_SW_H - SPS_SW_PAD * 2.0f)
#define SPS_SW_GAP      24.0f   // do fim do texto ate a trilha
// VAO EXTRA ANTES DO INTERRUPTOR. Os 18 px de SPS_GAP separam linhas do MESMO
// tipo; aqui a lista de gente e de acoes acaba e comeca um ajuste que fica.
// Sao 10 px, e nao um cabecalho de secao: um rotulo ali repetiria o titulo da
// propria linha, que e exatamente o ar de formulario que se quer evitar.
#define SPS_SEP_APARECER 10.0f
// A linha da Atividade: rosto pequeno, cartaz 64x96 e tres linhas de texto.
#define SPA_H         112.0f
#define SPA_AV         44.0f
#define SPA_PW         64.0f
#define SPA_PH         96.0f
// A cadeia sob a linha do amigo: a pilula (SVD_CHIP_H) e o ar ate ela.
#define SPS_CADEIA_H   42.0f

static int aberto, foco, marcaCatN = -1;
static float entrada, scrollY;
// Velocidade da rolagem de 2a ordem (anim_mola2): partida macia, como na home.
static float velY;
static float animFoco[SP_MAX];
// POSICAO DA BOLA DO INTERRUPTOR, 0 = desligado, 1 = ligado. E estado PROPRIO
// e nao uma leitura direta de recomenda_aparecer() por um motivo que e a razao
// de ser desta linha inteira: o deslize e a unica coisa na tela que responde
// "o OK MUDOU alguma coisa" em vez de "o OK ABRIU alguma coisa". Sem ele o
// desenho pularia entre dois retratos e voltaria a ser um botao.
// -1 = ainda nao lido; a primeira atualizacao assenta sem deslizar do nada.
static float animSw = -1.0f;
static char  pedido[24];
static int   temPedido;

// SEGURAR OK NUMA LINHA DE "SALVOS" abre o menu do cartaz (ctxmenu.c, modo
// painel): remover, mais informacoes, assistido. O toque curto continua
// abrindo o titulo — mas agora na SOLTURA, e nao no KEYDOWN: so no KEYUP se
// sabe quanto o dedo ficou. E a mesma medida da home e da Agenda (NV_HOLD_MS),
// e o menu abre NO LIMIAR, com o dedo ainda no botao, como na home: esperar a
// soltura deixaria a barra cheia na tela sem nada acontecer.
// 0 = nenhum OK afundado numa linha. So a aba Salvos arma; as outras abas
// continuam decidindo no KEYDOWN, porque nelas nao ha o que segurar.
static Uint32 okDesde;
// O FOCO SEGUE A REMOCAO. Ao abrir o menu, o painel guarda o titulo e o que vem
// logo depois dele; quando a lista remontar sem o titulo, o foco vai para o
// seguinte — e nao para "o mesmo indice", que depois de uma remocao pelo Trakt
// (a linha local sai antes, a copia do catalogo so no 2xx) apontaria para
// outra coisa no meio do caminho.
static char   menuId[24], menuProximo[24];
// O FOCO SEGUE O TITULO MOVIDO. Mover para uma categoria (com a lista agrupada
// por categoria) muda a posicao dele; o foco vai junto, na reconstrucao.
static char   seguirId[24];

int spainel_aberto(void)  { return aberto; }
static int abaExiste(int a);
static void trocarAba(int nova);
static int nVisiveis(void);
void spainel_ir_aba(int a) {
  if (!aberto) spainel_abrir();
  if (a < SP_ABA_SALVOS || a >= SP_ABA_N || !abaExiste(a)) return;
  if (a != aba) trocarAba(a);
  foco = nVisiveis() > 0 ? 0 : SP_FOCO_ABAS;
}
int spainel_pediu_perfil(char *id, size_t tam) {
  if (!temPedidoPerfil) return 0;
  temPedidoPerfil = 0;
  if (id && tam) snprintf(id, tam, "%s", pedidoPerfil);
  return 1;
}
int spainel_visivel(void) { return aberto || entrada > 0.002f; }

const char *spainel_pediu_abrir(void) {
  if (!temPedido) return NULL;
  temPedido = 0;
  return pedido;
}

static int ehSerie(const char *tipo, int nTemporadas) {
  return (tipo && !strcmp(tipo, "series")) || nTemporadas > 0;
}

// O QUE A LISTA MONTADA VIU: as revisoes do catalogo e da lista local no
// instante da reconstrucao. Reconstruir quando uma delas sobe, e so entao.
//
// ERA `cat_n() != marcaCatN || catTrocou()` — contagem e o id do item 0. O
// retrato tinha dois defeitos opostos: nao via mudanca de MARCA (um titulo
// salvo pela conta com o painel aberto nao aparecia) e, quando disparava, a
// reconstrucao andava pelo catalogo inteiro. Com as revisoes a pergunta por
// quadro e comparar dois inteiros, e a reconstrucao so acontece quando ha o
// que mostrar de diferente (tests/salvospainel.sh conta quantas vezes).
static unsigned marcaRevCat, marcaRevSalvos, marcaRevOrg;
static int reconstrucoes, fundosPintados;
int spainel_n_reconstrucoes(void) { return reconstrucoes; }
int spainel_n_fundos(void) { return fundosPintados; }
static int listaVelha(void) {
  sorg_carregar();   // troca de perfil sobe a revisao da organizacao
  return marcaCatN < 0 || sorg_revisao() != marcaRevOrg || cat_revisao_itens() != marcaRevCat ||
         salvos_revisao() != marcaRevSalvos || cat_n() != marcaCatN;
}

// Monta a lista visivel. A uniao (lista local + catalogo) vem de salvos_uniao,
// e a reordenacao e daqui:
//   1. a lista LOCAL, na ordem de insercao (ela existe mesmo sem catalogo);
//   2. cada TITULO que o catalogo tem marcado como naLista e ainda nao entrou;
//   3. os itens COM progresso sobem para o topo, virando a secao "Continuar".
// A reordenacao e uma insercao estavel: dentro de cada secao a ordem das duas
// passadas e preservada, senao a lista dancaria a cada reconstrucao.
//
// A DEDUPLICACAO NAO E MAIS DAQUI. Ela era um strcmp dos ids ja postos
// (`jaTem`), e o catalogo guarda a serie com progresso como "tt123:1:2" ao lado
// do "tt123" da lista local e da conta: Widows Bay aparecia duas vezes, as duas
// no mesmo episodio, porque a linha local puxava o progresso daquela copia e a
// copia entrava de novo por conta propria. salvos_uniao compara por titulo, e e
// a mesma regra que tests/salvos.sh cobra.
static void legendasDaBarra(SPLinha *l);
static int tipoGrupo(const char *tipo, const char *id, int serie);
static int anoDe(const char *meta);
static void organizar(void);
static void reconstruir(void) {
  static SalvosEntrada *uniao;
  static int capUniao;
  int i, n, escrita = 0;
  // As revisoes sao lidas ANTES de ler a lista: se a descoberta mudar o
  // catalogo no meio desta reconstrucao, a revisao guardada fica velha e o
  // quadro seguinte reconstroi de novo, em vez de guardar a nova e perder a
  // mudanca.
  unsigned revCat = cat_revisao_itens(), revSalvos = salvos_revisao();
  int catN = cat_n();
  reconstrucoes++;
  nLinhas = 0;
  n = salvos_n() + cat_n();
  if (n > SP_MAX) n = SP_MAX;
  if (n > capUniao) {
    SalvosEntrada *novo = (SalvosEntrada *)realloc(uniao, sizeof *uniao * (size_t)n);
    if (novo) { uniao = novo; capUniao = n; }
  }
  n = salvos_uniao(uniao, capUniao);
  for (i = 0; i < n && nLinhas < SP_MAX; i++) {
    const SalvoItem *s = uniao[i].local >= 0 ? salvos_item(uniao[i].local) : NULL;
    const CatItem *c = uniao[i].cat >= 0 ? cat_item(uniao[i].cat) : NULL;
    SPLinha *l;
    if (!s && !c) continue;
    if (!garantirLinhas(nLinhas + 1)) break;
    l = &linhas[nLinhas++];
    memset(l, 0, sizeof *l);
    if (s) {
      snprintf(l->id, sizeof l->id, "%s", s->id);
      snprintf(l->titulo, sizeof l->titulo, "%s", s->titulo);
      snprintf(l->poster, sizeof l->poster, "%s", s->poster);
      snprintf(l->meta, sizeof l->meta, "%s", s->meta);
      l->nota   = s->nota;
      l->quandoS = s->quandoS;
      l->serie  = ehSerie(s->tipo, 0);
    } else {
      snprintf(l->id, sizeof l->id, "%s", c->imdb);
      snprintf(l->titulo, sizeof l->titulo, "%s", c->titulo);
      snprintf(l->poster, sizeof l->poster, "%s", c->poster);
      snprintf(l->meta, sizeof l->meta, "%s", c->meta);
      l->nota   = c->nota;
    }
    // O PROGRESSO SO EXISTE NO CATALOGO. A lista local guarda o que e dela
    // (titulo, poster, quando entrou); posicao de retomada e de progresso.c e
    // muda sem passar por aqui. Guardar uma copia envelheceria em minutos.
    if (c) {
      l->progresso = c->progresso;
      l->temporada = c->temporada;
      l->episodio  = c->episodio;
      l->restanteMin = c->restanteMin;
      if (c->nota > 0) l->nota = c->nota;
      if (c->poster[0]) snprintf(l->poster, sizeof l->poster, "%s", c->poster);
      if (c->meta[0])   snprintf(l->meta, sizeof l->meta, "%s", c->meta);
      if (ehSerie(c->tipo, c->nTemporadas)) l->serie = 1;
      if (c->backdrop[0]) snprintf(l->fundo, sizeof l->fundo, "%s", c->backdrop);
    }
    l->tipoG = tipoGrupo(s ? s->tipo : c->tipo, l->id, l->serie);
    l->ano = anoDe(l->meta);
    l->cat = sorg_categoria_de(l->id);
  }
  for (i = 0; i < nLinhas; i++) { legendasDaBarra(&linhas[i]); linhas[i].orig = i; }
  // A ORDEM DE SEMPRE era: a uniao na ordem acima e, por cima, quem tem
  // progresso subindo para "Continuar". Agora a ordem e o agrupamento sao da
  // pessoa (salvosorg.h) — e o padrao dos dois reproduz exatamente aquilo.
  for (i = 0; i < nLinhas; i++) if (linhas[i].progresso > 0) escrita++;
  nCont = escrita;
  organizar();
  marcaRevOrg = sorg_revisao();
  marcaCatN = catN;
  marcaRevCat = revCat;
  marcaRevSalvos = revSalvos;
  // O FOCO DAS ABAS (-1) NAO E UM FOCO FORA DA FAIXA. Sem esta guarda, uma
  // reconstrucao com a lista vazia jogaria o foco de volta para a linha 0, que
  // nao existe, e a linha de abas perderia o anel debaixo do dedo.
  if (menuId[0] && foco >= 0) {
    // Com o menu do painel no ar (ou acabando de sair), o foco vai por
    // IDENTIDADE: fica no titulo enquanto ele existir, e cai no seguinte quando
    // ele sair. A animacao de foco nao e zerada — a linha que chega ao lugar
    // acende pela mola, sem piscar.
    int achou = -1, prox = -1;
    for (i = 0; i < nLinhas; i++) {
      if (achou < 0 && salvos_mesmo_titulo(linhas[i].id, menuId)) achou = i;
      if (prox < 0 && menuProximo[0] && salvos_mesmo_titulo(linhas[i].id, menuProximo)) prox = i;
    }
    if (achou >= 0) foco = achou;
    else if (prox >= 0) foco = prox;
  }
  if (seguirId[0] && foco >= 0) {
    for (i = 0; i < nLinhas; i++)
      if (salvos_mesmo_titulo(linhas[i].id, seguirId)) { foco = i; break; }
    seguirId[0] = 0;
  }
  if (foco >= 0 && foco >= nLinhas) foco = nLinhas > 0 ? nLinhas - 1 : 0;
}

// --- ORGANIZAR: grupos, ordem e o lugar de cada celula ------------------------
//
// Tudo isto roda NA RECONSTRUCAO, e so nela: a lista muda quando o catalogo, a
// lista local ou a organizacao mudam (as tres revisoes de listaVelha), e entre
// uma mudanca e outra o quadro so le `ly`/`lh` prontos. Com 300 salvos a conta
// inteira e uma ordenacao de 300 indices — nada que apareca num quadro.

// Secoes da lista montada: o rotulo e onde ele mora. `vazia` = categoria sem
// titulo nenhum, que ainda assim aparece (com uma dica no lugar das celulas):
// quem acabou de criar "Kids" tem de ver "Kids" na tela.
#define SP_SECOES_MAX (SORG_CAT_MAX + 8)
typedef struct { float y; char rot[SORG_NOME_MAX + 8]; int vazia; } SPSecao;
static SPSecao secoes[SP_SECOES_MAX];
static int nSecoes;
static float alturaConteudo;

// As medidas dos tres estilos. A coluna util e SP_INTERNO (696).
//   LISTA     a de sempre: cartaz 92x138 e texto ao lado, passo SP_PASSO.
//   GRADE     4 cartazes por fila, 159x238, titulo embaixo. 4 e nao 5: a 3 m,
//             um cartaz de 130 px ja nao deixa ler o titulo de baixo.
//   PAISAGEM  2 cartoes 16:9 por fila, 338x190, titulo e linha de apoio.
#define SPG_COLS        4
#define SPG_VAO        20.0f
#define SPG_W         ((SP_INTERNO - SPG_VAO * (SPG_COLS - 1)) / SPG_COLS)
#define SPG_POSTER_H  (SPG_W * 1.5f)
#define SPG_H         (SPG_POSTER_H + 48.0f)
#define SPG_PASSO     (SPG_H + 24.0f)
#define SPP_COLS        2
#define SPP_VAO        20.0f
#define SPP_W         ((SP_INTERNO - SPP_VAO * (SPP_COLS - 1)) / SPP_COLS)
#define SPP_IMG_H     (SPP_W * 9.0f / 16.0f)
#define SPP_H         (SPP_IMG_H + 82.0f)
#define SPP_PASSO     (SPP_H + 24.0f)
// Altura da dica de uma categoria vazia, abaixo do rotulo.
#define SP_VAZIA_H     64.0f

// O que o tipo do catalogo diz, reduzido aos quatro grupos que a pessoa ve.
// Canal: os addons de TV declaram "tv" ou "channel", e o id de canal do app e
// "cs:channel:..." (ver idbase.h). Colecao: o tipo pode chegar cortado em 8
// bytes ("collect"), entao basta o prefixo.
static int tipoGrupo(const char *tipo, const char *id, int serie) {
  if (tipo && (!strcmp(tipo, "tv") || !strcmp(tipo, "channel"))) return SPT_CANAL;
  if (id && !strncmp(id, "cs:", 3)) return SPT_CANAL;
  if (tipo && !strncmp(tipo, "coll", 4)) return SPT_COLECAO;
  if (serie) return SPT_SERIE;
  return SPT_FILME;
}

// O ano da meta ("2002", "2022 · 3 temporadas"): o primeiro numero de quatro
// digitos entre 1900 e 2099. 0 quando a meta nao traz ano.
static int anoDe(const char *meta) {
  const char *p;
  if (!meta) return 0;
  for (p = meta; *p; p++) {
    if (p[0] >= '0' && p[0] <= '9' && p[1] >= '0' && p[1] <= '9' &&
        p[2] >= '0' && p[2] <= '9' && p[3] >= '0' && p[3] <= '9' &&
        !(p[4] >= '0' && p[4] <= '9') && (p == meta || !(p[-1] >= '0' && p[-1] <= '9'))) {
      int a = (p[0] - '0') * 1000 + (p[1] - '0') * 100 + (p[2] - '0') * 10 + (p[3] - '0');
      if (a >= 1900 && a <= 2099) return a;
    }
  }
  return 0;
}

// Em que secao a linha cai, no agrupamento atual. Numero menor = mais acima.
static int grupoDe(const SPLinha *l, int g) {
  if (g == SORG_GRUPO_PROGRESSO) return l->progresso > 0 ? 0 : 1;
  if (g == SORG_GRUPO_TIPO) return l->tipoG;
  if (g == SORG_GRUPO_CATEGORIA) {
    int k = l->cat ? sorg_categoria_indice(l->cat) : -1;
    return k >= 0 ? k : SORG_CAT_MAX;   // "Sem categoria" fecha a lista
  }
  return 0;
}

// Quanto falta, para "Menos tempo restante": o minuto quando ha, senao o
// percentual que falta (escala alta, para cair depois de quem tem minuto).
static int restanteDe(const SPLinha *l) {
  if (l->progresso <= 0) return 1 << 30;
  if (l->restanteMin > 0) return l->restanteMin;
  return 100000 + (100 - l->progresso);
}

static int ordemAtual, grupoAtual;
static int compara(const void *pa, const void *pb) {
  const SPLinha *a = &linhas[*(const int *)pa], *b = &linhas[*(const int *)pb];
  int ga = grupoDe(a, grupoAtual), gb = grupoDe(b, grupoAtual), d = 0;
  if (ga != gb) return ga - gb;
  switch (ordemAtual) {
    case SORG_ORDEM_RECENTES:
      // Do Trakt ou da conta nao sabemos quando entrou (quandoS 0): depois.
      d = (b->quandoS > a->quandoS) - (b->quandoS < a->quandoS);
      break;
    case SORG_ORDEM_NOME: d = strcasecmp(a->titulo, b->titulo); break;
    case SORG_ORDEM_ANO:  d = b->ano - a->ano; break;
    case SORG_ORDEM_NOTA: d = b->nota - a->nota; break;
    case SORG_ORDEM_RESTANTE: {
      int ra = restanteDe(a), rb = restanteDe(b);
      d = (ra > rb) - (ra < rb);
      break; }
    default: break;
  }
  // O DESEMPATE E A ORDEM DE SEMPRE: qsort nao e estavel, e sem isto dois
  // titulos sem ano trocariam de lugar a cada reconstrucao.
  return d ? d : a->orig - b->orig;
}

// Rotulo da secao `g` no agrupamento atual. Passa por i18n no desenho
// (txt_linha), exceto o nome de categoria, que e da pessoa.
static void rotuloGrupo(int g, char *dst, size_t tam) {
  static const char *tipos[SPT_N] = { "Filmes", "Séries", "Coleções", "Canais" };
  if (grupoAtual == SORG_GRUPO_PROGRESSO)
    snprintf(dst, tam, "%s", g == 0 ? "Continuar" : (nCont > 0 ? "Não começados" : "Sua lista"));
  else if (grupoAtual == SORG_GRUPO_TIPO)
    snprintf(dst, tam, "%s", g >= 0 && g < SPT_N ? tipos[g] : "Sua lista");
  else if (grupoAtual == SORG_GRUPO_CATEGORIA) {
    const char *n = g < SORG_CAT_MAX ? sorg_categoria_nome_id(sorg_categoria_id(g)) : NULL;
    snprintf(dst, tam, "%s", n ? n : (sorg_n_categorias() > 0 ? "Sem categoria" : "Sua lista"));
  } else snprintf(dst, tam, "%s", "Sua lista");
}

static int novaSecao(float y, int g, int vazia) {
  SPSecao *sc;
  if (nSecoes >= SP_SECOES_MAX) return 0;
  sc = &secoes[nSecoes++];
  sc->y = y;
  sc->vazia = vazia;
  rotuloGrupo(g, sc->rot, sizeof sc->rot);
  return 1;
}

// Poe cada linha no lugar: secoes, filas, colunas.
static void montarLayout(void) {
  int estilo = sorg_estilo(), cols = 1, i, col = 0, fila = -1, gAnt = -999;
  int catVazias = grupoAtual == SORG_GRUPO_CATEGORIA;
  int proximaCat = 0;   // categorias vazias entram na ordem, entre as cheias
  float w = SP_INTERNO, h = SP_POSTER_H, passo = SP_PASSO, vao = 0.0f, y = 0.0f;
  if (estilo == SORG_ESTILO_GRADE) { cols = SPG_COLS; w = SPG_W; h = SPG_H; passo = SPG_PASSO; vao = SPG_VAO; }
  else if (estilo == SORG_ESTILO_PAISAGEM) { cols = SPP_COLS; w = SPP_W; h = SPP_H; passo = SPP_PASSO; vao = SPP_VAO; }
  nSecoes = 0;
  for (i = 0; i < nLinhas; i++) {
    SPLinha *l = &linhas[i];
    int g = grupoDe(l, grupoAtual);
    if (g != gAnt) {
      if (col > 0) { y += passo; col = 0; }
      // As categorias criadas e ainda vazias que vem antes desta secao.
      if (catVazias)
        for (; proximaCat < sorg_n_categorias() && proximaCat < g; proximaCat++) {
          novaSecao(y, proximaCat, 1);
          y += SP_SECAO_H + SP_VAZIA_H;
        }
      if (catVazias && g < SORG_CAT_MAX) proximaCat = g + 1;
      novaSecao(y, g, 0);
      y += SP_SECAO_H;
      gAnt = g;
    }
    if (col == 0) fila++;
    l->lx = (float)col * (w + vao);
    l->ly = y;
    l->lw = w;
    l->lh = h;
    l->fila = fila;
    if (++col >= cols) { col = 0; y += passo; }
  }
  if (col > 0) y += passo;
  if (catVazias)
    for (; proximaCat < sorg_n_categorias(); proximaCat++) {
      novaSecao(y, proximaCat, 1);
      y += SP_SECAO_H + SP_VAZIA_H;
    }
  alturaConteudo = y;
}

static void organizar(void) {
  static int *idx;
  static int capIdx;
  SPLinha *tmp;
  int i;
  ordemAtual = sorg_ordem();
  grupoAtual = sorg_grupo();
  // "Por categoria" sem nenhuma categoria criada e o mesmo que nenhum grupo:
  // uma secao "Sem categoria" sozinha nao diz nada.
  if (grupoAtual == SORG_GRUPO_CATEGORIA && sorg_n_categorias() < 1) grupoAtual = SORG_GRUPO_NENHUM;
  if (nLinhas > 1 && (ordemAtual != SORG_ORDEM_SALVOU || grupoAtual != SORG_GRUPO_NENHUM)) {
    if (nLinhas > capIdx) {
      int *n = (int *)realloc(idx, sizeof *idx * (size_t)nLinhas);
      if (n) { idx = n; capIdx = nLinhas; }
    }
    tmp = nLinhas <= capIdx ? (SPLinha *)malloc(sizeof *tmp * (size_t)nLinhas) : NULL;
    if (tmp) {
      for (i = 0; i < nLinhas; i++) idx[i] = i;
      qsort(idx, (size_t)nLinhas, sizeof *idx, compara);
      for (i = 0; i < nLinhas; i++) tmp[i] = linhas[idx[i]];
      memcpy(linhas, tmp, sizeof *tmp * (size_t)nLinhas);
      free(tmp);
    }
  }
  montarLayout();
}

// 1 quando o pacote tem o servico de recomendacoes. Com 0 nao ha aba, nao ha
// selo e nao ha uma linha de rede: o dono publica builds sem NUVIO_REC_URL.
static int temAbas(void) { return 1; }
// A Social so existe com o servico; sem ele as abas sao Salvos e Avisos.
static int temSocial(void) { return recomenda_ativo(); }
// A Atividade existe quando ha de onde ela vir: o servico, ou o Trakt ja ter
// trazido gente (socialvis.h).
static int temAtividade(void) {
  return temSocial() || socialvis_n_eventos() > 0 || socialvis_n_amigos() > 0;
}
static int abaExiste(int a) {
  if (a == SP_ABA_SOCIAL) return temSocial();
  if (a == SP_ABA_ATIVIDADE) return temAtividade();
  return a >= SP_ABA_SALVOS && a < SP_ABA_N;
}
static int proximaAba(int de, int dir) {
  int a = de + dir;
  while (a > SP_ABA_SALVOS && a < SP_ABA_AVISOS && !abaExiste(a)) a += dir;
  if (a < SP_ABA_SALVOS) return de;
  if (a > SP_ABA_AVISOS) return de;
  return a;
}

// Quantas linhas a aba corrente desenha. Uma funcao so para as duas, senao a
// rolagem e o desenho divergem na primeira mudanca.
static int nVisiveis(void) {
  // A ABA SOCIAL TEM SEMPRE UMA LINHA A MAIS: "Adicionar um amigo".
  //
  // Vazia, ela era uma frase dizendo que nao havia nada e mais nada — o D-pad
  // nao tinha para onde descer, e esse e exatamente o estado em que o dono
  // ficou preso (1 pessoa registrada, 0 contatos no servidor). Cheia, a tela
  // de amigos so seria alcancavel pelo menu de um cartaz — ou seja, para
  // adicionar alguem era preciso escolher um filme primeiro.
  if (aba == SP_ABA_SOCIAL) return nSocial;
  if (aba == SP_ABA_AVISOS) return avisos_lista_n();
  if (aba == SP_ABA_ATIVIDADE) return nAtv;
  return nLinhas;
}

// A BARRA DE OPCOES (Ordenar, Agrupar, Estilo, categorias) mora entre as abas
// e a lista, e empurra a lista SP_OPC_EXTRA para baixo so quando existe.
#define SP_FOCO_BARRA  (-2)
#define SP_OPC_Y     (SP_ABAS_Y + SP_ABAS_H + 12.0f)
#define SP_OPC_H      60.0f
#define SP_OPC_EXTRA (SP_OPC_Y + SP_OPC_H + 4.0f - SP_LISTA_Y)
static int temBarra(void);
static float listaTopo(void) { return SP_LISTA_Y + (temBarra() ? SP_OPC_EXTRA : 0.0f); }

// 1 enquanto a pergunta de primeira entrada esta na tela.
static int consentindo(void) {
  return aba == SP_ABA_SOCIAL && nSocial > 0 && social[0].tipo == SPS_CONSENT_NAO;
}
// 1 enquanto a pergunta do nivel (alcance) esta na tela.
static int perguntandoAlcance(void) {
  return aba == SP_ABA_SOCIAL && nSocial > 0 && social[0].tipo == SPS_ALC_0;
}

static float socialAlt(int i) {
  if (i < 0 || i >= nSocial) return 0.0f;
  switch (social[i].tipo) {
    case SPS_REC:       return SP_POSTER_H;
    case SPS_SUG:       return SPS_H_SUG;
    case SPS_AMIGO:     return SPS_H_AMIGO +
                          ((social[i].idx >= 0 && social[i].idx < REC_CONTATOS_MAX &&
                            cttCadeia[social[i].idx]) ? SPS_CADEIA_H : 0.0f);
    case SPS_ADICIONAR: return SPS_H_ACAO;
    case SPS_ENCONTRAR: return SPS_H_ACAO;
    case SPS_APARECER:  return SPS_H_APARECER;
    case SPS_ALCANCE:   return SPS_H_APARECER;
    case SPS_NOME:      return SPS_H_APARECER;
    default:            return SPS_H_CONSENT;
  }
}

// Espaco ANTES da linha `i`, quando houver. Sao dois casos, e so um deles
// carrega texto:
//   SPS_SUG  o cabecalho que separa as recomendacoes das sugestoes. Sem ele,
//            um nome desconhecido apareceria logo abaixo de uma recomendacao
//            de um amigo e leria como remetente.
//   SPS_APARECER  vao mudo. Ver SPS_SEP_APARECER.
// Quem desenha tem de olhar o tipo para saber se escreve o rotulo — um vao
// mudo com o cabecalho das sugestoes por cima seria pior que vao nenhum.
static float socialAntes(int i) {
  if (i < 0 || i >= nSocial) return 0.0f;
  // POR PESSOA: um rotulo com o nome de quem mandou antes da primeira
  // recomendacao de cada pessoa.
  if (social[i].tipo == SPS_REC && sorg_social() == SORG_SOCIAL_PESSOA) {
    if (i == 0 || social[i - 1].tipo != SPS_REC) return SP_SECAO_H;
    return strcmp(recs[social[i].idx].de, recs[social[i - 1].idx].de) ? SP_SECAO_H : 0.0f;
  }
  if (social[i].tipo == SPS_APARECER) return SPS_SEP_APARECER;
  if (social[i].tipo == SPS_NOME) return SPS_SEP_APARECER;
  if (social[i].tipo != SPS_SUG && social[i].tipo != SPS_AMIGO) return 0.0f;
  return (i == 0 || social[i - 1].tipo != social[i].tipo) ? SP_SECAO_H : 0.0f;
}

// Altura do bloco de texto que abre a lista e nao recebe foco.
static float socialTopo(void) {
  if (aba != SP_ABA_SOCIAL) return 0.0f;
  if (consentindo()) return SPS_CONSENT_TOPO;
  if (perguntandoAlcance()) return SPS_ALC_TOPO;
  return nRecs == 0 ? SPS_VAZIO_TOPO : 0.0f;
}

// Copia a lista de recomendacoes para dentro do painel. COPIA, e nao ponteiro:
// a lista de recomenda.c vive atras de um mutex que o fio de rede reescreve, e
// e exatamente o erro que derrubou este arquivo antes (ver a nota longa em
// SPLinha).
static void reconstruirSocial(void) {
  int i;
  nRecs = 0;
  nSugs = 0;
  nCtts = 0;
  nSocial = 0;
  consentEstado = -1;
  if (!temAbas()) return;
  consentEstado = recomenda_aparecer();

  // A PERGUNTA VEM ANTES DE TUDO, e ela e a lista inteira enquanto durar. Nao e
  // um cartaz por cima de uma lista que da para ler por baixo: o pedido foi
  // "quando entrar a primeira vez, perguntar", e uma pergunta que se pode
  // ignorar rolando a tela nao foi feita.
  //
  // O "NAO" VEM PRIMEIRO. E a resposta padrao, e a primeira linha e a que
  // recebe o foco quando o D-pad desce — quem apertar OK duas vezes sem ler
  // acaba em "nao", que e o unico lado em que errar nao custa nada a ninguem.
  if (consentEstado == REC_APARECER_NAO_PERGUNTADO) {
    social[nSocial].tipo = SPS_CONSENT_NAO; social[nSocial].idx = 0; nSocial++;
    social[nSocial].tipo = SPS_CONSENT_SIM; social[nSocial].idx = 0; nSocial++;
    return;
  }
#if SP_V2
  // A SEGUNDA PERGUNTA, a do NIVEL: quem ve o que eu assisto. Nada sai da TV
  // antes da resposta (recomenda.h, REC_ALCANCE_NAO_PERGUNTADO), entao ela vem
  // antes da lista como a primeira — e o "Ninguem" e a primeira linha, pelo
  // mesmo motivo do "Nao" de la.
  alcEstado = recomenda_alcance();
  if (alcEstado == REC_ALCANCE_NAO_PERGUNTADO || escolhendoAlcance) {
    social[nSocial].tipo = SPS_ALC_0; social[nSocial].idx = 0; nSocial++;
    social[nSocial].tipo = SPS_ALC_1; social[nSocial].idx = 0; nSocial++;
    social[nSocial].tipo = SPS_ALC_2; social[nSocial].idx = 0; nSocial++;
    return;
  }
#endif

  for (i = 0; i < REC_MAX && nRecs < REC_MAX; i++)
    if (recomenda_item(i, &recs[nRecs])) nRecs++;
    else break;
  // POR PESSOA (salvosorg.h): as recomendacoes de cada um juntas, as pessoas
  // em ordem de nome e, dentro de cada uma, a ordem de chegada de sempre.
  // Insercao estavel: sao no maximo REC_MAX (60) linhas.
  if (sorg_social() == SORG_SOCIAL_PESSOA) {
    int j;
    for (i = 1; i < nRecs; i++) {
      RecItem t = recs[i];
      for (j = i; j > 0; j--) {
        int d = strcasecmp(recs[j - 1].deNome, t.deNome);
        if (!d) d = strcmp(recs[j - 1].de, t.de);
        if (d <= 0) break;
        recs[j] = recs[j - 1];
      }
      recs[j] = t;
    }
  }
  for (i = 0; i < REC_SUGESTOES_MAX && nSugs < REC_SUGESTOES_MAX; i++)
    if (recomenda_sugestao(i, &sugs[nSugs])) nSugs++;
    else break;

  for (i = 0; i < nRecs && nSocial < SP_SOCIAL_MAX; i++) {
    social[nSocial].tipo = SPS_REC; social[nSocial].idx = (short)i; nSocial++;
  }
  for (i = 0; i < nSugs && nSocial < SP_SOCIAL_MAX; i++) {
    social[nSocial].tipo = SPS_SUG; social[nSocial].idx = (short)i; nSocial++;
  }
  // OS AMIGOS JA ADICIONADOS, com foto, antes de "Adicionar um amigo": a aba
  // dizia o codigo e oferecia adicionar, mas nunca mostrava QUEM ja estava
  // na lista.
  nCtts = recomenda_contatos(ctts, REC_CONTATOS_MAX);
  svRevSocial = socialvis_revisao();
  for (i = 0; i < nCtts; i++) {
    cttSv[i] = (short)socialvis_amigo_indice(ctts[i].id);
    cttCadeia[i] = (unsigned char)socialvis_ultima_enviada(ctts[i].id, NULL);
  }
  for (i = 0; i < nCtts && nSocial < SP_SOCIAL_MAX; i++) {
    social[nSocial].tipo = SPS_AMIGO; social[nSocial].idx = (short)i; nSocial++;
  }
  // ENCONTRAR PESSOAS logo acima de "Adicionar um amigo": as duas sao portas
  // para gente nova, e a segunda so serve a quem ja tem o codigo na mao.
  if (nSocial < SP_SOCIAL_MAX) {
    social[nSocial].tipo = SPS_ENCONTRAR; social[nSocial].idx = 0; nSocial++;
  }
  if (nSocial < SP_SOCIAL_MAX) {
    social[nSocial].tipo = SPS_ADICIONAR; social[nSocial].idx = 0; nSocial++;
  }
  // O INTERRUPTOR FECHA A ABA, e nao mora em Ajustes. Ele responde uma pergunta
  // que so faz sentido olhando para esta lista ("quem me ve?"), e quem quiser
  // mudar de ideia vai procura-lo onde a pergunta foi feita. A alternativa em
  // Ajustes esta descrita no relatorio; as duas podem coexistir.
#if SP_V2
  // COMO EU APARECO e QUEM VE O QUE EU ASSISTO, juntos do interruptor de
  // aparecer: sao as tres respostas sobre "eu para os outros".
  if (nSocial + 2 < SP_SOCIAL_MAX) {
    social[nSocial].tipo = SPS_NOME; social[nSocial].idx = 0; nSocial++;
    social[nSocial].tipo = SPS_ALCANCE; social[nSocial].idx = 0; nSocial++;
  }
#endif
  if (nSocial < SP_SOCIAL_MAX) {
    social[nSocial].tipo = SPS_APARECER; social[nSocial].idx = 0; nSocial++;
  }
}

static void reconstruirAtividade(void) {
  int i;
  atvRev = socialvis_revisao();
  nAtv = socialvis_n_eventos();
  if (nAtv > SV_EVENTOS_MAX) nAtv = SV_EVENTOS_MAX;
  for (i = 0; i < nAtv; i++)
    atvDia[i] = (unsigned char)socialvis_dia(socialvis_evento(i), atvRot[i], sizeof atvRot[i]);
}
// O rotulo do dia antes da linha `i`, quando o dia muda.
static float atvAntes(int i) {
  if (i < 0 || i >= nAtv) return 0.0f;
  if (i == 0) return SP_SECAO_H;
  return (atvDia[i] != atvDia[i - 1] || strcmp(atvRot[i], atvRot[i - 1])) ? SP_SECAO_H : 0.0f;
}

static void trocarAba(int nova) {
  if (!temAbas() || nova == aba) return;
  if (aba == SP_ABA_AVISOS) avisos_marcar_lidos();
  aba = nova;
  foco = SP_FOCO_ABAS;
  scrollY = 0.0f; velY = 0.0f;
  memset(animFoco, 0, sizeof animFoco);
  if (aba == SP_ABA_ATIVIDADE) { socialvis_atualizar(); reconstruirAtividade(); }
  if (aba == SP_ABA_SOCIAL) {
    // CONSULTA IMEDIATA ao entrar, para nao mostrar lista velha; e o selo some
    // porque a pessoa esta olhando justamente para ela.
    //
    // A ORDEM IMPORTA: a copia acontece ANTES de marcar como vistas, entao o
    // SELO da aba zera e os PONTOS das linhas ficam. Sao coisas diferentes —
    // o selo responde "ha algo novo?" e o ponto responde "qual delas e nova?",
    // e apagar os dois no mesmo instante deixaria a pessoa olhando uma lista
    // sem saber por que foi avisada. Na proxima abertura do painel a copia ja
    // le visto=1 e os pontos somem sozinhos.
    recomenda_pedir_agora();
    reconstruirSocial();
    // COM A PERGUNTA NA TELA NADA E MARCADO COMO LIDO. A lista esta atras dela:
    // apagar o selo agora diria "voce ja viu" sobre uma lista que ninguem viu,
    // e o aviso nao voltaria.
    if (!consentindo()) recomenda_marcar_vistas();
  }
}

// --- BARRA DE OPCOES, ESCOLHAS E CATEGORIAS -------------------------------
//
// O CONTROLE REMOTO DECIDE O DESENHO. Quatro pilulas numa fila so, cada uma
// dizendo o que vale agora ("Ordenar / Recentes"): o D-pad chega nelas subindo
// da lista, e o OK abre uma escolha curta por cima do painel — a lista de
// opcoes com a atual marcada. Nada de menu dentro de menu para ordenar: duas
// teclas e o efeito ja esta na lista, embaixo.
//
// A ESCOLHA ("pop") E UMA SO, para tudo: ordenar, agrupar, estilo, a lista de
// categorias, o que fazer com uma, a confirmacao de excluir e o "Mover para"
// que vem do menu do cartaz. Uma maquina so para sete usos, pela mesma razao
// do teclado.h: duas que fazem a mesma coisa divergem na primeira correcao.
enum { SPB_ORDEM = 0, SPB_GRUPO, SPB_ESTILO, SPB_CATEG, SPB_N };
static int   barraFoco;
static float animBarra[SPB_N];

static const char *ORDEM_CURTO[SORG_ORDEM_N] = {
  "Mais antigos", "Recentes", "Nome", "Ano", "Nota", "Restante" };
static const char *ORDEM_LONGO[SORG_ORDEM_N] = {
  "Salvos mais antigos primeiro", "Salvos mais recentes primeiro", "Nome (A–Z)",
  "Ano (mais novo primeiro)", "Nota do IMDb", "Menos tempo restante" };
static const char *GRUPO_CURTO[SORG_GRUPO_N] = {
  "Progresso", "Tipo", "Categoria", "Nenhum" };
static const char *GRUPO_LONGO[SORG_GRUPO_N] = {
  "Continuar e não começados", "Tipo: filmes, séries, coleções, canais",
  "Minhas categorias", "Sem agrupar" };
static const char *ESTILO_CURTO[SORG_ESTILO_N] = { "Lista", "Grade", "Paisagem" };
static const char *ESTILO_LONGO[SORG_ESTILO_N] = {
  "Lista com capa", "Grade de pôsteres", "Cartões paisagem" };
static const char *ESTILO_ICONE[SORG_ESTILO_N] = {
  "aj_rows-3", "aj_layout-dashboard", "aj_images" };
static const char *SOCIAL_CURTO[SORG_SOCIAL_N] = { "Recentes", "Por pessoa" };
static const char *SOCIAL_LONGO[SORG_SOCIAL_N] = {
  "Mais recentes primeiro", "Agrupar por pessoa" };

static int temBarra(void) {
  if (aba == SP_ABA_SALVOS) return nLinhas > 0;
  if (aba == SP_ABA_SOCIAL) return !consentindo() && !perguntandoAlcance() && nRecs >= 2;
  return 0;
}
static int nChips(void) { return aba == SP_ABA_SALVOS ? SPB_N : 1; }

enum { POP_NADA = 0, POP_ORDEM, POP_GRUPO, POP_ESTILO, POP_SOCIAL, POP_CATS,
       POP_CAT_ACOES, POP_EXCLUIR, POP_MOVER };
#define POP_MAX (SORG_CAT_MAX + 4)
#define POP_LINHA_H   64.0f
#define POP_LINHA_VAO  8.0f
#define POP_VISIVEIS   8
typedef struct {
  char rot[SORG_NOME_MAX + 40];
  const char *icone;
  int  valor, marcado;
} PopLinha;
static PopLinha popL[POP_MAX];
static int   pop, popN, popFoco, popCat;
static float popEntrada, popRol, popAnim[POP_MAX];
static char  popTitulo[200], popSub[200], popItem[24];

// O TECLADO DO APP (teclado.h) para nome de categoria: letras, numeros,
// espaco e hifen. A primeira letra sai maiuscula (sorg_nome_limpo).
enum { TK_NADA = 0, TK_CRIAR, TK_MOVER, TK_RENOMEAR, TK_NOME_SOCIAL };
static int tecladoPara;
static const char *ALFA_NOME = "abcdefghijklmnopqrstuvwxyz0123456789 -";
#define SP_NOME_LETRAS 24

static void popLinha(const char *rot, const char *icone, int valor, int marcado) {
  PopLinha *l;
  if (popN >= POP_MAX) return;
  l = &popL[popN++];
  snprintf(l->rot, sizeof l->rot, "%s", rot);
  l->icone = icone;
  l->valor = valor;
  l->marcado = marcado;
}

// Quantos titulos da lista montada estao na categoria `id`.
static int contaNaCategoria(int id) {
  int i, n = 0;
  for (i = 0; i < nLinhas; i++) if (linhas[i].cat == id) n++;
  return n;
}

static void popAbrir(int tipo) {
  int i, k;
  char b[SORG_NOME_MAX + 40];
  pop = tipo;
  popN = 0;
  popTitulo[0] = popSub[0] = 0;
  switch (tipo) {
    case POP_ORDEM:
      snprintf(popTitulo, sizeof popTitulo, "%s", "Ordenar por");
      for (i = 0; i < SORG_ORDEM_N; i++) popLinha(ORDEM_LONGO[i], NULL, i, i == sorg_ordem());
      break;
    case POP_GRUPO:
      snprintf(popTitulo, sizeof popTitulo, "%s", "Agrupar por");
      for (i = 0; i < SORG_GRUPO_N; i++) popLinha(GRUPO_LONGO[i], NULL, i, i == sorg_grupo());
      break;
    case POP_ESTILO:
      snprintf(popTitulo, sizeof popTitulo, "%s", "Estilo de exibição");
      for (i = 0; i < SORG_ESTILO_N; i++)
        popLinha(ESTILO_LONGO[i], ESTILO_ICONE[i], i, i == sorg_estilo());
      break;
    case POP_SOCIAL:
      snprintf(popTitulo, sizeof popTitulo, "%s", "Organizar recomendações");
      for (i = 0; i < SORG_SOCIAL_N; i++) popLinha(SOCIAL_LONGO[i], NULL, i, i == sorg_social());
      break;
    case POP_CATS:
      snprintf(popTitulo, sizeof popTitulo, "%s", "Categorias");
      snprintf(popSub, sizeof popSub, "%s", "Segure OK num título para mover para uma categoria.");
      popLinha("Nova categoria", "mais", -1, 0);
      for (i = 0; i < sorg_n_categorias(); i++) {
        int id = sorg_categoria_id(i), n = contaNaCategoria(id);
        const char *nome = sorg_categoria_nome_id(id);
        snprintf(b, sizeof b, "%s  ·  %d", nome ? nome : "", n);
        popLinha(b, "aj_folders", id, 0);
      }
      break;
    case POP_CAT_ACOES:
      { const char *nome = sorg_categoria_nome_id(popCat);
        snprintf(popTitulo, sizeof popTitulo, "%s", nome ? nome : ""); }
      popLinha("Renomear", "aj_keyboard", 1, 0);
      popLinha("Excluir categoria", NULL, 2, 0);
      break;
    case POP_EXCLUIR:
      { const char *nome = sorg_categoria_nome_id(popCat);
        snprintf(popTitulo, sizeof popTitulo, i18n("Excluir \xe2\x80\x9c%s\xe2\x80\x9d?"), nome ? nome : ""); }
      snprintf(popSub, sizeof popSub, "%s", "Os títulos continuam nos Salvos, só saem da categoria.");
      popLinha("Cancelar", NULL, 0, 0);
      popLinha("Excluir", NULL, 1, 0);
      break;
    case POP_MOVER: {
      int atual = sorg_categoria_de(popItem);
      const char *tit = "";
      for (i = 0; i < nLinhas; i++)
        if (salvos_mesmo_titulo(linhas[i].id, popItem)) { tit = linhas[i].titulo; break; }
      snprintf(popTitulo, sizeof popTitulo, "%s", "Mover para categoria");
      snprintf(popSub, sizeof popSub, "%s", tit);
      for (i = 0; i < sorg_n_categorias(); i++) {
        int id = sorg_categoria_id(i);
        const char *nome = sorg_categoria_nome_id(id);
        popLinha(nome ? nome : "", "aj_folders", id, id == atual);
      }
      popLinha("Sem categoria", NULL, 0, atual == 0);
      popLinha("Nova categoria", "mais", -1, 0);
      break; }
    default: pop = POP_NADA; return;
  }
  // O foco nasce na opcao que vale agora; sem nenhuma marcada, na primeira.
  popFoco = 0;
  for (k = 0; k < popN; k++) if (popL[k].marcado) { popFoco = k; break; }
  // "Excluir" nunca nasce com o foco: OK duas vezes sem ler cancela.
  if (tipo == POP_EXCLUIR) popFoco = 0;
  popRol = 0.0f;
  popEntrada = 0.0f;
  memset(popAnim, 0, sizeof popAnim);
}

static void tecladoNome(int para, const char *inicial) {
  tecladoPara = para;
  pop = POP_NADA;
  teclado_abrir_com(para == TK_RENOMEAR ? "Renomear categoria" : "Nova categoria",
                    "Ex.: Fim de semana, Kids. Até 24 letras.",
                    SP_NOME_LETRAS, ALFA_NOME, inicial);
}

// OK numa opcao da escolha aberta.
static void popOk(void) {
  int v;
  if (popFoco < 0 || popFoco >= popN) return;
  v = popL[popFoco].valor;
  switch (pop) {
    case POP_ORDEM:  sorg_definir_ordem(v);  pop = POP_NADA; break;
    case POP_GRUPO:  sorg_definir_grupo(v);  pop = POP_NADA; break;
    case POP_ESTILO:
      sorg_definir_estilo(v);
      pop = POP_NADA;
      // A lista trocou de forma: a rolagem recomeca de onde o foco esta.
      memset(animFoco, 0, sizeof animFoco);
      break;
    case POP_SOCIAL:
      sorg_definir_social(v);
      pop = POP_NADA;
      reconstruirSocial();
      break;
    case POP_CATS:
      if (v < 0) tecladoNome(TK_CRIAR, NULL);
      else { popCat = v; popAbrir(POP_CAT_ACOES); }
      break;
    case POP_CAT_ACOES:
      if (v == 1) tecladoNome(TK_RENOMEAR, sorg_categoria_nome_id(popCat));
      else popAbrir(POP_EXCLUIR);
      break;
    case POP_EXCLUIR:
      if (v == 1) { sorg_excluir_categoria(popCat); pop = POP_NADA; }
      else popAbrir(POP_CATS);
      break;
    case POP_MOVER:
      if (v < 0) { tecladoNome(TK_MOVER, NULL); break; }
      sorg_mover(popItem, v);
      snprintf(seguirId, sizeof seguirId, "%s", popItem);
      pop = POP_NADA;
      break;
    default: pop = POP_NADA; break;
  }
}

static void popVoltar(void) {
  if (pop == POP_CAT_ACOES || pop == POP_EXCLUIR) popAbrir(POP_CATS);
  else pop = POP_NADA;
}

static void popEvento(const SDL_Event *e) {
  SDL_Keycode k;
  if (e->type != SDL_KEYDOWN) return;
  k = e->key.keysym.sym;
  if (k == SDLK_AC_BACK || k == SDLK_ESCAPE || k == SDLK_BACKSPACE ||
      k == SDLK_DELETE || k == SDLK_LEFT || e->key.keysym.scancode == NV_SCANCODE_BACK) {
    popVoltar(); return;
  }
  if (k == SDLK_DOWN) { if (popFoco + 1 < popN) popFoco++; return; }
  if (k == SDLK_UP)   { if (popFoco > 0) popFoco--; return; }
  if (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE) {
    if (!e->key.repeat) popOk();
    return;
  }
}

// OK numa pilula da barra.
static void chipOk(void) {
  if (aba == SP_ABA_SOCIAL) { popAbrir(POP_SOCIAL); return; }
  switch (barraFoco) {
    case SPB_ORDEM:  popAbrir(POP_ORDEM);  break;
    case SPB_GRUPO:  popAbrir(POP_GRUPO);  break;
    case SPB_ESTILO: popAbrir(POP_ESTILO); break;
    default:
      // Sem nenhuma categoria a lista de categorias so teria "Nova": vai
      // direto ao teclado.
      if (sorg_n_categorias() < 1) tecladoNome(TK_CRIAR, NULL);
      else popAbrir(POP_CATS);
      break;
  }
}

// O que o teclado devolveu. Chamado por quadro enquanto ha pedido pendente.
static void tecladoResultado(void) {
  int r, id;
  const char *t;
  if (!tecladoPara || teclado_aberto()) return;
  r = teclado_resultado();
  if (r == TECLADO_NADA) return;
  t = teclado_texto();
#if SP_V2
  // O NOME PARA OS AMIGOS: vazio volta ao nome do perfil (recomenda.h).
  if (tecladoPara == TK_NOME_SOCIAL) {
    if (r == TECLADO_PRONTO && t) recomenda_definir_nome(t);
    tecladoPara = TK_NADA;
    return;
  }
#endif
  if (r == TECLADO_PRONTO && t && t[0]) {
    if (tecladoPara == TK_RENOMEAR) sorg_renomear_categoria(popCat, t);
    else if ((id = sorg_criar_categoria(t)) != 0) {
      if (tecladoPara == TK_MOVER) {
        sorg_mover(popItem, id);
        snprintf(seguirId, sizeof seguirId, "%s", popItem);
      }
      // QUEM CRIA A CATEGORIA PELA BARRA QUER VE-LA: a lista passa a ser
      // agrupada por categoria, e "Kids" aparece vazia, com a dica de como
      // por algo nela. Mover pelo menu do cartaz nao troca o agrupamento —
      // a linha ganha o selo da categoria, que basta como resposta.
      else sorg_definir_grupo(SORG_GRUPO_CATEGORIA);
    }
  }
  tecladoPara = TK_NADA;
}

// A celula da fila de cima ou de baixo mais perto, na horizontal, da focada.
// Na lista e a vizinha; na grade e a da mesma coluna (ou a ultima da fila,
// quando a fila de baixo e mais curta).
static int celulaVizinha(int de, int dir) {
  int i, melhor = -1, alvo;
  float cx, md = 1e9f;
  if (de < 0 || de >= nLinhas) return -1;
  alvo = linhas[de].fila + dir;
  cx = linhas[de].lx + linhas[de].lw * 0.5f;
  for (i = de + dir; i >= 0 && i < nLinhas; i += dir) {
    float d;
    if (dir > 0 ? linhas[i].fila > alvo : linhas[i].fila < alvo) break;
    if (linhas[i].fila != alvo) continue;
    d = linhas[i].lx + linhas[i].lw * 0.5f - cx;
    if (d < 0) d = -d;
    if (d < md) { md = d; melhor = i; }
  }
  return melhor;
}

// Pedido de "Mover para categoria" vindo do menu do cartaz.
static void popAbrirMover(const char *imdb) {
  snprintf(popItem, sizeof popItem, "%s", imdb);
  popAbrir(POP_MOVER);
}

// Nascer da ilha (salvospainel.h). `origem` e fixa durante a abertura; o
// destino da volta e renovado por quadro, porque a pilula pode ter mudado de
// largura (o cartao alternou) enquanto o painel estava aberto.
static int deIlha, destinoOk;
static GfxRect origem, destino;

void spainel_abrir_de(float x, float y, float w, float h) {
  if (aberto) return;
  spainel_abrir();
  deIlha = 1;
  origem = (GfxRect){ x, y, w, h };
}

void spainel_recolher_para(int ok, float x, float y, float w, float h) {
  destinoOk = ok;
  if (ok) destino = (GfxRect){ x, y, w, h };
}

int spainel_da_ilha(void) { return deIlha && spainel_visivel(); }

void spainel_abrir(void) {
  if (aberto) return;
  deIlha = 0;
  aberto = 1;
  foco = 0;
  okDesde = 0;
  menuId[0] = menuProximo[0] = 0;
  seguirId[0] = 0;
  pop = POP_NADA;
  barraFoco = 0;
  memset(animBarra, 0, sizeof animBarra);
  aba = SP_ABA_SALVOS;
  scrollY = 0.0f; velY = 0.0f;
  memset(animFoco, 0, sizeof animFoco);
  // O INTERRUPTOR ABRE NO ESTADO, e nao deslizando ate ele. A resposta pode ter
  // sido reconciliada com o servidor (respondida em OUTRA TV) com o painel
  // fechado; sem isto a pessoa abriria o painel e veria a bola andar sozinha,
  // que le como "alguem acabou de mexer aqui".
  animSw = -1.0f;
  reconstruir();
  reconstruirSocial();
}

void spainel_fechar(void) {
  if (aberto && aba == SP_ABA_AVISOS) avisos_marcar_lidos();
  pop = POP_NADA;
  aberto = 0;
}

// Altura ate o TOPO da linha `i`, contando o cabecalho de cada secao. Nao e
// `i * SP_PASSO`: o rotulo "Não começados" empurra tudo que vem depois dele, e
// sem contar esse empurrao a rolagem para a linha focada erra por 54px — o
// suficiente para o card focado ficar meio escondido atras do cabecalho.
static float topoDe(int i) {
  float y;
  // A ABA SOCIAL SOMA LINHA A LINHA, e nao multiplica por um passo fixo: as
  // linhas dela tem quatro alturas diferentes. Era `i * SP_PASSO` enquanto
  // todas eram recomendacoes; com uma sugestao de 112px no meio, a multiplicacao
  // erraria a partir dali e a rolagem pararia o foco meio fora da janela.
  if (aba == SP_ABA_SOCIAL) {
    int k;
    y = socialTopo();
    for (k = 0; k < i && k < nSocial; k++)
      y += socialAntes(k) + socialAlt(k) + SPS_GAP;
    return y + socialAntes(i);
  }
  if (aba == SP_ABA_AVISOS) return avisos_lista_y(i, foco);
  if (aba == SP_ABA_ATIVIDADE) {
    int k;
    y = 0.0f;
    for (k = 0; k < i && k < nAtv; k++) y += atvAntes(k) + SPA_H + SPS_GAP;
    return y + atvAntes(i);
  }
  // Rotulo da primeira secao, sempre; mais o de "Não começados" para quem vem
  // depois dele. Com nCont == 0 nao existe segunda secao — a unica que aparece
  // e "Sua lista", e o segundo termo tem de ser zero para todo mundo.
  // Salvos: o lugar que montarLayout ja calculou (secoes, filas da grade).
  return (i >= 0 && i < nLinhas) ? linhas[i].ly : 0.0f;
}

// Uma linha de Salvos em foco, ou seja, algo que o OK longo pode segurar.
static int linhaSeguravel(void) {
  return aba == SP_ABA_SALVOS && foco >= 0 && foco < nLinhas;
}

// Abre o menu do cartaz sobre a linha focada. A linha vira um CatItem com o
// que o painel sabe dela; o menu troca pela copia do catalogo quando ela
// existe (ver o modo painel em ctxmenu.c).
static void abrirMenu(void) {
  CatItem c;
  const SPLinha *l;
  if (!linhaSeguravel()) return;
  l = &linhas[foco];
  memset(&c, 0, sizeof c);
  snprintf(c.imdb, sizeof c.imdb, "%s", l->id);
  snprintf(c.tipo, sizeof c.tipo, "%s", l->serie ? "series" : "movie");
  snprintf(c.titulo, sizeof c.titulo, "%s", l->titulo);
  snprintf(c.poster, sizeof c.poster, "%s", l->poster);
  snprintf(c.meta, sizeof c.meta, "%s", l->meta);
  c.nota = l->nota;
  c.progresso = l->progresso;
  c.temporada = l->temporada;
  c.episodio = l->episodio;
  c.restanteMin = l->restanteMin;
  snprintf(menuId, sizeof menuId, "%s", l->id);
  // O seguinte, ou o anterior quando a linha e a ultima: e para onde o foco
  // vai se o titulo sair da lista.
  menuProximo[0] = 0;
  if (foco + 1 < nLinhas) snprintf(menuProximo, sizeof menuProximo, "%s", linhas[foco + 1].id);
  else if (foco > 0) snprintf(menuProximo, sizeof menuProximo, "%s", linhas[foco - 1].id);
  ctx_abrir_salvo(&c);
}

// O toque curto de sempre: entrega o IMDb e fecha, app.c abre o titulo.
static void abrirLinha(void) {
  if (foco >= 0 && foco < nLinhas) {
    snprintf(pedido, sizeof pedido, "%s", linhas[foco].id);
    temPedido = 1;
    aberto = 0;
  }
}

static int teclaOk(SDL_Keycode k) {
  return k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE;
}

void spainel_evento(const SDL_Event *e) {
  SDL_Keycode k;
  if (!aberto) return;
  // O teclado e a escolha aberta ficam POR CIMA da lista: a tecla e deles.
  if (teclado_aberto()) { teclado_evento(e); return; }
  if (pop) { popEvento(e); return; }
  // A SOLTURA DO OK numa linha de Salvos: curto abre o titulo, longo abre o
  // menu (se spainel_atualizar ainda nao o abriu no limiar — um quadro lento
  // ou um teste sem quadro). Soltura sem o KEYDOWN daqui nao e clique: e o OK
  // que fechou o menu do cartaz, ou o que abriu o painel por outra porta.
  if (e->type == SDL_KEYUP) {
    if (okDesde && teclaOk(e->key.keysym.sym)) {
      Uint32 dur = SDL_GetTicks() - okDesde;
      okDesde = 0;
      if (dur >= NV_HOLD_MS) abrirMenu();
      else abrirLinha();
    }
    return;
  }
  if (e->type != SDL_KEYDOWN) return;
  k = e->key.keysym.sym;
  // Qualquer outra tecla no meio desfaz o gesto, como na home (observarHold).
  if (!teclaOk(k)) okDesde = 0;
  // Mesmo conjunto de "voltar" que o menu lateral aceita, mais a ESQUERDA: o
  // painel encosta na borda direita da tela, entao sair por ele e ir para a
  // esquerda. E o gesto que perfil.c ja tinha nesta mesma posicao.
  if (k == SDLK_AC_BACK || k == SDLK_ESCAPE || k == SDLK_BACKSPACE ||
      k == SDLK_DELETE || e->key.keysym.scancode == NV_SCANCODE_BACK) {
    spainel_fechar(); return;
  }
  // ESQUERDA NA LINHA DE ABAS NAO FECHA SE HA PARA ONDE IR. Fora dela, e fora
  // da primeira aba, ela continua sendo "sair pela borda" — o gesto que
  // perfil.c ja tinha nesta posicao.
  if (k == SDLK_LEFT) {
    if (temAbas() && foco == SP_FOCO_ABAS && aba != SP_ABA_SALVOS) {
      trocarAba(proximaAba(aba, -1)); return;
    }
    // Na barra e na grade a esquerda anda; so na primeira coluna ela sai.
    if (foco == SP_FOCO_BARRA && barraFoco > 0) { barraFoco--; return; }
    if (aba == SP_ABA_SALVOS && foco > 0 && foco < nLinhas &&
        linhas[foco - 1].fila == linhas[foco].fila) { foco--; return; }
    spainel_fechar(); return;
  }
  if (k == SDLK_RIGHT) {
    if (temAbas() && foco == SP_FOCO_ABAS) trocarAba(proximaAba(aba, 1));
    else if (foco == SP_FOCO_BARRA) { if (barraFoco + 1 < nChips()) barraFoco++; }
    else if (aba == SP_ABA_SALVOS && foco >= 0 && foco + 1 < nLinhas &&
             linhas[foco + 1].fila == linhas[foco].fila) foco++;
    return;
  }
  if (k == SDLK_DOWN) {
    if (foco == SP_FOCO_ABAS) {
      if (temBarra()) { foco = SP_FOCO_BARRA; if (barraFoco >= nChips()) barraFoco = 0; }
      else if (nVisiveis() > 0) foco = 0;
      return;
    }
    if (foco == SP_FOCO_BARRA) { if (nVisiveis() > 0) foco = 0; return; }
    if (aba == SP_ABA_SALVOS) {
      int v = celulaVizinha(foco, 1);
      if (v >= 0) foco = v;
      return;
    }
    if (foco + 1 < nVisiveis()) foco++;
    return;
  }
  if (k == SDLK_UP) {
    // DE CIMA DA LISTA SOBE PARA A BARRA (se ha) E DAI PARA AS ABAS, e nao
    // para lugar nenhum. Sem isto a unica forma de trocar de aba seria fechar
    // e reabrir o painel.
    if (foco == SP_FOCO_BARRA) { foco = SP_FOCO_ABAS; return; }
    if (foco == SP_FOCO_ABAS) return;
    if (aba == SP_ABA_SALVOS && foco < nLinhas) {
      int v = celulaVizinha(foco, -1);
      if (v >= 0) { foco = v; return; }
      foco = temBarra() ? SP_FOCO_BARRA : SP_FOCO_ABAS;
      return;
    }
    if (foco == 0 && temAbas()) { foco = temBarra() ? SP_FOCO_BARRA : SP_FOCO_ABAS; return; }
    if (foco > 0) foco--;
    return;
  }
  if (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE) {
    if (foco == SP_FOCO_BARRA) { if (!e->key.repeat) chipOk(); return; }
    if (foco == SP_FOCO_ABAS) {
      // OK na linha de abas alterna, para quem nao descobriu a seta.
      { int p = proximaAba(aba, 1); trocarAba(p == aba ? SP_ABA_SALVOS : p); }
      return;
    }
    if (aba == SP_ABA_AVISOS) {
      if (avisos_lista_ok(foco)) spainel_fechar();
      return;
    }
    if (aba == SP_ABA_ATIVIDADE) {
      const SvEvento *ev = socialvis_evento(foco);
      if (ev && ev->imdb[0]) {
        snprintf(pedido, sizeof pedido, "%s", ev->imdb);
        temPedido = 1;
        aberto = 0;
      }
      return;
    }
    if (aba == SP_ABA_SOCIAL) {
      if (foco < 0 || foco >= nSocial) return;
      switch (social[foco].tipo) {
        case SPS_CONSENT_NAO:
        case SPS_CONSENT_SIM:
          recomenda_responder_aparecer(social[foco].tipo == SPS_CONSENT_SIM);
          reconstruirSocial();
          // A LISTA COMECA DO TOPO depois da resposta. Manter o foco na linha 1
          // deixaria o dedo em cima de uma sugestao que a pessoa nem viu
          // aparecer, e o proximo OK a adicionaria como contato.
          foco = 0;
          scrollY = 0.0f; velY = 0.0f;
          memset(animFoco, 0, sizeof animFoco);
          recomenda_marcar_vistas();
          return;
        case SPS_SUG:
          // UMA ACAO, como o pedido pediu: o OK vincula. O servidor recalcula
          // as sugestoes antes de aceitar, entao um id que ja nao esta na lista
          // (a pessoa revogou entre a tela e o OK) volta recusado.
          if (social[foco].idx >= 0 && social[foco].idx < nSugs)
            recomenda_adicionar_sugerido(sugs[social[foco].idx].id);
          reconstruirSocial();
          if (foco >= nSocial) foco = nSocial > 0 ? nSocial - 1 : 0;
          return;
        case SPS_ADICIONAR:
          // A TELA DE AMIGOS. O painel FICA ABERTO atras: a modal e uma camada
          // por cima dele e Voltar devolve o foco aqui, em vez de jogar a
          // pessoa de volta na home.
          recenviar_abrir_amigos();
          return;
        case SPS_ENCONTRAR:
          // Como a tela de amigos: o painel FICA aberto atras da modal.
          pessoas_abrir();
          return;
        case SPS_AMIGO:
          // O PERFIL DO AMIGO (amigoperfil.h). app.c abre e o painel fecha.
          if (social[foco].idx >= 0 && social[foco].idx < nCtts) {
            snprintf(pedidoPerfil, sizeof pedidoPerfil, "%s", ctts[social[foco].idx].id);
            temPedidoPerfil = 1;
            aberto = 0;
          }
          return;
#if SP_V2
        case SPS_ALC_0:
        case SPS_ALC_1:
        case SPS_ALC_2:
          recomenda_responder_alcance(social[foco].tipo - SPS_ALC_0);
          escolhendoAlcance = 0;
          reconstruirSocial();
          foco = 0;
          scrollY = 0.0f; velY = 0.0f;
          memset(animFoco, 0, sizeof animFoco);
          return;
        case SPS_ALCANCE: {
          // REABRE A PERGUNTA, com o foco na resposta de agora: trocar de nivel
          // e escolher de novo, lendo as tres, e nao um OK que gira valores.
          int n = recomenda_alcance();
          escolhendoAlcance = 1;
          reconstruirSocial();
          foco = (n >= 0 && n <= 2) ? n : 0;
          scrollY = 0.0f; velY = 0.0f;
          memset(animFoco, 0, sizeof animFoco);
          return; }
        case SPS_NOME:
          tecladoPara = TK_NOME_SOCIAL;
          teclado_abrir_com("Como você aparece",
                            "Seu nome para os amigos. Vazio usa o nome do perfil.",
                            32, "abcdefghijklmnopqrstuvwxyz0123456789 -'", recomenda_minha_exibicao());
          return;
#else
        case SPS_ALC_0: case SPS_ALC_1: case SPS_ALC_2: case SPS_ALCANCE: case SPS_NOME:
          return;
#endif
        case SPS_APARECER:
          // MUDAR DE IDEIA CUSTA UM OK, nos dois sentidos. Sem confirmacao de
          // proposito: desligar e a direcao segura, e pedir "tem certeza?" para
          // sair de uma lista e o padrao que faz as pessoas desistirem de sair.
          recomenda_responder_aparecer(recomenda_aparecer() != REC_APARECER_SIM);
          reconstruirSocial();
          return;
        default:
          // A ACAO QUE IMPORTA E ABRIR O TITULO, e o contrato para isso ja
          // existe: o painel entrega o IMDb e app.c resolve. Ele nao conhece
          // detail.c nem a descoberta, exatamente como antes.
          if (social[foco].idx >= 0 && social[foco].idx < nRecs) {
            snprintf(pedido, sizeof pedido, "%s", recs[social[foco].idx].imdb);
            temPedido = 1;
            aberto = 0;
          }
          return;
      }
    }
    // A decisao fica para a soltura (ou para o limiar, em spainel_atualizar).
    // A repeticao automatica do controle nao rearma: o relogio e do primeiro
    // KEYDOWN.
    if (linhaSeguravel() && !e->key.repeat && !okDesde) {
      okDesde = SDL_GetTicks();
      if (!okDesde) okDesde = 1;
    }
    return;
  }
}

void spainel_atualizar(float dt, Uint32 agora) {
  int i;
  float alvo, topo, base;
  // A barra de "Segure OK" do menu do cartaz, centrada no painel enquanto ele e
  // dono do D-pad; fora dele, no centro da tela como sempre.
  ctx_centro_dica(aberto && aba == SP_ABA_SALVOS ? SP_X + SP_W * 0.5f : -1.0f);
  if (!aberto) okDesde = 0;
  if (!aberto && entrada < 0.002f) {
    if (entrada != 0.0f) entrada = 0.0f;
    deIlha = 0;
    return;
  }
  // O catalogo pode ter sido republicado com o painel aberto (a descoberta faz
  // isso varias vezes por ciclo), e a conta pode ter marcado um titulo. Sem
  // reconstruir, a lista continuaria a do instante da abertura. Ver listaVelha:
  // a pergunta por quadro sao duas revisoes, e nao um retrato do catalogo.
  if (aberto && listaVelha()) reconstruir();
  // O teclado de nome de categoria e o que ele devolveu.
  if (tecladoPara) { teclado_atualizar(dt, agora); tecladoResultado(); }
  // "Mover para categoria" no menu do cartaz: a escolha abre aqui, por cima
  // do painel, com as categorias da pessoa.
  { const char *id = ctx_pediu_categoria();
    if (id && aberto) popAbrirMover(id); }
  // A barra some quando deixa de fazer sentido (a lista esvaziou, a Social
  // ficou com uma recomendacao so): o foco nao pode ficar num lugar que nao
  // e mais desenhado.
  if (foco == SP_FOCO_BARRA && !temBarra()) foco = SP_FOCO_ABAS;
  if (barraFoco >= nChips()) barraFoco = nChips() - 1;
  // O LIMIAR DO OK LONGO, com o dedo ainda no botao (ver okDesde).
  if (aberto && okDesde && SDL_GetTicks() - okDesde >= NV_HOLD_MS) {
    okDesde = 0;
    abrirMenu();
  }
  // "Mais informações" no menu do painel: o mesmo contrato do toque curto.
  { const char *id = ctx_pediu_detalhes_imdb();
    if (id && aberto) {
      snprintf(pedido, sizeof pedido, "%s", id);
      temPedido = 1;
      aberto = 0;
    } }
  // O menu saiu e a lista ja remontou o que tinha de remontar: o foco volta a
  // ser por indice, como sempre.
  if (menuId[0] && !ctx_aberto() && !listaVelha()) menuId[0] = menuProximo[0] = 0;
  // A LISTA SOCIAL TAMBEM MUDA COM O PAINEL ABERTO: o fio de recomenda.c sonda
  // a cada 60 s, e uma recomendacao que chega enquanto a aba esta na tela tem
  // de aparecer. A copia e barata (memcpy de ate 60 registros) e so acontece
  // com a aba Social visivel.
  // A LISTA SOCIAL TAMBEM MUDA COM O PAINEL ABERTO, e agora por tres motivos e
  // nao um: chegou recomendacao, chegou (ou saiu) sugestao, ou a resposta sobre
  // aparecer foi reconciliada com o servidor — esta ultima acontece quando a
  // pessoa respondeu SIM em outra TV e o registro deste aparelho adotou a
  // resposta. Sem ela a pergunta continuaria na tela ja respondida.
  if (aberto && (aba == SP_ABA_SOCIAL || aba == SP_ABA_ATIVIDADE)) socialvis_atualizar();
  if (aberto && aba == SP_ABA_ATIVIDADE && atvRev != socialvis_revisao()) {
    reconstruirAtividade();
    if (foco >= nAtv) foco = nAtv > 0 ? nAtv - 1 : SP_FOCO_ABAS;
  }
  if (aberto && aba == SP_ABA_SOCIAL &&
      (nRecs != recomenda_n() || nSugs != recomenda_n_sugestoes() ||
       consentEstado != recomenda_aparecer() || svRevSocial != socialvis_revisao()
#if SP_V2
       || (consentEstado != REC_APARECER_NAO_PERGUNTADO && alcEstado != recomenda_alcance())
#endif
       )) {
    reconstruirSocial();
    if (foco >= nVisiveis()) foco = nVisiveis() > 0 ? nVisiveis() - 1 : 0;
  }

  entrada = anim_rampa(entrada, aberto ? 1.0f : 0.0f, dt,
                       aberto ? SP_ABRIR_MS : SP_FECHAR_MS);
  for (i = 0; i < nVisiveis() && i < SP_MAX; i++) {
    float a = (aberto && i == foco) ? 1.0f : 0.0f;
    animFoco[i] = ajustes_animacoes_reduzidas()
      ? a
      : anim_mola(animFoco[i], a, dt,
                  a > animFoco[i] ? NV_MOLA_FOCO : SP_MOLA_DESFOCO);
  }
  // A BOLA DO INTERRUPTOR, na mesma mola do foco (NV_MOLA_FOCO, 95% em 120 ms):
  // as duas coisas acontecem no mesmo OK e tempos diferentes leriam como bug.
  //
  // COM ANIMACOES REDUZIDAS ELE SALTA, e continua legivel — o estado esta na
  // POSICAO e no preenchimento, nunca no movimento. O deslize so acrescenta a
  // leitura de "isto MUDOU", que e um ganho para quem pode ve-lo e nao uma
  // condicao para entender o controle.
  { float alvoSw = (temAbas() && recomenda_aparecer() == REC_APARECER_SIM)
                   ? 1.0f : 0.0f;
    if (animSw < 0.0f) animSw = alvoSw;   // primeira leitura: assenta sem deslizar
    animSw = ajustes_animacoes_reduzidas()
           ? alvoSw : anim_mola(animSw, alvoSw, dt, NV_MOLA_FOCO); }
  for (i = 0; i < SPB_N; i++) {
    float a = (aberto && foco == SP_FOCO_BARRA && i == barraFoco && !pop) ? 1.0f : 0.0f;
    animBarra[i] = ajustes_animacoes_reduzidas() ? a
      : anim_mola(animBarra[i], a, dt, a > animBarra[i] ? NV_MOLA_FOCO : SP_MOLA_DESFOCO);
  }
  // A ESCOLHA: entra em 140 ms, o foco na mola de sempre, e a lista dela rola
  // o minimo para a opcao focada caber (sete categorias ja passam da janela).
  if (pop) {
    float alvoR = popRol, topoR = (float)popFoco * (POP_LINHA_H + POP_LINHA_VAO);
    float janela = (float)POP_VISIVEIS * (POP_LINHA_H + POP_LINHA_VAO);
    popEntrada = anim_rampa(popEntrada, 1.0f, dt, 140.0f);
    for (i = 0; i < popN && i < POP_MAX; i++) {
      float a = i == popFoco ? 1.0f : 0.0f;
      popAnim[i] = ajustes_animacoes_reduzidas() ? a
        : anim_mola(popAnim[i], a, dt, a > popAnim[i] ? NV_MOLA_FOCO : SP_MOLA_DESFOCO);
    }
    if (topoR + POP_LINHA_H - alvoR > janela) alvoR = topoR + POP_LINHA_H - janela;
    if (topoR < alvoR) alvoR = topoR;
    popRol = ajustes_animacoes_reduzidas() ? alvoR : anim_mola(popRol, alvoR, dt, NV_MOLA_FOCO);
  }
  // Rola o MINIMO para a linha focada caber inteira, como a grade da
  // Biblioteca. Alinhar a focada ao topo joga o cabecalho para fora na primeira
  // descida e a pessoa perde de vista em que painel esta.
  alvo = scrollY;
  if (foco == SP_FOCO_ABAS || foco == SP_FOCO_BARRA) alvo = 0.0f;
  else if (nVisiveis() > 0 && foco >= 0 && foco < nVisiveis()) {
    float janela = SP_LISTA_BASE - listaTopo();
    topo = topoDe(foco);
    // A ALTURA DA LINHA FOCADA, e nao SP_POSTER_H sempre: na aba Social a linha
    // pode ter 84, 112 ou 138px, e usar a maior empurraria a rolagem 54px alem
    // do necessario num interruptor de 104.
    base = topo + (aba == SP_ABA_SOCIAL ? socialAlt(foco)
                 : aba == SP_ABA_ATIVIDADE ? SPA_H
                 : aba == SP_ABA_AVISOS ? avisos_lista_altura_linha(foco, foco) - 10.0f
                 : linhas[foco].lh);
    // O ar do foco nas duas pontas: o conteudo ja nasce SP_FOCO_AR abaixo do
    // recorte (ver SP_FOCO_AR), entao em cima basta `topo` e embaixo sao dois.
    if (base + 2.0f * SP_FOCO_AR - alvo > janela) alvo = base + 2.0f * SP_FOCO_AR - janela;
    if (topo - alvo < 0.0f) alvo = topo;
  }
  if (alvo < 0.0f) alvo = 0.0f;
  scrollY = anim_mola2_reduzida(&velY, scrollY, alvo, dt, NV_MOLA2_SCROLL,
                                ajustes_animacoes_reduzidas());
}

// "Salvo há 2 horas". A FRASE INTEIRA passa por i18n como FORMATO, nao montada
// de pedacos: "há" e "atrás" trocam de lugar na traducao e uma frase remendada
// aqui sairia "2 horas ago" em ingles.
static void quandoTexto(char *dst, size_t tam, long long quandoS) {
  long long agora = (long long)time(NULL);
  long long d = agora - quandoS;
  if (quandoS <= 0) { dst[0] = 0; return; }
  if (d < 0) d = 0;
  if (d < 90)            snprintf(dst, tam, "%s", i18n("Salvo agora"));
  else if (d < 5400)     snprintf(dst, tam, i18n("Salvo há %d min"), (int)(d / 60));
  else if (d < 172800)   snprintf(dst, tam, i18n("Salvo há %d h"),   (int)(d / 3600));
  else                   snprintf(dst, tam, i18n("Salvo há %d dias"),(int)(d / 86400));
}

// O catalogo guarda a posicao como percentual e o restante em minutos. Com
// os dois valores presentes, esta e uma aproximacao honesta da parte ja vista:
// restante * p / (100 - p). Nao usamos p=100 (divisao por zero) nem inventamos
// minutos quando a fonte so trouxe o percentual.
static int minutosAssistidosAprox(const SPLinha *l) {
  int p = l ? l->progresso : 0;
  if (p <= 0 || p >= 100 || l->restanteMin <= 0) return 0;
  return (int)(((float)l->restanteMin * (float)p /
                (100.0f - (float)p)) + 0.5f);
}

static void restanteTexto(char *dst, size_t tam, const SPLinha *l) {
  if (l->temporada > 0 && l->episodio > 0 && l->restanteMin > 0)
    snprintf(dst, tam, i18n("T%dE%d · %d min restantes"),
             l->temporada, l->episodio, l->restanteMin);
  else if (l->temporada > 0 && l->episodio > 0)
    snprintf(dst, tam, i18n("T%dE%d · retomar"), l->temporada, l->episodio);
  else if (l->restanteMin > 0)
    snprintf(dst, tam, i18n("%d min restantes"), l->restanteMin);
  else
    snprintf(dst, tam, "%s", i18n("Retomar"));
}

static void legendasDaBarra(SPLinha *l) {
  int vistoMin;
  l->txtRestante[0] = l->txtVisto[0] = 0;
  if (l->progresso <= 0) return;
  restanteTexto(l->txtRestante, sizeof l->txtRestante, l);
  vistoMin = minutosAssistidosAprox(l);
  if (vistoMin > 0) {
    char mins[80];
    snprintf(mins, sizeof mins, i18n("%d min assistidos"), vistoMin);
    snprintf(l->txtVisto, sizeof l->txtVisto, "≈%s", mins);
  } else {
    snprintf(l->txtVisto, sizeof l->txtVisto, i18n("%d%% assistido"), l->progresso);
  }
}


// O foco chega como uma mola, mas a superficie nao precisa obedecer a uma
// reta. A smoothstep deixa os primeiros pixels assentarem no fundo escuro e
// segura o ultimo brilho do accent — uma entrada mais "material" sem criar
// overshoot ou uma animacao mais longa.
static float focoVisual(float f) {
  f = anim_clamp(f, 0.0f, 1.0f);
  return f * f * (3.0f - 2.0f * f);
}

// Uma unica receita para a lateral: neutro em repouso; no foco, fill accent
// solido, tinta por contraste e halo curto para destacar sem aureola excessiva.
// No VIDRO o texto do foco nao inverte: a linha segue translucida (aro na cor do
// realce, gfx_vidro_foco) e o texto fica nas cores de repouso.
static float focoTexto(float f) { return ajustes_vidro() ? 0.0f : focoVisual(f); }

static void superficieItem(GfxRect r, float raio, float f, float a) {
  float cr, cg, cb, v = focoVisual(f);
  if (ajustes_vidro()) {
    gfx_vidro_superficie(r, raio, a);
    if (v > .001f) gfx_vidro_foco(r, raio, v, a);
    return;
  }
  ajustes_acento(&cr, &cg, &cb);
  gfx_cor(r, raio, .062f, .066f, .079f, .92f * a);
  if (v > .001f) {
    // Cartoes sao bem mais altos que botoes; reduzir a luz evita que o halo
    // invada a linha acima e a abaixo durante a rolagem.
    botao_luz(r, v * .5f, a);
    gfx_cor(r, raio, cr, cg, cb, v * a);
  }
}

// As duas cores ja ficam no cache de texto. O que muda por quadro e somente a
// opacidade, nunca a chave de rasterizacao: o titulo e a meta fazem a mesma
// travessia que a superficie, sem o estalo claro/escuro no meio da mola.
static void txt_foco_transicao(TxtLinha repouso, TxtLinha foco,
                               float x, float y, float f, float a) {
  if (f < 0.999f)
    txt_desenhar_alpha(repouso, x, y, a * (1.0f - f));
  if (f > 0.001f)
    txt_desenhar_alpha(foco, x, y, a * f);
}

// Os selos precisam acompanhar o texto principal. O fundo usa a mesma tabela
// de BADGE_NEUTRO/BADGE_SOBRE_REALCE de badges.c, mas interpolada em uma unica
// capsula para nao haver uma segunda pilula sobreposta durante o crossfade.
static float badge_foco_transicao(float x, float y, const char *texto,
                                  float f, float a) {
  TxtLinha repouso, foco;
  GfxRect p;
  // Selo interno e uma peca neutra, opaca; nao repete o accent do cartao nem
  // depende de transparencia para separar metadado do fundo.
  repouso = txt_linha(TXT_CAPTION, texto, 235, 235, 235, 255);
  foco = txt_linha(TXT_CAPTION, texto, 226, 226, 226, 255);
  p.x = x; p.y = y; p.w = (float)repouso.w + BADGE_PADX * 2.0f; p.h = BADGE_H;
  if (ajustes_vidro()) gfx_cor(p, 0.5f, 1, 1, 1, 0.10f * a);   /* selo de vidro */
  else gfx_cor(p, 0.5f, .095f, .102f, .116f, a);
  txt_foco_transicao(repouso, foco, x + BADGE_PADX,
                     y + (BADGE_H - (float)repouso.h) * 0.5f, f, a);
  return p.w;
}

static void badge_imdb_foco_transicao(float x, float y, int nota,
                                      float f, float a, int tintaFoco) {
  char texto[8];
  TxtLinha repouso, foco, marca;
  GfxRect p;
  if (nota <= 0) return;
  snprintf(texto, sizeof texto, idioma_ponto_decimal(ajustes_idioma()) ? "%d.%d" : "%d,%d",
           nota / 10, nota % 10);
  repouso = txt_linha(TXT_CAPTION, texto, 235, 235, 235, 255);
  foco = txt_linha(TXT_CAPTION, texto, tintaFoco, tintaFoco, tintaFoco, 255);
  p.x = x; p.y = y; p.w = BADGE_IMDB_W; p.h = BADGE_H;
  // A marca amarela nao troca de estado; somente a nota acompanha o texto da
  // linha. Assim nao ha um segundo amarelo semitransparente sobre o primeiro.
  gfx_cor(p, 5.0f / BADGE_H, 0.961f, 0.773f, 0.094f, a);
  marca = txt_linha(TXT_MINI, "IMDb", 10, 10, 10, 255);
  txt_desenhar_alpha(marca, p.x + (p.w - (float)marca.w) * 0.5f,
                     p.y + (p.h - (float)marca.h) * 0.5f, a);
  txt_foco_transicao(repouso, foco,
                     x + BADGE_IMDB_W + BADGE_IMDB_GAP,
                     y + (BADGE_H - (float)repouso.h) * 0.5f, f, a);
}

// ROTULO DE SECAO ("Continuar", "Nao comecados", "Seus amigos"): CAPTION2 em
// 165 com SP_SECAO_AR de ar acima e uma linha de 1 px a 8 % de branco por
// baixo, de borda a borda da coluna. A linha e o que separa a secao da lista
// anterior sem caixa nem cor — o rotulo colado na primeira linha (foto do
// dono, 21/09/2026) lia como parte do titulo. `y` e o topo do bloco de
// SP_SECAO_H que a proxima linha vai ocupar.
static void desenhaSecao(float x, float y, const char *rotulo, float a) {
  TxtLinha t = txt_linha(TXT_CAPTION, rotulo, 165, 168, 178, 255);
  float ty = y + SP_SECAO_AR;
  txt_desenhar_alpha(t, x, ty, a * 0.95f);
  gfx_cor((GfxRect){ x, ty + (float)t.h + 8.0f, SP_INTERNO, 1.0f }, 0.0f,
          1.0f, 1.0f, 1.0f, 0.08f * a);
}

// `dx` e o deslocamento da animacao de entrada. Ele PRECISA chegar ate aqui: as
// linhas sao desenhadas em coordenada absoluta, e sem somar o mesmo `dx` do
// painel elas ficariam paradas no lugar final enquanto a moldura ainda desliza
// — o conteudo apareceria antes da caixa que o contem.
static const char *tipoRotulo(const SPLinha *l) {
  switch (l->tipoG) {
    case SPT_CANAL:   return "Canal";
    case SPT_COLECAO: return "Coleção";
    case SPT_SERIE:   return "Série";
    default:          return "Filme";
  }
}

// A barra fina de progresso POR CIMA da arte (grade e paisagem): a mesma
// leitura da home, trilho escuro e preenchimento na cor de realce.
static void barraSobreArte(GfxRect arte, int progresso, float a) {
  float p = anim_clamp(progresso / 100.0f, 0.0f, 1.0f), pr, pg, pb;
  GfxRect t = { arte.x + 10.0f, arte.y + arte.h - 14.0f, arte.w - 20.0f, 5.0f };
  GfxRect c = t;
  if (progresso <= 0) return;
  ajustes_acento(&pr, &pg, &pb);
  c.w = t.w * p;
  gfx_cor(t, 0.5f, 0.0f, 0.0f, 0.0f, 0.55f * a);
  if (c.w > t.h) gfx_cor(c, 0.5f, pr, pg, pb, a);
}

// A arte de uma celula: textura pela largura de desenho (tex_obter_larg, a
// nota longa em desenhaLinha), recortada em `cover`, ou o esqueleto.
static void arteCelula(GfxRect r, const char *url, float raio, float a) {
  GLuint tex = url && url[0] ? tex_obter_larg(url, (int)r.w) : 0;
  if (tex) {
    gfx_tex_aspect_atual = tex_aspecto(url);
    gfx_card_forcar_cover_atual = 1.0f;
    gfx_rect(r, tex, GFX_CARD, 0.0f, 0.0f, 0.0f, raio, 0, 0, 0, a);
    gfx_card_forcar_cover_atual = 0.0f;
    gfx_tex_aspect_atual = 0.0f;
  } else {
    gfx_cor(r, raio, NV_COR_ESQUELETO_R, NV_COR_ESQUELETO_G, NV_COR_ESQUELETO_B, a);
  }
}

// GRADE DE POSTERES: o cartaz e o titulo embaixo. A superficie so existe no
// foco — em repouso a grade e so arte, que e o ponto do estilo.
static void desenhaCelulaGrade(int i, float dx, float y, float a) {
  const SPLinha *l = &linhas[i];
  float f = animFoco[i], v = focoTexto(f);
  float x = SP_X + dx + SP_PAD + l->lx;
  int tf = ajustes_tinta_foco(), tf2 = ajustes_tinta_foco2();
  GfxRect poster = { x, y, l->lw, SPG_POSTER_H };
  if (f > 0.01f) {
    GfxRect r = { x - 10.0f, y - 10.0f, l->lw + 20.0f, l->lh + 14.0f };
    superficieItem(r, NV_LINHA_RAIO_PX / r.h, f, a);
  }
  arteCelula(poster, l->poster, 0.06f, a);
  barraSobreArte(poster, l->progresso, a);
  { TxtLinha r = txt_linha_corta(TXT_CAPTION, l->titulo, 240, 240, 240, 255, l->lw);
    TxtLinha fo = txt_linha_corta(TXT_CAPTION, l->titulo, tf, tf, tf, 255, l->lw);
    txt_foco_transicao(r, fo, x, y + SPG_POSTER_H + 10.0f, v, a); }
  (void)tf2;
}

// CARTOES PAISAGEM: a arte 16:9 do catalogo (o cartaz recortado quando ela
// nao veio), o titulo e uma linha de apoio — o que falta, quando ha progresso;
// senao tipo e ano.
static void desenhaCelulaPaisagem(int i, float dx, float y, float a) {
  const SPLinha *l = &linhas[i];
  float f = animFoco[i], v = focoTexto(f);
  float x = SP_X + dx + SP_PAD + l->lx;
  int tf = ajustes_tinta_foco(), tf2 = ajustes_tinta_foco2();
  char apoio[192];
  GfxRect img = { x, y, l->lw, SPP_IMG_H };
  if (f > 0.01f) {
    GfxRect r = { x - 10.0f, y - 10.0f, l->lw + 20.0f, l->lh + 14.0f };
    superficieItem(r, NV_LINHA_RAIO_PX / r.h, f, a);
  }
  arteCelula(img, l->fundo[0] ? l->fundo : l->poster, 0.07f, a);
  barraSobreArte(img, l->progresso, a);
  if (l->progresso > 0 && l->txtRestante[0]) snprintf(apoio, sizeof apoio, "%s", l->txtRestante);
  else if (l->ano > 0) snprintf(apoio, sizeof apoio, "%s · %d", i18n(tipoRotulo(l)), l->ano);
  else snprintf(apoio, sizeof apoio, "%s", i18n(tipoRotulo(l)));
  { TxtLinha r = txt_linha_corta(TXT_BODY, l->titulo, 245, 245, 245, 255, l->lw);
    TxtLinha fo = txt_linha_corta(TXT_BODY, l->titulo, tf, tf, tf, 255, l->lw);
    txt_foco_transicao(r, fo, x, y + SPP_IMG_H + 12.0f, v, a); }
  { TxtLinha r = txt_linha_corta(TXT_CAPTION, apoio, 168, 172, 183, 255, l->lw);
    TxtLinha fo = txt_linha_corta(TXT_CAPTION, apoio, tf2, tf2, tf2, 255, l->lw);
    txt_foco_transicao(r, fo, x, y + SPP_IMG_H + 48.0f, v, a * 0.95f); }
}

static void desenhaLinha(int i, float dx, float y, float a) {
  const SPLinha *l = &linhas[i];
  float f = animFoco[i];
  float px = SP_X + dx + SP_PAD, tx = SP_X + dx + SP_TEXTO_X;
  char buf[192];
  GfxRect poster = { px, y, SP_POSTER_W, SP_POSTER_H };
  float v = focoTexto(f);   // cor do texto e dos selos no foco (vidro: nao inverte)
  int tintaFoco = ajustes_tinta_foco();
  int tintaFoco2 = ajustes_tinta_foco2();

  if (f > 0.01f) {
    GfxRect r = { px - 12.0f, y - SP_FOCO_PADY, SP_INTERNO + 24.0f,
                  SP_POSTER_H + SP_FOCO_PADY * 2.0f };
    superficieItem(r, NV_LINHA_RAIO_PX / r.h, f, a);
  } else {
    GfxRect r = { px - 12.0f, y - SP_FOCO_PADY, SP_INTERNO + 24.0f,
                  SP_POSTER_H + SP_FOCO_PADY * 2.0f };
    superficieItem(r, NV_LINHA_RAIO_PX / r.h, 0.0f, a);
  }

  // PELA LARGURA DE DESENHO (92), e nao tex_obter: este decodificava cada
  // cartaz a 640 de largura (~2,4 MB) para desenha-lo a 92: 121 salvos rolados
  // pedem ~290 MB a um cache de 300 (o log da C9 de 24/09, com o painel
  // aberto, mostrava gpu-cache=221 269MB), e cada quadro amostrava texturas
  // sete vezes maiores que o destino. tex_cache.h: "Prefira esta a tex_obter
  // em qualquer arte de lista". A Biblioteca ja fazia assim (96).
  { GLuint tex = l->poster[0] ? tex_obter_larg(l->poster, SP_POSTER_W) : 0;
    if (tex) {
      gfx_tex_aspect_atual = tex_aspecto(l->poster);
      gfx_rect(poster, tex, GFX_CARD, 0.0f, 0.0f, 0.0f, 0.08f, 0, 0, 0, a);
      gfx_tex_aspect_atual = 0.0f;
    } else {
      // Esqueleto VISIVEL (#2C2C2C), o mesmo da home e da biblioteca: um
      // retangulo da cor do fundo le como card quebrado, nao como carregando.
      gfx_cor(poster, 0.08f, NV_COR_ESQUELETO_R, NV_COR_ESQUELETO_G,
              NV_COR_ESQUELETO_B, a);
    } }

  // A TINTA FAZ CROSSFADE entre duas texturas fixas do cache. Antes ela trocava
  // em degrau no meio da mola: sobre o accent branco isso era exatamente o
  // "flash" da captura — o cartao ja estava claro, mas o texto ainda parecia
  // de repouso por um quadro. As duas chaves sao estaveis; so o alpha varia.
  {
    // Nao somar canais a 255: o cache aceita bytes e o estouro transforma
    // branco em vermelho quando o accent pede tinta clara.
    { TxtLinha repouso = txt_linha_corta(TXT_CALLOUT, l->titulo,
                                         245, 245, 245, 255, SP_TEXTO_W);
      TxtLinha foco = txt_linha_corta(TXT_CALLOUT, l->titulo,
                                      tintaFoco, tintaFoco, tintaFoco, 255, SP_TEXTO_W);
      txt_foco_transicao(repouso, foco, tx, y + 4.0f, v, a); }
    // META: tipo, ano e nota sao selos da mesma tabela. A estrela solta que
    // existia aqui nao dizia de onde vinha a nota e ainda duplicava o IMDb
    // quando a marca era desenhada ao lado.
    { float bx = tx, by = y + 42.0f;
      float w;
      w = badge_foco_transicao(bx, by, i18n(tipoRotulo(l)), v, a);
      bx += w + BADGE_GAP;
      if (l->meta[0]) {
        w = badge_foco_transicao(bx, by, l->meta, v, a);
        bx += w + BADGE_GAP;
      }
      if (l->nota > 0 && bx + badge_imdb_largura(l->nota) <= tx + SP_TEXTO_W) {
        badge_imdb_foco_transicao(bx, by, l->nota, v, a, tintaFoco);
        bx += badge_imdb_largura(l->nota) + BADGE_GAP;
      }
      // A CATEGORIA DA PESSOA, quando a lista nao esta agrupada por ela: e a
      // resposta visivel do "Mover para categoria" — sem o selo, mover com a
      // lista agrupada por progresso nao mudaria nada na tela.
      if (l->cat && grupoAtual != SORG_GRUPO_CATEGORIA) {
        const char *nome = sorg_categoria_nome_id(l->cat);
        if (nome && bx + txt_largura(TXT_CAPTION, nome) + BADGE_PADX * 2.0f <= tx + SP_TEXTO_W)
          badge_foco_transicao(bx, by, nome, v, a);
      } } }

  if (l->progresso > 0) {
    float p = anim_clamp(l->progresso / 100.0f, 0.0f, 1.0f);
    GfxRect trilho = { tx, y + 90.0f, SP_BARRA_W, SP_BARRA_H };
    GfxRect cheio  = { tx, y + 90.0f, SP_BARRA_W * p, SP_BARRA_H };
    float pr, pg, pb;
    ajustes_acento(&pr, &pg, &pb);
    // TRILHO cinza escuro (branco a 14 % sobre o painel) e PREENCHIMENTO NA
    // COR DE REALCE (dono, 21/09/2026) — era branco em repouso e a barra nao
    // dizia que era a mesma coisa que a barra dos cards da home. Sobre a
    // linha em foco (superficie na cor de realce) o trilho e a tinta a 22 % e
    // o preenchimento e a tinta cheia: realce sobre realce sumia.
    // A MESMA tinta do texto da linha (ajustes_acento_tinta), e nao um degrau
    // proprio: com o degrau em 0,52 a barra saia escura sobre o azul enquanto
    // o texto ao lado saia branco — dois criterios para a mesma superficie.
    { float ti = ajustes_acento_tinta(NULL, NULL, NULL);
      // Duas camadas pequenas, ambas sobre a mesma barra: a leitura da
      // progressao nunca some enquanto a linha troca de superficie.
      gfx_cor(trilho, 0.5f, 1.0f, 1.0f, 1.0f, 0.14f * (1.0f - v) * a);
      gfx_cor(trilho, 0.5f, ti, ti, ti, 0.22f * v * a);
      if (cheio.w > SP_BARRA_H) {
        gfx_cor(cheio, 0.5f, pr, pg, pb, (1.0f - v) * a);
        gfx_cor(cheio, 0.5f, ti, ti, ti, v * a);
      } }
    // O tempo ja visto vem da mesma posicao/remaining reais do catalogo. Quando
    // ha duracao suficiente para derivar minutos, o prefixo "≈" deixa claro o
    // arredondamento; sem isso, cai para percentual em vez de fabricar tempo.
    // A etiqueta fica ao lado da barra, em uma largura reservada fixa: assim a
    // largura do trilho nao muda a cada quadro nem rasteriza uma linha nova.
    // HIERARQUIA (dono, 21/09/2026): o que FALTA e o dado — "T3E4 · 43 min
    // restantes" em tinta principal ao lado do trilho; o "≈35 min assistidos"
    // e contexto e vai embaixo, em secundario. Antes os dois tinham a mesma
    // cor e o restante ficava em segundo plano.
      { int c2Repouso = 168;
        float labelX = tx + SP_BARRA_W + SP_BARRA_LABEL_GAP;
      { TxtLinha repouso = txt_linha_corta(TXT_CAPTION, l->txtRestante, 235, 235, 235, 255,
                                           SP_TEXTO_W - SP_BARRA_W - SP_BARRA_LABEL_GAP);
        TxtLinha foco = txt_linha_corta(TXT_CAPTION, l->txtRestante, tintaFoco, tintaFoco,
                                        tintaFoco, 255,
                                        SP_TEXTO_W - SP_BARRA_W - SP_BARRA_LABEL_GAP);
        txt_foco_transicao(repouso, foco, labelX,
                           y + 90.0f + (SP_BARRA_H - (float)repouso.h) * 0.5f, v, a); }
      { TxtLinha repouso = txt_linha(TXT_CAPTION, l->txtVisto, c2Repouso, c2Repouso + 4,
                                     c2Repouso + 14, 255);
        TxtLinha foco = txt_linha(TXT_CAPTION, l->txtVisto, tintaFoco2,
                                  tintaFoco2, tintaFoco2, 255);
        txt_foco_transicao(repouso, foco, tx, y + 108.0f, v, a * 0.95f); } }
  } else {
    quandoTexto(buf, sizeof buf, l->quandoS);
    if (buf[0]) {
      TxtLinha repouso = txt_linha_corta(TXT_CAPTION, buf, 168, 172, 183, 255, SP_TEXTO_W);
      TxtLinha foco = txt_linha_corta(TXT_CAPTION, buf, tintaFoco2,
                                      tintaFoco2, tintaFoco2, 255, SP_TEXTO_W);
      txt_foco_transicao(repouso, foco, tx, y + 92.0f, v, a * 0.9f);
    }
  }
}

// Uma linha da aba Social: cartaz, titulo, a CARA e o nome de quem mandou,
// filme-ou-serie, a nota do IMDb, a frase — e, quando ainda nao foi lida, a
// barra de acento na borda esquerda.
//
// O cartaz e a MESMA tex_cache do resto do app; a recomendacao guarda a URL do
// poster, a do avatar e a nota no proprio arquivo (recomenda.h), entao a linha
// se desenha sem depender do catalogo. ISSO E O PONTO: o titulo recomendado
// costuma NAO estar no catalogo de quem recebe — se a nota fosse procurada
// ali, ela apareceria justamente nas linhas em que menos importa.
//
// AS QUATRO FAIXAS, dentro dos 138px do cartaz:
//   +0    titulo                       (TXT_CALLOUT, 28)
//   +38   disco de 30 + "Nome · há 2 h"
//   +76   selos: [Filme] [IMDb 8,3]    (REC_SELO_H = 30)
//   +108  a frase entre aspas          (TXT_CAPTION, 22)
#define SPR_AVATAR   30.0f
#define SPR_AV_GAP   12.0f
#define SPR_Y_NOME   38.0f
#define SPR_Y_SELOS  76.0f
#define SPR_Y_FRASE 108.0f
// Barra de "ainda nao lida" na borda esquerda da linha.
#define SPR_BARRA_W   6.0f
// A LINHA DE SUGESTAO. 56 e o mesmo disco do cartao de abertura (RC_AVATAR) e o
// menor em que a INICIAL ainda se le a 3 m — sugestao de conta Nuvio quase
// nunca tem foto, entao a inicial e o caso comum e nao a excecao.
#define SPS_SUG_AV   56.0f
#define SPS_SUG_GAP  18.0f
#define SPS_SUG_PADX 14.0f
#define SPS_AMIGO_AV 48.0f
#define SPS_AMIGO_GAP 16.0f

// `linha` e a posicao na lista da aba (de onde sai a animacao de foco) e `idx`
// a posicao na lista de recomendacoes. Os dois coincidem hoje — as
// recomendacoes vem primeiro — e sao parametros separados para que continuem
// coincidindo por construcao e nao por sorte no dia em que algo entrar antes.
static void desenhaRecLinha(int linha, int idx, float dx, float y, float a) {
  const RecItem *r = &recs[idx];
  float f = (linha >= 0 && linha < SP_MAX) ? animFoco[linha] : 0.0f;
  float px = SP_X + dx + SP_PAD, tx = SP_X + dx + SP_TEXTO_X;
  char buf[320], quando[64];
  GfxRect poster = { px, y, SP_POSTER_W, SP_POSTER_H };

  { GfxRect card = { px - 12.0f, y - SP_FOCO_PADY, SP_INTERNO + 24.0f,
                     SP_POSTER_H + SP_FOCO_PADY * 2.0f };
    superficieItem(card, 14.0f / card.h, f, a); }

  if (!r->visto) {
    // A MARCA DE "NAO LIDA" E UMA BARRA, e nao mais um ponto de 10px.
    //
    // O ponto morava no vao entre o cartaz e o texto, tinha a area de um grao
    // de arroz a tres metros e sumia por completo quando a linha ganhava a
    // pilula clara do foco (azul 0.42/0.72/0.98 sobre branco 0.96). A barra
    // ocupa a altura INTEIRA do cartaz na borda da camada, onde nada mais
    // desenha, e por isso continua visivel com e sem foco.
    //
    // NA COR DE ACENTO DO APARELHO, e nao no azul cravado de antes: o dono
    // escolhe o acento em Ajustes e toda marca de estado do app ja o obedece.
    //
    // E POR ISSO ELA FICA FORA DA PILULA DO FOCO, em px-24 contra os px-12 em
    // que a pilula comeca. O acento PADRAO e BRANCO: pintada por dentro da
    // pilula clara, a barra sumiria justamente na linha em que o dedo esta.
    // CONFERIDO nas duas capturas — tema OCEANO (azul) e tema padrao (branco).
    //
    // O RAIO E 3px EXPRESSOS EM ALTURAS, e nao 0.5: o SDF de gfx.c normaliza
    // pela ALTURA do retangulo (uAspect = w/h), entao 0.5 num retangulo de
    // 6x138 pede um raio de 69px e o que sai e uma lente pontuda de 50px — foi
    // exatamente o que a primeira captura mostrou.
    float ar, ag, ab;
    GfxRect barra = { px - 24.0f, y, SPR_BARRA_W, SP_POSTER_H };
    ajustes_acento(&ar, &ag, &ab);
    { float ti = anim_mistura(ar, ajustes_acento_tinta(NULL, NULL, NULL),
                              focoTexto(f));
      ar = ag = ab = ti; }
    gfx_cor(barra, SPR_BARRA_W * 0.5f / SP_POSTER_H, ar, ag, ab, a);
  }

  { GLuint tex = r->poster[0] ? tex_obter_larg(r->poster, SP_POSTER_W) : 0;
    if (tex) {
      gfx_tex_aspect_atual = tex_aspecto(r->poster);
      gfx_rect(poster, tex, GFX_CARD, 0.0f, 0.0f, 0.0f, 0.08f, 0, 0, 0, a);
      gfx_tex_aspect_atual = 0.0f;
    } else {
      gfx_cor(poster, 0.08f, NV_COR_ESQUELETO_R, NV_COR_ESQUELETO_G,
              NV_COR_ESQUELETO_B, a);
    } }

  { TxtLinha repouso = txt_linha_corta(TXT_CALLOUT, r->titulo,
                                        245, 245, 245, 255, SP_TEXTO_W);
    TxtLinha foco = txt_linha_corta(TXT_CALLOUT, r->titulo,
                                     ajustes_tinta_foco(),
                                     ajustes_tinta_foco(),
                                     ajustes_tinta_foco(), 255, SP_TEXTO_W);
    txt_foco_transicao(repouso, foco, tx, y + 2.0f, focoTexto(f), a); }

  // "Gustavo · há 2 h" — as PARTES passam por i18n e a juncao nao, pela mesma
  // razao de metaTexto: a chave da tabela e uma string inteira, e a frase
  // montada nunca existiria como chave.
  rec_quando_texto(quando, sizeof quando, r->criado);
  if (quando[0]) snprintf(buf, sizeof buf, "%s · %s", r->deNome, quando);
  else           snprintf(buf, sizeof buf, "%s", r->deNome);
  // As DUAS linhas de baixo invertem junto com o titulo: com a pilula clara,
  // cinza-claro sobre claro fica ilegivel — foi o que a captura mostrou antes
  // de isto existir.
  { int esc = f > 0.5f;
    int c2 = esc ? ajustes_tinta_foco2() : 168;
    int c3 = esc ? ajustes_tinta_foco2() : 214;
    // O DISCO DA FOTO ANTES DO NOME. Com quatro recomendacoes na tela, a cara
    // e o que distingue uma linha da outra antes de qualquer leitura — era o
    // pedido do dono, e e o unico item da linha que nao depende de ler.
    { GfxRect av = { tx, y + SPR_Y_NOME, SPR_AVATAR, SPR_AVATAR };
      rec_avatar(av, r->deAvatar, r->deNome, r->de, a); }
    { float nx = tx + SPR_AVATAR + SPR_AV_GAP;
      TxtLinha t = txt_linha_corta(TXT_CAPTION, buf, c2, c2 + 4, c2 + 14, 255,
                                   SP_TEXTO_W - (SPR_AVATAR + SPR_AV_GAP));
      txt_desenhar_alpha(t, nx, y + SPR_Y_NOME + (SPR_AVATAR - t.h) * 0.5f,
                         a * 0.95f); }
    // FILME OU SÉRIE, E A NOTA. `tipo` sempre existiu no RecItem e nunca era
    // desenhado: a linha nao dizia se o amigo estava mandando um filme de duas
    // horas ou oito temporadas.
    { float sx = tx;
      int pct, te, ee, est;
      sx += rec_selo_tipo(sx, y + SPR_Y_SELOS, r->tipo, esc, a) + REC_SELO_GAP;
      if (r->nota > 0) sx += rec_selo_imdb(sx, y + SPR_Y_SELOS, r->nota, esc, a) + REC_SELO_GAP;
      // O MEU ESTADO NO QUE ME MANDARAM (tela A): "Voce comecou › T1E3" ou
      // "Voce viu", pelo catalogo. Sem estado, nada.
      est = socialvis_meu_estado(r->imdb, &pct, &te, &ee);
      if (est == 2) svd_chip(sx + 8.0f, y + SPR_Y_SELOS + (REC_SELO_H - SVD_CHIP_H) * 0.5f,
                             "Você viu", 1, esc, a);
      else if (est == 1) {
        char d[32];
        float cy = y + SPR_Y_SELOS + (REC_SELO_H - SVD_CHIP_H) * 0.5f;
        sx += 8.0f + svd_chip(sx + 8.0f, cy, "Você começou", 1, esc, a);
        sx += svd_seta(sx, cy, esc, a);
        if (te > 0 && ee > 0) snprintf(d, sizeof d, i18n("T%dE%d"), te, ee);
        else snprintf(d, sizeof d, "%d%%", pct);
        svd_chip(sx, cy, d, 0, esc, a);
      } }
    { const char *frase = rec_frase(r);
      if (frase[0]) {
        snprintf(buf, sizeof buf, "\xe2\x80\x9c%s\xe2\x80\x9d", frase);
        { TxtLinha t = txt_linha_corta(TXT_CAPTION, buf, c3, c3 + 4, c3 + 14, 255,
                                       SP_TEXTO_W);
          txt_desenhar_alpha(t, tx, y + SPR_Y_FRASE, a * 0.95f); }
      } } }
}

// A LINHA DE ABAS. Sem animacao de cor de proposito: a cor faz parte da chave
// do cache de linhas de text.c, e uma cor por quadro cria uma rasterizacao TTF
// e uma textura GL por quadro — estourado o orcamento, a linha simplesmente
// NAO E DESENHADA (ver a nota longa em ctxmenu.c). O foco aparece no anel, que
// e geometria e nao custa texto.
// Largura da faixa de abas, para a contagem do cabecalho parar antes dela.
static float abasLargura(void) {
  const char *rot[SP_ABA_N]; int i; float w = 0.0f;
  int novasRec = recomenda_n_novas(), novasAv = avisos_n_novos();
  rot[SP_ABA_SALVOS] = "Salvos"; rot[SP_ABA_ATIVIDADE] = "Atividade";
  rot[SP_ABA_SOCIAL] = "Amigos"; rot[SP_ABA_AVISOS] = "Avisos";
  for (i = 0; i < SP_ABA_N; i++) {
    int ativa = (i == aba);
    int novas = i == SP_ABA_SOCIAL ? novasRec : i == SP_ABA_AVISOS ? novasAv : 0;
    TxtLinha t;
    if (!abaExiste(i)) continue;
    t = txt_linha(TXT_CALLOUT, i18n(rot[i]), 176, 176, 176, 255);
    w += t.w + (ativa ? 20.0f : 0.0f) + ((!ativa && novas > 0) ? 34.0f : 0.0f) + SP_ABA_GAP;
  }
  return w;
}

static void desenhaAbas(float dx, float a) {
  const char *rot[SP_ABA_N];
  float x = SP_X + dx + SP_PAD;
  int i, novasRec = recomenda_n_novas(), novasAv = avisos_n_novos();
  rot[SP_ABA_SALVOS] = "Salvos";
  rot[SP_ABA_ATIVIDADE] = "Atividade";
  rot[SP_ABA_SOCIAL] = "Amigos";
  rot[SP_ABA_AVISOS] = "Avisos";
  for (i = 0; i < SP_ABA_N; i++) {
    int ativa = (i == aba);
    int focada = ativa && foco == SP_FOCO_ABAS;
    int cor = focada ? ajustes_tinta_foco() : ativa ? 245 : 176;
    int novas = i == SP_ABA_SOCIAL ? novasRec : i == SP_ABA_AVISOS ? novasAv : 0;
    TxtLinha t;
    if (!abaExiste(i)) continue;
    t = txt_linha(TXT_CALLOUT, i18n(rot[i]), cor, cor, cor, 255);
    // O selo so aparece na aba que NAO esta aberta. Ele responde "ha algo
    // novo la?"; com a aba Social na tela, a propria lista responde isso, e o
    // numero ficaria repetido a dois centimetros da contagem do cabecalho.
    // O SELO E PEQUENO E MORA DENTRO DA PILULA (dono, 20/09/2026: "o numero
    // ta feio, menos amador"): 24 px, numeral TXT_MINI, 10 px depois do
    // rotulo, e a pilula cresce para ele — antes eram 32 px encostados na
    // borda, e a contagem do cabecalho vinha logo atras sem folga.
    float selo = (!ativa && novas > 0) ? 34.0f : 0.0f;
    float margem = ativa ? 20.0f : 0.0f;
    GfxRect p = { x, SP_ABAS_Y, t.w + margem + selo, SP_ABAS_H };
    // Aba ativa fica como rotulo e indicador curto quando a lista tem foco;
    // so vira pilula accent quando o D-pad esta realmente na faixa de abas.
    if (focada) superficieItem(p, NV_RAIO_PILL, 1.0f, a);
    else if (ativa) {
      float ar, ag, ab;
      ajustes_acento(&ar, &ag, &ab);
      gfx_cor((GfxRect){x + 6.0f, SP_ABAS_Y + SP_ABAS_H - 3.0f,
                         t.w + 8.0f, 3.0f}, 0.5f, ar, ag, ab, a);
    }
    txt_desenhar_alpha(t, x + (ativa ? 10.0f : 0.0f),
                       SP_ABAS_Y + (SP_ABAS_H - t.h) * 0.5f, a);
    if (selo > 0.0f) {
      char n[16];
      GfxRect b;
      TxtLinha tn;
      float br, bg, bb;
      snprintf(n, sizeof n, "%d", novas > 99 ? 99 : novas);
      ajustes_acento(&br, &bg, &bb);
      { int ink = ajustes_tinta_foco();
        tn = txt_linha(TXT_MINI, n, ink, ink, ink, 255); }
      b.w = 24.0f; b.h = 24.0f;
      b.x = x + t.w + 10.0f;
      b.y = SP_ABAS_Y + (SP_ABAS_H - b.h) * 0.5f;
      gfx_rect(b, 0, GFX_DISCO, 0, 0, 0, 0, br, bg, bb, a);
      txt_desenhar_alpha(tn, b.x + (b.w - tn.w) * 0.5f,
                         b.y + (b.h - tn.h) * 0.5f, a);
    }
    x += p.w + SP_ABA_GAP;
  }
}

// A BARRA DE OPCOES. Cada pilula diz O QUE ela muda (rotulo pequeno em cima)
// e O QUE VALE AGORA (embaixo, na tinta principal): "Ordenar / Recentes". A
// ultima e a porta das categorias. Mesma superficie e mesmo foco das linhas,
// e por isso o mesmo vidro quando o vidro esta ligado.
static void chipTextos(int k, const char **cap, const char **val, const char **icone,
                       char *buf, size_t tam) {
  *cap = NULL; *val = NULL; *icone = NULL;
  if (aba == SP_ABA_SOCIAL) { *cap = "Organizar"; *val = SOCIAL_CURTO[sorg_social()]; return; }
  switch (k) {
    case SPB_ORDEM:  *cap = "Ordenar"; *val = ORDEM_CURTO[sorg_ordem()]; break;
    case SPB_GRUPO:  *cap = "Agrupar"; *val = GRUPO_CURTO[sorg_grupo()]; break;
    case SPB_ESTILO: *cap = "Estilo";  *val = ESTILO_CURTO[sorg_estilo()];
                     *icone = ESTILO_ICONE[sorg_estilo()]; break;
    default:
      if (sorg_n_categorias() < 1) { *val = "Nova categoria"; *icone = "mais"; }
      else {
        *cap = "Categorias";
        snprintf(buf, tam, "%d", sorg_n_categorias());
        *val = buf; *icone = "aj_folders";
      }
      break;
  }
}

static void desenhaBarra(float dx, float a) {
  float x = SP_X + dx + SP_PAD;
  int k, n = nChips(), tf = ajustes_tinta_foco(), tf2 = ajustes_tinta_foco2();
  float tinta = ajustes_acento_tinta(NULL, NULL, NULL);
  for (k = 0; k < n; k++) {
    const char *cap, *val, *icone;
    char buf[24];
    float f = animBarra[k], v = focoTexto(f), w, ix;
    TxtLinha vr, vf, cr, cf;
    GfxRect r;
    chipTextos(k, &cap, &val, &icone, buf, sizeof buf);
    vr = txt_linha(TXT_CAPTION, val, 240, 240, 244, 255);
    vf = txt_linha(TXT_CAPTION, val, tf, tf, tf, 255);
    w = (float)vr.w;
    if (cap) {
      cr = txt_linha(TXT_MINI, cap, 150, 154, 166, 255);
      cf = txt_linha(TXT_MINI, cap, tf2, tf2, tf2, 255);
      if ((float)cr.w > w) w = (float)cr.w;
    }
    w += 40.0f + (icone ? 30.0f : 0.0f);
    r = (GfxRect){ x, SP_OPC_Y, w, SP_OPC_H };
    // Em repouso a pilula precisa existir: a superficie neutra das linhas
    // some sobre o painel escuro, e uma opcao invisivel nao se le como botao.
    superficieItem(r, 18.0f / SP_OPC_H, f, a);
    if (!ajustes_vidro())
      gfx_cor(r, 18.0f / SP_OPC_H, 1.0f, 1.0f, 1.0f, 0.07f * (1.0f - focoVisual(f)) * a);
    ix = x + 20.0f;
    if (icone) {
      float ic = anim_mistura(220.0f / 255.0f, tinta, v);
      gfx_icone((GfxRect){ ix, SP_OPC_Y + (SP_OPC_H - 22.0f) * 0.5f, 22.0f, 22.0f },
                icone, ic, ic, ic, a);
      ix += 30.0f;
    }
    if (cap) {
      float bloco = (float)cr.h + 2.0f + (float)vr.h;
      float ty = SP_OPC_Y + (SP_OPC_H - bloco) * 0.5f;
      txt_foco_transicao(cr, cf, ix, ty, v, a * 0.95f);
      txt_foco_transicao(vr, vf, ix, ty + (float)cr.h + 2.0f, v, a);
    } else {
      txt_foco_transicao(vr, vf, ix, SP_OPC_Y + (SP_OPC_H - (float)vr.h) * 0.5f, v, a);
    }
    x += w + NV_CTRL_VAO;
  }
}

// A ESCOLHA por cima do painel: um veu sobre a lista, a folha no centro do
// painel com o titulo, a frase de apoio e as opcoes. A atual leva o "check".
static void desenhaPop(float a) {
  float e, h, w, x, y, topoL, janela;
  int i, vis, algumIcone;
  if (!pop) return;
  e = anim_suave(popEntrada) * a;
  vis = popN < POP_VISIVEIS ? popN : POP_VISIVEIS;
  janela = (float)vis * (POP_LINHA_H + POP_LINHA_VAO) - POP_LINHA_VAO;
  topoL = popSub[0] ? 118.0f : 84.0f;
  w = SP_W - 80.0f;
  h = topoL + janela + 28.0f;
  x = SP_X + 40.0f;
  y = SP_Y + (SP_H - h) * 0.42f + (1.0f - e) * 18.0f;
  gfx_cor((GfxRect){ SP_X, SP_Y, SP_W, SP_H }, 28.0f / SP_H, 0.0f, 0.0f, 0.0f, 0.50f * e);
  { GfxRect r = { x, y, w, h };
    if (ajustes_vidro()) gfx_vidro_folha(r, 26.0f / h, e);
    else {
      gfx_cor(r, 26.0f / h, 0.085f, 0.089f, 0.104f, e);
      gfx_cor((GfxRect){ x, y, w, 1.0f }, 0.0f, 1, 1, 1, 0.06f * e);
    } }
  { TxtLinha t = txt_linha_corta(TXT_CALLOUT, popTitulo, 246, 247, 252, 255, w - 64.0f);
    txt_desenhar_alpha(t, x + 32.0f, y + 26.0f, e); }
  if (popSub[0]) {
    TxtLinha t = txt_linha_corta(TXT_CAPTION, popSub, 168, 172, 183, 255, w - 64.0f);
    txt_desenhar_alpha(t, x + 32.0f, y + 72.0f, e * 0.95f);
  }
  // Com icone em alguma opcao, todas reservam a coluna dele: texto alinhado
  // numa coluna so, como num menu, e nao em degraus.
  { int k; algumIcone = 0; for (k = 0; k < popN; k++) if (popL[k].icone) algumIcone = 1; }
  gfx_recorte(x, y + topoL - 8.0f, w, janela + 16.0f);
  for (i = 0; i < popN; i++) {
    float ry = y + topoL + (float)i * (POP_LINHA_H + POP_LINHA_VAO) - popRol;
    float f = popAnim[i], v = focoTexto(f), tx = x + 44.0f;
    int tf = ajustes_tinta_foco();
    float tinta = ajustes_acento_tinta(NULL, NULL, NULL);
    GfxRect r = { x + 20.0f, ry, w - 40.0f, POP_LINHA_H };
    if (ry + POP_LINHA_H < y + topoL - 8.0f || ry > y + topoL + janela + 8.0f) continue;
    if (f > 0.01f) superficieItem(r, 16.0f / POP_LINHA_H, f, e);
    if (popL[i].icone) {
      float ic = anim_mistura(200.0f / 255.0f, tinta, v);
      gfx_icone((GfxRect){ tx, ry + (POP_LINHA_H - 24.0f) * 0.5f, 24.0f, 24.0f },
                popL[i].icone, ic, ic, ic, e);
    }
    if (algumIcone) tx += 38.0f;
    { TxtLinha rp = txt_linha_corta(TXT_CALLOUT, popL[i].rot, 236, 237, 242, 255, w - 160.0f);
      TxtLinha fo = txt_linha_corta(TXT_CALLOUT, popL[i].rot, tf, tf, tf, 255, w - 160.0f);
      txt_foco_transicao(rp, fo, tx, ry + (POP_LINHA_H - (float)rp.h) * 0.5f, v, e); }
    if (popL[i].marcado) {
      float ar, ag, ab;
      ajustes_acento(&ar, &ag, &ab);
      if (v > 0.5f) ar = ag = ab = tinta;
      gfx_icone((GfxRect){ r.x + r.w - 52.0f, ry + (POP_LINHA_H - 26.0f) * 0.5f, 26.0f, 26.0f },
                "check", ar, ag, ab, e);
    }
  }
  gfx_sem_recorte();
}

// O ESTADO VAZIO DIZ POR QUE ESTA VAZIO, e nao so que esta.
//
// A versao anterior dizia "quando um amigo mandar um filme, ele aparece aqui",
// que e verdade e nao ajuda em nada: o dono tinha ZERO contatos e a tela nao
// dava nenhuma pista de que faltava um passo — o vinculo do Trakt so alcanca
// quem JA usa o servico, e em 15/09/2026 isso eram zero pessoas. Aqui a tela
// diz a razao, mostra o codigo que ele precisa ditar e oferece a porta.
// Devolve o y logo abaixo do texto, para a linha-botao nascer colada nele em
// vez de boiar no fim do painel.
// A PERGUNTA DO NIVEL (alcance), por extenso. Mesma forma da pergunta de
// aparecer: o que cada resposta faz, sem sermao, e onde mudar depois.
static void desenhaAlcancePergunta(float dx, float y0, float a) {
  float x = SP_X + dx + SP_PAD, y = y0 + 8.0f;
  { TxtLinha t = txt_linha(TXT_CALLOUT, "Quem vê o que você assiste?", 246, 247, 252, 255);
    txt_desenhar_alpha(t, x, y, a); y += t.h + 18.0f; }
  y += txt_bloco(TXT_CAPTION,
      "Seus amigos podem ver o que você está assistindo, o que terminou e do que gostou. Nada sai desta TV antes de você escolher.",
      214, 218, 228, x, y, SP_INTERNO, 30.0f, a * 0.95f, 4) + 14.0f;
  y += txt_bloco(TXT_CAPTION,
      "Amigos dos seus amigos veem só o título e a ação, sem a sua foto.",
      190, 194, 204, x, y, SP_INTERNO, 30.0f, a * 0.9f, 2) + 14.0f;
  txt_bloco(TXT_CAPTION, "Dá para mudar quando quiser, no fim desta aba.",
            160, 164, 175, x, y, SP_INTERNO, 30.0f, a * 0.85f, 2);
}

static void desenhaSocialVazio(float dx, float y0, float a) {
  float x = SP_X + dx + SP_PAD;
  float y = y0 + 8.0f;
  const char *cod = recomenda_meu_codigo();
  { TxtLinha t = txt_linha(TXT_CALLOUT, "Nenhuma recomendação ainda",
                           240, 242, 248, 255);
    txt_desenhar_alpha(t, x, y, a * 0.96f); y += t.h + 14.0f; }
  // EM BLOCO: a coluna do painel tem 688px e a frase tem duas oracoes; numa
  // linha so, a captura saiu cortada no meio.
  y += txt_bloco(TXT_CAPTION,
      "Quando alguém da sua lista te recomendar um filme ou série, ele aparece aqui.",
      190, 194, 204, x, y, SP_INTERNO, 30.0f, a * 0.9f, 3) + 22.0f;
  if (cod[0]) {
    // O CODIGO TAMBEM AQUI, e nao so na tela de amigos: este e o painel que o
    // dono abre com uma tecla, e ditar seis caracteres ao telefone e a acao que
    // resolve uma lista vazia sem depender de ninguem ter aceitado aparecer.
    TxtLinha r = txt_linha(TXT_CAPTION, "Seu código", 160, 164, 175, 255);
    TxtLinha c = txt_linha(TXT_TITULO2, cod, 246, 248, 255, 255);
    txt_desenhar_alpha(r, x, y, a * 0.88f);
    y += r.h + 6.0f;
    txt_desenhar_alpha(c, x, y, a);
    y += c.h + 20.0f;
  }
  { TxtLinha t = txt_linha_corta(TXT_CAPTION,
        "Peça o código do seu amigo e adicione-o abaixo.",
        168, 172, 182, 255, SP_INTERNO);
    txt_desenhar_alpha(t, x, y, a * 0.85f); }
}

// A PERGUNTA DA PRIMEIRA ENTRADA, por extenso.
//
// AS QUATRO FRASES SAO A FUNCAO INTEIRA, e a ordem delas foi escolhida: o que
// os OUTROS passam a ver vem primeiro, o que eles NAO veem vem logo depois, o
// preco de recusar (nenhum) vem em terceiro e a reversibilidade fecha. Quem ler
// so a primeira ja sabe o essencial; quem ler ate o fim nao encontra nenhuma
// ressalva que contradiga o comeco. Nao ha frase aqui que o codigo nao cumpra:
// o servidor so guarda nome, foto e contatos, e a consulta de sugestao filtra
// por `descobrivel = 1` nos DOIS ramos justamente para esta lista ser verdade.
static void desenhaConsentimento(float dx, float y0, float a) {
  float x = SP_X + dx + SP_PAD;
  float y = y0 + 8.0f;
  // TXT_CALLOUT E NAO TXT_TITULO2. Com o titulo grande a pergunta saiu da
  // captura como "Aparecer para outras pessoa" — 688px de coluna nao cabem uma
  // frase de 29 caracteres naquele corpo, e um enunciado cortado ao meio e
  // pior que um enunciado menor.
  { TxtLinha t = txt_linha(TXT_CALLOUT, "Aparecer para outras pessoas?",
                           246, 247, 252, 255);
    txt_desenhar_alpha(t, x, y, a); y += t.h + 18.0f; }
  y += txt_bloco(TXT_CAPTION,
      "Se você aceitar, quem já te segue no Trakt e os amigos dos seus amigos passam a ver seu nome e sua foto numa lista de sugestões, e podem te adicionar como contato.",
      214, 218, 228, x, y, SP_INTERNO, 30.0f, a * 0.95f, 4) + 18.0f;
  y += txt_bloco(TXT_CAPTION,
      "Eles não veem o que você assiste, o que você salvou nem o que você recomendou. Nada disso sai desta TV.",
      214, 218, 228, x, y, SP_INTERNO, 30.0f, a * 0.95f, 3) + 18.0f;
  y += txt_bloco(TXT_CAPTION,
      "Se você recusar, continua recebendo e enviando recomendações do mesmo jeito. Você só não aparece na lista de ninguém.",
      190, 194, 204, x, y, SP_INTERNO, 30.0f, a * 0.9f, 3) + 18.0f;
  txt_bloco(TXT_CAPTION,
      "Dá para mudar essa resposta quando quiser, no fim desta aba.",
      160, 164, 175, x, y, SP_INTERNO, 30.0f, a * 0.85f, 2);
}

// Uma linha-botao compacta: pilula que inverte no foco, com um subtitulo
// opcional. A altura e menor que a de um card porque isto e uma acao da lista,
// nao conteudo para consumir a tela.
// Acao compacta da lista. A caixa neutra permanece visivel em repouso e recebe
// o mesmo foco tonal dos cartoes: sem contorno fino e sem glow, que em 1080p
// viravam um aro luminoso ao redor de uma acao pequena. `sub` e mantido porque
// a assinatura atende os dois antigos chamadores.
static void desenhaBotaoLinha(int i, float dx, float y, float alt, float a,
                              const char *titulo, const char *sub,
                              const char *icone, int primario) {
  float f = (i >= 0 && i < SP_MAX) ? animFoco[i] : 0.0f;
  float v = focoTexto(f), tinta = ajustes_acento_tinta(NULL, NULL, NULL);
  int tintaFoco = ajustes_tinta_foco();
  float h = primario ? BOTAO_H_PRIMARIO : BOTAO_H_SECUNDARIO;
  float w = botao_largura(titulo, icone, primario);
  GfxRect r = { SP_X + dx + SP_PAD, y + (alt - h) * 0.5f, w, h };
  float grupo, x0;
  TxtLinha repouso, foco;
  (void)sub;
  if (r.w > SP_INTERNO) r.w = SP_INTERNO;
  superficieItem(r, 0.5f, f, a);
  repouso = txt_linha(TXT_DET_BOTAO, titulo, 220, 220, 224, 255);
  foco = txt_linha(TXT_DET_BOTAO, titulo, tintaFoco, tintaFoco, tintaFoco, 255);
  grupo = (float)repouso.w + ((icone && icone[0]) ? BOTAO_ICONE + BOTAO_ICONE_GAP : 0.0f);
  x0 = r.x + (r.w - grupo) * 0.5f;
  if (icone && icone[0]) {
    float ic = anim_mistura(220.0f / 255.0f, tinta, v);
    GfxRect ir = { x0, r.y + (r.h - BOTAO_ICONE) * 0.5f,
                   BOTAO_ICONE, BOTAO_ICONE };
    gfx_icone(ir, icone, ic, ic, ic, a);
    x0 += BOTAO_ICONE + BOTAO_ICONE_GAP;
  }
  txt_foco_transicao(repouso, foco, x0,
                     r.y + (r.h - (float)repouso.h) * 0.5f, v, a);
}

// O INTERRUPTOR DE "APARECER PARA OUTRAS PESSOAS", e por que ele deixou de ser
// mais uma desenhaBotaoLinha.
//
// O DEFEITO: ele era a MESMA pilula, do MESMO tamanho, com o MESMO foco
// invertido de "Adicionar um amigo" e das duas respostas do consentimento. Mas
// "Adicionar um amigo" e uma ACAO — o OK abre um teclado e alguma coisa
// acontece — e isto aqui e um ESTADO que fica, o interruptor de privacidade da
// aba inteira. Duas especies de coisa com a mesma silhueta significa que, do
// sofa, nao da para saber se o OK vai FAZER ou vai MUDAR. Num controle de
// privacidade esse e o pior lugar possivel para ficar ambiguo.
//
// O QUE FOI REJEITADO, e por que:
//
// 1. A CONVENCAO DE AJUSTES — rotulo a esquerda e o valor em texto a direita
//    ("Ligado"/"Desligado", V_LIGA em ajustes.c:135). Rejeitada por tres
//    razoes, e a primeira e a que decide: la o valor a direita e uma COLUNA —
//    TODA linha da tela tem uma, e e a coluna que faz uma palavra ser lida como
//    valor. Aqui seria uma palavra solta na margem direita de uma lista de
//    posteres e de rostos, sem nenhuma outra na mesma prumada; ela leria como
//    um pedaco do subtitulo. Segunda: em Ajustes o OK ENTRA EM EDICAO e sao
//    ESQUERDA/DIREITA que trocam o valor (o bloco `emEdicao` em ajustes.c:2051).
//    Aqui o OK inverte na hora. Vestir a roupa de Ajustes com outro gesto e
//    defeito pior que o que se esta corrigindo. Terceira: "Ligado" nao diz o
//    que esta ligado.
// 2. TRILHA SEMPRE EM COR DE ESTADO, como num telefone. A trilha desligada
//    fica cinza neutro e a ligada usa o accent; a bola branca e a posicao
//    mantem a leitura mesmo quando o foco altera a luminancia da linha.
// 3. ESQUERDA = desligar, DIREITA = ligar. Rejeitada: ESQUERDA ja significa
//    "sair do painel" nesta camada (ver spainel_evento), gesto herdado de
//    perfil.c. Um interruptor em que um lado inverte e o outro fecha a tela
//    inteira e pior que interruptor sem lados.
// 4. UMA PALAVRA AO LADO DA BOLA ("Sim"/"Nao"). Rejeitada: se o controle diz o
//    estado, a palavra e a segunda coisa dizendo a mesma coisa — e duas coisas
//    dizendo o mesmo e o que da cara de formulario a uma linha.
//
// COMO O ESTADO E LIDO: a POSICAO da bola (48 px de percurso, ~35' de arco a
// 3 m) e a diferenca entre a trilha neutra e a trilha accent. A bola permanece
// branca para que o foco nao troque o significado da cor.
//
// QUAL LADO E O SEGURO, sem sermao. O padrao — e a resposta que o produto
// defende — e NAO aparecer, e esse e o lado da trilha VAZIA. Ligar acende
// alguma coisa; desligado nao ha nada aceso. A assimetria esta na fisica do
// interruptor, e nao num aviso, num alerta ou numa cor de perigo: o dono ja
// recusou copy moralista neste app e um interruptor que repreende e a mesma
// coisa desenhada.
static void desenhaAparecer(int i, float dx, float y, float alt, float a) {
  GfxRect r = { SP_X + dx + SP_PAD, y, SP_INTERNO, alt };
  GfxRect trilho, bola;
  float f = (i >= 0 && i < SP_MAX) ? animFoco[i] : 0.0f;
  float ar, ag, ab, ti = ajustes_acento_tinta(&ar, &ag, &ab);
  int esc = f >= 0.5f;
  int c1 = esc ? ajustes_tinta_foco() : 240;
  int c2 = esc ? ajustes_tinta_foco2() : 168;
  // Clamp de seguranca: animSw nasce em -1 e so assenta na primeira
  // atualizacao. Um quadro desenhado antes dela (o painel abre e desenha no
  // mesmo quadro) deslocaria a bola para fora da capsula.
  float lig = animSw < 0.0f ? 0.0f : anim_clamp(animSw, 0.0f, 1.0f);
  float largTexto = SP_INTERNO - 64.0f - SPS_SW_W - SPS_SW_GAP;
  const char *sub = recomenda_aparecer() == REC_APARECER_SIM
      // O SUBTITULO PAROU DE REPETIR O ESTADO. Ele dizia "Sim, voce aparece
      // nas sugestoes de quem te conhece" / "Nao, voce nao aparece na lista de
      // ninguem", que era a unica coisa na linha dizendo ligado ou desligado —
      // agora quem diz isso e o interruptor. Sobrou para o subtitulo o que o
      // interruptor NAO consegue dizer: ligado, QUEM passa a te achar; e
      // desligado, o que voce NAO perde por recusar (nada) — que e a mesma
      // promessa que a pergunta de primeira entrada ja faz, nas mesmas
      // palavras, e a duvida real de quem esta com o dedo em cima.
      //
      // AS DUAS TEM 36 CARACTERES, e isso foi medido e nao estimado: a coluna
      // de texto tem 520 px (688 - 32 - 32 - 80 de trilha - 24 de vao) e o
      // TXT_CAPTION de 22 px gasta ~10,8 px por caractere aqui. "Voce continua
      // recebendo e enviando recomendacoes" pedia ~511 px e saiu da PRIMEIRA
      // captura como "Voce continua recebendo e enviando…", com a palavra que
      // importa cortada fora. "Trocando" diz os dois sentidos numa palavra e
      // guarda o substantivo.
      ? "Quem te conhece te acha nas sugestões"
      : "Você continua trocando recomendações";

  superficieItem(r, 14.0f / alt, f, a);

  { TxtLinha t = txt_linha_corta(TXT_CALLOUT, "Aparecer para outras pessoas",
                                 c1, c1, c1, 255, largTexto);
    TxtLinha s = txt_linha_corta(TXT_CAPTION, sub, c2, c2 + 4, c2 + 14, 255,
                                 largTexto);
    float h = t.h + 8.0f + s.h;
    txt_desenhar_alpha(t, r.x + 32.0f, y + (alt - h) * 0.5f, a);
    txt_desenhar_alpha(s, r.x + 32.0f, y + (alt - h) * 0.5f + t.h + 8.0f,
                       a * 0.95f); }

  // A TRILHA. Raio 0,5 numa caixa 80x40: o teto 0,5*w/h vale 1,0, entao os
  // 0,5 passam inteiros e as pontas saem em semicirculo de 20 px. Capsula de
  // verdade, e nao "quase".
  //
  // OS 32 px DE RECUO SAO OS MESMOS DO TEXTO. Sem eles a capsula encostava na
  // borda da pilula — apareceu na primeira captura e parecia a linha cortada.
  trilho.x = r.x + SP_INTERNO - 32.0f - SPS_SW_W;
  trilho.y = y + (alt - SPS_SW_H) * 0.5f;
  trilho.w = SPS_SW_W;
  trilho.h = SPS_SW_H;
  //
  // DESLIGADA A TRILHA E UM ANEL, e nao uma capsula chapada de alfa baixo. A
  // primeira versao pintava tinta a 16% e MEDI o resultado na captura: a
  // capsula saia em 76 sobre uma linha de 45, ou seja 1,56:1 — e em foco, 209
  // sobre 245, 1,41:1. Nos dois casos a capsula praticamente nao existia, e
  // sem ela a bola clara a esquerda flutua sem dizer que ha um percurso.
  //
  // O ANEL E EXATO, e nao GFX_ANEL: aquele modo sai losangudo (squircle) em
  // diametro pequeno. Duas formas CONCENTRICAS preenchidas dao um anel exato,
  // com 4 px de espessura e centro preservado mesmo depois da compactacao.
  // A regra assentada e simples: trilho cinza quando desligado, accent quando
  // ligado. Durante a animacao a mistura evita um salto de luminancia.
  // O INTERRUPTOR (dono, 21/09/2026): trilho na COR DE REALCE ligado, cinza
  // desligado, bola clara. A regra que faz isso valer em toda combinacao e
  // "a bola contrasta com o trilho e o trilho com a linha":
  //   linha em repouso  desligado: trilho branco a 30 %, bola branca
  //                     ligado:    trilho realce, bola na TINTA do realce
  //                                (branca sobre rosa, escura sobre branco)
  //   linha em foco     desligado: trilho tinta a 25 %, bola tinta
  //   (superficie=realce) ligado:  trilho tinta cheia, bola na cor de realce
  // Sem isso, realce sobre realce sumia na linha em foco e bola branca sumia
  // sobre trilho branco no tema padrao.
  { float tr, tg, tb, ta;
    if (esc) { tr = tg = tb = ti; ta = anim_mistura(0.25f, 1.0f, lig); }
    else { tr = anim_mistura(0.30f, ar, lig); tg = anim_mistura(0.30f, ag, lig);
           tb = anim_mistura(0.30f, ab, lig); ta = 1.0f; }
    gfx_cor(trilho, 0.5f, tr, tg, tb, ta * a); }

  // A BOLA. Quadrada com raio 0,5 = circulo EXATO pelo SDF (com asp = 1 a
  // funcao vira length(p) - 0,5). Nao e GFX_ANEL: aquele sai losangudo em
  // diametro pequeno, e aqui nem ha anel — sao dois discos preenchidos, que e
  // a saida que gfx.h ja recomenda.
  bola.w = SPS_SW_BOLA;
  bola.h = SPS_SW_BOLA;
  bola.x = trilho.x + SPS_SW_PAD +
           (SPS_SW_W - SPS_SW_PAD * 2.0f - SPS_SW_BOLA) * lig;
  bola.y = trilho.y + SPS_SW_PAD;
  // A bola e sempre branca; a sombra curta conserva a leitura sobre a linha
  // clara quando o foco esta ativo.
  { GfxRect sombra = { bola.x - 2.0f, bola.y - 2.0f,
                       bola.w + 4.0f, bola.h + 4.0f };
    float br, bg, bb;
    // Em DEGRAU no meio do percurso (um interruptor de verdade vira, nao
    // esmaece): interpolada, a bola passaria pela cor do trilho e sumiria.
    if (esc) { if (lig > 0.5f) { br = ar; bg = ag; bb = ab; } else br = bg = bb = ti; }
    else     { if (lig > 0.5f) br = bg = bb = ti; else br = bg = bb = 0.96f; }
    gfx_cor(sombra, 0.5f, 0.0f, 0.0f, 0.0f, 0.18f * a);
    gfx_cor(bola, 0.5f, br, bg, bb, a); }
}

// "COMO VOCE APARECE" e "QUEM VE O QUE VOCE ASSISTE": rotulo e o valor de
// agora embaixo, na superficie das linhas. OK abre o teclado / reabre a
// pergunta.
static void desenhaAjusteSocial(int i, float dx, float y, float alt, float a) {
  GfxRect r = { SP_X + dx + SP_PAD, y, SP_INTERNO, alt };
  float f = (i >= 0 && i < SP_MAX) ? animFoco[i] : 0.0f, v = focoTexto(f);
  int tf = ajustes_tinta_foco(), tf2 = ajustes_tinta_foco2();
  const char *titulo = "", *valor = "";
  char buf[96];
#if SP_V2
  if (social[i].tipo == SPS_NOME) {
    titulo = "Como você aparece";
    snprintf(buf, sizeof buf, "%s", recomenda_meu_nome()[0] ? recomenda_meu_nome()
                                                            : i18n("Nome do perfil"));
    valor = buf;
  } else {
    int n = recomenda_alcance();
    titulo = "Quem vê o que você assiste?";
    valor = n == REC_ALCANCE_AMIGOS ? i18n("Só meus amigos")
          : n == REC_ALCANCE_AMIGOS2 ? i18n("Amigos e amigos deles") : i18n("Ninguém");
  }
#else
  (void)buf;
#endif
  superficieItem(r, 14.0f / alt, f, a);
  { TxtLinha t = txt_linha_corta(TXT_CALLOUT, titulo, 240, 240, 240, 255, SP_INTERNO - 64.0f);
    TxtLinha tF = txt_linha_corta(TXT_CALLOUT, titulo, tf, tf, tf, 255, SP_INTERNO - 64.0f);
    TxtLinha s2 = txt_linha_corta(TXT_CAPTION, valor, 168, 172, 182, 255, SP_INTERNO - 64.0f);
    TxtLinha sF = txt_linha_corta(TXT_CAPTION, valor, tf2, tf2, tf2, 255, SP_INTERNO - 64.0f);
    float h = t.h + 8.0f + s2.h;
    txt_foco_transicao(t, tF, r.x + 32.0f, y + (alt - h) * 0.5f, v, a);
    txt_foco_transicao(s2, sF, r.x + 32.0f, y + (alt - h) * 0.5f + t.h + 8.0f, v, a * 0.95f); }
}

// Uma sugestao: a cara, o nome, POR ONDE ela chegou, e a pilula que diz o que
// o OK faz.
//
// A LINHA DA ORIGEM NAO E ENFEITE — e ela que separa "alguem que voce talvez
// conheca" de "um estranho que o app resolveu mostrar". Sem "Segue no Trakt" ou
// "Amigo de Gustavo", a resposta honesta a "quem e essa pessoa?" seria "nao
// sei", e a de quem esta olhando seria recusar.
static void desenhaSugLinha(int i, int idx, float dx, float y, float a) {
  const RecSugestao *s = &sugs[idx];
  float f = (i >= 0 && i < SP_MAX) ? animFoco[i] : 0.0f;
  float px = SP_X + dx + SP_PAD;
  char origem[128];
  int esc = f > 0.5f;

  { GfxRect p = { px - 12.0f, y - SP_FOCO_PADY, SP_INTERNO + 24.0f,
                  SPS_H_SUG + SP_FOCO_PADY * 2.0f };
    superficieItem(p, 14.0f / p.h, f, a); }
  { GfxRect av = { px, y + (SPS_H_SUG - SPS_SUG_AV) * 0.5f,
                   SPS_SUG_AV, SPS_SUG_AV };
    rec_avatar(av, s->avatar, s->nome, s->id, a); }
  rec_sugestao_origem(origem, sizeof origem, s);
  { float tx = px + SPS_SUG_AV + SPS_SUG_GAP;
    // A PILULA DA ACAO E MEDIDA ANTES DO NOME, e o nome e cortado para caber ao
    // lado dela: sem isso, "Carolina Menezes" passava por baixo de "Adicionar"
    // e as duas ficavam ilegiveis na captura.
    int tinta = esc ? ajustes_tinta_foco() : 222;
    TxtLinha acao = txt_linha(TXT_CAPTION, "Adicionar", tinta, tinta, tinta, 255);
    float pw = acao.w + SPS_SUG_PADX * 2.0f;
    float larg = SP_INTERNO - (SPS_SUG_AV + SPS_SUG_GAP) - pw - 20.0f;
    int c1 = esc ? ajustes_tinta_foco() : 245;
    int c2 = esc ? ajustes_tinta_foco2() : 168;
    { TxtLinha t = txt_linha_corta(TXT_CALLOUT, s->nome, c1, c1, c1, 255,
                                   larg);
      txt_desenhar_alpha(t, tx, y + 26.0f, a); }
    { TxtLinha t = txt_linha_corta(TXT_CAPTION, origem, c2, c2 + 4, c2 + 14,
                                   255, larg);
      txt_desenhar_alpha(t, tx, y + 62.0f, a * 0.95f); }
    // A acao e um botao quieto opaco, sem contorno; fica legivel sobre a linha
    // neutra e tambem sobre o accent solido quando o cartao recebe foco.
    { GfxRect p = { px + SP_INTERNO - pw, y + (SPS_H_SUG - 36.0f) * 0.5f,
                    pw, 36.0f };
      if (ajustes_vidro()) gfx_cor(p, 0.5f, 1, 1, 1, 0.10f * a);   /* selo de vidro */
  else gfx_cor(p, 0.5f, .095f, .102f, .116f, a);
      txt_desenhar_alpha(acao, p.x + SPS_SUG_PADX,
                         p.y + (36.0f - acao.h) * 0.5f, a); } }
}

// Linha de um AMIGO JA ADICIONADO: foto (ou inicial), nome e de onde veio
// (Trakt ou codigo). Mesma caixa e mesmo foco da sugestao, sem o botao —
// nao ha acao aqui alem de reconhecer quem esta na lista.
static void desenhaAmigoLinha(int i, int idx, float dx, float y, float a, Uint32 agora) {
  const RecContato *c = &ctts[idx];
  float f = (i >= 0 && i < SP_MAX) ? animFoco[i] : 0.0f;
  float v = focoTexto(f);   // cor do texto e dos selos no foco (vidro: nao inverte)
  float px = SP_X + dx + SP_PAD, alt = socialAlt(i);
  const SvAmigo *am = (cttSv[idx] >= 0) ? socialvis_amigo(cttSv[idx]) : NULL;
  { GfxRect p = { px - 12.0f, y - SP_FOCO_PADY, SP_INTERNO + 24.0f,
                  alt + SP_FOCO_PADY * 2.0f };
    superficieItem(p, 14.0f / p.h, f, a); }
  { GfxRect av = { px, y + (SPS_H_AMIGO - SPS_AMIGO_AV) * 0.5f,
                   SPS_AMIGO_AV, SPS_AMIGO_AV };
    if (am) svd_rosto(av, am, 0.0f, a, agora);
    else rec_avatar(av, c->avatar, c->nome, c->id, a); }
  { float tx = px + SPS_AMIGO_AV + SPS_AMIGO_GAP;
    float larg = SP_INTERNO - (SPS_AMIGO_AV + SPS_AMIGO_GAP) - 20.0f;
    int tf = ajustes_tinta_foco(), tf2 = ajustes_tinta_foco2();
    int vivo = am && am->agora && am->nTit > 0;
    char linha2[260];
    // O QUE ELE ESTA FAZENDO (tela A, 02/10/2026), do modelo do social: "●
    // Agora · The Bear · T3E4 · faltam 12 min" com a barra, ou "Project Hail
    // Mary · Terminou e gostou · ontem". Sem atividade, como ele chegou — e so
    // isso; a FONTE (Trakt) nao e escrita aqui, fica no perfil.
    if (am && am->nTit > 0) {
      char st[160];
      const SvEvento *e = &am->tit[0];
      socialvis_status(e, st, sizeof st);
      if (vivo) {
        // "Agora · T3E4 · faltam..." com o titulo depois do "Agora".
        const char *resto = strstr(st, " \xc2\xb7 ");
        snprintf(linha2, sizeof linha2, "%s \xc2\xb7 %s%s", i18n("Agora"), e->titulo,
                 resto ? resto : "");
      } else snprintf(linha2, sizeof linha2, "%s \xc2\xb7 %s", e->titulo, st);
    } else snprintf(linha2, sizeof linha2, "%s", i18n("Ainda sem atividade"));
    { TxtLinha repouso = txt_linha_corta(TXT_BODY, c->nome, 245, 245, 245, 255, larg - 30.0f);
      TxtLinha foco = txt_linha_corta(TXT_BODY, c->nome, tf, tf, tf, 255, larg - 30.0f);
      float sx = tx + (vivo ? 24.0f : 0.0f);
      TxtLinha subRepouso = vivo ? txt_linha_corta(TXT_CAPTION, linha2, 255, 196, 196, 255, larg - 24.0f)
                                 : txt_linha_corta(TXT_CAPTION, linha2, 168, 172, 182, 255, larg);
      TxtLinha subFoco = txt_linha_corta(TXT_CAPTION, linha2, tf2, tf2, tf2, 255,
                                         vivo ? larg - 24.0f : larg);
      int barra = vivo && am->tit[0].pct >= 0;
      float bloco = (float)repouso.h + 6.0f + (float)subRepouso.h + (barra ? 14.0f : 0.0f);
      float ty = y + (SPS_H_AMIGO - bloco) * 0.5f;
      txt_foco_transicao(repouso, foco, tx, ty, v, a);
      if (vivo) svd_ponto_vivo(tx + 8.0f, ty + repouso.h + 6.0f + subRepouso.h * 0.5f, 14.0f, 0.0f, a, agora);
      txt_foco_transicao(subRepouso, subFoco, sx, ty + repouso.h + 6.0f, v, a * 0.95f);
      if (barra)
        svd_barra((GfxRect){ tx, ty + repouso.h + 6.0f + subRepouso.h + 9.0f, larg * 0.6f, 5.0f },
                  am->tit[0].pct, a); }
    // A CADEIA DO QUE EU MANDEI: "Voce mandou X › viu › gostou".
    if (cttCadeia[idx]) {
      SvEnviada m;
      if (socialvis_ultima_enviada(c->id, &m)) {
        char t1[220];
        int esc = v > 0.5f;
        float cx = tx, cy = y + SPS_H_AMIGO - 2.0f;
        snprintf(t1, sizeof t1, i18n("Você mandou %s"), m.titulo);
        cx += svd_chip(cx, cy, t1, 0, esc, a);
        cx += svd_seta(cx, cy, esc, a);
        if (m.estado == SV_REC_VIU) {
          cx += svd_chip(cx, cy, "viu", 1, esc, a);
          if (m.reacao == SV_REAC_GOSTOU) {
            cx += svd_seta(cx, cy, esc, a);
            svd_chip(cx, cy, "gostou", 2, esc, a);
          }
        } else svd_chip(cx, cy, m.estado == SV_REC_ABRIU ? "abriu" : "ainda não viu", 0, esc, a);
      }
    } }
}

// A ATIVIDADE VAZIA: diz o que vai aparecer e de onde, sem prometer dado que
// ainda nao existe.
static void desenhaAtividadeVazia(float dx, float y0, float a) {
  float x = SP_X + dx + SP_PAD, y = y0 + 8.0f;
  TxtLinha t = txt_linha(TXT_CALLOUT, "Nada por aqui ainda", 240, 242, 248, 255);
  txt_desenhar_alpha(t, x, y, a * 0.96f);
  y += (float)t.h + 14.0f;
  txt_bloco(TXT_CAPTION,
      "Quando seus amigos começarem, terminarem ou gostarem de um título, aparece aqui.",
      190, 194, 204, x, y, SP_INTERNO, 30.0f, a * 0.9f, 3);
}

// UMA LINHA DO FEED (tela B): o rosto, o cartaz e tres linhas — "Marina esta
// vendo", "The Bear · T3E4" e quando. O nome e o verbo sao DUAS linhas de
// texto lado a lado (o verbo e chave de i18n inteira; a frase com o nome
// dentro nunca seria). Mesma superficie e mesmo foco das outras linhas.
static void desenhaAtvLinha(int i, float dx, float y, float a, Uint32 agora) {
  const SvEvento *e = socialvis_evento(i);
  float f = (i >= 0 && i < SP_MAX) ? animFoco[i] : 0.0f, v = focoTexto(f);
  float px = SP_X + dx + SP_PAD, tx, larg;
  int tf = ajustes_tinta_foco(), tf2 = ajustes_tinta_foco2();
  char linha[220], ep[24], q[48];
  if (!e) return;
  superficieItem((GfxRect){ px - 12.0f, y - SP_FOCO_PADY, SP_INTERNO + 24.0f,
                            SPA_H + SP_FOCO_PADY * 2.0f }, 14.0f / (SPA_H + 12.0f), f, a);
  { SvAmigo am;
    memset(&am, 0, sizeof am);
    snprintf(am.id, sizeof am.id, "%s", e->pessoaId);
    snprintf(am.nome, sizeof am.nome, "%s", e->pessoaNome);
    snprintf(am.avatar, sizeof am.avatar, "%s", e->pessoaAvatar);
    am.agora = e->acao == SV_AGORA;
    am.novo = 0;
    svd_rosto((GfxRect){ px + 4.0f, y + 8.0f, SPA_AV, SPA_AV }, &am, 0.0f, a, agora); }
  svd_poster((GfxRect){ px + SPA_AV + 22.0f, y + (SPA_H - SPA_PH) * 0.5f, SPA_PW, SPA_PH },
             e->poster[0] ? e->poster : e->arte, 0.08f, a);
  tx = px + SPA_AV + 22.0f + SPA_PW + 20.0f;
  larg = SP_INTERNO - (tx - px) - 8.0f;
  { TxtLinha nr = txt_linha_corta(TXT_CALLOUT, e->pessoaNome, 245, 245, 247, 255, larg * 0.5f);
    TxtLinha nf = txt_linha_corta(TXT_CALLOUT, e->pessoaNome, tf, tf, tf, 255, larg * 0.5f);
    const char *verbo = socialvis_verbo(e);
    TxtLinha vr = txt_linha_corta(TXT_CALLOUT, verbo, 200, 198, 210, 255, larg - (float)nr.w - 10.0f);
    TxtLinha vf = txt_linha_corta(TXT_CALLOUT, verbo, tf2, tf2, tf2, 255, larg - (float)nr.w - 10.0f);
    txt_foco_transicao(nr, nf, tx, y + 8.0f, v, a);
    txt_foco_transicao(vr, vf, tx + (float)nr.w + 10.0f, y + 8.0f, v, a); }
  socialvis_ep(e, ep, sizeof ep);
  if (ep[0]) snprintf(linha, sizeof linha, "%s \xc2\xb7 %s", e->titulo, ep);
  else snprintf(linha, sizeof linha, "%s", e->titulo);
  { TxtLinha r = txt_linha_corta(TXT_CAPTION, linha, 228, 226, 236, 255, larg);
    TxtLinha fo = txt_linha_corta(TXT_CAPTION, linha, tf, tf, tf, 255, larg);
    txt_foco_transicao(r, fo, tx, y + 48.0f, v, a); }
  // QUANDO, e o que o cartao da home tambem diria: "faltam 12 min", "aos 18 %".
  socialvis_quando(e->quando, q, sizeof q);
  linha[0] = 0;
  if (e->acao == SV_AGORA && e->restanteMin > 0)
    snprintf(linha, sizeof linha, i18n("faltam %d min"), e->restanteMin);
  else if (e->acao == SV_ABANDONO && e->pct >= 0)
    snprintf(linha, sizeof linha, i18n("Parou aos %d%%"), e->pct);
  if (q[0]) {
    size_t k = strlen(linha);
    snprintf(linha + k, sizeof linha - k, "%s%s", k ? " \xc2\xb7 " : "", q);
  }
  if (linha[0]) {
    TxtLinha r = txt_linha_corta(TXT_MINI, linha, 160, 158, 171, 255, larg);
    TxtLinha fo = txt_linha_corta(TXT_MINI, linha, tf2, tf2, tf2, 255, larg);
    txt_foco_transicao(r, fo, tx, y + 82.0f, v, a);
  }
}

static void desenhaVazio(float dx, float a) {
  float cx = SP_X + dx + SP_W * 0.5f;
  // DESTINO SIMKL SEM VINCULO (issue #110): a lista vazia muda seria lida como
  // "o Plan to Watch esta vazio". O que falta e o vinculo, e e isso que sai.
  const char *semSimkl = simkl_aviso_sem_vinculo(ajustes_salvos_no_simkl());
  TxtLinha t1 = txt_linha(TXT_CALLOUT, semSimkl ? semSimkl : "Nada salvo por enquanto",
                          240, 242, 248, 255);
  TxtLinha t2 = txt_linha_corta(TXT_CAPTION,
      semSimkl ? "O + salva no Plan to Watch do Simkl, e ele ainda não está vinculado nesta TV."
               : "Aperte + em um filme ou série e ele aparece aqui.",
      168, 172, 182, 255, SP_INTERNO);
  gfx_icone((GfxRect){ cx - 30.0f, listaTopo() + 140.0f, 60.0f, 60.0f },
            "mais", 0.55f, 0.57f, 0.62f, a);
  txt_desenhar_alpha(t1, cx - t1.w * 0.5f, listaTopo() + 232.0f, a * 0.96f);
  txt_desenhar_alpha(t2, cx - t2.w * 0.5f, listaTopo() + 278.0f, a * 0.85f);
}

// 1 = o veu ja esta no fundo (a copia parada foi pintada com ele).
static int veuNoFundo;

// O veu usa a rampa CRUA e o painel a suavizada, pelo mesmo motivo do menu
// lateral: a medida da referencia para o escurecimento e uma reta, e um bloco
// deste tamanho parando de vez no fim do percurso le como corte.
static void veuInteiro(void) {
  gfx_cor((GfxRect){ 0, 0, NV_TELA_W, NV_TELA_H }, 0.0f, 0, 0, 0, SP_VEU * entrada);
}

// O FUNDO PARADO. Com o painel inteiro na tela (a entrada terminou), o que
// esta atras dele — a home escurecida pelo veu — nao muda de um quadro para o
// outro: nao ha foco la, o trailer do destaque fecha com o painel aberto e a
// troca de arte do destaque so acontece no desenho da home. Entao ele e pintado
// UMA vez no FBO do snapshot (gfx_snap), com o veu por cima, e dali em diante o
// quadro e a copia (uma textura de tela cheia, sem SDF) e o painel.
//
// O PORQUE, MEDIDO na C9 do dono (24/09/2026): home sozinha a 60 fps; com o
// painel aberto 52-54 fps, `clr` de 21-30 ms nos piores quadros (o glClear
// esperando a GPU terminar o anterior) e fill=4,80x. A home inteira, mais um
// veu de tela cheia, mais o painel, eram redesenhados a cada quadro para
// mostrar uma imagem parada a 42 % de brilho — e gfx.c ja registrava que duas
// camadas de tela cheia derrubam a Mali-G71 para ~40 fps.
//
// `podeParar` e de quem chama (app.c: so na home, sem menu, detalhe ou outra
// camada no meio). `rev` e o que invalida a copia — app.c passa cat_revisao,
// que sobe quando as fileiras de baixo mudam. Sem FBO, ou com o painel
// entrando ou saindo, `fundo` e desenhado direto, como sempre foi.
// A COPIA E REFEITA TRES VEZES NOS PRIMEIROS SEGUNDOS (0,4 s, 1,5 s e 4 s
// depois da primeira) e depois fica. Arte da home que ainda estava subindo
// quando o painel abriu — um cartaz, o fundo do destaque logo depois do
// arranque — ficaria congelada como esqueleto; tres pinturas a mais custam
// tres quadros no ritmo de antes, e so na abertura.
static const Uint32 SP_FUNDO_REFAZ_MS[] = { 400, 1500, 4000 };
void spainel_fundo(int podeParar, unsigned rev, void (*fundo)(void *), void *ctx) {
  static int pronto, refeitas;
  static unsigned revPronto;
  static Uint32 desde;
  int parado = podeParar && aberto && entrada >= 0.999f && gfx_snap_ok();
  if (!parado) { pronto = 0; refeitas = 0; }
  else if (rev != revPronto) pronto = 0;
  else if (pronto && refeitas < (int)(sizeof SP_FUNDO_REFAZ_MS / sizeof *SP_FUNDO_REFAZ_MS) &&
           SDL_GetTicks() - desde >= SP_FUNDO_REFAZ_MS[refeitas]) {
    pronto = 0;
    refeitas++;
  }
  veuNoFundo = parado;
  if (parado && pronto) { gfx_snap_desenhar(); return; }
  if (parado) {
    gfx_snap_comecar();
    gfx_sem_recorte();
    glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
  }
  if (fundo) fundo(ctx);
  if (parado) {
    veuInteiro();
    gfx_sem_recorte();
    gfx_snap_terminar();
    gfx_snap_desenhar();
    if (!refeitas) desde = SDL_GetTicks();
    pronto = 1;
    revPronto = rev;
    fundosPintados++;
  }
}

// O RECORTE DO MORPH. Nascendo da ilha, o painel e um retangulo que cresce da
// pilula ate SP_X/SP_W; o conteudo fica no lugar FINAL e aparece por dentro do
// retangulo, sem ser escalado (nenhuma textura nova, nenhum FBO: o mesmo
// desenho de sempre com um recorte). Todo recorte de dentro do painel passa
// por aqui para nao vazar do retangulo enquanto ele cresce.
static int morfOn;
static GfxRect morfR;
static void spRecorte(float x, float y, float w, float h) {
  if (morfOn) {
    float x1 = x + w, y1 = y + h, mx1 = morfR.x + morfR.w, my1 = morfR.y + morfR.h;
    if (x < morfR.x) x = morfR.x;
    if (y < morfR.y) y = morfR.y;
    if (x1 > mx1) x1 = mx1;
    if (y1 > my1) y1 = my1;
    w = x1 - x; h = y1 - y;
  }
  gfx_recorte(x, y, w, h);
}
#define gfx_recorte spRecorte

// Saida com repique curto (o "pulo" da pilula da ilha), so na forma.
static float voltaSuave(float t) {
  const float c1 = 1.25f, c3 = c1 + 1.0f;
  float u = t - 1.0f;
  return 1.0f + c3 * u * u * u + c1 * u * u;
}

static void desenharPainel(Uint32 agora);
void spainel_desenhar(Uint32 agora) {
  desenharPainel(agora);
  if (entrada < 0.002f) return;
  // POR CIMA DE TUDO DO PAINEL: a escolha aberta e o teclado de nome.
  desenhaPop(anim_suave(entrada));
  if (tecladoPara) teclado_desenhar(agora);
}

static void desenharPainel(Uint32 agora) {
  float a = anim_suave(entrada), x, y;
  int i;
  char buf[160];
  GfxRect forma = { SP_X, SP_Y, SP_W, SP_H };
  float raioForma = 28.0f / SP_W;
  morfOn = 0;
  if (entrada < 0.002f) return;

  // Entra deslizando da BORDA DIREITA. `x` e o deslocamento: em a=0 o painel
  // esta inteiro fora da tela.
  x = (1.0f - a) * (NV_TELA_W - SP_X);
  // ...ou NASCE DA ILHA: o retangulo vai da pilula (origem) ate o painel, o
  // raio em pixels de meia altura da pilula ate o do painel, e o conteudo
  // entra na segunda metade. Animacoes reduzidas: entrada ja e 0 ou 1 (anim.h).
  if (deIlha) {
    GfxRect o = (aberto || !destinoOk) ? origem : destino;
    float e = aberto ? voltaSuave(entrada) : anim_suave(entrada);
    float rFim = raioForma * SP_H, rIni = o.h * 0.5f, ec = e > 1.0f ? 1.0f : e;
    forma.x = o.x + (SP_X - o.x) * e;  forma.y = o.y + (SP_Y - o.y) * e;
    forma.w = o.w + (SP_W - o.w) * e;  forma.h = o.h + (SP_H - o.h) * e;
    raioForma = (rIni + (rFim - rIni) * ec) / (forma.h > 1.0f ? forma.h : 1.0f);
    x = 0.0f;
    a = (entrada - 0.45f) / 0.55f;
    a = a < 0.0f ? 0.0f : a > 1.0f ? 1.0f : a;
    morfOn = entrada < 0.999f;
    morfR = forma;
  }
  // Com o fundo parado o veu ja esta na copia (spainel_fundo).
  //
  // O VEU CONTINUA INTEIRO, e nao so em volta do painel. Tentei faixas em volta
  // (o painel e 94 % opaco, o veu debaixo dele e quase todo desperdicio): a
  // borda de todo gfx_cor e suavizada em ~0,6 % da ALTURA do retangulo, e numa
  // faixa de tela inteira isso sao ~6 px de meio-veu colados no contorno do
  // painel — um fio claro na comparacao pixel a pixel. Com o fundo parado o
  // veu ja nao custa nada por quadro; o que sobra sao a entrada, a saida e o
  // painel sobre outras telas.
  if (!veuNoFundo) veuInteiro();
  // Painel flutuante escuro e neutro; o veu separa a camada do conteudo sem
  // uma luz decorativa colorida competindo com posters e selos.
  { GfxRect p = { SP_X + x, SP_Y, SP_W, SP_H };
    float as = a;
    if (deIlha) { p = forma; as = entrada > 0.08f ? 1.0f : entrada / 0.08f; }
    // Vidro: folha translucida sem contorno, como o menu e Fontes (dono, 30/09).
    if (ajustes_vidro()) gfx_vidro_folha(p, raioForma, as);
    else gfx_cor(p, raioForma, 0.055f, 0.058f, 0.068f, 0.94f * as);
  }

  // Tudo daqui para baixo fica preso ao painel: sem o recorte, a lista rolada
  // desenha por cima do cabecalho e por baixo da borda inferior.
  gfx_recorte(SP_X + x, SP_Y, SP_W, SP_H);

  // Cabecalho: a linha de resumo em cima e o nome grande embaixo, como na
  // referencia. As PARTES passam por i18n; a juncao, nao (ver metaTexto).
  // recomenda_n() E NAO nRecs: com a pergunta de consentimento na tela a lista
  // local esta vazia de proposito, e escrever "0 recomendações" ao lado de uma
  // aba com o selo em 2 seria o painel se contradizendo em dois centimetros.
  if (aba == SP_ABA_SOCIAL) {
    int n = recomenda_n();
    snprintf(buf, sizeof buf, "%d %s", n,
             i18n(n == 1 ? "recomendação" : "recomendações"));
  }
  else if (aba == SP_ABA_ATIVIDADE) {
    int nv = socialvis_n_ao_vivo();
    if (nv > 0) snprintf(buf, sizeof buf, i18n("assistindo agora: %d"), nv);
    else buf[0] = 0;
  }
  else if (aba == SP_ABA_AVISOS) {
    int n = avisos_lista_n(), nv = avisos_n_novos();
    if (nv > 0) snprintf(buf, sizeof buf, i18n("%d avisos · %d novos"), n, nv);
    else snprintf(buf, sizeof buf, "%d %s", n, i18n(n == 1 ? "aviso" : "avisos"));
  }
  else {
    snprintf(buf, sizeof buf, "%d %s   ·   %d %s", nLinhas,
             i18n(nLinhas == 1 ? "título" : "títulos"),
             nCont, i18n("para retomar"));
  }
  // COM ABAS a contagem vai para a DIREITA da propria linha de abas, e nao
  // numa linha solta acima delas: sozinha la em cima ela lia como um titulo
  // orfao, que foi a primeira coisa que saltou na foto ampliada.
  { // Nunca por cima das abas: o que nao cabe entre a faixa e a borda sai
    // com reticencias, em vez de a contagem colar no selo (foto de 20/09).
    float sobra = SP_W - 2.0f * SP_PAD - (temAbas() ? abasLargura() + 24.0f : 0.0f);
    TxtLinha t = txt_linha(TXT_CAPTION, buf, 160, 164, 175, 255);
    // Com tres abas nao sobra lugar para a contagem na mesma linha: ela
    // SOME em vez de sair cortada ("124 titles · 2 to…" nao diz nada). A
    // informacao continua na propria lista.
    if (temAbas()) {
      if (t.w <= sobra)
        txt_desenhar_alpha(t, SP_X + x + SP_W - SP_PAD - t.w,
                           SP_ABAS_Y + (SP_ABAS_H - t.h) * 0.5f, a * 0.95f);
    }
    else
      txt_desenhar_alpha(t, SP_X + x + SP_PAD, SP_Y + 38.0f, a * 0.95f); }
  if (temAbas()) desenhaAbas(x, a);
  if (temBarra()) desenhaBarra(x, a);
  if (!temAbas()) {
    TxtLinha t = txt_linha(TXT_TITULO2, "Salvos", 246, 247, 252, 255);
    txt_desenhar_alpha(t, SP_X + x + SP_PAD, SP_Y + 74.0f, a);
  }

  if (aba == SP_ABA_ATIVIDADE) {
    gfx_recorte(SP_X + x, listaTopo(), SP_W, SP_LISTA_BASE - listaTopo());
    y = listaTopo() + SP_FOCO_AR - scrollY;
    if (nAtv == 0) desenhaAtividadeVazia(x, y, a);
    for (i = 0; i < nAtv; i++) {
      float cab = atvAntes(i);
      if (cab > 0.0f) {
        if (y + cab >= listaTopo() && y <= SP_LISTA_BASE)
          desenhaSecao(SP_X + x + SP_PAD, y, atvRot[i], a);
        y += cab;
      }
      if (y + SPA_H >= listaTopo() && y <= SP_LISTA_BASE)
        desenhaAtvLinha(i, x, y, a, agora);
      y += SPA_H + SPS_GAP;
    }
    gfx_sem_recorte();
    return;
  }
  if (aba == SP_ABA_AVISOS) {
    gfx_recorte(SP_X + x, listaTopo(), SP_W, SP_LISTA_BASE - listaTopo());
    avisos_lista_desenhar(SP_X + x + SP_PAD, listaTopo() + SP_FOCO_AR - scrollY, SP_INTERNO, a, foco);
    gfx_sem_recorte();
    return;
  }

  if (aba == SP_ABA_SOCIAL) {
    gfx_recorte(SP_X + x, listaTopo(), SP_W, SP_LISTA_BASE - listaTopo());
    y = listaTopo() + SP_FOCO_AR - scrollY;
    // O BLOCO DE TEXTO ROLA COM A LISTA, e nao fica preso no topo: ele explica
    // a lista que vem logo abaixo, e um texto fixo com linhas passando por
    // baixo dele leria como duas telas empilhadas.
    if (consentindo())    desenhaConsentimento(x, y, a);
    else if (perguntandoAlcance()) desenhaAlcancePergunta(x, y, a);
    else if (nRecs == 0)  desenhaSocialVazio(x, y, a);
    y += socialTopo();
    for (i = 0; i < nSocial; i++) {
      float alt = socialAlt(i);
      float cab = socialAntes(i);
      if (cab > 0.0f) {
        // So a secao das sugestoes tem rotulo; o vao do interruptor e mudo.
        if ((social[i].tipo == SPS_SUG || social[i].tipo == SPS_AMIGO) &&
            y + cab >= listaTopo() && y <= SP_LISTA_BASE) {
          desenhaSecao(SP_X + x + SP_PAD, y + cab - SP_SECAO_H,
                       social[i].tipo == SPS_SUG ? "Pessoas que você talvez conheça"
                                                 : "Seus amigos", a);
        }
        // Por pessoa: o nome de quem mandou abre o grupo dele.
        if (social[i].tipo == SPS_REC && y + cab >= listaTopo() && y <= SP_LISTA_BASE)
          desenhaSecao(SP_X + x + SP_PAD, y + cab - SP_SECAO_H,
                       recs[social[i].idx].deNome, a);
        y += cab;
      }
      // Fora da janela nao custa texto nem textura — mesma razao da lista de
      // Salvos logo abaixo.
      if (y + alt >= listaTopo() && y <= SP_LISTA_BASE) {
        switch (social[i].tipo) {
          case SPS_REC: desenhaRecLinha(i, social[i].idx, x, y, a); break;
          case SPS_SUG: desenhaSugLinha(i, social[i].idx, x, y, a); break;
          case SPS_AMIGO: desenhaAmigoLinha(i, social[i].idx, x, y, a, agora); break;
          case SPS_ENCONTRAR: {
            char rot[96];
            int np = recomenda_n_pedidos();
            if (np > 0) snprintf(rot, sizeof rot, i18n("Encontrar pessoas (%d)"), np);
            else snprintf(rot, sizeof rot, "%s", "Encontrar pessoas");
            desenhaBotaoLinha(i, x, y, alt, a, rot, NULL, "menu_search", 0);
            break; }
          case SPS_ADICIONAR:
            // A linha de "Adicionar um amigo" fecha a lista, e nao um botao
            // solto no rodape: ela rola com o resto e recebe foco como qualquer
            // outra.
            desenhaBotaoLinha(i, x, y, alt, a, "Adicionar um amigo", NULL, "mais", 0);
            break;
          case SPS_APARECER:
            // NAO e desenhaBotaoLinha, e a diferenca e o ponto todo desta
            // linha. Ver a nota longa em desenhaAparecer.
            desenhaAparecer(i, x, y, alt, a);
            break;
          case SPS_CONSENT_SIM:
            desenhaBotaoLinha(i, x, y, alt, a, "Sim, pode me mostrar", NULL, NULL, 1);
            break;
          case SPS_ALC_0:
            desenhaBotaoLinha(i, x, y, alt, a, "Ninguém", NULL, NULL, 0);
            break;
          case SPS_ALC_1:
            desenhaBotaoLinha(i, x, y, alt, a, "Só meus amigos", NULL, NULL, 1);
            break;
          case SPS_ALC_2:
            desenhaBotaoLinha(i, x, y, alt, a, "Amigos e amigos deles", NULL, NULL, 0);
            break;
          case SPS_ALCANCE:
          case SPS_NOME:
            desenhaAjusteSocial(i, x, y, alt, a);
            break;
          default:
            desenhaBotaoLinha(i, x, y, alt, a, "Não, não quero aparecer", NULL, NULL, 0);
            break;
        }
      }
      y += alt + SPS_GAP;
    }
    gfx_sem_recorte();
    return;
  }

  if (nLinhas == 0) { desenhaVazio(x, a); gfx_sem_recorte(); return; }

  // A lista rola dentro da propria janela, com um segundo recorte: o cabecalho
  // fica de fora dele e por isso nunca e coberto por um card subindo.
  gfx_recorte(SP_X + x, listaTopo(), SP_W, SP_LISTA_BASE - listaTopo());
  y = listaTopo() + SP_FOCO_AR - scrollY;

  // OS ROTULOS DAS SECOES, onde montarLayout os pos. A categoria vazia leva
  // a dica de como por algo nela, no lugar das celulas que ainda nao tem.
  for (i = 0; i < nSecoes; i++) {
    float sy = y + secoes[i].y;
    float sh = SP_SECAO_H + (secoes[i].vazia ? SP_VAZIA_H : 0.0f);
    if (sy + sh < listaTopo() || sy > SP_LISTA_BASE) continue;
    desenhaSecao(SP_X + x + SP_PAD, sy, secoes[i].rot, a);
    if (secoes[i].vazia) {
      TxtLinha t = txt_linha_corta(TXT_CAPTION,
          "Vazia. Segure OK num título e escolha \xe2\x80\x9cMover para categoria\xe2\x80\x9d.",
          150, 154, 166, 255, SP_INTERNO);
      txt_desenhar_alpha(t, SP_X + x + SP_PAD, sy + SP_SECAO_H + 6.0f, a * 0.9f);
    }
  }
  { int estilo = sorg_estilo();
    for (i = 0; i < nLinhas; i++) {
      float cy = y + linhas[i].ly;
      // Fora da janela nao custa texto nem textura: numa lista de 200 titulos
      // rasterizar as 195 invisiveis estouraria o orcamento de linhas por
      // quadro de text.c e as visiveis sairiam EM BRANCO (ver ctxmenu.c).
      if (cy + linhas[i].lh < listaTopo() || cy > SP_LISTA_BASE) continue;
      if (estilo == SORG_ESTILO_GRADE) desenhaCelulaGrade(i, x, cy, a);
      else if (estilo == SORG_ESTILO_PAISAGEM) desenhaCelulaPaisagem(i, x, cy, a);
      else desenhaLinha(i, x, cy, a);
    } }

  gfx_sem_recorte();
}
