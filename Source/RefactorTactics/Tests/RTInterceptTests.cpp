#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Combat/RTCombatLibrary.h"
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
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"

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
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeVektor()); // attacco base a colpo singolo
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell;
		return U;
	}

	/**
	 * Il colpo pieno dell'attacco base, letto dal catalogo invece che scritto nei test.
	 *
	 * Era `25`, il Tiro del Ranger legacy. Le proprieta' qui sotto — chi incassa al posto di chi, quale
	 * copertura vale per chi si interpone — non dipendono da quanto fa quel colpo, ma ci si appoggiavano.
	 */
	int32 ItcFullHit()
	{
		const URTHeroData* Hero = URTHeroCatalogLibrary::MakeVektor();
		return (Hero && Hero->Actions.Num() > 0 && Hero->Actions[0]) ? Hero->Actions[0]->Power : 0;
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

	/** Copertura bassa sul bordo indicato della cella, sull'asset gia' montato nell'actor mappa. */
	void AddItcLowCover(ARTHexMapActor* MapActor, const FRTCellId& Cell, ERTHexDirection Edge)
	{
		if (!MapActor || !MapActor->MapAsset) { return; }
		const FRTHexCellData* Existing = MapActor->MapAsset->FindCell(Cell);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Cell);
		Data.Covers.Add(FRTHexCover(Edge, ERTHexCoverType::Low,
			FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low)));
		MapActor->MapAsset->AddOrUpdateCell(Data);
		MapActor->MapAsset->SortCells();
	}

	/** Danno registrato dal TurnLog sulla cella indicata (-1 se nessuna voce di combattimento la nomina). */
	int32 LoggedCombatDamageOn(const ARTTurnManager* TM, const FRTCellId& Cell)
	{
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Combat && E.TgtCell == Cell) { return E.Amount; }
		}
		return -1;
	}

	int32 CountInterceptOutcome(const ARTTurnManager* TM, ERTReactionOutcome Outcome)
	{
		int32 N = 0;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Reaction && E.Outcome == static_cast<uint8>(Outcome)) { ++N; }
		}
		return N;
	}

	/** Esito di una partita di prova con l'interposizione e una copertura bassa da una parte o dall'altra. */
	struct FItcCoverOutcome
	{
		int32 SaverDamage = -1;    // danno incassato da CHI intercetta
		int32 VictimDamage = -1;   // danno incassato dal bersaglio originale (deve restare 0)
		int32 Activations = 0;     // reazioni attivate nel turno
		int32 LoggedOnSaver = -1;  // danno che il TurnLog registra sulla cella dell'intercettore
	};

	/**
	 * Attaccante (4,0) team 1 spara alla vittima (0,0) team 0; l'intercettore (1,0) team 0 si interpone.
	 * La copertura bassa si mette sul bordo E — quello attraversato dal colpo in arrivo — della vittima,
	 * dell'intercettore, o di nessuno dei due: e' l'unica differenza fra le tre partite.
	 */
	FItcCoverOutcome RunItcCoverScene(bool bCoverOnVictim, bool bCoverOnSaver,
		ERTHexDirection SaverFacing = ERTHexDirection::E)
	{
		FItcCoverOutcome Out;
		UWorld* World = MakeItcWorld();
		if (!World) { return Out; }

		ARTHexMapActor* MapActor = SpawnItcMap(World);
		if (bCoverOnVictim) { AddItcLowCover(MapActor, FRTCellId(0, 0), ERTHexDirection::E); }
		if (bCoverOnSaver) { AddItcLowCover(MapActor, FRTCellId(1, 0), ERTHexDirection::E); }

		ARTUnit* Attacker = SpawnItcUnit(World, 1, FRTCellId(4, 0));
		ARTUnit* Victim = SpawnItcUnit(World, 0, FRTCellId(0, 0));
		ARTUnit* Saver = SpawnItcUnit(World, 0, FRTCellId(1, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Attacker || !Victim || !Saver || !TM)
		{
			DestroyItcWorld(World);
			return Out;
		}

		Saver->PlannedReactionAbility = AddItcAbility(Saver, TEXT("Action.Intercept"));
		Saver->PlannedAbilityIndex = INDEX_NONE;
		Saver->Facing = SaverFacing; // CP 16.2: da che lato e' scoperto CHI incassa

		const int32 VictimHealth = Victim->Health;
		const int32 SaverHealth = Saver->Health;
		Attacker->PlannedAbilityIndex = 0; // Tiro: 25, colpo singolo
		Attacker->PlannedAttackTarget = Victim;

		RunItcTurn(TM);

		Out.SaverDamage = SaverHealth - Saver->Health;
		Out.VictimDamage = VictimHealth - Victim->Health;
		Out.Activations = CountInterceptOutcome(TM, ERTReactionOutcome::Activated);
		Out.LoggedOnSaver = LoggedCombatDamageOn(TM, FRTCellId(1, 0));

		DestroyItcWorld(World);
		return Out;
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

	TestEqual(TEXT("l'interposizione si e' attivata"), CountInterceptOutcome(TM, ERTReactionOutcome::Activated), 1);
	TestEqual(TEXT("l'alleato protetto non incassa nulla"), Victim->Health, VictimHealth);
	TestEqual(TEXT("l'intercettore incassa al posto suo"), SaverHealth - Saver->Health, ItcFullHit());

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
		CountInterceptOutcome(TM, ERTReactionOutcome::Activated), 1);
	TestEqual(TEXT("il Counter del protetto risulta non innescato"),
		CountInterceptOutcome(TM, ERTReactionOutcome::NotTriggered), 1);

	DestroyItcWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInterceptRecalculatesCoverTest,
	"RefactorTactics.Cover.InterceptRecalculatesOnEffectiveTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInterceptRecalculatesCoverTest::RunTest(const FString&)
{
	// D-017: la geometria TARGET-DEPENDENT si rivaluta su chi il colpo lo incassa davvero. La copertura e'
	// una proprieta' del bordo davanti al BERSAGLIO: conservare quella del bersaglio originale farebbe
	// comparire nel combat log una protezione che davanti all'intercettore non esiste — o sparire quella che
	// c'e'. Le tre partite sono identiche tranne che per dove sta il muretto, quindi il -10 puo' venire solo
	// da li'.
	const int32 Reduction = URTCombatLibrary::LowCoverDamageReduction;

	const FItcCoverOutcome Bare = RunItcCoverScene(/*bCoverOnVictim=*/ false, /*bCoverOnSaver=*/ false);
	TestEqual(TEXT("controllo: senza coperture l'intercettore incassa il colpo pieno"), Bare.SaverDamage, ItcFullHit());
	TestEqual(TEXT("controllo: l'interposizione si e' attivata"), Bare.Activations, 1);

	// La vittima era riparata, l'intercettore no: il colpo che arriva a LUI e' pieno. E' il caso che il
	// comportamento precedente sbagliava, riportando i 15 calcolati davanti alla vittima.
	const FItcCoverOutcome CoveredVictim = RunItcCoverScene(/*bCoverOnVictim=*/ true, /*bCoverOnSaver=*/ false);
	TestEqual(TEXT("l'interposizione si e' attivata"), CoveredVictim.Activations, 1);
	TestEqual(TEXT("la copertura del bersaglio ORIGINALE non protegge chi si interpone"),
		CoveredVictim.SaverDamage, ItcFullHit());
	TestEqual(TEXT("il bersaglio protetto non incassa nulla"), CoveredVictim.VictimDamage, 0);

	// Caso simmetrico, ed e' quello che rende il test discriminante: senza di lui passerebbe anche una
	// rivalidazione che si limitasse a ignorare sempre la copertura.
	const FItcCoverOutcome CoveredSaver = RunItcCoverScene(/*bCoverOnVictim=*/ false, /*bCoverOnSaver=*/ true);
	TestEqual(TEXT("l'interposizione si e' attivata"), CoveredSaver.Activations, 1);
	TestEqual(TEXT("la copertura di CHI intercetta lo protegge davvero"),
		CoveredSaver.SaverDamage, ItcFullHit() - Reduction);
	TestEqual(TEXT("il bersaglio protetto non incassa nulla"), CoveredSaver.VictimDamage, 0);

	// Il TurnLog deve raccontare il contesto difensivo EFFETTIVAMENTE usato: il danno registrato sulla cella
	// dell'intercettore e' quello rivalidato, non quello calcolato davanti alla vittima.
	TestEqual(TEXT("il TurnLog registra sull'intercettore il danno pieno"), CoveredVictim.LoggedOnSaver, ItcFullHit());
	TestEqual(TEXT("e quello ridotto quando e' lui a essere riparato"),
		CoveredSaver.LoggedOnSaver, ItcFullHit() - Reduction);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInterceptRevalidatesFacingTest,
	"RefactorTactics.Cover.InterceptRevalidatesFacingOnEffectiveTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInterceptRevalidatesFacingTest::RunTest(const FString&)
{
	// La geometria target-dependent di D-017 include l'ORIENTAMENTO da quando CP 16.2 esiste: l'emisfero
	// posteriore e' scoperto, e un colpo fuori dall'arco frontale annulla la riduzione da copertura. Chi si
	// interpone ha il proprio facing, non quello di chi stava per essere colpito — quindi la rivalidazione
	// deve rileggere anche quello, o l'intercettore si porta dietro un riparo che il suo orientamento non
	// gli concede.
	const int32 Reduction = URTCombatLibrary::LowCoverDamageReduction;

	// Riparato dal lato giusto E rivolto verso l'attaccante: il muretto vale.
	const FItcCoverOutcome Facing = RunItcCoverScene(/*bCoverOnVictim=*/ false, /*bCoverOnSaver=*/ true,
		ERTHexDirection::E);
	TestEqual(TEXT("controllo: l'interposizione si e' attivata"), Facing.Activations, 1);
	TestEqual(TEXT("frontale: la copertura di chi intercetta vale"), Facing.SaverDamage, ItcFullHit() - Reduction);

	// Stesso muretto, stessa cella, stesso colpo: cambia solo da che parte guarda chi incassa. L'attaccante
	// e' a est, lui guarda a ovest — il colpo arriva alle spalle e la copertura non lo ripara.
	const FItcCoverOutcome Rear = RunItcCoverScene(/*bCoverOnVictim=*/ false, /*bCoverOnSaver=*/ true,
		ERTHexDirection::W);
	TestEqual(TEXT("l'interposizione si e' attivata lo stesso"), Rear.Activations, 1);
	TestEqual(TEXT("alle spalle: la copertura non ripara chi intercetta"), Rear.SaverDamage, ItcFullHit());
	TestEqual(TEXT("e il TurnLog registra il danno pieno"), Rear.LoggedOnSaver, ItcFullHit());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInterceptNoSecondOpportunityTest,
	"RefactorTactics.Cover.InterceptDoesNotOpenSecondOpportunity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInterceptNoSecondOpportunityTest::RunTest(const FString&)
{
	// D-017: rivalidare la geometria sul bersaglio effettivo NON apre una seconda finestra di reazione. Il
	// terzo alleato e' a 3 celle dalla vittima (fuori dalle 2 del catalogo) ma a 2 dall'intercettore: se il
	// colpo redirezionato riaprisse un'opportunita', lui potrebbe intercettarlo a sua volta — una catena.
	UWorld* World = MakeItcWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnItcMap(World);

	ARTUnit* Attacker = SpawnItcUnit(World, 1, FRTCellId(5, 0));
	ARTUnit* Victim = SpawnItcUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Saver = SpawnItcUnit(World, 0, FRTCellId(1, 0));  // a 1 dalla vittima: puo' intercettare
	ARTUnit* Third = SpawnItcUnit(World, 0, FRTCellId(3, 0));  // a 3 dalla vittima, a 2 dall'intercettore
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Attacker"), Attacker) || !TestNotNull(TEXT("Victim"), Victim)
		|| !TestNotNull(TEXT("Saver"), Saver) || !TestNotNull(TEXT("Third"), Third)
		|| !TestNotNull(TEXT("TM"), TM))
	{
		DestroyItcWorld(World);
		return false;
	}

	Saver->PlannedReactionAbility = AddItcAbility(Saver, TEXT("Action.Intercept"));
	Saver->PlannedAbilityIndex = INDEX_NONE;
	const int32 ThirdReactionIdx = AddItcAbility(Third, TEXT("Action.Intercept"));
	Third->PlannedReactionAbility = ThirdReactionIdx;
	Third->PlannedAbilityIndex = INDEX_NONE;

	const int32 VictimHealth = Victim->Health;
	const int32 SaverHealth = Saver->Health;
	const int32 ThirdHealth = Third->Health;
	Attacker->PlannedAbilityIndex = 0; // Tiro: 25, colpo singolo
	Attacker->PlannedAttackTarget = Victim;

	RunItcTurn(TM);

	TestEqual(TEXT("una sola interposizione, non una catena"),
		CountInterceptOutcome(TM, ERTReactionOutcome::Activated), 1);
	TestEqual(TEXT("il terzo alleato non aveva un colpo da intercettare"),
		CountInterceptOutcome(TM, ERTReactionOutcome::NotTriggered), 1);
	TestEqual(TEXT("incassa chi si e' interposto"), SaverHealth - Saver->Health, ItcFullHit());
	TestEqual(TEXT("il terzo alleato non incassa nulla"), ThirdHealth - Third->Health, 0);
	TestEqual(TEXT("il bersaglio originale non incassa nulla"), VictimHealth - Victim->Health, 0);
	TestTrue(TEXT("la reazione del terzo alleato resta disponibile: non l'ha spesa"),
		Third->CanUseAbility(ThirdReactionIdx));

	DestroyItcWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
