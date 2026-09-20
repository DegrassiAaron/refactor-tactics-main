// Il file di seduta, e il gate che vieta la scorciatoia.
//
// 🔴 Il test che conta qui e' `VerdictIsNotDeducedFromMachineOutcome`. La scorciatoia piu' tentante di
// questo sottosistema e' far scendere il verdetto umano dall'esito delle `expect` — sembra corretto,
// costa una riga, e cancellerebbe l'unica informazione che una seduta PIE produce e un gate no.

#include "Misc/AutomationTest.h"
#include "PieSession/RTPieSessionWriter.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FRTPieSessionStep PieWriterPasso(const TCHAR* Voce, ERTPieVerdict Verdict)
	{
		FRTPieSessionStep S;
		S.PieItem = Voce;
		S.ScenarioId = TEXT("Scen.A");
		S.Verdict = Verdict;
		return S;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionVerdictIsNotDeducedTest,
	"RefactorTactics.PieSession.VerdictIsNotDeducedFromMachineOutcome",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionVerdictIsNotDeducedTest::RunTest(const FString&)
{
	// La coppia contraddittoria e' il caso reale, non un caso limite: seduta U54, feed verde e dock
	// rosso nella stessa apertura.
	FRTPieSessionStep Step = PieWriterPasso(TEXT("PIE-A"), ERTPieVerdict::Pass);
	Step.MachineOutcome = TEXT("FAIL 3/7");

	const FString Json = URTPieSessionWriter::ToJson(TEXT("20260920-000000"), TEXT("deadbeef"), { Step });

	TestTrue(TEXT("il file porta l'esito macchina"), Json.Contains(TEXT("FAIL 3/7")));
	TestTrue(TEXT("e il verdetto umano, che e' l'opposto"), Json.Contains(TEXT("\"PASS\"")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionUnknownCommitIsDeclaredTest,
	"RefactorTactics.PieSession.UnknownCommitIsDeclaredNotOmitted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionUnknownCommitIsDeclaredTest::RunTest(const FString&)
{
	const FString Json = URTPieSessionWriter::ToJson(TEXT("20260920-000000"), FString(),
		{ PieWriterPasso(TEXT("PIE-A"), ERTPieVerdict::Pass) });

	// Un campo assente si legge come «non pertinente»; `null` si legge come «non lo so», che e' il fatto.
	TestTrue(TEXT("il commit ignoto e' null, non assente"),
		Json.Contains(TEXT("\"commit\": null")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionKnownCommitIsWrittenTest,
	"RefactorTactics.PieSession.KnownCommitIsWritten",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionKnownCommitIsWrittenTest::RunTest(const FString&)
{
	// Il controllo positivo del test qui sopra: senza, «contiene null» passerebbe anche se il writer
	// scrivesse null SEMPRE, e il campo non direbbe piu' niente.
	const FString Json = URTPieSessionWriter::ToJson(TEXT("20260920-000000"), TEXT("15c50b96"),
		{ PieWriterPasso(TEXT("PIE-A"), ERTPieVerdict::Pass) });

	TestTrue(TEXT("il commit noto si scrive"), Json.Contains(TEXT("\"commit\": \"15c50b96\"")));
	TestFalse(TEXT("e non resta null"), Json.Contains(TEXT("\"commit\": null")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionReasonRequiredTest,
	"RefactorTactics.PieSession.NonHumanVerdictCarriesAReason",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionReasonRequiredTest::RunTest(const FString&)
{
	// Un `BLOCKED` senza motivo e' un buco silenzioso: chi legge il file dopo non sa se il motivo non
	// c'era o se nessuno l'ha scritto.
	const FString Json = URTPieSessionWriter::ToJson(TEXT("20260920-000000"), TEXT("c"),
		{ PieWriterPasso(TEXT("PIE-A"), ERTPieVerdict::Blocked) });

	TestTrue(TEXT("un motivo mancante si dichiara tale"),
		Json.Contains(TEXT("(motivo non registrato)")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionHeadCommitIsReadableTest,
	"RefactorTactics.PieSession.HeadCommitIsReadableFromTheRepo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionHeadCommitIsReadableTest::RunTest(const FString&)
{
	// Gira dentro il repository, quindi `.git/HEAD` c'e' e lo SHA deve uscire. Se un giorno il progetto
	// venisse eseguito da un albero esportato senza `.git`, questo test dira' che il campo diventa null
	// — che e' il comportamento voluto, ma va saputo.
	const FString Commit = URTPieSessionWriter::ReadHeadCommit();
	TestTrue(TEXT("lo SHA si legge"), Commit.Len() >= 7);
	TestFalse(TEXT("e non e' rimasto il 'ref:'"), Commit.Contains(TEXT("ref:")));
	return true;
}

#endif
