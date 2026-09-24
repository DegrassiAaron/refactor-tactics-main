# -*- coding: utf-8 -*-
"""Le misure di E50, con il comando che le genera — e un asse che l'audit non aveva.

    Uso:  python tools/architettura/misure-strutturali.py [--ref <git-ref>] [--base <git-ref>]
                                                          [--churn] [--check] [--soglia N]
                                                          [--markdown] [--json <file>]

Senza argomenti misura l'albero di lavoro. Con `--base` stampa il confronto fra due commit, che e'
la forma in cui #1818 chiede di riportare ogni fetta. Con `--check` fa il gate di non-regressione
di E50 contro il merge-base con `origin/main`.

## `--check`: il gate che E50 non aveva

⚠️ **Ordina, non decide.** Esce 1 quando una grandezza sorvegliata e' cresciuta, e quell'1 non e' un
fallimento: e' «dichiaralo nella PR». Una feature che aggiunge un metodo al TurnManager e' legittima —
quella che lo aggiunge *in silenzio* fa salire il fondo mentre ogni singola fetta di #1818 riesce.
Nei quattro giorni dell'audit e' successo esattamente questo: **+395 righe di codice e +10 metodi**,
non per colpa delle fette (che registrano anche i saldi sfavorevoli) ma per i **53 commit** di gameplay
ordinario atterrati sulla stessa classe, perche' e' li' che vive il sequenziamento.

Sorveglia **metodi** e **righe di codice**, e non le righe del file: vedi la sezione qui sotto — un gate
che sale quando qualcuno scrive la ragione di una regola insegna a non scriverla. Il metodo e' il segnale
piu' pulito dei due, perche' un metodo nuovo e' una decisione, non un dettaglio di formattazione.

Guarda il solo `ARTTurnManager`, che e' l'oggetto di #1818. Non dice nulla del resto del modulo.

## Cosa NON copre

⛔ **Non e' un veto, e il suo 1 non e' un fallimento**: e' «dichiaralo nella PR». Una crescita
motivata va bene; una silenziosa e' cio' che E50 non puo' vedere.

⛔ **Guarda il solo `ARTTurnManager`** — i tre file di #1818 — e **non dice nulla del resto del
modulo**. Una responsabilita' spostata dal TurnManager a una classe nuova risulta qui come una
**discesa**, ed e' esattamente cio' che #1818 chiede: il gate misura dove il grasso si accumula, non
dove e' andato.

⛔ **Sorveglia due grandezze, non la qualita'.** Metodi e righe di codice: un metodo lungo e contorto
conta uno come un metodo breve, e una riga di commento non conta affatto — vedi piu' sotto, un gate
che sale quando qualcuno scrive la ragione di una regola insegna a non scriverla.

⛔ **Non confronta alberi disomogenei senza dirlo**: se il base non ha gli stessi tre file, il delta
include un file comparso, e il gate lo stampa prima dei numeri.

⛔ **Non dice se i test siano verdi.** Non li esegue.

## Perche' esiste

L'audit del 2026-08-30 (`docs/roadmap/plans/architecture-hardening-spec-panel-2026-08-30.md`) chiude
dicendo che la metrica che conta per E50 non e' il punteggio SOLID: e' **quanti test hanno bisogno di un
mondo**, «74 su 153», e che «va rimisurata a ogni fetta e riportata in #1818».

Il numero c'e'. Il **comando** che lo produce no. Rimisurando su `b45c314b` — lo stesso commit dell'audit —
il denominatore torna esatto (153 file di test), i numeratori no: `SpawnActor` da' 67, `UWorld` da' 80,
`ARTTurnManager` da' 62 contro i 66 dichiarati. Otto varianti provate, nessuna riproduce 74 e 66.

Non e' un errore di chi ha misurato: e' che una misura senza il suo comando **non e' confrontabile con la
prossima**. Il delta fra due misure prese con due grep leggermente diversi e' rumore che sembra un
progresso. Questo file fissa i comandi, cosi' che «prima» e «dopo» misurino la stessa cosa.

E' lo stesso difetto che #1818 ha gia' trovato da se' nel commento del 2026-08-31 — «l'audit dichiara 19
`GetAllActorsOfClass`, le chiamate vere sono 11, le altre 8 sono in commento» — applicato alle metriche
rimaste.

## 🔴 LE RIGHE DEL FILE NON SONO LE RIGHE DEL CODICE

Misurato su `b45c314b..bbb20f16` (2026-08-30 -> 2026-09-03), sui tre file di `ARTTurnManager`:

    righe aggiunte      1 616
      commento            949   (58 %)
      vuote               103   ( 6 %)
      codice              564   (34 %)

**Il 58 % della crescita e' documentazione.** In questo repository il commento accanto al codice cita le
`D-nnn` e porta la ragione della regola: e' un bene, non debito. Ma la metrica «righe del file» — quella
che l'Epic usa per dire `10 412` — sale allo stesso modo se aggiungi una regola o se spieghi quella che
c'e' gia'. Una fetta che toglie 100 righe di codice e ne aggiunge 150 di spiegazione **peggiora il
numero** mentre migliora il codice.

Per questo ogni misura di ampiezza viene data due volte: `righe` e `righe di codice`. Chi confronta due
fette guardi la seconda.

## L'asse che l'audit non aveva: la lunghezza delle funzioni

L'audit del 2026-08-30 ha misurato l'**isolamento** (zero `GetWorld` nelle library di regole) e
l'**ampiezza** (righe e metodi per classe). Non ha misurato la **complessita' interna**: una funzione pura
puo' essere perfettamente isolata e lunga 249 righe. `URTHexBotLibrary::ScorePlan` lo e'.

Sul modulo, 91 funzioni su 1 643 superano le 100 righe e contengono il **32 %** di tutto il codice di
produzione. 31 stanno in Actor — dove il principio #1 di `architettura-codice.md` dice che la matematica
non dovrebbe esserci — e 28 in library gia' dichiarate a posto dall'audit.

⚠️ **La lunghezza da sola produce falsi positivi.** `URTCatalogLibrary::GetCoreActionCatalog` misura 589
righe e sarebbe il quarto imputato di qualunque classifica: contiene **4 rami** in 589 righe, perche' e'
una tabella di dati con la ragione di ogni voce scritta accanto. Spezzarla non toglierebbe complessita':
sposterebbe dati. Per questo la classifica riporta `rami` accanto a `righe`, e chi la legge scarta le voci
a densita' di rami quasi nulla prima di farne una worklist.

## `--churn`: una funzione lunga che nessuno tocca non costa niente

`git log -L a,b:file` segue il movimento delle righe, quindi conta i commit che hanno toccato *quella
funzione*, non il file che la contiene. Il prodotto `righe x commit` riordina la classifica, e l'ordine
cambia:

    ResolveCombatPasses    888 righe   103 commit   costo  91 464
    PlanBots               958 righe    50 commit   costo  47 900
    ApplyDisplacements     410 righe     9 commit   costo   3 690

`PlanBots` e' la piu' lunga, `ResolveCombatPasses` e' quella che si paga: 103 commit in 34 giorni, tre al
giorno, perche' ogni feature di combattimento le passa dentro. `ApplyDisplacements` e' grande e ferma.

Il churn conferma anche il falso positivo da un secondo lato: `GetCoreActionCatalog` ha **42 commit** con
**zero rami**. Non e' logica che cambia, e' una tabella a cui si aggiungono voci.

⚠️ La storia del repository e' di **34 giorni** (primo commit 2026-08-01). E' tutto il churn che esiste, ma
e' un campione corto: una funzione toccata 9 volte puo' essere ferma o solo non ancora arrivata al suo
turno.

## Cosa NON misura

- **la complessita' ciclomatica reale**: `rami` conta le parole chiave, non i cammini.
  🔴 **E non vede i ternari ne' `&&`/`||`**, che e' la sintassi in cui le decisioni di
  presentazione sono quasi sempre scritte: `bOwn ? ciano : giallo`. Una fetta che porta fuori tre
  decisioni scritte cosi' lascia `rami` **invariato** — misurato su #2184, fetta 3: `35 -> 35`.
  Per questo accanto a `rami` c'e' ora **`scelte`**, che conta anche ternari e corto circuito (e che
  toglie i letterali di stringa prima di contare, perche' un `?` in un testo non e' una decisione).
  ⚠️ `rami` NON e' stato cambiato: baseline gia' dichiarate in #1818, #1816 e nei referti datati
  continuano a valere. Chi apre una fetta di presentazione guardi `scelte`; chi confronta con una
  baseline vecchia guardi `rami`;
- **il flusso raggiunto attraverso una FIXTURE**, e anche qui la correzione e' additiva.
  🔴 `PATTERNS_FLUSSO` cerca cinque stringhe nel testo del `.cpp`. Ma `RTWorldFixtures::PlayOneTurn`
  — `PlanBotsForTest()` + `LockInAndResolve()` + un ciclo `Tick(0.05f)` — vive in un **header**:
  chi la chiama non contiene nessun marcatore e risulta **allestimento** pur girando la risoluzione
  vera. Misurato il 2026-09-24: **quattro** file su 37 (`RTAutobattleInputInertTests`,
  `RTHexOccupancyTests`, `RTReplayRecordingIntegrationTests`, `RTUnitIdentityTests`), e il piu'
  pesante gioca una partita fino a `MatchEnded` verificando il manifest col checksum.
  Accanto a `di cui ALLESTISCONO soltanto` c'e' ora **`— e al NETTO delle fixture`**, che risolve
  le fixture di header. ⚠️ Il primo numero NON e' cambiato, per la stessa ragione di `rami`:
  #2182 e i suoi referti lo portano scritto. Chi cerca un bersaglio per una fetta guardi il
  **netto**; chi confronta col delta di una fetta precedente guardi il primo;
- **se una fetta e' una buona idea**: dice dov'e' il peso, non cosa farne.
"""

