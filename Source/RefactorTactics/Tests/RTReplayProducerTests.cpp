#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Replay/RTReplayAuditLibrary.h"
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
#include "Ability/RTHeroCatalogLibrary.h"
#include "Turn/RTMatchStateHash.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Tests/RTWorldFixtures.h"

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
			RTWorldFixtures::PlayOneTurn(TM);
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
	RTWorldFixtures::PlayOneTurn(TM);
	RTWorldFixtures::PlayOneTurn(TM);

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

/**
 * 🔑 **Una partita giocata lascia un'evidenza AUDITABILE, e i due controlli la trovano pulita.**
 *
 * E' il test che distingue un archivio che *contiene* la conoscenza da uno che *dimostra* qualcosa: i
 * quattro record di [D-313] esistono su disco, si rileggono, e le due domande d'audit ottengono una
 * risposta invece di una promessa.
 *
 * ⚠️ **Bot contro bot**, quindi entrambe le squadre si giudicano: l'harness gia' lo fa, e qui serve —
 * un controllo d'equita' che escludesse una delle due meta' del campo proverebbe meta' di cio' che dice.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayAuditProducerTest,
	"RefactorTactics.Replay.Audit.AMatchLeavesAuditableEvidence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayAuditProducerTest::RunTest(const FString&)
{
	const FString Root = ProducerRoot(TEXT("AuditEvidence"));
	PuliscIProducer(Root);

	UWorld* World = MakeReplayProducerWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTTurnManager* TM = SetUpMatch(World);
	if (!TM) { DestroyReplayProducerWorld(World); return false; }

	TM->ReplaysRootOverride = Root;
	TM->BeginReplayRecording();
	const FGuid MatchId = TM->GetReplayMatchId();

	const int32 TurnsPlayed = PlayToCompletion(TM);
	if (!TestTrue(TEXT("sono stati giocati piu' turni"), TurnsPlayed > 1))
	{
		DestroyReplayProducerWorld(World);
		PuliscIProducer(Root);
		return false;
	}

	FRTReplayManifest Manifest;
	if (!TestTrue(TEXT("il manifest si rilegge"),
			URTReplayRecorderLibrary::LoadManifest(Root, MatchId, Manifest)))
	{
		DestroyReplayProducerWorld(World);
		PuliscIProducer(Root);
		return false;
	}

	int32 ConVerdetti = 0;
	int32 ConScelte = 0;
	for (int32 Turno = 1; Turno <= TurnsPlayed; ++Turno)
	{
		const int64 AncoraAttesa = Manifest.OrderedHashPerTurn.IsValidIndex(Turno - 1)
			? Manifest.OrderedHashPerTurn[Turno - 1]
			: 0;

		FRTTurnAudit Audit;
		if (!TestTrue(*FString::Printf(TEXT("l'audit del turno %d si rilegge, e l'ancora tiene"), Turno),
				URTReplayAuditLibrary::LoadTurnAudit(Root, MatchId, Turno, Audit, AncoraAttesa)))
		{
			continue;
		}

		// L'ancora tiene: l'evidenza dichiara la TRACCIA a cui appartiene, non solo il turno.
		if (Manifest.OrderedHashPerTurn.IsValidIndex(Turno - 1))
		{
			TestEqual(*FString::Printf(TEXT("turno %d: l'ancora coincide col manifest"), Turno),
				Audit.OrderedHash, Manifest.OrderedHashPerTurn[Turno - 1]);
		}

		// Due conoscenze per squadra: se una delle due liste fosse vuota, uno dei due controlli sarebbe
		// verde per assenza di soggetto invece che per assenza di difetti.
		TestTrue(*FString::Printf(TEXT("turno %d: la conoscenza di Planning c'e'"), Turno),
			Audit.PlanningKnowledge.Num() > 0);
		TestTrue(*FString::Printf(TEXT("turno %d: la conoscenza di Blast c'e'"), Turno),
			Audit.BlastKnowledge.Num() > 0);

		// 🔑 Il controllo di coerenza dei verdetti: l'anti-vacuita' di [D-223], su una partita vera.
		const TArray<FString> Divergenze = URTReplayAuditLibrary::FindVerdictMismatches(Audit);
		TestEqual(*FString::Printf(TEXT("turno %d: nessun verdetto diverge dalla conoscenza registrata (%s)"),
			Turno, *FString::Join(Divergenze, TEXT(" | "))), Divergenze.Num(), 0);
		ConVerdetti += Audit.Verdicts.Num();

		// 🔑 E l'equita', sulla partita vera: nessun bot ha SCELTO un bersaglio che la sua squadra non
		// conosceva. E' la domanda di `D-276`, posta al predicato di produzione sulla conoscenza registrata.
		const TArray<FString> NonAutorizzati = URTReplayAuditLibrary::FindUnauthorizedTargets(Audit);
		TestEqual(*FString::Printf(TEXT("turno %d: nessun bot ha scelto cio' che non conosceva (%s)"),
			Turno, *FString::Join(NonAutorizzati, TEXT(" | "))), NonAutorizzati.Num(), 0);
		// ⚠️ **Si contano le scelte GIUDICATE, non i record.** `CaptureBotDecisionsForAudit` emette una voce
		// per ogni bot vivo, bersaglio o non bersaglio, e `FindUnauthorizedTargets` salta chi non ha scelto
		// nulla: contare i record farebbe passare l'anti-vacuita' con un archivio in cui NESSUNA voce e'
		// stata esaminata — la stessa vacuita', un piano piu' in basso.
		for (const FRTAuditBotDecision& Scelta : Audit.BotDecisions)
		{
			if (Scelta.TargetUnitId != INDEX_NONE) { ++ConScelte; }
		}

		// E per turno: un turno senza NESSUNA voce di scelta e' un turno la cui equita' non e' stata
		// archiviata, e il totale sopra lo nasconderebbe dietro i turni che invece ce l'hanno.
		TestTrue(*FString::Printf(TEXT("turno %d: le scelte dei bot sono archiviate"), Turno),
			Audit.BotDecisions.Num() > 0);

		// I verdetti registrati sono tanti quante le voci della traccia: e' l'invariante posizionale che
		// [D-313] §7 dichiara, e qui si MISURA invece di restare scritta.
		TArray<FRTTurnLogEntry> Voci;
		const FString TracePath = FPaths::Combine(
			URTReplayRecorderLibrary::MatchDirectory(Root, MatchId),
			URTReplayRecorderLibrary::TurnFileName(Turno));
		if (URTTurnLogLibrary::LoadTurnLogFromFile(TracePath, Voci))
		{
			TestEqual(*FString::Printf(TEXT("turno %d: un verdetto per voce"), Turno),
				Audit.Verdicts.Num(), Voci.Num());
		}
	}

	// ⚠️ **Anti-vacuita' dell'intero test**: senza un verdetto registrato, `FindVerdictMismatches` sarebbe
	// verde su un array vuoto e questo test non proverebbe niente. Una partita giocata ne produce.
	TestTrue(TEXT("la partita ha prodotto verdetti da verificare"), ConVerdetti > 0);

	// ⚠️ E lo stesso per l'equita': con zero scelte registrate `FindUnauthorizedTargets` sarebbe verde su un
	// array vuoto, che e' precisamente la vacuita' con cui la prima stesura di questo test era passata.
	TestTrue(TEXT("la partita ha prodotto scelte di bot da verificare"), ConScelte > 0);

	DestroyReplayProducerWorld(World);
	PuliscIProducer(Root);
	return true;
}


/**
 * 🔴 **Una CARICA e' una scelta di bersaglio, e deve finire nell'archivio come tale.**
 *
 * Due difetti indipendenti la rendevano invisibile, e questo test li tiene insieme perche' in partita
 * arrivano insieme:
 *
 * 1. **Il percorso.** La cattura viveva in `StartPlanningTimer`, non in `PlanBots`. Ma il commento sopra
 *    `EnsureMatchRoster` in `LockInAndResolve` dichiara da sempre che i percorsi sono **due** — il gioco
 *    entra dal timer, l'harness e i test chiamano `PlanBotsForTest()` e poi `LockInAndResolve()`. Sul
 *    secondo l'archivio restava vuoto, e un archivio vuoto non e' un'assoluzione: e' un'assenza di prove
 *    che il controllo legge come «nessuna violazione».
 *
 * 2. **La carica.** Il ramo `bIsCharge` di `PlanBots` non scrive `PlannedAttackTarget` — il colpo e'
 *    dell'azione di movimento, quindi non c'e' un'azione principale a cui appendere il bersaglio. Una
 *    cattura che leggesse solo quel campo perderebbe **l'intera classe di scelte piu' aggressiva del bot**:
 *    proprio quella su cui la domanda d'equita' morde di piu'.
 *
 * ⚠️ Il test **verifica prima** che il bot abbia scelto una carica *senza* `PlannedAttackTarget`: senza
 * quell'asserzione, un giorno in cui l'utility preferisse un tiro normale questo test resterebbe verde
 * misurando il ramo facile.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayAuditChargeIsAChoiceTest,
	"RefactorTactics.Replay.Audit.ABotChargeIsArchivedAsAChoice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayAuditChargeIsAChoiceTest::RunTest(const FString&)
{
	const FString Root = ProducerRoot(TEXT("AuditCharge"));
	PuliscIProducer(Root);

	UWorld* World = MakeReplayProducerWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnReplayProducerMap(World, /*Radius=*/ 5);

	// Stessa geometria di `HexBotPlay.ChargeItPlannedActuallyLands`, dove la carica e' gia' misurata come
	// la mossa che l'utility preferisce: Riktor ha `Ram` (20 + spinta) contro `ImpactShot` (8).
	ARTUnit* Bot = SpawnReplayProducerUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0));
	ARTUnit* Foe = SpawnReplayProducerUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Bot || !Foe) { DestroyReplayProducerWorld(World); PuliscIProducer(Root); return false; }

	Foe->bIsBotControlled = false; // il bersaglio sta fermo: qui si misura la scelta di CHI carica
	{
		FRTMatchRules Rules;
		Rules.FormatId = URTMatchFormatLibrary::Skirmish2v2FormatId;
		TM->SetMatchRules(Rules);
	}
	TM->ReplaysRootOverride = Root;
	TM->BeginReplayRecording();
	const FGuid MatchId = TM->GetReplayMatchId();

	// Il percorso dell'harness, non quello del timer: e' meta' del difetto.
	TM->PlanBotsForTest();

	const URTActionData* Dash = Bot->PlannedDashAbility != INDEX_NONE
		? Bot->GetAbility(Bot->PlannedDashAbility) : nullptr;
	if (!TestTrue(TEXT("il bot ha scelto una CARICA da solo"),
			Dash != nullptr && Dash->Def.MovementStyle == ERTMovementStyle::LinearCharge))
	{
		DestroyReplayProducerWorld(World);
		PuliscIProducer(Root);
		return false;
	}
	// 🔑 E la carica NON passa da `PlannedAttackTarget`: e' l'altra meta' del difetto, dichiarata invece
	// che assunta. Se un giorno cambiasse, questa riga lo direbbe subito.
	TestNull(TEXT("e una carica non scrive PlannedAttackTarget"), Bot->PlannedAttackTarget.Get());

	TM->LockInAndResolve();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

	FRTTurnAudit Audit;
	if (!TestTrue(TEXT("l'audit del turno si rilegge"),
			URTReplayAuditLibrary::LoadTurnAudit(Root, MatchId, 1, Audit)))
	{
		DestroyReplayProducerWorld(World);
		PuliscIProducer(Root);
		return false;
	}

	const FRTAuditBotDecision* Scelta = Audit.BotDecisions.FindByPredicate(
		[Bot](const FRTAuditBotDecision& D) { return D.UnitId == Bot->StableUnitId; });
	if (!TestNotNull(TEXT("l'archivio contiene la scelta di chi ha caricato"), Scelta))
	{
		DestroyReplayProducerWorld(World);
		PuliscIProducer(Root);
		return false;
	}

	TestEqual(TEXT("e nomina il bersaglio della carica"), Scelta->TargetUnitId, Foe->StableUnitId);
	TestEqual(TEXT("con la sua squadra"), Scelta->TargetTeamId, Foe->TeamId);
	TestEqual(TEXT("e la cella da cui il cancello lo ha autorizzato"), Scelta->TargetCell, FRTCellId(2, 0));

	DestroyReplayProducerWorld(World);
	PuliscIProducer(Root);
	return true;
}


