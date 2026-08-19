#!/usr/bin/env python3
"""Gate anti-deriva: una tabella Markdown non perde righe per strada.

Nasce dal difetto **#870** del 2026-08-14: `RT_PDR_00_Decision_Log.md` aveva l'intestazione
alla riga 16 e diciassette righe vuote sparse fra una decisione e la successiva. In GFM una
riga vuota **termina** la tabella, e le righe `| ... |` successive non hanno piu' un header
sopra: non sono una tabella, sono un paragrafo coi pipe in vista.

La pagina servita da GitHub rendeva **una tabella con 11 righe** su 144. Centotrentatre righe
di decisioni erano testo grezzo, e nessun gate lo vedeva: `check-docs-links.py` guarda i link,
`check-docs-symbols.py` i simboli, `rt_shared_id.py` i duplicati di ID. La struttura di una
tabella non la guardava nessuno.

E' un difetto che **ricompare da solo**: cresce quando il documento cresce, e chi aggiunge una
riga vuota per respiro visivo ne butta fuori centotrenta senza accorgersene.

Uso:
    python scripts/check-docs-tables.py             # dalla radice del repo
    python scripts/check-docs-tables.py --list      # elenca anche i documenti esentati
    python scripts/check-docs-tables.py --fix       # rimuove le righe vuote che spezzano

Esce con codice 1 se un documento normativo contiene righe di tabella rese come testo.

Il criterio non e' dedotto: e' stato validato contro il parser vero di GitHub
(`POST /markdown`) su cinque casi limite, e due euristiche piu' semplici sono cadute li'.
  - una riga che va **a capo** dentro una tabella non la termina (GFM la rende come riga
    nuova); solo una riga **vuota** la termina;
  - un diagramma ASCII con un pipe per riga non e' una tabella;
  - le righe dentro un code fence non sono tabelle.

Esenzioni, per costruzione e non per elenco a mano — stesse cartelle di `check-docs-symbols.py`:
`docs/archive/` e' storico, `docs/research/` e' input non consumato, e `docs/roadmap/plans/` sono
piani datati. Un documento storico puo' contenere una tabella rotta:
descrive com'era. L'elenco vive **una volta sola**, in `EXEMPT_DIRS`: questa riga lo descrive e non
lo duplica — un elenco scritto due volte diverge alla prima aggiunta.
"""
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DOCS = os.path.join(REPO, "docs")

# ⚠️ Esenzioni per **prefisso di path**: spostare un documento ne cambia la copertura in silenzio.
# Misurato il 2026-08-17 con `EXEMPT_DIRS = ()`: lo scope passa da 165 a 404 documenti e il gate esce
# 1 su sei file (i quattro PRD, oggi in `docs/research/prd/`, `docs/roadmap/plans/h5c7-flood-fill-spec.md`,
# `docs/archive/pdr-v0.1/RT_PDR_v0.1_consolidato.md`).
# ➕ `docs/research/` dal 2026-08-18, ➖ `docs/src/` dal 2026-08-19 perche' svuotata: **165 documenti**
# prima e dopo, che e' il modo di dimostrare che uno spostamento non ha spento un pezzo di gate.
# Vedi la nota gemella in `check-docs-symbols.py`, che spiega anche perche' `docs/generated/` resta
# **fuori**.
EXEMPT_DIRS = ("docs/archive/", "docs/research/", "docs/roadmap/plans/")

# Un delimitatore GFM: `|---|---|`, `| :--- | ---: |`, con o senza pipe ai bordi.
DELIM_RE = re.compile(r"^\s*\|?(\s*:?-+:?\s*\|)+(\s*:?-+:?\s*)?\|?\s*$")

# Il prefisso di un blockquote: una tabella dentro `>` e' una tabella a tutti gli effetti.
QUOTE_RE = re.compile(r"^\s*(>\s?)+")


def unquote(s):
    """Toglie i marcatori `>` di blockquote; il resto e' Markdown normale."""
    return QUOTE_RE.sub("", s)


