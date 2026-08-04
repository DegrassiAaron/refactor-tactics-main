# H5c.7 Flood-fill (secchiello) — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Aggiungere un tool "secchiello" all'Editor Mode hex che, cliccando su una cella, riempie la regione contigua della stessa superficie applicando il pennello corrente, in un solo Undo.

**Architecture:** Logica pura `URTHexMapAsset::FloodRegion` (flood-fill same-layer/same-surface, headless-testabile) sul runtime; nuovo tool editor `URTHexFillTool : USingleClickTool` che risolve il click, chiama `FloodRegion`, e riscrive la regione via le primitive di stroke (H5c.3) dentro una `FScopedTransaction`. Rispetta l'invariante #1 (l'editor scrive solo dati d'asset).

**Tech Stack:** UE 5.8.1, C++; Interactive Tools Framework (`USingleClickTool`, `UInteractiveToolBuilder`, `UInteractiveToolPropertySet`); Unreal Automation (`IMPLEMENT_SIMPLE_AUTOMATION_TEST`).

## Global Constraints

- **Prefissi**: classi con prefisso `RT`/`URT` (mai `AT`/`UAT`).
- **Split runtime/editor** (invariante #1): la logica pura (`FloodRegion`) sta nel modulo runtime `RefactorTactics`; ITF/editor (tool, builder, transazioni) stanno nel modulo `RefactorTacticsEditor`. L'unica dipendenza editor tollerata nel runtime è la `FScopedTransaction` già sotto `#if WITH_EDITOR` — **questa fetta non ne aggiunge** (la transazione del flood vive nel tool editor).
- **Determinismo** (invariante #4): `FloodRegion` deterministica; nessuna logica di turno toccata.
- **Undo**: un solo Undo per flood — `BeginStroke()` fa `Modify()` una volta, dentro un'unica `FScopedTransaction`.
- **Scope**: contigue + stessa superficie + stesso layer. Fill globale, fill-erase, tolleranza, cross-layer, overlay-nel-Fill = fuori scope.
- **Lingua**: commenti in italiano; identificatori in inglese.
- **Precauzione build modulo editor**: se `UnrealEditor.exe` è in esecuzione la build del modulo editor fallisce (DLL lockata). Se l'editor è aperto → **STOP/BLOCKED**, non chiuderlo di iniziativa.
- **`.uproject`**: `EngineAssociation` deve restare `"5.8"`; se un'apertura editor lo risporca con un GUID, ripristinalo con `git checkout -- RefactorTactics.uproject` e non committarlo.

---

### Task 1: `URTHexMapAsset::FloodRegion` + test headless (H5c.7a)

**Files:**
- Modify: `Source/RefactorTactics/Map/RTHexMapAsset.h` (dichiarazione, dopo `CellsInLayer`, riga ~87)
- Modify: `Source/RefactorTactics/Map/RTHexMapAsset.cpp` (definizione; `#include "Map/RTHexLibrary.h"` già presente alla riga 2)
- Test: `Source/RefactorTactics/Tests/RTHexMapTests.cpp` (nuovo `IMPLEMENT_SIMPLE_AUTOMATION_TEST`, prima di `#endif`)
- Modify: `docs/design/test-manuali-pie.md:4` (bump contatore Automation: `92/92` → `93/93`, +1 test headless)

**Interfaces:**
- Consumes: `URTHexLibrary::Neighbors(const FRTCellId&) → TArray<FRTCellId>` (6 vicini stesso layer); `URTHexMapAsset::FindCell(const FRTCellId&) const → const FRTHexCellData*`; `FRTHexCellData::Surface` (`ERTHexSurface`).
- Produces: `TArray<FRTCellId> URTHexMapAsset::FloodRegion(const FRTCellId& Start) const` — usato dal tool Fill del Task 2.

- [ ] **Step 1: Scrivi il test che fallisce**

In `Source/RefactorTactics/Tests/RTHexMapTests.cpp`, subito prima della riga `#endif // WITH_DEV_AUTOMATION_TESTS`, aggiungi (riusa l'helper anonimo `Cell(X,Y,Layer)` già nel file):

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapFloodRegionTest,
	"RefactorTactics.HexMap.FloodRegion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapFloodRegionTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = NewObject<URTHexMapAsset>();

	// Helper locale: aggiunge una cella con superficie esplicita (Cell() di default e' Normal).
	auto AddSurf = [Map](int32 X, int32 Y, ERTHexSurface S)
	{
		FRTHexCellData C = Cell(X, Y);
		C.Surface = S;
		Map->AddOrUpdateCell(C);
	};

	// Regione contigua di 3 celle Normal: (0,0)-(1,0)-(0,1) (entrambe adiacenti a (0,0)).
	AddSurf(0, 0, ERTHexSurface::Normal);
	AddSurf(1, 0, ERTHexSurface::Normal);
	AddSurf(0, 1, ERTHexSurface::Normal);
	// Bordo: (2,0) adiacente a (1,0) ma Water (superficie diversa -> esclusa).
	AddSurf(2, 0, ERTHexSurface::Water);
	// Normal ma NON contigua alla regione -> esclusa.
	AddSurf(5, 5, ERTHexSurface::Normal);

	const TArray<FRTCellId> Region = Map->FloodRegion(FRTCellId(0, 0));
	const TSet<FRTCellId> RegionSet(Region);
	TestEqual(TEXT("3 celle nella regione"), Region.Num(), 3);
	TestTrue(TEXT("include (0,0)"), RegionSet.Contains(FRTCellId(0, 0)));
	TestTrue(TEXT("include (1,0)"), RegionSet.Contains(FRTCellId(1, 0)));
	TestTrue(TEXT("include (0,1)"), RegionSet.Contains(FRTCellId(0, 1)));
	TestFalse(TEXT("esclude bordo Water (2,0)"), RegionSet.Contains(FRTCellId(2, 0)));
	TestFalse(TEXT("esclude Normal non contigua (5,5)"), RegionSet.Contains(FRTCellId(5, 5)));

	// Start su cella inesistente -> regione vuota.
	TestEqual(TEXT("start inesistente -> vuoto"), Map->FloodRegion(FRTCellId(9, 9)).Num(), 0);
	return true;
}
```

- [ ] **Step 2: Compila e verifica che fallisca**

Build del modulo `RefactorTactics` (target Editor). Atteso: **errore di compilazione** `FloodRegion` non è membro di `URTHexMapAsset` (metodo ancora inesistente). È il "red" atteso della TDD.

Comando (aggiusta la versione del BuildTool a quella installata):
```
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" RefactorTacticsEditor Win64 Development -Project="D:\Repositories\refactor-tactics-main\RefactorTactics.uproject" -WaitMutex
```

- [ ] **Step 3: Dichiara `FloodRegion` nell'header**

In `Source/RefactorTactics/Map/RTHexMapAsset.h`, subito dopo il blocco `CellsInLayer` (attorno alla riga 87, prima di `AddTransition`), inserisci:

```cpp
	/**
	 * Regione contigua (flood-fill) a partire da Start: visita i vicini dello STESSO layer che ESISTONO nell'asset
	 * e hanno la STESSA superficie di Start (frontiera a stack; l'ordine di visita non altera l'insieme risultante).
	 * Include Start. Se Start non esiste -> regione vuota. Pura/read-only/deterministica.
	 */
	TArray<FRTCellId> FloodRegion(const FRTCellId& Start) const;
```

- [ ] **Step 4: Implementa `FloodRegion` nel .cpp**

In `Source/RefactorTactics/Map/RTHexMapAsset.cpp` aggiungi la definizione (in fondo al file va bene; `RTHexLibrary.h` è già incluso alla riga 2):

```cpp
TArray<FRTCellId> URTHexMapAsset::FloodRegion(const FRTCellId& Start) const
{
	TArray<FRTCellId> Region;
	const FRTHexCellData* StartData = FindCell(Start);
	if (!StartData)
	{
		return Region; // start inesistente: nessuna regione
	}
	const ERTHexSurface Target = StartData->Surface;

	TSet<FRTCellId> Visited;
	Visited.Add(Start);
	TArray<FRTCellId> Frontier;
	Frontier.Add(Start);

	while (Frontier.Num() > 0)
	{
		const FRTCellId Current = Frontier.Pop(EAllowShrinking::No);
		Region.Add(Current);
		for (const FRTCellId& N : URTHexLibrary::Neighbors(Current))
		{
			if (Visited.Contains(N))
			{
				continue;
			}
			const FRTHexCellData* NData = FindCell(N);
			if (NData && NData->Surface == Target)
			{
				Visited.Add(N);
				Frontier.Add(N);
			}
		}
	}
	return Region;
}
```

- [ ] **Step 5: Compila e lancia il test**

Build del target Editor (comando dello Step 2) → verde. Poi esegui la suite hex headless:
```
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\Repositories\refactor-tactics-main\RefactorTactics.uproject" -ExecCmds="Automation RunTests RefactorTactics.HexMap; Quit" -unattended -nop4 -nosplash -log
```
Atteso: tutti i test `RefactorTactics.HexMap.*` **Success**, incluso `RefactorTactics.HexMap.FloodRegion`.

- [ ] **Step 6: Aggiorna il contatore Automation nel doc PIE**

In `docs/design/test-manuali-pie.md`, riga 4, aggiorna il conteggio dei test Automation da `92/92 verdi` a `93/93 verdi` (il nuovo test headless `FloodRegion` porta la suite a 93). Non toccare altro nel file (la voce PIE-HEX-MODE-N la aggiunge il Task 2).

- [ ] **Step 7: Commit**

```bash
git add Source/RefactorTactics/Map/RTHexMapAsset.h Source/RefactorTactics/Map/RTHexMapAsset.cpp Source/RefactorTactics/Tests/RTHexMapTests.cpp docs/design/test-manuali-pie.md
git commit -m "feat(hex): H5c.7a - URTHexMapAsset::FloodRegion (flood-fill same-layer/same-surface) + test"
```

---

### Task 2: `URTHexFillTool` + comando + registrazione (H5c.7b)

**Files:**
- Create: `Source/RefactorTacticsEditor/Private/Tools/RTHexFillTool.h`
- Create: `Source/RefactorTacticsEditor/Private/Tools/RTHexFillTool.cpp`
- Modify: `Source/RefactorTacticsEditor/Public/RTHexEditorModeCommands.h` (aggiungi `FillTool`, dopo `ArchTool`, riga ~22)
- Modify: `Source/RefactorTacticsEditor/Private/RTHexEditorModeCommands.cpp` (UI_COMMAND + `ToolCommands.Add`, dopo `ArchTool`, riga ~27)
- Modify: `Source/RefactorTacticsEditor/Private/RTHexEditorMode.cpp` (include + `RegisterTool`, dopo la registrazione di `ArchTool`, riga ~30)

**Interfaces:**
- Consumes: `URTHexMapAsset::FloodRegion` (Task 1); `RTHexEditor::FindTargetMapActor(UWorld*)`, `RTHexEditor::ResolveClickedCell(UWorld*, ARTHexMapActor*, const FInputDeviceRay&, FRTCellId&, FVector&)`, `RTHexEditor::DrawHexMarker(FPrimitiveDrawInterface*, const FVector&, float, const FColor&)` (namespace `RTHexEditor` in `RTHexEditorClick.h`); `URTHexMapAsset::BeginStroke/PaintCellInStroke/EndStroke`; `ARTHexMapActor::RebuildInstances()`, `ARTHexMapActor::MapAsset`, `ARTHexMapActor::HexSize`.
- Produces: tool `RTHexFillTool` registrato nel mode (Select resta default).

- [ ] **Step 1: Crea l'header del tool**

Crea `Source/RefactorTacticsEditor/Private/Tools/RTHexFillTool.h`:

```cpp
#pragma once

#include "BaseTools/SingleClickTool.h"
#include "Map/RTHexCellData.h" // FRTCellId + ERTHexSurface (pennello + readout)
#include "RTHexFillTool.generated.h"

class ARTHexMapActor;
class IToolsContextRenderAPI;

/** Factory del tool secchiello. */
UCLASS()
class URTHexFillToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()
public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override { return true; }
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
};

/** Proprieta' del secchiello: pennello (Surface/costo/blocco) applicato alla regione + readout ultimo riempimento. */
UCLASS(Transient)
class URTHexFillToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	/** Superficie applicata alle celle della regione. */
	UPROPERTY(EditAnywhere, Category = "Hex|Fill")
	ERTHexSurface Surface = ERTHexSurface::Normal;

	/** Costo di movimento applicato (intero). */
	UPROPERTY(EditAnywhere, Category = "Hex|Fill", meta = (ClampMin = "0"))
	int32 MoveCost = 1;

	/** Blocca il movimento. */
	UPROPERTY(EditAnywhere, Category = "Hex|Fill")
	bool bBlocksMovement = false;

	/** Cella di partenza dell'ultimo riempimento (sola lettura). */
	UPROPERTY(VisibleAnywhere, Category = "Hex|Fill")
	FRTCellId LastCell;

	/** Numero di celle riempite nell'ultimo flood (sola lettura). */
	UPROPERTY(VisibleAnywhere, Category = "Hex|Fill")
	int32 FilledCount = 0;
};

