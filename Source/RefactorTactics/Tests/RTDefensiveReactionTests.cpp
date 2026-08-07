#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTCombatResolver.h"
#include "Core/RTGameplayTags.h"
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
 * Le cinque difensive del catalogo v0.1 §4 (CP 5.2). ATTENZIONE alla colonna «Slot»: solo `Counter` e
 * `Deflect` sono REAZIONI (slot dedicato, trigger sullo snapshot del Blast); `Brace`, `Shield` e `Cleanse`
 * sono azioni PRINCIPALI che si dichiarano e basta. Stare nella stessa sezione del catalogo non le rende
 * lo stesso tipo di cosa.
 */
namespace
{
	UWorld* MakeDefWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyDefWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTHexMapActor* SpawnDefMap(UWorld* World, int32 Radius = 6)
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

	/** Ranger: attacco base a colpo SINGOLO da 25, portata 6 — il "danno diretto" su cui i trigger reagiscono. */
	ARTUnit* SpawnDefUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false;
		U->ConfigureAsArchetype(ERTArchetype::Ranger);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell; // fermo: questi test guardano il Blast
		return U;
	}

	/**
	 * Sostituisce lo scatto del Ranger (indice 3) invece di accodare: `AbilityCooldowns` e' dimensionato sul
	 * conteggio originale delle abilita' e non si allarga da solo. Specchia anche i campi legacy che
	 * `ARTTurnManager` legge ancora (`CooldownTurns`/`RangeCells`), come fa `MakeHeroAction` per gli eroi, e
	 * azzera `Power` (default legacy 30) per le azioni che non dichiarano danno.
	 */
	int32 AddDefAbility(ARTUnit* Unit, const TCHAR* ActionId, int32 SlotIndex = 3)
	{
		if (!Unit || !Unit->Abilities.IsValidIndex(SlotIndex)) { return INDEX_NONE; }
		URTActionData* Ability = NewObject<URTActionData>(Unit);
		Ability->Def = URTCatalogLibrary::FindCoreAction(FName(ActionId));
		Ability->CooldownTurns = Ability->Def.CooldownTurns;
		Ability->RangeCells = Ability->Def.RangeCells;
		Ability->Power = 0;
		for (const FRTActionEffectSpec& Spec : Ability->Def.Effects)
		{
			if (Spec.Effect == ERTActionEffect::Damage) { Ability->Power = Spec.Amount; break; }
		}
		Unit->Abilities[SlotIndex] = Ability;
		return SlotIndex;
	}

	void RunDefTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	int32 CountReactionOutcome(const ARTTurnManager* TM, ERTReactionOutcome Outcome)
	{
		int32 N = 0;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Reaction && E.Outcome == static_cast<uint8>(Outcome)) { ++N; }
		}
		return N;
	}

	int32 CountCombatEntries(const ARTTurnManager* TM)
	{
		int32 N = 0;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Combat) { ++N; }
		}
		return N;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDefensivesMatchCatalogTest,
	"RefactorTactics.Reactions.DefensivesMatchCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDefensivesMatchCatalogTest::RunTest(const FString&)
{
	// Numeri e SLOT della tabella §4. Lo slot e' la parte che si sbaglia piu' facilmente: tre delle cinque
	// difensive sono azioni principali, non reazioni.
	struct FExpected { const TCHAR* Id; int32 Priority; int32 Cooldown; ERTActionSlot Slot; ERTMatchPhase Phase; };
	const FExpected Expected[] = {
		{ TEXT("Action.Deflect"), 15, 2, ERTActionSlot::Reaction, ERTMatchPhase::Blast },
		{ TEXT("Action.Counter"), 20, 2, ERTActionSlot::Reaction, ERTMatchPhase::Blast },
		{ TEXT("Action.Cleanse"), 25, 2, ERTActionSlot::Main,     ERTMatchPhase::Blast },
		{ TEXT("Action.Brace"),   30, 1, ERTActionSlot::Main,     ERTMatchPhase::Prep  },
		{ TEXT("Action.Shield"),  35, 2, ERTActionSlot::Main,     ERTMatchPhase::Prep  },
	};

	for (const FExpected& E : Expected)
	{
		const FRTActionDef Def = URTCatalogLibrary::FindCoreAction(FName(E.Id));
		if (!TestTrue(FString::Printf(TEXT("%s e' nel catalogo"), E.Id), Def.ActionId == FName(E.Id))) { continue; }
		TestEqual(FString::Printf(TEXT("%s: priorita'"), E.Id), Def.Priority, E.Priority);
		TestEqual(FString::Printf(TEXT("%s: cooldown"), E.Id), Def.CooldownTurns, E.Cooldown);
		TestTrue(FString::Printf(TEXT("%s: slot"), E.Id), Def.Slot == E.Slot);
		TestTrue(FString::Printf(TEXT("%s: macro-fase"), E.Id),
			URTCatalogLibrary::MapResolutionPhase(Def.ResolutionPhase) == E.Phase);
	}

	// Solo le reazioni dichiarano un trigger; le principali no — altrimenti il loop delle reazioni le
	// raccoglierebbe per sbaglio.
	TestTrue(TEXT("Counter ha un trigger"),
		URTCatalogLibrary::FindCoreAction(TEXT("Action.Counter")).ReactionTrigger == ERTReactionTrigger::HitByDirectAttack);
	TestTrue(TEXT("Deflect ha un trigger"),
		URTCatalogLibrary::FindCoreAction(TEXT("Action.Deflect")).ReactionTrigger == ERTReactionTrigger::HitByDirectAttack);
	TestTrue(TEXT("Brace non e' una reazione: nessun trigger"),
		URTCatalogLibrary::FindCoreAction(TEXT("Action.Brace")).ReactionTrigger == ERTReactionTrigger::None);

	// I numeri che il catalogo dichiara come effetti stanno nei DATI, non in una costante del resolver.
	const FRTActionDef Counter = URTCatalogLibrary::FindCoreAction(TEXT("Action.Counter"));
	int32 CounterDamage = 0;
	for (const FRTActionEffectSpec& S : Counter.Effects)
	{
		if (S.Effect == ERTActionEffect::Damage) { CounterDamage = S.Amount; }
	}
	TestEqual(TEXT("Counter: contrattacco da 16"), CounterDamage, 16);

	const FRTActionDef Shield = URTCatalogLibrary::FindCoreAction(TEXT("Action.Shield"));
	int32 ShieldAmount = 0;
	for (const FRTActionEffectSpec& S : Shield.Effects)
	{
		if (S.Effect == ERTActionEffect::Shield) { ShieldAmount = S.Amount; }
	}
	TestEqual(TEXT("Shield: 25 punti"), ShieldAmount, 25);

	const TArray<FString> Errors = URTCatalogLibrary::ValidateActions(URTCatalogLibrary::GetCoreActionCatalog());
	for (const FString& Err : Errors) { AddError(Err); }
	TestEqual(TEXT("le azioni generiche restano valide"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCounterDealsDamageTest,
	"RefactorTactics.Reactions.Counter.DealsDamageToAttacker",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCounterDealsDamageTest::RunTest(const FString&)
{
	// Counter esegue un attacco da 16 contro CHI ha colpito. Il bersaglio del contrattacco non e' una scelta
	// del resolver: e' l'attaccante che ha innescato il trigger.
	UWorld* World = MakeDefWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnDefMap(World);

	ARTUnit* Reactor = SpawnDefUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Attacker = SpawnDefUnit(World, 1, FRTCellId(1, 0));
	ARTUnit* Bystander = SpawnDefUnit(World, 1, FRTCellId(-1, 0)); // non colpisce: non deve incassare nulla
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Reactor"), Reactor) || !TestNotNull(TEXT("Attacker"), Attacker)
		|| !TestNotNull(TEXT("Bystander"), Bystander) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyDefWorld(World);
		return false;
	}

	Reactor->PlannedReactionAbility = AddDefAbility(Reactor, TEXT("Action.Counter"));
	Reactor->PlannedAbilityIndex = INDEX_NONE;

	const int32 AttackerHealth = Attacker->Health;
	const int32 BystanderHealth = Bystander->Health;
	Attacker->PlannedAbilityIndex = 0; // attacco base, colpo singolo
	Attacker->PlannedAttackTarget = Reactor;

	RunDefTurn(TM);

	TestEqual(TEXT("la reazione si e' attivata"), CountReactionOutcome(TM, ERTReactionOutcome::Activated), 1);
	TestEqual(TEXT("chi ha colpito incassa i 16 del contrattacco"), AttackerHealth - Attacker->Health, 16);
	TestEqual(TEXT("chi non ha colpito non viene contrattaccato"), Bystander->Health, BystanderHealth);

	DestroyDefWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCounterIgnoresEnvironmentalTest,
	"RefactorTactics.Reactions.Counter.IgnoresEnvironmentalDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCounterIgnoresEnvironmentalTest::RunTest(const FString&)
{
	// Nome vincolante della DoD. Il danno AMBIENTALE non esiste ancora (epic E8/CP 8.2: nessun hazard applica
	// danno oggi), quindi la clausola si verifica per come e' COSTRUITO il trigger, non simulando un hazard
	// che non c'e': `HitByDirectAttack` guarda solo i colpi di `Plan.Hits` con intento a forma `Single`.
	// Un danno che non nasce da un colpo diretto non entra in quell'array e non puo' innescare la reazione.
	//
	// Il caso piu' vicino che oggi si puo' davvero costruire e' un'AREA: danno reale, stesso bersaglio, ma non
	// un attacco diretto. Deve lasciare la reazione ferma.
	TArray<FRTHexAttackHit> Hits;
	Hits.Add(FRTHexAttackHit(/*Attacker*/ 1, /*Target*/ 0, /*Power*/ 18, /*IntentIndex*/ 0));

	TArray<FRTHexAttackIntent> AreaIntents;
	FRTHexAttackIntent Area;
	Area.AttackerId = 1;
	Area.TargetId = 0;
	Area.Shape = ERTAbilityShape::Area;
	AreaIntents.Add(Area);

	TestEqual(TEXT("un'area che ferisce non innesca il contrattacco"),
		URTReactionLibrary::FindTriggeringAttacker(ERTReactionTrigger::HitByDirectAttack, 0, Hits, AreaIntents),
		static_cast<int32>(INDEX_NONE));

	// Nessun colpo affatto (il caso in cui finira' il danno ambientale quando E8 lo introdurra'): nessun
	// trigger, senza bisogno di sapere da dove venga il danno.
	TestEqual(TEXT("danno che non passa da un colpo raccolto: nessun trigger"),
		URTReactionLibrary::FindTriggeringAttacker(ERTReactionTrigger::HitByDirectAttack, 0, {}, {}),
		static_cast<int32>(INDEX_NONE));

	// Controprova: lo stesso colpo, dichiarato diretto, innesca e identifica l'attaccante.
	TArray<FRTHexAttackIntent> DirectIntents;
	FRTHexAttackIntent Direct;
	Direct.AttackerId = 1;
	Direct.TargetId = 0;
	Direct.Shape = ERTAbilityShape::Single;
	DirectIntents.Add(Direct);
	TestEqual(TEXT("un colpo diretto innesca, e sa chi e' stato"),
		URTReactionLibrary::FindTriggeringAttacker(ERTReactionTrigger::HitByDirectAttack, 0, Hits, DirectIntents), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDeflectReducesDamageTest,
	"RefactorTactics.Reactions.Deflect.ReducesDirectDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDeflectReducesDamageTest::RunTest(const FString&)
{
	// -20 sul colpo diretto che ha innescato la reazione: 25 del Tiro -> 5.
	UWorld* World = MakeDefWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnDefMap(World);

	ARTUnit* Reactor = SpawnDefUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Attacker = SpawnDefUnit(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Reactor"), Reactor) || !TestNotNull(TEXT("Attacker"), Attacker) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyDefWorld(World);
		return false;
	}

	Reactor->PlannedReactionAbility = AddDefAbility(Reactor, TEXT("Action.Deflect"));
	Reactor->PlannedAbilityIndex = INDEX_NONE;

	const int32 Before = Reactor->Health;
	Attacker->PlannedAbilityIndex = 0; // Tiro: 25 danni
	Attacker->PlannedAttackTarget = Reactor;

	RunDefTurn(TM);

	TestEqual(TEXT("la reazione si e' attivata"), CountReactionOutcome(TM, ERTReactionOutcome::Activated), 1);
	TestEqual(TEXT("25 - 20 = 5 danni incassati"), Before - Reactor->Health,
		25 - URTCombatLibrary::DeflectDamageReduction);
	TestEqual(TEXT("non riflette: chi ha colpito non incassa nulla"), Attacker->Health, Attacker->MaxHealth);

	DestroyDefWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDeflectZeroDamageStillHitsTest,
	"RefactorTactics.Reactions.Deflect.ZeroDamageStillHits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDeflectZeroDamageStillHitsTest::RunTest(const FString&)
{
	// Nome vincolante della DoD. "Se il danno arriva a zero l'attacco e' comunque considerato avvenuto (conta
	// per trigger e marchi)": il clamp e' sul VALORE, non sulla voce — il colpo resta nell'array, quindi
	// continua a comparire nel TurnLog e a innescare le reazioni. Verificato al livello puro (dove il caso
	// "danno azzerato" si costruisce esattamente) e poi in partita.
	TArray<FRTAttack> Attacks;
	Attacks.Add(FRTAttack(/*Target*/ 0, /*Power*/ 15)); // meno dei 20 di Deflect: si azzera

	TArray<int32> Delta;
	Delta.Init(0, 1);
	Delta[0] = -URTCombatLibrary::DeflectDamageReduction;

	const TArray<FRTAttack> Reduced = URTCombatResolver::ApplyFirstHitDelta(Attacks, Delta);
	TestEqual(TEXT("il colpo NON sparisce dall'array"), Reduced.Num(), 1);
	TestEqual(TEXT("il danno e' azzerato, non negativo"), Reduced[0].Power, 0);
	TestEqual(TEXT("resta lo stesso bersaglio"), Reduced[0].TargetIndex, 0);

	// In partita: un colpo azzerato da Deflect e' comunque un colpo avvenuto, quindi lascia la sua voce di
	// combattimento nel TurnLog — che e' il modo in cui "conta per trigger e marchi" e' osservabile oggi.
	UWorld* World = MakeDefWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnDefMap(World);

	ARTUnit* Reactor = SpawnDefUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Attacker = SpawnDefUnit(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Reactor"), Reactor) || !TestNotNull(TEXT("Attacker"), Attacker) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyDefWorld(World);
		return false;
	}

	Reactor->PlannedReactionAbility = AddDefAbility(Reactor, TEXT("Action.Deflect"));
	Reactor->PlannedAbilityIndex = INDEX_NONE;
	// Guardia ADDOSSO al deflettore: -15 (Guard) -20 (Deflect) su un colpo da 25 -> ampiamente sotto zero.
	Reactor->ApplyStatus(TAG_Status_Guarded, 1);

	const int32 Before = Reactor->Health;
	Attacker->PlannedAbilityIndex = 0;
	Attacker->PlannedAttackTarget = Reactor;

	RunDefTurn(TM);

	TestEqual(TEXT("danno azzerato: nessun HP perso"), Reactor->Health, Before);
	TestTrue(TEXT("l'attacco resta registrato come avvenuto nel TurnLog"), CountCombatEntries(TM) >= 1);
	TestEqual(TEXT("e la reazione risulta comunque attivata"), CountReactionOutcome(TM, ERTReactionOutcome::Activated), 1);

	DestroyDefWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBraceTest,
	"RefactorTactics.Reactions.Brace.ReducesEveryHitAndBlocksPush",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBraceTest::RunTest(const FString&)
{
	// Le tre promesse di Brace in un turno solo: -10 su OGNI danno diretto (non solo il primo, che e' cio' che
	// lo distingue da Guard), spinta bloccata, movimento volontario bloccato.
	UWorld* World = MakeDefWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnDefMap(World, /*Radius*/ 8);

	ARTUnit* Bracer = SpawnDefUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* AttackerA = SpawnDefUnit(World, 1, FRTCellId(2, 0));
	ARTUnit* AttackerB = SpawnDefUnit(World, 1, FRTCellId(-2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Bracer"), Bracer) || !TestNotNull(TEXT("AttackerA"), AttackerA)
		|| !TestNotNull(TEXT("AttackerB"), AttackerB) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyDefWorld(World);
		return false;
	}

	// Brace e' un'azione PRINCIPALE di Prep: si pianifica come tale, non nello slot reazione.
	Bracer->PlannedAbilityIndex = AddDefAbility(Bracer, TEXT("Action.Brace"));
	Bracer->PlannedCell = FRTCellId(3, 0); // proverebbe a muoversi: Brace deve impedirglielo

	const int32 Before = Bracer->Health;
	AttackerA->PlannedAbilityIndex = 0; // Tiro 25
	AttackerA->PlannedAttackTarget = Bracer;
	AttackerB->PlannedAbilityIndex = 0; // Tiro 25
	AttackerB->PlannedAttackTarget = Bracer;

	RunDefTurn(TM);

	// DUE colpi da 25, ridotti ENTRAMBI di 10: 15 + 15 = 30. Con la meccanica "primo colpo" sarebbero 40.
	TestEqual(TEXT("-10 su OGNI colpo, non solo sul primo"), Before - Bracer->Health,
		2 * (25 - URTCombatLibrary::BraceDamageReduction));
	TestTrue(TEXT("il movimento volontario e' bloccato"), Bracer->Cell == FRTCellId(0, 0));

	DestroyDefWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBraceBlocksPushTest,
	"RefactorTactics.Reactions.Brace.BlocksFirstPush",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBraceBlocksPushTest::RunTest(const FString&)
{
	// "Impedisce la prima spinta", senza il limite di 1 cella che vincola Guard: qui la spinta e' di DUE celle
	// (Action.Push ne fa 1; si usa la Spazzata del Guardian, che ne dichiara 2) e Brace la annulla comunque.
	UWorld* World = MakeDefWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnDefMap(World, /*Radius*/ 8);

	ARTUnit* Bracer = SpawnDefUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Pusher = SpawnDefUnit(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Bracer"), Bracer) || !TestNotNull(TEXT("Pusher"), Pusher) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyDefWorld(World);
		return false;
	}

	Bracer->PlannedAbilityIndex = AddDefAbility(Bracer, TEXT("Action.Brace"));
	Bracer->PlannedCell = Bracer->Cell;

	Pusher->PlannedAbilityIndex = AddDefAbility(Pusher, TEXT("Action.Push"));
	Pusher->PlannedAttackTarget = Bracer;
	Pusher->PlannedCell = Pusher->Cell;

	RunDefTurn(TM);

	TestTrue(TEXT("irrigidito: la spinta non lo sposta"), Bracer->Cell == FRTCellId(0, 0));

	DestroyDefWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTShieldTest,
	"RefactorTactics.Reactions.Shield.AbsorbsBeforeHealth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTShieldTest::RunTest(const FString&)
{
	// 25 punti di scudo consumati PRIMA della salute. Riusa lo stesso `AddTemporaryShield` di Guardian.Barrier:
	// qui si verifica che l'azione sia cablata, non che il meccanismo di scudo funzioni (quello ha gia' i suoi test).
	UWorld* World = MakeDefWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnDefMap(World);

	ARTUnit* Shielded = SpawnDefUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Attacker = SpawnDefUnit(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Shielded"), Shielded) || !TestNotNull(TEXT("Attacker"), Attacker) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyDefWorld(World);
		return false;
	}

	// Il Ranger parte senza scudo: quello che si vede dopo viene tutto dall'azione.
	TestEqual(TEXT("il Ranger parte senza scudo"), Shielded->Shield, 0);

	Shielded->PlannedAbilityIndex = AddDefAbility(Shielded, TEXT("Action.Shield"));
	Shielded->PlannedCell = Shielded->Cell;

	const int32 BeforeHealth = Shielded->Health;
	Attacker->PlannedAbilityIndex = 0; // Tiro: 25 danni, esattamente quanto lo scudo
	Attacker->PlannedAttackTarget = Shielded;

	RunDefTurn(TM);

	TestEqual(TEXT("i 25 danni finiscono sullo scudo, non sugli HP"), Shielded->Health, BeforeHealth);

	DestroyDefWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCleanseTest,
	"RefactorTactics.Reactions.Cleanse.RemovesOneDeclaredStatus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCleanseTest::RunTest(const FString&)
{
	// Rimuove UNO stato, e lo sceglie il PIANO: si scorre `PlannedCleansePriority` e si toglie il primo
	// presente. L'unita' ne porta due, la lista ne mette uno solo davanti -> l'altro resta.
	UWorld* World = MakeDefWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnDefMap(World);

	ARTUnit* Cleanser = SpawnDefUnit(World, 0, FRTCellId(0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Cleanser"), Cleanser) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyDefWorld(World);
		return false;
	}

	Cleanser->ApplyStatus(TAG_Status_Root, 3);
	Cleanser->ApplyStatus(TAG_Status_Marked, 3);
	Cleanser->PlannedAbilityIndex = AddDefAbility(Cleanser, TEXT("Action.Cleanse"));
	Cleanser->PlannedCell = Cleanser->Cell;
	// L'ordine dichiarato mette Marked davanti: e' quello che deve sparire, non Root.
	Cleanser->PlannedCleansePriority = { TAG_Status_Marked, TAG_Status_Root };

	RunDefTurn(TM);

	TestFalse(TEXT("il primo stato della lista e' stato purificato"), Cleanser->HasStatus(TAG_Status_Marked));
	TestTrue(TEXT("UNO solo: il secondo resta"), Cleanser->HasStatus(TAG_Status_Root));

	DestroyDefWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCleanseNoImplicitChoiceTest,
	"RefactorTactics.Reactions.Cleanse.NoImplicitChoice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCleanseNoImplicitChoiceTest::RunTest(const FString&)
{
	// "La priorita' di rimozione e' scelta dal giocatore DURANTE il planning (non a runtime: nessuna scelta
	// implicita)". Senza ordine dichiarato non si rimuove nulla: il resolver non indovina al posto suo,
	// nemmeno quando ci sarebbe un solo candidato ovvio.
	UWorld* World = MakeDefWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnDefMap(World);

	ARTUnit* Cleanser = SpawnDefUnit(World, 0, FRTCellId(0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Cleanser"), Cleanser) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyDefWorld(World);
		return false;
	}

	Cleanser->ApplyStatus(TAG_Status_Root, 3);
	Cleanser->PlannedAbilityIndex = AddDefAbility(Cleanser, TEXT("Action.Cleanse"));
	Cleanser->PlannedCell = Cleanser->Cell;
	Cleanser->PlannedCleansePriority.Reset(); // nessuna scelta dichiarata

	RunDefTurn(TM);

	TestTrue(TEXT("nessun ordine dichiarato: lo stato resta, il resolver non sceglie"),
		Cleanser->HasStatus(TAG_Status_Root));

	// Controprova sullo stato NON elencato: dichiarare una lista che non contiene lo stato posseduto non
	// autorizza a togliere quello che c'e'.
	Cleanser->PlannedAbilityIndex = AddDefAbility(Cleanser, TEXT("Action.Cleanse"));
	Cleanser->PlannedCell = Cleanser->Cell;
	Cleanser->PlannedCleansePriority = { TAG_Status_Exposed }; // che l'unita' non ha

	RunDefTurn(TM);

	TestTrue(TEXT("si toglie solo cio' che il piano elenca"), Cleanser->HasStatus(TAG_Status_Root));

	DestroyDefWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTApplyDamageDeltaTest,
	"RefactorTactics.Combat.ApplyDamageDeltaHitsEveryAttack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTApplyDamageDeltaTest::RunTest(const FString&)
{
	// La differenza fra le due funzioni, isolata: stesso input, stesso delta, esiti diversi. E' l'unica
	// ragione per cui `ApplyDamageDelta` esiste invece di un parametro su `ApplyFirstHitDelta`.
	TArray<FRTAttack> Attacks;
	Attacks.Add(FRTAttack(/*Target*/ 0, /*Power*/ 25));
	Attacks.Add(FRTAttack(/*Target*/ 0, /*Power*/ 25));
	Attacks.Add(FRTAttack(/*Target*/ 1, /*Power*/ 25)); // altro bersaglio: non deve essere toccato

	TArray<int32> Delta;
	Delta.Init(0, 2);
	Delta[0] = -10;

	const TArray<FRTAttack> Every = URTCombatResolver::ApplyDamageDelta(Attacks, Delta);
	TestEqual(TEXT("ogni colpo del bersaglio e' ridotto (1/2)"), Every[0].Power, 15);
	TestEqual(TEXT("ogni colpo del bersaglio e' ridotto (2/2)"), Every[1].Power, 15);
	TestEqual(TEXT("un altro bersaglio non e' toccato"), Every[2].Power, 25);

	const TArray<FRTAttack> First = URTCombatResolver::ApplyFirstHitDelta(Attacks, Delta);
	TestEqual(TEXT("a confronto, il 'primo colpo' riduce solo quello"), First[0].Power, 15);
	TestEqual(TEXT("e lascia intero il secondo"), First[1].Power, 25);

	// Un delta che supera il danno azzera senza andare sotto zero, e non cancella la voce.
	TArray<int32> Huge;
	Huge.Init(0, 2);
	Huge[0] = -100;
	const TArray<FRTAttack> Clamped = URTCombatResolver::ApplyDamageDelta(Attacks, Huge);
	TestEqual(TEXT("il danno si azzera, non diventa negativo"), Clamped[0].Power, 0);
	TestEqual(TEXT("e il colpo resta nell'array"), Clamped.Num(), 3);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
