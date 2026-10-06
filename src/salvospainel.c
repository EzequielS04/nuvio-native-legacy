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
#include "cwretido.h"
#include "salvos.h"
#include "salvosorg.h"
#include "teclado.h"
#include "simkl.h"
#include "recomenda.h"
#include "atividade.h"
#include "avisos.h"
#include "agenda.h"
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
#include "botoes.h"
#include "socialvis.h"
#include "svdesenho.h"
#include "recresp.h"
#include "reacao.h"
#define NV_ESCALA_TELA   // o arquivo inteiro mede pela tela virtual (escala.h)
#include "escala.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

// Mesma pegada do painel "Sua atividade" que ele substitui (perfil.c desenhava
// em x=1120, 776x1032): quem ja tinha o gesto na memoria muscular encontra a
// camada no mesmo lugar, so com outro conteudo.
#define SP_X          (NV_TELA_W - 800.0f)   // 1120 em 1080; tela virtual (escala.h)
#define SP_W           776.0f
// A ILHA FLUTUA A NV_FOLHA_MARGEM das tres bordas e com o raio da folha de
// Fontes (streams.c): as duas camadas da direita sao o mesmo objeto, e uma
// colada no topo ao lado da outra solta leria como dois materiais.
#define SP_Y          NV_FOLHA_MARGEM
#define SP_H          (NV_TELA_H - 2.0f * NV_FOLHA_MARGEM)
#define SP_RAIO        NV_FOLHA_RAIO
// A GRAMATICA DO MOCKUP APROVADO ("Glass UI — ilha", tela 3; o CSS esta em
// design/glass-ilha/glass-ilha.html), em px de 1080p e RELATIVA AO TOPO E A
// ESQUERDA DO PAINEL — o mockup poe a ilha a 40 da borda e a folha de Fontes
// real a 24, entao o que se copia e a distancia de dentro, nao a absoluta:
//   cabecalho  40 de ar; o kicker (15 caixa alta) e "Social" (40 bold) a 48 da
//              borda. SEM o disco de fechar do mockup: o dono pediu para tirar
//              o X (03/10); VOLTAR e ESQUERDA na borda fecham.
//   abas       136 do topo, o seletor a 42 da borda: 5 de folga, segmentos de
//              45 (19 semibold, 20 de recuo, contagem 16 em cinza a 9 do rotulo)
//   lista      217 do topo; linhas de 26 a 26 das bordas, raio 22, 18/22 de
//              recuo; secao 22 bold com um fio de 1 px a 8 % embaixo
// Era o painel "parecido": abas no lugar do titulo, rotulos de secao em
// cinza 22, rosto e capa juntos a esquerda (foto do dono lado a lado com o
// mockup, 02/10: "o mockup ta bem mais polido que a build").
#define SP_PAD          48.0f   // do painel ao texto (26 da linha + 22 dela)
#define SP_INTERNO    (SP_W - SP_PAD * 2.0f)
#define SP_LINHA_X      26.0f   // do painel a superficie da linha
#define SP_LINHA_W    (SP_W - SP_LINHA_X * 2.0f)
#define SP_LINHA_PADX   22.0f
#define SP_LINHA_RAIO   22.0f
#define SP_KICK_Y      (SP_Y + 40.0f)
#define SP_TIT_Y       (SP_Y + 62.0f)
// ABAS. Elas so existem quando o servico de recomendacoes foi compilado
// (recomenda_ativo); sem ele o painel e exatamente o que era, sem uma linha a
// mais de cromo para uma funcao que nao existe naquele pacote.
#define SP_ABAS_Y      (SP_Y + 136.0f)
#define SP_ABAS_H        55.0f
#define SP_ABAS_X       42.0f
// AR DO FOCO entre o recorte da lista e a primeira linha. A superficie da linha
// focada comeca no `y` da linha, e a lista comecava exatamente no topo do
// recorte: a primeira linha em foco — e qualquer linha que a rolagem alinhava
// ao topo — saia com a borda de cima reta, sem os cantos (foto do dono, 01/10,
// aba Social com vidro; medido no tests/spainel_foco_shot). O conteudo comeca
// SP_FOCO_AR abaixo do recorte e a rolagem guarda o mesmo ar em cima e embaixo
// da linha focada. O recorte fica 12 px abaixo das abas, e o conteudo nasce
// em 217 do topo do painel, onde o mockup poe a lista.
#define SP_FOCO_AR      14.0f
#define SP_LISTA_Y     (SP_Y + 217.0f - SP_FOCO_AR)
#define SP_LISTA_BASE  (SP_Y + SP_H - 24.0f)
// A LINHA DE SALVOS: cartaz 2:3 de 64x96 com 18 px de recuo em cima e
// embaixo, a mesma altura de ritmo das linhas do feed (capa 52x76 e 114). O
// cartaz de 92x138 de antes fazia a aba Salvos parecer outra tela ao lado da
// Atividade.
#define SP_POSTER_W     76.0f   // 64 antes: a lista com capa +19 % (pedido do dono, 03/10)
#define SP_POSTER_H    114.0f   // 96 antes; 2:3 mantido
#define SP_LINHA_PADY   21.0f   // 18 antes
#define SP_LINHA_SALVO (SP_POSTER_H + SP_LINHA_PADY * 2.0f)
// As linhas se encostam, como no mockup: so uma tem superficie (a focada), e
// o recuo de 18 de cada uma ja e o ar entre os textos.
#define SP_PASSO       SP_LINHA_SALVO
// ROTULO DE SECAO ("Hoje", "Continuar"): 22 bold em branco, 10 px ate um fio
// de 1 px a 8 % de borda a borda da LINHA, e 6 px ate a primeira linha. Acima,
// 6 px na primeira secao da lista e 22 nas outras (.sec do mockup, margem 6 e
// "margin-top:22px" na segunda). Era CAPTION 22 cinza com 24 de ar: lia como
// nota de rodape, nao como divisao.
#define SP_SECAO_AR1     6.0f
#define SP_SECAO_AR     22.0f
#define SP_SECAO_TXT    27.0f   // a altura da linha de 22 px
#define SP_SECAO_H     (SP_SECAO_AR + SP_SECAO_TXT + 10.0f + 1.0f + 6.0f)
#define SP_SECAO_H1    (SP_SECAO_AR1 + SP_SECAO_TXT + 10.0f + 1.0f + 6.0f)
#define SP_TEXTO_X    (SP_PAD + SP_POSTER_W + 21.0f)
#define SP_TEXTO_W    (SP_INTERNO - SP_POSTER_W - 21.0f)
// Barra de progresso do card de retomada: o trilho fino do mockup, com o que
// FALTA ao lado ("T3E4 · 43 min restantes"), que e o dado que importa.
#define SP_BARRA_W     230.0f
#define SP_BARRA_LABEL_GAP 14.0f
#define SP_BARRA_H       5.0f
// A LINHA DA ILHA (ver linhaIlhaRet e andaresIlha, mais abaixo).
#define SPI_H         114.0f   // 18 + 29 + 4 + 23 + 4 + 18 + 18
#define SPI_H2         92.0f   // dois andares: 18 + 29 + 4 + 23 + 18
#define SPI_AV         52.0f
#define SPI_AV_GAP     18.0f
#define SPI_CAP_W      52.0f
#define SPI_CAP_H      76.0f
#define SPI_CAP_RAIO    9.0f
// A RECOMENDACAO E UM CARTAO DEITADO (dono, F08): arte 16:9 na altura da capa.
#define SPI_CAPD_W    135.0f
#define SPI_FG_R 243
#define SPI_FG_G 242
#define SPI_FG_B 239
#define SP_ABRIR_MS    230.0f
#define SP_FECHAR_MS   150.0f
// O VEU DA FOLHA DE FONTES (streams.c): 30 % sobre o vidro, 42 % no solido.
// Era 58 %: a arte atras do painel apagava, e o vidro nao tinha o que mostrar
// — no mockup o fundo continua vivo a esquerda da ilha.
#define SP_VEU   (ajustes_vidro() ? 0.30f : 0.42f)
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
// "ASSISTIDAS" (dono, 03/10): a rec marcada com "Ja assisti" (ou concluida no
// player) desce para um grupo proprio no fim das recomendacoes. Retrato de
// recresp.h na reconstrucao; `recResp` e a resposta dada (para a 3a linha).
static unsigned char recVista[REC_MAX];
static RecResp recResp[REC_MAX];
static unsigned rrRev;

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
       SPS_ALC_0, SPS_ALC_1, SPS_ALC_2, SPS_ALCANCE, SPS_NOME,
       // F08: o Trakt ligado a este perfil (unir / separar). So existe com o
       // servidor que sabe ("identidade1") e com algo a dizer.
       SPS_IDENT,
       // F08: Simkl (so com login nesta TV ou ja ligado) e Letterboxd (usuario
       // DECLARADO, o servidor nao confere). Mesma gramatica da linha do Trakt.
       SPS_SIMKL, SPS_LETTERBOXD };
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
// A linha do Trakt ligado: o que a reconstrucao viu (situacao*8 + op) e o
// segundo OK que separa (o primeiro so pergunta).
// identConfirma guarda (tipo da linha + 1) que espera o segundo OK; 0 = nenhuma.
static int identVisto = -1, identConfirma, identSeparando;
static int identChave(void) {
  int k = recomenda_identidade_situacao() * 8 + recomenda_identidade_op();
  k = k * 4 + recomenda_identidade_estado(REC_IDENT_SIMKL);
  k = k * 8 + recomenda_identidade_op_de(REC_IDENT_SIMKL);
  k = k * 4 + recomenda_identidade_estado(REC_IDENT_LETTERBOXD);
  k = k * 8 + recomenda_identidade_op_de(REC_IDENT_LETTERBOXD);
  return k;
}
// Simkl/Letterboxd: a linha existe quando da para ligar, ja esta ligada, ou
// ainda ha um aviso do ultimo pedido para ler (depois do OK ela some, se for o caso).
static int identServicoVisivel(int prov) {
  int e = recomenda_identidade_estado(prov), op = recomenda_identidade_op_de(prov);
  return e == REC_IDENT_E_PODE || e == REC_IDENT_E_LIGADO ||
         (op != REC_IDENT_OP_NADA && op != REC_IDENT_OP_OK && e != REC_IDENT_E_INDISPONIVEL) ||
         op == REC_IDENT_OP_SEM_SERVICO;
}
#endif
#define SPS_ALC_TOPO 205.0f   // medido na captura com o corpo da ilha (19/27)

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
// GLASS UI (mockup "ilha" tela 3): as linhas se ENCOSTAM — so a focada tem
// superficie, e os 18 px de recuo de cada uma ja sao o ar entre os textos.
// Pessoa e recomendacao tem os tres andares do mockup (114); sugestao tem dois
// (92); as acoes sao chips de 56 com 8 de ar em cima e embaixo; as caixas de
// ajuste tem 92 com 6 de ar (a caixa aparece em repouso, entao precisa de vao
// para nao encostar na vizinha).
#define SPS_GAP          0.0f
#define SPS_H_CONSENT   84.0f
#define SPS_H_SUG      SPI_H2
#define SPS_H_AMIGO    SPI_H
#define SPS_H_ACAO      72.0f
#define SPS_H_APARECER 104.0f
// Alturas dos dois blocos de texto que NAO sao linha e por isso nao recebem
// foco: o enunciado da pergunta e a explicacao do estado vazio. Sao constantes
// e nao medidas porque a rolagem precisa delas ANTES do desenho — e as duas
// foram conferidas na captura, que e o unico juiz util aqui.
#define SPS_CONSENT_TOPO 345.0f
#define SPS_VAZIO_TOPO   280.0f

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
// A linha da Atividade: rosto, cartaz e tres linhas de texto. MAIOR que a
// linha da ilha (dono, 03/10: "na aba activity eu to achando muito pequeno o
// poster; vamos tentar primeiro aumentando o texto e o poster"): capa 72x106
// em vez de 52x76 e o texto na escala _L da lista com capa dos Salvos.
#define SPA_H        142.0f
#define SPA_AV        60.0f
#define SPA_CAP_W     72.0f
#define SPA_CAP_H    106.0f
#define SPA_CAP_RAIO  11.0f
// A cadeia sob a linha do amigo: a pilula (SVD_CHIP_H) e o ar ate ela.
#define SPS_CADEIA_H    0.0f   // a cadeia virou o terceiro andar da linha

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
void spainel_abrir_titulo(const char *imdb) {
  int i, n;
  spainel_ir_aba(SP_ABA_ATIVIDADE);
  if (aba != SP_ABA_ATIVIDADE || !imdb || !imdb[0]) return;
  socialvis_atualizar();
  n = socialvis_n_eventos();
  for (i = 0; i < n; i++) {
    const SvEvento *ev = socialvis_evento(i);
    if (ev && !strcmp(ev->imdb, imdb)) { foco = i; break; }
  }
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
static unsigned marcaRevCat, marcaRevSalvos, marcaRevOrg, marcaRevRet;
static int reconstrucoes, fundosPintados;
int spainel_n_reconstrucoes(void) { return reconstrucoes; }
int spainel_n_fundos(void) { return fundosPintados; }
static int listaVelha(void) {
  sorg_carregar();   // troca de perfil sobe a revisao da organizacao
  return marcaCatN < 0 || sorg_revisao() != marcaRevOrg || cat_revisao_itens() != marcaRevCat ||
         salvos_revisao() != marcaRevSalvos || cat_n() != marcaCatN ||
         cw_retido_rev() != marcaRevRet;
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
// O QUE ACABOU DE SER VISTO ENTRA EM "CONTINUAR" (dono, 03/10: "as coisas que
// acabei de ver nao tao aparecendo no social, na parte saved; tem que aparecer
// ele com a barra e o tempo"). A uniao (salvos_uniao) so leva o que esta
// SALVO — a lista local e o que a conta marcou naLista —, entao um filme
// comecado sem "+" nunca virava linha, e um salvo da conta cujo progresso so
// existe na copia da fileira Continuar assistindo (sem a marca) ficava sem
// barra. Aqui a fileira "continue_watching" do catalogo, que e onde o progresso
// local, o da conta e o do Trakt ja chegam juntos (descoberta.c), completa a
// lista: titulo novo vira linha com a barra e o tempo; titulo ja listado sem
// progresso ganha o da fileira. So itens com progresso: o "proximo episodio"
// sem nada visto e assunto da home, nao de "Continuar".
static void preencherDoCat(SPLinha *l, const CatItem *c) {
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
static void vistosAgora(void) {
  int r, nf = cat_n_fileiras(), base = nLinhas;
  for (r = 0; r < nf; r++) {
    const CatFileira *f = cat_fileira(r);
    int k;
    if (!f || strcmp(f->chave, "continue_watching")) continue;
    for (k = f->ini; k < f->ini + f->n; k++) {
      const CatItem *c = cat_item(k);
      int j, achou = -1;
      SPLinha *l;
      if (!c || !c->imdb[0] || c->progresso <= 0) continue;
      // UM LUGAR SO (cwretido.h): o que o cartao da ilha / a faixa "Retomar
      // agora" segura so vira linha daqui quando sair de la.
      if (cw_retido_exclui(c->imdb)) continue;
      for (j = 0; j < nLinhas && achou < 0; j++)
        if (salvos_mesmo_titulo(linhas[j].id, c->imdb)) achou = j;
      if (achou >= 0) {
        // Ja listado: so completa a barra de quem nao tinha (os acrescentados
        // aqui ja vieram com a copia mais recente da fileira).
        if (achou < base && linhas[achou].progresso <= 0) preencherDoCat(&linhas[achou], c);
        continue;
      }
      if (nLinhas >= SP_MAX || !garantirLinhas(nLinhas + 1)) return;
      l = &linhas[nLinhas++];
      memset(l, 0, sizeof *l);
      snprintf(l->id, sizeof l->id, "%s", c->imdb);
      snprintf(l->titulo, sizeof l->titulo, "%s", c->titulo);
      preencherDoCat(l, c);
      l->tipoG = tipoGrupo(c->tipo, l->id, l->serie);
      l->ano = anoDe(l->meta);
      l->cat = sorg_categoria_de(l->id);
    }
    break;
  }
}

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
    if (c) preencherDoCat(l, c);
    l->tipoG = tipoGrupo(s ? s->tipo : c->tipo, l->id, l->serie);
    l->ano = anoDe(l->meta);
    l->cat = sorg_categoria_de(l->id);
  }
  vistosAgora();
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
  marcaRevRet = cw_retido_rev();
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
  float w = SP_INTERNO, h = SP_LINHA_SALVO, passo = SP_PASSO, vao = 0.0f, y = 0.0f;
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
          y += (y <= 0.0f ? SP_SECAO_H1 : SP_SECAO_H) + SP_VAZIA_H;
        }
      if (catVazias && g < SORG_CAT_MAX) proximaCat = g + 1;
      novaSecao(y, g, 0);
      y += y <= 0.0f ? SP_SECAO_H1 : SP_SECAO_H;
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
      y += (y <= 0.0f ? SP_SECAO_H1 : SP_SECAO_H) + SP_VAZIA_H;
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
  if (aba == SP_ABA_AVISOS) return avisos_lista_linhas();   // + "Dispensar todos"
  if (aba == SP_ABA_ATIVIDADE) return nAtv;
  return nLinhas;
}

