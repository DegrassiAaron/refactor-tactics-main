# Work order — #79: il movimento negato in pianificazione non lascia traccia nel TurnLog

Ingresso della wave `issue-79-combat-log-blocked-move/1`. È il file che `RT3_CONTRACT.md` §4 richiede come `INPUT_HANDOFF`: un artefatto rileggibile, non testo incollato.

Emesso da DEV-LEAD. Non è un handoff §9: non porta verdetti, e non li porterà — DEV-LEAD non compare in nessuna colonna della matrice §7.

> **Consegna**: [PR #2631](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2631) → `main`, aperta in **draft** il 2026-09-06.
> ⛔ Draft e non pronta: il branch di wave ha ricevuto **tre commit da altre sessioni** — `b5badb79` (3 PNG), `6c6bfddc` e `f44196e7` (`tools/rt3/`) — che portano la PR da 856 a **10.488 righe**. Non li ho estratti perché riscrivere la storia cambierebbe gli SHA su cui EDITOR e VALIDATION hanno misurato, e ogni `EVIDENCE_REF` della wave smetterebbe di essere raggiungibile dalla PR.
> ⚠️ **La PR non chiude #79**: i tre check a oracolo umano restano `NOT RUN`.

## Busta

```text
FEATURE:         issue-79-combat-log-blocked-move
WAVE_ID:         issue-79-combat-log-blocked-move/1
BRANCH:          fix/79-blocked-move-turnlog
PARENT_BRANCH:   main
BASE_SHA:        a59671c87a7fe3407c1d3158521280267857d20b
INPUT_HANDOFF:   none (wave entry)
SEED_SOURCE:     none — nessun RNG sul percorso misurato
```

`BASE_SHA` è `origin/main` al 2026-09-06 15:02 UTC+2, ed è il commit da cui il branch è stato creato. **Non** è l'`HEAD` che le prime due istanze DEV hanno misurato: quello era `75ab6287`, sei commit indietro. I sei commit non toccano nessun file di questa wave — `git diff --stat 75ab6287 a59671c8 -- Source/RefactorTactics/{Turn,Player,Unit,ScenarioHarness} Scenarios/` è vuoto — quindi le misure di codice dei contributi restano valide; i loro `HEAD` no.

### Nota di preflight, dichiarata invece che nascosta

Il prompt di ingresso di questa wave portava `PARENT_BRANCH: <PARENT_BRANCH_REALE>` e `BASE_SHA: <BASE_SHA_REALE>`: due placeholder nella forma che §4 nomina, e su cui le istanze `DEV-MAIN:39180` e `DEV-TEST:60316` hanno correttamente chiuso `BLOCKED / MISSING_INPUT`. **Avevano ragione**, e la loro sospensione non è un errore da correggere.

I due valori qui sopra non sono l'inferenza che §4 vieta: sono il risultato della **misura** che lo stesso prompt ordinava (`git status --short`, `git rev-parse HEAD`, `git rev-parse --abbrev-ref HEAD`), eseguita da DEV-LEAD, che possiede l'arbitrato dell'ingresso. La differenza è di sede, non di stile: un DEV che deduce il proprio `BASE_SHA` inventa il proprio contratto; DEV-LEAD che misura il commit su cui apre la wave lo dichiara.

`BRANCH` non esisteva ed è stato creato da `BASE_SHA` — è la condizione 2 di entrambi gli `UNBLOCK` ricevuti.

## Origine

Issue [#79](https://github.com/DegrassiAaron/refactor-tactics-main/issues/79) — `CP 11.3 — Combat log con reason code completi`, `OPEN`, milestone `v0.1 · Leggibilità`, Epic #25, `P1`.

Il DoD di codice della issue è dichiarato completo da [#419](https://github.com/DegrassiAaron/refactor-tactics-main/pull/419) (`Priority` per voce, formato v7). Questa wave chiude il **residuo** aperto dai due commenti del 2026-09-04, che riguardano la seconda riga del DoD:

> *«I **fallback applicati** sono espliciti ("percorso bloccato → fermo", "bersaglio non valido → annullato")»*

Il primo dei due esempi che il DoD nomina è quello che manca, e manca nel modo meno visibile: non è un reason code scritto male, è un reason code che **non viene generato**.

## Il difetto, misurato

Caso diagnostico: `Scenarios/Movement/CollisionChoke.json`, turno 4. `A1` dichiara un movimento verso la cella che `B1` occupa **stando ferma**.

```text
Turno 3   Gadget: resta (q=-1,r=0,L=0) (Action.Move, p50)   ← non aveva dichiarato niente
Turno 4   Gadget: resta (q=-1,r=0,L=0) (Action.Move, p50)   ← ha tentato il varco, occupato
```

Le due righe sono **la stessa stringa**. Nel TurnLog e nel combat log, *«ho provato e me l'hanno negato»* e *«non ho provato»* sono indistinguibili — e la sola riga che li separa oggi (`[RT-Test] percorso rifiutato per 'A1'`) la emette `RTScenarioSession`, cioè il runner degli scenari: ⛔ **in una partita normale quella riga non esiste**.

Confermato headless (commento del 2026-09-04) e **in PIE**, seduta `U14` su `PIE-V01-COLL` (`PASS 6/6`, `stateHash 154f84b7`).

## Catena misurata

Ricostruita su `a59671c8`, read-only. Ogni riga è verificabile.

| # | Stadio | Sede | Cosa succede |
|---:|---|---|---|
| 1 | input | `Player/RTPlayerController.cpp:1268` | il click risolve la cella sul piano attivo e chiama `HandleClickOnCell` |
| 2 | planning | `Player/RTPlayerController.cpp:1497` | `HandleClickOnCell`, ramo movimento normale |
| 3 | prova | `Player/RTPlayerController.cpp:1624` | `PlannedWaypoints.Add(Cell)` — il waypoint si aggiunge **in prova** |
| 4 | validazione | `Player/RTPlayerController.cpp:1625` | `BuildCompositeHexPath` sullo snapshot autorevole |
| 5 | 🔴 rifiuto | `Player/RTPlayerController.cpp:1629` | `PlannedWaypoints.Pop()` + un `UE_LOG` diagnostico. **Nessuno stato sopravvive alla riga** |
| 6 | commit | `Turn/RTTurnManager.cpp:1922` → `:3181` | `ValidatePlansAtLockIn` giudica **slot e cooldown**, non i percorsi; scrive su `AddLogEvent`, e `RTTurnManager.cpp:3216` dichiara per iscritto che **da lì non nasce nessuna voce TurnLog** |
| 7 | snapshot | `Turn/RTTurnManager.cpp:7137` | `MakeCurrentSnapshot` dentro `ResolveMovement` |
| 8 | resolution | `Turn/RTTurnManager.cpp:7165-7172` | il path del turno viene da `PlannedPath` (se ancorato) o da `PlannedCell`; per l'unità rifiutata sono **entrambi vuoti/uguali alla cella** |
| 9 | 🔴 collasso | `Turn/RTTurnManager.cpp:7185` | `Path = { Unit->Cell }; // fermo` |
| 10 | esito | `Turn/RTHexSimLibrary.cpp:982` | `FinalizeHexMovementOutcomes`: `Paths[i].Num() <= 1` → `ERTMoveOutcome::Stayed` |
| 11 | TurnLog | `Turn/RTHexSimLibrary.cpp:1143` | `BuildMoveLog` — **una voce per unità**, `Action.Move`, `Priority` dal catalogo |
| 12 | formatter | `Turn/RTTurnLogLibrary.cpp:348`, `:400` | `Stayed` → «resta» |
| 13 | replay | — | la voce è nel TurnLog serializzato: «resta» è ciò che il replay porta, per entrambi i casi |

## I produttori sono TRE, e non si comportano allo stesso modo

Corretto il 2026-09-06 dopo il Finding `…/1-F3` di VALIDATION, che ne aveva contati tre contro i due di questa stessa sezione. **Aveva ragione sul numero**, e la misura di verifica mostra che i tre si dividono in **due famiglie**, non in tre casi.

| Produttore | Cosa dichiara | Dove il piano viene validato | Sopravvive la destinazione richiesta? |
|---|---|---|---|
| Player | waypoint → `PlannedWaypoints` + `PlannedPath` | **in pianificazione**, `Player/RTPlayerController.cpp:1625` | 🔴 **no** — `Pop()` a `:1629` la cancella |
| Scenario Harness | `move` dello scenario → gli stessi due campi | **in pianificazione**, `ScenarioHarness/RTScenarioSession.cpp:1339` | 🔴 **no** — il ramo `else` a `:1348` non scrive il piano |
| Bot | **solo** `PlannedCell` (`Turn/RTTurnManager.cpp:1240`, `:1342`, `:1565`, `:1586`) | ⚠️ **nella fase Move**, `Turn/RTTurnManager.cpp:7172` | ✅ **sì** — `PlannedCell` è ancora leggibile al collasso |

Il bot *«pianifica destinazioni, non percorsi a waypoint»* (`Turn/RTTurnManager.cpp:775`): non ha nessun ramo di rifiuto in pianificazione — nessun `Pop()`, nessun piano scartato — e la sua destinazione arriva intatta a `ResolveMovement`, dove `FindPathForUnit` fallisce e si cade sullo stesso `Path = { Unit->Cell }` di `:7185`.

⚠️ `URTHexBotLibrary::ReservePlannedRoute` (`Bot/RTHexBotLibrary.cpp:685-713`) **non è un quarto sito di validazione**: prenota celle nello snapshot *di squadra* per non far scegliere la stessa destinazione a due compagne (`#1088`), ed è chiamata durante il planning dei bot (`Turn/RTTurnManager.cpp:700`). Il suo ramo `Found.Path.Num() < 2` logga e prenota la destinazione; **non tocca il piano dell'unità**.

🔑 **La conseguenza semplifica il contratto invece di allargarlo.** Tutti e tre i produttori collassano nello **stesso punto** — `Turn/RTTurnManager.cpp:7183-7186` — e in quel punto la domanda «aveva dichiarato un movimento?» ha già una sede unica: `ARTUnit::HasPlannedNormalMove()` (`Unit/RTUnit.h:541`), che esiste proprio perché quella domanda non abbia due risposte diverse in due file.

`ERTMoveOutcome::Stayed` è dichiarato *«non pianificava movimento (path < 2 celle)»* (`Turn/RTTurnLog.h:450`). Su un'unità che ha dichiarato ed è stata negata, quella voce **dice il falso**, non solo «troppo poco».

## Il gap, in una riga

> Fra lo stadio 5 e lo stadio 9 non esiste alcun dato che porti «ho dichiarato una destinazione e mi è stata negata».

Misurato: nessun campo di rejection esiste. Una ricerca su `Reject*` / `Pending*` / `Requested*` in `Unit/`, `Turn/` e `Player/` non restituisce **nessun campo di stato** — solo commenti, il testo diagnostico `DescribeWaypointRejection` (`Player/RTPlayerController.h:405`) e gli esiti di **altri** domini (`ERTFacingOutcome::DeclarationRejected`, `CoverRejected`, `SurfaceRejected`).

⚠️ **Il gap non è uniforme fra i tre produttori, e questo cambia la dimensione del lavoro.** Per il **bot** non serve alcuno stato nuovo: `PlannedCell` è ancora lì quando il percorso fallisce, ed è già la destinazione richiesta. Lo stato serve al **player** e all'**harness**, e serve per portare la `TgtCell` che il `Pop()` ha cancellato — non per sapere *che* l'unità ha tentato, cosa che `HasPlannedNormalMove()` sa già dire.

## ⛔ Falsi punti di partenza

Elencati perché sono le strade che l'analisi imbocca da sola, e ciascuna è già stata scartata da una decisione viva.

1. ⛔ **Cambiare il resolver o la policy di collisione.** Il rifiuto in pianificazione è il comportamento **voluto e concordato**: `PIE-HEXPLAY-5` lo dichiara da agosto per lo scambio diretto `A↔B`, e `MOV-4` è chiusa da `D-325`. Manca la traccia, non la regola.
2. ⛔ **Scrivere una voce al lock-in.** `RTTurnManager.cpp:3216` la esclude con una motivazione ancora valida: *«una voce scritta al lock-in porterebbe una `Phase` che nessun consumatore del replay ha mai visto»*. La stessa riga indica dove la traccia va scritta: *«ciò che il turno scarta davvero lo dice `ResolveDash`, dove lo scarta»*.
3. ⛔ **Un secondo canale di log.** Il Player Event Log (#1936) è una **proiezione** e la nota di confine sulla issue lo dice: il contratto di CP 11.3 non si indebolisce e non si sposta lì.
4. ⛔ **Registrare ogni click invalido.** Il TurnLog è un formato serializzato, ordinato e riprodotto: ogni tentativo esplorativo diventerebbe un fatto del replay. Il soggetto è il **piano finale**, non la sequenza di click.
5. ⛔ **Correggere il solo `RTScenarioSession`.** È il runner degli scenari: una riga che vede solo lui è la fotografia esatta del difetto che la issue descrive.
6. ⛔ **Bump di formato TurnLog.** Non è dimostrato necessario — vedi `EXPECTED BEHAVIOR`. E la versione di formato è una risorsa contesa: #419 ha già dovuto rinunciare alla `v6` perché un altro ramo l'aveva presa.

## Meccanismi esistenti da riusare — `SEARCH → REUSE`

Il repository **non** possiede un rejection pending. Possiede però, già scritti, tutti i pezzi con cui costruirlo, e la regola su **dove** la traccia si scrive.

| Meccanismo | Sede | Cosa insegna |
|---|---|---|
| `ERTMoveOutcome::BlockedByTopology` | `Turn/RTTurnLog.h:461` | Il precedente esatto: *«lo scrive il chiamante e non `ResolveHexPaths`: il troncamento avviene PRIMA che il resolver veda il percorso»*. Identico al nostro caso, un solo stadio più a monte |
| il suo veicolo | `Turn/RTTurnManager.cpp:7147`, `:7209`, `:7320` | `TArray<bool> bStoppedByTopology` raccolto nel ciclo dei path e **riscritto sull'outcome** dopo `BuildMoveLog`. La forma da imitare |
| `ERTMoveOutcome::SupersededByDash` | `Turn/RTTurnManager.cpp:4719-4750` | Il precedente di contenuto: `SrcCell` = partenza, `TgtCell` = *«la destinazione dichiarata e mai raggiunta»*, `ActionId`/`Priority` letti dal **catalogo** (`FindCoreAction("Action.Move")`, mai cablati), `Amount` = celle del percorso scartato |
| `FRTPlannedMovement` | `Turn/RTHexSim.h:122` | Il canale già esistente per *«l'informazione che il resolver non può ricostruire»*, per-unità e dichiaratamente indipendente dall'ordine di iterazione |
| `bDeclaresPlannedFacing` + `PlannedFacing` | `Unit/RTUnit.h:360-365` | La forma flag+valore, e la sua ragione: un valore legittimo non può fare da «non dichiarato». Una cella di partenza è una destinazione legittima |
| `ERTFacingOutcome::DeclarationRejected` | `Turn/RTFacingLibrary.h:276` | Il precedente di **contratto**: una dichiarazione rifiutata è un esito osservabile *proprio perché non cambia nulla* |
| `ERTHexWaypointReason` | `Turn/RTHexSimLibrary.h:18` | Il vocabolario del motivo esiste già: `Occupied` · `BlocksMovement` · `NotOnMap` · (`Ok` = il rifiuto è di budget). Nessuna regola nuova da scrivere: `ClassifyWaypointCell` risponde |

## CONTRACT SOURCE

Il contratto di questa wave **non** si deriva dal codice corrente. Fonti, in ordine di autorità:

1. **DoD della issue #79**, seconda riga — *«i fallback applicati sono espliciti ("percorso bloccato → fermo"…)»*. È l'obbligo.
2. **I due commenti dell'owner del 2026-09-04** sulla issue #79 (misura headless e conferma PIE `U14`): fissano il caso, e fissano che la policy **non** cambia.
3. `Turn/RTTurnManager.cpp:3212-3218` — decisione viva su **dove** una traccia di scarto si scrive: nella fase in cui lo scarto avviene, mai al lock-in.
4. `Turn/RTTurnLog.h:456-465` — precedente normativo di un esito prodotto dal chiamante perché il resolver non può conoscerlo.
5. `docs/gameplay/spec-tassonomia-movimento.md` · `docs/technical/architecture/spec-turnlog.md` — owner specification dei due domini toccati.
6. `Scenarios/Movement/CollisionChoke.json`, nota dell'assertion `BlockedByUnit = 0` — dichiara in chiaro di essere **la misura di un buco, non un invariante**, e chiede di essere aggiornata da chi chiude #79.

⚠️ Per `DEV-TEST:60316`, che ha sollevato il punto: i quattro numeri di `CollisionChoke` **non** vanno presi dal prompt di chat. Tre di essi (`BlockedContested = 4`, `Moved = 1`, `TurnsCompleted = 4`) sono già nel file, versionati, con la loro derivazione scritta accanto; il quarto (`BlockedByUnit`) è il solo che questa wave cambia, e il suo valore nuovo si deriva dal contratto qui sotto — non da una run.

## EXPECTED BEHAVIOR

Contratto comportamentale della feature, forma di `WAVE_DEV_LEAD.md`. Un campo non applicabile porta `N/A` col motivo.

| Campo | Valore |
|---|---|
| **Given** | un'unità viva in fase `Planning`, con un piano di movimento il cui **percorso finale** è stato rifiutato dalla validazione di pianificazione perché la **destinazione richiesta** è occupata da un'altra unità |
| **When** | il turno viene committato e la fase `Move` si risolve senza che l'unità abbia un percorso percorribile |
| **Then** | il TurnLog porta **una** voce `Phase = Move`, `Category = Move`, `Outcome = ERTMoveOutcome::BlockedByUnit` per quell'unità, **al posto** della voce `Stayed` che oggi porta |
| **Authority** | server / `ARTTurnManager`. Il client propone il waypoint; la classificazione del rifiuto e la scrittura della voce sono autoritative. Nessun esito deciso dal client |
| **Timing boundary** | lo stato di rifiuto nasce in `Planning`, sopravvive al commit e **si consuma nella fase `Move`**. Non produce nulla al lock-in (`RTTurnManager.cpp:3216`) |
| **Target/recipient** | l'unità che ha dichiarato. La voce è sua: `SrcCell` è la sua cella di partenza — chiave stabile del turno — e non nomina l'occupante |
| **Failure** | rifiuto di pianificazione con motivo diverso da «destinazione occupata» (budget, cella non percorribile, fuori mappa): **fuori scope di questa wave**, comportamento invariato (`Stayed`). Il motivo si legge da `ERTHexWaypointReason`, non si inventa |
| **Fallback** | deterministico: assenza di stato di rifiuto ⇒ `Stayed`, esattamente come oggi. Il ramo nuovo è additivo sul dato, sostitutivo sull'esito |
| **Ordering** | invariato. La voce resta quella che `BuildMoveLog` già emette per quell'unità, nella stessa posizione dell'ordine d'input; nessuna voce aggiuntiva, nessuna iterazione di `TMap`/`TSet` |
| **TurnLog** | `UnitId` · `SrcCell` (partenza) · `TgtCell` (**destinazione richiesta e negata**, non la cella di partenza) · `ActionId = Action.Move` e `Priority` letti dal **catalogo** · `Outcome = BlockedByUnit` · `Amount` = `0` (nessuna cella percorsa) · `TurnNumber`, `GraphRevision`, verdetto di visibilità: invariati, li scrive `AppendLogEntry` |
| **Replay** | nessun campo nuovo, nessun enumeratore nuovo, **nessun bump di formato**. Cambia il **valore** di `Outcome` e di `TgtCell` su una voce che esisteva già. Le tracce v2–v7 già scritte non cambiano significato |
| **Privacy** | invariata, e da non degradare. Misurato: `Source/` non contiene **nessuna** `UPROPERTY(Replicated)` fuori dalle fixture di `RTServerOnlyGuardTests`. Lo stato nuovo **non** sarà `Replicated`; se il tipo tocca l'intento, va valutata la marcatura `RTServerOnly` che `Core/RTServerOnlyGuard.h` presidia |
| **SEED_SOURCE** | `none` — nessun RNG sul percorso misurato, dal click alla voce |
| **Editor-visible expectation** | in PIE, la riga del combat log del turno 4 di `PIE-V01-COLL` deve leggere «fermo: cella occupata» (testo già esistente, `RTTurnLogLibrary.cpp:402`) invece di «resta». È un'**attesa** per EDITOR, non un risultato: `Result: NOT RUN` |

### I tre casi, esplicitamente

| Caso | Situazione | Esito atteso |
|---|---|---|
| **A** | nessun movimento dichiarato | `Move / Stayed` — invariato |
| **B** | movimento dichiarato, destinazione finale occupata | `Move / BlockedByUnit`, con `TgtCell` = destinazione richiesta |
| **C** | tentativo rifiutato, poi **nuova pianificazione valida** committata | il piano valido prevale: nessun rejection stale, nessuna voce di blocco. L'esito è quello del movimento eseguito |

## Arbitrati DEV-LEAD

Le due domande che i contributi ricevuti hanno posto, decise qui perché sono di questo ruolo.

**1. `Stayed` e `BlockedByUnit` sono alternativi o coesistenti?** → **Alternativi.** La voce di blocco **sostituisce** `Stayed`, non si aggiunge. Tre ragioni, tutte verificabili:
- `BuildMoveLog` emette **una voce per unità** per la fase `Move` (`RTHexSimLibrary.cpp:1146-1160`): due voci `Move` per la stessa unità nella stessa fase non hanno precedente, e i lettori che contano per unità le leggerebbero come due movimenti;
- il precedente `BlockedByTopology` **riscrive** l'outcome dopo `BuildMoveLog` (`RTTurnManager.cpp:7320`): è già la forma in uso per un blocco che il resolver non può vedere;
- `Stayed` significa per **dichiarazione** «non pianificava movimento»: lasciarlo su chi ha pianificato è il difetto stesso, non un'informazione in più. Coesistere renderebbe `Stayed` ambiguo invece di disambiguarlo.

`SupersededByDash` è additiva e non contraddice: sta in `Phase = Dash`, cioè in un'altra fase.

**2. Serve un valore nuovo in `ERTMoveOutcome`?** → **No, per il caso B.** `BlockedByUnit` è dichiarato *«fermata (o parziale) per cella occupata da un'unità ferma»* (`RTTurnLog.h:453`) — che è letteralmente il caso — il formatter lo traduce già (`RTTurnLogLibrary.cpp:402`), e riusarlo evita il bump. Se durante l'implementazione emergesse una necessità **dimostrata** di un valore proprio (p.es. per distinguere «negato prima di partire» da «fermato per strada»), è una **richiesta bloccante verso DEV-LEAD**, non una decisione di scope: `Turn/RTTurnLog.h` è `INTEGRATION-OWNED`.

**3. Dove vive lo stato di rifiuto?** → Deciso il **contratto**, non l'implementazione: dev'essere un dato **per-unità**, non replicato, che nasce in `Planning`, muore quando un piano valido lo sostituisce (caso C) e quando il turno si chiude, e che i **due** siti di pianificazione — controller e harness — scrivono con la **stessa** funzione. La scelta fra un campo su `ARTUnit` (precedente: `bDeclaresPlannedFacing`) e un'estensione di `FRTPlannedMovement` (precedente: `bSlideRequested`) appartiene a DEV-MAIN, che la motiva nel proprio `PUBLIC CONTRACT`.

## DEV-MAIN — `ASSIGNED_SCOPE`

Owner del meccanismo: far sopravvivere il rifiuto fino alla fase `Move` ed emettere la voce.

```text
Source/RefactorTactics/Unit/RTUnit.h
Source/RefactorTactics/Unit/RTUnit.cpp
Source/RefactorTactics/Player/RTPlayerController.h
Source/RefactorTactics/Player/RTPlayerController.cpp
Source/RefactorTactics/Turn/RTTurnManager.h
Source/RefactorTactics/Turn/RTTurnManager.cpp
Source/RefactorTactics/Turn/RTHexSim.h
Source/RefactorTactics/Turn/RTHexSimLibrary.h
Source/RefactorTactics/Turn/RTHexSimLibrary.cpp
Source/RefactorTactics/Turn/RTTurnLogLibrary.cpp
```

Lavoro previsto:
1. lo stato di rifiuto per-unità (nascita, morte, non-replica), con il predicato che lo classifica **delegando** a `ClassifyWaypointCell` — nessun secondo vocabolario;
2. la scrittura in `HandleClickOnCell` al ramo di rifiuto (`RTPlayerController.cpp:1629`) e il suo azzeramento al ramo di successo (`:1644`) e all'annullamento waypoint (`:2029`, `:2272`) — è il caso **C**;
3. il consumo in `ResolveMovement` e la riscrittura dell'outcome dopo `BuildMoveLog`, sulla forma di `bStoppedByTopology` (`RTTurnManager.cpp:7147` · `:7209` · `:7320`);
4. `TgtCell` = destinazione richiesta, `Amount = 0`, `ActionId`/`Priority` dal catalogo — mai cablati;
5. `RTTurnLogLibrary.cpp` è in scope **solo** se il testo esistente non basta: nel percorso preferito è un no-op, e va detto;
6. ➕ **il caso del bot, che non costa un file in più** (Finding `…/1-F3`). Il bot dichiara `PlannedCell` e non viene mai rifiutato in pianificazione: al punto (3) la sua destinazione è ancora leggibile. Il predicato di «ha dichiarato» è `ARTUnit::HasPlannedNormalMove()` (`Unit/RTUnit.h:541`), che è già il punto unico di quella domanda — **non scriverne un secondo**. Lo stato nuovo serve al player e all'harness per portare la `TgtCell` che il `Pop()` cancella, non per rispondere a quella domanda.

### DEV-MAIN — `OUT_OF_SCOPE`

- ⛔ `Source/RefactorTactics/Turn/RTTurnLog.h` — `INTEGRATION-OWNED`. Un valore nuovo di enum si **chiede**;
- ⛔ `Source/RefactorTactics/ScenarioHarness/**` — `INTEGRATION-OWNED`, vedi sotto;
- ⛔ `Source/RefactorTactics/Tests/**` e `Scenarios/**` — sono DEV-TEST;
- ⛔ regola del resolver di movimento, `ResolveHexPaths`, microstep, priorità di collisione;
- ⛔ `ValidatePlansAtLockIn` e `URTPlanValidationLibrary`: giudicano slot e cooldown, e la loro scelta di non scrivere nel TurnLog resta;
- ⛔ Player Event Log / `ComposeVisibleLogLines` (#1936, #1937);
- ⛔ qualunque `UPROPERTY(Replicated)`.

## DEV-TEST — `ASSIGNED_SCOPE`

Owner degli oracoli. Deriva gli expected da `EXPECTED BEHAVIOR` qui sopra, **non** dal codice corrente.

```text
Source/RefactorTactics/Tests/RTPlayerInteractionTests.cpp
Source/RefactorTactics/Tests/RTHexMovementIntegrationTests.cpp
Scenarios/Movement/CollisionChoke.json
```

Lavoro previsto:
1. **caso A** — nessuna dichiarazione ⇒ `Stayed`. È il gemello che impedisce a un ramo incondizionato di passare per il caso B: senza, `BlockedByUnit` scritto sempre supererebbe il test di B (stessa disciplina di `LockInStaysSilentOnALegalPlan`, `RTPlayerInteractionTests.cpp:669`);
2. **caso B** — dichiarazione verso cella occupata da unità ferma ⇒ una voce `Move/BlockedByUnit` con `TgtCell` = destinazione richiesta e `SrcCell` = partenza. L'asserzione guarda **i campi**, non il conteggio: contare le voci non distingue una `TgtCell` giusta da una sbagliata;
3. **caso C** — tentativo rifiutato, poi replan valido ⇒ **nessuna** voce di blocco, e l'esito del movimento eseguito. È il test che discrimina «esito del piano finale» da «ogni click esplorativo», ed è quello che il vincolo 4 dei falsi punti di partenza esige;
4. `CollisionChoke.json` — aggiornare l'assertion `LogEventCount(Move/BlockedByUnit)` da `0` al valore che il contratto produce, **riscrivendo la nota**: il file dichiara in chiaro di misurare un buco, e la nota va sostituita con la ragione nuova. ⚠️ Il turno 4 è l'unico tentativo negato da unità ferma; i turni 1-2 sono contesa simultanea e restano `BlockedContested = 4`. Se la misura desse un numero diverso, **è una scoperta**, non un valore da riallineare;
5. **anti-vacuità**: per ciascun test, la mutazione che lo rende rosso, dichiarata. Il caso B senza mutazione non prova niente — `Stayed` e `BlockedByUnit` sono entrambi «l'unità non si è mossa»;
6. ➕ **almeno un caso sul percorso NON-harness** (regressione richiesta dal Finding `…/1-F3`). `CollisionChoke` da solo esercita il Scenario Harness: se restasse l'unica copertura, un evento emesso lì renderebbe il verde e lascerebbe la partita muta. Il caso B del punto 2 vive già in `RTPlayerInteractionTests.cpp` e soddisfa il requisito; un caso **bot** — destinazione dichiarata, poi occupata da un'unità ferma — è il complemento naturale in `RTHexMovementIntegrationTests.cpp`, e se risulta non costruibile va detto con la ragione invece di essere omesso.

### DEV-TEST — `OUT_OF_SCOPE`

- ⛔ ogni file di `Source/RefactorTactics/{Unit,Player,Turn}/**` — sono DEV-MAIN;
- ⛔ `Source/RefactorTactics/ScenarioHarness/**` — `INTEGRATION-OWNED`;
- ⛔ `Source/RefactorTactics/Tests/Golden/**` — la rigenerazione del corpus è integrazione, e solo se misurata necessaria;
- ⛔ modificare un expected per far passare il codice, in **entrambe** le direzioni: né allentare un rosso, né fissare un expected che il contratto non dichiara;
- ⛔ eseguire i test: `rt-suite`, Automation e Scenario Harness sono `VALIDATION`. Il ruolo DEV non occupa Unreal.

## `INTEGRATION-OWNED FILES`

Restano a DEV-LEAD. Nessuna istanza DEV li scrive; toccarli è una richiesta.

```text
Source/RefactorTactics/ScenarioHarness/RTScenarioSession.cpp
Source/RefactorTactics/Turn/RTTurnLog.h
Source/RefactorTactics/Tests/Golden/**
docs/rt-three-terminals/waves/issue-79-combat-log-blocked-move/**
```

Perché ciascuno:

- **`RTScenarioSession.cpp`** è il punto in cui i due scope si toccherebbero. È il **secondo produttore** dello stesso stato — il rifiuto del piano lì è codice di produzione (`:1339-1350`), non codice di test — quindi la sua correzione appartiene al meccanismo di DEV-MAIN; ma è anche il file che DEV-TEST esercita per l'unico scenario diagnostico della wave. Assegnarlo a uno dei due significherebbe che l'altro lavora su una base che cambia sotto di lui. ⚠️ **Il lavoro qui è serializzato**: DEV-LEAD lo applica dopo il contributo di DEV-MAIN, riusando la funzione che DEV-MAIN espone. Correggere il solo controller lascerebbe l'harness muto, ed è metà del difetto.
- **`RTTurnLog.h`** ospita `ERTMoveOutcome`, cioè il formato. Una modifica lì è una versione, e la versione è una risorsa contesa fra rami: #419 ha già dovuto cedere la `v6`.
- **`Tests/Golden/**`** contiene otto corpora con checksum (`*.rttl`, `binary` in `.gitattributes`). `Movement.Collision` è fra questi. Se l'esito di una voce cambia, il digest cambia: è una rigenerazione, e va **misurata** prima di essere eseguita.

## Acceptance criteria

1. In una partita normale — **senza** `RTScenarioSession` nel percorso — un movimento dichiarato verso una destinazione occupata da un'unità ferma produce una voce `Move/BlockedByUnit` nel TurnLog, con `TgtCell` = destinazione richiesta.
2. Un'unità che non ha dichiarato nulla continua a produrre `Move/Stayed`. I due casi sono distinguibili **leggendo la sola voce**, senza log di runner.
3. Un tentativo rifiutato seguito da una pianificazione valida non lascia rejection: prevale il piano finale.
4. **Tutti e tre i produttori** — player, Scenario Harness, bot — producono lo **stesso** esito per lo stesso stato. Una sola regola, tre chiamanti, un solo punto di decisione (`Turn/RTTurnManager.cpp:7183`). ⚠️ Un `CollisionChoke` verde ottenuto emettendo l'evento dentro `RTScenarioSession` **non** soddisfa questo criterio: sarebbe il difetto di oggi spostato di un livello.
5. Nessun campo nuovo, nessun enumeratore nuovo, **nessun bump di versione del TurnLog**; oppure la necessità è dimostrata e arbitrata da DEV-LEAD prima di essere implementata.
6. Nessuna `UPROPERTY(Replicated)` aggiunta: `git grep -c 'UPROPERTY(.*Replicated' Source/` resta a `2` (le sole fixture di `RTServerOnlyGuardTests`).
7. Anti-vacuità: per ogni test nuovo, la mutazione che lo rende rosso è dichiarata **e** verificata sul file prima del fix.
8. `CollisionChoke.json` porta il valore nuovo **con la nota riscritta**: il rosso atteso non deve sembrare una regressione a chi lo incontra.

## Downstream in scope — `RT3_CONTRACT.md` §8

Il write-set tocca il produttore delle voci `Move`. Per §8 punto 3 entrano in scope come regressione, anche senza file modificati:

`MOVEMENT` · `PLANNING` · `READY/COMMIT` · `COMBAT LOG` · `TURNLOG/REPLAY` · `DETERMINISM` · `AUTOMATION/SCENARIO` · `UI/HUD` (il combat log deriva dalle voci, #1932) · `SAVE/RELOAD` (le tracce già scritte devono restare leggibili).

`PRIVACY` e `NETWORK AUTHORITY` entrano come **verifica di non-degrado**, non come modifica.

Fuori write-set, `N/A` con motivo: `ASSETS` · `BLUEPRINT` · `MAP` · `CAMERA` · `TARGETING` · `LOS/COVER` · `DAMAGE` · `REACTIONS` · `ENVIRONMENT` · `OBJECTIVES` · `PACKAGED`.

## `USER_REQUIRED` — check a oracolo umano previsti

| Check | Result |
|---|---|
| `PIE-V01-LOG` — la verifica PIE che tiene aperta la issue #79 | `NOT RUN` |
| `PIE-V01-COLL` clausola **(c)** — registrata `❌`, la voce resta 🟡 apposta; va **rimisurata** quando questa wave chiude | `NOT RUN` |
| lettura **in partita** del turno 4 — il widget, non l'Output Log | `NOT RUN` |

📘 **La seduta ha una guida**: [`guida-seduta-chiusura-79-denial-e-log.md`](../../../technical/runbooks/guida-seduta-chiusura-79-denial-e-log.md). Non riscrive l'allestimento — quello sta in [`guida-seduta-u14…`](../../../technical/runbooks/guida-seduta-u14-collisione-stallo-e-denial.md) e resta valido — dice cosa è cambiato sotto e convoca una **rimisura**.

Tre fatti che la guida mette in cima, perché ciascuno può far misurare la cosa sbagliata:

1. 🔴 **Ricompilare prima.** Misurato: la `DLL` era delle `09:48`, i sorgenti del fix delle `15:47`. Aprire PIE senza build misura il codice **precedente** al fix — cioè riproduce il difetto, leggibile come «il fix non funziona». Il fix vive da `3fff65a6`, e su `main` non c'è ancora.
2. **Due voci, un solo Play.** La clausola (c) e la lettura del turno 4 sono la stessa osservazione: `Movement.CollisionChoke`, guardare i turni **3** e **4**, che sono controllo e caso. Erano la stessa stringa; devono essere diverse.
3. ⛔ **`PIE-V01-LOG` ha un banco diverso**, ed è dichiarato: su `L_HexArena` dodici turni non hanno prodotto un solo fallback — *«è un fatto sulla mappa, non sul codice»*. Il suo banco è `Visual.Environment.Acceptance`. Usare `CollisionChoke` per chiuderla darebbe un `✅` su un campione di uno.

⚠️ La guida porta anche ciò che ho misurato sull'affordance: un click sulla **mesh** di un'unità va a `OnClickUnit` — carica, attacco o niente — e non produce un waypoint. Solo un click sul **terreno** della cella occupata arriva a `HandleClickOnCell`. Un «non succede niente» cliccando il nemico **non è** il difetto di #79: è la porta d'ingresso che non passa di lì. Va registrato come osservazione separata, mai come esito di (c) — che si giudica sul log.

## Rischi dichiarati

1. **Il corpus golden.** `Movement.Collision` è nel corpus. Se una sua unità dichiara un movimento poi negato **in pianificazione**, il digest cambia. Va misurato prima di rigenerare: una rigenerazione non misurata trasforma una scoperta in un allineamento.
2. **Il caso B non è l'unico rifiuto.** Budget, cella non percorribile e fuori mappa restano `Stayed`. È una scelta di scope, non una svista: dichiararla qui evita che il prossimo lettore la scambi per un bug.
3. **Due siti, una regola.** Controller e harness devono chiamare la **stessa** funzione. Due copie divergerebbero alla prima modifica, e la divergenza sarebbe visibile solo in PIE.
4. **`Amount = 0` è un valore, non un'assenza.** `SupersededByDash` insegna che questo campo è già stato causa di un'asserzione sbagliata: va scritto con la sua ragione accanto.
5. **Dipendenza #77 dichiarata dalla issue.** Non verificata come chiusa in questa istruttoria: il residuo di #79 che questa wave chiude non la nomina, ma chi chiude la issue la deve rileggere.

## Risposte ai contributi ricevuti

`WAVE_DEV_LEAD.md` § *Obbligo di risposta*: un contributo `BLOCKED` è una richiesta bloccante, non una notifica.

| Contributo | `CREATED` | `STATUS` | Risposta |
|---|---|---|---|
| `contrib/DEV-MAIN-39180-01.md` | 2026-09-06 15:18 | `BLOCKED / MISSING_INPUT` | **APPLIED BY LEAD** |
| `contrib/DEV-TEST-60316-01.md` | 2026-09-06 15:18 | `BLOCKED / MISSING_INPUT` | **APPLIED BY LEAD** |

**APPLIED BY LEAD** — l'ingresso mancante è stato prodotto, ed è questo file:

- path: `docs/rt-three-terminals/waves/issue-79-combat-log-blocked-move/WORK-ORDER.md`;
- branch `fix/79-blocked-move-turnlog`, creato da `a59671c8` — condizione 2 di entrambi gli `UNBLOCK`;
- `BASE_SHA` dichiarato, `ASSIGNED_SCOPE` e `OUT_OF_SCOPE` come path espliciti e disgiunti — condizioni 1 e 3;
- `CONTRACT SOURCE` ed `EXPECTED BEHAVIOR` sopra — condizione 3 di DEV-TEST, e i due arbitrati che `DEV-MAIN-39180-01` chiedeva per nome (`Stayed` vs `BlockedByUnit`, e l'owner runtime del punto di estensione).

Entrambe le istanze ripartono dallo step 1 con un contributo nuovo — `DEV-MAIN-39180-02.md` e `DEV-TEST-60316-02.md`, ciascuno con `SUPERSEDES:` sul proprio `-01` — come impone la regola append-only.

⚠️ La directory `contrib/` che i due contributi hanno aperto **non era orfana**: il feature-slug che avevano usato è quello di questa wave. Nessun file va rimosso.

## Risposte ai Finding di VALIDATION e di EDITOR

Alle 15:33 `VALIDATION` ha consegnato `RT3-VALIDATION-a59671c.md`, alle 15:49 `EDITOR` ha consegnato `RT3-EDITOR-a59671c.md`. Entrambi `STATUS: BLOCKED`, sette Finding in tutto, tutti con `OWNER: DEV-LEAD`. Entrambe le sessioni erano state avviate **prima** che esistesse un handoff di consegna da consumare. La sessione era stata avviata **prima** che esistesse un handoff di consegna da validare, e la sua `MATRICE` è `NOT RUN` su tutti i sistemi in scope: nessun sistema della matrice canonica è stato verificato, e §6 lo dice — `NOT RUN` non conta come `PASS`.

Il `BLOCKED` di VALIDATION è **corretto e non si contesta**: `RT3-DEVLEAD-<sha7>.md` è l'handoff di **uscita**, nasce a implementazione fatta, e oggi non esiste. Questo work order è l'**ingresso** e non lo sostituisce — lo dice la propria riga di apertura.

| Finding | Severità | Risposta |
|---|---|---|
| `…/1-F1` — i quattro ruoli avviati in parallelo all'ingresso | `P2` | **ACCETTATO** |
| `…/1-F2` — l'oracolo `BlockedByUnit = 1` è un obiettivo, non un contratto | `P3` | **ACCETTATO — già normato** |
| `…/1-F3` — i produttori sono tre, il terzo non è in nessuno scope | `P2` | **ACCETTATO — APPLIED BY LEAD** |
| `…/1-F4` — i wrapper `rt*` non sono risolvibili | `P3` | **ACCETTATO in parte — corretto da `F4R`** |
| `…/1-F4R` — alias non caricati (falso sintomo) vs `rtstatus` non versionato (reale) | `P3` | **ACCETTATO** |
| `…/1-F5` — la reason di blocco va misurata, non prescritta | `P3` | **ACCETTATO** |
| `…/1-F6` — il fix vive solo nel working tree, nessun commit lo contiene | `P2` | **ACCETTATO — al consolidamento** |

**F1 — ACCETTATO.** La causa è di processo e appartiene a questo ruolo: l'ingresso va emesso **prima** dei DEV, e i due handoff di consegna **prima** di VALIDATION. Non c'è fix di codice e non c'è test di regressione: il controllo è il preflight §4, che ha funzionato — tre istanze su tre si sono fermate senza inventare input. La sequenza per il resto di questa wave è: contributi `-02` dei due DEV → consolidamento e `RT3-DEVLEAD-<sha7>` → EDITOR → VALIDATION.

**F2 — ACCETTATO, già normato a monte.** Il criterio 8 e la sezione `CONTRACT SOURCE` prescrivono già che il numero si derivi dal contratto e che una divergenza sia una scoperta. Resta come voce di verifica **a valle**, sul file finale: è la lettura giusta, e non richiede modifiche qui.

**F3 — ACCETTATO, ed è il finding che ha cambiato il documento.** Il numero era sbagliato: i produttori sono tre. La verifica indipendente ha aggiunto il pezzo che il Finding non conteneva — le tre famiglie **non** sono simmetriche:

- il bot non viene mai rifiutato in pianificazione, perché dichiara `PlannedCell` e non un percorso (`Turn/RTTurnManager.cpp:775`, `:1240`, `:1342`, `:1565`, `:1586`); il suo percorso si calcola **nella fase Move** (`:7172`) e fallisce **dentro** `ResolveMovement`;
- quindi per il bot la destinazione richiesta **è ancora leggibile** al collasso, e non serve nessuno stato che sopravviva;
- `ReservePlannedRoute` non è un sito di validazione del piano: prenota celle nello snapshot di squadra (`Bot/RTHexBotLibrary.cpp:685-713`, chiamata da `Turn/RTTurnManager.cpp:700`) e non tocca il piano dell'unità.

Risposta alla domanda posta per iscritto — *«se un bot dichiara una destinazione che a risoluzione risulta occupata da un'unità ferma, quale voce produce?»*: **oggi `Stayed`, e con questa wave `BlockedByUnit`, come per gli altri due.** Non è un'estensione di scope: i tre collassano nello stesso punto (`Turn/RTTurnManager.cpp:7183`), che è già assegnato a DEV-MAIN. `Bot/RTHexBotLibrary.cpp` resta **fuori scope, con la ragione misurata**: non contiene il ramo da correggere.

Applicato: nuova sezione *I produttori sono TRE*, criterio 4 riscritto (tre chiamanti, un punto di decisione, e il divieto esplicito del verde ottenuto dentro l'harness), DEV-MAIN punto 6, DEV-TEST punto 6 con la regressione sul percorso non-harness che il Finding chiedeva.

**F4 — ACCETTATO sul sintomo, e il Finding `…/1-F4R` di EDITOR ne corregge la portata.** Verificato in modo indipendente **in `pwsh`**: `rtstatus`, `rtws`, `rtlease`, `rtsuite`, `rtmcp` non sono risolvibili, e `$PROFILE.CurrentUserAllHosts` — `C:\Users\Utente\OneDrive\Documents\PowerShell\profile.ps1` — **non esiste**.

🔴 **Ma la conclusione che ne avevo tratto era sbagliata, e la correggo qui.** Avevo scritto che senza quei wrapper `EDITOR` non può fare il proprio preflight né prendere il lease. **È falso**, e `RT3-EDITOR-a59671c.md` lo misura: invocati **per path**, `scripts/rt-workspace.ps1 -Action verify` risponde `OK: workspace MAIN` e `scripts/rt-lease.ps1 -Action status` risponde `ENGINE LEASE: LIBERO`. L'ambiente EDITOR è pronto; ciò che manca è l'handoff a monte.

I due casi vanno separati, come `F4R` chiede:

- **falso sintomo** — gli alias di profilo non sono caricati in una shell non interattiva. Gli script ci sono e rispondono. ⚠️ Nessuna misura RT3 deve dedurre l'assenza di tooling da un alias non risolto: è la stessa forma di errore che questo work order rimprovera altrove — un'assenza letta al posto di una misura;
- **difetto reale** — `rtstatus` è prescritto da `CLAUDE.md` §2 (*«esegui `rtstatus` quando disponibile»*) e **nessuno script con quel nome è versionato**: `git ls-files scripts/` non ne contiene. O lo script si crea, o la prescrizione si toglie. È tooling RT3, non `#79`: **fuori da questa wave**, e va deciso da chi possiede `CLAUDE.md`.

**F5 (EDITOR) — ACCETTATO.** Il mandato di EDITOR prescriveva `REASON: Unreal/Editor capability unavailable` come esito di blocco. EDITOR ha rifiutato di scriverla perché **misurata falsa**, e ha ragione: avrebbe mandato questo ruolo a riparare una macchina che funziona. La regola che ne discende vale per tutti i mandati di questa wave: **la reason si misura, non si prescrive**.

**F6 (EDITOR) — ACCETTATO, e la sua esecuzione è il prossimo passo di questo ruolo, non di adesso.** `git diff a59671c8..HEAD -- Source/` è vuoto mentre `git status` mostra il fix: il codice della #79 vive **solo nel working tree condiviso**, e nessun commit lo contiene. È esatto. Ma al momento di questa risposta il working tree è **in crescita**: committare ora catturerebbe due contributi a metà, e produrrebbe uno SHA che non descrive ciò che dichiara. Il commit e il `PRODUCED_SHA` arrivano al **consolidamento**, con i contributi `-02` dei due DEV, ed entrano in `RT3-DEVLEAD-<sha7>.md`. ✅ Il gate d'ingresso che `F6` propone — *«`git diff BASE..HEAD -- Source/` non vuoto quando la wave dichiara un fix di codice»* — è adottato come **precondizione di convocazione** di EDITOR e VALIDATION per questa wave.

⚠️ Nessuna di queste risposte trasforma un `NOT RUN` in un `PASS`. I `BLOCKED` di VALIDATION e di EDITOR restano validi fino a quando la catena non produce l'handoff di consegna che a entrambi manca.

### Stato reale della catena, misurato alle 15:49

| Ruolo | Stato | Evidenza |
|---|---|---|
| DEV-MAIN | 🔄 **in scrittura** | `RTUnit.{h,cpp}`, `RTPlayerController.cpp`, `RTTurnManager.cpp` modificati; `NoteMovePlanRejection` presente in `Source/` |
| DEV-TEST | 🔄 **in scrittura** | `RTPlayerInteractionTests.cpp`, `RTHexMovementIntegrationTests.cpp` modificati; `CollisionChoke.json` porta `BlockedByUnit = 1` con la nota riscritta |
| EDITOR | ⛔ `BLOCKED` | manca `RT3-DEVLEAD-<sha7>.md`; ambiente pronto (workspace `MAIN`, lease `LIBERO`) |
| VALIDATION | ⛔ `BLOCKED` | manca l'handoff di EDITOR; `MATRICE` tutta `NOT RUN` |

⛔ **Nessuno committa `Source/` o `Scenarios/` se non DEV-LEAD al consolidamento.** Un `git add -A` in questo working tree assorbe il lavoro non finito di due istanze e i tre handoff untracked — è il caso che `RT3-EDITOR-a59671c.md` segnala nel proprio corollario.

## Sequenza di consolidamento — vincolante

Confermata dall'owner della wave dopo la run EDITOR anticipata. Non è una raccomandazione: è la condizione perché un ruolo a valle abbia qualcosa da misurare.

```text
1. contributi -02 di DEV-MAIN e DEV-TEST        ← stato al 2026-09-06 16:0x: NON PERVENUTI
2. integrazione SERIALE dei file INTEGRATION-OWNED
3. working tree pulito, verificato
4. commit di produzione + test + documentazione
5. RT3-DEVLEAD-<sha7>.md riferito al PRODUCED_SHA REALE
6. solo allora: NUOVA run EDITOR
```

⛔ **Nessuna via libera a EDITOR prima del punto 5.** La run del 2026-09-06 15:49 è chiusa e non si riapre: il suo `BASE_SHA` non conterrà mai il fix. Una seconda convocazione è una **run nuova**, su un `PRODUCED_SHA` che oggi non esiste.

### Il gate d'ingresso, adottato da `1-F6`

Prima di convocare EDITOR o VALIDATION, questo comando deve produrre output **non vuoto**:

```powershell
git diff --name-only <BASE_SHA>..<PRODUCED_SHA> -- Source/ Scenarios/
```

Misurato ora, su `HEAD = bcde0be9`: **vuoto**. Il fix vive solo nel working tree condiviso — i simboli `bMovePlanRejectedByOccupant`, `RejectedMoveDestination` e `NoteMovePlanRejection` sono in `Source/`, in `RTUnit.h`, `RTUnit.cpp` e `RTPlayerController.cpp`, e **nessuno SHA li contiene**.

🔑 **Il difetto che il gate previene non è «manca un commit», è che il target è mobile.** EDITOR ha misurato file che comparivano *durante* la misura: qualunque verdetto avrebbe descritto uno stato che non esiste più un minuto dopo. `CLAUDE.md` §6 lo chiama per nome — se HEAD, working tree o binari cambiano durante una finestra di misura, la misura è `NON VALIDA`, non `FAIL`.

⚠️ **`NON VALIDA` non è `FAIL`, e la distinzione va conservata nel referto.** Un `FAIL` manderebbe DEV a cercare un difetto nel fix; qui non c'è nessun difetto misurato — c'è una misura che non poteva essere presa.

## Decisione di defect policy — il wrapper `rtstatus`

Reperto di tooling emerso da `1-F4R`, misurato in modo indipendente e **preservato qui perché questo file è versionato**, mentre le shell delle sessioni non lo sono.

| Misura | Comando | Esito |
|---|---|---|
| script versionato | `git ls-files \| grep -iE 'rtstatus\|rt-status'` | **nessuno** |
| script `rt-*` esistenti | `git ls-files scripts/` | 8: `rt-lease` · `rt-mcp-guard` · `rt-mcp-server` · `rt-mode` · `rt-suite` · `rt-suite-safe` · `rt-terminal` · `rt-workspace` |
| sedi che lo prescrivono | `grep -n rtstatus` | `CLAUDE.md:55` · `CLAUDE_INSTALL_AND_INTEGRATE.md:156` · `TERMINAL_EDITOR.md:60` · `TERMINAL_VALIDATION.md:13` |
| script invocabili per path | `scripts/rt-workspace.ps1 -Action verify` · `scripts/rt-lease.ps1 -Action status` | `OK: workspace MAIN` · `ENGINE LEASE: LIBERO` |

⚠️ **`RTStatusTests.cpp` è un omonimo, e conta dirlo.** È l'unico file che una ricerca per `rtstatus` restituisce, ma i suoi test sono `RefactorTactics.Status.Burning.*`, `Status.WetRemovesBurning`, `Status.PersistsWhileOnCell`: sono gli **status effect di gameplay**, non il wrapper. Letto come copertura del wrapper darebbe per verificata una cosa che nessuno verifica — e `rtstatus` oggi non ha né script né test.

**Decisione: FUORI da questa wave, follow-up tooling.** Severità `P3`, confermata.

Perché fuori:

- il write-set di questa wave è il combat log del movimento negato. `rtstatus` tocca `CLAUDE.md` §2 e **due prompt di ruolo**: è il contratto operativo RT3, non `#79`. Sono due contratti diversi, e mescolarli renderebbe il `WRITE_SET` di `RT3-DEVLEAD-<sha7>` illeggibile per lo scoping §8 a valle;
- ⛔ **non blocca**: gli script esistono e rispondono se invocati per path. È una prescrizione falsa, non una capability assente — la distinzione è esattamente ciò che `1-F4R` ha corretto in questo file;
- la wave è già in esecuzione con quattro ruoli. Allargarne lo scope adesso è la definizione di scope creep, e §12 non lo richiede: un `P3` di processo non impone una correzione dentro il ciclo in corso.

Le due uscite del follow-up sono già poste, e sono alternative: **creare `scripts/rt-status.ps1`**, oppure **togliere la prescrizione dalle quattro sedi**. Non si decide qui: appartiene a chi possiede `CLAUDE.md`.

➡️ **Follow-up aperto: [#2602](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2602)** — `documentation` · `P3`. Porta le quattro misure, le due uscite, l'avvertenza sull'omonimia e una DoD che chiede l'applicazione **in tutte e quattro le sedi**: una prescrizione tolta a metà è peggio di una intera. Nessun'altra issue apre lo stesso reperto: verificato prima di crearla.

## Risposte ai Finding della run EDITOR su `8dcd3a76`

La seconda run EDITOR — quella convocata **dopo** il consolidamento, come la sequenza impone — ha chiuso `PARTIAL`: nessun sistema `FAIL`, tre check umani `NOT RUN`. Handoff: `RT3-EDITOR-8dcd3a76.md`.

🔑 **Il fatto che chiude il difetto originale**, misurato headless sul binario ricompilato:

```text
T3   Gadget: resta (q=-1,r=0,L=0) (Action.Move, p50)                     <- non aveva dichiarato
T4   Gadget: fermo: cella occupata (q=-1,r=0,L=0) (Action.Move, p50)     <- ha tentato, negato
```

Prima del fix erano **la stessa identica stringa**. `Movement.CollisionChoke`: `PASS` 6/6, con `BlockedByUnit = 1` dove il 2026-09-04 dava `0`.

⚠️ EDITOR ha tenuto `COMBAT LOG` a `OBSERVED` invece del `PASS` che il tetto §7 gli permetterebbe, «perché ho visto il formatter in un log headless, non il widget in partita». È la lettura giusta, e vale la pena dirlo: un ruolo che si tiene **sotto** il proprio tetto quando l'evidenza non arriva fin lì è ciò che rende leggibile il resto della matrice.

| Finding | Severità | Risposta |
|---|---|---|
| `…/1-F8` — `HEAD ≠ BASE_SHA` per un commit estraneo | `P3` | **RATIFICATO — la lettura di EDITOR è quella corretta** |
| `…/1-F9` — la riga non mostra la destinazione negata | `P3` | **ACCOLTO — fuori wave, follow-up [#2627](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2627)** |
| `…/1-F10` — la prima run si è invalidata da sola | `P3` | **REGISTRATO — errore auto-diagnosticato, nessuna azione** |

**F8 — RATIFICATO.** `HEAD` era `b5badb79` («maps») contro un `BASE_SHA` di `8dcd3a76`. EDITOR ha applicato la lettura *«nessun delta sui path che la misura tocca»* **dichiarandola**, e ha chiesto se §5 vada invece letto come uguaglianza stretta. Verificato in proprio: `git diff --name-only 8dcd3a76..b5badb79` restituisce **tre PNG in `docs/research/maps/`**, zero file di `Source/`, `Scenarios/` o `Content/`.

La lettura è ratificata per questa wave, e la ragione è strutturale: il working tree è **condiviso**, e in questa sola wave `HEAD` si è mosso sette volte per commit documentali. Un'uguaglianza stretta renderebbe impossibile qualunque misura ogni volta che un'altra sessione committa un file di documentazione — cioè trasformerebbe §5 da guardia della validità in un divieto di misurare.

⚠️ **Ciò che §5 protegge è la stabilità del soggetto misurato, non l'immobilità del repository.** `CLAUDE.md` §6 lo dice per esteso: se `HEAD`, working tree, binari o processi cambiano **durante** una finestra di misura, quella misura è `NON VALIDA`. Il criterio è il **delta sui path osservati**, e va dichiarato ogni volta invece di essere assunto.

➡️ La sede per renderlo normativo è `RT3_CONTRACT.md` §5, non questo file: **non l'ho modificato** — è fuori dal write-set della wave, ed è un emendamento di contratto che appartiene al suo owner. Registrato qui perché la prossima wave non debba riscoprirlo.

**F9 — ACCOLTO, e fuori da questa wave.** La riga mostra `SrcCell`, dove l'unità si trova; la destinazione negata è nel TurnLog ma non nel testo. L'argomento di EDITOR è esatto e sta nel codice: il commento che mette `SupersededByDash` nel ramo lungo dice *«un rendering che stampa solo `SrcCell` la nasconde»*, e dopo questa wave vale identico per il diniego.

⛔ **Misurato prima di decidere, e la misura ha cambiato la risposta: `BlockedByUnit` ha ora DUE semantiche di `TgtCell`.**

| Produttore | `TgtCell` | Il ramo lungo stamperebbe |
|---|---|---|
| diniego in pianificazione (#79) | destinazione richiesta e negata | ✅ `(-1,0,0) -> (0,0,0)` — informativo |
| resolver, blocco durante il percorso | `Results[i].Final`, dove si è fermata | ⚠️ se bloccata al primo passo, `Src == Tgt`: una **rotta lunga zero** |

Il secondo caso non è ipotetico: `RefactorTactics.UI.LogOmitsRememberedEnemyBlockedMove` (`RTCombatLogTests.cpp:834`) lo costruisce apposta e lo documenta. Aggiungere l'outcome al ramo lungo *così com'è* — la correzione che sembra di una riga — produrrebbe quella riga. La condizione giusta è `TgtCell != SrcCell`, ed è scritta nel follow-up perché chi lo implementerà non ci ricada.

Perché fuori: il difetto per cui la #79 esiste è chiuso e **misurato**. Questo è il secondo grado della stessa leggibilità — non «il blocco è muto», ma «il blocco non dice quale passaggio» — e richiede una condizione nuova **più** un test che copra entrambi i rami: è lavoro DEV-MAIN e DEV-TEST, non un'integrazione. Rifarlo qui invaliderebbe una misura EDITOR valida e costerebbe una run completa. EDITOR aveva già proposto la stessa uscita e non ha toccato produzione per allineare il testo a un'aspettativa: la disciplina è corretta.

**F10 — REGISTRATO.** Errore di EDITOR, auto-diagnosticato e corretto: il log della suite veniva scritto con `Tee-Object` **dentro** la working directory che `rt-suite` stava misurando, e la misura si è invalidata da sola (`albero 3353f578 -> 88c9afdc`). `174/174, 0 fail` — e **non registrabile**. Rilanciata con il log fuori dal repository; le tre misure riportate sono `VALIDA`.

🔑 Vale la pena conservarlo, perché è un caso particolare di una regola nota che qui morde in modo controintuitivo: **raccogliere l'evidenza dentro il repository invalida la misura che l'evidenza documenta**. La cartella `evidence/` è la destinazione giusta per l'artefatto, ma non mentre la suite gira. Nessuna azione: EDITOR ha applicato `NON VALIDA` invece di registrare il verde, che è esattamente ciò che il contratto chiede.

## Risposte ai Finding di VALIDATION su `fa9e3ed2`

VALIDATION ha chiuso `PARTIAL`: **tutti i gate eseguibili verdi**, gli oracoli della #79 verificati **sul TurnLog e non sul conteggio**, e tre check umani che restano `NOT RUN` perché nessuno strumento li dà.

Il criterio che ha aperto la issue, letto sulla run di VALIDATION:

```text
T3   Gadget: resta (q=-1,r=0,L=0) (Action.Move, p50)                    ← Stayed: non dichiarò
T4   Gadget: fermo: cella occupata (q=-1,r=0,L=0) (Action.Move, p50)    ← BlockedByUnit = 1
```

⚠️ Vale la pena notare **come** l'ha verificato: `Movement.CollisionChoke` passa `6/6`, ma *«un `6/6` è un conteggio»* — ha riletto il TurnLog della propria run voce per voce. `BlockedContested = 4`, `BlockedByUnit = 1`, `Moved = 1`, tutti e tre confermati sul dato.

| Finding | Severità | Risposta |
|---|---|---|
| `…/1-F11` — tre `ERTMoveOutcome` senza testo | `P2` | **ACCOLTO — fuori wave, [#2628](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2628)** |
| `…/1-F12` — tre rossi ereditati da `main` | `P2` | **ACCOLTO — misurata l'ereditarietà; uno già in #2551, due in [#2629](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2629)** |
| `…/1-F13` — il branch si muove sotto chi misura | `P3` | **DECISO — vedi sotto, insieme a `1-F8`** |

**F11 — ACCOLTO, fuori wave.** Il formatter traduce 13 dei 16 `ERTMoveOutcome`: `BlockedByTopology` (6), `BlockedByCycle` (12) e `Fell` (15) cadono nel ramo di fallback, e il giocatore legge «esito di movimento non tradotto (15)». **Preesistente e misurato tale**: lo stesso messaggio è in `rt-suite.log` delle 08:32, sette ore prima che il fix fosse scritto — fuori dal write-set, non una regressione di questa wave.

È lo stesso difetto che #79 chiude, su un altro esito, e va nella stessa direzione di `1-F9`/#2627: **stessa famiglia, canale diverso**. La DoD che entrambi toccano — *«ogni esito spiegabile leggendo il log»* — non giustifica di riaprire una wave consolidata e misurata per un difetto che le preesiste.

⚠️ Il ramo di fallback **non va tolto**: è la rete che ha reso il reperto visibile, e il suo commento lo dice già. Ciò che manca è un test di **esaustività dell'enum** — oggi nessuno copre quel confine, ed è la ragione per cui tre valori sono rimasti indietro senza che nulla diventasse rosso. È nella DoD di #2628.

**F12 — ACCOLTO, e l'ereditarietà è misurata.** VALIDATION dichiarava l'ipotesi senza la prova, e nominava la misura mancante. Ne ho fatta una che non richiede il motore:

```text
git merge-base --is-ancestor 748fc090 a59671c8    -> vero
git merge-base --is-ancestor 748fc090 origin/main  -> vero
```

`748fc090` — il rename `Hero.Riktor` → `Hero.Branth`, 2026-09-05 19:32 — è **antenato del `BASE_SHA` della wave**, e nessuno dei tre test è toccato dal write-set. I rossi esistevano **alla biforcazione**.

➕ E per uno dei tre la prova è già scritta da qualcun altro: **[#2551](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2551)** documenta `UI.Icon.Identity.Branth` dicendo *«il gate è rosso su main»*. Gli altri due non erano tracciati: ora sono in **#2629**.

⚠️ **Questo prova l'ereditarietà, non la causa.** Che il rename *sia* la causa resta l'ipotesi di VALIDATION; confermarla richiede la full suite su `origin/main`, che nessuno ha eseguito. Non la do per fatta, ed è scritta come primo punto della DoD di #2629.

🔴 Un reperto che ho trovato rispondendo, e che vale più del finding: il test prescrive per il proprio rosso di *«rimisurare `BOT-STALL-1` e aggiornare `OPEN_DECISIONS`»* — ma **`BOT-STALL-1` è chiusa** dal 2026-08-29 (`D-244`). La prescrizione punta a una decisione già presa: chi raccoglie quel rosso non ha una sede dove scrivere l'esito. È nella DoD di #2629.

**F13 + F8 — DECISI insieme, perché sono la stessa domanda.**

`1-F13` misura che il branch di wave si è mosso sotto una sessione di misura **tre volte, in tre ruoli diversi**, e osserva che nessuna delle tre ha invalidato una misura *«ma è coincidenza favorevole, non una protezione»*. `1-F8` chiede di decidere se §5 vada letto come uguaglianza stretta. Due ruoli hanno applicato la stessa lettura; nessuno dei due poteva ratificarla.

**Decisione, valida per questa wave:**

> Una misura è valida se il delta fra `BASE_SHA` e `HEAD` **non tocca i path che la misura osserva**, e la sessione lo **dichiara** con il comando che l'ha verificato.

Non è un allentamento di §5: è ciò che §5 protegge, cioè la stabilità del **soggetto** misurato. `CLAUDE.md` §6 già norma il caso peggiore — se il soggetto cambia *durante* la finestra, la misura è `NON VALIDA`, non `FAIL` — e `rt-suite` ha il rilevatore, il marker `[RT-MEASURE]`, che ha segnalato il movimento **senza che nessuno glielo chiedesse**.

⛔ **Ciò che la decisione NON copre**, e che `1-F13` ha ragione a chiamare coincidenza: non esiste nulla che *impedisca* a un commit di codice di arrivare durante una misura. Tre volte su tre erano documenti e PNG. Alla quarta potrebbe non esserlo, e nessuno se ne accorgerebbe se non guardando il marker.

➡️ **La finestra di misura è la protezione mancante**, e la propongo come emendamento a `RT3_CONTRACT.md` §5: *mentre un ruolo misura, il branch di wave non riceve commit*. **Non ho modificato il contratto**: è fuori dal write-set di questa wave e appartiene al suo owner. Registrato qui, con la misura che lo motiva, perché la prossima wave non riparta da zero.

## `NOT RUN`

| Elemento | Motivo |
|---|---|
| Build / compile | dominio `VALIDATION`. Il ruolo DEV non occupa Unreal |
| `rt-suite`, Automation, Scenario Harness | come sopra |
| PIE, `PIE-V01-LOG`, `PIE-V01-COLL` (c) | dominio `EDITOR` / `VALIDATION` |
| Rigenerazione del corpus golden | non ancora misurata come necessaria |

`RT3_CONTRACT.md` §6: `NOT RUN` non conta come `PASS`. Nessuna riga di questo file è un verdetto.

## Chiusura del consolidamento — il ciclo di rientro ha un ingresso

Handoff: [`RT3-DEVLEAD-56f8c5c.md`](RT3-DEVLEAD-56f8c5c.md) — wave `issue-79-combat-log-blocked-move/**2**`, `STATUS: PARTIAL`.

Le risposte ai Finding vivevano **solo in questo file**, che non è uno dei tre `RT3-*` di §10. Il ciclo di rientro non aveva quindi un ingresso leggibile per il ruolo a valle, che per contratto legge `RT3-DEVLEAD-<sha7>.md` dal filesystem e non ricostruisce l'handoff dalla chat. Ora ce l'ha.

| Punto dell'`UNBLOCK` | Esito | Sede |
|---|---|---|
| 1 · decidere `1-F11` e scrivere la decisione | ✅ chiuso — fuori wave, e la lettura della DoD è scritta su #79 | #2628 · commento su #79 |
| 2 · sanare o dichiarare `1-F12` | ✅ dichiarato — la misura conclusiva resta `NOT RUN`, con esecutore nominato | #2551 · #2629 |
| 3 · decidere `1-F8` | ✅ deciso, **e il predicato corretto** | #2638 |
| 4 · PR sul `PARENT_BRANCH` dichiarato | ✅ aperta su `main`, draft — e misurata **non mergiabile** per cause estranee | PR #2631 |

### 🔴 Correzione alla decisione scritta sopra su `F13 + F8`

Il merito regge. **Il predicato no**, ed è stato corretto misurando.

Il blocco *«F13 + F8 — DECISI insieme»* di questo stesso file scrive la lettura come:

> *il delta fra `BASE_SHA` e `HEAD` non tocca i path che la misura osserva*

e il comando che la verifica è `git diff --name-only BASE_SHA..HEAD -- <path>`. **Quel comando risponde alla prima delle quattro condizioni di §5 e tace sulla terza** — *«il working tree contiene modifiche non dichiarate nel write-set in ingresso»*. Un file non committato non compare in un `diff` fra due commit, né se è `untracked` né se è una modifica non messa in stage.

Misurato il **2026-09-06T18:07:03Z** su `HEAD 56f8c5cd`, mentre il consolidamento era in corso:

```text
git diff --name-only fa9e3ed2..HEAD -- Source/ Content/ Config/ Scenarios/   -> 0 file
git status --porcelain              -- Source/ Content/ Config/ Scenarios/   -> 7 voci
```

Sette file di C++ di un'altra sessione, sotto `Source/RefactorTactics/Map/` — una directory del modulo, quindi compilata — di cui uno è un file di test. Il predicato diceva **verde**.

**Decisione corretta, operativa per questa wave:**

> Una misura è valida se, sui path che osserva, **entrambi** questi comandi tacciono:
>
> ```text
> git diff --name-only <BASE_SHA>..<HEAD> -- <path misurati>   -> vuoto
> git status --porcelain                  -- <path misurati>   -> vuoto
> ```
>
> e la sessione li dichiara **con il loro output**, non con la loro conclusione.

⚠️ Un predicato che nomina una condizione su quattro **autorizza esattamente ciò che la condizione taciuta vietava**. È il motivo per cui la domanda non poteva restare registrata solo qui.

➡️ La domanda di contratto ha ora una **sede**: [#2638](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2638). ⛔ `RT3_CONTRACT.md` **non è stato modificato**, e alla ragione di owner se ne aggiunge una misurata: il branch è **45 commit indietro** rispetto a `origin/main`, dove `D-342` ha già riscritto la sezione `Aperti`. Una riga aggiunta da qui colpisce la stessa tabella.

### Cosa il consolidamento ha misurato e nessuno aveva misurato

| Misura | Esito |
|---|---|
| `git merge-tree --write-tree --name-only origin/main HEAD` | **10 conflitti `add/add`**, tutti `tools/rt3/` + `RT3_CONTROL_PLANE.md`. **Zero** su `Source/` `Content/` `Scenarios/` `Config/` |
| `RTTurnManager.cpp`, toccato da entrambi i lati | **auto-merge**: `main` cambia `RunReactionPass`/`ResolveCombatPasses` (~4966–5983), la wave `ResolveMovement` (~7150–7609). Regioni disgiunte |
| i 3 test rossi di `1-F12` su `origin/main` dopo la biforcazione | **0 commit** ciascuno. Catalogo icone e `Content/`: **0 file** cambiati |
| `RT3_CONTRACT.md` §5 su `origin/main` | **invariato** — la domanda di `1-F8` è aperta sul contratto vivo, non su una copia stantia |
| `RT3_CONTRACT.md` §6 su `origin/main` | ⚠️ **cambiato**: `D-342` ha reso canonico il vocabolario di `SEED_SOURCE` e ha **ritirato `fixed`**. Il contratto su questo branch non è quello vivo |

🔑 **Il codice della wave mergia pulito.** I dieci conflitti sono al 100% del control plane `rt3`, atterrato qui da altre sessioni: è il costo — ora misurato — di ciò che la nota di consegna aveva già dichiarato, e che `1-F13` descrive sulla consegna invece che sulla misura.

⛔ **Non risolti, e non per prudenza**: arbitrare due stesure parallele di `tools/rt3/store.py` è fuori dallo scope di un DEV-LEAD della #79.

### Il collo di bottiglia non è un Finding

Con tutti e cinque i Finding chiusi o instradati, la wave resta `PARTIAL`: i **tre check a oracolo umano** restano `NOT RUN` e non li produce nessuna suite. Guida operativa: [`guida-seduta-chiusura-79-denial-e-log.md`](../../../technical/runbooks/guida-seduta-chiusura-79-denial-e-log.md).

🔴 **Precondizione della seduta**, dalla misura qui sopra: la guida si apre con *«il passo che viene prima di tutto: ricompilare»*, e compilare adesso significa compilare anche i sette file di un'altra sessione. Prima della seduta `git status --porcelain -- Source/` deve tacere, o la seduta deve dichiarare cosa c'era dentro il binario che ha misurato.
