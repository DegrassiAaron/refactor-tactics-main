// Le ultime due decisioni di presentazione dentro `ARTHUD::DrawHUD`: il colore della terna degli slot e
// la headline di fine partita.
//
// Perché esistono (#2184): entrambe vivevano nel metodo che il motore chiama ogni fotogramma e che **non
// ha copertura headless**. Sono l'ultimo residuo estraibile — la verifica di chiusura del 2026-09-05 ha
// classificato i 29 condizionali rimasti e queste due sono le sole decisioni che siano pure funzioni del
// modello; il resto sono guardie di camera, null, CVar, o scelte che leggono `Map`.
//
// 🔴 **La terna è lo stesso schema della barra abilità.** `ComposeSlotLines` produce già
// `FRTSlotLine{Text, bOccupied}` ed è testata, ma il **colore derivato da `bOccupied`** era rimasto nel
// Canvas: il modello portava il dato e chi disegna ne ri-derivava la presentazione.
//
// ⚠️ **`TestEqualSensitive` e non `TestEqual`**: su stringhe `TestEqual` passa da `FCString::Stricmp`
// (`AutomationTest.cpp:2163`) ed è case-insensitive.

#include "Misc/AutomationTest.h"
#include "UI/RTHUD.h"
#include "Turn/RTTurnRules.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * La terna movimento / principale / reazione: bianco per lo slot speso, grigio per quello libero.
 *
 * ⚠️ **Il colore è l'unico canale**: il testo di uno slot libero (`«Reazione: libero»`) e di uno speso
 * (`«Reazione: Contrattacco»`) sono entrambi frasi compiute, quindi scambiare i colori non produce nulla
 * di visibilmente rotto — produce una lettura invertita, che è peggio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudSlotLineStyleTest,
	"RefactorTactics.HUD.SlotLineColorsSpentAndFreeApart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudSlotLineStyleTest::RunTest(const FString&)
{
	FRTSlotLine Speso;
	Speso.Text = TEXT("Reazione: Contrattacco");
	Speso.bOccupied = true;

	FRTSlotLine Libero;
	Libero.Text = TEXT("Reazione: libero");
	Libero.bOccupied = false;

	const FRTHudTextLine A = ARTHUD::ComposeSlotLineStyle(Speso);
	const FRTHudTextLine B = ARTHUD::ComposeSlotLineStyle(Libero);

	// Il testo passa intatto: questa funzione decide il COLORE, non riscrive l'etichetta — quella è di
	// `ComposeSlotLines`, che ha i suoi test.
	TestEqualSensitive(TEXT("il testo dello slot passa intatto"), A.Text, FString(TEXT("Reazione: Contrattacco")));
	TestEqualSensitive(TEXT("anche quello dello slot libero"), B.Text, FString(TEXT("Reazione: libero")));

	TestTrue(TEXT("slot occupato: bianco"), A.Color.Equals(FLinearColor::White));
	TestTrue(TEXT("slot libero: grigio"), B.Color.Equals(FLinearColor(0.55f, 0.55f, 0.55f, 1.f)));
	TestFalse(TEXT("i due colori non collassano"), A.Color.Equals(B.Color));

	// 🔴 **Il valore del grigio è pinnato di proposito.** Il commento accanto al codice afferma «la stessa
	// scala che la barra abilità usa già», ma quella usa `0.8` e `0.45` mentre la terna usa `0.55`: senza
	// questo caso l'affermazione non ha soggetto e il numero può derivare a ogni ritocco. Se un giorno le
	// due scale vanno davvero unificate, è questo test a doverlo dichiarare.
	TestTrue(TEXT("il grigio della terna e' 0.55"),
		FMath::IsNearlyEqual(B.Color.R, 0.55f) && FMath::IsNearlyEqual(B.Color.G, 0.55f)
			&& FMath::IsNearlyEqual(B.Color.B, 0.55f));

	return true;
}

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
