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

/**
 * Un asset mappa ASSEGNATO ma VUOTO deve comportarsi come "nessuna mappa d'autore": si gioca sull'arena di
 * ripiego. Difetto reale osservato in PIE su L_DevSandbox — il cui DA_HexMap_Sandbox si e' ritrovato a 0 celle:
 * il ripiego guardava solo `!MapAsset` (puntatore nullo), quindi non scattava e la partita non si allestiva.
 * Sintomo per chi gioca: viewport nera, nessuna unita', pareggio immediato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGameModeFallbackOnEmptyAssetTest,
	"RefactorTactics.MatchSetup.GameModeFallsBackOnEmptyMapAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGameModeFallbackOnEmptyAssetTest::RunTest(const FString&)
{
	UWorld* World = MakeSetupWorld();
	if (!TestNotNull(TEXT("world di prova"), World))
	{
		return false;
	}

	// Asset presente ma senza celle: il puntatore non e' nullo, quindi il vecchio controllo lo lasciava passare.
	URTHexMapAsset* Empty = NewObject<URTHexMapAsset>();
	TestEqual(TEXT("premessa: l'asset e' vuoto"), Empty->NumCells(), 0);

	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Empty;

	World->SpawnActor<ARTGameMode>()->SetupHexMatch(MapActor);

	int32 NumUnits = 0;
	for (TActorIterator<ARTUnit> It(World); It; ++It)
	{
		++NumUnits;
		TestTrue(TEXT("l'unita' sta su una cella della mappa in uso"),
			MapActor->MapAsset && MapActor->MapAsset->ContainsCell(It->Cell));
	}
	TestEqual(TEXT("board 2v2 sull'arena di ripiego"), NumUnits, 4);
	TestTrue(TEXT("la mappa in uso ha celle"),
		MapActor->MapAsset && MapActor->MapAsset->NumCells() > 0);

	DestroySetupWorld(World);
	return true;
}

/** L'opt-out resta: con DemoArenaRadius = 0 non si inventa nessuna arena. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGameModeNoFallbackWhenDisabledTest,
	"RefactorTactics.MatchSetup.GameModeNoFallbackWhenRadiusZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGameModeNoFallbackWhenDisabledTest::RunTest(const FString&)
{
	UWorld* World = MakeSetupWorld();
	if (!TestNotNull(TEXT("world di prova"), World))
	{
		return false;
	}

	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = NewObject<URTHexMapAsset>(); // vuoto

	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	GameMode->DemoArenaRadius = 0; // ripiego disattivato
	GameMode->SetupHexMatch(MapActor);

	int32 NumUnits = 0;
	for (TActorIterator<ARTUnit> It(World); It; ++It)
	{
		++NumUnits;
	}
	TestEqual(TEXT("ripiego disattivato -> nessuna unita'"), NumUnits, 0);

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

/**
 * La sorgente della mappa e' una SCELTA, non una catena di flag: `MapSource` decide da dove arriva l'arena su
 * cui si gioca. Le voci generate valgono anche se il livello porta una mappa d'autore — sceglierle
 * esplicitamente significa volerle, e il log lo dichiara — mentre il default resta la mappa del livello.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGameModeMapSourceTestArenaTest,
	"RefactorTactics.MatchSetup.MapSourceTestArenaWinsOverLevelAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGameModeMapSourceTestArenaTest::RunTest(const FString&)
{
	UWorld* World = MakeSetupWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	// Il livello porta una mappa d'autore VALIDA: la scelta esplicita deve comunque prevalere.
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	URTHexMapAsset* Authored = MakeHexMap(/*Radius=*/ 2);
	MapActor->MapAsset = Authored;

	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	GameMode->MapSource = ERTMapSource::GeneratedTestArena;
	GameMode->SetupHexMatch(MapActor);

	TestTrue(TEXT("la mappa d'autore e' stata sostituita"), MapActor->MapAsset != Authored);
	if (!TestNotNull(TEXT("mappa in uso presente"), MapActor->MapAsset.Get())) { DestroySetupWorld(World); return false; }

	// Le caratteristiche della mappa di prova: ostacoli e piattaforma su un secondo layer.
	int32 BlocksMove = 0;
	int32 Layer1 = 0;
	for (const FRTHexCellData& Cell : MapActor->MapAsset->Cells)
	{
		BlocksMove += Cell.bBlocksMovement ? 1 : 0;
		Layer1     += (Cell.Id.Layer == 1) ? 1 : 0;
	}
	TestTrue(TEXT("la mappa in uso ha ostacoli"), BlocksMove >= 3);
	TestTrue(TEXT("la mappa in uso ha un secondo layer"), Layer1 >= 4);

	int32 NumUnits = 0;
	for (TActorIterator<ARTUnit> It(World); It; ++It) { ++NumUnits; }
	TestEqual(TEXT("la partita 2v2 si allestisce sulla mappa di prova"), NumUnits, 4);

	DestroySetupWorld(World);
	return true;
}

