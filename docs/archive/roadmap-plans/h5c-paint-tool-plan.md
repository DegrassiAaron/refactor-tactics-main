# H5c prima fetta — Tool "Paint-a-click" (Paint + Erase) — Implementation Plan

> ## 📦 `DELIVERED PLAN` — PIANO GIA' ESEGUITO, NON NORMATIVO
>
> Il piano di esecuzione, gia' eseguito. **Il corpo qui sotto non va aggiornato**: comandi, nomi di branch e percorsi sono quelli di allora, e
> correggerli falsificherebbe la storia invece di renderla utile.
>
> Stato corrente: [`roadmap-checkpoint.md`](../roadmap-checkpoint.md) · indice dei documenti correnti:
> [`../../README.md`](../../README.md). Banner aggiunto il 2026-08-08.

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or
> superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Dipingere o cancellare una cella della mappa esagonale cliccandola nel viewport, senza digitare coordinate nel pannello Details.

**Architecture:** Un nuovo tool ITF `URTHexPaintTool : USingleClickTool` nel modulo editor riusa la pipeline `raycast→WorldToAxial` (estratta in helper condivisi con `URTHexSelectTool`) e scrive via l'API dell'`ARTHexMapActor` (`PaintCellData`/`EraseCell`, estratte dal `PaintTargetCell` esistente). La logica di merge del pennello diventa una funzione pura statica testabile headless (`URTHexMapAsset::ApplyBrush`).

**Tech Stack:** UE 5.8.1 C++; moduli `InteractiveToolsFramework`, `EditorInteractiveToolsFramework`, `UnrealEd`, `EditorFramework`, `Slate/SlateCore`, `LevelEditor`. Scaffold di riferimento su disco: `D:/EpicGames/UE_5.8/Engine/Plugins/Editor/SampleToolsEditorMode`. Spec: [`h5c-paint-tool-spec.md`](h5c-paint-tool-spec.md).

## Global Constraints

