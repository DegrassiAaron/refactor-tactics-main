#include "Turn/RTPredictiveLibrary.h"

namespace
{
	/**
	 * Una candidatura: al passo `Step`, l'unita' `UnitId` entra nella cella bloccata dal colpo `ShotIndex`.
	 *
	 * Non e' ancora un esito. Diventa un colpo a segno solo se, quando il suo turno arriva nell'ordine, il
	 * colpo non e' gia' stato consumato e la vittima non e' gia' stata fermata prima di quel passo.
	 */
	struct FCandidate
	{
		int32 Step = 0;
		int32 UnitId = INDEX_NONE;
		int32 ShotIndex = INDEX_NONE;
	};
}

TArray<FRTPredictiveResolution> URTPredictiveLibrary::ResolvePredictions(
	const TArray<FRTPredictiveShot>& Shots, const TArray<FRTHexMoveResult>& Moves)
{
	// Un esito per colpo, preinizializzato a WHIFF. E' il default giusto: un colpo che non trova nessuno non
	// e' un errore ne' un caso mancante, e' l'esito normale di una previsione sbagliata.
	TArray<FRTPredictiveResolution> Out;
	Out.SetNum(Shots.Num());
	for (int32 s = 0; s < Shots.Num(); ++s)
	{
		Out[s].ShotIndex = s;
	}

	// Quanti passi restano validi per ciascuna unita'. Parte dalla rotta piena: nessuno e' ancora fermato.
	TArray<int32> StepsKept;
	StepsKept.Reserve(Moves.Num());
	for (const FRTHexMoveResult& M : Moves)
	{
		StepsKept.Add(M.Entered.Num());
	}

	TArray<FCandidate> Candidates;
	for (int32 s = 0; s < Shots.Num(); ++s)
	{
		const FRTPredictiveShot& Shot = Shots[s];
		for (int32 u = 0; u < Moves.Num(); ++u)
		{
			// Solo gli ostili di CHI HA ARMATO: la cella bloccata non e' una mina, e' un colpo mirato.
			// Chi ha armato non puo' cogliere se stesso nemmeno passandoci sopra.
			if (u == Shot.SourceUnitId || !Shot.Hostiles.Contains(u))
			{
				continue;
			}
			const TArray<FRTCellId>& Entered = Moves[u].Entered;
			for (int32 k = 0; k < Entered.Num(); ++k)
			{
				if (Entered[k] == Shot.LockedCell)
				{
					Candidates.Add(FCandidate{ k, u, s });
				}
			}
		}
	}

	// Le tre chiavi sono TOTALI: passo, poi unita', poi colpo. Nessun pareggio residuo, quindi permutare
	// `Shots` o `Moves` non puo' cambiare l'esito — che e' esattamente cio' che `PermutationInvariant`
	// verifica, e la ragione per cui questa funzione non guarda ne' il mondo ne' l'orologio.
	Candidates.Sort([](const FCandidate& A, const FCandidate& B)
	{
		if (A.Step != B.Step)       { return A.Step < B.Step; }
		if (A.UnitId != B.UnitId)   { return A.UnitId < B.UnitId; }
		return A.ShotIndex < B.ShotIndex;
	});

	// I colpi si influenzano a vicenda: chi viene fermato al passo 2 non entra piu' da nessuna parte al 3.
	// Per questo si processa in ordine di PASSO e non un colpo alla volta fino in fondo — altrimenti due
	// colpi coglierebbero la stessa unita' su celle che il primo le ha impedito di raggiungere.
	for (const FCandidate& C : Candidates)
	{
		if (Out[C.ShotIndex].bMatched)
		{
			continue; // colpo gia' consumato: se ne arma uno per turno, e ha gia' sparato
		}
		if (C.Step >= StepsKept[C.UnitId])
		{
			continue; // la vittima e' stata fermata prima di arrivarci: quel passo non avviene piu'
		}

		Out[C.ShotIndex].bMatched = true;
		Out[C.ShotIndex].VictimUnitId = C.UnitId;
		// ENTRA nella cella e si ferma: `Step + 1` passi conservati, l'ultimo e' la cella bloccata. Non viene
		// respinta indietro — sarebbe un `Push`, cioe' un altro effetto con un altro numero.
		Out[C.ShotIndex].VictimStepsKept = C.Step + 1;
		StepsKept[C.UnitId] = C.Step + 1;
	}

	return Out;
}

void URTPredictiveLibrary::ApplyPredictionsToMoves(const TArray<FRTPredictiveResolution>& Resolutions,
	TArray<FRTHexMoveResult>& Moves)
{
	for (const FRTPredictiveResolution& R : Resolutions)
	{
		if (!R.bMatched || !Moves.IsValidIndex(R.VictimUnitId))
		{
			continue;
		}

		FRTHexMoveResult& M = Moves[R.VictimUnitId];
		if (R.VictimStepsKept >= M.Entered.Num())
		{
			continue; // colta sull'ultimo passo: non c'era nulla da troncare, la rotta e' gia' quella
		}

		M.Entered.SetNum(R.VictimStepsKept);
		// `Final` deve restare coerente con `Entered`, altrimenti l'unita' verrebbe piazzata dove sarebbe
		// arrivata se non l'avessero fermata, e il TurnLog racconterebbe un movimento che non e' avvenuto.
		M.Final = M.Entered.Last();
		M.Outcome = ERTMoveOutcome::StoppedByPrediction;
	}
}
