# H5c.4 — Pennello a raggio N — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or
> superpowers:executing-plans. Steps use checkbox (`- [ ]`) syntax.

**Goal:** Il tool Paint dipinge/cancella un'area esagonale di raggio N attorno alla cella (raggio 0 = una cella).

**Architecture:** Solo `URTHexPaintTool.{h,cpp}`. Aggiunge `BrushRadius` al property set; `ApplyOne` (una cella) diventa `ApplyBrushAt` (area via `URTHexLibrary::HexArea`, dedup per-cella, ritorna se ha cambiato); `OnClickPress`/`OnClickDrag` chiamano `ApplyBrushAt` e ricostruiscono l'ISM solo se cambiato. Nessun cambio runtime.

**Tech Stack:** UE 5.8.1 C++. Spec: [`h5c4-radius-brush-spec.md`](h5c4-radius-brush-spec.md).

## Global Constraints
- UE 5.8.1; `EngineAssociation` deve restare `"5.8"` (ripristina con `git checkout -- RefactorTactics.uproject` se risporcato).
- Prefissi `RT`/`URT`. NO `Build.cs` change. Nessun cambio al modulo runtime.
- Branch **corrente** `feat/hex-grid`; no worktree/switch.
- **Staging solo-hex**: NIENTE `git add -A`/`git add .`; solo i file dello Step di commit. (Untracked `docs/PDR/*.pdf` e `docs/use-case-list.md` modificato = utente, NON committare.)
- **Editor CHIUSO** durante il rebuild.
- Build: `"D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -project="D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -waitmutex` → `Result: Succeeded`.
- Test headless: `"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -ExecCmds="Automation RunTests RefactorTactics; Quit" -unattended -nopause -nosplash -nullrhi` → `Saved/Logs/RefactorTactics.log`, `Fail` = 0.

---

## Task 1 (H5c.4): Pennello a raggio N nel tool Paint

**Files:**
- Modify: `Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.h` (property `BrushRadius`; rinomina `ApplyOne`→`ApplyBrushAt` con ritorno `bool`)
- Modify: `Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.cpp` (area loop + include + press/drag)
- Modify: `docs/design/test-manuali-pie.md` (voce PIE-HEX-MODE-K)
- Modify: `docs/design/hex-map-roadmap.md` (riga H5: H5c.4 fatta)

**Interfaces:**
- Consumes: `URTHexLibrary::HexArea(const FRTCellId&, int32)`; `URTHexMapAsset::{PaintCellInStroke,EraseCellInStroke,FindCell,HexSize}`; `RTHexEditor::{FindTargetMapActor,ResolveClickedCell}`; `ARTHexMapActor::{MapAsset,ActiveLayer,RebuildInstances}`.

- [ ] **Step 1: Nota testing (nessun test headless)**

Nessuna nuova logica pura: `HexArea` è già coperta da `RefactorTactics.Hex.HexAreaRadius`; l'applicazione ad area è editor-bound. Verifica in editor → PIE-HEX-MODE-K (voce in lista allo Step 5). *(Dichiarazione DoD.)*

- [ ] **Step 2: Aggiungere `BrushRadius` al property set**

In `Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.h`, dentro `URTHexPaintToolProperties`, subito dopo il blocco di `bBlocksMovement`, aggiungere:
```cpp
	/** Raggio del pennello (celle): 0 = una cella; N = esagono pieno di raggio N (HexArea). */
	UPROPERTY(EditAnywhere, Category = "Hex|Pennello", meta = (ClampMin = "0", ClampMax = "8"))
	int32 BrushRadius = 0;
```

- [ ] **Step 3: Rinominare la dichiarazione `ApplyOne` → `ApplyBrushAt` (ritorno bool)**

In `Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.h`, nella sezione `protected` di `URTHexPaintTool`, sostituire:
```cpp
	void ApplyOne(ARTHexMapActor* Actor, const FRTCellId& Cell, const FVector& Center);
```
con:
```cpp
	/** Applica il pennello (area di raggio BrushRadius) attorno a CenterCell; dedup per-cella. Ritorna se ha cambiato qualcosa. */
	bool ApplyBrushAt(ARTHexMapActor* Actor, const FRTCellId& CenterCell, const FVector& CenterWorld);
```

- [ ] **Step 4: Implementare l'applicazione ad area + press/drag**

In `Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.cpp`:

(a) In cima, dopo `#include "Map/RTHexMapAsset.h"`, aggiungere:
```cpp
#include "Map/RTHexLibrary.h" // HexArea
```

(b) Sostituire l'INTERO metodo `ApplyOne` (attuale, ~righe 36-56) con `ApplyBrushAt`:
```cpp
bool URTHexPaintTool::ApplyBrushAt(ARTHexMapActor* Actor, const FRTCellId& CenterCell, const FVector& CenterWorld)
{
	URTHexMapAsset* Map = Actor->MapAsset; // il caller garantisce Actor && Map non nulli
	const bool bPaint = (Properties->Operation == ERTHexPaintOp::Paint);
	const bool bCenterExistedBefore = (Map->FindCell(CenterCell) != nullptr); // per il readout, pre-mutazione

	const TArray<FRTCellId> Area = URTHexLibrary::HexArea(CenterCell, FMath::Max(0, Properties->BrushRadius));
	bool bAnyChanged = false;
	for (const FRTCellId& C : Area)
	{
		if (PaintedThisStroke.Contains(C)) { continue; } // dedup per-cella nella pennellata
		const bool bApplied = bPaint
			? Map->PaintCellInStroke(C, Properties->Surface, Properties->MoveCost, Properties->bBlocksMovement)
			: Map->EraseCellInStroke(C);
		PaintedThisStroke.Add(C);
		bAnyChanged = bAnyChanged || bApplied;
	}

	// Readout/marker sul centro.
	MarkerColor = bPaint ? FColor::Green : FColor::Red;
	Properties->bLastExisted = bCenterExistedBefore;
	Properties->LastCell = CenterCell;
	Properties->ActiveLayer = Actor->ActiveLayer;
	MarkerCenter = CenterWorld;
	MarkerRadius = Map->HexSize * 0.9f;
	bHasMarker = true;
	return bAnyChanged;
}
```