import argparse
import json
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

# -- I comandi, dichiarati una volta sola --------------------------------------
# Cambiare uno di questi invalida il confronto con le misure precedenti: se lo fai,
# scrivilo nel commento della issue insieme al numero nuovo.

TURNMANAGER_FILES = [
    "Source/RefactorTactics/Turn/RTTurnManager.h",
    "Source/RefactorTactics/Turn/RTTurnManager.cpp",
    "Source/RefactorTactics/Turn/RTTurnManager_Blast.cpp",
]
PATTERN_MONDO = "SpawnActor"          # un test che spawna un Actor ha bisogno di un mondo

# I segni che un test gira il FLUSSO REALE del turno, e non solo lo allestisce.
#
# 🔑 **Esistono perche' «quota test con mondo» da sola non sa distinguere due cose opposte** (#2182). Un
# test che spawna un `ARTTurnManager` e gli fa risolvere tre turni ha bisogno del mondo per la ragione
# giusta: verifica il comportamento del turno, e togliergli il mondo sarebbe perdere copertura
# d'integrazione, non debito. Un test che spawna un GameMode per leggere una precedenza ha il mondo come
# impalcatura. Il pavimento della metrica e' quindi il primo gruppo, non zero — e senza questa riga il
# residuo su cui si puo' davvero lavorare non e' misurabile, cioe' va ricontato a mano a ogni fetta.
#
# ⚠️ **L'elenco e' una EURISTICA dichiarata, non una verita'.** Sovrastima chi nomina una di queste parole
# in un commento e sottostima chi fa girare il turno per una strada che non le usa. Cambiarlo invalida il
# confronto con le misure precedenti: se lo fai, scrivilo nella issue insieme al numero nuovo.
PATTERNS_FLUSSO = (
    "RunTurn",              # il pompaggio diretto di un turno
    "LockInAndResolve",     # la risoluzione vera, dal lock-in in giu'
    "->Tick(",              # chi fa avanzare il turn manager a mano
    "URTScenarioRunner::",  # chi passa dall'harness, che il flusso lo gira per intero
    "FRTScenarioSession",   # idem, un gradino piu' in basso
)

