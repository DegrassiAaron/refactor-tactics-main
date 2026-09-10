// Il SELETTORE SEMANTICO delle `turns[].decisions` — `on` (`"version": 5`).
//
// Il difetto che chiude, e non e' ipotetico: fino alla `4` una risposta scriptata si abbinava alla propria
// finestra **per ordine di dichiarazione** — la prima voce di un'unita' alla prima opportunity che si apre per
// lei. `Spec.Overwatch.HoldThenFire` e' il caso che lo mostra: due voci `V1` consecutive, `HOLD` poi `FIRE`,
// dove *quale* delle due risponda a *quale* varco lo decide il micro-step in cui i due mover entrano nella
// zona. Cambiare un waypoint di `R1` — un dato che quello scenario non sta verificando — scambia le due
// risposte senza che nulla lo dica: lo scenario resta verde e verifica un'altra cosa.
//
// Quattro proprieta', separate perche' falliscono per ragioni diverse:
//
//   1. il selettore si carica, si riscrive identico, e il reactor resta UNA verita' sola (`Decision::Unit`);
//   2. le forme malformate sono un `ERROR` con il motivo, non un selettore che non vincola niente;
//   3. il matching e' SEMANTICO: due decisioni dichiarate nell'ordine sbagliato finiscono sulle finestre
//      giuste, e con l'abbinamento per ordine finirebbero scambiate;
//   4. un selettore soddisfatto da PIU' finestre e' un `ERROR` di scenario ambiguo, distinto dalla finestra
//      scoperta — i due difetti si correggono in posti opposti.
//
// Cio' che questi test NON coprono, e va detto: la CELLA del trigger. Non e' un campo del selettore perche'
// non e' un campo di `FRTReactionOpportunity`, che ha un elenco **chiuso** protetto da
// `RefactorTactics.Overwatch.OpportunityLeaksNoFuture` — vedi `FRTScenarioOpportunitySelector`.

