#include "Misc/AutomationTest.h"

#include "Misc/ConfigCacheIni.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * **La configurazione di packaging del namespace d'authoring** (#1804, `D-280`).
 *
 * 🔴 **Cosa questi test NON provano, ed e' la prima cosa da dire.** Non provano che un pacchetto escluda
 * davvero gli strumenti d'authoring: leggono `DefaultGame.ini`, non un `.utoc`. L'oracolo di quel fatto e'
 * un pacchetto vero — `BuildCookRun` piu' `UnrealPak -List` — ed e' registrato fra le verifiche manuali,
 * validato per mutazione. Questi test sono **guardiani di regressione**: impediscono che la riga sparisca
 * senza che nulla lo dica, il che e' il modo in cui un'esclusione di packaging si perde davvero.
 *
 * ⚠️ Leggono il **file**, non l'oggetto di settings del motore: `UProjectPackagingSettings` vive in
 * `DeveloperToolSettings`, e tirarsi dentro un modulo per rileggere due righe costerebbe piu' di cio' che
 * aggiunge. Il divario resta dichiarato qui invece di essere lasciato credere.
 *
 * 🔴 **E il divario si e' rivelato piu' largo di cosi', misurato il 2026-09-03 su pacchetti veri.**
 * Cotto due volte con `BuildCookRun`, una con `DirectoriesToNeverCook` e una senza, i container sono
 * risultati **identici**: `WBP_RT_ScenarioComposer` e `WBP_RT_GrayKitPlayground` sono `EditorUtilityWidget`,
 * quindi editor-only, e il cook li scarta **per classe** (`LogCook: SkipOnlyEditorOnly is enabled`) prima
 * che una directory conti qualcosa. ∴ **Oggi la riga che questo test presidia non e' cio' che tiene i due
 * strumenti fuori dal pacchetto**, e un verde qui non va letto come "gli strumenti sono esclusi".
 *
 * ✅ **Cio' che il test presidia resta necessario**, e anche questo e' misurato: un never-cook di controllo
 * su `/Game/RT/UI/Icons` fa sparire 62 package dal container, quindi il meccanismo funziona e batte
 * `DirectoriesToAlwaysCook` su una sottodirectory. La riga e' la garanzia della **famiglia aperta** — il
 * primo asset NON editor-only messo sotto `/Game/RT/Editor/` sara' escluso solo da lei.
 * ⛔ Quindi questo test difende un invariante **futuro**: e' il caso in cui un guardiano di regressione
 * vale di piu' del fatto che oggi verifica, non di meno.
 */

namespace
{
	const TCHAR* RTPackagingSection = TEXT("/Script/UnrealEd.ProjectPackagingSettings");

	/** Le voci di una direttiva `+Chiave=` come le legge `GConfig`, cioe' senza il prefisso di merge. */
	TArray<FString> ReadPackagingArray(const TCHAR* Key)
	{
		TArray<FString> Values;
		GConfig->GetArray(RTPackagingSection, Key, Values, GGameIni);
		return Values;
	}

	bool AnyContains(const TArray<FString>& Values, const TCHAR* Needle)
	{
		return Values.ContainsByPredicate([Needle](const FString& Value) { return Value.Contains(Needle); });
	}
}

/**
 * 🔑 **Il namespace d'authoring e' escluso, e l'esclusione ha senso solo CONTRO l'inclusione.**
 *
 * ⛔ Un test che verificasse la sola riga di never-cook sarebbe verde anche il giorno in cui qualcuno
 * togliesse `DirectoriesToAlwaysCook`: a quel punto l'esclusione non escluderebbe piu' niente di
 * particolare, e il pannello sparirebbe dal pacchetto **per un'altra ragione**. Le due righe si leggono
 * insieme, e questo test le pretende entrambe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPackagingEditorNamespaceNeverCookedTest,
	"RefactorTactics.Packaging.EditorNamespaceIsNeverCooked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPackagingEditorNamespaceNeverCookedTest::RunTest(const FString&)
{
	const TArray<FString> AlwaysCook = ReadPackagingArray(TEXT("DirectoriesToAlwaysCook"));
	const TArray<FString> NeverCook  = ReadPackagingArray(TEXT("DirectoriesToNeverCook"));

	// ⛔ Senza questa, le asserzioni sotto sarebbero vacue: due liste vuote passerebbero un test che
	// cercasse solo l'assenza di qualcosa.
	if (!TestTrue(TEXT("DefaultGame.ini dichiara delle directory da cuocere"), AlwaysCook.Num() > 0))
	{
		return false;
	}

	TestTrue(TEXT("/Game/RT e' incluso nel cook"), AnyContains(AlwaysCook, TEXT("/Game/RT")));
	TestTrue(TEXT("e /Game/RT/Editor e' escluso, altrimenti la riga sopra lo includerebbe"),
		AnyContains(NeverCook, TEXT("/Game/RT/Editor")));

	// ⚠️ L'esclusione e' una FAMIGLIA: vale per la directory, non per i singoli strumenti. Un elenco di
	// asset qui dichiarerebbe esclusi solo quelli che qualcuno ha gia' creato — e `D-280` istituisce un
	// namespace proprio per non doverlo aggiornare a ogni strumento nuovo.
	TestFalse(TEXT("l'esclusione non nomina i singoli asset, ma la directory"),
		AnyContains(NeverCook, TEXT(".uasset")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
