// Runner degli scenari + report.
//
// I due test che contano davvero sono la coppia PASS/FAIL: un harness che sa dire solo «verde» non serve a
// niente, perche' il suo lavoro comincia quando qualcosa si rompe. Il test del FAIL verifica che il report
// dica **cosa** ci si aspettava e **cosa** e' successo, non solo che e' fallito.

#include "Misc/AutomationTest.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/RTTestReportWriter.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nomi distinti da ogni altro file di test: la unity build condivide la translation unit.
	UWorld* MakeRunnerWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyRunnerWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Carica uno scenario versionato per ID; fallisce il test se manca (e' un ERROR d'ambiente, non un FAIL). */
	bool LoadShippedScenario(FAutomationTestBase& Test, const TCHAR* ScenarioId, FRTTestScenario& Out)
	{
		FString Error;
		const FString Path = URTScenarioIndex::ResolvePath(ScenarioId, Error);
		if (Path.IsEmpty() || !URTScenarioLoader::LoadFromFile(Path, Out, Error))
		{
			Test.AddError(FString::Printf(TEXT("scenario '%s' non caricabile (%s): %s"), ScenarioId, *Path, *Error));
			return false;
		}
		return true;
	}
}

/**
 * Movement.Basic esegue e PASSA: l'unita' finisce sulla cella dichiarata, passando dal percorso di gioco
 * reale (piano -> LockInAndResolve -> resolver), non da un `SetActorLocation`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioRunnerMovementBasicTest,
	"RefactorTactics.Scenario.RunnerMovementBasicPasses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioRunnerMovementBasicTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!LoadShippedScenario(*this, TEXT("Movement.Basic"), Scenario)) { return false; }

	UWorld* World = MakeRunnerWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	DestroyRunnerWorld(World);

	if (Result.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("ERROR invece di PASS: %s"), *Result.ErrorMessage));
		return false;
	}
	TestEqual(TEXT("esito PASS"), Result.OutcomeString(), FString(TEXT("PASS")));
	TestEqual(TEXT("un turno giocato"), Result.TurnsPlayed, 1);
	TestEqual(TEXT("nessuna assertion fallita"), Result.FailedCount(), 0);
	TestTrue(TEXT("almeno una assertion valutata"), Result.Assertions.Num() > 0);

	// L'unita' si e' MOSSA davvero: se il runner non avesse toccato il gioco, l'actual sarebbe la cella di
	// partenza. Questo distingue «il test passa» da «il test non ha fatto niente».
	const FRTAssertionResult* AtCell = Result.Assertions.FindByPredicate(
		[](const FRTAssertionResult& A) { return A.Kind == ERTAssertionKind::UnitAtCell; });
	if (TestNotNull(TEXT("c'e' una assertion UnitAtCell"), AtCell))
	{
		TestEqual(TEXT("l'unita' e' sulla cella attesa"), AtCell->Actual, AtCell->Expected);
		TestNotEqual(TEXT("e non e' rimasta alla partenza"), AtCell->Actual, FRTCellId(-2, 0, 0).ToString());
	}
	return true;
}

/**
 * Il FAIL intenzionale: la simulazione va a termine, l'aspettativa non e' soddisfatta, e il report lo dice
 * con expected/actual. E' `Fail`, **non** `Error`: il gioco ha funzionato, e' l'aspettativa a non tornare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioRunnerDiagnosesFailureTest,
	"RefactorTactics.Scenario.RunnerDiagnosesFailure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioRunnerDiagnosesFailureTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!LoadShippedScenario(*this, TEXT("Movement.BasicFailsOnPurpose"), Scenario)) { return false; }

	UWorld* World = MakeRunnerWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	DestroyRunnerWorld(World);

	TestEqual(TEXT("esito FAIL (non ERROR: la simulazione e' andata a termine)"),
		Result.OutcomeString(), FString(TEXT("FAIL")));
	TestEqual(TEXT("nessun messaggio d'errore infrastrutturale"), Result.ErrorMessage, FString());
	TestEqual(TEXT("una assertion fallita"), Result.FailedCount(), 1);

	// Il punto del test: il report DIAGNOSTICA invece di dire solo «fallito».
	const FRTAssertionResult* Failed = Result.Assertions.FindByPredicate(
		[](const FRTAssertionResult& A) { return !A.bPassed; });
	if (TestNotNull(TEXT("c'e' una assertion fallita"), Failed))
	{
		TestFalse(TEXT("l'atteso e' riportato"), Failed->Expected.IsEmpty());
		TestFalse(TEXT("l'osservato e' riportato"), Failed->Actual.IsEmpty());
		TestNotEqual(TEXT("atteso e osservato differiscono davvero"), Failed->Expected, Failed->Actual);
		TestFalse(TEXT("si sa quale assertion era"), Failed->Description.IsEmpty());
	}
	return true;
}

/** Uno scenario invalido produce ERROR, mai FAIL: chi legge deve sapere che il difetto e' nel test. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioRunnerInvalidIsErrorTest,
	"RefactorTactics.Scenario.RunnerInvalidScenarioIsError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioRunnerInvalidIsErrorTest::RunTest(const FString&)
{
	UWorld* World = MakeRunnerWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	FRTTestScenario Broken;
	Broken.ScenarioId = TEXT("Broken.NoUnits");
	Broken.MapRadius = 3; // nessuna unita', nessuna assertion

	const FRTTestResult Result = URTScenarioRunner::Run(World, Broken);
	DestroyRunnerWorld(World);

	TestEqual(TEXT("esito ERROR"), Result.OutcomeString(), FString(TEXT("ERROR")));
	TestFalse(TEXT("il motivo e' riportato"), Result.ErrorMessage.IsEmpty());
	TestEqual(TEXT("nessuna assertion valutata"), Result.Assertions.Num(), 0);
	return true;
}

/** Il report contiene cio' che serve a diagnosticare senza aprire il log del motore. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioReportContentTest,
	"RefactorTactics.Scenario.ReportIsSelfSufficient",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioReportContentTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!LoadShippedScenario(*this, TEXT("Movement.BasicFailsOnPurpose"), Scenario)) { return false; }

	UWorld* World = MakeRunnerWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	DestroyRunnerWorld(World);

	const FString Json = URTTestReportWriter::ToJson(Result, TEXT("test-run"));

	// Si verifica il JSON PARSATO, non con grep di stringhe: un test che cerca `"result":"FAIL"` si rompe
	// appena il writer cambia spaziatura, e fallirebbe per un motivo che non c'entra con cio' che verifica.
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (!TestTrue(TEXT("il report e' JSON valido"), FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid()))
	{
		AddError(FString::Printf(TEXT("contenuto: %s"), *Json));
		return false;
	}

	int32 Schema = 0;
	TestTrue(TEXT("dichiara la versione dello schema"), Root->TryGetNumberField(TEXT("schemaVersion"), Schema));
	TestEqual(TEXT("schema alla versione corrente"), Schema, URTTestReportWriter::SchemaVersion);

	FString Field;
	TestTrue(TEXT("dice quale scenario"), Root->TryGetStringField(TEXT("scenario"), Field));
	TestEqual(TEXT("scenario giusto"), Field, FString(TEXT("Movement.BasicFailsOnPurpose")));
	TestTrue(TEXT("dice l'esito"), Root->TryGetStringField(TEXT("result"), Field));
	TestEqual(TEXT("esito FAIL"), Field, FString(TEXT("FAIL")));

	int32 Turns = -1;
	TestTrue(TEXT("riporta i turni giocati"), Root->TryGetNumberField(TEXT("turnsPlayed"), Turns));
	TestTrue(TEXT("almeno un turno"), Turns >= 1);

	int32 SeedField = -1;
	TestTrue(TEXT("riporta il seed dichiarato"), Root->TryGetNumberField(TEXT("seed"), SeedField));

	// Le assertion fallite hanno un campo DEDICATO: chi diagnostica non deve filtrare l'elenco completo.
	const TArray<TSharedPtr<FJsonValue>>* Failures = nullptr;
	if (TestTrue(TEXT("elenca le assertion fallite"), Root->TryGetArrayField(TEXT("failures"), Failures)))
	{
		TestEqual(TEXT("una sola fallita"), Failures->Num(), 1);
		const TSharedPtr<FJsonObject> First = (*Failures)[0]->AsObject();
		if (TestTrue(TEXT("la voce e' un oggetto"), First.IsValid()))
		{
			FString Expected, Actual;
			TestTrue(TEXT("riporta l'atteso"), First->TryGetStringField(TEXT("expected"), Expected));
			TestTrue(TEXT("riporta l'osservato"), First->TryGetStringField(TEXT("actual"), Actual));
			TestNotEqual(TEXT("atteso e osservato differiscono"), Expected, Actual);
		}
	}

	// Un report di FAIL non deve portare il campo `error`, che appartiene solo agli ERROR: se ci fosse,
	// chi legge non saprebbe piu' distinguere un difetto del gioco da uno del test.
	TestFalse(TEXT("un FAIL non ha il campo error"), Root->HasField(TEXT("error")));

	// E su disco: la cartella della run esiste e il file e' leggibile.
	FString Dir, Error;
	if (TestTrue(TEXT("report scritto su disco"), URTTestReportWriter::Write(Result, TEXT("selftest"), Dir, Error)))
	{
		FString Loaded;
		TestTrue(TEXT("result.json rileggibile"),
			FFileHelper::LoadFileToString(Loaded, *FPaths::Combine(Dir, TEXT("result.json"))));
		TestFalse(TEXT("non e' vuoto"), Loaded.IsEmpty());
	}
	else
	{
		AddError(Error);
	}
	return true;
}

/**
 * `RunById` e' il punto d'ingresso di console e auto-run: carica per ID, esegue e **scrive il report**.
 * Va coperto qui perche' i comandi console non sono verificabili headless senza caricare una mappa, e la
 * parte che conta — «eseguire lascia sempre una traccia leggibile» — e' in questa funzione, non nel comando.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioRunByIdWritesReportTest,
	"RefactorTactics.Scenario.RunByIdWritesReport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioRunByIdWritesReportTest::RunTest(const FString&)
{
	UWorld* World = MakeRunnerWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	FString ReportDir;
	const FRTTestResult Result = URTScenarioRunner::RunById(World, TEXT("Movement.Basic"), ReportDir);
	DestroyRunnerWorld(World);

	TestEqual(TEXT("esito PASS"), Result.OutcomeString(), FString(TEXT("PASS")));
	if (TestFalse(TEXT("la cartella della run e' stata creata"), ReportDir.IsEmpty()))
	{
		FString Json;
		TestTrue(TEXT("result.json esiste ed e' leggibile"),
			FFileHelper::LoadFileToString(Json, *FPaths::Combine(ReportDir, TEXT("result.json"))));
		TestTrue(TEXT("il report parla dello scenario giusto"), Json.Contains(TEXT("Movement.Basic")));
	}

	// Anche un ID inesistente deve lasciare un report: e' il caso in cui si ha piu' bisogno di sapere perche'.
	UWorld* World2 = MakeRunnerWorld();
	if (!TestNotNull(TEXT("world 2"), World2)) { return false; }
	FString MissingDir;
	const FRTTestResult Missing = URTScenarioRunner::RunById(World2, TEXT("Non.Esiste"), MissingDir);
	DestroyRunnerWorld(World2);

	TestEqual(TEXT("scenario inesistente -> ERROR"), Missing.OutcomeString(), FString(TEXT("ERROR")));
	TestTrue(TEXT("l'errore dice DOVE ha cercato"), Missing.ErrorMessage.Contains(TEXT("Non")));
	return true;
}

/** L'elenco degli scenari deriva dai file: l'ID e' il percorso, quindi non esiste un indice da mantenere. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioListIdsTest,
	"RefactorTactics.Scenario.ListIdsFromFiles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioListIdsTest::RunTest(const FString&)
{
	const TArray<FString> Ids = URTScenarioRunner::ListScenarioIds();
	TestTrue(TEXT("almeno uno scenario trovato"), Ids.Num() > 0);
	TestTrue(TEXT("Movement.Basic e' nell'elenco"), Ids.Contains(TEXT("Movement.Basic")));
	TestTrue(TEXT("il FAIL intenzionale e' nell'elenco"), Ids.Contains(TEXT("Movement.BasicFailsOnPurpose")));
	return true;
}

/**
 * `Movement.Blocked`: un muro di ostacoli separa l'unita' dalla destinazione. Il percorso non esiste, quindi
 * il piano viene rifiutato e l'unita' resta dov'e'. Il turno si chiude comunque: un piano impossibile non
 * blocca la partita, e nessuno attraversa l'ostacolo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioMovementBlockedTest,
	"RefactorTactics.Scenario.RunnerMovementBlocked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioMovementBlockedTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!LoadShippedScenario(*this, TEXT("Movement.Blocked"), Scenario)) { return false; }
	TestTrue(TEXT("lo scenario dichiara degli ostacoli"), Scenario.Cells.Num() > 0);

	UWorld* World = MakeRunnerWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	DestroyRunnerWorld(World);

	if (Result.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("ERROR invece di PASS: %s"), *Result.ErrorMessage));
		return false;
	}
	TestEqual(TEXT("esito PASS: l'unita' resta ferma, come previsto"), Result.OutcomeString(), FString(TEXT("PASS")));
	TestEqual(TEXT("il turno si e' chiuso lo stesso"), Result.TurnsPlayed, 1);
	return true;
}

/**
 * `Movement.Collision`: due unita' verso la stessa cella si fermano ENTRAMBE. E' anche il caso in cui
 * l'ordine dell'array non deve poter decidere: se lo decidesse, una delle due entrerebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioMovementCollisionTest,
	"RefactorTactics.Scenario.RunnerMovementCollision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioMovementCollisionTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!LoadShippedScenario(*this, TEXT("Movement.Collision"), Scenario)) { return false; }

	UWorld* World = MakeRunnerWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	DestroyRunnerWorld(World);

	if (Result.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("ERROR invece di PASS: %s"), *Result.ErrorMessage));
		return false;
	}
	TestEqual(TEXT("esito PASS: destinazione contesa, entrambe ferme"), Result.OutcomeString(), FString(TEXT("PASS")));

	// Le due assertion di posizione devono essere ENTRAMBE verdi: se una sola unita' fosse entrata, il
	// contested avrebbe scelto un vincitore - cioe' l'ordine dell'array avrebbe deciso l'esito.
	int32 Positions = 0;
	for (const FRTAssertionResult& A : Result.Assertions)
	{
		if (A.Kind == ERTAssertionKind::UnitAtCell) { ++Positions; TestTrue(A.Description, A.bPassed); }
	}
	TestEqual(TEXT("verificate le posizioni di entrambe"), Positions, 2);
	return true;
}

/** Le celle modificate dallo scenario finiscono davvero nell'arena: senza, l'ostacolo non bloccherebbe nulla. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioCellOverridesApplyTest,
	"RefactorTactics.Scenario.CellOverridesReachTheArena",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioCellOverridesApplyTest::RunTest(const FString&)
{
	// Scenario costruito qui: la stessa destinazione, una volta libera e una volta murata. Se gli override
	// non arrivassero all'arena, i due casi darebbero lo STESSO risultato - ed e' esattamente cio' che il
	// confronto smaschera.
	auto MakeScenario = [](bool bWall)
	{
		FRTTestScenario S;
		S.ScenarioId = bWall ? TEXT("Probe.Walled") : TEXT("Probe.Open");
		S.MapRadius = 3;
		FRTScenarioUnit U;
		U.Id = TEXT("A1"); U.HeroId = TEXT("Hero.Flux"); U.TeamId = 0; U.Cell = FRTCellId(-2, 0, 0);
		S.Units.Add(U);
		if (bWall)
		{
			// Muro sulla colonna q=-1. In coordinate assiali il raggio limita anche r: per q=-1 l'intervallo
			// valido e' [-2, 3], non [-3, 3]. Sforarlo faceva rifiutare lo scenario dalla validazione con un
			// ERROR - correttamente, ed e' cosi' che questo test si e' accorto di essere scritto male.
			for (int32 R = -2; R <= 3; ++R)
			{
				FRTScenarioCell C; C.Cell = FRTCellId(-1, R, 0); C.bBlocksMovement = true; S.Cells.Add(C);
			}
		}
		FRTScenarioTurn T;
		FRTScenarioIntent I; I.UnitId = TEXT("A1"); I.Move.Add(FRTCellId(0, 0, 0));
		T.Intents.Add(I); S.Turns.Add(T);
		FRTTestExpectation E; E.Kind = ERTAssertionKind::UnitAtCell; E.UnitId = TEXT("A1"); E.Cell = FRTCellId(0, 0, 0);
		S.Expect.Add(E);
		return S;
	};

	UWorld* W1 = MakeRunnerWorld();
	if (!TestNotNull(TEXT("world 1"), W1)) { return false; }
	const FRTTestResult Open = URTScenarioRunner::Run(W1, MakeScenario(false));
	DestroyRunnerWorld(W1);

	UWorld* W2 = MakeRunnerWorld();
	if (!TestNotNull(TEXT("world 2"), W2)) { return false; }
	const FRTTestResult Walled = URTScenarioRunner::Run(W2, MakeScenario(true));
	DestroyRunnerWorld(W2);

	TestEqual(TEXT("senza muro l'unita' arriva"), Open.OutcomeString(), FString(TEXT("PASS")));
	TestEqual(TEXT("con il muro NON arriva"), Walled.OutcomeString(), FString(TEXT("FAIL")));
	return true;
}

/**
 * `Movement.SwapRejectedByPlanning`: due unita' adiacenti NON si scambiano di posto.
 *
 * E' un test di CARATTERIZZAZIONE: fissa il comportamento attuale, non una regola desiderata. La
 * pianificazione rifiuta un percorso verso una cella occupata (`FindPathForUnit`: goal occupato -> NoPath),
 * quindi lo scambio non arriva mai al resolver.
 *
 * ⚠️ Il resolver invece lo CONSENTE — `HexSim.ResolveSwapAllowed` lo verifica, ma passando percorsi costruiti
 * a mano e quindi bypassando il planner. Le due regole insieme rendono lo scambio **irraggiungibile dal
 * gioco**, ed e' il tipo di difetto che solo un test d'integrazione puo' mostrare: entrambe le regole,
 * guardate da sole, sono verdi e sensate.
 *
 * Se un giorno lo scambio dovra' essere possibile, sara' il planner a cambiare e questo test diventera'
 * rosso: e' il segnale che si vuole, non un fastidio da mettere a tacere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioSwapRejectedTest,
	"RefactorTactics.Scenario.RunnerSwapRejectedByPlanning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioSwapRejectedTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!LoadShippedScenario(*this, TEXT("Movement.SwapRejectedByPlanning"), Scenario)) { return false; }

	UWorld* World = MakeRunnerWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	DestroyRunnerWorld(World);

	if (Result.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("ERROR invece di PASS: %s"), *Result.ErrorMessage));
		return false;
	}
	TestEqual(TEXT("esito PASS: entrambe restano ferme (comportamento attuale)"),
		Result.OutcomeString(), FString(TEXT("PASS")));
	return true;
}

/**
 * `Movement.LongWalk`: due unita' attraversano l'arena, tre celle per turno, per due turni.
 *
 * Esiste per essere GUARDATO in PIE (`PIE-TEST-VISUAL`): su una cella sola non si distingue un'animazione da
 * un teletrasporto. Ma resta un test normale, non uno scenario di serie B — se il movimento multi-cella su
 * piu' turni si rompesse, questo lo direbbe prima che qualcuno se ne accorga a schermo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLongWalkTest,
	"RefactorTactics.Scenario.RunnerLongWalk",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLongWalkTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!LoadShippedScenario(*this, TEXT("Movement.LongWalk"), Scenario)) { return false; }
	TestEqual(TEXT("sono due turni, non uno"), Scenario.Turns.Num(), 2);

	UWorld* World = MakeRunnerWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	DestroyRunnerWorld(World);

	if (Result.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("ERROR invece di PASS: %s"), *Result.ErrorMessage));
		return false;
	}
	TestEqual(TEXT("esito PASS"), Result.OutcomeString(), FString(TEXT("PASS")));
	TestEqual(TEXT("due turni giocati"), Result.TurnsPlayed, 2);

	// Entrambe hanno percorso la loro strada: se una fosse rimasta indietro, l'assertion di posizione lo dice
	// con la cella dove si e' fermata — che e' il primo posto dove guardare se un giorno diventasse rosso.
	for (const FRTAssertionResult& A : Result.Assertions)
	{
		if (A.Kind == ERTAssertionKind::UnitAtCell)
		{
			TestTrue(FString::Printf(TEXT("%s -> %s (atteso %s)"), *A.Description, *A.Actual, *A.Expected), A.bPassed);
		}
	}
	return true;
}

/**
 * `Combat.BasicAttack`: il primo scenario che verifica un DANNO invece di una posizione.
 *
 * Flux colpisce Bastion con `Flux.ArcPulse` (22 danni, portata 4) a distanza 2: Bastion scende da 120 a 98.
 * I numeri vengono dal catalogo eroi v0.1 — se qualcuno li cambia senza aggiornare il catalogo, questo
 * diventa rosso, ed e' il punto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioCombatBasicAttackTest,
	"RefactorTactics.Scenario.RunnerCombatBasicAttack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioCombatBasicAttackTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!LoadShippedScenario(*this, TEXT("Combat.BasicAttack"), Scenario)) { return false; }

	UWorld* World = MakeRunnerWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	DestroyRunnerWorld(World);

	if (Result.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("ERROR invece di PASS: %s"), *Result.ErrorMessage));
		return false;
	}
	TestEqual(TEXT("esito PASS"), Result.OutcomeString(), FString(TEXT("PASS")));

	// Ogni assertion sugli HP deve essere verde: se il colpo non fosse partito, il bersaglio sarebbe a 120 e
	// l'`actual` lo direbbe — e' il primo posto dove guardare se un giorno diventasse rosso.
	for (const FRTAssertionResult& A : Result.Assertions)
	{
		if (A.Kind == ERTAssertionKind::UnitHpEquals)
		{
			TestTrue(FString::Printf(TEXT("%s -> %s (atteso %s)"), *A.Description, *A.Actual, *A.Expected), A.bPassed);
		}
	}
	return true;
}

/**
 * `Combat.BlockedByWall`: lo stesso attacco, con un muro in mezzo, NON arriva.
 *
 * E' il complemento necessario del test sopra. Da solo, «l'attacco fa 22 danni» non distingue un gioco che
 * rispetta la copertura da uno che spara attraverso i muri: servono entrambi, e devono usare la stessa
 * abilita' e la stessa coppia di eroi, altrimenti si confronterebbero due cose diverse.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioCombatBlockedTest,
	"RefactorTactics.Scenario.RunnerCombatBlockedByWall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioCombatBlockedTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!LoadShippedScenario(*this, TEXT("Combat.BlockedByWall"), Scenario)) { return false; }
	TestTrue(TEXT("lo scenario dichiara un muro"), Scenario.Cells.Num() > 0);

	UWorld* World = MakeRunnerWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	DestroyRunnerWorld(World);

	if (Result.Outcome == ERTTestOutcome::Error)
	{
		AddError(FString::Printf(TEXT("ERROR invece di PASS: %s"), *Result.ErrorMessage));
		return false;
	}
	TestEqual(TEXT("esito PASS: il muro ferma il colpo"), Result.OutcomeString(), FString(TEXT("PASS")));
	return true;
}

/**
 * Il DANNO e' davvero il discrimine fra i due scenari.
 *
 * I due test sopra passano entrambi, ma da soli non provano che sia il MURO a fare la differenza: se
 * l'attacco non partisse mai — abilita' sbagliata, bersaglio nullo, intent ignorato — sarebbero verdi lo
 * stesso, perche' «120 HP» e' anche il risultato di «non e' successo niente». Qui si confrontano i due
 * stati finali e si pretende che DIFFERISCANO.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioWallActuallyBlocksTest,
	"RefactorTactics.Scenario.WallIsWhatStopsTheShot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioWallActuallyBlocksTest::RunTest(const FString&)
{
	FRTTestScenario Open, Walled;
	if (!LoadShippedScenario(*this, TEXT("Combat.BasicAttack"), Open)) { return false; }
	if (!LoadShippedScenario(*this, TEXT("Combat.BlockedByWall"), Walled)) { return false; }

	UWorld* W1 = MakeRunnerWorld();
	if (!TestNotNull(TEXT("world 1"), W1)) { return false; }
	const FRTTestResult OpenResult = URTScenarioRunner::Run(W1, Open);
	DestroyRunnerWorld(W1);

	UWorld* W2 = MakeRunnerWorld();
	if (!TestNotNull(TEXT("world 2"), W2)) { return false; }
	const FRTTestResult WalledResult = URTScenarioRunner::Run(W2, Walled);
	DestroyRunnerWorld(W2);

	// Stati finali diversi -> hash diversi. Se fossero uguali, l'attacco non starebbe facendo niente in
	// NESSUNO dei due casi, e i due test verdi sopra non proverebbero nulla.
	TestNotEqual(TEXT("con e senza muro producono stati diversi"), OpenResult.StateHash, WalledResult.StateHash);
	return true;
}

/**
 * Esegue uno scenario e riporta OGNI assertion sugli HP con il suo `actual`.
 *
 * Il nome e' prolisso di proposito: la unity build fonde piu' file in una sola unita' di traduzione, e un
 * helper con un nome ovvio (`RunAndCheck`) collide col primo omonimo di un altro file di test — e' gia'
 * successo con tre helper insieme. Neppure un namespace anonimo protegge: fusi nella stessa TU, due namespace
 * anonimi sono lo STESSO namespace.
 */
