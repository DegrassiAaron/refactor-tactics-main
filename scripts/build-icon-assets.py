#!/usr/bin/env python3
"""Produce i master iconografici di RefactorTactics da un'unica definizione geometrica.

Perche' esiste
--------------
Un glifo ha due consegne — SVG master e PNG a piu' dimensioni — e scriverle a mano significa
mantenere due volte la stessa forma. Alla prima correzione divergono, e la divergenza non e'
visibile finche' qualcuno non confronta i due file a occhio.

Qui la geometria e' dichiarata **una volta** come lista di primitivi astratti; SVG e PNG sono due
backend che la leggono. Correggere un'icona significa toccare un solo blocco di coordinate.

Convenzioni (docs/research/design/icon/visual-language/01-principi.md)
---------------------------------------------------------------------
canvas 24x24 · visual bounds 20x20 · centro logico 12,12 · stroke ~2 px · max 3 componenti
dominanti · master monocromatico e tintabile (bianco su alpha).

Uso
---
    python scripts/build-icon-assets.py            # genera SVG + PNG 16/24/48/96 + contact sheet
    python scripts/build-icon-assets.py --check    # fallisce se l'output non e' aggiornato
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

try:
    from PIL import Image, ImageDraw
except ImportError:  # pragma: no cover
    sys.exit("Serve Pillow: python -m pip install Pillow")

CANVAS = 24
"""Lato del canvas master in px. Ogni coordinata delle icone e' espressa qui dentro."""

SS = 32
"""Fattore di supersampling. Si disegna a 24*32 px e si riduce con LANCZOS: e' cio' che produce
un bordo pulito a 24 px senza che il glifo debba essere ridisegnato per ogni dimensione."""

# ⚠️ Spostato il 2026-08-19 da `docs/src/design/icon/assets` (#1165, fase 2). Questi file sono
# **output**, non ricerca: hanno un generatore committato e un `--check`, quindi vivono in
# `docs/generated/`. La riga cambia **nello stesso commit** del `git mv` — una vista segue la
# sorgente, e un generatore che scrive dove i file non stanno piu' li ricrea nel posto vecchio
# senza che nessuno se ne accorga.
OUT = Path("docs/generated/icons")
PNG_SIZES = (16, 24, 48, 96)


# --------------------------------------------------------------------------------------
# Primitivi
# --------------------------------------------------------------------------------------

def line(pts, w=2.0):
    """Polilinea aperta con estremi arrotondati."""
    return {"kind": "polyline", "pts": pts, "w": w}


def poly(pts, w=2.0, fill=False):
    """Poligono chiuso. `fill=True` lo rende massa piena invece che contorno."""
    return {"kind": "polygon", "pts": pts, "w": w, "fill": fill}


def dot(cx, cy, r):
    """Cerchio pieno."""
    return {"kind": "dot", "c": (cx, cy), "r": r}


def arc(cx, cy, r, a0, a1, w=2.0):
    """Arco di cerchio. Angoli in gradi, 0 = ore 3, crescenti in senso orario (convenzione PIL)."""
    return {"kind": "arc", "c": (cx, cy), "r": r, "a0": a0, "a1": a1, "w": w}


def ring(cx, cy, r, w=2.0):
    """Cerchio di contorno. Distinto da `dot` (pieno) e da `arc` (aperto)."""
    return {"kind": "ring", "c": (cx, cy), "r": r, "w": w}


# --------------------------------------------------------------------------------------
# Le quattro difese
#
# Sono il batch di calibrazione perche' sono la collisione semantica piu' probabile del sistema:
# quattro concetti diversi che il giocatore incontra nello stesso momento della stessa schermata.
# La distinzione e' portata dalla SILHOUETTE, non dal colore — i master sono monocromatici.
#
#   Guard   arco aperto sopra un corpo      postura: protetto DAVANTI, scoperto ai lati
#   Brace   corpo su puntelli a terra       ancoraggio: piantato per ricevere l'impatto
#   Shield  contorno chiuso e vuoto         risorsa astratta: un pool che assorbe
#   Cover   massa piena ancorata al suolo   materia della mappa: un riparo che esiste nel mondo
#
# Le quattro classi di forma sono deliberatamente disgiunte: arco / treppiede / contorno vuoto /
# massa piena. Nessuna coppia condivide la classe, quindi nessuna coppia collassa in grayscale.
# --------------------------------------------------------------------------------------

