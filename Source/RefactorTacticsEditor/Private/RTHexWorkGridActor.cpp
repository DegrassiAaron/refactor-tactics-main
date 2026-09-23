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
	 * `MaterialIndex = -1` — geometria completa, nessun materiale, **niente a schermo**. Misurato nel
	 * pacchetto il 2026-08-30. Qui le due estremita' del legame distano sei righe, che e' la sola difesa
	 * possibile finche' la costante resta privata di quel `.cpp`.
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

	// La scala porta la mesh — circumraggio `RTCellPrismRadius` — alle misure della cella. Il fattore di
	// forma (`OuterScaleInHexSize`, `Thickness`) e' gia' dentro la mesh: qui resta solo `HexSize`.
	const float PlanarScale = HexSize / RTCellPrismRadius;

	// ⚠️ `FromSRGBColor` e non una divisione per 255: `GhostColour` e' un `FColor` sRGB a 8 bit e il
	// materiale legge LINEARE. E' la stessa correzione che `RTHexMapActor.cpp:1905-1909` porta scritta —
	// dividere per 255 darebbe una tinta slavata.
	const FLinearColor Ghost = FLinearColor::FromSRGBColor(RTHexWorkGrid::GhostColour());

	for (const FRTCellId& Cell : Cells)
	{
		FVector World = URTHexLibrary::AxialToWorld(Cell, Origin, HexSize, LayerHeight);
		World.Z += RTHexWorkGrid::LiftZ;

		const int32 Index = Ghosts->AddInstance(
			FTransform(FRotator::ZeroRotator, World, FVector(PlanarScale, PlanarScale, 1.f)),
			/*bWorldSpace=*/ true);

		// ⚠️ **Dopo `AddInstance`, che restituisce l'indice**: `NumCustomDataFloats` alloca i float alla
		// creazione dell'istanza, e scritto prima `SetCustomDataValue` uscirebbe senza dire niente.
		Ghosts->SetCustomDataValue(Index, 0, Ghost.R);
		Ghosts->SetCustomDataValue(Index, 1, Ghost.G);
		// `bMarkRenderStateDirty` una volta sola, sull'ultima istanza: farlo a ogni canale ricostruirebbe
		// il buffer 3N volte (`RTHexMapActor.cpp:1914-1915`).
		Ghosts->SetCustomDataValue(Index, 2, Ghost.B,
			/*bMarkRenderStateDirty=*/ Index == Cells.Num() - 1);
	}
}
