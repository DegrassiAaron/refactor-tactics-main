// `Status.Unbalanced` e `Status.Prone` — la catena che [D-319] apre e che `#2253` implementa.
//
// Il modello sta in `docs/gameplay/brief-stati-unbalanced-prone.md`. Qui si misura, e la domanda che ogni
// test deve poter far cadere e' scritta sopra di lui.
//
// 🔑 **Perche' un file proprio e non una coda di `RTStatusTests.cpp`.** Quel file misura il CONTRATTO degli
// stati temporanei (durata, scadenza nel Cleanup, sentinella di cella) ed e' gia' lungo; questi misurano una
// catena di gioco che attraversa Dash, Blast e Move piu' il bot. Sono due soggetti, e tenerli separati e'
// anche cio' che permette a `-Filter RefactorTactics.Status.Unbalanced` di girare da solo.
//
// ⚠️ **L'invariante piu' importante e' la prima**: `Status.Unbalanced` c'e' **se e solo se** il TurnLog di
// quel movimento dice `Slid`. Non «se il percorso e' stato allungato» — quella e' l'intenzione, e fra
// l'intenzione e la fine del Move ci sono microstep, Overwatch e predizioni che possono fermare l'unita'.

#include "Misc/AutomationTest.h"

#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Bot/RTHexBotLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTActionFallbackLibrary.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTReactionOpportunityTypes.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTTurnManager.h"
#include "UI/RTIconCatalogData.h"
#include "UI/RTIconLibrary.h"
#include "Unit/RTUnit.h"

#include "RTAbilityFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	UWorld* MakeFallWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyFallWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Arena piatta, con una sola cella di superficie scelta (usata per il ghiaccio). */
	ARTHexMapActor* SpawnFallMap(UWorld* World, int32 Radius, const FRTCellId& Special = FRTCellId(),
		ERTHexSurface Surface = ERTHexSurface::Floor)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		if (Surface != ERTHexSurface::Floor)
		{
			FRTHexCellData Cell(Special);
			Cell.Surface = Surface;
			M->AddOrUpdateCell(Cell);
			M->SortCells();
		}

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return Actor;
	}

	ARTUnit* SpawnFallUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell, const URTHeroData* Hero = nullptr)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // i piani li scriviamo noi
		U->ConfigureFromHeroData(Hero ? Hero : URTHeroCatalogLibrary::MakeWraith());
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		// `FRTCellId()` di default e' `(0,0,0)`, che e' una cella VERA: senza questa riga ogni unita' senza
		// piano pianifica un movimento verso l'origine, con traversate spurie e voci che nessun test ha
		// chiesto. Stessa neutralizzazione di `RTControlActionTests.cpp`.
		U->PlannedCell = Cell;
		U->Shield = 0;
		return U;
	}

	void StandStill(ARTUnit* Unit)
	{
		if (!Unit) { return; }
		Unit->PlannedAbilityIndex = INDEX_NONE;
		Unit->PlannedDashAbility = INDEX_NONE;
		Unit->PlannedPath.Reset();
		Unit->PlannedCell = Unit->Cell;
	}

	void RunFallTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/** L'esito della voce `Move` di questa unita' in questo turno, o `MAX_uint8` se non ce n'e' nessuna. */
	uint8 MoveOutcomeOf(const ARTTurnManager* TM, const ARTUnit* Unit)
	{
		if (!TM || !Unit) { return MAX_uint8; }
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Move && E.UnitId == Unit->StableUnitId)
			{
				return E.Outcome;
			}
		}
		return MAX_uint8;
	}

	/** Quante voci `Status` con questo tag e questo esito porta il TurnLog corrente. */
	int32 CountStatusEntries(const ARTTurnManager* TM, const FGameplayTag& Tag, ERTStatusOutcome Outcome)
	{
		int32 N = 0;
		if (!TM) { return N; }
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Status && E.ActionId == Tag.GetTagName()
				&& E.Outcome == static_cast<uint8>(Outcome))
			{
				++N;
			}
		}
		return N;
	}

	/** Installa un'azione del catalogo core e la pianifica contro `Target`. Torna l'indice, o INDEX_NONE. */
	int32 PlanCoreAttack(ARTUnit* Attacker, const TCHAR* ActionId, ARTUnit* Target)
	{
		const int32 Idx = RTAbilityFixtures::AddCoreAbility(Attacker, ActionId);
		if (Idx == INDEX_NONE) { return INDEX_NONE; }
		Attacker->PlannedAbilityIndex = Idx;
		Attacker->PlannedAttackTarget = Target;
		Attacker->PlannedCell = Attacker->Cell;
		return Idx;
	}
}

// =========================================================================================================
// 1. L'INVARIANTE: `Unbalanced` c'e' se e solo se il TurnLog dice `Slid`
// =========================================================================================================

/**
 * Chi finisce il Move sul ghiaccio con budget residuo scivola, e riceve `Status.Unbalanced`.
 *
 * 🔑 **Le due asserzioni sono legate di proposito**: se un giorno lo stato si applicasse su un'altra
 * condizione, il TurnLog e l'unita' racconterebbero due storie diverse dello stesso turno — ed e' il difetto
 * che il prerequisito `#2258` esiste per non produrre.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnbalancedOnSlideTest,
	"RefactorTactics.Status.UnbalancedIffSlid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnbalancedOnSlideTest::RunTest(const FString&)
{
	UWorld* World = MakeFallWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnFallMap(World, /*Radius=*/ 5, FRTCellId(1, 0, 0), ERTHexSurface::Ice);

	ARTUnit* Mover = SpawnFallUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Foe = SpawnFallUnit(World, 1, FRTCellId(5, 0)); // fuori portata: nessun colpo a disturbare
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover || !Foe) { DestroyFallWorld(World); return false; }

	Mover->PlannedAbilityIndex = INDEX_NONE;
	Mover->PlannedPath = { FRTCellId(0, 0), FRTCellId(1, 0) };
	Mover->PlannedCell = FRTCellId(1, 0);
	StandStill(Foe);

	RunFallTurn(TM);

	// Premessa: senza lo scivolamento le due asserzioni sotto sarebbero vere a vuoto.
	TestEqual(TEXT("premessa: e' arrivato una cella OLTRE la destinazione pianificata"),
		Mover->Cell, FRTCellId(2, 0));
	TestEqual(TEXT("il TurnLog dice Slid"), MoveOutcomeOf(TM, Mover),
		static_cast<uint8>(ERTMoveOutcome::Slid));
	TestTrue(TEXT("e l'unita' e' sbilanciata"), Mover->HasStatus(TAG_Status_Unbalanced));
	TestEqual(TEXT("la nascita e' attribuita al TERRENO, non a un'azione"),
		CountStatusEntries(TM, TAG_Status_Unbalanced, ERTStatusOutcome::AppliedByTerrain), 1);

	DestroyFallWorld(World);
	return true;
}

