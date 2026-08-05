#include "Misc/AutomationTest.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLog.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

/**
 * Movimento end-to-end su griglia esagonale (CP 6.2): il turno passa dallo strato puro hex
 * (FRTHexSnapshot -> ResolveHexPaths -> BuildMoveLog) e non dal resolver quadrato.
 * I piani si scrivono a mano sui campi Planned*, cosi' il test programma il turno invece di guardarlo.
 */
namespace
{
	UWorld* MakeHexMoveWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyHexMoveWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Mappa esagonale di prova nel livello: esagono pieno di raggio Radius sul layer 0. */
	ARTHexMapActor* SpawnHexMap(UWorld* World, int32 Radius)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return Actor;
	}

	ARTUnit* SpawnHexUnit(UWorld* World, int32 TeamId, ERTArchetype Arch, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // i piani li scriviamo noi: niente decisioni del bot in mezzo
		U->ConfigureAsArchetype(Arch);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/** Porta a termine risoluzione e playback, cosi' le posizioni visive sono quelle finali. */
	void RunTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMoveReachesPlannedCellTest,
	"RefactorTactics.HexMove.UnitReachesPlannedCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMoveReachesPlannedCellTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMap(World, /*Radius=*/ 4);

	ARTUnit* Mover = SpawnHexUnit(World, 0, ERTArchetype::Ranger, FRTCellId(0, 0));
	ARTUnit* Foe = SpawnHexUnit(World, 1, ERTArchetype::Guardian, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover || !Foe) { DestroyHexMoveWorld(World); return false; }

	// Due celle a est lungo l'asse assiale: entro il budget, nessun ostacolo.
	const FRTCellId Goal(2, -1);
	Mover->PlannedCell = Goal;
	Foe->PlannedCell = Foe->Cell; // fermo

	RunTurn(TM);

	TestTrue(TEXT("l'unita' e' sulla cella pianificata"), Mover->Cell == Goal);

	// Nessuna deriva: la posizione visiva coincide con quella che la cella logica impone.
	const FVector Expected = Mover->WorldForCell(Mover->Cell, FVector::ZeroVector, 100.f, 250.f);
	TestTrue(TEXT("posizione visiva = cella logica (nessuna deriva)"),
		Mover->GetActorLocation().Equals(Expected, 1.0f));

	DestroyHexMoveWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMoveRejectsOutOfBudgetTest,
	"RefactorTactics.HexMove.RejectsUnreachableCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMoveRejectsOutOfBudgetTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMap(World, /*Radius=*/ 4);

	ARTUnit* Mover = SpawnHexUnit(World, 0, ERTArchetype::Ranger, FRTCellId(0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover) { DestroyHexMoveWorld(World); return false; }

	const FRTCellId Start = Mover->Cell;

	// Cella FUORI dalla mappa: la validazione autorevole non deve produrre alcun percorso.
	Mover->PlannedCell = FRTCellId(50, -50);
	RunTurn(TM);
	TestTrue(TEXT("destinazione inesistente -> l'unita' resta ferma"), Mover->Cell == Start);

	DestroyHexMoveWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexDashReachesCellTest,
	"RefactorTactics.HexMove.DashReachesCellOnHex",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexDashReachesCellTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMap(World, /*Radius=*/ 6);

	// Destinazione OBLIQUA (3,-3): distanza ESAGONALE 3, dentro la portata 5 dello scatto — ma distanza di
	// Manhattan 6 e coordinate negative, quindi irraggiungibile per il pathfinding quadrato. E' il caso che
	// distingue le due geometrie: se lo scatto girasse ancora sul quadrato, l'unita' resterebbe ferma.
	ARTUnit* Runner = SpawnHexUnit(World, 0, ERTArchetype::Ranger, FRTCellId(0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Runner) { DestroyHexMoveWorld(World); return false; }

	const int32 DashIdx = Runner->FindDashAbilityIndex();
	if (!TestTrue(TEXT("il Ranger ha un'abilita' di scatto"), DashIdx != INDEX_NONE))
	{
		DestroyHexMoveWorld(World);
		return false;
	}

	const FRTCellId Goal(3, -3);
	Runner->PlannedCell = Runner->Cell; // nessun movimento normale: si verifica solo lo scatto
	Runner->PlannedDashAbility = DashIdx;
	Runner->PlannedDashCell = Goal;

	RunTurn(TM);

	TestTrue(TEXT("lo scatto porta l'unita' sulla cella pianificata"), Runner->Cell == Goal);
	TestTrue(TEXT("posizione visiva = cella logica"),
		Runner->GetActorLocation().Equals(Runner->WorldForCell(Goal, FVector::ZeroVector, 100.f, 250.f), 1.0f));

	DestroyHexMoveWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexDashRejectsOutOfBudgetTest,
	"RefactorTactics.HexMove.DashRejectsOutOfBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexDashRejectsOutOfBudgetTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMap(World, /*Radius=*/ 8);

	ARTUnit* Runner = SpawnHexUnit(World, 0, ERTArchetype::Ranger, FRTCellId(0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Runner) { DestroyHexMoveWorld(World); return false; }

	const FRTCellId Start = Runner->Cell;
	Runner->PlannedCell = Start;
	Runner->PlannedDashAbility = Runner->FindDashAbilityIndex();
	Runner->PlannedDashCell = FRTCellId(7, 0); // distanza 7 > portata 5 dello scatto

	RunTurn(TM);

	TestTrue(TEXT("scatto oltre la portata: l'unita' resta ferma"), Runner->Cell == Start);

	DestroyHexMoveWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMoveContestedCellTest,
	"RefactorTactics.HexMove.ContestedCellStopsBoth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMoveContestedCellTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMap(World, /*Radius=*/ 4);

	// Due unita' equidistanti da una stessa cella: la contesa e' simultanea, nessuna delle due la ottiene.
	const FRTCellId Contested(0, 0);
	ARTUnit* A = SpawnHexUnit(World, 0, ERTArchetype::Ranger, FRTCellId(1, 0));
	ARTUnit* B = SpawnHexUnit(World, 1, ERTArchetype::Ranger, FRTCellId(-1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !A || !B) { DestroyHexMoveWorld(World); return false; }

	const FRTCellId StartA = A->Cell;
	const FRTCellId StartB = B->Cell;
	A->PlannedCell = Contested;
	B->PlannedCell = Contested;

	RunTurn(TM);

	TestTrue(TEXT("A resta ferma sulla contesa"), A->Cell == StartA);
	TestTrue(TEXT("B resta ferma sulla contesa"), B->Cell == StartB);
	TestTrue(TEXT("nessuna delle due occupa la cella contesa"), A->Cell != Contested && B->Cell != Contested);

	// L'esito deve essere spiegato nel TurnLog, non solo nella posizione finale.
	const TArray<FRTTurnLogEntry>& Log = TM->GetTurnLog();
	int32 Contests = 0;
	for (const FRTTurnLogEntry& E : Log)
	{
		if (E.Category == ERTLogCategory::Move
			&& E.Outcome == static_cast<uint8>(ERTMoveOutcome::BlockedContested))
		{
			++Contests;
		}
	}
	TestEqual(TEXT("il TurnLog registra due esiti di contesa"), Contests, 2);

	DestroyHexMoveWorld(World);
	return true;
}
