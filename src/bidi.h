// Arabic shaping + RTL ordering for ONE already-wrapped line of subtitle text.
//
// SDL_ttf here has no HarfBuzz (Android, probably webOS) and draws one
// codepoint at a time: Arabic came out as isolated letters in left-to-right
// order (#239, #245, #247). This turns a logical-order UTF-8 line into the
// string the glyph-per-codepoint renderer must draw: Arabic letters replaced by
// their Presentation Forms-B (contextual) codepoints, the whole line in VISUAL
// order. Self-contained, no dependency (libass/fribidi stay untouched: embedded
// ASS never goes through here).
//
// Wrap and measure on the LOGICAL text first, then call this per wrapped line.
#ifndef NUVIO_BIDI_H
#define NUVIO_BIDI_H
#include <stddef.h>

// Writes the visual form of `in` into `out` (NUL terminated, at most tam bytes).
// Returns 0 when the text needs no change (no Hebrew/Arabic codepoint): `out`
// is then a byte-identical copy. Returns 1 when it was reshaped/reordered.
// Returns -1 when `out` was too small for the result or the line is too long
// to process: `out` then holds the unchanged text, truncated at a character
// boundary. Never writes past tam; tam == 0 writes nothing.
int bidi_visual_utf8(const char *in, char *out, size_t tam);

#endif
