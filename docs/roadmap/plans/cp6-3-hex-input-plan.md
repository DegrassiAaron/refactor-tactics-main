# CP 6.3 — Input, selezione e anteprima su hex (issue #33)

> **Stato**: codice **completo** (fondamenta + wiring + anteprima) · ⏳ restano `PIE-HEXPLAY-2/3`
> **Branch**: `feat/33-hex-input-planning` (padre: `main`) · **Data**: 2026-08-05
> Roadmap: [`roadmap-checkpoint.md`](../roadmap-checkpoint.md) CP 6.3 ≡ [`roadmap-v0.1.md`](../roadmap-v0.1.md) CP 2.3

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

## Fatto: wiring del controller (`Player/RTPlayerController.cpp`)

Nessun uso residuo di `ARTGridActor`/`URTGridLibrary` nel controller (la rimozione dei file resta CP 3.2, issue #40).

- **Hover** (`PlayerTick`): `WorldToCellId` sul punto colpito; valido solo se `Map->ContainsCell(Cell)`.
  Il layer viene dalla **quota** del punto (clicchi il ponte, evidenzi il ponte).
- **Movimento**: snapshot dall'autorità (`TurnManager->MakeCurrentSnapshot`), `UnitId` = indice dell'unità,
  waypoint aggiunto **in prova** e `BuildCompositeHexPath`; se non è `Success` → **pop del waypoint** e log del
  motivo (fuori mappa / oltre budget / bloccata / occupata). Il piano precedente resta intatto.
- **Undo** (`RebuildPlannedPath`): stessa via senza aggiungere waypoint; piano non più valido → «resto fermo».
- **Scatto**: `FindPathAvoiding` con `MaxCost = GetEffectiveDashRange(...)`, celle occupate da altri esclusi;
  budget nullo intercettato **prima** della chiamata (`MaxCost == 0` significa illimitato per l'A*).
- **Targeting fail-closed**: `URTCombatLibrary::CanTargetHexCell` (distanza esagonale + LOS, `Map == nullptr`
  → falso). Test di regressione: `RefactorTactics.Combat.HexTargetingIsFailClosed`.

## Fatto: anteprima visiva (`Map/RTHexMapActor`)

`SetHoveredCell(FRTCellId, bool)` e `SetPreviewPath(TArray<FRTCellId>)` — **sola presentazione** (invariante #1).
Disegno a **debug-line**, decisione dell'utente: sblocca subito `PIE-HEXPLAY-2/3` e lascia a **M8** la
presentazione curata (mesh + materiale), dove sta già il resto della leggibilità tattica.

- Contorno giallo sulla cella sotto il cursore, contorno ciano + segmenti fra i centri per il percorso.
- I vertici vengono da `URTHexLibrary::HexCorners`, **condivisa** con il marker dell'editor: un solo
  orientamento (pointy-top, primo vertice a −30°), i due disegni non possono divergere.
  Test: `RefactorTactics.Hex.HexCornersPointyTop`.
- Il tick dell'actor è **spento** all'avvio e si accende solo quando c'è un'anteprima da disegnare.
- L'anteprima segue la selezione: cambiando unità mostra il piano di quella scelta.

**Suite: 192 test, 0 fail.** Build Game + Editor pulita, nessun warning nuovo.

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
