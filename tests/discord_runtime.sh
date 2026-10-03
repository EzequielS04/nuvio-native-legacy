#!/bin/bash
# Deterministic transport/auth lifecycle tests: no network or account.
set -eu
cd "$(dirname "$0")/.."
flags=(-O1 -g -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -pthread)
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc "${flags[@]}" tests/discordws_local.c -o /tmp/nuvio-discordws-local
/tmp/nuvio-discordws-local
cc "${flags[@]}" tests/discord_lifecycle.c src/js.c src/jsw.c -o /tmp/nuvio-discord-lifecycle
/tmp/nuvio-discord-lifecycle
cc "${flags[@]}" -DNV_TPK40 tests/discord_lifecycle.c src/js.c src/jsw.c -o /tmp/nuvio-discord-lifecycle-tpk40
/tmp/nuvio-discord-lifecycle-tpk40
cc "${flags[@]}" -ffunction-sections -fdata-sections -Wl,-dead_strip tests/discord_tls.c src/js.c -o /tmp/nuvio-discord-tls
/tmp/nuvio-discord-tls
