// Sem disco nos testes que compilam trakt.c sozinhos.
#include "../src/fichameta.h"
#include <stddef.h>
char *fichameta_ler(const char *c, long t) { (void)c; (void)t; return NULL; }
void fichameta_gravar(const char *c, const char *b) { (void)c; (void)b; }
