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
	//
	// 🔴 **A difenderla e' `Actions.UnitOrderStableIdBeatsActorName`**, e quel test esiste perche' i primi
	// quattro non ci riuscivano: in ogni loro fixture l'ordine dei nomi CONCORDAVA con quello degli id,
	// quindi togliendo questa riga restavano tutti verdi — il nome decideva allo stesso modo. Una chiave
	// difesa solo da casi in cui la chiave successiva direbbe lo stesso non e' difesa. Trovato in code review.
	if (A.StableUnitId != B.StableUnitId)
	{
		return A.StableUnitId < B.StableUnitId;
	}

	// 3) Il nome dell'Actor, per il solo caso in cui l'identita' non sia ancora stata assegnata (vale `0` per
	// entrambe): la pianificazione prima del primo lock-in.
	//
	// `LexicalLess` e non `FastLess`: l'engine documenta il primo *«stable / deterministic over process
	// runs»* e il secondo come stabile solo dentro un processo — che qui sarebbe il difetto, non un dettaglio.
	// E' lo stesso confronto che `InstanceLess` usa per `ActionId`, poche righe piu' su.
	return A.ActorName.LexicalLess(B.ActorName);
}

FRTUnitOrderKey URTActionQueueLibrary::MakeUnitOrderKey(const ARTUnit& Unit)
{
	return FRTUnitOrderKey(Unit.Cell, Unit.StableUnitId, Unit.GetFName());
}

void URTActionQueueLibrary::SortUnitsForResolution(TArray<ARTUnit*>& Units)
{
	// In place, con la chiave costruita DENTRO il comparatore — quindi due volte per confronto, cioe'
	// O(N log N) costruzioni invece delle O(N) di una decorazione. ⚠️ **E' un compromesso scelto, non una
	// svista**: la stesura precedente decorava per ottenere le O(N), e pagava tre cose peggiori — `Keys` e
	// `Order` temporanei, e un `Units = MoveTemp(Sorted)` che buttava via il buffer che `CollectLivingUnits`
	// riusa con `Reset()`+`Reserve()` e che `FRTScenarioSession` tiene per tutta la partita.
	//
	// 🔑 Cio' che rende accettabile il compromesso e' che la chiave **non alloca**: `FRTCellId` e' tre
	// interi, `StableUnitId` uno, e `GetFName()` non passa da `FName::ToString()`. Sono copie, non
	// allocazioni — il contrario della prima stesura, che teneva una `FString` e per cui la decorazione era
	// l'unica uscita. Se un giorno la chiave crescesse fino ad allocare, la decorazione per INDICI (senza
	// `MoveTemp` sull'array del chiamante) e' la forma giusta, e vive gia' in `ResolveContestedBoundary`.
	Units.Sort([](const ARTUnit& A, const ARTUnit& B)
	{
		return UnitOrderLess(MakeUnitOrderKey(A), MakeUnitOrderKey(B));
	});
}