/** Arena di ripiego scelta esplicitamente: pavimento liscio del raggio indicato, nessun ostacolo. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGameModeMapSourceDemoArenaTest,
	"RefactorTactics.MatchSetup.MapSourceDemoArenaIsPlainFloor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGameModeMapSourceDemoArenaTest::RunTest(const FString&)
{
	UWorld* World = MakeSetupWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	GameMode->MapSource = ERTMapSource::GeneratedDemoArena;
	GameMode->DemoArenaRadius = 3;
	GameMode->SetupHexMatch(MapActor);

	if (!TestNotNull(TEXT("arena generata"), MapActor->MapAsset.Get())) { DestroySetupWorld(World); return false; }
	int32 BlocksMove = 0;
	for (const FRTHexCellData& Cell : MapActor->MapAsset->Cells) { BlocksMove += Cell.bBlocksMovement ? 1 : 0; }
	TestEqual(TEXT("il ripiego e' pavimento liscio"), BlocksMove, 0);
	TestEqual(TEXT("raggio 3 -> 37 celle"), MapActor->MapAsset->NumCells(), 37);

	DestroySetupWorld(World);
	return true;
}

/** Default: la mappa del livello si usa COM'E', senza sostituzioni silenziose. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGameModeMapSourceLevelAssetTest,
	"RefactorTactics.MatchSetup.MapSourceLevelAssetKeepsAuthoredMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGameModeMapSourceLevelAssetTest::RunTest(const FString&)
{
	UWorld* World = MakeSetupWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	URTHexMapAsset* Authored = MakeHexMap(/*Radius=*/ 2);
	MapActor->MapAsset = Authored;

	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	TestTrue(TEXT("il default e' la mappa del livello"), GameMode->MapSource == ERTMapSource::LevelAsset);
	GameMode->SetupHexMatch(MapActor);

	// CP 8.4: la partita ora lavora su una COPIA della mappa d'autore, perche' il terreno dinamico la modifica
	// (fuoco, acqua) e l'asset su disco non deve cambiare. Il test passa quindi dall'identita' del puntatore al
	// CONTENUTO: quello che il checkpoint garantiva — «la mappa d'autore non viene rimpiazzata dall'arena
	// demo» — resta verificato, e in piu' si verifica che la copia sia fedele.
	TestNotNull(TEXT("la mappa e' allestita"), MapActor->MapAsset.Get());
	TestTrue(TEXT("la mappa d'autore NON viene sostituita da un'altra arena"),
		MapActor->MapAsset && MapActor->MapAsset->ComputeHash() == Authored->ComputeHash());
	TestTrue(TEXT("ma e' una copia di lavoro: l'asset d'autore non viene modificato dalla partita"),
		MapActor->MapAsset != Authored);

	DestroySetupWorld(World);
	return true;
}
