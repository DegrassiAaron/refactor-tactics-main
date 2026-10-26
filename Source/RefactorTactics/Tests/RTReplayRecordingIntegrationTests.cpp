#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Replay/RTReplayRecorderLibrary.h"
#include "Replay/RTReplayManifest.h"
#include "Replay/RTMatchHistoryLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTMatchFormatData.h"
#include "Turn/RTMatchFormatLibrary.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTReplayRecording.h"
#include "Replay/RTReplayStateLibrary.h"
#include "Unit/RTUnit.h"
#include "RTGameMode.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"
#include "Tests/RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Una partita vera lascia un archivio su disco (`#469`).
 *
 * I test del recorder verificano il FORMATO su dati costruiti a mano; questo verifica il **cablaggio**: che
 * il turno reale, risolto dal resolver, arrivi fino al file. E' la differenza fra «la libreria sa scrivere»
 * e «il gioco scrive», che in questo repository e' gia' costata piu' di una feature dichiarata pronta.
 */
namespace
{
	// ⚠️ Nomi prefissati `Rec`: UE compila piu' `.cpp` in un'unica unita' di traduzione, e due helper
	// omonimi in namespace anonimi diversi diventano una ridefinizione — un difetto che compare e sparisce
	// col raggruppamento unity, quindi un build verde non dimostra che non ci sia.
	UWorld* MakeRecWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyRecWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	void SpawnRecMap(UWorld* World, int32 Radius)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
	}

	ARTUnit* SpawnRecUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = true;
		U->DispatchBeginPlay();
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayRecordingIntegrationTest,
	"RefactorTactics.Replay.Recording.AMatchLeavesAnArchive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayRecordingIntegrationTest::RunTest(const FString&)
{
	const FString Root = FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("RecIntegration"));
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	if (PF.DirectoryExists(*Root)) { PF.DeleteDirectoryRecursively(*Root); }

	UWorld* World = MakeRecWorld();
	if (!TestNotNull(TEXT("mondo creato"), World)) { return false; }

	SpawnRecMap(World, /*Radius=*/ 4);

	SpawnRecUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-2, 0, 0));
	SpawnRecUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 0, 0));

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("TurnManager"), TM)) { DestroyRecWorld(World); return false; }

	TM->ReplaysRootOverride = Root; // configurazione, non un ramo «se test»: la radice e' un parametro
	TM->DispatchBeginPlay();

	TestFalse(TEXT("dopo il solo BeginPlay la registrazione NON e' partita"),
		TM->GetReplayMatchId().IsValid());

	// Quello che fa il GameMode dopo aver risolto il formato: e' li' che si sa di stare allestendo una
	// partita vera, e non nel BeginPlay di un attore che anche i test spawnano. Il formato si imposta prima,
	// come fa `ApplyMatchFormat`: senza, `BeginReplayRecording` si rifiuta — e giustamente.
	FRTMatchRules Rules;
	Rules.FormatId = URTMatchFormatLibrary::Skirmish2v2FormatId;
	TM->SetMatchRules(Rules);
	TM->BeginReplayRecording();

	const FGuid MatchId = TM->GetReplayMatchId();
	TestTrue(TEXT("ora la partita ha un id"), MatchId.IsValid());

	RTWorldFixtures::PlayOneTurn(TM);
	RTWorldFixtures::PlayOneTurn(TM);

	// Le tracce dei due turni sono su disco, col nome che il recorder dichiara.
	const FString Dir = URTReplayRecorderLibrary::MatchDirectory(Root, MatchId);
	TestTrue(TEXT("la traccia del turno 1 esiste"),
		PF.FileExists(*FPaths::Combine(Dir, URTReplayRecorderLibrary::TurnFileName(1))));
	TestTrue(TEXT("e quella del turno 2"),
		PF.FileExists(*FPaths::Combine(Dir, URTReplayRecorderLibrary::TurnFileName(2))));

	// Il manifest c'e' gia' A PARTITA IN CORSO, e dice di non essere chiuso: e' il caso che rende utile
	// l'archivio quando il gioco muore a meta'.
	FRTReplayManifest Manifest;
	if (!TestTrue(TEXT("il manifest si rilegge"),
		URTReplayRecorderLibrary::LoadManifest(Root, MatchId, Manifest)))
	{
		DestroyRecWorld(World);
		return false;
	}
	TestFalse(TEXT("la partita e' ancora in corso, quindi non e' chiuso"), Manifest.bClosed);
	TestEqual(TEXT("due turni registrati"), Manifest.TurnCount, 2);

	// La traccia su disco e' quella che il gioco ha davvero prodotto: stesso hash ordinato.
	if (!TestEqual(TEXT("due hash"), Manifest.OrderedHashPerTurn.Num(), 2))
	{
		DestroyRecWorld(World);
		return false;
	}
	TestNotEqual(TEXT("e non sono a zero: una partita vera ha prodotto voci"),
		Manifest.OrderedHashPerTurn[0], (int64)0);

	// Il checksum di stato esiste gia' a partita in corso: si cattura a ogni turno.
	TestNotEqual(TEXT("il checksum di stato e' stato calcolato, e non e' zero"),
		TM->GetPendingFinalStateHash(), (int64)0);

	// E ora fino alla FINE, perche' l'acceptance criterion di #469 parla del checksum **nel manifest** di
	// una partita reale — non di un valore in memoria a meta' strada. Verificare il proxy sarebbe stato
	// piu' comodo e avrebbe lasciato scoperto proprio il tratto che chiude la issue.
	int32 Giri = 0;
	while (TM->GetPhase() != ERTMatchPhase::MatchEnded && Giri < 60)
	{
		RTWorldFixtures::PlayOneTurn(TM);
		++Giri;
	}
	if (!TestTrue(TEXT("la partita si decide entro il limite"), TM->GetPhase() == ERTMatchPhase::MatchEnded))
	{
		DestroyRecWorld(World);
		return false;
	}

	FRTReplayManifest Chiuso;
	if (!TestTrue(TEXT("il manifest della partita finita si rilegge"),
		URTReplayRecorderLibrary::LoadManifest(Root, MatchId, Chiuso)))
	{
		DestroyRecWorld(World);
		return false;
	}
	TestTrue(TEXT("ora l'archivio si dichiara chiuso"), Chiuso.bClosed);
	TestNotEqual(TEXT("e porta il checksum di fine partita, calcolato da una partita vera"),
		Chiuso.FinalStateHash, (int64)0);
	TestNotEqual(TEXT("con un esito, non «in corso»"), Chiuso.Outcome, ERTMatchOutcome::InProgress);

	DestroyRecWorld(World);
	if (PF.DirectoryExists(*Root)) { PF.DeleteDirectoryRecursively(*Root); }
	return true;
}

