# Spec H5c (prima fetta) — Tool "Paint-a-click" (Paint + Erase) nell'Editor Mode hex

> **Stato**: Approvata (design) · **Data**: 2026-08-04 · **Branch**: `feat/hex-grid`
> **Fonte**: milestone H5 di [`hex-map-roadmap.md`](../hex-map-roadmap.md) (riga H5c+); spec sorgente §9–§10
> (`docs/guides/Implementazione Griglia Esagonale ed Editor Mappa.docx`); [`adr-0002-griglia-esagonale.md`](../../decisions/adr-0002-griglia-esagonale.md).
> Prosecuzione della prima consegna di H5 ([`h5-editor-mode-spec.md`](h5-editor-mode-spec.md), [`h5-editor-mode-plan.md`](h5-editor-mode-plan.md)).
> Non tocca il quadrato (`feat/skeletal-units`, base di rollback).

## 1. Contesto e obiettivo

Dopo H5a+H5b l'Editor Mode hex esiste e la **selezione** avviene nel viewport (`URTHexSelectTool`, sola lettura).
Il criterio **Done** di H5 chiede che «il workflow non dipenda più **principalmente** dal pannello Details». Resta però
scoperta l'operazione di authoring n.1: **dipingere una cella** (superficie/costo/blocco). Oggi passa dai `CallInEditor`
dell'`ARTHexMapActor` (`PaintTargetCell`), che richiedono di **digitare le coordinate** in `PaintCellTarget` nel Details.
Inoltre **non esiste** un modo per rimuovere *una singola* cella dall'editor (c'è solo `ClearAsset`, che svuota tutto).

**Obiettivo di questa fetta**: un tool ITF che, cliccando nel viewport, **dipinge o cancella** la cella cliccata sul
layer attivo, con i parametri del pennello presi dal pannello del tool (non dal Details). Chiude il gap di H5 per
l'operazione più frequente e rende l'authoring nel viewport completo (crea / modifica / rimuovi).

## 2. Decisioni di design (fissate col dev)

| # | Decisione | Motivo |
|---|-----------|--------|
| D1 | **Click singolo** su `USingleClickTool` | Stessa base di H5b: riuso massimo, una transazione per click, fetta minima e compilabile. Il drag-brush continuo è H5c.2. |
| D2 | **Un solo tool** con enum `Operation { Paint, Erase }` | Un tool con modalità evita due tool quasi identici; Erase riusa `RemoveCell` (già esistente). |
| D3 | **Palette = property set del tool** (`UInteractiveToolPropertySet`) | Meccanismo ITF-native, identico al readout di H5b: niente Slate custom (rinviato a fetta successiva, YAGNI). |
| D4 | **Riuso via helper condivisi** (no duplicazione) | CLAUDE.md «non duplicare»: la scrittura cella e la pipeline click→cella diventano codice condiviso; SelectTool cambia solo per *chiamarlo* (comportamento invariato). |
| D5 | Il tool **usa** il runtime, non lo duplica | Scrive solo dati d'asset via l'API dell'actor/asset (invariante #1: l'editor non decide gameplay). |

## 3. Architettura

### 3.1 Nuovo tool (modulo editor)

`Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.{h,cpp}`:

- **`URTHexPaintToolBuilder : UInteractiveToolBuilder`** — factory (come `URTHexSelectToolBuilder`).
- **`URTHexPaintToolProperties : UInteractiveToolPropertySet`** — la palette minima (nel pannello del tool):
  - `ERTHexPaintOp Operation = Paint` — enum **editor-only**, definito nel tool (`Paint`, `Erase`).
  - `ERTHexSurface Surface = Normal`, `int32 MoveCost = 1`, `bool bBlocksMovement = false` — pennello, editabili
    (rilevanti in `Paint`).
  - readout `VisibleAnywhere`: `int32 ActiveLayer`, `FRTCellId LastCell`, `bool bLastExisted`.
- **`URTHexPaintTool : USingleClickTool`** — `OnClicked` esegue Paint o Erase; `Render` disegna il marker esagonale
  (**verde** = paint, **rosso** = erase; il giallo resta a Select).

