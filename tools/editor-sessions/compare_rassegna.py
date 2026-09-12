# tools/editor-sessions/compare_rassegna.py
"""Il gate del verbale: ogni bloccante dichiarato ha la sua prova scritta.

Due direzioni, e non sono simmetriche.

  Un `requires` nello yaml che il verbale non giustifica e' un DIFETTO: un
  check e' uscito dall'ordine del giorno e nessuno ha scritto perche'. Si
  esce non-zero.

  Un check nel bacino che il verbale non nomina NON e' un difetto: e'
  comparso dopo la rassegna, che e' archeologia datata e non un registro da
  tenere aggiornato. Si stampa, e si esce zero.

Invertire le due direzioni sarebbe il difetto: un verbale che DEVE restare
aggiornato diventa la sesta rappresentazione che nessuno legge, ed e' il modo
esatto in cui e' morto `editormap.shortlist.md` (D-181).

    python3 tools/editor-sessions/compare_rassegna.py
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

import oracles
import pie_status
import registry
from registry import Wire

VERBALE = Path("docs/roadmap/plans/sedute-componibili-rassegna-requires-2026-09-12.md")

# Una riga di verbale: | `PIE-XXX` | `SET-YYY` | decisione | prova |
# La classe include le MINUSCOLE per la stessa ragione di `pie_status.RIGA`: nove voci
# reali portano un suffisso minuscolo, e una classe `[A-Z0-9-]` le SALTA in silenzio.
RIGA = re.compile(r"^\|\s*`(PIE-[A-Za-z0-9-]+)`\s*\|[^|]*\|([^|]*)\|")

# Un `requires` ha forma chiusa: <tipo>:<Nome> con un eventuale #<issue>. Estrarre i TOKEN
# invece di cercare sottostringhe e' cio' che impedisce a `asset:WBP_RT_EventLog` di risultare
# giustificato da un verbale che parla di `asset:WBP_RT_EventLogRight`: un nome che e' PREFISSO
# di un altro passerebbe il containment, e il gate tacerebbe proprio dove deve parlare.
PREREQ = re.compile(r"\b(?:" + "|".join(oracles.TIPI) + r"):[A-Za-z0-9_.-]+(?:#\d+)?")


def leggi_verbale(testo: str) -> dict[str, str]:
    """La decisione registrata per ogni check esaminato, come testo."""
    return {m.group(1): m.group(2).strip() for m in map(RIGA.match, testo.splitlines()) if m}


def bacino(wires: list[Wire], stato: dict[str, dict]) -> list[str]:
    """I check cablati e non verdi: quelli che la rassegna doveva esaminare.

    Un check verde e' finito: non c'e' piu' niente da guardare, quindi non ha
    senso chiedersi se sia bloccato. Un check non cablato appartiene alla coda
    scoperta, che e' un'altra misura.
    """
    return sorted(
        w.check
        for w in wires
        if w.check in stato and stato[w.check]["stato"] != pie_status.VERDE
    )


def confronta(verbale: str, wires: list[Wire], stato: dict[str, dict]) -> dict:
    deciso = leggi_verbale(verbale)
    senza_prova = sorted(
        w.check
        for w in wires
        if w.requires and not set(w.requires) <= set(PREREQ.findall(deciso.get(w.check, "")))
    )
    esaminati = set(deciso)
    return {
        "senza_prova": senza_prova,
        "non_esaminati": [c for c in bacino(wires, stato) if c not in esaminati],
        "esaminati": len(esaminati),
        "bloccanti": sum(1 for w in wires if w.requires),
    }


def main() -> int:
    sys.stdout.reconfigure(encoding="utf-8")
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--mattoni", default=registry.MATTONI, type=Path)
    ap.add_argument("--registro", default=pie_status.REGISTRO, type=Path)
    ap.add_argument("--verbale", default=VERBALE, type=Path)
    a = ap.parse_args()

    for p in (a.mattoni, a.registro, a.verbale):
        if not p.exists():
            sys.exit(f"file non trovato: {p}")

    try:
        _, wires = registry.load(a.mattoni)
    except registry.RegistryError as e:
        sys.exit(f"registro dei mattoni rifiutato: {e}")

    stato = pie_status.load(a.registro)
    r = confronta(a.verbale.read_text(encoding="utf-8"), wires, stato)

    print(f"check esaminati dal verbale: {r['esaminati']}   requires dichiarati: {r['bloccanti']}")
    print("COMPARSI DOPO LA RASSEGNA — cablati, non verdi, e il verbale non li nomina:")
    for c in r["non_esaminati"] or ["  (nessuno)"]:
        print(f"  {c}")

    if r["senza_prova"]:
        print("SENZA PROVA — bloccano un check e il verbale non lo giustifica:")
        for c in r["senza_prova"]:
            print(f"  {c}")
        sys.exit(
            "\nUn bloccante senza prova toglie un check dall'ordine del giorno senza dire perche'. "
            f"Scrivi la riga in {a.verbale}, oppure togli il `requires`."
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
