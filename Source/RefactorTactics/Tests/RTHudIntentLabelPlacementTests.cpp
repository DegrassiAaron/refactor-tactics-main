// Dove finisce l'etichetta d'intento: il centraggio, il bordo, e la coppia di metriche che deve coincidere.
//
// Perché esiste (#2184): la posa dell'etichetta era scritta dentro `ARTHUD::DrawHUD` come quattro
// letterali — `36.f` due volte, `0.85f` due volte — in coppie che **dovevano coincidere per costruzione**
// e che nulla legava. Non erano protette da un condizionale, quindi la verifica di chiusura, che
// guardava i rami, non le aveva catalogate: questi non sono rami.
//
// 🔑 **La coppia `36.f` è la banda riservata E lo scarto con cui si risale.**
// `ClampOverlayAnchor` riceve `AboveAnchor` per tenere dentro al viewport ciò che sta SOPRA l'ancora;
// poi il testo viene scritto a `Anchor.Y - 36.f`. Se i due numeri divergessero, l'etichetta uscirebbe
// dal bordo esattamente di quanto differiscono — cioè tornerebbe il difetto di #729, che
// `ClampOverlayAnchor` è stato scritto per chiudere. Un nome solo toglie il modo in cui la divergenza
// succede davvero — per distrazione — ma non la vieta: la mutazione 1 qui sotto riscrive un letterale
// dove c'è il nome, compila, e deve cadere.
//
// ⛔ **E la prova che i due coincidono NON è un test sul valore**: è il caso al bordo superiore. Lì
// l'ancora viene spinta a `Margin + Above` e il testo risale di `Above`, quindi il risultato è `Margin`
// — **qualunque sia** `Above`. Un test che si aspetta `Margin` cade sotto qualunque divergenza e sotto
// nessun cambio concorde: è l'invariante, non il numero.
//
// ⚠️ **Il numero, separatamente, si pinna lo stesso.** Lo Scope della fetta vieta di cambiare i pixel,
// e `36.f` non aveva alcuna copertura: il caso centrale asserisce il valore esatto perché una modifica
// a quella costante sia una modifica che si vede, non una che passa.
//
// ── TABELLA DELLE ATTESE DI MUTAZIONE ────────────────────────────────────────────────────────────────
// Scritta prima di lanciare ed eseguibile: ogni riga nomina un test che esiste e una mutazione che
// compila, a partire dal codice spedito. ⚠️ **Gli esiti sono rimisurati sull'insieme di test FINALE** —
// vedi la nota in fondo alla tabella, che è il difetto di metodo più utile di questa fetta.
//
//   # | mutazione                                                     | rosso atteso              | esito
//  ---|----------------------------------------------------------------|---------------------------|-------
//   1 | `Anchor.Y - IntentLabelAbove` → `Anchor.Y - 40.f` (DIVERGENZA)  | …AtTheTopEdge…            | 3 rossi
//   2 | `IntentLabelAbove = 36.f` → `= 37.f` (cambio CONCORDE)          | …AtTheTopEdge… VERDE      | 2 rossi
//   3 | `FMath::Max(BarWidth, LabelWidth)` → `LabelWidth`               | …ReservesTheWiderBlock…   | 1, esatto
//   4 | `Anchor.X - LabelWidth * 0.5f` → `Anchor.X`                     | …CentresOnTheLabel…       | 2 rossi
//   5 | `/*BelowAnchor=*/ 0.f` → `36.f`                                 | …ReservesNothingBelow… SOLO | 1, esatto
//
// 🔑 **La 1 e la 2 sono la coppia che dà senso al file, e l'attesa della 2 va letta su UN test, non sul
// totale.** La 1 rompe la coincidenza fra banda e scarto; la 2 cambia il valore lasciandole concordi.
// Ciò che le distingue è `…AtTheTopEdge…`, dove l'ancora viene spinta a `Margin + Above` e il testo
// risale di `Above`: resta `Margin` **qualunque sia** `Above`.
//
//     mutazione 1 (divergenza)   →  …AtTheTopEdge…  ROSSO   (0 invece di 4)
//     mutazione 2 (concorde)     →  …AtTheTopEdge…  VERDE
//
// ⛔ È l'unica riga di questa tabella che prova qualcosa sull'**invariante** invece che su un numero. Gli
// altri test cadono sotto entrambe, ed è corretto che sia così: pinnano i pixel, che lo Scope della fetta
// vieta di spostare.
//
// 🔴 **La 5 c'è perché la prima stesura della tabella NON la conteneva, e il buco era reale.** I quattro
// letterali del reperto erano `36.f` e `0.85f`; il **quinto** — `BelowAnchor = 0.f` — è dello stesso tipo
// e non lo avevo catalogato. Con i primi tre test, mutarlo in `36.f` lasciava **tre verdi**: le loro teste
// stanno a `Y ∈ {400, 10}` su un viewport alto 800, e il limite inferiore non entra in gioco finché
// `BelowAnchor` non supera 396 — a 396 esatti `Clamp(400, 40, 400)` rende ancora 400, quindi la soglia è
// «strettamente maggiore». Un letterale che nessuna asserzione raggiunge è un letterale che può cambiare
// in silenzio: esattamente il difetto che questa fetta esiste per chiudere.
//
// ⌫ **E gli esiti delle righe 1-4 erano SBAGLIATI, per una ragione che vale più della correzione.**
// Erano stati misurati davvero — non inventati — ma su un insieme di test a cui il quarto è stato
// aggiunto DOPO. La 1 diceva «2 rossi» e ne fa 3; la 2 diceva «1, esatto» e ne fa 2, perché
// `…ReservesNothingBelow…` dipende da `IntentLabelAbove` in entrambe le sue asserzioni.
//
// 🔑 Una tabella di mutazioni è una misura sull'insieme dei test, non su una mutazione: **cambia da sola
// quando cambia l'insieme**, come un totale in prosa. Si rimisura per ultima, dopo l'ultimo test, o non
// vale. Trovato dalla revisione della PR #3349, non da chi ha scritto la tabella.
//
// ⛔ **`IntentLabelScale` NON compare nella tabella, e va detto invece di lasciarlo dedurre.** Le sue due
// occorrenze sono `GetTextSize` e `DrawText`, entrambe membri di `AHUD` che vogliono un font e un canvas:
// non esiste, in questo file, una domanda headless che le interroghi. Quella coppia è protetta dal
// **nome**, non da un test — ed è esattamente l'argomento della fetta: un nome solo toglie il modo in cui
// la divergenza succede, mentre un test la cercherebbe soltanto. Scriverne una riga di tabella non
// eseguibile sarebbe la promessa vuota della fetta 5.

