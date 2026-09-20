#include "PieSession/RTPieSessionSubsystem.h"

#include "PieSession/RTPieSessionWriter.h"
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

void URTPieSessionSubsystem::Begin(TArray<FRTPieSessionStep> InSteps)
{
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
		const ERTScenarioStart Start = Ports.Launch
			? Ports.Launch(Step.ScenarioId)
			: ERTScenarioStart::NotLoadable;

		if (Start == ERTScenarioStart::Started)
		{
			UE_LOG(LogRT, Warning, TEXT("[RT-Pie] passo %d/%d — %s da %s"),
				Cursor + 1, SessionSteps.Num(), *Step.PieItem, *Step.ScenarioId);
			return;
		}

		// ⛔ Non si chiede un giudizio su una scena che non si e' allestita, e non ci si ferma nemmeno:
		// fermarsi qui costringerebbe a riaprire l'Editor per le voci a valle.
		Step.Verdict = ERTPieVerdict::NotJudgeable;
		Step.Reason = FString::Printf(TEXT("scenario non caricabile: %s"), *Step.ScenarioId);
		Step.At = FDateTime::UtcNow();
		UE_LOG(LogRT, Error, TEXT("[RT-Pie] %s -> NON GIUDICABILE: %s"), *Step.PieItem, *Step.Reason);
		++Cursor;
	}

	Finish();
}

void URTPieSessionSubsystem::OnScenarioFinished(const FRTTestResult& Result)
{
	if (!SessionSteps.IsValidIndex(Cursor) || SessionState != ERTPieSessionState::Playing)
	{
		return;
	}

	FRTPieSessionStep& Step = SessionSteps[Cursor];
	Step.MachineOutcome = FString::Printf(TEXT("%s %d/%d"), *Result.OutcomeString(),
		Result.PassedCount(), Result.Assertions.Num());

	if (!Result.ErrorMessage.IsEmpty())
	{
		// La sessione e' esistita ma non ha mai giocato: chiedere un verdetto sarebbe chiedere di
		// guardare una scena mai partita, e qualunque risposta sarebbe su niente.
		Step.Verdict = ERTPieVerdict::Blocked;
		Step.Reason = Result.ErrorMessage;
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

	FRTPieSessionStep& Step = SessionSteps[Cursor];
	Step.Verdict = Verdict;
	Step.Reason = Reason;
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
