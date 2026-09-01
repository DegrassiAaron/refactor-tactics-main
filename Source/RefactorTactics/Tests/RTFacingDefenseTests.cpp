#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTCombatResolver.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexMapAsset.h"
// La meta' `Guard` della regola non passa dal piano puro: vive in `ApplyFirstHitDelta` del TurnManager,
// quindi il suo test ha bisogno di un turno vero.
#include "Ability/RTHeroCatalogLibrary.h"
#include "Core/RTGameplayTags.h" // TAG_Status_Guarded
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexMapActor.h"
#include "Tests/RTAbilityFixtures.h" // AddCoreAbilityInSlot: montare Action.Deflect sul difensore
#include "Tests/RTWorldFixtures.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

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
		I.bCountsAsAttack = true; // intento d'attacco, e da [`INT-8`] va dichiarato
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

	ARTUnit* ArcSpawnUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // il piano lo scriviamo noi: qui si prova la difesa, non il bot
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeWraith());
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/**
	 * HP persi dal difensore in UN turno intero, cambiando solo cio' che il caso vuole cambiare.
	 *
	 * Passa dal `TurnManager` e non da `CollectHexAttacks` perche' la riduzione di `Guard` non sta nel piano:
	 * il piano porta la potenza, il delta del primo colpo si applica dopo (`ApplyFirstHitDelta`). Misurarla
	 * sul piano darebbe lo stesso numero nei due casi e il test sarebbe verde su qualunque implementazione.
	 *
	 * `-1` = montaggio fallito, gia' segnalato da `Test`: il chiamante non lo confronta con nient'altro.
	 */
	int32 ArcGuardDamageTaken(FAutomationTestBase& Test, ERTHexDirection DefenderFacing, bool bGuarded)
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!Test.TestNotNull(TEXT("mondo di prova"), World)) { return -1; }

		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		if (MapActor) { MapActor->MapAsset = MakeArcMap(6); }
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();

		// Attaccante a OVEST del difensore, che sta in (0,0,0): con `DefenderFacing` a E gli da' le spalle,
		// con W lo guarda. E' l'UNICA differenza fra i due casi.
		ARTUnit* Attaccante = ArcSpawnUnit(World, /*TeamId=*/ 0, FRTCellId(-1, 0, 0));
		ARTUnit* Difensore = ArcSpawnUnit(World, /*TeamId=*/ 1, FRTCellId(0, 0, 0));

		if (!Test.TestNotNull(TEXT("mappa"), MapActor) || !Test.TestNotNull(TEXT("turn manager"), TM)
			|| !Test.TestNotNull(TEXT("attaccante"), Attaccante) || !Test.TestNotNull(TEXT("difensore"), Difensore))
		{
			RTWorldFixtures::DestroyWorld(World);
			return -1;
		}

		Difensore->Facing = DefenderFacing;
		if (bGuarded) { Difensore->ApplyStatus(TAG_Status_Guarded, 1); }
		Difensore->PlannedAbilityIndex = INDEX_NONE; // il difensore non fa nulla: incassa e basta

		Attaccante->PlannedAbilityIndex = 0; // indice 0 = attacco base (`Hero.Wraith.PulseShot`)
		Attaccante->PlannedAttackTarget = Difensore;

		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}

		const int32 Taken = Difensore->MaxHealth - Difensore->Health;
		RTWorldFixtures::DestroyWorld(World);
		return Taken;
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
	const FRTDamageResult Front = URTCombatLibrary::ApplyDamage(FrontPower, ERTDamageSource::Direct, Shield, 0, 90);
	const FRTDamageResult Rear = URTCombatLibrary::ApplyDamage(RearPower, ERTDamageSource::Direct, Shield, 0, 90);
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


