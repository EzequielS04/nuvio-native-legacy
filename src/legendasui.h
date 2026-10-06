// SUBTITLE SELECTOR (F04, 1.8): the island panel that grows from the clock
// when the person opens Subtitles in the player.
//
// SIMPLE VIEW. One row per language + origin ("Português · Embutida",
// "Português · OpenSubtitles"), filtered to the configured MAIN and SECOND
// subtitle languages (Ajustes / account). Each row carries its slot marker
// (Principal / Secundária) and the check of the slot it is active in. Each
// slot has its own "Nenhuma". "Mais opções" lists every candidate with the
// real details we have (release name, codec, letreiro flag, provider, format,
// offset/state) and lets the person aim at either slot. No configured
// language = no filter (the primary slot lists everything).
//
// FOCUS is an opaque row identity, never an index: embedded tracks discovered
// late and addon results arriving late insert rows without moving focus or
// closing the panel.
//
// SECOND SUBTITLE: only external SRT/VTT files (legenda2.h). The simple view
// never offers an embedded track or an ASS file for the second slot (its
// "Nenhuma" row says why); More options lists them dimmed with the reason, and
// choosing one there changes nothing — no check, the primary is never touched.
//
// The full Estilo bar stays in faixas.c; this module is reached through
// faixas_abrir_em(1) and hands control back with LEGUI_ESTILO / LEGUI_FECHAR.
#ifndef NV_LEGENDASUI_H
#define NV_LEGENDASUI_H
#include "gfx.h"
#include "addons.h"
#include "video.h"
#include <SDL2/SDL.h>

// --- panel, driven by faixas.c -----------------------------------------------
void  legendasui_abrir(void);
// 1 = the person is in "Mais opções".
int   legendasui_mais(void);
// 1 while the versions list of one language row is open (#202).
int   legendasui_versoes(void);
enum { LEGUI_NADA = 0, LEGUI_TRATADO, LEGUI_FECHAR, LEGUI_ESTILO };
int   legendasui_evento(const SDL_Event *e);
float legendasui_altura(void);
void  legendasui_corpo(GfxRect corpo, float a);
// Media change (faixas_reiniciar): drops the second subtitle and the focus.
void  legendasui_reiniciar(void);

// --- second subtitle on screen, called by player.c -----------------------------
// Geometry is passed explicitly by the player (real 1920x1080 coordinates).
typedef struct {
  float videoX, videoY, videoW, videoH;  // where the picture is this frame
  float osd;       // 0..1 controls on screen (the clock island moves the band)
  float alpha;     // player fade-in
  double pos;      // playback position used by the primary overlay (s)
  // R4: "Junto da principal". 1 = stacked: the block ends `baseY` (y of the TOP of
  // the primary's bottom stack, 1080 coordinates) minus a gap and grows upward.
  int   junto;
  float baseY;
} LegendasGeo;
// Draws the second document in the TOP band (primary stays at the bottom).
// Must run before any primary early return so both show in every branch.
void  legendasui_desenhar_secundaria(const LegendasGeo *g);
// R4: forgets the top band of the previous frame. The stacked mode draws the second
// subtitle AFTER the primary, so the primary must not read last frame's band.
void  legendasui_banda_zerar(void);
// Lowest y the primary's TOP-anchored stack (\an7-9) may start at so it does
// not collide with the second band drawn this frame. `minimo` when no band.
float legendasui_altura_secundaria(const LegendasGeo *g);
float legendasui_topo_livre(float minimo);

// --- AutoSync (F05) plugs in here ---------------------------------------------
// A NULL provider (the default) hides the sync row. `estado` returns the state
// text for a slot (0 = Principal, 1 = Secundária), already translated, or
// NULL/"" to hide that slot's row. `acoes` fills up to `max` translated action
// labels ("Sincronizar", "Desfazer", "Tentar outra referência") available now;
// `executar` runs action `i` of that list. All called on the UI thread.
typedef struct {
  const char *(*estado)(int slot, void *u);
  int  (*acoes)(int slot, const char **rotulos, int max, void *u);
  void (*executar)(int slot, int acao, void *u);
  void *u;
} LegendasSyncProvider;
void legendasui_definir_sync(const LegendasSyncProvider *p);
// Opaque identity of what each slot shows ("" = none / not yet active).
const char *legendasui_slot_identidade(int slot);

// --- model (pure; tests use it without SDL windows) ---------------------------
#define LEGUI_MAX_CAND (NV_FAIXA_MAX + LEG_MAX)
typedef struct {
  char id[24];       // opaque identity: 'e'/'a' + 16 hex digits
  int  embutida;     // 1 = track inside the file
  int  indice;       // embedded track index, or addon list index (snapshot)
  char idioma[16];
  char rotulo[64];   // embedded: track label; addon: the addon's label
  char origem[64];   // provider name, "" for embedded
  char arquivo[96];  // release/file name when the addon sent one
  char codec[24];    // Matroska CodecID when probed
  char formato[8];   // "SRT"/"VTT"/"ASS"/"PGS" when actually known, else ""
  int  letreiro;     // forced / signs track
  int  tipo;         // LING_LEG_* of an embedded track (#287); 0 for addons
  int  completaPar;  // 1 = plain embedded track whose language also has a forced/signs one:
                     // shown as "Legenda completa" so the two read apart (#287)
  Legenda leg;       // private copy (URL never shown or logged)
} LegUiCand;
// Identity helpers: stable within a media session.
void legendasui_id_embutida(const VideoFaixa *f, char out[24]);
void legendasui_id_addon(const Legenda *l, char out[24]);
int  legendasui_montar(const VideoFaixa *const *emb, int nEmb,
                       const Legenda *add, int nAdd, LegUiCand *out, int max);
// Short translated kind of an embedded candidate ("Forçada", "Letreiros",
// "SDH", "Legenda completa"); NULL when there is nothing to say (#287).
const char *legendasui_tipo_rotulo(const LegUiCand *c);
// 1 when the candidate can be the SECOND subtitle (external, not ASS).
int  legendasui_cand_secundaria_ok(const LegUiCand *c);

#endif
