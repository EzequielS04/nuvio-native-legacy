// Regressao do parser ASS/SSA (#92). Sem SDL, sem rede: le as fixtures de
// tests/fixtures/ass e confere o que a tela vai receber por legenda_cues.
#include "legenda.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Stub de rede.h: legenda.c chama rede_baixar no fio de download, que este
// teste nunca dispara.
char *rede_baixar(const char *url, int segundos) { (void)url; (void)segundos; return NULL; }
char *rede_baixar_bin(const char *url, int segundos, long *n) { (void)url; (void)segundos; (void)n; return NULL; }

static int falhas;
#define OK(cond, ...) do { if (cond) printf("ok   " __VA_ARGS__); else { printf("FALHA " __VA_ARGS__); falhas++; } printf("\n"); } while (0)

static char *ler(const char *nome) {
  char caminho[400]; FILE *f; long n; char *s;
  snprintf(caminho, sizeof caminho, "tests/fixtures/ass/%s", nome);
  f = fopen(caminho, "rb"); if (!f) { printf("FALHA abrir %s\n", caminho); exit(1); }
  fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET);
  s = malloc((size_t)n + 1); if (fread(s, 1, (size_t)n, f) != (size_t)n) exit(1); s[n] = 0; fclose(f);
  return s;
}
static int semTagCrua(const LegendaCue *v, int n) {
  int i; for (i = 0; i < n; i++) if (strchr(v[i].texto, '{') || strchr(v[i].texto, '\\')) return 0;
  return 1;
}

