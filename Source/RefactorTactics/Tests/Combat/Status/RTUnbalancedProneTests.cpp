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
#include "Ability/RTMovementProfileLibrary.h"

#include "Ability/RTActionData.h"
#include "Ability/Catalog/RTCatalogLibrary.h"
#include "Ability/Catalog/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Bot/RTHexBotLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexCellData.h"
#include "Map/Actor/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Replay/RTReplayAuditLibrary.h"   // [D-366]: l'audit di [D-313] ricalcola il verdetto del disarmo per regola
#include "Replay/RTReplayPrivacyLibrary.h" // [D-366]: chi legge il disarmo, col filtro del replay per osservatore
#include "Turn/Actions/RTActionFallbackLibrary.h"
#include "Turn/Actions/RTActionQueueLibrary.h" // SortUnitsForResolution: l'ordine reale in cui il resolver passa le unita' (#2922)
#include "Turn/Resolution/RTHexSim.h"
#include "Turn/Resolution/RTHexSimLibrary.h"
#include "Turn/Format/RTMatchSetupLibrary.h"
#include "Turn/Reaction/RTReactionOpportunityTypes.h"
#include "Turn/Reaction/RTReactionLibrary.h" // ControlSeverityRank: la gravita' si CHIEDE, non si ricopia
#include "Turn/Log/RTTurnLog.h"
#include "Turn/Log/RTTurnLogLibrary.h"
#include "Turn/RTTurnManager.h"
#include "UI/Inspect/RTIconCatalogData.h"
#include "UI/Inspect/RTIconLibrary.h"
#include "Unit/RTUnit.h"

#include "Tests/Shared/RTAbilityFixtures.h"
#include "Tests/Shared/RTUnitPlanTestAccess.h"

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
		U->ConfigureFromHeroData(Hero ? Hero : URTHeroCatalogLibrary::MakeIvrin());
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		// `FRTCellId()` di default e' `(0,0,0)`, che e' una cella VERA: senza questa riga ogni unita' senza
		// piano pianifica un movimento verso l'origine, con traversate spurie e voci che nessun test ha
		// chiesto. Stessa neutralizzazione di `RTControlActionTests.cpp`.
		U->SetPlannedCell(Cell);
		U->Shield = 0;
		return U;
	}

	/**
	 * Azzera OGNI intento dell'unita': azione, scatto, percorso e destinazione.
	 *
	 * ⚠️ **Si chiamava `StandStill`, come l'omonima di `RTStatusTests.cpp`, e le due collidevano** —
	 * [#2397](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2397). Entrambe in anonymous
	 * namespace, il che e' legale per due unita' di traduzione separate; ma l'unity build le mette nello
	 * stesso blob, e il conflitto scattava appena il raggruppamento cambiava — cioe' appena qualcuno
	 * aggiungeva un file al progetto, in un punto qualsiasi.
	 *
	 * ⛔ **Non sono state unificate, ed e' deliberato**: questa azzera anche `PlannedDashAbility` e regge un
	 * puntatore nullo, l'altra no. Fonderle cambierebbe il comportamento di test che nessuno ha chiesto di
	 * cambiare — il nome era in comune, il contratto no. E' il nome a dover essere specifico.
	 */
	void NeutralizeAllIntents(ARTUnit* Unit)
	{
		if (!Unit) { return; }
		Unit->ClearPlannedMainAction();
		Unit->ClearPlannedDash();
		FRTUnitPlanTestAccess::ResetPath(*Unit);
		Unit->SetPlannedCell(Unit->Cell);
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
		Attacker->SetPlannedMainAction(Idx);
		Attacker->DeclareAttackOnUnit(Target);
		Attacker->SetPlannedCell(Attacker->Cell);
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

	Mover->ClearPlannedMainAction();
	FRTUnitPlanTestAccess::SetPath(*Mover, { FRTCellId(0, 0), FRTCellId(1, 0) });
	Mover->SetPlannedCell(FRTCellId(1, 0));
	NeutralizeAllIntents(Foe);

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
	Mover->ClearPlannedMainAction();
	FRTUnitPlanTestAccess::SetPath(*Mover, { FRTCellId(0, 0), FRTCellId(1, 0) });
	Mover->SetPlannedCell(FRTCellId(1, 0));
	NeutralizeAllIntents(Foe);

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
	NeutralizeAllIntents(Victim);

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
	NeutralizeAllIntents(Victim);

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
	NeutralizeAllIntents(Victim);
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
	NeutralizeAllIntents(Fermo);
	NeutralizeAllIntents(Foe);

	Mosso->ClearPlannedMainAction();
	FRTUnitPlanTestAccess::SetPath(*Mosso, { FRTCellId(0, 1), FRTCellId(1, 1) });
	Mosso->SetPlannedCell(FRTCellId(1, 1));

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

	ARTUnit* Attacker = SpawnFallUnit(World, 0, FRTCellId(0, 0), URTHeroCatalogLibrary::MakeBranth());
	ARTUnit* Defender = SpawnFallUnit(World, 1, FRTCellId(1, 0), URTHeroCatalogLibrary::MakeBranth());
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attacker || !Defender) { DestroyFallWorld(World); return false; }

	const int32 ReactionIdx = RTAbilityFixtures::AddCoreAbility(Defender, TEXT("Action.Deflect"));
	if (ReactionIdx == INDEX_NONE)
	{
		AddError(TEXT("`Action.Deflect` non e' nel catalogo core: la premessa del test non regge"));
		DestroyFallWorld(World);
		return false;
	}
	Defender->SetPlannedReaction(ReactionIdx);
	Defender->ApplyStatus(TAG_Status_Prone, URTCombatLibrary::ProneDurationTurns);
	NeutralizeAllIntents(Defender);

	Attacker->SetPlannedMainAction(0); // attacco base
	Attacker->DeclareAttackOnUnit(Defender);
	Attacker->SetPlannedCell(Attacker->Cell);

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
	NeutralizeAllIntents(Victim);
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

	NeutralizeAllIntents(Pusher);
	NeutralizeAllIntents(Victim);
	RunFallTurn(TM); // turno N+1: fermo, non paga -> il Cleanup di N+1 lo spegne
	TestFalse(TEXT("e dopo il Cleanup di quel turno lo stato e' scaduto"),
		Victim->HasStatus(TAG_Status_Prone));
	TestEqual(TEXT("il budget torna intero"), Victim->GetEffectiveMoveRange(), BudgetIntero);

	DestroyFallWorld(World);
	return true;
}


namespace
{
	/** Cio' che il banco del varco lascia dietro di se': il colpo, le finestre, e il TurnLog del turno. */
	struct FRTVarcoEsito
	{
		/** Salute + scudo del mover prima, meno dopo; -1 se il banco non si e' montato (la ragione e' in `Nota`). */
		int32 Danno = -1;
		/** Le finestre di reazione decise: senza, uno zero danni non dice se il watcher non ha sparato o se non gli e' mai stato chiesto. */
		int32 Decisioni = 0;
		/** Il TurnLog del turno, copiato prima che il mondo venga distrutto. */
		TArray<FRTTurnLogEntry> Log;
		FRTCellId WatcherCell;
		bool bWatcherProne = false;
		FString Nota;
	};

	/** La squadra del guardiano e quella di chi lo spinge, nel banco del varco. */
	constexpr int32 VarcoWatcherTeam = 0;
	constexpr int32 VarcoEnemyTeam = 1;

	/**
	 * IL BANCO DEL VARCO: un guardiano con l'Overwatch armato verso EST, un nemico che entra nella sua linea e, se
	 * `bConSpinta`, una spinta da EST che lo allontana lungo la stessa retta. `bSbilanciato` decide se, spinto, cade.
	 * `bSgambetto` lo fa cadere SENZA spostarlo: un'azione sintetica nel Blast gli applica `Prone`, da OVEST, dietro di
	 * lui e fuori dal corridoio sorvegliato.
	 *
	 * Lo usano `Status.ProneDisarmsOverwatchAndSpendsCharge` e `Overwatch.ForcedDisplacementDisarmsWithItsCause`
	 * ([D-366]). La geometria, e perche' e' quella, e' scritta sopra il primo dei due.
	 */
	FRTVarcoEsito GiocaIlVarco(bool bConSpinta, bool bSbilanciato, const FRTCellId& PostoDelGuardiano,
		bool bSgambetto = false)
	{
		FRTVarcoEsito Esito;

		UWorld* World = MakeFallWorld();
		if (!World) { Esito.Nota = TEXT("world non creato"); return Esito; }
		SpawnFallMap(World, /*Radius=*/ 7);

		// Il guardiano guarda a EST (facing di default) e la spinta lo allontana lungo lo STESSO asse: la linea guardata
		// trasla su se stessa invece di ruotare via dal varco.
		ARTUnit* Watcher = SpawnFallUnit(World, VarcoWatcherTeam, PostoDelGuardiano);
		ARTUnit* Pusher = SpawnFallUnit(World, VarcoEnemyTeam, FRTCellId(1, 0));
		ARTUnit* Mover = SpawnFallUnit(World, VarcoEnemyTeam, FRTCellId(0, 1));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Watcher || !Pusher || !Mover) { DestroyFallWorld(World); Esito.Nota = TEXT("spawn fallito"); return Esito; }

		// Un decisore che spara a qualunque finestra glielo consenta: senza, un boundary aperto scadrebbe in `Hold Ground`
		// e il caso di controllo direbbe «non ha sparato» per la ragione sbagliata.
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
		if (OverwatchIdx == INDEX_NONE) { DestroyFallWorld(World); Esito.Nota = TEXT("Action.Overwatch non installabile"); return Esito; }
		Watcher->SetPlannedMainAction(OverwatchIdx); // armare costa l'azione PRINCIPALE (catalogo §1)
		Watcher->SetPlannedCell(Watcher->Cell);

		if (bSbilanciato)
		{
			Watcher->ApplyStatus(TAG_Status_Unbalanced, URTCombatLibrary::UnbalancedDurationTurns);
		}

		if (bConSpinta)
		{
			if (PlanCoreAttack(Pusher, TEXT("Action.Push"), Watcher) == INDEX_NONE)
			{
				DestroyFallWorld(World); Esito.Nota = TEXT("Action.Push non installabile"); return Esito;
			}
		}
		else
		{
			NeutralizeAllIntents(Pusher);
		}

