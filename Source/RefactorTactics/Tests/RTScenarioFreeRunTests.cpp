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
	 * Il 2v2 bot-contro-bot su cui girano questi test: **il file versionato** `AutoBattle.OpenField`, con il
	 * solo `maxTurns` sovrascritto.
	 *
	 * ⚠️ **Non una copia scritta in C++**, e la differenza non e' di stile: una copia dello stesso allestimento
	 * diverge in silenzio al primo edit del file — si sposta un'unita' nel JSON e i test continuano a misurare
	 * il vecchio, dicendo di caratterizzare lo scenario spedito. Caricarlo significa anche che ogni test qui
	 * passa dalla stessa validazione del corpus: `LoadFromFile` chiama `Validate`, quindi un file rotto fallisce
	 * dicendo cosa, invece di produrre un `Fail` di gioco.
	 *
	 * Il `maxTurns` resta un parametro perche' e' cio' che i casi limite devono poter muovere: il file dichiara
	 * la sua guardia, il test ne dichiara un'altra per vedere cosa succede quando la si tocca. **`0` lascia
	 * quella del file** — che e' il caso di chi vuole misurare lo scenario spedito com'e'.
	 */
	bool LoadAutobattleScenarioById(FAutomationTestBase& Test, const TCHAR* Id, int32 MaxTurns,
		FRTTestScenario& Out)
	{
		FString ResolveError;
		const FString Path = URTScenarioIndex::ResolvePath(Id, ResolveError);
		if (Path.IsEmpty())
		{
			Test.AddError(FString::Printf(TEXT("%s non risolto: %s"), Id, *ResolveError));
			return false;
		}

		FString LoadError;
		if (!URTScenarioLoader::LoadFromFile(Path, Out, LoadError))
		{
			Test.AddError(FString::Printf(TEXT("%s non si carica: %s"), Id, *LoadError));
			return false;
		}
		if (MaxTurns > 0)
		{
			Out.MaxTurns = MaxTurns;
		}
		return true;
	}

	/** L'allestimento storico di questo file: `AutoBattle.OpenField`. */
	bool LoadAutobattleScenario(FAutomationTestBase& Test, int32 MaxTurns, FRTTestScenario& Out)
	{
		return LoadAutobattleScenarioById(Test, TEXT("AutoBattle.OpenField"), MaxTurns, Out);
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
 * 🔴 **Il tetto raggiunto e' un `Fail`, non un `Pass`.**
 *
 * Lo scenario spedito `AutoBattle.OpenField` — che si decide al decimo turno — con `maxTurns = 2`. La partita
 * e' ancora in corso quando la sessione si ferma, e l'unica risposta corretta e' rossa: una partita che non
 * finisce e' un difetto del GIOCO, ed e' il difetto che `#1088` ha misurato.
 *
 * ⚠️ Il `Fail` deve arrivare **dall'assertion**, non da un codice d'esito speciale: e' cio' che porta nel
 * report l'atteso e l'ottenuto, e che distingue «non e' finita» da «lo scenario e' scritto male» (`Error`).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFreeRunCapIsFailTest,
	"RefactorTactics.Scenario.FreeRun.CapReachedIsFailNotPass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFreeRunCapIsFailTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!LoadAutobattleScenario(*this, /*MaxTurns=*/ 2, Scenario)) { return false; }

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
	FRTTestScenario Scenario;
	if (!LoadAutobattleScenario(*this, /*MaxTurns=*/ 40, Scenario)) { return false; }
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

	FRTTestScenario Scenario;
	if (!LoadAutobattleScenario(*this, /*MaxTurns=*/ 40, Scenario)) { return false; }
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
	FRTTestScenario Scenario;
	if (!LoadAutobattleScenario(*this, /*MaxTurns=*/ 40, Scenario)) { return false; }
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

	// Il free-run ben formato da cui partono i casi: si carica UNA volta e ogni caso ne rompe una cosa sola.
	FRTTestScenario Valido;
	if (!LoadAutobattleScenario(*this, /*MaxTurns=*/ 40, Valido)) { return false; }
	auto ConTetto = [&Valido](int32 MaxTurns)
	{
		FRTTestScenario S = Valido;
		S.MaxTurns = MaxTurns;
		return S;
	};

	// Il free-run e i turni enumerati dicono due cose e ne eseguono una.
	{
		FRTTestScenario S = ConTetto(40);
		S.Turns.Add(FRTScenarioTurn());
		Rejects(TEXT("freeRun con turni"), S, TEXT("freeRun con 1 turni"));
	}
	// Un tetto che il file non dichiara e' un tetto che nessuno rivede.
	{
		FRTTestScenario S = ConTetto(0);
		Rejects(TEXT("freeRun senza maxTurns"), S, TEXT("maxTurns"));
	}
	// Oltre il tetto del runner la guardia non e' applicabile.
	{
		FRTTestScenario S = ConTetto(URTScenarioRunner::MaxTurnsHardCap + 1);
		Rejects(TEXT("maxTurns oltre l'hard cap"), S, TEXT("tetto del runner"));
	}
	// Un'unita' non-bot in free-run non ha nessuno che le scriva l'intent.
	{
		FRTTestScenario S = ConTetto(40);
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
		FRTTestScenario S = ConTetto(40);
		S.RepeatCount = 0;
		Rejects(TEXT("repeatCount zero"), S, TEXT("almeno 1"));
	}

	// Un tetto di ripetizioni che il runner non sa applicare non e' una guardia.
	{
		FRTTestScenario S = ConTetto(40);
		S.RepeatCount = URTScenarioRunner::MaxRepeatCount + 1;
		Rejects(TEXT("repeatCount oltre il tetto"), S, TEXT("oltre il tetto del runner"));
	}
	// Un campo di presentazione che in free-run non puo' fare niente.
	{
		FRTTestScenario S = ConTetto(40);
		S.PreviewUnit = TEXT("A1");
		Rejects(TEXT("previewUnit con freeRun"), S, TEXT("previewUnit con freeRun"));
	}

	// E il caso valido resta valido: un validator che rifiutasse tutto passerebbe i dieci casi qui sopra.
	//
	// ⚠️ `Error` si legge DOPO la chiamata che lo scrive, e in una variabile: leggerlo dentro l'argomento
	// messaggio della stessa `TestTrue` che invoca `Validate` e' indeterminatamente sequenziato — il messaggio
	// stamperebbe una stringa vuota **proprio quando il test fallisce**, cioe' l'unica volta in cui serve.
	{
		FString Error;
		const bool bValido = URTScenarioLoader::Validate(Valido, Error);
		TestTrue(FString::Printf(TEXT("il free-run ben formato e' accettato (%s)"), *Error), bValido);
	}
	return true;
}