# 🔴 **L'euristica qui sopra SOTTOSTIMA il flusso, e di quanto si misura** (#2182, 2026-09-24).
#
# Cerca le cinque stringhe nel testo del `.cpp`. Ma `RTWorldFixtures::PlayOneTurn` — che e'
# `PlanBotsForTest()` + `LockInAndResolve()` + un ciclo `Tick(0.05f)`, cioe' il flusso reale — vive in un
# **header**, quindi chi la chiama non contiene nessuno dei marcatori e viene contato come allestimento.
#
# Il comando che trova i file classificati male:
#
#     for f in Source/RefactorTactics/Tests/*.cpp; do
#       grep -q "SpawnActor" "$f" \
#         && ! grep -qE "RunTurn|LockInAndResolve|->Tick\(|URTScenarioRunner::|FRTScenarioSession" "$f" \
#         && grep -qE "RunScenarioIsolated|PlayOneTurn" "$f" && basename "$f"; done
#
# ⛔ **La correzione e' ADDITIVA, ed e' deliberato**: `test_file_allestimento` resta calcolato come
# sempre, perche' #2182, #1818 e ogni referto datato portano baseline scritte con quel numero — e un
# numero che cambia sotto i piedi rende incomparabili i delta gia' pubblicati. E' la stessa scelta che
# `#2349` ha fatto per i ternari: e' nata la colonna `scelte` e `rami` e' rimasto intatto.
#
# Nasce quindi `test_file_allestimento_netto`, che toglie chi gira il flusso **attraverso una fixture**.
RE_FIXTURE_INLINE = re.compile(r"^\s*inline\s+[\w:<>,\s\*&]*?\b(\w+)\s*\(")
PATTERN_TURNMANAGER = "ARTTurnManager"
RE_TEST_NAME = re.compile(r'"RefactorTactics\.[A-Za-z0-9_.]+"')
RE_METODO_TM = re.compile(r"ARTTurnManager::[A-Za-z0-9_~]+")
RE_COMMENTO = re.compile(r"^\s*(//|/\*|\*)")
RE_RAMO = re.compile(r"\b(if|for|while|switch|else)\b")
# Ogni punto in cui il controllo O UN VALORE dipende da una condizione: le parole chiave di `RE_RAMO`
# piu' il ternario e i due operatori di corto circuito. Vedi `scelte` in "Cosa NON misura".
RE_SCELTA = re.compile(r"\b(if|for|while|switch|else)\b|\?|&&|\|\|")
# I letterali di stringa si tolgono PRIMA di contare `scelte`, perche' un `?` dentro un testo italiano
# ("Vuoi uscire?") non e' una decisione. `RE_RAMO` resta senza questo filtro di proposito: cambiarlo
# muoverebbe numeri gia' dichiarati in baseline altrui.
RE_STRINGA = re.compile(r'"(?:[^"\\]|\\.)*"')
RE_FIRMA = re.compile(r"([A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z0-9_~]+)?)\s*\(")
RE_NON_FUNZIONE = re.compile(r"\b(namespace|class|struct|enum|union)\b")
NON_FUNZIONI = {"if", "for", "while", "switch", "return", "sizeof", "catch", "do"}


def leggi(path):
    return path.read_text(encoding="utf-8", errors="replace").splitlines()


def conta_righe(righe):
    """(totali, codice) — codice = ne' commento ne' vuote. Vedi la docstring, sezione righe."""
    commento = sum(1 for r in righe if RE_COMMENTO.match(r))
    vuote = sum(1 for r in righe if not r.strip())
    return len(righe), len(righe) - commento - vuote


def spoglia(riga):
    riga = re.sub(r"\\.", "", riga)
    riga = re.sub(r'"[^"]*"', "", riga)
    riga = re.sub(r"'[^']*'", "", riga)
    return re.sub(r"//.*$", "", riga)


def fixture_di_flusso(testo):
    """PURA. I nomi delle funzioni `inline` di un header il cui CORPO gira il flusso del turno (#2182).

    Serve a chiudere il punto cieco di `PATTERNS_FLUSSO`: quelle cinque stringhe si cercano nel `.cpp`,
    ma una fixture come `RTWorldFixtures::PlayOneTurn` le contiene nell'**header**, e chi la chiama
    risulta allestimento pur girando la risoluzione vera.

    ⚠️ **Anche questa e' un'euristica**, e i suoi limiti sono due:

    * riconosce solo le funzioni che cominciano con `inline` a inizio riga — la forma che `Tests/*.h`
      usa, non l'unica possibile;
    * non segue le chiamate a catena: una fixture che ne chiami un'altra senza nominare un marcatore
      non viene riconosciuta.

    ⛔ **Le righe di commento non contano.** Un marcatore *citato* in una nota — e in questo repository
    le note citano spesso il codice che spiegano — non e' una chiamata, ed e' lo stesso falso positivo
    che `PATTERNS_FLUSSO` ha gia' sul lato `.cpp`.
    """
    nomi = set()
    corrente = None
    profondita = 0
    aperta = False
    for riga in testo.split("\n"):
        if corrente is None:
            trovata = RE_FIXTURE_INLINE.match(riga)
            if not trovata:
                continue
            corrente, profondita, aperta = trovata.group(1), 0, False
        # ⛔ Due forme di commento, e servono entrambe: `RE_COMMENTO` toglie le righe che sono commento
        # per intero (`* …` dentro un blocco `/** */`), `spoglia` toglie quello IN LINEA dopo il codice.
        # La prima stesura usava solo la prima, e l'autotest l'ha bocciata su `X.Num(); // niente RunTurn`.
        if not RE_COMMENTO.match(riga) and any(p in spoglia(riga) for p in PATTERNS_FLUSSO):
            nomi.add(corrente)
        profondita += riga.count("{") - riga.count("}")
        if "{" in riga:
            aperta = True
        # ⛔ **Una DICHIARAZIONE non apre nessun corpo, e va chiusa qui.** `inline void Helper(int32);`
        # non contiene mai `{`: senza questa riga `corrente` resterebbe agganciata a lei per sempre, la
        # fixture successiva verrebbe saltata — il blocco d'ingresso vuole `corrente is None` — e ogni
        # marcatore del resto del file finirebbe attribuito al nome sbagliato. Trovato in code review
        # riproducendolo, e ora c'e' un caso che lo tiene fermo.
        elif not aperta and ";" in spoglia(riga):
            corrente = None
            continue
        if aperta and profondita <= 0:
            corrente = None
    return nomi


