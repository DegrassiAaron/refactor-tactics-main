# tools/editor-sessions/compare_rassegna.py
"""Il gate del verbale: ogni bloccante dichiarato ha la sua prova scritta.

Due direzioni, e non sono simmetriche.

  Un `requires` nello yaml che il verbale non giustifica e' un DIFETTO: un
  check e' uscito dall'ordine del giorno e nessuno ha scritto perche'. Si
  esce non-zero. «Giustificato» vuol dire DUE cose, non una: la colonna
  decisione ripete il token del `requires` (§ `PREREQ`), E la colonna prova
  non e' vuota. La prima da sola non basta — una riga puo' ripetere il token
  in decisione e lasciare la prova vuota, ed e' esattamente il buco che
  questo gate esiste per chiudere.

  Un check nel bacino che il verbale non nomina NON e' un difetto: e'
  comparso dopo la rassegna, che e' archeologia datata e non un registro da
  tenere aggiornato. Si stampa, e si esce zero.

Invertire le due direzioni sarebbe il difetto: un verbale che DEVE restare
aggiornato diventa la sesta rappresentazione che nessuno legge, ed e' il modo
esatto in cui e' morto `editormap.shortlist.md` (D-181).

⚠️ Il confronto e' per TOKEN esatto (vedi `PREREQ` sotto), non per sottostringa
libera: una decisione scritta in prosa naturale come
`` `mount:WBP_RT_EventLogRight` (#2697) `` — col numero fuori dal token, dopo
uno spazio — NON giustifica un `requires: [ mount:WBP_RT_EventLogRight#2697 ]`,
perche' il token estratto e' `mount:WBP_RT_EventLogRight` senza `#2697`. Il
gate fallisce su quel verbale, anche se un lettore umano lo leggerebbe come
corretto. E' voluto: fallire rumorosamente su un verbale scritto in una forma
imprecisa e' la direzione sicura, l'alternativa (allentare il confronto) e' il
modo in cui un bloccante ingiustificato passerebbe zitto. Scrivi la decisione
coi token ESATTI, nella forma che `docs/roadmap/sedute-mattoni.yaml` dichiara:
`mount:WBP_RT_EventLogRight#2697`, non `mount:WBP_RT_EventLogRight (#2697)`.

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
# Cattura ANCHE la quarta colonna (prova): senza, il gate verifica l'eco della decisione
# e non la prova che dovrebbe giustificarla.
RIGA = re.compile(r"^\|\s*`(PIE-[A-Za-z0-9-]+)`\s*\|[^|]*\|([^|]*)\|([^|]*)\|")

# Un `requires` ha forma chiusa: <tipo>:<Nome> con un eventuale #<issue>. Estrarre i TOKEN
# invece di cercare sottostringhe e' cio' che impedisce a `asset:WBP_RT_EventLog` di risultare
# giustificato da un verbale che parla di `asset:WBP_RT_EventLogRight`: un nome che e' PREFISSO
# di un altro passerebbe il containment, e il gate tacerebbe proprio dove deve parlare.
PREREQ = re.compile(r"\b(?:" + "|".join(oracles.TIPI) + r"):[A-Za-z0-9_.-]+(?:#\d+)?")


def leggi_verbale(testo: str) -> dict[str, str]:
    """La decisione registrata per ogni check esaminato, come testo."""
    return {m.group(1): m.group(2).strip() for m in map(RIGA.match, testo.splitlines()) if m}


def leggi_prove(testo: str) -> dict[str, str]:
    """La prova registrata per ogni check esaminato — la QUARTA colonna del verbale.

    Separata da `leggi_verbale` apposta: la decisione (colonna 3) puo' ripetere il
    token di un `requires` mentre la prova (colonna 4) resta vuota, e le due domande
    — «la decisione nomina il bloccante?» e «c'e' una prova scritta?» — sono
    verifiche diverse. Confonderle e' il difetto che questo modulo esiste per
    correggere.
    """
    return {m.group(1): m.group(3).strip() for m in map(RIGA.match, testo.splitlines()) if m}


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
    prove = leggi_prove(verbale)
    senza_prova = sorted(
        w.check
        for w in wires
        if w.requires
        and (
            not set(w.requires) <= set(PREREQ.findall(deciso.get(w.check, "")))
            or not prove.get(w.check, "")
        )
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
    testo_verbale = a.verbale.read_text(encoding="utf-8")
    r = confronta(testo_verbale, wires, stato)

    print(f"check esaminati dal verbale: {r['esaminati']}   requires dichiarati: {r['bloccanti']}")
    print("COMPARSI DOPO LA RASSEGNA — cablati, non verdi, e il verbale non li nomina:")
    for c in r["non_esaminati"] or ["  (nessuno)"]:
        print(f"  {c}")

    if r["senza_prova"]:
        prove = leggi_prove(testo_verbale)
        print("SENZA PROVA — bloccano un check e il verbale non lo giustifica:")
        for c in r["senza_prova"]:
            if not prove.get(c, ""):
                motivo = "colonna prova vuota"
            else:
                motivo = "la decisione non ripete il token del requires"
            print(f"  {c} — {motivo}")
        sys.exit(
            "\nUn bloccante senza prova toglie un check dall'ordine del giorno senza dire perche'. "
            f"Scrivi la riga in {a.verbale}, oppure togli il `requires`."
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
