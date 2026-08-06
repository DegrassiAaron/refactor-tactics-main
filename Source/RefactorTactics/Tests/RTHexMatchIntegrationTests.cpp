#include "Misc/AutomationTest.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTTurnRules.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

/**
 * Partita 2v2 COMPLETA su griglia esagonale, dal primo turno alla vittoria (CP 6.8, parte headless).
 *
 * I checkpoint precedenti verificano una fase per volta; qui si verifica che il turno REGGA ripetuto: bot che
 * pianificano, unita' che muoiono, partita che finisce. E' la prova che il playtest in PIE non puo' dare
 * (nessuno gioca 30 turni a mano per vedere se al dodicesimo qualcosa si rompe) e che il playtest non
 * sostituisce (a schermo si vede la leggibilita', qui la tenuta).
 */
namespace
{
	UWorld* MakeHexMatchWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyHexMatchWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	URTHexMapAsset* SpawnHexMatchMap(UWorld* World, int32 Radius)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return M;
	}

	ARTUnit* SpawnHexMatchUnit(UWorld* World, int32 TeamId, ERTArchetype Arch, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureAsArchetype(Arch);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = true; // 2v2 bot contro bot: nessuna mano umana, la partita si gioca da sola
		// Senza BeginPlay i cooldown non vengono inizializzati (`AbilityCooldowns` resta vuoto) e OGNI abilita'
		// risulta sempre pronta: una partita di prova che non lo chiama misura un gioco che non esiste.
		U->DispatchBeginPlay();
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/** Un turno completo: pianificazione dei bot, risoluzione e playback fino in fondo. */
	void PlayOneTurn(ARTTurnManager* TM)
	{
		TM->PlanBotsForTest();
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexFullMatchTest,
	"RefactorTactics.HexMatch.PlaysToCompletion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexFullMatchTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMatchWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	URTHexMapAsset* Map = SpawnHexMatchMap(World, /*Radius=*/ 5);

	// 2v2 su lati opposti dell'arena, in diagonale (dove la distanza esagonale conta davvero).
	ARTUnit* A1 = SpawnHexMatchUnit(World, 0, ERTArchetype::Ranger,   FRTCellId(-4, 2));
	ARTUnit* A2 = SpawnHexMatchUnit(World, 0, ERTArchetype::Guardian, FRTCellId(-4, 3));
	ARTUnit* B1 = SpawnHexMatchUnit(World, 1, ERTArchetype::Ranger,   FRTCellId(4, -2));
	ARTUnit* B2 = SpawnHexMatchUnit(World, 1, ERTArchetype::Guardian, FRTCellId(4, -3));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !A1 || !A2 || !B1 || !B2) { DestroyHexMatchWorld(World); return false; }

	// Tetto di sicurezza, non una regola di gioco: serve a fallire invece di girare all'infinito.
	// MISURATO (2026-08-06): con gli archetipi attuali questa partita si decide al **turno 25**. Il catalogo
	// v0.1 prevede un limite di 12 turni: o il bilanciamento si accorcia (E6), o la fine partita a tre vie
	// (CP 10.3) deve decidere chi vince allo scadere — vedi issue di bilanciamento aperta al CP 6.8.
	const int32 MaxTurns = 40;
	int32 TurnsPlayed = 0;
	while (TM->GetPhase() != ERTMatchPhase::MatchEnded && TurnsPlayed < MaxTurns)
	{
		PlayOneTurn(TM);
		++TurnsPlayed;
		// INVARIANTE DI TURNO: nessuna unita' viva finisce fuori dalla mappa o sopra un'altra.
		TArray<AActor*> Actors;
		UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), Actors);
		TSet<FRTCellId> Occupied;
		for (AActor* Actor : Actors)
		{
			const ARTUnit* Unit = Cast<ARTUnit>(Actor);
			if (!Unit || !Unit->IsAlive()) { continue; }
			TestTrue(FString::Printf(TEXT("turno %d: %s e' su una cella della mappa"), TurnsPlayed, *Unit->GetName()),
				Map->ContainsCell(Unit->Cell));
			TestFalse(FString::Printf(TEXT("turno %d: nessuna sovrapposizione su %s"), TurnsPlayed, *Unit->Cell.ToString()),
				Occupied.Contains(Unit->Cell));
			Occupied.Add(Unit->Cell);
		}
	}

	TestTrue(TEXT("la partita si e' decisa entro il limite di turni"), TM->GetPhase() == ERTMatchPhase::MatchEnded);
	TestTrue(TEXT("sono stati giocati piu' turni (non si e' decisa al primo)"), TurnsPlayed > 1);

	// Una squadra e' stata eliminata: e' la condizione di vittoria del vertical slice attuale.
	int32 Team0Alive = 0, Team1Alive = 0;
	TArray<AActor*> Survivors;
	UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), Survivors);
	for (AActor* Actor : Survivors)
	{
		const ARTUnit* Unit = Cast<ARTUnit>(Actor);
		if (Unit && Unit->IsAlive()) { (Unit->TeamId == 0 ? Team0Alive : Team1Alive)++; }
	}
	TestTrue(TEXT("una delle due squadre e' stata eliminata"), Team0Alive == 0 || Team1Alive == 0);

	DestroyHexMatchWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMatchLogTest,
	"RefactorTactics.HexMatch.TurnLogExplainsTheTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMatchLogTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMatchWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMatchMap(World, /*Radius=*/ 4);

	ARTUnit* A = SpawnHexMatchUnit(World, 0, ERTArchetype::Ranger, FRTCellId(-3, 1));
	ARTUnit* B = SpawnHexMatchUnit(World, 1, ERTArchetype::Guardian, FRTCellId(3, -1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !A || !B) { DestroyHexMatchWorld(World); return false; }

	PlayOneTurn(TM);

	// Osservabilita': il turno deve lasciare una traccia leggibile, con celle in coordinate assiali.
	const TArray<FRTTurnLogEntry>& Log = TM->GetTurnLog();
	TestTrue(TEXT("il turno ha prodotto voci di TurnLog"), Log.Num() > 0);
	for (const FRTTurnLogEntry& Entry : Log)
	{
		const FString Text = URTTurnLogLibrary::DescribeEntry(Entry);
		TestFalse(TEXT("ogni voce ha una descrizione leggibile"), Text.IsEmpty());
		TestTrue(TEXT("la descrizione riporta le coordinate assiali"), Text.Contains(TEXT("q=")));
	}

	// L'ordinamento e' totale e deterministico: rileggere il log ordinato non lo cambia.
	TArray<FRTTurnLogEntry> Copy = Log;
	URTTurnLogLibrary::SortTurnLog(Copy);
	TestEqual(TEXT("il TurnLog e' gia' in ordine canonico"),
		URTTurnLogLibrary::HashTurnLog(Copy), URTTurnLogLibrary::HashTurnLog(Log));

	DestroyHexMatchWorld(World);
	return true;
}
