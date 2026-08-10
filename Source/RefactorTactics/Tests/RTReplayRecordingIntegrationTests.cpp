#include "Misc/AutomationTest.h"
#include "Replay/RTReplayRecorderLibrary.h"
#include "Replay/RTReplayManifest.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"

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
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();

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

	void PlayOneRecTurn(ARTTurnManager* TM)
	{
		TM->PlanBotsForTest();
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
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

	SpawnRecUnit(World, 0, URTHeroCatalogLibrary::MakeVektor(), FRTCellId(-2, 0, 0));
	SpawnRecUnit(World, 1, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(2, 0, 0));

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("TurnManager"), TM)) { DestroyRecWorld(World); return false; }

	TM->ReplaysRootOverride = Root; // configurazione, non un ramo «se test»: la radice e' un parametro
	TM->DispatchBeginPlay();

	TestFalse(TEXT("dopo il solo BeginPlay la registrazione NON e' partita"),
		TM->GetReplayMatchId().IsValid());

	// Quello che fa il GameMode dopo aver risolto il formato: e' li' che si sa di stare allestendo una
	// partita vera, e non nel BeginPlay di un attore che anche i test spawnano.
	TM->BeginReplayRecording();

	const FGuid MatchId = TM->GetReplayMatchId();
	TestTrue(TEXT("ora la partita ha un id"), MatchId.IsValid());

	PlayOneRecTurn(TM);
	PlayOneRecTurn(TM);

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
		PlayOneRecTurn(TM);
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
	SpawnRecUnit(World, 0, URTHeroCatalogLibrary::MakeVektor(), FRTCellId(-2, 0, 0));
	SpawnRecUnit(World, 1, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(2, 0, 0));

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("TurnManager"), TM)) { DestroyRecWorld(World); return false; }

	TM->ReplaysRootOverride = Root;
	TM->DispatchBeginPlay();   // e nessuna `BeginReplayRecording`: e' il caso di ogni test del repository

	PlayOneRecTurn(TM);
	PlayOneRecTurn(TM);

	TestFalse(TEXT("nessun id di registrazione"), TM->GetReplayMatchId().IsValid());
	TestFalse(TEXT("e nessuna cartella di archivi creata"), PF.DirectoryExists(*Root));

	DestroyRecWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
