#include "Tools/RTHexArchTool.h"
#include "RTHexEditorClick.h"
#include "InteractiveToolManager.h"
#include "ToolContextInterfaces.h"
#include "PrimitiveDrawingUtils.h" // FPrimitiveDrawInterface / SDPG_*
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"
#include "InteractiveGizmoManager.h"
#include "BaseGizmos/TransformProxy.h"
#include "BaseGizmos/CombinedTransformGizmo.h"
#include "InteractiveGizmo.h" // ETransformGizmoSubElements
#include "Map/RTHexCellData.h" // ERTHexTransitionKind

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
	Properties->WeakTool = this;
}

void URTHexArchToolProperties::Commit()
{
	if (URTHexArchTool* T = WeakTool.Get()) { T->CommitArch(); }
}

void URTHexArchToolProperties::ClearArch()
{
	if (URTHexArchTool* T = WeakTool.Get()) { T->ClearPending(); }
}

void URTHexArchTool::OnClicked(const FInputDeviceRay& ClickPos)
{
	ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld);
	if (!Actor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HexMode] Nessun ARTHexMapActor bersaglio."));
		return;
	}
	if (Properties && Properties->Operation == ERTHexArchOp::Remove)
	{
		RemoveNearestArch(Actor, ClickPos);
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

void URTHexArchTool::RemoveNearestArch(ARTHexMapActor* Actor, const FInputDeviceRay& ClickPos)
{
	DestroyPendingGizmo(); // esci da un eventuale Add pendente

	const URTHexMapAsset* Map = Actor->MapAsset;
	if (!Map || Map->Transitions.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[HexMode] Remove: nessuna transizione nell'asset."));
		return;
	}

	const FVector Origin = Actor->GetActorLocation();
	const float HexSize = Map->HexSize;
	const float LayerH = Map->LayerHeight;
	const FVector RayO = ClickPos.WorldRay.Origin;
	const FVector RayD = ClickPos.WorldRay.Direction;

	int32 BestIdx = INDEX_NONE;
	float BestDist = TNumericLimits<float>::Max();
	for (int32 I = 0; I < Map->Transitions.Num(); ++I)
	{
		const FRTHexEdge& E = Map->Transitions[I];
		const FVector A = URTHexLibrary::AxialToWorld(E.From, Origin, HexSize, LayerH);
		const FVector B = URTHexLibrary::AxialToWorld(E.To, Origin, HexSize, LayerH);
		const float Dist = URTHexLibrary::DistanceRayToSegment(RayO, RayD, A, B);
		if (Dist < BestDist) { BestDist = Dist; BestIdx = I; }
	}

	if (BestIdx != INDEX_NONE && BestDist <= HexSize * 0.6f)
	{
		// Copia From/To PRIMA di rimuovere (RemoveTransitionData muta l'array Transitions).
		const FRTCellId F = Map->Transitions[BestIdx].From;
		const FRTCellId T = Map->Transitions[BestIdx].To;
		Actor->RemoveTransitionData(F, T, /*bBothDirections=*/true);
		UE_LOG(LogTemp, Log, TEXT("[HexMode] Arco rimosso %s -> %s (dist %.1f)."), *F.ToString(), *T.ToString(), BestDist);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[HexMode] Nessun arco entro soglia (min dist %.1f)."), BestDist);
	}
}

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

#undef LOCTEXT_NAMESPACE
