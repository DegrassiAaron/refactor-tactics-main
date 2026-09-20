// La CONDUZIONE di una seduta, verificata senza aprire l'Editor.
//
// 🔑 E' il gate che giustifica le due porte. Se il conduttore parlasse direttamente con
// `FRTScenarioCoordinator` — classe concreta, posseduta per valore e privata dal GameMode — ognuno di
// questi casi richiederebbe un mondo, uno scenario vero e un Editor. In un progetto dove una verifica
// PIE costa una persona a schermo, la parte che le automatizza non puo' essere giudicabile solo a
// schermo.

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "PieSession/RTPieSessionSubsystem.h"
#include "ScenarioHarness/RTTestResult.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Le porte finte: registrano cosa e' stato chiesto, senza aprire niente. */
	struct FPieSessionBanco
	{
		/** Ordine REALE degli eventi, non due contatori: `"launch:Scen.A"`, `"teardown"`. */
		TArray<FString> Eventi;
		FRTPieLaunchOutcome Esito = FRTPieLaunchOutcome::Avviato();

		FRTPieSessionPorts Porte()
		{
			FRTPieSessionPorts P;
			P.Launch = [this](const FString& Id)
			{
				Eventi.Add(FString::Printf(TEXT("launch:%s"), *Id));
				return Esito;
			};
			P.TearDown = [this]() { Eventi.Add(TEXT("teardown")); };
			return P;
		}

		TArray<FString> Lanci() const
		{
			TArray<FString> Fuori;
			for (const FString& E : Eventi)
			{
				if (E.StartsWith(TEXT("launch:"))) { Fuori.Add(E.RightChop(7)); }
			}
			return Fuori;
		}
		int32 TearDowns() const
		{
			int32 N = 0;
			for (const FString& E : Eventi) { if (E == TEXT("teardown")) { ++N; } }
			return N;
		}
	};

	/**
	 * 🔴 **L'outer dev'essere una `UGameInstance`, e non e' una formalita'.**
	 *
	 * `URTPieSessionSubsystem` eredita da `UGameInstanceSubsystem`, che dichiara
	 * `ClassWithin = UGameInstance`: un `NewObject` senza outer valido scatta un ensure —
	 * *«Object None of class RTPieSessionSubsystem with ClassWithin of GameInstance was created in
	 * invalid Outer /Script/CoreUObject.Package»* — e l'ensure fa fallire il test che gli capita
	 * accanto, non quello che l'ha causato. Misurato il 2026-09-20: tre test rossi su cinque, e i due
	 * verdi lo erano solo perche' l'ensure e' one-shot per occorrenza.
	 */
	URTPieSessionSubsystem* PieSessionNuovoConduttore()
	{
		UGameInstance* GI = NewObject<UGameInstance>(GetTransientPackage());
		return NewObject<URTPieSessionSubsystem>(GI);
	}

	FRTPieSessionStep PieSessionPasso(const TCHAR* Voce, const TCHAR* Scenario)
	{
		FRTPieSessionStep S;
		S.PieItem = Voce;
		S.ScenarioId = Scenario;
		return S;
	}

	FRTTestResult PieSessionEsito(const TCHAR* ScenarioId, bool bPassed)
	{
		FRTTestResult R;
		R.ScenarioId = ScenarioId;
		FRTAssertionResult A;
		A.bPassed = bPassed;
		A.Description = TEXT("finta");
		R.Assertions.Add(A);
		return R;
	}

	FRTTestResult PieSessionErrore(const TCHAR* ScenarioId, const TCHAR* Messaggio)
	{
		FRTTestResult R;
		R.ScenarioId = ScenarioId;
		R.ErrorMessage = Messaggio;
		return R;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionAdvancesInOrderTest,
	"RefactorTactics.PieSession.AdvancesInOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionAdvancesInOrderTest::RunTest(const FString&)
{
	URTPieSessionSubsystem* Conduttore = PieSessionNuovoConduttore();
	FPieSessionBanco Banco;

	Conduttore->Begin({ PieSessionPasso(TEXT("PIE-A"), TEXT("Scen.A")),
	                    PieSessionPasso(TEXT("PIE-B"), TEXT("Scen.B")) }, Banco.Porte());

	TestEqual(TEXT("parte un solo scenario"), Banco.Lanci().Num(), 1);
	TestEqual(TEXT("ed e' il primo"), Banco.Lanci()[0], TEXT("Scen.A"));

	Conduttore->OnScenarioFinished(PieSessionEsito(TEXT("Scen.A"), true), TEXT("Saved/RTTests/Finto/20260920-000000"));
	TestTrue(TEXT("ora aspetta un verdetto"),
		Conduttore->State() == ERTPieSessionState::AwaitingVerdict);
	TestEqual(TEXT("e non ha lanciato nient'altro prima di averlo"), Banco.Lanci().Num(), 1);

	Conduttore->SubmitVerdict(ERTPieVerdict::Pass, FString());
	// ⛔ L'ORDINE, non due conteggi: `TearDowns == 1` resterebbe verde anche se il teardown avvenisse
	// DOPO il lancio successivo, cioe' smontando la scena appena allestita.
	const TArray<FString> Atteso = { TEXT("launch:Scen.A"), TEXT("teardown"), TEXT("launch:Scen.B") };
	if (Banco.Eventi != Atteso)
	{
		AddError(FString::Printf(TEXT("sequenza attesa [%s], ottenuta [%s]"),
			*FString::Join(Atteso, TEXT(" -> ")), *FString::Join(Banco.Eventi, TEXT(" -> "))));
	}

	Conduttore->OnScenarioFinished(PieSessionEsito(TEXT("Scen.B"), true), TEXT("Saved/RTTests/Finto/20260920-000000"));
	Conduttore->SubmitVerdict(ERTPieVerdict::Fail, FString());

	TestTrue(TEXT("coda vuota -> Done"), Conduttore->State() == ERTPieSessionState::Done);
	TestTrue(TEXT("il primo verdetto e' quello dato"),
		Conduttore->Steps()[0].Verdict == ERTPieVerdict::Pass);
	TestTrue(TEXT("e il secondo anche, ed e' opposto"),
		Conduttore->Steps()[1].Verdict == ERTPieVerdict::Fail);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionRedExpectStillAsksTest,
	"RefactorTactics.PieSession.FailedExpectStillAsksForAVerdict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionRedExpectStillAsksTest::RunTest(const FString&)
{
	// La coppia che il progetto produce davvero: nella seduta U54 il feed passava a verde mentre il
	// dock falliva. Un conduttore che chiudesse il passo da solo su `expect` rosse perderebbe meta'
	// dell'informazione, e sarebbe la scorciatoia piu' facile da scrivere.
	URTPieSessionSubsystem* Conduttore = PieSessionNuovoConduttore();
	FPieSessionBanco Banco;

	Conduttore->Begin({ PieSessionPasso(TEXT("PIE-A"), TEXT("Scen.A")) }, Banco.Porte());
	Conduttore->OnScenarioFinished(PieSessionEsito(TEXT("Scen.A"), /*bPassed=*/ false), TEXT("Saved/RTTests/Finto/20260920-000000"));

	TestTrue(TEXT("expect rosse non chiudono il passo da sole"),
		Conduttore->State() == ERTPieSessionState::AwaitingVerdict);
	TestTrue(TEXT("il verdetto resta da dare"),
		Conduttore->Steps()[0].Verdict == ERTPieVerdict::Pending);
	TestTrue(TEXT("ma l'esito macchina e' gia' registrato"),
		!Conduttore->Steps()[0].MachineOutcome.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionNotLoadableIsNotAskedTest,
	"RefactorTactics.PieSession.NotLoadableIsNotAsked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionNotLoadableIsNotAskedTest::RunTest(const FString&)
{
	// Automation fa fallire un test per ogni `UE_LOG(..., Error, ...)` che compare mentre gira, e qui
	// due ne compaiono di proposito. Dichiararli attesi NON e' un aggiramento: se il conduttore
	// smettesse di dire perche' ha saltato un passo, questa riga diventerebbe rossa da sola.
	AddExpectedError(TEXT("NON GIUDICABILE: scenario non caricabile"),
		EAutomationExpectedErrorFlags::Contains, 2);

	URTPieSessionSubsystem* Conduttore = PieSessionNuovoConduttore();
	FPieSessionBanco Banco;
	Banco.Esito = FRTPieLaunchOutcome::NonCaricabile();

	Conduttore->Begin({ PieSessionPasso(TEXT("PIE-A"), TEXT("Scen.Assente")),
	                    PieSessionPasso(TEXT("PIE-B"), TEXT("Scen.B")) }, Banco.Porte());

	TestTrue(TEXT("chiuso NotJudgeable senza chiedere"),
		Conduttore->Steps()[0].Verdict == ERTPieVerdict::NotJudgeable);
	TestTrue(TEXT("con un motivo scritto"), !Conduttore->Steps()[0].Reason.IsEmpty());
	TestEqual(TEXT("e si prosegue invece di fermarsi"), Banco.Lanci().Num(), 2);
	TestTrue(TEXT("la seduta arriva in fondo"), Conduttore->State() == ERTPieSessionState::Done);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionErroredSessionIsBlockedTest,
	"RefactorTactics.PieSession.ErroredSessionIsBlockedNotAsked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionErroredSessionIsBlockedTest::RunTest(const FString&)
{
	// Come sopra: il motivo del blocco si scrive nel log, e che ci finisca fa parte del contratto.
	AddExpectedError(TEXT("BLOCCATO: unita' V9 non esiste"),
		EAutomationExpectedErrorFlags::Contains, 1);

	URTPieSessionSubsystem* Conduttore = PieSessionNuovoConduttore();
	FPieSessionBanco Banco;

	Conduttore->Begin({ PieSessionPasso(TEXT("PIE-A"), TEXT("Scen.A")) }, Banco.Porte());
	Conduttore->OnScenarioFinished(PieSessionErrore(TEXT("Scen.A"), TEXT("unita' V9 non esiste")), TEXT("Saved/RTTests/Finto/20260920-000000"));

	TestTrue(TEXT("una scena mai partita non si fa giudicare"),
		Conduttore->Steps()[0].Verdict == ERTPieVerdict::Blocked);
	TestEqual(TEXT("e il motivo e' quello del harness"),
		Conduttore->Steps()[0].Reason, TEXT("unita' V9 non esiste"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionAbortKeepsWhatWasGivenTest,
	"RefactorTactics.PieSession.AbortKeepsWhatWasGiven",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionAbortKeepsWhatWasGivenTest::RunTest(const FString&)
{
	URTPieSessionSubsystem* Conduttore = PieSessionNuovoConduttore();
	FPieSessionBanco Banco;

	Conduttore->Begin({ PieSessionPasso(TEXT("PIE-A"), TEXT("Scen.A")),
	                    PieSessionPasso(TEXT("PIE-B"), TEXT("Scen.B")) }, Banco.Porte());
	Conduttore->OnScenarioFinished(PieSessionEsito(TEXT("Scen.A"), true), TEXT("Saved/RTTests/Finto/20260920-000000"));
	Conduttore->SubmitVerdict(ERTPieVerdict::Pass, FString());
	Conduttore->Abort();

	TestTrue(TEXT("il verdetto gia' dato resta"),
		Conduttore->Steps()[0].Verdict == ERTPieVerdict::Pass);
	TestTrue(TEXT("il resto e' NotRun, non Pending"),
		Conduttore->Steps()[1].Verdict == ERTPieVerdict::NotRun);
	TestTrue(TEXT("e la seduta e' chiusa"), Conduttore->State() == ERTPieSessionState::Done);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionImposesItsScenarioTest,
	"RefactorTactics.PieSession.AConductingSessionImposesItsScenario",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionImposesItsScenarioTest::RunTest(const FString&)
{
	// La quarta sorgente di `ARTGameMode::ResolveScenarioToRun`, e perche' vince: una CVar
	// `rt.Test.Scenario` rimasta impostata da una prova precedente dirotterebbe in silenzio ogni passo,
	// e chi guarda crederebbe di giudicare la voce che il conduttore ha appena annunciato.
	//
	// ⚠️ Cio' che questo gate NON copre: le tre righe che nel GameMode chiamano questa funzione prima
	// del blocco della console. Il mondo dei test non ha una `UGameInstance`, quindi li' dentro
	// `GetSubsystem` darebbe sempre null e un test non distinguerebbe «nessuna seduta» da «non ho
	// potuto chiedere». Quel collegamento si verifica nella prima seduta reale.
	TestTrue(TEXT("nessun conduttore non impone niente"),
		URTPieSessionSubsystem::ScenarioImposedBy(nullptr).IsEmpty());

	URTPieSessionSubsystem* Conduttore = PieSessionNuovoConduttore();
	TestTrue(TEXT("un conduttore fermo non impone niente"),
		URTPieSessionSubsystem::ScenarioImposedBy(Conduttore).IsEmpty());

	FPieSessionBanco Banco;
	Conduttore->Begin({ PieSessionPasso(TEXT("PIE-A"), TEXT("Scen.DellaSeduta")) }, Banco.Porte());
	TestEqual(TEXT("mentre conduce impone il passo corrente"),
		URTPieSessionSubsystem::ScenarioImposedBy(Conduttore), FString(TEXT("Scen.DellaSeduta")));

	Conduttore->OnScenarioFinished(PieSessionEsito(TEXT("Scen.DellaSeduta"), true), TEXT("Saved/RTTests/Finto/20260920-000000"));
	Conduttore->SubmitVerdict(ERTPieVerdict::Pass, FString());
	TestTrue(TEXT("a seduta finita torna a non imporre niente"),
		URTPieSessionSubsystem::ScenarioImposedBy(Conduttore).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionDeadOnArrivalIsBlockedTest,
	"RefactorTactics.PieSession.ASessionDeadOnArrivalIsBlockedNotAwaited",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionDeadOnArrivalIsBlockedTest::RunTest(const FString&)
{
	// 🔴 **Il caso che il conduttore non vedeva, trovato in code review il 2026-09-20.**
	// `FRTScenarioCoordinator::Start` risponde `Started` anche quando la sessione fallisce all'avvio —
	// deliberatamente — ma quella sessione nasce `Finished`, quindi `Tick` esce subito e **nessun
	// `OnScenarioFinished` arriva mai**. Un conduttore che aspettasse quell'evento resterebbe in
	// `Playing` per sempre, e l'unica uscita sarebbe `rt.Pie.Session.Abort`.
	AddExpectedError(TEXT("BLOCCATO: scenario non valido"), EAutomationExpectedErrorFlags::Contains, 1);

	URTPieSessionSubsystem* Conduttore = PieSessionNuovoConduttore();
	FPieSessionBanco Banco;
	Banco.Esito = FRTPieLaunchOutcome::MortoAllaNascita(TEXT("scenario non valido: unita' V9 assente"));

	Conduttore->Begin({ PieSessionPasso(TEXT("PIE-A"), TEXT("Scen.Morto")) }, Banco.Porte());

	TestTrue(TEXT("il passo e' chiuso, non in attesa"),
		Conduttore->Steps()[0].Verdict == ERTPieVerdict::Blocked);
	TestTrue(TEXT("col motivo del harness"),
		Conduttore->Steps()[0].Reason.Contains(TEXT("unita' V9 assente")));
	TestTrue(TEXT("e la seduta non resta appesa"), Conduttore->State() == ERTPieSessionState::Done);
	TestEqual(TEXT("la scena morta viene smontata lo stesso"), Banco.TearDowns(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionRefusesASecondSessionTest,
	"RefactorTactics.PieSession.ASecondBeginDoesNotDiscardVerdicts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionRefusesASecondSessionTest::RunTest(const FString&)
{
	// Digitare `rt.Pie.Session <altro>` a meta' seduta buttava via i verdetti gia' dati senza scrivere
	// nulla — l'esatto contrario di cio' che `Abort` promette.
	AddExpectedError(TEXT("seduta gia' in corso"), EAutomationExpectedErrorFlags::Contains, 1);

	URTPieSessionSubsystem* Conduttore = PieSessionNuovoConduttore();
	FPieSessionBanco Banco;
	Conduttore->Begin({ PieSessionPasso(TEXT("PIE-A"), TEXT("Scen.A")),
	                    PieSessionPasso(TEXT("PIE-B"), TEXT("Scen.B")) }, Banco.Porte());
	Conduttore->OnScenarioFinished(PieSessionEsito(TEXT("Scen.A"), true), TEXT("Saved/RTTests/Finto/20260920-000000"));
	Conduttore->SubmitVerdict(ERTPieVerdict::Pass, FString());

	Conduttore->Begin({ PieSessionPasso(TEXT("PIE-Z"), TEXT("Scen.Z")) });

	TestEqual(TEXT("la coda e' ancora quella"), Conduttore->Steps().Num(), 2);
	TestTrue(TEXT("e il verdetto dato non e' sparito"),
		Conduttore->Steps()[0].Verdict == ERTPieVerdict::Pass);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionRecordsTheReportTest,
	"RefactorTactics.PieSession.StepRecordsTheReportItCameFrom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionRecordsTheReportTest::RunTest(const FString&)
{
	// `runId` e `reportDir` erano campi morti: il file li scriveva vuoti mentre la spec li mostrava
	// pieni, e sono il ponte verso il referto della run — cioe' esattamente cio' che serve al passo
	// successivo, la propagazione al registro.
	URTPieSessionSubsystem* Conduttore = PieSessionNuovoConduttore();
	FPieSessionBanco Banco;
	Conduttore->Begin({ PieSessionPasso(TEXT("PIE-A"), TEXT("Scen.A")) }, Banco.Porte());
	Conduttore->OnScenarioFinished(PieSessionEsito(TEXT("Scen.A"), true),
		TEXT("Saved/RTTests/Scen.A/20260920-101500"));

	TestEqual(TEXT("la cartella del referto e' registrata"),
		Conduttore->Steps()[0].ReportDir, TEXT("Saved/RTTests/Scen.A/20260920-101500"));
	TestEqual(TEXT("e il runId e' il suo ultimo segmento"),
		Conduttore->Steps()[0].RunId, TEXT("20260920-101500"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionHumanVerdictIsMarkedHumanTest,
	"RefactorTactics.PieSession.AHumanVerdictIsMarkedAsHuman",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionHumanVerdictIsMarkedHumanTest::RunTest(const FString&)
{
	URTPieSessionSubsystem* Conduttore = PieSessionNuovoConduttore();
	FPieSessionBanco Banco;
	Conduttore->Begin({ PieSessionPasso(TEXT("PIE-A"), TEXT("Scen.A")) }, Banco.Porte());
	Conduttore->OnScenarioFinished(PieSessionEsito(TEXT("Scen.A"), true), FString());
	Conduttore->SubmitVerdict(ERTPieVerdict::NotJudgeable, FString());

	// Senza la sorgente, questo `NOT_JUDGEABLE` sarebbe indistinguibile da uno scritto dal conduttore
	// perche' lo scenario non si allestiva — due fatti opposti nello stesso valore.
	TestTrue(TEXT("il verdetto e' marcato come umano"),
		Conduttore->Steps()[0].Source == ERTPieVerdictSource::Human);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionRejectsANonHumanVerdictTest,
	"RefactorTactics.PieSession.RejectsAVerdictOnlyTheConductorMayWrite",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionRejectsANonHumanVerdictTest::RunTest(const FString&)
{
	AddExpectedError(TEXT("non e' un verdetto che possa dare una persona"),
		EAutomationExpectedErrorFlags::Contains, 1);

	URTPieSessionSubsystem* Conduttore = PieSessionNuovoConduttore();
	FPieSessionBanco Banco;
	Conduttore->Begin({ PieSessionPasso(TEXT("PIE-A"), TEXT("Scen.A")) }, Banco.Porte());
	Conduttore->OnScenarioFinished(PieSessionEsito(TEXT("Scen.A"), true), FString());

	// `Pending` chiuderebbe il passo scrivendo «PENDING» nel file come se fosse un esito.
	Conduttore->SubmitVerdict(ERTPieVerdict::Pending, FString());

	TestTrue(TEXT("il passo resta in attesa"),
		Conduttore->State() == ERTPieSessionState::AwaitingVerdict);
	TestTrue(TEXT("e nessun verdetto e' stato scritto"),
		Conduttore->Steps()[0].Verdict == ERTPieVerdict::Pending);
	return true;
}

#endif
