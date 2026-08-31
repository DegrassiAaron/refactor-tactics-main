#include "Tools/RTHexProbeTool.h"

#include "RTHexEditorClick.h"
#include "RTHexProbeReadout.h"

#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "BaseBehaviors/MouseHoverBehavior.h"
#include "InteractiveToolManager.h"
#include "Map/RTHexLibrary.h"      // AxialToWorld: dove DISEGNARE, non cosa decidere
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "PrimitiveDrawingUtils.h" // FPrimitiveDrawInterface / SDPG_*
#include "ToolContextInterfaces.h"
#include "Turn/RTHexSimLibrary.h"

#define LOCTEXT_NAMESPACE "URTHexProbeTool"

namespace
{
	/** Il ventaglio: azzurro tenue. La cella sorvolata e il suo percorso: giallo. Esclusa: rosso. */
	const FColor ReachColor(70, 160, 230);
	const FColor PathColor(240, 210, 60);
	const FColor ExcludedColor(230, 70, 60);

	/** Sopra il disco della cella, per la ragione che `RTMapVisuals.h` documenta: sotto, sparirebbe. */
	constexpr float ProbeLift = 6.0f;

	/** L'unico id di simulazione che questa sonda usa: c'e' una sola unita' nello snapshot. */
	constexpr int32 ProbeUnitId = 0;
}

UInteractiveTool* URTHexProbeToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	URTHexProbeTool* NewTool = NewObject<URTHexProbeTool>(SceneState.ToolManager);
	NewTool->SetWorld(SceneState.World);
	return NewTool;
}

void URTHexProbeTool::SetWorld(UWorld* World)
{
	TargetWorld = World;
}

void URTHexProbeTool::Setup()
{
	USingleClickTool::Setup();
	Properties = NewObject<URTHexProbeToolProperties>(this);

	// Il primo eroe del roster come partenza: un `HeroId` vuoto avrebbe mostrato budget 0 e un pannello che
	// sembra rotto prima ancora di essere usato. Il roster e' il dato, non un default d'editor.
	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
	if (Roster.Num() > 0 && Roster[0])
	{
		Properties->HeroId = Roster[0]->HeroId;
	}
	RefreshBudgetFromCatalog();

	AddToolPropertySource(Properties);

	UMouseHoverBehavior* HoverBehavior = NewObject<UMouseHoverBehavior>(this);
	HoverBehavior->Initialize(this);
	AddInputBehavior(HoverBehavior);
}

ARTHexMapActor* URTHexProbeTool::FindTargetMapActor() const
{
	return RTHexEditor::FindTargetMapActor(TargetWorld);
}

void URTHexProbeTool::RefreshBudgetFromCatalog()
{
	if (!Properties)
	{
		return;
	}

	// 🔴 **L'UNICO punto che interroga il catalogo, e si chiama solo quando l'eroe cambia.** La prima
	// stesura chiedeva il roster da dentro `MakeProbeSnapshot`, cioe' DUE volte per ogni cella sorvolata:
	// `GetHeroRoster()` costruisce quattro `URTHeroData` e una `NewObject<URTActionData>` per ciascuna delle
	// loro azioni, quindi erano decine di UObject per movimento del mouse. Misurato dall'esterno: mentre la
	// sonda era in uso il game thread non rispondeva piu' — le chiamate al ponte MCP, che di norma tornano
	// in meno di un secondo, scadevano a sessanta.
	const RTHexProbe::FBudget Resolved = RTHexProbe::ResolveBudget(Properties->HeroId);
	bKnownHero = Resolved.bKnown;
	Properties->Budget = Resolved.Points;
}

FRTHexSnapshot URTHexProbeTool::MakeProbeSnapshot(const URTHexMapAsset* Map) const
{
	// Il budget e' gia' derivato e vive nel pannello: richiederlo al catalogo qui significherebbe
	// ricostruire il roster a ogni chiamata, e questa funzione viene chiamata per ogni cella sorvolata.
	const int32 Budget = Properties ? Properties->Budget : 0;
	FRTHexSimUnit Unit(ProbeUnitId, StartCell, Budget);
	return URTHexSimLibrary::MakeSnapshot(Map, { Unit });
}

