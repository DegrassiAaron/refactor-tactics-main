#include "RTHexWorkGridActor.h"

#include "RTHexWorkGrid.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Map/RTCellId.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTMapVisuals.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	/**
	 * 🔴 **Il nome che lega la sezione della mesh al suo slot materiale, ricopiato qui perche' NON e'
	 * esportato.** L'originale e' `RTProceduralMeshSlotName` in `RTHexMapActor.cpp:54`: uno
	 * `static const FName` a scope di file, cioe' internal linkage — nessun header lo dichiara, e il modulo
	 * editor non lo puo' vedere.
	 *
	 * ⚠️ **Il valore e' `"Default"` e deve combaciare col `PolygonGroupMaterialSlotName`** (#1665):
	 * `BuildFromMeshDescriptions` accoppia sezione e slot PER NOME, e se non combaciano la sezione esce con
	 * `MaterialIndex = -1` — geometria completa, nessun materiale, **niente a schermo**.
	 *
	 * 🔑 **Perche' questa copia e' al riparo, e non e' la ragione che verrebbe da dare.** Non e' che
	 * le due estremita' distino poche righe: e' che quel difetto **si manifesta solo nel cotto**.
	 * `RTHexMapMeshBoundsTests.cpp:20-27` lo ha misurato per mutazione — rinominato un solo lato del
	 * legame, in `EditorContext` il test resta **verde** con `MaterialIndex = 0`, perche' l'Editor ha un
	 * fallback che risolve l'indice; nel pacchetto no, e li' esce `-1`. Questa mesh vive in un modulo
	 * **editor-only** e in un pacchetto non entra mai, quindi il modo in cui il legame puo' rompersi non la
	 * raggiunge. ⛔ Se un giorno una mesh procedurale del modulo editor dovesse finire in un binario
	 * cotto, questa costante torna a essere un rischio e va unificata con quella del runtime.
	 */
	const FName RTWorkGridMeshSlotName(TEXT("Default"));
}

const FName ARTHexWorkGridActor::WorkGridTag(TEXT("RTHexWorkGrid"));

ARTHexWorkGridActor::ARTHexWorkGridActor()
{
	// ⛔ Nessun Tick: la griglia si rifa' quando il dato cambia, e chi decide quando e' il mode.
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Ghosts = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Ghosts"));
	Ghosts->SetupAttachment(Root);

	// ⛔ **Nessuna collisione, e la ragione sta nel docstring della classe**: un ISM collidibile fa ripiegare
	// il raycast dei tool in silenzio, e il pennello scrive altrove.
	Ghosts->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ghosts->SetCastShadow(false);

	// Tre float per istanza, come `CellBorders` (`RTHexMapActor.cpp:894`): senza, l'anello renderebbe col
	// materiale di default della mesh e la tinta scritta qui sotto non arriverebbe da nessuna parte.
	Ghosts->NumCustomDataFloats = 3;

	Tags.Add(WorkGridTag);
}

