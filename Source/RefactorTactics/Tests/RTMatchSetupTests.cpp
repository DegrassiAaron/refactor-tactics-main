#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"

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
