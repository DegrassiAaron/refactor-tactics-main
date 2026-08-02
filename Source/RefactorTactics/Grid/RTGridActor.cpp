#include "Grid/RTGridActor.h"
#include "Grid/RTGridLibrary.h"
#include "Core/RTTypes.h"
#include "Terrain/RTTerrainLibrary.h"
#include "RefactorTactics.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
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

	// Celle del ponte (layer 1): mesh calpestabile a quota, con collisione per il click->layer.
	BridgeCells = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BridgeCells"));
	BridgeCells->SetupAttachment(Cells);
	BridgeCells->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BridgeCells->SetCollisionResponseToAllChannels(ECR_Block);
	if (PlaneMesh.Succeeded())
	{
		BridgeCells->SetStaticMesh(PlaneMesh.Object);
	}

	// Coperture centrali di default per il demo (bloccano la linea di tiro).
	BlockedCells = { FRTGridCoord(4, 4), FRTGridCoord(5, 4), FRTGridCoord(4, 5), FRTGridCoord(5, 5) };
}

void ARTGridActor::BeginPlay()
{
	Super::BeginPlay();
	SpawnBridge();
	SpawnDemoTerrain();
	BuildGrid();
}

void ARTGridActor::SpawnBridge()
{
	Layer1Cells.Reset();
	Edges.Reset();

	// Passerella (layer 1) lungo y=4, da x=3 a x=7: passa sopra le coperture centrali.
	// NB: non parte da x=2 per non stare sopra lo spawn del Guardian (2,4), che resterebbe non cliccabile.
	for (int32 X = 3; X <= 7; ++X)
	{
		Layer1Cells.Add(FRTGridCoord(X, 4, 1));
	}

	// Rampe (archi bidirezionali, costo 2) alle due estremita': terra <-> ponte.
	auto AddRamp = [this](const FRTGridCoord& Ground, const FRTGridCoord& Bridge)
	{
		Edges.Add(FRTTraversalEdge(Ground, Bridge, 2));
		Edges.Add(FRTTraversalEdge(Bridge, Ground, 2));
	};
	AddRamp(FRTGridCoord(2, 4, 0), FRTGridCoord(3, 4, 1)); // base sotto/accanto al Guardian di team 0
	AddRamp(FRTGridCoord(8, 4, 0), FRTGridCoord(7, 4, 1));

	UE_LOG(LogRT, Log, TEXT("[RT] GridActor: ponte (%d celle layer 1, %d archi)"), Layer1Cells.Num(), Edges.Num());
}

const URTTerrainData* ARTGridActor::GetTerrainAt(const FRTGridCoord& Cell) const
{
	const TObjectPtr<URTTerrainData>* Found = TerrainCells.Find(Cell);
	return Found ? Found->Get() : nullptr;
}

