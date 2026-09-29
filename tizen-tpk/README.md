# Nuvio .tpk (Samsung Tizen 6+)

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

### Tizen 4/5: `.so` propria, sem TLS, e rastro de etapas (#137, #180)

O `NuvioTpk40` carrega a `libnuvio.so` por memfd (`syscall 385` +
`dlopen("/proc/self/fd/N")`) ou, se a UEP barrar, por um carregador de ELF
proprio em memoria anonima (`NuvioTpk40/Program40.cs`). Esse carregador nao
monta TLS de compilador, e a TV 2018-2020 fechava na primeira requisicao HTTPS
(o primeiro acesso a um `_Thread_local` de `src/rede.c`). Por isso:

- `tools/tpk.sh` compila uma **segunda** `.so` so para este pacote
  (`build/tpk/libnuvio-tpk40.so`, `-DNV_TPK40`): `rede.c` guarda o estado por
  fio em `pthread_key` em vez de `_Thread_local`, e o link leva
  `--hash-style=both`. O script falha se ela sair com `PT_TLS`, relocacao
  `R_ARM_TLS_*` ou sem `DT_HASH`; `tests/tpk40_tls.sh` confere de novo. A
  `.so` dos pacotes 6+ e a de sempre.
- O carregador de ELF **recusa** (com a frase na tela, em ingles) `PT_TLS`,
  relocacao que nao sabe aplicar, falta de `DT_HASH` e simbolo indefinido
  nao-fraco que o `dlsym` nao acha — antes pulava tudo isso em silencio.
- `data/tpk-etapas.txt` recebe `begin X` / `ok X` / `fail X` / `note ...` do
  host e do C (`nv_tpk40_etapa`, `open`+`write` com `O_APPEND`, sem handler de
  sinal). No arranque seguinte ele vira `tpk-etapas-anterior.txt`, entra no
  `nuvio.log` e, se acabou numa etapa sem `ok`, a tela mostra
  "Previous launch stopped at: ..." por 30 s para fotografar.

## O que falta

- cabecalhos de addon alem de `User-Agent` e `Cookie` (o player da Samsung
  nao aceita `Referer`);
- nada disto rodou numa TV ainda: so no host falso (`tools/tpk-testa.sh`).