def gira_il_flusso(testo, fixture):
    """PURA. Il file gira il flusso del turno — direttamente, o attraverso una fixture di header?"""
    if any(p in testo for p in PATTERNS_FLUSSO):
        return True
    return any((f + "(") in testo for f in fixture)


def funzioni_di(path):
    """Estensione di ogni funzione contando le graffe. Restituisce (righe, rami, nome, riga_inizio)."""
    testo = leggi(path)
    profondita = 0
    in_commento = False
    stack = []
    esiti = []
    recenti = []
    for i, grezza in enumerate(testo, 1):
        riga = grezza
        if in_commento:
            if "*/" in riga:
                riga = riga.split("*/", 1)[1]
                in_commento = False
            else:
                continue
        while "/*" in riga:
            prima, resto = riga.split("/*", 1)
            if "*/" in resto:
                riga = prima + resto.split("*/", 1)[1]
            else:
                riga = prima
                in_commento = True
                break
        pulita = spoglia(riga)
        apre = pulita.count("{")
        chiude = pulita.count("}")
        if apre > 0 and not stack:
            ctx = " ".join(recenti[-3:] + [pulita])
            if not RE_NON_FUNZIONE.search(ctx):
                cand = [c for c in RE_FIRMA.findall(ctx) if c not in NON_FUNZIONI]
                if cand:
                    stack.append([cand[-1], i, profondita, 0, 0])
        if stack:
            stack[-1][3] += len(RE_RAMO.findall(pulita))
            stack[-1][4] += len(RE_SCELTA.findall(RE_STRINGA.sub('""', pulita)))
        profondita += apre - chiude
        if stack and profondita <= stack[-1][2]:
            nome, inizio, _, rami, scelte = stack.pop()
            esiti.append((i - inizio + 1, rami, scelte, nome, inizio))
        if pulita.strip():
            recenti.append(pulita)
            if len(recenti) > 4:
                recenti.pop(0)
    return esiti


def contenitore(nome):
    cls = nome.split("::")[0] if "::" in nome else ""
    if cls.startswith("A"):
        return "Actor (A*)"
    if cls.endswith("Library"):
        return "Function Library (pura)"
    if cls.startswith("U"):
        return "UObject/Asset (U*)"
    if cls.startswith("F"):
        return "struct/classe pura (F*)"
    if cls.startswith("S"):
        return "Slate widget (S*)"
    if not cls:
        return "funzione libera / ns anonimo"
    return "altro"


def misura(radice, soglia):
    m = {"soglia": soglia}

    tot = 0
    cod = 0
    m["turnmanager_file"] = {}
    for rel in TURNMANAGER_FILES:
        p = radice / rel
        if not p.exists():
            continue
        t, c = conta_righe(leggi(p))
        m["turnmanager_file"][Path(rel).name] = {"righe": t, "righe_codice": c}
        tot += t
        cod += c
    m["turnmanager_righe"] = tot
    m["turnmanager_righe_codice"] = cod

    metodi = set()
    for p in (radice / "Source/RefactorTactics/Turn").glob("*.cpp"):
        metodi.update(RE_METODO_TM.findall("\n".join(leggi(p))))
    m["turnmanager_metodi"] = len(metodi)

    # Le fixture di header che girano il flusso: si scoprono una volta e valgono per tutti i file.
    fixture = set()
    for p in sorted((radice / "Source/RefactorTactics/Tests").glob("*.h")):
        fixture |= fixture_di_flusso("\n".join(leggi(p)))

    test_file = sorted((radice / "Source/RefactorTactics/Tests").glob("*.cpp"))
    nomi = set()
    con_mondo = 0
    con_tm = 0
    allestimento = 0
    allestimento_netto = 0
    for p in test_file:
        testo = "\n".join(leggi(p))
        nomi.update(RE_TEST_NAME.findall(testo))
        if PATTERN_MONDO in testo:
            con_mondo += 1
            # Allestimento = spawna un Actor ma NON gira il flusso del turno. E' il residuo di E50 su cui
            # una fetta puo' lavorare senza togliere copertura d'integrazione (#2182).
            #
            # ⛔ **Questa riga NON cambia**, e il numero che produce nemmeno: le baseline di #2182, #1818 e
            # dei referti datati sono scritte con lui. Il conteggio corretto vive accanto, non al suo posto.
            if not any(p in testo for p in PATTERNS_FLUSSO):
                allestimento += 1
                # Il conteggio NETTO toglie chi gira il flusso attraverso una fixture di header: sono
                # test d'integrazione etichettati male, e puntarli sarebbe togliere copertura credendo
                # di togliere debito.
                if not gira_il_flusso(testo, fixture):
                    allestimento_netto += 1
        if PATTERN_TURNMANAGER in testo:
            con_tm += 1
    n_file = len(test_file)
    m["test_file"] = n_file
    m["test_unici"] = len(nomi)
    m["test_file_con_mondo"] = con_mondo
    m["test_file_allestimento"] = allestimento
    m["test_file_allestimento_netto"] = allestimento_netto
    m["test_file_con_flusso"] = con_mondo - allestimento
    m["fixture_di_flusso"] = sorted(fixture)
    m["test_file_con_turnmanager"] = con_tm
    m["quota_mondo"] = round(100.0 * con_mondo / n_file, 1) if n_file else 0.0
    m["quota_turnmanager"] = round(100.0 * con_tm / n_file, 1) if n_file else 0.0

    righe_prod = 0
    lunghe = []
    totali = 0
    for p in (radice / "Source").rglob("*.cpp"):
        if "Tests" in p.parts:
            continue
        righe_prod += len(leggi(p))
        path = p.relative_to(radice).as_posix()
        rel = path.replace("Source/RefactorTactics/", "")
        for lung, rami, scelte, nome, inizio in funzioni_di(p):
            totali += 1
            if lung >= soglia:
                lunghe.append({"righe": lung, "rami": rami, "scelte": scelte, "nome": nome,
                               "file": rel, "path": path, "linea": inizio})
    lunghe.sort(key=lambda r: -r["righe"])
    m["produzione_righe_cpp"] = righe_prod
    m["funzioni_totali"] = totali
    m["funzioni_lunghe"] = len(lunghe)
    m["righe_in_funzioni_lunghe"] = sum(r["righe"] for r in lunghe)
    m["quota_righe_in_funzioni_lunghe"] = (
        round(100.0 * m["righe_in_funzioni_lunghe"] / righe_prod, 1) if righe_prod else 0.0
    )
    m["per_contenitore"] = {}
    for r in lunghe:
        v = m["per_contenitore"].setdefault(contenitore(r["nome"]), {"funzioni": 0, "righe": 0})
        v["funzioni"] += 1
        v["righe"] += r["righe"]
    m["classifica"] = lunghe[:25]
    return m


