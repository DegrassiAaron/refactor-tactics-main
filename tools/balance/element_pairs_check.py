#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Validatore di copertura del catalogo delle coppie elementali (D-275, D-588, D-593; #1803).

Legge `docs/balance/RT_ElementPairCatalog_v0.1.md`, trova la tabella delle coppie e verifica:
  1. gli elementi sono esattamente i dodici di D-593, nell'ordine dichiarato in testa al catalogo;
  2. ogni coppia NON ordinata di elementi distinti compare esattamente una volta (12·11/2 = 66);
  3. ogni voce ha uno e un solo stato fra CURRENT, DESIGNED, OPEN, NO_INTERACTION;
  4. una voce CURRENT o DESIGNED porta un ReactionId; una voce OPEN o NO_INTERACTION non ne porta.

Uso: python tools/balance/element_pairs_check.py [percorso-del-catalogo]
Esce con 0 se tutto regge, 1 altrimenti, e stampa ogni difetto con la riga.
"""
import io, os, re, sys

ELEMENTS = ["Warp", "Fire", "Water", "Earth", "Electric", "Frost", "Decay", "Life", "Arcane", "Aegis", "Air", "Disintegration"]
STATES = {"CURRENT", "DESIGNED", "OPEN", "NO_INTERACTION"}
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DEFAULT = os.path.join(ROOT, "docs", "balance", "RT_ElementPairCatalog_v0.1.md")


def main(path):
    text = io.open(path, encoding="utf-8").read().replace("\r\n", "\n")
    errors = []
    seen = {}
    rows = 0
    in_pairs = False
    for n, line in enumerate(text.split("\n"), 1):
        if line.startswith("## "):
            in_pairs = line.startswith("## Le coppie")
            continue
        if not in_pairs or not line.startswith("| `Element."):
            continue
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if len(cells) < 4:
            errors.append("riga %d: colonne insufficienti" % n); continue
        a, b, state, reaction = cells[0], cells[1], cells[2], cells[3]
        ma, mb = re.fullmatch(r"`Element\.(\w+)`", a), re.fullmatch(r"`Element\.(\w+)`", b)
        if not ma or not mb:
            errors.append("riga %d: identificatori non nella forma `Element.<X>`" % n); continue
        ea, eb = ma.group(1), mb.group(1)
        for e in (ea, eb):
            if e not in ELEMENTS:
                errors.append("riga %d: `%s` non e' uno dei dodici elementi di D-593" % (n, e))
        if ea == eb:
            errors.append("riga %d: coppia di un elemento con se stesso (D-275 conta coppie di elementi distinti)" % n)
        key = tuple(sorted((ea, eb)))
        if key in seen:
            errors.append("riga %d: coppia %s gia' presente alla riga %d" % (n, "+".join(key), seen[key]))
        seen[key] = n
        states_found = [s for s in STATES if re.search(r"\b%s\b" % s, state)]
        if len(states_found) != 1:
            errors.append("riga %d: stato non univoco (%s)" % (n, state))
            continue
        st = states_found[0]
        has_reaction = bool(re.search(r"`Reaction\.[\w.]+`", reaction))
        if st in ("CURRENT", "DESIGNED") and not has_reaction:
            errors.append("riga %d: stato %s senza `Reaction.<Id>`" % (n, st))
        if st in ("OPEN", "NO_INTERACTION") and has_reaction:
            errors.append("riga %d: stato %s con un `Reaction.<Id>`: una voce aperta non porta effetti" % (n, st))
        rows += 1
    expected = len(ELEMENTS) * (len(ELEMENTS) - 1) // 2
    missing = []
    for i, x in enumerate(ELEMENTS):
        for y in ELEMENTS[i + 1:]:
            if (tuple(sorted((x, y)))) not in seen:
                missing.append(x + "+" + y)
    if missing:
        errors.append("coppie mancanti (%d): %s" % (len(missing), " ".join(missing)))
    if len(seen) != expected:
        errors.append("coppie distinte trovate: %d, attese %d" % (len(seen), expected))
    counts = {}
    in_pairs = False
    for line in text.split("\n"):
        if line.startswith("## "):
            in_pairs = line.startswith("## Le coppie"); continue
        if in_pairs and line.startswith("| `Element."):
            for s in STATES:
                if re.search(r"\|\s*%s\b" % s, line):
                    counts[s] = counts.get(s, 0) + 1
    print("righe: %d · coppie distinte: %d/%d · stati: %s" % (rows, len(seen), expected, ", ".join("%s=%d" % kv for kv in sorted(counts.items()))))
    for e in errors:
        print("DIFETTO:", e)
    return 0 if not errors else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1] if len(sys.argv) > 1 else DEFAULT))
