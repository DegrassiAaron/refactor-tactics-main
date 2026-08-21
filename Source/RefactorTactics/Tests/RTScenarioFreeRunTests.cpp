// Free-run degli scenari (CP 47.4): «gioca fino alla fine partita» invece di enumerare i turni.
//
// Il test che conta di piu' non e' quello verde: e' `CapReachedIsFailNotPass`. Il tetto di turni esiste per
// cogliere una partita che NON finisce — lo stallo misurato in `#1088`, dodici round di soli spostamenti e un
// pareggio allo scadere — e un tetto che producesse `Pass` renderebbe verde esattamente cio' che deve cogliere.
// Senza quel test, il free-run sarebbe un modo elaborato di non verificare niente.

#include "Misc/AutomationTest.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/RTScenarioSession.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nomi distinti da ogni altro file di test: la unity build condivide la translation unit.
	UWorld* MakeFreeRunWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyFreeRunWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/**
	 * Il 2v2 bot-contro-bot su cui girano questi test, con la GEOMETRIA di `HexMatch.PlaysToCompletion`:
	 * arena di raggio 5, squadre agli angoli opposti in diagonale, dove la distanza esagonale conta davvero.
	 *
	 * Riferirsi a quella configurazione invece di inventarne una non e' pigrizia: e' l'unico allestimento del
	 * repository di cui si sappia, misurato, che la partita si decide — e un free-run che non finisse per la
	 * geometria direbbe `Fail` parlando dell'arena invece che del free-run.
	 */
	FRTTestScenario MakeAutobattleScenario(int32 MaxTurns)
	{
		FRTTestScenario S;
		S.ScenarioId = TEXT("FreeRun.Fixture");
		S.Version = 4;
		S.MapRadius = 5;
		S.bFreeRun = true;
		S.MaxTurns = MaxTurns;

		auto AddBot = [&S](const TCHAR* Id, const TCHAR* Hero, int32 Team, const FRTCellId& Cell)
		{
			FRTScenarioUnit U;
			U.Id = Id;
			U.HeroId = FName(Hero);
			U.TeamId = Team;
			U.Cell = Cell;
			U.bBotControlled = true;
			S.Units.Add(U);
		};

		AddBot(TEXT("A1"), TEXT("Hero.Gadget"), 0, FRTCellId(-4, 2, 0));
		AddBot(TEXT("A2"), TEXT("Hero.Phase"),  0, FRTCellId(-4, 3, 0));
		AddBot(TEXT("B1"), TEXT("Hero.Riktor"), 1, FRTCellId(4, -2, 0));
		AddBot(TEXT("B2"), TEXT("Hero.Wraith"), 1, FRTCellId(4, -3, 0));

		// ⚠️ L'`expect` serve anche a un free-run, e la regola non e' stata rilassata per fargli spazio: «almeno
		// una assertion, altrimenti lo scenario passerebbe sempre» vale identica. `MatchReachedEnd` la genera la
		// sessione, e una regola che accettasse un file vuoto perche' «tanto una assertion arriva dopo» sposterebbe
		// il gate dal file al runtime — cioe' lo toglierebbe a chi scrive lo scenario.
		FRTTestExpectation Played;
		Played.Kind = ERTAssertionKind::TurnsCompleted;
		Played.Value = 1;
		S.Expect.Add(Played);
		return S;
	}

	/** L'assertion generata dal free-run, per descrizione. Nullptr se il free-run non l'ha prodotta. */
	const FRTAssertionResult* FindMatchReachedEnd(const FRTTestResult& Result)
	{
		return Result.Assertions.FindByPredicate([](const FRTAssertionResult& A)
		{
			return A.Description.Contains(TEXT("MatchReachedEnd"));
		});
	}

	/** Uno scenario valido a turni enumerati, minimo: serve ai test che verificano i RIFIUTI del loader. */
	FRTTestScenario MakeTurnBasedScenario()
	{
		FRTTestScenario S;
		S.ScenarioId = TEXT("FreeRun.TurnBased");
		S.Version = 4;
		S.MapRadius = 3;

		FRTScenarioUnit U;
		U.Id = TEXT("A1");
		U.HeroId = TEXT("Hero.Gadget");
		U.TeamId = 0;
		U.Cell = FRTCellId(-1, 0, 0);
		S.Units.Add(U);

		FRTScenarioTurn T;
		S.Turns.Add(T);

		FRTTestExpectation Played;
		Played.Kind = ERTAssertionKind::TurnsCompleted;
		Played.Value = 1;
		S.Expect.Add(Played);
		return S;
	}
}

