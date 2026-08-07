#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Esagono pieno di raggio N sul layer 0, tutto visibile. Nome distinto per file (unity build). */
	URTHexMapAsset* MakeCoverMap(int32 Radius)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();
		return M;
	}

	/** Copertura bassa sul bordo indicato della cella (la cella deve esistere). */
	void SetLowCoverEdge(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		Data.Covers.Add(FRTHexCover(Edge));
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	FRTHexCombatUnit CoverUnit(int32 UnitId, int32 TeamId, const FRTCellId& Cell)
	{
		FRTHexCombatUnit U;
		U.UnitId = UnitId;
		U.TeamId = TeamId;
		U.Cell = Cell;
		U.bAlive = true;
		return U;
	}

	FRTHexAttackIntent CoverIntent(int32 AttackerId, int32 TargetId, ERTAbilityShape Shape,
		int32 RangeCells, int32 Power, int32 AreaRadius = 0)
	{
		FRTHexAttackIntent I;
		I.AttackerId = AttackerId;
		I.TargetId = TargetId;
		I.Shape = Shape;
		I.RangeCells = RangeCells;
		I.AreaRadius = AreaRadius;
		I.Power = Power;
		return I;
	}

	/** Danno arrivato al bersaglio indicato (-1 se non e' stato colpito). Il colpo a Power 0 resta un colpo. */
	int32 CoverPowerOn(const FRTHexBlastPlan& Plan, int32 TargetId)
	{
		for (const FRTHexAttackHit& Hit : Plan.Hits)
		{
			if (Hit.TargetId == TargetId) { return Hit.Power; }
		}
		return -1;
	}
}

