#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "RefactorTactics.h"
#if WITH_EDITOR
#include "ScopedTransaction.h"
#endif

#define LOCTEXT_NAMESPACE "RTHexMap"

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

	// Filtro layer (H4): in ActiveOnly mostra solo il layer attivo, cosi' i piani sovrapposti non si confondono.
	auto PassesLayerFilter = [this](int32 Layer)
	{
		return LayerView == ERTLayerViewMode::AllLayers || Layer == ActiveLayer;
	};

	TArray<FRTCellId> CellIds;
	TArray<int32> Heights;
	if (MapAsset && MapAsset->NumCells() > 0)
	{
		CellIds.Reserve(MapAsset->NumCells());
		Heights.Reserve(MapAsset->NumCells());
		for (const FRTHexCellData& C : MapAsset->Cells)
		{
			if (!PassesLayerFilter(C.Id.Layer))
			{
				continue;
			}
			CellIds.Add(C.Id);
			Heights.Add(C.Height);
		}
	}
	else if (DemoRadius > 0)
	{
		// Demo graybox sul layer attivo (visibile sia in AllLayers sia in ActiveOnly).
		CellIds = URTHexLibrary::HexArea(FRTCellId(0, 0, ActiveLayer), DemoRadius);
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

void ARTHexMapActor::GenerateIntoAsset()
{
	if (!MapAsset)
	{
		UE_LOG(LogRT, Warning, TEXT("[HexMap] Nessun MapAsset assegnato: assegnalo prima di generare."));
		return;
	}
#if WITH_EDITOR
	const FScopedTransaction Transaction(LOCTEXT("HexGenerate", "Hex: Generate Area"));
#endif
	MapAsset->Modify();
	const TArray<FRTCellId> Ids = URTHexLibrary::HexArea(FRTCellId(0, 0, ActiveLayer), FMath::Max(0, DemoRadius));
	for (const FRTCellId& Id : Ids)
	{
		FRTHexCellData Cell(Id);
		Cell.Surface = DemoSurface;
		MapAsset->AddOrUpdateCell(Cell);
	}
	MapAsset->SortCells();
	MapAsset->MarkPackageDirty();
	RebuildInstances();
	UE_LOG(LogRT, Log, TEXT("[HexMap] Generate: %d celle nell'asset (raggio %d, layer %d)."), Ids.Num(), DemoRadius, ActiveLayer);
}

void ARTHexMapActor::ClearAsset()
{
	if (!MapAsset)
	{
		return;
	}
#if WITH_EDITOR
	const FScopedTransaction Transaction(LOCTEXT("HexClear", "Hex: Clear"));
#endif
	MapAsset->Modify();
	MapAsset->Cells.Reset();
	MapAsset->Transitions.Reset();
	MapAsset->MarkPackageDirty();
	RebuildInstances();
	UE_LOG(LogRT, Log, TEXT("[HexMap] Asset svuotato."));
}

void ARTHexMapActor::ValidateAsset()
{
	if (!MapAsset)
	{
		UE_LOG(LogRT, Warning, TEXT("[HexMap] Nessun MapAsset assegnato."));
		return;
	}
	const TArray<FString> Errors = MapAsset->ValidateMap();
	if (Errors.Num() == 0)
	{
		UE_LOG(LogRT, Log, TEXT("[HexMap] Validazione OK: nessun errore (%d celle)."), MapAsset->NumCells());
	}
	for (const FString& E : Errors)
	{
		UE_LOG(LogRT, Warning, TEXT("[HexMap] %s"), *E);
	}
}

void ARTHexMapActor::PaintTargetCell()
{
	PaintCellData(PaintCellTarget, PaintSurface, PaintMoveCost, bPaintBlocksMovement);
}

void ARTHexMapActor::PaintCellData(const FRTCellId& Id, ERTHexSurface Surface, int32 MoveCost, bool bBlocksMovement)
{
	if (!MapAsset)
	{
		UE_LOG(LogRT, Warning, TEXT("[HexMap] Nessun MapAsset assegnato."));
		return;
	}
#if WITH_EDITOR
	const FScopedTransaction Transaction(LOCTEXT("HexPaint", "Hex: Paint Cell"));
#endif
	MapAsset->BeginStroke();
	MapAsset->PaintCellInStroke(Id, Surface, MoveCost, bBlocksMovement);
	MapAsset->EndStroke();
	RebuildInstances();
	UE_LOG(LogRT, Log, TEXT("[HexMap] Paint su %s (superficie %d, costo %d, blocca=%d)."),
		*Id.ToString(), static_cast<int32>(Surface), MoveCost, bBlocksMovement ? 1 : 0);
}

bool ARTHexMapActor::EraseCell(const FRTCellId& Id)
{
	if (!MapAsset)
	{
		UE_LOG(LogRT, Warning, TEXT("[HexMap] Nessun MapAsset assegnato."));
		return false;
	}
	if (!MapAsset->ContainsCell(Id))
	{
		// Niente da cancellare: nessuna transazione no-op sullo stack di Undo.
		return false;
	}
#if WITH_EDITOR
	const FScopedTransaction Transaction(LOCTEXT("HexErase", "Hex: Erase Cell"));
#endif
	MapAsset->BeginStroke();
	const bool bRemoved = MapAsset->EraseCellInStroke(Id);
	MapAsset->EndStroke();
	RebuildInstances();
	UE_LOG(LogRT, Log, TEXT("[HexMap] Erase %s: %s."), *Id.ToString(), bRemoved ? TEXT("rimossa") : TEXT("assente"));
	return bRemoved;
}

void ARTHexMapActor::AddVerticalTransition()
{
	AddTransitionData(TransitionFrom, TransitionTo, FMath::Max(0, TransitionCost), TransitionKind, bTransitionBidirectional);
}

void ARTHexMapActor::AddTransitionData(const FRTCellId& From, const FRTCellId& To, int32 Cost,
	ERTHexTransitionKind Kind, bool bBidirectional)
{
	if (!MapAsset)
	{
		UE_LOG(LogRT, Warning, TEXT("[HexMap] Nessun MapAsset assegnato."));
		return;
	}
	if (!MapAsset->ContainsCell(From) || !MapAsset->ContainsCell(To))
	{
		UE_LOG(LogRT, Warning, TEXT("[HexMap] Transizione %s -> %s: una delle due celle non esiste nell'asset."),
			*From.ToString(), *To.ToString());
		return;
	}
#if WITH_EDITOR
	const FScopedTransaction Transaction(LOCTEXT("HexAddTransition", "Hex: Add Vertical Transition"));
#endif
	MapAsset->Modify();
	MapAsset->AddTransition(From, To, FMath::Max(0, Cost), Kind, bBidirectional);
	MapAsset->MarkPackageDirty();
	RebuildInstances();
	UE_LOG(LogRT, Log, TEXT("[HexMap] Transizione aggiunta %s -> %s (tipo %d, costo %d, bidirezionale=%d)."),
		*From.ToString(), *To.ToString(), static_cast<int32>(Kind), Cost, bBidirectional ? 1 : 0);
}

void ARTHexMapActor::RemoveVerticalTransition()
{
	if (!MapAsset)
	{
		UE_LOG(LogRT, Warning, TEXT("[HexMap] Nessun MapAsset assegnato."));
		return;
	}
#if WITH_EDITOR
	const FScopedTransaction Transaction(LOCTEXT("HexRemoveTransition", "Hex: Remove Vertical Transition"));
#endif
	MapAsset->Modify();
	const bool bRemoved = MapAsset->RemoveTransition(TransitionFrom, TransitionTo, bTransitionBidirectional);
	if (bRemoved)
	{
		MapAsset->MarkPackageDirty();
		RebuildInstances();
	}
	UE_LOG(LogRT, Log, TEXT("[HexMap] Rimozione transizione %s -> %s: %s."),
		*TransitionFrom.ToString(), *TransitionTo.ToString(), bRemoved ? TEXT("rimossa") : TEXT("non trovata"));
}

#undef LOCTEXT_NAMESPACE