/**
 * Il caso nominale: quattro bot, nessun turno enumerato, e la partita arriva a una fine.
 *
 * Verifica anche che il free-run abbia giocato PIU' di un turno: uno scenario che finisse al primo darebbe
 * `Pass` senza aver dimostrato che il ciclo si ripete, ed e' il modo in cui un free-run puo' essere verde e
 * vuoto insieme.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFreeRunPlaysToMatchEndTest,
	"RefactorTactics.Scenario.FreeRun.PlaysToMatchEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFreeRunPlaysToMatchEndTest::RunTest(const FString&)
{
	const FRTTestScenario Scenario = MakeAutobattleScenario(/*MaxTurns=*/ 40);

	FString ValidationError;
	if (!TestTrue(TEXT("lo scenario free-run e' valido"), URTScenarioLoader::Validate(Scenario, ValidationError)))
	{
		AddError(ValidationError);
		return false;
	}

	UWorld* World = MakeFreeRunWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	DestroyFreeRunWorld(World);

	TestEqual(TEXT("esito"), Result.OutcomeString(), FString(TEXT("PASS")));
	TestTrue(FString::Printf(TEXT("piu' di un turno giocato (ne ha giocati %d)"), Result.TurnsPlayed),
		Result.TurnsPlayed > 1);
	TestTrue(FString::Printf(TEXT("il tetto non e' stato raggiunto (%d < 40)"), Result.TurnsPlayed),
		Result.TurnsPlayed < 40);

	const FRTAssertionResult* Reached = FindMatchReachedEnd(Result);
	if (!TestNotNull(TEXT("il free-run genera l'assertion MatchReachedEnd"), Reached))
	{
		return false;
	}
	TestTrue(FString::Printf(TEXT("la partita e' finita: %s"), *Reached->Actual), Reached->bPassed);
	return true;
}

/**
 * 🔴 **Il tetto raggiunto e' un `Fail`, non un `Pass`.**
 *
 * Stesso allestimento del test qui sopra — che si decide ben oltre il secondo turno — con `maxTurns = 2`. La
 * partita e' ancora in corso quando la sessione si ferma, e l'unica risposta corretta e' rossa: una partita
 * che non finisce e' un difetto del GIOCO, ed e' il difetto che `#1088` ha misurato.
 *
 * ⚠️ Il `Fail` deve arrivare **dall'assertion**, non da un codice d'esito speciale: e' cio' che porta nel
 * report l'atteso e l'ottenuto, e che distingue «non e' finita» da «lo scenario e' scritto male» (`Error`).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFreeRunCapIsFailTest,
	"RefactorTactics.Scenario.FreeRun.CapReachedIsFailNotPass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFreeRunCapIsFailTest::RunTest(const FString&)
{
	const FRTTestScenario Scenario = MakeAutobattleScenario(/*MaxTurns=*/ 2);

	UWorld* World = MakeFreeRunWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	DestroyFreeRunWorld(World);

	TestEqual(TEXT("esito"), Result.OutcomeString(), FString(TEXT("FAIL")));
	TestEqual(TEXT("il tetto ha fermato la partita al secondo turno"), Result.TurnsPlayed, 2);

	const FRTAssertionResult* Reached = FindMatchReachedEnd(Result);
	if (!TestNotNull(TEXT("il free-run genera l'assertion MatchReachedEnd"), Reached))
	{
		return false;
	}
	TestFalse(TEXT("l'assertion e' rossa"), Reached->bPassed);
	// Il messaggio deve nominare il tetto: un rosso che dicesse solo «false» rimanderebbe a rieseguire.
	TestTrue(FString::Printf(TEXT("l'attesa nomina il tetto: '%s'"), *Reached->Expected),
		Reached->Expected.Contains(TEXT("2 turni")));
	TestTrue(FString::Printf(TEXT("l'osservato dice che e' ancora in corso: '%s'"), *Reached->Actual),
		Reached->Actual.Contains(TEXT("ancora in corso")));
	return true;
}

