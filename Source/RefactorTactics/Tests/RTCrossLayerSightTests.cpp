#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Map/RTGeometryGrammar.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"
#include "Perception/RTPerceptionLibrary.h"
#include "Terrain/RTTerrainData.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * La linea di vista **fra due piani** (`D-511`, #3743). Decisione d'autore del 2026-10-13: «Linea vera in 3D».
 *
 * Quattro scelte, tutte sue, e ogni test qui sotto ne fissa una:
 *
 * - la linea va **da testa a testa** -> lo stesso esito nei due versi (`SameAnswerBothWays`);
 * - la testa sta a **sei decimi di piano** -> chi sta sul bordo si vede da una cella, chi sta una cella piu'
 *   indietro da tre, due piu' indietro da cinque (`EdgeHidesWhoStandsBehind`);
 * - contano i **muri del piano attraversato** (`ObstaclesCountOnTheLayerTheLinePasses`);
 * - vale per le **celle**, non solo per chi le occupa: quello lo misura `RTPerceptionTests.cpp`, attraverso
 *   `VisibleCells`.
 *
 * 🔑 Ogni asserto chiede la RAGIONE e la CELLA, non il solo booleano: «ferma» ha quattro cause, e un test che
 * si accontenta di `false` resta verde anche quando la linea si ferma per la causa sbagliata.
 */

namespace
{
	// Prefisso `Xls`: la unity build fonde i namespace anonimi (`#1530`).

	FString XlsName(ERTCrossLayerSightBlock Block)
	{
		return StaticEnum<ERTCrossLayerSightBlock>()->GetNameStringByValue(static_cast<int64>(Block));
	}

	FString XlsCell(const FRTCellId& Cell)
	{
		return FString::Printf(TEXT("(%d,%d,%d)"), Cell.X, Cell.Y, Cell.Layer);
	}

	/** La linea e' ferma PER QUESTA ragione e SU QUESTA cella, nei due versi. */
	bool XlsExpectStopped(FAutomationTestBase& Test, const TCHAR* What, const URTHexMapAsset* Map,
		const FRTCellId& A, const FRTCellId& B, ERTCrossLayerSightBlock Why, const FRTCellId& At)
	{
		bool bOk = true;
		for (int32 Verso = 0; Verso < 2; ++Verso)
		{
			const FRTCellId& From = (Verso == 0) ? A : B;
			const FRTCellId& To = (Verso == 0) ? B : A;
			const FRTCrossLayerSightResult R = URTPerceptionLibrary::DescribeSightAcrossLayers(Map, From, To);
			bOk &= Test.TestTrue(
				FString::Printf(TEXT("%s — %s -> %s: attesa %s su %s, trovata %s su %s"), What, *XlsCell(From),
					*XlsCell(To), *XlsName(Why), *XlsCell(At), *XlsName(R.Block), *XlsCell(R.BlockedAt)),
				R.Block == Why && R.BlockedAt == At);
			bOk &= Test.TestFalse(FString::Printf(TEXT("%s — il booleano dice lo stesso"), What),
				URTPerceptionLibrary::HasSightAcrossLayers(Map, From, To));
		}
		return bOk;
	}

	/** La linea passa, nei due versi. */
	bool XlsExpectClear(FAutomationTestBase& Test, const TCHAR* What, const URTHexMapAsset* Map,
		const FRTCellId& A, const FRTCellId& B)
	{
		bool bOk = true;
		for (int32 Verso = 0; Verso < 2; ++Verso)
		{
			const FRTCellId& From = (Verso == 0) ? A : B;
			const FRTCellId& To = (Verso == 0) ? B : A;
			const FRTCrossLayerSightResult R = URTPerceptionLibrary::DescribeSightAcrossLayers(Map, From, To);
			bOk &= Test.TestTrue(
				FString::Printf(TEXT("%s — %s -> %s: attesa libera, trovata %s su %s"), What, *XlsCell(From),
					*XlsCell(To), *XlsName(R.Block), *XlsCell(R.BlockedAt)),
				R.IsClear());
			bOk &= Test.TestTrue(FString::Printf(TEXT("%s — il booleano dice lo stesso"), What),
				URTPerceptionLibrary::HasSightAcrossLayers(Map, From, To));
		}
		return bOk;
	}

	/** Un piano terra pieno di raggio `Radius`, piu' le celle date sugli altri piani. */
	URTHexMapAsset* XlsGroundWith(int32 Radius, const TArray<FRTCellId>& Extra)
	{
		URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		for (const FRTCellId& Id : Extra)
		{
			Map->AddOrUpdateCell(FRTHexCellData(Id));
		}
		Map->SortCells();
		return Map;
	}

	void XlsBlockSight(URTHexMapAsset* Map, const FRTCellId& Id)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		Data.bBlocksLineOfSight = true;
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	/** Copertura ALTA sul bordo fra `From` e `To`, dichiarata da `From`. La direzione si chiede alla libreria. */
	bool XlsHighCover(URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To)
	{
		ERTHexDirection Edge = ERTHexDirection::E;
		const FRTHexCellData* Existing = Map->FindCell(From);
		if (Existing == nullptr || !URTHexCoverLibrary::EdgeDirection(From, To, Edge))
		{
			return false;
		}
		FRTHexCellData Data = *Existing;
		Data.Covers.Add(FRTHexCover(Edge, ERTHexCoverType::High, FRTHexCover::DefaultIntegrity(ERTHexCoverType::High)));
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
		return true;
	}

	/** Un muro interno ALTO sul diametro verticale della cella: taglia la corda `W -> E`, a `Offset` dal centro. */
	void XlsInteriorWall(URTHexMapAsset* Map, const FRTCellId& Cell, int32 Offset = 0)
	{
		FRTGeometrySegment S;
		S.Axis = ERTTacticalAxis::Deg90;
		S.Offset = Offset;
		S.AlongStart = -RT_GeometryQuanta / 2;
		S.AlongEnd = RT_GeometryQuanta / 2;
		S.Layer = Cell.Layer;
		S.WallType = ERTHexCoverType::High;
		Map->InteriorWalls.Add(FRTHexInteriorWall(Cell, S));
	}

