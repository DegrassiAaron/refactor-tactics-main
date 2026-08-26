#!/usr/bin/env python3
"""Generatore degli asset statici della HUD di RefactorTactics.

Perche' uno script e non un file di disegno: gli asset qui prodotti sono **derivati**, non lavoro
d'autore. La grammatica che li governa vive in `docs/research/design/icon/visual-language/` e cambia;
un glifo disegnato a mano invecchia in silenzio, uno generato cambia con una riga e mostra il diff.

Output (tutto sotto `Content/RT/UI/_Generated/`, ignorato da git — si rigenera):

    Icons/    master SVG monocromatico + PNG RGBA a 16/20/24/32/48 px
    Frames/   chrome 9-slice: pannelli, slot, bottoni, portrait, timeline
    Review/   contact sheet a colori e in scala di grigi (test di accettazione)
    manifest.json   scheda di consegna per ogni asset (07-export-e-naming.md §3)

Regole rispettate, e non sono negoziabili:

- **niente testo incorporato**: nessun keybind, costo, cooldown o percentuale finisce dentro un glifo
  (07-export-e-naming.md §2). I chip di keybind sono cornici vuote, il testo lo mette UMG;
- **master monocromatico e tintabile**: la tinta semantica (Okabe-Ito, 02-color-system.md §2) la
  applica il widget, non l'export. Un glifo per tinta sarebbe una texture per ogni tema;
- **il colore e' il secondo canale**: ogni coppia di §7 di 03-forme-e-primitive.md deve restare
  distinguibile nella contact sheet in scala di grigi.

Uso:  python3 tools/hud-assets/generate_hud_assets.py [--out <dir>]
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from action_axes import action_axes

try:
    import cairosvg
except ImportError:  # pragma: no cover - dipendenza opzionale
    cairosvg = None


# --------------------------------------------------------------------------------------------------
# Palette
# --------------------------------------------------------------------------------------------------
# Chrome: `progettazione-hud.md` §32. Semantica: `02-color-system.md` §2 (base Okabe-Ito).
# Nessun valore inventato qui: se un colore serve e non c'e', si aggiunge prima al documento owner.

CHROME = {
    "BG_Deep": "#080F14",
    "BG_Panel": "#151A23",
    "BG_Raised": "#212733",
    "Frame_Deep": "#203542",
    "Frame_Mid": "#4A5568",
    "Cyan": "#00E0FF",
    "Violet": "#7C5CFF",
    "Amber": "#FFD456",
    "Red": "#FF4D4D",
    "White": "#FFFFFF",
}

SEMANTIC = {
    "Movement": "#009E73",
    "Attack": "#D55E00",
    "Utility": "#0072B2",
    "Hazard": "#E69F00",
    "Reaction": "#CC79A7",
    "Electric": "#F0E442",
    "Defense": "#56B4E9",
    "Disabled": "#6B7280",
}

# Griglia dei glifi: 24 unita', 2 di padding. Tratto 1.8 — regge a 16 px senza chiudersi.
GRID = 24.0
STROKE = 1.8


# --------------------------------------------------------------------------------------------------
# Helper SVG
# --------------------------------------------------------------------------------------------------

def svg_doc(body: str, size: float = GRID, extra: str = "") -> str:
    return (
        f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {_n(size)} {_n(size)}" '
        f'width="{_n(size)}" height="{_n(size)}" fill="none" stroke="#FFFFFF" '
        f'stroke-width="{STROKE}" stroke-linecap="round" stroke-linejoin="round"{extra}>\n'
        f"{body}\n</svg>\n"
    )


def svg_rect_doc(body: str, w: float, h: float, extra: str = "") -> str:
    return (
        f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {_n(w)} {_n(h)}" '
        f'width="{_n(w)}" height="{_n(h)}" fill="none" stroke="#FFFFFF" '
        f'stroke-width="{STROKE}" stroke-linecap="round" stroke-linejoin="round"{extra}>\n'
        f"{body}\n</svg>\n"
    )


def _n(v: float) -> str:
    """Numero compatto: evita `12.000000001` nei diff."""
    return f"{v:.3f}".rstrip("0").rstrip(".") if isinstance(v, float) else str(v)


def ghost(body: str, opacity: float = 0.55) -> str:
    """Opacita' ghost. `05-certainty-states.md` §2 la prescrive per `Predicted`, ed e' un canale a se':
    il tratteggio dice «previsto», il ghost dice «meno saldo». Sopravvive alla tinta, perche' UMG
    moltiplica: un master al 55% tinto di verde resta un verde al 55%."""
    return f'  <g stroke-opacity="{opacity}" fill-opacity="{opacity}">\n{body}\n  </g>'


def path(d: str, **kw) -> str:
    attrs = "".join(f' {k.replace("_", "-")}="{v}"' for k, v in kw.items())
    return f'  <path d="{d}"{attrs}/>'


def circle(cx: float, cy: float, r: float, **kw) -> str:
    attrs = "".join(f' {k.replace("_", "-")}="{v}"' for k, v in kw.items())
    return f'  <circle cx="{_n(cx)}" cy="{_n(cy)}" r="{_n(r)}"{attrs}/>'


def dot(cx: float, cy: float, r: float = 1.15) -> str:
    return circle(cx, cy, r, fill="#FFFFFF", stroke="none")


def line(x1: float, y1: float, x2: float, y2: float, **kw) -> str:
    attrs = "".join(f' {k.replace("_", "-")}="{v}"' for k, v in kw.items())
    return (
        f'  <line x1="{_n(x1)}" y1="{_n(y1)}" x2="{_n(x2)}" y2="{_n(y2)}"{attrs}/>'
    )


def chevron(x: float, y: float, w: float, h: float) -> str:
    return path(f"M{_n(x)} {_n(y - h)} L{_n(x + w)} {_n(y)} L{_n(x)} {_n(y + h)}")


def arrow_head(x: float, y: float, size: float = 3.0) -> str:
    return path(f"M{_n(x - size)} {_n(y - size)} L{_n(x)} {_n(y)} L{_n(x - size)} {_n(y + size)}")


def arc_deg(cx: float, cy: float, r: float, a0: float, a1: float, **kw) -> str:
    """Arco per angoli in gradi. `y` cresce verso il basso: -90 e' in alto."""
    x0, y0 = cx + r * math.cos(math.radians(a0)), cy + r * math.sin(math.radians(a0))
    x1, y1 = cx + r * math.cos(math.radians(a1)), cy + r * math.sin(math.radians(a1))
    large = 1 if abs(a1 - a0) > 180 else 0
    sweep = 1 if a1 > a0 else 0
    return path(f"M{_n(x0)} {_n(y0)} A {_n(r)} {_n(r)} 0 {large} {sweep} {_n(x1)} {_n(y1)}", **kw)


def rounded_rect(x: float, y: float, w: float, h: float, r: float, **kw) -> str:
    return path(
        f"M{_n(x + r)} {_n(y)} L{_n(x + w - r)} {_n(y)} Q{_n(x + w)} {_n(y)} {_n(x + w)} {_n(y + r)} "
        f"L{_n(x + w)} {_n(y + h - r)} Q{_n(x + w)} {_n(y + h)} {_n(x + w - r)} {_n(y + h)} "
        f"L{_n(x + r)} {_n(y + h)} Q{_n(x)} {_n(y + h)} {_n(x)} {_n(y + h - r)} "
        f"L{_n(x)} {_n(y + r)} Q{_n(x)} {_n(y)} {_n(x + r)} {_n(y)} Z", **kw)


def arrow_head_left(x: float, y: float, size: float = 3.0) -> str:
    return path(f"M{_n(x + size)} {_n(y - size)} L{_n(x)} {_n(y)} L{_n(x + size)} {_n(y + size)}")


def waves(y: float, x0: float = 4.4, span: float = 15.2, amp: float = 1.9, **kw) -> str:
    """Superficie d'acqua. `Water` non e' una goccia: e' una superficie, e si legge dal profilo."""
    q = span / 4.0
    return path(f"M{_n(x0)} {_n(y)} q{_n(q)} {_n(-amp)} {_n(2 * q)} 0 t{_n(2 * q)} 0", **kw)


def polygon(points: list[tuple[float, float]], **kw) -> str:
    pts = " ".join(f"{_n(px)},{_n(py)}" for px, py in points)
    attrs = "".join(f' {k.replace("_", "-")}="{v}"' for k, v in kw.items())
    return f'  <polygon points="{pts}"{attrs}/>'


def hexagon(cx: float, cy: float, r: float, flat_top: bool = True) -> list[tuple[float, float]]:
    offset = 0.0 if flat_top else math.pi / 6.0
    return [
        (cx + r * math.cos(offset + i * math.pi / 3.0), cy + r * math.sin(offset + i * math.pi / 3.0))
        for i in range(6)
    ]


def chamfer_rect(x: float, y: float, w: float, h: float, cut: float,
                 corners: str = "tl,br") -> str:
    """Rettangolo con angoli tagliati. E' la firma di forma del mock: taglio, non raggio."""
    active = set(c.strip() for c in corners.split(","))
    tl = cut if "tl" in active else 0.0
    tr = cut if "tr" in active else 0.0
    br = cut if "br" in active else 0.0
    bl = cut if "bl" in active else 0.0
    pts = [
        (x + tl, y), (x + w - tr, y),
        (x + w, y + tr), (x + w, y + h - br),
        (x + w - br, y + h), (x + bl, y + h),
        (x, y + h - bl), (x, y + tl),
    ]
    d = " ".join(f"{'M' if i == 0 else 'L'}{_n(px)} {_n(py)}" for i, (px, py) in enumerate(pts))
    return path(d + " Z")


