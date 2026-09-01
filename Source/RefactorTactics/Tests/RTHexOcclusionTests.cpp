#include "Misc/AutomationTest.h"
#include "Combat/RTOffensiveActionLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTGeometryGrammar.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexOcclusionLibrary.h"
#include "Map/RTHexOccupancyLibrary.h"
#include "Map/RTHexVisionLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Prefisso `Occl`: la unity build fonde i namespace anonimi, e un nome generico collide con l'omonimo di
	// un altro file di test (`#1530`).

	URTHexMapAsset* OcclMakeArena(int32 Radius)
	{
		return URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
	}

	/**
	 * Il DIAMETRO VERTICALE della cella: giacitura a 90 gradi, per il centro, da un vertice all'altro.
	 * Taglia in due la corda orizzontale `EdgeMid(W) -> EdgeMid(E)`.
	 */
	FRTGeometrySegment OcclVerticalDiameter(ERTHexCoverType Type = ERTHexCoverType::High, int32 Layer = 0)
	{
		FRTGeometrySegment S;
		S.Axis = ERTTacticalAxis::Deg90;
		S.Offset = 0;
		S.AlongStart = -RT_GeometryQuanta;
		S.AlongEnd = RT_GeometryQuanta;
		S.Layer = Layer;
		S.WallType = Type;
		return S;
	}

	/** Il DIAMETRO ORIZZONTALE: giacitura a 0 gradi, per il centro. E' COLLINEARE alla corda `W -> E`. */
	FRTGeometrySegment OcclHorizontalDiameter()
	{
		FRTGeometrySegment S;
		S.Axis = ERTTacticalAxis::Deg0;
		S.Offset = 0;
		S.AlongStart = -RT_GeometryQuanta;
		S.AlongEnd = RT_GeometryQuanta;
		S.Layer = 0;
		S.WallType = ERTHexCoverType::High;
		return S;
	}

	/**
	 * Un muro verticale a meta' strada fra il centro e il lato `E`: taglia la corda del TIRATORE
	 * (`Center -> EdgeMid(E)`) senza toccare il centro.
	 */
	FRTGeometrySegment OcclHalfwayToEast()
	{
		FRTGeometrySegment S;
		S.Axis = ERTTacticalAxis::Deg90;
		S.Offset = RT_GeometryQuanta / 2;
		S.AlongStart = -RT_GeometryQuanta / 2;
		S.AlongEnd = RT_GeometryQuanta / 2;
		S.Layer = 0;
		S.WallType = ERTHexCoverType::High;
		return S;
	}

	/** Il muro che CHIUDE il lato `E`: verticale, traslato fino al punto medio del lato (l'esempio canonico). */
	FRTGeometrySegment OcclOnEastEdge()
	{
		FRTGeometrySegment S;
		S.Axis = ERTTacticalAxis::Deg90;
		S.Offset = RT_GeometryQuanta;
		S.AlongStart = -RT_GeometryQuanta / 2;
		S.AlongEnd = RT_GeometryQuanta / 2;
		S.Layer = 0;
		S.WallType = ERTHexCoverType::High;
		return S;
	}

	void OcclAddWall(URTHexMapAsset* Map, const FRTCellId& Cell, const FRTGeometrySegment& Segment,
		FName StableId = NAME_None)
	{
		FRTHexInteriorWall Wall(Cell, Segment);
		Wall.StableId = StableId;
		Map->InteriorWalls.Add(Wall);
	}
}

// ---------------------------------------------------------------------------------------------------------
// Le due ancore: la geometria esatta non deve poter mentire
// ---------------------------------------------------------------------------------------------------------

