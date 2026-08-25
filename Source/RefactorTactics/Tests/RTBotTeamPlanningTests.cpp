// `ReservePlannedRoute`: una rotta prenotata è occupata per le ALTRE unità e libera per la propria (#1088).
//
// È il mattone della correzione, ed è testato qui **da solo** perché la sua proprietà non dipende
// dall'utility del bot: qualunque cella scelga, dopo la prenotazione quella rotta non deve essere
// disponibile a nessun altro. L'effetto sul difetto — le 24 contese fra compagni misurate da #1088 — si
// verifica invece nel ciclo che le ha prodotte, in `RTBotStalemateProbeTests.cpp`, perché lì la
// pianificazione passa dal filtro di percezione ed è quella la condizione in cui lo stallo si forma.
//
// 🔴 **La prima stesura di questo file sbagliava proprio qui**: costruiva due compagne con conoscenza
// perfetta dei nemici e dava per scontato che scegliessero la stessa cella. Non è così — misurato:
// `u1 -> (0,-3)` e `u2 -> (-1,4)`, destinazioni opposte — perché senza il filtro di percezione il bot
// valuta la minaccia in modo diverso. Il test l'ha detto invece di restare verde a vuoto, e solo perché
// asseriva la propria premessa. Vedi il controllo di non-vacuità nel probe.