def corner_brackets(x: float, y: float, w: float, h: float, arm: float) -> str:
    """Le quattro squadrette agli angoli: il cue di 'pannello tattico' del mock."""
    parts = [
        f"M{_n(x)} {_n(y + arm)} L{_n(x)} {_n(y)} L{_n(x + arm)} {_n(y)}",
        f"M{_n(x + w - arm)} {_n(y)} L{_n(x + w)} {_n(y)} L{_n(x + w)} {_n(y + arm)}",
        f"M{_n(x + w)} {_n(y + h - arm)} L{_n(x + w)} {_n(y + h)} L{_n(x + w - arm)} {_n(y + h)}",
        f"M{_n(x + arm)} {_n(y + h)} L{_n(x)} {_n(y + h)} L{_n(x)} {_n(y + h - arm)}",
    ]
    return "\n".join(path(d) for d in parts)


# --------------------------------------------------------------------------------------------------
# Glifi — asse grammatica (`03-forme-e-primitive.md`)
# --------------------------------------------------------------------------------------------------
# Ogni funzione restituisce il corpo di un SVG 24x24. Il commento sopra ciascuna dice QUALE primitiva
# la definisce: e' il campo che rende falsificabile «questa icona e' sbagliata».


def g_move() -> str:
    """`Move` = `●──•──•──►`. I **nodi intermedi** sono cio' che la definisce (§5)."""
    return "\n".join([
        path("M4 18 L8.4 18 L12.4 12 L17.4 12"),
        dot(4, 18, 1.5), dot(8.4, 18), dot(12.4, 12),
        arrow_head(20.4, 12, 2.9),
    ])


def g_sprint() -> str:
    """`Sprint`: stessa famiglia di Move, stride lungo, doppia trail, **endpoint che chiude**.

    Non e' «Move piu' veloce»: consuma entrambi gli slot e nega la reazione del turno (§5), quindi la
    barra terminale e' semantica, non decorazione. Nessun nodo intermedio: sono di Move.
    """
    return "\n".join([
        path("M3.2 12.2 L15.6 12.2"),
        path("M5.4 8 L13 8", stroke_width=1.3),
        path("M5.4 16.4 L13 16.4", stroke_width=1.3),
        arrow_head(18.6, 12.2, 2.7),
        path("M21.2 7.4 L21.2 17"),
    ])


def g_dash() -> str:
    """`Dash` = `●──»──»──►`. A 16 px collassa in `● » ►` e regge (§5)."""
    return "\n".join([
        dot(3.6, 12, 1.5),
        chevron(6.6, 12, 3.1, 3.3),
        chevron(11.2, 12, 3.1, 3.3),
        arrow_head(20.4, 12, 3.1),
    ])


def g_basic_attack() -> str:
    """`BasicAttack`: reticolo + impatto. Deve restare distinto da `Overwatch` (occhio + settore, §7)."""
    return "\n".join([
        circle(12, 12, 6.2),
        line(12, 2.6, 12, 5.2), line(12, 18.8, 12, 21.4),
        line(2.6, 12, 5.2, 12), line(18.8, 12, 21.4, 12),
        dot(12, 12, 1.5),
        path("M13.6 10.4 L15.8 8.2 M10.2 13.4 L8.2 15.4 M13.2 13.6 L14.8 15.4",
             stroke_width=1.3),
    ])


def g_guard() -> str:
    """`Guard`: postura + **arco frontale** + facing.

    L'arco decade fuori dal settore frontale (ADR-0005 §4a): i due terminali dicono dove finisce, ed
    e' esattamente cio' che due archi concentrici non comunicano — quelli leggono come onde, e allora
    `Guard` e `ArcPulse` diventano lo stesso disegno. Non riusa il glifo di `Cover` (§4.1).
    """
    return "\n".join([
        path("M4.8 14.2 A 8.4 8.4 0 0 1 19.2 14.2"),
        path("M4.8 14.2 L7 15.5 M19.2 14.2 L17 15.5", stroke_width=1.4),
        dot(12, 17.8, 1.7),
        path("M8.6 21.4 L15.4 21.4", stroke_width=1.4),
    ])


def g_brace() -> str:
    """`Brace`: body/anchor + **cuneo di contrasto**. Non riusa il glifo di `Shield` (§4.1)."""
    return "\n".join([
        path("M9.8 1.6 L12 3.8 L14.2 1.6", stroke_width=1.3),
        path("M12 6.2 L6.8 12 L17.2 12 Z"),
        dot(12, 15.6, 1.7),
        path("M5.8 19.4 L18.2 19.4"),
        path("M9.2 19.4 L7.6 21.9 M14.8 19.4 L16.4 21.9", stroke_width=1.4),
    ])


def g_interact() -> str:
    """`Interact`: `Object` (blocco + anchor) ingaggiato da fuori. Distinto dall'interazione con
    obiettivo, che e' `Objective` e ha un glifo suo (§7)."""
    return "\n".join([
        chamfer_rect(12.4, 7.6, 8.4, 10.4, 2.6, "tl"),
        path("M11.6 20.4 L21.6 20.4", stroke_width=1.4),
        path("M2.8 12.8 L7.6 12.8"),
        arrow_head(10.6, 12.8, 2.5),
    ])


def g_wait() -> str:
    """`Wait`: tempo trattenuto. La collisione da evitare e' con `Hold` (§7), che e' la risposta di
    default alla Fast Reaction e non un'azione pianificata."""
    return "\n".join([
        path("M7.6 4.4 L16.4 4.4 L12 12 L16.4 19.6 L7.6 19.6 L12 12 Z"),
        path("M6.4 4.4 L17.6 4.4 M6.4 19.6 L17.6 19.6"),
    ])


def g_overwatch() -> str:
    """`Overwatch`: occhio + **settore**. Il cono non e' un parametro: e' il facing dell'unita'
    (ADR-0005 §4c), quindi il glifo mostra un settore, mai una direzione dichiarata."""
    return "\n".join([
        path("M5.8 7.8 Q12 3.2 18.2 7.8 Q12 12.4 5.8 7.8 Z"),
        dot(12, 7.8, 1.5),
        path("M12 10.8 L4.4 19.6 M12 10.8 L19.6 19.6", stroke_width=1.4),
        path("M4.4 19.6 Q12 22.6 19.6 19.6", stroke_width=1.3),
    ])


# --- Kit di Gadget (`Hero.Gadget.*`, i cinque token che il roster dichiara davvero) ---------------

def g_arc_pulse() -> str:
    """`ArcPulse`: origine + impulsi ad arco. Non e' `Chain`: nessun nodo di salto."""
    return "\n".join([
        dot(6, 18, 1.6),
        path("M6 12.6 A 5.4 5.4 0 0 1 11.4 18"),
        path("M6 9 A 9 9 0 0 1 15 18"),
        path("M6 5.4 A 12.6 12.6 0 0 1 18.6 18", stroke_width=1.4),
    ])


def g_linear_discharge() -> str:
    """`LinearDischarge`: grammatica `Line` (origine, segmento, punta) con payload `Electric`.

    ⚠️ Nel mock questa e' «Chain Discharge» con tre salti. `Chain` e' un'altra primitiva (§3) e va
    disegnata con i nodi visibili: qui non ce ne sono, perche' qui non ci sono salti.

    ⚠️ E non e' `LineAttack` con una piega: la prima stesura — punto, segmento dritto, piccola
    zigzagatura, punta — misurava **0.087** contro di lei. Il fulmine ora occupa tutta la larghezza e
    non ha punta, perche' una scarica non punta: attraversa. La linea sotto e' la superficie.
    """
    return "\n".join([
        dot(3, 12, 1.7),
        path("M5.2 12 L10.4 5.4 L11.8 12.6 L17 6 L18.4 13.2 L21 9.6", stroke_width=2.0),
        path("M13.4 17.6 L16.4 17.6 M9.6 19.8 L18.6 19.8", stroke_width=1.2),
    ])


def g_conductive_node() -> str:
    """`ConductiveNode`: nodo posato + tabs di connessione. E' l'unico glifo del kit che dichiara di
    restare sul campo, e la silhouette esagonale lo lega alla cella."""
    return "\n".join([
        polygon(hexagon(12, 12.6, 5.2)),
        dot(12, 12.6, 1.7),
        path("M12 7.4 L12 3.4", stroke_width=1.4),
        path("M16.5 15.2 L19.9 17.2", stroke_width=1.4),
        path("M7.5 15.2 L4.1 17.2", stroke_width=1.4),
    ])


def g_reactive_capacitor() -> str:
    """`ReactiveCapacitor`: carica immagazzinata + ciclo di reazione che rientra."""
    return "\n".join([
        path("M6.6 9.6 L17.4 9.6"),
        path("M6.6 13.4 L17.4 13.4"),
        path("M12 4.2 L12 9.6", stroke_width=1.4),
        path("M12 13.4 L9.8 16.8 L13.8 16.8 L11.2 21", stroke_width=1.5),
    ])


def g_overload() -> str:
    """`Overload`: contenimento che cede + nucleo. Grammatica `Damage`: **quattro** diramazioni al
    massimo (§4), altrimenti diventa un'esplosione generica e smette di dire chi l'ha causata."""
    return "\n".join([
        dot(12, 12, 2.3),
        circle(12, 12, 7.1, stroke_dasharray="6.2 3.4", stroke_width=1.6),
    ] + [
        # L'anello e' spezzato (tratteggio) e l'energia esce: quattro diramazioni al massimo (§4).
        # Due archi contrapposti no: formavano una lente, e una lente e' un occhio — cioe' `Overwatch`.
        path(f"M{_n(12 + 8.2 * math.cos(math.radians(a)))} "
             f"{_n(12 + 8.2 * math.sin(math.radians(a)))} "
             f"L{_n(12 + 11 * math.cos(math.radians(a)))} "
             f"{_n(12 + 11 * math.sin(math.radians(a)))}", stroke_width=1.4)
        for a in (-135, -45, 45, 135)
    ])


# --- Fasi ------------------------------------------------------------------------------------------
# `Phase.Dash` e `Action.Dash` sono due chiavi diverse e non possono avere la stessa silhouette: una
# dice «in che fase siamo», l'altra «cosa sto per fare». Il differenziatore e' il **chip di fase** —
# il contenitore esagonale — non il glifo interno. Un widget che mostra un glifo di fase senza chip
# sta dicendo la cosa sbagliata.