/**
 * Secchiello (flood-fill): click su una cella esistente -> riempie l'intera regione contigua della stessa superficie
 * col pennello corrente, in un solo Undo. Click su cella vuota (assente nell'asset) -> nessuna azione.
 */
UCLASS()
class URTHexFillTool : public USingleClickTool
{
	GENERATED_BODY()
public:
	virtual void SetWorld(UWorld* World);
	virtual void Setup() override;
	virtual void OnClicked(const FInputDeviceRay& ClickPos) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;

protected:
	UPROPERTY()
	TObjectPtr<URTHexFillToolProperties> Properties;

	UWorld* TargetWorld = nullptr;

	bool bHasMarker = false;
	FVector MarkerCenter = FVector::ZeroVector;
	float MarkerRadius = 50.f;
};
```

- [ ] **Step 2: Crea il .cpp del tool**

Crea `Source/RefactorTacticsEditor/Private/Tools/RTHexFillTool.cpp`:

```cpp
#include "Tools/RTHexFillTool.h"
#include "RTHexEditorClick.h"
#include "InteractiveToolManager.h"
#include "ToolContextInterfaces.h"
#include "ScopedTransaction.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"

#define LOCTEXT_NAMESPACE "URTHexFillTool"

UInteractiveTool* URTHexFillToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	URTHexFillTool* NewTool = NewObject<URTHexFillTool>(SceneState.ToolManager);
	NewTool->SetWorld(SceneState.World);
	return NewTool;
}

