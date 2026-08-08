# H5c.3 — Drag-brush (Paint continuo, 1 Undo/pennellata) — Implementation Plan

> ## 📦 `DELIVERED PLAN` — PIANO GIA' ESEGUITO, NON NORMATIVO
>
> Il piano di esecuzione, gia' eseguito. **Il corpo qui sotto non va aggiornato**: comandi, nomi di branch e percorsi sono quelli di allora, e
> correggerli falsificherebbe la storia invece di renderla utile.
>
> Stato corrente: [`roadmap-checkpoint.md`](../roadmap-checkpoint.md) · indice dei documenti correnti:
> [`../../README.md`](../../README.md). Banner aggiunto il 2026-08-08.

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or
> superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Dipingere/cancellare più celle tenendo premuto e trascinando (una pennellata), con un solo Undo per pennellata; il click singolo resta una pennellata di 1 cella.

**Architecture:** Primitive di stroke **sull'asset** `URTHexMapAsset` (`BeginStroke`/`PaintCellInStroke`/`EraseCellInStroke`/`EndStroke`, pure operazioni d'asset headless-testabili); `PaintCellData`/`EraseCell` dell'actor ri-espressi in termini di quelle (comportamento invariato). Il tool `URTHexPaintTool` passa da `USingleClickTool` a `UClickDragTool`: apre una `FScopedTransaction` al press, chiama le primitive per ogni cella nuova durante il drag (dedup), chiude a release/terminate/shutdown. `RebuildInstances` (viz) resta sull'actor, chiamato dal caller.

**Tech Stack:** UE 5.8.1 C++; `InteractiveToolsFramework` (BaseTools/ClickDragTool). Spec: [`h5c3-drag-brush-spec.md`](h5c3-drag-brush-spec.md).

## Global Constraints

