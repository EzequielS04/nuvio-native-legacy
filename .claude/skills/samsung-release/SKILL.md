---
name: samsung-release
description: Atualizar TODAS as builds Samsung do Nuvio de uma vez (.wgt WASM + os quatro .tpk nativos + anexos de auto-atualizacao) dentro do release normal vX.Y.Z, junto do .ipk da LG — ordem, ferramenta, o que conferir, onde publicar e como avisar. Use quando o dono pedir "atualiza as builds da Samsung", "sai versao nova", ao fazer a release da 1.6 ou depois de um release da LG.
---

# Todas as builds Samsung

Desde a 1.5.4 (30/09/2026) o release normal `vX.Y.Z` leva a Samsung INTEIRA:

| Arquivo | TVs | Estado |
|---|---|---|
| `NuvioTV-<v>-tizen.wgt` | Tizen 5.5+ (2020+) | WASM, o de sempre |
| `Nuvio-<v>-NuvioTpk40.tpk` | Tizen 4.0–5.5 (2018–2020) | nativo, provado em 2x 4.0 e 2x 5.0 |
| `Nuvio-<v>-NuvioTpk60.tpk` | Tizen 6.0 (2021) | nativo, provado |
| `Nuvio-<v>-NuvioTpk65.tpk` | Tizen 6.5–7 (2022–2023) | nativo, sem relato forte |
| `Nuvio-<v>-NuvioTpk.tpk` | Tizen 8–9 (2024+) | nativo (GLView), provado em 3 TVs 9.0 |
| `libnuvio-<v>-tpk-arm.so` | todos os `.tpk` 6+ | anexo da auto-atualizacao |
| `libnuvio-<v>-tpk40-arm.so` | `.tpk` 4/5 | anexo da auto-atualizacao do 4/5 |

Detalhe de cada alvo: skills `samsung-wgt`, `samsung-tpk`, `samsung-tpk-legacy`.

## A ferramenta

```bash
git worktree add --detach ../nuvio-build-<v> <commit-da-release>
cd ../nuvio-build-<v>
NUVIO_PROPERTIES="/Users/hrocha/Projetos/Pessoal/LG WEB/NuvioWeb-0.3.38-beta/local.properties" \
  bash tools/release-samsung.sh
```

`tools/release-samsung.sh` compila o `.wgt` e os quatro `.tpk`, e RECUSA:
arvore suja; versao diferente entre `appinfo.json` e `tizen-config.xml`;
arquivo de pessoa em qualquer pacote; `drminfo` no 4/5; lib do 4/5 com TLS;
manifesto com outra versao; anexo `.so` diferente da `.so` do pacote. Sai em
`build/release-<v>/` com `SHA256SUMS-samsung`. NAO publica.

Numa worktree nova o `.wgt` para em "libass WASM ausente": faca
`ln -s "<checkout principal>/build/ass-wasm" build/ass-wasm` (dependencia ja
compilada, gitignorada; symlink e nao copia, porque o `tizen.sh` confere que o
atalho temporario `$TMPDIR/nuvio-ass-wasm-root-<uid>` aponta para o mesmo
lugar) ou rode `tools/build-ass-wasm.sh` com o emsdk ativo.

`NUVIO_PROPERTIES` so e preciso fora do checkout principal (o `tools/env.sh`
procura `../NuvioWeb-0.3.38-beta` dois niveis acima, e da worktree o caminho
nao existe: o pacote sairia sem servidor e sem login).

## Ordem

0. **Auditoria**: `bash tools/auditoria-release.sh` na arvore que vai sair
   (docs/auditoria-release.md). FAIL para a release; NAO VERIFICADO se explica.
1. **Versao**: `deploy/app/appinfo.json` e `tools/tizen-config.xml`. Commit
   `vX.Y.Z`. O `.tpk` pega sozinho (`tools/tpk.sh` reescreve os manifestos no
   build e a ferramenta devolve o original depois).
2. **LG**: receita da LG (`arm.sh --ipk`, as duas variantes `_arm.ipk` e
   `_arm-highcache.ipk`, `hb-repo.sh` -> `repo.json` + `webosbrew.manifest.json`).
3. **Samsung**: `tools/release-samsung.sh` (acima).
4. **Android**: `tools/release-android.sh` na mesma worktree (skill
   `android-release`). Sai `Nuvio-<v>-android.apk` + `SHA256SUMS-android` em
   `build/release-<v>/`. Precisa da chave de release em `~/.nuvio-android/`.
5. **Publicar TUDO no mesmo `gh release create vX.Y.Z`**: `.ipk` x2, `repo.json`,
   `webosbrew.manifest.json`, `.wgt`, os 4 `.tpk`, os 2 `libnuvio-*.so`, o
   `Nuvio-<v>-android.apk`, e o `SHA256SUMS` unindo LG + `SHA256SUMS-samsung` +
   `SHA256SUMS-android`. Release normal vira
   latest (o app e o Homebrew Channel leem `releases/latest`). Sem os `.so`
   anexados, os `.tpk` instalados nao se atualizam sozinhos.
