#include "PieSession/RTPieSessionSubsystem.h"

#include "PieSession/RTPieSessionWriter.h"
#include "Misc/Paths.h"
#include "RefactorTactics.h" // LogRT
#include "ScenarioHarness/RTTestResult.h"

FString URTPieSessionSubsystem::ScenarioImposedBy(const URTPieSessionSubsystem* Conduttore)
{
	if (!Conduttore || !Conduttore->IsConducting())
	{
		return FString();
	}
	const FRTPieSessionStep* Passo = Conduttore->CurrentStep();
	return Passo ? Passo->ScenarioId : FString();
}

void URTPieSessionSubsystem::Begin(TArray<FRTPieSessionStep> InSteps, FRTPieSessionPorts InPorts)
{
	SetPorts(MoveTemp(InPorts));
	Begin(MoveTemp(InSteps));
}

void URTPieSessionSubsystem::Deinitialize()
{
	// 🔴 **Lo Stop di PIE distrugge la GameInstance, ed e' il modo piu' naturale di chiudere una seduta
	// a meta'.** Senza questo, i verdetti gia' dati morivano con lei e nessun file veniva scritto —
	// l'opposto di cio' che `Abort` promette. Trovato in code review il 2026-09-20.
	//
	// ⛔ Non si toccano le porte: il mondo se ne sta andando, e chiamare `TearDown` su un GameMode in
	// distruzione e' esattamente il difetto che il teardown esiste per evitare.
	if (IsConducting())
	{
		for (int32 I = FMath::Max(Cursor, 0); I < SessionSteps.Num(); ++I)
		{
			if (SessionSteps[I].Verdict == ERTPieVerdict::Pending)
			{
				SessionSteps[I].Verdict = ERTPieVerdict::NotRun;
				SessionSteps[I].Reason = TEXT("PIE chiusa prima della fine della seduta");
				SessionSteps[I].Source = ERTPieVerdictSource::Conductor;
				SessionSteps[I].At = FDateTime::UtcNow();
			}
		}
		Ports = FRTPieSessionPorts();
		Finish();
	}

	Super::Deinitialize();
}

void URTPieSessionSubsystem::Begin(TArray<FRTPieSessionStep> InSteps)
{
	// ⛔ Una seconda `Begin` a seduta aperta butterebbe via i verdetti gia' dati senza scriverli.
	if (IsConducting())
	{
		UE_LOG(LogRT, Error,
			TEXT("[RT-Pie] seduta gia' in corso (%s, passo %d/%d): chiudila con rt.Pie.Session.Abort"),
			*CurrentSessionId, Cursor + 1, SessionSteps.Num());
		return;
	}

	if (!Ports.IsValid())
	{
		// Senza porte non si allestisce niente, e aprire una seduta che non puo' lanciare nulla
		// produrrebbe un file di `NotJudgeable` che sembrerebbe un difetto del corpus.
		UE_LOG(LogRT, Error, TEXT("[RT-Pie] nessuna porta installata: la seduta non parte"));
		return;
	}

	SessionSteps = MoveTemp(InSteps);
	CurrentSessionId = URTPieSessionWriter::MakeSessionId(FDateTime::UtcNow());
	SessionFilePath.Reset();
	Cursor = 0;
	SessionState = ERTPieSessionState::Playing;

	if (SessionSteps.Num() == 0)
	{
		UE_LOG(LogRT, Warning, TEXT("[RT-Pie] seduta %s senza passi: niente da condurre"), *CurrentSessionId);
		Finish();
		return;
	}

	UE_LOG(LogRT, Warning, TEXT("[RT-Pie] seduta %s: %d passi"), *CurrentSessionId, SessionSteps.Num());
	LaunchCurrent();
}

void URTPieSessionSubsystem::LaunchCurrent()
{
	while (SessionSteps.IsValidIndex(Cursor))
	{
		SessionState = ERTPieSessionState::Playing;

		FRTPieSessionStep& Step = SessionSteps[Cursor];
		const FRTPieLaunchOutcome Esito = Ports.Launch
			? Ports.Launch(Step.ScenarioId)
			: FRTPieLaunchOutcome::NonCaricabile();

		if (Esito.Start == ERTScenarioStart::Started && !Esito.bFinishedOnArrival)
		{
			UE_LOG(LogRT, Warning, TEXT("[RT-Pie] passo %d/%d — %s da %s"),
				Cursor + 1, SessionSteps.Num(), *Step.PieItem, *Step.ScenarioId);
			return;
		}

		if (Esito.bFinishedOnArrival)
		{
			// 🔴 La sessione e' nata gia' finita: nessun `OnScenarioFinished` arrivera' mai, e aspettarlo
			// significherebbe restare in `Playing` per sempre. Il passo si chiude qui.
			Step.Verdict = ERTPieVerdict::Blocked;
			Step.Reason = Esito.Error.IsEmpty() ? TEXT("la sessione non e' mai partita") : Esito.Error;
			Step.Source = ERTPieVerdictSource::Conductor;
			Step.At = FDateTime::UtcNow();
			UE_LOG(LogRT, Error, TEXT("[RT-Pie] %s -> BLOCCATO: %s"), *Step.PieItem, *Step.Reason);

			if (Ports.TearDown) { Ports.TearDown(); }
			++Cursor;
			continue;
		}

		// ⛔ Non si chiede un giudizio su una scena che non si e' allestita, e non ci si ferma nemmeno:
		// fermarsi qui costringerebbe a riaprire l'Editor per le voci a valle.
		Step.Verdict = ERTPieVerdict::NotJudgeable;
		Step.Reason = FString::Printf(TEXT("scenario non caricabile: %s"), *Step.ScenarioId);
		Step.Source = ERTPieVerdictSource::Conductor;
		Step.At = FDateTime::UtcNow();
		UE_LOG(LogRT, Error, TEXT("[RT-Pie] %s -> NON GIUDICABILE: %s"), *Step.PieItem, *Step.Reason);
		++Cursor;
	}

	Finish();
}

