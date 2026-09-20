// `verifies` nel formato scenario: gli ID delle voci PIE che quell'allestimento permette di giudicare.
//
// 🔑 Il test che conta qui e' il ROUND-TRIP, non la lettura. Il campo `Tags` fu letto e non riscritto,
// e un `load → save` lo cancellava da ogni file che lo dichiarava — lo racconta il commento del loader
// accanto al blocco che li legge. Stessa trappola, stesso posto, quindi lo stesso gate.

#include "Misc/AutomationTest.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTTestScenario.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nomi con prefisso proprio: la unity build condivide la translation unit.
	//
	// ⚠️ **Unita' ED `expect` ci sono perche' SERVONO**, e ci sono volute due misure per saperlo: il
	// loader rifiuta `units: []` con «uno scenario deve schierare almeno una unita'» e poi, superato
	// quello, `expect: []` con «nessuna assertion dichiarata: lo scenario passerebbe sempre».
	// Un fixture invalido faceva fallire i test per la ragione sbagliata — e il test sul campo
	// malformato passava lo stesso, perche' `TestFalse` non guarda PERCHE' il caricamento e' fallito.
	// E' il motivo per cui l'asserzione sul motivo stampa l'errore che ha trovato: e' stata lei a
	// dire quale fosse il secondo controllo.
	const TCHAR* PieSessionMinimalJson = TEXT(R"({
		"scenarioId": "Spec.PieSession.Fixture",
		"version": 1,
		"mapRadius": 3,
		"verifies": ["PIE-VIS-SIGHTWALL", "PIE-V01-LOG"],
		"units": [{ "id": "F1", "hero": "Hero.Aevik", "team": 0, "cell": [-1, 0, 0] }],
		"turns": [],
		"expect": [{ "type": "TurnsCompleted", "value": 0 }]
	})");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionVerifiesLoadsTest,
	"RefactorTactics.PieSession.VerifiesSurvivesLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionVerifiesLoadsTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	FString Error;
	TestTrue(TEXT("il JSON si carica"),
		URTScenarioLoader::LoadFromString(PieSessionMinimalJson, Scenario, Error));
	TestEqual(TEXT("due voci dichiarate"), Scenario.Verifies.Num(), 2);
	TestEqual(TEXT("la prima e' quella scritta"), Scenario.Verifies[0], TEXT("PIE-VIS-SIGHTWALL"));
	TestEqual(TEXT("e la seconda anche"), Scenario.Verifies[1], TEXT("PIE-V01-LOG"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionVerifiesSurvivesRoundTripTest,
	"RefactorTactics.PieSession.VerifiesSurvivesRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionVerifiesSurvivesRoundTripTest::RunTest(const FString&)
{
	FRTTestScenario Andata;
	FString Error;
	if (!URTScenarioLoader::LoadFromString(PieSessionMinimalJson, Andata, Error))
	{
		AddError(FString::Printf(TEXT("il fixture non si carica: %s"), *Error));
		return false;
	}

	// ⚠️ Non esiste un `URTScenarioWriter::ToJson`: `RTScenarioWriter.cpp` non ha header, e la
	// serializzazione passa da `URTScenarioLoader::SaveToFile` e da nessun altro posto
	// (`RTScenarioAuthoring.h:21`). Il round-trip si fa quindi su un file temporaneo.
	const FString Temp = FPaths::Combine(FPaths::ProjectSavedDir(),
		TEXT("RTPieSessionTests"), TEXT("roundtrip.json"));
	TestTrue(TEXT("si salva"), URTScenarioLoader::SaveToFile(Andata, Temp, Error));

	FRTTestScenario Ritorno;
	TestTrue(TEXT("e si ricarica"), URTScenarioLoader::LoadFromFile(Temp, Ritorno, Error));
	// `TestEqual` non ha un overload per `TArray<FString>`: si confronta l'array e, se diverge, si
	// stampa cosa e' tornato — un `TestTrue` nudo direbbe solo «falso».
	if (Ritorno.Verifies != Andata.Verifies)
	{
		AddError(FString::Printf(TEXT("le voci non sopravvivono al salvataggio: atteso [%s], ottenuto [%s]"),
			*FString::Join(Andata.Verifies, TEXT(", ")), *FString::Join(Ritorno.Verifies, TEXT(", "))));
	}

	IFileManager::Get().Delete(*Temp);
	return true;
}

// Un formato che accetta una voce vuota lascerebbe entrare in coda un passo senza ancora: la voce
// non si potrebbe ritrovare nel registro, e il verdetto non avrebbe a cosa attaccarsi.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionVerifiesRejectsNonStringTest,
	"RefactorTactics.PieSession.VerifiesRejectsANonString",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionVerifiesRejectsNonStringTest::RunTest(const FString&)
{
	// 🔑 Lo scenario e' per il resto VALIDO — unita' compresa. Senza, il caricamento fallirebbe per le
	// unita' mancanti e questo test sarebbe verde senza aver mai guardato `verifies`: e' il difetto
	// misurato il 2026-09-20, ed e' la ragione per cui l'asserzione sul MOTIVO vale piu' di `TestFalse`.
	const TCHAR* Malformato = TEXT(R"({
		"scenarioId": "Spec.PieSession.Malformato",
		"mapRadius": 3,
		"verifies": ["PIE-V01-LOG", 42],
		"units": [{ "id": "F1", "hero": "Hero.Aevik", "team": 0, "cell": [-1, 0, 0] }],
		"turns": [],
		"expect": [{ "type": "TurnsCompleted", "value": 0 }]
	})");

	FRTTestScenario Scenario;
	FString Error;
	TestFalse(TEXT("un numero fra le voci non passa"),
		URTScenarioLoader::LoadFromString(Malformato, Scenario, Error));
	TestTrue(FString::Printf(TEXT("e l'errore nomina il campo, invece di dire '%s'"), *Error),
		Error.Contains(TEXT("verifies")));
	return true;
}

#endif