/**
 * 🔴 **La riga della lista delle partite si completa alla CHIUSURA** (`#3859`).
 *
 * `AppendOrUpdate` scrive la riga di `history.rtindex` quando la partita comincia e la aggiorna **in luogo** quando
 * finisce: e' l'indice da cui la schermata `MatchHistory` costruisce la lista, e non apre le cartelle degli archivi.
 * Dal 2026-09-04 (`0e636014f`, `#2286`) la seconda chiamata mancava: `FRTReplayRecording::Close` chiudeva il
 * manifest e basta, e ogni riga restava **«in corso, 0 turni, registrazione incompleta»** anche a partita finita.
 *
 * ⛔ **Perche' nessun test se n'era accorto.** `RTMatchHistoryTests` costruisce la riga con `AppendOrUpdate` scritto
 * a mano dopo `CloseMatch`: pinna la libreria, e non il collegamento. Questo test passa dal **percorso di produzione**
 * — `ARTTurnManager` → `FRTReplayRecording::Close` → indice — e rilegge la riga dal file, non dalla memoria.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayHistoryRowCompletedAtCloseTest,
	"RefactorTactics.Replay.Recording.TheHistoryRowIsCompletedWhenTheMatchEnds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayHistoryRowCompletedAtCloseTest::RunTest(const FString&)
{
	const FString Root = FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("RecHistoryRow"));
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	if (PF.DirectoryExists(*Root)) { PF.DeleteDirectoryRecursively(*Root); }

	UWorld* World = MakeRecWorld();
	if (!TestNotNull(TEXT("mondo creato"), World)) { return false; }

	SpawnRecMap(World, /*Radius=*/ 4);
	SpawnRecUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-2, 0, 0));
	SpawnRecUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 0, 0));

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("TurnManager"), TM)) { DestroyRecWorld(World); return false; }

	TM->ReplaysRootOverride = Root;
	TM->DispatchBeginPlay();

	FRTMatchRules Rules;
	Rules.FormatId = URTMatchFormatLibrary::Skirmish2v2FormatId;
	TM->SetMatchRules(Rules);
	TM->BeginReplayRecording();

	const FGuid MatchId = TM->GetReplayMatchId();
	if (!TestTrue(TEXT("la partita ha un id"), MatchId.IsValid())) { DestroyRecWorld(World); return false; }

	// Rilegge l'indice DAL FILE e cerca la riga della partita: e' cio' che farebbe la schermata.
	auto RigaDellaPartita = [&Root, &MatchId](int32& OutQuante, FRTMatchHistoryEntry& OutRiga) -> bool
	{
		TArray<FRTMatchHistoryEntry> Righe;
		OutQuante = 0;
		if (!URTMatchHistoryLibrary::LoadIndex(Root, Righe)) { return false; }
		for (const FRTMatchHistoryEntry& R : Righe)
		{
			if (R.MatchId == MatchId) { ++OutQuante; OutRiga = R; }
		}
		return OutQuante > 0;
	};

	// A partita in corso la riga c'e' gia' (`#416`: una partita interrotta deve comparire) e dice «in corso».
	int32 Quante = 0;
	FRTMatchHistoryEntry InCorso;
	if (!TestTrue(TEXT("a partita iniziata la riga e' gia' nell'indice"), RigaDellaPartita(Quante, InCorso)))
	{
		DestroyRecWorld(World);
		return false;
	}
	TestTrue(TEXT("e dice «in corso»"), InCorso.Outcome == ERTMatchOutcome::InProgress);
	TestFalse(TEXT("col replay non ancora completo"), InCorso.bReplayComplete);

	int32 Giri = 0;
	while (TM->GetPhase() != ERTMatchPhase::MatchEnded && Giri < 60)
	{
		RTWorldFixtures::PlayOneTurn(TM);
		++Giri;
	}
	if (!TestTrue(TEXT("la partita si decide entro il limite"), TM->GetPhase() == ERTMatchPhase::MatchEnded))
	{
		DestroyRecWorld(World);
		return false;
	}

	FRTReplayManifest Chiuso;
	if (!TestTrue(TEXT("il manifest della partita finita si rilegge"),
		URTReplayRecorderLibrary::LoadManifest(Root, MatchId, Chiuso)))
	{
		DestroyRecWorld(World);
		return false;
	}
	// Premesse misurate: senza, il confronto sotto confronterebbe due «zero».
	TestTrue(TEXT("premessa: il manifest e' chiuso"), Chiuso.bClosed);
	TestTrue(TEXT("premessa: il manifest ha un esito"), Chiuso.Outcome != ERTMatchOutcome::InProgress);
	TestTrue(TEXT("premessa: il manifest ha dei turni"), Chiuso.TurnCount > 0);

	FRTMatchHistoryEntry Finita;
	if (!TestTrue(TEXT("a partita finita la riga c'e' ancora"), RigaDellaPartita(Quante, Finita)))
	{
		DestroyRecWorld(World);
		return false;
	}
	TestEqual(TEXT("ed e' UNA sola: la chiusura aggiorna la riga in luogo, non ne accoda una seconda"), Quante, 1);
	TestTrue(TEXT("la riga porta lo stesso esito del manifest"), Finita.Outcome == Chiuso.Outcome);
	TestEqual(TEXT("e gli stessi turni"), Finita.TurnCount, Chiuso.TurnCount);
	TestTrue(TEXT("e dichiara il replay completo"), Finita.bReplayComplete);
	TestEqual(TEXT("la data d'inizio non cambia con la chiusura"), Finita.StartedUtc, InCorso.StartedUtc);

	DestroyRecWorld(World);
	if (PF.DirectoryExists(*Root)) { PF.DeleteDirectoryRecursively(*Root); }
	return true;
}

