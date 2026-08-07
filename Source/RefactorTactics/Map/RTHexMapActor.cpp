#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "DrawDebugHelpers.h" // anteprima di pianificazione (presentazione, non logica)
#include "EngineUtils.h" // TActorIterator
#include "UObject/ConstructorHelpers.h"
#include "RefactorTactics.h"
#if WITH_EDITOR
#include "ScopedTransaction.h"
#endif

#define LOCTEXT_NAMESPACE "RTHexMap"

namespace
{
	/**
	 * Geometria del disco che rappresenta una cella. Il cilindro dell'engine ha mezza-altezza 50 uu ed e'
	 * CENTRATO sull'origine: con `RTCellFlatScale` la sua faccia superiore sta a `RTCellTopZ` sopra il centro
	 * della cella.
	 *
	 * Perche' sono costanti condivise e non numeri sparsi: le linee di debug disegnate SOTTO `RTCellTopZ`
	 * finiscono dentro il disco e diventano invisibili. E' successo davvero — il contorno della superficie era
	 * a 2.0 con la faccia a 2.5, quindi fango e acqua non si vedevano mentre i marcatori a 3.0 si vedevano.
	 * Legare i lift a questa costante fa si' che cambiare lo spessore del disco non riapra il difetto.
	 */
	constexpr float RTCellFlatScale = 0.05f;
	constexpr float RTCellTopZ = 50.f * RTCellFlatScale; // 2.5 uu

	/** Quote di disegno, tutte sopra la faccia del disco e in ordine di priorita' di lettura. */
	constexpr float RTLiftSurface = RTCellTopZ + 0.5f;  // contorno della superficie (contesto)
	constexpr float RTLiftMarker  = RTCellTopZ + 1.5f;  // blocca-movimento / blocca-vista
	constexpr float RTLiftPreview = RTCellTopZ + 2.5f;  // anteprima di pianificazione (sopra a tutto)
}

ARTHexMapActor::ARTHexMapActor()
{
	// Tick abilitabile ma SPENTO all'avvio: si accende solo quando c'e' un'anteprima da disegnare
	// (SetHoveredCell/SetPreviewPath), cosi' la mappa resta inerte fuori dalla pianificazione.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

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

void ARTHexMapActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// L'asset e' la fonte autorevole; le istanze ISM ne sono solo la VISTA. Ricostruirla a ogni costruzione
	// (apertura del livello, spostamento dell'actor, undo, spawn in gioco) tiene le due cose allineate senza
	// dover premere RebuildInstances a mano. Copre anche il caso del gioco: SpawnActor chiama OnConstruction.
	RebuildInstances();

#if WITH_EDITOR
	BindToMapAsset();
#endif
}

#if WITH_EDITOR
void ARTHexMapActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Cambiare MapAsset, ActiveLayer, LayerView, DemoRadius, HexSize/LayerHeight o CellMesh cambia cosa si deve
	// vedere: si ricostruisce sempre (l'actor ha poche proprieta' e la ricostruzione e' idempotente).
	BindToMapAsset(); // se e' cambiato l'asset, si seguono le notifiche di quello nuovo
	RebuildInstances();
}

void ARTHexMapActor::BindToMapAsset()
{
	if (BoundAsset.Get() == MapAsset)
	{
		return; // gia' iscritti all'asset giusto
	}
	UnbindFromMapAsset();
	if (MapAsset)
	{
		MapChangedHandle = MapAsset->OnMapChanged.AddUObject(this, &ARTHexMapActor::RebuildInstances);
		BoundAsset = MapAsset;
	}
}

void ARTHexMapActor::UnbindFromMapAsset()
{
	if (URTHexMapAsset* Previous = BoundAsset.Get())
	{
		Previous->OnMapChanged.Remove(MapChangedHandle);
	}
	MapChangedHandle.Reset();
	BoundAsset = nullptr;
}

void ARTHexMapActor::BeginDestroy()
{
	UnbindFromMapAsset();
	Super::BeginDestroy();
}
#endif

FRTCellId ARTHexMapActor::CellForInstance(int32 InstanceIndex) const
{
	return InstanceCells.IsValidIndex(InstanceIndex) ? InstanceCells[InstanceIndex] : FRTCellId();
}

