#include "Tools/RTHexLosTool.h"

#include "RTHexEditorClick.h"
#include "RTHexLosReadout.h"

#include "BaseBehaviors/MouseHoverBehavior.h"
#include "InteractiveToolManager.h"
#include "Map/RTHexLibrary.h"      // AxialToWorld: dove DISEGNARE il punto di blocco, non come deciderlo
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"
#include "PrimitiveDrawingUtils.h" // FPrimitiveDrawInterface / SDPG_*
#include "ToolContextInterfaces.h"

#define LOCTEXT_NAMESPACE "URTHexLosTool"

namespace
{
	/** Via libera: verde. Bloccata: rosso. Il colore ACCOMPAGNA il testo, non lo sostituisce. */
	const FColor ClearColor(60, 220, 90);
	const FColor BlockedColor(230, 70, 60);

	/** Sopra il disco della cella, per la ragione che `RTMapVisuals.h` documenta: sotto, sparirebbe. */
	constexpr float LineLift = 6.0f;
}

UInteractiveTool* URTHexLosToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	URTHexLosTool* NewTool = NewObject<URTHexLosTool>(SceneState.ToolManager);
	NewTool->SetWorld(SceneState.World);
	return NewTool;
}

void URTHexLosTool::SetWorld(UWorld* World)
{
	TargetWorld = World;
}

void URTHexLosTool::Setup()
{
	USingleClickTool::Setup();
	Properties = NewObject<URTHexLosToolProperties>(this);
	AddToolPropertySource(Properties);

	// Il primo hover del progetto. Il click resta di `USingleClickTool`: le due cose non si contendono, una
	// sceglie l'origine e l'altra il bersaglio.
	UMouseHoverBehavior* HoverBehavior = NewObject<UMouseHoverBehavior>(this);
	HoverBehavior->Initialize(this);
	AddInputBehavior(HoverBehavior);
}

ARTHexMapActor* URTHexLosTool::FindTargetMapActor() const
{
	return RTHexEditor::FindTargetMapActor(TargetWorld);
}

void URTHexLosTool::OnClicked(const FInputDeviceRay& ClickPos)
{
	ARTHexMapActor* Actor = FindTargetMapActor();
	if (!Actor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HexLos] Nessun ARTHexMapActor bersaglio (selezionane uno se ce ne sono piu' di uno)."));
		return;
	}

	FRTCellId Cell;
	FVector Center;
	if (!RTHexEditor::ResolveClickedCell(TargetWorld, Actor, ClickPos, Cell, Center))
	{
		return;
	}

	OriginCell = Cell;
	OriginWorld = Center;
	bHasOrigin = true;

	// L'origine e' cambiata: il verdetto precedente parlava di un'altra linea e non vale piu'.
	RefreshReadout();
}

FInputRayHit URTHexLosTool::BeginHoverSequenceHitTest(const FInputDeviceRay& PressPos)
{
	// L'ispettore vuole sapere dove sta il cursore ovunque si trovi, anche fuori dalla mappa: e' proprio
	// uscendo che il pannello deve smettere di mostrare l'ultimo verdetto.
	return FInputRayHit(0.0f);
}

void URTHexLosTool::OnBeginHover(const FInputDeviceRay& DevicePos)
{
	UpdateHoveredCell(DevicePos);
}

bool URTHexLosTool::OnUpdateHover(const FInputDeviceRay& DevicePos)
{
	UpdateHoveredCell(DevicePos);
	return true;
}

void URTHexLosTool::OnEndHover()
{
	if (bHasHovered)
	{
		bHasHovered = false;
		RefreshReadout();
	}
}

void URTHexLosTool::UpdateHoveredCell(const FInputDeviceRay& DevicePos)
{
	ARTHexMapActor* Actor = FindTargetMapActor();

	FRTCellId Cell;
	FVector Center = FVector::ZeroVector;
	const bool bResolved = Actor
		&& RTHexEditor::ResolveClickedCell(TargetWorld, Actor, DevicePos, Cell, Center);

	// 🔴 Il filtro che rende l'hover event-driven invece che per-fotogramma. Il mouse produce eventi mentre
	// si muove DENTRO la stessa cella: senza questa domanda, ogni pixel sarebbe una query LOS.
	if (!RTHexLos::ShouldRequery(bHasHovered, HoveredCell, bResolved, Cell))
	{
		return;
	}

	HoveredCell = Cell;
	HoveredWorld = Center;
	bHasHovered = bResolved;
	RefreshReadout();
}

void URTHexLosTool::RefreshReadout()
{
	if (!Properties)
	{
		return;
	}

	bHasBlockPoint = false;

	FRTLineOfSightResult Los;
	if (bHasOrigin && bHasHovered)
	{
		const ARTHexMapActor* Actor = FindTargetMapActor();
		const URTHexMapAsset* Map = Actor ? Actor->MapAsset : nullptr;

		// 🔑 L'UNICA riga che decide. E' lo stesso attraversamento da cui esce il `bool` di
		// `HasLineOfSight`, quello che bot, combat, arena e percezione usano per decidere davvero.
		Los = URTHexVisionLibrary::DescribeLineOfSight(Map, OriginCell, HoveredCell);
		++QueryCount;

		if (!Los.IsClear() && Actor)
		{
			// Solo per DISEGNARE il punto: la cella colpevole l'ha gia' scelta il runtime.
			FVector MapOrigin = FVector::ZeroVector;
			float HexSize = 0.f;
			float LayerHeight = 0.f;
			Actor->GetHexContext(MapOrigin, HexSize, LayerHeight);
			BlockedWorld = URTHexLibrary::AxialToWorld(Los.BlockedAt, MapOrigin, HexSize, LayerHeight);
			bHasBlockPoint = true;
		}
	}

	const RTHexLos::FReadout Readout =
		RTHexLos::Describe(bHasOrigin, OriginCell, bHasHovered, HoveredCell, Los);

	Properties->Selected = bHasOrigin ? OriginCell.ToString() : TEXT("—");
	Properties->Hovered = bHasHovered ? HoveredCell.ToString() : TEXT("—");
	Properties->LOS = Readout.Verdict;
	Properties->Reason = Readout.Reason;
	Properties->Layer = Readout.Layer;
}

void URTHexLosTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (!RenderAPI) { return; }
	FPrimitiveDrawInterface* PDI = RenderAPI->GetPrimitiveDrawInterface();
	if (!PDI) { return; }

	if (!bHasOrigin)
	{
		return;
	}

	const FVector Lift(0.f, 0.f, LineLift);

	if (!bHasHovered)
	{
		// Solo l'origine scelta: si marca, senza una linea che andrebbe verso niente.
		RTHexEditor::DrawHexMarker(PDI, OriginWorld, 40.f, FColor::Yellow);
		return;
	}

	const bool bClear = Properties && Properties->LOS == TEXT("CLEAR");
	const FColor LineColor = bClear ? ClearColor : BlockedColor;

	PDI->DrawLine(OriginWorld + Lift, HoveredWorld + Lift, LineColor, SDPG_Foreground, 2.0f);
	RTHexEditor::DrawHexMarker(PDI, OriginWorld, 40.f, FColor::Yellow);

	if (bHasBlockPoint)
	{
		// DOVE si ferma, non solo che si ferma: un verdetto senza il punto obbliga a cercarlo a occhio.
		RTHexEditor::DrawHexMarker(PDI, BlockedWorld, 55.f, BlockedColor, 3.0f);
	}
}

#undef LOCTEXT_NAMESPACE