def albero_di(ref, radice_repo):
    """Estrae `ref` in una directory temporanea: misurare un commit non deve toccare l'albero di lavoro."""
    tmp = Path(tempfile.mkdtemp(prefix="rt-misure-"))
    tar = tmp / "albero.tar"
    try:
        with tar.open("wb") as fh:
            subprocess.run(["git", "archive", ref, "Source"],
                           cwd=str(radice_repo), stdout=fh, stderr=subprocess.PIPE, check=True)
        subprocess.run(["tar", "-xf", str(tar), "-C", str(tmp)], check=True)
    except subprocess.CalledProcessError:
        shutil.rmtree(str(tmp), ignore_errors=True)
        raise SystemExit(
            "non riesco a leggere `Source/` da `%s`: il ref non esiste, oppure e' anteriore alla\n"
            "cartella. Un confronto con un albero che non ha i file misurati non dice niente." % ref
        )
    tar.unlink()
    return tmp


def aggiungi_churn(m, repo):
    """Quante volte l'intervallo di righe di ogni funzione e' stato toccato.

    `git log -L a,b:file` segue il movimento delle righe, quindi conta i commit che hanno toccato
    *quella funzione*, non il file che la contiene. Il prodotto `righe x commit` e' il costo che si
    paga davvero: una funzione lunga che nessuno tocca non costa niente a nessuno.

    🔴 `--no-merges`, ed e' una scelta, non un dettaglio. Senza, `ResolveCombatPasses` conta 108
    commit invece di 103: i cinque in piu' sono merge che hanno toccato quelle righe risolvendo un
    conflitto. Sono eventi reali — un conflitto li' e' un costo di coordinamento — ma non sono
    *lavoro deliberato sulla funzione*, e questo repository integra ogni PR con un merge commit,
    quindi contarli misura la forma del workflow insieme al codice. La domanda a cui il numero deve
    rispondere e' «quante volte qualcuno ha messo mano a questa funzione», e la risposta esclude i
    merge. Cambiare questa riga cambia tutti i numeri: dichiaralo dove li riporti.
    """
    for r in m["classifica"]:
        a = r["linea"]
        b = a + r["righe"] - 1
        try:
            esito = subprocess.run(
                ["git", "log", "--no-merges", "-L", "%d,%d:%s" % (a, b, r["path"]),
                 "--format=%h", "-s"],
                cwd=str(repo), capture_output=True, text=True, timeout=120,
            )
            r["commit"] = len([x for x in esito.stdout.splitlines() if x.strip()])
        except (subprocess.SubprocessError, OSError):
            r["commit"] = None
        r["costo"] = r["righe"] * r["commit"] if r["commit"] else 0
    return m


def delta(dopo, prima):
    d = dopo - prima
    return ("%+d" % d) if isinstance(d, int) else ("%+.1f" % d)


