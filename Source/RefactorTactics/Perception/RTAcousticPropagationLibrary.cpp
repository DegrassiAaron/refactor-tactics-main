#include "Perception/RTAcousticPropagationLibrary.h"

#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Terrain/RTTerrainLibrary.h"

namespace
{
	/**
	 * I vicini ACUSTICI di una cella, col costo che il suono paga per raggiungerli.
	 *
	 * Ricalca `URTHexPathLibrary::GraphNeighbors` — stesso grafo, stesse transizioni — con **una** differenza,
	 * che e' tutto il punto dell'occlusione: un arco che il movimento non puo' attraversare (copertura alta,
	 * porta chiusa) il suono lo attraversa **pagando**. Chiamare `GraphNeighbors` e basta avrebbe dato un
	 * suono che si ferma ai muri come un'unita', cioe' avrebbe cancellato la distinzione fra
	 * `OcclusionAttenuates` e `UsesGraphNotEuclideanRadius`.
	 *
	 * Cosa NON attraversa, e perche':
	 * - una cella `bBlocksMovement` e' volume pieno, non un varco chiuso: dentro non c'e' nessuno da
	 *   assordare, e farci passare il suono richiederebbe una regola sullo spessore che nessuno ha deciso;
	 * - una transizione non `Active` (ponte alzato, scala ritratta) non e' un collegamento affievolito: non
	 *   c'e' proprio, esattamente come per il movimento.
	 *
	 * Il costo NON e' `MoveCost`: quello e' quanto costa camminarci. Il suono paga un passo, piu' l'eventuale
	 * occlusione dell'arco. L'asprezza della superficie agisce altrove — alla SORGENTE, vedi `Propagate`.
	 */
	TArray<TPair<FRTCellId, int32>> AcousticNeighbors(const URTHexMapAsset* Map, const FRTCellId& Cell)
	{
		TArray<TPair<FRTCellId, int32>> Out;
		if (!Map)
		{
			return Out;
		}

		for (const FRTCellId& N : URTHexLibrary::Neighbors(Cell))
		{
			const FRTHexCellData* D = Map->FindCell(N);
			if (!D || D->bBlocksMovement)
			{
				continue;
			}
			const int32 Occlusion = URTHexCoverLibrary::BlocksTraversal(Map, Cell, N)
				? URTAcousticPropagationLibrary::BlockedEdgeAttenuation
				: 0;
			Out.Add(TPair<FRTCellId, int32>(N, URTAcousticPropagationLibrary::StepCost + Occlusion));
		}

		// Transizioni verticali: una rampa porta il suono di sopra come porta un'unita'. E' la meta' della
		// ragione per cui la propagazione e' sul GRAFO e non su una sfera — una sfera salirebbe di piano
		// anche dove non esiste un collegamento.
		for (const FRTHexEdge& E : Map->Transitions)
		{
			if (E.From != Cell || E.State != ERTHexArcState::Active)
			{
				continue;
			}
			const FRTHexCellData* D = Map->FindCell(E.To);
			if (!D || D->bBlocksMovement)
			{
				continue;
			}
			Out.Add(TPair<FRTCellId, int32>(E.To, URTAcousticPropagationLibrary::StepCost));
		}
		return Out;
	}
}

int32 URTAcousticPropagationLibrary::SurfaceNoiseDelta(ERTHexSurface Surface)
{
	// Dal catalogo terreni, che e' l'owner del dato: una superficie assente (`Void`, che nel catalogo non
	// c'e') ricade sul default `0` di `FindTerrainDef`. Non e' una scelta di bilanciamento — e' che su `Void`
	// non ci sta nessuno, quindi non ci si fa rumore.
	return URTTerrainLibrary::FindTerrainDef(Surface).NoiseDelta;
}

