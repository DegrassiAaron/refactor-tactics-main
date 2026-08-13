#include "Misc/AutomationTest.h"
#include "Map/RTGeometryBake.h"
#include "Map/RTGeometryGrammar.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexOccupancyLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr float BakeHexSize = 100.0f;
	const FRTCellId Origin{ 0, 0, 0 };

	/** Una mappa con una sola cella all'origine: il minimo su cui una cottura di bordi sia osservabile. */
	URTHexMapAsset* MakeOneCellMap()
	{
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
		FRTHexCellData Cell;
		Cell.Id = Origin;
		Map->AddOrUpdateCell(Cell);
		return Map;
	}

	/** Il muro sul lato `E`: asse `Deg90`, offset di un punto notevole, estremi sui due vertici. */
	FRTGeometrySegment WallOnEdge(ERTHexCoverType Type)
	{
		FRTGeometrySegment S;
		S.Axis = ERTTacticalAxis::Deg90;
		S.Offset = RT_GeometryQuanta;
		S.AlongStart = -RT_GeometryQuanta / 2;
		S.AlongEnd = RT_GeometryQuanta / 2;
		S.Layer = 0;
		S.WallType = Type;
		return S;
	}

	const FRTHexCover* FindCover(const URTHexMapAsset* Map, ERTHexDirection Edge)
	{
		const FRTHexCellData* Cell = Map->FindCell(Origin);
		if (Cell == nullptr) { return nullptr; }
		return Cell->Covers.FindByPredicate([Edge](const FRTHexCover& C) { return C.Edge == Edge; });
	}
}

