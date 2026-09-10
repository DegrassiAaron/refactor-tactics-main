#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTTurnLog.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamagePartlyAbsorbedTest,
	"RefactorTactics.Combat.DamagePartlyAbsorbedByShield",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamagePartlyAbsorbedTest::RunTest(const FString&)
{
	// 30 danni, scudo 20 -> scudo assorbe 20, 10 agli HP: 100 -> 90.
	const FRTDamageResult R = URTCombatLibrary::ApplyDamage(30, ERTDamageSource::Direct, 20, 0, 100);
	TestEqual(TEXT("scudo consumato"), R.Shield, 0);
	TestEqual(TEXT("HP 90"), R.Health, 90);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageFullyAbsorbedTest,
	"RefactorTactics.Combat.DamageFullyAbsorbedByShield",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageFullyAbsorbedTest::RunTest(const FString&)
{
	// 15 danni, scudo 20 -> scudo assorbe tutto (resta 5), HP intatti.
	const FRTDamageResult R = URTCombatLibrary::ApplyDamage(15, ERTDamageSource::Direct, 20, 0, 100);
	TestEqual(TEXT("scudo residuo 5"), R.Shield, 5);
	TestEqual(TEXT("HP 100"), R.Health, 100);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageExceedsAllTest,
	"RefactorTactics.Combat.DamageExceedingHealthClampsToZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageExceedsAllTest::RunTest(const FString&)
{
	// Danno enorme -> scudo e HP a 0 (nessun valore negativo).
	const FRTDamageResult R = URTCombatLibrary::ApplyDamage(200, ERTDamageSource::Direct, 20, 0, 100);
	TestEqual(TEXT("scudo 0"), R.Shield, 0);
	TestEqual(TEXT("HP 0"), R.Health, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageNoShieldTest,
	"RefactorTactics.Combat.DamageWithoutShieldHitsHealth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageNoShieldTest::RunTest(const FString&)
{
	const FRTDamageResult R = URTCombatLibrary::ApplyDamage(30, ERTDamageSource::Direct, 0, 0, 100);
	TestEqual(TEXT("scudo 0"), R.Shield, 0);
	TestEqual(TEXT("HP 70"), R.Health, 70);
	return true;
}

// ─── [D-224] Scudo base e sorgente del danno ────────────────────────────────────────────────────────
//
// Lo scudo di un'unita' e' UN numero con una quota: `Shield` e' il totale, `TemporaryShield` la parte che
// scade nel Cleanup, e cio' che avanza e' la BASE. Il danno erode sempre il temporaneo per primo; la base
// partecipa solo se il colpo e' diretto.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageDirectErodesTemporaryFirstTest,
	"RefactorTactics.Combat.DirectDamageErodesTemporaryShieldBeforeBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageDirectErodesTemporaryFirstTest::RunTest(const FString&)
{
	// 5 base + 25 temporaneo = 30. Dieci danni diretti escono TUTTI dal temporaneo.
	const FRTDamageResult R = URTCombatLibrary::ApplyDamage(
		10, ERTDamageSource::Direct, /*Shield*/ 30, /*TemporaryShield*/ 25, /*Health*/ 100);
	TestEqual(TEXT("scudo totale 20"), R.Shield, 20);
	TestEqual(TEXT("temporaneo 15"), R.TemporaryShield, 15);
	TestEqual(TEXT("la base non e' stata toccata"), R.Shield - R.TemporaryShield, 5);
	TestEqual(TEXT("HP intatti"), R.Health, 100);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageDirectConsumesBaseAfterTemporaryTest,
	"RefactorTactics.Combat.DirectDamageConsumesBaseOnceTemporaryIsGone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageDirectConsumesBaseAfterTemporaryTest::RunTest(const FString&)
{
	// 5 base + 25 temporaneo, 28 danni diretti: 25 dal temporaneo, 3 dalla base, 0 agli HP.
	const FRTDamageResult R = URTCombatLibrary::ApplyDamage(
		28, ERTDamageSource::Direct, /*Shield*/ 30, /*TemporaryShield*/ 25, /*Health*/ 100);
	TestEqual(TEXT("temporaneo esaurito"), R.TemporaryShield, 0);
	TestEqual(TEXT("base ridotta a 2"), R.Shield, 2);
	TestEqual(TEXT("HP intatti"), R.Health, 100);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageEnvironmentalSkipsBaseTest,
	"RefactorTactics.Combat.EnvironmentalDamageSkipsBaseShield",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageEnvironmentalSkipsBaseTest::RunTest(const FString&)
{
	// 5 di sola base, 8 di Burning: la base NON assorbe, tutti e 8 arrivano agli HP.
	const FRTDamageResult R = URTCombatLibrary::ApplyDamage(
		URTCombatLibrary::BurningCleanupDamage, ERTDamageSource::Environmental,
		/*Shield*/ URTCombatLibrary::BaseShield, /*TemporaryShield*/ 0, /*Health*/ 100);
	TestEqual(TEXT("la base resta intera"), R.Shield, URTCombatLibrary::BaseShield);
	TestEqual(TEXT("HP 92"), R.Health, 92);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageEnvironmentalErodesTemporaryTest,
	"RefactorTactics.Combat.EnvironmentalDamageStillErodesTemporaryShield",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageEnvironmentalErodesTemporaryTest::RunTest(const FString&)
{
	// L'ambientale salta la BASE, non il temporaneo: lo scudo che qualcuno ha speso un'azione per
	// costruire copre anche dagli hazard. Questo test PINNA quella scelta — se un giorno si decidesse che
	// `Burning` ignora ogni scudo, il rosso dice quale decisione si sta superando.
	const FRTDamageResult R = URTCombatLibrary::ApplyDamage(
		URTCombatLibrary::BurningCleanupDamage, ERTDamageSource::Environmental,
		/*Shield*/ 30, /*TemporaryShield*/ 25, /*Health*/ 100);
	TestEqual(TEXT("temporaneo 17"), R.TemporaryShield, 17);
	TestEqual(TEXT("la base resta intera"), R.Shield - R.TemporaryShield, 5);
	TestEqual(TEXT("HP intatti"), R.Health, 100);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageEnvironmentalCanKillThroughBaseTest,
	"RefactorTactics.Combat.EnvironmentalDamageCanKillThroughBaseShield",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageEnvironmentalCanKillThroughBaseTest::RunTest(const FString&)
{
	// Il caso che la regola esiste per preservare: 5 di base non salvano da un Burning letale. Senza la
	// distinzione di sorgente questo colpo farebbe 3 danni invece di 8, e il danno nel tempo smetterebbe
	// di uccidere chiunque.
	const FRTDamageResult R = URTCombatLibrary::ApplyDamage(
		URTCombatLibrary::BurningCleanupDamage, ERTDamageSource::Environmental,
		/*Shield*/ URTCombatLibrary::BaseShield, /*TemporaryShield*/ 0, /*Health*/ 6);
	TestEqual(TEXT("HP a zero"), R.Health, 0);
	TestEqual(TEXT("lo scudo base e' ancora li', e non ha salvato nessuno"),
		R.Shield, URTCombatLibrary::BaseShield);
	return true;
}

// `GainEnergyClampsToMax` e `UltimateReadyAtFullEnergy` stavano qui, e sono stati rimossi insieme alle due
// funzioni che misuravano — `URTCombatLibrary::GainEnergy` e `IsUltimateReady` — quando
// [D-324](../../../../docs/decisions/RT_PDR_00_Decision_Log.md) ha tolto `Energy` dal gameplay.
//
// ⚠️ Non erano test deboli: `IsUltimateReady` non aveva **nessun** chiamante di produzione, e questi due casi
// erano l'unica cosa che la tenesse viva. Un'API il cui solo consumatore e' il proprio test non e' coperta:
// e' sopravvissuta.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEffectiveMoveRangeTest,
	"RefactorTactics.Combat.EffectiveMoveRangeWithStatus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEffectiveMoveRangeTest::RunTest(const FString&)
{
	// CP 4.7: Slow non passa piu' da qui (era un dimezzamento pre-CP4.2, sostituito dal costo per cella nel
	// pathfinding — vedi RefactorTactics.Actions.Slow.ExtraCostPerCell). Qui resta solo Root.
	TestEqual(TEXT("non radicato -> base"), URTCombatLibrary::EffectiveMoveRange(4, false), 4);
	TestEqual(TEXT("radicato -> 0"), URTCombatLibrary::EffectiveMoveRange(4, true), 0);
	TestEqual(TEXT("radicato con budget gia' zero -> 0"), URTCombatLibrary::EffectiveMoveRange(0, true), 0);
	return true;
}

/**
 * Il gate di un'abilita', che da [D-324](../../../../docs/decisions/RT_PDR_00_Decision_Log.md) e' **solo** il
 * cooldown.
 *
 * Si chiamava `AbilityUsableByCooldownAndEnergy` e provava due clausole. Il nome e' cambiato con la firma:
 * un test che continuasse a chiamarsi «...AndEnergy» direbbe al lettore che esiste una seconda condizione,
 * e nessun gate legge i nomi dei test per accorgersene.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityUsableTest,
	"RefactorTactics.Combat.AbilityUsableByCooldownOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityUsableTest::RunTest(const FString&)
{
	TestTrue(TEXT("fuori ricarica -> usabile"), URTCombatLibrary::IsAbilityUsable(0));
	TestFalse(TEXT("in ricarica -> non usabile"), URTCombatLibrary::IsAbilityUsable(2));
	TestFalse(TEXT("un solo turno di ricarica basta a negarla"), URTCombatLibrary::IsAbilityUsable(1));
	// Il contatore non scende sotto zero nei chiamanti, ma la funzione non lo assume: un negativo e' «fuori
	// ricarica», non un caso da rifiutare.
	TestTrue(TEXT("contatore negativo -> usabile"), URTCombatLibrary::IsAbilityUsable(-1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIntentVisibilityTest,
	"RefactorTactics.Combat.IntentVisibleToAlliesAlwaysEnemiesOnlyIfRevealed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIntentVisibilityTest::RunTest(const FString&)
{
	// Alleati (stessa squadra): il piano e' sempre visibile, rivelato o no.
	TestTrue(TEXT("alleato vede il piano"), URTCombatLibrary::IsIntentVisibleTo(0, 0, false));
	TestTrue(TEXT("alleato vede anche se rivelato"), URTCombatLibrary::IsIntentVisibleTo(0, 0, true));
	// Avversari (squadra diversa): visibile solo se il proprietario e' rivelato (invariante #6).
	TestFalse(TEXT("nemico NON vede il piano privato"), URTCombatLibrary::IsIntentVisibleTo(1, 0, false));
	TestTrue(TEXT("nemico vede il piano rivelato"), URTCombatLibrary::IsIntentVisibleTo(1, 0, true));
	TestFalse(TEXT("nemico (altra squadra) non rivelato"), URTCombatLibrary::IsIntentVisibleTo(0, 1, false));
	return true;
}

// Il nome dice «bonus dichiarato» e non «altura» per una ragione registrata: D-024 ha tolto alla quota il
// bonus al danno, ma il parametro e' rimasto perche' serve a chi lo dichiara (tratto, abilita',
// equipaggiamento). Il vecchio nome — `EffectiveAttackPowerWithTerrainBonus` — insegnava una semantica che
// il codice non ha piu': ogni call site runtime passa `0`. Rinominato con `#538`.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEffectiveAttackPowerTest,
	"RefactorTactics.Combat.EffectiveAttackPowerAddsDeclaredBonus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEffectiveAttackPowerTest::RunTest(const FString&)
{
	TestEqual(TEXT("bonus dichiarato +10 -> 40"), URTCombatLibrary::EffectiveAttackPower(30, 10), 40);
	TestEqual(TEXT("nessun bonus -> base"), URTCombatLibrary::EffectiveAttackPower(30, 0), 30);
	TestEqual(TEXT("malus enorme -> clamp 0"), URTCombatLibrary::EffectiveAttackPower(10, -30), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNewlyDefeatedTest,
	"RefactorTactics.Combat.NewlyDefeatedDetectsFreshDeaths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNewlyDefeatedTest::RunTest(const FString&)
{
	// Una sola unita' passa da viva a morta -> il suo indice.
	{
		const TArray<int32> D = URTCombatLibrary::NewlyDefeated({ 100, 100 }, { 100, 0 });
		TestEqual(TEXT("una morte, indice 1"), D.Num(), 1);
		TestTrue(TEXT("indice corretto"), D.Num() == 1 && D[0] == 1);
	}
	// Chi era gia' morto PRIMA non e' "appena eliminato".
	{
		const TArray<int32> D = URTCombatLibrary::NewlyDefeated({ 0, 100 }, { 0, 100 });
		TestEqual(TEXT("gia' morta -> nessuna nuova morte"), D.Num(), 0);
	}
	// HP negativi contano come morte.
	{
		const TArray<int32> D = URTCombatLibrary::NewlyDefeated({ 100 }, { -5 });
		TestTrue(TEXT("HP negativi -> morta"), D.Num() == 1 && D[0] == 0);
	}
	// Morti multiple, indici in ordine.
	{
		const TArray<int32> D = URTCombatLibrary::NewlyDefeated({ 50, 50, 50 }, { 0, 50, 0 });
		TestTrue(TEXT("due morti agli indici 0 e 2"), D.Num() == 2 && D[0] == 0 && D[1] == 2);
	}
	// Lunghezze diverse: itera fino al minimo, senza crash.
	{
		const TArray<int32> D = URTCombatLibrary::NewlyDefeated({ 100, 100, 100 }, { 0 });
		TestTrue(TEXT("solo l'indice 0 confrontabile"), D.Num() == 1 && D[0] == 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTClassifyCombatOutcomeTest,
	"RefactorTactics.Combat.ClassifyCombatOutcome",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTClassifyCombatOutcomeTest::RunTest(const FString&)
{
	// ShieldAbsorbed: HP invariati (lo scudo ha assorbito tutto).
	TestTrue(TEXT("shield"), URTCombatLibrary::ClassifyCombatOutcome(100, 100, 0) == ERTCombatOutcome::ShieldAbsorbed);
	// Hit: HP calano, nessun bonus, non letale.
	TestTrue(TEXT("hit"), URTCombatLibrary::ClassifyCombatOutcome(100, 70, 0) == ERTCombatOutcome::Hit);
	// TerrainBonus: HP calano, bonus altura > 0, non letale.
	TestTrue(TEXT("terrain"), URTCombatLibrary::ClassifyCombatOutcome(100, 55, 15) == ERTCombatOutcome::TerrainBonus);
	// Lethal: HP a 0 (priorita' su tutto).
	TestTrue(TEXT("lethal"), URTCombatLibrary::ClassifyCombatOutcome(30, 0, 0) == ERTCombatOutcome::Lethal);
	// Lethal ha priorita' sul bonus altura.
	TestTrue(TEXT("lethal>terrain"), URTCombatLibrary::ClassifyCombatOutcome(30, 0, 15) == ERTCombatOutcome::Lethal);
	return true;
}

/**
 * Autorita' sull'unita': si comanda solo la propria squadra. Regola parente dell'invariante #6 (quel che non e'
 * tuo non lo vedi e non lo comandi); senza, il click seleziona anche le unita' avversarie e da li' si finisce a
 * pianificarne i turni — oltre a rendere inselezionabili le proprie (vedi ARTPlayerController::OnSelect).
 * Il comportamento completo del click resta verificabile solo in PIE: qui si fissa la regola.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCombatControlAuthorityTest,
	"RefactorTactics.Combat.PlayerControlsOwnUnitsOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCombatControlAuthorityTest::RunTest(const FString&)
{
	TestTrue(TEXT("il giocatore comanda le proprie unita'"), URTCombatLibrary::CanPlayerControlUnit(0, 0));
	TestFalse(TEXT("non comanda quelle avversarie"), URTCombatLibrary::CanPlayerControlUnit(1, 0));

	// Vale simmetricamente per l'altra squadra (il team del giocatore non e' cablato a 0).
	TestTrue(TEXT("simmetrico per il team 1"), URTCombatLibrary::CanPlayerControlUnit(1, 1));
	TestFalse(TEXT("simmetrico: il team 1 non comanda il team 0"), URTCombatLibrary::CanPlayerControlUnit(0, 1));
	return true;
}

/**
 * CP 6.3: la validazione del bersaglio deve essere FAIL-CLOSED. Il difetto che questo test previene esisteva
 * davvero: il controller valutava `bHasLOS = !Grid || HasLineOfSight(...)`, quindi quando la griglia non c'era
 * piu' (dopo CP 6.1/6.2 il GameMode non la spawna) la linea di tiro risultava sempre valida e si poteva
 * bersagliare attraverso i muri. Senza mappa autorevole non si ingaggia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCombatHexTargetingTest,
	"RefactorTactics.Combat.HexTargetingIsFailClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCombatHexTargetingTest::RunTest(const FString&)
{
	const FRTCellId From(0, 0, 0);
	const FRTCellId To(3, 0, 0); // distanza 3

	// Mappa piena, con un muro che blocca la vista a meta' strada (aggiunto al punto 4).
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 4);

	// 1. Senza mappa: NIENTE ingaggio, mai (regressione fail-open).
	TestFalse(TEXT("mappa assente -> non si ingaggia"),
		URTCombatLibrary::CanTargetHexCell(nullptr, From, To, /*RangeCells=*/ 5, ERTLineOfSightPolicy::Required));

	// 2. In portata e con linea libera -> si ingaggia.
	TestTrue(TEXT("in portata e vista libera -> si ingaggia"),
		URTCombatLibrary::CanTargetHexCell(Map, From, To, /*RangeCells=*/ 5, ERTLineOfSightPolicy::Required));

	// 3. Fuori portata -> no (distanza ESAGONALE, non quadrata).
	TestFalse(TEXT("oltre la portata -> no"),
		URTCombatLibrary::CanTargetHexCell(Map, From, To, /*RangeCells=*/ 2, ERTLineOfSightPolicy::Required));

	// 4. Muro sulla traiettoria -> no, pur restando in portata.
	FRTHexCellData Wall(FRTCellId(2, 0, 0));
	Wall.bBlocksLineOfSight = true;
	Map->AddOrUpdateCell(Wall);
	Map->SortCells();
	TestFalse(TEXT("muro sulla linea di tiro -> no"),
		URTCombatLibrary::CanTargetHexCell(Map, From, To, /*RangeCells=*/ 5, ERTLineOfSightPolicy::Required));
	return true;
}

/**
 * Il MOTIVO del rifiuto va distinto, non solo il si/no. Difetto reale osservato in PIE: il controller usava
 * CanTargetHexCell (che verifica portata **e** vista) come se fosse il solo esito della linea di tiro, cosi' un
 * bersaglio semplicemente FUORI PORTATA veniva loggato come «coperto (nessuna linea di tiro)» — su un'arena
 * senza un solo muro. Il messaggio «fuori portata» era diventato irraggiungibile.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCombatTargetReasonTest,
	"RefactorTactics.Combat.HexTargetingReasonDistinguishesRangeFromCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCombatTargetReasonTest::RunTest(const FString&)
{
	const FRTCellId From(0, 0, 0);
	const FRTCellId To(3, 0, 0); // distanza 3

	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 4);

	// Nessuna mappa: motivo dedicato, non "coperto".
	TestTrue(TEXT("mappa assente -> NoMap"),
		URTCombatLibrary::ClassifyHexTargeting(nullptr, From, To, 5, ERTLineOfSightPolicy::Required)
			== ERTHexTargetReason::NoMap);

	// In portata e senza ostacoli: ingaggiabile.
	TestTrue(TEXT("in portata e vista libera -> Ok"),
		URTCombatLibrary::ClassifyHexTargeting(Map, From, To, 5, ERTLineOfSightPolicy::Required)
			== ERTHexTargetReason::Ok);

	// FUORI PORTATA su mappa senza muri: deve dire OutOfRange, MAI NoLineOfSight.
	const ERTHexTargetReason FarReason =
		URTCombatLibrary::ClassifyHexTargeting(Map, From, To, 2, ERTLineOfSightPolicy::Required);
	TestTrue(TEXT("oltre la portata -> OutOfRange"), FarReason == ERTHexTargetReason::OutOfRange);
	TestFalse(TEXT("oltre la portata NON deve risultare 'coperto'"),
		FarReason == ERTHexTargetReason::NoLineOfSight);

	// In portata ma con un muro in mezzo: NoLineOfSight.
	FRTHexCellData Wall(FRTCellId(2, 0, 0));
	Wall.bBlocksLineOfSight = true;
	Map->AddOrUpdateCell(Wall);
	Map->SortCells();
	TestTrue(TEXT("muro in portata -> NoLineOfSight"),
		URTCombatLibrary::ClassifyHexTargeting(Map, From, To, 5, ERTLineOfSightPolicy::Required)
			== ERTHexTargetReason::NoLineOfSight);

	// Il gate booleano resta coerente con la classificazione.
	TestFalse(TEXT("CanTargetHexCell coerente col motivo"),
		URTCombatLibrary::CanTargetHexCell(Map, From, To, 5, ERTLineOfSightPolicy::Required));
	return true;
}

/**
 * `CP 19.3` / `#1124` — il GRUPPO DI CONTROLLO partiziona la squadra.
 *
 * La squadra dice contro chi si combatte; il gruppo dice CHI, fra i giocatori di quella squadra, comanda una
 * data unita'. Sono due domande distinte, e fino a `#1124` il codice sapeva rispondere solo alla prima.
 *
 * ⚠️ **Nella v0.1 questo test non descrive un comportamento visibile.** Un giocatore per squadra significa un
 * solo gruppo, e ogni sua asserzione sulla partizione cade nel caso degenere. Il valore e' che la regola sia
 * ESPRIMIBILE e fissata prima che i posti diventino due: il giorno in cui `SeatsPerTeam` sale, il comando non
 * si decide per inerzia da come `GetAllActorsOfClass` ha restituito gli Actor.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTControlGroupPartitionTest,
	"RefactorTactics.Combat.ControlGroupPartitionsTheTeam",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTControlGroupPartitionTest::RunTest(const FString&)
{
	// --- La partizione: chi finisce in quale gruppo -------------------------------------------------------
	// Con due unita' a testa, le prime due sono del primo giocatore e le altre due del secondo. E' una
	// divisione intera, non un modulo: i gruppi devono essere CONTIGUI, perche' un giocatore comanda una
	// squadretta, non le unita' pari.
	TestEqual(TEXT("la prima unita' e' del primo giocatore"), URTCombatLibrary::ControlGroupForUnit(0, 2), 0);
	TestEqual(TEXT("anche la seconda"), URTCombatLibrary::ControlGroupForUnit(1, 2), 0);
	TestEqual(TEXT("la terza passa al secondo giocatore"), URTCombatLibrary::ControlGroupForUnit(2, 2), 1);
	TestEqual(TEXT("e la quarta resta con lui"), URTCombatLibrary::ControlGroupForUnit(3, 2), 1);

	// La v0.1: due unita' per squadra, due per giocatore. Un solo gruppo, e nessuno perde il comando.
	TestEqual(TEXT("v0.1: entrambe le unita' allo stesso, unico gruppo"),
		URTCombatLibrary::ControlGroupForUnit(0, 2), URTCombatLibrary::ControlGroupForUnit(1, 2));

	// Fail-closed sugli ingressi che non descrivono una partizione. `INDEX_NONE` non e' "gruppo zero": e'
	// "gruppo ignoto", e piu' sotto si verifica che un gruppo ignoto NON dia il comando.
	TestEqual(TEXT("zero unita' per giocatore non partiziona niente"),
		URTCombatLibrary::ControlGroupForUnit(0, 0), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("ne' un numero negativo"),
		URTCombatLibrary::ControlGroupForUnit(0, -1), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("un indice negativo non e' una posizione nella squadra"),
		URTCombatLibrary::ControlGroupForUnit(-1, 2), static_cast<int32>(INDEX_NONE));

	// --- Il comando: la squadra prima, il gruppo poi ------------------------------------------------------
	TestTrue(TEXT("stessa squadra e stesso gruppo: comanda"),
		URTCombatLibrary::CanPlayerControlUnitInGroup(0, 0, 0, 0));
	TestFalse(TEXT("stessa squadra, gruppo diverso: la comanda un altro giocatore"),
		URTCombatLibrary::CanPlayerControlUnitInGroup(0, 1, 0, 0));

	// ⚠️ **La squadra decide per prima, e da sola.** Un gruppo uguale non riapre l'unita' avversaria: senza
	// questo ordine, il giocatore `0` del gruppo `0` comanderebbe il gruppo `0` della squadra nemica.
	TestFalse(TEXT("gruppo uguale non basta a comandare un'avversaria"),
		URTCombatLibrary::CanPlayerControlUnitInGroup(1, 0, 0, 0));

	// Gruppo ignoto da una parte o dall'altra: si rifiuta. Un'unita' mai assegnata non finisce per sbaglio
	// nelle mani di chi ha il gruppo `0`, che e' il default del campo.
	TestFalse(TEXT("unita' senza gruppo assegnato: nessuno la comanda"),
		URTCombatLibrary::CanPlayerControlUnitInGroup(0, INDEX_NONE, 0, 0));
	TestFalse(TEXT("giocatore senza gruppo assegnato: non comanda"),
		URTCombatLibrary::CanPlayerControlUnitInGroup(0, 0, 0, INDEX_NONE));

	// La regola del bot resta quella di `#937`: superare squadra e gruppo non basta se la pianifica il bot.
	TestFalse(TEXT("un compagno del bot non si comanda, gruppo o no"),
		URTCombatLibrary::CanPlayerControlUnitInGroup(0, 0, 0, 0, /*bUnitIsBotControlled=*/true));

	// --- Compatibilita' all'indietro ----------------------------------------------------------------------
	// ⚠️ La regola nuova sostituisce la vecchia nei due call site del `PlayerController`: se in v0.1 le due
	// dessero risposte diverse, il cambio sarebbe una regressione muta sul comando. Con tutti a gruppo `0`
	// devono coincidere su ogni combinazione di squadre.
	for (int32 UnitTeam = 0; UnitTeam <= 1; ++UnitTeam)
	{
		for (int32 PlayerTeam = 0; PlayerTeam <= 1; ++PlayerTeam)
		{
			TestEqual(TEXT("v0.1: la regola nuova risponde come quella vecchia"),
				URTCombatLibrary::CanPlayerControlUnitInGroup(UnitTeam, 0, PlayerTeam, 0),
				URTCombatLibrary::CanPlayerControlUnit(UnitTeam, PlayerTeam));
		}
	}

	return true;
}

/**
 * I DUE RIFIUTI NOTI RESTANO DISTINTI, PERCHE' SUGGERISCONO AZIONI DIVERSE — `#2741`.
 *
 * Non e' fedelta' all'enum: «coperto» si corregge spostandosi di lato, «fuori portata» avvicinandosi.
 * Appiattirli darebbe al giocatore un rifiuto che non gli dice cosa cambiare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRefusalDistinguishesCoverFromRangeTest,
	"RefactorTactics.Combat.RefusalDistinguishesCoverFromRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRefusalDistinguishesCoverFromRangeTest::RunTest(const FString&)
{
	const bool bNoto = true;

	TestEqual(TEXT("una traiettoria interrotta si mostra come COPERTURA"),
		URTCombatLibrary::RefusalForObserver(ERTHexTargetReason::NoLineOfSight, bNoto),
		ERTTargetRefusal::Cover);

	TestEqual(TEXT("oltre la portata si mostra come PORTATA"),
		URTCombatLibrary::RefusalForObserver(ERTHexTargetReason::OutOfRange, bNoto),
		ERTTargetRefusal::Range);

	// ⚠️ **Ridondante rispetto alle due righe sopra, e tenuta apposta come guardia di REGRESSIONE.** Quelle
	// pinnano due costanti distinte, quindi nessuna implementazione puo' soddisfarle e insieme collassare i
	// due esiti: questa riga non puo' cadere da sola. Vale perche' dichiara il REQUISITO — la distinzione —
	// invece dei due valori presi uno alla volta, e sopravvive a una riscrittura che cambiasse le costanti.
	TestNotEqual(TEXT("e i due non collassano l'uno sull'altro"),
		URTCombatLibrary::RefusalForObserver(ERTHexTargetReason::NoLineOfSight, bNoto),
		URTCombatLibrary::RefusalForObserver(ERTHexTargetReason::OutOfRange, bNoto));

	TestEqual(TEXT("un bersaglio ingaggiabile non produce rifiuto"),
		URTCombatLibrary::RefusalForObserver(ERTHexTargetReason::Ok, bNoto),
		ERTTargetRefusal::None);
	return true;
}

/**
 * ⛔ IL CANARY: UN BERSAGLIO IGNOTO E UNA CELLA VUOTA SONO INDISTINGUIBILI — `#2741`, [D-225].
 *
 * 🔴 **Il difetto che questo test esiste per impedire non e' un crash: e' un'informazione CORRETTA
 * consegnata a chi non doveva riceverla.** Chi clicca su una cella apparentemente vuota e riceve
 * «nessuna linea di tiro» ha appena appreso che li' c'e' qualcuno.
 *
 * ⚠️ **E non basta che il rifiuto «non nomini il nemico».** Due messaggi diversi — anche entrambi
 * generici — sono essi stessi il canale: la differenza fra i due E' l'informazione. Per questo il test
 * asserisce l'UGUAGLIANZA fra i due esiti, non la genericita' di ciascuno.
 *
 * 🔑 **Perche' il caso e' reale, corretto il 2026-09-09 (#2755).** Una stesura precedente di questo
 * commento diceva che *«il collider di un'unita' velata resta attivo»*, e citava `RTUnit.cpp:62-63` —
 * cioe' il COSTRUTTORE. Era falso: la stessa `RefreshComponentVisibility`, in coda, chiama
 * `SetActorEnableCollision(bRender)` dal commit `678cc8fc`. Un nemico velato **non** viene colpito dal
 * trace del click, e `RefactorTactics.Veil.HiddenEnemyIsNotPickable` ora lo misura sul percorso reale.
 *
 * Il caso resta reale per una ragione diversa, e piu' stretta: `bKnownToObserver` nasce **`true`**
 * (`RTUnit.h:1392`) e il velo lo corregge nel `Tick` dell'HUD. Fra lo spawn di un'unita' e il primo tick
 * c'e' una finestra in cui la collisione e' accesa su un nemico che l'osservatore non ha mai visto. Il
 * collasso qui sotto e' quindi difesa in profondita' su una finestra breve, non l'unica difesa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRefusalHidesTheUnknownTargetTest,
	"RefactorTactics.Combat.RefusalHidesTheUnknownTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRefusalHidesTheUnknownTargetTest::RunTest(const FString&)
{
	const bool bIgnoto = false;

	// La cella VUOTA: nessuna geometria da valutare, quindi il verdetto interno non e' nemmeno un rifiuto.
	const ERTTargetRefusal CellaVuota =
		URTCombatLibrary::RefusalForObserver(ERTHexTargetReason::Ok, bIgnoto);

	// Le due situazioni in cui un nemico IGNOTO produce un verdetto interno diverso — ed e' precisamente
	// li' che il canale si aprirebbe.
	const ERTTargetRefusal IgnotoCoperto =
		URTCombatLibrary::RefusalForObserver(ERTHexTargetReason::NoLineOfSight, bIgnoto);
	const ERTTargetRefusal IgnotoLontano =
		URTCombatLibrary::RefusalForObserver(ERTHexTargetReason::OutOfRange, bIgnoto);

	TestEqual(TEXT("un nemico ignoto e coperto e' indistinguibile da una cella vuota"),
		IgnotoCoperto, CellaVuota);
	TestEqual(TEXT("un nemico ignoto e lontano e' indistinguibile da una cella vuota"),
		IgnotoLontano, CellaVuota);
	TestEqual(TEXT("e i due casi ignoti non si distinguono nemmeno FRA LORO"),
		IgnotoCoperto, IgnotoLontano);

	// 🔴 E la meta' che impedisce al test di essere soddisfatto da un'implementazione degenere: se
	// `RefusalForObserver` restituisse sempre `Nothing`, le tre righe sopra passerebbero e la feature non
	// esisterebbe. Con lo STESSO verdetto interno, un bersaglio NOTO deve dare qualcosa di diverso.
	TestNotEqual(TEXT("ma su un bersaglio NOTO lo stesso verdetto interno dice qualcosa"),
		URTCombatLibrary::RefusalForObserver(ERTHexTargetReason::NoLineOfSight, /*bKnown=*/ true),
		IgnotoCoperto);
	return true;
}