#include "Misc/AutomationTest.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Bot/RTHexBotLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotReserveRouteTest,
	"RefactorTactics.Bot.ReservedRouteBlocksTeammatesOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTBotReserveRouteTest::RunTest(const FString&)
{
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(GetTransientPackage());
	if (!TestNotNull(TEXT("arena di prova generata"), Arena)) { return false; }

	const FRTCellId CellA(-2, 0, 0);
	const FRTCellId CellB(-2, 1, 0);

	TArray<FRTHexSimUnit> SimUnits;
	SimUnits.Add(FRTHexSimUnit(1, CellA, /*budget*/ 5));
	SimUnits.Add(FRTHexSimUnit(2, CellB, /*budget*/ 5));
	const FRTHexSnapshot Snapshot = URTHexSimLibrary::MakeSnapshot(Arena, SimUnits);

	// Una destinazione qualsiasi ma RAGGIUNGIBILE: la rotta dev'essere reale, altrimenti il test verificherebbe
	// la prenotazione di un percorso vuoto — che è vera per costruzione e non dice niente.
	const FRTCellId Dest(0, 0, 0);
	const TArray<FRTCellId> Route = URTHexSimLibrary::FindPathForUnit(Snapshot, 1, Dest).Path;
	if (!TestTrue(TEXT("premessa: la rotta esiste ed è di almeno due celle"), Route.Num() >= 2)) { return false; }

	FString Printed;
	for (const FRTCellId& C : Route) { Printed += (Printed.IsEmpty() ? TEXT("") : TEXT(" -> ")) + C.ToString(); }
	AddInfo(FString::Printf(TEXT("rotta di u1: %s"), *Printed));

	FRTHexSnapshot Reserved = Snapshot;
	URTHexBotLibrary::ReservePlannedRoute(Reserved, /*UnitId=*/ 1, Dest);

	// --- 1. Ogni cella della rotta risulta occupata, e dall'unità che l'ha prenotata.
	int32 Missing = 0;
	int32 WrongOwner = 0;
	for (const FRTCellId& Cell : Route)
	{
		const int32* Owner = Reserved.Occupancy.Find(Cell);
		if (!Owner) { ++Missing; }
		else if (*Owner != 1) { ++WrongOwner; }
	}
	TestEqual(TEXT("nessuna cella della rotta è rimasta libera"), Missing, 0);
	TestEqual(TEXT("e nessuna risulta di un'altra unità"), WrongOwner, 0);

	// --- 2. Per la COMPAGNA quelle celle sono occupate: è il punto della prenotazione.
	//
	// Si misura sulle celle raggiungibili, che è la funzione da cui il bot genera le candidate: se una cella
	// prenotata comparisse ancora fra le raggiungibili di `u2`, `BuildCandidates` potrebbe riproporla e la
	// prenotazione non servirebbe a niente.
	const TArray<FRTHexReachableCell> ReachableBefore = URTHexSimLibrary::ReachableCells(Snapshot, /*UnitId=*/ 2);
	const TArray<FRTHexReachableCell> ReachableAfter = URTHexSimLibrary::ReachableCells(Reserved, /*UnitId=*/ 2);

	auto Contains = [](const TArray<FRTHexReachableCell>& Cells, const FRTCellId& Target)
	{
		for (const FRTHexReachableCell& C : Cells) { if (C.Cell == Target) { return true; } }
		return false;
	};

	// ⚠️ Il controllo di non-vacuità: se `u2` non potesse già raggiungere nessuna cella della rotta, «dopo non
	// le raggiunge» sarebbe vero senza che la prenotazione abbia fatto niente.
	int32 ReachableOnRouteBefore = 0;
	int32 ReachableOnRouteAfter = 0;
	for (int32 I = 1; I < Route.Num(); ++I)     // dalla 1: la cella di partenza di u1 era già occupata
	{
		if (Contains(ReachableBefore, Route[I])) { ++ReachableOnRouteBefore; }
		if (Contains(ReachableAfter, Route[I])) { ++ReachableOnRouteAfter; }
	}
	AddInfo(FString::Printf(TEXT("celle della rotta raggiungibili da u2: prima %d, dopo %d"),
		ReachableOnRouteBefore, ReachableOnRouteAfter));

	TestTrue(TEXT("premessa: prima della prenotazione u2 poteva entrare nella rotta di u1"),
		ReachableOnRouteBefore > 0);
	TestEqual(TEXT("dopo la prenotazione, nessuna cella della rotta è raggiungibile da u2"),
		ReachableOnRouteAfter, 0);

	// --- 3. Ma u1 la sua rotta la percorre ancora: `ReachableCells` non blocca un'unità con se stessa
	// (`*Occupant != UnitId`), ed è la ragione per cui si prenota con l'id del prenotante e non con un
	// marcatore generico. Con un id qualsiasi, l'unità si sbarrerebbe la strada da sola.
	const TArray<FRTCellId> RouteAfter = URTHexSimLibrary::FindPathForUnit(Reserved, /*UnitId=*/ 1, Dest).Path;
	TestEqual(TEXT("u1 percorre ancora la propria rotta, invariata"), RouteAfter.Num(), Route.Num());

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// E la stessa proprietà su `ARTTurnManager::PlanBots`, cioè sul percorso che il gioco esegue davvero.
//
// ⚠️ **Serve perché il test qui sopra e il probe non lo coprono.** Il primo verifica la funzione di
// prenotazione in isolamento; il secondo riproduce il ciclo di `PlanBots` invece di chiamarlo. Nessuno dei
// due fallirebbe se l'innesto in `RTTurnManager.cpp` venisse rimosso — e un difetto che nessun test vede è
// il difetto che torna.
// ---------------------------------------------------------------------------------------------------------

namespace
{
	UWorld* MakeTeamPlanningWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyTeamPlanningWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTUnit* SpawnTeamPlanningUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell,
		bool bBot)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = bBot;
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell;
		return U;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanBotsNoTeammateOverlapTest,
	"RefactorTactics.Bot.PlanBotsGivesTeammatesDistinctCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTPlanBotsNoTeammateOverlapTest::RunTest(const FString&)
{
	UWorld* World = MakeTeamPlanningWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	// L'arena della configurazione spedita, quella su cui lo stallo è stato misurato.
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeTestArena(GetTransientPackage());
	if (!Map) { DestroyTeamPlanningWorld(World); return false; }
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Map;

	// Due bot per squadra, alle stesse celle del probe: è la configurazione in cui le compagne si sono
	// bloccate a vicenda per dodici turni.
	ARTUnit* BotA = SpawnTeamPlanningUnit(World, 0, URTHeroCatalogLibrary::MakeGadget(), FRTCellId(-2, 0, 0), true);
	ARTUnit* BotB = SpawnTeamPlanningUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(-2, 1, 0), true);
	ARTUnit* FoeA = SpawnTeamPlanningUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(2, 0, 0), true);
	ARTUnit* FoeB = SpawnTeamPlanningUnit(World, 1, URTHeroCatalogLibrary::MakePhase(), FRTCellId(2, -1, 0), true);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !BotA || !BotB || !FoeA || !FoeB) { DestroyTeamPlanningWorld(World); return false; }

	TM->PlanBotsForTest();

	AddInfo(FString::Printf(TEXT("squadra 0: %s e %s | squadra 1: %s e %s"),
		*BotA->PlannedCell.ToString(), *BotB->PlannedCell.ToString(),
		*FoeA->PlannedCell.ToString(), *FoeB->PlannedCell.ToString()));

	// La proprietà: due COMPAGNE non pianificano la stessa cella. Chi scatta non entra nel confronto — la
	// sua cella d'arrivo la decide la fase Dash, che ha le proprie priorità e non è dove il difetto vive.
	auto MovesNormally = [](const ARTUnit* U) { return U && U->PlannedDashAbility == INDEX_NONE; };

	// ⚠️ **Le due verifiche si fanno PER SQUADRA, e la prima stesura le faceva globali.** Contando le mosse
	// su tutte e quattro le unità, la squadra 0 poteva essere nello stallo originale — entrambe ferme sulla
	// propria cella, quindi «celle distinte» vero per costruzione — e il test restava verde perché si era
	// mossa un'unità della squadra 1. Il difetto che questo test esiste per cogliere sarebbe passato.
	auto CheckTeam = [&](const TCHAR* Label, ARTUnit* First, ARTUnit* Second)
	{
		// ⚠️ Uno scatto salterebbe il confronto sulle celle: allora la squadra non verifica nulla, e va detto
		// invece di tacere — un'asserzione che non gira è indistinguibile da una che passa.
		if (MovesNormally(First) && MovesNormally(Second))
		{
			TestFalse(FString::Printf(TEXT("%s: le due compagne non puntano la stessa cella"), Label),
				First->PlannedCell == Second->PlannedCell);
		}
		else
		{
			AddWarning(FString::Printf(
				TEXT("%s: confronto celle NON eseguito (almeno una scatta) — la squadra resta non verificata"),
				Label));
		}

		// Non-vacuità della SQUADRA: almeno una delle due deve davvero spostarsi, altrimenti «celle distinte»
		// è soddisfatto dallo stallo.
		int32 Moving = 0;
		for (const ARTUnit* U : { First, Second })
		{
			const bool bDashes = U && U->PlannedDashAbility != INDEX_NONE;
			if (U && (bDashes ? !(U->PlannedDashCell == U->Cell) : !(U->PlannedCell == U->Cell))) { ++Moving; }
		}
		AddInfo(FString::Printf(TEXT("%s: unita' che si spostano %d/2"), Label, Moving));
		TestTrue(FString::Printf(TEXT("%s: almeno una si muove davvero"), Label), Moving > 0);
	};

	CheckTeam(TEXT("squadra 0"), BotA, BotB);
	CheckTeam(TEXT("squadra 1"), FoeA, FoeB);

	DestroyTeamPlanningWorld(World);
	return true;
}