/**
 * **L'altra meta' di CP 16.2: il colpo alle spalle annulla anche `Action.Guard`.**
 *
 * 🔴 Il DoD di CP 16.2 nomina quattro test e ne esistevano **tre**: `Combat.BackAttackIgnoresGuard` non era
 * mai stato scritto, benche' due commenti di questo stesso file lo citino come se ci fosse (il gemello di
 * `FlankAttackKeepsCover` lo nomina fra i test che un'implementazione sbagliata «passerebbe comunque»).
 * Il comportamento pero' c'e' — `RTTurnManager.cpp`, ramo `bGuardHolds` con esito `RearHitBypassedGuard` —
 * e finora era pinnato solo da `UI.RearHitCreditsTheSameUnitInBothLogs`, che guarda **la traccia** e non il
 * danno: la voce di log poteva restare corretta mentre i -15 tornavano ad applicarsi.
 *
 * ⚠️ **Il terzo caso non e' ridondante.** Senza il controllo «stessa scena, nessuna guardia» il test non
 * distinguerebbe «la guardia e' stata annullata» da «esiste un bonus di fianco»: sono due regole diverse, e
 * ADR-0005 §4a ne ammette una sola — si TOGLIE una protezione, non si aggiunge danno.
 *
 * Riferimento: ADR-0005 §4a, D-020, issue #177.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCombatBackAttackIgnoresGuardTest,
	"RefactorTactics.Combat.BackAttackIgnoresGuard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCombatBackAttackIgnoresGuardTest::RunTest(const FString&)
{
	// 1. In guardia e RIVOLTO all'attaccante: la guardia regge.
	const int32 Frontale = ArcGuardDamageTaken(*this, ERTHexDirection::W, /*bGuarded=*/ true);
	// 2. Stessa scena, stessa guardia: cambia SOLO da che parte guarda il difensore.
	const int32 Posteriore = ArcGuardDamageTaken(*this, ERTHexDirection::E, /*bGuarded=*/ true);
	// 3. Controllo: alle spalle SENZA guardia. E' il caso che rende non vacuo il confronto.
	const int32 PosterioreScoperto = ArcGuardDamageTaken(*this, ERTHexDirection::E, /*bGuarded=*/ false);

	if (Frontale < 0 || Posteriore < 0 || PosterioreScoperto < 0)
	{
		return false; // montaggio gia' segnalato: proseguire misurerebbe un turno che non e' avvenuto
	}

	// Premessa esplicita: il colpo deve essere piu' forte della riduzione, altrimenti il frontale finirebbe a
	// zero per clamp e la differenza direbbe meno di quanto promette.
	TestTrue(TEXT("premessa: di fronte la guardia riduce ma non azzera"), Frontale > 0);

	TestEqual(TEXT("alle spalle si incassa la riduzione di catalogo in piu'"),
		Posteriore - Frontale, URTCombatLibrary::GuardFirstHitReduction);
	TestEqual(TEXT("e alle spalle la guardia non vale nulla: come non averla"),
		Posteriore, PosterioreScoperto);
	// Si TOGLIE una protezione, non si aggiunge un bonus: chi non ha guardia incassa lo stesso da ogni lato.
	TestTrue(TEXT("nessun bonus di fianco: il colpo pieno resta il massimo"),
		Posteriore <= PosterioreScoperto);
	return true;
}

