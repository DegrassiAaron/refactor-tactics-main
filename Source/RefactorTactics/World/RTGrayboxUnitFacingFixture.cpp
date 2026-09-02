#include "World/RTGrayboxUnitFacingFixture.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Map/RTHexLibrary.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/**
	 * Le misure delle primitive engine a scala `1`, dichiarate invece che sparse nelle divisioni.
	 *
	 * ⚠️ `50` per il raggio del cilindro non e' un numero mio: `RTScenarioPreviewActor.cpp` lo dichiara
	 * gia' come `UnitCylinderRadius`, ed e' lo stesso da cui discende il raggio `60` di un'unita'
	 * (`50 * BaseMeshScale.X`). Riscriverlo qui a memoria sarebbe la terza copia dello stesso numero.
	 */
	constexpr float RTGrayboxCylinderRadius = 50.f;  // /Engine/BasicShapes/Cylinder, raggio a scala 1
	constexpr float RTGrayboxPrimitiveSize  = 100.f; // lato/altezza delle primitive engine a scala 1

	/** Spessore del marker come frazione della primitiva: `0.16` e' il cuneo di `RTScenarioPreviewActor`. */
	constexpr float RTGrayboxMarkerThickness = 0.16f;

	/** Il disco a terra: sottile, e largo quanto il corpo — serve a dire DOVE poggia, non a decorare. */
	constexpr float RTGrayboxAnchorThickness = 0.02f;
}

ARTGrayboxUnitFacingFixture::ARTGrayboxUnitFacingFixture()
{
	PrimaryActorTick.bCanEverTick = false;

	// ⛔ Root NEUTRO: uno `USceneComponent` nudo, mai una mesh. Se il root fosse una delle mesh, la sua
	// scala diventerebbe la scala dell'attore e deformerebbe in silenzio tutto il resto — #593.
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));


	UnitBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UnitBody"));
	UnitBody->SetupAttachment(SceneRoot);
	UnitBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (CylinderMesh.Succeeded()) { UnitBody->SetStaticMesh(CylinderMesh.Object); }

	FacingMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FacingMarker"));
	FacingMarker->SetupAttachment(SceneRoot);
	FacingMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (CubeMesh.Succeeded()) { FacingMarker->SetStaticMesh(CubeMesh.Object); }

	GroundAnchor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundAnchor"));
	GroundAnchor->SetupAttachment(SceneRoot);
	GroundAnchor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (CylinderMesh.Succeeded()) { GroundAnchor->SetStaticMesh(CylinderMesh.Object); }

	OptionalLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("OptionalLabel"));
	OptionalLabel->SetupAttachment(SceneRoot);
	OptionalLabel->SetHorizontalAlignment(EHTA_Center);
	OptionalLabel->SetWorldSize(28.f);

	// Solo i PATH: nessun caricamento qui, quindi nessuna dipendenza dall'ordine con cui il commandlet
	// crea gli asset. Si risolvono in `OnConstruction`, quando esistono.
	BodyMaterial   = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/RT/World/Graybox/Materials/MI_Graybox_Fixture_Body.MI_Graybox_Fixture_Body")));
	MarkerMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/RT/World/Graybox/Materials/MI_Graybox_Fixture_Marker.MI_Graybox_Fixture_Marker")));
	AnchorMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/RT/World/Graybox/Materials/MI_Graybox_Fixture_Anchor.MI_Graybox_Fixture_Anchor")));
}