/**
 * SENZA MAPPA NON SI AFFERMA NIENTE — `#2741`, fail-closed.
 *
 * `NoMap` non e' «non c'e' linea di tiro»: e' «non ho verificato». Mostrarlo come copertura sarebbe
 * affermare al giocatore qualcosa che nessuno ha valutato — la stessa ragione per cui
 * `ClassifyHexTargeting` restituisce `NoMap` invece di indovinare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRefusalWithoutMapAffirmsNothingTest,
	"RefactorTactics.Combat.RefusalWithoutMapAffirmsNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRefusalWithoutMapAffirmsNothingTest::RunTest(const FString&)
{
	TestEqual(TEXT("senza mappa autorevole non si mostra un motivo"),
		URTCombatLibrary::RefusalForObserver(ERTHexTargetReason::NoMap, /*bKnown=*/ true),
		ERTTargetRefusal::Nothing);
	return true;
}

/**
 * 🔴 **IL LOG DICE IL LIMITE APPLICATO, NON QUELLO DICHIARATO** — `#2766`.
 *
 * ## Il difetto
 *
 * `ClassifyHexTargeting` confronta la distanza con `EffectiveTargetingRange`, che il Fumo cappa a **2**.
 * Il log stampava `Ability->RangeCells`, cioe' **5**. Un bersaglio a distanza 3 produceva quindi
 * *«fuori portata (max 5)»*, e chi legge conclude che il classificatore e' rotto — `3 <= 5` — e cerca un
 * difetto che non c'e'. E' successo davvero, in una code review su `#2754`.
 *
 * ## Le DUE sponde, e perche' servono entrambe
 *
 * Il caso e' definito da una congiunzione, non da una disuguaglianza:
 *
 *   - `distanza <= RangeCells` — altrimenti sarebbe un fuori portata ORDINARIO, e non ci sarebbe niente
 *     di ingannevole da correggere;
 *   - `distanza > EffectiveRange` — altrimenti il rifiuto non ci sarebbe affatto.
 *
 * ⚠️ Un test che asserisse solo la seconda passerebbe anche con `RangeCells = 2`, dove dichiarata ed
 * effettiva coincidono e il log non ha mai mentito.
 *
 * ## Il controllo positivo
 *
 * Il primo blocco misura la stessa mappa **senza** fumo: il bersaglio e' ingaggiabile e il testo resta a
 * un numero solo. Senza, un'implementazione che stampasse sempre due portate sarebbe verde, e il caso
 * ordinario guadagnerebbe una precisazione che non ha ragione d'essere.
 *
 * ## Cosa questo test NON copre, ed e' dichiarato
 *
 * ⛔ Il **sito di chiamata** in `ARTPlayerController::OnSelect` non e' esercitato qui: comporre il log
 * richiede un controller, un'unita' selezionata e un click, cioe' PIE. Il test lega i tre anelli che si
 * possono legare in Automation — `EffectiveTargetingRange` -> `ClassifyHexTargeting` -> il testo — e la
 * riga del controller passa a `OutOfRangeDiagnostic` esattamente questi due valori.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOutOfRangeDiagnosticNamesAppliedLimitTest,
	"RefactorTactics.Combat.OutOfRangeDiagnosticNamesTheAppliedLimit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOutOfRangeDiagnosticNamesAppliedLimitTest::RunTest(const FString&)
{
	const FRTCellId From(0, 0, 0);
	const FRTCellId To(3, 0, 0);
	const int32 Dichiarata = 5;

	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 4);
	if (!TestNotNull(TEXT("arena di prova"), Map)) { return false; }

	// ── CONTROLLO POSITIVO: senza fumo non c'e' nulla da correggere.
	TestEqual(TEXT("premessa: senza terreno che cappa, effettiva == dichiarata"),
		URTTerrainLibrary::EffectiveTargetingRange(Map, From, To, Dichiarata), Dichiarata);
	TestTrue(TEXT("premessa: e il bersaglio e' ingaggiabile"),
		URTCombatLibrary::ClassifyHexTargeting(Map, From, To, Dichiarata, ERTLineOfSightPolicy::Required)
			== ERTHexTargetReason::Ok);
	TestEqual(TEXT("senza cap il log resta a UN numero"),
		URTCombatLibrary::OutOfRangeDiagnostic(Dichiarata, Dichiarata),
		FString(TEXT("fuori portata (max 5)")));

	// ── Il fumo a meta' strada. Non blocca la vista: cappa la distanza.
	FRTHexCellData Fumo(FRTCellId(2, 0, 0));
	Fumo.Surface = ERTHexSurface::Smoke;
	Map->AddOrUpdateCell(Fumo);
	Map->SortCells();

	const int32 Effettiva = URTTerrainLibrary::EffectiveTargetingRange(Map, From, To, Dichiarata);
	const int32 Distanza  = URTHexLibrary::HexDistance(From, To);

	// ── LE DUE SPONDE che definiscono il caso.
	TestEqual(TEXT("il fumo cappa la portata a 2"), Effettiva, 2);
	TestTrue(TEXT("sponda 1: la distanza sta DENTRO la portata dichiarata"), Distanza <= Dichiarata);
	TestTrue(TEXT("sponda 2: ma oltre quella effettiva"), Distanza > Effettiva);
	TestTrue(TEXT("ed e' per questo che il classificatore rifiuta"),
		URTCombatLibrary::ClassifyHexTargeting(Map, From, To, Dichiarata, ERTLineOfSightPolicy::Required)
			== ERTHexTargetReason::OutOfRange);

	// ── 🔴 Il cuore.
	const FString Diagnostico = URTCombatLibrary::OutOfRangeDiagnostic(Dichiarata, Effettiva);
	TestTrue(TEXT("il log nomina il limite APPLICATO"), Diagnostico.Contains(TEXT("max 2")));
	TestTrue(TEXT("e dice anche quella dichiarata, che senza contesto sembrerebbe smentirlo"),
		Diagnostico.Contains(TEXT("5 dichiarata")));
	TestFalse(TEXT("e non spaccia piu' la dichiarata per il limite: era esattamente la stringa vecchia"),
		Diagnostico.Contains(TEXT("max 5")));

	// ── E al GIOCATORE non cambia niente, che e' una decisione e non una dimenticanza: il cap limita la
	//    distanza, non la traiettoria, quindi «avvicinati» resta l'azione giusta anche col fumo.
	TestEqual(TEXT("il rifiuto mostrato resta quello di distanza"),
		URTCombatLibrary::RefusalForObserver(ERTHexTargetReason::OutOfRange, /*bNoto=*/ true),
		ERTTargetRefusal::Range);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
