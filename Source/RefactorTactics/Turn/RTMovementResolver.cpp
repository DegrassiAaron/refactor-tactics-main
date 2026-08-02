#include "Turn/RTMovementResolver.h"

TArray<FRTGridCoord> URTMovementResolver::ResolveMoves(const TArray<FRTMoveRequest>& Requests)
{
	const int32 Num = Requests.Num();

	// Destinazione tentativa di ciascuna unita' (parte da To).
	TArray<FRTGridCoord> Dest;
	Dest.Reserve(Num);
	for (const FRTMoveRequest& Request : Requests)
	{
		Dest.Add(Request.To);
	}

	auto IsMoving = [&](int32 Index)
	{
		return !(Dest[Index] == Requests[Index].From);
	};

	// Punto fisso: le unita' possono solo passare da "in movimento" a "ferma" (monotono),
	// quindi il risultato converge ed e' indipendente dall'ordine.
	bool bChanged = true;
	while (bChanged)
	{
		bChanged = false;

		// 1) Destinazione contesa: se 2+ unita' finiscono sulla stessa cella, quelle in
		//    movimento tornano ferme (una cella occupata da chi resta ferma blocca comunque).
		TMap<FRTGridCoord, int32> Occupancy;
		for (int32 i = 0; i < Num; ++i)
		{
			Occupancy.FindOrAdd(Dest[i])++;
		}
		for (int32 i = 0; i < Num; ++i)
		{
			if (IsMoving(i) && Occupancy[Dest[i]] >= 2)
			{
				Dest[i] = Requests[i].From;
				bChanged = true;
			}
		}
		if (bChanged)
		{
			continue; // ricomputa l'occupazione dopo i "revert"
		}

		// 2) Bloccata da unita' ferma: un'unita' in movimento la cui destinazione e' la
		//    cella d'origine di un'unita' che resta ferma -> bloccata (scambi esclusi,
		//    perche' in uno scambio nessuna delle due e' ferma).
		for (int32 i = 0; i < Num; ++i)
		{
			if (!IsMoving(i))
			{
				continue;
			}
			for (int32 j = 0; j < Num; ++j)
			{
				if (i != j && !IsMoving(j) && Dest[i] == Requests[j].From)
				{
					Dest[i] = Requests[i].From;
					bChanged = true;
					break;
				}
			}
		}
	}

	return Dest;
}
