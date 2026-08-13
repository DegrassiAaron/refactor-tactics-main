// La sovrapposizione dell'unita' resta dentro il viewport.
//
// Nasce da #729, trovato eseguendo `PIE-NAME`: l'ancora del blocco (nome, barra HP, scudo, energia, status)
// e' un punto in WORLD space sopra l'attore, proiettato. Un offset fisso nel mondo produce uno spostamento
// **variabile** sullo schermo — tanto piu' grande quanto l'unita' e' vicina alla camera — e l'unico controllo
// era «dietro la camera». Un'unita' vicina perdeva l'intera sovrapposizione, in silenzio.
//
// ⚠️ Sparivano nome **e** barre insieme, perche' condividono la stessa Y: il difetto sembrava cosmetico e
// toglieva anche gli HP.
//
// Qui si verifica la parte pura. Che l'etichetta si VEDA resta `PIE-NAME`: `ARTHUD::DrawHUD` non e'
// esercitato da nessun test automatico, e questo file non colma quella lacuna — la circoscrive.

#include "Misc/AutomationTest.h"

#include "UI/RTHUD.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudOverlayClampTest,
	"RefactorTactics.UI.OverlayStaysInsideViewport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudOverlayClampTest::RunTest(const FString&)
{
	const FVector2D Viewport(1600.f, 900.f);
	const float Half = 40.f;    // meta' larghezza del blocco
	const float Above = 36.f;   // riga del nome sopra l'ancora
	const float Below = 12.f;   // fondo della barra energia sotto l'ancora
	const float Margin = 4.f;

	// 1. Un'ancora gia' dentro non si tocca: il vincolo non deve spostare cio' che era a posto.
	const FVector2D Dentro(800.f, 450.f);
	TestEqual(TEXT("un'ancora interna resta dov'e'"),
		ARTHUD::ClampOverlayAnchor(Dentro, Half, Above, Below, Viewport, Margin), Dentro);

	// 2. Il caso di #729: unita' vicina alla camera, ancora proiettata SOPRA il bordo superiore.
	//    Prima della correzione il blocco veniva disegnato fuori e spariva.
	const FVector2D Sopra = ARTHUD::ClampOverlayAnchor(
		FVector2D(800.f, -250.f), Half, Above, Below, Viewport, Margin);
	TestEqual(TEXT("un'ancora sopra il bordo scende al margine"), static_cast<float>(Sopra.Y), Margin + Above);
	TestTrue(TEXT("la riga del nome e' dentro il viewport"), Sopra.Y - Above >= Margin - KINDA_SMALL_NUMBER);
	TestEqual(TEXT("la X non cambia se era gia' buona"), static_cast<float>(Sopra.X), 800.f);

	// 3. Gli altri tre bordi, perche' un vincolo scritto su un lato solo e' un vincolo che si dimentica.
	const FVector2D Sotto = ARTHUD::ClampOverlayAnchor(
		FVector2D(800.f, 5000.f), Half, Above, Below, Viewport, Margin);
	TestEqual(TEXT("sotto: il fondo del blocco resta dentro"),
		static_cast<float>(Sotto.Y), static_cast<float>(Viewport.Y) - Margin - Below);

	const FVector2D Sinistra = ARTHUD::ClampOverlayAnchor(
		FVector2D(-900.f, 450.f), Half, Above, Below, Viewport, Margin);
	TestEqual(TEXT("sinistra: il bordo del blocco resta dentro"), static_cast<float>(Sinistra.X), Margin + Half);

	const FVector2D Destra = ARTHUD::ClampOverlayAnchor(
		FVector2D(9000.f, 450.f), Half, Above, Below, Viewport, Margin);
	TestEqual(TEXT("destra: il bordo del blocco resta dentro"),
		static_cast<float>(Destra.X), static_cast<float>(Viewport.X) - Margin - Half);

	// 4. Il vincolo insoddisfacibile: blocco piu' grande del viewport. `Min` supera `Max`, e un `Clamp`
	//    ingenuo restituirebbe il bordo deciso dall'ORDINE degli argomenti invece che dalla geometria.
	//    La scelta dichiarata e' il bordo superiore/sinistro: il nome sta in alto ed e' cio' che identifica
	//    l'unita', quindi se qualcosa deve uscire non e' quello.
	const FVector2D Minuscolo(50.f, 50.f);
	const FVector2D Stretto = ARTHUD::ClampOverlayAnchor(
		FVector2D(25.f, 25.f), Half, Above, Below, Minuscolo, Margin);
	TestEqual(TEXT("viewport piu' piccolo del blocco: si preferisce il bordo sinistro"), static_cast<float>(Stretto.X), Margin + Half);
	TestEqual(TEXT("viewport piu' piccolo del blocco: si preferisce il bordo superiore"), static_cast<float>(Stretto.Y), Margin + Above);

	// 5. Margine zero: il blocco tocca il bordo esatto, senza uscirne. Serve perche' il margine e' un
	//    parametro e qualcuno lo mettera' a zero.
	const FVector2D SenzaMargine = ARTHUD::ClampOverlayAnchor(
		FVector2D(-10.f, -10.f), Half, Above, Below, Viewport, 0.f);
	TestEqual(TEXT("margine zero: X al bordo"), static_cast<float>(SenzaMargine.X), Half);
	TestEqual(TEXT("margine zero: Y al bordo"), static_cast<float>(SenzaMargine.Y), Above);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
