---
name: samsung-release
description: Atualizar TODAS as builds Samsung do Nuvio de uma vez (.wgt normal, .wgt Tizen 4 experimental, .tpk Tizen 6+, .tpk Tizen 4/5) — ordem, o que cada uma exige, onde publicar e como avisar. Use quando o dono pedir "atualiza as builds da Samsung", "sai versão nova na Samsung", ou depois de um release da LG.
---

# Todas as builds Samsung

Quatro entregáveis. Cada um tem skill própria com os detalhes; esta é a ordem.

| Build | Skill | TVs | Release |
|---|---|---|---|
| `.wgt` (WASM) | `samsung-wgt` | Tizen 5.5+ (2020+) | release normal `vX.Y.Z`, junto do `.ipk` da LG |
| `.wgt` Tizen 4 exp | `samsung-wgt` (seção Tizen 4) | 2018–2019 | pre-release `native-tizen4-exp.N` |
| `.tpk` 6+ (NUI GLWindow) | `samsung-tpk` | Tizen 6.0+ (2021+) | pre-release `native-tpk-exp.N` |
| `.tpk` 4/5 (TVGLApplication) | `samsung-tpk-legacy` | Tizen 4.0–5.5 (2018–2020) | mesma pre-release do `.tpk` |

## Ordem

1. **Versão**: bater `deploy/app/appinfo.json` e `tools/tizen-config.xml` (e
   `tools/tizen4-config.xml` na branch do Tizen 4). `tools/env.sh` aborta se
   discordarem. O `.tpk` pega a versão sozinho (`tools/tpk.sh` reescreve os
   `tizen-manifest.xml`).
2. **Worktree limpa para compilar**: todo script compila a ÁRVORE DE TRABALHO.
   Outra sessão pode ter WIP em `src/` (já entrou num commit e num build do
   `.tpk` em 27/09/2026). Compile de uma worktree destacada no commit que vai
   ser publicado: `git worktree add --detach ../nuvio-build-X <commit>`.
3. **Builds**, cada uma pela sua skill.
4. **Conferir credenciais em cada pacote** (a skill de cada alvo diz como).
   Tudo tem de dar 0 arquivos de segredo (`addons.txt`, `trakt.txt`,
   `tmdb.txt`, `mdblist.txt`, `sessao.txt`, `collections.json`).
5. **Publicar**. Pre-releases de Samsung são sempre `--prerelease
   --latest=false`: o app normal consulta `releases/latest` para se atualizar,
   e uma pre-release marcada como latest quebra a atualização de todo mundo.
6. **Avisar**:
   - issues: curto, em inglês (o público é de fora). Só dizer "fixed" depois
     que a release com o pacote existe.
   - dentro do app: `avisos.json` no **master** (o app lê de
     `raw.githubusercontent.com/.../master/avisos.json`). Título e texto em
     INGLÊS nos dois campos (`titulo`/`titulo_en`, `texto`/`texto_en`).
     `plataforma`: `"tizen"` alcança o `.wgt` e o `.tpk`; `"tizen-tpk"` só o
     `.tpk`; `"lg"` só a LG. Anúncio do próprio `.tpk` leva `tpk-preview` no
     `id` (o `.tpk` ignora esses; ver `src/avisos.c`).

## Logs dos testadores

Relatórios automáticos vão para o D1 `nuvio-recomendacoes`, tabela `registro`
(`plataforma` = `tizen`, `tizen-tpk`; spikes em `pessoa LIKE 'diag:%'`). Ler
de `servidor/recomendacoes` com `npx wrangler d1 execute nuvio-recomendacoes
--remote --json --command "..."`. A saída do wrangler vem com lixo antes e
depois do JSON: recorte do primeiro `[` e use `json.JSONDecoder().raw_decode`.
Se os envios derem HTTP 500, conferir `npx wrangler d1 info` — em 27/09/2026 o
banco bateu 500 MB (limite grátis). Apagar registros é irreversível: só com o
ok do dono.
