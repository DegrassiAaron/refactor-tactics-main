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
#include "Turn/RTReactionOpportunityTypes.h" // BoundaryCapableReactionIds (#2866)
#include "Turn/RTTurnLog.h"   // ERTLogCategory, ERTReactionDecisionOutcome: il filtro di fase (#2867)
#include "Turn/RTTurnRules.h" // ERTMatchPhase
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

	// ── `on.reaction` (#2866) ────────────────────────────────────────────────────────────────────────
	//
	// Il refuso: due lettere scambiate. Senza questa guardia il file caricava verde e il difetto riemergeva
	// a fine turno come «nessuna finestra ha soddisfatto il selettore» — vero, e nel posto sbagliato.
	Rifiuta(TEXT("reaction con un refuso"), 5,
		TEXT(R"({ "on": { "reactor": "B1", "reaction": "Action.Overwtach" }, "respond": "HOLD" })"),
		TEXT("Action.Overwtach"));
	// 🔑 **Il caso che decide la forma della guardia, e che il refuso da solo non coglie**: `Action.Counter`
	// e' una reaction VERA del catalogo — la base di `Reaction.CounterShot` — e non apre nessuna finestra.
	// Una guardia costruita sul catalogo lo accetterebbe, e lo scenario resterebbe verde nominando una
	// finestra che non esistera' mai: il difetto spostato, non chiuso.
	Rifiuta(TEXT("reaction del catalogo che non apre finestre"), 5,
		TEXT(R"({ "on": { "reactor": "B1", "reaction": "Action.Counter" }, "respond": "HOLD" })"),
		TEXT("Action.Counter"));
	return true;
}

