#include "Misc/AutomationTest.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Identita' STABILE dell'istanza di unita' (#405, [D-063]).
 *
 * Il TurnLog v6 porta `UnitId` dal merge di #396, ma non aveva niente da scriverci: il progetto conosceva
 * l'unita' solo come indice in `MakeCurrentSnapshot`, che filtra i vivi e si ricostruisce a ogni fase. In una
 * traccia che si rilegge a partita finita quell'indice significherebbe un'unita' diversa a ogni turno.
 *
 * Questi test non verificano che la struct abbia un campo — quello lo direbbe anche un `= 3` scritto a mano.
 * Verificano le tre proprieta' per cui il campo esiste: che l'identita' sopravviva a una morte, che non
 * dipenda dall'ordine in cui gli Actor sono stati creati, e che lo `0` resti libero per «nessuna unita'».
 */
namespace
{
	UWorld* MakeIdentityWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyIdentityWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	void SpawnIdentityMap(UWorld* World, int32 Radius)
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

	ARTUnit* SpawnIdentityUnit(UWorld* World, int32 TeamId, ERTArchetype Arch, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureAsArchetype(Arch);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = true;
		U->DispatchBeginPlay();
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/** Un turno completo sul percorso reale: pianificazione dei bot, risoluzione, playback fino in fondo. */
	void PlayOneTurn(ARTTurnManager* TM)
	{
		TM->PlanBotsForTest();
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/** Le unita' VIVE del mondo, cioe' esattamente cio' che `MakeCurrentSnapshot` indicizzerebbe. */
	TArray<ARTUnit*> LiveUnits(UWorld* World)
	{
		TArray<AActor*> Actors;
		UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), Actors);
		TArray<ARTUnit*> Out;
		for (AActor* Actor : Actors)
		{
			ARTUnit* Unit = Cast<ARTUnit>(Actor);
			if (Unit && Unit->IsAlive()) { Out.Add(Unit); }
		}
		return Out;
	}
}

/**
 * L'identita' NON si muove quando un'unita' muore. E' l'oracolo che distingue questo campo dall'indice che
 * il progetto aveva gia': fatto morire qualcuno, l'indice fra i vivi scala e il pointer sparisce
 * (`DestroyDefeatedUnits` distrugge l'Actor), mentre l'id delle superstiti deve restare quello di prima.
 *
 * La morte arriva giocando la partita vera, non azzerando `Health` a mano: una precondizione fabbricata
 * proverebbe che il campo non cambia da solo, non che regge il percorso in cui cambia tutto il resto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitIdentitySurvivesDeathTest,
	"RefactorTactics.UnitIdentity.SurvivesADeath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitIdentitySurvivesDeathTest::RunTest(const FString&)
{
	UWorld* World = MakeIdentityWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnIdentityMap(World, /*Radius=*/ 5);

	SpawnIdentityUnit(World, 0, ERTArchetype::Ranger,   FRTCellId(-4, 2));
	SpawnIdentityUnit(World, 0, ERTArchetype::Guardian, FRTCellId(-4, 3));
	SpawnIdentityUnit(World, 1, ERTArchetype::Ranger,   FRTCellId(4, -2));
	SpawnIdentityUnit(World, 1, ERTArchetype::Guardian, FRTCellId(4, -3));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("turn manager"), TM)) { DestroyIdentityWorld(World); return false; }

	PlayOneTurn(TM); // il primo turno costruisce il roster

	// Fotografia dell'identita' PRIMA di qualunque morte.
	TMap<ARTUnit*, int32> IdBefore;
	for (ARTUnit* Unit : LiveUnits(World)) { IdBefore.Add(Unit, Unit->StableUnitId); }
	const int32 AliveBefore = IdBefore.Num();
	if (!TestEqual(TEXT("si parte da quattro unita' vive"), AliveBefore, 4))
	{
		DestroyIdentityWorld(World); return false;
	}

	// Si gioca finche' qualcuno cade davvero. Il tetto e' di sicurezza: la partita 2v2 si decide molto prima.
	int32 TurnsPlayed = 0;
	while (LiveUnits(World).Num() == AliveBefore && TurnsPlayed < 40
		&& TM->GetPhase() != ERTMatchPhase::MatchEnded)
	{
		PlayOneTurn(TM);
		++TurnsPlayed;
	}
	const TArray<ARTUnit*> Survivors = LiveUnits(World);
	if (!TestTrue(TEXT("almeno un'unita' e' caduta (altrimenti il test non prova niente)"),
		Survivors.Num() < AliveBefore))
	{
		DestroyIdentityWorld(World); return false;
	}

	// L'oracolo: ogni superstite porta ANCORA il suo id di partenza.
	int32 Drifted = 0;
	for (ARTUnit* Unit : Survivors)
	{
		const int32* Before = IdBefore.Find(Unit);
		if (Before && *Before != Unit->StableUnitId) { ++Drifted; }
	}
	TestEqual(TEXT("nessun superstite ha cambiato identita' dopo la morte di un'altra unita'"), Drifted, 0);

	// E restano distinti: un'identita' che collide ha smesso di identificare.
	TSet<int32> Distinct;
	for (ARTUnit* Unit : Survivors) { Distinct.Add(Unit->StableUnitId); }
	TestEqual(TEXT("gli id dei superstiti sono ancora tutti diversi"), Distinct.Num(), Survivors.Num());

	DestroyIdentityWorld(World);
	return true;
}