6. **Notas** em ingles, `## Added` / `## Fixed` / `## Notes`, balas curtas (o
   app mostra 3 linhas por bala). Toda release termina com a TABELA "Which file
   do I need?" (pedido do dono, 29/09/2026), LG e Samsung juntas, com o nome
   EXATO de cada anexo. Ela vai DEPOIS de `## Notes`: o cartao de atualizacao
   (atualizacao.c) para de ler em "Notes", entao os `|` da tabela nunca chegam
   a tela da TV. Diga que o nativo Samsung e opt-in (quem prefere segue no
   `.wgt`). Modelo (troque X.Y.Z):

   ```markdown
   ## Which file do I need?

   | TV | System | File |
   |---|---|---|
   | LG 2016 and newer | webOS 3 or newer | `space.nuvio.native.legacy_X.Y.Z_arm.ipk` |
   | LG with 2 GB of RAM or more | webOS 3 or newer | `space.nuvio.native.legacy_X.Y.Z_arm-highcache.ipk` (bigger image cache) |
   | Samsung 2018–2020 | Tizen 4.0 / 5.0 / 5.5 | `Nuvio-X.Y.Z-NuvioTpk40.tpk` (native) |
   | Samsung 2021 | Tizen 6.0 | `Nuvio-X.Y.Z-NuvioTpk60.tpk` (native) |
   | Samsung 2022–2023 | Tizen 6.5 / 7.0 | `Nuvio-X.Y.Z-NuvioTpk65.tpk` (native) |
   | Samsung 2024 and newer | Tizen 8.0 / 9.0 | `Nuvio-X.Y.Z-NuvioTpk.tpk` (native) |
   | Samsung 2020 and newer | Tizen 5.5 or newer | `NuvioTV-X.Y.Z-tizen.wgt` (web version) |

   You don't need `libnuvio-*.so`, `repo.json`, `webosbrew.manifest.json` or
   `SHA256SUMS`: they're for self-update, the Homebrew Channel and checksums.
   ```
   ANDROID AINDA E SEGREDO (dono, 30/09/2026): o `.apk` vai como anexo, mas sem
   linha na tabela e sem mencao nas notas ate o dono liberar (skill
   `android-release`).
7. **Avisar**: issues curtas em ingles, so dizer "fixed" com a release no ar.
   `avisos.json` no master (`plataforma` `tizen` alcanca `.wgt` e `.tpk`;
   `tizen-tpk` so o `.tpk`; ids com `tpk-preview` sao ignorados pelo `.tpk`).

## Pre-releases (canarios)

Mudanca de HOST (.NET, `tizen-tpk/*.cs`) ainda nao vista numa TV vai antes num
canario: `bash tools/tpk.sh` na branch do canario, copiar de `build/tpk/`
com sufixo (`Nuvio-<v>-NuvioTpk60-canario-<tema>.tpk`), conferir credenciais,
`gh release create canario-<tema>-tpk.N --prerelease --latest=false`, e um
testador com aquela TV confirma. (`NV_TPK_NIVEL=1|2` e o unico canario que o
`tpk.sh` ja faz sozinho: nivel de GPU forcado.) So entao entra no
`vX.Y.Z`. NUNCA marcar pre-release como latest.

## Release da 1.6 (ou qualquer outra depois da 1.5.4)

A 1.6 foi feita em branches que NAO tem o trabalho Samsung da 1.5.4
(`feat/novidades160`, `feat/home-hero-cheio`, `fix/pos-152`, `feat/entre-amigos`,
`feat/home-decisoes`, `feat/agenda-modal-noticias`, `feat/notas-fontes`).

1. Parta do `master` (que tem a 1.5.4) e mescle as branches da 1.6 nele —
   nunca o contrario, e nunca volte a partir do `v1.5.3`.
2. Conflitos provaveis e como resolver:
   - `src/ajustes.c`: o enum `AJ_*`, `OPCOES`, `CHAVE` e `valor[]` sao
     POSICIONAIS. A 1.5.4 acrescentou `AJ_GPU_EFEITOS` NO FIM. Qualquer ajuste
     novo da 1.6 vai DEPOIS dele, nas quatro listas, na mesma ordem. O
     `_Static_assert` pega lista curta, nao lista trocada: confira a ordem.
   - `src/idioma_tab.h`: tabela ordenada por BYTES UTF-8 (busca binaria).
     Ao juntar chaves, reordene; `tests/i18n.sh` acusa.
   - `src/main.c`, `src/gfx.c`, `src/home.c`, `src/detail.c`, `src/player.c`:
     a 1.5.4 mexeu nesses arquivos com `#ifdef NV_TPK`. Preserve os blocos
     `NV_TPK` / `NV_TPK40` inteiros.
   - `src/rede.c` / `rede.h`: o 4/5 troca `_Thread_local` por `pthread_key`
     sob `NV_TPK40`. Variavel nova por fio na 1.6 precisa ir para dentro da
     struct `RedeFio`, senao volta o TLS e `tests/tpk40_tls.sh` falha.
3. `bash tools/testa-tudo.sh` (6 falhas conhecidas desde a 1.5.3: `home`,
   `syncordem`, `salvos_segurar`, `gifbanco-tizen`, `home-trailer-timer`,
   `tizen-globalthis` — compare com o `master` antes de culpar a 1.6).
4. `bash tools/tpk-testa.sh 60` (host falso) e `tests/tpk40_tls.sh`.
5. Se a 1.6 mudou algo em `tizen-tpk/*.cs`: canario antes (regra acima).
6. Siga a Ordem acima.

## Logs dos testadores

D1 `nuvio-recomendacoes`, tabela `registro` (`plataforma` `tizen` e
`tizen-tpk`). O `.tpk` escreve `[host] [tv] modelo=... tizen=... host=api8|api9|api11`
~3 s apos abrir, e `[etapa]` / `[etapa-anterior]` com cada passo do arranque
(o arquivo `data/tpk-etapas.txt` da abertura anterior sobe no log seguinte).
Ler de `servidor/recomendacoes`:
`export npm_config_cache="$TMPDIR/npmcache"; npx wrangler d1 execute nuvio-recomendacoes --remote --json --command "..."`
(saida com lixo: recorte do primeiro `[` e `json.JSONDecoder().raw_decode`).
Apagar registros e irreversivel: so com o ok do dono.
