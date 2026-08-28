#!/usr/bin/env python3
"""Colorimetria: la misura con cui la palette delle fasi e' stata SCELTA (D-233).

`T8` verifica che ogni icona d'azione porti il colore della propria macro-fase. Questo modulo
verifica l'altra meta', che fino al 2026-08-28 non esisteva in nessuno script: che quei quattro
colori siano davvero **distinguibili**. La misura era stata eseguita a mano in una sessione, e
[D-233] lo dichiarava — «riproducibile ma non verificata a macchina». Qui smette di esserlo.

Perche' serve: la combinazione che veniva naturale — `Move` rosa `#CC79A7` invece di blu — e'
**scartata**, e il motivo non si vede a occhio su uno schermo sano. In tritanopia `Blast` e `Move`
arrivano a `dE = 1.0`: diventano lo stesso colore. Chi in futuro ritoccasse una tinta «perche' sta
meglio» rifarebbe quell'errore senza che nulla lo fermi.

Zero dipendenze, come il resto della toolchain. Tre misure:

  * **contrasto WCAG 2.1** sul fondo, per 1.4.11 (componenti grafici, soglia 3:1);
  * **dE CIE76** in spazio Lab fra ogni coppia — semplice e sufficiente a separare 1.0 da 19.6;
  * **simulazione CVD** con la matrice di Vienot-Brettel-Mollon 1999 in spazio LMS, per le tre
    dicromazie (protanopia, deuteranopia, tritanopia).

    python3 tools/hud-assets/color_metrics.py            # il referto della palette
    python3 tools/hud-assets/color_metrics.py --json     # lo stesso, per una macchina

⚠️ **La luminanza e' una misura a parte e non si somma alle altre**: due colori possono avere `dE`
alto e restare vicini in grayscale. E' il caso di `Blast` e `Dash` (`dL = 0.035`), ed e' la ragione
per cui [D-232] tiene il marker di fase obbligatorio invece di lasciare la fase al solo colore.
"""

from __future__ import annotations

import itertools
import json
import math

# --------------------------------------------------------------------------------------------------
# Le soglie. Nessuna e' inventata qui: ognuna viene da una decisione gia' presa, e la riga dice quale.
# --------------------------------------------------------------------------------------------------

# Due fasi non devono confondersi sotto NESSUNA dicromazia. Il set attuale misura **19.6** nel caso
# peggiore (tritanopia, Prep/Dash); il set intuitivo scartato da D-233 misurava **1.0**. La soglia sta
# in mezzo e non a ridosso del valore corrente: un gate tarato sulla misura esatta diventa un veto su
# ogni ritocco di tinta, che non e' cio' che si vuole impedire.
CVD_MIN_DELTA_E = 15.0

# Un colore di FASE non deve somigliare a un colore di STATO: mescolare due assi e' peggio che
# avvicinare due valori dello stesso asse. Questa soglia NON e' scelta qui — e' la riga che D-233 ha
# gia' tracciato scartando il giallo `#F0E442`, che dista **18.3** dall'ambra di `Selected`, mentre la
# coppia piu' stretta del set attuale (`Prep` vs `Cyan`) sta a **26.2**.
STATE_MIN_DELTA_E = 20.0

# WCAG 2.1 §1.4.11 «Non-text Contrast»: un componente grafico che veicola informazione vuole 3:1.
MIN_CONTRAST = 3.0

# ⛔ La deroga dichiarata, sull'esempio di `COLOR_DEBT` e `ALPHABET_EXEMPT`: `Move #0072B2` passa su
# `BG_Panel` (3.36) e NON su `BG_Raised` (2.89). D-233 lo registra come limite noto invece di
# nasconderlo: su superficie rialzata il glifo va bordato o schiarito, ed e' una verifica a schermo.
# La deroga sta qui, esplicita e stampata, cosi' chi la paga sa che esiste.
CONTRAST_EXEMPT = {
    ("Move", "BG_Raised"): "D-233: bordare o schiarire il glifo su superficie rialzata (verifica PIE)",
}


# --------------------------------------------------------------------------------------------------
# Colorimetria
# --------------------------------------------------------------------------------------------------

def hex_to_rgb(value: str) -> tuple[float, float, float]:
    """`#RRGGBB` -> tre canali in [0,1]. Solleva su una forma che non e' un colore."""
    text = value.lstrip("#")
    if len(text) != 6 or any(c not in "0123456789abcdefABCDEF" for c in text):
        raise ValueError(f"non e' un colore `#RRGGBB`: {value!r}")
    return tuple(int(text[i:i + 2], 16) / 255 for i in (0, 2, 4))  # type: ignore[return-value]