(c) In `OnClickPress`, sostituire le righe:
```cpp
	ApplyOne(Actor, Cell, Center);
	Actor->RebuildInstances();
```
con:
```cpp
	if (ApplyBrushAt(Actor, Cell, Center))
	{
		Actor->RebuildInstances();
	}
```

(d) Sostituire l'INTERO metodo `OnClickDrag` attuale (toglie l'early-out sul centro; dedup ora per-cella dell'area; rebuild se cambiato). Da:
```cpp
void URTHexPaintTool::OnClickDrag(const FInputDeviceRay& DragPos)
{
	if (!bStrokeActive || !TargetActor || !TargetActor->MapAsset) { return; }

	FRTCellId Cell;
	FVector Center;
	if (!RTHexEditor::ResolveClickedCell(TargetWorld, TargetActor, DragPos, Cell, Center)) { return; }
	if (PaintedThisStroke.Contains(Cell)) { return; } // dedup: trascinare all'indietro non ridipinge

	ApplyOne(TargetActor, Cell, Center);
	TargetActor->RebuildInstances();
}
```
a:
```cpp
void URTHexPaintTool::OnClickDrag(const FInputDeviceRay& DragPos)
{
	if (!bStrokeActive || !TargetActor || !TargetActor->MapAsset) { return; }

	FRTCellId Cell;
	FVector Center;
	if (!RTHexEditor::ResolveClickedCell(TargetWorld, TargetActor, DragPos, Cell, Center)) { return; }

	if (ApplyBrushAt(TargetActor, Cell, Center))
	{
		TargetActor->RebuildInstances();
	}
}
```

- [ ] **Step 5: Build + suite + voce PIE**

Build (editor chiuso) → `Result: Succeeded`. Poi run suite `RefactorTactics` → **0 Fail** (nessun cambio runtime → invariata).
Aggiungere a `docs/design/test-manuali-pie.md`:
```markdown
| **PIE-HEX-MODE-K** | Pennello a raggio N (H5c.4) | mode Hex Map, tool Paint, `ARTHexMapActor` con `MapAsset` | `BrushRadius=0` → 1 cella (come prima); `BrushRadius=N>0` → un click dipinge/cancella l'esagono pieno di raggio N; drag dipinge fasce larghe (dedup); **un** Ctrl+Z annulla l'intera pennellata | ⏳ (branch `feat/hex-grid`, H5c.4) |
```

- [ ] **Step 6: Aggiornare la roadmap (riga H5)**

In `docs/design/hex-map-roadmap.md`, nella cella Stato della riga **H5**, aggiungere in coda: `H5c.4: pennello a raggio N - URTHexPaintToolProperties::BrushRadius (0=1 cella; N=HexArea); ApplyOne -> ApplyBrushAt (area, dedup per-cella, RebuildInstances se cambiato). Verifica editor PIE-HEX-MODE-K aperta.`

- [ ] **Step 7: Commit**

```bash
git checkout -- RefactorTactics.uproject   # se risporcato
git add Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.h \
        Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.cpp \
        docs/design/test-manuali-pie.md docs/design/hex-map-roadmap.md
git commit -m "feat(hex): H5c.4 - pennello a raggio N (BrushRadius + ApplyBrushAt area)"
```

---

## Self-Review (eseguita)
- **Copertura spec**: §3 property → Step 2; ApplyBrushAt area+dedup+ritorno → Step 3-4b; press/drag rebuild-se-cambiato + no early-out → Step 4c-4d; include HexArea → Step 4a; PIE → Step 5; roadmap → Step 6. Nessun gap.
- **Placeholder**: nessuno; codice completo.
- **Consistenza**: `ApplyBrushAt(Actor, CenterCell, CenterWorld)` dichiarato (Step 3) e definito/usato (Step 4) coerente; `HexArea(FRTCellId, int32)` e `PaintCellInStroke`/`EraseCellInStroke` come da API esistente.

## Rischi noti
- Rimuovere l'early-out sul centro in `OnClickDrag`: coperto dal dedup per-cella + `RebuildInstances`-se-cambiato (raggio 0 → area `[centro]`, stesso effetto dell'early-out).
- Il follow-up H5c.3 (erase su cella inesistente → transazione no-op) NON è affrontato qui; con raggio>0 in erase, `bAnyChanged` resta false se nessuna cella dell'area esisteva → nessun `RebuildInstances` inutile (ma la transazione è comunque aperta in `OnClickPress`, come da follow-up tracciato).
- Modulo editor: rebuild a editor chiuso.
