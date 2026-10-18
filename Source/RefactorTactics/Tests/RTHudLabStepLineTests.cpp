// Dove cade la seconda riga del banco Ability Lab («BANCO i/n · ...») — #3795.
//
// Il difetto: la riga stava sotto la banda «questa non e' una partita», in alto al centro, e le etichette delle unita'
// (widget sopra il canvas, a ridosso del centro dell'inquadratura del banco) la coprivano: nel PIE del 2026-10-15 se ne
// leggevano solo dei pezzi. La regola da pinnare non e' un numero ma una proprieta': la riga NON sta nella fascia alta
// dello schermo, resta dentro il canvas e STA SOPRA IL DOCK, a ogni risoluzione. Il dock e' una fascia in unita' Slate
// (`RTHudLayout::Bands().Bottom`) moltiplicata per la scala dell'interfaccia: un inset in pixel fissi lo invadeva a 1440p
// e a 4K (review di #3822), ed e' il caso che il test sotto chiude.

#include "Misc/AutomationTest.h"
#include "UI/RTHUD.h"
#include "UI/RTHudLayout.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudLabStepLinePlacementTest,
	"RefactorTactics.HUD.LabStepLineSitsBelowTheUnitLabelsBand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTHudLabStepLinePlacementTest::RunTest(const FString&)
{
	const FVector2f Testo(520.f, 22.f);
	// Il rettangolo scuro dietro il testo sporge di 4 pixel in basso (`DrawRect(Posa.Y - 4, ... PH + 8)`).
	constexpr float PadBasso = 4.f;

	for (const FVector2f& Canvas : {FVector2f(1914.f, 1048.f), FVector2f(1280.f, 720.f), FVector2f(2560.f, 1440.f),
			 FVector2f(3840.f, 2160.f)})
	{
		const FVector2f Posa = ARTHUD::ComposeLabStepLinePlacement(Canvas, Testo);
		const FString Dove = FString::Printf(TEXT("canvas %.0fx%.0f"), Canvas.X, Canvas.Y);

		// Non nella fascia alta: dove stavano banda e riga, e dove arrivano le etichette delle unita' del banco.
		TestTrue(*(Dove + TEXT(": la riga sta nella meta' bassa dello schermo")), Posa.Y > Canvas.Y * 0.5f);
		// Dentro il canvas, e centrata.
		TestTrue(*(Dove + TEXT(": la riga finisce dentro il canvas")), Posa.Y + Testo.Y <= Canvas.Y);
		TestTrue(*(Dove + TEXT(": e il testo e' centrato")), FMath::Abs((Posa.X + Testo.X * 0.5f) - Canvas.X * 0.5f) < 0.01f);
		TestTrue(*(Dove + TEXT(": e non esce a sinistra")), Posa.X >= 0.f);

		// SOPRA IL DOCK: il fondo del rettangolo scuro sta sopra il bordo alto della fascia bassa. La fascia e' quella
		// che usa la scena, nella scala che le da' lo stesso viewport.
		const float Scala = RTHudLayout::UiScaleFor(FIntPoint(FMath::RoundToInt(Canvas.X), FMath::RoundToInt(Canvas.Y)));
		const float DockTop = Canvas.Y - RTHudLayout::Bands().Bottom * Scala;
		TestTrue(*(Dove + TEXT(": la riga sta sopra il dock")), Posa.Y + Testo.Y + PadBasso <= DockTop);
	}

	// Anti-vacuita', sul codice che lo regge: la posa di un inset in pixel fissi (200, il primo tentativo di #3795)
	// entra nel dock a 2560x1440. Se questa non entrasse, l'asserzione «sopra il dock» non distinguerebbe niente.
	{
		const FVector2f Canvas(2560.f, 1440.f);
		const float Scala = RTHudLayout::UiScaleFor(FIntPoint(2560, 1440));
		const float DockTop = Canvas.Y - RTHudLayout::Bands().Bottom * Scala;
		const float FondoConInsetFisso = Canvas.Y - 200.f; // fondo del testo con 200 px dal bordo basso
		TestTrue(TEXT("un inset di 200 px fissi a 1440p entra nel dock: l'asserzione sopra lo vede"),
			FondoConInsetFisso + PadBasso > DockTop);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