UStaticMesh* ARTHexWorkGridActor::GetWorkGridRingMesh()
{
	// Una sola per processo, con la disciplina di `ARTHexMapActor::GetCellBorderMesh`: `TStrongObjectPtr`
	// tiene la mesh fuori dalla portata del GC senza `AddToRoot` a mano.
	static TStrongObjectPtr<UStaticMesh> Cached;
	if (Cached.IsValid())
	{
		return Cached.Get();
	}

	FMeshDescription Description;
	FStaticMeshAttributes Attributes(Description);
	Attributes.Register();
	TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();

	const FPolygonGroupID Group = Description.CreatePolygonGroup();
	Attributes.GetPolygonGroupMaterialSlotNames()[Group] = RTWorkGridMeshSlotName;

	// ⚠️ I vertici vengono da `HexCorners`, NON da un secondo `cos(60k-30)` scritto qui: e' il vincolo che
	// #712 ha gia' pagato a schermo, dove il bordo era un esagono e il pieno un cerchio. Un segno che non
	// segue la convenzione pointy-top della cella che marca indicherebbe il posto sbagliato.
	const TArray<FVector> OuterCorners = URTHexLibrary::HexCorners(
		FVector::ZeroVector, RTCellPrismRadius * RTHexWorkGrid::OuterScaleInHexSize);
	const TArray<FVector> InnerCorners = URTHexLibrary::HexCorners(
		FVector::ZeroVector, RTCellPrismRadius * (RTHexWorkGrid::OuterScaleInHexSize - RTHexWorkGrid::Thickness));
	if (OuterCorners.Num() != 6 || InnerCorners.Num() != 6)
	{
		return nullptr;
	}

	TArray<FVertexID> OuterIds;
	TArray<FVertexID> InnerIds;
	for (int32 Corner = 0; Corner < 6; ++Corner)
	{
		const FVertexID O = Description.CreateVertex();
		Positions[O] = FVector3f(static_cast<float>(OuterCorners[Corner].X),
			static_cast<float>(OuterCorners[Corner].Y), 0.f);
		OuterIds.Add(O);

		const FVertexID I = Description.CreateVertex();
		Positions[I] = FVector3f(static_cast<float>(InnerCorners[Corner].X),
			static_cast<float>(InnerCorners[Corner].Y), 0.f);
		InnerIds.Add(I);
	}

	// Sei quad fra i due esagoni. Piatta come l'anello di bordo: un segno VOLUMETRICO proietterebbe un
	// fianco che a camera tattica diventa una seconda linea sfalsata.
	for (int32 Edge = 0; Edge < 6; ++Edge)
	{
		const int32 Next = (Edge + 1) % 6;
		TArray<FVertexInstanceID> Instances;
		for (const FVertexID V : { InnerIds[Edge], InnerIds[Next], OuterIds[Next], OuterIds[Edge] })
		{
			Instances.Add(Description.CreateVertexInstance(V));
		}
		Description.CreatePolygon(Group, Instances);
	}

	UStaticMesh* Mesh = NewObject<UStaticMesh>(GetTransientPackage(), TEXT("RT_WorkGridRing"), RF_Transient);

	// Slot inizializzato (#1665): un `FStaticMaterial()` nudo lascia `UVChannelData.bInitialized = false`, e
	// fuori dall'Editor nessuno lo ripara — parte un `ensure` da `GetMaterialStreamingData()`.
	FStaticMaterial RingSlot;
	RingSlot.MaterialSlotName = RTWorkGridMeshSlotName;
	RingSlot.UVChannelData = FMeshUVChannelInfo(1.f);
	Mesh->GetStaticMaterials().Add(RingSlot);

	UStaticMesh::FBuildMeshDescriptionsParams Params;
	Params.bBuildSimpleCollision = false;
	Params.bFastBuild = true;
	Mesh->BuildFromMeshDescriptions({ &Description }, Params);

	Cached.Reset(Mesh);
	return Mesh;
}

void ARTHexWorkGridActor::ClearCells()
{
	if (Ghosts)
	{
		Ghosts->ClearInstances();
	}
}

void ARTHexWorkGridActor::MoveTo(const FVector& Origin)
{
	SetActorLocation(Origin);
}

int32 ARTHexWorkGridActor::NumGhosts() const
{
	return Ghosts ? Ghosts->GetInstanceCount() : 0;
}

