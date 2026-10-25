// La PAROLA della fase: `URTHudViewModel::ComposePhaseWord`, la sede unica che l'header UMG (`PhaseText`) mostra.
//
// Perche' questo file esiste (D-539): le sei parole - `Pianificazione`, `Preparazione`, `Preparazione (in pausa)`,
// `Ready`, `Risoluzione`, `Fine` - erano pinnate SOLO attraverso `ARTHUD::ComposeMatchStatusLine` (ritirata con D-539), una riga di stato
// senza chiamanti di produzione. Sul lato vivo (`PhaseText` dell'header) era pinnata una sola, `PIANIFICAZIONE`: ritirare
// la riga senza questo test avrebbe lasciato cinque parole e la loro PRECEDENZA senza nessuna guardia.
//
// La precedenza e' il punto: durante il countdown del Ready e durante la finestra di preparazione la fase e' ANCORA
// `Planning` (`#2358`, `#2386`), quindi una parola che dipendesse solo dalla fase direbbe «Pianificazione» mentre il piano
// sta per partire. I due stati devono distinguersi SENZA contare i secondi, e se sono armati insieme vince la preparazione.
//
// Il secondo test del file (`NoCommitClockOutsidePlanning`) pinna la guardia che la riga ritirata aveva e che il numero
// non aveva: nessun commit in arrivo fuori dal Planning.
//
// ATTENZIONE: i confronti sono `TestEqualSensitive`. `TestEqual` su `FString` e' case-insensitive: `Fine` contro
// `FINE` non cadrebbe.

#include "Misc/AutomationTest.h"
#include "UI/RTHudViewModel.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nome distinto da ogni altro file di test: la unity build condivide la translation unit.
	FRTMatchHeaderView MakePhaseWordHeader(ERTMatchPhase Phase)
	{
		FRTMatchHeaderView Header;
		Header.Phase = Phase;
		return Header;
	}
}