TArray<FRTNoiseReception> URTAcousticPropagationLibrary::Propagate(const URTHexMapAsset* Map,
	const FRTNoiseEvent& Event)
{
	TArray<FRTNoiseReception> Out;

	// La superficie della SORGENTE alza il rumore prodotto: e' il verso che [D-042] dichiara alla lettera —
	// «l'acqua bassa AGGIUNGE +2 al rumore ... sprintarci fa 7 su 10, come un Dash su terreno libero».
	// La scala lo conferma da sola: il fuoco (+4) crepita, non ovatta, e il ghiaccio (+1) scricchiola sotto
	// i passi. Non e' quindi un'attenuazione in transito: una superficie rumorosa rende piu' forte CHI CI
	// AGISCE SOPRA, e da li' in poi il suono si attenua per distanza e per archi.
	int32 Budget = FMath::Max(0, Event.Intensity);
	if (Map)
	{
		if (const FRTHexCellData* Origin = Map->FindCell(Event.OriginCell))
		{
			Budget += SurfaceNoiseDelta(Origin->Surface);
		}
	}
	if (Budget <= 0)
	{
		return Out; // `Action.Wait` vale 0: il silenzio non produce un evento udibile da nessuno
	}

	Out.Add(FRTNoiseReception(Event.OriginCell, Budget));
	if (!Map)
	{
		return Out; // fail-closed: senza mappa non si valutano ne' archi ne' superfici
	}

	// Dijkstra a costi interi, con la stessa disciplina del pathfinding: si estrae il minimo e i pari merito
	// si rompono con `StableLess`. Senza il tie-break, due celle allo stesso costo verrebbero finalizzate
	// nell'ordine in cui l'array le ha incontrate — e la propagazione dipenderebbe dall'ordine di scoperta,
	// che e' proprio cio' che `PermutationInvariant` vieta.
	TMap<FRTCellId, int32> Spent;
	Spent.Add(Event.OriginCell, 0);
	TArray<FRTCellId> Frontier;
	Frontier.Add(Event.OriginCell);
	TSet<FRTCellId> Closed;

	while (Frontier.Num() > 0)
	{
		int32 BestIdx = 0;
		for (int32 I = 1; I < Frontier.Num(); ++I)
		{
			const int32 D = Spent[Frontier[I]];
			const int32 BestD = Spent[Frontier[BestIdx]];
			if (D < BestD || (D == BestD && URTHexLibrary::StableLess(Frontier[I], Frontier[BestIdx])))
			{
				BestIdx = I;
			}
		}
		const FRTCellId Current = Frontier[BestIdx];
		Frontier.RemoveAt(BestIdx);
		if (Closed.Contains(Current))
		{
			continue;
		}
		Closed.Add(Current);

		for (const TPair<FRTCellId, int32>& Step : AcousticNeighbors(Map, Current))
		{
			const int32 Tentative = Spent[Current] + Step.Value;
			// `>=` e non `>`: a costo pari all'intensita' il rumore residuo sarebbe 0, e «arriva a zero» e
			// «non arriva» sono la stessa cosa. Tenerle costringerebbe ogni consumatore a rifiltrarle, e la
			// prima volta che se ne dimenticasse uno la soglia direbbe di si' al silenzio.
			if (Tentative >= Budget)
			{
				continue;
			}
			const int32* Existing = Spent.Find(Step.Key);
			if (!Existing || Tentative < *Existing)
			{
				Spent.Add(Step.Key, Tentative);
				Frontier.Add(Step.Key);
			}
		}
	}

	for (const TPair<FRTCellId, int32>& Pair : Spent)
	{
		if (Pair.Key == Event.OriginCell)
		{
			continue; // gia' aggiunta con l'intensita' piena
		}
		Out.Add(FRTNoiseReception(Pair.Key, Budget - Pair.Value));
	}

	// Ordine canonico. `Spent` e' una TMap e la si e' appena iterata: senza questo Sort l'output dipenderebbe
	// dall'hash delle chiavi, che e' esattamente il tipo di dipendenza che l'invariante #3 vieta.
	Out.Sort([](const FRTNoiseReception& A, const FRTNoiseReception& B)
		{ return URTHexLibrary::StableLess(A.Cell, B.Cell); });
	return Out;
}

int32 URTAcousticPropagationLibrary::NoiseAtCell(const TArray<FRTNoiseReception>& Received,
	const FRTCellId& ListenerCell)
{
	for (const FRTNoiseReception& R : Received)
	{
		if (R.Cell == ListenerCell)
		{
			return R.ReceivedNoise;
		}
	}
	return 0;
}

bool URTAcousticPropagationLibrary::IsAudible(int32 ReceivedNoise, int32 HearingThreshold)
{
	// Soglia BASSA = orecchio fine (D-041). Il verso e' controintuitivo, ed e' il motivo per cui questa
	// riga sta in una funzione invece che sparsa nei chiamanti.
	return ReceivedNoise > 0 && ReceivedNoise >= HearingThreshold;
}