	void XlsSetSurface(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexSurface Surface)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		Data.Surface = Surface;
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	/** Chi guarda da `Eye` verso `Facing` vede la cella `Target`? Vista 5, come il roster piu' lungo. */
	bool XlsSees(const URTHexMapAsset* Map, const FRTCellId& Eye, ERTHexDirection Facing, const FRTCellId& Target)
	{
		FRTPerceiver P;
		P.Cell = Eye;
		P.Facing = Facing;
		P.VisionRange = 5;
		return URTPerceptionLibrary::VisibleCells(Map, P).Contains(Target);
	}
}

/**
 * I casi della proposta, sulla piattaforma di `TestArena`: sono quelli che l'autore ha letto prima di scegliere.
 *
 * 🔑 **La premessa e' la regola di ieri**: per ognuna di queste coppie `HasLineOfSight` risponde ancora «libera».
 * E' cio' che dice che la differenza la fa la regola nuova, e che la linea di tiro non e' stata toccata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCrossLayerProposalCasesTest,
	"RefactorTactics.Vision.AcrossLayers.ProposalCasesOnTestArena",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCrossLayerProposalCasesTest::RunTest(const FString&)
{
	const URTHexMapAsset* Map = URTMatchSetupLibrary::MakeTestArena(GetTransientPackage());
	if (!TestNotNull(TEXT("premessa: TestArena esiste"), Map))
	{
		return false;
	}

	const FRTCellId Sotto(2, 0, 0);     // a terra, SOTTO la piattaforma: e' dove stava Aevik nella tavola S1
	const FRTCellId Scoperto(1, 0, 0);  // a terra, accanto alla piattaforma e allo scoperto
	const FRTCellId AltroLato(4, 0, 0); // a terra, dall'altro lato
	const FRTCellId Bordo(2, 0, 1);     // sulla piattaforma, la cella di bordo verso `Scoperto`
	const FRTCellId Dietro(3, 0, 1);    // sulla piattaforma, una cella piu' indietro

	for (const FRTCellId& Giu : { Sotto, Scoperto, AltroLato })
	{
		for (const FRTCellId& Su : { Bordo, Dietro })
		{
			if (!TestTrue(TEXT("premessa: la linea di TIRO fra le due celle e' libera, come ieri"),
				URTHexVisionLibrary::HasLineOfSight(Map, Giu, Su)))
			{
				return false;
			}
		}
	}

	XlsExpectStopped(*this, TEXT("sotto la piattaforma, chi sta sopra la testa"), Map, Sotto, Bordo,
		ERTCrossLayerSightBlock::Floor, Bordo);
	XlsExpectStopped(*this, TEXT("sotto la piattaforma, chi sta una cella piu' in la': il pavimento e' quello sopra la testa"),
		Map, Sotto, Dietro, ERTCrossLayerSightBlock::Floor, Bordo);
	XlsExpectClear(*this, TEXT("allo scoperto, chi sta sulla cella di bordo"), Map, Scoperto, Bordo);
	XlsExpectStopped(*this, TEXT("allo scoperto, chi sta una cella piu' indietro: il bordo lo copre"), Map, Scoperto,
		Dietro, ERTCrossLayerSightBlock::Floor, Bordo);
	XlsExpectClear(*this, TEXT("dall'altro lato, chi da quel lato e' sul bordo"), Map, AltroLato, Dietro);
	XlsExpectStopped(*this, TEXT("dall'altro lato, chi da quel lato sta piu' indietro"), Map, AltroLato, Bordo,
		ERTCrossLayerSightBlock::Floor, Dietro);

	// Il muro di `q = 0` sta a terra, e la linea da oltre il muro verso la piattaforma lo attraversa in basso.
	XlsExpectStopped(*this, TEXT("da oltre il muro di q=0"), Map, FRTCellId(-1, 0, 0), Bordo,
		ERTCrossLayerSightBlock::CellBlocker, FRTCellId(0, 0, 0));
	return true;
}

/**
 * **Chi sta sul bordo si vede da una cella, chi sta una cella piu' indietro da tre, due piu' indietro da
 * cinque.** E' la regola pratica che l'autore ha letto nell'opzione «0,6 del piano», ed e' cio' che fissa
 * l'altezza della testa: a 0,5 e a 0,72 queste distanze sono altre.
 *
 * Una riga dritta: la piattaforma occupa `x >= 0` al piano 1, chi guarda sta a terra a `x < 0`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCrossLayerEdgeHidesTest,
	"RefactorTactics.Vision.AcrossLayers.EdgeHidesWhoStandsBehind",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCrossLayerEdgeHidesTest::RunTest(const FString&)
{
	const int32 HeadNum = URTPerceptionLibrary::SightHeadHeightNum;
	const int32 HeadDen = URTPerceptionLibrary::SightHeadHeightDen;
	TestEqual(TEXT("la testa sta a TRE quinti di piano"), HeadNum, 3);
	TestEqual(TEXT("la testa sta a tre QUINTI di piano"), HeadDen, 5);

	TArray<FRTCellId> Platform;
	for (int32 X = 0; X <= 4; ++X)
	{
		Platform.Add(FRTCellId(X, 0, 1));
	}
	const URTHexMapAsset* Map = XlsGroundWith(8, Platform);
	const FRTCellId Edge(0, 0, 1);

	// Quante celle dietro il bordo -> la prima DISTANZA (fra chi guarda e chi e' guardato) da cui si vede.
	const int32 FirstSeenAt[] = { 1, 3, 5 };
	for (int32 Behind = 0; Behind < UE_ARRAY_COUNT(FirstSeenAt); ++Behind)
	{
		const FRTCellId Target(Behind, 0, 1);
		for (int32 Distance = Behind + 1; Distance <= Behind + 6; ++Distance)
		{
			const FRTCellId Viewer(Behind - Distance, 0, 0);
			const FString What = FString::Printf(TEXT("%d dietro il bordo, da %d celle"), Behind, Distance);
			if (Distance < FirstSeenAt[Behind])
			{
				// Ed e' sempre la cella di BORDO a coprire: la zona cieca e' sua, non di un pavimento qualunque.
				XlsExpectStopped(*this, *What, Map, Viewer, Target, ERTCrossLayerSightBlock::Floor, Edge);
			}
			else
			{
				XlsExpectClear(*this, *What, Map, Viewer, Target);
			}
		}
	}
	return true;
}

/** Dove la cella non c'e', c'e' un buco: la stessa linea che il pavimento ferma, dal buco passa. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCrossLayerHoleTest,
	"RefactorTactics.Vision.AcrossLayers.HoleInTheFloorLetsTheLineThrough",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCrossLayerHoleTest::RunTest(const FString&)
{
	const FRTCellId Viewer(0, 0, 0);
	const FRTCellId Target(2, 0, 1);
	const FRTCellId Crossed(1, 0, 1); // dove la linea raggiunge la quota del piano 1: a due quinti della distanza

	auto Ceiling = [](const FRTCellId* Hole)
	{
		TArray<FRTCellId> Upper;
		for (const FRTCellId& Flat : URTHexLibrary::HexArea(FRTCellId(0, 0, 1), 4))
		{
			if (Hole == nullptr || !(Flat == *Hole))
			{
				Upper.Add(Flat);
			}
		}
		return XlsGroundWith(4, Upper);
	};

	const URTHexMapAsset* Full = Ceiling(nullptr);
	XlsExpectStopped(*this, TEXT("sotto un soffitto pieno"), Full, Viewer, Target, ERTCrossLayerSightBlock::Floor, Crossed);

	const URTHexMapAsset* Holed = Ceiling(&Crossed);
	if (!TestNull(TEXT("premessa: il buco c'e'"), Holed->FindCell(Crossed))
		|| !TestNotNull(TEXT("premessa: sopra la testa di chi guarda il soffitto e' rimasto"), Holed->FindCell(FRTCellId(0, 0, 1))))
	{
		return false;
	}
	XlsExpectClear(*this, TEXT("dal buco la linea passa, anche con il soffitto sopra la testa"), Holed, Viewer, Target);

	// Un buco ALTROVE non serve: la linea buca il pavimento in un punto solo.
	const FRTCellId Elsewhere(1, -1, 1);
	const URTHexMapAsset* WrongHole = Ceiling(&Elsewhere);
	XlsExpectStopped(*this, TEXT("un buco accanto non apre la linea"), WrongHole, Viewer, Target,
		ERTCrossLayerSightBlock::Floor, Crossed);
	return true;
}

/**
 * Due piani di dislivello: la linea attraversa DUE quote, e ognuna ha il suo punto. Con la testa a tre quinti
 * sono a due decimi e a sette decimi della distanza, contando da chi sta sotto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCrossLayerTwoFloorsTest,
	"RefactorTactics.Vision.AcrossLayers.TwoLayersOfRiseCrossTwoFloors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCrossLayerTwoFloorsTest::RunTest(const FString&)
{
	const FRTCellId Viewer(0, 0, 0);
	const FRTCellId Target(4, 0, 2);
	const FRTCellId FirstFloor(1, 0, 1);  // 0,2 * 4 = 0,8 celle da chi guarda
	const FRTCellId SecondFloor(3, 0, 2); // 0,7 * 4 = 2,8 celle da chi guarda

	XlsExpectClear(*this, TEXT("niente in mezzo"), XlsGroundWith(6, { Target }), Viewer, Target);
	XlsExpectStopped(*this, TEXT("il pavimento del piano di mezzo"), XlsGroundWith(6, { Target, FirstFloor }), Viewer,
		Target, ERTCrossLayerSightBlock::Floor, FirstFloor);
	XlsExpectStopped(*this, TEXT("il pavimento del piano di chi e' guardato"), XlsGroundWith(6, { Target, SecondFloor }),
		Viewer, Target, ERTCrossLayerSightBlock::Floor, SecondFloor);

	// Un pavimento che la linea NON buca non la ferma: esiste, ma la linea a quella quota passa altrove.
	XlsExpectClear(*this, TEXT("pavimenti fuori dai due punti"),
		XlsGroundWith(6, { Target, FRTCellId(2, 0, 1), FRTCellId(3, 0, 1), FRTCellId(1, 0, 2), FRTCellId(2, 0, 2) }),
		Viewer, Target);

	// Il CONFINE dell'intervallo. Da (0,0,0) a (5,0,2) la linea lascia la cella 3 esattamente alla quota del
	// piano 2: quel piano lo tocca solo uscendone, quindi un muro del piano 2 su quella cella non la ferma.
	// Lo stesso muro al piano 1, dove la linea in quella cella ci sta davvero, si'.
	{
		const FRTCellId Far(5, 0, 2);
		const FRTCellId Touched(3, 0, 2);
		URTHexMapAsset* Map = XlsGroundWith(6, { Far, Touched, FRTCellId(3, 0, 1) });
		if (!XlsExpectClear(*this, TEXT("premessa: le due celle in piu' non sono pavimenti che la linea buca"), Map, Viewer, Far))
		{
			return false;
		}
		XlsBlockSight(Map, Touched);
		XlsExpectClear(*this, TEXT("muro al piano che la linea tocca solo uscendo dalla cella"), Map, Viewer, Far);
		XlsBlockSight(Map, FRTCellId(3, 0, 1));
		XlsExpectStopped(*this, TEXT("muro al piano in cui la linea attraversa la cella"), Map, Viewer, Far,
			ERTCrossLayerSightBlock::CellBlocker, FRTCellId(3, 0, 1));
	}

	// ⚠️ Un LIMITE dichiarato, e questo asserto lo misura invece di raccontarlo. In quella stessa linea il punto
	// in cui si raggiunge la quota del piano 2 cade a 3,5 celle: esattamente sul confine fra due celle. Decide lo
	// spareggio fisso di `HexLine`, che spinge sempre dalla stessa parte. Verso +q il punto finisce nella cella 4
	// e la (3,0,2) non e' un pavimento bucato — e' la premessa qui sopra. Sulla mappa SPECCHIATA finisce nella
	// cella 3, e la stessa cella ferma la linea. La risposta resta la stessa nei due versi; non fra le due mappe.
	{
		const FRTCellId MirrorFar(-5, 0, 2);
		const FRTCellId MirrorTouched(-3, 0, 2);
		XlsExpectStopped(*this, TEXT("la stessa linea, specchiata: il punto sul confine cade dall'altra parte"),
			XlsGroundWith(6, { MirrorFar, MirrorTouched }), Viewer, MirrorFar, ERTCrossLayerSightBlock::Floor, MirrorTouched);
	}
	return true;
}

/**
 * **Contano gli ostacoli del piano in cui la linea sta passando**, non quelli del piano di chi guarda.
 *
 * Da `(0,0,0)` a `(4,0,1)` la linea sta al piano terra sopra le celle 1 e 2, e al piano 1 sopra le celle 2 e
 * 3: la cella 2 e' quella in cui cambia piano. Lo stesso ostacolo, spostato di un piano, smette di contare.
 *
 * 🔑 Dove la regola di ieri avrebbe detto il contrario, la premessa lo asserisce su `HasLineOfSight`: e' la
 * differenza fra le due linee, misurata e non raccontata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCrossLayerObstaclesTest,
	"RefactorTactics.Vision.AcrossLayers.ObstaclesCountOnTheLayerTheLinePasses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCrossLayerObstaclesTest::RunTest(const FString&)
{
	const FRTCellId Viewer(0, 0, 0);
	const FRTCellId Target(4, 0, 1);

	// Il piano 1 ha la cella di chi e' guardato e due celle su cui appoggiare un ostacolo. NON ha la `(2,0,1)`,
	// che e' il punto in cui la linea raggiunge la sua quota: li' deve esserci un buco, o il test misurerebbe
	// il pavimento.
	auto Fresh = [&]() { return XlsGroundWith(6, { Target, FRTCellId(1, 0, 1), FRTCellId(3, 0, 1) }); };

	if (!XlsExpectClear(*this, TEXT("premessa: senza ostacoli la linea passa"), Fresh(), Viewer, Target))
	{
		return false;
	}

	// --- la cella che blocca la vista
	{
		URTHexMapAsset* Map = Fresh();
		XlsBlockSight(Map, FRTCellId(1, 0, 0));
		XlsExpectStopped(*this, TEXT("muro a terra sotto il tratto BASSO"), Map, Viewer, Target,
			ERTCrossLayerSightBlock::CellBlocker, FRTCellId(1, 0, 0));
	}
	{
		URTHexMapAsset* Map = Fresh();
		XlsBlockSight(Map, FRTCellId(3, 0, 0));
		TestFalse(TEXT("premessa: la linea di TIRO, che sta sul piano di chi guarda, quel muro lo trova"),
			URTHexVisionLibrary::HasLineOfSight(Map, Viewer, Target));
		XlsExpectClear(*this, TEXT("muro a terra sotto il tratto ALTO: la linea gli passa sopra"), Map, Viewer, Target);
	}
	{
		URTHexMapAsset* Map = Fresh();
		XlsBlockSight(Map, FRTCellId(3, 0, 1));
		TestTrue(TEXT("premessa: la linea di TIRO quel muro non lo vede, sta su un altro piano"),
			URTHexVisionLibrary::HasLineOfSight(Map, Viewer, Target));
		XlsExpectStopped(*this, TEXT("muro sulla piattaforma nel tratto ALTO"), Map, Viewer, Target,
			ERTCrossLayerSightBlock::CellBlocker, FRTCellId(3, 0, 1));
	}
	{
		URTHexMapAsset* Map = Fresh();
		XlsBlockSight(Map, FRTCellId(1, 0, 1));
		XlsExpectClear(*this, TEXT("muro al piano 1 sopra il tratto BASSO: la linea gli passa sotto"), Map, Viewer, Target);
	}
	{
		URTHexMapAsset* Map = Fresh();
		XlsBlockSight(Map, FRTCellId(2, 0, 0));
		XlsExpectStopped(*this, TEXT("la cella in cui la linea cambia piano risponde anche del piano che lascia"), Map,
			Viewer, Target, ERTCrossLayerSightBlock::CellBlocker, FRTCellId(2, 0, 0));
	}

	// --- il bordo: copertura alta
	{
		URTHexMapAsset* Map = Fresh();
		if (!TestTrue(TEXT("premessa: la copertura si posa"), XlsHighCover(Map, FRTCellId(1, 0, 0), FRTCellId(2, 0, 0))))
		{
			return false;
		}
		XlsExpectStopped(*this, TEXT("copertura alta a terra, su un bordo del tratto BASSO"), Map, Viewer, Target,
			ERTCrossLayerSightBlock::EdgeBlocker, FRTCellId(2, 0, 0));
	}
	{
		URTHexMapAsset* Map = Fresh();
		if (!TestTrue(TEXT("premessa: la copertura si posa"), XlsHighCover(Map, FRTCellId(2, 0, 0), FRTCellId(3, 0, 0))))
		{
			return false;
		}
		TestFalse(TEXT("premessa: la linea di TIRO quella copertura la trova"),
			URTHexVisionLibrary::HasLineOfSight(Map, Viewer, Target));
		XlsExpectClear(*this, TEXT("copertura alta a terra, su un bordo del tratto ALTO"), Map, Viewer, Target);
	}
	{
		// Il parapetto: chi sta sul bordo della piattaforma, dietro una copertura alta, da sotto non si vede.
		const FRTCellId OnEdge(1, 0, 1);
		URTHexMapAsset* Map = XlsGroundWith(6, { OnEdge });
		if (!XlsExpectClear(*this, TEXT("premessa: sul bordo, senza parapetto, si vede"), Map, Viewer, OnEdge)
			|| !TestTrue(TEXT("premessa: il parapetto si posa"), XlsHighCover(Map, OnEdge, FRTCellId(0, 0, 1))))
		{
			return false;
		}
		XlsExpectStopped(*this, TEXT("parapetto alto sul bordo della piattaforma"), Map, Viewer, OnEdge,
			ERTCrossLayerSightBlock::EdgeBlocker, OnEdge);
	}

	// --- il muro interno alto
	{
		URTHexMapAsset* Map = Fresh();
		XlsInteriorWall(Map, FRTCellId(1, 0, 0));
		XlsExpectStopped(*this, TEXT("muro interno a terra, nel tratto BASSO"), Map, Viewer, Target,
			ERTCrossLayerSightBlock::InteriorGeometry, FRTCellId(1, 0, 0));
	}
	{
		URTHexMapAsset* Map = Fresh();
		XlsInteriorWall(Map, FRTCellId(3, 0, 0));
		XlsExpectClear(*this, TEXT("muro interno a terra, sotto il tratto ALTO"), Map, Viewer, Target);
	}
	{
		URTHexMapAsset* Map = Fresh();
		XlsInteriorWall(Map, FRTCellId(3, 0, 1));
		XlsExpectStopped(*this, TEXT("muro interno sulla piattaforma, nel tratto ALTO"), Map, Viewer, Target,
			ERTCrossLayerSightBlock::InteriorGeometry, FRTCellId(3, 0, 1));
	}
	{
		// Nella cella di chi guarda: sta fra lui e l'uscita, come dentro un piano.
		URTHexMapAsset* Map = Fresh();
		XlsInteriorWall(Map, Viewer, RT_GeometryQuanta / 2);
		XlsExpectStopped(*this, TEXT("muro interno nella cella di chi sta sotto"), Map, Viewer, Target,
			ERTCrossLayerSightBlock::InteriorGeometry, Viewer);
	}
	{
		URTHexMapAsset* Map = Fresh();
		XlsInteriorWall(Map, Target, -RT_GeometryQuanta / 2);
		XlsExpectStopped(*this, TEXT("muro interno nella cella di chi sta sopra"), Map, Viewer, Target,
			ERTCrossLayerSightBlock::InteriorGeometry, Target);
	}
	return true;
}

/**
 * **Se io ti vedo, tu mi vedi.** Su ogni coppia di celle di piani diversi di tre arene a piu' piani, la
 * risposta e' la stessa nei due versi: stessa ragione, stessa cella.
 *
 * E' vero per costruzione (la linea si traccia sempre dalla cella piu' bassa), quindi questo test non scopre
 * la proprieta': la sorveglia. Cade il giorno in cui qualcuno fa dipendere il tracciato da chi e' `From`.
 *
 * ⚠️ Gli asserti di non-vacuita' servono davvero: una regola che ferma tutto, o che non ferma niente, e'
 * simmetrica anche lei.
 *
 * 🔑 **La regola non toglie soltanto, e questo test lo conta.** Rispetto alla regola di ieri (`D-510`: la linea
 * di tiro, meno la stessa colonna) ci sono contatti PERSI, dove un pavimento copre, e contatti GUADAGNATI, dove
 * un muro basso sta sotto il tratto alto della linea: da terra, da lontano, si vede sopra il muro chi sta su una
 * piattaforma. Quelli guadagnati sono proprio le coppie che ieri erano a senso unico — dall'alto si vedeva gia'.
 * La prima stesura di questo test asseriva che la regola «toglie contatto»: era vero per conteggio su queste tre
 * arene e falso come proprieta', e una verifica di mutazione l'ha mostrato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCrossLayerSymmetryTest,
	"RefactorTactics.Vision.AcrossLayers.SameAnswerBothWays",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCrossLayerSymmetryTest::RunTest(const FString&)
{
	struct FArena { const TCHAR* Name; const URTHexMapAsset* Map; };
	const FArena Arenas[] = {
		{ TEXT("TestArena"), URTMatchSetupLibrary::MakeTestArena(GetTransientPackage()) },
		{ TEXT("LayerStack"), URTMatchSetupLibrary::MakeLayerStackArena(GetTransientPackage()) },
		{ TEXT("LayerYard"), URTMatchSetupLibrary::MakeLayerYardArena(GetTransientPackage()) },
	};

	for (const FArena& Arena : Arenas)
	{
		if (!TestNotNull(*FString::Printf(TEXT("premessa: %s esiste"), Arena.Name), Arena.Map))
		{
			return false;
		}

		int32 Pairs = 0;
		int32 OneWay = 0;
		int32 ByReason[5] = { 0, 0, 0, 0, 0 };
		int32 Yesterday = 0; // libere per la regola di ieri
		int32 Lost = 0;      // ieri libere, oggi ferme
		int32 Gained = 0;    // ieri ferme, oggi libere
		for (const FRTHexCellData& A : Arena.Map->Cells)
		{
			for (const FRTHexCellData& B : Arena.Map->Cells)
			{
				// Entro sette celle: la portata di vista piu' lunga del roster, e la stessa della proposta.
				if (A.Id.Layer == B.Id.Layer || URTHexLibrary::HexDistance(A.Id, B.Id) > 7)
				{
					continue;
				}
				++Pairs;
				const FRTCrossLayerSightResult Forth = URTPerceptionLibrary::DescribeSightAcrossLayers(Arena.Map, A.Id, B.Id);
				const FRTCrossLayerSightResult Back = URTPerceptionLibrary::DescribeSightAcrossLayers(Arena.Map, B.Id, A.Id);
				++ByReason[static_cast<int32>(Forth.Block)];
				// La regola di ieri (`D-510`): la linea di tiro, sul piano di chi guarda, meno la stessa colonna.
				const bool bYesterday = !(A.Id.X == B.Id.X && A.Id.Y == B.Id.Y)
					&& URTHexVisionLibrary::HasLineOfSight(Arena.Map, A.Id, B.Id);
				Yesterday += bYesterday ? 1 : 0;
				Lost += (bYesterday && !Forth.IsClear()) ? 1 : 0;
				Gained += (!bYesterday && Forth.IsClear()) ? 1 : 0;
				if (Forth.Block != Back.Block || (!Forth.IsClear() && !(Forth.BlockedAt == Back.BlockedAt)))
				{
					// Le prime si nominano, le altre si contano: migliaia di righe d'errore rompono il referto.
					if (++OneWay <= 5)
					{
						AddError(FString::Printf(TEXT("%s: %s -> %s da' %s su %s, al contrario %s su %s"), Arena.Name,
							*XlsCell(A.Id), *XlsCell(B.Id), *XlsName(Forth.Block), *XlsCell(Forth.BlockedAt),
							*XlsName(Back.Block), *XlsCell(Back.BlockedAt)));
					}
				}
			}
		}

		const int32 Clear = ByReason[static_cast<int32>(ERTCrossLayerSightBlock::None)];
		const int32 Floors = ByReason[static_cast<int32>(ERTCrossLayerSightBlock::Floor)];
		TestEqual(*FString::Printf(TEXT("%s: nessuna coppia a senso unico"), Arena.Name), OneWay, 0);
		TestTrue(*FString::Printf(TEXT("%s: non-vacuita' — qualche coppia si vede"), Arena.Name), Clear > 0);
		TestTrue(*FString::Printf(TEXT("%s: non-vacuita' — qualche pavimento ferma"), Arena.Name), Floors > 0);
		TestTrue(*FString::Printf(TEXT("%s: non-vacuita' — qualche contatto di ieri non c'e' piu'"), Arena.Name), Lost > 0);
		TestEqual(*FString::Printf(TEXT("%s: i conti chiudono — oggi = ieri - persi + guadagnati"), Arena.Name),
			Clear, Yesterday - Lost + Gained);

		// La misura, per chi legge il log: e' da qui che vengono i numeri dei documenti, non da un prototipo.
		AddInfo(FString::Printf(
			TEXT("%s: coppie di celle su piani diversi entro 7 celle = %d · libere ieri (D-510) = %d · libere oggi (D-511) = %d · perse = %d · guadagnate = %d · Floor = %d · CellBlocker = %d · EdgeBlocker = %d · InteriorGeometry = %d"),
			Arena.Name, Pairs, Yesterday, Clear, Lost, Gained, Floors,
			ByReason[static_cast<int32>(ERTCrossLayerSightBlock::CellBlocker)],
			ByReason[static_cast<int32>(ERTCrossLayerSightBlock::EdgeBlocker)],
			ByReason[static_cast<int32>(ERTCrossLayerSightBlock::InteriorGeometry)]));
	}
	return true;
}

/**
 * I due bordi della regola. Nella stessa COLONNA la linea e' verticale e si ferma sul pavimento di chi sta
 * sopra (`D-510`), anche se l'asset quella cella non ce l'ha. Sullo stesso PIANO la domanda non e' sua.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCrossLayerBoundsTest,
	"RefactorTactics.Vision.AcrossLayers.SameColumnStopsAndSameLayerIsNotAsked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCrossLayerBoundsTest::RunTest(const FString&)
{
	const URTHexMapAsset* Map = URTMatchSetupLibrary::MakeTestArena(GetTransientPackage());

	XlsExpectStopped(*this, TEXT("stessa colonna, un piano"), Map, FRTCellId(2, 0, 0), FRTCellId(2, 0, 1),
		ERTCrossLayerSightBlock::Floor, FRTCellId(2, 0, 1));
	if (!TestNull(TEXT("premessa: la cella a quattro piani non e' nell'asset"), Map->FindCell(FRTCellId(-3, 0, 4))))
	{
		return false;
	}
	XlsExpectStopped(*this, TEXT("stessa colonna, cella che l'asset non ha"), Map, FRTCellId(-3, 0, 0), FRTCellId(-3, 0, 4),
		ERTCrossLayerSightBlock::Floor, FRTCellId(-3, 0, 4));

	// Stesso piano, col muro di `q = 0` in mezzo: la linea di TIRO e' ferma, e questa funzione non lo dice
	// perche' non e' lei a deciderlo. Chi smista e' il contatto.
	const FRTCellId West(-1, 0, 0);
	const FRTCellId East(1, 0, 0);
	if (!TestFalse(TEXT("premessa: fra le due celle c'e' il muro"), URTHexVisionLibrary::HasLineOfSight(Map, West, East)))
	{
		return false;
	}
	TestTrue(TEXT("sullo stesso piano risponde None"), URTPerceptionLibrary::DescribeSightAcrossLayers(Map, West, East).IsClear());

	TestTrue(TEXT("senza mappa non c'e' pavimento noto"),
		URTPerceptionLibrary::DescribeSightAcrossLayers(nullptr, FRTCellId(0, 0, 0), FRTCellId(1, 0, 1)).IsClear());
	return true;
}

/**
 * `HexLinePoint` E' l'arrotondamento di `HexLine`: ai campioni interi le due funzioni danno la stessa cella, su
 * ogni coppia di un'area. E il punto non dipende dal verso in cui si percorre la retta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLinePointMatchesHexLineTest,
	"RefactorTactics.Hex.LinePointIsTheRoundingOfHexLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLinePointMatchesHexLineTest::RunTest(const FString&)
{
	const TArray<FRTCellId> Area = URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 5);
	int32 Samples = 0;
	int32 Mismatches = 0;
	int32 NotReversible = 0;
	for (const FRTCellId& A : Area)
	{
		for (const FRTCellId& B : Area)
		{
			const TArray<FRTCellId> Line = URTHexLibrary::HexLine(A, B);
			const int32 N = Line.Num() - 1;
			for (int32 I = 0; I <= N && N > 0; ++I)
			{
				++Samples;
				if (!(URTHexLibrary::HexLinePoint(A, B, I, N) == Line[I]))
				{
					++Mismatches;
				}
			}
			// Una frazione che NON e' un campione: i quinti, che sono quelli della vista fra piani.
			for (int32 Fifth = 0; Fifth <= 5; ++Fifth)
			{
				const FRTCellId Forth = URTHexLibrary::HexLinePoint(A, B, Fifth, 5);
				const FRTCellId Back = URTHexLibrary::HexLinePoint(B, A, 5 - Fifth, 5);
				if (Forth.X != Back.X || Forth.Y != Back.Y)
				{
					++NotReversible;
				}
			}
		}
	}
	TestTrue(TEXT("non-vacuita': qualche campione e' stato confrontato"), Samples > 1000);
	TestEqual(TEXT("ai campioni interi HexLinePoint e HexLine danno la stessa cella"), Mismatches, 0);
	TestEqual(TEXT("il punto e' lo stesso nei due versi"), NotReversible, 0);

	const FRTCellId A(1, -2, 3);
	TestTrue(TEXT("il piano e' quello di A"), URTHexLibrary::HexLinePoint(A, FRTCellId(4, 0, 0), 2, 5).Layer == 3);
	TestTrue(TEXT("senza denominatore si resta in A"), URTHexLibrary::HexLinePoint(A, FRTCellId(4, 0, 0), 2, 0) == A);
	TestTrue(TEXT("oltre la fine si arriva alla colonna di B"),
		URTHexLibrary::HexLinePoint(A, FRTCellId(4, 0, 0), 9, 5) == FRTCellId(4, 0, 3));
	return true;
}

/**
 * `#3757`: fra due piani il fumo conta **come i muri** (scelta d'autore del 2026-10-18) — il fumo del piano in cui
 * la linea attraversa ciascuna cella, da testa a testa — e il contatto torna uguale nei due versi.
 *
 * L'esempio della issue: Aevik a terra, nel fumo; Branth su una piattaforma, a tre celle, fuori dal fumo. Prima
 * la linea di Branth stava sul SUO piano, dove il fumo non c'e': lui vedeva Aevik e Aevik non vedeva lui.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCrossLayerSmokeTest,
	"RefactorTactics.Vision.AcrossLayers.SmokeCountsOnTheLayerTheLinePasses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCrossLayerSmokeTest::RunTest(const FString&)
{
	const FRTCellId Aevik(0, 0, 0);
	const FRTCellId Branth(3, 0, 1);
	const TArray<FRTCellId> Platform = { Branth, FRTCellId(4, 0, 1) };
	constexpr int32 Reach = 5;

	// Controllo: senza fumo il contatto c'e' nei due versi, e il tetto non scende.
	URTHexMapAsset* Clear = XlsGroundWith(6, Platform);
	TestEqual(TEXT("senza fumo il tetto non scende (Aevik -> Branth)"), URTPerceptionLibrary::EffectiveContactRange(Clear, Aevik, Branth, Reach), Reach);
	TestTrue(TEXT("senza fumo Aevik vede Branth"), XlsSees(Clear, Aevik, ERTHexDirection::E, Branth));
	TestTrue(TEXT("senza fumo Branth vede Aevik"), XlsSees(Clear, Branth, ERTHexDirection::W, Aevik));

	// L'esempio: Aevik nel fumo. Il tetto e' 2 nei due versi, e a tre celle nessuno dei due vede l'altro.
	URTHexMapAsset* InSmoke = XlsGroundWith(6, Platform);
	XlsSetSurface(InSmoke, Aevik, ERTHexSurface::Smoke);
	TestEqual(TEXT("fumo sul capo basso: tetto 2, Aevik -> Branth"), URTPerceptionLibrary::EffectiveContactRange(InSmoke, Aevik, Branth, Reach), 2);
	TestEqual(TEXT("fumo sul capo basso: tetto 2, Branth -> Aevik"), URTPerceptionLibrary::EffectiveContactRange(InSmoke, Branth, Aevik, Reach), 2);
	TestFalse(TEXT("Aevik non vede Branth"), XlsSees(InSmoke, Aevik, ERTHexDirection::E, Branth));
	TestFalse(TEXT("🔴 e Branth non vede Aevik: il senso unico non c'e' piu'"), XlsSees(InSmoke, Branth, ERTHexDirection::W, Aevik));

	// Il fumo sul capo ALTO conta, anche visto da chi sta a terra e non ha fumo sul proprio piano.
	URTHexMapAsset* AtTheTop = XlsGroundWith(6, Platform);
	XlsSetSurface(AtTheTop, Branth, ERTHexSurface::Smoke);
	TestEqual(TEXT("fumo sul capo alto: tetto 2, Aevik -> Branth"), URTPerceptionLibrary::EffectiveContactRange(AtTheTop, Aevik, Branth, Reach), 2);
	TestEqual(TEXT("fumo sul capo alto: tetto 2, Branth -> Aevik"), URTPerceptionLibrary::EffectiveContactRange(AtTheTop, Branth, Aevik, Reach), 2);

	// Fumo a terra SOTTO la parte alta della linea: la linea passa sopra, e non conta. E' la meta' «come i muri»
	// che il planare di prima sbagliava (la linea di Aevik, sul piano 0, lo attraversava).
	URTHexMapAsset* Under = XlsGroundWith(6, Platform);
	XlsSetSurface(Under, FRTCellId(2, 0, 0), ERTHexSurface::Smoke);
	TestEqual(TEXT("la linea passa sopra il fumo: tetto invariato, Aevik -> Branth"), URTPerceptionLibrary::EffectiveContactRange(Under, Aevik, Branth, Reach), Reach);
	TestEqual(TEXT("la linea passa sopra il fumo: tetto invariato, Branth -> Aevik"), URTPerceptionLibrary::EffectiveContactRange(Under, Branth, Aevik, Reach), Reach);

	// Le celle INTERMEDIE contano sul piano in cui la linea le attraversa. (1,0) e' attraversata sui piani 0 e 1
	// (la linea parte dalla testa a tre quinti di piano): il fumo su uno dei due la cappa. (2,0) solo sul piano 1.
	for (const FRTCellId& SmokeAt : { FRTCellId(1, 0, 0), FRTCellId(1, 0, 1), FRTCellId(2, 0, 1) })
	{
		URTHexMapAsset* Mid = XlsGroundWith(6, { Branth, FRTCellId(4, 0, 1), FRTCellId(1, 0, 1), FRTCellId(2, 0, 1) });
		XlsSetSurface(Mid, SmokeAt, ERTHexSurface::Smoke);
		const FString Where = XlsCell(SmokeAt);
		TestEqual(*FString::Printf(TEXT("fumo intermedio in %s: tetto 2, Aevik -> Branth"), *Where), URTPerceptionLibrary::EffectiveContactRange(Mid, Aevik, Branth, Reach), 2);
		TestEqual(*FString::Printf(TEXT("fumo intermedio in %s: tetto 2, Branth -> Aevik"), *Where), URTPerceptionLibrary::EffectiveContactRange(Mid, Branth, Aevik, Reach), 2);
	}

	// Due piani di dislivello (Rise = 2): il capo alto e quello basso rispondono del proprio piano, i due versi uguali.
	URTHexMapAsset* Tall = XlsGroundWith(6, { FRTCellId(3, 0, 2), FRTCellId(4, 0, 2) });
	XlsSetSurface(Tall, FRTCellId(3, 0, 2), ERTHexSurface::Smoke);
	TestEqual(TEXT("due piani: fumo sul capo alto, Aevik -> cima"), URTPerceptionLibrary::EffectiveContactRange(Tall, Aevik, FRTCellId(3, 0, 2), Reach), 2);
	TestEqual(TEXT("due piani: fumo sul capo alto, cima -> Aevik"), URTPerceptionLibrary::EffectiveContactRange(Tall, FRTCellId(3, 0, 2), Aevik, Reach), 2);

	// Stesso piano: la regola di sempre, la stessa funzione.
	URTHexMapAsset* Flat = XlsGroundWith(6, {});
	XlsSetSurface(Flat, FRTCellId(1, 0, 0), ERTHexSurface::Smoke);
	TestEqual(TEXT("stesso piano: invariato"), URTPerceptionLibrary::EffectiveContactRange(Flat, Aevik, FRTCellId(4, 0, 0), Reach),
		URTTerrainLibrary::EffectiveTargetingRange(Flat, Aevik, FRTCellId(4, 0, 0), Reach));
	TestEqual(TEXT("senza mappa: il numero in ingresso"), URTPerceptionLibrary::EffectiveContactRange(nullptr, Aevik, Branth, Reach), Reach);

	// Simmetria su tutte le coppie di piani diversi di una mappa con fumo su piu' piani. Non-vacuita': qualche
	// coppia e' cappata e qualche altra no.
	URTHexMapAsset* Stack = XlsGroundWith(4, { FRTCellId(1, 0, 1), FRTCellId(2, 0, 1), FRTCellId(3, 0, 1), FRTCellId(2, 1, 1), FRTCellId(2, 0, 2), FRTCellId(3, -1, 2) });
	XlsSetSurface(Stack, FRTCellId(0, 0, 0), ERTHexSurface::Smoke);
	XlsSetSurface(Stack, FRTCellId(2, 0, 1), ERTHexSurface::Smoke);
	XlsSetSurface(Stack, FRTCellId(3, -1, 2), ERTHexSurface::Smoke);
	int32 Pairs = 0, Capped = 0, Uncapped = 0, OneWay = 0;
	for (const FRTHexCellData& A : Stack->Cells)
	{
		for (const FRTHexCellData& B : Stack->Cells)
		{
			if (A.Id.Layer == B.Id.Layer)
			{
				continue;
			}
			++Pairs;
			const int32 Forth = URTPerceptionLibrary::EffectiveContactRange(Stack, A.Id, B.Id, Reach);
			const int32 Back = URTPerceptionLibrary::EffectiveContactRange(Stack, B.Id, A.Id, Reach);
			OneWay += (Forth != Back) ? 1 : 0;
			Capped += (Forth < Reach) ? 1 : 0;
			Uncapped += (Forth == Reach) ? 1 : 0;
		}
	}
	TestEqual(TEXT("nessuna coppia a senso unico"), OneWay, 0);
	TestTrue(TEXT("non-vacuita': qualche coppia e' cappata"), Capped > 0);
	TestTrue(TEXT("non-vacuita': qualche coppia no"), Uncapped > 0);
	TestTrue(TEXT("non-vacuita': ci sono coppie"), Pairs > 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
