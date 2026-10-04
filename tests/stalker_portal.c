// Stalker (src/stalker.c): normalizacao do portal e varredura de rotas (#237),
// sem rede e sem disco. O prefixo de caminho colado ("/meuportal/c/") tem de
// ficar; "/c/" e barras finais, nao.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "stalker.h"

static char disco[2048]; static int temDisco;
char *dados_ler(const char *nome) { (void)nome; return temDisco ? strdup(disco) : NULL; }
int dados_gravar(const char *nome, const char *c) { (void)nome; snprintf(disco, sizeof disco, "%s", c); temDisco = 1; return 1; }
void dados_apagar(const char *nome) { (void)nome; temDisco = 0; }
static int perfil = 1;
int perfis_ativo(void) { return perfil; }

static char urls[16][300]; static int nUrls; static char ultimoRef[300];
char *rede_baixar_com(const char *url, int segundos, const char *const *cab) {
  int i;
  (void)segundos;
  if (nUrls < 16) snprintf(urls[nUrls++], sizeof urls[0], "%s", url);
  for (i = 0; cab && cab[i]; i++)
    if (!strncmp(cab[i], "Referer: ", 9)) snprintf(ultimoRef, sizeof ultimoRef, "%s", cab[i]);
  return strdup("<html>nao e portal</html>");
}

static void confere(const char *entrada, const char *curto) {
  stalker_definir_portal(entrada);
  if (strcmp(stalker_portal_curto(), curto)) {
    fprintf(stderr, "FALHOU: '%s' -> '%s' (esperava '%s')\n", entrada, stalker_portal_curto(), curto);
    exit(1);
  }
}

int main(void) {
  StalkerCanal sai[4];
  stalker_definir_portal("meu.portal.tv:8080");
  confere("meu.portal.tv", "meu.portal.tv");
  confere("meu.portal.tv:8080", "meu.portal.tv:8080");
  confere("http://meu.portal.tv:8080/", "meu.portal.tv:8080");
  confere("https://meu.portal.tv:8080/c/", "meu.portal.tv:8080");
  confere("meu.portal.tv:8080/c", "meu.portal.tv:8080");
  confere("  meu.portal.tv:8080/c/  ", "meu.portal.tv:8080");
  confere("meu.portal.tv/stalker_portal/c/", "meu.portal.tv/stalker_portal");
  confere("http://meu.portal.tv/custom/c/", "meu.portal.tv/custom");
  confere("http://meu.portal.tv:80/a/b///", "meu.portal.tv:80/a/b");
  confere("meu.portal.tv/meuportal?x=1", "meu.portal.tv/meuportal");
  puts("ok  normaliza host, porta, /c/, prefixo, esquema e espacos");

  // Valor antigo (so host, gravado antes) segue valendo.
  perfil = 2;                 // outro perfil: forca a releitura do "disco"
  snprintf(disco, sizeof disco, "portal\thttp://velho.tv:8080\nmac\t00:1a:79:00:00:01\n");
  stalker_carregar();
  assert(stalker_configurado() && !strcmp(stalker_portal_curto(), "velho.tv:8080"));
  puts("ok  cadastro antigo carrega");

  perfil = 1; temDisco = 0;
  // Rotas: com prefixo, ele vem antes; stalker_portal nao duplica.
  stalker_definir_mac("00:1a:79:00:00:01");
  stalker_definir_portal("host.tv/meuportal/c/");
  nUrls = 0;
  stalker_canais(sai, 4);
  assert(nUrls >= 3);
  assert(!strncmp(urls[0], "http://host.tv/meuportal/server/load.php?", 41));
  assert(!strncmp(urls[1], "http://host.tv/meuportal/portal.php?", 36));
  assert(!strncmp(urls[2], "http://host.tv/meuportal/stalker_portal/server/load.php?", 56));
  assert(!strcmp(ultimoRef, "Referer: http://host.tv/meuportal/c/"));
  stalker_definir_portal("host.tv/stalker_portal/c/");
  nUrls = 0;
  stalker_canais(sai, 4);
  { int i; for (i = 0; i < nUrls; i++) assert(!strstr(urls[i], "stalker_portal/stalker_portal")); }
  assert(nUrls >= 2);
  assert(!strncmp(urls[0], "http://host.tv/stalker_portal/server/load.php?", 46));
  assert(!strncmp(urls[1], "http://host.tv/stalker_portal/portal.php?", 41));
  stalker_definir_portal("host.tv:8080");
  nUrls = 0;
  stalker_canais(sai, 4);
  assert(nUrls >= 3 && strstr(urls[2], "http://host.tv:8080/stalker_portal/server/load.php?") == urls[2]);
  puts("ok  rotas com prefixo, sem duplicar stalker_portal");
  puts("stalker_portal: tudo ok");
  return 0;
}