void URTPieSessionSubsystem::OnScenarioFinished(const FRTTestResult& Result, const FString& ReportDir)
{
	if (!SessionSteps.IsValidIndex(Cursor) || SessionState != ERTPieSessionState::Playing)
	{
		return;
	}

	FRTPieSessionStep& Step = SessionSteps[Cursor];
	Step.MachineOutcome = FString::Printf(TEXT("%s %d/%d"), *Result.OutcomeString(),
		Result.PassedCount(), Result.Assertions.Num());

	// `runId` e `reportDir` erano campi morti — il file li scriveva vuoti mentre la spec li mostrava
	// pieni. Sono il ponte verso il referto della run, cioe' esattamente cio' che serve al passo
	// successivo (la propagazione al registro). Trovato in code review il 2026-09-20.
	Step.ReportDir = ReportDir;
	Step.RunId = ReportDir.IsEmpty() ? FString() : FPaths::GetCleanFilename(ReportDir);

	if (!Result.ErrorMessage.IsEmpty())
	{
		// La sessione e' esistita ma non ha mai giocato: chiedere un verdetto sarebbe chiedere di
		// guardare una scena mai partita, e qualunque risposta sarebbe su niente.
		Step.Verdict = ERTPieVerdict::Blocked;
		Step.Reason = Result.ErrorMessage;
		Step.Source = ERTPieVerdictSource::Conductor;
		Step.At = FDateTime::UtcNow();
		UE_LOG(LogRT, Error, TEXT("[RT-Pie] %s -> BLOCCATO: %s"), *Step.PieItem, *Step.Reason);

		if (Ports.TearDown) { Ports.TearDown(); }
		++Cursor;
		LaunchCurrent();
		return;
	}

	// 🔑 `expect` rosse NON chiudono il passo: l'esito macchina e il giudizio di chi guarda sono due
	// cose diverse, e la loro coppia contraddittoria e' informazione, non contraddizione.
	SessionState = ERTPieSessionState::AwaitingVerdict;
	UE_LOG(LogRT, Warning, TEXT("[RT-Pie] %s — macchina: %s. In attesa del verdetto (1 si', 2 no, 3 non giudicabile)"),
		*Step.PieItem, *Step.MachineOutcome);
}

void URTPieSessionSubsystem::SubmitVerdict(ERTPieVerdict Verdict, const FString& Reason)
{
	if (SessionState != ERTPieSessionState::AwaitingVerdict || !SessionSteps.IsValidIndex(Cursor))
	{
		return;
	}

	// ⛔ Da fuori arrivano SOLO i tre verdetti di una persona: `Blocked` e `NotRun` li scrive il
	// conduttore, e `Pending` chiuderebbe un passo con «PENDING» scritto nel file come se fosse un esito.
	if (Verdict != ERTPieVerdict::Pass && Verdict != ERTPieVerdict::Fail
		&& Verdict != ERTPieVerdict::NotJudgeable)
	{
		UE_LOG(LogRT, Error, TEXT("[RT-Pie] %s non e' un verdetto che possa dare una persona"),
			LexToString(Verdict));
		return;
	}

	FRTPieSessionStep& Step = SessionSteps[Cursor];
	Step.Verdict = Verdict;
	Step.Reason = Reason;
	Step.Source = ERTPieVerdictSource::Human;
	Step.At = FDateTime::UtcNow();
	UE_LOG(LogRT, Warning, TEXT("[RT-Pie] %s -> %s"), *Step.PieItem, LexToString(Verdict));

	if (Ports.TearDown) { Ports.TearDown(); }
	++Cursor;
	LaunchCurrent();
}

void URTPieSessionSubsystem::Abort()
{
	if (SessionState == ERTPieSessionState::Done || SessionState == ERTPieSessionState::Idle)
	{
		return;
	}

	for (int32 I = FMath::Max(Cursor, 0); I < SessionSteps.Num(); ++I)
	{
		if (SessionSteps[I].Verdict == ERTPieVerdict::Pending)
		{
			SessionSteps[I].Verdict = ERTPieVerdict::NotRun;
			SessionSteps[I].Reason = TEXT("seduta interrotta");
			SessionSteps[I].Source = ERTPieVerdictSource::Conductor;
			SessionSteps[I].At = FDateTime::UtcNow();
		}
	}

	if (Ports.TearDown) { Ports.TearDown(); }
	Finish();
}

void URTPieSessionSubsystem::Finish()
{
	if (SessionState == ERTPieSessionState::Done)
	{
		return;
	}
	SessionState = ERTPieSessionState::Done;

	const FString Commit = URTPieSessionWriter::ReadHeadCommit();

	FString Path, Error;
	if (URTPieSessionWriter::Write(CurrentSessionId, Commit, SessionSteps, Path, Error))
	{
		SessionFilePath = Path;
		UE_LOG(LogRT, Warning, TEXT("[RT-Pie] seduta %s chiusa · %s"), *CurrentSessionId, *Path);
	}
	else
	{
		UE_LOG(LogRT, Error, TEXT("[RT-Pie] file di seduta non scritto: %s"), *Error);
	}

	if (Commit.IsEmpty())
	{
		// ⚠️ Si annuncia, non si tace: in questo progetto una misura vale se avviene su un commit
		// dichiarato, e una seduta che non sa dirlo va letta sapendolo.
		UE_LOG(LogRT, Error,
			TEXT("[RT-Pie] commit non dichiarato — questa seduta non e' attribuibile a un albero"));
	}
}
