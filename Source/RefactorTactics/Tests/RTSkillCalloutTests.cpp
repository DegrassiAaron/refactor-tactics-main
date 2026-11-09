// Il fumetto della skill (#4025), fetta 1: le funzioni PURE che decidono cosa dice e come si dissolve.
//
// Il widget non ha copertura headless: `URTUnitOverlayWidget` e' un `UUserWidget` ([D-320] punto 5), quindi
// ogni giudizio vive in `URTHudViewModel` e si prova qui, chiamandolo senza costruire niente. L'UNITA' invece si
// spawna in un mondo di prova: la seconda sezione di questo file fissa il suo canale (`ShowSkillCallout`,
// `SetSkillCalloutClock`, `ClearSkillCallout`) leggendo `LastSkillCalloutForTest`, perche' senza widget
// quel campo e' l'unico modo di vedere se il fumetto e' arrivato alla presentazione.
// Cio' che questi test fissano e' il CONTRATTO del fumetto: chi grida, che parola compare, e le due curve del
// tempo. Il tempo stesso non e' qui: lo detta `ARTTurnManager::PlaybackElapsedTotal`, mai un `DeltaTime` locale.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Kismet/GameplayStatics.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Unit/RTUnit.h"
#include "UI/RTHudViewModel.h"
#include "UI/RTSkillCalloutView.h"
#include "RTWorldFixtures.h"

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

// ------------------------------------------------------------------------------------------------------------
// L'UNITA': il canale del fumetto, senza widget.
// ------------------------------------------------------------------------------------------------------------

namespace
{
	/** L'identita' con cui la fixture marca l'unita': diversa da 0 perche' lo 0 e' «nessuno» ([D-063]). */
	constexpr int32 SkillCalloutProvaUnitId = 7;

	/**
	 * Un'unita' di Muiren in un mondo di prova (modello `SpawnBeatUnit`, `RTPlaybackActivationTests.cpp`).
	 *
	 * ⚠️ `StableUnitId` lo assegna `EnsureMatchRoster` a partita avviata: qui non c'e' una partita, quindi la
	 * fixture lo scrive a mano. Lasciato a 0 il test che confronta chi grida con l'unita' sarebbe vacuo, perche'
	 * `0 == 0` anche se il canale ignorasse l'id.
	 */
	ARTUnit* SpawnSkillCalloutUnit(UWorld* World)
	{
		ARTUnit* Unit = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!Unit) { return nullptr; }
		Unit->TeamId = 0;
		Unit->bIsBotControlled = false;
		Unit->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeMuiren());
		UGameplayStatics::FinishSpawningActor(Unit, FTransform::Identity);
		Unit->PlaceOnCell(FRTCellId(0, 0), FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		Unit->StableUnitId = SkillCalloutProvaUnitId;
		return Unit;
	}
}