/**
 * 🔴 **La riga della lista segue i TURNI mentre la partita gira, non solo alla chiusura** (`#3871`).
 *
 * `#3859` ha fatto completare la riga alla CHIUSURA. Resta la partita che alla chiusura non arriva — un processo
 * terminato, una cattura che si ferma a un numero di frame: il manifest si riscrive a ogni turno (`RecordTurn`), ma la
 * riga di `history.rtindex` restava com'era l'aveva scritta `Begin`, e la lista diceva «0 turni · interrotta» per
 * partite che ne avevano giocati tre o quattro. E' l'altra meta' della lamentela dell'autore sul passo 5 di `G16`
 * («nel replay appaiono partite non concluse o non salvate»): le interrotte restano elencate (`#416`), ma devono dire
 * la verita' su quanto sono arrivate.
 *
 * Il test passa dal **percorso di produzione**, come quello di `#3859` — `ARTTurnManager` →
 * `FRTReplayRecording::RecordTurn` → indice — e rilegge la riga dal FILE, a partita in corso, **senza chiuderla**.
 *
 * Le gambe: (a) dopo un turno la riga dice 1, e il manifest lo conferma (la premessa: senza, si confronterebbero due
 * zeri); (b) dopo il secondo dice 2; (c) la riga resta UNA sola; (d) resta «in corso» e con il replay non completo: i
 * turni non fingono una chiusura.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayHistoryRowFollowsTheTurnsTest,
	"RefactorTactics.Replay.Recording.TheHistoryRowFollowsTheTurnsWhileTheMatchRuns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayHistoryRowFollowsTheTurnsTest::RunTest(const FString&)
{
	const FString Root = FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("RecHistoryTurns"));
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	if (PF.DirectoryExists(*Root)) { PF.DeleteDirectoryRecursively(*Root); }

	UWorld* World = MakeRecWorld();
	if (!TestNotNull(TEXT("mondo creato"), World)) { return false; }

	SpawnRecMap(World, /*Radius=*/ 4);
	SpawnRecUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-2, 0, 0));
	SpawnRecUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 0, 0));

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("TurnManager"), TM)) { DestroyRecWorld(World); return false; }

	TM->ReplaysRootOverride = Root;
	TM->DispatchBeginPlay();

	FRTMatchRules Rules;
	Rules.FormatId = URTMatchFormatLibrary::Skirmish2v2FormatId;
	TM->SetMatchRules(Rules);
	TM->BeginReplayRecording();

	const FGuid MatchId = TM->GetReplayMatchId();
	if (!TestTrue(TEXT("la partita ha un id"), MatchId.IsValid())) { DestroyRecWorld(World); return false; }

	// Rilegge l'indice DAL FILE e cerca la riga della partita: e' cio' che farebbe la schermata.
	auto RigaDellaPartita = [&Root, &MatchId](int32& OutQuante, FRTMatchHistoryEntry& OutRiga) -> bool
	{
		TArray<FRTMatchHistoryEntry> Righe;
		OutQuante = 0;
		if (!URTMatchHistoryLibrary::LoadIndex(Root, Righe)) { return false; }
		for (const FRTMatchHistoryEntry& R : Righe)
		{
			if (R.MatchId == MatchId) { ++OutQuante; OutRiga = R; }
		}
		return OutQuante > 0;
	};

	// Un turno alla volta, SENZA arrivare alla fine: la partita resta aperta, come una cattura interrotta.
	for (int32 Atteso = 1; Atteso <= 2; ++Atteso)
	{
		RTWorldFixtures::PlayOneTurn(TM);
		if (!TestTrue(TEXT("premessa: la partita non e' finita, e il test non la chiude"),
			TM->GetPhase() != ERTMatchPhase::MatchEnded))
		{
			DestroyRecWorld(World);
			return false;
		}

		FRTReplayManifest Manifest;
		if (!TestTrue(TEXT("il manifest a partita aperta si rilegge"), URTReplayRecorderLibrary::LoadManifest(Root, MatchId, Manifest)))
		{
			DestroyRecWorld(World);
			return false;
		}
		// Premesse misurate: il manifest e' gia' al turno atteso e NON e' chiuso.
		TestEqual(TEXT("premessa: il manifest conta i turni giocati"), Manifest.TurnCount, Atteso);
		TestFalse(TEXT("premessa: il manifest non e' chiuso"), Manifest.bClosed);

		int32 Quante = 0;
		FRTMatchHistoryEntry Riga;
		if (!TestTrue(TEXT("la riga e' nell'indice"), RigaDellaPartita(Quante, Riga)))
		{
			DestroyRecWorld(World);
			return false;
		}
		TestEqual(*FString::Printf(TEXT("(a)(b) dopo %d turni la riga dice %d, come il manifest"), Atteso, Atteso), Riga.TurnCount, Manifest.TurnCount);
		TestEqual(TEXT("(c) ed e' UNA sola: ogni turno aggiorna la riga in luogo"), Quante, 1);
		TestTrue(TEXT("(d) resta «in corso»"), Riga.Outcome == ERTMatchOutcome::InProgress);
		TestFalse(TEXT("(d) e col replay non completo: i turni non fingono una chiusura"), Riga.bReplayComplete);
	}

	DestroyRecWorld(World);
	if (PF.DirectoryExists(*Root)) { PF.DeleteDirectoryRecursively(*Root); }
	return true;
}

