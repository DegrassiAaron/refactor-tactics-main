#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
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
#include "Turn/RTReactionOpportunityTypes.h" // RequiresDecisionBoundary: la cardinalita' del profilo la decide LEI
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Bot/RTHexBotLibrary.h" // DecideReactionResponse: la risposta del bot dev'essere legale anche sul `Brace`

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
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);

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
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeWraith());
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell; // fermo: questi test guardano il Blast
		// [D-224] Lo scudo base sta a 0 in questo file: qui si misura una RIDUZIONE di danno, e sommarci
		// una costante di bilanciamento renderebbe l'asserto illeggibile ("15" diventerebbe "15 piu' 5") e
		// legherebbe questi test al valore del base. Chi vuole lo scudo se lo da' esplicitamente.
		U->Shield = 0;
		return U;
	}

	/** Il colpo pieno dell'attacco base, dal catalogo: era scritto `25`, il Tiro del Ranger legacy. */
	int32 DefFullHit()
	{
		const URTHeroData* Hero = URTHeroCatalogLibrary::MakeWraith();
		return (Hero && Hero->Actions.Num() > 0 && Hero->Actions[0]) ? Hero->Actions[0]->Power : 0;
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

	int32 CountDefensiveReactionOutcome(const ARTTurnManager* TM, ERTReactionOutcome Outcome)
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
	// 🔁 **Sostituita con E14.7 ([D-047]).** Diceva «Brace non e' una reazione: nessun trigger», e il FATTO
	// resta vero — `ReactionTrigger` e' `None` — ma la conclusione no: dal Reaction Profile il `Brace` **e'**
	// l'accesso a una reazione, solo che non scatta su un evento. Non ha un trigger perche' e' il giocatore
	// ad armarla in `Prep`, non perche' non reagisca a niente.
	// ⚠️ L'asserzione non si cancella: senza, il giorno in cui qualcuno desse al `Brace` un
	// `ERTReactionTrigger` il loop delle reazioni comincerebbe a raccoglierlo per conto suo, e nessun test lo
	// direbbe. Cambia cio' che il messaggio **insegna**, non cio' che il codice controlla.
	TestTrue(TEXT("Brace non scatta su un evento: lo arma il giocatore, quindi nessun trigger"),
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

	TestEqual(TEXT("la reazione si e' attivata"), CountDefensiveReactionOutcome(TM, ERTReactionOutcome::Activated), 1);
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

	TestEqual(TEXT("la reazione si e' attivata"), CountDefensiveReactionOutcome(TM, ERTReactionOutcome::Activated), 1);
	TestEqual(TEXT("Deflect toglie la sua riduzione al colpo pieno"), Before - Reactor->Health,
		DefFullHit() - URTCombatLibrary::DeflectDamageReduction);
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
	TestEqual(TEXT("e la reazione risulta comunque attivata"), CountDefensiveReactionOutcome(TM, ERTReactionOutcome::Activated), 1);

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

	// DUE colpi pieni, ridotti ENTRAMBI: con la meccanica "solo il primo" il totale sarebbe piu' alto di
	// una riduzione. Il colpo pieno si legge dal catalogo, la proprieta' e' «Brace vale su ogni colpo».
	TestEqual(TEXT("la riduzione vale su OGNI colpo, non solo sul primo"), Before - Bracer->Health,
		2 * (DefFullHit() - URTCombatLibrary::BraceDamageReduction));
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


// ─── E14.7 · Reaction Profile ([D-047]) ─────────────────────────────────────────────────────────────────

/**
 * Col **profilo base** il `Brace` ha **una** risposta legale, quindi nessun boundary si apre ([D-047] §2.3).
 *
 * E' la voce di DoD che protegge tutto cio' che e' verde oggi: se il profilo base aprisse una finestra, ogni
 * `Brace` della v0.1 diventerebbe un prompt e la resolution rallenterebbe per ogni unita' che si copre.
 *
 * ⚠️ La verifica passa da `RequiresDecisionBoundary` — la stessa funzione che il resolver interroga — e non
 * da un conteggio scritto qui: un test che contasse `Num() == 1` per conto proprio resterebbe verde anche se
 * la regola del boundary cambiasse soglia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBraceBaseProfileSingleResponseTest,
	"RefactorTactics.Reactions.Brace.BaseProfileHasSingleResponse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBraceBaseProfileSingleResponseTest::RunTest(const FString&)
{
	const TArray<FString> Base = URTCatalogLibrary::BraceAllowedResponses(NAME_None);

	TestEqual(TEXT("il profilo base offre una sola risposta"), Base.Num(), 1);
	TestEqual(TEXT("ed e' `Hold Ground`, la risposta universale"), Base[0], FString(TEXT("Hold Ground")));

	FRTReactionOpportunity Opp;
	Opp.AllowedResponses = Base;
	TestFalse(TEXT("quindi nessun decision boundary si apre"),
		URTReactionOpportunityLibrary::RequiresDecisionBoundary(Opp));

	// Un profilo SCONOSCIUTO da' lo stesso esito, ed e' fail-closed nel verso giusto: un piano salvato prima
	// che il catalogo cambiasse non deve poter aprire una finestra su scelte che il gioco non ha piu'.
	const TArray<FString> Unknown = URTCatalogLibrary::BraceAllowedResponses(TEXT("Profile.NonEsiste"));
	TestEqual(TEXT("un profilo sconosciuto ricade sul base"), Unknown.Num(), 1);

	return true;
}

/**
 * Un profilo d'eroe con una **seconda** risposta porta la cardinalita' a >= 2 e apre la finestra con la
 * regola che ADR-0004 §2 **ha gia'** ([D-047] §2.3): nessuna regola nuova, nessun enum di tipo.
 *
 * I tre profili del roster si verificano **uno per uno con la propria cardinalita' attesa**, e non «almeno
 * due»: `Profile.Glance` ne porta **due** di extra, quindi 3, ed e' l'unico. Un test che chiedesse solo
 * `>= 2` non distinguerebbe Glance da Sidestep, e la differenza e' l'unica cosa che quel profilo aggiunge.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBraceRicherProfileOpensWindowTest,
	"RefactorTactics.Reactions.Brace.RicherProfileOpensWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBraceRicherProfileOpensWindowTest::RunTest(const FString&)
{
	struct FCase { const TCHAR* ProfileId; int32 Expected; };
	const FCase Cases[] = {
		{ TEXT("Profile.Grounding"), 2 },
		{ TEXT("Profile.Sidestep"),  2 },
		{ TEXT("Profile.Glance"),    3 },
	};

	for (const FCase& C : Cases)
	{
		const TArray<FString> Responses = URTCatalogLibrary::BraceAllowedResponses(FName(C.ProfileId));
		TestEqual(FString::Printf(TEXT("%s: cardinalita'"), C.ProfileId), Responses.Num(), C.Expected);

		// `Hold Ground` resta la prima e non viene sostituita: un profilo AGGIUNGE, non rimpiazza.
		TestEqual(FString::Printf(TEXT("%s: `Hold Ground` resta la risposta universale"), C.ProfileId),
			Responses[0], FString(TEXT("Hold Ground")));

		FRTReactionOpportunity Opp;
		Opp.AllowedResponses = Responses;
		TestTrue(FString::Printf(TEXT("%s: apre il decision boundary"), C.ProfileId),
			URTReactionOpportunityLibrary::RequiresDecisionBoundary(Opp));
	}

	// Il catalogo ne dichiara TRE, e il conteggio e' parte della verifica: un quarto profilo comparso senza
	// una decisione sarebbe un eroe che apre una finestra che nessuno ha discusso.
	TestEqual(TEXT("il catalogo dichiara tre profili"),
		URTCatalogLibrary::GetReactionProfileCatalog().Num(), 3);

	return true;
}

/**
 * **Riktor non ha un profilo, e nessuna finestra si apre per lui** ([D-047] §2.5).
 *
 * E' la baseline con cui CP 14.6 confronta gli altri tre quando misura il pacing, e va pinnata: se un giorno
 * qualcuno gliene assegnasse uno, il caso «un eroe senza finestra» sparirebbe dal roster e la misura del
 * pacing perderebbe il proprio controllo — senza che nulla diventi rosso.
 *
 * ⚠️ Il test verifica **entrambe** le metà: che Riktor non ne abbia uno, e che gli altri tre sì. Solo la
 * prima passerebbe anche su un roster in cui nessuno ha un profilo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBraceRiktorHasNoProfileTest,
	"RefactorTactics.Reactions.Brace.RiktorHasNoProfile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBraceRiktorHasNoProfileTest::RunTest(const FString&)
{
	int32 WithProfile = 0;
	int32 WithoutProfile = 0;

	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		if (!Hero) { continue; }

		const TArray<FString> Responses = URTCatalogLibrary::BraceAllowedResponses(Hero->ReactionProfileId);
		FRTReactionOpportunity Opp;
		Opp.AllowedResponses = Responses;
		const bool bOpens = URTReactionOpportunityLibrary::RequiresDecisionBoundary(Opp);

		if (Hero->HeroId == FName(TEXT("Hero.Riktor")))
		{
			TestTrue(TEXT("Riktor non dichiara un profilo"), Hero->ReactionProfileId.IsNone());
			TestFalse(TEXT("e il suo `Brace` non apre nessuna finestra"), bOpens);
		}

		if (bOpens) { ++WithProfile; } else { ++WithoutProfile; }
	}

	// L'altra meta': tre eroi APRONO la finestra. Senza, «Riktor non ne ha uno» sarebbe vero anche su un
	// roster in cui il profilo non esiste per nessuno — cioe' su una feature che non e' stata costruita.
	TestEqual(TEXT("tre eroi del roster aprono la finestra sul `Brace`"), WithProfile, 3);
	TestEqual(TEXT("e uno solo non la apre"), WithoutProfile, 1);

	return true;
}

/**
 * **Cio' che il profilo DICHIARA e cio' che il gioco sa ESEGUIRE oggi non coincidono, e la distanza si
 * misura** ([D-132] contro `spec-reaction-clash-e14.md` §2.5).
 *
 * Non e' un difetto da correggere qui: `Profile.Grounding` e `Profile.Glance` sono contenuto **deciso**,
 * mentre «Charge del `Grounding`» e «ampiezza della deviazione» sono dichiarati **aperti** dalla stessa spec.
 * Finche' lo restano, quelle due risposte non hanno effetti e il resolver non le offre — offrire una scelta
 * che non sa applicare significherebbe fermare la resolution per non fare niente.
 *
 * ⚠️ **Questo test e' scritto per DIVENTARE ROSSO quando una delle due voci si chiude**, ed e' il suo scopo:
 * chi aggiunge gli effetti a `GROUND` aggiorna qui il conteggio e trova, nella riga accanto, che deve anche
 * togliere la voce da `OPEN_DECISIONS.md`. Un test che dicesse solo `>= 1` lascerebbe la chiusura passare in
 * silenzio, e la distanza fra i due elenchi tornerebbe implicita.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBraceDeclaredIsNotYetExecutableTest,
	"RefactorTactics.Reactions.Brace.DeclaredIsNotYetExecutable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBraceDeclaredIsNotYetExecutableTest::RunTest(const FString&)
{
	struct FCase { const TCHAR* ProfileId; int32 Declared; int32 Executable; const TCHAR* Why; };
	const FCase Cases[] = {
		{ TEXT("Profile.Grounding"), 2, 1, TEXT("Charge del Grounding: aperta, spec §2.5") },
		{ TEXT("Profile.Sidestep"),  2, 2, TEXT("decisa da BAS-4: SelfReposition 1") },
		{ TEXT("Profile.Glance"),    3, 1, TEXT("ampiezza della deviazione: aperta, spec §2.5") },
	};

	for (const FCase& C : Cases)
	{
		const FName Id(C.ProfileId);
		TestEqual(FString::Printf(TEXT("%s: dichiarate (%s)"), C.ProfileId, C.Why),
			URTCatalogLibrary::BraceAllowedResponses(Id).Num(), C.Declared);
		TestEqual(FString::Printf(TEXT("%s: eseguibili"), C.ProfileId),
			URTCatalogLibrary::BraceExecutableResponses(Id).Num(), C.Executable);

		// `Hold Ground` apre entrambi gli elenchi e non ha effetti propri: il suo esito e' il ramo
		// `Status.Braced` del resolver, che gira da CP 5.2 e che [D-047] dichiara invariato.
		TestEqual(FString::Printf(TEXT("%s: `Hold Ground` non passa dal motore effetti"), C.ProfileId),
			URTCatalogLibrary::BraceResponseEffects(Id, TEXT("Hold Ground")).Num(), 0);
	}

	// La meta' che conta per il resolver: **solo** Sidestep apre davvero una finestra oggi. Un test che
	// contasse le sole cardinalita' dichiarate resterebbe verde anche se il cablaggio non esistesse.
	int32 OpensInPlay = 0;
	for (const FCase& C : Cases)
	{
		FRTReactionOpportunity Opp;
		Opp.AllowedResponses = URTCatalogLibrary::BraceExecutableResponses(FName(C.ProfileId));
		if (URTReactionOpportunityLibrary::RequiresDecisionBoundary(Opp)) { ++OpensInPlay; }
	}
	TestEqual(TEXT("un solo profilo su tre apre la finestra in partita"), OpensInPlay, 1);

	// E l'effetto di `SIDESTEP` e' la primitiva esistente, non una nuova: `SelfReposition 1`, la stessa che
	// `Reaction.EmergencyDash` e `Reaction.HazardEscape` portano da D-093. Se qualcuno la sostituisse con un
	// effetto proprio, il ramo del resolver che la legge continuerebbe a funzionare e la ragione per cui
	// «nessun numero nuovo entra» sarebbe falsa senza che nulla diventi rosso.
	const TArray<FRTActionEffectSpec> Sidestep =
		URTCatalogLibrary::BraceResponseEffects(TEXT("Profile.Sidestep"), TEXT("SIDESTEP"));
	if (TestEqual(TEXT("SIDESTEP dichiara un solo effetto"), Sidestep.Num(), 1))
	{
		TestTrue(TEXT("ed e' `SelfReposition`"), Sidestep[0].Effect == ERTActionEffect::SelfReposition);
		TestEqual(TEXT("di una cella, come EmergencyDash e HazardEscape"), Sidestep[0].Amount, 1);
	}

	return true;
}


namespace
{
	/** Come `SpawnDefUnit`, ma l'eroe lo sceglie il chiamante: il profilo di reazione viene da li'. */
	ARTUnit* SpawnDefHeroUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell, const URTHeroData* Hero)
	{
		if (!World || !Hero) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell;
		// [D-224] Lo scudo base sta a 0 in questo file: qui si misura una RIDUZIONE di danno, e sommarci
		// una costante di bilanciamento renderebbe l'asserto illeggibile ("15" diventerebbe "15 piu' 5") e
		// legherebbe questi test al valore del base. Chi vuole lo scudo se lo da' esplicitamente.
		U->Shield = 0;
		return U;
	}

	/**
	 * Un Blast in cui `Attaccante` spinge `Difensore`, che e' in `Brace`. **Una sola fixture per entrambe le
	 * famiglie di test del `Brace`**, e i tre puntatori dicono cosa serve a chi chiama.
	 *
	 * 🔴 **Erano DUE funzioni quasi identiche**, e una code review ha misurato il costo: ~35 righe uguali —
	 * mondo, mappa, due spawn, l'abilita', lo stato `Braced`, il giro del turno, la distruzione — che
	 * differivano solo per come si ottiene la risposta. Qualunque modifica alla fixture (l'id della spinta, la
	 * cella di partenza, la durata di `Status.Braced`) andava fatta due volte, e farla una sola volta avrebbe
	 * lasciato le due famiglie a esercitare **setup diversi restando entrambe verdi**.
	 *
	 * `Trace` non nullo = **replay**: le decisioni si armano da li' e **nessun decisore viene collegato**, che
	 * e' la condizione del Verifier — se il replay tornasse a chiedere a qualcuno non starebbe verificando la
	 * traccia, la starebbe riscrivendo.
	 *
	 * Restituisce la cella in cui il difensore finisce, e `bOutRan` dice se il turno e' **davvero girato**.
	 *
	 * 🔴 **`bOutRan` esiste perche' senza di lui un fallimento di fixture passava per un successo.** La
	 * versione precedente restituiva `FRTCellId()` — cioe' `(0,0,0)` — quando mondo, unita' o turn manager non
	 * si costruivano, e l'asserzione che conta e' `ConScarto != Start`: il sentinella la **soddisfa**, quindi
	 * «il difensore lascia la cella» diventava verde senza che nessuno si fosse mosso. I due casi
	 * `Hold Ground` sarebbero caduti al posto suo, facendo leggere una fixture rotta come un difetto del
	 * `Brace`: il rosso sarebbe arrivato, ma indicando la cosa sbagliata.
	 */
	FRTCellId RunBraceTurn(const URTHeroData* DefenderHero, const TCHAR* Response,
		const TArray<FRTTurnLogEntry>* Trace, int32* OutPrompts, TArray<FRTTurnLogEntry>* OutTrace,
		bool& bOutRan, TArray<FString>* OutDivergences = nullptr)
	{
		if (OutPrompts) { *OutPrompts = 0; }
		if (OutTrace) { OutTrace->Reset(); }
		if (OutDivergences) { OutDivergences->Reset(); }
		bOutRan = false;

		UWorld* World = MakeDefWorld();
		if (!World) { return FRTCellId(); }
		SpawnDefMap(World);

		ARTUnit* Attaccante = SpawnDefUnit(World, 0, FRTCellId(0, 0, 0));
		ARTUnit* Difensore  = SpawnDefHeroUnit(World, 1, FRTCellId(1, 0, 0), DefenderHero);
		ARTTurnManager* TM  = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

		FRTCellId Result;
		if (Attaccante && Difensore && TM)
		{
			const int32 Push = AddDefAbility(Attaccante, TEXT("Action.Push"));
			Attaccante->PlannedAbilityIndex = Push;
			Attaccante->PlannedAttackTarget = Difensore;
			Difensore->ApplyStatus(TAG_Status_Braced, 1);
			Difensore->PlannedAbilityIndex = INDEX_NONE;

			if (Trace && Trace->Num() > 0)
			{
				TM->ArmRecordedReactionDecisions(*Trace);
			}
			else if (Response)
			{
				TM->ReactionDecider.BindLambda(
					[Response, OutPrompts](const FRTReactionOpportunity&, int32) -> FString
					{
						// Conta le volte in cui la finestra ha chiesto davvero: e' la differenza fra «si apre»
						// e «si committa da sola», che il solo esito non distingue.
						if (OutPrompts) { ++(*OutPrompts); }
						return FString(Response);
					});
			}

			RunDefTurn(TM);
			Result = Difensore->Cell;
			if (OutTrace) { *OutTrace = TM->GetTurnLog(); }
			// Il verdetto del Verifier esce con la cella: e' l'unico modo per distinguere «la traccia copriva
			// la finestra» da «non la copriva e il ripiego e' finito per caso nello stesso posto». Senza,
			// un test sul solo esito posizionale sarebbe verde anche col canale di verifica rotto.
			if (OutDivergences) { *OutDivergences = TM->GetVerificationDivergences(); }
			bOutRan = true; // il turno e' girato davvero: da qui in poi `Result` e' una misura, non un default
		}

		DestroyDefWorld(World);
		return Result;
	}

	/** Il caso «gioca e conta i prompt», che e' quello della maggior parte dei test del `Brace`. */
	FRTCellId RunBracePushTurn(const URTHeroData* DefenderHero, const TCHAR* Response, int32& OutPrompts,
		bool& bOutRan)
	{
		return RunBraceTurn(DefenderHero, Response, /*Trace*/ nullptr, &OutPrompts, /*OutTrace*/ nullptr,
			bOutRan);
	}
}

