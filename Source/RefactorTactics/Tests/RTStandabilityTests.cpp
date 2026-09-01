#include "Misc/AutomationTest.h"

#include "Map/RTGeometryBake.h"
#include "Map/RTGeometryGrammar.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverPlacementLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexOccupancyLibrary.h"
#include "Pathfinding/RTHexPath.h"
#include "Pathfinding/RTHexPathLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * LA CALPESTABILITA' E' L'ESITO DI UNA POSA — `E23.6`, `#1827`, `D-289`.
 *
 * 🔴 **Il difetto che questi test chiudono non era un calcolo sbagliato: era un anello aperto.**
 * `URTHexCoverPlacementLibrary::HasLegalPlacement` esisteva in `main` con tredici test verdi, e `git grep`
 * non le trovava **un solo chiamante di produzione**. La calpestabilita' rispondeva percio' a due
 * scorciatoie che `D-289` dichiara superate — *«>= 6 settori occupati => Blocked»* e *«centro toccato =>
 * Blocked»* — e la primitiva giusta stava li' a non essere chiamata da nessuno.
 *
 * ⚠️ **Perche' qui e non in `Scenarios/Spec/Map/`.** L'issue dichiara tre scenari —
 * `WallCrossesCellStillStandable`, `FootprintCollisionBlocksCell`, `NinetyDegreeCornerBakesCorrectly` — ma
 * l'harness non sa costruire un muro interno: `FRTTestScenarioCell` ha `bBlocksMovement` e **nessun campo
 * di geometria**. Scriverli oggi chiederebbe prima di estendere l'harness, che e' un lavoro suo. Questi
 * test coprono gli stessi criteri, incluso quello sul **percorso reale**, e la lacuna e' dichiarata.
 */

namespace
{
	// Prefisso `Stand`: la unity build fonde i namespace anonimi (vedi `IncidenceHexSize`, `#1530`).
	constexpr float StandHexSize = 100.0f;

	/** Un diametro fra due VERTICI: attraversa la cella e non giace su nessun bordo, quindi e' interno. */
	FRTGeometrySegment StandDiameter(ERTTacticalAxis Axis)
	{
		FRTGeometrySegment S;
		S.Axis = Axis;
		S.Offset = 0;
		S.AlongStart = -RT_GeometryQuanta;
		S.AlongEnd = RT_GeometryQuanta;
		S.Layer = 0;
		return S;
	}

	/** Una riga di celle da `-Radius` a `+Radius` sull'asse `q`: il corridoio su cui si misura il passo. */
	URTHexMapAsset* StandCorridor(int32 Radius)
	{
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
		for (int32 Q = -Radius; Q <= Radius; ++Q)
		{
			FRTHexCellData Cell;
			Cell.Id = FRTCellId(Q, 0, 0);
			Cell.MoveCost = 1;
			Map->AddOrUpdateCell(Cell);
		}
		return Map;
	}

	int32 StandOccupiedWedges(const URTHexMapAsset* Map, const FRTCellId& Id)
	{
		TArray<FRTOccupancyPolyline> Geometry;
		for (const FRTHexInteriorWall& Wall : Map->InteriorWalls)
		{
			if (Wall.Cell == Id)
			{
				Geometry.Add(URTGeometryGrammarLibrary::ToPolyline(Wall.Segment, StandHexSize));
			}
		}
		const FRTOccupancyMask Mask = URTHexOccupancyLibrary::ComputeMask(Geometry, StandHexSize);
		int32 N = 0;
		for (int32 W = 0; W < RT_OccupancySectorCount; ++W)
		{
			if ((Mask.Sectors & (1 << W)) != 0) { ++N; }
		}
		return N;
	}
}

