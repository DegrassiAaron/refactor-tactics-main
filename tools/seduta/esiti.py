#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Deriva gli esiti MECCANICI di una seduta dal log e dal CSV di pacing, invece di greppare a mano.

    python tools/seduta/esiti.py <log> [--csv <pacing.csv>] [--crashes <dir>]
    python tools/seduta/esiti.py Saved/Logs/g13-seduta.log --csv Saved/.../pacing_20261003-120000.csv

Perche' esiste
--------------
I verdetti meccanici di `G13` si leggono dal log e dal CSV, e finora si greppavano a mano. Questo
repository ha una storia documentata di pattern CIECHI: un grep che non puo' colpire rende un gate
verde perche' **non guarda**, e un verde ottenuto cosi' e' indistinguibile da un verde vero.

Tre cose che questo script fa e che un grep a mano non fa:

1. **Tre reti sui crash, e dichiara QUALE ha colpito.** La cella `G2` del DoD misura il caso: su un
   log con GPU crash, `Critical error`, `Fatal error` e `Assertion failed` rispondono **0 tutti e
   tre**. Le reti che lo prendono sono `Error:` e una cartella nuova sotto `Saved/Crashes/`.
   Un solo pattern non copre la classe, e sapere quale ha colpito cambia la diagnosi.

2. **Un CANCELLO POSITIVO prima di tutto.** Gli esiti negativi attendono zero: una seduta che non
   parte li soddisfa **tutti**. Senza un testimone che la partita sia avvenuta, quegli zeri sono
   un'assenza e non una misura, e lo script rifiuta di emettere verdetti.

3. **Un CONTROLLO POSITIVO su ogni pattern.** Uno zero non distingue *«non c'e'»* da *«non ho
   cercato bene»*. Ogni ricerca negativa gira accanto a una ricerca che **deve** colpire su quello
   stesso file: se il controllo non colpisce, il verdetto non e' `PASS`, e' `PATTERN ROTTO`.