/**
 * L'ALTRA META', e senza di lei la prima non prova niente: chi termina sul ghiaccio **senza** budget non
 * scivola, il log dice `Moved`, e nessuno stato viene applicato.
 *
 * ⚠️ Il controllo e' sulla stessa cella di ghiaccio e sullo stesso percorso: l'unica differenza e' il
 * budget. Un test che cambiasse anche il terreno misurerebbe due cose insieme.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNoUnbalancedWithoutSlideTest,
	"RefactorTactics.Status.UnbalancedNotAppliedWithoutSlide",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNoUnbalancedWithoutSlideTest::RunTest(const FString&)
{
	UWorld* World = MakeFallWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnFallMap(World, /*Radius=*/ 5, FRTCellId(1, 0, 0), ERTHexSurface::Ice);

	ARTUnit* Mover = SpawnFallUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Foe = SpawnFallUnit(World, 1, FRTCellId(5, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover || !Foe) { DestroyFallWorld(World); return false; }

	// Budget esatto per il passo, zero residuo: la condizione dello scivolamento e' «residuo >= 2».
	Mover->MoveRange = 1;
	Mover->PlannedAbilityIndex = INDEX_NONE;
	Mover->PlannedPath = { FRTCellId(0, 0), FRTCellId(1, 0) };
	Mover->PlannedCell = FRTCellId(1, 0);
	StandStill(Foe);

	RunFallTurn(TM);

	TestEqual(TEXT("premessa: e' arrivato dove aveva chiesto, sul ghiaccio"), Mover->Cell, FRTCellId(1, 0));
	TestEqual(TEXT("il TurnLog dice Moved"), MoveOutcomeOf(TM, Mover),
		static_cast<uint8>(ERTMoveOutcome::Moved));
	TestFalse(TEXT("nessuno stato applicato"), Mover->HasStatus(TAG_Status_Unbalanced));

	DestroyFallWorld(World);
	return true;
}

// =========================================================================================================
// 2. L'AMPLIFICAZIONE, e la caduta
// =========================================================================================================

/**
 * Una spinta di UNA cella su un bersaglio `Unbalanced` ne sposta DUE, e il bersaglio finisce `Prone` con
 * `Unbalanced` consumato.
 *
 * ⚠️ **Tre asserzioni e non una**: la distanza prova l'amplificazione, `Prone` prova la caduta, e
 * l'assenza di `Unbalanced` prova il tetto anti-catena — chi si rialza non e' riabbattibile subito.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPushOnUnbalancedFallsTest,
	"RefactorTactics.Status.PushOnUnbalancedFalls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPushOnUnbalancedFallsTest::RunTest(const FString&)
{
	UWorld* World = MakeFallWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnFallMap(World, /*Radius=*/ 6);

	ARTUnit* Pusher = SpawnFallUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Victim = SpawnFallUnit(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Pusher || !Victim) { DestroyFallWorld(World); return false; }

	Victim->ApplyStatus(TAG_Status_Unbalanced, URTCombatLibrary::UnbalancedDurationTurns);
	StandStill(Victim);

	if (PlanCoreAttack(Pusher, TEXT("Action.Push"), Victim) == INDEX_NONE)
	{
		AddError(TEXT("`Action.Push` non e' nel catalogo core: la premessa del test non regge"));
		DestroyFallWorld(World);
		return false;
	}

	RunFallTurn(TM);

	// `Action.Push` spinge di 1; `Unbalanced` aggiunge la seconda cella. Da (1,0) lungo +q: (3,0).
	TestEqual(TEXT("spinto di DUE celle, non una"), Victim->Cell, FRTCellId(3, 0));
	TestTrue(TEXT("e' a terra"), Victim->HasStatus(TAG_Status_Prone));
	TestFalse(TEXT("`Unbalanced` e' stato consumato"), Victim->HasStatus(TAG_Status_Unbalanced));
	TestEqual(TEXT("il consumo e' registrato come `Spent`, non come scadenza"),
		CountStatusEntries(TM, TAG_Status_Unbalanced, ERTStatusOutcome::Spent), 1);
	TestEqual(TEXT("la caduta e' attribuita a un'AZIONE: c'e' una sorgente"),
		CountStatusEntries(TM, TAG_Status_Prone, ERTStatusOutcome::AppliedByAction), 1);

	DestroyFallWorld(World);
	return true;
}

/**
 * ⛔ **Niente movimento, niente caduta.** Una spinta che non sposta — qui perche' dietro il bersaglio c'e'
 * il bordo della mappa — non produce `Prone`, e `Unbalanced` **resta**: non e' stato consumato da nulla.
 *
 * E' la domanda §8.2 del brief, chiusa da [D-319], e il caso che distingue «l'ho spinto» da «l'ho fatto
 * cadere».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTResistedPushDoesNotFallTest,
	"RefactorTactics.Status.ResistedPushDoesNotFall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTResistedPushDoesNotFallTest::RunTest(const FString&)
{
	UWorld* World = MakeFallWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnFallMap(World, /*Radius=*/ 2); // raggio piccolo: dietro il bersaglio non c'e' mappa

	ARTUnit* Pusher = SpawnFallUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Victim = SpawnFallUnit(World, 1, FRTCellId(2, 0)); // sul bordo
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Pusher || !Victim) { DestroyFallWorld(World); return false; }

	Victim->ApplyStatus(TAG_Status_Unbalanced, URTCombatLibrary::UnbalancedDurationTurns);
	StandStill(Victim);

	if (PlanCoreAttack(Pusher, TEXT("Action.Push"), Victim) == INDEX_NONE)
	{
		AddError(TEXT("`Action.Push` non e' nel catalogo core: la premessa del test non regge"));
		DestroyFallWorld(World);
		return false;
	}

	RunFallTurn(TM);

	TestEqual(TEXT("premessa: non si e' mosso"), Victim->Cell, FRTCellId(2, 0));
	TestFalse(TEXT("nessuna caduta"), Victim->HasStatus(TAG_Status_Prone));
	TestTrue(TEXT("`Unbalanced` non e' stato consumato"), Victim->HasStatus(TAG_Status_Unbalanced));

	DestroyFallWorld(World);
	return true;
}

