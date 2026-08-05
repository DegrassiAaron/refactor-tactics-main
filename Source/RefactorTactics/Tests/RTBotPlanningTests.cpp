#include "Misc/AutomationTest.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Core/RTTypes.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Grid/RTGridLibrary.h"
#include "Ability/RTAbilityData.h"

// Test d'INTEGRAZIONE del bot: costruisce un mondo di gioco minimale (nessun rendering/tick), spawna un 2v2,
// invoca la pianificazione (ARTTurnManager::PlanBotsForTest) e verifica le DECISIONI del bot tramite i campi
// Planned*. Complementare ai test puri: qui si esercita PlanBots end-to-end senza PIE manuale.

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Crea un World di gioco minimale (nessun tick/rendering). Il chiamante deve chiamare DestroyBotTestWorld.
	UWorld* MakeBotTestWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyBotTestWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	// Spawna un'unita' configurata per archetipo, sulla cella data. Team 1 = bot (come in RTGameMode::SpawnUnit).
	ARTUnit* SpawnTestUnit(UWorld* World, int32 TeamId, ERTArchetype Arch, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = (TeamId == 1);
		U->ConfigureAsArchetype(Arch);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 200.f);
		return U;
	}

	struct FBotScenario
	{
		ARTTurnManager* TM = nullptr;
		ARTUnit* BotRanger = nullptr;   // team 1
		ARTUnit* BotGuardian = nullptr; // team 1
		ARTUnit* FoeRanger = nullptr;   // team 0
		ARTUnit* FoeGuardian = nullptr; // team 0
	};

	// Scenario 2v2 standard: giocatore (team 0) e bot (team 1), ciascuno Ranger + Guardian, + il TurnManager.
	FBotScenario Make2v2(UWorld* World)
	{
		FBotScenario S;
		S.FoeRanger   = SpawnTestUnit(World, 0, ERTArchetype::Ranger,   FRTCellId(2, 2));
		S.FoeGuardian = SpawnTestUnit(World, 0, ERTArchetype::Guardian, FRTCellId(2, 4));
		S.BotRanger   = SpawnTestUnit(World, 1, ERTArchetype::Ranger,   FRTCellId(7, 7));
		S.BotGuardian = SpawnTestUnit(World, 1, ERTArchetype::Guardian, FRTCellId(7, 5));
		S.TM = World ? World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass()) : nullptr;
		return S;
	}

	// Vero se l'unita' ha pianificato una qualsiasi azione (muoversi, attaccare o scattare).
	bool HasPlannedAction(const ARTUnit* U)
	{
		return U && (U->PlannedCell != U->Cell || U->PlannedAttackTarget != nullptr || U->PlannedDashAbility != INDEX_NONE);
	}
}

// Smoke: l'infrastruttura World funziona e il bot pianifica (valida spawn + GetAllActorsOfClass + PlanBots + Planned*).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotPlanningSmokeTest,
	"RefactorTactics.Bot.PlanningSmoke",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotPlanningSmokeTest::RunTest(const FString&)
{
	UWorld* World = MakeBotTestWorld();
	TestNotNull(TEXT("World creato"), World);
	if (!World) { return false; }

	FBotScenario S = Make2v2(World);
	TestNotNull(TEXT("TurnManager spawnato"), S.TM);
	TestNotNull(TEXT("Bot Ranger spawnato"), S.BotRanger);
	if (S.TM) { S.TM->PlanBotsForTest(); }

	// Con nemici presenti nel mondo, almeno un bot deve pianificare un'azione (conferma che PlanBots li ha visti).
	TestTrue(TEXT("almeno un bot ha pianificato un'azione"),
		HasPlannedAction(S.BotRanger) || HasPlannedAction(S.BotGuardian));

	DestroyBotTestWorld(World);
	return true;
}

// Panic: un kiter (Ranger, standoff 4) con un nemico entro standoff/2 fugge SENZA attaccare.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotPanicTest,
	"RefactorTactics.Bot.PlanningPanicKiter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotPanicTest::RunTest(const FString&)
{
	UWorld* World = MakeBotTestWorld();
	TestNotNull(TEXT("World creato"), World);
	if (!World) { return false; }

	ARTUnit* BotRanger = SpawnTestUnit(World, 1, ERTArchetype::Ranger, FRTCellId(5, 5));
	// Nemico mischia a distanza 1 (<= standoff/2 = 2) -> panic.
	SpawnTestUnit(World, 0, ERTArchetype::Guardian, FRTCellId(5, 6));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (TM) { TM->PlanBotsForTest(); }

	const FRTCellId Threat(5, 6);
	const bool bFled =
		BotRanger->PlannedDashAbility != INDEX_NONE // fuga con scatto difensivo
		|| URTGridLibrary::ManhattanDistance(BotRanger->PlannedCell, Threat)
		   > URTGridLibrary::ManhattanDistance(BotRanger->Cell, Threat); // oppure si allontana a piedi
	TestTrue(TEXT("panic: il kiter fugge dalla minaccia"), bFled);
	TestTrue(TEXT("panic: nessun attacco pianificato"), BotRanger->PlannedAttackTarget == nullptr);

	DestroyBotTestWorld(World);
	return true;
}