/**
 * 🔴 **UN MURO CHE ATTRAVERSA LA CELLA NON LA CHIUDE, E LO DICE IL PERCORSO.**
 *
 * E' il criterio n. 1 di `#1827`, e la sua formulazione e' precisa: *«un test sul percorso reale — non sul
 * predicato isolato»*. Un predicato verde che nessuno consulta e' esattamente lo stato da cui questa issue
 * parte: `HasLegalPlacement` era gia' corretto, e la cella restava impraticabile lo stesso.
 *
 * Il muro e' un diametro fra due vertici: attraversa la cella da parte a parte, occupa i quattro settori
 * che percorre — `D-306` — e lascia liberi gli altri otto in due regioni. Con il profilo identita' una posa
 * esiste, quindi il passo deve passare di li'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStandWallCrossesCellStillStandableTest,
	"RefactorTactics.Standability.WallCrossesCellStillStandable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStandWallCrossesCellStillStandableTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = StandCorridor(3);
	const FRTCellId Middle(0, 0, 0);

	// Un muro obliquo nel mezzo del corridoio.
	URTGeometryBakeLibrary::BakeCell(Map, Middle, { StandDiameter(ERTTacticalAxis::Deg90) }, StandHexSize);

	TestEqual(TEXT("il muro e' interno, non una copertura"), Map->InteriorWalls.Num(), 1);
	TestEqual(TEXT("e occupa i quattro settori che attraversa"), StandOccupiedWedges(Map, Middle), 4);

	const FRTHexCellData* Cell = Map->FindCell(Middle);
	if (!TestNotNull(TEXT("la cella esiste"), Cell)) { return false; }
	TestFalse(TEXT("la cella resta calpestabile"), Cell->bBlocksMovement);

	// LA PROVA SUL PERCORSO, che e' il criterio: non basta il predicato.
	const FRTHexPathResult Result =
		URTHexPathLibrary::FindPath(Map, FRTCellId(-3, 0, 0), FRTCellId(3, 0, 0), 999);
	TestTrue(TEXT("il percorso esiste"), Result.Status == ERTHexPathStatus::Success);
	TestTrue(TEXT("e attraversa la cella col muro"), Result.Path.Contains(Middle));

	return true;
}

/**
 * SENZA POSA LEGALE LA CELLA SI CHIUDE, e il passo la evita.
 *
 * 🔑 **Tre diametri bastano a chiudere tutti e dodici i settori**, ed e' aritmetica dei quattro che ciascuno
 * occupa: `Deg30` prende `{1,2,7,8}`, `Deg90` prende `{3,4,9,10}`, `Deg150` prende `{0,5,6,11}`. L'unione e'
 * l'intera cella, non resta una regione libera, e nessun footprint ci sta — nemmeno quello identita'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStandFootprintCollisionBlocksCellTest,
	"RefactorTactics.Standability.FootprintCollisionBlocksCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStandFootprintCollisionBlocksCellTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = StandCorridor(3);
	const FRTCellId Middle(0, 0, 0);

	URTGeometryBakeLibrary::BakeCell(Map, Middle, {
		StandDiameter(ERTTacticalAxis::Deg30),
		StandDiameter(ERTTacticalAxis::Deg90),
		StandDiameter(ERTTacticalAxis::Deg150)
	}, StandHexSize);

	TestEqual(TEXT("tutti e dodici i settori sono occupati"), StandOccupiedWedges(Map, Middle), 12);

	const FRTHexCellData* Cell = Map->FindCell(Middle);
	if (!TestNotNull(TEXT("la cella esiste"), Cell)) { return false; }
	TestTrue(TEXT("senza posa legale la cella non e' calpestabile"), Cell->bBlocksMovement);
	TestTrue(TEXT("e il blocco e' DERIVATO, non d'autore"), Cell->bMovementBlockGenerated);

	// Il corridoio e' una riga sola: chiusa la cella di mezzo, non esiste un altro modo di passare.
	const FRTHexPathResult Result =
		URTHexPathLibrary::FindPath(Map, FRTCellId(-3, 0, 0), FRTCellId(3, 0, 0), 999);
	TestTrue(TEXT("il passo non attraversa una cella senza posa"),
		Result.Status != ERTHexPathStatus::Success || !Result.Path.Contains(Middle));

	return true;
}

/**
 * 🔴 **OTTO SETTORI OCCUPATI E LA CELLA SI CAMMINA ANCORA** — la scorciatoia superata, misurata.
 *
 * `FRTOccupancyThresholds::BlockedFrom` diceva *«>= 6 settori occupati => Blocked»*, e `D-289` l'ha
 * superata. Qui due diametri ne occupano **otto**, due oltre quella soglia, e la cella resta calpestabile:
 * i quattro liberi formano regioni in cui il footprint ci sta.
 *
 * ⚠️ Il criterio dell'issue parla di *«sei settori»*; con la geometria che la grammatica esprime il caso
 * costruibile piu' vicino ne da' otto, che dimostra la stessa cosa **piu' in forte**: se contasse il numero,
 * a otto sarebbe chiusa da un pezzo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStandOccupiedWedgesDoNotDecideTest,
	"RefactorTactics.Standability.OccupiedWedgeCountDoesNotDecide",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStandOccupiedWedgesDoNotDecideTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = StandCorridor(2);
	const FRTCellId Middle(0, 0, 0);

	URTGeometryBakeLibrary::BakeCell(Map, Middle, {
		StandDiameter(ERTTacticalAxis::Deg30),
		StandDiameter(ERTTacticalAxis::Deg90)
	}, StandHexSize);

	const int32 Occupied = StandOccupiedWedges(Map, Middle);
	TestEqual(TEXT("otto settori occupati"), Occupied, 8);
	TestTrue(TEXT("cioe' oltre la vecchia soglia di sei"), Occupied >= 6);

	const FRTHexCellData* Cell = Map->FindCell(Middle);
	if (!TestNotNull(TEXT("la cella esiste"), Cell)) { return false; }
	TestFalse(TEXT("e la cella si cammina lo stesso: il conteggio non decide"), Cell->bBlocksMovement);

	return true;
}

/**
 * 🔑 **L'AUTORE VINCE SUL DERIVATO**, ed e' la meta' per cui il campo di provenienza esiste.
 *
 * La cottura non ha l'autorita' di contraddire una scelta di design. Una cella dichiarata impraticabile a
 * mano resta impraticabile anche quando la geometria lascerebbe posa — e il rebake non deve «ripulirla».
 *
 * ⚠️ E' la stessa disciplina di `FRTHexCover::bGenerated` (`D-131`): senza il campo, il bake dovrebbe
 * scegliere fra due difetti — non essere idempotente, oppure cancellare la scelta dell'autore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStandAuthoredBlockWinsTest,
	"RefactorTactics.Standability.AuthoredBlockWinsOverDerived",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStandAuthoredBlockWinsTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = StandCorridor(1);
	const FRTCellId Middle(0, 0, 0);

	// L'autore dichiara la cella impraticabile, senza alcuna geometria.
	{
		FRTHexCellData Cell;
		Cell.Id = Middle;
		Cell.MoveCost = 1;
		Cell.bBlocksMovement = true;
		Cell.bMovementBlockGenerated = false; // d'autore, ed e' il default
		Map->AddOrUpdateCell(Cell);
	}

	// Una cottura che lascerebbe posa legale non deve toglierlo.
	URTGeometryBakeLibrary::BakeCell(Map, Middle, { StandDiameter(ERTTacticalAxis::Deg90) }, StandHexSize);

	const FRTHexCellData* Cell = Map->FindCell(Middle);
	if (!TestNotNull(TEXT("la cella esiste"), Cell)) { return false; }
	TestTrue(TEXT("il blocco d'autore sopravvive alla cottura"), Cell->bBlocksMovement);
	TestFalse(TEXT("e resta d'autore"), Cell->bMovementBlockGenerated);

	return true;
}

/**
 * IL REBAKE E' IDEMPOTENTE, E TOGLIERE IL MURO RESTITUISCE LA CELLA.
 *
 * 🔑 E' la proprieta' che il campo di provenienza compra: il bake puo' togliere **il proprio** blocco. Senza,
 * rimuovere il muro che rendeva la cella impraticabile non la restituirebbe al gioco — e il difetto si
 * vedrebbe solo cancellando geometria, cioe' tardi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStandRebakeIsReversibleTest,
	"RefactorTactics.Standability.RebakeIsIdempotentAndReversible",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStandRebakeIsReversibleTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = StandCorridor(1);
	const FRTCellId Middle(0, 0, 0);

	const TArray<FRTGeometrySegment> Closing = {
		StandDiameter(ERTTacticalAxis::Deg30),
		StandDiameter(ERTTacticalAxis::Deg90),
		StandDiameter(ERTTacticalAxis::Deg150)
	};

	URTGeometryBakeLibrary::BakeCell(Map, Middle, Closing, StandHexSize);
	TestTrue(TEXT("chiusa dalla geometria"), Map->FindCell(Middle)->bBlocksMovement);

	// IDEMPOTENZA: la stessa cottura due volte non cambia niente.
	URTGeometryBakeLibrary::BakeCell(Map, Middle, Closing, StandHexSize);
	TestTrue(TEXT("il rebake identico la lascia chiusa"), Map->FindCell(Middle)->bBlocksMovement);
	TestEqual(TEXT("e non accumula muri"), Map->InteriorWalls.Num(), 3);

	// REVERSIBILITA': tolta la geometria, la cella torna praticabile.
	URTGeometryBakeLibrary::BakeCell(Map, Middle, {}, StandHexSize);
	TestEqual(TEXT("i muri sono spariti"), Map->InteriorWalls.Num(), 0);
	TestFalse(TEXT("e la cella e' tornata calpestabile"), Map->FindCell(Middle)->bBlocksMovement);
	TestFalse(TEXT("senza lasciare la provenienza accesa"), Map->FindCell(Middle)->bMovementBlockGenerated);

	return true;
}

/**
 * `ComputeHash` DISTINGUE DUE MAPPE CHE DIFFERISCONO SOLO PER LA CALPESTABILITA' DERIVATA.
 *
 * 🔑 **E non deve distinguere quelle che differiscono solo per la PROVENIENZA.** Sono le due meta' dello
 * stesso criterio — *«ci entra cio' che puo' cambiare un esito»* — e sbagliarne una e' un falso positivo
 * contro `replay divergence = 0`: due mappe che si giocano identiche avrebbero hash diversi solo perche' in
 * una il blocco e' stato cotto invece che dipinto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStandHashSeesStandabilityTest,
	"RefactorTactics.Standability.HashSeesStandabilityNotProvenance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStandHashSeesStandabilityTest::RunTest(const FString&)
{
	const FRTCellId Middle(0, 0, 0);
	const TArray<FRTGeometrySegment> Closing = {
		StandDiameter(ERTTacticalAxis::Deg30),
		StandDiameter(ERTTacticalAxis::Deg90),
		StandDiameter(ERTTacticalAxis::Deg150)
	};

	URTHexMapAsset* Open = StandCorridor(1);
	URTHexMapAsset* Closed = StandCorridor(1);
	URTGeometryBakeLibrary::BakeCell(Closed, Middle, Closing, StandHexSize);

	TestNotEqual(TEXT("la calpestabilita' cambia l'hash"), Open->ComputeHash(), Closed->ComputeHash());

	// ⛔ La PROVENIENZA no: stesse celle, stesso blocco, solo l'origine diversa.
	URTHexMapAsset* ByAuthor = StandCorridor(1);
	URTHexMapAsset* ByBake = StandCorridor(1);
	for (URTHexMapAsset* M : { ByAuthor, ByBake })
	{
		FRTHexCellData Cell = *M->FindCell(Middle);
		Cell.bBlocksMovement = true;
		Cell.bMovementBlockGenerated = (M == ByBake);
		M->AddOrUpdateCell(Cell);
	}
	TestEqual(TEXT("ma la provenienza non entra nell'hash"), ByAuthor->ComputeHash(), ByBake->ComputeHash());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