namespace
{
/**
 * Una partita **giocata e archiviata**, riletta da disco — e i suoi difetti DICHIARATI invece che ignorati.
 *
 * 🔴 **I byte accanto alle voci, e i byte sono la misura.** `DescribeFirstDivergence` confronta due voci con
 * `GoldenEntriesMatch`, cioe' con l'**hash** — e `UnitId`, `TurnNumber`, `Priority`, `ReactionInstanceId` e
 * `OriginalTargetUnitId` nell'hash non entrano ([D-063]). Ma `SerializeTurnLog` li **scrive**. Un test che
 * dicesse «archivi identici» guardando solo l'hash resterebbe verde con due archivi diversi su disco in ogni
 * voce: basterebbe togliere il `Sort` di `EnsureMatchRoster`. Le voci restano, e servono a **nominare** la
 * divergenza; a dichiararla sono i byte.
 */
struct FRTPartitaArchiviata
{
	TArray<TArray<uint8>> Byte;                  // i byte di ogni turno, come stanno su disco
	TArray<TArray<FRTTurnLogEntry>> Voci;        // le stesse tracce lette, per dire DOVE
	FGuid MatchId;
	int32 TurniGiocati = 0;
	bool bFinita = false;                        // ha raggiunto `MatchEnded`, non il tetto di sicurezza
	bool bTutteLette = true;                     // ogni turno giocato ha il suo file, e si e' letto
};

/**
 * Gioca una partita registrata e la rilegge da disco.
 *
 * `PerturbaVerso` sposta l'unita' di partenza della squadra 0 — **scelta per squadra e cella, non per
 * ordine di container**: `GetAllActorsOfClass` non ha un ordine contrattuale, e farci dipendere QUALE
 * unita' viene spostata renderebbe l'esperimento diverso da quello descritto senza che niente lo dica.
 *
 * ⚠️ **Non azzera `Out`: lo pretende vuoto.** Un helper che accoda dove il chiamante crede si riempia
 * concatenerebbe due partite in silenzio, e il confronto leggerebbe il turno 3 di una contro il 22
 * dell'altra chiamandoli entrambi «turno 3».
 */
bool GiocaEArchivia(FAutomationTestBase& Test, const TCHAR* Nome, const FRTCellId* PerturbaVerso,
	FRTPartitaArchiviata& Out)
{
	check(Out.Byte.Num() == 0 && Out.Voci.Num() == 0);

	const FString Root = ProducerRoot(Nome);
	PuliscIProducer(Root);

	UWorld* World = MakeReplayProducerWorld();
	if (!Test.TestNotNull(*FString::Printf(TEXT("%s: world di prova"), Nome), World)) { return false; }

	ARTTurnManager* TM = SetUpMatch(World);
	if (!Test.TestNotNull(*FString::Printf(TEXT("%s: TurnManager"), Nome), TM))
	{
		DestroyReplayProducerWorld(World);
		PuliscIProducer(Root);
		return false;
	}

	if (PerturbaVerso != nullptr)
	{
		TArray<AActor*> Attori;
		UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), Attori);

