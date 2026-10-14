#pragma once

#include "CoreMinimal.h"
#include "ScenarioHarness/RTTestScenario.h"

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
}

#endif // WITH_DEV_AUTOMATION_TESTS
