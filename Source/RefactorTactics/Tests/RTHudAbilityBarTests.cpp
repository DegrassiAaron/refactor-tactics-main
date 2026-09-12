// La barra abilità dell'unità selezionata, e la riga di zona sopra di lei: cosa scrivono e di che colore.
//
// Perché esiste (#2184): le due righe vivevano dentro `ARTHUD::DrawHUD`, che il motore chiama ogni
// fotogramma e che **non ha copertura headless**. Sono decisioni testuali e cromatiche — un plurale, una
// precedenza fra due motivi, tre livelli di grigio, un avviso di fuoco amico — e nessuna di loro era
// verificabile se non guardando lo schermo.
//
// 🔴 **Il Canvas ricalcolava a mano una vista che l'altra via consuma già.** `bUsable` da
// `ARTUnit::CanUseAbility` e `CD` da `GetAbilityCooldown`, mentre `FRTAbilityCooldownView` porta
// `bUsableNow` e `TurnsRemaining` — e `WBP_RT_ActionSlot` li legge: con entrambi i layer accesi, due vie
// rendevano nello stesso fotogramma lo stesso dato letto da due sorgenti diverse.
//
// ⚠️ **Quel Canvas non esiste più** (#1936): i pannelli screen-space e la CVar `rt.HUD.CanvasPanels` sono
// usciti da `ARTHUD`. Resta il §4.2 world-space, e restano queste funzioni pure — è da qui che lo Screen
// HUD in UMG (#613) prende testo e colore. Finché #613 non le consuma, **questi test sono il loro unico
// chiamante**: non è codice morto, è codice in attesa del suo consumatore, e i test dicono cosa promette.
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
	// 🔑 **Il tasto si DICHIARA, e prima si deduceva da `AbilityIndex`** (`#2987`). Comporlo qui è ciò che
	// rende questo test cieco al difetto che `ComposeAbilityLine` aveva: finché la riga calcolava
	// `Index + 1`, una vista con `AbilityIndex = 0` produceva `1.` **qualunque** tabella di binding
	// esistesse. Ora la riga porta ciò che la vista dichiara, e chi la dichiara è `BuildAbilityCooldowns`
	// leggendo `AbilityHotkeys()`.
	Ability.HotkeyLabel = FText::FromString(TEXT("1"));

	// Il tasto viene dalla vista, non dall'indice: è la scorciatoia che il giocatore preme.
	TestEqualSensitive(TEXT("pronta: numero e nome, nessun motivo"),
		ARTHUD::ComposeAbilityLine(Ability, /*bArmed=*/ false).Text,
		FString(TEXT("1. Scatto")));

	// Solo ricarica.
	Ability.TurnsRemaining = 2;
	Ability.bUsableNow = false;
	TestEqualSensitive(TEXT("in ricarica: i turni che mancano"),
		ARTHUD::ComposeAbilityLine(Ability, false).Text,
		FString(TEXT("1. Scatto  (ricarica 2)")));

	// ⛔ Qui stavano due assertion su `Contains("(ricarica 2)")` e `!Contains("energia")` sullo STESSO stato
	// gia' pinnato dall'uguaglianza esatta qui sopra: strettamente piu' deboli, quindi nessuna mutazione
	// poteva farle cadere lasciando verde la riga precedente. Erano il residuo del caso «entrambi i motivi»,
	// che `D-324` ha reso impossibile.

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
	Ability.HotkeyLabel = FText::FromString(TEXT("3")); // dichiarato, non dedotto da `AbilityIndex` (#2987)

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

