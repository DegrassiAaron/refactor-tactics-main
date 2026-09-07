=== RT3 HANDOFF ===

FROM:          EDITOR
TO:            VALIDATION
FEATURE:       issue-79-combat-log-blocked-move
WAVE_ID:       issue-79-combat-log-blocked-move/1

BRANCH:        fix/79-blocked-move-turnlog
PARENT_BRANCH: main
BASE_SHA:      8dcd3a765251f012961ea87c4bf518a50d60e014
               (feature PRODUCED_SHA 55e31404705c7883f8cc60eb80be6ddfdac9c760, letto
                dall'INPUT_HANDOFF — 8dcd3a76 e' il commit documentale che lo rende disponibile)
PRODUCED_SHA:  8dcd3a76 — uguale a BASE_SHA: nessuna scrittura binaria, nessun commit.
HEAD_AT_OPEN:  b5badb7903c0b6d6aa4023bdea905f644ab45852
HEAD_AT_CLOSE: b5badb79 — invariato per tutta la sessione

INPUT_HANDOFF: docs/rt-three-terminals/waves/issue-79-combat-log-blocked-move/RT3-DEVLEAD-55e3140.md
               letto dal filesystem, 18507 byte

WRITE_SET:     docs/rt-three-terminals/waves/issue-79-combat-log-blocked-move/evidence/
                 build-editor-b5badb79.log
                 suite-scenario-b5badb79.log
                 suite-playerinteraction-b5badb79.log
                 suite-hexmove-b5badb79.log
                 collisionchoke-turnlog-b5badb79.log
               docs/rt-three-terminals/waves/issue-79-combat-log-blocked-move/RT3-EDITOR-8dcd3a76.md
BINARY_ASSETS: none

STATUS: PARTIAL

⚠️ `PARTIAL` e non `READY`: i tre check che tengono aperta la #79 sono a **oracolo umano** e restano
`NOT RUN`. Nessun sistema e' `FAIL`. Vedi § `USER_REQUIRED`.

---

```text
RT3 INIT

Tipo:               EDITOR
Feature:            issue-79-combat-log-blocked-move
Wave:               issue-79-combat-log-blocked-move/1
Branch:             fix/79-blocked-move-turnlog
Parent branch:      main
Base SHA:           8dcd3a76
Engine:             5.8   (EngineAssociation da RefactorTactics.uproject — non dedotto)
Mappa:              nessuna .umap aperta. Scenario Scenarios/Movement/CollisionChoke.json
Write-set in scope: 8 file dall'INPUT_HANDOFF (7 Source/ + 1 Scenarios/)
Sistemi in scope:   PLANNING · READY/COMMIT · MOVEMENT · COMBAT LOG · AUTOMATION/SCENARIO
                    TURNLOG/REPLAY · UI/HUD · SAVE/RELOAD · BUILD · DETERMINISM
                    NETWORK AUTHORITY · PRIVACY  (non-degrado)
Binary ownership:   nessuna — BINARY_ASSETS: none
Lease:              6cc5a254f710 · EDITOR · task 79 · acquisito 14:45:27Z · RELEASED
```

---

## 1. Preflight §4

| Campo | Valore | Esito |
|---|---|---|
| `FEATURE` | `issue-79-combat-log-blocked-move` | risolto |
| `BRANCH` | `fix/79-blocked-move-turnlog` | risolto, corrisponde a `--abbrev-ref HEAD` |
| `BASE_SHA` | `8dcd3a76…` | risolto — valore reale, non placeholder |
| `INPUT_HANDOFF` | `…/RT3-DEVLEAD-55e3140.md` | risolto — file esistente, 18507 byte, letto dal filesystem |

Quattro campi su quattro risolti: §4 non blocca. La run `RT3-EDITOR-a59671c.md`
(`BLOCKED / MISSING_INPUT`) resta evidenza storica e **non** e' il verdetto di questa run.

## 2. Precondizioni §5 — divergenza dichiarata, non ignorata

