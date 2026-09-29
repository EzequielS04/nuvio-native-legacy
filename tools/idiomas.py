#!/usr/bin/env python3
"""Confere as tabelas de traducao da interface (src/idioma_tab.h e irmas).

ESTRUTURA. idioma_tab.h e a tabela mestra: { chave em portugues, ingles },
ORDENADA por strcmp dos BYTES DECODIFICADOS (aspas = 0x22, \\n = 0x0A — nao o
texto literal com a barra). idioma_ro.h, idioma_uk.h e idioma_ru.h tem UMA
linha por entrada da mestra, na MESMA ordem:  T("chave pt", "traducao").
O compilador descarta a chave (macro T); ela existe para o revisor humano ler
a linha inteira e para este script conferir que o alinhamento nao escorregou.

O QUE CONFERE (sai com codigo 1 no primeiro defeito de qualquer idioma):
  1. mesma quantidade de entradas em todas as tabelas;
  2. a mestra esta em ordem estrita de strcmp (fora de ordem = idioma.c
     DESLIGA a traducao inteira, em silencio);
  3. a chave de cada linha das irmas e IDENTICA a da mestra, na mesma posicao;
  4. mesmos marcadores printf (%s %d %.1f %% ...) NA MESMA ORDEM que a chave —
     o codigo passa os argumentos na ordem do portugues;
  5. nenhum valor vazio;
  6. mesmo numero de \\n que a chave (quebra de linha e layout);
  7. o valor e UTF-8 valido e nao tem barra solta / aspa sem escape;
  8. mesmos espacos na borda que a chave (" carregados" e uma chave de
     verdade: o espaco da frente separa de um numero desenhado antes);
  9. ucraniano e russo tem de conter cirilico, salvo o que e nome proprio,
     sigla ou formato (VERBATIM abaixo) ou igual a chave/ao ingles — texto em
     alfabeto latino ali e traducao que ficou por fazer.

Uso:
    python3 tools/idiomas.py                 # confere tudo
    python3 tools/idiomas.py --sincronizar   # reescreve as irmas com as chaves
                                             # da mestra (entrada nova = vazia,
                                             # que o passo 5 recusa)
    python3 tools/idiomas.py --revisao ro    # pt | en | ro lado a lado
"""
import re, sys, pathlib

RAIZ = pathlib.Path(__file__).resolve().parent.parent
SRC = RAIZ / "src"
IDIOMAS = ("ro", "uk", "ru")

LIT = r'"((?:[^"\\]|\\.)*)"'
LINHA_MESTRA = re.compile(r'^\s*\{\s*' + LIT + r'\s*,\s*' + LIT + r'\s*\},?\s*$')
LINHA_IRMA = re.compile(r'^\s*T\(\s*' + LIT + r'\s*,\s*' + LIT + r'\s*\),?\s*$')
# Valores sem cirilico que sao intencionais no uk/ru: os que nao sao igual ao
# ingles nem a chave. "S%dE%d" e o formato de temporada/episodio que o mundo todo
# le assim (a chave e T%dE%d, do portugues).
VERBATIM = {"Watchlist Trakt", "S%dE%d", "S%d:E%d", "%s S%dE%d", "%s — S%dE%d%s%s",
            "S%dE%d  ·  %s%s%.22s", "S%dE%d · %s", "S%dE%d%s%s", "Português (Brasil)",
            "Português (Portugal)", "Français"}
SIMPLES = {"n": 10, "t": 9, "r": 13, "0": 0, '"': 34, "\\": 92, "'": 39}

def decodificar(s):
    """Literal C -> bytes, como o compilador decodifica (\\xNN, \\uNNNN, simples)."""
    out, i, n = bytearray(), 0, len(s)
    while i < n:
        c = s[i]
        if c != "\\":
            out += c.encode("utf-8"); i += 1; continue
        i += 1
        c = s[i]
        if c == "x":
            j = i + 1
            while j < n and s[j] in "0123456789abcdefABCDEF": j += 1
            out.append(int(s[i + 1:j], 16) & 0xFF); i = j
        elif c == "u":
            out += chr(int(s[i + 1:i + 5], 16)).encode("utf-8"); i += 5
        elif c in SIMPLES:
            out.append(SIMPLES[c]); i += 1
        else:
            raise ValueError("escape desconhecido \\" + c)
    return bytes(out)

MARCADOR = re.compile(rb"%[-+ #0]*[0-9]*(?:\.[0-9]+)?(?:hh|h|ll|l|z|j|t)?[diouxXeEfgGcsp%]")
def marcadores(b):
    return [m for m in MARCADOR.findall(b) if m != b"%%"]

def ler_mestra():
    itens = []
    for n, linha in enumerate((SRC / "idioma_tab.h").read_text(encoding="utf-8").splitlines(), 1):
        if linha.lstrip().startswith("//") or not linha.strip():
            continue
        m = LINHA_MESTRA.match(linha)
        if not m:
            raise SystemExit("idioma_tab.h:%d: linha fora do formato { \"pt\", \"en\" }" % n)
        itens.append((n, m.group(1), m.group(2)))
    return itens