void URTHexProbeTool::RebuildReachableSet(const FRTHexSnapshot& Snapshot)
{
	ReachableSet.Reset();
	if (!bHasStart || !Snapshot.Map)
	{
		return;
	}

	// ⚠️ Qui non c'e' un algoritmo, c'e' una **domanda**. Budget, blocchi, occupanti e archi li ha gia'
	// applicati il servizio runtime.
	ReachableSet = URTHexSimLibrary::ReachableCells(Snapshot, ProbeUnitId);
}

void URTHexProbeTool::OnClicked(const FInputDeviceRay& ClickPos)
{
	ARTHexMapActor* Actor = FindTargetMapActor();
	if (!Actor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HexProbe] Nessun ARTHexMapActor bersaglio (selezionane uno se ce ne sono piu' di uno)."));
		return;
	}

	FRTCellId Cell;
	FVector Center;
	if (!RTHexEditor::ResolveClickedCell(TargetWorld, Actor, ClickPos, Cell, Center))
	{
		return;
	}

	StartCell = Cell;
	StartWorld = Center;
	bHasStart = true;

	// La partenza e' cambiata: il ventaglio precedente descriveva un'altra domanda.
	RefreshReadout();
}

void URTHexProbeTool::OnPropertyModified(UObject* PropertySet, FProperty* Property)
{
	// Cambiare eroe cambia il budget, e con esso l'intero ventaglio. Il campo `Budget` si riallinea da solo:
	// e' derivato, e mostrarlo stantio sarebbe peggio che non mostrarlo.
	// L'eroe puo' essere cambiato: e' l'UNICO evento che giustifica una lettura del catalogo.
	RefreshBudgetFromCatalog();
	RefreshReadout();
}

FInputRayHit URTHexProbeTool::BeginHoverSequenceHitTest(const FInputDeviceRay& PressPos)
{
	// La sonda vuole sapere dove sta il cursore ovunque si trovi, anche fuori dalla mappa: e' proprio uscendo
	// che il pannello deve smettere di mostrare l'ultima risposta.
	return FInputRayHit(0.0f);
}

void URTHexProbeTool::OnBeginHover(const FInputDeviceRay& DevicePos)
{
	UpdateHoveredCell(DevicePos);
}

bool URTHexProbeTool::OnUpdateHover(const FInputDeviceRay& DevicePos)
{
	UpdateHoveredCell(DevicePos);
	return true;
}

void URTHexProbeTool::OnEndHover()
{
	if (bHasHovered)
	{
		bHasHovered = false;
		RefreshReadout();
	}
}

void URTHexProbeTool::UpdateHoveredCell(const FInputDeviceRay& DevicePos)
{
	ARTHexMapActor* Actor = FindTargetMapActor();

	FRTCellId Cell;
	FVector Center = FVector::ZeroVector;
	const bool bResolved = Actor
		&& RTHexEditor::ResolveClickedCell(TargetWorld, Actor, DevicePos, Cell, Center);

	// 🔴 Il filtro che rende l'hover event-driven invece che per-fotogramma. Senza, per ogni pixel su una
	// cella esclusa e libera partirebbe un A* a costo illimitato.
	if (!RTHexProbe::ShouldRequery(bHasHovered, HoveredCell, bResolved, Cell))
	{
		return;
	}

	HoveredCell = Cell;
	bHasHovered = bResolved;
	RefreshReadout();
}

