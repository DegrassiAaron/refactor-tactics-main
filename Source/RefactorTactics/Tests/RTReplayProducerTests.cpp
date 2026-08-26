#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h" // MakeFlatArena: un solo builder di arena piatta
#include "Replay/RTReplayRecorderLibrary.h"
#include "Replay/RTReplayManifest.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTMatchFormatData.h"
#include "Turn/RTMatchFormatLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTTurnRules.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Turn/RTMatchStateHash.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Il PRODUTTORE dell'archivio replay (#469).
 *
 * `RTReplayRecorderTests.cpp` verifica la libreria: che sappia scrivere, rileggere, rifiutare. Questo file
 * verifica una cosa diversa e piu' fragile — che una **partita vera** la chiami. E' la distinzione che questo
 * repository ha gia' pagato tre volte: `UnitId` (D-063), `GraphRevision` (D-067) e il checksum di fine partita
 * sono atterrati tutti come «formato implementato, produttore assente», con la suite interamente verde.
 *
 * Il percorso e' quello del gioco: bot che pianificano, `LockInAndResolve`, turni fino all'esito. Nessuna
 * scorciatoia, nessun `SetActorLocation`.
 */
namespace
{
	UWorld* MakeReplayProducerWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyReplayProducerWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	URTHexMapAsset* SpawnReplayProducerMap(UWorld* World, int32 Radius)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return M;
	}

	ARTUnit* SpawnReplayProducerUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = true; // bot contro bot: la partita si gioca da sola, senza mano umana
		U->DispatchBeginPlay();      // senza, i cooldown non nascono e ogni abilita' e' sempre pronta
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/** Radice temporanea per test: due test che condividono una cartella si mascherano i difetti. */
	FString ProducerRoot(const TCHAR* Nome)
	{
		return FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ReplayProd"), Nome);
	}

	void PuliscIProducer(const FString& Root)
	{
		IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
		if (PF.DirectoryExists(*Root)) { PF.DeleteDirectoryRecursively(*Root); }
	}

	/** Allestisce la stessa identica partita 2v2 in un mondo nuovo. Le posizioni non cambiano fra le chiamate. */
	ARTTurnManager* SetUpMatch(UWorld* World)
	{
		SpawnReplayProducerMap(World, /*Radius=*/ 5);
		SpawnReplayProducerUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(),  FRTCellId(-4, 2));
		SpawnReplayProducerUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(-4, 3));
		SpawnReplayProducerUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(),  FRTCellId(4, -2));
		SpawnReplayProducerUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(4, -3));

		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

		// Il formato va risolto PRIMA di registrare, come fa il GameMode con `ApplyMatchFormat`: da quando
		// `BeginReplayRecording` si rifiuta senza un formato (un archivio con `FormatId = None` non e'
		// confrontabile con niente), un allestimento di prova che lo salta non simula piu' una partita.
		if (TM)
		{
			FRTMatchRules Rules;
			Rules.FormatId = URTMatchFormatLibrary::Skirmish2v2FormatId;
			TM->SetMatchRules(Rules);
		}
		return TM;
	}

	/** Un turno completo: pianificazione dei bot, risoluzione, playback fino in fondo. */
	void PlayOneTurn(ARTTurnManager* TM)
	{
		TM->PlanBotsForTest();
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/**
	 * Digest dello stato del mondo, costruito DAL TEST con la stessa funzione che usano partita e harness.
	 *
	 * Non si chiede il valore al `TurnManager`: quello lo cattura solo se sta registrando, e la registrazione
	 * e' precisamente la variabile indipendente dell'esperimento. Misurare con lo strumento che si sta
	 * verificando renderebbe il test cieco proprio al difetto che cerca.
	 */
	uint32 DigestOfWorld(UWorld* World, ARTTurnManager* TM)
	{
		TArray<AActor*> Actors;
		UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), Actors);

		TArray<ARTUnit*> Units;
		Units.Reserve(Actors.Num());
		for (AActor* Actor : Actors)
		{
			if (ARTUnit* Unit = Cast<ARTUnit>(Actor)) { Units.Add(Unit); }
		}

		const ARTHexMapActor* MapActor = ARTHexMapActor::FindInWorld(World);
		const TArray<int32> TeamScores = { TM->GetTeamScore(0), TM->GetTeamScore(1) };

		return URTMatchStateHashLibrary::HashMatchState(MapActor ? MapActor->MapAsset : nullptr,
			URTMatchStateHashLibrary::BuildUnitDigests(Units), TeamScores);
	}

	/** Gioca fino all'esito (o al tetto di sicurezza) e restituisce i turni giocati. */
	int32 PlayToCompletion(ARTTurnManager* TM, int32 MaxTurns = 40)
	{
		int32 Played = 0;
		while (TM->GetPhase() != ERTMatchPhase::MatchEnded && Played < MaxTurns)
		{
			PlayOneTurn(TM);
			++Played;
		}
		return Played;
	}
}

