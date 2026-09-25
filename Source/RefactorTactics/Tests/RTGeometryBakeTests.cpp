#include "Misc/AutomationTest.h"
#include "Map/RTGeometryBake.h"
#include "Map/RTHexCoverPlacementLibrary.h"
#include "Map/RTGeometryGrammar.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexOccupancyLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr float BakeHexSize = 100.0f;
	// ⚠️ Nome specifico, non `Origin`: nella unity build questo namespace anonimo finisce nella stessa unit
	// di traduzione di altri test, e un `Origin` qui NASCONDE l'omonimo la' (C4459, che qui e' un errore).
	// Non e' teoria: e' successo appena il raggruppamento dei file e' cambiato.
	const FRTCellId BakeOrigin{ 0, 0, 0 };

	/** Una mappa con una sola cella all'origine: il minimo su cui una cottura di bordi sia osservabile. */
	URTHexMapAsset* MakeOneCellMap()
	{
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
		FRTHexCellData Cell;
		Cell.Id = BakeOrigin;
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
		const FRTHexCellData* Cell = Map->FindCell(BakeOrigin);
		if (Cell == nullptr) { return nullptr; }
		return Cell->Covers.FindByPredicate([Edge](const FRTHexCover& C) { return C.Edge == Edge; });
	}

	/** L'origine piu' i suoi sei vicini: il minimo su cui «quante celle ha toccato» sia una domanda vera. */
	URTHexMapAsset* MakeNeighbourhoodMap()
	{
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
		FRTHexCellData Centre;
		Centre.Id = BakeOrigin;
		Map->AddOrUpdateCell(Centre);
		for (const FRTCellId& N : URTHexLibrary::Neighbors(BakeOrigin))
		{
			FRTHexCellData Cell;
			Cell.Id = N;
			Map->AddOrUpdateCell(Cell);
		}
		return Map;
	}

	/**
	 * Le coperture di ogni cella, in forma confrontabile. Non l'hash della mappa: quello dice **che**
	 * qualcosa e' cambiato, non **dove** — e qui la domanda e' esattamente dove.
	 */
	TMap<FRTCellId, FString> SnapshotCovers(const URTHexMapAsset* Map)
	{
		TMap<FRTCellId, FString> Out;
		for (const FRTHexCellData& Cell : Map->Cells)
		{
			// Ordine di bordo crescente: `Covers` e' un array, e due mappe uguali non devono differire
			// per come le coperture ci sono finite dentro.
			TArray<FString> Parts;
			for (int32 EdgeIndex = 0; EdgeIndex < 6; ++EdgeIndex)
			{
				const ERTHexDirection Edge = static_cast<ERTHexDirection>(EdgeIndex);
				const FRTHexCover* Cover = Cell.Covers.FindByPredicate(
					[Edge](const FRTHexCover& C) { return C.Edge == Edge; });
				if (Cover)
				{
					Parts.Add(FString::Printf(TEXT("%d:%d/%d/%d"), EdgeIndex,
						static_cast<int32>(Cover->Type), Cover->Integrity, Cover->bGenerated ? 1 : 0));
				}
			}
			Out.Add(Cell.Id, FString::Join(Parts, TEXT(",")));
		}
		return Out;
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
		Map, BakeOrigin, { WallOnEdge(ERTHexCoverType::High) }, BakeHexSize);

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
	const FRTHexCellData* Cell = Map->FindCell(BakeOrigin);
	TestEqual(TEXT("un solo bordo murato"), Cell ? Cell->Covers.Num() : -1, 1);

	// Il muretto cuoce nell'altro valore canonico, con la sua integrità.
	URTHexMapAsset* LowMap = MakeOneCellMap();
	URTGeometryBakeLibrary::BakeCell(LowMap, BakeOrigin, { WallOnEdge(ERTHexCoverType::Low) }, BakeHexSize);
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

	URTGeometryBakeLibrary::BakeCell(Map, BakeOrigin, Geometry, BakeHexSize);
	const uint32 AfterFirst = Map->ComputeHash();

	URTGeometryBakeLibrary::BakeCell(Map, BakeOrigin, Geometry, BakeHexSize);
	const uint32 AfterSecond = Map->ComputeHash();

	TestEqual(TEXT("rieseguire il bake non cambia l'asset"), AfterSecond, AfterFirst);
	TestEqual(TEXT("e non accumula coperture"), URTGeometryBakeLibrary::CountGeneratedCovers(Map, BakeOrigin), 1);

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
		FRTHexCellData Cell = *Map->FindCell(BakeOrigin);
		Cell.Covers.Add(FRTHexCover(ERTHexDirection::W, ERTHexCoverType::Low, 30));
		Map->AddOrUpdateCell(Cell);
	}

	URTGeometryBakeLibrary::BakeCell(Map, BakeOrigin, { WallOnEdge(ERTHexCoverType::High) }, BakeHexSize);

	TestNotNull(TEXT("la copertura a mano è ancora lì dopo il bake"), FindCover(Map, ERTHexDirection::W));
	TestNotNull(TEXT("e quella generata è stata scritta"), FindCover(Map, ERTHexDirection::E));

	// TOGLIERE il segmento: la copertura generata sparisce, quella a mano no.
	URTGeometryBakeLibrary::BakeCell(Map, BakeOrigin, {}, BakeHexSize);

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
		FRTHexCellData Cell = *Map->FindCell(BakeOrigin);
		Cell.Covers.Add(FRTHexCover(ERTHexDirection::E, ERTHexCoverType::Low, 30));
		Map->AddOrUpdateCell(Cell);
	}

	const int32 Generated = URTGeometryBakeLibrary::BakeCell(
		Map, BakeOrigin, { WallOnEdge(ERTHexCoverType::High) }, BakeHexSize);

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
		FRTHexCellData Cell = *Painted->FindCell(BakeOrigin);
		Cell.Covers.Add(FRTHexCover(ERTHexDirection::E, ERTHexCoverType::High, 50));
		Painted->AddOrUpdateCell(Cell);
	}

	URTHexMapAsset* Baked = MakeOneCellMap();
	URTGeometryBakeLibrary::BakeCell(Baked, BakeOrigin, { WallOnEdge(ERTHexCoverType::High) }, BakeHexSize);

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
	URTGeometryBakeLibrary::BakeCell(Forward, BakeOrigin, { WallE, WallW }, BakeHexSize);

	URTHexMapAsset* Backward = MakeOneCellMap();
	URTGeometryBakeLibrary::BakeCell(Backward, BakeOrigin, { WallW, WallE }, BakeHexSize);

	TestEqual(TEXT("stesso hash comunque ordinati i segmenti"), Backward->ComputeHash(), Forward->ComputeHash());
	TestEqual(TEXT("due bordi murati"), URTGeometryBakeLibrary::CountGeneratedCovers(Forward, BakeOrigin), 2);

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

	const FRTHexCellData* Before = Map->FindCell(BakeOrigin);
	const bool bBlockedBefore = Before && Before->bBlocksMovement;
	const bool bLosBefore = Before && Before->bBlocksLineOfSight;
	const int32 SurchargeBefore = Before ? Before->OccupancySurcharge : -1;

	URTGeometryBakeLibrary::BakeCell(Map, BakeOrigin, { WallOnEdge(ERTHexCoverType::High) }, BakeHexSize);

	const FRTHexCellData* After = Map->FindCell(BakeOrigin);
	TestTrue(TEXT("bBlocksMovement invariato"), After && After->bBlocksMovement == bBlockedBefore);
	TestTrue(TEXT("bBlocksLineOfSight invariato"), After && After->bBlocksLineOfSight == bLosBefore);
	TestEqual(TEXT("il sovrapprezzo di occupancy resta di #619"),
		After ? After->OccupancySurcharge : -1, SurchargeBefore);

	return true;
}