def _to_linear(channel: float) -> float:
    """sRGB -> lineare. La curva NON e' una gamma 2.2: ha un tratto lineare sotto 0.04045."""
    return channel / 12.92 if channel <= 0.04045 else ((channel + 0.055) / 1.055) ** 2.4


def _to_srgb(channel: float) -> float:
    channel = max(0.0, min(1.0, channel))
    return 12.92 * channel if channel <= 0.0031308 else 1.055 * channel ** (1 / 2.4) - 0.055


def relative_luminance(value: str) -> float:
    """Luminanza relativa WCAG. E' anche la misura del grayscale: due colori con la stessa
    luminanza sono LO STESSO grigio, per quanto diverse siano le loro tinte."""
    r, g, b = (_to_linear(c) for c in hex_to_rgb(value))
    return 0.2126 * r + 0.7152 * g + 0.0722 * b


def contrast_ratio(first: str, second: str) -> float:
    """Rapporto di contrasto WCAG: da 1 (identici) a 21 (bianco su nero)."""
    a, b = relative_luminance(first), relative_luminance(second)
    return (max(a, b) + 0.05) / (min(a, b) + 0.05)


# La matrice sRGB -> XYZ (D65, osservatore 2°), e il bianco che ne DISCENDE.
#
# ⚠️ Il punto di bianco NON e' la costante tabulata `(0.95047, 1.0, 1.08883)`: e' la somma delle
# righe di QUESTA matrice. La differenza e' il residuo di arrotondamento dei coefficienti pubblicati
# a quattro decimali, e vale ~0.01 in `b*` — invisibile su qualunque colore reale, ma abbastanza da
# far cadere il self-check sul bianco. Derivandolo dalla matrice la trasformazione diventa esatta per
# costruzione, invece di esserlo «entro una tolleranza» che qualcuno dovrebbe scegliere.
_M_SRGB_XYZ = ((0.4124, 0.3576, 0.1805),
               (0.2126, 0.7152, 0.0722),
               (0.0193, 0.1192, 0.9505))
_WHITE = tuple(sum(row) for row in _M_SRGB_XYZ)


def rgb_to_lab(value: str) -> tuple[float, float, float]:
    """sRGB -> CIE L*a*b*, illuminante D65, osservatore 2°."""
    r, g, b = (_to_linear(c) for c in hex_to_rgb(value))
    x, y, z = (sum(m * c for m, c in zip(row, (r, g, b))) / w
               for row, w in zip(_M_SRGB_XYZ, _WHITE))

    def f(t: float) -> float:
        return t ** (1 / 3) if t > 0.008856 else 7.787 * t + 16 / 116

    fx, fy, fz = f(x), f(y), f(z)
    return (116 * fy - 16, 500 * (fx - fy), 200 * (fy - fz))


def delta_e(first: str, second: str) -> float:
    """Distanza CIE76 in Lab. Grossolana rispetto a CIEDE2000, e **basta**: qui si separa 1.0 da
    19.6, non si giudica una prova di stampa. Una formula piu' fine cambierebbe i decimali e nessuna
    delle decisioni che questo modulo verifica."""
    return math.dist(rgb_to_lab(first), rgb_to_lab(second))


CVD_KINDS = ("protanopia", "deuteranopia", "tritanopia")


def simulate_cvd(value: str, kind: str) -> str:
    """Come un dicromatico vede questo colore. Vienot, Brettel & Mollon 1999, in spazio LMS.

    Non e' un filtro estetico: ogni dicromazia PROIETTA i tre coni su un piano, e due colori che
    cadono sullo stesso punto diventano indistinguibili. E' esattamente cio' che accade a `Blast` e
    a un `Move` rosa in tritanopia.
    """
    if kind not in CVD_KINDS:
        raise ValueError(f"dicromazia sconosciuta: {kind!r} (attese: {', '.join(CVD_KINDS)})")
    r, g, b = (_to_linear(c) for c in hex_to_rgb(value))
    long_ = 17.8824 * r + 43.5161 * g + 4.11935 * b
    med = 3.45565 * r + 27.1554 * g + 3.86714 * b
    short = 0.0299566 * r + 0.184309 * g + 1.46709 * b

    if kind == "protanopia":       # il cono L manca: si ricostruisce da M e S
        long_ = 2.02344 * med - 2.52581 * short
    elif kind == "deuteranopia":   # il cono M manca
        med = 0.494207 * long_ + 1.24827 * short
    else:                          # tritanopia: manca S
        short = -0.395913 * long_ + 0.801109 * med

    out = (
        0.0809444479 * long_ - 0.130504409 * med + 0.116721066 * short,
        -0.0102485335 * long_ + 0.0540193266 * med - 0.113614708 * short,
        -0.000365296938 * long_ - 0.00412161469 * med + 0.693511405 * short,
    )
    return "#" + "".join(f"{round(_to_srgb(c) * 255):02X}" for c in out)