- Motore **UE 5.8.1**; `RefactorTactics.uproject` `EngineAssociation` deve restare `"5.8"` (ripristinare con `git checkout -- RefactorTactics.uproject` se l'editor lo risporca a GUID).
- Prefissi classi **`RT`/`URT`**; documentazione in `docs/`.
- **L'editor non decide gameplay** (invariante #1): il tool scrive solo dati d'asset, e solo via `Modify()`/`FScopedTransaction` (Undo/Redo).
- **Dipendenze editor confinate** nel modulo `RefactorTacticsEditor`; il runtime packaged non deve includerle. Il refactor runtime (`PaintCellData`/`EraseCell`/`ApplyBrush`) non aggiunge dipendenze editor (la `FScopedTransaction` è già `#if WITH_EDITOR` in `RTHexMapActor.cpp`).
- **Determinismo** (#4): nessuna logica di turno toccata; `URTHexLibrary::WorldToAxial` deterministico (arrotondamento cubico, interi).
- **No duplicazione** (CLAUDE.md): SelectTool e PaintTool condividono la pipeline click→cella; `PaintTargetCell` e il tool condividono la scrittura.
- Un **commit per task**; build Editor **verde** prima di dichiarare fatto; verifiche editor-bound dichiarate come **manuali** (non «dovrebbe funzionare»).
- **Editor CHIUSO** durante il rebuild: un modulo editor modificato non è ricaricabile via Live Coding → chiudere l'editor, rebuild, riaprire.
- Build (editor chiuso): `"D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -project="D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -waitmutex` → cercare `Result: Succeeded`.
- Test headless (editor chiuso): `"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -ExecCmds="Automation RunTests RefactorTactics; Quit" -unattended -nopause -nosplash -nullrhi` → risultati in `Saved/Logs/RefactorTactics.log` (cercare righe `Result=` / `... Test Completed. Result={Success}` e assenza di `Fail`).

---

## Task 1 (H5c.1a): Refactor DRY + funzione pura del pennello (+ test)

Estrae la scrittura cella riusabile sull'actor, la logica di merge come funzione pura testabile, e gli helper editor condivisi; rifattorizza `URTHexSelectTool` per usarli (comportamento invariato). **Nessun tool nuovo qui.** Deliverable: build Editor verde + tutti i test hex verdi (incluso il nuovo) + SelectTool identico all'uso.

**Files:**
- Modify: `Source/RefactorTactics/Map/RTHexMapAsset.h` (dichiara `static ApplyBrush`)
- Modify: `Source/RefactorTactics/Map/RTHexMapAsset.cpp` (implementa `ApplyBrush`)
- Modify: `Source/RefactorTactics/Map/RTHexMapActor.h` (dichiara `PaintCellData`, `EraseCell`)
- Modify: `Source/RefactorTactics/Map/RTHexMapActor.cpp` (implementa `PaintCellData`/`EraseCell`; `PaintTargetCell` diventa wrapper)
- Create: `Source/RefactorTacticsEditor/Private/RTHexEditorClick.h`
- Create: `Source/RefactorTacticsEditor/Private/RTHexEditorClick.cpp`
- Modify: `Source/RefactorTacticsEditor/Private/Tools/RTHexSelectTool.cpp` (usa gli helper condivisi)
- Test: `Source/RefactorTactics/Tests/RTHexMapTests.cpp` (nuovo test `ApplyBrushMerge`)

**Interfaces:**
- Produces:
  - `static FRTHexCellData URTHexMapAsset::ApplyBrush(const FRTHexCellData* Existing, const FRTCellId& Id, ERTHexSurface Surface, int32 MoveCost, bool bBlocksMovement)`
  - `void ARTHexMapActor::PaintCellData(const FRTCellId& Id, ERTHexSurface Surface, int32 MoveCost, bool bBlocksMovement)`
  - `bool ARTHexMapActor::EraseCell(const FRTCellId& Id)`
  - `namespace RTHexEditor`: `ARTHexMapActor* FindTargetMapActor(UWorld*)`, `bool ResolveClickedCell(UWorld*, ARTHexMapActor*, const FInputDeviceRay&, FRTCellId& OutCell, FVector& OutCenter)`, `void DrawHexMarker(FPrimitiveDrawInterface*, const FVector& Center, float Radius, const FColor&)`
- Consumes (esistenti): `URTHexMapAsset::{FindCell,AddOrUpdateCell,RemoveCell,SortCells,HexSize,LayerHeight}`, `ARTHexMapActor::{MapAsset,HexSize,LayerHeight,ActiveLayer,GetActorLocation,RebuildInstances}`, `URTHexLibrary::{WorldToAxial,AxialToWorld}`.

- [ ] **Step 1: Scrivere il test che fallisce (`ApplyBrushMerge`)**

In `Source/RefactorTactics/Tests/RTHexMapTests.cpp`, prima della riga `#endif // WITH_DEV_AUTOMATION_TESTS`, aggiungere:
```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexApplyBrushTest,
	"RefactorTactics.HexMap.ApplyBrushMerge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexApplyBrushTest::RunTest(const FString&)
{
	const FRTCellId Id(2, -1, 1);

	// Cella nuova (Existing = nullptr): default Height=0, LOS=false; applica surface/cost/block.
	const FRTHexCellData New = URTHexMapAsset::ApplyBrush(nullptr, Id, ERTHexSurface::Water, 3, true);
	TestTrue(TEXT("Id impostato"), New.Id == Id);
	TestTrue(TEXT("Surface applicata"), New.Surface == ERTHexSurface::Water);
	TestEqual(TEXT("MoveCost applicato"), New.MoveCost, 3);
	TestTrue(TEXT("bBlocksMovement applicato"), New.bBlocksMovement);
	TestEqual(TEXT("Height default 0"), New.Height, 0);
	TestFalse(TEXT("LOS default false"), New.bBlocksLineOfSight);

	// Cella esistente con Height=3 e LOS=true: paint cambia surface/cost/block ma PRESERVA Height e LOS.
	FRTHexCellData Existing(Id);
	Existing.Height = 3;
	Existing.bBlocksLineOfSight = true;
	Existing.Surface = ERTHexSurface::Normal;
	Existing.MoveCost = 1;
	Existing.bBlocksMovement = false;
	const FRTHexCellData Painted = URTHexMapAsset::ApplyBrush(&Existing, Id, ERTHexSurface::Fire, 5, true);
	TestTrue(TEXT("Surface aggiornata"), Painted.Surface == ERTHexSurface::Fire);
	TestEqual(TEXT("MoveCost aggiornato"), Painted.MoveCost, 5);
	TestTrue(TEXT("bBlocksMovement aggiornato"), Painted.bBlocksMovement);
	TestEqual(TEXT("Height preservato"), Painted.Height, 3);
	TestTrue(TEXT("LOS preservato"), Painted.bBlocksLineOfSight);
	return true;
}
```
*(Riusa il file esistente: nessun nuovo helper in namespace anonimo → nessuna collisione unity-build.)*

- [ ] **Step 2: Build → verificare che fallisce a compilazione**

Run: `"D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -project="D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -waitmutex`
Expected: **FAIL** — errore di compilazione «`ApplyBrush` is not a member of `URTHexMapAsset`» (la funzione non esiste ancora).

- [ ] **Step 3: Dichiarare `ApplyBrush` sull'asset**

In `Source/RefactorTactics/Map/RTHexMapAsset.h`, subito dopo la dichiarazione di `AddOrUpdateCell` (riga ~49), aggiungere:
```cpp
	/**
	 * Logica pura del 'pennello': se Existing != nullptr parte da esso (PRESERVA Height e bBlocksLineOfSight),
	 * altrimenti da una cella nuova con l'Id dato; applica Surface/MoveCost/bBlocksMovement. Non muta l'asset.
	 */
	static FRTHexCellData ApplyBrush(const FRTHexCellData* Existing, const FRTCellId& Id,
		ERTHexSurface Surface, int32 MoveCost, bool bBlocksMovement);
```

- [ ] **Step 4: Implementare `ApplyBrush`**

In `Source/RefactorTactics/Map/RTHexMapAsset.cpp`, aggiungere (in fondo, prima di eventuali `#undef`/blocchi finali, o dopo `AddOrUpdateCell`):
```cpp
FRTHexCellData URTHexMapAsset::ApplyBrush(const FRTHexCellData* Existing, const FRTCellId& Id,
	ERTHexSurface Surface, int32 MoveCost, bool bBlocksMovement)
{
	FRTHexCellData Cell = Existing ? *Existing : FRTHexCellData(Id);
	Cell.Id = Id; // garantisce l'Id (anche se Existing arrivasse con Id diverso)
	Cell.Surface = Surface;
	Cell.MoveCost = MoveCost;
	Cell.bBlocksMovement = bBlocksMovement;
	return Cell;
}
```

- [ ] **Step 5: Build + eseguire i test → il nuovo test passa**

Run build (comando dello Step 2) → `Result: Succeeded`. Poi:
`"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -ExecCmds="Automation RunTests RefactorTactics.HexMap.ApplyBrushMerge; Quit" -unattended -nopause -nosplash -nullrhi`
Expected: in `Saved/Logs/RefactorTactics.log` il test `RefactorTactics.HexMap.ApplyBrushMerge` risulta **Success**, 0 Fail.

- [ ] **Step 6: Estrarre `PaintCellData`/`EraseCell` sull'actor**

In `Source/RefactorTactics/Map/RTHexMapActor.h`, subito dopo la dichiarazione di `PaintTargetCell()` (riga ~93), aggiungere:
```cpp
	/** Scrive Surface/MoveCost/bBlocksMovement sulla cella Id (la crea se assente, preserva Height/LOS). Annullabile. */
	void PaintCellData(const FRTCellId& Id, ERTHexSurface Surface, int32 MoveCost, bool bBlocksMovement);

	/** Rimuove la cella Id dall'asset. Vero se esisteva. Annullabile. */
	bool EraseCell(const FRTCellId& Id);
```

In `Source/RefactorTactics/Map/RTHexMapActor.cpp`, sostituire l'intero corpo attuale di `PaintTargetCell()` (righe ~164-190) con il wrapper + le due nuove funzioni:
```cpp
void ARTHexMapActor::PaintTargetCell()
{
	PaintCellData(PaintCellTarget, PaintSurface, PaintMoveCost, bPaintBlocksMovement);
}

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
	MapAsset->Modify();
	const FRTHexCellData Cell = URTHexMapAsset::ApplyBrush(MapAsset->FindCell(Id), Id, Surface, MoveCost, bBlocksMovement);
	MapAsset->AddOrUpdateCell(Cell);
	MapAsset->SortCells();
	MapAsset->MarkPackageDirty();
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
#if WITH_EDITOR
	const FScopedTransaction Transaction(LOCTEXT("HexErase", "Hex: Erase Cell"));
#endif
	MapAsset->Modify();
	const bool bRemoved = MapAsset->RemoveCell(Id);
	if (bRemoved)
	{
		MapAsset->MarkPackageDirty();
		RebuildInstances();
	}
	UE_LOG(LogRT, Log, TEXT("[HexMap] Erase %s: %s."), *Id.ToString(), bRemoved ? TEXT("rimossa") : TEXT("assente"));
	return bRemoved;
}
```
*(Nota: `MapAsset->FindCell(Id)` è dereferenziata dentro `ApplyBrush` che ritorna una copia PRIMA che `AddOrUpdateCell` mostri l'array: nessun dangling pointer.)*

- [ ] **Step 7: Creare gli helper editor condivisi**

`Source/RefactorTacticsEditor/Private/RTHexEditorClick.h`:
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"

class UWorld;
class ARTHexMapActor;
class FPrimitiveDrawInterface;
struct FInputDeviceRay;

/** Helper condivisi tra i tool click dell'Editor Mode hex (SelectTool, PaintTool, ...). */
namespace RTHexEditor
{
	/** ARTHexMapActor bersaglio: selezionato nel Level Editor; altrimenti l'unico presente; altrimenti nullptr (ambiguo). */
	ARTHexMapActor* FindTargetMapActor(UWorld* World);

	/** Risolve la cella cliccata sul layer attivo dell'actor (raycast ISM del target, fallback piano del layer).
	 *  Ritorna false se Actor è nullptr. OutCenter = centro-mondo della cella (per il marker). */
	bool ResolveClickedCell(UWorld* World, ARTHexMapActor* Actor, const FInputDeviceRay& ClickPos,
		FRTCellId& OutCell, FVector& OutCenter);

	/** Disegna un esagono pointy-top (marker) sul PDI. */
	void DrawHexMarker(FPrimitiveDrawInterface* PDI, const FVector& Center, float Radius, const FColor& Color);
}
```

`Source/RefactorTacticsEditor/Private/RTHexEditorClick.cpp`:
```cpp
#include "RTHexEditorClick.h"
#include "PrimitiveDrawingUtils.h" // FPrimitiveDrawInterface / SDPG_*
#include "InputState.h"            // FInputDeviceRay
#include "Engine/World.h"
#include "Engine/HitResult.h"
#include "EngineUtils.h"           // TActorIterator
#include "Editor.h"                // GEditor
#include "Selection.h"             // USelection
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"

namespace RTHexEditor
{
ARTHexMapActor* FindTargetMapActor(UWorld* World)
{
	// Preferenza: un ARTHexMapActor selezionato nel Level Editor.
	if (GEditor)
	{
		if (USelection* Sel = GEditor->GetSelectedActors())
		{
			for (FSelectionIterator It(*Sel); It; ++It)
			{
				if (ARTHexMapActor* A = Cast<ARTHexMapActor>(*It))
				{
					return A;
				}
			}
		}
	}
	// Fallback: l'unico ARTHexMapActor nel mondo.
	ARTHexMapActor* Found = nullptr;
	if (World)
	{
		for (TActorIterator<ARTHexMapActor> It(World); It; ++It)
		{
			if (Found) { return nullptr; } // piu' di uno e nessuno selezionato: ambiguo -> nessuna azione
			Found = *It;
		}
	}
	return Found;
}

bool ResolveClickedCell(UWorld* World, ARTHexMapActor* Actor, const FInputDeviceRay& ClickPos,
	FRTCellId& OutCell, FVector& OutCenter)
{
	if (!Actor) { return false; }

	const URTHexMapAsset* Map = Actor->MapAsset;
	const float HexSize = Map ? Map->HexSize : Actor->HexSize;
	const float LayerH = Map ? Map->LayerHeight : Actor->LayerHeight;
	const FVector Origin = Actor->GetActorLocation();
	const int32 Layer = Actor->ActiveLayer;

	// Punto-mondo del click: colpo sull'ISM del TARGET se c'e', altrimenti intersezione col piano del layer attivo.
	FVector HitPoint;
	const FVector RayStart = ClickPos.WorldRay.Origin;
	const FVector RayEnd = ClickPos.WorldRay.PointAt(999999.0);
	FHitResult Result;
	const bool bHitTarget = World
		&& World->LineTraceSingleByObjectType(Result, RayStart, RayEnd,
			FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects))
		&& (Result.GetActor() == Actor); // solo l'ISM del target: ignora unita'/ostacoli tra camera e mappa
	if (bHitTarget)
	{
		HitPoint = Result.ImpactPoint;
	}
	else
	{
		const double PlaneZ = Origin.Z + static_cast<double>(Layer) * static_cast<double>(LayerH);
		const FPlane LayerPlane(FVector(0, 0, PlaneZ), FVector(0, 0, 1));
		HitPoint = FMath::RayPlaneIntersection(ClickPos.WorldRay.Origin, ClickPos.WorldRay.Direction, LayerPlane);
	}

	OutCell = URTHexLibrary::WorldToAxial(HitPoint, Origin, HexSize, Layer);
	OutCenter = URTHexLibrary::AxialToWorld(OutCell, Origin, HexSize, LayerH);
	return true;
}

void DrawHexMarker(FPrimitiveDrawInterface* PDI, const FVector& Center, float Radius, const FColor& Color)
{
	if (!PDI) { return; }
	// Esagono pointy-top (6 vertici) sul piano orizzontale.
	FVector Prev = FVector::ZeroVector;
	for (int32 I = 0; I <= 6; ++I)
	{
		const double Angle = PI / 180.0 * (60.0 * I - 30.0); // pointy-top: primo vertice a -30 gradi
		const FVector V = Center + FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), 2.0);
		if (I > 0)
		{
			PDI->DrawLine(Prev, V, Color, SDPG_Foreground, 2.0f);
		}
		Prev = V;
	}
}
} // namespace RTHexEditor
```

- [ ] **Step 8: Rifattorizzare `URTHexSelectTool` per usare gli helper (comportamento invariato)**

Sostituire l'INTERO contenuto di `Source/RefactorTacticsEditor/Private/Tools/RTHexSelectTool.cpp` con:
```cpp
#include "Tools/RTHexSelectTool.h"
#include "RTHexEditorClick.h"
#include "InteractiveToolManager.h"
#include "ToolContextInterfaces.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h" // FRTHexCellData (readout)