/**
 * L'insieme delle reaction che aprono un boundary e' **chiuso**, e il messaggio di rifiuto lo elenca.
 *
 * ⚠️ **Questo test non ripete i valori dell'elenco**, e l'omissione e' il punto: un test che scrivesse
 * `{Action.Overwatch, Action.Brace}` accanto alla funzione che li dichiara confermerebbe se stesso, e
 * resterebbe verde anche il giorno in cui un produttore emette un id che l'elenco non ha. Qui si verificano
 * due proprieta' che non dipendono dai valori — l'elenco non e' vuoto, e il predicato e' coerente con esso —
 * mentre il **pin sui produttori reali** lo portano gli scenari del corpus, che nominano la propria reaction
 * nel selettore: `Spec.Overwatch.HoldThenFire` e `Spec.Brace.ProfileChangesResponse` diventano rossi se un
 * produttore cambia `ActionId`, perche' il loro selettore smette di trovare la finestra.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioBoundaryReactionSetTest,
	"RefactorTactics.Scenario.BoundaryCapableReactionsAreAClosedSet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioBoundaryReactionSetTest::RunTest(const FString&)
{
	const TArray<FName>& Ammesse = URTReactionOpportunityLibrary::BoundaryCapableReactionIds();

	// Anti-vacuita': con l'elenco vuoto il predicato rifiuterebbe tutto e la guardia bloccherebbe ogni
	// scenario che usa `on.reaction` — un rosso generalizzato, non una guardia.
	if (!TestTrue(TEXT("anti-vacuita': l'elenco non e' vuoto"), Ammesse.Num() > 0)) { return false; }

	// Il predicato dice esattamente cio' che l'elenco contiene: due verita' che non possono divergere.
	for (const FName& Id : Ammesse)
	{
		TestTrue(*FString::Printf(TEXT("'%s' e' ammessa dal predicato"), *Id.ToString()),
			URTReactionOpportunityLibrary::IsBoundaryCapableReaction(Id));
	}

	// `NAME_None` non e' un'ammissione: e' il campo non dichiarato, e il loader lo tratta come «nessun
	// vincolo» prima ancora di interrogare il predicato. Se qui passasse, un `"reaction": ""` diventerebbe
	// un vincolo silenziosamente vero.
	TestFalse(TEXT("NAME_None non e' una reaction ammessa"),
		URTReactionOpportunityLibrary::IsBoundaryCapableReaction(NAME_None));

	// Una reaction VERA del catalogo che non apre finestre resta fuori. E' la proprieta' per cui questo
	// elenco esiste separato dal catalogo, e l'unico valore che il test nomina — perche' cio' che afferma e'
	// un'ASSENZA, e un'assenza non si puo' derivare dall'elenco che si sta verificando.
	TestFalse(TEXT("'Action.Counter' apre finestre? no: e' a catalogo ma non e' boundary-capable"),
		URTReactionOpportunityLibrary::IsBoundaryCapableReaction(FName(TEXT("Action.Counter"))));
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

/**
 * La guardia vale anche per uno scenario COSTRUITO IN MEMORIA, non solo per uno letto da JSON.
 *
 * 🔴 **E' la meta' che si dimentica**, ed e' gia' successo una volta in questo file: i controlli scritti
 * dentro `LoadFromString` li vede solo chi arriva dal parser, mentre `Validate` e' il gate che ogni strada
 * attraversa — `FRTScenarioSession::Start` lo chiama. Ogni scenario costruito da codice (l'editor di
 * scenari, ogni chiamante di `RunScenarioIsolated`, e i test di questa stessa fase) salterebbe la guardia.
 *
 * Il commento di `ValidateDecisionForm` lo dichiara: la funzione esiste perche' erano DUE copie e sono
 * divergite alla prima aggiunta. Questo test e' cio' che impedisce che accada di nuovo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioBoundaryReactionInMemoryTest,
	"RefactorTactics.Scenario.BoundaryReactionGuardAlsoCoversValidate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioBoundaryReactionInMemoryTest::RunTest(const FString&)
{
	FRTTestScenario Scenario = MakeTwoWindowScenario();
	// Una reaction del catalogo che non apre finestre: il caso che una guardia costruita sul catalogo
	// lascerebbe passare.
	FRTScenarioDecision D = MakeSelectorDecision(TEXT("W1"), TEXT("M1"), TEXT("HOLD"), nullptr);
	D.On.Reaction = FName(TEXT("Action.Counter"));
	Scenario.Turns[0].Decisions.Add(D);

	FString Error;
	const bool bValido = URTScenarioLoader::Validate(Scenario, Error);
	TestFalse(TEXT("`Validate` rifiuta la reaction non-boundary"), bValido);
	TestTrue(FString::Printf(TEXT("il motivo nomina 'Action.Counter' (era: '%s')"), *Error),
		Error.Contains(TEXT("Action.Counter")));

	// E il caso positivo, per non lasciare il test verde su un rifiuto generalizzato: la stessa decisione
	// con una reaction che un produttore emette davvero passa.
	Scenario.Turns[0].Decisions.Empty();
	Scenario.Turns[0].Decisions.Add(
		MakeSelectorDecision(TEXT("W1"), TEXT("M1"), TEXT("HOLD"), nullptr));
	FString ErrorePositivo;
	TestTrue(FString::Printf(TEXT("`Validate` accetta 'Action.Overwatch' (errore: '%s')"), *ErrorePositivo),
		URTScenarioLoader::Validate(Scenario, ErrorePositivo));
	return true;
}

// === Filtro di fase sulle assertion del TurnLog (#2867, "version": 6) ===============================

/**
 * 🔴 **Il filtro DISCRIMINA**, ed e' la sola proprieta' che conta: lo stesso evento contato con la fase in
 * cui e' avvenuto e con un'altra deve dare risultati diversi.
 *
 * ⚠️ Senza questa riga un filtro che non filtrasse nulla resterebbe verde: passerebbe il round-trip,
 * passerebbe il parsing, e conterebbe gli eventi di ogni fase come se il vincolo non ci fosse. E' lo stesso
 * difetto che `LogActionId` evita dichiarando che il confronto e' esatto sull'`FName` e non un prefisso.
 *
 * Il caso: `M2` viene fermata da un colpo di Overwatch, che e' una reazione risolta **dentro la fase Move**
 * — l'opportunity nasce a un micro-step del movimento. Contare quel colpo chiedendo `Move` lo trova;
 * chiedendo `Blast` non lo trova, e la differenza dimostra che la fase entra nel confronto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLogPhaseFilterDiscriminatesTest,
	"RefactorTactics.Scenario.LogPhaseFilterDiscriminates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLogPhaseFilterDiscriminatesTest::RunTest(const FString&)
{
	// Conta le voci `ReactionDecision`/`FireChosen` chiedendo (o no) una fase, e dice se l'harness e' PASS.
	auto ContaConFase = [this](bool bConFase, ERTMatchPhase Fase, int32 Atteso) -> bool
	{
		FRTTestScenario Scenario = MakeTwoWindowScenario();
		Scenario.Version = 6;
		Scenario.Turns[0].Decisions.Add(
			MakeSelectorDecision(TEXT("W1"), TEXT("M2"), TEXT("FIRE"), TEXT("M2")));
		Scenario.Turns[0].Decisions.Add(
			MakeSelectorDecision(TEXT("W1"), TEXT("M1"), TEXT("HOLD"), nullptr));

		FRTTestExpectation Conteggio;
		Conteggio.Kind = ERTAssertionKind::LogEventCount;
		Conteggio.LogCategory = ERTLogCategory::ReactionDecision;
		Conteggio.LogOutcome = static_cast<uint8>(ERTReactionDecisionOutcome::FireChosen);
		Conteggio.Value = Atteso;
		Conteggio.bHasLogPhase = bConFase;
		Conteggio.LogPhase = Fase;
		Scenario.Expect.Add(Conteggio);

		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
		const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
		RTWorldFixtures::DestroyWorld(World);

		const bool bPass = Result.Outcome == ERTTestOutcome::Pass;
		if (!bPass)
		{
			AddInfo(FString::Printf(TEXT("esito: %s · note: %s"),
				*Result.OutcomeString(), *FString::Join(Result.Notes, TEXT(" | "))));
		}
		return bPass;
	};

	// Premessa: senza filtro il `FIRE` c'e' ed e' uno solo. Se questa cade non e' il filtro a non
	// funzionare — e' lo scenario a non produrre l'evento, e le due righe sotto non direbbero niente.
	if (!TestTrue(TEXT("premessa: senza filtro il FIRE e' contato una volta"),
		ContaConFase(/*bConFase*/ false, ERTMatchPhase::Move, 1)))
	{
		return false;
	}

	// 🔑 Le due righe che dimostrano il filtro: stesso evento, fasi diverse, conteggi attesi diversi.
	TestTrue(TEXT("con `phase: Move` il colpo di Overwatch si trova"),
		ContaConFase(/*bConFase*/ true, ERTMatchPhase::Move, 1));
	TestTrue(TEXT("con `phase: Blast` lo stesso colpo NON si trova (atteso 0)"),
		ContaConFase(/*bConFase*/ true, ERTMatchPhase::Blast, 0));
	return true;
}

