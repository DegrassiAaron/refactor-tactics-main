#include "ScenarioHarness/RTScenarioCoordinator.h"

#include "RefactorTactics.h" // LogRT
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTScenarioSession.h"
#include "ScenarioHarness/RTTestReportWriter.h"
#include "ScenarioHarness/RTTestResult.h"
#include "ScenarioHarness/RTTestScenario.h"

ERTScenarioStart FRTScenarioCoordinator::Start(UWorld* World, const FString& ScenarioId,
	const FString& SourceLabel, float InTurnPauseSeconds)
{
	if (ScenarioId.IsEmpty())
	{
		// Nessuna richiesta: non si tocca niente, nemmeno il log. Una riga qui comparirebbe a ogni avvio di
		// ogni partita normale, cioe' sarebbe rumore su cio' che non e' successo.
		return ERTScenarioStart::NotRequested;
	}

	if (!World)
	{
		UE_LOG(LogRT, Error, TEXT("[RT-Test] scenario '%s' non avviabile: nessun mondo."), *ScenarioId);
		return ERTScenarioStart::NotLoadable;
	}

	FString ScenarioError;
	FRTTestScenario Scenario;
	const FString ScenarioPath = URTScenarioIndex::ResolvePath(ScenarioId, ScenarioError);
	if (ScenarioPath.IsEmpty() || !URTScenarioLoader::LoadFromFile(ScenarioPath, Scenario, ScenarioError))
	{
		UE_LOG(LogRT, Error, TEXT("[RT-Test] scenario '%s' non caricabile: %s"), *ScenarioId, *ScenarioError);
		return ERTScenarioStart::NotLoadable;
	}

	// La FONTE va dichiarata sempre, non solo quando c'e' conflitto: chi legge il log deve poter dire
	// «sta girando quello che ho scelto io» senza dedurlo dal comportamento a schermo.
	//
	// ⚠️ Un free-run non enumera turni: `Turns.Num()` li' vale **zero**, e questa riga annuncerebbe «0 turni»
	// un istante prima di giocarne dieci. La riga esiste perche' chi guarda possa dire «sta girando quello
	// che ho scelto io» senza dedurlo dallo schermo, quindi dire il falso la rende peggio che assente.
	const int32 TurniAnnunciati = Scenario.bFreeRun ? Scenario.MaxTurns : Scenario.Turns.Num();
	UE_LOG(LogRT, Warning, TEXT("[RT-Test] AUTO-RUN %s (da: %s): %s%d turni, pausa %.1fs — avanza un passo per frame"),
		*ScenarioId, *SourceLabel, Scenario.bFreeRun ? TEXT("free-run, tetto ") : TEXT(""),
		TurniAnnunciati, InTurnPauseSeconds);

	// La sessione parte QUI ma avanza in `Tick`, un passo per frame: e' cio' che rende lo scenario
	// osservabile. Risolvendo tutto dentro `BeginPlay` finiva prima del primo fotogramma, e quel che si
	// vedeva muoversi erano turni fantasma — misurato in PIE, non supposto.
	Session = MakeShared<FRTScenarioSession>();
	Session->TurnPauseSeconds = InTurnPauseSeconds;
	if (!Session->Start(World, Scenario))
	{
		UE_LOG(LogRT, Error, TEXT("[RT-Test] %s -> ERROR: %s"),
			*ScenarioId, *Session->GetResult().ErrorMessage);
	}

	// ⚠️ `Started` anche quando `Start` ha fallito, ed e' deliberato: la sessione ESISTE, il suo esito e'
	// gia' un `Error` leggibile, e la partita normale non deve essere allestita al suo posto.
	return ERTScenarioStart::Started;
}

void FRTScenarioCoordinator::Tick(float DeltaSeconds)
{
	if (!Session.IsValid() || Session->IsFinished())
	{
		return;
	}

	// `bPumpTurnManager = false`: qui il mondo ticca gia' il turn manager. Pomparlo anche da qui lo farebbe
	// correre al doppio della velocita', e il playback che si vuole GUARDARE passerebbe in meta' del tempo.
	Session->Step(DeltaSeconds, /*bPumpTurnManager=*/ false);

	if (!Session->IsFinished())
	{
		return;
	}

	const FRTTestResult& Result = Session->GetResult();
	FString ReportDir, WriteError;
	if (!URTTestReportWriter::Write(Result, FString(), ReportDir, WriteError))
	{
		UE_LOG(LogRT, Error, TEXT("[RT-Test] report non scritto: %s"), *WriteError);
	}
	UE_LOG(LogRT, Warning, TEXT("[RT-Test] FINITO %s -> %s (%d/%d assertion, %d turni) · report: %s"),
		*Result.ScenarioId, *Result.OutcomeString(), Result.PassedCount(), Result.Assertions.Num(),
		Result.TurnsPlayed, ReportDir.IsEmpty() ? TEXT("non scritto") : *ReportDir);

	for (const FRTAssertionResult& A : Result.Assertions)
	{
		if (!A.bPassed)
		{
			UE_LOG(LogRT, Error, TEXT("[RT-Test]   FALLITA %s: atteso %s, ottenuto %s"),
				*A.Description, *A.Expected, *A.Actual);
		}
	}
}

bool FRTScenarioCoordinator::IsRunning() const
{
	return Session.IsValid() && !Session->IsFinished();
}

FString FRTScenarioCoordinator::OutcomeString() const
{
	if (!Session.IsValid() || !Session->IsFinished())
	{
		return FString();
	}
	return Session->GetResult().OutcomeString();
}