/**
 * Una partita giocata dall'inizio alla fine lascia un archivio leggibile, senza che nessuno lo chieda a mano.
 *
 * Copre quattro dei sei criteri di #469 in un colpo solo, perche' sono proprieta' dello STESSO archivio: che
 * esista, che abbia un blocco per turno, che le tracce siano byte-identiche a `SerializeTurnLog`, e che il
 * checksum di fine partita sia stato calcolato da una partita vera.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayProducerWritesArchiveTest,
	"RefactorTactics.Replay.Producer.MatchLeavesArchive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayProducerWritesArchiveTest::RunTest(const FString&)
{
	const FString Root = ProducerRoot(TEXT("MatchLeavesArchive"));
	PuliscIProducer(Root);

	UWorld* World = MakeReplayProducerWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTTurnManager* TM = SetUpMatch(World);
	if (!TM) { DestroyReplayProducerWorld(World); return false; }

	TM->ReplaysRootOverride = Root;
	TM->BeginReplayRecording();

	const FGuid MatchId = TM->GetReplayMatchId();
	TestTrue(TEXT("il MatchId e' stato generato"), MatchId.IsValid());

	const int32 TurnsPlayed = PlayToCompletion(TM);
	TestTrue(TEXT("la partita si e' decisa entro il limite di turni"), TM->GetPhase() == ERTMatchPhase::MatchEnded);
	TestTrue(TEXT("sono stati giocati piu' turni"), TurnsPlayed > 1);

	// 1. L'ARCHIVIO ESISTE, e lo si rilegge da disco: non si guarda il manifest in memoria, che sarebbe la
	//    prova che il TurnManager ha una variabile — non che qualcosa sia stato scritto.
	FRTReplayManifest Letto;
	if (!TestTrue(TEXT("il manifest e' su disco e si rilegge"),
			URTReplayRecorderLibrary::LoadManifest(Root, MatchId, Letto)))
	{
		DestroyReplayProducerWorld(World);
		PuliscIProducer(Root);
		return false;
	}

	// 2. UN BLOCCO PER TURNO: esattamente N, non «almeno N».
	TestEqual(TEXT("il manifest conta i turni giocati"), Letto.TurnCount, TurnsPlayed);
	TestEqual(TEXT("un hash ordinato per turno"), Letto.OrderedHashPerTurn.Num(), TurnsPlayed);

	// 3. IL MANIFEST E' CHIUSO con l'esito vero: una partita finita non lascia un archivio parziale.
	TestTrue(TEXT("il manifest e' chiuso"), Letto.bClosed);
	TestTrue(TEXT("l'esito non e' 'in corso'"), Letto.Outcome != ERTMatchOutcome::InProgress);

	// 4. IL CHECKSUM DI FINE PARTITA E' CALCOLATO DA UNA PARTITA VERA, e non e' zero. E' il criterio per cui
	//    questa issue esiste: il formato lo prevedeva gia', e nessuno lo produceva.
	TestNotEqual(TEXT("il checksum di fine partita non e' zero"), Letto.FinalStateHash, static_cast<int64>(0));
	TestEqual(TEXT("il checksum su disco e' quello che la partita ha catturato"),
		Letto.FinalStateHash, TM->GetPendingFinalStateHash());

	// 5. LE TRACCE SONO BYTE-IDENTICHE a quelle che `SerializeTurnLog` produce: il recorder non e' un secondo
	//    serializzatore. Si verifica sull'ULTIMO turno, l'unico il cui TurnLog e' ancora in memoria.
	{
		const FString UltimaTraccia = FPaths::Combine(
			URTReplayRecorderLibrary::MatchDirectory(Root, MatchId),
			URTReplayRecorderLibrary::TurnFileName(TurnsPlayed));

		TArray<uint8> DaDisco;
		if (TestTrue(TEXT("la traccia dell'ultimo turno e' su disco"), FFileHelper::LoadFileToArray(DaDisco, *UltimaTraccia)))
		{
			// Stessi argomenti che il recorder passa: la topologia dichiarata dal manifest e il suo `FormatId`.
			// Con un `FormatId` diverso i byte differirebbero nell'header, e il test direbbe «non identiche»
			// per colpa propria.
			const TArray<uint8> Attesi = URTTurnLogLibrary::SerializeTurnLog(
				TM->GetTurnLog(), ERTLogTopology::Hex, Letto.FormatId);
			TestEqual(TEXT("la traccia e' byte-identica a SerializeTurnLog"), DaDisco, Attesi);
		}
	}

	DestroyReplayProducerWorld(World);
	PuliscIProducer(Root);
	return true;
}

/**
 * Registrare NON cambia l'esito.
 *
 * E' l'unico criterio che verifica il gioco invece del formato: gli altri dicono che l'archivio regge, questo
 * dice che **l'osservazione non altera l'osservato**. La stessa partita, con e senza recorder, deve finire
 * nello stesso stato — stesso checksum, stesso numero di turni, stesso esito.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayProducerIsNotObservableTest,
	"RefactorTactics.Replay.Producer.RecordingDoesNotChangeTheMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayProducerIsNotObservableTest::RunTest(const FString&)
{
	const FString Root = ProducerRoot(TEXT("NotObservable"));
	PuliscIProducer(Root);

	// Partita SENZA recorder.
	uint32 HashSenza = 0;
	int32 TurniSenza = 0;
	ERTMatchOutcome EsitoSenza = ERTMatchOutcome::InProgress;
	{
		UWorld* World = MakeReplayProducerWorld();
		if (!TestNotNull(TEXT("world senza recorder"), World)) { return false; }
		ARTTurnManager* TM = SetUpMatch(World);
		if (!TM) { DestroyReplayProducerWorld(World); return false; }

		// Registrazione SPENTA: e' la variabile indipendente dell'esperimento.
		TM->bRecordReplay = false;
		TurniSenza = PlayToCompletion(TM);
		HashSenza = DigestOfWorld(World, TM);
		EsitoSenza = TM->GetMatchResult().Outcome;
		DestroyReplayProducerWorld(World);
	}

	// Stessa partita CON recorder.
	uint32 HashCon = 0;
	int32 TurniCon = 0;
	ERTMatchOutcome EsitoCon = ERTMatchOutcome::InProgress;
	{
		UWorld* World = MakeReplayProducerWorld();
		if (!TestNotNull(TEXT("world con recorder"), World)) { return false; }
		ARTTurnManager* TM = SetUpMatch(World);
		if (!TM) { DestroyReplayProducerWorld(World); return false; }

		TM->ReplaysRootOverride = Root;
		TM->BeginReplayRecording();
		TurniCon = PlayToCompletion(TM);
		HashCon = DigestOfWorld(World, TM);
		EsitoCon = TM->GetMatchResult().Outcome;
		DestroyReplayProducerWorld(World);
	}

	TestEqual(TEXT("stesso numero di turni"), TurniCon, TurniSenza);
	TestTrue(TEXT("stesso esito"), EsitoCon == EsitoSenza);
	TestEqual(TEXT("stesso checksum di fine partita"), HashCon, HashSenza);

	PuliscIProducer(Root);
	return true;
}

/**
 * `MatchId` non entra in nessun hash della traccia.
 *
 * Il criterio e' verificabile e non dichiarativo: si registra due volte la stessa partita — due `MatchId`
 * diversi per costruzione — e gli hash ordinati per turno devono coincidere uno per uno. Se il `MatchId`
 * filtrasse in un hash, questi due archivi divergerebbero pur raccontando la stessa identica partita.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayProducerMatchIdOutOfHashesTest,
	"RefactorTactics.Replay.Producer.MatchIdDoesNotEnterHashes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayProducerMatchIdOutOfHashesTest::RunTest(const FString&)
{
	const FString Root = ProducerRoot(TEXT("MatchIdOutOfHashes"));
	PuliscIProducer(Root);

	auto GiocaERegistra = [&](TArray<int64>& OutHashes, FGuid& OutMatchId, int64& OutFinalHash) -> bool
	{
		UWorld* World = MakeReplayProducerWorld();
		if (!World) { return false; }
		ARTTurnManager* TM = SetUpMatch(World);
		if (!TM) { DestroyReplayProducerWorld(World); return false; }

		TM->ReplaysRootOverride = Root;
		TM->BeginReplayRecording();
		OutMatchId = TM->GetReplayMatchId();
		PlayToCompletion(TM);

		// Dal DISCO, non dalla memoria: l'archivio e' l'artefatto di cui si parla, e il manifest in memoria
		// non fa parte dell'API pubblica del TurnManager.
		FRTReplayManifest Letto;
		const bool bLetto = URTReplayRecorderLibrary::LoadManifest(Root, OutMatchId, Letto);
		OutHashes = Letto.OrderedHashPerTurn;
		OutFinalHash = Letto.FinalStateHash;
		DestroyReplayProducerWorld(World);
		return bLetto;
	};

	TArray<int64> HashesA, HashesB;
	FGuid IdA, IdB;
	int64 FinalA = 0, FinalB = 0;
	if (!TestTrue(TEXT("prima registrazione"), GiocaERegistra(HashesA, IdA, FinalA))) { return false; }
	if (!TestTrue(TEXT("seconda registrazione"), GiocaERegistra(HashesB, IdB, FinalB))) { return false; }

	TestTrue(TEXT("due registrazioni, due MatchId diversi"), IdA != IdB);
	TestEqual(TEXT("stesso numero di turni"), HashesB.Num(), HashesA.Num());
	TestEqual(TEXT("gli hash ordinati per turno non dipendono dal MatchId"), HashesB, HashesA);
	TestEqual(TEXT("il checksum di fine partita non dipende dal MatchId"), FinalB, FinalA);

	PuliscIProducer(Root);
	return true;
}

/**
 * Una partita INTERROTTA lascia un archivio parziale riconoscibile.
 *
 * Non serve un crash per provarlo: il manifest si chiude alla fine, quindi un manifest **senza chiusura** *e'*
 * la dichiarazione di parzialita'. Si gioca qualche turno e si smette — che e' cio' che il disco vede quando
 * il processo muore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayProducerPartialArchiveTest,
	"RefactorTactics.Replay.Producer.InterruptedMatchIsRecognizable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayProducerPartialArchiveTest::RunTest(const FString&)
{
	const FString Root = ProducerRoot(TEXT("Interrupted"));
	PuliscIProducer(Root);

	UWorld* World = MakeReplayProducerWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTTurnManager* TM = SetUpMatch(World);
	if (!TM) { DestroyReplayProducerWorld(World); return false; }

	TM->ReplaysRootOverride = Root;
	TM->BeginReplayRecording();
	const FGuid MatchId = TM->GetReplayMatchId();

	// Due turni e basta: la partita non e' finita, e nessuno chiude niente.
	PlayOneTurn(TM);
	PlayOneTurn(TM);

	FRTReplayManifest Letto;
	if (!TestTrue(TEXT("l'archivio parziale si apre"),
			URTReplayRecorderLibrary::LoadManifest(Root, MatchId, Letto)))
	{
		DestroyReplayProducerWorld(World);
		PuliscIProducer(Root);
		return false;
	}

	// Si apre e NON mente sul proprio stato: due turni scritti, manifest non chiuso, esito non dichiarato.
	TestEqual(TEXT("i turni giocati sono su disco"), Letto.TurnCount, 2);
	TestFalse(TEXT("il manifest non e' chiuso"), Letto.bClosed);
	TestTrue(TEXT("l'esito resta 'in corso'"), Letto.Outcome == ERTMatchOutcome::InProgress);
	TestEqual(TEXT("nessun checksum di fine partita"), Letto.FinalStateHash, static_cast<int64>(0));

	DestroyReplayProducerWorld(World);
	PuliscIProducer(Root);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
