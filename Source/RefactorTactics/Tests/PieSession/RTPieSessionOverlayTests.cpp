// L'overlay, per la meta' che non richiede uno schermo.
//
// Il prompt e la mappatura dei tasti sono funzioni pure, e quindi hanno un gate. Che il pannello sia
// LEGGIBILE sopra la scena non ce l'ha, ed e' dichiarato nella spec §4.5: quella meta' si giudica nella
// prima seduta vera, che e' anche la verifica di questo sottosistema.

#include "Misc/AutomationTest.h"
#include "PieSession/RTPieVerdictOverlay.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieOverlayNamesTheItemTest,
	"RefactorTactics.PieSession.OverlayNamesTheItemBeingJudged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieOverlayNamesTheItemTest::RunTest(const FString&)
{
	const FString Prompt = URTPieVerdictOverlay::ComposePrompt(TEXT("PIE-V01-LOG"), INDEX_NONE,
		TEXT("Guarda la colonna di destra: le righe compaiono mentre i turni scorrono.")).ToString();

	TestTrue(TEXT("nomina la voce, che e' l'ancora nel registro"),
		Prompt.Contains(TEXT("PIE-V01-LOG")));
	TestTrue(TEXT("dice dove guardare"), Prompt.Contains(TEXT("colonna di destra")));
	TestTrue(TEXT("e ricorda i tasti"), Prompt.Contains(TEXT("[1]")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieOverlayShowsTheCriterionWhenThereIsOneTest,
	"RefactorTactics.PieSession.OverlayShowsTheCriterionWhenThereIsOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieOverlayShowsTheCriterionWhenThereIsOneTest::RunTest(const FString&)
{
	// Una voce con criteri numerati si giudica un criterio per volta: senza il numero a schermo, chi
	// preme il tasto non sa quale dei sei sta chiudendo.
	const FString ConCriterio =
		URTPieVerdictOverlay::ComposePrompt(TEXT("PIE-V01-SCREENHUD"), 3, FString()).ToString();
	TestTrue(TEXT("il numero compare"), ConCriterio.Contains(TEXT("(3)")));

	const FString SenzaCriterio =
		URTPieVerdictOverlay::ComposePrompt(TEXT("PIE-V01-SCREENHUD"), INDEX_NONE, FString()).ToString();
	TestFalse(TEXT("e quando la voce si giudica intera non si inventa un numero"),
		SenzaCriterio.Contains(TEXT("criterio")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieOverlayKeysMapToVerdictsTest,
	"RefactorTactics.PieSession.OverlayKeysMapToVerdicts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieOverlayKeysMapToVerdictsTest::RunTest(const FString&)
{
	TestTrue(TEXT("1 = PASS"),
		URTPieVerdictOverlay::VerdictForKey(EKeys::One) == ERTPieVerdict::Pass);
	TestTrue(TEXT("2 = FAIL"),
		URTPieVerdictOverlay::VerdictForKey(EKeys::Two) == ERTPieVerdict::Fail);
	TestTrue(TEXT("3 = non giudicabile"),
		URTPieVerdictOverlay::VerdictForKey(EKeys::Three) == ERTPieVerdict::NotJudgeable);

	// ⛔ Il controllo che conta: un tasto qualunque NON e' un verdetto. Senza, muovere la camera con
	// WASD mentre si guarda chiuderebbe il passo.
	TestTrue(TEXT("W non e' un verdetto"),
		URTPieVerdictOverlay::VerdictForKey(EKeys::W) == ERTPieVerdict::Pending);
	TestTrue(TEXT("e nemmeno Enter"),
		URTPieVerdictOverlay::VerdictForKey(EKeys::Enter) == ERTPieVerdict::Pending);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieOverlaySitsInTheLeftColumnTest,
	"RefactorTactics.PieSession.OverlaySitsInTheLeftColumnAndNotOnTheRoster",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieOverlaySitsInTheLeftColumnTest::RunTest(const FString&)
{
	// 🔴 Il difetto che questo gate esiste per impedire ha un verdetto d'autore: *«la scritta sta sotto
	// i nomi degli eroi e non e' visualizzabile»* (#3242, prima seduta reale). La causa era il DEFAULT:
	// nessun allineamento, quindi in alto a sinistra — sopra `URTTeamRosterWidget`, zona `TopLeft`.
	const FRTPieOverlayPlacement Posa = URTPieVerdictOverlay::Placement();

	TestTrue(TEXT("colonna sinistra"), Posa.Horizontal == HAlign_Left);
	TestTrue(TEXT("centrato in verticale, non in cima dove sta il roster"),
		Posa.Vertical == VAlign_Center);
	TestTrue(TEXT("staccato dal bordo"), Posa.LeftMargin > 0.f);

	// ⛔ Il tetto di larghezza non e' estetica: il centro dello schermo resta libero per contratto —
	// la board non si copre — e un prompt senza tetto cresce con la nota dello scenario.
	TestTrue(TEXT("la larghezza ha un tetto"), Posa.MaxWidth > 0.f);
	TestTrue(TEXT("e il tetto lascia libero il centro"), Posa.MaxWidth <= 640.f);
	return true;
}

#endif
