// Regra do AVANCO SEGURADO (setas esquerda/direita do player), sem SDL nem
// estado global para poder ser testada com um fluxo de teclas simulado.
//
// O QUE ESTAVA ERRADO: o passo crescia por REPETICAO da tecla. O controle da
// C9 repete a cada ~100 ms, entao em 0,6 s cada repeticao ja valia 30 s e em
// 2,6 s valia 120 s: ~20 minutos por segundo. "As vezes voce quer pular so um
// pedacinho e ele ja voa pro final." E o ritmo de repeticao muda de TV e de
// controle (Samsung, Android), entao o mesmo gesto dava distancias diferentes.
//
// AGORA O RELOGIO MANDA, nao a contagem de teclas:
//  - o degrau vem do TEMPO desde a primeira tecla da rajada;
//  - no maximo um passo a cada SALTO_INTERVALO_MS; repeticoes extras entre
//    dois passos sao IGNORADAS (nao acumulam). 100 ms ou 40 ms de repeticao
//    dao a mesma distancia.
// Toque unico continua sendo exatamente 10 s.
#ifndef NUVIO_SALTO_H
#define NUVIO_SALTO_H

#define SALTO_SEG           10.0f
#define SALTO_INTERVALO_MS  300u     // ~3 passos por segundo, qualquer repeticao
#define SALTO_T1_MS        1500u     // ate aqui: 10 s por passo
#define SALTO_T2_MS        4000u     // ate aqui: 30 s
#define SALTO_T3_MS        8000u     // ate aqui: 60 s; depois, o teto
#define SALTO_TETO_SEG      120.0f
// Episodio curto: 120 s por passo e quase um quinto do episodio. Abaixo de 40
// min o teto cai para 60 s.
#define SALTO_CURTO_SEG     (40.0f * 60.0f)
#define SALTO_TETO_CURTO    60.0f

// Passo (segundos) para quem esta com a tecla ha `heldMs` numa midia de
// `duracaoSeg`.
static inline float salto_passo(unsigned heldMs, float duracaoSeg) {
  float teto = (duracaoSeg > 0.0f && duracaoSeg < SALTO_CURTO_SEG)
             ? SALTO_TETO_CURTO : SALTO_TETO_SEG;
  float p = SALTO_SEG;
  if      (heldMs >= SALTO_T3_MS) p = SALTO_SEG * 12.0f;
  else if (heldMs >= SALTO_T2_MS) p = SALTO_SEG * 6.0f;
  else if (heldMs >= SALTO_T1_MS) p = SALTO_SEG * 3.0f;
  return p > teto ? teto : p;
}

// Decide se uma tecla em `agoraMs` aplica um passo. `inicioMs`/`ultimoMs` sao
// o estado da rajada; `novo` = primeira tecla (ou direcao mudou). Devolve o
// passo (0 = ignorar a repeticao).
static inline float salto_tecla(int novo, unsigned agoraMs, unsigned *inicioMs,
                                unsigned *ultimoMs, float duracaoSeg) {
  if (novo) { *inicioMs = agoraMs; *ultimoMs = agoraMs; return SALTO_SEG; }
  if (agoraMs - *ultimoMs < SALTO_INTERVALO_MS) return 0.0f;
  *ultimoMs = agoraMs;
  return salto_passo(agoraMs - *inicioMs, duracaoSeg);
}

#endif
