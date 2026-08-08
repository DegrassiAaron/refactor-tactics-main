#include "Map/RTHexArcLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"

namespace
{
	/**
	 * I DUE versi dello stesso ponte. `AddTransition` bidirezionale ne scrive due, e chi comanda «spegnilo»
	 * non deve sapere quale dei due e' stato scritto per primo: senza questa raccolta si potrebbe spegnere
	 * l'andata e lasciare aperto il ritorno, cioe' un ponte percorribile in un senso solo per errore.
	 */
	TArray<FRTHexEdge> BothWays(const URTHexMapAsset* Map, const FRTCellId& A, const FRTCellId& B)
	{
		TArray<FRTHexEdge> Found;
		if (Map == nullptr)
		{
			return Found;
		}
		for (const FRTHexEdge& Edge : Map->Transitions)
		{
			if ((Edge.From == A && Edge.To == B) || (Edge.From == B && Edge.To == A))
			{
				Found.Add(Edge);
			}
		}
		// Ordine canonico: da qui escono voci di TurnLog.
		Found.Sort([](const FRTHexEdge& X, const FRTHexEdge& Y)
		{
			if (!(X.From == Y.From)) { return URTHexLibrary::StableLess(X.From, Y.From); }
			return URTHexLibrary::StableLess(X.To, Y.To);
		});
		return Found;
	}

	FRTArcChange MakeChange(const FRTHexEdge& Edge)
	{
		FRTArcChange Change;
		Change.From = Edge.From;
		Change.To = Edge.To;
		Change.State = Edge.State;
		Change.RemainingIntegrity = Edge.Integrity;
		Change.bBroken = Edge.State != ERTHexArcState::Active;
		return Change;
	}
}

const FRTHexEdge* URTHexArcLibrary::FindArc(const URTHexMapAsset* Map, const FRTCellId& From,
	const FRTCellId& To)
{
	if (Map == nullptr)
	{
		return nullptr;
	}
	for (const FRTHexEdge& Edge : Map->Transitions)
	{
		if (Edge.From == From && Edge.To == To)
		{
			return &Edge;
		}
	}
	return nullptr;
}

bool URTHexArcLibrary::IsArcTraversable(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To)
{
	const FRTHexEdge* Edge = FindArc(Map, From, To);
	return Edge != nullptr && Edge->State == ERTHexArcState::Active;
}

bool URTHexArcLibrary::ArcConductsElectricity(const URTHexMapAsset* Map, const FRTCellId& From,
	const FRTCellId& To)
{
	// Un ponte SPENTO non conduce, per quanto dichiari la conduttivita': non c'e' collegamento da risalire.
	const FRTHexEdge* Edge = FindArc(Map, From, To);
	return Edge != nullptr && Edge->State == ERTHexArcState::Active && Edge->bConductsElectricity;
}

TArray<FRTArcChange> URTHexArcLibrary::SetArcState(URTHexMapAsset* Map, const FRTCellId& From,
	const FRTCellId& To, ERTHexArcState State)
{
	TArray<FRTArcChange> Changes;
	if (Map == nullptr)
	{
		return Changes;
	}

	TArray<FRTHexEdge> Updated;
	for (FRTHexEdge Edge : BothWays(Map, From, To))
	{
		if (Edge.State == State)
		{
			continue; // gia' li': non e' un cambio, e la revisione non deve muoversi
		}
		if (Edge.State == ERTHexArcState::Destroyed)
		{
			continue; // terminale: un ponte abbattuto non si riattiva
		}
		Edge.State = State;
		Updated.Add(Edge);
		Changes.Add(MakeChange(Edge));
	}

	// UNA sola revisione per il ponte: due archi, un evento (stessa regola del portone di CP 9.3).
	Map->UpdateTransitions(Updated);
	return Changes;
}

TArray<FRTArcChange> URTHexArcLibrary::DamageArc(URTHexMapAsset* Map, const FRTCellId& From,
	const FRTCellId& To, int32 Amount)
{
	TArray<FRTArcChange> Changes;
	if (Map == nullptr || Amount <= 0)
	{
		return Changes;
	}

	TArray<FRTHexEdge> Updated;
	for (FRTHexEdge Edge : BothWays(Map, From, To))
	{
		if (Edge.State == ERTHexArcState::Destroyed)
		{
			continue; // gia' crollato: non incassa altro
		}
		Edge.Integrity -= Amount;
		if (Edge.Integrity <= 0)
		{
			Edge.Integrity = 0;
			Edge.State = ERTHexArcState::Destroyed;
		}
		Updated.Add(Edge);
		Changes.Add(MakeChange(Edge));
	}

	Map->UpdateTransitions(Updated);
	return Changes;
}
