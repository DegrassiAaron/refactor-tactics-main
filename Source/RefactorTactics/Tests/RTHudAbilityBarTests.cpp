// La barra abilità dell'unità selezionata, e la riga di zona sopra di lei: cosa scrivono e di che colore.
//
// Perché esiste (#2184): le due righe vivevano dentro `ARTHUD::DrawHUD`, che il motore chiama ogni
// fotogramma e che **non ha copertura headless**. Sono decisioni testuali e cromatiche — un plurale, una
// precedenza fra due motivi, tre livelli di grigio, un avviso di fuoco amico — e nessuna di loro era
// verificabile se non guardando lo schermo.
//
// 🔴 **E il Canvas ricalcolava a mano una vista che l'altra via consuma già.** `bUsable` da
// `ARTUnit::CanUseAbility` e `CD` da `GetAbilityCooldown`, mentre `FRTAbilityCooldownView` porta
// `bUsableNow` e `TurnsRemaining` — e `WBP_RT_ActionSlot` li legge. Con `rt.HUD.CanvasPanels` a `1` le
// due vie rendono nello stesso fotogramma leggendo lo stesso dato da due sorgenti diverse.
//
// ⚠️ **`TestEqualSensitive` e non `TestEqual`**: su stringhe `TestEqual` passa da `FCString::Stricmp`
// (`AutomationTest.cpp:2163`) ed è case-insensitive. Qui si pinna testo, e `ALLEATO` → `Alleato` deve
// cadere.

#include "Misc/AutomationTest.h"
#include "UI/RTHUD.h"
#include "UI/RTHudViewModel.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * La riga di un'abilità: numero, nome, e il motivo per cui non la si può usare.
 *
 * 🔴 **La ricarica è l'UNICO motivo, e il test pinna anche il motivo che NON deve comparire.**
 *
 * Questo test verificava una **precedenza** fra due motivi — ricarica ed energia — perché un'abilità poteva
 * essere entrambe le cose. [D-324](../../../../docs/decisions/RT_PDR_00_Decision_Log.md) ha tolto `Energy`
 * dal gameplay: la precedenza non ha più due termini, e il test è stato riscritto invece di essere
 * cancellato. ⚠️ Il caso che conta è l'ultimo: una vista costruita a mano con `bUsableNow = false` fuori
 * ricarica — impossibile in produzione, dove `bUsableNow` coincide con `TurnsRemaining == 0` — **non deve
 * produrre nessun motivo**. È il posto dove si accorgerebbe qualcuno se il ramo «energia» tornasse.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudAbilityLineReasonTest,
	"RefactorTactics.HUD.AbilityLineShowsCooldownOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudAbilityLineReasonTest::RunTest(const FString&)
{
	FRTAbilityCooldownView Ability;
	Ability.AbilityIndex = 0;
	Ability.DisplayName = FText::FromString(TEXT("Scatto"));
	Ability.bUsableNow = true;

	// Il numero mostrato è 1-based: è la scorciatoia che il giocatore preme, non l'indice del kit.
	TestEqualSensitive(TEXT("pronta: numero e nome, nessun motivo"),
		ARTHUD::ComposeAbilityLine(Ability, /*bArmed=*/ false).Text,
		FString(TEXT("1. Scatto")));

	// Solo ricarica.
	Ability.TurnsRemaining = 2;
	Ability.bUsableNow = false;
	TestEqualSensitive(TEXT("in ricarica: i turni che mancano"),
		ARTHUD::ComposeAbilityLine(Ability, false).Text,
		FString(TEXT("1. Scatto  (ricarica 2)")));

	// La riga non nomina nessun altro motivo accanto alla ricarica.
	const FString InRicarica = ARTHUD::ComposeAbilityLine(Ability, false).Text;
	TestTrue(TEXT("in ricarica la riga dice i turni"), InRicarica.Contains(TEXT("(ricarica 2)")));
	TestFalse(TEXT("e non nomina l'energia"), InRicarica.Contains(TEXT("energia")));

	// 🔴 **Il caso che sostituisce la precedenza**: fuori ricarica ma dichiarata inutilizzabile.
	//
	// In produzione non accade — `bUsableNow` viene da `CanUseAbility`, che da `D-324` e' il solo cooldown,
	// quindi coincide con `TurnsRemaining == 0`. Ma la vista e' una struct, e un chiamante puo' comporla
	// cosi': la riga deve tacere invece di inventare un motivo che non esiste piu'.
	Ability.TurnsRemaining = 0;
	Ability.bUsableNow = false;
	TestEqualSensitive(TEXT("fuori ricarica e inutilizzabile: nessun motivo da scrivere"),
		ARTHUD::ComposeAbilityLine(Ability, false).Text,
		FString(TEXT("1. Scatto")));

	// Fuori ricarica e usabile: nessun motivo da scrivere.
	Ability.bUsableNow = true;
	TestEqualSensitive(TEXT("pronta di nuovo: nessun motivo"),
		ARTHUD::ComposeAbilityLine(Ability, false).Text,
		FString(TEXT("1. Scatto")));

	return true;
}

