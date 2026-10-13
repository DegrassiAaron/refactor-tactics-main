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
import re
import sys

from action_axes import REPO_ROOT  # la radice del repository ha gia' un owner fra i moduli fratelli

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


# --------------------------------------------------------------------------------------------------
# Il world overlay, dove lo stesso canale porta DUE assi
# --------------------------------------------------------------------------------------------------
# `check_palette` guarda la palette delle ICONE: fase contro fase, fase contro stato, fase contro
# fondo. Il world overlay e' un'altra superficie con un'altra grammatica — li' il colore dice
# l'IDENTITA' di squadra, ciano le proprie e giallo il nemico rivelato — ma la linea di scatto porta
# la FASE. Due assi sullo stesso canale, e nessuna delle misure sopra li ha mai confrontati.
#
# 🔴 **E' esattamente cosi' che il magenta e' entrato.** `FLinearColor(1, 0.2, 0.9)` non apparteneva a
# nessuna palette e nessun gate poteva dirlo. Misurato dopo (D-234): distava `dE 20.7` dal ciano di
# squadra in deuteranopia e `20.9` dal giallo in tritanopia — sopra la soglia per meno di un punto,
# su due dicromazie diverse.
#
# 🔴 **Ed e' la ragione per cui i criteri sono DUE e non uno.** Il magenta passava la misura numerica
# — `20.7` contro una soglia di `20.0`, per sette decimi — e a prenderlo e' la seconda regola: un
# colore che non appartiene a nessuna palette non e' entrato per decisione, e' entrato per caso. La
# distanza dice se due colori si confondono; la provenienza dice se qualcuno li ha scelti.
#
# ⚠️ La soglia e' `STATE_MIN_DELTA_E` e **non** `CVD_MIN_DELTA_E`, e la ragione e' gia' scritta: D-233
# ha scartato il giallo `#F0E442` perche' *«un colore di fase indistinguibile da uno stato e' un
# difetto peggiore di due fasi vicine, perche' mescola due assi invece di due valori dello stesso
# asse»*. Identita' di squadra e fase sono due assi.


def _linear_to_hex(triple: tuple[float, float, float]) -> str:
    """`FLinearColor` porta valori LINEARI, la palette e' in sRGB.

    Confrontarli senza codificare darebbe numeri sbagliati e **plausibili**: il ciano di squadra e'
    `#7CF3FF`, non `#33E6FF`. E' lo stesso errore che `Map/RTHexMapActor.cpp:909` documenta al
    contrario — *«dividere per 255 darebbe tinte slavate»*.
    """
    return "#" + "".join(f"{round(_to_srgb(c) * 255):02X}" for c in triple)


def _strip_cpp_comments(text: str) -> str:
    """Via i commenti del C++, e via il CONTENUTO delle stringhe. Una riga commentata non e' una riga del gioco.

    Un lessico vero e non due sostituzioni, e la differenza e' misurata: togliendo prima i commenti a
    blocco e poi quelli di riga, un `/*` scritto DENTRO un commento di riga — `// vedi Content/RT/UI/*` —
    apre un blocco che inghiotte il codice vivo fino al prossimo `*/`. E un `//` dentro una stringa
    (`TEXT("http://x")`) taglia il resto della riga. Trovati in code review (`#3626`).

    Le stringhe restano come `""`: cosi' un ancoraggio citato in un letterale non conta come codice. I
    ritorni a capo si conservano, perche' i messaggi d'errore non devono cambiare riga.
    """
    out: list[str] = []
    index, size = 0, len(text)
    while index < size:
        char = text[index]
        pair = text[index:index + 2]
        if pair == "//":
            end = text.find("\n", index)
            index = size if end == -1 else end
        elif pair == "/*":
            end = text.find("*/", index + 2)
            stop = size if end == -1 else end + 2
            out.append("\n" * text.count("\n", index, stop) or " ")
            index = stop
        elif char in "\"'":
            quote = char
            out.append(quote)
            index += 1
            while index < size and text[index] != quote and text[index] != "\n":
                index += 2 if text[index] == "\\" else 1
            out.append(quote)
            # Un letterale non chiuso sulla riga non si porta via il ritorno a capo.
            if index < size and text[index] == quote:
                index += 1
        else:
            out.append(char)
            index += 1
    return "".join(out)


