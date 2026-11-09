// Il fumetto della skill (#4025), fetta 1: le funzioni PURE che decidono cosa dice e come si dissolve.
//
// Il widget e l'unita' non hanno copertura headless: `URTUnitOverlayWidget` e' un `UUserWidget` ([D-320]
// punto 5), quindi ogni giudizio vive in `URTHudViewModel` e si prova qui, chiamandolo senza costruire niente.
// Cio' che questi test fissano e' il CONTRATTO del fumetto: chi grida, che parola compare, e le due curve del
// tempo. Il tempo stesso non e' qui: lo detta `ARTTurnManager::PlaybackElapsedTotal`, mai un `DeltaTime` locale.

#include "Misc/AutomationTest.h"
#include "UI/RTHudViewModel.h"
#include "UI/RTSkillCalloutView.h"

#if WITH_DEV_AUTOMATION_TESTS

// ⚠️ Nomi distinti per file: in unity build i test condividono la translation unit. Tutti i nomi di questo
// file portano il prefisso `FRTSkillCallout`.

/**
 * Grida solo una skill di FIRMA: nessun `BaseActionId` e fuori dal catalogo core (R1b).
 *
 * 🔴 L'ultima riga e' quella che da' senso alle altre: una variante di un'azione core (`Hero.X.Variant`
 * che deriva da `Action.Overwatch`) ha un `BaseActionId` e NON grida, anche se il suo id non e' nel
 * catalogo core. Chi guardasse solo l'id la farebbe gridare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutShoutsOnlyForASignatureSkillTest,
	"RefactorTactics.HudViewModel.SkillCallout.ShoutsOnlyForASignatureSkill",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutShoutsOnlyForASignatureSkillTest::RunTest(const FString&)
{
	TestTrue(TEXT("una skill di firma grida: nessuna base, non e' core"),
		URTHudViewModel::ActionShouts(FName(TEXT("Hero.Muiren.TideGuard")), NAME_None, /*bIsCoreAction=*/ false));

	TestFalse(TEXT("una variante con base non grida: e' l'attacco base con un altro nome"),
		URTHudViewModel::ActionShouts(FName(TEXT("Hero.Branth.ImpactShot")), FName(TEXT("Action.BasicAttack")), false));

	TestFalse(TEXT("un'azione del catalogo core non grida, anche senza base"),
		URTHudViewModel::ActionShouts(FName(TEXT("Action.Guard")), NAME_None, /*bIsCoreAction=*/ true));

	TestFalse(TEXT("(R1b) una variante di un'azione core non grida: la base la esclude"),
		URTHudViewModel::ActionShouts(FName(TEXT("Hero.X.Variant")), FName(TEXT("Action.Overwatch")), false));

	return true;
}

/** Il testo del fumetto e' il nome che il kit da' alla skill, cosi' com'e' e senza bandiera di ripiego. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutLabelComesFromTheKitTest,
	"RefactorTactics.HudViewModel.SkillCallout.LabelComesFromTheKit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutLabelComesFromTheKitTest::RunTest(const FString&)
{
	const FRTSkillCalloutView View = URTHudViewModel::BuildSkillCallout(
		FText::FromString(TEXT("Marea circolare")), FName(TEXT("Hero.Muiren.CircularTide")), /*Source*/ 3, /*StartAt*/ 1.25f);

	TestEqual(TEXT("l'etichetta e' il nome del kit, non l'id"), View.Label, FString(TEXT("Marea circolare")));
	TestFalse(TEXT("col nome del kit il ripiego non scatta"), View.bFromFallback);
	TestEqual(TEXT("chi grida e' l'unita' che lancia"), View.SourceStableUnitId, 3);
	TestNearlyEqual(TEXT("l'istante di comparsa e' quello passato"), View.StartAt, 1.25f, 0.0001f);
	TestEqual(TEXT("l'azione e' conservata, cosi' com'e'"), View.ActionId, FName(TEXT("Hero.Muiren.CircularTide")));

	return true;
}

