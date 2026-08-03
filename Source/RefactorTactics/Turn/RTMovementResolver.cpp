#include "Turn/RTMovementResolver.h"

TArray<FRTPathResult> URTMovementResolver::ResolvePaths(const TArray<TArray<FRTGridCoord>>& Paths)
{
	const int32 N = Paths.Num();
	TArray<FRTPathResult> Results;
	Results.SetNum(N);

	TArray<FRTGridCoord> Pos;   Pos.SetNum(N);   // posizione corrente
	TArray<int32> Prog;         Prog.SetNum(N);  // indice nel path
	TArray<bool> Done;          Done.SetNum(N);  // path esaurito / nessun movimento
	for (int32 i = 0; i < N; ++i)
	{
		Pos[i] = Paths[i].Num() > 0 ? Paths[i][0] : FRTGridCoord();
		Prog[i] = 0;
		Done[i] = Paths[i].Num() <= 1;
		Results[i].Final = Pos[i];
	}

	// Motivo dell'ultimo congelamento per unita' (per l'outcome del TurnLog).
	TArray<ERTMoveOutcome> BlockReason; BlockReason.Init(ERTMoveOutcome::BlockedByUnit, N);

	// Microstep sincroni: tutti avanzano di 1 cella, si risolvono le collisioni, si ripete.
	while (true)
	{
		TArray<FRTGridCoord> Target; Target.SetNum(N);
		TArray<bool> Moving;         Moving.SetNum(N);
		for (int32 i = 0; i < N; ++i)
		{
			Moving[i] = !Done[i];
			Target[i] = Done[i] ? Pos[i] : Paths[i][Prog[i] + 1];
		}

		// Punto fisso del microstep: congela chi non puo' muoversi (monotono -> ordine-indipendente).
		bool bChanged = true;
		while (bChanged)
		{
			bChanged = false;
			TArray<int32> ToFreeze;
			for (int32 i = 0; i < N; ++i)
			{
				if (!Moving[i]) { continue; }

				// Destinazione contesa: 2+ unita' in movimento verso la stessa cella.
				int32 Contenders = 0;
				for (int32 j = 0; j < N; ++j)
				{
					if (Moving[j] && Target[j] == Target[i]) { ++Contenders; }
				}
				bool bBlocked = (Contenders >= 2);

				// Bloccata da un'unita' che RESTA (done o congelata) sulla cella di destinazione.
				if (!bBlocked)
				{
					for (int32 j = 0; j < N; ++j)
					{
						if (j != i && !Moving[j] && Pos[j] == Target[i]) { bBlocked = true; break; }
					}
				}
				if (bBlocked)
				{
					ToFreeze.Add(i);
					BlockReason[i] = (Contenders >= 2) ? ERTMoveOutcome::BlockedContested : ERTMoveOutcome::BlockedByUnit;
				}
			}
			for (int32 Idx : ToFreeze) { Moving[Idx] = false; bChanged = true; }
		}

		// Applica i movimenti del microstep.
		bool bAnyMoved = false;
		for (int32 i = 0; i < N; ++i)
		{
			if (!Moving[i]) { continue; }
			Pos[i] = Target[i];
			Results[i].Entered.Add(Target[i]);
			Results[i].Final = Target[i];
			++Prog[i];
			if (Prog[i] >= Paths[i].Num() - 1) { Done[i] = true; }
			bAnyMoved = true;
		}
		if (!bAnyMoved) { break; }
	}

	// Classifica l'esito di ogni unita' per il TurnLog (dipende solo da Final/Paths: ordine-indipendente).
	for (int32 i = 0; i < N; ++i)
	{
		if (Paths[i].Num() <= 1) { Results[i].Outcome = ERTMoveOutcome::Stayed; }
		else if (Results[i].Final == Paths[i].Last()) { Results[i].Outcome = ERTMoveOutcome::Moved; }
		else { Results[i].Outcome = BlockReason[i]; }
	}

	return Results;
}

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
