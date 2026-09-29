# Nuvio .tpk (Samsung Tizen 6+)

> **Acompanhamento:** [Project (quadro)](https://github.com/users/iqui27/projects/2) · [Milestone](https://github.com/iqui27/nuvio-native-legacy/milestone/1) · [issue #137 (testes)](https://github.com/iqui27/nuvio-native-legacy/issues/137). Na aba Issues, filtre `label:samsung-native` (tudo do nativo) e `label:"status: blocked"` (travado).

O mesmo app C do webOS e do `.wgt`, empacotado como app .NET da Samsung.

    bash tools/tpk.sh          # build/tpk/Nuvio-<versao>-NuvioTpk{60,65,}.tpk
    bash tools/tpk-testa.sh    # roda a .so num host falso no container ARM

## Como funciona

- **`libnuvio.so`**: `src/*.c` compilado com `-DNV_TPK`, no container
  `tools/tpk/Dockerfile` (Debian buster armel, glibc 2.28). SDL2, SDL2_image e
  SDL2_ttf entram estaticos, com o video "dummy" do SDL: o SDL so da fila de
  eventos, tempo, fios e superficies.
- **Host .NET** (`Program.cs`): abre um `GLWindow` de tela cheia. A cada quadro
  do NUI chama `nv_tpk_quadro()`, que passa o contexto EGL para o fio do app e
  espera o `SDL_GL_SwapWindow` dele (`src/tpk.c`, `src/tpk.h`). Teclas vao pelo
  nome (`XF86Back`, `Up`, ...).
- **Video**: `Tizen.Multimedia.Player` no host, no plano de video, com o
  `GLWindow` translucido por cima. O C pede e le o estado por `src/video_tpk.c`.

## Pacotes

| Pacote | TFM | Para |
|---|---|---|
| `NuvioTpk40` | tizen40 + TVGLApplication | Tizen 4.0-5.5 (TVs 2018-2020) |
| `NuvioTpk60` | tizen80 (API8) | Tizen 6.0 (TVs 2021) |
| `NuvioTpk65` | tizen90 (API9) | Tizen 6.5 e 7.0 (2022-2023) |
| `NuvioTpk` | net6.0-tizen8.0 (API11) | Tizen 8+ (2024 em diante) |

O `GLWindow` mudou de assinatura na API9 e de nome na API11, por isso tres
pacotes para 6+. No 4.0-5.5 nao ha GLWindow: o `NuvioTpk40` usa a
`TVGLApplication` da Samsung (pacote `Tizen.NET.TV`), no molde do
JuvoPlayer.OpenGL, com o video numa janela ElmSharp rebaixada. La a `.so`
propria foi recusada com certificado Public (spike.2, #137, cara de UEP); o
manifesto declara um privilegio Partner para o Apps2Samsung assinar como
Partner. Se ainda assim a TV recusar, a tela mostra o erro do `dlopen`.

## Estado atual (2026-09-29)

Provado em TV real, nao afirmado:

- **Tizen 6.0 (2021): FUNCIONA.** rawldon (AU7000) roda como app principal —
  video, audio, legenda, GIF e trailer. `Tpk60`/`Tpk65`/`Tpk` sao a rota boa
  para 6+.
- **Tizen 9 (2024): sem confirmacao.** Hov1122 (QN90D) nao reabriu depois do
  revert `fdc2d34`; falta relato.
- **Tizen 4.0/5.0 (2018-2019): a `.so` de ARQUIVO e barrada pela UEP.**
  Medido no probe (optiman, QE55Q6FNA, Tizen 4.0): `dlopen` de `lib/` e de
  `data/` falham ("failed to map segment"), MAS memoria anonima executavel e
  permitida (`mprotect +EXEC` OK). Rota em teste: carregar a `libnuvio.so` por
  `memfd_create` (syscall 385, a libc da TV nao exporta o nome) + `dlopen`
  em `/proc/self/fd/N`, com um carregador de ELF em memoria anonima como
  reserva. Host `NuvioTpk40` sendo adaptado para isso.
- **NaCl (.nexe em .wgt): descartado** — nao instala nas TVs testadas
  (optiman erro 118019, rawldon idem).

## O que falta

- cabecalhos de addon alem de `User-Agent` e `Cookie` (o player da Samsung
  nao aceita `Referer`);
- zoom/recorte do trailer no `.tpk` (ROI do player), e audio inconsistente em
  tela cheia (#178, Tizen 6.0);
- foco de audio: o canario `AudioStreamPolicy` NAO calou o YouTube por baixo
  nem devolveu o audio ao sair (rawldon, Tizen 6.0) — precisa de outra via.