PHASE_CHIP = [(5.4, 3.6), (18.6, 3.6), (22, 12), (18.6, 20.4), (5.4, 20.4), (2, 12)]


def _chip(inner: str) -> str:
    return "\n".join([polygon(PHASE_CHIP, stroke_width=1.4), inner])


def g_phase_prep() -> str:
    """`Prep`: si arma, non si esegue. Freccia che sale dentro il chip."""
    return _chip("\n".join([
        path("M12 16.6 L12 8.6"),
        path("M9.2 11.4 L12 8.6 L14.8 11.4"),
    ]))


def g_phase_dash() -> str:
    """`Dash` come fase: doppio chevron, coerente con `Action.Dash` ma dentro il chip."""
    return _chip("\n".join([
        chevron(8.4, 12, 2.6, 3.0),
        chevron(12.2, 12, 2.6, 3.0),
    ]))


def g_phase_blast() -> str:
    """`Blast`: risoluzione radiale. Non e' `Overload`: nessun contenimento che cede, nessun nucleo."""
    rays = []
    for i in range(6):
        a = i * math.pi / 3.0
        rays.append(path(
            f"M{_n(12 + 4.2 * math.cos(a))} {_n(12 + 4.2 * math.sin(a))} "
            f"L{_n(12 + 7.2 * math.cos(a))} {_n(12 + 7.2 * math.sin(a))}",
            stroke_width=1.4))
    return _chip("\n".join([circle(12, 12, 2.4, stroke_width=1.4)] + rays))


def g_phase_move() -> str:
    """`Move` come fase: percorso con nodi, dentro il chip."""
    return _chip("\n".join([
        path("M6.6 15.4 L10 15.4 L13.6 10.4 L17.4 10.4", stroke_width=1.5),
        dot(6.6, 15.4, 1.2), dot(10, 15.4, 1.0), dot(17.4, 10.4, 1.2),
    ]))


# --- Certezza --------------------------------------------------------------------------------------
# Tre chiavi richieste da `URTIconLibrary::RequiredIconIds()`. La distinzione **non** e' la tinta:
# tratto solido / tratteggiato / puntinato, piu' una marca di forma diversa (`05-certainty-states.md`
# §2). Il `?` di `Uncertain` e' un overlay del widget, non un pezzo del glifo: qui la marca e'
# l'**area**, perche' se la posizione e' incerta si mostra un'area e non una cella esatta (§2.1).


def g_certainty_confirmed() -> str:
    """Tratto solido, fill normale, opacita' piena. E' il termine di paragone degli altri due: le tre
    chiavi sono la LEGENDA di modificatori che altrove si applicano come stile, e una legenda deve
    mostrare la resa vera — tratto, riempimento e opacita' insieme."""
    return "\n".join([
        path("M3.4 8.6 L20.6 8.6"),
        dot(12, 16.4, 2.6),
    ])


def g_certainty_predicted() -> str:
    """Tratteggio, fill hollow, **e il micro-marker di intento** che `05-certainty-states.md` §2
    dichiara opzionale.

    ⚠️ Qui l'opzionale diventa obbligatorio, e la ragione e' misurata: senza, il glifo dista **0.089**
    da `Confirmed` — vicino per forma E per peso, cioe' non separabile. Il tratteggio da solo non
    basta perche' il cerchio porta quasi tutto l'inchiostro, e a 24 px la differenza fra un disco
    pieno e un anello si assottiglia. Il chevron dice «va da qualche parte», che e' esattamente cio'
    che un intento previsto e' e uno confermato non e'.
    """
    return ghost("\n".join([
        path("M3.4 8.6 L20.6 8.6", stroke_dasharray="4 3"),
        circle(11, 16.4, 2.6, stroke_width=1.5),
        path("M16 13.8 L18.8 16.4 L16 19", stroke_width=1.6),
    ]))


def g_certainty_uncertain() -> str:
    return ghost("\n".join([
        path("M3.4 8.6 L20.6 8.6", stroke_dasharray="0.1 3.4"),
        path("M6 16.4 A 6 3.4 0 1 0 18 16.4 A 6 3.4 0 1 0 6 16.4",
             stroke_dasharray="0.1 3", stroke_width=1.6),
    ]), opacity=0.38)


# --------------------------------------------------------------------------------------------------
# Chrome — cornici 9-slice (`progettazione-hud.md` §41.1, §43)
# --------------------------------------------------------------------------------------------------
# Sono le forme che il mock porta e che vale la pena tenere: taglio d'angolo (non raggio), squadrette,
# testa esagonale. Sono cornici **vuote**: nessun fill composito, nessun gradiente cotto, nessun testo.
# Il riempimento e la tinta li mette UMG, ed e' cio' che permette a un solo asset di coprire tutti gli
# stati di `07-export-e-naming.md` §4.


def f_panel() -> str:
    body = "\n".join([
        chamfer_rect(2, 2, 92, 92, 16, "tl,br"),
        corner_brackets(7, 7, 82, 82, 10),
        path("M2 18 L2 76", stroke_width=1.2),
        path("M94 20 L94 78", stroke_width=1.2),
    ])
    return svg_rect_doc(body, 96, 96)


def f_slot(selected: bool = False) -> str:
    parts = [
        chamfer_rect(3, 3, 58, 58, 11, "tl,br"),
        path("M23 3 L32 3 L36 8 L28 8 Z", stroke_width=1.3),  # notch superiore: pip di stato
    ]
    if selected:
        parts += [
            chamfer_rect(7, 7, 50, 50, 8, "tl,br"),
            path("M3 22 L3 42 M61 22 L61 42", stroke_width=2.2),
        ]
    return svg_rect_doc("\n".join(parts), 64, 64)


def f_button_primary() -> str:
    """La forma del `READY` del mock: testa esagonale su entrambi i lati. Vuota: il testo e'
    dinamico e localizzato, e un bottone con la parola cotta dentro non si traduce."""
    body = "\n".join([
        path("M2 22 L14 4 L146 4 L158 22 L146 40 L14 40 Z"),
        # I due chevron stanno DENTRO i margini fissi del 9-slice (24 px per lato): un accento che
        # cade nella fascia centrale si deforma quando il bottone si allarga.
        path("M12 14 L20 22 L12 30", stroke_width=1.6),
        path("M148 14 L140 22 L148 30", stroke_width=1.6),
    ])
    return svg_rect_doc(body, 160, 44)


def f_button_secondary() -> str:
    body = "\n".join([
        path("M2 18 L12 3 L118 3 L126 18 L118 33 L12 33 Z", stroke_width=1.5),
    ])
    return svg_rect_doc(body, 128, 36)


def f_portrait_hex() -> str:
    body = "\n".join([
        polygon(hexagon(48, 48, 44, flat_top=False)),
        polygon(hexagon(48, 48, 38, flat_top=False), stroke_width=1.2),
        path("M48 4 L48 12 M48 84 L48 92", stroke_width=2.0),
    ])
    return svg_rect_doc(body, 96, 96)


def f_timeline_node() -> str:
    body = "\n".join([
        polygon(hexagon(16, 16, 12)),
        dot(16, 16, 3.0),
    ])
    return svg_rect_doc(body, 32, 32)


def f_bar_frame() -> str:
    body = "\n".join([
        path("M2 2 L126 2 L126 12 L2 12 Z", stroke_width=1.4),
        path("M6 2 L6 12 M122 2 L122 12", stroke_width=1.2),
    ])
    return svg_rect_doc(body, 128, 14)


def f_keybind_chip() -> str:
    """Chip vuoto. Il tasto e' rebindabile: rasterizzarlo produce una texture per ogni tasto."""
    body = chamfer_rect(2, 2, 24, 18, 4, "tl,br")
    return svg_rect_doc(body, 28, 22)


# --------------------------------------------------------------------------------------------------
# Le 45 chiavi che mancavano
# --------------------------------------------------------------------------------------------------
# Fin qui il set copriva i riquadri del mock. `RequiredIconIds()` ne pretende 61, e il mock ne toccava
# 16: il resto non e' «extra», e' il debito che il mock nascondeva mostrando una hotbar di cinque slot
# dove il gioco ha 37 azioni generiche, 11 stati e un roster.
#
# Due regole di famiglia, decise qui una volta:
#
# 1. **Azione contro stato.** `Action.Root` e `Status.Root` sono chiavi diverse e devono leggersi
#    diverse: l'azione ha un cue di bersaglio (si fa a qualcuno), lo stato ha il punto unita' (si e').
#    Vale anche per `Slow`, e per la coppia `Guard`/`Status.Guarded`, `Brace`/`Status.Braced`.
# 2. **Base condivisa degli stati.** Ogni `Status.*` porta il punto unita' in basso al centro. E' cio'
#    che rende la famiglia riconoscibile prima ancora del singolo stato — a 16 px il giocatore vede
#    «e' uno stato su un'unita'» anche quando non distingue quale.

STATUS_UNIT_Y = 19.6


def _status(mark: str) -> str:
    return "\n".join([dot(12, STATUS_UNIT_Y, 1.7), mark])


# --- Movimento ------------------------------------------------------------------------------------

def g_charge() -> str:
    """`Charge`: movimento che finisce addosso. Move family + `Damage` all'arrivo."""
    return "\n".join([
        dot(3.4, 12, 1.4),
        chevron(6, 12, 2.6, 3.0),
        chevron(9.6, 12, 2.6, 3.0),
        path("M19.4 5.4 L19.4 18.6", stroke_width=2.4),
        path("M15.4 8.2 L17.8 10.4 M14.8 12 L17.6 12 M15.4 15.8 L17.8 13.6", stroke_width=1.5),
    ])


def g_leap() -> str:
    """`Leap`: arco balistico. Origine e destinazione sono due celle, il mezzo non e' percorribile."""
    return "\n".join([
        dot(4.2, 18.4, 1.6),
        path("M4.2 18.4 Q12 1.6 19.8 18.4"),
        path("M16.7 16.1 L19.8 18.4 L20.1 14.5", stroke_width=1.6),
        path("M2.4 21.2 L7.4 21.2 M16.6 21.2 L21.6 21.2", stroke_width=1.3),
    ])