ICONS = {
    # `UI.Icon.Action.Guard` — D-025, difesa direzionale che decade fuori dall'arco frontale
    # (ADR-0005 §4a). L'arco e' APERTO ai lati: e' il disegno che dice dove la riduzione non vale.
    #
    # Iterazione 2: nella prima passata arco e corpo si toccavano e a 24 px il glifo collassava in
    # un blob illeggibile. L'arco e' stato alzato e il corpo abbassato fino a ~4 px di stacco: e'
    # il vuoto fra i due a dire «riparo SOPRA un'unita'» invece di «macchia».
    "RT_UI_Icon_Action_Guard": [
        arc(12, 12.6, 8.6, 196, 344, w=2.3),
        dot(12, 17.6, 2.9),
    ],

    # `UI.Icon.Action.Brace` — prepararsi a ricevere l'impatto. Puntelli divaricati e linea di
    # terra: la lettura e' «piantato», non «coperto». Non riusa il glifo di Shield.
    #
    # Iterazione 3: due gambe SIMMETRICHE da un vertice unico chiudono un triangolo, e il glifo
    # leggeva «piramide» a ogni dimensione — allungare la terra non era servito. La postura ora e'
    # ASIMMETRICA: gamba posteriore lunga in diagonale, gamba anteriore corta e quasi verticale.
    # E' l'asimmetria a dire «mi puntello CONTRO qualcosa», e a togliere la lettura geometrica.
    "RT_UI_Icon_Action_Brace": [
        line([(3.0, 20.6), (21.0, 20.6)], w=1.8),
        line([(13.2, 10.2), (6.0, 19.2)], w=2.5),
        line([(14.8, 10.4), (16.4, 19.2)], w=2.5),
        dot(13.8, 7.0, 3.2),
    ],

    # `UI.Icon.Action.Shield` — pool che assorbe. Contorno chiuso con ampio negative space: e' una
    # RISORSA, non una postura. Nessuna linea di terra: non appartiene al mondo, appartiene all'unita'.
    "RT_UI_Icon_Action_Shield": [
        poly([(12, 3.0), (20.0, 6.2), (20.0, 12.0), (12, 21.0), (4.0, 12.0), (4.0, 6.2)], w=2.2),
    ],

    # `UI.Icon.Environment.Cover` — proprieta' della MAPPA. Massa piena ancorata al suolo, con
    # l'unita' protetta dietro: e' materia, e ha un lato. Categoria `Environment` = E25, non v0.1.
    #
    # Iterazione 2: il corpo era troppo distante dal muro e leggeva come un secondo oggetto slegato.
    # Avvicinato e abbassato — «acquattato dietro», non «palla accanto a un blocco».
    "RT_UI_Icon_Environment_Cover": [
        line([(3.0, 20.6), (21.0, 20.6)], w=1.8),
        poly([(4.6, 7.6), (12.4, 7.6), (12.4, 19.4), (4.6, 19.4)], fill=True),
        dot(15.9, 16.6, 2.9),
    ],

    # ----------------------------------------------------------------------------------
    # BATCH 02 — movimento
    #
    # Tre azioni che il giocatore sceglie nella stessa riga della skill bar, piu' una primitiva
    # di controllo. La collisione qui non e' solo di forma: e' di SIGNIFICATO. `Action.Sprint`
    # consuma entrambi gli slot e nega la reazione per il turno, quindi un glifo che lo fa
    # sembrare «Move piu' veloce» mente nel momento in cui il giocatore sceglie.
    #
    #   Move    path a scalino CON nodi        un percorso che si pianifica cella per cella
    #   Sprint  stesso scalino SENZA nodi      stride lungo, tratto spesso, speed trail
    #   Dash    retta con chevron              scatto, non percorso
    #   Line    retta pulita                   primitiva: origine + segmento + punta
    # ----------------------------------------------------------------------------------

    # `UI.Icon.Action.Move` — i NODI intermedi sono cio' che la definisce: senza, e' una `Line`.
    #
    # Iterazione 2: con nodi a r=1.7 su tratto w=2.0 le perle sporgevano 0.7 px per lato e a
    # 24 px sparivano — cioe' spariva proprio il tratto distintivo, e Move collassava su Sprint.
    # Il tratto e' stato assottigliato E i nodi ingranditi: e' il CONTRASTO fra i due a rendere
    # leggibile il percorso, non la dimensione assoluta dei nodi.
    "RT_UI_Icon_Action_Move": [
        line([(5.0, 17.6), (10.6, 17.6), (14.6, 10.6), (17.2, 10.6)], w=1.8),
        dot(5.0, 17.6, 2.7),
        dot(10.6, 17.6, 2.2),
        dot(14.6, 10.6, 2.2),
        poly([(16.2, 7.4), (21.2, 10.6), (16.2, 13.8)], fill=True),
    ],

    # `UI.Icon.Action.Sprint` — stessa famiglia di Move (path spezzato), ritmo opposto: nessun
    # nodo, un solo stride, tratto piu' spesso, speed trail dietro. NON e' Move con un `x2`.
    # Il costo reale — slot doppio, reazione negata — lo comunica lo stato dello slot, non il
    # glifo: un'icona non puo' portare una regola, e fingere che lo faccia sarebbe peggio.
    # Iterazione 2: i due trattini staccati leggevano come rumore invece che come scia. Ora sono
    # piu' lunghi e affiancano l'attacco del path, sopra e sotto: la lettura e' «qualcosa e'
    # passato di qui in fretta». Il path resta spezzato — Sprint e' un profilo di movimento su
    # percorso, non uno scatto — ma senza nodi, ed e' l'assenza di nodi a separarlo da Move.
    "RT_UI_Icon_Action_Sprint": [
        line([(2.6, 14.4), (8.0, 14.4)], w=1.7),
        line([(2.6, 21.0), (8.0, 21.0)], w=1.7),
        line([(5.4, 17.8), (13.0, 17.8), (17.0, 10.3)], w=2.8),
        poly([(15.6, 6.9), (21.4, 10.3), (15.6, 13.7)], fill=True),
    ],

    # `UI.Icon.Action.Dash` — scatto, non percorso: retta, chevron, punta piena. A 16 px resta
    # `● » ►`. La progressione aperto -> pieno legge come accelerazione.
    "RT_UI_Icon_Action_Dash": [
        dot(4.2, 12.0, 2.6),
        line([(9.0, 7.8), (13.0, 12.0), (9.0, 16.2)], w=2.2),
        poly([(14.8, 7.6), (20.4, 12.0), (14.8, 16.4)], fill=True),
    ],

    # PRIMITIVA di grammatica, non voce di catalogo: prefisso `RT_Prim_`, non `RT_UI_Icon_`.
    # Esiste in questo batch per un motivo solo — verificare che `Move` non le collida contro.
    "RT_Prim_Geometry_Line": [
        dot(3.8, 12.0, 2.6),
        line([(6.4, 12.0), (15.6, 12.0)], w=2.2),
        poly([(14.6, 8.2), (20.6, 12.0), (14.6, 15.8)], fill=True),
    ],

    # ----------------------------------------------------------------------------------
    # BATCH 03 — elettricita' e reazione
    #
    # La collisione dichiarata e' `Electric` vs `Reaction`: ogni convenzione visiva del genere
    # tende a fondere le due su un fulmine, e il progetto le tiene separate per regola — il
    # fulmine appartiene a Electric, la reazione usa un anello spezzato. Ma disegnandole si
    # scopre una SECONDA collisione che nessun documento aveva previsto: il frame di uno
    # `Status` e' un anello CHIUSO, quello di una reazione un anello SPEZZATO. Due anelli.
    #
    # Il ciclo di reazione qui e' solo quello che il gioco produce davvero:
    # `URTReactionOpportunityLibrary::BuildOverwatchTriggers` costruisce le opportunity, e
    # `ERTReactionOutcome` ha tre esiti. `Armed`, `Consumed`, `Expired` e `Invalidated` sono
    # voci del manifest sorgente senza alcuna fonte nel codice: non si disegnano.
    # ----------------------------------------------------------------------------------

    # `UI.Icon.Environment.Electric` — payload nudo, senza frame: e' l'elemento in se'.
    # Massa piena, max 3 segmenti. Categoria `Environment` = E25.
    "RT_UI_Icon_Environment_Electric": [
        poly([(14.6, 2.6), (7.2, 13.4), (11.3, 13.4), (9.4, 21.4),
              (16.8, 10.6), (12.7, 10.6)], fill=True),
    ],

    # `UI.Icon.Status.Electrified` — lo STESSO payload dentro un anello di stato CHIUSO.
    # Non e' il fulmine ricolorato: e' il fulmine incorniciato, ed e' il frame a dire
    # «questo e' lo stato di un'unita'», non «questa e' una cella». Set obbligatorio v0.1.
    # Costruita con `_status()` piu' sotto, come gli altri dieci.

    # `UI.Icon.Reaction.Generic` — anello spezzato in DUE punti, simmetrico: due parentesi
    # circolari che lasciano il centro vuoto. Nessun fulmine, nessun orologio.
    #
    # Iterazione 2: la prima versione era un anello con un solo gap piu' un notch, cioe'
    # esattamente la silhouette dell'icona REFRESH — una delle convenzioni piu' forti che
    # esistano. Il giocatore avrebbe letto «ricarica». La simmetria toglie la rotazione: due
    # archi opposti non girano, aspettano. Categoria `Reaction` = E25.
    "RT_UI_Icon_Reaction_Generic": [
        arc(12, 12, 8.6, 302, 58, w=2.4),
        arc(12, 12, 8.6, 122, 238, w=2.4),
    ],

    # `UI.Icon.Reaction.Opportunity` — la stessa finestra, ma con un bersaglio dentro. La
    # progressione e' leggibile senza label: parentesi vuote -> parentesi con bersaglio. E'
    # lo stato in cui il giocatore deve scegliere entro 3,0 s, prodotto davvero da
    # `URTReactionOpportunityLibrary::BuildOverwatchTriggers`.
    "RT_UI_Icon_Reaction_Opportunity": [
        arc(12, 12, 8.6, 302, 58, w=2.4),
        arc(12, 12, 8.6, 122, 238, w=2.4),
        dot(12, 12, 3.6),
    ],

    # `UI.Icon.Action.Wait` — controllo del batch: clessidra, classe di forma disgiunta dagli
    # anelli. Serve a verificare che nessuna delle due reazioni le somigli. Obbligatoria v0.1.
    #
    # Iterazione 2: due triangoli soli leggevano come un papillon. Le barre superiore e
    # inferiore sono cio' che rende una clessidra una clessidra.
    "RT_UI_Icon_Action_Wait": [
        line([(6.8, 3.6), (17.2, 3.6)], w=2.1),
        line([(6.8, 20.4), (17.2, 20.4)], w=2.1),
        poly([(8.0, 5.2), (16.0, 5.2), (12.0, 11.4)], fill=True),
        poly([(12.0, 12.6), (16.0, 18.8), (8.0, 18.8)], fill=True),
    ],

    # ----------------------------------------------------------------------------------
    # BATCH 04 — le quattro fasi
    #
    # `RequiredIconIds()` le genera da `ERTMatchPhase`, nell'ordine del turno, e prende solo le
    # quattro VOLONTARIE: `Planning` e `Cleanup` non sono fasi in cui il giocatore agisce, e la
    # reazione non e' una quinta fase — mostrarla qui la trasformerebbe in una.
    #
    # La decisione di design che conta e' NON dare loro un pittogramma. Una fase non e' un
    # gesto, e' un momento dentro una sequenza: quattro tacche, quella corrente piu' alta e
    # piena. Ne discendono tre cose.
    #
    #  1. `Phase.Dash` e `Action.Dash` non possono collidere: una e' un indicatore di
    #     posizione, l'altra un gesto. Nessuna forma condivisa. Idem `Phase.Move`/`Action.Move`.
    #  2. La Ghost Timeline «sembra una timeline di fase, non una action queue» perche' i suoi
    #     nodi lo sono davvero (progettazione-hud.md §8, §47.6).
    #  3. Il prezzo, dichiarato: standalone le quattro si somigliano molto — differiscono per
    #     QUALE tacca e' alta. E' corretto (sono la stessa cosa in momenti diversi) ma le rende
    #     deboli fuori da un contesto sequenziale, ed e' il punto da verificare guardandole.
    # ----------------------------------------------------------------------------------
}

