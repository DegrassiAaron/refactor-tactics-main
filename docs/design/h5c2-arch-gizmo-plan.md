# H5c.2 — Tool "Transizioni" con gizmo (Add-only) — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or
> superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Creare transizioni (`FRTHexEdge`) tra celle esagonali nel viewport — click su `From`, gizmo di traslazione per `To` (snap esagonale, anche su altro layer), bottone Commit — e visualizzare le transizioni esistenti.

**Architecture:** Nuovo tool ITF `URTHexArchTool : USingleClickTool` nel modulo editor. Riusa gli helper `RTHexEditor` (H5c.1) e scrive via un nuovo `ARTHexMapActor::AddTransitionData` (estratto da `AddVerticalTransition`). Il gizmo è `UCombinedTransformGizmo` + `UTransformProxy` creati da `GetPairedGizmoManager()->CreateCustomTransformGizmo(...)` (i default gizmo sono auto-registrati dal tools context di `UEdMode` → nessuna registrazione manuale). Lo snap usa `URTHexLibrary::WorldToLayer` (nuovo, puro) + `WorldToAxial`.

**Tech Stack:** UE 5.8.1 C++; `InteractiveToolsFramework` (BaseGizmos: CombinedTransformGizmo, TransformProxy; InteractiveGizmoManager), `EditorInteractiveToolsFramework` (già linkati in `RefactorTacticsEditor.Build.cs`). Spec: [`h5c2-arch-gizmo-spec.md`](h5c2-arch-gizmo-spec.md).

## Global Constraints