/**
 * 🔴 **La riga legge il MANIFEST, non il contatore di chi chiama** (`#3871`).
 *
 * Il test sopra gioca turni veri, dove il numero del turno e i turni del manifest coincidono sempre: non distingue una
 * riga che copia i turni dal manifest da una che copia `TurnNumber`. Qui si passa dalla struct di produzione
 * (`FRTReplayRecording`, esercitabile senza un mondo) e si forzano i due casi in cui le due letture divergono:
 * (a) un turno che il registratore RIFIUTA (saltato, ripetuto) non muove il manifest, e la riga non deve avanzare;
 * (b) dopo la chiusura la riga resta quella del manifest — esito e replay completo — anche se qualcuno chiama ancora
 * `RecordTurn`: la riga non e' un terzo stato, e' sempre `EntryFromManifest` di quello che il manifest dice ADESSO.
 *
 * Il confronto e' contro `GetManifest()`, mai contro un numero scritto a mano: il test pinna l'accoppiamento riga-manifest,
 * non una particolarita' del registratore (che accetti o no un turno dopo la chiusura non e' affare di questa issue).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayHistoryRowReadsTheManifestTest,
	"RefactorTactics.Replay.Recording.TheHistoryRowReadsTheManifestNotTheCaller",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayHistoryRowReadsTheManifestTest::RunTest(const FString&)
{
	const FString Root = FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("RecHistoryManifest"));
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	if (PF.DirectoryExists(*Root)) { PF.DeleteDirectoryRecursively(*Root); }

	FRTReplayRecording Rec;
	Rec.Begin(URTMatchFormatLibrary::Skirmish2v2FormatId, TArray<int32>{ 0, 1 }, /*LocalObserverTeamId=*/ 0, Root);
	const FGuid MatchId = Rec.GetMatchId();
	TestTrue(TEXT("la registrazione ha un id"), MatchId.IsValid());

	// Rilegge l'indice DAL FILE e confronta la riga della partita con il manifest IN MEMORIA di chi registra.
	auto LaRigaDiceCioCheIlManifest = [this, &Root, &MatchId, &Rec](const TCHAR* Quando)
	{
		TArray<FRTMatchHistoryEntry> Righe;
		if (!TestTrue(*FString::Printf(TEXT("%s: l'indice si rilegge"), Quando), URTMatchHistoryLibrary::LoadIndex(Root, Righe)))
		{
			return;
		}
		const FRTMatchHistoryEntry* Riga = Righe.FindByPredicate(
			[&MatchId](const FRTMatchHistoryEntry& R) { return R.MatchId == MatchId; });
		if (!TestNotNull(*FString::Printf(TEXT("%s: la riga c'e'"), Quando), Riga))
		{
			return;
		}
		const FRTReplayManifest& Manifest = Rec.GetManifest();
		TestEqual(*FString::Printf(TEXT("%s: i turni della riga sono quelli del manifest"), Quando), Riga->TurnCount, Manifest.TurnCount);
		TestTrue(*FString::Printf(TEXT("%s: l'esito della riga e' quello del manifest"), Quando), Riga->Outcome == Manifest.Outcome);
		TestEqual(*FString::Printf(TEXT("%s: il replay e' completo se e solo se il manifest e' chiuso"), Quando), Riga->bReplayComplete, Manifest.bClosed);
	};

	const TArray<FRTTurnLogEntry> Vuoto;

	TestTrue(TEXT("il turno 1 si registra"), Rec.RecordTurn(Root, 1, Vuoto));
	LaRigaDiceCioCheIlManifest(TEXT("dopo il turno 1"));

	// (a) Un turno SALTATO e uno RIPETUTO sono rifiutati: il manifest resta a 1, e la riga con lui. Una riga che copiasse il
	// numero passato da chi chiama direbbe 3: e' la gamba che la uccide.
	TestFalse(TEXT("(a) un turno saltato e' rifiutato"), Rec.RecordTurn(Root, 3, Vuoto));
	TestEqual(TEXT("(a) premessa: il manifest non si e' mosso"), Rec.GetManifest().TurnCount, 1);
	LaRigaDiceCioCheIlManifest(TEXT("(a) dopo un turno saltato"));

	TestFalse(TEXT("(a) un turno ripetuto e' rifiutato"), Rec.RecordTurn(Root, 1, Vuoto));
	LaRigaDiceCioCheIlManifest(TEXT("(a) dopo un turno ripetuto"));

	TestTrue(TEXT("il turno 2 si registra"), Rec.RecordTurn(Root, 2, Vuoto));
	LaRigaDiceCioCheIlManifest(TEXT("dopo il turno 2"));

	// (b) Chiusa la partita, la riga e' quella del manifest chiuso: esito dichiarato, replay completo.
	TestTrue(TEXT("la partita si chiude"), Rec.Close(Root, ERTMatchOutcome::Draw, /*FinalStateHash=*/ 0));
	TestTrue(TEXT("premessa: il manifest e' chiuso e ha un esito"),
		Rec.GetManifest().bClosed && Rec.GetManifest().Outcome == ERTMatchOutcome::Draw);
	LaRigaDiceCioCheIlManifest(TEXT("(b) dopo la chiusura"));

	// ...e un `RecordTurn` che arriva DOPO non la riporta «in corso»: la riga legge il manifest, che e' ancora chiuso.
	Rec.RecordTurn(Root, 3, Vuoto);
	TestTrue(TEXT("premessa: il manifest e' ancora chiuso"), Rec.GetManifest().bClosed);
	LaRigaDiceCioCheIlManifest(TEXT("(b) dopo un turno tardivo"));

	if (PF.DirectoryExists(*Root)) { PF.DeleteDirectoryRecursively(*Root); }
	return true;
}

