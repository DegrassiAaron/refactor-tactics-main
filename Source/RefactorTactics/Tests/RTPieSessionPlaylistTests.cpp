// Comporre la coda: cosa entra, cosa resta fuori, e cosa ferma tutto.

#include "Misc/AutomationTest.h"
#include "PieSession/RTPieSessionPlaylist.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPiePlaylistReadsVerifiesWithoutFullLoadTest,
	"RefactorTactics.PieSession.ReadsVerifiesWithoutAFullLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPiePlaylistReadsVerifiesWithoutFullLoadTest::RunTest(const FString&)
{
	// 🔑 Lo scenario qui sotto e' INVALIDO per il loader — non ha unita' — e deve comunque dichiarare le
	// proprie voci. Se la composizione passasse dal loader completo, uno scenario con un difetto in
	// fondo al file sparirebbe dalla coda invece di fallire con un motivo quando lo si avvia.
	const TCHAR* Json = TEXT(R"({
		"scenarioId": "Spec.PieSession.SenzaUnita",
		"verifies": ["PIE-A", "PIE-B"],
		"units": []
	})");

	TArray<FString> Voci;
	TestTrue(TEXT("si legge lo stesso"), URTPieSessionPlaylist::ReadVerifies(Json, Voci));
	TestEqual(TEXT("e porta due voci"), Voci.Num(), 2);
	TestEqual(TEXT("nell'ordine del file"), Voci[0], TEXT("PIE-A"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPiePlaylistNoFieldIsNotAnErrorTest,
	"RefactorTactics.PieSession.AScenarioWithoutVerifiesIsNotAnError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPiePlaylistNoFieldIsNotAnErrorTest::RunTest(const FString&)
{
	// La maggioranza del corpus non dichiara voci, ed e' lecito: non e' un difetto da segnalare a ogni
	// composizione, altrimenti il rumore coprirebbe le esclusioni che contano.
	const TCHAR* Json = TEXT(R"({ "scenarioId": "Spec.PieSession.Muto", "units": [] })");

	TArray<FString> Voci;
	TestTrue(TEXT("il file si legge"), URTPieSessionPlaylist::ReadVerifies(Json, Voci));
	TestEqual(TEXT("e non dichiara niente"), Voci.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPiePlaylistPrefixTakesEveryItemTest,
	"RefactorTactics.PieSession.PrefixTakesEveryItemOfTheScenario",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPiePlaylistPrefixTakesEveryItemTest::RunTest(const FString&)
{
	// Il guadagno principale in una riga: un allestimento che copre sette voci si apre UNA volta.
	const FRTPieSessionPlan Plan = URTPieSessionPlaylist::Compose(TEXT("Visual.Perception.*"));

	TestTrue(TEXT("la coda si puo' eseguire"), Plan.IsRunnable());
	TestEqual(TEXT("sette passi da un solo allestimento"), Plan.Steps.Num(), 7);
	for (const FRTPieSessionStep& S : Plan.Steps)
	{
		TestEqual(TEXT("tutti dallo stesso scenario"), S.ScenarioId,
			TEXT("Visual.Perception.Acceptance"));
		TestTrue(TEXT("e ognuno nomina la propria voce"), S.PieItem.StartsWith(TEXT("PIE-")));
	}
	TestEqual(TEXT("nell'ordine in cui il file le dichiara"), Plan.Steps[0].PieItem, TEXT("PIE-KNOW1"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPiePlaylistByItemIdTest,
	"RefactorTactics.PieSession.ItemIdResolvesToItsScenario",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPiePlaylistByItemIdTest::RunTest(const FString&)
{
	const FRTPieSessionPlan Plan = URTPieSessionPlaylist::Compose(TEXT("PIE-VIS-SIGHTWALL"));

	TestTrue(TEXT("la coda si puo' eseguire"), Plan.IsRunnable());
	TestEqual(TEXT("un passo"), Plan.Steps.Num(), 1);
	TestEqual(TEXT("e risale al suo allestimento"), Plan.Steps[0].ScenarioId,
		TEXT("Visual.Map.SightWallIsWalkable"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPiePlaylistUnknownItemIsExcludedTest,
	"RefactorTactics.PieSession.UnknownItemIsExcludedWithAReason",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPiePlaylistUnknownItemIsExcludedTest::RunTest(const FString&)
{
	const FRTPieSessionPlan Plan = URTPieSessionPlaylist::Compose(TEXT("PIE-NON-DICHIARATA-DA-NESSUNO"));

	TestEqual(TEXT("nessun passo"), Plan.Steps.Num(), 0);
	TestEqual(TEXT("ma compare fra le escluse"), Plan.Excluded.Num(), 1);
	TestTrue(TEXT("col proprio nome dentro"),
		Plan.Excluded[0].Contains(TEXT("PIE-NON-DICHIARATA-DA-NESSUNO")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPiePlaylistMixedSelectionKeepsWhatItCanTest,
	"RefactorTactics.PieSession.MixedSelectionKeepsWhatItCan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPiePlaylistMixedSelectionKeepsWhatItCanTest::RunTest(const FString&)
{
	// Una voce sconosciuta non deve buttare via quella buona: chi compone una coda di dieci voci e ne
	// sbaglia una non vuole riscriverle tutte.
	const FRTPieSessionPlan Plan =
		URTPieSessionPlaylist::Compose(TEXT("PIE-VIS-SIGHTWALL,PIE-NON-ESISTE"));

	TestEqual(TEXT("la voce buona entra"), Plan.Steps.Num(), 1);
	TestEqual(TEXT("e quella sbagliata si dichiara"), Plan.Excluded.Num(), 1);
	return true;
}

#endif