/**
 * Il caso NOMINALE del free-run, sullo scenario versionato: `AutoBattle.OpenField` arriva a un vincitore.
 *
 * `EveryShippedScenarioRuns` lo esegue gia', ma accetta `BLOCKED` per costruzione: non potrebbe distinguere
 * «la partita si e' decisa» da «lo scenario aspetta qualcosa». Qui la domanda e' quella.
 *
 * ⚠️ Verifica anche che i turni giocati siano **piu' di uno e meno del tetto**: una partita decisa al primo
 * turno darebbe `Pass` senza aver dimostrato che il ciclo si ripete — verde e vuoto insieme — e un tetto
 * raggiunto sarebbe il caso di `CapReachedIsFailNotPass`, che vive nel suo test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFreeRunShippedOpenFieldTest,
	"RefactorTactics.Scenario.FreeRun.ShippedOpenFieldReachesAWinner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFreeRunShippedOpenFieldTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	// Il tetto del FILE, non uno del test: qui si misura lo scenario spedito com'e'.
	if (!LoadAutobattleScenario(*this, /*MaxTurns=*/ 0, Scenario)) { return false; }
	const int32 TettoDelFile = Scenario.MaxTurns;
	TestTrue(TEXT("e' dichiarato free-run"), Scenario.bFreeRun);
	TestEqual(TEXT("non enumera turni"), Scenario.Turns.Num(), 0);
	TestTrue(TEXT("il file dichiara il proprio tetto"), TettoDelFile > 0);

	UWorld* World = MakeFreeRunWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	DestroyFreeRunWorld(World);

	TestEqual(TEXT("esito"), Result.OutcomeString(), FString(TEXT("PASS")));
	TestTrue(FString::Printf(TEXT("piu' di un turno giocato (ne ha giocati %d)"), Result.TurnsPlayed),
		Result.TurnsPlayed > 1);
	TestTrue(FString::Printf(TEXT("il tetto non e' stato raggiunto (%d < %d)"), Result.TurnsPlayed, TettoDelFile),
		Result.TurnsPlayed < TettoDelFile);

	const FRTAssertionResult* Reached = FindMatchReachedEnd(Result);
	if (!TestNotNull(TEXT("MatchReachedEnd"), Reached)) { return false; }
	TestTrue(FString::Printf(TEXT("la partita si decide: %s"), *Reached->Actual), Reached->bPassed);
	// Un pareggio allo scadere sarebbe una fine partita, ma non un vincitore: e' il caso di `#1088`, e qui
	// dev'essere escluso per nome.
	TestFalse(FString::Printf(TEXT("non e' un pareggio allo scadere: %s"), *Reached->Actual),
		Reached->Actual.Contains(TEXT("allo scadere")));
	return true;
}