/**
 * [D-312] misurata sulla CATENA REALE, che e' l'unica sede in cui l'ordine dei due pool esiste.
 *
 * Perche' serve, benche' `Combat.DeflectPoolAbsorbsBeforeGuardPool` parli gia' dell'ordine: quello chiama
 * `ApplyAbsorptionPool` DIRETTAMENTE, quindi prova che i due ordini divergono ma resta verde qualunque
 * ordine usi `RTTurnManager`. Misurato prima di scrivere questo: invertendo le due chiamate reali,
 * 100 test su 100 restavano verdi. Il buco era esattamente qui.
 *
 * La scena: difensore rivolto a OVEST, in Guardia e con `Action.Deflect` armata, preso fra due attaccanti —
 * uno davanti (`Guard` eleggibile) e uno dietro (`Guard` no, `Deflect` si', perche' non ha clausola d'arco).
 * E' la sovrapposizione parziale delle maschere, cioe' la condizione in cui l'ordine decide.
 *
 * ⚠️ **Il numero atteso e' un'OSSERVAZIONE, non una derivazione, e va trattato come tale.** Fra il pool e
 * gli HP passano altre riduzioni che questo test non isola: i colpi valgono `21` nel resolver e arrivano
 * ridotti. Cio' che rende il test valido non e' che `16` sia calcolabile a mano, ma che **discrimini**:
 * verificato per mutazione il 2026-09-01: invertendo le due chiamate in `RTTurnManager.cpp` il danno passa
 * da `16` a `2` e questo test diventa rosso — ed e' l'UNICO che cade, gli altri 100 della stessa passata
 * restano verdi. E' cio' che distingue un test che protegge la decisione da uno che la descrive soltanto.
 *
 * ⚠️ **E dipende da QUALE colpo e' primo nell'array.** Con il colpo frontale in prima posizione i due
 * ordini danno `21` e `7` prima delle riduzioni; con il frontale in seconda danno `7` entrambi, e il test
 * sarebbe cieco. Se un giorno cambiasse la costruzione di `Plan.Hits`, questo test va **rimisurato** — e
 * ricontrollato per mutazione — non aggiornato d'ufficio al nuovo numero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGuardAndDeflectOrderTest,
	"RefactorTactics.Combat.GuardAndDeflectAbsorbInDeclaredOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGuardAndDeflectOrderTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	if (MapActor) { MapActor->MapAsset = MakeArcMap(6); }
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();

	// Difensore in (0,0,0) rivolto a OVEST: chi sta a ovest lo colpisce di fronte, chi sta a est alle spalle.
	// L'ordine di spawn NON e' indifferente: mette il colpo frontale per primo in `Plan.Hits`, ed e' cio' che
	// rende i due ordini d'assorbimento distinguibili (vedi il commento sopra).
	ARTUnit* Frontale = ArcSpawnUnit(World, /*TeamId=*/ 0, FRTCellId(-1, 0, 0));
	ARTUnit* AlleSpalle = ArcSpawnUnit(World, /*TeamId=*/ 0, FRTCellId(1, 0, 0));
	ARTUnit* Difensore = ArcSpawnUnit(World, /*TeamId=*/ 1, FRTCellId(0, 0, 0));

	if (!TestNotNull(TEXT("mappa"), MapActor) || !TestNotNull(TEXT("turn manager"), TM)
		|| !TestNotNull(TEXT("attaccante frontale"), Frontale)
		|| !TestNotNull(TEXT("attaccante alle spalle"), AlleSpalle)
		|| !TestNotNull(TEXT("difensore"), Difensore))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	Difensore->Facing = ERTHexDirection::W;
	Difensore->ApplyStatus(TAG_Status_Guarded, 1);
	Difensore->PlannedReactionAbility =
		RTAbilityFixtures::AddCoreAbilityInSlot(Difensore, TEXT("Action.Deflect"), 3);
	Difensore->PlannedAbilityIndex = INDEX_NONE; // incassa e basta

	Frontale->PlannedAbilityIndex = 0;
	Frontale->PlannedAttackTarget = Difensore;
	AlleSpalle->PlannedAbilityIndex = 0;
	AlleSpalle->PlannedAttackTarget = Difensore;

	TM->LockInAndResolve();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
	{
		TM->Tick(0.05f);
	}

	const int32 Taken = Difensore->MaxHealth - Difensore->Health;
	RTWorldFixtures::DestroyWorld(World);

	// ANTI-VACUITA': lo stesso colpo, senza Guardia e senza reazione, moltiplicato per due. Misurato invece
	// che scritto come costante, cosi' segue il catalogo. Senza questo confronto l'uguaglianza sotto
	// passerebbe anche per difese che non tolgono niente.
	const int32 NominaleSenzaDifese = 2 * ArcGuardDamageTaken(*this, ERTHexDirection::E, /*bGuarded=*/ false);
	TestTrue(TEXT("le due difese tolgono qualcosa rispetto ai due colpi nudi"),
		Taken > 0 && Taken < NominaleSenzaDifese);

	TestEqual(TEXT("con Deflect che assorbe prima di Guard (D-312) il difensore incassa 16"), Taken, 16);

	return true;
}


