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


/**
 * `#3115` — il piano dell'unita' che stai pianificando ADESSO si distingue da quello dell'altra tua.
 *
 * 🔴 **Il difetto che questo test chiude**: `bOwn` valeva `View.bIsAlly` e nient'altro, quindi le due
 * unita' di `Format.Skirmish2v2` — `UnitsPerPlayer = 2` su `UnitsPerTeam = 2`, un gruppo solo — ricevevano
 * lo STESSO prefisso e lo STESSO colore. Sulla mappa: due rotte ciano tratteggiate identiche, e nessun
 * segnale su quale delle due si stia scrivendo.
 *
 * ⚠️ L'anello di selezione (`ARTUnit::ShouldShowSelectionRing`) diceva gia' quale unita' e' selezionata.
 * Il suo PIANO no, ed e' li' che il giocatore guarda mentre pianifica una rotta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudIntentSelectedPlanTest,
	"RefactorTactics.HUD.IntentLineMarksTheSelectedPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudIntentSelectedPlanTest::RunTest(const FString&)
{
	const FRTIntentCertaintyStyle Style;

	// Due viste PROPRIE identiche in ogni campo: differiscono solo per la selezione, che non e' un campo
	// della vista. E' la geometria del difetto, riprodotta.
	FRTIntentView Vista;
	Vista.bIsAlly = true;
	Vista.bMoving = true;

	const FRTIntentPresentation Selezionata = ARTHUD::ComposeIntentPresentation(Vista, Style, /*bIsSelected=*/ true);
	const FRTIntentPresentation Altra       = ARTHUD::ComposeIntentPresentation(Vista, Style, /*bIsSelected=*/ false);

	// AC-1 — le due presentazioni differiscono su almeno un canale osservabile.
	TestNotEqual(TEXT("AC-1: il piano selezionato non ha la stessa etichetta dell'altro"),
		Selezionata.Label, Altra.Label);

	// AC-2 — la distinzione e' funzione della SELEZIONE, non dell'identita' dell'unita': la stessa vista
	// chiesta due volte come non selezionata da due volte la stessa cosa.
	const FRTIntentPresentation AltraDiNuovo = ARTHUD::ComposeIntentPresentation(Vista, Style, /*bIsSelected=*/ false);
	TestEqual(TEXT("AC-2: deselezionate, le due presentazioni coincidono"),
		Altra.Label, AltraDiNuovo.Label);

	// ⛔ AC-3, la meta' falsificante. Senza questa, AC-1 passerebbe anche promuovendo la selezione a una
	// TERZA classe di appartenenza — che e' il confine sbagliato: un nemico rivelato non diventa un'altra
	// cosa perche' io ho un'unita' selezionata.
	FRTIntentView Nemica;
	Nemica.bIsAlly = false;
	Nemica.bMoving = true;
	const FRTIntentPresentation NemicaConSel = ARTHUD::ComposeIntentPresentation(Nemica, Style, /*bIsSelected=*/ true);
	const FRTIntentPresentation NemicaSenza  = ARTHUD::ComposeIntentPresentation(Nemica, Style, /*bIsSelected=*/ false);
	TestEqual(TEXT("AC-3: il nemico rivelato non cambia con la selezione (etichetta)"),
		NemicaConSel.Label, NemicaSenza.Label);
	TestTrue(TEXT("AC-3: il nemico rivelato resta [REVEAL]"),
		NemicaConSel.Label.StartsWith(TEXT("[REVEAL] "), ESearchCase::CaseSensitive));

	// AC-4 — il canale NON e' il colore. Ciano e' l'identita' di squadra, e la coppia ciano/giallo e'
	// protetta dal gate `T9` con le distanze misurate in dicromazia ([D-233], [D-234]). Una stesura
	// precedente ci aveva gia' sovrapposto la certezza ed e' stata ritirata: due semantiche sullo stesso
	// canale, e la seconda pagata dalla prima.
	TestTrue(TEXT("AC-4: la selezione non tocca il colore"),
		Selezionata.Color.Equals(Altra.Color));
	TestTrue(TEXT("AC-4: il ciano di squadra resta quello"),
		Selezionata.Color.Equals(FLinearColor(0.2f, 0.9f, 1.f, 1.f)));

	// Il corpo dell'etichetta resta di `ComposeIntentLabel`: entrambe lo portano, e questa sede non lo
	// riscrive. Si verifica che il marcatore sia un PREFISSO, non una riscrittura del corpo.
	TestTrue(TEXT("selezionata: il corpo dell'etichetta e' ancora attaccato"),
		Selezionata.Label.EndsWith(ARTHUD::ComposeIntentLabel(Vista, Style)));
	TestTrue(TEXT("non selezionata: idem"),
		Altra.Label.EndsWith(ARTHUD::ComposeIntentLabel(Vista, Style)));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