def stampa(m, base=None, markdown=False):
    def riga(etichetta, chiave, suffisso=""):
        v = m[chiave]
        if base is None:
            corpo = "%s%s" % (v, suffisso)
            if markdown:
                print("| %s | %s |" % (etichetta, corpo))
            else:
                print("  %-44s %s" % (etichetta, corpo))
            return
        b = base[chiave]
        if markdown:
            print("| %s | %s%s | %s%s | **%s** |" % (etichetta, b, suffisso, v, suffisso, delta(v, b)))
        else:
            print("  %-44s %8s%s -> %8s%s   %s" % (etichetta, b, suffisso, v, suffisso, delta(v, b)))

    if markdown:
        testa = ["Metrica", "prima", "dopo", "delta"] if base else ["Metrica", "valore"]
        print("| " + " | ".join(testa) + " |")
        print("|" + "|".join(["---"] * len(testa)) + "|")
    else:
        print("=== MISURE STRUTTURALI (%s) ===" % m["ref"])

    riga("`ARTTurnManager` righe (3 file)", "turnmanager_righe")
    riga("`ARTTurnManager` righe di **codice**", "turnmanager_righe_codice")
    riga("metodi `ARTTurnManager::`", "turnmanager_metodi")
    riga("file di test", "test_file")
    riga("test unici", "test_unici")
    riga("file di test che spawnano un Actor", "test_file_con_mondo")
    riga("quota test con mondo", "quota_mondo", " %")
    riga("  di cui girano il flusso del turno", "test_file_con_flusso")
    riga("  di cui ALLESTISCONO soltanto", "test_file_allestimento")
    riga("  — e al NETTO delle fixture", "test_file_allestimento_netto")
    riga("file di test che dipendono dal TurnManager", "test_file_con_turnmanager")
    riga("quota test col TurnManager", "quota_turnmanager", " %")
    riga("funzioni >= %d righe" % m["soglia"], "funzioni_lunghe")
    riga("righe dentro quelle funzioni", "righe_in_funzioni_lunghe")
    riga("quota del codice di produzione", "quota_righe_in_funzioni_lunghe", " %")

    print()
    ordinati = sorted(m["per_contenitore"].items(), key=lambda kv: -kv[1]["righe"])
    if markdown:
        print("**Funzioni >= %d righe, per contenitore**\n" % m["soglia"])
        print("| contenitore | funzioni | righe |")
        print("|---|---|---|")
        for k, v in ordinati:
            print("| %s | %d | %d |" % (k, v["funzioni"], v["righe"]))
        print()
        print("**Le 12 piu' lunghe** — `rami` quasi nullo = tabella di dati, non complessita'\n")
        print("| righe | rami | scelte | funzione | file:linea |")
        print("|---|---|---|---|---|")
        for r in m["classifica"][:12]:
            print("| %d | %d | %d | `%s` | `%s:%d` |"
                  % (r["righe"], r["rami"], r["scelte"], r["nome"], r["file"], r["linea"]))
    else:
        print("=== FUNZIONI >= %d RIGHE, PER CONTENITORE ===" % m["soglia"])
        for k, v in ordinati:
            print("  %-32s %4d funzioni %7d righe" % (k, v["funzioni"], v["righe"]))
        print()
        print("=== LE 12 PIU' LUNGHE (rami quasi nullo = dati, non complessita') ===")
        print("  %6s %5s %6s  %-48s %s" % ("RIGHE", "RAMI", "SCELTE", "FUNZIONE", "FILE:LINEA"))
        for r in m["classifica"][:12]:
            print("  %6d %5d %6d  %-48s %s:%d"
                  % (r["righe"], r["rami"], r["scelte"], r["nome"][:48], r["file"], r["linea"]))

    if any("commit" in r for r in m["classifica"]):
        per_costo = sorted(m["classifica"], key=lambda r: -r.get("costo", 0))[:12]
        print()
        if markdown:
            print("**Worklist per costo** — `righe x commit che hanno toccato quelle righe`\n")
            print("| costo | commit | righe | rami | scelte | funzione | file:linea |")
            print("|---|---|---|---|---|---|---|")
            for r in per_costo:
                print("| %d | %s | %d | %d | %d | `%s` | `%s:%d` |"
                      % (r.get("costo", 0), r.get("commit", "?"), r["righe"], r["rami"], r["scelte"],
                         r["nome"], r["file"], r["linea"]))
        else:
            print("=== WORKLIST PER COSTO (righe x commit che hanno toccato quelle righe) ===")
            print("  %7s %7s %6s %5s %6s  %-44s %s"
                  % ("COSTO", "COMMIT", "RIGHE", "RAMI", "SCELTE", "FUNZIONE", "FILE:LINEA"))
            for r in per_costo:
                print("  %7d %7s %6d %5d %6d  %-44s %s:%d"
                      % (r.get("costo", 0), r.get("commit", "?"), r["righe"], r["rami"], r["scelte"],
                         r["nome"][:44], r["file"], r["linea"]))


# Le grandezze che il gate sorveglia, e perche' proprio queste.
#
# NON le righe del file: il 58% di cio' che si aggiunge qui e' commento normativo, e un gate che
# sale quando qualcuno scrive la ragione di una regola insegna a non scriverla.
SORVEGLIATE = [
    ("turnmanager_metodi", "metodi `ARTTurnManager::`"),
    ("turnmanager_righe_codice", "righe di codice (3 file)"),
]


def base_di_confronto(repo):
    """Il merge-base con `origin/main`: misura cio' che il branch aggiunge, non cio' che main ha nel frattempo."""
    for riferimento in ("origin/main", "main"):
        esito = subprocess.run(["git", "merge-base", "HEAD", riferimento],
                               cwd=str(repo), capture_output=True, text=True)
        if esito.returncode == 0 and esito.stdout.strip():
            return esito.stdout.strip()
    return None


def esegui_check(m, base):
    """Stampa il delta delle grandezze sorvegliate. Esce 1 se una e' cresciuta.

    ⚠️ **Ordina, non decide.** L'uscita 1 non e' un fallimento e non e' un veto: e' «qui c'e' una
    crescita, dichiarala». Una feature che aggiunge un metodo al TurnManager e' legittima; una che lo
    aggiunge *in silenzio* fa salire il fondo mentre ogni singola fetta di #1818 riesce, ed e' cosi'
    che la classe e' cresciuta di 395 righe di codice e 10 metodi nei quattro giorni dell'audit.

    Sorveglia il solo `ARTTurnManager`, che e' l'oggetto di #1818. Non dice nulla del resto del modulo.
    """
    print("=== GATE DI NON-REGRESSIONE - E50 #1816 ===")
    print("base: %s\n" % base["ref"])

    # Se nel base mancava uno dei tre file, il delta racconta una crescita che e' solo un file
    # comparso. `RTTurnManager_Blast.cpp` esiste da `e18eee50` (2026-08-17): un base anteriore
    # produrrebbe un +2 277 che nessuno ha scritto.
    mancanti = set(base["turnmanager_file"]) ^ set(m["turnmanager_file"])
    if mancanti:
        print("  ! il base non ha gli stessi file: %s" % ", ".join(sorted(mancanti)))
        print("    il delta qui sotto include un file comparso, non solo codice scritto.\n")

    cresciute = []
    for chiave, etichetta in SORVEGLIATE:
        prima, dopo = base[chiave], m[chiave]
        d = dopo - prima
        if d > 0:
            cresciute.append((etichetta, prima, dopo, d))
        print("  %-30s %6d -> %6d  %+6d   %s"
              % (etichetta, prima, dopo, d, "DA DICHIARARE" if d > 0 else "ok"))
    print()
    if not cresciute:
        print("OK - nessuna crescita da dichiarare.")
        return 0
    print("CRESCITA DA DICHIARARE - non e' un veto.")
    print("  Incolla nella sezione \"Verifiche\" della PR:\n")
    for etichetta, prima, dopo, d in cresciute:
        print("    %s: %d -> %d (%+d), perche': ..." % (etichetta, prima, dopo, d))
    print("\n  Una crescita motivata va bene. Una crescita silenziosa e' cio' che E50 non puo' vedere.")
    return 1


