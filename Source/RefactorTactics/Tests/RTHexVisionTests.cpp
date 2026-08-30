#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h"
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

	/** Copertura ALTA sul bordo indicato: nega l'attraversamento (CP 9.2), quindi anche la vista. */
	void SetVisionHighCoverEdge(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		Data.Covers.Add(FRTHexCover(Edge, ERTHexCoverType::High, FRTHexCover::DefaultIntegrity(ERTHexCoverType::High)));
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
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

// ---------------------------------------------------------------------------------------------------------
// La RAGIONE del blocco (#1712): la stessa decisione, con il suo perche' e il suo punto
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexVisionReasonNamesTheCellTest,
	"RefactorTactics.HexVision.ReasonNamesTheCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexVisionReasonNamesTheCellTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeVisionMap(4);
	SetSightBlocker(M, FRTCellId(2, 0));

	const FRTLineOfSightResult R = URTHexVisionLibrary::DescribeLineOfSight(M, FRTCellId(0, 0), FRTCellId(3, 0));

	TestFalse(TEXT("la vista non passa"), R.IsClear());
	TestTrue(TEXT("la causa e' la CELLA, non il bordo"), R.Block == ERTLineOfSightBlock::CellBlocker);
	TestTrue(TEXT("nomina la cella colpevole"), R.BlockedAt == FRTCellId(2, 0));
	TestTrue(TEXT("nomina da dove si entrava"), R.BlockedFrom == FRTCellId(1, 0));
	TestEqual(TEXT("e il passo lungo la linea"), R.StepIndex, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexVisionReasonNamesTheEdgeTest,
	"RefactorTactics.HexVision.ReasonNamesTheEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexVisionReasonNamesTheEdgeTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeVisionMap(4);
	SetVisionHighCoverEdge(M, FRTCellId(2, 0), ERTHexDirection::W);

	// Precondizione esplicita: se il bordo non negasse l'attraversamento questo test misurerebbe altro.
	TestTrue(TEXT("il bordo (1,0)->(2,0) nega l'attraversamento"),
		URTHexCoverLibrary::BlocksTraversal(M, FRTCellId(1, 0), FRTCellId(2, 0)));

	const FRTLineOfSightResult R = URTHexVisionLibrary::DescribeLineOfSight(M, FRTCellId(0, 0), FRTCellId(3, 0));

	TestFalse(TEXT("la vista non passa"), R.IsClear());
	TestTrue(TEXT("la causa e' il BORDO, non la cella"), R.Block == ERTLineOfSightBlock::EdgeBlocker);
	TestTrue(TEXT("il lato colpevole parte da (1,0)"), R.BlockedFrom == FRTCellId(1, 0));
	TestTrue(TEXT("e arriva a (2,0)"), R.BlockedAt == FRTCellId(2, 0));

	// La proprieta' che distingue il bordo dalla cella: conta anche l'ULTIMO passo. Un muro addossato al
	// bersaglio lo copre, mentre `bBlocksLineOfSight` sul bersaglio non lo farebbe.
	URTHexMapAsset* Adjacent = MakeVisionMap(4);
	SetVisionHighCoverEdge(Adjacent, FRTCellId(3, 0), ERTHexDirection::W);
	const FRTLineOfSightResult Last = URTHexVisionLibrary::DescribeLineOfSight(Adjacent, FRTCellId(0, 0), FRTCellId(3, 0));
	TestTrue(TEXT("il bordo davanti al bersaglio blocca, ed e' un EdgeBlocker"),
		Last.Block == ERTLineOfSightBlock::EdgeBlocker && Last.BlockedAt == FRTCellId(3, 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexVisionReasonIsNoneWhereSightPassesTest,
	"RefactorTactics.HexVision.ReasonIsNoneWhereSightPasses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexVisionReasonIsNoneWhereSightPassesTest::RunTest(const FString&)
{
	// I tre modi di rispondere «passa» senza che ci sia una via libera nel senso comune: se uno di questi
	// producesse una ragione, un ispettore mostrerebbe un blocco dove il gioco non ne vede.
	const FRTCellId A(0, 0);
	const FRTCellId B(3, 0);

	TestTrue(TEXT("mappa nulla: nessun ostacolo NOTO, nessuna ragione"),
		URTHexVisionLibrary::DescribeLineOfSight(nullptr, A, B).IsClear());

	URTHexMapAsset* M = MakeVisionMap(4);
	TestTrue(TEXT("stessa colonna: niente in mezzo, nessuna ragione"),
		URTHexVisionLibrary::DescribeLineOfSight(M, A, A).IsClear());

	M->RemoveCell(FRTCellId(2, 0));
	M->SortCells();
	TestTrue(TEXT("cella assente: il vuoto non e' un muro, nessuna ragione"),
		URTHexVisionLibrary::DescribeLineOfSight(M, A, B).IsClear());

	// E gli estremi: bloccati entrambi, non si oscurano da soli.
	URTHexMapAsset* Ends = MakeVisionMap(4);
	SetSightBlocker(Ends, A);
	SetSightBlocker(Ends, B);
	const FRTLineOfSightResult R = URTHexVisionLibrary::DescribeLineOfSight(Ends, A, B);
	TestTrue(TEXT("gli estremi non producono una ragione"), R.IsClear());
	TestEqual(TEXT("e nessun passo e' colpevole"), R.StepIndex, static_cast<int32>(INDEX_NONE));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexVisionReasonKeepsTheLayerTest,
	"RefactorTactics.HexVision.ReasonKeepsTheLayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexVisionReasonKeepsTheLayerTest::RunTest(const FString&)
{
	// Multilayer: la linea resta sul layer del TIRATORE, quindi la cella colpevole deve portare QUEL layer.
	// Un ispettore che perdesse il layer disegnerebbe il blocco sulla cella sbagliata di due che condividono
	// (X, Y) — ed e' esattamente il caso in cui l'informazione serve.
	URTHexMapAsset* M = MakeVisionMap(4);

	FRTHexCellData Above(FRTCellId(2, 0, 1));
	Above.bBlocksLineOfSight = true;
	M->AddOrUpdateCell(Above);
	M->AddOrUpdateCell(FRTHexCellData(FRTCellId(0, 0, 1)));
	M->AddOrUpdateCell(FRTHexCellData(FRTCellId(3, 0, 1)));
	M->SortCells();

	TestTrue(TEXT("dal layer 0 si spara sotto il ponte: nessuna ragione"),
		URTHexVisionLibrary::DescribeLineOfSight(M, FRTCellId(0, 0, 0), FRTCellId(3, 0, 0)).IsClear());

	const FRTLineOfSightResult R = URTHexVisionLibrary::DescribeLineOfSight(M, FRTCellId(0, 0, 1), FRTCellId(3, 0, 1));
	TestTrue(TEXT("dal layer 1 lo stesso ostacolo blocca"), R.Block == ERTLineOfSightBlock::CellBlocker);
	TestEqual(TEXT("e la cella colpevole porta il layer del tiratore"), R.BlockedAt.Layer, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexVisionReasonAgreesWithHasLineOfSightTest,
	"RefactorTactics.HexVision.ReasonAgreesWithHasLineOfSight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexVisionReasonAgreesWithHasLineOfSightTest::RunTest(const FString&)
{
	// ⚠️ Oggi questa parita' e' STRUTTURALE — `HasLineOfSight` delega a `DescribeLineOfSight` — e il test
	// non puo' fallire per divergenza. Non e' ridondante: e' il test che si accorge del giorno in cui
	// qualcuno riscrive `HasLineOfSight` con un attraversamento proprio «per non pagare la struct», che e'
	// esattamente il modo in cui il debug comincia a mentire. Se la delega salta, questo diventa l'unico
	// oracolo che resta.
	URTHexMapAsset* M = MakeVisionMap(3);
	SetSightBlocker(M, FRTCellId(1, 0));
	SetSightBlocker(M, FRTCellId(-1, 1));
	SetSightBlocker(M, FRTCellId(0, -2));
	SetVisionHighCoverEdge(M, FRTCellId(1, -1), ERTHexDirection::W);
	SetVisionHighCoverEdge(M, FRTCellId(-2, 2), ERTHexDirection::E);

	const TArray<FRTHexCellData>& Cells = M->Cells;
	int32 Pairs = 0;
	int32 Blocked = 0;
	bool bAgrees = true;

	for (const FRTHexCellData& From : Cells)
	{
		for (const FRTHexCellData& To : Cells)
		{
			const FRTLineOfSightResult R = URTHexVisionLibrary::DescribeLineOfSight(M, From.Id, To.Id);
			bAgrees &= (R.IsClear() == URTHexVisionLibrary::HasLineOfSight(M, From.Id, To.Id));
			++Pairs;
			Blocked += R.IsClear() ? 0 : 1;
		}
	}

	TestTrue(TEXT("il corpus non e' vuoto"), Pairs > 100);
	TestTrue(TEXT("e contiene sia coppie libere sia coppie bloccate"), Blocked > 0 && Blocked < Pairs);
	TestTrue(TEXT("su ogni coppia il bool e' `ragione == None`"), bAgrees);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
