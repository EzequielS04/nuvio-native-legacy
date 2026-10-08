# Android TV na pipeline de release — passagem para o agente de release

Escrito em 01/10/2026. Leia inteiro antes da próxima release (1.6.6+).

## Resumo em 5 linhas

1. O Android TV (`.apk`) é o 4º alvo do mesmo núcleo C: LG `.ipk`, Samsung `.wgt`/`.tpk`, **Android `.apk`**.
2. O código está na branch **`feat/android`** (GitHub), que já contém a `v1.6.5` mesclada. **Ainda não está no `master`.**
3. Um comando gera e confere o pacote: `bash tools/release-android.sh` → `build/release-<v>/Nuvio-<v>-android.apk`.
4. O `.apk` vai **no mesmo `gh release create vX.Y.Z`** da LG e da Samsung. É dele que a atualização automática do Android depende.
5. **O Android ainda é segredo**: anexo sim; nas notas, na tabela "Which file do I need?", no README e nos avisos do app, **não**.

## Estado atual (provado)

- `v1.6.5` (latest) tem o anexo `Nuvio-1.6.5-android.apk`, assinado, sha256 `5426f6d4…6e01`. Ele foi feito da `feat/android` no commit `607aa09`, que é a v1.6.5 mais a camada Android.
- **A atualização automática foi provada na TV de teste.** Uma TCL Smart TV Pro (Android 14, 32 bits) com o APK de release 1.6.4 achou a 1.6.5 na latest, baixou, conferiu o sha256 e abriu o instalador do Android (`InstallSuccess`). A TV ficou em `versionName=1.6.5`.
- Pre-release `android-preview.1`: build de testes, fora da latest. Pode ser ignorada.

## Passo 0 — juntar o Android ao `master` (uma vez, antes da 1.6.6)

A release sai do `master`. Antes de mexer na 1.6.6:

```bash
git fetch origin
git checkout master && git merge --ff-only origin/master
git merge origin/feat/android        # deve ser limpo: feat/android ja tem a v1.6.5
```

O que a `feat/android` muda fora de `android/` (tudo atrás de `#ifdef NV_ANDROID`, salvo os itens marcados com **TODAS**):
- `src/android.c/.h` e `src/video_android.c` (novos).
- Guardas `NV_ANDROID` em: `main.c`, `video.c`, `ponteiro.c`, `atualizacao.c`, `extras.c`, `trailerapple.c`, `gif.c`, `text.c`, `assrender.c`, `avisos.c`, `registro.c`, `rede.c`, `jpegrapido.c`, `webp.c`, `dados.c`, `gpunivel.c`.
- `rede.c`: o teste de velocidade no Android mede em 4 conexões.
- **TODAS** as plataformas:
  - `guia.c`: a cópia de 900 canais saiu da pilha (`static`).
  - `trailer.c`: não muda nada (ficou o `trailer_recorte` da v1.6.5).
  - `app.c/.h`: nova função `app_zap_ativo()`.
  - `ajustes.c/.h`: ajuste novo **"Som do trailer na página do título"** (`AJ_DET_TRAILER_SOM`, chave `trailerDetalheSomLocal`, desligado por padrão). Está **depois** de `AJ_SELOS_CORES` nas quatro listas posicionais.
  - `ajustes.c`: correção do `-Wreturn-stack-address` do apelido.
- i18n: chaves novas (CH+/CH−, som do trailer, permissão do instalador) nas 29 tabelas.
- Testes novos: `tests/atualizacao_android.sh`. Os de sempre seguem valendo.

Depois do merge, rode `bash tools/testa-tudo.sh` e compare com o `master` antes de culpar o Android (falhas conhecidas na skill `samsung-release`).

## Pré-requisitos no Mac (uma vez por máquina)

| Item | Onde | Observação |
|---|---|---|
| JDK 17 | `~/.local/jdks/jdk-17*` | O cask do Homebrew pede sudo; foi baixado da API da Adoptium |
| Android SDK | `~/Library/Android/sdk` | platform 35, build-tools 35.0.0, **NDK 27.2.12479018**, cmake 3.22.1 |
| Dependências nativas | `bash tools/android/deps.sh` | curl+mbedTLS, libjpeg-turbo e libwebp em `~/.cache/nuvio-android/prefix`, idempotente |
| **Chave de release** | `~/.nuvio-android/release.jks` + `release.env` (chmod 600) | **Cópia no Vaultwarden (o dono guarda).** Sem ela o script recusa |

