#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ARTHexMapActor::ARTHexMapActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Cells = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Cells"));
	SetRootComponent(Cells);
	Cells->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // per il raycast di selezione (H2)
	Cells->SetCollisionResponseToAllChannels(ECR_Block);

	// Fallback graybox: cilindro engine (appiattito a disco in RebuildInstances).
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Cells->SetStaticMesh(CylinderMesh.Object);
	}
}

void ARTHexMapActor::BeginPlay()
{
	Super::BeginPlay();
	RebuildInstances();
}

FRTCellId ARTHexMapActor::CellForInstance(int32 InstanceIndex) const
{
	return InstanceCells.IsValidIndex(InstanceIndex) ? InstanceCells[InstanceIndex] : FRTCellId();
}

void ARTHexMapActor::RebuildInstances()
{
	if (!Cells)
	{
		return;
	}

	// Mesh configurabile con fallback (nessun materiale/mesh obbligatorio hardcoded).
	if (UStaticMesh* Mesh = CellMesh.LoadSynchronous())
	{
		Cells->SetStaticMesh(Mesh);
	}

	Cells->ClearInstances();
	InstanceCells.Reset();

	// Sorgente celle: l'asset se popolato, altrimenti un graybox demo (esagono pieno di raggio DemoRadius).
	const float UseHexSize = MapAsset ? MapAsset->HexSize : HexSize;
	const float UseLayerH = MapAsset ? MapAsset->LayerHeight : LayerHeight;

	TArray<FRTCellId> CellIds;
	TArray<int32> Heights;
	if (MapAsset && MapAsset->NumCells() > 0)
	{
		CellIds.Reserve(MapAsset->NumCells());
		Heights.Reserve(MapAsset->NumCells());
		for (const FRTHexCellData& C : MapAsset->Cells)
		{
			CellIds.Add(C.Id);
			Heights.Add(C.Height);
		}
	}
	else if (DemoRadius > 0)
	{
		CellIds = URTHexLibrary::HexArea(FRTCellId(0, 0, 0), DemoRadius);
		Heights.Init(0, CellIds.Num());
	}

	// Cilindro engine: raggio 50 uu, mezza-altezza 50 uu. Scala X,Y per coprire ~l'esagono, Z sottile (disco).
	const float PlanarScale = UseHexSize / 50.f * 0.95f;
	const float FlatScale = 0.05f;

	for (int32 I = 0; I < CellIds.Num(); ++I)
	{
		FVector World = URTHexLibrary::AxialToWorld(CellIds[I], GetActorLocation(), UseHexSize, UseLayerH);
		World.Z += static_cast<double>(Heights[I]);
		const FTransform Xf(FRotator::ZeroRotator, World, FVector(PlanarScale, PlanarScale, FlatScale));
		Cells->AddInstance(Xf, /*bWorldSpace=*/ true);
		InstanceCells.Add(CellIds[I]);
	}
}