def g_reposition() -> str:
    """`Reposition`: freccia breve origine -> destinazione. Distinta dal path di `Move`: nessun nodo
    intermedio, perche' non c'e' un percorso da leggere."""
    return "\n".join([
        circle(5.4, 16.6, 2.6, stroke_width=1.4),
        path("M7.8 14.4 L13.4 9.6"),
        path("M12.6 12.4 L15.4 9.6 L12.6 7.2", stroke_width=1.5),
        polygon([(18.4, 4), (21.6, 7.2), (18.4, 10.4), (15.2, 7.2)], stroke_width=1.4),
    ])


def g_evade() -> str:
    """`Evade`: scarto laterale. La posizione vecchia resta come traccia — se sparisse, il glifo direbbe
    «mi sposto», che e' `Move`."""
    return "\n".join([
        circle(6.4, 15.6, 2.4, stroke_dasharray="1.6 1.8", stroke_width=1.4),
        path("M9.4 14 Q13 12 14.6 8.2"),
        dot(16.4, 6.6, 2.2),
        path("M4.2 20.4 L19.8 20.4", stroke_width=1.3),
    ])


# --- Offesa ---------------------------------------------------------------------------------------

def g_precision_attack() -> str:
    """`PrecisionAttack`: mira stretta, un colpo solo. Contro `BasicAttack` cambia il diametro e sparisce
    la crepa: la precisione non e' un impatto piu' grande."""
    return "\n".join([
        circle(12, 12, 4.4, stroke_width=1.5),
        dot(12, 12, 1.4),
        path("M12 2.8 L12 6.2 M12 17.8 L12 21.2 M2.8 12 L6.2 12 M17.8 12 L21.2 12",
             stroke_width=1.3),
    ])


def g_heavy_attack() -> str:
    """`HeavyAttack`: impatto pesante. La grammatica `Damage` concede 3-4 diramazioni: qui sono quattro,
    e la massa la porta il tratto, non il numero."""
    return "\n".join([
        dot(12, 12, 2.6),
        path("M12 9 L11 3.4 M15 11 L20.8 9.4 M13 15 L15.6 20.4 M9 13.4 L3.4 15.4",
             stroke_width=2.2),
    ])


def g_line_attack() -> str:
    """`LineAttack`: primitiva `Line` — origine, segmento, punta — con le due celle attraversate.

    **Senza nodi intermedi**: i nodi sono di `Move`, e una linea con i nodi diventa un percorso. Le
    due traverse non sono nodi: sono i bersagli sulla traiettoria, ed esistono perche' senza di loro
    il glifo misurava **0.087** contro `Hero.Gadget.LinearDischarge`, che e' anch'essa origine,
    segmento e punta. Due azioni che colpiscono in modo diverso lungo una linea devono dirlo.
    """
    return "\n".join([
        dot(3.4, 12, 1.8),
        path("M5.6 12 L17.4 12", stroke_width=2.0),
        arrow_head(20.6, 12, 3.0),
        path("M9.6 8.6 L9.6 15.4 M14.2 8.6 L14.2 15.4", stroke_width=1.3),
    ])


def g_circular_aoe() -> str:
    """`CircularAoE`: primitiva `Circle` — anello esterno + punto centrale, niente altro. I tick esterni
    li ha `BasicAttack`, e metterli qui farebbe due reticoli."""
    return "\n".join([
        circle(12, 12, 7.6),
        dot(12, 12, 2.0),
    ])


def g_suppressive_line() -> str:
    """`SuppressiveLine`: negazione d'area lungo una linea — una fascia tratteggiata trasversalmente,
    non un fascio di traiettorie.

    ⚠️ Tre linee parallele con una barra terminale misuravano **0.115** contro `Sprint`, che e' una
    linea con due trail e una barra terminale. Il tratteggio trasversale toglie la direzione di
    marcia, che e' esattamente cio' che una zona negata non ha: non ci si passa, non ci si corre.
    """
    return "\n".join([
        dot(3.4, 12, 1.7),
        path("M5.4 7.4 L20.6 7.4 M5.4 16.6 L20.6 16.6", stroke_width=1.6),
        path("M7.6 7.4 L7.6 16.6 M11.4 7.4 L11.4 16.6 M15.2 7.4 L15.2 16.6 M19 7.4 L19 16.6",
             stroke_width=1.2),
    ])


def g_mark_target() -> str:
    """`MarkTarget`: si segna, non si colpisce. Il gancio e' la bandierina — un reticolo da solo sarebbe
    `PrecisionAttack`."""
    return "\n".join([
        circle(12, 15, 4.8, stroke_width=1.5),
        dot(12, 15, 1.5),
        path("M8.4 4.4 L12 8 L15.6 4.4", stroke_width=1.8),
        path("M12 8 L12 9.6", stroke_width=1.4),
    ])


# --- Reazione e difesa ----------------------------------------------------------------------------

def g_counter() -> str:
    """`Counter`: cio' che arriva torna indietro. Due frecce opposte, nessuna superficie: la superficie
    e' di `Deflect`, ed e' quello che le tiene distinte."""
    return "\n".join([
        path("M3.4 8.2 L13.6 8.2"),
        arrow_head(16.6, 8.2, 2.5),
        path("M20.6 15.8 L10.4 15.8"),
        arrow_head_left(7.4, 15.8, 2.5),
    ])


def g_deflect() -> str:
    """`Deflect`: la superficie inclinata devia. L'angolo e' il glifo: senza, e' `Counter`."""
    return "\n".join([
        path("M6.6 18.6 L18.6 6.6", stroke_width=2.4),
        path("M2.8 5.4 L8.6 10"),
        path("M5.2 10.4 L9.6 10.8 L9.2 6.4", stroke_width=1.5),
        path("M13.4 15.6 L19.4 20.4"),
        path("M16 20.8 L20.4 21.2 L20 16.8", stroke_width=1.5),
    ])


def g_intercept() -> str:
    """`Intercept`: due traiettorie che si incontrano. Chi intercetta **va incontro**.

    ⚠️ La prima stesura era una linea spezzata da una barra verticale, e il test di collisione la
    misurava contro `Interrupt` a **0.085**: lo stesso disegno con la barra in un altro punto. Sono
    due cose diverse — `Interrupt` spezza un'azione gia' partita, `Intercept` ne incrocia una — e
    finche' condividevano la silhouette il giocatore non aveva modo di saperlo.
    """
    return "\n".join([
        dot(3, 7.6, 1.5),
        path("M4.8 8.4 L10.6 11.6"),
        dot(3, 20.4, 1.5),
        path("M4.8 19.6 L10.6 16.4"),
        path("M13.6 14 L16.6 14 M13.6 14 L13.6 11 M15 15.4 L18.4 18.8 M15 12.6 L18.4 9.2",
             stroke_width=1.5),
        dot(12.8, 14, 2.2),
    ])


def g_anchor() -> str:
    """`Anchor`: si radica per non essere spostati. E' l'inverso di `Push`/`Pull`, e il cue e' il
    terreno che tiene, non il corpo."""
    return "\n".join([
        dot(12, 7.4, 2.0),
        path("M12 9.6 L12 17.4"),
        path("M6.6 17.4 L17.4 17.4"),
        path("M8.6 17.4 L6.2 21.4 M15.4 17.4 L17.8 21.4", stroke_width=1.4),
    ])


def g_shield() -> str:
    """`Shield`: risorsa che assorbe. Scudo geometrico con **ampio negative space** — non e' `Cover`
    (barriera ancorata) e non e' `Brace` (cuneo di contrasto)."""
    return "\n".join([
        path("M12 3.4 L19.6 6.6 L19.6 12.6 Q19.6 18.6 12 21.4 Q4.4 18.6 4.4 12.6 L4.4 6.6 Z"),
    ])


def g_evade_placeholder() -> str:  # pragma: no cover - segnaposto non registrato
    return g_evade()


# --- Controllo ------------------------------------------------------------------------------------

def g_push() -> str:
    """`Push`: impulso esterno, punto unita', uscita. E' movimento **subito**, e la grammatica §7 lo
    vuole distinto da `Dash`, che l'unita' sceglie.

    ⚠️ La prima stesura usava i chevron — `» ● ─►`, come scrive §4 — e il test di collisione misurava
    `Push` contro `Dash` a **0.113**: indistinguibili a 24 px. I chevron sono il segno di `Dash`, e
    riusarli per l'impulso esterno collassava proprio la coppia che il documento dichiara critica. La
    faccia piena dice «qualcosa mi ha spinto» senza contendere il segno dell'accelerazione.
    """
    return "\n".join([
        path("M3.4 5.6 L3.4 18.4", stroke_width=2.6),
        path("M5 8.6 L7.6 12 L5 15.4", stroke_width=1.4),
        dot(12.4, 12, 2.4),
        path("M15.4 12 L18 12"),
        arrow_head(20.8, 12, 2.5),
    ])


def g_pull() -> str:
    """`Pull`: `◄─ ● «` — lo stesso impulso al contrario."""
    return "\n".join([
        path("M20.6 5.6 L20.6 18.4", stroke_width=2.6),
        path("M19 8.6 L16.4 12 L19 15.4", stroke_width=1.4),
        dot(11.6, 12, 2.4),
        path("M8.6 12 L6 12"),
        arrow_head_left(3.2, 12, 2.5),
    ])


def g_root() -> str:
    """`Action.Root`: si radica **qualcuno**. Il cue di bersaglio (la freccia che entra) e' cio' che la
    separa da `Status.Root`, che invece e' lo stato addosso a un'unita'."""
    return "\n".join([
        path("M2.8 3.6 L7.6 8"),
        path("M4.6 8.8 L8.4 8.8 L8.4 5", stroke_width=1.5),
        dot(13.6, 12.4, 2.0),
        path("M7.4 17.4 L20.4 17.4"),
        path("M10.4 17.4 L10.4 20.8 M13.6 17.4 L13.6 20.8 M16.8 17.4 L16.8 20.8", stroke_width=1.4),
    ])


