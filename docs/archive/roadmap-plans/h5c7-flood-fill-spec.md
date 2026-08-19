# Spec H5c.7 — Flood-fill (secchiello)

> ## 🧱 `AS-BUILT` — SPECIFICA DI CIO' CHE FU CONSEGNATO
>
> La specifica del checkpoint, congelata. **Il corpo qui sotto non va aggiornato**: comandi, nomi di branch e percorsi sono quelli di allora, e
> correggerli falsificherebbe la storia invece di renderla utile.
>
> Stato corrente: [`roadmap-checkpoint.md`](../../roadmap/roadmap-checkpoint.md) · indice dei documenti correnti:
> [`../../README.md`](../../README.md). Banner aggiunto il 2026-08-08.

> **Stato**: Approvata (design) · **Data**: 2026-08-04 · **Branch**: `feat/hex-grid`
> **Fonte**: milestone H5 di [`hex-map-roadmap.md`](../../roadmap/hex-map-roadmap.md) (riga H5c+, shape/fill). Non tocca il quadrato.

## 1. Contesto e obiettivo
Dipingere cella-per-cella (anche con drag/raggio) è lento per riempire regioni ampie della stessa superficie.
**Obiettivo**: **flood-fill (secchiello)** — click su una cella esistente → riempie in un colpo la **regione contigua** di
celle con la **stessa superficie**, applicando il pennello corrente (Surface/costo/blocco). Un Undo per riempimento.

## 2. Decisioni di design (fissate col dev)
| # | Decisione | Motivo |
|---|-----------|--------|
| D1 | **Tool dedicato** `URTHexFillTool : USingleClickTool` (non modalità del Paint) | Il flood è un'azione a click singolo; `USingleClickTool` lo modella pulito (una transazione), senza complicare il click-drag del Paint. |
| D2 | Flood = **contigue + stessa superficie + stesso layer** | Paint-bucket classico: flood-fill (frontiera a stack) sui vicini esistenti con la stessa superficie di partenza. Non crea celle nuove (i vicini inesistenti sono bordo). |
| D3 | Applica il **pennello** (Surface/costo/blocco) alle celle della regione | Coerente col Paint; il costo/blocco possono cambiare insieme alla superficie. |
| D4 | **`FloodRegion` puro sull'asset** (headless-testabile) | Il calcolo della regione è logica pura read-only → test Automation. |
| D5 | **Una transazione** per flood via le primitive di stroke (H5c.3) | `BeginStroke` + N×`PaintCellInStroke` + `EndStroke` in una `FScopedTransaction` → un Undo. |

## 3. Architettura

### 3.1 Runtime — logica pura (`URTHexMapAsset`)
`Source/RefactorTactics/Map/RTHexMapAsset.{h,cpp}`:
- `TArray<FRTCellId> FloodRegion(const FRTCellId& Start) const` — se `Start` non esiste → regione vuota; altrimenti
  flood-fill sui `URTHexLibrary::Neighbors(Cur)` (stesso layer) che **esistono** e hanno la **stessa superficie** di
  `Start` (frontiera a stack; l'ordine di visita non altera l'insieme); ritorna la regione (incluso `Start`). Read-only,
  deterministica. `RTHexLibrary.h` è già incluso in `RTHexMapAsset.cpp`.

### 3.2 Editor — tool dedicato (`URTHexFillTool`)
`Source/RefactorTacticsEditor/Private/Tools/RTHexFillTool.{h,cpp}`:
- `URTHexFillToolBuilder` (factory). `URTHexFillToolProperties`: `Surface`/`MoveCost`/`bBlocksMovement` (pennello) +
  readout `LastCell`/`FilledCount`.
- `URTHexFillTool : USingleClickTool` — `OnClicked`: `FindTargetMapActor` + `ResolveClickedCell`; se la cella **non
  esiste** nell'asset → log e stop; altrimenti `Region = Map->FloodRegion(cell)` → `FScopedTransaction` +
  `Map->BeginStroke()` + `Map->PaintCellInStroke(C, brush)` per ogni `C` della regione + `Map->EndStroke()` +
  `Actor->RebuildInstances()` + marker (verde) + readout. Un solo Undo.
- Comando `FillTool` (ToggleButton) + registrazione in `URTHexEditorMode::Enter()`; Select resta il tool default.

## 4. Flusso
`Fill` attivo → click su una cella esistente → `FloodRegion` → riempimento dell'intera regione in una transazione →
`RebuildInstances`. **Un Ctrl+Z** annulla l'intero riempimento. Click su cella vuota (assente) → nessuna azione.

## 5. Milestone (piccole e compilabili)
| ID | Contenuto | Done quando |
|----|-----------|-------------|
| **H5c.7a** FloodRegion | `URTHexMapAsset::FloodRegion` + test `RefactorTactics.HexMap.FloodRegion` | Build Editor+Game verdi; suite hex verde (incl. nuovo) |
| **H5c.7b** Fill tool | `URTHexFillTool` + property set + comando `FillTool` + registrazione nel mode | Build Editor verde; click su una regione la riempie; Undo la ripristina |

## 6. Testing e Definition of Done
- **Logica pura → test headless**: `RefactorTactics.HexMap.FloodRegion` in `Source/RefactorTactics/Tests/RTHexMapTests.cpp`:
  regione di 3 celle contigue stessa superficie; un vicino di superficie diversa = bordo (escluso); una cella stessa
  superficie ma **non contigua** = esclusa; start su cella inesistente → regione vuota.
- **Editor-bound (non headless, dichiarato)** → voce PIE in lista [`test-manuali-pie.md`](../../technical/test-manuali-pie.md):
  `PIE-HEX-MODE-N` — in Fill, click su una regione la riempie col pennello; un Undo la ripristina; click su cella vuota
  non fa nulla; con l'overlay (Select/Paint) si vedono i nuovi colori.
- **DoD**: build Editor+Game verdi; nessuna dip. editor nel runtime oltre la `FScopedTransaction` già `#if WITH_EDITOR`;
  roadmap H5 aggiornata; invarianti #1 (editor scrive solo dati d'asset) e #4 (determinismo) verificati.

## 7. Invarianti e vincoli rispettati
- **Editor non decide gameplay** (#1): il tool scrive solo dati d'asset via le primitive di stroke.
- **Determinismo** (#4): `FloodRegion` deterministica (`Neighbors` a ordine fisso 0..5 + `TSet` dei visitati); nessuna logica di turno toccata.
- **Undo/Redo**: una `FScopedTransaction` per flood (`BeginStroke` fa `Modify()` una volta).
- **No duplicazione**: `FloodRegion` unica; il tool riusa `FindTargetMapActor`/`ResolveClickedCell`/`DrawHexMarker` + le primitive di stroke.

## 8. Fuori scope (YAGNI)
Fill **globale** (tutte le celle di una superficie, non contigue); **fill-erase** (rimuove la regione); tolleranza/soglia
sulla superficie; flood **cross-layer**; overlay superfici **nel tool Fill** (per ora si usa quello di Select/Paint).

## 9. Rischi
- Bassi. `FloodRegion` è coperta da test headless; il resto (tool a click) è editor-bound (PIE-HEX-MODE-N). Su mappe
  enormi il flood tocca molte celle in una transazione (accettabile a scala di authoring). Rebuild a editor **chiuso**.