```text
BASE_SHA  8dcd3a765251f012961ea87c4bf518a50d60e014
HEAD      b5badb7903c0b6d6aa4023bdea905f644ab45852
BRANCH    fix/79-blocked-move-turnlog          ✅ corrisponde
STATUS    pulito all'apertura                  ✅
```

`HEAD` non corrisponde a `BASE_SHA`: e' avanzato di **un** commit, `b5badb79 "maps"`.

```text
git diff --stat 8dcd3a76..HEAD
  docs/research/maps/map.fiume.large.png     | Bin 0 -> 2702638 bytes
  docs/research/maps/map.fiume.long.png      | Bin 0 -> 2413834 bytes
  docs/research/maps/section.ponte-fiume.png | Bin 0 -> 2677248 bytes

git diff --name-only 8dcd3a76..HEAD -- Source/ Content/ Config/ Scenarios/ docs/rt-three-terminals/
  (vuoto)
```

Tre PNG di ricerca. **Zero** file compilati, zero asset caricati dal gioco, zero file della wave.
La divergenza non puo' influenzare binario, PIE o TurnLog, e la misura resta attribuibile: ogni
`EVIDENCE_REF` di questo handoff cita `b5badb79`, non `8dcd3a76`. Finding `1-F8`.

⚠️ `HEAD` e' rimasto `b5badb79` dall'apertura alla chiusura: nessuna finestra di misura e' stata
attraversata da un movimento di `HEAD`.

## 3. Build — il presupposto che veniva prima di tutto

Il binario era **stale**, e non di poco:

```text
UnrealEditor-RefactorTactics.dll   PRIMA: 2026-09-06 09:48:38  (14674944 byte)
Source/.../RTUnit.cpp              modificato: 2026-09-06 15:47:16
Source/.../RTTurnManager.cpp       modificato: 2026-09-06 15:53:58
```

🔑 **Aprire PIE senza ricompilare avrebbe misurato il codice precedente al fix** — cioe' avrebbe
riprodotto il difetto della #79 e me lo sarei potuto leggere come conferma che il fix non funziona.
`RT3_CONTRACT.md` §3: *file modificato != build verificato*.

```text
Build.bat RefactorTacticsEditor Win64 Development -waitmutex
  -> Result: Succeeded        (0 occorrenze di "error C" / "error LNK" / "fatal error")
  -> DLL DOPO: 2026-09-06 16:47:17  (14749184 byte)
```

## MATRICE

Tetti da §7, colonna `EDITOR max`. Dove il tetto e' `OBSERVED` il verdetto **non** e' un `PASS`
anche quando l'evidenza e' forte: quel ruolo non possiede lo strumento che lo prova.

