#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Inventario dei numeri che la passata ×10 di E53 (CP 53.3, D-589 (2)) deve moltiplicare.

Legge il repository e stampa, in Markdown, ogni letterale che oggi esprime HP, danno, cura, scudo o una
riduzione piatta: effetti del catalogo (`ERTActionEffect::Damage|Heal|Shield, N`), `MaxHealth` e
`GuardReduction` degli eroi, le costanti intere di `URTCombatLibrary`, le tabelle di `docs/balance/` con una
colonna numerica, i golden e gli scenari con attese numeriche. E' una MISURA: non modifica nulla.

Uso: python tools/balance/scale_x10_inventory.py [--json]
"""
import io, json, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

def rd(rel):
    return io.open(os.path.join(ROOT, rel), encoding="utf-8", errors="replace").read()

def effects(rel):
    """Ogni effetto numerico, attribuito all'ultimo `TEXT("Action.X")` o `TEXT("Hero.X.Y")` che lo precede."""
    out = []
    s = rd(rel)
    names = [(m.start(), m.group(1)) for m in re.finditer(r'TEXT\("((?:Action|Hero)\.[A-Za-z.]+)"\)', s)]
    for e in re.finditer(r'ERTActionEffect::(Damage|Heal|Shield),\s*(\d+)', s):
        owner = "?"
        for pos, name in names:
            if pos < e.start():
                owner = name
            else:
                break
        out.append({"file": rel, "action": owner, "effect": e.group(1), "value": int(e.group(2))})
    return out

def hero_stats(rel):
    s = rd(rel); out = []
    for m in re.finditer(r'(\w+)->(MaxHealth|GuardReduction)\s*=\s*(\d+)', s):
        out.append({"file": rel, "hero": m.group(1), "stat": m.group(2), "value": int(m.group(3))})
    return out

def combat_constants(rel):
    s = rd(rel); out = []
    for m in re.finditer(r'static constexpr int32 (\w+)\s*=\s*(\d+)', s):
        out.append({"file": rel, "name": m.group(1), "value": int(m.group(2))})
    return out

def balance_rows(rel):
    s = rd(rel)
    return sum(1 for l in s.split("\n") if l.startswith("|") and re.search(r"\|\s*\d{1,3}\s*\|", l))

def golden_files():
    base = os.path.join(ROOT, "Source", "RefactorTactics", "Tests", "Golden")
    out = []
    for d, _, files in os.walk(base):
        for f in files:
            if f.endswith(".rttl"):
                out.append(os.path.relpath(os.path.join(d, f), ROOT).replace("\\", "/"))
    return sorted(out)

def main(as_json):
    inv = {
        "effects": effects("Source/RefactorTactics/Ability/RTCatalogLibrary.cpp") + effects("Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp"),
        "hero_stats": hero_stats("Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp"),
        "constants": combat_constants("Source/RefactorTactics/Combat/RTCombatLibrary.h"),
        "balance_rows": {f: balance_rows("docs/balance/" + f) for f in sorted(os.listdir(os.path.join(ROOT, "docs", "balance"))) if f.endswith(".md")},
        "golden": golden_files(),
    }
    if as_json:
        print(json.dumps(inv, indent=2, ensure_ascii=False)); return 0
    print("## Effetti del catalogo con un numero (Damage, Heal, Shield)\n")
    print("| Azione | Effetto | Oggi | ×10 |\n|---|---|---|---|")
    for e in inv["effects"]:
        print("| `%s` | %s | %d | %d |" % (e["action"], e["effect"], e["value"], e["value"] * 10))
    print("\n## Statistiche degli eroi\n")
    print("| Eroe | Statistica | Oggi | ×10 |\n|---|---|---|---|")
    for h in inv["hero_stats"]:
        print("| %s | `%s` | %d | %d |" % (h["hero"], h["stat"], h["value"], h["value"] * 10))
    print("\n## Costanti intere di `URTCombatLibrary` (da leggere una per una: non tutte sono danni)\n")
    print("| Costante | Oggi |\n|---|---|")
    for c in inv["constants"]:
        print("| `%s` | %d |" % (c["name"], c["value"]))
    print("\n## Tabelle di `docs/balance/` con una colonna numerica (righe)\n")
    print("| File | Righe |\n|---|---|")
    for f, n in inv["balance_rows"].items():
        print("| `%s` | %d |" % (f, n))
    print("\n## Golden che la passata rigenera (tutti: ogni danno cambia)\n")
    for g in inv["golden"]:
        print("- `%s`" % g)
    print("\nMisurato con `python tools/balance/scale_x10_inventory.py`; i conteggi valgono al commit in cui si legge.")
    return 0

if __name__ == "__main__":
    sys.exit(main("--json" in sys.argv[1:]))
