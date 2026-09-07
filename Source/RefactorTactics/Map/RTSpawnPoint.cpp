#include "Map/RTSpawnPoint.h"

#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTMapTemplateLibrary.h"

#if WITH_EDITORONLY_DATA
#include "Components/BillboardComponent.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"
#endif

#define LOCTEXT_NAMESPACE "RTSpawnPoint"

ARTSpawnPoint::ARTSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	// ⛔ Root NEUTRO, come `ARTGrayboxUnitFacingFixture`: uno `USceneComponent` nudo. Un marker non ha corpo,
	// e attaccare il root a una primitiva darebbe alla sua scala un significato che questo attore non ha.
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

#if WITH_EDITORONLY_DATA
	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (SpriteComponent && !IsRunningCommandlet())
	{
		// L'icona di `APlayerStart`: e' una risorsa dell'ENGINE, quindi non aggiunge un `.uasset` al
		// progetto — e dice gia' al level designer «da qui parte qualcuno», che e' esattamente il fatto.
		struct FConstructorStatics
		{
			ConstructorHelpers::FObjectFinderOptional<UTexture2D> SpawnIcon;
			FConstructorStatics() : SpawnIcon(TEXT("/Engine/EditorResources/S_Player")) {}
		};
		static FConstructorStatics ConstructorStatics;

		SpriteComponent->Sprite = ConstructorStatics.SpawnIcon.Get();
		SpriteComponent->bIsScreenSizeScaled = true;
		SpriteComponent->SetupAttachment(SceneRoot);
	}
#endif
}

bool ARTSpawnPoint::ResolveCell(const ARTHexMapActor* MapActor, FRTCellId& OutCell) const
{
	OutCell = FRTCellId();
	if (!MapActor)
	{
		return false;
	}

	// ⚠️ Origine e scala si CHIEDONO a `GetHexContext`, che l'header di `ARTHexMapActor` dichiara «UNICO
	// punto da cui passano le conversioni cella<->mondo». Rileggere qui `GetActorLocation()` e `HexSize`
	// separatamente sarebbe una seconda strada verso la stessa geometria, e le due divergerebbero il giorno
	// in cui l'asset e l'attore non concordano — che e' precisamente il caso che `GetHexContext` arbitra.
	FVector GridOrigin = FVector::ZeroVector;
	float HexSize = 0.f;
	float LayerHeight = 0.f;
	const URTHexMapAsset* MapAsset = MapActor->GetHexContext(GridOrigin, HexSize, LayerHeight);

	return URTMapTemplateLibrary::ResolveWorldToCell(MapAsset, GridOrigin, HexSize, LayerHeight,
		GetActorLocation(), OutCell);
}

#if WITH_EDITOR
void ARTSpawnPoint::CheckForErrors()
{
	Super::CheckForErrors();

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Le regole stanno in un posto solo: qui si CHIEDE il verdetto e si emette la parte che riguarda questo
	// attore. Riscriverle sarebbe la seconda implementazione della stessa validazione — e quella che i test
	// coprono e' l'altra.
	int32 MapActorCount = 0;
	const URTHexMapAsset* MapAsset = nullptr;
	TArray<FRTSpawnPlacement> Spawns;
	URTMapTemplateLibrary::CollectFromWorld(World, MapActorCount, MapAsset, Spawns);

	TArray<FRTMapTemplateIssue> Issues;
	URTMapTemplateLibrary::ValidateTemplate(MapActorCount, MapAsset, Spawns, Issues);

	const FString MyName = GetName();
	for (const FRTMapTemplateIssue& Issue : Issues)
	{
		// Le segnalazioni di livello — attore mappa assente o duplicato, asset mancante — le emette
		// `ARTHexMapActor`: se le emettesse anche ogni marker, un livello con quattro spawn direbbe quattro
		// volte la stessa cosa.
		if (Issue.Label != MyName)
		{
			continue;
		}

		URTMapTemplateLibrary::EmitIssueToMapCheck(Issue, this);
	}
}
#endif

#undef LOCTEXT_NAMESPACE