# `MissingIcon` — il campo obbligatorio di `URTIconCatalogData`. Non e' una delle 51 chiavi: e' la
# 52esima, e il validator RIFIUTA un catalogo che non la dichiara («una chiave sconosciuta non
# avrebbe nulla da mostrare», RTIconLibrary.cpp:122). Si vede solo quando qualcosa e' rotto, quindi
# deve dire «manca il contenuto» e nient'altro: non `Invalid` (slash), non `Uncertain` (fade + `?`).
#
# Quattro angoli di un contenitore vuoto. ⚠️ Da riverificare quando arrivera' `Information.Targeted`,
# che secondo il manifest sorgente usa anch'essa dei bracket: li' la collisione sara' reale.
ICONS["RT_UI_Icon_MissingIcon"] = [
    line([(4.4, 8.4), (4.4, 4.4), (8.4, 4.4)], w=2.1),
    line([(15.6, 4.4), (19.6, 4.4), (19.6, 8.4)], w=2.1),
    line([(19.6, 15.6), (19.6, 19.6), (15.6, 19.6)], w=2.1),
    line([(8.4, 19.6), (4.4, 19.6), (4.4, 15.6)], w=2.1),
    dot(12.0, 12.0, 2.0),
]

# ----------------------------------------------------------------------------------
# BATCH 05 — gli undici Status
#
# La fonte non e' un documento: sono i tag registrati sotto `Status.` in `RTGameplayTags.cpp`,
# cioe' gli stessi che il gameplay applica davvero. Il manifest sorgente ne elencava quindici, di
# cui nove senza alcun tag corrispondente — e due erano quasi-omonimi insidiosi: `Rooted` per
# `Root`, `Slowed` per `Slow`. Un nome quasi giusto passa il validator e non risolve mai; da
# questa revisione lo intercetta `check_conformance()`.
#
# Tutti condividono un anello di stato CHIUSO: e' il frame a dire «questo riguarda un'unita'»,
# e a distinguerli dal payload nudo (`Environment`) e dalla cella (`Surface`). Dentro restano
# ~13 unita' di canvas, quindi il payload puo' avere al massimo due componenti.
# ----------------------------------------------------------------------------------


def _status(payload):
    """Anello di stato chiuso piu' il payload. L'anello e' definito qui una volta sola."""
    return [ring(12, 12, 9.0, w=1.9)] + payload


