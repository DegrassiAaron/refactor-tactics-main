#include "Misc/AutomationTest.h"
#include "Ability/RTActionDef.h"
#include "Turn/Resolution/RTHexSim.h"
#include "Turn/Actions/RTPlanValidationLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Unita' viva con un budget di movimento dichiarato: e' tutto cio' che la validazione legge dallo snapshot. */
	FRTHexSimUnit PlanUnit(int32 MoveBudget = 5)
	{
		return FRTHexSimUnit(0, FRTCellId(0, 0), MoveBudget, /*bAlive*/ true);
	}

	/**
	 * Voce di piano minima: l'azione, lo slot che occupa e il cooldown residuo.
	 *
	 * Il `CostMP` e' uscito da qui con [D-190]: il validatore non lo legge piu', e un parametro che nessuna
	 * asserzione osserva avrebbe fatto credere il contrario a chi scrive il prossimo test.
	 */
	FRTPlannedAction PlanEntry(const FName& ActionId, ERTActionSlot Slot, int32 CooldownRemaining = 0)
	{
		FRTPlannedAction Entry;
		Entry.Def.ActionId = ActionId;
		Entry.Def.Slot = Slot;
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
		PlanEntry(TEXT("Action.Move"), ERTActionSlot::Movement),
		PlanEntry(TEXT("Action.BasicAttack"), ERTActionSlot::Main),
		PlanEntry(TEXT("Action.Counter"), ERTActionSlot::Reaction),
	};

	const FRTPlanValidation Result = URTPlanValidationLibrary::ValidatePlan(PlanUnit(5), Plan);

	TestTrue(TEXT("il piano canonico e' legale"), Result.bLegal);
	TestEqual(TEXT("nessun motivo di rifiuto"), Result.Reason, ERTActionInvalidReason::None);
	return true;
}

