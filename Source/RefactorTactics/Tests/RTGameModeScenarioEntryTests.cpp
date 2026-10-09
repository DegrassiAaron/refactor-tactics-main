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
#include "Misc/ScopeExit.h"          // `#3267`: la CVar si ripristina, sempre
#include "Misc/FileHelper.h"         // `#3267`: il gate sul RAMO legge il sorgente
#include "Misc/Paths.h"
#include "HAL/IConsoleManager.h"
#include "Turn/RTTurnManager.h"      // `#3267`: ArePlaybackControlsEnabled

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

// =========================================================================================================
// `#3267` — L'ACCENSIONE DEI CONTROLLI NEL RAMO DELLO SCENARIO.
//
// 🔴 Trovata in seduta PIE provando a usare `K` per fermare una scena del corpus, e non ottenendo nulla.
// `rt.Debug.PlaybackControls 1` non aveva **alcun effetto** in auto-run: `BeginPlay` legge le due CVar
// dopo `SetupHexMatch`, e il ramo `ERTScenarioStart::Started` esce con un `return` cinquanta righe piu' su.
//
// ⚠️ **I test esistenti erano verdi e non potevano vederlo**:
// `Playback.ControlsAreReachableFromTheController` e le sue sorelle provano che i comandi facciano il loro
// lavoro **una volta accesi**. Nessuno provava che l'accensione avvenisse in quel ramo — e non e' una
// svista dei test, e' che quel ramo non li chiamava.
// =========================================================================================================

