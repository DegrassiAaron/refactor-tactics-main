// Il CORPUS come test: ogni scenario versionato viene eseguito, non solo caricato.
//
// Prima di questo file uno scenario diventava un test solo se qualcuno gli scriveva accanto la propria
// `IMPLEMENT_SIMPLE_AUTOMATION_TEST`. Chi ne aggiungeva uno senza ricordarsene otteneva un file che sembrava
// coperto — `ShippedScenariosAreValid` lo carica, quindi il verde c'e' — mentre nessuno lo eseguiva mai.
// E' successo: diciassette scenari sono stati committati cosi'.
//
// Qui il corpus si scopre da solo. Aggiungere un file JSON basta perche' venga eseguito, e questo test e'
// l'unico posto da cambiare quando cambia la regola su cosa e' un esito accettabile.

#include "Misc/AutomationTest.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nomi distinti da ogni altro file di test: la unity build condivide la translation unit.
	UWorld* MakeCorpusWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyCorpusWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Carica ed esegue uno scenario per ID. Mondo creato e distrutto qui: uno scenario non ne eredita da un altro. */
	bool RunCorpusScenario(FAutomationTestBase& Test, const FString& ScenarioId, FRTTestResult& OutResult)
	{
		FString ResolveError;
		const FString Path = URTScenarioIndex::ResolvePath(ScenarioId, ResolveError);
		if (Path.IsEmpty())
		{
			Test.AddError(FString::Printf(TEXT("%s: l'indice non lo risolve — %s"), *ScenarioId, *ResolveError));
			return false;
		}

		FRTTestScenario Scenario;
		FString LoadError;
		if (!URTScenarioLoader::LoadFromFile(Path, Scenario, LoadError))
		{
			Test.AddError(FString::Printf(TEXT("%s: non si carica — %s"), *ScenarioId, *LoadError));
			return false;
		}

		UWorld* World = MakeCorpusWorld();
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: mondo"), *ScenarioId), World))
		{
			return false;
		}
		OutResult = URTScenarioRunner::Run(World, Scenario);
		DestroyCorpusWorld(World);
		return true;
	}
}

