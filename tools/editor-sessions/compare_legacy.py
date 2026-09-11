# tools/editor-sessions/compare_legacy.py
"""Il gate della fetta 0: il calcolo riproduce le sedute scritte a mano?

`persi` sono i check che una seduta convocava e che l'agenda non nomina, ne'
fra i liberi ne' fra i bloccati. Sono la misura che puo' FALSIFICARE il
modello: se non e' vuota, o manca una riga di wiring, o il raggruppamento e'
sbagliato. Un check verde non e' perso — e' finito. Un check bloccato e
stampato non e' perso — e' dichiarato.

    python3 tools/editor-sessions/compare_legacy.py
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import yaml

import agenda
import oracles
import pie_status
import registry

VECCHIO = Path("docs/roadmap/editor-sessions.yaml")
CACHE = Path("tools/decision-log/github-cache.json")


def confronta(gruppi, scoperta, sessioni: list[dict], stato: dict[str, dict]) -> dict:
    nominati = {v.check for g in gruppi for v in g.liberi + g.bloccati}
    vecchi = {
        c
        for s in sessioni
        for c in (s.get("verifies") or [])
        if stato.get(c, {}).get("stato") not in (pie_status.VERDE, None)
    }
    return {
        "persi": sorted(vecchi - nominati),
        "guadagnati": sorted(nominati - vecchi),
        "aperture": len([g for g in gruppi if g.liberi]),
        "sedute": len(sessioni),
        "scoperta": sorted(scoperta),
    }


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--mattoni", default=registry.MATTONI, type=Path)
    ap.add_argument("--vecchio", default=VECCHIO, type=Path)
    ap.add_argument("--registro", default=pie_status.REGISTRO, type=Path)
    ap.add_argument("--cache", default=CACHE, type=Path)
    a = ap.parse_args()

    setups, wires = registry.load(a.mattoni)
    stato = pie_status.load(a.registro)
    cache = json.loads(a.cache.read_text(encoding="utf-8"))
    inventario = oracles.asset_tracciati()
    chiuse, note = oracles.issue_chiuse(cache), oracles.issue_note(cache)
    gruppi, scoperta = agenda.calcola(
        setups, wires, stato, lambda r: oracles.valuta(r, inventario, chiuse, note)
    )

    vecchio = yaml.safe_load(a.vecchio.read_text(encoding="utf-8")) or {}
    r = confronta(gruppi, scoperta, vecchio.get("sessions") or [], stato)

    print(f"aperture calcolate: {r['aperture']}   sedute scritte: {r['sedute']}")
    print("PERSI — convocati da una seduta e non dall'agenda:")
    for c in r["persi"] or ["  (nessuno)"]:
        print(f"  {c}")
    print("GUADAGNATI — convocati dall'agenda e da nessuna seduta:")
    for c in r["guadagnati"] or ["  (nessuno)"]:
        print(f"  {c}")
    print("CODA SCOPERTA — nel registro PIE e in nessuna riga di wiring:")
    for c in r["scoperta"] or ["  (nessuna)"]:
        print(f"  {c}")

    if r["persi"]:
        sys.exit(
            "\nIl calcolo perde dei check. Prima di proseguire con le fette 1-5, "
            "per ognuno stabilisci se manca una riga di wiring o se il raggruppamento e' sbagliato."
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