		// LO SGAMBETTO: `Prone` senza spinta. ⛔ **L'azione e' SINTETICA e dichiarata qui**: l'unico produttore di `Prone`
		// in partita e' la spinta su chi e' `Unbalanced`, e da [D-366] la spinta disarma da se'. Senza un'altra via il
		// disarmo di `Prone` non si potrebbe piu' misurare (review della PR #3783). La forma e' quella dello storditore di
		// `Status.StunSilencesAnArmedWatcher`: fase `Control`, effetto `Status`, aggressione dichiarata, e l'applicazione
		// passa da `ApplyStatusLogged`, cioe' da `DisarmPreparedReactions`.
		if (bSgambetto)
		{
			ARTUnit* Sgambettatore = SpawnFallUnit(World, VarcoEnemyTeam, FRTCellId(-3, 0));
			if (!Sgambettatore) { DestroyFallWorld(World); Esito.Nota = TEXT("sgambettatore non spawnato"); return Esito; }

			URTActionData* Azione = NewObject<URTActionData>(Sgambettatore);
			FRTActionDef Def;
			Def.ActionId = TEXT("Test.Trip");
			Def.ResolutionPhase = ERTResolutionPhase::Control;
			Def.Priority = 25;
			Def.RangeCells = 3;
			Def.CooldownTurns = 0;
			Def.Fallback = ERTActionFallback::Cancel;
			Def.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Prone,
				URTCombatLibrary::ProneDurationTurns));
			Def.bCountsAsAttack = true; // controllo OSTILE: raggiunge il bersaglio come colpo [`INT-8`]
			Azione->Def = Def;
			Azione->RangeCells = Def.RangeCells;
			Azione->Power = 0;
			Sgambettatore->Abilities.Add(Azione);

			Sgambettatore->SetPlannedMainAction(Sgambettatore->Abilities.Num() - 1);
			Sgambettatore->DeclareAttackOnUnit(Watcher);
			Sgambettatore->SetPlannedCell(Sgambettatore->Cell);
		}

		Mover->ClearPlannedMainAction();
		FRTUnitPlanTestAccess::SetPath(*Mover, { FRTCellId(0, 1), FRTCellId(0, 0) });
		Mover->SetPlannedCell(FRTCellId(0, 0));

		const int32 SaluteIniziale = Mover->Health + Mover->Shield;
		RunFallTurn(TM);
		Esito.Danno = SaluteIniziale - (Mover->Health + Mover->Shield);
		Esito.Log = TM->GetTurnLog();
		Esito.WatcherCell = Watcher->Cell;
		Esito.bWatcherProne = Watcher->HasStatus(TAG_Status_Prone);

		for (const FRTTurnLogEntry& E : Esito.Log)
		{
			if (E.Category == ERTLogCategory::ReactionDecision) { ++Esito.Decisioni; }
		}

		Esito.Nota = FString::Printf(
			TEXT("watcher %s->(q=%d,r=%d) prone=%d | mover (q=%d,r=%d) | decisioni=%d | danno=%d"),
			bConSpinta ? TEXT("spinto") : TEXT("fermo"),
			Watcher->Cell.X, Watcher->Cell.Y, Esito.bWatcherProne ? 1 : 0,
			Mover->Cell.X, Mover->Cell.Y, Esito.Decisioni, Esito.Danno);

		DestroyFallWorld(World);
		return Esito;
	}

	/** [D-366] Le voci `Reaction` / `Disarmed` del log. */
	TArray<FRTTurnLogEntry> VociDiDisarmo(const TArray<FRTTurnLogEntry>& Log)
	{
		TArray<FRTTurnLogEntry> Out;
		for (const FRTTurnLogEntry& E : Log)
		{
			if (E.Category == ERTLogCategory::Reaction
				&& static_cast<ERTReactionOutcome>(E.Outcome) == ERTReactionOutcome::Disarmed)
			{
				Out.Add(E);
			}
		}
		return Out;
	}

	/** [D-349] Le voci di silenzio `ChargeSpent` del log: «un nemico e' entrato nella linea, ma la carica era gia' spesa». */
	int32 SilenziCaricaSpesa(const TArray<FRTTurnLogEntry>& Log)
	{
		int32 N = 0;
		for (const FRTTurnLogEntry& E : Log)
		{
			if (E.Category == ERTLogCategory::Reaction
				&& static_cast<ERTReactionOutcome>(E.Outcome) == ERTReactionOutcome::NotTriggered
				&& E.Amount == static_cast<int32>(ERTOverwatchSilenceReason::ChargeSpent))
			{
				++N;
			}
		}
		return N;
	}
}

/**
 * `Prone` DISARMA l'Overwatch armato, e la charge e' **spesa**: non torna al proprietario.
 *
 * 🔑 **E' la linea di gioco che [D-319] costruisce**, ed e' la ragione per cui il blocco della reazione sta
 * su `Prone` e non su `Unbalanced`: spingere per disarmare costa un'azione all'avversario, mentre scivolare
 * non costa niente a nessuno (`brief` §4).
 *
 * 🔴 **Da [D-366] la spinta da sola disarma, e il caso che separava `Prone` dalla spinta e' cambiato.** Fino al
 * 2026-10-15 c'era un caso — spinto e in piedi, che SPARAVA — e serviva a dire che lo zero del guardiano caduto veniva
 * da `Prone`. Ora quel caso non spara per D-366, e sta in `Overwatch.ForcedDisplacementDisarmsWithItsCause`. Al suo
 * posto c'e' lo SGAMBETTO: `Prone` senza spinta, da un'azione sintetica, perche' l'unico produttore in partita e' la
 * spinta. Trovato in review (PR #3783): con i soli casi spinti, togliere `Prone` dalla regola del disarmo lasciava il
 * test verde.
 *
 * ∴ **Tre casi**: `A` fermo, che spara; `B` sgambettato e non spostato, che non spara e non ha voci di disarmo — `Prone`
 * non ne scrive —; `C` spinto e sbilanciato, che cade, non spara, e ha UNA voce di disarmo, quella della spinta.
 *
 * 🔴 **La geometria e' scelta perche' la spinta non porti il varco FUORI DAL CONO DI VISIONE, e la prima
 * stesura sbagliava proprio questo.** Metteva la sorgente a ovest: il guardiano veniva spinto a est, ma
 * `ApplyForcedDisplacement` lo GIRA verso chi l'ha spinto, cioe' a ovest — e il corridoio che sorvegliava
 * gli finiva alle spalle. Era il silenzio di `#2367`: zona ferma e occhi che ruotano, finche' [D-366] non l'ha
 * reso un disarmo dichiarato.
 *
 * ∴ La sorgente sta a EST, sulla stessa retta: la spinta allontana il guardiano dal varco **lasciandogli
 * il varco davanti**. Il varco e' la prima cella della linea in entrambe le posizioni post-spinta, e il caso `A`
 * nasce gia' nella cella in cui il guardiano finisce spinto.
 *
 * ⛔ **La predictive armata NON e' coperta.** `Prone` la rimuove nello stesso punto e con la stessa riga, ma
 * armare `Hero.Ivrin.InterceptShot` chiede un montaggio proprio: dichiarato come lacuna invece che
 * sottinteso come coperto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTProneDisarmsOverwatchTest,
	"RefactorTactics.Status.ProneDisarmsOverwatchAndSpendsCharge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTProneDisarmsOverwatchTest::RunTest(const FString&)
{
	// I casi si girano TUTTI prima di giudicare, e il referto li porta tutti: uscire al primo che cade lascia mezza
	// risposta, e un run del motore in piu' costa una finestra contesa fra sessioni.
	const FRTVarcoEsito A = GiocaIlVarco(/*bConSpinta=*/ false, /*bSbilanciato=*/ false, FRTCellId(-1, 0));
	const FRTVarcoEsito B = GiocaIlVarco(/*bConSpinta=*/ false, /*bSbilanciato=*/ false, FRTCellId(-1, 0),
		/*bSgambetto=*/ true);
	const FRTVarcoEsito C = GiocaIlVarco(/*bConSpinta=*/ true,  /*bSbilanciato=*/ true,  FRTCellId(0, 0));
	const FString Referto = FString::Printf(TEXT("A[%s] · B[%s] · C[%s]"), *A.Nota, *B.Nota, *C.Nota);

	// A — senza spinta: fissa che in QUESTO montaggio l'Overwatch spari davvero. Senza, lo zero sotto sarebbe vero
	// anche in un mondo dove l'Overwatch non funziona affatto.
	if (!TestTrue(FString::Printf(TEXT("A) premessa: un guardiano fermo SPARA — %s"), *Referto), A.Danno > 0))
	{
		return false;
	}

	// B — sgambettato: a terra, NON spostato, nella stessa cella di `A`. Se non spara e' `Prone` a disarmarlo, e nient'altro.
	TestTrue(FString::Printf(TEXT("B) premessa: lo sgambetto lo ha messo a terra — %s"), *Referto), B.bWatcherProne);
	TestEqual(FString::Printf(TEXT("B) premessa: e non lo ha spostato — %s"), *Referto), B.WatcherCell, FRTCellId(-1, 0));
	TestTrue(FString::Printf(TEXT("B) a terra senza spinta, non spara — %s"), *Referto), B.Danno <= 0);
	TestEqual(FString::Printf(TEXT("B) nessuna finestra: `Prone` lo ha disarmato — %s"), *Referto), B.Decisioni, 0);
	TestEqual(FString::Printf(TEXT("B) e nessuna voce di disarmo: `Prone` non ne scrive — %s"), *Referto),
		VociDiDisarmo(B.Log).Num(), 0);

	// C — spinto e sbilanciato: cade, e non spara.
	//
	// ⚠️ **`<= 0` e non `== 0`, e il perche' e' una misura, non una tolleranza.** La grandezza e'
	// `Health + Shield` prima meno dopo, e lo **scudo base rientra a inizio turno** ([D-224]:
	// `URTCombatLibrary::BaseShield`). Con il colpo il conto e' `21 - 5 assorbiti = 16`; senza, lo scudo
	// sale da 0 a 5 e la differenza e' **negativa**. Un `== 0` chiederebbe che nessuna delle due cose
	// accada, che non e' l'invariante: l'invariante e' che **nessun danno passi**.
	TestTrue(FString::Printf(TEXT("C) premessa: il guardiano spinto e sbilanciato e' caduto — %s"), *Referto),
		C.bWatcherProne);
	TestTrue(FString::Printf(TEXT("C) chi e' a terra non spara — %s"), *Referto), C.Danno <= 0);
	TestEqual(FString::Printf(TEXT("C) nessuna finestra: l'Overwatch non e' piu' armato — %s"), *Referto),
		C.Decisioni, 0);

	// 🔑 Una voce di disarmo, non due: la spinta l'ha gia' scritta ([D-366]), e `Prone` trova la charge spesa.
	TestEqual(FString::Printf(TEXT("C) la caduta non aggiunge una seconda voce di disarmo — %s"), *Referto),
		VociDiDisarmo(C.Log).Num(), 1);
	TestEqual(FString::Printf(TEXT("A) e chi non e' stato spinto non ne ha nessuna — %s"), *Referto),
		VociDiDisarmo(A.Log).Num(), 0);

	return true;
}

