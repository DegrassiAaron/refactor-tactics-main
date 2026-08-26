#!/usr/bin/env python3
"""Il test di collisione, eseguito invece che raccomandato.

`02-color-system.md` §1 e `progettazione-hud.md` §47-bis.1 chiedono la stessa verifica: portare la
tavola in scala di grigi e rileggerla. `03-forme-e-primitive.md` §7 elenca perfino le quattordici
coppie da controllare. Nessuno dei tre dice **come**, e una verifica «a occhio» su 67 glifi non e' una
verifica: e' un'opinione che cambia col monitor.

Qui la distanza fra due glifi si misura. Non e' percezione umana — nessuna metrica lo e' — ma ha due
proprieta' che l'occhio non ha: e' ripetibile, e ordina. Le coppie in cima alla lista sono quelle da
guardare per prime, ed e' tutto cio' che serve perche' il controllo smetta di dipendere da chi lo fa.

Metrica: alpha a 24 px (la taglia della skill bar), sfocatura leggera per non premiare differenze di
un pixel, distanza L2 normalizzata. Piu' la distanza e' bassa, piu' i due glifi si somigliano.

Uso:  python3 tools/hud-assets/collision_check.py [--top 20] [--size 24] [--pairs-from-doc]
"""

from __future__ import annotations

import argparse
import glob
import io
import itertools
import os
from pathlib import Path

import cairosvg
from PIL import Image, ImageFilter

REPO_ROOT = Path(__file__).resolve().parents[2]
ICONS = REPO_ROOT / "Content/RT/UI/_Generated/Icons"

# Le coppie che `03-forme-e-primitive.md` §7 dichiara critiche. Sono nominate come primitive, non come
# chiavi: la traduzione e' qui, ed e' l'unico punto in cui una tabella scritta a mano ha senso —
# perche' quel documento e' scritto a mano.
DOC_PAIRS = [
    ("Action_LineAttack", "Action_Move"),
    ("Action_Move", "Action_Sprint"),
    ("Action_Sprint", "Action_Dash"),
    ("Action_Hero_Gadget_ArcPulse", "Action_Overwatch"),
    ("Action_Shield", "Action_Brace"),
    ("Action_Brace", "Action_CreateCover"),
    ("Action_CreateCover", "Action_Guard"),
    ("Action_BasicAttack", "Action_Overwatch"),
    ("Action_Wait", "Action_Anchor"),
    ("Action_CreateWater", "Action_Ignite"),
    ("Status_Wet", "Action_CreateWater"),
    ("Status_Electrified", "Action_Electrify"),
    ("Status_Burning", "Action_Ignite"),
    ("Identity_Ally", "Identity_Enemy"),
    ("Certainty_Confirmed", "Certainty_Predicted"),
    ("Certainty_Predicted", "Certainty_Uncertain"),
    ("Action_Push", "Action_Dash"),
    ("Action_Interact", "Action_MarkTarget"),
    # aggiunte dalla misura, non dal documento: azione contro stato omonimo
    ("Action_Root", "Status_Root"),
    ("Action_Slow", "Status_Slow"),
    ("Action_Guard", "Status_Guarded"),
    ("Action_Brace", "Status_Braced"),
]


# Il contenitore di famiglia. Quattro famiglie condividono per PROGETTO una silhouette esterna, e
# quella silhouette e' la maggior parte dell'inchiostro: misurate cosi', due fasi o due superfici
# risultano sempre vicine, e il report grida al lupo su una somiglianza voluta.
#
# La correzione non e' silenziare l'avviso: e' togliere il contenitore e confrontare cio' che resta —
# che e' esattamente quello che fa un occhio umano quando ha gia' capito «e' una superficie» e sta
# cercando di capire QUALE.
#
# ⚠️ Il residuo va misurato, non dedotto: se dopo la sottrazione due glifi restano vicini, allora la
# somiglianza NON era del contenitore e il difetto e' vero.
FAMILY_TEMPLATES = {
    "Environment": "cell_hex()",
    "MapInteraction": "_edge_context()",
    "Phase": "polygon(PHASE_CHIP, stroke_width=1.4)",
    "Identity": "polygon(IDENTITY_BADGE, stroke_width=1.4)",
}


