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
 * Combat end-to-end su griglia esagonale (CP 6.4): la fase Blast valida portata e linea di tiro sulla
 * MAPPA esagonale (URTHexVisionLibrary) e risolve le forme con le primitive hex, non con la griglia
 * quadrata. I piani si scrivono a mano sui campi Planned*, cosi' il test programma il turno.
 */
namespace
{
	UWorld* MakeHexBlastWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyHexBlastWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Mappa esagonale di prova nel livello; le celle in SightBlockers bloccano la linea di tiro. */
	ARTHexMapActor* SpawnHexBlastMap(UWorld* World, int32 Radius, const TArray<FRTCellId>& SightBlockers = TArray<FRTCellId>())
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			FRTHexCellData Data(Id);
			Data.bBlocksLineOfSight = SightBlockers.Contains(Id);
			M->AddOrUpdateCell(Data);
		}
		M->SortCells();

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return Actor;
	}

	ARTUnit* SpawnHexBlastUnit(UWorld* World, int32 TeamId, ERTArchetype Arch, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // niente decisioni del bot in mezzo
		U->ConfigureAsArchetype(Arch);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell; // fermo: il test verifica il Blast, non il movimento
		return U;
	}

	void RunBlastTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/** Quante voci di combattimento col dato esito compaiono nel TurnLog. */
	int32 CountCombatOutcome(const ARTTurnManager* TM, ERTCombatOutcome Outcome)
	{
		int32 N = 0;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Combat && E.Outcome == static_cast<uint8>(Outcome)) { ++N; }
		}
		return N;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBlastDealsDamageTest,
	"RefactorTactics.HexBlast.AttackDealsDamageOnHexMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBlastDealsDamageTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBlastWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBlastMap(World, /*Radius=*/ 6);

	// Ranger: "Tiro" (Single, portata 6, 25 danni). Bersaglio a distanza esagonale 3, vista libera.
	ARTUnit* Shooter = SpawnHexBlastUnit(World, 0, ERTArchetype::Ranger, FRTCellId(0, 0));
	ARTUnit* Foe = SpawnHexBlastUnit(World, 1, ERTArchetype::Guardian, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Shooter || !Foe) { DestroyHexBlastWorld(World); return false; }

	const int32 HealthBefore = Foe->Health;
	const int32 ShieldBefore = Foe->Shield;
	Shooter->PlannedAbilityIndex = 0;
	Shooter->PlannedAttackTarget = Foe;

	RunBlastTurn(TM);

	TestTrue(TEXT("il bersaglio ha incassato il colpo"), Foe->Shield < ShieldBefore || Foe->Health < HealthBefore);
	TestEqual(TEXT("scudo assorbito per primo, poi HP"), Foe->Shield, FMath::Max(0, ShieldBefore - 25));
	TestEqual(TEXT("HP residui coerenti col danno"), Foe->Health, HealthBefore - FMath::Max(0, 25 - ShieldBefore));
	TestTrue(TEXT("il TurnLog registra il colpo"), CountCombatOutcome(TM, ERTCombatOutcome::Hit) >= 1);

	DestroyHexBlastWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBlastBlockedBySightTest,
	"RefactorTactics.HexBlast.NoLineOfSightOnHexMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBlastBlockedBySightTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBlastWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	// Muro esagonale sulla traiettoria (0,0) -> (3,0): la LOS deve essere valutata sull'ASSET della mappa.
	TArray<FRTCellId> Walls;
	Walls.Add(FRTCellId(1, 0));
	Walls.Add(FRTCellId(2, 0));
	SpawnHexBlastMap(World, /*Radius=*/ 6, Walls);

	ARTUnit* Shooter = SpawnHexBlastUnit(World, 0, ERTArchetype::Ranger, FRTCellId(0, 0));
	ARTUnit* Foe = SpawnHexBlastUnit(World, 1, ERTArchetype::Guardian, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Shooter || !Foe) { DestroyHexBlastWorld(World); return false; }

	const int32 HealthBefore = Foe->Health;
	const int32 ShieldBefore = Foe->Shield;
	Shooter->PlannedAbilityIndex = 0;
	Shooter->PlannedAttackTarget = Foe;

	RunBlastTurn(TM);

	TestEqual(TEXT("nessun danno attraverso la copertura"), Foe->Health, HealthBefore);
	TestEqual(TEXT("scudo intatto"), Foe->Shield, ShieldBefore);
	TestEqual(TEXT("il TurnLog spiega il perche' (nessuna linea di tiro)"),
		CountCombatOutcome(TM, ERTCombatOutcome::NoLineOfSight), 1);

	DestroyHexBlastWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBlastKnockbackTest,
	"RefactorTactics.HexBlast.KnockbackOnHexGrid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBlastKnockbackTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBlastWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBlastMap(World, /*Radius=*/ 6);

	// La "Spazzata" del Guardian (cono, portata 3) respinge di 2 celle. Bersaglio in direzione OBLIQUA NE:
	// la spinta esagonale prosegue lungo (+1,-1) fino a (3,-3), mentre quella cardinale del quadrato
	// sceglierebbe l'asse X (delta pari) e finirebbe in (3,-1). E' il caso che distingue le due geometrie.
	ARTUnit* Bruiser = SpawnHexBlastUnit(World, 1, ERTArchetype::Guardian, FRTCellId(0, 0));
	ARTUnit* Victim = SpawnHexBlastUnit(World, 0, ERTArchetype::Ranger, FRTCellId(1, -1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Bruiser || !Victim) { DestroyHexBlastWorld(World); return false; }

	Bruiser->PlannedAbilityIndex = 0; // Spazzata
	Bruiser->PlannedAttackTarget = Victim;

	RunBlastTurn(TM);

	TestTrue(TEXT("il bersaglio e' stato respinto di due celle esagonali"), Victim->Cell == FRTCellId(3, -3));
	TestTrue(TEXT("resta su una cella esistente della mappa"), URTHexLibrary::HexDistance(Victim->Cell, FRTCellId(0, 0)) <= 6);
	TestTrue(TEXT("posizione visiva coerente con la cella logica"),
		Victim->GetActorLocation().Equals(Victim->WorldForCell(Victim->Cell, FVector::ZeroVector, 100.f, 250.f), 1.0f));

	DestroyHexBlastWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBlastOutOfRangeTest,
	"RefactorTactics.HexBlast.OutOfHexRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBlastOutOfRangeTest::RunTest(const FString&)
{
	UWorld* World = MakeHexBlastWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexBlastMap(World, /*Radius=*/ 9);

	// Distanza ESAGONALE 8 > portata 6 del "Tiro": la portata si misura in celle esagonali, non in Manhattan.
	ARTUnit* Shooter = SpawnHexBlastUnit(World, 0, ERTArchetype::Ranger, FRTCellId(0, 0));
	ARTUnit* Foe = SpawnHexBlastUnit(World, 1, ERTArchetype::Guardian, FRTCellId(8, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Shooter || !Foe) { DestroyHexBlastWorld(World); return false; }

	const int32 HealthBefore = Foe->Health;
	Shooter->PlannedAbilityIndex = 0;
	Shooter->PlannedAttackTarget = Foe;

	RunBlastTurn(TM);

	TestEqual(TEXT("fuori portata: nessun danno"), Foe->Health, HealthBefore);
	TestEqual(TEXT("fuori portata non e' un problema di linea di tiro"),
		CountCombatOutcome(TM, ERTCombatOutcome::NoLineOfSight), 0);

	DestroyHexBlastWorld(World);
	return true;
}