/**
 * [D-366] UNO SPOSTAMENTO FORZATO DEL WATCHER DISARMA ESPLICITAMENTE L'OVERWATCH, CON LA CAUSA NEL TURNLOG.
 *
 * Due casi sullo stesso banco del varco, e la differenza fra i due e' la sola spinta:
 *   `A` il guardiano nasce in `(-1,0)` e spara: e' la controprova che il colpo, da li', parte.
 *   `B` il guardiano nasce in `(0,0)` e la spinta lo porta in `(-1,0)`, girato verso la sorgente a EST, cioe' con lo
 *       stesso facing di `A`. Stessa cella e stessi occhi: se non spara, e' perche' e' disarmato.
 *
 * 🔴 **`B` e' il caso che il commento del 2026-09-12 su `#2367` chiedeva, e piu' stretto.** Quel commento voleva un
 * guardiano spinto con un alleato a coprire la linea, perche' un disarmo scritto come «non percepisce piu'» restasse
 * rosso. Qui il guardiano vede il varco da se': un disarmo finto lo lascerebbe sparare, e il test sarebbe rosso.
 *
 * Della voce si misura cio' che la decisione chiede e cio' che l'autore ha precisato il 2026-10-15:
 * - **la causa**: `Reaction` / `Disarmed` con `ForcedDisplacement` in `Amount`, lo spostamento in `SrcCell` -> `TgtCell`,
 *   e la voce dello spostamento che la precede;
 * - **chi la legge**: la squadra del guardiano e non quella che l'ha spinto, con il filtro del replay per osservatore;
 *   e l'audit di [D-313] ricalcola lo stesso verdetto per regola;
 * - **la seconda voce**: il nemico rilevato che poi entra nella linea lascia «la carica era gia' spesa» ([D-349]),
 *   come dopo il disarmo di `Prone`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTForcedDisplacementDisarmsOverwatchTest,
	"RefactorTactics.Overwatch.ForcedDisplacementDisarmsWithItsCause",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTForcedDisplacementDisarmsOverwatchTest::RunTest(const FString&)
{
	const FRTVarcoEsito A = GiocaIlVarco(/*bConSpinta=*/ false, /*bSbilanciato=*/ false, FRTCellId(-1, 0));
	const FRTVarcoEsito B = GiocaIlVarco(/*bConSpinta=*/ true,  /*bSbilanciato=*/ false, FRTCellId(0, 0));
	const FString Referto = FString::Printf(TEXT("A[%s] · B[%s]"), *A.Nota, *B.Nota);

	if (!TestTrue(FString::Printf(TEXT("A) premessa: da (-1,0) il guardiano fermo SPARA — %s"), *Referto), A.Danno > 0))
	{
		return false;
	}
	TestEqual(FString::Printf(TEXT("A) e non ha voci di disarmo — %s"), *Referto), VociDiDisarmo(A.Log).Num(), 0);
	if (!TestEqual(FString::Printf(TEXT("B) premessa: la spinta lo porta nella cella di A — %s"), *Referto),
		B.WatcherCell, FRTCellId(-1, 0)))
	{
		return false;
	}
	TestFalse(FString::Printf(TEXT("B) premessa: in piedi, non e' la caduta a spegnerlo — %s"), *Referto), B.bWatcherProne);

	// 🔴 IL PUNTO: spinto, non spara, e non gli viene nemmeno chiesto.
	TestTrue(FString::Printf(TEXT("B) spinto, non spara — %s"), *Referto), B.Danno <= 0);
	TestEqual(FString::Printf(TEXT("B) nessuna finestra: l'Overwatch e' disarmato — %s"), *Referto), B.Decisioni, 0);

	// La causa, nel TurnLog.
	const TArray<FRTTurnLogEntry> Disarmi = VociDiDisarmo(B.Log);
	if (!TestEqual(FString::Printf(TEXT("B) una voce di disarmo, una sola — %s"), *Referto), Disarmi.Num(), 1))
	{
		return false;
	}
	const FRTTurnLogEntry& Voce = Disarmi[0];
	TestEqual(TEXT("la causa e' lo spostamento forzato"), Voce.Amount,
		static_cast<int32>(ERTOverwatchDisarmCause::ForcedDisplacement));
	TestEqual(TEXT("da dove e' partito"), Voce.SrcCell, FRTCellId(0, 0));
	TestEqual(TEXT("dove e' finito"), Voce.TgtCell, FRTCellId(-1, 0));
	TestEqual(TEXT("di quale reazione"), Voce.ActionId, FName(TEXT("Action.Overwatch")));
	TestEqual(TEXT("nel Blast, dove avviene la spinta"), static_cast<int32>(Voce.Phase),
		static_cast<int32>(ERTMatchPhase::Blast));

	// Lo spostamento precede il disarmo: chi rilegge il turno legge prima la causa, poi la conseguenza.
	const int32 IndiceDisarmo = B.Log.IndexOfByPredicate([](const FRTTurnLogEntry& E)
	{
		return E.Category == ERTLogCategory::Reaction
			&& static_cast<ERTReactionOutcome>(E.Outcome) == ERTReactionOutcome::Disarmed;
	});
	const int32 IndiceSpostamento = B.Log.IndexOfByPredicate([](const FRTTurnLogEntry& E)
	{
		return E.Category == ERTLogCategory::Move && E.Phase == ERTMatchPhase::Blast
			&& E.SrcCell == FRTCellId(0, 0) && E.TgtCell == FRTCellId(-1, 0);
	});
	TestTrue(FString::Printf(TEXT("la voce dello spostamento c'e' (indice %d) e precede il disarmo (indice %d)"),
		IndiceSpostamento, IndiceDisarmo), IndiceSpostamento != INDEX_NONE && IndiceSpostamento < IndiceDisarmo);

	// Chi la legge: la squadra del guardiano, e non chi l'ha spinto. La spinta la vedono tutti; che ci fosse un
	// Overwatch armato no.
	TestEqual(TEXT("la squadra del guardiano legge il disarmo"),
		VociDiDisarmo(URTReplayPrivacyLibrary::FilterEntriesForObserver(B.Log, VarcoWatcherTeam)).Num(), 1);
	TestEqual(TEXT("la squadra che ha spinto no: non sa che quell'unita' aveva un Overwatch"),
		VociDiDisarmo(URTReplayPrivacyLibrary::FilterEntriesForObserver(B.Log, VarcoEnemyTeam)).Num(), 0);

	// L'audit di [D-313] la ricalcola per REGOLA e la trova coerente, come le voci di silenzio di [D-349].
	const FRTAuditVerdictRecord Record = URTReplayAuditLibrary::MakeVerdictRecord(Voce);
	TestEqual(TEXT("la regola arriva al record d'audit"), static_cast<int32>(Record.PlanVerdict),
		static_cast<int32>(ERTPlanVerdict::OwnerTeam));
	TestTrue(TEXT("e il ricalcolo coincide col verdetto registrato"),
		URTReplayAuditLibrary::RecomputeVerdict(FRTTurnAudit(), Record) == Voce.Verdict);
	TestEqual(TEXT("il soggetto sta dove il guardiano e' finito"), Record.SubjectCell, FRTCellId(-1, 0));

	// La seconda voce (decisione d'autore del 2026-10-15): il nemico che poi entra nella linea lascia «la carica era
	// gia' spesa». E' il silenzio di [D-349], invariato: dice al giocatore il colpo che la spinta gli ha tolto.
	TestEqual(FString::Printf(TEXT("B) poi il nemico entra nella linea: «la carica era gia' spesa», una volta — %s"),
		*Referto), SilenziCaricaSpesa(B.Log), 1);
	TestEqual(FString::Printf(TEXT("A) chi ha sparato non lo scrive: la carica l'ha spesa dopo — %s"), *Referto),
		SilenziCaricaSpesa(A.Log), 0);

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
	// ⛔ **Raggio 14 e non 6, e la differenza l'ha trovata una verifica di mutazione.** Con `6` la
	// destinazione «oltre `1x`» di un eroe che vale 6 cadeva FUORI dalla mappa: l'unita' restava ferma
	// perche' non esisteva un percorso, non perche' [D-319] le negasse la corsa. Il test era verde sul
	// candidato sano **e** sulla mutazione che toglie il clamp — cioe' non misurava niente.
	SpawnFallMap(World, /*Radius=*/ 14);

	ARTUnit* Runner = SpawnFallUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Foe = SpawnFallUnit(World, 1, FRTCellId(-6, 0));
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
	Runner->ClearPlannedMainAction();
	// Lo Sprint si dichiarava come PROFILO dal 2026-09-13 ([D-116], `#641`), e questo test scriveva
	// `PlannedMovementProfileId = ProfileSprint` con `PlannedCell = Cell` — cioe' una corsa dichiarata
	// e zero celle percorse. [D-425] ha tolto quel canale: correre e' ANDARE LONTANO, quindi il
	// soggetto del rifiuto e' una destinazione oltre `1x` e non un'etichetta.
	//
	// La destinazione si ricava dal movimento base invece di essere cablata: un `6` scritto qui
	// renderebbe il test una funzione del roster, e il giorno in cui Ivrin valesse 6 la premessa
	// diventerebbe falsa in silenzio — il piano starebbe nel neutro e non ci sarebbe nulla da rifiutare.
	const int32 Base = Runner->GetEffectiveMoveRange();
	const int32 Oltre = Base + 1;
	Runner->SetPlannedCell(FRTCellId(Oltre, 0));
	// ⛔ **La premessa si ASSERISCE**: una destinazione fuori mappa fermerebbe l'unita' per mancanza di
	// percorso, e il test misurerebbe la topologia invece di [D-319].
	if (!TestTrue(TEXT("premessa: la destinazione oltre 1x e' DENTRO la mappa"), Oltre <= 14))
	{
		DestroyFallWorld(World);
		return false;
	}
	// 🔴 **IL CONTROLLO POSITIVO, e senza di lui il test non prova niente.** Un gemello identico che NON
	// e' sbilanciato, con la stessa distanza in un'altra direzione: se arriva, allora mappa, budget e
	// pathfinding consentono quel percorso, e il fatto che il primo non arrivi e' [D-319] e nient'altro.
	//
	// ⛔ **Serve perche' l'asserzione principale e' NEGATIVA**, e una negativa e' vera per troppe ragioni:
	// destinazione fuori mappa, cella bloccata, unita' mai spawnata. La prima stesura di questo test ne ha
	// centrata una davvero — raggio `6` contro una destinazione a `7` — e restava verde sia sul candidato
	// sano sia su una mutazione che toglieva il clamp.
	ARTUnit* Gemello = SpawnFallUnit(World, 0, FRTCellId(0, 3));
	if (!TestNotNull(TEXT("il gemello del controllo positivo"), Gemello))
	{
		DestroyFallWorld(World); return false;
	}
	Gemello->ClearPlannedMainAction();
	Gemello->SetPlannedCell(FRTCellId(Oltre, 3));

	NeutralizeAllIntents(Foe);

	RunFallTurn(TM);

	// La regola: chi e' sbilanciato non corre, quindi non arriva dove la corsa lo avrebbe portato.
	TestTrue(*FString::Printf(TEXT("non ha corso: non e' arrivato a %d celle (e' a q=%d)"),
			Oltre, Runner->Cell.X),
		Runner->Cell.X < Oltre);

	// Il controllo: lo stesso piano, senza lo status, ARRIVA — e ci arriva perche' oltre `1x` la banda e'
	// `Sprint`, che e' anche la prova che la banda alta e' raggiungibile senza dichiararla.
	TestEqual(TEXT("controllo positivo: senza lo status lo stesso piano arriva"),
		Gemello->Cell, FRTCellId(Oltre, 3));

	// ⚠️ **E il primo resta FERMO, non a meta' strada**, perche' un piano a sola destinazione e'
	// tutto-o-niente: `FindPathForUnit` cerca una rotta ENTRO il budget verso la cella richiesta, e se non
	// c'e' non ne consegna una piu' corta. Il troncamento parziale appartiene ai piani a waypoint, ed e' la
	// ragione per cui `PlayerInput.DeclaringSneakTruncatesToHalf` misura quello e non questo.
	TestEqual(TEXT("e resta dov'era: senza rotta nel budget non si parte a meta'"),
		Runner->Cell, FRTCellId(0, 0));

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

		const TArray<FRTCellId> Extended = URTHexSimLibrary::ApplyIceSliding(Snapshot, /*UnitId=*/ 0, Path).Path;
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

		const TArray<FRTCellId> Extended = URTHexSimLibrary::ApplyIceSliding(Snapshot, /*UnitId=*/ 0, Path).Path;
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

		const TArray<FRTCellId> Extended = URTHexSimLibrary::ApplyIceSliding(Snapshot, /*UnitId=*/ 0, Path).Path;
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

		const TArray<FRTCellId> Extended = URTHexSimLibrary::ApplyIceSliding(Snapshot, /*UnitId=*/ 0, SuFloor).Path;
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