/**
 * L'abilità armata si distingue **due volte**: dal prefisso e dal colore.
 *
 * ⚠️ Due canali per la stessa informazione non è ridondanza sprecata: la barra ha tre livelli di grigio
 * vicini, e il solo colore non basta a dire quale delle tre è quella che sto per usare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudAbilityLineArmedTest,
	"RefactorTactics.HUD.AbilityLineMarksTheArmedOneTwice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudAbilityLineArmedTest::RunTest(const FString&)
{
	FRTAbilityCooldownView Ability;
	Ability.AbilityIndex = 2;
	Ability.DisplayName = FText::FromString(TEXT("Guardia"));
	Ability.bUsableNow = true;

	const FRTHudTextLine Armata = ARTHUD::ComposeAbilityLine(Ability, /*bArmed=*/ true);
	TestEqualSensitive(TEXT("armata: il prefisso davanti al numero"),
		Armata.Text, FString(TEXT("> 3. Guardia")));
	TestTrue(TEXT("armata: bianco pieno"), Armata.Color.Equals(FLinearColor::White));

	const FRTHudTextLine Pronta = ARTHUD::ComposeAbilityLine(Ability, /*bArmed=*/ false);
	TestEqualSensitive(TEXT("non armata: nessun prefisso"), Pronta.Text, FString(TEXT("3. Guardia")));
	TestTrue(TEXT("pronta: grigio chiaro"), Pronta.Color.Equals(FLinearColor(0.8f, 0.8f, 0.8f, 1.f)));

	// 🔴 **Armata resta bianca anche se non e' usabile.** «Cosa sto per fare» e «posso farlo» sono due
	// domande, e il prefisso risponde alla prima: un'ultimate armata e ancora in ricarica deve restare
	// riconoscibile come quella scelta. Senza questo caso, far vincere `bUsableNow` sul bianco passerebbe.
	//
	// ⚠️ **Il motivo si legge su un'abilita' IN RICARICA, e non e' un dettaglio di comodo.** Questo blocco
	// asseriva che una riga armata e *scarica* — `TurnsRemaining = 0`, `bUsableNow = false` — mostrasse
	// comunque `(energia)`. [D-324](../../../../docs/decisions/RT_PDR_00_Decision_Log.md) ha tolto `Energy`
	// dal gameplay: quella combinazione non e' piu' producibile dal ViewModel — `bUsableNow` viene da
	// `CanUseAbility`, che ora e' il solo cooldown — e la riga non ha piu' un secondo motivo da scrivere.
	// Il caso si sposta su un'abilita' in ricarica, dove il motivo esiste ancora.
	Ability.bUsableNow = false;
	Ability.TurnsRemaining = 2;
	const FRTHudTextLine ArmataScarica = ARTHUD::ComposeAbilityLine(Ability, /*bArmed=*/ true);
	TestTrue(TEXT("armata e in ricarica: resta bianca"), ArmataScarica.Color.Equals(FLinearColor::White));
	TestTrue(TEXT("armata e in ricarica: il motivo si vede comunque"),
		ArmataScarica.Text.Contains(TEXT("(ricarica 2)")));

	// Non armata e non usabile: il grigio più scuro, il terzo livello.
	const FRTHudTextLine Spenta = ARTHUD::ComposeAbilityLine(Ability, /*bArmed=*/ false);
	TestTrue(TEXT("inutilizzabile: grigio scuro"),
		Spenta.Color.Equals(FLinearColor(0.45f, 0.45f, 0.45f, 1.f)));
	TestFalse(TEXT("i tre livelli non collassano"),
		Spenta.Color.Equals(Pronta.Color) || Spenta.Color.Equals(FLinearColor::White));

	return true;
}

/**
 * La riga di zona sopra la barra: quante celle sto per colpire, e se fra loro c'è un alleato.
 *
 * 🔴 **L'avviso di fuoco amico è la decisione con la conseguenza peggiore se sbagliata**: chi la legge sta
 * per premere. Si distingue per colore — arancione contro rosso — e il codice è lo stesso del nome marcato
 * sopra la testa, così le due informazioni si riconoscono come la stessa cosa detta in due posti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudPreviewZoneTest,
	"RefactorTactics.HUD.PreviewZoneWarnsAboutAlliesInTheBlast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudPreviewZoneTest::RunTest(const FString&)
{
	// Nessuna zona puntata: la riga non esiste. Non «TIRO: 0 celle», che direbbe che sto mirando a niente
	// invece che non star mirando.
	TestEqualSensitive(TEXT("nessuna cella: nessuna riga"),
		ARTHUD::ComposePreviewZoneLine(/*NumHitCells=*/ 0, /*NumAllyHitCells=*/ 0).Text, FString());

	const FRTHudTextLine Pulita = ARTHUD::ComposePreviewZoneLine(3, 0);
	TestEqualSensitive(TEXT("zona senza alleati: solo l'ampiezza"),
		Pulita.Text, FString(TEXT("TIRO: 3 celle")));
	TestTrue(TEXT("zona pulita: rosso"), Pulita.Color.Equals(FLinearColor(1.f, 0.35f, 0.3f, 1.f)));

	const FRTHudTextLine Uno = ARTHUD::ComposePreviewZoneLine(3, 1);
	TestEqualSensitive(TEXT("un alleato: singolare"),
		Uno.Text, FString(TEXT("TIRO: 3 celle  -  1 ALLEATO NELLA ZONA")));
	TestTrue(TEXT("con un alleato: arancione"), Uno.Color.Equals(FLinearColor(1.f, 0.6f, 0.12f, 1.f)));

	// 🔴 Il plurale italiano dentro `DrawHUD`: `ALLEATO` diventa `ALLEATO/I` da due in su. Senza questo
	// caso, togliere il ramo del plurale lascerebbe la suite verde.
	TestEqualSensitive(TEXT("due alleati: plurale"),
		ARTHUD::ComposePreviewZoneLine(3, 2).Text,
		FString(TEXT("TIRO: 3 celle  -  2 ALLEATO/I NELLA ZONA")));

	// Il colore dipende dalla PRESENZA di alleati, non da quanti: uno solo basta a cambiarlo.
	TestTrue(TEXT("il colore cambia gia' col primo alleato"),
		!Uno.Color.Equals(Pulita.Color));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
