// Due proprieta' del Decision Boundary che oggi sono vere PER COSTRUZIONE, e che nessun gate osservava (`#2461`).
//
// 🔑 **Perche' esistono, ed e' diverso dal solito.** Gli altri gate delle reazioni descrivono un comportamento.
// Questi due no: descrivono cio' che il codice **non puo' fare**, e lo descrivono perche' oggi non puo' farlo
// per come e' costruito — non perche' qualcuno lo impedisca esplicitamente.
//
//   · **nessun boundary annidato** — i `Triggers` si costruiscono UNA volta **prima** del ciclo
//     per-opportunity, il ciclo e' sequenziale e sincrono, e `ApplyReactionDecision` non rientra;
//   · **nessuna riscrittura retroattiva** — `TurnLog.Add(Entry)` e' l'UNICO punto di inserimento del suo
//     file, e nessun percorso muta una voce gia' inserita.
//
// ⚠️ **Una costruzione non e' una garanzia finche' nessuno la sorveglia.** Entrambe sparirebbero in silenzio
// se un domani i trigger si ricalcolassero dopo l'apply, o se comparisse un secondo sito di inserimento nel
// log. Questi test non aggiungono comportamento: rendono **falsificabile** cio' che oggi e' vero senza che
// nulla lo dica.
//
// 🔴 **Provenienza delle due proprieta', e non e' un ADR numerato.** Una prima stesura le citava come
// «invariante 3» e «invariante 8 di ADR-0004»: **falso**, ed e' stato corretto in code review. ADR-0004 §2 e'
// «Modello unificato: opportunity -> commit» e §8 e' «Parametri iniziali»; la numerazione veniva da
// `RefactorTactics_Claude_Reaction_Hardening_Roadmap_P0_2026-09-04.md`, che e' un handoff e **non e'
// autorita' corrente** (`CLAUDE.md §1`).
//
//   · l'annidamento lo scarta ADR-0004 davvero, ma a parole e non a numero: «limitatamente alla finestra
//     singola **non annidata**; lo stack LIFO resta scartato», e «interrupt annidati» fra gli scartati;
//   · la retroattivita' non ha oggi un owner documentale: la sua sede viva e' il tracker `#2462`.
//
// L'elenco canonico degli invarianti architetturali vive in `docs/product/piano-canonico-mvp.md §5`.
//
// ⚠️ Cio' che questi test NON coprono, e va detto: non dimostrano che il boundary sia atomico rispetto al
// resto della resolution, ne' che la revalidation sia mirata. Coprono due proprieta' del ciclo
// per-opportunity, non il boundary come concetto.
//
// ➕ Adiacente e **non** duplicato: `RefactorTactics.Overwatch.SecondFireOnDownedTargetLogsNoDamage` (`#1158`)
// sorveglia una **voce nuova falsa** — un `FIRE` che dichiara un danno mai inflitto. Qui si sorveglia una
// **voce esistente** che non deve cambiare. Sono due difetti diversi dello stesso vicinato.

#include "Misc/AutomationTest.h"

#include "EngineUtils.h"
#include "Map/RTCellId.h"
#include "Misc/ScopeExit.h"                  // ON_SCOPE_EXIT: il mondo si distrugge anche sui `return false`
#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/RTTestResult.h"
#include "ScenarioHarness/RTTestScenario.h"
#include "Tests/RTWorldFixtures.h"
#include "Turn/RTReactionOpportunityTypes.h" // FireResponseTarget: distingue un `FIRE` vero da un `HOLD`
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"           // GoldenEntriesMatch: l'elenco dei campi discriminanti sta li'
#include "Turn/RTTurnManager.h"

