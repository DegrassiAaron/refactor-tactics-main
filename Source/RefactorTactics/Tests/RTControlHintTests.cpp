// I suggerimenti dei gesti e il controllo di velocita' (D-538, #2764): UNA sede, due consumatori.
//
// `Spazio` (salta il playback), `V` (cicla la velocita'), `RMB` (annulla il Ready) e `P` (pausa della preparazione)
// sono cablati nel `PlayerController` e funzionano. I loro suggerimenti vivevano SOLO dentro
// `ARTHUD::ComposeMatchStatusLine`, che dal 2026-09-08 (#1936) non ha piu' un chiamante: i comandi erano muti, e
// nessun test lo diceva, perche' i test della riga di stato erano verdi.
//
// Qui si pinna la sede unica (`ComposeControlHint`, `ComposeControlLabel`) e il legame con la riga di stato, che la
// INCOLLA invece di avere una copia sua. Il legame col widget, che la mostra, sta in
// `ScreenHud.TurnHeaderShowsWhatTheKeysDoNow`.
//
// ATTENZIONE: i confronti sono `TestEqualSensitive` o `Equals(..., CaseSensitive)`. `TestEqual` su `FString` passa da
// `FCString::Stricmp` ed e' case-insensitive: `(RMB: annulla)` contro `(rmb: annulla)` deve cadere.

#include "Misc/AutomationTest.h"
#include "UI/RTHUD.h"
#include "UI/RTHudViewModel.h"

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
}

/**
 * Il suggerimento per ogni stato del turno, e la riga di stato che lo incolla.
 *
 * L'ultimo assert di ogni caso e' il punto: la riga di stato DEVE finire col suggerimento (senza obiettivo e' l'ultima
 * cosa che scrive). Se qualcuno reintroducesse in `ComposeMatchStatusLine` una copia del testo, con una grafia
 * diversa, il suggerimento e la riga smetterebbero di concordare e questo test cadrebbe.
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
		const FString Suggerimento = ARTHUD::ComposeControlHint(Caso.Header, Caso.Progresso);
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
 * La riga completa: suggerimento, separatore e controllo di velocita'. La velocita' si nomina SEMPRE, anche fuori dalla
 * risoluzione, ed e' una decisione d'autore antica (CP 47.7, #1015): chi guarda una partita non presidiata sceglie il
 * ritmo PRIMA che il round parta. Il tasto e' nominato accanto al valore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTControlLabelNamesTheSpeedKeyTest,
	"RefactorTactics.HUD.ControlLabelAlwaysNamesTheSpeedAndItsKey",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTControlLabelNamesTheSpeedKeyTest::RunTest(const FString&)
{
	const FRTMatchHeaderView Pianificazione = MakeControlHintHeader();

	// Fuori dalla risoluzione e senza altri gesti resta la sola velocita'. Non positiva vale `x1`: la stessa
	// convenzione di `EffectivePlaybackSpeed`, e un'etichetta `x0` direbbe che la riproduzione e' ferma.
	TestEqualSensitive(TEXT("velocita' non scelta: x1"),
		ARTHUD::ComposeControlLabel(Pianificazione, 0.f, 0.f), FString(TEXT("Velocit\u00e0: x1 (V)")));
	TestEqualSensitive(TEXT("x2"),
		ARTHUD::ComposeControlLabel(Pianificazione, 0.f, 2.f), FString(TEXT("Velocit\u00e0: x2 (V)")));
	TestEqualSensitive(TEXT("x4"),
		ARTHUD::ComposeControlLabel(Pianificazione, 0.f, 4.f), FString(TEXT("Velocit\u00e0: x4 (V)")));

	FRTMatchHeaderView Ready = MakeControlHintHeader();
	Ready.ReadyCountdownSecondsRemaining = 3.f;
	TestEqualSensitive(TEXT("Ready: il gesto, il separatore, la velocita'"),
		ARTHUD::ComposeControlLabel(Ready, 0.f, 1.f), FString(TEXT("(RMB: annulla)  -  Velocit\u00e0: x1 (V)")));

	FRTMatchHeaderView Risoluzione = MakeControlHintHeader();
	Risoluzione.bResolving = true;
	TestEqualSensitive(TEXT("risoluzione: avanzamento, gesto, velocita'"),
		ARTHUD::ComposeControlLabel(Risoluzione, 0.456f, 2.f),
		FString(TEXT("[46%]  (Spazio: salta)  -  Velocit\u00e0: x2 (V)")));

	// ANTI-VACUITA': i tre casi devono produrre righe DIVERSE fra loro. Una funzione che rendesse sempre la stessa
	// stringa passerebbe ogni confronto che la costruisse con la propria copia.
	TestNotEqual(TEXT("Ready e risoluzione non rendono la stessa riga"),
		ARTHUD::ComposeControlLabel(Ready, 0.f, 1.f), ARTHUD::ComposeControlLabel(Risoluzione, 0.f, 1.f));
	TestNotEqual(TEXT("ne' la pianificazione e il Ready"),
		ARTHUD::ComposeControlLabel(Pianificazione, 0.f, 1.f), ARTHUD::ComposeControlLabel(Ready, 0.f, 1.f));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
