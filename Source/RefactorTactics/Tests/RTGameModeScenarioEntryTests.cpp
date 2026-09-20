// LA PORTA D'INGRESSO allo scenario da riga di comando: `-RTScenario=<Id>`, e il suo posto nella precedenza.
//
// Esiste per una ragione misurata, non per simmetria con la console: in una build **Shipping**
// `-dpcvars=rt.Test.Scenario=...` non arriva. In `DeviceProfileManager.cpp` tutto il parsing di
// `-dpcvars=`/`-dpcvar=`/`-forcedpcvars=` sta dentro `#if !UE_BUILD_SHIPPING`, quindi la variabile non viene
// mai impostata, il GameMode legge vuoto e allestisce la partita normale — senza un errore che lo dica,
// perche' in Shipping anche il logging e' compilato fuori. Trovato eseguendo un pacchetto vero (`#926`).
//
// ⚠️ QUESTI TEST NON DUPLICANO `RTScenarioAutoRunTests.cpp`. Quel file copre il CABLAGGIO — che la banda a
// schermo attribuisca la scelta alla fonte giusta, in un mondo vero, per tutte e tre le sorgenti — mentre
// qui si copre la REGOLA: chi vince, e come si legge il flag.
//
// ---
//
// 🔑 **Nessun mondo, e nessuno stato globale toccato** (`#2182`). Fino al 2026-09-20 questi due test
// montavano un `UWorld`, spawnavano un `ARTGameMode` e mutavano **due** stati che sopravvivono al test — la
// console variable `rt.Test.Scenario` e la riga di comando del processo — per porre una domanda che non ha
// bisogno di nessuno dei tre. Servivano due guardie di ripristino, e il loro commento diceva perche': «un
// test che rompe gli altri e' peggio di un test assente».
//
// La precedenza e' ora `ARTGameMode::ChooseScenarioEntry`, che prende i tre valori gia' letti, e il parsing
// del flag e' `ARTGameMode::ReadScenarioFromCommandLine`, che prende la riga come parametro. ⛔ **La regola
// non si e' spostata di sede**: vive ancora in `RTGameMode.cpp` accanto alle sue due sorelle — sorgente
// mappa e autobattle — come `RTMatchBootstrapper.h` prescrive. E' cambiato solo da dove arrivano gli
// ingressi.
//
// ∴ e' precisamente lo scopo che `FRTMatchBootstrapConfig` dichiara per se': «un test puo' allestire una
// partita **senza toccare lo stato globale del processo**, che con le console variable non e' possibile».

