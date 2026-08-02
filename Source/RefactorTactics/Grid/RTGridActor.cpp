#include "Grid/RTGridActor.h"
#include "Grid/RTGridLibrary.h"
#include "Core/RTTypes.h"
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
	BuildGrid();
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