/**
 * 🔵 **Il Reaction Profile ha un consumatore in PARTITA** ([D-047], fetta 3 di E14.7).
 *
 * Fino al 2026-08-19 `BraceAllowedResponses` aveva **zero** chiamanti fuori dai test e
 * `ARTUnit::ReactionProfileId` era trasportato e mai letto: un dato con produttore e senza consumatore, che
 * e' il difetto che `#583` chiama per nome — «supera ogni test unitario, e il gioco non avra' mai una
 * condizione da valutare». Questo test e' la prova che il profilo attraversa il resolver.
 *
 * ⚠️ **Le tre meta' non sono ridondanti, e nessuna da sola dimostra la proprieta'**:
 * · con `SIDESTEP` il difensore si muove — se mancasse, il profilo sarebbe letto e ignorato;
 * · con `Hold Ground` resta — se mancasse, «si e' mosso» proverebbe solo che la spinta lo ha spostato, cioe'
 *   che il `Brace` ha smesso di funzionare;
 * · **senza decisore** resta, e la finestra non e' stata chiesta a nessuno — e' il fail-closed di ADR-0004
 *   §3 applicato al `Brace`: la scelta sicura e' `Hold Ground`, non il `HOLD` dell'Overwatch, che qui non
 *   sarebbe nemmeno una risposta legale.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBraceProfileDecidesInPlayTest,
	"RefactorTactics.Reactions.Brace.ProfileDecidesInPlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBraceProfileDecidesInPlayTest::RunTest(const FString&)
{
	const URTHeroData* Phase = URTHeroCatalogLibrary::MakePhase();
	if (!TestNotNull(TEXT("Phase, che porta `Profile.Sidestep`"), Phase)) { return false; }
	if (!TestTrue(TEXT("e il profilo e' quello atteso"),
		Phase->ReactionProfileId == FName(TEXT("Profile.Sidestep")))) { return false; }

	const FRTCellId Start(1, 0, 0);

	int32 PromptsSidestep = 0; bool bRanSidestep = false;
	const FRTCellId ConScarto = RunBracePushTurn(Phase, TEXT("SIDESTEP"), PromptsSidestep, bRanSidestep);
	// ⚠️ `bRan` PRIMA della cella, e non e' pedanteria: con la fixture rotta il valore di ritorno e' `(0,0,0)`,
	// che soddisfa `!= Start` — l'asserzione sotto sarebbe verde senza che nessuno si sia mosso.
	TestTrue(TEXT("il turno con `SIDESTEP` e' girato davvero"), bRanSidestep);
	TestTrue(TEXT("con `SIDESTEP` il difensore lascia la cella"), ConScarto != Start);
	TestEqual(TEXT("e la finestra e' stata chiesta una volta"), PromptsSidestep, 1);

	int32 PromptsHold = 0; bool bRanHold = false;
	const FRTCellId ConHold = RunBracePushTurn(Phase, TEXT("Hold Ground"), PromptsHold, bRanHold);
	TestTrue(TEXT("il turno con `Hold Ground` e' girato davvero"), bRanHold);
	TestTrue(TEXT("con `Hold Ground` resta dov'era"), ConHold == Start);
	TestEqual(TEXT("e la finestra e' stata chiesta anche qui"), PromptsHold, 1);

	int32 PromptsNessuno = 0; bool bRanNessuno = false;
	const FRTCellId SenzaDecisore = RunBracePushTurn(Phase, nullptr, PromptsNessuno, bRanNessuno);
	TestTrue(TEXT("il turno senza decisore e' girato davvero"), bRanNessuno);
	TestTrue(TEXT("senza decisore la scelta sicura tiene la posizione"), SenzaDecisore == Start);
	TestEqual(TEXT("e nessuno e' stato interrogato"), PromptsNessuno, 0);

	// 🔴 **La meta' che distingue «il profilo ha deciso» da «il `Brace` non si e' applicato», e senza la quale
	// tutto il resto e' ambiguo.** Con una spinta di **1** la cella dello scarto coincide con quella in cui
	// l'unita' sarebbe finita **subendo** la spinta: `SelfReposition` allontana lungo la stessa linea. Quindi
	// «R1 si e' mosso» da solo NON prova che abbia scelto — proverebbe la stessa cosa se il ramo `Braced` non
	// esistesse affatto. Il discriminante e' il **conteggio dei prompt**, che senza finestra resta a zero.
	//
	// ⚠️ E' il difetto che la code review ha trovato nello scenario gemello, dove la sola misura e' la cella:
	// li' rimuovere il ramo `Braced` lascia tutte le assertion verdi. Qui no, ed e' questa riga a impedirlo —
	// motivo per cui il test unitario e lo scenario non sono ridondanti.
	TestEqual(TEXT("e la cella dello scarto coincide con quella della spinta subita (spinta 1): "
		"e' il conteggio dei prompt a dire che c'e' stata una scelta"),
		PromptsSidestep, 1);

	// **La baseline**: Riktor non ha un profilo, quindi nessuna finestra si apre per lui e il decisore non
	// viene mai chiamato — nemmeno se ne colleghi uno che risponderebbe `SIDESTEP`. E' la meta' che protegge
	// tutto cio' che e' verde oggi: se il profilo base aprisse un prompt, ogni `Brace` della v0.1 ne
	// aprirebbe uno.
	const URTHeroData* Riktor = URTHeroCatalogLibrary::MakeRiktor();
	if (TestNotNull(TEXT("Riktor"), Riktor))
	{
		int32 PromptsRiktor = 0; bool bRanRiktor = false;
		const FRTCellId Piantato = RunBracePushTurn(Riktor, TEXT("SIDESTEP"), PromptsRiktor, bRanRiktor);
		TestTrue(TEXT("il turno di Riktor e' girato davvero"), bRanRiktor);
		TestTrue(TEXT("Riktor tiene la cella col comportamento base"), Piantato == Start);
		TestEqual(TEXT("e nessuna finestra si e' aperta per lui"), PromptsRiktor, 0);
	}

	// 🔴 **Wraith e' la meta' che pinna QUALE dei due elenchi il resolver interroga.** `Profile.Glance`
	// dichiara **tre** risposte ([D-132]) e nessuna delle due extra ha effetti, quindi le eseguibili sono
	// una sola e nessuna finestra si apre. Con `BraceAllowedResponses` al posto di `BraceExecutableResponses`
	// il resolver aprirebbe qui un prompt su `GLANCE LEFT`, che poi non saprebbe applicare — e senza questa
	// meta' la sostituzione resterebbe verde, perche' l'unita' finirebbe comunque ferma.
	const URTHeroData* Wraith = URTHeroCatalogLibrary::MakeWraith();
	if (TestNotNull(TEXT("Wraith"), Wraith))
	{
		int32 PromptsWraith = 0; bool bRanWraith = false;
		const FRTCellId Fermo = RunBracePushTurn(Wraith, TEXT("GLANCE LEFT"), PromptsWraith, bRanWraith);
		TestTrue(TEXT("il turno di Wraith e' girato davvero"), bRanWraith);
		TestEqual(TEXT("nessuna finestra per un profilo senza effetti dichiarati"), PromptsWraith, 0);
		TestTrue(TEXT("e il `Brace` di Wraith regge come sempre"), Fermo == Start);
	}

	return true;
}

/**
 * 🔴 **`SIDESTEP` esce dalla LINEA di spinta, e non la percorre** ([D-047] §2.5-bis).
 *
 * E' la correzione di un difetto di **design**, non di codice: fino al 2026-08-19 lo scarto usava
 * `HexKnockbackDestination`, che allontana dall'attaccante — cioe' mandava l'unita' **dove la spinta voleva
 * mandarla**. Siccome il ramo `Status.Braced` blocca gia' la spinta a *qualunque* distanza, `Hold Ground`
 * teneva la cella e `SIDESTEP` la cedeva, con lo **stesso danno**: una risposta strettamente dominata, cioe'
 * un boundary che costa un prompt e non compra niente.
 *
 * ⚠️ **Questo test asserisce la proprieta' che rende la scelta una scelta**, e non una cella particolare: la
 * destinazione **non sta sulla linea** — ne' dove la spinta spinge, ne' da dove arriva. Pinnare una cella
 * fissa avrebbe legato il test all'ordine canonico invece che alla regola, e sarebbe diventato rosso a ogni
 * riordino delle direzioni senza che nulla di sbagliato fosse accaduto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSidestepLeavesTheLineTest,
	"RefactorTactics.Reactions.Brace.SidestepLeavesThePushLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSidestepLeavesTheLineTest::RunTest(const FString&)
{
	UWorld* World = MakeDefWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnDefMap(World, 6);
	const URTHexMapAsset* Map = MapActor ? MapActor->MapAsset : nullptr;
	if (!TestNotNull(TEXT("mappa"), (void*)Map)) { DestroyDefWorld(World); return false; }

	// Spinta da ovest verso est: chi spinge e' a (-1,0), il bersaglio a (0,0).
	const FRTCellId PushFrom(-1, 0, 0);
	const FRTCellId Target(0, 0, 0);

	ERTHexDirection PushDir;
	if (!TestTrue(TEXT("la linea di spinta ha una direzione"),
		URTHexLibrary::DirectionTowards(PushFrom, Target, PushDir)))
	{
		DestroyDefWorld(World); return false;
	}
	const FRTCellId Ahead  = URTHexLibrary::Neighbor(Target, PushDir);
	const FRTCellId Behind = URTHexLibrary::Neighbor(Target, URTHexLibrary::OppositeDirection(PushDir));

	// (a) Campo libero: lo scarto esiste e NON sta sulla linea. E' la proprieta' che il difetto violava.
	{
		const FRTCellId Escape = URTReactionLibrary::FindSidestepCell(
			Map, Target, PushFrom, ERTHexDirection::NE, /*Occupied*/ {});

		TestTrue(TEXT("lo scarto porta via dalla cella"), Escape != Target);
		TestTrue(TEXT("e NON dove la spinta spingeva"), Escape != Ahead);
		TestTrue(TEXT("ne' verso chi spinge"), Escape != Behind);
		TestTrue(TEXT("resta comunque adiacente"),
			URTHexLibrary::Neighbors(Target).Contains(Escape));
	}

	// (b) **Il facing decide**, come per `Reaction.HazardEscape`: lo stesso identico caso con due
	// orientamenti diversi da' due scarti diversi. Senza questa meta', «esce dalla linea» sarebbe compatibile
	// con un ordine canonico che ignora il giocatore.
	{
		const FRTCellId VersoNE = URTReactionLibrary::FindSidestepCell(
			Map, Target, PushFrom, ERTHexDirection::NE, {});
		const FRTCellId VersoSE = URTReactionLibrary::FindSidestepCell(
			Map, Target, PushFrom, ERTHexDirection::SE, {});

		TestTrue(TEXT("guardando a NE si scarta a NE"),
			VersoNE == URTHexLibrary::Neighbor(Target, ERTHexDirection::NE));
		TestTrue(TEXT("guardando a SE si scarta a SE"),
			VersoSE == URTHexLibrary::Neighbor(Target, ERTHexDirection::SE));
		TestTrue(TEXT("e i due esiti differiscono davvero"), VersoNE != VersoSE);
	}

	// (c) **Il facing punta SULLA linea**: non si puo' obbedire, e si ripiega sull'ordine canonico — che e'
	// comunque fuori dalla linea. E' il caso che distingue «preferisci il facing» da «segui il facing».
	{
		const FRTCellId Escape = URTReactionLibrary::FindSidestepCell(
			Map, Target, PushFrom, PushDir, {});

		TestTrue(TEXT("il facing sulla linea non viene obbedito"), Escape != Ahead);
		TestTrue(TEXT("e nemmeno il suo opposto"), Escape != Behind);
		TestTrue(TEXT("ma uno scarto si trova lo stesso"), Escape != Target);
	}

	// (d) **Tutte le uscite occupate**: si ripiega su `Hold Ground`, e chi chiama lo legge da `== From`.
	// ⚠️ Si occupano i QUATTRO fuori linea; `Ahead` e `Behind` restano liberi di proposito — se la funzione
	// li offrisse come scarto, questo caso passerebbe comunque e il difetto originale tornerebbe da qui.
	{
		TArray<FRTCellId> Occupied;
		for (const FRTCellId& N : URTHexLibrary::Neighbors(Target))
		{
			if (N != Ahead && N != Behind) { Occupied.Add(N); }
		}
		TestEqual(TEXT("le uscite fuori linea sono quattro"), Occupied.Num(), 4);

		const FRTCellId Escape = URTReactionLibrary::FindSidestepCell(
			Map, Target, PushFrom, ERTHexDirection::NE, Occupied);
		TestTrue(TEXT("senza uscite si tiene la posizione, non si ripiega sulla linea"), Escape == Target);
	}

	// (e) Fail-closed senza mappa autorevole, come `FindEscapeCell`.
	TestTrue(TEXT("senza mappa non si scarta"),
		URTReactionLibrary::FindSidestepCell(nullptr, Target, PushFrom, ERTHexDirection::NE, {}) == Target);

	DestroyDefWorld(World);
	return true;
}

