# CP 6.3 — Input, selezione e anteprima su hex (issue #33)

> **Stato**: fondamenta fatte e testate · wiring del controller e overlay visivo **da fare**
> **Branch**: `feat/33-hex-input-planning` (padre: `main`) · **Data**: 2026-08-05
> Roadmap: [`roadmap-checkpoint.md`](roadmap-checkpoint.md) CP 6.3 ≡ [`roadmap-v0.1.md`](roadmap-v0.1.md) CP 2.3

## Perché è bloccante: la regressione trovata

Dopo CP 6.1/6.2 `ARTGameMode` allestisce la partita **solo** sulla mappa esagonale e non spawna più
`ARTGridActor`. `ARTPlayerController` però cerca ancora quell'actor. Conseguenze **verificate leggendo il codice**
(prima di questo checkpoint):

| Funzione | Riga | Effetto senza `ARTGridActor` |
|---|---|---|
| Evidenziazione cella sotto il cursore | `PlayerTick` (`if (!Grid) return;`) | **non funziona** |
| Pianificazione del movimento col mouse | `OnSelect` (`if (ARTGridActor* Grid = ...)`) | **non funziona**: nessun waypoint, nessun piano |
| Ricostruzione del percorso (undo) | `RebuildPlannedPath` (early return) | **non funziona** |
| LOS in pianificazione dell'attacco | `bHasLOS = !Grid \|\| HasLineOfSight(...)` | **fail-open**: senza griglia la LOS è sempre vera → si bersaglia attraverso i muri |
| Portata dell'attacco | `URTGridLibrary::IsWithinRange` | distanza **quadrata** su coordinate assiali → portata sbagliata su hex |

Selezione delle unità e risoluzione del turno funzionano (non passano dalla griglia). Quindi: la partita si
allestisce e il resolver gira, ma **il giocatore non può muoversi**. CP 6.3 non è una rifinitura.

## Fatto in questo passo (con test)

| Cosa | Dove | Test |
|---|---|---|
| `WorldToCellId`: punto-mondo → cella **completa** (layer dalla quota, poi assiale) | `Map/RTHexLibrary.{h,cpp}` | `RefactorTactics.Hex.WorldToCellIdRoundTripAcrossLayers` |
| `BuildCompositeHexPath`: percorso per waypoint, budget **cumulativo**, rifiuto intero | `Turn/RTHexSimLibrary.{h,cpp}` | `HexSim.CompositePathFollowsWaypoints`, `…BudgetIsCumulative`, `…RejectsCellOutOfBudget`, `…RejectsOccupiedCell`, `…EmptyWaypointsStays` |
| `FindInWorld` + `GetHexContext`: unica definizione della scala (asset autorevole, origine dall'actor) | `Map/RTHexMapActor.{h,cpp}` | `HexMapActor.HexContextAssetIsAuthoritativeOnScale` |
| `MakeCurrentSnapshot`: l'autorità espone lo stato, il client ne calcola solo la preview | `Turn/RTTurnManager.{h,cpp}` | coperto dai test d'integrazione del movimento (CP 6.2) |

**Duplicazioni rimosse** (la regola «scala dall'asset, origine dall'actor» viveva in 6 punti): `RTTurnManager`,
`RTGameMode`, `RTHexEditorClick`, `RTHexArchTool` ora delegano a `GetHexContext`. Restano due one-liner di
`MarkerRadius` nei tool editor (solo raggio di disegno, non geometria autorevole).

Suite: **190 test, 0 fail**. Build Game + Editor pulita.

### Nota sulla DoD: «riuso della logica di `RTHexEditorClick`»

Letteralmente non è possibile: `RTHexEditorClick` sta in `RefactorTacticsEditor` e il **modulo runtime non può
dipendere da un modulo Editor** (ADR-0002). Il riuso corretto — e quello realizzato — è che entrambi poggino
sulle stesse funzioni pure del runtime (`WorldToCellId`, `GetHexContext`). `ResolveClickedCell` è già stato
convertito a `GetHexContext`.

Differenza voluta fra i due usi: in **editor** il piano lo decide `ActiveLayer` (si dipinge sul layer scelto);
in **gioco** lo decide la **quota del punto colpito** (clicchi il ponte, selezioni il ponte).

## Da fare

### 1. Wiring del controller (`Player/RTPlayerController.cpp`)

- Helper privato: mappa del livello + contesto via `ARTHexMapActor::FindInWorld` + `GetHexContext`.
- `PlayerTick`: hover con `WorldToCellId`; valido solo se `Map->ContainsCell(Cell)`.
- Ramo movimento: `Snapshot = TurnManager->MakeCurrentSnapshot(Units)`, `UnitId = Units.IndexOfByKey(SelectedUnit)`,
  poi `BuildCompositeHexPath` col waypoint aggiunto in prova; se lo stato non è `Success` → **pop del waypoint**
  e log del reason (fuori budget / bloccata / occupata / fuori mappa).
- `RebuildPlannedPath`: stessa via, senza aggiungere waypoint.
- Ramo dash: `URTHexPathLibrary::FindPathAvoiding` con `MaxCost = GetEffectiveDashRange(...)`.
- **Fail-closed** sul targeting: portata con `URTHexLibrary::HexDistance`, LOS con
  `URTHexVisionLibrary::HasLineOfSight(Map, ...)`; **senza mappa si rifiuta**, non si passa.
- Rimuovere gli ultimi usi di `ARTGridActor`/`URTGridLibrary` dal controller (la rimozione dei file resta CP 3.2, issue #40).

### 2. Anteprima visiva (`Map/RTHexMapActor`)

Serve l'equivalente di `ARTGridActor::SetHoveredCell`, che sull'actor esagonale **non esiste**:
`SetHoveredCell(FRTCellId, bool)` e `SetPreviewPath(TArray<FRTCellId>)`, come **presentazione** (invariante #1:
non decidono nulla). Decisione aperta: secondo ISM di overlay (richiede mesh/materiale, coerente con M8) oppure
debug-draw temporaneo per rendere verificabile il CP e rinviare la qualità visiva a M8. **Da decidere con
l'utente**: la prima è più lavoro e tocca gli asset, la seconda sblocca subito `PIE-HEXPLAY-2/3`.

## Verifica

- Test automatici: quelli sopra (già verdi) + nessuna regressione sui 190.
- `PIE-HEXPLAY-2` (selezione e cella sotto il cursore, layer giusto su mappa multilivello).
- `PIE-HEXPLAY-3` (celle proposte = `ReachableCells`, rifiuto di oltre budget/bloccate/occupate, anteprima).
- Controprova della regressione: prima di questo CP il click su una cella non produceva **nessun** piano.

## Rischi

- Il `UnitId` dello snapshot è l'**indice** nell'array delle unità vive: cambia se un'unità muore. Va ricalcolato
  a ogni interazione (non memorizzato fra turni).
- `MaxCost == 0` significa **illimitato** per l'A*: il caso «budget residuo zero» va gestito prima della chiamata
  (già fatto in `BuildCompositeHexPath`, da ripetere nel ramo dash).