/**
 * La copertura bassa sul bordo ATTRAVERSATO dal colpo riduce di 10 il danno diretto.
 *
 * Geometria: attaccante in (0,0), bersaglio in (2,0). La linea passa per (1,0), quindi il colpo entra nella
 * cella del bersaglio dal bordo W. Una copertura su quel bordo e' interposta; la stessa scena senza copertura
 * e' il controllo che il -10 venga dalla copertura e non da altro.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverDirectionalDamageReductionTest,
	"RefactorTactics.Cover.DirectionalDamageReduction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverDirectionalDamageReductionTest::RunTest(const FString&)
{
	TArray<FRTHexCombatUnit> Units;
	Units.Add(CoverUnit(0, 0, FRTCellId(0, 0)));
	Units.Add(CoverUnit(1, 1, FRTCellId(2, 0)));

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(CoverIntent(0, 1, ERTAbilityShape::Single, 5, 30));

	URTHexMapAsset* Bare = MakeCoverMap(3);
	const FRTHexBlastPlan Uncovered = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Bare);
	TestEqual(TEXT("senza copertura: danno pieno"), CoverPowerOn(Uncovered, 1), 30);

	URTHexMapAsset* Covered = MakeCoverMap(3);
	SetLowCoverEdge(Covered, FRTCellId(2, 0), ERTHexDirection::W); // bordo verso l'attaccante
	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Covered);

	TestEqual(TEXT("il colpo avviene comunque"), Plan.Hits.Num(), 1);
	TestEqual(TEXT("copertura bassa dal lato protetto: -10"),
		CoverPowerOn(Plan, 1), 30 - URTCombatLibrary::LowCoverDamageReduction);
	TestEqual(TEXT("la riduzione e' quella di catalogo"), URTCombatLibrary::LowCoverDamageReduction, 10);
	return true;
}

/**
 * Da un'altra direzione la stessa copertura e' inefficace: e' un riparo su UN bordo, non un bonus dell'unita'.
 * Verificato sia sul bordo opposto sia sui due bordi adiacenti a quello attraversato, dove l'errore piu'
 * probabile (confondere "adiacente" con "attraversato") si vedrebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverLowCoverWrongSideTest,
	"RefactorTactics.Cover.LowCover.WrongSideNoReduction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverLowCoverWrongSideTest::RunTest(const FString&)
{
	TArray<FRTHexCombatUnit> Units;
	Units.Add(CoverUnit(0, 0, FRTCellId(0, 0)));
	Units.Add(CoverUnit(1, 1, FRTCellId(2, 0)));

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(CoverIntent(0, 1, ERTAbilityShape::Single, 5, 30));

	// Il colpo entra da W: ogni altro bordo non e' sulla traiettoria.
	const ERTHexDirection WrongSides[] = { ERTHexDirection::E, ERTHexDirection::NW, ERTHexDirection::SW };
	for (const ERTHexDirection Edge : WrongSides)
	{
		URTHexMapAsset* Map = MakeCoverMap(3);
		SetLowCoverEdge(Map, FRTCellId(2, 0), Edge);
		const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
		TestEqual(FString::Printf(TEXT("bordo %d non attraversato: danno pieno"), static_cast<int32>(Edge)),
			CoverPowerOn(Plan, 1), 30);
	}

	// Copertura sulla cella SBAGLIATA (quella dell'attaccante, sul bordo verso il bersaglio): non protegge
	// chi la subisce. La copertura appartiene alla cella che ripara, non alla traiettoria.
	URTHexMapAsset* Attackers = MakeCoverMap(3);
	SetLowCoverEdge(Attackers, FRTCellId(0, 0), ERTHexDirection::E);
	TestEqual(TEXT("copertura sulla cella dell'attaccante: danno pieno"),
		CoverPowerOn(URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Attackers), 1), 30);
	return true;
}

/**
 * L'area non e' un proiettile: la copertura bassa non ne ripara, nemmeno quando il centro sta dal lato
 * protetto — il caso in cui la protezione sembrerebbe dovuta. E' il confine dichiarato della regola: se il
 * centro fosse dall'altro lato la copertura non sarebbe comunque interposta, quindi "AoE mai ridotto".
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverLowCoverAoESameSideTest,
	"RefactorTactics.Cover.LowCover.AoESameSide",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverLowCoverAoESameSideTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeCoverMap(3);
	SetLowCoverEdge(Map, FRTCellId(2, 0), ERTHexDirection::W); // bordo verso il centro dell'area

	TArray<FRTHexCombatUnit> Units;
	Units.Add(CoverUnit(0, 0, FRTCellId(0, 0)));
	Units.Add(CoverUnit(1, 1, FRTCellId(1, 0)));  // centro dell'area, dal lato protetto
	Units.Add(CoverUnit(2, 1, FRTCellId(2, 0)));  // riparato sul bordo W, preso dal raggio

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(CoverIntent(0, 1, ERTAbilityShape::Area, 5, 30, /*AreaRadius*/ 1));

	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);

	TestEqual(TEXT("l'area colpisce entrambi"), Plan.Hits.Num(), 2);
	TestEqual(TEXT("centro dell'area: danno pieno"), CoverPowerOn(Plan, 1), 30);
	TestEqual(TEXT("copertura dal lato del centro: nessuna riduzione"), CoverPowerOn(Plan, 2), 30);
	return true;
}

/**
 * Un colpo piu' debole della copertura non cura: il danno si ferma a 0 e il colpo resta AVVENUTO (stessa
 * disciplina di `Deflect`, dove il clamp e' sul valore e non sulla voce: trigger e marchi contano lo stesso).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverLowCoverClampTest,
	"RefactorTactics.Cover.LowCover.NeverHealsTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverLowCoverClampTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeCoverMap(3);
	SetLowCoverEdge(Map, FRTCellId(2, 0), ERTHexDirection::W);

	TArray<FRTHexCombatUnit> Units;
	Units.Add(CoverUnit(0, 0, FRTCellId(0, 0)));
	Units.Add(CoverUnit(1, 1, FRTCellId(2, 0)));

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(CoverIntent(0, 1, ERTAbilityShape::Single, 5, 4)); // 4 - 10 sarebbe negativo

	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
	TestEqual(TEXT("il colpo resta nel piano"), Plan.Hits.Num(), 1);
	TestEqual(TEXT("danno azzerato, mai negativo"), CoverPowerOn(Plan, 1), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
