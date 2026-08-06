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
#include "Ability/RTActionData.h"
#include "Map/RTHexVisionLibrary.h"
#include "Pathfinding/RTHexPathLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"
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
	// MISURATO (2026-08-06): la partita si decide al **turno 10**, dentro il limite di 12 del catalogo v0.1.
	// Era 25 finche' lo scudo delle abilita' di supporto non scadeva e si accumulava (issue #96).
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

/**
 * Un turno in cui ENTRAMBE le squadre si muovono e agiscono, sulla mappa di prova generata da codice.
 *
 * I test precedenti guardano una fase o una squadra per volta; qui si verifica il caso che il playtest esercita
 * davvero: quattro unita' che nello stesso turno attaccano e si spostano, con la risoluzione simultanea a
 * decidere l'esito. I piani sono ESPLICITI per tutte e quattro (nessun bot): un test che dipendesse dall'utility
 * del bot verificherebbe le sue preferenze, non la tenuta del turno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBothTeamsActTest,
	"RefactorTactics.HexMatch.BothTeamsMoveAndActOnTestArena",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBothTeamsActTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMatchWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	if (!TestNotNull(TEXT("arena di prova generata"), Arena)) { DestroyHexMatchWorld(World); return false; }
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;

	// Tutti e quattro sullo stesso lato dei muri centrali, a portata reciproca: qui si vuole che gli attacchi
	// siano LEGALI, non provare la copertura (quella e' HexVision/PIE-HEXPLAY-6).
	ARTUnit* A_Shooter = SpawnHexMatchUnit(World, 0, ERTArchetype::Ranger,   FRTCellId(1, 1));
	ARTUnit* A_Mover   = SpawnHexMatchUnit(World, 0, ERTArchetype::Guardian, FRTCellId(1, 2));
	ARTUnit* B_Shooter = SpawnHexMatchUnit(World, 1, ERTArchetype::Ranger,   FRTCellId(3, 0));
	ARTUnit* B_Mover   = SpawnHexMatchUnit(World, 1, ERTArchetype::Guardian, FRTCellId(4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !A_Shooter || !A_Mover || !B_Shooter || !B_Mover)
	{
		DestroyHexMatchWorld(World);
		return false;
	}

	// Piani espliciti: nessuna unita' guidata dal bot in questo test.
	for (ARTUnit* U : { A_Shooter, A_Mover, B_Shooter, B_Mover }) { U->bIsBotControlled = false; }

	const FRTCellId AMoverFrom = A_Mover->Cell;
	const FRTCellId BMoverFrom = B_Mover->Cell;
	const FRTCellId AMoverTo(0, 3);
	const FRTCellId BMoverTo(4, -1);
	const int32 BShooterHealthBefore = B_Shooter->Health + B_Shooter->Shield;
	const int32 AShooterHealthBefore = A_Shooter->Health + A_Shooter->Shield;

	// Premessa: i due tiratori si vedono e sono a portata, altrimenti il test non prova quel che dice.
	TestTrue(TEXT("premessa: i tiratori si vedono"),
		URTHexVisionLibrary::HasLineOfSight(Arena, A_Shooter->Cell, B_Shooter->Cell));

	// Squadra 0: uno attacca, l'altro si muove. Squadra 1: idem, nello stesso turno.
	A_Shooter->PlannedAbilityIndex = 0;               // "Tiro"
	A_Shooter->PlannedAttackTarget = B_Shooter;
	A_Mover->PlannedCell = AMoverTo;
	B_Shooter->PlannedAbilityIndex = 0;
	B_Shooter->PlannedAttackTarget = A_Shooter;
	B_Mover->PlannedCell = BMoverTo;

	TM->LockInAndResolve();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
	{
		TM->Tick(0.05f);
	}

	// ENTRAMBE le squadre hanno agito: ciascun tiratore ha tolto punti all'avversario.
	TestTrue(TEXT("la squadra 0 ha colpito la squadra 1"),
		(B_Shooter->Health + B_Shooter->Shield) < BShooterHealthBefore);
	TestTrue(TEXT("la squadra 1 ha colpito la squadra 0"),
		(A_Shooter->Health + A_Shooter->Shield) < AShooterHealthBefore);

	// ENTRAMBE le squadre si sono mosse, ognuna sulla cella pianificata.
	TestTrue(TEXT("la squadra 0 si e' mossa"), A_Mover->Cell != AMoverFrom);
	TestTrue(TEXT("la squadra 1 si e' mossa"), B_Mover->Cell != BMoverFrom);
	TestTrue(TEXT("la squadra 0 e' arrivata dove aveva pianificato"), A_Mover->Cell == AMoverTo);
	TestTrue(TEXT("la squadra 1 e' arrivata dove aveva pianificato"), B_Mover->Cell == BMoverTo);

	// Nessuno finisce dentro un ostacolo: la mappa di prova ne ha, quindi la regola e' esercitata davvero.
	for (const ARTUnit* U : { A_Shooter, A_Mover, B_Shooter, B_Mover })
	{
		const FRTHexCellData* Data = Arena->FindCell(U->Cell);
		TestTrue(TEXT("nessuna unita' finisce su una cella che blocca il movimento"),
			Data != nullptr && !Data->bBlocksMovement);
	}

	// Il turno e' spiegato: il log contiene sia il movimento sia il combattimento.
	const TArray<FRTTurnLogEntry>& Log = TM->GetTurnLog();
	bool bHasMove = false;
	bool bHasCombat = false;
	for (const FRTTurnLogEntry& E : Log)
	{
		bHasMove   |= (E.Category == ERTLogCategory::Move);
		bHasCombat |= (E.Category == ERTLogCategory::Combat);
	}
	TestTrue(TEXT("il TurnLog riporta il movimento"), bHasMove);
	TestTrue(TEXT("il TurnLog riporta il combattimento"), bHasCombat);

	DestroyHexMatchWorld(World);
	return true;
}

/**
 * Caccia alle ANOMALIE su piu' turni della mappa di prova: non verifica un esito voluto, verifica che non
 * accada nulla di illegale. E' il complemento del playtest — a schermo si nota che "qualcosa e' strano", qui si
 * dice esattamente cosa, e a ogni turno.
 *
 * Gli invarianti sono quelli che un occhio umano non riesce a controllare turno per turno: nessuno dentro un
 * ostacolo, nessuno fuori mappa, nessuna sovrapposizione, e — il piu' importante su una mappa multilivello —
 * nessun cambio di layer che il grafo non consenta (cioe' nessun teletrasporto sulla piattaforma).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexArenaAnomalyTest,
	"RefactorTactics.HexMatch.TestArenaKeepsUnitsOnLegalCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexArenaAnomalyTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMatchWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	if (!TestNotNull(TEXT("arena di prova"), Arena)) { DestroyHexMatchWorld(World); return false; }
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;

	// Le squadre partono agli estremi, come in partita: e' la configurazione che il playtest esercita.
	const TArray<FRTCellId> Start = URTMatchSetupLibrary::PickStartCells(Arena, /*NumPerTeam=*/ 2, /*Layer=*/ 0);
	if (!TestEqual(TEXT("quattro celle di partenza"), Start.Num(), 4)) { DestroyHexMatchWorld(World); return false; }

	TArray<ARTUnit*> Units;
	Units.Add(SpawnHexMatchUnit(World, 0, ERTArchetype::Ranger,   Start[0]));
	Units.Add(SpawnHexMatchUnit(World, 0, ERTArchetype::Guardian, Start[1]));
	Units.Add(SpawnHexMatchUnit(World, 1, ERTArchetype::Ranger,   Start[2]));
	Units.Add(SpawnHexMatchUnit(World, 1, ERTArchetype::Guardian, Start[3]));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || Units.Contains(nullptr)) { DestroyHexMatchWorld(World); return false; }

	TMap<ARTUnit*, FRTCellId> Previous;
	for (ARTUnit* U : Units) { Previous.Add(U, U->Cell); }

	int32 Turns = 0;
	int32 LayerChanges = 0;
	while (TM->GetPhase() != ERTMatchPhase::MatchEnded && Turns < 12)
	{
		PlayOneTurn(TM);
		++Turns;

		TSet<FRTCellId> Occupied;
		for (ARTUnit* U : Units)
		{
			if (!U->IsAlive()) { continue; }

			const FRTHexCellData* Data = Arena->FindCell(U->Cell);
			TestNotNull(*FString::Printf(TEXT("turno %d: %s sta su una cella della mappa"), Turns, *U->GetName()),
				Data);
			if (Data)
			{
				TestFalse(*FString::Printf(TEXT("turno %d: %s non sta dentro un ostacolo"), Turns, *U->GetName()),
					Data->bBlocksMovement);
			}

			bool bAlreadyThere = false;
			Occupied.Add(U->Cell, &bAlreadyThere);
			TestFalse(*FString::Printf(TEXT("turno %d: nessuna sovrapposizione su %s"), Turns, *U->Cell.ToString()),
				bAlreadyThere);

			// Cambio di layer: deve essere consentito dal GRAFO, cioe' passare da una transizione. Se il path
			// fra la cella di prima e quella di adesso non esiste, l'unita' e' arrivata dove non poteva.
			const FRTCellId& Before = Previous[U];
			if (Before.Layer != U->Cell.Layer)
			{
				++LayerChanges;
				TestTrue(*FString::Printf(TEXT("turno %d: %s cambia layer per una via legale (%s -> %s)"),
						Turns, *U->GetName(), *Before.ToString(), *U->Cell.ToString()),
					URTHexPathLibrary::FindPath(Arena, Before, U->Cell).Status == ERTHexPathStatus::Success);
			}
			Previous[U] = U->Cell;
		}
	}

	TestTrue(TEXT("la partita e' andata avanti per piu' turni"), Turns > 1);
	AddInfo(FString::Printf(TEXT("turni giocati: %d, cambi di layer osservati: %d"), Turns, LayerChanges));

	DestroyHexMatchWorld(World);
	return true;
}

