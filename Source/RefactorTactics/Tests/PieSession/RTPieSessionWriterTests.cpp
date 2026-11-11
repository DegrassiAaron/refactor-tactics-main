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
	/** Quante volte `Ago` compare in `Pagliaio`. `FString` non ha un contatore di occorrenze. */
	int32 PieWriterOccorrenze(const FString& Pagliaio, const TCHAR* Ago)
	{
		int32 N = 0;
		int32 Da = 0;
		const int32 LenAgo = FCString::Strlen(Ago);
		while (true)
		{
			const int32 Trovato = Pagliaio.Find(Ago, ESearchCase::CaseSensitive, ESearchDir::FromStart, Da);
			if (Trovato == INDEX_NONE) { break; }
			++N;
			Da = Trovato + LenAgo;
		}
		return N;
	}

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

	// ⛔ Il CAMPO, non il valore sciolto: `Contains("FAIL 3/7")` resterebbe verde anche se i due valori
	// finissero l'uno nel campo dell'altro, che e' precisamente il difetto che questo gate esiste per
	// prendere.
	TestTrue(TEXT("l'esito macchina sta nel suo campo"),
		Json.Contains(TEXT("\"machineOutcome\": \"FAIL 3/7\"")));
	TestTrue(TEXT("e il verdetto umano, opposto, nel suo"),
		Json.Contains(TEXT("\"verdict\": \"PASS\"")));
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
	// c'era o se nessuno l'ha scritto. ⚠️ Il discrimine e' la SORGENTE: `Blocked` lo scrive solo il
	// conduttore, e questo passo lo dichiara — senza, il gate proverebbe il criterio vecchio.
	FRTPieSessionStep Bloccato = PieWriterPasso(TEXT("PIE-A"), ERTPieVerdict::Blocked);
	Bloccato.Source = ERTPieVerdictSource::Conductor;

	const FString Json = URTPieSessionWriter::ToJson(TEXT("20260920-000000"), TEXT("c"), { Bloccato });

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionHumanNotJudgeableIsNotADefectTest,
	"RefactorTactics.PieSession.AHumanNotJudgeableIsNotMarkedAsMissingAReason",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionHumanNotJudgeableIsNotADefectTest::RunTest(const FString&)
{
	// 🔴 Il tasto `3` e' l'unico giudizio non binario che il design offre, e prima marchiava
	// «(motivo non registrato)» a ogni pressione: l'unica risposta prevista produceva un difetto falso.
	FRTPieSessionStep Umano = PieWriterPasso(TEXT("PIE-A"), ERTPieVerdict::NotJudgeable);
	Umano.Source = ERTPieVerdictSource::Human;

	FRTPieSessionStep Conduttore = PieWriterPasso(TEXT("PIE-B"), ERTPieVerdict::NotJudgeable);
	Conduttore.Source = ERTPieVerdictSource::Conductor;

	const FString Json = URTPieSessionWriter::ToJson(TEXT("s"), TEXT("c"), { Umano, Conduttore });

	TestTrue(TEXT("la sorgente finisce nel file"),
		Json.Contains(TEXT("\"verdictSource\": \"human\"")));
	TestEqual(TEXT("il marcatore compare UNA volta sola, per il conduttore"),
		PieWriterOccorrenze(Json, TEXT("(motivo non registrato)")), 1);
	return true;
}

#endif