#define LOCTEXT_NAMESPACE "URTHexSelectTool"

UInteractiveTool* URTHexSelectToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	URTHexSelectTool* NewTool = NewObject<URTHexSelectTool>(SceneState.ToolManager);
	NewTool->SetWorld(SceneState.World);
	return NewTool;
}

void URTHexSelectTool::SetWorld(UWorld* World)
{
	TargetWorld = World;
}

void URTHexSelectTool::Setup()
{
	USingleClickTool::Setup();
	Properties = NewObject<URTHexSelectToolProperties>(this);
	AddToolPropertySource(Properties);
}

ARTHexMapActor* URTHexSelectTool::FindTargetMapActor() const
{
	return RTHexEditor::FindTargetMapActor(TargetWorld);
}

void URTHexSelectTool::OnClicked(const FInputDeviceRay& ClickPos)
{
	bHasSelection = false;
	if (!Properties) { return; }

	ARTHexMapActor* Actor = FindTargetMapActor();
	if (!Actor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HexMode] Nessun ARTHexMapActor bersaglio (selezionane uno se ce ne sono piu' di uno)."));
		return;
	}

	FRTCellId Cell;
	FVector Center;
	if (!RTHexEditor::ResolveClickedCell(TargetWorld, Actor, ClickPos, Cell, Center)) { return; }

	Properties->ActiveLayer = Actor->ActiveLayer;
	Properties->SelectedCell = Cell;

	// Readout dati cella: superficie/costo/blocco se la cella esiste nell'asset.
	const URTHexMapAsset* Map = Actor->MapAsset;
	const FRTHexCellData* Data = Map ? Map->FindCell(Cell) : nullptr;
	Properties->bSelectedCellExists = (Data != nullptr);
	if (Data)
	{
		Properties->Surface = Data->Surface;
		Properties->MoveCost = Data->MoveCost;
		Properties->bBlocksMovement = Data->bBlocksMovement;
	}

	SelectedWorldCenter = Center;
	MarkerRadius = (Map ? Map->HexSize : Actor->HexSize) * 0.9f;
	bHasSelection = true;

	UE_LOG(LogTemp, Log, TEXT("[HexMode] Selezione %s (esiste=%d) su layer %d."),
		*Cell.ToString(), Properties->bSelectedCellExists ? 1 : 0, Actor->ActiveLayer);
}

void URTHexSelectTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (!bHasSelection || !RenderAPI) { return; }
	RTHexEditor::DrawHexMarker(RenderAPI->GetPrimitiveDrawInterface(), SelectedWorldCenter, MarkerRadius, FColor::Yellow);
}

#undef LOCTEXT_NAMESPACE
```
*(La resa è identica: stesso raycast/fallback, stesso readout, stesso esagono giallo. L'header di SelectTool NON cambia.)*

- [ ] **Step 9: Build + eseguire l'intera suite → tutto verde**

Run build (comando Step 2) → `Result: Succeeded`. Poi:
`"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -ExecCmds="Automation RunTests RefactorTactics; Quit" -unattended -nopause -nosplash -nullrhi`
Expected: `Saved/Logs/RefactorTactics.log` mostra tutti i test hex Success, **0 Fail** (91 test attesi: 90 preesistenti + `ApplyBrushMerge`).

- [ ] **Step 10: Commit**

```bash
git checkout -- RefactorTactics.uproject   # solo se l'editor l'ha risporcato (EngineAssociation deve restare "5.8")
git add Source/RefactorTactics/Map/RTHexMapAsset.h Source/RefactorTactics/Map/RTHexMapAsset.cpp \
        Source/RefactorTactics/Map/RTHexMapActor.h Source/RefactorTactics/Map/RTHexMapActor.cpp \
        Source/RefactorTactics/Tests/RTHexMapTests.cpp \
        Source/RefactorTacticsEditor/Private/RTHexEditorClick.h \
        Source/RefactorTacticsEditor/Private/RTHexEditorClick.cpp \
        Source/RefactorTacticsEditor/Private/Tools/RTHexSelectTool.cpp
git commit -m "refactor(hex): H5c.1a - scrittura cella riusabile + ApplyBrush puro + helper click condivisi"
```

