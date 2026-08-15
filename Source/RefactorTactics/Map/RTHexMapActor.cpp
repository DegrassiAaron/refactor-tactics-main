#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Map/RTArenaCriteriaLibrary.h"
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

	/**
	 * Geometria dei volumi che mostrano le regole, in frazione del raggio della cella.
	 *
	 * Sono ANNIDATI e in ordine: contorno di superficie 0.85 (linea) > lastra della vista 0.75 > rilievo del
	 * costo 0.60 > colonna del blocco 0.40. Una cella che dice tre cose insieme le mostra tutte e tre, invece
	 * di nasconderne due sotto la terza — ed e' il criterio che tiene, non l'estetica.
	 */
	constexpr float RTSightSlabScale = 0.75f;
	constexpr float RTBlockColumnScale = 0.40f;

	/**
	 * Altezze (uu). La lastra della vista e' BASSA di proposito: comunica «ci si passa sopra», che e' la cosa
	 * piu' fraintesa della mappa. La colonna del blocco e' alta abbastanza da leggersi dall'alto, che e' la
	 * vista di lavoro, e resta due ordini di grandezza sotto `LayerHeight` (250) per non confondersi con un
	 * piano — stesso vincolo del rilievo di costo.
	 *
	 * ⚠️ **La lastra deve restare sotto `URTHexLibrary::ReliefUnitHeight` (15 uu)**, e non e' una
	 * preferenza estetica. Lastra e rilievo sono CONCENTRICI e partono dalla stessa quota, e la lastra e'
	 * la piu' larga: se la supera anche in altezza, la inghiotte. Il costo massimo del catalogo v0.1 e'
	 * `2`, quindi il rilievo piu' alto che una mappa possa produrre e' esattamente `ReliefUnitHeight` —
	 * il che rende il caso peggiore l'unico caso, e ogni cella costosa che blocca la vista smetterebbe di
	 * dire quanto costa. A 16 uu succedeva; `HexMapActor.CostReliefSurvivesTheSightSlab` lo misura.
	 */
	constexpr float RTSightSlabHeight = 10.f;
	constexpr float RTBlockColumnHeight = 55.f;

	/**
	 * Pannelli di bordo (cubo engine: 100 uu per lato, centrato). Lo spessore e' sottile perche' un bordo non
	 * ha profondita': quello che deve comunicare e' DOVE sta e QUANTO e' alto.
	 *
	 * Le altezze seguono la semantica, non l'estetica:
	 * - copertura BASSA ripara e lascia passare tutto -> un muretto che si scavalca con lo sguardo;
	 * - copertura ALTA nega vista, passo e proiettili -> alta quanto la colonna di blocco, perche' fa la
	 *   stessa cosa su un lato invece che su una cella;
	 * - porta CHIUSA nega passo e vista -> piena, ed e' la piu' alta: e' una barriera, non un riparo;
	 * - porta APERTA lascia passare -> una soglia bassa, che dice «qui c'e' una porta» senza dire «e' chiusa».
	 *   Non disegnarla affatto nasconderebbe l'informazione piu' utile: che quel passaggio puo' chiudersi.
	 */
	constexpr float RTEdgePanelThickness = 0.10f;
	constexpr float RTEdgePanelWidth = 0.92f;
	constexpr float RTCoverLowHeight = 22.f;
	constexpr float RTCoverHighHeight = 55.f;
	constexpr float RTDoorClosedHeight = 70.f;
	constexpr float RTDoorOpenHeight = 5.f;

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

	// Rilievo del costo: stessa mesh, ma SENZA collisione. E' la prima delle due difese — la seconda e'
	// `IsPickOnSelectableCell`, che scarta i colpi su qualunque componente diverso da `Cells`. Serve entrambe:
	// questa evita lavoro inutile al raycast, quella impedisce che una geometria futura rubi i click perche'
	// qualcuno si e' dimenticato di questa riga.
	Relief = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Relief"));
	Relief->SetupAttachment(Cells);
	Relief->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Relief->SetCollisionResponseToAllChannels(ECR_Ignore);
	Relief->CastShadow = false; // e' uno strumento di lettura, non scenografia: le ombre confonderebbero il profilo
	if (CylinderMesh.Succeeded())
	{
		Relief->SetStaticMesh(CylinderMesh.Object);
	}

	// Volumi delle regole di blocco: stessa disciplina del rilievo — nessuna collisione, nessuna ombra.
	Blockers = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Blockers"));
	Blockers->SetupAttachment(Cells);
	Blockers->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Blockers->SetCollisionResponseToAllChannels(ECR_Ignore);
	Blockers->CastShadow = false;
	if (CylinderMesh.Succeeded())
	{
		Blockers->SetStaticMesh(CylinderMesh.Object);
	}

	// Pannelli di bordo: mesh PROPRIA. Un pannello non e' un cilindro schiacciato, e nessuna scala trasforma
	// l'uno nell'altro — e' l'unico caso in cui un secondo componente aggiunge qualcosa.
	EdgeFeatures = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("EdgeFeatures"));
	EdgeFeatures->SetupAttachment(Cells);
	EdgeFeatures->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EdgeFeatures->SetCollisionResponseToAllChannels(ECR_Ignore);
	EdgeFeatures->CastShadow = false;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		EdgeFeatures->SetStaticMesh(CubeMesh.Object);
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

