#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Combat/RTCombatLibrary.h"
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
#include "Ability/RTHeroData.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * CP 5.5 — reazioni COMPONIBILI: il motore di E5 regge le reazioni degli eroi senza un ramo per eroe.
 *
 * Tre proprieta', una per test:
 * 1. una reazione applica TUTTI gli effetti che dichiara, non il primo che il resolver riconosce;
 * 2. una reazione d'eroe costruita sopra la semantica core conserva la PROPRIA identita' (ActionId nel
 *    TurnLog, cooldown proprio, effetti propri);
 * 3. il resolver non guarda l'ActionId: permutarlo, a parita' di semantica, non cambia l'esito.
 *
 * Prefissi `Comp*` negli helper: nella unity build questo file condivide la translation unit con gli altri
 * test delle reazioni, e i namespace anonimi si fondono — due helper omonime sarebbero una ridefinizione.
 */
namespace
{
	UWorld* MakeCompWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyCompWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	void SpawnCompMap(UWorld* World, int32 Radius = 6)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
	}

	/** Ranger: attacco base a colpo SINGOLO da 25 — il "danno diretto" su cui i trigger reagiscono. */
	ARTUnit* SpawnCompUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false;
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeVektor());
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell; // fermo: questi test guardano il Blast
		return U;
	}

	/**
	 * Mette la reazione nello slot 3 (lo scatto del Ranger) invece di accodarla: `AbilityCooldowns` e'
	 * dimensionato sul conteggio originale delle abilita' e non si allarga da solo.
	 */
	int32 SetCompReaction(ARTUnit* Unit, URTActionData* Reaction, int32 SlotIndex = 3)
	{
		if (!Unit || !Reaction || !Unit->Abilities.IsValidIndex(SlotIndex)) { return INDEX_NONE; }
		Unit->Abilities[SlotIndex] = Reaction;
		return SlotIndex;
	}

	void RunCompTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/** La prima voce di reazione ATTIVATA del TurnLog, o nullptr: l'identita' si legge da li'. */
	const FRTTurnLogEntry* FindCompActivatedReaction(const ARTTurnManager* TM)
	{
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Reaction
				&& E.Outcome == static_cast<uint8>(ERTReactionOutcome::Activated))
			{
				return &E;
			}
		}
		return nullptr;
	}

	constexpr int32 CompBasicAttackDamage = 25; // attacco base del Ranger, colpo singolo
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMultiEffectReactionTest,
	"RefactorTactics.Reactions.MultiEffectReactionAppliesAll",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMultiEffectReactionTest::RunTest(const FString&)
{
	// `Flux.ReactiveCapacitor` in miniatura (il cablaggio dell'eroe e' CP 6.7): scudo a chi reagisce **e**
	// danno a chi ha colpito, dichiarati nella stessa lista. Prima di CP 5.5 il resolver leggeva solo il primo
	// `Damage` e ignorava tutto il resto: meta' della reazione non arrivava mai in partita.
	UWorld* World = MakeCompWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnCompMap(World);

	ARTUnit* Reactor = SpawnCompUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Attacker = SpawnCompUnit(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Reactor"), Reactor) || !TestNotNull(TEXT("Attacker"), Attacker)
		|| !TestNotNull(TEXT("TM"), TM))
	{
		DestroyCompWorld(World);
		return false;
	}

	constexpr int32 ShieldAmount = 15;
	constexpr int32 CounterAmount = 10;
	URTActionData* Capacitor = URTHeroCatalogLibrary::MakeHeroReactionFromCoreAction(
		TEXT("Test.ReactiveCapacitor"), TEXT("Action.Counter"), /*Cooldown*/ 3,
		{ FRTActionEffectSpec(ERTActionEffect::Shield, ShieldAmount),
		  FRTActionEffectSpec(ERTActionEffect::Damage, CounterAmount) });
	if (!TestNotNull(TEXT("reazione d'eroe costruita dalla semantica core"), Capacitor))
	{
		DestroyCompWorld(World);
		return false;
	}

	Reactor->PlannedReactionAbility = SetCompReaction(Reactor, Capacitor);
	Reactor->PlannedAbilityIndex = INDEX_NONE;

	const int32 ReactorBefore = Reactor->Health;
	const int32 AttackerBefore = Attacker->Health;
	Attacker->PlannedAbilityIndex = 0; // attacco base, colpo singolo da 25
	Attacker->PlannedAttackTarget = Reactor;

	RunCompTurn(TM);

	TestNotNull(TEXT("la reazione risulta attivata nel TurnLog"), FindCompActivatedReaction(TM));
	// Lo scudo vale sul colpo che ha innescato la reazione, come il -20 di Deflect: altrimenti scadrebbe nel
	// Cleanup dello stesso turno senza aver protetto da nulla.
	TestEqual(TEXT("lo scudo della reazione assorbe parte del colpo"),
		ReactorBefore - Reactor->Health, CompBasicAttackDamage - ShieldAmount);
	TestEqual(TEXT("e il secondo effetto colpisce comunque l'attaccante"),
		AttackerBefore - Attacker->Health, CounterAmount);

	DestroyCompWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroReactionIdentityTest,
	"RefactorTactics.Reactions.HeroReactionKeepsIdentityInLog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroReactionIdentityTest::RunTest(const FString&)
{
	// Riusare la semantica core non significa diventare l'azione core: nel TurnLog resta l'ActionId D'EROE, e
	// il cooldown e' quello dichiarato dall'eroe, non quello di `Action.Deflect`.
	UWorld* World = MakeCompWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnCompMap(World);

	ARTUnit* Reactor = SpawnCompUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Attacker = SpawnCompUnit(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Reactor"), Reactor) || !TestNotNull(TEXT("Attacker"), Attacker)
		|| !TestNotNull(TEXT("TM"), TM))
	{
		DestroyCompWorld(World);
		return false;
	}

	const FRTActionDef CoreDeflect = URTCatalogLibrary::FindCoreAction(TEXT("Action.Deflect"));
	constexpr int32 HeroCooldown = 4; // diverso da quello del core: se il log mentisse, i due si confonderebbero
	URTActionData* Deflection = URTHeroCatalogLibrary::MakeHeroReactionFromCoreAction(
		TEXT("Test.Deflection"), TEXT("Action.Deflect"), HeroCooldown, CoreDeflect.Effects);
	if (!TestNotNull(TEXT("reazione d'eroe costruita dalla semantica core"), Deflection))
	{
		DestroyCompWorld(World);
		return false;
	}
	TestNotEqual(TEXT("il cooldown d'eroe non e' quello del core"), HeroCooldown, CoreDeflect.CooldownTurns);
	TestTrue(TEXT("il trigger arriva dalla semantica core"),
		Deflection->Def.ReactionTrigger == CoreDeflect.ReactionTrigger);
	TestTrue(TEXT("e occupa lo slot Reazione"), Deflection->Def.Slot == ERTActionSlot::Reaction);

	const int32 ReactionIndex = SetCompReaction(Reactor, Deflection);
	Reactor->PlannedReactionAbility = ReactionIndex;
	Reactor->PlannedAbilityIndex = INDEX_NONE;

	Attacker->PlannedAbilityIndex = 0;
	Attacker->PlannedAttackTarget = Reactor;

	RunCompTurn(TM);

	const FRTTurnLogEntry* Entry = FindCompActivatedReaction(TM);
	if (!TestNotNull(TEXT("la reazione risulta attivata nel TurnLog"), Entry))
	{
		DestroyCompWorld(World);
		return false;
	}
	TestTrue(TEXT("il TurnLog porta l'ActionId d'eroe, non quello del core"),
		Entry->ActionId == FName(TEXT("Test.Deflection")));

	// Cooldown PROPRIO, verificato dove il dato vive e su ENTRAMBI i campi: `ARTUnit::ConsumeAbility` legge
	// quello LEGACY, quindi un helper che popolasse solo `Def` produrrebbe una reazione riutilizzabile ogni
	// turno, senza che nulla lo segnali.
	//
	// Il commento qui sopra diceva che il residuo a runtime «non e' osservabile»: valeva fino a **#135**, che
	// ha riallineato `AbilityCooldowns` al kit in tutti i punti che lo popolano invece del solo `BeginPlay`.
	// Oggi il residuo si osserva su un'unita' configurata (`RefactorTactics.Unit.ArchetypeKitRecordsCooldown`).
	// Resta scoperto il caso degli helper che aggiungono abilita' DOPO la configurazione: quelli allungano
	// `Abilities` senza risincronizzare, e per quegli slot il cooldown legge ancora 0.
	TestEqual(TEXT("il cooldown d'eroe e' nel Def"), Deflection->Def.CooldownTurns, HeroCooldown);
	TestEqual(TEXT("e nel campo legacy che il turno consuma davvero"), Deflection->CooldownTurns, HeroCooldown);
	TestTrue(TEXT("l'indice della reazione e' valido"), ReactionIndex != INDEX_NONE);

	DestroyCompWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNoHeroBranchInResolverTest,
	"RefactorTactics.Reactions.NoHeroSpecificBranchInResolver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNoHeroBranchInResolverTest::RunTest(const FString&)
{
	// Il vincolo «nessun `if (ActionId == …)` nel resolver» verificato dal COMPORTAMENTO: a parita' di
	// semantica dichiarata, l'ActionId e' opaco al motore. Se restasse un ramo per `Action.Deflect`, la
	// versione d'eroe non ridurrebbe nulla e i due esiti divergerebbero.
	auto RunDeflectScenario = [this](const TCHAR* ActionId) -> int32
	{
		UWorld* World = MakeCompWorld();
		if (!World) { return INDEX_NONE; }
		SpawnCompMap(World);

		ARTUnit* Reactor = SpawnCompUnit(World, 0, FRTCellId(0, 0));
		ARTUnit* Attacker = SpawnCompUnit(World, 1, FRTCellId(1, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Reactor || !Attacker || !TM)
		{
			DestroyCompWorld(World);
			return INDEX_NONE;
		}

		const FRTActionDef CoreDeflect = URTCatalogLibrary::FindCoreAction(TEXT("Action.Deflect"));
		URTActionData* Reaction = URTHeroCatalogLibrary::MakeHeroReactionFromCoreAction(
			FName(ActionId), TEXT("Action.Deflect"), CoreDeflect.CooldownTurns, CoreDeflect.Effects);
		Reactor->PlannedReactionAbility = SetCompReaction(Reactor, Reaction);
		Reactor->PlannedAbilityIndex = INDEX_NONE;

		const int32 Before = Reactor->Health;
		Attacker->PlannedAbilityIndex = 0;
		Attacker->PlannedAttackTarget = Reactor;

		RunCompTurn(TM);
		const int32 Damage = Before - Reactor->Health;

		DestroyCompWorld(World);
		return Damage;
	};

	// Stessa semantica, tre identita' diverse: l'esito non cambia.
	const int32 CoreDamage = RunDeflectScenario(TEXT("Action.Deflect"));
	const int32 HeroDamage = RunDeflectScenario(TEXT("Vektor.Deflection"));
	const int32 OtherDamage = RunDeflectScenario(TEXT("Zzz.PermutedIdentity"));

	TestEqual(TEXT("la riduzione arriva dai dati: 25 - 20"),
		CoreDamage, CompBasicAttackDamage - URTCombatLibrary::DeflectDamageReduction);
	TestEqual(TEXT("un ActionId d'eroe non cambia l'esito"), HeroDamage, CoreDamage);
	TestEqual(TEXT("nemmeno un ActionId arbitrario"), OtherDamage, CoreDamage);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
