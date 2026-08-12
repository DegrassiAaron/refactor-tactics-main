#include "Misc/AutomationTest.h"
#include "Ability/RTActionDef.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTPlanValidationLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Unita' viva con un budget di movimento dichiarato: e' tutto cio' che la validazione legge dallo snapshot. */
	FRTHexSimUnit PlanUnit(int32 MoveBudget = 5)
	{
		return FRTHexSimUnit(0, FRTCellId(0, 0), MoveBudget, /*bAlive*/ true);
	}

	/** Voce di piano minima: l'azione, lo slot che occupa, quanto costa in MP e il cooldown residuo. */
	FRTPlannedAction PlanEntry(const FName& ActionId, ERTActionSlot Slot, int32 CostMP = 0,
		int32 CooldownRemaining = 0)
	{
		FRTPlannedAction Entry;
		Entry.Def.ActionId = ActionId;
		Entry.Def.Slot = Slot;
		Entry.Def.CostMP = CostMP;
		Entry.CooldownRemaining = CooldownRemaining;
		return Entry;
	}
}

/**
 * Il piano canonico di D-028: un movimento, una principale, una reazione. La reazione e' indipendente —
 * chi si muove E agisce la conserva comunque — ed e' il caso che un modello a budget numerico renderebbe
 * una questione di totale invece che di struttura.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanMovementMainAndReactionAreLegal,
	"RefactorTactics.Plan.MovementMainAndReactionAreLegal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanMovementMainAndReactionAreLegal::RunTest(const FString&)
{
	const TArray<FRTPlannedAction> Plan = {
		PlanEntry(TEXT("Action.Move"), ERTActionSlot::Movement, /*CostMP*/ 5),
		PlanEntry(TEXT("Action.BasicAttack"), ERTActionSlot::Main),
		PlanEntry(TEXT("Action.Counter"), ERTActionSlot::Reaction),
	};

	const FRTPlanValidation Result = URTPlanValidationLibrary::ValidatePlan(PlanUnit(5), Plan);

	TestTrue(TEXT("il piano canonico e' legale"), Result.bLegal);
	TestEqual(TEXT("nessun motivo di rifiuto"), Result.Reason, ERTActionInvalidReason::None);
	return true;
}

/** Due principali non si sommano: la legalita' e' STRUTTURALE (D-028, confermata da D-114). */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanSecondMainIsRejected,
	"RefactorTactics.Plan.SecondMainIsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanSecondMainIsRejected::RunTest(const FString&)
{
	const TArray<FRTPlannedAction> Plan = {
		PlanEntry(TEXT("Action.BasicAttack"), ERTActionSlot::Main),
		PlanEntry(TEXT("Action.Guard"), ERTActionSlot::Main),
	};

	const FRTPlanValidation Result = URTPlanValidationLibrary::ValidatePlan(PlanUnit(), Plan);

	TestFalse(TEXT("due principali non sono legali"), Result.bLegal);
	TestEqual(TEXT("il motivo e' lo slot occupato"), Result.Reason, ERTActionInvalidReason::SlotOccupied);
	TestEqual(TEXT("l'azione respinta e' la seconda"), Result.OffendingActionId, FName(TEXT("Action.Guard")));
	return true;
}

/**
 * `Sprint` occupa ENTRAMBI gli slot, quindi nessuna principale le sta accanto.
 *
 * E' anche la forma corretta del vincolo che il kit d'autore proponeva come regola a se'
 * (`OverwatchDisallowsVoluntaryDash`): D-070 dichiara che quel divieto non serve — chi riserva lo slot
 * movimento non VIETA lo scatto, semplicemente non ha piu' lo slot. Il motivo dice «slot occupato».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanSprintLeavesNoRoomForMain,
	"RefactorTactics.Plan.SprintLeavesNoRoomForMain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanSprintLeavesNoRoomForMain::RunTest(const FString&)
{
	const TArray<FRTPlannedAction> Plan = {
		PlanEntry(TEXT("Action.Sprint"), ERTActionSlot::MovementAndMain, /*CostMP*/ 8),
		PlanEntry(TEXT("Action.BasicAttack"), ERTActionSlot::Main),
	};

	const FRTPlanValidation Result = URTPlanValidationLibrary::ValidatePlan(PlanUnit(8), Plan);

	TestFalse(TEXT("Sprint + principale non e' legale"), Result.bLegal);
	TestEqual(TEXT("il motivo e' lo slot occupato"), Result.Reason, ERTActionInvalidReason::SlotOccupied);
	TestEqual(TEXT("l'azione respinta e' l'attacco"), Result.OffendingActionId,
		FName(TEXT("Action.BasicAttack")));
	return true;
}