/**
 * `Status.Guarded` diventa inerte sullo SPOSTAMENTO e resta intatto sul DANNO.
 *
 * 🔑 **E' la decisione di perimetro presa in `#2253`, e questo test e' la sua sede.** L'argomento di design
 * — *«chi e' sbilanciato non puo' piantarsi per non cadere»* — parla di equilibrio; il pool da 15 di
 * [D-292] e' protezione, e cancellarlo sarebbe un cambio di bilanciamento che nessuno ha misurato.
 * Se un giorno si decidesse di cancellarlo, questo test **deve** cadere: e' cio' che lo rende una
 * decisione invece che un dettaglio d'implementazione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnbalancedIgnoresGuardOnDisplacementOnlyTest,
	"RefactorTactics.Status.UnbalancedIgnoresGuardOnDisplacementOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnbalancedIgnoresGuardOnDisplacementOnlyTest::RunTest(const FString&)
{
	UWorld* World = MakeFallWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnFallMap(World, /*Radius=*/ 6);

	ARTUnit* Pusher = SpawnFallUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Victim = SpawnFallUnit(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Pusher || !Victim) { DestroyFallWorld(World); return false; }

	Victim->ApplyStatus(TAG_Status_Guarded, 2);
	Victim->ApplyStatus(TAG_Status_Unbalanced, URTCombatLibrary::UnbalancedDurationTurns);
	StandStill(Victim);
	const int32 HealthBefore = Victim->Health;

	if (PlanCoreAttack(Pusher, TEXT("Action.Push"), Victim) == INDEX_NONE)
	{
		AddError(TEXT("`Action.Push` non e' nel catalogo core: la premessa del test non regge"));
		DestroyFallWorld(World);
		return false;
	}

	RunFallTurn(TM);

	TestNotEqual(TEXT("la guardia NON regge la spinta: l'unita' si e' mossa"), Victim->Cell, FRTCellId(1, 0));
	TestTrue(TEXT("ed e' caduta"), Victim->HasStatus(TAG_Status_Prone));
	// L'altra meta': `Action.Push` del catalogo non porta danno, quindi il pool non ha niente da assorbire
	// e la sola cosa che si puo' asserire e' che la guardia non e' stata bruciata dalla spinta.
	TestTrue(TEXT("lo strato di DANNO della guardia e' ancora attivo"), Victim->HasStatus(TAG_Status_Guarded));
	TestEqual(TEXT("e nessun danno e' passato da una spinta che non ne porta"), Victim->Health, HealthBefore);

	DestroyFallWorld(World);
	return true;
}

// =========================================================================================================
// 3. `Prone`: il prezzo, la durata, la reazione
// =========================================================================================================

/**
 * `1 MP`: il budget di movimento di chi e' a terra vale uno in meno, e non scende sotto zero.
 *
 * 🔑 **Il sito e' `GetEffectiveMoveRange()`, ed e' l'unico**: da li' passano sia lo snapshot del Move sia la
 * validazione del piano, quindi chi pianifica e chi risolve vedono lo stesso prezzo senza che nessuno debba
 * tenerli d'accordo. E' anche cio' che rende VERA l'anti-ciclicita' del brief §6: la condizione dello
 * scivolamento legge `MoveBudget` dallo snapshot.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStandUpCostsOneMovePointTest,
	"RefactorTactics.Status.StandUpCostsOneMovePoint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStandUpCostsOneMovePointTest::RunTest(const FString&)
{
	UWorld* World = MakeFallWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTUnit* U = SpawnFallUnit(World, 0, FRTCellId(0, 0));
	if (!U) { DestroyFallWorld(World); return false; }

	const int32 Base = U->GetEffectiveMoveRange();
	TestTrue(TEXT("premessa: il budget base e' positivo"), Base > 0);

	U->ApplyStatus(TAG_Status_Prone, URTCombatLibrary::ProneDurationTurns);
	TestEqual(TEXT("a terra si paga un punto"), U->GetEffectiveMoveRange(),
		Base - URTCombatLibrary::StandUpMovePointCost);

	// Il bordo: chi ha un punto solo si rialza e basta, e non finisce a `-1`. Un budget negativo
	// attraverserebbe il pathfinding senza far rumore.
	U->MoveRange = 1;
	TestEqual(TEXT("con un solo punto: zero, non meno di zero"), U->GetEffectiveMoveRange(), 0);

	// `Root` azzera gia': il prezzo non lo rende negativo, e la durata solleva comunque — e' la ragione per
	// cui [D-319] sceglie «durata piu' rimozione anticipata» invece di uno stato che finisce solo se paghi.
	U->MoveRange = 5;
	U->ApplyStatus(TAG_Status_Root, 2);
	TestEqual(TEXT("radicato e a terra: zero"), U->GetEffectiveMoveRange(), 0);

	DestroyFallWorld(World);
	return true;
}

/**
 * Chi si muove PAGA e si rialza; chi resta fermo no. E' l'altra meta' della regola, quella che rende
 * `Prone` un costo invece che una tassa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStandUpOnlyWhenMovingTest,
	"RefactorTactics.Status.StandUpOnlyWhenMoving",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStandUpOnlyWhenMovingTest::RunTest(const FString&)
{
	UWorld* World = MakeFallWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnFallMap(World, /*Radius=*/ 5);

	ARTUnit* Fermo = SpawnFallUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Mosso = SpawnFallUnit(World, 0, FRTCellId(0, 1));
	ARTUnit* Foe = SpawnFallUnit(World, 1, FRTCellId(5, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Fermo || !Mosso || !Foe) { DestroyFallWorld(World); return false; }

	Fermo->ApplyStatus(TAG_Status_Prone, URTCombatLibrary::ProneDurationTurns);
	Mosso->ApplyStatus(TAG_Status_Prone, URTCombatLibrary::ProneDurationTurns);
	StandStill(Fermo);
	StandStill(Foe);

	Mosso->PlannedAbilityIndex = INDEX_NONE;
	Mosso->PlannedPath = { FRTCellId(0, 1), FRTCellId(1, 1) };
	Mosso->PlannedCell = FRTCellId(1, 1);

	RunFallTurn(TM);

	TestEqual(TEXT("premessa: chi doveva muoversi si e' mosso"), Mosso->Cell, FRTCellId(1, 1));
	TestFalse(TEXT("chi si e' mosso ha pagato e si e' rialzato"), Mosso->HasStatus(TAG_Status_Prone));
	TestTrue(TEXT("chi e' rimasto fermo non ha pagato e resta a terra"), Fermo->HasStatus(TAG_Status_Prone));
	TestEqual(TEXT("una sola voce `ShakenOff`, con il prezzo dentro"),
		CountStatusEntries(TM, TAG_Status_Prone, ERTStatusOutcome::ShakenOff), 1);

	DestroyFallWorld(World);
	return true;
}

