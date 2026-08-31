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
	Samples.Add(MakeSample(10000, ERTLockInSource::Input,   4000,  5000));
	Samples.Add(MakeSample(20000, ERTLockInSource::Timeout, 500,   6000));  // taglio vero
	Samples.Add(MakeSample(30000, ERTLockInSource::Timeout, 25000, 7000));  // attesa a vuoto
	Samples.Add(MakeSample(Unmeasured, ERTLockInSource::Timeout, Unmeasured, 8000));
	Samples.Add(MakeSample(Unmeasured, ERTLockInSource::Input,   Unmeasured, 9000, /*bSkipped=*/ true));

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

	// Il PLAYBACK invece vale per tutti: ha un cronometro suo, che non dipende dall'apertura del campione.
	// Nearest-rank su {5000, 6000, 7000, 8000, 9000} -> p50 = rango 3 = 7000.
	TestEqual(TEXT("la mediana del playback usa tutti i campioni"), S.MedianMsPlayback, 7000);
	TestEqual(TEXT("e un playback saltato si conta anche se i tempi non erano misurati"),
		S.SkippedPlaybacks, 1);

	return true;
}

/**
 * Un campione cronometrato ma senza l'origine dell'ULTIMO INPUT non e' un taglio del timer.
 *
 * La guardia su `MsToLockIn` non basta: a decidere la classificazione e' `MsSinceLastInput`, e i due campi
 * possono divergere — una riga ricaricata da CSV, o un percorso futuro che cronometra il lock-in e perde
 * l'origine dell'input. `Unmeasured < CutoffWindowMs` e' sempre vero, quindi senza una guardia sua quel
 * campione diventerebbe `TrueCutoffs`: il segnale piu' forte del sommario, e quello che alza
 * `PlanningSeconds` nella regola di taratura.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingUnclassifiableTimeoutTest,
	"RefactorTactics.Pacing.UnmeasuredLastInputIsNotACutoff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingUnclassifiableTimeoutTest::RunTest(const FString&)
{
	TArray<FRTPacingSample> Samples;
	Samples.Add(MakeSample(12000, ERTLockInSource::Timeout, FRTPacingSample::Unmeasured));

	const FRTPacingSummary S = URTPacingLibrary::SummarizeSamples(Samples, /*CutoffWindowMs=*/ 3000);

	TestEqual(TEXT("il lock-in cronometrato entra nella mediana"), S.MedianMsToLockIn, 12000);
	TestEqual(TEXT("ma il timeout non e' un taglio"), S.TrueCutoffs, 0);
	TestEqual(TEXT("ne' un'attesa a vuoto: e' non classificabile"), S.IdleTimeouts, 0);
	return true;
}

