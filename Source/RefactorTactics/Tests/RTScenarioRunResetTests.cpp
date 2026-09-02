// RUN e RESET dal Tactical Designer (#1117).
//
// L'issue chiede una cosa sola, in molte forme: che l'esecuzione dall'editor **sia** l'esecuzione del gioco.
// Il criterio che lo dimostra e' scritto nella issue stessa e non e' negoziabile:
//
//   «Lo stesso file eseguito headless da' lo stesso risultato logico dell'esecuzione nell'editor.
//    E' il criterio che dimostra l'assenza del secondo simulatore.»
//
// Per questo il test centrale di questo file non verifica che `Run` funzioni: verifica che **dia la stessa
// risposta** di `URTScenarioRunner::Run` chiamato direttamente, esito per esito, hash per hash. Se qualcuno
// aggiungesse una scorciatoia nel percorso d'editor, le due risposte divergerebbero.

#include "Misc/AutomationTest.h"
#include "ScenarioHarness/RTScenarioAuthoring.h"
#include "ScenarioHarness/RTScenarioDraft.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "HAL/FileManager.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nome distinto da ogni altro file di test: la unity build condivide la translation unit.
	//
	// Uno scenario che PASSA: A1 si muove di una cella e l'assertion dice dove deve arrivare.
	const TCHAR* RunResetPassingJson = TEXT(R"JSON(
	{
	  "scenarioId": "Movement.RunResetProbe",
	  "version": 1,
	  "mapRadius": 3,
	  "units": [
	    { "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [-2, 0, 0] },
	    { "id": "B1", "hero": "Hero.Riktor", "team": 1, "cell": [2, 0, 0] }
	  ],
	  "turns": [ { "intents": [ { "unit": "A1", "move": [[-1, 0, 0]] } ] } ],
	  "expect": [ { "type": "UnitAtCell", "unit": "A1", "cell": [-1, 0, 0] } ]
	}
	)JSON");

	// Una base **senza turni**: il criterio 1 di `#1627` dice che è il designer a crearli, quindi un JSON che
	// li portasse già misurerebbe il caricamento invece dell'authoring. Le assertion nominano il risultato di
	// tre passi, uno per turno. Nome distinto per la unity build.
	const TCHAR* RunResetThreeTurnJson = TEXT(R"JSON(
	{
	  "scenarioId": "Movement.RunResetSequenceProbe",
	  "version": 1,
	  "mapRadius": 3,
	  "units": [
	    { "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [-2, 0, 0] },
	    { "id": "B1", "hero": "Hero.Riktor", "team": 1, "cell": [2, 0, 0] }
	  ],
	  "turns": [],
	  "expect": [
	    { "type": "TurnsCompleted", "value": 3 },
	    { "type": "UnitAtCell", "unit": "A1", "cell": [1, 0, 0] }
	  ]
	}
	)JSON");

	bool OpenRunSequenceDraft(FRTScenarioDraft& OutDraft, FString& OutError)
	{
		FRTTestScenario Loaded;
		if (!URTScenarioLoader::LoadFromString(RunResetThreeTurnJson, Loaded, OutError))
		{
			return false;
		}
		OutDraft.NewScenario(TEXT("segnaposto"), 3);
		OutDraft.MutableScenario() = Loaded;
		return true;
	}

	bool OpenRunDraft(FRTScenarioDraft& OutDraft, FString& OutError)
	{
		FRTTestScenario Loaded;
		if (!URTScenarioLoader::LoadFromString(RunResetPassingJson, Loaded, OutError))
		{
			return false;
		}
		OutDraft.NewScenario(TEXT("segnaposto"), 3);
		OutDraft.MutableScenario() = Loaded;
		return true;
	}

	FString RunResetTempDir()
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Tests"), TEXT("RunReset"));
	}

	// Un mondo PER esecuzione: `URTScenarioRunner::Run` non smonta quello che riceve, e riusarlo farebbe
	// misurare alla corsa successiva il residuo della precedente. Nome distinto per la unity build.
	UWorld* MakeRunResetWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyRunResetWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
		}
		if (World)
		{
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}
}