/**
 * 🔴 **LA RIGA MOSTRA IL TASTO DELLA VISTA ANCHE QUANDO DIVERGE DALL'INDICE** (`#2987`).
 *
 * 🔑 **Esiste perche' senza di lui la correzione non sarebbe misurata da niente, e questo va detto per
 * intero.** Ogni altro test di `ComposeAbilityLine` costruisce viste in cui `HotkeyLabel` vale
 * `AbilityIndex + 1` — `0`→`"1"`, `2`→`"3"`, `3`→`"4"` — perche' quelle sono le posizioni normali del kit.
 * Su quelle viste l'aritmetica di prima e la lettura di adesso danno **la stessa stringa**: rimettere
 * `Index + 1` le lascerebbe tutte verdi. Il verde sarebbe vacuo esattamente come quello che `#2987`
 * denuncia in `ActionSlotLineCarriesKeyArmedAndReason`.
 *
 * ∴ i due casi qui sotto sono gli **unici** in cui le due formule divergono, e sono anche i due che il
 * difetto sbagliava in produzione:
 *
 *  - **A** — la decima posizione. `AbilityHotkeys()` chiude con `EKeys::Zero`, quindi l'indice `9` porta il
 *    tasto `0` e l'aritmetica direbbe `10`;
 *  - **B** — una posizione oltre la fila dei numeri, che `GenericHotkeys()` dichiara possibile (*«un eroe
 *    con sei azioni porta il kit a undici voci contro i dieci tasti numerici»*). Li' non c'e' un tasto, e
 *    l'aritmetica ne annuncerebbe uno.
 *
 * ⚠️ **Le etichette sono scritte a mano qui, e non lette da `AbilityHotkeys()`, di proposito.** Questo test
 * misura `ComposeAbilityLine`, che riceve una vista gia' composta: chi legge la tabella e' il ViewModel, e
 * l'oracolo di **quella** meta' e' `HudViewModel.ShortcutComesFromTheBindingTableNotTheIndex`. Interrogare
 * la tabella anche qui confonderebbe le due responsabilita' in un test solo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudAbilityLineUsesTheDeclaredKeyTest,
	"RefactorTactics.HUD.AbilityLineUsesTheDeclaredKeyNotTheIndex",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudAbilityLineUsesTheDeclaredKeyTest::RunTest(const FString&)
{
	// --- A. il tasto e' `0` dove l'aritmetica direbbe `10` --------------------------------------------
	FRTAbilityCooldownView Decima;
	Decima.AbilityIndex = 9;
	Decima.HotkeyLabel = FText::FromString(TEXT("0"));
	Decima.DisplayName = FText::FromString(TEXT("Attesa"));
	Decima.bUsableNow = true;

	TestEqualSensitive(TEXT("A: la decima posizione porta il tasto `0`, non un `10` calcolato"),
		ARTHUD::ComposeAbilityLine(Decima, /*bArmed=*/ false).Text,
		FString(TEXT("0. Attesa")));

	// --- B. senza tasto, nessun numero ----------------------------------------------------------------
	// ⛔ L'aritmetica risponde anche qui — direbbe `11.` — ed e' la risposta che non esiste: nessun tasto
	// arma quella posizione, e annunciarne uno la darebbe per premibile.
	FRTAbilityCooldownView Irraggiungibile;
	Irraggiungibile.AbilityIndex = 10;
	Irraggiungibile.HotkeyLabel = FText::GetEmpty();
	Irraggiungibile.DisplayName = FText::FromString(TEXT("Interagisci"));
	Irraggiungibile.bUsableNow = true;

	const FString Riga = ARTHUD::ComposeAbilityLine(Irraggiungibile, /*bArmed=*/ false).Text;
	TestEqualSensitive(TEXT("B: senza tasto la riga porta il solo nome"),
		Riga, FString(TEXT("Interagisci")));
	TestFalse(TEXT("B: e non annuncia un tasto che non esiste"), Riga.Contains(TEXT("11")));

	// --- C. il resto della riga non cambia ------------------------------------------------------------
	// Senza questo, togliere prefisso E motivo insieme passerebbe i due controlli qui sopra.
	Irraggiungibile.TurnsRemaining = 2;
	Irraggiungibile.bUsableNow = false;
	TestEqualSensitive(TEXT("C: senza tasto restano prefisso di selezione e motivo"),
		ARTHUD::ComposeAbilityLine(Irraggiungibile, /*bArmed=*/ true).Text,
		FString(TEXT("> Interagisci  (ricarica 2)")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
