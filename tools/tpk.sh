#!/bin/bash
# Nuvio como .tpk para Samsung Tizen 6+ (TVs 2021 em diante).
#
#   bash tools/tpk.sh          # compila libnuvio.so e gera o .tpk em build/tpk/
#
# O C do Nuvio vira libnuvio.so (src/*.c + src/tpk.c, -DNV_TPK) e um host .NET
# (tizen-tpk/NuvioTpk) abre o GLWindow e a chama a cada quadro. Por que assim, e
# nao um executavel C: ver src/tpk.h. O spike que provou cada peca (.so propria,
# fios, GLWindow) esta em tizen-tpk-spike/.
#
# Tizen 4/5 (2018-2020) NAO: la o dlopen da .so propria e recusado com
# certificado Public (spike.2, #137). Essas TVs seguem no .wgt.
#
# Primeira vez: dependencias estaticas (SDL2 com video dummy, SDL2_image com
# stb, SDL2_ttf com freetype embutido) em ~/.cache/nuvio-tpk/prefix, feitas por
# tools/tpk/deps.sh na imagem tools/tpk/Dockerfile.
set -euo pipefail
cd "$(dirname "$0")/.."
RAIZ="$PWD"
CACHE="${NUVIO_TPK_CACHE:-$HOME/.cache/nuvio-tpk}"
SAIDA="build/tpk"
mkdir -p "$SAIDA" "$CACHE"

docker info >/dev/null 2>&1 || { echo "Docker parado: abra o Docker/OrbStack" >&2; exit 1; }
docker image inspect nuvio-tpk-sdk >/dev/null 2>&1 ||
  docker build --platform linux/arm/v5 -t nuvio-tpk-sdk tools/tpk/

if [ ! -f "$CACHE/prefix/lib/libSDL2.a" ] || [ ! -f "$CACHE/prefix/lib/libSDL2_ttf.a" ]; then
  echo "[deps] SDL2/SDL2_image/SDL2_ttf estaticos (demora na primeira vez)"
  mkdir -p "$CACHE/src"
  for u in https://github.com/libsdl-org/SDL/releases/download/release-2.30.9/SDL2-2.30.9.tar.gz \
           https://github.com/libsdl-org/SDL_image/releases/download/release-2.8.2/SDL2_image-2.8.2.tar.gz \
           https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.22.0/SDL2_ttf-2.22.0.tar.gz; do
    d="$CACHE/src/$(basename "$u" .tar.gz)"
    [ -d "$d" ] || curl -fsSL "$u" | tar xz -C "$CACHE/src"
  done
  cp tools/tpk/deps.sh "$CACHE/deps.sh"
  docker run --rm --platform linux/arm/v5 -v "$CACHE:/w" nuvio-tpk-sdk sh /w/deps.sh
fi

