# tools/editor-sessions/pie_status.py
"""Lo stato di ogni voce PIE, letto dal suo unico owner.

`docs/technical/test-manuali-pie.md` possiede l'esito atteso e lo stato
raggiunto. Questo modulo li LEGGE: non scrive, non giudica, non deduce. Una
riga di tabella dichiara lo stato con un'emoji nell'ultima cella, e
l'appartenenza al subset del gate con `RELEASE-V01` nella prima.
"""
from __future__ import annotations

import re
from pathlib import Path

REGISTRO = Path("docs/technical/test-manuali-pie.md")

# ⚠️ La classe include le MINUSCOLE, e non e' un dettaglio: il registro avverte da se' che
# «nove righe reali portano un suffisso minuscolo che il regex, essendo `[A-Z0-9]`, tronca o
# accorpa» — `PIE-AS4a`, `PIE-AS4b`, `PIE-BU2b`, `PIE-BU2c`, `PIE-BU3c`, `PIE-HEXPLAY-3b`,
# `-4b`, `-6b`, `-6c`. Con `[A-Z0-9-]+` la riga non viene troncata: viene SALTATA, perche' il
# `**` di chiusura non arriva dove il regex lo aspetta. Sono voci che spariscono in silenzio.
RIGA = re.compile(r"^\|\s*\*\*(PIE-[A-Za-z0-9-]+)\*\*")
STATI = "✅🟡⏳❌⛔🔴"
VERDE = "✅"
SCONOSCIUTO = "?"


def parse(text: str) -> dict[str, dict]:
    out: dict[str, dict] = {}
    for line in text.splitlines():
        m = RIGA.match(line)
        if m is None:
            continue
        celle = [c.strip() for c in line.split("|")]
        ultima = celle[-2] if len(celle) > 2 else ""
        stato = next((c for c in ultima if c in STATI), SCONOSCIUTO)
        out[m.group(1)] = {"stato": stato, "release": "RELEASE-V01" in celle[1]}
    return out


def load(path: Path = REGISTRO) -> dict[str, dict]:
    return parse(path.read_text(encoding="utf-8"))