/**
 * Due principali del Blast si sommano entro il budget ([D-584] (1); E38 CP 38.14): fino a CP 38.14 la legalita' era
 * strutturale (D-028, D-114) e questo test le rifiutava. ➕ Da CP 38.16 neppure una mobilita' che prende anche la principale
 * (`MovementAndMain`) toglie posto: gli slot non decidono, e una mobilita' che «costa tutto il turno» lo dice col suo costo
 * ([D-628] (1)). Il nome e' storico: lo citano il piano dei checkpoint e le evidenze.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanSecondMainIsRejected,
	"RefactorTactics.Plan.SecondMainIsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanSecondMainIsRejected::RunTest(const FString&)
{
	const TArray<FRTPlannedAction> DueDelBlast = {
		PlanEntry(TEXT("Action.BasicAttack"), ERTActionSlot::Main),
		PlanEntry(TEXT("Action.Interact"), ERTActionSlot::Main),
	};
	TestTrue(TEXT("due principali del Blast sono legali"), URTPlanValidationLibrary::ValidatePlan(PlanUnit(), DueDelBlast).bLegal);

	const TArray<FRTPlannedAction> ConUnaCarica = {
		PlanEntry(TEXT("Test.Charge"), ERTActionSlot::MovementAndMain),
		PlanEntry(TEXT("Action.BasicAttack"), ERTActionSlot::Main),
		PlanEntry(TEXT("Action.Interact"), ERTActionSlot::Main),
	};
	TestTrue(TEXT("CP 38.16: senza costi, una mobilita' MovementAndMain e due principali stanno insieme"),
		URTPlanValidationLibrary::ValidatePlan(PlanUnit(8), ConUnaCarica).bLegal);

	// Controllo positivo: con i costi, a decidere e' il budget. La mobilita' che «costa tutto il turno» lo dice coi suoi AP.
	TArray<FRTPlannedAction> ConICosti = ConUnaCarica;
	ConICosti[0].Def.ActionPointCost = 6;
	ConICosti[1].Def.ActionPointCost = 2;
	ConICosti[2].Def.ActionPointCost = 2;
	for (int32 i = 0; i < ConICosti.Num(); ++i) { ConICosti[i].DeclarationIndex = i; }
	FRTHexSimUnit Unita = PlanUnit(8);
	Unita.ActionPointBudget = 6;
	const FRTPlanValidation Result = URTPlanValidationLibrary::ValidatePlan(Unita, ConICosti);
	TestFalse(TEXT("con un costo di 6 la mobilita' non lascia AP alla principale"), Result.bLegal);
	TestEqual(TEXT("il motivo e' il budget, non lo slot"), Result.Reason, ERTActionInvalidReason::OverBudget);
	TestEqual(TEXT("e si accusa la prima voce che sfora"), Result.OffendingActionId, FName(TEXT("Action.BasicAttack")));
	return true;
}

/**
 * Uno `Sprint` che dichiarasse ENTRAMBI gli slot (`MovementAndMain`, sintetico: nessuna riga del catalogo lo fa) e una principale.
 *
 * ⏱️ *Fino a CP 38.16 il piano era illegale con `SlotOccupied`, e il colpevole era l'attacco.* Da CP 38.16 gli slot non decidono
 * ([D-584] (1)), e `SlotOccupied` non ha piu' produttori: il piano e' legale finche' il budget lo paga. Il nome e' storico.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanSprintLeavesNoRoomForMain,
	"RefactorTactics.Plan.SprintLeavesNoRoomForMain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanSprintLeavesNoRoomForMain::RunTest(const FString&)
{
	const TArray<FRTPlannedAction> Plan = {
		PlanEntry(TEXT("Action.Sprint"), ERTActionSlot::MovementAndMain),
		PlanEntry(TEXT("Action.BasicAttack"), ERTActionSlot::Main),
	};

	const FRTPlanValidation Result = URTPlanValidationLibrary::ValidatePlan(PlanUnit(8), Plan);

	TestTrue(TEXT("CP 38.16: Sprint + principale e' legale"), Result.bLegal);
	TestEqual(TEXT("e nessun motivo"), Result.Reason, ERTActionInvalidReason::None);
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
		PlanEntry(TEXT("Action.Move"), ERTActionSlot::Movement),
		PlanEntry(TEXT("Action.BasicAttack"), ERTActionSlot::Main),
	};

	const FRTPlanValidation Result = URTPlanValidationLibrary::ValidatePlan(PlanUnit(5), Plan);

	TestTrue(TEXT("Wait non consuma nulla"), Result.bLegal);
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
		PlanEntry(TEXT("Action.Overcharge"), ERTActionSlot::Main, /*CooldownRemaining*/ 1),
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
		PlanEntry(TEXT("Action.Move"), ERTActionSlot::Movement),
		PlanEntry(TEXT("Action.BasicAttack"), ERTActionSlot::Main),
		PlanEntry(TEXT("Action.Overcharge"), ERTActionSlot::Main, /*CooldownRemaining*/ 2),
	};
	Plan.Sort([](const FRTPlannedAction& A, const FRTPlannedAction& B)
	{
		return A.Def.ActionId.LexicalLess(B.Def.ActionId);
	});
	// E38 CP 38.12: indici di dichiarazione fissi, cosi' a permutarsi e' solo l'array, e il verdetto nomina
	// sempre la stessa occorrenza.
	for (int32 i = 0; i < Plan.Num(); ++i) { Plan[i].DeclarationIndex = i; }

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
			TestEqual(TEXT("il colpevole non cambia con l'ordine"), Other.OffendingActionId, Reference.OffendingActionId);
			TestEqual(TEXT("ne' la sua occorrenza"), Other.OffendingDeclarationIndex, Reference.OffendingDeclarationIndex);
			++Permutations;
		}
	}
	TestEqual(TEXT("permutazioni provate"), Permutations, 6);
	return true;
}