		ARTUnit* DaSpostare = nullptr;
		bool bDestinazioneLibera = true;
		for (AActor* Attore : Attori)
		{
			ARTUnit* U = Cast<ARTUnit>(Attore);
			if (!U) { continue; }
			// L'unita' che `SetUpMatch` mette in (-4,2): nominata, non pescata.
			if (U->TeamId == 0 && U->Cell == FRTCellId(-4, 2)) { DaSpostare = U; }
			if (U->Cell == *PerturbaVerso) { bDestinazioneLibera = false; }
		}

		// 🔴 Entrambe le condizioni si DICHIARANO: se `SetUpMatch` cambia le sue celle di partenza, questo
		// test deve dirlo forte invece di spostare un'altra unita' — o di impilarne due sulla stessa cella,
		// che e' uno stato illegale da cui la divergenza verrebbe comunque, ma per la ragione sbagliata.
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: l'unita' da perturbare e' dove la fixture la mette"), Nome), DaSpostare)
			|| !Test.TestTrue(*FString::Printf(TEXT("%s: la cella di destinazione e' libera"), Nome), bDestinazioneLibera))
		{
			DestroyReplayProducerWorld(World);
			PuliscIProducer(Root);
			return false;
		}
		DaSpostare->PlaceOnCell(*PerturbaVerso, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
	}

	TM->ReplaysRootOverride = Root;
	TM->BeginReplayRecording();
	Out.MatchId = TM->GetReplayMatchId();

	// 🔴 `BeginReplayRecording` esce in silenzio senza formato o a registrazione spenta. Senza questa riga
	// il difetto arriverebbe travestito da «l'archivio non si legge», che manda chi legge dalla parte
	// sbagliata: il difetto sarebbe che la registrazione non e' mai partita.
	if (!Test.TestTrue(*FString::Printf(TEXT("%s: la registrazione e' partita"), Nome), Out.MatchId.IsValid()))
	{
		DestroyReplayProducerWorld(World);
		PuliscIProducer(Root);
		return false;
	}

	Out.TurniGiocati = PlayToCompletion(TM);
	Out.bFinita = (TM->GetPhase() == ERTMatchPhase::MatchEnded);

	for (int32 Turno = 1; Turno <= Out.TurniGiocati; ++Turno)
	{
		const FString Path = FPaths::Combine(
			URTReplayRecorderLibrary::MatchDirectory(Root, Out.MatchId),
			URTReplayRecorderLibrary::TurnFileName(Turno));

		TArray<uint8> Byte;
		TArray<FRTTurnLogEntry> Voci;
		// ⚠️ **Una lettura fallita si DICHIARA, non si salta.** Saltandola gli array si compattano, e da li'
		// in poi `Voci[i]` non e' piu' il turno `i+1`: il confronto direbbe «turno 3» guardando il 4. E
		// `Byte.Num()` diventerebbe «file che si sono letti» invece di «turni giocati», cioe' un archivio
		// rotto sarebbe indistinguibile da una divergenza vera.
		if (!FFileHelper::LoadFileToArray(Byte, *Path)
			|| !URTTurnLogLibrary::LoadTurnLogFromFile(Path, Voci))
		{
			Out.bTutteLette = false;
			break;
		}
		Out.Byte.Add(MoveTemp(Byte));
		Out.Voci.Add(MoveTemp(Voci));
	}

	DestroyReplayProducerWorld(World);
	PuliscIProducer(Root);
	return true;
}

