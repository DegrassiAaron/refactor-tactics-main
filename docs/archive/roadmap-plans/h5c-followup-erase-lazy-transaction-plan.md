# Follow-up H5c.3 — Erase senza transazione no-op (transazione lazy) — Implementation Plan

> ## 📦 `DELIVERED PLAN` — PIANO GIA' ESEGUITO, NON NORMATIVO
>
> Il piano di esecuzione, gia' eseguito. **Il corpo qui sotto non va aggiornato**: comandi, nomi di branch e percorsi sono quelli di allora, e
> correggerli falsificherebbe la storia invece di renderla utile.
>
> Stato corrente: [`roadmap-checkpoint.md`](../../roadmap/roadmap-checkpoint.md) · indice dei documenti correnti:
> [`../../README.md`](../../README.md). Banner aggiunto il 2026-08-08.

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development o executing-plans. Step con checkbox.

**Goal:** L'erase (click/drag) su celle inesistenti non deve creare una voce Undo no-op né marcare l'asset dirty. Ripristina per il drag tool la garanzia che il click singolo di H5c.1 aveva (`ARTHexMapActor::EraseCell` con early-out `!ContainsCell`).

**Architecture:** In `URTHexPaintTool`, la `FScopedTransaction` + `MapAsset->BeginStroke()` (Modify) non si aprono più subito in `OnClickPress`, ma **lazy** al **primo cambiamento reale** (`EnsureStrokeOpen`, chiamato in `ApplyBrushAt` solo quando una cella cambia davvero: paint sempre, erase solo se la cella esiste). `EndStrokeIfActive` chiude solo se la transazione è stata aperta. Un drag che parte su celle vuote e prosegue su celle reali apre la transazione al primo removal → vincolo rispettato. Solo `RTHexPaintTool.{h,cpp}`; nessun cambio runtime.

**Tech Stack:** UE 5.8.1 C++.

## Global Constraints
- UE 5.8.1; `EngineAssociation` deve restare `"5.8"` (ripristina con `git checkout -- RefactorTactics.uproject` se risporcato).
- Prefissi `RT`/`URT`. NO `Build.cs`/runtime change. Editor deps confinate.
- Branch **corrente** `feat/hex-grid`; no worktree/switch.
- **Staging solo-hex**: solo i file dello Step di commit; user's `docs/use-case-list.md` (modificato) e `docs/PDR/*.pdf` (untracked) NON committare.
- **Editor CHIUSO** durante il rebuild.
- Build: `"D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -project="D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -waitmutex` → `Result: Succeeded`.
- Test headless: `"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -ExecCmds="Automation RunTests RefactorTactics; Quit" -unattended -nopause -nosplash -nullrhi` → `Fail` = 0.

---

## Task 1: Transazione lazy in `URTHexPaintTool`

**Files:**
- Modify: `Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.h` (dichiara `EnsureStrokeOpen`; aggiunge `bStrokeOpened`)
- Modify: `Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.cpp` (lazy open in `ApplyBrushAt`; `OnClickPress` senza open eager; `EndStrokeIfActive` gate su `bStrokeOpened`; def. `EnsureStrokeOpen`)
- Modify: `docs/design/test-manuali-pie.md` (estende PIE-HEX-MODE-J)

- [ ] **Step 1: Nota testing (nessun test headless)**

La transazione/dirty è editor-bound; la logica d'asset (`EraseCellInStroke` su cella assente → `false`) è già coperta da `RefactorTactics.HexMap.StrokeEquivalence`/`AddFindContainsUpdateRemove`. Verifica in editor (PIE-HEX-MODE-J esteso). *(Dichiarazione DoD.)*

- [ ] **Step 2: Header — dichiarare `EnsureStrokeOpen` + campo `bStrokeOpened`**