/**
 * `Prone` blocca la reazione, e la blocca per il TURNO INTERO in cui si e' gia' a terra — non solo per la
 * coda di quello in cui si e' caduti.
 *
 * ⚠️ La misura passa da `ReactionBlockedThisTurn`, cioe' dallo stesso meccanismo con cui `Action.Sprint`
 * nega la reazione (CP 5.1): l'esito osservabile e' `ERTReactionOutcome::Unavailable` sulla voce di
 * reazione, che e' cio' che un replay legge.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTProneHasNoReactionTest,
	"RefactorTactics.Status.ProneHasNoReaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTProneHasNoReactionTest::RunTest(const FString&)
{
	UWorld* World = MakeFallWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnFallMap(World, /*Radius=*/ 5);

	ARTUnit* Attacker = SpawnFallUnit(World, 0, FRTCellId(0, 0), URTHeroCatalogLibrary::MakeRiktor());
	ARTUnit* Defender = SpawnFallUnit(World, 1, FRTCellId(1, 0), URTHeroCatalogLibrary::MakeRiktor());
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attacker || !Defender) { DestroyFallWorld(World); return false; }

	const int32 ReactionIdx = RTAbilityFixtures::AddCoreAbility(Defender, TEXT("Action.Deflect"));
	if (ReactionIdx == INDEX_NONE)
	{
		AddError(TEXT("`Action.Deflect` non e' nel catalogo core: la premessa del test non regge"));
		DestroyFallWorld(World);
		return false;
	}
	Defender->PlannedReactionAbility = ReactionIdx;
	Defender->ApplyStatus(TAG_Status_Prone, URTCombatLibrary::ProneDurationTurns);
	StandStill(Defender);

	Attacker->PlannedAbilityIndex = 0; // attacco base
	Attacker->PlannedAttackTarget = Defender;
	Attacker->PlannedCell = Attacker->Cell;

	RunFallTurn(TM);

	int32 Unavailable = 0;
	int32 Reazioni = 0;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category != ERTLogCategory::Reaction || E.UnitId != Defender->StableUnitId) { continue; }
		++Reazioni;
		if (E.Outcome == static_cast<uint8>(ERTReactionOutcome::Unavailable)) { ++Unavailable; }
	}

	// Anti-vacuita': se nessuna voce di reazione esistesse, «zero attivate» sarebbe vero su nulla.
	TestTrue(TEXT("premessa: una voce di reazione e' stata scritta"), Reazioni > 0);
	TestEqual(TEXT("e ogni voce dice `Unavailable`"), Unavailable, Reazioni);

	DestroyFallWorld(World);
	return true;
}

/**
 * Durata `2`: `Prone` applicato nel Blast del turno `N` sopravvive al Cleanup di `N` ed e' **efficace per
 * tutto** `N+1`, dove poi si spegne. E' il numero che rende vero *«chi non paga perde due occasioni di
 * muoversi, non una»*: con `1` sarebbe sparito nel Cleanup dello stesso turno in cui si cade.
 *
 * ⚠️ **Il MOMENTO in cui si guarda conta, e la prima stesura di questo test lo sbagliava.** Ogni
 * `RunFallTurn` include il proprio Cleanup, quindi «lo stato copre `N+1`» **non** si legge chiedendo
 * `HasStatus` DOPO aver girato `N+1`: li' e' gia' stato decrementato a zero, e l'asserzione sarebbe
 * caduta su un comportamento CORRETTO. Si legge in due momenti — sopravvissuto al Cleanup di `N`, e
 * ancora **efficace** all'inizio di `N+1`, dove il budget di movimento vale un punto in meno.
 *
 * ⚠️ L'unita' resta ferma dopo la caduta: se si muovesse, pagherebbe e si rialzerebbe (l'altro test).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTProneSurvivesToNextTurnTest,
	"RefactorTactics.Status.ProneSurvivesToNextTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTProneSurvivesToNextTurnTest::RunTest(const FString&)
{
	UWorld* World = MakeFallWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnFallMap(World, /*Radius=*/ 6);

	ARTUnit* Pusher = SpawnFallUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Victim = SpawnFallUnit(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Pusher || !Victim) { DestroyFallWorld(World); return false; }

	Victim->ApplyStatus(TAG_Status_Unbalanced, URTCombatLibrary::UnbalancedDurationTurns);
	StandStill(Victim);
	if (PlanCoreAttack(Pusher, TEXT("Action.Push"), Victim) == INDEX_NONE)
	{
		AddError(TEXT("`Action.Push` non e' nel catalogo core: la premessa del test non regge"));
		DestroyFallWorld(World);
		return false;
	}

	RunFallTurn(TM); // turno N: cade
	if (!TestTrue(TEXT("premessa: e' caduto"), Victim->HasStatus(TAG_Status_Prone)))
	{
		DestroyFallWorld(World);
		return false;
	}

	// All'INIZIO del turno `N+1` lo stato e' ancora efficace, e la misura non e' `HasStatus` ma il suo
	// EFFETTO: il budget di movimento vale un punto in meno. E' cio' che «copre tutto `N+1`» significa
	// per chi gioca, ed e' anche il controllo che distingue uno stato vivo da un tag rimasto appeso.
	const int32 BudgetIntero = URTCombatLibrary::EffectiveMoveRange(
		Victim->MoveRange, Victim->HasStatus(TAG_Status_Root));
	TestEqual(TEXT("all'inizio del turno successivo il prezzo si paga ancora"),
		Victim->GetEffectiveMoveRange(), BudgetIntero - URTCombatLibrary::StandUpMovePointCost);

	StandStill(Pusher);
	StandStill(Victim);
	RunFallTurn(TM); // turno N+1: fermo, non paga -> il Cleanup di N+1 lo spegne
	TestFalse(TEXT("e dopo il Cleanup di quel turno lo stato e' scaduto"),
		Victim->HasStatus(TAG_Status_Prone));
	TestEqual(TEXT("il budget torna intero"), Victim->GetEffectiveMoveRange(), BudgetIntero);

	DestroyFallWorld(World);
	return true;
}


