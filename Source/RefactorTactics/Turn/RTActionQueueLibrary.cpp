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
	// 🔴 **A difenderla e' `Actions.UnitOrderStableIdBeatsActorName`**, e quel test esiste perche' quelli
	// che c'erano prima non ci riuscivano: in ogni loro fixture l'ordine dei nomi CONCORDAVA con quello degli id,
	// quindi togliendo questa riga restavano tutti verdi — il nome decideva allo stesso modo. Una chiave
	// difesa solo da casi in cui la chiave successiva direbbe lo stesso non e' difesa. Trovato in code review.
	if (A.StableUnitId != B.StableUnitId)
	{
		return A.StableUnitId < B.StableUnitId;
	}

	// 3) Il nome dell'Actor, per il solo caso in cui l'identita' non sia ancora stata assegnata (vale `0` per
	// entrambe): la pianificazione prima del primo lock-in, e le unita' entrate dopo il congelamento del
	// roster, che `EnsureMatchRoster` non rinumera.
	//
	// 🔴 `Compare(..., CaseSensitive)` e NON `operator<`, e non un `FName`: il perche' di entrambi sta su
	// `FRTUnitOrderKey::ActorName`. In breve: `operator<` e `FName::Compare` sono case-INSENSITIVE, quindi
	// nessuno dei due e' un ordine totale, e il pareggio tornerebbe a `GetAllActorsOfClass`.
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
		return; // niente da ordinare, e nessuna chiave da costruire
	}

	// 🔑 **La chiave si costruisce UNA volta per unita', non a ogni confronto.** `AActor::GetName()`
	// passa da `FName::ToString()` e alloca: dentro il comparatore ne pagherebbe O(N log N) dove ne bastano
	// O(N). E' anche la ragione per cui `MatchRosterLess` il nome lo tocca solo nell'ultimo ramo.
	//
	// Si ordinano gli INDICI e non due array in parallelo, come fa gia' `ResolveContestedBoundary`: due sort
	// indipendenti sugli stessi criteri divergono al primo pareggio.
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
	// decidere. Due chiavi identiche vorrebbero due Actor con lo stesso nome, byte per byte, nello stesso
	// mondo — e l'ultima chiave e' case-sensitive proprio per non lasciare quella coppia indistinguibile.
	Order.Sort([&Keys](int32 A, int32 B) { return UnitOrderLess(Keys[A], Keys[B]); });

	// ⛔ **Si ricopia dentro `Units`, non si sostituisce l'array.** Una stesura precedente faceva
	// `Units = MoveTemp(Sorted)` e buttava via il buffer che `CollectLivingUnits` riusa con
	// `Reset()`+`Reserve()` — e che `FRTScenarioSession` tiene per tutta la partita. Trovato in code review.
	TArray<ARTUnit*> Originale = Units;
	for (int32 i = 0; i < Num; ++i)
	{
		Units[i] = Originale[Order[i]];
	}
}