/**
 * Un TurnManager che nessuno ha avviato NON scrive niente.
 *
 * E' il caso dei 27 file di test e dello `ScenarioHarness`, che spawnano un TurnManager a mano: se la
 * registrazione partisse dal `BeginPlay`, ognuno di loro lascerebbe archivi in `Saved/Replays` a ogni run —
 * un effetto collaterale che nessuno ha chiesto, e che si scopre solo guardando la cartella. Il difetto
 * c'era davvero: quattro archivi trovati dopo una singola esecuzione della suite.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayNoRecordingWithoutStartTest,
	"RefactorTactics.Replay.Recording.UnstartedManagerWritesNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayNoRecordingWithoutStartTest::RunTest(const FString&)
{
	const FString Root = FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("RecUnstarted"));
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	if (PF.DirectoryExists(*Root)) { PF.DeleteDirectoryRecursively(*Root); }

	UWorld* World = MakeRecWorld();
	if (!TestNotNull(TEXT("mondo creato"), World)) { return false; }

	SpawnRecMap(World, /*Radius=*/ 4);
	SpawnRecUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-2, 0, 0));
	SpawnRecUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 0, 0));

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("TurnManager"), TM)) { DestroyRecWorld(World); return false; }

	TM->ReplaysRootOverride = Root;
	TM->DispatchBeginPlay();   // e nessuna `BeginReplayRecording`: e' il caso di ogni test del repository

	RTWorldFixtures::PlayOneTurn(TM);
	RTWorldFixtures::PlayOneTurn(TM);

	TestFalse(TEXT("nessun id di registrazione"), TM->GetReplayMatchId().IsValid());
	TestFalse(TEXT("e nessuna cartella di archivi creata"), PF.DirectoryExists(*Root));

	DestroyRecWorld(World);
	return true;
}