/**
 * Le guardie comuni ai due test del determinismo: una partita che non e' finita, o che ha perso un turno,
 * non e' un ingresso valido per nessuno dei due confronti.
 *
 * 🔴 **`PlayToCompletion` esce in silenzio al tetto dei 40 turni**, e `PlayOneTurn` si arrende dopo 400 tick
 * con la risoluzione ancora aperta. Senza queste righe, un giorno in cui le partite smettessero di decidersi
 * si confronterebbero tre archivi tronchi e il test riporterebbe «determinismo» su tre partite mai finite.
 */
bool ArchivioUtilizzabile(FAutomationTestBase& Test, const TCHAR* Nome, const FRTPartitaArchiviata& P)
{
	bool bOk = Test.TestTrue(*FString::Printf(TEXT("%s: la partita si e' DECISA, non e' finita al tetto"), Nome), P.bFinita);
	bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: ogni turno giocato ha il suo file"), Nome), P.bTutteLette);
	bOk &= Test.TestEqual(*FString::Printf(TEXT("%s: e i turni letti sono quelli giocati"), Nome),
		P.Byte.Num(), P.TurniGiocati);
	// ⚠️ Anti-vacuita' del VOLUME: una partita di un turno solo renderebbe il confronto quasi muto.
	bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: e i turni sono piu' di uno"), Nome), P.TurniGiocati > 1);
	return bOk;
}
}