#include "Misc/AutomationTest.h"
#include "UI/RTHUD.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Il margine che `ComposeIntentLabelPlacement` passa al vincolo. Sta qui come **attesa**, non come
	// sorgente: se cambia nel codice di produzione questi test devono cadere, non seguirlo.
	//
	// ⚠️ `double`, non `float`: le componenti di `FVector2D` sono a doppia precisione in UE5, e un'attesa
	// in `float` rende ambiguo l'overload di `TestEqual` invece di confrontare.
	constexpr double MargineAtteso = 4.0;

	// ⚠️ **Il nome e' lungo apposta.** Un `Viewport` in un namespace anonimo a livello di file diventa,
	// sotto unity build, una globale che nasconde le variabili locali omonime degli ALTRI file del
	// blocco — e `RTHudOverlayClampTests.cpp` ne ha una. Il C4459 e' un errore, qui: la prima build
	// incrementale non lo vede, la prima che ricompone il blocco si'.
	const FVector2D ViewportDiProva(1000.f, 800.f);
}

/**
 * Lontano da ogni bordo: il testo è centrato sulla propria larghezza e risale di `IntentLabelAbove`.
 *
 * ⚠️ I due numeri sono asseriti **esatti**, non come `>=`: è il pinning dei pixel che lo Scope della
 * fetta 9 richiede — il refactor non doveva spostare niente di ciò che si vede.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudIntentLabelCentreTest,
	"RefactorTactics.HUD.IntentLabelCentresOnTheLabelAndRisesAboveTheHead",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudIntentLabelCentreTest::RunTest(const FString&)
{
	// Larghezza del testo **maggiore** di quella della barra: così il caso centrale non dipende da quale
	// delle due vince, e resta il solo caso al bordo a discriminarlo.
	const FVector2D Dove = ARTHUD::ComposeIntentLabelPlacement(
		FVector2D(500.f, 400.f), /*LabelWidth=*/ 80.f, /*BarWidth=*/ 60.f, ViewportDiProva);

	TestEqual(TEXT("centrato sulla meta' della larghezza del testo"), Dove.X, 500.0 - 40.0);
	TestEqual(TEXT("⛔ e risalito di 36 pixel esatti: lo Scope vieta di spostare i pixel"),
		Dove.Y, 400.0 - 36.0);

	return true;
}

/**
 * 🔑 **Al bordo superiore l'etichetta resta dentro, e il risultato NON dipende dal valore della banda.**
 *
 * L'ancora viene spinta a `Margin + Above`, il testo risale di `Above`: resta `Margin`. È l'unica forma
 * in cui la coincidenza delle due metriche è osservabile da fuori — e cade se divergono di un pixel.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudIntentLabelTopEdgeTest,
	"RefactorTactics.HUD.IntentLabelStaysOnScreenAtTheTopEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudIntentLabelTopEdgeTest::RunTest(const FString&)
{
	// Testa quasi sul bordo: senza vincolo il testo finirebbe a `10 - 36 = -26`, cioe' fuori schermo.
	// È il difetto osservato in PIE e diagnosticato in #729.
	const FVector2D Dove = ARTHUD::ComposeIntentLabelPlacement(
		FVector2D(500.f, 10.f), /*LabelWidth=*/ 80.f, /*BarWidth=*/ 60.f, ViewportDiProva);

	TestEqual(TEXT("🔑 il testo nasce esattamente al margine, qualunque sia la banda riservata"),
		Dove.Y, MargineAtteso);
	TestTrue(TEXT("⛔ e quindi NON esce dal viewport"), Dove.Y >= 0.f);

	return true;
}

