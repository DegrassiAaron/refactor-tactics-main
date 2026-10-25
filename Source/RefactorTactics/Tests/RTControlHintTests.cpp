// I suggerimenti dei gesti e il controllo di velocita' (D-538, #2764): UNA sede, due consumatori - e un tasto si
// nomina SOLO se risponde.
//
// `Spazio` (salta il playback), `V` (cicla la velocita'), `RMB` (annulla il Ready) e `P` (pausa della preparazione)
// sono cablati nel `PlayerController` e funzionano. I loro suggerimenti vivevano SOLO dentro
// `ARTHUD::ComposeMatchStatusLine`, che dal 2026-09-08 (#1936) non ha piu' un chiamante: i comandi erano muti, e
// nessun test lo diceva, perche' i test della riga di stato erano verdi.
//
// 🔴 La prima stesura nominava i tasti SEMPRE, e la revisione indipendente di #3847 ha mostrato che due su quattro
// possono essere inerti: `Spazio` e `RMB` in autobattle (`IsPlanningInputInert`), `V` quando chi guarda comanda
// un'unita' viva (D-350). Un suggerimento che promette un tasto che non risponde e' peggio di nessun suggerimento.
// Qui si pinna la sede unica (`ComposeControlHint`, `ComposeControlLabel`), che dipende da `FRTLiveGestures`, e il
// legame con la riga di stato, che la INCOLLA invece di avere una copia sua. Il legame col widget, che la mostra e
// che legge i gesti dalle sedi dei tasti, sta in `ScreenHud.TurnHeaderShowsWhatTheKeysDoNow`.
//
// ATTENZIONE: i confronti sono `TestEqualSensitive` o `Equals(..., CaseSensitive)`. `TestEqual` su `FString` passa da
// `FCString::Stricmp` ed e' case-insensitive: `(RMB: annulla)` contro `(rmb: annulla)` deve cadere.

#include "Misc/AutomationTest.h"
#include "UI/RTHUD.h"
#include "UI/RTHudViewModel.h"

#include <limits>

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nome distinto da ogni altro file di test: la unity build condivide la translation unit.
	FRTMatchHeaderView MakeControlHintHeader()
	{
		FRTMatchHeaderView Header;
		Header.Round = 3;
		Header.RoundLimit = 12;
		return Header;
	}

	// I tre gesti che dipendono dal contesto, scelti UNO PER UNO: la struct nasce tutta spenta.
	FRTLiveGestures MakeControlHintGestures(bool bSkip, bool bUnready, bool bSpeed)
	{
		FRTLiveGestures Gestures;
		Gestures.bSkip = bSkip;
		Gestures.bUnready = bUnready;
		Gestures.bSpeed = bSpeed;
		return Gestures;
	}
}