/**
 * `repeatCount`: due esecuzioni identiche danno lo stesso TurnLog e lo stesso StateHash.
 *
 * ⚠️ La premessa e' asserita nella stessa run — la partita ha giocato piu' di un turno — perche' due partite
 * in cui non succede niente hanno tracce identiche: e' la stessa cautela che `bExpectSameAcrossVariants`
 * scrive nel proprio commento, e senza di essa questo test sarebbe verde anche su un free-run che non parte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFreeRunRepeatDeterminismTest,
	"RefactorTactics.Scenario.FreeRun.RepeatedRunsAreIdentical",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFreeRunRepeatDeterminismTest::RunTest(const FString&)
{
	FRTTestScenario Scenario = MakeAutobattleScenario(/*MaxTurns=*/ 40);
	Scenario.RepeatCount = 2;

	FString ValidationError;
	if (!TestTrue(TEXT("lo scenario e' valido"), URTScenarioLoader::Validate(Scenario, ValidationError)))
	{
		AddError(ValidationError);
		return false;
	}

	UWorld* World = MakeFreeRunWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	DestroyFreeRunWorld(World);

	TestEqual(TEXT("esito"), Result.OutcomeString(), FString(TEXT("PASS")));
	TestTrue(FString::Printf(TEXT("la premessa: qualcosa e' successo (%d turni)"), Result.TurnsPlayed),
		Result.TurnsPlayed > 1);
	TestTrue(TEXT("le tracce della prima esecuzione sono nel report"), Result.TurnTraces.Num() > 1);

	const FRTAssertionResult* SameLog = Result.Assertions.FindByPredicate([](const FRTAssertionResult& A)
	{
		return A.Description.Contains(TEXT("SameTurnLogAcrossRuns"));
	});
	const FRTAssertionResult* SameHash = Result.Assertions.FindByPredicate([](const FRTAssertionResult& A)
	{
		return A.Description.Contains(TEXT("SameStateHashAcrossRuns"));
	});

	if (!TestNotNull(TEXT("confronto del TurnLog fra le due esecuzioni"), SameLog)) { return false; }
	if (!TestNotNull(TEXT("confronto dello StateHash fra le due esecuzioni"), SameHash)) { return false; }
	TestTrue(FString::Printf(TEXT("stesso TurnLog: %s"), *SameLog->Actual), SameLog->bPassed);
	TestTrue(FString::Printf(TEXT("stesso StateHash: %s contro %s"), *SameHash->Expected, *SameHash->Actual),
		SameHash->bPassed);
	// Entrambe le esecuzioni riportano le proprie assertion, col numero davanti: senza, un rosso non direbbe
	// in quale delle due e' caduto.
	const bool bHasRunPrefix = Result.Assertions.ContainsByPredicate([](const FRTAssertionResult& A)
	{
		return A.Description.StartsWith(TEXT("[run 2]"));
	});
	TestTrue(TEXT("le assertion della seconda esecuzione sono nel report, col prefisso"), bHasRunPrefix);
	return true;
}

/**
 * Una capability NOTA e non disponibile chiesta dall'intero scenario da' `Blocked`, non `Fail`.
 *
 * E' il buco che il free-run apriva: `requires` vive sul turno, e un free-run non ha turni.
 *
 * 🔴 **Verificato per mutazione, e l'esito senza il blocco non e' quello che sembra**: disattivando il
 * controllo, questo test cade dicendo `PASS` invece di `BLOCKED` con **10 turni** giocati. Cioe' il rischio
 * non era un rosso ingiusto — era un VERDE su una partita finita per **eliminazione** dentro uno scenario che
 * dichiara di misurare l'**obiettivo**.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFreeRunScenarioRequiresTest,
	"RefactorTactics.Scenario.FreeRun.ScenarioRequiresBlocksWithoutPlaying",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFreeRunScenarioRequiresTest::RunTest(const FString&)
{
	// La premessa del test, non un'assunzione: se un giorno `Objective` diventasse disponibile, questo test
	// direbbe cosa e' cambiato invece di fallire su un `Blocked` mancante.
	if (!TestFalse(TEXT("premessa: 'Objective' non e' ancora disponibile"),
		FRTScenarioSession::IsAvailableCapability(TEXT("Objective"))))
	{
		return false;
	}
	TestTrue(TEXT("premessa: 'Objective' e' comunque un nome noto"),
		FRTScenarioSession::IsKnownCapability(TEXT("Objective")));

	FRTTestScenario Scenario = MakeAutobattleScenario(/*MaxTurns=*/ 40);
	Scenario.Requires.Add(TEXT("Objective"));

	UWorld* World = MakeFreeRunWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	DestroyFreeRunWorld(World);

	TestEqual(TEXT("esito"), Result.OutcomeString(), FString(TEXT("BLOCKED")));
	TestEqual(TEXT("nessun turno giocato"), Result.TurnsPlayed, 0);
	TestTrue(FString::Printf(TEXT("il motivo nomina la capability: '%s'"), *Result.BlockedReason),
		Result.BlockedReason.Contains(TEXT("Objective")));
	// Le assertion di fine scenario NON si valutano su una partita che non e' stata giocata, e il free-run non
	// fa eccezione: un `MatchReachedEnd` rosso qui direbbe «il gioco e' rotto» di un'attesa legittima.
	TestNull(TEXT("nessuna MatchReachedEnd su uno scenario bloccato"), FindMatchReachedEnd(Result));
	return true;
}