/**
 * L'ESTENSIONE del rebake — `#883`, la voce di DoD di `#621` rimasta scoperta.
 *
 * Lo scope di `#621` diceva *«rebake della sola regione investita, non dell'intera mappa»*, e nessuno dei
 * sette test esistenti misura **quante** celle il bake tocca. `RebakeIsIdempotent` è la più vicina e non
 * basta: l'idempotenza è vera anche per un bake che riscrivesse **tutta** la mappa, purché la riscriva
 * sempre uguale.
 *
 * ⚠️ **Il caso da non sbagliare è il vicino a Est.** Quel bordo è condiviso, e la tentazione ovvia —
 * *«scrivo la copertura su tutte e due le facce, così è coerente»* — sarebbe un difetto: `CoverBetween`
 * legge già entrambe le facce (*«la barriera è fisica, non un attributo di chi la possiede»*), quindi la
 * seconda scrittura non aggiunge nulla al gioco e aggiunge un elemento all'array che entra in
 * `ComputeHash`. Due mappe che si giocano identiche avrebbero hash diversi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBakeTouchesOnlyTheInvestedRegionTest,
	"RefactorTactics.GeometryBake.RebakeTouchesOnlyTheInvestedRegion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBakeTouchesOnlyTheInvestedRegionTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeNeighbourhoodMap();
	TestEqual(TEXT("la fixture ha sette celle"), Map->Cells.Num(), 7);

	// Una copertura DIPINTA A MANO su un vicino: se il bake allargasse la regione, questa è la prima a
	// cadere — ed è anche il dato che `bGenerated` esiste per proteggere.
	const FRTCellId NorthEast = URTHexLibrary::Neighbor(BakeOrigin, ERTHexDirection::NE);
	{
		FRTHexCellData Cell = *Map->FindCell(NorthEast);
		FRTHexCover Hand(ERTHexDirection::W, ERTHexCoverType::Low, 30);
		Hand.bGenerated = false;
		Cell.Covers.Add(Hand);
		Map->AddOrUpdateCell(Cell);
	}

	const TMap<FRTCellId, FString> Before = SnapshotCovers(Map);

	URTGeometryBakeLibrary::BakeCell(Map, BakeOrigin, { WallOnEdge(ERTHexCoverType::High) }, BakeHexSize);

	const TMap<FRTCellId, FString> After = SnapshotCovers(Map);

	TestEqual(TEXT("il bake non aggiunge né toglie celle"), After.Num(), Before.Num());

	// Il conteggio, che è il criterio della voce di DoD: **una** cella cambiata, e si sa quale.
	TArray<FRTCellId> Changed;
	for (const TPair<FRTCellId, FString>& Pair : After)
	{
		const FString* Old = Before.Find(Pair.Key);
		if (Old == nullptr || *Old != Pair.Value)
		{
			Changed.Add(Pair.Key);
		}
	}
	TestEqual(TEXT("il bake ha toccato esattamente una cella"), Changed.Num(), 1);
	if (Changed.Num() == 1)
	{
		TestTrue(FString::Printf(TEXT("ed è la cella investita, non %s"), *Changed[0].ToString()),
			Changed[0] == BakeOrigin);
	}

	// Il verso opposto, esplicito: il vicino che CONDIVIDE il bordo murato non ha ricevuto niente.
	const FRTCellId East = URTHexLibrary::Neighbor(BakeOrigin, ERTHexDirection::E);
	const FRTHexCellData* EastCell = Map->FindCell(East);
	TestEqual(TEXT("il vicino a Est non riceve la faccia opposta del bordo"),
		EastCell ? EastCell->Covers.Num() : -1, 0);

	// E la copertura dipinta a mano su un altro vicino è ancora lì, intatta.
	const FRTHexCellData* NorthEastCell = Map->FindCell(NorthEast);
	TestEqual(TEXT("la copertura a mano di un vicino sopravvive"),
		NorthEastCell ? NorthEastCell->Covers.Num() : -1, 1);
	if (NorthEastCell && NorthEastCell->Covers.Num() == 1)
	{
		TestFalse(TEXT("e non è stata riclassificata come generata"), NorthEastCell->Covers[0].bGenerated);
	}

	return true;
}

/**
 * #712 / seduta `U22`: la copertura cotta finisce sul lato che guarda IL VICINO GIUSTO.
 *
 * 🔴 Non lo faceva. `EdgesTouchedBy` numerava i bordi per angolo crescente e li passava a
 * `ERTHexDirection` con un `static_cast`, ma quell'enum numera per **direzione di vicinato** e le due
 * girano in verso opposto: `E` e `W` coincidono, i quattro diagonali erano scambiati a coppie. Una
 * copertura disegnata a `NE` finiva a `SE`, e siccome `NeighborAcross` e `RTHexCombatLibrary` leggono
 * `Cover.Edge` come «verso quel vicino», bloccava vista e passo **dal lato opposto**.
 *
 * ⚠️ **I sette test qui sopra non potevano vederlo**: usano tutti `E` o `W`, cioe' esattamente i due punti
 * fissi del rispecchiamento. Il difetto non e' sopravvissuto a un test debole — e' sopravvissuto alla
 * scelta dei casi, che e' un modo piu' silenzioso di non coprire.
 *
 * Il test lega l'esito alla geometria del mondo invece che a una tabella: per ogni direzione costruisce il
 * muro sul lato **condiviso con quel vicino**, e pretende quella direzione. Una tabella di attesi
 * ricopierebbe la convenzione che sta verificando.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBakeCoverLandsTowardTheNeighbourTest,
	"RefactorTactics.Geometry.BakeCoverLandsTowardTheNeighbour",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBakeCoverLandsTowardTheNeighbourTest::RunTest(const FString&)
{
	constexpr float HexSize = 100.f;

	for (int32 DirIndex = 0; DirIndex < 6; ++DirIndex)
	{
		const ERTHexDirection Dir = static_cast<ERTHexDirection>(DirIndex);

		// Il lato condiviso col vicino `Dir`: il suo punto medio sta a meta' strada fra i due centri, e i
		// suoi due estremi sono i vertici a +-30 gradi da quella giacitura. Tutto derivato dal mondo.
		const FVector Here = URTHexLibrary::AxialToWorld(BakeOrigin, FVector::ZeroVector, HexSize, 0.f);
		const FVector There = URTHexLibrary::AxialToWorld(
			URTHexLibrary::Neighbor(BakeOrigin, Dir), FVector::ZeroVector, HexSize, 0.f);

		const double MidAngle = FMath::Atan2(There.Y - Here.Y, There.X - Here.X);
		const double Deg30 = PI / 6.0;
		const FVector2D A(HexSize * FMath::Cos(MidAngle - Deg30), HexSize * FMath::Sin(MidAngle - Deg30));
		const FVector2D B(HexSize * FMath::Cos(MidAngle + Deg30), HexSize * FMath::Sin(MidAngle + Deg30));

		FRTGeometrySegment Wall;
		if (!TestTrue(FString::Printf(TEXT("il lato verso %d si aggancia alla grammatica"), DirIndex),
			URTGeometryGrammarLibrary::SnapToGrammar(A, B, HexSize, Wall)))
		{
			continue;
		}
		Wall.WallType = ERTHexCoverType::High;

		TArray<ERTHexDirection> Touched;
		URTGeometryBakeLibrary::EdgesTouchedBy(Wall, HexSize, Touched);
		TestEqual(FString::Printf(TEXT("il lato verso %d mura un bordo solo"), DirIndex), Touched.Num(), 1);
		if (Touched.Num() != 1)
		{
			continue;
		}
		TestEqual(FString::Printf(TEXT("il muro verso %d mura proprio quel lato"), DirIndex),
			static_cast<int32>(Touched[0]), DirIndex);

		// E la stessa cosa attraverso la cottura vera, non solo attraverso il calcolo dei bordi.
		URTHexMapAsset* Map = MakeOneCellMap();
		URTGeometryBakeLibrary::BakeCell(Map, BakeOrigin, { Wall }, HexSize);
		TestNotNull(FString::Printf(TEXT("la cottura scrive la copertura verso %d"), DirIndex),
			FindCover(Map, Dir));
	}

	return true;
}

/**
 * #712 / seduta `U22`: due muri che condividono un vertice restano DUE.
 *
 * 🔴 Il difetto, trovato disegnando: il secondo tratto faceva sparire il primo. `BakeCell` rimuove tutte
 * le coperture generate della cella prima di riscrivere — contratto di **rebake**, giusto per chi possiede
 * l'elenco completo dei segmenti — e il tool d'editor ne possiede uno solo, perche' di un gesto per volta
 * e' tutto cio' che vede. Il commento sopra la chiamata diceva *«non accumula e non cancella»*, che era
 * vero delle coperture a mano e falso di quelle generate: il commento piu' pericoloso non e' quello
 * assente, e' quello che rassicura sulla meta' sbagliata.
 *
 * ⚠️ Il test tiene INSIEME le due proprieta', perche' separate si contraddicono senza che si veda:
 * la via additiva accumula fra gesti diversi, e `BakeCell` continua a NON accumulare fra rebake. Sono due
 * contratti diversi, e il difetto e' nato dall'averne usato uno al posto dell'altro.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAddSegmentsKeepsPreviousWallsTest,
	"RefactorTactics.Geometry.AddSegmentsKeepsPreviousWalls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAddSegmentsKeepsPreviousWallsTest::RunTest(const FString&)
{
	constexpr float HexSize = 100.f;

	// Due lati ADIACENTI, quindi con un vertice in comune: e' il caso che l'autore ha segnalato.
	auto WallOnDirection = [](ERTHexDirection Dir, ERTHexCoverType Type) -> FRTGeometrySegment
	{
		const FRTCellId Origin{ 0, 0, 0 };
		const FVector Here = URTHexLibrary::AxialToWorld(Origin, FVector::ZeroVector, HexSize, 0.f);
		const FVector There = URTHexLibrary::AxialToWorld(
			URTHexLibrary::Neighbor(Origin, Dir), FVector::ZeroVector, HexSize, 0.f);
		const double Mid = FMath::Atan2(There.Y - Here.Y, There.X - Here.X);
		const double Deg30 = PI / 6.0;

		FRTGeometrySegment Out;
		URTGeometryGrammarLibrary::SnapToGrammar(
			FVector2D(HexSize * FMath::Cos(Mid - Deg30), HexSize * FMath::Sin(Mid - Deg30)),
			FVector2D(HexSize * FMath::Cos(Mid + Deg30), HexSize * FMath::Sin(Mid + Deg30)),
			HexSize, Out);
		Out.WallType = Type;
		return Out;
	};

	const FRTGeometrySegment First = WallOnDirection(ERTHexDirection::NE, ERTHexCoverType::High);
	const FRTGeometrySegment Second = WallOnDirection(ERTHexDirection::NW, ERTHexCoverType::High);

	// --- la via ADDITIVA: due gesti, due muri -----------------------------------------------------------
	{
		URTHexMapAsset* Map = MakeOneCellMap();
		URTGeometryBakeLibrary::AddSegmentsToCell(Map, BakeOrigin, { First }, HexSize);
		TestNotNull(TEXT("dopo il primo gesto il muro NE c'e'"), FindCover(Map, ERTHexDirection::NE));

		URTGeometryBakeLibrary::AddSegmentsToCell(Map, BakeOrigin, { Second }, HexSize);
		TestNotNull(TEXT("dopo il secondo gesto il muro NW c'e'"), FindCover(Map, ERTHexDirection::NW));
		// La riga che il difetto faceva fallire.
		TestNotNull(TEXT("e il muro NE del primo gesto e' ANCORA li'"), FindCover(Map, ERTHexDirection::NE));
		TestEqual(TEXT("due coperture generate, non una"),
			URTGeometryBakeLibrary::CountGeneratedCovers(Map, BakeOrigin), 2);

		// Ripassare lo stesso segmento non accumula: un bordo ha al massimo una copertura.
		URTGeometryBakeLibrary::AddSegmentsToCell(Map, BakeOrigin, { First }, HexSize);
		TestEqual(TEXT("ripassare sopra non aggiunge una terza"),
			URTGeometryBakeLibrary::CountGeneratedCovers(Map, BakeOrigin), 2);
	}

	// --- e `BakeCell` NON cambia: il suo contratto di rebake regge ancora --------------------------------
	{
		URTHexMapAsset* Map = MakeOneCellMap();
		URTGeometryBakeLibrary::BakeCell(Map, BakeOrigin, { First }, HexSize);
		URTGeometryBakeLibrary::BakeCell(Map, BakeOrigin, { Second }, HexSize);
		TestNull(TEXT("il rebake sostituisce: il muro NE non c'e' piu'"), FindCover(Map, ERTHexDirection::NE));
		TestEqual(TEXT("una sola copertura generata dopo il rebake"),
			URTGeometryBakeLibrary::CountGeneratedCovers(Map, BakeOrigin), 1);
	}

	return true;
}

/**
 * #712 / seduta `U22`: un muro che ARRIVA su un bordo lo chiude, anche se si ferma li'.
 *
 * 🔴 Prima no. `SegmentClosesEdge` pretendeva un attraversamento **proprio** — segni strettamente opposti —
 * e un estremo appoggiato sul bordo e' un contatto, non una traversata. Conseguenza misurata: un muro dal
 * centro a un lato non muraglia nulla, e nemmeno un diametro da un lato a quello opposto. Funzionava solo
 * **sbordando**: lo stesso muro lungo `1.5x` l'inraggio dava due coperture, fermato sul bordo ne dava zero.
 *
 * ⚠️ Il test include i due casi che devono restare a ZERO, e non sono dimenticanze:
 * il vertice e' condiviso da due lati (`MSE-4`), quindi un estremo li' non chiude ne' l'uno ne' l'altro; e
 * un diametro fra vertici opposti tocca il perimetro **solo** sui vertici. Quest'ultimo e' un limite del
 * MODELLO — una copertura vive su un bordo, e quella linea non giace su nessun bordo — non una scelta di
 * questa regola. Asserirlo qui impedisce che qualcuno lo "aggiusti" reintroducendo l'ambiguita'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWallReachingAnEdgeClosesItTest,
	"RefactorTactics.Geometry.WallReachingAnEdgeClosesIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWallReachingAnEdgeClosesItTest::RunTest(const FString&)
{
	constexpr float HexSize = 100.f;
	const double Deg30 = PI / 6.0;

	// La giacitura del lato condiviso col vicino `Dir`, derivata dal mondo e non da una tabella.
	auto EdgeMidAngle = [](ERTHexDirection Dir)
	{
		const FRTCellId Origin{ 0, 0, 0 };
		const FVector Here = URTHexLibrary::AxialToWorld(Origin, FVector::ZeroVector, HexSize, 0.f);
		const FVector There = URTHexLibrary::AxialToWorld(
			URTHexLibrary::Neighbor(Origin, Dir), FVector::ZeroVector, HexSize, 0.f);
		return FMath::Atan2(There.Y - Here.Y, There.X - Here.X);
	};

	auto EdgesFor = [](const FVector2D& A, const FVector2D& B, TArray<ERTHexDirection>& Out)
	{
		FRTGeometrySegment Seg;
		Out.Reset();
		if (URTGeometryGrammarLibrary::SnapToGrammar(A, B, HexSize, Seg))
		{
			URTGeometryBakeLibrary::EdgesTouchedBy(Seg, HexSize, Out);
		}
	};

	const double InRadius = HexSize * FMath::Cos(Deg30);
	const FVector2D Centre(0.0, 0.0);
	TArray<ERTHexDirection> Edges;

	for (int32 DirIndex = 0; DirIndex < 6; ++DirIndex)
	{
		const ERTHexDirection Dir = static_cast<ERTHexDirection>(DirIndex);
		const double Mid = EdgeMidAngle(Dir);

		// 1. Centro -> punto medio del lato: una copertura, su QUEL lato.
		const FVector2D EdgeMid(InRadius * FMath::Cos(Mid), InRadius * FMath::Sin(Mid));
		EdgesFor(Centre, EdgeMid, Edges);
		TestEqual(FString::Printf(TEXT("centro -> lato %d: una copertura"), DirIndex), Edges.Num(), 1);
		if (Edges.Num() == 1)
		{
			TestEqual(FString::Printf(TEXT("centro -> lato %d: e' quel lato"), DirIndex),
				static_cast<int32>(Edges[0]), DirIndex);
		}

		// 2. Centro -> vertice: nessuna COPERTURA, perche' il vertice appartiene a due lati (`MSE-4`).
		//    ⚠️ Non vuol dire «niente»: dal formato v10 quel segmento diventa un muro interno, e lo
		//    asserisce `InteriorWallIsKeptAndHashes`. Qui si misura solo che non chiude bordi.
		const FVector2D Vertex(HexSize * FMath::Cos(Mid + Deg30), HexSize * FMath::Sin(Mid + Deg30));
		EdgesFor(Centre, Vertex, Edges);
		TestEqual(FString::Printf(TEXT("centro -> vertice presso %d: nessuna copertura"), DirIndex),
			Edges.Num(), 0);
	}

	// 3. Diametro fra due lati OPPOSTI: due coperture, e sono i due lati opposti.
	for (int32 DirIndex = 0; DirIndex < 3; ++DirIndex)
	{
		const ERTHexDirection Dir = static_cast<ERTHexDirection>(DirIndex);
		const double Mid = EdgeMidAngle(Dir);
		const FVector2D A(InRadius * FMath::Cos(Mid), InRadius * FMath::Sin(Mid));
		const FVector2D B(-A.X, -A.Y);

		EdgesFor(A, B, Edges);
		TestEqual(FString::Printf(TEXT("diametro lato %d: due coperture"), DirIndex), Edges.Num(), 2);
		if (Edges.Num() == 2)
		{
			TestTrue(FString::Printf(TEXT("diametro lato %d: include il lato tracciato"), DirIndex),
				Edges.Contains(Dir));
			TestTrue(FString::Printf(TEXT("diametro lato %d: include l'opposto"), DirIndex),
				Edges.Contains(URTHexLibrary::OppositeDirection(Dir)));
		}
	}

	// 4. Diametro fra due VERTICI opposti: niente, ed e' un limite del modello.
	for (int32 K = 0; K < 3; ++K)
	{
		const double Angle = Deg30 + K * (PI / 3.0);
		const FVector2D A(HexSize * FMath::Cos(Angle), HexSize * FMath::Sin(Angle));
		const FVector2D B(-A.X, -A.Y);

		EdgesFor(A, B, Edges);
		TestEqual(TEXT("diametro fra vertici opposti: nessuna copertura esprimibile"), Edges.Num(), 0);
	}

	return true;
}

/**
 * #712 / seduta `U22`: un muro che non giace su nessun bordo viene CONSERVATO, non buttato via.
 *
 * 🔴 Prima spariva in silenzio. Un segmento che non chiude bordi non produce coperture, e non c'era nessun
 * altro posto dove metterlo: veniva calcolato, disegnato come anteprima, e perso al rilascio. Il caso non
 * e' esotico — e' quello che l'autore ha chiesto: una retta che taglia l'esagono passando per due vertici
 * opposti. Tracciata sulla griglia attraversa **una cella su tre** per il centro.
 *
 * ⚠️ Il test asserisce anche cio' che NON deve finirci: un segmento che chiude un bordo e' gia' descritto
 * dalla sua copertura, e scriverlo anche fra i muri interni sarebbe una seconda verita' sullo stesso muro.
 *
 * 🔄 **E asseriva che l'hash NON cambia. Dal 2026-09-01 (`#1830`) asserisce l'opposto, e il ribaltamento e'
 * la conseguenza di una decisione, non un ripensamento.** La ragione di allora era vera di allora: *«il
 * movimento e' cella-a-cella, un muro dentro una cella non ne blocca nessuno»*. Ora ne blocca — `D-269` ha
 * reso la geometria intra-cella autorevole per vista e proiettili, e `URTHexOcclusionLibrary` la legge.
 *
 * Il criterio dell'hash non e' cambiato: ci entra cio' che puo' cambiare un ESITO. E' cambiato il fatto, e
 * con esso il verso di questa asserzione — lasciarla come stava avrebbe pinnato un falso negativo contro
 * `replay divergence = 0`, che e' la meta' peggiore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteriorWallIsKeptAndHashesTest,
	"RefactorTactics.Geometry.InteriorWallIsKeptAndHashes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInteriorWallIsKeptAndHashesTest::RunTest(const FString&)
{
	constexpr float HexSize = 100.f;
	const double Deg30 = PI / 6.0;

	// La corda per due vertici opposti: quella che l'autore voleva disegnare e che nessuna copertura regge.
	FRTGeometrySegment Chord;
	{
		const FVector2D A(HexSize * FMath::Cos(Deg30), HexSize * FMath::Sin(Deg30));
		const FVector2D B(-A.X, -A.Y);
		if (!TestTrue(TEXT("la corda si aggancia alla grammatica"),
			URTGeometryGrammarLibrary::SnapToGrammar(A, B, HexSize, Chord)))
		{
			return false;
		}
		Chord.WallType = ERTHexCoverType::High;

		TArray<ERTHexDirection> Edges;
		URTGeometryBakeLibrary::EdgesTouchedBy(Chord, HexSize, Edges);
		TestEqual(TEXT("e non chiude nessun bordo: e' il motivo per cui serve un posto nuovo"),
			Edges.Num(), 0);
	}

	URTHexMapAsset* Map = MakeOneCellMap();
	const uint32 HashBefore = Map->ComputeHash();

	URTGeometryBakeLibrary::AddSegmentsToCell(Map, BakeOrigin, { Chord }, HexSize);

	TestEqual(TEXT("il muro interno e' conservato"), Map->InteriorWalls.Num(), 1);
	TestEqual(TEXT("nessuna copertura, perche' non chiude bordi"),
		URTGeometryBakeLibrary::CountGeneratedCovers(Map, BakeOrigin), 0);
	if (Map->InteriorWalls.Num() == 1)
	{
		TestTrue(TEXT("appartiene alla cella disegnata"), Map->InteriorWalls[0].Cell == BakeOrigin);
	}

	// Ripassare sopra non duplica: un muro identico e' lo stesso muro.
	URTGeometryBakeLibrary::AddSegmentsToCell(Map, BakeOrigin, { Chord }, HexSize);
	TestEqual(TEXT("ridisegnarlo non lo duplica"), Map->InteriorWalls.Num(), 1);

	// L'hash CAMBIA: da `#1830` il muro interno e' dato di gioco, perche' ferma vista e proiettili.
	TestNotEqual(TEXT("l'hash della mappa cambia"), Map->ComputeHash(), HashBefore);

	// Un muro SU un bordo non finisce fra gli interni: e' gia' descritto dalla sua copertura.
	{
		const FRTCellId Origin{ 0, 0, 0 };
		const FVector Here = URTHexLibrary::AxialToWorld(Origin, FVector::ZeroVector, HexSize, 0.f);
		const FVector There = URTHexLibrary::AxialToWorld(
			URTHexLibrary::Neighbor(Origin, ERTHexDirection::NE), FVector::ZeroVector, HexSize, 0.f);
		const double Mid = FMath::Atan2(There.Y - Here.Y, There.X - Here.X);

		FRTGeometrySegment OnEdge;
		URTGeometryGrammarLibrary::SnapToGrammar(
			FVector2D(HexSize * FMath::Cos(Mid - Deg30), HexSize * FMath::Sin(Mid - Deg30)),
			FVector2D(HexSize * FMath::Cos(Mid + Deg30), HexSize * FMath::Sin(Mid + Deg30)),
			HexSize, OnEdge);

		URTHexMapAsset* Other = MakeOneCellMap();
		URTGeometryBakeLibrary::AddSegmentsToCell(Other, BakeOrigin, { OnEdge }, HexSize);
		TestEqual(TEXT("un muro su un bordo non entra fra gli interni"), Other->InteriorWalls.Num(), 0);
		TestNotNull(TEXT("ed e' descritto dalla sua copertura"), FindCover(Other, ERTHexDirection::NE));
	}

	// Il rebake li azzera insieme alle coperture generate: sono l'altra meta' dello stesso prodotto.
	URTGeometryBakeLibrary::BakeCell(Map, BakeOrigin, {}, HexSize);
	TestEqual(TEXT("il rebake a vuoto toglie anche i muri interni"), Map->InteriorWalls.Num(), 0);

	return true;
}

/**
 * #712 / seduta `U22`: le quattro reti che al muro interno mancavano, trovate da una code review.
 *
 * 🔴 Il campo `InteriorWalls` (v10) era arrivato senza le difese che ogni altro array d'autore ha. Nessuna
 * produceva divergenza di gioco — non entra nell'hash — ma tutte e quattro erano **lo stesso difetto che
 * questa PR dichiara di combattere**: una convenzione che esiste da un'altra parte e non viene chiamata.
 *
 * ⚠️ La prima e' la piu' istruttiva. Il dedup confrontava i campi del segmento **a mano**, uno per uno,
 * mentre `FRTGeometrySegment::operator==` esiste, usa `Min`/`Max` sugli estremi e porta scritto perche':
 * *«un segmento e' lo STESSO segmento anche percorso al contrario»*. Tracciando `V1→V2` e poi `V2→V1` il
 * duplicato passava. Riscrivere a mano un confronto che c'e' gia' e' come nascono i difetti di questa
 * seduta — e stavolta l'ho fatto io mentre li correggevo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteriorWallHygieneTest,
	"RefactorTactics.Geometry.InteriorWallHygiene",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInteriorWallHygieneTest::RunTest(const FString&)
{
	constexpr float HexSize = 100.f;
	const double Deg30 = PI / 6.0;

	// La corda per due vertici opposti: non chiude bordi, quindi e' un muro interno.
	const FVector2D A(HexSize * FMath::Cos(Deg30), HexSize * FMath::Sin(Deg30));
	const FVector2D B(-A.X, -A.Y);

	FRTGeometrySegment Forward;
	FRTGeometrySegment Backward;
	if (!TestTrue(TEXT("la corda si aggancia in entrambi i versi"),
		URTGeometryGrammarLibrary::SnapToGrammar(A, B, HexSize, Forward)
		&& URTGeometryGrammarLibrary::SnapToGrammar(B, A, HexSize, Backward)))
	{
		return false;
	}
	Forward.WallType = ERTHexCoverType::High;
	Backward.WallType = ERTHexCoverType::High;

	// I due sono lo STESSO muro percorso al contrario: gli estremi sono scambiati, e se non lo fossero il
	// test non proverebbe niente.
	TestTrue(TEXT("i due versi hanno gli estremi scambiati"),
		Forward.AlongStart == Backward.AlongEnd && Forward.AlongEnd == Backward.AlongStart);

	// --- 1. il muro percorso al contrario NON e' un secondo muro -----------------------------------
	{
		URTHexMapAsset* Map = MakeOneCellMap();
		URTGeometryBakeLibrary::AddSegmentsToCell(Map, BakeOrigin, { Forward }, HexSize);
		URTGeometryBakeLibrary::AddSegmentsToCell(Map, BakeOrigin, { Backward }, HexSize);
		TestEqual(TEXT("lo stesso muro al contrario non si duplica"), Map->InteriorWalls.Num(), 1);
	}

	// --- 2. un segmento fuori grammatica non viene scritto ------------------------------------------
	// ⚠️ `EdgesTouchedBy` esce con l'elenco vuoto ANCHE su un segmento illegale, quindi senza la
	//    rivalidazione i due casi finivano nello stesso ramo.
	{
		URTHexMapAsset* Map = MakeOneCellMap();
		FRTGeometrySegment Broken = Forward;
		Broken.AlongEnd = Broken.AlongStart; // lunghezza zero: `ZeroLength`
		TestTrue(TEXT("il segmento di prova e' davvero illegale"),
			URTGeometryGrammarLibrary::ValidateSegment(Broken) != ERTGeometryViolation::None);

		URTGeometryBakeLibrary::AddSegmentsToCell(Map, BakeOrigin, { Broken }, HexSize);
		TestEqual(TEXT("un segmento fuori grammatica non diventa un muro interno"),
			Map->InteriorWalls.Num(), 0);
	}

	// --- 3. `ValidateMap` vede i quattro modi in cui un muro interno puo' essere sbagliato ----------
	{
		URTHexMapAsset* Map = MakeOneCellMap();
		Map->HexSize = HexSize;

		// (a) sano: nessun errore. Senza questa controprova il test passerebbe con un validator che
		//     segnala sempre.
		Map->InteriorWalls.Add(FRTHexInteriorWall(BakeOrigin, Forward));
		TestEqual(TEXT("un muro interno sano non produce errori"), Map->ValidateMap().Num(), 0);

		// (b) orfano: cella inesistente.
		Map->InteriorWalls.Add(FRTHexInteriorWall(FRTCellId(9, 9, 0), Forward));
		TestTrue(TEXT("un muro interno su cella inesistente e' segnalato"), Map->ValidateMap().Num() > 0);
		Map->InteriorWalls.Pop();

		// (c) duplicato, e per giunta al contrario: e' il caso che il dedup a mano lasciava passare.
		Map->InteriorWalls.Add(FRTHexInteriorWall(BakeOrigin, Backward));
		TestTrue(TEXT("un duplicato al contrario e' segnalato"), Map->ValidateMap().Num() > 0);
		Map->InteriorWalls.Pop();

		// (d) l'invariante: un segmento che chiude un bordo e' una COPERTURA, e qui non ci va.
		const FRTCellId Origin{ 0, 0, 0 };
		const FVector Here = URTHexLibrary::AxialToWorld(Origin, FVector::ZeroVector, HexSize, 0.f);
		const FVector There = URTHexLibrary::AxialToWorld(
			URTHexLibrary::Neighbor(Origin, ERTHexDirection::NE), FVector::ZeroVector, HexSize, 0.f);
		const double Mid = FMath::Atan2(There.Y - Here.Y, There.X - Here.X);

		FRTGeometrySegment OnEdge;
		URTGeometryGrammarLibrary::SnapToGrammar(
			FVector2D(HexSize * FMath::Cos(Mid - Deg30), HexSize * FMath::Sin(Mid - Deg30)),
			FVector2D(HexSize * FMath::Cos(Mid + Deg30), HexSize * FMath::Sin(Mid + Deg30)),
			HexSize, OnEdge);

		Map->InteriorWalls.Add(FRTHexInteriorWall(BakeOrigin, OnEdge));
		TestTrue(TEXT("un muro che chiude un bordo non puo' stare fra gli interni"),
			Map->ValidateMap().Num() > 0);
	}

	return true;
}


// ---------------------------------------------------------------------------------------------------------
// #2085 — un segmento che passa per il centro e' un MURO INTERNO, non una copertura di bordo
// ---------------------------------------------------------------------------------------------------------

namespace
{
	/** Il muro fra due anchor della stessa cella, o un segmento nullo se la coppia non si esprime. */
	FRTGeometrySegment WallBetween(const FRTCellId& Cell, ERTAnchorKind KindA, int32 IndexA,
		ERTAnchorKind KindB, int32 IndexB, float HexSize)
	{
		FRTGeometrySegment S;
		URTGeometryGrammarLibrary::SegmentBetweenAnchors(
			FRTAnchorRef(Cell, KindA, IndexA), FRTAnchorRef(Cell, KindB, IndexB), HexSize, S);
		return S;
	}

	/** Una mappa di una cella sola, per far girare il bake vero invece di un predicato. */
	URTHexMapAsset* SingleCellMap(const FRTCellId& Cell, float HexSize)
	{
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>(GetTransientPackage());
		Map->HexSize = HexSize;
		FRTHexCellData Data(Cell);
		Map->Cells.Add(Data);
		return Map;
	}
}

