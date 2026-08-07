#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * CP 8.4 — fuoco e acqua si annullano, e la mappa cambia stato **nel TurnLog**.
 *
 * Il pezzo nuovo e' il TERRENO DINAMICO: fino a CP 8.3 la mappa era immutabile in partita. La superficie
 * corrente vive nella mappa (che tutti leggono gia'), la superficie originale e la durata nel `TurnManager`.
 *
 * Prefissi `Fw*` negli helper: unity build, namespace anonimi fusi con gli altri file di test.
 */
namespace
{
	UWorld* MakeFwWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyFwWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTHexMapActor* SpawnFwMap(UWorld* World, const TMap<FRTCellId, ERTHexSurface>& Surfaces, int32 Radius = 4)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			FRTHexCellData Cell(Id);
			if (const ERTHexSurface* Surface = Surfaces.Find(Id))
			{
				Cell.Surface = *Surface;
				Cell.MoveCost = URTTerrainLibrary::FindTerrainDef(*Surface).MoveCost;
			}
			M->AddOrUpdateCell(Cell);
		}
		M->SortCells();

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return Actor;
	}

	ARTUnit* SpawnFwUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false;
		U->ConfigureAsArchetype(ERTArchetype::Ranger);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell;
		return U;
	}

	/** Mette un'azione ambientale del catalogo nello slot 3 (lo scatto) e la pianifica sul bersaglio. */
	void PlanFwEnvironmentAction(ARTUnit* Caster, const TCHAR* ActionId, ARTUnit* Target)
	{
		URTActionData* Action = NewObject<URTActionData>(Caster);
		Action->Def = URTCatalogLibrary::FindCoreAction(FName(ActionId));
		Action->RangeCells = Action->Def.RangeCells;
		Action->CooldownTurns = Action->Def.CooldownTurns;
		Action->Power = URTCatalogLibrary::FirstDamage(Action->Def);
		Caster->Abilities[3] = Action;
		Caster->PlannedAbilityIndex = 3;
		Caster->PlannedAttackTarget = Target;
	}

	void RunFwTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }
	}

	ERTHexSurface FwSurfaceAt(const ARTHexMapActor* MapActor, const FRTCellId& Cell)
	{
		const FRTHexCellData* Data = MapActor && MapActor->MapAsset ? MapActor->MapAsset->FindCell(Cell) : nullptr;
		return Data ? Data->Surface : ERTHexSurface::Floor;
	}

	int32 CountFwEnvironmentEntries(const ARTTurnManager* TM, ERTEnvironmentOutcome Outcome)
	{
		int32 N = 0;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Environment && E.Outcome == static_cast<uint8>(Outcome)) { ++N; }
		}
		return N;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWaterExtinguishesFireTest,
	"RefactorTactics.Environment.WaterExtinguishesFire",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWaterExtinguishesFireTest::RunTest(const FString&)
{
	// **Nome vincolante** del catalogo §15. L'acqua che arriva su una cella in fiamme la spegne: la cella
	// diventa acqua, e il TurnLog lo dice con un esito PROPRIO — «si allaga» e «si spegne» non sono lo stesso
	// evento per chi legge il replay.
	UWorld* World = MakeFwWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	TMap<FRTCellId, ERTHexSurface> Surfaces;
	Surfaces.Add(FRTCellId(1, 0), ERTHexSurface::Fire); // la cella brucia gia'
	ARTHexMapActor* MapActor = SpawnFwMap(World, Surfaces);

	ARTUnit* Caster = SpawnFwUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Target = SpawnFwUnit(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Caster"), Caster) || !TestNotNull(TEXT("Target"), Target) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyFwWorld(World);
		return false;
	}

	TestTrue(TEXT("si parte da una cella in fiamme"),
		FwSurfaceAt(MapActor, FRTCellId(1, 0)) == ERTHexSurface::Fire);

	PlanFwEnvironmentAction(Caster, TEXT("Action.CreateWater"), Target);
	RunFwTurn(TM);

	TestTrue(TEXT("la cella e' diventata acqua"),
		FwSurfaceAt(MapActor, FRTCellId(1, 0)) == ERTHexSurface::ShallowWater);
	TestEqual(TEXT("e il TurnLog lo registra come «spento», non come un allagamento qualunque"),
		CountFwEnvironmentEntries(TM, ERTEnvironmentOutcome::SurfaceExtinguished), 1);

	DestroyFwWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFireDoesNotIgniteWaterOrMetalTest,
	"RefactorTactics.Environment.Fire.DoesNotIgniteWaterOrMetal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFireDoesNotIgniteWaterOrMetalTest::RunTest(const FString&)
{
	// Il catalogo terreni §2: il fuoco «non incendia automaticamente acqua o metallo». E' una proprieta' delle
	// SUPERFICI, non un elenco di eccezioni nel resolver — cosi' una superficie nuova dichiara da se' se brucia.
	TestFalse(TEXT("l'acqua non brucia"),
		URTTerrainLibrary::FindTerrainDef(ERTHexSurface::ShallowWater).bIsFlammable);
	TestFalse(TEXT("il metallo nemmeno"),
		URTTerrainLibrary::FindTerrainDef(ERTHexSurface::Conductive).bIsFlammable);
	TestTrue(TEXT("il terreno neutro si'"),
		URTTerrainLibrary::FindTerrainDef(ERTHexSurface::Floor).bIsFlammable);

	// **Limite dichiarato del v0.1**: il catalogo elenca come combustibili vegetazione, olio e gas, che NON
	// esistono fra le otto superfici. Il fuoco quindi non ha di che propagarsi da solo: la propagazione e'
	// vuota **per costruzione del catalogo**, non per codice mancante. Il giorno in cui una superficie
	// combustibile verra' aggiunta, questo conteggio cambiera' e il test lo dira'.
	int32 Flammable = 0;
	for (const FRTTerrainDef& Def : URTTerrainLibrary::GetTerrainCatalog())
	{
		if (Def.bIsFlammable) { ++Flammable; }
	}
	TestEqual(TEXT("due sole superfici combustibili nel v0.1 (Floor, Rough)"), Flammable, 2);

	// In partita: incendiare una cella d'acqua non la incendia, e il rifiuto e' REGISTRATO — un'azione che
	// non fa nulla in silenzio e' peggio di una che fallisce a voce alta.
	UWorld* World = MakeFwWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	TMap<FRTCellId, ERTHexSurface> Surfaces;
	Surfaces.Add(FRTCellId(1, 0), ERTHexSurface::ShallowWater);
	ARTHexMapActor* MapActor = SpawnFwMap(World, Surfaces);

	ARTUnit* Caster = SpawnFwUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Target = SpawnFwUnit(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Caster"), Caster) || !TestNotNull(TEXT("Target"), Target) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyFwWorld(World);
		return false;
	}

	PlanFwEnvironmentAction(Caster, TEXT("Action.Ignite"), Target);
	RunFwTurn(TM);

	TestTrue(TEXT("l'acqua resta acqua"),
		FwSurfaceAt(MapActor, FRTCellId(1, 0)) == ERTHexSurface::ShallowWater);
	TestEqual(TEXT("e il rifiuto compare nel TurnLog"),
		CountFwEnvironmentEntries(TM, ERTEnvironmentOutcome::SurfaceRejected), 1);

	DestroyFwWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEnvironmentChangesInTurnLogTest,
	"RefactorTactics.Environment.ChangesAppearInTurnLog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEnvironmentChangesInTurnLogTest::RunTest(const FString&)
{
	// Ogni modifica ambientale e' osservabile: accensione, durata e ritorno alla superficie originale. Senza,
	// un'unita' che a T+2 incassa 10 danni «senza motivo» resta inspiegabile nel replay.
	UWorld* World = MakeFwWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTHexMapActor* MapActor = SpawnFwMap(World, {}); // tutto pavimento: combustibile
	ARTUnit* Caster = SpawnFwUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Target = SpawnFwUnit(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Caster"), Caster) || !TestNotNull(TEXT("Target"), Target) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyFwWorld(World);
		return false;
	}

	PlanFwEnvironmentAction(Caster, TEXT("Action.Ignite"), Target);
	RunFwTurn(TM);

	TestTrue(TEXT("la cella ha preso fuoco"), FwSurfaceAt(MapActor, FRTCellId(1, 0)) == ERTHexSurface::Fire);
	TestEqual(TEXT("il cambio e' nel TurnLog"),
		CountFwEnvironmentEntries(TM, ERTEnvironmentOutcome::SurfaceChanged), 1);

	// La voce porta la cella e la CAUSA: un cambiamento anonimo non spiega niente.
	bool bHasCauseAndCell = false;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category == ERTLogCategory::Environment
			&& E.ActionId == FName(TEXT("Action.Ignite"))
			&& E.TgtCell == FRTCellId(1, 0)
			&& E.Amount == 2) // i turni di durata
		{
			bHasCauseAndCell = true;
		}
	}
	TestTrue(TEXT("con cella, causa e durata"), bHasCauseAndCell);

	// Due turni dopo la cella torna com'era, e anche questo si registra.
	RunFwTurn(TM);
	TestTrue(TEXT("dopo un turno brucia ancora"), FwSurfaceAt(MapActor, FRTCellId(1, 0)) == ERTHexSurface::Fire);
	RunFwTurn(TM);
	TestTrue(TEXT("scaduta la durata, la cella torna pavimento"),
		FwSurfaceAt(MapActor, FRTCellId(1, 0)) == ERTHexSurface::Floor);
	TestEqual(TEXT("il ripristino e' registrato"),
		CountFwEnvironmentEntries(TM, ERTEnvironmentOutcome::SurfaceRestored), 1);

	DestroyFwWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