| # | Sistema | Tetto | Verdetto | Note |
|---:|---|---|---|---|
| 1 | PROJECT | `PASS` | **`PASS`** | l'Editor carica col binario nuovo, `Engine is initialized` |
| 3 | BUILD | `OBSERVED` | **`OBSERVED`** | `Result: Succeeded`, 0 errori. Il `PASS` e' di VALIDATION |
| 12 | PLANNING | `PASS` | **`OBSERVED`** | provato dai test 1-4, **non** in PIE: non alzo il verdetto sopra cio' che ho visto |
| 13 | READY/COMMIT | `PASS` | `NOT RUN` | REASON: richiede una partita PIE |
| 15 | MOVEMENT | `PASS` | **`OBSERVED`** | test 5-6 + `CollisionChoke`; nessuna osservazione in partita |
| 25 | UI/HUD | `PASS` | `NOT RUN` | REASON: PIE non avviato — il widget del combat log non e' stato visto |
| 27 | COMBAT LOG | `PASS` | **`OBSERVED`** | il **formatter** produce due righe distinte (vedi sotto). La lettura **in partita** e' `USER_REQUIRED` |
| 28 | TURNLOG/REPLAY | `OBSERVED` | **`OBSERVED`** | `CollisionChoke` 6/6 include `BlockedByUnit = 1` |
| 29 | DETERMINISM | `OBSERVED` | `NOT RUN` | REASON: nessuna ripetizione eseguita. `SEED_SOURCE: fixed (seed 0)` dichiarato dallo scenario |
| 30 | NETWORK AUTHORITY | `OBSERVED` | **`OBSERVED`** | nessuna `UPROPERTY(Replicated)` aggiunta; i due campi sono `Transient` |
| 31 | PRIVACY | `OBSERVED` | **`OBSERVED`** | delta replication = 0, misurato in proprio. ⛔ Non e' una prova: vedi sotto |
| 32 | AUTOMATION/SCENARIO | `OBSERVED` | **`OBSERVED`** | tre run `VALIDA`, 0 fallimenti. Il verdetto di suite e' di VALIDATION |
| 33 | ERRORS | `PASS` | **`PASS`** | 0 `Error` / `Fatal` / `Assertion failed` nel log dell'Editor |
| 35 | SAVE/RELOAD | `PASS` | `NOT RUN` | REASON: nessun asset toccato, nessun dirty state da salvare |
| 4 | ASSETS | `PASS` | `N/A` | REASON: `BINARY_ASSETS: none` — fuori write-set |
| 5 | BLUEPRINT | `PASS` | `N/A` | REASON: fuori write-set |
| 8 | MAP | `PASS` | `N/A` | REASON: fuori write-set, nessuna `.umap` |
| 36 | PACKAGED | `N/A` | `N/A` | REASON: tetto `N/A` per EDITOR |

⚠️ **`PRIVACY` resta `OBSERVED` e non diventa `PASS` nemmeno con delta 0.** §7: l'assenza di un
dato dalla UI avversaria non prova la sua assenza sul client. La prova richiede canary lato
connessione e appartiene a VALIDATION. Ho misurato la *superficie di replica*, non il traffico.

## COMBAT LOG / TURNLOG COMPARISON

L'oracolo e' stato **rilevato dal formatter corrente**, non inventato:

| Caso | `ERTMoveOutcome` | Testo | Sede |
|---|---|---|---|
| A | `Stayed` | `resta` | `RTTurnLogLibrary.cpp:400` |
| B | `BlockedByUnit` | `fermo: cella occupata` | `RTTurnLogLibrary.cpp:362` |

`CollisionChoke` contiene entrambi i casi **nella stessa partita**, ed e' il confronto che la #79
chiede. Osservato su `b5badb79`, binario `16:47:17`:

```text
T3   [RT] Gadget: resta (q=-1,r=0,L=0) (Action.Move, p50)                     <- CASO A
     [RT] Branth: si muove (q=1,r=0,L=0) -> (q=0,r=0,L=0) (1 celle) (Action.Move, p50)
T4   [RT] Gadget: fermo: cella occupata (q=-1,r=0,L=0) (Action.Move, p50)     <- CASO B
     [RT] Branth: resta (q=0,r=0,L=0) (Action.Move, p50)
```

🔑 **Le due righe sono ora semanticamente diverse.** Prima del fix erano la stessa stringa —
`resta (q=-1,r=0,L=0) (Action.Move, p50)` ai turni 3 e 4 — ed e' il reperto che ha aperto la issue.

Confronto col TurnLog autoritativo:

| Elemento richiesto | Nel TurnLog | Nella riga del combat log |
|---|---|---|
| unita' | ✅ `UnitId` | ✅ `Gadget` |
| reason | ✅ `Outcome = BlockedByUnit` | ✅ «fermo: cella occupata» |
| cella sorgente | ✅ `SrcCell` | ✅ `(q=-1,r=0,L=0)` |
| **destinazione richiesta** | ✅ `TgtCell = (0,0,0)` | 🔴 **assente** — vedi `1-F9` |
| ActionId | ✅ | ✅ `Action.Move` |
| Priority | ✅ dal catalogo | ✅ `p50` |
| ordine | ✅ invariato | ✅ stessa posizione, nessuna voce aggiunta |

Il conteggio autoritativo e' verificato dall'assertion dello scenario:

```text
Movement.CollisionChoke: PASS (6/6 assertion, 4 turni)
```

