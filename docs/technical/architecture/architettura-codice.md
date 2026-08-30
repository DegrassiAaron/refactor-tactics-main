# RefactorTactics — Architettura del codice

Mappa del modulo C++ `Source/RefactorTactics/` allo stato attuale. Per le decisioni vincolanti vedi
[piano canonico](../../product/piano-canonico-mvp.md); per lo stato per checkpoint vedi [roadmap](../../roadmap/roadmap-checkpoint.md).

> `CURRENT` · **Owner** della mappa del codice · **Riallineata al codice il 2026-08-08**.
>
> Il substrato **quadrato è stato rimosso al CP 7.2**: non esistono più `Grid/`, `Bot/RTBotLibrary`,
> `Turn/RTMovementResolver` né `FRTGridCoord`. La posizione autorevole è `FRTCellId` (assiale) e la mappa è un
> asset (`URTHexMapAsset`). Il punto di ritorno storico è il tag `pre-hex-only`.
>
> *(La versione precedente diceva che anche `Terrain/` era sparito: **esiste**, ricostruito per E8. Ed elencava
> «329 test in 49 file», un numero di due settimane prima — vedi la riga `Tests/`.)*

## Principi applicati

1. **Logica pura separata dagli Actor.** Le regole (griglia, path finding, movimento, danno, terreno, bot)
   vivono in `UBlueprintFunctionLibrary` con funzioni statiche pure → **testabili senza mondo/Actor**. Gli Actor
   (`ARTUnit`, `ARTTurnManager`, …) orchestrano ma non contengono la matematica.
2. **Cella logica vs rendering.** La posizione autorevole è `FRTCellId` (assiale q/r + Layer); il `FVector`
   serve solo a `ARTHexMapActor`/`ARTUnit` per posizionare le mesh.
3. **Resolver "raccogli poi applica".** Movimento e attacchi calcolano l'esito su uno **snapshot** dello stato
   iniziale e lo applicano insieme → l'**ordine dell'input non cambia il risultato** (coperto da test). L'ordine
   deterministico degli effetti simultanei è normato in [piano §5.1](../../product/piano-canonico-mvp.md) (APNAP, `FR-RESOLVE-*`).
4. **Autorità nel `ARTTurnManager`.** Il controller propone piani (preview); il turn manager valida
   (range, path a costo, bersaglio nemico/vivo, LOS) e risolve. Predisposto al futuro server-authority.
5. **Presentazione isolata.** Camera, HUD, colore team, viz del percorso sono presentazione: non decidono l'esito.

## Mappa per cartella