/**
 * Movimento fra layer: l'unica via per la piattaforma e' la transizione, e togliendola non si "sale" comunque.
 * Copre headless PIE-HEXPLAY-8; al PIE resta da guardare che il playback porti l'unita' alla quota giusta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexClimbViaTransitionTest,
	"RefactorTactics.HexMove.ClimbsOnlyThroughTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexClimbViaTransitionTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMatchWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	if (!TestNotNull(TEXT("arena di prova"), Arena)) { DestroyHexMatchWorld(World); return false; }
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;

	const FRTCellId Ground(1, 0, 0);    // il piede della transizione
	const FRTCellId Platform(2, 0, 1);  // la piattaforma, un layer sopra

	// Uno scalatore e un avversario lontano (serve solo a non far finire la partita al primo turno).
	ARTUnit* Climber = SpawnHexMatchUnit(World, 0, ERTArchetype::Ranger, Ground);
	ARTUnit* Idle    = SpawnHexMatchUnit(World, 1, ERTArchetype::Guardian, FRTCellId(-4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Climber || !Idle) { DestroyHexMatchWorld(World); return false; }
	Climber->bIsBotControlled = false;
	Idle->bIsBotControlled = false;

	// Premessa: la piattaforma e' su un altro layer e il grafo la collega.
	TestEqual(TEXT("premessa: partenza sul layer 0"), Climber->Cell.Layer, 0);
	TestTrue(TEXT("premessa: il grafo collega terra e piattaforma"),
		URTHexPathLibrary::FindPath(Arena, Ground, Platform).Status == ERTHexPathStatus::Success);

	Climber->PlannedCell = Platform;
	PlayOneTurn(TM);

	TestTrue(TEXT("l'unita' e' salita sulla piattaforma"), Climber->Cell == Platform);
	TestEqual(TEXT("ora sta sul layer 1"), Climber->Cell.Layer, 1);

	// Togliamo l'arco e riproviamo dalla stessa posizione: senza transizione non si sale.
	Arena->RemoveTransition(Ground, Platform);
	Climber->PlaceOnCell(Ground, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
	Climber->PlannedWaypoints.Reset();
	Climber->PlannedPath.Reset();
	Climber->PlannedCell = Platform;
	PlayOneTurn(TM);

	TestEqual(TEXT("senza la transizione l'unita' resta sul layer 0"), Climber->Cell.Layer, 0);
	TestTrue(TEXT("e non e' finita sulla piattaforma"), Climber->Cell != Platform);

	DestroyHexMatchWorld(World);
	return true;
}

/**
 * Lo scatto non entra in una cella che blocca il movimento: la destinazione viene rifiutata e l'unita' resta.
 * Copre headless la parte verificabile di PIE-V01-DASHCOVER.
 *
 * Nota di scope: qui si verifica il comportamento ATTUALE (lo scatto usa l'A* sul grafo, quindi aggira gli
 * ostacoli). Il catalogo v0.1 vuole uno scatto LINEARE che non attraversa muri: e' la migrazione di CP 4.5
 * (issue #46) e va verificata quando c'e', non asserita qui in anticipo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexDashBlockedTest,
	"RefactorTactics.HexMove.DashRefusesBlockedDestination",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexDashBlockedTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMatchWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	if (!TestNotNull(TEXT("arena di prova"), Arena)) { DestroyHexMatchWorld(World); return false; }
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;

	// (2,1) e' uno degli ostacoli della mappa di prova.
	const FRTCellId Blocked(2, 1, 0);
	const FRTHexCellData* BlockedData = Arena->FindCell(Blocked);
	TestTrue(TEXT("premessa: la cella di prova blocca il movimento"),
		BlockedData != nullptr && BlockedData->bBlocksMovement);

	const FRTCellId From(2, 3, 0);
	ARTUnit* Dasher = SpawnHexMatchUnit(World, 0, ERTArchetype::Ranger, From);
	ARTUnit* Idle   = SpawnHexMatchUnit(World, 1, ERTArchetype::Guardian, FRTCellId(-4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Dasher || !Idle) { DestroyHexMatchWorld(World); return false; }
	Dasher->bIsBotControlled = false;
	Idle->bIsBotControlled = false;

	// Lo Scatto del Ranger e' la quarta abilita' (indice 3): portata 5, ricarica 2.
	const int32 DashIdx = 3;
	const URTActionData* Dash = Dasher->GetAbility(DashIdx);
	if (!TestNotNull(TEXT("premessa: il Ranger ha lo scatto"), (void*)Dash)) { DestroyHexMatchWorld(World); return false; }
	TestTrue(TEXT("premessa: e' un'abilita' di mobilita' rapida"), Dash->bDash);
	TestTrue(TEXT("premessa: la cella bloccata e' entro la portata dello scatto"),
		URTHexLibrary::HexDistance(From, Blocked) <= Dash->RangeCells);

	Dasher->PlannedDashAbility = DashIdx;
	Dasher->PlannedDashCell = Blocked;
	PlayOneTurn(TM);

	TestTrue(TEXT("lo scatto non entra nella cella bloccata"), Dasher->Cell != Blocked);
	TestEqual(TEXT("l'unita' resta dov'era"), Dasher->Cell.ToString(), From.ToString());

	DestroyHexMatchWorld(World);
	return true;
}

/**
 * Invariante del bot: se pianifica uno scatto, quello scatto si DEVE poter eseguire. Vale da quando lo scatto e'
 * lineare (CP 4.5): le candidate del bot nascono da `ReachableCells`, che segue il grafo, quindi senza un filtro
 * il bot proporrebbe destinazioni che il resolver rifiuta — sprecando l'abilita' senza dirlo a nessuno.
 *
 * La mappa mette ostacoli attorno al bot, cosi' molte celle raggiungibili sul grafo NON sono in linea: e'
 * proprio il caso in cui il difetto si manifesta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotDashExecutableTest,
	"RefactorTactics.HexBotPlay.PlannedDashIsAlwaysExecutable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotDashExecutableTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMatchWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	// Mappa con ostacoli sparsi: costringe i percorsi a deviare, cosi' "raggiungibile" != "in linea".
	URTHexMapAsset* Map = SpawnHexMatchMap(World, /*Radius=*/ 4);
	for (const FRTCellId& Id : { FRTCellId(1, 0), FRTCellId(1, -1), FRTCellId(0, 1), FRTCellId(-1, 2) })
	{
		FRTHexCellData Blocked(Id);
		Blocked.bBlocksMovement = true;
		Map->AddOrUpdateCell(Blocked);
	}
	Map->SortCells();

	// Un kiter bot (ha lo Scatto) e un avversario che gli si avvicina: e' la situazione che innesca il dash.
	ARTUnit* Bot   = SpawnHexMatchUnit(World, 1, ERTArchetype::Ranger,   FRTCellId(0, 0));
	ARTUnit* Enemy = SpawnHexMatchUnit(World, 0, ERTArchetype::Guardian, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Bot || !Enemy) { DestroyHexMatchWorld(World); return false; }
	Enemy->bIsBotControlled = false;

	int32 DashesPlanned = 0;
	for (int32 Turn = 1; Turn <= 6 && TM->GetPhase() != ERTMatchPhase::MatchEnded; ++Turn)
	{
		TM->PlanBotsForTest();

		// Cosa ha deciso il bot, prima che la risoluzione lo consumi.
		const bool bPlannedDash = Bot->PlannedDashAbility != INDEX_NONE;
		const FRTCellId Wanted = Bot->PlannedDashCell;
		const FRTCellId Before = Bot->Cell;

		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

		if (bPlannedDash && Wanted != Before)
		{
			++DashesPlanned;
			// Nessun contendente: se lo scatto era legale, l'unita' e' passata da quella cella. Se il bot avesse
			// proposto una destinazione non in linea, qui resterebbe indietro senza alcuna spiegazione.
			TestTrue(*FString::Printf(
					TEXT("turno %d: lo scatto pianificato su %s si e' potuto eseguire (partiva da %s)"),
					Turn, *Wanted.ToString(), *Before.ToString()),
				Bot->Cell != Before);
		}
	}

	AddInfo(FString::Printf(TEXT("scatti pianificati osservati: %d"), DashesPlanned));
	TestTrue(TEXT("il bot ha pianificato almeno uno scatto"), DashesPlanned > 0);

	// NOTA sul valore di questa verifica: e' uno SMOKE, non la prova dell'invariante. Verificato per mutazione
	// (2026-08-06): rimuovendo il filtro lineare dalle candidate del bot questo test resta VERDE, perche' nello
	// scenario il bot sceglie comunque una cella in linea. L'invariante e' provato da
	// HexSim.LinearFilterDropsGraphOnlyCells, che confronta le due raggiungibilita' su TUTTE le celle.

	DestroyHexMatchWorld(World);
	return true;
}
