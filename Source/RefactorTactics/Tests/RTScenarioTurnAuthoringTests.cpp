// Authoring dei turni: Move, Wait, expectation e preview di raggiungibilita' (#1116).
//
// La fetta e' sottile per scelta — due intent e due assertion — e i test lo sono di conseguenza. Quello che
// NON e' sottile e' la domanda a cui rispondono: **chi ha deciso quali celle sono raggiungibili?**
//
// L'issue lo chiede in una forma verificabile: *«la preview del Move chiama il servizio runtime — verificabile
// perche' il codice nuovo non contiene un algoritmo di pathfinding»*. Un test non puo' leggere il diff, ma puo'
// fare una cosa piu' forte: confrontare la preview con la risposta del servizio runtime interrogato
// direttamente. Se qualcuno scrivesse un pathfinder nell'editor, le due risposte divergerebbero al primo caso
// che il servizio tratta e la copia no — ed e' proprio l'ostacolo, l'occupante, l'arco.

#include "Misc/AutomationTest.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "ScenarioHarness/RTScenarioArena.h"
#include "ScenarioHarness/RTScenarioAuthoring.h"
#include "ScenarioHarness/RTScenarioDraft.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "HAL/FileManager.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nome distinto da ogni altro file di test: la unity build condivide la translation unit.
	const TCHAR* TurnAuthoringBaseJson = TEXT(R"JSON(
	{
	  "scenarioId": "Movement.TurnAuthoringProbe",
	  "version": 1,
	  "mapRadius": 3,
	  "units": [
	    { "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [-2, 0, 0] },
	    { "id": "B1", "hero": "Hero.Riktor", "team": 1, "cell": [2, 0, 0] }
	  ],
	  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
	}
	)JSON");

	// Per lo slot REATTIVO serve una unita' che possieda davvero una reazione: `ValidateScenarioTurns`
	// rifiuta `non possiede la reazione`, e provarlo su Gadget misurerebbe il rifiuto invece dello slot.
	// `Hero.Wraith.Deflection` e' la reazione che `Scenarios/Spec/Overwatch/` arma per davvero.
	const TCHAR* TurnAuthoringReactionJson = TEXT(R"JSON(
	{
	  "scenarioId": "Movement.TurnAuthoringReactionProbe",
	  "version": 1,
	  "mapRadius": 3,
	  "units": [
	    { "id": "W1", "hero": "Hero.Wraith", "team": 0, "cell": [-2, 0, 0] },
	    { "id": "B1", "hero": "Hero.Riktor", "team": 1, "cell": [2, 0, 0] }
	  ],
	  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
	}
	)JSON");

	bool OpenTurnDraft(FRTScenarioDraft& OutDraft, FString& OutError)
	{
		FRTTestScenario Loaded;
		if (!URTScenarioLoader::LoadFromString(TurnAuthoringBaseJson, Loaded, OutError))
		{
			return false;
		}
		OutDraft.NewScenario(TEXT("segnaposto"), 3);
		OutDraft.MutableScenario() = Loaded;
		return true;
	}

	FString TurnAuthoringTempDir()
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Tests"), TEXT("TurnAuthoring"));
	}

	// Il runner vuole un mondo vero: lo scenario passa dal percorso di gioco reale, non da una scorciatoia.
	// Stesso helper di `RTScenarioRunnerTests.cpp`, con un nome distinto per la unity build.
	UWorld* MakeTurnAuthoringWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyTurnAuthoringWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}
}

