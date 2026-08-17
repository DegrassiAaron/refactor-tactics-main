#include "RTHexEditorMode.h"
#include "RTHexEditorModeToolkit.h"
#include "RTHexEditorModeCommands.h"
#include "RTHexEditorClick.h"
#include "InteractiveToolManager.h"
#include "Tools/RTHexSelectTool.h"
#include "Tools/RTHexPaintTool.h"
#include "Tools/RTHexArchTool.h"
#include "Tools/RTHexGeometryTool.h"
#include "Tools/RTHexFillTool.h"

#include "Editor.h"                 // GEditor->MoveViewportCamerasToBox
#include "Toolkits/BaseToolkit.h"   // FModeToolkit::GetToolkitCommands
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogRTHexEditorMode, Log, All);

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

	const FRTHexEditorModeCommands& Commands = FRTHexEditorModeCommands::Get();
	RegisterTool(Commands.SelectTool, TEXT("RTHexSelectTool"), NewObject<URTHexSelectToolBuilder>(this));
	RegisterTool(Commands.PaintTool, TEXT("RTHexPaintTool"), NewObject<URTHexPaintToolBuilder>(this));
	RegisterTool(Commands.ArchTool, TEXT("RTHexArchTool"), NewObject<URTHexArchToolBuilder>(this));
	RegisterTool(Commands.FillTool, TEXT("RTHexFillTool"), NewObject<URTHexFillToolBuilder>(this));
	RegisterTool(Commands.GeometryTool, TEXT("RTHexGeometryTool"), NewObject<URTHexGeometryToolBuilder>(this));

	GetToolManager()->SelectActiveToolType(EToolSide::Left, TEXT("RTHexSelectTool"));
}

void URTHexEditorMode::CreateToolkit()
{
	Toolkit = MakeShareable(new FRTHexEditorModeToolkit);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> URTHexEditorMode::GetModeCommands() const
{
	return FRTHexEditorModeCommands::Get().GetCommands();
}

void URTHexEditorMode::BindCommands()
{
	// Prima quelli di `UEdMode` — fra cui `F` sulla selezione: sovrascriverli sarebbe togliere navigazione
	// che il viewport gia' fornisce, ed e' esattamente cio' che `#623` vieta.
	UEdMode::BindCommands();

	// `UEdMode::Toolkit` e' gia' un `TSharedPtr<FModeToolkit>`: nessun cast.
	if (!Toolkit.IsValid())
	{
		return; // senza toolkit non c'e' una command list a cui appendere: `UEdMode::BindCommands` fa lo stesso
	}

	Toolkit->GetToolkitCommands()->MapAction(
		FRTHexEditorModeCommands::Get().FrameMap,
		FExecuteAction::CreateUObject(this, &URTHexEditorMode::FrameEditableMap));
}

void URTHexEditorMode::FrameEditableMap()
{
	ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(GetWorld());
	if (!Actor)
	{
		// Stessa condizione che i tool gia' trattano cosi': nessun actor, oppure piu' d'uno e nessuno
		// selezionato — che e' ambiguo, non vuoto.
		UE_LOG(LogRTHexEditorMode, Warning,
			TEXT("Frame Map: nessun ARTHexMapActor bersaglio (assente, o piu' d'uno senza selezione)."));
		return;
	}

	FVector Origin = FVector::ZeroVector;
	float HexSize = 0.f;
	float LayerHeight = 0.f;
	const URTHexMapAsset* Map = Actor->GetHexContext(Origin, HexSize, LayerHeight);
	if (!Map)
	{
		// ⚠️ Il `DemoRadius` dell'actor NON e' una mappa, e inquadrarlo sarebbe la vista che mente contro cui
		// `#622` e il brief d'editor mettono in guardia: la vista mostrerebbe 61 esagoni dove l'asset non ha
		// nessuna cella. Meglio non muovere la camera e dirlo.
		UE_LOG(LogRTHexEditorMode, Warning,
			TEXT("Frame Map: l'actor non ha un MapAsset. Il DemoRadius e' una vista, non dati editabili."));
		return;
	}

	TArray<FRTCellId> Ids;
	Ids.Reserve(Map->Cells.Num());
	for (const FRTHexCellData& Cell : Map->Cells)
	{
		Ids.Add(Cell.Id);
	}

	const FBox Bounds = URTHexLibrary::CellsBoundsWorld(Ids, Origin, HexSize, LayerHeight);
	if (Bounds.IsValid == 0)
	{
		UE_LOG(LogRTHexEditorMode, Warning, TEXT("Frame Map: l'asset mappa non ha celle."));
		return;
	}

	if (GEditor)
	{
		GEditor->MoveViewportCamerasToBox(Bounds, /*bActiveViewportOnly*/ true);
	}
}

#undef LOCTEXT_NAMESPACE