def autotest():
    """`esegui_check` provata senza albero: dimostra che il gate SA fallire (`D-188`).

    ⚠️ **Prova la DECISIONE, non la misura.** Che `misura()` conti i metodi giusti lo dicono i suoi
    numeri su un albero vero; qui si prova che, dati due conteggi, il gate risponde come deve. Sono due
    difetti diversi e nessuno dei due copre l'altro.

    🔑 **Un gate nasce verde**, e questo nasceva verde da sempre: senza una mutazione che lo faccia
    diventare rosso, il suo 0 non distingue «ho guardato e non e' cresciuto» da «non ho guardato».
    """
    def m(metodi, righe, file=("h", "cpp", "blast")):
        return {"turnmanager_metodi": metodi,
                "turnmanager_righe_codice": righe,
                "turnmanager_file": list(file),
                "ref": "finto"}

    casi = [
        # (nome, dopo, prima, uscita attesa)
        ("nessuna crescita", m(120, 5000), m(120, 5000), 0),
        # 🔑 Il difetto STORICO, coi numeri che l'audit del 2026-08-30 ha misurato davvero: +395
        # righe di codice e +10 metodi in quattro giorni, non per colpa delle fette di #1818 ma per i
        # 53 commit di gameplay ordinario atterrati sulla stessa classe. E' il caso per cui questo
        # gate esiste, ed e' la ragione per cui questa prova non e' inventata.
        ("la crescita di agosto: +10 metodi, +395 righe", m(130, 5395), m(120, 5000), 1),
        ("cresce solo il conteggio dei metodi", m(121, 5000), m(120, 5000), 1),
        ("cresce solo il conteggio delle righe", m(120, 5001), m(120, 5000), 1),
        # Una discesa NON e' una crescita: #1818 chiede di spostare responsabilita' FUORI dal
        # TurnManager, e un gate che si lamentasse anche del miglioramento verrebbe spento.
        ("una responsabilita' spostata fuori", m(110, 4500), m(120, 5000), 0),
        ("una sale e l'altra scende", m(121, 4500), m(120, 5000), 1),
        # Il base che non ha gli stessi file: il delta include un file comparso. Il gate lo dichiara,
        # e la crescita resta da dichiarare — non si annulla.
        ("il base non ha gli stessi file", m(130, 5395), m(120, 5000, ("h", "cpp")), 1),
    ]

    falliti = []
    for nome, dopo, prima, atteso in casi:
        avuto = esegui_check(dopo, prima)
        print("  %s %s: atteso %d, avuto %d"
              % ("✅" if avuto == atteso else "❌", nome, atteso, avuto))
        if avuto != atteso:
            falliti.append(nome)

    # ── `fixture_di_flusso` e `gira_il_flusso`, provate sulle stringhe (#2182) ────────────────────────
    #
    # ⚠️ **Sono prove della funzione PURA, non della misura sull'albero.** Che i quattro file classificati
    # male siano davvero quattro lo dice il comando nel commento di `PATTERNS_FLUSSO`; qui si prova che,
    # dato un header, la funzione ne estragga i nomi giusti — e che sappia NON estrarli.
    print()
    HEADER_VERO = '''
    inline void PlayOneTurn(ARTTurnManager* TM)
    {
        TM->PlanBotsForTest();
        TM->LockInAndResolve();
        for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
        {
            TM->Tick(0.05f);
        }
    }

    inline ARTUnit* FirstUnitOfTeam(UWorld* World, int32 TeamId)
    {
        return nullptr;
    }
    '''
    # ⚠️ Il commento IN LINEA e quello a riga intera provano rami diversi (`spoglia` contro
    # `RE_COMMENTO`), e servono tutti e due: una prima stesura di questi casi metteva la nota **sopra**
    # l'`inline`, dove viene scartata prima ancora di arrivare a `RE_COMMENTO`, e quel caso non provava
    # cio' che dichiarava.
    HEADER_SOLO_COMMENTO = '''
    inline int32 ContaSoltanto(const TArray<int32>& X)
    {
        // Questa nota a riga intera cita `LockInAndResolve` per spiegare cosa NON fa.
        return X.Num(); // e questa lo cita in linea, con `RunTurn` accanto
    }
    '''
    # 🔴 Una DICHIARAZIONE prima della definizione vera: senza il ramo che la chiude, `corrente` resta
    # agganciata a `Dichiarata` e `PlayOneTurn` non viene mai vista.
    HEADER_CON_DICHIARAZIONE = '''
    inline void Dichiarata(int32 X);

    inline void PlayOneTurn(ARTTurnManager* TM)
    {
        TM->LockInAndResolve();
    }
    '''
    # 🔴 Due fixture in fila, di cui solo la SECONDA gira il flusso: se `corrente` non si riazzera alla
    # chiusura della prima, il marcatore della seconda le viene attribuito e il nome esce sbagliato.
    HEADER_DUE_IN_FILA = '''
    inline int32 Prima(int32 X)
    {
        return X;
    }

    inline void Seconda(ARTTurnManager* TM)
    {
        TM->LockInAndResolve();
    }
    '''
    casi_fixture = [
        ("la fixture che gira il turno e' riconosciuta", fixture_di_flusso(HEADER_VERO), {"PlayOneTurn"}),
        # ⛔ Anti-vacuita': un marcatore in un COMMENTO non e' una chiamata — ne' a riga intera ne' in
        # linea. Senza, la funzione promuoverebbe ogni fixture il cui docstring nomina il flusso, e in
        # questo repository i docstring lo nominano spesso.
        ("un marcatore citato in un commento non conta", fixture_di_flusso(HEADER_SOLO_COMMENTO), set()),
        # 🔑 Il riazzeramento di `corrente`: senza, il nome che esce e' `Prima`, non `Seconda`.
        ("chiusa una fixture, la successiva riparte col proprio nome",
         fixture_di_flusso(HEADER_DUE_IN_FILA), {"Seconda"}),
        # 🔑 La dichiarazione senza corpo non deve dirottare il nome.
        ("una dichiarazione senza corpo non mangia la fixture che segue",
         fixture_di_flusso(HEADER_CON_DICHIARAZIONE), {"PlayOneTurn"}),
        ("un .cpp che chiama la fixture gira il flusso",
         gira_il_flusso("RTWorldFixtures::PlayOneTurn(TM);", {"PlayOneTurn"}), True),
        ("un .cpp che non la chiama, no",
         gira_il_flusso("World->SpawnActor<ARTUnit>();", {"PlayOneTurn"}), False),
        # ⛔ Il `(` del confronto non e' decorativo: senza, il solo NOME nominato in un commento o in un
        # `#include` basterebbe a promuovere il file. Questo caso lo tiene fermo.
        ("il nome senza parentesi NON basta: serve una chiamata",
         gira_il_flusso("// vedi PlayOneTurn per il flusso completo", {"PlayOneTurn"}), False),
        # 🔑 Il caso che dimostra che il NETTO non e' l'ALLESTIMENTO: senza fixture note, un file che
        # chiama `PlayOneTurn` resta classificato come allestimento. E' il difetto storico, riprodotto.
        ("senza fixture note, lo stesso .cpp risulta allestimento",
         gira_il_flusso("RTWorldFixtures::PlayOneTurn(TM);", set()), False),
    ]
    for nome, avuto, atteso in casi_fixture:
        ok = avuto == atteso
        print("  %s %s: atteso %s, avuto %s" % ("✅" if ok else "❌", nome, atteso, avuto))
        if not ok:
            falliti.append(nome)
    casi = casi + casi_fixture

    print()
    # Anti-vacuita': con `SORVEGLIATE` vuota ogni caso tornerebbe 0, e questa prova resterebbe verde
    # raccontando che va tutto bene. E' il verde per costruzione che la prova esiste per escludere.
    if not SORVEGLIATE:
        print("❌ `SORVEGLIATE` e' vuota: ogni caso tornerebbe 0 e questa prova sarebbe verde a vuoto.")
        return 1
    if falliti:
        print("❌ autotest rosso: %s" % ", ".join(falliti))
        return 1
    print("✅ autotest verde — %d casi, e il gate sa tornare 1." % len(casi))
    return 0