ICONS.update({
    # Il fulmine di `Environment.Electric`, rimpicciolito e incorniciato.
    "RT_UI_Icon_Status_Electrified": _status([
        poly([(13.6, 5.4), (8.6, 12.7), (11.4, 12.7), (10.1, 18.4),
              (15.1, 11.1), (12.3, 11.1)], fill=True)]),

    # Goccia: punta dritta, corpo tondo. La differenza con `Burning` e' tutta qui — una goccia
    # e' simmetrica e chiusa, una fiamma e' asimmetrica e guizza.
    "RT_UI_Icon_Status_Wet": _status([
        poly([(12.0, 5.0), (16.2, 12.2), (7.8, 12.2)], fill=True),
        dot(12.0, 13.6, 4.2)]),

    # Fiamma: la punta e' piegata NETTAMENTE a destra e la base e' stretta — il contrario della
    # goccia, che ha punta dritta e base larga. E' l'unico modo per tenerle separate a 24 px:
    # dentro l'anello restano ~11 px effettivi, e li' una fiamma «realistica» e una goccia sono
    # la stessa macchia. Se il playtest le confonde ancora, la via non e' ridisegnarle ma dare
    # a entrambe una variante ottica a 16 px.
    #
    # Iterazione 3: la versione a zigzag risolveva `Wet` ma somigliava a un FULMINE, cioe'
    # violava la regola LOCKED piu' netta del progetto — il fulmine appartiene a `Electric` e a
    # nient'altro. Scambiare una collisione con una peggiore non e' un progresso. Ora il profilo
    # e' una fiamma vera: corpo tondo in basso, punta piegata a destra, incavo a sinistra.
    # Nessun angolo acuto ripetuto, che e' cio' che leggeva come scarica.
    "RT_UI_Icon_Status_Burning": _status([
        poly([(13.8, 4.8), (15.6, 9.0), (16.2, 12.6), (15.2, 16.0), (12.0, 18.2),
              (8.8, 16.2), (8.2, 12.8), (9.6, 9.8), (11.3, 11.6), (11.0, 8.0)], fill=True)]),

    # ⚠️ Iterazione 2 — il budget dentro un frame e' piu' stretto di quello di un glifo nudo.
    # `Braced`, `Guarded` ed `Exposed` avevano due o tre componenti ciascuna e a 24 px erano
    # macchie: l'anello mangia meta' della superficie, e cio' che resta regge UN componente.
    # Tutte e tre riscritte a un solo elemento.

    # Cuneo a base larga: abbassato e stabile. Volontario, dove `Root` e' subito.
    "RT_UI_Icon_Status_Braced": _status([
        poly([(7.8, 16.4), (16.2, 16.4), (12.0, 8.6)], fill=True)]),

    # Arco a tetto: coperto sopra, aperto ai lati. Iterazione 3 — era un chevron ANGOLARE, e
    # `Action.Guard` e' un arco CURVO: due forme di famiglia diversa per lo stesso concetto.
    # La parentela azione -> stato si conserva sulla curva (04-regole-di-composizione.md §4.1).
    "RT_UI_Icon_Status_Guarded": _status([
        arc(12, 14.4, 5.4, 200, 340, w=2.6)]),

    # Lo stesso chevron rovesciato: scoperto. Prodotto da `Sprint`, e con `Guarded` forma la
    # coppia che deve reggere meglio di tutte — protetto contro esposto, ∧ contro ∨.
    "RT_UI_Icon_Status_Exposed": _status([
        line([(7.4, 9.0), (12.0, 14.6), (16.6, 9.0)], w=2.6)]),

    # Rombo pieno: l'unico del set. Un marchio, non una postura.
    "RT_UI_Icon_Status_Marked": _status([
        poly([(12.0, 6.2), (16.4, 12.0), (12.0, 17.8), (7.6, 12.0)], fill=True)]),

    # Occhio barrato: nascosto. ⚠️ La barra riusa lo slash che `05-certainty-states.md` assegna a
    # `Invalid`. E' una sovrapposizione consapevole: li' e' un MODIFICATORE su un'azione, qui il
    # payload di uno stato, e non compaiono sulla stessa superficie. Se un giorno lo faranno,
    # a spostarsi e' questo.
    #
    # Iterazione 2: tre bande orizzontali leggevano come un menu hamburger, e toccavano l'anello.
    "RT_UI_Icon_Status_Obscured": _status([
        poly([(6.6, 12.0), (12.0, 8.4), (17.4, 12.0), (12.0, 15.6)], w=1.9),
        line([(8.0, 16.6), (16.0, 7.4)], w=2.2)]),

    # Lo stesso occhio, con la pupilla: rilevato. Insieme a `Obscured` formano la coppia piu'
    # leggibile del batch, ed e' la grammatica payload/frame a reggerla — lo stesso occhio nudo
    # sara' `Information.Visible`, che e' l'altro lato dello stesso fatto.
    #
    # Iterazione 2: onde concentriche leggevano come l'icona WIFI, una convenzione di
    # connettivita' troppo forte per un gioco tattico.
    "RT_UI_Icon_Status_Reveal": _status([
        poly([(6.6, 12.0), (12.0, 8.4), (17.4, 12.0), (12.0, 15.6)], w=1.9),
        dot(12.0, 12.0, 2.3)]),

    # T rovesciata: ancorato al suolo. Subito, dove `Braced` e' volontario.
    #
    # Iterazione 2: corpo piu' gambo piu' base leggeva come un lampione.
    "RT_UI_Icon_Status_Root": _status([
        line([(12.0, 7.2), (12.0, 15.2)], w=2.6),
        line([(7.4, 16.6), (16.6, 16.6)], w=2.6)]),

    # Un chevron che avanza e trova un muro: frenato. Richiama `Dash` e lo nega.
    "RT_UI_Icon_Status_Slow": _status([
        line([(8.4, 8.0), (12.2, 12.0), (8.4, 16.0)], w=2.1),
        line([(15.4, 7.4), (15.4, 16.6)], w=2.4)]),
})

# ----------------------------------------------------------------------------------
# BATCH 06 — le ventinove azioni rimanenti
#
# Non hanno tutte lo stesso pubblico, e non meritano lo stesso budget di cura:
#
#   UNIVERSALI   compaiono nella dock e si scelgono sotto pressione (§6.7 dell'HUD)
#   CONTROLLO    compaiono in tooltip, log e schede, e si leggono con calma
#
# Nove sono COMPOSIZIONE di primitive gia' definite in `03-forme-e-primitive.md` — `Line`,
# `Circle`, `Cover`, `Water`, `Push`, `Pull`. Il rischio non e' disegnarle male: e' disegnarle
# ex novo ignorando la grammatica che esiste.
#
# Quattro hanno uno `Status` omonimo gia' prodotto (`Root`, `Slow`, piu' `Guard`/`Brace` fatte
# nel batch 01). Per loro vale §4.1: dove il payload regge in entrambi i contesti resta
# IDENTICO, e non si inventano due disegni per la stessa cosa.
# ----------------------------------------------------------------------------------

_TARGET_DOT = dot(4.6, 12.0, 2.5)
"""Origine dell'azione: lo stesso punto di partenza di `Move`, `Dash` e `Line`."""


def _arrow(x, y, back=5.2, half=3.6):
    """Punta piena rivolta a destra, come quelle di `Move` e `Dash`."""
    return poly([(x - back, y - half), (x, y), (x - back, y + half)], fill=True)