/**
 * 🔴 **L'invariante dei pesi si misura sull'ISTANZA, non sul CDO** (`#1276`).
 *
 * `HexBot.ElevationNeverOutweighsClosingOneCell` la pinna leggendo `GetDefault<ARTTurnManager>()`. Ma
 * `ARTGameMode` **riusa** un `ARTTurnManager` gia' presente nel livello invece di spawnarlo, e un'istanza
 * piazzata serializza i propri `UPROPERTY` nel `.umap`: un livello che portasse ancora `WElevation = 20`
 * riaprirebbe lo stato assorbente di `#1088` **mentre quel test resta verde**.
 *
 * Un test non puo' vedere quel caso — non carica i livelli — quindi il presidio e' a runtime, e questo
 * test verifica che il presidio ci sia e che non urli quando non deve.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotWeightInvariantTest,
	"RefactorTactics.Bot.WeightInvariantIsCheckedOnTheLiveInstance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotWeightInvariantTest::RunTest(const FString&)
{
	// Mappa su TRE layer: l'invariante non e' una proprieta' dei soli pesi ma del loro rapporto con la
	// mappa, e su due layer gli stessi numeri reggerebbero.
	auto MakeMap = []()
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 3))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		for (int32 L = 1; L <= 2; ++L)
		{
			M->AddOrUpdateCell(FRTHexCellData(FRTCellId(0, 0, L)));
			M->AddTransition(FRTCellId(0, 0, L - 1), FRTCellId(0, 0, L), /*Cost=*/ 1);
		}
		M->SortCells();
		return M;
	};

	// --- (1) Pesi FUORI invariante: il presidio deve URLARE ---------------------------------------
	{
		UWorld* World = MakeTeamPlanningWorld();
		if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = MakeMap();

		ARTUnit* Bot = SpawnTeamPlanningUnit(World, 0, URTHeroCatalogLibrary::MakeGadget(), FRTCellId(-2, 0, 0), true);
		ARTUnit* Foe = SpawnTeamPlanningUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(2, 0, 0), true);
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Bot || !Foe) { DestroyTeamPlanningWorld(World); return false; }

		// Il valore che `#1088` ha tolto dal default, rimesso come farebbe un `.umap` mai riallineato.
		TM->WElevation = 20;
		TestTrue(TEXT("premessa: 20 * 2 supera davvero WApproach"),
			TM->WElevation * 2 >= TM->WApproach);

		AddExpectedError(TEXT("INVARIANTE PESI BOT VIOLATA"), EAutomationExpectedErrorFlags::Contains, 1);
		TM->PlanBotsForTest();

		DestroyTeamPlanningWorld(World);
	}

	// --- (2) Pesi DI DEFAULT: il presidio deve TACERE ----------------------------------------------
	// ⚠️ Senza questa meta' il test passerebbe anche con un controllo che urla sempre — e un allarme che
	// suona a ogni partita e' un allarme che si impara a ignorare.
	{
		UWorld* World = MakeTeamPlanningWorld();
		if (!TestNotNull(TEXT("secondo world"), World)) { return false; }
		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = MakeMap();

		ARTUnit* Bot = SpawnTeamPlanningUnit(World, 0, URTHeroCatalogLibrary::MakeGadget(), FRTCellId(-2, 0, 0), true);
		ARTUnit* Foe = SpawnTeamPlanningUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(2, 0, 0), true);
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Bot || !Foe) { DestroyTeamPlanningWorld(World); return false; }

		TestTrue(TEXT("premessa: i default rispettano l'invariante su questa mappa"),
			TM->WElevation * 2 < TM->WApproach);

		// Nessun `AddExpectedError`: se il presidio urlasse, l'errore non atteso farebbe cadere il test.
		TM->PlanBotsForTest();

		DestroyTeamPlanningWorld(World);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
