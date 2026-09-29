// A tabela de traducao esta ORDENADA e responde? Sao as duas unicas maneiras
// de ela falhar: fora de ordem a busca binaria erra em silencio, e chave
// ausente devolve o portugues sem avisar.
//
//   cc tests/idioma.c src/idioma.c -Isrc -I/opt/homebrew/include \
//      -o /tmp/t-idioma && /tmp/t-idioma
//
// Cobre tambem os outros idiomas (ro, uk, ru, fr, de, es): toda chave de idioma_tab.h tem de
// voltar traduzida, nao vazia, com os MESMOS marcadores printf e os mesmos \n
// que o portugues — o mesmo que tools/idiomas.py confere no texto, aqui pelo
// caminho que o app usa (i18n), que e onde um desalinhamento de tabela aparece.
// O -I do Homebrew e por causa do ajustes.h, que inclui SDL so pelo tipo de
// evento; nada de SDL e chamado por este teste.
#include "idioma.h"
#include "idiomacod.h"
#include "comentordem.h"
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
  static const int LINGUAS[] = { IDIOMA_EN, IDIOMA_RO, IDIOMA_UK, IDIOMA_RU,
                                 IDIOMA_FR, IDIOMA_DE, IDIOMA_ES };
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

  // Frances, alemao e espanhol: uma amostra com a marca de cada um (acento
  // agudo/grave, "ß"/umlaut, "¿"/"ñ") e o mes por extenso, que nesses tres nao
  // declina e sai da propria tabela.
  lg = IDIOMA_FR;
  confere("Ajustes", "Réglages");
  confere("Quem está assistindo?", "Qui regarde?");
  confere_mes(7, "julho", "juillet");
  confere_mes(8, "agosto", "août");
  lg = IDIOMA_DE;
  confere("Ajustes", "Einstellungen");
  confere_mes(3, "mar\xc3\xa7o", "März");
  lg = IDIOMA_ES;
  confere("Ajustes", "Ajustes");
  confere("Quem está assistindo?", "¿Quién está viendo?");
  confere_mes(7, "julho", "julio");

  // Mes abreviado e caixa alta: sem lixo no meio de um acento de 2 bytes.
  { char o[32];
    if (strcmp(idioma_mes_curto(IDIOMA_FR, 0), "janv.") || strcmp(idioma_mes_curto(IDIOMA_DE, 2), "März") ||
        strcmp(idioma_mes_curto(IDIOMA_ES, 8), "sep") || idioma_mes_curto(IDIOMA_ES, 12)[0]) {
      printf("FALHOU: mes curto fr/de/es\n"); falhas++; }
    idioma_maiusc(o, sizeof o, "ao\xc3\xbbt");        if (strcmp(o, "AO\xc3\x9bT"))  { printf("FALHOU: maiusc aout\n"); falhas++; }
    idioma_maiusc(o, sizeof o, "m\xc3\xa4rz");        if (strcmp(o, "M\xc3\x84RZ"))  { printf("FALHOU: maiusc marz\n"); falhas++; }
    idioma_maiusc(o, sizeof o, "c\xc5\x93ur");        if (strcmp(o, "C\xc5\x92UR"))  { printf("FALHOU: maiusc coeur\n"); falhas++; }
    idioma_maiusc(o, sizeof o, "a\xc3\x9f");          if (strcmp(o, "A\xc3\x9f"))    { printf("FALHOU: maiusc eszett\n"); falhas++; }
    idioma_maiusc(o, sizeof o, "se\xc3\xb1or");       if (strcmp(o, "SE\xc3\x91OR")) { printf("FALHOU: maiusc senor\n"); falhas++; } }

  // DETALHE DO TITULO: status, pais e duracao crus (fix/detalhe-traducao). As
  // chaves sao as que desc_status_chave / desc_pais_txt / desc_duracao_min usam.
  lg = IDIOMA_RU;
  confere("Estados Unidos", "\xd0\xa1\xd0\xa8\xd0\x90");
  confere("Em exibi\xc3\xa7\xc3\xa3o", "\xd0\x92 \xd1\x8d\xd1\x84\xd0\xb8\xd1\x80\xd0\xb5");
  confere("%dh %dmin", "%d \xd1\x87 %d \xd0\xbc\xd0\xb8\xd0\xbd");
  lg = IDIOMA_FR;
  confere("Reino Unido", "Royaume-Uni");
  confere("Finalizada", "Termin\xc3\xa9" "e");
  confere("%dh", "%d h");
  lg = IDIOMA_DE;
  confere("Coreia do Sul", "S\xc3\xbc" "dkorea");
  confere("%dmin", "%d Min.");
  lg = IDIOMA_ES;
  confere("Pa\xc3\xad" "ses Baixos", "Pa\xc3\xad" "ses Bajos");
  lg = IDIOMA_EN;
  confere("Pa\xc3\xad" "ses Baixos", "Netherlands");
  confere("Lan\xc3\xa7" "ado", "Released");

  // Codigo ISO do idioma da interface (compara com o `language` do Trakt).
  { static const char *ISO[] = { "pt", "en", "ro", "uk", "ru", "fr", "de", "es" };
    int i;
    for (i = 0; i < IDIOMA_N; i++)
      if (strcmp(idioma_iso(i), ISO[i])) { printf("FALHOU: idioma_iso(%d)\n", i); falhas++; } }

  // COMENTARIOS: os do idioma da interface primeiro, estavel; sem `language`
  // conta como do idioma (nao rebaixa o que talvez seja legivel).
  { ComentAchado v[5];
    const char *L[5] = { "en", "pt", "", "es", "pt" };
    int i, k;
    memset(v, 0, sizeof v);
    for (i = 0; i < 5; i++) { snprintf(v[i].u, sizeof v[i].u, "u%d", i); strcpy(v[i].l, L[i]); }
    k = coment_ordenar(v, 5, "pt");
    if (k != 3 || strcmp(v[0].u, "u1") || strcmp(v[1].u, "u2") || strcmp(v[2].u, "u4") ||
        strcmp(v[3].u, "u0") || strcmp(v[4].u, "u3")) {
      printf("FALHOU: coment_ordenar pt -> %s %s %s %s %s (k=%d)\n",
             v[0].u, v[1].u, v[2].u, v[3].u, v[4].u, k);
      falhas++; }
    k = coment_ordenar(v, 5, "ru");      /* ninguem e russo: so os sem idioma vem na frente */
    if (k != 1 || strcmp(v[0].u, "u2")) { printf("FALHOU: coment_ordenar ru\n"); falhas++; }
    if (!coment_mesmo_idioma("pt-br", "pt") || coment_mesmo_idioma("en", "pt")) {
      printf("FALHOU: coment_mesmo_idioma\n"); falhas++; } }

  // A tabela inteira, em cada idioma.
  varredura();
  lg = IDIOMA_EN;

  if (falhas) { printf("%d falha(s)\n", falhas); return 1; }
  printf("idioma ok\n");
  return 0;
}
