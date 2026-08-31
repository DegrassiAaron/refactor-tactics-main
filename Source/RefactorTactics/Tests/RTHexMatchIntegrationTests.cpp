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
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Turn/RTMovementActionLibrary.h"
#include "Map/RTHexVisionLibrary.h"
#include "Pathfinding/RTHexPathLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Tests/RTWorldFixtures.h"

// La guardia: senza, i test di questo file finiscono compilati DENTRO il binario Shipping che si
// distribuisce. Non e' una formalita' di build — e' cio' che tiene il codice di test fuori dal gioco.
// Aggiunta con #923, dopo che lo stesso difetto ha rotto la build Shipping due volte (2026-08-09 e
// 2026-08-15) senza che la suite, che gira sul target Editor, se ne accorgesse.
#if WITH_DEV_AUTOMATION_TESTS

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
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return M;
	}

	ARTUnit* SpawnHexMatchUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = true; // 2v2 bot contro bot: nessuna mano umana, la partita si gioca da sola
		// Senza BeginPlay i cooldown non vengono inizializzati (`AbilityCooldowns` resta vuoto) e OGNI abilita'
		// risulta sempre pronta: una partita di prova che non lo chiama misura un gioco che non esiste.
		U->DispatchBeginPlay();
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		// [D-224] Lo scudo base sta a 0 in questo file: qui si misura una RIDUZIONE di danno, e sommarci
		// una costante di bilanciamento renderebbe l'asserto illeggibile ("15" diventerebbe "15 piu' 5") e
		// legherebbe questi test al valore del base. Chi vuole lo scudo se lo da' esplicitamente.
		U->Shield = 0;
		return U;
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
	ARTUnit* A1 = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(),   FRTCellId(-4, 2));
	ARTUnit* A2 = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(-4, 3));
	ARTUnit* B1 = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(),   FRTCellId(4, -2));
	ARTUnit* B2 = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(4, -3));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !A1 || !A2 || !B1 || !B2) { DestroyHexMatchWorld(World); return false; }

	// Tetto di sicurezza, non una regola di gioco: serve a fallire invece di girare all'infinito.
	// MISURATO (2026-08-06): la partita si decide al **turno 10**, dentro il limite di 12 del catalogo v0.1.
	// Era 25 finche' lo scudo delle abilita' di supporto non scadeva e si accumulava (issue #96).
	const int32 MaxTurns = 40;
	int32 TurnsPlayed = 0;
	while (TM->GetPhase() != ERTMatchPhase::MatchEnded && TurnsPlayed < MaxTurns)
	{
		RTWorldFixtures::PlayOneTurn(TM);
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

	ARTUnit* A = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(-3, 1));
	ARTUnit* B = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(3, -1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !A || !B) { DestroyHexMatchWorld(World); return false; }

	RTWorldFixtures::PlayOneTurn(TM);

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
	ARTUnit* A_Shooter = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(),   FRTCellId(1, 1));
	ARTUnit* A_Mover   = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(1, 2));
	ARTUnit* B_Shooter = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(),   FRTCellId(3, 0));
	ARTUnit* B_Mover   = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(4, 0));
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
	Units.Add(SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(),   Start[0]));
	Units.Add(SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), Start[1]));
	Units.Add(SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(),   Start[2]));
	Units.Add(SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), Start[3]));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || Units.Contains(nullptr)) { DestroyHexMatchWorld(World); return false; }

	TMap<ARTUnit*, FRTCellId> Previous;
	for (ARTUnit* U : Units) { Previous.Add(U, U->Cell); }

	int32 Turns = 0;
	int32 LayerChanges = 0;
	while (TM->GetPhase() != ERTMatchPhase::MatchEnded && Turns < 12)
	{
		RTWorldFixtures::PlayOneTurn(TM);
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
	ARTUnit* Climber = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), Ground);
	ARTUnit* Idle    = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(-4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Climber || !Idle) { DestroyHexMatchWorld(World); return false; }
	Climber->bIsBotControlled = false;
	Idle->bIsBotControlled = false;

	// Premessa: la piattaforma e' su un altro layer e il grafo la collega.
	TestEqual(TEXT("premessa: partenza sul layer 0"), Climber->Cell.Layer, 0);
	TestTrue(TEXT("premessa: il grafo collega terra e piattaforma"),
		URTHexPathLibrary::FindPath(Arena, Ground, Platform).Status == ERTHexPathStatus::Success);

	Climber->PlannedCell = Platform;
	RTWorldFixtures::PlayOneTurn(TM);

	TestTrue(TEXT("l'unita' e' salita sulla piattaforma"), Climber->Cell == Platform);
	TestEqual(TEXT("ora sta sul layer 1"), Climber->Cell.Layer, 1);

	// Togliamo l'arco e riproviamo dalla stessa posizione: senza transizione non si sale.
	Arena->RemoveTransition(Ground, Platform);
	Climber->PlaceOnCell(Ground, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
	Climber->PlannedWaypoints.Reset();
	Climber->PlannedPath.Reset();
	Climber->PlannedCell = Platform;
	RTWorldFixtures::PlayOneTurn(TM);

	TestEqual(TEXT("senza la transizione l'unita' resta sul layer 0"), Climber->Cell.Layer, 0);
	TestTrue(TEXT("e non e' finita sulla piattaforma"), Climber->Cell != Platform);

	DestroyHexMatchWorld(World);
	return true;
}


