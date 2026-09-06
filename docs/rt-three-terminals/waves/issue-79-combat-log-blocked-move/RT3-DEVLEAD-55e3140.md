=== RT3 HANDOFF ===

FROM:          DEV-LEAD
TO:            EDITOR
FEATURE:       issue-79-combat-log-blocked-move
WAVE_ID:       issue-79-combat-log-blocked-move/1

BRANCH:        fix/79-blocked-move-turnlog
PARENT_BRANCH: main
BASE_SHA:      a59671c87a7fe3407c1d3158521280267857d20b
PRODUCED_SHA:  55e31404705c7883f8cc60eb80be6ddfdac9c760

WRITE_SET:     Source/RefactorTactics/Unit/RTUnit.h
               Source/RefactorTactics/Unit/RTUnit.cpp
               Source/RefactorTactics/Player/RTPlayerController.cpp
               Source/RefactorTactics/Turn/RTTurnManager.cpp
               Source/RefactorTactics/ScenarioHarness/RTScenarioSession.cpp   (integrazione DEV-LEAD)
               Source/RefactorTactics/Tests/RTPlayerInteractionTests.cpp
               Source/RefactorTactics/Tests/RTHexMovementIntegrationTests.cpp
               Scenarios/Movement/CollisionChoke.json
BINARY_ASSETS: nessuno

STATUS: READY

⚠️ `READY` significa che la wave ha un **ingresso leggibile** per EDITOR, non che qualcosa sia stato verificato. Nessun gate è stato eseguito da questo ruolo, e §6 vale: `NOT RUN` non conta come `PASS`.

---

```text
RT3 INIT

Tipo:               DEV-LEAD
Feature:            issue-79-combat-log-blocked-move
Wave:               issue-79-combat-log-blocked-move/1
Branch:             fix/79-blocked-move-turnlog
Parent branch:      main
Base SHA:           a59671c8
HEAD:               55e31404 (= PRODUCED_SHA prima di questo handoff)
Working tree:       pulito
Contributi letti:   contrib/DEV-MAIN-39180-02.md · contrib/DEV-TEST-60316-02.md  (entrambi READY)
                    i rispettivi -01 sono SUPERSEDED e non sono l'implementazione corrente
Scope assegnati:    disgiunti, dichiarati in WORK-ORDER.md; verificati contro il write-set reale
Write-set:          8 file — 4 DEV-MAIN · 3 DEV-TEST · 1 integrazione DEV-LEAD
Attempt:            nessun FINDING_ID riaperto (tutti al primo ciclo)
```

## Verifiche di consolidamento

Misurate prima di scrivere, non presunte.

| Controllo | Esito |
|---|---|
| write-set reale vs scope assegnati | ✅ 7 file modificati = 4 dichiarati da DEV-MAIN + 3 da DEV-TEST, **nessuna sovrapposizione** |
| modifiche non attribuibili nel working tree | ✅ **nessuna**: `diff` fra i path misurati e quelli dichiarati è vuoto |
| `Turn/RTTurnLog.h` | ✅ **non toccato** — nessun enumeratore nuovo, nessun bump di formato |
| `Tests/Golden/**` | ✅ **non toccati** — nessuna rigenerazione per previsione |
| `Scenarios/Movement/CollisionChoke.json` | ✅ un solo `expected` cambiato: `BlockedByUnit` `0` → `1`. Gli altri quattro invariati |
| delta replication footprint | ✅ **0** — baseline `a59671c8` = 10 occorrenze, `PRODUCED_SHA` = 10, aggiunte = 0 |
| gate d'ingresso di `1-F6` | ✅ `git diff --name-only a59671c8..55e31404 -- Source/ Scenarios/` → **8 file**, non vuoto |
| working tree dopo il commit | ✅ pulito |

## CONTRATTO COMPORTAMENTALE

