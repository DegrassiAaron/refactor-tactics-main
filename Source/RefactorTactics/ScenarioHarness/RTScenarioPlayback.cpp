#include "ScenarioHarness/RTScenarioPlayback.h"

namespace RTScenarioPlayback
{
	TArray<FRTScenarioUnitView> ViewsAtTracedStates(
		const TArray<FRTScenarioUnitView>& Views,
		const TArray<FRTTracedUnitState>& States,
		const TMap<int32, FString>& ScenarioIdByUnitId)
	{
		// Lo stato per identita' d'authoring, risolto UNA volta: il ciclo sotto e' per vista, e chiedere la
		// traduzione per ognuna significherebbe scorrere gli stati N volte.
		//
		// ⚠️ Uno stato il cui `UnitId` la mappa non traduce non entra qui — vedi il fail-closed dichiarato
		// sull'header. Non si inventa un'identita' per farlo entrare.
		TMap<FString, const FRTTracedUnitState*> PerId;
		PerId.Reserve(States.Num());
		for (const FRTTracedUnitState& S : States)
		{
			if (const FString* Id = ScenarioIdByUnitId.Find(S.UnitId))
			{
				PerId.Add(*Id, &S);
			}
		}

		TArray<FRTScenarioUnitView> Out;
		Out.Reserve(Views.Num());
		for (const FRTScenarioUnitView& V : Views)
		{
			const FRTTracedUnitState* const* Trovato = PerId.Find(V.Id);
			if (Trovato == nullptr || *Trovato == nullptr)
			{
				// La traccia non la nomina: la posa di partenza e' gia' la risposta giusta.
				Out.Add(V);
				continue;
			}

			const FRTTracedUnitState& S = **Trovato;
			if (!S.bAlive)
			{
				// Abbattuta: esce dall'elenco. Un marcatore su un morto racconterebbe una partita diversa.
				continue;
			}

			// ⚠️ Si copia la vista e si spostano i due campi che la traccia dichiara: `HeroId`, `TeamId` e
			// `bBotControlled` sono dell'authoring e la traccia non li cambia. Ricostruire la vista da zero
			// li perderebbe.
			FRTScenarioUnitView Spostata = V;
			Spostata.Cell = S.Cell;
			Spostata.Facing = S.Facing;
			Out.Add(MoveTemp(Spostata));
		}

		return Out;
	}

	TArray<FRTTracedUnitState> InitialStatesFromViews(
		const TArray<FRTScenarioUnitView>& Views,
		const TMap<int32, FString>& ScenarioIdByUnitId)
	{
		// L'indice inverso: la mappa va da `UnitId` a identita', e qui serve il verso opposto. Si costruisce
		// una volta invece di cercare linearmente per ogni vista.
		TMap<FString, int32> UnitIdPerId;
		UnitIdPerId.Reserve(ScenarioIdByUnitId.Num());
		for (const TPair<int32, FString>& Pair : ScenarioIdByUnitId)
		{
			UnitIdPerId.Add(Pair.Value, Pair.Key);
		}

		TArray<FRTTracedUnitState> Out;
		Out.Reserve(Views.Num());
		for (const FRTScenarioUnitView& V : Views)
		{
			const int32* UnitId = UnitIdPerId.Find(V.Id);
			if (UnitId == nullptr)
			{
				// Nessuno `StableUnitId`: quest'unita' non comparira' mai nella traccia, e darle uno stato
				// iniziale con un id inventato e' il modo in cui i due spazi tornano a confondersi.
				continue;
			}

			FRTTracedUnitState S;
			S.UnitId = *UnitId;
			S.Cell = V.Cell;
			S.Facing = V.Facing;
			S.bAlive = true; // all'inizio lo sono tutte: la traccia dira' chi cade
			Out.Add(S);
		}

		return Out;
	}
}
