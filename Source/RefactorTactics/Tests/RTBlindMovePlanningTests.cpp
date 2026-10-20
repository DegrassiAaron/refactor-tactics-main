// Blind Move, fetta 1 — la mappa ottimistica su cui il GIOCATORE pianifica un Move verso lo spazio non osservato
// (`#2794`, [D-372], [D-533]).
//
// Due livelli, perche' sono due difetti diversi: `MakeOptimisticPlanningMap` decide COSA si svuota, e
// `ARTTurnManager::AttachPlanningMap` decide PER CHI e quando — e un test sul primo non vede il secondo.

#include "Misc/AutomationTest.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Perception/RTTeamKnowledge.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: il progetto compila in unity build. */
	UWorld* MakeBlindMoveWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyBlindMoveWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Chi pianifica sta a ovest; il muro e la meta stanno oltre la sua vista, sulla stessa riga. */
	const FRTCellId BlindMoveStart{ -8, 0, 0 };
	const FRTCellId BlindMoveWall{ 4, 0, 0 };
	const FRTCellId BlindMoveGoal{ 6, 0, 0 };
	constexpr int32 BlindMoveArenaRadius = 12;

	FRTTeamKnowledge BlindMoveKnowledge(int32 TeamId, const TArray<FRTCellId>& Visible, const TArray<FRTCellId>& Explored)
	{
		FRTTeamKnowledge K;
		K.TeamId = TeamId;
		K.TurnNumber = 1;
		K.VisibleCells = Visible;
		K.ExploredCells = Explored;
		return K;
	}

	bool BlindMoveIsLit(const FRTTeamKnowledge& K, const FRTCellId& Cell)
	{
		return K.VisibleCells.Contains(Cell) || K.ExploredCells.Contains(Cell);
	}
}