| Campo | Valore |
|---|---|
| **Given** | un'unità viva in `Planning` il cui piano di movimento **finale** è negato perché la destinazione richiesta è occupata da un'altra unità — per rifiuto in pianificazione (player, Scenario Harness) o per fallimento del percorso alla risoluzione (bot) |
| **When** | la fase `Move` si risolve e per quell'unità non esiste percorso percorribile (`Path.Num() < 2`) |
| **Then** | la sua **unica** voce `Phase = Move, Category = Move` porta `Outcome = ERTMoveOutcome::BlockedByUnit` **al posto** di `Stayed`, con `TgtCell` = destinazione richiesta |
| **Authority** | server / `ARTTurnManager`. Classificazione e scrittura sono autoritative; `CLIENT PROPOSES → SERVER VALIDATES → SERVER APPLIES` invariato |
| **Timing boundary** | nasce in `Planning` (al collasso, per il bot), si consuma nella fase `Move`. **Nessuna scrittura al lock-in**: `RTTurnManager.cpp:3218` resta intatto |
| **Target/recipient** | l'unità che ha dichiarato. `SrcCell` = sua cella di partenza; la voce **non nomina l'occupante** |
| **Failure** | rifiuto per budget, cella non percorribile o fuori mappa ⇒ `Stayed`, invariato. Il motivo si legge da `ClassifyWaypointCell`, non si inventa |
| **Fallback** | assenza di rifiuto ⇒ `Stayed`, come prima. Additivo sul dato, sostitutivo sull'esito |
| **Ordering** | invariato: stessa voce, stessa posizione nell'ordine d'input. Nessuna voce aggiunta né rimossa. Nessuna iterazione di `TMap`/`TSet` |
| **TurnLog** | `UnitId` · `SrcCell` (partenza) · `TgtCell` (destinazione negata) · `ActionId = Action.Move` e `Priority` dal **catalogo**, letti da `BuildMoveLog` · `Outcome = BlockedByUnit` · `Amount = 0` · `TurnNumber`, `GraphRevision`, verdetto di visibilità invariati |
| **Replay** | nessun campo nuovo, nessun enumeratore nuovo, **nessun bump di formato**. Cambia il valore di `Outcome` e `TgtCell` su una voce già esistente. Le tracce v2–v7 restano leggibili |
| **Privacy** | non degradata, misurata: **zero** `UPROPERTY(Replicated)` aggiunte. I due campi sono `Transient`, non `BlueprintReadWrite`, e viaggiano sulla stessa strada di `PlannedWaypoints` |
| **SEED_SOURCE** | `none` — nessun RNG sul percorso, dal click alla voce |
| **Editor-visible expectation** | in PIE, il turno 4 di `PIE-V01-COLL` deve leggere «fermo: cella occupata» invece di «resta», col testo già esistente (`RTTurnLogLibrary.cpp:362`). ⚠️ È un'**attesa** per EDITOR, mai un risultato: `Result: NOT RUN` |

### I tre casi

| Caso | Situazione | Atteso |
|---|---|---|
| **A** | nessun movimento dichiarato | `Move / Stayed` — invariato |
| **B** | movimento dichiarato, destinazione finale occupata | `Move / BlockedByUnit`, `TgtCell` = destinazione richiesta |
| **C** | tentativo rifiutato, poi replan valido | il piano valido prevale: nessun rejection stale |

➕ **Caso D, deciso in questo consolidamento** — «occupato, poi rifiutato per un motivo diverso»: il rifiuto per occupazione **si spegne**. L'ultimo tentativo governa. `EXPECTED BEHAVIOR` non copriva il caso; `DEV-MAIN-39180-02` § `RISKS` 1 lo ha segnalato come arbitrabile invece di lasciarlo scoprire da un rosso, ed è stato arbitrato così: la #79 non implementa gli altri reason code, ma non deve **attribuire al piano finale la causa di un tentativo precedente**. L'alternativa (set-only, il primo rifiuto vince) produrrebbe `BlockedByUnit` su un piano il cui ultimo esito era fuori mappa.

## SISTEMI IN SCOPE

Derivati dal write-set con §8, **senza verdetto**: DEV-LEAD non compare in nessuna colonna della matrice §7, e questo handoff porta la busta, non il payload.

In scope perché toccati:

`PLANNING` · `READY/COMMIT` · `MOVEMENT` · `COMBAT LOG` · `AUTOMATION/SCENARIO`

In scope per §8 punto 3, a valle del produttore modificato, anche senza file propri toccati:

`TURNLOG/REPLAY` · `DETERMINISM` · `UI/HUD` (le righe del combat log derivano dalle voci, `#1932`) · `SAVE/RELOAD` (le tracce già scritte devono restare leggibili) · `BUILD`

In scope come **verifica di non-degrado**, non come modifica:

`NETWORK AUTHORITY` · `PRIVACY`

`N/A — REASON: fuori write-set`:

