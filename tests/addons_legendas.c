// Actual production subtitle worker; offline addon responses only.
#include "../src/addons.c"
#include <assert.h>
static const char *response;
static int requests;
char *rede_baixar(const char *url,int seconds) {
  assert(strstr(url,"https://fixture.invalid/subtitles/movie/tt123.json"));
  assert(seconds==25);requests++;return strdup(response);
}
const char *i18n(const char *text) { return text; }
static void run(void) {
  memset(addon,0,sizeof addon);nAddon=1;
  addon[0].ativo=addon[0].legenda=1;
  snprintf(addon[0].base,sizeof addon[0].base,"https://fixture.invalid");
  snprintf(legId,sizeof legId,"tt123");snprintf(legTipo,sizeof legTipo,"movie");
  legParar=0;fioLegVivo=1;buscarLegendas(NULL);assert(!fioLegVivo);
}
static void mixed(void) {
  response="{\"subtitles\":[{\"lang\":\"ron\",\"url\":\"https://fixture.invalid/ro.srt\"},{\"lang\":\"spa\",\"url\":\"https://fixture.invalid/es.srt\"},{\"lang\":\"eng\",\"url\":\"https://fixture.invalid/en.srt\"},{\"lang\":\"kor\",\"url\":\"https://fixture.invalid/ko.srt\"}]}";
}
int main(void) {
  mixed();ling_conta_legenda("ro");ling_conta_legenda2("");ling_local_legenda("*");run();
  assert(nLegs==4);assert(!strcmp(legs[0].idioma,"ron"));assert(!strcmp(legs[3].idioma,"kor"));
  ling_local_legenda("");ling_conta_legenda("");run();assert(nLegs==4);
  ling_conta_legenda("ro");ling_conta_legenda2("es");run();
  assert(nLegs==3);assert(!strcmp(legs[0].idioma,"ron"));assert(!strcmp(legs[1].idioma,"spa"));assert(!strcmp(legs[2].idioma,"eng"));
  // Each preferred group retains its share, including the English fallback.
  char json[16000]="{\"subtitles\":[";
  const char *langs[]={"ron","spa","eng","kor"};
  for(int l=0;l<4;l++)for(int i=0;i<LEG_MAX;i++) {
    char item[160];snprintf(item,sizeof item,"%s{\"lang\":\"%s\",\"url\":\"https://fixture.invalid/%d-%d.srt\"}",(l||i)?",":"",langs[l],l,i);
    strncat(json,item,sizeof json-strlen(json)-1);
  }
  strncat(json,"]}",sizeof json-strlen(json)-1);response=json;run();
  int counts[4]={0};for(int i=0;i<nLegs;i++)for(int l=0;l<4;l++)if(!strcmp(legs[i].idioma,langs[l]))counts[l]++;
  assert(nLegs==3*(LEG_MAX/3));assert(counts[0]==LEG_MAX/3&&counts[1]==LEG_MAX/3&&counts[2]==LEG_MAX/3&&!counts[3]);
  ling_local_legenda("*");run();assert(nLegs==LEG_MAX);
  assert(requests==5);puts("addon subtitles: All, absent preference, Romanian ISO, ordered preferences and bounded shares ok");
}
