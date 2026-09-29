#!/bin/bash
# Gera e CONFERE todos os pacotes Samsung de uma release vX.Y.Z, sem publicar.
#
#   bash tools/release-samsung.sh            # .wgt + os 4 .tpk + anexos
#   bash tools/release-samsung.sh --so-tpk   # pula o .wgt (Emscripten)
#
# Sai em build/release-<versao>/ com os nomes que vao para o GitHub:
#   NuvioTV-<v>-tizen.wgt                     WASM, Tizen 5.5+
#   Nuvio-<v>-NuvioTpk40.tpk                  nativo, Tizen 4.0-5.5 (2018-2020)
#   Nuvio-<v>-NuvioTpk60.tpk                  nativo, Tizen 6.0 (2021)
#   Nuvio-<v>-NuvioTpk65.tpk                  nativo, Tizen 6.5-7 (2022-2023)
#   Nuvio-<v>-NuvioTpk.tpk                    nativo, Tizen 8-9 (2024+)
#   libnuvio-<v>-tpk-arm.so / -tpk40-arm.so   auto-atualizacao (#181): SEM ESTES
#                                             ANEXOS a TV nao se atualiza sozinha
#   SHA256SUMS-samsung                        juntar ao SHA256SUMS da LG
#
# O que ele RECUSA (sai com erro, nada fica pela metade sem aviso):
#   - arvore suja (todo script compila a arvore de trabalho; WIP de outra
#     sessao ja entrou num pacote em 27/09/2026);
#   - appinfo.json e tools/tizen-config.xml com versoes diferentes;
#   - qualquer arquivo de pessoa dentro de qualquer pacote;
#   - pacote 4/5 com o privilegio Partner drminfo (a TV do rawldon recusava
#     com 118014; Public funciona desde 29/09/2026);
#   - lib do 4/5 com TLS ou sem DT_HASH (tests/tpk40_tls.sh);
#   - .tpk com versao de manifesto diferente da release.
#
# Depois dele: conferir o que imprimiu, juntar com o .ipk da LG e publicar pela
# skill samsung-release (gh release create com TUDO de build/release-<v>/).
set -eo pipefail
cd "$(dirname "$0")/.."

SO_TPK=0; [ "$1" = "--so-tpk" ] && SO_TPK=1

if [ -n "$(git status --porcelain)" ]; then
  echo "release-samsung: arvore suja. Compile de uma worktree limpa no commit da release:" >&2
  echo "  git worktree add --detach ../nuvio-build-X <commit>" >&2
  git status --short >&2; exit 1
fi
VER=$(sed -n 's/.*"version": *"\([0-9.]*\)".*/\1/p' deploy/app/appinfo.json)
VT=$(grep -v '<?xml' tools/tizen-config.xml | sed -n 's/.*[[:space:]]version="\([0-9.]*\)".*/\1/p' | head -1)
[ -n "$VER" ] && [ "$VER" = "$VT" ] || { echo "release-samsung: appinfo.json ($VER) != tizen-config.xml ($VT)" >&2; exit 1; }
OUT="build/release-$VER"; rm -rf "$OUT"; mkdir -p "$OUT"
echo "== release-samsung $VER (commit $(git rev-parse --short HEAD)) -> $OUT"

# Credenciais: a lista de exclusao dos scripts e INTENCAO; isto e o fato.
SEGREDO='(^|/)(trakt|addons|tmdb|mdblist|sessao|simkl[^/]*|fanart|diag-token)\.txt$|collections\.json$|catalogo-rede\.bin|local\.properties|\.env$|(^|/)(trakt|stalker|xtream|listas)-p[0-9]'
confere() {  # $1 pacote (zip)
  local n; n=$(unzip -Z1 "$1" | grep -c -E "$SEGREDO" || true)
  [ "$n" = "0" ] || { echo "release-samsung: $1 leva $n arquivo(s) de pessoa:" >&2; unzip -Z1 "$1" | grep -E "$SEGREDO" >&2; exit 1; }
}

# 1. .tpk (os quatro de uma vez; tools/tpk.sh reescreve os manifestos)
bash tools/tpk.sh
git checkout -q -- tizen-tpk/*/tizen-manifest.xml   # o bump e so do pacote
bash tests/tpk40_tls.sh
for H in NuvioTpk40 NuvioTpk60 NuvioTpk65 NuvioTpk; do
  T="build/tpk/Nuvio-$VER-$H.tpk"
  [ -f "$T" ] || { echo "release-samsung: faltou $T" >&2; exit 1; }
  confere "$T"
  MV=$(unzip -p "$T" tizen-manifest.xml | sed -n 's/.* version="\([0-9.]*\)".*/\1/p' | head -1)
  [ "$MV" = "$VER" ] || { echo "release-samsung: $T diz versao $MV" >&2; exit 1; }
  if [ "$H" = NuvioTpk40 ] && unzip -p "$T" tizen-manifest.xml | grep -q drminfo; then
    echo "release-samsung: $T pede drminfo (Partner). Tire do manifesto: Public funciona e Partner recusa em parte das TVs." >&2; exit 1
  fi
  cp "$T" "$OUT/"
done
cp "build/tpk/libnuvio-$VER-tpk-arm.so" "build/tpk/libnuvio-$VER-tpk40-arm.so" "$OUT/"
# O pacote 6+ tem de levar a MESMA .so do anexo, e o 4/5 a dele: a auto-
# atualizacao compara versao, nao bytes, mas um anexo errado seria carregado.
D=$(mktemp -d); trap 'rm -rf "$D"' EXIT
unzip -qo "$OUT/Nuvio-$VER-NuvioTpk.tpk" lib/libnuvio.so -d "$D/6"
unzip -qo "$OUT/Nuvio-$VER-NuvioTpk40.tpk" lib/libnuvio.so -d "$D/4"
cmp -s "$D/6/lib/libnuvio.so" "$OUT/libnuvio-$VER-tpk-arm.so"   || { echo "release-samsung: anexo 6+ != .so do pacote" >&2; exit 1; }
cmp -s "$D/4/lib/libnuvio.so" "$OUT/libnuvio-$VER-tpk40-arm.so" || { echo "release-samsung: anexo 4/5 != .so do pacote" >&2; exit 1; }

# 2. .wgt
if [ "$SO_TPK" = 0 ]; then
  bash tools/tizen.sh
  NUVIO_WGT_NOME="NuvioTV-$VER-tizen" bash tools/tizen-wgt.sh
  confere "NuvioTV-$VER-tizen.wgt"
  mv "NuvioTV-$VER-tizen.wgt" "$OUT/"
fi

( cd "$OUT" && shasum -a 256 * > SHA256SUMS-samsung )
git status --porcelain | grep -q . && { echo "release-samsung: o build sujou a arvore:" >&2; git status --short >&2; }
echo
echo "== pronto, sem publicar. Conteudo de $OUT:"
ls -la "$OUT"
cat <<EOF

Proximo passo (skill samsung-release):
  - juntar com o .ipk da LG e o repo.json/webosbrew.manifest.json (hb-repo.sh)
  - SHA256SUMS = o da LG + $OUT/SHA256SUMS-samsung
  - gh release create v$VER <tudo> --title ... --notes-file ...
    (release NORMAL: ela vira latest. Pre-release de teste e sempre
     --prerelease --latest=false)
EOF