// =====================================================================================================
// `Status.Stunned` — il TERZO stato di controllo ([D-416], `#3142`).
//
// Vive in questo file e non in uno proprio perche' condivide con `Prone` il banco e, soprattutto, il
// MECCANISMO: i tre effetti del disarmo sono la stessa funzione (`DisarmPreparedReactions`), e misurarli
// da due montaggi diversi renderebbe possibile che uno dei due resti verde mentre l'altro cade.
//
// ⛔ **Nessuna azione e nessun eroe infligge lo stordimento**: [D-416] non assegna produttori. Questi test
// applicano lo stato direttamente, ed e' la stessa cosa che fara' il primo kit che lo dichiari.
// =====================================================================================================

/**
 * 🔴 **Il budget di pianificazione di una stordita resta intero e quello di una radicata e' zero; la condizione che la
 * risoluzione legge e' vera solo per chi e' stordito E ha un Move pianificato** ([D-569], `#3973`). Questo test prova i DUE
 * PREDICATI e il budget, non la risoluzione: il movimento negato in partita lo misurano
 * `Status.StunnedMovement.TheWalkIsRefusedWhenStunned` e `Status.StunCensus.EveryMobilityIsRefusedWhenStunned`.
 *
 * ⌫ *Questo test si chiamava `Status.StunDeniesTheActionNotTheMovement` e diceva l'opposto della regola di oggi, poi
 * `Status.StunDeniesTheMovementAtResolutionNotInPlanning` (un nome che prometteva piu' di quanto asserisse).* [D-416] (1)
 * scriveva che lo stordimento «non tocca il movimento», perche' quello e' `Root`, e il test pinnava che il budget di una stordita
 * restasse intero: vero, e lo e' ancora. Quello che [D-569] ribalta e' un'altra cosa — la stordita NON CAMMINA —, ma lo ribalta
 * alla risoluzione: il click continua ad accettare il piano di una stordita ([D-565] (2), `#3134`), quindi il budget di
 * pianificazione non si tocca, mentre `Root` lo azzera e il click rifiuta prima.
 *
 * Il montaggio e' costruito perche' cada in ENTRAMBI i versi: se `Stunned` finisse per azzerare il budget sarebbe rosso il
 * primo asserto (il click rifiuterebbe il piano che [D-565] (2) vuole accettato), e se `Root` smettesse di azzerarlo sarebbe
 * rosso il secondo.
 *
 * ⚠️ **Due unita' e non una riusata**: `RemoveStatus` fra i due casi misurerebbe anche la rimozione, che e'
 * un'altra proprieta'. Con due soggetti lo zero del secondo non puo' venire da un residuo del primo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStunDeniesActionNotMovementTest,
	"RefactorTactics.Status.StunKeepsThePlanningBudgetAndNamesTheMoveItDenies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStunDeniesActionNotMovementTest::RunTest(const FString&)
{
	UWorld* World = MakeFallWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnFallMap(World, /*Radius=*/ 4);

	ARTUnit* Stordito = SpawnFallUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Radicato = SpawnFallUnit(World, 0, FRTCellId(2, 0));
	if (!Stordito || !Radicato) { DestroyFallWorld(World); return false; }

	const int32 BudgetPieno = Stordito->GetEffectiveMoveRange();
	if (!TestTrue(TEXT("il banco parte da un budget non nullo, altrimenti non distingue niente"),
		BudgetPieno > 0))
	{
		DestroyFallWorld(World);
		return false;
	}

	Stordito->ApplyStatus(TAG_Status_Stunned, URTCombatLibrary::StunnedDurationTurns);
	Radicato->ApplyStatus(TAG_Status_Root, /*Turni=*/ 1);

	TestEqual(TEXT("chi e' stordito conserva il budget di pianificazione: il click accetta il suo piano, lo nega la risoluzione"),
		Stordito->GetEffectiveMoveRange(), BudgetPieno);
	TestEqual(TEXT("chi e' radicato no: il budget e' azzerato, e il click rifiuta prima"),
		Radicato->GetEffectiveMoveRange(), 0);

	// La condizione che la RISOLUZIONE legge ([D-569]): stordita E con un Move pianificato. Senza il Move non c'e' niente da
	// negare (e una stordita ferma non scriverebbe un rifiuto a ogni turno); senza lo stordimento il Move si compie.
	FRTUnitPlanTestAccess::SetPath(*Stordito, { FRTCellId(0, 0), FRTCellId(1, 0) });
	Stordito->SetPlannedCell(FRTCellId(1, 0));
	FRTUnitPlanTestAccess::SetPath(*Radicato, { FRTCellId(2, 0), FRTCellId(3, 0) });
	Radicato->SetPlannedCell(FRTCellId(3, 0));
	TestTrue(TEXT("premessa: il Move di chi e' stordito e' pianificato"), Stordito->HasPlannedNormalMove());
	TestTrue(TEXT("chi e' stordito e ha un Move pianificato: la risoluzione lo nega"), Stordito->PlannedNormalMoveDeniedByStun());
	TestFalse(TEXT("chi e' solo radicato: non e' questa condizione (lo nega il budget)"), Radicato->PlannedNormalMoveDeniedByStun());
	FRTUnitPlanTestAccess::ResetPath(*Stordito);
	Stordito->SetPlannedCell(Stordito->Cell);
	TestFalse(TEXT("chi e' stordito ma non ha pianificato un Move: niente da negare"), Stordito->PlannedNormalMoveDeniedByStun());

	// ⚠️ Il rango di gravita' e' l'altra meta' della distinzione, e si chiede a chi la possiede invece di
	// riscriverla: `Stunned` toglie di piu', quindi viene prima. Se i due si scambiassero, `Reaction.Cleanse`
	// annullerebbe il controllo sbagliato quando arrivano insieme.
	TestTrue(TEXT("`Stunned` e' piu' grave di `Root`"),
		URTReactionLibrary::ControlSeverityRank(TAG_Status_Stunned)
			< URTReactionLibrary::ControlSeverityRank(TAG_Status_Root));

	DestroyFallWorld(World);
	return true;
}

