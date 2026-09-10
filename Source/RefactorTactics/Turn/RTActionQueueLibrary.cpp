#include "Turn/RTActionQueueLibrary.h"
#include "Ability/RTCatalogLibrary.h"
#include "Map/RTHexLibrary.h" // StableLess: la prima chiave dell'ordine delle unita' e' la cella
#include "Unit/RTUnit.h"      // ARTUnit: solo qui, perche' l'header resti senza un Actor dentro

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

// --- Ordine delle UNITA' (#2922) ------------------------------------------------------------------------

bool URTActionQueueLibrary::UnitOrderLess(const FRTUnitOrderKey& A, const FRTUnitOrderKey& B)
{
	// 1) La cella: e' la chiave che c'era prima di questa funzione, e resta la prima. Dove le celle
	// differiscono l'ordine e' identico a quello di ieri, e non si sposta niente.
	if (!(A.Cell == B.Cell))
	{
		return URTHexLibrary::StableLess(A.Cell, B.Cell);
	}

	// 2) L'identita' di partita ([D-063]). Due unita' sulla stessa cella esistono — `FRTHexSnapshot::Overlaps`
	// le registra — e senza questa riga a ordinarle resterebbe `GetAllActorsOfClass`.
	if (A.StableUnitId != B.StableUnitId)
	{
		return A.StableUnitId < B.StableUnitId;
	}

	// 3) Il nome dell'Actor, per il solo caso in cui l'identita' non sia ancora stata assegnata (vale `0` per
	// entrambe): pianificazione prima del primo lock-in, e `AssignUnitControlGroups` a inizio partita.
	//
	// 🔴 `Compare(..., CaseSensitive)` e NON `operator<`: `FString::UEOpLessThan` e' `Stricmp(...) < 0` —
	// case INSENSITIVE — quindi non e' un ordine totale sui byte, e due nomi che differiscono solo per il
	// caso resterebbero a pari merito. E' lo stesso difetto che `URTTurnLogLibrary::EntryLess` ha gia' pagato
	// una volta sulla v10, con la ragione scritta li'.
	return A.ActorName.Compare(B.ActorName, ESearchCase::CaseSensitive) < 0;
}

FRTUnitOrderKey URTActionQueueLibrary::MakeUnitOrderKey(const ARTUnit& Unit)
{
	return FRTUnitOrderKey(Unit.Cell, Unit.StableUnitId, Unit.GetName());
}

void URTActionQueueLibrary::SortUnitsForResolution(TArray<ARTUnit*>& Units)
{
	const int32 Num = Units.Num();
	if (Num < 2)
	{
		return; // niente da ordinare, e niente chiave da costruire
	}

	// 🔑 **La chiave si costruisce UNA volta per unita', non a ogni confronto.** `AActor::GetName()` passa da
	// `FName::ToString()` e alloca una `FString`: dentro il comparatore ne pagherebbe O(N log N) dove ne
	// bastano O(N), e questi sort girano piu' volte per turno su ogni fase. E' anche la ragione per cui
	// `MatchRosterLess` il nome lo tocca solo nell'ultimo ramo, quando tutto il resto ha gia' pareggiato.
	//
	// Si ordinano gli INDICI e non due array in parallelo, come fa gia'
	// `URTReactionOpportunityTypesLibrary::SortParticipantsCanonically`: due sort indipendenti sugli stessi
	// criteri divergono al primo pareggio, ed e' il difetto che questa funzione esiste per chiudere.
	TArray<FRTUnitOrderKey> Keys;
	Keys.Reserve(Num);
	TArray<int32> Order;
	Order.Reserve(Num);
	for (int32 i = 0; i < Num; ++i)
	{
		Keys.Add(MakeUnitOrderKey(*Units[i])); // stessa precondizione di prima: nessun `nullptr` nell'array
		Order.Add(i);
	}

	// `Sort` e non `StableSort`: `UnitOrderLess` e' un ordine TOTALE, quindi la stabilita' non ha niente da
	// decidere. Due chiavi identiche vorrebbero due Actor con lo stesso nome nello stesso mondo, che UE non
	// produce — ed e' `Actions.UnitOrderPermutationInvariant` a tenere onesta questa frase.
	Order.Sort([&Keys](int32 A, int32 B) { return UnitOrderLess(Keys[A], Keys[B]); });

	TArray<ARTUnit*> Sorted;
	Sorted.Reserve(Num);
	for (int32 Idx : Order)
	{
		Sorted.Add(Units[Idx]);
	}
	Units = MoveTemp(Sorted);
}