#include "Misc/AutomationTest.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/RTTestResult.h"
#include "ScenarioHarness/RTTestScenario.h"
#include "Tests/RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/**
	 * L'allestimento di `Spec.Overwatch.HoldThenFire` compresso in UN turno.
	 *
	 * ⚠️ **La geometria non e' scelta a caso e non va ritoccata senza rifare il conto**: `MakeSuppressiveZone`
	 * costruisce una LINEA lungo il facing, quindi `W1` a `(2,-1)` che guarda `W` controlla `(1,-1) (0,-1)
	 * (-1,-1) (-2,-1)` — quattro celle, la portata di `Hero.Ivrin`. I due mover sono scelti perche' entrano
	 * nella linea a **micro-step diversi**: `BuildOverwatchTriggers` raggruppa per passo, e due bersagli nello
	 * stesso passo darebbero UNA sola opportunity (`Overwatch.SimultaneousTargetsSingleOpportunity`) — cioe'
	 * una decisione sola, e questi test non direbbero piu' quello che dicono.
	 *
	 * ⚠️ Lo scenario si costruisce in MEMORIA e non in `Scenarios/`: il corpus versionato non si tocca finche'
	 * loader, writer e compatibilita' non sono verdi. Il percorso pero' e' quello vero —
	 * `URTScenarioRunner::Run` entra dagli stessi ingressi del giocatore — quindi qui non nasce nessun
	 * resolver parallelo.
	 */
	FRTTestScenario MakeTwoWindowScenario()
	{
		FRTTestScenario Scenario;
		Scenario.ScenarioId = TEXT("Unit.Scenario.SelectorTwoWindows");
		Scenario.Version = 5;
		Scenario.Seed = 0;
		Scenario.MapRadius = 5;

		FRTScenarioUnit W1;
		W1.Id = TEXT("W1");
		W1.HeroId = TEXT("Hero.Ivrin");
		W1.TeamId = 1;
		W1.Cell = FRTCellId(2, -1, 0);
		// Il cono E' il facing (ADR-0005 §4c), e si dichiara al PIAZZAMENTO: `Armed.Facing = Unit->Facing`
		// viene letto in Prep, mentre una rotazione dichiarata si applica dentro `ResolveMovement` — troppo
		// tardi per il proprio cono. Un watcher che non si muove conserva il facing con cui e' stato posato.
		W1.Facing = ERTHexDirection::W;
		Scenario.Units.Add(W1);

		FRTScenarioUnit M1;
		M1.Id = TEXT("M1");
		M1.HeroId = TEXT("Hero.Aevik");
		M1.TeamId = 0;
		M1.Cell = FRTCellId(-2, 0, 0);
		Scenario.Units.Add(M1);

		FRTScenarioUnit M2;
		M2.Id = TEXT("M2");
		M2.HeroId = TEXT("Hero.Muiren");
		M2.TeamId = 0;
		M2.Cell = FRTCellId(-2, 1, 0);
		Scenario.Units.Add(M2);

		FRTScenarioTurn Turn;
		// La finestra live e' una capability: dichiararla e' cio' che distingue un `BLOCKED` onesto da un `Error`.
		Turn.Requires.Add(TEXT("DecisionBoundary"));

		FRTScenarioIntent Arm;
		Arm.UnitId = TEXT("W1");
		Arm.Ability = TEXT("Action.Overwatch");
		Turn.Intents.Add(Arm);

		FRTScenarioIntent MoveM1;
		MoveM1.UnitId = TEXT("M1");
		MoveM1.Move.Add(FRTCellId(-2, -1, 0)); // entra nella linea al primo passo
		Turn.Intents.Add(MoveM1);

		FRTScenarioIntent MoveM2;
		MoveM2.UnitId = TEXT("M2");
		MoveM2.Move.Add(FRTCellId(-1, 0, 0));
		MoveM2.Move.Add(FRTCellId(0, -1, 0));  // entra nella linea al secondo passo
		MoveM2.Move.Add(FRTCellId(1, -1, 0));
		Turn.Intents.Add(MoveM2);

		Scenario.Turns.Add(Turn);

		// L'harness rifiuta uno scenario senza `expect` — «passerebbe sempre». Questa dice qualcosa di vero e
		// non banale: chi arma un Overwatch spende l'azione principale e non si muove.
		FRTTestExpectation WatcherStays;
		WatcherStays.Kind = ERTAssertionKind::UnitAtCell;
		WatcherStays.UnitId = TEXT("W1");
		WatcherStays.Cell = FRTCellId(2, -1, 0);
		Scenario.Expect.Add(WatcherStays);

		return Scenario;
	}

	FRTScenarioDecision MakeSelectorDecision(const TCHAR* Reactor, const TCHAR* TriggerUnit,
		const TCHAR* Respond, const TCHAR* Target)
	{
		FRTScenarioDecision D;
		D.bHasSelector = true;
		D.On.Reactor = Reactor;
		D.On.Reaction = FName(TEXT("Action.Overwatch"));
		if (TriggerUnit) { D.On.TriggerUnit = TriggerUnit; }
		// In memoria il reactor resta un campo solo, come fa il loader leggendo `on.reactor`.
		D.Unit = Reactor;
		D.Respond = Respond;
		if (Target) { D.Target = Target; }
		return D;
	}

	void AddCellExpectation(FRTTestScenario& Scenario, const TCHAR* UnitId, const FRTCellId& Cell)
	{
		FRTTestExpectation Where;
		Where.Kind = ERTAssertionKind::UnitAtCell;
		Where.UnitId = UnitId;
		Where.Cell = Cell;
		Scenario.Expect.Add(Where);
	}
}