/**
 * **Test 1** — `MakeOptimisticPlanningMap` svuota la cella mai osservata, tiene quella osservata e non tocca `Map`.
 *
 * 🔑 **Tre celle, tre ragioni**: `Dark` non e' ne' vista ne' ricordata (si svuota), `Seen` e' solo visibile e
 * `Remembered` solo ricordata (restano autorevoli). `Seen` esiste per l'**unione**: `ExploredCells` e' un
 * sovrainsieme di `VisibleCells` solo dopo il refresh della conoscenza, e leggere una sola delle due liste
 * svuoterebbe la cella sotto i piedi di chi pianifica.
 *
 * ⛔ **Verifica di mutazione**: leggere solo `ExploredCells` rende rosso `Seen`; non svuotare `bBlocksMovement`
 * rende rosso `Dark`; ricostruire anche le `Transitions` rende rosso l'ultimo asserto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlindMoveOptimisticMapEmptiesOnlyTheNeverObservedTest,
	"RefactorTactics.BlindMove.OptimisticMapEmptiesOnlyTheNeverObserved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlindMoveOptimisticMapEmptiesOnlyTheNeverObservedTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 3);
	if (!TestNotNull(TEXT("premessa: l'arena esiste"), Map)) { return false; }

	const FRTCellId Dark{ 2, 0, 0 };
	const FRTCellId Seen{ 0, 1, 0 };
	const FRTCellId Remembered{ 1, -1, 0 };
	for (const FRTCellId& Id : { Dark, Seen, Remembered })
	{
		FRTHexCellData Data(Id);
		Data.bBlocksMovement = true;
		Data.bBlocksLineOfSight = true;
		Data.Surface = ERTHexSurface::Ice;
		Data.MoveCost = 3;
		Data.OccupancySurcharge = 2;
		Data.Doors.Add(FRTHexDoor(ERTHexDirection::E));
		Map->AddOrUpdateCell(Data);
	}
	Map->SortCells();

	const uint32 HashBefore = Map->ComputeHash();
	const int32 TransitionsBefore = Map->Transitions.Num();

	const FRTTeamKnowledge K = BlindMoveKnowledge(0, { Seen }, { Remembered });
	if (!TestTrue(TEXT("premessa: Dark non e' ne' vista ne' ricordata"), !BlindMoveIsLit(K, Dark))) { return false; }

	const URTHexMapAsset* Copy = URTHexSimLibrary::MakeOptimisticPlanningMap(GetTransientPackage(), Map, K);
	if (!TestNotNull(TEXT("la copia esiste"), Copy)) { return false; }
	if (!TestTrue(TEXT("ed e' un altro oggetto"), Copy != Map)) { return false; }

	const FRTHexCellData* CopyDark = Copy->FindCell(Dark);
	const FRTHexCellData* CopySeen = Copy->FindCell(Seen);
	const FRTHexCellData* CopyRemembered = Copy->FindCell(Remembered);
	if (!TestTrue(TEXT("le tre celle esistono ancora: la forma della board e' pubblica"),
		CopyDark && CopySeen && CopyRemembered)) { return false; }

	TestFalse(TEXT("Dark: non blocca piu' il movimento"), CopyDark->bBlocksMovement);
	TestFalse(TEXT("Dark: non blocca piu' la vista"), CopyDark->bBlocksLineOfSight);
	TestEqual(TEXT("Dark: superficie pavimento"), CopyDark->Surface, ERTHexSurface::Floor);
	TestEqual(TEXT("Dark: costo base"), CopyDark->TotalMoveCost(), 1);
	TestEqual(TEXT("Dark: nessuna porta"), CopyDark->Doors.Num(), 0);

	TestTrue(TEXT("Seen (solo visibile) resta autorevole: l'unione delle due liste"), CopySeen->bBlocksMovement);
	TestTrue(TEXT("Remembered (solo ricordata) resta autorevole"), CopyRemembered->bBlocksMovement);
	TestEqual(TEXT("Remembered conserva la superficie"), CopyRemembered->Surface, ERTHexSurface::Ice);

	TestEqual(TEXT("`Map` non e' stata toccata: stesso hash"), Map->ComputeHash(), HashBefore);
	TestTrue(TEXT("e il muro mai osservato e' ancora un muro nella mappa autorevole"),
		Map->FindCell(Dark) && Map->FindCell(Dark)->bBlocksMovement);
	TestEqual(TEXT("le transizioni fra piani non cambiano"), Copy->Transitions.Num(), TransitionsBefore);

	// Conoscenza senza celle: nessuna vista ottimistica, si pianifica su `Map` come prima.
	TestNull(TEXT("conoscenza vuota -> nessuna copia"),
		URTHexSimLibrary::MakeOptimisticPlanningMap(GetTransientPackage(), Map, BlindMoveKnowledge(0, {}, {})));
	return true;
}

/**
 * **Test 2** — dal percorso VERO del giocatore: `AttachPlanningMap` dota la fotografia, e il muro mai osservato
 * non piega piu' il tracciato.
 *
 * 🔴 **E' il canary di `BlindActions.NeverObservedWallBendsTheLitPath` letto dall'altra parte.** Quel banco misura
 * una fotografia senza `PlanningMap`, cioe' la vista dei bot e della Resolution, e resta verde; questo misura la
 * fotografia che `PlanningSnapshotFor` consegna al giocatore. Qui il muro in `BlindMoveWall` e' mai osservato e la
 * meta oltre di lui pure: sulla mappa autorevole il tracciato lo aggira (costo +1), sulla mappa ottimistica lo
 * attraversa.
 *
 * Le premesse sono misurate e non assunte: il muro e la meta non sono ne' visti ne' ricordati, e la squadra ha una
 * conoscenza non vuota (altrimenti `PlanningMap` resterebbe `nullptr` e il test misurerebbe l'assenza).
 *
 * ⛔ **Verifica di mutazione**: `Snapshot.PlanningMap = Snapshot.Map` in `AttachPlanningMap` rende rosso il costo; non
 * rispettare `RTObserver::Omniscient` rende rossa la Resolution.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlindMovePlayerPlansThroughTheDarkWallTest,
	"RefactorTactics.BlindMove.PlayerPlansThroughANeverObservedWall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlindMovePlayerPlansThroughTheDarkWallTest::RunTest(const FString&)
{
	UWorld* World = MakeBlindMoveWorld();
	if (!TestNotNull(TEXT("il mondo esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ DestroyBlindMoveWorld(World); };

	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(World, BlindMoveArenaRadius);
	if (!TestNotNull(TEXT("l'arena esiste"), Map)) { return false; }
	FRTHexCellData Wall(BlindMoveWall);
	Wall.bBlocksMovement = true;
	Map->AddOrUpdateCell(Wall);
	Map->SortCells();

	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Map;
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	ARTUnit* Unit = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
	if (!TestNotNull(TEXT("l'unita' esiste"), Unit)) { return false; }
	Unit->TeamId = 0;
	Unit->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeBranth());
	UGameplayStatics::FinishSpawningActor(Unit, FTransform::Identity);
	Unit->bIsBotControlled = false;
	Unit->DispatchBeginPlay();
	Unit->PlaceOnCell(BlindMoveStart, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);

	TM->RefreshTeamKnowledgeNow();

	TArray<ARTUnit*> Units;
	FRTHexSnapshot Snapshot = TM->MakeCurrentSnapshot(Units, Unit->TeamId);
	const int32 UnitId = Units.IndexOfByKey(Unit);
	if (!TestTrue(TEXT("premessa: l'unita' e' nella fotografia"), Snapshot.Units.IsValidIndex(UnitId))) { return false; }
	Snapshot.Units[UnitId].MoveBudget = 40;

	const FRTTeamKnowledge* K = Snapshot.TeamKnowledge.FindByPredicate(
		[](const FRTTeamKnowledge& X) { return X.TeamId == 0; });
	if (!TestNotNull(TEXT("premessa: la squadra ha una conoscenza"), K)) { return false; }
	if (!TestTrue(TEXT("premessa: la conoscenza non e' vuota"), K->VisibleCells.Num() > 0)) { return false; }
	if (!TestTrue(TEXT("premessa: il muro NON e' ne' visto ne' ricordato"), !BlindMoveIsLit(*K, BlindMoveWall))) { return false; }
	if (!TestTrue(TEXT("premessa: la meta NON e' ne' vista ne' ricordata"), !BlindMoveIsLit(*K, BlindMoveGoal))) { return false; }

	// Prima: la fotografia di Resolution e bot. Il muro piega il tracciato.
	TestNull(TEXT("la fotografia grezza non ha una vista ottimistica"), Snapshot.PlanningMap);
	const FRTHexPathResult Authoritative = URTHexSimLibrary::FindPathForUnit(Snapshot, UnitId, BlindMoveGoal);
	if (!TestTrue(TEXT("premessa: sulla mappa autorevole un percorso esiste"),
		Authoritative.Status == ERTHexPathStatus::Success)) { return false; }
	TestFalse(TEXT("sulla mappa autorevole il tracciato aggira il muro"), Authoritative.Path.Contains(BlindMoveWall));

	// Dopo: la fotografia del giocatore.
	TM->AttachPlanningMap(Snapshot, Unit->TeamId);
	if (!TestNotNull(TEXT("il giocatore riceve una vista ottimistica"), Snapshot.PlanningMap)) { return false; }
	TestTrue(TEXT("che non e' `Map`"), Snapshot.PlanningMap != Snapshot.Map);
	TestFalse(TEXT("e `Map` resta fresca: hash e revisione non cambiano"), URTHexSimLibrary::IsSnapshotStale(Snapshot));

	const FRTHexPathResult Optimistic = URTHexSimLibrary::FindPathForUnit(Snapshot, UnitId, BlindMoveGoal);
	if (!TestTrue(TEXT("sulla mappa ottimistica un percorso esiste"), Optimistic.Status == ERTHexPathStatus::Success)) { return false; }
	TestTrue(TEXT("il tracciato attraversa la cella del muro mai osservato"), Optimistic.Path.Contains(BlindMoveWall));
	TestTrue(TEXT("e costa meno: la deviazione non c'e'"), Optimistic.TotalCost < Authoritative.TotalCost);
	TestEqual(TEXT("e ha il costo della riga libera"), Optimistic.TotalCost,
		URTHexLibrary::HexDistance(BlindMoveStart, BlindMoveGoal));

	// La classificazione di un waypoint legge la stessa mappa: la cella del muro non e' piu' un rifiuto.
	TestEqual(TEXT("ClassifyWaypointCell non rifiuta la cella mai osservata"),
		URTHexSimLibrary::ClassifyWaypointCell(Snapshot, UnitId, BlindMoveWall), ERTHexWaypointReason::Ok);

	// La copia si RIUSA finche' nulla cambia, e non nasce per la Resolution ne' per i bot.
	const URTHexMapAsset* First = Snapshot.PlanningMap;
	TM->AttachPlanningMap(Snapshot, Unit->TeamId);
	TestTrue(TEXT("una seconda richiesta con la stessa conoscenza riusa la copia"), Snapshot.PlanningMap == First);

	TArray<ARTUnit*> ResolutionUnits;
	const FRTHexSnapshot Resolution = TM->MakeCurrentSnapshot(ResolutionUnits, RTObserver::Omniscient);
	TestNull(TEXT("la fotografia onnisciente della Resolution non ha una vista ottimistica"), Resolution.PlanningMap);
	FRTHexSnapshot Omniscient = Resolution;
	TM->AttachPlanningMap(Omniscient, RTObserver::Omniscient);
	TestNull(TEXT("e AttachPlanningMap non gliene da' una nemmeno se glielo si chiede"), Omniscient.PlanningMap);
	TestNull(TEXT("la fotografia di un bot (AsObserver) nasce senza"),
		URTHexSimLibrary::AsObserver(Snapshot, Unit->TeamId).PlanningMap);
	return true;
}

// =========================================================================================================
// Fetta B — Resolution con il muro VERO
// =========================================================================================================
//
// La mappa ottimistica serve a pianificare; la Resolution resta onnisciente e non la vede. Il giocatore porta un
// `PlannedPath`, quindi Resolution lo cammina e `TruncatePathToTopology` lo ferma dove la verita' lo ferma. Un bot
// non lo porta (`PlannedPath.Reset()`), e Resolution lo ricalcola: e' il motivo per cui [D-533] lo lascia fuori.

namespace
{
	/** Un turno con un muro vero in `Wall` e un'unita' che parte da `(0,0)` con il piano dato. */
	struct FBlindMoveResolutionRun
	{
		FRTCellId End;
		int32 StoppedByTopologyEntries = 0;
		int32 MoveRange = 0;
		bool bOk = false;
	};

	FBlindMoveResolutionRun RunMoveAgainstARealWall(const FRTCellId& Wall, const TArray<FRTCellId>& PlannedPath,
		const FRTCellId& PlannedCell)
	{
		FBlindMoveResolutionRun Out;
		UWorld* World = MakeBlindMoveWorld();
		if (!World) { return Out; }
		ON_SCOPE_EXIT{ DestroyBlindMoveWorld(World); };

		URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(World, 6);
		if (!Map) { return Out; }
		FRTHexCellData Data(Wall);
		Data.bBlocksMovement = true;
		Map->AddOrUpdateCell(Data);
		Map->SortCells();

		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = Map;
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

		ARTUnit* Unit = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!Unit || !TM) { return Out; }
		Unit->TeamId = 0;
		Unit->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeBranth());
		UGameplayStatics::FinishSpawningActor(Unit, FTransform::Identity);
		Unit->bIsBotControlled = false;
		Unit->DispatchBeginPlay();
		Unit->PlaceOnCell(FRTCellId(0, 0, 0), FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		Out.MoveRange = Unit->GetEffectiveMoveRange();

		Unit->PlannedPath = PlannedPath;
		Unit->PlannedCell = PlannedCell;

		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

		Out.End = Unit->Cell;
		for (const FRTTurnLogEntry& Entry : TM->GetTurnLog())
		{
			if (Entry.Category == ERTLogCategory::Move
				&& Entry.Outcome == static_cast<uint8>(ERTMoveOutcome::BlockedByTopology))
			{
				++Out.StoppedByTopologyEntries;
			}
		}
		Out.bOk = true;
		return Out;
	}
}