/**
 * Nessun campione cronometrato -> la mediana dice di NON esserci, non «zero».
 *
 * Zero e' un lock-in istantaneo, cioe' il valore legittimo da cui `Unmeasured` esiste per distinguersi:
 * farlo tornare dal sommario rimetterebbe un piano piu' su lo stesso dato plausibile e falso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingAllUnmeasuredSummaryTest,
	"RefactorTactics.Pacing.SummaryOfAllUnmeasuredIsNotZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingAllUnmeasuredSummaryTest::RunTest(const FString&)
{
	const int32 Unmeasured = FRTPacingSample::Unmeasured;

	TArray<FRTPacingSample> Samples;
	Samples.Add(MakeSample(Unmeasured, ERTLockInSource::Input, Unmeasured, 4000));
	Samples.Add(MakeSample(Unmeasured, ERTLockInSource::Input, Unmeasured, 6000));

	const FRTPacingSummary S = URTPacingLibrary::SummarizeSamples(Samples, /*CutoffWindowMs=*/ 3000);

	TestEqual(TEXT("due turni"), S.SampleCount, 2);
	TestEqual(TEXT("nessuno cronometrato"), S.UnmeasuredSamples, 2);
	TestEqual(TEXT("la mediana lo dichiara"), S.MedianMsToLockIn, Unmeasured);
	TestEqual(TEXT("e il p90 pure"), S.P90MsToLockIn, Unmeasured);
	// Il playback resta misurato: nearest-rank su {4000, 6000} -> p50 = rango 1 = 4000.
	TestEqual(TEXT("il playback invece c'era"), S.MedianMsPlayback, 4000);
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
	S.ReactionWindowsOpened = 6; // due unita' armate dello stesso giocatore, tre passi in zona (CP 14.6)

	TArray<FString> HeaderCols;
	TArray<FString> RowCols;
	URTPacingLibrary::CsvHeader().ParseIntoArray(HeaderCols, TEXT(","), /*InCullEmpty=*/ false);
	URTPacingLibrary::CsvRow(S).ParseIntoArray(RowCols, TEXT(","), /*InCullEmpty=*/ false);

	// ⚠️ **Quattordici dal 2026-08-28**, con `ReactionWindows` aggiunta in CODA (CP 14.6, `#166`). Il numero
	// e' pinnato apposta: le colonne di questo CSV si leggono per POSIZIONE da fogli e script gia' scritti, e
	// una colonna inserita in mezzo sposterebbe ogni colonna a valle senza che nessun errore lo dica. Questo
	// test e' l'unico posto in cui quel movimento diventa visibile — ed e' cosi' che ha preso l'aggiunta.
	TestEqual(TEXT("quattordici colonne nell'intestazione"), HeaderCols.Num(), 14);
	TestEqual(TEXT("la riga ha le stesse colonne dell'intestazione"), RowCols.Num(), HeaderCols.Num());

	// Ogni colonna e' un intero: se un float si intrufolasse, con locale italiano stamperebbe una virgola
	// e spezzerebbe la riga in **quindici** colonne. Il controllo qui sopra lo prende; questo dice PERCHE'.
	// ⚠️ Il numero in questa frase segue il conteggio delle colonne: diceva «14» quando l'intestazione ne
	// aveva 13, ed e' rimasto indietro all'aggiunta della quattordicesima — cioe' spiegava il caso ROTTO
	// nominando quello sano. Se aggiungi una colonna, questa riga si aggiorna con l'assert sopra.
	for (const FString& Col : RowCols)
	{
		TestTrue(FString::Printf(TEXT("colonna intera: %s"), *Col), Col.IsNumeric() && !Col.Contains(TEXT(".")));
	}

	// I valori finiscono nelle colonne giuste, nell'ordine dichiarato.
	TestEqual(TEXT("prima colonna = turno"), RowCols[0], TEXT("7"));
	TestEqual(TEXT("nona colonna = MsToLockIn"), RowCols[8], TEXT("18400"));
	TestEqual(TEXT("undicesima colonna = LockInSource Input = 0"), RowCols[10], TEXT("0"));
	TestEqual(TEXT("tredicesima colonna = playback non saltato"), RowCols[12], TEXT("0"));

	// L'ultima, e il suo posto conta quanto il suo valore: se qualcuno la inserisse prima di
	// `PlaybackSkipped`, la riga sopra leggerebbe le finestre come un booleano e passerebbe comunque.
	TestEqual(TEXT("ultima colonna = finestre di reazione aperte"), RowCols[13], TEXT("6"));
	return true;
}

// --- ApplyOpeningCounts / RespondersForPacing ------------------------------------------------
//
// Le regole di conteggio del campione vivevano dentro due `GetAllActorsOfClass` in `ARTTurnManager`, e per
// esercitarle serviva spawnare un mondo (#1818). Qui girano headless, e i sei casi sotto sono le decisioni
// che erano scritte solo come commento: quale squadra alimenta `ActionsAvailable`, chi entra fra i
// responder, e chi ne resta fuori.

