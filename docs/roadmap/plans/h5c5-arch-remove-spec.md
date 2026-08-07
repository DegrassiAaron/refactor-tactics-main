# Spec H5c.5 — Rimozione archi via tool (click-to-remove)

> **Stato**: Approvata (design) · **Data**: 2026-08-04 · **Branch**: `feat/hex-grid`
> **Fonte**: milestone H5 di [`hex-map-roadmap.md`](../hex-map-roadmap.md) (riga H5c+); completa
> [`h5c2-arch-gizmo-spec.md`](h5c2-arch-gizmo-spec.md) (Add-only → Add+Remove). Non tocca il quadrato.

## 1. Contesto e obiettivo

Il tool **Arch** (H5c.2) crea transizioni (`FRTHexEdge`) via gizmo ma è **Add-only**: per rimuovere/correggere un arco
serve ancora il `RemoveVerticalTransition` `CallInEditor` del Details. **Obiettivo**: **rimuovere un arco cliccandoci
sopra** nel viewport. Fetta solo-editor + un piccolo refactor runtime; l'edit/riposizionamento resta follow-up.

## 2. Decisioni di design (fissate col dev)

| # | Decisione | Motivo |
|---|-----------|--------|
| D1 | **`ERTHexArchOp Operation { Add, Remove }`** nel property set | Add = flusso gizmo attuale; Remove = click-to-remove. |
| D2 | **Hit-test ray↔segmento** (`DistanceRayToSegment`), soglia world-space `HexSize*0.6` | Gli archi sono segmenti mondo (From→To, anche cross-layer): distanza 3D ray↔segmento, arco più vicino entro soglia. |
| D3 | **v1 rimuove l'intera connessione** (`bBoth=true`) | Un click sull'arco toglie sia From→To sia To→From (se bidirezionale). Rimozione per-direzione = YAGNI. |
| D4 | **Geometria pura in `URTHexLibrary`** (headless-testabile) | `DistanceRayToSegment` è matematica pura → test Automation, come il resto. |
| D5 | **`RemoveTransitionData` sull'actor** (estratto da `RemoveVerticalTransition`) | DRY, come `AddTransitionData` in H5c.2; il tool scrive via l'API dell'actor (invariante #1). |

## 3. Architettura

### 3.1 Geometria pura (runtime, `URTHexLibrary`)
`Source/RefactorTactics/Map/RTHexLibrary.{h,cpp}`:
- `static float DistanceRayToSegment(const FVector& RayOrigin, const FVector& RayDir, const FVector& A, const FVector& B)`
  — distanza minima tra il ray (semi-retta `Origin + t*Dir`, `t>=0`) e il segmento `A..B`. Pura, deterministica.

### 3.2 Refactor runtime (DRY, `ARTHexMapActor`)
`Source/RefactorTactics/Map/RTHexMapActor.{h,cpp}`:
- `bool RemoveTransitionData(const FRTCellId& From, const FRTCellId& To, bool bBothDirections)` — estratto da
  `RemoveVerticalTransition`: `FScopedTransaction` `#if WITH_EDITOR` + `Modify` + `MapAsset->RemoveTransition(From,To,bBoth)`
  + (se rimosso) `MarkPackageDirty` + `RebuildInstances`; ritorna se ha rimosso. `RemoveVerticalTransition()` diventa wrapper.

### 3.3 Tool (`URTHexArchTool`)
- **Property set**: `ERTHexArchOp Operation = Add`.
- **`OnClicked`** si dirama:
  - **Add** → comportamento attuale (click=From, spawn gizmo, Commit/ClearArch).
  - **Remove** → `DestroyPendingGizmo()`; risolve l'actor bersaglio; per ogni `FRTHexEdge` di `MapAsset->Transitions`
    calcola `A=AxialToWorld(From)`, `B=AxialToWorld(To)` e `DistanceRayToSegment(ClickPos.WorldRay.Origin,
    ClickPos.WorldRay.Direction, A, B)`; prende il minimo; se `< HexSize*0.6` → `Actor->RemoveTransitionData(From, To, true)`.
    Nessun arco entro soglia → nessuna azione (+ `UE_LOG`).
- **`Render`** invariato (disegna tutte le transizioni → l'arco rimosso sparisce dopo `RebuildInstances`/redraw).

## 4. Flusso (rimozione)
`Operation=Remove` → click vicino a un arco → hit-test su tutte le transizioni → rimuove la più vicina entro soglia
(una `FScopedTransaction`; **Undo** la ripristina).

## 5. Milestone (piccole e compilabili)

| ID | Contenuto | Done quando |
|----|-----------|-------------|
| **H5c.5a** Geometria + refactor | `URTHexLibrary::DistanceRayToSegment` (+ test); `ARTHexMapActor::RemoveTransitionData` (+ `RemoveVerticalTransition` wrapper) | Build Editor+Game verdi; suite hex verde (incl. nuovo); comportamento `RemoveVerticalTransition` invariato |
| **H5c.5b** Tool Remove | `ERTHexArchOp Operation`; branch Remove in `OnClicked` (hit-test + `RemoveTransitionData`) | Build Editor verde; in Remove, click su un arco lo rimuove; Undo lo ripristina |

## 6. Testing e Definition of Done
- **Logica pura → test headless**: `RefactorTactics.Hex.DistanceRayToSegment` in `Source/RefactorTactics/Tests/RTHexTests.cpp`:
  ray che interseca il segmento → ~0; ray parallelo a distanza D → ~D; punto di closest oltre un estremo → distanza
  dall'endpoint; degeneri (segmento di lunghezza 0 → distanza punto-ray). `RemoveTransition` già coperto da H4.
- **Editor-bound (non headless, dichiarato)** → voce PIE in lista [`test-manuali-pie.md`](../../technical/test-manuali-pie.md):
  `PIE-HEX-MODE-L` — in `Operation=Remove`, click su un arco disegnato lo rimuove (Undo lo ripristina); click nel vuoto
  (nessun arco entro soglia) non fa nulla; in `Operation=Add` il flusso gizmo resta invariato.
- **DoD**: build Editor+Game verdi; nessuna dip. editor nel runtime oltre la `FScopedTransaction` già `#if WITH_EDITOR`;
  roadmap H5 aggiornata; invarianti #1 (editor scrive solo dati d'asset) e #4 (determinismo) verificati.

## 7. Invarianti e vincoli rispettati
- **Editor non decide gameplay** (#1): il tool scrive solo dati d'asset via `RemoveTransitionData`.
- **Determinismo** (#4): `DistanceRayToSegment` deterministica; nessuna logica di turno toccata.
- **Undo/Redo**: rimozione via `FScopedTransaction`/`Modify` (come Add).
- **No duplicazione**: `RemoveVerticalTransition` e il tool condividono `RemoveTransitionData`; `DistanceRayToSegment` unica.

## 8. Fuori scope (YAGNI)
Edit/riposizionamento di un estremo di un arco esistente (follow-up); highlight/hover dell'arco più vicino prima del
click (hover behavior); rimozione per-direzione singola; soglia screen-space (v1 = world-space `HexSize*0.6`, limite dichiarato).

## 9. Rischi
- Bassi-medi. La geometria ray↔segmento è coperta da test headless; la selezione dell'arco più vicino + la soglia sono
  editor-bound (PIE-HEX-MODE-L). Soglia world-space: archi lontani dalla camera più difficili da centrare (accettabile v1).
- Modulo editor: rebuild a editor **chiuso** (gotcha ricorrente).