/**
 * Il selettore si carica, il reactor finisce in `Unit`, e un round-trip lo riscrive identico.
 *
 * ⚠️ **Il round-trip e' meta' del test e non un extra**: writer e loader sono due meta' di una regola sola
 * (`RTScenarioLoader.h`), e una chiave aggiunta da una parte e non dall'altra si nota solo qui — un editor
 * che salvasse uno scenario perderebbe il selettore in silenzio, restituendo un file che l'ordine riabbina.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioSelectorRoundTripTest,
	"RefactorTactics.Scenario.SelectorLoadsAndRoundTrips",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioSelectorRoundTripTest::RunTest(const FString&)
{
	const TCHAR* Json = TEXT(R"JSON(
	{
	  "scenarioId": "Spec.Selector.RoundTrip", "version": 5, "mapRadius": 3,
	  "units": [
	    { "id": "A1", "hero": "Hero.Branth", "team": 0, "cell": [-2, 0, 0] },
	    { "id": "B1", "hero": "Hero.Ivrin",  "team": 1, "cell": [ 2, 0, 0] }
	  ],
	  "turns": [ { "intents": [], "decisions": [
	    { "on": { "reactor": "B1", "reaction": "Action.Overwatch", "triggerUnit": "A1" },
	      "respond": "FIRE", "target": "A1" },
	    { "on": { "reactor": "B1" }, "respond": "HOLD" }
	  ] } ],
	  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
	}
	)JSON");

	FRTTestScenario Scenario;
	FString Error;
	if (!TestTrue(TEXT("scenario col selettore accettato"),
		URTScenarioLoader::LoadFromString(Json, Scenario, Error)))
	{
		AddError(FString::Printf(TEXT("il loader ha rifiutato: '%s'"), *Error));
		return false;
	}
	if (!TestEqual(TEXT("un turno"), Scenario.Turns.Num(), 1)) { return false; }
	const FRTScenarioTurn& T = Scenario.Turns[0];
	if (!TestEqual(TEXT("due decisioni"), T.Decisions.Num(), 2)) { return false; }

	TestTrue (TEXT("prima: il selettore e' dichiarato"), T.Decisions[0].bHasSelector);
	TestEqual(TEXT("prima: reactor"),     T.Decisions[0].On.Reactor,     FString(TEXT("B1")));
	TestEqual(TEXT("prima: reaction"),    T.Decisions[0].On.Reaction,    FName(TEXT("Action.Overwatch")));
	TestEqual(TEXT("prima: triggerUnit"), T.Decisions[0].On.TriggerUnit, FString(TEXT("A1")));
	// 🔑 Il reactor e' UNA verita' sola: `on.reactor` popola `Unit`, che e' cio' che validazione, messaggi e
	// matching per unita' leggono. Senza questa riga il selettore sarebbe una seconda sorgente dello stesso
	// fatto, e le due divergerebbero al primo campo aggiunto.
	TestEqual(TEXT("prima: il reactor e' anche `Unit`"), T.Decisions[0].Unit, FString(TEXT("B1")));

	TestTrue(TEXT("seconda: il selettore e' dichiarato"), T.Decisions[1].bHasSelector);
	TestTrue(TEXT("seconda: nessun vincolo di reaction"), T.Decisions[1].On.Reaction.IsNone());
	TestTrue(TEXT("seconda: nessun vincolo di trigger"),  T.Decisions[1].On.TriggerUnit.IsEmpty());

	FString Written;
	if (!TestTrue(TEXT("lo scenario si riserializza"),
		URTScenarioLoader::SaveToString(Scenario, Written, Error)))
	{
		AddError(FString::Printf(TEXT("il writer ha rifiutato: '%s'"), *Error));
		return false;
	}
	FRTTestScenario Reread;
	if (!TestTrue(TEXT("il testo scritto si rilegge"),
		URTScenarioLoader::LoadFromString(Written, Reread, Error)))
	{
		AddError(FString::Printf(TEXT("rilettura fallita: '%s' — testo:\n%s"), *Error, *Written));
		return false;
	}
	if (!TestEqual(TEXT("round-trip: un turno"), Reread.Turns.Num(), 1)) { return false; }
	if (!TestEqual(TEXT("round-trip: due decisioni"), Reread.Turns[0].Decisions.Num(), 2)) { return false; }
	TestEqual(TEXT("round-trip: reactor"),
		Reread.Turns[0].Decisions[0].On.Reactor, FString(TEXT("B1")));
	TestEqual(TEXT("round-trip: reaction"),
		Reread.Turns[0].Decisions[0].On.Reaction, FName(TEXT("Action.Overwatch")));
	TestEqual(TEXT("round-trip: triggerUnit"),
		Reread.Turns[0].Decisions[0].On.TriggerUnit, FString(TEXT("A1")));
	TestTrue(TEXT("round-trip: il selettore sopravvive alla scrittura"),
		Reread.Turns[0].Decisions[1].bHasSelector);
	// ⛔ `unit` e `on` non convivono: il writer scrive l'uno o l'altro, e il testo prodotto non deve contenere
	// una forma che il loader poi rifiuta.
	TestFalse(TEXT("il testo scritto non porta `unit` accanto a `on`"),
		Written.Contains(TEXT("\"unit\"")));
	return true;
}

/**
 * Le forme malformate del selettore sono un `ERROR` con il motivo.
 *
 * ⚠️ **Il messaggio conta quanto il rifiuto**: un `on` che non vincola niente e passasse in silenzio sarebbe
 * una risposta che *dichiara* di scegliere una finestra e poi prende la prima disponibile — cioe' il difetto
 * dell'ordine, riaperto dalla chiave che esiste per chiuderlo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioSelectorRejectTest,
	"RefactorTactics.Scenario.SelectorRejectsMalformedForms",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioSelectorRejectTest::RunTest(const FString&)
{
	auto Rifiuta = [this](const TCHAR* Cosa, int32 Versione, const TCHAR* Decision, const TCHAR* Atteso)
	{
		const FString Json = FString::Printf(TEXT(R"JSON(
		{
		  "scenarioId": "Spec.Selector.Reject", "version": %d, "mapRadius": 3,
		  "units": [
		    { "id": "A1", "hero": "Hero.Branth", "team": 0, "cell": [-2, 0, 0] },
		    { "id": "B1", "hero": "Hero.Ivrin",  "team": 1, "cell": [ 2, 0, 0] }
		  ],
		  "turns": [ { "intents": [], "decisions": [ %s ] } ],
		  "expect": [ { "type": "TurnsCompleted", "value": 1 } ]
		}
		)JSON"), Versione, Decision);

		FRTTestScenario Scenario;
		FString Error;
		const bool bLoaded = URTScenarioLoader::LoadFromString(*Json, Scenario, Error);
		TestFalse(FString::Printf(TEXT("%s: rifiutato"), Cosa), bLoaded);
		// Il messaggio deve NOMINARE il difetto: un rifiuto muto manda a cercare nel posto sbagliato.
		TestTrue(FString::Printf(TEXT("%s: il motivo nomina '%s' (era: '%s')"), Cosa, Atteso, *Error),
			Error.Contains(Atteso));
	};

	Rifiuta(TEXT("selettore vuoto"), 5,
		TEXT(R"({ "on": {}, "respond": "HOLD" })"), TEXT("nessun vincolo"));
	Rifiuta(TEXT("selettore senza reactor"), 5,
		TEXT(R"({ "on": { "reaction": "Action.Overwatch" }, "respond": "HOLD" })"), TEXT("reactor"));
	Rifiuta(TEXT("unit e on insieme"), 5,
		TEXT(R"({ "unit": "B1", "on": { "reactor": "B1" }, "respond": "HOLD" })"), TEXT("non convivono"));
	Rifiuta(TEXT("on non e' un oggetto"), 5,
		TEXT(R"({ "on": [], "respond": "HOLD" })"), TEXT("oggetto"));
	// ⚠️ `triggerCell` cade QUI, e il messaggio lo dice elencando le chiavi che esistono: la cella del trigger
	// non e' in `FRTReactionOpportunity`, quindi un selettore che la nominasse dichiarerebbe un vincolo che il
	// matching non puo' verificare.
	Rifiuta(TEXT("triggerCell non e' una chiave del selettore"), 5,
		TEXT(R"({ "on": { "reactor": "B1", "triggerCell": [0, -1, 0] }, "respond": "HOLD" })"),
		TEXT("triggerCell"));
	Rifiuta(TEXT("triggerUnit non schierata"), 5,
		TEXT(R"({ "on": { "reactor": "B1", "triggerUnit": "Z9" }, "respond": "HOLD" })"), TEXT("Z9"));
	Rifiuta(TEXT("reactor non schierato"), 5,
		TEXT(R"({ "on": { "reactor": "Z9" }, "respond": "HOLD" })"), TEXT("Z9"));
	// 🔴 Il verso che conta: un file che usa `on` deve DICHIARARE la versione che lo ammette, o una build a
	// `SupportedVersion = 4` lo accuserebbe di una «chiave sconosciuta» che invece la build non conosce.
	Rifiuta(TEXT("il selettore richiede version 5"), 4,
		TEXT(R"({ "on": { "reactor": "B1" }, "respond": "HOLD" })"), TEXT("version"));
	return true;
}

/**
 * 🔴 **IL PUNTO: il matching e' semantico, non posizionale.**
 *
 * Le due decisioni sono dichiarate nell'ordine **opposto** a quello in cui le finestre si aprono: prima la
 * risposta al trigger di `M2` (che entra nella zona al secondo passo), poi quella al trigger di `M1` (che
 * entra al primo). Con l'abbinamento per ordine la prima finestra — quella di `M1` — riceverebbe un
 * `FIRE:<M2>` che non e' fra le sue risposte legali: `IsResponseAllowed` lo respinge, l'esito diventa
 * `Rejected`, e `M1` prosegue mentre `M2` non viene mai fermata.
 *
 * ⚠️ **Si misura sulla POSIZIONE FINALE dei due mover**, che e' l'osservabile che distingue i due
 * comportamenti: un `FIRE` tronca il movimento nella cella raggiunta, un `HOLD` no. Contare le decisioni
 * applicate non basterebbe — nel ramo posizionale una delle due viene comunque «consumata».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioSelectorPicksWindowTest,
	"RefactorTactics.Scenario.SelectorPicksWindowByTrigger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioSelectorPicksWindowTest::RunTest(const FString&)
{
	FRTTestScenario Scenario = MakeTwoWindowScenario();
	// Ordine di dichiarazione INVERTITO rispetto all'ordine temporale delle finestre: e' cio' che rende il
	// test falsificabile. Se un giorno il matching tornasse posizionale, questa riga e' il motivo per cui il
	// test se ne accorge.
	Scenario.Turns[0].Decisions.Add(
		MakeSelectorDecision(TEXT("W1"), TEXT("M2"), TEXT("FIRE"), TEXT("M2")));
	Scenario.Turns[0].Decisions.Add(
		MakeSelectorDecision(TEXT("W1"), TEXT("M1"), TEXT("HOLD"), nullptr));

	// L'osservabile: `M1` ha ricevuto `HOLD` e ha completato il proprio passo; `M2` ha ricevuto `FIRE` ed e'
	// stata troncata nella cella in cui il colpo l'ha raggiunta, senza mai vedere il terzo waypoint.
	AddCellExpectation(Scenario, TEXT("M1"), FRTCellId(-2, -1, 0));
	AddCellExpectation(Scenario, TEXT("M2"), FRTCellId(0, -1, 0));

	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);

	// `BLOCKED` non e' un successo, e senza questa riga lo diventerebbe in silenzio: ogni conteggio sotto
	// darebbe zero, cioe' «difetto non riprodotto» indistinguibile da «assente».
	TestEqual(FString::Printf(
			TEXT("lo scenario e' PASS (esito: %s · error: '%s' · blocked: '%s' · note: %s)"),
			*Result.OutcomeString(), *Result.ErrorMessage, *Result.BlockedReason,
			*FString::Join(Result.Notes, TEXT(" | "))),
		static_cast<int32>(Result.Outcome), static_cast<int32>(ERTTestOutcome::Pass));

	TestEqual(TEXT("entrambe le decisioni sono state applicate"), Result.ScriptedDecisionsApplied, 2);
	TestEqual(TEXT("nessuna decisione e' rimasta inutilizzata"), Result.ScriptedDecisionsUnused, 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * Un selettore soddisfatto da PIU' finestre e' un `ERROR` di scenario ambiguo.
 *
 * ⚠️ **Distinto dalla finestra scoperta, e la distinzione non e' cosmetica**: i due difetti si correggono in
 * posti opposti — qui si STRINGE il selettore, li' se ne AGGIUNGE una. Un messaggio unico manderebbe ad
 * aggiungere una seconda risposta a un selettore che ne intercetta gia' due, cioe' a scrivere una decisione
 * che resterebbe a sua volta ambigua.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioSelectorAmbiguousTest,
	"RefactorTactics.Scenario.SelectorAmbiguousIsAnError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioSelectorAmbiguousTest::RunTest(const FString&)
{
	FRTTestScenario Scenario = MakeTwoWindowScenario();
	// Un solo vincolo — il reactor — che entrambe le finestre di `W1` soddisfano.
	Scenario.Turns[0].Decisions.Add(
		MakeSelectorDecision(TEXT("W1"), nullptr, TEXT("HOLD"), nullptr));

	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);

	TestEqual(FString::Printf(TEXT("scenario ambiguo -> ERROR (esito: %s · note: %s)"),
			*Result.OutcomeString(), *FString::Join(Result.Notes, TEXT(" | "))),
		static_cast<int32>(Result.Outcome), static_cast<int32>(ERTTestOutcome::Error));

	// Il motivo deve NOMINARE l'ambiguita': un `ERROR` che dicesse «finestra senza una decisione che la
	// nomini» manderebbe a correggere l'opposto di cio' che serve.
	const FString Tutto = Result.ErrorMessage + TEXT(" ") + FString::Join(Result.Notes, TEXT(" | "));
	TestTrue(FString::Printf(TEXT("il motivo dice 'ambiguo' (era: '%s')"), *Tutto),
		Tutto.Contains(TEXT("ambiguo")));

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