/**
 * 🔵 **Il GIRO COMPLETO: la decisione del `Brace` entra nel TurnLog e il replay la riapplica.** È la
 * proprietà per cui la v10 del formato esiste, e nessuna delle sue due metà da sola la dimostra.
 *
 * Fino al 2026-08-19 la decisione non lasciava traccia: in ri-simulazione `ArmRecordedReactionDecisions` non
 * trovava la chiave, segnalava «finestra non coperta dalla traccia» e applicava la scelta sicura — l'unità
 * che aveva scartato **restava ferma nel replay**, e il Verifier accusava la traccia di un difetto dello
 * *scrittore*.
 *
 * ⚠️ **Il replay gira SENZA decisore collegato**, ed è la condizione che rende il test una verifica e non una
 * ripetizione: se tornasse a chiedere a qualcuno non starebbe verificando la traccia, la starebbe
 * riscrivendo. Tutto ciò che sa della scelta deve venire dai byte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBraceDecisionRoundTripsThroughTraceTest,
	"RefactorTactics.Reactions.Brace.DecisionRoundTripsThroughTrace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBraceDecisionRoundTripsThroughTraceTest::RunTest(const FString&)
{
	const URTHeroData* Phase = URTHeroCatalogLibrary::MakePhase();
	if (!TestNotNull(TEXT("Phase, che porta `Profile.Sidestep`"), Phase)) { return false; }
	const FRTCellId Start(1, 0, 0);

	// --- 1. La partita: si sceglie `SIDESTEP`, e la traccia se ne accorge --------------------------------
	TArray<FRTTurnLogEntry> Traccia;
	bool bRan = false;
	const FRTCellId Originale = RunBraceTurn(Phase, TEXT("SIDESTEP"), /*Trace*/ nullptr, /*OutPrompts*/ nullptr, &Traccia, bRan);

	if (!TestTrue(TEXT("il turno originale e' girato"), bRan)) { return false; }
	TestTrue(TEXT("con `SIDESTEP` il difensore lascia la cella"), Originale != Start);

	// La voce esiste, ed e' quella giusta: categoria, esito e **token**. Senza il token la riga successiva
	// sarebbe verde con una traccia che non dice quale risposta sia stata scelta.
	const FRTTurnLogEntry* Voce = Traccia.FindByPredicate([](const FRTTurnLogEntry& E)
	{
		return E.Category == ERTLogCategory::ReactionDecision
			&& E.ActionId == FName(TEXT("Action.Brace"));
	});
	if (!TestNotNull(TEXT("la decisione del `Brace` e' nel TurnLog"), (void*)Voce)) { return false; }
	TestEqual(TEXT("l'esito distingue lo scarto dal tenere la cella"), Voce->Outcome,
		static_cast<uint8>(ERTReactionDecisionOutcome::ResponseChosen));
	TestEqual(TEXT("e il token nomina la risposta"), Voce->ReactionResponse, FString(TEXT("SIDESTEP")));

	// --- 2. Il replay: stessa traccia, NESSUN decisore, stesso esito -------------------------------------
	TArray<FRTTurnLogEntry> TracciaReplay;
	bool bRanReplay = false;
	TArray<FString> Divergenze;
	const FRTCellId Replay = RunBraceTurn(Phase, /*Response*/ nullptr, &Traccia, nullptr, &TracciaReplay,
		bRanReplay, &Divergenze);

	if (!TestTrue(TEXT("il replay e' girato"), bRanReplay)) { return false; }
	TestTrue(TEXT("IL PUNTO: il replay riapplica lo scarto, non la scelta sicura"), Replay == Originale);
	TestTrue(TEXT("e quindi non e' rimasto fermo"), Replay != Start);
	// ⚠️ **La cella da sola non basta**, ed e' il rilievo di una code review: se un domani
	// `DeriveOpportunityId` divergesse fra scrittura e replay — un campo nuovo nella chiave, un altro spazio
	// di id — `AskReactionDecision` prenderebbe il ramo «finestra non coperta» e ripiegherebbe sulla scelta
	// sicura. Le due asserzioni sopra resterebbero vere ogni volta che il ripiego finisce nella stessa cella,
	// mentre il canale di verifica e' rotto. Il verdetto va letto.
	TestEqual(TEXT("e il Verifier non segnala NIENTE: la traccia copriva davvero la finestra"),
		Divergenze.Num(), 0);

	// --- 3. La META' NEGATIVA, senza cui il test sopra sarebbe verde anche col token ignorato ------------
	// La stessa traccia **privata del token**: è ciò che una v9 porterebbe. La ricostruzione dà `HOLD`, che
	// per una finestra di `Brace` non è legale — quindi il replay NON deve riapplicare lo scarto.
	TArray<FRTTurnLogEntry> SenzaToken = Traccia;
	for (FRTTurnLogEntry& E : SenzaToken)
	{
		E.ReactionResponse.Empty();
	}
	TArray<FRTTurnLogEntry> Ignorata;
	bool bRanSenza = false;
	TArray<FString> DivergenzeSenza;
	const FRTCellId SenzaTokenCell = RunBraceTurn(Phase, nullptr, &SenzaToken, nullptr, &Ignorata, bRanSenza,
		&DivergenzeSenza);

	TestTrue(TEXT("il turno senza token e' girato"), bRanSenza);
	TestTrue(TEXT("senza token lo scarto NON si riapplica: il token e' cio' che porta l'informazione"),
		SenzaTokenCell == Start);
	// E il Verifier lo DICHIARA invece di lasciarlo dedurre dalla cella: la ricostruzione produce `HOLD`, che
	// per questa finestra non e' legale. E' l'altro verso dell'asserzione sulle divergenze qui sopra — insieme
	// dimostrano che quel conteggio misura qualcosa, invece di essere zero per costruzione.
	TestTrue(TEXT("e il Verifier segnala la risposta registrata come illegale"), DivergenzeSenza.Num() > 0);

	// --- 4. **Una lacuna della traccia NON si ripulisce da sola** ----------------------------------------
	// Traccia non vuota — quindi si e' in ri-simulazione — ma che NON copre questa finestra: il resolver
	// prende il ramo «finestra non coperta», segnala la divergenza e ripiega sulla scelta sicura.
	//
	// 🔴 **Il punto e' cosa NON deve finire nel TurnLog del replay.** Scrivendo li' una voce completa, quella
	// finestra risulterebbe coperta: ridando il log del replay al Verifier, `ArmRecordedReactionDecisions`
	// troverebbe la chiave e la corsa uscirebbe **pulita** — una traccia nota come incompleta si sarebbe
	// lavata da sola in un giro. Una lacuna che sparisce e' peggio di una lacuna. Trovato da una code review.
	FRTTurnLogEntry AltraFinestra;
	AltraFinestra.Category = ERTLogCategory::ReactionDecision;
	AltraFinestra.Outcome = static_cast<uint8>(ERTReactionDecisionOutcome::HoldChosen);
	AltraFinestra.OpportunityId = TEXT("T9|P9|M9|U9|action.inesistente|S9");
	const TArray<FRTTurnLogEntry> TracciaEstranea = { AltraFinestra };

	TArray<FRTTurnLogEntry> LogDelReplay;
	TArray<FString> DivergenzeLacuna;
	bool bRanLacuna = false;
	RunBraceTurn(Phase, nullptr, &TracciaEstranea, nullptr, &LogDelReplay, bRanLacuna, &DivergenzeLacuna);

	TestTrue(TEXT("il turno con traccia estranea e' girato"), bRanLacuna);
	TestTrue(TEXT("il Verifier dichiara la finestra non coperta"), DivergenzeLacuna.Num() > 0);

	const bool bHaScrittoLaLacuna = LogDelReplay.ContainsByPredicate([](const FRTTurnLogEntry& E)
	{
		return E.Category == ERTLogCategory::ReactionDecision
			&& E.ActionId == FName(TEXT("Action.Brace"));
	});
	TestFalse(TEXT("e il log del replay NON registra la finestra scoperta come se fosse stata decisa"),
		bHaScrittoLaLacuna);

	return true;
}

