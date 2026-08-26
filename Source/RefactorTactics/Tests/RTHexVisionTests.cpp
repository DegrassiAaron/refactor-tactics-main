#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Esagono pieno di raggio N sul layer 0, nessun ostacolo alla vista. Nome distinto per file (unity build). */
	URTHexMapAsset* MakeVisionMap(int32 Radius)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		return M;
	}

	/** Marca una cella come ostacolo alla vista (creandola se assente). */
	void SetSightBlocker(URTHexMapAsset* Map, const FRTCellId& Id)
	{
		FRTHexCellData Data = Map->FindCell(Id) ? *Map->FindCell(Id) : FRTHexCellData(Id);
		Data.Id = Id;
		Data.bBlocksLineOfSight = true;
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}
}

// ---------------------------------------------------------------------------------------------------------
// Geometria: linea
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLineStraightTest,
	"RefactorTactics.Hex.HexLineStraight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLineStraightTest::RunTest(const FString&)
{
	const FRTCellId A(0, 0);
	const FRTCellId B(3, 0);
	const TArray<FRTCellId> Line = URTHexLibrary::HexLine(A, B);

	TestEqual(TEXT("lunghezza = distanza + 1"), Line.Num(), URTHexLibrary::HexDistance(A, B) + 1);
	TestTrue(TEXT("parte da A"), Line.Num() > 0 && Line[0] == A);
	TestTrue(TEXT("arriva a B"), Line.Num() > 0 && Line.Last() == B);
	TestTrue(TEXT("segue la direzione E"), Line.Num() == 4 && Line[1] == FRTCellId(1, 0) && Line[2] == FRTCellId(2, 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLineAdjacencyTest,
	"RefactorTactics.Hex.HexLineAdjacency",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLineAdjacencyTest::RunTest(const FString&)
{
	// Linee oblique: la proprieta' fondamentale e' che ogni passo sia verso un vicino (nessun "salto").
	const FRTCellId Ends[] = { FRTCellId(4, -2), FRTCellId(-3, 5), FRTCellId(2, 3), FRTCellId(-4, -1) };
	for (const FRTCellId& End : Ends)
	{
		const FRTCellId Start(0, 0);
		const TArray<FRTCellId> Line = URTHexLibrary::HexLine(Start, End);
		TestEqual(*FString::Printf(TEXT("lunghezza verso %s"), *End.ToString()),
			Line.Num(), URTHexLibrary::HexDistance(Start, End) + 1);

		bool bAllAdjacent = Line.Num() > 0;
		for (int32 i = 1; i < Line.Num(); ++i)
		{
			bAllAdjacent &= URTHexLibrary::HexDistance(Line[i - 1], Line[i]) == 1;
		}
		TestTrue(*FString::Printf(TEXT("passi adiacenti verso %s"), *End.ToString()), bAllAdjacent);
		TestTrue(TEXT("estremi corretti"), Line.Num() > 0 && Line[0] == Start && Line.Last() == End);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLineDegenerateTest,
	"RefactorTactics.Hex.HexLineDegenerate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLineDegenerateTest::RunTest(const FString&)
{
	const TArray<FRTCellId> Same = URTHexLibrary::HexLine(FRTCellId(2, -1), FRTCellId(2, -1));
	TestEqual(TEXT("A == B -> una sola cella"), Same.Num(), 1);

	const TArray<FRTCellId> Adjacent = URTHexLibrary::HexLine(FRTCellId(0, 0), FRTCellId(0, 1));
	TestEqual(TEXT("vicini -> due celle"), Adjacent.Num(), 2);

	// Il layer di partenza e' quello della linea (planare, come HexDistance).
	const TArray<FRTCellId> Elevated = URTHexLibrary::HexLine(FRTCellId(0, 0, 2), FRTCellId(2, 0, 2));
	bool bSameLayer = Elevated.Num() > 0;
	for (const FRTCellId& C : Elevated)
	{
		bSameLayer &= C.Layer == 2;
	}
	TestTrue(TEXT("la linea resta sul layer di partenza"), bSameLayer);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLineSymmetricLengthTest,
	"RefactorTactics.Hex.HexLineSymmetricLength",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLineSymmetricLengthTest::RunTest(const FString&)
{
	const FRTCellId A(-2, 3);
	const FRTCellId B(3, -1);
	const TArray<FRTCellId> Forward = URTHexLibrary::HexLine(A, B);
	const TArray<FRTCellId> Backward = URTHexLibrary::HexLine(B, A);

	TestEqual(TEXT("stessa lunghezza nei due versi"), Backward.Num(), Forward.Num());
	TestTrue(TEXT("estremi scambiati"),
		Forward.Num() > 0 && Backward.Num() > 0 && Forward[0] == Backward.Last() && Forward.Last() == Backward[0]);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Geometria: cono
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexConeCoverageTest,
	"RefactorTactics.Hex.HexConeCoverage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexConeCoverageTest::RunTest(const FString&)
{
	const FRTCellId From(0, 0);
	const FRTCellId Target(3, 0); // direzione E

	const TArray<FRTCellId> Near = URTHexLibrary::HexCone(From, Target, 1);
	TestEqual(TEXT("raggio 1 -> 3 celle (ventaglio 120 gradi)"), Near.Num(), 3);
	TestTrue(TEXT("include la cella davanti"), Near.Contains(FRTCellId(1, 0)));
	TestTrue(TEXT("include le due adiacenti al fronte"),
		Near.Contains(FRTCellId(1, -1)) && Near.Contains(FRTCellId(0, 1)));
	TestFalse(TEXT("esclude l'origine"), Near.Contains(From));
	TestFalse(TEXT("esclude la cella alle spalle"), Near.Contains(FRTCellId(-1, 0)));

	const TArray<FRTCellId> Far = URTHexLibrary::HexCone(From, Target, 2);
	TestEqual(TEXT("raggio 2 -> 8 celle (3 + 5)"), Far.Num(), 8);

	bool bWithinRange = true;
	for (const FRTCellId& C : Far)
	{
		bWithinRange &= URTHexLibrary::HexDistance(From, C) <= 2 && URTHexLibrary::HexDistance(From, C) >= 1;
	}
	TestTrue(TEXT("tutte le celle entro il raggio, origine esclusa"), bWithinRange);

	bool bSorted = true;
	for (int32 i = 1; i < Far.Num(); ++i)
	{
		bSorted &= URTHexLibrary::StableLess(Far[i - 1], Far[i]);
	}
	TestTrue(TEXT("output ordinato in modo stabile (nessun duplicato)"), bSorted);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexConeDegenerateTest,
	"RefactorTactics.Hex.HexConeDegenerate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexConeDegenerateTest::RunTest(const FString&)
{
	const FRTCellId From(1, 1);
	TestEqual(TEXT("bersaglio sull'origine -> vuoto"), URTHexLibrary::HexCone(From, From, 3).Num(), 0);
	TestEqual(TEXT("raggio 0 -> vuoto"), URTHexLibrary::HexCone(From, FRTCellId(3, 1), 0).Num(), 0);
	TestEqual(TEXT("raggio negativo -> vuoto"), URTHexLibrary::HexCone(From, FRTCellId(3, 1), -2).Num(), 0);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Linea di vista sulla mappa
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexVisionWallBlocksTest,
	"RefactorTactics.HexVision.WallBlocks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexVisionWallBlocksTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeVisionMap(4);
	const FRTCellId Shooter(0, 0);
	const FRTCellId Target(3, 0);

	TestTrue(TEXT("senza ostacoli la vista passa"), URTHexVisionLibrary::HasLineOfSight(M, Shooter, Target));

	SetSightBlocker(M, FRTCellId(2, 0)); // in mezzo alla linea
	TestFalse(TEXT("un muro sulla linea blocca"), URTHexVisionLibrary::HasLineOfSight(M, Shooter, Target));

	// Un muro FUORI dalla linea non blocca.
	URTHexMapAsset* Clear = MakeVisionMap(4);
	SetSightBlocker(Clear, FRTCellId(0, 2));
	TestTrue(TEXT("un muro fuori linea non blocca"), URTHexVisionLibrary::HasLineOfSight(Clear, Shooter, Target));

	TestTrue(TEXT("mappa nulla -> nessun ostacolo noto"), URTHexVisionLibrary::HasLineOfSight(nullptr, Shooter, Target));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexVisionEndpointsTest,
	"RefactorTactics.HexVision.EndpointsNeverBlock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexVisionEndpointsTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeVisionMap(4);
	const FRTCellId Shooter(0, 0);
	const FRTCellId Target(3, 0);

	SetSightBlocker(M, Shooter);
	SetSightBlocker(M, Target);
	TestTrue(TEXT("tiratore e bersaglio non si coprono da soli"),
		URTHexVisionLibrary::HasLineOfSight(M, Shooter, Target));

	TestTrue(TEXT("stessa cella -> sempre visibile"), URTHexVisionLibrary::HasLineOfSight(M, Shooter, Shooter));

	// Celle adiacenti: nessuna cella intermedia, quindi nulla puo' bloccare.
	URTHexMapAsset* Walled = MakeVisionMap(4);
	SetSightBlocker(Walled, FRTCellId(1, 0));
	TestTrue(TEXT("adiacenti sempre visibili"),
		URTHexVisionLibrary::HasLineOfSight(Walled, Shooter, FRTCellId(1, 0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexVisionEmptyCellTest,
	"RefactorTactics.HexVision.EmptyCellDoesNotBlock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexVisionEmptyCellTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeVisionMap(4);
	M->RemoveCell(FRTCellId(2, 0)); // buco nella mappa lungo la linea
	M->SortCells();

	TestTrue(TEXT("il vuoto non e' un muro"),
		URTHexVisionLibrary::HasLineOfSight(M, FRTCellId(0, 0), FRTCellId(3, 0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexVisionElevationTest,
	"RefactorTactics.HexVision.ElevationRule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexVisionElevationTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeVisionMap(4);

	// Stesso (X,Y) del punto di mezzo ma su un layer diverso: non deve bloccare chi spara dal layer 0.
	FRTHexCellData Above(FRTCellId(2, 0, 1));
	Above.bBlocksLineOfSight = true;
	M->AddOrUpdateCell(Above);
	M->SortCells();

	TestTrue(TEXT("ostacolo su un altro layer non blocca (si spara sotto il ponte)"),
		URTHexVisionLibrary::HasLineOfSight(M, FRTCellId(0, 0, 0), FRTCellId(3, 0, 0)));

	// Lo stesso ostacolo blocca chi spara DAL suo layer.
	M->AddOrUpdateCell(FRTHexCellData(FRTCellId(0, 0, 1)));
	M->AddOrUpdateCell(FRTHexCellData(FRTCellId(3, 0, 1)));
	M->SortCells();
	TestFalse(TEXT("lo stesso ostacolo blocca sul proprio layer"),
		URTHexVisionLibrary::HasLineOfSight(M, FRTCellId(0, 0, 1), FRTCellId(3, 0, 1)));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