void URTHexProbeTool::RefreshReadout()
{
	if (!Properties)
	{
		return;
	}

	HoverPathWorld.Reset();

	const ARTHexMapActor* Actor = FindTargetMapActor();
	const URTHexMapAsset* Map = Actor ? Actor->MapAsset : nullptr;

	// 🔴 **UNO snapshot per evento, e serve a entrambe le domande.** Il ventaglio si rifa' a ogni domanda —
	// la mappa puo' essere cambiata sotto, e non c'e' una cache da invalidare — ma `MakeSnapshot` ricalcola
	// l'hash dell'intera mappa: costruirlo due volte era lo spreco gemello di quello sul catalogo.
	const FRTHexSnapshot Snapshot = Map ? MakeProbeSnapshot(Map) : FRTHexSnapshot();
	RebuildReachableSet(Snapshot);

	ERTHexProbeExclusion Exclusion = ERTHexProbeExclusion::NoRoute;
	int32 Cost = 0;
	int32 PathCells = 0;

	if (bHasStart && bHasHovered && Map)
	{
		// 🔑 Le UNICHE due righe che decidono, ed entrambe stanno nel runtime.
		Exclusion = URTHexSimLibrary::ClassifyProbeCell(Snapshot, ProbeUnitId, ReachableSet, HoveredCell);
		const TArray<FRTCellId> Path = URTHexSimLibrary::ProbePathTo(ReachableSet, HoveredCell);
		++QueryCount;

		PathCells = Path.Num();
		if (const FRTHexReachableCell* Reached = ReachableSet.FindByPredicate(
			[this](const FRTHexReachableCell& R) { return R.Cell == HoveredCell; }))
		{
			Cost = Reached->Cost;
		}

		if (Actor && PathCells > 0)
		{
			// Solo per DISEGNARE: le celle del percorso le ha gia' scelte il Dijkstra del runtime.
			FVector MapOrigin = FVector::ZeroVector;
			float HexSize = 0.f;
			float LayerHeight = 0.f;
			Actor->GetHexContext(MapOrigin, HexSize, LayerHeight);

			HoverPathWorld.Reserve(PathCells);
			for (const FRTCellId& Step : Path)
			{
				HoverPathWorld.Add(URTHexLibrary::AxialToWorld(Step, MapOrigin, HexSize, LayerHeight));
			}
		}
	}

	const RTHexProbe::FReadout Readout = RTHexProbe::Describe(
		bHasStart && bHasHovered, Exclusion, Cost, Properties->Budget, PathCells, bKnownHero);

	Properties->Start = bHasStart ? StartCell.ToString() : TEXT("—");
	Properties->Hovered = bHasHovered ? HoveredCell.ToString() : TEXT("—");
	Properties->Reachable = ReachableSet.Num();
	Properties->Cost = Readout.Cost;
	Properties->Steps = Readout.Steps;
	Properties->Reason = Readout.Reason;
}

void URTHexProbeTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (!RenderAPI) { return; }
	FPrimitiveDrawInterface* PDI = RenderAPI->GetPrimitiveDrawInterface();
	if (!PDI || !bHasStart) { return; }

	const ARTHexMapActor* Actor = FindTargetMapActor();
	if (!Actor) { return; }

	FVector MapOrigin = FVector::ZeroVector;
	float HexSize = 0.f;
	float LayerHeight = 0.f;
	Actor->GetHexContext(MapOrigin, HexSize, LayerHeight);

	const FVector Lift(0.f, 0.f, ProbeLift);

	// Il ventaglio. La partenza si marca a parte piu' sotto: qui e' una cella come le altre.
	for (const FRTHexReachableCell& Reach : ReachableSet)
	{
		const FVector Center = URTHexLibrary::AxialToWorld(Reach.Cell, MapOrigin, HexSize, LayerHeight);
		RTHexEditor::DrawHexMarker(PDI, Center, 30.f, ReachColor);
	}

	// Il percorso in hover, se la cella si raggiunge.
	for (int32 i = 1; i < HoverPathWorld.Num(); ++i)
	{
		PDI->DrawLine(HoverPathWorld[i - 1] + Lift, HoverPathWorld[i] + Lift, PathColor, SDPG_Foreground, 2.0f);
	}
	if (HoverPathWorld.Num() > 0)
	{
		RTHexEditor::DrawHexMarker(PDI, HoverPathWorld.Last(), 45.f, PathColor, 2.0f);
	}
	else if (bHasHovered)
	{
		// Sorvolata ma esclusa: si marca in rosso. Il PERCHE' e' una riga del pannello — il colore
		// distingue i due casi, non li spiega, e cinque motivi non stanno in una tinta.
		const FVector Center = URTHexLibrary::AxialToWorld(HoveredCell, MapOrigin, HexSize, LayerHeight);
		RTHexEditor::DrawHexMarker(PDI, Center, 45.f, ExcludedColor, 2.0f);
	}

	RTHexEditor::DrawHexMarker(PDI, StartWorld, 40.f, FColor::Yellow, 2.0f);
}

#undef LOCTEXT_NAMESPACE
