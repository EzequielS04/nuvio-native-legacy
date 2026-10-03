/* Local policy and resource paths; no display, account, or network needed. */
#include "iconeapp.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int selected;
static const char *directory = "";
static const char *override;
int ajustes_icone_app(void) { return selected; }
const char *dados_dir(void) { return directory; }
char *dados_ler(const char *name) {
  assert(!strcmp(name, "apoiador.txt"));
  return override ? strdup(override) : NULL;
}
GLuint tex_obter_larg(const char *p, float w) { (void)p; (void)w; return 0; }
void gfx_cor(GfxRect r,float a,float b,float c,float d,float e) {
  (void)r;(void)a;(void)b;(void)c;(void)d;(void)e;
}
void gfx_rect(GfxRect r,GLuint t,GfxModo m,float a,float b,float c,float d,float e,float f,float g,float h) {
  (void)r;(void)t;(void)m;(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;
}
int main(void) {
  unsetenv("NUVIO_APOIADOR"); selected = 3;
  assert(iconeapp_atual() == 0);
  directory = "/local-test"; override = "1";
  assert(iconeapp_atual() == 3); /* Early no-data query must not cache denial. */
  override = NULL; apoiador_reler(); assert(iconeapp_atual() == 0);
  setenv("NUVIO_APOIADOR", "1", 1); apoiador_reler();
  assert(iconeapp_atual() == 3);
  selected = -1; assert(iconeapp_atual() == 0);
  selected = ICONEAPP_N; assert(iconeapp_atual() == 0);
  for (int i=0;i<ICONEAPP_N;i++) assert(iconeapp_id(i)[0]);
  assert(!iconeapp_id(-1)[0] && !iconeapp_id(ICONEAPP_N)[0]);
  iconeapp_iniciar("/art/first");
  assert(!strcmp(iconeapp_caminho(3),"/art/first/icones-app/tvlaranja.png"));
  iconeapp_iniciar("/art/second");
  assert(!strcmp(iconeapp_caminho(3),"/art/second/icones-app/tvlaranja.png"));
  assert(!iconeapp_caminho(-1)[0]);
  puts("PASS: app icon local gate, bounds, and resource directory refresh");
}