/** `Wait` non occupa slot: resta osservabile nel TurnLog senza togliere nulla al piano. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanWaitOccupiesNoSlot,
	"RefactorTactics.Plan.WaitOccupiesNoSlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanWaitOccupiesNoSlot::RunTest(const FString&)
{
	const TArray<FRTPlannedAction> Plan = {
		PlanEntry(TEXT("Action.Wait"), ERTActionSlot::None),
		PlanEntry(TEXT("Action.Move"), ERTActionSlot::Movement, /*CostMP*/ 3),
		PlanEntry(TEXT("Action.BasicAttack"), ERTActionSlot::Main),
	};

	const FRTPlanValidation Result = URTPlanValidationLibrary::ValidatePlan(PlanUnit(5), Plan);

	TestTrue(TEXT("Wait non consuma nulla"), Result.bLegal);
	return true;
}

/** Il costo in MP si somma e si confronta col budget dell'unita': sforare e' illegale in Planning. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanMovementPointsOverflowIsRejected,
	"RefactorTactics.Plan.MovementPointsOverflowIsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanMovementPointsOverflowIsRejected::RunTest(const FString&)
{
	const TArray<FRTPlannedAction> Plan = {
		PlanEntry(TEXT("Action.Move"), ERTActionSlot::Movement, /*CostMP*/ 5),
	};

	const FRTPlanValidation Result = URTPlanValidationLibrary::ValidatePlan(PlanUnit(/*MoveBudget*/ 4), Plan);

	TestFalse(TEXT("5 MP su un budget di 4 non e' legale"), Result.bLegal);
	TestEqual(TEXT("il motivo sono i punti movimento"), Result.Reason,
		ERTActionInvalidReason::InsufficientMovementPoints);
	return true;
}

/** Il confine: spendere ESATTAMENTE il budget e' legale. E' il caso che un `<` al posto di `<=` rompe. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanMovementPointsExactlyOnBudgetIsLegal,
	"RefactorTactics.Plan.MovementPointsExactlyOnBudgetIsLegal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanMovementPointsExactlyOnBudgetIsLegal::RunTest(const FString&)
{
	const TArray<FRTPlannedAction> Plan = {
		PlanEntry(TEXT("Action.Move"), ERTActionSlot::Movement, /*CostMP*/ 5),
	};

	const FRTPlanValidation Result = URTPlanValidationLibrary::ValidatePlan(PlanUnit(/*MoveBudget*/ 5), Plan);

	TestTrue(TEXT("spendere tutto il budget e' legale"), Result.bLegal);
	return true;
}

/**
 * Il cooldown rifiuta un'azione mentre lo slot e' LIBERO: e' il caso che distingue un motivo dall'altro,
 * ed e' il buco dichiarato di `RT-FEAT-ACTION-COOLDOWNS` (scenario `CooldownBlocksWithSlotFree`).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanCooldownBlocksWithSlotFree,
	"RefactorTactics.Plan.CooldownBlocksWithSlotFree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanCooldownBlocksWithSlotFree::RunTest(const FString&)
{
	const TArray<FRTPlannedAction> Plan = {
		PlanEntry(TEXT("Action.Overcharge"), ERTActionSlot::Main, /*CostMP*/ 0, /*CooldownRemaining*/ 1),
	};

	const FRTPlanValidation Result = URTPlanValidationLibrary::ValidatePlan(PlanUnit(), Plan);

	TestFalse(TEXT("un'abilita' in cooldown non e' pianificabile"), Result.bLegal);
	TestEqual(TEXT("il motivo e' il cooldown, non lo slot"), Result.Reason,
		ERTActionInvalidReason::OnCooldown);
	return true;
}

