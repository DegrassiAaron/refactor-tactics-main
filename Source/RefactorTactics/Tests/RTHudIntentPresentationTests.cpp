// La riga d'intento sopra la testa: chi ce l'ha, con che prefisso, di che colore.
//
// Perché esiste (#2184): le tre decisioni vivevano dentro `ARTHUD::DrawHUD`, che il motore chiama ogni
// fotogramma e che **non ha copertura headless**. Sono scritte in mezzo alla proiezione, ma non dipendono
// da lei: `bOwn` è `View.bIsAlly` e `bHasPlan` è un `||` di cinque campi della vista — nessuna tocca
// `Project()`, `Canvas` o una coordinata.
//
// 🔴 **La decisione che nessuno misurava è un'ASIMMETRIA.** `bOwn && !bHasPlan` salta **solo** le proprie
// unità senza ordini: un nemico rivelato che non ha ancora pianificato resta annunciato, perché il fatto
// stesso di vederlo è l'informazione. La lettura ingenua — «senza piano, niente riga» — è simmetrica ed è
// sbagliata, e senza un caso che la tenga ferma nessuno se ne accorgerebbe.
//
// ⚠️ **`TestEqualSensitive` e non `TestEqual`**: su stringhe `TestEqual` passa da `FCString::Stricmp`
// (`AutomationTest.cpp:2163`) ed è case-insensitive. `[PIANO]` contro `[Piano]` deve cadere.
//
// ⛔ Non si verifica qui il CORPO dell'etichetta: quello lo compone `ARTHUD::ComposeIntentLabel`, che ha
// già i suoi test in `RTIntentPrivacyTests`. Qui si verifica ciò che le sta intorno — il prefisso, la
// presenza della riga, il colore.

#include "Misc/AutomationTest.h"
#include "UI/RTHUD.h"
#include "Turn/RTIntentPrivacyLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Chi ha una riga d'intento, e l'asimmetria fra le proprie unità e il nemico rivelato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudIntentPresenceTest,
	"RefactorTactics.HUD.IntentLineSkipsOwnIdleUnitsButNeverRevealedEnemies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudIntentPresenceTest::RunTest(const FString&)
{
	const FRTIntentCertaintyStyle Style;

	// Unità PROPRIA senza alcun ordine: nessuna riga. Una riga vuota sopra ogni unità inattiva
	// trasformerebbe la HUD in rumore, ed è il caso più comune del gioco fuori dalla pianificazione.
	FRTIntentView Ferma;
	Ferma.bIsAlly = true;
	TestFalse(TEXT("unita' propria senza ordini: nessuna riga"),
		ARTHUD::ComposeIntentPresentation(Ferma, Style).bShow);

	// 🔴 **L'asimmetria.** Stessa vista, ma di un nemico RIVELATO: la riga c'è lo stesso, perché
	// l'informazione non è «cosa farà» — è «lo sto vedendo». Togliendo `bOwn` dalla guardia la regola
	// diventa simmetrica e questo caso è l'unico che se ne accorge.
	FRTIntentView NemicoFermo;
	NemicoFermo.bIsAlly = false;
	TestTrue(TEXT("nemico rivelato senza piano: la riga c'e' lo stesso"),
		ARTHUD::ComposeIntentPresentation(NemicoFermo, Style).bShow);

	// Ognuno dei cinque segnali di piano basta da solo a mostrare la riga di un'unità propria.
	{
		FRTIntentView V; V.bIsAlly = true; V.bMoving = true;
		TestTrue(TEXT("un movimento basta"), ARTHUD::ComposeIntentPresentation(V, Style).bShow);
	}
	{
		FRTIntentView V; V.bIsAlly = true; V.bHasTarget = true;
		TestTrue(TEXT("un bersaglio basta"), ARTHUD::ComposeIntentPresentation(V, Style).bShow);
	}
	{
		FRTIntentView V; V.bIsAlly = true; V.ActionName = FText::FromString(TEXT("Scatto"));
		TestTrue(TEXT("un'azione basta"), ARTHUD::ComposeIntentPresentation(V, Style).bShow);
	}
	{
		FRTIntentView V; V.bIsAlly = true; V.bDashing = true;
		TestTrue(TEXT("uno scatto basta"), ARTHUD::ComposeIntentPresentation(V, Style).bShow);
	}
	// 🔴 La reazione armata e' l'unico dei cinque che non si vede muovere sulla mappa, ed e' quindi il
	// termine piu' facile da perdere in una riscrittura del `||`.
	{
		FRTIntentView V; V.bIsAlly = true; V.ReactionName = FText::FromString(TEXT("Contrattacco"));
		TestTrue(TEXT("una reazione armata da sola basta a mostrare la riga"),
			ARTHUD::ComposeIntentPresentation(V, Style).bShow);
	}

	return true;
}

/**
 * Il prefisso dice **di chi è** il piano, e il colore lo ripete.
 *
 * ⚠️ Due canali per lo stesso confine non è ridondanza sprecata: l'etichetta è piccola e sta sopra una
 * mappa, e il colore regge dove il testo non si legge. Sbagliarne **uno solo** è peggio che sbagliarli
 * entrambi, perché produce due segnali che dissentono.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudIntentOwnershipTest,
	"RefactorTactics.HUD.IntentLineNamesWhoseThePlanIsTwice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudIntentOwnershipTest::RunTest(const FString&)
{
	const FRTIntentCertaintyStyle Style;

	FRTIntentView Mia;
	Mia.bIsAlly = true;
	Mia.bMoving = true;
	const FRTIntentPresentation Propria = ARTHUD::ComposeIntentPresentation(Mia, Style);

	FRTIntentView Sua;
	Sua.bIsAlly = false;
	Sua.bMoving = true;
	const FRTIntentPresentation Nemica = ARTHUD::ComposeIntentPresentation(Sua, Style);

	// Il prefisso: `[PIANO]` è una mia intenzione, `[REVEAL]` è un'informazione strappata all'avversario.
	// Scambiarli fa credere al giocatore di guardare la cosa sbagliata.
	TestTrue(TEXT("propria: prefisso [PIANO]"), Propria.Label.StartsWith(TEXT("[PIANO] "), ESearchCase::CaseSensitive));
	TestTrue(TEXT("nemico rivelato: prefisso [REVEAL]"), Nemica.Label.StartsWith(TEXT("[REVEAL] "), ESearchCase::CaseSensitive));
	TestFalse(TEXT("i due prefissi non si confondono"), Propria.Label.Contains(TEXT("REVEAL")));

	// Il colore: ciano le mie, giallo il nemico rivelato.
	TestTrue(TEXT("propria: ciano"), Propria.Color.Equals(FLinearColor(0.2f, 0.9f, 1.f, 1.f)));
	TestTrue(TEXT("nemico rivelato: giallo"), Nemica.Color.Equals(FLinearColor(1.f, 0.9f, 0.2f, 1.f)));
	TestFalse(TEXT("i due colori non collassano"), Propria.Color.Equals(Nemica.Color));

	// ⛔ Il CORPO dell'etichetta resta di `ComposeIntentLabel`: qui si verifica solo che ci sia attaccato,
	// non cosa dice. Duplicare qui le sue asserzioni significherebbe due sedi per la stessa regola.
	TestTrue(TEXT("il corpo dell'etichetta e' attaccato al prefisso"),
		Propria.Label.Len() > FString(TEXT("[PIANO] ")).Len());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
