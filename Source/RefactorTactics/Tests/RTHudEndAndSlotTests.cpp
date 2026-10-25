// Una decisione di presentazione uscita da `ARTHUD::DrawHUD`: la headline di fine partita.
//
// Qui stava anche il colore della terna degli slot (`ComposeSlotLineStyle`), ritirato con D-538 (#2764) insieme
// alla terna che colorava: il Canvas era uscito con #1936 e il pannello UMG decide i propri colori con i token
// della palette (`Text_Primary`, `Text_Disabled`), con i suoi test.
//
// Perché esistono (#2184): entrambe vivevano nel metodo che il motore chiama ogni fotogramma e che **non
// ha copertura headless**. La verifica di chiusura del **2026-09-05** classificava i 29 condizionali *di
// allora* e dava queste due per le sole decisioni pure rimaste; il resto erano guardie di camera, null,
// CVar, o scelte che leggono `Map`.
//
// ⌫ **Questa riga diceva «sono l'ultimo residuo estraibile», e non lo erano.** Lo stesso giorno `#2386`
// ha aggiunto a `DrawHUD` la scelta della prospettiva da cui chiedere le viste d'intento, che è anch'essa
// una pura funzione del modello ed è uscita il 2026-09-24 in `ComposeVisibleIntentViews`. Il difetto non
// era la classificazione: era il tempo verbale. 🔑 **Una verifica di chiusura misura uno stato che altre
// issue muovono**, quindi si data e si rifà — non si scrive al presente come se fosse una proprietà.
//
// ⚠️ **`TestEqualSensitive` e non `TestEqual`**: su stringhe `TestEqual` passa da `FCString::Stricmp`
// (`AutomationTest.cpp:2163`) ed è case-insensitive.

#include "Misc/AutomationTest.h"
#include "UI/RTHUD.h"
#include "Turn/RTTurnRules.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * La headline di fine partita: **l'esito e la VIA che l'ha determinato**, in quest'ordine.
 *
 * 🔴 È la regola di CP 10.3, e il commento nel codice la enuncia da sempre: *«Vince il team 0» da solo non
 * distingue un'eliminazione da un punto di vantaggio allo scadere dei round*. La regola non è che i due
 * pezzi esistano — esistono già in `URTTurnRules` — è che compaiano **entrambi**, **in quest'ordine** e
 * **separati**, e finora niente lo teneva fermo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudMatchEndHeadlineTest,
	"RefactorTactics.HUD.MatchEndHeadlineNamesOutcomeThenReason",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudMatchEndHeadlineTest::RunTest(const FString&)
{
	// Eliminazione.
	TestEqualSensitive(TEXT("headline completa, con il separatore"),
		ARTHUD::ComposeMatchEndHeadline(FRTMatchResult(ERTMatchOutcome::Team0Wins, ERTMatchEndReason::Elimination)),
		FString(TEXT("Vince il team 0 (blu) - per eliminazione")));

	// 🔴 **L'ordine è la regola.** Lo stesso esito con una via diversa deve leggersi come la stessa vittoria
	// ottenuta in un altro modo: se i due pezzi si invertissero, la frase nominerebbe la causa prima del
	// fatto e le due partite si distinguerebbero peggio, non meglio.
	const FString AlloScadere = ARTHUD::ComposeMatchEndHeadline(
		FRTMatchResult(ERTMatchOutcome::Team0Wins, ERTMatchEndReason::RoundLimit));
	TestTrue(TEXT("l'esito viene prima della via"),
		AlloScadere.StartsWith(TEXT("Vince il team 0"), ESearchCase::CaseSensitive));
	TestTrue(TEXT("e la via c'e'"), AlloScadere.Contains(TEXT("allo scadere dei round"), ESearchCase::CaseSensitive));

	// Lo stesso esito, due vie: è precisamente la distinzione che CP 10.3 chiede e che l'esito da solo
	// non porta.
	TestFalse(TEXT("due vie per lo stesso esito non danno la stessa frase"),
		AlloScadere.Equals(ARTHUD::ComposeMatchEndHeadline(
			FRTMatchResult(ERTMatchOutcome::Team0Wins, ERTMatchEndReason::Elimination))));

	TestEqualSensitive(TEXT("pareggio per obiettivo"),
		ARTHUD::ComposeMatchEndHeadline(FRTMatchResult(ERTMatchOutcome::Draw, ERTMatchEndReason::Objective)),
		FString(TEXT("Pareggio - per obiettivo")));

	// ⚠️ **STATO DI FATTO, non una regola desiderabile.** Con una partita non finita la headline rende
	// «Partita in corso - nessuna via», che è una frase senza senso. Oggi è IRRAGGIUNGIBILE perché
	// `DrawHUD` chiama questa funzione solo dentro `GetPhase() == ERTMatchPhase::MatchEnded`: il gate e la
	// composizione stanno in due posti, e questo caso registra l'accoppiamento invece di lasciarlo implicito.
	//
	// ⛔ Non è stato «corretto» rendendo stringa vuota: sarebbe AGGIUNGERE una reticenza che oggi non
	// esiste, e #2184 mette fuori scope «cambiare ciò che si vede». Se un secondo chiamante arriva — il
	// viewer di replay nomina già le viste dell'HUD — quella diventa una decisione da prendere, e questo
	// test è il posto dove qualcuno se ne accorgerà.
	TestEqualSensitive(TEXT("partita non finita: la frase resta quella di oggi, senza senso e irraggiungibile"),
		ARTHUD::ComposeMatchEndHeadline(FRTMatchResult(ERTMatchOutcome::InProgress, ERTMatchEndReason::None)),
		FString(TEXT("Partita in corso - nessuna via")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