/**
 * Allestire una partita NON e' avviarla: `SetupHexMatch` chiamata da sola non registra niente.
 *
 * Il difetto e' gia' tornato una volta. La prima correzione aveva spostato l'avvio della registrazione dal
 * `BeginPlay` del TurnManager dentro `SetupHexMatch`, credendo che quello fosse «il punto in cui si sa che
 * e' una partita vera». Non lo era: `RTHeroSpawnTests` chiama `SetupHexMatch` **direttamente**, perche'
 * verifica lo spawn del roster attraverso il percorso vero — e cosi' due test lasciavano un
 * `history.rtindex` nella `Saved/` del progetto a ogni esecuzione della suite.
 *
 * Ora l'avvio sta dopo `SetupHexMatch` dentro `BeginPlay`, dove ci si arriva solo giocando. Questo test
 * pinna la distinzione, cosi' non serve accorgersene una terza volta guardando una cartella.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplaySetupWithoutBeginPlayTest,
	"RefactorTactics.Replay.Recording.SetupAloneStartsNoRecording",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplaySetupWithoutBeginPlayTest::RunTest(const FString&)
{
	UWorld* World = MakeRecWorld();
	if (!TestNotNull(TEXT("mondo creato"), World)) { return false; }

	SpawnRecMap(World, /*Radius=*/ 4);

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TM || !GameMode) { DestroyRecWorld(World); return false; }

	ARTHexMapActor* HexMap = Cast<ARTHexMapActor>(
		UGameplayStatics::GetActorOfClass(World, ARTHexMapActor::StaticClass()));
	if (!TestNotNull(TEXT("mappa nel mondo"), HexMap)) { DestroyRecWorld(World); return false; }

	// Il percorso di `RTHeroSpawnTests`: si allestisce, non si gioca.
	GameMode->SetupHexMatch(HexMap);

	TestFalse(TEXT("allestire non avvia la registrazione"), TM->GetReplayMatchId().IsValid());

	// E anche chiamandola a mano: senza un formato risolto non si registra. E' il caso in cui
	// `ApplyMatchFormat` fallisce e `SetupHexMatch` esce prima — il chiamante e' fuori da quella funzione e
	// non lo sa, quindi la guardia sta qui dentro e vale per ogni chiamante.
	FRTMatchRules SenzaFormato;
	TM->SetMatchRules(SenzaFormato); // `FormatId` di default e' `NAME_None`
	TM->BeginReplayRecording();
	TestFalse(TEXT("senza formato risolto non si registra"), TM->GetReplayMatchId().IsValid());

	DestroyRecWorld(World);
	return true;
}