/**
 * 🔴 **Lo stordimento cancella l'azione principale, e il turno DICE perche'.**
 *
 * Due bracci, e il primo e' obbligatorio: senza, uno zero colpi non distingue *«l'azione e' stata
 * rifiutata»* da *«il banco non colpisce comunque»*. Il braccio `A` fissa che il colpo arrivi davvero; solo
 * allora lo zero del braccio `B` significa qualcosa.
 *
 * ⚠️ **Si asserisce anche la VOCE, non solo il colpo mancato.** Un'azione che sparisce in silenzio e'
 * indistinguibile da un difetto — e' la disciplina che `Fallback`/`Cancelled` esiste per applicare, la
 * stessa del rifiuto per `Unbalanced` in `ResolveDash`. Il motivo viaggia in `Amount`.
 *
 * 🔑 **E il braccio `A` asserisce anche l'ASSENZA del rifiuto**: senza, un'implementazione che scrivesse la
 * voce sempre — stordito o no — passerebbe il braccio `B` e nessuno se ne accorgerebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStunCancelsMainActionTest,
	"RefactorTactics.Status.StunCancelsTheMainActionWithItsReason",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStunCancelsMainActionTest::RunTest(const FString&)
{
	// Riempie `OutColpi` col numero di voci `Combat` del turno e `OutRifiutata` con true se il TurnLog porta
	// il rifiuto per stordimento. Torna `false` se il montaggio non e' riuscito.
	//
	// 🔑 **Si misura il TURNLOG e non la salute, ed e' una correzione pagata.** La prima stesura confrontava
	// `Health + Shield` prima e dopo, e il braccio di controllo tornava **-5**: lo scudo base che l'unita'
	// riceve nel turno entra nella stessa somma, quindi il delta non era il danno. Il colpo e' un EVENTO, e
	// l'evento e' quello che il turno registra.
	auto GiraIlTurno = [this](bool bStordito, int32& OutColpi, bool& OutRifiutata, FString& OutNota) -> bool
	{
		OutColpi = 0;
		OutRifiutata = false;
		OutNota.Reset();

		UWorld* World = MakeFallWorld();
		if (!World) { OutNota = TEXT("world non creato"); return false; }
		SpawnFallMap(World, /*Radius=*/ 5);

		ARTUnit* Attaccante = SpawnFallUnit(World, 0, FRTCellId(0, 0));
		ARTUnit* Bersaglio = SpawnFallUnit(World, 1, FRTCellId(1, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Attaccante || !Bersaglio) { DestroyFallWorld(World); OutNota = TEXT("spawn fallito"); return false; }

		// ⛔ **L'attacco base e' quello dell'EROE, non `Action.BasicAttack` del catalogo generico.** Quella
		// riga dichiara identita', fase e priorita' e lascia fuori DANNO e PORTATA di proposito — li mette
		// `MakeBasicAttack` per arma (`RTCatalogLibrary.cpp:1120-1125`). Installarla con
		// `AddCoreAbility` da' un'azione con portata 0 e nessun effetto: e' il difetto per cui il braccio di
		// controllo di questo test, alla prima stesura, non produceva nessun colpo.
		//
		// Si cerca per `BaseActionId`, che e' la stessa disciplina con cui l'Overwatch trova l'arma da cui
		// prende portata e danno (`RTTurnManager.cpp`, `Heroes.BasicAttackDeclaresItsBaseAction`).
		int32 AttaccoBase = INDEX_NONE;
		for (int32 A = 0; A < Attaccante->NumAbilities(); ++A)
		{
			const URTActionData* Arma = Attaccante->GetAbility(A);
			if (Arma && Arma->Def.BaseActionId == FName(TEXT("Action.BasicAttack"))) { AttaccoBase = A; break; }
		}
		if (AttaccoBase == INDEX_NONE)
		{
			DestroyFallWorld(World); OutNota = TEXT("l'eroe non dichiara un attacco base"); return false;
		}
		Attaccante->SetPlannedMainAction(AttaccoBase);
		Attaccante->DeclareAttackOnUnit(Bersaglio);
		Attaccante->SetPlannedCell(Attaccante->Cell);
		NeutralizeAllIntents(Bersaglio);

		if (bStordito)
		{
			Attaccante->ApplyStatus(TAG_Status_Stunned, URTCombatLibrary::StunnedDurationTurns);
		}

		RunFallTurn(TM);

		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Combat) { ++OutColpi; }

			// ⚠️ **Si cerca il motivo ATTESO, non l'ultima voce annullata.** Prendere l'ultima renderebbe il
			// verdetto dipendente da quante altre voci il turno produce dopo — e un fallimento direbbe «il
			// motivo e' sbagliato» dove il difetto vero e' «la voce non c'e' affatto».
			if (E.Category == ERTLogCategory::Fallback
				&& E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Cancelled)
				&& E.Amount == static_cast<int32>(ERTActionInvalidReason::Stunned))
			{
				OutRifiutata = true;
			}
		}

		OutNota = FString::Printf(TEXT("stordito=%d | colpi=%d | rifiutata=%d"),
			bStordito ? 1 : 0, OutColpi, OutRifiutata ? 1 : 0);

		DestroyFallWorld(World);
		return true;
	};

	int32 ColpiA = 0; bool RifiutataA = false; FString NotaA;
	if (!TestTrue(TEXT("il montaggio A regge"), GiraIlTurno(/*bStordito=*/ false, ColpiA, RifiutataA, NotaA)))
	{
		return false;
	}

	int32 ColpiB = 0; bool RifiutataB = false; FString NotaB;
	if (!TestTrue(TEXT("il montaggio B regge"), GiraIlTurno(/*bStordito=*/ true, ColpiB, RifiutataB, NotaB)))
	{
		return false;
	}

	// (A) IL CONTROLLO. Senza questo, lo zero del caso (B) non e' una misura: direbbe «non ha colpito» anche
	// se il banco non colpisse comunque.
	if (!TestTrue(FString::Printf(TEXT("A — non stordito, il colpo c'e' [%s]"), *NotaA), ColpiA > 0))
	{
		return false;
	}
	TestFalse(TEXT("A — e nessuna voce parla di stordimento"), RifiutataA);

	// (B) IL CUORE.
	TestEqual(FString::Printf(TEXT("B — stordito, nessun colpo [%s]"), *NotaB), ColpiB, 0);
	TestTrue(TEXT("B — e il turno dice PERCHE': la voce porta il motivo `Stunned`"), RifiutataB);

	// La frase che il giocatore legge esiste e non e' la resa generica: e' la stessa proprieta' che il giro
	// sull'enum di `DescribeInvalidReason` sorveglia per ogni motivo nuovo.
	const FString Detto = URTTurnLogLibrary::DescribeInvalidReason(ERTActionInvalidReason::Stunned);
	TestNotEqual(TEXT("il motivo e' tradotto, non generico"), Detto, FString(TEXT("non eseguibile")));

	return true;
}

/**
 * 🔴 **Un guardiano stordito non spara, e le due strade per cui non spara sono DUE regole diverse.**
 *
 * Stesso banco della caduta — il guardiano ha il varco davanti a se' — perche' la geometria e' gia' stata
 * pagata li': il commento di `ProneDisarmsOverwatchAndSpendsCharge` spiega perche' un montaggio ingenuo
 * finisce per misurare la rotazione del facing invece del disarmo.
 *
 * ⚠️ **TRE bracci, e ciascuno isola una cosa sola.**
 *   `A` guardiano lucido — fissa che in questo montaggio l'Overwatch spari davvero. Senza, gli zeri di `B`
 *       e `C` non sarebbero misure.
 *   `B` gia' stordito PRIMA del turno — non arriva nemmeno ad armare: armare costa l'azione principale
 *       (catalogo §1) e lo stordimento la nega nel **Prep**.
 *   `C` arma, e viene stordito DOPO — nel Blast, da un'azione nemica. Qui l'Overwatch c'e' ed e'
 *       `DisarmPreparedReactions` a spegnerlo, cioe' la stessa funzione che usa `Prone`.
 *
 * 🔑 **`C` e' il braccio che misura la riga di [D-416] «disattiva cio' che l'unita' aveva armato»**, e
 * senza di lui il test direbbe solo che uno stordito non puo' armare — che e' vero, ma e' un'altra frase.
 *
 * ⏱️ **La prima stesura aveva due bracci e il braccio `B` FALLIVA**, con `danno=16`: il rifiuto
 * dell'azione principale esisteva nel solo Blast, e l'Overwatch si arma nel Prep. Il difetto era nel
 * prodotto, non nel banco, ed e' stato corretto aggiungendo il secondo sito del rifiuto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStunSilencesTheWatcherTest,
	"RefactorTactics.Status.StunSilencesAnArmedWatcher",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStunSilencesTheWatcherTest::RunTest(const FString&)
{
	// Quando lo stordimento colpisce il guardiano.
	enum class EQuando : uint8 { Mai, PrimaDelTurno, NelBlast };

	// `OutDecisioni` conta le finestre di reazione decise — e' il vero soggetto: l'Overwatch che spara e'
	// un EVENTO, e un delta di salute non lo e'. `OutRifiutataInPrep` distingue il braccio `B` dal `C`.
	auto DannoNelVarco = [this](EQuando Quando, int32& OutDecisioni, bool& OutRifiutataInPrep,
		FString& OutNota) -> int32
	{
		OutDecisioni = 0;
		OutRifiutataInPrep = false;
		OutNota.Reset();

		UWorld* World = MakeFallWorld();
		if (!World) { OutNota = TEXT("world non creato"); return -1; }
		SpawnFallMap(World, /*Radius=*/ 7);

		ARTUnit* Watcher = SpawnFallUnit(World, 0, FRTCellId(-1, 0));
		ARTUnit* Mover = SpawnFallUnit(World, 1, FRTCellId(0, 1));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Watcher || !Mover) { DestroyFallWorld(World); OutNota = TEXT("spawn fallito"); return -1; }

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
		Watcher->SetPlannedMainAction(OverwatchIdx);
		Watcher->SetPlannedCell(Watcher->Cell);

		if (Quando == EQuando::PrimaDelTurno)
		{
			Watcher->ApplyStatus(TAG_Status_Stunned, URTCombatLibrary::StunnedDurationTurns);
		}

		// LO STORDITORE, solo per il braccio `C`. Sta a OVEST del guardiano — dietro di lui, fuori dal
		// corridoio sorvegliato — perche' il banco non misuri per sbaglio un'occlusione.
		//
		// ⛔ **L'azione e' SINTETICA e dichiarata qui**: [D-416] non assegna produttori allo stordimento, e
		// nessuna azione di catalogo lo infligge. Costruirla e' l'unico modo di misurare il percorso vero —
		// l'applicazione passa da `ApplyStatusLogged`, quindi da `DisarmPreparedReactions` — ed e' lo stesso
		// idioma di `MakePush2Def` in `RTControlActionTests`. La forma copia `Action.Root`
		// (`RTCatalogLibrary.cpp`): fase `Control`, effetto `Status`, aggressione dichiarata.
		if (Quando == EQuando::NelBlast)
		{
			ARTUnit* Storditore = SpawnFallUnit(World, 1, FRTCellId(-3, 0));
			if (!Storditore) { DestroyFallWorld(World); OutNota = TEXT("storditore non spawnato"); return -1; }

			URTActionData* Azione = NewObject<URTActionData>(Storditore);
			FRTActionDef Def;
			Def.ActionId = TEXT("Test.Stun");
			Def.ResolutionPhase = ERTResolutionPhase::Control;
			Def.Priority = 25;
			Def.RangeCells = 3;
			Def.CooldownTurns = 0;
			Def.Fallback = ERTActionFallback::Cancel;
			Def.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Stunned,
				URTCombatLibrary::StunnedDurationTurns));
			Def.bCountsAsAttack = true; // controllo OSTILE: raggiunge il bersaglio come colpo [`INT-8`]
			Azione->Def = Def;
			Azione->RangeCells = Def.RangeCells;
			Azione->Power = 0;
			Storditore->Abilities.Add(Azione);

			Storditore->SetPlannedMainAction(Storditore->Abilities.Num() - 1);
			Storditore->DeclareAttackOnUnit(Watcher);
			Storditore->SetPlannedCell(Storditore->Cell);
		}

		Mover->ClearPlannedMainAction();
		FRTUnitPlanTestAccess::SetPath(*Mover, { FRTCellId(0, 1), FRTCellId(0, 0) });
		Mover->SetPlannedCell(FRTCellId(0, 0));

		const int32 SaluteIniziale = Mover->Health + Mover->Shield;
		RunFallTurn(TM);
		const int32 Danno = SaluteIniziale - (Mover->Health + Mover->Shield);

		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::ReactionDecision) { ++OutDecisioni; }

			// Il rifiuto dell'azione principale nel **Prep**: e' cio' che accade a chi e' gia' stordito
			// quando prova ad armare. Nel braccio `C` NON deve esserci — li' il guardiano arma davvero, e
			// lo stordimento arriva dopo.
			if (E.Phase == ERTMatchPhase::Prep
				&& E.Category == ERTLogCategory::Fallback
				&& E.Amount == static_cast<int32>(ERTActionInvalidReason::Stunned))
			{
				OutRifiutataInPrep = true;
			}
		}

		OutNota = FString::Printf(
			TEXT("quando=%d | stordito=%d | mover (q=%d,r=%d) | decisioni=%d | rifiutoPrep=%d | danno=%d"),
			static_cast<int32>(Quando), Watcher->HasStatus(TAG_Status_Stunned) ? 1 : 0,
			Mover->Cell.X, Mover->Cell.Y, OutDecisioni, OutRifiutataInPrep ? 1 : 0, Danno);

		DestroyFallWorld(World);
		return Danno;
	};

	int32 DecisioniA = 0; bool PrepA = false; FString NotaA;
	const int32 DannoA = DannoNelVarco(EQuando::Mai, DecisioniA, PrepA, NotaA);

	int32 DecisioniB = 0; bool PrepB = false; FString NotaB;
	const int32 DannoB = DannoNelVarco(EQuando::PrimaDelTurno, DecisioniB, PrepB, NotaB);

	int32 DecisioniC = 0; bool PrepC = false; FString NotaC;
	const int32 DannoC = DannoNelVarco(EQuando::NelBlast, DecisioniC, PrepC, NotaC);

	// ⚠️ **Sul danno si asserisce `<= 0`, non `== 0`, e la ragione e' misurata.** Lo scudo base che l'unita'
	// riceve nel turno entra nella stessa somma `Health + Shield`: il delta del braccio `B` e' **-5** anche
	// quando non arriva un solo colpo. Il soggetto vero e' il numero di finestre decise — l'Overwatch che
	// spara e' un evento — e il danno resta come conferma, non come misura.

	// (A) IL CONTROLLO: in questo montaggio l'Overwatch spara davvero.
	if (!TestTrue(FString::Printf(TEXT("A — guardiano lucido, l'Overwatch spara [%s]"), *NotaA),
		DannoA > 0 && DecisioniA > 0))
	{
		return false;
	}

	// (B) Gia' stordito: non arma nemmeno, perche' armare costa l'azione principale — e il turno lo DICE,
	// con un rifiuto in fase Prep.
	TestEqual(FString::Printf(TEXT("B — nessuna finestra aperta [%s]"), *NotaB), DecisioniB, 0);
	TestTrue(TEXT("B — e il rifiuto e' dichiarato nel Prep, dove si sarebbe armato"), PrepB);
	TestTrue(TEXT("B — il varco non fa male"), DannoB <= 0);

	// (C) IL CUORE: aveva armato — nessun rifiuto in Prep lo prova — ed e' il DISARMO a spegnerlo.
	TestEqual(FString::Printf(TEXT("C — nessuna finestra aperta [%s]"), *NotaC), DecisioniC, 0);
	TestFalse(TEXT("C — e il guardiano AVEVA armato: nessun rifiuto in Prep"), PrepC);
	TestTrue(TEXT("C — il varco non fa male"), DannoC <= 0);

	return true;
}

