#include "Grid/RTGridActor.h"
#include "Grid/RTGridLibrary.h"
#include "Core/RTTypes.h"
#include "Terrain/RTTerrainLibrary.h"
#include "RefactorTactics.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ARTGridActor::ARTGridActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Cells = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Cells"));
	SetRootComponent(Cells);
	Cells->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Cells->SetCollisionResponseToAllChannels(ECR_Block);

	// Mesh di default: il Plane base dell'engine (100x100 uu). Sostituibile nel dettaglio dell'attore.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh.Succeeded())
	{
		Cells->SetStaticMesh(PlaneMesh.Object);
	}

	Obstacles = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Obstacles"));
	Obstacles->SetupAttachment(Cells);
	Obstacles->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Obstacles->SetCollisionResponseToAllChannels(ECR_Block);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Obstacles->SetStaticMesh(CubeMesh.Object);
	}

	// Coperture centrali di default per il demo (bloccano la linea di tiro).
	BlockedCells = { FRTGridCoord(4, 4), FRTGridCoord(5, 4), FRTGridCoord(4, 5), FRTGridCoord(5, 5) };
}

void ARTGridActor::BeginPlay()
{
	Super::BeginPlay();
	SpawnDemoTerrain();
	BuildGrid();
}

const URTTerrainData* ARTGridActor::GetTerrainAt(const FRTGridCoord& Cell) const
{
	const TObjectPtr<URTTerrainData>* Found = TerrainCells.Find(Cell);
	return Found ? Found->Get() : nullptr;
}

void ARTGridActor::BuildCostMap(TMap<FRTGridCoord, int32>& OutCost) const
{
	OutCost.Reset();
	for (const FRTGridCoord& Blocked : BlockedCells)
	{
		OutCost.Add(Blocked, RT_BLOCKED_COST);
	}
	for (const TPair<FRTGridCoord, TObjectPtr<URTTerrainData>>& Pair : TerrainCells)
	{
		if (Pair.Value)
		{
			OutCost.Add(Pair.Key, URTTerrainLibrary::CellMoveCost(Pair.Value->GetProps()));
		}
	}
}

TArray<FRTGridCoord> ARTGridActor::GetMoveBlockers() const
{
	TArray<FRTGridCoord> Out = BlockedCells;
	for (const TPair<FRTGridCoord, TObjectPtr<URTTerrainData>>& Pair : TerrainCells)
	{
		if (Pair.Value && URTTerrainLibrary::BlocksMovement(Pair.Value->GetProps()))
		{
			Out.AddUnique(Pair.Key);
		}
	}
	return Out;
}

TArray<FRTGridCoord> ARTGridActor::GetVisionBlockers() const
{
	TArray<FRTGridCoord> Out = BlockedCells;
	for (const TPair<FRTGridCoord, TObjectPtr<URTTerrainData>>& Pair : TerrainCells)
	{
		if (Pair.Value && URTTerrainLibrary::BlocksVision(Pair.Value->GetProps()))
		{
			Out.AddUnique(Pair.Key);
		}
	}
	return Out;
}

void ARTGridActor::SpawnDemoTerrain()
{
	TerrainCells.Reset();

	// Fango: attraversamento piu' caro (costo 2). Striscia centrale tra le due squadre.
	URTTerrainData* Fango = NewObject<URTTerrainData>(this);
	Fango->DisplayName = FText::FromString(TEXT("Fango"));
	Fango->DisplayColor = FLinearColor(0.40f, 0.26f, 0.13f, 1.f);
	Fango->Props.ExtraMoveCost = 1;
	for (const FRTGridCoord& C : { FRTGridCoord(6, 3), FRTGridCoord(6, 4), FRTGridCoord(6, 5) })
	{
		TerrainCells.Add(C, Fango);
	}

	// Cespuglio: blocca la linea di tiro, non il movimento. Siepe laterale.
	URTTerrainData* Cespuglio = NewObject<URTTerrainData>(this);
	Cespuglio->DisplayName = FText::FromString(TEXT("Cespuglio"));
	Cespuglio->DisplayColor = FLinearColor(0.10f, 0.50f, 0.15f, 1.f);
	Cespuglio->Props.bBlocksVision = true;
	for (const FRTGridCoord& C : { FRTGridCoord(3, 6), FRTGridCoord(4, 6) })
	{
		TerrainCells.Add(C, Cespuglio);
	}

	// Altura: chi ci sta sopra infligge +10 danno (buff, valutato pre-movimento).
	URTTerrainData* Altura = NewObject<URTTerrainData>(this);
	Altura->DisplayName = FText::FromString(TEXT("Altura"));
	Altura->DisplayColor = FLinearColor(0.60f, 0.60f, 0.72f, 1.f);
	Altura->Props.OccupantDamageBonus = 10;
	for (const FRTGridCoord& C : { FRTGridCoord(4, 2), FRTGridCoord(5, 2) })
	{
		TerrainCells.Add(C, Altura);
	}

	UE_LOG(LogRT, Log, TEXT("[RT] GridActor: terreno demo (%d celle)"), TerrainCells.Num());
}

void ARTGridActor::BuildGrid()
{
	if (!Cells)
	{
		return;
	}

	Cells->ClearInstances();

	const FVector Origin = GetActorLocation();
	constexpr float PlaneBaseSize = 100.f;             // dimensione del Plane base in uu
	constexpr float GridZOffset = 2.f;                 // solleva la griglia per evitare z-fighting col pavimento
	const float Scale = (CellSize * 0.95f) / PlaneBaseSize; // 5% di margine tra le celle

	int32 Count = 0;
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			FVector World = URTGridLibrary::CellToWorld(FRTGridCoord(X, Y), Origin, CellSize);
			World.Z += GridZOffset;
			FTransform CellTransform;
			CellTransform.SetLocation(World);
			CellTransform.SetScale3D(FVector(Scale, Scale, 1.f));
			Cells->AddInstance(CellTransform, /*bWorldSpace=*/ true);
			++Count;
		}
	}

	// Ostacoli: un cubo su ogni cella-copertura, alto abbastanza da bloccare la vista.
	if (Obstacles)
	{
		Obstacles->ClearInstances();
		constexpr float CubeBaseSize = 100.f; // dimensione del Cube base in uu
		const float XYScale = (CellSize * 0.9f) / CubeBaseSize;
		const float ZScale = 250.f / CubeBaseSize; // ~250 uu di altezza
		for (const FRTGridCoord& Blocked : BlockedCells)
		{
			FVector World = URTGridLibrary::CellToWorld(Blocked, Origin, CellSize);
			World.Z += GridZOffset + (250.f * 0.5f); // base appoggiata sopra la griglia
			FTransform T;
			T.SetLocation(World);
			T.SetScale3D(FVector(XYScale, XYScale, ZScale));
			Obstacles->AddInstance(T, /*bWorldSpace=*/ true);
		}
	}

	UE_LOG(LogRT, Log, TEXT("[RT] GridActor: %d celle, %d ostacoli (%dx%d, cella %.0f uu)"),
		Count, BlockedCells.Num(), Width, Height, CellSize);
}