A impressão SHA-256 do certificado está fixada em `tools/release-android.sh` (`CERT_SHA256=c3c967d4…2add`). **Nunca gere outra chave**: um APK com outra assinatura não instala por cima, e todo mundo teria de desinstalar e entrar de novo.

## Passo a passo dentro da release vX.Y.Z

A ordem da versão inteira está na skill **`samsung-release`**. O Android é o passo 4 dela.

0. Auditoria antes de tudo: `bash tools/auditoria-release.sh` (docs/auditoria-release.md).
1. Versão em `deploy/app/appinfo.json` e `tools/tizen-config.xml`, como sempre. O Android lê o `versionName` daí e calcula o `versionCode` como `X*10000+Y*100+Z`.
2. LG: igual.
3. Samsung: `tools/release-samsung.sh`, igual.
4. **Android**, na mesma worktree limpa:
   ```bash
   bash tools/release-android.sh
   ```
   Ele recusa:
   - árvore suja;
   - versão divergente entre os dois arquivos;
   - chave ausente ou diferente da fixada;
   - `versionName` errado;
   - arquivo de pessoa no APK;
   - biblioteca faltando em qualquer das duas ABIs (`libmain`, `libSDL2`, `libcurl`, `libjpeg`, `libwebp`, `libffmpegJNI`).

   Sai `build/release-<v>/Nuvio-<v>-android.apk` + `SHA256SUMS-android`.
5. **Publicar**: o `gh release create vX.Y.Z` leva também o `Nuvio-<v>-android.apk`. O `SHA256SUMS` junta LG + `SHA256SUMS-samsung` + `SHA256SUMS-android`.
6. **Notas e tabela: NÃO mencionar o Android** enquanto o dono não liberar. Quando ele liberar, a linha da tabela é:
   `| Android TV / Google TV | Android 7 or newer | \`Nuvio-X.Y.Z-android.apk\` |`
7. **Conferir depois de publicar**: baixe o `.apk` pela URL pública e compare o sha256. **O GitHub respondeu 504 por ~1 minuto** no anexo recém-enviado da 1.6.5, enquanto os anexos antigos baixavam normal. Espere e repita. O app tenta 3 vezes, mas a release não deve sair com o link quebrado.

## Contrato da atualização automática (não quebrar)

- O app lê **`releases/latest`** e procura um anexo com nome terminando **exatamente** em `-android.apk`. `-android-debug.apk` e `-android-preview.N.apk` **não** casam, de propósito.
- Ele exige o `digest` sha256 do anexo (o GitHub preenche sozinho) e confere o arquivo baixado contra ele.
- Release **latest sem o `.apk`** = nenhum Android se atualiza naquela versão. Não quebra nada, só não atualiza.
- **Nunca** publique uma latest só com o `.apk`: ela tira o `repo.json` do Homebrew Channel (LG) e os pacotes Samsung da latest.

## Previews de teste (fora do vX.Y.Z)

`gh release create android-preview.N --prerelease --latest=false`, com o APK renomeado para `Nuvio-<v>-android-preview.N.apk`. Nunca marcar como latest.

## TV de teste do dono (para o agente que tiver acesso)

TCL Smart TV Pro, Android 14, `adb connect 192.168.1.128:5555`, APK de **release** instalado. Detalhes e armadilhas em `.claude/skills/android-release/SKILL.md`: pilha de 8 MB, HDR/Dolby Vision e o plano de vídeo que prende a geometria, janela TCP e 4 conexões, CH+/CH−, logcat de 256 KB (`adb logcat -G 16M`).

## Onde está o resto

- Skill **`android-release`** (`.claude/skills/android-release/SKILL.md`): o porte, o ambiente e as armadilhas.
- Skill **`samsung-release`**: a receita da versão inteira, já com o passo 4 Android.
- `tools/release-android.sh`, `tools/android.sh`, `tools/android/deps.sh`.
- Plano original do porte: `docs/PLANO-ANDROID.md` (no checkout principal; parte já está desatualizada).