/**
 * Ogni stato ha la sua parola, e la precedenza fra i due orologi armati e' quella dichiarata.
 *
 * Il campo che dice «non si applica» e' NEGATIVO (`-1.f`, il default della vista): uno zero e' un valore vero, quindi il
 * caso `0.f` e' pinnato a parte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudPhaseWordTest,
	"RefactorTactics.HudViewModel.PhaseWordNamesTheClockInForce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudPhaseWordTest::RunTest(const FString&)
{
	const auto Parola = [](const FRTMatchHeaderView& Header) { return URTHudViewModel::ComposePhaseWord(Header); };

	// --- Planning e i suoi due stati interni -------------------------------------------------------------------------
	FRTMatchHeaderView Planning = MakePhaseWordHeader(ERTMatchPhase::Planning);
	TestEqualSensitive(TEXT("Planning senza altro orologio"), Parola(Planning), FString(TEXT("Pianificazione")));

	FRTMatchHeaderView Ready = Planning;
	Ready.ReadyCountdownSecondsRemaining = 3.f;
	TestEqualSensitive(TEXT("countdown del Ready armato: la parola cambia, la fase no"), Parola(Ready), FString(TEXT("Ready")));

	FRTMatchHeaderView ReadyAZero = Planning;
	ReadyAZero.ReadyCountdownSecondsRemaining = 0.f;
	TestEqualSensitive(TEXT("Ready a ZERO e' ancora Ready: zero e' «commit adesso», non «non si applica»"),
		Parola(ReadyAZero), FString(TEXT("Ready")));

	FRTMatchHeaderView Prep = Planning;
	Prep.PrepWindowSecondsRemaining = 12.f;
	TestEqualSensitive(TEXT("finestra di preparazione armata"), Parola(Prep), FString(TEXT("Preparazione")));

	FRTMatchHeaderView PrepInPausa = Prep;
	PrepInPausa.bPrepWindowPaused = true;
	TestEqualSensitive(TEXT("e in pausa lo dice"), Parola(PrepInPausa), FString(TEXT("Preparazione (in pausa)")));

	FRTMatchHeaderView Entrambi = Prep;
	Entrambi.ReadyCountdownSecondsRemaining = 3.f;
	TestEqualSensitive(TEXT("preparazione e Ready insieme: vince la preparazione"), Parola(Entrambi), FString(TEXT("Preparazione")));

	// --- Fuori da Planning gli orologi non parlano -----------------------------------------------------------------------
	// La parola dipende dalla fase, e gli orologi armati valgono solo dentro `Planning`.
	for (const ERTMatchPhase Fase : { ERTMatchPhase::Prep, ERTMatchPhase::Dash, ERTMatchPhase::Blast, ERTMatchPhase::Move,
		ERTMatchPhase::Cleanup })
	{
		FRTMatchHeaderView Interna = MakePhaseWordHeader(Fase);
		Interna.ReadyCountdownSecondsRemaining = 3.f;
		Interna.PrepWindowSecondsRemaining = 12.f;
		TestEqualSensitive(*FString::Printf(TEXT("una fase interna (%d) si legge Risoluzione, qualunque orologio sia armato"),
			static_cast<int32>(Fase)), Parola(Interna), FString(TEXT("Risoluzione")));
	}

	FRTMatchHeaderView Fine = MakePhaseWordHeader(ERTMatchPhase::MatchEnded);
	Fine.ReadyCountdownSecondsRemaining = 3.f;
	TestEqualSensitive(TEXT("a partita finita: Fine, anche con un orologio rimasto armato"), Parola(Fine), FString(TEXT("Fine")));

	// ANTI-VACUITA': le sei parole sono TUTTE diverse. Una funzione che rendesse sempre la stessa stringa passerebbe ogni
	// confronto che la costruisse con la propria copia.
	TSet<FString> Distinte;
	for (const FRTMatchHeaderView& Stato : { Planning, Ready, Prep, PrepInPausa, MakePhaseWordHeader(ERTMatchPhase::Blast), Fine })
	{
		Distinte.Add(Parola(Stato));
	}
	TestEqual(TEXT("sei stati, sei parole diverse"), Distinte.Num(), 6);

	return true;
}

/**
 * NESSUN COMMIT IN ARRIVO FUORI DAL PLANNING, NE' MENTRE LA RISOLUZIONE SCORRE (revisione di #3849).
 *
 * La riga di stato ritirata (D-539) lo garantiva con un `Phase == Planning` suo, e `ComputeSecondsUntilCommit` non lo
 * aveva: un secondo chiamante che costruisse la vista a mano - il viewer di replay - mostrava un conto alla rovescia
 * su una partita finita. Durante la riproduzione `Phase` resta `Planning`, quindi la distingue `bResolving`.
 *
 * Il CONTROLLO e' lo stesso header in Planning: senza, «negativo fuori dal Planning» passerebbe anche per una funzione
 * che risponde sempre negativo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudNoCommitClockOutsidePlanningTest,
	"RefactorTactics.HudViewModel.NoCommitClockOutsidePlanning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudNoCommitClockOutsidePlanningTest::RunTest(const FString&)
{
	// Tutti e tre gli orologi armati: nessuno deve poter far comparire un numero fuori dal Planning.
	const auto Armato = [](ERTMatchPhase Fase, bool bResolving)
	{
		FRTMatchHeaderView Header = MakePhaseWordHeader(Fase);
		Header.bResolving = bResolving;
		Header.PlanningSecondsRemaining = 5.f;
		Header.ReadyCountdownSecondsRemaining = 3.f;
		Header.PrepWindowSecondsRemaining = 12.f;
		return Header;
	};

	// Controllo positivo: in Planning, non in riproduzione, la finestra di preparazione ha la precedenza e il numero c'e'.
	TestNearlyEqual(TEXT("controllo: in Planning il commit e' il residuo della finestra di preparazione"),
		URTHudViewModel::ComputeSecondsUntilCommit(Armato(ERTMatchPhase::Planning, false)), 12.f, 0.01f);

	TestTrue(TEXT("durante la riproduzione (Phase resta Planning) non c'e' nessun commit in arrivo"),
		URTHudViewModel::ComputeSecondsUntilCommit(Armato(ERTMatchPhase::Planning, true)) < 0.f);

	for (const ERTMatchPhase Fase : { ERTMatchPhase::Prep, ERTMatchPhase::Dash, ERTMatchPhase::Blast, ERTMatchPhase::Move,
		ERTMatchPhase::Cleanup, ERTMatchPhase::MatchEnded })
	{
		TestTrue(*FString::Printf(TEXT("fase %d: nessun commit in arrivo, qualunque orologio sia armato"), static_cast<int32>(Fase)),
			URTHudViewModel::ComputeSecondsUntilCommit(Armato(Fase, false)) < 0.f);
	}

	// Il Planning col solo tetto, senza altri orologi: il numero e' quello del tetto.
	FRTMatchHeaderView SoloTetto = MakePhaseWordHeader(ERTMatchPhase::Planning);
	SoloTetto.PlanningSecondsRemaining = 5.f;
	TestNearlyEqual(TEXT("controllo: il solo tetto del Planning"),
		URTHudViewModel::ComputeSecondsUntilCommit(SoloTetto), 5.f, 0.01f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
