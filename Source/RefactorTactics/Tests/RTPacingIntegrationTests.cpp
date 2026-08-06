#include "Misc/AutomationTest.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTPacing.h"
#include "Turn/RTTurnRules.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Telemetria di pacing con un mondo vero. Helper locali come in RTHexBotIntegrationTests.cpp e
 * RTHexCombatIntegrationTests.cpp: ogni file d'integrazione tiene i propri, in namespace anonimo.
 */
namespace
{
	UWorld* MakeHexPacingWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyHexPacingWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	void SpawnHexPacingMap(UWorld* World, int32 Radius)
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

	ARTUnit* SpawnHexPacingUnit(UWorld* World, int32 TeamId, ERTArchetype Arch, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureAsArchetype(Arch);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = true;
		U->DispatchBeginPlay(); // senza, i cooldown restano vuoti e ogni abilita' risulta sempre pronta
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/**
	 * TurnManager pronto a giocare. DispatchBeginPlay e' NECESSARIO: e' BeginPlay a chiamare
	 * StartPlanningTimer, che apre il campione di pacing del PRIMO turno. Senza, i campioni sarebbero
	 * sistematicamente uno in meno dei turni giocati.
	 */
	ARTTurnManager* SpawnHexPacingTurnManager(UWorld* World)
	{
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (TM) { TM->DispatchBeginPlay(); }
		return TM;
	}

	/**
	 * Nome con suffisso `Pacing` e non `PlayOneTurn`: UE compila piu' .cpp in un'unica unita' di
	 * traduzione (unity build), quindi due helper omonimi in namespace anonimo di file diversi
	 * collidono appena il raggruppamento li mette insieme. `RTHexMatchIntegrationTests.cpp` ha gia' un
	 * `PlayOneTurn`, e la collisione compare o sparisce al cambiare dei file del modulo: un build verde
	 * non e' garanzia.
	 */
	void PlayOnePacingTurn(ARTTurnManager* TM)
	{
		TM->PlanBotsForTest();
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingSamplePerTurnTest,
	"RefactorTactics.Pacing.EveryTurnProducesOneSample",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingSamplePerTurnTest::RunTest(const FString&)
{
	UWorld* World = MakeHexPacingWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexPacingMap(World, /*Radius=*/ 5);

	ARTUnit* A1 = SpawnHexPacingUnit(World, 0, ERTArchetype::Ranger,   FRTCellId(-4, 2));
	ARTUnit* A2 = SpawnHexPacingUnit(World, 0, ERTArchetype::Guardian, FRTCellId(-4, 3));
	ARTUnit* B1 = SpawnHexPacingUnit(World, 1, ERTArchetype::Ranger,   FRTCellId(4, -2));
	ARTUnit* B2 = SpawnHexPacingUnit(World, 1, ERTArchetype::Guardian, FRTCellId(4, -3));
	ARTTurnManager* TM = SpawnHexPacingTurnManager(World);
	if (!TM || !A1 || !A2 || !B1 || !B2) { DestroyHexPacingWorld(World); return false; }

	int32 TurnsPlayed = 0;
	while (TM->GetPhase() != ERTMatchPhase::MatchEnded && TurnsPlayed < 40)
	{
		PlayOnePacingTurn(TM);
		++TurnsPlayed;
	}

	// Un campione per turno, ULTIMO COMPRESO. Il turno decisivo passa da ConcludeTurn, che pero' esce
	// prima di incrementare TurnNumber quando la partita finisce: se il campione si chiudesse dopo quel
	// ritorno anticipato, l'unico turno che decide la partita non verrebbe mai misurato.
	TestEqual(TEXT("un campione per turno giocato"), TM->GetPacingSamples().Num(), TurnsPlayed);
	TestTrue(TEXT("la partita si e' decisa"), TM->GetPhase() == ERTMatchPhase::MatchEnded);

	// I numeri di turno sono progressivi da 1: nessun buco, nessun doppione.
	for (int32 I = 0; I < TM->GetPacingSamples().Num(); ++I)
	{
		TestEqual(FString::Printf(TEXT("campione %d e' del turno %d"), I, I + 1),
			TM->GetPacingSamples()[I].TurnNumber, I + 1);
	}

	DestroyHexPacingWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingCompositionTest,
	"RefactorTactics.Pacing.RecordsDecisionComposition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingCompositionTest::RunTest(const FString&)
{
	UWorld* World = MakeHexPacingWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexPacingMap(World, /*Radius=*/ 3);

	ARTUnit* A1 = SpawnHexPacingUnit(World, 0, ERTArchetype::Ranger,   FRTCellId(-2, 1));
	ARTUnit* B1 = SpawnHexPacingUnit(World, 1, ERTArchetype::Guardian, FRTCellId(2, -1));
	ARTTurnManager* TM = SpawnHexPacingTurnManager(World);
	if (!TM || !A1 || !B1) { DestroyHexPacingWorld(World); return false; }

	// Il turno 1 e' in pianificazione: simuliamo la mano del giocatore senza passare dal controller.
	TM->RecordPlanningInput(ERTPlanningInput::Selection);
	TM->RecordPlanningInput(ERTPlanningInput::Order);
	TM->RecordPlanningInput(ERTPlanningInput::Order);
	TM->RecordPlanningInput(ERTPlanningInput::Undo);
	TM->RecordPlanningInput(ERTPlanningInput::Click); // attivita' generica: non incrementa nessun contatore

	PlayOnePacingTurn(TM);

	if (!TestTrue(TEXT("almeno un campione"), TM->GetPacingSamples().Num() >= 1))
	{
		DestroyHexPacingWorld(World);
		return false;
	}
	const FRTPacingSample& S = TM->GetPacingSamples()[0];
	TestEqual(TEXT("una selezione"), S.SelectionCount, 1);
	TestEqual(TEXT("due ordini"), S.OrderCount, 2);
	TestEqual(TEXT("un annullamento"), S.UndoCount, 1);
	TestTrue(TEXT("il lock-in manuale non e' un timeout"), S.LockInSource == ERTLockInSource::Input);
	TestEqual(TEXT("due unita' vive, una per squadra"), S.UnitsAliveTeam0 + S.UnitsAliveTeam1, 2);

	DestroyHexPacingWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingHashInvarianceTest,
	"RefactorTactics.Pacing.DoesNotAffectTurnLogHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingHashInvarianceTest::RunTest(const FString&)
{
	// La stessa partita giocata due volte, con la sonda spenta e accesa: gli esiti autoritativi devono
	// essere IDENTICI hash per hash. E' la dimostrazione eseguibile dell'invariante: la telemetria non ha
	// ritorno verso il gameplay. Oggi passa quasi per costruzione; serve per quando qualcuno sara' tentato
	// di far leggere un tempo di parete a una decisione.
	auto PlayMatchHashes = [this](bool bRecord, TArray<uint32>& OutHashes) -> bool
	{
		UWorld* World = MakeHexPacingWorld();
		if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
		SpawnHexPacingMap(World, /*Radius=*/ 5);

		ARTUnit* A1 = SpawnHexPacingUnit(World, 0, ERTArchetype::Ranger,   FRTCellId(-4, 2));
		ARTUnit* A2 = SpawnHexPacingUnit(World, 0, ERTArchetype::Guardian, FRTCellId(-4, 3));
		ARTUnit* B1 = SpawnHexPacingUnit(World, 1, ERTArchetype::Ranger,   FRTCellId(4, -2));
		ARTUnit* B2 = SpawnHexPacingUnit(World, 1, ERTArchetype::Guardian, FRTCellId(4, -3));
		ARTTurnManager* TM = SpawnHexPacingTurnManager(World);
		if (!TM || !A1 || !A2 || !B1 || !B2) { DestroyHexPacingWorld(World); return false; }

		TM->bRecordPacing = bRecord;
		int32 TurnsPlayed = 0;
		while (TM->GetPhase() != ERTMatchPhase::MatchEnded && TurnsPlayed < 40)
		{
			PlayOnePacingTurn(TM);
			++TurnsPlayed;
			OutHashes.Add(URTTurnLogLibrary::HashTurnLog(TM->GetTurnLog()));
		}
		DestroyHexPacingWorld(World);
		return true;
	};

	TArray<uint32> Off;
	TArray<uint32> On;
	if (!PlayMatchHashes(/*bRecord=*/ false, Off)) { return false; }
	if (!PlayMatchHashes(/*bRecord=*/ true,  On))  { return false; }

	TestEqual(TEXT("stesso numero di turni con sonda accesa e spenta"), On.Num(), Off.Num());
	const int32 Count = FMath::Min(On.Num(), Off.Num());
	for (int32 I = 0; I < Count; ++I)
	{
		// TestTrue e non TestEqual: gli hash sono uint32 e le overload di TestEqual sono ambigue su quel tipo.
		TestTrue(FString::Printf(TEXT("turno %d: hash del TurnLog identico"), I + 1), On[I] == Off[I]);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
