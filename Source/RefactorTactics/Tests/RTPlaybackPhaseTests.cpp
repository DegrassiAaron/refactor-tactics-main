#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Core/RTTypes.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnRules.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Mondo per il playback (nomi distinti per file: in unity build i test condividono la translation unit). */
	UWorld* MakePlaybackWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyPlaybackWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		if (GEngine)
		{
			GEngine->DestroyWorldContext(World);
		}
		World->DestroyWorld(false);
	}

	ARTUnit* SpawnPlaybackUnit(UWorld* World, int32 TeamId, ERTArchetype Arch, const FRTGridCoord& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // i piani li scriviamo noi: niente decisioni del bot in mezzo
		U->ConfigureAsArchetype(Arch);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 200.f);
		return U;
	}

	/** Avanza il playback finche' la fase riprodotta non e' quella attesa (o finche' non finisce). */
	bool AdvanceUntilPhase(ARTTurnManager* TM, const FString& PhaseName, int32 MaxSteps = 400)
	{
		for (int32 I = 0; I < MaxSteps && TM->IsResolving(); ++I)
		{
			if (TM->GetPlaybackPhaseName() == PhaseName)
			{
				return true;
			}
			TM->Tick(0.05f);
		}
		return TM->IsResolving() && TM->GetPlaybackPhaseName() == PhaseName;
	}

	void AdvanceUntilDone(ARTTurnManager* TM, int32 MaxSteps = 400)
	{
		for (int32 I = 0; I < MaxSteps && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}
}

/**
 * Il flag di corsa (letto dall'AnimBP) deve valere solo per la fase in cui l'unita' si muove davvero.
 * Regressione osservata in PIE: chi scattava nel Dash restava "in corsa" per tutto il Blast, perche' il flag
 * veniva spento solo a fine risoluzione. Qui si programma il turno invece di guardarlo: piani espliciti,
 * risoluzione, e il playback avanzato a mano con Tick.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackMovingFlagPhaseTest,
	"RefactorTactics.Playback.MovingFlagClearedOnPhaseChange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackMovingFlagPhaseTest::RunTest(const FString&)
{
	UWorld* World = MakePlaybackWorld();
	TestNotNull(TEXT("World creato"), World);
	if (!World) { return false; }

	// Due unita' vicine: il Guardian scatta, il Ranger avversario resta fermo e fa da bersaglio.
	ARTUnit* Dasher = SpawnPlaybackUnit(World, 0, ERTArchetype::Guardian, FRTGridCoord(2, 2));
	ARTUnit* Target = SpawnPlaybackUnit(World, 1, ERTArchetype::Ranger, FRTGridCoord(4, 2));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	TestNotNull(TEXT("TurnManager spawnato"), TM);
	if (!TM || !Dasher || !Target) { DestroyPlaybackWorld(World); return false; }

	// Piano: scatto verso una cella adiacente + attacco, cosi' la risoluzione ha sia Dash sia Blast.
	const int32 DashIdx = Dasher->FindDashAbilityIndex();
	TestTrue(TEXT("il Guardian ha un'abilita' di scatto"), DashIdx != INDEX_NONE);
	if (DashIdx == INDEX_NONE) { DestroyPlaybackWorld(World); return false; }

	Dasher->PlannedDashAbility = DashIdx;
	Dasher->PlannedDashCell = FRTGridCoord(3, 2);
	Dasher->PlannedAbilityIndex = 0;          // attacco base
	Dasher->PlannedAttackTarget = Target;

	TM->LockInAndResolve();

	// Durante il Dash chi scatta risulta in corsa...
	if (AdvanceUntilPhase(TM, TEXT("Dash")))
	{
		TestTrue(TEXT("nel Dash l'unita' che scatta e' in corsa"), Dasher->bIsMovingVisually);
	}
	else
	{
		AddWarning(TEXT("fase Dash non riprodotta: scenario non esercitato come previsto"));
	}

	// ...e appena si passa al Blast NON deve piu' esserlo (era il difetto).
	if (AdvanceUntilPhase(TM, TEXT("Blast")))
	{
		TestFalse(TEXT("nel Blast l'unita' non e' piu' in corsa"), Dasher->bIsMovingVisually);
	}
	else
	{
		AddWarning(TEXT("fase Blast non riprodotta: scenario non esercitato come previsto"));
	}

	// A risoluzione conclusa nessuno resta in corsa.
	AdvanceUntilDone(TM);
	TestFalse(TEXT("a fine risoluzione nessuna corsa residua (attaccante)"), Dasher->bIsMovingVisually);
	TestFalse(TEXT("a fine risoluzione nessuna corsa residua (bersaglio)"), Target->bIsMovingVisually);

	DestroyPlaybackWorld(World);
	return true;
}

/**
 * Un'unita' che non si muove non deve mai risultare in corsa, in nessuna fase: l'AnimBP la terrebbe a correre
 * sul posto pur restando ferma.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackStillUnitNeverRunsTest,
	"RefactorTactics.Playback.StillUnitNeverRuns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackStillUnitNeverRunsTest::RunTest(const FString&)
{
	UWorld* World = MakePlaybackWorld();
	TestNotNull(TEXT("World creato"), World);
	if (!World) { return false; }

	ARTUnit* Mover = SpawnPlaybackUnit(World, 0, ERTArchetype::Ranger, FRTGridCoord(2, 2));
	ARTUnit* Still = SpawnPlaybackUnit(World, 0, ERTArchetype::Guardian, FRTGridCoord(6, 6));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover || !Still) { DestroyPlaybackWorld(World); return false; }

	// Solo Mover ha un piano di movimento; Still resta dov'e'.
	Mover->PlannedCell = FRTGridCoord(4, 2);
	TM->LockInAndResolve();

	bool bStillEverRan = false;
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
	{
		bStillEverRan |= Still->bIsMovingVisually;
		TM->Tick(0.05f);
	}
	bStillEverRan |= Still->bIsMovingVisually;

	TestFalse(TEXT("l'unita' ferma non risulta mai in corsa"), bStillEverRan);
	TestFalse(TEXT("nessuna corsa residua a fine risoluzione"), Mover->bIsMovingVisually);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