/**
 * Un EROE del catalogo scatta in partita: `ARTUnit::ConfigureFromHeroData` e' l'unico percorso di
 * configurazione, ed e' quello che il GameMode usa per schierare i quattro eroi.
 *
 * Il gate «questa e' un'azione di scatto» legge la fase del catalogo (#142), quindi le azioni d'eroe — che il
 * flag legacy non l'hanno mai avuto — sono ora pianificabili. Questo test verifica che, una volta pianificate,
 * risolvano sul ramo LINEARE: senza `MovementStyle` dichiarato finirebbero sul pathfinding e aggirerebbero
 * l'ostacolo. E' esattamente il difetto che #142 chiude, sul catalogo che il gioco schiera.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexHeroDashIsLinearTest,
	"RefactorTactics.HexMatch.HeroDashResolvesLinearly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexHeroDashIsLinearTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMatchWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	URTHexMapAsset* Map = SpawnHexMatchMap(World, /*Radius=*/ 5);

	// Muro a due celle sulla traiettoria: con il pathfinding lo si aggirerebbe, in linea ci si ferma davanti.
	FRTHexCellData Wall(FRTCellId(2, 0, 0));
	Wall.bBlocksMovement = true;
	Map->AddOrUpdateCell(Wall);
	Map->SortCells();

	int32 HeroesWithDash = 0;
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		if (!Hero) { continue; }

		ARTUnit* Unit = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!TestNotNull(TEXT("unita' d'eroe"), Unit)) { DestroyHexMatchWorld(World); return false; }
		Unit->TeamId = 0;
		Unit->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(Unit, FTransform::Identity);
		Unit->bIsBotControlled = false;
		Unit->DispatchBeginPlay();
		Unit->PlaceOnCell(FRTCellId(0, 0), FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);

		const int32 DashIdx = Unit->FindDashAbilityIndex();
		if (DashIdx == INDEX_NONE)
		{
			Unit->Destroy();
			continue; // eroe senza mobilita' rapida: non c'e' nulla da verificare
		}
		++HeroesWithDash;

		const URTActionData* Dash = Unit->GetAbility(DashIdx);
		const FString Who = Hero->HeroId.ToString();

		// Ogni mobilita' rapida d'eroe e' LINEARE nella v0.1: e' il dato che instrada la fase Dash.
		if (!TestTrue(FString::Printf(TEXT("%s: lo scatto dichiara uno stile lineare"), *Who),
			Dash && URTMovementActionLibrary::IsLinear(Dash->Def.MovementStyle)))
		{
			Unit->Destroy();
			continue;
		}

		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM) { DestroyHexMatchWorld(World); return false; }

		Unit->PlannedDashAbility = DashIdx;
		Unit->PlannedDashCell = FRTCellId(3, 0); // oltre il muro, in linea
		RTWorldFixtures::PlayOneTurn(TM);

		// Il muro ferma la traiettoria: si arriva a (1,0), non a (3,0) girandoci attorno.
		TestTrue(FString::Printf(TEXT("%s: lo scatto non supera il muro"), *Who),
			Unit->Cell != FRTCellId(3, 0));
		TestEqual(FString::Printf(TEXT("%s: si ferma davanti al muro"), *Who),
			Unit->Cell.ToString(), FRTCellId(1, 0, 0).ToString());

		TM->Destroy();
		Unit->Destroy();
	}

	// Senza questa riga il test passerebbe anche se nessun eroe avesse una mobilita' rapida — cioe' proprio
	// nel caso in cui il gate di #142 fosse tornato a non riconoscerle.
	TestTrue(TEXT("almeno un eroe del roster ha una mobilita' rapida da esercitare"), HeroesWithDash > 0);

	DestroyHexMatchWorld(World);
	return true;
}