#include "Misc/AutomationTest.h"
#include "RTGameMode.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nomi distinti da ogni altro file di test: la unity build condivide la translation unit.
	using FRTEntryChoiceUnderTest = ARTGameMode::FScenarioEntryChoice;
	using ERTEntrySourceUnderTest = ARTGameMode::EScenarioEntrySource;
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
	// Nessuna delle tre: partita normale. E' la controprova che rende significative le altre — senza, «vale
	// il flag» passerebbe anche con un'implementazione che restituisce sempre qualcosa.
	{
		const FRTEntryChoiceUnderTest Niente =
			ARTGameMode::ChooseScenarioEntry(FString(), FString(), FString());
		TestTrue(TEXT("niente proprieta', niente flag, niente console -> partita normale"),
			Niente.ScenarioId.IsEmpty());
		TestEqual(TEXT("e la fonte e' la proprieta', che e' l'ultimo gradino"),
			Niente.Source, ERTEntrySourceUnderTest::Property);
	}

	// Solo il flag: decide lui, anche senza proprieta'. E' il caso del pacchetto, dove la proprieta' e' vuota.
	{
		const FRTEntryChoiceUnderTest SoloFlag =
			ARTGameMode::ChooseScenarioEntry(FString(), TEXT("Movement.Collision"), FString());
		TestEqual(TEXT("solo il flag -> vale il flag"),
			SoloFlag.ScenarioId, FString(TEXT("Movement.Collision")));
		TestEqual(TEXT("e la fonte e' la riga di comando"),
			SoloFlag.Source, ERTEntrySourceUnderTest::CommandLine);
		TestTrue(TEXT("senza proprieta' non c'e' nessun conflitto da segnalare"),
			SoloFlag.OverrideWarning.IsEmpty());
	}

	// Flag e proprieta' su scenari DIVERSI: vince il flag, e non in silenzio.
	{
		const FRTEntryChoiceUnderTest Conflitto =
			ARTGameMode::ChooseScenarioEntry(TEXT("Movement.Basic"), TEXT("Movement.Collision"), FString());
		TestEqual(TEXT("flag + proprieta' -> vince il flag"),
			Conflitto.ScenarioId, FString(TEXT("Movement.Collision")));

		// 🔑 L'avviso si LEGGE invece di intercettarlo con `AddExpectedError`: prima l'unico modo di
		// verificarlo era catturare una riga di log, cioe' misurare la presentazione di un fatto invece del
		// fatto. Ora la frase e' un valore, e il test puo' chiedere che nomini le due cose che servono a chi
		// legge: cosa ha vinto, e cosa togliere per tornare indietro.
		TestFalse(TEXT("l'override non e' silenzioso"), Conflitto.OverrideWarning.IsEmpty());
		TestTrue(TEXT("l'avviso nomina il flag, non la console"),
			Conflitto.OverrideWarning.Contains(TEXT("-RTScenario=")));
		TestTrue(TEXT("e nomina la proprieta' scavalcata"),
			Conflitto.OverrideWarning.Contains(TEXT("Movement.Basic")));
	}

	// Flag e proprieta' UGUALI: niente da segnalare. Un avviso che compare sempre e' rumore, e il rumore si
	// impara a ignorare — e' la stessa cura gia' presa per la console.
	{
		const FRTEntryChoiceUnderTest Uguali = ARTGameMode::ChooseScenarioEntry(
			TEXT("Movement.Collision"), TEXT("Movement.Collision"), FString());
		TestEqual(TEXT("flag e proprieta' uguali -> vale comunque il flag"),
			Uguali.ScenarioId, FString(TEXT("Movement.Collision")));
		TestTrue(TEXT("flag e proprieta' uguali -> nessun conflitto da segnalare"),
			Uguali.OverrideWarning.IsEmpty());
	}

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
	{
		const FRTEntryChoiceUnderTest Entrambe = ARTGameMode::ChooseScenarioEntry(
			FString(), TEXT("Movement.Basic"), TEXT("Movement.Collision"));
		TestEqual(TEXT("console + flag -> vince la console"),
			Entrambe.ScenarioId, FString(TEXT("Movement.Collision")));
		TestEqual(TEXT("e la fonte e' la console"),
			Entrambe.Source, ERTEntrySourceUnderTest::ConsoleVariable);
	}

	// Tolta la console, il flag torna a valere: la precedenza non lo ha consumato.
	{
		const FRTEntryChoiceUnderTest SenzaConsole =
			ARTGameMode::ChooseScenarioEntry(FString(), TEXT("Movement.Basic"), FString());
		TestEqual(TEXT("tolta la console, vale di nuovo il flag"),
			SenzaConsole.ScenarioId, FString(TEXT("Movement.Basic")));
		TestEqual(TEXT("e la fonte torna a essere la riga di comando"),
			SenzaConsole.Source, ERTEntrySourceUnderTest::CommandLine);
	}

	// ⚠️ **La fonte e' la CONSOLE anche quando i tre valori coincidono**, e va asserito: una precedenza
	// implementata «chi e' diverso dalla proprieta'» darebbe la stessa risposta su ogni caso qui sopra e
	// cadrebbe solo su questo. E' la banda a schermo a leggere `Source`, quindi sbagliarlo attribuirebbe la
	// scelta al `BP_GameMode` mentre a decidere e' stata la console.
	{
		const FRTEntryChoiceUnderTest Tutte = ARTGameMode::ChooseScenarioEntry(
			TEXT("Movement.Basic"), TEXT("Movement.Basic"), TEXT("Movement.Basic"));
		TestEqual(TEXT("tre valori uguali -> vince comunque la console"),
			Tutte.Source, ERTEntrySourceUnderTest::ConsoleVariable);
		TestTrue(TEXT("e non c'e' nessun conflitto da segnalare"), Tutte.OverrideWarning.IsEmpty());
	}

	return true;
}

/**
 * Il FLAG si legge da una riga di comando qualunque, e solo quando c'e'.
 *
 * 🔑 **Prima questa domanda non era ponibile senza riscrivere la riga di comando del PROCESSO**, che dura
 * quanto il processo e vale per ogni test successivo della unity build. Prendendola come parametro si
 * possono provare gli ingressi che contano — assente, presente fra altri flag, riga nulla — invece del solo
 * caso che si riusciva ad allestire.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioCommandLineParsingTest,
	"RefactorTactics.Scenario.CommandLineFlagIsReadFromTheLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioCommandLineParsingTest::RunTest(const FString&)
{
	TestTrue(TEXT("riga vuota -> nessuno scenario"),
		ARTGameMode::ReadScenarioFromCommandLine(TEXT("")).IsEmpty());
	TestTrue(TEXT("riga senza il flag -> nessuno scenario"),
		ARTGameMode::ReadScenarioFromCommandLine(TEXT("-nullrhi -unattended")).IsEmpty());

	TestEqual(TEXT("il flag da solo"),
		ARTGameMode::ReadScenarioFromCommandLine(TEXT("-RTScenario=Movement.Basic")),
		FString(TEXT("Movement.Basic")));

	// Il caso reale: il flag arriva in mezzo agli altri, non da solo.
	TestEqual(TEXT("il flag in mezzo ad altri"),
		ARTGameMode::ReadScenarioFromCommandLine(
			TEXT("Progetto.uproject -nullrhi -RTScenario=Spec.Map.InteractOpensDoor -unattended")),
		FString(TEXT("Spec.Map.InteractOpensDoor")));

	// ⛔ **`nullptr` non e' una riga vuota, ed e' il caso che un test non poteva allestire finche' la riga
	// arrivava da `FCommandLine::Get()`**: quella non e' mai nulla. Ora la funzione e' chiamabile da chiunque,
	// e il ramo di guardia ha un testimone.
	TestTrue(TEXT("riga nulla -> nessuno scenario, senza schiantare"),
		ARTGameMode::ReadScenarioFromCommandLine(nullptr).IsEmpty());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