/**
 * Il campo si scrive SEMPRE, anche senza widget.
 *
 * 🔑 E' il punto di tutto il canale: in un mondo headless `GetOverlayWidgetObject()` risponde `nullptr`, e se la
 * scrittura stesse dentro il controllo del widget nessun test potrebbe dire se il fumetto e' arrivato alla
 * presentazione. La premessa lo misura invece di darlo per scontato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutUnitRecordsTheCalloutWithoutAWidgetTest,
	"RefactorTactics.HudViewModel.SkillCallout.UnitRecordsTheCalloutWithoutAWidget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutUnitRecordsTheCalloutWithoutAWidgetTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTUnit* Unit = SpawnSkillCalloutUnit(World);
	if (!TestNotNull(TEXT("unita' di prova"), Unit)) { return false; }
	TestNull(TEXT("premessa: in un mondo headless il widget non esiste"), Unit->GetOverlayWidgetObject());

	const FName ActionId(TEXT("Hero.Muiren.TideGuard"));
	Unit->ShowSkillCallout(ActionId, 2.f);

	const FRTSkillCalloutView& Registrato = Unit->LastSkillCalloutForTest();
	TestEqual(TEXT("l'azione registrata e' quella che grida"), Registrato.ActionId, ActionId);
	TestFalse(TEXT("l'etichetta non e' vuota"), Registrato.Label.IsEmpty());
	TestEqual(TEXT("l'etichetta e' il nome che il kit da' alla skill"),
		Registrato.Label, Unit->KitDisplayNameFor(ActionId).ToString());
	TestFalse(TEXT("il kit di Muiren nomina la skill: il ripiego non scatta"), Registrato.bFromFallback);
	TestEqual(TEXT("chi grida e' questa unita'"), Registrato.SourceStableUnitId, SkillCalloutProvaUnitId);
	TestNearlyEqual(TEXT("l'istante di comparsa e' quello passato"), Registrato.StartAt, 2.f, 0.0001f);
	TestTrue(TEXT("il fumetto e' in vita"), Unit->IsSkillCalloutLive());

	return true;
}

/**
 * Un secondo fumetto sostituisce il primo: un fumetto per unita', il piu' recente.
 *
 * ⚠️ E' il limite «un evento, un token» che il token di danno ha gia' dichiarato, letto dalla parte del campo:
 * il registro non e' una coda. Se due skill gridano nello stesso playback la seconda e' quella che si vede.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutSecondCalloutReplacesTheFirstTest,
	"RefactorTactics.HudViewModel.SkillCallout.SecondCalloutReplacesTheFirst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutSecondCalloutReplacesTheFirstTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTUnit* Unit = SpawnSkillCalloutUnit(World);
	if (!TestNotNull(TEXT("unita' di prova"), Unit)) { return false; }

	Unit->ShowSkillCallout(FName(TEXT("Hero.Muiren.TideGuard")), 2.f);
	Unit->ShowSkillCallout(FName(TEXT("Hero.Muiren.CircularTide")), 2.5f);

	TestEqual(TEXT("resta l'azione della seconda"),
		Unit->LastSkillCalloutForTest().ActionId, FName(TEXT("Hero.Muiren.CircularTide")));
	TestNearlyEqual(TEXT("e il suo istante di comparsa"), Unit->LastSkillCalloutForTest().StartAt, 2.5f, 0.0001f);
	TestTrue(TEXT("il fumetto e' in vita"), Unit->IsSkillCalloutLive());

	return true;
}

/**
 * `ClearSkillCallout` chiude la VITA e lascia il REGISTRO.
 *
 * 🔑 Lo scopo del campo e' farsi leggere a playback finito: un test di partita guarda cosa l'unita' ha gridato
 * dopo che la sequenza e' terminata. Se `Clear` azzerasse anche il registro, l'oracolo sparirebbe nel momento
 * in cui serve, e il test passerebbe o fallirebbe a seconda di quando lo si legge.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutClearKeepsTheRecordButEndsTheLifeTest,
	"RefactorTactics.HudViewModel.SkillCallout.ClearKeepsTheRecordButEndsTheLife",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutClearKeepsTheRecordButEndsTheLifeTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTUnit* Unit = SpawnSkillCalloutUnit(World);
	if (!TestNotNull(TEXT("unita' di prova"), Unit)) { return false; }

	TestFalse(TEXT("premessa: un'unita' nuova non ha un fumetto in vita"), Unit->IsSkillCalloutLive());

	const FName ActionId(TEXT("Hero.Muiren.TideGuard"));
	Unit->ShowSkillCallout(ActionId, 2.f);
	TestTrue(TEXT("premessa: dopo Show il fumetto e' in vita"), Unit->IsSkillCalloutLive());

	Unit->ClearSkillCallout();

	TestFalse(TEXT("dopo Clear la vita e' finita"), Unit->IsSkillCalloutLive());
	TestEqual(TEXT("ma il registro resta: l'azione e' ancora leggibile"),
		Unit->LastSkillCalloutForTest().ActionId, ActionId);
	TestFalse(TEXT("e anche l'etichetta"), Unit->LastSkillCalloutForTest().Label.IsEmpty());

	return true;
}

/**
 * Il tempo che l'unita' porta al widget viene registrato.
 *
 * ⚠️ L'unita' non misura il tempo: lo riceve dal chiamante (`PlaybackElapsedTotal`) e lo inoltra. Il registro
 * dell'ultimo valore e' cio' che rende osservabile, senza widget, che il tempo sia davvero passato di qui.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSkillCalloutClockIsRecordedTest,
	"RefactorTactics.HudViewModel.SkillCallout.ClockIsRecorded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSkillCalloutClockIsRecordedTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTUnit* Unit = SpawnSkillCalloutUnit(World);
	if (!TestNotNull(TEXT("unita' di prova"), Unit)) { return false; }

	Unit->SetSkillCalloutClock(4.2f);

	TestNearlyEqual(TEXT("l'orologio registrato e' l'ultimo ricevuto"), Unit->SkillCalloutClockForTest(), 4.2f, 0.0001f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