bool ARTHexMapActor::IsPickOnSelectableCell(const UPrimitiveComponent* HitComponent, int32 InstanceIndex) const
{
	// L'indice di istanza appartiene al componente COLPITO: va quindi validato contro quel componente, non
	// contro l'actor che lo contiene. Le due condizioni sono una regola sola.
	return HitComponent != nullptr
		&& HitComponent == Cells.Get()
		&& InstanceCells.IsValidIndex(InstanceIndex);
}

FRTCellId ARTHexMapActor::CellForInstance(int32 InstanceIndex) const
{
	return InstanceCells.IsValidIndex(InstanceIndex) ? InstanceCells[InstanceIndex] : FRTCellId();
}

const TArray<FRTCellId>& ARTHexMapActor::GetUnreachableCells() const
{
	if (!bUnreachableDirty)
	{
		return UnreachableCells;
	}
	bUnreachableDirty = false;
	UnreachableCells.Reset();

	// Gli spawn si chiedono alla stessa funzione che allestisce la partita: misurare la raggiungibilita' da
	// una cella qualunque risponderebbe di una partita che non si gioca.
	if (MapAsset && MapAsset->NumCells() > 0)
	{
		const TArray<FRTCellId> Start = URTMatchSetupLibrary::PickStartCells(MapAsset, /*NumPerTeam=*/ 2, /*Layer=*/ 0);
		if (Start.Num() > 0)
		{
			UnreachableCells = URTArenaCriteriaLibrary::FindUnreachableCells(MapAsset, Start[0]);
		}
	}
	return UnreachableCells;
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
	// `bThroughUnits`: disegna il contorno in FOREGROUND, cioe' senza test di profondita'.
	//
	// Serve alle celle che possono essere OCCUPATE. Il lift di 2.5 unita' mette l'anteprima sopra il terreno,
	// non sopra un'unita': da camera dall'alto il cilindro di chi sta sulla cella copre il contorno per intero.
	// L'arancione del fuoco amico ne era la vittima sistematica — esiste solo dove c'e' un alleato, quindi
	// finiva SEMPRE sotto un cilindro, e l'avviso che deve arrivare prima del lock-in non arrivava mai.
	// Osservato in PIE il 2026-08-08 (`PIE-PREVIEW-AREA`).
	//
	// Non si applica a tutto: celle raggiungibili e traccia del percorso stanno per definizione su celle
	// VUOTE — niente le copre, e metterle in foreground le farebbe vedere attraverso il terreno senza che
	// nessuno l'abbia chiesto.
	const auto DrawCellOutline = [World, &Origin, Size, LayerH, &CellLift](const FRTCellId& Cell, const FColor& Color, float Scale, bool bThroughUnits = false)
	{
		const FVector Center = URTHexLibrary::AxialToWorld(Cell, Origin, Size, LayerH)
			+ FVector(0, 0, CellLift(Cell) + RTLiftPreview);
		const TArray<FVector> Corners = URTHexLibrary::HexCorners(Center, Size * Scale);
		const uint8 Depth = bThroughUnits ? SDPG_Foreground : SDPG_World;
		for (int32 I = 0; I < Corners.Num(); ++I)
		{
			DrawDebugLine(World, Corners[I], Corners[(I + 1) % Corners.Num()], Color,
				/*bPersistentLines=*/ false, /*LifeTime=*/ -1.f, Depth, /*Thickness=*/ 3.f);
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
		DrawCellOutline(Cell, bAlly ? FColor(255, 150, 30) : FColor(230, 60, 50), bAlly ? 0.80f : 0.68f,
			/*bThroughUnits=*/ true);
	}

	// Cella sotto il cursore: disegnata per ultima e piu' larga, cosi' resta leggibile sopra la traccia.
	// Anche questa attraversa le unita': si punta un bersaglio molto piu' spesso di una cella vuota, e
	// l'evidenziazione che sparisce proprio quando indichi qualcuno e' peggio che non averla.
	if (bHoveredValid)
	{
		DrawCellOutline(HoveredCell, FColor::Yellow, 0.88f, /*bThroughUnits=*/ true);
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

	// Celle isolate: si INVALIDA soltanto, il calcolo lo fa `GetUnreachableCells` alla prima richiesta.
	// RebuildInstances viene chiamata a ogni OnClickDrag del pennello — molte volte al secondo mentre si
	// trascina — e una BFS sull'intero grafo a ogni cella dipinta sarebbe lavoro sprecato per un dato che
	// serve solo a chi guarda l'overlay, e solo se e' acceso.
	bUnreachableDirty = true;
	if (Relief)
	{
		if (UStaticMesh* Mesh = CellMesh.LoadSynchronous()) { Relief->SetStaticMesh(Mesh); }
		Relief->ClearInstances();
	}
	if (Blockers)
	{
		if (UStaticMesh* Mesh = CellMesh.LoadSynchronous()) { Blockers->SetStaticMesh(Mesh); }
		Blockers->ClearInstances();
	}
	if (EdgeFeatures)
	{
		// La mesh dei bordi NON segue `CellMesh`: quella e' la mesh della cella, e un pannello non e' una cella.
		EdgeFeatures->ClearInstances();
	}

	// Sorgente celle: l'asset se popolato, altrimenti un graybox demo (esagono pieno di raggio DemoRadius).
	const float UseHexSize = MapAsset ? MapAsset->HexSize : HexSize;
	const float UseLayerH = MapAsset ? MapAsset->LayerHeight : LayerHeight;

	// Filtro layer (H4): solo AllLayers impila i piani; ActiveOnly e Focus tengono le ISTANZE sul solo layer
	// attivo. La differenza fra i due e' di sola presentazione — Focus disegna gli altri piani a contorno
	// (`RTHexEditor::DrawSurfaceOverlay`) — e sta fuori di qui apposta: i piani di contesto non devono
	// diventare istanze, o tornerebbero ad avere collisione e a intercettare il click del pennello.
	auto PassesLayerFilter = [this](int32 Layer)
	{
		return LayerView == ERTLayerViewMode::AllLayers || Layer == ActiveLayer;
	};

	TArray<FRTCellId> CellIds;
	TArray<int32> Heights;
	TArray<int32> MoveCosts;
	TArray<bool> BlocksMove;
	TArray<bool> BlocksSight;
	// Puntatori alle celle d'asset per leggerne coperture e porte: sparsi, quindi la stragrande maggioranza
	// delle celle non produce nulla. Nel ramo demo restano nulli — un graybox non ha bordi d'autore.
	TArray<const FRTHexCellData*> EdgeSources;
	if (MapAsset && MapAsset->NumCells() > 0)
	{
		CellIds.Reserve(MapAsset->NumCells());
		Heights.Reserve(MapAsset->NumCells());
		MoveCosts.Reserve(MapAsset->NumCells());
		BlocksMove.Reserve(MapAsset->NumCells());
		BlocksSight.Reserve(MapAsset->NumCells());
		for (const FRTHexCellData& C : MapAsset->Cells)
		{
			if (!PassesLayerFilter(C.Id.Layer))
			{
				continue;
			}
			CellIds.Add(C.Id);
			Heights.Add(C.Height);
			MoveCosts.Add(C.TotalMoveCost()); // il rilievo mostra il costo VERO: una cella stretta si alza
			BlocksMove.Add(C.bBlocksMovement);
			BlocksSight.Add(C.bBlocksLineOfSight);
			EdgeSources.Add(&C);
		}
	}
	else if (DemoRadius > 0)
	{
		// Demo graybox sul layer attivo (visibile sia in AllLayers sia in ActiveOnly).
		CellIds = URTHexLibrary::HexArea(FRTCellId(0, 0, ActiveLayer), DemoRadius);
		Heights.Init(0, CellIds.Num());
		MoveCosts.Init(1, CellIds.Num()); // il graybox non ha terreni: tutto pavimento, quindi piatto
		BlocksMove.Init(false, CellIds.Num());
		BlocksSight.Init(false, CellIds.Num());
		EdgeSources.Init(nullptr, CellIds.Num());
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

		// Rilievo del costo: un blocco alto quanto il SOVRAPPREZZO della cella. Il pavimento non ne produce
		// nessuno — una mappa senza terreni costosi resta piatta, ed e' giusto: non c'e' niente da segnalare.
		const float ReliefHeight = URTHexLibrary::ReliefHeightForCost(MoveCosts[I]);
		if (Relief && ReliefHeight > 0.f)
		{
			// Poggia sulla faccia del disco e cresce verso l'alto; il cilindro engine e' CENTRATO, quindi il
			// suo centro sta a meta' altezza. Piu' stretto della cella (0.6) per non coprire il contorno
			// colorato della superficie, che resta il canale del *tipo* di terreno.
			FVector ReliefCenter = World;
			ReliefCenter.Z += RTCellTopZ + ReliefHeight * 0.5;
			const FTransform ReliefXf(FRotator::ZeroRotator, ReliefCenter,
				FVector(PlanarScale * 0.6f, PlanarScale * 0.6f, ReliefHeight / 100.f));
			Relief->AddInstance(ReliefXf, /*bWorldSpace=*/ true);
		}

		// Volumi delle due regole. Sono INDIPENDENTI: una cella puo' averne una, l'altra o entrambe, e in
		// quest'ultimo caso si vedono tutte e due — la lastra larga e bassa attorno alla colonna stretta e
		// alta. Confonderle e' l'errore piu' frequente su questa mappa: una cella che blocca la vista si
		// ATTRAVERSA, ed e' cio' che serve a una rotta coperta ma percorribile.
		if (Blockers)
		{
			auto AddVolume = [&](float PlanarFraction, float VolumeHeight)
			{
				FVector Center = World;
				Center.Z += RTCellTopZ + VolumeHeight * 0.5;
				const FTransform VolumeXf(FRotator::ZeroRotator, Center,
					FVector(PlanarScale * PlanarFraction, PlanarScale * PlanarFraction, VolumeHeight / 100.f));
				Blockers->AddInstance(VolumeXf, /*bWorldSpace=*/ true);
			};

			if (BlocksSight[I]) { AddVolume(RTSightSlabScale, RTSightSlabHeight); }
			if (BlocksMove[I])  { AddVolume(RTBlockColumnScale, RTBlockColumnHeight); }
		}

		// Pannelli di BORDO: coperture e porte. Il punto e l'orientamento si CHIEDONO alla libreria
		// (`EdgeMidpointWorld`, `EdgeRotation`), che li deriva dai due centri di cella: se la convenzione dei
		// sei lati cambiasse, la geometria seguirebbe invece di mentire.
		if (EdgeFeatures && EdgeSources[I])
		{
			const FRTHexCellData& Data = *EdgeSources[I];
			auto AddEdgePanel = [&](ERTHexDirection Edge, float PanelHeight)
			{
				FVector Center = URTHexLibrary::EdgeMidpointWorld(CellIds[I], Edge, GetActorLocation(),
					UseHexSize, UseLayerH);
				Center.Z += static_cast<double>(Heights[I]) + RTCellTopZ + PanelHeight * 0.5;
				// Il cubo engine e' 100 uu per lato: X sottile (spessore), Y lungo il bordo, Z l'altezza.
				const FTransform PanelXf(URTHexLibrary::EdgeRotation(CellIds[I], Edge), Center,
					FVector(RTEdgePanelThickness,
						UseHexSize / 100.f * RTEdgePanelWidth,
						PanelHeight / 100.f));
				EdgeFeatures->AddInstance(PanelXf, /*bWorldSpace=*/ true);
			};

			for (const FRTHexCover& Cover : Data.Covers)
			{
				if (Cover.Type == ERTHexCoverType::None) { continue; }
				AddEdgePanel(Cover.Edge,
					Cover.Type == ERTHexCoverType::High ? RTCoverHighHeight : RTCoverLowHeight);
			}
			for (const FRTHexDoor& Door : Data.Doors)
			{
				// `Destroyed` e' terminale e non si richiude: si mostra come aperta, perche' e' cio' che e'.
				const bool bBlocking = (Door.State == ERTHexDoorState::Closed
					|| Door.State == ERTHexDoorState::Locked);
				AddEdgePanel(Door.Edge, bBlocking ? RTDoorClosedHeight : RTDoorOpenHeight);
			}
		}
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

void ARTHexMapActor::GenerateArenaV01IntoAsset()
{
	// Scorciatoia per la fixture piu' usata: la seduta U1 la nomina, e cambiarle nome costringerebbe a
	// riscrivere una guida per un pulsante che fa gia' la cosa giusta.
	const FString Previous = FixtureId;
	FixtureId = TEXT("ArenaV01");
	GenerateFixtureIntoAsset();
	FixtureId = Previous;
}

void ARTHexMapActor::GenerateFixtureIntoAsset()
{
	if (!MapAsset)
	{
		UE_LOG(LogRT, Warning, TEXT("[HexMap] Nessun MapAsset assegnato: assegnalo prima di generare."));
		return;
	}

	const URTHexMapAsset* Source = URTMatchSetupLibrary::MakeFixtureArena(GetTransientPackage(), FixtureId);
	if (!Source)
	{
		// Nome sconosciuto: non si tocca nulla. Svuotare l'asset per un refuso sarebbe il danno peggiore, e
		// il messaggio dice QUALE nome non esiste invece di lamentarsi in astratto.
		UE_LOG(LogRT, Warning,
			TEXT("[HexMap] Fixture '%s' sconosciuta: asset invariato. Nomi validi: ArenaV01, RelayBasin, ")
			TEXT("RelayLite, TestArena, CoverYard, DemoArena."), *FixtureId);
		return;
	}

#if WITH_EDITOR
	const FScopedTransaction Transaction(LOCTEXT("HexGenerateFixture", "Hex: Generate Fixture"));
#endif
	MapAsset->Modify();
	// Sostituisce, non fonde: mescolare una fixture con quello che c'era darebbe una mappa che non e' ne'
	// l'una ne' l'altra, e i criteri smetterebbero di dire qualcosa su cio' che si ha davanti.
	MapAsset->Cells.Reset();
	MapAsset->Transitions.Reset();
	for (const FRTHexCellData& Cell : Source->Cells)
	{
		MapAsset->AddOrUpdateCell(Cell);
	}
	MapAsset->Transitions = Source->Transitions;
	MapAsset->SortCells();
	MapAsset->MarkPackageDirty();
	RebuildInstances();
	UE_LOG(LogRT, Log, TEXT("[HexMap] Fixture '%s' scritta nell'asset: %d celle, %d transizioni."),
		*FixtureId, MapAsset->NumCells(), MapAsset->Transitions.Num());
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
	// Il reset passa dall'asset, come ogni altra modifica: e' lui a possedere `Revision`. Scrivere sui due
	// array da qui la lasciava ferma, ed era l'unica modifica strutturale a farlo (#902).
	MapAsset->ClearAll();
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