/**
 * 🔴 **Il test 8 di `#2914`: la traccia regge quando un arco dura piu' di un micro-step.**
 *
 * `FRTTurnLogEntry::MicroStepIndex` e' **serializzato** (`ERTTurnLogFormatVersion::WithMicroStep`) e fa parte
 * del boundary `(TurnNumber, Phase, MicroStepIndex)` su cui il replay indicizza. Con [D-381] il numero di
 * micro-step di un turno non e' piu' il numero di celle percorse: e' la **somma dei costi d'ingresso**.
 *
 * ⚠️ **Nessuno dei tre test di replay esistenti esercita terreno costoso** — misurato: `MoveCost = [2-9]` da'
 * `0` occorrenze in `RTReplayPlayerTests`, `RTReplayProducerTests` e in questo file. Giravano tutti su
 * pavimento a costo `1`, dove un arco vale un micro-step e la distinzione non esiste.
 *
 * 🔑 **Cosa misura, e cosa NON misura.** Misura che una risoluzione con durate variabili produca una traccia
 * **rileggibile**: stesse celle finali ricostruite dalla traccia e dallo stato vivo. ⛔ Non misura il seek per
 * micro-step — quello ha i suoi test su tracce sintetiche — e non aggiunge lo scenario al corpus golden, che
 * resta una decisione con il suo costo di rigenerazione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayCostlyTerrainRoundTripTest,
	"RefactorTactics.Replay.Recording.CostlyTerrainSurvivesTheRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayCostlyTerrainRoundTripTest::RunTest(const FString&)
{
	UWorld* World = MakeRecWorld();
	if (!TestNotNull(TEXT("mondo creato"), World)) { return false; }

	// Arena con una cella CARA sul percorso: e' l'unica differenza rispetto agli altri test di replay.
	URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), /*Radius=*/ 4);
	const FRTCellId Costosa(0, 0, 0);
	int32 CelleCare = 0;
	for (FRTHexCellData& Cell : M->Cells)
	{
		if (Cell.Id == Costosa) { Cell.MoveCost = 3; ++CelleCare; }
	}
	// Premessa misurata: senza la cella cara questo test girerebbe come gli altri tre, e sarebbe un doppione.
	if (!TestEqual(TEXT("premessa: l'arena dichiara la cella cara"), CelleCare, 1))
	{
		DestroyRecWorld(World);
		return false;
	}
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = M;

	ARTUnit* Mover = SpawnRecUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-1, 0, 0));
	ARTUnit* Altro = SpawnRecUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(3, 0, 0));
	if (!TestNotNull(TEXT("mover"), Mover) || !TestNotNull(TEXT("avversario"), Altro))
	{
		DestroyRecWorld(World);
		return false;
	}
	Mover->bIsBotControlled = false;
	Altro->bIsBotControlled = false;

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("TurnManager"), TM)) { DestroyRecWorld(World); return false; }
	TM->DispatchBeginPlay();

	// Il mover attraversa la cella cara e prosegue: due archi, di cui uno da tre micro-step.
	Mover->PlannedPath = { FRTCellId(-1, 0, 0), Costosa, FRTCellId(1, 0, 0) };
	Mover->PlannedCell = FRTCellId(1, 0, 0);

	// ⚠️ `ResolveMovement` e' `protected`. La terna pubblica — `Begin`/`Advance`/`Finish` — **e' cio' che
	// quella funzione esegue**, e non una seconda implementazione: guidarla a mano da qui e' la stessa
	// risoluzione, con in piu' la possibilita' di contare i micro-step.
	TM->BeginMovementResolution();
	int32 MicroStep = 0;
	while (TM->AdvanceMovementResolution() == ERTMovementAdvanceResult::Advanced && MicroStep < 64)
	{
		++MicroStep;
	}
	TM->FinishMovementResolution();

	// 🔑 **La prova che le durate sono ATTIVE su questo percorso**: due archi, di cui uno su una cella a
	// costo 3, valgono piu' di due micro-step. Senza questa riga il round-trip qui sotto sarebbe verde
	// anche su una risoluzione che le ignora — cioe' misurerebbe il replay e non il caso che lo mette alla
	// prova.
	TestTrue(FString::Printf(TEXT("il costo del terreno allunga la risoluzione: %d micro-step per 2 archi"),
		MicroStep), MicroStep > 2);

	const TArray<FRTTurnLogEntry> Traccia = TM->GetTurnLog();
	TestTrue(TEXT("la risoluzione ha lasciato una traccia"), Traccia.Num() > 0);

	// 🔑 **Il round-trip**: lo stato ricostruito DALLA TRACCIA deve dire dove sta l'unita' quanto lo stato
	// vivo. Se la durata variabile rompesse l'indicizzazione, questa e' la riga che cade.
	// ⚠️ `UnitsAtEnd` ricostruisce PARTENDO da uno stato iniziale: la traccia porta i delta, non le posizioni
	// assolute. Lo stato iniziale e' dove le unita' stavano PRIMA della risoluzione.
	TArray<FRTTracedUnitState> Iniziale;
	{
		FRTTracedUnitState S;
		S.UnitId = Mover->StableUnitId;
		S.Cell = FRTCellId(-1, 0, 0);
		Iniziale.Add(S);
	}
	const TArray<FRTTracedUnitState> Ricostruito =
		URTReplayStateLibrary::UnitsAtEnd(Traccia, Iniziale);

	const FRTTracedUnitState* Stato = Ricostruito.FindByPredicate(
		[Mover](const FRTTracedUnitState& S) { return S.UnitId == Mover->StableUnitId; });

	if (TestNotNull(TEXT("la traccia conosce il mover"), Stato))
	{
		TestEqual(TEXT("la cella ricostruita e' quella viva"), Stato->Cell, Mover->Cell);
	}

	// ⚠️ **E il mover si e' MOSSO davvero**: senza questa riga il round-trip sarebbe verde anche su
	// un'unita' rimasta ferma, cioe' su una traccia che non racconta nulla.
	TestNotEqual(TEXT("premessa: non e' rimasto alla partenza"), Mover->Cell, FRTCellId(-1, 0, 0));

	DestroyRecWorld(World);
	return true;
}


