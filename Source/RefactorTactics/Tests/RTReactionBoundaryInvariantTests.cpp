// Due invarianti del Decision Boundary che oggi sono veri PER COSTRUZIONE, e che nessun gate osservava (`#2461`).
//
// 🔑 **Perche' esistono, ed e' diverso dal solito.** Gli altri invarianti di `ADR-0004` hanno test che
// descrivono un comportamento. Questi due no: descrivono cio' che il codice **non puo' fare**, e lo descrivono
// perche' oggi non puo' farlo per come e' costruito — non perche' qualcuno lo impedisca esplicitamente.
//
//   · invariante 8 — nessun boundary annidato: i `Triggers` si costruiscono UNA volta **prima** del ciclo
//     per-opportunity, il ciclo e' sequenziale e sincrono, e `ApplyReactionDecision` non rientra;
//   · invariante 3 — nessuna riscrittura retroattiva: `TurnLog.Add(Entry)` e' l'UNICO punto di inserimento
//     del suo file, e nessun percorso muta una voce gia' inserita.
//
// ⚠️ **Una costruzione non e' una garanzia finche' nessuno la sorveglia.** Entrambe le proprieta' sparirebbero
// in silenzio se un domani i trigger si ricalcolassero dopo l'apply, o se comparisse un secondo sito di
// inserimento nel log. Questi due test sono la sveglia: non aggiungono comportamento, rendono **falsificabile**
// cio' che oggi e' vero senza che nulla lo dica.
//
// 🔴 **Il primo test e' DIFFERENZIALE, e la forma non e' un vezzo.** Provare «un boundary non ne annida un
// altro» chiedendo al codice di annidarne uno e' impossibile: il flusso non lo consente, e un test che non puo'
// fallire non e' un gate. Si prova invece l'osservabile equivalente — *cio' che un responder ha gia'
// committato non cambia quando un altro responder agisce nello stesso boundary* — confrontando lo
// stesso scenario eseguito con un watcher solo e con entrambi.
//
// ⚠️ Cio' che questi test NON coprono, e va detto: non dimostrano che il boundary sia atomico rispetto al
// resto della resolution, ne' che la revalidation sia mirata. Coprono due proprieta' del ciclo per-opportunity,
// non il boundary come concetto.
//
// ➕ Adiacente e **non** duplicato: `RefactorTactics.Overwatch.SecondFireOnDownedTargetLogsNoDamage` (`#1158`)
// sorveglia una **voce nuova falsa** — un `FIRE` che dichiara un danno mai inflitto. Qui si sorveglia una
// **voce esistente** che non deve cambiare. Sono due difetti diversi dello stesso vicinato.

#include "Misc/AutomationTest.h"

#include "EngineUtils.h"
#include "Map/RTCellId.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/RTTestResult.h"
#include "ScenarioHarness/RTTestScenario.h"
#include "Tests/RTWorldFixtures.h"
#include "Turn/RTReactionOpportunityTypes.h"
#include "Turn/RTTurnLog.h"
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
	 * 🔑 **Chi ARMA e' l'unica variabile fra le esecuzioni**, ed e' cio' che rende il confronto del
	 * primo test una misura invece di una coincidenza: unita', indici, posizioni, percorso e seed restano
	 * identici, quindi `FRTReactionOpportunityKey` di ciascun watcher — e con essa il suo `OpportunityId` —
	 * si ricalcola uguale. Cambiare anche solo l'ordine di `Scenario.Units` invaliderebbe il confronto senza
	 * che nulla lo segnali.
	 *
	 * ⚠️ Entrambi i watcher restano nell'elenco anche quando non armano: `OwnerId` e' un INDICE di
	 * unita', e toglierne uno sposterebbe gli indici degli altri.
	 *
	 * ⚠️ **`M1` usa la salute di roster e non un valore basso**, al contrario di `#1158` che ne aveva bisogno
	 * per abbattere il bersaglio al primo colpo. Qui serve l'opposto: il mover deve **sopravvivere a entrambi**
	 * i colpi, altrimenti il secondo `FIRE` troverebbe un bersaglio a terra e la guardia di `#1158` gli
	 * azzererebbe il danno — e la differenza fra le due esecuzioni sarebbe legittima invece che un difetto,
	 * cioe' il test fallirebbe dicendo il falso.
	 */
	static FRTTestScenario MakeScenario(bool bArmW1, bool bArmW2)
	{
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

		// W2 entra nell'elenco in ENTRAMBE le esecuzioni anche quando non arma: gli indici di unita' devono
		// restare gli stessi, perche' `OwnerId` di `FRTReactionOpportunityKey` e' un indice.
		FRTScenarioUnit W2;
		W2.Id = TEXT("W2");
		W2.HeroId = TEXT("Hero.Wraith");
		W2.TeamId = 1;
		W2.Cell = FRTCellId(-3, -1, 0);
		W2.Facing = ERTHexDirection::E;
		Scenario.Units.Add(W2);

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
		MoveM1.Move.Add(FRTCellId(-1, -1, 0));
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

		// L'harness RIFIUTA uno scenario senza `expect`. Le due qui dicono qualcosa di vero e non banale: chi
		// arma un Overwatch spende l'azione principale e non si muove.
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

		return Scenario;
	}

	/** Le voci `ReactionDecision` del TurnLog, nell'ordine in cui sono state committate. */
	static TArray<FRTTurnLogEntry> ReactionEntriesOf(UWorld* World)
	{
		TArray<FRTTurnLogEntry> Out;
		ARTTurnManager* Manager = nullptr;
		for (TActorIterator<ARTTurnManager> It(World); It; ++It)
		{
			Manager = *It;
			break;
		}
		if (!Manager)
		{
			return Out;
		}
		for (const FRTTurnLogEntry& Entry : Manager->GetTurnLog())
		{
			if (Entry.Category == ERTLogCategory::ReactionDecision)
			{
				Out.Add(Entry);
			}
		}
		return Out;
	}
}

