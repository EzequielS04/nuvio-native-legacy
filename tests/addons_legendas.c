// Actual production subtitle worker; offline providers and measurements only.
#include "../src/addons.c"
#include <assert.h>
static const char *responses[3];
static int requests, status[3] = {200, 200, 200};
char *rede_baixar_medido_controle(const char *url, int seconds,
                                  const char *const *headers,
                                  const RedeControle *controle,
                                  RedeMedida *medida) {
  int idx = -1;
  assert(!headers && !controle && seconds == 25);
  for (int i = 0; i < 3; i++) {
    char base[80]; snprintf(base, sizeof base, "https://fixture.invalid/provider%d/", i);
    if (!strncmp(url, base, strlen(base))) idx = i;
  }
  assert(idx >= 0 && strstr(url, "/subtitles/") && strstr(url, ".json"));
  requests++;
  *medida = (RedeMedida){ .status = status[idx], .bytes = responses[idx] ? (long)strlen(responses[idx]) : 0, .ms = 7 };
  return responses[idx] ? strdup(responses[idx]) : NULL;
}
const char *i18n(const char *text) { return text; }
static void run(int n, const char *id, const char *tipo) {
  const char *nomes[] = {"OpenSubtitles", "Subs.ro", "Community Subtitles"};
  memset(addon, 0, sizeof addon); nAddon = n;
  for (int i = 0; i < n; i++) {
    addon[i].ativo = addon[i].legenda = 1;
    snprintf(addon[i].base, sizeof addon[i].base, "https://fixture.invalid/provider%d", i);
    snprintf(addon[i].nome, sizeof addon[i].nome, "%s", nomes[i]);
  }
  snprintf(legId, sizeof legId, "%s", id); snprintf(legTipo, sizeof legTipo, "%s", tipo);
  legParar = 0; fioLegVivo = 1; buscarLegendas(NULL); assert(!fioLegVivo);
}
static void dense(char *json, size_t size, int provider) {
  const char *langs[] = {"ron", "spa", "eng", "kor"};
  snprintf(json, size, "{\"subtitles\":[");
  for (int l = 0; l < 4; l++) for (int i = 0; i < LEG_MAX + 2; i++) {
    char item[220];
    snprintf(item, sizeof item, "%s{\"lang\":\"%s\",\"url\":\"https://fixture.invalid/PRIVATE_TOKEN/%d-%d-%d.srt\"}",
             (l || i) ? "," : "", langs[l], provider, l, i);
    strncat(json, item, size - strlen(json) - 1);
  }
  strncat(json, "]}", size - strlen(json) - 1);
}
int main(void) {
  responses[0] = "{\"subtitles\":[{\"lang\":\"ron\",\"url\":\"https://fixture.invalid/ro.srt\"},{\"lang\":\"spa\",\"url\":\"https://fixture.invalid/es.srt\"},{\"lang\":\"eng\",\"url\":\"https://fixture.invalid/en.srt\"},{\"lang\":\"kor\",\"url\":\"https://fixture.invalid/ko.srt\"}]}";
  ling_conta_legenda("ro"); ling_conta_legenda2(""); ling_local_legenda("*"); run(1, "tt123", "movie");
  assert(nLegs == 4 && !strcmp(legs[0].idioma, "ron") && !strcmp(legs[3].idioma, "kor"));
  assert(!strcmp(legs[0].provedor, "OpenSubtitles"));
  ling_local_legenda(""); ling_conta_legenda(""); run(1, "tt123", "movie"); assert(nLegs == 4);
  ling_conta_legenda("ro"); ling_conta_legenda2("es"); run(1, "tt123", "movie");
  assert(nLegs == 3 && !strcmp(legs[0].idioma, "ron") && !strcmp(legs[1].idioma, "spa") && !strcmp(legs[2].idioma, "eng"));

  char primeiro[16000], segundo[16000]; dense(primeiro, sizeof primeiro, 0); dense(segundo, sizeof segundo, 1);
  responses[0] = primeiro; responses[1] = segundo;
  run(1, "tt123", "movie");
  int counts[3] = {0}; const char *langs[] = {"ron", "spa", "eng"};
  for (int i = 0; i < nLegs; i++) for (int l = 0; l < 3; l++) if (!strcmp(legs[i].idioma, langs[l])) counts[l]++;
  assert(nLegs == LEG_MAX && counts[0] == 4 && counts[1] == 4 && counts[2] == 4);

  // First provider has >12 candidates. Both must be requested and contribute,
  // with principal/secondary/English quotas and real origin attached.
  int antes = requests; run(2, "tt123", "movie"); assert(requests - antes == 2 && nLegs == LEG_MAX);
  int providers[2] = {0}; memset(counts, 0, sizeof counts);
  for (int i = 0; i < nLegs; i++) {
    assert(!strcmp(legs[i].idioma, langs[i / 4]));
    for (int j = 0; j < 2; j++) if (!strcmp(legs[i].provedor, j ? "Subs.ro" : "OpenSubtitles")) providers[j]++;
    counts[i / 4]++;
  }
  assert(providers[0] == 6 && providers[1] == 6 && counts[0] == 4 && counts[1] == 4 && counts[2] == 4);
  ling_local_legenda("*"); antes = requests; run(2, "tt123", "movie");
  assert(requests - antes == 2 && nLegs == LEG_MAX);
  for (int i = 0; i < nLegs; i++) assert(!strcmp(legs[i].provedor, (i % 2) ? "Subs.ro" : "OpenSubtitles"));

  // Empty, missing array and HTTP404 are distinguishable without blocking the
  // later provider, and malformed entries never become menu options.
  responses[0] = "{\"subtitles\":[]}";
  responses[1] = "{\"subtitles\":[{\"lang\":\"ron\",\"url\":\"https://fixture.invalid/PRIVATE_TOKEN/ro.srt\"},{\"lang\":\"spa\"},{\"url\":\"https://fixture.invalid/no-lang.srt\"}]}";
  antes = requests; run(2, "tt123", "movie");
  assert(requests - antes == 2 && nLegs == 1 && !strcmp(legs[0].provedor, "Subs.ro"));
  responses[0] = "{\"error\":\"PRIVATE_TOKEN\"}"; run(2, "tt123", "movie"); assert(nLegs == 1);
  responses[0] = NULL; status[0] = 404; run(2, "tt123", "movie"); assert(nLegs == 1);

  responses[0] = "{\"subtitles\":[{\"lang\":\"ron\",\"url\":\"https://fixture.invalid/ok.srt\",\"season\":2,\"episode\":4},{\"lang\":\"ron\",\"url\":\"https://fixture.invalid/wrong.srt\",\"season\":2,\"episode\":3},{\"lang\":\"ron\",\"url\":\"https://fixture.invalid/filename-wrong.srt\",\"subtitleFileName\":\"Show.S02E03.srt\"}]}";
  status[0] = 200; run(1, "tt123:2:4", "series");
  assert(nLegs == 1 && strstr(legs[0].url, "/ok.srt"));
  puts("addon subtitles: provider fairness, real origin, ordered languages, empty/missing/HTTP and episode checks ok");
}
