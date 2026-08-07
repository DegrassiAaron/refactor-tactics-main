# Spec H5c.3 — Drag-brush (Paint continuo con 1 Undo per pennellata)

> **Stato**: Approvata (design) · **Data**: 2026-08-04 · **Branch**: `feat/hex-grid`
> **Fonte**: milestone H5 di [`hex-map-roadmap.md`](../hex-map-roadmap.md) (riga H5c+); prosecuzione di
> [`h5c-paint-tool-spec.md`](h5c-paint-tool-spec.md) e [`h5c2-arch-gizmo-spec.md`](h5c2-arch-gizmo-spec.md);
> [`adr-0002-griglia-esagonale.md`](../../decisions/adr-0002-griglia-esagonale.md). Non tocca il quadrato (`feat/skeletal-units`).

## 1. Contesto e obiettivo

Il tool **Paint** (H5c.1) dipinge/cancella **una cella per click**. Riempire una mappa cella-per-cella è lento.
**Obiettivo**: **drag-brush** — tenendo premuto e trascinando si dipingono/cancellano più celle in una **pennellata**,
con **un solo Undo per pennellata**. Il click singolo resta una pennellata di 1 cella (nessuna regressione su H5c.1).

## 2. Decisioni di design (fissate col dev)

| # | Decisione | Motivo |
|---|-----------|--------|
| D1 | **Evolvi il tool Paint**: base `USingleClickTool` → `UClickDragTool` | Un solo tool per paint/erase; il click è una pennellata degenere (press+release). Niente duplicazione (contro un tool separato). |
| D2 | **Un Undo per pennellata** | UX corretta (una pennellata di 20 celle = un Ctrl+Z). Richiede un percorso di scrittura senza transazione per cella. |
| D3 | **Primitive di stroke sull'actor**, `RebuildInstances` **fuori** dalle primitive | Le primitive diventano pure operazioni d'asset (headless-testabili); il refresh lo chiama il caller (tool per feedback live; `PaintCellData` a fine stroke). |
| D4 | Dedup celle nella pennellata (`TSet<FRTCellId>`) | Trascinare all'indietro non ridipinge la stessa cella. |

## 3. Architettura

### 3.1 Primitive di stroke (runtime, su `URTHexMapAsset` — non sull'actor)