/**
 * `Riktor.Ram` e' una CARICA, non uno scatto qualsiasi: percorre una linea retta, si ferma addosso al primo
 * nemico che incontra e lo colpisce nel Blast (20 danni piu' una spinta di 1, gli stessi di `Action.Charge`).
 *
 * Fino a #142 la carica non dichiarava lo stile di movimento, quindi la fase Dash la instradava sul
 * pathfinding normale: chi caricava AGGIRAVA il nemico e arrivava sulla cella chiesta senza colpirlo. Questo
 * test guarda l'esito di un turno vero — posizione finale, danno, spinta — non il contenuto del catalogo.
 *
 * Girava su `Guardian.Charge`, sparita con gli archetipi legacy il 2026-08-10. La carica non e' sparita con
 * lei: e' nel kit di Riktor, ed e' la stessa azione con un nome d'eroe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexChargeImpactTest,
	"RefactorTactics.HexMatch.ChargeStopsOnEnemyAndHits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexChargeImpactTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMatchWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMatchMap(World, /*Radius=*/ 5);

	// Chi carica e il bersaglio allineati sull'asse q: fra loro due celle libere, poi il nemico.
	ARTUnit* Charger = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0));
	ARTUnit* Target  = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(),   FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Charger || !Target) { DestroyHexMatchWorld(World); return false; }
	Charger->bIsBotControlled = false; // i piani li scrive il test, non l'utility del bot
	Target->bIsBotControlled = false;

	// La carica si cerca per ActionId, non per indice: l'indice e' una convenzione del kit e cambierebbe
	// in silenzio il soggetto del test se il catalogo eroi riordinasse le azioni.
	int32 ChargeIdx = INDEX_NONE;
	for (int32 i = 0; i < Charger->NumAbilities(); ++i)
	{
		const URTActionData* A = Charger->GetAbility(i);
		if (A && A->Def.ActionId == FName(TEXT("Hero.Riktor.Ram"))) { ChargeIdx = i; break; }
	}
	const URTActionData* Charge = Charger->GetAbility(ChargeIdx);
	if (!TestNotNull(TEXT("premessa: Riktor ha la carica nel kit"), (void*)Charge))
	{
		DestroyHexMatchWorld(World); return false;
	}
	TestTrue(TEXT("premessa: dichiara lo stile carica"), Charge->Def.MovementStyle == ERTMovementStyle::LinearCharge);

	const int32 HealthBefore = Target->Health;
	Charger->PlannedDashAbility = ChargeIdx;
	Charger->PlannedDashCell = Target->Cell; // si carica CONTRO il nemico
	RTWorldFixtures::PlayOneTurn(TM);

	// Si ferma ADDOSSO: adiacente al bersaglio, non sopra e non oltre.
	TestEqual(TEXT("chi carica si ferma davanti al nemico"), Charger->Cell.ToString(), FRTCellId(2, 0, 0).ToString());

	// E colpisce: gli effetti della carica sono dati del catalogo, applicati nel Blast (codice 20/30).
	TestEqual(TEXT("l'impatto toglie 20 punti vita"), Target->Health, HealthBefore - 20);
	TestEqual(TEXT("e spinge il bersaglio di una cella"), Target->Cell.ToString(), FRTCellId(4, 0, 0).ToString());

	DestroyHexMatchWorld(World);
	return true;
}

