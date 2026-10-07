// INTERFACE RESOLUTION (src/resolucao.h): the migration of the old setting and
// the 4K watch, plus the disk round trip through ajustes_dir.
//
// Owner, 06/10/2026: Automatic by default, never 720p on its own; 4K only when
// picked, falling back to 1080p when the GPU does not hold it.
#include "../src/ajustes.c"
#include <assert.h>
#include <unistd.h>

static void escrever(const char *caminho, const char *texto) {
  FILE *f = fopen(caminho, "w");
  assert(f);
  fputs(texto, f);
  fclose(f);
}

static int carregar(const char *dir, const char *texto) {
  char caminho[700];
  snprintf(caminho, sizeof caminho, "%s/ajustes.txt", dir);
  escrever(caminho, texto);
  valor[AJ_RESOLUCAO] = RES_AUTO;
  ajustes_dir(dir);
  return valor[AJ_RESOLUCAO];
}

int main(void) {
  char dir[] = "/tmp/nuvio-resolucao-XXXXXX";
  ResVigia v;
  int i;
  assert(mkdtemp(dir));

  // 1. Migration: old 0 (the old default, 1080p) is Automatic; 4K and 720p
  //    were explicit and stay.
  assert(res_migrar(0) == RES_AUTO);
  assert(res_migrar(1) == RES_4K);
  assert(res_migrar(2) == RES_720);
  assert(res_migrar(-1) == RES_AUTO && res_migrar(9) == RES_AUTO);
  puts("ok  res_migrar: 1080p antigo -> Automatica, 4K e 720p ficam");

  // 2. Factory default and the list order.
  assert(OPCOES[AJ_RESOLUCAO].n == RES_N);
  assert(!strcmp(V_RESOLUCAO[RES_AUTO], "Automática"));
  assert(!strcmp(V_RESOLUCAO[RES_4K], "4K (experimental)"));
  assert(!strcmp(V_RESOLUCAO[RES_720], "720p (leve)"));
  puts("ok  lista: Automatica primeiro, padrao de fabrica");

  // 3. Disk: old key migrates; the new key wins over the old one.
  assert(carregar(dir, "resolucao_ui 0\n") == RES_AUTO && !ajustes_4k() && !ajustes_720p());
  assert(carregar(dir, "resolucao_ui 1\n") == RES_4K && ajustes_4k());
  assert(carregar(dir, "resolucao_ui 2\n") == RES_720 && ajustes_720p());
  assert(carregar(dir, "resolucao_ui 1\nresolucaoUi 1\n") == RES_1080 && !ajustes_4k());
  assert(carregar(dir, "resolucaoUi 0\nresolucao_ui 2\n") == RES_AUTO);
  assert(carregar(dir, "idioma 0\n") == RES_AUTO);
  puts("ok  ajustes.txt: resolucao_ui migra, resolucaoUi vence");

  // 4. The 4K watch: 4 bad reports in a row (GPU clock) = fallback, once.
  memset(&v, 0, sizeof v);
  for (i = 0; i < 3; i++) assert(!res_vigia_amostra(&v, 20, 55.0, 1));
  assert(res_vigia_amostra(&v, 20, 55.0, 1));
  assert(!res_vigia_amostra(&v, 20, 55.0, 1));   // only once
  puts("ok  4K: gpu 55 ms por 4 relatorios -> recua uma vez");

  // 5. A good report resets the streak; invalid ones neither count nor reset.
  memset(&v, 0, sizeof v);
  for (i = 0; i < 3; i++) assert(!res_vigia_amostra(&v, 25, 40.0, 1));
  assert(!res_vigia_amostra(&v, 60, 14.0, 1));
  for (i = 0; i < 3; i++) assert(!res_vigia_amostra(&v, 25, 40.0, 1));
  assert(!res_vigia_amostra(&v, 10, 80.0, 0));   // player open: ignored
  assert(res_vigia_amostra(&v, 25, 40.0, 1));
  puts("ok  4K: relatorio bom zera, fora da interface nao conta");

  // 6. Without the GPU clock it goes by FPS; a GPU at 25 ms (40 fps) holds.
  memset(&v, 0, sizeof v);
  for (i = 0; i < 10; i++) assert(!res_vigia_amostra(&v, 45, 0.0, 1));
  for (i = 0; i < 10; i++) assert(!res_vigia_amostra(&v, 38, 25.0, 1));
  for (i = 0; i < 3; i++) assert(!res_vigia_amostra(&v, 30, 0.0, 1));
  assert(res_vigia_amostra(&v, 30, 0.0, 1));
  puts("ok  4K: sem relogio de GPU decide pelo FPS < 40");
  return 0;
}
