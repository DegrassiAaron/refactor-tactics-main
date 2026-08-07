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
 * `Action.Intercept` (CP 5.3): l'unica reazione che cambia il bersaglio di un attacco ALTRUI. Per questo
 * risolve in un pass suo, prima delle altre (priorita' 10, la piu' bassa del catalogo): se il bersaglio
 * originale valutasse il proprio Counter dopo la redirezione, contrattaccherebbe per un colpo mai ricevuto.
 */
namespace
{
	UWorld* MakeItcWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyItcWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Mappa piena; le celle in Blockers fermano la linea di tiro (per il test sulla traiettoria). */
	URTHexMapAsset* MakeItcMapAsset(int32 Radius, const TArray<FRTCellId>& Blockers = TArray<FRTCellId>())
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			FRTHexCellData Data(Id);
			Data.bBlocksLineOfSight = Blockers.Contains(Id);
			M->AddOrUpdateCell(Data);
		}
		M->SortCells();
		return M;
	}

	ARTHexMapActor* SpawnItcMap(UWorld* World, int32 Radius = 6, const TArray<FRTCellId>& Blockers = TArray<FRTCellId>())
	{
		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = MakeItcMapAsset(Radius, Blockers);
		return Actor;
	}

	ARTUnit* SpawnItcUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false;
		U->ConfigureAsArchetype(ERTArchetype::Ranger); // Tiro: 25 danni, colpo singolo, portata 6
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell;
		return U;
	}

	int32 AddItcAbility(ARTUnit* Unit, const TCHAR* ActionId, int32 SlotIndex = 3)
	{
		if (!Unit || !Unit->Abilities.IsValidIndex(SlotIndex)) { return INDEX_NONE; }
		URTActionData* Ability = NewObject<URTActionData>(Unit);
		Ability->Def = URTCatalogLibrary::FindCoreAction(FName(ActionId));
		Ability->CooldownTurns = Ability->Def.CooldownTurns;
		Ability->RangeCells = Ability->Def.RangeCells;
		Ability->Power = 0;
		Unit->Abilities[SlotIndex] = Ability;
		return SlotIndex;
	}

	void RunItcTurn(ARTTurnManager* TM)
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

	/** Scenario puro: attaccante 0 (team 1) colpisce la vittima 1 (team 0); l'intercettore 2 e' team 0. */
	struct FItcScene
	{
		TArray<FRTHexCombatUnit> Units;
		TArray<FRTHexAttackHit> Hits;
		TArray<FRTHexAttackIntent> Intents;

		FItcScene(ERTAbilityShape Shape, const FRTCellId& InterceptorCell)
		{
			FRTHexCombatUnit Attacker; Attacker.UnitId = 0; Attacker.TeamId = 1; Attacker.Cell = FRTCellId(4, 0);
			FRTHexCombatUnit Victim;   Victim.UnitId = 1;   Victim.TeamId = 0;   Victim.Cell = FRTCellId(0, 0);
			FRTHexCombatUnit Saver;    Saver.UnitId = 2;    Saver.TeamId = 0;    Saver.Cell = InterceptorCell;
			Units = { Attacker, Victim, Saver };

			FRTHexAttackIntent Intent;
			Intent.AttackerId = 0;
			Intent.TargetId = 1;
			Intent.Shape = Shape;
			Intents = { Intent };

			Hits.Add(FRTHexAttackHit(/*Attacker*/ 0, /*Target*/ 1, /*Power*/ 25, /*IntentIndex*/ 0));
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInterceptCatalogTest,
	"RefactorTactics.Reactions.InterceptMatchesCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInterceptCatalogTest::RunTest(const FString&)
{
	const FRTActionDef Def = URTCatalogLibrary::FindCoreAction(TEXT("Action.Intercept"));
	if (!TestTrue(TEXT("Action.Intercept e' nel catalogo"), Def.ActionId == FName(TEXT("Action.Intercept"))))
	{
		return false;
	}
	TestEqual(TEXT("priorita' 10"), Def.Priority, 10);
	TestEqual(TEXT("cooldown 2"), Def.CooldownTurns, 2);
	TestEqual(TEXT("range 2 (dichiarato dal catalogo)"), Def.RangeCells, 2);
	TestTrue(TEXT("slot reazione"), Def.Slot == ERTActionSlot::Reaction);
	TestTrue(TEXT("trigger sull'alleato colpito"), Def.ReactionTrigger == ERTReactionTrigger::AllyHitByDirectAttack);
	TestEqual(TEXT("nessun effetto: cambia CHI subisce, non cosa succede"), Def.Effects.Num(), 0);

	// La priorita' e' la regola che lo fa risolvere prima delle altre reazioni: se qualcuno la cambiasse,
	// il bersaglio originale contrattaccherebbe per un colpo che non ha piu' ricevuto.
	TestTrue(TEXT("Intercept precede Deflect"),
		Def.Priority < URTCatalogLibrary::FindCoreAction(TEXT("Action.Deflect")).Priority);
	TestTrue(TEXT("Intercept precede Counter"),
		Def.Priority < URTCatalogLibrary::FindCoreAction(TEXT("Action.Counter")).Priority);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInterceptTest,
	"RefactorTactics.Reactions.Intercept",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInterceptTest::RunTest(const FString&)
{
	// Nome vincolante del catalogo. L'intercettore DIVENTA il bersaglio: incassa lui, l'alleato no.
	UWorld* World = MakeItcWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnItcMap(World);

	ARTUnit* Attacker = SpawnItcUnit(World, 1, FRTCellId(4, 0));
	ARTUnit* Victim = SpawnItcUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Saver = SpawnItcUnit(World, 0, FRTCellId(1, 0)); // adiacente alla vittima: entro le 2 celle
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Attacker"), Attacker) || !TestNotNull(TEXT("Victim"), Victim)
		|| !TestNotNull(TEXT("Saver"), Saver) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyItcWorld(World);
		return false;
	}

	Saver->PlannedReactionAbility = AddItcAbility(Saver, TEXT("Action.Intercept"));
	Saver->PlannedAbilityIndex = INDEX_NONE;

	const int32 VictimHealth = Victim->Health;
	const int32 SaverHealth = Saver->Health;
	Attacker->PlannedAbilityIndex = 0; // Tiro: 25, colpo singolo
	Attacker->PlannedAttackTarget = Victim;

	RunItcTurn(TM);

	TestEqual(TEXT("l'interposizione si e' attivata"), CountReactionOutcome(TM, ERTReactionOutcome::Activated), 1);
	TestEqual(TEXT("l'alleato protetto non incassa nulla"), Victim->Health, VictimHealth);
	TestEqual(TEXT("l'intercettore incassa al posto suo"), SaverHealth - Saver->Health, 25);

	// Il TurnLog deve dire da CHI a chi e' passato il colpo: un danno su un'unita' mai bersagliata sarebbe
	// altrimenti inspiegabile nel replay.
	bool bFound = false;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category == ERTLogCategory::Reaction
			&& E.Outcome == static_cast<uint8>(ERTReactionOutcome::Activated)
			&& E.SrcCell == FRTCellId(0, 0)   // bersaglio ORIGINALE
			&& E.TgtCell == FRTCellId(1, 0))  // bersaglio FINALE
		{
			bFound = true;
		}
	}
	TestTrue(TEXT("il TurnLog registra bersaglio originale e finale"), bFound);

	DestroyItcWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInterceptRejectsAoETest,
	"RefactorTactics.Reactions.InterceptRejectsAoE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInterceptRejectsAoETest::RunTest(const FString&)
{
	// Nome vincolante. Un'area non ha una traiettoria in cui mettersi in mezzo: il catalogo lo esclude.
	URTHexMapAsset* Map = MakeItcMapAsset(6);
	const TSet<int32> NoClaims;

	FItcScene Area(ERTAbilityShape::Area, FRTCellId(1, 0));
	TestEqual(TEXT("un'area non e' intercettabile"),
		URTReactionLibrary::FindInterceptableHit(/*SelfId*/ 2, /*Range*/ 2, Area.Hits, Area.Intents,
			Area.Units, Map, NoClaims),
		static_cast<int32>(INDEX_NONE));

	// Anche la linea, per la stessa ragione: colpisce lungo un percorso, non un bersaglio da coprire.
	FItcScene Line(ERTAbilityShape::Line, FRTCellId(1, 0));
	TestEqual(TEXT("nemmeno una linea"),
		URTReactionLibrary::FindInterceptableHit(2, 2, Line.Hits, Line.Intents, Line.Units, Map, NoClaims),
		static_cast<int32>(INDEX_NONE));

	// Controprova: lo stesso identico scenario, ma con un colpo SINGOLO, si intercetta.
	FItcScene Single(ERTAbilityShape::Single, FRTCellId(1, 0));
	TestEqual(TEXT("un colpo diretto invece si intercetta"),
		URTReactionLibrary::FindInterceptableHit(2, 2, Single.Hits, Single.Intents, Single.Units, Map, NoClaims), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInterceptRejectsHazardTest,
	"RefactorTactics.Reactions.InterceptRejectsHazard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInterceptRejectsHazardTest::RunTest(const FString&)
{
	// Nome vincolante. Gli hazard ambientali non esistono ancora (epic E8/CP 8.2): la clausola si verifica per
	// COSTRUZIONE, non simulando un hazard che non c'e'. Un danno ambientale non nasce da un colpo raccolto in
	// `Plan.Hits` e non ha un `AttackerId`, quindi non c'e' nulla da intercettare — e' esattamente cio' che
	// succede quando l'array dei colpi non contiene la ferita.
	URTHexMapAsset* Map = MakeItcMapAsset(6);
	const TSet<int32> NoClaims;

	FItcScene Scene(ERTAbilityShape::Single, FRTCellId(1, 0));

	// Nessun colpo raccolto = la forma che avra' il danno ambientale: niente da intercettare.
	TestEqual(TEXT("un danno che non passa dai colpi non e' intercettabile"),
		URTReactionLibrary::FindInterceptableHit(2, 2, {}, {}, Scene.Units, Map, NoClaims),
		static_cast<int32>(INDEX_NONE));

	// Un colpo senza attaccante valido (l'altra forma possibile di danno non attribuito) viene scartato invece
	// di essere intercettato "da nessuno".
	TArray<FRTHexAttackHit> Orphan;
	Orphan.Add(FRTHexAttackHit(/*Attacker*/ 99, /*Target*/ 1, /*Power*/ 12, /*IntentIndex*/ 0));
	TestEqual(TEXT("un colpo senza attaccante valido non e' intercettabile"),
		URTReactionLibrary::FindInterceptableHit(2, 2, Orphan, Scene.Intents, Scene.Units, Map, NoClaims),
		static_cast<int32>(INDEX_NONE));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInterceptTrajectoryTest,
	"RefactorTactics.Reactions.InterceptRequiresCompatibleTrajectory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInterceptTrajectoryTest::RunTest(const FString&)
{
	// Nome vincolante. Ci si mette in mezzo a un colpo, non lo si teletrasporta addosso: se la linea
	// dall'attaccante all'INTERCETTORE e' bloccata, l'interposizione non avviene — anche se l'alleato e'
	// vicinissimo e il colpo su di lui passa benissimo.
	const TSet<int32> NoClaims;

	// L'intercettore sta in (0,1); un muro in (2,1) taglia la linea dall'attaccante (4,0) verso di lui,
	// lasciando invece libera quella verso la vittima in (0,0).
	TArray<FRTCellId> Blockers;
	Blockers.Add(FRTCellId(2, 1));
	URTHexMapAsset* Blocked = MakeItcMapAsset(6, Blockers);

	FItcScene Scene(ERTAbilityShape::Single, FRTCellId(0, 1));
	const int32 WithWall = URTReactionLibrary::FindInterceptableHit(2, 2, Scene.Hits, Scene.Intents,
		Scene.Units, Blocked, NoClaims);

	// Stessa identica scena senza il muro: e' il muro a fare la differenza, non la geometria.
	URTHexMapAsset* Clear = MakeItcMapAsset(6);
	const int32 WithoutWall = URTReactionLibrary::FindInterceptableHit(2, 2, Scene.Hits, Scene.Intents,
		Scene.Units, Clear, NoClaims);

	TestEqual(TEXT("traiettoria bloccata: nessuna interposizione"), WithWall, static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("stessa scena senza muro: si intercetta"), WithoutWall, 0);

	// Fail-closed: senza mappa la traiettoria non e' verificabile, quindi non si intercetta.
	TestEqual(TEXT("senza mappa non si intercetta"),
		URTReactionLibrary::FindInterceptableHit(2, 2, Scene.Hits, Scene.Intents, Scene.Units, nullptr, NoClaims),
		static_cast<int32>(INDEX_NONE));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInterceptRangeAndAllyTest,
	"RefactorTactics.Reactions.InterceptOnlyNearbyAllies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInterceptRangeAndAllyTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeItcMapAsset(8);
	const TSet<int32> NoClaims;

	// Entro 2 celle: si intercetta. A 3: no. Il numero e' quello del catalogo.
	FItcScene Near(ERTAbilityShape::Single, FRTCellId(2, 0));
	TestEqual(TEXT("alleato a 2 celle: intercettabile"),
		URTReactionLibrary::FindInterceptableHit(2, 2, Near.Hits, Near.Intents, Near.Units, Map, NoClaims), 0);

	FItcScene Far(ERTAbilityShape::Single, FRTCellId(3, 0));
	TestEqual(TEXT("alleato a 3 celle: fuori portata"),
		URTReactionLibrary::FindInterceptableHit(2, 2, Far.Hits, Far.Intents, Far.Units, Map, NoClaims),
		static_cast<int32>(INDEX_NONE));

	// Un NEMICO colpito non si protegge, per quanto vicino.
	FItcScene Enemy(ERTAbilityShape::Single, FRTCellId(1, 0));
	Enemy.Units[1].TeamId = 1; // la vittima passa nella squadra dell'attaccante
	TestEqual(TEXT("non ci si interpone per un nemico"),
		URTReactionLibrary::FindInterceptableHit(2, 2, Enemy.Hits, Enemy.Intents, Enemy.Units, Map, NoClaims),
		static_cast<int32>(INDEX_NONE));

	// Non ci si interpone davanti a se stessi: quello e' il caso di Counter/Deflect, non di Intercept.
	FItcScene Self(ERTAbilityShape::Single, FRTCellId(1, 0));
	Self.Hits[0].TargetId = 2; // il colpo e' gia' sull'intercettore
	TestEqual(TEXT("non ci si interpone per se stessi"),
		URTReactionLibrary::FindInterceptableHit(2, 2, Self.Hits, Self.Intents, Self.Units, Map, NoClaims),
		static_cast<int32>(INDEX_NONE));

	// Un colpo gia' reclamato da un altro intercettore si salta invece di contenderlo.
	FItcScene Claimed(ERTAbilityShape::Single, FRTCellId(1, 0));
	TSet<int32> Taken;
	Taken.Add(0);
	TestEqual(TEXT("un colpo gia' intercettato si salta"),
		URTReactionLibrary::FindInterceptableHit(2, 2, Claimed.Hits, Claimed.Intents, Claimed.Units, Map, Taken),
		static_cast<int32>(INDEX_NONE));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInterceptBeforeOtherReactionsTest,
	"RefactorTactics.Reactions.InterceptResolvesBeforeOtherReactions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInterceptBeforeOtherReactionsTest::RunTest(const FString&)
{
	// La ragione per cui Intercept ha priorita' 10 e un pass tutto suo: il bersaglio ORIGINALE non deve
	// contrattaccare per un colpo che non ha piu' ricevuto. Qui la vittima tiene pronto un Counter e viene
	// protetta: il suo contrattacco NON deve partire.
	UWorld* World = MakeItcWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnItcMap(World);

	ARTUnit* Attacker = SpawnItcUnit(World, 1, FRTCellId(4, 0));
	ARTUnit* Victim = SpawnItcUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Saver = SpawnItcUnit(World, 0, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Attacker"), Attacker) || !TestNotNull(TEXT("Victim"), Victim)
		|| !TestNotNull(TEXT("Saver"), Saver) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyItcWorld(World);
		return false;
	}

	Saver->PlannedReactionAbility = AddItcAbility(Saver, TEXT("Action.Intercept"));
	Saver->PlannedAbilityIndex = INDEX_NONE;
	Victim->PlannedReactionAbility = AddItcAbility(Victim, TEXT("Action.Counter"));
	Victim->PlannedAbilityIndex = INDEX_NONE;

	const int32 AttackerHealth = Attacker->Health;
	Attacker->PlannedAbilityIndex = 0;
	Attacker->PlannedAttackTarget = Victim;

	RunItcTurn(TM);

	TestEqual(TEXT("l'attaccante non subisce il contrattacco di chi non e' stato colpito"),
		Attacker->Health, AttackerHealth);
	TestEqual(TEXT("una attivazione (l'interposizione) e una non-attivazione (il contrattacco)"),
		CountReactionOutcome(TM, ERTReactionOutcome::Activated), 1);
	TestEqual(TEXT("il Counter del protetto risulta non innescato"),
		CountReactionOutcome(TM, ERTReactionOutcome::NotTriggered), 1);

	DestroyItcWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