// =========================================================================================================
// 5. Lo scatto A BUDGET negato dallo stato: il ramo di `ResolveDash` che il catalogo non raggiunge ([D-471])
// =========================================================================================================
namespace
{
	/** La voce del rifiuto di `ResolveDash` per questa unita', o `nullptr`. */
	const FRTTurnLogEntry* BudgetDashRefusalOf(const ARTTurnManager* TM, const ARTUnit* Unit)
	{
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Fallback && E.UnitId == Unit->StableUnitId
				&& E.Phase == ERTMatchPhase::Dash
				&& E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Cancelled)
				&& E.Amount == static_cast<int32>(ERTActionInvalidReason::Unbalanced))
			{
				return &E;
			}
		}
		return nullptr;
	}

	/**
	 * Un turno con uno scatto pianificato di DUE celle: `Action.Dodge`, con lo stile che chiede il test.
	 * Torna la cella dell'unita' a turno risolto, e la voce del rifiuto se c'e'.
	 */
	FRTCellId RunBudgetDashTurn(FAutomationTestBase& Test, ERTMovementStyle Stile, bool bSbilanciato,
		bool& bOutRifiutato)
	{
		bOutRifiutato = false;
		UWorld* World = MakeFallWorld();
		if (!Test.TestNotNull(TEXT("world di prova"), World)) { return FRTCellId(); }
		SpawnFallMap(World, /*Radius=*/ 6);
		ARTUnit* Runner = SpawnFallUnit(World, 0, FRTCellId(0, 0));
		ARTUnit* Foe = SpawnFallUnit(World, 1, FRTCellId(-6, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Runner || !Foe) { DestroyFallWorld(World); return FRTCellId(); }
		NeutralizeAllIntents(Foe);
		NeutralizeAllIntents(Runner);

		// 🔑 **L'azione si COSTRUISCE**: nessuna mobilita' rapida spedita e' a budget, quindi il ramo si raggiunge
		// solo cosi'. `Action.Dodge` resta `FastMovement`; cambia il solo stile, che e' cio' che il criterio legge.
		const int32 Scatto = RTAbilityFixtures::AddCoreAbility(Runner, TEXT("Action.Dodge"));
		if (!Test.TestTrue(TEXT("premessa: Action.Dodge e' nel catalogo core"), Scatto != INDEX_NONE))
		{
			DestroyFallWorld(World);
			return FRTCellId();
		}
		Runner->GetAbility(Scatto)->Def.MovementStyle = Stile;
		if (bSbilanciato)
		{
			Runner->ApplyStatus(TAG_Status_Unbalanced, URTCombatLibrary::UnbalancedDurationTurns);
		}
		Runner->SetPlannedDash(Scatto, FRTCellId(2, 0));
		Test.TestTrue(TEXT("premessa: lo scatto si applica, per la regola del catalogo"), Runner->PlannedDashApplies());

		RunFallTurn(TM);
		const FRTCellId Arrivo = Runner->Cell;
		bOutRifiutato = BudgetDashRefusalOf(TM, Runner) != nullptr;
		DestroyFallWorld(World);
		return Arrivo;
	}
}

/**
 * **`Unbalanced` nega lo scatto a BUDGET** ([D-319], [D-471]): l'unita' resta ferma, e il rifiuto ha la sua voce.
 *
 * Fino a [D-471] la condizione era scritta dentro `ResolveDash`; ora e' `ARTUnit::PlannedDashDeniedByStatus()`,
 * la stessa che anteprima e mira leggono. Questo test pinna che il resolver non e' cambiato: stessa voce, stesso
 * esito. ⚠️ Nessun test lo raggiungeva prima: da [D-425] lo `Sprint` non e' piu' una mobilita' rapida.
 *
 * 🔴 **Due controlli positivi**, perche' l'asserzione principale e' negativa — «non si e' mosso» e' vera anche per
 * una mappa che non lo permette. Lo stesso scatto SENZA lo stato arriva; uno scatto LINEARE con lo stato arriva
 * anche lui, perche' [D-319] nega la corsa, non lo slancio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBudgetDashRefusedWhileUnbalancedTest,
	"RefactorTactics.Status.BudgetDashRefusedWhileUnbalanced",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBudgetDashRefusedWhileUnbalancedTest::RunTest(const FString&)
{
	const FRTCellId Partenza(0, 0);
	const FRTCellId Meta(2, 0);
	bool bRifiutato = false;

	// CONTROLLO 1 — lo stesso scatto a budget, senza lo stato: arriva, e nessun rifiuto.
	const FRTCellId SenzaStato = RunBudgetDashTurn(*this, ERTMovementStyle::Budget, /*bSbilanciato=*/ false, bRifiutato);
	TestEqual(TEXT("controllo: senza Unbalanced lo scatto a budget arriva"), SenzaStato, Meta);
	TestFalse(TEXT("controllo: e non lascia un rifiuto"), bRifiutato);

	// IL CUORE — con lo stato resta fermo, e il rifiuto e' dichiarato.
	const FRTCellId ConStato = RunBudgetDashTurn(*this, ERTMovementStyle::Budget, /*bSbilanciato=*/ true, bRifiutato);
	TestEqual(TEXT("Unbalanced: lo scatto a budget non parte"), ConStato, Partenza);
	TestTrue(TEXT("e il TurnLog dice perche' (Fallback/Cancelled, motivo Unbalanced)"), bRifiutato);

	// CONTROLLO 2 — lo slancio lineare, con lo stato: arriva lo stesso.
	const FRTCellId Lineare = RunBudgetDashTurn(*this, ERTMovementStyle::LinearDash, /*bSbilanciato=*/ true, bRifiutato);
	TestEqual(TEXT("controllo: uno scatto lineare con Unbalanced arriva"), Lineare, Meta);
	TestFalse(TEXT("controllo: e non lascia un rifiuto"), bRifiutato);
	return true;
}

// =========================================================================================================
// 6. Lo stordimento NEGA la mobilita' ([D-567], #3964, esteso a OGNI mobilita' da [D-569], #3973)
// =========================================================================================================
namespace
{
	/** Ricarica dichiarata della mobilita' di prova: dopo un addebito e il decremento del Cleanup ne restano due. */
	constexpr int32 StunMobCooldown = 3;

	// La scena sta SPOSTATA di un offset non nullo: se chi scatta partisse da `FRTCellId()` (0,0), l'asserto 'le sue due celle
	// sono quella di partenza' sarebbe vero anche per una voce con una cella mai scritta, e un `TgtCell` non impostato non
	// cadrebbe (N05 della mutazione indipendente).
	const FRTCellId StunMobStart(1, 0);      //< chi scatta (la stordita)
	const FRTCellId StunMobGoal(3, 0);       //< la sua meta pianificata, a due celle
	const FRTCellId StunMobMateStart(1, 2);  //< il compagno: stessa X e Y maggiore, quindi DOPO di lei nell'ordine di raccolta
	const FRTCellId StunMobMateGoal(3, 2);   //< la sua meta pianificata, a due celle

	/** Che cosa cambia da un braccio all'altro del test: tutto il resto e' uguale. */
	struct FStunMobCase
	{
		ERTActionSlot Slot = ERTActionSlot::MovementAndMain;
		bool bStordita = false;
		bool bSbilanciata = false;    //< anche `Status.Unbalanced`
		bool bABudget = false;        //< stile `Budget`, che `Unbalanced` nega ([D-319]); altrimenti quello del catalogo, `LinearDash`
		bool bGiaInRicarica = false;  //< la ricarica e' gia' partita: `PlannedDashApplies()` e' falso
		bool bCompagno = false;       //< un compagno NON stordito, ordinato DOPO la stordita, con la mobilita' del catalogo
	};

