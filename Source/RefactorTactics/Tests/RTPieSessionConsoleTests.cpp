// I comandi esistono, e si chiamano come il runbook dice.
//
// Costa tre righe e prende due cose reali: un rename che lascerebbe il runbook a puntare a un comando
// che non c'e', e una translation unit caduta fuori dalla unity build — nel qual caso i comandi
// spariscono in silenzio e il difetto si scopre in PIE, con l'Editor aperto.

#include "Misc/AutomationTest.h"
#include "HAL/IConsoleManager.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionCommandsAreRegisteredTest,
	"RefactorTactics.PieSession.ConsoleCommandsAreRegistered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionCommandsAreRegisteredTest::RunTest(const FString&)
{
	for (const TCHAR* Nome : { TEXT("rt.Pie.Session"), TEXT("rt.Pie.Verdict"),
	                           TEXT("rt.Pie.Session.Abort") })
	{
		TestNotNull(*FString::Printf(TEXT("%s e' registrato"), Nome),
			IConsoleManager::Get().FindConsoleObject(Nome));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionCommandsAreDocumentedTest,
	"RefactorTactics.PieSession.ConsoleCommandsCarryTheirUsage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionCommandsAreDocumentedTest::RunTest(const FString&)
{
	// Chi conduce legge l'help dalla console, non il runbook: un comando senza uso scritto costringe
	// a tornare al documento proprio nel momento in cui si sta guardando lo schermo.
	const IConsoleObject* Sessione = IConsoleManager::Get().FindConsoleObject(TEXT("rt.Pie.Session"));
	if (!TestNotNull(TEXT("rt.Pie.Session esiste"), Sessione)) { return false; }

	const FString Help = Sessione->GetHelp();
	TestTrue(TEXT("l'help nomina il dry-run"), Help.Contains(TEXT("dry")));
	TestTrue(TEXT("e dice che senza argomenti non avvia"), Help.Contains(TEXT("senza avviare")));
	return true;
}

#endif