def g_slow() -> str:
    """`Action.Slow`: si rallenta qualcuno. Chevron che perdono passo contro la barra che frena."""
    return "\n".join([
        path("M2.6 6.4 L7 10"),
        arrow_head(9 , 11.6, 2.2),
        chevron(11.6, 15, 2.4, 2.8),
        chevron(15, 15, 1.8, 2.2),
        path("M19.6 11.4 L19.6 18.6", stroke_width=2.0),
    ])


def g_interrupt() -> str:
    """`Interrupt`: la rottura di una linea gia' avviata. Il pezzo dopo la rottura resta, tratteggiato:
    se sparisse, il glifo direbbe «non e' mai partita»."""
    return "\n".join([
        path("M3 12 L9.4 12", stroke_width=1.8),
        path("M11.4 5.6 L14 12 L11.4 18.4", stroke_width=2.0),
        path("M16 12 L21 12", stroke_dasharray="1.4 2.4", stroke_width=1.4),
    ])


def g_modify_arc() -> str:
    """`ModifyArc`: si ruota il settore frontale. L'arco e' quello di `Guard`; a cambiare e' la freccia
    di rotazione, ed e' l'unica differenza ammessa (ADR-0005 §4c: il cono E' il facing)."""
    return "\n".join([
        dot(12, 17.4, 1.7),
        path("M5.4 14.6 A 8 8 0 0 1 18.6 14.6"),
        path("M7 7.4 A 7.6 7.6 0 0 1 17.8 6.6", stroke_width=1.4),
        path("M14.8 4.2 L18.2 6.6 L14.8 9", stroke_width=1.5),
    ])


# --- Supporto -------------------------------------------------------------------------------------

def g_heal() -> str:
    """`Heal`: pulse geometrico. Usato **solo** per Heal, mai per `Ally` (§4)."""
    return "\n".join([
        path("M12 6.4 L12 17.6 M6.4 12 L17.6 12", stroke_width=2.4),
        circle(12, 12, 8.4, stroke_dasharray="2.6 3.2", stroke_width=1.3),
    ])


def g_cleanse() -> str:
    """`Cleanse`: lo stato si stacca e sale. Su un alleato — `Purge` fa la stessa cosa a un avversario,
    e la differenza la porta il segno (qui si solleva, la' si taglia)."""
    return "\n".join([
        dot(12, 19.4, 1.8),
        path("M12 17 L12 10.6"),
        path("M9.2 13.2 L12 10.4 L14.8 13.2"),
        path("M8.4 7.4 L15.6 7.4", stroke_width=1.5),
        path("M9.8 4 L14.2 4", stroke_width=1.3),
    ])


def g_purge() -> str:
    """`Purge`: lo stato si toglie con la forza. Il taglio e' il segno di `Invalid` riusato come
    **azione**, ed e' voluto: dice «questo non vale piu'»."""
    return "\n".join([
        circle(11.4, 11.4, 5.6, stroke_width=1.5),
        path("M8.4 11.4 L14.4 11.4", stroke_width=1.6),
        path("M4.6 19.4 L20.4 4.6", stroke_width=2.2),
    ])


# --- Ambiente -------------------------------------------------------------------------------------

def g_create_cover() -> str:
    """`CreateCover`: barriera ancorata al terreno + segno di creazione. `Cover` e' proprieta' della
    mappa (§4.1): senza il terreno sotto, il glifo direbbe `Shield`."""
    return "\n".join([
        path("M8.6 6.4 L8.6 18.4", stroke_width=2.2),
        path("M4.4 18.4 L13.4 18.4", stroke_width=1.5),
        path("M11.4 9.4 L11.4 15.4", stroke_width=1.2),
        path("M18 6.4 L18 12.4 M15 9.4 L21 9.4", stroke_width=1.6),
    ])


def g_create_water() -> str:
    """`CreateWater`: superficie, non goccia. `Water` non cambia colore in base alla squadra (§2.2)."""
    return "\n".join([
        waves(13.4),
        waves(17.6, stroke_width=1.5),
        path("M17.4 4.4 L17.4 10.4 M14.4 7.4 L20.4 7.4", stroke_width=1.6),
    ])


def g_electrify() -> str:
    """`Electrify`: payload `Electric` su una superficie. Contro `Status.Electrified`: qui c'e' la
    superficie sotto, la' c'e' l'unita'."""
    return "\n".join([
        path("M12 3.4 L8.6 10.4 L13 10.4 L9.6 16.6", stroke_width=1.8),
        path("M3.6 19.6 L20.4 19.6", stroke_width=1.6),
        path("M15 14.6 L18.6 14.6 M16.2 11.4 L19.4 11.4", stroke_width=1.3),
    ])


def g_ignite() -> str:
    """`Ignite`: calore che sale da una superficie. Niente fiamma disegnata — la grammatica `Damage`
    vieta l'icona-oggetto, e una fiamma qui competerebbe con `Status.Burning`."""
    return "\n".join([
        path("M3.6 19.6 L20.4 19.6", stroke_width=1.6),
        path("M8 16.4 Q6 12.4 9 9.4 Q10.4 12.4 12 10.4", stroke_width=1.5),
        path("M14.6 16.4 Q12.4 11.4 16 6.6 Q17.4 11.4 19.4 12.4", stroke_width=1.5),
    ])


# --- Stati ----------------------------------------------------------------------------------------

def g_status_braced() -> str:
    """Lo stato che `Action.Brace` lascia: il cuneo resta, il corpo e' gia' piantato."""
    return _status("\n".join([
        path("M12 4.6 L7.4 11.4 L16.6 11.4 Z"),
        path("M6.6 15.4 L17.4 15.4", stroke_width=1.5),
    ]))


def g_status_guarded() -> str:
    """Lo stato che `Action.Guard` lascia: arco frontale, senza la posa dell'azione."""
    return _status("\n".join([
        arc_deg(12, STATUS_UNIT_Y, 6.2, -168, -12, stroke_width=2.0),
        path("M12 16 L12 12.4", stroke_width=1.3),
    ]))


def g_status_burning() -> str:
    return _status(path("M12 3.4 Q7.4 9.4 9.6 14.4 Q11 11.4 13 12.4 Q16.6 8.4 12 3.4 Z"))


def g_status_electrified() -> str:
    return _status("\n".join([
        path("M13.6 3.4 L8.6 11 L13 11 L9.4 16.6", stroke_width=1.9),
        path("M17 7.4 L19.6 7.4 M16 11.4 L18.6 11.4", stroke_width=1.3),
    ]))


def g_status_wet() -> str:
    return _status("\n".join([
        waves(9.4, x0=4.6, span=14.8),
        waves(13.6, x0=4.6, span=14.8, stroke_width=1.5),
    ]))


def g_status_exposed() -> str:
    """`Exposed`: la copertura non c'e'. Il segno e' la barriera **spezzata**, non un punto esclamativo:
    un warning generico non dice perche'."""
    return _status("\n".join([
        path("M4.6 14.6 L4.6 5.4 L9.6 5.4", stroke_width=1.6),
        path("M19.4 14.6 L19.4 5.4 L14.4 5.4", stroke_width=1.6),
        path("M11 9 L13 12.4 M13 9 L11 12.4", stroke_width=1.5),
    ]))


def g_status_marked() -> str:
    return _status("\n".join([
        circle(12, 10.4, 4.4, stroke_dasharray="2.4 2.6", stroke_width=1.4),
        path("M12 3.4 L12 6 M12 14.8 L12 17.4 M5 10.4 L7.6 10.4 M16.4 10.4 L19 10.4",
             stroke_width=1.3),
    ]))


def g_status_obscured() -> str:
    """`Obscured`: si vede meno, non si vede altro. Il velo copre in parte — se coprisse tutto sarebbe
    «non visibile», che e' un'altra cosa e appartiene a `Information`."""
    return _status("\n".join([
        path("M4.4 8.4 L19.6 8.4", stroke_dasharray="2.8 2.4", stroke_width=1.6),
        path("M4.4 12.4 L19.6 12.4", stroke_dasharray="2.8 2.4", stroke_width=1.6),
        path("M6.4 4.6 L17.6 4.6", stroke_dasharray="2.8 2.4", stroke_width=1.3),
    ]))


def g_status_reveal() -> str:
    """`Reveal`: marca da sensore. Distinta dall'occhio + settore di `Overwatch` (§4): qui non c'e' un
    osservatore, c'e' un rilevamento."""
    return _status("\n".join([
        circle(12, 10.6, 5.6, stroke_width=1.5),
        path("M12 10.6 L16 6.6", stroke_width=1.6),
        dot(15.2, 12.6, 1.5),
        path("M12 3.6 L12 5.4", stroke_width=1.3),
    ]))


def g_status_root() -> str:
    """`Status.Root`: radicato. Nessun cue di bersaglio — chi l'ha applicato non e' parte dello stato."""
    return _status("\n".join([
        dot(12, 9.4, 2.0),
        path("M5.4 15.4 L18.6 15.4"),
        path("M8.4 15.4 L8.4 18.6 M12 15.4 L12 18.6 M15.6 15.4 L15.6 18.6", stroke_width=1.4),
    ]))


def g_status_slow() -> str:
    return _status("\n".join([
        chevron(6.4, 10.4, 2.6, 3.0),
        chevron(10.4, 10.4, 2.0, 2.4),
        chevron(13.8, 10.4, 1.4, 1.8),
        path("M18.4 6.4 L18.4 14.4", stroke_width=2.0),
    ]))


# --- Identita' ------------------------------------------------------------------------------------
# Il badge esagonale a punta in alto e' il contenitore dell'identita', e non e' il chip di fase (che ha
# le punte ai lati). Due contenitori diversi perche' rispondono a due domande diverse: «chi e'» e «in
# che fase siamo». Chi li disegnasse uguali costringerebbe il giocatore a leggere l'interno per sapere
# quale delle due sta guardando.

