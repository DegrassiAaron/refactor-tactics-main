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
		TArray<FString> Lanciati;
		int32 TearDowns = 0;
		ERTScenarioStart Esito = ERTScenarioStart::Started;

		FRTPieSessionPorts Porte()
		{
			FRTPieSessionPorts P;
			P.Launch = [this](const FString& Id) { Lanciati.Add(Id); return Esito; };
			P.TearDown = [this]() { ++TearDowns; };
			return P;
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

	TestEqual(TEXT("parte un solo scenario"), Banco.Lanciati.Num(), 1);
	TestEqual(TEXT("ed e' il primo"), Banco.Lanciati[0], TEXT("Scen.A"));

	Conduttore->OnScenarioFinished(PieSessionEsito(TEXT("Scen.A"), true));
	TestTrue(TEXT("ora aspetta un verdetto"),
		Conduttore->State() == ERTPieSessionState::AwaitingVerdict);
	TestEqual(TEXT("e non ha lanciato nient'altro prima di averlo"), Banco.Lanciati.Num(), 1);

	Conduttore->SubmitVerdict(ERTPieVerdict::Pass, FString());
	TestEqual(TEXT("il teardown precede il passo dopo"), Banco.TearDowns, 1);
	TestEqual(TEXT("e il secondo parte"), Banco.Lanciati.Num(), 2);
	TestEqual(TEXT("ed e' Scen.B"), Banco.Lanciati[1], TEXT("Scen.B"));

	Conduttore->OnScenarioFinished(PieSessionEsito(TEXT("Scen.B"), true));
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
	Conduttore->OnScenarioFinished(PieSessionEsito(TEXT("Scen.A"), /*bPassed=*/ false));

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
	Banco.Esito = ERTScenarioStart::NotLoadable;

	Conduttore->Begin({ PieSessionPasso(TEXT("PIE-A"), TEXT("Scen.Assente")),
	                    PieSessionPasso(TEXT("PIE-B"), TEXT("Scen.B")) }, Banco.Porte());

	TestTrue(TEXT("chiuso NotJudgeable senza chiedere"),
		Conduttore->Steps()[0].Verdict == ERTPieVerdict::NotJudgeable);
	TestTrue(TEXT("con un motivo scritto"), !Conduttore->Steps()[0].Reason.IsEmpty());
	TestEqual(TEXT("e si prosegue invece di fermarsi"), Banco.Lanciati.Num(), 2);
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
	Conduttore->OnScenarioFinished(PieSessionErrore(TEXT("Scen.A"), TEXT("unita' V9 non esiste")));

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
	Conduttore->OnScenarioFinished(PieSessionEsito(TEXT("Scen.A"), true));
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

	Conduttore->OnScenarioFinished(PieSessionEsito(TEXT("Scen.DellaSeduta"), true));
	Conduttore->SubmitVerdict(ERTPieVerdict::Pass, FString());
	TestTrue(TEXT("a seduta finita torna a non imporre niente"),
		URTPieSessionSubsystem::ScenarioImposedBy(Conduttore).IsEmpty());
	return true;
}

#endif