/**
 * I DODICI RAGGI sono muri interni, e non solo i sei che partivano dai vertici.
 *
 * 🔴 **Il difetto che questo test coglie, isolato in seduta PIE il 2026-09-02** partendo da una domanda
 * d'autore: *«come disegno un muro dal centro a meta' di un lato?»*. Non si poteva: quel gesto produceva
 * una **copertura di bordo**, mentre lo stesso gesto verso un **vertice** produceva un muro interno.
 * Stessa grammatica, stesso `Offset = 0`, due esiti.
 *
 * 🔑 La causa non era una tolleranza ne' un errore di calcolo — misurato: `d4 = 0.000e+00` esatto. Era il
 * ramo *«il muro arriva da dentro e si ferma sul bordo»*, scritto in `362e42c1` (#712) **prima** che
 * `InteriorWalls` esistesse, quando trasformare il raggio in copertura era l'unico modo di farlo esistere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGeometryBakeRadiusIsInteriorTest,
	"RefactorTactics.GeometryBake.RadiusBecomesInteriorWallNotEdgeCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGeometryBakeRadiusIsInteriorTest::RunTest(const FString&)
{
	const FRTCellId Cell(0, 0, 0);
	const float HexSize = 150.f;

	for (int32 Index = 0; Index < 6; ++Index)
	{
		for (const ERTAnchorKind Kind : { ERTAnchorKind::EdgeMid, ERTAnchorKind::Vertex })
		{
			const FRTGeometrySegment Radius =
				WallBetween(Cell, ERTAnchorKind::Center, 0, Kind, Index, HexSize);

			const TCHAR* KindName = (Kind == ERTAnchorKind::EdgeMid) ? TEXT("EdgeMid") : TEXT("Vertex");

			TestEqual(FString::Printf(TEXT("il raggio verso %s%d passa per il centro"), KindName, Index),
				Radius.Offset, 0);

			URTHexMapAsset* Map = SingleCellMap(Cell, HexSize);
			const int32 Baked = URTGeometryBakeLibrary::AddSegmentsToCell(Map, Cell, { Radius }, HexSize);

			// ⚠️ Il valore di ritorno conta le COPERTURE, non i muri: la doc di `AddSegmentsToCell` dice
			// *«restituisce quante coperture ha aggiunto questo gesto»*. Per un muro interno vale ZERO, ed
			// e' esattamente cio' che si vuole asserire — e la ragione per cui il pannello del tool, che
			// mostra `LastBakedCovers`, diceva `1` prima di questa correzione.
			TestEqual(FString::Printf(TEXT("il raggio verso %s%d non aggiunge coperture"), KindName, Index),
				Baked, 0);
			TestEqual(FString::Printf(TEXT("il raggio verso %s%d e' un MURO INTERNO"), KindName, Index),
				Map->InteriorWalls.Num(), 1);

			const FRTHexCellData* Data = Map->FindCell(Cell);
			TestEqual(FString::Printf(TEXT("e non una copertura di bordo (verso %s%d)"), KindName, Index),
				Data ? Data->Covers.Num() : -1, 0);
		}
	}

	return true;
}

/**
 * IL DIAMETRO FRA PUNTI MEDI arriva davvero al modello di posa, passando dal bake.
 *
 * 🔑 **E' la prova end-to-end che il panel ha chiesto, e senza cui la correzione non serve a niente.**
 * `CoverPlacement.ContinuousWallSeparatesSidesAndRejectsTransition` verifica che quel diametro produca due
 * regioni e due facce — ma costruisce il segmento **a mano**. Finche' il bake lo trasformava in due
 * coperture, nessuna mappa autorata poteva produrlo: il consumatore era verde e il produttore assente.
 * E' il difetto che `D-304` ha nominato per il GrayKit, rovesciato.
 *
 * La catena percorsa qui e' quella vera: bake → `InteriorWalls` → `ComputeMask` → `ComputeFreeRegions`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGeometryBakeDiameterFeedsPlacementTest,
	"RefactorTactics.GeometryBake.EdgeMidDiameterFeedsThePlacementModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGeometryBakeDiameterFeedsPlacementTest::RunTest(const FString&)
{
	const FRTCellId Cell(0, 0, 0);
	const float HexSize = 150.f;

	// Il diametro fra due punti medi opposti: e' `Diameter(Deg0)` di `RTHexCoverPlacementTests`.
	const FRTGeometrySegment Diameter =
		WallBetween(Cell, ERTAnchorKind::EdgeMid, 0, ERTAnchorKind::EdgeMid, 3, HexSize);
	TestEqual(TEXT("il diametro passa per il centro"), Diameter.Offset, 0);

	URTHexMapAsset* Map = SingleCellMap(Cell, HexSize);
	URTGeometryBakeLibrary::AddSegmentsToCell(Map, Cell, { Diameter }, HexSize);

	TestEqual(TEXT("il bake lo scrive come muro interno"), Map->InteriorWalls.Num(), 1);

	// E ora la catena fino al modello di posa, che e' cio' che rende la correzione utile.
	TArray<FRTOccupancyPolyline> Geometry;
	for (const FRTHexInteriorWall& Wall : Map->InteriorWalls)
	{
		Geometry.Add(URTGeometryGrammarLibrary::ToPolyline(Wall.Segment, HexSize));
	}
	const FRTOccupancyMask Mask = URTHexOccupancyLibrary::ComputeMask(Geometry, HexSize);

	TArray<FRTPlacementRegion> Regions;
	URTHexCoverPlacementLibrary::ComputeFreeRegions(Mask, Regions);

	// Il muro continuo divide lo spazio di posa in DUE: e' `spec-cover-placement-intra-hex.md` §5.3.
	TestEqual(TEXT("il modello di posa vede due regioni, una per semipiano"), Regions.Num(), 2);

	return true;
}

/**
 * CONTROPROVA: un muro che GIACE sul lato lo chiude ancora.
 *
 * ⚠️ **Senza questa asserzione la correzione facile e sbagliata passerebbe**: svuotare `EdgesTouchedBy`,
 * o classificare interno qualunque cosa, renderebbe verdi i due test qui sopra e romperebbe le coperture.
 * Un muro da vertice a vertice adiacente ha `Offset` massimo — non zero — e non passa dal ramo nuovo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGeometryBakeEdgeWallStillClosesTest,
	"RefactorTactics.GeometryBake.WallLyingOnTheEdgeStillClosesIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGeometryBakeEdgeWallStillClosesTest::RunTest(const FString&)
{
	const FRTCellId Cell(0, 0, 0);
	const float HexSize = 150.f;

	const FRTGeometrySegment OnTheEdge =
		WallBetween(Cell, ERTAnchorKind::Vertex, 0, ERTAnchorKind::Vertex, 1, HexSize);

	TestNotEqual(TEXT("un muro sul lato NON passa per il centro"), OnTheEdge.Offset, 0);

	URTHexMapAsset* Map = SingleCellMap(Cell, HexSize);
	URTGeometryBakeLibrary::AddSegmentsToCell(Map, Cell, { OnTheEdge }, HexSize);

	const FRTHexCellData* Data = Map->FindCell(Cell);
	TestTrue(TEXT("produce una copertura di bordo"), Data && Data->Covers.Num() >= 1);
	TestEqual(TEXT("e nessun muro interno"), Map->InteriorWalls.Num(), 0);

	return true;
}

// ══════════════════════════════════════════════════════════════════════════════════════════════════
//  REGIONI NO-WALK (#1868, D-439)
// ══════════════════════════════════════════════════════════════════════════════════════════════════

namespace
{
	/** Una colonna di celle su due piani, piu' la regione che si vuole. `HexSize` fisso e dichiarato. */
	constexpr float NoWalkHexSize = 100.f;

	URTHexMapAsset* NoWalkMap()
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		M->HexSize = NoWalkHexSize;
		for (int32 Q = -2; Q <= 2; ++Q)
		{
			for (int32 R = -2; R <= 2; ++R)
			{
				M->AddOrUpdateCell(FRTHexCellData(FRTCellId(Q, R, 0)));
				M->AddOrUpdateCell(FRTHexCellData(FRTCellId(Q, R, 1)));
			}
		}
		M->SortCells();
		return M;
	}

	/** Un triangolo largo attorno all'origine, sul layer richiesto: i vertici sono CENTRI di cella. */
	FRTNoWalkArea NoWalkTriangolo(int32 Layer, FName Id = TEXT("Area"))
	{
		FRTNoWalkArea Area;
		Area.Layer = Layer;
		Area.StableId = Id;
		for (const FRTCellId& C : { FRTCellId(-2, 2, Layer), FRTCellId(2, 0, Layer), FRTCellId(0, -2, Layer) })
		{
			FRTAnchorRef Ref;
			Ref.Cell = C;
			Ref.Kind = ERTAnchorKind::Center;
			Ref.Index = 0;
			Area.Vertices.Add(Ref);
		}
		return Area;
	}
}