namespace
{
	/** Unita' vista dal pacing: squadra, id stabile, viva, abilita' utilizzabili. */
	FRTPacingUnitFacts Facts(int32 TeamId, int32 StableUnitId, bool bAlive, int32 Usable)
	{
		return FRTPacingUnitFacts(TeamId, StableUnitId, bAlive, Usable);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingOpeningCountsBothTeamsTest,
	"RefactorTactics.Pacing.AliveCountsBothTeamsButActionsOnlyTheMeasuredOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingOpeningCountsBothTeamsTest::RunTest(const FString&)
{
	// Due vive per parte. La squadra misurata e' la 0, e solo le sue abilita' contano.
	const TArray<FRTPacingUnitFacts> Units = {
		Facts(0, 1, true, 3), Facts(0, 2, true, 2),
		Facts(1, 3, true, 5), Facts(1, 4, true, 4),
	};

	FRTPacingSample Sample;
	URTPacingLibrary::ApplyOpeningCounts(Units, /*PacingTeamId*/ 0, Sample);

	TestEqual(TEXT("vive squadra 0"), Sample.UnitsAliveTeam0, 2);
	TestEqual(TEXT("vive squadra 1 — il campo descrive il CAMPO, non la squadra misurata"),
		Sample.UnitsAliveTeam1, 2);
	// 3+2 e non 3+2+5+4: le nove dell'avversario non sono spazio di decisione di chi decide.
	TestEqual(TEXT("azioni solo della squadra misurata"), Sample.ActionsAvailable, 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingOpeningCountsFollowsTeamTest,
	"RefactorTactics.Pacing.ActionsFollowTheMeasuredTeam",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingOpeningCountsFollowsTeamTest::RunTest(const FString&)
{
	// Stessi dati, squadra misurata diversa: e' il caso che distingue «conta la mia» da «conta la prima».
	const TArray<FRTPacingUnitFacts> Units = {
		Facts(0, 1, true, 3),
		Facts(1, 2, true, 7),
	};

	FRTPacingSample AsTeam1;
	URTPacingLibrary::ApplyOpeningCounts(Units, /*PacingTeamId*/ 1, AsTeam1);
	TestEqual(TEXT("misurando la squadra 1 si contano le SUE azioni"), AsTeam1.ActionsAvailable, 7);

	FRTPacingSample AsTeam0;
	URTPacingLibrary::ApplyOpeningCounts(Units, /*PacingTeamId*/ 0, AsTeam0);
	TestEqual(TEXT("misurando la squadra 0 si contano le sue"), AsTeam0.ActionsAvailable, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingOpeningCountsDeadTest,
	"RefactorTactics.Pacing.DeadUnitsBringNeitherPresenceNorActions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingOpeningCountsDeadTest::RunTest(const FString&)
{
	// Una caduta non e' sul campo e non ha azioni: esce da entrambi i conteggi di apertura.
	// (Nei RESPONDER invece resta — vedi il test dedicato: e' la differenza fra le due funzioni.)
	const TArray<FRTPacingUnitFacts> Units = {
		Facts(0, 1, true,  2),
		Facts(0, 2, false, 9),   // caduta, con abilita' che non deve portare
		Facts(1, 3, false, 4),
	};

	FRTPacingSample Sample;
	URTPacingLibrary::ApplyOpeningCounts(Units, /*PacingTeamId*/ 0, Sample);

	TestEqual(TEXT("solo la viva della squadra 0"), Sample.UnitsAliveTeam0, 1);
	TestEqual(TEXT("nessuna viva nella squadra 1"), Sample.UnitsAliveTeam1, 0);
	TestEqual(TEXT("le nove della caduta non entrano"), Sample.ActionsAvailable, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingRespondersKeepTheFallenTest,
	"RefactorTactics.Pacing.RespondersKeepTheUnitsThatFellDuringTheTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingRespondersKeepTheFallenTest::RunTest(const FString&)
{
	// 🔴 Il caso che giustifica l'assenza del filtro su `bIsAlive`: chi e' caduto DURANTE il turno ha
	// comunque potuto aprire finestre prima di cadere. Scartarlo farebbe sparire le attese dei turni piu'
	// concitati — quelle che tarano il bank.
	const TArray<FRTPacingUnitFacts> Units = {
		Facts(0, 1, true,  0),
		Facts(0, 2, false, 0),   // caduta nel turno: resta un responder
	};

	const TArray<int32> Responders = URTPacingLibrary::RespondersForPacing(Units, /*PacingTeamId*/ 0);
	TestEqual(TEXT("due responder, non uno"), Responders.Num(), 2);
	TestTrue(TEXT("la caduta e' fra i responder"), Responders.Contains(2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingRespondersExcludeZeroTest,
	"RefactorTactics.Pacing.RespondersExcludeTheReservedZeroId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingRespondersExcludeZeroTest::RunTest(const FString&)
{
	// [D-063]: lo 0 e' «nessuna unita' dichiarata», e un'unita' spawnata dopo il congelamento del roster lo
	// conserva. Con 0 nel set, una voce di log senza soggetto finirebbe nel bank del giocatore misurato.
	const TArray<FRTPacingUnitFacts> Units = {
		Facts(0, 0, true, 0),   // id non assegnato: NON e' un responder
		Facts(0, 5, true, 0),
	};

	const TArray<int32> Responders = URTPacingLibrary::RespondersForPacing(Units, /*PacingTeamId*/ 0);
	TestEqual(TEXT("un solo responder"), Responders.Num(), 1);
	TestFalse(TEXT("lo 0 non e' un id"), Responders.Contains(0));
	TestTrue(TEXT("il 5 c'e'"), Responders.Contains(5));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingRespondersFilterAndOrderTest,
	"RefactorTactics.Pacing.RespondersAreTheMeasuredTeamInStableOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingRespondersFilterAndOrderTest::RunTest(const FString&)
{
	// [D-167]: due unita' armate dello STESSO giocatore sono due finestre in fila su una persona; due di
	// squadre diverse sono due attese parallele. Senza il filtro il conteggio non sa quale sta misurando.
	// L'ordine e' crescente e non quello di raccolta: una telemetria non deve dipendere dall'ordine in cui
	// gli Actor sono stati trovati, nemmeno quando non decide nulla.
	const TArray<FRTPacingUnitFacts> Units = {
		Facts(0, 9, true, 0), Facts(1, 4, true, 0), Facts(0, 2, true, 0), Facts(1, 7, false, 0),
	};

	const TArray<int32> Responders = URTPacingLibrary::RespondersForPacing(Units, /*PacingTeamId*/ 0);
	TestEqual(TEXT("solo la squadra misurata"), Responders.Num(), 2);
	TestEqual(TEXT("ordinati: il 2 prima del 9, non nell'ordine di raccolta"), Responders[0], 2);
	TestEqual(TEXT("poi il 9"), Responders[1], 9);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