// ======================================================================================================
// `D-302` punto (3): per un'AREA la direzione d'impatto e' centro d'impatto -> bersaglio, non lanciatore
// -> bersaglio. Il pool `Guard` e' l'unico consumatore in cui la differenza si vede (`#2009`).
// ======================================================================================================

namespace
{
	/**
	 * HP persi dal difensore quando un'AREA gli cade addosso, con il centro e il lanciatore su lati scelti.
	 *
	 * Perche' un'area e non un colpo singolo: per `Single`, `Line` e `Cone` la direzione d'impatto coincide
	 * gia' con `attaccante->bersaglio`, quindi `D-302` e' soddisfatta e non c'e' niente da distinguere. Solo
	 * l'area puo' avere il centro da una parte e chi la lancia dall'altra.
	 *
	 * `-1` = montaggio fallito, gia' segnalato da `Test`.
	 */
	int32 ArcAreaDamageTaken(FAutomationTestBase& Test, const FRTCellId& ThrowerCell,
		const FRTCellId& ImpactCenter, bool bGuarded)
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!Test.TestNotNull(TEXT("mondo di prova"), World)) { return -1; }

		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		if (MapActor) { MapActor->MapAsset = MakeArcMap(6); }
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();

		ARTUnit* Lanciatore = ArcSpawnUnit(World, /*TeamId=*/ 0, ThrowerCell);
		ARTUnit* Difensore  = ArcSpawnUnit(World, /*TeamId=*/ 1, FRTCellId(0, 0, 0));

		if (!Test.TestNotNull(TEXT("mappa"), MapActor) || !Test.TestNotNull(TEXT("turn manager"), TM)
			|| !Test.TestNotNull(TEXT("lanciatore"), Lanciatore) || !Test.TestNotNull(TEXT("difensore"), Difensore))
		{
			RTWorldFixtures::DestroyWorld(World);
			return -1;
		}

		// Rivolto a OVEST: chi sta a ovest lo prende di fronte, chi sta a est alle spalle — la stessa
		// convenzione degli altri test di questo file.
		Difensore->Facing = ERTHexDirection::W;
		if (bGuarded) { Difensore->ApplyStatus(TAG_Status_Guarded, 1); }
		Difensore->PlannedAbilityIndex = INDEX_NONE; // incassa e basta: nessuna reazione da isolare

		// Un'area montata sull'attacco base invece che pescata dal kit di un eroe: qui contano la FORMA e il
		// raggio, non i numeri di bilanciamento di `Hero.Gadget.Overload`, e legare il test a quelli lo
		// farebbe cadere al prossimo ritocco del catalogo.
		const int32 Slot = RTAbilityFixtures::AddCoreAbilityInSlot(Lanciatore, TEXT("Action.BasicAttack"), 3);
		if (!Test.TestTrue(TEXT("slot dell'area"), Slot != INDEX_NONE))
		{
			RTWorldFixtures::DestroyWorld(World);
			return -1;
		}
		URTActionData* Area = Lanciatore->Abilities[Slot];
		Area->Shape = ERTAbilityShape::Area;
		Area->AreaRadius = 1;
		Area->RangeCells = 4; // basta per entrambe le geometrie provate qui
		// ⚠️ **Il danno va messo a mano, e senza di esso il test misurava zero.** Il catalogo dichiara di
		// `Action.BasicAttack` *«identita', fase, priorita' e fallback [...]; DANNO e PORTATA no»*, quindi
		// `FirstDamage` risponde `0` e il ripiego `Power` resta `0`: l'area partiva, investiva il
		// difensore e gli toglieva niente. L'ha intercettata la riga anti-vacuita' dei due test.
		Area->Power = 20;

		Lanciatore->PlannedAbilityIndex = Slot;
		Lanciatore->PlannedAttackTarget = nullptr;
		Lanciatore->PlannedAttackCell = ImpactCenter;
		Lanciatore->bAttackTargetsCell = true;

		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}

		const int32 Taken = Difensore->MaxHealth - Difensore->Health;
		RTWorldFixtures::DestroyWorld(World);
		return Taken;
	}
}