namespace RTBoundaryInvariants
{
	/**
	 * Lo scenario condiviso dai due test: due Wraith che guardano la STESSA cella da lati opposti, e un
	 * Gadget che ci entra con un solo micro-step.
	 *
	 * ⚠️ `MakeSuppressiveZone` costruisce una LINEA lungo il facing, quindi `(-1,-1,0)` cade nella zona di
	 * entrambi i watcher. Sono due WATCHER diversi sullo stesso bersaglio — quindi **due opportunity** — e non
	 * due bersagli nello stesso passo, che ne darebbero una sola
	 * (`Overwatch.SimultaneousTargetsSingleOpportunity`).
	 *
	 * 🔑 **Chi ARMA e' l'unica variabile fra le esecuzioni**, ed e' cio' che rende il confronto una misura
	 * invece di una coincidenza: unita', indici, posizioni, percorso e seed restano identici, quindi
	 * `FRTReactionOpportunityKey` di ciascun watcher — e con essa il suo `OpportunityId` — si ricalcola uguale.
	 *
	 * ⚠️ Entrambi i watcher restano nell'elenco anche quando non armano: `OwnerId` e' un INDICE di unita', e
	 * toglierne uno sposterebbe gli indici degli altri.
	 */
	static FRTTestScenario MakeScenario(bool bArmW1, bool bArmW2)
	{
		const FRTCellId MoverDestination(-1, -1, 0);

		FRTTestScenario Scenario;
		Scenario.ScenarioId = FString::Printf(TEXT("Unit.Reactions.BoundaryInvariants.%s%s"),
			bArmW1 ? TEXT("W1") : TEXT(""), bArmW2 ? TEXT("W2") : TEXT(""));
		// Versione del FORMATO, non del contenuto: la stessa che usa `SecondFireOnDownedTargetLogsNoDamage`,
		// perche' due scenari dello stesso vicinato non devono dichiarare formati diversi.
		Scenario.Version = 2;
		Scenario.Seed = 0;
		Scenario.MapRadius = 5;

		FRTScenarioUnit W1;
		W1.Id = TEXT("W1");
		W1.HeroId = TEXT("Hero.Wraith");
		W1.TeamId = 1;
		W1.Cell = FRTCellId(2, -1, 0);
		W1.Facing = ERTHexDirection::W;
		Scenario.Units.Add(W1);

		FRTScenarioUnit W2;
		W2.Id = TEXT("W2");
		W2.HeroId = TEXT("Hero.Wraith");
		W2.TeamId = 1;
		W2.Cell = FRTCellId(-3, -1, 0);
		W2.Facing = ERTHexDirection::E;
		Scenario.Units.Add(W2);

		// ⚠️ **Salute di roster e non un valore basso**, al contrario di `#1158` che ne aveva bisogno per
		// abbattere il bersaglio al primo colpo. Qui serve l'opposto: il mover deve **sopravvivere a
		// entrambi** i colpi, altrimenti la guardia di `#1158` azzererebbe il danno del secondo. La
		// sopravvivenza non e' assunta: e' un `Expect` qui sotto.
		FRTScenarioUnit M1;
		M1.Id = TEXT("M1");
		M1.HeroId = TEXT("Hero.Gadget");
		M1.TeamId = 0;
		M1.Cell = FRTCellId(-2, 0, 0);
		Scenario.Units.Add(M1);

		FRTScenarioTurn Turn;
		// La finestra live e' una capability: dichiararla e' cio' che distingue un `BLOCKED` onesto da un
		// `Error`, e i test sotto pretendono `Pass` proprio per non leggere zero come successo.
		Turn.Requires.Add(TEXT("DecisionBoundary"));

		if (bArmW1)
		{
			FRTScenarioIntent ArmW1;
			ArmW1.UnitId = TEXT("W1");
			ArmW1.Ability = TEXT("Action.Overwatch");
			Turn.Intents.Add(ArmW1);
		}

		if (bArmW2)
		{
			FRTScenarioIntent ArmW2;
			ArmW2.UnitId = TEXT("W2");
			ArmW2.Ability = TEXT("Action.Overwatch");
			Turn.Intents.Add(ArmW2);
		}

		// Un solo micro-step dentro la zona: due celle darebbero due passi, quindi piu' opportunity, e il
		// conteggio del secondo test smetterebbe di misurare l'annidamento.
		FRTScenarioIntent MoveM1;
		MoveM1.UnitId = TEXT("M1");
		MoveM1.Move.Add(MoverDestination);
		Turn.Intents.Add(MoveM1);

		if (bArmW1)
		{
			FRTScenarioDecision FireW1;
			FireW1.Unit = TEXT("W1");
			FireW1.Respond = TEXT("FIRE");
			FireW1.Target = TEXT("M1");
			Turn.Decisions.Add(FireW1);
		}

		if (bArmW2)
		{
			// Entrambi `FIRE` e non `HOLD`: servono due response che **cambiano stato**. Con due `HOLD` il
			// secondo test sarebbe verde per assenza di stimolo — se il sistema rivalutasse i trigger dopo
			// l'apply, senza un cambiamento di stato non ci sarebbe nulla da rivalutare.
			FRTScenarioDecision FireW2;
			FireW2.Unit = TEXT("W2");
			FireW2.Respond = TEXT("FIRE");
			FireW2.Target = TEXT("M1");
			Turn.Decisions.Add(FireW2);
		}

		Scenario.Turns.Add(Turn);

		// L'harness RIFIUTA uno scenario senza `expect`, ed e' la stessa disciplina che vieta i test vacui.
		// Queste quattro pinnano le PREMESSE del caso, non il suo esito: chi arma un Overwatch non si muove,
		// il mover arriva davvero dove deve, e sopravvive. Se una cade, il test lo dice con la sua causa vera
		// invece di travestirla da violazione dell'invariante.
		FRTTestExpectation W1Stays;
		W1Stays.Kind = ERTAssertionKind::UnitAtCell;
		W1Stays.UnitId = TEXT("W1");
		W1Stays.Cell = FRTCellId(2, -1, 0);
		Scenario.Expect.Add(W1Stays);

		FRTTestExpectation W2Stays;
		W2Stays.Kind = ERTAssertionKind::UnitAtCell;
		W2Stays.UnitId = TEXT("W2");
		W2Stays.Cell = FRTCellId(-3, -1, 0);
		Scenario.Expect.Add(W2Stays);

		FRTTestExpectation MoverArrives;
		MoverArrives.Kind = ERTAssertionKind::UnitAtCell;
		MoverArrives.UnitId = TEXT("M1");
		MoverArrives.Cell = MoverDestination;
		Scenario.Expect.Add(MoverArrives);

		// ⚠️ Se due Overwatch bastassero ad abbattere il mover, la guardia di `#1158` azzererebbe il danno del
		// secondo e la differenza fra le esecuzioni sarebbe LEGITTIMA. Senza questa riga quel giorno
		// arriverebbe travestito da «riscrittura retroattiva», che e' la diagnosi sbagliata.
		FRTTestExpectation MoverSurvives;
		MoverSurvives.Kind = ERTAssertionKind::UnitAlive;
		MoverSurvives.UnitId = TEXT("M1");
		// ⚠️ **`Value` NON e' opzionale qui**: `UnitAlive` lo legge come `bWantAlive = (Value != 0)`, quindi
		// lasciarlo al default `0` chiederebbe che il mover sia **abbattuto** — l'opposto della premessa.
		// Scoperto perche' l'aspettativa e' caduta alla prima esecuzione, che e' il motivo per cui c'e'.
		MoverSurvives.Value = 1;
		Scenario.Expect.Add(MoverSurvives);

		return Scenario;
	}

