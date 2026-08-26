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
from pathlib import Path

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
    """
    return "\n".join([
        dot(3.6, 12, 1.5),
        path("M5.4 12 L9.6 12 L11.4 9 L13.4 15 L15.2 12 L17.4 12"),
        arrow_head(20.4, 12, 2.8),
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
    return "\n".join([
        path("M3.4 8.6 L20.6 8.6"),
        dot(12, 16.4, 2.6),
    ])


def g_certainty_predicted() -> str:
    return "\n".join([
        path("M3.4 8.6 L20.6 8.6", stroke_dasharray="4 3"),
        circle(12, 16.4, 2.6, stroke_width=1.5),
    ])


def g_certainty_uncertain() -> str:
    return "\n".join([
        path("M3.4 8.6 L20.6 8.6", stroke_dasharray="0.1 3.4"),
        path("M6 16.4 A 6 3.4 0 1 0 18 16.4 A 6 3.4 0 1 0 6 16.4",
             stroke_dasharray="0.1 3", stroke_width=1.6),
    ])


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
# Registro
# --------------------------------------------------------------------------------------------------
# `IconId` e nome asset non sono intercambiabili (`07-export-e-naming.md` §1): il primo lo risolve
# `URTIconLibrary`, il secondo lo vede il content browser. Qui stanno insieme una volta sola, ed e' il
# punto in cui un refuso diventa visibile invece che silenzioso.
#
# `origine_mock` dice da quale riquadro della tavola viene l'elemento e cosa e' cambiato. Un asset
# senza quella colonna non si sa se e' stato verificato o solo ricalcato.

ICONS = [
    # (AssetName, IconId, Categoria, glifo, tinta semantica, origine_mock)
    ("RT_UI_Icon_Action_Move", "UI.Icon.Action.Move", "Action", g_move, "Movement",
     "05/11 — invariato"),
    ("RT_UI_Icon_Action_Sprint", "UI.Icon.Action.Sprint", "Action", g_sprint, "Movement",
     "05/12 — endpoint che chiude: Sprint nega la reazione, il mock lo disegnava come Move piu' lungo"),
    ("RT_UI_Icon_Action_Dash", "UI.Icon.Action.Dash", "Action", g_dash, "Movement",
     "05/13 — invariato"),
    ("RT_UI_Icon_Action_BasicAttack", "UI.Icon.Action.BasicAttack", "Action", g_basic_attack, "Attack",
     "06/14 — invariato"),
    ("RT_UI_Icon_Action_Guard", "UI.Icon.Action.Guard", "Action", g_guard, "Defense",
     "AGGIUNTO — assente dal mock, ma e' una delle sette generiche (D-025)"),
    ("RT_UI_Icon_Action_Brace", "UI.Icon.Action.Brace", "Action", g_brace, "Defense",
     "07/19 — spostato da Reaction a Defense: Brace e' azione generica, non ramo di reazione"),
    ("RT_UI_Icon_Action_Interact", "UI.Icon.Action.Interact", "Action", g_interact, "Utility",
     "09/22 — la mano diventa Object + ingaggio: la mano non e' nel vocabolario di §2"),
    ("RT_UI_Icon_Action_Wait", "UI.Icon.Action.Wait", "Action", g_wait, "Utility",
     "09/23 — invariato"),
    ("RT_UI_Icon_Action_Overwatch", "UI.Icon.Action.Overwatch", "Action", g_overwatch, "Reaction",
     "07/20 — invariato"),

    ("RT_UI_Icon_Action_Hero_Gadget_ArcPulse", "UI.Icon.Action.Hero.Gadget.ArcPulse", "Action",
     g_arc_pulse, "Electric", "16 — rimappato: il kit del mock e' di un eroe fuori roster"),
    ("RT_UI_Icon_Action_Hero_Gadget_LinearDischarge", "UI.Icon.Action.Hero.Gadget.LinearDischarge",
     "Action", g_linear_discharge, "Electric",
     "16 — «Chain Discharge» non esiste; i salti erano Chain, questa e' Line"),
    ("RT_UI_Icon_Action_Hero_Gadget_ConductiveNode", "UI.Icon.Action.Hero.Gadget.ConductiveNode",
     "Action", g_conductive_node, "Electric", "17 — «Static Field» rimappato su ConductiveNode"),
    ("RT_UI_Icon_Action_Hero_Gadget_ReactiveCapacitor",
     "UI.Icon.Action.Hero.Gadget.ReactiveCapacitor", "Action", g_reactive_capacitor, "Electric",
     "AGGIUNTO — quinto token del kit di Gadget, assente dal mock"),
    ("RT_UI_Icon_Action_Hero_Gadget_Overload", "UI.Icon.Action.Hero.Gadget.Overload", "Action",
     g_overload, "Hazard", "18 — nome gia' corretto, glifo ridisegnato sulla grammatica Damage"),

    ("RT_UI_Phase_Prep", "UI.Icon.Phase.Prep", "Phase", g_phase_prep, "Utility",
     "03 — sostituisce la timeline a iniziativa"),
    ("RT_UI_Phase_Dash", "UI.Icon.Phase.Dash", "Phase", g_phase_dash, "Movement", "03 — idem"),
    ("RT_UI_Phase_Blast", "UI.Icon.Phase.Blast", "Phase", g_phase_blast, "Attack", "03 — idem"),
    ("RT_UI_Phase_Move", "UI.Icon.Phase.Move", "Phase", g_phase_move, "Movement", "03 — idem"),

    ("RT_UI_Icon_Certainty_Confirmed", "UI.Icon.Certainty.Confirmed", "Certainty",
     g_certainty_confirmed, None, "08 — invariato, piu' la marca di forma"),
    ("RT_UI_Icon_Certainty_Predicted", "UI.Icon.Certainty.Predicted", "Certainty",
     g_certainty_predicted, None, "08 — invariato, piu' la marca di forma"),
    ("RT_UI_Icon_Certainty_Uncertain", "UI.Icon.Certainty.Uncertain", "Certainty",
     g_certainty_uncertain, None, "08 — la marca e' un'area, non una cella esatta (§2.1)"),
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
    cols, cell, pad = 6, 104, 28
    rows = (len(ICONS) + cols - 1) // cols
    w, h = cols * cell + pad * 2, rows * cell + pad * 2 + 40
    parts = [f'  <rect width="{w}" height="{h}" fill="{CHROME["BG_Deep"]}"/>']
    for i, (asset, _icon_id, _cat, glyph, token, _origin) in enumerate(ICONS):
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


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", default="Content/RT/UI/_Generated",
                    help="cartella di output (default: Content/RT/UI/_Generated)")
    args = ap.parse_args()

    root = Path(args.out)
    manifest: dict = {
        "generator": "tools/hud-assets/generate_hud_assets.py",
        "grammatica": "docs/research/design/icon/visual-language/",
        "nota": "Asset derivati e rigenerabili. Master monocromatico e tintabile, niente testo "
                "incorporato, niente riempimento percentuale cotto.",
        "icons": [],
        "frames": [],
    }
    rasterized = 0

    for asset, icon_id, category, glyph, token, origin in ICONS:
        svg = svg_doc(glyph())
        _write(root / "Icons" / f"{asset}.svg", svg)
        min_size = MIN_READABLE.get(category, DEFAULT_MIN_READABLE)
        sizes = [s for s in PNG_SIZES if s >= min_size]
        for size in sizes:
            if _rasterize(svg, root / "Icons" / f"{asset}_{size}.png", size, size):
                rasterized += 1
        manifest["icons"].append({
            "AssetName": asset,
            "IconId": icon_id,
            "Category": category,
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

    for name, gray in (("contact-sheet", False), ("contact-sheet-grayscale", True)):
        sheet = contact_sheet(gray)
        _write(root / "Review" / f"{name}.svg", sheet)
        if _rasterize(sheet, root / "Review" / f"{name}.png", 6 * 104 + 56, 0 or 4 * 104 + 96):
            rasterized += 1

    _write(root / "manifest.json", json.dumps(manifest, indent=2, ensure_ascii=False) + "\n")

    print(f"{len(ICONS)} icone + {len(FRAMES)} cornici -> {root}")
    if cairosvg is None:
        print("⚠️  cairosvg assente: scritti solo gli SVG. `pip install cairosvg` per i PNG.")
    else:
        print(f"{rasterized} PNG rasterizzati")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