`PROJECT` · `ARCHITECTURE` · `ASSETS` · `BLUEPRINT` · `DATA` · `DATA VALIDATORS` · `MAP` · `GRID/GRAPH` · `INPUT` · `CAMERA` · `SNAPSHOT` · `TARGETING` · `LOS/COVER` · `DAMAGE` · `STATUS/CONTROL` · `DISPLACEMENT` · `REACTIONS` · `ENVIRONMENT` · `OBJECTIVES` · `KO/CLEANUP` · `CERTAINTY` · `ERRORS` · `PERFORMANCE` · `PACKAGED`

## CONTRIBUTI CONSOLIDATI

| File | `CREATED` | `STATUS` | Esito |
|---|---|---|---|
| `contrib/DEV-MAIN-39180-01.md` | 15:18 | `BLOCKED / MISSING_INPUT` | **SUPERSEDED** da `-02` |
| `contrib/DEV-TEST-60316-01.md` | 15:18 | `BLOCKED / MISSING_INPUT` | **SUPERSEDED** da `-02` |
| `contrib/DEV-MAIN-39180-02.md` | 15:56 | `READY` | consolidato — 4 file |
| `contrib/DEV-TEST-60316-02.md` | 16:02 | `READY` | consolidato — 3 file |

Ordinati per `CREATED`. I `-01` restano su disco per la regola append-only e **non sono l'implementazione corrente**.

⚠️ Entrambi i `-02` dichiarano `BASE_SHA: 586ad594`, non `a59671c8`: hanno lavorato mentre quattro commit **documentali** si succedevano. Verificato che nessuno dei quattro tocca codice — `git diff --name-only 586ad594..55e31404 -- Source/ Scenarios/` restituisce solo i file di questo write-set, e i quattro commit intermedi toccano esclusivamente `docs/rt-three-terminals/waves/`. Il loro lavoro non è stato assorbito né invalidato.

🔑 **Uno dei quattro ha però cambiato il contratto sotto DEV-MAIN**, e lo dichiara: `bcde0be9` ha accolto il Finding `…/1-F3` e aggiunto il terzo produttore. DEV-MAIN ha riprogettato il consumo prima di chiudere invece di consegnare contro la revisione vecchia. È il motivo per cui il criterio 4 è soddisfatto.

## RISPOSTE agli `INTEGRATION REQUIRED`

### DEV-MAIN — `ScenarioHarness/RTScenarioSession.cpp`

**APPLIED BY LEAD** — `PRODUCED_SHA` `55e31404`, file `Source/RefactorTactics/ScenarioHarness/RTScenarioSession.cpp`.

Applicata la patch proposta, con la sostanza invariata: al ramo di rifiuto (`:1346-1370` sul sorgente prodotto) il Scenario Harness chiama **`ARTUnit::NoteMovePlanRejection`**, la stessa funzione del controller, e al ramo di piano valido chiama `ClearMovePlanRejection()` per il caso **C**. ⛔ **Nessuna voce TurnLog costruita dentro l'harness**, come l'arbitraggio 3 impone: una copia locale della regola renderebbe verde `CollisionChoke` lasciando muta la partita.

`Snapshot` e `UnitId` erano già in scope; `UnitId` è l'indice **nello snapshot** (`SnapshotUnits.IndexOfByKey`), che è ciò che la firma richiede — verificato contro `RTUnit.h:408`. La riga `UE_LOG(Warning, "[RT-Test] … percorso rifiutato …")` è **rimasta**, con un commento che dice perché: è diagnostica del runner e non è più l'unica traccia.

### DEV-TEST — richiesta 1 (`RTScenarioSession.cpp`)

**APPLIED BY LEAD** — stessa patch. `CollisionChoke` è l'unico caso diagnostico che passa di qui.

### DEV-TEST — richiesta 2 (terzo produttore, file di DEV-MAIN)

**APPLIED BY DEV-MAIN, verificato in consolidamento.** Il ramo `else if (HasPlannedNormalMove() && ClassifyWaypointCell(...) == Occupied)` è a `RTTurnManager.cpp:7219`. ✅ Nessun secondo predicato di «ha dichiarato»: `HasPlannedNormalMove()` resta la sede unica, come il work order vieta di duplicare. `Bot/RTHexBotLibrary.cpp` **non è stato toccato** ed è fuori write-set, con la ragione misurata: non contiene il ramo da correggere.

### DEV-TEST — richiesta 3 (`Turn/RTTurnLog.h`)

**REJECTED — non serve, e la ragione è misurata.** `ERTMoveOutcome::BlockedByUnit` esiste (`RTTurnLog.h:453`) e il formatter lo traduce già in «fermo: cella occupata». ⚠️ **Correzione di riferimento**: il work order citava `RTTurnLogLibrary.cpp:402` per quel testo; a `:402` c'è un **commento** che nomina `BlockedByUnit`, la traduzione vera è a **`:362`**. Il rilievo è di `DEV-MAIN-39180-02` § `FILES` ed è corretto. Nessun bump, nessun enumeratore nuovo: l'arbitraggio 4 è rispettato.