// --- Move e Wait ----------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioTurnAuthoringMoveAndWaitTest,
	"RefactorTactics.Scenario.TurnAuthoringWritesMoveAndWait",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioTurnAuthoringMoveAndWaitTest::RunTest(const FString&)
{
	FRTScenarioDraft Draft;
	FString Error;
	if (!TestTrue(TEXT("scenario di partenza caricato"), OpenTurnDraft(Draft, Error)))
	{
		AddError(Error);
		return false;
	}

	TestEqual(TEXT("si parte senza turni"), Draft.NumTurns(), 0);

	int32 Turn = INDEX_NONE;
	TestEqual(TEXT("il turno si aggiunge"),
		Draft.AddTurn(Turn, Error), ERTScenarioAuthoringResult::Success);
	TestEqual(TEXT("il primo turno ha indice 0"), Turn, 0);
	TestEqual(TEXT("ora c'e' un turno"), Draft.NumTurns(), 1);

	// --- Move: il percorso finisce in `FRTScenarioIntent::Move`, il campo che il formato ha gia'.
	const TArray<FRTCellId> Path = { FRTCellId(-1, 0, 0), FRTCellId(0, 0, 0) };
	if (!TestEqual(TEXT("A1 riceve un Move"),
		Draft.SetMoveIntent(Turn, TEXT("A1"), Path, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}

	const FRTScenarioTurn& Written = Draft.GetScenario().Turns[0];
	if (TestEqual(TEXT("un intent scritto"), Written.Intents.Num(), 1))
	{
		TestEqual(TEXT("nomina A1"), Written.Intents[0].UnitId, TEXT("A1"));
		TestEqual(TEXT("il percorso e' nel dato canonico"), Written.Intents[0].Move.Num(), 2);
		TestEqual(TEXT("prima cella"), Written.Intents[0].Move[0], FRTCellId(-1, 0, 0));
		TestEqual(TEXT("ultima cella"), Written.Intents[0].Move[1], FRTCellId(0, 0, 0));
	}

	// --- Wait: l'unita' e' nel turno e non fa nulla. NON e' `ability: "Action.Wait"`.
	if (!TestEqual(TEXT("B1 riceve un Wait"),
		Draft.SetWaitIntent(Turn, TEXT("B1"), Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}
	const FRTScenarioIntent& Wait = Draft.GetScenario().Turns[0].Intents[1];
	TestEqual(TEXT("il Wait nomina B1"), Wait.UnitId, TEXT("B1"));
	TestEqual(TEXT("e non dichiara movimento"), Wait.Move.Num(), 0);
	TestTrue(TEXT("ne' un'abilita'"), Wait.Ability.IsNone());
	TestTrue(TEXT("ne' un bersaglio"), Wait.Target.IsEmpty());

	// --- Sostituzione: la stessa unita' non accumula due piani nello stesso turno.
	TestEqual(TEXT("A1 cambia idea e attende"),
		Draft.SetWaitIntent(Turn, TEXT("A1"), Error), ERTScenarioAuthoringResult::Success);
	TestEqual(TEXT("gli intent restano due, non tre"), Draft.GetScenario().Turns[0].Intents.Num(), 2);
	TestEqual(TEXT("e quello di A1 non ha piu' movimento"),
		Draft.GetScenario().Turns[0].Intents[0].Move.Num(), 0);

	// --- Rimozione.
	TestEqual(TEXT("l'intent di A1 si toglie"),
		Draft.RemoveIntent(Turn, TEXT("A1"), Error), ERTScenarioAuthoringResult::Success);
	TestEqual(TEXT("ne resta uno"), Draft.GetScenario().Turns[0].Intents.Num(), 1);
	TestEqual(TEXT("togliere due volte -> NotFound"),
		Draft.RemoveIntent(Turn, TEXT("A1"), Error), ERTScenarioAuthoringResult::NotFound);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioTurnAuthoringRefusalsTest,
	"RefactorTactics.Scenario.TurnAuthoringNamesWhatItRefuses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioTurnAuthoringRefusalsTest::RunTest(const FString&)
{
	FRTScenarioDraft Draft;
	FString Error;
	if (!TestTrue(TEXT("scenario di partenza caricato"), OpenTurnDraft(Draft, Error)))
	{
		AddError(Error);
		return false;
	}

	const TArray<FRTCellId> Path = { FRTCellId(-1, 0, 0) };

	// Turno inesistente: non c'e' ancora nessun turno.
	TestEqual(TEXT("Move su un turno inesistente -> NotFound"),
		Draft.SetMoveIntent(0, TEXT("A1"), Path, Error), ERTScenarioAuthoringResult::NotFound);
	TestTrue(*FString::Printf(TEXT("e nomina il turno (era: %s)"), *Error), Error.Contains(TEXT("turno")));

	int32 Turn = INDEX_NONE;
	Draft.AddTurn(Turn, Error);

	// Unita' non schierata.
	TestEqual(TEXT("Move per un'unita' inesistente -> NotFound"),
		Draft.SetMoveIntent(Turn, TEXT("Fantasma"), Path, Error), ERTScenarioAuthoringResult::NotFound);
	TestTrue(*FString::Printf(TEXT("e la nomina (era: %s)"), *Error), Error.Contains(TEXT("Fantasma")));
	TestEqual(TEXT("Wait per un'unita' inesistente -> NotFound"),
		Draft.SetWaitIntent(Turn, TEXT("Fantasma"), Error), ERTScenarioAuthoringResult::NotFound);

	// Un Move senza celle non e' un'attesa: e' un piano che non dice dove.
	TestEqual(TEXT("Move senza celle -> Invalid"),
		Draft.SetMoveIntent(Turn, TEXT("A1"), TArray<FRTCellId>(), Error), ERTScenarioAuthoringResult::Invalid);
	TestTrue(*FString::Printf(TEXT("e suggerisce Wait (era: %s)"), *Error), Error.Contains(TEXT("Wait")));

	// Nessun rifiuto ha lasciato traccia.
	TestEqual(TEXT("il turno e' rimasto vuoto"), Draft.GetScenario().Turns[0].Intents.Num(), 0);

	// Le expectation: unita' non schierata, e conteggio negativo.
	TestEqual(TEXT("UnitAtCell su un'unita' inesistente -> NotFound"),
		Draft.AddExpectationUnitAtCell(TEXT("Fantasma"), FRTCellId(0, 0, 0), Error),
		ERTScenarioAuthoringResult::NotFound);
	TestEqual(TEXT("LogEventCount negativo -> Invalid"),
		Draft.AddExpectationLogEventCount(ERTLogCategory::Move, 0, -1, Error),
		ERTScenarioAuthoringResult::Invalid);

	// Un draft chiuso risponde `NoScenarioOpen` a tutte, invece di fingere.
	FRTScenarioDraft Empty;
	int32 Ignored = INDEX_NONE;
	TestEqual(TEXT("AddTurn su draft chiuso"),
		Empty.AddTurn(Ignored, Error), ERTScenarioAuthoringResult::NoScenarioOpen);
	TestEqual(TEXT("SetMoveIntent su draft chiuso"),
		Empty.SetMoveIntent(0, TEXT("A1"), Path, Error), ERTScenarioAuthoringResult::NoScenarioOpen);
	TestEqual(TEXT("SetWaitIntent su draft chiuso"),
		Empty.SetWaitIntent(0, TEXT("A1"), Error), ERTScenarioAuthoringResult::NoScenarioOpen);
	TestEqual(TEXT("RemoveIntent su draft chiuso"),
		Empty.RemoveIntent(0, TEXT("A1"), Error), ERTScenarioAuthoringResult::NoScenarioOpen);
	TestEqual(TEXT("AddExpectationUnitAtCell su draft chiuso"),
		Empty.AddExpectationUnitAtCell(TEXT("A1"), FRTCellId(0, 0, 0), Error),
		ERTScenarioAuthoringResult::NoScenarioOpen);
	TestEqual(TEXT("AddExpectationLogEventCount su draft chiuso"),
		Empty.AddExpectationLogEventCount(ERTLogCategory::Move, 0, 1, Error),
		ERTScenarioAuthoringResult::NoScenarioOpen);
	TestEqual(TEXT("RemoveExpectation su draft chiuso"),
		Empty.RemoveExpectation(0, Error), ERTScenarioAuthoringResult::NoScenarioOpen);

	return true;
}

// --- la preview NON e' un secondo pathfinder ------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioPreviewComesFromTheRuntimeTest,
	"RefactorTactics.Scenario.ReachabilityPreviewComesFromTheRuntimeService",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioPreviewComesFromTheRuntimeTest::RunTest(const FString&)
{
	// ⚠️ **Il test che #1116 chiede davvero.** L'issue vuole che la preview passi dal servizio runtime, e lo
	// vuole *verificabile*. Qui la verifica e' un confronto: la stessa domanda, posta due volte — una volta
	// dall'authoring, una volta a `URTHexSimLibrary::ReachableCells` direttamente — deve dare la STESSA
	// risposta. Un pathfinder scritto nell'editor divergerebbe al primo caso che il servizio tratta e la copia
	// no, e questo test diventerebbe rosso invece di lasciarlo scoprire in partita.
	FRTScenarioDraft Draft;
	FString Error;
	if (!TestTrue(TEXT("scenario di partenza caricato"), OpenTurnDraft(Draft, Error)))
	{
		AddError(Error);
		return false;
	}

	// Un ostacolo in mezzo, cosi' la risposta non e' banalmente «un disco di raggio N»: se la preview lo
	// ignorasse, offrirebbe celle che il resolver non concede.
	FRTScenarioCell Blocker;
	Blocker.Cell = FRTCellId(-1, 0, 0);
	Blocker.bBlocksMovement = true;
	Draft.MutableScenario().Cells.Add(Blocker);

	const TArray<FRTCellId> FromAuthoring = Draft.GetReachableCells(TEXT("A1"), GetTransientPackage(), Error);
	if (!TestTrue(*FString::Printf(TEXT("la preview risponde (errore: %s)"), *Error), Error.IsEmpty()))
	{
		return false;
	}
	TestTrue(TEXT("e non e' vuota"), FromAuthoring.Num() > 0);

	// La stessa domanda, posta al servizio runtime a mano.
	URTHexMapAsset* Map = URTScenarioArenaLibrary::BuildArena(Draft.GetScenario(), GetTransientPackage());
	if (!TestNotNull(TEXT("l'arena si costruisce"), Map))
	{
		return false;
	}

	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
	TArray<FRTHexSimUnit> SimUnits;
	for (const FRTScenarioUnit& Unit : Draft.GetScenario().Units)
	{
		FRTHexSimUnit Sim;
		Sim.UnitId = SimUnits.Num();
		Sim.Cell = Unit.Cell;
		Sim.bAlive = true;
		Sim.Facing = Unit.Facing;
		URTHeroData* const* Found = Roster.FindByPredicate(
			[&Unit](const URTHeroData* H) { return H && H->HeroId == Unit.HeroId; });
		Sim.MoveBudget = (Found && *Found) ? (*Found)->MovePoints : 0;
		SimUnits.Add(Sim);
	}
	const FRTHexSnapshot Snapshot = URTHexSimLibrary::MakeSnapshot(Map, SimUnits);
	const TArray<FRTHexReachableCell> FromRuntime = URTHexSimLibrary::ReachableCells(Snapshot, /*UnitId=*/ 0);

	if (!TestEqual(TEXT("preview e servizio runtime danno lo stesso numero di celle"),
		FromAuthoring.Num(), FromRuntime.Num()))
	{
		AddError(TEXT("la preview ha smesso di essere una domanda al runtime: se qualcuno ha scritto un ")
			TEXT("pathfinder nell'editor, e' questo il test che lo dice"));
		return false;
	}
	for (int32 I = 0; I < FromRuntime.Num(); ++I)
	{
		TestEqual(*FString::Printf(TEXT("cella %d identica"), I), FromAuthoring[I], FromRuntime[I].Cell);
	}

	// L'ostacolo NON e' fra le celle offerte: e' la prova che la preview vede la mappa vera, non un disco.
	TestFalse(TEXT("la cella che blocca il movimento non e' raggiungibile"),
		FromAuthoring.Contains(FRTCellId(-1, 0, 0)));

	// E nemmeno la cella occupata da B1: le altre unita' contano.
	TestFalse(TEXT("la cella di un'altra unita' non e' offerta"),
		FromAuthoring.Contains(FRTCellId(2, 0, 0)));

	// Rifiuti della preview.
	Draft.GetReachableCells(TEXT("Fantasma"), GetTransientPackage(), Error);
	TestFalse(TEXT("una unita' inesistente produce un errore"), Error.IsEmpty());

	FRTScenarioDraft Empty;
	Empty.GetReachableCells(TEXT("A1"), GetTransientPackage(), Error);
	TestFalse(TEXT("un draft chiuso produce un errore"), Error.IsEmpty());

	return true;
}

// --- il giro completo, e la verifica di mutazione -------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioTurnAuthoringRoundTripAndMutationTest,
	"RefactorTactics.Scenario.TurnAuthoringSurvivesSaveAndDetectsMutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioTurnAuthoringRoundTripAndMutationTest::RunTest(const FString&)
{
	const FString Dir = TurnAuthoringTempDir();
	const FString Path = FPaths::Combine(Dir, TEXT("Authored.json"));
	ON_SCOPE_EXIT{ IFileManager::Get().DeleteDirectory(*Dir, false, true); };

	FRTScenarioDraft Draft;
	FString Error;
	if (!TestTrue(TEXT("scenario di partenza caricato"), OpenTurnDraft(Draft, Error)))
	{
		AddError(Error);
		return false;
	}

	// Uno scenario costruito interamente dall'authoring: turno, Move, Wait, expectation.
	int32 Turn = INDEX_NONE;
	if (!TestEqual(TEXT("turno aggiunto"), Draft.AddTurn(Turn, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}
	const TArray<FRTCellId> Path1 = { FRTCellId(-1, 0, 0) };
	TestEqual(TEXT("Move scritto"),
		Draft.SetMoveIntent(Turn, TEXT("A1"), Path1, Error), ERTScenarioAuthoringResult::Success);
	TestEqual(TEXT("Wait scritto"),
		Draft.SetWaitIntent(Turn, TEXT("B1"), Error), ERTScenarioAuthoringResult::Success);
	TestEqual(TEXT("expectation scritta"),
		Draft.AddExpectationUnitAtCell(TEXT("A1"), FRTCellId(-1, 0, 0), Error),
		ERTScenarioAuthoringResult::Success);

	// `Validate` deve passare sullo scenario prodotto: e' un criterio esplicito di #1116, e si asserisce
	// invece di osservarlo a mano.
	if (!TestEqual(TEXT("lo scenario costruito e' valido"),
		Draft.Validate(Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}

	if (!TestEqual(TEXT("salvato"), Draft.SaveToFile(Path, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}

	// Save/reload preserva Turn, Intent ed Expectation — riletti da disco, non dalla memoria.
	FRTScenarioDraft Reloaded;
	if (!TestEqual(TEXT("riaperto"), Reloaded.OpenFromFile(Path, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}
	TestEqual(TEXT("un turno"), Reloaded.NumTurns(), 1);
	if (TestEqual(TEXT("due intent"), Reloaded.GetScenario().Turns[0].Intents.Num(), 2))
	{
		TestEqual(TEXT("il Move di A1 sopravvive"),
			Reloaded.GetScenario().Turns[0].Intents[0].Move[0], FRTCellId(-1, 0, 0));
		TestEqual(TEXT("il Wait di B1 sopravvive senza movimento"),
			Reloaded.GetScenario().Turns[0].Intents[1].Move.Num(), 0);
	}
	TestEqual(TEXT("due assertion"), Reloaded.GetScenario().Expect.Num(), 2);

	// ⚠️ **Verifica di mutazione, richiesta da #1116**: *«cambiare la cella di destinazione fa fallire
	// l'expectation UnitAtCell quando lo scenario viene eseguito»*. Non basta che il dato cambi: deve cambiare
	// l'ESITO, e l'unico modo di saperlo e' farlo girare dal percorso reale.
	// 🔴 **Un mondo PER RUN.** La prima stesura ne usava uno solo per entrambe le esecuzioni, e la review di
	// `#1116` ha mostrato perche' non vale: `URTScenarioRunner::Run` non smonta il mondo, quindi le unita' e
	// il turn manager del primo run sopravvivono. Il secondo run ne avrebbe trovate quattro invece di due, e
	// l'esito sarebbe cambiato **per il residuo, non per la mutazione** — l'assertion sarebbe passata per la
	// ragione sbagliata, cioe' il verde che non prova niente. Tutti gli altri test del runner costruiscono un
	// mondo per esecuzione, e questo era l'unico a non farlo.
	UWorld* WorldA = MakeTurnAuthoringWorld();
	if (!TestNotNull(TEXT("mondo per la prima esecuzione"), WorldA))
	{
		return false;
	}
	const FRTTestResult Passing = URTScenarioRunner::Run(WorldA, Reloaded.GetScenario());
	DestroyTurnAuthoringWorld(WorldA);
	if (!TestEqual(TEXT("lo scenario costruito dall'authoring PASSA"),
		static_cast<int32>(Passing.Outcome), static_cast<int32>(ERTTestOutcome::Pass)))
	{
		AddError(FString::Printf(TEXT("esito inatteso: %s"), *Passing.ErrorMessage));
		return false;
	}

	FRTScenarioDraft Mutated;
	if (!TestEqual(TEXT("riaperto per la mutazione"),
		Mutated.OpenFromFile(Path, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}
	// Si sposta la destinazione del Move e si lascia l'expectation dov'era: l'unita' finira' altrove.
	const TArray<FRTCellId> Elsewhere = { FRTCellId(-2, 1, 0) };
	TestEqual(TEXT("destinazione mutata"),
		Mutated.SetMoveIntent(0, TEXT("A1"), Elsewhere, Error), ERTScenarioAuthoringResult::Success);

	UWorld* WorldB = MakeTurnAuthoringWorld();
	if (!TestNotNull(TEXT("mondo per la seconda esecuzione"), WorldB))
	{
		return false;
	}
	const FRTTestResult AfterMutation = URTScenarioRunner::Run(WorldB, Mutated.GetScenario());
	DestroyTurnAuthoringWorld(WorldB);

	// I due esiti devono DIFFERIRE. Se restassero uguali, l'expectation non starebbe misurando la
	// destinazione — cioe' lo scenario passerebbe comunque, ed e' il verde che non prova niente.
	TestNotEqual(TEXT("mutare la destinazione cambia l'esito dello scenario"),
		static_cast<int32>(AfterMutation.Outcome), static_cast<int32>(Passing.Outcome));

	return true;
}

// --- l'authoring dei turni e' raggiungibile da Blueprint ------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioTurnAuthoringIsExposedTest,
	"RefactorTactics.Scenario.TurnAuthoringIsReachableFromBlueprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioTurnAuthoringIsExposedTest::RunTest(const FString&)
{
	UClass* Facade = URTScenarioAuthoring::StaticClass();
	if (!TestNotNull(TEXT("la facade esiste"), Facade)) { return false; }

	// ⚠️ Gli slot di combattimento di `#1626` stanno in questa lista e non in una nuova: una funzione
	// esposta senza guardia e' esattamente il buco su cui qualcuno costruisce un widget, e la ragione per
	// cui questo elenco esiste vale per le sette nuove quanto per le nove vecchie.
	for (const FName& Name : { FName(TEXT("AddTurn")), FName(TEXT("GetTurnCount")), FName(TEXT("SetMoveIntent")),
		FName(TEXT("SetWaitIntent")), FName(TEXT("RemoveIntent")), FName(TEXT("AddExpectationUnitAtCell")),
		FName(TEXT("AddExpectationLogEventCount")), FName(TEXT("RemoveExpectation")),
		FName(TEXT("GetReachableCells")), FName(TEXT("DuplicateTurn")),
		FName(TEXT("SetDashIntent")), FName(TEXT("SetMainAction")), FName(TEXT("SetMainActionOnUnit")),
		FName(TEXT("SetMainActionOnCell")), FName(TEXT("SetFacingIntent")), FName(TEXT("SetCoverEdgeIntent")),
		FName(TEXT("SetReactionIntent")) })
	{
		const UFunction* Function = Facade->FindFunctionByName(Name);
		if (!TestNotNull(*FString::Printf(TEXT("'%s' e' una UFUNCTION"), *Name.ToString()), Function))
		{
			continue;
		}
		TestTrue(*FString::Printf(TEXT("'%s' e' chiamabile da Blueprint"), *Name.ToString()),
			Function->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_BlueprintPure));
	}

	// `ERTLogCategory` attraversa il confine come parametro di `AddExpectationLogEventCount`: senza
	// `BlueprintType` quella funzione non sarebbe cablabile a nulla.
	const UEnum* LogCategory = StaticEnum<ERTLogCategory>();
	if (TestNotNull(TEXT("ERTLogCategory esiste"), LogCategory))
	{
		// Stessa ragione di sopra: `GetBoolMetaData` e' sotto `WITH_METADATA`, spento in un target Game.
#if WITH_METADATA
		TestTrue(TEXT("ed e' BlueprintType"), LogCategory->GetBoolMetaData(TEXT("BlueprintType")));
#endif
	}

	// Un giro completo attraverso la facade: e' il percorso che fara' l'Editor.
	URTScenarioAuthoring* Authoring = URTScenarioAuthoring::CreateScenarioDraft(nullptr);
	if (!TestNotNull(TEXT("draft creato"), Authoring)) { return false; }

	FString Error;
	FRTTestScenario Loaded;
	if (!TestTrue(TEXT("base caricata"),
		URTScenarioLoader::LoadFromString(TurnAuthoringBaseJson, Loaded, Error)))
	{
		AddError(Error);
		return false;
	}
	Authoring->NewScenario(TEXT("segnaposto"), 3);
	Authoring->GetDraft().MutableScenario() = Loaded;

	int32 FacadeTurn = INDEX_NONE;
	TestEqual(TEXT("la facade aggiunge un turno"),
		Authoring->AddTurn(FacadeTurn, Error), ERTScenarioAuthoringResult::Success);
	TestEqual(TEXT("con indice 0"), FacadeTurn, 0);
	TestEqual(TEXT("e lo conta"), Authoring->GetTurnCount(), 1);
	TestEqual(TEXT("la facade scrive un Move"),
		Authoring->SetMoveIntent(0, TEXT("A1"), { FRTCellId(-1, 0, 0) }, Error),
		ERTScenarioAuthoringResult::Success);
	TestEqual(TEXT("la facade scrive un Wait"),
		Authoring->SetWaitIntent(0, TEXT("B1"), Error), ERTScenarioAuthoringResult::Success);
	TestEqual(TEXT("la facade scrive una expectation"),
		Authoring->AddExpectationUnitAtCell(TEXT("A1"), FRTCellId(-1, 0, 0), Error),
		ERTScenarioAuthoringResult::Success);

	// La preview attraverso la facade non ha bisogno di un Outer esplicito: se lo prende da se'.
	const TArray<FRTCellId> Reachable = Authoring->GetReachableCells(TEXT("A1"), Error);
	TestTrue(*FString::Printf(TEXT("la preview risponde dalla facade (errore: %s)"), *Error), Error.IsEmpty());
	TestTrue(TEXT("e offre almeno una cella"), Reachable.Num() > 0);

	return true;
}

// --- i difetti che la review ha trovato -----------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioIntentPreservesOtherFieldsTest,
	"RefactorTactics.Scenario.SettingAMoveDoesNotEraseTheRestOfTheIntent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioIntentPreservesOtherFieldsTest::RunTest(const FString&)
{
	// 🔴 La prima stesura costruiva un `FRTScenarioIntent` NUOVO e lo assegnava sopra quello esistente:
	// abilita', bersaglio, reazione e condizione sparivano in silenzio per un click che voleva solo cambiare
	// il percorso. E' la stessa cancellazione muta del lavoro altrui che `RemoveUnit` si rifiuta di fare.
	FRTScenarioDraft Draft;
	FString Error;
	if (!TestTrue(TEXT("scenario di partenza caricato"), OpenTurnDraft(Draft, Error)))
	{
		AddError(Error);
		return false;
	}

	int32 Turn = INDEX_NONE;
	Draft.AddTurn(Turn, Error);

	// Un intent ricco, come ne esistono negli scenari scritti a mano.
	FRTScenarioIntent Rich;
	Rich.UnitId = TEXT("A1");
	Rich.Ability = FName(TEXT("Hero.Gadget.ArcPulse"));
	Rich.Target = TEXT("B1");
	Rich.Reaction = FName(TEXT("Action.Overwatch"));
	Rich.Facing = ERTHexDirection::NW;
	Rich.bDeclaresFacing = true;
	Draft.MutableScenario().Turns[0].Intents.Add(Rich);

	// Cambiare il percorso NON deve toccare il resto.
	const TArray<FRTCellId> Path = { FRTCellId(-1, 0, 0) };
	TestEqual(TEXT("il Move si scrive"),
		Draft.SetMoveIntent(Turn, TEXT("A1"), Path, Error), ERTScenarioAuthoringResult::Success);

	const FRTScenarioIntent& After = Draft.GetScenario().Turns[0].Intents[0];
	TestEqual(TEXT("il nuovo percorso c'e'"), After.Move.Num(), 1);
	TestEqual(TEXT("l'abilita' e' sopravvissuta"), After.Ability, FName(TEXT("Hero.Gadget.ArcPulse")));
	TestEqual(TEXT("il bersaglio e' sopravvissuto"), After.Target, TEXT("B1"));
	TestEqual(TEXT("la reazione e' sopravvissuta"), After.Reaction, FName(TEXT("Action.Overwatch")));
	TestTrue(TEXT("la rotazione dichiarata e' sopravvissuta"), After.bDeclaresFacing);
	TestEqual(TEXT("e la sua direzione"), After.Facing, ERTHexDirection::NW);

	// `Wait` invece AZZERA — e' il suo significato — ma non in silenzio: deve dire cosa ha tolto.
	TestEqual(TEXT("Wait si scrive"),
		Draft.SetWaitIntent(Turn, TEXT("A1"), Error), ERTScenarioAuthoringResult::Success);
	const FRTScenarioIntent& Waiting = Draft.GetScenario().Turns[0].Intents[0];
	TestTrue(TEXT("l'abilita' e' stata tolta"), Waiting.Ability.IsNone());
	TestTrue(TEXT("e il movimento pure"), Waiting.Move.Num() == 0);
	TestFalse(TEXT("ma l'operazione non e' stata muta"), Error.IsEmpty());
	TestTrue(*FString::Printf(TEXT("e nomina cio' che ha tolto (era: %s)"), *Error),
		Error.Contains(TEXT("ArcPulse")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLogEventOutcomeIsValidatedTest,
	"RefactorTactics.Scenario.AuthoringRefusesALogOutcomeItCannotWrite",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLogEventOutcomeIsValidatedTest::RunTest(const FString&)
{
	// 🔴 `Outcome` arriva come `uint8` nudo — in Blueprint e' un pin Byte senza tendina — e senza controllo un
	// valore fuori scala passava `Validate`, veniva serializzato come `"outcome": ""` e il file non si
	// rileggeva piu': **lo strumento scriveva uno scenario che non sapeva riaprire.**
	FRTScenarioDraft Draft;
	FString Error;
	if (!TestTrue(TEXT("scenario di partenza caricato"), OpenTurnDraft(Draft, Error)))
	{
		AddError(Error);
		return false;
	}

	TestEqual(TEXT("un esito fuori scala e' rifiutato"),
		Draft.AddExpectationLogEventCount(ERTLogCategory::Move, 200, 1, Error),
		ERTScenarioAuthoringResult::Invalid);
	TestTrue(*FString::Printf(TEXT("e l'errore elenca quelli previsti (era: %s)"), *Error),
		Error.Contains(TEXT("previsti")));

	// Un esito legale invece passa, e il file che ne esce si rilegge.
	const int32 Before = Draft.GetScenario().Expect.Num();
	if (!TestEqual(TEXT("un esito legale e' accettato"),
		Draft.AddExpectationLogEventCount(ERTLogCategory::Move, 0, 1, Error),
		ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}
	TestEqual(TEXT("l'assertion e' stata aggiunta"), Draft.GetScenario().Expect.Num(), Before + 1);

	// Il giro completo: scrivere e rileggere. E' la prova che il rifiuto sopra evitava un file rotto.
	FString Json;
	if (TestTrue(TEXT("lo scenario si serializza"),
		URTScenarioLoader::SaveToString(Draft.GetScenario(), Json, Error)))
	{
		FRTTestScenario ReloadedFromJson;
		TestTrue(TEXT("e si rilegge"),
			URTScenarioLoader::LoadFromString(Json, ReloadedFromJson, Error));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioTurnAuthoringListsAndRemovesTest,
	"RefactorTactics.Scenario.TurnAuthoringCanShowAndUndoWhatItWrote",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioTurnAuthoringListsAndRemovesTest::RunTest(const FString&)
{
	// Un editor che scrive e non mostra non e' un editor: `RemoveIntent` e `RemoveExpectation(indice)`
	// chiedevano di nominare qualcosa che nessuna API sapeva elencare.
	FRTScenarioDraft Draft;
	FString Error;
	if (!TestTrue(TEXT("scenario di partenza caricato"), OpenTurnDraft(Draft, Error)))
	{
		AddError(Error);
		return false;
	}

	int32 Turn = INDEX_NONE;
	Draft.AddTurn(Turn, Error);
	Draft.SetMoveIntent(Turn, TEXT("A1"), { FRTCellId(-1, 0, 0) }, Error);
	Draft.SetWaitIntent(Turn, TEXT("B1"), Error);

	const TArray<FRTScenarioIntentView> Intents = Draft.ListIntents(Turn);
	if (TestEqual(TEXT("due intent elencati"), Intents.Num(), 2))
	{
		TestEqual(TEXT("il primo e' A1"), Intents[0].UnitId, TEXT("A1"));
		TestTrue(TEXT("e dichiara un movimento"), Intents[0].bHasMove);
		TestTrue(*FString::Printf(TEXT("con una riga leggibile (era: %s)"), *Intents[0].Summary),
			Intents[0].Summary.Contains(TEXT("Move")));
		TestFalse(TEXT("il secondo non si muove"), Intents[1].bHasMove);
		TestEqual(TEXT("ed e' descritto come Wait"), Intents[1].Summary, TEXT("Wait"));
	}
	TestEqual(TEXT("un turno inesistente non elenca nulla"), Draft.ListIntents(99).Num(), 0);

	Draft.AddExpectationUnitAtCell(TEXT("A1"), FRTCellId(-1, 0, 0), Error);
	const TArray<FRTScenarioExpectationView> Expectations = Draft.ListExpectations();
	if (TestEqual(TEXT("due assertion elencate"), Expectations.Num(), 2))
	{
		// L'indice riportato e' quello che `RemoveExpectation` accetta: e' il contratto fra le due.
		TestEqual(TEXT("indici progressivi"), Expectations[1].Index, 1);
		TestEqual(TEXT("il tipo e' quello del JSON"), Expectations[1].Type, TEXT("UnitAtCell"));
		TestEqual(TEXT("e nomina l'unita'"), Expectations[1].UnitId, TEXT("A1"));

		TestEqual(TEXT("si toglie per indice"),
			Draft.RemoveExpectation(Expectations[1].Index, Error), ERTScenarioAuthoringResult::Success);
		TestEqual(TEXT("ne resta una"), Draft.ListExpectations().Num(), 1);
	}

	// Un turno aggiunto per sbaglio ora si toglie: senza, `Validate` lo accettava e il runner lo giocava.
	int32 Extra = INDEX_NONE;
	Draft.AddTurn(Extra, Error);
	TestEqual(TEXT("due turni"), Draft.NumTurns(), 2);
	TestEqual(TEXT("il turno di troppo si toglie"),
		Draft.RemoveTurn(Extra, Error), ERTScenarioAuthoringResult::Success);
	TestEqual(TEXT("ne resta uno"), Draft.NumTurns(), 1);
	TestEqual(TEXT("togliere un turno inesistente -> NotFound"),
		Draft.RemoveTurn(99, Error), ERTScenarioAuthoringResult::NotFound);

	// E un intent per una unita' del bot viene rifiutato QUI, non al salvataggio.
	Draft.MutableScenario().Units[1].bBotControlled = true;
	TestEqual(TEXT("Move su unita' bot -> Invalid"),
		Draft.SetMoveIntent(0, TEXT("B1"), { FRTCellId(1, 0, 0) }, Error),
		ERTScenarioAuthoringResult::Invalid);
	TestTrue(*FString::Printf(TEXT("e spiega perche' (era: %s)"), *Error), Error.Contains(TEXT("bot")));

	return true;
}

/**
 * IL DESIGNER AUTHORA MOVIMENTO E AZIONE INSIEME, E IL FILE LI PORTA ENTRAMBI — `#1626`.
 *
 * 🔑 **È il primo criterio della issue, misurato dove è misurabile.** Il criterio dice *«senza toccare
 * JSON»*: il senso operativo è che i campi si riempiono chiamando la facade, non scrivendo testo. Questo
 * test authora, salva su disco e **rilegge dal file** — se un campo non passasse dalla facade, o non
 * sopravvivesse al giro, cadrebbe qui.
 *
 * ⚠️ Cosa NON prova: che un umano ci riesca cliccando nel Composer. Quella metà è `U26`, classe C, e
 * nessun test headless la può sostituire. Ma prima di `#1626` la facade authorava **Move e Wait** — con
 * quelle sole due funzioni nessun widget avrebbe potuto esprimere un attacco, per quanto ben disegnato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioCombatIntentAuthoringTest,
	"RefactorTactics.Scenario.CombatIntentAuthoringFillsBothSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioCombatIntentAuthoringTest::RunTest(const FString&)
{
	const FString Dir = TurnAuthoringTempDir();
	const FString Path = FPaths::Combine(Dir, TEXT("Combat.json"));
	ON_SCOPE_EXIT{ IFileManager::Get().DeleteDirectory(*Dir, false, true); };

	FRTScenarioDraft Draft;
	FString Error;
	if (!TestTrue(TEXT("scenario di partenza caricato"), OpenTurnDraft(Draft, Error)))
	{
		AddError(Error);
		return false;
	}

	int32 Turn = INDEX_NONE;
	if (!TestEqual(TEXT("turno aggiunto"), Draft.AddTurn(Turn, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}

	// 🎯 I DUE SLOT INSIEME, che il doc header chiama «la norma, non un caso limite».
	const TArray<FRTCellId> Path1 = { FRTCellId(-1, 0, 0) };
	TestEqual(TEXT("slot movimento"),
		Draft.SetMoveIntent(Turn, TEXT("A1"), Path1, Error), ERTScenarioAuthoringResult::Success);
	TestEqual(TEXT("slot principale, sulla stessa unita' e nello stesso turno"),
		Draft.SetMainActionOnUnit(Turn, TEXT("A1"), TEXT("Action.BasicAttack"), TEXT("B1"), Error),
		ERTScenarioAuthoringResult::Success);
	TestEqual(TEXT("modificatore: rotazione"),
		Draft.SetFacingIntent(Turn, TEXT("A1"), true, ERTHexDirection::NE, Error),
		ERTScenarioAuthoringResult::Success);
	TestEqual(TEXT("l'avversario si mette in guardia"),
		Draft.SetMainAction(Turn, TEXT("B1"), TEXT("Action.Guard"), Error),
		ERTScenarioAuthoringResult::Success);

	// La lista che la UI mostra deve dire CHI viene attaccato: una facade che scrive un bersaglio e non
	// sa mostrarlo e' il difetto che la review di `#1116` ha trovato su `RemoveIntent`.
	const TArray<FRTScenarioIntentView> Views = Draft.ListIntents(Turn);
	if (TestEqual(TEXT("due intent in lista"), Views.Num(), 2))
	{
		TestEqual(TEXT("la vista porta il bersaglio"), Views[0].Target, FString(TEXT("B1")));
		TestTrue(TEXT("e la riga lo nomina"), Views[0].Summary.Contains(TEXT("B1")));
		TestTrue(TEXT("la riga nomina anche il movimento"), Views[0].Summary.Contains(TEXT("Move")));
	}

	if (!TestEqual(TEXT("lo scenario costruito e' valido"),
		Draft.Validate(Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}
	if (!TestEqual(TEXT("salvato"), Draft.SaveToFile(Path, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}

	// ✅ Riletto DAL FILE: e' il file prodotto che il criterio giudica, non lo stato in memoria.
	FRTScenarioDraft Reloaded;
	if (!TestEqual(TEXT("riaperto"), Reloaded.OpenFromFile(Path, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}
	if (TestEqual(TEXT("due intent nel file"), Reloaded.GetScenario().Turns[0].Intents.Num(), 2))
	{
		const FRTScenarioIntent& A = Reloaded.GetScenario().Turns[0].Intents[0];
		TestEqual(TEXT("lo slot movimento e' popolato"), A.Move.Num(), 1);
		TestEqual(TEXT("e anche quello principale"), A.Ability, FName(TEXT("Action.BasicAttack")));
		TestEqual(TEXT("con il suo bersaglio"), A.Target, FString(TEXT("B1")));
		TestTrue(TEXT("e il modificatore sopravvive"), A.bDeclaresFacing);
		TestEqual(TEXT("nella direzione scelta"), A.Facing, ERTHexDirection::NE);

		const FRTScenarioIntent& B = Reloaded.GetScenario().Turns[0].Intents[1];
		TestEqual(TEXT("la Guardia non ha preteso un bersaglio"), B.Ability, FName(TEXT("Action.Guard")));
		TestTrue(TEXT("e non ne ha uno"), B.Target.IsEmpty());
	}
	return true;
}

/**
 * LE DUE FORME DI BERSAGLIO NON COESISTONO MAI NEL FILE PRODOTTO — `#1626`.
 *
 * 🔴 **È il criterio 3, e la trappola è nella forma stessa dell'API.** Gli slot si compongono — ciascun
 * setter scrive solo il proprio campo — ma i due bersagli *non sono due slot*: sono due forme dello stesso.
 * Senza la pulizia reciproca, un designer che sceglie una unità e poi cambia idea per una cella lascia
 * **entrambi** i campi pieni, e `Validate` gli dice che lo scenario è insalvabile per una sequenza di
 * click perfettamente legittima.
 *
 * ⚠️ Il test cambia idea in **entrambe le direzioni**: una pulizia scritta da un lato solo passerebbe un
 * test che prova una direzione sola, ed è l'errore che rende verde metà del lavoro.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioTargetFormsExclusiveTest,
	"RefactorTactics.Scenario.TargetFormsNeverCoexistInTheAuthoredFile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioTargetFormsExclusiveTest::RunTest(const FString&)
{
	FRTScenarioDraft Draft;
	FString Error;
	if (!TestTrue(TEXT("scenario di partenza caricato"), OpenTurnDraft(Draft, Error)))
	{
		AddError(Error);
		return false;
	}
	int32 Turn = INDEX_NONE;
	if (!TestEqual(TEXT("turno aggiunto"), Draft.AddTurn(Turn, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}

	auto Intent = [&Draft, Turn]() -> const FRTScenarioIntent&
	{
		return Draft.GetScenario().Turns[Turn].Intents[0];
	};

	// Unita' -> cella.
	TestEqual(TEXT("bersaglio per unita'"),
		Draft.SetMainActionOnUnit(Turn, TEXT("A1"), TEXT("Action.BasicAttack"), TEXT("B1"), Error),
		ERTScenarioAuthoringResult::Success);
	TestEqual(TEXT("bersaglio per cella dopo aver cambiato idea"),
		Draft.SetMainActionOnCell(Turn, TEXT("A1"), TEXT("Action.BasicAttack"), FRTCellId(1, 0, 0), Error),
		ERTScenarioAuthoringResult::Success);
	TestTrue(TEXT("la cella e' il bersaglio"), Intent().bTargetsCell);
	TestTrue(TEXT("e l'unita' non lo e' piu'"), Intent().Target.IsEmpty());
	if (!TestEqual(TEXT("e lo scenario resta salvabile"), Draft.Validate(Error),
		ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
	}

	// Cella -> unita', la direzione opposta.
	TestEqual(TEXT("di nuovo per unita'"),
		Draft.SetMainActionOnUnit(Turn, TEXT("A1"), TEXT("Action.BasicAttack"), TEXT("B1"), Error),
		ERTScenarioAuthoringResult::Success);
	TestFalse(TEXT("il flag della cella si e' spento"), Intent().bTargetsCell);
	TestEqual(TEXT("e la cella e' tornata al default"), Intent().TargetCell, FRTCellId());
	TestEqual(TEXT("l'unita' e' il bersaglio"), Intent().Target, FString(TEXT("B1")));
	if (!TestEqual(TEXT("lo scenario e' ancora salvabile"), Draft.Validate(Error),
		ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
	}

	// E l'azione senza bersaglio li toglie entrambi.
	TestEqual(TEXT("azione su se stessi"),
		Draft.SetMainAction(Turn, TEXT("A1"), TEXT("Action.Guard"), Error),
		ERTScenarioAuthoringResult::Success);
	TestTrue(TEXT("nessun bersaglio per unita'"), Intent().Target.IsEmpty());
	TestFalse(TEXT("nessun bersaglio per cella"), Intent().bTargetsCell);

	return true;
}

/**
 * OGNI SLOT SCRIVE SOLO IL PROPRIO, E SVUOTARNE UNO NON TOCCA GLI ALTRI — `#1626`.
 *
 * 🔑 È l'invariante che rende la UI a slot possibile. `SettingAMoveDoesNotEraseTheRestOfTheIntent` la prova
 * per il movimento; qui si prova per i quattro slot insieme, che è il caso che la UI produce davvero: il
 * designer riempie, cambia idea su uno, e gli altri tre devono essere ancora lì.
 *
 * ⚠️ Senza la seconda metà — svuotare uno e ricontrollare gli altri — il test resterebbe verde con setter
 * che ricostruiscono l'intero intent, perché riempire in sequenza funziona anche così.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioSlotsComposeTest,
	"RefactorTactics.Scenario.SlotsComposeAndClearOnlyThemselves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioSlotsComposeTest::RunTest(const FString&)
{
	FRTScenarioDraft Draft;
	FString Error;
	if (!TestTrue(TEXT("scenario di partenza caricato"), OpenTurnDraft(Draft, Error)))
	{
		AddError(Error);
		return false;
	}
	int32 Turn = INDEX_NONE;
	if (!TestEqual(TEXT("turno aggiunto"), Draft.AddTurn(Turn, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}
	auto Intent = [&Draft, Turn]() -> const FRTScenarioIntent&
	{
		return Draft.GetScenario().Turns[Turn].Intents[0];
	};

	const TArray<FRTCellId> Route = { FRTCellId(-1, 0, 0) };
	Draft.SetMoveIntent(Turn, TEXT("A1"), Route, Error);
	Draft.SetMainActionOnUnit(Turn, TEXT("A1"), TEXT("Action.BasicAttack"), TEXT("B1"), Error);
	Draft.SetFacingIntent(Turn, TEXT("A1"), true, ERTHexDirection::SE, Error);
	Draft.SetCoverEdgeIntent(Turn, TEXT("A1"), true, ERTHexDirection::NW, Error);

	TestEqual(TEXT("un solo intent per unita' per turno"),
		Draft.GetScenario().Turns[Turn].Intents.Num(), 1);
	TestEqual(TEXT("movimento"), Intent().Move.Num(), 1);
	TestEqual(TEXT("azione"), Intent().Ability, FName(TEXT("Action.BasicAttack")));
	TestTrue(TEXT("rotazione"), Intent().bDeclaresFacing);
	TestTrue(TEXT("bordo"), Intent().bHasCoverEdge);

	// ⛔ Ora se ne svuota UNO: gli altri tre devono essere intatti.
	TestEqual(TEXT("il bordo si toglie"),
		Draft.SetCoverEdgeIntent(Turn, TEXT("A1"), false, ERTHexDirection::E, Error),
		ERTScenarioAuthoringResult::Success);
	TestFalse(TEXT("il bordo e' andato"), Intent().bHasCoverEdge);
	TestEqual(TEXT("il movimento e' rimasto"), Intent().Move.Num(), 1);
	TestEqual(TEXT("l'azione e' rimasta"), Intent().Ability, FName(TEXT("Action.BasicAttack")));
	TestEqual(TEXT("con il suo bersaglio"), Intent().Target, FString(TEXT("B1")));
	TestTrue(TEXT("la rotazione e' rimasta"), Intent().bDeclaresFacing);

	// E svuotare lo slot principale non tocca il movimento.
	TestEqual(TEXT("l'azione si toglie"),
		Draft.SetMainAction(Turn, TEXT("A1"), NAME_None, Error), ERTScenarioAuthoringResult::Success);
	TestTrue(TEXT("l'azione e' andata"), Intent().Ability.IsNone());
	TestEqual(TEXT("il movimento e' ancora li'"), Intent().Move.Num(), 1);

	return true;
}

/**
 * LA CONDIZIONE SE NE VA CON LA REAZIONE CHE LA REGGE — `#1626`.
 *
 * 🔴 Una condizione senza reazione non è un campo di troppo: è precisamente ciò che
 * `ARTUnit::SetPlannedReactionCondition` rifiuta, e che il loader rifiuta a sua volta per non lasciarlo
 * passare in silenzio. Se lo slot reattivo si svuotasse lasciandola, il designer si troverebbe uno
 * scenario **insalvabile** dopo aver tolto una reazione — che è un'operazione che non dovrebbe rompere
 * nulla.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioReactionConditionTest,
	"RefactorTactics.Scenario.ReactionAndConditionLeaveTogether",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioReactionConditionTest::RunTest(const FString&)
{
	FRTTestScenario Loaded;
	FString Error;
	if (!TestTrue(TEXT("base con Wraith caricata"),
		URTScenarioLoader::LoadFromString(TurnAuthoringReactionJson, Loaded, Error)))
	{
		AddError(Error);
		return false;
	}
	FRTScenarioDraft Draft;
	Draft.NewScenario(TEXT("segnaposto"), 3);
	Draft.MutableScenario() = Loaded;

	int32 Turn = INDEX_NONE;
	if (!TestEqual(TEXT("turno aggiunto"), Draft.AddTurn(Turn, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}
	auto Intent = [&Draft, Turn]() -> const FRTScenarioIntent&
	{
		return Draft.GetScenario().Turns[Turn].Intents[0];
	};

	TestEqual(TEXT("reazione e condizione armate"),
		Draft.SetReactionIntent(Turn, TEXT("W1"), TEXT("Hero.Wraith.Deflection"),
			TEXT("TargetHealthAtOrBelowPercent"), 10, Error),
		ERTScenarioAuthoringResult::Success);
	TestEqual(TEXT("la reazione c'e'"), Intent().Reaction, FName(TEXT("Hero.Wraith.Deflection")));
	TestEqual(TEXT("la condizione anche"), Intent().Condition.Id, FName(TEXT("TargetHealthAtOrBelowPercent")));
	TestEqual(TEXT("con il suo parametro"), Intent().Condition.Param, 10);
	if (!TestEqual(TEXT("lo scenario e' salvabile"), Draft.Validate(Error),
		ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
	}

	// ⛔ Tolta la reazione, la condizione non puo' restare.
	TestEqual(TEXT("lo slot reattivo si svuota"),
		Draft.SetReactionIntent(Turn, TEXT("W1"), NAME_None, NAME_None, 0, Error),
		ERTScenarioAuthoringResult::Success);
	TestTrue(TEXT("la reazione e' andata"), Intent().Reaction.IsNone());
	TestFalse(TEXT("e la condizione con lei"), Intent().Condition.IsDeclared());
	if (!TestEqual(TEXT("e lo scenario resta salvabile"), Draft.Validate(Error),
		ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
	}

	// E chi prova a tenerla viene avvisato invece di scoprirlo al salvataggio.
	Draft.SetReactionIntent(Turn, TEXT("W1"), TEXT("Hero.Wraith.Deflection"),
		TEXT("TargetHealthAtOrBelowPercent"), 10, Error);
	Draft.SetReactionIntent(Turn, TEXT("W1"), NAME_None, TEXT("TargetHealthAtOrBelowPercent"), 10, Error);
	TestFalse(TEXT("la condizione non sopravvive alla reazione"), Intent().Condition.IsDeclared());
	TestTrue(FString::Printf(TEXT("e il messaggio lo dice (era: '%s')"), *Error),
		Error.Contains(TEXT("condizione")));

	return true;
}

/**
 * UNA CHIAMATA RIFIUTATA NON LASCIA TRACCE — `#1626`.
 *
 * 🔴 **Trovato in review sul mio stesso diff.** I setter degli slot ottengono l'intent da un helper che lo
 * **crea** se non c'è; la prima stesura chiamava l'helper e *poi* controllava gli argomenti. Una chiamata
 * rifiutata lasciava dietro di sé un intent che nomina l'unità e nient'altro — cioè, nel formato,
 * **un Wait**. Il designer avrebbe letto un errore e si sarebbe ritrovato un'attesa che non ha chiesto.
 *
 * ⚠️ Un editor che modifica in silenzio mentre dice «rifiutato» è peggio di uno che rifiuta e basta: chi
 * legge l'errore non ha ragione di andare a ricontrollare il piano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioRejectedIntentLeavesNoTraceTest,
	"RefactorTactics.Scenario.ARejectedIntentLeavesNoTrace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioRejectedIntentLeavesNoTraceTest::RunTest(const FString&)
{
	FRTScenarioDraft Draft;
	FString Error;
	if (!TestTrue(TEXT("scenario di partenza caricato"), OpenTurnDraft(Draft, Error)))
	{
		AddError(Error);
		return false;
	}
	int32 Turn = INDEX_NONE;
	if (!TestEqual(TEXT("turno aggiunto"), Draft.AddTurn(Turn, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}
	auto Intents = [&Draft, Turn]() { return Draft.GetScenario().Turns[Turn].Intents.Num(); };
	TestEqual(TEXT("il turno nasce vuoto"), Intents(), 0);

	// Bersaglio senza nome: rifiutata.
	TestEqual(TEXT("abilita' senza bersaglio nominato"),
		Draft.SetMainActionOnUnit(Turn, TEXT("A1"), TEXT("Action.BasicAttack"), FString(), Error),
		ERTScenarioAuthoringResult::Invalid);
	TestEqual(TEXT("e il turno e' ancora vuoto"), Intents(), 0);

	// Bersaglio senza abilita': rifiutata.
	TestEqual(TEXT("bersaglio senza abilita'"),
		Draft.SetMainActionOnUnit(Turn, TEXT("A1"), NAME_None, TEXT("B1"), Error),
		ERTScenarioAuthoringResult::Invalid);
	TestEqual(TEXT("ancora vuoto"), Intents(), 0);

	// Cella senza abilita': rifiutata.
	TestEqual(TEXT("cella senza abilita'"),
		Draft.SetMainActionOnCell(Turn, TEXT("A1"), NAME_None, FRTCellId(1, 0, 0), Error),
		ERTScenarioAuthoringResult::Invalid);
	TestEqual(TEXT("ancora vuoto"), Intents(), 0);

	// ✅ E una accettata scrive, altrimenti il test sarebbe verde anche su una facade che non fa nulla.
	TestEqual(TEXT("la chiamata buona invece scrive"),
		Draft.SetMainActionOnUnit(Turn, TEXT("A1"), TEXT("Action.BasicAttack"), TEXT("B1"), Error),
		ERTScenarioAuthoringResult::Success);
	TestEqual(TEXT("un intent"), Intents(), 1);

	return true;
}

/**
 * IL TURNO DUPLICATO È INDIPENDENTE, IN ENTRAMBE LE DIREZIONI — `#1627`.
 *
 * ⚠️ **Il criterio della issue è quasi vacuo in C++, e questo test punta altrove.** `TArray<FRTScenarioTurn>`
 * contiene valori, e `FRTScenarioTurn` è fatto di `TArray` per valore: qualunque copia è già profonda, e un
 * test che verifica «non condividono» sarebbe verde per costruzione del linguaggio.
 *
 * 🔑 Serve comunque, per due ragioni misurate. La prima è che l'implementazione **ovvia** non compila un
 * comportamento, lo fa crashare: `Turns.Insert(Turns[i], i + 1)` passa a `Insert` un riferimento dentro
 * l'array che `Insert` sta per riallocare, e `TArray::CheckAddress` è un `checkf` esplicito. La seconda è
 * che un'implementazione futura che introducesse un indice o un handle condiviso deve cadere **qui**, non in
 * PIE tre settimane dopo.
 *
 * ⛔ Su un turno **non vuoto**: duplicare due strutture vuote e confrontarle non misura niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioDuplicatedTurnIsIndependentTest,
	"RefactorTactics.Scenario.DuplicatedTurnIsIndependentInBothDirections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioDuplicatedTurnIsIndependentTest::RunTest(const FString&)
{
	FRTScenarioDraft Draft;
	FString Error;
	if (!TestTrue(TEXT("scenario di partenza caricato"), OpenTurnDraft(Draft, Error)))
	{
		AddError(Error);
		return false;
	}
	int32 Source = INDEX_NONE;
	if (!TestEqual(TEXT("turno aggiunto"), Draft.AddTurn(Source, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}

	// Un turno con qualcosa dentro: movimento, azione con bersaglio, e un secondo intent.
	const TArray<FRTCellId> Route = { FRTCellId(-1, 0, 0) };
	Draft.SetMoveIntent(Source, TEXT("A1"), Route, Error);
	Draft.SetMainActionOnUnit(Source, TEXT("A1"), TEXT("Action.BasicAttack"), TEXT("B1"), Error);
	Draft.SetMainAction(Source, TEXT("B1"), TEXT("Action.Guard"), Error);

	int32 Copy = INDEX_NONE;
	if (!TestEqual(TEXT("turno duplicato"),
		Draft.DuplicateTurn(Source, Copy, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}
	TestEqual(TEXT("la copia sta subito dopo l'originale"), Copy, Source + 1);
	TestEqual(TEXT("i turni sono due"), Draft.NumTurns(), 2);

	auto IntentsOf = [&Draft](int32 T) { return Draft.GetScenario().Turns[T].Intents.Num(); };
	TestEqual(TEXT("la copia porta gli stessi intent"), IntentsOf(Copy), IntentsOf(Source));
	if (!TestEqual(TEXT("due intent per turno"), IntentsOf(Source), 2))
	{
		return false;
	}

	// --- direzione 1: tocco la COPIA, l'originale non cambia.
	Draft.SetMainAction(Copy, TEXT("A1"), TEXT("Action.Brace"), Error);
	TestEqual(TEXT("la copia e' cambiata"),
		Draft.GetScenario().Turns[Copy].Intents[0].Ability, FName(TEXT("Action.Brace")));
	TestEqual(TEXT("e l'originale no"),
		Draft.GetScenario().Turns[Source].Intents[0].Ability, FName(TEXT("Action.BasicAttack")));
	TestEqual(TEXT("nemmeno nel suo movimento"),
		Draft.GetScenario().Turns[Source].Intents[0].Move.Num(), 1);

	// --- direzione 2: tocco l'ORIGINALE, la copia non cambia. Senza questa meta', una condivisione a senso
	// unico — la copia che punta all'originale — passerebbe il test.
	Draft.RemoveIntent(Source, TEXT("B1"), Error);
	TestEqual(TEXT("l'originale ha perso un intent"), IntentsOf(Source), 1);
	TestEqual(TEXT("e la copia li ha ancora entrambi"), IntentsOf(Copy), 2);

	return true;
}

/**
 * L'ORDINE DEI TURNI SOPRAVVIVE A `save → load` — `#1627`.
 *
 * 🔑 **È l'invariante che vale comunque, e la issue non la isolava.** Il criterio scritto dice *«il file
 * scritto dopo un riordino rilegge nello stesso ordine mostrato dalla UI»* — ma un altro criterio permette
 * di **non offrire** il riordino, e infatti non lo si offre (la ragione sta su `DuplicateTurn`). Preso alla
 * lettera, quel criterio resterebbe senza soggetto.
 *
 * Ciò che si può misurare, e che serve davvero, è che l'ordine **dichiarato** attraversi il file: tre turni
 * distinguibili, salvati e riletti da disco, nella stessa sequenza.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioTurnOrderSurvivesSaveLoadTest,
	"RefactorTactics.Scenario.TurnOrderSurvivesSaveAndLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioTurnOrderSurvivesSaveLoadTest::RunTest(const FString&)
{
	const FString Dir = TurnAuthoringTempDir();
	const FString Path = FPaths::Combine(Dir, TEXT("Sequenza.json"));
	ON_SCOPE_EXIT{ IFileManager::Get().DeleteDirectory(*Dir, false, true); };

	FRTScenarioDraft Draft;
	FString Error;
	if (!TestTrue(TEXT("scenario di partenza caricato"), OpenTurnDraft(Draft, Error)))
	{
		AddError(Error);
		return false;
	}

	// Tre turni resi distinguibili dal loro contenuto: senza, l'ordine non e' osservabile e il test
	// sarebbe verde su qualunque permutazione.
	const TCHAR* Abilities[] = { TEXT("Action.Guard"), TEXT("Action.Brace"), TEXT("Action.Overwatch") };
	for (const TCHAR* Ability : Abilities)
	{
		int32 T = INDEX_NONE;
		if (!TestEqual(TEXT("turno aggiunto"), Draft.AddTurn(T, Error), ERTScenarioAuthoringResult::Success))
		{
			AddError(Error);
			return false;
		}
		TestEqual(FString::Printf(TEXT("azione %s scritta"), Ability),
			Draft.SetMainAction(T, TEXT("A1"), FName(Ability), Error), ERTScenarioAuthoringResult::Success);
	}
	TestEqual(TEXT("tre turni"), Draft.NumTurns(), 3);

	if (!TestEqual(TEXT("valido"), Draft.Validate(Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}
	if (!TestEqual(TEXT("salvato"), Draft.SaveToFile(Path, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}

	FRTScenarioDraft Reloaded;
	if (!TestEqual(TEXT("riaperto"), Reloaded.OpenFromFile(Path, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}
	if (!TestEqual(TEXT("tre turni riletti"), Reloaded.NumTurns(), 3))
	{
		return false;
	}
	for (int32 T = 0; T < 3; ++T)
	{
		const TArray<FRTScenarioIntent>& Intents = Reloaded.GetScenario().Turns[T].Intents;
		if (TestEqual(FString::Printf(TEXT("turno %d ha un intent"), T), Intents.Num(), 1))
		{
			TestEqual(FString::Printf(TEXT("turno %d e' rimasto il suo"), T),
				Intents[0].Ability, FName(Abilities[T]));
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