IDENTITY_BADGE = [(12, 2.4), (20.4, 7.2), (20.4, 16.8), (12, 21.6), (3.6, 16.8), (3.6, 7.2)]


def _identity(inner: str) -> str:
    return "\n".join([polygon(IDENTITY_BADGE, stroke_width=1.4), inner])


def g_identity_gadget() -> str:
    """Gadget: il nodo conduttivo, che e' la cosa che solo lui lascia sul campo."""
    return _identity("\n".join([
        polygon(hexagon(12, 12, 3.6), stroke_width=1.4),
        dot(12, 12, 1.4),
        path("M12 8.4 L12 6.4 M15.1 13.8 L16.8 14.8 M8.9 13.8 L7.2 14.8", stroke_width=1.3),
    ]))


def g_identity_phase() -> str:
    """Phase: la superficie che si muove — il fluido e' la sua materia."""
    return _identity("\n".join([
        waves(11, x0=6.4, span=11.2, amp=1.6, stroke_width=1.5),
        waves(14.6, x0=6.4, span=11.2, amp=1.6, stroke_width=1.3),
    ]))


def g_identity_riktor() -> str:
    """Riktor: massa e pannello cinetico. La base larga e' il punto: e' l'eroe che non si sposta."""
    return _identity("\n".join([
        path("M7.6 15.6 L7.6 8.6 L16.4 8.6 L16.4 15.6", stroke_width=1.6),
        path("M6 15.6 L18 15.6", stroke_width=1.8),
        path("M12 8.6 L12 15.6", stroke_width=1.2),
    ]))


def g_identity_wraith() -> str:
    """Wraith: la lama che passa. Il tratto interrotto dice il transito, che e' la sua firma."""
    return _identity("\n".join([
        path("M7.4 16.6 L16.6 7.4", stroke_width=1.9),
        path("M8.6 8.4 L11 10.8", stroke_dasharray="1.4 1.8", stroke_width=1.3),
        path("M13 13.2 L15.4 15.6", stroke_dasharray="1.4 1.8", stroke_width=1.3),
    ]))


def g_identity_ally() -> str:
    """`Ally`: unit marker **rounded** + connection tabs. Nessun `+` (§2) — la croce e' di `Heal`."""
    return "\n".join([
        rounded_rect(5.6, 5.6, 12.8, 12.8, 4.4, stroke_width=2.0),
        dot(12, 12, 2.0),
        path("M2.6 12 L5.6 12 M18.4 12 L21.4 12", stroke_width=1.8),
    ])


def g_identity_enemy() -> str:
    """`Enemy`: unit marker **angular** + notch. La differenza da `Ally` e' rounded contro angular, non
    la tinta: in monocromia deve reggere lo stesso (§2)."""
    return "\n".join([
        polygon([(12, 5.2), (18.8, 12), (12, 18.8), (5.2, 12)], stroke_width=2.0),
        dot(12, 12, 2.0),
        path("M3.2 12 L5.6 12 M18.4 12 L20.8 12", stroke_width=1.6),
    ])


# --- La cinquantaduesima --------------------------------------------------------------------------

def g_missing_icon() -> str:
    """Non e' una chiave del dizionario: e' il campo `MissingIcon` di `URTIconCatalogData`, e senza di
    lei il catalogo **non passa la validazione**.

    Si vede solo quando qualcosa e' rotto, e per questo non deve somigliare ne' a `Invalid` (slash) ne'
    a `Uncertain` (fade + `?`): deve dire «manca il contenuto», che e' un'altra cosa. Quattro angoli di
    un contenitore vuoto.
    """
    return corner_brackets(4.4, 4.4, 15.2, 15.2, 4.6)


# --------------------------------------------------------------------------------------------------
# Copertura — le chiavi che `RequiredIconIds()` genera davvero
# --------------------------------------------------------------------------------------------------
# La lista **si interroga, non si copia**: un manifest scritto a mano invecchia il giorno in cui
# qualcuno registra un tag nuovo. Qui non gira Unreal, quindi si legge la stessa sorgente che legge
# `URTIconLibrary::RequiredIconIds()` — il catalogo generico, i tag `Status.`, le fasi volontarie, il
# roster — e si confronta con cio' che lo script disegna. Se qualcosa manca, il generatore lo dice.

REPO_ROOT = Path(__file__).resolve().parents[2]

SOURCES = {
    "actions": REPO_ROOT / "Source/RefactorTactics/Ability/RTCatalogLibrary.cpp",
    "status": REPO_ROOT / "Source/RefactorTactics/Core/RTGameplayTags.cpp",
    "heroes": REPO_ROOT / "Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp",
    "library": REPO_ROOT / "Source/RefactorTactics/UI/RTIconLibrary.cpp",
}


def required_icon_ids() -> list[str]:
    """L'immagine testuale di `URTIconLibrary::RequiredIconIds()`.

    E' una lettura del sorgente, non una copia della lista: cambia quando cambia il gioco. Resta un
    surrogato — l'autorita' e' la funzione C++ — e per questo `check_coverage` dice sempre da dove ha
    letto, cosi' un disallineamento si vede invece di restare implicito.
    """
    import re

    ids: list[str] = []

    # Fasi: le quattro volontarie, nell'ordine dichiarato dentro RequiredIconIds.
    lib = SOURCES["library"].read_text(encoding="utf-8")
    phases = re.search(r"const ERTMatchPhase VoluntaryPhases\[\] = \{(.*?)\};", lib, re.S)
    for name in re.findall(r"ERTMatchPhase::(\w+)", phases.group(1) if phases else ""):
        ids.append(f"UI.Icon.Phase.{name}")

    # Azioni: quelle che il catalogo generico SPEDISCE.
    cat = SOURCES["actions"].read_text(encoding="utf-8")
    body = re.search(r"TArray<FRTActionDef> URTCatalogLibrary::GetCoreActionCatalog\(\).*?\n\}",
                     cat, re.S)
    for action in re.findall(r'ShippedAction\(TEXT\("(Action\.\w+)"\)', body.group(0) if body else ""):
        ids.append(f"UI.Icon.{action}")

    # Stati: i tag registrati sotto `Status.`, ordinati come li ordina la funzione.
    tags = SOURCES["status"].read_text(encoding="utf-8")
    ids += sorted({f"UI.Icon.{t}" for t in re.findall(r'"(Status\.\w+)"', tags)})

    # Certezza: i tre livelli DISEGNABILI, elencati a mano nel C++ e non derivati dall'enum (che ne ha
    # quattro: `Unknown` non ha una resa).
    ids += [f"UI.Icon.Certainty.{lvl}" for lvl in ("Confirmed", "Predicted", "Uncertain")]

    # Identita': i quattro eroi (il prefisso `Hero.` diventa `Identity.`) piu' la relazione di squadra.
    roster = SOURCES["heroes"].read_text(encoding="utf-8")
    hero_ids = re.search(r"TArray<FName> URTHeroCatalogLibrary::GetHeroIds\(\).*?\n\}", roster, re.S)
    for hero in re.findall(r'TEXT\("Hero\.(\w+)"\)', hero_ids.group(0) if hero_ids else ""):
        ids.append(f"UI.Icon.Identity.{hero}")
    ids += ["UI.Icon.Identity.Ally", "UI.Icon.Identity.Enemy"]

    # Deduplica conservando l'ordine, come fa `AddUnique`.
    seen, unique = set(), []
    for i in ids:
        if i not in seen:
            seen.add(i)
            unique.append(i)
    return unique


def check_coverage(drawn: set[str]) -> tuple[list[str], list[str]]:
    """Restituisce (mancanti, extra). Gli extra non sono un errore: le ability degli eroi hanno una
    chiave regolare sotto `Action.` ma non sono nel catalogo generico, quindi `RequiredIconIds()` non
    le pretende — servono comunque alla skill bar."""
    required = required_icon_ids()
    missing = [i for i in required if i not in drawn]
    extra = sorted(d for d in drawn if d not in set(required))
    return missing, extra


# --------------------------------------------------------------------------------------------------
# Registro
# --------------------------------------------------------------------------------------------------
# `IconId` e nome asset non sono intercambiabili (`07-export-e-naming.md` §1): il primo lo risolve
# `URTIconLibrary`, il secondo lo vede il content browser. Qui stanno insieme una volta sola, ed e' il
# punto in cui un refuso diventa visibile invece che silenzioso.
#
# `origine_mock` dice da quale riquadro della tavola viene l'elemento e cosa e' cambiato. Un asset
# senza quella colonna non si sa se e' stato verificato o solo ricalcato.

# `IconId` e nome asset non sono intercambiabili (`07-export-e-naming.md` §1): il primo lo risolve
# `URTIconLibrary`, il secondo lo vede il content browser. Qui c'e' **solo la semantica**, e le due
# forme si DERIVANO da essa:
#
#     UI.Icon.Action.Move   <- ICON_ID_PREFIX + semantica
#     RT_UI_Icon_Action_Move <- "RT_" + IconId con i punti in underscore
#
# Derivare invece di elencare non e' pigrizia: e' cio' che permette al commandlet dell'Editor di
# trovare la texture di una chiave **senza una tabella di corrispondenza**, e a un refuso di diventare
# visibile invece che silenzioso.
#
# ⚠️ `progettazione-hud.md` §43 porta esempi come `RT_UI_Phase_Blast`, senza il segmento `Icon`. La
# regola generale di `07-export-e-naming.md` §1 (`RT_UI_Icon_<...>`) vince qui, perche' e' l'unica che
# si deriva: un'eccezione per categoria costringerebbe a una tabella, che e' esattamente cio' che
# D-031 evita.

ICON_ID_PREFIX = "UI.Icon."


def icon_id(semantic: str) -> str:
    return ICON_ID_PREFIX + semantic


def asset_name(semantic: str) -> str:
    return "RT_" + icon_id(semantic).replace(".", "_")


