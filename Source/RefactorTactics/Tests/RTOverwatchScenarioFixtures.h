#pragma once

#include "CoreMinimal.h"
#include "ScenarioHarness/RTTestScenario.h"
#include "Turn/RTReactionOpportunityTypes.h" // [D-522]: l'id della condizione si prende dalla funzione che lo scrive

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Scenari dell'Overwatch condivisi fra piu' file di test.
 *
 * Esiste perche' lo stesso montaggio serve a due domande diverse: `Overwatch.SecondWatcherHoldsOnADownedTarget` lo
 * gioca e legge la traccia, `Replay.Verifier.DownedTargetCollapseIgnoresTheTrace` lo ri-simula. Due copie dello
 * stesso scenario divergerebbero alla prima modifica, e i due test smetterebbero di parlare dello stesso caso.
 */
namespace RTOverwatchScenarioFixtures
{
	/**
	 * [D-518] (`#3745`) Due Ivrin guardano la STESSA cella da lati opposti, e un Aevik con pochi punti vita ci entra
	 * con un passo solo. Il primo `FIRE` lo abbatte; al secondo watcher quel `FIRE` non si offre.
	 *
	 * `MakeSuppressiveZone` costruisce una LINEA lungo il facing, quindi (-1,-1,0) cade nella zona di entrambi:
	 * sono due WATCHER diversi sullo stesso bersaglio, quindi due opportunity, non due bersagli nello stesso passo
	 * (`Overwatch.SimultaneousTargetsSingleOpportunity`).
	 *
	 * ⚠️ `Health` basso E DICHIARATO: serve che il PRIMO colpo abbatta il bersaglio. Col valore di roster il mover
	 * sopravvivrebbe, e il secondo `FIRE` resterebbe legittimo.
	 *
	 * ⚠️ **Una decisione sola, di W2**: e' lui il primo a essere chiesto, nell'ordine canonico dei trigger (ADR-0004
	 * §4) — misurato: con la decisione su W1 l'harness rispondeva *«finestra aperta per 'W2' senza una decisione che la
	 * nomini»*. Il secondo, W1, non viene chiesto, e una decisione scritta per lui non avrebbe una finestra che la
	 * consuma: l'harness la chiamerebbe errore.
	 */
	inline FRTTestScenario MakeSecondWatcherOnDownedTargetScenario()
	{
		FRTTestScenario Scenario;
		Scenario.ScenarioId = TEXT("Unit.Overwatch.SecondWatcherOnDownedTarget");
		Scenario.Version = 2;
		Scenario.Seed = 0;
		Scenario.MapRadius = 5;

		FRTScenarioUnit W1;
		W1.Id = TEXT("W1");
		W1.HeroId = TEXT("Hero.Ivrin");
		W1.TeamId = 1;
		W1.Cell = FRTCellId(2, -1, 0);
		W1.Facing = ERTHexDirection::W;
		Scenario.Units.Add(W1);

		FRTScenarioUnit W2;
		W2.Id = TEXT("W2");
		W2.HeroId = TEXT("Hero.Ivrin");
		W2.TeamId = 1;
		W2.Cell = FRTCellId(-3, -1, 0);
		W2.Facing = ERTHexDirection::E;
		Scenario.Units.Add(W2);

		FRTScenarioUnit M1;
		M1.Id = TEXT("M1");
		M1.HeroId = TEXT("Hero.Aevik");
		M1.TeamId = 0;
		M1.Cell = FRTCellId(-2, 0, 0);
		M1.Health = 10;
		Scenario.Units.Add(M1);

		FRTScenarioTurn Turn;
		// La finestra live e' una capability: dichiararla distingue un `BLOCKED` onesto da un `Error`.
		Turn.Requires.Add(TEXT("DecisionBoundary"));

		FRTScenarioIntent ArmW1;
		ArmW1.UnitId = TEXT("W1");
		ArmW1.Ability = TEXT("Action.Overwatch");
		Turn.Intents.Add(ArmW1);

		FRTScenarioIntent ArmW2;
		ArmW2.UnitId = TEXT("W2");
		ArmW2.Ability = TEXT("Action.Overwatch");
		Turn.Intents.Add(ArmW2);

		FRTScenarioIntent MoveM1;
		MoveM1.UnitId = TEXT("M1");
		MoveM1.Move.Add(FRTCellId(-1, -1, 0));
		Turn.Intents.Add(MoveM1);

		FRTScenarioDecision FireW2;
		FireW2.Unit = TEXT("W2");
		FireW2.Respond = TEXT("FIRE");
		FireW2.Target = TEXT("M1");
		Turn.Decisions.Add(FireW2);

		Scenario.Turns.Add(Turn);

		// L'harness RIFIUTA uno scenario senza `expect`. Chi arma un Overwatch spende l'azione principale e non si
		// muove: i watcher devono trovarsi dove sono stati posati.
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

	/**
	 * [D-522] (`#3771`) Lo stesso montaggio di `MakeSecondWatcherOnDownedTargetScenario`, con due differenze: Aevik entra
	 * con 60/90 PV, e W1 dichiara «spara solo se il bersaglio e' al 50% dei PV o meno».
	 *
	 * All'ingresso 60/90 (66%) non soddisfa la condizione. W2, che e' chiesto per primo e non ne ha, spara: 21 danni
	 * dell'arma di Ivrin portano Aevik a 39/90 (43%), vivo e sotto la soglia. Quando arriva il turno di W1 la condizione
	 * e' soddisfatta, e la sua finestra si apre. Prima di [D-522] si chiudeva con `CollapsedByCondition`, sullo stato di
	 * prima del colpo.
	 *
	 * ⚠️ **Le decisioni sono DUE, ed e' la meta' discriminante.** La seconda, di W1, ha una finestra che la consuma solo
	 * se la condizione si legge al momento della domanda; altrimenti l'harness la chiama decisione orfana, e lo
	 * scenario esce `Error`.
	 *
	 * ⚠️ **W1 arma anche `Hero.Ivrin.Deflection`**: `ARTUnit::SetPlannedReactionCondition` rifiuta una condizione senza
	 * lo slot reazione armato, e il loader lo dice (vedi `FRTScenarioIntent::Condition`). E' lo stesso montaggio di
	 * `Spec.Overwatch.ConditionCollapsesToHold`. L'ordine dei watcher non cambia: i tie-break vengono dall'azione
	 * `Action.Overwatch` e dallo `StableUnitId`, non dallo slot reazione.
	 */
	inline FRTTestScenario MakeConditionMetAfterFirstShotScenario()
	{
		FRTTestScenario Scenario = MakeSecondWatcherOnDownedTargetScenario();
		Scenario.ScenarioId = TEXT("Unit.Overwatch.ConditionMetAfterFirstShot");

		for (FRTScenarioUnit& Unit : Scenario.Units)
		{
			if (Unit.Id == TEXT("M1"))
			{
				Unit.Health = 60;
			}
		}

		FRTScenarioTurn& Turn = Scenario.Turns[0];
		Turn.Requires.Add(TEXT("Reaction"));
		for (FRTScenarioIntent& Intent : Turn.Intents)
		{
			if (Intent.UnitId == TEXT("W1"))
			{
				Intent.Reaction = TEXT("Hero.Ivrin.Deflection");
				Intent.Condition = FRTDeclaredCondition(URTReactionOpportunityLibrary::TargetHealthAtOrBelowPercent(), 50);
			}
		}

		FRTScenarioDecision FireW1;
		FireW1.Unit = TEXT("W1");
		FireW1.Respond = TEXT("FIRE");
		FireW1.Target = TEXT("M1");
		Turn.Decisions.Add(FireW1);

		return Scenario;
	}

	/**
	 * [D-522] (`#3771`) Il cambio di ETICHETTA che D-522 §4 dichiara: un bersaglio escluso all'ingresso dalla condizione,
	 * e poi abbattuto dal watcher chiesto prima.
	 *
	 * Lo stesso montaggio di `MakeSecondWatcherOnDownedTargetScenario` — Aevik con 10 PV, W2 che spara per primo e lo
	 * abbatte — piu' una condizione su W1: «al 10% dei PV o meno». 10/90 e' l'11%, quindi all'ingresso W1 non ha un
	 * `FIRE`. Prima di [D-522] il pompaggio non toglieva niente, e la voce era `CollapsedByCondition`; ora nessuno
	 * dei bersagli e' in piedi, e la voce e' `CollapsedTargetDown`.
	 *
	 * Una decisione sola, di W2, come nel montaggio di [D-518]: la finestra di W1 non si apre in nessuna delle due regole.
	 */
	inline FRTTestScenario MakeConditionExcludedThenDownedScenario()
	{
		FRTTestScenario Scenario = MakeSecondWatcherOnDownedTargetScenario();
		Scenario.ScenarioId = TEXT("Unit.Overwatch.ConditionExcludedThenDowned");

		FRTScenarioTurn& Turn = Scenario.Turns[0];
		Turn.Requires.Add(TEXT("Reaction"));
		for (FRTScenarioIntent& Intent : Turn.Intents)
		{
			if (Intent.UnitId == TEXT("W1"))
			{
				Intent.Reaction = TEXT("Hero.Ivrin.Deflection");
				Intent.Condition = FRTDeclaredCondition(URTReactionOpportunityLibrary::TargetHealthAtOrBelowPercent(), 10);
			}
		}
		return Scenario;
	}

	/**
	 * [D-349] Un Overwatch armato sul BORDO della mappa, rivolto verso fuori: la sua linea non contiene nessuna cella.
	 * Un Aevik nemico cammina per due passi altrove.
	 *
	 * La voce attesa e' una sola, `ZoneEmpty`, anche se la linea si rifa' a due micro-step: al piu' una voce per
	 * Overwatch e per motivo in un turno. `(5,-2,0)` sta sul bordo di una mappa di raggio 5, e la cella a est, `(6,-2,0)`,
	 * e' fuori: e' quella che la voce nomina.
	 */
	inline FRTTestScenario MakeLineEmptyAtTheEdgeScenario()
	{
		FRTTestScenario Scenario;
		Scenario.ScenarioId = TEXT("Unit.Overwatch.LineEmptyAtTheEdge");
		Scenario.Version = 2;
		Scenario.Seed = 0;
		Scenario.MapRadius = 5;

		FRTScenarioUnit W;
		W.Id = TEXT("W");
		W.HeroId = TEXT("Hero.Ivrin");
		W.TeamId = 1;
		W.Cell = FRTCellId(5, -2, 0);
		W.Facing = ERTHexDirection::E;
		Scenario.Units.Add(W);

		FRTScenarioUnit M1;
		M1.Id = TEXT("M1");
		M1.HeroId = TEXT("Hero.Aevik");
		M1.TeamId = 0;
		M1.Cell = FRTCellId(-2, 0, 0);
		Scenario.Units.Add(M1);

		FRTScenarioTurn Turn;
		FRTScenarioIntent ArmW;
		ArmW.UnitId = TEXT("W");
		ArmW.Ability = TEXT("Action.Overwatch");
		Turn.Intents.Add(ArmW);

		FRTScenarioIntent MoveM1;
		MoveM1.UnitId = TEXT("M1");
		MoveM1.Move = { FRTCellId(-1, 0, 0), FRTCellId(0, 0, 0) };
		Turn.Intents.Add(MoveM1);
		Scenario.Turns.Add(Turn);

		FRTTestExpectation WStays;
		WStays.Kind = ERTAssertionKind::UnitAtCell;
		WStays.UnitId = TEXT("W");
		WStays.Cell = FRTCellId(5, -2, 0);
		Scenario.Expect.Add(WStays);

		FRTTestExpectation M1Arrives;
		M1Arrives.Kind = ERTAssertionKind::UnitAtCell;
		M1Arrives.UnitId = TEXT("M1");
		M1Arrives.Cell = FRTCellId(0, 0, 0);
		Scenario.Expect.Add(M1Arrives);
		return Scenario;
	}

	/**
	 * [D-349] Il watcher CAMMINA fino al bordo durante il Move, con l'Overwatch armato verso est: la linea, rifatta dalla
	 * cella attuale (ADR-0004 §6), si svuota quando arriva in `(5,-2,0)`.
	 *
	 * 🔑 **E' il caso che distingue la cella del FATTO da quella di partenza** (`#2142`): `ARTUnit::Cell` la scrive
	 * `PlaceOnCell` dopo il ciclo dei micro-step, quindi durante il passo l'Actor dice ancora `(4,-2,0)`. Un soggetto di
	 * log costruito dall'Actor metterebbe nell'audit la cella sbagliata, e negli altri scenari — watcher fermi — le due
	 * coincidono e nessuno se ne accorgerebbe.
	 */
	inline FRTTestScenario MakeLineEmptiesWhenTheWatcherWalksToTheEdgeScenario()
	{
		FRTTestScenario Scenario;
		Scenario.ScenarioId = TEXT("Unit.Overwatch.LineEmptiesAtTheEdge");
		Scenario.Version = 2;
		Scenario.Seed = 0;
		Scenario.MapRadius = 5;

		FRTScenarioUnit W;
		W.Id = TEXT("W");
		W.HeroId = TEXT("Hero.Ivrin");
		W.TeamId = 1;
		W.Cell = FRTCellId(4, -2, 0);
		W.Facing = ERTHexDirection::E;
		Scenario.Units.Add(W);

		FRTScenarioUnit M1;
		M1.Id = TEXT("M1");
		M1.HeroId = TEXT("Hero.Aevik");
		M1.TeamId = 0;
		M1.Cell = FRTCellId(-2, 0, 0);
		Scenario.Units.Add(M1);

		FRTScenarioTurn Turn;
		FRTScenarioIntent ArmW;
		ArmW.UnitId = TEXT("W");
		ArmW.Ability = TEXT("Action.Overwatch");
		ArmW.Move = { FRTCellId(5, -2, 0) };
		Turn.Intents.Add(ArmW);
		Scenario.Turns.Add(Turn);

		FRTTestExpectation WArrives;
		WArrives.Kind = ERTAssertionKind::UnitAtCell;
		WArrives.UnitId = TEXT("W");
		WArrives.Cell = FRTCellId(5, -2, 0);
		Scenario.Expect.Add(WArrives);
		return Scenario;
	}

	/**
	 * [D-349] Un Overwatch spara, e DOPO un secondo nemico rilevato attraversa la sua linea: `ChargeSpent`.
	 *
	 * W guarda verso est da `(-3,-1,0)`: la linea e' `(-2,-1)` … `(1,-1)`. Al primo passo M1 entra in `(-1,-1,0)` e W
	 * risponde `FIRE` (21 danni: M1 resta in piedi). M2 parte da `(1,0,0)`: al primo passo e' in `(0,0,0)`, fuori dalla
	 * linea; al secondo entra in `(0,-1,0)` e al terzo in `(1,-1,0)`, entrambe nella linea, a 3 e 4 celle da W, nel suo
	 * arco e dentro la sua vista. La voce attesa e' una sola, sulla prima cella: la regola e' al piu' una per motivo.
	 */
	inline FRTTestScenario MakeChargeSpentThenAnotherEntersScenario()
	{
		FRTTestScenario Scenario;
		Scenario.ScenarioId = TEXT("Unit.Overwatch.ChargeSpentThenAnotherEnters");
		Scenario.Version = 2;
		Scenario.Seed = 0;
		Scenario.MapRadius = 5;

		FRTScenarioUnit W;
		W.Id = TEXT("W");
		W.HeroId = TEXT("Hero.Ivrin");
		W.TeamId = 1;
		W.Cell = FRTCellId(-3, -1, 0);
		W.Facing = ERTHexDirection::E;
		Scenario.Units.Add(W);

		FRTScenarioUnit M1;
		M1.Id = TEXT("M1");
		M1.HeroId = TEXT("Hero.Aevik");
		M1.TeamId = 0;
		M1.Cell = FRTCellId(-2, 0, 0);
		Scenario.Units.Add(M1);

		FRTScenarioUnit M2;
		M2.Id = TEXT("M2");
		M2.HeroId = TEXT("Hero.Aevik");
		M2.TeamId = 0;
		M2.Cell = FRTCellId(1, 0, 0);
		Scenario.Units.Add(M2);

		FRTScenarioTurn Turn;
		Turn.Requires.Add(TEXT("DecisionBoundary"));

		FRTScenarioIntent ArmW;
		ArmW.UnitId = TEXT("W");
		ArmW.Ability = TEXT("Action.Overwatch");
		Turn.Intents.Add(ArmW);

		FRTScenarioIntent MoveM1;
		MoveM1.UnitId = TEXT("M1");
		MoveM1.Move = { FRTCellId(-1, -1, 0) };
		Turn.Intents.Add(MoveM1);

		FRTScenarioIntent MoveM2;
		MoveM2.UnitId = TEXT("M2");
		MoveM2.Move = { FRTCellId(0, 0, 0), FRTCellId(0, -1, 0), FRTCellId(1, -1, 0) };
		Turn.Intents.Add(MoveM2);

		FRTScenarioDecision FireW;
		FireW.Unit = TEXT("W");
		FireW.Respond = TEXT("FIRE");
		FireW.Target = TEXT("M1");
		Turn.Decisions.Add(FireW);
		Scenario.Turns.Add(Turn);

		FRTTestExpectation WStays;
		WStays.Kind = ERTAssertionKind::UnitAtCell;
		WStays.UnitId = TEXT("W");
		WStays.Cell = FRTCellId(-3, -1, 0);
		Scenario.Expect.Add(WStays);

		FRTTestExpectation M2Arrives;
		M2Arrives.Kind = ERTAssertionKind::UnitAtCell;
		M2Arrives.UnitId = TEXT("M2");
		M2Arrives.Cell = FRTCellId(1, -1, 0);
		Scenario.Expect.Add(M2Arrives);
		return Scenario;
	}
}

#endif // WITH_DEV_AUTOMATION_TESTS
