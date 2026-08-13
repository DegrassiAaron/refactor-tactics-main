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

La copertura era parziale, e dal 2026-08-13 non lo e' piu'
----------------------------------------------------------
Alla nascita di questo gate la prosa legacy misurata era **832 occorrenze in 72
file**: molto piu' di quanto una sola PR potesse correggere onestamente. Un gate
rosso ovunque non protegge niente — diventa rumore che si impara a ignorare, e il
repository ha gia' pagato questo difetto (vedi `docs/archive/src/README.md` sul
conteggio dei sorgenti). Percio' partiva con una **allowlist** di tre file.

Quella ragione e' scaduta: l'arretrato bonificabile e' stato chiuso, e cio' che
resta sono **solo** i registri datati, che non si bonificano per definizione. Una
allowlist ora farebbe il danno opposto — elencherebbe i file gia' bonificati e
lascerebbe **muta** una regressione in tutti gli altri, che sono la maggioranza.

⚠️ Nessun conteggio e' scritto qui, ed e' deliberato. La prima stesura ne fissava
due — «47 file» e «175» — ed erano **sbagliati**: derivati a mano sommando i file
toccati da un solo passaggio, mentre i file passati da sporchi a puliti erano
**52** (72 sporchi alla base, meno i 20 registri esenti che restano tali). Il
referto stampa i numeri veri a ogni esecuzione: si leggono li'.

Quindi il gate e' invertito: **tutto e' protetto tranne cio' che e' esente**, e
l'esenzione e' esplicita e motivata. Un file nuovo nasce protetto senza che nessuno
si ricordi di aggiungerlo a una lista.

Il numero dell'arretrato esente si **rimisura** eseguendo lo script: non si copia
da un documento.

Uso
---
    python scripts/check-docs-naming.py             # referto completo
    python scripts/check-docs-naming.py --check     # gate: esce 1 se un file protetto e' sporco
    python scripts/check-docs-naming.py --all       # elenca anche i registri esenti, file per file
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

# Esenzioni: gli unici documenti dove un nome legacy in prosa NON e' un difetto.
#
# ⚠️ Sono **registri datati**, e l'esenzione non e' una dimenticanza. Il Decision
# Log, OPEN_DECISIONS, il changelog, gli ADR e i referti di `roadmap/plans/` sono
# *log*: ogni riga e' un'affermazione con una data, e D-037, D-041, D-058, D-069
# descrivono il roster com'era chiamato quando furono scritte. D-120 le supera nella
# parte nominale; non le rende false a posteriori. Riscriverle per far passare un
# gate sarebbe la stessa falsificazione che `docs/archive/src/README.md` vieta per i
# sorgenti — la correzione e' una **nota accanto all'affermazione**, non una modifica
# del paragrafo.
#
# Tutto il resto e' protetto: e' il roster **corrente** letto da chi apre il file oggi.
EXEMPT_FILES = (
    "docs/decisions/RT_PDR_00_Decision_Log.md",
    "docs/OPEN_DECISIONS.md",
    "docs/CHANGELOG_DOCUMENTATION.md",
)
EXEMPT_PREFIXES = (
    "docs/decisions/adr-",      # una ADR e' datata e firmata: si annota, non si riscrive
    "docs/roadmap/plans/",      # referti e triage, datati nel nome
)


def is_exempt(rel: str) -> bool:
    return rel in EXEMPT_FILES or rel.startswith(EXEMPT_PREFIXES)

# --- maschere: ciò che NON è prosa player-facing -----------------------------
# L'ordine conta: i blocchi recintati spariscono prima degli inline, altrimenti un
# backtick dentro un blocco sbilancia il conteggio degli inline.
#
# ⚠️ La chiusura e' opzionale (`|\Z`). Con `` ```.*?``` `` un blocco **aperto e mai
# chiuso** non veniva mascherato affatto: la regex non trovava il delimitatore finale
# e lasciava tutto il resto del file come prosa, facendo fallire il gate per una
# svista di formattazione invece che per un problema di naming.
FENCED = re.compile(r"```.*?(?:```|\Z)", re.DOTALL)

