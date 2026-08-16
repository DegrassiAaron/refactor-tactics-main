#!/usr/bin/env python3
"""Le tre tabelle di default in C++ dicono ancora quello che dice §4 del catalogo?

`docs/balance/RT_EquipmentCatalog_v0.1.md` §4 e' la fonte autorevole dei loadout iniziali; le tabelle
di `URTCatalogLibrary::Default*For` ne sono una **copia**, e una copia diverge. Questo gate impedisce
che diverga in silenzio.

⚠️ **Non e' `#576`**, che chiede il confronto generale markdown↔C++ per tutti i cataloghi. Questo
copre i soli default d'equipaggiamento — la fetta A di `#63` ne aveva bisogno subito, e una fetta di
gate che esiste vale piu' di un gate completo che non c'e'.

COSA NON VERIFICA, dichiarato perche' un gate silenzioso su meta' del problema e' peggio di nessun
gate: che il pezzo prescritto sia **spedito**. §4 assegna a due eroi un gadget che `MakeGadgets` non
costruisce (`Gadget.Insulator` e' un passivo che aspetta E36; `Gadget.Sensor` aspetta E13), e la
copia C++ e' **fedele lo stesso** — e' `DefaultLoadoutFor` a filtrare, interrogando il catalogo. Qui
si confronta cosa e' scritto, non cosa e' costruibile.

    python scripts/check-equipment-defaults.py --check
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

RADICE = Path(__file__).resolve().parent.parent
CATALOGO = RADICE / "docs/balance/RT_EquipmentCatalog_v0.1.md"
SORGENTE = RADICE / "Source/RefactorTactics/Ability/RTCatalogLibrary.cpp"

# La colonna di §4 -> la funzione C++ che deve dire la stessa cosa.
COLONNE = {
    2: ("Gadget", "DefaultGadgetFor"),
    3: ("Reaction", "DefaultReactionModuleFor"),
    4: ("Weapon", "DefaultWeaponVariantFor"),
}


def celle(riga: str) -> list[str]:
    """Le celle di una riga di tabella markdown, senza i bordi vuoti."""
    return [c.strip() for c in riga.strip().strip("|").split("|")]


def mappa_nome_a_id(testo: str) -> dict[str, str]:
    """`Precisione -> Weapon.Precision`, letta dalle tabelle §1-3 invece che trascritta.

    Trascriverla qui creerebbe la terza copia, cioe' il difetto che questo gate esiste per impedire.
    """
    mappa: dict[str, str] = {}
    for riga in testo.splitlines():
        m = re.match(r"\|\s*`((?:Weapon|Gadget|Reaction)\.[A-Za-z]+)`\s*\|\s*([^|]+?)\s*\|", riga)
        if m:
            mappa[m.group(2).strip()] = m.group(1)
    return mappa


def default_dal_markdown(testo: str) -> dict[str, dict[str, str]]:
    """`{Hero.Gadget: {Gadget: Gadget.Insulator, ...}}` dalla tabella §4."""
    nome_a_id = mappa_nome_a_id(testo)
    inizio = testo.index("## 4. Loadout")
    fine = testo.index("## 5.", inizio)
    fuori: dict[str, dict[str, str]] = {}
    sconosciuti: list[str] = []

    for riga in testo[inizio:fine].splitlines():
        if not riga.startswith("| **"):
            continue
        c = celle(riga)
        if len(c) < 5:
            continue
        eroe = f"Hero.{c[0].strip('*')}"
        for indice, (categoria, _) in COLONNE.items():
            # La colonna della variante d'arma porta la motivazione dopo un trattino:
            # `**Precisione** — 18 a portata 5: ...`. Si tiene il solo nome.
            nome = re.sub(r"\s+[—-]\s+.*$", "", c[indice]).strip().strip("*").strip()
            ident = nome_a_id.get(nome)
            if ident is None:
                sconosciuti.append(f"{eroe} / {categoria}: «{nome}» non compare in nessuna tabella §1-3")
                continue
            fuori.setdefault(eroe, {})[categoria] = ident

    if sconosciuti:
        for s in sconosciuti:
            print(f"ERRORE  {s}")
    return fuori


def default_dal_cpp(testo: str) -> dict[str, dict[str, str]]:
    """Le tre `TMap` di `Default*For`, lette dal corpo di ciascuna funzione."""
    fuori: dict[str, dict[str, str]] = {}
    for _, (categoria, funzione) in COLONNE.items():
        m = re.search(rf"URTCatalogLibrary::{funzione}\(.*?\)\s*\{{(.*?)\n\}}", testo, re.S)
        if not m:
            print(f"ERRORE  {funzione} non trovata in {SORGENTE.name}")
            continue
        for chiave, valore in re.findall(
            r'\{\s*FName\(TEXT\("(Hero\.[A-Za-z]+)"\)\),\s*FName\(TEXT\("([A-Za-z.]+)"\)\)\s*\}',
            m.group(1),
        ):
            fuori.setdefault(chiave, {})[categoria] = valore
    return fuori


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--check", action="store_true", help="esce 1 se le due fonti divergono")
    args = p.parse_args()

    md = CATALOGO.read_text(encoding="utf-8")
    cpp = SORGENTE.read_text(encoding="utf-8")

    atteso = default_dal_markdown(md)
    trovato = default_dal_cpp(cpp)

    errori = 0
    eroi = sorted(set(atteso) | set(trovato))
    for eroe in eroi:
        for _, (categoria, funzione) in sorted(COLONNE.items()):
            a = atteso.get(eroe, {}).get(categoria)
            t = trovato.get(eroe, {}).get(categoria)
            if a == t:
                continue
            errori += 1
            print(f"ERRORE  {eroe} / {categoria}")
            print(f"        §4 del catalogo : {a or '(assente)'}")
            print(f"        {funzione:26}: {t or '(assente)'}")

    confronti = len(eroi) * len(COLONNE)
    print(f"\nEroi confrontati: {len(eroi)} · confronti: {confronti} · divergenze: {errori}")
    if errori == 0:
        print("OK — le tabelle di default in C++ dicono quello che dice §4 del catalogo.")

    # Informativo, non un errore: la copia e' fedele anche quando il pezzo non e' costruibile.
    # `DefaultLoadoutFor` filtra interrogando il catalogo, quindi il difetto non arriva in partita.
    print("\nNota: questo gate NON verifica che i pezzi siano spediti da `MakeGadgets`/"
          "`MakeReactionModules`/`MakeWeaponVariants`.")
    print("      Lo fa `Equipment.DefaultLoadoutIsOnePerSlotForEveryHero`, che pinna «hai il loadout")
    print("      se e solo se tutti e tre i pezzi prescritti sono spediti».")

    return 1 if (errori and args.check) else 0


if __name__ == "__main__":
    sys.exit(main())