In `Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.h`, sostituire:
```cpp
	bool ApplyBrushAt(ARTHexMapActor* Actor, const FRTCellId& CenterCell, const FVector& CenterWorld);
	void EndStrokeIfActive();
```
con:
```cpp
	bool ApplyBrushAt(ARTHexMapActor* Actor, const FRTCellId& CenterCell, const FVector& CenterWorld);
	void EndStrokeIfActive();
	/** Apre lazy la transazione + MapAsset->BeginStroke() al primo cambiamento reale (una sola volta per pennellata). */
	void EnsureStrokeOpen();
```
e sostituire:
```cpp
	TUniquePtr<FScopedTransaction> StrokeTransaction;
	bool bStrokeActive = false;
	TSet<FRTCellId> PaintedThisStroke;
```
con:
```cpp
	TUniquePtr<FScopedTransaction> StrokeTransaction;
	bool bStrokeActive = false;
	bool bStrokeOpened = false; // transazione+BeginStroke aperti (lazy, al primo cambiamento reale)?
	TSet<FRTCellId> PaintedThisStroke;
```

- [ ] **Step 3: `ApplyBrushAt` — apertura lazy prima della prima mutazione reale**

In `Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.cpp`, dentro `ApplyBrushAt`, sostituire il corpo del `for`:
```cpp
	for (const FRTCellId& C : Area)
	{
		if (PaintedThisStroke.Contains(C)) { continue; } // dedup per-cella nella pennellata
		const bool bApplied = bPaint
			? Map->PaintCellInStroke(C, Properties->Surface, Properties->MoveCost, Properties->bBlocksMovement)
			: Map->EraseCellInStroke(C);
		PaintedThisStroke.Add(C);
		bAnyChanged = bAnyChanged || bApplied;
	}
```
con:
```cpp
	for (const FRTCellId& C : Area)
	{
		if (PaintedThisStroke.Contains(C)) { continue; } // dedup per-cella nella pennellata
		// Apri la transazione/stroke LAZY solo se questa cella cambia davvero (paint cambia sempre; erase solo se esiste).
		if (bPaint || Map->ContainsCell(C))
		{
			EnsureStrokeOpen();
		}
		const bool bApplied = bPaint
			? Map->PaintCellInStroke(C, Properties->Surface, Properties->MoveCost, Properties->bBlocksMovement)
			: Map->EraseCellInStroke(C);
		PaintedThisStroke.Add(C);
		bAnyChanged = bAnyChanged || bApplied;
	}
```

- [ ] **Step 4: `OnClickPress` — niente apertura eager**

In `Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.cpp`, dentro `OnClickPress`, sostituire:
```cpp
	StrokeTransaction = MakeUnique<FScopedTransaction>(LOCTEXT("HexBrushStroke", "Hex: Brush Stroke"));
	TargetActor = Actor;
	Actor->MapAsset->BeginStroke();
	bStrokeActive = true;
	PaintedThisStroke.Reset();
```
con:
```cpp
	// La transazione + BeginStroke si aprono LAZY al primo cambiamento reale (EnsureStrokeOpen): niente transazione
	// no-op quando l'erase non tocca celle esistenti (garanzia ripristinata rispetto al click singolo di H5c.1).
	TargetActor = Actor;
	bStrokeActive = true;
	bStrokeOpened = false;
	PaintedThisStroke.Reset();
```

- [ ] **Step 5: `EndStrokeIfActive` — gate su `bStrokeOpened` + `EnsureStrokeOpen`**