/**
 * `Prone` DISARMA l'Overwatch armato, e la charge e' **spesa**: non torna al proprietario.
 *
 * 🔑 **E' la linea di gioco che [D-319] costruisce**, ed e' la ragione per cui il blocco della reazione sta
 * su `Prone` e non su `Unbalanced`: spingere per disarmare costa un'azione all'avversario, mentre scivolare
 * non costa niente a nessuno (`brief` §4).
 *
 * ⚠️ **TRE casi in un solo montaggio, e non e' abbondanza.** Il guardiano che cade viene anche SPOSTATO, e
 * lo spostamento e' un secondo cambiamento fra i due casi: senza un terzo termine, uno zero danni non
 * distingue *«e' stato disarmato»* da *«la spinta lo ha portato dove non vede piu' niente»*. Il caso `A` —
 * nessuna spinta — fissa che l'Overwatch spari davvero in questo montaggio; il caso `B` — spinto, in piedi
 * — fissa che la spinta da sola non lo spenga; solo allora lo zero del caso `C` significa qualcosa.
 *
 * 🔴 **La geometria e' scelta perche' la spinta non porti il varco FUORI DAL CONO DI VISIONE, e la prima
 * stesura sbagliava proprio questo.** Metteva la sorgente a ovest: il guardiano veniva spinto a est, ma
 * `ApplyForcedDisplacement` lo GIRA verso chi l'ha spinto, cioe' a ovest — e il corridoio che sorvegliava
 * gli finiva alle spalle. Non sparava piu' **ne' da caduto ne' in piedi**, e i due casi erano
 * indistinguibili.
 *
 * ⚠️ **Il commento che stava qui diceva che il facing «non c'entra», ed era vero a meta'**: la ZONA e'
 * congelata all'armamento (`FRTArmedOverwatch::Facing`) e infatti non ruota, ma la PERCEZIONE segue il
 * facing vivo — `URTPerceptionLibrary::VisibleCells` e' un `HexCone` lungo `Facing`, piu' due celle di
 * consapevolezza ravvicinata. Zona ferma e occhi che ruotano: si finisce a sorvegliare un corridoio che
 * non si vede. E' un'interazione preesistente, indipendente da `Prone`, misurata qui e non decisa da
 * nessuno.
 *
 * ∴ La sorgente sta a EST, sulla stessa retta: la spinta allontana il guardiano dal varco **lasciandogli
 * il varco davanti**. Il varco e' la prima cella della linea in entrambe le posizioni post-spinta.
 *
 * ⛔ **La predictive armata NON e' coperta.** `Prone` la rimuove nello stesso punto e con la stessa riga, ma
 * armare `Hero.Wraith.InterceptShot` chiede un montaggio proprio: dichiarato come lacuna invece che
 * sottinteso come coperto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTProneDisarmsOverwatchTest,
	"RefactorTactics.Status.ProneDisarmsOverwatchAndSpendsCharge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTProneDisarmsOverwatchTest::RunTest(const FString&)
{
	// `bConSpinta` decide se il guardiano viene spinto; `bSbilanciato` se, essendolo, cade.
	// Torna il danno subito da chi attraversa il varco, e riempie `OutNota` con cio' che serve a leggerlo.
	auto DannoNelVarco = [this](bool bConSpinta, bool bSbilanciato, const FRTCellId& PostoDelGuardiano,
		FString& OutNota) -> int32
	{
		OutNota.Reset();

		UWorld* World = MakeFallWorld();
		if (!World) { OutNota = TEXT("world non creato"); return -1; }
		SpawnFallMap(World, /*Radius=*/ 7);

		// Il guardiano guarda a EST (facing di default) e la spinta lo allontana lungo lo STESSO asse: la
		// linea guardata trasla su se stessa invece di ruotare via dal varco.
		ARTUnit* Watcher = SpawnFallUnit(World, 0, PostoDelGuardiano);
		ARTUnit* Pusher = SpawnFallUnit(World, 1, FRTCellId(1, 0));
		ARTUnit* Mover = SpawnFallUnit(World, 1, FRTCellId(0, 1));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Watcher || !Pusher || !Mover) { DestroyFallWorld(World); OutNota = TEXT("spawn fallito"); return -1; }

		// Un decisore che spara a qualunque finestra glielo consenta: senza, un boundary aperto scadrebbe in
		// `Hold Ground` e il caso di controllo direbbe «non ha sparato» per la ragione sbagliata.
		TM->ReactionDecider.BindLambda(
			[](const FRTReactionOpportunity& Opportunity, int32) -> FString
			{
				for (const FString& Response : Opportunity.AllowedResponses)
				{
					if (URTReactionOpportunityLibrary::FireResponseTarget(Response) != INDEX_NONE)
					{
						return Response;
					}
				}
				return FString();
			});

		const int32 OverwatchIdx = RTAbilityFixtures::AddCoreAbilityInSlot(
			Watcher, TEXT("Action.Overwatch"), /*SlotIndex=*/ 3);
		if (OverwatchIdx == INDEX_NONE) { DestroyFallWorld(World); OutNota = TEXT("Action.Overwatch non installabile"); return -1; }
		Watcher->PlannedAbilityIndex = OverwatchIdx; // armare costa l'azione PRINCIPALE (catalogo §1)
		Watcher->PlannedCell = Watcher->Cell;

		if (bSbilanciato)
		{
			Watcher->ApplyStatus(TAG_Status_Unbalanced, URTCombatLibrary::UnbalancedDurationTurns);
		}

		if (bConSpinta)
		{
			if (PlanCoreAttack(Pusher, TEXT("Action.Push"), Watcher) == INDEX_NONE)
			{
				DestroyFallWorld(World); OutNota = TEXT("Action.Push non installabile"); return -1;
			}
		}
		else
		{
			StandStill(Pusher);
		}

		Mover->PlannedAbilityIndex = INDEX_NONE;
		Mover->PlannedPath = { FRTCellId(0, 1), FRTCellId(0, 0) };
		Mover->PlannedCell = FRTCellId(0, 0);

		const int32 SaluteIniziale = Mover->Health + Mover->Shield;
		RunFallTurn(TM);
		const int32 Danno = SaluteIniziale - (Mover->Health + Mover->Shield);

		// Quante finestre di reazione sono state aperte, e quante decise: senza, uno zero danni non dice se
		// il watcher non ha sparato o se non gli e' mai stato chiesto.
		int32 Opportunita = 0;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::ReactionDecision) { ++Opportunita; }
		}

		OutNota = FString::Printf(
			TEXT("watcher %s->(q=%d,r=%d) prone=%d | mover (q=%d,r=%d) | decisioni=%d | danno=%d"),
			bConSpinta ? TEXT("spinto") : TEXT("fermo"),
			Watcher->Cell.X, Watcher->Cell.Y, Watcher->HasStatus(TAG_Status_Prone) ? 1 : 0,
			Mover->Cell.X, Mover->Cell.Y, Opportunita, Danno);

		DestroyFallWorld(World);
		return Danno;
	};

	// I TRE casi si girano TUTTI prima di giudicare, e il referto li porta tutti.
	//
	// 🔑 **Uscire al primo che cade lascia mezza risposta.** La prima stesura si fermava sulla premessa
	// fallita, e il messaggio diceva solo quel caso: per capire se il difetto fosse nel montaggio o nella
	// meccanica servivano gli altri due, che non erano stati nemmeno eseguiti. Tre mondi costano
	// millisecondi; un run del motore in piu' costa una finestra contesa fra sessioni.
	// `A` nasce GIA' nella cella in cui `B` finisce spinto: e' il controllo che separa «lo spostamento
	// spegne l'Overwatch» da «da li' il varco non e' guardato». Il primo montaggio non lo aveva, e le due
	// diagnosi — opposte — erano indistinguibili.
	FString NotaA, NotaB, NotaC;
	const int32 DannoA = DannoNelVarco(/*bConSpinta=*/ false, /*bSbilanciato=*/ false, FRTCellId(-1, 0), NotaA);
	const int32 DannoB = DannoNelVarco(/*bConSpinta=*/ true,  /*bSbilanciato=*/ false, FRTCellId(0, 0),  NotaB);
	const int32 DannoC = DannoNelVarco(/*bConSpinta=*/ true,  /*bSbilanciato=*/ true,  FRTCellId(0, 0),  NotaC);

	const FString Referto = FString::Printf(TEXT("A[%s] · B[%s] · C[%s]"), *NotaA, *NotaB, *NotaC);

	// A — senza spinta: fissa che in QUESTO montaggio l'Overwatch spari davvero. Senza, gli zeri sotto
	// sarebbero veri anche in un mondo dove l'Overwatch non funziona affatto.
	if (!TestTrue(FString::Printf(TEXT("A) premessa: un guardiano fermo SPARA — %s"), *Referto), DannoA > 0))
	{
		return false;
	}

	// B — spinto ma in piedi: fissa che a spegnerlo non basta lo spostamento.
	if (!TestTrue(FString::Printf(TEXT("B) premessa: spinto ma in piedi SPARA ancora — %s"), *Referto), DannoB > 0))
	{
		return false;
	}

	// C — spinto e sbilanciato: cade, e non spara.
	//
	// ⚠️ **`<= 0` e non `== 0`, e il perche' e' una misura, non una tolleranza.** La grandezza e'
	// `Health + Shield` prima meno dopo, e lo **scudo base rientra a inizio turno** ([D-224]:
	// `URTCombatLibrary::BaseShield`). Con il colpo il conto e' `21 - 5 assorbiti = 16`; senza, lo scudo
	// sale da 0 a 5 e la differenza e' **negativa**. Un `== 0` chiederebbe che nessuna delle due cose
	// accada, che non e' l'invariante: l'invariante e' che **nessun danno passi**.
	//
	// La seconda riga tiene l'asserzione non vacua: senza, `DannoC <= 0` sarebbe vero anche in un mondo
	// dove l'Overwatch non spara mai — ed e' il caso `B` a escluderlo, non una premessa scritta a parte.
	TestTrue(FString::Printf(
		TEXT("C) chi e' a terra non spara: Overwatch disarmato e charge persa — %s"), *Referto), DannoC <= 0);
	TestTrue(FString::Printf(
		TEXT("C) e la differenza col guardiano in piedi e' il colpo intero — %s"), *Referto), DannoC < DannoB);

	return true;
}