/**
 * LA TABELLA INTERA DICE LO STESSO DELL'ORACOLO IN VIRGOLA MOBILE.
 *
 * 🔑 **E' il test che tiene onesta l'unica trascrizione della libreria.** `OcclusionBoundaryR3/One` riscrive
 * in interi cio' che `SectorBoundaryPoints` calcola con dei coseni: se la convenzione dei dodici confini
 * cambiasse — o se la trascrizione fosse sbagliata fin dall'inizio — nulla se ne accorgerebbe, perche' tutto
 * il resto della libreria e' coerente *con la tabella*, non con la geometria.
 *
 * Provato su DUE `HexSize` diversi: la forma esatta e' proporzionale al raggio, e un errore di scala
 * passerebbe su un raggio solo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOcclusionBoundaryTableTest,
	"RefactorTactics.Occlusion.BoundaryTableMatchesTheFloatOracle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOcclusionBoundaryTableTest::RunTest(const FString&)
{
	for (const float HexSize : { 100.0f, 37.5f })
	{
		TArray<FVector2D> Oracle;
		URTHexOccupancyLibrary::SectorBoundaryPoints(HexSize, Oracle);
		TestEqual(TEXT("l'oracolo ha dodici punti"), Oracle.Num(), RT_OccupancySectorCount);

		for (int32 I = 0; I < Oracle.Num(); ++I)
		{
			const FVector2D Exact = URTHexOcclusionLibrary::ToLocal(URTHexOcclusionLibrary::BoundaryPointQ(I), HexSize);
			TestTrue(FString::Printf(TEXT("confine %d coincide (HexSize %.1f)"), I, HexSize),
				Exact.Equals(Oracle[I], 0.001));
		}
	}
	return true;
}

/**
 * IL PUNTO MEDIO DI LATO CHIESTO ALLA GRAMMATICA E' QUELLO CHE LA MAPPA DISEGNA — su tutte e sei le direzioni.
 *
 * 🔴 **`DirectionForEdgeIndex(k)` vale `(6 - k) % 6`**: l'ordinale di `ERTHexDirection` e l'indice geometrico
 * di lato girano in versi OPPOSTI e coincidono solo su `E` e `W`. Chi costruisce la corda trascrivendo la
 * corrispondenza a mano sbaglia su quattro direzioni su sei — ed e' un difetto SIMMETRICO, quindi su
 * un'arena simmetrica ogni altro test passerebbe lo stesso. E' l'errore che `#1920` ha pagato due volte.
 *
 * L'oracolo e' `EdgeMidpointWorld`, che deriva il punto dai due CENTRI di cella e non passa ne' da
 * `SectorBoundaryPoints` ne' da `EdgeIndexForDirection`: e' indipendente per costruzione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOcclusionEdgeMidOracleTest,
	"RefactorTactics.Occlusion.EdgeMidMatchesTheWorldOracle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOcclusionEdgeMidOracleTest::RunTest(const FString&)
{
	constexpr float HexSize = 100.0f;
	const FVector Origin = FVector::ZeroVector;
	// Una cella ASIMMETRICA: su `(0,0)` un errore di verso sarebbe invisibile, perche' il centro e' l'origine
	// e i sei punti medi si mappano l'uno sull'altro per simmetria.
	const FRTCellId Cell(2, -1);
	const FVector Center = URTHexLibrary::AxialToWorld(Cell, Origin, HexSize, 0.0f);

	for (int32 D = 0; D < 6; ++D)
	{
		const ERTHexDirection Dir = static_cast<ERTHexDirection>(D);
		const FVector World = URTHexLibrary::EdgeMidpointWorld(Cell, Dir, Origin, HexSize, 0.0f);
		const FVector2D Expected(World.X - Center.X, World.Y - Center.Y);

		const FRTLocalPointQ Q = URTHexOcclusionLibrary::AnchorPointQ(
			ERTAnchorKind::EdgeMid, URTHexLibrary::EdgeIndexForDirection(Dir));
		const FVector2D Actual = URTHexOcclusionLibrary::ToLocal(Q, HexSize);

		TestTrue(FString::Printf(TEXT("il punto medio del lato %d coincide con quello del mondo"), D),
			Actual.Equals(Expected, 0.001));
	}
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// La regola d'attraversamento
// ---------------------------------------------------------------------------------------------------------

/** Un muro alto che taglia la cella ferma la vista, e la ragione lo NOMINA. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOcclusionCrossingWallBlocksTest,
	"RefactorTactics.Occlusion.CrossingWallBlocksSight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOcclusionCrossingWallBlocksTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = OcclMakeArena(3);
	const FRTCellId From(0, 0);
	const FRTCellId Mid(1, 0);
	const FRTCellId To(2, 0);

	TestTrue(TEXT("senza geometria interna la vista passa"), URTHexVisionLibrary::HasLineOfSight(Map, From, To));

	OcclAddWall(Map, Mid, OcclVerticalDiameter());

	TestFalse(TEXT("il muro interno ferma la vista"), URTHexVisionLibrary::HasLineOfSight(Map, From, To));

	const FRTLineOfSightResult Result = URTHexVisionLibrary::DescribeLineOfSight(Map, From, To);
	TestEqual(TEXT("la ragione e' la geometria interna"), Result.Block, ERTLineOfSightBlock::InteriorGeometry);
	TestTrue(TEXT("nomina la cella col muro"), Result.BlockedAt == Mid);
	TestTrue(TEXT("nomina la cella da cui si entrava"), Result.BlockedFrom == From);
	TestEqual(TEXT("l'indice e' quello della cella"), Result.StepIndex, 1);

	// ⚠️ Indipendenza dall'ordine (`D-269`): la stessa geometria vista dall'altro capo.
	TestFalse(TEXT("bloccata anche al contrario"), URTHexVisionLibrary::HasLineOfSight(Map, To, From));
	return true;
}

/** Un muro nella cella del TIRATORE lo chiude dentro: gli estremi non sono esclusi. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOcclusionWallInShooterCellTest,
	"RefactorTactics.Occlusion.WallInShooterCellBlocks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOcclusionWallInShooterCellTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = OcclMakeArena(3);
	const FRTCellId From(0, 0);
	const FRTCellId To(2, 0);

	OcclAddWall(Map, From, OcclHalfwayToEast());

	const FRTLineOfSightResult Result = URTHexVisionLibrary::DescribeLineOfSight(Map, From, To);
	TestEqual(TEXT("il muro davanti al tiratore blocca"), Result.Block, ERTLineOfSightBlock::InteriorGeometry);
	TestTrue(TEXT("la cella colpevole e' la sua"), Result.BlockedAt == From);
	TestEqual(TEXT("l'indice puo' valere zero"), Result.StepIndex, 0);
	return true;
}

/** `D-271`: `Low` e' copertura direzionale parziale, non occlusione. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOcclusionLowWallTest,
	"RefactorTactics.Occlusion.LowWallDoesNotOcclude",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOcclusionLowWallTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = OcclMakeArena(3);
	OcclAddWall(Map, FRTCellId(1, 0), OcclVerticalDiameter(ERTHexCoverType::Low));

	TestTrue(TEXT("un muretto non toglie la vista"),
		URTHexVisionLibrary::HasLineOfSight(Map, FRTCellId(0, 0), FRTCellId(2, 0)));
	return true;
}

/** La regola d'elevazione vale anche qui: un muro su un altro piano non e' in mezzo a niente. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOcclusionOtherLayerTest,
	"RefactorTactics.Occlusion.WallOnOtherLayerDoesNotOcclude",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOcclusionOtherLayerTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = OcclMakeArena(3);
	// Stessa colonna, PIANO diverso: la cella che la linea attraversa e' quella sul layer del tiratore.
	OcclAddWall(Map, FRTCellId(1, 0, 1), OcclVerticalDiameter(ERTHexCoverType::High, 1));

	TestTrue(TEXT("un muro di un altro piano non toglie la vista"),
		URTHexVisionLibrary::HasLineOfSight(Map, FRTCellId(0, 0), FRTCellId(2, 0)));
	return true;
}

/**
 * TANGENZA E COLLINEARITA' NON BLOCCANO, ed e' una scelta dichiarata.
 *
 * Toccare un muro in un estremo della corda non e' attraversarlo — il muro sul lato `E` e' un BORDO, e come
 * bordo lo rappresenta `FRTHexCover`, che e' l'approssimazione gia' dichiarata su `InteriorWalls`. Guardare
 * LUNGO un muro non e' guardarci attraverso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOcclusionDegenerateTest,
	"RefactorTactics.Occlusion.TangentAndCollinearDoNotBlock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOcclusionDegenerateTest::RunTest(const FString&)
{
	const FRTCellId From(0, 0);
	const FRTCellId Mid(1, 0);
	const FRTCellId To(2, 0);

	URTHexMapAsset* Tangent = OcclMakeArena(3);
	OcclAddWall(Tangent, Mid, OcclOnEastEdge());
	TestTrue(TEXT("un muro sul lato e' toccato in un estremo, non attraversato"),
		URTHexVisionLibrary::HasLineOfSight(Tangent, From, To));

	URTHexMapAsset* Collinear = OcclMakeArena(3);
	OcclAddWall(Collinear, Mid, OcclHorizontalDiameter());
	TestTrue(TEXT("guardare lungo un muro non e' guardarci attraverso"),
		URTHexVisionLibrary::HasLineOfSight(Collinear, From, To));
	return true;
}

/** Una cella senza geometria interna si comporta esattamente come prima: nessuna regressione. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOcclusionNoRegressionTest,
	"RefactorTactics.Occlusion.CellWithoutInteriorGeometryIsUnchanged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOcclusionNoRegressionTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = OcclMakeArena(3);

	// Ogni coppia ordinata dell'arena: senza muri interni, nessuna deve produrre la ragione nuova.
	int32 Pairs = 0;
	for (const FRTHexCellData& A : Map->Cells)
	{
		for (const FRTHexCellData& B : Map->Cells)
		{
			const FRTLineOfSightResult R = URTHexVisionLibrary::DescribeLineOfSight(Map, A.Id, B.Id);
			if (R.Block == ERTLineOfSightBlock::InteriorGeometry)
			{
				AddError(FString::Printf(TEXT("geometria interna inventata su %s -> %s"),
					*A.Id.ToString(), *B.Id.ToString()));
			}
			++Pairs;
		}
	}
	TestTrue(TEXT("il corpus non e' vuoto"), Pairs > 100);
	return true;
}

/**
 * LA GEOMETRIA NON SELEZIONATA CONTINUA A VALERE — l'AC che il Decision Record del 2026-08-30 pretende:
 * *«la CoverOption selezionata non rende intangibili gli altri ostacoli»*.
 *
 * 🔑 Qui la garanzia e' STRUTTURALE prima che asserita: `BlocksSight` non riceve nessuna `FRTCoverOption` e
 * non ha modo di sapere quale sia attiva. Il test lo esercita comunque con una sorgente di copertura
 * concorrente sulla cella — una copertura di BORDO su un altro lato, che e' la sorgente che un'unita'
 * sceglierebbe — e pretende che il muro interno fermi la linea lo stesso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOcclusionSelectedCoverTest,
	"RefactorTactics.Occlusion.SelectedCoverDoesNotDisarmGeometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOcclusionSelectedCoverTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = OcclMakeArena(3);
	const FRTCellId Mid(1, 0);

	// Una sorgente di copertura CONCORRENTE, su un bordo che la linea non attraversa: e' «la roccia a nord».
	FRTHexCellData Data = *Map->FindCell(Mid);
	Data.Covers.Add(FRTHexCover(ERTHexDirection::NE, ERTHexCoverType::Low,
		FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low)));
	Map->AddOrUpdateCell(Data);
	Map->SortCells();

	OcclAddWall(Map, Mid, OcclVerticalDiameter());

	TestFalse(TEXT("l'albero a sud resta tangibile"),
		URTHexVisionLibrary::HasLineOfSight(Map, FRTCellId(0, 0), FRTCellId(2, 0)));
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Vista e proiettile: la stessa funzione, non due
// ---------------------------------------------------------------------------------------------------------

/**
 * LE DUE RISPOSTE NON POSSONO DIVERGERE, e il test lo esercita su un corpus invece di fidarsi della forma.
 *
 * `D-269`: *«risposte diverse renderebbero visibile un bersaglio che non si puo' colpire»*. La garanzia e'
 * strutturale — `DescribeLineOfSight` e `LineCells` chiamano la STESSA `URTHexOcclusionLibrary::BlocksSight`
 * sulla stessa corda — e questo test e' la rete: se un giorno qualcuno riscrivesse l'attraversamento in uno
 * dei due «per non pagare la chiamata», cadrebbe qui.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOcclusionSightProjectileAgreeTest,
	"RefactorTactics.Occlusion.SightAndProjectileAgree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOcclusionSightProjectileAgreeTest::RunTest(const FString&)
{
	const FRTCellId From(0, 0);
	const FRTCellId Mid(1, 0);
	const FRTCellId To(2, 0);

	URTHexMapAsset* Clear = OcclMakeArena(3);
	TestTrue(TEXT("senza muri la vista passa"), URTHexVisionLibrary::HasLineOfSight(Clear, From, To));
	TestTrue(TEXT("senza muri il colpo arriva"),
		URTOffensiveActionLibrary::LineCells(Clear, From, To, 2).Contains(To));

	URTHexMapAsset* Walled = OcclMakeArena(3);
	OcclAddWall(Walled, Mid, OcclVerticalDiameter());

	TestFalse(TEXT("col muro la vista non passa"), URTHexVisionLibrary::HasLineOfSight(Walled, From, To));

	const TArray<FRTCellId> Cells = URTOffensiveActionLibrary::LineCells(Walled, From, To, 2);
	TestFalse(TEXT("col muro il colpo non arriva"), Cells.Contains(To));
	TestTrue(TEXT("ma entra nella cella del muro"), Cells.Contains(Mid));

	// Il reason code NOMINA la causa: e' cio' che il combat log deve poter distinguere da «non c'e' copertura».
	const FRTLineAttackResult Attack = URTOffensiveActionLibrary::ResolveLineAttack(
		Walled, From, To, 2, TMap<FRTCellId, int32>(), TSet<int32>());
	TestEqual(TEXT("il log dice che e' stata la geometria interna"), Attack.Stop,
		ERTLineStop::BlockedByInteriorGeometry);

	// ⚠️ E il muro alto di CELLA continua a dire l'altra cosa: i due motivi restano distinti.
	URTHexMapAsset* CellBlocked = OcclMakeArena(3);
	FRTHexCellData Blocker = *CellBlocked->FindCell(Mid);
	Blocker.bBlocksLineOfSight = true;
	CellBlocked->AddOrUpdateCell(Blocker);
	CellBlocked->SortCells();
	const FRTLineAttackResult ByCover = URTOffensiveActionLibrary::ResolveLineAttack(
		CellBlocked, From, To, 2, TMap<FRTCellId, int32>(), TSet<int32>());
	TestEqual(TEXT("una copertura di cella resta BlockedByCover"), ByCover.Stop, ERTLineStop::BlockedByCover);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// L'hash
// ---------------------------------------------------------------------------------------------------------

/**
 * UN MURO INTERNO CAMBIA L'HASH DELLA MAPPA — e rinominarlo no.
 *
 * 🔴 Fino a `#1830` `InteriorWalls` restava fuori da `ComputeHash`, e la ragione scritta sul tipo era che
 * *«vista e passo non lo consultano»*. Ora lo consultano: lasciarlo fuori significherebbe che due mappe che
 * si giocano diversamente hanno lo stesso hash, che `IsSnapshotStale` lascia «fresco» uno snapshot dopo uno
 * spostamento, e che una divergenza di replay diventa non diagnosticabile.
 *
 * ⛔ Il nome no: nessuno risolve un muro interno per nome a runtime. E' il criterio di sempre — nell'hash
 * entra cio' che puo' cambiare un ESITO — applicato al caso opposto a `FRTHexDoor::StableId`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOcclusionHashTest,
	"RefactorTactics.HexMap.InteriorWallEntersHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOcclusionHashTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = OcclMakeArena(2);
	const uint32 Bare = Map->ComputeHash();

	OcclAddWall(Map, FRTCellId(1, 0), OcclVerticalDiameter(), TEXT("Wall_A"));
	const uint32 WithWall = Map->ComputeHash();
	TestNotEqual(TEXT("un muro interno cambia l'hash"), WithWall, Bare);

	// SPOSTARLO cambia chi vede chi, quindi cambia l'hash.
	Map->InteriorWalls[0].Segment.Offset = RT_GeometryQuanta / 2;
	const uint32 Moved = Map->ComputeHash();
	TestNotEqual(TEXT("spostarlo cambia l'hash"), Moved, WithWall);

	// ABBASSARLO cambia se occlude, quindi cambia l'hash.
	Map->InteriorWalls[0].Segment.WallType = ERTHexCoverType::Low;
	TestNotEqual(TEXT("abbassarlo cambia l'hash"), Map->ComputeHash(), Moved);
	Map->InteriorWalls[0].Segment.WallType = ERTHexCoverType::High;

	// RINOMINARLO no: il nome non decide nessun esito.
	Map->InteriorWalls[0].StableId = TEXT("Wall_Rinominato");
	TestEqual(TEXT("rinominarlo NON cambia l'hash"), Map->ComputeHash(), Moved);

	// L'ordine dell'array lo decide chi edita l'asset: non deve entrare nell'hash.
	URTHexMapAsset* Ordered = OcclMakeArena(2);
	OcclAddWall(Ordered, FRTCellId(1, 0), OcclVerticalDiameter());
	OcclAddWall(Ordered, FRTCellId(0, 1), OcclHorizontalDiameter());
	const uint32 OneWay = Ordered->ComputeHash();
	Ordered->InteriorWalls.Swap(0, 1);
	TestEqual(TEXT("l'ordine di inserimento non cambia l'hash"), Ordered->ComputeHash(), OneWay);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
