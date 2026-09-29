// A REGRA DO NIVEL DE GPU ADAPTATIVO (src/gpunivel.h), sem TV e sem GL.
//
// O quadro chega como main.c o entrega (dt, espera = clr+swap, cpu) e a
// regra tem de: ficar no 0 quando o FPS e bom (a Tizen 6 a 60 nunca desce);
// descer 0 -> 1 -> 2 quando a espera domina a 25 fps (o registro 8825); nao
// descer quando quem pesa e a CPU; ignorar quadros fora da home; e gravar o
// nivel alcancado. gfx e dados entram como dubles: o que se testa e a regra.
#include "gpunivel.h"
#include "gfx.h"
#include "dados.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void gpun_teste_reiniciar(void);
int  gpun_teste_decidido(void);

// --- dubles ------------------------------------------------------------------
float gfx_tex_aspect_atual, gfx_opacidade_grupo = 1.0f;
static int leves = -1;
void gfx_definir_efeitos_leves(int l) { leves = l; }
int  gfx_efeitos_leves(void) { return leves; }
void gfx_tex_esquecer(GLuint t) { (void)t; }
void gfx_tamanho_alvo(int w, int h) { (void)w; (void)h; }
void gfx_rect(GfxRect r, GLuint tex, GfxModo modo, float foco, float px, float py, float raio,
              float cr, float cg, float cb, float ca) {
  (void)r; (void)tex; (void)modo; (void)foco; (void)px; (void)py; (void)raio;
  (void)cr; (void)cg; (void)cb; (void)ca;
}
static char gravado[512];
int dados_gravar(const char *nome, const char *c) {
  assert(!strcmp(nome, "gpu-nivel.txt"));
  snprintf(gravado, sizeof gravado, "%s", c);
  return 1;
}
char *dados_ler(const char *nome) { (void)nome; return NULL; }

// `seg` segundos de quadros iguais.
static void rodar(double seg, double dt, double espera, double cpu, int naHome, int cheia) {
  double t;
  for (t = 0; t < seg * 1000.0; t += dt) gpun_medir(dt, espera, cpu, naHome, cheia);
}

int main(void) {
  // 1. Tizen 6: 60 fps (a espera e o vsync, grande, mas o FPS e bom) -> fica.
  gpun_teste_reiniciar(); gravado[0] = 0;
  rodar(12, 16.7, 12.0, 4.0, 1, 1);
  assert(gpun_nivel() == 0 && gpun_teste_decidido());
  assert(strstr(gravado, "nivel=0"));
  puts("ok  60 fps: fica no nivel 0 e grava");

  // 2. O registro 8825: 25 fps, espera 34 ms contra 5 de CPU -> 1 e PARA.
  //    O 720p (nivel 2) nunca e escolhido sozinho: no teste das duas Tizen 5.0
  //    (#180) o texto ficou borrado demais; "efeitos" ganhou.
  gpun_teste_reiniciar(); gravado[0] = 0;
  rodar(8, 40.0, 34.0, 5.0, 1, 1);          // aquece 3 s + janela de 4 s
  assert(gpun_nivel() == 1 && leves == 1 && !gpun_teste_decidido());
  assert(strstr(gravado, "nivel=1"));
  rodar(7, 40.0, 34.0, 5.0, 1, 1);          // assenta 2 s + janela de 4 s
  assert(gpun_nivel() == 1 && gpun_teste_decidido());
  assert(!strstr(gravado, "nivel=2"));
  puts("ok  25 fps presos na GPU: desce 0 -> 1, grava e para (720p so forcado)");

  // 3. Desce ate onde o FPS fica bom e para ali.
  gpun_teste_reiniciar();
  rodar(8, 40.0, 34.0, 5.0, 1, 1);
  assert(gpun_nivel() == 1);
  rodar(7, 16.7, 10.0, 4.0, 1, 1);
  assert(gpun_nivel() == 1 && gpun_teste_decidido());
  puts("ok  com o nivel 1 a 60 fps: fica no 1");

  // 4. Lento pela CPU (des domina): menos pixel nao ajuda -> nao desce.
  gpun_teste_reiniciar();
  rodar(8, 40.0, 3.0, 36.0, 1, 1);
  assert(gpun_nivel() == 0 && gpun_teste_decidido());
  puts("ok  lento pela CPU: nao desce");

  // 5. Fora da home, ou com a home vazia: nada conta.
  gpun_teste_reiniciar();
  rodar(30, 40.0, 34.0, 5.0, 0, 1);
  rodar(30, 40.0, 34.0, 5.0, 1, 0);
  assert(gpun_nivel() == 0 && !gpun_teste_decidido());
  // Entrar e sair no meio da janela zera a janela: 3 s de aquece + 3 s, sai,
  // volta: 2 s de aquece + 3 s ainda nao fecham uma janela de 4 s.
  rodar(6, 40.0, 34.0, 5.0, 1, 1);
  rodar(1, 40.0, 34.0, 5.0, 0, 1);
  rodar(5, 40.0, 34.0, 5.0, 1, 1);
  assert(gpun_nivel() == 0 && !gpun_teste_decidido());
  puts("ok  fora da home ou home vazia nao mede; sair zera a janela");

  puts("gpunivel: tudo ok");
  return 0;
}