	/** Quello che un turno con UNA mobilita' pianificata di due celle lascia da leggere. */
	struct FStunMobOutcome
	{
		bool bValido = false;
		FRTCellId Arrivo;
		bool bApplicaPrima = false;     //< `PlannedDashApplies()` PRIMA del turno: per la regola del catalogo lo scatto parte
		bool bSpostaPrima = false;      //< `PlannedDashMoves()` PRIMA del turno: la domanda di anteprima e mira
		int32 Rifiuti = 0;              //< voci Fallback/Cancelled `Stunned` in fase Dash che nominano l'azione dello scatto
		int32 RifiutiDaFermo = 0;       //< fra quelle, le due celle della voce sono la cella di partenza (la meta e' piano privato)
		int32 RifiutiUnbalanced = 0;    //< voci Fallback/Cancelled `Unbalanced` in fase Dash di chi scatta
		int32 Attivazioni = 0;          //< eventi `AbilityActivated` di chi scatta: un rifiuto non si attiva
		int32 Ricarica = -1;            //< il residuo a fine turno (il Cleanup decrementa di uno)
		FRTCellId ArrivoCompagno;       //< dove e' finito il compagno (solo con `bCompagno`)
		bool bCompagnoDopo = false;     //< premessa: nell'ordine REALE di raccolta il compagno viene dopo la stordita
	};

	/**
	 * Un turno con `Action.Dodge` costruita come dice `C`, pianificata di due celle da una unita' (stordita o no).
	 *
	 * 🔑 **L'azione si COSTRUISCE**: nessuna azione di catalogo dichiara `MovementAndMain` (il censimento
	 * `Status.StunCensus.EveryMainActionHasAKnownSite` lo vieta finche' non ha la sua riga), quindi il ramo non si raggiunge
	 * altrimenti. `Action.Dodge` resta `FastMovement`, cioe' macro-fase Dash; cambiano lo slot e, per un braccio, lo stile,
	 * che sono cio' che i criteri leggono.
	 */
	FStunMobOutcome RunStunMobTurn(FAutomationTestBase& Test, const FStunMobCase& C)
	{
		FStunMobOutcome Esito;
		UWorld* World = MakeFallWorld();
		if (!Test.TestNotNull(TEXT("world di prova"), World)) { return Esito; }
		SpawnFallMap(World, /*Radius=*/ 6);
		ARTUnit* Runner = SpawnFallUnit(World, 0, StunMobStart);
		ARTUnit* Foe = SpawnFallUnit(World, 1, FRTCellId(-6, 0));
		// Il compagno sta DOPO la stordita nell'ordine di raccolta (stessa X, Y maggiore): se la guardia uscisse dal ciclo
		// invece di passare all'unita' successiva, e' lui a non scattare.
		ARTUnit* Compagno = C.bCompagno ? SpawnFallUnit(World, 0, StunMobMateStart) : nullptr;
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Runner || !Foe || (C.bCompagno && !Compagno)) { DestroyFallWorld(World); return Esito; }
		NeutralizeAllIntents(Foe);
		NeutralizeAllIntents(Runner);

		// ⚠️ **Nello slot 3 e non appesa in coda**: `AddCoreAbility` la metterebbe a un indice che `AbilityCooldowns` non copre,
		// `ConsumeAbility` non scriverebbe niente e «non ha pagato» sarebbe vero anche col difetto. Lo slot 3 non coincide
		// nemmeno con l'indice che le unita' hanno nell'ordine di raccolta.
		const int32 Scatto = RTAbilityFixtures::AddCoreAbilityInSlot(Runner, TEXT("Action.Dodge"), 3);
		if (!Test.TestTrue(TEXT("premessa: Action.Dodge e' nel catalogo core e lo slot 3 esiste"), Scatto != INDEX_NONE))
		{
			DestroyFallWorld(World);
			return Esito;
		}
		URTActionData* Azione = Runner->GetAbility(Scatto);
		Azione->Def.Slot = C.Slot;
		if (C.bABudget) { Azione->Def.MovementStyle = ERTMovementStyle::Budget; }
		Azione->Def.CooldownTurns = StunMobCooldown;
		Azione->CooldownTurns = StunMobCooldown;
		const FName AzioneAttesa = Azione->Def.ActionId;
		const FRTCellId Partenza = Runner->Cell;
		Test.TestTrue(TEXT("premessa: chi scatta non parte dalla cella di default"), !(Partenza == FRTCellId()));

		if (C.bStordita) { Runner->ApplyStatus(TAG_Status_Stunned, URTCombatLibrary::StunnedDurationTurns); }
		if (C.bSbilanciata) { Runner->ApplyStatus(TAG_Status_Unbalanced, URTCombatLibrary::UnbalancedDurationTurns); }
		if (C.bGiaInRicarica)
		{
			Runner->ConsumeAbility(Scatto);
			Runner->TickCooldowns(); // StunMobCooldown - 1: un secondo `ConsumeAbility` la riporterebbe al valore pieno
		}
		Runner->SetPlannedDash(Scatto, StunMobGoal);
		Esito.bApplicaPrima = Runner->PlannedDashApplies();
		Esito.bSpostaPrima = Runner->PlannedDashMoves();
		Test.TestTrue(TEXT("premessa: il Def porta lo slot del caso"), Azione->Def.Slot == C.Slot);
		Test.TestTrue(TEXT("premessa: TakesMainSlot lo riconosce per Main e MovementAndMain"),
			URTCatalogLibrary::TakesMainSlot(Azione->Def) == (C.Slot == ERTActionSlot::Main || C.Slot == ERTActionSlot::MovementAndMain));
		Test.TestEqual(TEXT("premessa: la ricarica prima del turno e' quella del caso"), Runner->GetAbilityCooldown(Scatto),
			C.bGiaInRicarica ? StunMobCooldown - 1 : 0);

		if (Compagno)
		{
			NeutralizeAllIntents(Compagno);
			const int32 ScattoCompagno = RTAbilityFixtures::AddCoreAbilityInSlot(Compagno, TEXT("Action.Dodge"), 3);
			if (!Test.TestTrue(TEXT("premessa: il compagno ha il suo scatto"), ScattoCompagno != INDEX_NONE))
			{
				DestroyFallWorld(World);
				return Esito;
			}
			Compagno->SetPlannedDash(ScattoCompagno, StunMobMateGoal);
			Test.TestTrue(TEXT("premessa: lo scatto del compagno si applica"), Compagno->PlannedDashApplies());
			// L'ordine si MISURA con la funzione vera, su un array passato nell'ordine sbagliato perche' il sort debba muoverlo.
			TArray<ARTUnit*> Ordine = { Compagno, Runner, Foe };
			URTActionQueueLibrary::SortUnitsForResolution(Ordine);
			Esito.bCompagnoDopo = Ordine.IndexOfByKey(Compagno) > Ordine.IndexOfByKey(Runner);
		}

		RunFallTurn(TM);

		Esito.Arrivo = Runner->Cell;
		Esito.Ricarica = Runner->GetAbilityCooldown(Scatto);
		if (Compagno) { Esito.ArrivoCompagno = Compagno->Cell; }
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category != ERTLogCategory::Fallback || E.UnitId != Runner->StableUnitId || E.Phase != ERTMatchPhase::Dash
				|| E.Outcome != static_cast<uint8>(ERTFallbackOutcome::Cancelled))
			{
				continue;
			}
			if (E.Amount == static_cast<int32>(ERTActionInvalidReason::Stunned) && E.ActionId == AzioneAttesa)
			{
				++Esito.Rifiuti;
				if (E.SrcCell == Partenza && E.TgtCell == Partenza) { ++Esito.RifiutiDaFermo; }
			}
			if (E.Amount == static_cast<int32>(ERTActionInvalidReason::Unbalanced))
			{
				++Esito.RifiutiUnbalanced;
			}
		}
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::AbilityActivated && Ev.SourceStableUnitId == Runner->StableUnitId)
			{
				++Esito.Attivazioni;
			}
		}
		Esito.bValido = true;
		DestroyFallWorld(World);
		return Esito;
	}
}

