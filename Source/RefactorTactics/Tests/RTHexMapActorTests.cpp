#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Mondo minimo per spawnare un actor (nomi distinti per file: unity build). */
	UWorld* MakeMapActorWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyMapActorWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		if (GEngine)
		{
			GEngine->DestroyWorldContext(World);
		}
		World->DestroyWorld(false);
	}

	URTHexMapAsset* MakeActorTestAsset(int32 Radius, int32 Layer = 0)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, Layer), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();
		return M;
	}

	/** Spawna l'actor con l'asset gia' assegnato, cosi' OnConstruction lo vede (come al caricamento del livello). */
	ARTHexMapActor* SpawnMapActor(UWorld* World, URTHexMapAsset* Asset, int32 ActiveLayer = 0,
		ERTLayerViewMode View = ERTLayerViewMode::AllLayers)
	{
		ARTHexMapActor* Actor = World->SpawnActorDeferred<ARTHexMapActor>(
			ARTHexMapActor::StaticClass(), FTransform::Identity);
		if (!Actor)
		{
			return nullptr;
		}
		Actor->MapAsset = Asset;
		Actor->ActiveLayer = ActiveLayer;
		Actor->LayerView = View;
		Actor->FinishSpawning(FTransform::Identity);
		return Actor;
	}
}

// La vista deve rigenerarsi da sola alla costruzione dell'actor: senza questo, riaprendo il livello la griglia
// non viene ridisegnata e la mappa istanza->cella resta vuota (il click nel viewport non trova piu' le celle).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapActorRebuildOnConstructionTest,
	"RefactorTactics.HexMapActor.RebuildsOnConstruction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapActorRebuildOnConstructionTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();
	TestNotNull(TEXT("World creato"), World);
	if (!World) { return false; }

	URTHexMapAsset* Asset = MakeActorTestAsset(/*Radius*/ 1); // 7 celle
	ARTHexMapActor* Actor = SpawnMapActor(World, Asset);
	TestNotNull(TEXT("actor spawnato"), Actor);

	if (Actor)
	{
		TestEqual(TEXT("le 7 celle dell'asset sono rappresentate senza chiamare RebuildInstances"),
			Actor->NumInstanceCells(), 7);

		// La mappa istanza -> cella e' popolata: e' cio' che serve al raycast di selezione.
		const FRTCellId First = Actor->CellForInstance(0);
		TestTrue(TEXT("la prima istanza corrisponde a una cella dell'asset"), Asset->ContainsCell(First));
	}

	DestroyMapActorWorld(World);
	return true;
}

// Il filtro di layer va applicato dalla ricostruzione automatica, non solo dal pulsante manuale.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapActorLayerFilterTest,
	"RefactorTactics.HexMapActor.LayerFilterOnConstruction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapActorLayerFilterTest::RunTest(const FString&)
{
	UWorld* World = MakeMapActorWorld();
	TestNotNull(TEXT("World creato"), World);
	if (!World) { return false; }

	// 7 celle sul layer 0 + 7 sul layer 1.
	URTHexMapAsset* Asset = MakeActorTestAsset(/*Radius*/ 1, /*Layer*/ 0);
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 1), 1))
	{
		Asset->AddOrUpdateCell(FRTHexCellData(Id));
	}
	Asset->SortCells();

	if (ARTHexMapActor* All = SpawnMapActor(World, Asset, /*ActiveLayer*/ 0, ERTLayerViewMode::AllLayers))
	{
		TestEqual(TEXT("AllLayers rappresenta entrambi i piani"), All->NumInstanceCells(), 14);
	}

	if (ARTHexMapActor* Active = SpawnMapActor(World, Asset, /*ActiveLayer*/ 1, ERTLayerViewMode::ActiveOnly))
	{
		TestEqual(TEXT("ActiveOnly rappresenta solo il layer attivo"), Active->NumInstanceCells(), 7);
		bool bAllOnActiveLayer = Active->NumInstanceCells() > 0;
		for (int32 I = 0; I < Active->NumInstanceCells(); ++I)
		{
			bAllOnActiveLayer &= Active->CellForInstance(I).Layer == 1;
		}
		TestTrue(TEXT("tutte le celle rappresentate stanno sul layer attivo"), bAllOnActiveLayer);
	}

	DestroyMapActorWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