int32 ARTGridActor::LayerFromHitComponent(const UPrimitiveComponent* Comp) const
{
	return (Comp && Comp == BridgeCells) ? 1 : 0;
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

	// Layer 1: solo le celle-ponte sono calpestabili; tutte le altre di layer 1 sono impassabili
	// (altrimenti il pathfinding le tratterebbe come costo 1 e le unita' "galleggerebbero").
	TSet<FRTGridCoord> BridgeSet(Layer1Cells);
	for (int32 X = 0; X < Width; ++X)
	{
		for (int32 Y = 0; Y < Height; ++Y)
		{
			const FRTGridCoord Cell(X, Y, 1);
			if (!BridgeSet.Contains(Cell))
			{
				OutCost.Add(Cell, RT_BLOCKED_COST);
			}
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

void ARTGridActor::BuildBotCostMap(TMap<FRTGridCoord, int32>& OutCost) const
{
	BuildCostMap(OutCost);
	// Il bot evita del tutto gli hazard: li tratta come impassabili in pianificazione.
	for (const TPair<FRTGridCoord, TObjectPtr<URTTerrainData>>& Pair : TerrainCells)
	{
		if (Pair.Value && Pair.Value->GetProps().EndTurnDamage > 0)
		{
			OutCost.Add(Pair.Key, RT_BLOCKED_COST);
		}
	}
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

void ARTGridActor::SetTerrainAt(const FRTGridCoord& Cell, URTTerrainData* Terrain, int32 TurnsLeft)
{
	if (Terrain) { TerrainCells.Add(Cell, Terrain); }
	else { TerrainCells.Remove(Cell); }
	if (TurnsLeft > 0) { TerrainTurnsLeft.Add(Cell, TurnsLeft); }
	else { TerrainTurnsLeft.Remove(Cell); }
}

void ARTGridActor::TickTerrain()
{
	TArray<FRTGridCoord> Expired;
	for (TPair<FRTGridCoord, int32>& It : TerrainTurnsLeft)
	{
		if (--It.Value <= 0) { Expired.Add(It.Key); }
	}
	for (const FRTGridCoord& Cell : Expired)
	{
		const URTTerrainData* Cur = GetTerrainAt(Cell);
		URTTerrainData* Revert = Cur ? Cur->RevertsTo.Get() : nullptr;
		SetTerrainAt(Cell, Revert, Revert ? Revert->Props.TransientDuration : 0);
	}
}

void ARTGridActor::SpawnDemoTerrain()
{
	TerrainCells.Reset();
	TerrainTurnsLeft.Reset();

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

	// Lava: hazard permanente - danno a chi vi termina il turno.
	URTTerrainData* Lava = NewObject<URTTerrainData>(this);
	Lava->DisplayName = FText::FromString(TEXT("Lava"));
	Lava->DisplayColor = FLinearColor(0.85f, 0.20f, 0.05f, 1.f);
	Lava->Props.CrossDamage = 10;   // danno ad attraversarla
	Lava->Props.EndTurnDamage = 20; // + danno a terminarci sopra (double-dip)
	for (const FRTGridCoord& C : { FRTGridCoord(1, 4), FRTGridCoord(1, 5) })
	{
		TerrainCells.Add(C, Lava);
	}

	// Fuoco: hazard temporaneo (2 turni), generato incendiando l'Erba; poi la cella torna normale.
	URTTerrainData* Fuoco = NewObject<URTTerrainData>(this);
	Fuoco->DisplayName = FText::FromString(TEXT("Fuoco"));
	Fuoco->DisplayColor = FLinearColor(1.0f, 0.45f, 0.05f, 1.f);
	Fuoco->Props.CrossDamage = 8;
	Fuoco->Props.EndTurnDamage = 15;
	Fuoco->Props.TransientDuration = 2;
	Fuoco->RevertsTo = nullptr; // brucia -> terreno normale

	// Erba secca: infiammabile -> diventa Fuoco se colpita da un'abilita' che incendia.
	URTTerrainData* Erba = NewObject<URTTerrainData>(this);
	Erba->DisplayName = FText::FromString(TEXT("Erba secca"));
	Erba->DisplayColor = FLinearColor(0.55f, 0.65f, 0.15f, 1.f);
	Erba->Props.bFlammable = true;
	Erba->IgnitesTo = Fuoco;
	for (const FRTGridCoord& C : { FRTGridCoord(6, 6), FRTGridCoord(7, 6) })
	{
		TerrainCells.Add(C, Erba);
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

	// Ponte: celle di layer 1 renderizzate a quota (LayerHeight), calpestabili e cliccabili.
	if (BridgeCells)
	{
		BridgeCells->ClearInstances();
		for (const FRTGridCoord& Cell : Layer1Cells)
		{
			FVector World = URTGridLibrary::CellToWorld(Cell, Origin, CellSize);
			World.Z += GridZOffset + Cell.Layer * LayerHeight;
			FTransform T;
			T.SetLocation(World);
			T.SetScale3D(FVector(Scale, Scale, 1.f));
			BridgeCells->AddInstance(T, /*bWorldSpace=*/ true);
		}
	}

	RefreshTerrainVisuals();

	UE_LOG(LogRT, Log, TEXT("[RT] GridActor: %d celle, %d ostacoli, %d ponte (%dx%d, cella %.0f uu)"),
		Count, BlockedCells.Num(), Layer1Cells.Num(), Width, Height, CellSize);
}

void ARTGridActor::RefreshTerrainVisuals()
{
	// Rimuove i piani precedenti (il terreno puo' essere cambiato: ignite/reversione).
	for (UStaticMeshComponent* Comp : TerrainVisuals)
	{
		if (Comp) { Comp->DestroyComponent(); }
	}
	TerrainVisuals.Reset();

	if (!Cells || !Cells->GetStaticMesh())
	{
		return;
	}
	UStaticMesh* Plane = Cells->GetStaticMesh();
	UMaterialInterface* Base = TerrainMaterial.LoadSynchronous();
	const FVector Origin = GetActorLocation();
	constexpr float PlaneBaseSize = 100.f;
	const float Scale = (CellSize * 0.9f) / PlaneBaseSize;

	for (const TPair<FRTGridCoord, TObjectPtr<URTTerrainData>>& Pair : TerrainCells)
	{
		if (!Pair.Value) { continue; }

		UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this);
		Comp->SetupAttachment(Cells);
		Comp->RegisterComponent();
		Comp->SetStaticMesh(Plane);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // i click passano alla griglia sotto

		FVector World = URTGridLibrary::CellToWorld(Pair.Key, Origin, CellSize);
		World.Z += 4.f; // appena sopra le celle base
		Comp->SetWorldLocation(World);
		Comp->SetWorldScale3D(FVector(Scale, Scale, 1.f));

		if (Base)
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, this);
			MID->SetVectorParameterValue(TEXT("Color"), Pair.Value->DisplayColor);
			Comp->SetMaterial(0, MID);
		}
		TerrainVisuals.Add(Comp);
	}
}