Cosa NON fa
-----------
Non da' il verdetto del gate. Le due domande percettive di `G13` - che l'interfaccia sia leggibile e
che il giocatore TROVI l'affordance - non stanno in nessun file, e questo script non le simula.
`NOT RUN` non equivale a `PASS`.
"""

import argparse
import csv
import io
import os
import sys

csv.field_size_limit(min(sys.maxsize, 2**31 - 1))

PASS, FAIL, NOTRUN, ROTTO = "PASS", "FAIL", "NOT RUN", "PATTERN ROTTO"

# Il token che DEVE esserci in qualunque log di un processo Unreal avviato: e' il controllo positivo
# di ogni ricerca negativa su questo file. Se manca, il file non e' un log di avvio e i conteggi a
# zero non significano niente.
CONTROLLO_LOG = "LogInit"

# Su un log di EDITOR la rete larga `Error:` parte da 2 anche a run pulita: l'Engine esercita il
# proprio sistema di errori all'inizializzazione. Misurato il 2026-10-04 su due log indipendenti
# (2 e 2); sui log di pacchetto il conteggio e' 0, quindi nessuno sconto si applica la'.
PAVIMENTO_EDITOR = 2

# Il cancello: la partita e' davvero avvenuta? Il primo e' quello che il log di una partita 2v2
# emette all'allestimento; gli altri due dicono che la partita e' ARRIVATA A UN ESITO.
CANCELLO = [
    ("allestimento", "Board 2v2 esagonale avviata"),
    ("fine partita (frontend)", "Fine partita al round"),
    ("esito deciso (simulazione)", "Partita finita:"),
]

# Gli esiti negativi. Ogni voce: etichetta, lista di pattern alternativi, nota.
NEGATIVI = [
    ("nessun crash", ["Fatal error", "Assertion failed", "Critical error"],
     "rete storica: CIECA ai GPU crash, vedi 'rete larga'"),
    ("nessun crash (rete larga)", ["Error:"],
     "prende la classe che la rete storica non vede. ATTENZIONE AL PAVIMENTO: su un log di EDITOR "
     "vale 2 anche a run pulita, ed e' l'Engine che esercita il proprio sistema di errori "
     "all'inizializzazione (`UE::UnifiedErrorTest`, poi due `LogAutomationTest: Error: Condition "
     "failed`). Misurato il 2026-10-04 su due log di Editor indipendenti: 2 e 2; sui log di "
     "PACCHETTO: 0. Quindi su un log di Editor i primi due non sono tuoi"),
    ("nessun GPU crash", ["TerminateOnGPUCrash", "DXGI_ERROR_DEVICE_REMOVED"],
     "terza rete, ortogonale alle due sopra"),
    ("nessun asset mancante", ["Failed to find object"],
     "si LEGGE, non si conta: il Warning e' emesso una volta per processo e poi soppresso"),
    ("nessun overlay di debug", ["DrawCells", "DrawIntent", "ShowDebug", "DebugDraw"], ""),
    ("nessun travel failure", ["Travel Failure"], ""),
]

# Le colonne del CSV che il criterio di G13 nomina. Si cercano per NOME nell'intestazione:
# l'indice non e' stabile, e un indice cablato misura un'altra serie.
COLONNE = ["SelectionCount", "OrderCount", "UndoCount"]


def leggi(path):
    with io.open(path, encoding="utf-8", errors="replace") as f:
        return f.read()


def conta(testo, pattern):
    return testo.count(pattern)


def esiti_log(path):
    """Ritorna (righe, valido) dove valido dice se i verdetti si possono leggere."""
    righe = []
    if not path or not os.path.isfile(path):
        return [("log", NOTRUN, "file assente: " + str(path), "")], False
    testo = leggi(path)

    # --- controllo positivo del file ---
    n_ctrl = conta(testo, CONTROLLO_LOG)
    if n_ctrl == 0:
        righe.append(("controllo del file", ROTTO,
                      "'%s' non compare: questo non e' un log di avvio, e i conteggi a zero qui "
                      "non significano niente" % CONTROLLO_LOG, ""))
        return righe, False
    righe.append(("controllo del file", PASS, "'%s' x%d" % (CONTROLLO_LOG, n_ctrl),
                  "ogni zero qui sotto e' stato cercato in un file che sa rispondere"))

    # --- il cancello positivo ---
    colpiti = [(et, conta(testo, p)) for et, p in CANCELLO]
    aperto = any(n > 0 for _, n in colpiti)
    dettaglio = " | ".join("%s=%d" % (et, n) for et, n in colpiti)
    righe.append(("CANCELLO: la partita e' avvenuta", PASS if aperto else FAIL, dettaglio,
                  "almeno uno > 0" if aperto else
                  "nessun testimone: gli esiti negativi sotto sarebbero un'ASSENZA, non una misura"))

    # Il log e' di un EDITOR o di un pacchetto? Lo dice un token dell'Engine che il pacchetto non ha.
    e_editor = conta(testo, 'UnifiedErrorTest') > 0
    if e_editor:
        righe.append(("tipo di log", PASS, "EDITOR (`UnifiedErrorTest` presente)",
                      "la rete larga `Error:` ha qui un pavimento di 2 dall'Engine, e viene scontato"))

    # --- gli esiti negativi ---
    for etichetta, pattern, nota in NEGATIVI:
        per_pattern = [(p, conta(testo, p)) for p in pattern]
        tot = sum(n for _, n in per_pattern)
        if e_editor and pattern == ["Error:"]:
            # Si scontano i due dell'Engine, e si DICHIARA di averlo fatto: scontare in silenzio
            # sarebbe la cecita' che questo script esiste per non avere.
            tot = max(0, tot - PAVIMENTO_EDITOR)
            per_pattern = [("Error:", tot)] if tot else []
        quali = ", ".join("%s=%d" % (p, n) for p, n in per_pattern if n)
        righe.append((etichetta,
                      PASS if tot == 0 else FAIL,
                      "0" if tot == 0 else quali,   # <-- dichiara QUALE rete ha colpito
                      nota))
    return righe, aperto


def esiti_csv(path):
    if not path:
        return [("CSV di pacing", NOTRUN, "non fornito (--csv)",
                 "i tre congiunti di input restano NOT RUN, che non e' PASS")]
    if not os.path.isfile(path):
        return [("CSV di pacing", NOTRUN, "file assente: " + path, "")]
    with io.open(path, encoding="utf-8", errors="replace", newline="") as f:
        r = csv.reader(f)
        try:
            head = next(r)
        except StopIteration:
            return [("CSV di pacing", FAIL, "file vuoto", "")]
        idx = {}
        for nome in COLONNE:
            trovati = [i for i, h in enumerate(head) if h.strip() == nome]
            if len(trovati) != 1:
                return [("CSV di pacing", ROTTO,
                         "colonna '%s' trovata %d volte. Intestazione: %s"
                         % (nome, len(trovati), ",".join(head)),
                         "le colonne si cercano per nome: un indice cablato misura un'altra serie")]
            idx[nome] = trovati[0]
        turni, scartate = [], 0
        for row in r:
            if not row:
                continue
            if row[0].strip().upper() == "EVENTS" or row[0].strip().startswith("["):
                scartate += 1
                continue
            try:
                turni.append({n: int(row[idx[n]]) for n in COLONNE})
            except (ValueError, IndexError):
                scartate += 1
    if not turni:
        return [("CSV di pacing", FAIL, "nessuna riga di turno (scartate %d)" % scartate,
                 "il file esiste ma non porta turni")]
    buoni = [t for t in turni if all(t[n] >= 1 for n in COLONNE)]
    righe = [("CSV: colonne per nome", PASS,
              ", ".join("%s=col.%d" % (n, idx[n] + 1) for n in COLONNE),
              "cercate per nome nell'intestazione, non per indice")]
    righe.append(("CSV: turni letti", PASS, "%d turni (righe scartate: %d)" % (len(turni), scartate), ""))
    massimi = ", ".join("%s max=%d" % (n, max(t[n] for t in turni)) for n in COLONNE)
    righe.append(("input funzionante (>=1 su un turno)",
                  PASS if buoni else FAIL,
                  "%d turni soddisfano tutti e tre | %s" % (len(buoni), massimi),
                  "" if buoni else
                  "ATTENZIONE: UndoCount e' l'unico legato al TASTO DESTRO, e BackSpace lo "
                  "soddisfa mancando il difetto per cui il congiunto esiste"))
    return righe


def esiti_crashes(d):
    if not d:
        return [("cartelle di crash", NOTRUN, "non fornita (--crashes)",
                 "e' la terza rete: una cartella NUOVA dopo la run")]
    if not os.path.isdir(d):
        return [("cartelle di crash", PASS, "la cartella non esiste: nessun crash ha prodotto artefatti", "")]
    sotto = sorted(x for x in os.listdir(d) if os.path.isdir(os.path.join(d, x)))
    return [("cartelle di crash", PASS if not sotto else FAIL,
             "%d: %s" % (len(sotto), ", ".join(sotto[:4]) + (" ..." if len(sotto) > 4 else "")),
             "" if not sotto else
             "fotografa questa cartella PRIMA della seduta: le preesistenti non sono tue")]


def main():
    ap = argparse.ArgumentParser(description="Esiti meccanici di una seduta, dal log e dal CSV.")
    ap.add_argument("log", help="il log di -abslog della seduta")
    ap.add_argument("--csv", help="il CSV di pacing prodotto dalla seduta")
    ap.add_argument("--crashes", help="la cartella Saved/Crashes del processo")
    a = ap.parse_args()

    righe, valido = esiti_log(a.log)
    righe += esiti_csv(a.csv)
    righe += esiti_crashes(a.crashes)

    w = max(len(r[0]) for r in righe)
    print()
    for et, verdetto, misura, nota in righe:
        print("  %-*s  %-13s %s" % (w, et, verdetto, misura))
        if nota:
            print("  %-*s  %-13s   ^ %s" % (w, "", "", nota))
    print()
    if not valido:
        print("  [STOP] IL CANCELLO NON E' APERTO: gli esiti negativi non si leggono come PASS.")
        print("     Senza un testimone che la partita sia avvenuta, uno zero e' un'assenza.")
        return 2
    rotti = [r for r in righe if r[1] == ROTTO]
    if rotti:
        print("  [STOP] PATTERN ROTTO: il verdetto non si emette.")
        return 3
    falliti = [r for r in righe if r[1] == FAIL]
    nonrun = [r for r in righe if r[1] == NOTRUN]
    if falliti:
        print("  [FAIL] %d esiti FAIL: %s" % (len(falliti), ", ".join(r[0] for r in falliti)))
        return 1
    if nonrun:
        print("  [ATTESA] Nessun FAIL, ma %d NOT RUN: %s" % (len(nonrun), ", ".join(r[0] for r in nonrun)))
        print("     NOT RUN non equivale a PASS. Le due domande percettive restano all'occhio.")
        return 0
    print("  [OK] Tutti gli esiti meccanici sono PASS. Le due domande percettive restano all'occhio.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
