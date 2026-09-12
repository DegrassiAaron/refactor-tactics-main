// Le otto zone dello Screen HUD, provate come CLASSE e non come albero.
//
// 🔑 **La divisione con `RTMatchWidgetAssetTests.cpp` e' deliberata**: questi test girano senza caricare
// nessun asset e sono verdi appena il Task 1 compila. I due gate sull'albero vivono nell'altro file e
// restano rossi finche' il `.uasset` non e' rimontato — tenerli insieme significherebbe non poter chiudere
// verde nessun task prima dell'authoring.

#include "Misc/AutomationTest.h"
#include "UI/RTHudZoneWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudZoneColorsAreAllDistinctTest,
	"RefactorTactics.ScreenHud.HudZoneColorsAreAllDistinct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTHudZoneColorsAreAllDistinctTest::RunTest(const FString&)
{
	// 🔑 Due zone dello stesso colore renderebbero il blockout inutile proprio dove serve: a dire QUALE
	// riquadro si sta guardando.
	const int32 Quante = static_cast<int32>(ERTHudZone::Count);

	for (int32 A = 0; A < Quante; ++A)
	{
		const ERTHudZone ZonaA = static_cast<ERTHudZone>(A);
		const FLinearColor ColoreA = URTHudZoneWidget::BlockoutColor(ZonaA);

		AddInfo(FString::Printf(TEXT("  %-14s -> R%.2f G%.2f B%.2f"),
			*URTHudZoneWidget::ZoneName(ZonaA), ColoreA.R, ColoreA.G, ColoreA.B));

		for (int32 B = A + 1; B < Quante; ++B)
		{
			const ERTHudZone ZonaB = static_cast<ERTHudZone>(B);
			const FLinearColor ColoreB = URTHudZoneWidget::BlockoutColor(ZonaB);

			// `Equals` con tolleranza: due tinte adiacenti sul cerchio non devono coincidere, e un
			// confronto esatto su float direbbe «diverse» anche per una differenza invisibile.
			if (ColoreA.Equals(ColoreB, 0.05f))
			{
				AddError(FString::Printf(
					TEXT("le zone `%s` e `%s` hanno lo stesso colore di blockout (R%.2f G%.2f B%.2f): ")
					TEXT("a schermo non si distinguono, ed e' l'unica cosa che il blockout deve fare."),
					*URTHudZoneWidget::ZoneName(ZonaA), *URTHudZoneWidget::ZoneName(ZonaB),
					ColoreA.R, ColoreA.G, ColoreA.B));
			}
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudZoneNamesCoverEveryValueTest,
	"RefactorTactics.ScreenHud.HudZoneNamesCoverEveryValue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTHudZoneNamesCoverEveryValueTest::RunTest(const FString&)
{
	// 🔑 **Il default dello switch e' una trappola silenziosa**: chi aggiunge un valore all'enum e dimentica
	// `ZoneName` non rompe la compilazione, ottiene «<non e' una zona>» dentro i messaggi degli ALTRI test —
	// che diventano illeggibili proprio mentre qualcosa non va.
	for (int32 I = 0; I < static_cast<int32>(ERTHudZone::Count); ++I)
	{
		const ERTHudZone Zona = static_cast<ERTHudZone>(I);
		const FString Nome = URTHudZoneWidget::ZoneName(Zona);

		TestFalse(
			*FString::Printf(TEXT("la zona di indice %d ha un nome (ne ha reso `%s`)"), I, *Nome),
			Nome.Contains(TEXT("non e' una zona")));
	}

	// E la sentinella NON deve avere un nome: se lo avesse, sarebbe diventata una zona senza che nessuno
	// lo decidesse.
	TestTrue(TEXT("`Count` non e' una zona, e il suo nome lo dice"),
		URTHudZoneWidget::ZoneName(ERTHudZone::Count).Contains(TEXT("non e' una zona")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