/**
 * IL CENTRO D'IMPATTO DAVANTI SALVA LA GUARDIA, ANCHE SE CHI LANCIA STA DIETRO.
 *
 * [D-302] punto (3): per un'area la direzione d'impatto e' **centro d'impatto -> bersaglio**. Una granata
 * fatta cadere DAVANTI a un'unita' in Guardia da un lanciatore che le sta alle SPALLE arriva, per quella
 * unita', da davanti: il pool va consumato.
 *
 * 🔴 Il difetto che il test misura (`#2009`): `ResolveCombatPasses` passava a `IsInFrontalArc` la cella di
 * **chi ha attaccato**, qualunque fosse la forma. Con il lanciatore alle spalle la Guardia veniva
 * scavalcata e usciva un `RearHitBypassedGuard` — per un colpo che il difensore vedeva arrivare.
 *
 * ⚠️ **Il caso non era coperto da niente**: nessun test del repository combinava `TAG_Status_Guarded` con
 * `ERTAbilityShape::Area`, quindi la suite non proteggeva questo comportamento e non l'ha protetto finora.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAreaGuardUsesImpactCenterTest,
	"RefactorTactics.Combat.AreaGuardUsesImpactCenter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAreaGuardUsesImpactCenterTest::RunTest(const FString&)
{
	// Lanciatore a EST (dietro), centro dell'area a OVEST (davanti): l'area di raggio 1 attorno a (-1,0,0)
	// investe (0,0,0), dove sta il difensore.
	const FRTCellId Lanciatore(2, 0, 0);
	const FRTCellId Centro(-1, 0, 0);

	const int32 ConGuardia   = ArcAreaDamageTaken(*this, Lanciatore, Centro, /*bGuarded=*/ true);
	const int32 SenzaGuardia = ArcAreaDamageTaken(*this, Lanciatore, Centro, /*bGuarded=*/ false);
	if (ConGuardia < 0 || SenzaGuardia < 0) { return false; }

	// ANTI-VACUITA': se l'area non arrivasse affatto, entrambi i numeri sarebbero zero e l'asserzione sotto
	// passerebbe per il motivo sbagliato — il modo piu' facile di scrivere un verde su una geometria rotta.
	if (!TestTrue(TEXT("l'area investe davvero il difensore"), SenzaGuardia > 0))
	{
		return false;
	}

	TestTrue(TEXT("il centro davanti fa consumare il pool, malgrado il lanciatore alle spalle"),
		ConGuardia < SenzaGuardia);
	return true;
}

/**
 * E IL SIMMETRICO: IL CENTRO DIETRO SCAVALCA LA GUARDIA, ANCHE SE CHI LANCIA STA DAVANTI.
 *
 * L'altra meta' di [D-302] punto (3), e serve entrambe: un test solo sarebbe passato anche con la regola
 * invertita — *«per le aree il pool si consuma sempre»* — che e' un'implementazione sbagliata e comoda.
 * Qui l'ordigno esplode ALLE SPALLE del difensore mentre chi l'ha lanciato gli sta di fronte, e il pool
 * **non** deve proteggerlo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAreaGuardBypassedFromBehindTest,
	"RefactorTactics.Combat.AreaGuardIsBypassedWhenImpactCenterIsBehind",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAreaGuardBypassedFromBehindTest::RunTest(const FString&)
{
	// Lanciatore a OVEST (davanti), centro dell'area a EST (dietro).
	const FRTCellId Lanciatore(-2, 0, 0);
	const FRTCellId Centro(1, 0, 0);

	const int32 ConGuardia   = ArcAreaDamageTaken(*this, Lanciatore, Centro, /*bGuarded=*/ true);
	const int32 SenzaGuardia = ArcAreaDamageTaken(*this, Lanciatore, Centro, /*bGuarded=*/ false);
	if (ConGuardia < 0 || SenzaGuardia < 0) { return false; }

	if (!TestTrue(TEXT("l'area investe davvero il difensore"), SenzaGuardia > 0))
	{
		return false;
	}

	TestEqual(TEXT("il centro alle spalle scavalca il pool, malgrado il lanciatore di fronte"),
		ConGuardia, SenzaGuardia);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