def family_of(name: str) -> str | None:
    head = name.split("_")[0]
    return head if head in FAMILY_TEMPLATES else None


def load_templates(size: int) -> dict[str, "Image.Image"]:
    """Rasterizza il solo contenitore di ogni famiglia, usando il generatore come sorgente: se il
    contenitore cambia li', qui cambia da solo."""
    import sys
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    import generate_hud_assets as gen

    out = {}
    for family, expr in FAMILY_TEMPLATES.items():
        body = eval(expr, vars(gen))  # noqa: S307 — espressione nostra, non input esterno
        png = cairosvg.svg2png(bytestring=gen.svg_doc(body).encode("utf-8"),
                               output_width=size, output_height=size)
        out[family] = Image.open(io.BytesIO(png)).convert("RGBA").split()[3].filter(
            ImageFilter.GaussianBlur(radius=size / 24.0))
    return out


def interior_distance(a: "Image.Image", b: "Image.Image", template: "Image.Image") -> float:
    """Distanza calcolata SOLO dove il contenitore non c'e', e normalizzata su quell'area.

    ⚠️ Il primo tentativo sottraeva il contenitore da entrambi e rimisurava. Non serviva a niente, ed
    e' un errore che vale la pena lasciare scritto: togliere lo stesso valore da A e da B non cambia
    |A - B|, quindi la distanza restava identica al millesimo. Il problema non era l'inchiostro
    condiviso nella differenza — era la NORMALIZZAZIONE: due glifi che differiscono solo in un'area
    piccola prendono un punteggio basso perche' quell'area e' divisa per tutta l'immagine.

    Qui il denominatore diventa l'area dell'interno. Un residuo alto significa «gli interni sono
    davvero diversi, era il contenitore a schiacciare il punteggio»; un residuo basso significa che i
    due glifi si somigliano dove conta, e il difetto e' vero.
    """
    pa, pb, pt = a.load(), b.load(), template.load()
    total, count = 0.0, 0
    for y in range(a.height):
        for x in range(a.width):
            if pt[x, y] > 40:      # dentro il contenitore: non e' li' che si distinguono
                continue
            diff = (pa[x, y] - pb[x, y]) / 255.0
            total += diff * diff
            count += 1
    return (total / count) ** 0.5 if count else 0.0


def load_masks(size: int) -> dict[str, "Image.Image"]:
    masks = {}
    for path in sorted(glob.glob(str(ICONS / "*.svg"))):
        name = os.path.basename(path).replace("RT_UI_Icon_", "").replace(".svg", "")
        png = cairosvg.svg2png(url=path, output_width=size, output_height=size)
        alpha = Image.open(io.BytesIO(png)).convert("RGBA").split()[3]
        masks[name] = alpha.filter(ImageFilter.GaussianBlur(radius=size / 24.0))
    return masks


def ink(a: "Image.Image") -> float:
    """Quantita' di tratto. E' il secondo segnale, e serve a coprire il punto cieco del primo.

    ⚠️ La distanza L2 su alpha sfocato **non vede il tratteggio**: `Certainty.Confirmed` e
    `Certainty.Predicted` differiscono per dash pattern — il differenziatore che
    `05-certainty-states.md` §2 prescrive — e la metrica le misura a 0.089, cioe' «identiche». La
    sfocatura che impedisce di premiare differenze di un pixel e' la stessa che cancella i vuoti fra i
    trattini. Una coppia vicina per forma ma lontana per inchiostro E' separabile, e da un canale
    legittimo: il peso.
    """
    pixels = a.load()
    return sum(pixels[x, y] for y in range(a.height) for x in range(a.width)) / (
        255.0 * a.width * a.height)