ICONS.update({
    # --- ATTACCHI (universali) — tutti condividono l'impatto di `Effect.Damage` ---------

    # Iterazione 2. Nella prima passata `BasicAttack` e `PrecisionAttack` erano lo stesso reticolo
    # con raggi diversi — indistinguibili a 24 px, e sono due voci ADIACENTI nella stessa dock.
    # Ora differiscono per struttura: anello con tacche contro croce a tutta larghezza.

    # Reticolo: anello, quattro tacche, punto. L'attacco generico, senza arma (D-025).
    "RT_UI_Icon_Action_BasicAttack": [
        ring(12, 12, 6.6, w=2.1),
        line([(12, 3.0), (12, 6.2)], w=2.1), line([(12, 17.8), (12, 21.0)], w=2.1),
        line([(3.0, 12), (6.2, 12)], w=2.1), line([(21.0, 12), (17.8, 12)], w=2.1),
        dot(12, 12, 2.3),
    ],
    # Mirino APERTO: quattro tacche che puntano al centro e si fermano prima, piu' un punto
    # piccolo. E' il gap a portare il significato — «stringo su un bersaglio» — e a distinguerlo
    # sia da `BasicAttack` (che ha l'anello) sia dal segno `+`.
    #
    # Iterazione 4, e vale la pena registrare le tre scartate: croce piena = `+` («aggiungi»);
    # cerchio con manico = lente («cerca»); stesso reticolo di BasicAttack con raggi diversi =
    # indistinguibile a 24 px. Il costo di questi glifi non e' disegnarli, e' schivare i
    # significati gia' occupati.
    # ⚠️ A 16 px il gap si chiude e la lettura si avvicina a `+`: e' un caso da variante ottica.
    "RT_UI_Icon_Action_PrecisionAttack": [
        line([(12, 2.8), (12, 8.0)], w=2.0), line([(12, 16.0), (12, 21.2)], w=2.0),
        line([(2.8, 12), (8.0, 12)], w=2.0), line([(21.2, 12), (16.0, 12)], w=2.0),
        dot(12, 12, 1.8),
    ],
    # Massa d'impatto piu' TRE schegge, in direzioni e lunghezze diverse. `Effect.Damage`
    # prescrive «3-4 diramazioni max»: e' il numero a impedire la lettura come stella.
    # Iterazione 3 — un poligono a punte, per quanto irregolari, resta una stella a cinque punte,
    # cioe' «preferito». Prima era uno `sparkle`. Entrambe le volte il problema era la SIMMETRIA
    # e il numero di punte, non la forma del bordo.
    "RT_UI_Icon_Action_HeavyAttack": [
        dot(11.6, 12.4, 5.0),
        line([(13.8, 8.6), (19.6, 3.8)], w=2.4),
        line([(16.2, 14.2), (21.4, 17.2)], w=2.2),
        line([(8.0, 16.4), (4.6, 20.4)], w=2.0),
    ],
    # `Geometry.Line` piu' l'impatto in punta: e' l'impatto a farne un attacco invece che una
    # primitiva. Senza, sarebbe identica al glifo di calibrazione `Line`.
    "RT_UI_Icon_Action_LineAttack": [
        _TARGET_DOT,
        line([(7.2, 12.0), (13.2, 12.0)], w=2.2),
        _arrow(19.0, 12.0),
        line([(20.4, 6.6), (20.4, 17.4)], w=1.8),
    ],
    # `Geometry.Circle` piu' massa al centro: l'area, non il bersaglio.
    "RT_UI_Icon_Action_CircularAoE": [
        ring(12, 12, 8.4, w=2.2),
        dot(12, 12, 3.6),
    ],
    # Una linea che batte verso il basso a intervalli: la zona sotto tiro, non un colpo.
    # Iterazione 3 — tacche che ATTRAVERSANO la linea disegnano un cancelletto. Tenendole da un
    # lato solo la figura diventa asimmetrica e smette di essere un simbolo tipografico.
    "RT_UI_Icon_Action_SuppressiveLine": [
        line([(3.2, 8.6), (20.8, 8.6)], w=2.3),
        line([(7.4, 10.4), (7.4, 17.2)], w=1.9),
        line([(12.0, 10.4), (12.0, 19.4)], w=1.9),
        line([(16.6, 10.4), (16.6, 17.2)], w=1.9),
    ],
    # --- SPOSTAMENTO (universali) — famiglia di `Move`/`Dash`: origine, tratto, punta -----

    # Dash che finisce contro qualcosa: la carica.
    "RT_UI_Icon_Action_Charge": [
        dot(3.8, 12.0, 2.5),
        line([(7.6, 8.4), (11.2, 12.0), (7.6, 15.6)], w=2.2),
        _arrow(17.6, 12.0),
        line([(19.4, 6.4), (19.4, 17.6)], w=2.4),
    ],
    # Arco sopra un ostacolo: il salto. L'unica azione di movimento che lascia il suolo.
    "RT_UI_Icon_Action_Leap": [
        dot(4.2, 18.6, 2.4),
        arc(12, 18.6, 8.0, 190, 350, w=2.2),
        dot(19.8, 18.6, 2.4),
    ],
    # Spostamento breve origine -> destinazione, senza percorso: non e' un `Move`.
    "RT_UI_Icon_Action_Reposition": [
        ring(6.4, 15.6, 3.0, w=2.0),
        line([(9.6, 13.0), (14.4, 8.6)], w=2.0),
        dot(17.4, 6.6, 3.2),
    ],
    # `Effect.Push`: impulso esterno, unita', uscita.
    "RT_UI_Icon_Action_Push": [
        line([(3.0, 7.6), (3.0, 16.4)], w=2.4),
        dot(9.0, 12.0, 2.8),
        _arrow(21.0, 12.0),
    ],
    # `Effect.Pull`: lo stesso impulso al contrario. Coppia esatta con `Push`.
    "RT_UI_Icon_Action_Pull": [
        line([(21.0, 7.6), (21.0, 16.4)], w=2.4),
        dot(15.0, 12.0, 2.8),
        poly([(8.2, 8.4), (3.0, 12.0), (8.2, 15.6)], fill=True),
    ],

    # --- DIFESA E REAZIONE (controllo) — famiglia dell'anello spezzato di `Reaction` -------

    # Il colpo torna indietro dalla difesa. Iterazione 2 — un arco con la punta e' l'icona
    # REFRESH, che in questa sessione si e' ripresentata tre volte: la grammatica dell'anello
    # spezzato appartiene a `Reaction`, e un anello con una freccia significa «ricarica» ovunque.
    "RT_UI_Icon_Action_Counter": [
        line([(17.6, 4.6), (17.6, 19.4)], w=2.8),
        line([(15.0, 12.0), (7.4, 12.0)], w=2.3),
        poly([(8.6, 7.2), (3.0, 12.0), (8.6, 16.8)], fill=True),
    ],
    # Colpo deviato di lato: cambia direzione, non si ferma.
    "RT_UI_Icon_Action_Deflect": [
        line([(3.0, 19.4), (11.4, 11.0)], w=2.2),
        line([(9.4, 4.6), (15.4, 16.8)], w=2.6),
        _arrow(21.0, 6.4, back=4.6, half=3.2),
    ],
    # Colpo fermato a meta' strada, prima del bersaglio.
    "RT_UI_Icon_Action_Intercept": [
        dot(3.6, 12.0, 2.4),
        line([(6.6, 12.0), (11.4, 12.0)], w=2.2),
        line([(13.4, 5.6), (13.4, 18.4)], w=2.8),
        ring(18.6, 12.0, 2.8, w=2.0),
    ],
    # Scarto laterale: l'unita' non e' piu' dov'era.
    "RT_UI_Icon_Action_Evade": [
        line([(3.4, 6.4), (11.0, 6.4)], w=2.2),
        _arrow(16.0, 6.4, back=4.2, half=3.0),
        line([(11.6, 9.0), (11.6, 14.2)], w=1.8),
        dot(11.6, 18.0, 3.2),
    ],
    # Ancora vera — gambo e marre curve. Iterazione 2: era una T rovesciata con un punto sopra,
    # cioe' `Root` piu' un dettaglio, e a 24 px il dettaglio spariva. Sono due azioni diverse
    # (`Anchor` resiste alla spinta, `Root` blocca un altro) e devono avere due silhouette.
    "RT_UI_Icon_Action_Anchor": [
        dot(12.0, 4.6, 2.6),
        line([(12.0, 7.0), (12.0, 19.6)], w=2.4),
        arc(12, 13.4, 7.2, 25, 155, w=2.4),
    ],

    # --- SUPPORTO E STATO (misto) -----------------------------------------------------

    # Battito: il recupero come segno vitale. Iterazione 2 — il «pulse geometrico» prescritto da
    # `Effect.Heal` e' uscito come un CUORE, che e' la convenzione piu' templata del mondo e
    # stona in un tattico. Una croce sarebbe stata l'altra trappola: croce medica, o `+`.
    "RT_UI_Icon_Action_Heal": [
        line([(2.6, 12.0), (7.4, 12.0), (9.8, 5.4), (13.6, 18.6), (15.8, 12.0), (21.4, 12.0)],
             w=2.3),
    ],
    # Uno stato che se ne va: l'anello si apre e il contenuto esce. Iterazione 2 — un anello con
    # una barra diagonale E' il simbolo Ø, cioe' «vietato».
    "RT_UI_Icon_Action_Cleanse": [
        arc(12, 13.6, 7.0, 20, 250, w=2.3),
        line([(14.4, 8.4), (18.0, 4.8)], w=2.1),
        poly([(14.8, 2.4), (20.6, 2.4), (20.6, 8.2)], fill=True),
    ],
    # Lo stesso gesto, due volte: non si toglie uno stato, si svuota tutto.
    "RT_UI_Icon_Action_Purge": [
        arc(12, 14.6, 6.4, 20, 250, w=2.3),
        line([(13.8, 10.4), (16.6, 7.6)], w=2.0),
        poly([(13.4, 5.4), (18.6, 5.4), (18.6, 10.6)], fill=True),
        line([(7.4, 8.0), (10.2, 5.2)], w=2.0),
        poly([(7.0, 3.0), (12.2, 3.0), (12.2, 8.2)], fill=True),
    ],
    # `Effect.Interrupt`: una linea d'azione spezzata a meta'.
    "RT_UI_Icon_Action_Interrupt": [
        line([(3.2, 12.0), (9.0, 12.0)], w=2.4),
        line([(15.0, 12.0), (20.8, 12.0)], w=2.4),
        line([(13.4, 5.0), (10.6, 19.0)], w=2.6),
    ],
    # Rombo di `Status.Marked` piu' il mirino che lo assegna: azione e stato condividono il rombo.
    "RT_UI_Icon_Action_MarkTarget": [
        poly([(12.0, 5.0), (17.4, 12.0), (12.0, 19.0), (6.6, 12.0)], fill=True),
        line([(12, 2.0), (12, 4.0)], w=1.8), line([(12, 20.0), (12, 22.0)], w=1.8),
        line([(2.4, 12), (4.4, 12)], w=1.8), line([(21.6, 12), (19.6, 12)], w=1.8),
    ],
    # Identico a `Status.Root`: il payload regge in entrambi i contesti (§4.1).
    "RT_UI_Icon_Action_Root": [
        line([(12.0, 4.6), (12.0, 16.0)], w=2.8),
        line([(5.0, 18.0), (19.0, 18.0)], w=2.8),
    ],
    # Identico a `Status.Slow`: chevron che avanza e trova il freno.
    "RT_UI_Icon_Action_Slow": [
        dot(4.0, 12.0, 2.4),
        line([(8.4, 6.6), (13.6, 12.0), (8.4, 17.4)], w=2.4),
        line([(18.4, 4.8), (18.4, 19.2)], w=2.8),
    ],

    # --- AMBIENTE E MAPPA (misto) — composizione con `Cover`, `Water`, `Electric`, `Fire` ---

    # `Cover` che nasce: la massa piena di `Environment.Cover` piu' il gesto di crearla.
    "RT_UI_Icon_Action_CreateCover": [
        line([(2.6, 20.4), (21.4, 20.4)], w=1.9),
        poly([(7.0, 9.6), (17.0, 9.6), (17.0, 19.2), (7.0, 19.2)], fill=True),
        line([(12, 2.2), (12, 7.4)], w=2.4),
        line([(9.0, 5.2), (12, 2.2), (15.0, 5.2)], w=2.2),
    ],
    # La goccia di `Water` che scende su una cella.
    "RT_UI_Icon_Action_CreateWater": [
        poly([(12.0, 3.0), (15.8, 9.2), (8.2, 9.2)], fill=True),
        dot(12.0, 10.6, 3.8),
        line([(4.4, 19.6), (19.6, 19.6)], w=2.4),
    ],
    # Il fulmine di `Electric` su una cella: elettrificare una superficie.
    "RT_UI_Icon_Action_Electrify": [
        poly([(13.6, 2.4), (7.6, 11.2), (11.0, 11.2), (9.4, 17.2),
              (15.4, 8.6), (12.0, 8.6)], fill=True),
        line([(4.4, 19.8), (19.6, 19.8)], w=2.4),
    ],
    # La fiamma di `Burning` su una cella: incendiare una superficie.
    "RT_UI_Icon_Action_Ignite": [
        poly([(13.4, 2.6), (15.6, 6.6), (16.2, 10.4), (15.0, 14.2), (12.0, 16.6),
              (8.8, 14.4), (8.2, 10.6), (9.8, 7.4), (11.4, 9.4), (11.0, 5.4)], fill=True),
        line([(4.4, 19.8), (19.6, 19.8)], w=2.4),
    ],
    # Un settore che ruota: cambiare l'arco coperto. Composizione con `Geometry.Arc`.
    "RT_UI_Icon_Action_ModifyArc": [
        dot(12.0, 17.4, 2.6),
        arc(12, 17.4, 9.0, 200, 340, w=2.3),
        poly([(16.8, 4.0), (21.4, 6.6), (16.6, 9.0)], fill=True),
    ],
    # Mano che tocca un dispositivo: `Interact` assorbe anche `Activate` (D-025).
    "RT_UI_Icon_Action_Interact": [
        poly([(6.0, 6.4), (14.6, 6.4), (14.6, 17.6), (6.0, 17.6)], w=2.1),
        dot(10.3, 12.0, 2.2),
        line([(17.2, 9.4), (20.6, 9.4)], w=2.0),
        line([(17.2, 14.6), (20.6, 14.6)], w=2.0),
    ],
})

