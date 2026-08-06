#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexVisionLibrary.h"
#include "Pathfinding/RTHexPathLibrary.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"

namespace
{
	/** Mappa di prova: le celle indicate sul layer 0; quelle in Blocked non sono percorribili. */
	URTHexMapAsset* MakeSetupMap(const TArray<FRTCellId>& Cells, const TArray<FRTCellId>& Blocked)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : Cells)
		{
			FRTHexCellData Data(Id);
			Data.bBlocksMovement = Blocked.Contains(Id);
			M->AddOrUpdateCell(Data);
		}
		M->SortCells();
		return M;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchSetupPickStartCellsTest,
	"RefactorTactics.MatchSetup.PickStartCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchSetupPickStartCellsTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeSetupMap({ {0,0}, {1,0}, {2,0}, {3,0}, {4,0} }, {});

	const TArray<FRTCellId> Start = URTMatchSetupLibrary::PickStartCells(Map, 2, 0);
	TestEqual(TEXT("quattro celle per un 2v2"), Start.Num(), 4);
	TestEqual(TEXT("nessuna cella ripetuta"), TSet<FRTCellId>(Start).Num(), 4);

	// Le due squadre partono dagli estremi dell'ordine stabile, non mescolate.
	TestTrue(TEXT("team 0 dall'inizio dell'ordine"), Start[0] == FRTCellId(0, 0) && Start[1] == FRTCellId(1, 0));
	TestTrue(TEXT("team 1 dalla fine dell'ordine"), Start[2] == FRTCellId(4, 0) && Start[3] == FRTCellId(3, 0));

	// Determinismo: la stessa mappa produce sempre lo stesso allestimento (nessun RNG, nessun ordine di TMap).
	TestTrue(TEXT("allestimento deterministico"), Start == URTMatchSetupLibrary::PickStartCells(Map, 2, 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchSetupSkipsBlockedTest,
	"RefactorTactics.MatchSetup.SkipsBlockedCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchSetupSkipsBlockedTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeSetupMap({ {0,0}, {1,0}, {2,0}, {3,0}, {4,0} }, { {1,0}, {3,0} });

	const TArray<FRTCellId> Start = URTMatchSetupLibrary::PickStartCells(Map, 1, 0);
	TestEqual(TEXT("due celle per un 1v1"), Start.Num(), 2);
	TestFalse(TEXT("niente partenze su celle bloccanti"), Start.Contains(FRTCellId(1, 0)));
	TestFalse(TEXT("niente partenze su celle bloccanti"), Start.Contains(FRTCellId(3, 0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchSetupInsufficientCellsTest,
	"RefactorTactics.MatchSetup.InsufficientCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchSetupInsufficientCellsTest::RunTest(const FString&)
{
	// Meno celle percorribili delle unita' da posare: non si allestisce a meta'.
	URTHexMapAsset* Map = MakeSetupMap({ {0,0}, {1,0} }, {});
	TestEqual(TEXT("mappa troppo piccola -> nessun allestimento"),
		URTMatchSetupLibrary::PickStartCells(Map, 2, 0).Num(), 0);
	TestEqual(TEXT("mappa nulla -> nessun allestimento"),
		URTMatchSetupLibrary::PickStartCells(nullptr, 2, 0).Num(), 0);

	// Il layer chiesto non esiste: le celle stanno sul layer 0.
	TestEqual(TEXT("layer vuoto -> nessun allestimento"),
		URTMatchSetupLibrary::PickStartCells(Map, 1, 3).Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchSetupOccupancyTest,
	"RefactorTactics.MatchSetup.BuildOccupancy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchSetupOccupancyTest::RunTest(const FString&)
{
	const TArray<FRTCellId> Cells { FRTCellId(0,0), FRTCellId(1,0), FRTCellId(2,0) };
	const TArray<int32> Ids { 10, 11, 12 };
	const TArray<bool> Alive { true, false, true };

	const TMap<FRTCellId, int32> Occ = URTMatchSetupLibrary::BuildOccupancy(Cells, Ids, Alive);
	TestEqual(TEXT("le unita' morte non occupano"), Occ.Num(), 2);
	TestEqual(TEXT("cella 0 occupata da 10"), Occ[FRTCellId(0, 0)], 10);
	TestEqual(TEXT("cella 2 occupata da 12"), Occ[FRTCellId(2, 0)], 12);
	TestFalse(TEXT("cella dell'unita' morta libera"), Occ.Contains(FRTCellId(1, 0)));

	// Input incoerente: non si indovina, si restituisce vuoto.
	TestEqual(TEXT("array di lunghezza diversa -> occupazione vuota"),
		URTMatchSetupLibrary::BuildOccupancy(Cells, { 10 }, Alive).Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchSetupDemoArenaTest,
	"RefactorTactics.MatchSetup.DemoArenaIsPlayable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchSetupDemoArenaTest::RunTest(const FString&)
{
	// Arena di ripiego: serve quando il livello non porta una mappa esagonale. Senza, il GameMode spawna un
	// ARTHexMapActor con l'asset VUOTO e la partita non si allestisce — premere Play non mostra nulla.
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeDemoArena(GetTransientPackage(), /*Radius=*/ 4);
	if (!TestNotNull(TEXT("l'arena di ripiego viene creata"), Arena)) { return false; }

	// Esagono pieno di raggio 4: 3*4*(4+1)+1 = 61 celle.
	TestEqual(TEXT("l'arena ha le celle di un esagono di raggio 4"), Arena->NumCells(), 61);
	TestTrue(TEXT("il centro esiste"), Arena->ContainsCell(FRTCellId(0, 0, 0)));

	// Deve bastare ad allestire un 2v2: e' l'unico requisito che il GameMode le chiede.
	const TArray<FRTCellId> Start = URTMatchSetupLibrary::PickStartCells(Arena, /*NumPerTeam=*/ 2, /*Layer=*/ 0);
	TestEqual(TEXT("l'arena permette di allestire un 2v2"), Start.Num(), 4);

	// Le due squadre partono lontane: un'arena in cui si parte adiacenti non e' giocabile.
	const int32 DistanceBetweenTeams = URTHexLibrary::HexDistance(Start[0], Start[3]);
	TestTrue(TEXT("le squadre partono a distanza di gioco"), DistanceBetweenTeams >= 4);

	// Raggio non valido: nessuna arena inventata a meta'.
	TestNull(TEXT("raggio negativo -> nessuna arena"),
		URTMatchSetupLibrary::MakeDemoArena(GetTransientPackage(), /*Radius=*/ -1));
	return true;
}

/**
 * La MAPPA DI PROVA deve avere davvero le caratteristiche che promette, altrimenti i playtest che poggiano su
 * di lei verificano meno di quanto dichiarano: e' proprio quello che e' successo con l'arena di ripiego, dove
 * «cella bloccata» e «LOS» non erano osservabili perche' la mappa non aveva ne' ostacoli ne' muri.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTestArenaFeaturesTest,
	"RefactorTactics.MatchSetup.TestArenaHasTheFeaturesItPromises",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTestArenaFeaturesTest::RunTest(const FString&)
{
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(GetTransientPackage());
	if (!TestNotNull(TEXT("arena generata"), Arena))
	{
		return false;
	}

	TestNull(TEXT("Outer nullo -> nessuna arena"), URTMatchSetupLibrary::MakeTestArena(nullptr));

	int32 BlocksMove = 0;
	int32 BlocksSight = 0;
	int32 Costly = 0;
	int32 Layer1 = 0;
	for (const FRTHexCellData& Cell : Arena->Cells)
	{
		BlocksMove  += Cell.bBlocksMovement ? 1 : 0;
		BlocksSight += Cell.bBlocksLineOfSight ? 1 : 0;
		Costly      += (Cell.MoveCost > 1) ? 1 : 0;
		Layer1      += (Cell.Id.Layer == 1) ? 1 : 0;
	}

	TestTrue(TEXT("ci sono ostacoli al movimento"), BlocksMove >= 3);
	TestTrue(TEXT("ci sono celle che bloccano la vista"), BlocksSight >= 3);
	TestTrue(TEXT("c'e' terreno costoso (MoveCost > 1)"), Costly >= 3);
	TestTrue(TEXT("c'e' una piattaforma sul layer 1"), Layer1 >= 4);

	// Una partita 2v2 ci si allestisce: e' il requisito minimo perche' la mappa sia giocabile.
	const TArray<FRTCellId> Start = URTMatchSetupLibrary::PickStartCells(Arena, /*NumPerTeam=*/ 2, /*Layer=*/ 0);
	TestEqual(TEXT("quattro celle di partenza sul layer 0"), Start.Num(), 4);

	// I muri stanno FRA le due meta': la vista da una parte all'altra non passa (e' il caso di PIE-HEXPLAY-6).
	if (Start.Num() == 4)
	{
		TestFalse(TEXT("nessuna linea di tiro fra le due meta' del campo"),
			URTHexVisionLibrary::HasLineOfSight(Arena, Start[0], Start[3]));
	}

	// Il terreno costoso morde davvero: raggiungere una cella oltre la fascia costa piu' del suo numero di passi.
	{
		const FRTHexSnapshot Snapshot = URTHexSimLibrary::MakeSnapshot(Arena,
			{ FRTHexSimUnit(0, FRTCellId(-4, 0, 0), /*MoveBudget=*/ 4) });
		const TArray<FRTHexReachableCell> Reach = URTHexSimLibrary::ReachableCells(Snapshot, 0);
		const FRTHexReachableCell* Beyond = Reach.FindByPredicate(
			[](const FRTHexReachableCell& R) { return R.Cell == FRTCellId(-1, 0, 0); });
		// (-4,0) -> (-1,0) sono 3 passi, ma uno attraversa la fascia di fango: il costo deve superare i 3 passi.
		TestTrue(TEXT("attraversare il fango costa piu' dei passi percorsi"),
			Beyond == nullptr || Beyond->Cost > 3);
	}

	// La piattaforma e' raggiungibile SOLO tramite la transizione: togliendola, il percorso deve fallire.
	{
		const FRTCellId Ground(1, 0, 0);
		const FRTCellId Platform(2, 0, 1);
		TestTrue(TEXT("con la transizione la piattaforma e' raggiungibile"),
			URTHexPathLibrary::FindPath(Arena, Ground, Platform).Status == ERTHexPathStatus::Success);

		Arena->RemoveTransition(Ground, Platform);
		TestTrue(TEXT("senza la transizione la piattaforma e' irraggiungibile (il path FALLISCE, non teletrasporta)"),
			URTHexPathLibrary::FindPath(Arena, Ground, Platform).Status != ERTHexPathStatus::Success);
	}
	return true;
}
