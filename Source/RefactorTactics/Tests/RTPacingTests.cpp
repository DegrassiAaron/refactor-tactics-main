#include "Misc/AutomationTest.h"
#include "Turn/RTPacing.h"
#include "Turn/RTPacingLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Campione minimo: solo i campi che il test in questione guarda. */
	FRTPacingSample MakeSample(int32 MsToLockIn, ERTLockInSource Source, int32 MsSinceLastInput,
		int32 MsPlayback = 0, bool bSkipped = false)
	{
		FRTPacingSample S;
		S.MsToLockIn = MsToLockIn;
		S.LockInSource = Source;
		S.MsSinceLastInput = MsSinceLastInput;
		S.MsPlayback = MsPlayback;
		S.bPlaybackSkipped = bSkipped;
		return S;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingPercentileTest,
	"RefactorTactics.Pacing.PercentileNearestRank",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingPercentileTest::RunTest(const FString&)
{
	// Sette valori: il nearest-rank restituisce sempre un valore OSSERVATO, mai una media.
	// p50 -> rango ceil(0.50*7) = 4 -> quarto valore = 400. p90 -> ceil(0.90*7) = 7 -> settimo = 700.
	const TArray<int32> Sorted = { 100, 200, 300, 400, 500, 600, 700 };
	TestEqual(TEXT("p50 = quarto valore"), URTPacingLibrary::PercentileNearestRank(Sorted, 50), 400);
	TestEqual(TEXT("p90 = settimo valore"), URTPacingLibrary::PercentileNearestRank(Sorted, 90), 700);
	TestEqual(TEXT("p100 = ultimo valore"), URTPacingLibrary::PercentileNearestRank(Sorted, 100), 700);

	// Un solo campione: ogni percentile e' quel campione.
	const TArray<int32> One = { 42 };
	TestEqual(TEXT("p90 di un solo valore"), URTPacingLibrary::PercentileNearestRank(One, 90), 42);

	// Array vuoto: 0, nessun accesso fuori range.
	const TArray<int32> Empty;
	TestEqual(TEXT("array vuoto -> 0"), URTPacingLibrary::PercentileNearestRank(Empty, 50), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingEmptySummaryTest,
	"RefactorTactics.Pacing.SummaryOfEmptySampleIsZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingEmptySummaryTest::RunTest(const FString&)
{
	// Fail-closed: nessun campione -> tutto zero, nessuna divisione per zero, nessun indice fuori range.
	const FRTPacingSummary S = URTPacingLibrary::SummarizeSamples(TArray<FRTPacingSample>(), 3000);
	TestEqual(TEXT("nessun campione"), S.SampleCount, 0);
	TestEqual(TEXT("mediana 0"), S.MedianMsToLockIn, 0);
	TestEqual(TEXT("p90 0"), S.P90MsToLockIn, 0);
	TestEqual(TEXT("nessun taglio"), S.TrueCutoffs, 0);
	TestEqual(TEXT("nessuna attesa"), S.IdleTimeouts, 0);
	TestEqual(TEXT("nessun playback saltato"), S.SkippedPlaybacks, 0);
	TestEqual(TEXT("mediana playback 0"), S.MedianMsPlayback, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingCutoffTest,
	"RefactorTactics.Pacing.CutoffVsIdleTimeout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingCutoffTest::RunTest(const FString&)
{
	TArray<FRTPacingSample> Samples;
	// Timeout con input a 500 ms dalla fine: il timer ha TAGLIATO una decisione in corso.
	Samples.Add(MakeSample(30000, ERTLockInSource::Timeout, 500));
	// Timeout con ultimo input 12 s prima: il giocatore aveva finito, il timer e' scaduto A VUOTO.
	Samples.Add(MakeSample(30000, ERTLockInSource::Timeout, 12000));
	// Confine ESATTO: uguale alla soglia conta come attesa, non come taglio.
	Samples.Add(MakeSample(30000, ERTLockInSource::Timeout, 3000));
	// Lock-in manuale: non e' ne' l'uno ne' l'altro.
	Samples.Add(MakeSample(8000, ERTLockInSource::Input, 200, 4000, /*bSkipped=*/ true));

	const FRTPacingSummary S = URTPacingLibrary::SummarizeSamples(Samples, /*CutoffWindowMs=*/ 3000);
	TestEqual(TEXT("quattro campioni"), S.SampleCount, 4);
	TestEqual(TEXT("un solo taglio vero"), S.TrueCutoffs, 1);
	TestEqual(TEXT("due attese a vuoto (12 s e il confine esatto)"), S.IdleTimeouts, 2);
	TestEqual(TEXT("un playback saltato"), S.SkippedPlaybacks, 1);
	// Ordinati: 8000, 30000, 30000, 30000 -> p50 = rango ceil(0.50*4) = 2 -> 30000.
	TestEqual(TEXT("mediana del lock-in"), S.MedianMsToLockIn, 30000);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