- Motore **UE 5.8.1**; `RefactorTactics.uproject` `EngineAssociation` deve restare `"5.8"` (ripristinare con `git checkout -- RefactorTactics.uproject` se risporcato).
- Prefissi `RT`/`URT`; documentazione in `docs/`. NO `Build.cs` change.
- **L'editor non decide gameplay** (#1): scrittura solo su dati d'asset via `Modify()`/`FScopedTransaction`.
- **Split asset/actor**: le primitive di stroke sono sull'**asset** (`Modify`/`AddOrUpdateCell`/`RemoveCell`/`SortCells`/`MarkPackageDirty` = metodi `UObject`/asset, nessuna dip. editor); `RebuildInstances` resta sull'actor; ITF/`FScopedTransaction`/`UClickDragTool` nel modulo editor.
- **Determinismo** (#4): nessuna logica di turno toccata; `SortCells` a fine stroke mantiene l'ordine stabile.
- **No duplicazione**: click singolo e drag condividono le primitive; `PaintCellData`/`EraseCell` ri-espressi (non duplicati).
- Un **commit per task**; build Editor verde prima di dichiarare fatto; verifiche editor-bound → **voci PIE nella lista** (non gate bloccanti).
- **Editor CHIUSO** durante il rebuild.
- Build (editor chiuso): `"D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -project="D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -waitmutex` → `Result: Succeeded`.
- Test headless: `"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -ExecCmds="Automation RunTests <PATTERN>; Quit" -unattended -nopause -nosplash -nullrhi` → `Saved/Logs/RefactorTactics.log`, `Fail` = 0.

---

## Task 1 (H5c.3a): Primitive di stroke sull'asset + ri-espressione + test

Aggiunge `BeginStroke`/`PaintCellInStroke`/`EraseCellInStroke`/`EndStroke` a `URTHexMapAsset` (con test headless), e ri-esprime `ARTHexMapActor::PaintCellData`/`EraseCell` in termini di quelle (comportamento invariato). Nessun tool toccato qui. Deliverable: build verde + suite hex verde + PIE-HEX-MODE-C/D ancora validi.

**Files:**
- Modify: `Source/RefactorTactics/Map/RTHexMapAsset.h` (dichiara le 4 primitive)
- Modify: `Source/RefactorTactics/Map/RTHexMapAsset.cpp` (implementa le 4 primitive)
- Modify: `Source/RefactorTactics/Map/RTHexMapActor.cpp` (ri-esprime `PaintCellData`/`EraseCell`)
- Test: `Source/RefactorTactics/Tests/RTHexMapTests.cpp` (nuovo `StrokeEquivalence`)

**Interfaces:**
- Produces: `void URTHexMapAsset::BeginStroke()`; `bool URTHexMapAsset::PaintCellInStroke(const FRTCellId&, ERTHexSurface, int32, bool)`; `bool URTHexMapAsset::EraseCellInStroke(const FRTCellId&)`; `void URTHexMapAsset::EndStroke()`.
- Consumes (esistenti): `URTHexMapAsset::{ApplyBrush(static),FindCell,AddOrUpdateCell,ContainsCell,RemoveCell,SortCells}`, `UObject::{Modify,MarkPackageDirty}`.

- [ ] **Step 1: Scrivere il test che fallisce (`StrokeEquivalence`)**

In `Source/RefactorTactics/Tests/RTHexMapTests.cpp`, prima di `#endif // WITH_DEV_AUTOMATION_TESTS`, aggiungere:
```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexStrokeEquivalenceTest,
	"RefactorTactics.HexMap.StrokeEquivalence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexStrokeEquivalenceTest::RunTest(const FString&)
{
	// Una pennellata di 3 celle produce lo stesso contenuto di 3 AddOrUpdateCell, e celle ordinate dopo EndStroke.
	URTHexMapAsset* Stroke = NewObject<URTHexMapAsset>();
	Stroke->BeginStroke();
	TestTrue(TEXT("paint 1"), Stroke->PaintCellInStroke(FRTCellId(2, -1, 0), ERTHexSurface::Water, 3, true));
	TestTrue(TEXT("paint 2"), Stroke->PaintCellInStroke(FRTCellId(0, 0, 0), ERTHexSurface::Normal, 1, false));
	TestTrue(TEXT("paint 3"), Stroke->PaintCellInStroke(FRTCellId(1, 0, 1), ERTHexSurface::Mud, 2, false));
	Stroke->EndStroke();

	URTHexMapAsset* Direct = NewObject<URTHexMapAsset>();
	FRTHexCellData C1(FRTCellId(2, -1, 0)); C1.Surface = ERTHexSurface::Water; C1.MoveCost = 3; C1.bBlocksMovement = true;
	FRTHexCellData C2(FRTCellId(0, 0, 0)); // default: Normal, costo 1, no block
	FRTHexCellData C3(FRTCellId(1, 0, 1)); C3.Surface = ERTHexSurface::Mud; C3.MoveCost = 2;
	Direct->AddOrUpdateCell(C1); Direct->AddOrUpdateCell(C2); Direct->AddOrUpdateCell(C3);
	Direct->SortCells();

	TestEqual(TEXT("stesso NumCells"), Stroke->NumCells(), Direct->NumCells());
	TestEqual(TEXT("stesso hash (contenuto)"), Stroke->ComputeHash(), Direct->ComputeHash());

	bool bSorted = true;
	for (int32 I = 1; I < Stroke->Cells.Num(); ++I)
	{
		bSorted = bSorted && URTHexLibrary::StableLess(Stroke->Cells[I - 1].Id, Stroke->Cells[I].Id);
	}
	TestTrue(TEXT("celle ordinate dopo EndStroke"), bSorted);

	// EraseCellInStroke: assente -> false; presente -> true + rimossa.
	TestFalse(TEXT("erase assente = false"), Stroke->EraseCellInStroke(FRTCellId(9, 9, 0)));
	TestTrue(TEXT("erase presente = true"), Stroke->EraseCellInStroke(FRTCellId(0, 0, 0)));
	TestFalse(TEXT("cella rimossa"), Stroke->ContainsCell(FRTCellId(0, 0, 0)));
	return true;
}
```

- [ ] **Step 2: Build → verificare che fallisce a compilazione**

Run: `"D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -project="D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -waitmutex`
Expected: **FAIL** — «`BeginStroke` is not a member of `URTHexMapAsset`».

- [ ] **Step 3: Dichiarare le primitive sull'asset**

In `Source/RefactorTactics/Map/RTHexMapAsset.h`, subito dopo la dichiarazione di `AddOrUpdateCell` (riga ~49), aggiungere:
```cpp
	/** Inizia una pennellata: Modify() una volta (stato pre-pennellata per l'undo). Nessuna transazione (la apre il caller). */
	void BeginStroke();

	/** Dipinge una cella dentro una pennellata: crea/aggiorna (preserva Height/LOS). Vero se applicata. Niente Sort/Dirty/refresh. */
	bool PaintCellInStroke(const FRTCellId& Id, ERTHexSurface Surface, int32 MoveCost, bool bBlocksMovement);

	/** Cancella una cella dentro una pennellata. Vero se esisteva ed è stata rimossa. Niente Sort/Dirty/refresh. */
	bool EraseCellInStroke(const FRTCellId& Id);

	/** Chiude la pennellata: SortCells + MarkPackageDirty. */
	void EndStroke();
```

- [ ] **Step 4: Implementare le primitive**

In `Source/RefactorTactics/Map/RTHexMapAsset.cpp`, dopo l'implementazione di `AddOrUpdateCell` (o `ApplyBrush`), aggiungere:
```cpp
void URTHexMapAsset::BeginStroke()
{
	Modify();
}

bool URTHexMapAsset::PaintCellInStroke(const FRTCellId& Id, ERTHexSurface Surface, int32 MoveCost, bool bBlocksMovement)
{
	AddOrUpdateCell(URTHexMapAsset::ApplyBrush(FindCell(Id), Id, Surface, MoveCost, bBlocksMovement));
	return true;
}

bool URTHexMapAsset::EraseCellInStroke(const FRTCellId& Id)
{
	if (!ContainsCell(Id))
	{
		return false;
	}
	return RemoveCell(Id);
}

void URTHexMapAsset::EndStroke()
{
	SortCells();
	MarkPackageDirty();
}
```

- [ ] **Step 5: Build + eseguire il test → passa**

Run build (Step 2) → `Result: Succeeded`. Poi:
`"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -ExecCmds="Automation RunTests RefactorTactics.HexMap.StrokeEquivalence; Quit" -unattended -nopause -nosplash -nullrhi`
Expected: `RefactorTactics.HexMap.StrokeEquivalence` **Success**, 0 Fail.

- [ ] **Step 6: Ri-esprimere `PaintCellData`/`EraseCell` sull'actor (comportamento invariato)**

In `Source/RefactorTactics/Map/RTHexMapActor.cpp`, sostituire i corpi ATTUALI di `PaintCellData` (righe ~169-187) ed `EraseCell` (righe ~189-213) con:
```cpp
void ARTHexMapActor::PaintCellData(const FRTCellId& Id, ERTHexSurface Surface, int32 MoveCost, bool bBlocksMovement)
{
	if (!MapAsset)
	{
		UE_LOG(LogRT, Warning, TEXT("[HexMap] Nessun MapAsset assegnato."));
		return;
	}
#if WITH_EDITOR
	const FScopedTransaction Transaction(LOCTEXT("HexPaint", "Hex: Paint Cell"));
#endif
	MapAsset->BeginStroke();
	MapAsset->PaintCellInStroke(Id, Surface, MoveCost, bBlocksMovement);
	MapAsset->EndStroke();
	RebuildInstances();
	UE_LOG(LogRT, Log, TEXT("[HexMap] Paint su %s (superficie %d, costo %d, blocca=%d)."),
		*Id.ToString(), static_cast<int32>(Surface), MoveCost, bBlocksMovement ? 1 : 0);
}

bool ARTHexMapActor::EraseCell(const FRTCellId& Id)
{
	if (!MapAsset)
	{
		UE_LOG(LogRT, Warning, TEXT("[HexMap] Nessun MapAsset assegnato."));
		return false;
	}
	if (!MapAsset->ContainsCell(Id))
	{
		// Niente da cancellare: nessuna transazione no-op sullo stack di Undo.
		return false;
	}
#if WITH_EDITOR
	const FScopedTransaction Transaction(LOCTEXT("HexErase", "Hex: Erase Cell"));
#endif
	MapAsset->BeginStroke();
	const bool bRemoved = MapAsset->EraseCellInStroke(Id);
	MapAsset->EndStroke();
	RebuildInstances();
	UE_LOG(LogRT, Log, TEXT("[HexMap] Erase %s: %s."), *Id.ToString(), bRemoved ? TEXT("rimossa") : TEXT("assente"));
	return bRemoved;
}
```
> Comportamento invariato: stessa guardia `!MapAsset`, stessa early-out `!ContainsCell` (evita transazione no-op), stessa `FScopedTransaction` `#if WITH_EDITOR`, stessa sequenza `Modify`(BeginStroke)/mutazione/`SortCells`+`MarkPackageDirty`(EndStroke)/`RebuildInstances`/log. `EndStroke` chiama `SortCells` anche su erase: no-op su array già ordinato (la rimozione mantiene l'ordine).

- [ ] **Step 7: Build + suite completa → verde**

Run build (Step 2) → `Result: Succeeded`. Poi:
`"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -ExecCmds="Automation RunTests RefactorTactics; Quit" -unattended -nopause -nosplash -nullrhi`
Expected: tutti i test hex Success, **0 Fail** (incluso il nuovo `StrokeEquivalence`).

- [ ] **Step 8: Commit**

```bash
git checkout -- RefactorTactics.uproject   # se risporcato
git add Source/RefactorTactics/Map/RTHexMapAsset.h Source/RefactorTactics/Map/RTHexMapAsset.cpp \
        Source/RefactorTactics/Map/RTHexMapActor.cpp \
        Source/RefactorTactics/Tests/RTHexMapTests.cpp
git commit -m "refactor(hex): H5c.3a - primitive di stroke su URTHexMapAsset + ri-espressione PaintCellData/EraseCell"
```

---

## Task 2 (H5c.3b): `URTHexPaintTool` → `UClickDragTool` (drag-brush)

Evolve il tool Paint: base `UClickDragTool`, transazione per pennellata, dedup celle. Click singolo = pennellata di 1 cella. **Editor-bound**: verifiche → voci PIE nella lista.

**Files:**
- Modify: `Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.h` (base + override + stato)
- Modify: `Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.cpp` (drag handlers + stroke)
- Modify: `docs/design/test-manuali-pie.md` (voci PIE-HEX-MODE-I/J)
- Modify: `docs/design/hex-map-roadmap.md` (riga H5: H5c.3 fatta)

**Interfaces:**
- Consumes: `UClickDragTool` (`CanBeginClickDragSequence`/`OnClickPress`/`OnClickDrag`/`OnClickRelease`/`OnTerminateDragSequence`/`Setup`/`Shutdown`); `FInputRayHit`; `RTHexEditor::{FindTargetMapActor,ResolveClickedCell,DrawHexMarker}`; `URTHexMapAsset::{BeginStroke,PaintCellInStroke,EraseCellInStroke,EndStroke,FindCell,HexSize,LayerHeight}`; `ARTHexMapActor::{MapAsset,ActiveLayer,RebuildInstances,GetActorLocation}`.

- [ ] **Step 1: Nota testing (nessun test headless)**

Il drag/stroke è editor-bound (input viewport): non unit-testabile headless. La logica d'asset (`StrokeEquivalence`) è coperta dal Task 1. Verifica in editor (PIE-HEX-MODE-I/J, voci in lista allo Step 5). *(Dichiarazione esplicita DoD.)*

- [ ] **Step 2: Header del tool (base UClickDragTool)**

Sostituire l'INTERO `Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.h` con:
```cpp
#pragma once

#include "BaseTools/ClickDragTool.h"
#include "Map/RTHexCellData.h" // FRTCellId + ERTHexSurface (brush)
#include "ScopedTransaction.h" // TUniquePtr<FScopedTransaction> membro
#include "RTHexPaintTool.generated.h"

class ARTHexMapActor;
class IToolsContextRenderAPI;

/** Operazione del pennello: dipinge (crea/aggiorna) o cancella (rimuove) la cella. */
UENUM()
enum class ERTHexPaintOp : uint8
{
	Paint,
	Erase
};

/** Factory del tool di paint. */
UCLASS()
class URTHexPaintToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()
public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override { return true; }
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
};

/** Palette minima del pennello (invariata rispetto a H5c.1). */
UCLASS(Transient)
class URTHexPaintToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Hex|Pennello")
	ERTHexPaintOp Operation = ERTHexPaintOp::Paint;

	UPROPERTY(EditAnywhere, Category = "Hex|Pennello")
	ERTHexSurface Surface = ERTHexSurface::Normal;

	UPROPERTY(EditAnywhere, Category = "Hex|Pennello", meta = (ClampMin = "0"))
	int32 MoveCost = 1;

	UPROPERTY(EditAnywhere, Category = "Hex|Pennello")
	bool bBlocksMovement = false;

	UPROPERTY(VisibleAnywhere, Category = "Hex")
	int32 ActiveLayer = 0;

	UPROPERTY(VisibleAnywhere, Category = "Hex|Ultimo")
	FRTCellId LastCell;

	UPROPERTY(VisibleAnywhere, Category = "Hex|Ultimo")
	bool bLastExisted = false;
};

/**
 * Drag-brush: dipinge/cancella celle cliccando o trascinando nel viewport. Una pennellata (press->release) = una
 * transazione (un Undo). Click singolo = pennellata di 1 cella. Scrive via le primitive di stroke di URTHexMapAsset.
 */
UCLASS()
class URTHexPaintTool : public UClickDragTool
{
	GENERATED_BODY()
public:
	virtual void SetWorld(UWorld* World);
	virtual void Setup() override;
	virtual FInputRayHit CanBeginClickDragSequence(const FInputDeviceRay& PressPos) override;
	virtual void OnClickPress(const FInputDeviceRay& PressPos) override;
	virtual void OnClickDrag(const FInputDeviceRay& DragPos) override;
	virtual void OnClickRelease(const FInputDeviceRay& ReleasePos) override;
	virtual void OnTerminateDragSequence() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;

protected:
	void ApplyOne(ARTHexMapActor* Actor, const FRTCellId& Cell, const FVector& Center);
	void EndStrokeIfActive();

	UPROPERTY()
	TObjectPtr<URTHexPaintToolProperties> Properties;

	UPROPERTY()
	TObjectPtr<ARTHexMapActor> TargetActor;

	UWorld* TargetWorld = nullptr;

	TUniquePtr<FScopedTransaction> StrokeTransaction;
	bool bStrokeActive = false;
	TSet<FRTCellId> PaintedThisStroke;

	bool bHasMarker = false;
	FVector MarkerCenter = FVector::ZeroVector;
	float MarkerRadius = 50.f;
	FColor MarkerColor = FColor::Green;
};
```

- [ ] **Step 3: Implementazione del tool**

Sostituire l'INTERO `Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.cpp` con:
```cpp
#include "Tools/RTHexPaintTool.h"
#include "RTHexEditorClick.h"
#include "InteractiveToolManager.h"
#include "ToolContextInterfaces.h"
#include "InputState.h" // FInputDeviceRay / FInputRayHit
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"

#define LOCTEXT_NAMESPACE "URTHexPaintTool"

UInteractiveTool* URTHexPaintToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	URTHexPaintTool* NewTool = NewObject<URTHexPaintTool>(SceneState.ToolManager);
	NewTool->SetWorld(SceneState.World);
	return NewTool;
}

void URTHexPaintTool::SetWorld(UWorld* World)
{
	TargetWorld = World;
}

void URTHexPaintTool::Setup()
{
	UClickDragTool::Setup();
	Properties = NewObject<URTHexPaintToolProperties>(this);
	AddToolPropertySource(Properties);
}

FInputRayHit URTHexPaintTool::CanBeginClickDragSequence(const FInputDeviceRay& PressPos)
{
	// Accetta ogni click nel viewport (la cella si risolve in OnClickPress). bHit=true via profondità.
	return FInputRayHit(TNumericLimits<double>::Max());
}

void URTHexPaintTool::ApplyOne(ARTHexMapActor* Actor, const FRTCellId& Cell, const FVector& Center)
{
	URTHexMapAsset* Map = Actor->MapAsset; // il caller garantisce Actor && Map non nulli
	if (Properties->Operation == ERTHexPaintOp::Paint)
	{
		Properties->bLastExisted = (Map->FindCell(Cell) != nullptr); // prima della mutazione
		Map->PaintCellInStroke(Cell, Properties->Surface, Properties->MoveCost, Properties->bBlocksMovement);
		MarkerColor = FColor::Green;
	}
	else
	{
		Properties->bLastExisted = Map->EraseCellInStroke(Cell);
		MarkerColor = FColor::Red;
	}
	PaintedThisStroke.Add(Cell);
	Properties->LastCell = Cell;
	Properties->ActiveLayer = Actor->ActiveLayer;
	MarkerCenter = Center;
	MarkerRadius = Map->HexSize * 0.9f;
	bHasMarker = true;
}

void URTHexPaintTool::OnClickPress(const FInputDeviceRay& PressPos)
{
	bHasMarker = false;
	if (!Properties) { return; }

	ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld);
	if (!Actor || !Actor->MapAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HexMode] Paint: nessun ARTHexMapActor con MapAsset (nessuna pennellata)."));
		return; // niente stroke senza asset (M3)
	}

	FRTCellId Cell;
	FVector Center;
	if (!RTHexEditor::ResolveClickedCell(TargetWorld, Actor, PressPos, Cell, Center)) { return; }

	StrokeTransaction = MakeUnique<FScopedTransaction>(LOCTEXT("HexBrushStroke", "Hex: Brush Stroke"));
	TargetActor = Actor;
	Actor->MapAsset->BeginStroke();
	bStrokeActive = true;
	PaintedThisStroke.Reset();

	ApplyOne(Actor, Cell, Center);
	Actor->RebuildInstances();
}

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

void URTHexPaintTool::OnClickRelease(const FInputDeviceRay& ReleasePos)
{
	EndStrokeIfActive();
}

void URTHexPaintTool::OnTerminateDragSequence()
{
	EndStrokeIfActive();
}

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

void URTHexPaintTool::Shutdown(EToolShutdownType ShutdownType)
{
	EndStrokeIfActive(); // chiude uno stroke aperto a un cambio-tool/uscita mode a metà drag (S1)
	UClickDragTool::Shutdown(ShutdownType);
}

void URTHexPaintTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (!bHasMarker || !RenderAPI) { return; }
	RTHexEditor::DrawHexMarker(RenderAPI->GetPrimitiveDrawInterface(), MarkerCenter, MarkerRadius, MarkerColor);
}

#undef LOCTEXT_NAMESPACE
```

- [ ] **Step 4: Build del target Editor (editor chiuso)**

Run: `"D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -project="D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -waitmutex`
Expected: `Result: Succeeded`. Poi (sanity, nessun cambio runtime in 3b): run suite `RefactorTactics` → **0 Fail**.

- [ ] **Step 5: Voci PIE nella lista (editor-bound)**

Aggiungere a `docs/design/test-manuali-pie.md`:
```markdown
| **PIE-HEX-MODE-I** | Drag-paint (H5c.3b) | mode Hex Map, tool Paint (`Operation=Paint`), `ARTHexMapActor` con `MapAsset` | Tenere premuto e trascinare dipinge più celle in una pennellata (dedup: ripassare non ridipinge); **un** Ctrl+Z annulla l'intera pennellata; click singolo = 1 cella (PIE-C invariato) | ⏳ (branch `feat/hex-grid`, H5c.3b) |
| **PIE-HEX-MODE-J** | Drag-erase (H5c.3b) | mode Hex Map, tool Paint (`Operation=Erase`) | Trascinare cancella più celle in una pennellata; un Undo le ripristina tutte; cambiare tool a metà drag non lascia transazioni aperte | ⏳ (branch `feat/hex-grid`, H5c.3b) |
```

- [ ] **Step 6: Aggiornare la roadmap (riga H5)**

In `docs/design/hex-map-roadmap.md`, nella cella Stato della riga **H5**, aggiungere in coda: `H5c.3: drag-brush - URTHexPaintTool passa a UClickDragTool (pennellata press->release, dedup, una FScopedTransaction/Undo per pennellata); primitive di stroke su URTHexMapAsset (BeginStroke/PaintCellInStroke/EraseCellInStroke/EndStroke, test RefactorTactics.HexMap.StrokeEquivalence); PaintCellData/EraseCell ri-espressi. Verifica editor PIE-HEX-MODE-I/J aperta.`

- [ ] **Step 7: Commit**

```bash
git checkout -- RefactorTactics.uproject   # se risporcato
git add Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.h \
        Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.cpp \
        docs/design/test-manuali-pie.md docs/design/hex-map-roadmap.md
git commit -m "feat(hex): H5c.3b - drag-brush (URTHexPaintTool -> UClickDragTool, 1 Undo per pennellata)"
```

---

## Self-Review (eseguita)

- **Copertura spec**: §3.1 primitive asset → T1 Step 3-4; ri-espressione PaintCellData/EraseCell → T1 Step 6; §3.2 tool (Setup/CanBegin/Press/Drag/Release/Terminate/Shutdown/ApplyOne/stato) → T2 Step 2-3; §6 test headless → T1 Step 1-5,7; PIE → T2 Step 5; roadmap → T2 Step 6. Nessun gap.
- **Placeholder**: nessuno. `CanBeginClickDragSequence` restituisce `FInputRayHit(TNumericLimits<double>::Max())` (bHit=true).
- **Consistenza firme**: `BeginStroke/PaintCellInStroke/EraseCellInStroke/EndStroke` (asset) coerenti tra dichiarazione (T1 Step 3), uso nella ri-espressione (T1 Step 6) e uso nel tool (T2 Step 3). `ResolveClickedCell(World,Actor,Pos,Cell,Center)` a 5 arg. `Shutdown(EToolShutdownType)`, `OnClickPress/Drag/Release`, `OnTerminateDragSequence`, `CanBeginClickDragSequence` come da header UE 5.8.
- **Behavior-preservation**: PaintCellData/EraseCell ri-espressi mantengono guardie, transazione, sequenza e log (M2 guard in cima preservato).

## Rischi noti

- **Cambio base del Paint** (H5c.1): il click resta caso degenere → riverifica PIE-HEX-MODE-C/D (voci già in lista).
- **Transazione**: `TUniquePtr<FScopedTransaction>` chiusa in release/terminate **e** Shutdown (S1); `bStrokeActive` (M3) evita EndStroke su stroke mai partito.
- **Modulo editor**: rebuild a editor chiuso.
