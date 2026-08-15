// LA PORTA D'INGRESSO allo scenario da riga di comando: `-RTScenario=<Id>`.
//
// Esiste per una ragione misurata, non per simmetria con la console: in una build **Shipping**
// `-dpcvars=rt.Test.Scenario=...` non arriva. In `DeviceProfileManager.cpp` tutto il parsing di
// `-dpcvars=`/`-dpcvar=`/`-forcedpcvars=` sta dentro `#if !UE_BUILD_SHIPPING`, quindi la variabile non viene
// mai impostata, il GameMode legge vuoto e allestisce la partita normale — senza un errore che lo dica,
// perche' in Shipping anche il logging e' compilato fuori. Trovato eseguendo un pacchetto vero (`#926`).
//
// ⚠️ QUESTI TEST NON DUPLICANO `RTScenarioAutoRunTests.cpp`. Quel file copre la coppia
// proprieta' vs console (`Scenario.AutoRunConsoleOverridesProperty`) e il fatto che l'override non sia
// silenzioso; qui si copre **solo** la sorgente nuova e il suo posto nella precedenza.

#include "Misc/AutomationTest.h"
#include "RTWorldFixtures.h"
#include "RTGameMode.h"
#include "Misc/CommandLine.h"
#include "HAL/IConsoleManager.h"

#if WITH_DEV_AUTOMATION_TESTS

/** Definita in ScenarioHarness/RTTestConsole.cpp. */
extern TAutoConsoleVariable<FString> CVarRTTestScenario;

namespace
{
	// Nomi distinti da ogni altro file di test: la unity build condivide la translation unit.

	/**
	 * Ripristina la riga di comando qualunque cosa succeda nel test.
	 *
	 * `FCommandLine` e' stato globale del processo, e da quando esiste `-RTScenario=` **lo legge anche
	 * `ResolveScenarioToRun`**: un flag lasciato sporco qui farebbe fallire i test di
	 * `RTScenarioAutoRunTests.cpp`, che si aspettano «solo la proprieta' -> vale la proprieta'». In una unity
	 * build il test successivo puo' essere qualsiasi cosa, e un test che rompe gli altri e' peggio di un test
	 * assente: manda a cercare il difetto nel posto sbagliato.
	 */
	struct FRTScopedEntryCommandLine
	{
		FString Saved;
		FRTScopedEntryCommandLine() : Saved(FCommandLine::Get()) {}
		~FRTScopedEntryCommandLine() { FCommandLine::Set(*Saved); }
		void SetScenario(const TCHAR* Id) { FCommandLine::Set(*(Saved + FString::Printf(TEXT(" -RTScenario=%s"), Id))); }
		void Clear() { FCommandLine::Set(*Saved); }
	};

	/** Come sopra, per la console variable: stessa ragione, stato che sopravvive al test. */
	struct FRTScopedEntryCVar
	{
		FString Saved;
		FRTScopedEntryCVar() : Saved(CVarRTTestScenario.GetValueOnGameThread()) {}
		~FRTScopedEntryCVar() { CVarRTTestScenario->Set(*Saved, ECVF_SetByCode); }
		void Set(const TCHAR* Value) { CVarRTTestScenario->Set(Value, ECVF_SetByCode); }
	};
}