/**
 * Senza un nome nel kit il fumetto dice l'ultimo segmento dell'id, e lo dichiara con `bFromFallback`.
 *
 * ⚠️ Il vuoto e lo spazio sono la STESSA assenza: un `FText` di soli spazi stamperebbe un fumetto vuoto,
 * cioe' una bolla che non dice niente sopra la testa di chi ha lanciato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutFallsBackToTheLastIdSegmentTest,
	"RefactorTactics.HudViewModel.SkillCallout.FallsBackToTheLastIdSegment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutFallsBackToTheLastIdSegmentTest::RunTest(const FString&)
{
	const FName ActionId(TEXT("Hero.Muiren.CircularTide"));

	const FRTSkillCalloutView Empty = URTHudViewModel::BuildSkillCallout(FText::GetEmpty(), ActionId, 3, 0.f);
	TestEqual(TEXT("(vuoto) l'etichetta e' l'ultimo segmento dell'id"), Empty.Label, FString(TEXT("CircularTide")));
	TestTrue(TEXT("(vuoto) il ripiego e' dichiarato"), Empty.bFromFallback);

	const FRTSkillCalloutView Blank = URTHudViewModel::BuildSkillCallout(
		FText::FromString(TEXT("   ")), ActionId, 3, 0.f);
	TestEqual(TEXT("(spazi) lo stesso esito del vuoto"), Blank.Label, FString(TEXT("CircularTide")));
	TestTrue(TEXT("(spazi) il ripiego e' dichiarato"), Blank.bFromFallback);

	// Un id senza punti non ha segmenti da tagliare: il ripiego e' l'id intero, non una stringa vuota.
	const FRTSkillCalloutView NoDot = URTHudViewModel::BuildSkillCallout(
		FText::GetEmpty(), FName(TEXT("Shout")), 3, 0.f);
	TestEqual(TEXT("(senza punti) il ripiego e' l'id intero"), NoDot.Label, FString(TEXT("Shout")));
	TestTrue(TEXT("(senza punti) il ripiego e' dichiarato"), NoDot.bFromFallback);

	return true;
}

/**
 * La curva dell'opacita': piena, poi una coda lineare, poi finita.
 *
 * 🔑 Si guardano i bordi (`0`, inizio coda, fine) e un punto interno, perche' e' ai bordi che una curva a
 * tratti sbaglia di uno. Le durate degenerate (`<= 0`) valgono «spento», non «sempre pieno»: 0 e' il valore
 * con cui il designer spegne il fumetto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutAlphaIsFullThenFadesThenEndsTest,
	"RefactorTactics.HudViewModel.SkillCallout.AlphaIsFullThenFadesThenEnds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutAlphaIsFullThenFadesThenEndsTest::RunTest(const FString&)
{
	TestNearlyEqual(TEXT("all'inizio e' pieno"), URTHudViewModel::SkillCalloutAlpha(0.f, 3.f), 1.f, 0.01f);
	TestNearlyEqual(TEXT("a inizio coda e' ancora pieno"), URTHudViewModel::SkillCalloutAlpha(2.6f, 3.f), 1.f, 0.01f);
	TestNearlyEqual(TEXT("a meta' della coda e' a meta'"), URTHudViewModel::SkillCalloutAlpha(2.8f, 3.f), 0.5f, 0.01f);
	TestNearlyEqual(TEXT("alla fine e' spento"), URTHudViewModel::SkillCalloutAlpha(3.f, 3.f), 0.f, 0.01f);
	TestNearlyEqual(TEXT("oltre la fine resta spento"), URTHudViewModel::SkillCalloutAlpha(4.f, 3.f), 0.f, 0.01f);
	TestNearlyEqual(TEXT("durata zero: spento"), URTHudViewModel::SkillCalloutAlpha(1.f, 0.f), 0.f, 0.01f);
	TestNearlyEqual(TEXT("durata negativa: spento"), URTHudViewModel::SkillCalloutAlpha(1.f, -1.f), 0.f, 0.01f);

	return true;
}

/**
 * Prima dell'inizio il fumetto e' pieno, non spento.
 *
 * ⚠️ `Elapsed` e' `PlaybackElapsedTotal - StartAt`: se il fumetto e' registrato un attimo PRIMA che il tempo
 * del playback lo raggiunga, il valore e' negativo. Trattarlo come «finito» farebbe lampeggiare il fumetto
 * spento proprio nel frame in cui nasce.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutAlphaBeforeStartIsFullTest,
	"RefactorTactics.HudViewModel.SkillCallout.AlphaBeforeStartIsFull",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutAlphaBeforeStartIsFullTest::RunTest(const FString&)
{
	TestNearlyEqual(TEXT("un tempo negativo e' pieno"), URTHudViewModel::SkillCalloutAlpha(-0.5f, 3.f), 1.f, 0.01f);
	return true;
}

/** Il pop: nasce ingrandito a 1,15 e si assesta a 1 in `PopSeconds`, poi resta a 1. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutScalePopsThenSettlesTest,
	"RefactorTactics.HudViewModel.SkillCallout.ScalePopsThenSettles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutScalePopsThenSettlesTest::RunTest(const FString&)
{
	TestNearlyEqual(TEXT("alla nascita e' ingrandito"), URTHudViewModel::SkillCalloutScale(0.f), 1.15f, 0.01f);
	TestNearlyEqual(TEXT("a meta' del pop e' a meta' strada"), URTHudViewModel::SkillCalloutScale(0.05f), 1.075f, 0.01f);
	TestNearlyEqual(TEXT("a fine pop e' assestato"), URTHudViewModel::SkillCalloutScale(0.1f), 1.f, 0.01f);
	TestNearlyEqual(TEXT("dopo il pop resta a 1"), URTHudViewModel::SkillCalloutScale(2.f), 1.f, 0.01f);
	TestNearlyEqual(TEXT("un tempo negativo e' la scala di nascita"), URTHudViewModel::SkillCalloutScale(-1.f), 1.15f, 0.01f);
	TestNearlyEqual(TEXT("pop di durata zero: nessun pop, scala 1"), URTHudViewModel::SkillCalloutScale(0.f, 0.f), 1.f, 0.01f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
