# tools/editor-sessions/registry.py
"""I mattoni delle sedute: allestimenti e cablaggio.

Legge `docs/roadmap/sedute-mattoni.yaml`. Non sa niente di esiti: quelli
appartengono a `docs/technical/test-manuali-pie.md`, e in questo schema manca
il campo dove scriverli.

Ogni difetto del registro viene RIFIUTATO col proprio nome, mai lasciato
cadere: un input malformato che produce un verde e' il difetto peggiore di
questa famiglia di strumenti (precedente: D-182).
"""
from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path

import yaml

MATTONI = Path("docs/roadmap/sedute-mattoni.yaml")


class RegistryError(Exception):
    """Un difetto del registro. Si rifiuta, non si ignora."""


@dataclass(frozen=True)
class Setup:
    """Un allestimento: cosa si apre, con quale comando, chi e' umano."""

    id: str
    extends: str | None = None
    fields: dict = field(default_factory=dict)


@dataclass(frozen=True)
class Wire:
    """Il cablaggio di un check: dove si guarda, e cosa serve perche' si possa."""

    check: str
    setup: str
    requires: tuple[str, ...] = ()
    issue: int | None = None


def load(path: Path = MATTONI) -> tuple[dict[str, Setup], list[Wire]]:
    raw = yaml.safe_load(path.read_text(encoding="utf-8")) or {}

    setups: dict[str, Setup] = {}
    for r in raw.get("setups") or []:
        sid = r["id"]
        if sid in setups:
            raise RegistryError(f"setup dichiarato due volte: {sid}")
        setups[sid] = Setup(
            id=sid,
            extends=r.get("extends"),
            fields={k: v for k, v in r.items() if k not in ("id", "extends")},
        )

    for s in setups.values():
        if s.extends is not None and s.extends not in setups:
            raise RegistryError(f"{s.id} estende un setup inesistente: {s.extends}")

    wires: list[Wire] = []
    padrone: dict[str, str] = {}
    for r in raw.get("wiring") or []:
        check = r["check"]
        if check in padrone:
            raise RegistryError(
                f"check rivendicato due volte: {check} da {padrone[check]} e da {r['setup']}"
            )
        if r["setup"] not in setups:
            raise RegistryError(f"{check} cita un setup inesistente: {r['setup']}")
        padrone[check] = r["setup"]
        wires.append(
            Wire(
                check=check,
                setup=r["setup"],
                requires=tuple(r.get("requires") or []),
                issue=r.get("issue"),
            )
        )

    return setups, wires


def root_of(setup_id: str, setups: dict[str, Setup]) -> str:
    """L'allestimento di base della catena `extends`.

    Due check la cui catena finisce nella stessa radice cadono nella stessa
    apertura: `SET-HEX-TURN` non riapre l'Editor rispetto a `SET-HEX-MATCH`,
    aggiunge uno stato.
    """
    visti: list[str] = []
    cur = setup_id
    while True:
        if cur in visti:
            raise RegistryError("ciclo in extends: " + " -> ".join(visti + [cur]))
        visti.append(cur)
        nxt = setups[cur].extends
        if nxt is None:
            return cur
        cur = nxt