// Support: un Guardian ferito (< meta' HP) con la Barriera (self-target) pronta usa il supporto, non attacca.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotSupportTest,
	"RefactorTactics.Bot.PlanningSupportWhenHurt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotSupportTest::RunTest(const FString&)
{
	UWorld* World = MakeBotTestWorld();
	TestNotNull(TEXT("World creato"), World);
	if (!World) { return false; }

	ARTUnit* BotGuardian = SpawnTestUnit(World, 1, ERTArchetype::Guardian, FRTCellId(5, 5));
	SpawnTestUnit(World, 0, ERTArchetype::Ranger, FRTCellId(5, 7)); // un nemico in vista (contesto)
	BotGuardian->ApplyCombatState(50, BotGuardian->Shield); // Health 50 < 140/2 -> ferito
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (TM) { TM->PlanBotsForTest(); }

	const URTAbilityData* Planned = BotGuardian->GetAbility(BotGuardian->PlannedAbilityIndex);
	TestNotNull(TEXT("support: un'abilita' e' pianificata"), Planned);
	TestTrue(TEXT("support: e' self-target (Barriera)"), Planned != nullptr && Planned->bSelfTarget);
	TestTrue(TEXT("support: nessun bersaglio d'attacco"), BotGuardian->PlannedAttackTarget == nullptr);

	DestroyBotTestWorld(World);
	return true;
}

// Tuning: il peso WThreat del TurnManager cambia la decisione. Con WThreat alto il Guardian resta lontano dal
// nemico (avanzare lo esporrebbe); con WThreat 0 la mischia chiude la distanza. (Scatto disabilitato per isolare
// la scelta di movimento normale.)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotTuningTest,
	"RefactorTactics.Bot.PlanningWThreatTuning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotTuningTest::RunTest(const FString&)
{
	UWorld* World = MakeBotTestWorld();
	TestNotNull(TEXT("World creato"), World);
	if (!World) { return false; }

	ARTUnit* BotGuardian = SpawnTestUnit(World, 1, ERTArchetype::Guardian, FRTCellId(5, 5));
	SpawnTestUnit(World, 0, ERTArchetype::Guardian, FRTCellId(5, 1)); // nemico mischia (gittata 3)
	// Rimuove lo scatto (Carica): il dash-avvicinamento non e' pesato da WThreat e mascherebbe l'effetto.
	// Senza dash il bot decide col MOVIMENTO normale, dove WThreat determina resta-vs-avanza.
	const int32 DashIdx = BotGuardian->FindDashAbilityIndex();
	if (DashIdx != INDEX_NONE) { BotGuardian->Abilities.RemoveAt(DashIdx); }
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	TestNotNull(TEXT("TurnManager spawnato"), TM);
	if (!TM) { DestroyBotTestWorld(World); return false; }

	const FRTCellId Foe(5, 1);
	// WThreat enorme: avanzare espone -> resta lontano.
	TM->WThreat = 100000;
	TM->PlanBotsForTest();
	const int32 DistHighThreat = URTGridLibrary::ManhattanDistance(BotGuardian->PlannedCell, Foe);
	// WThreat nullo: la minaccia non conta -> la mischia chiude.
	TM->WThreat = 0;
	TM->PlanBotsForTest();
	const int32 DistLowThreat = URTGridLibrary::ManhattanDistance(BotGuardian->PlannedCell, Foe);

	TestTrue(TEXT("WThreat basso avvicina il Guardian al nemico piu' che WThreat alto"),
		DistLowThreat < DistHighThreat);

	DestroyBotTestWorld(World);
	return true;
}

// Il dash-avvicinamento deve rispettare WThreat: con la minaccia alta il bot NON scatta in una cella esposta.
// Qui il dash resta disponibile e si verifica la posizione EFFETTIVA (cella di scatto se scatta, altrimenti move).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotDashThreatTest,
	"RefactorTactics.Bot.PlanningDashRespectsThreat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotDashThreatTest::RunTest(const FString&)
{
	UWorld* World = MakeBotTestWorld();
	TestNotNull(TEXT("World creato"), World);
	if (!World) { return false; }

	ARTUnit* BotGuardian = SpawnTestUnit(World, 1, ERTArchetype::Guardian, FRTCellId(5, 5)); // con Carica (dash)
	SpawnTestUnit(World, 0, ERTArchetype::Guardian, FRTCellId(5, 1)); // nemico mischia (gittata 3)
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	TestNotNull(TEXT("TurnManager spawnato"), TM);
	if (!TM) { DestroyBotTestWorld(World); return false; }

	const FRTCellId Foe(5, 1);
	// Posizione effettiva del bot: la cella di scatto se scatta, altrimenti la cella di movimento.
	auto Effective = [](const ARTUnit* U) -> FRTCellId
	{
		return (U->PlannedDashAbility != INDEX_NONE) ? U->PlannedDashCell : U->PlannedCell;
	};

	TM->WThreat = 100000; // minaccia enorme: scattare in una cella esposta deve essere rifiutato
	TM->PlanBotsForTest();
	const int32 DistHigh = URTGridLibrary::ManhattanDistance(Effective(BotGuardian), Foe);
	TM->WThreat = 0;      // minaccia ignorata: il bot chiude col dash
	TM->PlanBotsForTest();
	const int32 DistLow = URTGridLibrary::ManhattanDistance(Effective(BotGuardian), Foe);

	TestTrue(TEXT("col dash: WThreat alto tiene il bot piu' lontano (niente scatto in cella esposta)"),
		DistLow < DistHigh);

	DestroyBotTestWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
