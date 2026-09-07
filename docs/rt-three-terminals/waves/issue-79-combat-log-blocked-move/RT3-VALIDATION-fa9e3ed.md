=== RT3 HANDOFF ===

FROM:          VALIDATION
TO:            DEV-LEAD
FEATURE:       issue-79-combat-log-blocked-move
WAVE_ID:       issue-79-combat-log-blocked-move/1

BRANCH:        fix/79-blocked-move-turnlog
PARENT_BRANCH: main
BASE_SHA:      8dcd3a765251f012961ea87c4bf518a50d60e014   (EXPECTED_SHA dall'handoff EDITOR)
PRODUCED_SHA:  fa9e3ed2c598d5f08c168ba0c7311e6135bfed9c
               = HEAD misurato. VALIDATION non ha scritto codice: il write-set è questo
               handoff e le proprie evidenze.
HEAD_AT_OPEN:  b5badb7903c0b6d6aa4023bdea905f644ab45852
HEAD_AT_CLOSE: fa9e3ed2 — mosso una volta durante la sessione, vedi §2

INPUT_HANDOFF_DEV:    docs/rt-three-terminals/waves/issue-79-combat-log-blocked-move/RT3-DEVLEAD-55e3140.md
                      letto dal filesystem, STATUS: READY
INPUT_HANDOFF_EDITOR: docs/rt-three-terminals/waves/issue-79-combat-log-blocked-move/RT3-EDITOR-8dcd3a76.md
                      letto dal filesystem, STATUS: PARTIAL

WRITE_SET:     docs/rt-three-terminals/waves/issue-79-combat-log-blocked-move/RT3-VALIDATION-fa9e3ed.md
               docs/rt-three-terminals/waves/issue-79-combat-log-blocked-move/evidence/
                 build-validation-b5badb79.log
                 suite-val79-playerinteraction-fa9e3ed2.log
                 suite-val79-hexmove-fa9e3ed2.log
                 suite-val79-scenario-fa9e3ed2.log
                 suite-val79-full-fa9e3ed2.log
BINARY_ASSETS: nessuno

STATUS: PARTIAL

⚠️ `PARTIAL` e non `READY`: **nessun sistema in scope è `FAIL`**, ogni gate eseguibile è verde e
gli oracoli della wave sono verificati. Ma §13 lega `DONE` alla Definition of Done **viva** della
#79, che porta due check a oracolo umano ancora `NOT RUN`, e la full suite eredita da `main` tre
test rossi che non appartengono a questa wave. Vedi § `USER_REQUIRED` e Finding `1-F12`.

---

```text
RT3 INIT

Tipo:               VALIDATION
Feature:            issue-79-combat-log-blocked-move
Wave:               issue-79-combat-log-blocked-move/1
Branch:             fix/79-blocked-move-turnlog
Parent branch:      main
Base SHA:           8dcd3a76   (EXPECTED_SHA dichiarato da EDITOR)
HEAD:               fa9e3ed2
Working tree:       pulito salvo le proprie evidenze (untracked al momento della misura)
Engine:             5.8 — D:\EpicGames\UE_5.8 (EngineAssociation da RefactorTactics.uproject)
Lease:              416e5d77a740 · SUITE · VALIDATION · task 79 · RELEASED
                    (preceduto da bfcc9151c27e · BUILD · RELEASED)
Sistemi in scope:   dal WRITE_SET dell'handoff DEV-LEAD (8 file) + downstream §8
```

Questa è la **seconda** run VALIDATION della wave. La prima —
[`RT3-VALIDATION-a59671c.md`](RT3-VALIDATION-a59671c.md), `BLOCKED / MISSING_INPUT` — resta
evidenza storica: allora il branch non portava codice e i due handoff non esistevano. Non è il
verdetto di questa run.

---

## 1. Preflight §4 — passa

| Campo | Valore | Esito |
|---|---|---|
| `FEATURE` | `issue-79-combat-log-blocked-move` | risolto |
| `BRANCH` | `fix/79-blocked-move-turnlog` | risolto, corrisponde a `--abbrev-ref HEAD` |
| `BASE_SHA` / `EXPECTED_SHA` | `8dcd3a76…` | risolto — valore reale |
| `INPUT_HANDOFF_DEV` | `…/RT3-DEVLEAD-55e3140.md` | risolto, file esistente, letto dal disco |
| `INPUT_HANDOFF_EDITOR` | `…/RT3-EDITOR-8dcd3a76.md` | risolto, file esistente, letto dal disco |

§11: nessuno dei due handoff in ingresso è `BLOCKED`. `PARTIAL` di EDITOR non blocca — ma i
sistemi che ha lasciato `OBSERVED` o `NOT RUN` **non** diventano `PASS` per ereditarietà: quelli
che ho misurato li porto io, gli altri restano non provati.

---

## 2. Precondizioni §5 — due divergenze, entrambe misurate

### 2.1 `HEAD` ≠ `EXPECTED_SHA` all'apertura

```text
EXPECTED_SHA  8dcd3a76   (PRODUCED_SHA dichiarato da EDITOR)
HEAD apertura b5badb79
git diff --name-only 8dcd3a76..b5badb79 -- Source/ Content/ Config/ Scenarios/   -> (vuoto)
git diff --stat     8dcd3a76..b5badb79   -> 3 PNG in docs/research/maps/
```

La lettera di §5 direbbe `BLOCKED`. **Non ho bloccato**, e dichiaro la lettura invece di
applicarla in silenzio: il delta è di tre PNG di ricerca fuori da ogni percorso compilato,
caricato o misurato. Bloccare non avrebbe protetto nessuna misura. È la stessa scelta che EDITOR
ha dichiarato in `1-F8`, che **ratifico** — e la ratifica di due ruoli indipendenti non
sostituisce la decisione di contratto che quel Finding chiede.

### 2.2 `HEAD` si è mosso **durante** la sessione

```text
apertura      b5badb79
durante       fa9e3ed2   (rilevato dal marker [RT-MEASURE] della prima run di suite)
git log --oneline b5badb79..fa9e3ed2
  fa9e3ed2 docs(rt3): risposte ai tre finding EDITOR — uno ratificato, uno esce in #2627
  3e66205b docs(rt3): il referto EDITOR su 8dcd3a76 e le cinque evidenze, con la riga che chiude #79
git diff --stat b5badb79 fa9e3ed2 -- Source/ Scenarios/   -> (vuoto)
```

🔑 **La review avversariale e la build sono state fatte su `b5badb79`, le suite su `fa9e3ed2`.**
Sarebbero due misure di due commit diversi se il codice fosse cambiato: **non è cambiato**. I due
commit sono documentazione della wave. Le misure restano attribuibili allo stesso codice, e ogni
riga di questo handoff cita l'`HEAD` su cui è stata prodotta.

⚠️ Le quattro run di suite riportano tutte `albero 3103b044` invariato **durante** la run: nessuna
finestra di misura è stata attraversata da un movimento. È la condizione che §5 protegge, ed è
soddisfatta run per run.

📌 Terza volta in questa wave che il branch si muove sotto una sessione di misura. Finding `1-F13`.

### 2.3 Write-set e binari

- il working tree conteneva, all'apertura, esattamente il `WRITE_SET` dichiarato da EDITOR
  (`RT3-EDITOR-8dcd3a76.md` + `evidence/`): **conforme**, non modifiche non dichiarate;
- `BINARY_ASSETS: none` di EDITOR è **verificato**: nessun `.uasset`/`.umap` nel diff della wave.
  I 3 PNG di §2.1 sono binari, ma di `docs/research/` e di un commit estraneo — sono la
  sostanza di `1-F8`, non un asset di gioco non dichiarato.

---

## 3. Review avversariale del diff — prima della build

Diff: `a59671c8..fa9e3ed2` — 8 file, 856 inserzioni (205 di produzione, 651 di test).

I tredici punti che il mandato chiede di cercare, con l'esito misurato sul codice.

| # | Punto di review | Esito |
|---:|---|---|
| 1 | logging fatto solo nel Scenario Harness | ✅ **no** — `ARTUnit::NoteMovePlanRejection` è **una** funzione chiamata da `RTPlayerController.cpp:1636` e `RTScenarioSession.cpp:1365`. La voce non si scrive in nessuno dei due: si scrive in `ResolveMovement` |
| 2 | evento su ogni click invalido invece che sul piano finale | ✅ **no** — il click registra un **intento** su due campi `Transient`; la voce nasce nella fase `Move`. Il commento a `RTPlayerController.cpp:1631` dichiara la ragione: *«il TurnLog è un formato ordinato e riprodotto, e ogni click esplorativo diventerebbe un fatto del replay»* |
| 3 | pending rejection non cancellato dal replan | ✅ **cancellato in 3 siti**: ramo valido del controller (`:1656`), `RebuildPlannedPath` (`:2083` — copre `OnUndoWaypoint` e il passo indietro), ramo valido dell'harness (`:1349`) |
| 4 | pending rejection non resettato al nuovo turno | ✅ **resettato** — era il mio sospetto principale: `PlaceOnCell` azzera, ma **un'unità bloccata non si muove e non ci passa**. Il reset vero è nel ciclo finale di `ResolveMovement` (`:7616`), prima dei rami, e copre anche chi muore nel turno |
| 5 | evento `Stayed` duplicato con `BlockedByUnit` | ✅ **sostituisce** — `MoveLog[i].Outcome = BlockedByUnit` riscrive la voce che `BuildMoveLog` ha già emesso. Una voce per unità per fase. Provato da `ReplanAfterADenialWins`, che asserisce `Voci.Num() == 1` |
| 6 | `ActionId`/`Priority` hardcoded | ✅ **no** — restano quelli scritti da `BuildMoveLog` dal catalogo. Il ramo non li tocca, e il log lo conferma: `(Action.Move, p50)` |
| 7 | `TMap` iterata per produrre ordine | ✅ **no** — `TArray<bool>` e `TArray<FRTCellId>` indicizzati per unità, `Init(...)` sulla stessa cardinalità di `Units` |
| 8 | TurnLog creato senza wrapper canonico | ✅ **no** — nessuna voce nuova viene creata: si riscrivono due campi di una voce esistente |
| 9 | leak di planning prima del commit | ✅ **no** — i due campi sono `UPROPERTY(Transient)` e **non** `Replicated`. Delta replication misurato da DEV-LEAD e ricontrollato da EDITOR: baseline 10 occorrenze / 5 file, dopo 10/5, **delta 0** |
| 10 | cambi di serializzazione non dichiarati | ✅ **nessun cambio** — `Turn/RTTurnLog.h` non è nel diff: nessun enumeratore nuovo, nessun bump di formato. Cambia il **valore** di `Outcome` e `TgtCell` su una voce che esisteva già |
| 11 | golden rigenerati senza motivazione | ✅ **non rigenerati** — `Tests/Golden/**` non è nel diff, e i 12 test Golden passano (§5). Il rischio che avevo segnalato **non si è materializzato** |
| 12 | comportamento diverso fra PlayerController e Scenario Harness | ✅ **una regola sola** — stessa funzione, e il punto di scrittura della voce è comune a entrambi |
| 13 | bot o altro produttore che crea lo stesso stato | ✅ **coperto** — vedi sotto |

### 3.1 Il terzo produttore: il residuo del mio `1-F3` è stato accolto

La prima run VALIDATION aveva rilevato che i chiamanti erano **tre**, non due, e che il bot
(`RTHexBotLibrary.cpp:688`) usa `FindPathForUnit` e non compariva in nessuno scope. Il codice
consegnato lo copre, e lo copre **nel punto giusto** — non aggiungendo un quarto sito di
scrittura, ma nel collasso comune (`RTTurnManager.cpp:7213-7224`):

```cpp
if (Unit->bMovePlanRejectedByOccupant) { … }              // player + harness: stato portato fin qui
else if (Unit->HasPlannedNormalMove()
      && ClassifyWaypointCell(Snapshot, i, Unit->PlannedCell) == Occupied) { … }   // bot
```

Le due fonti non sono simmetriche e il commento lo dichiara: player e harness rifiutano **in
pianificazione** e il `Pop()` cancella la destinazione, quindi serve uno stato che sopravviva; il
bot non ha ramo di rifiuto e il suo percorso fallisce **qui**, dove la destinazione è ancora
leggibile. Il criterio di accettazione 4 del WORK-ORDER dice «due chiamanti»: il codice ne
soddisfa tre. ✅ `1-F3` chiuso.

### 3.2 Qualità dei test

Sei test nuovi, e la loro disciplina anti-vacuità è reale, non dichiarata:

- `UndeclaredMoveDoesNotDeclareADenial` è il **gemello** del caso B: senza, un ramo incondizionato
  che scrive sempre `BlockedByUnit` passerebbe il test di B;
- `BudgetDenialIsNotAUnitDenial` pinna lo **scope**: budget esaurito resta `Stayed`;
- `ReplanAfterADenialWins` verifica `ContaEsitoMove(BlockedByUnit) == 0` su **tutto** il log, non
  solo sulla voce dell'unità — coglierebbe anche un evento scritto al click — e asserisce
  `Voci.Num() == 1`, `Outcome == Moved`, `TgtCell == Ripiego`. Le sue premesse sono asserite
  prima (`il primo tentativo È stato negato`, `la correzione È stata accettata`): non è un test
  che passa perché non è successo niente.

---

## 4. Build — eseguita da me, con la qualifica esatta

```text
command:      D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat RefactorTacticsEditor
              Win64 Development -project=D:\Repositories\refactor-tactics-main\RefactorTactics.uproject
              -waitmutex
HEAD:         b5badb79
exit code:    0
esito:        Result: Succeeded — 0 occorrenze di "error C" / "error LNK" / "fatal error"
verdetto:     PASS
EVIDENCE_REF: docs/.../evidence/build-validation-b5badb79.log
```

⚠️ **UBT non ha ricompilato**: `Target is up to date`, 0 action, 1.52 s. Il binario resta quello
prodotto da EDITOR (16:47:17), e `WAVE_VALIDATION.md` §B è esplicito — *«un binario che non hai
prodotto non è una misura tua»*. Non lo nascondo dietro un `Result: Succeeded`: ho verificato la
sostanza che quella regola protegge, cioè che il binario corrisponda ai sorgenti che sto validando.

```text
DLL                       2026-09-06 16:47:17
file più recente di Source/ rispetto alla DLL:
  find Source/ -newer Binaries/Win64/UnrealEditor-RefactorTactics.dll -type f   -> (vuoto)
```

**Nessun** file di `Source/` — non solo del write-set: dell'intero albero — è più recente del
binario. «Up to date» è quindi una verifica sostanziale e non una scorciatoia. Registro `PASS` su
questa base, dichiarando che un rebuild pulito non è stato eseguito.

---

## 5. Suite — quattro run, tutte `VALIDA`

```text
command:      scripts\rt-suite.ps1 -Filter "RefactorTactics.PlayerInteraction" -LogName rt-suite-val79-playerinteraction.log
HEAD:         fa9e3ed2        albero 3103b044 (invariato durante la run)
found N:      13
performed N:  13
passed N:     13
failed N:     0
exit code:    0
verdetto:     PASS
EVIDENCE_REF: suite: docs/.../evidence/suite-val79-playerinteraction-fa9e3ed2.log
```

```text
command:      scripts\rt-suite.ps1 -Filter "RefactorTactics.HexMove" -LogName rt-suite-val79-hexmove.log
HEAD:         fa9e3ed2        albero 3103b044
found N:      19
performed N:  19
passed N:     19
failed N:     0
exit code:    0
verdetto:     PASS
EVIDENCE_REF: suite: docs/.../evidence/suite-val79-hexmove-fa9e3ed2.log
```

```text
command:      scripts\rt-suite.ps1 -Filter "RefactorTactics.Scenario" -LogName rt-suite-val79-scenario.log
HEAD:         fa9e3ed2        albero 3103b044
found N:      174
performed N:  174
passed N:     174
failed N:     0
exit code:    0
verdetto:     PASS
EVIDENCE_REF: suite: docs/.../evidence/suite-val79-scenario-fa9e3ed2.log
              -> Movement.CollisionChoke: PASS (6/6 assertion, 4 turni)
```

```text
command:      scripts\rt-suite.ps1 -Filter "RefactorTactics" -LogName rt-suite-val79-full.log
HEAD:         fa9e3ed2        albero 3103b044
found N:      2138
performed N:  2138
passed N:     2135
failed N:     3
exit code:    0        ⚠️ l'exit code NON è l'oracolo: 0 con 3 fallimenti
verdetto:     PASS per i sistemi in scope · i 3 rossi sono fuori wave, vedi §5.2
EVIDENCE_REF: suite: docs/.../evidence/suite-val79-full-fa9e3ed2.log
```

`performed = 0` non compare in nessuna run. I sei test della wave sono verificati **per nome** con
`Result={Success}` negli estratti, non dedotti da un totale.

### 5.1 Gate nominati dal mandato

Conteggi sui `Result={Success}` / `Result={Fail}` della full suite, per famiglia:

| Gate | Success | Fail |
|---|---:|---:|
| Golden (corpora `*.rttl`) | 12 | **0** |
| Determinism | 21 | **0** |
| Replay | 100 | **0** |
| TurnLog | 69 | **0** |
| Serialization | 4 | **0** |

🔑 **Il rischio che avevo aperto come candidato numero uno a un `FAIL` non si è materializzato.**
`TgtCell` cambia valore su una voce, e i corpora golden con checksum potevano cambiare digest:
non è successo, e i 12 test Golden lo provano. La ragione è nel codice — nessuno scenario del
corpus contiene un movimento dichiarato verso una cella occupata da un'unità ferma.

### 5.2 I tre rossi della full suite — **non attribuibili a questa wave**

```text
RefactorTactics.Bot.StallDefinitionsOnTheGeneratedTestArena
RefactorTactics.IconCatalog.RealCatalogCoversRequiredIds
RefactorTactics.Match.Autobattle.EngagesOnTheGeneratedTestArena
```

Messaggi:

```text
Expected 'la (b) tocca la soglia in uso (margine zero) — 11 su 4' to be 4, but it was 11.
  RTStallDefinitionMeasureTests.cpp(545)
1 chiave/i richiesta/e non coperta/e dal catalogo reale: UI.Icon.Identity.Branth
  RTUnbalancedProneTests.cpp(1027)
Expected 'nessuna unita' si parcheggia: piu' lunga sequenza ferma 11 turni (limite 4) …' to be true.
  RTMatchAutobattleTests.cpp(2130)
```

Due misurano **la stessa cosa** (11 turni fermi contro un limite di 4); il terzo è un'icona
mancante per `Branth`. **Quattro evidenze indipendenti** li escludono da questa wave:

1. **La sonda di stallo è cieca a ciò che la wave tocca.** `FRTStallDefinitionProbe::Observe`
   (`RTStallDefinitionProbeForTest.h:151-154`) misura per **posizione**:
   ```cpp
   const bool bFerma = U.bHaPrecedente && U.Precedente == Cell;
   ```
   Nessun riferimento a `Outcome` o al TurnLog. Il fix non cambia nessuna posizione: riscrive
   `MoveLog[i].Outcome` e `TgtCell`, e `Path` era già `{ Unit->Cell }` **prima** del ramo nuovo.
2. **Nessuno dei tre file è nel write-set**: `git diff --name-only a59671c8 fa9e3ed2 -- Source/`
   non contiene `RTStallDefinitionMeasureTests.cpp`, `RTUnbalancedProneTests.cpp` né
   `RTMatchAutobattleTests.cpp`.
3. **La causa comune è già in `main`.** `UI.Icon.Identity.Branth` viene dal rename
   `Hero.Riktor → Hero.Branth` (`748fc090`), e `git merge-base --is-ancestor 748fc090 origin/main`
   → **vero**: il commit è in `main`, non in questa wave. Lo stesso rename cambia il roster
   dell'arena generata su cui i due test di stallo misurano.
4. **Il test dichiara di sé che non è un difetto del bot**
   (`RTStallDefinitionMeasureTests.cpp:536-540`):
   > *«Un rosso qui NON è un difetto del bot, ed è il punto: significa che il margine registrato
   > in `OPEN_DECISIONS` è scaduto. Si rimisura e si aggiorna la riga — non si ritara la soglia.»*

⚠️ **Cosa NON ho fatto, e va detto**: non ho eseguito la full suite su `origin/main`. Sarebbe la
prova conclusiva, e costa un checkout più un rebuild su un branch che si è mosso tre volte sotto
questa sessione. Le quattro evidenze sopra sono convergenti e verificabili, ma restano un
**argomento**, non una misura su `main`. Finding `1-F12`.

---

## 6. Oracoli specifici della #79 — verificati sul TurnLog, non sul conteggio

Il mandato fissa `BlockedContested = 4`, `BlockedByUnit = 1`, `Moved = 1`. Lo scenario li
asserisce e passa `6/6` — ma un `6/6` è un conteggio, e l'ho verificato leggendo il TurnLog della
**mia** run:

```text
T2   Gadget: fermo: cella contesa (q=-1,r=0,L=0) (Action.Move, p50)     ┐ BlockedContested
     Branth: fermo: cella contesa (q=1,r=0,L=0) (Action.Move, p50)      ┘ (+2 al T1 = 4)
T3   Gadget: resta (q=-1,r=0,L=0) (Action.Move, p50)                    ← Stayed: non dichiarò
     Branth: si muove (q=1,r=0,L=0) -> (q=0,r=0,L=0) (1 celle)          ← Moved = 1
T4   Gadget: fermo: cella occupata (q=-1,r=0,L=0) (Action.Move, p50)    ← BlockedByUnit = 1
     Branth: resta (q=0,r=0,L=0) (Action.Move, p50)
```

| Oracolo | Atteso | Misurato | Esito |
|---|---:|---:|---|
| `Move/BlockedContested` | 4 | 4 | ✅ |
| `Move/BlockedByUnit` | 1 | 1 | ✅ |
| `Move/Moved` | 1 | 1 | ✅ |

### 6.1 «nessun tentativo ≠ tentativo bloccato»

🔑 **È il criterio che ha aperto la issue, ed è soddisfatto.** Al T3 `Gadget: resta`, al T4
`Gadget: fermo: cella occupata`. Prima del fix erano **la stessa identica stringa** —
`resta (q=-1,r=0,L=0) (Action.Move, p50)` a entrambi i turni, confermato in PIE nella seduta
`U14`. Le due righe sono ora semanticamente distinte leggendo la sola voce, senza il
`Warning [RT-Test]` del runner.

⚠️ Il `Warning: [RT-Test] … percorso rifiutato per 'A1'` **è ancora nel log**, ed è corretto che
ci sia: è diagnostica del runner, non più l'unica traccia. La sua presenza non riapre il difetto.

### 6.2 Replan invalido → valido, senza evento stale

Provato da `PlayerInteraction.ReplanAfterADenialWins` (`Result={Success}`), che asserisce
`ContaEsitoMove(BlockedByUnit) == 0` su tutto il log dopo un tentativo negato seguito da un piano
valido, una sola voce `Move`, `Outcome == Moved` e `TgtCell == Ripiego`. ✅

### 6.3 Ripetizione deterministica

`Movement.CollisionChoke` è stato eseguito **due volte** in questa sessione — nella run `Scenario`
e nella run `full` — con esito identico `PASS (6/6 assertion, 4 turni)`. I 21 test `Determinism`
della full suite passano. Lo scenario dichiara `seed 0`, `SEED_SOURCE: fixed`.

⚠️ Due esecuzioni nello stesso processo di misura non sono una prova di determinismo
cross-macchina: sono la ripetizione che i test e la spec correnti richiedono, ed è quanto il
mandato chiede. Non la estendo a un'affermazione più forte.

---

## MATRICE

Colonna VALIDATION, tetti da §7, sistemi in scope da §8 sul write-set dell'handoff DEV-LEAD.

| # | Sistema | Tetto | Verdetto | EVIDENCE_REF / REASON |
|---:|---|---|---|---|
| 1 | PROJECT | `PASS` | **`PASS`** | build carica il progetto, 0 errori |
| 3 | BUILD | `PASS` | **`PASS`** | `Result: Succeeded`, 0 errori — con la qualifica di §4 |
| 12 | PLANNING | `PASS` | **`PASS`** | 13/13 PlayerInteraction, i 4 test del piano per nome |
| 13 | READY/COMMIT | `PASS` | **`PASS`** | `ReplanAfterADenialWins` + `LockIn*` verdi nella run mirata |
| 15 | MOVEMENT | `PASS` | **`PASS`** | 19/19 HexMove + 174/174 Scenario |
| 25 | UI/HUD | `OBSERVED` | **`OBSERVED`** | il formatter produce due righe distinte (§6). La lettura **in partita** è `USER_REQUIRED` |
| 27 | COMBAT LOG | `PASS` | **`PASS`** | TurnLog di §6, verificato riga per riga sulla mia run |
| 28 | TURNLOG/REPLAY | `PASS` | **`PASS`** | TurnLog 69/69 · Replay 100/100 · Serialization 4/4 · Golden 12/12 |
| 29 | DETERMINISM | `PASS` | **`PASS`** | 21/21 + CollisionChoke ripetuto 2× con esito identico |
| 30 | NETWORK AUTHORITY | `PASS` | **`OBSERVED`** | i due campi sono `Transient`, non `Replicated`; delta replication 0. Non ho eseguito una prova con canary di connessione |
| 31 | PRIVACY | `PASS` | **`OBSERVED`** | idem. ⛔ Il tetto sarebbe `PASS`, ma il `PASS` richiede canary lato connessione: non l'ho fatto, quindi non lo emetto |
| 32 | AUTOMATION/SCENARIO | `PASS` | **`PASS`** | quattro run `VALIDA`, albero invariato durante ciascuna |
| 33 | ERRORS | `PASS` | **`PASS`** | 0 `Fatal` / `Assertion failed` nelle quattro run |
| 35 | SAVE/RELOAD | `PASS` | **`PASS`** | nessun cambio di formato; tracce v2–v7 leggibili, Replay 100/100 |
| 36 | PACKAGED | `PASS` | `N/A` | REASON: la Definition of Done viva della #79 non richiede packaged |
| 4 · 5 · 8 · 11 · 16 · 17 · 18 · 21 · 22 · 23 | ASSETS · BLUEPRINT · MAP · CAMERA · TARGETING · LOS/COVER · DAMAGE · REACTIONS · ENVIRONMENT · OBJECTIVES | — | `N/A` | REASON: fuori write-set (8 file, nessun asset, nessuna `.umap`) |

⚠️ **`PRIVACY` e `NETWORK AUTHORITY` restano `OBSERVED` benché il mio tetto sia `PASS`.** §7
assegna a VALIDATION la prova con canary lato connessione: non l'ho eseguita, e un tetto alto non
è un verdetto. La superficie di replica è misurata (delta 0, tre misure indipendenti: DEV-LEAD,
EDITOR, e il diff che non contiene `Replicated`), il **traffico** no.

---

## FINDINGS

```text
FINDING_ID:   issue-79-combat-log-blocked-move/1-F11
SEVERITY:     P2
EVIDENCE_REF: code: Source/RefactorTactics/Turn/RTTurnLogLibrary.cpp:415 (ramo di fallback)
              log:  docs/.../evidence/suite-val79-scenario-fa9e3ed2.log
                    -> "[RT] Branth: esito di movimento non tradotto (15) (q=-1,r=-2,L=0)"
              code: Source/RefactorTactics/Turn/RTTurnLog.h:448-475 (16 enumeratori)
ROOT_CAUSE:   il formatter traduce 13 dei 16 ERTMoveOutcome. Restano senza testo:
              BlockedByTopology (6), BlockedByCycle (12), Fell (15). Non sono casi teorici:
              BlockedByTopology è SCRITTO esplicitamente dal resolver (RTTurnManager.cpp:7361)
              e Fell si osserva 3 volte nella mia run di Scenario. Il giocatore legge
              «esito di movimento non tradotto (15)», che è esattamente il difetto che la #79
              esiste per chiudere — su un altro outcome.
              ⚠️ PREESISTENTE: lo stesso messaggio compare in Saved/Logs/rt-suite.log delle
              08:32 di oggi, sette ore prima che il fix fosse scritto. NON è una regressione
              di questa wave, ed è fuori dal suo write-set.
OWNER:        DEV-LEAD
REQUIRED_FIX: nessuno in questa wave. È materia della DoD di #79 — «ogni esito deve essere
              spiegabile leggendo il log» — e va deciso se chiuderlo qui o in una issue
              separata, come è stato fatto per 1-F9 con #2627.
REGRESSION:   un test che, per ogni valore di ERTMoveOutcome, asserisca che il formatter non
              cade nel ramo di fallback. Oggi nessun test copre l'esaustività dell'enum.
ATTEMPT:      1
```

```text
FINDING_ID:   issue-79-combat-log-blocked-move/1-F12
SEVERITY:     P2
EVIDENCE_REF: suite: docs/.../evidence/suite-val79-full-fa9e3ed2.log -> 2138/2138, 3 fallimenti
              shell: git merge-base --is-ancestor 748fc090 origin/main -> vero
              code:  Source/RefactorTactics/Tests/RTStallDefinitionProbeForTest.h:151-154
ROOT_CAUSE:   la full suite del branch è rossa su tre test che NON appartengono a questa wave:
              Bot.StallDefinitionsOnTheGeneratedTestArena, Match.Autobattle.Engages…,
              IconCatalog.RealCatalogCoversRequiredIds. La causa comune plausibile è il rename
              Hero.Riktor -> Hero.Branth (748fc090), già in origin/main: lascia scoperta
              UI.Icon.Identity.Branth e cambia il roster dell'arena generata su cui i due test
              di stallo misurano con margine zero.
OWNER:        DEV-LEAD
REQUIRED_FIX: (a) rimisurare il margine BOT-STALL-1 e aggiornare OPEN_DECISIONS, che è ciò che
              il test stesso prescrive per un proprio rosso; (b) coprire UI.Icon.Identity.Branth.
              Entrambi appartengono a main, non alla #79.
              ⛔ Finché restano rossi, «la suite è verde» non è dicibile su questo branch, e
              una PR che li porta dentro eredita tre rossi che non ha causato.
REGRESSION:   la full suite su origin/main deve mostrare gli STESSI tre rossi. NON l'ho
              eseguita: è la sola misura che trasformerebbe l'argomento di §5.2 in una prova.
ATTEMPT:      1
```

```text
FINDING_ID:   issue-79-combat-log-blocked-move/1-F13
SEVERITY:     P3
EVIDENCE_REF: (questo handoff, §2.2) — HEAD b5badb79 -> fa9e3ed2 durante la sessione
              RT3-VALIDATION-a59671c.md §3 — HEAD 75ab6287 -> a59671c8 durante la run precedente
              RT3-EDITOR-8dcd3a76.md 1-F8 — HEAD 8dcd3a76 -> b5badb79 prima della sessione EDITOR
ROOT_CAUSE:   il branch di wave si è mosso sotto una sessione di misura TRE volte, in tre ruoli
              diversi. Nessuna delle tre ha invalidato una misura — due volte per documentazione,
              una per PNG estranei — ma è coincidenza favorevole, non una protezione.
OWNER:        DEV-LEAD
REQUIRED_FIX: dichiarare una finestra in cui il branch di wave non riceve commit mentre un ruolo
              misura, oppure accettare formalmente la lettura «nessun delta sui path misurati»
              che 1-F8 chiede di decidere.
REGRESSION:   nessun test: è processo. Il rilevatore esiste già ed è il marker [RT-MEASURE] di
              rt-suite, che ha segnalato il movimento senza che nessuno glielo chiedesse.
ATTEMPT:      1
```

⚠️ **`1-F8` (divergenza HEAD/BASE_SHA) è RATIFICATO.** La mia sessione ha applicato la stessa
lettura di EDITOR e per la stessa ragione misurata. Due ruoli concordi non sostituiscono la
decisione di contratto che il Finding chiede: resta aperto verso DEV-LEAD.

⚠️ **`1-F9` (la destinazione negata non compare nella riga renderizzata) è CONFERMATO** e già
instradato in #2627. Non l'ho riaperto. Concordo con la lettura di EDITOR: il DoD chiede che il
log comunichi **il blocco** invece di «resta», e questo è soddisfatto.

✅ **`1-F3` (il terzo produttore) è CHIUSO.** Vedi §3.1.

---

## EVIDENCE

```text
suite:    rt-suite.ps1 -Filter "RefactorTactics.PlayerInteraction" -> exit 0, found 13, performed 13, passed 13, failed 0
suite:    rt-suite.ps1 -Filter "RefactorTactics.HexMove"           -> exit 0, found 19, performed 19, passed 19, failed 0
suite:    rt-suite.ps1 -Filter "RefactorTactics.Scenario"          -> exit 0, found 174, performed 174, passed 174, failed 0
suite:    rt-suite.ps1 -Filter "RefactorTactics"                   -> exit 0, found 2138, performed 2138, passed 2135, failed 3
log:      docs/.../evidence/build-validation-b5badb79.log            (Result: Succeeded, 0 errori)
log:      docs/.../evidence/suite-val79-playerinteraction-fa9e3ed2.log
log:      docs/.../evidence/suite-val79-hexmove-fa9e3ed2.log
log:      docs/.../evidence/suite-val79-scenario-fa9e3ed2.log        (TurnLog CollisionChoke T2-T4)
log:      docs/.../evidence/suite-val79-full-fa9e3ed2.log            (i 3 rossi, nominati)
turnlog:  §6 di questo handoff — righe T2/T3/T4 di Movement.CollisionChoke dalla run VALIDATION
lease:    416e5d77a740 SUITE VALIDATION task 79 -> RELEASED
lease:    bfcc9151c27e BUILD VALIDATION task 79 -> RELEASED
shell:    find Source/ -newer Binaries/Win64/UnrealEditor-RefactorTactics.dll -type f -> (vuoto)
shell:    git diff --stat b5badb79 fa9e3ed2 -- Source/ Scenarios/ -> (vuoto)
shell:    git merge-base --is-ancestor 748fc090 origin/main -> vero
code:     Source/RefactorTactics/Tests/RTStallDefinitionProbeForTest.h:151-154 (stallo per posizione)
```

I log completi delle quattro run vivono in `Saved/Logs/rt-suite-val79-*.log` (13 MB, non
versionati: il repository ha LFS disattivato). In `evidence/` stanno gli **estratti**, che portano
il riassunto `[RT-MEASURE]`, gli esiti per nome dei test della wave, il TurnLog di CollisionChoke
e l'elenco dei fallimenti.

---

## USER_REQUIRED

```text
PIE-V01-LOG                       Result: NOT RUN
  DoD della #79, oracolo umano. hud-v01-editor-verification-roadmap.md:71 — sedute U15 · U43 · U46.

PIE-V01-COLL clausola (c)         Result: NOT RUN
  Registrata ❌ in docs/technical/test-manuali-pie.md:1001, voce ferma a 🟡 in attesa di questa
  issue. Va RIMISURATA ora che il fix è atterrato: è il check che chiede se «ho provato e me
  l'hanno negato» sia distinguibile A SCHERMO, non solo nel TurnLog.

Riga del combat log al turno 4    Result: NOT RUN
  Attesa Editor-visible dichiarata dal WORK-ORDER e non verificata da nessun ruolo: EDITOR non ha
  avviato PIE. Headless ho provato che il formatter produce «fermo: cella occupata»; che il
  giocatore la legga in partita è oracolo umano.
```

---

## P0

nessuno.

## P1

nessuno.

## P2

- `1-F11` — tre `ERTMoveOutcome` senza testo nel formatter (`BlockedByTopology`, `BlockedByCycle`,
  `Fell`). Preesistente, fuori write-set, **dentro il dominio della DoD di #79**.
- `1-F12` — la full suite del branch eredita da `main` tre test rossi. Non causati da questa wave,
  ma impediscono di dire «suite verde» sul branch.

## P3

- `1-F13` — il branch di wave si è mosso sotto una sessione di misura tre volte, in tre ruoli.
- `1-F8` — **ratificato**, resta aperto: §5 va letto come uguaglianza stretta o come «nessun delta
  sui path misurati»? È decisione di contratto.
- `1-F9` — **confermato**, già instradato in #2627.

---

## Verdetto sulla chiusura della #79

Il mandato chiede esplicitamente di distinguere «la suite mirata è verde» da «la issue è
tecnicamente chiusa». Sono due cose diverse e qui divergono.

**La wave ha fatto ciò che il suo contratto dichiara.** Il difetto che ha aperto la issue è
chiuso e misurato: al turno 4 di `CollisionChoke` il TurnLog porta `Move/BlockedByUnit` con
`TgtCell` = la destinazione negata, e la riga del combat log non è più identica a quella di chi
non ha dichiarato nulla. Nessun sistema in scope è `FAIL`.

**La Definition of Done viva della #79 non è soddisfatta**, e non per un dettaglio:

1. `PIE-V01-LOG` è `NOT RUN` — è la verifica PIE che la issue nomina fra i propri test, e nessun
   ruolo di questa wave ha avviato PIE;
2. la clausola **(c)** di `PIE-V01-COLL` è ancora registrata `❌` e va rimisurata: è il check che
   chiede se il diniego sia leggibile **a schermo**, che è precisamente ciò che la seduta `U14` ha
   trovato muto;
3. `1-F11` tocca la prima riga della DoD — *«ogni voce riporta … e ogni esito deve essere
   spiegabile leggendo il log»* — e tre esiti restano illeggibili.

⛔ **Una chiusura decisa sulla sola suite verde chiuderebbe la #79 sopra la parte di DoD che
nessun test headless osserva** — ed è lo stesso errore che ha reso necessaria questa wave: il
difetto era invisibile a tutti gli oracoli automatici e lo ha trovato un occhio umano in PIE.

Il punto 3 è materia di decisione: se `1-F11` esce in una issue separata come `1-F9`, la DoD di
questa issue va letta di conseguenza e la decisione va scritta, non sottintesa.

---

STATUS:   PARTIAL
REASON:   Tutti i gate eseguibili sono verdi e gli oracoli della wave sono verificati sul
          TurnLog, non su un conteggio. `DONE` non è emettibile per §13: la Definition of Done
          viva della #79 porta due check a oracolo umano `NOT RUN` (`PIE-V01-LOG`, clausola (c)
          di `PIE-V01-COLL`), e la full suite del branch eredita da `main` tre test rossi
          (`1-F12`) che rendono indicibile «suite verde» finché restano.
UNBLOCK:  Per portare la wave a `DONE`:
          1. eseguire `PIE-V01-LOG` e rimisurare la clausola (c) di `PIE-V01-COLL` in una seduta
             PIE — sono USER_REQUIRED, non li produce nessuna suite;
          2. decidere `1-F11`: chiuso qui o issue separata come `1-F9`/#2627, e scrivere la
             decisione perché tocca la prima riga della DoD;
          3. sanare o dichiarare `1-F12` — i tre rossi sono di `main`, e la prova conclusiva è
             una full suite su `origin/main` che mostri gli stessi tre;
          4. decidere `1-F8`: come si legge §5 quando il delta non tocca i path misurati.
          ⛔ Nessuno dei quattro punti chiede una modifica al codice di questa wave. Non ho
          toccato produzione: §12 vieta a VALIDATION di riparare e poi approvare sé stessa.

RISULTATO: PARTIAL
