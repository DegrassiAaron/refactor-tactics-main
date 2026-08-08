// Configurazione dell'AUTO-RUN: quale scenario parte al Play, e con che ritmo.
//
// Due cose che si sbagliano facilmente e che nessuno nota finche' non fanno perdere tempo davvero:
// - la PRECEDENZA fra la proprieta' persistente e l'override da riga di comando;
// - il timer di pianificazione lasciato a 30 secondi mentre si guarda uno scenario che dura un secondo.

#include "Misc/AutomationTest.h"
#include "RTGameMode.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnRules.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

#if WITH_DEV_AUTOMATION_TESTS

/** Definita in ScenarioHarness/RTTestConsole.cpp. */
extern TAutoConsoleVariable<FString> CVarRTTestScenario;

namespace
{
	// Nomi distinti da ogni altro file di test: la unity build condivide la translation unit.
	UWorld* MakeAutoRunWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyAutoRunWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/**
	 * Ripristina la console variable qualunque cosa succeda nel test.
	 *
	 * Senza, un test che la imposta e fallisce a meta' la lascerebbe sporca per TUTTI i test successivi —
	 * e in una unity build il successivo potrebbe essere qualsiasi cosa. Un test che rompe gli altri e' peggio
	 * di un test assente, perche' manda a cercare il difetto nel posto sbagliato.
	 */
	struct FScopedScenarioCVar
	{
		FString Saved;
		FScopedScenarioCVar() : Saved(CVarRTTestScenario.GetValueOnGameThread()) {}
		~FScopedScenarioCVar() { CVarRTTestScenario->Set(*Saved, ECVF_SetByCode); }
		void Set(const TCHAR* Value) { CVarRTTestScenario->Set(Value, ECVF_SetByCode); }
	};
}

/**
 * La console variable PREVALE sulla proprieta'.
 *
 * La proprieta' e' configurazione persistente («questo progetto, per ora, esegue questo scenario»); la console
 * variable e' l'intento estemporaneo di chi lancia («adesso, solo per questa volta, un altro»). Il piu'
 * specifico vince — la stessa regola di ogni override di configurazione. Se si invertisse, impostare la
 * proprieta' renderebbe impossibile eseguire uno scenario diverso da riga di comando, che e' esattamente cio'
 * che serve in CI.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAutoRunScenarioPrecedenceTest,
	"RefactorTactics.Scenario.AutoRunConsoleOverridesProperty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAutoRunScenarioPrecedenceTest::RunTest(const FString&)
{
	UWorld* World = MakeAutoRunWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("game mode"), GameMode)) { DestroyAutoRunWorld(World); return false; }

	FScopedScenarioCVar Guard;

	// Nessuna delle due: partita normale.
	Guard.Set(TEXT(""));
	GameMode->ScenarioToRun.Reset();
	TestTrue(TEXT("niente proprieta' e niente console -> partita normale"),
		GameMode->ResolveScenarioToRun().IsEmpty());

	// Solo la proprieta': e' lei a decidere. E' il caso «imposto una volta in BP_GameMode e al Play parte».
	GameMode->ScenarioToRun = TEXT("Movement.Basic");
	TestEqual(TEXT("solo proprieta' -> vale la proprieta'"),
		GameMode->ResolveScenarioToRun(), FString(TEXT("Movement.Basic")));

	// Entrambe: vince la console variable.
	Guard.Set(TEXT("Movement.Collision"));
	TestEqual(TEXT("console + proprieta' -> vince la console"),
		GameMode->ResolveScenarioToRun(), FString(TEXT("Movement.Collision")));

	// Solo la console: vale comunque, anche senza proprieta' (il caso della riga di comando).
	GameMode->ScenarioToRun.Reset();
	TestEqual(TEXT("solo console -> vale la console"),
		GameMode->ResolveScenarioToRun(), FString(TEXT("Movement.Collision")));

	DestroyAutoRunWorld(World);
	return true;
}

/**
 * Cambiare la durata della pianificazione vale SUBITO, non dal turno dopo.
 *
 * E' il punto della modifica: uno scenario lascia il turn manager in pianificazione, e col timer normale si
 * resterebbe a guardare un turno vuoto per mezzo minuto. Se il valore nuovo valesse solo dal turno successivo,
 * il primo mezzo minuto lo si aspetterebbe lo stesso — cioe' proprio nel turno in cui serviva non farebbe niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanningSecondsAppliesImmediatelyTest,
	"RefactorTactics.Turn.PlanningSecondsAppliesImmediately",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanningSecondsAppliesImmediatelyTest::RunTest(const FString&)
{
	UWorld* World = MakeAutoRunWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("turn manager"), TM)) { DestroyAutoRunWorld(World); return false; }

	TestEqual(TEXT("valore di partenza: 30 s, quelli della partita normale"), TM->GetPlanningSeconds(), 30.f);

	TM->SetPlanningSeconds(3.f);
	TestEqual(TEXT("il valore nuovo e' quello letto"), TM->GetPlanningSeconds(), 3.f);

	// 0 = nessuna scadenza: serve a fermare l'immagine e guardare con calma, e non deve essere confuso con
	// «un timer da zero secondi» che farebbe scattare il lock-in immediatamente.
	TM->SetPlanningSeconds(0.f);
	TestEqual(TEXT("zero e' ammesso (nessuna scadenza)"), TM->GetPlanningSeconds(), 0.f);

	// Un valore negativo non deve diventare un timer impossibile: si porta a zero.
	TM->SetPlanningSeconds(-5.f);
	TestEqual(TEXT("un negativo diventa zero, non un timer assurdo"), TM->GetPlanningSeconds(), 0.f);

	DestroyAutoRunWorld(World);
	return true;
}

/** Il ritmo dello scenario e' configurabile dal GameMode, con un default che non fa aspettare. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioPacingDefaultTest,
	"RefactorTactics.Scenario.AutoRunPacingHasShortDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioPacingDefaultTest::RunTest(const FString&)
{
	UWorld* World = MakeAutoRunWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("game mode"), GameMode)) { DestroyAutoRunWorld(World); return false; }

	// Il default deve essere BREVE ma non nullo: qualche secondo per vedere il campo, non mezzo minuto di
	// attesa ne' un salto istantaneo che non lascia guardare niente.
	TestTrue(TEXT("il ritmo di default e' positivo"), GameMode->ScenarioPlanningSeconds > 0.f);
	TestTrue(TEXT("ed e' molto piu' corto della pianificazione normale (30 s)"),
		GameMode->ScenarioPlanningSeconds < 10.f);

	DestroyAutoRunWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