---

## Task 2 (H5c.1b): Tool "Paint-a-click" (Paint + Erase)

Aggiunge `URTHexPaintTool : USingleClickTool`: al click, `Paint` scrive superficie/costo/blocco sulla cella cliccata, `Erase` la rimuove; parametri dal property set del tool (palette), marker verde/rosso. **Editor-bound**: verifica in editor (PIE-HEX-MODE-C/D), non headless.

**Files:**
- Create: `Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.h`
- Create: `Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.cpp`
- Modify: `Source/RefactorTacticsEditor/Public/RTHexEditorModeCommands.h` (dichiara `PaintTool`)
- Modify: `Source/RefactorTacticsEditor/Private/RTHexEditorModeCommands.cpp` (registra il comando `PaintTool`)
- Modify: `Source/RefactorTacticsEditor/Private/RTHexEditorMode.cpp` (registra il ToolBuilder in `Enter()`)
- Modify: `docs/design/test-manuali-pie.md` (voci PIE-HEX-MODE-C e PIE-HEX-MODE-D)
- Modify: `docs/design/hex-map-roadmap.md` (aggiorna la riga H5 con lo stato H5c prima fetta)

**Interfaces:**
- Consumes: `RTHexEditor::{FindTargetMapActor,ResolveClickedCell,DrawHexMarker}`; `ARTHexMapActor::{PaintCellData,EraseCell,MapAsset,HexSize,ActiveLayer}`; `URTHexMapAsset::{FindCell,HexSize}`; `FRTHexEditorModeCommands::PaintTool`.
- Produces: `URTHexPaintTool`, `URTHexPaintToolBuilder`, `URTHexPaintToolProperties` (con `ERTHexPaintOp Operation`, brush `Surface/MoveCost/bBlocksMovement`, readout `ActiveLayer/LastCell/bLastExisted`).