/**
 * 🔑 **L'archivio permette di RIPRODURRE il determinismo, non solo di raccontarlo** — l'AC di
 * [#1805](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1805) *«stesso stato/input/regole →
 * stesso esito»*.
 *
 * ⚠️ **Sembrava coperta e non lo era.** `Replay.Verifier.ReportsFirstDivergence` esiste e nomina turno, fase
 * e `ActionId` — ma il suo stesso commento dichiara il confine: *«Il confronto e' fra due tracce. Chi
 * produce la seconda ri-simulando e' il chiamante»*. Nessuno era quel chiamante. E il corpus golden non
 * copre questo: le sue referenze sono file `.rttl` **committati**, non archivi **prodotti da una partita**.
 *
 * Qui l'anello si chiude: due partite allestite identiche, **entrambe registrate**, e gli archivi
 * riconfrontati **da disco** — non dagli array in memoria, che proverebbero che il `TurnManager` ha una
 * variabile.
 *
 * 🔴 **Il confronto e' sui BYTE**, per la ragione scritta su `FRTPartitaArchiviata`: l'hash non guarda
 * `UnitId` e altri quattro campi che pero' finiscono su disco.
 *
 * ⚠️ **Il caso in cui l'archivio DEVE distinguere vive nel suo test**, `AChangedInputMakesTheArchiveDiverge`:
 * un braccio che fallisce dietro il `return` di un altro non e' un braccio che si misura.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayDeterminismFromArchiveTest,
	"RefactorTactics.Replay.Producer.TwoIdenticalMatchesLeaveIdenticalArchives",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayDeterminismFromArchiveTest::RunTest(const FString&)
{
	FRTPartitaArchiviata A, B;
	if (!GiocaEArchivia(*this, TEXT("DetA"), nullptr, A)) { return false; }
	if (!GiocaEArchivia(*this, TEXT("DetB"), nullptr, B)) { return false; }
	if (!ArchivioUtilizzabile(*this, TEXT("A"), A) || !ArchivioUtilizzabile(*this, TEXT("B"), B)) { return false; }

	if (!TestEqual(TEXT("le due partite hanno prodotto lo stesso numero di turni"), B.Byte.Num(), A.Byte.Num()))
	{
		return false;
	}

	int32 VociConfrontate = 0;
	for (int32 i = 0; i < A.Byte.Num(); ++i)
	{
		// I BYTE decidono; le voci servono solo a dire DOVE quando i byte non coincidono.
		if (A.Byte[i] != B.Byte[i])
		{
			const FString Dove = URTTurnLogLibrary::DescribeFirstDivergence(i + 1, A.Voci[i], B.Voci[i]);
			AddError(FString::Printf(
				TEXT("turno %d: i due archivi differiscono su disco (%d vs %d byte). %s"),
				i + 1, A.Byte[i].Num(), B.Byte[i].Num(),
				Dove.IsEmpty() ? TEXT("Nessuna voce diverge per hash: la differenza e' in un campo che l'hash non guarda.") : *Dove));
		}
		VociConfrontate += A.Voci[i].Num();
	}
	// ⚠️ Anti-vacuita' delle VOCI: turni vuoti si confrontano uguali senza dire niente.
	TestTrue(TEXT("il confronto ha guardato delle voci"), VociConfrontate > 0);

	AddInfo(FString::Printf(TEXT("determinismo: %d turni, %d voci, archivi byte-identici"),
		A.Byte.Num(), VociConfrontate));
	return true;
}

/**
 * 🔑 **L'anti-vacuita' del test qui sopra, e vive separata perche' e' una claim separata**: un confronto fra
 * due cose uguali resta verde anche se il confronto non guarda.
 *
 * La partita cambia **un solo** ingresso — la cella di partenza di un'unita' — e l'archivio deve
 * distinguerla, **dicendo dove**. Il messaggio e' quello che il DoD di `#178` pretende: turno, fase,
 * `ActionId`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayArchiveDistinguishesInputTest,
	"RefactorTactics.Replay.Producer.AChangedInputMakesTheArchiveDiverge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayArchiveDistinguishesInputTest::RunTest(const FString&)
{
	const FRTCellId Perturbata(-3, 2);

	FRTPartitaArchiviata A, C;
	if (!GiocaEArchivia(*this, TEXT("DetRif"), nullptr, A)) { return false; }
	if (!GiocaEArchivia(*this, TEXT("DetPert"), &Perturbata, C)) { return false; }
	if (!ArchivioUtilizzabile(*this, TEXT("riferimento"), A)
		|| !ArchivioUtilizzabile(*this, TEXT("perturbata"), C))
	{
		return false;
	}

	// La divergenza si cerca **prima nei byte**, che e' la misura, e poi si NOMINA con le voci.
	int32 TurnoDiverso = INDEX_NONE;
	const int32 Comuni = FMath::Min(A.Byte.Num(), C.Byte.Num());
	for (int32 i = 0; i < Comuni && TurnoDiverso == INDEX_NONE; ++i)
	{
		if (A.Byte[i] != C.Byte[i]) { TurnoDiverso = i; }
	}

	if (!TestTrue(TEXT("una cella di partenza diversa produce un archivio diverso"),
			TurnoDiverso != INDEX_NONE || C.Byte.Num() != A.Byte.Num()))
	{
		return false;
	}

	// 🔴 **E la diagnosi si pretende, non si spera.** Con questa asserzione dentro un `if` sul risultato
	// precedente, il giorno in cui la perturbazione cambiasse solo la LUNGHEZZA della partita il test
	// resterebbe verde senza aver mai verificato che una divergenza dica DOVE — che e' l'unica cosa per cui
	// `DescribeFirstDivergence` esiste.
	if (!TestTrue(TEXT("e la divergenza cade in un turno che ENTRAMBE le partite hanno giocato"),
			TurnoDiverso != INDEX_NONE))
	{
		return false;
	}

	const FString Dove = URTTurnLogLibrary::DescribeFirstDivergence(
		TurnoDiverso + 1, A.Voci[TurnoDiverso], C.Voci[TurnoDiverso]);

	// Il formato e' «turno N, voce M: fase F, azione '...' — ...». Si pinna dall'INIZIO invece di cercare tre
	// parole dentro la riga intera: quelle parole compaiono anche nella descrizione delle due voci, quindi
	// un `Contains` resterebbe verde su un'intestazione riscritta — cioe' proprio quando il DoD si rompe.
	TestTrue(*FString::Printf(TEXT("e la divergenza dice DOVE: %s"), *Dove),
		Dove.StartsWith(FString::Printf(TEXT("turno %d, voce "), TurnoDiverso + 1))
		&& Dove.Contains(TEXT(": fase ")) && Dove.Contains(TEXT("azione ")));

	AddInfo(FString::Printf(TEXT("distinzione: riferimento %d turni, perturbata %d; prima divergenza al turno %d — %s"),
		A.Byte.Num(), C.Byte.Num(), TurnoDiverso + 1, *Dove));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