/**
 * **Test 3** — il Move che il giocatore ha pianificato attraverso un muro mai visto si FERMA davanti al muro vero, e un
 * piano senza percorso invece ricalcola.
 *
 * Il giocatore porta il `PlannedPath` che l'anteprima ottimistica gli ha fatto tracciare dritto, e la Resolution, che
 * legge la mappa autorevole, lo cammina e lo taglia dove il muro c'e': l'unita' resta sull'ultima cella valida e non
 * aggira niente ([D-045]). Il secondo caso e' il **limite dichiarato** di [D-533], e lo pinna perche' non diventi un
 * comportamento che nessuno ha scelto: senza percorso portato — un bot — Resolution chiama `FindPathForUnit`, e quindi
 * deviando raggiunge la meta senza alcun arresto per topologia.
 *
 * ⚠️ Il motivo dell'arresto resta `BlockedByTopology` (*«fermo: il varco si e' chiuso»*), che per un muro mai visto e'
 * falso: lo sistema la fetta C.
 *
 * ⛔ **Verifica di mutazione**: far ricalcolare la Resolution anche quando il piano porta un percorso (`Path =
 * FindPathForUnit(...)` al posto di `Path = Unit->PlannedPath`) rende rosso il primo caso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlindMoveResolutionStopsAtTheRealWallTest,
	"RefactorTactics.BlindMove.ResolutionStopsAtTheRealWallAndDoesNotReroute",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlindMoveResolutionStopsAtTheRealWallTest::RunTest(const FString&)
{
	const FRTCellId Wall(2, 0, 0);
	const TArray<FRTCellId> Straight = { FRTCellId(0, 0, 0), FRTCellId(1, 0, 0), FRTCellId(2, 0, 0), FRTCellId(3, 0, 0) };

	const FBlindMoveResolutionRun Carried = RunMoveAgainstARealWall(Wall, Straight, Straight.Last());
	if (!TestTrue(TEXT("il primo turno gira"), Carried.bOk)) { return false; }
	if (!TestTrue(TEXT("premessa: il budget copre il percorso dritto"), Carried.MoveRange >= 3)) { return false; }
	TestEqual(TEXT("con un percorso portato l'unita' si ferma davanti al muro vero"), Carried.End, FRTCellId(1, 0, 0));
	TestTrue(TEXT("e non lo aggira per arrivare alla meta"), Carried.End != Straight.Last());
	TestEqual(TEXT("l'arresto e' registrato dalla topologia"), Carried.StoppedByTopologyEntries, 1);

	// Il limite dichiarato: la stessa destinazione senza percorso portato.
	const FBlindMoveResolutionRun Recomputed = RunMoveAgainstARealWall(Wall, {}, Straight.Last());
	if (!TestTrue(TEXT("il secondo turno gira"), Recomputed.bOk)) { return false; }
	TestEqual(TEXT("senza percorso portato nessun arresto per topologia: Resolution ha ricalcolato"),
		Recomputed.StoppedByTopologyEntries, 0);
	TestTrue(TEXT("e l'unita' non ha mai occupato la cella del muro"), Recomputed.End != Wall);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
