// Parser isolado: permite ASan sem carregar o inicializador de SDL do macOS.
#include "streams.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void) {
  char json[32000];size_t n=0;
  n+=snprintf(json+n,sizeof json-n,"{\"streams\":[");
  for(int i=0;i<100;i++) n+=snprintf(json+n,sizeof json-n,
    "%s{\"url\":\"https://example.invalid/%d\",\"behaviorHints\":{\"filename\":\"title.%s\"}}",
    i?",":"",i,i==99?"2160p.DV.Atmos.mp4":"1080p.DVDRip.mkv");
  snprintf(json+n,sizeof json-n,"]}");
  Stream *v=NULL;int count=stream_extrair(json,"fixture",&v);
  assert(count==100 && v[99].mp4 && v[99].dolbyVision && v[99].altura==2160);
  assert(!v[0].dolbyVision);free(v);
  count=stream_extrair("{\"streams\":[]}","fixture",&v);assert(count==0);free(v);
  count=stream_extrair("{\"streams\":["
    "{\"infoHash\":\"abc\",\"fileIdx\":1e99},"
    "{\"url\":\"https://example.invalid/a\",\"videoSize\":1e99},"
    "{\"url\":\"https://example.invalid/b\",\"title\":\"999999999999999999999999999999 GB\"},"
    "{\"infoHash\":\"def\",\"fileIdx\":2},"
    "{\"url\":\"https://example.invalid/c\",\"videoSize\":1073741824}]}","fixture",&v);
  assert(count==5 && v[0].fileIdx==-1 && v[1].tamanhoMB==0 && v[2].tamanhoMB==0);
  assert(v[3].fileIdx==2 && v[4].tamanhoMB==1024);free(v);
  count=stream_extrair("{\"streams\":["
    "{\"infoHash\":\"abc\",\"title\":\"Show S01E01 1080p\\n\xF0\x9F\x91\xA4 37 \xF0\x9F\x92\xBE 1.4 GB\"},"
    "{\"infoHash\":\"abd\",\"description\":\"Seeders: 0 | 2.1 GB\"},"
    "{\"infoHash\":\"abe\",\"title\":\"Show 720p 900 MB\"},"
    "{\"infoHash\":\"abf\",\"title\":\"\xF0\x9F\x91\xA4 99999999999999999999\"},"
    "{\"url\":\"https://example.invalid/d\",\"title\":\"\xF0\x9F\x91\xA4 12\"}]}","fixture",&v);
  assert(count==5 && v[0].temSemeadores && v[0].semeadores==37);
  assert(v[1].temSemeadores && v[1].semeadores==0);
  assert(!v[2].temSemeadores && v[3].temSemeadores && v[3].semeadores==999999999);
  assert(!v[4].temSemeadores);free(v);
  puts("PASS ASan/UBSan: parser isolado, 100 fontes, MP4/DV na última posição.");
}