/**
 * Un nome di capability che non esiste resta un `Error`, anche dalla chiave di scenario: un refuso e' un
 * difetto di chi scrive, non un'attesa del gioco. E' la stessa distinzione di
 * `Scenario.UnknownCapabilityIsErrorNotBlocked`, sul percorso che il free-run ha aggiunto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFreeRunUnknownCapabilityTest,
	"RefactorTactics.Scenario.FreeRun.UnknownScenarioCapabilityIsError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFreeRunUnknownCapabilityTest::RunTest(const FString&)
{
	FRTTestScenario Scenario = MakeAutobattleScenario(/*MaxTurns=*/ 40);
	Scenario.Requires.Add(TEXT("ObjectiveTypo"));

	UWorld* World = MakeFreeRunWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	DestroyFreeRunWorld(World);

	TestEqual(TEXT("esito"), Result.OutcomeString(), FString(TEXT("ERROR")));
	TestTrue(FString::Printf(TEXT("il motivo nomina il refuso: '%s'"), *Result.ErrorMessage),
		Result.ErrorMessage.Contains(TEXT("ObjectiveTypo")));
	return true;
}

/**
 * Le regole di forma del free-run, una per caso, con il rifiuto atteso.
 *
 * Sta in `Validate` e non nel parser perche' `Validate` e' il gate che OGNI strada attraversa — anche gli
 * scenari costruiti in memoria, come quelli di questo file. Un controllo scritto solo in `LoadFromString` lo
 * vedrebbe chi arriva da JSON e nessun altro.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFreeRunValidationTest,
	"RefactorTactics.Scenario.FreeRun.ValidatorRejectsIncoherentForms",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFreeRunValidationTest::RunTest(const FString&)
{
	auto Rejects = [this](const TCHAR* What, const FRTTestScenario& Scenario, const TCHAR* MustMention)
	{
		FString Error;
		const bool bValid = URTScenarioLoader::Validate(Scenario, Error);
		TestFalse(FString::Printf(TEXT("%s: rifiutato"), What), bValid);
		TestTrue(FString::Printf(TEXT("%s: il motivo nomina '%s' (era: %s)"), What, MustMention, *Error),
			Error.Contains(MustMention));
	};

	// Il free-run e i turni enumerati dicono due cose e ne eseguono una.
	{
		FRTTestScenario S = MakeAutobattleScenario(40);
		S.Turns.Add(FRTScenarioTurn());
		Rejects(TEXT("freeRun con turni"), S, TEXT("freeRun con 1 turni"));
	}
	// Un tetto che il file non dichiara e' un tetto che nessuno rivede.
	{
		FRTTestScenario S = MakeAutobattleScenario(0);
		Rejects(TEXT("freeRun senza maxTurns"), S, TEXT("maxTurns"));
	}
	// Oltre il tetto del runner la guardia non e' applicabile.
	{
		FRTTestScenario S = MakeAutobattleScenario(URTScenarioRunner::MaxTurnsHardCap + 1);
		Rejects(TEXT("maxTurns oltre l'hard cap"), S, TEXT("tetto del runner"));
	}
	// Un'unita' non-bot in free-run non ha nessuno che le scriva l'intent.
	{
		FRTTestScenario S = MakeAutobattleScenario(40);
		S.Units[0].bBotControlled = false;
		Rejects(TEXT("free-run con un'unita' umana"), S, TEXT("non guidata dal bot"));
	}
	// I campi del free-run in uno scenario a turni sono campi che non contano.
	{
		FRTTestScenario S = MakeTurnBasedScenario();
		S.MaxTurns = 12;
		Rejects(TEXT("maxTurns senza freeRun"), S, TEXT("senza freeRun"));
	}
	{
		FRTTestScenario S = MakeTurnBasedScenario();
		S.Requires.Add(TEXT("Objective"));
		Rejects(TEXT("requires di scenario senza freeRun"), S, TEXT("il posto del requisito e' il turno"));
	}
	// Le due domande opposte non si mescolano.
	//
	// ⚠️ Le varianti sono **due e ben formate**, e non e' un dettaglio del test: con una sola, a parlare
	// sarebbe la regola «o sono due o piu'» e il caso verificherebbe quella. Un caso che viola due regole
	// insieme misura l'ORDINE dei controlli, non il controllo che dice di misurare.
	{
		FRTTestScenario S = MakeTurnBasedScenario();
		S.RepeatCount = 2;
		auto MakeVariant = [](const TCHAR* Name, const FRTCellId& Cell)
		{
			FRTScenarioVariantUnit Moved;
			Moved.Id = TEXT("A1");
			Moved.Cell = Cell;

			FRTScenarioVariant V;
			V.Name = Name;
			V.Units.Add(Moved);
			return V;
		};
		S.Variants.Add(MakeVariant(TEXT("sinistra"), FRTCellId(0, 0, 0)));
		S.Variants.Add(MakeVariant(TEXT("destra"), FRTCellId(1, 0, 0)));
		Rejects(TEXT("repeatCount con variants"), S, TEXT("repeatCount e variants insieme"));
	}
	{
		FRTTestScenario S = MakeAutobattleScenario(40);
		S.RepeatCount = 0;
		Rejects(TEXT("repeatCount zero"), S, TEXT("almeno 1"));
	}

	// E il caso valido resta valido: un validator che rifiutasse tutto passerebbe i sette casi qui sopra.
	{
		FString Error;
		const FRTTestScenario S = MakeAutobattleScenario(40);
		TestTrue(FString::Printf(TEXT("il free-run ben formato e' accettato (%s)"), *Error),
			URTScenarioLoader::Validate(S, Error));
	}
	return true;
}

/**
 * Lo scenario VERSIONATO `AutoBattle.OpenField` arriva a un vincitore.
 *
 * `EveryShippedScenarioRuns` lo esegue gia', ma accetta `BLOCKED` per costruzione: non potrebbe distinguere
 * «la partita si e' decisa» da «lo scenario aspetta qualcosa». Qui la domanda e' quella, e vale la pena
 * chiederla su un file versionato invece che su un allestimento scritto nel test — e' la differenza fra
 * verificare il gioco e verificare il proprio setup.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFreeRunShippedOpenFieldTest,
	"RefactorTactics.Scenario.FreeRun.ShippedOpenFieldReachesAWinner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFreeRunShippedOpenFieldTest::RunTest(const FString&)
{
	FString ResolveError;
	const FString Path = URTScenarioIndex::ResolvePath(TEXT("AutoBattle.OpenField"), ResolveError);
	if (Path.IsEmpty())
	{
		AddError(FString::Printf(TEXT("AutoBattle.OpenField non risolto: %s"), *ResolveError));
		return false;
	}

	FRTTestScenario Scenario;
	FString LoadError;
	if (!URTScenarioLoader::LoadFromFile(Path, Scenario, LoadError))
	{
		AddError(FString::Printf(TEXT("AutoBattle.OpenField non si carica: %s"), *LoadError));
		return false;
	}
	TestTrue(TEXT("e' dichiarato free-run"), Scenario.bFreeRun);
	TestEqual(TEXT("non enumera turni"), Scenario.Turns.Num(), 0);

	UWorld* World = MakeFreeRunWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	DestroyFreeRunWorld(World);

	TestEqual(TEXT("esito"), Result.OutcomeString(), FString(TEXT("PASS")));

	const FRTAssertionResult* Reached = FindMatchReachedEnd(Result);
	if (!TestNotNull(TEXT("MatchReachedEnd"), Reached)) { return false; }
	TestTrue(FString::Printf(TEXT("la partita si decide: %s"), *Reached->Actual), Reached->bPassed);
	// Un pareggio allo scadere sarebbe una fine partita, ma non un vincitore: e' il caso di `#1088`, e qui
	// dev'essere escluso per nome.
	TestFalse(FString::Printf(TEXT("non e' un pareggio allo scadere: %s"), *Reached->Actual),
		Reached->Actual.Contains(TEXT("allo scadere")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
