#ifndef NUVIO_AJUSTES_UX_H
#define NUVIO_AJUSTES_UX_H

typedef struct {
  int op;
  char titulo[160];
  char caminho[160];
  char valor[160];
  int avancado;
  int bloqueado;
} AjusteBuscaResultado;

/* Arte local usada nas amostras ilustrativas, sem rede. */
void ajustes_recursos(const char *dirArte);

/* Consulta apenas o catalogo local de Ajustes, sem rede ou valores privados. */
int ajustes_buscar(const char *consulta, AjusteBuscaResultado *resultados, int capacidade);
void ajustes_abrir_opcao(int op);
/* Consumido: 1 inicia busca; 2 retorna à consulta anterior. */
int ajustes_pediu_busca(void);

#endif