/**
 * **Lo stordimento NEGA la mobilita' qualunque slot occupi, e il rifiuto PAGA** ([D-567], `#3964`, esteso da [D-569], `#3973`).
 *
 * [D-416] (1) toglie la principale alla stordita; [D-567] (2026-11-02, risposta `a`) aveva deciso che una mobilita'
 * `MovementAndMain` — che prende anche la principale — non si compie, che la voce `Fallback`/`Cancelled`/`Stunned` lo dice in
 * fase `Dash` e che il rifiuto paga come ogni principale negata ([D-563] (2)). ⌫ *Lasciava permesso tutto cio' che prende il
 * solo movimento, e questo test lo pinnava nel braccio CONFINE.* [D-569] lo ribalta: una stordita non compie nessun movimento
 * volontario, e il braccio CONFINE ora asserisce il contrario (la mobilita' di solo movimento e' negata e paga come le altre).
 * Il criterio non e' piu' lo slot: i bracci `MovementAndMain` e `Main` restano perche' il ramo deve valere per ogni slot, ma
 * l'asserto che li separava dal solo movimento e' caduto. Il test per ogni mobilita' dei cataloghi e'
 * `Status.StunCensus.EveryMobilityIsRefusedWhenStunned`.
 *
 * 🔴 **Bracci, perche' l'asserzione centrale e' negativa**: «non si e' mosso» e «non ha attivato» sono vere anche per
 * una fixture che non permette lo scatto. Ognuno chiude un modo di sbagliare la guardia:
 *  - CONTROLLO: la stessa mobilita' `MovementAndMain` da una unita' NON stordita arriva, attiva e paga;
 *  - CUORE: la stessa, stordita, resta dov'e', lascia UNA voce di rifiuto in fase Dash con le due celle uguali alla sua,
 *    non attiva e paga; un COMPAGNO non stordito, ordinato DOPO di lei, arriva comunque (la guardia passa all'unita'
 *    successiva, non esce dal ciclo);
 *  - SOLO MOVIMENTO (era il CONFINE di [D-567]): stordita con lo slot `Movement` del catalogo, e' negata come le altre ([D-569]);
 *  - PRINCIPALE DI FASE DASH: lo stesso con lo slot `Main` — il criterio non e' lo slot;
 *  - GIA' IN RICARICA: stordita, ma lo scatto non si applica (`PlannedDashApplies()` falso): nessuna voce e nessun
 *    secondo addebito, perche' `ConsumeAbility` scrive un valore FISSO e la riporterebbe al valore pieno;
 *  - STORDITA E SBILANCIATA, con la mobilita' a budget che `Unbalanced` nega: vince lo stordimento (una voce `Stunned`,
 *    nessuna `Unbalanced`, e paga: l'altro rifiuto non paga);
 *  - ALTRO STATO: sbilanciata ma NON stordita, con lo slancio lineare: arriva, perche' la guardia legge lo stordimento e non
 *    `Unbalanced`, l'altro stato di controllo che ha un rifiuto proprio (Slow, Prone e Root non sono provati).
 *
 * ⚠️ `ConsumeAbility` scrive un valore FISSO: uno o due addebiti lasciano lo stesso residuo, quindi il test distingue «ha
 * pagato» da «non ha pagato», non quante volte. ⚠️ **Il residuo e' letto a fine turno**: il Cleanup decrementa di uno.
 *
 * ⚠️ **Anteprima e mira** leggono `PlannedDashMoves()` ([D-471]): CONTROLLO, CUORE, SOLO MOVIMENTO, PRINCIPALE e ALTRO STATO ne
 * asseriscono il valore PRIMA del turno, perche' un ghost o una mira che partissero da uno scatto negato mentirebbero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStunDeniesAMobilityThatAlsoCostsTheMainActionTest,
	"RefactorTactics.Status.StunDeniesAMobilityThatAlsoCostsTheMainAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStunDeniesAMobilityThatAlsoCostsTheMainActionTest::RunTest(const FString&)
{
	const FRTCellId Partenza = StunMobStart;
	const FRTCellId Meta = StunMobGoal;
	const FRTCellId MetaCompagno = StunMobMateGoal;
	const int32 Pagato = StunMobCooldown - 1;       // un addebito e il decremento del Cleanup
	const int32 NonRipagato = StunMobCooldown - 2;  // era gia' a `StunMobCooldown - 1`, e scende di uno

	// CONTROLLO — non stordita: lo scatto parte, attiva e paga.
	FStunMobCase Caso;
	const FStunMobOutcome Libera = RunStunMobTurn(*this, Caso);
	if (!TestTrue(TEXT("controllo: il turno e' girato"), Libera.bValido)) { return false; }
	TestTrue(TEXT("controllo: lo scatto si applica"), Libera.bApplicaPrima);
	TestTrue(TEXT("controllo: e sposta (anteprima e mira)"), Libera.bSpostaPrima);
	TestEqual(TEXT("controllo: arriva a destinazione"), Libera.Arrivo, Meta);
	TestEqual(TEXT("controllo: nessun rifiuto"), Libera.Rifiuti, 0);
	TestTrue(TEXT("controllo: attiva l'azione (il sensore delle attivazioni vede)"), Libera.Attivazioni >= 1);
	TestEqual(TEXT("controllo: la ricarica e' pagata"), Libera.Ricarica, Pagato);

	// IL CUORE — stordita: la mobilita' che prende la principale e' negata, e paga. Un compagno ordinato dopo di lei arriva.
	Caso.bStordita = true;
	Caso.bCompagno = true;
	const FStunMobOutcome Stordita = RunStunMobTurn(*this, Caso);
	if (!TestTrue(TEXT("cuore: il turno e' girato"), Stordita.bValido)) { return false; }
	TestTrue(TEXT("premessa: per la regola del catalogo lo scatto si applica ancora"), Stordita.bApplicaPrima);
	TestFalse(TEXT("ma lo stordimento lo nega: lo scatto non sposta (anteprima e mira)"), Stordita.bSpostaPrima);
	TestEqual(TEXT("la stordita resta dov'e'"), Stordita.Arrivo, Partenza);
	TestEqual(TEXT("il TurnLog dice perche': UNA voce Fallback/Cancelled/Stunned in fase Dash che nomina l'azione"),
		Stordita.Rifiuti, 1);
	TestEqual(TEXT("la voce non rivela la meta pianificata: le sue due celle sono quella di partenza"),
		Stordita.RifiutiDaFermo, 1);
	TestEqual(TEXT("e l'azione non si attiva"), Stordita.Attivazioni, 0);
	TestEqual(TEXT("il rifiuto PAGA la ricarica"), Stordita.Ricarica, Pagato);
	TestTrue(TEXT("premessa: nell'ordine reale di raccolta il compagno viene DOPO la stordita"), Stordita.bCompagnoDopo);
	TestEqual(TEXT("il compagno ordinato dopo di lei arriva: la guardia passa all'unita' successiva"),
		Stordita.ArrivoCompagno, MetaCompagno);

	// SOLO MOVIMENTO — stordita, con la mobilita' che prende il SOLO movimento (lo slot del catalogo, di ogni scatto spedito).
	// ⌫ Fino a [D-569] questo era il CONFINE della regola di [D-567]: D-416 (1) lasciava il movimento a `Root` e la stordita
	// arrivava, senza rifiuti e pagando. Ora e' negata come tutte le altre: l'asserzione e' rovesciata, non indebolita.
	FStunMobCase SoloMovimento;
	SoloMovimento.Slot = ERTActionSlot::Movement;
	SoloMovimento.bStordita = true;
	const FStunMobOutcome Confine = RunStunMobTurn(*this, SoloMovimento);
	if (!TestTrue(TEXT("solo movimento: il turno e' girato"), Confine.bValido)) { return false; }
	TestTrue(TEXT("solo movimento: per la regola del catalogo lo scatto si applica ancora"), Confine.bApplicaPrima);
	TestFalse(TEXT("solo movimento: ma lo stordimento lo nega (anteprima e mira)"), Confine.bSpostaPrima);
	TestEqual(TEXT("solo movimento: la stordita resta dov'e'"), Confine.Arrivo, Partenza);
	TestEqual(TEXT("solo movimento: UNA voce di rifiuto in fase Dash"), Confine.Rifiuti, 1);
	TestEqual(TEXT("solo movimento: con le due celle uguali alla sua"), Confine.RifiutiDaFermo, 1);
	TestEqual(TEXT("solo movimento: l'azione non si attiva"), Confine.Attivazioni, 0);
	TestEqual(TEXT("solo movimento: il rifiuto paga la ricarica"), Confine.Ricarica, Pagato);

	// PRINCIPALE DI FASE DASH — lo slot `Main` con la macro-fase Dash: il criterio e' `TakesMainSlot`, non `MovementAndMain`.
	FStunMobCase Principale;
	Principale.Slot = ERTActionSlot::Main;
	const FStunMobOutcome PrincipaleLibera = RunStunMobTurn(*this, Principale);
	if (!TestTrue(TEXT("principale di fase Dash, controllo: il turno e' girato"), PrincipaleLibera.bValido)) { return false; }
	TestEqual(TEXT("principale di fase Dash, controllo: non stordita arriva"), PrincipaleLibera.Arrivo, Meta);
	TestEqual(TEXT("principale di fase Dash, controllo: nessun rifiuto"), PrincipaleLibera.Rifiuti, 0);
	Principale.bStordita = true;
	const FStunMobOutcome PrincipaleStordita = RunStunMobTurn(*this, Principale);
	if (!TestTrue(TEXT("principale di fase Dash: il turno e' girato"), PrincipaleStordita.bValido)) { return false; }
	TestFalse(TEXT("principale di fase Dash: lo stordimento la nega (anteprima e mira)"), PrincipaleStordita.bSpostaPrima);
	TestEqual(TEXT("principale di fase Dash: la stordita resta dov'e'"), PrincipaleStordita.Arrivo, Partenza);
	TestEqual(TEXT("principale di fase Dash: UNA voce di rifiuto"), PrincipaleStordita.Rifiuti, 1);
	TestEqual(TEXT("principale di fase Dash: l'azione non si attiva"), PrincipaleStordita.Attivazioni, 0);
	TestEqual(TEXT("principale di fase Dash: il rifiuto paga"), PrincipaleStordita.Ricarica, Pagato);

	// GIA' IN RICARICA — lo scatto non si applica: la guardia sta DOPO `if (!bDashApplies)`, nessun rifiuto e nessun addebito.
	FStunMobCase InRicarica;
	InRicarica.bStordita = true;
	InRicarica.bGiaInRicarica = true;
	const FStunMobOutcome Ricarica = RunStunMobTurn(*this, InRicarica);
	if (!TestTrue(TEXT("gia' in ricarica: il turno e' girato"), Ricarica.bValido)) { return false; }
	TestFalse(TEXT("gia' in ricarica: lo scatto non si applica"), Ricarica.bApplicaPrima);
	TestEqual(TEXT("gia' in ricarica: la stordita resta dov'e'"), Ricarica.Arrivo, Partenza);
	TestEqual(TEXT("gia' in ricarica: nessuna voce di rifiuto"), Ricarica.Rifiuti, 0);
	TestEqual(TEXT("gia' in ricarica: l'azione non si attiva"), Ricarica.Attivazioni, 0);
	TestEqual(TEXT("gia' in ricarica: nessun secondo addebito, la ricarica scende di uno"), Ricarica.Ricarica, NonRipagato);

	// STORDITA E SBILANCIATA — la mobilita' a budget che `Unbalanced` nega: vince lo stordimento, e paga.
	FStunMobCase Entrambi;
	Entrambi.bStordita = true;
	Entrambi.bSbilanciata = true;
	Entrambi.bABudget = true;
	const FStunMobOutcome StorditaSbilanciata = RunStunMobTurn(*this, Entrambi);
	if (!TestTrue(TEXT("stordita e sbilanciata: il turno e' girato"), StorditaSbilanciata.bValido)) { return false; }
	TestTrue(TEXT("stordita e sbilanciata: lo scatto si applica per la regola del catalogo"), StorditaSbilanciata.bApplicaPrima);
	TestEqual(TEXT("stordita e sbilanciata: resta dov'e'"), StorditaSbilanciata.Arrivo, Partenza);
	TestEqual(TEXT("stordita e sbilanciata: la voce e' quella dello stordimento"), StorditaSbilanciata.Rifiuti, 1);
	TestEqual(TEXT("stordita e sbilanciata: e non c'e' quella di Unbalanced"), StorditaSbilanciata.RifiutiUnbalanced, 0);
	TestEqual(TEXT("stordita e sbilanciata: lo stordimento paga"), StorditaSbilanciata.Ricarica, Pagato);

	// ALTRO STATO — sbilanciata ma NON stordita, con lo slancio lineare che `Unbalanced` non nega: arriva.
	FStunMobCase SoloSbilanciata;
	SoloSbilanciata.bSbilanciata = true;
	const FStunMobOutcome Sbilanciata = RunStunMobTurn(*this, SoloSbilanciata);
	if (!TestTrue(TEXT("altro stato: il turno e' girato"), Sbilanciata.bValido)) { return false; }
	TestTrue(TEXT("altro stato: lo scatto sposta (anteprima e mira)"), Sbilanciata.bSpostaPrima);
	TestEqual(TEXT("altro stato: la sbilanciata non stordita arriva"), Sbilanciata.Arrivo, Meta);
	TestEqual(TEXT("altro stato: nessun rifiuto per stordimento"), Sbilanciata.Rifiuti, 0);
	TestEqual(TEXT("altro stato: e nessuno per Unbalanced"), Sbilanciata.RifiutiUnbalanced, 0);
	TestEqual(TEXT("altro stato: la ricarica e' pagata"), Sbilanciata.Ricarica, Pagato);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
