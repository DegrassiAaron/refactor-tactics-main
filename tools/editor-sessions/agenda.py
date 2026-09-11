# tools/editor-sessions/agenda.py
"""Da mattoni a ordine del giorno.

Il calcolo in cinque mosse:

  1. per ogni check, lo stato e i requires non soddisfatti;
  2. si scartano i verdi — non c'e' piu' niente da guardare;
  3. si raggruppa per la RADICE della catena `extends`, perche' e' l'apertura
     dell'Editor a costare, non il Play;
  4. un gruppo con almeno un check libero e' una apertura; i bloccati restano
     stampati sotto, col motivo;
  5. si ordina: prima chi porta il subset di release, poi per numero di check
     liberi, poi per ID — perche' l'ordine dev'essere totale.

Non tocca ne' git ne' la rete: `valuta` arriva gia' legata al suo inventario.
"""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Callable

import oracles
import pie_status
from registry import Setup, Wire, root_of


class AgendaError(Exception):
    """Un cablaggio che non corrisponde al registro. Si rifiuta."""


@dataclass(frozen=True)
class Voce:
    check: str
    bloccanti: tuple[str, ...]
    release: bool


@dataclass
class Gruppo:
    setup: str
    liberi: list[Voce] = field(default_factory=list)
    bloccati: list[Voce] = field(default_factory=list)


def calcola(
    setups: dict[str, Setup],
    wires: list[Wire],
    stato: dict[str, dict],
    valuta: Callable[[str], oracles.Esito],
) -> tuple[list[Gruppo], list[str]]:
    gruppi: dict[str, Gruppo] = {}
    cablati: set[str] = set()

    for w in wires:
        if w.check not in stato:
            raise AgendaError(
                f"{w.check} e' cablato ma non esiste nel registro PIE"
            )
        cablati.add(w.check)
        if stato[w.check]["stato"] == pie_status.VERDE:
            continue

        bloccanti = tuple(
            e.perche for e in (valuta(r) for r in w.requires) if not e.soddisfatto
        )
        voce = Voce(check=w.check, bloccanti=bloccanti, release=stato[w.check]["release"])
        radice = root_of(w.setup, setups)
        g = gruppi.setdefault(radice, Gruppo(setup=radice))
        (g.bloccati if bloccanti else g.liberi).append(voce)

    for g in gruppi.values():
        g.liberi.sort(key=lambda v: v.check)
        g.bloccati.sort(key=lambda v: v.check)

    ordinati = sorted(
        gruppi.values(),
        key=lambda g: (
            0 if any(v.release for v in g.liberi) else 1,
            -len(g.liberi),
            g.setup,
        ),
    )

    scoperta = sorted(
        c
        for c, v in stato.items()
        if c not in cablati and v["stato"] != pie_status.VERDE
    )
    return ordinati, scoperta