def _function_body(code: str, name: str) -> str:
    """Il corpo di `ARTHUD::<name>(...) { ... }`, graffe escluse. `code` e' gia' senza commenti.

    Serve a contare le assegnazioni a `Out.Color` DENTRO la funzione dell'identita' e non in tutto il file:
    in `RTHUD.cpp` piu' funzioni hanno un locale che si chiama `Out`, e un colore assegnato in una di loro
    non c'entra con l'identita' di squadra.
    """
    heads = [m for m in re.finditer(r"\bARTHUD::" + re.escape(name) + r"\s*\(", code)]
    if len(heads) != 1:
        raise RuntimeError(
            f"`ARTHUD::{name}` compare {len(heads)} volte in RTHUD.cpp (attesa: una definizione): la "
            "funzione che dichiara l'identita' di squadra e' stata rinominata o spostata.")
    open_brace = code.find("{", heads[0].end())
    if open_brace == -1:
        raise RuntimeError(f"`ARTHUD::{name}` non ha un corpo in RTHUD.cpp.")
    depth = 0
    for index in range(open_brace, len(code)):
        if code[index] == "{":
            depth += 1
        elif code[index] == "}":
            depth -= 1
            if depth == 0:
                return code[open_brace + 1:index]
    raise RuntimeError(f"il corpo di `ARTHUD::{name}` non si chiude in RTHUD.cpp.")


# Un letterale numerico del C++ come lo scrive il modulo: `0.2f`, `1.f`, `1`, `.5f`.
_FLOAT = r"(\d+\.?\d*|\.\d+)f?"
_BYTE = r"(\d{1,3})"
_SRGB_ARGS = rf"\s*{_BYTE}\s*,\s*{_BYTE}\s*,\s*{_BYTE}\s*(?:,\s*\d{{1,3}}\s*)?"
_LINEAR_ARGS = rf"\s*{_FLOAT}\s*,\s*{_FLOAT}\s*,\s*{_FLOAT}\s*(?:,\s*{_FLOAT}\s*)?"
# Le forme di colore che il referto sa misurare. Ognuna deve coprire l'espressione PER INTERO.
_COLOR_FORMS = (
    ("srgb", re.compile(rf"FLinearColor::FromSRGBColor\(\s*FColor\({_SRGB_ARGS}\)\s*\)")),
    ("srgb", re.compile(rf"FLinearColor\(\s*FColor\({_SRGB_ARGS}\)\s*\)")),
    ("srgb", re.compile(rf"FColor\({_SRGB_ARGS}\)")),
    ("linear", re.compile(rf"FLinearColor\({_LINEAR_ARGS}\)")),
)


def _parse_color_expr(expr: str, where: str) -> str:
    """Un LETTERALE di colore del C++ -> HEX sRGB. L'espressione deve essere il letterale, e nient'altro.

    Le forme ammesse:

      * `FLinearColor::FromSRGBColor(FColor(r, g, b))`, `FLinearColor(FColor(r, g, b))` e `FColor(r, g, b)`:
        byte sRGB. Il costruttore da `FColor` converte come `FromSRGBColor`;
      * `FLinearColor(r, g, b, a)`: valori LINEARI.

    🔴 **Si confronta l'espressione INTERA, non si cerca un colore dentro.** Cercarlo accettava in silenzio
    tutto cio' che gli stava intorno: `A * 0.25f`, `FMath::Lerp(A, C, 0.5f)`, `A.Desaturate(0.8f)`, un
    ternario annidato — in ciascuno si misurava il primo letterale e non il colore che il gioco disegna.
    Il referto misura letterali: un colore CALCOLATO va dichiarato qui prima, con la sua forma.

    ⚠️ **L'ordine delle forme conta.** Letta come lineare, `FLinearColor(FColor(51, 230, 255))` darebbe
    `#FFFFFF`: tre «float» sopra 1, schiacciati a bianco, senza errore. Trovati in code review (`#3626`).
    """
    text = expr.strip()
    for kind, form in _COLOR_FORMS:
        match = form.fullmatch(text)
        if not match:
            continue
        triple = match.groups()[:3]
        if kind == "srgb":
            if any(int(c) > 255 for c in triple):
                raise RuntimeError(f"colore fuori scala in {where}: `{text[:70]}` porta un byte sopra 255.")
            return "#" + "".join(f"{int(c):02X}" for c in triple)
        return _linear_to_hex(tuple(float(c) for c in triple))
    raise RuntimeError(
        f"colore non riconosciuto in {where}: `{text[:70]}`. Il referto misura LETTERALI, nelle forme "
        "`FLinearColor(r, g, b, a)` (lineare) e `FColor(r, g, b)` (sRGB), anche dentro "
        "`FLinearColor::FromSRGBColor(...)`. Un colore calcolato — un prodotto, un `Lerp`, un ternario — "
        "non si misura da qui.")