### 3.2 Helper editor condivisi (estratti da SelectTool)

`Source/RefactorTacticsEditor/Private/RTHexEditorClick.{h,cpp}` (namespace `RTHexEditor`):

- `ARTHexMapActor* FindTargetMapActor(UWorld*)` — actor selezionato; altrimenti l'unico presente; altrimenti
  `nullptr` (più di uno e nessuno selezionato → ambiguo, nessuna azione).
- `bool ResolveClickedCell(UWorld*, ARTHexMapActor*, const FInputDeviceRay&, FRTCellId& OutCell, FVector& OutCenter)` —
  raycast sull'ISM del **target** (fallback: piano matematico alla quota del layer attivo) → `URTHexLibrary::WorldToAxial`
  sul layer attivo dell'actor. `OutCenter` = centro-mondo della cella per il marker.
- `void DrawHexMarker(FPrimitiveDrawInterface*, const FVector& Center, float Radius, const FColor&)` — l'esagono
  pointy-top oggi inline in `URTHexSelectTool::Render`.

`URTHexSelectTool` viene rifattorizzato per usare questi helper: **stessa resa, zero cambi di comportamento**.

### 3.3 Refactor runtime (`ARTHexMapActor`)

`Source/RefactorTactics/Map/RTHexMapActor.{h,cpp}`:

- `void PaintCellData(const FRTCellId& Id, ERTHexSurface Surface, int32 MoveCost, bool bBlocksMovement)` — estratto dal
  corpo attuale di `PaintTargetCell` (`FScopedTransaction` `#if WITH_EDITOR` + `Modify()` + merge-da-esistente +
  `AddOrUpdateCell` + `SortCells` + `MarkPackageDirty` + `RebuildInstances`).
- `bool EraseCell(const FRTCellId& Id)` — `FScopedTransaction` `#if WITH_EDITOR` + `Modify()` + `RemoveCell` +
  `MarkPackageDirty` + `RebuildInstances`; ritorna se ha rimosso.
- `PaintTargetCell()` diventa un wrapper: `PaintCellData(PaintCellTarget, PaintSurface, PaintMoveCost, bPaintBlocksMovement)`.

### 3.4 Logica pura estratta (per il test headless)

`Source/RefactorTactics/Map/RTHexMapAsset.{h,cpp}`:

- `static FRTHexCellData ApplyBrush(const FRTHexCellData* Existing, const FRTCellId& Id, ERTHexSurface, int32 MoveCost, bool bBlocksMovement)`
  — se `Existing` è valido parte da esso (preserva `Height`, `bBlocksLineOfSight`), altrimenti da `FRTHexCellData(Id)`;
  applica surface/cost/block. `PaintCellData` la usa. È la logica di merge, ora testabile senza editor.

### 3.5 Registrazione

- `RTHexEditorModeCommands.{h,cpp}`: aggiunge `TSharedPtr<FUICommandInfo> PaintTool` (ToggleButton) nella palette `NAME_Default`.
- `RTHexEditorMode.cpp` `Enter()`: `RegisterTool(Commands.PaintTool, TEXT("RTHexPaintTool"), NewObject<URTHexPaintToolBuilder>(this))`.
  `Select` resta il tool attivo di default (`SelectActiveToolType`); `Paint` è opt-in dai bottoni del mode.

## 4. Flusso dati (un click)

1. Click → `USingleClickInputBehavior` → `OnClicked`.
2. `RTHexEditor::FindTargetMapActor` → se `nullptr`, warning e stop (nessuna azione a vuoto).
3. `RTHexEditor::ResolveClickedCell` → `FRTCellId` sul layer attivo + centro-mondo per il marker.
4. `Operation == Paint` → `Actor->PaintCellData(Cell, Surface, MoveCost, bBlocksMovement)` (crea se assente, preserva
   `Height`/`LOS`). `Operation == Erase` → `Actor->EraseCell(Cell)`.
5. Aggiorna readout (`LastCell`, `bLastExisted`, `ActiveLayer`), marker (verde/rosso), `UE_LOG`.
6. **Undo/Redo**: una `FScopedTransaction` per click, dentro `PaintCellData`/`EraseCell` (coerente con H2/H4).

