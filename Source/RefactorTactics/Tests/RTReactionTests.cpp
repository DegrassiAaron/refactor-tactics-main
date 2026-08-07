#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTReactionLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Infrastruttura dello slot reazione (CP 5.1): trigger valutato sullo snapshot del Blast, nessuna attesa nel
 * resolver, massimo un'attivazione per turno. `Action.Counter` e' il veicolo reale del checkpoint (rileva e
 * registra il trigger), ma il contrattacco da 16 danni NON e' ancora applicato: arriva con CP 5.2, sulla
 * stessa pipeline — vedi RTCatalogLibrary.cpp.
 */
namespace
{
	UWorld* MakeReactionWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyReactionWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTHexMapActor* SpawnReactionMap(UWorld* World, int32 Radius = 6)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return Actor;
	}

	ARTUnit* SpawnReactionUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // i piani li scriviamo noi
		U->ConfigureAsArchetype(ERTArchetype::Ranger); // attacco base a colpo singolo (Ranger.Shot, 25, range 6)
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell; // fermo: i test guardano il Blast, non il movimento
		return U;
	}

	/**
	 * Sostituisce lo scatto del Ranger (indice 3, inutilizzato nei test qui sotto) con la reazione, invece di
	 * accodarla con `Abilities.Add`: `AbilityCooldowns` (privato, parallelo a `Abilities`) e' dimensionato da
	 * `ConfigureAsArchetype` sul conteggio ORIGINALE e non si allarga da solo — un'abilita' accodata dopo finirebbe
	 * fuori indice, e `ConsumeAbility`/`GetAbilityCooldown` la ignorerebbero in silenzio (nessun cooldown
	 * osservabile). Sovrascrivere un indice esistente resta dentro i limiti dell'array senza toccare codice
	 * condiviso per un'esigenza solo di test.
	 *
	 * `ARTUnit::ConsumeAbility`/`GetAbilityCooldown` leggono inoltre il campo LEGACY `URTActionData::CooldownTurns`,
	 * non `Def.CooldownTurns` (stessa disciplina di `RTHeroCatalogLibrary::MakeHeroAction`, che li specchia
	 * entrambi): senza mirroring il cooldown della reazione non scatterebbe mai, in silenzio.
	 */
	int32 AddReactionAbility(ARTUnit* Unit, const TCHAR* ActionId)
	{
		if (!Unit || !Unit->Abilities.IsValidIndex(3)) { return INDEX_NONE; }
		URTActionData* Ability = NewObject<URTActionData>(Unit);
		Ability->Def = URTCatalogLibrary::FindCoreAction(FName(ActionId));
		Ability->CooldownTurns = Ability->Def.CooldownTurns;
		Ability->RangeCells = Ability->Def.RangeCells;
		Ability->Power = 0; // Action.Counter non dichiara ancora un effetto Damage (CP 5.2): niente danno fantasma
		Unit->Abilities[3] = Ability;
		return 3;
	}

	void RunReactionTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	int32 CountSlotReactionOutcome(const ARTTurnManager* TM, ERTReactionOutcome Outcome)
	{
		int32 N = 0;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Reaction && E.Outcome == static_cast<uint8>(Outcome)) { ++N; }
		}
		return N;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionSingleActivationTest,
	"RefactorTactics.Reactions.SingleActivation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionSingleActivationTest::RunTest(const FString&)
{
	// Nome vincolante del catalogo v0.1. Il Reactor pianifica `Action.Counter` come reazione (slot dedicato,
	// non l'azione principale: qui il Reactor non pianifica nulla in Blast) e viene colpito UNA volta da un
	// attacco diretto: il trigger scatta e finisce nel TurnLog come attivato.
	UWorld* World = MakeReactionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnReactionMap(World);

	ARTUnit* Reactor = SpawnReactionUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Attacker = SpawnReactionUnit(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Reactor"), Reactor) || !TestNotNull(TEXT("Attacker"), Attacker) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyReactionWorld(World);
		return false;
	}

	const int32 CounterIdx = AddReactionAbility(Reactor, TEXT("Action.Counter"));
	Reactor->PlannedReactionAbility = CounterIdx;
	Reactor->PlannedAbilityIndex = INDEX_NONE; // niente azione principale: solo la reazione, pronta

	Attacker->PlannedAbilityIndex = 0; // attacco base (Ranger.Shot, colpo singolo)
	Attacker->PlannedAttackTarget = Reactor;

	RunReactionTurn(TM);

	TestEqual(TEXT("una attivazione registrata"), CountSlotReactionOutcome(TM, ERTReactionOutcome::Activated), 1);
	TestEqual(TEXT("nessun'altra voce di reazione"), CountSlotReactionOutcome(TM, ERTReactionOutcome::NotTriggered), 0);
	// Il cooldown POST-attivazione non si verifica qui: `AbilityCooldowns` (privato) si dimensiona in
	// `ARTUnit::BeginPlay`, che questi world di test (`UWorld::CreateWorld` senza `World->BeginPlay()`, lo
	// stesso schema di ogni altro file di test in questa suite) non arrivano mai a chiamare — la lettura
	// resterebbe 0 indipendentemente dal fatto che il consumo sia avvenuto. Non e' un limite di CP 5.1.

	DestroyReactionWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionNoTriggerLoggedTest,
	"RefactorTactics.Reactions.NotTriggeredIsLogged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionNoTriggerLoggedTest::RunTest(const FString&)
{
	// L'attivazione — o la non-attivazione — compare SEMPRE nel TurnLog: un Reactor pronto ma mai colpito
	// non sparisce in silenzio, registra "nessun trigger".
	UWorld* World = MakeReactionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnReactionMap(World);

	ARTUnit* Reactor = SpawnReactionUnit(World, 0, FRTCellId(0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Reactor"), Reactor) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyReactionWorld(World);
		return false;
	}

	const int32 CounterIdx = AddReactionAbility(Reactor, TEXT("Action.Counter"));
	Reactor->PlannedReactionAbility = CounterIdx;
	Reactor->PlannedAbilityIndex = INDEX_NONE;

	RunReactionTurn(TM);

	TestEqual(TEXT("nessuna attivazione: nessuno ha colpito"), CountSlotReactionOutcome(TM, ERTReactionOutcome::Activated), 0);
	TestEqual(TEXT("registrata come non innescata"), CountSlotReactionOutcome(TM, ERTReactionOutcome::NotTriggered), 1);
	// Non si verifica qui il cooldown (vedi il commento in SingleActivation): in questo world di test resta
	// sempre 0, attivazione o no, quindi non distinguerebbe "non consumato" da "non misurabile".

	DestroyReactionWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionMultipleTriggersStillOnceTest,
	"RefactorTactics.Reactions.MultipleTriggersStillOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionMultipleTriggersStillOnceTest::RunTest(const FString&)
{
	// Nome vincolante del catalogo v0.1. Il Reactor viene colpito da DUE attacchi diretti distinti nello
	// stesso Blast: due trigger validi, ma una reazione sola — non due voci nel TurnLog, non doppio consumo.
	UWorld* World = MakeReactionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnReactionMap(World);

	ARTUnit* Reactor = SpawnReactionUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* AttackerA = SpawnReactionUnit(World, 1, FRTCellId(1, 0));
	ARTUnit* AttackerB = SpawnReactionUnit(World, 1, FRTCellId(-1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Reactor"), Reactor) || !TestNotNull(TEXT("AttackerA"), AttackerA)
		|| !TestNotNull(TEXT("AttackerB"), AttackerB) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyReactionWorld(World);
		return false;
	}

	const int32 CounterIdx = AddReactionAbility(Reactor, TEXT("Action.Counter"));
	Reactor->PlannedReactionAbility = CounterIdx;
	Reactor->PlannedAbilityIndex = INDEX_NONE;

	AttackerA->PlannedAbilityIndex = 0;
	AttackerA->PlannedAttackTarget = Reactor;
	AttackerB->PlannedAbilityIndex = 0;
	AttackerB->PlannedAttackTarget = Reactor;

	RunReactionTurn(TM);

	TestEqual(TEXT("una sola attivazione, anche con due trigger validi"),
		CountSlotReactionOutcome(TM, ERTReactionOutcome::Activated), 1);

	DestroyReactionWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionSprintBlocksTest,
	"RefactorTactics.Reactions.SprintBlocksReaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionSprintBlocksTest::RunTest(const FString&)
{
	// `FRTActionDef::bAllowsReaction`: chi corre a perdifiato (Action.Sprint) non para. Il trigger sarebbe
	// valido (colpo diretto), ma la reazione resta ferma — "non disponibile", non "attivata".
	UWorld* World = MakeReactionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnReactionMap(World, /*Radius*/ 8);

	ARTUnit* Sprinter = SpawnReactionUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Attacker = SpawnReactionUnit(World, 1, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Sprinter"), Sprinter) || !TestNotNull(TEXT("Attacker"), Attacker) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyReactionWorld(World);
		return false;
	}

	const int32 CounterIdx = AddReactionAbility(Sprinter, TEXT("Action.Counter"));
	Sprinter->PlannedReactionAbility = CounterIdx;

	URTActionData* Sprint = NewObject<URTActionData>(Sprinter);
	Sprint->Def = URTCatalogLibrary::FindCoreAction(TEXT("Action.Sprint"));
	Sprinter->Abilities.Add(Sprint);
	Sprinter->PlannedDashAbility = Sprinter->Abilities.Num() - 1;
	Sprinter->PlannedDashCell = FRTCellId(1, 0); // scatto breve, resta a tiro dell'attaccante
	Sprinter->PlannedAbilityIndex = INDEX_NONE;
	Sprinter->PlannedCell = Sprinter->Cell;

	Attacker->PlannedAbilityIndex = 0;
	Attacker->PlannedAttackTarget = Sprinter;

	RunReactionTurn(TM);

	TestEqual(TEXT("nessuna attivazione: lo Sprint la vieta"), CountSlotReactionOutcome(TM, ERTReactionOutcome::Activated), 0);
	TestEqual(TEXT("registrata come non disponibile"), CountSlotReactionOutcome(TM, ERTReactionOutcome::Unavailable), 1);

	DestroyReactionWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionNoResolverWaitTest,
	"RefactorTactics.Reactions.NoResolverWait",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionNoResolverWaitTest::RunTest(const FString&)
{
	// Invariante #3: il trigger si valuta su un ARRAY, non su un evento a cui reagire mentre il turno gira.
	// `EvaluateReactionTrigger` e' una funzione PURA — nessun Actor, nessun UWorld, nessun timer: non ha
	// letteralmente modo di introdurre un Delay/timeline/montage nel resolver. Lo dimostra chiamandola
	// direttamente, senza mai passare da un ARTTurnManager.
	TArray<FRTHexAttackHit> Hits;
	Hits.Add(FRTHexAttackHit(/*Attacker*/ 1, /*Target*/ 0, /*Power*/ 25, /*IntentIndex*/ 0));

	TArray<FRTHexAttackIntent> Intents;
	FRTHexAttackIntent Direct;
	Direct.AttackerId = 1;
	Direct.TargetId = 0;
	Direct.Shape = ERTAbilityShape::Single;
	Intents.Add(Direct);

	TestTrue(TEXT("colpo diretto sul bersaglio -> trigger scattato"),
		URTReactionLibrary::EvaluateReactionTrigger(ERTReactionTrigger::HitByDirectAttack, /*SelfId*/ 0, Hits, Intents));
	TestFalse(TEXT("stesso colpo, ma non e' il MIO bersaglio -> nessun trigger"),
		URTReactionLibrary::EvaluateReactionTrigger(ERTReactionTrigger::HitByDirectAttack, /*SelfId*/ 2, Hits, Intents));

	// Un colpo ad AREA sullo stesso bersaglio non e' "diretto": Counter/Deflect non se ne accorgono.
	TArray<FRTHexAttackIntent> AreaIntents;
	FRTHexAttackIntent Area;
	Area.AttackerId = 1;
	Area.TargetId = 0;
	Area.Shape = ERTAbilityShape::Area;
	AreaIntents.Add(Area);
	TestFalse(TEXT("colpo ad area -> non e' un trigger diretto"),
		URTReactionLibrary::EvaluateReactionTrigger(ERTReactionTrigger::HitByDirectAttack, /*SelfId*/ 0, Hits, AreaIntents));

	// Nessun colpo -> nessun trigger, senza bisogno di un turno per saperlo.
	TestFalse(TEXT("nessun colpo -> nessun trigger"),
		URTReactionLibrary::EvaluateReactionTrigger(ERTReactionTrigger::HitByDirectAttack, /*SelfId*/ 0, {}, {}));

	// Nessun trigger dichiarato (azione non di reazione, o dato incompleto) -> mai vero, qualunque sia Hits.
	TestFalse(TEXT("trigger None -> mai scatta"),
		URTReactionLibrary::EvaluateReactionTrigger(ERTReactionTrigger::None, /*SelfId*/ 0, Hits, Intents));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
