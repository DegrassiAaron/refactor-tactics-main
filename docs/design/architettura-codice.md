# RefactorTactics — Architettura del codice

Mappa del modulo C++ `Source/RefactorTactics/` allo stato attuale (**MVP + incrementi post-MVP**: path finding,
terreno v1, movimento v2). Per le decisioni vincolanti vedi [piano canonico](piano-canonico-mvp.md); per lo
stato per checkpoint vedi [roadmap](roadmap-checkpoint.md).

## Principi applicati

1. **Logica pura separata dagli Actor.** Le regole (griglia, path finding, movimento, danno, terreno, bot)
   vivono in `UBlueprintFunctionLibrary` con funzioni statiche pure → **testabili senza mondo/Actor**. Gli Actor
   (`ARTUnit`, `ARTTurnManager`, …) orchestrano ma non contengono la matematica.
2. **Griglia logica vs rendering.** La posizione autorevole è `FRTGridCoord`; il `FVector` serve solo a
   `ARTGridActor`/`ARTUnit` per posizionare le mesh.
3. **Resolver "raccogli poi applica".** Movimento e attacchi calcolano l'esito su uno **snapshot** dello stato
   iniziale e lo applicano insieme → l'**ordine dell'input non cambia il risultato** (coperto da test). L'ordine
   deterministico degli effetti simultanei è normato in [piano §5.1](piano-canonico-mvp.md) (APNAP, `FR-RESOLVE-*`).
4. **Autorità nel `ARTTurnManager`.** Il controller propone piani (preview); il turn manager valida
   (range, path a costo, bersaglio nemico/vivo, LOS) e risolve. Predisposto al futuro server-authority.
5. **Presentazione isolata.** Camera, HUD, colore team, viz del percorso sono presentazione: non decidono l'esito.

## Mappa per cartella

