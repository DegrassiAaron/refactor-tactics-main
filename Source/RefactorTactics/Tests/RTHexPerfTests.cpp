#include "Misc/AutomationTest.h"
#include "Pathfinding/RTHexPathLibrary.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "HAL/PlatformTime.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"

/**
 * Misura dei budget di performance (KPI del PDR: path < 2 ms, resolver < 100 ms/turno).
 *
 * Questi test **misurano e registrano**: le soglie assertite sono volutamente larghe (10x il target), perche'
 * un test che fallisse al primo rumore di scheduling verrebbe disabilitato dopo due settimane — e un test
 * disabilitato non misura niente. La soglia larga cattura la regressione catastrofica (un A* che diventa
 * quadratico, un resolver che ricalcola tutto per micro-step); il NUMERO stampato nel log e' il dato che
 * finisce nella tabella KPI.
 *
 * I valori misurati vanno letti come ordine di grandezza su questa macchina, non come garanzia: il PC target
 * non e' mai stato definito (canone §9).
 */
namespace
{
	URTHexMapAsset* MakePerfMap(int32 Radius)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();
		return M;
	}

	UWorld* MakePerfWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyPerfWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Mediana in millisecondi di un campione di durate (mediana, non media: un picco non deve spostarla). */
	double MedianMs(TArray<double>& Samples)
	{
		if (Samples.Num() == 0) { return 0.0; }
		Samples.Sort();
		return Samples[Samples.Num() / 2];
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexPathPerfTest,
	"RefactorTactics.Perf.PathfindingMedian",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexPathPerfTest::RunTest(const FString&)
{
	// Mappa r=12 (469 celle): piu' grande dell'arena di gioco, cosi' la misura ha margine.
	URTHexMapAsset* Map = MakePerfMap(12);

	// Percorsi lunghi da un bordo all'altro, in direzioni diverse: il caso peggiore realistico.
	const TArray<TPair<FRTCellId, FRTCellId>> Routes = {
		{ FRTCellId(-12, 0),  FRTCellId(12, 0) },
		{ FRTCellId(0, -12),  FRTCellId(0, 12) },
		{ FRTCellId(-12, 12), FRTCellId(12, -12) },
		{ FRTCellId(-8, -4),  FRTCellId(8, 4) },
	};

	TArray<double> Samples;
	for (int32 Iteration = 0; Iteration < 50; ++Iteration)
	{
		for (const TPair<FRTCellId, FRTCellId>& Route : Routes)
		{
			const double Start = FPlatformTime::Seconds();
			const FRTHexPathResult Result = URTHexPathLibrary::FindPath(Map, Route.Key, Route.Value);
			Samples.Add((FPlatformTime::Seconds() - Start) * 1000.0);
			TestTrue(TEXT("il percorso esiste (altrimenti si misurerebbe una ricerca fallita)"),
				Result.Status == ERTHexPathStatus::Success);
		}
	}

	const double Median = MedianMs(Samples);
	UE_LOG(LogTemp, Display, TEXT("[KPI] Pathfinding A* su mappa r=12 (469 celle): mediana %.3f ms su %d campioni"),
		Median, Samples.Num());
	AddInfo(FString::Printf(TEXT("KPI pathfinding: mediana %.3f ms (target PDR: < 2 ms)"), Median));

	// Soglia larga: cattura la regressione catastrofica, non il rumore.
	TestTrue(FString::Printf(TEXT("il pathfinding resta nell'ordine di grandezza atteso (%.3f ms)"), Median),
		Median < 20.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexResolverPerfTest,
	"RefactorTactics.Perf.TurnResolverMedian",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexResolverPerfTest::RunTest(const FString&)
{
	UWorld* World = MakePerfWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Map = MakePerfMap(6);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Map;

	// 2v2 come in partita, con il playback DISATTIVATO: qui si misura la risoluzione (logica autoritativa),
	// non la sua riproduzione nel tempo, che e' presentazione.
	auto Spawn = [World](int32 Team, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		U->TeamId = Team;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = true;
		U->DispatchBeginPlay();
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, 250.f);
		return U;
	};
	Spawn(0, URTHeroCatalogLibrary::MakeVektor(),   FRTCellId(-5, 2));
	Spawn(0, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(-5, 3));
	Spawn(1, URTHeroCatalogLibrary::MakeVektor(),   FRTCellId(5, -2));
	Spawn(1, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(5, -3));

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM) { DestroyPerfWorld(World); return false; }
	TM->bEnablePlayback = false;

	TArray<double> Samples;
	for (int32 Turn = 0; Turn < 10 && TM->GetPhase() != ERTMatchPhase::MatchEnded; ++Turn)
	{
		TM->PlanBotsForTest();
		const double Start = FPlatformTime::Seconds();
		TM->LockInAndResolve();
		Samples.Add((FPlatformTime::Seconds() - Start) * 1000.0);
	}

	TestTrue(TEXT("sono stati misurati dei turni"), Samples.Num() > 0);
	const double Median = MedianMs(Samples);
	UE_LOG(LogTemp, Display, TEXT("[KPI] Resolver di turno (2v2, mappa r=6, senza playback): mediana %.3f ms su %d turni"),
		Median, Samples.Num());
	AddInfo(FString::Printf(TEXT("KPI resolver: mediana %.3f ms/turno (target PDR: < 100 ms)"), Median));

	TestTrue(FString::Printf(TEXT("il resolver resta nell'ordine di grandezza atteso (%.3f ms)"), Median),
		Median < 1000.0);

	DestroyPerfWorld(World);
	return true;
}
