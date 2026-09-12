# tools/editor-sessions/build_agenda.py
"""L'ordine del giorno della prossima apertura dell'Editor.

Risponde a una domanda sola: apro l'Editor adesso — cosa allestisco e cosa
guardo. L'uscita NON si committa: `build/` e' ignorato, e un ordine del giorno
committato invecchierebbe in silenzio, che e' il modo in cui e' morta la vista
rimossa da D-181.

    python3 tools/decision-log/fetch_github_cache.py     # aggiorna la cache (serve `gh`)
    python3 tools/editor-sessions/build_agenda.py --out build/ordine-del-giorno.md
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import agenda
import oracles
import pie_status
import registry

CACHE = Path("tools/decision-log/github-cache.json")


def rendi(gruppi, scoperta, setups) -> str:
    righe = ["# Ordine del giorno — la prossima apertura dell'Editor", ""]
    righe.append("> Rigenerato da `tools/editor-sessions/build_agenda.py`. **Non si committa.**")
    righe.append("> L'esito di ogni voce appartiene a `docs/technical/test-manuali-pie.md`.")
    righe.append("")

    aperture = [g for g in gruppi if g.liberi]
    sospesi = [g for g in gruppi if not g.liberi]

    for n, g in enumerate(aperture, start=1):
        s = setups[g.setup]
        righe.append(f"## Apertura {n} — `{g.setup}`")
        righe.append("")
        for k, v in s.fields.items():
            righe.append(f"- **{k}**: `{v}`")
        righe.append("")
        righe.append("Da guardare:")
        for v in g.liberi:
            marca = " `RELEASE-V01`" if v.release else ""
            righe.append(f"- `{v.check}`{marca}")
        if g.bloccati:
            righe.append("")
            righe.append("Bloccati in questa stessa apertura:")
            for v in g.bloccati:
                righe.append(f"- `{v.check}` — {'; '.join(v.bloccanti)}")
        righe.append("")

    if sospesi:
        righe.append("## Nessuna apertura: ogni voce e' bloccata")
        righe.append("")
        for g in sospesi:
            righe.append(f"### `{g.setup}`")
            for v in g.bloccati:
                righe.append(f"- `{v.check}` — {'; '.join(v.bloccanti)}")
            righe.append("")

    righe.append("## Coda scoperta — voci che nessuna riga di wiring cabla")
    righe.append("")
    if scoperta:
        for c in scoperta:
            righe.append(f"- `{c}`")
    else:
        righe.append("Nessuna.")
    righe.append("")
    return "\n".join(righe)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--mattoni", default=registry.MATTONI, type=Path)
    ap.add_argument("--registro", default=pie_status.REGISTRO, type=Path)
    ap.add_argument("--cache", default=CACHE, type=Path)
    ap.add_argument("--out", default=Path("build/ordine-del-giorno.md"), type=Path)
    a = ap.parse_args()

    for p in (a.mattoni, a.registro, a.cache):
        if not p.exists():
            sys.exit(f"file non trovato: {p}")

    try:
        setups, wires = registry.load(a.mattoni)
    except registry.RegistryError as e:
        sys.exit(f"registro dei mattoni rifiutato: {e}")

    stato = pie_status.load(a.registro)
    cache = json.loads(a.cache.read_text(encoding="utf-8"))
    inventario = oracles.asset_tracciati()
    chiuse = oracles.issue_chiuse(cache)
    note = oracles.issue_note(cache)

    try:
        gruppi, scoperta = agenda.calcola(
            setups, wires, stato, lambda r: oracles.valuta(r, inventario, chiuse, note)
        )
    except (agenda.AgendaError, registry.RegistryError, oracles.OracoloError, ValueError) as e:
        sys.exit(f"calcolo rifiutato: {e}")

    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(rendi(gruppi, scoperta, setups), encoding="utf-8")
    print(f"scritto {a.out}")
    print(f"aperture: {len([g for g in gruppi if g.liberi])}")
    print(f"voci in coda scoperta: {len(scoperta)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