In `Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.cpp`, sostituire l'INTERO metodo `EndStrokeIfActive`:
```cpp
void URTHexPaintTool::EndStrokeIfActive()
{
	if (bStrokeActive && TargetActor && TargetActor->MapAsset)
	{
		TargetActor->MapAsset->EndStroke();
		TargetActor->RebuildInstances(); // riallinea InstanceCells all'ordine post-SortCells
	}
	StrokeTransaction.Reset(); // chiude/commit la transazione (o no-op se non aperta)
	bStrokeActive = false;
	TargetActor = nullptr;
	PaintedThisStroke.Reset();
}
```
con (gate su `bStrokeOpened`, e definizione di `EnsureStrokeOpen` subito dopo):
```cpp
void URTHexPaintTool::EndStrokeIfActive()
{
	if (bStrokeOpened && TargetActor && TargetActor->MapAsset)
	{
		TargetActor->MapAsset->EndStroke();
		TargetActor->RebuildInstances(); // riallinea InstanceCells all'ordine post-SortCells
	}
	StrokeTransaction.Reset(); // chiude/commit la transazione (o no-op se non aperta)
	bStrokeActive = false;
	bStrokeOpened = false;
	TargetActor = nullptr;
	PaintedThisStroke.Reset();
}

void URTHexPaintTool::EnsureStrokeOpen()
{
	if (bStrokeOpened) { return; }
	StrokeTransaction = MakeUnique<FScopedTransaction>(LOCTEXT("HexBrushStroke", "Hex: Brush Stroke"));
	if (TargetActor && TargetActor->MapAsset)
	{
		TargetActor->MapAsset->BeginStroke(); // Modify() dopo l'apertura della transazione: cattura lo stato pre-pennellata
	}
	bStrokeOpened = true;
}
```

- [ ] **Step 6: Build + suite + PIE**

Build (editor chiuso) → `Result: Succeeded`. Poi run suite `RefactorTactics` → **0 Fail** (nessun cambio runtime → invariata).
In `docs/design/test-manuali-pie.md`, estendere la riga **PIE-HEX-MODE-J** (colonna «Esito atteso»). Sostituire:
```markdown
| **PIE-HEX-MODE-J** | Drag-erase (H5c.3b) | mode Hex Map, tool Paint (`Operation=Erase`) | Trascinare cancella più celle in una pennellata; un Undo le ripristina tutte; cambiare tool a metà drag non lascia transazioni aperte | ⏳ (branch `feat/hex-grid`, H5c.3b) |
```
con:
```markdown
| **PIE-HEX-MODE-J** | Drag-erase (H5c.3b) | mode Hex Map, tool Paint (`Operation=Erase`) | Trascinare cancella più celle in una pennellata; un Undo le ripristina tutte; cambiare tool a metà drag non lascia transazioni aperte; **erase su celle inesistenti/vuote NON crea voci Undo né marca l'asset dirty** (transazione lazy) | ⏳ (branch `feat/hex-grid`, H5c.3b) |
```

- [ ] **Step 7: Commit**

```bash
git checkout -- RefactorTactics.uproject   # se risporcato
git add Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.h \
        Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.cpp \
        docs/design/test-manuali-pie.md
git commit -m "fix(hex): erase-brush non apre transazione no-op su celle inesistenti (transazione lazy)"
```

---

## Self-Review (eseguita)
- **Correttezza ordine undo**: `EnsureStrokeOpen` apre la `FScopedTransaction` **prima** di `BeginStroke()` (Modify), che precede la mutazione (`PaintCellInStroke`/`EraseCellInStroke`) → lo stato pre-pennellata è catturato. Paint: apre al primo click (paint cambia sempre) = invariato. Erase: apre al primo removal reale; erase-all-miss = mai aperta → nessun Undo/dirty.
- **Vincolo drag-su-vuoto**: `bStrokeActive=true` sul press permette a `OnClickDrag` di proseguire; `EnsureStrokeOpen` apre lazy quando il drag tocca la prima cella reale.
- **`EndStrokeIfActive`**: ora gate su `bStrokeOpened` (non `bStrokeActive`) → salta `EndStroke`/`RebuildInstances`/dirty se nulla è stato aperto; `StrokeTransaction.Reset()` è no-op se null.
- **Placeholder**: nessuno; anchor `old_string` verificati sul file on-disk (post-H5c.4).

## Rischi noti
- Solo tool; nessun cambio runtime. Editor rebuild a editor chiuso. Verifica end-to-end in PIE-HEX-MODE-J (esteso).