/**
 * La funzione estratta accende davvero, secondo la CVar — `#3267`.
 *
 * ⚠️ **Non e' il gate del ramo**: e' la meta' che rende il gemello leggibile. Senza, un `ramo che chiama una
 * funzione vuota` passerebbe il controllo statico e non accenderebbe niente.
 *
 * 🔑 **La CVar si ripristina**, ed e' la regola che l'intestazione di questo file dichiara: *«un test che
 * rompe gli altri e' peggio di un test assente»*. `rt.Debug.PlaybackControls` sopravvive al test, e
 * lasciarla accesa cambierebbe il comportamento di ogni playback misurato dopo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackControlCVarsTurnOnTheControlsTest,
	"RefactorTactics.Playback.ControlCVarsTurnOnTheControls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackControlCVarsTurnOnTheControlsTest::RunTest(const FString&)
{
	IConsoleVariable* Controlli = IConsoleManager::Get().FindConsoleVariable(TEXT("rt.Debug.PlaybackControls"));
	if (!TestNotNull(TEXT("⛔ premessa: la CVar esiste"), Controlli)) { return false; }

	const int32 Prima = Controlli->GetInt();
	ON_SCOPE_EXIT{ Controlli->Set(Prima, ECVF_SetByCode); };

	// ⚠️ **`NewObject`, non uno spawn, e non e' una scorciatoia** (#2182). Ne' `ApplyPlaybackControlCVars`
	// ne' i due setter che chiama toccano il mondo: leggono due CVar e scrivono campi membro. Cio' che
	// serve a questa domanda e' un'ISTANZA, non una partita — ed e' la stessa forma che la fetta 2 ha
	// adottato per l'anteprima di pianificazione e la fetta 8 per la geometria della HUD.
	//
	// 🔑 **E non si perde `BeginPlay`, perche' non c'era.** I mondi di prova nati da `UWorld::CreateWorld`
	// non lo fanno partire — e' la stessa constatazione che `RTUnit.h:818` e `RTUnit.cpp:33` hanno gia'
	// pagato — quindi anche la versione precedente interrogava un attore il cui `BeginPlay` non era mai
	// girato. Qui non sparisce una fase: sparisce un mondo che non ne eseguiva nessuna.
	//
	// ⌫ **E questa riga NOMINAVA la chiamata che il file non fa piu'.** La metrica di #2182 cerca quel
	// nome nel testo, non nel codice: scriverlo in un commento ha rimesso il file nel conteggio e ha
	// annullato la fetta, in silenzio. La regola e' **scrivere la proprieta', non il nome** — «la versione
	// precedente», non il nome della chiamata assente.
	//
	// ⛔ **Che il test conservi i denti e' stato MISURATO, non dedotto.** Un'istanza non registrata
	// potrebbe ignorare gli effetti, e allora questo test passerebbe a vuoto. Due mutazioni su
	// `ARTGameMode::ApplyPlaybackControlCVars`, entrambe uccise:
	//
	//   togliere `SetPlaybackControlsEnabled(true)`          → cade «con la CVar a 1 ... ACCESI»
	//   rendere l'accensione incondizionata (`if (true)`)    → cade «con la CVar a 0 restano spenti»
	//
	// 🔑 Servono **entrambe**, e la ragione riguarda la FUNZIONE, non lo stato di nascita dell'oggetto:
	// la prima prova che l'effetto arriva, la seconda che non arriva **sempre**. Una funzione che
	// accendesse incondizionatamente passerebbe la prima.
	//
	// ⌫ **Qui c'era una frase sbagliata, e la correzione dice qualcosa.** Diceva che senza la seconda
	// «un `NewObject` che nascesse con i controlli gia' accesi sarebbe indistinguibile». Non e' vero:
	// quel caso lo intercetta la PREMESSA qui sotto, che asserisce «partono SPENTI» **prima** di
	// chiamare la funzione e esce con `return false`. Attribuire a un'asserzione la protezione che
	// appartiene a un'altra e' il modo in cui una giustificazione sembra solida e non lo e'.
	// Trovato dalla revisione della PR #3350.
	ARTGameMode* GM = NewObject<ARTGameMode>();
	ARTTurnManager* TM = NewObject<ARTTurnManager>();
	if (!TestNotNull(TEXT("GameMode"), GM) || !TestNotNull(TEXT("TurnManager"), TM)) { return false; }

	// ⛔ PREMESSA: si parte da spenti. Senza, un `true` finale non direbbe che l'accensione e' avvenuta.
	if (!TestFalse(TEXT("⛔ premessa: i controlli partono SPENTI"), TM->ArePlaybackControlsEnabled()))
	{
		return false;
	}

	Controlli->Set(1, ECVF_SetByCode);
	GM->ApplyPlaybackControlCVars(TM);
	TestTrue(TEXT("🔴 con la CVar a 1 i controlli sono ACCESI"), TM->ArePlaybackControlsEnabled());

	// ⛔ E il verso opposto: a zero non si accende nulla. Un'accensione incondizionata passerebbe la riga
	// sopra e sarebbe il difetto opposto — comandi vivi in una sessione che non li ha chiesti.
	ARTTurnManager* Secondo = NewObject<ARTTurnManager>();
	if (TestNotNull(TEXT("secondo TurnManager"), Secondo))
	{
		Controlli->Set(0, ECVF_SetByCode);
		GM->ApplyPlaybackControlCVars(Secondo);
		TestFalse(TEXT("⛔ con la CVar a 0 restano spenti"), Secondo->ArePlaybackControlsEnabled());
	}

	// ⚠️ Un `TurnManager` nullo non e' un errore: non c'e' nessuno da accendere.
	GM->ApplyPlaybackControlCVars(nullptr);

	return true;
}

/**
 * E il ramo dell'auto-run la CHIAMA, prima di aprire il primo turno — `#3267`.
 *
 * 🔴 **E' il gate che la issue chiede: pinna il RAMO, non il comportamento dei comandi.** Il difetto non
 * era che i comandi non funzionassero — funzionavano — ma che quel percorso non li accendesse mai.
 *
 * ⚠️ **E' un controllo sul SORGENTE, e va saputo prima di fidarsene.** Far correre `BeginPlay` headless
 * richiederebbe una mappa caricata, uno scenario reale e il coordinatore: la via che
 * [[build-e-test-unreal]] misura come non percorribile (`-RTScenario` apre il livello di bootstrap e resta
 * li'). Cio' che si puo' verificare senza l'Editor e' che la chiamata **esista nel ramo**, e dove.
 *
 * 🔑 **L'ordine e' parte dell'asserzione, non un di piu'**: l'accensione deve precedere
 * `OpenClaimedFirstTurn`, o il primo playback — proprio quello che chi lancia con `PlaybackStartPaused`
 * vuole guardare fermo — scorrerebbe con i comandi ancora spenti. E' la stessa ragione che il percorso
 * normale dichiara per se'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioBranchAppliesPlaybackControlsTest,
	"RefactorTactics.Playback.ScenarioBranchAppliesPlaybackControlCVars",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioBranchAppliesPlaybackControlsTest::RunTest(const FString&)
{
	const FString Percorso = FPaths::Combine(FPaths::ProjectDir(),
		TEXT("Source/RefactorTactics/RTGameMode.cpp"));
	FString Testo;
	if (!TestTrue(TEXT("⛔ premessa: il sorgente del GameMode si legge"),
		FFileHelper::LoadFileToString(Testo, *Percorso)))
	{
		return false;
	}

	// Il ramo dell'auto-run: dal `case` fino al primo `return`, che e' la sua uscita.
	const int32 Inizio = Testo.Find(TEXT("case ERTScenarioStart::Started:"));
	if (!TestTrue(TEXT("⛔ premessa: il ramo `Started` esiste"), Inizio != INDEX_NONE)) { return false; }
	const int32 Fine = Testo.Find(TEXT("return;"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Inizio);
	if (!TestTrue(TEXT("⛔ premessa: il ramo esce con un `return`"), Fine != INDEX_NONE)) { return false; }

	const FString Ramo = Testo.Mid(Inizio, Fine - Inizio);

	// --- IL FATTO ------------------------------------------------------------------------------------
	//
	// ⛔ **Si cercano le CHIAMATE, con la parentesi, non i nomi.** La prima stesura cercava i soli
	// identificatori e misurava il **commento**: la riga che spiega *«prima di `OpenClaimedFirstTurn`»*
	// precede la chiamata, quindi l'ordine risultava invertito e il gate era rosso su codice corretto. Un
	// controllo sul sorgente che non distingue il codice dalla prosa misura la prosa.
	const int32 Accensione = Ramo.Find(TEXT("ApplyPlaybackControlCVars("));
	TestTrue(TEXT("🔴 il ramo dell'auto-run accende i controlli di playback"), Accensione != INDEX_NONE);

	// 🔑 E PRIMA di aprire il primo turno.
	const int32 Apertura = Ramo.Find(TEXT("OpenClaimedFirstTurn();"));
	if (TestTrue(TEXT("⛔ premessa: il ramo apre il primo turno"), Apertura != INDEX_NONE)
		&& Accensione != INDEX_NONE)
	{
		TestTrue(TEXT("🔴 e lo fa PRIMA di aprire il turno 1: dopo, il primo playback scorrerebbe spento"),
			Accensione < Apertura);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
