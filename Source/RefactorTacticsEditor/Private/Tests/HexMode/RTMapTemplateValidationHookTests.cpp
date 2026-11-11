// L'aggancio che fa girare DA SOLA la validazione del map template.
//
// 🔴 **Il difetto che questo file esiste per non far tornare** (misurato il 2026-09-07): le regole di
// `URTMapTemplateLibrary` erano scritte, testate e verdi — e non le eseguiva nessuno. Vivevano in
// `AActor::CheckForErrors`, che il `MAP CHECKDEP` automatico dell'editor **non invoca**: con uno spawn
// duplicato salvato nel livello, il check all'apertura riportava `0 Error(s)`. L'unico modo di vederle era
// `Build > Map Check` a mano, cioe' una cosa da ricordarsi.
//
// ⚠️ **Questo test misura l'ISCRIZIONE, non le regole.** Le regole hanno gia' i loro sei test nel modulo
// runtime, e resterebbero tutti verdi cancellando le due righe di `StartupModule` — che e' precisamente il
// modo in cui la feature e' gia' morta una volta. E' la stessa disciplina di
// `LayoutExtensionIsRegisteredOnStartup`.

#include "Misc/AutomationTest.h"

#include "Modules/ModuleManager.h"
#include "RefactorTacticsEditorModule.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapTemplateValidationHookedTest,
	"RefactorTactics.MapTemplate.ValidationIsHookedOnStartup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapTemplateValidationHookedTest::RunTest(const FString&)
{
	const FRefactorTacticsEditorModule* const Module =
		FModuleManager::GetModulePtr<FRefactorTacticsEditorModule>(TEXT("RefactorTacticsEditor"));

	if (!TestNotNull(TEXT("il modulo editor e' caricato"), Module))
	{
		return false;
	}

	// Entrambe le iscrizioni, e non una: al salvataggio serve a chi sta autorando, all'apertura serve a chi
	// eredita una mappa che qualcun altro ha lasciato rotta.
	TestTrue(TEXT("la validazione e' agganciata a PostSaveWorldWithContext e a OnMapOpened"),
		Module->IsValidationHooked());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