/**
 * IL MAPPING, e il BORDO GIUSTO — che è la metà che un test sbagliato lascerebbe passare.
 *
 * Un muro appoggiato al lato `E` deve produrre una copertura sul bordo `E`, non su uno qualsiasi: la
 * direzionalità è l'unica cosa che una copertura porta oltre al tipo, e sbagliarla dà un riparo che protegge
 * dal lato opposto a quello murato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBakeWallToCoverTest,
	"RefactorTactics.GeometryBake.WallBakesToCoverOnTheRightEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBakeWallToCoverTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeOneCellMap();

	const int32 Generated = URTGeometryBakeLibrary::BakeCell(
		Map, Origin, { WallOnEdge(ERTHexCoverType::High) }, BakeHexSize);

	TestEqual(TEXT("un muro perimetrale genera una copertura"), Generated, 1);

	const FRTHexCover* Cover = FindCover(Map, ERTHexDirection::E);
	TestNotNull(TEXT("la copertura sta sul bordo E"), Cover);
	if (Cover)
	{
		TestTrue(TEXT("WALL -> High"), Cover->Type == ERTHexCoverType::High);
		TestEqual(TEXT("integrità di catalogo per High"), Cover->Integrity, 50);
		TestTrue(TEXT("ed è marcata come generata"), Cover->bGenerated);
	}

	// Nessun altro bordo è stato murato: un bake che marcasse tutti i sei bordi passerebbe il controllo sopra.
	const FRTHexCellData* Cell = Map->FindCell(Origin);
	TestEqual(TEXT("un solo bordo murato"), Cell ? Cell->Covers.Num() : -1, 1);

	// Il muretto cuoce nell'altro valore canonico, con la sua integrità.
	URTHexMapAsset* LowMap = MakeOneCellMap();
	URTGeometryBakeLibrary::BakeCell(LowMap, Origin, { WallOnEdge(ERTHexCoverType::Low) }, BakeHexSize);
	const FRTHexCover* LowCover = FindCover(LowMap, ERTHexDirection::E);
	TestNotNull(TEXT("anche il muretto cuoce"), LowCover);
	if (LowCover)
	{
		TestTrue(TEXT("LOW WALL -> Low"), LowCover->Type == ERTHexCoverType::Low);
		TestEqual(TEXT("integrità di catalogo per Low"), LowCover->Integrity, 30);
	}

	return true;
}

/**
 * IDEMPOTENZA — la proprietà per cui `bGenerated` esiste (`D-131`).
 *
 * Rieseguire il bake sulla stessa geometria non deve cambiare l'asset. Il confronto è sull'**hash**, non sul
 * conteggio: un bake che accumulasse coperture duplicate su bordi diversi passerebbe un test sui numeri.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBakeIsIdempotentTest,
	"RefactorTactics.GeometryBake.RebakeIsIdempotent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBakeIsIdempotentTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeOneCellMap();
	const TArray<FRTGeometrySegment> Geometry{ WallOnEdge(ERTHexCoverType::High) };

	URTGeometryBakeLibrary::BakeCell(Map, Origin, Geometry, BakeHexSize);
	const uint32 AfterFirst = Map->ComputeHash();

	URTGeometryBakeLibrary::BakeCell(Map, Origin, Geometry, BakeHexSize);
	const uint32 AfterSecond = Map->ComputeHash();

	TestEqual(TEXT("rieseguire il bake non cambia l'asset"), AfterSecond, AfterFirst);
	TestEqual(TEXT("e non accumula coperture"), URTGeometryBakeLibrary::CountGeneratedCovers(Map, Origin), 1);

	// E l'hash non è banalmente costante: senza questo, l'uguaglianza sopra passerebbe con un `ComputeHash`
	// che ignora le coperture.
	URTHexMapAsset* Bare = MakeOneCellMap();
	TestNotEqual(TEXT("una mappa senza cottura hasha diversamente"), Bare->ComputeHash(), AfterFirst);

	return true;
}

/**
 * LE DUE METÀ CHE LA PROVENIENZA RENDE POSSIBILI: togliere un segmento toglie la sua copertura, e una
 * copertura dipinta a mano sopravvive.
 *
 * È il nodo di `MSE-1`. Senza `bGenerated` nessuna delle due è esprimibile: un rebake che cancella tutto
 * distrugge il lavoro a mano, uno che non cancella nulla non sa togliere ciò che ha prodotto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBakeProvenanceTest,
	"RefactorTactics.GeometryBake.HandPaintedSurvivesAndRemovedSegmentUnbakes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBakeProvenanceTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeOneCellMap();

	// Una copertura dipinta a mano su un bordo DIVERSO da quello che il muro murerà.
	{
		FRTHexCellData Cell = *Map->FindCell(Origin);
		Cell.Covers.Add(FRTHexCover(ERTHexDirection::W, ERTHexCoverType::Low, 30));
		Map->AddOrUpdateCell(Cell);
	}

	URTGeometryBakeLibrary::BakeCell(Map, Origin, { WallOnEdge(ERTHexCoverType::High) }, BakeHexSize);

	TestNotNull(TEXT("la copertura a mano è ancora lì dopo il bake"), FindCover(Map, ERTHexDirection::W));
	TestNotNull(TEXT("e quella generata è stata scritta"), FindCover(Map, ERTHexDirection::E));

	// TOGLIERE il segmento: la copertura generata sparisce, quella a mano no.
	URTGeometryBakeLibrary::BakeCell(Map, Origin, {}, BakeHexSize);

	TestNull(TEXT("tolto il segmento, la sua copertura non c'è più"), FindCover(Map, ERTHexDirection::E));
	const FRTHexCover* Hand = FindCover(Map, ERTHexDirection::W);
	TestNotNull(TEXT("la copertura a mano sopravvive a un rebake che svuota"), Hand);
	if (Hand)
	{
		TestTrue(TEXT("ed è rimasta non generata"), !Hand->bGenerated);
	}

	return true;
}

/**
 * UNA COPERTURA A MANO VINCE SULLO STESSO BORDO.
 *
 * Caso separato dal precedente perché è quello che si perde per primo scrivendo il bake: là i bordi erano
 * diversi, qui il muro insiste esattamente dove l'autore aveva già deciso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBakeHandPaintedWinsTest,
	"RefactorTactics.GeometryBake.HandPaintedWinsOnTheSameEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBakeHandPaintedWinsTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeOneCellMap();
	{
		FRTHexCellData Cell = *Map->FindCell(Origin);
		Cell.Covers.Add(FRTHexCover(ERTHexDirection::E, ERTHexCoverType::Low, 30));
		Map->AddOrUpdateCell(Cell);
	}

	const int32 Generated = URTGeometryBakeLibrary::BakeCell(
		Map, Origin, { WallOnEdge(ERTHexCoverType::High) }, BakeHexSize);

	TestEqual(TEXT("il bake non genera nulla su un bordo già dell'autore"), Generated, 0);

	const FRTHexCover* Cover = FindCover(Map, ERTHexDirection::E);
	TestNotNull(TEXT("la copertura del bordo esiste ancora"), Cover);
	if (Cover)
	{
		TestTrue(TEXT("ed è rimasta quella a mano, Low"), Cover->Type == ERTHexCoverType::Low);
		TestTrue(TEXT("non marcata come generata"), !Cover->bGenerated);
	}

	return true;
}

/**
 * `bGenerated` NON ENTRA NELL'HASH — il vincolo di `D-131`.
 *
 * Due mappe che si giocano in modo **identico** devono avere lo stesso hash. La provenienza di una copertura
 * non cambia una partita: se entrasse, una mappa disegnata e una dipinta identiche divergerebbero, che è un
 * falso positivo contro il KPI `replay divergence = 0`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBakeProvenanceIsNotInHashTest,
	"RefactorTactics.GeometryBake.ProvenanceDoesNotChangeTheHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBakeProvenanceIsNotInHashTest::RunTest(const FString&)
{
	// Stessa copertura, stesso bordo, stesso tipo, stessa integrità: cambia SOLO la provenienza.
	URTHexMapAsset* Painted = MakeOneCellMap();
	{
		FRTHexCellData Cell = *Painted->FindCell(Origin);
		Cell.Covers.Add(FRTHexCover(ERTHexDirection::E, ERTHexCoverType::High, 50));
		Painted->AddOrUpdateCell(Cell);
	}

	URTHexMapAsset* Baked = MakeOneCellMap();
	URTGeometryBakeLibrary::BakeCell(Baked, Origin, { WallOnEdge(ERTHexCoverType::High) }, BakeHexSize);

	// Le due coperture sono identiche salvo `bGenerated`: la controprova è qui sotto, altrimenti il test
	// direbbe solo che due mappe a caso hanno lo stesso hash.
	const FRTHexCover* A = FindCover(Painted, ERTHexDirection::E);
	const FRTHexCover* B = FindCover(Baked, ERTHexDirection::E);
	TestNotNull(TEXT("copertura dipinta"), A);
	TestNotNull(TEXT("copertura cotta"), B);
	if (A && B)
	{
		TestTrue(TEXT("stesso bordo"), A->Edge == B->Edge);
		TestTrue(TEXT("stesso tipo"), A->Type == B->Type);
		TestEqual(TEXT("stessa integrità"), A->Integrity, B->Integrity);
		TestTrue(TEXT("e differiscono SOLO per la provenienza"), A->bGenerated != B->bGenerated);
	}

	TestEqual(TEXT("la provenienza non cambia l'hash"), Baked->ComputeHash(), Painted->ComputeHash());

	return true;
}

/**
 * DETERMINISMO: l'esito non dipende dall'ordine in cui i segmenti arrivano, e `ValidateMap` regge sul cotto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBakeOrderAndValidationTest,
	"RefactorTactics.GeometryBake.OrderIndependentAndValidateMapStillPasses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBakeOrderAndValidationTest::RunTest(const FString&)
{
	FRTGeometrySegment WallE = WallOnEdge(ERTHexCoverType::High);

	// Un secondo muro, sul lato opposto: asse `Deg90`, offset speculare.
	FRTGeometrySegment WallW = WallOnEdge(ERTHexCoverType::Low);
	WallW.Offset = -RT_GeometryQuanta;

	URTHexMapAsset* Forward = MakeOneCellMap();
	URTGeometryBakeLibrary::BakeCell(Forward, Origin, { WallE, WallW }, BakeHexSize);

	URTHexMapAsset* Backward = MakeOneCellMap();
	URTGeometryBakeLibrary::BakeCell(Backward, Origin, { WallW, WallE }, BakeHexSize);

	TestEqual(TEXT("stesso hash comunque ordinati i segmenti"), Backward->ComputeHash(), Forward->ComputeHash());
	TestEqual(TEXT("due bordi murati"), URTGeometryBakeLibrary::CountGeneratedCovers(Forward, Origin), 2);

	// `ValidateMap` continua a passare sui dati cotti: un bordo con due coperture, o un'integrità nulla,
	// sarebbero errori che il bake può introdurre senza accorgersene.
	const TArray<FString> Errors = Forward->ValidateMap();
	TestEqual(TEXT("ValidateMap non segnala errori sul cotto"), Errors.Num(), 0);
	if (Errors.Num() > 0)
	{
		AddError(FString::Printf(TEXT("primo errore: %s"), *Errors[0]));
	}

	return true;
}

/**
 * IL CONFINE CON `D-129`: il bake NON tocca il volume.
 *
 * `bBlocksMovement` resta del pennello, un produttore solo. Un bake che lo scrivesse creerebbe il campo a due
 * produttori che `MSE-1` aveva sollevato e che `D-129` ha evitato togliendo il volume dallo scope.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBakeDoesNotTouchVolumeTest,
	"RefactorTactics.GeometryBake.BakeDoesNotWriteMovementBlocking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBakeDoesNotTouchVolumeTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeOneCellMap();

	const FRTHexCellData* Before = Map->FindCell(Origin);
	const bool bBlockedBefore = Before && Before->bBlocksMovement;
	const bool bLosBefore = Before && Before->bBlocksLineOfSight;
	const int32 SurchargeBefore = Before ? Before->OccupancySurcharge : -1;

	URTGeometryBakeLibrary::BakeCell(Map, Origin, { WallOnEdge(ERTHexCoverType::High) }, BakeHexSize);

	const FRTHexCellData* After = Map->FindCell(Origin);
	TestTrue(TEXT("bBlocksMovement invariato"), After && After->bBlocksMovement == bBlockedBefore);
	TestTrue(TEXT("bBlocksLineOfSight invariato"), After && After->bBlocksLineOfSight == bLosBefore);
	TestEqual(TEXT("il sovrapprezzo di occupancy resta di #619"),
		After ? After->OccupancySurcharge : -1, SurchargeBefore);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