Le sei includono `LogEventCount / Move / BlockedByUnit = 1`, che alla misura del 2026-09-04 dava
`0`. ⚠️ Questo prova il **percorso harness**. La #79 nasce dal fatto che *«l'unica traccia del
diniego la emetteva `RTScenarioSession`: in una partita normale quella riga non esisteva»* — per
questo il percorso **player** e' stato misurato a parte.

## PIE RESULT

```text
Mappa:        nessuna — PIE non avviato
Modo:         n/a
Player:       n/a
SEED:         0            (dichiarato da CollisionChoke.json, non osservato dopo)
SEED_SOURCE:  fixed
Scenario:     Movement.CollisionChoke  (eseguito headless, non in PIE)
Atteso:       T3 «resta» ≠ T4 «fermo: cella occupata»
Osservato:    conforme nel canale testuale del formatter; NON osservato nella UI di partita
TurnLog:      6/6 assertion, BlockedByUnit = 1
```

**`PIE RESULT: NOT RUN`.** L'Editor e' stato avviato, ha caricato senza errori ed e' stato chiuso.
La partita PIE non e' stata giocata: i tre check richiedono il percorso reale del giocatore —
selezione, pianificazione, `Ready`, quattro turni — e il loro oracolo e' umano.

⛔ Non converto in `PASS` cio' che ho letto in un log headless. `animation success != simulator
correctness` ha un gemello che vale qui: **log headless != lettura in partita**.

## EVIDENCE

```text
build:    docs/.../evidence/build-editor-b5badb79.log            -> "Result: Succeeded", 0 errori
suite:    ./scripts/rt-suite.ps1 -Filter RefactorTactics.Scenario
          -> VALIDA, HEAD b5badb79, albero 3353f578, 174/174, 0 fallimenti, 00:50
suite:    ./scripts/rt-suite.ps1 -Filter RefactorTactics.PlayerInteraction
          -> VALIDA, HEAD b5badb79, albero 3a37b836, 13/13, 0 fallimenti, 00:29
suite:    ./scripts/rt-suite.ps1 -Filter RefactorTactics.HexMove
          -> VALIDA, HEAD b5badb79, albero 3a37b836, 19/19, 0 fallimenti, 00:26
turnlog:  docs/.../evidence/collisionchoke-turnlog-b5badb79.log   -> T3/T4, righe A e B
log:      Saved/Logs/rt-suite.log  -> "Movement.CollisionChoke: PASS (6/6 assertion, 4 turni)"
log:      Saved/Logs/RefactorTactics.log#L2287 -> "Engine is initialized"
code:     Source/RefactorTactics/Turn/RTTurnLogLibrary.cpp:362 · :400 · :434-442
shell:    git grep -c 'UPROPERTY(.*Replicated' {a59671c8|55e31404|b5badb79} -- Source/
          -> 10 occorrenze / 5 file su tutti e tre
shell:    git diff --name-only 8dcd3a76..HEAD -- Source/ Content/ Config/ Scenarios/ -> (vuoto)
```

I sei test consegnati sono stati verificati **per nome**, non per cardinalita':
`DeniedMoveDeclaresTheDenial`, `UndeclaredMoveDoesNotDeclareADenial`, `ReplanAfterADenialWins`,
`BudgetDenialIsNotAUnitDenial`, `DenialAndStillnessAreDistinguishable`,
`DeclaredDestinationDeniedByOccupantDeclaresIt` — 7 occorrenze ciascuno in `rt-suite.log`. Presente
anche la non-regressione `ContestedCellStopsBoth`.

⛔ **Nessun `EVIDENCE_REF` di PIE o packaged: non ne esistono.** Il gate `4 | GoldenCorpus` dei
comandi DEV-LEAD **non** e' stato eseguito: appartiene a VALIDATION, e l'ordine `3 -> 4` che
l'handoff dichiara vincolante non va spezzato da me.

## `USER_REQUIRED`

```text
=== USER EDITOR CHECK ===

