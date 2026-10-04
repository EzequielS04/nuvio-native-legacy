#!/bin/bash
# Fixtures MKV reais para tests/legref.sh e tests/legsync.sh (ffmpeg + mkvmerge).
#   bash tests/legref_fixtures.sh DIR
set -eu
D=${1:-${TMPDIR:-/tmp}/nv-legref-fx}; mkdir -p "$D"
[ -f "$D/.ok" ] && exit 0
command -v ffmpeg >/dev/null && command -v mkvmerge >/dev/null || { echo "precisa de ffmpeg e mkvmerge"; exit 2; }
python3 - "$D" <<'PY'
import random, sys
d = sys.argv[1]
random.seed(5)
def ts(t, sep=','):
    ms = int(round(t * 1000))
    return "%02d:%02d:%02d%s%03d" % (ms // 3600000, ms // 60000 % 60, ms // 1000 % 60, sep, ms % 1000)
def ass_ts(t):
    cs = int(round(t * 100))
    return "%d:%02d:%02d.%02d" % (cs // 360000, cs // 6000 % 60, cs // 100 % 60, cs % 100)
t = 10.0; ev = []
while t < 600:
    dur = random.uniform(1.0, 3.5)
    ev.append((t, t + dur, "Line number %d says something %d" % (len(ev) + 1, random.randint(0, 99999))))
    t += dur + random.uniform(0.3, 6.0)
def srt(name, shift=0.0, pt=False):
    with open("%s/%s" % (d, name), "w") as f:
        for i, (a, b, s) in enumerate(ev):
            txt = ("Fala traduzida numero %d diferente" % (i + 1)) if pt else s
            f.write("%d\n%s --> %s\n%s\n\n" % (i + 1, ts(a + shift), ts(b + shift), txt))
srt("emb.srt"); srt("ext_mais2500.srt", 2.5, pt=True); srt("ext_menos1200.srt", -1.2)
with open("%s/emb.ass" % d, "w") as f:
    f.write("[Script Info]\nScriptType: v4.00+\nPlayResX: 1920\nPlayResY: 1080\n\n[V4+ Styles]\n"
            "Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\n"
            "Style: Default,Arial,48,&H00FFFFFF,&H000000FF,&H00000000,&H00000000,0,0,0,0,100,100,0,0,1,2,0,2,10,10,10,1\n\n"
            "[Events]\nFormat: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n")
    for a, b, s in ev:
        f.write("Dialogue: 0,%s,%s,Default,,0,0,0,,{\\i1}%s{\\i0}\n" % (ass_ts(a), ass_ts(b), s))
print(len(ev))
PY
cd "$D"
# ~23 MB: os blocos de legenda ficam ESPALHADOS entre megabytes de video, como num filme.
ffmpeg -loglevel error -y -f lavfi -t 610 -i "testsrc2=s=640x360:r=10" -c:v libx264 -preset ultrafast -b:v 300k -g 50 video.mkv
# ffmpeg: SRT em ingles, sem nada mais.
ffmpeg -loglevel error -y -i video.mkv -i emb.srt -map 0 -map 1 -c copy -c:s srt -metadata:s:s:0 language=eng ff.mkv
# mkvmerge: forced (letreiro) primeiro, depois a de dialogo em ASS, depois SRT em portugues.
mkvmerge -q -o mm.mkv video.mkv --language 0:eng --forced-display-flag 0:1 --track-name 0:Signs emb.srt \
  --language 0:eng emb.ass --language 0:por ext_mais2500.srt
# Sem Cues para a legenda: o indice so aponta o video.
mkvmerge -q -o semcues.mkv video.mkv --cues 0:none --language 0:eng emb.srt
# So letreiro.
mkvmerge -q -o soforced.mkv video.mkv --language 0:eng --forced-display-flag 0:1 emb.srt
ffmpeg -loglevel error -y -i video.mkv -c copy video.mp4
touch .ok
