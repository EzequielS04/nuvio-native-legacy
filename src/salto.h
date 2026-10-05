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

// ESTADO DA RAJADA e a regra de quem e toque e quem e tecla segurada.
//
// TOQUE SOLTO nunca e perdido: cada KEYDOWN novo aplica 10 s na hora e NAO
// inicia a rampa — cinco toques rapidos sao +50 s. So a REPETICAO de tecla
// segurada passa pelo limite de SALTO_INTERVALO_MS e pela rampa de tempo.
//
// Como saber que e repeticao, se cada plataforma entrega de um jeito: o
// teclado do SDL marca key.repeat; o firmware da TV manda a tecla segurada como
// KEYDOWNs SEPARADOS com repeat=0 (mesma observacao de app.c, issue #11), e a
// casca Tizen ainda despacha um par keydown+keyup por tecla, entao "descida sem
// subida" nao serve de criterio. Sobra o RELOGIO: repeticao de controle vem a
// ~100 ms (SDL de teclado, ~30 ms); o toque mais rapido de uma mao, ~150 ms.
// Logo, descida a menos de SALTO_REP_MS da anterior e repeticao. Abaixo de
// SALTO_REP_MIN_MS (dois eventos do mesmo quadro) nao existe mao nem controle:
// sao dois toques entregues juntos, e valem os dois.
#define SALTO_REP_MS      130u
#define SALTO_REP_MIN_MS   20u

typedef struct { unsigned inicio, ultimaTecla, ultimoPasso; } SaltoEst;

// `novo`: rajada nova (primeira tecla, ou a direcao mudou). `flagRepeat`:
// key.repeat do SDL. Devolve o passo em segundos (0 = ignorar a repeticao).
static inline float salto_tecla(SaltoEst *st, int novo, int flagRepeat,
                                unsigned agoraMs, float duracaoSeg) {
  unsigned gap = agoraMs - st->ultimaTecla;
  int rep = !novo && (flagRepeat || (gap >= SALTO_REP_MIN_MS && gap <= SALTO_REP_MS));
  st->ultimaTecla = agoraMs;
  if (!rep) {                       // toque: 10 s agora, rampa recomeca
    st->inicio = agoraMs; st->ultimoPasso = agoraMs;
    return SALTO_SEG;
  }
  if (agoraMs - st->ultimoPasso < SALTO_INTERVALO_MS) return 0.0f;
  st->ultimoPasso = agoraMs;
  return salto_passo(agoraMs - st->inicio, duracaoSeg);
}


// QUANDO O AVANCO TERMINA (#235). O avanco ja e "escolher e depois confirmar":
// video pausado, so a barra anda, UMA busca no fim, OK confirma na hora. O que
// faltava e o tempo de espera: 420 ms nao davam folga para olhar a miniatura
// do Seekr e acertar o ponto ("1 s de atraso antes de aplicar"). Com o Seekr
// ligado a espera e 1 s; sem miniatura nao ha o que olhar e ficam os 420 ms,
// para o filme voltar a tocar logo depois de um toque so.
#define SALTO_FIM_MS        420u
#define SALTO_FIM_SEEKR_MS 1000u
static inline unsigned salto_fim_ms(int seekrLigado) {
  return seekrLigado ? SALTO_FIM_SEEKR_MS : SALTO_FIM_MS;
}

#endif
