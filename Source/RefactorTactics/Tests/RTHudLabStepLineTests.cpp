// Dove cade la seconda riga del banco Ability Lab («BANCO i/n · ...») — #3795.
//
// Il difetto: la riga stava sotto la banda «questa non e' una partita», in alto al centro, e le etichette delle unita'
// (widget sopra il canvas, a ridosso del centro dell'inquadratura del banco) la coprivano: nel PIE del 2026-10-15 se ne
// leggevano solo dei pezzi. La regola da pinnare non e' un numero ma una proprieta': la riga NON sta nella fascia alta
// dello schermo, e resta dentro il canvas.

#include "Misc/AutomationTest.h"
#include "UI/RTHUD.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudLabStepLinePlacementTest,
	"RefactorTactics.HUD.LabStepLineSitsBelowTheUnitLabelsBand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTHudLabStepLinePlacementTest::RunTest(const FString&)
{
	const FVector2f Testo(520.f, 22.f);
	for (const FVector2f& Canvas : {FVector2f(1914.f, 1048.f), FVector2f(1280.f, 720.f), FVector2f(2560.f, 1440.f)})
	{
		const FVector2f Posa = ARTHUD::ComposeLabStepLinePlacement(Canvas, Testo);
		const FString Dove = FString::Printf(TEXT("canvas %.0fx%.0f"), Canvas.X, Canvas.Y);

		// Non nella fascia alta: dove stavano banda e riga, e dove arrivano le etichette delle unita' del banco.
		TestTrue(*(Dove + TEXT(": la riga sta nella meta' bassa dello schermo")), Posa.Y > Canvas.Y * 0.5f);
		// Dentro il canvas, e centrata.
		TestTrue(*(Dove + TEXT(": la riga finisce dentro il canvas")), Posa.Y + Testo.Y <= Canvas.Y);
		TestTrue(*(Dove + TEXT(": e il testo e' centrato")), FMath::Abs((Posa.X + Testo.X * 0.5f) - Canvas.X * 0.5f) < 0.01f);
		TestTrue(*(Dove + TEXT(": e non esce a sinistra")), Posa.X >= 0.f);
	}

	// Anti-vacuita': la posa di prima era sotto la banda (44 + altezza + 14 px). Quella non passa la prima asserzione.
	TestFalse(TEXT("la vecchia posa (sotto la banda) non soddisfa la regola"), (44.f + Testo.Y + 14.f) > 1048.f * 0.5f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
