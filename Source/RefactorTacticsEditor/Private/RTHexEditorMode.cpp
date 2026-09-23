#include "RTHexEditorMode.h"
#include "RTHexEditorModeSettings.h"
#include "RTHexEditorModeToolkit.h"
#include "RTHexEditorModeCommands.h"
#include "RTHexEditorClick.h"
#include "RTHexSelectionStore.h"
#include "RTHexWorkGrid.h"        // #622: la regola della griglia di lavoro, pura e provata headless
#include "RTHexWorkGridActor.h"   // #622: il portatore transiente che la posa
#include "Engine/World.h"         // SpawnActor
#include "Map/RTMapEditLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Editor.h"
#include "ScopedTransaction.h"
#include "InteractiveToolManager.h"
#include "ContextObjectStore.h"
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

	// #921: le impostazioni del mode. `UEdMode::Enter` le istanzia da qui, ne fa `LoadConfig()` e le passa al
	// toolkit, che le mostra SOPRA i tool; `UEdMode::Exit` ne fa `SaveConfig()`. E' il meccanismo canonico per
	// «impostazione del mode», e un flag letto da sette strumenti e' esattamente quello.
	SettingsClass = URTHexEditorModeSettings::StaticClass();
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

	// 🔴 **L'anello che `UEdMode` NON fornisce.** `Enter()` qui sopra ha creato `SettingsObject` e l'ha dato al
	// toolkit, ma non l'ha registrato da nessuna parte che un tool possa interrogare: un `UInteractiveTool`
	// vede il proprio `GetToolManager()`, non il `UEdMode` che l'ha costruito. Il canale che colma il salto e'
	// il context store dell'ITF, ed e' questa riga — senza, i sette `Render` non avrebbero un soggetto.
	if (SettingsObject)
	{
		RTHexEditor::PublishSurfaceOverlaySettings(GetToolManager()->GetContextObjectStore(), SettingsObject);
	}

	// 🔑 **#622, e la posizione di questa riga E' un criterio del DoD**: *«entrando in Hex Map mode la
	// griglia di lavoro e' visibile senza accendere nulla»*. Non poteva stare nel `Render` di un tool —
	// `SelectActiveToolType` qui sopra assegna soltanto il *builder*, e nessun tool e' attivo finche' non
	// si clicca la palette (`InteractiveToolManager.cpp:94-112`), quindi nessuno di quei sette `Render`
	// gira. Sta qui perche' qui e' l'unico momento che «entrare nel mode» significa davvero.
	//
	// ⚠️ **Questa chiamata non e' coperta da nessun automation test**, ed e' il limite noto della fetta:
	// cancellandola la suite resterebbe verde e la griglia non comparirebbe mai. E' la stessa lacuna che
	// `RTHexOverlaySettingsTests.cpp:116-121` dichiara per la pubblicazione del settings, e come quella e'
	// a carico di una voce PIE — qui `PIE-MAPED-GRID`.
	RefreshWorkGrid();
}

void URTHexEditorMode::ModeTick(float /*DeltaTime*/)
{
	// Un confronto per fotogramma; il lavoro vero solo quando la chiave cambia. Il perche' del polling al
	// posto degli hook sta nel docstring di `RTHexWorkGrid::FWatch`.
	RefreshWorkGrid();
}

void URTHexEditorMode::Exit()
{
	// ⚠️ **Questa rimozione e' ridondante, e resta esplicita di proposito.** `UEdMode::Exit()` chiama
	// `DestroyInteractiveToolsContexts()`, che azzera lo store del mode; e `UEdMode::Enter()` ne crea uno
	// NUOVO. ∴ al rientro nel mode il settings vecchio non e' li' comunque, e non esiste il caso «due
	// settings nello store, `FindContext` prende il primo».
	//
	// 🔑 Si toglie lo stesso perche' chi pubblica un oggetto in uno store altrui **dichiara anche quando
	// smette**: affidarsi al fatto che l'Engine distrugga lo store e appoggiarsi a un dettaglio interno che
	// nessun test di questo repository verifica. Va PRIMA di `UEdMode::Exit()`, che e' cio' che smonta il
	// contesto: dopo, non ci sarebbe piu' uno store da cui rimuovere.
	if (SettingsObject && GetToolManager())
	{
		RTHexEditor::WithdrawSurfaceOverlaySettings(GetToolManager()->GetContextObjectStore(), SettingsObject);
	}

	// 🔑 **#622, terzo criterio del DoD**: *«uscire dal mode e rientrare non lascia residui»*. E' vero per
	// costruzione — il portatore nasce `RF_Transient` e non entra nel `.umap` — ma lasciarlo nel mondo
	// dopo l'uscita mostrerebbe una griglia di lavoro a chi non sta piu' lavorando alla mappa. Chi posa
	// dichiara anche quando smette, come per il settings qui sopra.
	if (ARTHexWorkGridActor* Carrier = WorkGrid.Get())
	{
		Carrier->Destroy();
	}
	WorkGrid.Reset();
	WorkGridWatch = RTHexWorkGrid::FWatch();

	UEdMode::Exit();
}