/**
 * UNA REGIONE CHIUDE IL PROPRIO PIANO, E SOLO QUELLO (#1868) — il difetto che la code review ha impedito.
 *
 * 🔴 **Senza il confronto del layer questa regola sarebbe silenziosamente sbagliata.** `AxialToWorld` mette
 * il piano interamente nella `Z` — `Wx` e `Wy` dipendono solo da `q` e `r` — quindi il test di
 * appartenenza 2D **non distingue i piani**: una regione al piano terra chiuderebbe le celle impilate
 * sopra. ⚠️ Non e' un caso di laboratorio: l'arena committata ha tre celle sul layer 1 sopra celle del
 * layer 0, e una di quelle e' l'unica destinazione della sua sola transizione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNoWalkRespectsLayerTest,
	"RefactorTactics.GeometryBake.NoWalkAreaClosesItsOwnLayerOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNoWalkRespectsLayerTest::RunTest(const FString&)
{
	const FRTNoWalkArea Area = NoWalkTriangolo(/*Layer*/ 0);

	// La cella d'origine sta dentro il triangolo su ENTRAMBI i piani, geometricamente: le due differiscono
	// solo per il layer, che e' esattamente cio' che si sta verificando.
	TestTrue(TEXT("la cella al piano della regione e' coperta"),
		URTGeometryBakeLibrary::AreaCoversCell(Area, FRTCellId(0, 0, 0), NoWalkHexSize));
	TestFalse(TEXT("la cella IMPILATA SOPRA non lo e'"),
		URTGeometryBakeLibrary::AreaCoversCell(Area, FRTCellId(0, 0, 1), NoWalkHexSize));

	// ⛔ CONTROPROVA: la stessa regione dichiarata sul layer 1 copre quella sopra e non quella sotto.
	// Senza, una funzione che rispondesse sempre `false` sul layer 1 passerebbe l'asserzione qui sopra.
	const FRTNoWalkArea Sopra = NoWalkTriangolo(/*Layer*/ 1);
	TestTrue(TEXT("e una regione dichiarata sul piano di sopra copre quella di sopra"),
		URTGeometryBakeLibrary::AreaCoversCell(Sopra, FRTCellId(0, 0, 1), NoWalkHexSize));
	TestFalse(TEXT("e non quella di sotto"),
		URTGeometryBakeLibrary::AreaCoversCell(Sopra, FRTCellId(0, 0, 0), NoWalkHexSize));

	// Una cella fuori dal triangolo non e' coperta su nessun piano: senza, «copre tutto» passerebbe.
	TestFalse(TEXT("una cella lontana non e' coperta"),
		URTGeometryBakeLibrary::AreaCoversCell(Area, FRTCellId(-2, -2, 0), NoWalkHexSize));

	// ⚠️ **Sotto i tre vertici la risposta e' `false`, e questa asserzione pinna la PROPRIETA', non la
	// guardia.** Misurato con una mutazione: togliendo il controllo esplicito questa riga resta verde,
	// perche' `PointInPolygon` risponde gia' `false` su meno di tre punti. Vale la pena asserirla lo
	// stesso — e' il comportamento su cui la copertura fa affidamento — ma non si spacci per la prova di
	// una guardia che non e' osservabile.
	FRTNoWalkArea Degenere = Area;
	Degenere.Vertices.SetNum(2);
	TestFalse(TEXT("due vertici non sono un poligono"),
		URTGeometryBakeLibrary::AreaCoversCell(Degenere, FRTCellId(0, 0, 0), NoWalkHexSize));
	return true;
}