/**
 * L'identita' non la decide l'ordine in cui gli Actor sono stati creati.
 *
 * `GetAllActorsOfClass` restituisce il mondo nell'ordine in cui il livello lo tiene, che non e' un dato di
 * gioco: se l'id venisse da li', due esecuzioni della stessa partita darebbero due tracce diverse — cioe'
 * esattamente la classe di difetto che `EntryLess` e `InstanceLess` esistono per impedire.
 *
 * Due mondi identici, unita' spawnate in ordine INVERSO: la stessa unita' — stessa squadra, stessa cella di
 * partenza — deve prendere lo stesso id in entrambi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitIdentityIgnoresSpawnOrderTest,
	"RefactorTactics.UnitIdentity.DoesNotDependOnSpawnOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitIdentityIgnoresSpawnOrderTest::RunTest(const FString&)
{
	struct FPlacement { int32 TeamId; ERTArchetype Arch; FRTCellId Cell; };
	const TArray<FPlacement> Placements = {
		{ 0, ERTArchetype::Ranger,   FRTCellId(-4, 2) },
		{ 0, ERTArchetype::Guardian, FRTCellId(-4, 3) },
		{ 1, ERTArchetype::Ranger,   FRTCellId(4, -2) },
		{ 1, ERTArchetype::Guardian, FRTCellId(4, -3) },
	};

	// Chiave = la cella di PARTENZA, cioe' «chi e' questa unita'» in termini di piazzamento: i due mondi hanno
	// Actor diversi, e confrontarli per indirizzo non direbbe niente. La cella corrente non servirebbe — dopo
	// un turno le unita' si sono mosse — quindi il legame Actor→piazzamento si tiene dallo spawn.
	auto RunWithOrder = [&](bool bReversed, TMap<FString, int32>& OutIds) -> bool
	{
		UWorld* World = MakeIdentityWorld();
		if (!World) { return false; }
		SpawnIdentityMap(World, /*Radius=*/ 5);

		TMap<ARTUnit*, FString> PlacedAs;
		for (int32 i = 0; i < Placements.Num(); ++i)
		{
			const FPlacement& P = Placements[bReversed ? Placements.Num() - 1 - i : i];
			if (ARTUnit* Unit = SpawnIdentityUnit(World, P.TeamId, P.Arch, P.Cell))
			{
				PlacedAs.Add(Unit, P.Cell.ToString());
			}
		}
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM) { DestroyIdentityWorld(World); return false; }

		PlayOneTurn(TM); // costruisce il roster

		for (ARTUnit* Unit : LiveUnits(World))
		{
			if (const FString* Key = PlacedAs.Find(Unit))
			{
				OutIds.Add(*Key, Unit->StableUnitId);
			}
		}
		DestroyIdentityWorld(World);
		return true;
	};

	TMap<FString, int32> Forward, Reversed;
	if (!TestTrue(TEXT("il mondo in ordine diretto gira"), RunWithOrder(false, Forward))) { return false; }
	if (!TestTrue(TEXT("il mondo in ordine inverso gira"), RunWithOrder(true, Reversed))) { return false; }

	TestEqual(TEXT("i due mondi hanno lo stesso numero di unita' identificate"), Forward.Num(), Reversed.Num());

	// PRIMA di confrontare i due mondi: gli id devono essere identita' vere. Senza questo controllo il test
	// sarebbe verde anche con tutti gli id a `0` — coinciderebbero perfettamente, e non significherebbero
	// niente. E' la trappola del test che smette di verificare restando verde.
	TSet<int32> DistinctForward;
	int32 Unassigned = 0;
	for (const TPair<FString, int32>& Pair : Forward)
	{
		if (Pair.Value <= 0) { ++Unassigned; }
		DistinctForward.Add(Pair.Value);
	}
	TestEqual(TEXT("ogni unita' ha un'identita' assegnata (nessuno zero)"), Unassigned, 0);
	TestEqual(TEXT("le identita' sono tutte diverse fra loro"), DistinctForward.Num(), Forward.Num());

	int32 Mismatched = 0;
	for (const TPair<FString, int32>& Pair : Forward)
	{
		const int32* Other = Reversed.Find(Pair.Key);
		if (!Other || *Other != Pair.Value) { ++Mismatched; }
	}
	TestEqual(TEXT("la stessa unita' prende lo stesso id in entrambi gli ordini di spawn"), Mismatched, 0);
	return true;
}

/**
 * Gli id sono `1..N`, e lo `0` resta libero.
 *
 * Non e' estetica: [D-063] assegna allo `0` il significato «nessuna unita' dichiarata», ed e' cio' che una
 * voce ambientale del TurnLog dice davvero. Se il roster partisse da `0`, la prima unita' della partita e il
 * fuoco che si spegne da solo scriverebbero lo stesso numero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitIdentityStartsAtOneTest,
	"RefactorTactics.UnitIdentity.StartsAtOneSoZeroMeansNoUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitIdentityStartsAtOneTest::RunTest(const FString&)
{
	UWorld* World = MakeIdentityWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnIdentityMap(World, /*Radius=*/ 4);

	SpawnIdentityUnit(World, 0, ERTArchetype::Ranger,   FRTCellId(-3, 1));
	SpawnIdentityUnit(World, 1, ERTArchetype::Guardian, FRTCellId(3, -1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("turn manager"), TM)) { DestroyIdentityWorld(World); return false; }

	PlayOneTurn(TM);

	TArray<int32> Ids;
	for (ARTUnit* Unit : LiveUnits(World)) { Ids.Add(Unit->StableUnitId); }
	Ids.Sort();

	TestEqual(TEXT("due unita' identificate"), Ids.Num(), 2);
	if (Ids.Num() == 2)
	{
		TestEqual(TEXT("il primo id e' 1, non 0"), Ids[0], 1);
		TestEqual(TEXT("il secondo id e' 2"), Ids[1], 2);
	}

	DestroyIdentityWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