// A BARRA DE OPCOES (Ordenar, Agrupar, Estilo, categorias) mora entre as abas
// e a lista, e empurra a lista SP_OPC_EXTRA para baixo so quando existe.
#define SP_FOCO_BARRA  (-2)
#define SP_OPC_Y     (SP_ABAS_Y + SP_ABAS_H + 14.0f)
#define SP_OPC_H      NV_CTRL_H
#define SP_OPC_EXTRA (SP_OPC_Y + SP_OPC_H + 12.0f - SP_LISTA_Y)
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
    case SPS_REC:       return SPI_H;
    case SPS_SUG:       return SPS_H_SUG;
    case SPS_AMIGO:     return SPS_H_AMIGO +
                          ((social[i].idx >= 0 && social[i].idx < REC_CONTATOS_MAX &&
                            cttCadeia[social[i].idx]) ? SPS_CADEIA_H : 0.0f);
    case SPS_ADICIONAR: return SPS_H_ACAO;
    case SPS_ENCONTRAR: return SPS_H_ACAO;
    case SPS_APARECER:  return SPS_H_APARECER;
    case SPS_ALCANCE:   return SPS_H_APARECER;
    case SPS_NOME:      return SPS_H_APARECER;
    case SPS_IDENT:     return SPS_H_APARECER;
    case SPS_SIMKL:     return SPS_H_APARECER;
    case SPS_LETTERBOXD: return SPS_H_APARECER;
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
static float socialTopo(void);
static float socialAntes(int i) {
  // A PRIMEIRA SECAO DA LISTA tem 6 px de ar, e nao 22 (ver SP_SECAO_AR1).
  float sh = (i == 0 && socialTopo() <= 0.0f) ? SP_SECAO_H1 : SP_SECAO_H;
  if (i < 0 || i >= nSocial) return 0.0f;
  // POR PESSOA: um rotulo com o nome de quem mandou antes da primeira
  // recomendacao de cada pessoa.
  if (social[i].tipo == SPS_REC && recVista[social[i].idx]) {
    if (i == 0 || social[i - 1].tipo != SPS_REC || !recVista[social[i - 1].idx]) return sh;
    return 0.0f;
  }
  if (social[i].tipo == SPS_REC && sorg_social() == SORG_SOCIAL_PESSOA) {
    if (i == 0 || social[i - 1].tipo != SPS_REC) return sh;
    return strcmp(recs[social[i].idx].de, recs[social[i - 1].idx].de) ? sh : 0.0f;
  }
  if (social[i].tipo == SPS_APARECER) return SPS_SEP_APARECER;
  if (social[i].tipo == SPS_NOME) return SPS_SEP_APARECER;
  if (social[i].tipo != SPS_SUG && social[i].tipo != SPS_AMIGO) return 0.0f;
  return (i == 0 || social[i - 1].tipo != social[i].tipo) ? sh : 0.0f;
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
  // AS ASSISTIDAS VAO PARA O FIM, em grupo proprio, sem mudar a ordem dentro
  // de cada metade (particao estavel).
  rrRev = recresp_revisao();
  { RecItem tmp[REC_MAX];
    int k = 0, pass;
    for (pass = 0; pass < 2; pass++)
      for (i = 0; i < nRecs; i++) {
        int v = recresp_assistida(recs[i].id);
        if (v == pass) tmp[k++] = recs[i];
      }
    memcpy(recs, tmp, sizeof recs[0] * (size_t)nRecs);
    for (i = 0; i < nRecs; i++) {
      memset(&recResp[i], 0, sizeof recResp[i]);
      recResp[i].reacao = RECRESP_SEM_REACAO;
      recresp_ler(recs[i].id, &recResp[i]);
      recVista[i] = (unsigned char)(recResp[i].assistida ? 1 : 0);
    } }
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
  // O TRAKT NESTE PERFIL (F08). Sem servidor novo, ou sem as duas contas no
  // aparelho e nada ligado, a linha nao existe: nao ha o que oferecer.
  identVisto = identChave();
  { int sit = recomenda_identidade_situacao(), op = recomenda_identidade_op();
    if ((sit == REC_IDENT_PODE_UNIR || sit == REC_IDENT_UNIDA ||
         op == REC_IDENT_OP_CONFLITO || (op == REC_IDENT_OP_FALHA && sit != REC_IDENT_INDISPONIVEL)) &&
        nSocial < SP_SOCIAL_MAX) {
      social[nSocial].tipo = SPS_IDENT; social[nSocial].idx = 0; nSocial++;
    } }
  if (identServicoVisivel(REC_IDENT_SIMKL) && nSocial < SP_SOCIAL_MAX) {
    social[nSocial].tipo = SPS_SIMKL; social[nSocial].idx = 0; nSocial++;
  }
  if (identServicoVisivel(REC_IDENT_LETTERBOXD) && nSocial < SP_SOCIAL_MAX) {
    social[nSocial].tipo = SPS_LETTERBOXD; social[nSocial].idx = 0; nSocial++;
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
  if (i == 0) return SP_SECAO_H1;
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
enum { TK_NADA = 0, TK_CRIAR, TK_MOVER, TK_RENOMEAR, TK_NOME_SOCIAL, TK_LETTERBOXD };
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
  // O USUARIO DO LETTERBOXD: declarado. Vazio ou curto demais nao faz nada.
  if (tecladoPara == TK_LETTERBOXD) {
    if (r == TECLADO_PRONTO && t && t[0]) { recomenda_identidade_letterboxd_declarar(t); reconstruirSocial(); }
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

// UMA LINHA DAS ABAS ATIVIDADE/AMIGOS que o OK longo segura: titulo do feed,
// recomendacao recebida ou amigo (dono, 03/10: "falta o menu contextual na
// aba activity e friends").
static int linhaSocialSeguravel(void) {
  if (aba == SP_ABA_ATIVIDADE) {
    const SvEvento *ev = foco >= 0 ? socialvis_evento(foco) : NULL;
    return ev && ev->imdb[0];
  }
  if (aba == SP_ABA_SOCIAL && foco >= 0 && foco < nSocial)
    return (social[foco].tipo == SPS_REC && social[foco].idx >= 0 && social[foco].idx < nRecs) ||
           (social[foco].tipo == SPS_AMIGO && social[foco].idx >= 0 && social[foco].idx < nCtts);
  return 0;
}

// O QUE AS EXTRAS DO MENU SOCIAL FAZEM. O menu (ctxmenu.c) so devolve o
// indice; a acao e a linha ficam guardadas aqui ate ele responder.
enum { SPX_ASSISTI = 1, SPX_RESPONDER, SPX_PERFIL, SPX_REMOVER,
       SPX_AV_DISPENSAR, SPX_AV_LEMBRETE, SPX_AV_TODOS };
static int      menuSxAcao[CTX_EXTRAS_MAX];
static RecItem  menuSxRec;
static char     menuSxPessoa[96], menuSxNome[64];
static char     menuAvId[72], menuAvImdb[24];

// SEGURAR OK NUMA LINHA DA ABA AVISOS (06/10, "muito alerta sem dispensar"):
// o mesmo menu das outras abas, so com as acoes do aviso — Dispensar, Remover
// lembrete (estreia com o lembrete ligado) e Dispensar todos. O toque curto
// continua abrindo o alvo, agora decidido na soltura. Dispensar tira a linha e
// grava a chave por perfil (avisodisp.h); o lembrete da serie so sai com
// "Remover lembrete", que tambem dispensa o aviso daquele episodio.
// Depois de dispensar: o foco fica na linha que tomou o lugar, na ultima, ou
// sobe para as abas quando a lista esvaziou.
static void focoAvisosValido(void) {
  int k = avisos_lista_linhas();
  if (foco < 0) return;
  if (k <= 0) foco = temAbas() ? SP_FOCO_ABAS : 0;
  else if (foco >= k) foco = k - 1;
}
static int linhaAvisoSeguravel(void) {
  return aba == SP_ABA_AVISOS && foco >= 0 && foco < avisos_lista_n();
}
static void abrirMenuAvisos(void) {
  CatItem c;
  CtxExtra ex[CTX_EXTRAS_MAX];
  int n = 0;
  static const CtxExtra DISP = { "Dispensar", "aj_x", 0, NULL, NULL, NULL };
  static const CtxExtra LEMB = { "Remover lembrete", "aj_bell", 0, NULL, NULL, NULL };
  static const CtxExtra TODOS = { "Dispensar todos", "aj_x", 0, NULL, NULL, NULL };
  if (!linhaAvisoSeguravel()) return;
  memset(&c, 0, sizeof c);
  if (!avisos_lista_item(foco, menuAvId, sizeof menuAvId, c.titulo, sizeof c.titulo,
                         menuAvImdb, sizeof menuAvImdb)) return;
  menuSxAcao[n] = SPX_AV_DISPENSAR; ex[n++] = DISP;
  if (menuAvImdb[0]) { menuSxAcao[n] = SPX_AV_LEMBRETE; ex[n++] = LEMB; }
  menuSxAcao[n] = SPX_AV_TODOS; ex[n++] = TODOS;
  ctx_abrir_social(&c, ex, n);
}

static void abrirMenuSocial(void) {
  CatItem c;
  CtxExtra ex[CTX_EXTRAS_MAX];
  int n = 0;
  static const CtxExtra PERFIL = { "Ver perfil", "aj_users", 0, NULL, NULL, NULL };
  // "Já assisti" TOMA O LUGAR de "Marcar como assistido": faz o historico da
  // conta e so depois marca a rec e abre o cartao (juntaAssistido, ctxmenu.h).
  static const CtxExtra ASSISTI = { "Já assisti", "check", 0, NULL, NULL, NULL, 1 };
  static const CtxExtra RESPONDER = { "Responder", "aj_users", 0, NULL, NULL, NULL };
  static const CtxExtra REMOVER = { "Remover amigo", "aj_x", 1, "Amigos",
    "Remover %s dos amigos?",
    "O vínculo é desfeito nos dois lados e as recomendações não lidas dessa pessoa somem." };
  if (!linhaSocialSeguravel()) return;
  memset(&c, 0, sizeof c);
  memset(&menuSxRec, 0, sizeof menuSxRec);
  menuSxPessoa[0] = menuSxNome[0] = 0;
  if (aba == SP_ABA_ATIVIDADE) {
    const SvEvento *ev = socialvis_evento(foco);
    snprintf(c.imdb, sizeof c.imdb, "%s", ev->imdb);
    snprintf(c.tipo, sizeof c.tipo, "%s", ev->tipo[0] ? ev->tipo : "movie");
    snprintf(c.titulo, sizeof c.titulo, "%s", ev->titulo);
    snprintf(c.poster, sizeof c.poster, "%s", ev->poster[0] ? ev->poster : ev->arte);
    if (ev->pessoaId[0]) {
      snprintf(menuSxPessoa, sizeof menuSxPessoa, "%s", ev->pessoaId);
      menuSxAcao[n] = SPX_PERFIL; ex[n++] = PERFIL;
    }
  } else if (social[foco].tipo == SPS_REC) {
    const RecItem *r = &recs[social[foco].idx];
    menuSxRec = *r;
    snprintf(c.imdb, sizeof c.imdb, "%s", r->imdb);
    snprintf(c.tipo, sizeof c.tipo, "%s", r->tipo[0] ? r->tipo : "movie");
    snprintf(c.titulo, sizeof c.titulo, "%s", r->titulo);
    snprintf(c.poster, sizeof c.poster, "%s", r->poster);
    c.nota = r->nota;
    if (!recVista[social[foco].idx]) { menuSxAcao[n] = SPX_ASSISTI; ex[n++] = ASSISTI; }
    else if (!recResp[social[foco].idx].respondida) { menuSxAcao[n] = SPX_RESPONDER; ex[n++] = RESPONDER; }
    if (r->de[0]) {
      snprintf(menuSxPessoa, sizeof menuSxPessoa, "%s", r->de);
      menuSxAcao[n] = SPX_PERFIL; ex[n++] = PERFIL;
    }
    // "Mais informações" a partir daqui abre o titulo COM a origem, como o OK.
    atividade_marcar_origem(r->id, r->imdb, r->deNome);
  } else {
    const RecContato *ct = &ctts[social[foco].idx];
    rec_nome_exibicao(c.titulo, sizeof c.titulo, ct->nome, ct->id);
    snprintf(c.poster, sizeof c.poster, "%s", ct->avatar);
    snprintf(menuSxPessoa, sizeof menuSxPessoa, "%s", ct->id);
    snprintf(menuSxNome, sizeof menuSxNome, "%s", c.titulo);
    menuSxAcao[n] = SPX_PERFIL; ex[n++] = PERFIL;
    menuSxAcao[n] = SPX_REMOVER; ex[n++] = REMOVER;
  }
  ctx_abrir_social(&c, ex, n);
}

// A extra escolhida no menu social (por quadro, em spainel_atualizar).
static void extraSocial(int k) {
  if (k < 0 || k >= CTX_EXTRAS_MAX) return;
  switch (menuSxAcao[k]) {
    case SPX_ASSISTI:
      recresp_marcar_assistida(menuSxRec.id);
      recomenda_pedir_agora();
      reconstruirSocial();
      /* fall through: "se quiser, responder" — o mesmo cartao dos creditos */
    case SPX_RESPONDER:
      reacao_rec_abrir(menuSxRec.id, menuSxRec.imdb, menuSxRec.titulo, menuSxRec.tipo,
                       menuSxRec.poster, menuSxRec.deNome, SP_X + SP_W * 0.5f);
      break;
    case SPX_PERFIL:
      if (menuSxPessoa[0]) {
        snprintf(pedidoPerfil, sizeof pedidoPerfil, "%s", menuSxPessoa);
        temPedidoPerfil = 1;
        aberto = 0;
      }
      break;
    case SPX_REMOVER:
      if (menuSxPessoa[0]) { recomenda_remover_contato(menuSxPessoa); reconstruirSocial(); }
      if (foco >= nSocial) foco = nSocial > 0 ? nSocial - 1 : 0;
      break;
    case SPX_AV_LEMBRETE:
      // Desliga o lembrete (so se ainda ligado: alternar religaria) e cai no
      // dispensar: o aviso daquele episodio tambem sai.
      if (menuAvImdb[0] && agenda_lembrete(menuAvImdb) == 1) agenda_alternar_lembrete(menuAvImdb);
      /* fall through */
    case SPX_AV_DISPENSAR:
      if (menuAvId[0]) avisos_dispensar(menuAvId);
      focoAvisosValido();
      break;
    case SPX_AV_TODOS:
      avisos_dispensar_todos();
      focoAvisosValido();
      break;
    default: break;
  }
}

// Abre o menu do cartaz sobre a linha focada. A linha vira um CatItem com o
// que o painel sabe dela; o menu troca pela copia do catalogo quando ela
// existe (ver o modo painel em ctxmenu.c).
static void abrirMenu(void) {
  CatItem c;
  const SPLinha *l;
  if (aba == SP_ABA_ATIVIDADE || aba == SP_ABA_SOCIAL) { abrirMenuSocial(); return; }
  if (aba == SP_ABA_AVISOS) { abrirMenuAvisos(); return; }
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
static void okSocial(void);
static void abrirLinha(void) {
  if (aba == SP_ABA_ATIVIDADE || aba == SP_ABA_SOCIAL) { okSocial(); return; }
  if (aba == SP_ABA_AVISOS) { if (avisos_lista_ok(foco)) spainel_fechar(); return; }
  if (foco >= 0 && foco < nLinhas) {
    snprintf(pedido, sizeof pedido, "%s", linhas[foco].id);
    temPedido = 1;
    aberto = 0;
  }
}

// O TOQUE CURTO nas abas Atividade e Amigos: o que o KEYDOWN fazia antes de
// essas linhas ganharem o menu do OK longo (agora decidido na soltura).
static void okSocial(void) {
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
      case SPS_IDENT: {
        // UNIR E UM OK; SEPARAR SAO DOIS. Separar nao desfaz o que ja foi
        // fundido (o contrato diz) e e a direcao que da trabalho de voltar.
        int sit = recomenda_identidade_situacao();
        if (recomenda_identidade_op() == REC_IDENT_OP_INDO) return;
        if (sit == REC_IDENT_UNIDA) {
          if (identConfirma != SPS_IDENT + 1) { identConfirma = SPS_IDENT + 1; return; }
          identConfirma = 0; identSeparando = 1;
          recomenda_identidade_separar();
        } else if (sit == REC_IDENT_PODE_UNIR) {
          identSeparando = 0;
          recomenda_identidade_unir();
        } else recomenda_identidade_op_limpar();
        reconstruirSocial();
        return; }
      case SPS_SIMKL: {
        // O token e o do Simkl em Ajustes: nunca e pedido de novo aqui.
        int op = recomenda_identidade_op_de(REC_IDENT_SIMKL), e;
        if (op == REC_IDENT_OP_INDO) return;
        if (op == REC_IDENT_OP_CONFLITO || op == REC_IDENT_OP_RECUSADO || op == REC_IDENT_OP_SEM_SERVICO) {
          recomenda_identidade_op_limpar_de(REC_IDENT_SIMKL);
          reconstruirSocial();
          return;
        }
        recomenda_identidade_op_limpar_de(REC_IDENT_SIMKL);
        e = recomenda_identidade_estado(REC_IDENT_SIMKL);
        if (e == REC_IDENT_E_LIGADO) {
          if (identConfirma != SPS_SIMKL + 1) { identConfirma = SPS_SIMKL + 1; return; }
          identConfirma = 0; identSeparando = 1;
          recomenda_identidade_simkl_separar();
        } else if (e == REC_IDENT_E_PODE) {
          identSeparando = 0;
          recomenda_identidade_simkl_unir();
        }
        reconstruirSocial();
        return; }
      case SPS_LETTERBOXD: {
        int op = recomenda_identidade_op_de(REC_IDENT_LETTERBOXD), e;
        if (op == REC_IDENT_OP_INDO) return;
        if (op == REC_IDENT_OP_CONFLITO || op == REC_IDENT_OP_RECUSADO) {
          recomenda_identidade_op_limpar_de(REC_IDENT_LETTERBOXD);
          reconstruirSocial();
          return;
        }
        recomenda_identidade_op_limpar_de(REC_IDENT_LETTERBOXD);
        e = recomenda_identidade_estado(REC_IDENT_LETTERBOXD);
        if (e == REC_IDENT_E_LIGADO) {
          if (identConfirma != SPS_LETTERBOXD + 1) { identConfirma = SPS_LETTERBOXD + 1; return; }
          identConfirma = 0; identSeparando = 1;
          recomenda_identidade_letterboxd_separar();
          reconstruirSocial();
        } else if (e == REC_IDENT_E_PODE) {
          identSeparando = 0;
          tecladoPara = TK_LETTERBOXD;
          teclado_abrir_com("Seu usuário no Letterboxd",
                            "Só você vê. O servidor não confere se é seu.",
                            30, "abcdefghijklmnopqrstuvwxyz0123456789_", "");
        }
        return; }
      case SPS_NOME:
        tecladoPara = TK_NOME_SOCIAL;
        teclado_abrir_com("Como você aparece",
                          "Seu nome para os amigos. Vazio usa o nome do perfil.",
                          32, "abcdefghijklmnopqrstuvwxyz0123456789 -'", recomenda_minha_exibicao());
        return;
#else
      case SPS_ALC_0: case SPS_ALC_1: case SPS_ALC_2: case SPS_ALCANCE: case SPS_NOME: case SPS_IDENT:
      case SPS_SIMKL: case SPS_LETTERBOXD:
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
          atividade_marcar_origem(recs[social[foco].idx].id, recs[social[foco].idx].imdb,
                                  recs[social[foco].idx].deNome);
          temPedido = 1;
          aberto = 0;
        }
        return;
    }
  }
}

static int teclaOk(SDL_Keycode k) {
  return k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE;
}

void spainel_evento(const SDL_Event *e) {
  SDL_Keycode k;
  if (!aberto) return;
  // O CARTAO "O QUE ACHOU?" de uma rec ("Ja assisti") e modal sobre o painel,
  // com o teclado dele por cima.
  if (reacao_painel_aberta()) { reacao_evento(e, 0); return; }
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
    if (temAbas() && foco == SP_FOCO_ABAS) {
      int p = proximaAba(aba, 1);
      if (p != aba) trocarAba(p);   // da ultima aba nao ha para onde ir
    }
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
    // Sem abas a barra e o topo: CIMA para nela.
    if (foco == SP_FOCO_BARRA) { if (temAbas()) foco = SP_FOCO_ABAS; return; }
    if (foco == SP_FOCO_ABAS) return;
    if (aba == SP_ABA_SALVOS && foco < nLinhas) {
      int v = celulaVizinha(foco, -1);
      if (v >= 0) { foco = v; return; }
      if (temBarra()) foco = SP_FOCO_BARRA;
      else if (temAbas()) foco = SP_FOCO_ABAS;
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
      // Linha de aviso: curto abre, longo abre o menu (soltura/limiar). A
      // ultima, "Dispensar todos", age ja: nao ha o que segurar.
      if (linhaAvisoSeguravel()) {
        if (!e->key.repeat && !okDesde) { okDesde = SDL_GetTicks(); if (!okDesde) okDesde = 1; }
        return;
      }
      if (!e->key.repeat && avisos_lista_ok(foco)) spainel_fechar();
      focoAvisosValido();
      return;
    }
    // ATIVIDADE E AMIGOS: titulo e amigo tem menu (OK longo); o resto decide
    // no KEYDOWN, como sempre.
    if (aba == SP_ABA_ATIVIDADE || aba == SP_ABA_SOCIAL) {
      if (linhaSocialSeguravel()) {
        if (!e->key.repeat && !okDesde) { okDesde = SDL_GetTicks(); if (!okDesde) okDesde = 1; }
        return;
      }
      okSocial();
      return;
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
  ctx_centro_dica(aberto && (aba == SP_ABA_SALVOS || aba == SP_ABA_ATIVIDADE || aba == SP_ABA_SOCIAL ||
                             aba == SP_ABA_AVISOS)
                  ? SP_X + SP_W * 0.5f : -1.0f);
  reacao_painel_atualizar(dt, agora);
  if (!aberto && reacao_painel_aberta()) reacao_fechar();
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
  // A acao da linha social escolhida no menu ("Ja assisti", "Ver perfil"...).
  { int k = ctx_pediu_extra();
    if (k >= 0 && aberto) extraSocial(k); }
  // "Ja assisti"/resposta mudou (aqui, no player ou pelo fio): a lista remonta
  // e a rec desce para "Assistidas".
  if (aberto && aba == SP_ABA_SOCIAL && rrRev != recresp_revisao() && !reacao_painel_aberta()) {
    int f = foco;
    reconstruirSocial();
    foco = f < nSocial ? f : (nSocial > 0 ? nSocial - 1 : 0);
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
#if SP_V2
  // O "OK de novo para separar" vale so enquanto o foco esta na linha.
  if (identConfirma && !(aberto && aba == SP_ABA_SOCIAL && foco >= 0 && foco < nSocial &&
                         social[foco].tipo + 1 == identConfirma)) identConfirma = 0;
#endif
  if (aberto && aba == SP_ABA_SOCIAL &&
      (nRecs != recomenda_n() || nSugs != recomenda_n_sugestoes() ||
       consentEstado != recomenda_aparecer() || svRevSocial != socialvis_revisao()
#if SP_V2
       || (consentEstado != REC_APARECER_NAO_PERGUNTADO && alcEstado != recomenda_alcance())
       || (consentEstado != REC_APARECER_NAO_PERGUNTADO && identVisto != identChave())
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
                 : aba == SP_ABA_AVISOS ? avisos_lista_altura_linha(foco, foco)
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

// GLASS UI (dono, 02/10, mockup "ilha" tela 3): FOCO DE LINHA = SUPERFICIE UM
// DEGRAU MAIS CLARA, nos dois materiais — branco a 12 % sobre o vidro, cinza
// opaco sobre o solido. Sem aro, sem bloco cheio no acento e por isso SEM
// inverter o texto: a linha focada segue com as cores de repouso, e o acento
// fica para o estado (a barra de "nao lida", o ponto ao vivo, o selo da aba).
// Em repouso a linha nao tem superficie nenhuma: ela mora direto na ilha, como
// as linhas da folha de Fontes. focoTexto continua existindo para os chips e
// botoes (botaoSup), que sao FOCO DE BOTAO e esses sim invertem.
static float focoTexto(float f) { (void)f; return 0.0f; }

static void superficieItem(GfxRect r, float raio, float f, float a) {
  float v = focoVisual(f);
  if (v <= .001f || a <= .001f) return;
  if (ajustes_vidro()) { gfx_cor(r, raio, 1, 1, 1, .12f * v * a); return; }
  // NO SOLIDO A LINHA FOCADA SOBE: #2b2d34 com a sombra curta do mockup
  // (".row.foco" solido: 0 10px 30px a 40 %) e o fio claro de 1 px no topo
  // ("inset 0 1px 0" a 6 %). Sem a sombra o degrau cinza sobre o cinza do
  // painel e o unico sinal, e de longe ele some.
  gfx_rect((GfxRect){ r.x - 14.0f, r.y - 2.0f, r.w + 28.0f, r.h + 30.0f }, 0, GFX_SOMBRA,
           1.0f, 0, 0, 0.5f, 0, 0, 0, .30f * v * a);
  gfx_cor(r, raio, .169f, .176f, .204f, v * a);
  { float rp = raio * (r.h < r.w ? r.h : r.w);
    gfx_cor((GfxRect){ r.x + rp, r.y, r.w - 2.0f * rp, 1.0f }, 0.0f, 1, 1, 1, .06f * v * a); }
}

// CAIXA DE AJUSTE (o interruptor de aparecer, nivel e nome): precisa de um
// fundo em repouso, senao o interruptor boia sem dizer onde a linha comeca.
// Um degrau abaixo do foco: branco a 5 % no vidro, cinza opaco no solido.
static void superficieCaixa(GfxRect r, float raio, float f, float a) {
  if (ajustes_vidro()) gfx_cor(r, raio, 1, 1, 1, .05f * a);
  else gfx_cor(r, raio, .098f, .102f, .118f, a);
  superficieItem(r, raio, f, a);
}

// FOCO DE BOTAO (chips da barra, "Adicionar um amigo", as respostas do
// consentimento): o mesmo chipFolha da folha de Fontes. Repouso em branco a
// 8 % (vidro) ou cinza opaco (solido); foco = pilula CHEIA no acento — no
// vidro gfx_vidro_pilula_cheia, no solido com a luz de botao atras — e o texto
// vai para ajustes_tinta_foco (quem chama usa focoVisual, nao focoTexto).
static void botaoSup(GfxRect r, float raio, float f, float a) {
  float v = focoVisual(f), ar, ag, ab;
  if (ajustes_vidro()) gfx_cor(r, raio, 1, 1, 1, .08f * (1.0f - v) * a);
  else gfx_cor(r, raio, .14f, .148f, .17f, (1.0f - v) * a);
  if (v <= .001f) return;
  if (ajustes_vidro()) { gfx_vidro_pilula_cheia(r, raio, v, a); return; }
  ajustes_acento(&ar, &ag, &ab);
  botao_luz(r, v * .55f, a);
  gfx_cor(r, raio, ar, ag, ab, v * a);
}

// CAIXA ALTA ESPACADA (".kick" do mockup: 15 bold, 0,14 em): o kicker do
// cabecalho e o rotulo dos chips. A mesma conta de caixaAlta em streams.c —
// i18n antes da caixa alta, porque a tabela de idioma guarda a frase normal.
// x = -1 so mede.
static float caixaAltaIlha(const char *s, int r, int g, int b, float x, float y, float a) {
  char up[200];
  size_t k;
  snprintf(up, sizeof up, "%s", i18n(s));
  for (k = 0; up[k]; k++)
    if (up[k] >= 'a' && up[k] <= 'z') up[k] = (char)(up[k] - 32);
    else if ((unsigned char)up[k] == 0xC3 && up[k + 1] && (unsigned char)up[k + 1] >= 0xA0 &&
             (unsigned char)up[k + 1] <= 0xBE) { up[k + 1] = (char)((unsigned char)up[k + 1] - 0x20); k++; }
  return txt_tracking(TXT_MINI, up, r, g, b, x, y, a, 2.1f);
}
static float secaoAlt(int primeira) { return primeira ? SP_SECAO_H1 : SP_SECAO_H; }
static void desenhaSecao(float x, float y, const char *rotulo, float a, int primeira) {
  TxtLinha t = txt_linha_corta(TXT_ILHA_SECAO, rotulo, 243, 242, 239, 255, SP_INTERNO);
  float ty = y + (primeira ? SP_SECAO_AR1 : SP_SECAO_AR);
  txt_desenhar_alpha(t, x + SP_PAD, ty, a);
  gfx_cor((GfxRect){ x + SP_LINHA_X, ty + SP_SECAO_TXT + 10.0f, SP_LINHA_W, 1.0f }, 0.0f,
          1.0f, 1.0f, 1.0f, 0.08f * a);
}

// --- A LINHA DA ILHA ---------------------------------------------------------
// Uma gramatica so para tudo que tem rosto ou capa (feed, recomendacao, amigo,
// sugestao, aviso): a superficie de 26 a 26 das bordas, raio 22, 18/22 de
// recuo; o rosto de 52 a esquerda, 18 px, o texto em ate tres andares e, quando
// ha, a CAPA 52x76 A DIREITA. Os andares, do CSS do mockup:
//   nome    24 semibold, branco a 88 % (100 % no foco) + o verbo 24 regular a 55 %
//   titulo  19 regular a 62 %, 4 px abaixo
//   quando  15 regular a 38 %, 4 px abaixo
// A tinta e a cor do mockup (#f3f2ef) com ALFA, e nao um cinza opaco: sobre o
// vidro o texto secundario deixa a arte passar do mesmo jeito que no CSS, e a
// chave do cache de texto continua uma so por cor.
static TxtLinha txtIlha(TxtEstilo e, const char *s, float maxW) {
  return txt_linha_corta(e, s, SPI_FG_R, SPI_FG_G, SPI_FG_B, 255, maxW);
}
// A superficie da linha: so no foco (as linhas moram direto na ilha).
static GfxRect linhaIlhaRet(float dx, float y, float h) {
  return (GfxRect){ SP_X + dx + SP_LINHA_X, y, SP_LINHA_W, h };
}
// Nome + verbo na mesma linha de base. O nome atravessa de 88 % para branco
// cheio com o foco, como ".row.foco" do mockup; o verbo fica a 55 %.
static float nomeVerboEst(TxtEstilo es, const char *nome, const char *verbo, float x, float y,
                          float larg, float v, float a) {
  TxtLinha n = txtIlha(es, nome, larg * 0.6f);
  TxtLinha nb = txt_linha_corta(es, nome, 255, 255, 255, 255, larg * 0.6f);
  txt_desenhar_alpha(n, x, y, a * 0.88f * (1.0f - v));
  txt_desenhar_alpha(nb, x, y, a * v);
  if (verbo && verbo[0] && larg - (float)n.w - 7.0f > 40.0f) {
    TxtLinha vb = txtIlha(TXT_ILHA_CORPO, verbo, larg - (float)n.w - 7.0f);
    txt_desenhar_alpha(vb, x + (float)n.w + 7.0f, y, a * 0.55f);
  }
  return (float)n.w;
}
static float nomeVerbo(const char *nome, const char *verbo, float x, float y, float larg,
                       float v, float a) {
  return nomeVerboEst(TXT_ILHA_NOME, nome, verbo, x, y, larg, v, a);
}
// Os tres andares a partir de `tx`, centrados na altura `h` da linha quando
// faltam andares (o mockup centra o bloco ao lado do rosto, align-items).
static void andaresIlha(float tx, float y, float h, float larg, float v, float a,
                        const char *nome, const char *verbo, const char *l2, const char *l3) {
  float bloco = 29.0f + (l2 && l2[0] ? 27.0f : 0.0f) + (l3 && l3[0] ? 22.0f : 0.0f);
  float ty = y + (h - bloco) * 0.5f;
  nomeVerbo(nome, verbo, tx, ty, larg, v, a);
  ty += 29.0f + 4.0f;
  if (l2 && l2[0]) {
    txt_desenhar_alpha(txtIlha(TXT_ILHA_SUB, l2, larg), tx, ty, a * 0.62f);
    ty += 23.0f + 4.0f;
  }
  if (l3 && l3[0]) txt_desenhar_alpha(txtIlha(TXT_ILHA_HORA, l3, larg), tx, ty, a * 0.38f);
}
// O ROSTO: a foto, ou o disco na cor da pessoa com a inicial em 20 bold
// (".av" do mockup). rec_avatar desenha a inicial no CALLOUT de 28, grande
// demais para o disco de 52 — entao a letra dele vai em branco (" ") e a
// inicial e escrita aqui. AO VIVO o anel vermelho e o ponto continuam: e
// estado, e estado e o que leva cor na ilha.
static void rostoIlha(GfxRect r, const char *url, const char *nome, const char *id,
                      int vivo, float a, Uint32 agora) {
  GLuint foto = (url && url[0]) ? tex_obter_larg(url, (int)r.w) : 0;
  if (vivo) gfx_anel_fora(r, 0.5f, 3.0f, 3.0f, SVD_VIVO_R, SVD_VIVO_G, SVD_VIVO_B, a);
  rec_avatar(r, url, foto ? nome : " ", id && id[0] ? id : nome, a);
  if (!foto) {
    char ini[8];
    size_t z = 1;
    TxtLinha l;
    if (!nome || !nome[0]) nome = "?";
    while (z < 4 && (nome[z] & 0xc0) == 0x80) z++;
    memcpy(ini, nome, z);
    ini[z] = 0;
    if (ini[0] >= 'a' && ini[0] <= 'z') ini[0] = (char)(ini[0] - 32);
    l = txt_linha(TXT_ILHA_INICIAL, ini, 255, 255, 255, 255);
    txt_desenhar_alpha(l, r.x + (r.w - l.w) * 0.5f, r.y + (r.h - l.h) * 0.5f, a);
  }
  if (vivo) {
    float pd = r.w * 0.26f;
    svd_ponto_vivo(r.x + r.w * 0.5f + r.w * 0.5f * 0.7071f,
                   r.y + r.h * 0.5f + r.w * 0.5f * 0.7071f, pd, pd * 0.18f, a, agora);
  }
}
// A CAPA A DIREITA (".cap"): 52x76, raio 9, com o recuo de 22 da linha.
// A arte ENCHE a caixa (cover), recortada pelo raio: um cartaz que chega
// em outra proporcao (arte 16:9 no lugar do 2:3) sairia com faixas pretas
// dentro da capa, que e o que a primeira captura mostrou.
static void capaArte(GfxRect r, const char *url, float raioPx, float a) {
  GLuint tex = url && url[0] ? tex_obter_larg(url, (int)r.w) : 0;
  float raio = raioPx / (r.w < r.h ? r.w : r.h);
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
// A arte DEITADA de uma recomendacao: o fundo do titulo no catalogo, ou o da
// metahub pelo id do IMDb (a mesma regra de artemetahub.h). Se ela falhar, o
// cartaz recortado no mesmo quadro — o cartao continua deitado, de proposito.
static void capaDeitadaIlha(float dx, float y, float h, const char *imdb, const char *poster, float a) {
  GfxRect c = { SP_X + dx + SP_LINHA_X + SP_LINHA_W - SP_LINHA_PADX - SPI_CAPD_W,
                y + (h - SPI_CAP_H) * 0.5f, SPI_CAPD_W, SPI_CAP_H };
  char arte[512] = "";
  int k = imdb && imdb[0] ? cat_indice_por_imdb(imdb) : -1;
  const CatItem *it = k >= 0 ? cat_item(k) : NULL;
  if (it && it->backdrop[0]) snprintf(arte, sizeof arte, "%s", it->backdrop);
  else if (imdb && !strncmp(imdb, "tt", 2))
    snprintf(arte, sizeof arte, "https://images.metahub.space/background/medium/%s/img", imdb);
  capaArte(c, arte[0] && !tex_falhou(arte) ? arte : poster, SPI_CAP_RAIO, a);
}
static void capaIlha(float dx, float y, float h, const char *url, float a) {
  GfxRect c = { SP_X + dx + SP_LINHA_X + SP_LINHA_W - SP_LINHA_PADX - SPI_CAP_W,
                y + (h - SPI_CAP_H) * 0.5f, SPI_CAP_W, SPI_CAP_H };
  capaArte(c, url, SPI_CAP_RAIO, a);
}
// Nota do IMDb em texto ("IMDb 8,1"), com o separador do idioma.
static void notaTexto(char *dst, size_t tam, int nota) {
  if (nota <= 0) { dst[0] = 0; return; }
  snprintf(dst, tam, idioma_ponto_decimal(ajustes_idioma()) ? "IMDb %d.%d" : "IMDb %d,%d",
           nota / 10, nota % 10);
}
// Junta `parte` ao fim de `dst` com " · ".
static void juntar(char *dst, size_t tam, const char *parte) {
  size_t k = strlen(dst);
  if (!parte || !parte[0] || k + 1 >= tam) return;
  snprintf(dst + k, tam - k, "%s%s", k ? " \xc2\xb7 " : "", parte);
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
    superficieItem(r, SP_LINHA_RAIO / r.h, f, a);
  }
  arteCelula(poster, l->poster, 0.06f, a);
  barraSobreArte(poster, l->progresso, a);
  // O titulo na tinta da ilha: 19 a 88 %, branco cheio no foco.
  { TxtLinha r = txtIlha(TXT_ILHA_SUB, l->titulo, l->lw);
    TxtLinha fo = txt_linha_corta(TXT_ILHA_SUB, l->titulo, 255, 255, 255, 255, l->lw);
    float fv = focoVisual(f);
    txt_desenhar_alpha(r, x, y + SPG_POSTER_H + 12.0f, a * 0.88f * (1.0f - fv));
    txt_desenhar_alpha(fo, x, y + SPG_POSTER_H + 12.0f, a * fv); }
  (void)tf2; (void)tf; (void)v;
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
    superficieItem(r, SP_LINHA_RAIO / r.h, f, a);
  }
  arteCelula(img, l->fundo[0] ? l->fundo : l->poster, 0.07f, a);
  barraSobreArte(img, l->progresso, a);
  if (l->progresso > 0 && l->txtRestante[0]) snprintf(apoio, sizeof apoio, "%s", l->txtRestante);
  else if (l->ano > 0) snprintf(apoio, sizeof apoio, "%s · %d", i18n(tipoRotulo(l)), l->ano);
  else snprintf(apoio, sizeof apoio, "%s", i18n(tipoRotulo(l)));
  // Os dois andares de cima da linha da ilha: o titulo (24 semibold) e o
  // apoio (19 a 62 %).
  nomeVerbo(l->titulo, NULL, x, y + SPP_IMG_H + 12.0f, l->lw / 0.6f, focoVisual(f), a);
  txt_desenhar_alpha(txtIlha(TXT_ILHA_SUB, apoio, l->lw), x, y + SPP_IMG_H + 45.0f, a * 0.62f);
  (void)tf; (void)tf2; (void)v;
}

// A LINHA DE SALVOS, na gramatica da ilha: o cartaz e a coisa (fica a
// esquerda, onde o feed poe o rosto), o titulo em 24 semibold, o que ele e em
// 19 a 62 % ("Série · 2016 · IMDb 8,7 · Kids") e o andar de baixo em 15 —
// quando foi salvo, ou o trilho fino com o que falta. Os SELOS em pilula
// (tipo, ano, IMDb amarelo) sairam: o mockup escreve qualidade como texto, e
// tres pilulas por linha eram o "menos polido" que o dono apontou.
static void desenhaLinha(int i, float dx, float y, float a) {
  const SPLinha *l = &linhas[i];
  float f = animFoco[i], v = focoVisual(f);
  float px = SP_X + dx + SP_PAD, tx = SP_X + dx + SP_TEXTO_X;
  char buf[192], meta[256];
  GfxRect poster = { px, y + SP_LINHA_PADY, SP_POSTER_W, SP_POSTER_H };
  { GfxRect r = linhaIlhaRet(dx, y, SP_LINHA_SALVO);
    superficieItem(r, SP_LINHA_RAIO / r.h, f, a); }

  // PELA LARGURA DE DESENHO, e nao tex_obter: este decodificava cada cartaz a
  // 640 de largura (~2,4 MB) para desenha-lo pequeno: 121 salvos rolados
  // pedem ~290 MB a um cache de 300 (o log da C9 de 24/09, com o painel
  // aberto, mostrava gpu-cache=221 269MB). tex_cache.h: "Prefira esta a
  // tex_obter em qualquer arte de lista".
  // Esqueleto VISIVEL (#2C2C2C) enquanto nao chega, o mesmo da home: um
  // retangulo da cor do fundo le como card quebrado, nao como carregando.
  capaArte(poster, l->poster, 10.0f, a);

  meta[0] = 0;
  juntar(meta, sizeof meta, i18n(tipoRotulo(l)));
  juntar(meta, sizeof meta, l->meta);
  notaTexto(buf, sizeof buf, l->nota);
  juntar(meta, sizeof meta, buf);
  // A CATEGORIA DA PESSOA, quando a lista nao esta agrupada por ela: e a
  // resposta visivel do "Mover para categoria" — sem ela, mover com a lista
  // agrupada por progresso nao mudaria nada na tela.
  if (l->cat && grupoAtual != SORG_GRUPO_CATEGORIA)
    juntar(meta, sizeof meta, sorg_categoria_nome_id(l->cat));

  { float ty = y + SP_LINHA_PADY + (SP_POSTER_H - 92.0f) * 0.5f;
    nomeVerboEst(TXT_ILHA_NOME_L, l->titulo, NULL, tx, ty, SP_TEXTO_W / 0.6f, v, a);
    txt_desenhar_alpha(txtIlha(TXT_ILHA_SUB_L, meta, SP_TEXTO_W), tx, ty + 39.0f, a * 0.62f);
    ty += 71.0f;
    if (l->progresso > 0) {
      // O TRILHO DA HOME em miniatura: branco a 18 % e o preenchimento no
      // acento (e estado — o quanto falta). Ao lado, o que FALTA ("T3E4 · 43
      // min restantes") a 62 %, e o ja visto, contexto, a 38 %.
      float p = anim_clamp(l->progresso / 100.0f, 0.0f, 1.0f), pr, pg, pb;
      float lx = tx + SP_BARRA_W + SP_BARRA_LABEL_GAP;
      GfxRect trilho = { tx, ty + (21.0f - SP_BARRA_H) * 0.5f, SP_BARRA_W, SP_BARRA_H };
      GfxRect cheio = trilho;
      TxtLinha r = txtIlha(TXT_ILHA_HORA_L, l->txtRestante, SP_TEXTO_W - SP_BARRA_W - SP_BARRA_LABEL_GAP);
      ajustes_acento(&pr, &pg, &pb);
      cheio.w = trilho.w * p;
      gfx_cor(trilho, 0.5f, 1.0f, 1.0f, 1.0f, 0.18f * a);
      if (cheio.w > SP_BARRA_H) gfx_cor(cheio, 0.5f, pr, pg, pb, a);
      txt_desenhar_alpha(r, lx, ty, a * 0.62f);
      if (l->txtVisto[0] && lx + (float)r.w + 20.0f < tx + SP_TEXTO_W) {
        snprintf(buf, sizeof buf, "\xc2\xb7 %s", l->txtVisto);
        txt_desenhar_alpha(txtIlha(TXT_ILHA_HORA_L, buf, tx + SP_TEXTO_W - lx - (float)r.w - 7.0f),
                           lx + (float)r.w + 7.0f, ty, a * 0.38f);
      }
    } else {
      quandoTexto(buf, sizeof buf, l->quandoS);
      if (buf[0]) txt_desenhar_alpha(txtIlha(TXT_ILHA_HORA_L, buf, SP_TEXTO_W), tx, ty, a * 0.38f);
    } }
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

// `linha` e a posicao na lista da aba (de onde sai a animacao de foco) e `idx`
// a posicao na lista de recomendacoes. Os dois coincidem hoje — as
// recomendacoes vem primeiro — e sao parametros separados para que continuem
// coincidindo por construcao e nao por sorte no dia em que algo entrar antes.
static void desenhaRecLinha(int linha, int idx, float dx, float y, float a, Uint32 agora) {
  const RecItem *r = &recs[idx];
  float f = (linha >= 0 && linha < SP_MAX) ? animFoco[linha] : 0.0f, v = focoVisual(f);
  float px = SP_X + dx + SP_PAD, tx = px + SPI_AV + SPI_AV_GAP;
  float larg = SP_X + dx + SP_LINHA_X + SP_LINHA_W - SP_LINHA_PADX - SPI_CAPD_W - 18.0f - tx;
  char l2[256], l3[320], q[64], nota[24];
  int pct, te, ee, est;
  { GfxRect c = linhaIlhaRet(dx, y, SPI_H);
    superficieItem(c, SP_LINHA_RAIO / c.h, f, a); }
  // "NAO LIDA" E UM PONTO DE ACENTO no recuo esquerdo da linha, e nao mais a
  // barra de 6x138 de fora: com a linha na gramatica do mockup (rosto a
  // esquerda, sem cartaz alto) a barra virava um traco solto. O ponto de 8 px
  // mora nos 22 de recuo, antes do rosto, e e estado — por isso no acento.
  if (!r->visto) {
    float ar, ag, ab;
    ajustes_acento(&ar, &ag, &ab);
    gfx_cor((GfxRect){ SP_X + dx + SP_LINHA_X + 7.0f, y + SPI_H * 0.5f - 4.0f, 8.0f, 8.0f },
            0.5f, ar, ag, ab, a);
  }
  // O MESMO ROSTO DA ATIVIDADE: quem mandou e o que distingue uma linha da
  // outra antes de qualquer leitura (o pedido original do dono), e a linha
  // diz a frase do feed — "Gustavo te mandou" — como a primeira do mockup.
  rostoIlha((GfxRect){ px, y + (SPI_H - SPI_AV) * 0.5f, SPI_AV, SPI_AV },
            r->deAvatar, r->deNome, r->de, 0, a, agora);
  capaDeitadaIlha(dx, y, SPI_H, r->imdb, r->poster, a);
  // TITULO · FILME OU SERIE · NOTA: o tipo e a nota continuam (a linha nao
  // dizia se era um filme de duas horas ou oito temporadas), agora em texto.
  // A nota vem do proprio RecItem: o titulo recomendado costuma NAO estar no
  // catalogo de quem recebe.
  snprintf(l2, sizeof l2, "%s", r->titulo);
  juntar(l2, sizeof l2, i18n(r->tipo[0] && !strncmp(r->tipo, "series", 6) ? "Série" : "Filme"));
  notaTexto(nota, sizeof nota, r->nota);
  juntar(l2, sizeof l2, nota);
  // QUANDO, O MEU ESTADO NO QUE ME MANDARAM (tela A: "Você começou · T1E3")
  // e a frase dele entre aspas — o andar de baixo do mockup, a 38 %.
  rec_quando_texto(q, sizeof q, r->criado);
  snprintf(l3, sizeof l3, "%s", q);
  est = socialvis_meu_estado(r->imdb, &pct, &te, &ee);
  if (recVista[idx]) {
    const RecResp *rr = &recResp[idx];
    juntar(l3, sizeof l3, i18n("Você assistiu"));
    juntar(l3, sizeof l3, rr->reacao == 1 ? i18n("Gostou") : rr->reacao == 0 ? i18n("Mais ou menos")
                         : rr->reacao == -1 ? i18n("Não gostou") : "");
    if (rr->texto[0]) {
      char fr[96];
      snprintf(fr, sizeof fr, "\xe2\x80\x9c%s\xe2\x80\x9d", rr->texto);
      juntar(l3, sizeof l3, fr);
    }
    est = 0;
  } else
  if (est == 2) juntar(l3, sizeof l3, i18n("Você viu"));
  else if (est == 1) {
    char d[64];
    if (te > 0 && ee > 0) {
      char ep[24];
      snprintf(ep, sizeof ep, i18n("T%dE%d"), te, ee);
      snprintf(d, sizeof d, "%s %s", i18n("Você começou"), ep);
    } else snprintf(d, sizeof d, "%s %d%%", i18n("Você começou"), pct);
    juntar(l3, sizeof l3, d);
  }
  { const char *frase = rec_frase(r);
    if (frase[0]) {
      char fr[200];
      snprintf(fr, sizeof fr, "\xe2\x80\x9c%s\xe2\x80\x9d", frase);
      juntar(l3, sizeof l3, fr);
    } }
  andaresIlha(tx, y, SPI_H, larg, v, a, r->deNome, i18n("te mandou"), l2, l3);
}

// A LINHA DE ABAS. Sem animacao de cor de proposito: a cor faz parte da chave
// do cache de linhas de text.c, e uma cor por quadro cria uma rasterizacao TTF
// e uma textura GL por quadro — estourado o orcamento, a linha simplesmente
// NAO E DESENHADA (ver a nota longa em ctxmenu.c). O foco aparece no anel, que
// e geometria e nao custa texto.
// Largura da faixa de abas, para a contagem do cabecalho parar antes dela.
// AS ABAS SAO UM SELETOR SEGMENTADO (Glass UI, mockup "ilha" tela 3; o mesmo
// seletor de addon da folha de Fontes): um conteiner pilula em branco a 5 %
// (solido: cinza opaco), a aba aberta num segmento um degrau mais claro e, com
// o D-pad na faixa, esse segmento vira a pilula cheia no acento (foco de botao).
// Era rotulo solto com um traco de acento embaixo da aba aberta: lia como
// texto, nao como controle, e o traco gastava o acento num estado que a
// superficie ja diz.
#define SP_SEG_PAD   5.0f
#define SP_SEG_VAO   4.0f
#define SP_SEG_TXT  20.0f
#define SP_SEG_NUM   9.0f
static const char *rotuloAba(int i) {
  return i == SP_ABA_SALVOS ? "Salvos" : i == SP_ABA_ATIVIDADE ? "Atividade"
       : i == SP_ABA_SOCIAL ? "Amigos" : "Avisos";
}
// A CONTAGEM AO LADO DO ROTULO (".sg .n" do mockup: "Salvos 12", "Amigos 4",
// "Avisos 3"), 16 px a 35 %. O que ha de NOVO (recomendacao recebida, aviso
// nao lido) e estado: o numero passa a ser o das novidades e vai no acento —
// era um disco de acento com o numeral dentro, e o mockup nao tem disco.
// `*novo` diz qual dos dois. 0 = sem numero (Atividade, ou lista vazia).
static int contaDaAba(int i, int *novo) {
  int n;
  *novo = 0;
  if (i == SP_ABA_SALVOS) return nLinhas;
  if (i == SP_ABA_SOCIAL) {
    if ((n = recomenda_n_novas()) > 0) { *novo = 1; return n; }
    return socialvis_n_amigos();
  }
  if (i == SP_ABA_AVISOS) {
    if ((n = avisos_n_novos()) > 0) { *novo = 1; return n; }
    return avisos_lista_n();
  }
  return 0;
}
static float segLargura(int i, TxtLinha *t, TxtLinha *num, int cor) {
  int novo, n = contaDaAba(i, &novo);
  char b[16];
  *t = txt_linha(TXT_ILHA_SEG, i18n(rotuloAba(i)), cor, cor, cor, 255);
  num->w = 0; num->h = 0; num->tex = 0;
  if (n > 0) {
    float ar, ag, ab;
    snprintf(b, sizeof b, "%d", n > 99 ? 99 : n);
    ajustes_acento(&ar, &ag, &ab);
    *num = novo ? txt_linha(TXT_ILHA_NUM, b, (int)(ar * 255), (int)(ag * 255), (int)(ab * 255), 255)
                : txt_linha(TXT_ILHA_NUM, b, SPI_FG_R, SPI_FG_G, SPI_FG_B, 255);
  }
  return (float)t->w + SP_SEG_TXT * 2.0f + (num->w > 0 ? SP_SEG_NUM + (float)num->w : 0.0f);
}
static float abasLargura(void) {
  float w = SP_SEG_PAD * 2.0f - SP_SEG_VAO;
  int i;
  for (i = 0; i < SP_ABA_N; i++) {
    TxtLinha t, n;
    if (!abaExiste(i)) continue;
    w += segLargura(i, &t, &n, 176) + SP_SEG_VAO;
  }
  return w;
}

// AS ABAS SAO UM SELETOR SEGMENTADO (".seg" do mockup): conteiner pilula em
// branco a 6 % (solido #1d1e23), a aba aberta num segmento a 14 % (#34363e)
// em branco, as outras a 55 %. Com o D-pad na faixa o segmento aberto vira a
// pilula cheia no acento — e foco de botao, como os chips da folha de Fontes.
// Sem animacao de cor de proposito: a cor faz parte da chave do cache de
// linhas de text.c, e uma cor por quadro cria uma rasterizacao TTF por quadro.
// Por isso o rotulo e branco e o 55 % e ALFA, e nao um cinza.
static void desenhaAbas(float dx, float a) {
  float x = SP_X + dx + SP_ABAS_X, ar, ag, ab;
  int i;
  GfxRect caixa = { x, SP_ABAS_Y, abasLargura(), SP_ABAS_H };
  ajustes_acento(&ar, &ag, &ab);
  if (ajustes_vidro()) gfx_cor(caixa, 0.5f, 1, 1, 1, .06f * a);
  else gfx_cor(caixa, 0.5f, .113f, .118f, .137f, a);
  x += SP_SEG_PAD;
  for (i = 0; i < SP_ABA_N; i++) {
    int ativa = (i == aba);
    int focada = ativa && foco == SP_FOCO_ABAS;
    int cor = focada ? ajustes_tinta_foco() : 255;
    float alfaTxt = focada || ativa ? 1.0f : 0.55f;
    TxtLinha t, num;
    float w;
    GfxRect p;
    if (!abaExiste(i)) continue;
    w = segLargura(i, &t, &num, cor);
    p = (GfxRect){ x, SP_ABAS_Y + SP_SEG_PAD, w, SP_ABAS_H - SP_SEG_PAD * 2.0f };
    if (focada) {
      if (ajustes_vidro()) gfx_vidro_pilula_cheia(p, 0.5f, 1.0f, a);
      else { botao_luz(p, .55f, a); gfx_cor(p, 0.5f, ar, ag, ab, a); }
    } else if (ativa) {
      if (ajustes_vidro()) gfx_cor(p, 0.5f, 1, 1, 1, .14f * a);
      else gfx_cor(p, 0.5f, .204f, .212f, .243f, a);
    }
    txt_desenhar_alpha(t, x + SP_SEG_TXT, p.y + (p.h - t.h) * 0.5f, a * alfaTxt);
    if (num.w > 0) {
      int novo;
      contaDaAba(i, &novo);
      // Sobre a pilula de acento a contagem vai na tinta do foco: no acento
      // ela sumiria justamente na aba em que o dedo esta.
      if (focada) {
        char b[16];
        snprintf(b, sizeof b, "%d", contaDaAba(i, &novo) > 99 ? 99 : contaDaAba(i, &novo));
        num = txt_linha(TXT_ILHA_NUM, b, cor, cor, cor, 255);
        novo = 0;
      }
      // Na base do rotulo, como "align-items:baseline" do mockup: o numeral
      // de 16 desce 3 px em relacao ao centro do de 19.
      txt_desenhar_alpha(num, x + SP_SEG_TXT + (float)t.w + SP_SEG_NUM,
                         p.y + (p.h - t.h) * 0.5f + (float)(t.h - num.h) * 0.78f,
                         a * (novo ? 1.0f : focada ? 0.7f : 0.35f));
    }
    x += w + SP_SEG_VAO;
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

// Os chips da ilha (".chip" do mockup): 56 de altura, branco a 8 % (solido
// #24262c), o valor em 19 semibold a 85 % e o rotulo do que ele muda em
// caixa alta pequena a 45 % por cima — "ORDENAR / Recentes". Foco = pilula
// cheia no acento (botaoSup), como os chips da folha de Fontes.
static void desenhaBarra(float dx, float a) {
  float x = SP_X + dx + SP_ABAS_X;
  int k, n = nChips(), tf = ajustes_tinta_foco();
  float tinta = ajustes_acento_tinta(NULL, NULL, NULL);
  for (k = 0; k < n; k++) {
    const char *cap, *val, *icone;
    char buf[24];
    float f = animBarra[k], v = focoVisual(f), w, ix, cw = 0.0f;
    TxtLinha vr, vf;
    GfxRect r;
    chipTextos(k, &cap, &val, &icone, buf, sizeof buf);
    vr = txt_linha(TXT_ILHA_SEG, i18n(val), SPI_FG_R, SPI_FG_G, SPI_FG_B, 255);
    vf = txt_linha(TXT_ILHA_SEG, i18n(val), tf, tf, tf, 255);
    w = (float)vr.w;
    if (cap) { cw = caixaAltaIlha(cap, 255, 255, 255, -1.0f, 0.0f, 1.0f); if (cw > w) w = cw; }
    w += 44.0f + (icone ? 30.0f : 0.0f);
    r = (GfxRect){ x, SP_OPC_Y, w, SP_OPC_H };
    botaoSup(r, 0.5f, f, a);
    ix = x + 22.0f;
    if (icone) {
      float ic = anim_mistura(.85f * 243.0f / 255.0f, tinta, v);
      gfx_icone((GfxRect){ ix, SP_OPC_Y + (SP_OPC_H - 22.0f) * 0.5f, 22.0f, 22.0f },
                icone, ic, ic, ic, a);
      ix += 30.0f;
    }
    if (cap) {
      float bloco = 18.0f + 1.0f + (float)vr.h;
      float ty = SP_OPC_Y + (SP_OPC_H - bloco) * 0.5f;
      // O rotulo em caixa alta nao cruza para a tinta do foco por alfa: ele
      // e desenhado duas vezes (repouso a 45 %, foco na tinta), como o valor.
      caixaAltaIlha(cap, 255, 255, 255, ix, ty, a * 0.45f * (1.0f - v));
      caixaAltaIlha(cap, tf, tf, tf, ix, ty, a * 0.8f * v);
      txt_desenhar_alpha(vr, ix, ty + 19.0f, a * 0.85f * (1.0f - v));
      txt_desenhar_alpha(vf, ix, ty + 19.0f, a * v);
    } else {
      float ty = SP_OPC_Y + (SP_OPC_H - (float)vr.h) * 0.5f;
      txt_desenhar_alpha(vr, ix, ty, a * 0.85f * (1.0f - v));
      txt_desenhar_alpha(vf, ix, ty, a * v);
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
  gfx_cor((GfxRect){ SP_X, SP_Y, SP_W, SP_H }, SP_RAIO / SP_W, 0.0f, 0.0f, 0.0f, 0.50f * e);
  // A ESCOLHA E UMA ILHA por cima da ilha do painel: o mesmo material, um
  // degrau mais clara no solido para se separar dele sem aro.
  { GfxRect r = { x, y, w, h };
    const int vid = ajustes_vidro();
    gfx_rect((GfxRect){ x - 18.0f, y - 8.0f, w + 36.0f, h + 40.0f }, 0, GFX_SOMBRA,
             1.0f, 0, 0, 0.5f, 0, 0, 0, .42f * e);
    if (vid) gfx_vidro_folha(r, 28.0f / h, e);
    else gfx_cor(r, 28.0f / h, .098f, .102f, .118f, .99f * e);
    gfx_luz_canto(r, 28.0f / h, w * .25f, -h * .25f, w * .9f, 1, 1, 1, (vid ? .06f : .04f) * e); }
  // O TEXTO NA ESCALA DA ILHA: titulo 24 semibold, apoio 19 a 62 %, opcoes
  // 24 regular a 88 % (branco cheio na focada).
  { TxtLinha t = txtIlha(TXT_ILHA_NOME, popTitulo, w - 64.0f);
    txt_desenhar_alpha(t, x + 32.0f, y + 26.0f, e); }
  if (popSub[0]) {
    TxtLinha t = txtIlha(TXT_ILHA_SUB, popSub, w - 64.0f);
    txt_desenhar_alpha(t, x + 32.0f, y + 66.0f, e * 0.62f);
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
    if (f > 0.01f) superficieItem(r, SP_LINHA_RAIO / POP_LINHA_H, f, e);
    if (popL[i].icone) {
      float ic = anim_mistura(200.0f / 255.0f, tinta, v);
      gfx_icone((GfxRect){ tx, ry + (POP_LINHA_H - 24.0f) * 0.5f, 24.0f, 24.0f },
                popL[i].icone, ic, ic, ic, e);
    }
    if (algumIcone) tx += 38.0f;
    { TxtLinha rp = txtIlha(TXT_ILHA_CORPO, popL[i].rot, w - 160.0f);
      TxtLinha fo = txt_linha_corta(TXT_ILHA_CORPO, popL[i].rot, 255, 255, 255, 255, w - 160.0f);
      float fv = focoVisual(f);
      (void)tf;
      txt_desenhar_alpha(rp, tx, ry + (POP_LINHA_H - (float)rp.h) * 0.5f, e * 0.88f * (1.0f - fv));
      txt_desenhar_alpha(fo, tx, ry + (POP_LINHA_H - (float)fo.h) * 0.5f, e * fv); }
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
  { TxtLinha t = txt_linha(TXT_ILHA_NOME, "Quem vê o que você assiste?", 243, 242, 239, 255);
    txt_desenhar_alpha(t, x, y, a); y += t.h + 18.0f; }
  y += txt_bloco(TXT_ILHA_SUB,
      "Seus amigos podem ver o que você está assistindo, o que terminou e do que gostou. Nada sai desta TV antes de você escolher.",
      243, 242, 239, x, y, SP_INTERNO, 27.0f, a * 0.62f, 4) + 14.0f;
  y += txt_bloco(TXT_ILHA_SUB,
      "Amigos dos seus amigos veem só o título e a ação, sem a sua foto.",
      243, 242, 239, x, y, SP_INTERNO, 27.0f, a * 0.5f, 2) + 14.0f;
  txt_bloco(TXT_ILHA_SUB, "Dá para mudar quando quiser, no fim desta aba.",
            243, 242, 239, x, y, SP_INTERNO, 27.0f, a * 0.42f, 2);
}

static void desenhaSocialVazio(float dx, float y0, float a) {
  float x = SP_X + dx + SP_PAD;
  float y = y0 + 8.0f;
  const char *cod = recomenda_meu_codigo();
  { TxtLinha t = txt_linha(TXT_ILHA_NOME, "Nenhuma recomendação ainda",
                           243, 242, 239, 255);
    txt_desenhar_alpha(t, x, y, a); y += t.h + 14.0f; }
  // EM BLOCO: a coluna do painel tem 688px e a frase tem duas oracoes; numa
  // linha so, a captura saiu cortada no meio.
  y += txt_bloco(TXT_ILHA_SUB,
      "Quando alguém da sua lista te recomendar um filme ou série, ele aparece aqui.",
      243, 242, 239, x, y, SP_INTERNO, 27.0f, a * 0.5f, 3) + 22.0f;
  if (cod[0]) {
    // O CODIGO TAMBEM AQUI, e nao so na tela de amigos: este e o painel que o
    // dono abre com uma tecla, e ditar seis caracteres ao telefone e a acao que
    // resolve uma lista vazia sem depender de ninguem ter aceitado aparecer.
    TxtLinha r = txt_linha(TXT_ILHA_SUB, "Seu código", 243, 242, 239, 255);
    TxtLinha c = txt_linha(TXT_TITULO2, cod, 243, 242, 239, 255);
    txt_desenhar_alpha(r, x, y, a * 0.45f);
    y += r.h + 6.0f;
    txt_desenhar_alpha(c, x, y, a);
    y += c.h + 20.0f;
  }
  { TxtLinha t = txt_linha_corta(TXT_ILHA_SUB,
        "Peça o código do seu amigo e adicione-o abaixo.",
        243, 242, 239, 255, SP_INTERNO);
    txt_desenhar_alpha(t, x, y, a * 0.42f); }
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
  // TXT_ILHA_NOME E NAO TXT_TITULO2. Com o titulo grande a pergunta saiu da
  // captura como "Aparecer para outras pessoa" — 688px de coluna nao cabem uma
  // frase de 29 caracteres naquele corpo, e um enunciado cortado ao meio e
  // pior que um enunciado menor.
  { TxtLinha t = txt_linha(TXT_ILHA_NOME, "Aparecer para outras pessoas?",
                           243, 242, 239, 255);
    txt_desenhar_alpha(t, x, y, a); y += t.h + 18.0f; }
  y += txt_bloco(TXT_ILHA_SUB,
      "Se você aceitar, quem já te segue no Trakt e os amigos dos seus amigos passam a ver seu nome e sua foto numa lista de sugestões, e podem te adicionar como contato.",
      243, 242, 239, x, y, SP_INTERNO, 27.0f, a * 0.62f, 4) + 18.0f;
  y += txt_bloco(TXT_ILHA_SUB,
      "Eles não veem o que você assiste, o que você salvou nem o que você recomendou. Nada disso sai desta TV.",
      243, 242, 239, x, y, SP_INTERNO, 27.0f, a * 0.62f, 3) + 18.0f;
  y += txt_bloco(TXT_ILHA_SUB,
      "Se você recusar, continua recebendo e enviando recomendações do mesmo jeito. Você só não aparece na lista de ninguém.",
      243, 242, 239, x, y, SP_INTERNO, 27.0f, a * 0.5f, 3) + 18.0f;
  txt_bloco(TXT_ILHA_SUB,
      "Dá para mudar essa resposta quando quiser, no fim desta aba.",
      243, 242, 239, x, y, SP_INTERNO, 27.0f, a * 0.42f, 2);
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
  float v = focoVisual(f), tinta = ajustes_acento_tinta(NULL, NULL, NULL);
  int tintaFoco = ajustes_tinta_foco();
  float h = NV_CTRL_H;
  TxtLinha repouso, foco;
  float grupo, x0, w;
  GfxRect r;
  (void)sub; (void)primario;
  // O CHIP DA ILHA (".chip": 56 de altura, 19 semibold a 85 %, 22 de recuo),
  // e nao mais a pilula de 25 px da tela de detalhe: as acoes da lista sao do
  // mesmo tamanho que os chips da folha de Fontes e da barra de Salvos. O
  // primario nao ganha corpo maior — e o foco que diz onde o OK cai.
  repouso = txt_linha(TXT_ILHA_SEG, titulo, SPI_FG_R, SPI_FG_G, SPI_FG_B, 255);
  foco = txt_linha(TXT_ILHA_SEG, titulo, tintaFoco, tintaFoco, tintaFoco, 255);
  grupo = (float)repouso.w + ((icone && icone[0]) ? 20.0f + 10.0f : 0.0f);
  w = grupo + 44.0f;
  if (w > SP_INTERNO) w = SP_INTERNO;
  r = (GfxRect){ SP_X + dx + SP_PAD, y + (alt - h) * 0.5f, w, h };
  botaoSup(r, 0.5f, f, a);
  x0 = r.x + 22.0f;
  if (icone && icone[0]) {
    float ic = anim_mistura(.85f * 243.0f / 255.0f, tinta, v);
    gfx_icone((GfxRect){ x0, r.y + (r.h - 20.0f) * 0.5f, 20.0f, 20.0f }, icone, ic, ic, ic, a);
    x0 += 30.0f;
  }
  txt_desenhar_alpha(repouso, x0, r.y + (r.h - (float)repouso.h) * 0.5f, a * 0.85f * (1.0f - v));
  txt_desenhar_alpha(foco, x0, r.y + (r.h - (float)foco.h) * 0.5f, a * v);
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
  int esc = focoTexto(f) >= 0.5f;
  int c1 = esc ? ajustes_tinta_foco() : 240;
  int c2 = esc ? ajustes_tinta_foco2() : 168;
  // Clamp de seguranca: animSw nasce em -1 e so assenta na primeira
  // atualizacao. Um quadro desenhado antes dela (o painel abre e desenha no
  // mesmo quadro) deslocaria a bola para fora da capsula.
  float lig = animSw < 0.0f ? 0.0f : anim_clamp(animSw, 0.0f, 1.0f);
  float largTexto = SP_LINHA_W - 2.0f * SP_LINHA_PADX - SPS_SW_W - SPS_SW_GAP;
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

  // A CAIXA DA ILHA: a mesma borda e o mesmo raio das linhas (26/22), 6 px
  // de ar em cima e embaixo dentro da altura da linha, o titulo em 24
  // semibold e o subtitulo em 19 a 62 % — os dois andares de cima do mockup.
  r.x = SP_X + dx + SP_LINHA_X; r.w = SP_LINHA_W; r.y = y + 6.0f; r.h = alt - 12.0f;
  superficieCaixa(r, SP_LINHA_RAIO / r.h, f, a);
  (void)c1; (void)c2;
  { TxtLinha t = txtIlha(TXT_ILHA_NOME, "Aparecer para outras pessoas", largTexto);
    TxtLinha s = txtIlha(TXT_ILHA_SUB, sub, largTexto);
    float ty = r.y + (r.h - 56.0f) * 0.5f;
    txt_desenhar_alpha(t, r.x + SP_LINHA_PADX, ty, a * (0.88f + 0.12f * focoVisual(f)));
    txt_desenhar_alpha(s, r.x + SP_LINHA_PADX, ty + 33.0f, a * 0.62f); }

  // A TRILHA. Raio 0,5 numa caixa 80x40: o teto 0,5*w/h vale 1,0, entao os
  // 0,5 passam inteiros e as pontas saem em semicirculo de 20 px. Capsula de
  // verdade, e nao "quase".
  //
  // OS 32 px DE RECUO SAO OS MESMOS DO TEXTO. Sem eles a capsula encostava na
  // borda da pilula — apareceu na primeira captura e parecia a linha cortada.
  trilho.x = r.x + r.w - SP_LINHA_PADX - SPS_SW_W;
  trilho.y = r.y + (r.h - SPS_SW_H) * 0.5f;
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
  if (social[i].tipo == SPS_IDENT) {
    int sit = recomenda_identidade_situacao(), op = recomenda_identidade_op();
    titulo = "Trakt neste perfil";
    if (op == REC_IDENT_OP_INDO) valor = i18n(identSeparando ? "Separando…" : "Unindo…");
    else if (op == REC_IDENT_OP_CONFLITO) valor = i18n("Essa conta Trakt já está em outro perfil");
    else if (op == REC_IDENT_OP_FALHA) valor = i18n("Não foi possível. OK para tentar de novo");
    else if (sit == REC_IDENT_UNIDA && identConfirma == SPS_IDENT + 1)
      valor = i18n("OK de novo para separar. O que já foi unido continua aqui");
    else if (sit == REC_IDENT_UNIDA) {
      snprintf(buf, sizeof buf, i18n("Unido a %s · amigos e atividade juntos"), recomenda_identidade_trakt());
      valor = buf;
    } else valor = i18n("OK para unir amigos e atividade do Trakt a este perfil");
  } else
  if (social[i].tipo == SPS_SIMKL) {
    int e = recomenda_identidade_estado(REC_IDENT_SIMKL), op = recomenda_identidade_op_de(REC_IDENT_SIMKL);
    titulo = "Simkl neste perfil";
    if (op == REC_IDENT_OP_INDO) valor = i18n(identSeparando ? "Separando…" : "Unindo…");
    else if (op == REC_IDENT_OP_SEM_SERVICO) valor = i18n("O servidor ainda não oferece o Simkl");
    else if (op == REC_IDENT_OP_CONFLITO) valor = i18n("Essa conta Simkl já está em outro perfil");
    else if (op == REC_IDENT_OP_RECUSADO) valor = i18n("O Simkl não aceitou o login desta TV. Entre de novo em Ajustes");
    else if (op == REC_IDENT_OP_FALHA) valor = i18n("Não foi possível. OK para tentar de novo");
    else if (e == REC_IDENT_E_LIGADO && identConfirma == SPS_SIMKL + 1)
      valor = i18n("OK de novo para separar. O que já foi unido continua aqui");
    else if (e == REC_IDENT_E_LIGADO) valor = i18n("Simkl unido a este perfil");
    else valor = i18n("OK para unir o Simkl a este perfil");
  } else
  if (social[i].tipo == SPS_LETTERBOXD) {
    int e = recomenda_identidade_estado(REC_IDENT_LETTERBOXD), op = recomenda_identidade_op_de(REC_IDENT_LETTERBOXD);
    titulo = "Letterboxd neste perfil";
    if (op == REC_IDENT_OP_INDO) valor = i18n(identSeparando ? "Separando…" : "Unindo…");
    else if (op == REC_IDENT_OP_CONFLITO || op == REC_IDENT_OP_RECUSADO || op == REC_IDENT_OP_FALHA)
      valor = i18n("Não foi possível. OK para tentar de novo");
    else if (e == REC_IDENT_E_LIGADO && identConfirma == SPS_LETTERBOXD + 1) {
      snprintf(buf, sizeof buf, i18n("OK de novo para remover @%s"), recomenda_identidade_usuario_letterboxd());
      valor = buf;
    } else if (e == REC_IDENT_E_LIGADO) {
      snprintf(buf, sizeof buf, i18n("@%s · informado por você, só você vê"), recomenda_identidade_usuario_letterboxd());
      valor = buf;
    } else valor = i18n("OK para informar seu usuário do Letterboxd. Só você vê");
  } else
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
  r.x = SP_X + dx + SP_LINHA_X; r.w = SP_LINHA_W; r.y = y + 6.0f; r.h = alt - 12.0f;
  superficieCaixa(r, SP_LINHA_RAIO / r.h, f, a);
  (void)v; (void)tf; (void)tf2;
  { float lw = r.w - 2.0f * SP_LINHA_PADX, ty = r.y + (r.h - 56.0f) * 0.5f;
    txt_desenhar_alpha(txtIlha(TXT_ILHA_NOME, titulo, lw), r.x + SP_LINHA_PADX, ty,
                       a * (0.88f + 0.12f * focoVisual(f)));
    txt_desenhar_alpha(txtIlha(TXT_ILHA_SUB, valor, lw), r.x + SP_LINHA_PADX, ty + 33.0f, a * 0.62f); }
}

// Uma sugestao: a cara, o nome, POR ONDE ela chegou, e a pilula que diz o que
// o OK faz.
//
// A LINHA DA ORIGEM NAO E ENFEITE — e ela que separa "alguem que voce talvez
// conheca" de "um estranho que o app resolveu mostrar". Sem "Segue no Trakt" ou
// "Amigo de Gustavo", a resposta honesta a "quem e essa pessoa?" seria "nao
// sei", e a de quem esta olhando seria recusar.
static void desenhaSugLinha(int i, int idx, float dx, float y, float a, Uint32 agora) {
  const RecSugestao *s = &sugs[idx];
  float f = (i >= 0 && i < SP_MAX) ? animFoco[i] : 0.0f, v = focoVisual(f);
  float px = SP_X + dx + SP_PAD, tx = px + SPI_AV + SPI_AV_GAP;
  float fimLinha = SP_X + dx + SP_LINHA_X + SP_LINHA_W - SP_LINHA_PADX;
  char origem[128];
  int tf = ajustes_tinta_foco();
  { GfxRect r = linhaIlhaRet(dx, y, SPS_H_SUG);
    superficieItem(r, SP_LINHA_RAIO / r.h, f, a); }
  rostoIlha((GfxRect){ px, y + (SPS_H_SUG - SPI_AV) * 0.5f, SPI_AV, SPI_AV },
            s->avatar, s->nome, s->id, 0, a, agora);
  rec_sugestao_origem(origem, sizeof origem, s);
  // A PILULA DA ACAO E MEDIDA ANTES DO NOME, e o nome e cortado para caber ao
  // lado dela: sem isso, "Carolina Menezes" passava por baixo de "Adicionar"
  // e as duas ficavam ilegiveis na captura. E um chip da ilha em tamanho de
  // linha (44 de altura): repouso a 8 %, e com a linha em foco ele vira a
  // pilula cheia no acento — o OK desta linha E "Adicionar".
  { TxtLinha acR = txtIlha(TXT_ILHA_SEG, "Adicionar", 300.0f);
    TxtLinha acF = txt_linha(TXT_ILHA_SEG, "Adicionar", tf, tf, tf, 255);
    float pw = (float)acR.w + 40.0f;
    GfxRect p = { fimLinha - pw, y + (SPS_H_SUG - 44.0f) * 0.5f, pw, 44.0f };
    andaresIlha(tx, y, SPS_H_SUG, p.x - 18.0f - tx, v, a, s->nome, NULL, origem, NULL);
    botaoSup(p, 0.5f, f, a);
    txt_desenhar_alpha(acR, p.x + 20.0f, p.y + (p.h - acR.h) * 0.5f, a * 0.85f * (1.0f - v));
    txt_desenhar_alpha(acF, p.x + 20.0f, p.y + (p.h - acF.h) * 0.5f, a * v); }
}

// Linha de um AMIGO JA ADICIONADO, na gramatica do feed: o rosto (anel
// vermelho e ponto se esta vendo agora), o nome, o que ele esta fazendo e, a
// direita, a capa do titulo dele. Sem acao alem de abrir o perfil.
static void desenhaAmigoLinha(int i, int idx, float dx, float y, float a, Uint32 agora) {
  const RecContato *c = &ctts[idx];
  float f = (i >= 0 && i < SP_MAX) ? animFoco[i] : 0.0f, v = focoVisual(f);
  float px = SP_X + dx + SP_PAD, tx = px + SPI_AV + SPI_AV_GAP, alt = socialAlt(i);
  float larg = SP_X + dx + SP_LINHA_X + SP_LINHA_W - SP_LINHA_PADX - tx;
  const SvAmigo *am = (cttSv[idx] >= 0) ? socialvis_amigo(cttSv[idx]) : NULL;
  int vivo = am && am->agora && am->nTit > 0;
  char linha2[260], linha3[300];
  { GfxRect r = linhaIlhaRet(dx, y, alt);
    superficieItem(r, SP_LINHA_RAIO / r.h, f, a); }
  rostoIlha((GfxRect){ px, y + (alt - SPI_AV) * 0.5f, SPI_AV, SPI_AV },
            am ? am->avatar : c->avatar, c->nome, c->id, vivo, a, agora);
  if (am && am->nTit > 0) {
    larg -= SPI_CAP_W + 18.0f;
    capaIlha(dx, y, alt, am->tit[0].poster[0] ? am->tit[0].poster : am->tit[0].arte, a);
  }
  // O QUE ELE ESTA FAZENDO (tela A, 02/10/2026), do modelo do social: "Agora ·
  // The Bear · T3E4 · faltam 12 min", ou "Project Hail Mary · Terminou e
  // gostou · ontem". Sem atividade, "Ainda sem atividade" — a FONTE (Trakt)
  // nao e escrita aqui, fica no perfil.
  if (am && am->nTit > 0) {
    char st[160];
    const SvEvento *e = &am->tit[0];
    socialvis_status(e, st, sizeof st);
    if (vivo) {
      const char *resto = strstr(st, " \xc2\xb7 ");
      snprintf(linha2, sizeof linha2, "%s \xc2\xb7 %s%s", i18n("Agora"), e->titulo,
               resto ? resto : "");
    } else snprintf(linha2, sizeof linha2, "%s \xc2\xb7 %s", e->titulo, st);
  } else snprintf(linha2, sizeof linha2, "%s", i18n("Ainda sem atividade"));
  // A CADEIA DO QUE EU MANDEI ("Voce mandou X › viu › gostou") e o andar de
  // baixo, em texto como o resto da ilha — eram pilulas coloridas que o
  // mockup nao tem.
  linha3[0] = 0;
  if (cttCadeia[idx]) {
    SvEnviada m;
    if (socialvis_ultima_enviada(c->id, &m)) {
      snprintf(linha3, sizeof linha3, i18n("Você mandou %s"), m.titulo);
      { size_t k = strlen(linha3);
        snprintf(linha3 + k, sizeof linha3 - k, " \xe2\x80\xba %s",
                 socialvis_enviada_rotulo(&m, NULL));
      }
    }
  }
  andaresIlha(tx, y, alt, larg, v, a, c->nome, NULL, linha2, linha3);
  // O SELO DE CRIADOR depois do nome, na mesma conta de andaresIlha (o nome
  // ocupa ate 60 % da largura; o bloco e centrado na altura da linha).
  if (rec_selo_pessoa_largura(c->selo) > 0.0f) {
    float bloco = 29.0f + (linha2[0] ? 27.0f : 0.0f) + (linha3[0] ? 22.0f : 0.0f);
    float nw = (float)txtIlha(TXT_ILHA_NOME, c->nome, larg * 0.6f).w;
    if (tx + nw + 12.0f + rec_selo_pessoa_largura(c->selo) <= tx + larg)
      rec_selo_pessoa(tx + nw + 12.0f, y + (alt - bloco) * 0.5f + (29.0f - BADGE_H) * 0.5f + 1.0f, c->selo, a);
  }
}

// A ATIVIDADE VAZIA: diz o que vai aparecer e de onde, sem prometer dado que
// ainda nao existe.
static void desenhaAtividadeVazia(float dx, float y0, float a) {
  float x = SP_X + dx + SP_PAD, y = y0 + 8.0f;
  TxtLinha t = txt_linha(TXT_ILHA_NOME, "Nada por aqui ainda", 243, 242, 239, 255);
  txt_desenhar_alpha(t, x, y, a);
  y += (float)t.h + 14.0f;
  txt_bloco(TXT_ILHA_SUB,
      "Quando seus amigos começarem, terminarem ou gostarem de um título, aparece aqui.",
      243, 242, 239, x, y, SP_INTERNO, 27.0f, a * 0.5f, 3);
}

// UMA LINHA DO FEED (tela B): o rosto, o cartaz e tres linhas — "Marina esta
// vendo", "The Bear · T3E4" e quando. O nome e o verbo sao DUAS linhas de
// texto lado a lado (o verbo e chave de i18n inteira; a frase com o nome
// dentro nunca seria). Mesma superficie e mesmo foco das outras linhas.
static void desenhaAtvLinha(int i, float dx, float y, float a, Uint32 agora) {
  const SvEvento *e = socialvis_evento(i);
  float f = (i >= 0 && i < SP_MAX) ? animFoco[i] : 0.0f, v = focoVisual(f);
  float px = SP_X + dx + SP_PAD, tx = px + SPA_AV + SPI_AV_GAP;
  float larg = SP_X + dx + SP_LINHA_X + SP_LINHA_W - SP_LINHA_PADX - SPA_CAP_W - 18.0f - tx;
  char linha[220], ep[24], q[48], tit[220];
  if (!e) return;
  { GfxRect r = linhaIlhaRet(dx, y, SPA_H);
    superficieItem(r, SP_LINHA_RAIO / r.h, f, a); }
  rostoIlha((GfxRect){ px, y + (SPA_H - SPA_AV) * 0.5f, SPA_AV, SPA_AV },
            e->pessoaAvatar, e->pessoaNome, e->pessoaId, e->acao == SV_AGORA, a, agora);
  { GfxRect c = { SP_X + dx + SP_LINHA_X + SP_LINHA_W - SP_LINHA_PADX - SPA_CAP_W,
                  y + (SPA_H - SPA_CAP_H) * 0.5f, SPA_CAP_W, SPA_CAP_H };
    capaArte(c, e->poster[0] ? e->poster : e->arte, SPA_CAP_RAIO, a); }
  socialvis_ep(e, ep, sizeof ep);
  if (ep[0]) snprintf(tit, sizeof tit, "%s \xc2\xb7 %s", e->titulo, ep);
  else snprintf(tit, sizeof tit, "%s", e->titulo);
  // QUANDO, e o que o cartao da home tambem diria: "faltam 12 min", "aos 18 %".
  socialvis_quando(e->quando, q, sizeof q);
  linha[0] = 0;
  if (e->acao == SV_AGORA && e->restanteMin > 0)
    snprintf(linha, sizeof linha, i18n("faltam %d min"), e->restanteMin);
  else if (e->acao == SV_ABANDONO && e->pct >= 0)
    snprintf(linha, sizeof linha, i18n("Parou aos %d%%"), e->pct);
  juntar(linha, sizeof linha, q);
  // OS TRES ANDARES NA ESCALA _L (28 / 22 / 17), centrados na linha de 142.
  { float ty = y + (SPA_H - (34.0f + 5.0f + 27.0f + 5.0f + 21.0f)) * 0.5f;
    nomeVerboEst(TXT_ILHA_NOME_L, e->pessoaNome, socialvis_verbo(e), tx, ty, larg, v, a);
    ty += 34.0f + 5.0f;
    txt_desenhar_alpha(txtIlha(TXT_ILHA_SUB_L, tit, larg), tx, ty, a * 0.62f);
    ty += 27.0f + 5.0f;
    if (linha[0]) txt_desenhar_alpha(txtIlha(TXT_ILHA_HORA_L, linha, larg), tx, ty, a * 0.38f); }
}

static void desenhaVazio(float dx, float a) {
  float cx = SP_X + dx + SP_W * 0.5f;
  // DESTINO SIMKL SEM VINCULO (issue #110): a lista vazia muda seria lida como
  // "o Plan to Watch esta vazio". O que falta e o vinculo, e e isso que sai.
  const char *semSimkl = simkl_aviso_sem_vinculo(ajustes_salvos_no_simkl());
  TxtLinha t1 = txt_linha(TXT_ILHA_NOME, semSimkl ? semSimkl : "Nada salvo por enquanto",
                          243, 242, 239, 255);
  TxtLinha t2 = txt_linha_corta(TXT_ILHA_SUB,
      semSimkl ? "O + salva no Plan to Watch do Simkl, e ele ainda não está vinculado nesta TV."
               : "Aperte + em um filme ou série e ele aparece aqui.",
      243, 242, 239, 255, SP_INTERNO);
  gfx_icone((GfxRect){ cx - 30.0f, listaTopo() + 140.0f, 60.0f, 60.0f },
            "mais", 0.55f, 0.57f, 0.62f, a);
  txt_desenhar_alpha(t1, cx - t1.w * 0.5f, listaTopo() + 232.0f, a);
  txt_desenhar_alpha(t2, cx - t2.w * 0.5f, listaTopo() + 278.0f, a * 0.42f);
}

// 1 = o veu ja esta no fundo (a copia parada foi pintada com ele).
static int veuNoFundo;

// O veu usa a rampa CRUA e o painel a suavizada, pelo mesmo motivo do menu
// lateral: a medida da referencia para o escurecimento e uma reta, e um bloco
// deste tamanho parando de vez no fim do percurso le como corte.
// Tela REAL inteira: o veu vai tambem para a copia parada (spainel_fundo), que
// e pintada fora da camada ampliada.
static void veuInteiro(void) {
  ESCALA_REAL_INI();
  gfx_cor((GfxRect){ 0, 0, 1920.0f, 1080.0f }, 0.0f, 0, 0, 0, SP_VEU * entrada);
  ESCALA_REAL_FIM();
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
//
// A COPIA TAMBEM E REFEITA ENQUANTO SAIU FALTANDO ARTE (relato do dono, 03/10:
// "quando a sidebar social ta aberta, depois de um tempo a home some e so
// volta quando eu fecho"). Parada, a home nao e desenhada, entao a arte dela
// esfria no cache e e a primeira a ser despejada quando as capas e os rostos
// do painel entram. Ate ai a copia segura a imagem. Mas a primeira
// republicacao do catalogo (Continuar assistindo refeito, sync) sobe
// cat_revisao, a copia e repintada com o cache sem a arte da home — e ficava
// assim: os pedidos daquele unico quadro caducavam (pedido velho, tex_cache.c)
// e nada mais repintava. REPRODUZIDO em tests/spainel_fundo_tempo.sh: arte
// nova no cache + republicar = 60 % -> 3 % de conteudo na faixa da home.
// Agora, se a pintura teve pedido sem textura (tex_n_falta), ela e refeita a
// cada SP_FUNDO_FALTA_MS — abaixo dos 200 ms que fazem um pedido caducar — ate
// sair inteira, com um teto para arte que nunca chega.
static const Uint32 SP_FUNDO_REFAZ_MS[] = { 400, 1500, 4000 };
#define SP_FUNDO_FALTA_MS   150
#define SP_FUNDO_FALTA_MAX  80     /* ~12 s de tentativas por invalidacao */
void spainel_fundo(int podeParar, unsigned rev, void (*fundo)(void *), void *ctx) {
  static int pronto, refeitas, faltou, tentativas;
  static unsigned revPronto;
  static Uint32 desde, pintadoEm;
  int parado = podeParar && aberto && entrada >= 0.999f && gfx_snap_ok();
  const char *motivo = "";
  if (!parado) { pronto = 0; refeitas = 0; faltou = 0; tentativas = 0; }
  else if (rev != revPronto && pronto) {
    // OS TEMPOS DE REPINTURA RECOMECAM A CADA REPUBLICACAO (relato do dono,
    // 05/10, na TCL: a home continuou sumindo com o painel aberto). A home
    // tem animacoes de ENTRADA presas ao relogio: a arte que chega esvanece
    // por 220 ms (revela_arte) e a fileira nova sobe com atraso por coluna
    // (revela_entra, ate ~1 s). A republicacao de um catalogo que ja estava
    // aberto (Continuar assistindo refeito a cada 10 min, sync) pinta a copia
    // NO QUADRO EM QUE ISSO COMECA: cartaz em opacidade 0, e `faltou` fica 0
    // porque a textura existe. As tres repinturas de 0,4/1,5/4 s so valiam
    // para a PRIMEIRA copia do painel; depois delas a copia ficava com a
    // fileira apagada ate o painel fechar. Reproduzido em
    // tests/spainel_fundo_tempo.sh (cartazes em branco no rodape da home
    // depois de republicar, com faltou=0 na linha [spainel] abaixo).
    pronto = 0; tentativas = 0; refeitas = 0; motivo = "revisao";
  }
  else if (rev != revPronto) { pronto = 0; tentativas = 0; }
  else if (pronto && faltou && tentativas < SP_FUNDO_FALTA_MAX &&
           SDL_GetTicks() - pintadoEm >= SP_FUNDO_FALTA_MS) {
    pronto = 0;
    tentativas++;
    motivo = "arte faltando";
  }
  else if (pronto && refeitas < (int)(sizeof SP_FUNDO_REFAZ_MS / sizeof *SP_FUNDO_REFAZ_MS) &&
           SDL_GetTicks() - desde >= SP_FUNDO_REFAZ_MS[refeitas]) {
    pronto = 0;
    refeitas++;
    motivo = "tempo";
  }
  if (parado && !pronto && !motivo[0]) motivo = "abertura";
  veuNoFundo = parado;
  if (parado && pronto) { gfx_snap_desenhar(); return; }
  if (parado) {
    gfx_snap_comecar();
    gfx_sem_recorte();
    glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
  }
  { unsigned faltas0 = tex_n_falta;
    if (fundo) fundo(ctx);
    faltou = tex_n_falta != faltas0; }
  // UMA LINHA POR REPINTURA (as de "arte faltando" podem ser 80 por
  // republicacao: saem a primeira e a ultima). E o que diria quando e por que
  // a copia da home de tras foi refeita e se saiu inteira (faltou=0).
  if (parado && motivo[0] && (motivo[0] != 'a' || motivo[1] != 'r' || tentativas == 1 ||
                              tentativas == SP_FUNDO_FALTA_MAX)) {
    printf("[spainel] fundo repintado: motivo=%s rev=%u faltou=%d por-tempo=%d tentativas=%d%s\n",
           motivo, rev, faltou, refeitas, tentativas,
           tentativas == SP_FUNDO_FALTA_MAX && faltou ? " (DESISTIU: arte ainda faltando)" : "");
    fflush(stdout);
  }
  if (parado) {
    veuInteiro();
    gfx_sem_recorte();
    gfx_snap_terminar();
    gfx_snap_desenhar();
    if (!refeitas) desde = SDL_GetTicks();
    pintadoEm = SDL_GetTicks();
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
static void spainel_desenharCorpo_(Uint32 agora);
// Camada ampliada (escala.h): o corpo desenha na tela virtual.
void spainel_desenhar(Uint32 agora) {
  { ESCALA_INI();
    spainel_desenharCorpo_(agora);
    ESCALA_FIM(); }
  reacao_painel_desenhar(agora);   // "Ja assisti" -> "O que achou?" (reacao.h)
}
static void spainel_desenharCorpo_(Uint32 agora) {
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
  float raioForma = SP_RAIO / SP_W;
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
    const int vid = ajustes_vidro();
    if (deIlha) { p = forma; as = entrada > 0.08f ? 1.0f : entrada / 0.08f; }
    // O PAINEL E UMA ILHA (Glass UI, mockup tela 3), no material da folha de
    // Fontes: sombra curta, miolo de vidro (gfx_vidro_folha) ou solido e uma
    // luz larga e fraca no canto de cima. Sem aro. Nascendo da pilula do
    // relogio a forma e a mesma, so o retangulo muda.
    gfx_sombra_sob((GfxRect){ p.x - 18.0f, p.y - 8.0f, p.w + 36.0f, p.h + 40.0f }, 1.0f, 0, 0.5f,
                   0, 0, 0, .38f * as, p, raioForma * p.h, vid ? 0.0f : .98f * as);
    if (vid) gfx_vidro_folha(p, raioForma, as);
    else gfx_cor(p, raioForma, .071f, .075f, .086f, .98f * as);
    gfx_luz_canto(p, raioForma, p.w * .25f, -p.h * .25f, p.w * .9f, 1, 1, 1, (vid ? .06f : .04f) * as);
  }

  // Tudo daqui para baixo fica preso ao painel: sem o recorte, a lista rolada
  // desenha por cima do cabecalho e por baixo da borda inferior.
  gfx_recorte(SP_X + x, SP_Y, SP_W, SP_H);

  // O CABECALHO DO MOCKUP (".kick" + ".ttl" + ".dsc"): o resumo da aba em
  // caixa alta pequena e espacada a 45 %, "Social" em 40 bold embaixo (o
  // disco de fechar do mockup saiu a pedido do dono, 03/10). Era a contagem
  // solta a direita das abas e nenhum titulo — o painel nao dizia o nome
  // dele (dono, 02/10, comparando com o mockup). As PARTES passam por i18n; a
  // juncao, nao (ver metaTexto).
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
    if (nv > 0) snprintf(buf, sizeof buf, "%d %s", nv, i18n("assistindo agora"));
    else {
      // Sem ninguem ao vivo, quantos amigos ha; sem amigos no modelo (o feed
      // pode vir so de recomendacoes), o nome da aba — "0 amigos" em cima de
      // uma lista cheia seria o painel se contradizendo.
      int na = socialvis_n_amigos();
      if (na > 0) snprintf(buf, sizeof buf, "%d %s", na, i18n(na == 1 ? "amigo" : "amigos"));
      else snprintf(buf, sizeof buf, "%s", i18n("Atividade"));
    }
  }
  else if (aba == SP_ABA_AVISOS) {
    int n = avisos_lista_n(), nv = avisos_n_novos();
    if (nv > 0) snprintf(buf, sizeof buf, i18n("%d avisos · %d novos"), n, nv);
    else snprintf(buf, sizeof buf, "%d %s", n, i18n(n == 1 ? "aviso" : "avisos"));
  }
  else if (nCont > 0) {
    snprintf(buf, sizeof buf, "%d %s \xc2\xb7 %d %s", nLinhas,
             i18n(nLinhas == 1 ? "título" : "títulos"),
             nCont, i18n("para retomar"));
  }
  else snprintf(buf, sizeof buf, "%d %s", nLinhas, i18n(nLinhas == 1 ? "título" : "títulos"));
  caixaAltaIlha(buf, SPI_FG_R, SPI_FG_G, SPI_FG_B, SP_X + x + SP_PAD, SP_KICK_Y, a * 0.45f);
  { TxtLinha t = txt_linha(TXT_ILHA_TITULO, temAbas() ? "Social" : "Salvos",
                           SPI_FG_R, SPI_FG_G, SPI_FG_B, 255);
    txt_desenhar_alpha(t, SP_X + x + SP_PAD, SP_TIT_Y, a); }
  if (temAbas()) desenhaAbas(x, a);
  if (temBarra()) desenhaBarra(x, a);

  if (aba == SP_ABA_ATIVIDADE) {
    gfx_recorte(SP_X + x, listaTopo(), SP_W, SP_LISTA_BASE - listaTopo());
    y = listaTopo() + SP_FOCO_AR - scrollY;
    if (nAtv == 0) desenhaAtividadeVazia(x, y, a);
    for (i = 0; i < nAtv; i++) {
      float cab = atvAntes(i);
      if (cab > 0.0f) {
        if (y + cab >= listaTopo() && y <= SP_LISTA_BASE)
          desenhaSecao(SP_X + x, y, atvRot[i], a, i == 0);
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
    avisos_lista_desenhar(SP_X + x + SP_LINHA_X, listaTopo() + SP_FOCO_AR - scrollY, SP_LINHA_W, a, foco);
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
          desenhaSecao(SP_X + x, y,
                       social[i].tipo == SPS_SUG ? "Pessoas que você talvez conheça"
                                                 : "Seus amigos", a, cab < SP_SECAO_H - 0.5f);
        }
        // Por pessoa: o nome de quem mandou abre o grupo dele.
        if (social[i].tipo == SPS_REC && y + cab >= listaTopo() && y <= SP_LISTA_BASE)
          desenhaSecao(SP_X + x, y, recVista[social[i].idx] ? "Assistidas"
                                                             : recs[social[i].idx].deNome,
                       a, cab < SP_SECAO_H - 0.5f);
        y += cab;
      }
      // Fora da janela nao custa texto nem textura — mesma razao da lista de
      // Salvos logo abaixo.
      if (y + alt >= listaTopo() && y <= SP_LISTA_BASE) {
        switch (social[i].tipo) {
          case SPS_REC: desenhaRecLinha(i, social[i].idx, x, y, a, agora); break;
          case SPS_SUG: desenhaSugLinha(i, social[i].idx, x, y, a, agora); break;
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
          case SPS_IDENT:
          case SPS_SIMKL:
          case SPS_LETTERBOXD:
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
    float sa = secaoAlt(secoes[i].y <= 0.0f);
    float sh = sa + (secoes[i].vazia ? SP_VAZIA_H : 0.0f);
    if (sy + sh < listaTopo() || sy > SP_LISTA_BASE) continue;
    desenhaSecao(SP_X + x, sy, secoes[i].rot, a, secoes[i].y <= 0.0f);
    if (secoes[i].vazia) {
      TxtLinha t = txtIlha(TXT_ILHA_SUB,
          "Vazia. Segure OK num título e escolha \xe2\x80\x9cMover para categoria\xe2\x80\x9d.",
          SP_INTERNO);
      txt_desenhar_alpha(t, SP_X + x + SP_PAD, sy + sa + 6.0f, a * 0.5f);
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

int spainel_n_continuar(void) { if (listaVelha()) reconstruir(); return nCont; }
