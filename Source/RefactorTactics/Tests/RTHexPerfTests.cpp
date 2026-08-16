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
#include "Ability/RTActionData.h"
#include "Combat/RTHexCombatLibrary.h"

// La guardia: senza, i test di questo file finiscono compilati DENTRO il binario Shipping che si
// distribuisce. Non e' una formalita' di build — e' cio' che tiene il codice di test fuori dal gioco.
// Aggiunta con #923, dopo che lo stesso difetto ha rotto la build Shipping due volte (2026-08-09 e
// 2026-08-15) senza che la suite, che gira sul target Editor, se ne accorgesse.
#if WITH_DEV_AUTOMATION_TESTS

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

	/**
	 * Percentile per nearest-rank su un campione GIA' ordinato — lo stesso metodo che la tabella KPI usa per
	 * `Decisione del giocatore`, e per la stessa ragione: su una latenza che un essere umano *aspetta*, cio'
	 * che infastidisce e' il caso peggiore, non quello tipico. La mediana la nasconde per costruzione.
	 */
	double PercentileMs(const TArray<double>& SortedSamples, double Percentile)
	{
		if (SortedSamples.Num() == 0) { return 0.0; }
		const int32 Rank = FMath::Clamp(
			FMath::CeilToInt(Percentile * SortedSamples.Num()) - 1, 0, SortedSamples.Num() - 1);
		return SortedSamples[Rank];
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
	Spawn(0, URTHeroCatalogLibrary::MakeWraith(),   FRTCellId(-5, 2));
	Spawn(0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(-5, 3));
	Spawn(1, URTHeroCatalogLibrary::MakeWraith(),   FRTCellId(5, -2));
	Spawn(1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(5, -3));

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

/**
 * KPI **Preview completa** (CP 3.3, #41) — il costo di ricalcolare l'anteprima di pianificazione.
 *
 * ## Cosa misura, e perche' e' questo e non il disegno
 *
 * La riga della tabella KPI dice *«profiling della preview di pianificazione»*, e nel codice esistono due
 * cose diverse con quel nome: il **calcolo** (`RefreshPlanningPreview` -> snapshot, celle raggiungibili,
 * celle colpite) e il **disegno** (`ARTHexMapActor::DrawPlanningPreview`, chiamato da `Tick`, piu' i mark di
 * `ARTHUD::DrawHUD`).
 *
 * Il target — **< 50 ms** — decide da solo quale delle due: a 60 FPS il frame INTERO e' 16,6 ms, quindi un
 * budget di 50 ms per il solo disegno sarebbe tre frame e contraddirebbe la riga `Client FPS` della stessa
 * tabella. 50 ms e' una **latenza di risposta a un input**: il tempo fra «il giocatore sposta il bersaglio» e
 * «l'area mostrata e' quella nuova». Deciso con l'autore il 2026-08-14.
 *
 * Il costo di disegno resta dov'e' gia' contato, dentro `Client FPS`, che aspetta E21.
 *
 * ## Cosa NON e', dichiarato
 *
 * `RefreshPlanningPreview` vive in un namespace anonimo dentro `RTPlayerController.cpp` e non e' chiamabile
 * da qui. Questo test misura le **tre funzioni che compongono il suo costo**, chiamate nello stesso ordine e
 * con gli stessi argomenti; restano fuori l'orchestrazione (due `if` e due copie di array) e il passaggio
 * dei risultati all'actor di mappa. E' una misura del costo dominante, non della funzione intera — dirlo
 * conta, perche' un KPI che sembra misurare piu' di quanto misura e' il modo in cui un budget diventa falso.
 *
 * ⚠️ Non c'e' calcolo parallelo: le funzioni sono **le stesse** che decidono l'esito
 * (`ReachableCells`, `HexHitCells`), come impone il commento di `RefreshPlanningPreview`. Se un giorno la
 * preview cambiasse strada, questo test misurerebbe la strada vecchia — ed e' il motivo per cui il nome
 * delle tre funzioni sta scritto qui sopra invece che solo nel codice.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexPlanningPreviewPerfTest,
	"RefactorTactics.Perf.PlanningPreviewMedian",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexPlanningPreviewPerfTest::RunTest(const FString&)
{
	// r=6 come `TurnResolverMedian`: e' la scala del 2v2, cioe' quella su cui la preview gira davvero.
	URTHexMapAsset* Map = MakePerfMap(6);

	// 2v2 con budget di movimento pieno: le celle raggiungibili sono il pezzo che cresce con il budget, e
	// misurarlo con un budget piccolo direbbe che la preview costa meno di quanto costa in partita.
	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(-5, 2), /*budget*/ 4));
	Units.Add(FRTHexSimUnit(2, FRTCellId(-5, 3), /*budget*/ 4));
	Units.Add(FRTHexSimUnit(3, FRTCellId(5, -2), /*budget*/ 4));
	Units.Add(FRTHexSimUnit(4, FRTCellId(5, -3), /*budget*/ 4));

	TArray<double> Samples;
	for (int32 Iteration = 0; Iteration < 100; ++Iteration)
	{
		const double Start = FPlatformTime::Seconds();

		// La catena di `RefreshPlanningPreview`, nello stesso ordine.
		const FRTHexSnapshot Snapshot = URTHexSimLibrary::MakeSnapshot(Map, Units);
		const TArray<FRTHexReachableCell> Reachable = URTHexSimLibrary::ReachableCells(Snapshot, /*UnitId=*/ 1);
		const TArray<FRTCellId> Hit = URTHexCombatLibrary::HexHitCells(
			ERTAbilityShape::Area, FRTCellId(-5, 2), FRTCellId(5, -2), /*RangeCells=*/ 12, /*AreaRadius=*/ 2);

		Samples.Add((FPlatformTime::Seconds() - Start) * 1000.0);

		// Senza questo, un compilatore che vedesse i risultati inutilizzati potrebbe accorciare la misura.
		TestTrue(TEXT("la preview produce celle raggiungibili"), Reachable.Num() > 0);
		TestTrue(TEXT("la preview produce un'area colpita"), Hit.Num() > 0);
	}

	Samples.Sort();
	const double Median = MedianMs(Samples);
	const double P90 = PercentileMs(Samples, 0.90);

	UE_LOG(LogTemp, Display,
		TEXT("[KPI] Preview di pianificazione (2v2, mappa r=6, budget 4): mediana %.3f ms, p90 %.3f ms su %d campioni"),
		Median, P90, Samples.Num());
	AddInfo(FString::Printf(TEXT("KPI preview: mediana %.3f ms, p90 %.3f ms (target PDR: < 50 ms)"), Median, P90));

	// Soglia larga come gli altri due KPI di questo file, e per la stessa ragione scritta in testa: cattura
	// la regressione catastrofica, non il rumore di scheduling. Il numero che conta e' quello stampato.
	TestTrue(FString::Printf(TEXT("la preview resta nell'ordine di grandezza atteso (p90 %.3f ms)"), P90),
		P90 < 500.0);
	return true;
}

// Chi aggiunge un test in fondo a questo file lo aggiunge PRIMA di questa riga: e' il difetto di #923,
// invisibile in Editor dove la guardia vale 1. Il controllo che lo dimostra e'
// `Build.bat RefactorTactics Win64 Shipping`, non la suite.
#endif // WITH_DEV_AUTOMATION_TESTS
