#!/usr/bin/env python3
"""Gera docs/issues/MAPA.md a partir de docs/issues/mapa.json.
Uso: python3 docs/issues/mapa.py   (edite so o json; o md e derivado)"""
import json,os,re,collections
D=os.path.dirname(os.path.abspath(__file__))
J=json.load(open(os.path.join(D,'mapa.json')))
R=J['itens']
def vk(t): return [int(p) for p in re.findall(r'\d+',t)]
def grupo(r):
    s,rel=r['status'],r['release']
    if s!='lancada' and rel.startswith('2.0.3'): return '203'
    if rel.startswith('2.0.4'): return '204'
    if rel.startswith(('2.1','2.2','futuro')) and s not in ('lancada','duplicada'): return 'fut'
    if s=='lancada': return 'ok'
    if r['estado_github']=='aberta': return 'sem'
    return 'fech'
G=collections.defaultdict(list)
for r in R: G[grupo(r)].append(r)
def resp(r):
    u=r['ultima_resposta']
    if not u['data']: return 'sem comentários'
    return ('nós ' if u['nos'] else 'autor ')+u['data'][5:]
def cell(s): return str(s).replace('|','/').replace('\n',' ')
def tab(rs):
    out=['| # | Título | Plat. | Tipo | Status | Release | Conserto | Última resposta | Próximo passo |','|---|---|---|---|---|---|---|---|---|']
    for r in rs:
        fx=', '.join(h.split(' ')[0] for h in r['conserto'][:3]) if r['conserto'] and r['conserto'][0]!='sem commit' else ('sem commit' if r['conserto'] else '-')
        if r['status'] in ('aberta','precisa-log','respondida','por-desenho','fora-do-escopo','duplicada','fechada-sem-resposta') and not r['conserto']: fx='-'
        out.append(f"| [#{r['numero']}]({r['url']}) | {cell(r['titulo'])[:60]} | {cell(r['plataforma'])} | {r['tipo']} | {r['status']} | {cell(r['release'])} | {cell(fx)} | {resp(r)} | {cell(r['proximo_passo'])[:90]} |")
    return '\n'.join(out)
def notas(rs):
    ls=[f"- **#{r['numero']}**: {r['notas']}" for r in rs if r['notas']]
    return ('\n\nNotas:\n\n'+'\n'.join(ls)) if ls else ''
L=[]
L.append(f"# Mapa vivo das issues\n\nBase: `{J['base']}` (integracao/2.0.3). Atualizado em {J['atualizado_em']}. {len(R)} issues (abertas e fechadas) de iqui27/nuvio-native-legacy.\n")
L.append("""## Como atualizar

1. Chegou issue nova, ou um conserto entrou numa branch/tag: edite **uma** entrada em `docs/issues/mapa.json` (procure por `"numero": N`) e rode `python3 docs/issues/mapa.py`, que reescreve este arquivo. Sem o script, edite a linha equivalente aqui.
2. Campos: `numero`, `titulo`, `plataforma` (LG, Samsung .tpk, Samsung .wgt, Android, all, `?`), `tipo` (bug, feature, question, meta), `status`, `release` (2.0.2 ou tag antiga, 2.0.3 = integracao/2.0.3, `2.0.4 (branch)`, 2.1/2.2/futuro, `-`), `conserto` (hashes curtos com a ref entre parênteses), `ultima_resposta` (`nos` = o último comentário é nosso, `data`, `ultimo_comentario_por`, `ultima_nossa`), `proximo_passo`, `notas`.
3. Vocabulário de `status`: `aberta`, `respondida`, `consertada-nao-lancada`, `lancada`, `por-desenho`, `fora-do-escopo`, `precisa-log`, `duplicada`. Extra: `fechada-sem-resposta` (issue fechada sem nenhum comentário).
4. Regra de honestidade: só vale `consertada-nao-lancada`/`lancada` com commit na ref. "Lançado" = commit contido numa tag `v*`. Sem commit, escreva "suspeita" ou "sem commit" em `conserto`/`notas`. "Lançada" em issue antiga sem commit com `#N` quer dizer: a nossa resposta cita uma versão que existe como tag (ver nota na linha).
5. Atenção: mensagens de commit com `(#203)` / `(#204)` falam da VERSÃO 2.0.3 / 2.0.4, não das issues #203/#204. Esses dois números foram ignorados na busca por commits.
6. Próximo passo "postar correção": há rascunhos em `/Volumes/ExternalSSD/tmp/203-respostas-correcao.md` (#302 #350 #286 #312 #368 #360); o dono decide. Correção conhecida: "Dolby Vision in MKV" é DESLIGADO por padrão (a resposta do #312 disse ligado).
""")
c=collections.Counter(r['status'] for r in R)
L.append("## Resumo\n\nPor status:\n\n| Status | Qtd |\n|---|---|")
for k,v in c.most_common(): L.append(f"| {k} | {v} |")
rc=collections.Counter()
for r in R:
    rel=r['release']
    if r['status']=='lancada': rc['lançadas em tag v* (qualquer versão)']+=1
    elif rel.startswith('2.0.3'): rc['2.0.3 (integracao/2.0.3)']+=1
    elif rel.startswith('2.0.4'): rc['2.0.4 (branches)']+=1
    elif rel.startswith(('2.1','2.2','futuro')): rc['futuro (2.1/2.2)']+=1
    else: rc['sem release']+=1
