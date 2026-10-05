// AMIGOS DE UM TITULO: quem, entre os amigos, ASSISTIU e/ou GOSTOU de um
// titulo. Alimenta o chip de rostos no cartaz da Home, a linha "Fabi gostou ·
// Rafa e Mari assistiram" do destaque e a ilha da pagina do titulo.
//
// NAO HA INDICE POR TITULO NO SERVIDOR, e nenhuma rota nova foi criada: o
// indice sai do que socialvis.h ja tem — o feed de atividade dos amigos
// (assistindo agora, terminou, reacao do "O que achou?", nota de tracker) —,
// virado do avesso: imdb -> ate AMT_MAX amigos. So existe o que o amigo
// PUBLICOU (o servidor ja filtra por alcance/atividade ligada); este modulo
// nunca pede nem inventa dado.
//
// CUSTO: o indice e remontado so quando socialvis_revisao() muda (no maximo a
// cada 250 ms a consulta ve isso). Quem desenha le uma copia pequena e nunca
// aloca. `amigostitulo_revisao()` sobe a cada remontagem.
#ifndef NV_AMIGOSTITULO_H
#define NV_AMIGOSTITULO_H
#include "socialvis.h"

#define AMT_MAX 8          // amigos guardados por titulo (o total real vai em `total`)
#define AMT_TITULOS_MAX SV_EVENTOS_MAX

typedef struct {
  char id[96], nome[64], avatar[256];
  int  gostou;             // reacao positiva ou nota de tracker >= 70
  int  viu;                // terminou, esta vendo, reagiu ou avaliou
  int  agora;              // assistindo agora
  int  temporada, episodio;// ate onde viu; 0 = nao se sabe / filme
} AmigoTit;

typedef struct {
  char imdb[24];
  int  n;                  // quantos em a[] (<= AMT_MAX)
  int  total;              // quantos amigos ao todo
  int  nGostou, nViu;      // totais: quem gostou / quem so assistiu
  AmigoTit a[AMT_MAX];     // quem gostou primeiro, depois quem so assistiu; o mais novo antes
} AmigosTitulo;

// Monta o indice a partir de eventos (mais novo primeiro). Puro: nao le
// socialvis; e o que os testes usam. Devolve quantos titulos entraram.
int  amigostitulo_montar(const SvEvento *ev, int n);
// Por quadro, barato: reconfere o feed e remonta so quando ele mudou.
void amigostitulo_atualizar(void);
unsigned amigostitulo_revisao(void);
// 1 = ha amigos neste titulo (copia em *saida, que pode ser NULL).
int  amigostitulo_obter(const char *imdb, AmigosTitulo *saida);
// Texto da linha do destaque: "Fabi gostou · Rafa e Mari assistiram". "" sem amigos.
void amigostitulo_linha_destaque(const AmigosTitulo *t, char *dst, size_t tam);
// Texto da ilha: "Mari gostou · Fabi viu até o E8" (ate 2 amigos, "+N" no resto).
void amigostitulo_linha_ilha(const AmigosTitulo *t, char *dst, size_t tam);
// Primeiro nome do amigo (ate o primeiro espaco), para caber nas frases.
void amigostitulo_primeiro_nome(const char *nome, char *dst, size_t tam);
#endif