# Dove `RTHUD.cpp` dichiara l'asse dell'IDENTITA' di squadra, e in quale forma. Una sola, e scritta qui
# perche' la usano la lettura e i suoi messaggi d'errore.
#
# ⛔ **Non si accettano le forme passate.** Fino a `#2184` la riga era `const FLinearColor Color = bOwn ? … : …;`;
# riconoscerla ancora terrebbe vivo un ramo che nessun sorgente esercita, e nasconderebbe il giorno in cui
# la forma cambia di nuovo. Una forma che non e' QUESTA e' un errore esplicito.
_IDENTITY_FUNCTION = "ComposeIntentPresentation"
_IDENTITY_ANCHOR = r"\bOut\.Color\s*=\s*bOwn\b(.*?);"
_IDENTITY_FORM = "`Out.Color = bOwn ? <colore> : <colore>;`"
# OGNI scrittura del campo: `=`, un operatore composto (`*=`), un canale (`Out.Color.R = …`).
# `=(?!=)`: un confronto `==` non e' una scrittura.
_IDENTITY_WRITE = r"\bOut\.Color\b\s*(?:\.\s*\w+\s*)?(?:[-+*/%&|^]|<<|>>)?=(?!=)"
# La dichiarazione della linea di scatto: `= <colore>;` oppure il costruttore `(<argomenti>);`.
_DASH_ANCHOR = r"\bconst\s+FLinearColor\s+DashColor\b\s*(=?)\s*([^;]*);"


def _split_ternary(body: str) -> tuple[str, str] | None:
    """`? A : B` -> `(A, B)`. Il `:` e' quello a parentesi chiuse, e non meta' di un `::`.

    I due rami si passano INTERI a `_parse_color_expr`, che ne pretende un letterale: un ternario annidato
    in uno dei due — una terza tinta — non e' un letterale, e fa fallire la lettura invece di essere
    letto a meta'.
    """
    body = body.strip()
    if not body.startswith("?"):
        return None
    depth = 0
    for index, char in enumerate(body):
        if char in "([{":
            depth += 1
        elif char in ")]}":
            depth -= 1
        elif char == ":" and depth == 0:
            if body[index - 1:index] == ":" or body[index + 1:index + 2] == ":":
                continue
            return body[1:index].strip(), body[index + 1:].strip()
    return None


def parse_overlay_colors(text: str) -> tuple[dict, dict]:
    """I colori del world overlay da un TESTO C++: identita' di squadra, e fase della linea di scatto.

    🔑 **Una funzione sul testo, non sul file** (`#3626`). La lettura si era rotta il 2026-09-04, quando
    `#2184` ha riscritto la riga dell'identita', ed e' rimasta rotta fino al 2026-10-13: il referto falliva
    forte e nessun comando lo lanciava. Separare il testo dal file permette di provare cio' che conta — la
    forma attuale si legge, quella vecchia e una inventata danno errore — senza toccare `RTHUD.cpp`
    (`color_metrics_test.py`).

    🔑 **I commenti si tolgono PRIMA di cercare.** Senza, una dichiarazione vecchia lasciata in un commento
    sopra quella viva verrebbe misurata al suo posto, e il referto uscirebbe verde su colori che il gioco
    non disegna piu'.

    Fallisce **forte**, invece di restituire un insieme a meta' e passare in silenzio, se:

      * la funzione dell'identita' o un ancoraggio spariscono, o compaiono piu' di una volta;
      * il campo dell'identita' e' scritto anche altrove NELLA STESSA FUNZIONE — una terza tinta, un `*=`,
        un canale — perche' non starebbe nel ternario e non verrebbe misurata contro nessuno;
      * il lato destro non e' un ternario a due rami, o un ramo non e' un letterale di colore.

    ⚠️ **Che cosa NON vede.** Un colore cambiato dopo, fuori da questa funzione o con un metodo che non e'
    un'assegnazione, non passa di qui. La lettura e' per forma: chiude i casi che conosce, e lo dichiara.
    """
    code = _strip_cpp_comments(text)

    body = _function_body(code, _IDENTITY_FUNCTION)
    idents = re.findall(_IDENTITY_ANCHOR, body, re.S)
    if not idents:
        raise RuntimeError(
            f"{_IDENTITY_FORM} non trovato in `ARTHUD::{_IDENTITY_FUNCTION}`: l'asse dell'IDENTITA' di "
            "squadra ha cambiato forma. I colori non si riscrivono qui, si rileggono da la': aggiorna "
            "l'ancoraggio.")
    if len(idents) != 1:
        raise RuntimeError(
            f"{_IDENTITY_FORM} compare {len(idents)} volte in `ARTHUD::{_IDENTITY_FUNCTION}`: "
            "l'ancoraggio e' ambiguo, e scegliere la prima occorrenza misurerebbe una riga a caso.")
    writes = len(re.findall(_IDENTITY_WRITE, body))
    if writes != 1:
        raise RuntimeError(
            f"`Out.Color` e' scritto {writes} volte in `ARTHUD::{_IDENTITY_FUNCTION}`, e solo una e' il "
            "ternario su `bOwn`: un colore scritto da un'altra istruzione non verrebbe misurato. Se "
            "l'identita' ha una terza tinta, va letta e confrontata qui prima di essere disegnata.")
    arms = _split_ternary(idents[0])
    if arms is None:
        raise RuntimeError(
            f"l'asse dell'identita' non ha la forma {_IDENTITY_FORM}: dopo `bOwn` non c'e' un ternario a "
            "due rami (proprie / nemico rivelato), e la lettura sarebbe parziale.")
    identity = {"proprie": _parse_color_expr(arms[0], "RTHUD.cpp identita' (proprie)"),
                "nemico": _parse_color_expr(arms[1], "RTHUD.cpp identita' (nemico)")}

    dashes = re.findall(_DASH_ANCHOR, code)
    if not dashes:
        raise RuntimeError(
            "`const FLinearColor DashColor` non trovato in RTHUD.cpp: la preview dello scatto ha cambiato "
            "forma. E' il solo elemento dell'overlay che porta un colore di FASE, e senza non c'e' niente "
            "da misurare.")
    if len(dashes) != 1:
        raise RuntimeError(
            f"`const FLinearColor DashColor` compare {len(dashes)} volte in RTHUD.cpp: l'ancoraggio e' "
            "ambiguo, e scegliere la prima occorrenza misurerebbe una riga a caso.")
    assigned, initializer = dashes[0]
    # Il costruttore `DashColor(r, g, b, a)` arriva senza il nome del tipo: glielo si rimette, cosi' la
    # grammatica dei letterali resta una sola.
    dash_expr = initializer if assigned else "FLinearColor" + initializer.strip()
    return identity, {"Dash": _parse_color_expr(dash_expr, "RTHUD.cpp DashColor")}