_PHASE_X = (5.0, 9.6, 14.2, 18.8)


def _phase(active: int):
    """Tre punti e una barra: la `active`-esima (0-based) e' la barra.

    Iterazione 2: quattro TACCHE di altezza diversa leggevano come un equalizzatore audio — e il
    progetto ha un overlay `Sound` con contatti acustici (progettazione-hud.md §26), quindi la
    lettura sbagliata era anche disponibile. Punti piu' barra e' invece il pattern universale
    della posizione in una sequenza, e non somiglia a nulla nel resto del set.
    """
    out = []
    for i, x in enumerate(_PHASE_X):
        if i == active:
            out.append(line([(x, 6.4), (x, 17.6)], w=3.4))
        else:
            out.append(dot(x, 12.0, 1.9))
    return out


for _i, _name in enumerate(("Prep", "Dash", "Blast", "Move")):
    ICONS[f"RT_UI_Icon_Phase_{_name}"] = _phase(_i)

BATCHES = {
    "01-difese": ["RT_UI_Icon_Action_Guard", "RT_UI_Icon_Action_Brace",
                  "RT_UI_Icon_Action_Shield", "RT_UI_Icon_Environment_Cover"],
    "02-movimento": ["RT_UI_Icon_Action_Move", "RT_UI_Icon_Action_Sprint",
                     "RT_UI_Icon_Action_Dash", "RT_Prim_Geometry_Line"],
    "03-elettricita-e-reazione": ["RT_UI_Icon_Environment_Electric", "RT_UI_Icon_Status_Electrified",
                                  "RT_UI_Icon_Reaction_Generic", "RT_UI_Icon_Reaction_Opportunity",
                                  "RT_UI_Icon_Action_Wait"],
    # Le due azioni omonime chiudono il confronto: e' l'unica collisione che questo batch rischia.
    "04-fasi": ["RT_UI_Icon_Phase_Prep", "RT_UI_Icon_Phase_Dash", "RT_UI_Icon_Phase_Blast",
                "RT_UI_Icon_Phase_Move", "RT_UI_Icon_Action_Dash", "RT_UI_Icon_Action_Move"],
    "05-status": ["RT_UI_Icon_Status_Wet", "RT_UI_Icon_Status_Burning",
                  "RT_UI_Icon_Status_Electrified", "RT_UI_Icon_Status_Braced",
                  "RT_UI_Icon_Status_Guarded", "RT_UI_Icon_Status_Exposed"],
    "05-status-bis": ["RT_UI_Icon_Status_Marked", "RT_UI_Icon_Status_Obscured",
                      "RT_UI_Icon_Status_Reveal", "RT_UI_Icon_Status_Root",
                      "RT_UI_Icon_Status_Slow", "RT_UI_Icon_MissingIcon"],
    "06-attacchi": ["RT_UI_Icon_Action_BasicAttack", "RT_UI_Icon_Action_PrecisionAttack",
                    "RT_UI_Icon_Action_HeavyAttack", "RT_UI_Icon_Action_LineAttack",
                    "RT_UI_Icon_Action_CircularAoE", "RT_UI_Icon_Action_SuppressiveLine"],
    "07-spostamento": ["RT_UI_Icon_Action_Charge", "RT_UI_Icon_Action_Leap",
                       "RT_UI_Icon_Action_Reposition", "RT_UI_Icon_Action_Push",
                       "RT_UI_Icon_Action_Pull", "RT_UI_Icon_Action_Evade"],
    "08-reazione": ["RT_UI_Icon_Action_Counter", "RT_UI_Icon_Action_Deflect",
                    "RT_UI_Icon_Action_Intercept", "RT_UI_Icon_Action_Anchor",
                    "RT_UI_Icon_Action_Root", "RT_UI_Icon_Action_Slow"],
    "09-supporto": ["RT_UI_Icon_Action_Heal", "RT_UI_Icon_Action_Cleanse",
                    "RT_UI_Icon_Action_Purge", "RT_UI_Icon_Action_Interrupt",
                    "RT_UI_Icon_Action_MarkTarget", "RT_UI_Icon_Action_Interact"],
    "10-ambiente": ["RT_UI_Icon_Action_CreateCover", "RT_UI_Icon_Action_CreateWater",
                    "RT_UI_Icon_Action_Electrify", "RT_UI_Icon_Action_Ignite",
                    "RT_UI_Icon_Action_ModifyArc"],
}


