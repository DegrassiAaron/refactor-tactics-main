// Gate di DETERMINISMO della simulazione (CP 12.1).
//
// Il catalogo di bilanciamento nomina questi test per nome e li rende vincolanti: sono gli ultimi due dei
// dieci richiesti. La proprieta' che verificano e' quella su cui poggia tutto il resto — se la stessa
// partita giocata due volte finisce diversamente, nessun altro test dice piu' niente, perche' il suo verde
// potrebbe essere un caso.
//
// Si appoggiano allo Scenario Harness invece di ricostruire una partita a mano: cosi' il determinismo e'
// verificato sul percorso di gioco REALE (piani -> snapshot -> resolver -> stato), non su una simulazione
// scritta apposta per passare.

#include "Misc/AutomationTest.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Algo/Reverse.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nomi distinti da ogni altro file di test: la unity build condivide la translation unit.
	UWorld* MakeDeterminismWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyDeterminismWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Esegue lo scenario in un mondo NUOVO e lo distrugge: nessuno stato sopravvive fra una prova e l'altra. */
	FRTTestResult RunIsolated(const FRTTestScenario& Scenario)
	{
		UWorld* World = MakeDeterminismWorld();
		if (!World)
		{
			return FRTTestResult();
		}
		const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
		DestroyDeterminismWorld(World);
		return Result;
	}

	bool LoadDeterminismScenario(FAutomationTestBase& Test, const TCHAR* Id, FRTTestScenario& Out)
	{
		FString Error;
		const FString Path = URTScenarioIndex::ResolvePath(Id, Error);
		if (Path.IsEmpty() || !URTScenarioLoader::LoadFromFile(Path, Out, Error))
		{
			Test.AddError(FString::Printf(TEXT("scenario '%s' non caricabile: %s"), Id, *Error));
			return false;
		}
		return true;
	}
}

/**
 * Stessa scenario + stesse definizioni ⇒ stesso stato finale, su **100 ripetizioni**.
 *
 * Cento e non due perche' il non-determinismo che conta e' quello raro: un `TMap` iterato in ordine diverso,
 * un puntatore usato come chiave, un indice che dipende dall'ordine di spawn. Con due ripetizioni un difetto
 * del genere passa quasi sempre; con cento si vede.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSimulationDeterministicReplayTest,
	"RefactorTactics.Simulation.DeterministicReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSimulationDeterministicReplayTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!LoadDeterminismScenario(*this, TEXT("Movement.Collision"), Scenario)) { return false; }

	const FRTTestResult First = RunIsolated(Scenario);
	if (First.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("la prima esecuzione e' fallita: %s"), *First.ErrorMessage));
		return false;
	}
	// Un hash a zero significherebbe «nessuno stato»: confrontare zeri fra loro non proverebbe nulla.
	TestNotEqual(TEXT("lo stato finale produce un hash reale"), First.StateHash, 0u);

	constexpr int32 Repetitions = 100;
	int32 Divergences = 0;
	for (int32 I = 1; I < Repetitions; ++I)
	{
		const FRTTestResult Again = RunIsolated(Scenario);
		if (Again.StateHash != First.StateHash || Again.OutcomeString() != First.OutcomeString())
		{
			++Divergences;
			if (Divergences <= 3) // le prime tre bastano a diagnosticare; oltre e' rumore
			{
				AddError(FString::Printf(
					TEXT("divergenza alla ripetizione %d: hash %08x invece di %08x, esito '%s' invece di '%s'"),
					I, Again.StateHash, First.StateHash, *Again.OutcomeString(), *First.OutcomeString()));
			}
		}
	}

	TestEqual(FString::Printf(TEXT("nessuna divergenza su %d ripetizioni"), Repetitions), Divergences, 0);
	return true;
}

/**
 * L'ORDINE dell'input non cambia l'esito.
 *
 * E' la stessa scenario con gli intent e le unita' elencati al contrario. L'invariante #3 dice che l'ordine
 * dell'array non deve cambiare il risultato: qui lo si verifica dall'esterno, sul percorso di gioco intero,
 * invece che sulla singola funzione pura. Se questo test diventasse rosso, il resolver starebbe usando
 * l'ordine di iterazione come regola di gioco.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSimulationChecksumPermutationTest,
	"RefactorTactics.Simulation.ChecksumStableAcrossPermutations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSimulationChecksumPermutationTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!LoadDeterminismScenario(*this, TEXT("Movement.Collision"), Scenario)) { return false; }
	if (!TestTrue(TEXT("lo scenario ha almeno due unita' da permutare"), Scenario.Units.Num() >= 2))
	{
		return false;
	}

	const FRTTestResult Straight = RunIsolated(Scenario);
	if (Straight.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("esecuzione diretta fallita: %s"), *Straight.ErrorMessage));
		return false;
	}

	// Permutazione: unita' e intent al contrario. Nient'altro cambia — stessa mappa, stesse celle, stesse
	// aspettative — quindi ogni differenza nell'esito verrebbe dall'ordine, che e' cio' che si vuole escludere.
	FRTTestScenario Reversed = Scenario;
	Algo::Reverse(Reversed.Units);
	for (FRTScenarioTurn& Turn : Reversed.Turns)
	{
		Algo::Reverse(Turn.Intents);
	}

	const FRTTestResult Permuted = RunIsolated(Reversed);
	if (Permuted.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("esecuzione permutata fallita: %s"), *Permuted.ErrorMessage));
		return false;
	}

	TestEqual(TEXT("stesso esito"), Permuted.OutcomeString(), Straight.OutcomeString());
	TestEqual(FString::Printf(TEXT("stesso stato finale (%08x vs %08x)"), Permuted.StateHash, Straight.StateHash),
		Permuted.StateHash, Straight.StateHash);
	TestEqual(TEXT("stesso numero di turni"), Permuted.TurnsPlayed, Straight.TurnsPlayed);
	return true;
}

/**
 * L'hash NON e' una costante travestita da verifica.
 *
 * I due test sopra confrontano hash fra loro: passerebbero anche se `HashFinalState` restituisse sempre lo
 * stesso numero, e non se ne accorgerebbe nessuno. Qui si esegue una situazione **diversa** e si pretende un
 * hash diverso — la controprova che rende significativi gli altri due.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSimulationHashDistinguishesStatesTest,
	"RefactorTactics.Simulation.StateHashDistinguishesOutcomes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSimulationHashDistinguishesStatesTest::RunTest(const FString&)
{
	FRTTestScenario Moving;
	if (!LoadDeterminismScenario(*this, TEXT("Movement.Basic"), Moving)) { return false; }

	// Stessa scenario senza l'intento di movimento: l'unita' resta alla partenza. Stato finale diverso.
	FRTTestScenario Still = Moving;
	for (FRTScenarioTurn& Turn : Still.Turns)
	{
		Turn.Intents.Reset();
	}

	const FRTTestResult WithMove = RunIsolated(Moving);
	const FRTTestResult WithoutMove = RunIsolated(Still);

	TestNotEqual(TEXT("posizioni finali diverse -> hash diversi"), WithMove.StateHash, WithoutMove.StateHash);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
