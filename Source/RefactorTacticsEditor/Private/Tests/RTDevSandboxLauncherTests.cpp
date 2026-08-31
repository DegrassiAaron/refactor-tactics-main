#include "Misc/AutomationTest.h"

#include "Editor.h"
#include "GameMapsSettings.h"
#include "RTDevSandboxLauncherSubsystem.h"
#include "RTLauncherWorkspace.h"
#include "ScenarioHarness/RTScenarioAuthoring.h"

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

/**
 * La sessione passa dalla facade, e un rifiuto non ne lascia una a meta' (#1682, slice `L6`).
 *
 * ⚠️ **L'asserzione che vale piu' delle altre e' l'ultima**: dopo un tentativo fallito il launcher NON deve
 * avere una sessione. Una facade aperta su uno scenario che la validazione ha respinto sembrerebbe
 * funzionare — e il primo `Run` misurerebbe qualcosa che nessuno ha dichiarato valido.
 *
 * ⛔ Non copre i pulsanti: `Start Session` e' Slate, ed e' voce di seduta. Copre chi possiede la sessione,
 * che e' la domanda `A3` del referto del 2026-08-29 §8, e cosa resta quando l'apertura non riesce.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDevSandboxLauncherSessionComesFromTheFacadeTest,
	"RefactorTactics.DevSandboxLauncher.SessionComesFromTheFacade",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDevSandboxLauncherSessionComesFromTheFacadeTest::RunTest(const FString&)
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

	// ⚠️ Lo stato d'ingresso si dichiara invece di assumerlo: questo subsystem vive per tutta la sessione
	// d'editor, e un test precedente potrebbe averci lasciato una sessione aperta.
	Launcher->EndSession();
	TestFalse(TEXT("si parte senza sessione"), Launcher->HasSession());

	// Uno scenario del corpus: se il corpus non si legge, il test lo dice invece di fallire sull'asserto
	// sbagliato.
	const FRTLauncherStartDecision Opened = Launcher->StartSession(TEXT("Combat.BasicAttack"));
	if (TestTrue(TEXT("uno scenario canonico apre la sessione"), Opened.bAllowed))
	{
		TestTrue(TEXT("e la sessione risulta aperta"), Launcher->HasSession());
		TestNotNull(TEXT("la sessione E' la facade d'authoring, non un terzo oggetto"), Launcher->GetSession());
	}

	// Un id che l'indice non conosce: rifiuto, con la causa giusta e nessun residuo.
	const FRTLauncherStartDecision Refused = Launcher->StartSession(TEXT("Scenario.CheNonEsiste"));
	TestFalse(TEXT("un id assente non apre la sessione"), Refused.bAllowed);
	TestTrue(TEXT("la causa e' `NotFound`"), Refused.Refusal == ERTLauncherStartRefusal::NotFound);
	TestFalse(TEXT("il motivo e' visibile"), Refused.Reason.IsEmpty());
	TestFalse(TEXT("e NON resta una sessione mezza aperta: ne' quella nuova, ne' quella di prima"), Launcher->HasSession());

	// Senza selezione il launcher non interroga nemmeno la facade.
	const FRTLauncherStartDecision NoSelection = Launcher->StartSession(FString());
	TestFalse(TEXT("senza id non si apre"), NoSelection.bAllowed);
	TestTrue(TEXT("e la causa e' la selezione"), NoSelection.Refusal == ERTLauncherStartRefusal::NoSelection);

	Launcher->EndSession();
	TestFalse(TEXT("chiudere lascia il launcher senza sessione"), Launcher->HasSession());

	// Idempotente: chiudere due volte non e' un errore.
	Launcher->EndSession();

	return true;
}

/**
 * Una superficie non dichiarata **rifiuta** invece di non fare niente.
 *
 * ⚠️ Il caso positivo — attivare Map o Scenario — non si asserisce qui: attiva un editor mode e invoca un
 * tab globale in mezzo alla suite, cioe' modifica l'editor di chi sta misurando. Cio' che si puo' misurare
 * senza effetti collaterali e' il rifiuto, ed e' anche il ramo dove il `silent fallback` si nasconderebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDevSandboxLauncherPendingSurfaceIsRefusedTest,
	"RefactorTactics.DevSandboxLauncher.PendingSurfaceIsRefused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDevSandboxLauncherPendingSurfaceIsRefusedTest::RunTest(const FString&)
{
	URTDevSandboxLauncherSubsystem* Launcher = GEditor ? GEditor->GetEditorSubsystem<URTDevSandboxLauncherSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("il subsystem del launcher e' istanziato"), Launcher))
	{
		return false;
	}

	// Dal registro, non da un nome scritto qui: quando #1625 consegnera' il playback questa voce diventera'
	// dichiarata, e il test seguira' il registro invece di restare a pinnare un'assenza superata.
	const FRTLauncherSurface* Pending = FRTLauncherWorkspace::Surfaces().FindByPredicate(
		[](const FRTLauncherSurface& Surface) { return !Surface.bDeclared; });

	if (Pending)
	{
		TestFalse(TEXT("una superficie non dichiarata non si attiva"), Launcher->ActivateSurface(Pending->Key));
	}

	TestFalse(TEXT("una chiave che non esiste non si attiva"), Launcher->ActivateSurface(TEXT("SuperficieCheNonEsiste")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