/**
 * Il rifiuto di `BeginReplayRecording` si DICHIARA, e dice QUALE delle due guardie ha chiuso (`#3463`).
 *
 * Nasce da un difetto che si e' rivelato un difetto di MISURA: su un pacchetto una partita completa
 * sembrava non registrare, e la causa era che `FPaths::ProjectSavedDir()` di un build staged non e' il
 * `Saved/` del repository -- gli archivi c'erano, nel posto giusto, e chi guardava era nel posto
 * sbagliato. 🔑 Ma l'istruttoria ha trovato il buco vero: quel `return` era **muto**, quindi fra
 * «non ha registrato» e «ho guardato male» non c'era modo di distinguere dal log. Ora c'e'.
 *
 * ⛔ **FALSIFICAZIONE**: togliere uno dei due `UE_LOG` fa fallire questo test con *«Expected ('Display')
 * level log message ... but it was found 0 time(s)»*. Senza questa riga la casella «l'uscita non e' piu'
 * silenziosa» sarebbe un'affermazione invece di un fatto.
 *
 * ⚠️ **`AddExpectedMessagePlain` e non `AddExpectedMessage`**: il secondo interpreta il pattern come
 * **regex** per default, e i messaggi contengono `(` e `)` -- un pattern che non matcherebbe mai, cioe'
 * un test rosso per la ragione sbagliata.
 *
 * ⚠️ **E `Occurrences = 1`, non `-1`**: `-1` vuol dire «ignora», ed e' cosa fanno le due dichiarazioni
 * in `RTFrontendMainMenuTests` -- quelle **sopprimono** rumore, questa **asserisce**. Un `-1` qui
 * renderebbe il test verde anche a log cancellato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayRefusalIsDeclaredTest,
	"RefactorTactics.Replay.Recording.RefusalIsDeclared",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayRefusalIsDeclaredTest::RunTest(const FString&)
{
	const FString Root = FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("RecRefusal"));
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	if (PF.DirectoryExists(*Root)) { PF.DeleteDirectoryRecursively(*Root); }

	UWorld* World = MakeRecWorld();
	if (!TestNotNull(TEXT("mondo creato"), World)) { return false; }

	SpawnRecMap(World, /*Radius=*/ 4);
	SpawnRecUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-2, 0, 0));
	SpawnRecUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 0, 0));

	FRTMatchRules Rules;
	Rules.FormatId = URTMatchFormatLibrary::Skirmish2v2FormatId;

	// ⚠️ Le due dichiarazioni **non sono scopate** al blocco che le segue: vivono per tutto il `RunTest`.
	// Stanno accanto al loro caso per leggibilita', non perche' il framework le delimiti.
	AddExpectedMessagePlain(TEXT("il formato non e' risolto"), ELogVerbosity::Display,
		EAutomationExpectedMessageFlags::Contains, /*Occurrences=*/ 1);
	AddExpectedMessagePlain(TEXT("bRecordReplay e' false"), ELogVerbosity::Display,
		EAutomationExpectedMessageFlags::Contains, /*Occurrences=*/ 1);

	// --- guardia 1: nessun formato risolto, registrazione accesa ---
	ARTTurnManager* SenzaFormato = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("TurnManager senza formato"), SenzaFormato)) { DestroyRecWorld(World); return false; }
	SenzaFormato->ReplaysRootOverride = Root;
	SenzaFormato->BeginReplayRecording();
	TestFalse(TEXT("senza formato: nessun id di registrazione"), SenzaFormato->GetReplayMatchId().IsValid());

	// --- guardia 2: formato risolto, ma registrazione SPENTA ---
	// 🔑 Il formato c'e' **di proposito**: e' cio' che costringe la seconda guardia a essere quella che
	// risponde. Senza, i due messaggi sarebbero indistinguibili e `!A || B` tornerebbe a non dire quale.
	ARTTurnManager* Spenta = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("TurnManager a registrazione spenta"), Spenta)) { DestroyRecWorld(World); return false; }
	Spenta->ReplaysRootOverride = Root;
	Spenta->SetMatchRules(Rules);
	Spenta->bRecordReplay = false;
	Spenta->BeginReplayRecording();
	TestFalse(TEXT("a registrazione spenta: nessun id"), Spenta->GetReplayMatchId().IsValid());

	// --- controllo POSITIVO ---
	// ⛔ Senza questa parte i due `TestFalse` sopra sarebbero verdi anche se `BeginReplayRecording` si
	// rifiutasse **sempre**, cioe' su una funzione completamente rotta.
	ARTTurnManager* Buona = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("TurnManager in regola"), Buona)) { DestroyRecWorld(World); return false; }
	Buona->ReplaysRootOverride = Root;
	Buona->SetMatchRules(Rules);
	Buona->BeginReplayRecording();
	TestTrue(TEXT("controllo positivo: con formato e registrazione accesa l'id esiste"),
		Buona->GetReplayMatchId().IsValid());

	DestroyRecWorld(World);
	if (PF.DirectoryExists(*Root)) { PF.DeleteDirectoryRecursively(*Root); }
	return true;
}


#endif // WITH_DEV_AUTOMATION_TESTS