/**
 * Il suggerimento per ogni stato del turno, e la riga di stato che lo incolla.
 *
 * L'ultimo assert di ogni caso e' il punto: la riga di stato DEVE finire col suggerimento (senza obiettivo e' l'ultima
 * cosa che scrive). Se qualcuno reintroducesse in `ComposeMatchStatusLine` una copia del testo, con una grafia
 * diversa, il suggerimento e la riga smetterebbero di concordare e questo test cadrebbe.
 *
 * La riga di stato non conosce la sessione, quindi incolla il suggerimento con TUTTI i gesti vivi (`All()`): e'
 * proprio il comportamento che questo test fissa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTControlHintMatchesStatusLineTest,
	"RefactorTactics.HUD.ControlHintIsWhatTheStatusLineAppends",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTControlHintMatchesStatusLineTest::RunTest(const FString&)
{
	struct FCaso
	{
		const TCHAR* Nome;
		FRTMatchHeaderView Header;
		float Progresso;
		const TCHAR* Atteso;
	};

	FRTMatchHeaderView Pianificazione = MakeControlHintHeader();
	Pianificazione.PlanningSecondsRemaining = 21.f;

	FRTMatchHeaderView Ready = MakeControlHintHeader();
	Ready.ReadyCountdownSecondsRemaining = 3.f;

	FRTMatchHeaderView Prep = MakeControlHintHeader();
	Prep.PrepWindowSecondsRemaining = 12.f;

	FRTMatchHeaderView PrepInPausa = Prep;
	PrepInPausa.bPrepWindowPaused = true;

	FRTMatchHeaderView Risoluzione = MakeControlHintHeader();
	Risoluzione.bResolving = true;

	const FCaso Casi[] = {
		{ TEXT("pianificazione senza altro orologio"), Pianificazione, 0.f, TEXT("") },
		{ TEXT("countdown del Ready"), Ready, 0.f, TEXT("(RMB: annulla)") },
		{ TEXT("finestra di preparazione"), Prep, 0.f, TEXT("(P: pausa)") },
		{ TEXT("finestra di preparazione in pausa"), PrepInPausa, 0.f, TEXT("(P: riprendi)") },
		{ TEXT("risoluzione a meta'"), Risoluzione, 0.456f, TEXT("[46%]  (Spazio: salta)") },
		{ TEXT("risoluzione oltre il fondo scala"), Risoluzione, 1.7f, TEXT("[100%]  (Spazio: salta)") },
		{ TEXT("risoluzione con avanzamento negativo"), Risoluzione, -3.f, TEXT("[0%]  (Spazio: salta)") },
	};

	for (const FCaso& Caso : Casi)
	{
		const FString Suggerimento = ARTHUD::ComposeControlHint(Caso.Header, Caso.Progresso, FRTLiveGestures::All());
		TestEqualSensitive(*FString::Printf(TEXT("%s: il suggerimento"), Caso.Nome), Suggerimento, FString(Caso.Atteso));

		const FString Riga = ARTHUD::ComposeMatchStatusLine(Caso.Header, TEXT("Blast"), Caso.Progresso,
			/*bHasObjectiveCell=*/ false);
		if (Suggerimento.IsEmpty())
		{
			// Nessun gesto da nominare: la riga di stato non ne nomina nessuno.
			TestFalse(*FString::Printf(TEXT("%s: la riga di stato non nomina nessun gesto (\"%s\")"), Caso.Nome, *Riga),
				Riga.Contains(TEXT("(")) || Riga.Contains(TEXT("[")));
		}
		else
		{
			TestTrue(*FString::Printf(TEXT("%s: la riga di stato finisce col suggerimento (\"%s\")"), Caso.Nome, *Riga),
				Riga.EndsWith(Suggerimento, ESearchCase::CaseSensitive));
		}
	}

	return true;
}

