# Dependencias estaticas do .tpk. Roda DENTRO de nuvio-tpk-sdk (tools/tpk/Dockerfile),
# com o cache montado em /w e os fontes em /w/src (tools/tpk.sh baixa).
set -e
P=/w/prefix; mkdir -p $P
cd /w/src/SDL2-2.30.9
[ -f $P/lib/libSDL2.a ] || { ./configure -q --prefix=$P --enable-static --disable-shared --disable-audio --disable-video-x11 --disable-video-wayland --disable-video-kmsdrm --disable-video-vulkan --disable-video-opengl --disable-video-opengles --disable-joystick --disable-haptic --disable-sensor --disable-hidapi --disable-pulseaudio --disable-alsa --disable-dbus --disable-ime --disable-ibus --disable-fcitx --disable-libudev --disable-video-rpi --disable-render --disable-sndio --disable-pipewire --disable-jack --disable-esd --disable-arts --disable-nas --disable-oss && make -j6 >/dev/null && make install >/dev/null; } || exit 1
export PKG_CONFIG_PATH=$P/lib/pkgconfig PATH=$P/bin:$PATH
cd /w/src/SDL2_image-2.8.2
[ -f $P/lib/libSDL2_image.a ] || { ./configure -q --prefix=$P --enable-static --disable-shared --enable-stb-image --disable-jpg-shared --disable-png-shared --disable-webp --disable-avif --disable-jxl --disable-tif --disable-qoi && make -j6 >/dev/null && make install >/dev/null; } || exit 1
cd /w/src/SDL2_ttf-2.22.0
[ -f $P/lib/libSDL2_ttf.a ] || { ./configure -q --prefix=$P --enable-static --disable-shared --enable-freetype-builtin --disable-harfbuzz && make -j6 >/dev/null && make install >/dev/null; } || exit 1
ls -la $P/lib/*.a
