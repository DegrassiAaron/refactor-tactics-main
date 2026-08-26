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

/**
 * Un campione NON MISURATO non entra nelle statistiche, e viene contato a parte.
 *
 * 🔴 E' la meta' della correzione di `#1421` che sta a valle della sonda: dichiarare «non misurato» nel
 * campione non serve a niente se poi l'aggregatore lo legge come un numero. Con la sentinella a `-1`:
 *
 * - finirebbe in `MedianMsToLockIn`/`P90MsToLockIn` come il lock-in piu' rapido mai visto, tirando giu' la
 *   mediana di una serie in cui non ha mai smesso di non significare niente;
 * - e soprattutto `-1 < CutoffWindowMs` e' **sempre vero**, quindi ogni timeout non misurato verrebbe
 *   classificato `TrueCutoffs` — cioe' come il segnale che questa metrica esiste per catturare («il timer ha
 *   tagliato una decisione in corso»). Non un outlier riconoscibile: la classificazione che si inverte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingUnmeasuredSummaryTest,
	"RefactorTactics.Pacing.SummaryIgnoresUnmeasuredSamples",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingUnmeasuredSummaryTest::RunTest(const FString&)
{
	const int32 Unmeasured = FRTPacingSample::Unmeasured;

	TArray<FRTPacingSample> Samples;
	Samples.Add(MakeSample(10000, ERTLockInSource::Input,   4000));
	Samples.Add(MakeSample(20000, ERTLockInSource::Timeout, 500));   // taglio vero
	Samples.Add(MakeSample(30000, ERTLockInSource::Timeout, 25000)); // attesa a vuoto
	Samples.Add(MakeSample(Unmeasured, ERTLockInSource::Timeout, Unmeasured));
	Samples.Add(MakeSample(Unmeasured, ERTLockInSource::Input,   Unmeasured));

	const FRTPacingSummary S = URTPacingLibrary::SummarizeSamples(Samples, /*CutoffWindowMs=*/ 3000);

	TestEqual(TEXT("i campioni ci sono tutti: il conteggio dei turni non cambia"), S.SampleCount, 5);
	TestEqual(TEXT("e due dicono di non essere stati misurati"), S.UnmeasuredSamples, 2);

	// Mediana e p90 sui TRE misurati: nearest-rank su {10000, 20000, 30000} -> p50 = 20000, p90 = 30000.
	// Con i non misurati dentro sarebbero 10000 e 30000: la mediana dimezzata da due valori che non sono
	// tempi.
	TestEqual(TEXT("mediana sui soli misurati"), S.MedianMsToLockIn, 20000);
	TestEqual(TEXT("p90 sui soli misurati"), S.P90MsToLockIn, 30000);

	TestEqual(TEXT("un taglio vero, e il timeout non misurato non lo diventa"), S.TrueCutoffs, 1);
	TestEqual(TEXT("un'attesa a vuoto"), S.IdleTimeouts, 1);

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingCsvTest,
	"RefactorTactics.Pacing.CsvRowMatchesHeader",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingCsvTest::RunTest(const FString&)
{
	FRTPacingSample S;
	S.TurnNumber = 7;
	S.UnitsAliveTeam0 = 2;
	S.UnitsAliveTeam1 = 1;
	S.ActionsAvailable = 6;
	S.MsToFirstInput = 1200;
	S.SelectionCount = 3;
	S.OrderCount = 2;
	S.UndoCount = 1;
	S.MsToLockIn = 18400;
	S.MsSinceLastInput = 900;
	S.LockInSource = ERTLockInSource::Input;
	S.MsPlayback = 5300;
	S.bPlaybackSkipped = false;

	TArray<FString> HeaderCols;
	TArray<FString> RowCols;
	URTPacingLibrary::CsvHeader().ParseIntoArray(HeaderCols, TEXT(","), /*InCullEmpty=*/ false);
	URTPacingLibrary::CsvRow(S).ParseIntoArray(RowCols, TEXT(","), /*InCullEmpty=*/ false);

	TestEqual(TEXT("tredici colonne nell'intestazione"), HeaderCols.Num(), 13);
	TestEqual(TEXT("la riga ha le stesse colonne dell'intestazione"), RowCols.Num(), HeaderCols.Num());

	// Ogni colonna e' un intero: se un float si intrufolasse, con locale italiano stamperebbe una virgola
	// e spezzerebbe la riga in 14 colonne. Il controllo qui sopra lo prende; questo dice PERCHE'.
	for (const FString& Col : RowCols)
	{
		TestTrue(FString::Printf(TEXT("colonna intera: %s"), *Col), Col.IsNumeric() && !Col.Contains(TEXT(".")));
	}

	// I valori finiscono nelle colonne giuste, nell'ordine dichiarato.
	TestEqual(TEXT("prima colonna = turno"), RowCols[0], TEXT("7"));
	TestEqual(TEXT("nona colonna = MsToLockIn"), RowCols[8], TEXT("18400"));
	TestEqual(TEXT("undicesima colonna = LockInSource Input = 0"), RowCols[10], TEXT("0"));
	TestEqual(TEXT("ultima colonna = playback non saltato"), RowCols[12], TEXT("0"));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
