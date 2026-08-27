#include "Misc/AutomationTest.h"
#include "Algo/Reverse.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTCatalogLibrary.h"
#include "Turn/RTActionQueue.h"
#include "Turn/RTActionQueueLibrary.h"
#include "Turn/RTTurnRules.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Istanza minima: chi la esegue, con quale definizione. Nome distinto per file (unity build). */
	FRTActionInstance QueuedAction(const FName& Id, ERTResolutionPhase Phase, int32 Priority,
		int32 SourceUnitId, int32 EventSequence = 0)
	{
		FRTActionInstance Instance;
		Instance.Def.ActionId = Id;
		Instance.Def.ResolutionPhase = Phase;
		Instance.Def.Priority = Priority;
		Instance.Def.Fallback = (Phase == ERTResolutionPhase::NormalMovement || Phase == ERTResolutionPhase::FastMovement)
			? ERTActionFallback::Stop : ERTActionFallback::Cancel;
		Instance.SourceUnitId = SourceUnitId;
		Instance.EventSequence = EventSequence;
		return Instance;
	}

	/** Gli ActionId nell'ordine in cui la coda li risolve. */
	TArray<FName> ResolvedOrder(const TArray<FRTActionInstance>& Queue)
	{
		TArray<FName> Out;
		for (const FRTActionInstance& Instance : Queue) { Out.Add(Instance.Def.ActionId); }
		return Out;
	}
}