- [ ] **Step 1: Nota di testing (nessun test headless)**

Il tool è editor-bound (input viewport, PDI, property panel): non è unit-testabile headless. La logica pura sottostante (`ApplyBrush`) è già coperta dal Task 1. Verifica in editor allo Step 6 (PIE-HEX-MODE-C/D). *(Dichiarazione esplicita DoD: parte non verificabile headless.)*

- [ ] **Step 2: Header del tool**

`Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.h`:
```cpp
#pragma once

#include "BaseTools/SingleClickTool.h"
#include "Map/RTHexCellData.h" // FRTCellId + ERTHexSurface (brush)
#include "RTHexPaintTool.generated.h"

class ARTHexMapActor;
class IToolsContextRenderAPI;

/** Operazione del pennello: dipinge (crea/aggiorna) o cancella (rimuove) la cella cliccata. */
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

/** Palette minima del pennello (property set del tool). */
UCLASS(Transient)
class URTHexPaintToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	/** Operazione: Paint (crea/aggiorna) o Erase (rimuove). */
	UPROPERTY(EditAnywhere, Category = "Hex|Pennello")
	ERTHexPaintOp Operation = ERTHexPaintOp::Paint;

	/** Superficie da applicare (modalita' Paint). */
	UPROPERTY(EditAnywhere, Category = "Hex|Pennello")
	ERTHexSurface Surface = ERTHexSurface::Normal;

	/** Costo di movimento da applicare (modalita' Paint). */
	UPROPERTY(EditAnywhere, Category = "Hex|Pennello", meta = (ClampMin = "0"))
	int32 MoveCost = 1;

	/** Blocca il movimento (modalita' Paint). */
	UPROPERTY(EditAnywhere, Category = "Hex|Pennello")
	bool bBlocksMovement = false;

	/** Layer attivo (sola lettura: rispecchia ARTHexMapActor::ActiveLayer). */
	UPROPERTY(VisibleAnywhere, Category = "Hex")
	int32 ActiveLayer = 0;

	/** Ultima cella toccata dal pennello. */
	UPROPERTY(VisibleAnywhere, Category = "Hex|Ultimo")
	FRTCellId LastCell;

	/** La cella esisteva prima dell'operazione? (Paint: gia' presente; Erase: presente e rimossa). */
	UPROPERTY(VisibleAnywhere, Category = "Hex|Ultimo")
	bool bLastExisted = false;
};

/**
 * Dipinge o cancella una cella cliccando nel viewport: ray -> punto-mondo (ISM del target o piano del layer attivo)
 * -> WorldToAxial -> PaintCellData/EraseCell sull'actor. Marker verde (paint) / rosso (erase).
 */
UCLASS()
class URTHexPaintTool : public USingleClickTool
{
	GENERATED_BODY()
public:
	virtual void SetWorld(UWorld* World);
	virtual void Setup() override;
	virtual void OnClicked(const FInputDeviceRay& ClickPos) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;

protected:
	UPROPERTY()
	TObjectPtr<URTHexPaintToolProperties> Properties;

	UWorld* TargetWorld = nullptr;

	bool bHasMarker = false;
	FVector MarkerCenter = FVector::ZeroVector;
	float MarkerRadius = 50.f;
	FColor MarkerColor = FColor::Green;
};
```

