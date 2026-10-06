// TAMANHO DA URL BASE DE UM ADDON — um numero so para o app inteiro (#201).
//
// A chave do addon vai embutida no CAMINHO da URL (addons.h), e ha addon que
// embute a configuracao INTEIRA: a do Comet e um base64 de ~830 caracteres, e a
// URL dele passa de 870. Enquanto cada modulo guardava a base num `char[600]`
// proprio, ela era cortada em silencio ja na leitura da conta, o Comet recebia
// uma configuracao pela metade e respondia o manifesto "❌ | Comet", com uma
// fonte so. Nada no log dizia que a URL tinha sido cortada.
//
// 2048 e nao 1024: a configuracao padrao do Comet ja ocupa 870, e cada servico
// de debrid a mais na mesma configuracao soma. 2048 da mais que o dobro do caso
// medido e ainda deixa <base> + caminho longe dos 4096 que rede.c aceita.
//
// QUEM GUARDA A BASE USA ESTE NUMERO. Quem monta um pedido (<base>/stream/...,
// <base>/catalog/...) usa NV_ADDON_PEDIDO_MAX e CONFERE o retorno do snprintf
// com NV_COUBE: pedido que nao coube nao e feito. URL cortada nao e uma URL
// pior, e OUTRA URL — e o addon responde a ela como se fosse valida.
//
// QUEM NAO USA, E POR QUE. Duas copias secundarias continuam em 600 porque
// alarga-las custaria memoria demais para o que rendem, e as duas tem a base
// inteira a mao por outro caminho:
//   - ColSource.base (colecoes.h), 8192 fontes: +12 MB. O que nao cabe fica
//     vazio e a leitura cai em addons_base_por_id(addonId);
//   - GCanal.base (guia.c), 3 x 900 canais: +3,9 MB. O que nao cabe fica vazio,
//     que e o caso "pergunta a fonte a todos os addons". Lembrete, fontecache,
//     spotlight e o diagnostico da Live TV recebem a base do canal e herdam
//     esse teto.
// Nas duas a regra e a mesma daqui: vazio sim, cortado nunca.
//
// NUNCA IMPRIMIR A BASE. Ela carrega a chave de debrid do dono; o log diz o
// nome do addon e o tamanho, e so.
#ifndef NV_ADDONURL_H
#define NV_ADDONURL_H

#define NV_ADDON_URL_MAX    2048
#define NV_ADDON_PEDIDO_MAX (NV_ADDON_URL_MAX + 1024)

// O snprintf que escreveu em `buf` coube inteiro? `w` e o retorno dele.
#define NV_COUBE(w, buf) ((w) >= 0 && (size_t)(w) < sizeof(buf))

#include <stdio.h>
#include <stddef.h>

// A PORTA DE ENTRADA. Toda URL de addon que chega (conta, arquivo, instalacao
// pela TV) passa por aqui antes de ser guardada. 1 = cabe. 0 = nao cabe, e a
// linha de log e a unica noticia que o dono tera disso: quem chama PULA o
// addon. `static inline` para os testes que compilam um modulo so nao
// precisarem de mais um arquivo na linha de comando.
static inline int nv_addon_url_cabe(const char *nome, size_t len) {
  if (len < NV_ADDON_URL_MAX) return 1;
  printf("[addons] %s: URL de %lu caracteres nao cabe (maximo %d): addon ignorado\n",
         nome && *nome ? nome : "Addon", (unsigned long)len, NV_ADDON_URL_MAX - 1);
  fflush(stdout);
  return 0;
}

// O PEDIDO (<base>/stream/..., /catalog/..., /manifest.json) COUBE NO BUFFER?
// `w` e o retorno do snprintf que o montou, `tam` o tamanho do buffer. Com a
// base limitada por NV_ADDON_URL_MAX e o buffer em NV_ADDON_PEDIDO_MAX isto nao
// deveria disparar; existe para que, se um id ou um caminho crescer, o pedido
// cortado NAO SAIA. Quem chama pula o pedido quando volta 0.
static inline int nv_addon_pedido_coube(const char *nome, int w, size_t tam) {
  if (w >= 0 && (size_t)w < tam) return 1;
  printf("[addons] %s: pedido de %d caracteres nao cabe (maximo %lu): pulado\n",
         nome && *nome ? nome : "Addon", w, (unsigned long)tam - 1);
  fflush(stdout);
  return 0;
}

#endif