ARTHexMapActor* ARTHexMapActor::FindInWorld(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ARTHexMapActor> It(const_cast<UWorld*>(World)); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

void ARTHexMapActor::SetHoveredCell(const FRTCellId& Cell, bool bValid)
{
	HoveredCell = Cell;
	bHoveredValid = bValid;
	// Il tick serve solo mentre c'e' qualcosa da disegnare: fuori dalla pianificazione l'actor resta inerte.
	SetActorTickEnabled(HasAnythingToDraw());
}

void ARTHexMapActor::SetPreviewPath(const TArray<FRTCellId>& Path)
{
	PreviewPath = Path;
	SetActorTickEnabled(HasAnythingToDraw());
}

bool ARTHexMapActor::HasAnythingToDraw() const
{
	return bCellOverlay
		|| bHoveredValid
		|| PreviewPath.Num() > 0
		|| PreviewHitCells.Num() > 0
		|| PreviewReachable.Num() > 0;
}

void ARTHexMapActor::SetPreviewHitCells(const TArray<FRTCellId>& HitCells, const TArray<FRTCellId>& AllyCells)
{
	// Si copia e basta: le celle arrivano gia' calcolate da URTHexCombatLibrary::HexHitCells. Rifiltrarle qui
	// sarebbe un secondo calcolo, e due calcoli della stessa cosa prima o poi divergono (invariante #1).
	PreviewHitCells = HitCells;
	PreviewAllyHitCells = AllyCells;
	SetActorTickEnabled(HasAnythingToDraw());
}

void ARTHexMapActor::SetPreviewReachableCells(const TArray<FRTCellId>& ReachableCells)
{
	PreviewReachable = ReachableCells;
	SetActorTickEnabled(HasAnythingToDraw());
}

void ARTHexMapActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bCellOverlay)
	{
		DrawCellOverlay(); // prima: l'anteprima di pianificazione deve restare leggibile SOPRA
	}
	DrawPlanningPreview();
}

void ARTHexMapActor::SetCellOverlayEnabled(bool bEnabled)
{
	bCellOverlay = bEnabled;
	SetActorTickEnabled(HasAnythingToDraw());
}

void ARTHexMapActor::DrawCellOverlay() const
{
	const UWorld* World = GetWorld();
	if (!World || !MapAsset)
	{
		return; // senza mappa d'autore non c'e' nulla di informativo da mostrare
	}

	FVector Origin = FVector::ZeroVector;
	float Size = 0.f;
	float LayerH = 0.f;
	GetHexContext(Origin, Size, LayerH);

	const auto DrawRing = [World, &Origin, Size, LayerH](const FRTCellId& Cell, const FColor& Color, float Scale,
		float Lift, float Thickness)
	{
		const FVector Center = URTHexLibrary::AxialToWorld(Cell, Origin, Size, LayerH) + FVector(0, 0, Lift);
		const TArray<FVector> Corners = URTHexLibrary::HexCorners(Center, Size * Scale);
		for (int32 I = 0; I < Corners.Num(); ++I)
		{
			DrawDebugLine(World, Corners[I], Corners[(I + 1) % Corners.Num()], Color,
				/*bPersistentLines=*/ false, /*LifeTime=*/ -1.f, /*DepthPriority=*/ 0, Thickness);
		}
	};

	for (const FRTHexCellData& Cell : MapAsset->Cells)
	{
		// `Height` alza l'ISTANZA (RebuildInstances), quindi deve alzare anche le linee: senza, su una cella
		// rialzata il contorno resterebbe sepolto di `Height` unita' dentro il disco.
		const float CellLift = static_cast<float>(Cell.Height);

		// Contorno esterno: che superficie e'. Il costo di traversata si legge dalla superficie, non da un numero.
		// La scala e' 0.90 (non 0.86) per stare appena FUORI dal disco, che copre 0.95 del raggio: cosi' il
		// contorno non lotta con la faccia superiore per lo stesso pixel.
		DrawRing(Cell.Id, URTHexLibrary::SurfaceColor(Cell.Surface), 0.90f, CellLift + RTLiftSurface, /*Thickness=*/ 2.0f);

		// Due marcatori DISTINTI, perche' sono due regole diverse: dove non si passa e dove non si vede.
		if (Cell.bBlocksMovement)
		{
			DrawRing(Cell.Id, URTHexLibrary::BlockedCellColor(), 0.45f, CellLift + RTLiftMarker, 2.5f);
		}
		if (Cell.bBlocksLineOfSight)
		{
			DrawRing(Cell.Id, URTHexLibrary::SightBlockerColor(), 0.64f, CellLift + RTLiftMarker, 2.0f);
		}
	}
}