def overlay_colors(repo_root=None) -> tuple[dict, dict]:
    """I colori del world overlay, **letti** da `RTHUD.cpp` invece che ricopiati qui.

    Stessa regola di `match_phase_map()` nel generatore: una mappa scritta a mano direbbe la cosa
    vecchia il giorno dopo il cambio, e nessuno se ne accorgerebbe. L'interpretazione e' in
    `parse_overlay_colors`. `repo_root` serve alla prova, che lo punta a un albero finto.
    """
    root = REPO_ROOT if repo_root is None else repo_root
    text = (root / "Source/RefactorTactics/UI/RTHUD.cpp").read_text(encoding="utf-8")
    return parse_overlay_colors(text)


def inks_by_owner(phase_ink: dict, chrome: dict, semantic: dict) -> dict:
    """Ogni HEX delle tre tabelle, con il nome di chi lo possiede. Chi le passa ne e' l'owner."""
    known: dict = {}
    for owner, table in (("fase", phase_ink), ("chrome", chrome), ("semantico", semantic)):
        for name, value in table.items():
            known.setdefault(value.upper(), f"{owner}/{name}")
    return known


def known_inks() -> dict:
    """Ogni HEX che il sistema conosce, con il nome di chi lo possiede."""
    from generate_hud_assets import CHROME, PHASE_INK, SEMANTIC  # noqa: PLC0415

    return inks_by_owner(PHASE_INK, CHROME, SEMANTIC)


def _by_vision(first: str, second: str) -> dict:
    """dE fra due colori: a occhio sano, e sotto ciascuna delle tre dicromazie."""
    row = {"normale": round(delta_e(first, second), 1)}
    for kind in CVD_KINDS:
        row[kind] = round(delta_e(simulate_cvd(first, kind), simulate_cvd(second, kind)), 1)
    return row


def overlay_pairs(identity: dict, phase: dict) -> dict:
    """dE fra ogni colore di fase dell'overlay e ogni colore di identita', per tipo di visione."""
    out: dict = {}
    for name, ink in sorted(phase.items()):
        for who, other in sorted(identity.items()):
            out[f"{name}/{who}"] = _by_vision(ink, other)
    return out


def identity_pairs(identity: dict) -> dict:
    """dE fra i colori dell'identita' di squadra, a due a due, per tipo di visione."""
    return {f"{a}/{b}": _by_vision(identity[a], identity[b])
            for a, b in itertools.combinations(sorted(identity), 2)}