/**
 * Il vincolo laterale riserva la larghezza del blocco **più largo**, il centraggio quella del testo.
 *
 * ⚠️ Sono due mezze larghezze diverse nella stessa funzione, ed è il punto del test: con una barra molto
 * più larga del testo, un'implementazione che usasse `LabelWidth` per entrambe spingerebbe l'ancora di
 * molto meno, e la barra uscirebbe dal bordo mentre l'etichetta resta dentro.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudIntentLabelWiderBlockTest,
	"RefactorTactics.HUD.IntentLabelReservesTheWiderBlockAtTheSideEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudIntentLabelWiderBlockTest::RunTest(const FString&)
{
	constexpr double LabelWidth = 10.0;
	constexpr double BarWidth = 200.0;

	// Testa incollata al bordo sinistro: l'ancora viene spinta a `Margin + BarWidth/2`.
	const FVector2D Dove = ARTHUD::ComposeIntentLabelPlacement(
		FVector2D(5.f, 400.f), static_cast<float>(LabelWidth), static_cast<float>(BarWidth), ViewportDiProva);

	// ⛔ L'asserzione che distingue le due mezze larghezze: con `LabelWidth` in entrambe uscirebbe
	// `MargineAtteso` esatto, che e' il valore qui sotto **meno 95**.
	TestEqual(TEXT("⛔ e' la BARRA a decidere quanto l'ancora si scosta dal bordo"),
		Dove.X, MargineAtteso + BarWidth * 0.5 - LabelWidth * 0.5);

	return true;
}

/**
 * 🔴 **Sotto l'ancora l'etichetta non riserva NIENTE, e si vede solo al bordo inferiore.**
 *
 * `ComposeIntentLabelPlacement` passa `BelowAnchor = 0.f` perché sotto l'etichetta non disegna nulla —
 * a differenza della sovrapposizione dell'unità (#729), dove la barra HP pende sotto l'ancora e quello
 * spazio va riservato. Riservarlo anche qui non farebbe uscire niente dallo schermo: farebbe **staccare
 * l'etichetta dall'unità prima del necessario**, ed è un difetto opposto a quello di #729 — l'etichetta
 * smette di stare sopra la testa che identifica.
 *
 * ⚠️ **È un letterale che i primi tre test non raggiungevano.** Le loro teste stanno lontane dal bordo
 * inferiore, e `BelowAnchor` non cambia niente finché non supera 396 su un viewport alto 800. Qui le due
 * asserzioni lo interrogano dai due lati del limite: sopra, dove l'etichetta deve **seguire** l'unità;
 * sotto, dove deve fermarsi **al margine** e non prima.
 *
 * ⛔ **Le due asserzioni NON hanno la stessa forza, e va saputo.** `Fermata` sta sempre nel ramo vincolato
 * (`Dove.Y = Viewport.Y - Margin - BelowAnchor - Above`), quindi **qualunque** `BelowAnchor` diverso da
 * zero la fa cadere — è lei a chiudere il buco. `Segue` sta nel ramo libero e ha una zona morta fino a
 * `BelowAnchor ≈ 6`, perché `790` esce dall'intervallo solo quando il limite scende sotto di lui.
 *
 * 🔑 Resta perché copre una classe di difetto che `Fermata` non vede: un'implementazione che sottraesse
 * `BelowAnchor` **anche dove non c'è vincolo**. Senza quel caso sarebbe ridondante, e andrebbe tolta
 * invece che tenuta per simmetria.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudIntentLabelBottomEdgeTest,
	"RefactorTactics.HUD.IntentLabelReservesNothingBelowAtTheBottomEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudIntentLabelBottomEdgeTest::RunTest(const FString&)
{
	// ⛔ Testa in basso ma ANCORA dentro il limite: l'ancora non viene toccata, e l'etichetta segue.
	// Se qualcosa fosse riservato sotto, il limite salirebbe e questa verrebbe tirata su.
	const FVector2D Segue = ARTHUD::ComposeIntentLabelPlacement(
		FVector2D(500.f, 790.f), /*LabelWidth=*/ 80.f, /*BarWidth=*/ 60.f, ViewportDiProva);

	TestEqual(TEXT("⛔ l'etichetta SEGUE l'unita' fin quasi al bordo, non si stacca prima"),
		Segue.Y, 790.0 - 36.0);

	// E oltre il limite: l'ancora si ferma a `Viewport.Y - Margin`, perche' sotto non c'e' niente da
	// tenere dentro. Il testo nasce di `IntentLabelAbove` piu' in alto.
	const FVector2D Fermata = ARTHUD::ComposeIntentLabelPlacement(
		FVector2D(500.f, 900.f), /*LabelWidth=*/ 80.f, /*BarWidth=*/ 60.f, ViewportDiProva);

	TestEqual(TEXT("🔑 e si ferma AL margine inferiore, non prima"),
		Fermata.Y, 800.0 - MargineAtteso - 36.0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
