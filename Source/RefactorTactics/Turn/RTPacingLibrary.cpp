#include "Turn/RTPacingLibrary.h"

int32 URTPacingLibrary::PercentileNearestRank(const TArray<int32>& SortedValues, int32 Percentile)
{
	if (SortedValues.Num() == 0)
	{
		return 0;
	}
	const int32 P = FMath::Clamp(Percentile, 1, 100);
	// Rango 1-based = ceil(P/100 * N), su interi per non passare mai da un float.
	const int32 Rank = FMath::DivideAndRoundUp(P * SortedValues.Num(), 100);
	return SortedValues[FMath::Clamp(Rank - 1, 0, SortedValues.Num() - 1)];
}

FRTPacingSummary URTPacingLibrary::SummarizeSamples(const TArray<FRTPacingSample>& Samples, int32 CutoffWindowMs)
{
	FRTPacingSummary Out;
	Out.SampleCount = Samples.Num();
	if (Samples.Num() == 0)
	{
		return Out; // fail-closed: nessun campione -> tutto zero
	}

	TArray<int32> LockIn;
	TArray<int32> Playback;
	LockIn.Reserve(Samples.Num());
	Playback.Reserve(Samples.Num());

	for (const FRTPacingSample& S : Samples)
	{
		Playback.Add(S.MsPlayback); // il playback ha un cronometro suo: non dipende dall'apertura del campione

		// ⚠️ Un campione NON MISURATO non e' un lock-in rapido: e' l'assenza di una misura, e va tolto da
		// ogni statistica che risponde «quanto tempo». La sentinella e' negativa apposta, ma escluderla non
		// e' automatico — `Unmeasured < CutoffWindowMs` e' sempre vero, quindi senza questa guardia OGNI
		// timeout non misurato si classificherebbe come taglio del timer, che e' il segnale piu' forte che
		// questo sommario produce (`#1421`).
		if (S.MsToLockIn == FRTPacingSample::Unmeasured)
		{
			++Out.UnmeasuredSamples;
			continue;
		}
		LockIn.Add(S.MsToLockIn);

		if (S.LockInSource == ERTLockInSource::Timeout)
		{
			// Sotto soglia = il giocatore stava ancora agendo -> taglio. Dalla soglia in su -> attesa a vuoto.
			(S.MsSinceLastInput < CutoffWindowMs ? Out.TrueCutoffs : Out.IdleTimeouts)++;
		}
		if (S.bPlaybackSkipped)
		{
			++Out.SkippedPlaybacks;
		}
	}

	LockIn.Sort();
	Playback.Sort();
	Out.MedianMsToLockIn = PercentileNearestRank(LockIn, 50);
	Out.P90MsToLockIn = PercentileNearestRank(LockIn, 90);
	Out.MedianMsPlayback = PercentileNearestRank(Playback, 50);
	return Out;
}

FString URTPacingLibrary::CsvHeader()
{
	return TEXT("Turn,AliveT0,AliveT1,ActionsAvailable,MsToFirstInput,SelectionCount,OrderCount,")
		   TEXT("UndoCount,MsToLockIn,MsSinceLastInput,LockInSource,MsPlayback,PlaybackSkipped");
}

FString URTPacingLibrary::CsvRow(const FRTPacingSample& Sample)
{
	// Tutti %d: nessun float, quindi nessuna virgola decimale da locale che spezzi le colonne.
	return FString::Printf(TEXT("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d"),
		Sample.TurnNumber,
		Sample.UnitsAliveTeam0,
		Sample.UnitsAliveTeam1,
		Sample.ActionsAvailable,
		Sample.MsToFirstInput,
		Sample.SelectionCount,
		Sample.OrderCount,
		Sample.UndoCount,
		Sample.MsToLockIn,
		Sample.MsSinceLastInput,
		static_cast<int32>(Sample.LockInSource),
		Sample.MsPlayback,
		Sample.bPlaybackSkipped ? 1 : 0);
}
