#include "Misc/AutomationTest.h"
#include "Tests/RTWorldFixtures.h"
#include "Replay/RTMatchHistoryLibrary.h"
#include "Replay/RTReplayManifest.h"
#include "Replay/RTReplayPlayerLibrary.h"
#include "Replay/RTReplayRecorderLibrary.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/RTTestResult.h"
#include "ScenarioHarness/RTTestScenario.h"
#include "Turn/RTMatchFormatLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLogLibrary.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Misc/StringOutputDevice.h"
#include "Engine/Engine.h"

// La guardia: il codice di test non deve finire nel binario Shipping.
#if WITH_DEV_AUTOMATION_TESTS

/**
 * Il PONTE scenario -> archivio replay (`#3035`, fetta headless).
 *
 * Prima di questa fetta uno scenario accumulava le tracce in `FRTTestResult::TurnTraces` e il Replay Player apriva
 * archivi su disco: i due pezzi esistevano e non si parlavano. Qui si verifica il collegamento, non i pezzi.
 *
 * Tutto cio' che un test crea sta in `AutomationTransientDir` e viene tolto a fine test, **anche su asserto
 * fallito**: la rimozione sta nel distruttore di una guardia, non in coda al corpo.
 */
namespace
{
	// ⚠️ Nomi prefissati `ScenarioBridge`: la unity build fonde le unita' di traduzione, e due helper omonimi in
	// namespace anonimi diversi diventano una ridefinizione che compare e sparisce col raggruppamento.

	/** Una cartella di prova: si garantisce INESISTENTE all'ingresso e si toglie all'uscita, comunque finisca il test. */
	struct FScenarioBridgeRoot
	{
		FString Path;

		explicit FScenarioBridgeRoot(const TCHAR* Name)
			: Path(FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ScenarioBridge"), Name))
		{
			Remove();
		}

		~FScenarioBridgeRoot() { Remove(); }

		bool Exists() const { return IFileManager::Get().DirectoryExists(*Path); }

		void Remove() const
		{
			IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
			if (PF.DirectoryExists(*Path)) { PF.DeleteDirectoryRecursively(*Path); }

			// E la cartella che le contiene tutte, quando resta vuota: `DeleteDirectory` non e' ricorsiva e fallisce, senza
			// danno, finche' un'altra guardia dello stesso test ne tiene una viva. L'ultima a sparire porta via anche lei.
			PF.DeleteDirectory(*FPaths::GetPath(Path));
		}

		/** Le sottocartelle dirette: ogni archivio vive in una cartella che porta il suo `MatchId`. */
		TArray<FString> MatchDirectories() const
		{
			TArray<FString> Dirs;
			IFileManager::Get().FindFiles(Dirs, *FPaths::Combine(Path, TEXT("*")), /*Files=*/ false, /*Directories=*/ true);
			Dirs.Sort();
			return Dirs;
		}
	};

	bool ScenarioBridgeLoad(FAutomationTestBase& Test, const TCHAR* ScenarioId, FRTTestScenario& OutScenario)
	{
		FString LoadError;
		const FString Path = URTScenarioIndex::ResolvePath(ScenarioId, LoadError);
		if (Path.IsEmpty() || !URTScenarioLoader::LoadFromFile(Path, OutScenario, LoadError))
		{
			Test.AddError(FString::Printf(TEXT("%s non si carica: %s"), ScenarioId, *LoadError));
			return false;
		}
		return true;
	}

	/** Una esecuzione in un mondo nuovo, distrutto subito dopo: nessuno stato sopravvive fra una run e l'altra. */
	FRTTestResult ScenarioBridgeRun(const FRTTestScenario& Scenario, const FString& ReplayRoot)
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!World)
		{
			return FRTTestResult();
		}
		const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario, ReplayRoot);
		RTWorldFixtures::DestroyWorld(World);
		return Result;
	}

	/** Lo showcase deve essere `Pass` perche' le tracce da confrontare siano quelle di una partita giocata. */
	bool ScenarioBridgeExpectPass(FAutomationTestBase& Test, const FRTTestResult& Result)
	{
		if (!Test.TestEqual(TEXT("lo scenario gira fino in fondo"),
			static_cast<int32>(Result.Outcome), static_cast<int32>(ERTTestOutcome::Pass)))
		{
			Test.AddError(FString::Printf(TEXT("esito %s - %s%s"), *Result.OutcomeString(),
				*Result.BlockedReason, *Result.ErrorMessage));
			return false;
		}
		return true;
	}
}