/**
 * Invariante 3 di `ADR-0004` — *«una reaction non modifica retroattivamente un evento gia' committed»*.
 *
 * 🔴 **Si confrontano TUTTE le voci del boundary, non una scelta a priori.** Solo le voci non-ultime sono
 * esposte a una riscrittura retroattiva: dopo l'ultima non viene committato piu' nulla che possa toccarla.
 * Quale watcher finisca in quale posizione lo decidono i cinque tie-break di `ADR-0004 §4`, e un test che
 * ne fissasse uno diventerebbe vacuo il giorno in cui l'ordine cambia — senza che nulla lo segnali.
 *
 * ⚠️ **La prima stesura confrontava la sola voce di W1**, ed e' stata sostituita dopo che la prova
 * anti-vacuity l'ha trovata insufficiente. Confrontarle tutte toglie la dipendenza dall'ordine invece di
 * assumerlo: e' cio' che rende questo gate indipendente da una decisione che non gli appartiene.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionCommittedEntryNotRewrittenTest,
	"RefactorTactics.Reactions.CommittedEntriesAreNotRewrittenByALaterResponse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionCommittedEntryNotRewrittenTest::RunTest(const FString&)
{
	using namespace RTBoundaryInvariants;

	// Una esecuzione dello scenario, con il guard che impedisce di leggere `BLOCKED` come un successo.
	auto RunAndCollect = [this](bool bArmW1, bool bArmW2, const TCHAR* Label, TArray<FRTTurnLogEntry>& Out) -> bool
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(FString::Printf(TEXT("%s: il mondo di prova esiste"), Label), World))
		{
			return false;
		}
		const FRTTestResult Result = URTScenarioRunner::Run(World, MakeScenario(bArmW1, bArmW2));

		// ⚠️ `BLOCKED` non e' un successo: uno scenario che dichiara una capability indisponibile esce senza
		// eseguire nulla, e ogni conteggio darebbe zero — cioe' «invariante rispettato» sarebbe
		// indistinguibile da «niente e' successo».
		if (!TestEqual(FString::Printf(TEXT("%s: PASS (esito %d · error '%s' · blocked '%s')"), Label,
				static_cast<int32>(Result.Outcome), *Result.ErrorMessage, *Result.BlockedReason),
			static_cast<int32>(Result.Outcome), static_cast<int32>(ERTTestOutcome::Pass)))
		{
			return false;
		}
		Out = ReactionEntriesOf(World);
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

	// --- L'esecuzione a due responder nello stesso boundary ---------------------------------------------
	TArray<FRTTurnLogEntry> Both;
	if (!RunAndCollect(true, true, TEXT("W1+W2"), Both)) { return false; }

	// La premessa del caso. Se cade, non e' l'invariante ad essere violato: e' lo scenario a non descrivere
	// piu' due finestre, e va corretto prima di leggere le righe dopo.
	if (!TestEqual(TEXT("W1+W2: due voci ReactionDecision, una per watcher"), Both.Num(), 2))
	{
		return false;
	}

	// 🔴 IL PUNTO: **ogni** voce, non solo l'ultima. La prima committata e' quella che una response
	// successiva potrebbe riscrivere, e includerla e' cio' che distingue questo test dalla sua prima
	// stesura vacua.
	for (int32 I = 0; I < Both.Num(); ++I)
	{
		const FRTTurnLogEntry& Committed = Both[I];
		const FRTTurnLogEntry* Solo = Baseline.Find(Committed.OpportunityId);

		// Che l'`OpportunityId` si ritrovi identico e' gia' meta' della misura: e' l'identita' della finestra,
		// funzione dello stato, e la presenza di un secondo responder non deve spostarla.
		if (!TestNotNull(*FString::Printf(TEXT("voce #%d: la finestra %s ha una baseline in solitaria"),
				I, *Committed.OpportunityId), Solo))
		{
			continue;
		}

		const FString Where = FString::Printf(TEXT("voce #%d (%s)"), I, *Committed.OpportunityId);
		TestEqual(*(Where + TEXT(": l'esito committato non cambia")),
			static_cast<int32>(Committed.Outcome), static_cast<int32>(Solo->Outcome));
		TestEqual(*(Where + TEXT(": la risposta committata non cambia")),
			Committed.ReactionResponse, Solo->ReactionResponse);
		TestEqual(*(Where + TEXT(": il bersaglio committato non cambia")),
			Committed.SelectedTargetUnitId, Solo->SelectedTargetUnitId);
		TestEqual(*(Where + TEXT(": l'azione committata non cambia")),
			Committed.ActionId, Solo->ActionId);
		TestTrue(*(Where + TEXT(": la cella sorgente committata non cambia")),
			Committed.SrcCell == Solo->SrcCell);

		// ⚠️ `Amount` e' il campo che `#1158` ha dimostrato scrivibile a sproposito, ed e' quindi quello su cui
		// la riscrittura retroattiva farebbe piu' danno: un danno alterato dopo il commit e' un TurnLog che
		// mente.
		TestEqual(*(Where + TEXT(": il danno committato non cambia")), Committed.Amount, Solo->Amount);
	}

	return true;
}

/**
 * Invariante 8 di `ADR-0004` — *«nessun nested Decision Boundary nell'MVP»*.
 *
 * Osservabile: due watcher armati su un solo micro-step producono ESATTAMENTE due decisioni. Mai tre.
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
	const FRTTestResult Result = URTScenarioRunner::Run(World, MakeScenario(true, true));

	TestEqual(FString::Printf(TEXT("lo scenario e' PASS (esito %d · error '%s' · blocked '%s')"),
			static_cast<int32>(Result.Outcome), *Result.ErrorMessage, *Result.BlockedReason),
		static_cast<int32>(Result.Outcome), static_cast<int32>(ERTTestOutcome::Pass));

	const TArray<FRTTurnLogEntry> Entries = ReactionEntriesOf(World);

	// 🔴 IL PUNTO: i trigger si costruiscono una volta prima del ciclo, quindi due watcher armati danno due
	// decisioni. Una terza voce significherebbe che una response applicata ha generato una nuova opportunity
	// **dentro** il boundary in cui e' stata applicata — cioe' un boundary annidato.
	TestEqual(TEXT("due watcher su un micro-step producono due decisioni, mai tre"), Entries.Num(), 2);

	// Le due appartengono a finestre DISTINTE: due voci con lo stesso `OpportunityId` non sarebbero un
	// annidamento ma una collisione di identita', e il test direbbe verde su un difetto diverso.
	if (Entries.Num() == 2)
	{
		TestNotEqual(TEXT("le due decisioni appartengono a due finestre distinte"),
			Entries[0].OpportunityId, Entries[1].OpportunityId);

		// Tutte nello stesso micro-step: se il conteggio tornasse perche' le due decisioni cadono in passi
		// diversi, lo scenario non descriverebbe piu' un boundary solo.
		TestEqual(TEXT("le due decisioni cadono nello stesso micro-step"),
			Entries[0].MicroStepIndex, Entries[1].MicroStepIndex);
	}

	return true;
}