FTransform ARTGrayboxUnitFacingFixture::MarkerTransform(ERTHexDirection Facing, float BodyRadius,
	float FaceHeight, float MarkerLength)
{
	// 🔑 **Nessuna trigonometria qui.** Origine e orientamento vengono dalla libreria: e' cio' che rende
	// questo fixture un CONSUMATORE della convenzione dei sei lati invece di una seconda copia. Il
	// guardiano e' `FixtureMarkerComesFromTheLibrary`, che spawna l'attore e confronta col valore vero.
	const FVector  Origin   = URTHexLibrary::FacingMarkerOrigin(Facing, FVector::ZeroVector, BodyRadius, FaceHeight);
	const FRotator Rotation = URTHexLibrary::FacingRotation(Facing);

	// ⚠️ Il cubo engine e' CENTRATO: posarlo sull'origine ne seppellirebbe meta' dentro il corpo. Il
	// contratto dice dove il marker COMINCIA, quindi il centro della mesh sta mezza lunghezza piu' avanti.
	const FVector Centre = Origin + Rotation.Vector() * (static_cast<double>(MarkerLength) * 0.5);

	const FVector Scale(MarkerLength / RTGrayboxPrimitiveSize, RTGrayboxMarkerThickness, RTGrayboxMarkerThickness);
	return FTransform(Rotation, Centre, Scale);
}

FTransform ARTGrayboxUnitFacingFixture::BodyTransform(float BodyRadius, float BodyHeight)
{
	// Il cilindro engine e' centrato sulla propria altezza: per farlo POGGIARE sul piano del root il centro
	// sale di meta'. Senza, il corpo starebbe mezzo sottoterra e il facing sembrerebbe partire dal suolo.
	const FVector Scale(BodyRadius / RTGrayboxCylinderRadius,
	                    BodyRadius / RTGrayboxCylinderRadius,
	                    BodyHeight / RTGrayboxPrimitiveSize);
	return FTransform(FRotator::ZeroRotator, FVector(0.0, 0.0, static_cast<double>(BodyHeight) * 0.5), Scale);
}

void ARTGrayboxUnitFacingFixture::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// ⚠️ Questa funzione non DECIDE niente: applica. Ogni riga di geometria sta nelle due statiche sopra,
	// che un test puo' interrogare senza mondo — ed e' per questo che il legame fra l'asset e la libreria
	// e' verificabile, cosa che con un Blueprint puro non era.
	// ⚠️ I materiali si applicano QUI e non nel costruttore: alla costruzione del CDO gli asset possono
	// non esistere ancora. Se un path non risolve il componente resta grigio invece di rompersi — un
	// checkout che non ha ancora eseguito il commandlet deve poter aprire la mappa.
	auto Dress = [](UStaticMeshComponent* Component, const TSoftObjectPtr<UMaterialInterface>& Material)
	{
		if (Component)
		{
			if (UMaterialInterface* Resolved = Material.LoadSynchronous())
			{
				Component->SetMaterial(0, Resolved);
			}
		}
	};
	Dress(UnitBody, BodyMaterial);
	Dress(FacingMarker, MarkerMaterial);
	Dress(GroundAnchor, AnchorMaterial);

	if (UnitBody)
	{
		UnitBody->SetRelativeTransform(BodyTransform(BodyRadius, BodyHeight));
	}
	if (FacingMarker)
	{
		FacingMarker->SetRelativeTransform(MarkerTransform(Facing, BodyRadius, FaceHeight, MarkerLength));
	}
	if (GroundAnchor)
	{
		GroundAnchor->SetRelativeTransform(FTransform(FRotator::ZeroRotator, FVector::ZeroVector,
			FVector(BodyRadius / RTGrayboxCylinderRadius, BodyRadius / RTGrayboxCylinderRadius, RTGrayboxAnchorThickness)));
	}
	if (OptionalLabel)
	{
		OptionalLabel->SetVisibility(bShowLabel);
		// La label sta SOPRA il corpo: se stesse davanti competerebbe col marker, che e' il canale primario.
		OptionalLabel->SetRelativeLocation(FVector(0.0, 0.0, static_cast<double>(BodyHeight) + 20.0));
		OptionalLabel->SetText(FText::FromString(
			StaticEnum<ERTHexDirection>()->GetNameStringByValue(static_cast<int64>(Facing))));
	}
}
