#include "Misc/AutomationTest.h"
#include "Map/RTGeometryBake.h"
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

#endif // WITH_DEV_AUTOMATION_TESTS