## 5. Milestone (piccole e compilabili)

| ID | Contenuto | Done quando |
|----|-----------|-------------|
| **H5c.1a** Refactor DRY | Helper editor `RTHexEditorClick` (SelectTool li usa, invariato); `ARTHexMapActor::PaintCellData`/`EraseCell` (+ `PaintTargetCell` wrapper); `URTHexMapAsset::ApplyBrush`; test `RefactorTactics.Hex.PaintBrushMerge` | Build Editor + Game verdi; test hex verdi (incluso il nuovo); `PIE-HEX-MODE-B` ancora valido |
| **H5c.1b** Paint tool | `URTHexPaintTool` + property set (palette) + comando `PaintTool` + registrazione nel mode | Build Editor verde; nel viewport si dipinge/cancella una cella cliccando, senza Details; Undo/Redo coerente |

## 6. Testing e Definition of Done

- **Logica pura → test headless**: `RefactorTactics.Hex.PaintBrushMerge` in `Source/RefactorTactics/Tests/RTHexMapTests.cpp`:
  - cella nuova → `Height=0`, `LOS=false`, applica surface/cost/block;
  - cella esistente con `Height=3`, `bBlocksLineOfSight=true` → cambia surface/cost/block ma **preserva** `Height` e `LOS`.
  - (Erase = `RemoveCell`, già coperto da H1.)
  - *Gotcha unity-build*: eventuali helper in namespace anonimo nel file di test devono avere nomi **unici per file**.
- **Editor-bound (non headless, dichiarato)**: click/marker/pannello/switch tool → nuove voci manuali in
  [`test-manuali-pie.md`](../../technical/test-manuali-pie.md):
  - `PIE-HEX-MODE-C` — paint a click (crea/aggiorna cella, marker verde, readout coerente, cambio `ActiveLayer`);
  - `PIE-HEX-MODE-D` — erase a click (rimuove la cella cliccata, marker rosso, Undo la ripristina).
- **DoD applicabile**: build Editor + Game verdi; nessun nuovo warning non spiegato; nessuna dip. editor nel runtime
  packaged (garantito dal Type `Editor`); roadmap H5 aggiornata; limiti dichiarati; invarianti #1 e #4 verificati.

## 7. Invarianti e vincoli rispettati

- **Regole in C++, editor non decide gameplay** (#1): il tool scrive solo dati d'asset, via l'API di actor/asset.
- **Determinismo** (#4): nessuna logica di turno toccata; `WorldToAxial` deterministico (arrotondamento cubico, interi).
- **Undo/Redo**: ogni modifica passa da `Modify()`/`FScopedTransaction`.
- **Separazione runtime/editor**: le dip. editor restano confinate nel modulo `RefactorTacticsEditor`; il refactor
  runtime (`PaintCellData`/`EraseCell`/`ApplyBrush`) non aggiunge dipendenze editor (la `FScopedTransaction` è già
  `#if WITH_EDITOR` in `RTHexMapActor.cpp`).
- **No duplicazione**: SelectTool e PaintTool condividono la pipeline click→cella; `PaintTargetCell` e il tool
  condividono la scrittura.

## 8. Fuori scope (YAGNI)

Drag-brush continuo (H5c.2); pennello a raggio N; palette Slate custom; toggle per-attributo (dipingi solo superficie);
multi-selezione Shift/Ctrl-click; shape/fill; strumenti archi con gizmo; copia/incolla; overlay debug layer.

## 9. Rischi

- **API ITF version-specific** (property set, registrazione tool, `FInputDeviceRay`, PDI): pinnate sui doc/scaffold
  **UE 5.8** (`Engine/Plugins/Editor/SampleToolsEditorMode`) in fase di writing-plans, prima di scrivere codice.
- **Modulo editor modificato**: rebuild a **editor chiuso** (Live Coding non basta per un modulo editor), come da procedura di progetto.
- **Refactor di SelectTool**: rischio basso (sola estrazione, resa identica); verificato da build + `PIE-HEX-MODE-B`.