	/**
	 * Le aspettative CADUTE, con atteso e trovato.
	 *
	 * ⚠️ Senza questo, un `Outcome` di `Fail` arriva come un numero: la prima esecuzione di questo file
	 * riportava «esito 1» e nient'altro, e quale delle quattro `Expect` fosse caduta si e' dovuto cercarlo
	 * nel sorgente dell'harness. Un messaggio che non nomina la sua causa costa un giro a chiunque lo legga.
	 */
	static FString DescribeFailedExpectations(const FRTTestResult& Result)
	{
		TArray<FString> Failed;
		for (const FRTAssertionResult& A : Result.Assertions)
		{
			if (!A.bPassed)
			{
				Failed.Add(FString::Printf(TEXT("%s atteso '%s' trovato '%s'"),
					*A.Description, *A.Expected, *A.Actual));
			}
		}
		return Failed.Num() > 0 ? FString::Join(Failed, TEXT(" · ")) : TEXT("nessuna");
	}

	/** Le voci `ReactionDecision` del TurnLog di questo mondo, nell'ordine in cui sono state committate. */
	static bool CollectReactionEntries(FAutomationTestBase& Test, UWorld* World, const TCHAR* Label,
		TArray<FRTTurnLogEntry>& Out)
	{
		ARTTurnManager* Manager = nullptr;
		for (TActorIterator<ARTTurnManager> It(World); It; ++It)
		{
			Manager = *It;
			break;
		}
		// Senza questo guard un mondo senza TurnManager darebbe zero voci, e i test fallirebbero piu' avanti
		// dicendo «una sola voce ReactionDecision» — cioe' una regressione del sistema di reazione al posto
		// di una fixture rotta.
		if (!Test.TestNotNull(FString::Printf(TEXT("%s: il TurnManager e' nel mondo"), Label), Manager))
		{
			return false;
		}

		Out.Reset();
		for (const FRTTurnLogEntry& Entry : Manager->GetTurnLog())
		{
			if (Entry.Category == ERTLogCategory::ReactionDecision)
			{
				Out.Add(Entry);
			}
		}
		return true;
	}
}