/**
 * Lo scatto non entra in una cella che blocca il movimento: si FERMA nell'ultima cella libera della
 * traiettoria (`Fallback.Stop`). Copre headless la parte verificabile di PIE-V01-DASHCOVER.
 *
 * Aggiornato con #142: da quando `Ranger.Dash` dichiara `LinearDash`, lo scatto non passa piu' dall'A' sul
 * grafo, quindi il muro non si aggira. Il test asseriva «l'unita' resta dov'era» e partiva da (2,3), che sulla
 * mappa di prova (esagono di raggio 4) **non esiste**: l'unita' restava ferma perche' il pathfinding falliva
 * sempre, non perche' la cella fosse bloccata. Ora la partenza e' una cella vera e la traiettoria e' reale.
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

	// (2,1) e' uno degli ostacoli della mappa di prova. Da (0,3) ci si arriva in LINEA (direzione q+1/r-1) in
	// due celle: la prima, (1,2), e' libera — e' li' che lo scatto deve fermarsi.
	const FRTCellId Blocked(2, 1, 0);
	const FRTCellId From(0, 3, 0);
	const FRTCellId LastFree(1, 2, 0);
	const FRTHexCellData* BlockedData = Arena->FindCell(Blocked);
	TestTrue(TEXT("premessa: la cella di prova blocca il movimento"),
		BlockedData != nullptr && BlockedData->bBlocksMovement);

	// Premessa che mancava: senza celle vere il test misurerebbe un pathfinding fallito, non un muro.
	TestTrue(TEXT("premessa: la cella di partenza esiste"), Arena->ContainsCell(From));
	const FRTHexCellData* FreeData = Arena->FindCell(LastFree);
	TestTrue(TEXT("premessa: la cella intermedia esiste ed e' libera"),
		FreeData != nullptr && !FreeData->bBlocksMovement);

	ARTUnit* Dasher = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), From);
	ARTUnit* Idle   = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(-4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Dasher || !Idle) { DestroyHexMatchWorld(World); return false; }
	Dasher->bIsBotControlled = false;
	Idle->bIsBotControlled = false;

	// La mobilita' rapida si CERCA, non si assume all'indice 3: quello era lo Scatto del Ranger legacy, e
	// dopo la migrazione al roster l'indice 3 di Wraith e' `Deflection`, una reazione — le premesse qui sotto
	// fallivano su un'abilita' che non c'entra. `FindDashAbilityIndex` interroga la fase del catalogo, che e'
	// il modo in cui il gioco stesso riconosce uno scatto (#142).
	const int32 DashIdx = Dasher->FindDashAbilityIndex();
	const URTActionData* Dash = Dasher->GetAbility(DashIdx);
	if (!TestNotNull(TEXT("premessa: chi scatta ha una mobilita' rapida nel kit"), (void*)Dash)) { DestroyHexMatchWorld(World); return false; }
	TestTrue(TEXT("premessa: e' un'abilita' di mobilita' rapida"),
		URTCatalogLibrary::IsFastMovement(Dash->Def));
	TestTrue(TEXT("premessa: ed e' LINEARE (altrimenti il muro si aggirerebbe)"),
		URTMovementActionLibrary::IsLinear(Dash->Def.MovementStyle));
	TestTrue(TEXT("premessa: la cella bloccata e' entro la portata dello scatto"),
		URTHexLibrary::HexDistance(From, Blocked) <= Dash->Def.RangeCells);

	Dasher->PlannedDashAbility = DashIdx;
	Dasher->PlannedDashCell = Blocked;
	RTWorldFixtures::PlayOneTurn(TM);

	TestTrue(TEXT("lo scatto non entra nella cella bloccata"), Dasher->Cell != Blocked);
	// `Fallback.Stop`: ci si ferma nell'ultima cella valida della traiettoria, non si annulla e non si aggira.
	TestEqual(TEXT("si ferma davanti al muro"), Dasher->Cell.ToString(), LastFree.ToString());

	DestroyHexMatchWorld(World);
	return true;
}


/**
 * `#1525` — l'evento di playback porta il VERDETTO, non solo la rotta.
 *
 * 🔴 **Copre la meta' del difetto che i test puri non vedono.** `ObservedPrefixLength` ha cinque test, e
 * resterebbero tutti verdi anche se nessuno gli passasse i verdetti: il difetto non era la regola — era
 * che `FRTResolvedEvent` portava le celle **senza** il verdetto che `FreezeRouteVerdicts` aveva gia'
 * congelato due righe sopra, sulla stessa `Route`. La traccia lo aveva, il modello no, e da quello scarto
 * nasceva la contraddizione che [D-223] nomina.
 *
 * ⚠️ **Verifica di mutazione**: togliendo `Ev.CellVerdicts = Tracked.CellVerdicts;` dal sito del Move
 * questo test diventa rosso (il conteggio scende a zero) e i cinque test puri restano **verdi**. E' la
 * ragione per cui esiste separatamente da loro.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackEventCarriesVerdictsTest,
	"RefactorTactics.HexMatch.PlaybackEventCarriesKnowledgeVerdicts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackEventCarriesVerdictsTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMatchWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMatchMap(World, /*Radius=*/ 5);

	ARTUnit* A = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(-3, 1));
	ARTUnit* B = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(3, -1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !A || !B) { DestroyHexMatchWorld(World); return false; }

	// I bot si avvicinano: a distanza sei celle il primo turno produce movimento per entrambi.
	RTWorldFixtures::PlayOneTurn(TM);

	// Almeno una delle due si e' mossa, altrimenti la prova sarebbe vuota e passerebbe per assenza.
	const int32 VerdettiA = TM->ResolvedMoveVerdictCountForTest(A->StableUnitId);
	const int32 VerdettiB = TM->ResolvedMoveVerdictCountForTest(B->StableUnitId);
	const bool bQualcunoSiEMosso = (VerdettiA != INDEX_NONE) || (VerdettiB != INDEX_NONE);
	TestTrue(TEXT("almeno un'unita' ha un evento di movimento: senza, il test sarebbe vacuo"),
		bQualcunoSiEMosso);

	// 🔴 Il cuore: ogni evento di movimento porta ALMENO un verdetto. Zero significa che la rotta viaggia
	// senza la risposta a «chi puo' vederla», ed e' esattamente lo stato da cui `#1525` e' nata.
	if (VerdettiA != INDEX_NONE)
	{
		TestTrue(TEXT("l'evento di movimento di A porta i verdetti per cella, non zero"), VerdettiA > 0);
	}
	if (VerdettiB != INDEX_NONE)
	{
		TestTrue(TEXT("l'evento di movimento di B porta i verdetti per cella, non zero"), VerdettiB > 0);
	}

	DestroyHexMatchWorld(World);
	return true;
}

// Chi aggiunge un test in fondo a questo file lo aggiunge PRIMA di questa riga: e' il difetto di #923,
// invisibile in Editor dove la guardia vale 1. Il controllo che lo dimostra e'
// `Build.bat RefactorTactics Win64 Shipping`, non la suite.
#endif // WITH_DEV_AUTOMATION_TESTS