void ARTHexWorkGridActor::ShowCells(const TArray<FRTCellId>& Cells, const FVector& Origin,
	float HexSize, float LayerHeight)
{
	// Si riparte sempre da zero invece di aggiornare per differenza: la griglia e' un derivato del dato, e
	// una posa incrementale lascerebbe un fantasma dove una cella e' appena stata dipinta.
	ClearCells();

	if (!Ghosts || Cells.Num() == 0)
	{
		return;
	}

	if (UStaticMesh* Ring = GetWorkGridRingMesh())
	{
		// ⚠️ Prima la mesh e poi il materiale: `SetStaticMesh` ripristina gli slot della mesh, e montarlo
		// prima lo farebbe sparire. E' la stessa sequenza di `RTHexMapActor.cpp:1643-1646`.
		if (Ghosts->GetStaticMesh() != Ring)
		{
			Ghosts->SetStaticMesh(Ring);
		}
	}
	else
	{
		return;
	}

	// ⚠️ **Caricato qui e non nel costruttore**: un `ConstructorHelpers` su un asset di `/Game/` lega il CDO
	// a quel percorso e fallisce rumorosamente dove il contenuto non c'e'. ⛔ Se il materiale manca si posa
	// lo stesso, in grigio: una griglia assente sarebbe peggio di una che non ha ancora la sua tinta.
	if (UMaterialInterface* Tinta = GhostMaterial.LoadSynchronous())
	{
		Ghosts->SetMaterial(0, Tinta);
	}

	// 🔑 **Il portatore si mette SULL'origine della mappa e le istanze si posano in spazio LOCALE.**
	// Cosi' seguire una mappa spostata e' una traslazione dell'actor (`MoveTo`) invece di una ricostruzione
	// del buffer di istanze. Il costo e' una riga; il risparmio e' l'intero buffer, a ogni fotogramma di
	// trascinamento.
	SetActorLocation(Origin);

	// La scala porta la mesh — circumraggio `RTCellPrismRadius` — alle misure della cella. Il fattore di
	// forma (`OuterScaleInHexSize`, `Thickness`) e' gia' dentro la mesh: qui resta solo `HexSize`.
	const float PlanarScale = HexSize / RTCellPrismRadius;

	// ⚠️ `FromSRGBColor` e non una divisione per 255: `GhostColour` e' un `FColor` sRGB a 8 bit e il
	// materiale legge LINEARE. E' la stessa correzione che `RTHexMapActor.cpp:1905-1909` porta scritta —
	// dividere per 255 darebbe una tinta slavata.
	const FLinearColor Ghost = FLinearColor::FromSRGBColor(RTHexWorkGrid::GhostColour());

	// 🔑 **Una `AddInstances` sola invece di N `AddInstance`.** Ogni chiamata singola invalida i bounds,
	// fa crescere `PerInstanceSMData` e `PerInstanceSMCustomData` senza riserva, rivaluta
	// `IsNavigationRelevant()` e fa un broadcast di `OnInstanceIndexUpdated`. Con un tetto di 4096 fantasmi
	// e una ricostruzione a ogni pennellata, e' quel costo moltiplicato per quattromila. La versione a lotto
	// riserva una volta, alloca i custom data una volta e fa un broadcast solo.
	TArray<FTransform> Pose;
	Pose.Reserve(Cells.Num());
	for (const FRTCellId& Cell : Cells)
	{
		// ⚠️ Origine ZERO e non `Origin`: la posizione della mappa la porta la trasformata
		// dell'actor, non ogni singola istanza. E' cio' che rende `MoveTo` sufficiente.
		FVector Locale = URTHexLibrary::AxialToWorld(Cell, FVector::ZeroVector, HexSize, LayerHeight);
		Locale.Z += RTHexWorkGrid::LiftZ;
		Pose.Emplace(FRotator::ZeroRotator, Locale, FVector(PlanarScale, PlanarScale, 1.f));
	}

	const TArray<int32> Indici = Ghosts->AddInstances(Pose, /*bShouldReturnIndices=*/ true, /*bWorldSpace=*/ false);

	for (int32 I = 0; I < Indici.Num(); ++I)
	{
		// ⚠️ **Dopo `AddInstances`, che restituisce gli indici**: `NumCustomDataFloats` alloca i float alla
		// creazione dell'istanza, e scritto prima `SetCustomDataValue` uscirebbe senza dire niente.
		Ghosts->SetCustomDataValue(Indici[I], 0, Ghost.R);
		Ghosts->SetCustomDataValue(Indici[I], 1, Ghost.G);
		// `bMarkRenderStateDirty` una volta sola, sull'ultima istanza: farlo a ogni canale ricostruirebbe
		// il buffer 3N volte (`RTHexMapActor.cpp:1914-1915`).
		Ghosts->SetCustomDataValue(Indici[I], 2, Ghost.B, /*bMarkRenderStateDirty=*/ I == Indici.Num() - 1);
	}
}
