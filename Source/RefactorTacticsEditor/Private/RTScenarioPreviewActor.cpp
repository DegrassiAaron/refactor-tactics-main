#include "RTScenarioPreviewActor.h"

#include "RTScenarioViewportModel.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "ScenarioHarness/RTScenarioDraft.h" // FRTScenarioUnitView
#include "UObject/ConstructorHelpers.h"

namespace
{
	/**
	 * Le stesse misure di `ARTUnit`, non misure nuove: il marcatore d'authoring e l'unita' in partita devono
	 * somigliarsi, altrimenti il designer impara due vocabolari per la stessa cosa.
	 *
	 * ⚠️ **Scala ASSOLUTA e non moltiplicata per `HexSize`**, come in `ARTUnit`. Scalare il marcatore con la
	 * cella sembrerebbe piu' coerente e invece lo romperebbe: con `HexSize = 150` il fattore sarebbe `3`, e
	 * l'anello della squadra 0 passerebbe da 80 uu a 240 contro un passo di griglia di ~260 — due unita'
	 * adiacenti con gli anelli che si toccano.
	 */
	constexpr float UnitCylinderRadius = 50.f;      // il cilindro engine a scala 1
	const FVector BodyScale(1.2f, 1.2f, 1.8f);      // `ARTUnit::BaseMeshScale`
	constexpr float BodyHalfHeight = 1.8f * UnitCylinderRadius; // 90 uu: la base poggia sulla faccia della cella

	constexpr float RingThickness = 0.02f;          // `ARTUnit::TeamRing`, disco piatto
	constexpr float RingLocalZ = 1.0f;              // appena sopra la faccia, che `MarkerTransform` ha gia' raggiunto

	/** Il cuneo sta DAVANTI al corpo: appena fuori dal suo raggio (60 uu), non dentro. */
	constexpr float WedgeForward = 78.f;
	const FVector WedgeScale(0.4f, 0.16f, 0.16f);   // 40 x 16 x 16 uu: si legge, non domina
	constexpr float WedgeLocalZ = 24.f;
}

const FName ARTScenarioPreviewActor::PreviewTag(TEXT("RTScenarioPreview"));

ARTScenarioPreviewActor::ARTScenarioPreviewActor()
{
	PrimaryActorTick.bCanEverTick = false; // ⛔ nessun Tick: l'anteprima si aggiorna quando lo scenario cambia

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	auto MakeLayer = [this, Root](const TCHAR* Name, UStaticMesh* Mesh) -> UInstancedStaticMeshComponent*
	{
		UInstancedStaticMeshComponent* Component = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Component->SetupAttachment(Root);
		// Nessuna collisione: un'anteprima che si puo' cliccare intercetterebbe i click dei tool di disegno,
		// ed e' lo stesso difetto del tag ma in forma di raycast.
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCastShadow(false);
		if (Mesh)
		{
			Component->SetStaticMesh(Mesh);
		}
		return Component;
	};

	Bodies = MakeLayer(TEXT("Bodies"), CylinderMesh.Succeeded() ? CylinderMesh.Object : nullptr);
	TeamRings = MakeLayer(TEXT("TeamRings"), CylinderMesh.Succeeded() ? CylinderMesh.Object : nullptr);
	FacingWedges = MakeLayer(TEXT("FacingWedges"), CubeMesh.Succeeded() ? CubeMesh.Object : nullptr);
}

void ARTScenarioPreviewActor::ClearUnits()
{
	if (Bodies) { Bodies->ClearInstances(); }
	if (TeamRings) { TeamRings->ClearInstances(); }
	if (FacingWedges) { FacingWedges->ClearInstances(); }
}

int32 ARTScenarioPreviewActor::NumMarkers() const
{
	return Bodies ? Bodies->GetInstanceCount() : 0;
}

void ARTScenarioPreviewActor::ShowUnits(const TArray<FRTScenarioUnitView>& Units,
	const FVector& Origin, float HexSize, float LayerHeight)
{
	// Si riparte sempre da zero invece di aggiornare per differenza: uno scenario si riapre intero, e una
	// posa incrementale lascerebbe i marcatori di un'unita' cancellata dove il file non la dichiara piu'.
	ClearUnits();

	if (!Bodies || !TeamRings || !FacingWedges)
	{
		return;
	}

	for (const FRTScenarioUnitView& Unit : Units)
	{
		// L'unico punto in cui si decide DOVE e VERSO DOVE. Il resto qui sotto e' taglia e quota.
		const FTransform Base = RTScenarioViewport::MarkerTransform(
			Unit.Cell, Unit.Facing, Origin, HexSize, LayerHeight);

		FTransform Body(FRotator::ZeroRotator, FVector(0.f, 0.f, BodyHalfHeight), BodyScale);
		Bodies->AddInstance(Body * Base, /*bWorldSpace=*/ true);

		const float RingScale = RTScenarioViewport::TeamRingScale(Unit.TeamId);
		FTransform Ring(FRotator::ZeroRotator, FVector(0.f, 0.f, RingLocalZ),
			FVector(RingScale, RingScale, RingThickness));
		TeamRings->AddInstance(Ring * Base, /*bWorldSpace=*/ true);

		FTransform Wedge(FRotator::ZeroRotator, FVector(WedgeForward, 0.f, WedgeLocalZ), WedgeScale);
		FacingWedges->AddInstance(Wedge * Base, /*bWorldSpace=*/ true);
	}
}
