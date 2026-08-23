#include "Misc/AutomationTest.h"
#include "Frontend/RTMatchResultViewModel.h"
#include "Turn/RTTurnRules.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * CP 46.5 (#940) — il Result LEGGE il risultato canonico, non lo ricalcola.
 *
 * 🔴 **L'oracolo e' un risultato che un ricalcolo CORREGGEREBBE.** Il DoD dice: «la UI non ricalcola il
 * risultato… un secondo calcolo nel widget sarebbe una seconda autorita' che puo' divergere». Verificarlo
 * con un risultato coerente non proverebbe niente — lettura e ricalcolo darebbero lo stesso numero, e il
 * test resterebbe verde su entrambe le implementazioni.
 *
 * Qui l'esito dice «vince il team 0» mentre le unita' vive raccontano il contrario: chi legge riporta il
 * team 0, chi ricalcola risponde team 1. Solo cosi' le due implementazioni si distinguono.
 *
 * ⚠️ Non e' uno stato che il gioco produce — la condizione di fine partita e' di E10 e il TurnLog ne e' il
 * registro. E' uno stato costruito APPOSTA perche' il test possa vedere la differenza.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchResultReadsNotRecomputesTest,
	"RefactorTactics.Frontend.MatchResultReadsInsteadOfRecomputing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchResultReadsNotRecomputesTest::RunTest(const FString&)
{
	FRTMatchResult Canonical;
	Canonical.Outcome = ERTMatchOutcome::Team0Wins;
	Canonical.Reason = ERTMatchEndReason::Elimination;

	FRTMatchState State;
	State.Team0Alive = 0;   // ...e le unita' vive direbbero il contrario
	State.Team1Alive = 2;
	State.RoundNumber = 7;

	const FRTMatchResultViewModel VM = FRTMatchResultViewModel::From(Canonical, State);

	TestEqual(TEXT("l'esito e' quello CANONICO, non quello dedotto dalle unita' vive"),
		VM.Outcome, ERTMatchOutcome::Team0Wins);
	TestEqual(TEXT("e la via pure"), VM.Reason, ERTMatchEndReason::Elimination);
	TestEqual(TEXT("il numero di round arriva dallo stato"), VM.RoundNumber, 7);
	TestTrue(TEXT("e c'e' un vincitore dichiarato"), VM.bHasWinner);
	TestEqual(TEXT("che e' il team 0"), VM.WinningTeamId, 0);
	return true;
}

/**
 * Il pareggio non ha vincitore, e la differenza si vede.
 *
 * ⚠️ [D-184] dichiara il pareggio allo scadere un esito **legittimo** della v0.1: il Result deve saperlo
 * mostrare, non trattarlo come un caso degenere. `WinningTeamId` resta `INDEX_NONE` invece di scivolare a
 * `0`, che sarebbe indistinguibile da «vince il team 0».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchResultDrawHasNoWinnerTest,
	"RefactorTactics.Frontend.MatchResultDrawHasNoWinner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchResultDrawHasNoWinnerTest::RunTest(const FString&)
{
	FRTMatchResult Canonical;
	Canonical.Outcome = ERTMatchOutcome::Draw;
	Canonical.Reason = ERTMatchEndReason::RoundLimit;

	FRTMatchState State;
	State.RoundNumber = 12;

	const FRTMatchResultViewModel VM = FRTMatchResultViewModel::From(Canonical, State);

	TestFalse(TEXT("nessun vincitore"), VM.bHasWinner);
	TestEqual(TEXT("e l'id del vincitore non scivola a 0"), VM.WinningTeamId, (int32)INDEX_NONE);
	TestEqual(TEXT("la via e' lo scadere dei round"), VM.Reason, ERTMatchEndReason::RoundLimit);
	TestEqual(TEXT("e i round sono quelli giocati"), VM.RoundNumber, 12);
	return true;
}

/**
 * Una partita ancora in corso non e' un Result: il view model lo DICE.
 *
 * ⚠️ Senza questa distinzione un widget aperto troppo presto mostrerebbe «pareggio» — `InProgress` e
 * `Draw` sono entrambi «nessun vincitore», e confonderli e' il modo in cui una schermata di fine partita
 * appare a meta' turno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchResultInProgressIsNotAResultTest,
	"RefactorTactics.Frontend.MatchResultInProgressIsNotAResult",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchResultInProgressIsNotAResultTest::RunTest(const FString&)
{
	FRTMatchResult Canonical;   // default: InProgress / None
	FRTMatchState State;
	State.RoundNumber = 3;

	const FRTMatchResultViewModel VM = FRTMatchResultViewModel::From(Canonical, State);

	TestFalse(TEXT("la partita non e' finita"), VM.bIsFinished);
	TestFalse(TEXT("e non c'e' vincitore"), VM.bHasWinner);

	FRTMatchResult Finished;
	Finished.Outcome = ERTMatchOutcome::Draw;
	Finished.Reason = ERTMatchEndReason::RoundLimit;
	TestTrue(TEXT("mentre un pareggio E' una partita finita"),
		FRTMatchResultViewModel::From(Finished, State).bIsFinished);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