namespace
{
	/**
	 * Il tetto sul numero di esagoni della griglia di lavoro (#622, quarto criterio del DoD).
	 *
	 * 🔑 **Il caso che morde non e' la mappa piena, e' quella SPARSA.** Una board esagonale di raggio 50 ha
	 * `3·50·51+1 = 7651` celle, e dilatarla di due anelli ne aggiunge `6·51 + 6·52 = 618`: il margine su una
	 * mappa compatta e' quasi gratis. Ma 500 celle *isolate* dilatate di due anelli fanno `500 · 18 = 9000`
	 * fantasmi — e li' il tetto riduce di un anello, che e' esattamente cio' che deve fare.
	 *
	 * ⚠️ **Passato come parametro a `BuildPlan` e non letto li' dentro**, perche' un tetto che un test non
	 * puo' stringere e' un tetto che nessun test esercita.
	 */
	constexpr int32 RTWorkGridMaxGhosts = 4096;
}

void URTHexEditorMode::RefreshWorkGrid()
{
	UWorld* World = GetWorld();
	const URTHexEditorModeSettings* Settings = Cast<URTHexEditorModeSettings>(SettingsObject);
	ARTHexMapActor* Map = RTHexEditor::FindTargetMapActor(World);

	RTHexWorkGrid::FWatch Now;
	Now.bShow = Settings != nullptr && Settings->bShowWorkGrid;
	Now.Margin = Settings ? Settings->WorkGridMargin : 0;
	Now.SeedRadius = Settings ? Settings->WorkGridSeedRadius : 0;

	FVector Origin = FVector::ZeroVector;
	float HexSize = 0.f;
	float LayerHeight = 0.f;
	const URTHexMapAsset* Asset = nullptr;

	if (Map)
	{
		Now.MapActorId = Map->GetUniqueID();
		Now.ActiveLayer = Map->ActiveLayer;

		// ⚠️ **Da `GetHexContext` e non dai campi dell'actor**: e' l'unico punto da cui passano le
		// conversioni cella↔mondo, e sa gia' che la scala viene dall'asset quando c'e' (`RTHexMapActor.h:726-732`).
		Asset = Map->GetHexContext(Origin, HexSize, LayerHeight);
		Now.Origin = Origin;
		Now.HexSize = HexSize;
		Now.LayerHeight = LayerHeight;

		if (Asset)
		{
			Now.Revision = Asset->Revision;
			Now.NumCells = Asset->NumCells();
		}
	}

	// ⚠️ **`WorkGridDrawn` e non `bShow` nella guardia.** Un portatore assente e' lo stato normale finche'
	// non c'e' niente da mostrare — senza asset, o con il layer saturo. Leggerlo come «portatore perduto»
	// farebbe rifare il piano a ogni fotogramma, e scrivere una riga di log per ciascuno.
	const bool bCarrierOk = WorkGrid.IsValid() && WorkGrid->GetWorld() == World;
	if (Now == WorkGridWatch && (bCarrierOk || WorkGridDrawn == 0))
	{
		return;
	}
	WorkGridWatch = Now;

	if (!Now.bShow)
	{
		if (ARTHexWorkGridActor* Carrier = WorkGrid.Get())
		{
			Carrier->ClearCells();
		}
		WorkGridDrawn = 0;
		return;
	}

	RTHexWorkGrid::FInput In;
	In.bHasAsset = Asset != nullptr;
	In.Layer = Now.ActiveLayer;
	In.Margin = Now.Margin;
	In.SeedRadius = Now.SeedRadius;
	In.MaxCells = RTWorkGridMaxGhosts;
	if (Asset)
	{
		In.ExistingOnLayer = Asset->CellsInLayer(Now.ActiveLayer);
	}

	const RTHexWorkGrid::FPlan Plan = RTHexWorkGrid::BuildPlan(In);

	if (!bCarrierOk)
	{
		// Un portatore rimasto in un mondo che non e' piu' quello su cui si lavora va tolto, non riusato:
		// il riferimento e' debole, quindi se quel mondo e' gia' sparito qui non c'e' niente da distruggere.
		if (ARTHexWorkGridActor* Stale = WorkGrid.Get())
		{
			Stale->Destroy();
		}
		WorkGrid.Reset();

		if (World && Plan.Cells.Num() > 0)
		{
			// Spawn transiente e fuori dall'outliner, con i parametri di `RTScenarioPreviewSubsystem.cpp:57-61`:
			// il portatore non deve poter finire nel livello salvato.
			FActorSpawnParameters Params;
			Params.ObjectFlags = RF_Transient;
			Params.bHideFromSceneOutliner = true;
			Params.bTemporaryEditorActor = true;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			WorkGrid = World->SpawnActor<ARTHexWorkGridActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
		}
	}

	if (ARTHexWorkGridActor* Carrier = WorkGrid.Get())
	{
		Carrier->ShowCells(Plan.Cells, Origin, HexSize, LayerHeight);
	}
	WorkGridDrawn = Plan.Cells.Num();

	// 🔑 **Una riga oggettiva prima della domanda percettiva.** La seduta `PIE-MAPED-GRID` giudica come la
	// griglia si *legge*; questo log dice se e' stata *posata*, e quante. E' la stessa forma con cui
	// `PIE-MAPED-FRAME` si e' chiusa su `LogRTHexEditorMode: Frame Map: 64 celle su 2 layer.`
	UE_LOG(LogRTHexEditorMode, Log,
		TEXT("Griglia di lavoro: %d esagoni (%s), layer %d, anelli %d/%d%s."),
		Plan.Cells.Num(), RTHexWorkGrid::SourceName(Plan.Source), Now.ActiveLayer,
		Plan.AppliedReach, Plan.RequestedReach,
		Plan.bClamped ? TEXT(" — RIDOTTA dal tetto") : TEXT(""));
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