/**
 * Il verdetto NON dipende da `MoveBudget`. E' il gate di [D-190].
 *
 * ⚠️ `MoveBudget` sono i punti movimento, non gli Action Point. Il budget di AP e' un altro campo dell'unita',
 * `ActionPointBudget` (E38 CP 38.11, [D-613] (1)), e quello il verdetto lo legge: lo pinna
 * `Plan.BudgetIsReadFromTheUnit`. Le voci di `PlanEntry` non dichiarano un costo (`ActionPointCost` 0), quindi
 * il piano qui sotto non arriva al budget di AP qualunque sia.
 *
 * Fino a quella decisione un ramo sommava i `CostMP` del piano e li confrontava col budget dell'unita'.
 * Uscito il ramo, lo snapshot non entra piu' nel verdetto — e la proprieta' va PINNATA invece che lasciata
 * alla lettura del corpo: chi reintroducesse la sottrazione respinta da [D-114] farebbe cadere questo test,
 * invece di aggiungere in silenzio un rifiuto che il pathfinding in pianificazione gia' produce, e meglio.
 *
 * Sostituisce `Plan.EmptyPlanIsLegalEvenWithDirtyBudget`, che difendeva il clamp `FMath::Max(0, MoveBudget)`
 * del ramo uscito. Il suo caso limite — il budget sporco — resta qui sotto: e' l'unico pezzo di quel test
 * che parli di una proprieta' ancora viva.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanVerdictIgnoresMoveBudget,
	"RefactorTactics.Plan.VerdictIgnoresMoveBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanVerdictIgnoresMoveBudget::RunTest(const FString&)
{
	// Un piano che si muove E agisce: e' il caso su cui un ramo a budget morderebbe per primo.
	const TArray<FRTPlannedAction> Plan = {
		PlanEntry(TEXT("Action.Move"), ERTActionSlot::Movement),
		PlanEntry(TEXT("Action.BasicAttack"), ERTActionSlot::Main),
	};

	// Nullo, sporco, normale e abbondante: quattro budget, un solo verdetto.
	for (const int32 Budget : { 0, -1, 5, 99 })
	{
		const FRTPlanValidation Result = URTPlanValidationLibrary::ValidatePlan(PlanUnit(Budget), Plan);
		TestTrue(FString::Printf(TEXT("budget %d: il piano canonico resta legale"), Budget), Result.bLegal);
		TestEqual(FString::Printf(TEXT("budget %d: nessun motivo di rifiuto"), Budget), Result.Reason,
			ERTActionInvalidReason::None);
	}

	// E il piano VUOTO resta legale col budget negativo: era il caso che il clamp del vecchio ramo
	// difendeva, dove `0 > -1` rendeva illegale perfino chi non si muove.
	const TArray<FRTPlannedAction> Empty;
	TestTrue(TEXT("piano vuoto, budget negativo"),
		URTPlanValidationLibrary::ValidatePlan(PlanUnit(-1), Empty).bLegal);
	return true;
}

/**
 * Invarianza per permutazione quando a decidere e' la regola dello scatto, non il cooldown (E38 CP 38.16).
 *
 * ⏱️ *Fino a CP 38.16 a decidere erano gli slot*: `Sprint, attacco, Move` era `SlotOccupied` con colpevole l'attacco. Gli slot non
 * decidono piu', e la regola di classe che resta e' quella dello scatto ([D-625] (4)). Due scatti diversi, dichiarati in un ordine
 * fisso: in qualunque ordine dell'array il motivo e' `SecondDash`, il colpevole il secondo dichiarato e il detentore il primo.
 * Il nome e' storico.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanSlotVerdictIsPermutationInvariant,
	"RefactorTactics.Plan.SlotVerdictIsPermutationInvariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanSlotVerdictIsPermutationInvariant::RunTest(const FString&)
{
	auto Scatto = [](const TCHAR* Id)
	{
		FRTPlannedAction Entry = PlanEntry(Id, ERTActionSlot::Movement);
		Entry.Def.ResolutionPhase = ERTResolutionPhase::FastMovement;
		return Entry;
	};
	TArray<FRTPlannedAction> Plan = {
		Scatto(TEXT("Action.Leap")),
		PlanEntry(TEXT("Action.BasicAttack"), ERTActionSlot::Main),
		Scatto(TEXT("Action.Dodge")),
		PlanEntry(TEXT("Action.Move"), ERTActionSlot::Movement),
	};
	for (int32 i = 0; i < Plan.Num(); ++i) { Plan[i].DeclarationIndex = i; } // E38 CP 38.12: indici fissi

	const FRTPlanValidation Reference = URTPlanValidationLibrary::ValidatePlan(PlanUnit(10), Plan);
	TestFalse(TEXT("due scatti diversi sono vietati"), Reference.bLegal);
	TestEqual(TEXT("il motivo e' il secondo scatto"), Reference.Reason, ERTActionInvalidReason::SecondDash);
	TestEqual(TEXT("il colpevole e' il secondo dichiarato"), Reference.OffendingActionId, FName(TEXT("Action.Dodge")));
	TestEqual(TEXT("e il detentore il primo"), Reference.HolderActionId, FName(TEXT("Action.Leap")));

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
			TestEqual(TEXT("ne' la sua occorrenza"), Other.OffendingDeclarationIndex, Reference.OffendingDeclarationIndex);
			TestEqual(TEXT("ne' il detentore e la sua occorrenza"), Other.HolderDeclarationIndex, Reference.HolderDeclarationIndex);
		}
	}
	return true;
}

/**
 * **[D-625] (4) — uno scatto per piano** (E38 CP 38.16): due scatti DIVERSI sono vietati anche quando il budget li paga; la stessa
 * azione due volte resta la copia vietata, che il validatore guarda prima; scatto + Move restano legali (la lettura (e)).
 *
 * Nessun eroe porta due scatti nel kit, quindi lo scenario `Spec.ActionEconomy.SecondRamIsRefused` prova solo la copia: il caso
 * delle due azioni diverse vive qui. Ogni rifiuto ha il suo controllo positivo nello stesso budget.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanSecondDifferentDashIsRefused,
	"RefactorTactics.Plan.SecondDifferentDashIsRefused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanSecondDifferentDashIsRefused::RunTest(const FString&)
{
	auto Voce = [](const TCHAR* Id, bool bScatto, int32 Costo, int32 CooldownDiRiga, int32 Dichiarata)
	{
		FRTPlannedAction Entry = PlanEntry(Id, bScatto ? ERTActionSlot::Movement : ERTActionSlot::Main);
		if (bScatto) { Entry.Def.ResolutionPhase = ERTResolutionPhase::FastMovement; }
		Entry.Def.ActionPointCost = Costo;
		Entry.CatalogCooldownTurns = CooldownDiRiga;
		Entry.DeclarationIndex = Dichiarata;
		return Entry;
	};
	FRTHexSimUnit Unita = PlanUnit(10);
	Unita.ActionPointBudget = 6;

	const FRTPlanValidation Due = URTPlanValidationLibrary::ValidatePlan(Unita,
		{ Voce(TEXT("Action.Leap"), true, 2, 2, 0), Voce(TEXT("Action.Dodge"), true, 2, 2, 1) });
	TestFalse(TEXT("due scatti diversi da 4 AP su 6: vietati"), Due.bLegal);
	TestEqual(TEXT("col motivo SecondDash"), Due.Reason, ERTActionInvalidReason::SecondDash);
	TestTrue(TEXT("e il motivo svuota il piano al lock-in"), URTPlanValidationLibrary::EmptiesPlanAtLockIn(Due.Reason));

	const FRTPlanValidation Copia = URTPlanValidationLibrary::ValidatePlan(Unita,
		{ Voce(TEXT("Action.Leap"), true, 2, 2, 0), Voce(TEXT("Action.Leap"), true, 2, 2, 1) });
	TestEqual(TEXT("la stessa azione due volte resta la copia vietata"), Copia.Reason, ERTActionInvalidReason::RepeatedWithCooldown);

	const FRTPlanValidation ConMove = URTPlanValidationLibrary::ValidatePlan(Unita,
		{ Voce(TEXT("Action.Leap"), true, 2, 2, 0), Voce(TEXT("Action.Move"), false, 2, 0, 1),
		  Voce(TEXT("Action.BasicAttack"), false, 2, 0, 2) });
	TestTrue(TEXT("controllo positivo: scatto + Move + attacco, 6 su 6, legale"), ConMove.bLegal);

	const FRTPlanValidation Uno = URTPlanValidationLibrary::ValidatePlan(Unita, { Voce(TEXT("Action.Leap"), true, 2, 2, 0) });
	TestTrue(TEXT("controllo positivo: uno scatto solo e' legale"), Uno.bLegal);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
