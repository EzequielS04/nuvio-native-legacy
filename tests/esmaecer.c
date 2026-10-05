// Protecao de OLED (esmaecer.h): estagios por tempo, tecla que acorda e e
// consumida, nunca durante a reproducao, e o fator de brilho do OSD.
#define ESMAECER_SEM_GFX
#include "../src/esmaecer.c"
#include <assert.h>
#include <stdio.h>

#define MIN(n) ((unsigned)(n) * 60u * 1000u)

static void quadros(unsigned de, unsigned ate, int reproduzindo) {
  unsigned t;
  for (t = de; t <= ate; t += 100) esmaecer_quadro(t, 0.1f, reproduzindo);
}

int main(void) {
  // Tempos -> estagio (5 min: veu aos 5, escuro 3 min depois).
  assert(esmaecer_estagio_para(MIN(4) + 59000u, 2) == ESM_ACESO);
  assert(esmaecer_estagio_para(MIN(5), 2) == ESM_VEU);
  assert(esmaecer_estagio_para(MIN(8) - 1u, 2) == ESM_VEU);
  assert(esmaecer_estagio_para(MIN(8), 2) == ESM_ESCURO);
  assert(esmaecer_estagio_para(MIN(2), 1) == ESM_VEU && esmaecer_estagio_para(MIN(10), 3) == ESM_VEU);
  assert(esmaecer_estagio_para(MIN(1000), 0) == ESM_ACESO);   // desligado
  assert(esmaecer_estagio_para(MIN(1000), 9) == ESM_ACESO);   // escolha invalida

  // Ocioso: acende, esmaece em ~1,5 s ate 60%, depois a 92%.
  esmaecer_reiniciar(); esmaecer_escolha(2);
  esmaecer_quadro(0, 0.1f, 0);
  quadros(100, MIN(5) - 100, 0);
  assert(esmaecer_estagio() == ESM_ACESO && esmaecer_veu() == 0.0f);
  quadros(MIN(5), MIN(5) + 700, 0);
  assert(esmaecer_estagio() == ESM_VEU && esmaecer_veu() > 0.0f && esmaecer_veu() < ESM_ALFA_VEU);
  quadros(MIN(5) + 800, MIN(5) + 2000, 0);
  assert(esmaecer_veu() == ESM_ALFA_VEU && !esmaecer_apagado());
  quadros(MIN(5) + 2100, MIN(8) + 6000, 0);
  assert(esmaecer_estagio() == ESM_ESCURO && esmaecer_apagado() && esmaecer_veu() == ESM_ALFA_ESCURO);
  { int esc = -1; assert(esmaecer_mudou_escuro(&esc) && esc == 1 && !esmaecer_mudou_escuro(&esc)); }

  // A primeira tecla acorda na hora e e consumida; a repeticao logo depois
  // tambem; depois de 300 ms passa; tecla solta/movimento acorda mas nao e engolido.
  { unsigned t = MIN(8) + 7000u; int esc = -1;
    assert(esmaecer_entrada(t, 1) == 1);
    assert(esmaecer_estagio() == ESM_ACESO && esmaecer_veu() == 0.0f);
    assert(esmaecer_mudou_escuro(&esc) && esc == 0);
    assert(esmaecer_entrada(t + 100, 1) == 1);
    assert(esmaecer_entrada(t + 400, 1) == 0);
    esmaecer_quadro(t + 500, 0.1f, 0);
    assert(esmaecer_estagio() == ESM_ACESO);
    // O relogio recomeca da tecla: de novo 5 min ate o veu.
    quadros(t + 600, t + 400 + MIN(5) - 200, 0);
    assert(esmaecer_estagio() == ESM_ACESO);
    quadros(t + 400 + MIN(5) - 100, t + 400 + MIN(5) + 300, 0);
    assert(esmaecer_estagio() == ESM_VEU);
    assert(esmaecer_entrada(t + 400 + MIN(5) + 400, 0) == 0);   // movimento: acorda, nao engole
    assert(esmaecer_estagio() == ESM_ACESO);
  }

  // NUNCA durante a reproducao: tocando por 30 minutos sem tecla nao esmaece.
  esmaecer_reiniciar(); esmaecer_escolha(1);
  { unsigned t;
    for (t = 0; t <= MIN(30); t += 1000) esmaecer_quadro(t, 1.0f, 1);
    assert(esmaecer_estagio() == ESM_ACESO && esmaecer_veu() == 0.0f);
    // Pausou: os minutos contam da pausa, nao do ultimo toque.
    quadros(MIN(30) + 100, MIN(30) + MIN(2) - 200, 0);
    assert(esmaecer_estagio() == ESM_ACESO);
    quadros(MIN(30) + MIN(2) - 100, MIN(30) + MIN(2) + 2500, 0);
    assert(esmaecer_estagio() == ESM_VEU && esmaecer_veu() == ESM_ALFA_VEU);
    // Voltou a tocar com o veu ligado (ex.: proximo episodio): some na hora.
    esmaecer_quadro(MIN(30) + MIN(2) + 2600, 0.1f, 1);
    assert(esmaecer_estagio() == ESM_ACESO && esmaecer_veu() == 0.0f);
  }

  // Desligado: nada esmaece, nem tecla e engolida.
  esmaecer_reiniciar(); esmaecer_escolha(0);
  quadros(0, MIN(60), 0);
  assert(esmaecer_estagio() == ESM_ACESO && esmaecer_entrada(MIN(60) + 1000u, 1) == 0);

  // Brilho do OSD: bases, degrau automatico so tocando, rampa suave, piso.
  assert(esmaecer_brilho_base(0) == 1.0f && esmaecer_brilho_base(1) == 0.80f);
  assert(esmaecer_brilho_base(2) == 0.65f && esmaecer_brilho_base(3) == 0.50f);
  assert(esmaecer_brilho_base(7) == 0.80f);
  assert(esmaecer_brilho_osd(1, 0, 1) == 0.80f && esmaecer_brilho_osd(1, BRILHO_AUTO_MS, 1) == 0.80f);
  { float meio = esmaecer_brilho_osd(1, BRILHO_AUTO_MS + BRILHO_AUTO_RAMPA / 2, 1);
    float fim = esmaecer_brilho_osd(1, 60000u, 1);
    assert(meio < 0.80f && meio > fim);
    assert(fim > 0.679f && fim < 0.681f);   // 0,80 * 0,85
  }
  assert(esmaecer_brilho_osd(1, 60000u, 0) == 0.80f);   // pausado: sem degrau automatico
  assert(esmaecer_brilho_osd(0, 60000u, 1) > 0.849f && esmaecer_brilho_osd(0, 60000u, 1) < 0.851f);
  assert(esmaecer_brilho_osd(3, 60000u, 1) >= 0.40f);   // piso de legibilidade
  puts("esmaecer: estagios, tecla consumida, nunca tocando e brilho do OSD ok");
  return 0;
}
