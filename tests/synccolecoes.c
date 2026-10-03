// Exercise sync.c's actual network/copy publication with the actual collection
// parser and cache. Reuse the account/profile/app doubles from syncordem.
#define main syncordem_main
#define sessao_rpc syncordem_rpc
#define sessao_tabela syncordem_tabela
#define col_definir_json syncordem_col_stub
#include "syncordem.c"
#undef main
#undef sessao_rpc
#undef sessao_tabela
#undef col_definir_json
#include "colecoes.h"
#include "contacache.h"
#include <assert.h>

static int collectionsStatus = 200, outageBeforeCollections;
static const char *payload;
static const char *object = "{\"collections_json\":{\"collections\":[{\"id\":\"account-group\",\"title\":\"Account\",\"folders\":[{\"id\":\"remote-folder\",\"title\":\"Remote\",\"sources\":[{\"addonBaseUrl\":\"https://fixture.example\",\"type\":\"movie\",\"catalogId\":\"movies\"}]}]}]}}";
const char *addons_base_por_id(const char *id) { (void)id; return ""; }
char *sessao_rpc(const char *funcao, const char *corpo, int *status) {
  if (!strcmp(funcao, "sync_pull_collections")) {
    *status = collectionsStatus;
    return payload ? strdup(payload) : NULL;
  }
  return syncordem_rpc(funcao, corpo, status);
}
char *sessao_tabela(const char *table, const char *query, int *status) {
  if (outageBeforeCollections && !strcmp(table, "addons")) {
    *status = 503; return strdup("{\"error\":\"unavailable\"}");
  }
  return syncordem_tabela(table, query, status);
}
static void folder(const char *id) {
  assert(col_n() == 1 && col_folder(0));
  assert(!strcmp(col_folder(0)->id, id));
}
int main(int argc, char **argv) {
  assert(argc == 2);
  escolher(2);
  payload = object;
  if (!strcmp(argv[1], "network")) {
    ciclo();
    folder("remote-folder");
    char *copy = contacache_ler(CC_COLECOES, 2, sessao_usuario(), NULL);
    assert(copy && !strcmp(copy, object)); free(copy);
    assert(!contacache_ler(CC_COLECOES, 1, sessao_usuario(), NULL));
    int before = remontagens;
    payload = "{\"collections_json\":{\"collections\":[]}}";
    ciclo(); folder("remote-folder");
    // Empty/invalid payload retains the current collections and does not
    // request collection-only reconstruction (catalog-order remains unchanged).
    assert(remontagens == before);
    payload = "{\"collections_json\":null}";
    ciclo(); folder("remote-folder"); assert(remontagens == before);
  } else if (!strcmp(argv[1], "copy") || !strcmp(argv[1], "fallback")) {
    assert(contacache_gravar(CC_COLECOES, 2, sessao_usuario(), object));
    collectionsStatus = 503; payload = NULL;
    outageBeforeCollections = !strcmp(argv[1], "copy");
    ciclo(); folder("remote-folder");
    assert(sync_servidor_fora() == 503);
  } else if (!strcmp(argv[1], "isolated")) {
    assert(contacache_gravar(CC_COLECOES, 1, sessao_usuario(), object));
    assert(col_definir_json(object) == 1);
    col_esquecer_perfil(); assert(col_n() == 0);
    collectionsStatus = 503; payload = NULL; outageBeforeCollections = 1;
    ciclo(); assert(col_n() == 0);
  } else if (!strcmp(argv[1], "no-copy")) {
    assert(col_definir_json(object) == 1);
    collectionsStatus = 503; payload = NULL;
    ciclo(); folder("remote-folder");
  } else assert(0);
  puts("synccolecoes: ok");
  return 0;
}