/**
 * 1. Con la registrazione richiesta, lo showcase lascia un archivio che `OpenArchive` apre, con un turno per ogni
 *    turno giocato.
 *
 * ⚠️ La radice e' quella del TEST e non `Saved/Replays`: e' un parametro, e il test lo dimostra scrivendo altrove.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioBridgeOpenableArchiveTest,
	"RefactorTactics.Replay.ScenarioBridge.ShowcaseLeavesAnOpenableArchive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioBridgeOpenableArchiveTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!ScenarioBridgeLoad(*this, TEXT("RT_Showcase_Relay_v01"), Scenario)) { return false; }

	const FScenarioBridgeRoot Root(TEXT("Openable"));
	if (!TestFalse(TEXT("la radice di prova parte inesistente"), Root.Exists())) { return false; }

	const FRTTestResult Result = ScenarioBridgeRun(Scenario, Root.Path);
	if (!ScenarioBridgeExpectPass(*this, Result)) { return false; }

	if (!TestTrue(TEXT("il risultato porta il MatchId della registrazione"), Result.ReplayMatchId.IsValid()))
	{
		return false;
	}

	FRTReplaySession Session;
	const ERTReplayOpenResult Opened = URTReplayPlayerLibrary::OpenArchive(Root.Path, Result.ReplayMatchId, Session);
	if (!TestTrue(TEXT("OpenArchive apre l'archivio dello scenario"), Opened == ERTReplayOpenResult::Opened))
	{
		return false;
	}

	// Il numero atteso viene dalla RUN, non da una costante: lo showcase puo' crescere, il legame no.
	TestTrue(TEXT("lo showcase ha giocato almeno un turno"), Result.TurnsPlayed > 0);
	TestEqual(TEXT("un turno nel manifest per ogni turno giocato"), Session.Manifest.TurnCount, Result.TurnsPlayed);
	TestEqual(TEXT("e una traccia caricata per ognuno"), Session.Traces.Num(), Result.TurnsPlayed);
	TestEqual(TEXT("il manifest e' quello di QUESTA registrazione"), Session.Manifest.MatchId, Result.ReplayMatchId);

	// Il formato e' uno di quelli GIA' registrati (`Skirmish2v2`), non uno inventato per lo scenario.
	TestEqual(TEXT("il formato dichiarato e' un formato registrato"),
		Session.Manifest.FormatId, URTMatchFormatLibrary::Skirmish2v2FormatId);
	return true;
}

/**
 * 2. Le tracce caricate dall'archivio sono, dopo `SerializeTurnLog`, **byte per byte** quelle del risultato dello
 *    STESSO run. E' la frase che rende vera la riga di `FRTTestResult::TurnTraces` («un solo sistema»).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioBridgeTracesMatchTheRunTest,
	"RefactorTactics.Replay.ScenarioBridge.ArchiveTracesAreByteEqualToTheRunTraces",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioBridgeTracesMatchTheRunTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!ScenarioBridgeLoad(*this, TEXT("RT_Showcase_Relay_v01"), Scenario)) { return false; }

	const FScenarioBridgeRoot Root(TEXT("ByteEqual"));
	const FRTTestResult Result = ScenarioBridgeRun(Scenario, Root.Path);
	if (!ScenarioBridgeExpectPass(*this, Result)) { return false; }
	if (!TestTrue(TEXT("c'e' un MatchId"), Result.ReplayMatchId.IsValid())) { return false; }

	FRTReplaySession Session;
	if (!TestTrue(TEXT("l'archivio si apre"),
		URTReplayPlayerLibrary::OpenArchive(Root.Path, Result.ReplayMatchId, Session) == ERTReplayOpenResult::Opened))
	{
		return false;
	}
	if (!TestEqual(TEXT("tante tracce quante le tracce del run"), Session.Traces.Num(), Result.TurnTraces.Num()))
	{
		return false;
	}
	TestTrue(TEXT("il run ha prodotto tracce"), Result.TurnTraces.Num() > 0);

	for (int32 Turn = 0; Turn < Result.TurnTraces.Num(); ++Turn)
	{
		// ⚠️ Anti-vacuita' turno per turno: due tracce vuote sarebbero uguali per il motivo sbagliato.
		TestTrue(FString::Printf(TEXT("il turno %d ha voci nell'archivio"), Turn + 1), Session.Traces[Turn].Num() > 0);

		// Stessa topologia e stesso `FormatId` assente di `Session.cpp`: il confronto e' sulle VOCI, non sull'intestazione
		// (quella dell'archivio porta il formato, quella del run no, e non e' un difetto).
		const TArray<uint8> Riserializzata =
			URTTurnLogLibrary::SerializeTurnLog(Session.Traces[Turn], ERTLogTopology::Hex);
		TestTrue(FString::Printf(TEXT("il turno %d e' byte-uguale a quello del run"), Turn + 1),
			Riserializzata == Result.TurnTraces[Turn].Bytes);
	}
	return true;
}

/**
 * 3. Registrare **non cambia la partita**: `StateHash` e tracce dello showcase sono identici con la registrazione
 *    accesa e spenta. Estende `Replay.Producer.RecordingDoesNotChangeTheMatch` al percorso dello scenario.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioBridgeRecordingDoesNotChangeTheRunTest,
	"RefactorTactics.Replay.ScenarioBridge.RecordingDoesNotChangeTheScenarioResult",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioBridgeRecordingDoesNotChangeTheRunTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!ScenarioBridgeLoad(*this, TEXT("RT_Showcase_Relay_v01"), Scenario)) { return false; }

	const FScenarioBridgeRoot Root(TEXT("NoEffect"));

	const FRTTestResult Spenta = ScenarioBridgeRun(Scenario, FString());
	const FRTTestResult Accesa = ScenarioBridgeRun(Scenario, Root.Path);
	if (!ScenarioBridgeExpectPass(*this, Spenta) || !ScenarioBridgeExpectPass(*this, Accesa)) { return false; }

	// Il controllo positivo: senza, due run identiche sarebbero uguali anche se la registrazione non fosse mai partita.
	if (!TestFalse(TEXT("spenta: nessun MatchId"), Spenta.ReplayMatchId.IsValid())) { return false; }
	if (!TestTrue(TEXT("accesa: la registrazione e' davvero partita"), Accesa.ReplayMatchId.IsValid())) { return false; }

	TestTrue(TEXT("lo StateHash esiste (non e' il numero finto di uno scenario non eseguito)"), Spenta.StateHash != 0);
	TestEqual(TEXT("StateHash identico con la registrazione accesa e spenta"), Accesa.StateHash, Spenta.StateHash);
	TestEqual(TEXT("stesso numero di turni"), Accesa.TurnsPlayed, Spenta.TurnsPlayed);
	if (TestEqual(TEXT("stesso numero di tracce"), Accesa.TurnTraces.Num(), Spenta.TurnTraces.Num()))
	{
		for (int32 Turn = 0; Turn < Spenta.TurnTraces.Num(); ++Turn)
		{
			TestTrue(FString::Printf(TEXT("la traccia del turno %d non cambia"), Turn + 1),
				Accesa.TurnTraces[Turn].Bytes == Spenta.TurnTraces[Turn].Bytes);
		}
	}
	TestEqual(TEXT("stesse assertion valutate"), Accesa.Assertions.Num(), Spenta.Assertions.Num());
	return true;
}

/**
 * 4. Senza richiesta esplicita **nessun archivio**: ne' un MatchId, ne' una cartella. Il default e' spento e la radice
 *    non ha un valore "di comodo" (`Saved/Replays`).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioBridgeOffByDefaultTest,
	"RefactorTactics.Replay.ScenarioBridge.NothingIsWrittenUnlessAsked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioBridgeOffByDefaultTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!ScenarioBridgeLoad(*this, TEXT("RT_Showcase_Relay_v01"), Scenario)) { return false; }

	const FScenarioBridgeRoot Root(TEXT("NotAsked"));

	// La forma a un argomento e quella con la radice vuota sono la STESSA richiesta mancata.
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	const FRTTestResult SenzaArgomento = URTScenarioRunner::Run(World, Scenario);
	RTWorldFixtures::DestroyWorld(World);

	const FRTTestResult RadiceVuota = ScenarioBridgeRun(Scenario, FString());

	for (const FRTTestResult* Result : { &SenzaArgomento, &RadiceVuota })
	{
		if (!ScenarioBridgeExpectPass(*this, *Result)) { return false; }
		TestFalse(TEXT("nessun MatchId: la registrazione non e' partita"), Result->ReplayMatchId.IsValid());
	}
	TestFalse(TEXT("e la radice di prova non e' stata creata"), Root.Exists());
	return true;
}

/**
 * 5. `MatchId` non entra in `StateHash` ne' nel digest delle tracce: due run con MatchId **diversi** (generati a ogni
 *    registrazione) danno gli stessi hash. Estende `Replay.Producer.MatchIdDoesNotEnterHashes`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioBridgeMatchIdOutsideHashesTest,
	"RefactorTactics.Replay.ScenarioBridge.MatchIdDoesNotEnterTheScenarioHashes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioBridgeMatchIdOutsideHashesTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!ScenarioBridgeLoad(*this, TEXT("RT_Showcase_Relay_v01"), Scenario)) { return false; }

	const FScenarioBridgeRoot RootA(TEXT("MatchIdA"));
	const FScenarioBridgeRoot RootB(TEXT("MatchIdB"));

	const FRTTestResult A = ScenarioBridgeRun(Scenario, RootA.Path);
	const FRTTestResult B = ScenarioBridgeRun(Scenario, RootB.Path);
	if (!ScenarioBridgeExpectPass(*this, A) || !ScenarioBridgeExpectPass(*this, B)) { return false; }

	if (!TestTrue(TEXT("entrambe le run hanno un MatchId"), A.ReplayMatchId.IsValid() && B.ReplayMatchId.IsValid()))
	{
		return false;
	}
	// Senza questo le due run potrebbero avere lo STESSO id e l'uguaglianza degli hash non direbbe nulla.
	TestNotEqual(TEXT("i due MatchId sono diversi"), A.ReplayMatchId, B.ReplayMatchId);

	TestTrue(TEXT("lo StateHash esiste"), A.StateHash != 0);
	TestEqual(TEXT("StateHash uguale nonostante MatchId diversi"), B.StateHash, A.StateHash);
	if (TestEqual(TEXT("stesso numero di tracce"), B.TurnTraces.Num(), A.TurnTraces.Num()))
	{
		for (int32 Turn = 0; Turn < A.TurnTraces.Num(); ++Turn)
		{
			TestTrue(FString::Printf(TEXT("la traccia del turno %d non porta il MatchId"), Turn + 1),
				B.TurnTraces[Turn].Bytes == A.TurnTraces[Turn].Bytes);
		}
	}
	return true;
}

/**
 * 6. Varianti e ripetizioni giocano piu' partite: un solo `MatchId` non le descriverebbe, e registrarne una sola
 *    nasconderebbe le altre. La richiesta si **rifiuta** dichiarandolo, e non scrive niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioBridgeRefusesRepeatsTest,
	"RefactorTactics.Replay.ScenarioBridge.RefusesToRecordRepeatedRuns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioBridgeRefusesRepeatsTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!ScenarioBridgeLoad(*this, TEXT("RT_Showcase_Relay_v01"), Scenario)) { return false; }
	Scenario.RepeatCount = 2;

	const FScenarioBridgeRoot Root(TEXT("Repeated"));
	const FRTTestResult Result = ScenarioBridgeRun(Scenario, Root.Path);

	TestEqual(TEXT("la richiesta e' un errore dichiarato"),
		static_cast<int32>(Result.Outcome), static_cast<int32>(ERTTestOutcome::Error));
	TestTrue(TEXT("e il motivo nomina la registrazione"), Result.ErrorMessage.Contains(TEXT("registra")));
	TestFalse(TEXT("nessun MatchId"), Result.ReplayMatchId.IsValid());
	TestFalse(TEXT("nessuna cartella scritta"), Root.Exists());
	return true;
}

/**
 * 7. Un archivio smette di crescere quando lo scenario finisce.
 *
 * `RunSingle` lascia il turn manager nel mondo (`bTearDownAfter = false`) e uno scenario successivo nello stesso mondo
 * lo RIUSA - e' il caso della console in una seduta PIE. Se la registrazione restasse accesa, i turni del secondo
 * scenario finirebbero in coda all'archivio del primo, che dichiarerebbe una partita che non e' mai stata giocata
 * cosi'. Una run che non ha chiesto l'archivio non deve toccarne uno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioBridgeArchiveStopsGrowingTest,
	"RefactorTactics.Replay.ScenarioBridge.AnArchiveStopsGrowingWhenTheScenarioEnds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioBridgeArchiveStopsGrowingTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!ScenarioBridgeLoad(*this, TEXT("Movement.Basic"), Scenario)) { return false; }

	const FScenarioBridgeRoot Root(TEXT("StopsGrowing"));

	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	const FRTTestResult Prima = URTScenarioRunner::Run(World, Scenario, Root.Path);
	const FRTTestResult Dopo = URTScenarioRunner::Run(World, Scenario); // stesso mondo, stesso turn manager, nessuna richiesta
	RTWorldFixtures::DestroyWorld(World);

	if (!ScenarioBridgeExpectPass(*this, Prima) || !ScenarioBridgeExpectPass(*this, Dopo)) { return false; }
	if (!TestTrue(TEXT("la prima run ha registrato"), Prima.ReplayMatchId.IsValid())) { return false; }
	TestFalse(TEXT("la seconda non l'ha chiesto"), Dopo.ReplayMatchId.IsValid());
	TestTrue(TEXT("e ha giocato dei turni (altrimenti il confronto sotto non direbbe nulla)"), Dopo.TurnsPlayed > 0);

	FRTReplaySession Session;
	if (!TestTrue(TEXT("l'archivio si apre"),
		URTReplayPlayerLibrary::OpenArchive(Root.Path, Prima.ReplayMatchId, Session) == ERTReplayOpenResult::Opened))
	{
		return false;
	}
	TestEqual(TEXT("l'archivio ha i turni della PRIMA run e nessuno della seconda"),
		Session.Manifest.TurnCount, Prima.TurnsPlayed);
	return true;
}

/**
 * 8. Il CHIAMANTE REALE: il comando console `rt.Test.Run <Id> -ReplayRoot=<dir>`.
 *
 * ⛔ Un campo letto solo dal proprio test e' il difetto ricorrente. Qui si passa dalla console vera, con il parser vero,
 * e si controlla il risultato sul DISCO: la cartella dell'archivio porta il `MatchId`, e `OpenArchive` la apre.
 *
 * `RunById` scrive anche il report in `Saved/RTTests/...` (come ogni test che lo chiama): la cartella della run si legge
 * dalla riga che la console stampa e si toglie a fine test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioBridgeConsoleTest,
	"RefactorTactics.Replay.ScenarioBridge.ConsoleFlagRecordsAndOnlyWhenAsked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioBridgeConsoleTest::RunTest(const FString&)
{
	const FScenarioBridgeRoot Root(TEXT("Console"));
	const FScenarioBridgeRoot RootEmpty(TEXT("ConsoleEmptyValue"));

	TArray<FString> ReportDirs;
	auto Esegui = [&ReportDirs](const FString& Comando) -> FString
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		FStringOutputDevice Out;
		if (World)
		{
			IConsoleManager::Get().ProcessUserConsoleInput(*Comando, Out, World);
			RTWorldFixtures::DestroyWorld(World);
		}
		const FString Testo = Out;

		// Il report che la console ha scritto: va tolto, perche' non e' del test ma lo ha creato il test.
		static const FString Marker = TEXT("report: ");
		int32 Pos = Testo.Find(Marker);
		if (Pos != INDEX_NONE)
		{
			FString Riga = Testo.Mid(Pos + Marker.Len());
			Riga.Split(TEXT("\n"), &Riga, nullptr);
			Riga.TrimStartAndEndInline();
			ReportDirs.Add(FPaths::GetPath(Riga));
		}
		return Testo;
	};

	// Senza il flag: il comando di sempre, nessuna cartella.
	const FString Senza = Esegui(FString::Printf(TEXT("rt.Test.Run Movement.Basic")));
	TestTrue(TEXT("il comando senza flag gira (PASS)"), Senza.Contains(TEXT("PASS")));
	TestFalse(TEXT("e non dichiara nessun archivio"), Senza.Contains(TEXT("MatchId")));

	// Col flag: l'archivio c'e', e si apre.
	const FString Con = Esegui(FString::Printf(TEXT("rt.Test.Run Movement.Basic -ReplayRoot=%s"), *Root.Path));
	TestTrue(TEXT("il comando col flag gira (PASS)"), Con.Contains(TEXT("PASS")));

	const TArray<FString> Archivi = Root.MatchDirectories();
	if (TestEqual(TEXT("esattamente un archivio nella radice richiesta"), Archivi.Num(), 1))
	{
		FGuid MatchId;
		if (TestTrue(TEXT("la cartella porta un MatchId"), FGuid::Parse(Archivi[0], MatchId)))
		{
			FRTReplaySession Session;
			TestTrue(TEXT("OpenArchive apre l'archivio che la console ha lasciato"),
				URTReplayPlayerLibrary::OpenArchive(Root.Path, MatchId, Session) == ERTReplayOpenResult::Opened);
			TestTrue(TEXT("con almeno un turno"), Session.Manifest.TurnCount > 0);
			TestTrue(TEXT("e la console dice QUALE archivio ha scritto"), Con.Contains(Archivi[0]));
		}
	}

	// Un flag senza valore non diventa "radice di default": si rifiuta, e non scrive niente.
	const FString Vuoto = Esegui(TEXT("rt.Test.Run Movement.Basic -ReplayRoot="));
	TestFalse(TEXT("senza valore la radice di default NON si inventa"), RootEmpty.Exists());
	TestFalse(TEXT("e non dichiara nessun archivio"), Vuoto.Contains(TEXT("MatchId")));

	for (const FString& Dir : ReportDirs)
	{
		// Si toglie la cartella della run e, se resta vuota, quella dello scenario: mai altro.
		IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
		if (!Dir.IsEmpty() && PF.DirectoryExists(*Dir))
		{
			PF.DeleteDirectoryRecursively(*Dir);
			PF.DeleteDirectory(*FPaths::GetPath(Dir)); // fallisce, senza danno, se contiene altre run
		}
	}
	return true;
}

/**
 * 9. **Due run registrate di fila nello STESSO mondo** lasciano due archivi distinti, ciascuno col suo numero di turni.
 *
 * E' la seduta PIE: la console due volte, un solo turn manager. Il turn manager sopravvive alla sessione, e se la
 * prima run gli lasciasse l'identita' della registrazione, la seconda verrebbe rifiutata come «sta gia' registrando»
 * (`#3035`, rilievo di review). Le due run hanno un numero di turni diverso, cosi' un archivio che inghiottisse i
 * turni dell'altro avrebbe un `TurnCount` sbagliato invece di coincidere per caso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioBridgeTwoRecordedRunsTest,
	"RefactorTactics.Replay.ScenarioBridge.TwoRecordedRunsInOneWorldLeaveTwoArchives",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioBridgeTwoRecordedRunsTest::RunTest(const FString&)
{
	FRTTestScenario Breve;
	FRTTestScenario Showcase;
	if (!ScenarioBridgeLoad(*this, TEXT("Movement.Basic"), Breve)) { return false; }
	if (!ScenarioBridgeLoad(*this, TEXT("RT_Showcase_Relay_v01"), Showcase)) { return false; }

	const FScenarioBridgeRoot Root(TEXT("TwoRuns"));

	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	const FRTTestResult Prima = URTScenarioRunner::Run(World, Breve, Root.Path);
	const FRTTestResult Seconda = URTScenarioRunner::Run(World, Showcase, Root.Path); // stesso mondo, stesso turn manager
	RTWorldFixtures::DestroyWorld(World);

	if (!ScenarioBridgeExpectPass(*this, Prima) || !ScenarioBridgeExpectPass(*this, Seconda)) { return false; }
	if (!TestTrue(TEXT("la prima run ha registrato"), Prima.ReplayMatchId.IsValid())) { return false; }
	if (!TestTrue(TEXT("e anche la seconda: non e' rifiutata come «sta gia' registrando»"),
		Seconda.ReplayMatchId.IsValid()))
	{
		return false;
	}
	TestNotEqual(TEXT("con due identita' diverse"), Prima.ReplayMatchId, Seconda.ReplayMatchId);
	TestNotEqual(TEXT("e due numeri di turni diversi (altrimenti il confronto sotto non distingue)"),
		Prima.TurnsPlayed, Seconda.TurnsPlayed);
	TestEqual(TEXT("due cartelle di archivio nella radice"), Root.MatchDirectories().Num(), 2);

	const TPair<const FRTTestResult*, const TCHAR*> Run[] = { { &Prima, TEXT("prima") }, { &Seconda, TEXT("seconda") } };
	for (const TPair<const FRTTestResult*, const TCHAR*>& R : Run)
	{
		FRTReplaySession Session;
		if (!TestTrue(FString::Printf(TEXT("l'archivio della %s run si apre"), R.Value),
			URTReplayPlayerLibrary::OpenArchive(Root.Path, R.Key->ReplayMatchId, Session) == ERTReplayOpenResult::Opened))
		{
			continue;
		}
		TestEqual(FString::Printf(TEXT("l'archivio della %s run ha i turni di QUELLA run"), R.Value),
			Session.Manifest.TurnCount, R.Key->TurnsPlayed);
	}
	return true;
}

/**
 * 10. Il turn manager esce dalla run **com'era entrato**: radice, formato e identita' della registrazione.
 *
 * Il caso e' un GameMode che arriva DOPO: il suo turn manager non registra ancora, e la run di scenario gli
 * lascerebbe la radice di prova e `Skirmish2v2` come formato. Qui il turn manager e' configurato con valori propri, la
 * run registra, e poi si fa quello che fa il GameMode: `BeginReplayRecording`. L'archivio che ne esce deve stare nella
 * SUA radice e col SUO formato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioBridgeLeavesTurnManagerTest,
	"RefactorTactics.Replay.ScenarioBridge.TheTurnManagerIsLeftAsItWasFound",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioBridgeLeavesTurnManagerTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!ScenarioBridgeLoad(*this, TEXT("Movement.Basic"), Scenario)) { return false; }

	const FScenarioBridgeRoot RootScenario(TEXT("TmScenario"));
	const FScenarioBridgeRoot RootGame(TEXT("TmGame"));
	const FName FormatoProprio(TEXT("ScenarioBridgePriorFormat"));

	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }

	// Il turn manager c'e' PRIMA della run, e porta la configurazione di chi lo possiede.
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("turn manager"), TM))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	TM->ReplaysRootOverride = RootGame.Path;
	FRTMatchRules Regole = TM->GetMatchRules();
	Regole.FormatId = FormatoProprio;
	TM->SetMatchRules(Regole);
	const bool bRecordPrima = TM->bRecordReplay;

	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario, RootScenario.Path);

	if (ScenarioBridgeExpectPass(*this, Result))
	{
		TestTrue(TEXT("la run ha registrato"), Result.ReplayMatchId.IsValid());
		TestEqual(TEXT("una cartella di archivio nella radice dello scenario"), RootScenario.MatchDirectories().Num(), 1);

		TestEqual(TEXT("la radice del turn manager e' tornata la sua"), TM->ReplaysRootOverride, RootGame.Path);
		TestEqual(TEXT("e il formato"), TM->GetMatchRules().FormatId, FormatoProprio);
		TestEqual(TEXT("e bRecordReplay non e' stato toccato"), TM->bRecordReplay, bRecordPrima);
		TestFalse(TEXT("e non resta appesa l'identita' di una registrazione che non c'e' piu'"),
			TM->GetReplayMatchId().IsValid());

		// Il GameMode che arriva dopo.
		TM->BeginReplayRecording();
		if (TestTrue(TEXT("il turn manager puo' aprire la SUA registrazione"), TM->GetReplayMatchId().IsValid()))
		{
			TArray<FRTMatchHistoryEntry> RigheGame;
			URTMatchHistoryLibrary::LoadIndex(RootGame.Path, RigheGame);
			if (TestEqual(TEXT("e la registra nella sua radice"), RigheGame.Num(), 1))
			{
				TestEqual(TEXT("con la sua identita'"), RigheGame[0].MatchId, TM->GetReplayMatchId());
				TestEqual(TEXT("e col suo formato"), RigheGame[0].FormatId, FormatoProprio);
			}

			TArray<FRTMatchHistoryEntry> RigheScenario;
			URTMatchHistoryLibrary::LoadIndex(RootScenario.Path, RigheScenario);
			TestEqual(TEXT("la radice dello scenario non si e' mossa"), RigheScenario.Num(), 1);
		}
	}
	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 11. L'archivio dello showcase e' **chiuso** quando lo scenario finisce, anche se nessuno ha vinto.
 *
 * Lo showcase gioca otto turni e non ha un'assertion di esito di partita: il turn manager chiude l'archivio solo se la
 * PARTITA si decide, quindi senza la chiusura della sessione il manifest resterebbe `bClosed = false`, il Player lo
 * aprirebbe `bComplete = false` e la lista mostrerebbe «registrazione incompleta» per uno scenario terminato
 * regolarmente (`#3035`, rilievo di review).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioBridgeClosesArchiveTest,
	"RefactorTactics.Replay.ScenarioBridge.ShowcaseArchiveIsClosedWhenTheScenarioEnds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioBridgeClosesArchiveTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	if (!ScenarioBridgeLoad(*this, TEXT("RT_Showcase_Relay_v01"), Scenario)) { return false; }

	const FScenarioBridgeRoot Root(TEXT("Closed"));
	const FRTTestResult Result = ScenarioBridgeRun(Scenario, Root.Path);
	if (!ScenarioBridgeExpectPass(*this, Result)) { return false; }
	if (!TestTrue(TEXT("c'e' un MatchId"), Result.ReplayMatchId.IsValid())) { return false; }

	FRTReplaySession Session;
	if (!TestTrue(TEXT("l'archivio si apre"),
		URTReplayPlayerLibrary::OpenArchive(Root.Path, Result.ReplayMatchId, Session) == ERTReplayOpenResult::Opened))
	{
		return false;
	}
	TestTrue(TEXT("il manifest e' chiuso"), Session.Manifest.bClosed);
	TestTrue(TEXT("e il Player lo apre come completo"), Session.bComplete);
	TestNotEqual(TEXT("con l'hash di stato finale (la chiusura non e' un involucro vuoto)"),
		Session.Manifest.FinalStateHash, static_cast<int64>(0));
	TestEqual(TEXT("e tutti i turni giocati"), Session.Manifest.TurnCount, Result.TurnsPlayed);

	// La riga della lista lo dice con le stesse parole: niente «registrazione incompleta».
	TArray<FRTMatchHistoryEntry> Righe;
	URTMatchHistoryLibrary::LoadIndex(Root.Path, Righe);
	if (TestEqual(TEXT("una riga nello storico"), Righe.Num(), 1))
	{
		TestTrue(TEXT("e la riga dichiara il replay completo"), Righe[0].bReplayComplete);
		TestEqual(TEXT("coi turni giocati"), Righe[0].TurnCount, Result.TurnsPlayed);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