L.append("\nPor release (grupo de planejamento):\n\n| Grupo | Qtd |\n|---|---|")
for k,v in rc.most_common(): L.append(f"| {k} | {v} |")
rl=collections.Counter(r['release'] for r in R if r['status']=='lancada')
L.append("\nLançadas por versão: "+', '.join(f"{k}: {v}" for k,v in sorted(rl.items(),key=lambda kv:vk(kv[0])))+".")
ab=sum(1 for r in R if r['estado_github']=='aberta'); L.append(f"\nAbertas no GitHub: {ab}. Fechadas: {len(R)-ab}.")
L.append(f"Abertas sem nenhum comentário nosso: {sum(1 for r in R if r['estado_github']=='aberta' and not r['ultima_resposta']['ultima_nossa'])}.\n")
sec=[('203','## Sai na 2.0.3 (integracao/2.0.3, ainda não lançada)'),('204','## Planejado na 2.0.4 (com branch)'),('fut','## Futuro (2.1, 2.2, depois)'),('sem','## Aberta sem plano'),('ok','## Já lançado'),('fech','## Fechado sem conserto (por-desenho, fora-do-escopo, duplicada, respondida, sem resposta)')]
for k,t in sec:
    rs=G.get(k,[])
    L.append(f"{t}\n\n{len(rs)} issues.\n")
    if k=='ok':
        a=[r for r in rs if r['estado_github']=='aberta']; f=[r for r in rs if r['estado_github']!='aberta']
        L.append(f"### Lançadas e ainda abertas no GitHub ({len(a)})\n\n"+tab(a)+notas(a)+f"\n\n### Lançadas e fechadas ({len(f)})\n\n"+tab(f)+notas([r for r in f if r['notas']])+"\n")
    else:
        L.append(tab(rs)+notas(rs)+"\n")
L.append("## Fora do GitHub (Reddit)\n")
for o in J['fora_do_github']:
    L.append(f"### {o['titulo']}\n\n- Plataforma: {o['plataforma']}\n- Status: {o['status']}\n- Release: {o['release']}\n- Conserto: "+('; '.join(o['conserto']) if o['conserto'] else 'nenhum encontrado')+f"\n- Próximo passo: {o['proximo_passo']}\n- Notas: {o['notas']}\n")
open(os.path.join(D,'MAPA.md'),'w').write('\n'.join(L))
print('ok',len(R))
