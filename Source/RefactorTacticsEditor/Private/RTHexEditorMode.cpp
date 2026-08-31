#include "RTHexEditorMode.h"
#include "RTHexEditorModeToolkit.h"
#include "RTHexEditorModeCommands.h"
#include "RTHexEditorClick.h"
#include "RTHexSelectionStore.h"
#include "Map/RTMapEditLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Editor.h"
#include "ScopedTransaction.h"
#include "InteractiveToolManager.h"
#include "Tools/RTHexSelectTool.h"
#include "Tools/RTHexPaintTool.h"
#include "Tools/RTHexArchTool.h"
#include "Tools/RTHexGeometryTool.h"
#include "Tools/RTHexFillTool.h"
#include "Tools/RTHexLosTool.h"
#include "Tools/RTHexProbeTool.h"

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
	// #1755: sesto tool. Sola lettura — non tocca la mappa, la interroga.
	RegisterTool(Commands.LosTool, TEXT("RTHexLosTool"), NewObject<URTHexLosToolBuilder>(this));
	// #711: settimo tool. Sola lettura come il sesto — interroga il movimento, non lo cambia.
	RegisterTool(Commands.ProbeTool, TEXT("RTHexProbeTool"), NewObject<URTHexProbeToolBuilder>(this));

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

	Toolkit->GetToolkitCommands()->MapAction(
		FRTHexEditorModeCommands::Get().EraseSelection,
		FExecuteAction::CreateUObject(this, &URTHexEditorMode::EraseSelection));
}

void URTHexEditorMode::EraseSelection()
{
	ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(GetWorld());
	URTHexMapAsset* Map = Actor ? Actor->MapAsset : nullptr;
	URTHexSelectionStore* Store = GEditor ? GEditor->GetEditorSubsystem<URTHexSelectionStore>() : nullptr;

	if (!Map || !Store || Store->GetSelection().Num() == 0)
	{
		UE_LOG(LogRTHexEditorMode, Warning,
			TEXT("Erase: niente da cancellare (mappa assente o selezione vuota)."));
		return;
	}

	// UNA sola transazione per l'intera operazione, cascata compresa: e' il criterio di #1864 — «una
	// operazione composta e' un solo Undo». Aprirne una per elemento farebbe premere Ctrl+Z tante volte
	// quanti erano gli elementi, e l'autore non sa quanti ne ha portati via la cascata.
	const FScopedTransaction Transaction(
		NSLOCTEXT("RTHexEditorMode", "EraseSelection", "Cancella la selezione"));
	Map->Modify();

	int32 Applied = 0;
	for (const FRTMapElementHandle& Handle : Store->GetSelection())
	{
		const ERTMapEditOutcome Outcome = URTMapEditLibrary::DeleteElement(Map, Handle);
		if (Outcome == ERTMapEditOutcome::Applied)
		{
			++Applied;
		}
		else
		{
			// ⚠️ Il rifiuto si LOGGA con la sua ragione, non si ingoia: e' la disciplina di
			// `ERTHexArchPendingCloseReason` (#996). Un elemento che sparisce dalla selezione senza essere
			// stato cancellato, e senza che nessuno lo dica, e' il difetto silenzioso peggiore.
			UE_LOG(LogRTHexEditorMode, Warning,
				TEXT("Erase: rifiutato '%s' (esito %d)."),
				*URTHexSelectionStore::Describe({ Handle }), static_cast<int32>(Outcome));
		}
	}

	Store->Clear();

	// La vista non si aggiorna da sola: l'asset non notifica l'actor (stessa trappola delle porte in
	// `PIE-HEX-VIZ-PORTE`, dove senza un ridisegno forzato si guarda la geometria vecchia). Stesso gesto di
	// `SetActiveLayer`, che per la stessa ragione chiama `RebuildInstances` dopo aver scritto.
	Actor->RebuildInstances();

	UE_LOG(LogRTHexEditorMode, Log, TEXT("Erase: %d elementi cancellati."), Applied);
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

	// Le celle INTERE, non i soli id: l'overload su `FRTHexCellData` tiene conto della quota d'autore, che
	// `RebuildInstances` applica al render. Con i soli id una mappa con celle alzate verrebbe inquadrata
	// piatta sul piano del layer, e le piu' alte resterebbero fuori.
	const FBox Bounds = URTHexLibrary::CellsBoundsWorld(Map->Cells, Origin, HexSize, LayerHeight);
	if (Bounds.IsValid == 0)
	{
		UE_LOG(LogRTHexEditorMode, Warning, TEXT("Frame Map: l'asset mappa non ha celle."));
		return;
	}

	// ⚠️ Si inquadrano TUTTE le celle, anche quando `LayerView` ne disegna solo una parte — in `ActiveOnly`
	// il filtro instanzia il solo piano attivo. Non e' una svista: `PIE-MAPED-FRAME` chiede esattamente
	// *«tutte le celle esistenti, comprese quelle su layer diversi da `ActiveLayer`»*, quindi restringere
	// al disegnato violerebbe il criterio. Il prezzo e' che in `ActiveOnly` la camera indietreggia per
	// includere geometria non visibile, e chi guarda vede lo stesso piano piu' lontano senza capire perche':
	// per questo il log dice **quanti** piani sono entrati nel conto.
	TSet<int32> Layers;
	for (const FRTHexCellData& Cell : Map->Cells)
	{
		Layers.Add(Cell.Id.Layer);
	}
	UE_LOG(LogRTHexEditorMode, Log, TEXT("Frame Map: %d celle su %d layer."),
		Map->Cells.Num(), Layers.Num());

	if (GEditor)
	{
		GEditor->MoveViewportCamerasToBox(Bounds, /*bActiveViewportOnly*/ true);
	}
}

#undef LOCTEXT_NAMESPACE