def ler_irma(cod):
    caminho = SRC / ("idioma_%s.h" % cod)
    itens = []
    if not caminho.exists():
        return itens
    for n, linha in enumerate(caminho.read_text(encoding="utf-8").splitlines(), 1):
        if linha.lstrip().startswith("//") or not linha.strip():
            continue
        m = LINHA_IRMA.match(linha)
        if not m:
            raise SystemExit("idioma_%s.h:%d: linha fora do formato T(\"pt\", \"traducao\")" % (cod, n))
        itens.append((n, m.group(1), m.group(2)))
    return itens

def conferir():
    erros = []
    def erro(msg):
        if len(erros) < 60: erros.append(msg)
    mestra = ler_mestra()
    # 2. ordem da mestra, pelos bytes decodificados
    ant = None
    for n, pt, en in mestra:
        b = decodificar(pt)
        if ant is not None and ant >= b:
            erro("idioma_tab.h:%d: fora de ordem (ou duplicada): %r" % (n, pt))
        ant = b
    # mestra: en com os mesmos marcadores e \n da chave
    for n, pt, en in mestra:
        if not en: erro("idioma_tab.h:%d: ingles vazio" % n)
        if marcadores(decodificar(pt)) != marcadores(decodificar(en)):
            erro("idioma_tab.h:%d: marcadores do ingles diferem: %r -> %r" % (n, pt, en))
    for cod in IDIOMAS:
        irma = ler_irma(cod)
        arq = "idioma_%s.h" % cod
        # 1. quantidade
        if len(irma) != len(mestra):
            erro("%s: %d entradas, a mestra tem %d" % (arq, len(irma), len(mestra)))
        for (n, pt, en), (m, kpt, val) in zip(mestra, irma):
            # 3. alinhamento
            if kpt != pt:
                erro("%s:%d: chave difere da mestra (linha %d): %r != %r" % (arq, m, n, kpt, pt)); break
            try:
                bv = decodificar(val); bk = decodificar(pt)
                bv.decode("utf-8")
            except Exception as e:
                erro("%s:%d: valor invalido (%s): %r" % (arq, m, e, val)); continue
            # 5. vazio
            if not bv.strip():
                erro("%s:%d: valor vazio para %r" % (arq, m, pt)); continue
            # 4. marcadores, na ordem
            if marcadores(bk) != marcadores(bv):
                erro("%s:%d: marcadores diferem: %r -> %r" % (arq, m, pt, val))
            # 8. espacos da borda
            if (len(val) - len(val.lstrip()), len(val) - len(val.rstrip())) != \
               (len(pt) - len(pt.lstrip()), len(pt) - len(pt.rstrip())):
                erro("%s:%d: espacos na borda diferem de %r: %r" % (arq, m, pt, val))
            # 9. cirilico
            if cod in ("uk", "ru") and not re.search("[\u0400-\u04ff]", val) \
               and val not in (pt, en) and val not in VERBATIM:
                erro("%s:%d: sem cirilico e diferente da chave e do ingles: %r -> %r" % (arq, m, pt, val))
            # 6. quebras de linha
            if bk.count(b"\n") != bv.count(b"\n"):
                erro("%s:%d: numero de \\n difere: %r -> %r" % (arq, m, pt, val))
    return mestra, erros

def sincronizar():
    mestra = ler_mestra()
    for cod in IDIOMAS:
        atual = {pt: val for _, pt, val in ler_irma(cod)}
        linhas = ["// Traducao para %s. Uma linha por entrada de idioma_tab.h, na mesma ordem." % cod,
                  "// Gerado por tools/idiomas.py --sincronizar; a chave e so para leitura (macro T)."]
        for _, pt, _en in mestra:
            linhas.append('  T("%s", "%s"),' % (pt, atual.get(pt, "")))
        (SRC / ("idioma_%s.h" % cod)).write_text("\n".join(linhas) + "\n", encoding="utf-8")

def revisao(cod):
    mestra = ler_mestra()
    irma = ler_irma(cod)
    for (n, pt, en), (m, _k, val) in zip(mestra, irma):
        print("%s\n   en: %s\n   %s: %s" % (pt, en, cod, val))

if __name__ == "__main__":
    if "--sincronizar" in sys.argv: sincronizar(); sys.exit(0)
    if "--revisao" in sys.argv: revisao(sys.argv[sys.argv.index("--revisao") + 1]); sys.exit(0)
    mestra, erros = conferir()
    for e in erros: print("ERRO", e)
    if erros:
        print("idiomas: %d problema(s)" % len(erros)); sys.exit(1)
    print("idiomas: %d entradas x %d idiomas (%s) ok" % (len(mestra), len(IDIOMAS), ", ".join(IDIOMAS)))