/**
 * 🔴 **Il BOT deve poter rispondere a una finestra di `Brace`, e la sua risposta dev'essere LEGALE.**
 *
 * `URTHexBotLibrary::DecideReactionResponse` cerca un `FIRE:` e altrimenti ripiega sulla scelta sicura. Fino
 * al 2026-08-19 quel ripiego era la **costante** `HOLD`, che per una finestra di `Brace` non e' fra le
 * `AllowedResponses` — `{Hold Ground, SIDESTEP}` — quindi il resolver la rifiutava con `HoldRejected`, cioe'
 * l'esito riservato a una risposta **stale o inventata**. In v0.1, che e' **2v2 offline vs bot**, ogni
 * unita' del bot in `Brace` con un profilo cadeva in quel caso: non un caso limite, il caso normale.
 *
 * ⚠️ **Il test verifica l'esito e non solo la stringa**: una risposta «giusta» che il resolver rifiuta
 * produce lo stesso spostamento (nessuno) di una accettata, quindi guardare la cella non distinguerebbe il
 * difetto. Cio' che cambia e' `ERTReactionDecisionOutcome`, ed e' quello che si legge qui.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBraceBotAnswerIsLegalTest,
	"RefactorTactics.Reactions.Brace.BotAnswerIsLegal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBraceBotAnswerIsLegalTest::RunTest(const FString&)
{
	// La finestra che il `Brace` di Phase apre davvero, costruita dal catalogo e non a mano: se domani il
	// vocabolario cambiasse, questo test cambierebbe con lui invece di pinnare una stringa morta.
	FRTReactionOpportunity Opp;
	Opp.AllowedResponses = URTCatalogLibrary::BraceExecutableResponses(TEXT("Profile.Sidestep"));
	if (!TestTrue(TEXT("la finestra del `Brace` si apre davvero"),
		URTReactionOpportunityLibrary::RequiresDecisionBoundary(Opp))) { return false; }

	const FString BotAnswer = URTHexBotLibrary::DecideReactionResponse(Opp);

	TestTrue(TEXT("il bot risponde qualcosa"), !BotAnswer.IsEmpty());
	TestTrue(TEXT("e la sua risposta e' LEGALE per quella finestra"),
		URTReactionOpportunityLibrary::IsResponseAllowed(Opp, BotAnswer));
	TestEqual(TEXT("ed e' la scelta sicura, cioe' `Hold Ground` e non `HOLD`"),
		BotAnswer, FString(TEXT("Hold Ground")));

	// L'altra meta': sull'Overwatch **niente cambia**. La stessa funzione, con un vocabolario che contiene
	// `HOLD`, continua a preferire il `FIRE:` disponibile — la correzione non ha spostato il comportamento
	// del bot dove funzionava.
	FRTReactionOpportunity Overwatch;
	Overwatch.AllowedResponses = {
		URTReactionOpportunityLibrary::FireResponse(3),
		URTReactionOpportunityLibrary::HoldResponse()
	};
	TestEqual(TEXT("sull'Overwatch il bot spara come sempre"),
		URTHexBotLibrary::DecideReactionResponse(Overwatch),
		URTReactionOpportunityLibrary::FireResponse(3));

	return true;
}

/**
 * **Il modulo reazione di default non duplica la reazione che l'eroe ha già nel kit.**
 *
 * Uno slot di loadout speso su una capacità che l'eroe possiede già non è una scelta: è una voce in più
 * nella lista, e il giocatore che arma l'una o l'altra ottiene la stessa cosa. Trovato misurando `#1403`:
 * `Hero.Riktor.Interposition` e `Reaction.AllyIntercept` erano **entrambi** costruiti su `Action.Intercept`.
 *
 * ⚠️ **A tabella, e la forma è la lezione di [D-201]**: il difetto era su **due** eroi su quattro, e
 * correggerne uno solo avrebbe reso quello l'eccezione mentre l'altro restava. Una riga per eroe, e chi
 * resta duplicato deve dichiararsi.
 *
 * ⚠️ **`DerivedFromActionId` e non `ActionId`**: sia `MakeHeroReactionFromCoreAction` sia
 * `MakeEquipmentAction` riscrivono il nome e conservano la derivazione ([D-195], `#1443`). Confrontare i
 * nomi darebbe sempre «diversi», e il test sarebbe verde per costruzione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDefaultReactionModuleIsNotADuplicateTest,
	"RefactorTactics.Equipment.DefaultReactionModuleIsNotADuplicate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDefaultReactionModuleIsNotADuplicateTest::RunTest(const FString&)
{
	struct FCaso
	{
		const TCHAR* HeroId;
		const URTHeroData* (*Make)();
		bool bDuplicatoDichiarato; //< eccezione nota, con la sua ragione qui sotto
		const TCHAR* Nota;
	};

	const FCaso Casi[] = {
		{ TEXT("Hero.Gadget"), []() -> const URTHeroData* { return URTHeroCatalogLibrary::MakeGadget(); },
		  true,
		  TEXT("⚠️ DUPLICATO DICHIARATO: `ReactiveShield` e `ReactiveCapacitor` sono entrambi `Action.Counter`. "
		       "Non corretto insieme a Riktor perche' il loadout di Gadget e' comunque VUOTO — `Gadget.Insulator` "
		       "non e' spedito e `DefaultLoadoutFor` e' tutto-o-niente — quindi il duplicato non raggiunge il "
		       "campo. Va corretto quando quel gadget arriva, e questa riga e' il promemoria (#1403).") },
		{ TEXT("Hero.Phase"), []() -> const URTHeroData* { return URTHeroCatalogLibrary::MakePhase(); },
		  false, TEXT("nessuna reazione nel kit: qualunque modulo e' una capacita' nuova") },
		{ TEXT("Hero.Riktor"), []() -> const URTHeroData* { return URTHeroCatalogLibrary::MakeRiktor(); },
		  false, TEXT("`Reaction.Cleanse` contro `Interposition` di kit: due capacita' diverse ([D-218])") },
		{ TEXT("Hero.Wraith"), []() -> const URTHeroData* { return URTHeroCatalogLibrary::MakeWraith(); },
		  false, TEXT("`EmergencyDash` (Counter) contro `Deflection` di kit (Deflect)") },
	};

	for (const FCaso& C : Casi)
	{
		const URTHeroData* Hero = C.Make();
		if (!TestNotNull(*FString::Printf(TEXT("%s: dato d'eroe"), C.HeroId), Hero)) { continue; }

		// La reazione che l'eroe porta nel proprio kit, se ne ha una.
		FName CoreDelKit;
		for (const URTActionData* A : Hero->Actions)
		{
			if (A && A->Def.Slot == ERTActionSlot::Reaction)
			{
				CoreDelKit = A->Def.DerivedFromActionId;
				break;
			}
		}

		const FName ModuloId = URTCatalogLibrary::DefaultReactionModuleFor(FName(C.HeroId));
		const URTEquipmentData* Modulo = URTCatalogLibrary::FindEquipment(ModuloId);
		if (!TestNotNull(*FString::Printf(TEXT("%s: il modulo prescritto e' spedito (%s)"),
			C.HeroId, *ModuloId.ToString()), Modulo))
		{
			continue;
		}

		const URTActionData* Concessa = URTCatalogLibrary::MakeEquipmentAction(Modulo, GetTransientPackage());
		if (!TestNotNull(*FString::Printf(TEXT("%s: il modulo concede un'azione"), C.HeroId), Concessa))
		{
			continue;
		}
		const FName CoreDelModulo = Concessa->Def.DerivedFromActionId;

		AddInfo(FString::Printf(TEXT("%s: kit=%s modulo=%s (%s) — %s"),
			C.HeroId, *CoreDelKit.ToString(), *ModuloId.ToString(), *CoreDelModulo.ToString(), C.Nota));

		// Un eroe senza reazione di kit non puo' duplicare niente: la riga passa per costruzione, ed e'
		// giusto che ci sia lo stesso — il giorno in cui quell'eroe ne acquista una, la tabella se ne accorge.
		if (CoreDelKit.IsNone()) { continue; }

		const bool bDuplicato = (CoreDelKit == CoreDelModulo);
		TestEqual(*FString::Printf(TEXT("%s: lo slot di loadout non spende su cio' che l'eroe ha gia'"), C.HeroId),
			bDuplicato, C.bDuplicatoDichiarato);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
