# Localized title beside artwork (#227)

The detail page always shows the current catalog's textual title above the logo. The catalog's existing localization updates remain visible after opening; artwork keeps its independent session identity. No OCR, locale guessing, or additional metadata request is introduced.

The complete title wraps without a line limit. Its measured height is reserved above the logo, which shrinks when needed to keep the caption above the agenda, resume rows and action buttons. The logo is skipped if no drawable height remains. Titles without a logo use the same textual block.

Validation: `tests/detail_remonta.sh`, `tests/heroidentidade.sh`, and `NUVIO_SHOT_TITULO=1 bash tests/detail_eps_shot.sh <output-prefix>`. The last command uses production rendering with an offline synthetic catalog, verifies live title replacement, unrestricted wrapping and viewport bounds, and captures loaded-logo and no-logo variants. Different names and logo artwork in those fixtures intentionally exercise the localization mismatch. It is a Mac GL fixture, not physical-TV validation. The loading path uses the same unconditional caption branch but is not independently timed in the fixture; combined agenda/resume layout is anchored to the existing shared `yEstado` calculation.
