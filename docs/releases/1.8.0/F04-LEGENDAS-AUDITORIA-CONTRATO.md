# F04 selector and second subtitle — integration contract

Current integration audit, 2026-10-03. No F04 source changes yet.

`legendasui.c/h` do not exist. `faixas.c` owns all subtitle choices and automatic primary selection. `escolherLegenda()` is static and changes singleton native/legenda/mkvass/libass state. `legenda.c` already supplies immutable documents and independent cue lookup; AutoSync supplies two selections/manual+automatic offsets. Neither API implements a second overlay or independent embedded-track collector. `assrender.c` has one library/renderer/track/worker/GL texture state. `mkvass_iniciar*()` replaces the existing collector. Calling these for a secondary would silently replace the primary and is forbidden.

## Implementation boundaries to coordinate

1. New `legendasui.c/h`: selector model and island panel. Initial view filters strictly to configured main/secondary language families; language + embedded/provider origin, distinct slot markers. No configured languages preserves unfiltered behavior. None available per slot. More options exposes all candidates already returned, with original title/release, embedded codec/letreiro flag, provider, supported format/offset/state only when real. No invented metadata. Stable focus is an opaque identity, never a combined index; arriving embedded tracks/addons remap identity without closing or resetting focus.
2. New subtitle session state/loader: secondary external bytes become an independent `LegendaDocumento`, with session + selection generation guards, bounded download, cancellation, independent offset, explicit loading/failure/unsupported status. Selection is not called active until a usable document is published. It never calls primary `legenda_carregar`, `assrender_carregar` or `mkvass_iniciar`.
3. Narrow `faixas.c/h`: wrap primary selection/active selection; open simple selector from subtitle entry. Existing full list/style remains More options. Delegation through faixas open/event/update/draw/restart avoids changing app routing and player retention logic. Reset secondary session when faixas resets.
4. Narrow `player.c`: second document cue rendering, before every primary ASS/no-cues early return; independent offset exactly once. Renderer must reserve separate area and account for OSD/next-episode/style overlays, not merely draw a second label. A public function may be called once from the common player subtitle drawing site, with clock/media-area/control geometry passed explicitly. Root owns this integration until coordinated.
5. Optional `addons.c/h` atomic candidate snapshot getter: existing addon accessor returns a pointer after unlocking and can race replacement. Gathering a model needs copies under the list mutex. Keep IDs opaque; a URL may be used privately to disambiguate current files, never as a UI detail/log/document identity.

## Honest initial capability

An external SRT/VTT secondary has a real implementation path using immutable document cues. Embedded secondary does not currently have an independent extractor, including SRT/MP4; unavailable must be explicit. Full ASS secondary needs another libass context plus positioning/collision handling. Parser fallback alone loses authored tags and is not equivalent. Picking an unavailable embedded/ASS secondary must not show an active check or modify the primary.

The resulting limited implementation can close selector behavior and external plain-text dual rendering, but must not claim F04 fully complete across embedded/ASS/backends. Complete dual embedded/ASS rendering requires a coordinated extension of collector + assrender beyond selector ownership. F05 must still show AutoSync unavailable without a genuine complete independent reference; no native track changes to fabricate reference.

Tests: languages/origins, variants, None/More options, main+secondary selection, late data/focus identity, indices shifted by embedded discovery, metadata absent/unknown, stale downloads after media/profile/selection changes, one slot offset not modifying another, multiline/ASS positions vs controls and second band, actual simultaneous external document GL rendering. Device proofs remain separate.
