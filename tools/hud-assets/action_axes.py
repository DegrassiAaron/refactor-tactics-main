#!/usr/bin/env python3
"""Gli assi che ogni azione del catalogo generico DICHIARA.

Un alfabeto iconografico che codifichi fase ed effetti ha senso solo se legge i dati reali: se la
mappa azione -> fase la scrive una persona in un documento, il giorno in cui `Action.Purge` cambia
`ResolutionPhase` l'icona continua a dire la cosa vecchia, e nessuno se ne accorge finche' un
giocatore non sbaglia un turno.

Qui non gira Unreal, quindi si legge la stessa sorgente che `URTCatalogLibrary::GetCoreActionCatalog`
compila: `RTCatalogLibrary.cpp`. E' un surrogato — l'autorita' resta il C++ — e per questo ogni
funzione dice da dove ha letto e fallisce forte se la forma della chiamata cambia, invece di
restituire una tabella a meta'.

Uso:  python3 tools/hud-assets/action_axes.py [--json]
"""

from __future__ import annotations

import json
import re
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
CATALOG_CPP = REPO_ROOT / "Source/RefactorTactics/Ability/RTCatalogLibrary.cpp"

# La firma da cui si legge tutto (`RTCatalogLibrary.cpp`, namespace anonimo):
#
#   ShippedAction(Id, Phase, Priority, Range, Cooldown, Fallback, Effects,
#                 bInterruptible = true, Slot = Main, Movement = None)
#
# I tre parametri finali hanno un default, quindi possono mancare: chi legge deve trattarli come
# opzionali o perde le nove azioni che non li passano.
SHIPPED_DEFAULTS = {"interruptible": True, "slot": "Main", "movement": "None"}


def _catalog_body(text: str) -> str:
    match = re.search(r"TArray<FRTActionDef> URTCatalogLibrary::GetCoreActionCatalog\(\).*?\n\}",
                      text, re.S)
    if not match:
        raise RuntimeError(
            "GetCoreActionCatalog non trovata in RTCatalogLibrary.cpp: la firma e' cambiata, e una "
            "tabella dedotta da una sorgente che non si riconosce piu' e' peggio di nessuna tabella")
    return match.group(0)


def _split_args(call: str) -> list[str]:
    """Divide gli argomenti al livello di parentesi/graffe zero.

    Serve perche' il settimo argomento e' una lista di effetti — `{ FRTActionEffectSpec(...), ... }` —
    e uno split sulle virgole la farebbe a pezzi.
    """
    args, depth, current = [], 0, []
    for ch in call:
        if ch in "({[":
            depth += 1
        elif ch in ")}]":
            depth -= 1
        if ch == "," and depth == 0:
            args.append("".join(current).strip())
            current = []
        else:
            current.append(ch)
    if current:
        args.append("".join(current).strip())
    return args


def _strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)   # i /*Priority*/ inline
    return re.sub(r"//[^\n]*", " ", text)


def _effects(blob: str) -> list[dict]:
    """Gli effetti dichiarati, con il tag di stato quando c'e'.

    Un effetto `Status` senza il suo tag non dice niente di utile a un'icona: `Root` e `Exposed` sono
    lo stesso `ERTActionEffect` e due cose diverse per chi guarda.
    """
    out = []
    for spec in re.finditer(r"FRTActionEffectSpec\(([^)]*(?:\([^)]*\)[^)]*)*)\)", blob):
        args = _split_args(spec.group(1))
        kind = re.sub(r".*ERTActionEffect::", "", args[0]).strip() if args else "None"
        tag = None
        for arg in args[1:]:
            tag_match = re.search(r"TAG_(\w+)", arg)
            if tag_match:
                tag = tag_match.group(1).replace("_", ".")
                break
        out.append({"kind": kind, "status": tag})
    return out


def _shipped_calls(body: str) -> list[str]:
    """Il contenuto di ogni `ShippedAction(...)`, con le parentesi bilanciate.

    ⚠️ Un `re.finditer(r"ShippedAction\\((.*?)\\)\\);")` sembra funzionare e **perde 11 azioni su 37**:
    il match non greedy si ferma sul primo `));`, che nelle azioni con piu' di un effetto cade DENTRO
    la lista invece che alla fine della chiamata. Il difetto e' silenzioso — restituisce 26 record
    validi e nessun errore — ed e' esattamente il modo in cui una tabella dedotta mente.
    """
    calls, needle = [], "ShippedAction("
    index = body.find(needle)
    while index != -1:
        start = index + len(needle)
        depth, cursor = 1, start
        while cursor < len(body) and depth > 0:
            if body[cursor] == "(":
                depth += 1
            elif body[cursor] == ")":
                depth -= 1
            cursor += 1
        if depth == 0:
            calls.append(body[start:cursor - 1])
        index = body.find(needle, cursor)
    return calls


def action_axes() -> dict[str, dict]:
    """`Action.X` -> {phase, priority, range, cooldown, fallback, slot, movement, effects}."""
    body = _catalog_body(CATALOG_CPP.read_text(encoding="utf-8"))
    axes: dict[str, dict] = {}

    for raw_call in _shipped_calls(body):
        raw = _strip_comments(raw_call)
        args = _split_args(raw)
        if len(args) < 7:
            raise RuntimeError(f"chiamata ShippedAction con {len(args)} argomenti: forma inattesa")

        action_id = re.search(r'TEXT\("([^"]+)"\)', args[0])
        if not action_id:
            continue

        def enum_value(arg: str, default: str | None = None) -> str | None:
            found = re.search(r"::(\w+)", arg)
            return found.group(1) if found else default

        record = {
            "phase": enum_value(args[1]),
            "priority": int(re.sub(r"\D", "", args[2]) or 0),
            "range": int(re.sub(r"\D", "", args[3]) or 0),
            "cooldown": int(re.sub(r"\D", "", args[4]) or 0),
            "fallback": enum_value(args[5]),
            "effects": _effects(args[6]),
            "interruptible": SHIPPED_DEFAULTS["interruptible"],
            "slot": SHIPPED_DEFAULTS["slot"],
            "movement": SHIPPED_DEFAULTS["movement"],
        }
        if len(args) > 7:
            record["interruptible"] = "true" in args[7]
        if len(args) > 8:
            record["slot"] = enum_value(args[8], SHIPPED_DEFAULTS["slot"])
        if len(args) > 9:
            record["movement"] = enum_value(args[9], SHIPPED_DEFAULTS["movement"])

        axes[action_id.group(1)] = record

    return axes


def main() -> int:
    axes = action_axes()
    print(f"{len(axes)} azioni lette da {CATALOG_CPP.relative_to(REPO_ROOT)}\n")

    by_phase: dict[str, list[str]] = {}
    for action, record in axes.items():
        by_phase.setdefault(record["phase"], []).append(action)

    for phase in sorted(by_phase, key=lambda p: -len(by_phase[p])):
        names = sorted(a.replace("Action.", "") for a in by_phase[phase])
        print(f"{phase:>16}  {len(names):>2}  {' '.join(names)}")

    print()
    for action in sorted(axes):
        record = axes[action]
        effects = ", ".join(
            e["kind"] + (f"({e['status']})" if e["status"] else "") for e in record["effects"]) or "—"
        print(f"{action:<28} {record['phase']:<15} slot={record['slot']:<16} "
              f"mov={record['movement']:<14} {effects}")
    return 0


if __name__ == "__main__":
    import sys
    if "--json" in sys.argv:
        print(json.dumps(action_axes(), indent=2, ensure_ascii=False))
    else:
        raise SystemExit(main())