/**
 * **Lo scenario da cui #959 prende l'evidenza arriva a un VINCITORE, e su una mappa multilivello.**
 *
 * Gemello di `ShippedOpenFieldReachesAWinner`, e la ragione per cui non basta quello: `OpenField` gira su
 * `mapRadius`, cioe' un campo generato a **un solo layer**. Il DoD di
 * [#959](https://github.com/DegrassiAaron/refactor-tactics-main/issues/959) chiede una partita «dall'avvio
 * alla vittoria» su **mappa esagonale multilivello** e non sull'arena generata di test — e delle due
 * fixture con piu' di un layer, `TestArena` e' esclusa per nome dal DoD stesso. Resta `ArenaV01`, che e'
 * la geometria del `DA_HexMap_Arena` d'autore: 61 celle di raggio 4 piu' la piattaforma a `L=1`.
 *
 * ⚠️ **Il caso discriminante e' proprio il pareggio.** `D-184` dichiara legittimo il pareggio allo scadere
 * del free-run spedito, e questo test lo esclude PER NOME: un'evidenza che filma un pareggio non e'
 * l'evidenza che `G10` e `G13` chiedono. Misurato il 2026-08-23: **vince il team 0 per eliminazione al
 * turno 19**.
 *
 * ⚠️ **19 e' oltre il `RoundLimit` 12 di `Format.Skirmish2v2`, e non e' una contraddizione**: la sessione
 * di scenario non assegna un `MatchFormat` — il log dice `formato None` — quindi a fermare la partita c'e'
 * il solo tetto del file. E' esattamente cio' che `D-184` prescrive: lo scenario si risolve, il formato
 * spedito non si ritara.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFreeRunArenaV01Test,
	"RefactorTactics.Scenario.FreeRun.ArenaV01ReachesAWinner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFreeRunArenaV01Test::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!LoadAutobattleScenarioById(*this, TEXT("AutoBattle.ArenaV01"), /*MaxTurns=*/ 0, Scenario))
	{
		return false;
	}
	const int32 TettoDelFile = Scenario.MaxTurns;
	TestTrue(TEXT("e' dichiarato free-run"), Scenario.bFreeRun);
	TestEqual(TEXT("riferisce la fixture multilivello"), Scenario.Fixture, FString(TEXT("ArenaV01")));

	UWorld* World = MakeFreeRunWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	DestroyFreeRunWorld(World);

	TestEqual(TEXT("esito"), Result.OutcomeString(), FString(TEXT("PASS")));
	TestTrue(FString::Printf(TEXT("il tetto non e' stato raggiunto (%d < %d)"), Result.TurnsPlayed, TettoDelFile),
		Result.TurnsPlayed < TettoDelFile);

	const FRTAssertionResult* Reached = FindMatchReachedEnd(Result);
	if (!TestNotNull(TEXT("MatchReachedEnd"), Reached)) { return false; }
	AddInfo(FString::Printf(TEXT("turni giocati: %d · esito: %s"), Result.TurnsPlayed, *Reached->Actual));
	TestTrue(FString::Printf(TEXT("la partita si decide: %s"), *Reached->Actual), Reached->bPassed);
	TestFalse(FString::Printf(TEXT("non e' un pareggio allo scadere: %s"), *Reached->Actual),
		Reached->Actual.Contains(TEXT("allo scadere")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