- Motore **UE 5.8.1**; `RefactorTactics.uproject` `EngineAssociation` deve restare `"5.8"` (ripristinare con `git checkout -- RefactorTactics.uproject` se l'editor lo risporca a GUID).
- Prefissi classi **`RT`/`URT`**; documentazione in `docs/`.
- **L'editor non decide gameplay** (#1): il tool scrive solo dati d'asset, via `Modify()`/`FScopedTransaction`.
- **Dipendenze editor confinate** nel modulo `RefactorTacticsEditor`; il refactor runtime (`AddTransitionData`, `WorldToLayer`) non aggiunge dip. editor (la `FScopedTransaction` è già `#if WITH_EDITOR` in `RTHexMapActor.cpp`). **Nessun cambio a `Build.cs`.**
- **Determinismo** (#4): nessuna logica di turno toccata; `WorldToAxial`/`WorldToLayer` deterministici su interi.
- **No duplicazione**: pipeline click→cella condivisa (`RTHexEditor`); scrittura archi condivisa (`AddTransitionData`).
- **Ciclo di vita gizmo**: teardown su `Shutdown` (`DestroyAllGizmosByOwner(this)`); niente gizmo duplicati su re-click.
- Un **commit per task**; build Editor **verde** prima di dichiarare fatto; verifiche editor-bound dichiarate **manuali**.
- **Editor CHIUSO** durante il rebuild (modulo editor non ricaricabile via Live Coding).
- Build (editor chiuso): `"D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -project="D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -waitmutex` → cercare `Result: Succeeded`.
- Test headless (editor chiuso): `"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -ExecCmds="Automation RunTests <PATTERN>; Quit" -unattended -nopause -nosplash -nullrhi` → risultati in `Saved/Logs/RefactorTactics.log` (`Fail` = 0).

---

## Task 1 (H5c.2a): `WorldToLayer` + `AddTransitionData` + tool "guscio" con rendering degli archi

Estrae la logica pura `WorldToLayer` (con test), il metodo runtime riusabile `AddTransitionData`, e crea `URTHexArchTool` che **disegna le transizioni esistenti** (oggi invisibili). Nessun gizmo qui. Deliverable: build verde + test hex verdi + archi visibili nel viewport quando il tool è attivo.

**Files:**
- Modify: `Source/RefactorTactics/Map/RTHexLibrary.h` (dichiara `WorldToLayer`)
- Modify: `Source/RefactorTactics/Map/RTHexLibrary.cpp` (implementa `WorldToLayer`)
- Modify: `Source/RefactorTactics/Map/RTHexMapActor.h` (dichiara `AddTransitionData`)
- Modify: `Source/RefactorTactics/Map/RTHexMapActor.cpp` (implementa `AddTransitionData`; `AddVerticalTransition` wrapper)
- Create: `Source/RefactorTacticsEditor/Private/Tools/RTHexArchTool.h`
- Create: `Source/RefactorTacticsEditor/Private/Tools/RTHexArchTool.cpp`
- Modify: `Source/RefactorTacticsEditor/Public/RTHexEditorModeCommands.h` (dichiara `ArchTool`)
- Modify: `Source/RefactorTacticsEditor/Private/RTHexEditorModeCommands.cpp` (registra comando `ArchTool`)
- Modify: `Source/RefactorTacticsEditor/Private/RTHexEditorMode.cpp` (registra il ToolBuilder)
- Test: `Source/RefactorTactics/Tests/RTHexTests.cpp` (nuovo test `WorldToLayer`)
- Modify: `docs/design/test-manuali-pie.md` (voce PIE-HEX-MODE-F)

**Interfaces:**
- Produces: `static int32 URTHexLibrary::WorldToLayer(double WorldZ, double OriginZ, float LayerHeight)`; `void ARTHexMapActor::AddTransitionData(const FRTCellId& From, const FRTCellId& To, int32 Cost, ERTHexTransitionKind Kind, bool bBidirectional)`; `URTHexArchTool`, `URTHexArchToolBuilder`, `URTHexArchToolProperties`; `FRTHexEditorModeCommands::ArchTool`.
- Consumes: `RTHexEditor::FindTargetMapActor`; `URTHexLibrary::AxialToWorld`; `URTHexMapAsset::{Transitions,HexSize,LayerHeight}`; `URTHexMapAsset::{AddTransition,ContainsCell}`.

- [ ] **Step 1: Scrivere il test che fallisce (`WorldToLayer`)**

In `Source/RefactorTactics/Tests/RTHexTests.cpp`, prima di `#endif // WITH_DEV_AUTOMATION_TESTS`, aggiungere:
```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexWorldToLayerTest,
	"RefactorTactics.Hex.WorldToLayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexWorldToLayerTest::RunTest(const FString&)
{
	const double OriginZ = 200.0;
	const float LayerH = 250.f;

	// LayerHeight <= 0 -> 0 (nessuna divisione).
	TestEqual(TEXT("LayerHeight 0 -> layer 0"), URTHexLibrary::WorldToLayer(9999.0, OriginZ, 0.f), 0);

	// Z al centro di un layer -> quel layer.
	TestEqual(TEXT("Z all'origine -> 0"), URTHexLibrary::WorldToLayer(OriginZ, OriginZ, LayerH), 0);
	TestEqual(TEXT("Z = origin + 2*LayerH -> 2"), URTHexLibrary::WorldToLayer(OriginZ + 2.0 * LayerH, OriginZ, LayerH), 2);
	TestEqual(TEXT("Z = origin - 1*LayerH -> -1"), URTHexLibrary::WorldToLayer(OriginZ - 1.0 * LayerH, OriginZ, LayerH), -1);

	// Tie-break floor(x+0.5): +1.5 -> 2 ; -0.5 -> 0.
	TestEqual(TEXT("Z = origin + 1.5*LayerH -> 2"), URTHexLibrary::WorldToLayer(OriginZ + 1.5 * LayerH, OriginZ, LayerH), 2);
	TestEqual(TEXT("Z = origin - 0.5*LayerH -> 0"), URTHexLibrary::WorldToLayer(OriginZ - 0.5 * LayerH, OriginZ, LayerH), 0);
	return true;
}
```

- [ ] **Step 2: Build → verificare che fallisce a compilazione**

Run: `"D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -project="D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -waitmutex`
Expected: **FAIL** — «`WorldToLayer` is not a member of `URTHexLibrary`».

- [ ] **Step 3: Dichiarare `WorldToLayer`**

In `Source/RefactorTactics/Map/RTHexLibrary.h`, subito dopo la dichiarazione di `WorldToAxial` (riga ~40), aggiungere:
```cpp
	/** Layer (intero) corrispondente a una quota-mondo Z. LayerHeight<=0 -> 0. RoundToInt = floor(x+0.5). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static int32 WorldToLayer(double WorldZ, double OriginZ, float LayerHeight);
```

- [ ] **Step 4: Implementare `WorldToLayer`**

In `Source/RefactorTactics/Map/RTHexLibrary.cpp`, dopo `WorldToAxial` (dopo riga ~90), aggiungere:
```cpp
int32 URTHexLibrary::WorldToLayer(double WorldZ, double OriginZ, float LayerHeight)
{
	if (LayerHeight <= 0.f)
	{
		return 0;
	}
	return FMath::RoundToInt((WorldZ - OriginZ) / static_cast<double>(LayerHeight));
}
```

- [ ] **Step 5: Build + eseguire il test → passa**

Run build (Step 2) → `Result: Succeeded`. Poi:
`"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -ExecCmds="Automation RunTests RefactorTactics.Hex.WorldToLayer; Quit" -unattended -nopause -nosplash -nullrhi`
Expected: `RefactorTactics.Hex.WorldToLayer` **Success**, 0 Fail.

- [ ] **Step 6: Estrarre `AddTransitionData` sull'actor**

In `Source/RefactorTactics/Map/RTHexMapActor.h`, dopo la dichiarazione di `AddVerticalTransition()` (riga ~121), aggiungere:
```cpp
	/** Aggiunge la transizione From->To (e l'inversa se bidirezionale) se entrambe le celle esistono. Annullabile. */
	void AddTransitionData(const FRTCellId& From, const FRTCellId& To, int32 Cost,
		ERTHexTransitionKind Kind, bool bBidirectional);
```

In `Source/RefactorTactics/Map/RTHexMapActor.cpp`, sostituire l'intero corpo di `AddVerticalTransition()` (righe ~215-238) con il wrapper + il nuovo metodo:
```cpp
void ARTHexMapActor::AddVerticalTransition()
{
	AddTransitionData(TransitionFrom, TransitionTo, FMath::Max(0, TransitionCost), TransitionKind, bTransitionBidirectional);
}

void ARTHexMapActor::AddTransitionData(const FRTCellId& From, const FRTCellId& To, int32 Cost,
	ERTHexTransitionKind Kind, bool bBidirectional)
{
	if (!MapAsset)
	{
		UE_LOG(LogRT, Warning, TEXT("[HexMap] Nessun MapAsset assegnato."));
		return;
	}
	if (!MapAsset->ContainsCell(From) || !MapAsset->ContainsCell(To))
	{
		UE_LOG(LogRT, Warning, TEXT("[HexMap] Transizione %s -> %s: una delle due celle non esiste nell'asset."),
			*From.ToString(), *To.ToString());
		return;
	}
#if WITH_EDITOR
	const FScopedTransaction Transaction(LOCTEXT("HexAddTransition", "Hex: Add Vertical Transition"));
#endif
	MapAsset->Modify();
	MapAsset->AddTransition(From, To, FMath::Max(0, Cost), Kind, bBidirectional);
	MapAsset->MarkPackageDirty();
	RebuildInstances();
	UE_LOG(LogRT, Log, TEXT("[HexMap] Transizione aggiunta %s -> %s (tipo %d, costo %d, bidirezionale=%d)."),
		*From.ToString(), *To.ToString(), static_cast<int32>(Kind), Cost, bBidirectional ? 1 : 0);
}
```

- [ ] **Step 7: Header del tool (guscio con rendering)**

`Source/RefactorTacticsEditor/Private/Tools/RTHexArchTool.h`:
```cpp
#pragma once

#include "BaseTools/SingleClickTool.h"
#include "Map/RTHexCellData.h" // FRTCellId + ERTHexTransitionKind
#include "RTHexArchTool.generated.h"

class ARTHexMapActor;
class IToolsContextRenderAPI;

/** Factory del tool archi. */
UCLASS()
class URTHexArchToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()
public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override { return true; }
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
};

/** Palette del tool archi: parametri della transizione + readout. (Bottoni Commit/Clear aggiunti in H5c.2b.) */
UCLASS(Transient)
class URTHexArchToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Hex|Arco")
	ERTHexTransitionKind Kind = ERTHexTransitionKind::Stair;

	UPROPERTY(EditAnywhere, Category = "Hex|Arco", meta = (ClampMin = "0"))
	int32 Cost = 2;

	UPROPERTY(EditAnywhere, Category = "Hex|Arco")
	bool bBidirectional = true;

	UPROPERTY(VisibleAnywhere, Category = "Hex|Arco")
	FRTCellId From;

	UPROPERTY(VisibleAnywhere, Category = "Hex|Arco")
	bool bHasFrom = false;

	UPROPERTY(VisibleAnywhere, Category = "Hex|Arco")
	FRTCellId To;

	UPROPERTY(VisibleAnywhere, Category = "Hex|Arco")
	bool bToValid = false;
};

/**
 * Crea transizioni (FRTHexEdge) nel viewport: click su From, gizmo per To (H5c.2b). In H5c.2a disegna solo le
 * transizioni esistenti dell'asset (rese visibili per la prima volta). Non modifica dati in questa fetta.
 */
UCLASS()
class URTHexArchTool : public USingleClickTool
{
	GENERATED_BODY()
public:
	virtual void SetWorld(UWorld* World);
	virtual void Setup() override;
	virtual void OnClicked(const FInputDeviceRay& ClickPos) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;

protected:
	UPROPERTY()
	TObjectPtr<URTHexArchToolProperties> Properties;

	UWorld* TargetWorld = nullptr;
};
```

- [ ] **Step 8: Implementazione del tool (guscio con rendering)**

`Source/RefactorTacticsEditor/Private/Tools/RTHexArchTool.cpp`:
```cpp
#include "Tools/RTHexArchTool.h"
#include "RTHexEditorClick.h"
#include "InteractiveToolManager.h"
#include "ToolContextInterfaces.h"
#include "PrimitiveDrawingUtils.h" // FPrimitiveDrawInterface / SDPG_*
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"

#define LOCTEXT_NAMESPACE "URTHexArchTool"

namespace
{
	// Colore per tipo di transizione (solo visualizzazione).
	FColor RTHexArchKindColor(ERTHexTransitionKind Kind)
	{
		switch (Kind)
		{
		case ERTHexTransitionKind::Stair:    return FColor(80, 200, 255);
		case ERTHexTransitionKind::Ramp:     return FColor(120, 255, 120);
		case ERTHexTransitionKind::Bridge:   return FColor(255, 200, 80);
		case ERTHexTransitionKind::Tunnel:   return FColor(200, 120, 255);
		case ERTHexTransitionKind::Elevator: return FColor(255, 120, 120);
		case ERTHexTransitionKind::Jump:     return FColor(255, 255, 255);
		default:                             return FColor::White;
		}
	}

	// Linea con freccia verso B (arrowhead sul piano orizzontale).
	void RTHexArchDrawArrow(FPrimitiveDrawInterface* PDI, const FVector& A, const FVector& B, const FColor& Color)
	{
		PDI->DrawLine(A, B, Color, SDPG_Foreground, 2.f);
		const FVector Dir = (B - A).GetSafeNormal();
		if (!Dir.IsNearlyZero())
		{
			const FVector Side = FVector::CrossProduct(Dir, FVector::UpVector).GetSafeNormal();
			const double H = 30.0;
			PDI->DrawLine(B, B - Dir * H + Side * (H * 0.5), Color, SDPG_Foreground, 2.f);
			PDI->DrawLine(B, B - Dir * H - Side * (H * 0.5), Color, SDPG_Foreground, 2.f);
		}
	}
}

UInteractiveTool* URTHexArchToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	URTHexArchTool* NewTool = NewObject<URTHexArchTool>(SceneState.ToolManager);
	NewTool->SetWorld(SceneState.World);
	return NewTool;
}

void URTHexArchTool::SetWorld(UWorld* World)
{
	TargetWorld = World;
}

void URTHexArchTool::Setup()
{
	USingleClickTool::Setup();
	Properties = NewObject<URTHexArchToolProperties>(this);
	AddToolPropertySource(Properties);
}

void URTHexArchTool::OnClicked(const FInputDeviceRay& ClickPos)
{
	// H5c.2a: nessuna azione al click (il gizmo arriva in H5c.2b). Placeholder no-op consapevole.
}

void URTHexArchTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (!RenderAPI) { return; }
	FPrimitiveDrawInterface* PDI = RenderAPI->GetPrimitiveDrawInterface();
	if (!PDI) { return; }

	const ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld);
	if (!Actor || !Actor->MapAsset) { return; }

	const FVector Origin = Actor->GetActorLocation();
	const float HexSize = Actor->MapAsset->HexSize;
	const float LayerH = Actor->MapAsset->LayerHeight;
	for (const FRTHexEdge& E : Actor->MapAsset->Transitions)
	{
		const FVector A = URTHexLibrary::AxialToWorld(E.From, Origin, HexSize, LayerH);
		const FVector B = URTHexLibrary::AxialToWorld(E.To, Origin, HexSize, LayerH);
		RTHexArchDrawArrow(PDI, A, B, RTHexArchKindColor(E.Kind));
	}
}

#undef LOCTEXT_NAMESPACE
```
> Nota: `OnClicked` è un no-op DICHIARATO solo per H5c.2a (il tool è già `USingleClickTool` così H5c.2b vi aggancia il gizmo senza cambiare la classe base). Non è codice incompleto di produzione: è il guscio della fetta.

- [ ] **Step 9: Dichiarare e registrare il comando `ArchTool`**

In `Source/RefactorTacticsEditor/Public/RTHexEditorModeCommands.h`, dopo `TSharedPtr<FUICommandInfo> PaintTool;`, aggiungere:
```cpp
	/** Tool creazione transizioni con gizmo (H5c.2). */
	TSharedPtr<FUICommandInfo> ArchTool;
```

In `Source/RefactorTacticsEditor/Private/RTHexEditorModeCommands.cpp`, in fondo a `RegisterCommands()` (dopo `ToolCommands.Add(PaintTool);`), aggiungere:
```cpp
	UI_COMMAND(ArchTool, "Arch", "Crea transizioni tra celle cliccando e usando il gizmo (layer attivo)",
		EUserInterfaceActionType::ToggleButton, FInputChord());
	ToolCommands.Add(ArchTool);
```

- [ ] **Step 10: Registrare il ToolBuilder nel mode**

In `Source/RefactorTacticsEditor/Private/RTHexEditorMode.cpp`, aggiungere in cima l'include:
```cpp
#include "Tools/RTHexArchTool.h"
```
e in `Enter()`, dopo la `RegisterTool(...PaintTool...)` e PRIMA della `SelectActiveToolType(...)`, aggiungere:
```cpp
	RegisterTool(Commands.ArchTool, TEXT("RTHexArchTool"), NewObject<URTHexArchToolBuilder>(this));
```
*(Non cambiare la riga `SelectActiveToolType(EToolSide::Left, TEXT("RTHexSelectTool"));`: Select resta il default.)*

- [ ] **Step 11: Build + suite → verde**

Run build (Step 2) → `Result: Succeeded`. Poi:
`"D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -ExecCmds="Automation RunTests RefactorTactics; Quit" -unattended -nopause -nosplash -nullrhi`
Expected: tutti i test hex Success, **0 Fail** (92: 91 preesistenti + `WorldToLayer`).

- [ ] **Step 12: Verifica manuale (gate — non headless) + voce PIE**

Manuale: attiva il mode Hex Map → tool **Arch** con un `ARTHexMapActor` che ha transizioni (creale via `AddVerticalTransition` CallInEditor) → le transizioni si vedono come linee colorate con freccia. Aggiungere a `docs/design/test-manuali-pie.md`:
```markdown
| **PIE-HEX-MODE-F** | Render transizioni nel tool Arch (H5c.2a) | mode Hex Map, tool Arch, `ARTHexMapActor` con transizioni | Le transizioni esistenti appaiono come linee colorate (per Kind) con freccia From->To | ⏳ (branch `feat/hex-grid`, H5c.2a) |
```

- [ ] **Step 13: Commit**

```bash
git checkout -- RefactorTactics.uproject   # se risporcato
git add Source/RefactorTactics/Map/RTHexLibrary.h Source/RefactorTactics/Map/RTHexLibrary.cpp \
        Source/RefactorTactics/Map/RTHexMapActor.h Source/RefactorTactics/Map/RTHexMapActor.cpp \
        Source/RefactorTactics/Tests/RTHexTests.cpp \
        Source/RefactorTacticsEditor/Private/Tools/RTHexArchTool.h \
        Source/RefactorTacticsEditor/Private/Tools/RTHexArchTool.cpp \
        Source/RefactorTacticsEditor/Public/RTHexEditorModeCommands.h \
        Source/RefactorTacticsEditor/Private/RTHexEditorModeCommands.cpp \
        Source/RefactorTacticsEditor/Private/RTHexEditorMode.cpp \
        docs/design/test-manuali-pie.md
git commit -m "feat(hex): H5c.2a - WorldToLayer + AddTransitionData + tool Arch con render transizioni"
```

---

## Task 2 (H5c.2b): Gizmo (click From -> gizmo To -> Commit)

Aggiunge al tool: click su `From` + spawn gizmo; drag → snap `To` (anche altro layer) + preview; bottoni Commit/ClearArch; teardown del gizmo. **Editor-bound**: verifica in editor (PIE-HEX-MODE-E). Primo passo = **smoke test** del gizmo prima di cablare snap/commit.

**Files:**
- Modify: `Source/RefactorTacticsEditor/Private/Tools/RTHexArchTool.h` (stato gizmo, Commit/Clear/Shutdown, WeakTool)
- Modify: `Source/RefactorTacticsEditor/Private/Tools/RTHexArchTool.cpp` (gizmo, snap, commit, render preview)
- Modify: `docs/design/test-manuali-pie.md` (voce PIE-HEX-MODE-E)
- Modify: `docs/design/hex-map-roadmap.md` (riga H5: H5c.2 fatta)

**Interfaces:**
- Consumes: `GetToolManager()->GetPairedGizmoManager()->{CreateCustomTransformGizmo,DestroyAllGizmosByOwner}`; `UCombinedTransformGizmo::SetActiveTarget`; `UTransformProxy::{SetTransform,OnTransformChanged}`; `URTHexLibrary::{WorldToLayer,WorldToAxial,AxialToWorld}`; `RTHexEditor::{FindTargetMapActor,ResolveClickedCell,DrawHexMarker}`; `ARTHexMapActor::AddTransitionData`.
- Produces: gizmo interattivo + creazione transizione dal viewport.

- [ ] **Step 1: Nota testing (nessun test headless)**

Il gizmo/preview/commit è editor-bound: non unit-testabile headless. La logica pura sotto (`WorldToLayer`) è già coperta (Task 1). Verifica in editor allo Step 8 (PIE-HEX-MODE-E). *(Dichiarazione esplicita DoD.)*

- [ ] **Step 2: Header — stato gizmo, bottoni, teardown**

In `Source/RefactorTacticsEditor/Private/Tools/RTHexArchTool.h`:

(a) In cima, dopo gli `#include`, aggiungere le forward-declaration (NIENTE `struct FTransform;`: `FTransform` è un
type-alias già completo via `CoreMinimal.h`, dichiararlo come struct collide):
```cpp
class UTransformProxy;
class UCombinedTransformGizmo;
```
(b) Nella classe `URTHexArchToolProperties` (sezione public), dopo `bToValid`, aggiungere il back-pointer e i due bottoni:
```cpp
	/** Back-pointer al tool per i bottoni (impostato in Setup). */
	TWeakObjectPtr<class URTHexArchTool> WeakTool;

	/** Crea la transizione From->To con i parametri correnti. */
	UFUNCTION(CallInEditor, Category = "Hex|Arco")
	void Commit();

	/** Annulla l'arco pendente (nessuna scrittura). */
	UFUNCTION(CallInEditor, Category = "Hex|Arco")
	void ClearArch();
```
(c) Nella classe `URTHexArchTool`, aggiungere l'override di `Shutdown`, i metodi pubblici Commit/Clear e lo stato:
```cpp
public:
	virtual void Shutdown(EToolShutdownType ShutdownType) override;
	void CommitArch();
	void ClearPending();

protected:
	void OnGizmoMoved(UTransformProxy* InProxy, FTransform InTransform);
	void DestroyPendingGizmo();

	UPROPERTY()
	TObjectPtr<UTransformProxy> Proxy;

	UPROPERTY()
	TObjectPtr<UCombinedTransformGizmo> Gizmo;

	UPROPERTY()
	TObjectPtr<ARTHexMapActor> TargetActor = nullptr;

	FRTCellId From;
	FRTCellId To;
	bool bHasFrom = false;
	bool bToValid = false;
	bool bSnapping = false;
	FVector FromWorld = FVector::ZeroVector;
	FVector ToWorld = FVector::ZeroVector;
	float MarkerRadius = 90.f;
```
> `ARTHexMapActor` è già forward-dichiarata nell'header (Task 1). Aggiungere `#include "Map/RTCellId.h"` non serve (arriva via `Map/RTHexCellData.h`).

- [ ] **Step 3: Implementazione dei bottoni del property set**

In `Source/RefactorTacticsEditor/Private/Tools/RTHexArchTool.cpp`, dopo `Setup()`, impostare il back-pointer e implementare i bottoni. Prima, in `Setup()`, aggiungere dopo `AddToolPropertySource(Properties);`:
```cpp
	Properties->WeakTool = this;
```
Poi aggiungere:
```cpp
void URTHexArchToolProperties::Commit()
{
	if (URTHexArchTool* T = WeakTool.Get()) { T->CommitArch(); }
}

void URTHexArchToolProperties::ClearArch()
{
	if (URTHexArchTool* T = WeakTool.Get()) { T->ClearPending(); }
}
```

- [ ] **Step 4: SMOKE TEST del gizmo — spawn su click, teardown su Shutdown**

Sostituire il corpo di `OnClicked` (no-op del Task 1) e aggiungere `Shutdown`/`DestroyPendingGizmo`. Includere in cima al `.cpp`:
```cpp
#include "InteractiveGizmoManager.h"
#include "BaseGizmos/TransformProxy.h"
#include "BaseGizmos/CombinedTransformGizmo.h"
#include "InteractiveGizmo.h" // ETransformGizmoSubElements
```
`OnClicked`:
```cpp
void URTHexArchTool::OnClicked(const FInputDeviceRay& ClickPos)
{
	ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld);
	if (!Actor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HexMode] Nessun ARTHexMapActor bersaglio."));
		return;
	}
	FRTCellId Cell;
	FVector Center;
	if (!RTHexEditor::ResolveClickedCell(TargetWorld, Actor, ClickPos, Cell, Center)) { return; }

	DestroyPendingGizmo(); // no duplicati su re-click

	TargetActor = Actor;
	From = Cell;
	To = Cell;
	bHasFrom = true;
	bToValid = false;
	FromWorld = Center;
	MarkerRadius = (Actor->MapAsset ? Actor->MapAsset->HexSize : Actor->HexSize) * 0.9f;

	Proxy = NewObject<UTransformProxy>(this);
	Proxy->SetTransform(FTransform(Center));
	Gizmo = GetToolManager()->GetPairedGizmoManager()->CreateCustomTransformGizmo(
		ETransformGizmoSubElements::TranslateAllAxes, this);
	Gizmo->SetActiveTarget(Proxy, nullptr);
	Proxy->OnTransformChanged.AddUObject(this, &URTHexArchTool::OnGizmoMoved);

	if (Properties) { Properties->From = From; Properties->bHasFrom = true; Properties->To = To; Properties->bToValid = false; }
	UE_LOG(LogTemp, Log, TEXT("[HexMode] Arco: From %s, gizmo spawnato."), *From.ToString());
}
```
> **Nota firma**: `RTHexEditor::ResolveClickedCell(UWorld*, ARTHexMapActor*, const FInputDeviceRay&, FRTCellId&, FVector&)`. La chiamata corretta è `RTHexEditor::ResolveClickedCell(TargetWorld, Actor, ClickPos, Cell, Center)`.

`Shutdown` + `DestroyPendingGizmo`:
```cpp
void URTHexArchTool::Shutdown(EToolShutdownType ShutdownType)
{
	DestroyPendingGizmo();
	USingleClickTool::Shutdown(ShutdownType);
}

void URTHexArchTool::DestroyPendingGizmo()
{
	if (GetToolManager() && GetToolManager()->GetPairedGizmoManager())
	{
		GetToolManager()->GetPairedGizmoManager()->DestroyAllGizmosByOwner(this);
	}
	Gizmo = nullptr;
	Proxy = nullptr;
	bHasFrom = false;
	bToValid = false;
	if (Properties) { Properties->bHasFrom = false; Properties->bToValid = false; }
}
```
Aggiungere `OnGizmoMoved` provvisorio (solo per compilare lo smoke test; snap completo allo Step 5):
```cpp
void URTHexArchTool::OnGizmoMoved(UTransformProxy* InProxy, FTransform InTransform)
{
	// riempito allo Step 5
}
```
**Build** (editor chiuso) → `Result: Succeeded`. **Verifica manuale smoke**: attiva tool Arch, clicca una cella → compare un gizmo di traslazione; cambia tool (Select) → il gizmo sparisce (nessun gizmo orfano). Se lo spawn/teardown non funziona, fermarsi qui e riportare (il resto dipende da questo).

- [ ] **Step 5: Snap del `To` nel `OnGizmoMoved`**

Sostituire il corpo di `OnGizmoMoved` con:
```cpp
void URTHexArchTool::OnGizmoMoved(UTransformProxy* InProxy, FTransform InTransform)
{
	if (bSnapping || !TargetActor || !bHasFrom || !InProxy) { return; }

	const URTHexMapAsset* Map = TargetActor->MapAsset;
	const float HexSize = Map ? Map->HexSize : TargetActor->HexSize;
	const float LayerH = Map ? Map->LayerHeight : TargetActor->LayerHeight;
	const FVector Origin = TargetActor->GetActorLocation();

	const FVector W = InTransform.GetLocation();
	const int32 Layer = URTHexLibrary::WorldToLayer(W.Z, Origin.Z, LayerH);
	const FRTCellId Cell = URTHexLibrary::WorldToAxial(W, Origin, HexSize, Layer);
	To = Cell;
	// Valido solo se distinto da From e se ENTRAMBE le celle esistono (Commit scriverebbe altrimenti a vuoto).
	bToValid = (Cell != From) && Map && Map->ContainsCell(Cell) && Map->ContainsCell(From);
	ToWorld = URTHexLibrary::AxialToWorld(Cell, Origin, HexSize, LayerH);

	// Ri-snap del proxy al centro della cella; SetTransform ri-emette OnTransformChanged -> guardia.
	bSnapping = true;
	InProxy->SetTransform(FTransform(ToWorld));
	bSnapping = false;

	if (Properties) { Properties->To = To; Properties->bToValid = bToValid; }
}
```

- [ ] **Step 6: Commit / ClearPending + preview nel Render**

Aggiungere gli include per il write:
```cpp
#include "Map/RTHexCellData.h" // ERTHexTransitionKind
```
Implementare:
```cpp
void URTHexArchTool::CommitArch()
{
	if (!TargetActor || !bHasFrom || !bToValid)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HexMode] Arco: niente da committare (serve From + To valido)."));
		return;
	}
	const ERTHexTransitionKind Kind = Properties ? Properties->Kind : ERTHexTransitionKind::Stair;
	const int32 Cost = Properties ? Properties->Cost : 2;
	const bool bBidir = Properties ? Properties->bBidirectional : true;
	TargetActor->AddTransitionData(From, To, Cost, Kind, bBidir);
	DestroyPendingGizmo();
}

void URTHexArchTool::ClearPending()
{
	DestroyPendingGizmo();
}
```
Sostituire l'INTERO `Render` (del Task 1, che aveva il `return` anticipato su `!Actor||!MapAsset`) con questa versione, che rimuove il return anticipato così il pendente si disegna anche senza asset popolato:
```cpp
void URTHexArchTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (!RenderAPI) { return; }
	FPrimitiveDrawInterface* PDI = RenderAPI->GetPrimitiveDrawInterface();
	if (!PDI) { return; }

	const ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld);

	// Transizioni esistenti (solo se l'asset e' popolato).
	if (Actor && Actor->MapAsset)
	{
		const FVector Origin = Actor->GetActorLocation();
		const float HexSize = Actor->MapAsset->HexSize;
		const float LayerH = Actor->MapAsset->LayerHeight;
		for (const FRTHexEdge& E : Actor->MapAsset->Transitions)
		{
			const FVector A = URTHexLibrary::AxialToWorld(E.From, Origin, HexSize, LayerH);
			const FVector B = URTHexLibrary::AxialToWorld(E.To, Origin, HexSize, LayerH);
			RTHexArchDrawArrow(PDI, A, B, RTHexArchKindColor(E.Kind));
		}
	}

	// Arco pendente (indipendente dall'asset).
	if (bHasFrom)
	{
		RTHexEditor::DrawHexMarker(PDI, FromWorld, MarkerRadius, FColor::Green);
		if (bToValid)
		{
			RTHexEditor::DrawHexMarker(PDI, ToWorld, MarkerRadius, FColor::Blue);
			RTHexArchDrawArrow(PDI, FromWorld, ToWorld, FColor::White);
		}
	}
}
```

- [ ] **Step 7: Build del target Editor (editor chiuso)**

Run: `"D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -project="D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -waitmutex`
Expected: `Result: Succeeded`.

- [ ] **Step 8: Verifica manuale in editor (gate — non headless) + voce PIE**

Con un `ARTHexMapActor` con celle su ≥2 layer (usa `GenerateIntoAsset` su layer diversi): attiva tool **Arch**, `Kind`/`Cost`/`bBidirectional` nel pannello → **clicca** la cella From → **trascina** il gizmo su un'altra cella (anche su un altro layer, alzando Z) → `To`/`bToValid` nel pannello + esagono blu + linea preview → premi **Commit** → la transizione compare (linea colorata) e Undo (Ctrl+Z) la rimuove; **ClearArch** annulla il pendente senza scrivere. Aggiungere a `docs/design/test-manuali-pie.md`:
```markdown
| **PIE-HEX-MODE-E** | Crea transizione via gizmo (H5c.2b) | mode Hex Map, tool Arch, `ARTHexMapActor` con celle su >=2 layer | Click From → gizmo → drag su To (anche altro layer, snap a cella) → Commit crea la transizione (visibile); Undo la rimuove; ClearArch annulla il pendente | ⏳ (branch `feat/hex-grid`, H5c.2b) |
```

- [ ] **Step 9: Aggiornare la roadmap (riga H5)**

In `docs/design/hex-map-roadmap.md`, nella cella Stato della riga **H5**, aggiungere in coda: `H5c.2: URTHexArchTool (transizioni con gizmo) - click From + UCombinedTransformGizmo su To (snap via WorldToLayer+WorldToAxial) + Commit -> AddTransitionData; render archi nel tool. Test RefactorTactics.Hex.WorldToLayer. Verifica editor PIE-HEX-MODE-E/F aperta.`

- [ ] **Step 10: Commit**

```bash
git checkout -- RefactorTactics.uproject   # se risporcato
git add Source/RefactorTacticsEditor/Private/Tools/RTHexArchTool.h \
        Source/RefactorTacticsEditor/Private/Tools/RTHexArchTool.cpp \
        docs/design/test-manuali-pie.md docs/design/hex-map-roadmap.md
git commit -m "feat(hex): H5c.2b - gizmo transizioni (click From + UCombinedTransformGizmo To + Commit)"
```

---

## Self-Review (eseguita)

- **Copertura spec**: §3.1 tool/properties → T1 Step 7 + T2 Step 2-3; §3.2 gizmo (no registrazione, spawn, snap, teardown) → T2 Step 4-6; §3.3 render → T1 Step 8 + T2 Step 6; §3.4 AddTransitionData → T1 Step 6; §3.5 WorldToLayer → T1 Step 3-4 (+test Step 1); §3.6 registrazione tool → T1 Step 9-10; §6 test headless → T1 Step 1-5,11; PIE manuali → T1 Step 12 (F) + T2 Step 8 (E); roadmap → T2 Step 9. Nessun gap.
- **Placeholder**: l'unico "no-op" è `OnClicked` in T1 Step 8, DICHIARATO come guscio (base class stabile per T2), non funzionalità di produzione mancante.
- **Consistenza tipi/firme**: `WorldToLayer`, `AddTransitionData`, `ResolveClickedCell(World,Actor,ClickPos,Cell,Center)`, `OnGizmoMoved(UTransformProxy*,FTransform)`, `CreateCustomTransformGizmo(ETransformGizmoSubElements,this)`, `SetActiveTarget(Proxy,nullptr)`, `DestroyAllGizmosByOwner(this)` — coerenti tra dichiarazione e uso e con gli header UE 5.8 verificati.

## Rischi noti

- **Snap durante il drag** (ri-snap del proxy in `OnGizmoMoved`): il punto più fragile; lo smoke test (T2 Step 4) isola lo spawn/teardown prima; se lo snap "salta", passare all'alternativa a fine-drag (`OnEndTransformEdit` + `ReinitializeGizmoTransform`) o snap solo-preview (gizmo libero, `To` letto al Commit).
- **Modulo editor**: rebuild a editor CHIUSO.
- **`ResolveClickedCell` firma**: confermata da `RTHexEditorClick.h` (H5c.1). Se differisse, allineare la chiamata in T2 Step 4.
