#!/usr/bin/env python3
"""Gate documentale sul naming del roster (D-120).

D-120 fissa i nomi canonici/player-facing del roster v0.1 — Gadget, Phase, Riktor,
Wraith — e declassa Flux/Riva/Bastion/Vektor a *legacy implementation identifiers*.
Quei token restano legittimi dove nominano qualcosa che esiste davvero in codice,
asset, scenario o replay: `Hero.Flux`, `Vektor.InterceptShot`, `Spec.State.Riva.Flow`.
Non sono piu' legittimi come **nome del personaggio in prosa**.

Questo script cerca esattamente quella differenza: un nome legacy che compare
*fuori* da un contesto tecnico. Dentro backtick, dentro un blocco di codice, dentro
un percorso o un URL non e' un'occorrenza player-facing e non viene segnalata.

Perche' non e' un search/replace
--------------------------------
Un replace globale romperebbe simboli C++, Stable ID, link e riferimenti storici
insieme: D-120 lo vieta esplicitamente. Il gate segnala, non riscrive.

Perche' la copertura e' parziale *per scelta*
---------------------------------------------
Al momento di scrivere questo gate la prosa legacy misurata e' molto piu' grande di
quanto una sola PR possa correggere onestamente. Un gate rosso ovunque non protegge
niente: diventa rumore che si impara a ignorare, e il repository ha gia' pagato
questo difetto (vedi `docs/archive/src/README.md` sul conteggio dei sorgenti).

Quindi il gate ha due modi:

* i file in ENFORCED sono **puliti e vanno tenuti puliti**: una violazione li' e'
  un errore e fa uscire 1;
* tutto il resto e' misurato e stampato come arretrato, senza far fallire il gate.

Un file si sposta in ENFORCED quando e' stato bonificato, mai prima. Il numero
dell'arretrato si **rimisura** eseguendo lo script: non si copia da un documento.

Uso
---
    python scripts/check-docs-naming.py             # referto completo
    python scripts/check-docs-naming.py --check     # gate: esce 1 se ENFORCED e' sporco
    python scripts/check-docs-naming.py --all       # elenca anche l'arretrato, file per file
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
DOCS = REPO / "docs"

# La prosa citata nel referto contiene emoji e accenti; la console Windows e' cp1252
# e alzerebbe UnicodeEncodeError a meta' elenco, cioe' proprio quando il gate ha
# qualcosa da dire. Il referto degrada il carattere, non si interrompe.
for _stream in (sys.stdout, sys.stderr):
    if hasattr(_stream, "reconfigure"):
        _stream.reconfigure(errors="replace")

# I quattro identificatori declassati da D-120.
LEGACY = ("Flux", "Riva", "Bastion", "Vektor")

# Cartelle che il gate non guarda affatto.
#   archive/  -> storico: il testo originale non si riscrive (regola di archive/src/README.md)
#   src/      -> input non ancora recepito, non e' autorita' (CLAUDE.md §1)
EXCLUDED_DIRS = ("archive", "src")

# File bonificati e da tenere puliti. Cresce, non si svuota.
#
# ⚠️ I registri datati NON entrano qui, e non e' una dimenticanza. Il Decision Log e
# OPEN_DECISIONS sono *log*: ogni riga e' un'affermazione con una data, e D-037,
# D-041, D-058, D-069 descrivono il roster com'era chiamato quando furono scritte.
# D-120 le supera nella parte nominale; non le rende false a posteriori. Riscriverle
# per far passare un gate sarebbe la stessa falsificazione che
# `docs/archive/src/README.md` vieta per i sorgenti — la correzione e' una nota
# accanto all'affermazione, non una modifica del paragrafo.
#
# Qui entrano i documenti che descrivono il roster **corrente** a chi legge oggi.
ENFORCED = (
    "docs/characters/index.md",
    "docs/characters/README.md",
    "docs/roadmap/roadmap-v0.1.md",
)

# --- maschere: ciò che NON è prosa player-facing -----------------------------
# L'ordine conta: i blocchi recintati spariscono prima degli inline, altrimenti un
# backtick dentro un blocco sbilancia il conteggio degli inline.
FENCED = re.compile(r"```.*?```", re.DOTALL)
INDENTED = re.compile(r"^(?: {4,}|\t).*$", re.MULTILINE)
INLINE_CODE = re.compile(r"`[^`\n]*`")
HTML_COMMENT = re.compile(r"<!--.*?-->", re.DOTALL)
# Target di link e immagini: `](qualcosa)` — il testo dell'etichetta resta visibile.
LINK_TARGET = re.compile(r"\]\([^)\n]*\)")
BARE_URL = re.compile(r"https?://\S+")
# Percorsi nudi tipo docs/characters/v0.1/flux.md, anche senza backtick.
BARE_PATH = re.compile(r"\b[\w./-]+\.(?:md|cpp|h|py|ts|json|ya?ml|uasset|umap)\b")


def mask(text: str) -> str:
    """Sostituisce i contesti tecnici con spazi, preservando gli offset di riga."""

    def blank(match: re.Match) -> str:
        # Le newline restano: il numero di riga deve continuare a tornare.
        return re.sub(r"[^\n]", " ", match.group(0))

    for pattern in (
        HTML_COMMENT,
        FENCED,
        INDENTED,
        INLINE_CODE,
        LINK_TARGET,
        BARE_URL,
        BARE_PATH,
    ):
        text = pattern.sub(blank, text)
    return text


NAME_RE = re.compile(r"\b(" + "|".join(LEGACY) + r")\b")


def scan(path: Path) -> list[tuple[int, str, str]]:
    """Ritorna [(riga, nome, testo)] delle occorrenze player-facing."""
    try:
        raw = path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError):
        return []

    masked = mask(raw)
    raw_lines = raw.splitlines()
    hits: list[tuple[int, str, str]] = []
    for lineno, line in enumerate(masked.splitlines(), start=1):
        for match in NAME_RE.finditer(line):
            original = raw_lines[lineno - 1].strip()
            hits.append((lineno, match.group(1), original[:120]))
    return hits


def markdown_files() -> list[Path]:
    files = []
    for path in sorted(DOCS.rglob("*.md")):
        rel_parts = path.relative_to(DOCS).parts
        if rel_parts and rel_parts[0] in EXCLUDED_DIRS:
            continue
        files.append(path)
    return files


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check",
        action="store_true",
        help="esce 1 se un file ENFORCED contiene prosa legacy",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="elenca anche l'arretrato, file per file",
    )
    args = parser.parse_args()

    files = markdown_files()
    enforced_hits: dict[str, list[tuple[int, str, str]]] = {}
    backlog_hits: dict[str, list[tuple[int, str, str]]] = {}

    for path in files:
        rel = path.relative_to(REPO).as_posix()
        hits = scan(path)
        if not hits:
            continue
        if rel in ENFORCED:
            enforced_hits[rel] = hits
        else:
            backlog_hits[rel] = hits

    total_files = len(files)
    enforced_present = sum(1 for f in ENFORCED if (REPO / f).exists())
    backlog_count = sum(len(h) for h in backlog_hits.values())

    print(f"File markdown normativi analizzati: {total_files}"
          f" (esclusi {'/'.join(EXCLUDED_DIRS)})")
    print(f"Sotto gate ENFORCED: {enforced_present}/{total_files} file"
          f" — copertura {enforced_present / total_files:.0%}")
    print()

    if enforced_hits:
        print("ERRORE — prosa legacy in file che devono restare puliti:")
        for rel, hits in enforced_hits.items():
            for lineno, name, text in hits:
                print(f"  {rel}:{lineno}: {name} -> {text}")
        print()
    else:
        print("OK — nessun nome legacy come prosa player-facing nei file ENFORCED.")
        print()

    print(f"Arretrato fuori gate: {backlog_count} occorrenze in {len(backlog_hits)} file.")
    if backlog_hits:
        ordered = sorted(backlog_hits.items(), key=lambda kv: -len(kv[1]))
        if args.all:
            for rel, hits in ordered:
                print(f"  {rel} ({len(hits)})")
                for lineno, name, text in hits:
                    print(f"      :{lineno}: {name} -> {text}")
        else:
            print("  primi dieci per densita' (--all per l'elenco completo):")
            for rel, hits in ordered[:10]:
                print(f"      {len(hits):4d}  {rel}")
    print()
    print("L'arretrato non fa fallire il gate: si bonifica un file per volta e lo si"
          " aggiunge a ENFORCED. Il numero qui sopra si rimisura eseguendo lo script.")

    if args.check and enforced_hits:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