def is_table_line(s):
    """Una riga che GFM puo' leggere come riga di tabella.

    Deve **cominciare** con `|`, non solo contenerlo. Contare i pipe e basta segnalava
    `<!-- ISSUE slug=E1 labels=v0.1|epic|P0 -->` e il codice inline
    `` `git show $b:file | head` ``: centotredici falsi positivi in un documento solo,
    e un gate rumoroso viene disattivato al terzo.

    Servono comunque due pipe: con uno solo si prendono i diagrammi ASCII (`A ----+`),
    che GitHub non rende affatto come tabella.
    """
    body = unquote(s)
    if not body.lstrip().startswith("|") or body.count("|") < 2:
        return False
    # quattro spazi di indentazione sono un code block, non una tabella
    return not body.startswith("    ")


def paragraphs(text):
    """(numero_di_riga_1based, [righe]) per ogni paragrafo fuori dai code fence.

    Il paragrafo — non il blocco contiguo di righe `|` — e' l'unita' giusta: in GFM la
    tabella arriva fino alla prossima riga VUOTA, e una riga andata a capo nel mezzo non
    la interrompe.
    """
    out, cur, start, fence = [], [], 1, None
    for n, line in enumerate(text.split("\n"), 1):
        # `unquote` anche qui: un code fence dentro un blockquote (`> ```bash`) e' un fence,
        # e le righe di continuazione di un comando shell (`>   | grep -o ...`) non sono
        # tabelle. Senza, il gate segnalava due documenti che citano una pipeline di shell.
        stripped = unquote(line).lstrip()
        if fence is None and (stripped.startswith("```") or stripped.startswith("~~~")):
            fence = stripped[:3]
            if cur:
                out.append((start, cur))
                cur = []
            continue
        if fence is not None:
            if stripped.startswith(fence):
                fence = None
            continue
        # Dentro un blockquote il separatore di paragrafo e' una riga `>` **vuota**, non una
        # riga vuota: senza `unquote` il paragrafo ingloba la prosa che sta sopra la tabella,
        # la sua seconda riga non e' un delimitatore, e il gate segnala tabelle sane. Erano
        # quindici documenti — tutti gli ADR con un banner di stato in testa.
        if unquote(line).strip() == "":
            if cur:
                out.append((start, cur))
                cur = []
            continue
        if not cur:
            start = n
        cur.append(line)
    if cur:
        out.append((start, cur))
    return out


def orphan_rows(text):
    """[(riga, testo)] delle righe di tabella che GitHub NON rendera' in una <table>."""
    bad = []
    for start, block in paragraphs(text):
        rows = [(start + i, l) for i, l in enumerate(block) if is_table_line(l)]
        if not rows:
            continue
        # valido se la seconda riga del paragrafo e' un delimitatore (anche dentro blockquote)
        if len(block) >= 2 and DELIM_RE.match(unquote(block[1]).strip()):
            continue
        bad.extend(rows)
    return bad


def fix(text):
    """Rimuove le righe vuote che stanno fra due righe di tabella. Solo cancellazioni."""
    lines = text.split("\n")

    def next_nonblank(k):
        """(indice, riga) della prima riga non vuota da k in avanti."""
        for j in range(k, len(lines)):
            if lines[j].strip():
                return j, lines[j]
        return None, ""

    drop, fence = set(), None
    for i, line in enumerate(lines):
        stripped = unquote(line).lstrip()
        if fence is None and (stripped.startswith("```") or stripped.startswith("~~~")):
            fence = stripped[:3]
            continue
        if fence is not None:
            if stripped.startswith(fence):
                fence = None
            continue
        if line.strip() != "":
            continue
        prev = next((lines[j] for j in range(i - 1, -1, -1) if lines[j].strip()), "")
        k, nxt = next_nonblank(i + 1)
        if not (is_table_line(prev) and is_table_line(nxt)):
            continue          # confina con della prosa: separatore legittimo
        if DELIM_RE.match(unquote(nxt).strip()):
            continue          # la riga vuota separa l'header dal suo delimitatore: non e' mia
        # `nxt` e' l'HEADER di una tabella nuova se la riga dopo di lui e' un delimitatore.
        # Senza questo controllo il fix **fondeva due tabelle distinte** — legittime, ciascuna
        # col suo header — in una sola, silenziosamente. Trovato in code review, non in uso.
        _, dopo = next_nonblank(k + 1)
        if DELIM_RE.match(unquote(dopo).strip()):
            continue
        drop.add(i)
    return "\n".join(l for i, l in enumerate(lines) if i not in drop), len(drop)