void URTHexFillTool::SetWorld(UWorld* World)
{
	TargetWorld = World;
}

void URTHexFillTool::Setup()
{
	USingleClickTool::Setup();
	Properties = NewObject<URTHexFillToolProperties>(this);
	AddToolPropertySource(Properties);
}

void URTHexFillTool::OnClicked(const FInputDeviceRay& ClickPos)
{
	bHasMarker = false;
	if (!Properties) { return; }

	ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld);
	if (!Actor || !Actor->MapAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HexMode] Fill: nessun ARTHexMapActor con MapAsset."));
		return;
	}
	URTHexMapAsset* Map = Actor->MapAsset;

	FRTCellId Cell;
	FVector Center;
	if (!RTHexEditor::ResolveClickedCell(TargetWorld, Actor, ClickPos, Cell, Center)) { return; }

	// Il secchiello riempie solo regioni ESISTENTI: cella vuota -> nessuna azione (non crea celle nuove).
	if (!Map->FindCell(Cell))
	{
		UE_LOG(LogTemp, Log, TEXT("[HexMode] Fill: cella %s vuota, niente da riempire."), *Cell.ToString());
		return;
	}

	const TArray<FRTCellId> Region = Map->FloodRegion(Cell);
	if (Region.Num() == 0) { return; }

	{
		const FScopedTransaction Transaction(LOCTEXT("HexFill", "Hex: Flood Fill"));
		Map->BeginStroke();
		for (const FRTCellId& C : Region)
		{
			Map->PaintCellInStroke(C, Properties->Surface, Properties->MoveCost, Properties->bBlocksMovement);
		}
		Map->EndStroke();
		Actor->RebuildInstances();
	}

	Properties->LastCell = Cell;
	Properties->FilledCount = Region.Num();
	MarkerCenter = Center;
	MarkerRadius = Map->HexSize * 0.9f;
	bHasMarker = true;

	UE_LOG(LogTemp, Log, TEXT("[HexMode] Fill: %d celle riempite dalla regione di %s."), Region.Num(), *Cell.ToString());
}

void URTHexFillTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (!bHasMarker || !RenderAPI) { return; }
	FPrimitiveDrawInterface* PDI = RenderAPI->GetPrimitiveDrawInterface();
	if (!PDI) { return; }
	RTHexEditor::DrawHexMarker(PDI, MarkerCenter, MarkerRadius, FColor(120, 255, 120));
}

#undef LOCTEXT_NAMESPACE
```

- [ ] **Step 3: Dichiara il comando `FillTool`**

In `Source/RefactorTacticsEditor/Public/RTHexEditorModeCommands.h`, subito dopo la dichiarazione di `ArchTool` (riga ~22), aggiungi:

```cpp
	TSharedPtr<FUICommandInfo> FillTool;
```

- [ ] **Step 4: Registra il `UI_COMMAND`**

In `Source/RefactorTacticsEditor/Private/RTHexEditorModeCommands.cpp`, subito dopo il blocco `ArchTool` (dopo `ToolCommands.Add(ArchTool);`, riga ~27), aggiungi:

```cpp
	UI_COMMAND(FillTool, "Fill", "Secchiello: riempie la regione contigua della stessa superficie col pennello corrente.",
		EUserInterfaceActionType::ToggleButton, FInputChord());
	ToolCommands.Add(FillTool);
```

- [ ] **Step 5: Registra il tool nel mode**

In `Source/RefactorTacticsEditor/Private/RTHexEditorMode.cpp`:
1. dopo `#include "Tools/RTHexArchTool.h"` (riga ~7) aggiungi:
```cpp
#include "Tools/RTHexFillTool.h"
```
2. dentro `Enter()`, subito dopo la `RegisterTool(...)` di `ArchTool` (riga ~30) e **prima** di `SelectActiveToolType(...)`, aggiungi:
```cpp
	RegisterTool(Commands.FillTool, TEXT("RTHexFillTool"), NewObject<URTHexFillToolBuilder>(this));
```
(Non toccare `SelectActiveToolType`: Select resta il tool attivo di default.)