def main():
    ap = argparse.ArgumentParser(description="Misure strutturali di E50 - #1816, #1818")
    ap.add_argument("--ref", help="misura questo commit invece dell'albero di lavoro")
    ap.add_argument("--base", help="commit di confronto: stampa prima/dopo/delta")
    ap.add_argument("--soglia", type=int, default=100, help="righe oltre le quali una funzione e' lunga")
    ap.add_argument("--markdown", action="store_true", help="tabelle da incollare in un commento di issue")
    ap.add_argument("--json", help="scrive le misure grezze in questo file")
    ap.add_argument("--churn", action="store_true",
                    help="conta i commit che hanno toccato ogni funzione e ordina per costo "
                         "(solo sull'albero di lavoro)")
    ap.add_argument("--check", action="store_true",
                    help="gate di non-regressione: confronta le grandezze sorvegliate col merge-base "
                         "(o con --base) ed esce 1 se sono cresciute. Non e' un veto: e' «dichiaralo»")
    ap.add_argument("--autotest", action="store_true",
                    help="prova `esegui_check` senza leggere l'albero: e' la prova che il gate sa fallire")
    a = ap.parse_args()

    # La console di Windows e' cp1252: senza questo, un `—` in una tabella `--markdown` non stampa
    # male, fa uscire lo strumento con un UnicodeEncodeError a meta' output. Misurato, non dedotto.
    #
    # ⌫ **Stava DOPO il ramo `--autotest`, e quel ramo stampa `✅`/`❌`**: su una console cp1252
    # `--autotest` moriva con `UnicodeEncodeError` prima del primo caso, cioe' il gate che dimostra che
    # il gate sa fallire non era eseguibile dove il progetto gira. Difetto preesistente, trovato in code
    # review il 2026-09-24 mentre se ne aggiungevano i casi, e riprodotto forzando `encoding='cp1252'`
    # su un sottoprocesso senza `PYTHONIOENCODING`.
    #
    # 🔴 **Ed e' un difetto che si nasconde da solo**: chi esporta `PYTHONIOENCODING=utf-8` per leggere
    # l'output non lo vede mai — ed e' esattamente come mi era sfuggito.
    for flusso in (sys.stdout, sys.stderr):
        if hasattr(flusso, "reconfigure"):
            flusso.reconfigure(errors="replace")

    # Prima di qualunque lettura dell'albero: l'autotest non ne ha bisogno, e pretenderlo lo renderebbe
    # ineseguibile proprio dove serve — su un clone senza `origin/main`.
    if a.autotest:
        return autotest()

    repo = Path(__file__).resolve().parents[2]
    temporanee = []
    try:
        radice = albero_di(a.ref, repo) if a.ref else repo
        if a.ref:
            temporanee.append(radice)
        m = misura(radice, a.soglia)
        m["ref"] = a.ref or "albero di lavoro"

        if a.churn:
            if a.ref:
                sys.stderr.write(
                    "--churn ignorato: `git log -L` legge il repository, non la copia temporanea di --ref.\n"
                )
            else:
                aggiungi_churn(m, repo)

        riferimento_base = a.base
        if a.check and not riferimento_base:
            riferimento_base = base_di_confronto(repo)
            if not riferimento_base:
                sys.stderr.write("--check: nessun `origin/main` da cui calcolare il merge-base.\n")
                return 2

        base = None
        if riferimento_base:
            rb = albero_di(riferimento_base, repo)
            temporanee.append(rb)
            base = misura(rb, a.soglia)
            base["ref"] = riferimento_base
            if a.markdown and not a.check:
                print("Misurato con `tools/architettura/misure-strutturali.py`, da `%s` a `%s`.\n"
                      % (riferimento_base, m["ref"]))

        if a.check:
            return esegui_check(m, base)

        stampa(m, base, a.markdown)

        if a.json:
            Path(a.json).write_text(
                json.dumps({"dopo": m, "prima": base}, indent=2, ensure_ascii=False), encoding="utf-8"
            )
            sys.stderr.write("\nmisure grezze: %s\n" % a.json)
    finally:
        for t in temporanee:
            shutil.rmtree(str(t), ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main() or 0)