def check_overlay(identity: dict, phase: dict, known: dict | None = None,
                  phase_ink: dict | None = None) -> list[str]:
    """I tre criteri dell'overlay del mondo (D-234). Vuoto = passano.

      1. **Provenienza.** Un colore di fase dell'overlay e' QUELLO che la palette assegna alla sua fase:
         `Dash` prende in prestito l'inchiostro di `Dash`, non un altro colore dichiarato. Senza
         `phase_ink` — o per una fase che la palette non ha — resta il criterio largo: appartiene a una
         palette qualunque.
      2. **Fase contro identita'**: due ASSI sullo stesso canale. Soglia `STATE_MIN_DELTA_E`, per la
         ragione scritta in testa a questa sezione.
      3. **Identita' contro identita'**: le proprie unita' e il nemico rivelato sono due VALORI dello
         stesso asse, come due fasi fra loro. Soglia `CVD_MIN_DELTA_E`, che e' quella con cui D-233 separa
         due valori di un asse. ⚠️ Fino a `#3626` nessuno misurava questa coppia, e due commenti in
         `RTHUD.cpp` e nei suoi test dicevano che `T9` la teneva.

    `known` sono gli inchiostri che il sistema dichiara (`inks_by_owner`) e `phase_ink` la palette delle
    fasi. Il generatore passa i propri; senza argomenti si leggono da lui.
    """
    errors = []
    known = known_inks() if known is None else known
    if phase_ink is None:
        from generate_hud_assets import PHASE_INK as phase_ink  # noqa: PLC0415

    for name, ink in sorted(phase.items()):
        expected = phase_ink.get(name)
        if expected is not None and ink.upper() != expected.upper():
            errors.append(
                f"overlay: `{name}` ({ink}) non e' l'inchiostro che la palette assegna alla fase `{name}` "
                f"({expected}) — sull'overlay la fase PRENDE IN PRESTITO quel colore, non ne sceglie un "
                "altro (D-234)")
        elif expected is None and ink.upper() not in known:
            errors.append(
                f"overlay: `{name}` ({ink}) non appartiene a NESSUNA palette — un colore nuovo si "
                "dichiara nel documento owner, non si scopre in un widget (D-234)")
    for pair, row in sorted(overlay_pairs(identity, phase).items()):
        for kind, distance in row.items():
            if distance < STATE_MIN_DELTA_E:
                errors.append(
                    f"overlay: `{pair}` dista dE={distance} in {kind}, sotto {STATE_MIN_DELTA_E} — "
                    "il colore di fase e quello di squadra sono due ASSI, e non devono confondersi")
    for pair, row in sorted(identity_pairs(identity).items()):
        for kind, distance in row.items():
            if distance < CVD_MIN_DELTA_E:
                errors.append(
                    f"overlay: le identita' `{pair}` distano dE={distance} in {kind}, sotto "
                    f"{CVD_MIN_DELTA_E} — in quella visione le proprie unita' e il nemico rivelato sono "
                    "lo stesso colore")
    return errors


# --------------------------------------------------------------------------------------------------
# Le tabelle del generatore sono COPIE, e i loro owner sono due documenti (#3627)
# --------------------------------------------------------------------------------------------------
# `CHROME` e `PHASE_INK` copiano i token `RT_UI_*` di `progettazione-hud.md` §32; `SEMANTIC` copia i token
# `RT_Sem_*` di `02-color-system.md` §2. Il generatore lo dichiara in un commento — *«nessun valore inventato
# qui: se un colore serve e non c'e', si aggiunge prima al documento owner»* — e fino a `#3627` nessuno
# confrontava le copie. `URTUIPalette`, la sede a runtime, e' confrontata con §32 da un test C++
# (`UI.Palette.MatchesStyleGuide`): questo chiude il terzo lato.
#
# 🔑 **Due vocabolari, non uno.** `RT_Sem_Disabled`, il grigio semantico delle icone, e
# `RT_UI_Text_Disabled`, il testo spento dell'HUD, sono due token con due owner e due valori. La issue li
# leggeva come due copie divergenti dello stesso valore: non lo sono, e il confronto e' PER NOME dentro
# ciascun vocabolario proprio per non fonderli (decisione d'autore `P4`, 2026-10-13).
#
# ⚠️ Il verso e' uno solo: ogni voce di una TABELLA deve avere il suo token nel documento, con lo stesso
# valore. Un token del documento senza voce in tabella non e' un errore — §32 ne ha molti che le icone non
# usano.
#
# ⚠️ **Si legge la SEZIONE owner, non il documento.** Altre sezioni degli stessi file riusano il prefisso, e
# una riga omonima fuori dalla sezione farebbe passare per dichiarato un token che la tabella owner non ha
# piu' — o, con un altro valore, farebbe dire «dichiarato due volte» a una tabella in ordine. E' il confine
# che legge anche il gemello in C++ (`RigheDiPalette`, da `# 32.` a `# 33.`).
#
# ⚠️ **Una riga che NOMINA il token e non porta un esadecimale e' una lettura fallita, non un token che
# manca.** Cercare nome e valore con una sola espressione scartava in silenzio la riga malformata, e il
# referto rispondeva «un colore si aggiunge prima al documento owner» di un colore che nel documento c'era:
# il rimedio sbagliato, col codice d'uscita sbagliato.

# Che cosa puo' andare storto LEGGENDO, e che non e' un criterio violato: la forma (`RuntimeError`, alzato
# da questo modulo), il file che manca o non si apre (`OSError`), il file che non e' UTF-8. Lo usa anche
# il generatore: e' UN elenco, perche' una classe di errore aggiunta qui e non la' farebbe uscire il
# generatore con un traceback dopo aver scritto le icone e prima del verdetto.
READ_ERRORS = (RuntimeError, OSError, UnicodeDecodeError)