- [ ] **Step 6: Compila il modulo editor**

**Precondizione**: `UnrealEditor.exe` **NON** in esecuzione (se aperto → STOP/BLOCKED, non chiudere di iniziativa).
```
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" RefactorTacticsEditor Win64 Development -Project="D:\Repositories\refactor-tactics-main\RefactorTactics.uproject" -WaitMutex
```
Atteso: build **verde**. (Il flood-fill nel viewport è editor-bound → verifica manuale PIE-HEX-MODE-N, non in questo step.)

- [ ] **Step 7: Aggiorna la voce PIE e la roadmap**

In `docs/design/test-manuali-pie.md` aggiungi una riga `PIE-HEX-MODE-N` (stato ⏳): "In Fill, click su una regione la riempie col pennello corrente; un Ctrl+Z ripristina l'intera regione; click su cella vuota non fa nulla; passando a Select/Paint con overlay si vedono i nuovi colori." (aggiorna anche il contatore in cima se presente).

In `docs/design/hex-map-roadmap.md` aggiorna la riga H5 aggiungendo `H5c.7 (flood-fill/secchiello)` tra le consegne.

- [ ] **Step 8: Verifica `.uproject` e committa**

Verifica che `RefactorTactics.uproject` non sia stato risporcato (`EngineAssociation` deve restare `"5.8"`; se un GUID è comparso: `git checkout -- RefactorTactics.uproject`).

```bash
git add Source/RefactorTacticsEditor/Private/Tools/RTHexFillTool.h Source/RefactorTacticsEditor/Private/Tools/RTHexFillTool.cpp Source/RefactorTacticsEditor/Public/RTHexEditorModeCommands.h Source/RefactorTacticsEditor/Private/RTHexEditorModeCommands.cpp Source/RefactorTacticsEditor/Private/RTHexEditorMode.cpp docs/design/test-manuali-pie.md docs/design/hex-map-roadmap.md
git commit -m "feat(hex): H5c.7b - tool secchiello URTHexFillTool (flood-fill via FloodRegion + stroke, un Undo)"
```

---

## Self-Review

- **Copertura spec**: §3.1 FloodRegion → Task 1; §3.2 tool+builder+properties+comando+registrazione → Task 2; §6 test headless → Task 1 Step 1/5; voce PIE-HEX-MODE-N + roadmap → Task 2 Step 7. ✅
- **Placeholder**: nessun TODO/TBD; ogni step ha codice o comando concreto. ✅
- **Coerenza tipi**: `FloodRegion(const FRTCellId&) const → TArray<FRTCellId>` identica in header/impl/uso; property set usa `ERTHexSurface`/`int32`/`bool` come `FRTHexCellData`; `PaintCellInStroke(Id, Surface, MoveCost, bBlocks)` firma reale (H5c.3); `RTHexEditor::*` firme reali da `RTHexEditorClick.h`. ✅
- **Rischio noto**: `TArray::Pop(EAllowShrinking::No)` è l'idioma UE 5.8; se la build segnalasse un problema di overload, usare `Frontier.Pop()` (semantica identica ai fini della regione).