/**
 * `-RTScenario=` sceglie lo scenario, e SCAVALCA la proprieta' del GameMode.
 *
 * L'invariante che protegge: **esiste un modo di scegliere uno scenario dall'esterno che non passa da una
 * console variable**, perche' in Shipping quella strada non c'e'. Se questo test diventa rosso, la procedura
 * `Packaged` di `#578` torna ineseguibile e non se ne accorge nessuno finche' non si impacchetta.
 *
 * La precedenza sulla proprieta' e' la stessa regola di ogni override di configurazione: la proprieta' dice
 * «questo progetto, per ora»; il flag dice «questo avvio». Il piu' specifico vince.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioCommandLineEntryTest,
	"RefactorTactics.Scenario.CommandLineEntryOverridesProperty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioCommandLineEntryTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("game mode"), GameMode)) { RTWorldFixtures::DestroyWorld(World); return false; }

	FRTScopedEntryCVar CVarGuard;
	FRTScopedEntryCommandLine CmdGuard;
	CVarGuard.Set(TEXT(""));

	// Nessuna delle tre: partita normale. E' la controprova che rende significative le altre — senza, «vale
	// il flag» passerebbe anche con un'implementazione che restituisce sempre qualcosa.
	GameMode->ScenarioToRun.Reset();
	CmdGuard.Clear();
	TestTrue(TEXT("niente proprieta', niente flag, niente console -> partita normale"),
		GameMode->ResolveScenarioToRun().IsEmpty());

	// Solo il flag: decide lui, anche senza proprieta'. E' il caso del pacchetto, dove la proprieta' e' vuota.
	CmdGuard.SetScenario(TEXT("Movement.Collision"));
	TestEqual(TEXT("solo il flag -> vale il flag"),
		GameMode->ResolveScenarioToRun(), FString(TEXT("Movement.Collision")));

	// Flag e proprieta' su scenari DIVERSI: vince il flag, e non in silenzio.
	GameMode->ScenarioToRun = TEXT("Movement.Basic");
	AddExpectedError(TEXT("-RTScenario=.*SCAVALCA la proprieta'"), EAutomationExpectedErrorFlags::Contains, 1);
	TestEqual(TEXT("flag + proprieta' -> vince il flag"),
		GameMode->ResolveScenarioToRun(), FString(TEXT("Movement.Collision")));

	// Flag e proprieta' UGUALI: niente da segnalare. Un avviso che compare sempre e' rumore, e il rumore si
	// impara a ignorare — e' la stessa cura gia' presa per la console.
	GameMode->ScenarioToRun = TEXT("Movement.Collision");
	TestEqual(TEXT("flag e proprieta' uguali -> nessun conflitto da segnalare"),
		GameMode->ResolveScenarioToRun(), FString(TEXT("Movement.Collision")));

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * La console PREVALE sul flag di riga di comando.
 *
 * E' la regola del piu' specifico applicata al **tempo**: la console si puo' digitare a meta' sessione,
 * quindi deve poter scavalcare cio' che l'avvio aveva chiesto. Se fosse il contrario, lanciare l'editor con
 * `-RTScenario=` renderebbe impossibile cambiare scenario senza riavviare — e in editor si cambia scenario
 * dieci volte di seguito.
 *
 * ⚠️ In Shipping questo test non descrive niente di raggiungibile: li' la console non arriva, e l'ordine fra
 * le due sorgenti non e' osservabile. E' un invariante dell'**editor**, ed e' giusto che lo sia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioConsoleBeatsCommandLineTest,
	"RefactorTactics.Scenario.ConsoleOverridesCommandLineEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioConsoleBeatsCommandLineTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("game mode"), GameMode)) { RTWorldFixtures::DestroyWorld(World); return false; }

	FRTScopedEntryCVar CVarGuard;
	FRTScopedEntryCommandLine CmdGuard;

	GameMode->ScenarioToRun.Reset();
	CmdGuard.SetScenario(TEXT("Movement.Basic"));
	CVarGuard.Set(TEXT("Movement.Collision"));

	TestEqual(TEXT("console + flag -> vince la console"),
		GameMode->ResolveScenarioToRun(), FString(TEXT("Movement.Collision")));

	// Tolta la console, il flag torna a valere: la precedenza non lo ha consumato.
	CVarGuard.Set(TEXT(""));
	TestEqual(TEXT("tolta la console, vale di nuovo il flag"),
		GameMode->ResolveScenarioToRun(), FString(TEXT("Movement.Basic")));

	// E la banda a schermo attribuisce la scelta alla sorgente giusta: e' l'unico punto in cui l'utente vede
	// da dove viene lo scenario, e una banda che nomina la fonte sbagliata e' peggio di una banda assente.
	const FString Banda = GameMode->GetScenarioBannerText();
	TestTrue(TEXT("la banda nomina il flag"), Banda.Contains(TEXT("-RTScenario=")));
	TestFalse(TEXT("e non attribuisce al BP_GameMode"), Banda.Contains(TEXT("BP_GameMode")));

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
