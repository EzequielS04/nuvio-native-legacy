#!/bin/bash
# Gera e CONFERE o APK Android TV de uma release vX.Y.Z, sem publicar.
#
#   bash tools/release-android.sh            # APK de release assinado
#
# Sai em build/release-<versao>/ (a mesma pasta do release-samsung.sh):
#   Nuvio-<v>-android.apk     Android 7+ (arm64-v8a + armeabi-v7a). E ESTE nome
#                             que a auto-atualizacao procura em releases/latest
#                             (atualizacao.c: sufixo "-android.apk"; debug e
#                             preview nao casam).
#   SHA256SUMS-android        juntar ao SHA256SUMS da LG/Samsung
#
# O que ele RECUSA (sai com erro):
#   - arvore suja (o APK leva o que esta no disco);
#   - appinfo.json e tools/tizen-config.xml com versoes diferentes;
#   - sem a chave de release (~/.nuvio-android/release.env, ou NUVIO_KEYSTORE*
#     no ambiente): APK de release com outra chave nao instala por cima;
#   - APK assinado com OUTRA chave que a fixada abaixo (CERT_SHA256);
#   - versionName diferente da release;
#   - arquivo de pessoa dentro do APK;
#   - faltando libmain/libSDL2/libcurl em alguma das duas ABIs.
set -eo pipefail
cd "$(dirname "$0")/.."

# Impressao SHA-256 do certificado da chave de release (gerada em 30/09/2026,
# copia de seguranca no Vaultwarden). Trocar a chave = ninguem atualiza por
# cima; so mude isto junto de um aviso para desinstalar e reinstalar.
CERT_SHA256=c3c967d4fac138de15126da01ea3ae43b52e6aa23106e336c2acc607ae0f2add

if [ -n "$(git status --porcelain)" ]; then
  echo "release-android: arvore suja. Compile de uma worktree limpa no commit da release:" >&2
  echo "  git worktree add --detach ../nuvio-build-X <commit>" >&2
  git status --short >&2; exit 1
fi
VER=$(sed -n 's/.*"version": *"\([0-9.]*\)".*/\1/p' deploy/app/appinfo.json)
VT=$(grep -v '<?xml' tools/tizen-config.xml | sed -n 's/.*[[:space:]]version="\([0-9.]*\)".*/\1/p' | head -1)
[ -n "$VER" ] && [ "$VER" = "$VT" ] || { echo "release-android: appinfo.json ($VER) != tizen-config.xml ($VT)" >&2; exit 1; }

if [ -z "${NUVIO_KEYSTORE:-}" ] && [ -f "$HOME/.nuvio-android/release.env" ]; then
  set -a; . "$HOME/.nuvio-android/release.env"; set +a
fi
[ -n "${NUVIO_KEYSTORE:-}" ] && [ -f "$NUVIO_KEYSTORE" ] || {
  echo "release-android: sem a chave de release. Restaure ~/.nuvio-android/release.jks e release.env do Vaultwarden." >&2; exit 1; }

OUT="build/release-$VER"; mkdir -p "$OUT"
rm -f "$OUT"/Nuvio-*-android*.apk "$OUT/SHA256SUMS-android"
echo "== release-android $VER (commit $(git rev-parse --short HEAD)) -> $OUT"

bash tools/android.sh
A="build/android/Nuvio-$VER-android.apk"
[ -f "$A" ] || { echo "release-android: faltou $A (o android.sh nao gerou o release)" >&2; exit 1; }

SDK="${ANDROID_HOME:-$HOME/Library/Android/sdk}"
BT=$(ls -d "$SDK"/build-tools/* | sort -V | tail -1)
CERT=$("$BT/apksigner" verify --print-certs "$A" 2>/dev/null | sed -n 's/^Signer #1 certificate SHA-256 digest: //p')
[ "$CERT" = "$CERT_SHA256" ] || { echo "release-android: $A assinado com $CERT, esperado $CERT_SHA256" >&2; exit 1; }
VN=$("$BT/aapt2" dump badging "$A" 2>/dev/null | sed -n "s/.*versionName='\([^']*\)'.*/\1/p" | head -1)
[ "$VN" = "$VER" ] || { echo "release-android: versionName $VN != $VER" >&2; exit 1; }

SEGREDO='(^|/)(trakt|addons|tmdb|mdblist|sessao|simkl[^/]*|fanart|diag-token)\.txt$|collections\.json$|catalogo-rede\.bin|local\.properties|\.env$|(^|/)(trakt|stalker|xtream|listas)-p[0-9]|\.jks$|\.keystore$'
# A lista uma vez so: `unzip | grep -q` com pipefail falha quando o grep fecha
# o pipe antes de o unzip terminar.
LISTA=$(unzip -Z1 "$A")
n=$(printf '%s\n' "$LISTA" | grep -c -E "$SEGREDO" || true)
[ "$n" = "0" ] || { echo "release-android: $A leva $n arquivo(s) de pessoa:" >&2; printf '%s\n' "$LISTA" | grep -E "$SEGREDO" >&2; exit 1; }
for abi in arm64-v8a armeabi-v7a; do
  for so in libmain.so libSDL2.so libcurl.so libjpeg.so libwebp.so libffmpegJNI.so; do
    case $'\n'"$LISTA"$'\n' in *$'\n'"lib/$abi/$so"$'\n'*) ;; *) echo "release-android: faltou lib/$abi/$so" >&2; exit 1;; esac
  done
done

cp "$A" "$OUT/"
( cd "$OUT" && shasum -a 256 Nuvio-*-android.apk > SHA256SUMS-android )
git status --porcelain | grep -q . && { echo "release-android: o build sujou a arvore:" >&2; git status --short >&2; }
echo
echo "== pronto, sem publicar:"
ls -la "$OUT"/Nuvio-*-android.apk "$OUT/SHA256SUMS-android"
cat <<EOF

Proximo passo (skill android-release):
  - SHA256SUMS = LG + SHA256SUMS-samsung + SHA256SUMS-android
  - anexar $OUT/Nuvio-$VER-android.apk no MESMO gh release create v$VER
    (sem ele no latest, nenhum Android se atualiza sozinho)
EOF
