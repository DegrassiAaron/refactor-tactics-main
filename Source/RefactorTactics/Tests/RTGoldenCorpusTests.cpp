#include "Misc/AutomationTest.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/RTTestScenario.h"
#include "ScenarioHarness/RTTestResult.h"
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

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
 *
 * ⚠️ **SE QUESTI TEST SONO DIVENTATI ROSSI DOPO UN RITOCCO DI BILANCIAMENTO, non e' una regressione.**
 * ([D-083](../../../docs/decisions/RT_PDR_00_Decision_Log.md), issue `#413`.)
 *
 * Il corpus golden e' protetto da una **convenzione**, non da un controllo: i file `.rttl` di `Golden/` vivono
 * nello stesso repository delle regole, quindi cambiare una costante di combat o un'azione del catalogo li fa
 * divergere: la convenzione e' che si **rigenerano nello stesso commit** che cambia la regola.
 *
 * Il rilevamento c'e' ed e' buono — la diagnosi qui sotto nomina turno, fase e `ActionId`. Cio' che manca e'
 * l'**attribuzione**: un rebalance legittimo e un difetto del resolver si presentano **identici**, e a
 * distinguerli e' solo chi ha il commit in mano. Sarebbe il mestiere di `ContentManifestHash`/`RulesVersion`,
 * che D-083 ha rinviato alla v0.2 **con il perimetro gia' deciso** — entra cio' che il resolver legge — perche'
 * in v0.1 quel «chi ha il commit in mano» c'e' sempre, ed e' una persona sola.
 *
 * L'innesco per costruirle: **quando un archivio esce dalla macchina che l'ha prodotto** (condivisione, bug
 * report, CI che confronta run di build diverse). Li' l'attribuzione smette di essere locale.
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

	// Divergenza SOLO nell'azione, su una voce di movimento. E' il caso che la verifica di mutazione ha
	// scoperto: `DescribeEntry` non stampa l'ActionId per le voci `Move`, quindi la diagnosi mostrava «atteso
	// [X], trovato [X]» — due stringhe identiche accanto alla parola «diverge». Chi legge conclude che il
	// confronto e' rotto, e ha ragione.
	{
		TArray<FRTTurnLogEntry> Actual = Golden;
		Actual[2].ActionId = FName(TEXT("Action.Sprint"));

		const FString Diag = URTTurnLogLibrary::DescribeFirstDivergence(/*TurnNumber*/ 2, Golden, Actual);

		TestTrue(TEXT("nomina l'azione ATTESA"), Diag.Contains(TEXT("Action.Move")));
		TestTrue(TEXT("e quella TROVATA"), Diag.Contains(TEXT("Action.Sprint")));
	}

	// Divergenza su un campo che `DescribeEntry` NON stampa per quella categoria: qui `TgtCell` su una voce
	// di movimento che non e' `Moved` (il ramo «fermo: …» stampa solo la cella di partenza). Senza il
	// fallback sui campi grezzi la diagnosi direbbe «atteso [X], trovato [X]» — la stessa forma del difetto
	// trovato con l'ActionId, in un altro punto.
	{
		TArray<FRTTurnLogEntry> Base = Golden;
		Base[2].Outcome = static_cast<uint8>(ERTMoveOutcome::BlockedByUnit);

		TArray<FRTTurnLogEntry> Actual = Base;
		Actual[2].TgtCell = FRTCellId(9, 9);

		const FString Diag = URTTurnLogLibrary::DescribeFirstDivergence(/*TurnNumber*/ 4, Base, Actual);

		TestFalse(TEXT("una divergenza va descritta"), Diag.IsEmpty());
		TestTrue(TEXT("quando la prosa non distingue, mostra i campi grezzi"), Diag.Contains(TEXT("campi:")));
		TestTrue(TEXT("e il campo che diverge e' leggibile"), Diag.Contains(TEXT("tgt=(9,9,0)")));
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


// ---------------------------------------------------------------------------------------------------------
// Il corpus vero: partite di riferimento su disco
// ---------------------------------------------------------------------------------------------------------

/**
 * Rigenerazione del corpus: **esplicita, mai automatica** (DoD di CP 12.6).
 *
 * Un corpus che si riscrive da solo quando non torna non e' un corpus: e' un test che si adegua a qualunque
 * regressione. Con questa a 1 i file vengono riscritti e il confronto NON viene fatto; la PR che li rigenera
 * dichiara *perche'* l'esito e' cambiato.
 *
 *   UnrealEditor-Cmd.exe <progetto> -ExecCmds="rt.Test.RegenerateGolden 1; Automation RunTests RefactorTactics.Simulation.GoldenCorpusMatches; Quit" -nullrhi
 */
static TAutoConsoleVariable<int32> CVarRegenerateGolden(
	TEXT("rt.Test.RegenerateGolden"),
	0,
	TEXT("1 = riscrive il corpus golden invece di confrontarlo. Mai in automatico: la PR dichiara il perche'."),
	ECVF_Default);

namespace
{
	/** Radice del corpus. Sta nel SORGENTE, accanto ai test che lo leggono, non in Content. */
	FString GoldenRoot()
	{
		return FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/RefactorTactics/Tests/Golden"));
	}

	FString GoldenTurnPath(const FString& ScenarioId, int32 TurnNumber)
	{
		return FPaths::Combine(GoldenRoot(), ScenarioId, FString::Printf(TEXT("turn-%02d.rttl"), TurnNumber));
	}

	UWorld* MakeGoldenWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyGoldenWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Le partite di riferimento. Poche e deterministiche: un corpus lento non viene eseguito. */
	const TCHAR* GoldenScenarioIds[] = { TEXT("Movement.Collision"), TEXT("Movement.Basic") };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGoldenCorpusMatchesTest,
	"RefactorTactics.Simulation.GoldenCorpusMatches",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGoldenCorpusMatchesTest::RunTest(const FString&)
{
	const bool bRegenerate = CVarRegenerateGolden.GetValueOnAnyThread() != 0;

	for (const TCHAR* ScenarioId : GoldenScenarioIds)
	{
		FString Error;
		const FString Path = URTScenarioIndex::ResolvePath(ScenarioId, Error);
		FRTTestScenario Scenario;
		if (Path.IsEmpty() || !URTScenarioLoader::LoadFromFile(Path, Scenario, Error))
		{
			AddError(FString::Printf(TEXT("scenario '%s' non caricabile: %s"), ScenarioId, *Error));
			continue;
		}

		UWorld* World = MakeGoldenWorld();
		if (!World)
		{
			AddError(TEXT("world di prova non creato"));
			continue;
		}
		const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
		DestroyGoldenWorld(World);

		// Uno scenario che non gira produrrebbe zero tracce, e un confronto su zero tracce e' verde per il
		// motivo sbagliato.
		if (Result.Outcome == ERTTestOutcome::Error || Result.Outcome == ERTTestOutcome::Blocked)
		{
			AddError(FString::Printf(TEXT("'%s' non eseguibile (%s): %s%s"),
				ScenarioId, *Result.OutcomeString(), *Result.ErrorMessage, *Result.BlockedReason));
			continue;
		}
		if (!TestTrue(FString::Printf(TEXT("'%s' ha prodotto almeno un turno"), ScenarioId),
			Result.TurnTraces.Num() > 0))
		{
			continue;
		}

		for (int32 i = 0; i < Result.TurnTraces.Num(); ++i)
		{
			const int32 TurnNumber = i + 1;
			const FString TurnPath = GoldenTurnPath(ScenarioId, TurnNumber);
			const TArray<uint8>& Fresh = Result.TurnTraces[i].Bytes;

			if (bRegenerate)
			{
				if (!FFileHelper::SaveArrayToFile(Fresh, *TurnPath))
				{
					AddError(FString::Printf(TEXT("corpus non scrivibile: %s"), *TurnPath));
				}
				continue;
			}

			TArray<uint8> Golden;
			if (!FFileHelper::LoadFileToArray(Golden, *TurnPath))
			{
				AddError(FString::Printf(
					TEXT("manca la traccia di riferimento %s — rigenerala con `rt.Test.RegenerateGolden 1`, ")
					TEXT("e dichiara nella PR perche' l'esito e' cambiato"), *TurnPath));
				continue;
			}

			const ERTTraceComparison Verdict = URTTurnLogLibrary::CompareSerializedTraces(Golden, Fresh);
			if (Verdict == ERTTraceComparison::Identical)
			{
				continue;
			}

			// Il verdetto dice CHE COSA e' successo; la diagnosi dice DOVE. Un corpus che si limita al primo
			// finisce rigenerato invece che letto.
			FString Detail;
			TArray<FRTTurnLogEntry> GoldenEntries;
			TArray<FRTTurnLogEntry> FreshEntries;
			if (URTTurnLogLibrary::DeserializeTurnLog(Golden, GoldenEntries)
				&& URTTurnLogLibrary::DeserializeTurnLog(Fresh, FreshEntries))
			{
				Detail = URTTurnLogLibrary::DescribeFirstDivergence(TurnNumber, GoldenEntries, FreshEntries);
			}

			AddError(FString::Printf(TEXT("'%s' diverge dal corpus (%s). %s"),
				ScenarioId,
				Verdict == ERTTraceComparison::Divergence ? TEXT("divergenza")
					: Verdict == ERTTraceComparison::FormatMismatch ? TEXT("formato diverso")
					: Verdict == ERTTraceComparison::TopologyMismatch ? TEXT("topologia diversa")
					: TEXT("traccia illeggibile"),
				Detail.IsEmpty() ? TEXT("(nessuna differenza voce per voce: e' il contesto a divergere)") : *Detail));
		}
	}

	if (bRegenerate)
	{
		AddWarning(TEXT("corpus RIGENERATO: nessun confronto eseguito. Dichiara nella PR perche' l'esito e' cambiato."));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