def icon_category(semantic: str) -> str:
    """La categoria e' il primo segmento, e DEVE essere un valore di `ERTIconCategory`:
    `ValidateIconCatalog` confronta la categoria dichiarata con quella dentro l'ID."""
    return semantic.split(".", 1)[0]


ICONS = [
    # (semantica dopo `UI.Icon.`, glifo, tinta suggerita, origine)
    ("Action.Move", g_move, "Movement",
     "mock 05/11 — invariato"),
    ("Action.Sprint", g_sprint, "Movement",
     "mock 05/12 — endpoint che chiude: Sprint nega la reazione"),
    ("Action.Dash", g_dash, "Movement",
     "mock 05/13 — invariato"),
    ("Action.Charge", g_charge, "Movement",
     "assente dal mock"),
    ("Action.Leap", g_leap, "Movement",
     "assente dal mock"),
    ("Action.Reposition", g_reposition, "Movement",
     "assente dal mock"),
    ("Action.Evade", g_evade, "Defense",
     "assente dal mock"),
    ("Action.BasicAttack", g_basic_attack, "Attack",
     "mock 06/14 — invariato"),
    ("Action.PrecisionAttack", g_precision_attack, "Attack",
     "assente dal mock"),
    ("Action.HeavyAttack", g_heavy_attack, "Attack",
     "assente dal mock"),
    ("Action.LineAttack", g_line_attack, "Attack",
     "assente dal mock"),
    ("Action.CircularAoE", g_circular_aoe, "Attack",
     "assente dal mock"),
    ("Action.SuppressiveLine", g_suppressive_line, "Attack",
     "assente dal mock"),
    ("Action.MarkTarget", g_mark_target, "Utility",
     "assente dal mock"),
    ("Action.Guard", g_guard, "Defense",
     "AGGIUNTO — una delle sette generiche (D-025)"),
    ("Action.Brace", g_brace, "Defense",
     "mock 07/19 — spostato da Reaction a Defense"),
    ("Action.Shield", g_shield, "Defense",
     "assente dal mock"),
    ("Action.Anchor", g_anchor, "Defense",
     "assente dal mock"),
    ("Action.Counter", g_counter, "Reaction",
     "assente dal mock"),
    ("Action.Deflect", g_deflect, "Reaction",
     "assente dal mock"),
    ("Action.Intercept", g_intercept, "Reaction",
     "assente dal mock"),
    ("Action.Overwatch", g_overwatch, "Reaction",
     "mock 07/20 — invariato"),
    ("Action.Push", g_push, "Hazard",
     "assente dal mock"),
    ("Action.Pull", g_pull, "Hazard",
     "assente dal mock"),
    ("Action.Root", g_root, "Hazard",
     "assente dal mock"),
    ("Action.Slow", g_slow, "Hazard",
     "assente dal mock"),
    ("Action.Interrupt", g_interrupt, "Hazard",
     "assente dal mock"),
    ("Action.Purge", g_purge, "Hazard",
     "assente dal mock"),
    ("Action.ModifyArc", g_modify_arc, "Utility",
     "assente dal mock"),
    ("Action.Heal", g_heal, "Utility",
     "assente dal mock"),
    ("Action.Cleanse", g_cleanse, "Utility",
     "assente dal mock"),
    ("Action.Interact", g_interact, "Utility",
     "mock 09/22 — la mano diventa Object + ingaggio"),
    ("Action.Wait", g_wait, "Utility",
     "mock 09/23 — invariato"),
    ("Action.CreateCover", g_create_cover, "Defense",
     "assente dal mock"),
    ("Action.CreateWater", g_create_water, "Utility",
     "assente dal mock"),
    ("Action.Electrify", g_electrify, "Electric",
     "assente dal mock"),
    ("Action.Ignite", g_ignite, "Attack",
     "assente dal mock"),
    ("Action.Hero.Gadget.ArcPulse", g_arc_pulse, "Electric",
     "mock 16 — rimappato: kit fuori roster"),
    ("Action.Hero.Gadget.LinearDischarge", g_linear_discharge, "Electric",
     "mock 16 — «Chain Discharge» non esiste: Line, non Chain"),
    ("Action.Hero.Gadget.ConductiveNode", g_conductive_node, "Electric",
     "mock 17 — «Static Field» rimappato"),
    ("Action.Hero.Gadget.ReactiveCapacitor", g_reactive_capacitor, "Electric",
     "AGGIUNTO — quinto token del kit"),
    ("Action.Hero.Gadget.Overload", g_overload, "Hazard",
     "mock 18 — nome corretto, glifo su grammatica Damage"),
    ("Phase.Prep", g_phase_prep, "Utility",
     "mock 03 — sostituisce la timeline a iniziativa"),
    ("Phase.Dash", g_phase_dash, "Movement",
     "mock 03 — idem"),
    ("Phase.Blast", g_phase_blast, "Attack",
     "mock 03 — idem"),
    ("Phase.Move", g_phase_move, "Movement",
     "mock 03 — idem"),
    ("Certainty.Confirmed", g_certainty_confirmed, None,
     "mock 08 — piu' la marca di forma"),
    ("Certainty.Predicted", g_certainty_predicted, None,
     "mock 08 — piu' la marca di forma"),
    ("Certainty.Uncertain", g_certainty_uncertain, None,
     "mock 08 — la marca e' un'area (§2.1)"),
    ("Status.Braced", g_status_braced, "Defense",
     "assente dal mock"),
    ("Status.Burning", g_status_burning, "Attack",
     "assente dal mock"),
    ("Status.Electrified", g_status_electrified, "Electric",
     "assente dal mock"),
    ("Status.Exposed", g_status_exposed, "Hazard",
     "assente dal mock"),
    ("Status.Guarded", g_status_guarded, "Defense",
     "assente dal mock"),
    ("Status.Marked", g_status_marked, "Hazard",
     "assente dal mock"),
    ("Status.Obscured", g_status_obscured, "Utility",
     "assente dal mock"),
    ("Status.Reveal", g_status_reveal, "Utility",
     "assente dal mock"),
    ("Status.Root", g_status_root, "Hazard",
     "assente dal mock"),
    ("Status.Slow", g_status_slow, "Hazard",
     "assente dal mock"),
    ("Status.Wet", g_status_wet, "Utility",
     "assente dal mock"),
    ("Identity.Gadget", g_identity_gadget, "Electric",
     "assente dal mock"),
    ("Identity.Phase", g_identity_phase, "Utility",
     "assente dal mock"),
    ("Identity.Riktor", g_identity_riktor, "Defense",
     "assente dal mock"),
    ("Identity.Wraith", g_identity_wraith, "Reaction",
     "assente dal mock"),
    ("Identity.Ally", g_identity_ally, "Defense",
     "assente dal mock"),
    ("Identity.Enemy", g_identity_enemy, "Hazard",
     "assente dal mock"),
]


FRAMES = [
    # (AssetName, factory, native (w,h), margini 9-slice L,T,R,B (None = non 9-slice), uso, origine)
    ("RT_UI_Panel_Primary_9S", f_panel, (96, 96), (20, 20, 20, 20),
     "pannelli HUD: stato unita', obiettivi, dettaglio abilita'", "01/02/04"),
    ("RT_UI_Slot_Universal_9S", lambda: f_slot(False), (64, 64), (14, 14, 14, 14),
     "slot di azione e di kit, stato base", "06/11-20"),
    ("RT_UI_Slot_Universal_Selected_9S", lambda: f_slot(True), (64, 64), (14, 14, 14, 14),
     "slot selezionato — variante di stato, non un secondo glifo", "06 (Chain Discharge attivo)"),
    ("RT_UI_Button_Primary_9S", f_button_primary, (160, 44), (24, 10, 24, 10),
     "conferma di piano / READY", "09/21"),
    ("RT_UI_Button_Secondary_9S", f_button_secondary, (128, 36), (18, 8, 18, 8),
     "azioni di contesto: Interact, Wait", "09/22/23"),
    ("RT_UI_PortraitFrame_Hex", f_portrait_hex, (96, 96), None,
     "ritratti roster e header di unita'", "01/03"),
    ("RT_UI_Timeline_Node", f_timeline_node, (32, 32), None,
     "nodo di fase sulla banda del turno", "03 (riusato, semantica cambiata)"),
    ("RT_UI_Bar_Frame_9S", f_bar_frame, (128, 14), (8, 4, 8, 4),
     "cornice di barra risorsa — il riempimento e' dinamico, mai cotto", "01"),
    ("RT_UI_Keybind_Chip_9S", f_keybind_chip, (28, 22), (7, 6, 7, 6),
     "chip del tasto — vuoto: il binding e' rimappabile", "05/06/07/09"),
]

# `MissingIcon` non e' una voce del dizionario: e' un campo di `URTIconCatalogData`, e senza di lei il
# catalogo non passa la validazione. Sta fuori da ICONS proprio per questo — se ci fosse dentro,
# `check_coverage` la conterebbe come «extra» e la segnalerebbe ogni volta.
MISSING_ICON_ASSET = "RT_UI_Icon_MissingIcon"

PNG_SIZES = [16, 20, 24, 32, 48]

# Taglia minima leggibile, misurata sulla strip a 16 px e non dedotta: sotto questa soglia il glifo si
# chiude e smette di dire quello che dice. Non si esporta un PNG che non si puo' usare — un asset
# illeggibile in cartella e' un invito a metterlo in uno slot dove non regge.
#
# Il chip di fase e' il caso limite: il contenitore esagonale e' cio' che distingue `Phase.Dash` da
# `Action.Dash`, e a 16 px il contenitore mangia il glifo. Le fasi vivono sulla banda del turno, dove
# lo spazio c'e'.
MIN_READABLE = {"Phase": 24, "Certainty": 20}
DEFAULT_MIN_READABLE = 16


# --------------------------------------------------------------------------------------------------
# Scrittura
# --------------------------------------------------------------------------------------------------

def _write(target: Path, text: str) -> None:
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text, encoding="utf-8")