HUD_STYLE_DOC = "docs/technical/systems/progettazione-hud.md"
SEMANTIC_DOC = "docs/research/design/icon/visual-language/02-color-system.md"
# Il titolo che apre la sezione owner, e la forma del primo titolo dello STESSO livello che la chiude.
_UI_SECTION = (re.compile(r"^# 32\.[^\n]*$", re.M), re.compile(r"^# \d+\.", re.M))
_SEM_SECTION = (re.compile(r"^## 2\.[^\n]*$", re.M), re.compile(r"^## \d+\.", re.M))
_HEX_CELL = re.compile(r"`(#[0-9A-Fa-f]{6})`")
# Il separatore di cella, non la barra con l'escape che una cella puo' contenere.
_CELL_SEPARATOR = re.compile(r"(?<!\\)\|")


def owner_section(text: str, bounds: tuple, where: str) -> str:
    """Il testo della sezione owner: dal suo titolo al primo titolo dello stesso livello che segue.

    Fallisce forte se il titolo non c'e' o c'e' due volte: senza un confine certo si tornerebbe a leggere
    l'intero documento, che e' il difetto che questa funzione chiude.
    """
    opens, closes = bounds
    found = list(opens.finditer(text))
    if len(found) != 1:
        raise RuntimeError(
            f"{where}: il titolo della sezione owner (`{opens.pattern}`) compare {len(found)} volte, e ne "
            "serve una. Il documento ha cambiato struttura, e il confronto con le copie non puo' essere fatto.")
    start = found[0].end()
    following = closes.search(text, start)
    return text[start:following.start() if following else len(text)]


def read_doc_tokens(text: str, prefix: str, bounds: tuple, where: str) -> dict:
    """I token che la sezione owner di un documento dichiara: nome (senza prefisso) -> HEX.

    Una riga di tabella dichiara un token quando una sua cella e' il token fra apici inversi, e la cella
    ACCANTO ne e' il valore. Fallisce forte in quattro casi, tutti letture fallite e non divergenze: la
    sezione non si trova; la cella accanto al token non e' un esadecimale a sei cifre; un token e'
    dichiarato due volte; nessun token letto.
    """
    token_cell = re.compile("`" + re.escape(prefix) + r"(\w+)`")
    tokens: dict = {}
    for line in owner_section(text, bounds, where).splitlines():
        if not line.lstrip().startswith("|"):
            continue
        cells = [cell.strip() for cell in _CELL_SEPARATOR.split(line.strip())[1:]]
        for index, cell in enumerate(cells):
            named = token_cell.fullmatch(cell)
            if named is None:
                continue
            name = named.group(1)
            beside = cells[index + 1] if index + 1 < len(cells) else ""
            value = _HEX_CELL.fullmatch(beside)
            if value is None:
                raise RuntimeError(
                    f"{where}: la riga del token `{prefix}{name}` non porta un esadecimale `#RRGGBB` nella "
                    f"cella accanto (c'e' «{beside}»). La tabella ha cambiato forma: il token c'e', ed e' "
                    "il suo valore che non si legge.")
            if name in tokens:
                raise RuntimeError(
                    f"{where} dichiara il token `{prefix}{name}` due volte ({tokens[name]} e "
                    f"{value.group(1)}): la sezione owner non e' piu' una fonte sola.")
            tokens[name] = value.group(1)
            break
    if not tokens:
        raise RuntimeError(
            f"{where}: nessun token letto. La tabella dei colori ha cambiato forma, e il confronto con le "
            "copie del generatore non puo' essere fatto.")
    return tokens


def check_table(table_name: str, table: dict, tokens: dict, token_prefix: str, where: str,
                key_prefix: str = "") -> list[str]:
    """Ogni voce della tabella ha il suo token nel documento, con lo stesso valore. Vuoto = coincidono."""
    errors = []
    for key, value in table.items():
        token = token_prefix + key_prefix + key
        declared = tokens.get(key_prefix + key)
        if declared is None:
            errors.append(
                f"`{table_name}[\"{key}\"]` ({value}) non ha il token `{token}` in {where} — un colore si "
                "aggiunge prima al documento owner, poi alla copia")
        elif declared.upper() != value.upper():
            errors.append(
                f"`{table_name}[\"{key}\"]` vale {value}, ma `{token}` in {where} vale {declared} — la copia "
                "e il documento owner divergono")
    return errors


def check_palette_owners(phase_ink: dict, chrome: dict, semantic: dict,
                         repo_root=None) -> tuple[list[str], list[str]]:
    """Le tre tabelle del generatore contro i due documenti owner: `(divergenze, letture_fallite)`.

    🔑 **I due documenti si leggono e si giudicano SEPARATAMENTE.** Un documento che non si legge non e' una
    divergenza, e non deve portarsi via il giudizio sull'altro: `CHROME` e `PHASE_INK` si confrontano con
    §32 anche se il documento semantico manca. Per questo la funzione non alza: restituisce le letture
    fallite accanto alle divergenze, e chi chiama tiene i due rossi distinti.
    """
    root = REPO_ROOT if repo_root is None else repo_root
    owners = (
        (HUD_STYLE_DOC, "RT_UI_", _UI_SECTION, (("CHROME", chrome, ""), ("PHASE_INK", phase_ink, "Phase_"))),
        (SEMANTIC_DOC, "RT_Sem_", _SEM_SECTION, (("SEMANTIC", semantic, ""),)),
    )
    divergences: list[str] = []
    unread: list[str] = []
    for where, prefix, bounds, tables in owners:
        try:
            tokens = read_doc_tokens((root / where).read_text(encoding="utf-8"), prefix, bounds, where)
        except READ_ERRORS as error:
            unread.append(f"{type(error).__name__}: {error}")
            continue
        for table_name, table, key_prefix in tables:
            divergences += check_table(table_name, table, tokens, prefix, where, key_prefix)
    return divergences, unread