def self_check() -> list[str]:
    """La misura misura? Valori noti, verificati contro la definizione e non contro se' stessi.

    Serve perche' un errore di colorimetria e' **silenzioso**: una matrice trasposta produce numeri
    plausibili, e il gate direbbe «palette valida» di una palette che non lo e'.
    """
    errors: list[str] = []

    if abs(contrast_ratio("#FFFFFF", "#000000") - 21.0) > 1e-9:
        errors.append("contrasto bianco/nero != 21: la luminanza relativa e' sbagliata")
    if abs(contrast_ratio("#123456", "#123456") - 1.0) > 1e-9:
        errors.append("contrasto di un colore con se stesso != 1")

    white = rgb_to_lab("#FFFFFF")
    if abs(white[0] - 100) > 0.01 or abs(white[1]) > 0.01 or abs(white[2]) > 0.01:
        errors.append(f"Lab del bianco != (100,0,0): {white}")
    if abs(rgb_to_lab("#000000")[0]) > 1e-9:
        errors.append("L* del nero != 0")
    if delta_e("#FFFFFF", "#FFFFFF") > 1e-9:
        errors.append("dE di un colore con se stesso != 0")

    # Un grigio non ha tinta: nessuna dicromazia puo' spostarlo. Se lo sposta, la matrice e' storta.
    for kind in CVD_KINDS:
        moved = delta_e("#808080", simulate_cvd("#808080", kind))
        if moved > 2.0:
            errors.append(f"{kind}: un grigio neutro si sposta di dE={moved:.1f} (matrice sospetta)")

    # La deuteranopia DEVE avvicinare rosso e verde: e' la sua definizione.
    if delta_e(simulate_cvd("#D55E00", "deuteranopia"),
               simulate_cvd("#009E73", "deuteranopia")) >= delta_e("#D55E00", "#009E73"):
        errors.append("deuteranopia: rosso e verde non si avvicinano — la simulazione non simula")

    return errors


# --------------------------------------------------------------------------------------------------
# Il referto della palette
# --------------------------------------------------------------------------------------------------

def check_palette(phase_ink: dict, states: dict, surfaces: dict) -> list[str]:
    """I criteri con cui D-233 ha scelto i quattro colori, riapplicati. Vuoto = passano.

    `phase_ink` fase -> HEX · `states` nome -> HEX degli stati HUD · `surfaces` nome -> HEX dei fondi.
    Nessuno dei tre e' scritto qui: li passa il chiamante, che e' l'owner dei propri colori.
    """
    errors = [f"colorimetria: {e}" for e in self_check()]
    if errors:
        return errors  # se la misura e' rotta, i suoi verdetti non valgono nulla

    for kind in CVD_KINDS:
        for a, b in itertools.combinations(sorted(phase_ink), 2):
            distance = delta_e(simulate_cvd(phase_ink[a], kind), simulate_cvd(phase_ink[b], kind))
            if distance < CVD_MIN_DELTA_E:
                errors.append(
                    f"`{a}` e `{b}` distano dE={distance:.1f} in {kind}, sotto {CVD_MIN_DELTA_E} — "
                    "in quella dicromazia sono lo stesso colore")

    for phase, ink in sorted(phase_ink.items()):
        for state, colour in sorted(states.items()):
            distance = delta_e(ink, colour)
            if distance < STATE_MIN_DELTA_E:
                errors.append(
                    f"`{phase}` ({ink}) dista dE={distance:.1f} dallo STATO `{state}` ({colour}), "
                    f"sotto {STATE_MIN_DELTA_E} — un colore di fase non deve somigliare a uno stato")

        for surface, background in sorted(surfaces.items()):
            ratio = contrast_ratio(ink, background)
            if ratio < MIN_CONTRAST and (phase, surface) not in CONTRAST_EXEMPT:
                errors.append(
                    f"`{phase}` ({ink}) ha contrasto {ratio:.2f} su `{surface}`, sotto "
                    f"{MIN_CONTRAST}:1 (WCAG 1.4.11)")

    for (phase, surface), why in sorted(CONTRAST_EXEMPT.items()):
        if phase not in phase_ink or surface not in surfaces:
            errors.append(f"deroga stantia: `{phase}` su `{surface}` non esiste piu' — {why}")
        elif contrast_ratio(phase_ink[phase], surfaces[surface]) >= MIN_CONTRAST:
            errors.append(
                f"deroga stantia: `{phase}` su `{surface}` ora PASSA il contrasto — "
                "toglila da CONTRAST_EXEMPT invece di lasciarla mentire")

    return errors