int main(void) {
  char *s; LegendaCue *v; int n;

  s = ler("simples.ass");
  OK(legenda_eh_ass(s), "simples: reconhecido como ASS");
  OK(!legenda_eh_ass("1\n00:00:01,000 --> 00:00:02,000\noi\n"), "SRT nao e ASS");
  n = legenda_extrair_ass(s, &v);
  OK(n == 3, "simples: 3 Dialogue, Comment fora (n=%d)", n);
  OK(n == 3 && !strcmp(v[0].texto, "Primeira fala"), "simples: texto do primeiro");
  OK(n == 3 && !strcmp(v[1].texto, "Segunda\nlinha dois"), "simples: \\N vira quebra de linha");
  OK(n == 3 && v[0].inicio == 1.0 && v[0].fim == 3.0, "simples: tempos");
  OK(n == 3 && v[0].cor == 0xFFFFFF && v[2].cor == 0x4080FF, "simples: cor do estilo (&HBBGGRR -> RGB): Default branco, Narrador laranja (c0=%x c2=%x)", n==3?v[0].cor:0, n==3?v[2].cor:0);
  OK(n == 3 && v[2].italico == 1 && v[0].italico == 0, "simples: italico do estilo");
  OK(n == 3 && v[0].resX == 1280 && v[0].resY == 720, "simples: PlayRes");
  OK(semTagCrua(v, n), "simples: nenhuma tag crua");
  free(v); free(s);

  s = ler("posicionado.ass");
  n = legenda_extrair_ass(s, &v);
  OK(n == 4, "posicionado: 4 Dialogue (n=%d)", n);
  OK(n == 4 && v[1].an == 8, "posicionado: \\an8 (an=%d)", n==4?v[1].an:0);
  OK(n == 4 && v[1].negrito == 1 && v[1].cor == 0xFFFF00, "posicionado: estilo Letreiro negrito amarelo (cor=%x)", n==4?v[1].cor:0);
  OK(n == 4 && v[2].posX == 640 && v[2].posY == 100, "posicionado: \\pos (%.0f,%.0f)", n==4?v[2].posX:0, n==4?v[2].posY:0);
  OK(n == 4 && v[2].cor == 0xFF0000, "posicionado: \\c&H0000FF& = vermelho (cor=%x)", n==4?v[2].cor:0);
  OK(n == 4 && !strcmp(v[2].texto, "Texto posicionado"), "posicionado: texto limpo");
  OK(n == 4 && !strcmp(v[3].texto, "Grito normal sussurro"), "posicionado: \\b/\\i tirados do texto (%s)", n==4?v[3].texto:"");
  OK(semTagCrua(v, n), "posicionado: nenhuma tag crua");
  free(v); free(s);

  s = ler("karaoke.ass");
  n = legenda_extrair_ass(s, &v);
  OK(n == 3, "karaoke: 3 Dialogue (n=%d)", n);
  OK(n == 3 && !strcmp(v[0].texto, "Karaoke"), "karaoke: \\k vira texto corrido (%s)", n==3?v[0].texto:"");
  OK(n == 3 && !strcmp(v[1].texto, "Com transformacao"), "karaoke: \\t e \\fad tirados (%s)", n==3?v[1].texto:"");
  OK(n == 3 && v[2].an == 5, "karaoke: \\an5");
  OK(semTagCrua(v, n), "karaoke: nenhuma tag crua");
  free(v); free(s);

  // legenda_extrair reconhece sozinho; SRT continua entrando por ele.
  n = legenda_extrair("1\n00:00:01,000 --> 00:00:02,000\noi\n\n", &v);
  OK(n == 1 && !strcmp(v[0].texto, "oi") && v[0].an == 0 && v[0].cor == -1, "SRT por legenda_extrair: 1 bloco, sem ancora nem cor");
  free(v);

  // VTT compartilha o caminho SRT: cabecalho WEBVTT, identificador opcional e
  // settings depois do fim nao podem virar parte do timestamp.
  n = legenda_extrair("WEBVTT\n\nintro\n00:00:01.000 --> 00:00:02.500 align:start\nola &amp; mundo\n\n", &v);
  OK(n == 1 && !strcmp(v[0].texto, "ola & mundo") && v[0].inicio == 1.0 && v[0].fim == 2.5,
     "VTT: 1 cue, settings e entidades preservados");
  free(v);

  // SSA reduzido sem secoes e com caixa/recuo diferentes ainda e reconhecido
  // como Dialogue, em vez de ser descartado pelo detector ASS.
  n = legenda_extrair("  dialogue: 0,0:00:03.00,0:00:04.00,Default,,0,0,0,,SSA reduzido\n", &v);
  OK(n == 1 && !strcmp(v[0].texto, "SSA reduzido") && legenda_eh_ass("dialogue: 0,0:00:03.00,0:00:04.00,Default,,0,0,0,,x\n"),
     "SSA reduzido: detector case-insensitive e com recuo");
  free(v);

  // --- CHARSET: o texto chega ao parser em UTF-8, venha como vier -----------
  { const char *cs; char *u;
    // "Não, você está enganado." em Windows-1252
    static const char c1252[] = "1\r\n00:00:01,000 --> 00:00:02,000\r\nN\xE3o, voc\xEA est\xE1 enganado\x85 \x93sim\x94\r\n";
    u = legenda_utf8(c1252, (long)strlen(c1252), &cs);
    OK(u && !strcmp(cs, "windows-1252") && strstr(u, "Não, você está enganado… “sim”"), "cp1252 vira UTF-8 (%s)", cs);
    n = u ? legenda_extrair(u, &v) : 0;
    OK(n == 1 && strstr(v[0].texto, "você"), "cp1252: o bloco sai com acento");
    if (n) free(v);
    free(u);
    // "Привет, как дела" em Windows-1251
    static const char c1251[] = "\xCF\xF0\xE8\xE2\xE5\xF2, \xEA\xE0\xEA \xE4\xE5\xEB\xE0";
    u = legenda_utf8(c1251, (long)strlen(c1251), &cs);
    OK(u && !strcmp(cs, "windows-1251") && !strcmp(u, "Привет, как дела"), "cp1251 cirilico (%s: %s)", cs, u ? u : "-");
    free(u);
    // UTF-8 com BOM: tira o BOM, nao mexe no resto
    static const char bom[] = "\xEF\xBB\xBFol\xC3\xA1";
    u = legenda_utf8(bom, (long)strlen(bom), &cs);
    OK(u && !strcmp(cs, "utf-8") && !strcmp(u, "olá"), "utf-8 com BOM");
    free(u);
    // UTF-16LE com BOM e zeros no meio
    static const char u16[] = { (char)0xFF, (char)0xFE, 'o',0, 'l',0, (char)0xE1,0, '\n',0 };
    u = legenda_utf8(u16, (long)sizeof u16, &cs);
    OK(u && !strcmp(cs, "utf-16le") && !strcmp(u, "olá\n"), "utf-16le");
    free(u);
    static const char u16be[] = { (char)0xFE, (char)0xFF, 0,'o', 0,'k', (char)0xD8,(char)0x3D,(char)0xDE,(char)0x00 };
    u = legenda_utf8(u16be, (long)sizeof u16be, &cs);
    OK(u && !strcmp(cs, "utf-16be") && !strcmp(u, "ok\xF0\x9F\x98\x80"), "utf-16be com par substituto");
    free(u);
    // Latino com UMA palavra de dois acentos seguidos nao vira cirilico
    static const char lat[] = "Cora\xE7\xE3o, n\xE3o, ent\xE3o, ma\xE7\xE3, a\xE7\xE3o, p\xE3o, m\xE3" "e, s\xE3o, irm\xE3";
    u = legenda_utf8(lat, (long)strlen(lat), &cs);
    OK(u && !strcmp(cs, "windows-1252") && strstr(u, "Coração"), "portugues com 'çã' fica 1252 (%s)", cs);
    free(u); }

  printf(falhas ? "legenda_ass: %d falha(s)\n" : "legenda_ass: tudo ok\n", falhas);
  return falhas ? 1 : 0;
}