/**
 * *«Una reaction non modifica retroattivamente un evento gia' committed.»*
 *
 * 🔴 **Si confrontano le voci NON-ULTIME, non tutte.** Solo quelle hanno qualcosa dopo di se' che potrebbe
 * riscriverle. L'ultima osserva legittimamente un mondo che le precedenti hanno gia' cambiato: il giorno in
 * cui il danno diventasse dipendente dallo stato — uno scudo da consumare, un bonus all'esecuzione — una
 * differenza lecita verrebbe segnalata come violazione, mandando chi indaga a cercare una riscrittura che
 * non c'e'.
 *
 * ⚠️ Quale watcher finisca in quale posizione lo decidono i cinque tie-break di ADR-0004 §4. Il test non ne
 * fissa nessuno: confronta per `OpportunityId`, quindi resta valido se l'ordine cambia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionCommittedEntryNotRewrittenTest,
	"RefactorTactics.Reactions.CommittedEntriesAreNotRewrittenByALaterResponse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionCommittedEntryNotRewrittenTest::RunTest(const FString&)
{
	using namespace RTBoundaryInvariants;

	// Una esecuzione completa: mondo, run, raccolta delle voci, DISTRUZIONE del mondo.
	//
	// ⚠️ `DestroyWorld` va chiamata su OGNI uscita, e la fixture lo scrive: «un mondo lasciato in piedi tiene
	// vivi gli actor della prova precedente, e due esecuzioni identiche lo sarebbero per il motivo sbagliato».
	// Per un differenziale a tre esecuzioni e' esattamente il confondente che non ci si puo' permettere.
	auto RunAndCollect = [this](bool bArmW1, bool bArmW2, const TCHAR* Label, TArray<FRTTurnLogEntry>& Out) -> bool
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(FString::Printf(TEXT("%s: il mondo di prova esiste"), Label), World))
		{
			return false;
		}
		ON_SCOPE_EXIT { RTWorldFixtures::DestroyWorld(World); };

		const FRTTestResult Result = URTScenarioRunner::Run(World, MakeScenario(bArmW1, bArmW2));

		// ⚠️ `BLOCKED` non e' un successo: uno scenario che dichiara una capability indisponibile esce senza
		// eseguire nulla, e ogni conteggio darebbe zero — cioe' «invariante rispettato» sarebbe
		// indistinguibile da «niente e' successo».
		if (!TestEqual(FString::Printf(
				TEXT("%s: PASS (esito %d · error '%s' · blocked '%s' · cadute: %s)"), Label,
				static_cast<int32>(Result.Outcome), *Result.ErrorMessage, *Result.BlockedReason,
				*DescribeFailedExpectations(Result)),
			static_cast<int32>(Result.Outcome), static_cast<int32>(ERTTestOutcome::Pass)))
		{
			return false;
		}

		if (!CollectReactionEntries(*this, World, Label, Out))
		{
			return false;
		}

		// 🔴 **Le decisioni devono essere `FIRE` VERI, e va verificato invece che assunto.** Se la traduzione
		// del bersaglio degradasse, la response diventerebbe un `HOLD` con `Amount` a zero: le voci ci
		// sarebbero comunque, gli `Expect` reggerebbero, e ogni confronto sotto tornerebbe per BANALITA' —
		// due `HOLD` identici. Il gate resterebbe verde misurando nulla, che e' la vacuita' che questo file
		// esiste per evitare. `#1158` fa la stessa verifica per la stessa ragione.
		for (int32 I = 0; I < Out.Num(); ++I)
		{
			if (!TestTrue(FString::Printf(TEXT("%s: la voce #%d e' un FIRE con un bersaglio, non un HOLD"),
					Label, I),
				URTReactionOpportunityLibrary::FireResponseTarget(Out[I].ReactionResponse) != INDEX_NONE))
			{
				return false;
			}
			if (!TestTrue(FString::Printf(TEXT("%s: la voce #%d dichiara danno (Amount %d)"),
					Label, I, Out[I].Amount), Out[I].Amount > 0))
			{
				return false;
			}
		}
		return true;
	};

	// --- Le due esecuzioni in solitaria: la baseline di ciascun watcher ---------------------------------
	TArray<FRTTurnLogEntry> SoloW1;
	TArray<FRTTurnLogEntry> SoloW2;
	if (!RunAndCollect(true, false, TEXT("solo W1"), SoloW1)) { return false; }
	if (!RunAndCollect(false, true, TEXT("solo W2"), SoloW2)) { return false; }

	if (!TestEqual(TEXT("solo W1: una sola voce ReactionDecision"), SoloW1.Num(), 1)) { return false; }
	if (!TestEqual(TEXT("solo W2: una sola voce ReactionDecision"), SoloW2.Num(), 1)) { return false; }

	TMap<FString, FRTTurnLogEntry> Baseline;
	Baseline.Add(SoloW1[0].OpportunityId, SoloW1[0]);
	Baseline.Add(SoloW2[0].OpportunityId, SoloW2[0]);

	// ⚠️ Senza questa riga una collisione di `OpportunityId` farebbe collassare la mappa a un elemento, ed
	// entrambe le voci verrebbero confrontate con la baseline dello STESSO watcher — un falso verde o un
	// falso rosso, entrambi attribuiti all'invariante sbagliato. E' lo stesso difetto di identita' che il
	// secondo test considera portante.
	if (!TestEqual(TEXT("i due watcher hanno OpportunityId distinti"), Baseline.Num(), 2)) { return false; }

	// --- L'esecuzione a due responder nello stesso boundary ---------------------------------------------
	TArray<FRTTurnLogEntry> Both;
	if (!RunAndCollect(true, true, TEXT("W1+W2"), Both)) { return false; }

	// La premessa del caso. Se cade, non e' l'invariante ad essere violato: e' lo scenario a non descrivere
	// piu' due finestre, e va corretto prima di leggere le righe dopo.
	if (!TestEqual(TEXT("W1+W2: due voci ReactionDecision, una per watcher"), Both.Num(), 2))
	{
		return false;
	}

	// 🔴 IL PUNTO. Il confronto passa da `GoldenEntriesMatch` invece di elencare i campi a mano: l'elenco dei
	// campi discriminanti vive in `MixEntryFields` e comprende anche `TgtCell`, `Phase` e `GraphRevision`,
	// che una lista scritta qui dimenticherebbe. Ed esclude `ReactionInstanceId`, che e' l'`ArmedIndex` e
	// diverge LEGITTIMAMENTE fra solitaria e boundary a due — cioe' esattamente il campo che un confronto
	// ingenuo avrebbe segnalato a torto.
	for (int32 I = 0; I < Both.Num(); ++I)
	{
		const FRTTurnLogEntry& Committed = Both[I];
		const FRTTurnLogEntry* Solo = Baseline.Find(Committed.OpportunityId);

		// L'identita' della finestra e' funzione dello stato: la presenza di un secondo responder non deve
		// spostarla. Vale per OGNI voce, anche l'ultima.
		if (!TestNotNull(*FString::Printf(TEXT("voce #%d: la finestra %s ha una baseline in solitaria"),
				I, *Committed.OpportunityId), Solo))
		{
			continue;
		}

		// L'ULTIMA voce non ha nulla dopo di se' che possa riscriverla, e osserva un mondo che le precedenti
		// hanno gia' cambiato: pretendere che sia identica alla sua solitaria vieterebbe un effetto
		// dipendente dallo stato, che nessuno ha vietato.
		if (I == Both.Num() - 1)
		{
			continue;
		}

		TestTrue(*FString::Printf(
				TEXT("voce #%d (%s): cio' che era gia' committato non e' stato riscritto ")
				TEXT("[solitaria: %s · boundary: %s]"),
				I, *Committed.OpportunityId,
				*URTTurnLogLibrary::DescribeEntry(*Solo), *URTTurnLogLibrary::DescribeEntry(Committed)),
			URTTurnLogLibrary::GoldenEntriesMatch(*Solo, Committed));
	}

	return true;
}

/**
 * *«Nessun Decision Boundary annidato»* — ADR-0004 scarta gli «interrupt annidati» e lo stack LIFO.
 *
 * Osservabile: due watcher armati su un solo micro-step producono ESATTAMENTE due decisioni. Mai tre.
 *
 * ⚠️ Il test rifa' la propria esecuzione invece di riusare quella del primo, ed e' deliberato: due Automation
 * Test che condividono stato falliscono insieme per cause diverse, e il costo di uno scenario in memoria e'
 * irrilevante rispetto al valore di sapere QUALE proprieta' si e' rotta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionNoNestedBoundaryTest,
	"RefactorTactics.Reactions.NoNestedDecisionBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionNoNestedBoundaryTest::RunTest(const FString&)
{
	using namespace RTBoundaryInvariants;

	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World))
	{
		return false;
	}
	ON_SCOPE_EXIT { RTWorldFixtures::DestroyWorld(World); };

	const FRTTestResult Result = URTScenarioRunner::Run(World, MakeScenario(true, true));

	// Early return e non una semplice assertion: su uno scenario `Blocked` le voci sarebbero zero, e il
	// conteggio sotto riporterebbe una regressione del sistema di reazione che non e' avvenuta.
	if (!TestEqual(FString::Printf(
			TEXT("lo scenario e' PASS (esito %d · error '%s' · blocked '%s' · cadute: %s)"),
			static_cast<int32>(Result.Outcome), *Result.ErrorMessage, *Result.BlockedReason,
			*DescribeFailedExpectations(Result)),
		static_cast<int32>(Result.Outcome), static_cast<int32>(ERTTestOutcome::Pass)))
	{
		return false;
	}

	TArray<FRTTurnLogEntry> Entries;
	if (!CollectReactionEntries(*this, World, TEXT("W1+W2"), Entries))
	{
		return false;
	}

	// 🔴 IL PUNTO: i trigger si costruiscono una volta prima del ciclo, quindi due watcher armati danno due
	// decisioni. Una terza voce significherebbe che una response applicata ha generato una nuova opportunity
	// **dentro** il boundary in cui e' stata applicata — cioe' un boundary annidato.
	if (!TestEqual(TEXT("due watcher su un micro-step producono due decisioni, mai tre"), Entries.Num(), 2))
	{
		return false;
	}

	// Le due appartengono a finestre DISTINTE: due voci con lo stesso `OpportunityId` non sarebbero un
	// annidamento ma una collisione di identita', e il test direbbe verde su un difetto diverso.
	TestNotEqual(TEXT("le due decisioni appartengono a due finestre distinte"),
		Entries[0].OpportunityId, Entries[1].OpportunityId);

	// Tutte nello stesso micro-step: se il conteggio tornasse perche' le due decisioni cadono in passi
	// diversi, lo scenario non descriverebbe piu' un boundary solo.
	TestEqual(TEXT("le due decisioni cadono nello stesso micro-step"),
		Entries[0].MicroStepIndex, Entries[1].MicroStepIndex);

	return true;
}
