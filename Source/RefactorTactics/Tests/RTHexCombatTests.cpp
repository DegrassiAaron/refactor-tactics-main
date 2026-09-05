#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Algo/Reverse.h"
#include "Ability/RTActionData.h"
#include "Combat/RTCombatResolver.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Esagono pieno di raggio N sul layer 0, tutto visibile. Nome distinto per file (unity build). */
	URTHexMapAsset* MakeCombatMap(int32 Radius)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		return M;
	}

	/** Marca una cella come ostacolo alla vista (creandola se assente). */
	void SetCombatSightBlocker(URTHexMapAsset* Map, const FRTCellId& Id)
	{
		FRTHexCellData Data = Map->FindCell(Id) ? *Map->FindCell(Id) : FRTHexCellData(Id);
		Data.Id = Id;
		Data.bBlocksLineOfSight = true;
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	/** Unita' di combattimento: l'UnitId coincide con l'indice nell'array (chiave di risultato). */
	FRTHexCombatUnit CombatUnit(int32 UnitId, int32 TeamId, const FRTCellId& Cell, bool bAlive = true)
	{
		FRTHexCombatUnit U;
		U.UnitId = UnitId;
		U.TeamId = TeamId;
		U.Cell = Cell;
		U.bAlive = bAlive;
		return U;
	}

	FRTHexAttackIntent CombatIntent(int32 AttackerId, int32 TargetId, ERTAbilityShape Shape,
		int32 RangeCells, int32 Power, int32 AreaRadius = 0)
	{
		FRTHexAttackIntent I;
		I.AttackerId = AttackerId;
		I.TargetId = TargetId;
		I.Shape = Shape;
		I.RangeCells = RangeCells;
		I.AreaRadius = AreaRadius;
		I.Power = Power;
		// Questo helper costruisce intenti d'ATTACCO, e da [`INT-8`] un attacco deve dichiararsi: con
		// `bCountsAsAttack` a `false` di default, ometterlo qui renderebbe muti sette test che parlano di
		// colpi. Chi vuole misurare una NON aggressione lo rimette a `false` sull'intento, come fa
		// `HexCombat.NonAttackProducesNoHit`.
		I.bCountsAsAttack = true;
		return I;
	}

	/** Vero se il piano contiene un colpo Attacker -> Target. */
	bool PlanHits(const FRTHexBlastPlan& Plan, int32 AttackerId, int32 TargetId)
	{
		for (const FRTHexAttackHit& Hit : Plan.Hits)
		{
			if (Hit.AttackerId == AttackerId && Hit.TargetId == TargetId) { return true; }
		}
		return false;
	}
}