def control_bytes(raw):
    """[(riga, byte)] dei byte di controllo: fermano il rendering di GitHub a meta' pagina.

    Non e' il difetto di #870, ma ne e' il gemello per conseguenza — e piu' silenzioso.
    Un solo NUL alla riga 45 di `asset-map.md` faceva rendere **zero** delle sue cinque
    tabelle: il documento appariva intero nel sorgente e mutilato sulla pagina. La
    bisezione l'ha trovato in dieci render; nessun controllo del repository lo vedeva.
    """
    return [(raw[:i].count(b"\n") + 1, b) for i, b in enumerate(raw)
            if b < 0x20 and b not in (0x09, 0x0A, 0x0D)]


def main():
    do_fix = "--fix" in sys.argv
    broken, checked, exempt, fixed = {}, 0, [], 0
    ctrl = {}

    for root, _dirs, files in os.walk(DOCS):
        for fn in sorted(files):
            if not fn.endswith(".md"):
                continue
            abs_p = os.path.join(root, fn)
            rel = os.path.relpath(abs_p, REPO).replace("\\", "/")
            if rel.startswith(EXEMPT_DIRS):
                exempt.append(rel)
                continue
            checked += 1
            cb = control_bytes(open(abs_p, "rb").read())
            if cb:
                ctrl[rel] = cb
            text = open(abs_p, encoding="utf-8").read()
            bad = orphan_rows(text)
            if not bad:
                continue
            if do_fix:
                new, n = fix(text)
                if n and not orphan_rows(new):
                    # invariante: nessuna riga di contenuto persa
                    assert [l for l in text.split("\n") if l.strip()] == \
                           [l for l in new.split("\n") if l.strip()], f"contenuto alterato in {rel}"
                    open(abs_p, "w", encoding="utf-8", newline="\n").write(new)
                    fixed += 1
                    print(f"  corretto  {rel}  ({n} righe vuote rimosse)")
                    continue
            broken[rel] = bad

    if "--list" in sys.argv:
        print(f"Esentati ({len(exempt)}):")
        for e in exempt:
            print("   ", e)
        print()

    if do_fix:
        print(f"\nDocumenti corretti: {fixed}")

    print(f"Documenti normativi controllati: {checked}")
    if not broken and not ctrl:
        print("OK — nessuna riga di tabella resa come testo.")
        return 0

    if ctrl:
        print(f"\nFALLITO — byte di controllo in {len(ctrl)} documenti.")
        print("GitHub smette di rendere la pagina dove li incontra: le tabelle "
              "successive spariscono tutte.\n")
        for rel in sorted(ctrl):
            for n, b in ctrl[rel][:6]:
                print(f"  {rel}:{n}  byte 0x{b:02X}")
        print("\n  Di solito e' un escape scritto letteralmente in un blocco di codice:")
        print("  `split('<NUL>')` invece di `split('\\\\0')`. Sostituisci il byte con la "
              "sua forma testuale.")
        if not broken:
            return 1
        print()

    tot = sum(len(v) for v in broken.values())
    print(f"\nFALLITO — {tot} righe di tabella rese come TESTO in {len(broken)} documenti.")
    print("GitHub non le mostrera' in una tabella: appariranno coi pipe in vista.\n")
    for rel in sorted(broken):
        print(f"  {rel}")
        for n, line in broken[rel][:6]:
            print(f"      :{n}  {line.strip()[:84]}")
        if len(broken[rel]) > 6:
            print(f"      ... e altre {len(broken[rel]) - 6} righe")
    print("\nDue cause possibili:")
    print("  1. una riga VUOTA spezza la tabella  -> rimuovila (`--fix` lo fa in automatico);")
    print("  2. il blocco non ha header+delimitatore -> aggiungi `| A | B |` e `|---|---|`.")
    print("\nUn documento storico o propositivo va sotto docs/archive/ o docs/src/.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