// =========================================================================================================
// 4. `Sprint` negato, e il ri-scivolamento
// =========================================================================================================

/**
 * Chi e' `Unbalanced` non corre: lo `Sprint` viene RIFIUTATO, e il rifiuto lascia una voce che ne nomina la
 * causa — `ERTActionInvalidReason::Unbalanced` in `Amount`.
 *
 * 🔑 **Rifiuto dichiarato e non scarto muto** (`brief` §8.7). Un'azione che sparisce senza spiegazione e' il
 * difetto che l'intero enum dei reason code esiste per evitare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSprintRefusedWhileUnbalancedTest,
	"RefactorTactics.Status.SprintRefusedWhileUnbalanced",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSprintRefusedWhileUnbalancedTest::RunTest(const FString&)
{
	UWorld* World = MakeFallWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnFallMap(World, /*Radius=*/ 6);

	ARTUnit* Runner = SpawnFallUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Foe = SpawnFallUnit(World, 1, FRTCellId(6, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Runner || !Foe) { DestroyFallWorld(World); return false; }

	const int32 SprintIdx = RTAbilityFixtures::AddCoreAbility(Runner, TEXT("Action.Sprint"));
	if (SprintIdx == INDEX_NONE)
	{
		AddError(TEXT("`Action.Sprint` non e' nel catalogo core: la premessa del test non regge"));
		DestroyFallWorld(World);
		return false;
	}

	Runner->ApplyStatus(TAG_Status_Unbalanced, URTCombatLibrary::UnbalancedDurationTurns);
	Runner->PlannedAbilityIndex = INDEX_NONE;
	Runner->PlannedDashAbility = SprintIdx;
	Runner->PlannedDashCell = FRTCellId(3, 0);
	Runner->PlannedCell = Runner->Cell;
	StandStill(Foe);

	RunFallTurn(TM);

	TestEqual(TEXT("non ha corso: e' rimasto dov'era"), Runner->Cell, FRTCellId(0, 0));

	int32 Rifiuti = 0;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category == ERTLogCategory::Fallback && E.UnitId == Runner->StableUnitId
			&& E.Amount == static_cast<int32>(ERTActionInvalidReason::Unbalanced))
		{
			++Rifiuti;
		}
	}
	TestEqual(TEXT("e il rifiuto e' nel TurnLog, con la causa nominata"), Rifiuti, 1);

	DestroyFallWorld(World);
	return true;
}