def distance(a: "Image.Image", b: "Image.Image") -> float:
    pa, pb = a.load(), b.load()
    total = 0.0
    for y in range(a.height):
        for x in range(a.width):
            diff = (pa[x, y] - pb[x, y]) / 255.0
            total += diff * diff
    return (total / (a.width * a.height)) ** 0.5


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--top", type=int, default=20)
    ap.add_argument("--size", type=int, default=24)
    ap.add_argument("--pairs-from-doc", action="store_true",
                    help="misura solo le coppie che 03-forme-e-primitive.md §7 dichiara critiche")
    args = ap.parse_args()

    masks = load_masks(args.size)
    templates = load_templates(args.size)
    if not masks:
        print(f"nessun glifo in {ICONS} — esegui prima generate_hud_assets.py")
        return 1
    print(f"{len(masks)} glifi, alpha a {args.size} px, in scala di grigi per costruzione "
          f"(il master e' monocromatico)\n")

    if args.pairs_from_doc:
        rows = []
        for left, right in DOC_PAIRS:
            if left in masks and right in masks:
                rows.append((distance(masks[left], masks[right]), left, right))
            else:
                missing = [n for n in (left, right) if n not in masks]
                print(f"  ⚠️  coppia non misurabile, glifo assente: {missing}")
        rows.sort()
        print("Coppie dichiarate critiche, dalla piu' vicina alla piu' distante:\n")
        for d, left, right in rows:
            weight = abs(ink(masks[left]) - ink(masks[right])) / max(
                ink(masks[left]), ink(masks[right]), 1e-6)
            separable = d >= 0.14 or weight >= 0.25
            flag = "  " if d >= 0.20 else ("⚠️ " if separable else "⛔")
            note = "" if d >= 0.20 else (
                f"   [forma vicina, ma inchiostro {weight:+.0%}: separabile per peso]"
                if separable and d < 0.14 else "")
            print(f"  {flag} {d:.3f}  {left}  vs  {right}{note}")
        irrisolte = [
            (d, l, r) for d, l, r in rows
            if d < 0.14 and abs(ink(masks[l]) - ink(masks[r])) / max(
                ink(masks[l]), ink(masks[r]), 1e-6) < 0.25]
        print(f"\nLa piu' vicina per forma e' a {rows[0][0]:.3f}." if rows else "")
        if irrisolte:
            print(f"⛔ {len(irrisolte)} coppie vicine per forma E per peso: non sono separabili")
            return 1
        print("✅ nessuna coppia critica resta indistinguibile: chi e' vicino per forma "
              "e' lontano per peso")
        return 0

    pairs = sorted(
        (distance(masks[a], masks[b]), a, b)
        for a, b in itertools.combinations(sorted(masks), 2))
    print(f"Le {args.top} coppie piu' vicine su {len(pairs)} confronti:\n")
    for d, left, right in pairs[:args.top]:
        flag = "⛔" if d < 0.14 else ("⚠️ " if d < 0.20 else "  ")
        # ⚠️ Una famiglia che condivide il contenitore (le fasi, le identita') misura sempre vicino:
        # il chip esagonale e' la maggior parte dell'inchiostro, e la sfocatura lo fa pesare ancora di
        # piu'. NON e' un difetto — il contenitore E' il segno di famiglia, e il differenziatore e'
        # l'interno. Senza questa nota il report grida al lupo su ogni coppia di fasi.
        family = family_of(left) if family_of(left) == family_of(right) else None
        note = ""
        if family:
            residual = interior_distance(masks[left], masks[right], templates[family])
            # ⚠️ La soglia 0.14 e' tarata sulla distanza a immagine piena. Applicata all'interno
            # normalizzato e' un prestito, non una calibrazione: serve a ORDINARE le coppie di una
            # famiglia, non a promuoverle o bocciarle. Una famiglia va comunque guardata.
            verdict = ("da guardare" if residual < 0.14
                       else "si somigliava solo il contenitore")
            note = f"   [famiglia {family}: interni a {residual:.3f} — {verdict}]"
            flag = "⛔" if residual < 0.14 else "  "
        elif left.split("_")[0] == right.split("_")[0] == "Certainty":
            note = "   [famiglia Certainty: la distinzione e' tratto e opacita', che la metrica non vede]"
            flag = "  "
        print(f"  {flag} {d:.3f}  {left}  vs  {right}{note}")
    print("\n⛔ sotto 0.14: da guardare, probabilmente indistinguibili a 24 px")
    print("⚠️  fra 0.14 e 0.20: distinguibili ma vicine; il colore NON basta a separarle")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