echo "[1/3] libnuvio.so (ARMv7 softfp, glibc <= 2.28)"
ENVF=$(mktemp); trap 'rm -f "$ENVF"' EXIT
tools/env.sh --env-file "$ENVF"
docker run --rm --platform linux/arm/v5 --env-file "$ENVF" \
  -e NUVIO_EXTRA_CFLAGS="${NUVIO_EXTRA_CFLAGS:-}" \
  -v "$RAIZ":/work -v "$CACHE/prefix":/deps -w /work nuvio-tpk-sdk sh -c '
  set -e
  mkdir -p /tmp/o
  # As -D vao num arquivo de resposta do gcc (@/tmp/flags): assim o xargs -P
  # abaixo compila em paralelo sem reabrir o problema de aspas das chaves.
  for k in NV_SUPABASE_URL NV_SUPABASE_ANON_KEY NV_TV_LOGIN_BASE NV_TRAKT_CLIENT_ID \
           NV_TRAKT_CLIENT_SECRET NV_SIMKL_CLIENT_ID NV_SIMKL_APP NV_TMDB_API_KEY \
           NV_REC_URL NV_VERSAO; do
    eval "v=\${$k:-}"
    # No arquivo de resposta o gcc tira aspas e barras como o shell: a aspa
    # da string C tem de ir escapada (-DX=\"valor\").
    v=$(printf "%s" "$v" | sed "s/[\\\\\"]/\\\\&/g")
    printf "%s\n" "-D$k=\\\"$v\\\""
  done > /tmp/flags
  ls src/*.c | grep -v "src/video_tizen.c" | xargs -P 6 -I{} sh -c \
    "gcc $CFLAGS -c {} -o /tmp/o/\$(basename {} .c).o -DNV_TPK -include src/tpk.h -fvisibility=hidden -Wno-unused-result $NUVIO_EXTRA_CFLAGS @/tmp/flags -I/deps/include -I/deps/include/SDL2" 
  # SDL e zlib ESTATICOS: a TV nao tem libSDL2 garantida, e a libz entra junto
  # para nao depender da versao do aparelho. GLES/EGL/dl/pthread/m sao do
  # sistema (API nativa publica do Tizen). curl, libjpeg e libwebp continuam
  # por dlopen em execucao, como na LG (rede.c, jpegrapido.c, webp.c).
  gcc -shared -o /work/build/tpk/libnuvio.so /tmp/o/*.o -Wl,--no-undefined \
    -Wl,-soname,libnuvio.so -Wl,--exclude-libs,ALL \
    -L/deps/lib -lSDL2_ttf -lSDL2_image -lSDL2 /usr/lib/arm-linux-gnueabi/libz.a \
    -lGLESv2 -lEGL -ldl -lpthread -lm -lrt
  echo "  $(ls -la /work/build/tpk/libnuvio.so | awk "{print \$5}") bytes"
  objdump -T /work/build/tpk/libnuvio.so | grep -oE "GLIBC_[0-9.]+" | sort -uV | tail -1 | sed "s/^/  glibc minima: /"
  objdump -p /work/build/tpk/libnuvio.so | grep NEEDED
  objdump -T /work/build/tpk/libnuvio.so | grep -E " nv_tpk_" | awk "{print \"  exporta \" \$NF}"
'

echo "[2/3] host .NET + pacotes"
export DOTNET_ROOT="${DOTNET_ROOT:-$HOME/.dotnet}" PATH="$HOME/.dotnet:$PATH" DOTNET_CLI_TELEMETRY_OPTOUT=1
VER=$(sed -n 's/^NV_VERSAO=//p' "$ENVF")
# A arte e a mesma reduzida do .wgt (tools/tizen-art.sh); fonts/ fica ao lado
# de art/, que e onde o main.c procura.
ARTE=$(bash tools/tizen-art.sh)
rm -f "$SAIDA"/*.tpk
for p in NuvioTpk60 NuvioTpk65 NuvioTpk; do
  H=tizen-tpk/$p
  rm -rf "$H/lib" "$H/res" "$H/bin" "$H/obj"
  mkdir -p "$H/lib" "$H/res" "$H/shared/res"
  cp "$SAIDA/libnuvio.so" "$H/lib/"
  cp -R "$ARTE" "$H/res/art"
  cp -R deploy/app/fonts "$H/res/fonts"
  cp deploy/app/tizen/icon.png "$H/shared/res/$p.png"
  sed -i '' "s/ version=\"[^\"]*\">/ version=\"$VER\">/" "$H/tizen-manifest.xml"
  dotnet build "$H/$p.csproj" -c Release -nologo -v q
  TPK=$(find "$H/bin/Release" -name '*.tpk' | head -1)
  [ -n "$TPK" ] || { echo "$p: dotnet nao gerou .tpk" >&2; exit 1; }
  cp "$TPK" "$SAIDA/Nuvio-$VER-$p.tpk"
done

echo "[3/3] conferindo"
for T in "$SAIDA"/*.tpk; do
  L=$(unzip -l "$T")
  grep -qE " lib/libnuvio.so$" <<<"$L" || { echo "$T sem lib/libnuvio.so" >&2; exit 1; }
  grep -qE " res/art/" <<<"$L" || { echo "$T sem res/art" >&2; exit 1; }
  echo "  $T ($(du -h "$T" | cut -f1))"
done
