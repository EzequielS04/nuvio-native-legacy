// A tabela de traducao esta ORDENADA e responde? Sao as duas unicas maneiras
// de ela falhar: fora de ordem a busca binaria erra em silencio, e chave
// ausente devolve o portugues sem avisar.
//
//   cc tests/idioma.c src/idioma.c -Isrc -I/opt/homebrew/include \
//      -o /tmp/t-idioma && /tmp/t-idioma
//
// Cobre tambem os outros idiomas (ro, uk, ru): toda chave de idioma_tab.h tem de
// voltar traduzida, nao vazia, com os MESMOS marcadores printf e os mesmos \n
// que o portugues — o mesmo que tools/idiomas.py confere no texto, aqui pelo
// caminho que o app usa (i18n), que e onde um desalinhamento de tabela aparece.
// O -I do Homebrew e por causa do ajustes.h, que inclui SDL so pelo tipo de
// evento; nada de SDL e chamado por este teste.
#include "idioma.h"
#include "idiomacod.h"
#include <stdio.h>
#include <string.h>

// Dubles: o teste nao sobe ajustes.c inteiro so para saber o idioma.
static int lg = IDIOMA_EN;
int ajustes_idioma(void) { return lg; }
int ajustes_idioma_ingles(void) { return lg == IDIOMA_EN; }

// As chaves, para varrer a tabela inteira sem tocar em idioma.c.
static const struct { const char *pt, *en; } CHAVES[] = {
#include "idioma_tab.h"
};

static int falhas;
static void confere(const char *pt, const char *esperado) {
  const char *r = i18n(pt);
  if (strcmp(r, esperado) != 0) {
    printf("FALHOU: \"%s\" -> \"%s\" (esperava \"%s\")\n", pt, r, esperado);
    falhas++;
  }
}

static void confere_mes(int mes, const char *pt, const char *esperado) {
  const char *r = idioma_mes_data(mes, pt);
  if (strcmp(r, esperado) != 0) {
    printf("FALHOU: mes %d (\"%s\") no idioma %d -> \"%s\" (esperava \"%s\")\n", mes, pt, lg, r, esperado);
    falhas++;
  }
}

// Marcadores printf de `s`, na ordem, um por linha de saida ("%d%s%.1f..."), sem
// os "%%". Comparados como texto: "%d" contra "%s" e diferente.
static void marcadores(const char *s, char *out, size_t cap) {
  size_t o = 0;
  while (*s) {
    const char *p;
    if (*s != '%') { s++; continue; }
    if (s[1] == '%') { s += 2; continue; }
    // flags, largura, precisao e tamanho; so conta se terminar numa conversao
    // (um "50%" solto no fim de "Escuro 50%" nao e marcador).
    p = s + 1;
    while (*p && strchr("-+ #0123456789.hlzjt", *p)) p++;
    if (*p && strchr("diouxXeEfgGcsp", *p) && o + (size_t)(p - s) + 3 < cap) {
      memcpy(out + o, s, (size_t)(p - s) + 1); o += (size_t)(p - s) + 1; out[o++] = ';';
      s = p + 1;
    } else s++;
  }
  out[o] = 0;
}
static int quebras(const char *s) { int n = 0; for (; *s; s++) if (*s == '\n') n++; return n; }

static void varredura(void) {
  static const int LINGUAS[] = { IDIOMA_EN, IDIOMA_RO, IDIOMA_UK, IDIOMA_RU };
  size_t i, k, n = sizeof CHAVES / sizeof *CHAVES;
  for (k = 0; k < sizeof LINGUAS / sizeof *LINGUAS; k++) {
    lg = LINGUAS[k];
    for (i = 0; i < n; i++) {
      const char *r = i18n(CHAVES[i].pt);
      char mk[128], mr[128];
      if (!r || !*r) { printf("FALHOU: idioma %d sem traducao de \"%s\"\n", lg, CHAVES[i].pt); falhas++; continue; }
      if (lg == IDIOMA_EN && strcmp(r, CHAVES[i].en) != 0) {
        printf("FALHOU: en de \"%s\" -> \"%s\"\n", CHAVES[i].pt, r); falhas++; }
      marcadores(CHAVES[i].pt, mk, sizeof mk); marcadores(r, mr, sizeof mr);
      if (strcmp(mk, mr) != 0 || quebras(CHAVES[i].pt) != quebras(r)) {
        printf("FALHOU: marcadores/quebras de \"%s\" no idioma %d: \"%s\"\n", CHAVES[i].pt, lg, r); falhas++; }
      if (falhas > 30) return;
    }
  }
}

