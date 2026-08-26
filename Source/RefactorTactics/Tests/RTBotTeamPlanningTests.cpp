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
#include "Turn/RTPlanValidationLibrary.h" // MakePlanFor: la premessa di non-vacuita del piano
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

	/**
	 * Quante righe di rifiuto al lock-in nominano questa unita'.
	 *
	 * Il prefisso e' scritto UNA volta: la stessa stringa e' ripetuta a mano in piu' punti fra i file di
	 * test, e cambiarla in `RTTurnManager` li renderebbe vacui tutti insieme senza che nessuno diventi rosso.
	 */
	int32 CountLockInRejections(const ARTTurnManager* TM, const FString& UnitName)
	{
		static const TCHAR* Prefisso = TEXT("piano non valido al lock-in");
		int32 N = 0;
		for (const FString& Evento : TM->GetRecentEvents())
		{
			if (Evento.Contains(Prefisso) && Evento.Contains(UnitName)) { ++N; }
		}
		return N;
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLockInValidatesBotPlansTooTest,
	"RefactorTactics.Bot.LockInValidatesBotPlansToo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLockInValidatesBotPlansTooTest::RunTest(const FString&)
{
	// 🔴 **L'invariante #1 del DoD di CP 38.2**: *«il bot passa dallo stesso punto — una validazione che il
	// bot aggira non e' una regola»*. `ValidatePlansAtLockIn` itera `CollectLivingUnits`, che non guarda
	// `bIsBotControlled`, quindi il bot ci passa gia'. Ma fino al 2026-08-26 **nessun test lo dimostrava**: i
	// due test del lock-in costruiscono un'unita' del giocatore, e una riga di DoD spuntata su
	// un'implementazione plausibile ma non misurata e' esattamente il difetto che questo repository paga di
	// piu'.
	//
	// Il piano si scrive a mano sui campi, come fa `PlanBots`: i bot pianificano a INIZIO turno, non al
	// lock-in, quindi un piano scritto qui sopravvive fino al validatore.
	UWorld* World = MakeTeamPlanningWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	// (!) `TestNotNull` e non un `return false` muto: l'automation ignora il bool di `RunTest` e decide
	// dall'assenza di errori, quindi un'uscita anticipata senza asserzioni riporta **Success** su un test
	// che non ha toccato niente.
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeTestArena(GetTransientPackage());
	if (!TestNotNull(TEXT("arena di prova"), Map)) { DestroyTeamPlanningWorld(World); return false; }
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	if (!TestNotNull(TEXT("attore mappa"), MapActor)) { DestroyTeamPlanningWorld(World); return false; }
	MapActor->MapAsset = Map;

	const FRTCellId Partenza(-2, 0, 0);
	const FRTCellId CellaScatto(-2, 1, 0);
	const FRTCellId CellaMovimento(-1, 0, 0);
	ARTUnit* Bot = SpawnTeamPlanningUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), Partenza, /*bBot=*/ true);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("bot"), Bot) || !TestNotNull(TEXT("turn manager"), TM))
	{
		DestroyTeamPlanningWorld(World);
		return false;
	}

	const int32 DashIdx = Bot->FindDashAbilityIndex();
	if (!TestNotEqual(TEXT("premessa: l'eroe ha una mobilita' rapida"), DashIdx, static_cast<int32>(INDEX_NONE))
		|| !TestTrue(TEXT("premessa: l'unita' e' controllata dal bot"), Bot->bIsBotControlled))
	{
		DestroyTeamPlanningWorld(World);
		return false;
	}

	// Il piano incoerente: scatto piu' movimento normale, due azioni sullo slot Movimento. Le due celle
	// sono DIVERSE di proposito: con la stessa cella, dopo lo scatto `Cell == PlannedCell` e il movimento
	// normale diventa un no-op, quindi il turno non distinguerebbe piu' quale delle due azioni ha vinto.
	Bot->PlannedDashAbility = DashIdx;
	Bot->PlannedDashCell = CellaScatto;
	Bot->PlannedCell = CellaMovimento;
	if (!TestTrue(TEXT("premessa: il piano dichiara un movimento normale"), Bot->HasPlannedNormalMove())
		|| !TestTrue(TEXT("premessa: il piano non e' vuoto, quindi il validatore non lo salta"),
			URTPlanValidationLibrary::MakePlanFor(Bot).Num() > 0))
	{
		DestroyTeamPlanningWorld(World);
		return false;
	}

	TM->LockInAndResolve();

	// (!) Il conteggio si fa **prima** del playback: `ValidatePlansAtLockIn` scrive all'inizio del turno,
	// quindi la sua riga e' la piu' VECCHIA — e `AddLogEvent` taglia `RecentEvents` dalla testa oltre
	// `MaxLogLines`. Con una unita' sola non si sfora, ma il giorno in cui questo allestimento ne guadagna
	// una seconda il test fallirebbe per sfratto, non per il validatore.
	const int32 Righe = CountLockInRejections(TM, Bot->GetName());

	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }
	TestFalse(TEXT("la risoluzione e' finita entro il numero di tick previsto"), TM->IsResolving());

	TestEqual(TEXT("il piano del BOT passa dallo stesso validatore del giocatore"), Righe, 1);

	DestroyTeamPlanningWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLockInStaysSilentOnALegalBotPlanTest,
	"RefactorTactics.Bot.LockInStaysSilentOnALegalBotPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLockInStaysSilentOnALegalBotPlanTest::RunTest(const FString&)
{
	// Il gemello: senza, un `AddLogEvent` incondizionato passerebbe il test qui sopra. Stesso allestimento,
	// stesso bot, ma un piano LEGALE — un movimento e basta.
	UWorld* World = MakeTeamPlanningWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeTestArena(GetTransientPackage());
	if (!TestNotNull(TEXT("arena di prova"), Map)) { DestroyTeamPlanningWorld(World); return false; }
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	if (!TestNotNull(TEXT("attore mappa"), MapActor)) { DestroyTeamPlanningWorld(World); return false; }
	MapActor->MapAsset = Map;

	ARTUnit* Bot = SpawnTeamPlanningUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(-2, 0, 0), /*bBot=*/ true);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("bot"), Bot) || !TestNotNull(TEXT("turn manager"), TM))
	{
		DestroyTeamPlanningWorld(World);
		return false;
	}

	Bot->PlannedCell = FRTCellId(-2, 1, 0); // un movimento e basta: nessuno scatto, nessun conflitto di slot

	// (!) **Le premesse contro la vacuita', e non sono decorative**: `ValidatePlansAtLockIn` salta le unita'
	// con un piano VUOTO (`if (Plan.Num() == 0) continue`). Senza queste tre righe, il giorno in cui il piano
	// diventasse vuoto — la cella di spawn spostata, `MakePlanFor` che smette di emettere `Action.Move` — il
	// conteggio resterebbe zero e il test resterebbe verde senza aver piu' attraversato il validatore.
	if (!TestTrue(TEXT("premessa: l'unita' e' controllata dal bot"), Bot->bIsBotControlled)
		|| !TestTrue(TEXT("premessa: il piano dichiara un movimento normale"), Bot->HasPlannedNormalMove())
		|| !TestTrue(TEXT("premessa: il piano non e' vuoto, quindi il validatore non lo salta"),
			URTPlanValidationLibrary::MakePlanFor(Bot).Num() > 0))
	{
		DestroyTeamPlanningWorld(World);
		return false;
	}

	TM->LockInAndResolve();

	// Stesso predicato del gemello — filtrato per nome — cosi' le due asserzioni restano confrontabili
	// anche quando questo allestimento guadagnera' una seconda unita'.
	const int32 Righe = CountLockInRejections(TM, Bot->GetName());

	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }
	TestFalse(TEXT("la risoluzione e' finita entro il numero di tick previsto"), TM->IsResolving());

	TestEqual(TEXT("un piano legale del bot non produce nessuna riga di rifiuto"), Righe, 0);

	DestroyTeamPlanningWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanBotsWritesWhatTheValidatorReadsTest,
	"RefactorTactics.Bot.PlanBotsWritesWhatTheValidatorReads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanBotsWritesWhatTheValidatorReadsTest::RunTest(const FString&)
{
	// I due test qui sopra scrivono il piano del bot A MANO, «come fa `PlanBots`». Quel «come» era
	// un'assunzione dichiarata in un commento e ancorata a niente: se `PlanBots` cominciasse a dichiarare il
	// movimento per un canale che `MakePlanFor` non legge — `PlannedWaypoints`, una rotta composita — il piano
	// del bot VERO smetterebbe di arrivare al validatore mentre quei due test restano verdi.
	//
	// Questo lega i due capi: fa pianificare il bot davvero, e verifica che cio' che scrive sia cio' che il
	// validatore legge.
	UWorld* World = MakeTeamPlanningWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeTestArena(GetTransientPackage());
	if (!TestNotNull(TEXT("arena di prova"), Map)) { DestroyTeamPlanningWorld(World); return false; }
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	if (!TestNotNull(TEXT("attore mappa"), MapActor)) { DestroyTeamPlanningWorld(World); return false; }
	MapActor->MapAsset = Map;

	// Un avversario serve: senza qualcuno da raggiungere, il bot puo' legittimamente non pianificare niente.
	ARTUnit* Bot = SpawnTeamPlanningUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(-2, 0, 0), /*bBot=*/ true);
	ARTUnit* Nemico = SpawnTeamPlanningUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(2, 0, 0), /*bBot=*/ true);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("bot"), Bot) || !TestNotNull(TEXT("nemico"), Nemico) || !TestNotNull(TEXT("turn manager"), TM))
	{
		DestroyTeamPlanningWorld(World);
		return false;
	}

	TM->PlanBotsForTest();

	// La proprieta': il piano che il bot ha scritto da solo e' VISIBILE al validatore. `MakePlanFor` e' il
	// punto da cui `ValidatePlansAtLockIn` ricava le voci da giudicare, e un piano vuoto lo fa `continue`.
	const TArray<FRTPlannedAction> Piano = URTPlanValidationLibrary::MakePlanFor(Bot);
	AddInfo(FString::Printf(TEXT("il bot ha pianificato: cella %s, dash %d, voci lette dal validatore %d"),
		*Bot->PlannedCell.ToString(), Bot->PlannedDashAbility, Piano.Num()));

	TestTrue(TEXT("il bot ha pianificato qualcosa"),
		Bot->HasPlannedNormalMove() || Bot->PlannedDashAbility != INDEX_NONE
		|| Bot->PlannedAbilityIndex != INDEX_NONE);
	TestTrue(TEXT("e cio' che ha scritto arriva al validatore, non a un canale che non legge"), Piano.Num() > 0);

	DestroyTeamPlanningWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