def hud_state_sets(chrome: dict, semantic: dict) -> tuple[dict, dict]:
    """Gli stati e le superfici dell'HUD contro cui si misurano i colori di fase: `(stati, superfici)`.

    UNA definizione, per il referto e per il gate `T9` del generatore. Erano due elenchi scritti a mano in
    due file, e avevano gia' preso due forme: un letterale da una parte, una voce di tabella dall'altra.

    🔑 **I grigi spenti sono DUE, e si misurano tutti e due** (`#3627`). `Text_Disabled` e' il testo spento
    che l'HUD disegna, un token di §32; `Sem_Disabled` e' la tinta semantica che il generatore assegna alle
    icone che dicono «non disponibile». Fino a `#3627` si misurava solo il secondo. Sostituirlo col primo
    avrebbe tolto dal gate un grigio che le icone portano ancora: una fase puo' somigliare a uno qualunque
    dei due.
    """
    states = {key: chrome[key] for key in ("Cyan", "Violet", "Amber", "Red")}
    states["Text_Disabled"] = chrome["Text_Disabled"]
    states["Sem_Disabled"] = semantic["Disabled"]
    surfaces = {key: chrome[key] for key in ("BG_Panel", "BG_Raised")}
    return states, surfaces


def _palette_sources() -> tuple[dict, dict, dict]:
    """I colori vivono nel generatore, che ne e' l'owner. L'import e' QUI e non in testa al file
    perche' il generatore importa questo modulo: a livello di modulo sarebbe circolare."""
    from generate_hud_assets import CHROME, PHASE_INK, SEMANTIC  # noqa: PLC0415

    states, surfaces = hud_state_sets(CHROME, SEMANTIC)
    return PHASE_INK, states, surfaces


# I codici d'uscita del referto. Due rossi diversi hanno due numeri diversi, come nel generatore (`#3002`):
# «un criterio e' violato» e «non sono riuscito a leggere una fonte» chiedono due rimedi diversi, e con
# un `1` per entrambi chi guarda il solo codice non sa quale applicare. Le fonti sono tre: `RTHUD.cpp` e
# i due documenti owner della palette. Se i due rossi accadono insieme vince il `2`: con una lettura
# fallita il giudizio sui criteri e' parziale, e il referto li stampa comunque.
EXIT_OK = 0
EXIT_CRITERIA_FAILED = 1
EXIT_READ_FAILED = 2


def _stdout_to_utf8() -> None:
    """Il referto stampa trattini lunghi e punti mediani. Su una console `cp850` la stampa alzerebbe
    `UnicodeEncodeError` — e il processo uscirebbe `1`, cioe' col codice di un criterio violato."""
    for stream in (sys.stdout, sys.stderr):
        reconfigure = getattr(stream, "reconfigure", None)
        if reconfigure is not None:
            reconfigure(encoding="utf-8", errors="replace")