/**
 * Chi scivola mentre e' gia' `Unbalanced` percorre DUE celle invece di una: `FRTTerrainDef::SlideCells`
 * smette di essere letto come un booleano.
 *
 * Test puro sulla libreria, come i quattro `Terrain.Ice.*` esistenti: il ri-scivolamento e' una proprieta'
 * dello strato esagonale, e `ExtraSlideCells` e' il modo in cui lo stato ci arriva senza che quello strato
 * conosca i Gameplay Tag.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIceSlidesTwoWhenUnbalancedTest,
	"RefactorTactics.Terrain.Ice.SlidesTwoWhenUnbalanced",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIceSlidesTwoWhenUnbalancedTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 4);
	FRTHexCellData IceCell(FRTCellId(1, 0, 0));
	IceCell.Surface = ERTHexSurface::Ice;
	Map->AddOrUpdateCell(IceCell);
	Map->SortCells();

	const TArray<FRTCellId> Path = { FRTCellId(0, 0, 0), FRTCellId(1, 0, 0) };

	// (a) senza lo stato: una cella, come prima. E' il controllo che rende non vacuo il caso (b).
	{
		FRTHexSimUnit Unit;
		Unit.UnitId = 0;
		Unit.Cell = FRTCellId(0, 0, 0);
		Unit.MoveBudget = 5;

		FRTHexSnapshot Snapshot;
		Snapshot.Map = Map;
		Snapshot.Units.Add(Unit);

		const TArray<FRTCellId> Extended = URTHexSimLibrary::ApplyIceSliding(Snapshot, /*UnitId=*/ 0, Path);
		TestEqual(TEXT("in piedi: si scivola di una cella"), Extended.Num(), 3);
		TestTrue(TEXT("e ci si ferma li'"), Extended.Last() == FRTCellId(2, 0, 0));
	}

	// (b) sbilanciato: due celle, nella STESSA direzione.
	{
		FRTHexSimUnit Unit;
		Unit.UnitId = 0;
		Unit.Cell = FRTCellId(0, 0, 0);
		Unit.MoveBudget = 5;
		Unit.ExtraSlideCells = URTCombatLibrary::UnbalancedExtraDisplacement;

		FRTHexSnapshot Snapshot;
		Snapshot.Map = Map;
		Snapshot.Units.Add(Unit);

		const TArray<FRTCellId> Extended = URTHexSimLibrary::ApplyIceSliding(Snapshot, /*UnitId=*/ 0, Path);
		TestEqual(TEXT("sbilanciato: si scivola di due celle"), Extended.Num(), 4);
		TestTrue(TEXT("in linea retta, senza curve"), Extended.Last() == FRTCellId(3, 0, 0));
	}

	// (c) l'estensione e' PARZIALE se la seconda cella non e' percorribile: la prima vale comunque.
	{
		FRTHexCellData Wall(FRTCellId(3, 0, 0));
		Wall.bBlocksMovement = true;
		Map->AddOrUpdateCell(Wall);
		Map->SortCells();

		FRTHexSimUnit Unit;
		Unit.UnitId = 0;
		Unit.Cell = FRTCellId(0, 0, 0);
		Unit.MoveBudget = 5;
		Unit.ExtraSlideCells = URTCombatLibrary::UnbalancedExtraDisplacement;

		FRTHexSnapshot Snapshot;
		Snapshot.Map = Map;
		Snapshot.Units.Add(Unit);

		const TArray<FRTCellId> Extended = URTHexSimLibrary::ApplyIceSliding(Snapshot, /*UnitId=*/ 0, Path);
		TestEqual(TEXT("un muro alla seconda cella non annulla la prima"), Extended.Num(), 3);
		TestTrue(TEXT("ci si ferma prima del muro"), Extended.Last() == FRTCellId(2, 0, 0));
	}

	// (d) `ExtraSlideCells` AMPLIFICA, non crea: fuori dal ghiaccio non si scivola comunque.
	{
		const TArray<FRTCellId> SuFloor = { FRTCellId(0, 0, 0), FRTCellId(0, 1, 0) };

		FRTHexSimUnit Unit;
		Unit.UnitId = 0;
		Unit.Cell = FRTCellId(0, 0, 0);
		Unit.MoveBudget = 5;
		Unit.ExtraSlideCells = URTCombatLibrary::UnbalancedExtraDisplacement;

		FRTHexSnapshot Snapshot;
		Snapshot.Map = Map;
		Snapshot.Units.Add(Unit);

		const TArray<FRTCellId> Extended = URTHexSimLibrary::ApplyIceSliding(Snapshot, /*UnitId=*/ 0, SuFloor);
		TestEqual(TEXT("su terreno normale non si scivola, sbilanciati o no"), Extended.Num(), 2);
	}

	return true;
}

// =========================================================================================================
// 5. Il bot capitalizza
// =========================================================================================================

