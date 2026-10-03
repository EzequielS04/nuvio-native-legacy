# 1.7.4 validation

Runtime changes use the shared C implementation. Production detail tests cover
Add action mapping, Modern focus/retract, restart, Back in filmography and
preserving an open detail page while switching titles. GL fixtures cover episode
replay/duration/remaining state, centered cast, smaller seasons, and grouped
hero actions. The new highlights card has keyboard, one-time marker and rendered
preview checks. Translation tables remain aligned across 28 secondary languages.

Catalogue tests reproduce the 64-entry truncation and explicit empty state
failure, then pass after the fix, including ASan/UBSan. Episode metadata tests
cover real runtime, missing/null runtime and preserving existing duration.

The initial Samsung package gate rejected Discord compiler TLS in the Tizen
4/5 library. That target now uses pthread keys with a destructor for per-thread
snapshots. Both normal and NV_TPK40 Discord lifecycle tests pass, including
ASan/UBSan. Final package verification must prove no PT_TLS, TLS relocations or
__tls_get_addr in that library; the modern Tizen library retains its own TLS.

Broad regression is tracked separately; early isolated discovery fixtures needed
SDL clock stubs after Home loading metrics were added. The fixtures were repaired
and rerun. Account tests require configured server values and retain the QR
reader skip where OpenCV is unavailable. The pre-existing anime-detail type
assertion from 1.7.3 remains a recorded failure, not a hidden pass.

Physical Android: user confirmed Discord and next-episode blur. Candidate APK
installation/hash and cold launch are verified separately. Synthetic captures do
not prove the physical remote flow or Samsung/LG playback. #223 remains open and
#233 is not claimed fully reproduced against the reporter's account.