# --------------------------------------------------------------------------------------
# Backend SVG
# --------------------------------------------------------------------------------------

def to_svg(prims) -> str:
    body = []
    for p in prims:
        if p["kind"] == "polyline":
            d = "M " + " L ".join(f"{x} {y}" for x, y in p["pts"])
            body.append(
                f'  <path d="{d}" fill="none" stroke="currentColor" stroke-width="{p["w"]}" '
                f'stroke-linecap="round" stroke-linejoin="round"/>'
            )
        elif p["kind"] == "polygon":
            d = "M " + " L ".join(f"{x} {y}" for x, y in p["pts"]) + " Z"
            if p["fill"]:
                body.append(f'  <path d="{d}" fill="currentColor" stroke="none"/>')
            else:
                body.append(
                    f'  <path d="{d}" fill="none" stroke="currentColor" stroke-width="{p["w"]}" '
                    f'stroke-linejoin="round"/>'
                )
        elif p["kind"] == "dot":
            cx, cy = p["c"]
            body.append(f'  <circle cx="{cx}" cy="{cy}" r="{p["r"]}" fill="currentColor"/>')
        elif p["kind"] == "ring":
            cx, cy = p["c"]
            body.append(
                f'  <circle cx="{cx}" cy="{cy}" r="{p["r"]}" fill="none" stroke="currentColor" '
                f'stroke-width="{p["w"]}"/>'
            )
        elif p["kind"] == "arc":
            cx, cy = p["c"]
            r = p["r"]
            import math
            x0 = cx + r * math.cos(math.radians(p["a0"]))
            y0 = cy + r * math.sin(math.radians(p["a0"]))
            x1 = cx + r * math.cos(math.radians(p["a1"]))
            y1 = cy + r * math.sin(math.radians(p["a1"]))
            large = 1 if (p["a1"] - p["a0"]) % 360 > 180 else 0
            body.append(
                f'  <path d="M {x0:.3f} {y0:.3f} A {r} {r} 0 {large} 1 {x1:.3f} {y1:.3f}" '
                f'fill="none" stroke="currentColor" stroke-width="{p["w"]}" stroke-linecap="round"/>'
            )
    return (
        f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {CANVAS} {CANVAS}" '
        f'width="{CANVAS}" height="{CANVAS}" color="#FFFFFF">\n' + "\n".join(body) + "\n</svg>\n"
    )


# --------------------------------------------------------------------------------------
# Backend raster
# --------------------------------------------------------------------------------------

def render(prims, size: int) -> Image.Image:
    """Disegna a `CANVAS*SS` e riduce a `size`. Il downsample e' l'antialiasing."""
    big = CANVAS * SS
    img = Image.new("RGBA", (big, big), (255, 255, 255, 0))
    d = ImageDraw.Draw(img)
    white = (255, 255, 255, 255)

    def s(v):
        return v * SS

    for p in prims:
        if p["kind"] == "polyline":
            pts = [(s(x), s(y)) for x, y in p["pts"]]
            d.line(pts, fill=white, width=int(s(p["w"])), joint="curve")
            # `line` non arrotonda gli estremi: i cap si aggiungono a mano.
            r = s(p["w"]) / 2
            for x, y in (pts[0], pts[-1]):
                d.ellipse([x - r, y - r, x + r, y + r], fill=white)
        elif p["kind"] == "polygon":
            pts = [(s(x), s(y)) for x, y in p["pts"]]
            if p["fill"]:
                d.polygon(pts, fill=white)
            else:
                d.line(pts + [pts[0]], fill=white, width=int(s(p["w"])), joint="curve")
                r = s(p["w"]) / 2
                for x, y in pts:
                    d.ellipse([x - r, y - r, x + r, y + r], fill=white)
        elif p["kind"] == "dot":
            cx, cy = s(p["c"][0]), s(p["c"][1])
            r = s(p["r"])
            d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=white)
        elif p["kind"] == "ring":
            cx, cy = s(p["c"][0]), s(p["c"][1])
            r = s(p["r"])
            d.ellipse([cx - r, cy - r, cx + r, cy + r], outline=white, width=int(s(p["w"])))
        elif p["kind"] == "arc":
            cx, cy = s(p["c"][0]), s(p["c"][1])
            r = s(p["r"])
            d.arc([cx - r, cy - r, cx + r, cy + r], p["a0"], p["a1"],
                  fill=white, width=int(s(p["w"])))
            import math
            rr = s(p["w"]) / 2
            for a in (p["a0"], p["a1"]):
                x = cx + r * math.cos(math.radians(a))
                y = cy + r * math.sin(math.radians(a))
                d.ellipse([x - rr, y - rr, x + rr, y + rr], fill=white)

    return img.resize((size, size), Image.LANCZOS)


# --------------------------------------------------------------------------------------
# Contact sheet
# --------------------------------------------------------------------------------------