/**
 * LA COTTURA CONSULTA LA REGIONE, E UN RIBAKE ESTRANEO NON PERDE IL VETO (#1868, [D-439]).
 *
 * 🔑 **E' la proprieta' per cui `D-439` ha scelto di CONSULTARE invece di scrivere.** Un secondo produttore
 * di `bBlocksMovement` avrebbe dovuto condividere `bMovementBlockGenerated` — che e' **un bit** — col
 * produttore geometrico, e il ramo che libera la cella avrebbe cancellato il veto della regione al primo
 * ribake per una ragione estranea. Consultando, il ribake **richiede** la stessa risposta e richiude.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNoWalkSurvivesRebakeTest,
	"RefactorTactics.GeometryBake.NoWalkSurvivesAnUnrelatedRebake",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNoWalkSurvivesRebakeTest::RunTest(const FString&)
{
	URTHexMapAsset* M = NoWalkMap();
	const FRTCellId Coperta(0, 0, 0);

	// Prima: nessuna regione, nessuna geometria -> la cella e' calpestabile.
	TestTrue(TEXT("senza regione la cottura lascia la cella aperta"),
		URTGeometryBakeLibrary::RederiveStandability(M, Coperta, NoWalkHexSize));
	TestFalse(TEXT("e infatti non e' bloccata"), M->FindCell(Coperta)->bBlocksMovement);

	M->NoWalkAreas.Add(NoWalkTriangolo(0));
	TestTrue(TEXT("la cottura gira"), URTGeometryBakeLibrary::RederiveStandability(M, Coperta, NoWalkHexSize));
	TestTrue(TEXT("e la regione chiude la cella"), M->FindCell(Coperta)->bBlocksMovement);
	TestTrue(TEXT("come blocco DERIVATO, non d'autore"), M->FindCell(Coperta)->bMovementBlockGenerated);

	// 🔴 IL PUNTO: un secondo ribake, per una ragione che non c'entra con la regione, deve RICHIUDERE.
	// Con un secondo produttore che scrive, qui il veto sarebbe sparito.
	TestTrue(TEXT("un ribake estraneo gira"),
		URTGeometryBakeLibrary::RederiveStandability(M, Coperta, NoWalkHexSize));
	TestTrue(TEXT("e la cella resta chiusa: il veto non si perde"), M->FindCell(Coperta)->bBlocksMovement);

	// E cancellare la regione la libera DA SE', senza codice di ripristino.
	M->NoWalkAreas.Reset();
	TestTrue(TEXT("la cottura gira dopo la cancellazione"),
		URTGeometryBakeLibrary::RederiveStandability(M, Coperta, NoWalkHexSize));
	TestFalse(TEXT("e la cella torna calpestabile"), M->FindCell(Coperta)->bBlocksMovement);

	// ⛔ CONTROPROVA sull'autore: un blocco dipinto A MANO non si tocca, nemmeno quando la regione se ne va.
	FRTHexCellData Autorata = *M->FindCell(FRTCellId(1, 0, 0));
	Autorata.bBlocksMovement = true;
	Autorata.bMovementBlockGenerated = false;
	M->AddOrUpdateCell(Autorata);
	M->NoWalkAreas.Add(NoWalkTriangolo(0));
	URTGeometryBakeLibrary::RederiveStandability(M, FRTCellId(1, 0, 0), NoWalkHexSize);
	M->NoWalkAreas.Reset();
	URTGeometryBakeLibrary::RederiveStandability(M, FRTCellId(1, 0, 0), NoWalkHexSize);
	TestTrue(TEXT("il blocco dell'autore sopravvive alla regione che va e viene"),
		M->FindCell(FRTCellId(1, 0, 0))->bBlocksMovement);
	TestFalse(TEXT("e resta d'autore"), M->FindCell(FRTCellId(1, 0, 0))->bMovementBlockGenerated);
	return true;
}

/**
 * IL VALIDATOR E LA COTTURA MISURANO LA STESSA COSA — la sede unica (#1868, [D-439]).
 *
 * 🔴 **E' il difetto che la code review ha trovato PRIMA che fosse scritto.** Il predicato aveva due
 * stesure: `DeriveStandability` per cuocere, `ValidateMapDetailed` per giudicare, tenute insieme solo dal
 * commento sopra la seconda e da **nessun test**. Aggiungendo la consultazione alla sola cottura, una cella
 * coperta usciva chiusa e derivata mentre il validator — che guardava la sola geometria — la vedeva sana:
 * **REGOLA 4** sarebbe scattata su ogni cella coperta dicendo *«Ricuoci la mappa: il prossimo rebake lo
 * toglierebbe»*, e il rebake invece lo **rimette**. Un avviso falso, con un rimedio che non fa niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNoWalkOneSeatTest,
	"RefactorTactics.GeometryBake.ValidatorAndBakeAgreeOnACoveredCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNoWalkOneSeatTest::RunTest(const FString&)
{
	URTHexMapAsset* M = NoWalkMap();
	const FRTCellId Coperta(0, 0, 0);
	M->NoWalkAreas.Add(NoWalkTriangolo(0));
	URTGeometryBakeLibrary::RederiveStandability(M, Coperta, NoWalkHexSize);

	auto ContaSu = [&](ERTMapValidationReason Ragione, const FRTCellId& Cella)
	{
		TArray<FRTMapValidationIssue> Issues;
		M->ValidateMapDetailed(Issues);
		return Issues.FilterByPredicate([&](const FRTMapValidationIssue& I)
		{
			return I.Reason == Ragione && I.Cell == Cella;
		}).Num();
	};

	// 🔴 La cella e' chiusa e derivata; con due sedi il validator l'avrebbe dichiarata «stantia».
	TestTrue(TEXT("l'allestimento e' quello giusto: chiusa e derivata"),
		M->FindCell(Coperta)->bBlocksMovement && M->FindCell(Coperta)->bMovementBlockGenerated);
	TestEqual(TEXT("e REGOLA 4 NON scatta: la regione giustifica il blocco"),
		ContaSu(ERTMapValidationReason::StaleGeneratedBlock, Coperta), 0);

	// ⛔ CONTROPROVA: la stessa REGOLA 4 scatta ancora dove deve — un blocco derivato che NIENTE giustifica.
	// Senza, una modifica che la spegnesse del tutto passerebbe l'asserzione qui sopra.
	FRTHexCellData Stantia = *M->FindCell(FRTCellId(-2, -2, 0));
	Stantia.bBlocksMovement = true;
	Stantia.bMovementBlockGenerated = true;
	M->AddOrUpdateCell(Stantia);
	TestEqual(TEXT("ma scatta su un blocco derivato che nessuna regione e nessuna geometria giustifica"),
		ContaSu(ERTMapValidationReason::StaleGeneratedBlock, FRTCellId(-2, -2, 0)), 1);

	// ── E REGOLA 1 nomina la CAUSA ────────────────────────────────────────────────────────────────────
	// Una cella coperta ma NON ancora ricotta: il validator la segnala, e il messaggio deve parlare della
	// regione, non di una geometria che non c'e'.
	FRTHexCellData NonCotta = *M->FindCell(FRTCellId(1, 0, 0));
	NonCotta.bBlocksMovement = false;
	NonCotta.bMovementBlockGenerated = false;
	M->AddOrUpdateCell(NonCotta);

	TArray<FRTMapValidationIssue> Issues;
	M->ValidateMapDetailed(Issues);
	const FRTMapValidationIssue* Posa = Issues.FindByPredicate([](const FRTMapValidationIssue& I)
	{
		return I.Reason == ERTMapValidationReason::NoLegalPlacement && I.Cell == FRTCellId(1, 0, 0);
	});
	if (!TestNotNull(TEXT("una cella coperta e non ricotta e' segnalata"), Posa))
	{
		return false;
	}
	TestTrue(FString::Printf(TEXT("e il messaggio nomina la REGIONE, non la geometria: %s"), *Posa->Message),
		Posa->Message.Contains(TEXT("regione No-Walk")));
	TestFalse(TEXT("e non parla di settori da liberare"), Posa->Message.Contains(TEXT("libera un settore")));
	return true;
}

/**
 * UN VOLUME OCCUPA LA CELLA IN CUI STA, ED E' CONSULTATO COME LA REGIONE (#1866, [D-440]).
 *
 * 🔑 **Stessa giuntura, terza ragione.** Il volume entra in `WhyNotStandable` accanto alla geometria e alla
 * regione No-Walk, invece di scrivere `bBlocksMovement`: sarebbe stato un **terzo** produttore su un bit di
 * provenienza che ne regge uno, e il primo ribake estraneo della cella ne avrebbe cancellato l'effetto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBoxVolumeOccupiesTest,
	"RefactorTactics.GeometryBake.BoxVolumeOccupiesItsCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBoxVolumeOccupiesTest::RunTest(const FString&)
{
	URTHexMapAsset* M = NoWalkMap();
	const FRTCellId Occupata(0, 0, 0);
	const FRTCellId Libera(1, 0, 0);

	FRTBoxVolume Volume;
	Volume.Cell = Occupata;
	Volume.CoverLevel = ERTHexCoverType::High;
	Volume.StableId = TEXT("Pilastro");
	M->BoxVolumes.Add(Volume);

	TestTrue(TEXT("la cottura gira"), URTGeometryBakeLibrary::RederiveStandability(M, Occupata, NoWalkHexSize));
	TestTrue(TEXT("il volume chiude la cella in cui sta"), M->FindCell(Occupata)->bBlocksMovement);
	TestTrue(TEXT("come blocco derivato"), M->FindCell(Occupata)->bMovementBlockGenerated);

	// ⛔ CONTROPROVA: la cella accanto non e' toccata. Senza, «chiude tutto» passerebbe.
	URTGeometryBakeLibrary::RederiveStandability(M, Libera, NoWalkHexSize);
	TestFalse(TEXT("e non tocca la cella accanto"), M->FindCell(Libera)->bBlocksMovement);

	// ⚠️ **Il volume sta su UNA cella, e il layer fa parte dell'identita' della cella**: la stessa `q,r` su
	// un altro piano e' un'altra cella, e non e' occupata.
	URTGeometryBakeLibrary::RederiveStandability(M, FRTCellId(0, 0, 1), NoWalkHexSize);
	TestFalse(TEXT("ne' la cella impilata sopra"), M->FindCell(FRTCellId(0, 0, 1))->bBlocksMovement);

	// Un ribake estraneo RICHIUDE, e cancellare il volume libera da se': e' la proprieta' per cui la
	// consultazione e' stata scelta al posto della scrittura.
	TestTrue(TEXT("un ribake estraneo gira"),
		URTGeometryBakeLibrary::RederiveStandability(M, Occupata, NoWalkHexSize));
	TestTrue(TEXT("e il volume tiene la cella chiusa"), M->FindCell(Occupata)->bBlocksMovement);

	M->BoxVolumes.Reset();
	URTGeometryBakeLibrary::RederiveStandability(M, Occupata, NoWalkHexSize);
	TestFalse(TEXT("tolto il volume, la cella torna calpestabile"), M->FindCell(Occupata)->bBlocksMovement);
	return true;
}

/**
 * LE CINQUE PROPRIETA' SONO DICHIARATE, NON DEDOTTE (#1866) — e tre sono quelle che entrano in `v0.1`.
 *
 * 🔑 **`CoverLevel` fuori da `None`/`Low`/`High` non e' rappresentabile, e lo garantisce il TIPO.** E' un'AC
 * di #1866 soddisfatta riusando `ERTHexCoverType` (`D-271`) invece di dichiarare un enum proprio: un enum
 * nuovo sarebbe stata una seconda autorita' sul vocabolario della copertura.
 *
 * ⚠️ **E la prova che occupazione e copertura sono INDIPENDENTI**: un volume che occupa senza riparare, e
 * uno che dichiara copertura alta. La issue lo chiede come AC, ed e' una proprieta' del dato, non del
 * comportamento — ma e' proprio per questo che va asserita: nulla nel codice la impone.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBoxVolumePropertiesAreIndependentTest,
	"RefactorTactics.GeometryBake.BoxVolumePropertiesAreDeclaredNotDeduced",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBoxVolumePropertiesAreIndependentTest::RunTest(const FString&)
{
	URTHexMapAsset* M = NoWalkMap();

	// Occupa e NON ripara: il caso che la issue nomina per primo.
	FRTBoxVolume Nudo;
	Nudo.Cell = FRTCellId(0, 0, 0);
	Nudo.CoverLevel = ERTHexCoverType::None;
	M->BoxVolumes.Add(Nudo);

	// Ripara e dichiara di fermare la vista: le due restano campi distinti.
	FRTBoxVolume Pieno;
	Pieno.Cell = FRTCellId(1, 0, 0);
	Pieno.CoverLevel = ERTHexCoverType::High;
	Pieno.bBlocksLineOfSight = true;
	M->BoxVolumes.Add(Pieno);

	URTGeometryBakeLibrary::RederiveStandability(M, FRTCellId(0, 0, 0), NoWalkHexSize);
	URTGeometryBakeLibrary::RederiveStandability(M, FRTCellId(1, 0, 0), NoWalkHexSize);

	// 🔑 ENTRAMBI occupano: l'occupazione non dipende dalla copertura.
	TestTrue(TEXT("il volume senza copertura occupa comunque"),
		M->FindCell(FRTCellId(0, 0, 0))->bBlocksMovement);
	TestTrue(TEXT("e quello con copertura alta pure"),
		M->FindCell(FRTCellId(1, 0, 0))->bBlocksMovement);

	// E i due campi dichiarati restano quelli, attraverso il dato.
	TestEqual(TEXT("la copertura dichiarata è None dove è stata scritta None"),
		M->BoxVolumes[0].CoverLevel, ERTHexCoverType::None);
	TestEqual(TEXT("ed è High dove è stata scritta High"),
		M->BoxVolumes[1].CoverLevel, ERTHexCoverType::High);
	TestFalse(TEXT("la LOS non si deduce dalla copertura"), M->BoxVolumes[0].bBlocksLineOfSight);
	TestTrue(TEXT("e resta quella dichiarata"), M->BoxVolumes[1].bBlocksLineOfSight);

	// ⛔ **DICHIARATO**: `CoverLevel` e `bBlocksLineOfSight` sono dati d'authoring che NESSUNO consuma
	// ancora. Le loro giunture hanno un nome — la cottura delle coperture per il primo, i molti lettori di
	// `FRTHexCellData::bBlocksLineOfSight` per il secondo — e questa asserzione pinna il limite invece di
	// lasciar credere che il volume ripari gia'.
	const FRTHexCellData* Cella = M->FindCell(FRTCellId(1, 0, 0));
	TestEqual(TEXT("il volume NON genera ancora una copertura sulla cella"), Cella->Covers.Num(), 0);
	TestFalse(TEXT("ne' scrive bBlocksLineOfSight: la giuntura non c'e' ancora"),
		Cella->bBlocksLineOfSight);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
