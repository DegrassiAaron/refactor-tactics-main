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
#include "Turn/RTTurnLogLibrary.h"
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

/**
 * I campi di contesto della v6 sono VALORIZZATI su una partita vera (#405).
 *
 * Il formato li porta dal merge di #396, ma per un po' nessuno li ha scritti: `ARTTurnManager` popolava solo
 * i sette campi preesistenti, e ogni traccia reale li portava a `0`. Un campo che nessuno produce e' peggio
 * di un campo assente — sembra una capacita' e non lo e'.
 *
 * Il test gira sul percorso di gioco REALE (Scenario Harness -> resolver -> TurnLog serializzato) e legge la
 * traccia del primo turno cosi' come finirebbe in un corpus golden. Non costruisce voci a mano: verificare i
 * campi su una voce fabbricata dal test direbbe solo che la struct ha dei membri.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogProducerStampsContextTest,
	"RefactorTactics.TurnLog.ProducerStampsContextOnRealMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogProducerStampsContextTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!LoadDeterminismScenario(*this, TEXT("Movement.Basic"), Scenario)) { return false; }

	const FRTTestResult Result = RunIsolated(Scenario);
	if (Result.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("esecuzione fallita: %s"), *Result.ErrorMessage));
		return false;
	}
	if (!TestTrue(TEXT("lo scenario ha prodotto almeno una traccia"), Result.TurnTraces.Num() > 0))
	{
		return false;
	}

	TArray<FRTTurnLogEntry> Entries;
	if (!TestTrue(TEXT("la traccia del turno 1 si rilegge"),
		URTTurnLogLibrary::DeserializeTurnLog(Result.TurnTraces[0].Bytes, Entries)))
	{
		return false;
	}
	if (!TestTrue(TEXT("il turno 1 ha prodotto voci"), Entries.Num() > 0)) { return false; }

	// Il numero di turno e' un dato di CONTESTO: vale per ogni voce, comprese quelle ambientali.
	int32 WrongTurn = 0;
	for (const FRTTurnLogEntry& E : Entries)
	{
		if (E.TurnNumber != 1) { ++WrongTurn; }
	}
	TestEqual(TEXT("ogni voce del turno 1 dichiara il turno 1"), WrongTurn, 0);

	// `UnitId` su una traccia vera: chi ha AGITO, non chi era su quella cella ([D-063]). Le voci di movimento
	// e combattimento hanno sempre un attore — se dopo il wiring ne restasse una a `0`, sarebbe un sito di
	// emissione che ha ereditato lo zero invece di dichiarare la propria unita', ed e' esattamente il difetto
	// che il parametro obbligatorio di `AppendLogEntry` esiste per rendere impossibile.
	int32 ActorlessCombat = 0;
	int32 WithActor = 0;
	for (const FRTTurnLogEntry& E : Entries)
	{
		const bool bHasActor = (E.Category == ERTLogCategory::Move || E.Category == ERTLogCategory::Combat);
		if (bHasActor && E.UnitId <= 0) { ++ActorlessCombat; }
		if (E.UnitId > 0) { ++WithActor; }
	}
	TestEqual(TEXT("nessuna voce di movimento o combattimento resta senza attore"), ActorlessCombat, 0);
	TestTrue(TEXT("almeno una voce dichiara la propria unita' (altrimenti il test non prova niente)"),
		WithActor > 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