ID:            PIE-V01-LOG
Mappa:         mappa di partita standard (non L_DevSandbox)
Modo PIE:      Play in Editor, 1 player
Player:        umano su team 0
Precondizioni: branch fix/79-blocked-move-turnlog, binario >= 2026-09-06 16:47:17

Passi:
1. avvia PIE e apri il pannello del combat log;
2. turno N: non dichiarare alcun movimento per un'unita'; Ready; osserva la riga;
3. turno N+1: dichiara come piano FINALE una destinazione occupata da un'altra unita';
   Ready; osserva la riga.

Atteso:            passo 2 -> «resta …»
                   passo 3 -> «fermo: cella occupata …»
Segnali fallimento: le due righe coincidono; oppure il passo 3 dice «resta»
Evidenza richiesta: screenshot delle due righe + estratto TurnLog dello stesso turno

Result: NOT RUN
```

```text
=== USER EDITOR CHECK ===

ID:            PIE-V01-COLL clausola (c)
Precondizioni: come sopra. Oggi la clausola e' registrata ❌ e la voce resta 🟡 apposta
Passi:         riproduci il choke di CollisionChoke in partita: due unita', un solo varco;
               cedi il varco all'avversario, poi tenta di entrarci al turno successivo
Atteso:        il diniego e' leggibile in partita, non solo nel runner degli scenari
Evidenza richiesta: screenshot + TurnLog

Result: NOT RUN
```

```text
=== USER EDITOR CHECK ===

ID:            Lettura in partita del Turno 4 di CollisionChoke
Precondizioni: come sopra
Passi:         riproduci la sequenza T1-T4 dello scenario col percorso del giocatore
Atteso:        T3 «resta (q=-1,r=0,L=0)» ≠ T4 «fermo: cella occupata (q=-1,r=0,L=0)»
               — le stesse due righe che questo handoff ha osservato headless
Segnali fallimento: T4 dice «resta»; oppure la voce non compare affatto
Evidenza richiesta: screenshot dei due turni

Result: NOT RUN
```

`NOT RUN` resta `NOT RUN` finche' una persona non risponde. Non lo converto.

## FINDINGS

```text
FINDING_ID:   issue-79-combat-log-blocked-move/1-F8
SEVERITY:     P3
EVIDENCE_REF: shell: git rev-parse HEAD -> b5badb79 (BASE_SHA dichiarato 8dcd3a76)
              shell: git diff --name-only 8dcd3a76..HEAD -- Source/ Content/ Config/ Scenarios/
                     docs/rt-three-terminals/ -> (vuoto)
              shell: git diff --stat 8dcd3a76..HEAD -> 3 PNG in docs/research/maps/
ROOT_CAUSE:   un commit estraneo alla wave e' atterrato sul branch fra l'emissione
              dell'handoff DEV-LEAD e la convocazione di EDITOR. §5 chiede HEAD == BASE_SHA
              e la lettera del contratto direbbe BLOCKED.
OWNER:        DEV-LEAD
REQUIRED_FIX: nessuna modifica di codice. Decidere se §5 vada letto come uguaglianza stretta
              o come «nessun delta sui path che la misura tocca». Questa run ha applicato la
              seconda lettura DICHIARANDOLA, perche' bloccare per tre PNG non avrebbe protetto
              nessuna misura — ma la scelta appartiene al contratto, non a chi misura.
REGRESSION:   se la prima lettura e' quella giusta, il branch di wave va protetto dai commit
              estranei fino al sign-off
ATTEMPT:      1
```

```text
FINDING_ID:   issue-79-combat-log-blocked-move/1-F9
SEVERITY:     P3
EVIDENCE_REF: code: Source/RefactorTactics/Turn/RTTurnLogLibrary.cpp:434-441
              turnlog: docs/.../evidence/collisionchoke-turnlog-b5badb79.log
                       -> "Gadget: fermo: cella occupata (q=-1,r=0,L=0) (Action.Move, p50)"