/**
 * Invarianza per permutazione: l'ordine in cui il giocatore compone il piano non cambia il verdetto.
 * Senza questa proprieta' la validazione dipenderebbe dall'ordine di inserimento, che e' presentazione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanVerdictIsPermutationInvariant,
	"RefactorTactics.Plan.VerdictIsPermutationInvariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanVerdictIsPermutationInvariant::RunTest(const FString&)
{
	TArray<FRTPlannedAction> Plan = {
		PlanEntry(TEXT("Action.Move"), ERTActionSlot::Movement, /*CostMP*/ 3),
		PlanEntry(TEXT("Action.BasicAttack"), ERTActionSlot::Main),
		PlanEntry(TEXT("Action.Overcharge"), ERTActionSlot::Main, /*CostMP*/ 0, /*CooldownRemaining*/ 2),
	};
	Plan.Sort([](const FRTPlannedAction& A, const FRTPlannedAction& B)
	{
		return A.Def.ActionId.LexicalLess(B.Def.ActionId);
	});

	const FRTPlanValidation Reference = URTPlanValidationLibrary::ValidatePlan(PlanUnit(5), Plan);
	TestFalse(TEXT("il piano di riferimento e' illegale"), Reference.bLegal);

	// Tutte le 6 permutazioni di tre voci, generate ruotando e scambiando: nessuna cambia il verdetto.
	int32 Permutations = 0;
	for (int32 I = 0; I < Plan.Num(); ++I)
	{
		for (int32 J = 0; J < Plan.Num(); ++J)
		{
			if (I == J)
			{
				continue;
			}
			TArray<FRTPlannedAction> Shuffled = Plan;
			Shuffled.Swap(I, J);
			const FRTPlanValidation Other = URTPlanValidationLibrary::ValidatePlan(PlanUnit(5), Shuffled);
			TestEqual(TEXT("il verdetto non cambia con l'ordine"), Other.bLegal, Reference.bLegal);
			TestEqual(TEXT("il motivo non cambia con l'ordine"), Other.Reason, Reference.Reason);
			++Permutations;
		}
	}
	TestEqual(TEXT("permutazioni provate"), Permutations, 6);
	return true;
}

/**
 * Invarianza per permutazione quando a decidere sono gli SLOT, non il cooldown.
 *
 * Serve un test proprio: nella prova precedente l'illegalita' la decide il cooldown, che si trova
 * scorrendo l'array in qualunque ordine — quindi quel test resta verde anche togliendo l'ordine canonico,
 * e non dimostra nulla sugli slot. Qui invece l'ordine conta davvero: composto come
 * `Sprint, attacco, Move` il rifiutato e' l'attacco, composto come `attacco, Move, Sprint` sarebbe lo
 * `Sprint` — due colpevoli diversi per lo stesso piano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanSlotVerdictIsPermutationInvariant,
	"RefactorTactics.Plan.SlotVerdictIsPermutationInvariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanSlotVerdictIsPermutationInvariant::RunTest(const FString&)
{
	const TArray<FRTPlannedAction> Plan = {
		PlanEntry(TEXT("Action.Sprint"), ERTActionSlot::MovementAndMain),
		PlanEntry(TEXT("Action.BasicAttack"), ERTActionSlot::Main),
		PlanEntry(TEXT("Action.Move"), ERTActionSlot::Movement),
	};

	const FRTPlanValidation Reference = URTPlanValidationLibrary::ValidatePlan(PlanUnit(10), Plan);
	TestFalse(TEXT("Sprint non lascia spazio ad altro"), Reference.bLegal);
	TestEqual(TEXT("il motivo e' lo slot occupato"), Reference.Reason,
		ERTActionInvalidReason::SlotOccupied);
	TestEqual(TEXT("il colpevole e' l'attacco, non lo Sprint"), Reference.OffendingActionId,
		FName(TEXT("Action.BasicAttack")));

	for (int32 I = 0; I < Plan.Num(); ++I)
	{
		for (int32 J = 0; J < Plan.Num(); ++J)
		{
			if (I == J)
			{
				continue;
			}
			TArray<FRTPlannedAction> Shuffled = Plan;
			Shuffled.Swap(I, J);
			const FRTPlanValidation Other = URTPlanValidationLibrary::ValidatePlan(PlanUnit(10), Shuffled);
			TestEqual(TEXT("il verdetto non cambia con l'ordine"), Other.bLegal, Reference.bLegal);
			TestEqual(TEXT("il motivo non cambia con l'ordine"), Other.Reason, Reference.Reason);
			TestEqual(TEXT("il colpevole non cambia con l'ordine"), Other.OffendingActionId,
				Reference.OffendingActionId);
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
