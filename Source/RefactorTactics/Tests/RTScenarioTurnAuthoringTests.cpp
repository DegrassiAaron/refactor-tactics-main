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

	const int32 Turn = Draft.AddTurn();
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

	const int32 Turn = Draft.AddTurn();

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
	TestEqual(TEXT("AddTurn su draft chiuso"), Empty.AddTurn(), INDEX_NONE);
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
	const int32 Turn = Draft.AddTurn();
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
	UWorld* World = MakeTurnAuthoringWorld();
	if (!TestNotNull(TEXT("mondo per l'esecuzione"), World))
	{
		return false;
	}
	ON_SCOPE_EXIT{ DestroyTurnAuthoringWorld(World); };

	const FRTTestResult Passing = URTScenarioRunner::Run(World, Reloaded.GetScenario());
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

	const FRTTestResult AfterMutation = URTScenarioRunner::Run(World, Mutated.GetScenario());

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

	for (const FName& Name : { FName(TEXT("AddTurn")), FName(TEXT("GetTurnCount")), FName(TEXT("SetMoveIntent")),
		FName(TEXT("SetWaitIntent")), FName(TEXT("RemoveIntent")), FName(TEXT("AddExpectationUnitAtCell")),
		FName(TEXT("AddExpectationLogEventCount")), FName(TEXT("RemoveExpectation")),
		FName(TEXT("GetReachableCells")) })
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
		TestTrue(TEXT("ed e' BlueprintType"), LogCategory->GetBoolMetaData(TEXT("BlueprintType")));
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

	TestEqual(TEXT("la facade aggiunge un turno"), Authoring->AddTurn(), 0);
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

#endif // WITH_DEV_AUTOMATION_TESTS
