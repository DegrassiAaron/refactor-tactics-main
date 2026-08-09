#include "Misc/AutomationTest.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Corpus golden di TurnLog (CP 12.6, issue #178) — la DIAGNOSI della divergenza.
 *
 * Il DoD e' esplicito sul punto che rende utile un corpus: «una divergenza **fallisce indicando turno, fase e
 * `ActionId`**: un test che dice solo "hash diverso" non serve a nessuno». Un confronto che risponde
 * `Divergence` e basta lascia a chi legge il lavoro di ricostruire *dove*, ed e' il motivo per cui i corpus
 * golden vengono rigenerati invece che letti.
 *
 * `URTTurnLogLibrary::CompareSerializedTraces` esiste gia' e risponde al livello del FORMATO
 * (`Identical`/`FormatMismatch`/`TopologyMismatch`/`Divergence`/`Unreadable`). Qui si aggiunge il livello
 * sotto: quale voce, in quale fase, di quale azione.
 */
namespace
{
	/** Voce minima: i campi che la diagnosi deve saper nominare. */
	FRTTurnLogEntry MakeGoldenEntry(ERTMatchPhase Phase, ERTLogCategory Category, uint8 Outcome,
		const TCHAR* ActionId, int32 Amount = 0)
	{
		FRTTurnLogEntry E;
		E.Phase = Phase;
		E.Category = Category;
		E.Outcome = Outcome;
		E.ActionId = FName(ActionId);
		E.Amount = Amount;
		E.SrcCell = FRTCellId(1, 0);
		E.TgtCell = FRTCellId(2, 0);
		return E;
	}

	/** Traccia di riferimento: tre voci, una per fase, con azioni distinguibili. */
	TArray<FRTTurnLogEntry> GoldenSample()
	{
		TArray<FRTTurnLogEntry> Log;
		Log.Add(MakeGoldenEntry(ERTMatchPhase::Prep, ERTLogCategory::Reaction,
			0, TEXT("Action.Guard")));
		Log.Add(MakeGoldenEntry(ERTMatchPhase::Blast, ERTLogCategory::Combat,
			static_cast<uint8>(ERTCombatOutcome::Hit), TEXT("Vektor.PulseShot"), 21));
		Log.Add(MakeGoldenEntry(ERTMatchPhase::Move, ERTLogCategory::Move,
			static_cast<uint8>(ERTMoveOutcome::Moved), TEXT("Action.Move"), 2));
		return Log;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGoldenCorpusDivergenceTest,
	"RefactorTactics.Simulation.GoldenCorpusDetectsDivergence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGoldenCorpusDivergenceTest::RunTest(const FString&)
{
	const TArray<FRTTurnLogEntry> Golden = GoldenSample();

	// Nessuna divergenza: nessuna descrizione. Una diagnosi che parla anche quando tutto va bene e' rumore
	// che insegna a ignorarla.
	TestTrue(TEXT("tracce identiche: nessuna divergenza da descrivere"),
		URTTurnLogLibrary::DescribeFirstDivergence(/*TurnNumber*/ 3, Golden, Golden).IsEmpty());

	// Divergenza di ESITO sulla voce di combattimento: stesso posto, stessa azione, esito diverso.
	{
		TArray<FRTTurnLogEntry> Actual = Golden;
		Actual[1].Outcome = static_cast<uint8>(ERTCombatOutcome::Lethal);

		const FString Diag = URTTurnLogLibrary::DescribeFirstDivergence(/*TurnNumber*/ 3, Golden, Actual);

		TestFalse(TEXT("una divergenza va descritta"), Diag.IsEmpty());
		TestTrue(TEXT("la diagnosi dice il TURNO"), Diag.Contains(TEXT("turno 3")));
		TestTrue(TEXT("la diagnosi dice la FASE"), Diag.Contains(TEXT("Blast")));
		TestTrue(TEXT("la diagnosi dice l'ACTIONID"), Diag.Contains(TEXT("Vektor.PulseShot")));
	}

	// La PRIMA divergenza, non l'ultima: chi legge parte dalla causa piu' probabile.
	{
		TArray<FRTTurnLogEntry> Actual = Golden;
		Actual[0].Outcome = 9;
		Actual[2].Amount = 99;

		const FString Diag = URTTurnLogLibrary::DescribeFirstDivergence(/*TurnNumber*/ 1, Golden, Actual);
		TestTrue(TEXT("riporta la prima divergenza (Prep), non le successive"), Diag.Contains(TEXT("Prep")));
		TestTrue(TEXT("e nomina la sua azione"), Diag.Contains(TEXT("Action.Guard")));
	}

	// Lunghezze diverse: e' una divergenza, e va detta come tale invece di leggere fuori dall'array.
	{
		TArray<FRTTurnLogEntry> Shorter = Golden;
		Shorter.RemoveAt(2);

		const FString Diag = URTTurnLogLibrary::DescribeFirstDivergence(/*TurnNumber*/ 7, Golden, Shorter);
		TestFalse(TEXT("meno voci del riferimento: divergenza"), Diag.IsEmpty());
		TestTrue(TEXT("e la diagnosi dice il turno"), Diag.Contains(TEXT("turno 7")));

		// E nel verso opposto, che e' il caso in cui una voce NUOVA compare senza essere attesa.
		TArray<FRTTurnLogEntry> Longer = Golden;
		Longer.Add(MakeGoldenEntry(ERTMatchPhase::Cleanup, ERTLogCategory::Environment,
			0, TEXT("Action.Ignite")));
		TestFalse(TEXT("piu' voci del riferimento: divergenza"),
			URTTurnLogLibrary::DescribeFirstDivergence(/*TurnNumber*/ 7, Golden, Longer).IsEmpty());
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
