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
