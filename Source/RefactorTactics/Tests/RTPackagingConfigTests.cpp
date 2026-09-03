#include "Misc/AutomationTest.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

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
 *
 * 🔴 **Riscritto in code review (#2136), che ha trovato la prima versione VACUA su tre fronti.** Vale la
 * pena elencarli, perche' sono tre modi diversi di scrivere un oracolo che non puo' fallire:
 *
 * 1. **Leggeva `GGameIni`, cioe' la gerarchia MERGED, mentre il docstring dichiarava di leggere il file.**
 *    `GConfig` fonde `Base*.ini` + `Config/DefaultGame.ini` + `Saved/Config/WindowsEditor/Game.ini`. Chi
 *    avesse quella sezione nel proprio `Saved/` — Unreal ce la scrive da se' con `SaveConfig()` — poteva
 *    cancellare la riga dal file **versionato**, vedere verde e mergiare. Cioe' esattamente *"la riga
 *    sparisce senza che nulla lo dica"*, l'unica regressione che questo test dichiara di impedire.
 *    ∴ ora legge `Config/DefaultGame.ini` dal disco, e il codice dice cio' che il commento prometteva.
 * 2. **Confrontava per SOTTOSTRINGA.** `Contains("/Game/RT/Editor")` e' vero anche per
 *    `(Path="/Game/RT/Editor/Scenario")` — cioe' restringere l'esclusione al solo Composer di oggi
 *    passava, perdendo la **famiglia aperta** su cui poggia tutto il disegno. Simmetricamente
 *    `Contains("/Game/RT")` sopravviveva a un `AlwaysCook` ristretto a `/Game/RT/UI`, che e' proprio il
 *    giorno in cui la coppia delle due righe smette di avere senso. ∴ ora il `Path` si estrae e si
 *    confronta per **uguaglianza esatta**.
 * 3. **Asseriva che nessuna voce contenesse `.uasset`.** `DirectoriesToNeverCook` e' un
 *    `TArray<FDirectoryPath>` di package path: `.uasset` non compare **mai**, nemmeno nel caso rotto —
 *    e il caso rotto che l'asserzione descriveva si scrive senza estensione
 *    (`(Path="/Game/RT/Editor/Scenario/WBP_RT_ScenarioComposer")`), quindi passava anche lui. Verde nel
 *    caso buono e verde in quello cattivo: la definizione di un non-oracolo.
 *    ∴ sostituita dal confronto esatto del punto 2, che il caso rotto lo **fa cadere**.
 */

namespace
{
	const TCHAR* RTPackagingSection = TEXT("[/Script/UnrealEd.ProjectPackagingSettings]");

	/** Il file VERSIONATO, non la gerarchia merged: e' la differenza fra sorvegliare il repository e sorvegliare la macchina. */
	FString RTDefaultGameIniPath()
	{
		return FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("DefaultGame.ini"));
	}

	/**
	 * I `Path` dichiarati da `+<Key>=(Path="...")` nella sezione di packaging, **esattamente come scritti**.
	 *
	 * ⚠️ Traccia la sezione: la stessa chiave sotto un'altra intestazione non conterebbe, e un file di
	 * config e' fatto di sezioni prima che di righe.
	 */
	TArray<FString> ReadPackagingPaths(const TArray<FString>& Righe, const TCHAR* Key)
	{
		const FString Prefisso = FString::Printf(TEXT("+%s=(Path=\""), Key);
		TArray<FString> Paths;
		bool bDentroLaSezione = false;

		for (const FString& Riga : Righe)
		{
			const FString Pulita = Riga.TrimStartAndEnd();
			if (Pulita.StartsWith(TEXT("[")))
			{
				bDentroLaSezione = Pulita.Equals(RTPackagingSection);
				continue;
			}
			if (!bDentroLaSezione || !Pulita.StartsWith(Prefisso))
			{
				continue;
			}

			const int32 Inizio = Prefisso.Len();
			const int32 Fine = Pulita.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, Inizio);
			if (Fine > Inizio)
			{
				Paths.Add(Pulita.Mid(Inizio, Fine - Inizio));
			}
		}
		return Paths;
	}
}

/**
 * 🔑 **Il namespace d'authoring e' escluso, e l'esclusione ha senso solo CONTRO l'inclusione.**
 *
 * ⛔ Un test che verificasse la sola riga di never-cook sarebbe verde anche il giorno in cui qualcuno
 * togliesse `DirectoriesToAlwaysCook`: a quel punto l'esclusione non escluderebbe piu' niente di
 * particolare, e il pannello sparirebbe dal pacchetto **per un'altra ragione**. Le due righe si leggono
 * insieme, e questo test le pretende entrambe — **per uguaglianza esatta**, perche' entrambe possono essere
 * ristrette invece che cancellate, e una restrizione non assomiglia a una regressione mentre lo e'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPackagingEditorNamespaceNeverCookedTest,
	"RefactorTactics.Packaging.EditorNamespaceIsNeverCooked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPackagingEditorNamespaceNeverCookedTest::RunTest(const FString&)
{
	TArray<FString> Righe;
	if (!TestTrue(TEXT("Config/DefaultGame.ini si legge"), FFileHelper::LoadFileToStringArray(Righe, *RTDefaultGameIniPath())))
	{
		return false;
	}

	const TArray<FString> AlwaysCook = ReadPackagingPaths(Righe, TEXT("DirectoriesToAlwaysCook"));
	const TArray<FString> NeverCook  = ReadPackagingPaths(Righe, TEXT("DirectoriesToNeverCook"));

	// ⛔ Senza questa, le asserzioni sotto sarebbero vacue: due liste vuote passerebbero un test che
	// cercasse solo l'assenza di qualcosa. ➕ E vale anche come controllo del PARSER: se un domani il
	// formato di queste righe cambiasse, `ReadPackagingPaths` restituirebbe liste vuote e questo test
	// direbbe "la riga non c'e'" per un difetto suo. Meglio rosso che verde su nulla.
	if (!TestTrue(TEXT("DefaultGame.ini dichiara delle directory da cuocere"), AlwaysCook.Num() > 0))
	{
		return false;
	}

	// 🔑 Uguaglianza, non sottostringa: `/Game/RT/UI` contiene `/Game/RT` e non e' la stessa promessa.
	TestTrue(TEXT("/Game/RT e' incluso nel cook, per intero e non una sua sottocartella"),
		AlwaysCook.Contains(TEXT("/Game/RT")));

	// 🔴 Qui l'uguaglianza e' il punto dell'intera issue: `/Game/RT/Editor/Scenario` proteggerebbe il
	// Composer di oggi e lascerebbe scoperto il prossimo strumento — che e' cio' che `D-280` istituisce il
	// namespace per NON dover fare a mano a ogni asset.
	TestTrue(TEXT("e la FAMIGLIA /Game/RT/Editor e' esclusa, non un suo singolo ramo"),
		NeverCook.Contains(TEXT("/Game/RT/Editor")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