/**
 * 🔴 **UN TASTO SI NOMINA SOLO SE FA QUALCOSA** (revisione indipendente di #3847).
 *
 * Il difetto: la prima stesura scriveva `(Spazio: salta)` e `(RMB: annulla)` anche in autobattle, dove
 * `OnLockIn` e il Back escono su `IsPlanningInputInert()` per non tagliare la registrazione. Ogni gesto dipende da
 * UN campo di `FRTLiveGestures` e da nessun altro; `P` non dipende da nessuno.
 *
 * Due controlli di sbieco, perche' un asserto «con il campo acceso compare» passa anche se il testo dipende da un
 * campo SBAGLIATO: ogni gesto e' provato con il solo campo degli altri accesi, e `P` con tutti spenti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHintNamesOnlyKeysThatWorkTest,
	"RefactorTactics.HUD.HintNamesOnlyKeysThatDoSomething",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHintNamesOnlyKeysThatWorkTest::RunTest(const FString&)
{
	FRTMatchHeaderView Risoluzione = MakeControlHintHeader();
	Risoluzione.bResolving = true;
	FRTMatchHeaderView Ready = MakeControlHintHeader();
	Ready.ReadyCountdownSecondsRemaining = 3.f;
	FRTMatchHeaderView Prep = MakeControlHintHeader();
	Prep.PrepWindowSecondsRemaining = 12.f;

	const auto Hint = [](const FRTMatchHeaderView& H, const FRTLiveGestures& G) { return ARTHUD::ComposeControlHint(H, 0.456f, G); };

	// --- Spazio: dipende da `bSkip` e da niente altro -----------------------------------------------------------------
	TestEqualSensitive(TEXT("Spazio vivo: la percentuale e il gesto"),
		Hint(Risoluzione, MakeControlHintGestures(true, false, false)), FString(TEXT("[46%]  (Spazio: salta)")));
	TestEqualSensitive(TEXT("Spazio inerte: resta la percentuale, che e' un dato e non un gesto"),
		Hint(Risoluzione, MakeControlHintGestures(false, false, false)), FString(TEXT("[46%]")));
	TestEqualSensitive(TEXT("gli ALTRI due campi accesi non riaccendono Spazio"),
		Hint(Risoluzione, MakeControlHintGestures(false, true, true)), FString(TEXT("[46%]")));

	// --- RMB: dipende da `bUnready` e da niente altro -----------------------------------------------------------------
	TestEqualSensitive(TEXT("RMB vivo: il gesto"),
		Hint(Ready, MakeControlHintGestures(false, true, false)), FString(TEXT("(RMB: annulla)")));
	TestEqualSensitive(TEXT("RMB inerte: niente, perche' non c'e' un gesto da nominare"),
		Hint(Ready, MakeControlHintGestures(false, false, false)), FString());
	TestEqualSensitive(TEXT("gli ALTRI due campi accesi non riaccendono RMB"),
		Hint(Ready, MakeControlHintGestures(true, false, true)), FString());

	// --- P: vivo sempre, anche in autobattle ----------------------------------------------------------------------------
	TestEqualSensitive(TEXT("P con tutti i campi spenti: la finestra di preparazione e' proprio dell'autobattle"),
		Hint(Prep, MakeControlHintGestures(false, false, false)), FString(TEXT("(P: pausa)")));
	FRTMatchHeaderView PrepInPausa = Prep;
	PrepInPausa.bPrepWindowPaused = true;
	TestEqualSensitive(TEXT("e cambia con lo stato, senza dipendere dai campi"),
		Hint(PrepInPausa, MakeControlHintGestures(false, false, false)), FString(TEXT("(P: riprendi)")));

	// --- Un avanzamento non finito non e' un traguardo -------------------------------------------------------------------
	// `FMath::Clamp` rende `Max` per un NaN: `[100%]` direbbe una riproduzione finita. Si legge come `0`.
	const FRTLiveGestures Tutti = FRTLiveGestures::All();
	TestEqualSensitive(TEXT("NaN: [0%] e non [100%]"),
		ARTHUD::ComposeControlHint(Risoluzione, std::numeric_limits<float>::quiet_NaN(), Tutti),
		FString(TEXT("[0%]  (Spazio: salta)")));
	TestEqualSensitive(TEXT("+inf: un valore non finito si legge come 0"),
		ARTHUD::ComposeControlHint(Risoluzione, std::numeric_limits<float>::infinity(), Tutti),
		FString(TEXT("[0%]  (Spazio: salta)")));
	TestEqualSensitive(TEXT("-inf: idem"),
		ARTHUD::ComposeControlHint(Risoluzione, -std::numeric_limits<float>::infinity(), Tutti),
		FString(TEXT("[0%]  (Spazio: salta)")));

	// --- Il default e' TUTTO SPENTO -----------------------------------------------------------------------------------------
	// Senza una lettura nessun tasto si nomina: e' il verso sicuro di un suggerimento.
	const FRTLiveGestures Nessuno;
	TestFalse(TEXT("default: Spazio spento"), Nessuno.bSkip);
	TestFalse(TEXT("default: RMB spento"), Nessuno.bUnready);
	TestFalse(TEXT("default: V spento"), Nessuno.bSpeed);
	TestTrue(TEXT("All(): tutti accesi"), Tutti.bSkip && Tutti.bUnready && Tutti.bSpeed);

	return true;
}

/**
 * La riga completa: suggerimento, separatore e controllo di velocita'. La velocita' si nomina SEMPRE, anche fuori dalla
 * risoluzione, ed e' una decisione d'autore antica (CP 47.7, #1015): chi guarda una partita non presidiata sceglie il
 * ritmo PRIMA che il round parta. Il TASTO `(V)` invece compare solo se `bSpeed`: da una partita presidiata la
 * manopola e' spenta (D-350) e promettere `V` sarebbe annunciare un tasto inerte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTControlLabelNamesTheSpeedKeyTest,
	"RefactorTactics.HUD.ControlLabelNamesTheSpeedAndTheKeyOnlyWhenTheKeyWorks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTControlLabelNamesTheSpeedKeyTest::RunTest(const FString&)
{
	const FRTMatchHeaderView Pianificazione = MakeControlHintHeader();
	const FRTLiveGestures Tutti = FRTLiveGestures::All();

	// Fuori dalla risoluzione e senza altri gesti resta la sola velocita'. Non positiva vale `x1`: la stessa
	// convenzione di `EffectivePlaybackSpeed`, e un'etichetta `x0` direbbe che la riproduzione e' ferma.
	TestEqualSensitive(TEXT("velocita' non scelta: x1"),
		ARTHUD::ComposeControlLabel(Pianificazione, 0.f, 0.f, Tutti), FString(TEXT("Velocit\u00e0: x1 (V)")));
	TestEqualSensitive(TEXT("x2"),
		ARTHUD::ComposeControlLabel(Pianificazione, 0.f, 2.f, Tutti), FString(TEXT("Velocit\u00e0: x2 (V)")));
	TestEqualSensitive(TEXT("x4"),
		ARTHUD::ComposeControlLabel(Pianificazione, 0.f, 4.f, Tutti), FString(TEXT("Velocit\u00e0: x4 (V)")));

	FRTMatchHeaderView Ready = MakeControlHintHeader();
	Ready.ReadyCountdownSecondsRemaining = 3.f;
	TestEqualSensitive(TEXT("Ready: il gesto, il separatore, la velocita'"),
		ARTHUD::ComposeControlLabel(Ready, 0.f, 1.f, Tutti), FString(TEXT("(RMB: annulla)  -  Velocit\u00e0: x1 (V)")));

	FRTMatchHeaderView Risoluzione = MakeControlHintHeader();
	Risoluzione.bResolving = true;
	TestEqualSensitive(TEXT("risoluzione: avanzamento, gesto, velocita'"),
		ARTHUD::ComposeControlLabel(Risoluzione, 0.456f, 2.f, Tutti),
		FString(TEXT("[46%]  (Spazio: salta)  -  Velocit\u00e0: x2 (V)")));

	// --- Il tasto `V` si toglie quando e' inerte, il VALORE resta ------------------------------------------------------
	const FRTLiveGestures SenzaV = MakeControlHintGestures(true, true, false);
	TestEqualSensitive(TEXT("partita presidiata: la velocita' si legge, il tasto non si promette"),
		ARTHUD::ComposeControlLabel(Pianificazione, 0.f, 2.f, SenzaV), FString(TEXT("Velocit\u00e0: x2")));
	TestEqualSensitive(TEXT("e la risoluzione conserva il salto, che e' vivo"),
		ARTHUD::ComposeControlLabel(Risoluzione, 0.456f, 2.f, SenzaV),
		FString(TEXT("[46%]  (Spazio: salta)  -  Velocit\u00e0: x2")));

	// In autobattle e' il contrario: `V` e' viva, `Spazio` e `RMB` no.
	const FRTLiveGestures Autobattle = MakeControlHintGestures(false, false, true);
	TestEqualSensitive(TEXT("autobattle, risoluzione: niente Spazio, ma V si"),
		ARTHUD::ComposeControlLabel(Risoluzione, 0.456f, 2.f, Autobattle), FString(TEXT("[46%]  -  Velocit\u00e0: x2 (V)")));
	TestEqualSensitive(TEXT("autobattle, Ready: niente RMB, resta la velocita' con V"),
		ARTHUD::ComposeControlLabel(Ready, 0.f, 1.f, Autobattle), FString(TEXT("Velocit\u00e0: x1 (V)")));

	// ANTI-VACUITA': i casi devono produrre righe DIVERSE fra loro. Una funzione che rendesse sempre la stessa
	// stringa passerebbe ogni confronto che la costruisse con la propria copia.
	TestNotEqual(TEXT("Ready e risoluzione non rendono la stessa riga"),
		ARTHUD::ComposeControlLabel(Ready, 0.f, 1.f, Tutti), ARTHUD::ComposeControlLabel(Risoluzione, 0.f, 1.f, Tutti));
	TestNotEqual(TEXT("ne' la pianificazione e il Ready"),
		ARTHUD::ComposeControlLabel(Pianificazione, 0.f, 1.f, Tutti), ARTHUD::ComposeControlLabel(Ready, 0.f, 1.f, Tutti));
	TestNotEqual(TEXT("ne' V viva e V inerte"),
		ARTHUD::ComposeControlLabel(Pianificazione, 0.f, 1.f, Tutti), ARTHUD::ComposeControlLabel(Pianificazione, 0.f, 1.f, SenzaV));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