/**
 * Il criterio di conformita' dichiarato da [D-319]: *«con un bersaglio `Unbalanced` a portata e un'azione
 * con `Push` disponibile, il bot la sceglie invece dell'attacco base A PARITA' di danno atteso»*.
 *
 * 🔑 **Si misura su `ScorePlan` con due candidate identiche in tutto tranne lo spostamento**, che e' l'unico
 * montaggio in cui «a parita' di danno atteso» e' una premessa e non una speranza: stessa cella, stesso
 * bersaglio, stesso danno.
 *
 * ⚠️ **Con il controllo negativo**: se il bersaglio NON e' sbilanciato i due punteggi coincidono. Senza,
 * un termine che desse sempre il bonus passerebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotPrefersPushOnUnbalancedTest,
	"RefactorTactics.HexBot.PrefersPushOnUnbalancedTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotPrefersPushOnUnbalancedTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 4);

	FRTHexBotContext Ctx;
	Ctx.Origin = FRTCellId(0, 0, 0);
	Ctx.Enemies.Add(FRTCellId(2, 0, 0));
	Ctx.EnemyRanges.Add(1);
	Ctx.EnemyHealth.Add(100); // molto sopra il danno: nessun `WKill` a coprire la differenza
	Ctx.EnemyFacings.Add(ERTHexDirection::E);

	FRTHexBotPlan Base;
	Base.DestCell = FRTCellId(1, 0, 0);
	Base.FromCell = FRTCellId(0, 0, 0);
	Base.bHasAttack = true;
	Base.TargetIndex = 0;
	Base.AttackDamage = 10;
	Base.TargetHealth = 100;
	Base.RangeCells = 2;

	FRTHexBotPlan ConSpinta = Base;
	ConSpinta.bAttackDisplaces = true;

	// (a) bersaglio SBILANCIATO: la spinta vale di piu'.
	Ctx.EnemyUnbalanced.Reset();
	Ctx.EnemyUnbalanced.Add(true);
	const int32 ScoreBase = URTHexBotLibrary::ScorePlan(Map, Base, Ctx);
	const int32 ScorePush = URTHexBotLibrary::ScorePlan(Map, ConSpinta, Ctx);
	TestTrue(TEXT("a parita' di danno, spingere un bersaglio sbilanciato vale di piu'"), ScorePush > ScoreBase);

	// (b) bersaglio IN PIEDI: nessuna differenza. E' il controllo che rende (a) una misura.
	Ctx.EnemyUnbalanced.Reset();
	Ctx.EnemyUnbalanced.Add(false);
	TestEqual(TEXT("su un bersaglio in piedi i due piani valgono uguale"),
		URTHexBotLibrary::ScorePlan(Map, ConSpinta, Ctx), URTHexBotLibrary::ScorePlan(Map, Base, Ctx));

	// (c) e la preferenza deve reggere anche a `ChooseBestPlan`, che e' cio' che il bot usa davvero.
	Ctx.EnemyUnbalanced.Reset();
	Ctx.EnemyUnbalanced.Add(true);
	const FRTHexBotPlan Scelto = URTHexBotLibrary::ChooseBestPlan(Map, { Base, ConSpinta }, Ctx);
	TestTrue(TEXT("`ChooseBestPlan` sceglie la candidata che sposta"), Scelto.bAttackDisplaces);

	return true;
}

// =========================================================================================================
// 6. Il gate che mancava: il catalogo icone REALE contro le chiavi richieste
// =========================================================================================================

/**
 * 🔴 **Questo gate non esisteva, ed e' il difetto che `#2253` ha trovato mentre ne cercava un altro.**
 *
 * La issue dichiarava che definire due tag `Status.` avrebbe **rotto** `RTIconCatalogTests`. Misurato:
 * falso. Quei sei test costruiscono il proprio catalogo con `MakeCoveringIconCatalog()`, che itera
 * `RequiredIconIds()` — la stessa funzione che i tag alimentano — quindi restano verdi **per costruzione**
 * qualunque tag si aggiunga.
 *
 * Il difetto vero e' l'opposto e piu' silenzioso: `URTIconLibrary::FindMissingRequiredIcons` esiste
 * apposta, e **nessun test lo chiamava sul catalogo che il gioco spedisce**. Due chiavi nuove sarebbero
 * cadute sul `MissingIcon` a schermo con la suite tutta verde.
 *
 * ⚠️ **Salta se il catalogo non e' caricabile**, e non fallisce: in un ambiente senza `Content/` montato
 * l'assenza dell'asset non e' un difetto di copertura. La riga di premessa dice quale dei due casi si e'
 * verificato, cosi' un verde per assenza si distingue da un verde per copertura.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRealIconCatalogCoversRequiredIdsTest,
	"RefactorTactics.IconCatalog.RealCatalogCoversRequiredIds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRealIconCatalogCoversRequiredIdsTest::RunTest(const FString&)
{
	const URTIconCatalogData* Catalog = LoadObject<URTIconCatalogData>(
		nullptr, TEXT("/Game/RT/UI/DA_IconCatalog.DA_IconCatalog"));

	if (Catalog == nullptr)
	{
		AddWarning(TEXT("`/Game/RT/UI/DA_IconCatalog` non caricabile: copertura NON misurata"));
		return true;
	}

	const TArray<FName> Missing = URTIconLibrary::FindMissingRequiredIcons(Catalog);
	if (Missing.Num() > 0)
	{
		AddError(FString::Printf(TEXT("%d chiave/i richiesta/e non coperta/e dal catalogo reale: %s"),
			Missing.Num(), *FString::JoinBy(Missing, TEXT(" | "),
				[](const FName& N) { return N.ToString(); })));
	}

	// Anti-vacuita': un catalogo vuoto darebbe «zero mancanze» solo se anche l'insieme richiesto fosse
	// vuoto, e questa riga lo esclude. E' lo stesso difetto che `FindMissingRequiredIcons` dichiara.
	TestTrue(TEXT("anti-vacuita': l'insieme richiesto non e' vuoto"),
		URTIconLibrary::RequiredIconIds().Num() > 0);

	// I due tag di questa issue sono richiesti: se un giorno sparissero, il gate sopra resterebbe verde
	// misurando un insieme piu' piccolo.
	TestTrue(TEXT("`UI.Icon.Status.Unbalanced` e' fra le chiavi richieste"),
		URTIconLibrary::RequiredIconIds().Contains(FName(TEXT("UI.Icon.Status.Unbalanced"))));
	TestTrue(TEXT("`UI.Icon.Status.Prone` e' fra le chiavi richieste"),
		URTIconLibrary::RequiredIconIds().Contains(FName(TEXT("UI.Icon.Status.Prone"))));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