def palette_report(phase_ink: dict, states: dict, surfaces: dict) -> dict:
    """Tutti i numeri, senza giudizio. E' cio' che si legge quando si sceglie, non quando si verifica."""
    pairs = {}
    for kind in ("normale",) + CVD_KINDS:
        seen = {k: (v if kind == "normale" else simulate_cvd(v, kind)) for k, v in phase_ink.items()}
        pairs[kind] = {f"{a}/{b}": round(delta_e(seen[a], seen[b]), 1)
                       for a, b in itertools.combinations(sorted(phase_ink), 2)}
    order = sorted((relative_luminance(v), k) for k, v in phase_ink.items())
    return {
        "fasi": {k: {"hex": v,
                     "luminanza": round(relative_luminance(v), 3),
                     "contrasto": {s: round(contrast_ratio(v, b), 2) for s, b in surfaces.items()},
                     "stato_piu_vicino": min(((round(delta_e(v, c), 1), s)
                                              for s, c in states.items()))}
                 for k, v in phase_ink.items()},
        "delta_e": pairs,
        "peggior_caso_cvd": min(min(p.values()) for k, p in pairs.items() if k != "normale"),
        "grayscale": {"ordine": [k for _, k in order],
                      "delta_luminanza_minima": round(
                          min(order[i + 1][0] - order[i][0] for i in range(len(order) - 1)), 3)},
        "soglie": {"cvd": CVD_MIN_DELTA_E, "stati": STATE_MIN_DELTA_E, "contrasto": MIN_CONTRAST},
        "deroghe": {f"{p}/{s}": w for (p, s), w in CONTRAST_EXEMPT.items()},
    }


def _palette_sources() -> tuple[dict, dict, dict]:
    """I colori vivono nel generatore, che ne e' l'owner. L'import e' QUI e non in testa al file
    perche' il generatore importa questo modulo: a livello di modulo sarebbe circolare."""
    from generate_hud_assets import CHROME, PHASE_INK  # noqa: PLC0415

    states = {k: CHROME[k] for k in ("Cyan", "Violet", "Amber", "Red")}
    states["Disabled"] = "#6B7280"  # il grigio di `SEMANTIC`, che e' uno stato e non una famiglia
    surfaces = {k: CHROME[k] for k in ("BG_Panel", "BG_Raised")}
    return PHASE_INK, states, surfaces


if __name__ == "__main__":
    import sys

    ink, hud_states, hud_surfaces = _palette_sources()
    if "--json" in sys.argv:
        print(json.dumps(palette_report(ink, hud_states, hud_surfaces), indent=2, ensure_ascii=False))
        raise SystemExit(0)

    report = palette_report(ink, hud_states, hud_surfaces)
    print("Palette delle macro-fasi (D-233)\n")
    for phase, data in report["fasi"].items():
        near = data["stato_piu_vicino"]
        contrasts = " ".join(f"{s}={v}" for s, v in data["contrasto"].items())
        print(f"  {phase:6s} {data['hex']}  L={data['luminanza']:.3f}  {contrasts}"
              f"  stato piu' vicino: {near[1]} (dE={near[0]})")
    print()
    for kind, pairs in report["delta_e"].items():
        worst = min(pairs.items(), key=lambda kv: kv[1])
        print(f"  {kind:14s} dE min={worst[1]:5.1f} ({worst[0]})")
    gray = report["grayscale"]
    print(f"\n  grayscale: {' < '.join(gray['ordine'])}   dL min={gray['delta_luminanza_minima']}")
    print("  (dL basso NON e' un difetto da correggere: e' perche' D-232 tiene il marker obbligatorio)")

    problems = check_palette(ink, hud_states, hud_surfaces)
    print()
    for (phase, surface), why in sorted(CONTRAST_EXEMPT.items()):
        print(f"  deroga dichiarata: {phase} su {surface} — {why}")
    if problems:
        print(f"\n{len(problems)} criteri non rispettati:")
        for problem in problems:
            print(f"   {problem}")
        raise SystemExit(1)
    print(f"\nOK: peggior caso CVD dE={report['peggior_caso_cvd']} (soglia {CVD_MIN_DELTA_E})")