## Golden — decisione

⛔ **Nessuna modifica a `Tests/Golden/**`**, e nessuna proposta. La sequenza è: VALIDATION **misura** il corpus, e si rigenera **solo** se esiste una divergenza spiegata dal nuovo evento canonico.

Il reperto di `DEV-TEST-60316-02` § `GOLDEN` restringe l'area ed è utile a chi misura: `Scenarios/Movement/Collision.json` ha **un turno solo**, e le due unità contendono `(0,0,0)` quando è **libera** — il percorso è accettato, il fermo avviene nel resolver come `BlockedContested`. Il caso della #79 (rifiuto **in pianificazione**) non si presenta. ⚠️ È un'**ipotesi derivata dalla lettura dello scenario, non una misura**: la conferma è `RefactorTactics.Simulation.GoldenCorpusMatches`.

Se un corpus divergesse: `GoldenDivergenceNamesTheField` dice quale campo. Divergenza su `Outcome`/`TgtCell` di una voce `Move` = attesa, da motivare. Su qualunque altro campo = **scoperta**, e la rigenerazione si ferma.

## Comandi per VALIDATION

Letti da `contrib/DEV-TEST-60316-02.md` § `COMMANDS` — fonte canonica sul filesystem, non il testo di una chat.

🔴 **Prima di tutto, compilare. `rt-suite.ps1` non compila.**

```powershell
& "D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat" RefactorTacticsEditor Win64 Development `
    -project="D:\Repositories\refactor-tactics-main\RefactorTactics.uproject" -waitmutex
```

Dimostra che i sei test **esistono nel binario**; cercare `Result: Succeeded`. Senza questo passo la suite misura il binario precedente, i test nuovi non ci sono e `found = 0` si legge come «tutto verde».

Poi, in quest'ordine:

| # | Comando | Cosa dimostra |
|---:|---|---|
| 1 | `./scripts/rt-suite.ps1 -Filter RefactorTactics.PlayerInteraction` | casi **A**, **B**, **C** e il confine (test 1-4) |
| 2 | `./scripts/rt-suite.ps1 -Filter RefactorTactics.HexMove` | distinguibilità, ordering/determinismo, terzo produttore (test 5-6) **e** la non-regressione di `ContestedCellStopsBoth`, `Move.PathBlocked`, `Move.CellConflict` |
| 3 | `./scripts/rt-suite.ps1 -Filter RefactorTactics.Scenario` | `CollisionChoke` via `Scenario.EveryShippedScenarioRuns` — il percorso **harness** |
| 4 | `./scripts/rt-suite.ps1 -Filter RefactorTactics.Simulation.GoldenCorpus` | se e quali corpus `*.rttl` divergono |
| 5 | `./scripts/rt-suite.ps1` | la suite intera, per il verdetto di wave |

`-WaitMinutes <n>` se il motore è occupato da un'altra sessione.

⚠️ **L'ordine 3 → 4 conta.** Un `CollisionChoke` verde con un corpus divergente è la firma del difetto che il criterio 4 vieta: l'evento emesso dentro l'harness invece che nel punto di collasso comune.

### I sei test consegnati

| # | Test | Caso |
|---:|---|---|
| 1 | `RefactorTactics.PlayerInteraction.DeniedMoveDeclaresTheDenial` | **B** |
| 2 | `RefactorTactics.PlayerInteraction.UndeclaredMoveDoesNotDeclareADenial` | **A** |
| 3 | `RefactorTactics.PlayerInteraction.ReplanAfterADenialWins` | **C** |
| 4 | `RefactorTactics.PlayerInteraction.BudgetDenialIsNotAUnitDenial` | confine |
| 5 | `RefactorTactics.HexMove.DenialAndStillnessAreDistinguishable` | criterio 2 |
| 6 | `RefactorTactics.HexMove.DeclaredDestinationDeniedByOccupantDeclaresIt` | criterio 4, terzo produttore |

⚠️ Le **mutazioni** di anti-vacuità sono dichiarate in `DEV-TEST-60316-02` § *Anti-vacuità*, tabella di undici righe, e **non sono state eseguite**: eseguirle richiede build e suite. Il criterio 7 chiede che siano verificate sul file *prima* del fix — è una misura, e appartiene a chi misura.

## Finding aperti

| `FINDING_ID` | Severità | Owner | Stato |
|---|---|---|---|
| `…/1-F1` | `P2` | DEV-LEAD | **APERTO — processo.** I quattro ruoli furono avviati in parallelo all'ingresso. La sequenza corretta è ora vincolante nel work order; questo handoff è il primo passo che la rispetta |
| `…/1-F2` | `P3` | DEV-LEAD | **CHIUSO in consegna.** `CollisionChoke` porta `1` derivato dal contratto, con la nota riscritta e la storia della riga conservata. Da verificare a valle sulla misura reale |
| `…/1-F3` | `P2` | DEV-LEAD | **CHIUSO.** Terzo produttore riconosciuto, implementato e coperto dal test 6. `Bot/RTHexBotLibrary.cpp` fuori write-set con ragione misurata |
| `…/1-F4` · `…/1-F4R` | `P3` | DEV-LEAD | **SPOSTATO fuori wave** → follow-up **#2602**. Gli script rispondono se invocati per path; `rtstatus` è prescritto da `CLAUDE.md` §2 e non esiste |
| `…/1-F5` | `P3` | DEV-LEAD | **CHIUSO come regola**: la reason di blocco si misura, non si prescrive |
| `…/1-F6` | `P2` | DEV-LEAD | **CHIUSO da questo handoff.** Il fix è in `55e31404`; il gate d'ingresso è soddisfatto e resta adottato come precondizione di convocazione |

### ➕ Finding nuovo, aperto in consolidamento

```text
FINDING_ID:   issue-79-combat-log-blocked-move/1-F7
SEVERITY:     P3
EVIDENCE_REF: WORK-ORDER.md § Acceptance criteria, criterio 6
              git grep -c 'UPROPERTY(.*Replicated' a59671c8 -- Source/  -> 10 occorrenze, 5 file