def contact_sheet(names) -> Image.Image:
    """La tavola di review: dimensioni reali su fondo HUD e su fondo chiaro.

    Serve a rispondere alla sola domanda che conta per un batch — i glifi sono distinguibili
    fra loro a 24 px, senza label e senza colore?
    """
    pad, col_w, label_h = 28, 150, 26
    rows = [("96 px", 96), ("48 px", 48), ("24 px  (dimensione reale)", 24), ("16 px", 16)]
    row_h = [max(112, 96), 64, 44, 36]

    W = pad * 2 + col_w * len(names)
    H = pad * 2 + label_h + sum(row_h) + 40 + sum(row_h[1:])
    sheet = Image.new("RGB", (W, H), (8, 15, 20))  # RT_UI_BG_Deep
    d = ImageDraw.Draw(sheet)

    for i, n in enumerate(names):
        short = (n.replace("RT_UI_Icon_", "").replace("RT_Prim_", "")
                  .replace("Action_", "").replace("Environment_", "").replace("Geometry_", ""))
        d.text((pad + i * col_w + 10, pad), short, fill=(242, 244, 247))

    y = pad + label_h
    for (lbl, px), rh in zip(rows, row_h):
        d.text((6, y + rh // 2 - 4), lbl[:7], fill=(107, 114, 128))
        for i, n in enumerate(names):
            img = render(ICONS[n], px)
            sheet.paste(img, (pad + i * col_w + (col_w - px) // 2, y + (rh - px) // 2), img)
        y += rh

    # Seconda banda: fondo chiaro. Un glifo bianco tintabile deve reggere anche invertito.
    y += 20
    d.rectangle([0, y, W, H], fill=(226, 229, 233))
    d.text((6, y + 8), "chiaro", fill=(60, 66, 74))
    y += 20
    for (lbl, px), rh in zip(rows[1:], row_h[1:]):
        for i, n in enumerate(names):
            img = render(ICONS[n], px)
            dark = Image.new("RGBA", img.size, (16, 22, 28, 255))
            dark.putalpha(img.getchannel("A"))
            sheet.paste(dark, (pad + i * col_w + (col_w - px) // 2, y + (rh - px) // 2), dark)
        y += rh

    return sheet


# --------------------------------------------------------------------------------------
# Conformita'
# --------------------------------------------------------------------------------------

SRC = Path("Source/RefactorTactics")

# Chiavi che appartengono a E25 e NON al set obbligatorio della v0.1. Vanno elencate a mano,
# perche' e' proprio l'elenco esplicito a impedire che un nome nuovo passi per distrazione.
E25_DECLARED = {
    "UI.Icon.Environment.Cover",
    "UI.Icon.Environment.Electric",
    "UI.Icon.Reaction.Generic",
    "UI.Icon.Reaction.Opportunity",
}

# Non e' un `IconId`: e' il campo `MissingIcon` del catalogo.
SPECIAL = {"RT_UI_Icon_MissingIcon"}


def required_icon_ids():
    """Ricostruisce `URTIconLibrary::RequiredIconIds()` dalle stesse fonti che legge il codice.

    E' il punto dell'intero controllo. Confrontare i file generati con i nomi di `ICONS` verifica
    solo che il generatore sia deterministico: un `Status.Rooted` scritto al posto di `Status.Root`
    passerebbe, e non risolverebbe mai a runtime. Qui il set atteso viene invece dal catalogo
    azioni, dai tag registrati e dalle fasi — cioe' da cio' che il gioco produce davvero.
    """
    import re
    cat = (SRC / "Ability/RTCatalogLibrary.cpp").read_text(encoding="utf-8", errors="ignore")
    tags = (SRC / "Core/RTGameplayTags.cpp").read_text(encoding="utf-8", errors="ignore")
    heroes_src = (SRC / "Ability/RTHeroCatalogLibrary.cpp").read_text(encoding="utf-8", errors="ignore")
    actions = set(re.findall(r'ShippedAction\(TEXT\("(Action\.[A-Za-z]+)"\)', cat))
    status = set(re.findall(r'"(Status\.[A-Za-z]+)"', tags))
    phases = {"Phase.Prep", "Phase.Dash", "Phase.Blast", "Phase.Move"}
    # `Identity` ha una fonte generativa come le altre tre (D-031, rettifica 2026-08-12). La fonte e'
    # `ShippedHeroIds()`, la STESSA che legge `URTIconLibrary::RequiredIconIds` — non i nomi delle
    # funzioni `Make*`, che coincidono con gli HeroId solo per abitudine e potrebbero divergere.
    #
    # L'`HeroId` e' `Hero.Flux`: il prefisso di dominio va tradotto in quello di catalogo, perche'
    # `Hero` non e' un valore di `ERTIconCategory` e `UI.Icon.Hero.Flux` verrebbe rifiutata.
    shipped = re.search(r'ShippedHeroIds\(\)\s*\{(.*?)\n\}', heroes_src, re.S)
    heroes = set(re.findall(r'TEXT\("Hero\.([A-Za-z]+)"\)', shipped.group(1))) if shipped else set()
    if not actions or not status or not heroes:
        raise SystemExit("Impossibile leggere azioni, tag o HeroId: lanciare dalla radice del repo.")
    return {f"UI.Icon.{x}" for x in actions | status | phases} | {f"UI.Icon.Identity.{h}" for h in heroes}


def check_conformance() -> int:
    """Rifiuta ogni `IconId` che nessuna fonte del gioco richiede. Ritorna il numero di errori."""
    required = required_icon_ids()
    produced, errors = set(), []

    for name in ICONS:
        if name in SPECIAL or not name.startswith("RT_UI_Icon_"):
            continue
        icon_id = "UI.Icon." + name[len("RT_UI_Icon_"):].replace("_", ".")
        produced.add(icon_id)
        if icon_id not in required and icon_id not in E25_DECLARED:
            errors.append(f"'{icon_id}': nessuna fonte lo richiede, e non e' dichiarato E25")

    if "RT_UI_Icon_MissingIcon" not in ICONS:
        errors.append("MissingIcon assente: il validator rifiuta un catalogo che non la dichiara")

    covered = produced & required
    print(f"conformita': {len(produced)} IconId prodotti, {len(errors)} non ammessi")
    print(f"copertura v0.1: {len(covered)}/{len(required)}"
          f"  ({100 * len(covered) // len(required)}%)   +{len(produced & E25_DECLARED)} E25")
    for e in errors:
        print("   ERRORE:", e)
    return len(errors)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true",
                    help="non scrive: esce 1 se un file manca o e' diverso")
    args = ap.parse_args()

    stale = []
    for name, prims in ICONS.items():
        # Le primitive di grammatica non si risolvono a runtime: nessun widget potra' mai
        # chiederle. Stanno in `calibration/`, non fra gli asset spedibili, altrimenti sono
        # esattamente il dato che nessuno legge.
        root = OUT / ("calibration" if name.startswith("RT_Prim_") else "")
        targets = [(root / "svg" / f"{name}.svg", to_svg(prims).encode("utf-8"))]
        for px in PNG_SIZES:
            p = root / "png" / f"{px}" / f"{name}.png"
            import io
            buf = io.BytesIO()
            render(prims, px).save(buf, "PNG")
            targets.append((p, buf.getvalue()))

        for path, data in targets:
            if args.check:
                if not path.exists() or path.read_bytes() != data:
                    stale.append(str(path))
            else:
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(data)

    for batch, names in BATCHES.items():
        sheet_path = OUT / f"calibration-batch-{batch}.png"
        if not args.check:
            sheet_path.parent.mkdir(parents=True, exist_ok=True)
            contact_sheet(names).save(sheet_path)

    bad = check_conformance()

    if args.check:
        if stale:
            print(f"NON aggiornati ({len(stale)}):")
            for s in stale[:10]:
                print("  ", s)
        if stale or bad:
            return 1
        print(f"OK - {len(ICONS)} icone, {len(PNG_SIZES)} dimensioni, tutto aggiornato e conforme.")
        return 0

    if bad:
        return 1
    print(f"Scritte {len(ICONS)} icone x ({len(PNG_SIZES)} PNG + 1 SVG) -> {OUT}")
    print(f"Contact sheet -> {sheet_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