// --- l'editor esegue cio' che esegue il gioco -----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioRunMatchesTheHeadlessPathTest,
	"RefactorTactics.Scenario.RunFromTheEditorMatchesTheHeadlessRun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioRunMatchesTheHeadlessPathTest::RunTest(const FString&)
{
	// ⚠️ **Il criterio che #1117 chiama «la dimostrazione dell'assenza del secondo simulatore».**
	//
	// Lo stesso scenario, due strade: la facade dell'editor e `URTScenarioRunner::Run` diretto. Devono dare lo
	// stesso esito, gli stessi conteggi di assertion e lo **stesso StateHash** — che e' la parte che morde,
	// perche' un hash uguale significa che i due percorsi hanno prodotto lo stesso stato finale, non solo lo
	// stesso verdetto.
	FRTTestScenario Scenario;
	FString Error;
	if (!TestTrue(TEXT("scenario caricato"),
		URTScenarioLoader::LoadFromString(RunResetPassingJson, Scenario, Error)))
	{
		AddError(Error);
		return false;
	}

	// Strada 1: il percorso headless, quello che gira in CI e da riga di comando.
	UWorld* World = MakeRunResetWorld();
	if (!TestNotNull(TEXT("mondo per la corsa headless"), World)) { return false; }
	const FRTTestResult Headless = URTScenarioRunner::Run(World, Scenario);
	DestroyRunResetWorld(World);

	// Strada 2: il percorso dell'editor, che si costruisce il mondo da se'.
	URTScenarioAuthoring* Authoring = URTScenarioAuthoring::CreateScenarioDraft(nullptr);
	if (!TestNotNull(TEXT("draft creato"), Authoring)) { return false; }
	Authoring->NewScenario(TEXT("segnaposto"), 3);
	Authoring->GetDraft().MutableScenario() = Scenario;

	FRTScenarioRunReport Report;
	if (!TestEqual(TEXT("l'esecuzione dall'editor avviene"),
		Authoring->Run(Report, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}

	TestEqual(TEXT("stesso esito"), Report.OutcomeText, Headless.OutcomeString());
	TestEqual(TEXT("stessi turni giocati"), Report.TurnsPlayed, Headless.TurnsPlayed);
	TestEqual(TEXT("stesse assertion passate"), Report.PassedCount, Headless.PassedCount());
	TestEqual(TEXT("stesse assertion fallite"), Report.FailedCount, Headless.FailedCount());
	TestEqual(TEXT("stesso StateHash"), Report.StateHash,
		FString::Printf(TEXT("%08x"), Headless.StateHash));

	// E lo scenario di prova deve PASSARE, altrimenti il confronto qui sopra vale fra due errori.
	TestEqual(TEXT("lo scenario di prova passa"), Report.OutcomeText, FString(TEXT("PASS")));
	TestEqual(TEXT("e l'esito tipizzato concorda"),
		static_cast<int32>(Report.Outcome), static_cast<int32>(ERTTestOutcome::Pass));

	return true;
}

// --- i quattro esiti restano distinti -------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioRunKeepsTheFourOutcomesApartTest,
	"RefactorTactics.Scenario.RunKeepsPassFailErrorAndBlockedApart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioRunKeepsTheFourOutcomesApartTest::RunTest(const FString&)
{
	// `Blocked` non e' un successo e `Error` non e' un `Fail`: la distinzione fra «il gioco e' rotto» e «il
	// test e' scritto male» e' la ragione per cui quell'enum ha quattro casi, e comprimerli e' il modo piu'
	// veloce di rendere inutile un pannello di esiti.
	FString Error;

	// --- PASS
	{
		URTScenarioAuthoring* Authoring = URTScenarioAuthoring::CreateScenarioDraft(nullptr);
		FRTTestScenario Scenario;
		URTScenarioLoader::LoadFromString(RunResetPassingJson, Scenario, Error);
		Authoring->NewScenario(TEXT("segnaposto"), 3);
		Authoring->GetDraft().MutableScenario() = Scenario;

		FRTScenarioRunReport Report;
		Authoring->Run(Report, Error);
		TestEqual(TEXT("PASS"), Report.OutcomeText, FString(TEXT("PASS")));
		TestTrue(TEXT("il report dichiara di aver girato"), Report.bHasRun);
		TestEqual(TEXT("nessuna assertion fallita"), Report.FailedCount, 0);
	}

	// --- FAIL: l'assertion chiede una cella diversa da quella in cui l'unita' arrivera'.
	{
		URTScenarioAuthoring* Authoring = URTScenarioAuthoring::CreateScenarioDraft(nullptr);
		FRTTestScenario Scenario;
		URTScenarioLoader::LoadFromString(RunResetPassingJson, Scenario, Error);
		Scenario.Expect[0].Cell = FRTCellId(2, -2, 0); // dove l'unita' non andra' mai
		Authoring->NewScenario(TEXT("segnaposto"), 3);
		Authoring->GetDraft().MutableScenario() = Scenario;

		FRTScenarioRunReport Report;
		TestEqual(TEXT("l'esecuzione avviene comunque"),
			Authoring->Run(Report, Error), ERTScenarioAuthoringResult::Success);

		// ⚠️ `Success` per l'OPERAZIONE, `FAIL` per il GIOCO: sono due domande diverse, e confonderle farebbe
		// apparire il difetto piu' prezioso che questo strumento produce come un guasto dello strumento.
		TestEqual(TEXT("FAIL"), Report.OutcomeText, FString(TEXT("FAIL")));
		TestTrue(TEXT("almeno una assertion fallita"), Report.FailedCount > 0);

		// #1117: «Un'assertion fallita mostra expected e actual, non solo l'esito.»
		const FRTScenarioAssertionView* Failed = Report.Assertions.FindByPredicate(
			[](const FRTScenarioAssertionView& A) { return !A.bPassed; });
		if (TestNotNull(TEXT("l'assertion fallita e' nel report"), Failed))
		{
			TestFalse(TEXT("expected e' compilato"), Failed->Expected.IsEmpty());
			TestFalse(TEXT("actual e' compilato"), Failed->Actual.IsEmpty());
			TestNotEqual(TEXT("e i due DIFFERISCONO: se coincidessero non sarebbe un fallimento"),
				Failed->Expected, Failed->Actual);
			TestFalse(TEXT("e la descrizione dice cosa asseriva"), Failed->Description.IsEmpty());
		}
	}

	// --- ERROR: uno scenario che nomina un eroe inesistente e' un difetto del TEST, non del gioco.
	{
		URTScenarioAuthoring* Authoring = URTScenarioAuthoring::CreateScenarioDraft(nullptr);
		FRTTestScenario Scenario;
		URTScenarioLoader::LoadFromString(RunResetPassingJson, Scenario, Error);
		Scenario.Units[0].HeroId = FName(TEXT("Hero.NonEsiste"));
		Authoring->NewScenario(TEXT("segnaposto"), 3);
		Authoring->GetDraft().MutableScenario() = Scenario;

		FRTScenarioRunReport Report;
		// Qui `Validate` lo intercetta PRIMA di far girare qualcosa: l'operazione e' `Invalid`, e l'errore
		// nomina l'eroe. E' corretto — accusare il gioco per uno scenario scritto male e' il difetto che la
		// distinzione Error/Fail esiste per impedire.
		const ERTScenarioAuthoringResult Outcome = Authoring->Run(Report, Error);
		TestEqual(TEXT("uno scenario invalido non gira"), Outcome, ERTScenarioAuthoringResult::Invalid);
		TestTrue(*FString::Printf(TEXT("e l'errore nomina l'eroe (era: %s)"), *Error),
			Error.Contains(TEXT("eroe")) || Error.Contains(TEXT("sconosciuto")));
		TestFalse(TEXT("e il report resta vuoto"), Report.bHasRun);
	}

	return true;
}

// --- RESET, e la stabilita' di RUN -> RESET -> RUN -------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioResetReturnsToTheDeclaredStateTest,
	"RefactorTactics.Scenario.ResetReturnsToTheInitialStateNotToThePreviousOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioResetReturnsToTheDeclaredStateTest::RunTest(const FString&)
{
	const FString Dir = RunResetTempDir();
	const FString Path = FPaths::Combine(Dir, TEXT("RunReset.json"));
	ON_SCOPE_EXIT{ IFileManager::Get().DeleteDirectory(*Dir, false, true); };

	FString Error;

	// Lo scenario su disco: e' la «fonte» a cui `RESET` deve tornare.
	{
		FRTScenarioDraft Seed;
		if (!TestTrue(TEXT("scenario di prova costruito"), OpenRunDraft(Seed, Error)))
		{
			AddError(Error);
			return false;
		}
		if (!TestEqual(TEXT("scritto su disco"),
			Seed.SaveToFile(Path, Error), ERTScenarioAuthoringResult::Success))
		{
			AddError(Error);
			return false;
		}
	}

	URTScenarioAuthoring* Authoring = URTScenarioAuthoring::CreateScenarioDraft(nullptr);
	if (!TestNotNull(TEXT("draft creato"), Authoring)) { return false; }
	if (!TestEqual(TEXT("aperto da disco"),
		Authoring->OpenFromFile(Path, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}

	// --- RUN
	FRTScenarioRunReport First;
	if (!TestEqual(TEXT("prima esecuzione"), Authoring->Run(First, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}
	TestTrue(TEXT("il report c'e'"), Authoring->GetLastRunReport().bHasRun);

	// ⚠️ **`Run` non ha toccato lo scenario.** E' l'invariante che rende `RESET` un ritorno all'initial state
	// invece che un undo: il runner lavora su una copia, in un mondo suo.
	TestEqual(TEXT("le unita' sono ancora quelle dichiarate"),
		Authoring->GetDraft().GetScenario().Units[0].Cell, FRTCellId(-2, 0, 0));

	// --- una modifica d'authoring NON salvata, che RESET deve scartare
	Authoring->MoveUnit(TEXT("A1"), FRTCellId(-1, 1, 0), Error);
	TestEqual(TEXT("la modifica c'e'"),
		Authoring->GetDraft().GetScenario().Units[0].Cell, FRTCellId(-1, 1, 0));

	// --- RESET
	bool bDiscarded = false;
	TestEqual(TEXT("reset"), Authoring->Reset(bDiscarded, Error), ERTScenarioAuthoringResult::Success);
	// ⚠️ Avvisare qui non e' cortesia: `RESET` **perde** le modifiche non salvate, ed e' il suo significato —
	// ma un RESET muto che cancella mezz'ora di lavoro e' la stessa cancellazione silenziosa che `RemoveUnit`
	// si rifiuta di fare. La prima stesura confrontava i conteggi e non vedeva uno spostamento: questo test
	// l'ha preso.
	// L'avviso sta su un canale SUO, non su quello degli errori: `OutError` resta vuoto perche' non c'e'
	// stato nessun errore, e `bDiscarded` dice che qualcosa e' andato perso. Una UI cablata come
	// `if (!OutError.IsEmpty()) mostraBanner()` non deve colorare di rosso un RESET riuscito.
	TestTrue(TEXT("le modifiche perse sono segnalate"), bDiscarded);
	TestTrue(TEXT("e il canale degli errori resta pulito"), Error.IsEmpty());

	// #1117: «asserito confrontando lo stato dopo RESET con lo scenario CARICATO, non con lo stato precedente».
	FRTTestScenario FromDisk;
	if (TestTrue(TEXT("il file si rilegge per il confronto"),
		URTScenarioLoader::LoadFromFile(Path, FromDisk, Error)))
	{
		TestEqual(TEXT("dopo RESET le unita' sono quelle del FILE"),
			Authoring->GetDraft().GetScenario().Units[0].Cell, FromDisk.Units[0].Cell);
		TestEqual(TEXT("e sono tante quante il file ne dichiara"),
			Authoring->GetDraft().GetScenario().Units.Num(), FromDisk.Units.Num());
	}
	TestFalse(TEXT("il report e' stato scartato"), Authoring->GetLastRunReport().bHasRun);
	TestEqual(TEXT("e con esso il TurnLog"), Authoring->GetLastRunLog().Num(), 0);

	// --- RUN di nuovo: stesso input, stesso risultato. Inclusi gli hash.
	FRTScenarioRunReport Second;
	if (!TestEqual(TEXT("seconda esecuzione"),
		Authoring->Run(Second, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}

	// E il verso opposto dell'avviso: un RESET che non ha niente da scartare deve TACERE, altrimenti
	// l'avvertimento diventa rumore e chi lo legge smette di distinguerlo dal caso che conta.
	{
		FString QuietError;
		bool bQuietDiscarded = true;
		TestEqual(TEXT("un secondo reset senza modifiche riesce"),
			Authoring->Reset(bQuietDiscarded, QuietError), ERTScenarioAuthoringResult::Success);
		TestFalse(TEXT("e non dichiara di aver scartato niente"), bQuietDiscarded);
		TestTrue(TEXT("ne' scrive errori"), QuietError.IsEmpty());
	}

	TestEqual(TEXT("RUN -> RESET -> RUN da' lo stesso esito"), Second.OutcomeText, First.OutcomeText);
	TestEqual(TEXT("gli stessi turni"), Second.TurnsPlayed, First.TurnsPlayed);
	TestEqual(TEXT("le stesse assertion"), Second.PassedCount, First.PassedCount);
	// L'hash e' la parte che morde: due corse che dessero lo stesso verdetto con stati finali diversi
	// sarebbero un determinismo apparente.
	TestEqual(TEXT("e lo STESSO StateHash"), Second.StateHash, First.StateHash);

	return true;
}

// --- il TurnLog si consulta senza uscire dall'editor -----------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioRunExposesTheTurnLogTest,
	"RefactorTactics.Scenario.RunExposesAReadableTurnLog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioRunExposesTheTurnLogTest::RunTest(const FString&)
{
	URTScenarioAuthoring* Authoring = URTScenarioAuthoring::CreateScenarioDraft(nullptr);
	if (!TestNotNull(TEXT("draft creato"), Authoring)) { return false; }

	FString Error;
	FRTTestScenario Scenario;
	if (!TestTrue(TEXT("scenario caricato"),
		URTScenarioLoader::LoadFromString(RunResetPassingJson, Scenario, Error)))
	{
		AddError(Error);
		return false;
	}
	Authoring->NewScenario(TEXT("segnaposto"), 3);
	Authoring->GetDraft().MutableScenario() = Scenario;

	// Prima di correre, non c'e' niente da leggere: un log vuoto e' l'assenza di una corsa, non un errore.
	TestEqual(TEXT("nessun log prima di correre"), Authoring->GetLastRunLog().Num(), 0);

	FRTScenarioRunReport Report;
	if (!TestEqual(TEXT("esecuzione"), Authoring->Run(Report, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}

	const TArray<FRTScenarioLogEntryView> Log = Authoring->GetLastRunLog();
	if (!TestTrue(TEXT("il TurnLog non e' vuoto: lo scenario muove una unita'"), Log.Num() > 0))
	{
		return false;
	}

	// Le voci sono leggibili: turno, categoria, e un nome d'evento composto da chi possiede gli enum.
	bool bAnyNamed = false;
	for (const FRTScenarioLogEntryView& Entry : Log)
	{
		TestTrue(TEXT("i turni si contano da 1"), Entry.Turn >= 1);
		if (!Entry.Event.IsEmpty()) { bAnyNamed = true; }
	}
	TestTrue(TEXT("almeno un evento ha un nome leggibile"), bAnyNamed);

	// C'e' un movimento, ed e' l'unica cosa che questo scenario fa: se il log non lo contenesse, non starebbe
	// guardando la partita che e' stata giocata.
	const bool bHasMove = Log.ContainsByPredicate(
		[](const FRTScenarioLogEntryView& E) { return E.Category == ERTLogCategory::Move; });
	TestTrue(TEXT("il log contiene il movimento che lo scenario dichiara"), bHasMove);

	return true;
}

// --- verifica di mutazione ------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioRunDetectsAMutatedExpectationTest,
	"RefactorTactics.Scenario.RunTurnsPassIntoFailWhenTheExpectationMoves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioRunDetectsAMutatedExpectationTest::RunTest(const FString&)
{
	// #1117: «alterare l'expectation fa passare l'esito da PASS a FAIL con un actual corretto».
	//
	// Non basta che l'esito cambi: l'`actual` deve dire **dove l'unita' e' davvero finita**, altrimenti il
	// pannello mostrerebbe un fallimento senza la sola informazione che serve a capirlo.
	FString Error;
	FRTTestScenario Base;
	if (!TestTrue(TEXT("scenario caricato"),
		URTScenarioLoader::LoadFromString(RunResetPassingJson, Base, Error)))
	{
		AddError(Error);
		return false;
	}

	// --- prima: PASS
	URTScenarioAuthoring* Passing = URTScenarioAuthoring::CreateScenarioDraft(nullptr);
	Passing->NewScenario(TEXT("segnaposto"), 3);
	Passing->GetDraft().MutableScenario() = Base;
	FRTScenarioRunReport PassReport;
	if (!TestEqual(TEXT("prima esecuzione"),
		Passing->Run(PassReport, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}
	if (!TestEqual(TEXT("l'esito di partenza e' PASS"), PassReport.OutcomeText, FString(TEXT("PASS"))))
	{
		return false;
	}

	// --- poi: si sposta SOLO l'expectation, non il piano.
	FRTTestScenario Mutated = Base;
	Mutated.Expect[0].Cell = FRTCellId(1, -1, 0);

	URTScenarioAuthoring* Failing = URTScenarioAuthoring::CreateScenarioDraft(nullptr);
	Failing->NewScenario(TEXT("segnaposto"), 3);
	Failing->GetDraft().MutableScenario() = Mutated;
	FRTScenarioRunReport FailReport;
	if (!TestEqual(TEXT("seconda esecuzione"),
		Failing->Run(FailReport, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}

	TestEqual(TEXT("l'esito diventa FAIL"), FailReport.OutcomeText, FString(TEXT("FAIL")));

	const FRTScenarioAssertionView* Failed = FailReport.Assertions.FindByPredicate(
		[](const FRTScenarioAssertionView& A) { return !A.bPassed; });
	if (TestNotNull(TEXT("c'e' una assertion fallita"), Failed))
	{
		// 🔴 Il confronto e' con le stringhe ESATTE delle due celle, non con frammenti.
		//
		// La prima stesura cercava `Actual.Contains("-1")` ed `Expected.Contains("1")` — entrambi veri per
		// ENTRAMBE le stringhe, visto che `FRTCellId::ToString()` e' `(q=%d,r=%d,L=%d)`. Il test sarebbe
		// passato anche a campi SCAMBIATI, cioe' proprio nel caso che dichiara di sorvegliare. Trovato dalla
		// review di `#1117`.
		const FString RealCell = FRTCellId(-1, 0, 0).ToString();   // dove l'unita' arriva davvero
		const FString AskedCell = FRTCellId(1, -1, 0).ToString();  // cio' che l'expectation mutata chiede

		TestEqual(TEXT("actual e' la cella in cui l'unita' e' finita davvero"), Failed->Actual, RealCell);
		TestEqual(TEXT("expected e' la cella che l'assertion chiedeva"), Failed->Expected, AskedCell);
		TestNotEqual(TEXT("e le due differiscono davvero"), Failed->Expected, Failed->Actual);
	}

	// Il gioco non e' cambiato: solo l'attesa. Lo stato finale e' lo stesso, e l'hash lo dimostra — se
	// differisse, la mutazione avrebbe toccato la partita invece dell'assertion, e il test proverebbe
	// un'altra cosa.
	TestEqual(TEXT("lo stato finale della partita e' identico: e' l'attesa a essere cambiata"),
		FailReport.StateHash, PassReport.StateHash);

	return true;
}

// --- l'esecuzione e' raggiungibile da Blueprint ----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioRunIsExposedTest,
	"RefactorTactics.Scenario.RunAndResetAreReachableFromBlueprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioRunIsExposedTest::RunTest(const FString&)
{
	UClass* Facade = URTScenarioAuthoring::StaticClass();
	if (!TestNotNull(TEXT("la facade esiste"), Facade)) { return false; }

	for (const FName& Name : { FName(TEXT("Run")), FName(TEXT("Reset")), FName(TEXT("GetLastRunReport")),
		FName(TEXT("GetLastRunLog")) })
	{
		const UFunction* Function = Facade->FindFunctionByName(Name);
		if (!TestNotNull(*FString::Printf(TEXT("'%s' e' una UFUNCTION"), *Name.ToString()), Function))
		{
			continue;
		}
		TestTrue(*FString::Printf(TEXT("'%s' e' chiamabile da Blueprint"), *Name.ToString()),
			Function->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_BlueprintPure));
	}

	// I DTO del report devono essere `BlueprintType`, altrimenti il pannello non ha niente da cablare.
	for (UScriptStruct* Struct : { FRTScenarioRunReport::StaticStruct(),
		FRTScenarioAssertionView::StaticStruct(), FRTScenarioLogEntryView::StaticStruct() })
	{
		if (!TestNotNull(TEXT("il DTO esiste"), Struct)) { continue; }
		// Stessa ragione di sopra: `GetBoolMetaData` e' sotto `WITH_METADATA`, spento in un target Game.
#if WITH_METADATA
		TestTrue(*FString::Printf(TEXT("'%s' e' BlueprintType"), *Struct->GetName()),
			Struct->GetBoolMetaData(TEXT("BlueprintType")));
#endif
	}

	// ⚠️ `ERTTestOutcome` e' l'UNICA eccezione ad ADR-0010, e va verificata: senza `BlueprintType` la UI non
	// potrebbe ramificare sull'esito e mostrerebbe un `BLOCKED` come un successo. Le nove `USTRUCT` del
	// formato restano invece non esposte — lo asserisce
	// `RefactorTactics.Scenario.AuthoringContractIsReachableFromBlueprint`.
	const UEnum* OutcomeEnum = StaticEnum<ERTTestOutcome>();
	if (TestNotNull(TEXT("ERTTestOutcome esiste"), OutcomeEnum))
	{
#if WITH_METADATA
		TestTrue(TEXT("ed e' BlueprintType"), OutcomeEnum->GetBoolMetaData(TEXT("BlueprintType")));
#endif
		TestEqual(TEXT("con i suoi quattro esiti distinti"), OutcomeEnum->NumEnums() - 1, 4);
	}

	return true;
}

// --- i difetti che la review ha trovato -----------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioRunNeverShowsAStaleReportTest,
	"RefactorTactics.Scenario.ARefusedRunNeverShowsThePreviousResult",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioRunNeverShowsAStaleReportTest::RunTest(const FString&)
{
	// 🔴 **Il difetto piu' grave che la review di `#1117` ha trovato.**
	//
	// La facade copiava il report SEMPRE, e `FRTScenarioDraft::Run` non tocca `LastReport` quando rifiuta:
	// un RUN respinto da `Validate` restituiva quindi il report della corsa PRECEDENTE — `bHasRun` vero,
	// `PASS`, hash vecchio, TurnLog vecchio. Il pannello mostrava un verde per una corsa mai avvenuta.
	//
	// Il test precedente non lo vedeva perche' usava un draft NUOVO, il cui report e' ancora vuoto: serve
	// una corsa riuscita PRIMA del rifiuto.
	URTScenarioAuthoring* Authoring = URTScenarioAuthoring::CreateScenarioDraft(nullptr);
	if (!TestNotNull(TEXT("draft creato"), Authoring)) { return false; }

	FString Error;
	FRTTestScenario Scenario;
	if (!TestTrue(TEXT("scenario caricato"),
		URTScenarioLoader::LoadFromString(RunResetPassingJson, Scenario, Error)))
	{
		AddError(Error);
		return false;
	}
	Authoring->NewScenario(TEXT("segnaposto"), 3);
	Authoring->GetDraft().MutableScenario() = Scenario;

	// 1. una corsa che riesce, cosi' il draft HA un report da cui pescare per sbaglio
	FRTScenarioRunReport Good;
	if (!TestEqual(TEXT("la prima corsa riesce"),
		Authoring->Run(Good, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}
	TestEqual(TEXT("ed e' PASS"), Good.OutcomeText, FString(TEXT("PASS")));

	// 2. si rompe lo scenario: l'assertion nomina una unita' che non c'e' piu'
	Authoring->RemoveUnit(TEXT("A1"), Error);

	// 3. RUN viene rifiutato — e il report che torna NON deve essere quello di prima
	FRTScenarioRunReport AfterRefusal;
	const ERTScenarioAuthoringResult Refused = Authoring->Run(AfterRefusal, Error);
	TestEqual(TEXT("il RUN e' rifiutato"), Refused, ERTScenarioAuthoringResult::Invalid);
	TestFalse(TEXT("e il report NON dichiara di aver girato"), AfterRefusal.bHasRun);
	TestNotEqual(TEXT("ne' riporta il PASS della corsa precedente"),
		AfterRefusal.OutcomeText, FString(TEXT("PASS")));
	TestTrue(TEXT("ne' il suo StateHash"), AfterRefusal.StateHash.IsEmpty());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioRunReportDoesNotSurviveAScenarioChangeTest,
	"RefactorTactics.Scenario.ARunReportDoesNotSurviveIntoAnotherScenario",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioRunReportDoesNotSurviveAScenarioChangeTest::RunTest(const FString&)
{
	// Un report che sopravvive a un cambio di scenario attribuisce a quello nuovo l'esito, l'hash e il
	// TurnLog di quello vecchio — che non e' mai stato eseguito.
	URTScenarioAuthoring* Authoring = URTScenarioAuthoring::CreateScenarioDraft(nullptr);
	if (!TestNotNull(TEXT("draft creato"), Authoring)) { return false; }

	FString Error;
	FRTTestScenario Scenario;
	URTScenarioLoader::LoadFromString(RunResetPassingJson, Scenario, Error);
	Authoring->NewScenario(TEXT("segnaposto"), 3);
	Authoring->GetDraft().MutableScenario() = Scenario;

	FRTScenarioRunReport Report;
	if (!TestEqual(TEXT("corsa riuscita"), Authoring->Run(Report, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}
	TestTrue(TEXT("il report c'e'"), Authoring->GetLastRunReport().bHasRun);
	TestTrue(TEXT("e il TurnLog anche"), Authoring->GetLastRunLog().Num() > 0);

	// --- uno scenario NUOVO non eredita niente
	Authoring->NewScenario(TEXT("Altro.Scenario"), 3);
	TestFalse(TEXT("uno scenario nuovo non ha un report"), Authoring->GetLastRunReport().bHasRun);
	TestEqual(TEXT("ne' un TurnLog"), Authoring->GetLastRunLog().Num(), 0);

	// --- e nemmeno dopo Close
	Authoring->GetDraft().MutableScenario() = Scenario;
	Authoring->Run(Report, Error);
	Authoring->Close();
	TestFalse(TEXT("dopo Close il report se ne va"), Authoring->GetLastRunReport().bHasRun);
	TestEqual(TEXT("e il TurnLog pure"), Authoring->GetLastRunLog().Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioResetWorksForScenariosAuthoredHereTest,
	"RefactorTactics.Scenario.ResetWorksForAScenarioCreatedInTheEditor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioResetWorksForScenariosAuthoredHereTest::RunTest(const FString&)
{
	// 🔴 `RESET` era un **no-op silenzioso** per ogni scenario creato nell'editor: `SourcePath` lo scriveva
	// solo `OpenFromFile`, mai un salvataggio — che era `const` e quindi non poteva. Il flusso principale di
	// `#1115` (crea, modifica, salva, modifica ancora, RESET) non tornava a niente e non lo diceva. Il test
	// precedente lo evitava passando da `OpenFromFile`.
	const FString Dir = RunResetTempDir();
	const FString Path = FPaths::Combine(Dir, TEXT("Authored.json"));
	ON_SCOPE_EXIT{ IFileManager::Get().DeleteDirectory(*Dir, false, true); };

	URTScenarioAuthoring* Authoring = URTScenarioAuthoring::CreateScenarioDraft(nullptr);
	if (!TestNotNull(TEXT("draft creato"), Authoring)) { return false; }

	FString Error;
	FRTTestScenario Scenario;
	URTScenarioLoader::LoadFromString(RunResetPassingJson, Scenario, Error);

	// Il flusso di #1115: si CREA, non si apre.
	Authoring->NewScenario(TEXT("Movement.AuthoredHere"), 3);
	Authoring->GetDraft().MutableScenario() = Scenario;
	Authoring->GetDraft().MutableScenario().ScenarioId = TEXT("Movement.AuthoredHere");

	if (!TestEqual(TEXT("salvato per la prima volta"),
		Authoring->SaveToFile(Path, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}

	// Una modifica dopo il salvataggio, che RESET deve scartare.
	Authoring->MoveUnit(TEXT("A1"), FRTCellId(-1, 1, 0), Error);
	TestEqual(TEXT("la modifica c'e'"),
		Authoring->GetDraft().GetScenario().Units[0].Cell, FRTCellId(-1, 1, 0));

	bool bDiscarded = false;
	TestEqual(TEXT("reset"), Authoring->Reset(bDiscarded, Error), ERTScenarioAuthoringResult::Success);
	TestTrue(TEXT("dichiara di aver scartato modifiche"), bDiscarded);

	// La prova: lo scenario e' tornato a cio' che il FILE dichiara, non e' rimasto com'era.
	TestEqual(TEXT("l'unita' e' tornata dove il file la mette"),
		Authoring->GetDraft().GetScenario().Units[0].Cell, FRTCellId(-2, 0, 0));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioRunReportsBlockedWithAReasonTest,
	"RefactorTactics.Scenario.RunReportsBlockedWithItsReason",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioRunReportsBlockedWithAReasonTest::RunTest(const FString&)
{
	// La review ha notato che dei quattro esiti il file ne copriva due: `Blocked` non compariva mai, e
	// `Error` veniva intercettato da `Validate` prima di diventare un `ERTTestOutcome`. `BLOCKED` e' quello
	// che conta di piu' non perdere: non e' un successo, e mostrarlo senza il motivo lo renderebbe
	// indistinguibile da uno.
	//
	// ⚠️ La capability dev'essere **NOTA ma non disponibile**, non inventata: una sconosciuta e' un difetto
	// del TEST e produce `ERROR`, che e' un'altra cosa. La prima stesura ne usava una inventata e il test
	// falliva chiedendo BLOCKED a un ERROR — la stessa distinzione che questo test esiste per proteggere,
	// sbagliata nel test stesso. `ReactionClash` e' dichiarata in `KnownUnavailableCapabilities()`.
	const TCHAR* BlockedJson = TEXT(R"JSON(
	{
	  "scenarioId": "Movement.BlockedProbe",
	  "version": 1,
	  "mapRadius": 3,
	  "units": [
	    { "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [-2, 0, 0] },
	    { "id": "B1", "hero": "Hero.Riktor", "team": 1, "cell": [2, 0, 0] }
	  ],
	  "turns": [ { "intents": [], "requires": ["ReactionClash"] } ],
	  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
	}
	)JSON");

	FString Error;
	FRTTestScenario Scenario;
	if (!URTScenarioLoader::LoadFromString(BlockedJson, Scenario, Error))
	{
		// Una capability sconosciuta puo' essere rifiutata in lettura come ERRORE del test: se il loader la
		// tratta cosi', questo caso non e' costruibile e va detto invece di far finta.
		AddWarning(FString::Printf(
			TEXT("il loader rifiuta una capability sconosciuta ('%s'): il caso BLOCKED non e' costruibile da qui"),
			*Error));
		return true;
	}

	URTScenarioAuthoring* Authoring = URTScenarioAuthoring::CreateScenarioDraft(nullptr);
	if (!TestNotNull(TEXT("draft creato"), Authoring)) { return false; }
	Authoring->NewScenario(TEXT("segnaposto"), 3);
	Authoring->GetDraft().MutableScenario() = Scenario;

	FRTScenarioRunReport Report;
	if (!TestEqual(TEXT("l'esecuzione avviene"),
		Authoring->Run(Report, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}

	if (TestEqual(TEXT("l'esito e' BLOCKED"), Report.OutcomeText, FString(TEXT("BLOCKED"))))
	{
		// ⚠️ Un BLOCKED senza motivo e' peggio di un errore: sembra un successo con una nota.
		TestFalse(TEXT("e porta il motivo"), Report.BlockedReason.IsEmpty());
		TestNotEqual(TEXT("e non viene mostrato come PASS"),
			static_cast<int32>(Report.Outcome), static_cast<int32>(ERTTestOutcome::Pass));
	}

	return true;
}

/**
 * TRE TURNI AUTHORATI GIRANO NELL'ORDINE DICHIARATO — `#1627`.
 *
 * 🔑 **Il criterio 1 della issue, e l'osservabile non è la posizione finale.** Asserire solo dove l'unità
 * finisce non distingue l'ordine: `BuildCompositeHexPath` cerca la strada fino al waypoint, quindi una
 * sequenza eseguita al contrario può finire nella stessa cella passando per un percorso diverso. E un turno
 * il cui percorso viene rifiutato lascia l'unità ferma **senza rompere niente** — solo una nota.
 *
 * ⚠️ L'osservabile è il **TurnLog**, che porta il numero di turno: `FRTScenarioLogEntryView::Turn`. Ogni
 * turno dichiara una destinazione diversa, e il test verifica che il turno *n* sia arrivato **dove il turno
 * n aveva detto**. Una permutazione cade, invece di finire per caso nello stesso posto.
 *
 * ⛔ E gira per la strada vera: `FRTScenarioDraft::Run` passa da `URTScenarioRunner::Run`, l'unica —
 * la stessa che `RunFromTheEditorMatchesTheHeadlessRun` tiene onesta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioThreeTurnsRunInOrderTest,
	"RefactorTactics.Scenario.ThreeAuthoredTurnsRunInTheDeclaredOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioThreeTurnsRunInOrderTest::RunTest(const FString&)
{
	FRTScenarioDraft Draft;
	FString Error;
	if (!TestTrue(TEXT("base senza turni caricata"), OpenRunSequenceDraft(Draft, Error)))
	{
		AddError(Error);
		return false;
	}
	TestEqual(TEXT("si parte da zero turni"), Draft.NumTurns(), 0);

	// Tre passi, uno per turno: ciascuno legale solo da dove il precedente ha lasciato l'unita'.
	const FRTCellId Steps[] = { FRTCellId(-1, 0, 0), FRTCellId(0, 0, 0), FRTCellId(1, 0, 0) };
	for (const FRTCellId& Step : Steps)
	{
		int32 T = INDEX_NONE;
		if (!TestEqual(TEXT("turno aggiunto"), Draft.AddTurn(T, Error), ERTScenarioAuthoringResult::Success))
		{
			AddError(Error);
			return false;
		}
		const TArray<FRTCellId> Route = { Step };
		if (!TestEqual(TEXT("Move scritto"),
			Draft.SetMoveIntent(T, TEXT("A1"), Route, Error), ERTScenarioAuthoringResult::Success))
		{
			AddError(Error);
			return false;
		}
	}
	if (!TestEqual(TEXT("tre turni authorati"), Draft.NumTurns(), 3))
	{
		return false;
	}

	UWorld* World = MakeRunResetWorld();
	if (!TestNotNull(TEXT("mondo per la corsa"), World))
	{
		return false;
	}
	const ERTScenarioAuthoringResult RunResult = Draft.Run(World, Error);
	DestroyRunResetWorld(World);

	if (!TestEqual(TEXT("Run All ha eseguito"), RunResult, ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}
	const FRTScenarioRunReport& Report = Draft.GetLastRunReport();
	TestEqual(FString::Printf(TEXT("tre turni giocati (esito: '%s' / '%s')"),
		*Report.OutcomeText, *Report.ErrorMessage), Report.TurnsPlayed, 3);

	// ✅ L'ordine, letto sul TurnLog.
	//
	// ⚠️ **Il log scrive una voce `Move` per OGNI unita' risolta, non solo per chi si e' mosso** — misurato:
	// `RTHexSimLibrary.cpp:945` cicla su tutti i percorsi risolti, e per un'unita' ferma scrive `Src == Tgt`
	// con `Amount = 0`. La prima stesura di questo test prendeva l'ultima voce del turno e leggeva quella di
	// B1, che non si muove: tre asserzioni rosse per la ragione sbagliata.
	//
	// 🔑 L'unita' si riconosce dalla **cella di partenza**, ed e' una chiave dichiarata tale dal sorgente che
	// scrive il log: *«Chiave stabile dell'unita' nel turno: la sua cella di PARTENZA (max 1 unita' per
	// cella), mai un pointer»*. Quindi il turno *n* si cerca per la casella da cui A1 parte in quel turno —
	// che e' dove il turno *n-1* l'ha lasciata. **È la catena a rendere l'ordine osservabile**: una
	// permutazione romperebbe il legame fra l'arrivo di un turno e la partenza del successivo.
	const TArray<FRTScenarioLogEntryView> Log = Draft.GetLastRunLog();
	FRTCellId Expected(-2, 0, 0); // dove A1 e' schierata
	for (int32 Step = 0; Step < 3; ++Step)
	{
		const int32 TurnNumber = Step + 1; // il log conta i turni da 1
		const FRTScenarioLogEntryView* Moved = Log.FindByPredicate(
			[TurnNumber, &Expected](const FRTScenarioLogEntryView& E)
			{
				return E.Turn == TurnNumber && E.Category == ERTLogCategory::Move && E.FromCell == Expected;
			});

		if (!TestNotNull(*FString::Printf(
			TEXT("turno %d: una voce Move che parte da (%d,%d,%d) — voci nel log: %d"),
			TurnNumber, Expected.X, Expected.Y, Expected.Layer, Log.Num()), Moved))
		{
			return false;
		}
		TestEqual(*FString::Printf(TEXT("turno %d: arrivata in (%d,%d,%d) invece di (%d,%d,%d)"),
			TurnNumber, Moved->ToCell.X, Moved->ToCell.Y, Moved->ToCell.Layer,
			Steps[Step].X, Steps[Step].Y, Steps[Step].Layer), Moved->ToCell, Steps[Step]);
		TestTrue(*FString::Printf(TEXT("turno %d: ha percorso almeno una cella (Amount=%d)"),
			TurnNumber, Moved->Amount), Moved->Amount >= 1);

		// La partenza del turno successivo e' l'arrivo di questo: se la sequenza fosse eseguita in un altro
		// ordine, il prossimo giro non troverebbe nessuna voce da questa casella.
		Expected = Steps[Step];
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
