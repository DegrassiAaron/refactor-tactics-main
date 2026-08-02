#include "Unit/RTUnit.h"
#include "Grid/RTGridLibrary.h"
#include "RefactorTactics.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Il cilindro base dell'engine e' alto ~100 uu; con scala Z 1.8 diventa ~180 (meta' = 90).
	constexpr float UnitHalfHeight = 90.f;
}

ARTUnit::ARTUnit()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->SetRelativeScale3D(FVector(1.2f, 1.2f, 1.8f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CylinderMesh.Object);
	}
}

void ARTUnit::BeginPlay()
{
	Super::BeginPlay();
	ApplyTeamColor();
}

void ARTUnit::ApplyTeamColor()
{
	if (Mesh && Mesh->GetStaticMesh())
	{
		DynMaterial = Mesh->CreateDynamicMaterialInstance(0);
		if (DynMaterial)
		{
			// Il materiale base delle BasicShapes espone il parametro "Color".
			DynMaterial->SetVectorParameterValue(TEXT("Color"), TeamId == 0 ? Team0Color : Team1Color);
		}
	}
}

void ARTUnit::PlaceOnCell(const FRTGridCoord& Cell, const FVector& GridOrigin, float CellSize)
{
	GridCell = Cell;
	const FVector Center = URTGridLibrary::CellToWorld(Cell, GridOrigin, CellSize);
	SetActorLocation(Center + FVector(0.f, 0.f, UnitHalfHeight));
}

void ARTUnit::OnSelected()
{
	UE_LOG(LogRT, Log, TEXT("[RT] Unit selezionata: team %d, cella (%d,%d)"), TeamId, GridCell.X, GridCell.Y);
	SetActorScale3D(FVector(1.15f)); // feedback visivo minimale
}

void ARTUnit::OnDeselected()
{
	SetActorScale3D(FVector(1.0f));
}