static bool RunScenarioAndReportHpAssertions(FAutomationTestBase& Test, const TCHAR* ScenarioId)
{
	FRTTestScenario Scenario;
	if (!LoadShippedScenario(Test, ScenarioId, Scenario)) { return false; }

	UWorld* World = MakeRunnerWorld();
	if (!Test.TestNotNull(TEXT("world"), World)) { return false; }
	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	DestroyRunnerWorld(World);

	if (Result.Outcome == ERTTestOutcome::Error)
	{
		Test.AddError(FString::Printf(TEXT("%s: ERROR invece di PASS: %s"), ScenarioId, *Result.ErrorMessage));
		return false;
	}
	Test.TestEqual(TEXT("esito PASS"), Result.OutcomeString(), FString(TEXT("PASS")));

	// Ogni HP atteso col suo valore reale: quando uno di questi diventa rosso, il messaggio dice gia' di
	// quanto ha sbagliato, senza bisogno di rieseguire lo scenario a mano.
	for (const FRTAssertionResult& A : Result.Assertions)
	{
		if (A.Kind == ERTAssertionKind::UnitHpEquals)
		{
			Test.TestTrue(FString::Printf(TEXT("%s -> %s (atteso %s)"), *A.Description, *A.Actual, *A.Expected), A.bPassed);
		}
	}
	return true;
}

