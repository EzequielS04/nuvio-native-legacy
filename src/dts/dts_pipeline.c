#define _POSIX_C_SOURCE 200809L
#include "dts_pipeline.h"
#include "adapter/adapter.h"
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
struct DtsPipeline { void *library, *native; const DtsAdapter *api; };
static void *open_adapter(int major, const DtsAdapter **api) {
  char path[PATH_MAX], executable[PATH_MAX];
  const char *override = getenv("NUVIO_DTS_ADAPTER_DIR");
  void *library;
  const DtsAdapter *(*entry)(void);
  ssize_t n;
  *api = NULL;
  if (major == 0) {
    void *auto_lib = open_adapter(4, api);
    return auto_lib ? auto_lib : open_adapter(3, api);
  }
  if (major < 3) return NULL;
  if (override && *override) {
    if (snprintf(path, sizeof path, "%s/dts-starfish-webos%d.so", override,
                 major == 3 ? 3 : 4) >= (int)sizeof path) return NULL;
  } else {
    n = readlink("/proc/self/exe", executable, sizeof executable - 1);
    if (n <= 0) return NULL;
    executable[n] = 0;
    char *slash = strrchr(executable, '/');
    if (!slash) return NULL;
    *slash = 0;
    if (snprintf(path, sizeof path, "%s/lib/dts-starfish-webos%d.so", executable,
                 major == 3 ? 3 : 4) >= (int)sizeof path) return NULL;
  }
  library = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  if (!library) return NULL;
  *(void **)(&entry) = dlsym(library, "nuvio_dts_adapter_v2");
  if (!entry) goto fail;
  *api = entry();
  if (!*api || (*api)->abi != DTS_ADAPTER_ABI || (*api)->size != sizeof(DtsAdapter) ||
      !(*api)->probe || !(*api)->create || !(*api)->destroy || !(*api)->load ||
      !(*api)->feed || !(*api)->play || !(*api)->pause || !(*api)->flush ||
      !(*api)->eos || !(*api)->media_id || !(*api)->error || !(*api)->probe()) goto fail;
  return library;
fail:
  *api = NULL; dlclose(library); return NULL;
}
int dts_pipeline_available(int major) {
  const DtsAdapter *api;
  void *lib = open_adapter(major, &api);
  if (!lib) return 0;
  dlclose(lib); return 1;
}
DtsPipeline *dts_pipeline_create(const char *app, const char *window, int major,
                                void (*event)(void *, const char *), void *user) {
  DtsPipeline *p = calloc(1, sizeof *p);
  if (!p) return NULL;
  p->library = open_adapter(major, &p->api);
  if (!p->library) { free(p); return NULL; }
  p->native = p->api->create(app, window, event, user);
  if (!p->native) { dlclose(p->library); free(p); return NULL; }
  return p;
}
void dts_pipeline_destroy(DtsPipeline *p) {
  if (!p) return;
  p->api->destroy(p->native); dlclose(p->library); free(p);
}
int dts_pipeline_load(DtsPipeline *p, const DtsMediaInfo *m, double t) { return p && m ? p->api->load(p->native,m,t) : 0; }
int dts_pipeline_feed(DtsPipeline *p, const DtsFrame *f) { return p && f ? p->api->feed(p->native,f) : -1; }
int dts_pipeline_play(DtsPipeline *p) { return p ? p->api->play(p->native) : 0; }
int dts_pipeline_pause(DtsPipeline *p) { return p ? p->api->pause(p->native) : 0; }
int dts_pipeline_flush(DtsPipeline *p, double t) { return p ? p->api->flush(p->native,t) : 0; }
int dts_pipeline_eos(DtsPipeline *p) { return p ? p->api->eos(p->native) : 0; }
const char *dts_pipeline_media_id(DtsPipeline *p) { return p ? p->api->media_id(p->native) : ""; }
const char *dts_pipeline_error(DtsPipeline *p) { return p ? p->api->error(p->native) : "native DTS adapter unavailable or ABI mismatch"; }

int dts_pipeline_volume(DtsPipeline *p, int pct) {
  return p && p->api->volume ? p->api->volume(p->native, pct) : 0;
}