def main(argv: list[str] | None = None, repo_root=None) -> int:
    """Il referto. `repo_root` serve alla prova, che lo punta a un albero finto senza toccare quello vero.

    Le tre parti sono indipendenti e si riportano tutte: la palette delle icone non legge `RTHUD.cpp`, il
    confronto delle tabelle legge due documenti, e una lettura fallita non deve portarsi via il resto —
    nemmeno l'altro documento, dentro la terza parte. Vale anche in `--json`.
    """
    argv = sys.argv[1:] if argv is None else argv
    as_json = "--json" in argv
    _stdout_to_utf8()

    ink, hud_states, hud_surfaces = _palette_sources()
    report = palette_report(ink, hud_states, hud_surfaces)
    problems = check_palette(ink, hud_states, hud_surfaces)

    # Il world overlay: stesso canale, due assi. Letto da RTHUD.cpp, mai ricopiato qui (D-234).
    read_error = None
    identity: dict = {}
    overlay_phase: dict = {}
    try:
        identity, overlay_phase = overlay_colors(repo_root)
    except READ_ERRORS as error:
        read_error = f"{type(error).__name__}: {error}"
    pairs = overlay_pairs(identity, overlay_phase)
    known = known_inks()
    if read_error is None:
        problems += check_overlay(identity, overlay_phase, known, ink)

    # Le tabelle del generatore contro i documenti che ne sono owner (`#3627`). Terza parte, indipendente
    # dalle altre due: legge due documenti, non `RTHUD.cpp`, e li giudica uno per volta.
    from generate_hud_assets import CHROME, SEMANTIC  # noqa: PLC0415
    divergences, unread_owners = check_palette_owners(ink, CHROME, SEMANTIC, repo_root)
    problems += divergences

    read_errors = ([read_error] if read_error else []) + unread_owners
    code = EXIT_READ_FAILED if read_errors else (EXIT_CRITERIA_FAILED if problems else EXIT_OK)

    if as_json:
        # ⚠️ Fino a `#3626` questo ramo stampava la palette e usciva `0` senza aprire `RTHUD.cpp` e senza
        # un verdetto: una macchina che consumasse il JSON restava verde mentre il testo era rosso.
        report["overlay"] = ({"errore_di_lettura": read_error} if read_error else
                             {"identita": identity, "fase": overlay_phase, "delta_e": pairs,
                              "delta_e_identita": identity_pairs(identity)})
        # Le due chiavi ci sono SEMPRE: un documento che non si legge non cancella le divergenze
        # trovate nell'altro, e chi consuma il JSON non deve indovinare quale delle due forme arriva.
        report["tabelle"] = {"divergenze": divergences, "letture_fallite": unread_owners}
        report["criteri_non_rispettati"] = problems
        report["exit"] = code
        print(json.dumps(report, indent=2, ensure_ascii=False))
        return code

    print("Palette delle macro-fasi (D-233)\n")
    for phase, data in report["fasi"].items():
        near = data["stato_piu_vicino"]
        contrasts = " ".join(f"{s}={v}" for s, v in data["contrasto"].items())
        print(f"  {phase:6s} {data['hex']}  L={data['luminanza']:.3f}  {contrasts}"
              f"  stato piu' vicino: {near[1]} (dE={near[0]})")
    print()
    for kind, kind_pairs in report["delta_e"].items():
        worst = min(kind_pairs.items(), key=lambda kv: kv[1])
        print(f"  {kind:14s} dE min={worst[1]:5.1f} ({worst[0]})")
    gray = report["grayscale"]
    print(f"\n  grayscale: {' < '.join(gray['ordine'])}   dL min={gray['delta_luminanza_minima']}")
    print("  (dL basso NON e' un difetto da correggere: e' perche' D-232 tiene il marker obbligatorio)")
    print()
    for (phase, surface), why in sorted(CONTRAST_EXEMPT.items()):
        print(f"  deroga dichiarata: {phase} su {surface} — {why}")

    print("\nWorld overlay — lo stesso canale porta due assi (D-234)\n")
    if read_error:
        print("  NON LETTO: vedi l'errore in coda.")
    else:
        for who, hex_value in sorted(identity.items()):
            print(f"  identita' {who:8s} {hex_value}  {known.get(hex_value.upper(), 'vocabolario proprio')}")
        for name, hex_value in sorted(overlay_phase.items()):
            owner = known.get(hex_value.upper(), "NESSUNA PALETTE")
            worst = min(min(r.values()) for k, r in pairs.items() if k.startswith(name + "/"))
            print(f"  fase      {name:8s} {hex_value}  {owner}   dE min vs identita' = {worst}")
        for pair, row in sorted(identity_pairs(identity).items()):
            print(f"  identita' fra loro  {pair}   dE min = {min(row.values())}")

    print("\nTabelle del generatore contro i documenti owner (#3627)\n")
    if unread_owners:
        print("  NON LETTE, in tutto o in parte: vedi l'errore in coda. Cio' che si e' letto e' giudicato.")
    if divergences:
        print("  DIVERGONO: vedi i criteri in coda.")
    if not unread_owners and not divergences:
        print(f"  CHROME e PHASE_INK coincidono con i token `RT_UI_*` di {HUD_STYLE_DOC}")
        print(f"  SEMANTIC coincide con i token `RT_Sem_*` di {SEMANTIC_DOC}")

    if problems:
        print(f"\n{len(problems)} criteri non rispettati:")
        for problem in problems:
            print(f"   {problem}")
    if read_errors:
        for error in read_errors:
            print(f"\nLETTURA FALLITA (exit {EXIT_READ_FAILED}): {error}", file=sys.stderr)
        return code
    if problems:
        return code
    worst_overlay = min(min(r.values()) for r in pairs.values())
    worst_identity = min(min(r.values()) for r in identity_pairs(identity).values())
    print(f"\nOK: peggior caso CVD dE={report['peggior_caso_cvd']} (soglia {CVD_MIN_DELTA_E})"
          f" · overlay fase/identita' dE={worst_overlay} (soglia {STATE_MIN_DELTA_E})"
          f" · identita' fra loro dE={worst_identity} (soglia {CVD_MIN_DELTA_E})")
    return code


if __name__ == "__main__":
    raise SystemExit(main())
