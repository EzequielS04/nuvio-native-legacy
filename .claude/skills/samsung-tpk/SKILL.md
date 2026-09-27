---
name: samsung-tpk
description: Gerar, testar e publicar o Nuvio nativo .tpk para Samsung Tizen 6.0+ (host .NET com NUI GLWindow, pacotes NuvioTpk60/65/NuvioTpk). Use para qualquer build .tpk, atualização da preview nativa da Samsung, ou mudança em src/tpk.c, src/video_tpk.c, tizen-tpk/.
---

# .tpk Tizen 6+ (GLWindow)

O mesmo C do app (`src/*.c`, `-DNV_TPK`) vira `libnuvio.so`. Um host .NET abre
um `GLWindow` e, a cada quadro, passa o contexto EGL ao fio do app
(`src/tpk.c`). O vídeo é `Tizen.Multimedia.Player` no plano de vídeo
(`tizen-tpk/Video.cs` + `src/video_tpk.c`). Arquitetura e tabela de pacotes:
`tizen-tpk/README.md`. O mesmo `tools/tpk.sh` gera também o pacote do 4/5
(skill `samsung-tpk-legacy`).

## Build

```bash
git worktree add --detach ../nuvio-tpk-build <commit>   # nunca da arvore suja
cd ../nuvio-tpk-build && bash tools/tpk.sh
```

Sai em `build/tpk/Nuvio-<versao>-NuvioTpk{40,60,65,}.tpk`.

Pré-requisitos (a primeira vez demora):
- Docker/OrbStack LIGADO. Se o daemon estiver parado, os scripts não avisam
  bem: `open -a OrbStack`. O compilador é a imagem `nuvio-tpk-sdk`
  (`tools/tpk/Dockerfile`, Debian buster armel, glibc 2.28). O gcc do Tizen
  Studio no Mac roda por Rosetta e dá "internal compiler error" no SDL.
- Dependências estáticas (SDL2 com vídeo dummy, SDL2_image com stb,
  SDL2_ttf com freetype embutido) em `~/.cache/nuvio-tpk/prefix`, feitas por
  `tools/tpk/deps.sh`. O caminho não pode ter espaço (autotools).
- `.NET 8 SDK` em `~/.dotnet` + workload Tizen.
- `tools/env.sh` (chaves do servidor em `../NuvioWeb-0.3.38-beta/local.properties`).

## Testar sem TV

```bash
bash tools/tpk-testa.sh 60
```

Roda a `.so` num host falso (`tools/tpk/hostfalso.c`) no container ARM, com
Mesa por software e o mesmo protocolo de quadro (1 troca / 0 pula / -1 fim).
Saída em `build/tpk/teste/`: `nuvio.log` e `quadro.png`. Esperado: `[t] ~3500
primeiro quadro na tela`, `[tecla]` da tecla injetada e, sem sessão, a tela
de login com QR. Os muitos "pulos" são o Mesa lento, não defeito.

## Regras que já custaram caro

- `nv_tpk_quadro` NUNCA pode segurar o framework: na API8 (Tizen 6.0) o
  callback do GLWindow roda no fio PRINCIPAL do NUI. Espera limitada
  (`nv_tpk_config`), -1 quando o app acabou.
- API8 troca buffers mesmo em quadro pulado (callback void): lá a espera é
  sem limite e o arranque limpa para preto. API9+ respeita o retorno 0.
- `eglSwapInterval(0)` na API9+: o swap com vsync derrubava o Tizen 9 a
  3 fps; o DALi já dorme até 60 Hz.
- EGL por `dlopen` (`src/tpk_egl.h`): nenhum rootstrap do Tizen traz libEGL.
  Não voltar a linkar `-lEGL`.
- Teclas: mesma tabela do `.wgt` (`tools/tizen-shell.html`). O play/pause do
  Smart Remote novo é `XF86PlayBack`, CH+ é `XF86RaiseChannel` (vira `s`).
- Avisos do canal: o `.tpk` aceita `plataforma` `tizen` e `tizen-tpk`
  (`src/avisos.c`), não os da LG.
- `Referer` não passa: o player só aceita `UserAgent` e `Cookie`.

## Publicar

1. Credenciais: `unzip -l <tpk> | grep -iE "addons\.txt|trakt\.txt|tmdb\.txt|mdblist|sessao|collections\.json"` → vazio.
2. `gh release upload native-tpk-exp.N build/tpk/*.tpk --clobber` (preview
   existente) ou `gh release create native-tpk-exp.N --prerelease
   --latest=false ...`.
3. Descrição com a tabela por ano de TV e o estado real do teste (até
   27/09/2026: só simulador + 1 TV Tizen 6.0 em #165). Avisar na #137 e pelo
   `avisos.json` (skill `samsung-release`).

## Referências que resolveram dúvidas

- `dali-adaptor` `gl-window-render-thread.cpp` / `gl-window-impl.cpp`
  (git.tizen.org): semântica do retorno do callback, swap e AddIdle da 6.0.
- Nomes de tecla: developer.samsung.com/smarttv/develop/tizen-net-tv/guides/user-interaction.html
- `tizen-tpk-spike/`: o que foi medido em TVs reais (lib/, pthread, GLWindow).
