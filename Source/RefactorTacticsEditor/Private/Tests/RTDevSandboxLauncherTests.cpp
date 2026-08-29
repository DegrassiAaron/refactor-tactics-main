#include "Misc/AutomationTest.h"

#include "Editor.h"
#include "GameMapsSettings.h"
#include "RTDevSandboxLauncherSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Il launcher del Tactical Designer (#1680, slice `L1`) e cosa di esso un test puo' davvero vedere.
 *
 * La slice apre un pannello quando l'editor apre il livello di bootstrap. Di quel comportamento, la parte
 * verificabile senza aprire un livello e' **una sola**: la decisione. Per questo `ShouldOpenFor` e' statica
 * e pura invece di vivere dentro l'handler — non per eleganza, ma perche' altrimenti non resterebbe niente
 * da misurare qui e la slice sarebbe interamente a carico dell'occhio umano.
 *
 * ⛔ **Cosa questi test NON coprono, e dove sta invece.** Che il pannello COMPAIA e' Slate su un editor
 * vivo: nessun automation test lo vede, ed e' una voce di seduta (`editor-sessions.yaml`, `U31`). Lo stesso
 * per il dirty di `L_DevSandbox.umap`: un predicato puro non puo' osservarlo.
 */

/** Il caso per cui la slice esiste. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDevSandboxLauncherOpensOnBootstrapTest,
	"RefactorTactics.DevSandboxLauncher.OpensOnTheBootstrapLevel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDevSandboxLauncherOpensOnBootstrapTest::RunTest(const FString&)
{
	TestTrue(TEXT("il livello di bootstrap apre l'ingresso"),
		URTDevSandboxLauncherSubsystem::ShouldOpenFor(
			TEXT("/Game/RT/Maps/Dev/L_DevSandbox/L_DevSandbox.umap")));

	return true;
}

/**
 * L'altra meta', e non e' simmetrica: qui non basta un caso inventato.
 *
 * I tre nomi sono i livelli **realmente versionati** del progetto accanto a quello di bootstrap. Un test
 * scritto contro un `L_Qualcosa` immaginario passerebbe anche se il predicato accettasse per sbaglio uno
 * dei livelli veri — che e' precisamente il difetto da prendere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDevSandboxLauncherIgnoresOtherLevelsTest,
	"RefactorTactics.DevSandboxLauncher.DoesNotOpenOnGameplayLevels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDevSandboxLauncherIgnoresOtherLevelsTest::RunTest(const FString&)
{
	const TCHAR* const Others[] = {
		TEXT("/Game/RT/Maps/Dev/L_HexArena/L_HexArena.umap"),
		TEXT("/Game/RT/Maps/Dev/L_Prototype/L_Prototype.umap"),
		TEXT("/Game/RT/Maps/Shared/L_Frontend/L_Frontend.umap"),
	};

	for (const TCHAR* const Path : Others)
	{
		TestFalse(FString::Printf(TEXT("%s non apre l'ingresso"), Path),
			URTDevSandboxLauncherSubsystem::ShouldOpenFor(Path));
	}

	return true;
}

/**
 * Il test che giustifica la firma.
 *
 * `FEditorDelegates::OnMapOpened` consegna un **percorso**, non un nome di mappa, e lo stesso livello arriva
 * scritto in forme diverse a seconda di come e' stato aperto — misurato: il log di un avvio reale porta
 * `"../../../../../Repositories/.../L_DevSandbox.umap"`, mentre da Content Browser arriva il percorso di
 * pacchetto. Un predicato che confrontasse la stringa intera direbbe si' a una forma e no alle altre, e il
 * sintomo sarebbe *«il launcher si apre solo quando lo apro in un certo modo»*: falso raramente, quindi
 * difficile da attribuire.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDevSandboxLauncherPathSpellingTest,
	"RefactorTactics.DevSandboxLauncher.PathSpellingDoesNotChangeTheAnswer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDevSandboxLauncherPathSpellingTest::RunTest(const FString&)
{
	const TCHAR* const SameLevel[] = {
		TEXT("/Game/RT/Maps/Dev/L_DevSandbox/L_DevSandbox"),                        // senza estensione
		TEXT("/Game/RT/Maps/Dev/L_DevSandbox/L_DevSandbox.umap"),                   // con estensione
		TEXT("../../../../../Repositories/x/Content/RT/Maps/Dev/L_DevSandbox/L_DevSandbox.umap"), // relativo, come nel log
		TEXT("D:/Repositories/x/Content/RT/Maps/Dev/L_DevSandbox/L_DevSandbox.umap"),             // assoluto
		TEXT("D:\\Repositories\\x\\Content\\RT\\Maps\\Dev\\L_DevSandbox\\L_DevSandbox.umap"),     // separatori Windows
		TEXT("/Game/RT/Maps/Dev/L_DevSandbox/l_devsandbox.umap"),                   // maiuscole diverse
	};

	for (const TCHAR* const Path : SameLevel)
	{
		TestTrue(FString::Printf(TEXT("la stessa mappa scritta come %s da' lo stesso esito"), Path),
			URTDevSandboxLauncherSubsystem::ShouldOpenFor(Path));
	}

	return true;
}

/**
 * ➕ Non richiesto da #1680, e chiude l'anello che il codice puo' rompere in silenzio.
 *
 * Il nome del livello di bootstrap vive in **due** posti: la costante di
 * `RTDevSandboxLauncherSubsystem.cpp` e `EditorStartupMap` in `Config/DefaultEngine.ini`. Se qualcuno
 * rinomina il livello e aggiorna solo l'ini, l'editor si apre dove deve e il launcher tace — con ogni altro
 * test ancora verde, perche' tutti confrontano la costante con se stessa.
 *
 * Qui il confronto e' fra la costante e **la configurazione reale del progetto**, che e' l'unica coppia in
 * cui una deriva puo' esistere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDevSandboxLauncherMatchesStartupMapTest,
	"RefactorTactics.DevSandboxLauncher.BootstrapNameMatchesTheEditorStartupMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDevSandboxLauncherMatchesStartupMapTest::RunTest(const FString&)
{
	const FString StartupMap = GetDefault<UGameMapsSettings>()->EditorStartupMap.GetLongPackageName();

	if (!TestFalse(TEXT("il progetto dichiara una EditorStartupMap"), StartupMap.IsEmpty()))
	{
		return false;
	}

	TestTrue(FString::Printf(TEXT("il launcher riconosce la EditorStartupMap del progetto (%s)"), *StartupMap),
		URTDevSandboxLauncherSubsystem::ShouldOpenFor(StartupMap));

	return true;
}

/**
 * Che l'iscrizione sia in piedi — e cosa questo test NON dimostra.
 *
 * ✅ Dimostra che `Initialize()` e' stato eseguito e ha lasciato un handle valido: se qualcuno togliesse la
 * riga dell'`AddUObject`, o un `return` anticipato la saltasse, questo test diventa rosso. E' la meta' che
 * conta di piu', perche' un launcher non iscritto non si apre mai.
 *
 * ⛔ **Non dimostra la deregistrazione.** Chiamare `Deinitialize()` qui lascerebbe l'editor della sessione
 * di test senza launcher e togliererebbe un tab spawner globale a meta' suite; e istanziarne uno nuovo per
 * fare il giro registrerebbe due volte lo stesso `TabId`. La deregistrazione resta coperta da lettura del
 * codice — `Deinitialize` toglie l'handle e lo azzera — e la si osserva, se si vuole, ricaricando il modulo
 * a mano. Detto invece di simulato: un test che facesse finta sarebbe peggio dell'assenza.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDevSandboxLauncherSubscribesTest,
	"RefactorTactics.DevSandboxLauncher.SubscribesOnInitialize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDevSandboxLauncherSubscribesTest::RunTest(const FString&)
{
	if (!TestNotNull(TEXT("GEditor esiste nel contesto editor"), GEditor))
	{
		return false;
	}

	URTDevSandboxLauncherSubsystem* Launcher = GEditor->GetEditorSubsystem<URTDevSandboxLauncherSubsystem>();
	if (!TestNotNull(TEXT("il subsystem del launcher e' istanziato"), Launcher))
	{
		return false;
	}

	TestTrue(TEXT("il launcher e' iscritto a OnMapOpened"), Launcher->IsSubscribed());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