def _rasterize(svg: str, target: Path, width: int, height: int) -> bool:
    if cairosvg is None:
        return False
    target.parent.mkdir(parents=True, exist_ok=True)
    cairosvg.svg2png(bytestring=svg.encode("utf-8"), write_to=str(target),
                     output_width=width, output_height=height, background_color=None)
    return True


def contact_sheet(grayscale: bool) -> str:
    """Foglio di accettazione. La versione in scala di grigi non e' un extra: e' il test di
    `02-color-system.md` §1 — se un'informazione sparisce senza colore, manca il secondo canale."""
    entries = [(g, t) for _sem, g, t, _o in ICONS] + [(g_missing_icon, None)]
    cols, cell, pad = 8, 104, 28
    rows = (len(entries) + cols - 1) // cols
    w, h = cols * cell + pad * 2, rows * cell + pad * 2 + 40
    parts = [f'  <rect width="{w}" height="{h}" fill="{CHROME["BG_Deep"]}"/>']
    for i, (glyph, token) in enumerate(entries):
        cx = pad + (i % cols) * cell
        cy = pad + 40 + (i // cols) * cell
        tint = CHROME["White"] if (grayscale or token is None) else SEMANTIC[token]
        parts.append(f'  <g transform="translate({cx + 16} {cy + 10}) scale(3)" '
                     f'stroke="{tint}" fill="none" stroke-width="{STROKE}" '
                     f'stroke-linecap="round" stroke-linejoin="round">')
        parts.append(glyph().replace('fill="#FFFFFF"', f'fill="{tint}"'))
        parts.append("  </g>")
    body = "\n".join(parts)
    filt = (' filter="url(#gs)"' if grayscale else "")
    defs = ('<defs><filter id="gs"><feColorMatrix type="saturate" values="0"/></filter></defs>'
            if grayscale else "")
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {w} {h}" width="{w}" '
            f'height="{h}">{defs}<g{filt}>\n{body}\n</g></svg>\n')


def _sheet_size() -> tuple[int, int]:
    cols, cell, pad = 8, 104, 28
    rows = (len(ICONS) + 1 + cols - 1) // cols
    return cols * cell + pad * 2, rows * cell + pad * 2 + 40


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", default="Content/RT/UI/_Generated",
                    help="cartella di output (default: Content/RT/UI/_Generated)")
    args = ap.parse_args()

    root = Path(args.out)
    # Gli assi dichiarati dal catalogo generico, letti dal C++: fase di risoluzione, slot, stile di
    # movimento, effetti. Sono il dato su cui una codifica di fase/effetto puo' poggiare senza che
    # nessuno riscriva a mano la mappa azione -> fase.
    AXES = action_axes()
    manifest: dict = {
        "generator": "tools/hud-assets/generate_hud_assets.py",
        "grammatica": "docs/research/design/icon/visual-language/",
        "nota": "Asset derivati e rigenerabili. Master monocromatico e tintabile, niente testo "
                "incorporato, niente riempimento percentuale cotto.",
        "icons": [],
        "frames": [],
    }
    rasterized = 0

    drawn: set[str] = set()
    for semantic, glyph, token, origin in ICONS:
        asset, iid, category = asset_name(semantic), icon_id(semantic), icon_category(semantic)
        drawn.add(iid)
        svg = svg_doc(glyph())
        _write(root / "Icons" / f"{asset}.svg", svg)
        min_size = MIN_READABLE.get(category, DEFAULT_MIN_READABLE)
        sizes = [s for s in PNG_SIZES if s >= min_size]
        for size in sizes:
            if _rasterize(svg, root / "Icons" / f"{asset}_{size}.png", size, size):
                rasterized += 1
        axes = AXES.get(semantic) if category == "Action" else None
        manifest["icons"].append({
            "AssetName": asset,
            "IconId": iid,
            "Category": category,
            "Axes": axes,
            "NativeSize": [int(GRID), int(GRID)],
            "PngSizes": sizes,
            "MinReadableSize": min_size,
            "SuggestedTint": SEMANTIC[token] if token else None,
            "Tintable": True,
            "AlphaMode": "straight RGBA, nessun matte",
            "EmbeddedText": False,
            "CVDSafe": "verificare sulla contact sheet in scala di grigi",
            "MockOrigin": origin,
        })

    for asset, factory, (w, h), margins, usage, origin in FRAMES:
        svg = factory()
        _write(root / "Frames" / f"{asset}.svg", svg)
        for mult in (1, 2):
            if _rasterize(svg, root / "Frames" / f"{asset}@{mult}x.png", w * mult, h * mult):
                rasterized += 1
        manifest["frames"].append({
            "AssetName": asset,
            "NativeSize": [w, h],
            "NineSliceMargins": list(margins) if margins else None,
            "DrawAs": "Box (9-slice)" if margins else "Image",
            "Tintable": True,
            "AlphaMode": "straight RGBA, nessun matte",
            "EmbeddedText": False,
            "ExpectedUMGUsage": usage,
            "MockOrigin": origin,
        })

    # `MissingIcon` non e' una chiave: e' il campo che il validator pretende, e senza il quale una
    # chiave sconosciuta non avrebbe nulla da mostrare.
    missing_svg = svg_doc(g_missing_icon())
    _write(root / "Icons" / f"{MISSING_ICON_ASSET}.svg", missing_svg)
    for size in PNG_SIZES:
        if _rasterize(missing_svg, root / "Icons" / f"{MISSING_ICON_ASSET}_{size}.png", size, size):
            rasterized += 1
    manifest["missingIcon"] = {
        "AssetName": MISSING_ICON_ASSET,
        "Field": "URTIconCatalogData::MissingIcon",
        "Nota": "non e' una voce di Icons[]: e' il campo che ResolveIcon restituisce con bResolved=false",
    }

    for name, gray in (("contact-sheet", False), ("contact-sheet-grayscale", True)):
        sheet = contact_sheet(gray)
        _write(root / "Review" / f"{name}.svg", sheet)
        sw, sh = _sheet_size()
        if _rasterize(sheet, root / "Review" / f"{name}.png", sw, sh):
            rasterized += 1

    _write(root / "manifest.json", json.dumps(manifest, indent=2, ensure_ascii=False) + "\n")

    # Il gate: la lista si interroga, non si copia.
    missing, extra = check_coverage(drawn)
    manifest["coverage"] = {
        "richieste": len(required_icon_ids()),
        "disegnate": len(drawn),
        "mancanti": missing,
        "fuori_dal_set_richiesto": extra,
        "letto_da": {k: str(v.relative_to(REPO_ROOT)) for k, v in SOURCES.items()},
    }
    print(f"{len(ICONS)} icone + {len(FRAMES)} cornici -> {root}")
    if missing:
        print(f"⛔ {len(missing)} chiavi richieste SENZA icona:")
        for m in missing:
            print(f"   {m}")
    else:
        print(f"✅ copertura completa: {len(required_icon_ids())} chiavi richieste, tutte disegnate")
    if extra:
        print(f"ℹ️  {len(extra)} icone fuori dal set richiesto (attese: ability degli eroi)")
    if cairosvg is None:
        print("⚠️  cairosvg assente: scritti solo gli SVG. `pip install cairosvg` per i PNG.")
    else:
        print(f"{rasterized} PNG rasterizzati")
    return 1 if missing else 0




# --------------------------------------------------------------------------------------------------
# Composizione — i due binari liberi
# --------------------------------------------------------------------------------------------------
# Misura, non intuizione: rasterizzando i 67 glifi sulla griglia 24 e contando quanti toccano ogni
# riga, due bande risultano di fatto vuote.
#
#     y= 0   1/67        y=22   7/67
#     y= 1   6/67        y=23   0/67
#     y= 2  18/67        y=21  29/67
#
# Le righe 0-1 e 22-23 sono libere in ~90% dei glifi. Sono DUE canali disponibili **senza ridisegnare
# niente**, ed e' il motivo per cui una codifica che vive li' costa quasi zero: i sei glifi che
# invadono la fascia alta si spostano di mezza unita', gli altri sessantuno restano come sono.
#
# Cosa ci sta e cosa no, misurato sul prototipo a 16/20/24/32 px:
#   - una POSIZIONE lungo il binario si legge da ~20 px in su. A 16 px collassa;
#   - un CONTEGGIO (una tacca, due, tre) non si legge mai sotto 24 px: contare e' piu' caro che
#     localizzare, e la skill bar mostra 16-24;
#   - la PRESENZA o assenza di un segno sul binario si legge anche a 16 px.
# Da cui la regola di degradazione: sotto 20 px il binario dice SE, non DOVE.

RAIL_TOP_Y = 1.4
RAIL_BOTTOM_Y = 22.6
RAIL_X0, RAIL_X1 = 3.4, 20.6


def rail(y: float, slot: int, slots: int, *, width: float = 2.2, guide: bool = True) -> str:
    """Un segno pieno nella posizione `slot` di `slots`, su un binario di sfondo.

    Il binario di sfondo non e' decorazione: senza, il giocatore vede un segno e non sa rispetto a
    quale scala leggerlo — la posizione e' informazione solo se si vedono gli estremi.
    """
    step = (RAIL_X1 - RAIL_X0) / slots
    cx = RAIL_X0 + step * slot + step / 2.0
    parts = []
    if guide:
        parts.append(path(f"M{_n(RAIL_X0)} {_n(y)} L{_n(RAIL_X1)} {_n(y)}",
                          stroke_width=0.7, stroke_opacity="0.4"))
    parts.append(path(f"M{_n(cx - width / 2)} {_n(y)} L{_n(cx + width / 2)} {_n(y)}",
                      stroke_width=2.3))
    return "\n".join(parts)


def compose(base: str, *layers: str) -> str:
    """Glifo base piu' strati. L'ordine e' quello di disegno: il base sotto, i marcatori sopra."""
    return "\n".join([base, *[layer for layer in layers if layer]])


if __name__ == "__main__":
    raise SystemExit(main())
