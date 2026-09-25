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
// compila, a partire dal codice spedito.
//
//   # | mutazione                                                     | rosso atteso              | esito
//  ---|----------------------------------------------------------------|---------------------------|-------
//   1 | `Anchor.Y - IntentLabelAbove` → `Anchor.Y - 40.f` (DIVERGENZA)  | …AtTheTopEdge…            | 2 rossi
//   2 | `IntentLabelAbove = 36.f` → `= 37.f` (cambio CONCORDE)          | …CentresOnTheLabel… solo  | 1, esatto
//   3 | `FMath::Max(BarWidth, LabelWidth)` → `LabelWidth`               | …ReservesTheWiderBlock…   | 1, esatto
//   4 | `Anchor.X - LabelWidth * 0.5f` → `Anchor.X`                     | …CentresOnTheLabel…       | 2 rossi
//
// 🔑 **La 1 e la 2 sono la coppia che dà senso al file, e l'esito è quello previsto.** La 1 rompe la
// coincidenza e uccide il caso al bordo — dove il valore non conta — oltre al caso centrale. La 2 cambia
// il valore restando concorde e uccide **soltanto** il centro: il bordo resta **verde**, il che dimostra
// che quel test misura l'invariante e non il numero. Se la 2 avesse fatto cadere anche il bordo, quel
// test non sarebbe servito a niente.
//
// ⛔ **`IntentLabelScale` NON compare nella tabella, e va detto invece di lasciarlo dedurre.** Le sue due
// occorrenze sono `GetTextSize` e `DrawText`, entrambe membri di `AHUD` che vogliono un font e un canvas:
// non esiste, in questo file, una domanda headless che le interroghi. Quella coppia è protetta dal
// **nome**, non da un test — ed è esattamente l'argomento della fetta: un nome solo rende la divergenza
// impossibile, mentre un test la cercherebbe soltanto. Scriverne una riga di tabella non eseguibile
// sarebbe la promessa vuota della fetta 5.

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

#endif // WITH_DEV_AUTOMATION_TESTS