// ---------------------------------------------------------------------------------------------------------
// Forme: ogni forma dell'abilita' si risolve sulle primitive esagonali
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexCombatShapeSingleTest,
	"RefactorTactics.HexCombat.ShapeSingle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexCombatShapeSingleTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeCombatMap(4);

	// Attaccante in (0,0); bersaglio in (2,0); un secondo nemico adiacente al bersaglio in (3,0).
	TArray<FRTHexCombatUnit> Units;
	Units.Add(CombatUnit(0, 0, FRTCellId(0, 0)));
	Units.Add(CombatUnit(1, 1, FRTCellId(2, 0)));
	Units.Add(CombatUnit(2, 1, FRTCellId(3, 0)));

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(CombatIntent(0, 1, ERTAbilityShape::Single, 5, 30));

	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);

	TestEqual(TEXT("Single colpisce un solo bersaglio"), Plan.Hits.Num(), 1);
	TestTrue(TEXT("colpisce il bersaglio designato"), PlanHits(Plan, 0, 1));
	TestFalse(TEXT("non colpisce il vicino del bersaglio"), PlanHits(Plan, 0, 2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexCombatShapeAreaTest,
	"RefactorTactics.HexCombat.ShapeArea",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexCombatShapeAreaTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeCombatMap(4);

	// Area raggio 1 attorno al bersaglio: prende il bersaglio e i suoi 6 vicini.
	TArray<FRTHexCombatUnit> Units;
	Units.Add(CombatUnit(0, 0, FRTCellId(0, 0)));
	Units.Add(CombatUnit(1, 1, FRTCellId(2, 0)));                                   // bersaglio
	Units.Add(CombatUnit(2, 1, URTHexLibrary::Neighbor(FRTCellId(2, 0), ERTHexDirection::NE))); // vicino: colpito
	Units.Add(CombatUnit(3, 1, FRTCellId(4, 0)));                                   // distante 2: illeso

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(CombatIntent(0, 1, ERTAbilityShape::Area, 5, 20, /*AreaRadius*/ 1));

	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);

	TestTrue(TEXT("colpisce il bersaglio"), PlanHits(Plan, 0, 1));
	TestTrue(TEXT("colpisce il nemico adiacente al bersaglio"), PlanHits(Plan, 0, 2));
	TestFalse(TEXT("non colpisce il nemico fuori area"), PlanHits(Plan, 0, 3));
	TestEqual(TEXT("due colpi in tutto"), Plan.Hits.Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexCombatShapeLineTest,
	"RefactorTactics.HexCombat.ShapeLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexCombatShapeLineTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeCombatMap(5);

	// Linea (0,0) -> (3,0): attraversa (1,0) e (2,0). Un nemico e' sulla traiettoria, uno fuori.
	TArray<FRTHexCombatUnit> Units;
	Units.Add(CombatUnit(0, 0, FRTCellId(0, 0)));
	Units.Add(CombatUnit(1, 1, FRTCellId(3, 0)));  // bersaglio
	Units.Add(CombatUnit(2, 1, FRTCellId(2, 0)));  // sulla linea: colpito di striscio
	Units.Add(CombatUnit(3, 1, FRTCellId(2, 1)));  // fuori linea: illeso

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(CombatIntent(0, 1, ERTAbilityShape::Line, 5, 25));

	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);

	TestTrue(TEXT("colpisce il bersaglio a fine linea"), PlanHits(Plan, 0, 1));
	TestTrue(TEXT("colpisce chi sta sulla traiettoria"), PlanHits(Plan, 0, 2));
	TestFalse(TEXT("non colpisce chi e' fuori dalla linea"), PlanHits(Plan, 0, 3));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexCombatShapeConeTest,
	"RefactorTactics.HexCombat.ShapeCone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexCombatShapeConeTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeCombatMap(5);

	// Cono da (0,0) verso E: il ventaglio a 120 grandi prende 3 celle a distanza 1.
	const FRTCellId From(0, 0);
	const TArray<FRTCellId> Fan = URTHexLibrary::HexCone(From, FRTCellId(2, 0), 2);
	TestTrue(TEXT("il ventaglio non e' vuoto"), Fan.Num() > 0);

	TArray<FRTHexCombatUnit> Units;
	Units.Add(CombatUnit(0, 0, From));
	Units.Add(CombatUnit(1, 1, FRTCellId(2, 0)));                     // bersaglio, nel cono
	Units.Add(CombatUnit(2, 1, Fan.Num() > 0 ? Fan[0] : FRTCellId(1, 0))); // altra cella del ventaglio
	Units.Add(CombatUnit(3, 1, URTHexLibrary::Neighbor(From, ERTHexDirection::W))); // dietro l'attaccante

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(CombatIntent(0, 1, ERTAbilityShape::Cone, 2, 15));

	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);

	TestTrue(TEXT("colpisce il bersaglio nel cono"), PlanHits(Plan, 0, 1));
	TestTrue(TEXT("colpisce un secondo nemico nel ventaglio"), PlanHits(Plan, 0, 2));
	TestFalse(TEXT("non colpisce alle spalle"), PlanHits(Plan, 0, 3));
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Identita' dell'azione: un colpo nasce solo da cio' che si DICHIARA aggressione (#1491, `INT-8`)
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexCombatNonAttackProducesNoHitTest,
	"RefactorTactics.HexCombat.NonAttackProducesNoHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexCombatNonAttackProducesNoHitTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeCombatMap(4);

	// Adiacenti, a portata e con danno zero: la forma esatta di `Action.Interact` puntata su un'unita'.
	TArray<FRTHexCombatUnit> Units;
	Units.Add(CombatUnit(0, 0, FRTCellId(0, 0)));
	Units.Add(CombatUnit(1, 1, FRTCellId(1, 0)));

	FRTHexAttackIntent Silenziosa = CombatIntent(0, 1, ERTAbilityShape::Single, /*Range*/ 1, /*Power*/ 0);
	Silenziosa.bCountsAsAttack = false;

	// ANTI-VACUITA': lo STESSO intento, dichiarato aggressione, deve produrre il colpo. Senza questa meta'
	// il test sarebbe verde anche se a impedire il colpo fosse la geometria invece del flag -- e l'unica
	// differenza fra i due rami e' quel bool, non una cella, non una portata, non una forma.
	FRTHexAttackIntent Dichiarata = Silenziosa;
	Dichiarata.bCountsAsAttack = true;

	TArray<FRTHexAttackIntent> SoloSilenziosa; SoloSilenziosa.Add(Silenziosa);
	TArray<FRTHexAttackIntent> SoloDichiarata; SoloDichiarata.Add(Dichiarata);

	TestEqual(TEXT("cio' che non si dichiara aggressione non produce colpi"),
		URTHexCombatLibrary::CollectHexAttacks(Units, SoloSilenziosa, Map).Hits.Num(), 0);
	TestEqual(TEXT("lo stesso intento, dichiarato aggressione, ne produce uno"),
		URTHexCombatLibrary::CollectHexAttacks(Units, SoloDichiarata, Map).Hits.Num(), 1);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Validazione: portata, linea di tiro, bersagli ammessi
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexCombatOutOfRangeTest,
	"RefactorTactics.HexCombat.OutOfRangeRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexCombatOutOfRangeTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeCombatMap(6);

	TArray<FRTHexCombatUnit> Units;
	Units.Add(CombatUnit(0, 0, FRTCellId(0, 0)));
	Units.Add(CombatUnit(1, 1, FRTCellId(5, 0)));  // distanza 5

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(CombatIntent(0, 1, ERTAbilityShape::Single, /*Range*/ 3, 30));

	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);

	TestEqual(TEXT("fuori portata: nessun colpo"), Plan.Hits.Num(), 0);
	TestEqual(TEXT("fuori portata non e' un problema di LOS"), Plan.BlockedIntents.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexCombatBlockedLineOfSightTest,
	"RefactorTactics.HexCombat.BlockedLineOfSight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexCombatBlockedLineOfSightTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeCombatMap(4);
	SetCombatSightBlocker(Map, FRTCellId(1, 0)); // muro tra (0,0) e (2,0)

	TArray<FRTHexCombatUnit> Units;
	Units.Add(CombatUnit(0, 0, FRTCellId(0, 0)));
	Units.Add(CombatUnit(1, 1, FRTCellId(2, 0)));

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(CombatIntent(0, 1, ERTAbilityShape::Single, 5, 30));

	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);

	TestEqual(TEXT("linea di tiro bloccata: nessun colpo"), Plan.Hits.Num(), 0);
	TestEqual(TEXT("l'intento e' registrato come senza linea di tiro"), Plan.BlockedIntents.Num(), 1);
	TestTrue(TEXT("indice dell'intento bloccato"), Plan.BlockedIntents.Num() == 1 && Plan.BlockedIntents[0] == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexCombatNoMapFailClosedTest,
	"RefactorTactics.HexCombat.NoMapFailClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexCombatNoMapFailClosedTest::RunTest(const FString&)
{
	// Senza mappa autorevole non si valida la linea di tiro: nessun attacco (stessa scelta di CanTargetHexCell).
	TArray<FRTHexCombatUnit> Units;
	Units.Add(CombatUnit(0, 0, FRTCellId(0, 0)));
	Units.Add(CombatUnit(1, 1, FRTCellId(1, 0)));

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(CombatIntent(0, 1, ERTAbilityShape::Single, 5, 30));

	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, nullptr);

	TestEqual(TEXT("mappa assente: nessun colpo"), Plan.Hits.Num(), 0);

	// Il MOTIVO deve restare distinto: «non valutabile» non e' «coperto». Confonderli farebbe scrivere nel
	// TurnLog un esito di gioco (NoLineOfSight) al posto di un errore di configurazione del livello — lo
	// stesso difetto corretto nel controller con ClassifyHexTargeting.
	TestEqual(TEXT("mappa assente: NON e' un blocco della linea di tiro"), Plan.BlockedIntents.Num(), 0);
	TestEqual(TEXT("mappa assente: registrato come non valutabile"), Plan.UnverifiableIntents.Num(), 1);
	TestTrue(TEXT("indica QUALE intento"),
		Plan.UnverifiableIntents.Num() == 1 && Plan.UnverifiableIntents[0] == 0);

	// Con la mappa, un muro produce invece un blocco vero della linea di tiro (e nessun «non valutabile»).
	URTHexMapAsset* Map = MakeCombatMap(4);
	SetCombatSightBlocker(Map, FRTCellId(1, 0));
	TArray<FRTHexCombatUnit> Far;
	Far.Add(CombatUnit(0, 0, FRTCellId(0, 0)));
	Far.Add(CombatUnit(1, 1, FRTCellId(2, 0)));
	const FRTHexBlastPlan Blocked = URTHexCombatLibrary::CollectHexAttacks(Far, Intents, Map);
	TestEqual(TEXT("muro: e' un blocco della linea di tiro"), Blocked.BlockedIntents.Num(), 1);
	TestEqual(TEXT("muro: nulla di 'non valutabile'"), Blocked.UnverifiableIntents.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexCombatFriendlyFireTest,
	"RefactorTactics.HexCombat.NoFriendlyFire",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexCombatFriendlyFireTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeCombatMap(4);

	// Un alleato dell'attaccante sta nell'area colpita: non viene colpito.
	TArray<FRTHexCombatUnit> Units;
	Units.Add(CombatUnit(0, 0, FRTCellId(0, 0)));
	Units.Add(CombatUnit(1, 1, FRTCellId(2, 0)));                                   // bersaglio
	Units.Add(CombatUnit(2, 0, URTHexLibrary::Neighbor(FRTCellId(2, 0), ERTHexDirection::NE))); // alleato nell'area
	Units.Add(CombatUnit(3, 1, URTHexLibrary::Neighbor(FRTCellId(2, 0), ERTHexDirection::SE), /*bAlive*/ false)); // morto

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(CombatIntent(0, 1, ERTAbilityShape::Area, 5, 20, /*AreaRadius*/ 1));

	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);

	TestTrue(TEXT("colpisce il nemico"), PlanHits(Plan, 0, 1));
	TestFalse(TEXT("non colpisce l'alleato nell'area"), PlanHits(Plan, 0, 2));
	TestFalse(TEXT("non colpisce un'unita' gia' morta"), PlanHits(Plan, 0, 3));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexCombatDeadAttackerTest,
	"RefactorTactics.HexCombat.DeadAttackerIgnored",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexCombatDeadAttackerTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeCombatMap(4);

	// Attaccante gia' morto PRIMA della fase (non "morto dal fuoco simultaneo"): non attacca.
	TArray<FRTHexCombatUnit> Units;
	Units.Add(CombatUnit(0, 0, FRTCellId(0, 0), /*bAlive*/ false));
	Units.Add(CombatUnit(1, 1, FRTCellId(2, 0)));

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(CombatIntent(0, 1, ERTAbilityShape::Single, 5, 30));

	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);

	TestEqual(TEXT("un'unita' morta non attacca"), Plan.Hits.Num(), 0);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Spinta (knockback) su sei direzioni
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexKnockbackPushesAwayTest,
	"RefactorTactics.HexCombat.KnockbackPushesAway",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexKnockbackPushesAwayTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeCombatMap(6);
	const TArray<FRTCellId> NoOccupants;

	// Spinta lungo la direzione attaccante -> bersaglio, per Distance celle adiacenti.
	const FRTCellId Dest = URTHexCombatLibrary::HexKnockbackDestination(
		FRTCellId(0, 0), FRTCellId(1, 0), /*Distance*/ 2, Map, NoOccupants);
	TestTrue(TEXT("respinto di due celle lungo la direzione del colpo"), Dest == FRTCellId(3, 0));

	// Direzione obliqua: la spinta segue il passo esagonale, non un asse cardinale.
	const FRTCellId Oblique = URTHexCombatLibrary::HexKnockbackDestination(
		FRTCellId(0, 0), FRTCellId(1, -1), /*Distance*/ 1, Map, NoOccupants);
	TestTrue(TEXT("la spinta obliqua resta su celle adiacenti"),
		URTHexLibrary::HexDistance(Oblique, FRTCellId(1, -1)) == 1);
	TestTrue(TEXT("la spinta allontana dall'attaccante"),
		URTHexLibrary::HexDistance(FRTCellId(0, 0), Oblique) == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexKnockbackBlockedTest,
	"RefactorTactics.HexCombat.KnockbackStopsBeforeObstacle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexKnockbackBlockedTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeCombatMap(6);
	{
		FRTHexCellData Wall(FRTCellId(3, 0));
		Wall.bBlocksMovement = true;
		Map->AddOrUpdateCell(Wall);
		Map->SortCells();
	}

	// Ostacolo a (3,0): la spinta si ferma sulla cella libera precedente.
	const FRTCellId Dest = URTHexCombatLibrary::HexKnockbackDestination(
		FRTCellId(0, 0), FRTCellId(1, 0), /*Distance*/ 3, Map, TArray<FRTCellId>());
	TestTrue(TEXT("si ferma prima dell'ostacolo"), Dest == FRTCellId(2, 0));

	// Unita' ferma a (2,0): non si spinge dentro un'altra unita'.
	TArray<FRTCellId> Occupied;
	Occupied.Add(FRTCellId(2, 0));
	const FRTCellId Blocked = URTHexCombatLibrary::HexKnockbackDestination(
		FRTCellId(0, 0), FRTCellId(1, 0), /*Distance*/ 2, Map, Occupied);
	TestTrue(TEXT("cella occupata: il bersaglio resta dov'e'"), Blocked == FRTCellId(1, 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexKnockbackEdgeTest,
	"RefactorTactics.HexCombat.KnockbackStopsAtMapEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexKnockbackEdgeTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeCombatMap(3); // il bordo della mappa e' a distanza 3 dall'origine
	const TArray<FRTCellId> NoOccupants;

	// Oltre il bordo non esistono celle: la spinta si ferma, non spinge nel vuoto.
	const FRTCellId Dest = URTHexCombatLibrary::HexKnockbackDestination(
		FRTCellId(1, 0), FRTCellId(3, 0), /*Distance*/ 2, Map, NoOccupants);
	TestTrue(TEXT("il bersaglio resta sul bordo"), Dest == FRTCellId(3, 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexKnockbackNoPushTest,
	"RefactorTactics.HexCombat.KnockbackNoPushCases",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexKnockbackNoPushTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeCombatMap(5);
	const TArray<FRTCellId> NoOccupants;
	const FRTCellId Target(2, 0);

	TestTrue(TEXT("distanza 0: nessuna spinta"),
		URTHexCombatLibrary::HexKnockbackDestination(FRTCellId(0, 0), Target, 0, Map, NoOccupants) == Target);
	TestTrue(TEXT("attaccante sulla stessa cella: nessuna direzione, nessuna spinta"),
		URTHexCombatLibrary::HexKnockbackDestination(Target, Target, 2, Map, NoOccupants) == Target);
	TestTrue(TEXT("senza mappa autorevole non si spinge (fail-closed)"),
		URTHexCombatLibrary::HexKnockbackDestination(FRTCellId(0, 0), Target, 2, nullptr, NoOccupants) == Target);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexKnockbackLayerTest,
	"RefactorTactics.HexCombat.KnockbackPreservesLayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexKnockbackLayerTest::RunTest(const FString&)
{
	// Piattaforma sul layer 1: la spinta resta sul piano del bersaglio (i layer si collegano con archi,
	// non con la spinta).
	URTHexMapAsset* Map = MakeCombatMap(4);
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 1), 4))
	{
		Map->AddOrUpdateCell(FRTHexCellData(Id));
	}
	Map->SortCells();

	const FRTCellId Dest = URTHexCombatLibrary::HexKnockbackDestination(
		FRTCellId(0, 0, 1), FRTCellId(1, 0, 1), /*Distance*/ 2, Map, TArray<FRTCellId>());
	TestTrue(TEXT("la spinta resta sul layer del bersaglio"), Dest == FRTCellId(3, 0, 1));
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Ordine-indipendenza: "raccogli poi applica" (invariante #3)
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexCombatPermutationTest,
	"RefactorTactics.HexCombat.PermutationInvariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexCombatPermutationTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeCombatMap(5);

	// Due contro due che si colpiscono a vicenda nello stesso Blast, uno dei quali letale.
	TArray<FRTHexCombatUnit> Units;
	Units.Add(CombatUnit(0, 0, FRTCellId(0, 0)));
	Units.Add(CombatUnit(1, 0, FRTCellId(0, 1)));
	Units.Add(CombatUnit(2, 1, FRTCellId(2, 0)));
	Units.Add(CombatUnit(3, 1, FRTCellId(2, 1)));

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(CombatIntent(0, 2, ERTAbilityShape::Single, 5, 60));
	Intents.Add(CombatIntent(1, 2, ERTAbilityShape::Single, 5, 60)); // focus-fire letale su 2
	Intents.Add(CombatIntent(2, 0, ERTAbilityShape::Single, 5, 40)); // il morente colpisce comunque
	Intents.Add(CombatIntent(3, 1, ERTAbilityShape::Area, 5, 30, 1));

	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);

	// Ordine invertito degli intenti: il piano canonico deve essere identico, colpo per colpo.
	TArray<FRTHexAttackIntent> Reversed = Intents;
	Algo::Reverse(Reversed);
	const FRTHexBlastPlan PlanReversed = URTHexCombatLibrary::CollectHexAttacks(Units, Reversed, Map);

	TestEqual(TEXT("stesso numero di colpi"), PlanReversed.Hits.Num(), Plan.Hits.Num());
	bool bSame = PlanReversed.Hits.Num() == Plan.Hits.Num();
	for (int32 i = 0; bSame && i < Plan.Hits.Num(); ++i)
	{
		bSame = Plan.Hits[i].AttackerId == PlanReversed.Hits[i].AttackerId
			&& Plan.Hits[i].TargetId == PlanReversed.Hits[i].TargetId
			&& Plan.Hits[i].Power == PlanReversed.Hits[i].Power;
	}
	TestTrue(TEXT("il piano e' identico permutando l'ordine degli intenti"), bSame);

	// E lo stato risultante coincide: il resolver applica sullo snapshot iniziale.
	TArray<FRTUnitCombatState> States;
	for (int32 i = 0; i < Units.Num(); ++i) { States.Add(FRTUnitCombatState(100, 0)); }

	const TArray<FRTUnitCombatState> A = URTCombatResolver::ResolveAttacks(States, URTHexCombatLibrary::ToAttacks(Plan));
	const TArray<FRTUnitCombatState> B = URTCombatResolver::ResolveAttacks(States, URTHexCombatLibrary::ToAttacks(PlanReversed));

	bool bSameState = A.Num() == B.Num();
	for (int32 i = 0; bSameState && i < A.Num(); ++i)
	{
		bSameState = A[i].Health == B[i].Health && A[i].Shield == B[i].Shield;
	}
	TestTrue(TEXT("stato finale invariante rispetto all'ordine"), bSameState);
	TestTrue(TEXT("il bersaglio focalizzato e' abbattuto"), A.IsValidIndex(2) && A[2].Health <= 0);
	// L'unita' 0 incassa 40 dal morente (2) e 30 dall'area di (3) centrata su (0,1), di cui (0,0) e' vicina:
	// il colpo di chi muore nello stesso Blast va comunque a segno (snapshot iniziale, invariante #3).
	TestTrue(TEXT("chi muore infligge comunque il proprio danno"), A.IsValidIndex(0) && A[0].Health == 30);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexAttackOnCellTest,
	"RefactorTactics.HexCombat.AttackTargetsCellWithoutUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexAttackOnCellTest::RunTest(const FString&)
{
	// Quel che serve a `Fallback.AttackCell`: un intento puo' mirare a una CELLA (TargetId = INDEX_NONE), e
	// l'area colpisce chi ci si trova. Senza, il fallback degli AoE non avrebbe dove far partire il colpo.
	URTHexMapAsset* Map = MakeCombatMap(6);

	TArray<FRTHexCombatUnit> Units;
	FRTHexCombatUnit Attacker; Attacker.UnitId = 0; Attacker.TeamId = 0; Attacker.Cell = FRTCellId(0, 0);
	FRTHexCombatUnit Victim;   Victim.UnitId = 1;   Victim.TeamId = 1;   Victim.Cell = FRTCellId(3, 0);
	FRTHexCombatUnit Friend;   Friend.UnitId = 2;   Friend.TeamId = 0;   Friend.Cell = FRTCellId(2, 0);
	Units.Add(Attacker); Units.Add(Victim); Units.Add(Friend);

	FRTHexAttackIntent OnCell;
	OnCell.AttackerId = 0;
	OnCell.TargetId = INDEX_NONE;      // il bersaglio non c'e' piu': si mira dove era stato puntato
	OnCell.TargetCell = FRTCellId(3, 0);
	OnCell.Shape = ERTAbilityShape::Area;
	OnCell.AreaRadius = 1;
	OnCell.RangeCells = 5;
	OnCell.Power = 20;
	OnCell.bCountsAsAttack = true; // intento d'attacco, e da [`INT-8`] va dichiarato

	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, { OnCell }, Map);
	TestEqual(TEXT("l'area colpisce chi si trova sulla cella mirata"), Plan.Hits.Num(), 1);
	TestTrue(TEXT("e colpisce il nemico, non l'alleato adiacente"),
		Plan.Hits.Num() == 1 && Plan.Hits[0].TargetId == 1);

	// La cella mirata resta soggetta alla portata: non e' una scorciatoia per sparare piu' lontano.
	FRTHexAttackIntent TooFar = OnCell;
	TooFar.RangeCells = 2;
	TestEqual(TEXT("cella fuori portata: nessun colpo"),
		URTHexCombatLibrary::CollectHexAttacks(Units, { TooFar }, Map).Hits.Num(), 0);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// L'IMPRONTA A TERRA DEL COLPO ([D-301], #1945)
//
// Il piano portava gia' `Hits`, che sono per VITTIMA. Queste prove riguardano cio' che gli `Hits` non
// possono dire: dove il colpo e' arrivato quando non ha preso nessuno, e quante volte un'area e' avvenuta.
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFootprintCarriesResolvedCellsTest,
	"RefactorTactics.HexCombat.FootprintCarriesResolvedCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFootprintCarriesResolvedCellsTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeCombatMap(4);

	TArray<FRTHexCombatUnit> Units;
	Units.Add(CombatUnit(0, 0, FRTCellId(0, 0)));
	Units.Add(CombatUnit(1, 1, FRTCellId(2, 0)));

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(CombatIntent(0, 1, ERTAbilityShape::Area, 5, 20, /*AreaRadius*/ 1));

	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);

	TestEqual(TEXT("un intento aggressivo produce un'impronta"), Plan.Footprints.Num(), 1);
	if (Plan.Footprints.Num() != 1)
	{
		AddError(TEXT("senza l'impronta le assertion seguenti non direbbero nulla"));
		return true;
	}

	// Il confronto e' fra DUE PRODUZIONI, non contro una costante scritta a mano: se `HexHitCells`
	// cambiasse forma, un elenco copiato qui resterebbe verde mentre il footprint mente. Cosi' invece le
	// due cadono insieme, che e' l'unico modo in cui questa assertion vale qualcosa.
	const TArray<FRTCellId> Attese = URTHexCombatLibrary::HexHitCells(
		ERTAbilityShape::Area, FRTCellId(0, 0), FRTCellId(2, 0), /*RangeCells*/ 5, /*AreaRadius*/ 1);

	TestEqual(TEXT("l'impronta porta le stesse celle che il resolver ha usato"),
		Plan.Footprints[0].HitCells, Attese);
	TestTrue(TEXT("la forma e' quella dichiarata dall'intento"),
		Plan.Footprints[0].Shape == ERTAbilityShape::Area);
	TestTrue(TEXT("l'origine e' la cella dell'attaccante al momento del calcolo"),
		Plan.Footprints[0].Origin == FRTCellId(0, 0));
	TestTrue(TEXT("la cella mirata sopravvive nell'impronta"),
		Plan.Footprints[0].AimCell == FRTCellId(2, 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFootprintOnEmptyCellsTest,
	"RefactorTactics.HexCombat.FootprintSurvivesOnEmptyCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFootprintOnEmptyCellsTest::RunTest(const FString&)
{
	// E' il caso per cui #1945 esiste. Un'area su celle vuote non produce nessun `Hit`, quindi fino a
	// [D-301] non lasciava traccia di essere avvenuta: la presentazione non aveva nulla da disegnare, e
	// l'unico modo di mostrarla sarebbe stato ricalcolare `HexHitCells` fuori dal resolver.
	URTHexMapAsset* Map = MakeCombatMap(4);

	TArray<FRTHexCombatUnit> Units;
	Units.Add(CombatUnit(0, 0, FRTCellId(0, 0)));

	FRTHexAttackIntent SullaCellaVuota = CombatIntent(0, INDEX_NONE, ERTAbilityShape::Area, 5, 20, 1);
	SullaCellaVuota.TargetCell = FRTCellId(2, 0); // nessuna unita' ci sta sopra

	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, { SullaCellaVuota }, Map);

	TestEqual(TEXT("nessuna unita' colpita"), Plan.Hits.Num(), 0);
	TestEqual(TEXT("ma l'area e' avvenuta, e l'impronta lo dice"), Plan.Footprints.Num(), 1);
	if (Plan.Footprints.Num() == 1)
	{
		TestTrue(TEXT("l'impronta non e' vuota: le celle investite ci sono"),
			Plan.Footprints[0].HitCells.Num() > 0);
		TestTrue(TEXT("la cella mirata e' fra quelle investite"),
			Plan.Footprints[0].HitCells.Contains(FRTCellId(2, 0)));
	}
	else
	{
		AddError(TEXT("nessuna impronta: il caso che questa issue chiude e' ancora aperto"));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFootprintOnePerIntentTest,
	"RefactorTactics.HexCombat.FootprintIsOnePerIntentNotPerVictim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFootprintOnePerIntentTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeCombatMap(4);

	// Due nemici dentro la stessa area: due colpi, UNA azione.
	TArray<FRTHexCombatUnit> Units;
	Units.Add(CombatUnit(0, 0, FRTCellId(0, 0)));
	Units.Add(CombatUnit(1, 1, FRTCellId(2, 0)));
	Units.Add(CombatUnit(2, 1, URTHexLibrary::Neighbor(FRTCellId(2, 0), ERTHexDirection::NE)));

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(CombatIntent(0, 1, ERTAbilityShape::Area, 5, 20, /*AreaRadius*/ 1));

	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);

	TestEqual(TEXT("due bersagli dentro l'area, due colpi"), Plan.Hits.Num(), 2);
	// La distinzione che l'evento `AttackFootprint` esiste per tenere: se l'impronta fosse un campo del
	// colpo, qui ne uscirebbero due identiche e chi disegna dovrebbe deduplicare - logica nella
	// presentazione, che [D-278] vieta.
	TestEqual(TEXT("ma UNA sola impronta: e' un fatto dell'azione, non del bersaglio"),
		Plan.Footprints.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFootprintShapeIsDeclaredTest,
	"RefactorTactics.HexCombat.FootprintShapeIsDeclaredNotDeduced",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFootprintShapeIsDeclaredTest::RunTest(const FString&)
{
	// Un `Single` e un'`Area` di raggio 0 investono la STESSA cella: a valle sono due disegni diversi, e
	// solo la forma dichiarata li distingue. Contare le celle non basterebbe.
	URTHexMapAsset* Map = MakeCombatMap(4);

	TArray<FRTHexCombatUnit> Units;
	Units.Add(CombatUnit(0, 0, FRTCellId(0, 0)));
	Units.Add(CombatUnit(1, 1, FRTCellId(2, 0)));

	const FRTHexBlastPlan Singolo = URTHexCombatLibrary::CollectHexAttacks(
		Units, { CombatIntent(0, 1, ERTAbilityShape::Single, 5, 20) }, Map);
	const FRTHexBlastPlan AreaZero = URTHexCombatLibrary::CollectHexAttacks(
		Units, { CombatIntent(0, 1, ERTAbilityShape::Area, 5, 20, /*AreaRadius*/ 0) }, Map);

	if (Singolo.Footprints.Num() == 1 && AreaZero.Footprints.Num() == 1)
	{
		TestEqual(TEXT("stesse celle investite"),
			Singolo.Footprints[0].HitCells.Num(), AreaZero.Footprints[0].HitCells.Num());
		TestTrue(TEXT("ma il Single si dichiara Single"),
			Singolo.Footprints[0].Shape == ERTAbilityShape::Single);
		TestTrue(TEXT("e l'Area si dichiara Area"),
			AreaZero.Footprints[0].Shape == ERTAbilityShape::Area);
	}
	else
	{
		AddError(TEXT("impronte mancanti: il confronto fra le due forme non e' stato eseguito"));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFootprintChannelIsOrderedTest,
	"RefactorTactics.HexCombat.FootprintChannelIsOrdered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFootprintChannelIsOrderedTest::RunTest(const FString&)
{
	// Stessa rete degli altri canali del piano: l'ordine in uscita segue `IntentIndex`, non l'ordine di
	// arrivo, o la sequenza a valle non sarebbe deterministica.
	URTHexMapAsset* Map = MakeCombatMap(4);

	TArray<FRTHexCombatUnit> Units;
	Units.Add(CombatUnit(0, 0, FRTCellId(0, 0)));
	Units.Add(CombatUnit(1, 0, FRTCellId(1, 0)));
	Units.Add(CombatUnit(2, 1, FRTCellId(2, 0)));
	Units.Add(CombatUnit(3, 1, FRTCellId(3, 0)));

	TArray<FRTHexAttackIntent> Ordinati;
	Ordinati.Add(CombatIntent(0, 2, ERTAbilityShape::Single, 5, 10));
	Ordinati.Add(CombatIntent(1, 3, ERTAbilityShape::Single, 5, 10));

	const FRTHexBlastPlan Piano = URTHexCombatLibrary::CollectHexAttacks(Units, Ordinati, Map);

	TestEqual(TEXT("due intenti, due impronte"), Piano.Footprints.Num(), 2);
	if (Piano.Footprints.Num() == 2)
	{
		TestTrue(TEXT("ordinate per IntentIndex crescente"),
			Piano.Footprints[0].IntentIndex < Piano.Footprints[1].IntentIndex);
		TestEqual(TEXT("ogni impronta nomina il proprio attaccante"),
			Piano.Footprints[0].AttackerId, 0);
		TestEqual(TEXT("e la seconda il suo"), Piano.Footprints[1].AttackerId, 1);
	}
	else
	{
		AddError(TEXT("impronte inattese: l'ordine non e' stato verificato"));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