- [ ] **Step 3: Implementazione del tool**

`Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.cpp`:
```cpp
#include "Tools/RTHexPaintTool.h"
#include "RTHexEditorClick.h"
#include "InteractiveToolManager.h"
#include "ToolContextInterfaces.h"
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
	USingleClickTool::Setup();
	Properties = NewObject<URTHexPaintToolProperties>(this);
	AddToolPropertySource(Properties);
}

void URTHexPaintTool::OnClicked(const FInputDeviceRay& ClickPos)
{
	bHasMarker = false;
	if (!Properties) { return; }

	ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld);
	if (!Actor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HexMode] Nessun ARTHexMapActor bersaglio (selezionane uno se ce ne sono piu' di uno)."));
		return;
	}

	FRTCellId Cell;
	FVector Center;
	if (!RTHexEditor::ResolveClickedCell(TargetWorld, Actor, ClickPos, Cell, Center)) { return; }

	const URTHexMapAsset* Map = Actor->MapAsset;
	const bool bExisted = (Map && Map->FindCell(Cell) != nullptr);

	if (Properties->Operation == ERTHexPaintOp::Paint)
	{
		Actor->PaintCellData(Cell, Properties->Surface, Properties->MoveCost, Properties->bBlocksMovement);
		MarkerColor = FColor::Green;
	}
	else // Erase
	{
		Actor->EraseCell(Cell);
		MarkerColor = FColor::Red;
	}

	Properties->ActiveLayer = Actor->ActiveLayer;
	Properties->LastCell = Cell;
	Properties->bLastExisted = bExisted;

	MarkerCenter = Center;
	MarkerRadius = (Map ? Map->HexSize : Actor->HexSize) * 0.9f;
	bHasMarker = true;

	UE_LOG(LogTemp, Log, TEXT("[HexMode] %s su %s (esisteva=%d) layer %d."),
		Properties->Operation == ERTHexPaintOp::Paint ? TEXT("Paint") : TEXT("Erase"),
		*Cell.ToString(), bExisted ? 1 : 0, Actor->ActiveLayer);
}

void URTHexPaintTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (!bHasMarker || !RenderAPI) { return; }
	RTHexEditor::DrawHexMarker(RenderAPI->GetPrimitiveDrawInterface(), MarkerCenter, MarkerRadius, MarkerColor);
}

#undef LOCTEXT_NAMESPACE
```

- [ ] **Step 4: Dichiarare e registrare il comando `PaintTool`**

In `Source/RefactorTacticsEditor/Public/RTHexEditorModeCommands.h`, dopo la riga `TSharedPtr<FUICommandInfo> SelectTool;` (riga ~16), aggiungere:
```cpp
	/** Tool di paint/erase a click (H5c). */
	TSharedPtr<FUICommandInfo> PaintTool;
```

In `Source/RefactorTacticsEditor/Private/RTHexEditorModeCommands.cpp`, in fondo a `RegisterCommands()` (dopo `ToolCommands.Add(SelectTool);`), aggiungere:
```cpp
	UI_COMMAND(PaintTool, "Paint", "Dipinge o cancella una cella cliccando nel viewport (layer attivo)",
		EUserInterfaceActionType::ToggleButton, FInputChord());
	ToolCommands.Add(PaintTool);
```

- [ ] **Step 5: Registrare il ToolBuilder nel mode**

In `Source/RefactorTacticsEditor/Private/RTHexEditorMode.cpp`, aggiungere in cima l'include:
```cpp
#include "Tools/RTHexPaintTool.h"
```
e in `Enter()`, dopo la `RegisterTool(...SelectTool...)` e PRIMA della `SelectActiveToolType(...)`, aggiungere:
```cpp
	RegisterTool(Commands.PaintTool, TEXT("RTHexPaintTool"), NewObject<URTHexPaintToolBuilder>(this));
```
*(Select resta il tool attivo di default: NON cambiare la riga `SelectActiveToolType(EToolSide::Left, TEXT("RTHexSelectTool"));`.)*

- [ ] **Step 6: Build del target Editor (editor chiuso)**

