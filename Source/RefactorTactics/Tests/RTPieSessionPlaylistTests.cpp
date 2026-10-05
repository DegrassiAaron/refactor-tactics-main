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

	// ⚠️ Il prefisso prende OGNI scenario che comincia cosi', e il corpus cresce: dal 2026-10-05 sotto
	// `Visual.Perception.` c'e' anche `RevealDuringMove` (#3458). Questo test diceva «sette passi, tutti da
	// `Acceptance`», cioe' fotografava il corpus di quel giorno invece della regola, ed e' caduto al primo
	// scenario nuovo. La regola e' questa: le voci di un allestimento entrano TUTTE, CONTIGUE — l'allestimento
	// si apre una volta — e nell'ordine in cui il file le dichiara.
	TArray<FString> DaAcceptance;
	int32 Primo = INDEX_NONE;
	int32 Ultimo = INDEX_NONE;
	for (int32 I = 0; I < Plan.Steps.Num(); ++I)
	{
		const FRTPieSessionStep& S = Plan.Steps[I];
		TestTrue(TEXT("ogni passo viene da uno scenario del prefisso"),
			S.ScenarioId.StartsWith(TEXT("Visual.Perception.")));
		TestTrue(TEXT("e nomina la propria voce"), S.PieItem.StartsWith(TEXT("PIE-")));
		if (S.ScenarioId == TEXT("Visual.Perception.Acceptance"))
		{
			if (Primo == INDEX_NONE) { Primo = I; }
			Ultimo = I;
			DaAcceptance.Add(S.PieItem);
		}
	}
	TestEqual(TEXT("sette passi da un solo allestimento"), DaAcceptance.Num(), 7);
	TestEqual(TEXT("contigui: l'allestimento si apre UNA volta"), Ultimo - Primo + 1, DaAcceptance.Num());
	if (DaAcceptance.Num() > 0)
	{
		TestEqual(TEXT("nell'ordine in cui il file le dichiara"), DaAcceptance[0], TEXT("PIE-KNOW1"));
	}
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
