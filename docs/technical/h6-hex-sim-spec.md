# Spec H6.1–H6.3 — Simulazione su griglia esagonale (snapshot, budget, collisioni, TurnLog)

> **Stato**: **Implementata** (TDD RED→GREEN; suite **140/140**, build Editor Succeeded) · **Data**: 2026-08-05 · **Branch**: `feat/hex-sim`
> **Fonte**: milestone **H6** di [`hex-map-roadmap.md`](../roadmap/hex-map-roadmap.md) («Integrazione simulatore»).
> Non tocca il turn loop quadrato (base di rollback, [ADR-0002](../decisions/adr-0002-griglia-esagonale.md)).

## 1. Contesto e obiettivo

L'editor esagonale (H0–H5c.7) produce mappe autorevoli (`URTHexMapAsset`) che **il simulatore non consuma**:
il turn loop lavora su `FRTGridCoord` e `URTGridLibrary` (griglia quadrata). H6 chiude il ponte.

**Obiettivo di questo slice**: gli ingredienti **puri e testabili** della risoluzione di un turno su hex —
**snapshot** della mappa + **occupazione**, **movement budget** su costi interi, **collisioni simultanee**.
Fuori da questo slice: il wiring in `ARTTurnManager` (che resta quadrato finché l'hex non è completo).

## 2. Decisioni di design (fissate col dev)

| # | Decisione | Motivo |
|---|-----------|--------|
| D1 | **Strato hex parallelo** in `Turn/RTHexSim*` su `FRTCellId`; `Grid/` e `ARTTurnManager` **intatti** | Il quadrato resta base di rollback (ADR-0002); zero rischio di regressione sui test esistenti. Lo switch del TurnManager è uno slice successivo. |
| D2 | Lo snapshot **copia l'occupazione** ma **referenzia** la mappa (puntatore + `MapHash` + `Revision`) | L'occupazione cambia durante la fase → va congelata. La mappa è immutabile per tutto il turno: copiarla a ogni fase sarebbe costoso. `IsSnapshotStale` rileva la violazione invece di ignorarla. |
| D3 | `FRTHexSnapshot` è uno **struct C++ puro** (non `USTRUCT`), vita = una risoluzione di fase | Contiene un puntatore non-`UPROPERTY` all'asset: renderlo esposto inviterebbe a conservarlo oltre la fase (rischio GC). Le funzioni sono `static` non-`UFUNCTION`, come `URTHexPathLibrary::GraphNeighbors`. |
| D4 | Identità dell'unità = **`UnitId` intero stabile**, mai un pointer | Determinismo e replay (stesso criterio del TurnLog, che usa la cella di partenza come chiave). |
| D5 | Il **budget** entra nel pathfinding (`MaxCost` dell'A* già esistente), non nel resolver | Il resolver dei microstep consuma path **già** troncati al budget: una responsabilità per funzione. |
| D6 | Le celle occupate da **altre** unità sono ostacoli per path e reachable; la cella dell'unità stessa no | Un'unità non blocca sé stessa; il resto è la regola «max 1 unità per cella». |
| D7 | **Duplicazione dichiarata** della logica microstep (quadrata in `RTMovementResolver`, hex in `RTHexSimLibrary`) | Il quadrato è destinato a sparire allo switch: la duplicazione è temporanea per costruzione. I test hex replicano gli scenari quadrati, così una divergenza fallisce invece di passare inosservata. |

## 3. Architettura

### 3.1 Tipi — `Source/RefactorTactics/Turn/RTHexSim.h`

- `FRTHexSimUnit` (`USTRUCT`) — `UnitId` (int32), `Cell` (`FRTCellId`), `bAlive`, `MoveBudget` (costo massimo
  spendibile nel turno; 0 = immobile).
- `FRTHexSnapshot` (struct puro) — `Map` (`const URTHexMapAsset*`), `MapHash` (uint32), `Revision` (int32),
  `Units` (ordinate per `UnitId`), `Occupancy` (`TMap<FRTCellId,int32>` cella → `UnitId`, **solo unità vive**).
- `FRTHexMoveResult` (`USTRUCT`) — `Final` (`FRTCellId`), `Entered` (celle attraversate, esclusa la partenza),
  `Outcome` (`ERTMoveOutcome`, riusato dal TurnLog: Stayed/Moved/BlockedContested/BlockedByUnit).
- `FRTHexReachableCell` (`USTRUCT`) — `Cell` + `Cost` (costo cumulato dalla partenza).

### 3.2 Funzioni — `Turn/RTHexSimLibrary.{h,cpp}` (pure, statiche)

| Funzione | Comportamento |
|---|---|
| `MakeSnapshot(Map, Units)` | Ordina le unità per `UnitId` (ordine stabile), costruisce `Occupancy` con le sole unità vive (in caso di collisione vince l'`UnitId` minore → deterministico), cattura `ComputeHash()` e `Revision`. |
| `ValidateSnapshot(Snapshot)` | Errori strutturali: due unità vive sulla stessa cella, unità su cella assente dalla mappa, `UnitId` duplicati, `MoveBudget` negativo. Stessa forma di `URTHexMapAsset::ValidateMap`. |
| `IsSnapshotStale(Snapshot)` | `true` se l'asset non c'è più o se `ComputeHash()`/`Revision` sono cambiati dopo la cattura (fail-fast sull'invariante «la mappa non cambia durante la risoluzione»). |
| `IsCellFree(Snapshot, Cell, ForUnitId)` | La cella esiste, non ha `bBlocksMovement` e non è occupata da un'unità **diversa** da `ForUnitId`. |
| `ReachableCells(Snapshot, UnitId)` | Dijkstra sui costi interi (`URTHexPathLibrary::GraphNeighbors`, archi inclusi) entro il `MoveBudget` dell'unità, saltando le celle occupate da altri. Include sempre la cella di partenza a costo 0. Output ordinato per cella (`StableLess`) → indipendente dall'ordine di `TMap`. |
| `FindPathForUnit(Snapshot, UnitId, Goal)` | A* con `MaxCost = MoveBudget` che evita le celle occupate da altri; goal occupato → `NoPath`. |
| `ResolveHexPaths(Paths)` | Microstep sincroni: a ogni passo tutte le unità avanzano di una cella; destinazione contesa (2+) → contendenti fermi da lì; cella di un'unità ferma → bloccata; scambio diretto → consentito. Punto fisso monotono → **indipendente dall'ordine** delle richieste. |

### 3.3 Estensione minima del pathfinding

`URTHexPathLibrary` guadagna un overload `FindPathAvoiding(Map, Start, Goal, const TSet<FRTCellId>* Blocked, MaxCost, MaxNodes)`;
`FindPath` diventa una chiamata con `Blocked = nullptr` (comportamento invariato, coperto dai test `RefactorTactics.HexPath.*`
esistenti). Motivo: l'occupazione è dinamica e non appartiene all'asset mappa.

## 4. Test — `Tests/RTHexSimTests.cpp` (TDD RED→GREEN), prefisso `RefactorTactics.HexSim.*`

| Test | Comportamento |
|---|---|
| `SnapshotCapturesOccupancy` | occupazione delle sole unità vive; hash/revision catturati |
| `SnapshotOrderIndependent` | stesse unità in ordine diverso → snapshot identico (unità ordinate, stessa occupazione) |
| `SnapshotStaleAfterMapChange` | modifica dell'asset dopo la cattura → `IsSnapshotStale` |
| `ValidateDetectsTwoUnitsOnSameCell` | due unità vive sulla stessa cella → errore |
| `ValidateDetectsUnitOffMap` | unità su cella assente → errore |
| `ReachableRespectsBudget` | budget 2 su costi 1 → esattamente le celle a distanza ≤ 2 |
| `ReachableRespectsTerrainCost` | cella a `MoveCost` 3 fuori da un budget 2 |
| `ReachableExcludesOccupied` | cella occupata da un'altra unità né raggiungibile né attraversabile |
| `ReachableUsesTransitions` | arco esplicito entro budget → cella su un altro layer raggiungibile |
| `PathStopsAtBudget` | goal oltre il budget → `NoPath` |
| `PathAvoidsOccupiedCell` | devia attorno a un'unità ferma; goal occupato → `NoPath` |
| `ResolveContestedDestination` | due unità verso la stessa cella → entrambe ferme prima, `BlockedContested` |
| `ResolveSwapAllowed` | scambio diretto A↔B consentito, `Moved` |
| `ResolveBlockedByStationary` | destinazione di un'unità ferma → `BlockedByUnit` |
| `ResolveOrderIndependent` | permutazione dell'input → stessi esiti per unità |

## 5. Definition of Done (raggiunta)

☑ TDD RED→GREEN (RED misurato: 14/14 falliti con gli stub; GREEN: 14/14 verdi) · ☑ build target **Editor**
`Succeeded` (editor chiuso) · ☑ suite Automation **140/140** misurata (126 preesistenti + 14) ·
☑ nessuna modifica a `Grid/`, `URTMovementResolver`, `ARTTurnManager` · ☑ solo interi nelle coordinate e nei
costi (invariante #4) · ☑ nessuna verifica PIE necessaria (slice interamente headless) · ☑ spec + roadmap H6 +
architettura aggiornate.

**Correzione fuori scope inclusa** (build rotta scoperta durante lo slice): `RTTurnLogLibraryTests.cpp` e
`RTTurnLogSerializationTests.cpp` definivano entrambi un helper `MakeEntry` in un namespace anonimo. Nella
*unity build* i due file finiscono nella stessa translation unit e i namespace anonimi si fondono → `error
C2084` (ridefinizione). Finora l'*adaptive unity build* li teneva separati perché erano nel working set di git:
il difetto era latente e sarebbe emerso in una build pulita/CI. Helper rinominata in `MakeSerEntry`.

---

# H6.3 — TurnLog e replay su celle esagonali

> **Stato**: **Implementata** (TDD RED→GREEN; suite **146/146**, build Editor Succeeded) · **Branch**:
> `feat/hex-turnlog` · Chiude il KPI «Replay divergence = 0» ([`roadmap-checkpoint.md`](../roadmap/roadmap-checkpoint.md))
> **per la topologia esagonale**.

## 3bis. Contesto

`ResolveHexPaths` produce esiti autorevoli che nessuno registra: senza TurnLog non c'è replay né confronto fra
due esecuzioni. `FRTTurnLogEntry` porta 3 interi per cella (`X,Y,Layer`), sufficienti sia per le coordinate
offset quadrate sia per le assiali `q,r,Layer`: la struttura non cambia, cambia solo **come vanno interpretati**.

## 3ter. Decisioni

| # | Decisione | Motivo |
|---|-----------|--------|
| D8 | **Topologia dichiarata nei `reserved/flags`** dell'header (`0 = Square`, `1 = Hex`) | Il campo esiste già ed è scritto a 0: i file esistenti restano leggibili (`flags = 0` → Square) e i byte del quadrato non cambiano. Senza marcatore un log hex e uno quadrato con gli stessi numeri sarebbero indistinguibili → un confronto incrociato darebbe un falso «nessuna divergenza». Loader **fail-closed** su valori sconosciuti, come per la versione. |
| D9 | La topologia **non entra nell'hash** | L'hash confronta due esecuzioni della *stessa* partita, che hanno per costruzione la stessa topologia; la protezione contro il file sbagliato spetta al loader. Così l'hash del quadrato resta invariato e nessun test esistente va ritoccato. |
| D10 | `BuildMoveLog` vive in `URTHexSimLibrary`, non nel TurnLog | Il TurnLog resta agnostico rispetto alla topologia; è lo strato hex a sapere come si traducono i propri esiti in voci. |
| D11 | `SrcCell` = cella di **partenza** del turno (chiave stabile), `Amount` = celle percorse | Stessa disciplina del quadrato (`spec-turnlog.md`): la chiave dell'unità è la cella iniziale, mai un pointer. |

## 4bis. API

- `ERTLogTopology : uint16 { Square = 0, Hex = 1 }` (in `RTTurnLog.h`, non `UENUM`: `uint16` esce dai vincoli
  UHT del `BlueprintType`, come `ERTTurnLogFormatVersion`).
- `URTTurnLogLibrary::SerializeTurnLog(Entries, Topology = Square)` — scrive la topologia nei flags.
- `URTTurnLogLibrary::DeserializeTurnLog(Bytes, OutEntries, OutTopology = nullptr)` — restituisce la topologia
  letta; **rifiuta** i valori sconosciuti (fail-closed).
- `SaveTurnLogToFile(Path, Entries, Topology = Square)` · `LoadTurnLogFromFile(Path, OutEntries, OutTopology = nullptr)`.
- `URTHexSimLibrary::ToLogCoord(const FRTCellId&)` — conversione esplicita cella hex → coordinate di log
  (i tre interi restano `q, r, Layer`: nessuna reinterpretazione geometrica).
- `URTHexSimLibrary::BuildMoveLog(Paths, Results)` — una voce per unità: `Phase = Move`, `Category = Move`,
  `Outcome = ERTMoveOutcome`, `SrcCell` = partenza, `TgtCell` = cella finale, `Amount` = celle percorse.

## 5bis. Test

| Test | Comportamento |
|---|---|
| `HexSim.BuildMoveLogEntries` | mossa / bloccata / ferma → voci con src, tgt, amount e reason corretti |
| `HexSim.MoveLogPermutationInvariant` | permutare le unità → **stesso hash** del log |
| `HexSim.ReplayDivergenceZero` | stessa risoluzione due volte (ordini diversi) → stesso hash; salvato e ricaricato da file → stesso hash + topologia `Hex`; un intento diverso → hash diverso |
| `TurnLog.TopologyRoundTrip` | serializzato come `Hex` → riletto come `Hex`; come `Square` → `Square` |
| `TurnLog.SquareBytesUnchanged` | il default produce byte identici alla serializzazione esplicita `Square`, con flags a 0 (retrocompatibilità dei file esistenti) |
| `TurnLog.RejectsUnknownTopology` | flags con un valore non previsto → `false`, output svuotato (checksum ricalcolato nel test, così a fallire è la topologia e non l'integrità) |

## 7bis. Definition of Done H6.3 (raggiunta)

☑ TDD RED→GREEN (RED misurato: 4 test falliti con gli stub; `MoveLogPermutationInvariant` rinforzato con
l'asserzione sul numero di voci, altrimenti due log vuoti avrebbero lo stesso hash) · ☑ build Editor `Succeeded` ·
☑ suite **146/146** misurata (140 + 6) · ☑ byte del quadrato invariati (retrocompatibilità verificata) ·
☑ i test file puliscono `Saved/` (nessun residuo) · ☑ nessuna verifica PIE necessaria · ☑ spec, roadmap H6,
KPI replay, `spec-turnlog-serialize` e architettura aggiornati.

## 6. Fuori scope (YAGNI, dichiarato)

Wiring in `ARTTurnManager` e switch del turn loop su hex · voci di **combattimento** su hex (abilità/targeting
esagonali non esistono ancora) · LOS/copertura esagonale · rendering delle rotte · bot su hex · knockback/dash ·
hazard di superficie.

## 7. Rischi

- **Divergenza** tra resolver quadrato e hex (D7): mitigata replicando gli scenari nei test; si chiude allo switch.
- **Snapshot con puntatore alla mappa** (D2/D3): mitigato da `IsSnapshotStale` + vita limitata alla fase; se in
  futuro la mappa dovesse mutare a metà turno (hazard dinamici, H8) servirà una copia vera dei dati di cella.
- Il costo dell'A* su mappe grandi non è misurato in questo slice (KPI di H9).

## 8. Riferimenti

- [`hex-map-roadmap.md`](../roadmap/hex-map-roadmap.md) — milestone H6 · [`adr-0002-griglia-esagonale.md`](../decisions/adr-0002-griglia-esagonale.md)
- Codice esistente riusato: `Pathfinding/RTHexPathLibrary` (A*, `GraphNeighbors`), `Map/RTHexMapAsset`
  (`ComputeHash`, `Revision`, `FindCell`), `Turn/RTTurnLog.h` (`ERTMoveOutcome`).
- Riferimento di semantica per i microstep: `Turn/RTMovementResolver.cpp` (versione quadrata).