# ⚠️ **Non esiste una maschera per i blocchi di codice indentati, ed e' deliberato.**
# La versione precedente aveva `^(?: {4,}|\t).*$`, che in Markdown non distingue un
# blocco di codice dalla **continuazione di una lista annidata** — e questo repository
# scrive prosa dentro sotto-punti a ogni pagina. Il risultato misurato era un falso
# negativo silenzioso: un sotto-punto che nominava `Bastion` come personaggio passava
# il gate con exit 0.
#
# Per un gate le due direzioni non hanno lo stesso costo. Un falso **positivo** e'
# rumoroso e si chiude mettendo il token in backtick, cioe' facendo esattamente cio'
# che la convenzione chiede. Un falso **negativo** non lo nota nessuno, ed e' il modo
# in cui un gate smette di proteggere restando verde. I blocchi recintati coprono gia'
# il codice che questi documenti scrivono davvero.
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


# ⚠️ I file di governance stanno FUORI da `docs/`, e questo gate li ha mancati fino
# al 2026-08-13: `AGENTS.md` ha continuato a dichiarare «Roster v0.1 corrente: Flux ·
# Riva · Bastion · Vektor» per un giorno intero dopo D-120, mentre ogni documento
# sotto `docs/` era stato corretto — ed e' il primo file che CLAUDE.md §1 impone di
# leggere. Un gate che copre la periferia e non il centro e' peggio di nessun gate:
# rassicura. Si elencano per nome perche' la root contiene anche kit datati, che
# sarebbero esenti per la stessa ragione dei registri.
ROOT_GOVERNANCE = ("CLAUDE.md", "AGENTS.md", "README.md")


def markdown_files() -> list[Path]:
    files = []
    for name in ROOT_GOVERNANCE:
        path = REPO / name
        if path.is_file():
            files.append(path)
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
        help="esce 1 se un file protetto contiene prosa legacy",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="elenca anche le occorrenze nei registri esenti, file per file",
    )
    args = parser.parse_args()

    files = markdown_files()
    enforced_hits: dict[str, list[tuple[int, str, str]]] = {}
    backlog_hits: dict[str, list[tuple[int, str, str]]] = {}
    enforced_files = 0

    for path in files:
        rel = path.relative_to(REPO).as_posix()
        exempt = is_exempt(rel)
        if not exempt:
            enforced_files += 1
        hits = scan(path)
        if not hits:
            continue
        if exempt:
            backlog_hits[rel] = hits
        else:
            enforced_hits[rel] = hits

    total_files = len(files)
    backlog_count = sum(len(h) for h in backlog_hits.values())

    print(f"File markdown normativi analizzati: {total_files}"
          f" (governance di root + docs/, esclusi {'/'.join(EXCLUDED_DIRS)})")
    if total_files:
        print(f"Protetti dal gate: {enforced_files}/{total_files} file"
              f" — copertura {enforced_files / total_files:.0%}"
              f" · esenti (registri datati): {total_files - enforced_files}")
    else:
        # Checkout parziale o `docs/` spostata: meglio dirlo che dividere per zero.
        print("Nessun file markdown trovato sotto docs/ — controlla il checkout.")
    print()

    if enforced_hits:
        print("ERRORE — prosa legacy in file protetti dal gate:")
        for rel, hits in enforced_hits.items():
            for lineno, name, text in hits:
                print(f"  {rel}:{lineno}: {name} -> {text}")
        print()
    else:
        print("OK — nessun nome legacy come prosa player-facing nei file protetti.")
        print()

    print(f"Nei registri esenti: {backlog_count} occorrenze in {len(backlog_hits)} file.")
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
    print("Le occorrenze nei registri datati non fanno fallire il gate e non si"
          " bonificano: si annotano accanto all'affermazione. Il numero qui sopra si"
          " rimisura eseguendo lo script.")

    if args.check and enforced_hits:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