*(Correzione da review, S2: le primitive stanno sull'**asset**. Toccano solo dati d'asset → headless-testabili con
`NewObject<URTHexMapAsset>`; affinano lo split invariante **asset = dati, actor = viz**. Nessun guard `!MapAsset`
interno: sono metodi dell'asset stesso, il null-check sta nel caller.)*

`Source/RefactorTactics/Map/RTHexMapAsset.{h,cpp}`:

- `void BeginStroke()` — `Modify()` una volta (registra lo stato pre-pennellata per l'undo).
- `bool PaintCellInStroke(const FRTCellId& Id, ERTHexSurface Surface, int32 MoveCost, bool bBlocksMovement)` —
  `AddOrUpdateCell(ApplyBrush(FindCell(Id), Id, Surface, MoveCost, bBlocksMovement))`; ritorna `true`.
  **Niente** `SortCells`/`MarkPackageDirty`.
- `bool EraseCellInStroke(const FRTCellId& Id)` — `if (!ContainsCell(Id)) return false;` poi `RemoveCell(Id)`; ritorna `true`.
- `void EndStroke()` — `SortCells()` + `MarkPackageDirty()`.

`Source/RefactorTactics/Map/RTHexMapActor.{h,cpp}` — **ri-espressione DRY** di `PaintCellData`/`EraseCell`
(comportamento invariato; **mantiene il guard `!MapAsset` in cima**, M2):

- `PaintCellData(Id,...)` = `if (!MapAsset){ warn; return; }` + `#if WITH_EDITOR FScopedTransaction #endif`
  + `MapAsset->BeginStroke()` + `MapAsset->PaintCellInStroke(Id,...)` + `MapAsset->EndStroke()` + `RebuildInstances()` + log.
- `EraseCell(Id)` = `if (!MapAsset){ warn; return false; }` + `if (!MapAsset->ContainsCell(Id)) return false;`
  (early-out no-op) + `FScopedTransaction` + `MapAsset->BeginStroke()` + `bRemoved = MapAsset->EraseCellInStroke(Id)`
  + `MapAsset->EndStroke()` + `RebuildInstances()` + log; ritorna `bRemoved`.

Il **RebuildInstances resta fuori** dalle primitive (lo chiama il caller: actor per il click singolo, tool per il drag).

### 3.2 Tool (editor, `URTHexPaintTool: UClickDragTool`)

`Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.{h,cpp}`:

- Base `USingleClickTool` → **`UClickDragTool`** (`IClickDragBehaviorTarget`). **`Setup()` chiama `UClickDragTool::Setup()`**
  (registra la `UClickDragInputBehavior`); **rimuovere l'override `OnClicked`** (M4). Property set e `Render` invariati.
- Override:
  - `FInputRayHit CanBeginClickDragSequence(const FInputDeviceRay& PressPos)` — accetta il click restituendo un hit
    **con profondità** (`FInputRayHit(TNumericLimits<double>::Max())` → `bHit=true`; un `FInputRayHit()` di default ha
    `bHit=false` e **non** avvierebbe il drag).
  - `void OnClickPress(const FInputDeviceRay& PressPos)` — risolvi actor+cella; **solo se** `Actor && Actor->MapAsset` e
    resolve ok: apri la transazione (`StrokeTransaction = MakeUnique<FScopedTransaction>(...)`), `TargetActor = Actor`,
    `MapAsset->BeginStroke()`, `bStrokeActive = true`, azzera `PaintedThisStroke`, `ApplyOne(cella)`, `RebuildInstances`,
    marker/readout. Se le precondizioni mancano, **non** avviare lo stroke (`bStrokeActive` resta false) — M3.
  - `void OnClickDrag(const FInputDeviceRay& DragPos)` — `if (!bStrokeActive || !TargetActor) return;` risolvi cella; se
    **non** in `PaintedThisStroke`: `ApplyOne` + `RebuildInstances` + marker.
  - `void OnClickRelease(...)` **e** `void OnTerminateDragSequence()` — `if (!bStrokeActive){ StrokeTransaction.Reset(); return; }`
    poi `TargetActor->MapAsset->EndStroke()` + `StrokeTransaction.Reset()` + azzera `bStrokeActive`/`TargetActor`/`PaintedThisStroke`.
  - `void Shutdown(EToolShutdownType)` (S1) — se `bStrokeActive`, chiudi lo stroke (`EndStroke` + `StrokeTransaction.Reset()`),
    poi `UClickDragTool::Shutdown(...)`: evita una transazione aperta a un cambio-tool/uscita mode a metà drag.
- Helper privato `ApplyOne(Actor, Cell)`: `Operation==Paint` → `bLastExisted = MapAsset->ContainsCell(Cell)` (**prima**, S4),
  poi `MapAsset->PaintCellInStroke(Cell, Surface, MoveCost, bBlocksMovement)` (marker verde); `Erase` → `bLastExisted =`
  risultato di `MapAsset->EraseCellInStroke(Cell)` (marker rosso); aggiorna `PaintedThisStroke`/`LastCell`/`ActiveLayer`.
- Stato: `TUniquePtr<FScopedTransaction> StrokeTransaction`; `bool bStrokeActive = false`; `TSet<FRTCellId> PaintedThisStroke`;
  `UPROPERTY() TObjectPtr<ARTHexMapActor> TargetActor` (risolto in `OnClickPress`, usato in drag/release/shutdown).

## 4. Flusso (una pennellata)

Press → apri transazione + `BeginStroke` + dipingi cella 1 · Drag → per ogni cella **nuova** dipingi · Release/Terminate
→ `EndStroke` + chiudi transazione. **Ctrl+Z annulla l'intera pennellata** (una `FScopedTransaction`). Click singolo =
press+release senza celle nuove oltre la prima = pennellata di 1 cella.

## 5. Milestone (piccole e compilabili)

| ID | Contenuto | Done quando |
|----|-----------|-------------|
| **H5c.3a** Stroke primitives | `BeginStroke`/`PaintCellInStroke`/`EraseCellInStroke`/`EndStroke`; ri-espressione di `PaintCellData`/`EraseCell`; test `RefactorTactics.HexMap.StrokeEquivalence` | Build Editor+Game verdi; suite hex verde (incl. nuovo); PIE-HEX-MODE-C/D ancora validi |
| **H5c.3b** Drag tool | `URTHexPaintTool` → `UClickDragTool` (press/drag/release, dedup, transazione per stroke) | Build Editor verde; una pennellata dipinge/cancella più celle, un Undo la annulla |

## 6. Testing e Definition of Done

- **Logica pura/asset → test headless**: `RefactorTactics.HexMap.StrokeEquivalence` in
  `Source/RefactorTactics/Tests/RTHexMapTests.cpp`, su `NewObject<URTHexMapAsset>` (le primitive sono **sull'asset**):
  `BeginStroke` + `PaintCellInStroke×N` + `EndStroke` produce lo stesso `NumCells`/dati di N `AddOrUpdateCell` e celle
  **ordinate** dopo `EndStroke`; `EraseCellInStroke` su cella assente ritorna `false`. `Modify()`/`MarkPackageDirty()`
  headless (`GUndo==nullptr`, `NewObject` transient) sono no-op innocui. Nessun `ARTHexMapActor`/RHI necessario.
- **Editor-bound (non headless, dichiarato)** → voci PIE nella lista [`test-manuali-pie.md`](../../technical/test-manuali-pie.md):
  - `PIE-HEX-MODE-I` — drag-paint: tenere premuto e trascinare dipinge più celle in una pennellata; **un** Ctrl+Z la annulla;
  - `PIE-HEX-MODE-J` — drag-erase (`Operation=Erase`): trascinare cancella più celle; un Undo ripristina.
- **DoD applicabile**: build Editor+Game verdi; nessun nuovo warning non spiegato; nessuna dip. editor nel runtime
  packaged; roadmap H5 aggiornata; invarianti #1 (editor scrive solo dati d'asset) e #4 (determinismo) verificati.

## 7. Invarianti e vincoli rispettati

- **Editor non decide gameplay** (#1): il tool scrive solo dati d'asset via le primitive di stroke dell'asset.
- **Split asset/actor** (affinato dalla review): le primitive di stroke sono **sull'asset** (`Modify`/`AddOrUpdateCell`/
  `RemoveCell`/`SortCells`/`MarkPackageDirty` = metodi `UObject`/asset, nessuna dip. editor); il `RebuildInstances` (viz)
  resta sull'actor; il `UClickDragTool`/ITF e la `FScopedTransaction` restano nel modulo editor.
- **Determinismo** (#4): nessuna logica di turno toccata; `SortCells` mantiene l'ordine stabile a fine stroke.
- **Undo/Redo**: una `FScopedTransaction` per pennellata; `BeginStroke` fa `Modify()` una volta prima delle mutazioni.
- **No duplicazione**: click singolo e drag condividono le primitive di stroke; `PaintCellData`/`EraseCell` ri-espressi.

## 8. Fuori scope (YAGNI)

Pennello a raggio N; rendering incrementale (durante la pennellata `RebuildInstances` ricostruisce tutte le istanze
per cella toccata — accettabile a scala di authoring; l'incrementale è follow-up); shape/fill; smoothing/interpolazione
del tratto tra due campioni di drag distanti; palette Slate custom.

## 9. Rischi

- **Cambio base del tool** (`USingleClickTool`→`UClickDragTool`) tocca il Paint di H5c.1: il click resta un caso
  degenere del drag → rischio contenuto; riverifica PIE-HEX-MODE-C/D.
- **Ciclo di vita della transazione**: `TUniquePtr<FScopedTransaction>` aperto in `OnClickPress`, chiuso in
  `OnClickRelease`/`OnTerminateDragSequence` **e** in `Shutdown` (S1: cambio-tool a metà drag). Il flag `bStrokeActive`
  (M3) garantisce che release/terminate non tocchino `EndStroke`/actor se lo stroke non è mai partito (press su vuoto,
  `CanBeginClickDragSequence` accetta ogni click). Alternativa non-RAII `BeginUndoTransaction/EndUndoTransaction`
  (+`CancelUndoTransaction`) valutata nel piano; con quella lo `Shutdown` teardown diventa **obbligatorio**.
- **`FInputRayHit` in `CanBeginClickDragSequence`** va costruito **con profondità** (`bHit=true`); un default ha
  `bHit=false` e non avvia il drag.
- **`PaintCellInStroke` senza `RebuildInstances`**: il caller deve chiamarlo (coperto da `PaintCellData` ri-espresso e
  dal tool per cella); se dimenticato, la viz non si aggiorna (non è corruzione dati).
- **API `UClickDragTool`/dedup** version-specific: pinnate sui doc/scaffold UE 5.8 in fase di piano.
- **Modulo editor modificato**: rebuild a editor chiuso.
