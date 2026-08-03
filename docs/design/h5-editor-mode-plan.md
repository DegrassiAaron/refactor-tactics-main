# Piano di implementazione — H5 Editor Mode dedicato (prima consegna: H5a + H5b)

> **Per worker agentici:** SUB-SKILL RICHIESTA: usare superpowers:subagent-driven-development (consigliato) o
> superpowers:executing-plans per implementare task per task. Gli step usano checkbox (`- [ ]`).

**Obiettivo:** dare alla mappa esagonale un **Editor Mode dedicato** (UEdMode + Interactive Tools Framework) in un
modulo editor-only, con **selezione a click nel viewport** che sostituisce la digitazione di coordinate nel Details.

**Architettura:** nuovo modulo `RefactorTacticsEditor` (Type `Editor`) → `URTHexEditorMode : UEdMode` +
`FRTHexEditorModeToolkit : FModeToolkit`, registrato automaticamente via CDO (`Info = FEditorModeInfo(...)`); il
modulo registra i **comandi** in `StartupModule`. La selezione è un tool ITF `URTHexSelectTool : USingleClickTool`:
al click il ray del viewport colpisce l'ISM (o un piano al layer attivo) → `URTHexLibrary::WorldToAxial` →
lookup nell'asset. Il runtime `RefactorTactics` resta autorevole e invariato; il mode lo *usa* via le sue API già
esportate (`REFACTORTACTICS_API`).

**Tech stack:** UE 5.8.1 C++; moduli `EditorFramework`, `UnrealEd`, `InteractiveToolsFramework`,
`EditorInteractiveToolsFramework`, `Slate/SlateCore`, `LevelEditor`. Scaffold di riferimento verificato su disco:
`Engine/Plugins/Editor/SampleToolsEditorMode` (UE 5.8).

## Global Constraints