ROOT_CAUSE:   il formatter stampa la coppia `Src -> Tgt` SOLO per Moved, Displaced, Slid,
              SlideBlocked e SupersededByDash; BlockedByUnit cade nel ramo breve
              (`Reason + SrcCell + Cause`). La destinazione negata (0,0,0) e' nel TurnLog ma
              NON compare nella riga letta dal giocatore.
              ⚠️ L'handoff DEV-LEAD motiva `TgtCell` proprio cosi': «la coppia SrcCell ->
              TgtCell descrive la rotta che NON e' stata percorsa, come in SupersededByDash».
              Il commento a :425-427 usa lo stesso argomento per mettere SupersededByDash nel
              ramo lungo — «un rendering che stampa solo SrcCell la nasconde». L'argomento vale
              identico per BlockedByUnit, ma l'outcome e' rimasto nel ramo breve.
OWNER:        DEV-LEAD
REQUIRED_FIX: nessuno in questa wave. ⛔ Non ho toccato produzione per allineare il testo
              all'aspettativa: il DoD della #79 chiede che il log comunichi IL BLOCCO invece di
              «resta», e quello e' soddisfatto. Decidere se la destinazione richiesta debba
              comparire e', se accolto, una issue separata.
REGRESSION:   se accolto: un test che asserisce la presenza di TgtCell nella riga renderizzata
ATTEMPT:      1
```

```text
FINDING_ID:   issue-79-combat-log-blocked-move/1-F10
SEVERITY:     P3
EVIDENCE_REF: log: prima run -> "[RT-MEASURE] NON VALIDA … albero 3353f578 -> 88c9afdc
              il contenuto e' cambiato durante la run … esito 174/174, 0 fail -> NON REGISTRABILE"
ROOT_CAUSE:   ERRORE DI QUESTA SESSIONE. Il log della suite veniva scritto con Tee-Object
              DENTRO la working directory (evidence/) mentre rt-suite ne misurava l'albero:
              la misura si e' invalidata da sola. 174/174 con 0 fallimenti — e non registrabile.
OWNER:        EDITOR
REQUIRED_FIX: gia' applicato: log nello scratchpad fuori dal repo, copiato in evidence/ DOPO
              la fine della run. Le tre misure registrate qui sono le rilanciate, tutte VALIDA.
REGRESSION:   nessuno strumento che scrive nel working tree durante una finestra di misura
ATTEMPT:      1
```

⚠️ **`1-F7` (replication footprint) e' confermato risolto, con misura indipendente.** Non ho usato
il criterio assoluto «resta a 2»: baseline `a59671c8` = **10 occorrenze / 5 file**, `55e31404` = 10/5,
`b5badb79` = 10/5. **Delta = 0.** Nessuna riga e' stata modificata per soddisfare un numero.

## Lifecycle

```text
lease acquire  -> ACQUIRED 6cc5a254f710 (EDITOR, task 79)      14:45:27Z
build          -> Result: Succeeded
suite x3       -> VALIDA
editor start   -> PID 24440
editor load    -> "Engine is initialized"   (0 Error / Fatal / Assertion)
PIE            -> non avviato
dirty state    -> nessun asset modificato; solo evidence/ untracked
editor close   -> CloseMainWindow -> TERMINATO pulitamente
processi UE    -> nessuno residuo (verificato)
lease release  -> RELEASED
```

⛔ Nessun commit creato. `PRODUCED_SHA` resta `BASE_SHA` perche' non ci sono scritture binarie —
non ho fatto avanzare `HEAD` per avere uno SHA nuovo.

---

STATUS:   PARTIAL
REASON:   nessun sistema in scope e' `FAIL`. I tre check che chiudono la #79 sono a oracolo
          umano e restano `NOT RUN`; `PIE` e `UI/HUD` non sono stati misurati.
UNBLOCK:  una seduta PIE umana che risponda ai tre `USER EDITOR CHECK`. Per VALIDATION:
          i gate 4 (`GoldenCorpus`) e 5 (suite intera) restano da eseguire, nell'ordine
          dichiarato dall'handoff DEV-LEAD.
