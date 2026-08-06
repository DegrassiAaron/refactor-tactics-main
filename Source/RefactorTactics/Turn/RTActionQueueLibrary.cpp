#include "Turn/RTActionQueueLibrary.h"
#include "Ability/RTCatalogLibrary.h"

bool URTActionQueueLibrary::InstanceLess(const FRTActionInstance& A, const FRTActionInstance& B)
{
	// 1) Macro-fase: e' l'ordine del turno, e viene prima di tutto il resto.
	const ERTMatchPhase PhaseA = URTCatalogLibrary::MapResolutionPhase(A.Def.ResolutionPhase);
	const ERTMatchPhase PhaseB = URTCatalogLibrary::MapResolutionPhase(B.Def.ResolutionPhase);
	if (PhaseA != PhaseB)
	{
		return static_cast<uint8>(PhaseA) < static_cast<uint8>(PhaseB); // enum confrontato per valore (no float)
	}

	// 2) Priorita' intera: valore minore risolve prima (regola del catalogo v0.1).
	if (A.Def.Priority != B.Def.Priority)
	{
		return A.Def.Priority < B.Def.Priority;
	}

	// 3..5) Tie-break assoluti: due azioni non restano mai indistinguibili, altrimenti a deciderle
	// resterebbe l'ordine di arrivo nel container — cioe' il caso.
	if (A.Def.ActionId != B.Def.ActionId)
	{
		return A.Def.ActionId.LexicalLess(B.Def.ActionId); // confronto lessicale: stabile fra esecuzioni
	}
	if (A.SourceUnitId != B.SourceUnitId)
	{
		return A.SourceUnitId < B.SourceUnitId;
	}
	return A.EventSequence < B.EventSequence;
}

void URTActionQueueLibrary::SortActionInstances(TArray<FRTActionInstance>& Instances)
{
	Instances.Sort([](const FRTActionInstance& A, const FRTActionInstance& B) { return InstanceLess(A, B); });
}

TArray<FRTActionInstance> URTActionQueueLibrary::InstancesForPhase(const TArray<FRTActionInstance>& Instances,
	ERTMatchPhase Phase)
{
	TArray<FRTActionInstance> Out;
	for (const FRTActionInstance& Instance : Instances)
	{
		if (URTCatalogLibrary::MapResolutionPhase(Instance.Def.ResolutionPhase) == Phase)
		{
			Out.Add(Instance);
		}
	}
	SortActionInstances(Out);
	return Out;
}