- Motore **UE 5.8.1**; `.uproject` `EngineAssociation` deve restare `"5.8"` (ripristinare con `git checkout -- RefactorTactics.uproject` se l'editor lo risporca a GUID).
- Prefissi classi **`RT`/`URT`** (mode/tool/toolkit/commands); documentazione in `docs/`.
- **L'editor non decide gameplay**: il mode scrive solo dati d'asset, e solo via `Modify()`/transaction (Undo/Redo).
- **Dipendenze editor confinate** nel nuovo modulo `Editor`; il runtime **packaged** non deve includerle (garantito dal Type `Editor`). Il Build.cs del modulo runtime resta invariato.
- Determinismo (#4): nessuna logica di turno toccata; conversione hit→cella deterministica (arrotondamento cubico, `URTHexLibrary::WorldToAxial`, già testata da `RefactorTactics.Hex.AxialToWorldRoundTrip`).
- Un commit per task; build Editor verde prima di dichiarare fatto; verifiche interattive dichiarate come manuali (non «dovrebbe funzionare»).
- Build (editor chiuso): `"D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -project="D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -waitmutex`. Cercare `Result: Succeeded`.

---

## Task 1 (H5a): Modulo editor + Editor Mode "guscio" che appare nella toolbar

Crea il modulo `RefactorTacticsEditor` e un `URTHexEditorMode` vuoto (nessun tool) che si registra da solo, appare
nella toolbar dei Modes del Level Editor e si attiva. Nessuna logica di selezione ancora.

**Files:**
- Create: `Source/RefactorTacticsEditor/RefactorTacticsEditor.Build.cs`
- Create: `Source/RefactorTacticsEditor/Public/RefactorTacticsEditorModule.h`
- Create: `Source/RefactorTacticsEditor/Private/RefactorTacticsEditorModule.cpp`
- Create: `Source/RefactorTacticsEditor/Public/RTHexEditorModeCommands.h`
- Create: `Source/RefactorTacticsEditor/Private/RTHexEditorModeCommands.cpp`
- Create: `Source/RefactorTacticsEditor/Public/RTHexEditorMode.h`
- Create: `Source/RefactorTacticsEditor/Private/RTHexEditorMode.cpp`
- Create: `Source/RefactorTacticsEditor/Public/RTHexEditorModeToolkit.h`
- Create: `Source/RefactorTacticsEditor/Private/RTHexEditorModeToolkit.cpp`
- Modify: `RefactorTactics.uproject` (aggiunge il modulo)
- Modify: `docs/design/test-manuali-pie.md` (voce PIE-HEX-MODE-A)

**Interfaces:**
- Produces: modulo `RefactorTacticsEditor`; `URTHexEditorMode` con `static const FEditorModeID EM_RTHexEditorModeId`; `FRTHexEditorModeCommands` (TCommands, `RegisterCommands()` vuoto in H5a, `static GetCommands()`); `FRTHexEditorModeToolkit`.
- Consumes: nulla (il runtime non è ancora usato in H5a).

- [ ] **Step 1: Build.cs del modulo editor**

`Source/RefactorTacticsEditor/RefactorTacticsEditor.Build.cs`:
```csharp
using UnrealBuildTool;

public class RefactorTacticsEditor : ModuleRules
{
	public RefactorTacticsEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core" });

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"InputCore",
			"EditorFramework",
			"UnrealEd",
			"LevelEditor",
			"InteractiveToolsFramework",
			"EditorInteractiveToolsFramework",
			"RefactorTactics" // modulo runtime: URTHexMapAsset / URTHexLibrary / ARTHexMapActor (usati da H5b)
		});
	}
}
```

- [ ] **Step 2: IModuleInterface del modulo**

`Public/RefactorTacticsEditorModule.h`:
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FRefactorTacticsEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
```

`Private/RefactorTacticsEditorModule.cpp`:
```cpp
#include "RefactorTacticsEditorModule.h"
#include "RTHexEditorModeCommands.h"

#define LOCTEXT_NAMESPACE "FRefactorTacticsEditorModule"

void FRefactorTacticsEditorModule::StartupModule()
{
	// Il mode si registra da solo via CDO (Info in costruttore). Qui registriamo solo i comandi dei tool.
	FRTHexEditorModeCommands::Register();
}

void FRefactorTacticsEditorModule::ShutdownModule()
{
	FRTHexEditorModeCommands::Unregister();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRefactorTacticsEditorModule, RefactorTacticsEditor)
```

- [ ] **Step 3: Commands (vuoto in H5a, popolato in H5b)**

`Public/RTHexEditorModeCommands.h`:
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

/** Comandi (bottoni tool) dell'Editor Mode hex. In H5a nessun comando; H5b aggiunge SelectTool. */
class FRTHexEditorModeCommands : public TCommands<FRTHexEditorModeCommands>
{
public:
	FRTHexEditorModeCommands();

	virtual void RegisterCommands() override;
	static TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> GetCommands();

protected:
	TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> Commands;
};
```

`Private/RTHexEditorModeCommands.cpp`:
```cpp
#include "RTHexEditorModeCommands.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "RTHexEditorModeCommands"

FRTHexEditorModeCommands::FRTHexEditorModeCommands()
	: TCommands<FRTHexEditorModeCommands>("RTHexEditorMode",
		NSLOCTEXT("RTHexEditorMode", "RTHexEditorModeCommands", "Hex Map Editor Mode"),
		NAME_None,
		FAppStyle::GetAppStyleSetName())
{
}

void FRTHexEditorModeCommands::RegisterCommands()
{
	// H5a: nessun tool. H5b registrera' qui il comando SelectTool.
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> FRTHexEditorModeCommands::GetCommands()
{
	return FRTHexEditorModeCommands::Get().Commands;
}

#undef LOCTEXT_NAMESPACE
```

- [ ] **Step 4: UEdMode (guscio)**

`Public/RTHexEditorMode.h`:
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Tools/UEdMode.h"
#include "RTHexEditorMode.generated.h"

/**
 * Editor Mode dedicato alla mappa esagonale (UEdMode + Interactive Tools Framework). Non ha autorita' sui dati di
 * gioco: scrive solo sull'asset mappa via transaction. In H5a e' un guscio (nessun tool); H5b aggiunge la selezione.
 */
UCLASS()
class URTHexEditorMode : public UEdMode
{
	GENERATED_BODY()

public:
	const static FEditorModeID EM_RTHexEditorModeId;

	URTHexEditorMode();

	// UEdMode interface
	virtual void Enter() override;
	virtual void CreateToolkit() override;
	virtual TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> GetModeCommands() const override;
};
```

`Private/RTHexEditorMode.cpp`:
```cpp
#include "RTHexEditorMode.h"
#include "RTHexEditorModeToolkit.h"
#include "RTHexEditorModeCommands.h"

#define LOCTEXT_NAMESPACE "RTHexEditorMode"

const FEditorModeID URTHexEditorMode::EM_RTHexEditorModeId = TEXT("EM_RTHexEditorMode");

URTHexEditorMode::URTHexEditorMode()
{
	// Impostare Info nel costruttore E' cio' che registra il mode nella toolbar (nessuna RegisterMode esplicita).
	Info = FEditorModeInfo(
		EM_RTHexEditorModeId,
		LOCTEXT("RTHexEditorModeName", "Hex Map"),
		FSlateIcon(),
		true /*bVisible*/);
}

void URTHexEditorMode::Enter()
{
	UEdMode::Enter();
	// H5b: registrare qui i ToolBuilder con RegisterTool(...).
}

void URTHexEditorMode::CreateToolkit()
{
	Toolkit = MakeShareable(new FRTHexEditorModeToolkit);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> URTHexEditorMode::GetModeCommands() const
{
	return FRTHexEditorModeCommands::Get().GetCommands();
}

#undef LOCTEXT_NAMESPACE
```

- [ ] **Step 5: Toolkit (pannello base)**

`Public/RTHexEditorModeToolkit.h`:
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Toolkits/BaseToolkit.h"
#include "RTHexEditorMode.h"

/** Pannello del mode: in H5a mostra la palette (vuota) e i details del tool attivo (nessuno). */
class FRTHexEditorModeToolkit : public FModeToolkit
{
public:
	FRTHexEditorModeToolkit();

	// FModeToolkit interface
	virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode) override;
	virtual void GetToolPaletteNames(TArray<FName>& PaletteNames) const override;

	// IToolkit interface
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
};
```

`Private/RTHexEditorModeToolkit.cpp`:
```cpp
#include "RTHexEditorModeToolkit.h"

#define LOCTEXT_NAMESPACE "RTHexEditorModeToolkit"

FRTHexEditorModeToolkit::FRTHexEditorModeToolkit()
{
}

void FRTHexEditorModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	FModeToolkit::Init(InitToolkitHost, InOwningMode);
}

void FRTHexEditorModeToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const
{
	PaletteNames.Add(NAME_Default);
}

FName FRTHexEditorModeToolkit::GetToolkitFName() const
{
	return FName("RTHexEditorMode");
}

FText FRTHexEditorModeToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("DisplayName", "Hex Map");
}

#undef LOCTEXT_NAMESPACE
```

- [ ] **Step 6: Registrare il modulo nel `.uproject`**

In `RefactorTactics.uproject`, dentro `"Modules"`, dopo il modulo `RefactorTactics`, aggiungere:
```json
		{
			"Name": "RefactorTacticsEditor",
			"Type": "Editor",
			"LoadingPhase": "Default"
		}
```
(ricordarsi la virgola dopo il blocco precedente; `EngineAssociation` resta `"5.8"`.)

- [ ] **Step 7: Build del target Editor (editor chiuso)**

Run: `"D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -project="D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -waitmutex`
Atteso: `Result: Succeeded` (compila il nuovo modulo + UHT del mode).

- [ ] **Step 8: Verifica manuale in editor (gate — non headless)**

Apri l'editor; nella toolbar dei **Modes** (in alto a sinistra nel Level Editor) deve comparire **"Hex Map"**;
selezionalo → il pannello del mode si apre senza crash (palette vuota). Uscendo dal mode nessun errore in Output Log.
Aggiungere la voce a `docs/design/test-manuali-pie.md`:
```markdown
| **PIE-HEX-MODE-A** | Editor Mode hex appare e si attiva (H5a) | modulo `RefactorTacticsEditor` compilato | Nella toolbar Modes compare «Hex Map»; attivandolo il pannello si apre senza crash (nessun tool) | ⏳ (branch `feat/hex-grid`, H5a) |
```

- [ ] **Step 9: Commit**

```bash
git checkout -- RefactorTactics.uproject   # solo se l'editor l'ha risporcato dopo lo Step 6; poi riapplica lo Step 6 se necessario
git add Source/RefactorTacticsEditor RefactorTactics.uproject docs/design/test-manuali-pie.md
git commit -m "feat(hex): H5a - modulo RefactorTacticsEditor + URTHexEditorMode guscio (UEdMode)"
```
> Nota: lo Step 6 modifica `.uproject` in modo legittimo (aggiunta modulo). Non confondere con il risporco
> `EngineAssociation`→GUID: verificare che dopo il commit `EngineAssociation` sia ancora `"5.8"`.

---

## Task 2 (H5b): Tool di selezione a click nel viewport

Aggiunge `URTHexSelectTool : USingleClickTool`: al click nel viewport, raycast → punto-mondo (ISM o piano del layer
attivo) → `WorldToAxial` sul layer attivo → lookup nell'asset; mostra la cella nel property set del tool e disegna un
marker. Toglie la dipendenza dal Details per la selezione.

**Files:**
- Create: `Source/RefactorTacticsEditor/Private/Tools/RTHexSelectTool.h`
- Create: `Source/RefactorTacticsEditor/Private/Tools/RTHexSelectTool.cpp`
- Modify: `Source/RefactorTacticsEditor/Private/RTHexEditorModeCommands.cpp` (aggiunge il comando `SelectTool`)
- Modify: `Source/RefactorTacticsEditor/Public/RTHexEditorModeCommands.h` (dichiara `TSharedPtr<FUICommandInfo> SelectTool`)
- Modify: `Source/RefactorTacticsEditor/Private/RTHexEditorMode.cpp` (registra il ToolBuilder in `Enter()`)
- Modify: `docs/design/test-manuali-pie.md` (voce PIE-HEX-MODE-B)

**Interfaces:**
- Consumes: `URTHexMapAsset` (`FindCell`, `HexSize`, `LayerHeight`, `Cells`), `URTHexLibrary::WorldToAxial/AxialToWorld`, `ARTHexMapActor` (`MapAsset`, `HexSize`, `LayerHeight`, `GetActorLocation()`) dal modulo runtime; `FRTHexEditorModeCommands::SelectTool`.
- Produces: `URTHexSelectTool`, `URTHexSelectToolBuilder`, `URTHexSelectToolProperties` (con `int32 ActiveLayer`, `FRTCellId SelectedCell`, `bool bSelectedCellExists`).

- [ ] **Step 1: Verifica logica pura già coperta (nessun nuovo test)**

La conversione hit→cella per-layer è `URTHexLibrary::WorldToAxial(World, Origin, HexSize, ActiveLayer)`, già coperta
dal round-trip `RefactorTactics.Hex.AxialToWorldRoundTrip`. Nessuna nuova logica pura ⇒ nessun nuovo Automation test;
il tool è editor-bound e si verifica in editor (Step 6). *(Dichiarazione esplicita DoD: parte non verificabile headless.)*

- [ ] **Step 2: Header del tool**

`Private/Tools/RTHexSelectTool.h`:
```cpp
#pragma once

#include "BaseTools/SingleClickTool.h"
#include "Map/RTCellId.h"
#include "RTHexSelectTool.generated.h"

class ARTHexMapActor;
class IToolsContextRenderAPI;

/** Factory del tool di selezione. */
UCLASS()
class URTHexSelectToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()
public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override { return true; }
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
};

/** Proprieta' del tool: layer attivo (input) + cella selezionata (sola lettura, mostrata nel pannello). */
UCLASS(Transient)
class URTHexSelectToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	/** Layer su cui interpretare il click (piano attivo). */
	UPROPERTY(EditAnywhere, Category = "Hex")
	int32 ActiveLayer = 0;

	/** Ultima cella selezionata (q, r, Layer). */
	UPROPERTY(VisibleAnywhere, Category = "Hex|Selezione")
	FRTCellId SelectedCell;

	/** La cella selezionata esiste nell'asset? */
	UPROPERTY(VisibleAnywhere, Category = "Hex|Selezione")
	bool bSelectedCellExists = false;
};

/**
 * Seleziona una cella cliccando nel viewport: ray -> punto-mondo (ISM o piano del layer attivo) -> WorldToAxial ->
 * lookup. Non modifica dati (sola selezione); un marker evidenzia la cella. La selezione con modifica arriva dopo.
 */
UCLASS()
class URTHexSelectTool : public USingleClickTool
{
	GENERATED_BODY()
public:
	virtual void SetWorld(UWorld* World);
	virtual void Setup() override;
	virtual void OnClicked(const FInputDeviceRay& ClickPos) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;

protected:
	UPROPERTY()
	TObjectPtr<URTHexSelectToolProperties> Properties;

	UWorld* TargetWorld = nullptr;

	/** ARTHexMapActor bersaglio: quello selezionato, altrimenti l'unico presente nel mondo. */
	ARTHexMapActor* FindTargetMapActor() const;

	bool bHasSelection = false;
	FVector SelectedWorldCenter = FVector::ZeroVector;
	float MarkerRadius = 50.f;
};
```

- [ ] **Step 3: Implementazione del tool**

`Private/Tools/RTHexSelectTool.cpp`:
```cpp
#include "Tools/RTHexSelectTool.h"
#include "InteractiveToolManager.h"
#include "ToolContextInterfaces.h"
#include "SceneManagement.h"
#include "Engine/World.h"
#include "Engine/HitResult.h"
#include "EngineUtils.h"       // TActorIterator
#include "Editor.h"           // GEditor
#include "Selection.h"        // USelection
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"

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
	if (TargetWorld)
	{
		for (TActorIterator<ARTHexMapActor> It(TargetWorld); It; ++It)
		{
			if (Found) { return nullptr; } // piu' di uno e nessuno selezionato: ambiguo -> nessuna azione
			Found = *It;
		}
	}
	return Found;
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

	const URTHexMapAsset* Map = Actor->MapAsset;
	const float HexSize = Map ? Map->HexSize : Actor->HexSize;
	const float LayerH = Map ? Map->LayerHeight : Actor->LayerHeight;
	const FVector Origin = Actor->GetActorLocation();
	const int32 Layer = Properties->ActiveLayer;

	// Punto-mondo del click: colpo sull'ISM se c'e', altrimenti intersezione col piano del layer attivo.
	FVector HitPoint;
	const FVector RayStart = ClickPos.WorldRay.Origin;
	const FVector RayEnd = ClickPos.WorldRay.PointAt(999999.0);
	FHitResult Result;
	const bool bHitWorld = TargetWorld && TargetWorld->LineTraceSingleByObjectType(
		Result, RayStart, RayEnd, FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects));
	if (bHitWorld)
	{
		HitPoint = Result.ImpactPoint;
	}
	else
	{
		const double PlaneZ = Origin.Z + static_cast<double>(Layer) * static_cast<double>(LayerH);
		const FPlane LayerPlane(FVector(0, 0, PlaneZ), FVector(0, 0, 1));
		HitPoint = FMath::RayPlaneIntersection(ClickPos.WorldRay.Origin, ClickPos.WorldRay.Direction, LayerPlane);
	}

	const FRTCellId Cell = URTHexLibrary::WorldToAxial(HitPoint, Origin, HexSize, Layer);
	Properties->SelectedCell = Cell;
	Properties->bSelectedCellExists = (Map != nullptr) && (Map->FindCell(Cell) != nullptr);

	// Centro-mondo della cella per il marker (usa la quota del layer, non quella del colpo).
	SelectedWorldCenter = URTHexLibrary::AxialToWorld(Cell, Origin, HexSize, LayerH);
	MarkerRadius = HexSize * 0.9f;
	bHasSelection = true;

	UE_LOG(LogTemp, Log, TEXT("[HexMode] Selezione %s (esiste=%d) su layer %d."),
		*Cell.ToString(), Properties->bSelectedCellExists ? 1 : 0, Layer);
}

void URTHexSelectTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (!bHasSelection || !RenderAPI) { return; }
	FPrimitiveDrawInterface* PDI = RenderAPI->GetPrimitiveDrawInterface();
	if (!PDI) { return; }

	// Esagono di selezione (6 vertici pointy-top) sul piano orizzontale, colore giallo.
	const FColor Color = FColor::Yellow;
	FVector Prev = FVector::ZeroVector;
	for (int32 I = 0; I <= 6; ++I)
	{
		const double Angle = PI / 180.0 * (60.0 * I - 30.0); // pointy-top: primo vertice a -30 gradi
		const FVector V = SelectedWorldCenter + FVector(MarkerRadius * FMath::Cos(Angle), MarkerRadius * FMath::Sin(Angle), 2.0);
		if (I > 0)
		{
			PDI->DrawLine(Prev, V, Color, SDPG_Foreground, 2.0f);
		}
		Prev = V;
	}
}

#undef LOCTEXT_NAMESPACE
```

- [ ] **Step 4: Dichiarare e registrare il comando SelectTool**

In `Public/RTHexEditorModeCommands.h`, dentro la classe (sezione public), aggiungere:
```cpp
	TSharedPtr<FUICommandInfo> SelectTool;
```

In `Private/RTHexEditorModeCommands.cpp`, dentro `RegisterCommands()`, sostituire il corpo con:
```cpp
	TArray<TSharedPtr<FUICommandInfo>>& ToolCommands = Commands.FindOrAdd(NAME_Default);
	UI_COMMAND(SelectTool, "Select", "Seleziona una cella cliccando nel viewport (layer attivo)",
		EUserInterfaceActionType::ToggleButton, FInputChord());
	ToolCommands.Add(SelectTool);
```

- [ ] **Step 5: Registrare il ToolBuilder in `Enter()`**

In `Private/RTHexEditorMode.cpp`, aggiungere gli include in cima:
```cpp
#include "InteractiveToolManager.h"
#include "Tools/RTHexSelectTool.h"
```
e sostituire il corpo di `Enter()` con:
```cpp
	UEdMode::Enter();

	const FRTHexEditorModeCommands& Commands = FRTHexEditorModeCommands::Get();
	RegisterTool(Commands.SelectTool, TEXT("RTHexSelectTool"), NewObject<URTHexSelectToolBuilder>(this));

	GetToolManager()->SelectActiveToolType(EToolSide::Left, TEXT("RTHexSelectTool"));
```

- [ ] **Step 6: Build del target Editor (editor chiuso)**

Run: `"D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -project="D:/Repositories/refactor-tactics-main/RefactorTactics.uproject" -waitmutex`
Atteso: `Result: Succeeded`.

- [ ] **Step 7: Verifica manuale in editor (gate — non headless)**

Apri l'editor; metti un `ARTHexMapActor` nel livello (con `DemoRadius>0` o `MapAsset` popolato). Attiva il mode
**Hex Map** → tool **Select** attivo → imposta `ActiveLayer` nel pannello → **clicca** su una cella nel viewport:
- l'esagono giallo evidenzia la cella cliccata;
- `SelectedCell` nel pannello mostra `(q,r,L)` corretto e `bSelectedCellExists` coerente;
- l'Output Log stampa `[HexMode] Selezione (q=..,r=..,L=..) ...`.
Verifica il click su celle sovrapposte cambiando `ActiveLayer` (L0 vs L1). Aggiungere a `test-manuali-pie.md`:
```markdown
| **PIE-HEX-MODE-B** | Selezione a click nel viewport (H5b) | mode Hex Map attivo, `ARTHexMapActor` nel livello | Click su una cella → esagono giallo + `SelectedCell` corretto nel pannello; cambiando `ActiveLayer` seleziona il piano giusto (celle sovrapposte) | ⏳ (branch `feat/hex-grid`, H5b) |
```

- [ ] **Step 8: Commit**

```bash
git checkout -- RefactorTactics.uproject   # se risporcato
git add Source/RefactorTacticsEditor docs/design/test-manuali-pie.md
git commit -m "feat(hex): H5b - tool selezione a click (USingleClickTool + raycast -> WorldToAxial per layer)"
```

---

## Dopo la prima consegna (fuori scope di questo piano)

H5c+ (piani separati): paint/brush + palette (custom Slate), shape/fill, **strumenti archi con gizmo**
(bridge/scala via `UInteractiveGizmo`), copia/incolla, overlay debug layer. Aggiornare `hex-map-roadmap.md`
(riga H5) al termine di H5a+H5b con lo stato reale e i criteri Done verificati in editor.
