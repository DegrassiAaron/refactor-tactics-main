# tools/editor-sessions/oracles.py
"""Un prerequisito e' soddisfatto oppure no, e chi lo stabilisce.

Due oracoli soli, per scelta:

  asset:<Nome>                 il file esiste fra quelli tracciati in Content/.
                               In questo clone e' un oracolo valido: disco e
                               indice coincidono, e .gitignore:48 ignora
                               Content/**/*.uasset con eccezioni negate, quindi
                               «non tracciato» significa «non esiste».

  <tipo>:<Nome>#<issue>        lo stabilisce chi possiede la issue. Chiusa =
                               soddisfatto. E' anche il modo in cui un
                               bloccante smette di nascondere un check per
                               sempre: quando la issue chiude, il check torna
                               nell'ordine del giorno da solo.

Un prerequisito che non serve NON si dichiara. L'assenza di una riga e' gia'
la dichiarazione, e un tipo per dire «non serve» sarebbe un campo in piu' che
puo' mentire.
"""
from __future__ import annotations

import subprocess
from dataclasses import dataclass

TIPI_CON_ISSUE = ("mount", "cue", "anim", "feature")
TIPI = ("asset",) + TIPI_CON_ISSUE


@dataclass(frozen=True)
class Esito:
    soddisfatto: bool
    perche: str


def asset_tracciati() -> set[str]:
    """I nomi (senza estensione) dei package versionati sotto `Content/`."""
    out = subprocess.run(
        ["git", "ls-files", "Content"], capture_output=True, text=True, check=True
    ).stdout
    nomi: set[str] = set()
    for riga in out.splitlines():
        nome = riga.rsplit("/", 1)[-1]
        for ext in (".uasset", ".umap"):
            if nome.endswith(ext):
                nomi.add(nome[: -len(ext)])
    return nomi


def issue_chiuse(cache: dict) -> set[int]:
    """I numeri chiusi nella cache GitHub, nella forma di `tools/decision-log/`."""
    return {
        int(n) for n, v in (cache.get("items") or {}).items() if v.get("state") == "closed"
    }


def issue_note(cache: dict) -> set[int]:
    """I numeri che la cache conosce, chiusi o aperti."""
    return {int(n) for n in (cache.get("items") or {})}


def valuta(req: str, inventario: set[str], chiuse: set[int], note: set[int]) -> Esito:
    tipo, _, resto = req.partition(":")
    if tipo not in TIPI:
        raise ValueError(f"tipo di requires sconosciuto: {tipo} (in {req!r})")

    if tipo == "asset":
        presente = resto in inventario
        return Esito(
            presente,
            f"{resto} tracciato" if presente else f"{resto} non esiste in Content/",
        )

    nome, sep, numero = resto.partition("#")
    if not sep or not numero.isdigit():
        raise ValueError(f"{tipo} richiede una issue owner nella forma nome#numero: {req!r}")
    n = int(numero)
    if n not in note:
        return Esito(False, f"{tipo} {nome}: #{n} non e' nella cache — rilancia il fetch")
    if n in chiuse:
        return Esito(True, f"{tipo} {nome}: #{n} chiusa")
    return Esito(False, f"{tipo} {nome}: #{n} aperta")