| Cartella / file | Tipo | Responsabilità |
|---|---|---|
| `RefactorTactics.{h,cpp}` | Modulo | Primary game module + categoria log `LogRT` |
| `Core/RTTypes.h` | `USTRUCT` | `FRTTraversalEdge` (arco di traversata esplicito fra due celle) |
| `Core/RTGameplayTags.h` | Tag nativi | `Status.Root` · `Status.Slow` · `Status.Reveal` |
| `Map/RTCellId.h`, `RTHexCellData.h` | `USTRUCT`/enum | `FRTCellId` (assiale q/r + Layer), `ERTHexDirection`, `FRTHexCellData` (superficie/costo/blocchi), `FRTHexEdge` (transizioni) |
| `Map/RTHexLibrary` | Function Library (pure) | Matematica esagonale pointy-top: vicini, `HexDistance`, `HexArea`, `AxialToWorld`/`WorldToAxial`/`WorldToLayer`, `StableLess`, **`HexLine`** (lerp intero + arrotondamento cubico) e **`HexCone`** (ventaglio 120° = due settori a 60°) |
| `Map/RTHexVisionLibrary` | Function Library (pure) | `HasLineOfSight` sulla mappa esagonale: linea planare sul layer del tiratore, estremi mai bloccanti, celle assenti non bloccanti ([spec](../systems/h6-4-hex-vision-spec.md)) |
| `Map/RTHexMapAsset`, `RTHexMapActor` | `UPrimaryDataAsset` / `AActor` | Asset autorevole della mappa (celle in ordine stabile, transizioni, `ComputeHash`, `Revision`, `ValidateMap`, primitive di stroke, `FloodRegion`) e rendering ISM. Le istanze sono una **vista derivata**: `OnConstruction` (apertura livello, spostamento, spawn) e `PostEditChangeProperty` la ricostruiscono dall'asset — non va tenuta allineata a mano. **Undo/redo**: l'actor non fa parte della transazione (a cambiare è l'asset), quindi `URTHexMapAsset::PostEditUndo` **invalida la cache** `Id→indice` e notifica via `OnMapChanged`, a cui l'actor si iscrive |
| `Pathfinding/RTHexPathLibrary` | Function Library (pure) | Grafo tattico: `GraphNeighbors` (vicini + archi espliciti), A* deterministico `FindPath` / `FindPathAvoiding` (ostacoli dinamici) |
| `Selection/RTSelectable.h` | `UINTERFACE` | `IRTSelectable` (`OnSelected`/`OnDeselected`) |
| `Camera/RTCameraPawn` | `APawn` | Camera tattica: SpringArm inclinato, pan, zoom |
| `Player/RTPlayerController` | `APlayerController` | Enhanced Input **in C++**; selezione; pianificazione abilità (1/2/3) + bersaglio + movimento (cella singola o **path a waypoint**); lock-in |
| `Player/RTPlayerState` | `APlayerState` | L'identita' di squadra del giocatore, e la sua unica porta di lettura ([D-285](../../decisions/RT_PDR_00_Decision_Log.md)). `TeamId` **non e' una `UPROPERTY` editabile**: e' stato di runtime scritto da `AssignTeam(int32)`. `TeamIdOf(const APlayerController*)`, statica e pura, **assorbe** l'ex `ARTHUD::ViewerTeamIdOf` e resta l'unico lettore del progetto; ripiega su `0` per tre cause distinte — controller nullo, nessun `PlayerState`, `PlayerState` di classe sbagliata (il caso che produce `InitializeActorsForPlay` nei mondi di prova) |
| `Unit/RTUnit` | `AActor` + `IRTSelectable` | Team, cella (`FRTCellId`), HP/scudo, energia/ultimate; lista abilità `TArray<URTActionData*>`; piani (`PlannedCell`/`PlannedPath`/`PlannedWaypoints`/`PlannedAttackTarget`/`PlannedAbilityIndex`/`PlannedDashAbility`/`PlannedReactionAbility`); status (tag→turni); kiting; colore team (`M_Unit`); eliminazione. `ERTArchetype{Ranger,Guardian}` sopravvive come **configurazione di test** (`ConfigureAsArchetype`): il roster di gioco viene dal catalogo eroi |
| `Ability/RTActionData`, `RTActionDef.h` | `UPrimaryDataAsset` + `USTRUCT` | `URTActionData` e `FRTActionDef`: `ActionId`, `ResolutionPhase`, `Priority` intera, `RangeCells`, `CostMP`, `CooldownTurns`, `Fallback`, `Slot`, `MovementStyle`, `Effects`, `ReactionTrigger`, `bFriendlyFire` ([catalogo](../../balance/RT_ActionCatalog_v0.1.md)) |
| `Ability/RTHeroData`, `RTHeroCatalogLibrary` | `UPrimaryDataAsset` + Function Library | `URTHeroData` (statistiche, kit, varianti) e il catalogo dei 4 eroi ([catalogo](../../balance/RT_HeroCatalog_v0.1.md)) |
| `Ability/RTEquipmentData`, `RTCatalogLibrary` | `UPrimaryDataAsset` + Function Library | Equipaggiamento e **validator** dei cataloghi (ID unici, niente float nei campi interi, mappatura di fase totale) |
| `Turn/RTTurnRules` | Function Library (pure) | `ERTMatchPhase` + `NextPhase`; `ERTMatchOutcome` + `EvaluateOutcome`; **fine partita a tre vie** (CP 10.3): `FRTMatchState`/`FRTMatchResult` + `EvaluateMatchEnd` (eliminazione → obiettivo → `RoundLimit`, parità = pareggio dichiarato) e i testi `DescribeOutcome`/`DescribeEndReason`, unica fonte per log e HUD |
| `Turn/RTMatchFormatData` | `UPrimaryDataAsset` + `USTRUCT` | `URTMatchFormatData` (formato di partita: `FormatId`, `FormatVersion`, `RoundLimit`, `ExpectedRounds`, `ScoreToWin`) e `FRTMatchRules`, le regole **risolte** che il `TurnManager` legge — una sola verità per valore ([issue #185](../../gameplay/spec-durata-partita-e-scala-mappe.md)) |
| `Turn/RTMatchFormatLibrary` | Function Library (pure) | `ValidateFormat`/`ValidateRules` (elenco errori, forma di `ValidateMap`), `ResolveRules` **fail-closed** (mai un ripiego: quello è politica di `ARTGameMode`), `MakeFallbackRules` + `FallbackFormatId` |
| `Turn/RTHexSim.h` | `USTRUCT`/struct | `FRTHexSimUnit` (UnitId/cella/vivo/`MoveBudget`), `FRTHexReachableCell`, `FRTHexMoveResult`; `FRTHexSnapshot` (struct puro: mappa referenziata + hash/revisione, occupazione copiata; vive solo dentro una fase) |
| `Turn/RTHexSimLibrary` | Function Library (pure) | Simulazione su griglia esagonale — **l'unica**, non più uno strato parallelo ([spec `AS-BUILT`](../systems/h6-hex-sim-spec.md)): `MakeSnapshot`/`ValidateSnapshot`/`IsSnapshotStale`, `IsCellFree`, `ReachableCells` (Dijkstra entro `MoveBudget`), `FindPathForUnit` (A* che evita le unità), `ResolveHexPaths` (microstep simultanei), `ToLogCoord`/`BuildMoveLog` (voci di TurnLog dagli esiti → replay) |
| `Turn/RTTurnLogLibrary` | Function Library (pure) | Ordine totale/hash permutazione-invariante del TurnLog; serializzazione binaria versionata con **topologia** (`ERTLogTopology`) nei flags dell'header e **identità del formato** (`FormatId`, versione 4) subito dopo, checksum FNV e I/O su file, tutto fail-closed; `CompareSerializedTraces` confronta il **contesto prima del contenuto** (`ERTTraceComparison`: *formato diverso* non è *divergenza*). Il `FormatId` **non entra nell'hash**: nessun hash golden viene invalidato |
| `Turn/RTTurnManager` | `AActor` | Orchestratore: fasi, timer 30s, `PlanBots`, `ResolvePrep` (scudo/self-buff), `ResolveCombat` (Blast), `ResolveMovement` (Move + hazard fine turno), combat log, `LastMoveRoutes` (viz post-lock), esito |
| `Combat/RTCombatLibrary` | Function Library (pure) | `ApplyDamage` (scudo poi HP), `GainEnergy`, `IsUltimateReady`, `EffectiveMoveRange` (Root/Slow), `IsAbilityUsable`, `IsIntentVisibleTo` (**invariante #6**), `EffectiveAttackPower(Base, OccupantDamageBonus)` — bonus **generico** di cella, **non** «altura»: ogni call site runtime passa `0` ([D-024](../../decisions/RT_PDR_00_Decision_Log.md)) |
| `Combat/RTHexCombatLibrary`, `RTOffensiveActionLibrary` | Function Library (pure) | Risoluzione del combattimento su hex: forme di targeting, LOS, niente fuoco amico; azioni offensive del catalogo |
| `Map/RTHexCoverLibrary` | Function Library (pure) | Copertura **di bordo** (E9): `EdgeDirection`, `CoverBetween`, `BlocksTraversal` (consultata **sia** dal path **sia** dalla LOS: una sola risposta a «questo bordo è chiuso»), `ApplyStructureDamage` → `CoverDamaged`/`CoverDestroyed` |
| `Terrain/RTTerrainData.h`, `RTTerrainLibrary` | `USTRUCT` + Function Library (pure) | Superfici, stati temporanei, propagazione elettrica, fuoco/acqua (E8) |
| `Turn/RTReactionLibrary` | Function Library (pure) | Reazioni componibili e `Intercept` (E5). È il caso `AllowedResponses ≤ 1` del modello unificato di [ADR-0004](../../decisions/adr-0004-finestre-di-reazione.md), non un meccanismo separato |
| `Turn/RTActionQueueLibrary`, `RTActionEffectLibrary`, `RTActionFallbackLibrary` | Function Library (pure) | Motore azioni (E4): ordine per priorità intera, effetti, fallback |
| `Turn/RTMovementActionLibrary` | Function Library (pure) | Profili di movimento: `IsLinear`, stili `Budget`/`LinearDash`/`LinearCharge`/`LinearLeap` |
| `Turn/RTMatchSetupLibrary` | Function Library (pure) | Allestimento della partita |
| `Turn/RTPacingLibrary`, `RTPacing.h` | Function Library (pure) | Pacing del turno misurato; console `rt.Debug.Pacing` |
| `Turn/RTPlaybackLibrary` | Function Library (pure) | Presentazione della risoluzione: **riproduce**, non decide (invariante #1) |
| `Turn/RTIntentPrivacyLibrary` | Function Library (pure) | `FilterForTeam` → `FRTIntentView` (**invariante #6**). Con [D-021](../../decisions/RT_PDR_00_Decision_Log.md) la privacy comprende anche il **tempo**: è qui che andrà il confine per le finestre di E14 |
| `ScenarioHarness/` | Function Library + `USTRUCT` | **Scenario Test Harness**: `URTScenarioLoader` (JSON versionato da `Scenarios/`), `URTScenarioRunner` (esegue nel **percorso di gioco reale**), `URTTestReportWriter` (`result.json`), `FRTTestScenario`/`FRTTestResult`, console `rt.Test.*` in `RTTestConsole.cpp`. **Nessun Actor di test** ([spec](../tooling/test-automatico-unreal.md)) |
| `Combat/RTCombatResolver` | Function Library | `FRTUnitCombatState`, `FRTAttack`; `ResolveAttacks` (raccogli-poi-applica, focus-fire) |
| `Bot/RTHexBotLibrary` | Function Library (pure) | Bot su griglia **esagonale** ([spec](../systems/h6-5-hex-bot-spec.md)): stessa politica del quadrato (`ScorePlan`/`ChooseBestPlan`) con distanza esagonale e LOS d'asset; `BuildCandidates` deriva le mosse da `ReachableCells` (budget/blocchi/occupanti/archi già applicati), `PlanUnit` sceglie |
| `UI/RTHUD` | `AHUD` | Barre HP/scudo, energia, barra abilità, timer/fase, combat log, anteprima piani (ciano/reveal), viz percorso (waypoint + traccia risolta post-lock) |
| `Map/RTHexOverlayConsole.cpp` | Console command | `rt.Debug.DrawCells`: overlay di leggibilità della mappa (superficie, blocca-movimento in rosso, blocca-vista in giallo). Strumento di sviluppo, sola lettura — in partita le celle sono cilindri identici e una mappa che non comunica le proprie regole rende il playtest cieco |
| Anteprima di pianificazione | in `ARTHexMapActor` | `SetPreviewPath` (percorso, ciano) · `SetPreviewReachableCells` (dove posso arrivare, verde tenue) · `SetPreviewHitCells` (zona colpita, **rosso**; alleati nell'area in **arancione**). Le celle arrivano **già calcolate** da `ReachableCells`/`HexHitCells`: l'anteprima non ricalcola nulla, altrimenti potrebbe divergere dall'esito (invariante #1). Alimentata da `RefreshPlanningPreview` nel controller; si spegne al lock-in |
| `RTGameMode` | `AGameModeBase` | **Composition root**, non allestitore: imposta pawn/controller/HUD/**`PlayerStateClass`**, decide *scenario vs partita*, risolve le tre precedenze (`ResolveMapSource`/`ResolveAutobattle`/`ResolveMatchPlanningSeconds`), chiama il bootstrapper, collega frontend e presentation, avvia il replay, **assegna i posti** (`AssignSeats()`, idempotente, derivati dal formato — `SeatsPerTeam = UnitsPerTeam / UnitsPerPlayer` — chiamata sia da `OnPostLogin` sia da `SetupHexMatch` perche' il loro ordine non e' garantito, [D-285](../../decisions/RT_PDR_00_Decision_Log.md)). Il *come* nasce una partita sta in `Match/` |
| `Match/RTMatchBootstrapper` | classe C++ pura (statica) | **COME nasce una partita**: sorgente mappa, formato e validazione contro la mappa, regole e ritmo al `TurnManager`, celle di partenza, formazioni, roster, spawn ed equipaggiamento. Riceve valori gia' risolti (`FRTMatchBootstrapConfig`) — **non legge console variable ne' riga di comando**, ed e' cio' che rende l'allestimento verificabile senza sporcare lo stato globale del processo |
| `Frontend/RTMatchFrontendBridge` | struct di funzioni statiche | La **politica** del confine col frontend dal lato partita: consuma la richiesta invece di leggerla, dichiara l'annuncio senza richiesta pendente, porta il verdetto al Result senza ricalcolarlo. Il *cablaggio* — delegate dinamici e `OpenLevelByName`, che e' il seam dei test — resta in `RTGameMode` |
| `Perception/RTKnowledgeVeilPresenter` | `UObject` | Stende il velo sulla board per la squadra del **proprio client**: il viewer e' `ARTPlayerState::TeamIdOf(Cast<APlayerController>(GetOuter()))`, non piu' un letterale sul controller. Presentation-only — non decide visibilita', non calcola conoscenza, non tocca stato autorevole. ⚠️ Legge ancora una conoscenza canonica **locale**: il canale sanificato per squadra e' [debito di rete dichiarato](../../decisions/RT_PDR_00_Decision_Log.md) |
| `ScenarioHarness/RTScenarioCoordinator` | classe C++ pura | Ciclo di vita a runtime di uno scenario: caricamento, sessione, avanzamento un passo per frame, referto. Il GameMode decide **se** si gioca uno scenario; questo sa **come** si esegue |
| `Tests/` | Automation | Resolver, fasi, combat, bot, terreno, hex (mappa/path/layer/**sim**), TurnLog (hash/serializzazione/topologia), visione, coperture, catalogo, azioni, reazioni, eroi, match, scenari. **Il numero si misura, non si cita** — ultima misura **419 unici in 64 file** al commit `3335e36` (2026-08-08); ripartizione per area in [`../../README.md`](../../README.md). Comando: `grep -rhoE '"RefactorTactics\.[A-Za-z0-9_.]+"' Source/RefactorTactics/Tests/*.cpp \| sort -u \| wc -l`. Attenzione: nella *unity build* i file di test condividono la translation unit → gli helper nei namespace anonimi devono avere **nomi distinti fra file** |

## La pipeline, dall'alto

```text
Input giocatore  ·  Bot  ·  Scenario Harness
                    ↓            (tre sorgenti di intento, UN solo percorso)
            Planned Intent
                    ↓
              Turn Manager                    ← unico punto di autorità (invariante #5)
                    ↓
           Segment Snapshot                   ← mappa + hash + revisione + occupazione
                    ↓
        Action Queue → Resolver               ← raccogli poi applica, ordine per priorità intera
                    ↓
         Decision Boundary?  ──sì──┐          ← E14: finestra di reazione, fra i segmenti
                    │ no           │
                    │         risposta autorizzata
                    │              │
                    │←─────────────┘          ← nuovo segmento, nuovo snapshot
                    ↓
                 TurnLog                      ← hash permutazione-invariante, serializzazione versionata
                    ↓
              Presentazione                   ← riproduce, non decide
```

Il punto che questa forma rende visibile: **lo Scenario Harness entra dalla stessa porta del giocatore**. Non
esiste un percorso di test che salta il resolver — se esistesse, i test misurerebbero un gioco diverso da
quello che si gioca.

Attraversano la pipeline, senza essere fasi: la **mappa** (`URTHexMapAsset`), il **path**
(`URTHexPathLibrary`), la **LOS** (`URTHexVisionLibrary`), le **coperture** (`URTHexCoverLibrary`),
l'**ambiente** (`URTTerrainLibrary`) e le **reazioni** (`URTReactionLibrary`).

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

**Ordine delle fasi**: `Prep → Dash → Blast → Move → Cleanup`. Il **Move normale è l'ultima fase volontaria**:
si agisce e si spara dalla posizione attuale, poi ci si sposta (modello *Atlas Reactor*). `Dash`, `Charge` e
`Leap` sono mobilità **speciali** della fase `Dash`; `Sneak`, `Move` e `Sprint` sono **profili** del movimento
normale e risolvono nel `Move` ([D-015](../../decisions/RT_PDR_00_Decision_Log.md)).

> ⚠️ **Divergenza documento/codice, misurata il 2026-08-08.** Nel codice `Action.Sprint` è in
> `ERTResolutionPhase::FastMovement` e consuma **movimento più azione principale**, mentre D-015 lo vuole
> profilo del `Move` nel **solo** slot movimento. La decisione è presa, la migrazione no: `Action.Sprint`
> compare in 15 file fra codice e test, e cancellarlo in una passata documentale romperebbe test e replay.
> Tracciato in [`../../DOC_CONFLICT_MATRIX.md`](../../DOC_CONFLICT_MATRIX.md) riga 41.

## Confine con ciò che non esiste ancora

| Area | Stato | Dove vive il confine |
|---|---|---|
| **E13 — Team Knowledge** | non implementata | Oggi la vista è una statistica a catalogo che non decide nulla. LOS, rilevamento e conoscenza di squadra andranno separati; il rumore è un **secondo canale**, propagato con interi sul grafo — mai `SphereOverlap` |
| **E14 — Decision Window** | non implementata | `URTReactionLibrary` è già il caso `AllowedResponses ≤ 1`. Ciò che manca è il **boundary fra segmenti**, non un secondo motore di reazioni. Vincolo di privacy: [D-021](../../decisions/RT_PDR_00_Decision_Log.md) |
| **E16 — Facing** | non implementata | Il facing esiste come **presentazione** (`URTPlaybackLibrary::DirectionYaw`). Diventerà stato di gioco con più valori per round ([D-020](../../decisions/RT_PDR_00_Decision_Log.md)): snapshot e TurnLog dovranno dire *quale* facing ha usato ciascun consumatore |
| **Rete** | fuori dalla v0.1 | `ARTTurnManager` è già l'unico punto di autorità; i piani diventeranno RPC server-side con replica filtrata per squadra |
| **GAS** | fuori dalla v0.1 | Le abilità sono dati (`URTActionData`), non `GameplayAbility`. Divergenza dal PDR **dichiarata**, non dimenticata |
| **StateTree · Behavior Tree · EQS · Learning Agents · Mass AI** | perimetro deciso, nessuno adottato | [D-095](../../decisions/RT_PDR_00_Decision_Log.md). Il core dell'AI competitiva è un **utility planner custom**, perché deve produrre un *breakdown* di punteggio e un albero di comportamento non lo produce. StateTree può orchestrare le macro-fasi del turno, mai calcolare il piano migliore; Behavior Tree resta per prototipi non competitivi; EQS è laboratorio, e il runtime interroga la mappa tattica discreta; Learning Agents solo offline; Mass AI fuori scope. **Non riapre NavMesh**, chiusa da [ADR-0002](../../decisions/adr-0002-griglia-esagonale.md) |

> **Perché quella riga esiste.** Fino al 2026-08-11 il progetto aveva deciso cosa il bot *fa* e mai con quale
> strumento di Unreal: `StateTree`, `EQS` e `Learning Agents` avevano **zero occorrenze** in `docs/` e
> `Source/`. È la lacuna in cui un framework entra per abitudine invece che per scelta — sono gli strumenti
> che l'editor offre per primi, e chi apre il progetto per implementare il bot li trova prima di trovare il
> Decision Log.

## Come si estende

Ogni nuova regola nasce come **funzione pura con test** prima del wiring negli Actor. È la ragione per cui la
maggior parte di questa mappa è fatta di `UBlueprintFunctionLibrary`: si testano senza mondo e senza Actor.

Il pathfinding multilivello (PF.4) e le reazioni componibili (E5), che le versioni precedenti di questo
documento elencavano qui come *north-star* da costruire, sono **fatti**: vedi
[`spec-pathfinding-pf3-pf4.md`](spec-pathfinding-pf3-pf4.md) e `Turn/RTReactionLibrary`.