// ---------------------------------------------------------------------------------------------------------
// Ordinamento: macro-fase -> priorita' -> ActionId -> SourceUnitId -> EventSequence
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionOrderByPriorityTest,
	"RefactorTactics.Actions.OrderByPriority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionOrderByPriorityTest::RunTest(const FString&)
{
	// Dentro la stessa macro-fase, priorita' MINORE risolve prima (regola del catalogo v0.1).
	TArray<FRTActionInstance> Queue;
	Queue.Add(QueuedAction(TEXT("Action.HeavyAttack"),     ERTResolutionPhase::Attack, 80, /*Unit*/ 0));
	Queue.Add(QueuedAction(TEXT("Action.PrecisionAttack"), ERTResolutionPhase::Attack, 60, /*Unit*/ 0));
	Queue.Add(QueuedAction(TEXT("Action.Push"),            ERTResolutionPhase::Control, 40, /*Unit*/ 0));
	Queue.Add(QueuedAction(TEXT("Action.Interrupt"),       ERTResolutionPhase::Control, 20, /*Unit*/ 0));

	URTActionQueueLibrary::SortActionInstances(Queue);
	const TArray<FName> Order = ResolvedOrder(Queue);

	// Controllo e attacco stanno entrambi nel Blast: e' la PRIORITA' a metterli in fila, non una fase in piu'.
	TestEqual(TEXT("interrupt (20) per primo"), Order[0], FName(TEXT("Action.Interrupt")));
	TestEqual(TEXT("push (40) secondo"),        Order[1], FName(TEXT("Action.Push")));
	TestEqual(TEXT("precisione (60) terzo"),    Order[2], FName(TEXT("Action.PrecisionAttack")));
	TestEqual(TEXT("pesante (80) per ultimo"),  Order[3], FName(TEXT("Action.HeavyAttack")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionPhaseRespectsAtlasTest,
	"RefactorTactics.Actions.PhaseMappingRespectsAtlas",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionPhaseRespectsAtlasTest::RunTest(const FString&)
{
	// La macro-fase viene PRIMA della priorita': un Move a priorita' 50 risolve dopo un attacco a priorita' 80,
	// perche' il Move e' una macro-fase successiva. E' l'identita' tattica di Atlas (ADR-0003 §1): l'attacco
	// vale da fermo, muoversi e' un impegno che si paga. Il catalogo v0.1 metteva il movimento PRIMA.
	TArray<FRTActionInstance> Queue;
	Queue.Add(QueuedAction(TEXT("Action.Move"),        ERTResolutionPhase::NormalMovement, 50, /*Unit*/ 0));
	Queue.Add(QueuedAction(TEXT("Action.HeavyAttack"), ERTResolutionPhase::Attack,         80, /*Unit*/ 0));
	Queue.Add(QueuedAction(TEXT("Action.Dodge"),        ERTResolutionPhase::FastMovement,   30, /*Unit*/ 0));
	Queue.Add(QueuedAction(TEXT("Action.Guard"),       ERTResolutionPhase::Preparation,    40, /*Unit*/ 0));

	URTActionQueueLibrary::SortActionInstances(Queue);
	const TArray<FName> Order = ResolvedOrder(Queue);

	TestEqual(TEXT("Prep per primo"),   Order[0], FName(TEXT("Action.Guard")));
	TestEqual(TEXT("poi il Dash"),      Order[1], FName(TEXT("Action.Dodge")));
	TestEqual(TEXT("poi il Blast"),     Order[2], FName(TEXT("Action.HeavyAttack")));
	TestEqual(TEXT("il Move per ULTIMO, dopo l'attacco"), Order[3], FName(TEXT("Action.Move")));

	// La stessa cosa detta come invariante, non come sequenza: qualunque coda contenga entrambe, il Move
	// non puo' mai precedere un attacco.
	const int32 MoveIdx = Order.IndexOfByKey(FName(TEXT("Action.Move")));
	const int32 AttackIdx = Order.IndexOfByKey(FName(TEXT("Action.HeavyAttack")));
	TestTrue(TEXT("Action.Move risolve DOPO Action.HeavyAttack"), MoveIdx > AttackIdx);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionPermutationInvariantTest,
	"RefactorTactics.Actions.PermutationInvariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionPermutationInvariantTest::RunTest(const FString&)
{
	// Coda con collisioni su OGNI chiave, cosi' il tie-break viene esercitato fino in fondo: stessa fase,
	// stessa priorita', stesso ActionId, e infine stesso SourceUnitId (a distinguere resta EventSequence).
	TArray<FRTActionInstance> Queue;
	Queue.Add(QueuedAction(TEXT("Action.BasicAttack"), ERTResolutionPhase::Attack, 50, /*Unit*/ 2, /*Seq*/ 0));
	Queue.Add(QueuedAction(TEXT("Action.BasicAttack"), ERTResolutionPhase::Attack, 50, /*Unit*/ 1, /*Seq*/ 0));
	Queue.Add(QueuedAction(TEXT("Action.Heal"),        ERTResolutionPhase::Attack, 50, /*Unit*/ 1, /*Seq*/ 0));
	Queue.Add(QueuedAction(TEXT("Action.BasicAttack"), ERTResolutionPhase::Attack, 50, /*Unit*/ 1, /*Seq*/ 1));
	Queue.Add(QueuedAction(TEXT("Action.Move"),        ERTResolutionPhase::NormalMovement, 50, /*Unit*/ 0));

	TArray<FRTActionInstance> Sorted = Queue;
	URTActionQueueLibrary::SortActionInstances(Sorted);

	// Ordine invertito in ingresso -> stessa sequenza in uscita.
	TArray<FRTActionInstance> Reversed = Queue;
	Algo::Reverse(Reversed);
	URTActionQueueLibrary::SortActionInstances(Reversed);

	// E una permutazione qualsiasi, per non verificare solo il caso simmetrico.
	TArray<FRTActionInstance> Shuffled;
	Shuffled.Add(Queue[3]); Shuffled.Add(Queue[0]); Shuffled.Add(Queue[4]);
	Shuffled.Add(Queue[2]); Shuffled.Add(Queue[1]);
	URTActionQueueLibrary::SortActionInstances(Shuffled);

	bool bSame = Sorted.Num() == Reversed.Num() && Sorted.Num() == Shuffled.Num();
	for (int32 i = 0; bSame && i < Sorted.Num(); ++i)
	{
		bSame = Sorted[i].Def.ActionId == Reversed[i].Def.ActionId
			&& Sorted[i].SourceUnitId == Reversed[i].SourceUnitId
			&& Sorted[i].EventSequence == Reversed[i].EventSequence
			&& Sorted[i].Def.ActionId == Shuffled[i].Def.ActionId
			&& Sorted[i].SourceUnitId == Shuffled[i].SourceUnitId
			&& Sorted[i].EventSequence == Shuffled[i].EventSequence;
	}
	TestTrue(TEXT("permutare l'ingresso non cambia l'ordine risolto"), bSame);

	// L'ordine e' TOTALE: due istanze adiacenti non possono mai essere "equivalenti", altrimenti a deciderle
	// resterebbe l'ordine di arrivo — cioe' il caso.
	bool bTotal = true;
	for (int32 i = 1; i < Sorted.Num(); ++i)
	{
		bTotal &= URTActionQueueLibrary::InstanceLess(Sorted[i - 1], Sorted[i]);
	}
	TestTrue(TEXT("ordine totale: nessuna coppia indistinguibile"), bTotal);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