/**
 * OGNI scenario versionato deve **girare**, e finire in uno dei due esiti che significano «il gioco sta bene»:
 *
 * - `PASS`    — ha giocato tutti i turni e le assertion tengono;
 * - `BLOCKED` — si e' fermato su una capability che non esiste ancora, e l'ha nominata.
 *
 * `BLOCKED` e' accettabile per costruzione: e' il meccanismo che permette di versionare uno scenario **prima**
 * dei suoi sistemi. Se lo trattassimo come rosso, l'unica strategia razionale sarebbe non scrivere piu' scenari
 * in anticipo — e si perderebbe il solo modo che il progetto ha di dichiarare una feature futura in forma
 * eseguibile.
 *
 * `FAIL` e' un difetto del GIOCO. `ERROR` e' un difetto dello SCENARIO. La distinzione e' gia' nel tipo di
 * esito, e qui viene riportata nel messaggio invece di essere appiattita su «non passa».
 *
 * Gli scenari con tag `expected-fail` sono esclusi: il loro mestiere e' fallire, e li verifica il test qui sotto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioCorpusRunsTest,
	"RefactorTactics.Scenario.EveryShippedScenarioRuns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioCorpusRunsTest::RunTest(const FString&)
{
	const TArray<FString> AllIds = URTScenarioIndex::ListIds(FString(), FString());
	const TArray<FString> ExpectedToFail = URTScenarioIndex::ListIds(TEXT("expected-fail"), FString());

	// Un corpus vuoto farebbe passare questo test senza eseguire niente: e' il modo piu' silenzioso in cui una
	// rete di sicurezza puo' smettere di esistere. La soglia e' bassa apposta — dice «l'indice funziona», non
	// «il corpus e' abbastanza grande».
	if (!TestTrue(TEXT("l'indice trova gli scenari"), AllIds.Num() >= 10))
	{
		AddError(FString::Printf(TEXT("l'indice ha restituito %d scenari: o il corpus e' sparito, o ListIds non elenca piu'"), AllIds.Num()));
		return false;
	}

	int32 Passed = 0;
	int32 Blocked = 0;

	for (const FString& Id : AllIds)
	{
		if (ExpectedToFail.Contains(Id))
		{
			continue;
		}

		FRTTestResult Result;
		if (!RunCorpusScenario(*this, Id, Result))
		{
			continue; // errore gia' riportato con il motivo
		}

		switch (Result.Outcome)
		{
		case ERTTestOutcome::Pass:
			++Passed;
			break;

		case ERTTestOutcome::Blocked:
			// Non e' un fallimento, ma non deve nemmeno passare in silenzio: il motivo compare nel log, cosi'
			// chi implementa la capability sa gia' quali scenari si accendono quando atterra.
			++Blocked;
			AddInfo(FString::Printf(TEXT("%s: BLOCKED — %s"), *Id, *Result.BlockedReason));
			break;

		case ERTTestOutcome::Fail:
			{
				// Il primo assert caduto, col valore reale: senza, si sa che qualcosa e' rosso ma non cosa,
				// e si finisce a rieseguire lo scenario a mano per leggere un dato che il runner aveva gia'.
				FString First = TEXT("(nessuna assertion registrata)");
				for (const FRTAssertionResult& A : Result.Assertions)
				{
					if (!A.bPassed)
					{
						First = FString::Printf(TEXT("%s — atteso %s, ottenuto %s (turno %d)"),
							*A.Description, *A.Expected, *A.Actual, A.Turn);
						break;
					}
				}
				AddError(FString::Printf(TEXT("%s: FAIL (difetto del GIOCO) — %s"), *Id, *First));
			}
			break;

		case ERTTestOutcome::Error:
		default:
			AddError(FString::Printf(TEXT("%s: ERROR (difetto dello SCENARIO) — %s"), *Id, *Result.ErrorMessage));
			break;
		}
	}

	AddInfo(FString::Printf(TEXT("corpus eseguito: %d PASS, %d BLOCKED, %d dichiarati expected-fail"),
		Passed, Blocked, ExpectedToFail.Num()));
	return true;
}

/**
 * Uno scenario `expected-fail` deve **fallire davvero**.
 *
 * Il tag esiste per dimostrare che il report diagnostica un fallimento; se il gioco cambiasse in modo da farlo
 * passare, il tag diventerebbe una bugia e — peggio — l'unica prova che l'harness sa dire «rosso» smetterebbe
 * di provarlo, senza che nulla diventi rosso. E' il caso in cui il verde e' il difetto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioExpectedFailTest,
	"RefactorTactics.Scenario.ExpectedFailScenariosReallyFail",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioExpectedFailTest::RunTest(const FString&)
{
	const TArray<FString> ExpectedToFail = URTScenarioIndex::ListIds(TEXT("expected-fail"), FString());
	if (!TestTrue(TEXT("esiste almeno uno scenario expected-fail"), ExpectedToFail.Num() > 0))
	{
		return false;
	}

	for (const FString& Id : ExpectedToFail)
	{
		FRTTestResult Result;
		if (!RunCorpusScenario(*this, Id, Result))
		{
			continue;
		}

		// FAIL, non ERROR: deve fallire perche' il GIOCO non fa quel che lo scenario si aspetta, non perche'
		// lo scenario sia rotto. Un `expected-fail` che va in ERROR ha smesso di dimostrare quel che doveva.
		TestEqual(FString::Printf(TEXT("%s: dichiarato expected-fail, quindi FAIL"), *Id),
			Result.OutcomeString(), FString(TEXT("FAIL")));
	}
	return true;
}

// =====================================================================================================
// `#601`/`#602` — lo scenario della reazione dichiarata deve PASSARE, non solo «non fallire».
//
// `EveryShippedScenarioRuns` accetta `BLOCKED` per costruzione, ed e' giusto: e' il meccanismo che permette
// di versionare uno scenario prima della sua capability. Ma per uno scenario che oggi **gira davvero**
// quell'accettazione e' troppo larga: se un domani `ReactionPlanning` tornasse indisponibile — o il
// produttore di `PlannedReactionAbility` sparisse — il file scivolerebbe in `BLOCKED` e la suite resterebbe
// verde, senza che nessuno sappia che il giocatore ha smesso di poter armare una reazione.
//
// Qui l'esito atteso e' pinnato: `Pass`. E' la stessa disciplina dei test che pinnano un limite, con il
// segno opposto — quelli diventano rossi quando il limite cade, questo quando una capacita' si perde.
// =====================================================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioAnchorRunsTest,
	"RefactorTactics.Scenario.DeclaredReactionScenarioPasses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioAnchorRunsTest::RunTest(const FString&)
{
	const FString Id = TEXT("Spec.Reaction.AnchorCancelsPush");

	FRTTestResult Result;
	if (!RunCorpusScenario(*this, Id, Result))
	{
		return false; // motivo gia' riportato
	}

	if (Result.Outcome == ERTTestOutcome::Blocked)
	{
		AddError(FString::Printf(
			TEXT("%s e' BLOCKED (%s): la capability c'era quando lo scenario e' stato scritto. ")
			TEXT("O il produttore di `PlannedReactionAbility` e' sparito, o l'elenco delle capability e' regredito."),
			*Id, *Result.BlockedReason));
		return false;
	}

	if (Result.Outcome != ERTTestOutcome::Pass)
	{
		FString First = TEXT("(nessuna assertion registrata)");
		for (const FRTAssertionResult& A : Result.Assertions)
		{
			if (!A.bPassed)
			{
				First = FString::Printf(TEXT("%s: atteso %s, osservato %s"), *A.Description, *A.Expected, *A.Actual);
				break;
			}
		}
		AddError(FString::Printf(TEXT("%s non passa: %s"), *Id, *First));
		return false;
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
