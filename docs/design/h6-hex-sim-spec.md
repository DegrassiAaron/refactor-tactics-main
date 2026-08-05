# Spec H6.1–H6.2 — Simulazione su griglia esagonale (snapshot, budget, collisioni)

> **Stato**: **Implementata** (TDD RED→GREEN; suite **140/140**, build Editor Succeeded) · **Data**: 2026-08-05 · **Branch**: `feat/hex-sim`
> **Fonte**: milestone **H6** di [`hex-map-roadmap.md`](hex-map-roadmap.md) («Integrazione simulatore»).
> Non tocca il turn loop quadrato (base di rollback, [ADR-0002](adr-0002-griglia-esagonale.md)).

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

## 6. Fuori scope (YAGNI, dichiarato)

Wiring in `ARTTurnManager` e switch del turn loop su hex · TurnLog con celle hex (`FRTTurnLogEntry` resta su
`FRTGridCoord`) · LOS/copertura esagonale · abilità/targeting su hex · rendering delle rotte · bot su hex ·
knockback/dash · hazard di superficie.

## 7. Rischi

- **Divergenza** tra resolver quadrato e hex (D7): mitigata replicando gli scenari nei test; si chiude allo switch.
- **Snapshot con puntatore alla mappa** (D2/D3): mitigato da `IsSnapshotStale` + vita limitata alla fase; se in
  futuro la mappa dovesse mutare a metà turno (hazard dinamici, H8) servirà una copia vera dei dati di cella.
- Il costo dell'A* su mappe grandi non è misurato in questo slice (KPI di H9).

## 8. Riferimenti

- [`hex-map-roadmap.md`](hex-map-roadmap.md) — milestone H6 · [`adr-0002-griglia-esagonale.md`](adr-0002-griglia-esagonale.md)
- Codice esistente riusato: `Pathfinding/RTHexPathLibrary` (A*, `GraphNeighbors`), `Map/RTHexMapAsset`
  (`ComputeHash`, `Revision`, `FindCell`), `Turn/RTTurnLog.h` (`ERTMoveOutcome`).
- Riferimento di semantica per i microstep: `Turn/RTMovementResolver.cpp` (versione quadrata).