/** Le forme malformate del filtro di fase sono un `ERROR` con il motivo. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLogPhaseRejectTest,
	"RefactorTactics.Scenario.LogPhaseRejectsMalformedForms",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLogPhaseRejectTest::RunTest(const FString&)
{
	auto Rifiuta = [this](const TCHAR* Cosa, int32 Versione, const TCHAR* Expect, const TCHAR* Atteso)
	{
		const FString Json = FString::Printf(TEXT(R"JSON(
		{
		  "scenarioId": "Spec.LogPhase.Reject", "version": %d, "mapRadius": 3,
		  "units": [ { "id": "A1", "hero": "Hero.Branth", "team": 0, "cell": [-2, 0, 0] } ],
		  "turns": [ { "intents": [] } ],
		  "expect": [ %s ]
		}
		)JSON"), Versione, Expect);

		FRTTestScenario Scenario;
		FString Error;
		TestFalse(FString::Printf(TEXT("%s: rifiutato"), Cosa),
			URTScenarioLoader::LoadFromString(*Json, Scenario, Error));
		TestTrue(FString::Printf(TEXT("%s: il motivo nomina '%s' (era: '%s')"), Cosa, Atteso, *Error),
			Error.Contains(Atteso));
	};

	Rifiuta(TEXT("fase sconosciuta"), 6,
		TEXT(R"({ "type": "LogEventCount", "category": "Combat", "outcome": "Hit", "phase": "Blastt" })"),
		TEXT("Blastt"));
	// Il caso che conta: un filtro su un'assertion che non legge il TurnLog SEMBRA chiedere «dov'era a fine
	// Blast» — che e' precisamente cio' che non fa — e resterebbe verde sullo stato finale.
	Rifiuta(TEXT("phase su un'assertion che non legge il log"), 6,
		TEXT(R"({ "type": "UnitAtCell", "unit": "A1", "cell": [-2, 0, 0], "phase": "Blast" })"),
		TEXT("TurnLog"));
	Rifiuta(TEXT("thenPhase fuori da LogEventOrder"), 6,
		TEXT(R"({ "type": "LogEventCount", "category": "Combat", "outcome": "Hit", "thenPhase": "Move" })"),
		TEXT("LogEventOrder"));
	// Il verso che conta: senza il gate, una build a `SupportedVersion = 5` ignorerebbe il filtro e
	// l'assertion verificherebbe piu' di quanto il file chiede, restando verde per la ragione sbagliata.
	Rifiuta(TEXT("il filtro di fase richiede version 6"), 5,
		TEXT(R"({ "type": "LogEventCount", "category": "Combat", "outcome": "Hit", "phase": "Blast" })"),
		TEXT("version"));
	return true;
}

/** Il filtro sopravvive a un round-trip, e la fase entra nel NOME dell'evento. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioLogPhaseRoundTripTest,
	"RefactorTactics.Scenario.LogPhaseRoundTrips",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioLogPhaseRoundTripTest::RunTest(const FString&)
{
	const TCHAR* Json = TEXT(R"JSON(
	{
	  "scenarioId": "Spec.LogPhase.RoundTrip", "version": 6, "mapRadius": 3,
	  "units": [ { "id": "A1", "hero": "Hero.Branth", "team": 0, "cell": [-2, 0, 0] } ],
	  "turns": [ { "intents": [] } ],
	  "expect": [
	    { "type": "LogEventOrder", "category": "Combat", "outcome": "Hit", "phase": "Blast",
	      "thenCategory": "Move", "thenOutcome": "Moved", "thenPhase": "Move" }
	  ]
	}
	)JSON");

	FRTTestScenario Scenario;
	FString Error;
	if (!TestTrue(TEXT("scenario col filtro di fase accettato"),
		URTScenarioLoader::LoadFromString(Json, Scenario, Error)))
	{
		AddError(FString::Printf(TEXT("il loader ha rifiutato: '%s'"), *Error));
		return false;
	}
	if (!TestEqual(TEXT("una assertion"), Scenario.Expect.Num(), 1)) { return false; }
	TestTrue (TEXT("il filtro sul primo evento e' dichiarato"), Scenario.Expect[0].bHasLogPhase);
	TestEqual(TEXT("la fase del primo evento"),
		static_cast<int32>(Scenario.Expect[0].LogPhase), static_cast<int32>(ERTMatchPhase::Blast));
	TestTrue (TEXT("il filtro sul secondo evento e' dichiarato"), Scenario.Expect[0].bHasThenPhase);

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
		AddError(FString::Printf(TEXT("rilettura fallita: '%s'"), *Error));
		return false;
	}
	TestTrue (TEXT("round-trip: il filtro sopravvive"), Reread.Expect[0].bHasLogPhase);
	TestEqual(TEXT("round-trip: la fase"),
		static_cast<int32>(Reread.Expect[0].LogPhase), static_cast<int32>(ERTMatchPhase::Blast));
	TestTrue (TEXT("round-trip: anche il secondo filtro"), Reread.Expect[0].bHasThenPhase);

	// ⚠️ La fase entra nel NOME dell'evento: due assertion che differiscono solo per essa devono produrre
	// messaggi di fallimento diversi, o chi legge il referto non sa quale delle due e' caduta.
	const FString ConFase = URTScenarioLoader::DescribeLogEvent(
		ERTLogCategory::Combat, /*Outcome*/ 0, NAME_None, /*bHasPhase*/ true, ERTMatchPhase::Blast);
	const FString SenzaFase = URTScenarioLoader::DescribeLogEvent(
		ERTLogCategory::Combat, /*Outcome*/ 0, NAME_None, /*bHasPhase*/ false, ERTMatchPhase::Blast);
	TestNotEqual(TEXT("il nome cambia quando la fase e' dichiarata"), ConFase, SenzaFase);
	TestTrue(FString::Printf(TEXT("il nome porta la fase (era: '%s')"), *ConFase),
		ConFase.Contains(TEXT("Blast")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
