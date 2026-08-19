# Spec H5c.4 — Pennello a raggio N (Paint ad area)

> ## 🧱 `AS-BUILT` — SPECIFICA DI CIO' CHE FU CONSEGNATO
>
> La specifica del checkpoint, congelata. **Il corpo qui sotto non va aggiornato**: comandi, nomi di branch e percorsi sono quelli di allora, e
> correggerli falsificherebbe la storia invece di renderla utile.
>
> Stato corrente: [`roadmap-checkpoint.md`](../roadmap-checkpoint.md) · indice dei documenti correnti:
> [`../../README.md`](../../README.md). Banner aggiunto il 2026-08-08.

> **Stato**: Approvata (design) · **Data**: 2026-08-04 · **Branch**: `feat/hex-grid`
> **Fonte**: milestone H5 di [`hex-map-roadmap.md`](../hex-map-roadmap.md) (riga H5c+); prosecuzione di
> [`h5c3-drag-brush-spec.md`](h5c3-drag-brush-spec.md). Non tocca il quadrato (`feat/skeletal-units`).

## 1. Contesto e obiettivo

Il tool **Paint** (H5c.1/H5c.3) dipinge/cancella una cella per click e in pennellata (drag). **Obiettivo**: dipingere
un'**area esagonale di raggio N** attorno alla cella (raggio 0 = una cella = comportamento attuale). Accelera
l'authoring di mappe grandi; si combina col drag-brush (pennellate larghe). Fetta **solo-tool**, nessun cambio runtime.

## 2. Decisioni di design (fissate col dev)

| # | Decisione | Motivo |
|---|-----------|--------|
| D1 | **Raggio nel property set** (`int32 BrushRadius = 0`, ClampMin 0, ClampMax 8) | Raggio 0 = comportamento attuale → nessuna regressione (PIE-C/D/I/J restano validi). |
| D2 | **Applicazione ad area** via `URTHexLibrary::HexArea(center, N)` (già esistente, testata) | Nessuna nuova logica: riuso l'esagono pieno + le primitive di stroke. |
| D3 | **Dedup per-cella dell'area** (non sul centro) | Con raggio>0 il centro può essere già dipinto mentre celle di bordo sono nuove: il dedup va per cella dell'area. Si toglie l'early-out sul centro in `OnClickDrag`. |
| D4 | **RebuildInstances solo se qualcosa cambia** | Trascinare dentro un'area già dipinta non ricostruisce l'ISM a vuoto. |
| D5 | **Marker sul centro** (v1) | Il footprint appare via `RebuildInstances`; la preview live del footprint sotto il cursore è follow-up (richiederebbe un hover behavior). |

## 3. Architettura (solo `URTHexPaintTool.{h,cpp}`)

- **Property set**: `int32 BrushRadius = 0` (`EditAnywhere`, `ClampMin=0`, `ClampMax=8`, categoria `Hex|Pennello`).
- **`bool ApplyBrushAt(ARTHexMapActor* Actor, const FRTCellId& CenterCell, const FVector& CenterWorld)`** (ex `ApplyOne`,
  ora ad area, ritorna se ha cambiato qualcosa):
  - `Area = URTHexLibrary::HexArea(CenterCell, FMath::Max(0, BrushRadius))`;
  - per ogni cella `C` **non** in `PaintedThisStroke`: `bApplied = Paint ? PaintCellInStroke(C,…) : EraseCellInStroke(C)`;
    `PaintedThisStroke.Add(C)`; `bAnyChanged |= bApplied`;
  - readout/marker sul **centro** (`bLastExisted` = il centro esisteva **prima**, calcolato pre-loop; `LastCell`=centro;
    `ActiveLayer`; marker verde/rosso su `CenterWorld`); ritorna `bAnyChanged`.
- **`OnClickPress`/`OnClickDrag`**: chiamano `ApplyBrushAt(...)` e fanno `RebuildInstances()` **solo se** ha ritornato true.
  `OnClickDrag` **non** ha più l'early-out `PaintedThisStroke.Contains(centro)` (il dedup è per-cella dell'area).
- Include aggiunto: `Map/RTHexLibrary.h` (per `HexArea`). Resto del tool invariato (transazione per stroke, `bStrokeActive`,
  `EndStrokeIfActive`, `Shutdown`, `Render`).

## 4. Flusso
Press/Drag → risolvi cella centro → `HexArea(centro, raggio)` → applica alle celle nuove (dedup per-cella) → se cambiato,
`RebuildInstances`. Release/Shutdown chiudono la transazione (invariato). **Un Undo** annulla l'intera pennellata (area inclusa).

## 5. Testing e Definition of Done
- **Nessuna nuova logica pura**: `HexArea` è già coperta da `RefactorTactics.Hex.HexAreaRadius`; l'applicazione ad area è
  tool-side (editor-bound). **Nessun nuovo test headless** (dichiarato); build verde + suite **93/93** invariata.
- **Editor-bound** → voce PIE in lista [`test-manuali-pie.md`](../../technical/test-manuali-pie.md): `PIE-HEX-MODE-K` — pennello a raggio N
  (raggio>0 dipinge/cancella l'esagono pieno; raggio 0 = 1 cella; un Undo per pennellata; drag + raggio = fasce larghe).
- **DoD**: build Editor verde; nessuna dip. editor nel runtime (non tocca il runtime); invarianti #1/#4; roadmap aggiornata.

## 6. Invarianti e vincoli rispettati
- **Editor non decide gameplay** (#1): scrittura solo via primitive di stroke dell'asset.
- **Determinismo** (#4): `HexArea` deterministico; ordine celle irrilevante (SortCells a fine stroke).
- **Undo**: una `FScopedTransaction` per pennellata (invariata).
- **No duplicazione**: riusa `HexArea` + le primitive di stroke; nessun nuovo percorso di scrittura.

## 7. Fuori scope (YAGNI)
Preview live del footprint sotto il cursore (hover behavior); footprint marker; falloff/gradiente sul raggio; raggio
per-attributo; forme non esagonali. Il follow-up noto di H5c.3 (erase su cella inesistente → transazione no-op) **non**
è affrontato qui (resta tracciato).

## 8. Rischi
- Bassi: fetta solo-tool, riusa `HexArea` (testata) + primitive di stroke. Rimuovere l'early-out sul centro in
  `OnClickDrag` è coperto dal dedup per-cella + dal `RebuildInstances`-se-cambiato. Rebuild modulo editor a editor chiuso.