Run: `"D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -project="D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -waitmutex`
Expected: `Result: Succeeded`.

- [ ] **Step 7: Verifica manuale in editor (gate — non headless)**

Apri l'editor; metti un `ARTHexMapActor` nel livello (con `DemoRadius>0` o `MapAsset` popolato). Attiva il mode **Hex Map** → seleziona il tool **Paint**:
- **Paint**: imposta `Operation=Paint`, scegli `Surface`/`MoveCost`/`bBlocksMovement`, imposta `ActiveLayer` sull'actor, **clicca** una cella → l'esagono **verde** la evidenzia; la cella viene creata/aggiornata (visibile nell'ISM); `LastCell` corretto; Undo (Ctrl+Z) ripristina lo stato precedente.
- **Erase**: imposta `Operation=Erase`, **clicca** una cella esistente → esagono **rosso**; la cella sparisce dall'ISM; Undo la ripristina.
- Celle sovrapposte: cambiando `ActiveLayer` sull'actor si dipinge/cancella sul piano giusto.

Aggiungere a `docs/design/test-manuali-pie.md`:
```markdown
| **PIE-HEX-MODE-C** | Paint a click nel viewport (H5c) | mode Hex Map attivo, tool Paint, `ARTHexMapActor` nel livello | Con `Operation=Paint`, click su una cella → esagono verde + cella creata/aggiornata (superficie/costo/blocco del pennello); `LastCell` corretto; Undo ripristina | ⏳ (branch `feat/hex-grid`, H5c) |
| **PIE-HEX-MODE-D** | Erase a click nel viewport (H5c) | mode Hex Map attivo, tool Paint | Con `Operation=Erase`, click su una cella esistente → esagono rosso + cella rimossa dall'ISM; Undo ripristina; cambiando `ActiveLayer` agisce sul piano giusto | ⏳ (branch `feat/hex-grid`, H5c) |
```

- [ ] **Step 8: Aggiornare la roadmap (riga H5)**

In `docs/design/hex-map-roadmap.md`, nella cella Stato della riga **H5**, aggiungere in coda: `H5c prima fetta: URTHexPaintTool (paint+erase a click, marker verde/rosso) via helper condivisi RTHexEditorClick + ARTHexMapActor::PaintCellData/EraseCell + URTHexMapAsset::ApplyBrush (test RefactorTactics.HexMap.ApplyBrushMerge). Verifica editor PIE-HEX-MODE-C/D aperta. Restano H5c.2+ (drag-brush, palette Slate, gizmo archi, copia/incolla, overlay).`

- [ ] **Step 9: Commit**

```bash
git checkout -- RefactorTactics.uproject   # se risporcato
git add Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.h \
        Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.cpp \
        Source/RefactorTacticsEditor/Public/RTHexEditorModeCommands.h \
        Source/RefactorTacticsEditor/Private/RTHexEditorModeCommands.cpp \
        Source/RefactorTacticsEditor/Private/RTHexEditorMode.cpp \
        docs/design/test-manuali-pie.md docs/design/hex-map-roadmap.md
git commit -m "feat(hex): H5c.1b - tool Paint-a-click (paint+erase, USingleClickTool)"
```

---

## Self-Review (eseguita)

- **Copertura spec**: §3.1 tool → Task 2; §3.2 helper editor → Task 1 Step 7-8; §3.3 refactor runtime → Task 1 Step 6; §3.4 `ApplyBrush` → Task 1 Step 3-4; §3.5 registrazione → Task 2 Step 4-5; §6 test headless → Task 1 Step 1-5, 9; §6 PIE manuali → Task 2 Step 7; roadmap → Task 2 Step 8. Nessun gap.
- **Placeholder**: nessun TBD/TODO; ogni step di codice ha il blocco reale.
- **Consistenza tipi**: `ApplyBrush`/`PaintCellData`/`EraseCell` e gli helper `RTHexEditor::*` hanno le stesse firme in dichiarazione (Task 1) e uso (Task 2). Test namespace `RefactorTactics.HexMap.ApplyBrushMerge` coerente con lo stile del file esistente.

## Rischi noti

- **API ITF version-specific** (`UInteractiveToolPropertySet`, `USingleClickTool::OnClicked`, `FInputDeviceRay`, `FPrimitiveDrawInterface`, `RegisterTool`): tutte già usate da `URTHexSelectTool` (H5b, build verde) → basso rischio. Se un include cambia nome in 5.8, confrontare con `SampleToolsEditorMode`.
- **Modulo editor**: rebuild a editor CHIUSO (Live Coding non basta).
- **Refactor SelectTool**: solo estrazione, resa identica; regressione coperta da `PIE-HEX-MODE-B` (ancora valido) + build.