| Cartella / file | Tipo | Responsabilità |
|---|---|---|
| `RefactorTactics.{h,cpp}` | Modulo | Primary game module + categoria log `LogRT` |
| `Core/RTTypes.h` | `USTRUCT` | `FRTGridCoord{X,Y}` (posizione logica; `Layer` riservato al multilivello) |
| `Core/RTGameplayTags.h` | Tag nativi | `Status.Root` · `Status.Slow` · `Status.Reveal` |
| `Grid/RTGridLibrary` | Function Library (pure) | Conversioni `CellToWorld`/`WorldToCell`, `ManhattanDistance`, `IsWithinRange`, `HasLineOfSight`, forme `CellsInRadius/Line/Cone`; path finding `ReachableCells`/`FindPath` (BFS) e **pesato** `ReachableCellsByCost`/`FindPathByCost` (Dijkstra), `PathCost`, `BuildCompositePath` (waypoint) |
| `Grid/RTGridActor` | `AActor` | Griglia visuale 10×10 (Instanced Static Mesh); `BlockedCells`; rendering celle-terreno colorate |
| `Map/RTCellId.h`, `RTHexCellData.h` | `USTRUCT`/enum | `FRTCellId` (assiale q/r + Layer), `ERTHexDirection`, `FRTHexCellData` (superficie/costo/blocchi), `FRTHexEdge` (transizioni) |
| `Map/RTHexLibrary` | Function Library (pure) | Matematica esagonale pointy-top: vicini, `HexDistance`, `HexArea`, `AxialToWorld`/`WorldToAxial`/`WorldToLayer`, `StableLess`, **`HexLine`** (lerp intero + arrotondamento cubico) e **`HexCone`** (ventaglio 120° = due settori a 60°) |
| `Map/RTHexVisionLibrary` | Function Library (pure) | `HasLineOfSight` sulla mappa esagonale: linea planare sul layer del tiratore, estremi mai bloccanti, celle assenti non bloccanti ([spec](h6-4-hex-vision-spec.md)) |
| `Map/RTHexMapAsset`, `RTHexMapActor` | `UPrimaryDataAsset` / `AActor` | Asset autorevole della mappa (celle in ordine stabile, transizioni, `ComputeHash`, `Revision`, `ValidateMap`, primitive di stroke, `FloodRegion`) e rendering ISM. Le istanze sono una **vista derivata**: `OnConstruction` (apertura livello, spostamento, spawn) e `PostEditChangeProperty` la ricostruiscono dall'asset — non va tenuta allineata a mano. **Undo/redo**: l'actor non fa parte della transazione (a cambiare è l'asset), quindi `URTHexMapAsset::PostEditUndo` **invalida la cache** `Id→indice` e notifica via `OnMapChanged`, a cui l'actor si iscrive |
| `Pathfinding/RTHexPathLibrary` | Function Library (pure) | Grafo tattico: `GraphNeighbors` (vicini + archi espliciti), A* deterministico `FindPath` / `FindPathAvoiding` (ostacoli dinamici) |
| `Selection/RTSelectable.h` | `UINTERFACE` | `IRTSelectable` (`OnSelected`/`OnDeselected`) |
| `Camera/RTCameraPawn` | `APawn` | Camera tattica: SpringArm inclinato, pan, zoom |
| `Player/RTPlayerController` | `APlayerController` | Enhanced Input **in C++**; selezione; pianificazione abilità (1/2/3) + bersaglio + movimento (cella singola o **path a waypoint**); lock-in |
| `Unit/RTUnit` | `AActor` + `IRTSelectable` | Archetipi (Ranger/Guardian); team, cella, HP/scudo, energia/ultimate; lista abilità (`URTAbilityData`); piani (`PlannedCell`/`PlannedPath`/`PlannedWaypoints`/`PlannedAttackTarget`/`PlannedAbilityIndex`); status (tag→turni); kiting; colore team (`M_Unit`); eliminazione |
| `Ability/RTAbilityData` | `PrimaryDataAsset` | `URTAbilityData` (range/power/`ERTAbilityShape` Single·Area·Line·Cone/area/status/cooldown/costo energia/`bSelfTarget` Prep/`bIgnites`) |
| `Terrain/RTTerrainTypes.h` | `USTRUCT`/enum | `FRTTerrainProps` (costo extra/blocco movimento/blocco vista/…) |
| `Terrain/RTTerrainData` | `PrimaryDataAsset` | `URTTerrainData` — 5 tipi: Fango (costo), Cespuglio (blocca vista), Altura (+danno), Lava (hazard), Erba→Fuoco (dinamico) |
| `Terrain/RTTerrainLibrary` | Function Library (pure) | `CellMoveCost`, `BlocksMovement`, `BlocksVision` |
| `Turn/RTTurnRules` | Function Library (pure) | `ERTMatchPhase` + `NextPhase`; `ERTMatchOutcome` + `EvaluateOutcome` |
| `Turn/RTMovementResolver` | Function Library | `FRTMoveRequest`/`ResolveMoves` (conflitti); `FRTPathResult`/`ResolvePaths` (microstep sincroni, cross-damage, **ordine-indipendente**) — griglia **quadrata** |
| `Turn/RTHexSim.h` | `USTRUCT`/struct | `FRTHexSimUnit` (UnitId/cella/vivo/`MoveBudget`), `FRTHexReachableCell`, `FRTHexMoveResult`; `FRTHexSnapshot` (struct puro: mappa referenziata + hash/revisione, occupazione copiata; vive solo dentro una fase) |
| `Turn/RTHexSimLibrary` | Function Library (pure) | Simulazione su griglia **esagonale** (strato parallelo al quadrato, [spec](h6-hex-sim-spec.md)): `MakeSnapshot`/`ValidateSnapshot`/`IsSnapshotStale`, `IsCellFree`, `ReachableCells` (Dijkstra entro `MoveBudget`), `FindPathForUnit` (A* che evita le unità), `ResolveHexPaths` (microstep simultanei), `ToLogCoord`/`BuildMoveLog` (voci di TurnLog dagli esiti → replay) |
| `Turn/RTTurnLogLibrary` | Function Library (pure) | Ordine totale/hash permutazione-invariante del TurnLog; serializzazione binaria versionata con **topologia** (`ERTLogTopology`) nei flags dell'header, checksum FNV e I/O su file, tutto fail-closed |
| `Turn/RTTurnManager` | `AActor` | Orchestratore: fasi, timer 30s, `PlanBots`, `ResolvePrep` (scudo/self-buff), `ResolveCombat` (Blast), `ResolveMovement` (Move + hazard fine turno), combat log, `LastMoveRoutes` (viz post-lock), esito |
| `Combat/RTCombatLibrary` | Function Library (pure) | `ApplyDamage` (scudo poi HP), `GainEnergy`, `IsUltimateReady`, `EffectiveMoveRange` (Root/Slow), `IsAbilityUsable`, `IsIntentVisibleTo` (**invariante #6**), `EffectiveAttackPower` (Altura) |
| `Combat/RTCombatResolver` | Function Library | `FRTUnitCombatState`, `FRTAttack`; `ResolveAttacks` (raccogli-poi-applica, focus-fire) |
| `Bot/RTBotLibrary` | Function Library | Avvicinamento (`StepToward`) + scelta bersaglio/abilità; **path/cost/hazard-aware**; kiting del Ranger — griglia **quadrata** |
| `Bot/RTHexBotLibrary` | Function Library (pure) | Bot su griglia **esagonale** ([spec](h6-5-hex-bot-spec.md)): stessa politica del quadrato (`ScorePlan`/`ChooseBestPlan`) con distanza esagonale e LOS d'asset; `BuildCandidates` deriva le mosse da `ReachableCells` (budget/blocchi/occupanti/archi già applicati), `PlanUnit` sceglie |
| `UI/RTHUD` | `AHUD` | Barre HP/scudo, energia, barra abilità, timer/fase, combat log, anteprima piani (ciano/reveal), viz percorso (waypoint + traccia risolta post-lock) |
| `RTGameMode` | `AGameModeBase` | Allestisce il demo (griglia, luce, 2v2, terreno, turn manager); imposta pawn/controller/HUD; marca team 1 come bot |
| `Tests/` | Automation | Griglia, resolver, fasi, combat, bot, terreno, hex (mappa/path/layer/**sim**), TurnLog (hash/serializzazione/topologia), visione hex, bot hex, vista mappa — **172 test** in 25 file (conteggio misurato 2026-08-05). Attenzione: nella *unity build* i file di test condividono la translation unit → gli helper nei namespace anonimi devono avere **nomi distinti tra file** |

## Flusso di un turno

```
Inizio pianificazione (ARTTurnManager::StartPlanningTimer, 30s)
  → PlanBots(): ogni unità bot sceglie bersaglio/abilità o si avvicina (path/cost/hazard-aware, kiting)
  → il giocatore pianifica (click): abilità (1/2/3) + bersaglio + movimento (cella singola o path a waypoint)
      · preview con validazione (range, path a costo, LOS)
  → timer 30s

Lock-in (Spazio) oppure timeout → LockInAndResolve()
  → NextPhase attraversa le fasi:
      · Prep  → ResolvePrep():    abilità di supporto (bSelfTarget) → scudo / self-buff
      · Dash  → (riservata: mobilità rapida, nessuna abilità la usa ancora)
      · Blast → ResolveCombat():  raccoglie gli attacchi validi (nemico/vivo/in portata/LOS, posizione ATTUALE)
                ResolveAttacks (snapshot, danni sommati per bersaglio, EffectiveAttackPower/Altura)
                → ApplyCombatState → status/energia → eliminazione a HP ≤ 0
      · Move  → ResolveMovement(): path compositi → ResolvePaths (microstep, cross-damage terreno)
                → PlaceOnCell → hazard di fine turno (Lava) + terreno dinamico (Erba→Fuoco)
  → TickCooldowns / TickStatuses ; EvaluateOutcome(vivi team0, vivi team1):
      · InProgress → nuovo turno (StartPlanningTimer)
      · altrimenti → MatchEnded (turni fermi, HUD "PARTITA FINITA")
```

**Ordine delle fasi**: Prep → Dash → **Blast prima di Move** → si agisce/spara dalla posizione attuale, poi ci
si sposta (modello *Atlas Reactor*).

## Come si estende (per le prossime feature)

- **Path finding multilivello** (PF.4, north-star): `FRTGridCoord.Layer` + grafo esplicito con archi — vedi
  [`spec-pathfinding-pf3-pf4.md`](spec-pathfinding-pf3-pf4.md).
- **Sistema di reazioni / ordinamento ricco** (north-star): l'ordine deterministico degli effetti simultanei è
  già normato in [piano §5.1](piano-canonico-mvp.md) (`FR-RESOLVE-*`); il modello completo (stack LIFO, reveal,
  categorie di velocità) è in [`spec-sequenza-turno.md`](spec-sequenza-turno.md).
- **Multiplayer** (post-MVP): il `RTTurnManager` è già il punto di autorità; i piani diventano RPC server-side
  con replica filtrata per squadra (privacy dell'intento, **invariante #6**).

Ogni nuova regola nasce come **funzione pura con test** prima del wiring negli Actor.
