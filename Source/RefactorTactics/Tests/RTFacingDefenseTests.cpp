#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTCombatResolver.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexMapAsset.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * CP 16.2 — l'emisfero posteriore e' SCOPERTO.
 *
 * La regola non aggiunge numeri: TOGLIE una protezione. Un colpo che non arriva dall'arco frontale annulla la
 * riduzione da copertura bassa (-10) e da `Action.Guard` (-15); scudi, `Brace` e `Deflect` restano validi da
 * ogni direzione, perche' proteggono la persona e non un lato.
 *
 * Riferimento: ADR-0005 §4a, issue #177.
 */

namespace
{
	/** Arena piena di raggio N. Nome distinto per file: la unity build condivide la translation unit. */
	URTHexMapAsset* MakeArcMap(int32 Radius)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		return M;
	}

	void SetArcLowCover(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		Data.Covers.Add(FRTHexCover(Edge, ERTHexCoverType::Low, FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low)));
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	FRTHexCombatUnit ArcUnit(int32 UnitId, int32 TeamId, const FRTCellId& Cell, ERTHexDirection Facing)
	{
		FRTHexCombatUnit U;
		U.UnitId = UnitId;
		U.TeamId = TeamId;
		U.Cell = Cell;
		U.bAlive = true;
		U.Facing = Facing;
		return U;
	}

	FRTHexAttackIntent ArcIntent(int32 AttackerId, int32 TargetId, int32 Power)
	{
		FRTHexAttackIntent I;
		I.AttackerId = AttackerId;
		I.TargetId = TargetId;
		I.Shape = ERTAbilityShape::Single;
		I.RangeCells = 5;
		I.AreaRadius = 0;
		I.Power = Power;
		return I;
	}

	int32 ArcPowerOn(const FRTHexBlastPlan& Plan, int32 TargetId)
	{
		for (const FRTHexAttackHit& Hit : Plan.Hits)
		{
			if (Hit.TargetId == TargetId) { return Hit.Power; }
		}
		return -1;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCombatBackAttackIgnoresCoverTest,
	"RefactorTactics.Combat.BackAttackIgnoresCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCombatBackAttackIgnoresCoverTest::RunTest(const FString&)
{
	// Attaccante a ovest, bersaglio in (2,0) con copertura bassa sul bordo W: il riparo e' interposto.
	const FRTCellId Defender(2, 0, 0);
	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(ArcIntent(0, 1, 30));

	// Difensore che GUARDA l'attaccante: la copertura vale.
	{
		TArray<FRTHexCombatUnit> Units;
		Units.Add(ArcUnit(0, 0, FRTCellId(0, 0, 0), ERTHexDirection::E));
		Units.Add(ArcUnit(1, 1, Defender, ERTHexDirection::W));

		URTHexMapAsset* Map = MakeArcMap(3);
		SetArcLowCover(Map, Defender, ERTHexDirection::W);
		const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
		TestEqual(TEXT("di fronte: la copertura protegge"),
			ArcPowerOn(Plan, 1), 30 - URTCombatLibrary::LowCoverDamageReduction);
	}

	// Stessa scena, stesso riparo, stesso colpo: cambia SOLO da che parte guarda il difensore.
	{
		TArray<FRTHexCombatUnit> Units;
		Units.Add(ArcUnit(0, 0, FRTCellId(0, 0, 0), ERTHexDirection::E));
		Units.Add(ArcUnit(1, 1, Defender, ERTHexDirection::E)); // dà le spalle all'attaccante

		URTHexMapAsset* Map = MakeArcMap(3);
		SetArcLowCover(Map, Defender, ERTHexDirection::W);
		const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);

		TestEqual(TEXT("alle spalle: la copertura non vale"), ArcPowerOn(Plan, 1), 30);
		// Si TOGLIE una protezione, non si aggiunge un bonus: il danno e' quello pieno, non di piu'.
		TestTrue(TEXT("nessun bonus di fianco"), ArcPowerOn(Plan, 1) <= 30);
		// Il DoD misura proprio questo: 10 danni in piu' su HP che vanno da 90 a 120.
		TestEqual(TEXT("la differenza e' la riduzione di catalogo"),
			30 - (30 - URTCombatLibrary::LowCoverDamageReduction), 10);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCombatFlankAttackKeepsCoverTest,
	"RefactorTactics.Combat.FlankAttackKeepsCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCombatFlankAttackKeepsCoverTest::RunTest(const FString&)
{
	// L'arco frontale e' un CONO di 120 gradi, non la sola direzione esatta: un attaccante spostato di un
	// settore resta davanti. Senza questo test, un'implementazione che confronta la direzione esatta
	// passerebbe comunque `BackAttackIgnoresCover` e `BackAttackIgnoresGuard`.
	const FRTCellId Defender(0, 0, 0);

	// Il difensore guarda a E. Un attaccante in (1,-1) sta a NE: fuori dalla direzione esatta, dentro il cono.
	TestTrue(TEXT("NE e' dentro l'arco di chi guarda a E"),
		URTHexCombatLibrary::IsInFrontalArc(Defender, ERTHexDirection::E, FRTCellId(1, -1, 0)));
	TestTrue(TEXT("SE pure"),
		URTHexCombatLibrary::IsInFrontalArc(Defender, ERTHexDirection::E, FRTCellId(0, 1, 0)));
	TestFalse(TEXT("W e' fuori"),
		URTHexCombatLibrary::IsInFrontalArc(Defender, ERTHexDirection::E, FRTCellId(-1, 0, 0)));
	TestFalse(TEXT("NW e' fuori"),
		URTHexCombatLibrary::IsInFrontalArc(Defender, ERTHexDirection::E, FRTCellId(0, -1, 0)));

	// Stessa cella dell'origine: non c'e' un lato da cui il colpo arrivi.
	TestTrue(TEXT("origine coincidente: frontale"),
		URTHexCombatLibrary::IsInFrontalArc(Defender, ERTHexDirection::E, Defender));

	// E vale anche da lontano: il cono e' profondo quanto serve, non troncato a un raggio fisso.
	TestTrue(TEXT("frontale a distanza 3"),
		URTHexCombatLibrary::IsInFrontalArc(Defender, ERTHexDirection::E, FRTCellId(3, 0, 0)));
	TestFalse(TEXT("posteriore a distanza 3"),
		URTHexCombatLibrary::IsInFrontalArc(Defender, ERTHexDirection::E, FRTCellId(-3, 0, 0)));

	// Il colpo di fianco conserva la copertura: (2,0) guarda a NW, attaccante in (0,0) a W — dentro il cono.
	TArray<FRTHexCombatUnit> Units;
	Units.Add(ArcUnit(0, 0, FRTCellId(0, 0, 0), ERTHexDirection::E));
	Units.Add(ArcUnit(1, 1, FRTCellId(2, 0, 0), ERTHexDirection::SW));

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(ArcIntent(0, 1, 30));

	URTHexMapAsset* Map = MakeArcMap(3);
	SetArcLowCover(Map, FRTCellId(2, 0, 0), ERTHexDirection::W);
	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
	TestEqual(TEXT("di fianco ma dentro l'arco: la copertura tiene"),
		ArcPowerOn(Plan, 1), 30 - URTCombatLibrary::LowCoverDamageReduction);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCombatShieldWorksFromAnyDirectionTest,
	"RefactorTactics.Combat.ShieldWorksFromAnyDirection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCombatShieldWorksFromAnyDirectionTest::RunTest(const FString&)
{
	// Scudi, `Brace` e `Deflect` proteggono la PERSONA, non un lato. Il modo di verificarlo non e' chiamare
	// due volte `ApplyDamage` con gli stessi argomenti — non ha un parametro di direzione, quindi confronterebbe
	// una funzione con se' stessa e resterebbe verde qualunque cosa faccia il percorso di combattimento.
	//
	// Si passa invece da `CollectHexAttacks`, che e' dove la direzione conta davvero, e si verifica la regola
	// nella forma in cui puo' essere smentita: **senza copertura, il facing non cambia il danno**. Se un giorno
	// qualcuno estendesse la penalita' direzionale oltre copertura e `Guard`, questo test cadrebbe.
	const FRTCellId Defender(2, 0, 0);
	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(ArcIntent(0, 1, 30));

	int32 FrontPower = -1;
	int32 RearPower = -1;
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		const ERTHexDirection Facing = (Pass == 0) ? ERTHexDirection::W : ERTHexDirection::E;
		TArray<FRTHexCombatUnit> Units;
		Units.Add(ArcUnit(0, 0, FRTCellId(0, 0, 0), ERTHexDirection::E));
		Units.Add(ArcUnit(1, 1, Defender, Facing));

		URTHexMapAsset* Bare = MakeArcMap(3); // nessuna copertura: non c'e' protezione da togliere
		const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Bare);
		(Pass == 0 ? FrontPower : RearPower) = ArcPowerOn(Plan, 1);
	}

	TestEqual(TEXT("senza copertura il colpo frontale fa danno pieno"), FrontPower, 30);
	TestEqual(TEXT("e quello alle spalle lo stesso: non c'e' bonus di fianco"), RearPower, FrontPower);

	// Lo SCUDO assorbe dopo, in `ApplyDamage`, che non ha e non deve avere un parametro di direzione: il colpo
	// che gli arriva e' identico nei due casi, quindi lo e' anche cio' che resta.
	const int32 Shield = 12;
	const FRTDamageResult Front = URTCombatLibrary::ApplyDamage(FrontPower, Shield, 90);
	const FRTDamageResult Rear = URTCombatLibrary::ApplyDamage(RearPower, Shield, 90);
	TestEqual(TEXT("lo scudo lascia lo stesso residuo di HP"), Front.Health, Rear.Health);
	TestEqual(TEXT("il conto e' quello del catalogo"), Front.Health, 90 - (30 - Shield));
	return true;
}


/**
 * `#649`, pezzo (a) — il colpo PORTA con sé quanto la direzione ha annullato.
 *
 * Finora quell'informazione moriva dentro `EffectiveCoverReduction`, che è pura: restituisce `0` sia quando
 * la copertura non c'è, sia quando c'è e la provenienza l'ha resa inutile. Dall'esterno un colpo pieno su un
 * bersaglio riparato era indistinguibile da un colpo pieno su un bersaglio scoperto — e il bot, che dal
 * 2026-08-12 conta proprio quel danno come bonus (CP 13.5), contava qualcosa che nessuno poteva verificare.
 *
 * ⚠️ **Questo test NON passa dal TurnLog, ed è deliberato.** È la metà del lavoro che non cambia nessun hash:
 * finché il campo viaggia soltanto dentro `FRTHexAttackHit`, il corpus golden resta valido. La voce di
 * traccia — che gli hash li cambia — è il pezzo (b), e ha il suo test separato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCombatHitCarriesBypassedCoverTest,
	"RefactorTactics.Combat.HitCarriesBypassedCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCombatHitCarriesBypassedCoverTest::RunTest(const FString&)
{
	const FRTCellId Defender(2, 0, 0);
	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(ArcIntent(0, 1, 30));

	auto BypassedOn = [](const FRTHexBlastPlan& Plan, int32 TargetId) -> int32
	{
		for (const FRTHexAttackHit& Hit : Plan.Hits)
		{
			if (Hit.TargetId == TargetId) { return Hit.CoverBypassedByFacing; }
		}
		return -1;
	};

	// 1. Difensore che GUARDA l'attaccante: la copertura vale, quindi non è stata scavalcata niente.
	{
		TArray<FRTHexCombatUnit> Units;
		Units.Add(ArcUnit(0, 0, FRTCellId(0, 0, 0), ERTHexDirection::E));
		Units.Add(ArcUnit(1, 1, Defender, ERTHexDirection::W));

		URTHexMapAsset* Map = MakeArcMap(3);
		SetArcLowCover(Map, Defender, ERTHexDirection::W);
		const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
		TestEqual(TEXT("di fronte: niente scavalcato"), BypassedOn(Plan, 1), 0);
	}

	// 2. Stessa scena, stesso riparo: cambia SOLO l'orientamento. È qui che il campo deve valere qualcosa.
	{
		TArray<FRTHexCombatUnit> Units;
		Units.Add(ArcUnit(0, 0, FRTCellId(0, 0, 0), ERTHexDirection::E));
		Units.Add(ArcUnit(1, 1, Defender, ERTHexDirection::E)); // dà le spalle

		URTHexMapAsset* Map = MakeArcMap(3);
		SetArcLowCover(Map, Defender, ERTHexDirection::W);
		const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);

		TestEqual(TEXT("alle spalle: scavalcata la riduzione di catalogo"),
			BypassedOn(Plan, 1), URTCombatLibrary::LowCoverDamageReduction);
		// Il danno resta quello di sempre: il campo SPIEGA la sottrazione mancata, non ne aggiunge una.
		TestEqual(TEXT("e il danno non cambia di un punto"), ArcPowerOn(Plan, 1), 30);
	}

	// 3. Nessuna copertura: la traccia deve dire «annullata», non «valutata». Senza questo caso il campo
	//    potrebbe valere sempre qualcosa e il test 2 passerebbe lo stesso.
	{
		TArray<FRTHexCombatUnit> Units;
		Units.Add(ArcUnit(0, 0, FRTCellId(0, 0, 0), ERTHexDirection::E));
		Units.Add(ArcUnit(1, 1, Defender, ERTHexDirection::E));

		URTHexMapAsset* Map = MakeArcMap(3); // arena liscia
		const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
		TestEqual(TEXT("senza copertura non c'e' niente da scavalcare"), BypassedOn(Plan, 1), 0);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