void ARTHexMapActor::DrawPlanningPreview() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector Origin = FVector::ZeroVector;
	float Size = 0.f;
	float LayerH = 0.f;
	GetHexContext(Origin, Size, LayerH);

	// Quota di una cella: `Height` alza l'istanza, quindi deve alzare anche le linee (senza, l'anteprima
	// sprofonda dentro il disco delle celle rialzate).
	const URTHexMapAsset* Map = MapAsset;
	const auto CellLift = [Map](const FRTCellId& Cell) -> float
	{
		const FRTHexCellData* Data = Map ? Map->FindCell(Cell) : nullptr;
		return Data ? static_cast<float>(Data->Height) : 0.f;
	};

	// Contorno di una cella, dai vertici condivisi con il marker dell'editor (stesso orientamento).
	const auto DrawCellOutline = [World, &Origin, Size, LayerH, &CellLift](const FRTCellId& Cell, const FColor& Color, float Scale)
	{
		const FVector Center = URTHexLibrary::AxialToWorld(Cell, Origin, Size, LayerH)
			+ FVector(0, 0, CellLift(Cell) + RTLiftPreview);
		const TArray<FVector> Corners = URTHexLibrary::HexCorners(Center, Size * Scale);
		for (int32 I = 0; I < Corners.Num(); ++I)
		{
			DrawDebugLine(World, Corners[I], Corners[(I + 1) % Corners.Num()], Color,
				/*bPersistentLines=*/ false, /*LifeTime=*/ -1.f, /*DepthPriority=*/ 0, /*Thickness=*/ 3.f);
		}
	};

	// Ordine di disegno: dal meno al piu' urgente, cosi' l'informazione critica resta leggibile sopra.
	// 1) dove POSSO andare  2) dove VADO  3) chi COLPISCO  4) cosa sto indicando.

	// Celle raggiungibili: contorno piccolo e tenue. Fa vedere il budget mordere (il fango accorcia il raggio)
	// senza coprire il resto: e' contesto, non una decisione presa.
	for (const FRTCellId& Cell : PreviewReachable)
	{
		DrawCellOutline(Cell, FColor(60, 110, 90), 0.52f);
	}

	// Traccia del percorso: contorno ciano su ogni cella + segmento fra i centri consecutivi.
	for (int32 I = 0; I < PreviewPath.Num(); ++I)
	{
		DrawCellOutline(PreviewPath[I], FColor(40, 220, 220), 0.72f);
		if (I > 0)
		{
			const FVector A = URTHexLibrary::AxialToWorld(PreviewPath[I - 1], Origin, Size, LayerH)
				+ FVector(0, 0, CellLift(PreviewPath[I - 1]) + RTLiftPreview + 1.5f);
			const FVector B = URTHexLibrary::AxialToWorld(PreviewPath[I], Origin, Size, LayerH)
				+ FVector(0, 0, CellLift(PreviewPath[I]) + RTLiftPreview + 1.5f);
			DrawDebugLine(World, A, B, FColor(40, 220, 220), false, -1.f, 0, 4.f);
		}
	}

	// Zona colpita dall'attacco pianificato. Rosso = minaccia; ARANCIONE = c'e' un alleato dentro, e va visto
	// PRIMA del lock-in: che il fuoco amico faccia danno e' gia' verificato dai test, che il giocatore lo sappia
	// mentre puo' ancora cambiare idea no — quella meta' esiste solo qui.
	for (const FRTCellId& Cell : PreviewHitCells)
	{
		const bool bAlly = PreviewAllyHitCells.Contains(Cell);
		DrawCellOutline(Cell, bAlly ? FColor(255, 150, 30) : FColor(230, 60, 50), bAlly ? 0.80f : 0.68f);
	}

	// Cella sotto il cursore: disegnata per ultima e piu' larga, cosi' resta leggibile sopra la traccia.
	if (bHoveredValid)
	{
		DrawCellOutline(HoveredCell, FColor::Yellow, 0.88f);
	}
}

const URTHexMapAsset* ARTHexMapActor::GetHexContext(FVector& OutOrigin, float& OutHexSize, float& OutLayerHeight) const
{
	// Dimensioni dall'asset AUTOREVOLE; se manca valgono quelle dell'actor (graybox demo).
	const URTHexMapAsset* Map = MapAsset;
	OutOrigin = GetActorLocation();
	OutHexSize = Map ? Map->HexSize : HexSize;
	OutLayerHeight = Map ? Map->LayerHeight : LayerHeight;
	return Map;
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
	// Lo spessore vive in `RTCellFlatScale` perche' le quote di disegno delle debug-line ci si appoggiano.
	const float PlanarScale = UseHexSize / 50.f * 0.95f;
	const float FlatScale = RTCellFlatScale;

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
	RemoveTransitionData(TransitionFrom, TransitionTo, bTransitionBidirectional);
}

bool ARTHexMapActor::RemoveTransitionData(const FRTCellId& From, const FRTCellId& To, bool bBothDirections)
{
	if (!MapAsset)
	{
		UE_LOG(LogRT, Warning, TEXT("[HexMap] Nessun MapAsset assegnato."));
		return false;
	}
#if WITH_EDITOR
	const FScopedTransaction Transaction(LOCTEXT("HexRemoveTransition", "Hex: Remove Vertical Transition"));
#endif
	MapAsset->Modify();
	const bool bRemoved = MapAsset->RemoveTransition(From, To, bBothDirections);
	if (bRemoved)
	{
		MapAsset->MarkPackageDirty();
		RebuildInstances();
	}
	UE_LOG(LogRT, Log, TEXT("[HexMap] Rimozione transizione %s -> %s: %s."),
		*From.ToString(), *To.ToString(), bRemoved ? TEXT("rimossa") : TEXT("non trovata"));
	return bRemoved;
}

#undef LOCTEXT_NAMESPACE
