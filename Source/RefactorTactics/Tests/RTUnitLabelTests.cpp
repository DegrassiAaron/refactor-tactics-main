// Etichetta dell'unita' a schermo: la parte PURA (da quale ID nasce il nome mostrato).
// Che l'etichetta si VEDA sopra la testa resta al PIE (`PIE-HEXPLAY-9`); qui si verifica che la
// presentazione non si inventi il nome ma lo derivi dall'ID stabile dell'eroe.

#include "Misc/AutomationTest.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitShortHeroNameTest,
	"RefactorTactics.Unit.ShortHeroNameFromStableId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitShortHeroNameTest::RunTest(const FString&)
{
	const FString Fallback = TEXT("RTUnit_0");

	// Il caso reale: gli HeroId del catalogo sono namespaced.
	TestEqual(TEXT("Hero.Flux -> Flux"), ARTUnit::ShortHeroName(TEXT("Hero.Flux"), Fallback), TEXT("Flux"));
	TestEqual(TEXT("Hero.Riva -> Riva"), ARTUnit::ShortHeroName(TEXT("Hero.Riva"), Fallback), TEXT("Riva"));
	TestEqual(TEXT("Hero.Bastion -> Bastion"), ARTUnit::ShortHeroName(TEXT("Hero.Bastion"), Fallback), TEXT("Bastion"));
	TestEqual(TEXT("Hero.Vektor -> Vektor"), ARTUnit::ShortHeroName(TEXT("Hero.Vektor"), Fallback), TEXT("Vektor"));

	// Unita' legacy (archetipo, nessun eroe): l'etichetta non deve sparire.
	TestEqual(TEXT("NAME_None -> fallback"), ARTUnit::ShortHeroName(NAME_None, Fallback), Fallback);

	// Robustezza: un ID senza punto resta se stesso, non diventa vuoto.
	TestEqual(TEXT("ID senza punto -> se stesso"), ARTUnit::ShortHeroName(TEXT("Flux"), Fallback), TEXT("Flux"));

	// Namespace annidato: conta l'ULTIMO segmento.
	TestEqual(TEXT("A.B.C -> C"), ARTUnit::ShortHeroName(TEXT("Hero.Elite.Flux"), Fallback), TEXT("Flux"));

	// Punto finale senza segmento: non deve produrre una stringa vuota a schermo.
	TestEqual(TEXT("punto finale -> stringa intera, mai vuota"),
		ARTUnit::ShortHeroName(TEXT("Hero."), Fallback), TEXT("Hero."));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
