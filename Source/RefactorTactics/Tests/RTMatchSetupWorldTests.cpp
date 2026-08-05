#include "Misc/AutomationTest.h"
#include "RTGameMode.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"

namespace
{
	/** Come in RTBotPlanningTests: World di gioco minimale, senza tick ne' rendering. */
	UWorld* MakeSetupWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroySetupWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Mappa esagonale di prova: esagono pieno di raggio Radius sul layer 0. */
	URTHexMapAsset* MakeHexMap(int32 Radius)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();
		return M;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGameModeHexSetupTest,
	"RefactorTactics.MatchSetup.GameModeSpawnsOnHexMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGameModeHexSetupTest::RunTest(const FString&)
{
	UWorld* World = MakeSetupWorld();
	if (!TestNotNull(TEXT("world di prova"), World))
	{
		return false;
	}

	URTHexMapAsset* Map = MakeHexMap(/*Radius=*/ 2);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Map;

	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	GameMode->SetupHexMatch(MapActor);

	int32 NumUnits = 0;
	int32 Team0 = 0;
	TSet<FRTCellId> Occupied;
	for (TActorIterator<ARTUnit> It(World); It; ++It)
	{
		++NumUnits;
		Team0 += (It->TeamId == 0) ? 1 : 0;
		Occupied.Add(It->Cell);
		TestTrue(TEXT("l'unita' sta su una cella della mappa"), Map->ContainsCell(It->Cell));
	}

	TestEqual(TEXT("board 2v2"), NumUnits, 4);
	TestEqual(TEXT("due unita' per squadra"), Team0, 2);
	TestEqual(TEXT("nessuna sovrapposizione di partenza"), Occupied.Num(), 4);

	DestroySetupWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGameModeSkipsSetupWithUnitsTest,
	"RefactorTactics.MatchSetup.GameModeSkipsWhenLevelHasUnits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGameModeSkipsSetupWithUnitsTest::RunTest(const FString&)
{
	UWorld* World = MakeSetupWorld();
	if (!TestNotNull(TEXT("world di prova"), World))
	{
		return false;
	}

	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = MakeHexMap(/*Radius=*/ 2);

	// Un'unita' gia' posata a mano nel livello: l'allestimento automatico non deve aggiungerne altre.
	World->SpawnActor<ARTUnit>();

	World->SpawnActor<ARTGameMode>()->SetupHexMatch(MapActor);

	int32 NumUnits = 0;
	for (TActorIterator<ARTUnit> It(World); It; ++It)
	{
		++NumUnits;
	}
	TestEqual(TEXT("nessun allestimento sopra le unita' esistenti"), NumUnits, 1);

	DestroySetupWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGameModeNoSetupOnTinyMapTest,
	"RefactorTactics.MatchSetup.GameModeSkipsWhenMapTooSmall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGameModeNoSetupOnTinyMapTest::RunTest(const FString&)
{
	UWorld* World = MakeSetupWorld();
	if (!TestNotNull(TEXT("world di prova"), World))
	{
		return false;
	}

	// Tre celle per quattro unita': non si allestisce una partita a meta'.
	URTHexMapAsset* Tiny = NewObject<URTHexMapAsset>();
	Tiny->AddOrUpdateCell(FRTHexCellData(FRTCellId(0, 0, 0)));
	Tiny->AddOrUpdateCell(FRTHexCellData(FRTCellId(1, 0, 0)));
	Tiny->AddOrUpdateCell(FRTHexCellData(FRTCellId(2, 0, 0)));
	Tiny->SortCells();

	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Tiny;

	World->SpawnActor<ARTGameMode>()->SetupHexMatch(MapActor);

	int32 NumUnits = 0;
	for (TActorIterator<ARTUnit> It(World); It; ++It)
	{
		++NumUnits;
	}
	TestEqual(TEXT("mappa troppo piccola -> nessuna unita'"), NumUnits, 0);

	DestroySetupWorld(World);
	return true;
}