/**
 * `Combat.SplashHitsAlliesNotSelf`: l'area colpisce chiunque ci sia dentro, tranne chi la lancia.
 *
 * Col fuoco amico attivo «l'alleato incassa» non e' piu' una notizia: lo e' il fatto che l'ATTACCANTE no,
 * pur stando dentro la propria esplosione. Lo scenario lo mette apposta nel raggio, cosi' l'esclusione e'
 * verificata come regola (`u == AttackerId`) e non come geografia: piazzandolo lontano, quell'assert
 * sarebbe verde anche in un gioco che si fa male da solo appena qualcuno gli si avvicina.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioSplashHitsAlliesTest,
	"RefactorTactics.Scenario.RunnerSplashHitsAlliesNotSelf",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioSplashHitsAlliesTest::RunTest(const FString&)
{
	return RunScenarioAndReportHpAssertions(*this, TEXT("Combat.SplashHitsAlliesNotSelf"));
}

/**
 * `Combat.LineHitsThrough`: la forma Line colpisce la traiettoria, non il bersaglio.
 *
 * B2 sta fra attaccante e bersaglio e non e' nominata da nessun intent. Se scende, la linea e' stata risolta
 * come linea; se resta piena mentre B1 scende, il gioco sta trattando `ERTAbilityShape::Line` come un colpo
 * singolo — un difetto che nessuno scenario a due unita' puo' vedere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLineHitsThroughTest,
	"RefactorTactics.Scenario.RunnerLineHitsThrough",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLineHitsThroughTest::RunTest(const FString&)
{
	return RunScenarioAndReportHpAssertions(*this, TEXT("Combat.LineHitsThrough"));
}

/**
 * Le forme colpiscono DAVVERO piu' di una cella.
 *
 * Controprova delle due sopra, nello spirito di `WallIsWhatStopsTheShot`. I numeri attesi negli scenari li ho
 * scritti io: se avessi sbagliato la geometria e messo B2 fuori dall'area, avrei scritto «B2 resta piena» e il
 * test sarebbe verde lo stesso, documentando una forma che non funziona. Qui la domanda e' posta al gioco e
 * non allo scenario: quante unita' hanno perso HP? Se la risposta fosse 1, entrambe le forme starebbero
 * degenerando in un colpo singolo, e i due test sopra passerebbero ugualmente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioShapesHitMoreThanOneTest,
	"RefactorTactics.Scenario.ShapesHitMoreThanOneUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioShapesHitMoreThanOneTest::RunTest(const FString&)
{
	for (const TCHAR* Id : { TEXT("Combat.SplashHitsAlliesNotSelf"), TEXT("Combat.LineHitsThrough") })
	{
		FRTTestScenario Scenario;
		if (!LoadShippedScenario(*this, Id, Scenario)) { return false; }

		// Quante unita' lo scenario si aspetta di vedere ferite, secondo i suoi stessi numeri? Conta le
		// UnitHpEquals il cui valore atteso e' sotto la salute massima dell'eroe schierato.
		const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
		int32 ExpectedWounded = 0;
		for (const FRTTestExpectation& A : Scenario.Expect)
		{
			if (A.Kind != ERTAssertionKind::UnitHpEquals) { continue; }
			for (const FRTScenarioUnit& U : Scenario.Units)
			{
				if (U.Id != A.UnitId) { continue; }
				for (const URTHeroData* Hero : Roster)
				{
					if (Hero != nullptr && Hero->HeroId == U.HeroId && A.Value < Hero->MaxHealth)
					{
						++ExpectedWounded;
					}
				}
			}
		}
		TestTrue(FString::Printf(TEXT("%s: la forma deve ferire piu' di una unita' (attese %d)"), Id, ExpectedWounded),
			ExpectedWounded >= 2);
	}
	return true;
}

/**
 * `Combat.CounterStrikesBack`: una reazione ARMATA scatta e colpisce chi ha colpito.
 *
 * Il numero che conta e' quello di Bastion: 110 invece di 120, senza che nessun intent glielo abbia fatto
 * fare. E' l'unico effetto del turno che nessuno ha chiesto esplicitamente, quindi l'unico che puo' venire
 * solo dalla reazione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioCounterStrikesBackTest,
	"RefactorTactics.Scenario.RunnerCounterStrikesBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioCounterStrikesBackTest::RunTest(const FString&)
{
	return RunScenarioAndReportHpAssertions(*this, TEXT("Combat.CounterStrikesBack"));
}

/** `Combat.NoCounterWhenUnarmed`: senza armarla, la stessa reazione non scatta. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioNoCounterUnarmedTest,
	"RefactorTactics.Scenario.RunnerNoCounterWhenUnarmed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioNoCounterUnarmedTest::RunTest(const FString&)
{
	return RunScenarioAndReportHpAssertions(*this, TEXT("Combat.NoCounterWhenUnarmed"));
}

/**
 * E' l'ARMARLA a fare la differenza, non qualcos'altro.
 *
 * Stessa forma di `WallIsWhatStopsTheShot`. I due test sopra sono verdi anche in un gioco dove la reazione
 * non esiste e Bastion non viene mai toccato — no: il primo fallirebbe. Ma sarebbero verdi entrambi in un
 * gioco dove la reazione scatta SEMPRE, se per caso i numeri attesi fossero stati scritti su quel
 * comportamento. Qui si confrontano i due stati finali e si pretende che differiscano: l'unica riga diversa
 * fra i due scenari e' il campo `reaction`, quindi la differenza non puo' venire da altro.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioArmingIsWhatFiresTest,
	"RefactorTactics.Scenario.ArmingIsWhatFiresTheReaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioArmingIsWhatFiresTest::RunTest(const FString&)
{
	FRTTestScenario Armed, Unarmed;
	if (!LoadShippedScenario(*this, TEXT("Combat.CounterStrikesBack"), Armed)) { return false; }
	if (!LoadShippedScenario(*this, TEXT("Combat.NoCounterWhenUnarmed"), Unarmed)) { return false; }

	UWorld* W1 = MakeRunnerWorld();
	if (!TestNotNull(TEXT("world 1"), W1)) { return false; }
	const FRTTestResult ArmedResult = URTScenarioRunner::Run(W1, Armed);
	DestroyRunnerWorld(W1);

	UWorld* W2 = MakeRunnerWorld();
	if (!TestNotNull(TEXT("world 2"), W2)) { return false; }
	const FRTTestResult UnarmedResult = URTScenarioRunner::Run(W2, Unarmed);
	DestroyRunnerWorld(W2);

	TestNotEqual(TEXT("armata e disarmata producono stati diversi"), ArmedResult.StateHash, UnarmedResult.StateHash);
	return true;
}

/**
 * Armare qualcosa che NON e' una reazione viene rifiutato, con un motivo.
 *
 * E' il modo di fallire peggiore che ci sia in questo campo: `Flux.ArcPulse` in `reaction` non scatterebbe
 * mai e non produrrebbe nessun errore: lo scenario girerebbe, l'assertion sui danni fallirebbe, e chi legge
 * cercherebbe una regressione del combattimento invece di una riga sbagliata nel JSON.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioBadReactionRejectedTest,
	"RefactorTactics.Scenario.NonReactionCannotBeArmed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioBadReactionRejectedTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!LoadShippedScenario(*this, TEXT("Combat.CounterStrikesBack"), Scenario)) { return false; }

	// Un'abilita' che Flux POSSIEDE davvero, ma che non e' una reazione: il controllo che conta e' sullo
	// SLOT. Usando un ID inesistente si verificherebbe solo che il nome non si trova, che e' un'altra cosa.
	Scenario.Turns[0].Intents[0].Reaction = FName(TEXT("Flux.ArcPulse"));

	FString Error;
	TestFalse(TEXT("lo scenario viene rifiutato"), URTScenarioLoader::Validate(Scenario, Error));
	TestTrue(TEXT("il motivo nomina lo slot"), Error.Contains(TEXT("non e' una reazione")));

	// E la reazione VERA continua a passare: il controllo rifiuta ciò che deve, non tutto.
	FRTTestScenario Good;
	if (!LoadShippedScenario(*this, TEXT("Combat.CounterStrikesBack"), Good)) { return false; }
	FString NoError;
	TestTrue(TEXT("la reazione vera resta valida"), URTScenarioLoader::Validate(Good, NoError));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
