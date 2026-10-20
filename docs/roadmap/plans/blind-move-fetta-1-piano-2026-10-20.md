# Blind Move, fetta 1 — pianificare un Move verso lo spazio non osservato

> `SNAPSHOT` · fotografia del 2026-10-20: vale finché è l'ultima misura del suo oggetto.

**Data**: 2026-10-20 · **Misurato su**: `main` `abe0dffd0` · **Issue**: [#2794](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2794) · **Decisioni**: [D-249](../../decisions/RT_PDR_00_Decision_Log.md), [D-371](../../decisions/RT_PDR_00_Decision_Log.md), [D-372](../../decisions/RT_PDR_00_Decision_Log.md), [D-432](../../decisions/RT_PDR_00_Decision_Log.md), [D-465](../../decisions/RT_PDR_00_Decision_Log.md) · **Stato**: seconda stesura, dopo la review avversaria; **attuato** sul branch `issue/2794-blind-move` nelle fette A, B, C ed E (vedi §9); la D e' un verdetto dell'autore sulle tavole e non e' stata fatta

> Non è una decisione: nessuna riga di questo documento cambia il Decision Log. Dice **come** si attua ciò che D-372 ha già deciso, e dove la misura dice di fermarsi.

> ⚠️ **La prima stesura di questo piano era sbagliata in tre punti**, trovati dalla review avversaria del 2026-10-20 e misurati sul codice: (1) diceva che Resolution «si ferma» sempre dove la verità la ferma, ma i bot **non portano un percorso** e Resolution lo **ricalcola**; (2) proponeva di sostituire `Snapshot.Map` con una copia, che rompe `IsSnapshotStale` e non dice chi possiede la copia; (3) diceva che il canary «si rovescia», ma il canary non passa dal ramo nuovo. La §8 elenca che cosa è cambiato.

## 1. Che cosa è già deciso, e che cosa no

| | Stato | Dove |
|---|---|---|
| L'occupazione autorevole non è pubblica, bot compreso | deciso | D-371 |
| La **forma** della board è pubblica, il **contenuto** no | deciso | D-373 |
| Un Move verso l'ignoto si pianifica su mappa ottimistica, tracciato oltre il noto dichiarato incerto | deciso | D-372 |
| L'osservatore è un ingresso di `MakeSnapshot` e nessun default silenzioso | deciso, attuato | D-432 |
| Nessun auto-reroute in Resolution | deciso | D-045, #2501 |
| Click e portata ricevono la **stessa** vista **dal commit** che la porta a `FindPath`, con la riga di TurnLog per il tiro scartato in un Fumo ignoto | deciso, a termine | D-465 punto (3) |
| Celle **esplorate ma non visibili ora**: quale geometria? | **aperto** | `Q4` del referto delle aree di contesto |
| Quale osservatore sono i bot | non toccato qui | `OBS-1` |

Il blocco che #2794 dichiarava (`BLIND-2` → `OBS-1`) è caduto: D-432 chiude `BLIND-2`. `OBS-1` resta aperta con scadenza lontana e non blocca un Move cieco.

## 2. Che cosa esiste già, misurato

- **Resolution è onnisciente e non riusa lo snapshot di pianificazione.** `Ctx.Snapshot` nasce da `MakeCurrentSnapshot(Units, RTObserver::Omniscient)` (`Turn/RTTurnManager_Movement.cpp:112`), come Dash e Blast. Gli snapshot per osservatore (`PlanningSnapshotFor`, `PlanBots`/`AsObserver`) restano locali.
- **Il percorso pianificato si cammina solo se esiste.** `RTTurnManager_Movement.cpp:186-193` usa `PlannedPath` se ha almeno 2 celle e parte da `Unit->Cell`; altrimenti chiama `FindPathForUnit` sulla mappa onnisciente, cioè **ricalcola**.
  - Il **giocatore** porta un `PlannedPath` costruito al click. Lo perde dopo uno Scatto che si applica (`RTPlanPreview.cpp:321-326`) o dopo uno spostamento forzato prima del Move (`RTTurnManager.cpp:2385-2390`).
  - I **bot** non lo portano mai: `Bot/RTBotPlanningLibrary.cpp:125` fa `PlannedPath.Reset()` e imposta solo `PlannedCell`. Per loro Resolution ricalcola sempre, oppure dà `NoPath` ed esito «Stayed».
- **La regola d'arresto canonica ferma un muro vero.** `TruncatePathToTopology` → `StepIsWalkable` → `GraphNeighbors`, che esclude `bBlocksMovement` e i bordi chiusi, poi `CanTransitCell` per i muri interni; il taglio al primo passo alza `bStoppedByTopology` (`RTTurnManager_Movement.cpp:277-278`). Vale **solo** sul percorso portato dal piano.
- **`TruncatePathToBudget` precede la topologia** e legge i costi veri: un tratto più caro del previsto viene accorciato senza alcun flag.
- **Il precedente della copia con celle vuote** è `URTEnemyArcLibrary` (#3762), con `DuplicateObject` in un pacchetto transitorio. Azzera solo vista, superficie, coperture, porte, guardie e muri interni: **non** `bBlocksMovement`, `MoveCost`, `OccupancySurcharge`, `Transitions`, `Height`.
- **`IsSnapshotStale`** (`RTHexSimLibrary.cpp:270`) confronta `ComputeHash()` e `Revision` di `Snapshot.Map` con quanto catturato; l'hash include `bBlocksMovement`, coperture, porte e muri interni.
- **`Snapshot.Map` è un puntatore grezzo non posseduto** (`RTHexSim.h:578-581`, nessuna protezione dal GC), e `MakeSnapshot` è pura e non riceve un `Outer`.

## 3. Il disegno

1. **Il giocatore e basta, in questa fetta.** I bot non portano un percorso e Resolution li ricalcola: una geometria ottimistica per loro produrrebbe esattamente l'auto-reroute che D-372 vieta, o un `NoPath` per un muro vero mai visto. Restano sulla vista attuale e lo dichiara il Decision Log, come D-532 per il minimo del mortaio. Riaprire la domanda richiede che il bot **porti** il percorso, ed è una modifica del bot, non del Move del giocatore.
2. **La geometria ottimistica è un campo a parte, non una sostituzione di `Map`.** `FRTHexSnapshot::Map` resta la mappa autorevole: hash, revisione e staleness non cambiano. Nasce un secondo puntatore, `PlanningMap`, valorizzato solo per un osservatore diverso da `RTObserver::Omniscient` con conoscenza **non vuota**, e **letto solo** dalle primitive del Move del giocatore (`ReachableCells*`, `FindPathForUnit`, `ClassifyWaypointCell`, `TruncatePathToBudget` lato anteprima). Con conoscenza vuota o assente `PlanningMap == Map`: nessun cambio di comportamento per i test e le sessioni senza `ARTGameMode`.
3. **Chi possiede la copia.** `MakeSnapshot` riceve un `UObject* Outer` per la copia (pacchetto transitorio, come `RTEnemyArcLibrary`), e la copia si **riusa** per `ReachableCellsAfterPlan` invece di rifarla: una copia per interazione di pianificazione, non una per chiamata. La chiave è `Revision` della mappa autorevole più un identificatore della conoscenza.
4. **Che cosa si svuota in una cella mai osservata**: `bBlocksMovement`, `bBlocksLineOfSight`, `Surface`, `MoveCost` (a 1), `OccupancySurcharge` (a 0), coperture, porte, guardie, muri interni. **Non** si toccano `Transitions` fra piani: la forma della board è pubblica (D-373). La cella **esiste** sempre.
5. **«Mai osservata» = né in `VisibleCells` né in `ExploredCells`.** `ExploredCells` è un sovrainsieme di `VisibleCells` solo dopo il refresh della conoscenza: l'unione è obbligatoria.
6. **Celle esplorate ma non visibili ora** (`Q4`, aperta): restano sulla mappa autorevole. È un canale dichiarato, a termine, della stessa natura di D-465: un muro caduto o una porta aperta in una cella ricordata si riflette nel percorso.
7. **Resolution** resta onnisciente e invariata, e per il giocatore si ferma dove la verità la ferma. Nessun reroute **per il giocatore**; lo Scatto che si applica e lo spostamento forzato lo fanno già oggi ricalcolare, ed è un limite dichiarato, non nuovo.
8. **Il motivo dell'arresto.** `BlockedByTopology` dice «fermo: il varco si è chiuso», falso per un muro mai visto. Serve un esito proprio, aggiunto **in coda** all'enum (il valore serializzato è l'indice). Anche l'arresto per budget (`TruncatePathToBudget`) non ha oggi un flag: **non e' stato attuato**, e D-534 punto (5) lo dichiara come limite.

## 4. Sequenza e criteri

| Fetta | Contenuto | Criterio binario |
|---|---|---|
| **0** | Censimento dei siti che leggono `Snapshot.Map` e `AsObserver`; misura dei gate dei bot (`Bot.StallDefinitions…`, `Match.Autobattle`) con la suite intera **prima** | Ogni sito classificato «vede `PlanningMap`» / «vede `Map`»; la baseline dei gate scritta |
| **A** | `PlanningMap` in `MakeSnapshot`, solo giocatore | **Con e senza muro mai osservato**: stessa anteprima, stesso costo, stesso reason code. Il test passa da `MakeSnapshot`/`AsObserver` con un osservatore vero, e la mutazione «`PlanningMap = Map`» lo rende rosso. Il canary del movimento diventa il suo opposto |
| **B** | Resolution con muro reale | Il movimento del giocatore si ferma all'ultima cella valida; un test è rosso se il Move ricalcola verso la destinazione **quando il piano porta un percorso** |
| **C** | Motivo dell'arresto | Un esito in coda all'enum, un testo che non dica «varco chiuso», nessuna informazione sull'occupante prima della Resolution |
| **D** | Resa del tracciato incerto | Verdetto dell'autore sulle tavole; non è una regola di gioco |
| **E** | Click e portata con la stessa vista, riga di TurnLog per il tiro scartato in un Fumo ignoto, rovesciamento del canary di #3270 | Vedi §7: D-465 lo vuole **nello stesso commit** di A; tocca schema, replay e digest |

## 5. Rischi, con la misura che li chiude

- **I bot.** Restano fuori (§3.1), ma la suite intera **prima e dopo** A resta il gate: lo snapshot per osservatore è condiviso e un cambio mal confinato li sposterebbe.
- **Costo della copia.** Si misura a motore libero.
- **Costi mostrati.** Una cella mai vista con costo reale 3 appare a costo 1: l'anteprima può costare meno del vero, e Resolution può fermarsi per budget, senza un motivo (limite dichiarato in D-534 punto (5)).
- **`AsObserver` componibile male.** Data una vista già ottimistica di A, `AsObserver(…, B)` ricostruirebbe da una `Map` che nel disegno resta autorevole: il problema sparisce **solo** perché `Map` non si sostituisce. Va comunque asserito.
- **Privacy.** L'insieme delle celle vuotate dipende solo dalla conoscenza dell'osservatore, mai dalle unità nemiche: si verifica sulla firma della funzione.

## 6. Non misurato

- Quanti test esistenti passano da `PlanningSnapshotFor` o da `MakeCurrentSnapshot(…, TeamId)` con conoscenza vuota (il disegno li lascia invariati per costruzione, ma non l'ho contato).
- La classificazione dei siti di lettura di `Snapshot.Map` (è la fetta 0).
- Il costo della copia e della sua cache.
- Il testo di D-249 e D-371 non è stato riletto per intero in questa stesura.

## 7. Decisioni che servono all'autore

| Domanda | Default usato | Stato |
|---|---|---|
| Celle esplorate ma non visibili (`Q4`) | mappa autorevole, canale dichiarato a termine | da confermare |
| Bot | fuori dalla fetta 1, sulla vista attuale, dichiarato nel Decision Log | da confermare |
| **D-465 punto (3) contro questo piano** | due commit nello stesso merge, con emendamento esplicito | ✅ **risolto dall'autore il 2026-10-20** (D-534) |

⛔ **Il conflitto.** D-465 dice che dal commit che porta la vista ottimistica a `FindPath`, click e portata ricevono la stessa vista, e che in quel commit si paga la riga di TurnLog per il tiro che il resolver scarta in silenzio in un Fumo ignoto, e il canary di #3270 si rovescia. Questo piano tiene il movimento (A–D) separato da click e portata (E). Ma `Player/RTPlayerController.cpp:2404` e `:5209` e `Turn/RTPlanPreview.cpp:237` passano la mappa a `ResolveLinearMove`: separare le due cose lascia un commit in cui il movimento è ottimistico e la mira no, cioè il canale che D-465 voleva chiudere insieme. Due uscite:
1. **Tutto in un commit** (A + E): rispetta D-465 alla lettera, ma tocca schema, replay e digest insieme alla geometria del Move.
2. **Due commit con un'emendamento esplicito di D-465**: A–D prima, E dopo, e una riga nel Decision Log che dice che la finestra fra i due è voluta e quanto dura.

Non scelgo io fra le due: è una decisione dell'autore. ✅ **L'autore ha scelto la seconda il 2026-10-20**: l'emendamento e' `D-534`, e la finestra fra i due commit vive nella storia del branch e non su `main`.

## 8. Che cosa è cambiato rispetto alla prima stesura

| Prima | Ora | Perché |
|---|---|---|
| «Resolution invariata: il piano si cammina e si ferma» | Vale per il giocatore che porta un percorso; i bot restano fuori | I bot non portano `PlannedPath` e Resolution li ricalcola (`RTBotPlanningLibrary.cpp:125`, `RTTurnManager_Movement.cpp:186-193`) |
| `Snapshot.Map` sostituita da una copia | Un campo `PlanningMap` a parte | `IsSnapshotStale` confronta hash e revisione di `Map`; la copia ha un hash diverso |
| Nessuna parola sul possesso della copia | `Outer` transitorio, copia riusata | `Snapshot.Map` è un puntatore grezzo senza protezione dal GC |
| «Costo base» | Elenco dei campi da svuotare | `MoveCost` si deriva da `Surface` e `OccupancySurcharge` fa filtrare il costo |
| Il canary «si rovescia» | Il test va riscritto per passare da `MakeSnapshot` | `NeverObservedWallBendsTheLitPath` usa `MakeSnapshotOmniscient` e non tocca il ramo nuovo |
| Criterio A: stessa anteprima con 0, 1, 2 nemici | Stessa anteprima **con e senza muro mai osservato** | Il criterio sugli occupanti è già soddisfatto da D-432 |
| Click e portata a parte, senza dirlo | Conflitto con D-465 (3) dichiarato | Il piano contraddiceva una decisione registrata |
| Conoscenza vuota non definita | `PlanningMap == Map` con conoscenza vuota o assente | Senza `ARTGameMode` nessun refresh della conoscenza |

## 9. Come e' stato attuato (2026-10-20)

| Fetta | Commit | Esito |
|---|---|---|
| **0** | — | Censimento: i siti che leggono `Snapshot.Map` per la pianificazione del Move sono in `Turn/RTHexSimLibrary.cpp`; quelli di Resolution (`RTTurnManager_Movement.cpp`, `RTTurnManager.cpp`) e dei bot (`RTBotPlanningLibrary.cpp`, `RTHexBotLibrary.cpp`) **non** sono stati toccati. Baseline: suite intera verde sul `main` di partenza |
| **A** | `PlanningMap` | `FRTHexSnapshot::PlanningMap`, `URTHexSimLibrary::MakeOptimisticPlanningMap`, `ARTTurnManager::AttachPlanningMap` chiamato da `PlanningSnapshotFor`. Test `BlindMove.OptimisticMapEmptiesOnlyTheNeverObserved` e `BlindMove.PlayerPlansThroughANeverObservedWall`; mutazioni «`PlanningMap = Map`», «solo `ExploredCells`» e «non svuotare `bBlocksMovement`» rosse |
| **B** | test | `BlindMove.ResolutionStopsAtTheRealWallAndDoesNotReroute`: nessun codice nuovo, la Resolution gia' si fermava; il test pinna anche il limite dei bot. Mutazione «Resolution ricalcola sempre» rossa |
| **C** | `BlockedByUnseenTerrain` | Nuovo esito in coda a `ERTMoveOutcome`, scelto dalla Resolution quando il passo negato porta in una cella ne' vista ne' ricordata. Test `BlindMove.UnseenWallStopsWithItsOwnReason`; due mutazioni rosse |
| **D** | — | **Non fatta**: e' il verdetto dell'autore sulle tavole del tracciato incerto |
| **E** | click, portata, TurnLog | `ARTTurnManager::PlanningMapFor`/`PlayerAimMap` alimentano i siti di mira del controller (anteprima, click su unita', rifiuto sotto il puntatore, click su cella, bordo), il HUD e il Dash lineare; `FRTHexBlastPlan::TerrainCappedIntents` scrive `Fallback`/`Cancelled`/`OutOfRange` per il tiro a cella cappato dal Fumo. Canary di #3270 rovesciato; `BlindActions.UnexploredSmokeDoesNotRefuseTheSlotOrTheClick` e `BlindFireOffensive.MortarShotCappedBySmokeLeavesALine`; mutazioni rosse |

**Non misurato**: il costo della copia e della sua cache (`PlanningMapFor` calcola `ComputeHash` a ogni chiamata), la PIE su `SmokeDark` (NOT RUN, vedi `PIE-V01-REFUSAL`), i siti di mira non coperti da una mutazione (`RefusalUnderPointer`, il bordo, il HUD, il Dash lineare).