ROOT_CAUSE:   Il criterio 6 prescrive un valore ASSOLUTO — «resta a 2 (le sole fixture di
              RTServerOnlyGuardTests)» — che non corrisponde alla baseline: su BASE_SHA sono
              10 occorrenze in 5 file, prima di qualunque riga di questa wave. Un criterio
              assoluto scritto senza misurare la baseline manda rosso un ruolo a valle su
              qualcosa che nessuno ha cambiato, e invita a modificare codice per soddisfare
              un numero.
OWNER:        DEV-LEAD
REQUIRED_FIX: il criterio si esprime come DELTA: «replication footprint della #79 = 0
              rispetto alla baseline misurata». Misurato su 55e31404: baseline 10, prodotto
              10, aggiunte 0. ⛔ Nessuna riga di codice è stata modificata per soddisfare il
              numero assoluto, ed è la decisione giusta.
REGRESSION:   delta = 0, non un valore assoluto. Il conteggio assoluto va misurato, mai
              scritto a memoria in un criterio.
ATTEMPT:      1
```

## `USER_REQUIRED`

Check a oracolo umano previsti. **Nessuno è un verdetto di questo handoff.**

| Check | Chi | `Result` |
|---|---|---|
| `PIE-V01-COLL` clausola **(c)** — oggi registrata `❌`, la voce resta 🟡 apposta; va **rimisurata** | EDITOR | `NOT RUN` |
| `PIE-V01-LOG` — la verifica PIE che tiene aperta la #79 | EDITOR | `NOT RUN` |
| Lettura in partita del turno 4 di `CollisionChoke`: «fermo: cella occupata» ≠ «resta» | EDITOR | `NOT RUN` |

## `NOT RUN`

| Elemento | Motivo |
|---|---|
| BUILD / compile | ruolo DEV: non occupa Unreal. §7 non assegna a DEV-LEAD nessuna colonna |
| Automation (i sei test) | come sopra |
| Scenario Harness / `CollisionChoke` | come sopra |
| Golden corpus | richiede l'esecuzione della suite |
| Verifica di mutazione | dichiarata da DEV-TEST, non eseguita: richiede build |
| PIE / packaged | dominio EDITOR e VALIDATION |

`RT3_CONTRACT.md` §6: nessuna di queste voci si legge `PASS`. §3: `file modificato != build/test/PIE/packaged verificato`.

---

STATUS:   READY
REASON:   n/a — l'ingresso per EDITOR è completo: `PRODUCED_SHA` esiste, contiene il fix e i
          test, il working tree è pulito e il contratto comportamentale è dichiarato.
UNBLOCK:  n/a
