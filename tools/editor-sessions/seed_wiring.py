# tools/editor-sessions/seed_wiring.py
"""Semina la bozza del cablaggio dal vecchio registro delle sedute.

Meccanica e fallibile per costruzione: deduce l'allestimento dagli indizi che
le sedute lasciano nella prosa. Cio' che NON sa dedurre lo dichiara fra gli
orfani, e non lo scrive: `registry.load` rifiuta un setup sconosciuto, quindi
una riga seminata male non puo' passare inosservata.

    python3 tools/editor-sessions/seed_wiring.py --out build/wiring-seminato.yaml
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

import yaml

VECCHIO = Path("docs/roadmap/editor-sessions.yaml")

# Dal piu' specifico al piu' generico: la prima corrispondenza vince.
MAPPA_SETUP: tuple[tuple[str, str], ...] = (
    ("rt.Match.Autobattle", "SET-HEX-BOT"),
    ("L_GrayKitPlayground", "SET-GRAYKIT"),
    ("L_Frontend", "SET-FRONTEND"),
    ("rt.Test.Scenario", "SET-SCEN"),
    ("L_DevSandbox", "SET-SANDBOX"),
    ("L_HexArena", "SET-HEX-MATCH"),
)

CAMPI_PROSA = ("title", "produces", "steps", "notes", "done_when")


def deduci_setup(blob: str) -> str | None:
    for indizio, setup in MAPPA_SETUP:
        if indizio in blob:
            return setup
    return None


def semina(sessioni: list[dict]) -> tuple[list[dict], dict[str, list[str]]]:
    """Le righe seminate, e i check orfani RAGGRUPPATI PER SEDUTA.

    Il raggruppamento non e' cosmetico: una seduta ERA un allestimento per
    costruzione, quindi chi assegna decide una volta per seduta invece che una
    volta per check. Sul registro del 2026-09-11 la differenza e' 24 giudizi
    invece di 95.
    """
    righe: dict[str, dict] = {}
    muti: dict[str, list[str]] = {}
    for s in sessioni:
        blob = " ".join(str(s.get(k) or "") for k in CAMPI_PROSA)
        setup = deduci_setup(blob)
        issue = (s.get("issues") or [None])[0]
        for check in s.get("verifies") or []:
            if check in righe:
                continue
            if setup is None:
                muti.setdefault(s["id"], []).append(check)
                continue
            riga = {"check": check, "setup": setup}
            if issue is not None:
                riga["issue"] = issue
            righe[check] = riga

    orfani = {
        sid: sorted(set(cs) - set(righe))
        for sid, cs in muti.items()
        if set(cs) - set(righe)
    }
    return [righe[c] for c in sorted(righe)], orfani


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--sessioni", default=VECCHIO, type=Path, help="il vecchio registro")
    ap.add_argument("--out", default=Path("build/wiring-seminato.yaml"), type=Path)
    a = ap.parse_args()

    if not a.sessioni.exists():
        sys.exit(f"registro non trovato: {a.sessioni}")
    vecchio = yaml.safe_load(a.sessioni.read_text(encoding="utf-8")) or {}
    righe, orfani = semina(vecchio.get("sessions") or [])

    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(
        yaml.safe_dump({"wiring": righe}, allow_unicode=True, sort_keys=False),
        encoding="utf-8",
    )
    print(f"righe seminate: {len(righe)} -> {a.out}")
    if orfani:
        quanti = sum(len(cs) for cs in orfani.values())
        print(f"SENZA ALLESTIMENTO: {quanti} check da {len(orfani)} sedute.")
        print("Decidi una volta per SEDUTA: era un allestimento per costruzione.")
        for sid in sorted(orfani):
            print(f"  {sid}: {' '.join(orfani[sid])}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