int main(void) {
  // Uma de cada tela, incluindo as que tem acento no meio e aspas escapadas.
  confere("Continuar assistindo", "Continue Watching");
  confere("Ajustes", "Settings");
  confere("Quem está assistindo?", "Who's watching?");
  confere("Nenhuma fonte direta disponível. Use Recarregar para tentar novamente.",
          "No direct source available. Use Reload to try again.");
  confere("Ficção científica", "Science Fiction");
  confere("Sáb", "Sat");
  confere("Mostrar \"Continuar assistindo\"", "Show \"Continue Watching\"");

  // Os selos da guia parental: nao sao literais no ponto de desenho (vem de
  // parental_rotulo/parental_gravidade), entao a varredura estatica nao os
  // cobre. Ficaram em portugues ate a v1.0.35 por isso.
  confere("Nudez", "Nudity");
  confere("Violência", "Violence");
  confere("Leve", "Mild");
  confere("Moderado", "Moderate");
  confere("Severo", "Severe");

  // Legendas montadas com %s: a frase pronta nunca casa com chave, entao a
  // parte em portugues tem de ser traduzida ANTES de entrar no formato.
  confere("Seleção do seu catálogo", "A selection from your catalog");
  confere("Conhecido por  %s", "Known for  %s");

  // Sem chave: devolve o proprio texto. E o caso de todo titulo de filme.
  confere("Blade Runner 2049", "Blade Runner 2049");
  confere("", "");

  // Em portugues NADA e traduzido, nem o que esta na tabela.
  lg = IDIOMA_PT;
  confere("Ajustes", "Ajustes");
  confere("Continuar assistindo", "Continuar assistindo");

  // Um de cada idioma novo, com a marca de cada escrita: diacriticos do romeno
  // (virgula embaixo, nao cedilha), cirilico ucraniano (і, ї, є) e russo (ы, э).
  lg = IDIOMA_RO;
  confere("Ajustes", "Setări");
  confere("Continuar assistindo", "Continuă vizionarea");
  confere("Sáb", "Sâm");
  lg = IDIOMA_UK;
  confere("Ajustes", "Налаштування");
  confere("Continuar assistindo", "Продовжити перегляд");
  lg = IDIOMA_RU;
  confere("Ajustes", "Настройки");
  confere("Continuar assistindo", "Продолжить просмотр");
  confere("Blade Runner 2049", "Blade Runner 2049");

  // Trocar de idioma com o cache quente devolve o idioma NOVO, nao o antigo:
  // o cache guarda so o indice da chave, e o idioma e lido a cada chamada.
  lg = IDIOMA_EN; confere("Ajustes", "Settings");
  lg = IDIOMA_RU; confere("Ajustes", "Настройки");
  lg = IDIOMA_EN; confere("Ajustes", "Settings");

  // Mes de uma data por extenso: russo e ucraniano declinam, o resto nao.
  lg = IDIOMA_PT; confere_mes(7, "julho", "julho");
  lg = IDIOMA_EN; confere_mes(7, "julho", "July");
  lg = IDIOMA_RO; confere_mes(7, "julho", "iulie");
  lg = IDIOMA_UK; confere_mes(7, "julho", "липня");
  lg = IDIOMA_RU; confere_mes(7, "julho", "июля");
  lg = IDIOMA_RU; confere_mes(3, "mar\xc3\xa7o", "марта");
  lg = IDIOMA_RU; confere_mes(13, "x", "x");   /* fora de 1..12: cai na tabela */

  // A tabela inteira, em cada idioma.
  varredura();
  lg = IDIOMA_EN;

  if (falhas) { printf("%d falha(s)\n", falhas); return 1; }
  printf("idioma ok\n");
  return 0;
}
