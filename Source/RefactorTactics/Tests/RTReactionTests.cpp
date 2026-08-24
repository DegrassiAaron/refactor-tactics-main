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
#include "Core/RTGameplayTags.h" // TAG_Status_Root/Slow/Burning: la lista degli stati di controllo (CP 7.5)
#include "Turn/RTReactionLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Infrastruttura dello slot reazione (CP 5.1): trigger valutato sullo snapshot del Blast, nessuna attesa nel
 * resolver, massimo un'attivazione per turno. `Action.Counter` e' il veicolo reale del checkpoint.
 *
 * ⚠️ **Il contrattacco da 16 danni E' applicato**, e questa riga diceva il contrario. `RTTurnManager.cpp`
 * riempie `Out.CounterAttacks` quando il trigger scatta e poi fa `Attacks.Append(Reactions.CounterAttacks)`:
 * il danno entra nella pipeline degli attacchi come ogni altro colpo. *(`Reaction.CounterShot`, la variante
 * d'equipaggiamento, ne fa **14** — «invece dei 16 di `Action.Counter`», e il catalogo lo dichiara.)*
 *
 * 🔴 **Diceva *«NON e' ancora applicato: arriva con CP 5.2»*, e il difetto era peggiore che altrove perche'
 * sta in un TEST**: chi legge un docstring per sapere cosa il test copre riceve una descrizione del sistema,
 * e qui la descrizione era vecchia di mesi senza che niente potesse dirlo (#1322).
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
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeWraith()); // attacco base a colpo singolo (Ranger.Shot, 25, range 6)
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell; // fermo: i test guardano il Blast, non il movimento
		return U;
	}

	/**
	 * Sostituisce un'abilita' esistente (indice 3, inutilizzata nei test qui sotto) con la reazione, invece di
	 * accodarla con `Abilities.Add`: `AbilityCooldowns` (privato, parallelo a `Abilities`) e' dimensionato dalla
	 * configurazione sul conteggio ORIGINALE e non si allarga da solo — un'abilita' accodata dopo finirebbe
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

// ⚠️ `ClientContext` oltre a `EditorContext`, e non per simmetria con i vicini: e' uno dei **dieci test
// vincolanti** del catalogo (`roadmap-v0.1.md` §6), e CP 12.3 (`#83`) chiede che la suite giri anche in un
// pacchetto. Con il solo `EditorContext` il controller lo filtra fuori dal target `Game` — misurato:
// `Automation List` in un pacchetto Development registrava **zero** test su 875, tutti dichiarati solo Editor.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionSingleActivationTest,
	"RefactorTactics.Reactions.SingleActivation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
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

// =====================================================================================================
// CP 7.5 (`#505`) — ogni trigger dichiara DOVE viene valutato.
//
// Il difetto che questo test esiste per impedire e' quello che ha tenuto fermi tre moduli del catalogo per
// due checkpoint: un trigger presente nel dato che nessun punto del turno guarda. Non e' un errore di
// compilazione, non rompe nessun altro test, e il test sul catalogo resta verde — semplicemente la reazione
// non scatta mai in partita.
// =====================================================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionTriggersHaveAPassPointTest,
	"RefactorTactics.Reaction.EveryTriggerHasAPassPoint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionTriggersHaveAPassPointTest::RunTest(const FString&)
{
	const UEnum* Enum = StaticEnum<ERTReactionTrigger>();
	if (!TestNotNull(TEXT("l'enum dei trigger e' riflesso"), Enum)) { return false; }

	int32 Valutati = 0;
	for (int32 i = 0; i < Enum->NumEnums() - 1; ++i) // -1: l'ultima voce e' `_MAX`, sintetizzata da UHT
	{
		const ERTReactionTrigger Trigger = static_cast<ERTReactionTrigger>(Enum->GetValueByIndex(i));
		const ERTReactionPassPoint Point = URTReactionLibrary::PassPointFor(Trigger);
		if (Trigger == ERTReactionTrigger::None)
		{
			TestTrue(TEXT("`None` non e' valutato da nessuno: e' l'ASSENZA di un trigger"),
				Point == ERTReactionPassPoint::Never);
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s dichiara dove viene valutato"), *Enum->GetNameStringByIndex(i)),
			Point != ERTReactionPassPoint::Never);
		++Valutati;
	}

	// Un numero CONCRETO, non «almeno uno»: un trigger nuovo che entrasse nell'enum senza passare da
	// `PassPointFor` farebbe cadere questo conteggio, e chiederebbe di essere deciso invece di scivolare via.
	TestEqual(TEXT("i trigger valutati in partita sono cinque"), Valutati, 5);

	// E il mapping esatto, che e' la parte che un refactor puo' spostare in silenzio.
	TestTrue(TEXT("i colpi diretti si valutano sui colpi gia' raccolti"),
		URTReactionLibrary::PassPointFor(ERTReactionTrigger::HitByDirectAttack)
			== ERTReactionPassPoint::BlastHits);
	TestTrue(TEXT("l'interposizione ha il suo ciclo, non il pass generale"),
		URTReactionLibrary::PassPointFor(ERTReactionTrigger::AllyHitByDirectAttack)
			== ERTReactionPassPoint::BlastIntercept);
	TestTrue(TEXT("lo spostamento si valuta dov'e' deciso e non ancora applicato"),
		URTReactionLibrary::PassPointFor(ERTReactionTrigger::AboutToBeDisplaced)
			== ERTReactionPassPoint::BlastDisplacement);
	TestTrue(TEXT("il controllo si valuta sugli stati raccolti, prima che vengano applicati"),
		URTReactionLibrary::PassPointFor(ERTReactionTrigger::AboutToReceiveControl)
			== ERTReactionPassPoint::BlastStatus);
	TestTrue(TEXT("l'hazard si valuta nel Cleanup, fra la nascita della superficie e i suoi effetti"),
		URTReactionLibrary::PassPointFor(ERTReactionTrigger::CellBecameHazardous)
			== ERTReactionPassPoint::CleanupSurfaceBirth);
	return true;
}

// =====================================================================================================
// CP 7.5 (`#505`) — quali stati sono di CONTROLLO, e in che ordine di gravita'.
//
// La lista vive nel codice e non nel catalogo (la v0.1 non ha un campo per dirlo), quindi e' un limite
// dichiarato: questo test lo pinna. Diventa rosso quando nasce un terzo stato di controllo, che e'
// esattamente il momento in cui qualcuno deve decidere dove metterlo — invece di scoprire tre mesi dopo
// che `Reaction.Cleanse` non lo vedeva.
// =====================================================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTControlStatusesAreTwoTest,
	"RefactorTactics.Reaction.ControlStatusesAreTwo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTControlStatusesAreTwoTest::RunTest(const FString&)
{
	const TArray<FGameplayTag>& Controls = URTReactionLibrary::ControlStatusesBySeverity();
	if (!TestEqual(TEXT("gli stati di controllo della v0.1 sono due"), Controls.Num(), 2)) { return false; }

	// L'ORDINE e' il contratto, non l'insieme: `Root` azzera il budget di movimento, `Slow` ne aumenta il
	// costo. Con due controlli nello stesso Blast si annulla il primo di questa lista.
	TestEqual(TEXT("`Root` e' il piu' grave"), URTReactionLibrary::ControlSeverityRank(TAG_Status_Root), 0);
	TestEqual(TEXT("`Slow` viene dopo"), URTReactionLibrary::ControlSeverityRank(TAG_Status_Slow), 1);

	// Uno stato che NON e' controllo non deve entrare nel confronto: senza questo, un rank di -1 usato come
	// indice sarebbe un difetto silenzioso.
	TestEqual(TEXT("`Burning` non e' un controllo"),
		URTReactionLibrary::ControlSeverityRank(TAG_Status_Burning), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("un tag vuoto non e' un controllo"),
		URTReactionLibrary::ControlSeverityRank(FGameplayTag()), static_cast<int32>(INDEX_NONE));
	return true;
}

// =====================================================================================================
// [D-092] — «una attivazione per turno» e' un CONTATORE, non una conseguenza.
//
// Senza `ReactionActivationsThisTurn` la regola sarebbe comunque vera, ma solo come effetto del fatto che
// `PassPointFor` da' a ogni trigger un punto di valutazione solo: nessuna unita' viene guardata due volte.
// E' una garanzia che vive nella forma del mapping invece che in una regola scritta — cadrebbe **in
// silenzio** il giorno in cui un trigger ne avesse due, o un punto girasse due volte.
//
// ⚠️ Questo test **pre-imposta il contatore**, e va detto perche' sembra un trucco: oggi nessun percorso di
// gioco valuta la stessa unita' in due punti, quindi la guardia non ha un caso naturale. Pre-impostarlo e'
// il modo di esercitare esattamente lo stato in cui l'unita' si troverebbe al SECONDO punto — cioe' la
// condizione che la guardia esiste per gestire. Senza, il contatore sarebbe codice che nessun test tocca.
// =====================================================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionCounterBlocksSecondTest,
	"RefactorTactics.Reactions.CounterBlocksASecondActivation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionCounterBlocksSecondTest::RunTest(const FString&)
{
	// --- PREMESSA: con il contatore a zero la reazione scatta -----------------------------------------
	// Senza, «non e' scattata» sarebbe anche l'esito di una scena in cui il colpo non arriva.
	{
		UWorld* World = MakeReactionWorld();
		if (!TestNotNull(TEXT("world della premessa"), World)) { return false; }
		SpawnReactionMap(World);
		ARTUnit* Reactor = SpawnReactionUnit(World, 0, FRTCellId(0, 0));
		ARTUnit* Attacker = SpawnReactionUnit(World, 1, FRTCellId(1, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Reactor || !Attacker || !TM) { DestroyReactionWorld(World); return false; }

		Reactor->PlannedReactionAbility = AddReactionAbility(Reactor, TEXT("Action.Counter"));
		Reactor->PlannedAbilityIndex = INDEX_NONE;
		Attacker->PlannedAbilityIndex = 0;
		Attacker->PlannedAttackTarget = Reactor;
		RunReactionTurn(TM);

		const int32 Attivate = CountSlotReactionOutcome(TM, ERTReactionOutcome::Activated);
		// E il contatore torna a ZERO a fine turno: «una per TURNO» ha bisogno che il turno finisca davvero,
		// altrimenti l'unita' non reagirebbe mai piu' per il resto della partita.
		const int32 Residuo = Reactor->ReactionActivationsThisTurn;
		DestroyReactionWorld(World);
		if (!TestEqual(TEXT("premessa: con il contatore a zero la reazione si attiva"), Attivate, 1))
		{
			return false;
		}
		TestEqual(TEXT("e a fine turno il contatore e' stato azzerato"), Residuo, 0);
	}

	// --- IL CASO: il contatore gia' speso blocca la seconda attivazione ------------------------------
	UWorld* World = MakeReactionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnReactionMap(World);

	ARTUnit* Reactor = SpawnReactionUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Attacker = SpawnReactionUnit(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!Reactor || !Attacker || !TM) { DestroyReactionWorld(World); return false; }

	Reactor->PlannedReactionAbility = AddReactionAbility(Reactor, TEXT("Action.Counter"));
	Reactor->PlannedAbilityIndex = INDEX_NONE;

	// Lo stato in cui l'unita' arriverebbe al SECONDO punto di valutazione dello stesso turno.
	Reactor->ReactionActivationsThisTurn = 1;

	Attacker->PlannedAbilityIndex = 0; // stesso colpo diretto della premessa
	Attacker->PlannedAttackTarget = Reactor;
	RunReactionTurn(TM);

	TestEqual(TEXT("la seconda attivazione e' bloccata"),
		CountSlotReactionOutcome(TM, ERTReactionOutcome::Activated), 0);
	// E l'esito e' DETTO, non silenzioso: `Unavailable` e' la stessa voce del cooldown e dello Sprint —
	// «pianificata ma non poteva scattare». Una reazione sparita senza riga sarebbe indistinguibile da un
	// trigger che non e' scattato.
	TestEqual(TEXT("e il TurnLog lo dichiara come `Unavailable`, non la tace"),
		CountSlotReactionOutcome(TM, ERTReactionOutcome::Unavailable), 1);

	DestroyReactionWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
