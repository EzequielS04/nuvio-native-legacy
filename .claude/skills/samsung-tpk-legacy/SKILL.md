---
name: samsung-tpk-legacy
description: O .tpk nativo do Nuvio para Samsung Tizen 4.0-5.5 (TVs 2018-2020), pacote NuvioTpk40 sobre a TVGLApplication da Samsung com assinatura Partner. Use ao mexer em tizen-tpk/NuvioTpk40, ao ler relatos de TVs 2018-2020, ou ao decidir o que fazer se a TV bloquear a .so.
---

# .tpk Tizen 4.0–5.5 (TVGLApplication)

Mesma `libnuvio.so` e mesmo protocolo de quadro do 6+ (skill `samsung-tpk`),
com outro host, porque essas APIs não têm GLWindow:

- `Tizen.TV.NUI.GLApplication.TVGLApplication` (pacote NuGet `Tizen.NET.TV`
  4.4.0.1341, `ExcludeAssets=Runtime`: a assembly já está na TV). `OnUpdate()`
  roda no fio principal com o contexto corrente; devolver `true` faz ela
  trocar os buffers. Espera do quadro: 50 ms (`nv_tpk_config(50, 0)`).
- Vídeo: janela ElmSharp separada, `Show()` + `Lower()`, dada ao
  `Tizen.Multimedia.Display`. É exatamente o molde do
  `JuvoPlayer.OpenGL` (github.com/SamsungDForum/JuvoPlayer). O exemplo mínimo
  da Samsung é github.com/SamsungDForum/OpenGLES.
- O DllImport do Tizen 4/5 não procura no `lib/` do pacote ("liblibX.so.so").
  O host abre a `.so` por caminho absoluto com `dlopen` e o DllImport casa
  pelo soname (`-Wl,-soname,libnuvio.so` em `tools/tpk.sh`). Chamar
  `dlerror()` uma vez ANTES do `dlopen`, senão o CLR zera a mensagem.

Build, teste e publicação: iguais aos da skill `samsung-tpk` (`tools/tpk.sh`
gera os quatro pacotes juntos).

## O bloqueio (UEP)

Com assinatura Public, TVs 4.0/5.0 recusaram a `.so` própria (dlopen com erro
vazio) e o executável próprio (`Operation not permitted`), no spike.2 de
26/09/2026 (#137). A causa provável é o UEP da Samsung, que verifica a
assinatura de código em partições graváveis. A FAQ da Samsung diz que `.so`
própria precisa ser "assinada" e que isso exige ser parceiro do Seller Office.

O que o pacote faz a respeito:
- O manifesto declara `http://developer.samsung.com/privilege/drminfo` (nível
  Partner). O Apps2Samsung vê isso (`WgtPrivileges.cs`) e re-assina o pacote
  como **Partner** sozinho. O app não usa DRM.
- Se mesmo assim a TV recusar, o host tenta a cópia em `data/` e mostra uma
  tela com o erro real do `dlopen`, a montagem da pasta, o modelo e a versão
  do Tizen, e pede FOTO na #137.

Ainda NÃO provado em TV (27/09/2026): se a assinatura Partner do pacote
basta, ou se a Samsung exige assinar a própria `.so` (só parceiro). Se as
fotos mostrarem o bloqueio mesmo com Partner, essas TVs ficam no `.wgt`
(Tizen 4 experimental, skill `samsung-wgt`). Não gaste mais tempo com truques
de caminho ou nome: nenhum deles assina o binário.

## Tizen 5.5 (2020)

API7, sem GLWindow: vai neste pacote também (api-version 4 roda no 5.5, e o
`Tizen.NET.TV` existe até a 5.5).
