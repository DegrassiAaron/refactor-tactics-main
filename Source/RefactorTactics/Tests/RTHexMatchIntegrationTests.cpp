#include "Misc/AutomationTest.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTTurnRules.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h" // [D-524]: la copertura alta che ferma la carica sul bordo
#include "Map/RTHexLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Turn/RTMovementActionLibrary.h"
#include "Map/RTHexVisionLibrary.h"
#include "Pathfinding/RTHexPathLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"
// `#2193` estensione: la regola di autorizzazione ha UN owner, e i test del Ready per partecipante la
// interrogano invece di riscriverla — stesso precedente di `IsIntentVisibleTo` (`#507`).
#include "Combat/RTCombatLibrary.h"
#include "Tests/RTAbilityFixtures.h" // `#3261`: AddCoreAbilityInSlot, per `Action.Push` fuori dal kit dell'eroe
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Tests/RTWorldFixtures.h"
#include "Ability/RTMovementProfileLibrary.h" // ProfileSneak: `Sneak` durante il playback non si dichiara (#3510)
// `#2359`: la sonda del commit, e il controller che vi si aggancia.
#include "Player/RTPlayerController.h"
#include "UI/RTScreenHudWidgets.h" // URTPlanCommitWidget: Annulla durante il countdown passa dal pulsante (#3471)
#include "Tests/RTLockInCommittedProbeForTest.h"
#include "Map/RTOverlayPalette.h" // `#3511`: AllMeanings, l'oracolo del reset
#include "Frontend/RTFrontendNavigator.h" // `#3511`: la pausa e' l'uscita `Modal`
#include "Engine/GameInstance.h"
#include "Core/RTGameplayTags.h"     // `#3750`: Slow e Unbalanced, gli stati pubblici che l'area del fuoco riporta
#include "Perception/RTTeamKnowledge.h" // `#3750`: la conoscenza di squadra si dichiara nel banco
#include "Player/RTFocusArea.h"        // `#3750`: l'oracolo dell'area si calcola con la distanza, non con la libreria
#include "EngineUtils.h"               // TActorIterator: la seconda unita' propria del banco

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
	ARTUnit* A1 = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(),   FRTCellId(-4, 2));
	ARTUnit* A2 = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(-4, 3));
	ARTUnit* B1 = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),   FRTCellId(4, -2));
	ARTUnit* B2 = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(4, -3));
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

	ARTUnit* A = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-3, 1));
	ARTUnit* B = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(3, -1));
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
	ARTUnit* A_Shooter = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(),   FRTCellId(1, 1));
	ARTUnit* A_Mover   = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(1, 2));
	ARTUnit* B_Shooter = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),   FRTCellId(3, 0));
	ARTUnit* B_Mover   = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(4, 0));
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
	Units.Add(SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(),   Start[0]));
	Units.Add(SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), Start[1]));
	Units.Add(SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),   Start[2]));
	Units.Add(SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), Start[3]));
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
	ARTUnit* Climber = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), Ground);
	ARTUnit* Idle    = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(-4, 0));
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
 * `Branth.Ram` e' una CARICA, non uno scatto qualsiasi: percorre una linea retta, si ferma addosso al primo
 * nemico che incontra e lo colpisce nel Blast (20 danni piu' una spinta di 1, gli stessi di `Action.Charge`).
 *
 * Fino a #142 la carica non dichiarava lo stile di movimento, quindi la fase Dash la instradava sul
 * pathfinding normale: chi caricava AGGIRAVA il nemico e arrivava sulla cella chiesta senza colpirlo. Questo
 * test guarda l'esito di un turno vero — posizione finale, danno, spinta — non il contenuto del catalogo.
 *
 * Girava su `Guardian.Charge`, sparita con gli archetipi legacy il 2026-08-10. La carica non e' sparita con
 * lei: e' nel kit di Branth, ed e' la stessa azione con un nome d'eroe.
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
	ARTUnit* Charger = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
	ARTUnit* Target  = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),   FRTCellId(3, 0));
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
		if (A && A->Def.ActionId == FName(TEXT("Hero.Branth.Ram"))) { ChargeIdx = i; break; }
	}
	const URTActionData* Charge = Charger->GetAbility(ChargeIdx);
	if (!TestNotNull(TEXT("premessa: Branth ha la carica nel kit"), (void*)Charge))
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
 * [D-523] / [D-524] (`#3793`) — **una carica su un nemico fuori dalle sei direzioni si risolve**: e' la scena di
 * `PIE-V01-CHARGECLICK` (4), Branth in `(-4,3)` e Muiren in `(-2,4)`, scarto `(+2,+1)`. La retta passa per `(-3,3)` e
 * `(-3,4)`; chi carica si ferma in `(-3,4)`, davanti al bersaglio, e lo colpisce.
 *
 * L'impatto parte dalla cella d'arrivo ed e' sempre una delle sei direzioni: da `(-3,4)` a `(-2,4)` e' `E`, quindi la
 * spinta porta il bersaglio in `(-1,4)`. Il facing di chi ha caricato e' invece l'ULTIMO PASSO PERCORSO, `(-3,3)` ->
 * `(-3,4)`, cioe' `SE`: sono due direzioni diverse, e il test le tiene distinte apposta.
 *
 * Il piano lo scrive il test, come in `HexMatch.ChargeStopsOnEnemyAndHits`: la pianificazione e' di
 * `PlayerInput.ChargeOnAnEnemyOffTheAxesFollowsTheFreeLine`. Il bersaglio e' Ivrin e non Muiren perche' qui si misura
 * la geometria della carica, non il kit di chi la subisce.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexChargeOffAxesTest,
	"RefactorTactics.HexMatch.ChargeOnAnEnemyOffTheAxesRunsAndHits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexChargeOffAxesTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMatchWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMatchMap(World, /*Radius=*/ 6);

	ARTUnit* Charger = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(-4, 3));
	ARTUnit* Target  = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(-2, 4));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Charger || !Target) { DestroyHexMatchWorld(World); return false; }
	Charger->bIsBotControlled = false;
	Target->bIsBotControlled = false;

	int32 ChargeIdx = INDEX_NONE;
	for (int32 i = 0; i < Charger->NumAbilities(); ++i)
	{
		const URTActionData* A = Charger->GetAbility(i);
		if (A && A->Def.ActionId == FName(TEXT("Hero.Branth.Ram"))) { ChargeIdx = i; break; }
	}
	int32 Distanza = 0;
	if (!TestTrue(TEXT("premessa: Branth ha la carica, il bersaglio e' a distanza 3 e fuori dalle sei direzioni"),
		ChargeIdx != INDEX_NONE && Charger->EffectiveDashRangeOf(ChargeIdx) >= 3
		&& URTHexLibrary::HexDistance(Charger->Cell, Target->Cell) == 3
		&& !URTMovementActionLibrary::IsStraightLine(Charger->Cell, Target->Cell, Distanza)))
	{
		DestroyHexMatchWorld(World); return false;
	}

	const int32 HealthBefore = Target->Health;
	Charger->Facing = ERTHexDirection::NW; // un verso che la carica non produce: distingue «gira» da «resta com'e'»
	Charger->PlannedDashAbility = ChargeIdx;
	Charger->PlannedDashCell = Target->Cell;
	RTWorldFixtures::PlayOneTurn(TM);

	TestEqual(TEXT("chi carica si ferma davanti al nemico, in (-3,4)"),
		Charger->Cell.ToString(), FRTCellId(-3, 4, 0).ToString());
	TestEqual(TEXT("l'impatto toglie 20 punti vita"), Target->Health, HealthBefore - 20);
	TestEqual(TEXT("e spinge il bersaglio di una cella nella direzione dell'impatto: E, verso (-1,4)"),
		Target->Cell.ToString(), FRTCellId(-1, 4, 0).ToString());
	TestTrue(TEXT("il facing di chi ha caricato e' l'ultimo passo PERCORSO, (-3,3) -> (-3,4): SE"),
		Charger->Facing == ERTHexDirection::SE);

	DestroyHexMatchWorld(World);
	return true;
}

/**
 * [D-524] (5) — **un bordo con copertura alta ferma la carica, e la traccia dice che e' il bordo.**
 *
 * Due mondi per una stessa regola: il bordo a META' della retta (la carica corre un passo e si ferma, senza urtare) e
 * il bordo sul PRIMO passo (non si muove, e la voce `Fallback`/`Cancelled` porta `DashEdgeBlocked`, non
 * `DashPathBlocked`). Chiude l'`IMPLEMENTATION DRIFT` di `RT_TestMatrix_v0.1.md` riga 3: fino a [D-524] gli scatti
 * attraversavano una copertura alta, che camminata, vista e attacchi gia' rispettavano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexChargeEdgeTest,
	"RefactorTactics.HexMatch.ChargeIsStoppedByAnEdgeOnTheLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexChargeEdgeTest::RunTest(const FString&)
{
	for (const bool bSulPrimoPasso : { false, true })
	{
		UWorld* World = MakeHexMatchWorld();
		if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
		URTHexMapAsset* Map = SpawnHexMatchMap(World, /*Radius=*/ 5);

		ARTUnit* Charger = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
		ARTUnit* Target  = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(3, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Charger || !Target || !Map) { DestroyHexMatchWorld(World); return false; }
		Charger->bIsBotControlled = false;
		Target->bIsBotControlled = false;

		int32 ChargeIdx = INDEX_NONE;
		for (int32 i = 0; i < Charger->NumAbilities(); ++i)
		{
			const URTActionData* A = Charger->GetAbility(i);
			if (A && A->Def.ActionId == FName(TEXT("Hero.Branth.Ram"))) { ChargeIdx = i; break; }
		}
		if (!TestTrue(TEXT("premessa: Branth ha la carica"), ChargeIdx != INDEX_NONE))
		{
			DestroyHexMatchWorld(World); return false;
		}

		// La copertura alta: sul bordo (0,0)-(1,0) se e' il primo passo, sul bordo (1,0)-(2,0) se e' a meta' retta.
		const FRTCellId DaCella = bSulPrimoPasso ? FRTCellId(0, 0, 0) : FRTCellId(1, 0, 0);
		const FRTCellId VersoCella = bSulPrimoPasso ? FRTCellId(1, 0, 0) : FRTCellId(2, 0, 0);
		ERTHexDirection Lato = ERTHexDirection::E;
		verify(URTHexLibrary::DirectionBetween(DaCella, VersoCella, Lato));
		verify(URTHexCoverLibrary::AddCover(Map, DaCella, Lato, ERTHexCoverType::High,
			FRTHexCover::DefaultIntegrity(ERTHexCoverType::High)));

		const int32 HealthBefore = Target->Health;
		const FRTCellId TargetCell = Target->Cell;
		Charger->PlannedDashAbility = ChargeIdx;
		Charger->PlannedDashCell = TargetCell;
		RTWorldFixtures::PlayOneTurn(TM);

		const TCHAR* Caso = bSulPrimoPasso ? TEXT("bordo sul primo passo") : TEXT("bordo a meta' retta");
		TestEqual(*FString::Printf(TEXT("%s: nessun impatto, il bersaglio e' intatto"), Caso), Target->Health, HealthBefore);
		TestEqual(*FString::Printf(TEXT("%s: e non si sposta"), Caso), Target->Cell.ToString(), TargetCell.ToString());
		TestEqual(*FString::Printf(TEXT("%s: chi carica si ferma davanti al bordo"), Caso),
			Charger->Cell.ToString(), (bSulPrimoPasso ? FRTCellId(0, 0, 0) : FRTCellId(1, 0, 0)).ToString());

		int32 Annullate = 0;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Fallback && E.Phase == ERTMatchPhase::Dash
				&& E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Cancelled)
				&& E.ActionId == FName(TEXT("Hero.Branth.Ram")))
			{
				++Annullate;
				TestEqual(*FString::Printf(TEXT("%s: il motivo e' il BORDO, non una cella"), Caso),
					E.Amount, static_cast<int32>(ERTActionInvalidReason::DashEdgeBlocked));
				TestTrue(*FString::Printf(TEXT("%s: e la riga lo dice in chiaro"), Caso),
					URTTurnLogLibrary::DescribeEntry(E).Contains(TEXT("bordo")));
			}
		}
		// A meta' retta la carica ha percorso un passo: e' un Move normale, nessun annullamento. Sul primo passo no.
		TestEqual(*FString::Printf(TEXT("%s: una voce di annullamento solo se non si e' mossa"), Caso),
			Annullate, bSulPrimoPasso ? 1 : 0);

		DestroyHexMatchWorld(World);
	}
	return true;
}

/**
 * [D-516] (`#3736`, decisione d'autore «Corre fino alla portata») — **una carica pianificata su un nemico in
 * linea ma OLTRE la portata corre per tutta la portata, si ferma, e non colpisce**. La traccia lo dice: un
 * ripiego `Stopped` col motivo `OutOfRange`, e nessun annullamento.
 *
 * ⌫ *Fino a [D-516] la stessa carica era annullata dalla fase Dash con `DashNotAligned`: l'unita' restava ferma
 * e l'azione era spesa.* Il piano lo scrive il test, come in `HexMatch.ChargeStopsOnEnemyAndHits`: la
 * pianificazione e' di `PlayerInput.ChargeBeyondRangeIsPlannedWithAWarning`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexChargeBeyondRangeTest,
	"RefactorTactics.HexMatch.ChargeBeyondRangeRunsToRangeAndStops",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexChargeBeyondRangeTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMatchWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMatchMap(World, /*Radius=*/ 7);

	ARTUnit* Charger = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Charger) { DestroyHexMatchWorld(World); return false; }
	Charger->bIsBotControlled = false;

	int32 ChargeIdx = INDEX_NONE;
	for (int32 i = 0; i < Charger->NumAbilities(); ++i)
	{
		const URTActionData* A = Charger->GetAbility(i);
		if (A && A->Def.ActionId == FName(TEXT("Hero.Branth.Ram"))) { ChargeIdx = i; break; }
	}
	const int32 Portata = Charger->EffectiveDashRangeOf(ChargeIdx);
	if (!TestTrue(TEXT("premessa: Branth ha la carica, con una portata che la mappa contiene due volte"),
		ChargeIdx != INDEX_NONE && Portata >= 2 && Portata + 2 <= 6))
	{
		DestroyHexMatchWorld(World); return false;
	}

	ARTUnit* Target = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(Portata + 2, 0));
	if (!Target) { DestroyHexMatchWorld(World); return false; }
	Target->bIsBotControlled = false;

	const int32 HealthBefore = Target->Health;
	const FRTCellId TargetCell = Target->Cell;
	Charger->PlannedDashAbility = ChargeIdx;
	Charger->PlannedDashCell = TargetCell;
	RTWorldFixtures::PlayOneTurn(TM);

	TestEqual(TEXT("chi carica corre fino all'ultima cella della portata"),
		Charger->Cell.ToString(), FRTCellId(Portata, 0, 0).ToString());
	TestEqual(TEXT("e non colpisce: il bersaglio era oltre"), Target->Health, HealthBefore);
	TestEqual(TEXT("ne' lo sposta"), Target->Cell.ToString(), TargetCell.ToString());

	int32 Fermate = 0;
	int32 Annullate = 0;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category != ERTLogCategory::Fallback || E.Phase != ERTMatchPhase::Dash
			|| E.ActionId != FName(TEXT("Hero.Branth.Ram")))
		{
			continue;
		}
		if (E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Stopped))
		{
			++Fermate;
			TestEqual(TEXT("il motivo e' «fuori portata»"),
				E.Amount, static_cast<int32>(ERTActionInvalidReason::OutOfRange));
			TestEqual(TEXT("la voce porta la cella da cui parte"), E.SrcCell.ToString(), FRTCellId(0, 0, 0).ToString());
			TestEqual(TEXT("e il bersaglio pianificato"), E.TgtCell.ToString(), TargetCell.ToString());
			TestTrue(TEXT("e la riga lo dice in chiaro"),
				URTTurnLogLibrary::DescribeEntry(E).Contains(TEXT("fuori portata")));
		}
		else if (E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Cancelled))
		{
			++Annullate;
		}
	}
	TestEqual(TEXT("una voce di carica fermata"), Fermate, 1);
	TestEqual(TEXT("e nessun annullamento: la carica e' partita"), Annullate, 0);

	DestroyHexMatchWorld(World);
	return true;
}

/**
 * [D-516] (`#3736`, seconda revisione della PR #3767) — **le condizioni della carica oltre la portata le rivalida il
 * server.** Un piano scritto da un altro produttore — lo scenario, un client — verso una cella vuota oltre la portata,
 * o verso un nemico dietro un terreno che non si percorre di corsa, resta annullato com'era prima di [D-516]: l'unita'
 * non si muove, e la traccia lo dice con `DashNotAligned`.
 *
 * Il controllo positivo e' `HexMatch.ChargeBeyondRangeRunsToRangeAndStops`: stessa geometria, piano legale, e la carica
 * corre.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexChargeBeyondRangeRevalidatedTest,
	"RefactorTactics.HexMatch.ChargeBeyondRangeIsRevalidatedByTheResolver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexChargeBeyondRangeRevalidatedTest::RunTest(const FString&)
{
	for (const bool bCellaVuota : { true, false })
	{
		UWorld* World = MakeHexMatchWorld();
		if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
		URTHexMapAsset* Map = SpawnHexMatchMap(World, /*Radius=*/ 7);

		ARTUnit* Charger = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Charger || !Map) { DestroyHexMatchWorld(World); return false; }
		Charger->bIsBotControlled = false;

		int32 ChargeIdx = INDEX_NONE;
		for (int32 i = 0; i < Charger->NumAbilities(); ++i)
		{
			const URTActionData* A = Charger->GetAbility(i);
			if (A && A->Def.ActionId == FName(TEXT("Hero.Branth.Ram"))) { ChargeIdx = i; break; }
		}
		const int32 Portata = Charger->EffectiveDashRangeOf(ChargeIdx);
		if (!TestTrue(TEXT("premessa: Branth ha la carica, e la mappa ne contiene due celle in piu'"),
			ChargeIdx != INDEX_NONE && Portata >= 2 && Portata + 2 <= 6))
		{
			DestroyHexMatchWorld(World); return false;
		}

		const FRTCellId Bersaglio(Portata + 2, 0, 0);
		if (!bCellaVuota)
		{
			// Un nemico c'e', ma la retta fino a lui passa da un terreno che nega la corsa, oltre la portata.
			ARTUnit* Nemico = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), Bersaglio);
			if (!Nemico) { DestroyHexMatchWorld(World); return false; }
			Nemico->bIsBotControlled = false;
			const FRTCellId Accidentata(Portata + 1, 0, 0);
			FRTHexCellData Dato = Map->FindCell(Accidentata) ? *Map->FindCell(Accidentata) : FRTHexCellData(Accidentata);
			Dato.Surface = ERTHexSurface::Rough;
			Map->AddOrUpdateCell(Dato);
			Map->SortCells();
		}

		Charger->PlannedDashAbility = ChargeIdx;
		Charger->PlannedDashCell = Bersaglio;
		RTWorldFixtures::PlayOneTurn(TM);

		const TCHAR* Caso = bCellaVuota ? TEXT("cella vuota") : TEXT("retta bloccata");
		TestEqual(FString::Printf(TEXT("%s: chi carica non si muove"), Caso),
			Charger->Cell.ToString(), FRTCellId(0, 0, 0).ToString());

		int32 Annullate = 0;
		int32 Fermate = 0;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category != ERTLogCategory::Fallback || E.Phase != ERTMatchPhase::Dash) { continue; }
			if (E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Cancelled)
				&& E.Amount == static_cast<int32>(ERTActionInvalidReason::DashNotAligned))
			{
				++Annullate;
			}
			if (E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Stopped)) { ++Fermate; }
		}
		TestEqual(FString::Printf(TEXT("%s: la carica e' annullata, come prima di D-516"), Caso), Annullate, 1);
		TestEqual(FString::Printf(TEXT("%s: e non si scrive una corsa fino alla portata"), Caso), Fermate, 0);

		DestroyHexMatchWorld(World);
	}
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

	ARTUnit* Dasher = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), From);
	ARTUnit* Idle   = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(-4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Dasher || !Idle) { DestroyHexMatchWorld(World); return false; }
	Dasher->bIsBotControlled = false;
	Idle->bIsBotControlled = false;

	// La mobilita' rapida si CERCA, non si assume all'indice 3: quello era lo Scatto del Ranger legacy, e
	// dopo la migrazione al roster l'indice 3 di Ivrin e' `Deflection`, una reazione — le premesse qui sotto
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

	ARTUnit* A = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-3, 1));
	ARTUnit* B = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(3, -1));
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

/**
 * IL TURNO 1 NON SI CHIUDE DA SOLO: senza lock-in e senza timer scaduto, resta aperto.
 *
 * L'esperimento che discrimina per `#1957`. Il referto di playtest misura un turno 1 chiuso dopo `6,06 s`
 * — trenta secondi prima del timer — e ne conclude che *«si e' chiuso da solo»*, perche' nella finestra non
 * compare nessun evento fra `Selezionata`, `Piano:`, `lock-in` e `Timer scaduto`.
 *
 * Qui la partita viene lasciata **sola**: nessun input, nessun lock-in, e il tempo scorre. Se esistesse un
 * secondo chiudi-turno, il turno cadrebbe qui e questo test lo direbbe.
 *
 * 🔑 **La sanita' che rende il test non-vacuo.** «Siamo ancora al turno 1» e' vero anche in un mondo dove il
 * timer non e' mai stato armato — cioe' il modo piu' facile di scrivere un verde che non misura niente.
 * Perche' l'attesa significhi qualcosa si verifica PRIMA che il timer sia armato, e DOPO che il residuo sia
 * davvero **sceso**: un cronometro che scorre e non fa scattare nulla e' un'altra cosa da un cronometro fermo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFirstTurnDoesNotCloseItselfTest,
	"RefactorTactics.HexMatch.FirstTurnDoesNotCloseItself",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFirstTurnDoesNotCloseItselfTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMatchWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMatchMap(World, /*Radius=*/ 5);

	// Team 0 UMANO: e' la configurazione del referto — un giocatore con due unita' proprie contro due bot.
	// Con tutte e quattro a bot la prova sarebbe di un'altra partita.
	ARTUnit* A1 = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-4, 2));
	ARTUnit* A2 = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(-4, 3));
	ARTUnit* B1 = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(4, -2));
	ARTUnit* B2 = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(4, -3));
	if (!A1 || !A2 || !B1 || !B2) { DestroyHexMatchWorld(World); return false; }
	A1->bIsBotControlled = false;
	A2->bIsBotControlled = false;

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("turn manager"), TM)) { DestroyHexMatchWorld(World); return false; }

	// E' `BeginPlay` ad aprire il turno 1: chiama `StartPlanningTimer`, che arma il timer e scrive
	// `Turno 1 - pianificazione`. Senza, non c'e' nessun turno da guardare.
	TM->DispatchBeginPlay();

	TestEqual(TEXT("la partita normale pianifica per 30 s"), TM->GetPlanningSeconds(), 30.f);
	TestEqual(TEXT("si parte dal turno 1"), TM->GetTurnNumber(), 1);
	TestTrue(TEXT("in pianificazione"), TM->GetPhase() == ERTMatchPhase::Planning);

	const float ArmedAt = TM->GetPlanningTimeRemaining();
	if (!TestTrue(TEXT("il timer e' ARMATO: senza, l'attesa qui sotto non proverebbe niente"), ArmedAt > 0.f))
	{
		DestroyHexMatchWorld(World);
		return false;
	}

	// Dieci secondi di partita lasciata sola — quasi il doppio dei 6,06 s del referto, e ancora venti sotto
	// la scadenza. Nessun input, nessun lock-in: solo il tempo che passa.
	constexpr float Step = 0.25f;
	constexpr int32 Steps = 40;
	for (int32 I = 0; I < Steps; ++I)
	{
		// 🔴 **`FTimerManager` ticka UNA VOLTA per frame**, e senza questa riga il tempo non passa affatto:
		// `Tick()` esce subito su `HasBeenTickedThisFrame()`, quindi trentanove chiamate su quaranta non
		// fanno niente — e la quarantesima nemmeno, perche' anche la prima trova il contatore gia' fermo su
		// questo frame. Il timer resta `Pending`, e `GetTimerRemaining` di un timer `Pending` restituisce la
		// **durata** invece del residuo: `30.000` prima dell'attesa e `30.000` dopo, misurato.
		//
		// ⚠️ Senza la riga di sanita' qui sotto la cosa sarebbe passata inosservata: il turno *sarebbe* stato
		// ancora l'uno, e il test verde — su dieci secondi che non sono mai trascorsi.
		++GFrameCounter;
		World->GetTimerManager().Tick(Step);
	}

	// Il cronometro ha DAVVERO camminato: senza questo, un timer fermo darebbe lo stesso verde.
	const float RemainingNow = TM->GetPlanningTimeRemaining();
	TestTrue(TEXT("il tempo e' sceso: il cronometro cammina"), RemainingNow < ArmedAt);
	TestTrue(TEXT("e non e' ancora scaduto"), RemainingNow > 0.f);

	TestEqual(TEXT("dopo 10 s senza toccare niente: ancora il turno 1"), TM->GetTurnNumber(), 1);
	TestTrue(TEXT("e ancora in pianificazione, non in risoluzione"),
		TM->GetPhase() == ERTMatchPhase::Planning);
	TestFalse(TEXT("nessuna risoluzione e' partita"), TM->IsResolving());

	DestroyHexMatchWorld(World);
	return true;
}

// =====================================================================================================
// Ready countdown e Unready (`#2193`)
//
// Il countdown vive FRA il Ready e il commit, e quattro prove lo circondano: che ritardi, che si annulli
// senza costo, che non allunghi il tetto, e che la via headless resti sincrona.
//
// 🔴 **Tutte e quattro devono far camminare il cronometro a mano.** `FTimerManager` ticka una volta per
// frame: senza `++GFrameCounter` prima di ogni `Tick`, il tempo non passa e i test sarebbero verdi su
// un'attesa mai avvenuta — lo stesso difetto che `FirstTurnDoesNotCloseItself` documenta piu' sopra.
// =====================================================================================================

namespace
{
	/** Fa scorrere `Seconds` di tempo di parete, un frame per passo. */
	void AdvanceWallClock(UWorld* World, float Seconds, float Step = 0.1f)
	{
		const int32 Steps = FMath::CeilToInt(Seconds / Step);
		for (int32 I = 0; I < Steps; ++I)
		{
			++GFrameCounter;
			World->GetTimerManager().Tick(Step);
		}
	}

	/**
	 * Porta a termine il playback della risoluzione, come `RTWorldFixtures::PlayOneTurn`.
	 *
	 * 🔴 **Serve perche' il commit e la fine del turno sono DUE momenti.** `LockInAndResolve` fissa l'esito
	 * logico e apre il playback; il numero di turno avanza solo quando il playback finisce, e senza questi
	 * tick non finisce mai. La prima stesura di questi test asseriva il turno subito dopo il commit e
	 * falliva su una risoluzione perfettamente riuscita — il difetto era l'oracolo.
	 *
	 * ⚠️ Il passo di 0,05 s non decide niente di competitivo: fa avanzare la sola presentazione. Le 400
	 * iterazioni sono un tetto perche' un turno che non chiude debba far fallire il test, non appendere la
	 * suite.
	 */
	void DrainPlayback(ARTTurnManager* TM)
	{
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/** Un mondo con TRE unita' (due proprie, una avversaria), il TurnManager avviato e il turno 1 aperto. */
	ARTTurnManager* MakeCountdownMatch(UWorld* World)
	{
		SpawnHexMatchMap(World, /*Radius=*/ 4);
		ARTUnit* A = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-3, 1));
		// 🔑 **Una SECONDA unita' propria** (`#2555`): senza, «il clic durante il playback cambia la
		// selezione» non e' esprimibile — l'unica alternativa sarebbe selezionare l'AVVERSARIA, che
		// `OnSelect` rifiuta (*«e' avversaria: seleziona prima una tua unita'»*) e che un test non deve
		// chiedere passando dall'API diretta: sarebbe un percorso che il gioco non produce.
		ARTUnit* A2 = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(-3, 0));
		ARTUnit* B = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(3, -1));
		if (!A || !A2 || !B) { return nullptr; }
		A->bIsBotControlled = false; // il Ready e' un gesto umano: con tutte a bot la prova sarebbe di un'altra partita
		A2->bIsBotControlled = false;

		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (TM) { TM->DispatchBeginPlay(); } // apre il turno 1 e arma il tetto
		return TM;
	}
}

/**
 * **Il Ready non committa: arma un countdown, e il commit arriva al suo scadere** (`#2193`).
 *
 * 🔑 La sanita' che rende il test non-vacuo e' la prima riga dopo il Ready: senza `IsReadyCountdownActive()`,
 * un `RequestLockIn` che non avesse fatto NIENTE darebbe lo stesso «non ha ancora risolto».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReadyCountdownDelaysCommitTest,
	"RefactorTactics.HexMatch.ReadyCountdownDelaysTheCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReadyCountdownDelaysCommitTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMatchWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTTurnManager* TM = MakeCountdownMatch(World);
	if (!TestNotNull(TEXT("turn manager"), TM)) { DestroyHexMatchWorld(World); return false; }

	TestEqual(TEXT("il countdown di default e' quello della spec §7.2"), TM->GetReadyCountdownSeconds(), 3.f);

	// 🔴 **L'oracolo del commit e' il NUMERO DI TURNO, non la fase**, e la prima stesura di questi test ci si
	// e' rotta sopra: `LockInAndResolve` risolve e RIAPRE la pianificazione per il turno dopo — l'header lo
	// dichiara, *«il ciclo delle fasi esce quando `Phase == Planning`»*. `Phase == Planning` e' quindi vero sia
	// prima del commit sia dopo, cioe' non distingue le due cose che questo test esiste per distinguere.
	const int32 TurnoPrima = TM->GetTurnNumber();

	TM->RequestLockIn();

	TestTrue(TEXT("il Ready ha ARMATO il countdown"), TM->IsReadyCountdownActive());
	TestEqual(TEXT("e NON ha committato: il turno non e' avanzato"), TM->GetTurnNumber(), TurnoPrima);

	// A meta' countdown non e' ancora successo niente: e' la finestra in cui l'Unready deve poter entrare.
	AdvanceWallClock(World, 1.5f);
	TestEqual(TEXT("a meta' countdown il commit non e' avvenuto"), TM->GetTurnNumber(), TurnoPrima);
	TestTrue(TEXT("e il countdown sta ancora scorrendo"), TM->IsReadyCountdownActive());
	TestTrue(TEXT("il cronometro CAMMINA: ne resta meno di quanto ne e' stato armato"),
		TM->GetReadyCountdownRemaining() < 3.f);

	// Oltre la scadenza: il commit e' avvenuto.
	AdvanceWallClock(World, 2.0f);
	TestFalse(TEXT("il countdown si e' esaurito"), TM->IsReadyCountdownActive());
	TestTrue(TEXT("e la RISOLUZIONE e' partita: il commit c'e' stato"), TM->IsResolving());

	// Il turno avanza a fine playback, non al commit: sono due momenti, e l'oracolo deve aspettare il secondo.
	DrainPlayback(TM);
	TestTrue(TEXT("e a playback finito il turno e' avanzato"), TM->GetTurnNumber() > TurnoPrima);

	DestroyHexMatchWorld(World);
	return true;
}

/**
 * **Unready torna alla pianificazione, e non regala tempo** (`#2193`).
 *
 * Le due meta' del criterio: il piano sopravvive, e il **tetto non si riarma**. La seconda e' quella che si
 * dimentica — un Unready che rimettesse `PlanningSeconds` a nuovo renderebbe Ready+Unready il modo di
 * pianificare senza limite.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnreadyKeepsThePlanTest,
	"RefactorTactics.HexMatch.UnreadyReturnsToPlanningWithThePlanIntact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnreadyKeepsThePlanTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMatchWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTTurnManager* TM = MakeCountdownMatch(World);
	if (!TestNotNull(TEXT("turn manager"), TM)) { DestroyHexMatchWorld(World); return false; }

	// Un piano vero da non perdere: senza, «il piano e' intatto» sarebbe vero su un piano vuoto.
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), Actors);
	ARTUnit* Umana = nullptr;
	for (AActor* Actor : Actors)
	{
		ARTUnit* U = Cast<ARTUnit>(Actor);
		if (U && !U->bIsBotControlled) { Umana = U; break; }
	}
	if (!TestNotNull(TEXT("l'unita' umana"), Umana)) { DestroyHexMatchWorld(World); return false; }
	Umana->PlannedWaypoints.Add(FRTCellId(-2, 1));
	const TArray<FRTCellId> PianoPrima = Umana->PlannedWaypoints;
	if (!TestTrue(TEXT("il piano di partenza NON e' vuoto"), PianoPrima.Num() > 0))
	{
		DestroyHexMatchWorld(World);
		return false;
	}

	AdvanceWallClock(World, 5.0f); // il giocatore pensa cinque secondi
	const float TettoPrimaDelReady = TM->GetPlanningTimeRemaining();
	const int32 TurnoPrima = TM->GetTurnNumber();

	TM->RequestLockIn();
	if (!TestTrue(TEXT("il countdown e' armato: senza, l'Unready non annullerebbe niente"),
		TM->IsReadyCountdownActive()))
	{
		DestroyHexMatchWorld(World);
		return false;
	}

	AdvanceWallClock(World, 1.0f);
	TM->CancelLockIn();

	TestFalse(TEXT("il countdown e' annullato"), TM->IsReadyCountdownActive());
	TestTrue(TEXT("si e' in pianificazione"), TM->GetPhase() == ERTMatchPhase::Planning);
	// ⚠️ La riga qui sopra da sola NON basta, ed e' la debolezza che ha fatto passare questo test mentre i
	// suoi tre gemelli cadevano: dopo una risoluzione la fase torna `Planning`. Il turno invariato e' cio' che
	// distingue «non e' successo niente» da «e' passato un turno intero».
	TestEqual(TEXT("e NESSUN turno e' stato consumato"), TM->GetTurnNumber(), TurnoPrima);
	TestFalse(TEXT("nessuna risoluzione e' partita"), TM->IsResolving());
	TestTrue(TEXT("il piano e' identico a prima del Ready"), Umana->PlannedWaypoints == PianoPrima);

	// 🔴 Il tetto e' SCESO durante il countdown, non e' stato riarmato: l'Unready non regala tempo.
	const float TettoDopo = TM->GetPlanningTimeRemaining();
	TestTrue(TEXT("il tetto non e' stato riarmato: ne resta MENO di prima del Ready"),
		TettoDopo < TettoPrimaDelReady);
	TestTrue(TEXT("e sta ancora scorrendo"), TettoDopo > 0.f);

	// E dopo l'Unready si puo' dichiarare Ready di nuovo: la finestra non e' a uso singolo.
	TM->RequestLockIn();
	TestTrue(TEXT("un secondo Ready riarma il countdown"), TM->IsReadyCountdownActive());

	DestroyHexMatchWorld(World);
	return true;
}

/**
 * **Il tetto vince sul countdown** (`#2193`).
 *
 * Se il timer massimo scade mentre il countdown scorre, il turno si chiude al **tetto**. E' la lettura che
 * *«il countdown non sostituisce il timer massimo»* impone, ed e' l'unica che tiene il turno limitato
 * superiormente: l'altra permetterebbe di sforare il tetto premendo Ready un istante prima.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanningCapWinsOverCountdownTest,
	"RefactorTactics.HexMatch.PlanningCapWinsOverTheReadyCountdown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanningCapWinsOverCountdownTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMatchWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTTurnManager* TM = MakeCountdownMatch(World);
	if (!TestNotNull(TEXT("turn manager"), TM)) { DestroyHexMatchWorld(World); return false; }

	// Un tetto piu' CORTO del countdown: e' la configurazione in cui i due orologi si contendono il turno.
	TM->SetPlanningSeconds(1.0f);
	TM->SetReadyCountdownSeconds(3.f);

	const int32 TurnoPrima = TM->GetTurnNumber();

	TM->RequestLockIn();
	TestTrue(TEXT("il countdown e' armato"), TM->IsReadyCountdownActive());

	// Oltre il tetto (1 s) ma DENTRO il countdown (3 s): se il countdown vincesse, qui non sarebbe successo
	// niente.
	AdvanceWallClock(World, 1.5f);

	TestTrue(TEXT("il tetto ha chiuso il turno: il commit e' avvenuto prima dei 3 s del countdown"),
		TM->IsResolving() || TM->GetTurnNumber() > TurnoPrima);
	TestFalse(TEXT("e il countdown e' stato spento dal commit, non lasciato armato"),
		TM->IsReadyCountdownActive());

	DrainPlayback(TM);
	TestTrue(TEXT("a playback finito il turno e' avanzato"), TM->GetTurnNumber() > TurnoPrima);

	DestroyHexMatchWorld(World);
	return true;
}

/**
 * **Con `ReadyCountdownSeconds = 0` il commit e' sincrono** (`#2193`).
 *
 * 🔑 **E' la prova che questa feature non tocca il determinismo.** Lo Scenario Harness e i test headless
 * chiamano `LockInAndResolve()` direttamente — non passano di qui — ma la via a zero e' quella che tiene
 * `RequestLockIn` utilizzabile da un percorso senza tempo di parete. Senza questa prova, «l'harness non vede
 * il countdown» resterebbe una promessa nel commento.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTZeroCountdownCommitsSynchronouslyTest,
	"RefactorTactics.HexMatch.ZeroReadyCountdownCommitsInTheSameFrame",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTZeroCountdownCommitsSynchronouslyTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMatchWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTTurnManager* TM = MakeCountdownMatch(World);
	if (!TestNotNull(TEXT("turn manager"), TM)) { DestroyHexMatchWorld(World); return false; }

	TM->SetReadyCountdownSeconds(0.f);
	const int32 TurnoPrima = TM->GetTurnNumber();

	TM->RequestLockIn();

	TestFalse(TEXT("nessun countdown e' stato armato"), TM->IsReadyCountdownActive());
	TestTrue(TEXT("il commit e' avvenuto NELLO STESSO frame, senza far scorrere un solo secondo"),
		TM->IsResolving());

	DrainPlayback(TM);
	TestTrue(TEXT("e il turno si chiude senza che sia passato tempo di parete"),
		TM->GetTurnNumber() > TurnoPrima);

	DestroyHexMatchWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// #2359 — la riserva di #2193, messa alla prova
//
// #2193 ha introdotto `OnLockInCommitted` dentro `LockInAndResolve` e ne ha dichiarato in PR un effetto
// collaterale: *«prima di questa PR solo `OnLockIn` spegneva l'anteprima, quindi un turno chiuso dal
// timeout la lasciava accesa per tutta la risoluzione. `OnLockInCommitted` scatta su tutti i percorsi e
// lo copre — ⚠️ effetto osservato, non testato in isolamento»*.
//
// Misurato prima di scrivere (`4d6964a4`): **zero** test nominavano `OnLockInCommitted`, e zero
// `HandleLockInCommitted`. La riserva era ancora tutta da verificare.
// ---------------------------------------------------------------------------------------------------------

/**
 * Il commit si annuncia anche quando a chiudere il turno e' il TETTO, non il giocatore.
 *
 * 🔑 **Il turno DEVE chiudersi dal timer, e non da `RequestLockIn()`.** Quella sarebbe la riga di comodo
 * che rende il test cieco proprio al difetto che deve coprire: il broadcast riportato dentro
 * `ARTPlayerController::OnLockIn` — cioe' la regressione storica — lascerebbe verde un test che passa
 * dal Ready, perche' e' l'unico percorso che quel tasto attraversa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLockInCommittedFiresOnTimeoutTest,
	"RefactorTactics.HexMatch.LockInCommittedFiresOnPlanningTimeout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLockInCommittedFiresOnTimeoutTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMatchWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTTurnManager* TM = MakeCountdownMatch(World);
	if (!TestNotNull(TEXT("turn manager"), TM)) { DestroyHexMatchWorld(World); return false; }

	URTLockInCommittedProbeForTest* Probe = NewObject<URTLockInCommittedProbeForTest>(TM);
	TM->OnLockInCommitted.AddDynamic(Probe, &URTLockInCommittedProbeForTest::OnLockInCommitted);

	// Un tetto corto, e NESSUN Ready: e' il percorso che passa da `OnPlanningTimeout`.
	TM->SetPlanningSeconds(1.0f);
	const int32 TurnoPrima = TM->GetTurnNumber();

	// ANTI-VACUITA' — senza questa riga «e' scattato» e «era gia' scattato» sono indistinguibili.
	TestEqual(TEXT("prima del tetto il commit non e' stato annunciato"), Probe->Broadcasts, 0);
	TestFalse(TEXT("e nessun countdown e' armato: non stiamo misurando la via del Ready"),
		TM->IsReadyCountdownActive());

	AdvanceWallClock(World, 1.5f);

	// ANCORA — il turno si e' chiuso DAVVERO. Senza, un avanzamento che non avesse committato niente
	// darebbe lo stesso «zero broadcast», e il test misurerebbe la propria fixture invece del delegate.
	TestTrue(TEXT("il tetto ha chiuso il turno"), TM->IsResolving() || TM->GetTurnNumber() > TurnoPrima);

	// Il cuore: il commit da timeout si annuncia, una volta sola.
	TestEqual(TEXT("il commit da timeout annuncia se stesso"), Probe->Broadcasts, 1);

	DrainPlayback(TM);
	TestTrue(TEXT("a playback finito il turno e' avanzato"), TM->GetTurnNumber() > TurnoPrima);
	TestEqual(TEXT("e la risoluzione non ha annunciato un secondo commit"), Probe->Broadcasts, 1);

	DestroyHexMatchWorld(World);
	return true;
}

// =====================================================================================================
// #2390 — chi ASCOLTA il commit, non chi lo annuncia
//
// `FRTLockInCommittedFiresOnTimeoutTest` qui sopra prova che `OnLockInCommitted` scatta anche quando a
// chiudere il turno e' il tetto, e resta VERDE mentre il difetto di #2390 esiste: attacca la propria sonda
// direttamente al `TurnManager`, quindi scavalca l'unico iscritto che conta a schermo. Le due domande sono
// diverse — *il commit si annuncia?* e *qualcuno lo sta ascoltando?* — e il verde della prima non dice
// niente della seconda.
//
// `ARTPlayerController` iscriveva `HandleLockInCommitted` **dentro il ramo del Ready** di `OnLockIn`.
// Nessun Ready, nessun iscritto: il broadcast parte, non lo raccoglie nessuno, e l'anteprima di
// pianificazione resta accesa per tutta la risoluzione mostrando una minaccia gia' decisa.
//
// 🔑 **Il soggetto qui e' `ARTPlayerController` insieme ad `ARTHexMapActor`, non il delegate.** Lo stato
// osservabile sono i contatori dell'anteprima, perche' il produttore reale (`RefreshPlanningPreview`) vive
// in un namespace anonimo dentro `RTPlayerController.cpp` e non e' chiamabile da un test — limite gia'
// dichiarato in `RTBlastPreviewTests.cpp:6`. Si osserva l'EFFETTO sui setter della mappa.
//
// ⚠️ **Le superfici dell'anteprima sono TRE e vanno guardate tutte**: il ventaglio raggiungibile, l'area
// colpita e la **rotta**. Un test che guardasse solo la prima resterebbe verde con la traccia ciano del
// piano ancora a schermo per tutta la risoluzione — ed e' meta' del sintomo che #2390 descrive.
// =====================================================================================================

namespace
{
	/** Il banco di #2390: partita col tetto armato, controller collegato alla presentazione, piano tracciato. */
	struct FRTLockInPreviewBench
	{
		UWorld* World = nullptr;
		ARTTurnManager* TM = nullptr;
		ARTHexMapActor* HexMap = nullptr;
		ARTUnit* Mine = nullptr;
		ARTPlayerController* PC = nullptr;
	};

	/**
	 * Quante celle l'anteprima sta disegnando, su TUTTE le sue superfici. Zero = spenta davvero.
	 *
	 * 🔑 **Somma per SIGNIFICATO, su ogni voce di `AllMeanings`** (`#3511`), non su un elenco di superfici scritto qui:
	 * fino a `#3511` contava tre superfici su sette — reachable, rotta, area colpita — e restava verde con la portata,
	 * gli alleati colpiti, la mira o il tratto di LOS ancora accesi. Un'area nuova entra nel conto dicendo il proprio
	 * significato a `NumPlanningPreviewCells`, che non ha `default`.
	 */
	int32 PreviewCellsLit(const ARTHexMapActor* HexMap)
	{
		int32 Lit = 0;
		for (const ERTOverlayMeaning Meaning : URTOverlayPalette::AllMeanings())
		{
			Lit += HexMap->NumPlanningPreviewCells(Meaning);
		}
		return Lit;
	}

	/**
	 * Allestisce il banco e ACCENDE l'anteprima con un piano vero. `false` se qualcosa manca.
	 *
	 * 🔴 **`InitializeActorsForPlay` non e' cerimoniale: senza, il broadcast NON raggiunge il
	 * `PlayerController`, e non lo dice.** `AActor::ProcessEvent` scarta ogni evento se
	 * `GetWorld()->AreActorsInitialized()` e' falso (`Actor.cpp:1513`), e un delegate DINAMICO invoca
	 * proprio da li'. Misurato il 2026-09-05: con il PC iscritto per nome, il broadcast recapitato e la
	 * mappa raggiungibile dal suo mondo, l'anteprima restava accesa — mentre lo stesso handler chiamato
	 * direttamente in C++ la spegneva. Una sonda `UObject` legata allo STESSO delegate riceveva, perche'
	 * passa da `UObject::ProcessEvent`, che quella guardia non ce l'ha: e' cio' che rendeva il difetto
	 * indistinguibile da un guasto di produzione, ed e' il motivo per cui il tentativo di #2359 fu ritirato
	 * come «misura la fixture, non il gioco».
	 *
	 * ⚠️ **Non basta `DispatchBeginPlay()` sull'actor.** Il flag e' del MONDO, non dell'actor, e le due
	 * domande si somigliano abbastanza da far cercare dalla parte sbagliata — lo stesso avvertimento sta
	 * gia' in `RTFrontendNavigationTests.cpp:1070`, scritto da chi ci era passato prima.
	 */
	bool MakeLockInPreviewBench(FAutomationTestBase& Test, FRTLockInPreviewBench& B)
	{
		B.World = MakeHexMatchWorld();
		if (!Test.TestNotNull(TEXT("world di prova"), B.World)) { return false; }

		B.World->InitializeActorsForPlay(FURL());

		B.TM = MakeCountdownMatch(B.World);
		if (!Test.TestNotNull(TEXT("turn manager"), B.TM)) { return false; }

		B.HexMap = ARTHexMapActor::FindInWorld(B.World);
		B.Mine = RTWorldFixtures::FirstUnitOfTeam(B.World, /*TeamId=*/ 0);
		B.PC = RTWorldFixtures::MakePlayerOnTeam(B.World, /*TeamId=*/ 0);
		if (!Test.TestNotNull(TEXT("mappa esagonale"), B.HexMap)
			|| !Test.TestNotNull(TEXT("unita' del giocatore"), B.Mine)
			|| !Test.TestNotNull(TEXT("player controller"), B.PC))
		{
			return false;
		}

		// L'anteprima si accende come si accende in partita: selezionando. `bRecordAsPlayerInput=false`
		// perche' a selezionare qui non e' una persona, e la telemetria di ritmo non deve contare la riga.
		B.PC->SelectUnit(B.Mine, /*bRecordAsPlayerInput=*/ false);

		// 🔑 **E con un PIANO, non a mani vuote.** Un'unita' appena spawnata ha `PlannedWaypoints` vuoto,
		// quindi `PreviewPath` resterebbe vuoto PER COSTRUZIONE e la rotta — meta' del sintomo di #2390 —
		// non sarebbe osservabile: il test sarebbe verde con la traccia ancora a schermo. Si prova un
		// vicino per volta perche' quale sia percorribile dipende dall'arena, e il primo che produce una
		// rotta e' quello buono; se nessuno la produce, l'anti-vacuita' del chiamante lo dichiara.
		for (const FRTCellId& Step : URTHexLibrary::Neighbors(B.Mine->Cell))
		{
			B.Mine->PlannedWaypoints.Reset();
			B.Mine->PlannedWaypoints.Add(Step);
			B.PC->RebuildPlannedPathForTest();
			if (B.HexMap->NumPreviewPathCells() > 0)
			{
				break;
			}
		}
		return true;
	}
}

/**
 * **CASO BUG (#2390).** Il tetto chiude il primo turno e l'anteprima si spegne — su tutte le sue
 * superfici — benche' nessun Ready sia mai stato premuto in questa partita.
 *
 * 🔑 **Il turno DEVE chiudersi dal tetto, e il Ready non va premuto nemmeno una volta.** Premerlo — anche
 * in un turno precedente — iscriverebbe il controller e renderebbe il test cieco proprio al difetto:
 * `AddUniqueDynamic` e' permanente, quindi dal primo Ready in poi il comportamento e' corretto per sempre.
 * E' il motivo per cui il difetto si vede **solo** prima del primo Ready.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanningPreviewClearsOnTimeoutWithoutReadyTest,
	"RefactorTactics.HexMatch.PlanningPreviewClearsOnTimeoutWithoutReady",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanningPreviewClearsOnTimeoutWithoutReadyTest::RunTest(const FString&)
{
	FRTLockInPreviewBench B;
	if (!MakeLockInPreviewBench(*this, B))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	// ANTI-VACUITA' — senza queste due righe «zero celle alla fine» e «zero celle da sempre» sono
	// indistinguibili, e il test misurerebbe la propria fixture invece del difetto. La seconda e' la piu'
	// importante: e' la rotta, cioe' la meta' che un banco senza piano non puo' vedere.
	TestTrue(TEXT("l'anteprima e' ACCESA prima del tetto"), PreviewCellsLit(B.HexMap) > 0);
	TestTrue(TEXT("e la ROTTA e' tracciata: il piano esiste davvero"), B.HexMap->NumPreviewPathCells() > 0);

	// ANCORA — la via del Ready non e' stata percorsa. E' la condizione stessa di #2390.
	TestFalse(TEXT("e nessun Ready e' stato premuto: non stiamo misurando la via del countdown"),
		B.TM->IsReadyCountdownActive());

	// 🔑 **LA TESI DI #2390, misurata direttamente e col SOGGETTO NOMINATO.** `IsBound()` da solo sarebbe
	// vero per qualunque iscritto — un HUD, il GameMode — e resterebbe verde con il controller staccato.
	TestTrue(TEXT("il controller ascolta il commit gia' prima del primo Ready"),
		B.TM->OnLockInCommitted.Contains(B.PC, FName(TEXT("HandleLockInCommitted"))));

	B.TM->SetPlanningSeconds(1.0f);
	const int32 TurnoPrima = B.TM->GetTurnNumber();

	AdvanceWallClock(B.World, 1.5f);

	// ANCORA — il turno si e' chiuso DAVVERO. Senza, un avanzamento che non avesse committato niente
	// darebbe lo stesso «anteprima spenta» solo perche' non e' successo nulla.
	TestTrue(TEXT("il tetto ha chiuso il turno"),
		B.TM->IsResolving() || B.TM->GetTurnNumber() > TurnoPrima);

	// IL CUORE — e' la riga che #2390 dice essere rossa.
	TestEqual(TEXT("il commit da tetto spegne l'anteprima anche senza un Ready precedente"),
		PreviewCellsLit(B.HexMap), 0);

	// ⚠️ **E resta spenta per TUTTA la risoluzione**, che e' cio' che la issue afferma: spegnerla
	// all'istante del commit e riaccenderla un frame dopo lascerebbe verde l'asserzione qui sopra.

	// 🔴 **DENTRO la risoluzione, non solo al suo istante iniziale** (`#2555`, code review). Il commento
	// che stava qui affermava che la garanzia «non riaccendersi durante la risoluzione» restasse asserita
	// piu' sopra: era falso. L'asserzione superstite campionava l'ISTANTE del commit, e un'implementazione
	// che spegnesse li' e riaccendesse un tick dopo sarebbe passata. Questi tick stanno strettamente
	// dentro il playback.
	for (int32 Passo = 0; Passo < 5 && B.TM->IsResolving(); ++Passo)
	{
		B.TM->Tick(0.05f);
		TestEqual(TEXT("e resta spenta MENTRE la risoluzione scorre"), PreviewCellsLit(B.HexMap), 0);
	}

	DrainPlayback(B.TM);
	TestTrue(TEXT("a playback finito il turno e' avanzato"), B.TM->GetTurnNumber() > TurnoPrima);
	// ⚠️ **E a risoluzione finita l'anteprima TORNA, e non e' un allentamento** (`#2555`): la garanzia che
	// questa riga proteggeva — non riaccendersi durante la risoluzione — e' asserita dal ciclo di tick qui
	// sopra, che sta strettamente DENTRO il playback. Cio' che cambia e' il dopo:
	// `OnResolvePlaybackFinished` ricalcola per la selezione corrente, ed e' il contrappeso che rende
	// `Inspect` concedibile. Pretendere ancora zero qui significherebbe pretendere che il giocatore entri
	// nel turno nuovo senza ventaglio.
	TestTrue(TEXT("e a risoluzione finita l'anteprima torna per la selezione corrente"),
		PreviewCellsLit(B.HexMap) > 0);

	DestroyHexMatchWorld(B.World);
	return true;
}

/**
 * **CONTROLLO POSITIVO.** La stessa anteprima, spenta dal percorso che gia' funziona: Ready -> countdown
 * -> commit.
 *
 * Non duplica il test qui sopra: serve a distinguere *«il fix ha acceso l'iscrizione»* da *«il banco
 * spegne l'anteprima comunque»*. Se questo diventasse rosso dopo un fix di #2390, quel fix avrebbe rotto
 * la via del Ready invece di aggiungerne una seconda.
 *
 * ⚠️ Il tetto e' messo a 60 s **apposta**: senza, il turno potrebbe chiudersi da solo prima del countdown
 * e questo test misurerebbe di nuovo il timeout, cioe' l'altro caso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanningPreviewClearsOnReadyCommitTest,
	"RefactorTactics.HexMatch.PlanningPreviewClearsOnReadyCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanningPreviewClearsOnReadyCommitTest::RunTest(const FString&)
{
	FRTLockInPreviewBench B;
	if (!MakeLockInPreviewBench(*this, B))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	TestTrue(TEXT("l'anteprima e' ACCESA prima del Ready"), PreviewCellsLit(B.HexMap) > 0);
	TestTrue(TEXT("e la ROTTA e' tracciata: il piano esiste davvero"), B.HexMap->NumPreviewPathCells() > 0);

	// La sonda misura che il commit sia stato ANNUNCIATO: senza, «l'anteprima e' spenta» non distingue
	// «il commit l'ha spenta» da «il commit non e' mai avvenuto».
	URTLockInCommittedProbeForTest* Probe = NewObject<URTLockInCommittedProbeForTest>(B.TM);
	B.TM->OnLockInCommitted.AddDynamic(Probe, &URTLockInCommittedProbeForTest::OnLockInCommitted);

	// Il tetto lontano isola la via del Ready: a chiudere il turno sara' il countdown, non il timeout.
	B.TM->SetPlanningSeconds(60.0f);
	B.TM->SetReadyCountdownSeconds(0.5f);
	const int32 TurnoPrima = B.TM->GetTurnNumber();

	B.PC->OnLockInForTest();

	// ANTI-VACUITA' — senza, un `OnLockInForTest` che non avesse fatto NIENTE darebbe lo stesso
	// «non ha ancora risolto», ed e' la stessa sanita' di `FRTReadyCountdownDelaysCommitTest`.
	TestTrue(TEXT("il Ready ha armato il countdown"), B.TM->IsReadyCountdownActive());

	AdvanceWallClock(B.World, 1.0f);

	TestTrue(TEXT("il countdown ha chiuso il turno"),
		B.TM->IsResolving() || B.TM->GetTurnNumber() > TurnoPrima);
	TestEqual(TEXT("e il commit ha raggiunto i suoi iscritti"), Probe->Broadcasts, 1);
	TestEqual(TEXT("il commit dal Ready spegne l'anteprima"), PreviewCellsLit(B.HexMap), 0);

	// 🔴 **DENTRO la risoluzione, non solo al suo istante iniziale** (`#2555`, code review). Il commento
	// che stava qui affermava che la garanzia «non riaccendersi durante la risoluzione» restasse asserita
	// piu' sopra: era falso. L'asserzione superstite campionava l'ISTANTE del commit, e un'implementazione
	// che spegnesse li' e riaccendesse un tick dopo sarebbe passata. Questi tick stanno strettamente
	// dentro il playback.
	for (int32 Passo = 0; Passo < 5 && B.TM->IsResolving(); ++Passo)
	{
		B.TM->Tick(0.05f);
		TestEqual(TEXT("e resta spenta MENTRE la risoluzione scorre"), PreviewCellsLit(B.HexMap), 0);
	}

	DrainPlayback(B.TM);
	// ⚠️ **E a risoluzione finita l'anteprima TORNA, e non e' un allentamento** (`#2555`): la garanzia che
	// questa riga proteggeva — non riaccendersi durante la risoluzione — e' asserita dal ciclo di tick qui
	// sopra, che sta strettamente DENTRO il playback. Cio' che cambia e' il dopo:
	// `OnResolvePlaybackFinished` ricalcola per la selezione corrente, ed e' il contrappeso che rende
	// `Inspect` concedibile. Pretendere ancora zero qui significherebbe pretendere che il giocatore entri
	// nel turno nuovo senza ventaglio.
	TestTrue(TEXT("e a risoluzione finita l'anteprima torna per la selezione corrente"),
		PreviewCellsLit(B.HexMap) > 0);

	DestroyHexMatchWorld(B.World);
	return true;
}


/**
 * **#2518 — durante il playback il mondo e' in SOLA LETTURA, e nessuno lo faceva rispettare.**
 *
 * Il nome non e' scelto qui: lo aveva gia' scelto il documento owner.
 * `spec-pointer-interaction.md:545` elencava `PlaybackRejectsPlanningInput` fra i test mancanti e lo
 * classificava *«feature travestita da test — nessuno consuma `ResolutionPlayback` per rifiutare
 * l'input»*. Misurato: nessuno lo **produceva** nemmeno.
 *
 * 🔑 **Il soggetto e' `HandleClickOnCell`, e ci sono voluti due tentativi** (code review, poi misura).
 * La prima stesura chiamava `SelectUnit` su un'unita' **avversaria**: in partita quel gesto non arriva
 * mai li' — `OnSelect` lo ferma prima con *«e' avversaria: seleziona prima una tua unita'»* — quindi
 * guidava un percorso che il gioco non puo' produrre, e per giunta asseriva come corretto mostrare il
 * ventaglio di un avversario. La seconda usava il gesto d'ATTACCO, ma qui le due unita' distano sei
 * celle: nessun attacco e' legale, e il controllo positivo non poteva diventare verde. Il clic sulla
 * **mappa** e' il gesto piu' comune del gioco, e' l'unico che scriva `PlannedWaypoints` — la mutazione
 * che sopravvivrebbe al turno — ed e' quello che la code review indicava come la porta piu' grande.
 *
 * ⚠️ **Il controllo positivo misura un DELTA, non uno stato.** La prima stesura asseriva
 * `PreviewCellsLit > 0` dopo il gesto — ma il banco lascia gia' l'anteprima accesa, quindi sarebbe
 * passato anche se il gesto non avesse fatto nulla. Qui il controllo pretende che il gesto **cambi**
 * qualcosa: il bersaglio d'attacco, che prima non c'era.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackRejectsPlanningInputTest,
	"RefactorTactics.HexMatch.PlaybackRejectsPlanningInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackRejectsPlanningInputTest::RunTest(const FString&)
{
	FRTLockInPreviewBench B;
	if (!MakeLockInPreviewBench(*this, B))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	// ANTI-VACUITA' — il banco ha davvero acceso l'anteprima, rotta compresa.
	TestTrue(TEXT("l'anteprima e' ACCESA in pianificazione"), PreviewCellsLit(B.HexMap) > 0);
	TestTrue(TEXT("e la ROTTA e' tracciata: il piano esiste davvero"), B.HexMap->NumPreviewPathCells() > 0);

	// La cella che il banco ha gia' dimostrato percorribile: e' il gesto piu' comune del gioco, e l'unico
	// che scriva `PlannedWaypoints`. ⚠️ Il gesto d'ATTACCO non serviva allo scopo — in questo allestimento
	// le due unita' distano sei celle, quindi nessun attacco e' legale e il gesto sarebbe un no-op anche
	// in pianificazione: un controllo positivo che non puo' diventare verde non controlla niente.
	if (!TestTrue(TEXT("il banco ha lasciato un waypoint da riusare"), B.Mine->PlannedWaypoints.Num() > 0))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}
	const FRTCellId CellaPercorribile = B.Mine->PlannedWaypoints[0];

	// CONTROLLO POSITIVO, come DELTA — in pianificazione il clic sulla mappa POSA un waypoint.
	B.Mine->PlannedWaypoints.Reset();
	B.PC->RebuildPlannedPathForTest();
	TestEqual(TEXT("controllo: si riparte da un piano vuoto"), B.Mine->PlannedWaypoints.Num(), 0);
	B.PC->HandleClickOnCellForTest(CellaPercorribile);
	TestTrue(TEXT("controllo: in pianificazione il clic sulla mappa posa un waypoint"),
		B.Mine->PlannedWaypoints.Num() > 0);
	TestTrue(TEXT("e riaccende la rotta"), B.HexMap->NumPreviewPathCells() > 0);

	// Il turno si chiude dal tetto e la risoluzione comincia.
	B.TM->SetPlanningSeconds(1.0f);
	AdvanceWallClock(B.World, 1.5f);

	// ANTI-VACUITA' — la fase e' davvero quella, e il commit ha davvero spento (e' #2390, gia' verde).
	TestTrue(TEXT("la risoluzione e' in corso"), B.TM->IsResolving());
	TestTrue(TEXT("e il puntatore lo sa: il mondo risulta in sola lettura"), B.PC->IsWorldReadOnly());
	TestEqual(TEXT("il contesto e' ResolutionPlayback"),
		B.PC->GetPointerContext(), ERTPointerContext::ResolutionPlayback);
	TestEqual(TEXT("il commit ha spento l'anteprima"), PreviewCellsLit(B.HexMap), 0);

	// IL CUORE — lo stesso gesto che poco fa posava un waypoint, adesso non deve posare niente.
	const int32 WaypointPrima = B.Mine->PlannedWaypoints.Num();
	B.PC->HandleClickOnCellForTest(CellaPercorribile);
	TestEqual(TEXT("il clic sulla mappa durante il playback non posa waypoint"),
		B.Mine->PlannedWaypoints.Num(), WaypointPrima);
	TestEqual(TEXT("e non riaccende l'anteprima"), PreviewCellsLit(B.HexMap), 0);

	// ⚠️ **La SELEZIONE invece si sposta, ed e' voluto** (`#2555`): guardare un'altra unita' mentre la
	// risoluzione scorre e' `Inspect`, che §5.3 consente. Questa riga fino al 2026-09-06 pretendeva il
	// contrario, perche' allora mancava il contrappeso che ripristina l'anteprima. Cio' che deve restare
	// fermo e' il PIANO, non lo sguardo — e la copertura dell'altro caso sta in
	// `PlaybackAllowsInspectAndRestoresPreview`.
	ARTUnit* Seconda = nullptr;
	{
		TArray<AActor*> Trovate;
		UGameplayStatics::GetAllActorsOfClass(B.World, ARTUnit::StaticClass(), Trovate);
		for (AActor* Attore : Trovate)
		{
			ARTUnit* U = Cast<ARTUnit>(Attore);
			if (U && U->TeamId == 0 && U != B.Mine) { Seconda = U; break; }
		}
	}
	// ⚠️ **Un `if` silenzioso qui renderebbe il test verde senza aver asserito niente** (code review): il
	// banco garantisce la seconda unita', quindi la sua assenza e' una fixture rotta, non un caso da
	// saltare.
	if (TestNotNull(TEXT("una seconda unita' propria da ispezionare"), Seconda))
	{
		B.PC->SelectUnit(Seconda, /*bRecordAsPlayerInput=*/ false);
		TestEqual(TEXT("l'anteprima resta spenta anche dopo aver ispezionato un'altra unita'"),
			PreviewCellsLit(B.HexMap), 0);
	}

	DrainPlayback(B.TM);

	DestroyHexMatchWorld(B.World);
	return true;
}


/**
 * **#2555 — durante il playback `Inspect` e' consentito, e l'anteprima torna quando la risoluzione finisce.**
 *
 * §5.3 dice *«`Inspect` e camera consentiti»* durante `ResolutionPlayback`, e blocca solo *«ogni input che
 * cambierebbe il piano»*. #2518 ha fermato il clic sul mondo **del tutto**, `Inspect` compreso: era la
 * deviazione piu' piccola disponibile allora, perche' lasciar cambiare la sola selezione lasciava l'unita'
 * scelta col ventaglio vuoto e nessuno la ripristinava.
 *
 * 🔑 **Il contrappeso mancante esisteva gia' e non lo usava nessuno**: `OnResolvePlaybackFinished`
 * (`RTTurnManager.h:884`, trasmesso a `.cpp:8102`) — la stessa forma che aveva `OnLockInCommitted` prima
 * di `#2390`. Uno spegne l'anteprima al commit, l'altro la riaccende a risoluzione finita: con la coppia,
 * `Inspect` puo' tornare consentito senza lasciare uno stato a meta'.
 *
 * ⚠️ **Il piano NON deve tornare quello di prima**: a risoluzione finita le unita' si sono mosse e i
 * waypoint sono stati consumati. Cio' che deve tornare e' *un'anteprima coerente con la selezione
 * corrente*, non la fotografia del turno passato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackAllowsInspectAndRestoresPreviewTest,
	"RefactorTactics.HexMatch.PlaybackAllowsInspectAndRestoresPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackAllowsInspectAndRestoresPreviewTest::RunTest(const FString&)
{
	FRTLockInPreviewBench B;
	if (!MakeLockInPreviewBench(*this, B))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	// La seconda unita' PROPRIA: e' il bersaglio legittimo di un `Inspect`, e l'unico gesto che il gioco
	// possa davvero produrre per la selezione — cliccare l'avversaria in playback la ispeziona (`#3748`), ma non
	// la seleziona: questo test guarda la propria.
	ARTUnit* Seconda = nullptr;
	{
		TArray<AActor*> Trovate;
		UGameplayStatics::GetAllActorsOfClass(B.World, ARTUnit::StaticClass(), Trovate);
		for (AActor* Attore : Trovate)
		{
			ARTUnit* U = Cast<ARTUnit>(Attore);
			if (U && U->TeamId == 0 && U != B.Mine)
			{
				Seconda = U;
				break;
			}
		}
	}
	if (!TestNotNull(TEXT("una seconda unita' propria da ispezionare"), Seconda))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	// ANTI-VACUITA' — il banco ha acceso l'anteprima, e la selezione parte dalla prima unita'.
	TestTrue(TEXT("l'anteprima e' ACCESA in pianificazione"), PreviewCellsLit(B.HexMap) > 0);
	TestEqual(TEXT("e la selezione e' l'unita' del banco"),
		static_cast<const AActor*>(B.PC->GetSelectedUnit()), static_cast<const AActor*>(B.Mine));

	// Il turno si chiude dal tetto e la risoluzione comincia.
	B.TM->SetPlanningSeconds(1.0f);
	AdvanceWallClock(B.World, 1.5f);

	TestTrue(TEXT("la risoluzione e' in corso"), B.TM->IsResolving());
	TestTrue(TEXT("e il mondo risulta in sola lettura"), B.PC->IsWorldReadOnly());
	TestEqual(TEXT("il commit ha spento l'anteprima"), PreviewCellsLit(B.HexMap), 0);

	// IL CUORE — `Inspect` e' CONSENTITO: la selezione si sposta...
	B.PC->SelectUnit(Seconda, /*bRecordAsPlayerInput=*/ false);
	TestEqual(TEXT("durante il playback il clic ISPEZIONA: la selezione si sposta"),
		static_cast<const AActor*>(B.PC->GetSelectedUnit()), static_cast<const AActor*>(Seconda));

	// ...ma NON accende il piano, che il commit ha spento apposta.
	TestEqual(TEXT("e non riaccende l'anteprima"), PreviewCellsLit(B.HexMap), 0);

	// E a risoluzione finita l'anteprima TORNA, coerente con la selezione corrente: e' il contrappeso di
	// `OnLockInCommitted`, ed e' cio' che rende `Inspect` consentibile senza lasciare uno stato a meta'.
	DrainPlayback(B.TM);
	TestFalse(TEXT("finita la risoluzione il mondo non e' piu' in sola lettura"), B.PC->IsWorldReadOnly());
	TestTrue(TEXT("e l'anteprima e' tornata"), PreviewCellsLit(B.HexMap) > 0);

	// 🔑 **PER QUALE unita', non solo «qualcosa e' acceso»** (code review). Contare le celle non distingue
	// «ricalcolata per la selezione corrente» da «ripristinata la fotografia del turno passato»: entrambe
	// darebbero un numero maggiore di zero, e la seconda e' proprio cio' che l'header dichiara di NON fare.
	TestEqual(TEXT("e la selezione e' rimasta quella ispezionata"),
		static_cast<const AActor*>(B.PC->GetSelectedUnit()), static_cast<const AActor*>(Seconda));
	TestTrue(TEXT("e il ventaglio contiene una cella adiacente ALL'UNITA' ISPEZIONATA"),
		B.HexMap->IsPreviewReachableCell(URTHexLibrary::Neighbors(Seconda->Cell)[0])
		|| B.HexMap->IsPreviewReachableCell(URTHexLibrary::Neighbors(Seconda->Cell)[1]));

	DestroyHexMatchWorld(B.World);
	return true;
}

// =====================================================================================================
// `#3510`, [D-468] — durante la risoluzione il mondo e' in sola lettura ANCHE PER LA TASTIERA.
//
// I click sul mondo escono su `IsWorldReadOnly()` da `#2518` (`PlaybackRejectsPlanningInput`, qui sopra). Le
// porte degli ordini da tastiera — armo, `Invio`, `Sneak` — guardavano al piu' il modale e l'autobattle.
//
// 🔑 **Il «dopo» conta quanto il «durante».** La risoluzione consuma il piano e il Cleanup disarma PRIMA del
// playback (`LockInAndResolve`, poi `ConcludeResolution`): cio' che un tasto scrive mentre il playback scorre
// non lo azzera nessuno, ed entra nel turno dopo. Per questo ogni test drena il playback e guarda il turno nuovo.
// =====================================================================================================

namespace
{
	/**
	 * Tutte le superfici di pianificazione. ⏱️ *Fino a `#3511` `PreviewCellsLit` ne contava tre e questa aggiungeva a mano
	 * la portata e gli alleati colpiti*: ora `PreviewCellsLit` le conta tutte per significato, e questa ne e' il nome storico.
	 */
	int32 PlanningSurfacesLit(const ARTHexMapActor* HexMap)
	{
		return PreviewCellsLit(HexMap);
	}

	/** La posizione del kit che porta `ActionId`, o `INDEX_NONE`. */
	int32 KitIndexOf(const ARTUnit* Unit, const FName& ActionId)
	{
		for (int32 I = 0; I < Unit->NumAbilities(); ++I)
		{
			const URTActionData* Ability = Unit->GetAbility(I);
			if (Ability && Ability->Def.ActionId == ActionId)
			{
				return I;
			}
		}
		return INDEX_NONE;
	}

	/** Chiude il turno dal tetto, come i test qui sopra: il Ready non va premuto (vedi `#2390`). */
	void CommitFromTimeout(const FRTLockInPreviewBench& B)
	{
		B.TM->SetPlanningSeconds(1.0f);
		AdvanceWallClock(B.World, 1.5f);
	}

	/** ANTI-VACUITA' — la risoluzione scorre davvero, e il puntatore lo sa. Senza, ogni «invariato» e' gratis. */
	bool AssertInPlayback(FAutomationTestBase& Test, const FRTLockInPreviewBench& B)
	{
		const bool bInCorso = Test.TestTrue(TEXT("la risoluzione e' in corso"), B.TM->IsResolving());
		Test.TestEqual(TEXT("e il contesto e' ResolutionPlayback"),
			B.PC->GetPointerContext(), ERTPointerContext::ResolutionPlayback);
		Test.TestEqual(TEXT("e il commit ha spento ogni superficie di pianificazione"),
			PlanningSurfacesLit(B.HexMap), 0);
		return bInCorso;
	}

	/** La causa che il rifiuto deve nominare ([D-468]): la frase di `PercheIlMondoESoloLettura`. */
	const TCHAR* const CausaPlayback = TEXT("fino alla fine del playback il piano e' in sola lettura");
}

/**
 * **#3510 — un tasto abilita' durante il playback non arma, e non accende nessuna area di pianificazione.**
 *
 * Il terzo criterio della issue, sul banco che il criterio nomina. I canali sono due e la porta una: lo slot
 * del dock (`ArmKitAbility`) e il tasto (`SelectAbilityForCurrent`).
 *
 * ⚠️ **L'azione si sceglie col CONTROLLO POSITIVO, non per nome**: quale posizione del kit abbia una portata
 * dipende dall'eroe del banco, e la prima che in pianificazione la accende e' quella buona — come il banco fa
 * coi vicini per la rotta. Un'azione senza portata renderebbe «zero celle» vero per costruzione.
 *
 * ⚠️ **Il dock va PRIMA del tasto**, e non per estetica: `ArmKitAbility` sulla posizione gia' armata DISARMA. Col
 * tasto per primo, senza la guardia il dock spegnerebbe cio' che il tasto aveva acceso, e la sua riga
 * passerebbe per la ragione sbagliata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackKeyboardArmIsANoOpTest,
	"RefactorTactics.HexMatch.PlaybackKeyboardArmIsANoOp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackKeyboardArmIsANoOpTest::RunTest(const FString&)
{
	FRTLockInPreviewBench B;
	if (!MakeLockInPreviewBench(*this, B))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	// CONTROLLO POSITIVO, come DELTA — in pianificazione il tasto ARMA, e la portata si accende.
	int32 Mirata = INDEX_NONE;
	for (int32 I = 0; I < B.Mine->NumAbilities() && Mirata == INDEX_NONE; ++I)
	{
		B.PC->SelectAbilityForCurrentForTest(I);
		if (B.Mine->SelectedAbilityIndex == I && B.HexMap->GetPreviewRangeCells().Num() > 0)
		{
			Mirata = I;
		}
		B.PC->SelectAbilityForCurrentForTest(INDEX_NONE);
	}
	if (!TestTrue(TEXT("controllo: in pianificazione un tasto arma un'azione e ne accende la portata"),
		Mirata != INDEX_NONE))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	CommitFromTimeout(B);
	if (!AssertInPlayback(*this, B))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}
	TestEqual(TEXT("e il Cleanup ha lasciato l'unita' disarmata"), B.Mine->SelectedAbilityIndex, (int32)INDEX_NONE);

	// IL LOG — la causa vera, una riga per canale. Prima diceva «autobattle, o fase che non accetta ordini», e
	// nessuna delle due era vera.
	AddExpectedMessagePlain(CausaPlayback, ELogVerbosity::Display,
		EAutomationExpectedMessageFlags::Contains, /*Occurrences=*/ 2);

	// IL CUORE — lo slot del dock...
	B.PC->ArmKitAbility(Mirata);
	TestEqual(TEXT("durante il playback lo slot del dock non arma"),
		B.Mine->SelectedAbilityIndex, (int32)INDEX_NONE);
	TestEqual(TEXT("e non accende nessuna area di pianificazione"), PlanningSurfacesLit(B.HexMap), 0);

	// ...e il tasto, che passa dalla stessa porta.
	B.PC->SelectAbilityForCurrentForTest(Mirata);
	TestEqual(TEXT("ne' il tasto abilita'"), B.Mine->SelectedAbilityIndex, (int32)INDEX_NONE);
	TestEqual(TEXT("e la portata resta spenta"), PlanningSurfacesLit(B.HexMap), 0);

	DrainPlayback(B.TM);
	TestFalse(TEXT("il playback e' finito"), B.TM->IsResolving());
	TestEqual(TEXT("e il turno nuovo comincia disarmato"), B.Mine->SelectedAbilityIndex, (int32)INDEX_NONE);

	DestroyHexMatchWorld(B.World);
	return true;
}

/**
 * **#3510 — un armo durante il playback non entra nel turno dopo: non e' un pre-armo** ([D-468] punto 4).
 *
 * E' la misura che la issue chiedeva prima di correggere: *«lo stato armato sopravvive al turno? Il piano
 * scritto vale per il turno dopo?»*. L'azione e' l'`Overwatch` perche' scrive il piano all'armo, senza
 * aspettare un bersaglio (`bSelfTarget`), ed e' una generica: la porta ogni unita'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackArmIsNotAPreArmTest,
	"RefactorTactics.HexMatch.PlaybackArmIsNotAPreArm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackArmIsNotAPreArmTest::RunTest(const FString&)
{
	FRTLockInPreviewBench B;
	if (!MakeLockInPreviewBench(*this, B))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	const int32 Overwatch = KitIndexOf(B.Mine, FName(TEXT("Action.Overwatch")));
	if (!TestTrue(TEXT("l'Overwatch e' nel kit: le generiche le riceve ogni unita'"), Overwatch != INDEX_NONE))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	// CONTROLLO POSITIVO, come DELTA — in pianificazione armarlo SCRIVE il piano, e il disarmo lo toglie.
	B.PC->SelectAbilityForCurrentForTest(Overwatch);
	TestEqual(TEXT("controllo: in pianificazione armare l'Overwatch lo mette nel piano"),
		B.Mine->PlannedAbilityIndex, Overwatch);
	B.PC->SelectAbilityForCurrentForTest(INDEX_NONE);
	TestEqual(TEXT("controllo: e il disarmo lo toglie"), B.Mine->PlannedAbilityIndex, (int32)INDEX_NONE);

	CommitFromTimeout(B);
	if (!AssertInPlayback(*this, B))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	AddExpectedMessagePlain(CausaPlayback, ELogVerbosity::Display,
		EAutomationExpectedMessageFlags::Contains, /*Occurrences=*/ 1);

	// IL CUORE — durante il playback il tasto non arma e non scrive il piano...
	B.PC->SelectAbilityForCurrentForTest(Overwatch);
	TestEqual(TEXT("durante il playback il tasto non arma"), B.Mine->SelectedAbilityIndex, (int32)INDEX_NONE);
	TestEqual(TEXT("e non scrive il piano"), B.Mine->PlannedAbilityIndex, (int32)INDEX_NONE);
	TestEqual(TEXT("e non accende nessuna area di pianificazione"), PlanningSurfacesLit(B.HexMap), 0);

	// ...e il turno dopo non lo eredita. E' la meta' che la sola guardia «durante» non dimostrerebbe.
	DrainPlayback(B.TM);
	TestFalse(TEXT("il playback e' finito"), B.TM->IsResolving());
	TestEqual(TEXT("il turno nuovo comincia disarmato"), B.Mine->SelectedAbilityIndex, (int32)INDEX_NONE);
	TestEqual(TEXT("e senza un Overwatch che nessuno ha pianificato"),
		B.Mine->PlannedAbilityIndex, (int32)INDEX_NONE);

	DestroyHexMatchWorld(B.World);
	return true;
}

/**
 * **#3510 — `Sneak` e `Invio` durante il playback non dichiarano niente, e il turno dopo non li eredita.**
 *
 * Hanno la forma dell'armo: una dichiarazione sul piano dell'unita', scritta da un tasto e da un pulsante della
 * barra (`ToggleSneakDeclaration`, `TogglePlanDeclaration`) che guardavano solo il modale. E nessuna delle due
 * la azzera il playback: la dichiarazione del piano la toglie il Cleanup, che gira prima; `Sneak` resta finche'
 * non lo si ritira ([D-425]).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackRejectsPlanDeclarationsTest,
	"RefactorTactics.HexMatch.PlaybackRejectsPlanDeclarations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackRejectsPlanDeclarationsTest::RunTest(const FString&)
{
	FRTLockInPreviewBench B;
	if (!MakeLockInPreviewBench(*this, B))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	const FName Sneak = URTMovementProfileLibrary::ProfileSneak;

	// CONTROLLI POSITIVI, come DELTA — in pianificazione i due tasti dichiarano, e ripremuti ritirano.
	TestTrue(TEXT("controllo: si parte senza Sneak"), B.Mine->PlannedMovementProfileId.IsNone());
	B.PC->ToggleSneakForTest();
	TestEqual(TEXT("controllo: in pianificazione M dichiara Sneak"), B.Mine->PlannedMovementProfileId, Sneak);
	B.PC->ToggleSneakForTest();
	TestTrue(TEXT("controllo: e ripremuto lo ritira"), B.Mine->PlannedMovementProfileId.IsNone());

	TestTrue(TEXT("controllo: in pianificazione Invio dichiara il piano"), B.PC->ToggleTurnPlanDeclaredForTest());
	TestTrue(TEXT("controllo: e la dichiarazione e' sull'unita'"), B.Mine->bTurnPlanDeclared);
	B.PC->ToggleTurnPlanDeclaredForTest();
	TestFalse(TEXT("controllo: e ripremuto la ritratta"), B.Mine->bTurnPlanDeclared);

	CommitFromTimeout(B);
	if (!AssertInPlayback(*this, B))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	AddExpectedMessagePlain(CausaPlayback, ELogVerbosity::Display,
		EAutomationExpectedMessageFlags::Contains, /*Occurrences=*/ 2);

	// IL CUORE — durante il playback nessuno dei due dichiara...
	B.PC->ToggleSneakForTest();
	TestTrue(TEXT("durante il playback M non dichiara Sneak"), B.Mine->PlannedMovementProfileId.IsNone());
	TestFalse(TEXT("e Invio rifiuta"), B.PC->ToggleTurnPlanDeclaredForTest());
	TestFalse(TEXT("e non dichiara il piano"), B.Mine->bTurnPlanDeclared);

	// ...e il turno dopo non eredita niente.
	DrainPlayback(B.TM);
	TestFalse(TEXT("il playback e' finito"), B.TM->IsResolving());
	TestTrue(TEXT("il turno nuovo comincia senza Sneak dichiarato"), B.Mine->PlannedMovementProfileId.IsNone());
	TestFalse(TEXT("e con l'unita' ancora da concludere"), B.Mine->bTurnPlanDeclared);

	DestroyHexMatchWorld(B.World);
	return true;
}

// =====================================================================================================
// `#2193`, estensione 2026-09-07 — il Ready e' del PARTECIPANTE, e il countdown parte dal QUORUM.
//
// 🔑 **Nessuno di questi test passa da `RequestLockIn()`**, ed e' deliberato: quella resta la porta
// dell'harness e dei test storici, cioe' «il quorum si e' chiuso» detto senza partecipanti. Qui si prova
// lo strato sopra, che e' l'unico che il gioco attraversa da `#2193` in poi.
// =====================================================================================================

namespace
{
	/** Una partita con due unita' umane nello stesso gruppo e una avversaria pianificata dal bot. */
	struct FRTParticipantMatch
	{
		UWorld* World = nullptr;
		ARTTurnManager* TM = nullptr;
		ARTUnit* A = nullptr;   // squadra 0, gruppo 0 — umana
		ARTUnit* A2 = nullptr;  // squadra 0, gruppo 0 — umana: il SECONDO Hero dello stesso Player
		ARTUnit* B = nullptr;   // squadra 1 — bot
	};

	/**
	 * ⚠️ **`B->bIsBotControlled` si scrive a mano, e non e' pignoleria**: `MakeCountdownMatch` lo lascia al
	 * default `false`, che in una partita vera non accade mai — `RTMatchBootstrapper` scrive
	 * `(TeamId == 1) || Config.bAutobattle || bBotAlly`. Lasciarlo falso renderebbe l'avversario un
	 * partecipante eleggibile, e il quorum di questi test non sarebbe quello del gioco.
	 */
	FRTParticipantMatch MakeParticipantMatch()
	{
		FRTParticipantMatch M;
		M.World = MakeHexMatchWorld();
		if (!M.World) { return M; }

		SpawnHexMatchMap(M.World, /*Radius=*/ 4);
		M.A  = SpawnHexMatchUnit(M.World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-3, 1));
		M.A2 = SpawnHexMatchUnit(M.World, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(-3, 0));
		M.B  = SpawnHexMatchUnit(M.World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(3, -1));
		if (!M.A || !M.A2 || !M.B) { return M; }

		// Il formato v0.1: `UnitsPerPlayer == UnitsPerTeam`, quindi un gruppo solo per squadra.
		M.A->ControlGroup = 0;  M.A->bIsBotControlled = false;
		M.A2->ControlGroup = 0; M.A2->bIsBotControlled = false;
		M.B->ControlGroup = 0;  M.B->bIsBotControlled = true;

		M.TM = M.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (M.TM) { M.TM->DispatchBeginPlay(); }
		return M;
	}
}

/**
 * **Il Ready dichiarato da chi comanda vale per TUTTE le unita' che comanda** (`D011`, `D016.2`).
 *
 * 🔑 La sanita' che rende il test non-vacuo e' la coppia di `CanPlayerControlUnitInGroup`: senza, «il
 * partecipante e' Ready» non direbbe nulla su **quante** unita' quel Ready copre — ed e' esattamente la
 * domanda che l'estensione esiste per chiudere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTParticipantReadyCoversEveryCommandedUnitTest,
	"RefactorTactics.HexMatch.ParticipantReadyCoversEveryCommandedUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTParticipantReadyCoversEveryCommandedUnitTest::RunTest(const FString&)
{
	FRTParticipantMatch M = MakeParticipantMatch();
	if (!TestNotNull(TEXT("turn manager"), M.TM)) { DestroyHexMatchWorld(M.World); return false; }

	TestFalse(TEXT("nessuno e' Ready all'apertura"), M.TM->IsParticipantReady(0, 0));
	TestFalse(TEXT("e il quorum non e' chiuso"), M.TM->HasReadyQuorum());

	M.TM->DeclareParticipantReady(0, 0);

	TestTrue(TEXT("il PARTECIPANTE e' Ready"), M.TM->IsParticipantReady(0, 0));
	TestTrue(TEXT("il quorum si e' chiuso"), M.TM->HasReadyQuorum());
	TestTrue(TEXT("e il countdown e' stato armato dal quorum"), M.TM->IsReadyCountdownActive());

	// 🔴 **Le due unita' rispondono allo STESSO partecipante**, ed e' cio' che rende impossibile lo stato
	// «A Ready / A2 non Ready»: non ci sono due stati da tenere allineati, ce n'e' uno solo.
	TestTrue(TEXT("il partecipante (0,0) comanda A"),
		URTCombatLibrary::CanPlayerControlUnitInGroup(M.A->TeamId, M.A->ControlGroup, 0, 0, M.A->bIsBotControlled));
	TestTrue(TEXT("e comanda anche A2"),
		URTCombatLibrary::CanPlayerControlUnitInGroup(M.A2->TeamId, M.A2->ControlGroup, 0, 0, M.A2->bIsBotControlled));

	// ⛔ E non comanda l'avversaria: il Ready non ha attraversato la squadra.
	TestFalse(TEXT("ma non comanda l'unita' della squadra 1"),
		URTCombatLibrary::CanPlayerControlUnitInGroup(M.B->TeamId, M.B->ControlGroup, 0, 0, M.B->bIsBotControlled));

	DestroyHexMatchWorld(M.World);
	return true;
}

/**
 * **L'Unready e' simmetrico e non perde il piano** (`D016.3`).
 *
 * ⚠️ L'asserto sul piano non e' decorativo: e' la meta' del criterio di `#2193` che un test puo' vedere, e
 * senza di esso «l'Unready funziona» sarebbe vero anche per un Unready che azzera la pianificazione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTParticipantUnreadyClearsTheWholeParticipantTest,
	"RefactorTactics.HexMatch.ParticipantUnreadyClearsTheWholeParticipant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTParticipantUnreadyClearsTheWholeParticipantTest::RunTest(const FString&)
{
	FRTParticipantMatch M = MakeParticipantMatch();
	if (!TestNotNull(TEXT("turn manager"), M.TM)) { DestroyHexMatchWorld(M.World); return false; }

	const FRTCellId Pianificata = URTHexLibrary::Neighbors(M.A->Cell)[0];
	M.A->PlannedCell = Pianificata;

	M.TM->DeclareParticipantReady(0, 0);
	if (!TestTrue(TEXT("premessa: il countdown e' armato"), M.TM->IsReadyCountdownActive()))
	{
		DestroyHexMatchWorld(M.World); return false;
	}

	M.TM->WithdrawParticipantReady(0, 0);

	TestFalse(TEXT("il partecipante non e' piu' Ready"), M.TM->IsParticipantReady(0, 0));
	TestFalse(TEXT("il quorum si e' riaperto"), M.TM->HasReadyQuorum());
	TestFalse(TEXT("e il countdown e' stato annullato"), M.TM->IsReadyCountdownActive());
	TestEqual(TEXT("il piano e' ancora quello di prima"), M.A->PlannedCell, Pianificata);

	// Il tetto non e' stato riarmato dall'Unready: e' la regola di `#2193`, e vale anche da questa strada.
	TestTrue(TEXT("il tetto della pianificazione scorre ancora"), M.TM->GetPlanningTimeRemaining() > 0.f);

	DestroyHexMatchWorld(M.World);
	return true;
}

/**
 * **Quorum VUOTO: nessun commit anticipato** (`D016.6`).
 *
 * 🔴 Il difetto che questo test coglie e' la misura diventata vuota — la stessa forma di `#2356`. Un
 * quorum scritto come *«tutti gli eleggibili sono Ready»* e' **vacuamente vero** su un insieme vuoto: in
 * autobattle chiuderebbe ogni turno dopo il countdown invece che al tetto, e nessun test che guardi solo
 * il caso umano se ne accorgerebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEmptyQuorumDoesNotArmAnEarlyCommitTest,
	"RefactorTactics.HexMatch.EmptyQuorumDoesNotArmAnEarlyCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEmptyQuorumDoesNotArmAnEarlyCommitTest::RunTest(const FString&)
{
	FRTParticipantMatch M = MakeParticipantMatch();
	if (!TestNotNull(TEXT("turn manager"), M.TM)) { DestroyHexMatchWorld(M.World); return false; }

	// La partita non presidiata: ogni unita' e' pianificata dal bot, come `Config.bAutobattle` la allestisce.
	M.A->bIsBotControlled = true;
	M.A2->bIsBotControlled = true;

	TestFalse(TEXT("con nessuno che possa premere, il quorum NON e' soddisfatto"), M.TM->HasReadyQuorum());

	const int32 TurnoPrima = M.TM->GetTurnNumber();
	M.TM->DeclareParticipantReady(0, 0);

	TestFalse(TEXT("un Ready da chi non e' eleggibile non entra"), M.TM->IsParticipantReady(0, 0));
	TestFalse(TEXT("e non arma nessun countdown"), M.TM->IsReadyCountdownActive());

	// Oltre la durata del countdown il turno non e' avanzato: il commit di queste partite resta quello del
	// tetto, cioe' `OnPlanningTimeout`.
	AdvanceWallClock(M.World, 4.0f);
	TestEqual(TEXT("il turno non si e' chiuso da solo dopo il countdown"), M.TM->GetTurnNumber(), TurnoPrima);

	DestroyHexMatchWorld(M.World);
	return true;
}

/**
 * **Partecipanti distinti non si contaminano, e il countdown aspetta l'ultimo** (`D016.5`, `D016.7`).
 *
 * E' il caso che in v0.1 non si osserva — un posto per squadra — e che esiste perche' il quorum sia gia'
 * corretto quando il formato lo produrra'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDistinctParticipantsDoNotContaminateTest,
	"RefactorTactics.HexMatch.DistinctParticipantsDoNotContaminate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDistinctParticipantsDoNotContaminateTest::RunTest(const FString&)
{
	FRTParticipantMatch M = MakeParticipantMatch();
	if (!TestNotNull(TEXT("turn manager"), M.TM)) { DestroyHexMatchWorld(M.World); return false; }

	// Due posti nella stessa squadra: e' la ripartizione che `UnitsPerPlayer = 1` produrrebbe.
	M.A2->ControlGroup = 1;

	M.TM->DeclareParticipantReady(0, 0);
	TestTrue(TEXT("il primo partecipante e' Ready"), M.TM->IsParticipantReady(0, 0));
	TestFalse(TEXT("ma il secondo NO: il Ready non ha attraversato il gruppo"), M.TM->IsParticipantReady(0, 1));
	TestFalse(TEXT("quindi il quorum non e' chiuso"), M.TM->HasReadyQuorum());
	TestFalse(TEXT("e nessun countdown e' partito"), M.TM->IsReadyCountdownActive());

	M.TM->DeclareParticipantReady(0, 1);
	TestTrue(TEXT("con l'ultimo il quorum si chiude"), M.TM->HasReadyQuorum());
	TestTrue(TEXT("e solo ora parte il countdown"), M.TM->IsReadyCountdownActive());

	DestroyHexMatchWorld(M.World);
	return true;
}

/**
 * **Il compagno pianificato dal bot non entra nel quorum — oggi** (`D014`).
 *
 * 🔑 Questo test pinna un CONFINE, non una funzionalita': la readiness del bot alleato — finestra di
 * coordinamento, replanning, revoca sopra soglia — e' di `#534` (`CP 26.4`, post-v0.1). Se un giorno il bot
 * entrera' nel quorum, questo test deve CADERE ed essere riscritto li': e' il modo in cui il confine si
 * fa sentire invece di essere dimenticato.
 *
 * ⚠️ E prova anche la ragione per cui l'eleggibilita' guarda `bIsBotControlled` e non `TeamId`: qui le due
 * unita' sono nella stessa squadra E nello stesso `ControlGroup`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAllyBotDoesNotJoinTheQuorumYetTest,
	"RefactorTactics.HexMatch.AllyBotDoesNotJoinTheQuorumYet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAllyBotDoesNotJoinTheQuorumYetTest::RunTest(const FString&)
{
	FRTParticipantMatch M = MakeParticipantMatch();
	if (!TestNotNull(TEXT("turn manager"), M.TM)) { DestroyHexMatchWorld(M.World); return false; }

	// `rt.Match.BotAllies=1`: il secondo Hero della squadra 0 lo pianifica il bot, e resta nel gruppo 0.
	M.A2->bIsBotControlled = true;

	M.TM->DeclareParticipantReady(0, 0);

	TestTrue(TEXT("il quorum si chiude sul solo Ready umano"), M.TM->HasReadyQuorum());
	TestTrue(TEXT("e il countdown parte"), M.TM->IsReadyCountdownActive());

	DestroyHexMatchWorld(M.World);
	return true;
}

/**
 * **La readiness non sopravvive al turno** (`D016.8`).
 *
 * ⛔ Il difetto che coglie: un `TSet` mai svuotato renderebbe il turno 2 gia' in quorum all'apertura, e il
 * commit arriverebbe dal countdown su un piano che nessuno ha ancora scritto. Non e' un caso di parete —
 * e' l'unico modo in cui uno stato di readiness puo' diventare autorita' senza che nessuno lo decida.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReadinessDoesNotSurviveIntoTheNextPlanningTest,
	"RefactorTactics.HexMatch.ReadinessDoesNotSurviveIntoTheNextPlanning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReadinessDoesNotSurviveIntoTheNextPlanningTest::RunTest(const FString&)
{
	FRTParticipantMatch M = MakeParticipantMatch();
	if (!TestNotNull(TEXT("turn manager"), M.TM)) { DestroyHexMatchWorld(M.World); return false; }

	const int32 TurnoPrima = M.TM->GetTurnNumber();
	M.TM->DeclareParticipantReady(0, 0);
	if (!TestTrue(TEXT("premessa: il countdown e' armato"), M.TM->IsReadyCountdownActive()))
	{
		DestroyHexMatchWorld(M.World); return false;
	}

	AdvanceWallClock(M.World, 3.5f); // il countdown scade e committa
	DrainPlayback(M.TM);             // il turno avanza quando il playback finisce

	if (!TestTrue(TEXT("premessa: il turno e' avanzato"), M.TM->GetTurnNumber() > TurnoPrima))
	{
		DestroyHexMatchWorld(M.World); return false;
	}

	TestFalse(TEXT("nel turno nuovo il partecipante NON e' piu' Ready"), M.TM->IsParticipantReady(0, 0));
	TestFalse(TEXT("il quorum e' riaperto"), M.TM->HasReadyQuorum());
	TestFalse(TEXT("e nessun countdown si e' riarmato da solo"), M.TM->IsReadyCountdownActive());

	DestroyHexMatchWorld(M.World);
	return true;
}

// =========================================================================================================
// `#3261` — LA SPINTA NON PASSAVA DAL PLAYBACK.
//
// 🔴 Trovata in seduta PIE guardando lo schermo, non leggendo il codice: *«il cilindro viene spinto, ma chi
// lo spinge non arriva vicino, prima vedo muoversi quello spinto e poi chi spinge»*. Il bersaglio era gia'
// sulla cella d'arrivo nel primo fotogramma della riproduzione.
//
// 🔑 **La catena**: l'evento di `ApplyForcedDisplacement` nasceva senza `CellVerdicts`;
// `ObservedPrefixLength` e' fail-closed sul disallineamento e rispondeva `0` per **qualunque** squadra; il
// `Visible < 2` di `BuildPlayback` scartava l'anim — mentre il passo 7 aveva gia' chiamato
// `SetVisualLocation`, durante la risoluzione, che gira tutta prima di `BeginPlayback`.
//
// ⚠️ **I test esistenti pinnano lo STATO FINALE e restavano verdi**: `HexMatch.ChargeStopsOnEnemyAndHits`
// asserisce che il bersaglio finisca una cella piu' in la', e resta vero con la spinta invisibile. Quello
// che segue pinna la **sequenza**.
// =========================================================================================================

namespace
{
	/** La scena della carica: Branth carica Ivrin, la colpisce e la spinge. E' quella della seduta PIE. */
	struct FRTPushScene
	{
		UWorld* World = nullptr;
		ARTTurnManager* TM = nullptr;
		ARTUnit* Charger = nullptr;
		ARTUnit* Target = nullptr;
		bool bValid = false;
	};

	FRTPushScene MakePushScene()
	{
		FRTPushScene S;
		S.World = MakeHexMatchWorld();
		if (!S.World) { return S; }
		SpawnHexMatchMap(S.World, /*Radius=*/ 5);

		S.Charger = SpawnHexMatchUnit(S.World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
		S.Target  = SpawnHexMatchUnit(S.World, 1, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(3, 0));
		S.TM = S.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!S.TM || !S.Charger || !S.Target) { return S; }
		S.Charger->bIsBotControlled = false;
		S.Target->bIsBotControlled = false;

		// La carica si cerca per `ActionId` e non per indice, come nel test gemello: l'indice e' una
		// convenzione del kit e cambierebbe in silenzio il soggetto della prova.
		int32 ChargeIdx = INDEX_NONE;
		for (int32 i = 0; i < S.Charger->NumAbilities(); ++i)
		{
			const URTActionData* A = S.Charger->GetAbility(i);
			if (A && A->Def.ActionId == FName(TEXT("Hero.Branth.Ram"))) { ChargeIdx = i; break; }
		}
		if (ChargeIdx == INDEX_NONE) { return S; }

		S.Charger->PlannedDashAbility = ChargeIdx;
		S.Charger->PlannedDashCell = S.Target->Cell;
		S.bValid = true;
		return S;
	}

	/** L'evento di playback dello spostamento di `StableUnitId`, se c'e'. */
	const FRTResolvedEvent* PushEventFor(const ARTTurnManager* TM, int32 StableUnitId)
	{
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::Move && Ev.SourceStableUnitId == StableUnitId
				&& Ev.Phase == ERTMatchPhase::Blast)
			{
				return &Ev;
			}
		}
		return nullptr;
	}
}

/**
 * L'evento della spinta porta i propri **verdetti**, uno per cella — `#3261`.
 *
 * 🔴 **E' la causa, e non nasceva da una svista.** Fino al 2026-08-30 non c'era nessun filtro davanti a
 * questo evento (`#541`, 2026-08-11); il filtro entra con `#1525`, applicato a **ogni** evento `Move`, e il
 * knockback era l'unico `Move` che non popolava `CellVerdicts`. E' caduto fuori senza che nulla lo dicesse.
 *
 * ⚠️ **Si chiede al BERSAGLIO, non al caricante**, ed e' la differenza fra un gate e un verde che non dice
 * nulla: sul caricante i verdetti c'erano gia' (li scrive `ResolveDash`), quindi chiederli a lui sarebbe
 * stato verde anche prima della correzione.
 *
 * 🔑 **E il numero deve COMBACIARE con le celle del percorso**, non essere «maggiore di zero»:
 * `ObservedPrefixLength` e' fail-closed proprio sul **disallineamento** — `CellVerdicts.Num() !=
 * Cells.Num()` risponde `0` — quindi un array di verdetti piu' corto del `Path` lascerebbe il difetto in
 * piedi e questo gate verde.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPushCarriesItsVerdictsTest,
	"RefactorTactics.HexMatch.ForcedDisplacementCarriesItsVerdicts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPushCarriesItsVerdictsTest::RunTest(const FString&)
{
	FRTPushScene S = MakePushScene();
	if (!TestTrue(TEXT("scena costruita"), S.bValid)) { DestroyHexMatchWorld(S.World); return false; }

	RTWorldFixtures::PlayOneTurn(S.TM);

	// ⛔ PREMESSA: la spinta e' davvero avvenuta. Senza, il gate misurerebbe l'assenza di un evento invece
	// del suo contenuto.
	const FRTResolvedEvent* Spinta = PushEventFor(S.TM, S.Target->StableUnitId);
	if (!TestNotNull(TEXT("⛔ premessa: il bersaglio ha un evento di spostamento nel Blast"), Spinta))
	{
		DestroyHexMatchWorld(S.World);
		return false;
	}
	if (!TestTrue(TEXT("⛔ premessa: il percorso ha almeno due celle"), Spinta->Path.Num() >= 2))
	{
		DestroyHexMatchWorld(S.World);
		return false;
	}

	// --- IL FATTO ------------------------------------------------------------------------------------
	const int32 Verdetti = S.TM->ResolvedMoveVerdictCountForTest(S.Target->StableUnitId);
	TestEqual(TEXT("🔴 un verdetto per cella: tanti quante le celle del percorso"),
		Verdetti, Spinta->Path.Num());

	// ⛔ **E la regola di `#1525` si conserva**: il filtro non e' stato allentato, all'evento e' stato dato
	// il dato che il filtro chiede. Una squadra che non e' fra gli osservatori non vede nulla — qui una
	// squadra che nella partita non esiste, che e' il caso deterministico.
	const int32 PrefissoIgnoto = URTTeamKnowledgeLibrary::ObservedPrefixLength(
		Spinta->Path, Spinta->CellVerdicts, /*TeamId=*/ 7);
	TestEqual(TEXT("⛔ una squadra che non osserva non vede il percorso: il filtro resta chiuso"),
		PrefissoIgnoto, 0);

	// ✅ Mentre la squadra del soggetto la propria traccia la vede sempre ([D-223]).
	const int32 PrefissoProprio = URTTeamKnowledgeLibrary::ObservedPrefixLength(
		Spinta->Path, Spinta->CellVerdicts, S.Target->TeamId);
	TestEqual(TEXT("✅ e la squadra di chi e' stato spinto lo vede per intero"),
		PrefissoProprio, Spinta->Path.Num());

	DestroyHexMatchWorld(S.World);
	return true;
}

/**
 * E a schermo la spinta **accade**, invece di essere gia' accaduta — `#3261`.
 *
 * 🔴 **E' la SEQUENZA, che nessun test pinnava.** I gate esistenti su carica e knockback asseriscono celle
 * e HP — lo stato **dopo** — e restavano verdi con il bersaglio gia' arrivato nel primo fotogramma. Questo
 * misura che la posizione visiva dell'unita' spinta **cambi durante la riproduzione**, che e' ciò che la
 * seduta PIE ha visto mancare.
 *
 * 🔑 **La sonda non ha bisogno di coordinate assolute.** Senza la correzione l'anim non nasce, quindi
 * nessuno riporta il cilindro sulla cella di partenza e la posizione all'inizio del playback e' **gia**'
 * quella finale: `Iniziale == Finale`. Con la correzione l'anim nasce, `BuildPlayback` lo riposiziona sullo
 * start, e le due posizioni differiscono.
 *
 * ⚠️ **Il passo 7 (`SetVisualLocation`) non e' stato toccato, ed e' corretto cosi'.** Il riposizionamento
 * di `BuildPlayback` lo annulla quando l'anim nasce; quando non nasce — un osservatore che non ha diritto
 * di vedere quel percorso — il cilindro deve restare dov'e' arrivato, e quel `SetVisualLocation` e'
 * l'unica cosa che ce lo porta.
 *
 * ⛔ **Cio' che questo gate NON misura**: che la finestra della fase sia dimensionata bene. Aprire una fase
 * e dimensionarla sono due cose distinte; la seconda si giudica a occhio, ed e' di una voce `PIE-*`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPushIsAnimatedNotAlreadyDoneTest,
	"RefactorTactics.HexMatch.ForcedDisplacementIsAnimatedNotAlreadyDone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPushIsAnimatedNotAlreadyDoneTest::RunTest(const FString&)
{
	FRTPushScene S = MakePushScene();
	if (!TestTrue(TEXT("scena costruita"), S.bValid)) { DestroyHexMatchWorld(S.World); return false; }

	const FRTCellId CellaIniziale = S.Target->Cell;

	// ⚠️ `LockInAndResolve` e non `PlayOneTurn`: serve il playback **a meta'**, non il suo esito.
	S.TM->LockInAndResolve();
	if (!TestTrue(TEXT("⛔ premessa: il turno sta riproducendo"), S.TM->IsResolving()))
	{
		DestroyHexMatchWorld(S.World);
		return false;
	}

	// ⛔ PREMESSA: la spinta c'e' ed e' nel Blast. Senza, tutto il resto confronterebbe due posizioni
	// uguali per una ragione che non c'entra.
	const FRTResolvedEvent* Spinta = PushEventFor(S.TM, S.Target->StableUnitId);
	if (!TestNotNull(TEXT("⛔ premessa: il bersaglio ha un evento di spostamento nel Blast"), Spinta))
	{
		DestroyHexMatchWorld(S.World);
		return false;
	}
	if (!TestTrue(TEXT("⛔ premessa: e la sua cella logica e' cambiata"), S.Target->Cell != CellaIniziale))
	{
		DestroyHexMatchWorld(S.World);
		return false;
	}

	// --- LA SEQUENZA ---------------------------------------------------------------------------------
	const FVector Iniziale = S.Target->GetActorLocation();

	FVector Precedente = Iniziale;
	int32 Cambiamenti = 0;
	for (int32 I = 0; I < 600 && S.TM->IsResolving(); ++I)
	{
		S.TM->Tick(0.05f);
		const FVector Ora = S.Target->GetActorLocation();
		if (!Ora.Equals(Precedente, 1.f)) { ++Cambiamenti; }
		Precedente = Ora;
	}
	const FVector Finale = S.Target->GetActorLocation();

	// 🔴 All'inizio della riproduzione il cilindro NON e' ancora arrivato.
	TestFalse(TEXT("🔴 la spinta non e' gia' avvenuta quando il playback comincia"),
		Iniziale.Equals(Finale, 1.f));
	// 🔴 E ci arriva muovendosi: almeno un istante in cui la posizione cambia.
	TestTrue(TEXT("🔴 e il cilindro si muove durante la riproduzione, invece di apparire"),
		Cambiamenti > 0);

	DestroyHexMatchWorld(S.World);
	return true;
}

/**
 * Una spinta **senza danno** si vede lo stesso — e il caso «spinta SOLA nel Blast» oggi non esiste — `#3261`.
 *
 * 🔑 **L'istruttoria della issue chiedeva di confermare che la fase si apra con la sola
 * `bHasBlastMove`.** `BlastPhaseIsActive` ha quattro ragioni indipendenti, e nello scenario della carica la
 * fase si apre perche' c'e' il colpo: li' la spinta ci viaggia dentro *per caso, non per diritto*.
 *
 * ⛔ **Misurato scrivendo questo gate: quel caso non e' raggiungibile con il catalogo attuale.**
 * `Action.Push` e' l'unica azione che spinge **senza fare danno**, e dichiara `bCountsAsAttack = true` —
 * *«controllo OSTILE: raggiunge il bersaglio come colpo»*. ∴ produce comunque un `Attack` (a danno zero) e
 * un'impronta, e la fase si apre per la **prima** ragione prima ancora che serva la quarta.
 *
 * ⚠️ **Il gate pinna quel fatto invece di assumerlo**, ed e' cio' che lo rende utile: il giorno in cui
 * nascera' una spinta che non conta come attacco — una reazione che sposta e basta — queste righe
 * diventeranno rosse, e chi le legge sapra' che il cancello di fase va riverificato. Finche' restano verdi,
 * l'argomento dell'istruttoria non ha un caso da difendere.
 *
 * ✅ E cio' che il gate misura davvero resta: una spinta **a danno zero** arriva a schermo come
 * movimento, invece di essere gia' avvenuta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPushWithoutDamageIsAnimatedTest,
	"RefactorTactics.HexMatch.PushWithoutDamageIsStillAnimated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPushWithoutDamageIsAnimatedTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMatchWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMatchMap(World, /*Radius=*/ 5);

	ARTUnit* Spingitore = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
	ARTUnit* Spinto     = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Spingitore || !Spinto) { DestroyHexMatchWorld(World); return false; }
	Spingitore->bIsBotControlled = false;
	Spinto->bIsBotControlled = false;

	// `Action.Push` si aggiunge all'istanza, come in tutti gli scenari che usano un'azione fuori dal kit
	// dell'eroe. ⚠️ Lo slot 3, per non sovrascrivere l'attacco base — che non dev'essere pianificato.
	RTAbilityFixtures::AddCoreAbilityInSlot(Spingitore, TEXT("Action.Push"), 3);
	Spingitore->PlannedAbilityIndex = 3;
	Spingitore->DeclareAttackOnUnit(Spinto);

	TM->LockInAndResolve();
	if (!TestTrue(TEXT("⛔ premessa: il turno sta riproducendo"), TM->IsResolving()))
	{
		DestroyHexMatchWorld(World);
		return false;
	}

	// --- ⛔ LE PREMESSE ----------------------------------------------------------------------------
	const FRTResolvedEvent* Spinta = PushEventFor(TM, Spinto->StableUnitId);
	if (!TestNotNull(TEXT("⛔ premessa: la spinta ha prodotto un evento nel Blast"), Spinta))
	{
		DestroyHexMatchWorld(World);
		return false;
	}

	// 🔴 **IL FATTO PINNATO, e non e' una premessa qualunque**: `Action.Push` non fa danno eppure
	// produce un `Attack`, perche' dichiara `bCountsAsAttack`. ∴ la fase si apre per la PRIMA ragione del
	// cancello, e il caso «spinta sola nel Blast» che l'istruttoria temeva non e' raggiungibile oggi.
	// ⚠️ Se una di queste due righe diventasse rossa, quel caso esisterebbe: il cancello di fase va
	// riverificato, ed e' il servizio che queste righe rendono.
	TestEqual(TEXT("🔴 `Action.Push` conta come attacco: un `Attack` c'e', a danno zero"),
		TM->ResolvedEventCountOfTypeForTest(ERTResolvedEventType::Attack), 1);
	TestEqual(TEXT("🔴 e con esso l'impronta del suo intento"),
		TM->ResolvedEventCountOfTypeForTest(ERTResolvedEventType::AttackFootprint), 1);
	// ✅ E il danno e' davvero zero: senza questa riga il test starebbe misurando un colpo normale.
	TestEqual(TEXT("✅ e il bersaglio non perde vita: e' controllo, non danno"),
		Spinto->Health, Spinto->MaxHealth);

	// --- IL FATTO: la fase si apre, e la spinta ci si vede dentro ------------------------------------
	bool bBlastVisto = false;
	const FVector Iniziale = Spinto->GetActorLocation();
	for (int32 I = 0; I < 600 && TM->IsResolving(); ++I)
	{
		if (TM->GetPlaybackPhaseName() == TEXT("Blast")) { bBlastVisto = true; }
		TM->Tick(0.05f);
	}

	TestTrue(TEXT("✅ la fase Blast si apre e la spinta ci sta dentro"), bBlastVisto);
	TestFalse(TEXT("🔴 e la spinta non era gia' avvenuta quando il playback e' cominciato"),
		Iniziale.Equals(Spinto->GetActorLocation(), 1.f));

	DestroyHexMatchWorld(World);
	return true;
}

/**
 * DUE attaccanti sullo stesso bersaglio: ogni voce porta la PROPRIA quota, e la somma e' il netto —
 * `#3271`, regola [D-438].
 *
 * 🔑 **E' il contratto, e prima era la caratterizzazione del difetto opposto.** Fino a [D-438] ogni voce
 * dichiarava il totale di FASE: chi guardava la seduta PIE ha riferito *«vedo due colpi da 37»* mentre i
 * colpi valgono 21 ciascuno, e sommare il feed dava il DOPPIO del danno reale. Questo test e' cambiato
 * con la decisione, che era il suo scopo dichiarato.
 *
 * 🔑 **Il caso a DUE attaccanti e' quello che ha protetto il difetto**: con uno solo il netto di fase
 * coincide con quello del colpo, le due formule non si distinguono, e nessun test del repository
 * costruiva la scena a due. E' anche la ragione per cui il corpus golden non si muove.
 *
 * ⚠️ **Il netto per-colpo non esiste a monte**: `URTCombatResolver::ApplyHits` somma i colpi per
 * bersaglio (`DamageByTarget += Attack.Power`) e applica lo scudo **una volta sola** sul totale
 * (invariante #3, stadio `ERTDamageStage::TargetSum`). ∴ la quota e' un'ATTRIBUZIONE dichiarata, non una
 * misura ritrovata — ed e' [D-438] a dichiararla.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTwoAttackersNetShareTest,
	"RefactorTactics.HexMatch.TwoAttackersSplitTheNetDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTwoAttackersNetShareTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMatchWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMatchMap(World, /*Radius=*/ 5);

	// Due attaccanti di squadra 0 ai lati del bersaglio, entrambi adiacenti: due colpi nella stessa fase.
	ARTUnit* PrimoA   = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
	ARTUnit* SecondoA = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(2, 0));
	ARTUnit* Bersaglio = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !PrimoA || !SecondoA || !Bersaglio) { DestroyHexMatchWorld(World); return false; }
	PrimoA->bIsBotControlled = false;
	SecondoA->bIsBotControlled = false;
	Bersaglio->bIsBotControlled = false;

	PrimoA->PlannedAbilityIndex = 0;
	PrimoA->DeclareAttackOnUnit(Bersaglio);
	SecondoA->PlannedAbilityIndex = 0;
	SecondoA->DeclareAttackOnUnit(Bersaglio);
	Bersaglio->ClearPlannedAttack();

	const int32 VitaPrima = Bersaglio->Health;
	RTWorldFixtures::PlayOneTurn(TM);

	// --- ⛔ LE PREMESSE: due voci di Combat sullo stesso bersaglio ------------------------------------
	TArray<FRTTurnLogEntry> Colpi;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category == ERTLogCategory::Combat && E.TgtCell == FRTCellId(1, 0)) { Colpi.Add(E); }
	}
	if (!TestEqual(TEXT("⛔ premessa: DUE colpi sullo stesso bersaglio nella stessa fase"), Colpi.Num(), 2))
	{
		DestroyHexMatchWorld(World);
		return false;
	}

	const int32 PersiDavvero = VitaPrima - Bersaglio->Health;
	if (!TestTrue(TEXT("⛔ premessa: il bersaglio ha perso vita"), PersiDavvero > 0))
	{
		DestroyHexMatchWorld(World);
		return false;
	}

	// --- IL CONTRATTO ---------------------------------------------------------------------------------
	//
	// 🔑 **La proprieta' che [D-438] promette: la somma delle voci e' il danno reale.** E' quella che il
	// giocatore usa senza saperlo — legge due righe e le somma — e quella che il difetto rompeva: prima
	// dava il DOPPIO.
	TestEqual(TEXT("🔑 la somma delle due voci e' cio' che gli HP hanno perso"),
		Colpi[0].Amount + Colpi[1].Amount, PersiDavvero);

	// ⛔ E nessuna delle due dichiara il totale: e' l'asserzione che distingue la ripartizione dal
	// comportamento precedente, dove entrambe lo dichiaravano.
	TestTrue(TEXT("⛔ la prima voce NON vale l'intera fase"), Colpi[0].Amount < PersiDavvero);
	TestTrue(TEXT("⛔ e nemmeno la seconda"), Colpi[1].Amount < PersiDavvero);
	TestTrue(TEXT("⛔ entrambe portano un danno proprio, non zero"),
		Colpi[0].Amount > 0 && Colpi[1].Amount > 0);

	// ⚠️ `Outcome` segue la STESSA ripartizione, letta come progressione: il bersaglio sopravvive, quindi
	// nessuna delle due voci e' `Lethal`. Il caso in cui muore lo misura
	// `HexMatch.TwoAttackersMarkOneLethalEntry`, che e' dove la lettura di fase sbagliava piu' forte.
	TestTrue(TEXT("⚠️ il bersaglio e' vivo: nessuna voce e' Lethal"),
		Colpi[0].Outcome != static_cast<uint8>(ERTCombatOutcome::Lethal)
		&& Colpi[1].Outcome != static_cast<uint8>(ERTCombatOutcome::Lethal));

	DestroyHexMatchWorld(World);
	return true;
}

/**
 * DUE attaccanti che UCCIDONO: una sola voce e' `Lethal` — `#3271`, regola [D-438].
 *
 * 🔑 **E' il caso in cui la lettura di FASE sbagliava piu' forte.** `ClassifyCombatOutcome(BeforeHP,
 * AfterHP)` sulle fotografie dell'intero Blast rispondeva `Lethal` per OGNI colpo della fase: il feed
 * dichiarava due uccisioni dove ce n'e' una, e il secondo attaccante si vedeva attribuire un colpo
 * mortale che non ha inferto.
 *
 * ⚠️ **La progressione non e' una seconda regola**: e' la ripartizione di [D-438] letta un'altra volta,
 * colpo dopo colpo. ∴ `Lethal` cade dove la quota cumulativa azzera gli HP — l'ultimo colpo sul
 * bersaglio — che e' anche la sola attribuzione che il codice puo' fare senza inventare: il commento di
 * `Defeated` rifiuta esplicitamente di nominare «chi ha inferto il colpo finale» per l'EVENTO di
 * eliminazione, e questo non lo contraddice, perche' qui la progressione e' dichiarata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTwoAttackersLethalEntryTest,
	"RefactorTactics.HexMatch.TwoAttackersMarkOneLethalEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTwoAttackersLethalEntryTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMatchWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMatchMap(World, /*Radius=*/ 5);

	ARTUnit* PrimoA   = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
	ARTUnit* SecondoA = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(2, 0));
	ARTUnit* Bersaglio = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !PrimoA || !SecondoA || !Bersaglio) { DestroyHexMatchWorld(World); return false; }
	PrimoA->bIsBotControlled = false;
	SecondoA->bIsBotControlled = false;
	Bersaglio->bIsBotControlled = false;

	// Vita bassa quanto basta perche' i due colpi insieme la esauriscano: e' la scena che discrimina.
	Bersaglio->Health = 20;

	PrimoA->PlannedAbilityIndex = 0;
	PrimoA->DeclareAttackOnUnit(Bersaglio);
	SecondoA->PlannedAbilityIndex = 0;
	SecondoA->DeclareAttackOnUnit(Bersaglio);
	Bersaglio->ClearPlannedAttack();

	const int32 VitaPrima = Bersaglio->Health;
	RTWorldFixtures::PlayOneTurn(TM);

	TArray<FRTTurnLogEntry> Colpi;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category == ERTLogCategory::Combat && E.TgtCell == FRTCellId(1, 0)) { Colpi.Add(E); }
	}
	if (!TestEqual(TEXT("⛔ premessa: DUE colpi sullo stesso bersaglio"), Colpi.Num(), 2))
	{
		DestroyHexMatchWorld(World);
		return false;
	}
	const int32 VitaDopo = IsValid(Bersaglio) ? Bersaglio->Health : 0;
	if (!TestTrue(TEXT("⛔ premessa: il bersaglio e' caduto"), VitaDopo <= 0))
	{
		DestroyHexMatchWorld(World);
		return false;
	}

	// --- IL CONTRATTO ---------------------------------------------------------------------------------
	int32 Letali = 0;
	for (const FRTTurnLogEntry& E : Colpi)
	{
		if (E.Outcome == static_cast<uint8>(ERTCombatOutcome::Lethal)) { ++Letali; }
	}
	TestEqual(TEXT("🔑 UNA sola voce e' Lethal, non entrambe"), Letali, 1);

	// ⚠️ Ed e' la SECONDA: la prima quota non basta ad azzerare, la seconda si'. Asserirlo tiene la
	// progressione, che e' cio' che distingue questa lettura da quella di fase.
	TestEqual(TEXT("⚠️ la voce letale e' l'ultimo colpo sul bersaglio"),
		Colpi[1].Outcome, static_cast<uint8>(ERTCombatOutcome::Lethal));

	// La somma resta il danno reale anche quando il bersaglio cade — la proprieta' non ha eccezioni.
	TestEqual(TEXT("🔑 la somma delle quote e' cio' che gli HP hanno perso"),
		Colpi[0].Amount + Colpi[1].Amount, VitaPrima - VitaDopo);

	DestroyHexMatchWorld(World);
	return true;
}

/**
 * `#3471` — IL PULSANTE `ANNULLA` DURANTE IL COUNTDOWN RITIRA IL READY, E IL PIANO RESTA ([D-458], `#2193`).
 *
 * 🔑 **Passa dal CONTROLLER, ed e' il pezzo che mancava**: `UnreadyReturnsToPlanningWithThePlanIntact` chiama
 * `TM->CancelLockIn()` direttamente, quindi il ramo del Back che ritira il Ready non aveva un test che lo
 * attraversasse dalla porta. Qui ci arriva dal pulsante della HUD.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUndoButtonWithdrawsReadyTest,
	"RefactorTactics.HexMatch.UndoButtonWithdrawsTheReadyAndKeepsThePlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUndoButtonWithdrawsReadyTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMatchWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTTurnManager* TM = MakeCountdownMatch(World);
	if (!TestNotNull(TEXT("turn manager"), TM)) { DestroyHexMatchWorld(World); return false; }

	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), Actors);
	ARTUnit* Umana = nullptr;
	for (AActor* Actor : Actors)
	{
		ARTUnit* U = Cast<ARTUnit>(Actor);
		if (U && !U->bIsBotControlled) { Umana = U; break; }
	}
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	URTPlanCommitWidget* W = NewObject<URTPlanCommitWidget>(World);
	if (!TestNotNull(TEXT("l'unita' umana"), Umana) || !TestNotNull(TEXT("controller"), PC)
		|| !TestNotNull(TEXT("widget"), W))
	{
		DestroyHexMatchWorld(World);
		return false;
	}
	PC->SelectActorForTest(Umana);
	W->SetCommandControllerForTest(PC);

	Umana->PlannedWaypoints.Add(FRTCellId(-2, 1));
	const TArray<FRTCellId> PianoPrima = Umana->PlannedWaypoints;

	// ⚠️ **Il Ready col GESTO del giocatore, non con `TM->RequestLockIn()`.** `Spazio` dichiara il Ready DEL
	// PARTECIPANTE (`DeclareParticipantReady`), ed e' quello che il Back ritira con `WithdrawParticipantReady`:
	// `RequestLockIn` arma il countdown per un'altra strada, che nessun partecipante possiede. La prima stesura
	// lo usava, ed era rossa per la premessa sbagliata, non per il prodotto.
	PC->OnLockInForTest();
	if (!TestTrue(TEXT("premessa: il countdown e' armato"), TM->IsReadyCountdownActive()))
	{
		DestroyHexMatchWorld(World);
		return false;
	}

	W->Undo();
	TestFalse(TEXT("Annulla ritira il Ready: il countdown e' annullato"), TM->IsReadyCountdownActive());
	TestTrue(TEXT("si e' in pianificazione"), TM->GetPhase() == ERTMatchPhase::Planning);
	TestTrue(TEXT("e il piano e' identico: il Back non ha smontato un waypoint"),
		Umana->PlannedWaypoints == PianoPrima);

	DestroyHexMatchWorld(World);
	return true;
}

/**
 * [D-528] (`#3769`) — **un piano oltre la portata con un altro nemico noto in mezzo risolve e COLPISCE quello in mezzo.**
 *
 * Il controller, da [D-528], pianifica la carica su B anche se A, entro la portata, sta sulla retta. Il resolver non cambia: la
 * corsa verso B incontra A e si ferma accanto a lui con un impatto ([D-296]); il caso non e' `OutOfRange`, quindi la rivalidazione
 * di [D-516] non lo annulla e la traccia non scrive un ripiego «fuori portata». Questo test lo pinna, perche' il piano che il
 * giocatore vede (anteprima ferma davanti ad A, avviso «colpira' A, non B») e' vero solo se la risoluzione fa cosi'.
 *
 * Il controllo positivo e' `HexMatch.ChargeBeyondRangeRunsToRangeAndStops`: stessa geometria senza A, e la carica corre per
 * tutta la portata senza colpire nessuno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexChargeBeyondRangeInTheWayTest,
	"RefactorTactics.HexMatch.ChargeBeyondRangeWithAnEnemyInTheWayHitsIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexChargeBeyondRangeInTheWayTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMatchWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMatchMap(World, /*Radius=*/ 7);

	ARTUnit* Charger = SpawnHexMatchUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Charger) { DestroyHexMatchWorld(World); return false; }
	Charger->bIsBotControlled = false;

	int32 ChargeIdx = INDEX_NONE;
	for (int32 i = 0; i < Charger->NumAbilities(); ++i)
	{
		const URTActionData* A = Charger->GetAbility(i);
		if (A && A->Def.ActionId == FName(TEXT("Hero.Branth.Ram"))) { ChargeIdx = i; break; }
	}
	const int32 Portata = Charger->EffectiveDashRangeOf(ChargeIdx);
	if (!TestTrue(TEXT("premessa: Branth ha la carica, con una portata che contiene A e lascia B oltre"),
		ChargeIdx != INDEX_NONE && Portata >= 2 && Portata + 2 <= 6))
	{
		DestroyHexMatchWorld(World); return false;
	}

	ARTUnit* InMezzo = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(2, 0));
	ARTUnit* Bersaglio = SpawnHexMatchUnit(World, 1, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(Portata + 2, 0));
	if (!InMezzo || !Bersaglio) { DestroyHexMatchWorld(World); return false; }
	InMezzo->bIsBotControlled = false;
	Bersaglio->bIsBotControlled = false;

	const int32 VitaInMezzo = InMezzo->Health;
	const int32 VitaBersaglio = Bersaglio->Health;
	const FRTCellId CellaBersaglio = Bersaglio->Cell;
	Charger->PlannedDashAbility = ChargeIdx;
	Charger->PlannedDashCell = CellaBersaglio; // come lo scrive il click, da [D-528]
	RTWorldFixtures::PlayOneTurn(TM);

	TestEqual(TEXT("chi carica si ferma accanto a quello in mezzo"), Charger->Cell.ToString(), FRTCellId(1, 0, 0).ToString());
	TestTrue(TEXT("e lo colpisce"), InMezzo->Health < VitaInMezzo);
	TestEqual(TEXT("il bersaglio scelto non e' toccato"), Bersaglio->Health, VitaBersaglio);
	TestEqual(TEXT("ne' spostato"), Bersaglio->Cell.ToString(), CellaBersaglio.ToString());

	int32 Annullate = 0;
	int32 FuoriPortata = 0;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category != ERTLogCategory::Fallback || E.Phase != ERTMatchPhase::Dash
			|| E.ActionId != FName(TEXT("Hero.Branth.Ram")))
		{
			continue;
		}
		if (E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Cancelled)) { ++Annullate; }
		if (E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Stopped)
			&& E.Amount == static_cast<int32>(ERTActionInvalidReason::OutOfRange)) { ++FuoriPortata; }
	}
	TestEqual(TEXT("nessun annullamento: la rivalidazione di D-516 riguarda la corsa che arriva in fondo"), Annullate, 0);
	TestEqual(TEXT("e nessun ripiego «fuori portata»: la corsa e' stata fermata da un impatto"), FuoriPortata, 0);

	DestroyHexMatchWorld(World);
	return true;
}

// =====================================================================================================
// `#3511` — le aree di pianificazione hanno UN ingresso, UN reset, UN contatore.
//
// Il banco e' quello di `#2390` (`FRTLockInPreviewBench`), e questi test non chiamano `PlayerTick`: chiamano
// `UpdatePlanningOverlay` direttamente, che e' esattamente il punto della issue — *«una funzione chiamabile senza
// `PlayerTick` e con un contatore d'invocazioni leggibile dai test»*. L'hover si posa dal seam di `FSlotRefusalBench`,
// `ARTHexMapActor::SetHoveredCell`.
//
// 🔑 **I test di reset staccano il controller dagli EVENTI** (`OnLockInCommitted`, `OnResolvePlaybackFinished`,
// `OnMatchEnded`): cosi' a spegnere le aree puo' essere solo lo STATO, letto da `ShowsAreas()`. Con gli eventi attaccati un
// test verde potrebbe esserlo grazie all'elenco di uscite che la issue vuole sostituire.
// =====================================================================================================

namespace
{
	/** Stacca il controller da ogni evento di presentazione: da qui in poi le aree le spegne solo lo stato. */
	void PlanOverlayDetachEvents(const FRTLockInPreviewBench& B)
	{
		B.TM->OnLockInCommitted.RemoveAll(B.PC);
		B.TM->OnResolvePlaybackFinished.RemoveAll(B.PC);
		B.TM->OnMatchEnded.RemoveAll(B.PC);
	}

	/** Accende a mano TUTTE le superfici di pianificazione, tratto di LOS e timeline compresi. */
	void PlanOverlayLightEverything(const FRTLockInPreviewBench& B)
	{
		const FRTCellId Origine = B.Mine->Cell;
		const FRTCellId Vicino = URTHexLibrary::Neighbors(Origine)[0];
		B.HexMap->SetPreviewReachableCells({ Origine, Vicino });
		B.HexMap->SetPreviewPath({ Origine, Vicino });
		B.HexMap->SetPreviewHitCells({ Vicino }, { Origine });
		B.HexMap->SetPreviewRangeCells({ Vicino });
		B.HexMap->SetPreviewAttack(Origine, Vicino, /*bValid=*/ true, /*bOriginPredicted=*/ false);
		B.HexMap->SetPreviewSightBlock(/*bBlocked=*/ true, Origine, Vicino);
		FRTPlanPreview Timeline;
		Timeline.Phases.AddDefaulted();
		B.HexMap->SetPlanPreview(Timeline);
	}

	FString PlanOverlayMeaningName(ERTOverlayMeaning Meaning)
	{
		return StaticEnum<ERTOverlayMeaning>()->GetNameStringByValue(static_cast<int64>(Meaning));
	}

	/** Le uscite della issue. Il Cleanup non ha una riga sua: precede il playback, e dal suo stato e' la stessa uscita. */
	enum class EPlanOverlayExit : uint8
	{
		CommitAndPlayback, Modal, KnockOut, MatchEnd, ExplicitReset
	};

	const TCHAR* PlanOverlayExitName(EPlanOverlayExit Exit)
	{
		switch (Exit)
		{
		case EPlanOverlayExit::CommitAndPlayback: return TEXT("commit, risoluzione, Cleanup e playback");
		case EPlanOverlayExit::Modal:             return TEXT("Modal");
		case EPlanOverlayExit::KnockOut:          return TEXT("KO dell'unita' selezionata");
		case EPlanOverlayExit::MatchEnd:          return TEXT("fine partita");
		case EPlanOverlayExit::ExplicitReset:     return TEXT("reset esplicito (nessuna unita')");
		}
		return TEXT("?");
	}
}

/**
 * **`#3511` — N tick a ingressi invariati danno un solo calcolo; ogni ingresso che cambia ne aggiunge UNO.**
 *
 * I quattro scenari della issue, uno dopo l'altro e ognuno riparte da un contatore letto subito prima:
 * dieci tick fermi → 0; la sola cella → +1; un'oscillazione fra due celle → un calcolo per cambio; la sola azione armata →
 * +1 (e la portata si accende, **senza** che nessuno chiami `RefreshPlanningOverlay`: la regola e' derivata dallo stato).
 *
 * 🔑 **Due contatori, perche' sono due domande.** Il cambio della sola cella fa salire `Computations` ma NON
 * `PlanComputations`: e' il criterio di `#711` — *«nessun produttore all'hover ricalcola il reachable set»* — e qui e' pinnato
 * sul ventaglio che resta lo stesso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanningOverlayComputesOnlyOnChangedInputsTest,
	"RefactorTactics.HexMatch.PlanningOverlayComputesOnlyWhenInputsChange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanningOverlayComputesOnlyOnChangedInputsTest::RunTest(const FString&)
{
	FRTLockInPreviewBench B;
	if (!MakeLockInPreviewBench(*this, B))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	const TArray<FRTCellId> Vicini = URTHexLibrary::Neighbors(B.Mine->Cell);

	// CONTROLLO POSITIVO — l'azione che accende una portata, scelta per DELTA come fa il banco di `#3510`: una senza
	// portata renderebbe «la sola azione armata aggiunge un calcolo» vera senza che la portata si veda.
	int32 Mirata = INDEX_NONE;
	for (int32 I = 0; I < B.Mine->NumAbilities() && Mirata == INDEX_NONE; ++I)
	{
		B.Mine->SelectAbility(I);
		B.PC->UpdatePlanningOverlay();
		if (B.Mine->SelectedAbilityIndex == I && B.HexMap->GetPreviewRangeCells().Num() > 0)
		{
			Mirata = I;
		}
		B.Mine->SelectAbility(INDEX_NONE);
		B.PC->UpdatePlanningOverlay();
	}
	if (!TestTrue(TEXT("controllo: c'e' un'azione che, armata, accende la portata"), Mirata != INDEX_NONE))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}
	TestEqual(TEXT("controllo: disarmata, la portata e' spenta"), B.HexMap->GetPreviewRangeCells().Num(), 0);
	TestTrue(TEXT("controllo: e il ventaglio del piano e' acceso"), B.HexMap->NumPreviewReachableCells() > 0);
	const int32 VentaglioDelPiano = B.HexMap->NumPreviewReachableCells();

	int32 C = 0;
	int32 P = 0;
	const auto Mark = [&]() { C = B.PC->GetPlanningOverlayComputations(); P = B.PC->GetPlanningOverlayPlanComputations(); };
	const auto DC = [&]() { return B.PC->GetPlanningOverlayComputations() - C; };
	const auto DP = [&]() { return B.PC->GetPlanningOverlayPlanComputations() - P; };

	// 1. DIECI TICK a ingressi invariati: nessun calcolo, di nessun tipo.
	Mark();
	for (int32 Tick = 0; Tick < 10; ++Tick) { B.PC->UpdatePlanningOverlay(); }
	TestEqual(TEXT("dieci tick a ingressi invariati non calcolano niente"), DC(), 0);
	TestEqual(TEXT("...ne' il piano"), DP(), 0);

	// 2. LA SOLA CELLA: +1 calcolo, e il ventaglio non si rifa'.
	//
	// 🔑 **Il ventaglio si prova con un SENTINELLA, non col contatore**: il contatore lo fa salire il codice sotto test, nello
	// stesso ramo che decide, e un produttore sull'hover che scrivesse il ventaglio da fuori non lo muoverebbe. Si sovrascrive
	// il ventaglio con UNA cella e si guarda che dopo il cambio ce ne sia ancora una: un ricalcolo la sostituirebbe con tutto
	// il ventaglio del piano (review di `#3511`).
	B.HexMap->SetPreviewReachableCells({ B.Mine->Cell });
	TestTrue(TEXT("controllo: il sentinella e' diverso dal ventaglio del piano"),
		B.HexMap->NumPreviewReachableCells() != VentaglioDelPiano);
	Mark();
	B.HexMap->SetHoveredCell(Vicini[0], /*bValid=*/ true);
	B.PC->UpdatePlanningOverlay();
	TestEqual(TEXT("la sola cella sotto il cursore aggiunge un calcolo"), DC(), 1);
	TestEqual(TEXT("e non rifa' ventaglio e portata (criterio di `#711`)"), DP(), 0);
	TestEqual(TEXT("il ventaglio e' ancora il sentinella: nessuno l'ha riscritto"), B.HexMap->NumPreviewReachableCells(), 1);
	for (int32 Tick = 0; Tick < 5; ++Tick) { B.PC->UpdatePlanningOverlay(); }
	TestEqual(TEXT("e i tick seguenti, con la cella ferma, non ne aggiungono"), DC(), 1);

	// 2b. IL SOLO PIANO ATTIVO: e' un ingresso, ma decide quale cella si seleziona e non il ventaglio.
	Mark();
	B.PC->ActiveLayer = 1;
	B.PC->UpdatePlanningOverlay();
	TestEqual(TEXT("il solo piano attivo aggiunge un calcolo"), DC(), 1);
	TestEqual(TEXT("ma non rifa' il piano"), DP(), 0);
	TestEqual(TEXT("e il ventaglio e' ancora il sentinella"), B.HexMap->NumPreviewReachableCells(), 1);
	B.PC->ActiveLayer = 0;
	B.PC->UpdatePlanningOverlay();
	B.PC->RefreshPlanningOverlay(B.Mine); // rimette il ventaglio vero prima dei passi che lo guardano
	TestEqual(TEXT("controllo: il rinfresco esplicito lo rimette"), B.HexMap->NumPreviewReachableCells(), VentaglioDelPiano);

	// 3. UN'OSCILLAZIONE fra due celle: tanti calcoli quanti sono i cambi.
	Mark();
	for (int32 Cambio = 0; Cambio < 6; ++Cambio)
	{
		B.HexMap->SetHoveredCell(Vicini[(Cambio + 1) % 2], /*bValid=*/ true);
		B.PC->UpdatePlanningOverlay();
	}
	TestEqual(TEXT("sei cambi di cella fra due celle danno sei calcoli"), DC(), 6);
	TestEqual(TEXT("e nessuno rifa' il ventaglio"), DP(), 0);

	// ...e posare DI NUOVO la cella in cui il cursore gia' sta non e' un cambio.
	Mark();
	B.HexMap->SetHoveredCell(Vicini[(5 + 1) % 2], /*bValid=*/ true);
	B.PC->UpdatePlanningOverlay();
	TestEqual(TEXT("la stessa cella riposata non aggiunge niente"), DC(), 0);

	// 4. LA SOLA VALIDITA' della cella.
	Mark();
	B.HexMap->SetHoveredCell(Vicini[(5 + 1) % 2], /*bValid=*/ false);
	B.PC->UpdatePlanningOverlay();
	TestEqual(TEXT("la sola validita' della cella e' un ingresso: +1"), DC(), 1);
	TestEqual(TEXT("e non e' il piano"), DP(), 0);

	// 5. LA SOLA AZIONE ARMATA: +1 e +1, e la portata si accende senza che nessuno chiami il refresh esplicito.
	Mark();
	B.Mine->SelectAbility(Mirata);
	B.PC->UpdatePlanningOverlay();
	TestEqual(TEXT("armare un'azione aggiunge un calcolo"), DC(), 1);
	TestEqual(TEXT("e questo rifa' il piano"), DP(), 1);
	TestTrue(TEXT("la portata si accende, dallo stato e non da un evento"), B.HexMap->GetPreviewRangeCells().Num() > 0);
	for (int32 Tick = 0; Tick < 5; ++Tick) { B.PC->UpdatePlanningOverlay(); }
	TestEqual(TEXT("e con l'azione ferma i tick non calcolano"), DC(), 1);

	Mark();
	B.Mine->SelectAbility(INDEX_NONE);
	B.PC->UpdatePlanningOverlay();
	TestEqual(TEXT("disarmarla ne aggiunge un altro"), DC(), 1);
	TestEqual(TEXT("e la portata si spegne"), B.HexMap->GetPreviewRangeCells().Num(), 0);

	// 6. IL RINFRESCO ESPLICITO: un waypoint aggiunto non cambia nessun altro campo, e la revisione del piano lo dice.
	Mark();
	B.PC->RefreshPlanningOverlay(B.Mine);
	TestEqual(TEXT("un rinfresco esplicito del piano e' sempre un calcolo"), DC(), 1);
	TestEqual(TEXT("e rifa' il piano"), DP(), 1);
	B.PC->UpdatePlanningOverlay();
	TestEqual(TEXT("il tick dopo trova gli ingressi uguali e non ne aggiunge"), DC(), 1);

	DestroyHexMatchWorld(B.World);
	return true;
}

/**
 * **`#3511` — dopo OGNI uscita nessuna area di pianificazione resta accesa, per ogni significato di `AllMeanings`.**
 *
 * Le uscite della issue: commit/risoluzione/Cleanup/playback, `Modal`, KO dell'unita' selezionata, fine partita, piu' il reset
 * esplicito. Per ciascuna: banco nuovo, controller staccato dagli eventi, **tutte** le superfici accese a mano (controllo
 * positivo, significato per significato), l'uscita applicata SOLO come stato, un `UpdatePlanningOverlay`, e tutte le voci di
 * `AllMeanings` a zero — oltre al tratto di LOS e alla timeline, che non sono significati.
 *
 * ⚠️ **Accenderle a mano e' la prova**: le superfici che un piano vero accende dipendono dal banco (un alleato nell'area, un
 * bersaglio coperto), e un reset provato solo sulle tre che il banco accende sarebbe cieco alle altre — il difetto di
 * `PreviewCellsLit`, che non contava la portata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanningOverlayEveryExitTurnsEveryMeaningOffTest,
	"RefactorTactics.HexMatch.PlanningOverlayEveryExitTurnsEveryMeaningOff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanningOverlayEveryExitTurnsEveryMeaningOffTest::RunTest(const FString&)
{
	const EPlanOverlayExit Exits[] = {
		EPlanOverlayExit::CommitAndPlayback, EPlanOverlayExit::Modal, EPlanOverlayExit::KnockOut,
		EPlanOverlayExit::MatchEnd, EPlanOverlayExit::ExplicitReset,
	};

	for (const EPlanOverlayExit Exit : Exits)
	{
		const FString Uscita = PlanOverlayExitName(Exit);
		FRTLockInPreviewBench B;
		if (!MakeLockInPreviewBench(*this, B))
		{
			DestroyHexMatchWorld(B.World);
			return false;
		}
		PlanOverlayDetachEvents(B);
		B.PC->UpdatePlanningOverlay(); // assesta gli ingressi: da qui l'uscita e' l'UNICA cosa che cambia
		PlanOverlayLightEverything(B);

		// CONTROLLO POSITIVO — prima dell'uscita ogni significato di pianificazione e' acceso.
		for (const ERTOverlayMeaning Meaning : URTOverlayPalette::AllMeanings())
		{
			if (Meaning == ERTOverlayMeaning::Hover) { continue; }
			TestTrue(*FString::Printf(TEXT("[%s] controllo: %s e' acceso prima dell'uscita"),
				*Uscita, *PlanOverlayMeaningName(Meaning)), B.HexMap->NumPlanningPreviewCells(Meaning) > 0);
		}

		UGameInstance* GameInstance = nullptr;
		URTFrontendNavigator* Navigator = nullptr;
		bool bUscitaApplicata = true;
		switch (Exit)
		{
		case EPlanOverlayExit::CommitAndPlayback:
			CommitFromTimeout(B);
			bUscitaApplicata = TestTrue(*FString::Printf(TEXT("[%s] la risoluzione e' in corso"), *Uscita), B.TM->IsResolving())
				&& TestEqual(*FString::Printf(TEXT("[%s] e il contesto e' ResolutionPlayback"), *Uscita),
					B.PC->GetPointerContext(), ERTPointerContext::ResolutionPlayback);
			break;

		case EPlanOverlayExit::Modal:
			GameInstance = NewObject<UGameInstance>(GetTransientPackage());
			GameInstance->AddToRoot();
			GameInstance->Init();
			Navigator = GameInstance->GetSubsystem<URTFrontendNavigator>();
			bUscitaApplicata = TestNotNull(*FString::Printf(TEXT("[%s] il navigatore esiste"), *Uscita), Navigator);
			if (bUscitaApplicata)
			{
				B.World->SetGameInstance(GameInstance);
				Navigator->EnterMatch();
				Navigator->ShowPause();
				bUscitaApplicata = TestEqual(*FString::Printf(TEXT("[%s] il contesto e' Modal"), *Uscita),
					B.PC->GetPointerContext(), ERTPointerContext::Modal);
			}
			break;

		case EPlanOverlayExit::KnockOut:
			B.Mine->Health = 0;
			bUscitaApplicata = TestFalse(*FString::Printf(TEXT("[%s] l'unita' selezionata e' KO"), *Uscita), B.Mine->IsAlive());
			break;

		case EPlanOverlayExit::MatchEnd:
		{
			ARTUnit* Avversaria = RTWorldFixtures::FirstUnitOfTeam(B.World, /*TeamId=*/ 1);
			bUscitaApplicata = TestNotNull(*FString::Printf(TEXT("[%s] c'e' un'avversaria da eliminare"), *Uscita), Avversaria);
			if (bUscitaApplicata)
			{
				Avversaria->Health = 0;
				CommitFromTimeout(B);
				DrainPlayback(B.TM);
				bUscitaApplicata = TestEqual(*FString::Printf(TEXT("[%s] la partita e' finita"), *Uscita),
					B.TM->GetPhase(), ERTMatchPhase::MatchEnded);
				// ANTI-VACUITA' — il puntatore NON dichiara la fine partita: se il contesto fosse gia' di sola
				// lettura, a spegnere le aree sarebbe lui e non `bMatchEnded`, e questa riga non proverebbe niente.
				TestTrue(*FString::Printf(TEXT("[%s] il contesto resta di pianificazione: la dichiara solo bMatchEnded"), *Uscita),
					B.PC->GetPointerContext() == ERTPointerContext::Planning
						|| B.PC->GetPointerContext() == ERTPointerContext::Pathing);
			}
			break;
		}

		case EPlanOverlayExit::ExplicitReset:
			B.PC->RefreshPlanningOverlay(nullptr);
			break;
		}

		if (bUscitaApplicata)
		{
			// ⚠️ **Il reset esplicito si guarda SUBITO, senza tick**: lo stato del banco (unita' viva e selezionata, in
			// pianificazione) dice che le aree si mostrano, quindi un tick le riaccenderebbe a ragione. I due chiamanti
			// reali di `nullptr` — commit e fine partita — sono in uno stato che dice il contrario, e hanno la loro riga.
			if (Exit != EPlanOverlayExit::ExplicitReset)
			{
				B.PC->UpdatePlanningOverlay();
			}

			for (const ERTOverlayMeaning Meaning : URTOverlayPalette::AllMeanings())
			{
				TestEqual(*FString::Printf(TEXT("[%s] dopo l'uscita %s e' spento"), *Uscita, *PlanOverlayMeaningName(Meaning)),
					B.HexMap->NumPlanningPreviewCells(Meaning), 0);
			}
			TestFalse(*FString::Printf(TEXT("[%s] ne' resta il tratto di LOS (`#3064`)"), *Uscita), B.HexMap->HasPreviewSightBlock());
			TestEqual(*FString::Printf(TEXT("[%s] ne' la timeline dei ghost"), *Uscita), B.HexMap->GetPlanPreview().Phases.Num(), 0);
		}

		if (GameInstance)
		{
			GameInstance->Shutdown();
			GameInstance->RemoveFromRoot();
		}
		DestroyHexMatchWorld(B.World);
	}
	return true;
}

/**
 * **`#3511` — a playback finito le aree tornano DALLO STATO, e il tick dopo non le ricalcola.**
 *
 * Fino a `#3511` le riaccendeva `HandlePlaybackFinished`, cioe' un evento (`#2555`). Ora e' la stessa funzione che le spegne:
 * stato di pianificazione → aree. Prima con il controller STACCATO dagli eventi, per provare che basta lo stato; poi con gli
 * eventi attaccati, come in partita, per provare che i due percorsi concordano sulle aree.
 *
 * ⚠️ **Non si escludono del tutto a vicenda**, e il test lo dice invece di tacerlo: `OnResolvePlaybackFinished` scatta PRIMA che
 * `ConcludeTurn` faccia salire il numero del turno (`RTTurnManager.cpp`, la lettura della review di `#3511`), quindi il primo
 * tick dopo l'evento puo' vedere un indice nuovo e ricalcolare UNA volta. Il test pone il tetto a uno e ne registra il valore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanningOverlayReturnsWithTheStateAfterPlaybackTest,
	"RefactorTactics.HexMatch.PlanningOverlayReturnsWithTheStateAfterPlayback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanningOverlayReturnsWithTheStateAfterPlaybackTest::RunTest(const FString&)
{
	for (const bool bEventiAttaccati : { false, true })
	{
		const FString Quando = bEventiAttaccati ? TEXT("eventi attaccati") : TEXT("solo stato");
		FRTLockInPreviewBench B;
		if (!MakeLockInPreviewBench(*this, B))
		{
			DestroyHexMatchWorld(B.World);
			return false;
		}
		if (!bEventiAttaccati) { PlanOverlayDetachEvents(B); }
		B.PC->UpdatePlanningOverlay();
		TestTrue(*FString::Printf(TEXT("[%s] controllo: in pianificazione le aree sono accese"), *Quando),
			PreviewCellsLit(B.HexMap) > 0);

		CommitFromTimeout(B);
		B.PC->UpdatePlanningOverlay();
		TestTrue(*FString::Printf(TEXT("[%s] la risoluzione e' in corso"), *Quando), B.TM->IsResolving());
		TestEqual(*FString::Printf(TEXT("[%s] e durante il playback il tick le tiene spente"), *Quando),
			PreviewCellsLit(B.HexMap), 0);

		DrainPlayback(B.TM);
		TestFalse(*FString::Printf(TEXT("[%s] il playback e' finito"), *Quando), B.TM->IsResolving());
		const int32 PrimaDelTick = B.PC->GetPlanningOverlayComputations();
		B.PC->UpdatePlanningOverlay();
		const int32 DelPrimoTick = B.PC->GetPlanningOverlayComputations() - PrimaDelTick;
		AddInfo(FString::Printf(TEXT("[%s] calcoli del primo tick dopo il playback: %d"), *Quando, DelPrimoTick));
		TestTrue(*FString::Printf(TEXT("[%s] il primo tick dopo il playback calcola al piu' una volta"), *Quando), DelPrimoTick <= 1);
		TestTrue(*FString::Printf(TEXT("[%s] a playback finito il tick le riaccende dallo stato"), *Quando),
			PreviewCellsLit(B.HexMap) > 0);

		const int32 Calcoli = B.PC->GetPlanningOverlayComputations();
		for (int32 Tick = 0; Tick < 5; ++Tick) { B.PC->UpdatePlanningOverlay(); }
		TestEqual(*FString::Printf(TEXT("[%s] e i tick seguenti, a ingressi invariati, non calcolano"), *Quando),
			B.PC->GetPlanningOverlayComputations(), Calcoli);

		DestroyHexMatchWorld(B.World);
	}
	return true;
}

/**
 * **`#3511` — il confronto lo fa `PlayerTick`, a ogni fotogramma: nessuno chiama `UpdatePlanningOverlay` a mano.**
 *
 * Gli altri test chiamano la funzione direttamente, come la issue chiede; questo prova il cablaggio. Senza, la riga di
 * `PlayerTick` potrebbe sparire e restare verdi tutti: la funzione esisterebbe, testata, e in partita non la chiamerebbe
 * nessuno — il difetto di una feature senza chiamante.
 *
 * L'azione si arma sull'UNITA' (`SelectAbility`), non dal controller: il controller la accenderebbe con un rinfresco
 * esplicito, e il test non distinguerebbe il tick da quello.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanningOverlayIsDrivenByPlayerTickTest,
	"RefactorTactics.HexMatch.PlanningOverlayIsDrivenByPlayerTick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanningOverlayIsDrivenByPlayerTickTest::RunTest(const FString&)
{
	FRTLockInPreviewBench B;
	if (!MakeLockInPreviewBench(*this, B))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	// CONTROLLO POSITIVO — l'azione che accende la portata, scelta per DELTA (vedi il test dei conteggi).
	int32 Mirata = INDEX_NONE;
	for (int32 I = 0; I < B.Mine->NumAbilities() && Mirata == INDEX_NONE; ++I)
	{
		B.Mine->SelectAbility(I);
		B.PC->UpdatePlanningOverlay();
		if (B.Mine->SelectedAbilityIndex == I && B.HexMap->GetPreviewRangeCells().Num() > 0)
		{
			Mirata = I;
		}
		B.Mine->SelectAbility(INDEX_NONE);
		B.PC->UpdatePlanningOverlay();
	}
	if (!TestTrue(TEXT("controllo: c'e' un'azione che, armata, accende la portata"), Mirata != INDEX_NONE))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	// `APlayerController::PlayerTick` pretende un `PlayerInput`, che un controller spawnato a mano non ha.
	B.PC->InitInputSystem();

	B.Mine->SelectAbility(Mirata);
	TestEqual(TEXT("armata sull'unita' senza tick, la portata non c'e' ancora"), B.HexMap->GetPreviewRangeCells().Num(), 0);

	B.PC->PlayerTickForTest(0.016f);
	TestTrue(TEXT("un fotogramma di PlayerTick accende la portata"), B.HexMap->GetPreviewRangeCells().Num() > 0);

	const int32 Calcoli = B.PC->GetPlanningOverlayComputations();
	for (int32 Tick = 0; Tick < 5; ++Tick) { B.PC->PlayerTickForTest(0.016f); }
	TestEqual(TEXT("e i fotogrammi seguenti, a ingressi invariati, non calcolano"),
		B.PC->GetPlanningOverlayComputations(), Calcoli);

	B.Mine->SelectAbility(INDEX_NONE);
	B.PC->PlayerTickForTest(0.016f);
	TestEqual(TEXT("disarmata, il fotogramma dopo spegne la portata"), B.HexMap->GetPreviewRangeCells().Num(), 0);

	DestroyHexMatchWorld(B.World);
	return true;
}

/**
 * **`#3511` — l'ULTIMO turno non riaccende le aree dietro la schermata dei risultati, senza aspettare il tick.**
 *
 * E' il difetto di `#2555`: `OnResolvePlaybackFinished` scatta prima che `ConcludeTurn` imposti `MatchEnded`, quindi
 * `HandlePlaybackFinished` ridipinge, e a chiudere resta `HandleMatchEndedPresentation`. Qui gli eventi sono ATTACCATI e il
 * tick non gira: e' il percorso per eventi, che l'ingresso unico non sostituisce ma completa. Dopo, un tick lo conferma.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanningOverlayMatchEndedPresentationTest,
	"RefactorTactics.HexMatch.PlanningOverlayMatchEndedLeavesNothingLit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanningOverlayMatchEndedPresentationTest::RunTest(const FString&)
{
	FRTLockInPreviewBench B;
	if (!MakeLockInPreviewBench(*this, B))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}
	ARTUnit* Avversaria = RTWorldFixtures::FirstUnitOfTeam(B.World, /*TeamId=*/ 1);
	if (!TestNotNull(TEXT("c'e' un'avversaria da eliminare"), Avversaria))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}
	TestTrue(TEXT("controllo: in pianificazione le aree sono accese"), PreviewCellsLit(B.HexMap) > 0);

	Avversaria->Health = 0;
	CommitFromTimeout(B);
	DrainPlayback(B.TM);
	if (!TestEqual(TEXT("la partita e' finita"), B.TM->GetPhase(), ERTMatchPhase::MatchEnded))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	TestEqual(TEXT("a partita finita nessuna area e' accesa, per evento e senza tick"), PreviewCellsLit(B.HexMap), 0);
	B.PC->UpdatePlanningOverlay();
	TestEqual(TEXT("e un tick non le riaccende"), PreviewCellsLit(B.HexMap), 0);

	DestroyHexMatchWorld(B.World);
	return true;
}

/**
 * **`#3511` — un controller NUOVO, N tick a ingressi invariati: un solo calcolo.**
 *
 * E' la frase della DoD alla lettera. Gli altri test misurano delta dopo l'allestimento del banco, che ha gia' calcolato piu'
 * volte; qui il contatore parte da zero. Due controller: senza selezione (il primo tick calcola che non c'e' niente da
 * mostrare, e lo spegne) e con una selezione (la selezione e' un rinfresco esplicito, che calcola il piano).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanningOverlayFreshControllerComputesOnceTest,
	"RefactorTactics.HexMatch.PlanningOverlayFreshControllerComputesOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanningOverlayFreshControllerComputesOnceTest::RunTest(const FString&)
{
	FRTLockInPreviewBench B;
	if (!MakeLockInPreviewBench(*this, B))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	ARTPlayerController* SenzaSelezione = RTWorldFixtures::MakePlayerOnTeam(B.World, /*TeamId=*/ 0);
	ARTPlayerController* ConSelezione = RTWorldFixtures::MakePlayerOnTeam(B.World, /*TeamId=*/ 0);
	if (!TestNotNull(TEXT("primo controller nuovo"), SenzaSelezione) || !TestNotNull(TEXT("secondo controller nuovo"), ConSelezione))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}
	TestEqual(TEXT("controllo: un controller nuovo parte da zero calcoli"), SenzaSelezione->GetPlanningOverlayComputations(), 0);

	for (int32 Tick = 0; Tick < 10; ++Tick) { SenzaSelezione->UpdatePlanningOverlay(); }
	TestEqual(TEXT("senza selezione, dieci tick danno un solo calcolo"), SenzaSelezione->GetPlanningOverlayComputations(), 1);
	TestEqual(TEXT("e non rifanno nessun piano: non c'e' niente da mostrare"), SenzaSelezione->GetPlanningOverlayPlanComputations(), 0);

	ConSelezione->SelectUnit(B.Mine, /*bRecordAsPlayerInput=*/ false);
	const int32 DopoLaSelezione = ConSelezione->GetPlanningOverlayComputations();
	TestEqual(TEXT("selezionare un'unita' e' un calcolo"), DopoLaSelezione, 1);
	TestEqual(TEXT("...che rifa' il piano"), ConSelezione->GetPlanningOverlayPlanComputations(), 1);
	for (int32 Tick = 0; Tick < 10; ++Tick) { ConSelezione->UpdatePlanningOverlay(); }
	TestEqual(TEXT("e dieci tick a ingressi invariati non ne aggiungono"), ConSelezione->GetPlanningOverlayComputations(), DopoLaSelezione);

	DestroyHexMatchWorld(B.World);
	return true;
}

/**
 * **`#3511` — la pausa spegne le aree e il RESUME le restituisce com'erano, rotta compresa.**
 *
 * Il difetto che la review ha trovato: `ClearPlanningPreview` azzera anche la rotta, e il ritorno dalla pausa rifaceva ventaglio,
 * portata e timeline ma non la rotta, che solo i siti degli eventi scrivevano. Il confronto e' per OGNI significato, prima e dopo:
 * «torna qualcosa» (`PreviewCellsLit > 0`) sarebbe verde con la rotta ancora spenta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanningOverlayComesBackFromModalAsItWasTest,
	"RefactorTactics.HexMatch.PlanningOverlayComesBackFromModalAsItWas",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanningOverlayComesBackFromModalAsItWasTest::RunTest(const FString&)
{
	FRTLockInPreviewBench B;
	if (!MakeLockInPreviewBench(*this, B))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}
	B.PC->UpdatePlanningOverlay();

	TArray<int32> Prima;
	for (const ERTOverlayMeaning Meaning : URTOverlayPalette::AllMeanings())
	{
		Prima.Add(B.HexMap->NumPlanningPreviewCells(Meaning));
	}
	TestTrue(TEXT("controllo: prima della pausa c'e' una rotta tracciata"), B.HexMap->NumPreviewPathCells() > 0);
	TestTrue(TEXT("controllo: e il piano ha dei waypoint"), B.Mine->PlannedWaypoints.Num() > 0);

	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	GameInstance->AddToRoot();
	GameInstance->Init();
	URTFrontendNavigator* Navigator = GameInstance->GetSubsystem<URTFrontendNavigator>();
	if (TestNotNull(TEXT("il navigatore esiste"), Navigator))
	{
		B.World->SetGameInstance(GameInstance);
		Navigator->EnterMatch();
		Navigator->ShowPause();
		B.PC->UpdatePlanningOverlay();
		TestEqual(TEXT("con la pausa aperta nessuna area e' accesa"), PreviewCellsLit(B.HexMap), 0);

		Navigator->ResumeMatch();
		B.PC->UpdatePlanningOverlay();
		int32 Indice = 0;
		for (const ERTOverlayMeaning Meaning : URTOverlayPalette::AllMeanings())
		{
			TestEqual(*FString::Printf(TEXT("dopo il RESUME %s e' com'era prima della pausa"), *PlanOverlayMeaningName(Meaning)),
				B.HexMap->NumPlanningPreviewCells(Meaning), Prima[Indice++]);
		}
	}

	GameInstance->Shutdown();
	GameInstance->RemoveFromRoot();
	DestroyHexMatchWorld(B.World);
	return true;
}

/**
 * **`#3891` — una reazione appena armata si vede nella timeline del piano, senza aspettare un altro rinfresco.**
 *
 * Il ramo della reazione di `SelectAbilityForCurrent` rinfrescava l'anteprima PRIMA di scrivere `PlannedReactionAbility`, e la
 * timeline la legge: `bReactionArmed` e la voce di Prep nascevano dal piano di prima. Dal `#3511` il tick non lo rimedia, perche'
 * ricalcola solo se un ingresso cambia e il campo non e' un ingresso.
 *
 * 🔑 **Dieci tick prima di guardare**, perche' in partita il fotogramma dopo l'armo c'e' sempre: un test che guardasse subito
 * non direbbe se il difetto dura. 🔑 **E un controllo positivo**: dopo un rinfresco esplicito la timeline VEDE la reazione nello
 * stesso banco, cosi' un rosso e' attribuibile all'ordine e non a una timeline che il banco non costruisce.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTArmedReactionShowsInThePlanTimelineTest,
	"RefactorTactics.HexMatch.ArmedReactionShowsInThePlanTimeline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTArmedReactionShowsInThePlanTimelineTest::RunTest(const FString&)
{
	FRTLockInPreviewBench B;
	if (!MakeLockInPreviewBench(*this, B))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	// `AddCoreAbilityInSlot` sostituisce invece di accodare: un'abilita' in coda sta a un indice che `AbilityCooldowns` non copre.
	const int32 Reazione = RTAbilityFixtures::AddCoreAbilityInSlot(B.Mine, TEXT("Action.Counter"), 3);
	const URTActionData* Counter = B.Mine->GetAbility(Reazione);
	if (!TestTrue(TEXT("premessa: la reazione e' entrata nel kit"), Reazione != INDEX_NONE && Counter != nullptr)
		|| !TestEqual(TEXT("premessa: e' davvero uno slot di reazione"),
			static_cast<int32>(Counter->Def.Slot), static_cast<int32>(ERTActionSlot::Reaction))
		|| !TestTrue(TEXT("premessa: la reazione e' pronta"), B.Mine->CanUseAbility(Reazione)))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	// La timeline esiste in questo banco (il piano ha un movimento) e prima dell'armo non porta nessuna reazione.
	B.PC->RefreshPlanningOverlay(B.Mine);
	TestTrue(TEXT("premessa: il banco costruisce la timeline del piano"), B.HexMap->GetPlanPreview().Phases.Num() > 0);
	TestFalse(TEXT("premessa: prima dell'armo la timeline non porta una reazione"), B.HexMap->GetPlanPreview().Reaction.bArmed);

	B.PC->SelectAbilityForCurrentForTest(Reazione);
	if (!TestEqual(TEXT("premessa: l'armo e' arrivato al piano"), B.Mine->PlannedReactionAbility, Reazione))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}
	for (int32 Tick = 0; Tick < 10; ++Tick) { B.PC->UpdatePlanningOverlay(); }

	TestTrue(TEXT("dopo l'armo e dieci tick la timeline porta la reazione"), B.HexMap->GetPlanPreview().Reaction.bArmed);

	// Controllo positivo: un rinfresco esplicito la mostra, quindi il banco sa disegnarla.
	B.PC->RefreshPlanningOverlay(B.Mine);
	TestTrue(TEXT("controllo positivo: dopo un rinfresco esplicito la timeline porta la reazione"),
		B.HexMap->GetPlanPreview().Reaction.bArmed);

	DestroyHexMatchWorld(B.World);
	return true;
}

// =====================================================================================================
// `#3896` — disarmare lo slot di una REAZIONE la toglie dal piano (decisione d'autore).
// =====================================================================================================

/**
 * 🔴 **DISARMARE LO SLOT DI UNA REAZIONE LA TOGLIE DAL PIANO.**
 *
 * Il caso misurato da `#3896`: il secondo click sullo slot armato spegneva `SelectedAbilityIndex` e lasciava
 * `PlannedReactionAbility` scritto. Il dock diceva «non armata», il resolver — che legge il piano — e la timeline
 * — che legge `PlannedReactionAbility` — la usavano ancora: due letture dello stesso modello che si contraddicono.
 *
 * 🔑 **Si arma e si disarma dalla porta del dock (`ArmKitAbility`)**, la stessa del click, e si leggono TRE oracoli
 * indipendenti — lo slot, il piano e la timeline — piu' la condizione dichiarata, che vive e muore con la reazione.
 * Nessuno di loro e' derivato dagli altri, quindi un verde dice che il modello e' coerente e non che due campi
 * sono stati azzerati insieme.
 *
 * ⚠️ **Le premesse sono dentro il test e fanno da controllo positivo**: l'armo dal dock scrive piano e timeline, la
 * condizione entra davvero, e DOPO il disarmo il riarmo funziona. Senza, «tutto neutro» sarebbe soddisfatto anche da
 * un dock inerte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDisarmedReactionLeavesThePlanTest,
	"RefactorTactics.HexMatch.DisarmedReactionLeavesThePlanSlotAndTimeline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDisarmedReactionLeavesThePlanTest::RunTest(const FString&)
{
	FRTLockInPreviewBench B;
	if (!MakeLockInPreviewBench(*this, B))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	const int32 Reazione = RTAbilityFixtures::AddCoreAbilityInSlot(B.Mine, TEXT("Action.Counter"), 3);
	const URTActionData* Counter = B.Mine->GetAbility(Reazione);
	if (!TestTrue(TEXT("premessa: la reazione e' entrata nel kit"), Reazione != INDEX_NONE && Counter != nullptr)
		|| !TestEqual(TEXT("premessa: e' davvero uno slot di reazione"),
			static_cast<int32>(Counter->Def.Slot), static_cast<int32>(ERTActionSlot::Reaction))
		|| !TestTrue(TEXT("premessa: la reazione e' pronta"), B.Mine->CanUseAbility(Reazione)))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	// --- premesse: l'armo dal dock scrive piano, slot, condizione e timeline -------------------------
	B.PC->ArmKitAbility(Reazione);
	// La condizione si dichiara DOPO l'armo: `SetPlannedReactionCondition` rifiuta se non c'e' una reazione nel piano.
	// L'id e' quello che il validator ammette (`URTReactionOpportunityLibrary::TargetHealthAtOrBelowPercent`).
	const bool bCondizione = B.Mine->SetPlannedReactionCondition(
		FRTDeclaredCondition(TEXT("TargetHealthAtOrBelowPercent"), 50));
	B.PC->RefreshPlanningOverlay(B.Mine);
	if (!TestEqual(TEXT("premessa: il dock accende lo slot"), B.Mine->SelectedAbilityIndex, Reazione)
		|| !TestEqual(TEXT("premessa: e il piano porta la reazione"), B.Mine->PlannedReactionAbility, Reazione)
		|| !TestTrue(TEXT("premessa: la condizione dichiarata entra nel piano"),
			bCondizione && B.Mine->PlannedReactionCondition.IsDeclared())
		|| !TestTrue(TEXT("premessa: la timeline la porta armata"), B.HexMap->GetPlanPreview().Reaction.bArmed))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	// --- il secondo click sullo stesso slot DISARMA -------------------------------------------------
	B.PC->ArmKitAbility(Reazione);
	TestEqual(TEXT("lo slot si spegne"), B.Mine->SelectedAbilityIndex, INDEX_NONE);
	TestEqual(TEXT("e la reazione esce dal piano: il resolver non la trova piu'"),
		B.Mine->PlannedReactionAbility, INDEX_NONE);
	TestFalse(TEXT("con lei esce la condizione dichiarata"), B.Mine->PlannedReactionCondition.IsDeclared());
	for (int32 Tick = 0; Tick < 10; ++Tick) { B.PC->UpdatePlanningOverlay(); }
	TestFalse(TEXT("e la timeline non la disegna piu' armata, nemmeno dopo dieci tick"),
		B.HexMap->GetPlanPreview().Reaction.bArmed);

	// --- controllo positivo: il disarmo non ha rotto l'armo -----------------------------------------
	B.PC->ArmKitAbility(Reazione);
	TestEqual(TEXT("dopo il disarmo la stessa reazione si riarma"), B.Mine->PlannedReactionAbility, Reazione);
	TestTrue(TEXT("e la timeline la ridisegna armata"), B.HexMap->GetPlanPreview().Reaction.bArmed);

	DestroyHexMatchWorld(B.World);
	return true;
}

/**
 * 🔑 **IL CONTROLLO DELL'ALTRO VERSO: disarmare un'azione PRINCIPALE non toglie una reazione INDIPENDENTE.**
 *
 * `Reaction` ha il proprio slot e il proprio campo (`PlannedReactionAbility`), distinto da `PlannedAbilityIndex`.
 * Armare una reazione e poi un'azione principale lascia `SelectedAbilityIndex` sulla seconda e la reazione nel
 * piano: sono due cose che il giocatore ha chiesto separatamente. La correzione piu' ovvia di `#3896` — azzerare la
 * reazione a OGNI disarmo — passerebbe il test qui sopra e romperebbe questo.
 *
 * ⛔ **La reazione si toglie solo se e' LEI lo slot che si disarma.**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDisarmingAMainActionKeepsAnIndependentReactionTest,
	"RefactorTactics.HexMatch.DisarmingAMainActionKeepsAnIndependentReaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDisarmingAMainActionKeepsAnIndependentReactionTest::RunTest(const FString&)
{
	FRTLockInPreviewBench B;
	if (!MakeLockInPreviewBench(*this, B))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	const int32 Reazione = RTAbilityFixtures::AddCoreAbilityInSlot(B.Mine, TEXT("Action.Counter"), 3);
	// `Action.Guard`: un'azione PRINCIPALE su se stessi, che entra nel piano all'armamento (`PlannedAbilityIndex`).
	const int32 Guardia = RTAbilityFixtures::AddCoreAbilityInSlot(B.Mine, TEXT("Action.Guard"), 1);
	const URTActionData* Counter = B.Mine->GetAbility(Reazione);
	const URTActionData* Guard = B.Mine->GetAbility(Guardia);
	if (!TestTrue(TEXT("premessa: reazione e guardia sono entrate nel kit"),
			Reazione != INDEX_NONE && Guardia != INDEX_NONE && Reazione != Guardia && Counter && Guard)
		|| !TestEqual(TEXT("premessa: la prima e' una reazione"),
			static_cast<int32>(Counter->Def.Slot), static_cast<int32>(ERTActionSlot::Reaction))
		|| !TestTrue(TEXT("premessa: la seconda NON e' una reazione"), Guard->Def.Slot != ERTActionSlot::Reaction)
		|| !TestTrue(TEXT("premessa: sono pronte entrambe"),
			B.Mine->CanUseAbility(Reazione) && B.Mine->CanUseAbility(Guardia)))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	B.PC->ArmKitAbility(Reazione);
	B.PC->ArmKitAbility(Guardia);
	B.PC->RefreshPlanningOverlay(B.Mine);
	if (!TestEqual(TEXT("premessa: lo slot acceso e' la guardia"), B.Mine->SelectedAbilityIndex, Guardia)
		|| !TestEqual(TEXT("premessa: la guardia e' nel piano"), B.Mine->PlannedAbilityIndex, Guardia)
		|| !TestEqual(TEXT("premessa: la reazione e' ANCORA nel piano, indipendente"),
			B.Mine->PlannedReactionAbility, Reazione)
		|| !TestTrue(TEXT("premessa: la timeline porta la reazione"), B.HexMap->GetPlanPreview().Reaction.bArmed))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	// Si disarma la PRINCIPALE, dalla stessa porta.
	B.PC->ArmKitAbility(Guardia);
	TestEqual(TEXT("la guardia si spegne"), B.Mine->SelectedAbilityIndex, INDEX_NONE);
	TestEqual(TEXT("e esce dal piano"), B.Mine->PlannedAbilityIndex, INDEX_NONE);
	TestEqual(TEXT("la reazione indipendente RESTA nel piano"), B.Mine->PlannedReactionAbility, Reazione);
	for (int32 Tick = 0; Tick < 10; ++Tick) { B.PC->UpdatePlanningOverlay(); }
	TestTrue(TEXT("e la timeline continua a disegnarla armata"), B.HexMap->GetPlanPreview().Reaction.bArmed);

	DestroyHexMatchWorld(B.World);
	return true;
}

/**
 * 🔑 **L'ALTRO RAMO DELLA STESSA INDIPENDENZA: un attacco armato in TARGETING non toglie la reazione** (`#3896`, dalla
 * review).
 *
 * `Action.Guard` entra in `PlannedAbilityIndex` appena armata; un attacco a bersaglio no: resta armato ma NON nel piano
 * finche' non si clicca il bersaglio. Il test sopra non vede questo caso, e due predicati sbagliati lo lasciano passare —
 * «c'e' una reazione e nessuna principale nel piano», «lo slot acceso non e' la principale nel piano» — perche' con la
 * guardia nel piano valgono falso e con la reazione sola valgono vero, esattamente come il predicato giusto.
 *
 * Qui la reazione e' armata, poi un attacco: `PlannedAbilityIndex` resta neutro e `SelectedAbilityIndex` e' l'attacco.
 * Disarmare l'attacco NON deve toccare la reazione. Il predicato giusto e' `SelectedAbilityIndex == PlannedReactionAbility`,
 * e solo questo caso lo separa dagli altri due.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDisarmingATargetingAttackKeepsAnIndependentReactionTest,
	"RefactorTactics.HexMatch.DisarmingATargetingAttackKeepsAnIndependentReaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDisarmingATargetingAttackKeepsAnIndependentReactionTest::RunTest(const FString&)
{
	FRTLockInPreviewBench B;
	if (!MakeLockInPreviewBench(*this, B))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	const int32 Reazione = RTAbilityFixtures::AddCoreAbilityInSlot(B.Mine, TEXT("Action.Counter"), 3);
	const int32 Attacco = RTAbilityFixtures::AddCoreAbilityInSlot(B.Mine, TEXT("Action.BasicAttack"), 1);
	const URTActionData* Counter = B.Mine->GetAbility(Reazione);
	const URTActionData* Basic = B.Mine->GetAbility(Attacco);
	if (!TestTrue(TEXT("premessa: reazione e attacco sono entrati nel kit"),
			Reazione != INDEX_NONE && Attacco != INDEX_NONE && Reazione != Attacco && Counter && Basic)
		|| !TestEqual(TEXT("premessa: la prima e' una reazione"),
			static_cast<int32>(Counter->Def.Slot), static_cast<int32>(ERTActionSlot::Reaction))
		|| !TestTrue(TEXT("premessa: il secondo NON e' una reazione"), Basic->Def.Slot != ERTActionSlot::Reaction)
		|| !TestTrue(TEXT("premessa: sono pronte entrambe"),
			B.Mine->CanUseAbility(Reazione) && B.Mine->CanUseAbility(Attacco)))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	B.PC->ArmKitAbility(Reazione);
	B.PC->ArmKitAbility(Attacco);
	B.PC->RefreshPlanningOverlay(B.Mine);
	// Il cuore del caso: l'attacco e' ARMATO ma non e' nel piano. Se la premessa cadesse, il test non separerebbe i predicati.
	if (!TestEqual(TEXT("premessa: lo slot acceso e' l'attacco"), B.Mine->SelectedAbilityIndex, Attacco)
		|| !TestEqual(TEXT("premessa: l'attacco e' in targeting, NON nel piano"), B.Mine->PlannedAbilityIndex, INDEX_NONE)
		|| !TestEqual(TEXT("premessa: la reazione e' nel piano, indipendente"), B.Mine->PlannedReactionAbility, Reazione)
		|| !TestTrue(TEXT("premessa: la timeline porta la reazione"), B.HexMap->GetPlanPreview().Reaction.bArmed))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	B.PC->ArmKitAbility(Attacco);
	TestEqual(TEXT("l'attacco si spegne"), B.Mine->SelectedAbilityIndex, INDEX_NONE);
	TestEqual(TEXT("la reazione indipendente RESTA nel piano"), B.Mine->PlannedReactionAbility, Reazione);
	for (int32 Tick = 0; Tick < 10; ++Tick) { B.PC->UpdatePlanningOverlay(); }
	TestTrue(TEXT("e la timeline continua a disegnarla armata"), B.HexMap->GetPlanPreview().Reaction.bArmed);

	DestroyHexMatchWorld(B.World);
	return true;
}

// =====================================================================================================
// `#3892` — la ROTTA si scrive da UN punto: `RefreshPlanningPreview`.
// =====================================================================================================

namespace
{
	/**
	 * La rotta a schermo e' il piano dell'unita'. Il confronto e' contro un dato INDIPENDENTE dall'anteprima: `SetPreviewPath`
	 * copia `PlannedPath` cella per cella, quindi il conteggio basta a vedere uno scarto — e non usa una seconda chiamata
	 * all'anteprima, che sarebbe sbagliata nello stesso modo.
	 */
	bool PlannedRouteShowsPlan(const FRTLockInPreviewBench& B)
	{
		return B.HexMap->NumPreviewPathCells() == B.Mine->PlannedPath.Num();
	}

	/** Un click sul vicino della fine del piano che il piano accetta. `false` se nessun vicino e' percorribile (budget finito). */
	bool PlannedRouteGrowByOneStep(FRTLockInPreviewBench& B)
	{
		const int32 Prima = B.Mine->PlannedWaypoints.Num();
		const FRTCellId Fine = B.Mine->PlannedPath.Num() > 0 ? B.Mine->PlannedPath.Last() : B.Mine->Cell;
		for (const FRTCellId& Vicino : URTHexLibrary::Neighbors(Fine))
		{
			if (Vicino == B.Mine->Cell || B.Mine->PlannedPath.Contains(Vicino))
			{
				continue;
			}
			B.PC->HandleClickOnCellForTest(Vicino);
			if (B.Mine->PlannedWaypoints.Num() > Prima)
			{
				return true;
			}
		}
		return false;
	}

	/** Allunga il piano fino al budget, un click per volta. Restituisce la lunghezza raggiunta. */
	int32 PlannedRouteGrowToBudget(FRTLockInPreviewBench& B)
	{
		for (int32 Giro = 0; Giro < 16 && PlannedRouteGrowByOneStep(B); ++Giro) {}
		return B.Mine->PlannedPath.Num();
	}
}

/**
 * **`#3892` — cambiare selezione cambia la rotta a schermo: quella del piano dell'unita' scelta, non un residuo.**
 *
 * 🔑 Il controllo positivo e' lo stato di partenza (rotta tracciata e uguale al piano); il cuore e' un'unita' SENZA piano, che deve
 * lasciare lo schermo senza rotta, e il ritorno sull'unita' col piano, che la deve riportare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlannedRouteFollowsSelectionTest,
	"RefactorTactics.HexMatch.PlannedRouteFollowsSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlannedRouteFollowsSelectionTest::RunTest(const FString&)
{
	FRTLockInPreviewBench B;
	if (!MakeLockInPreviewBench(*this, B))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	ARTUnit* Altra = nullptr;
	for (TActorIterator<ARTUnit> It(B.World); It; ++It)
	{
		if (*It != B.Mine && It->TeamId == B.Mine->TeamId && It->IsAlive()) { Altra = *It; break; }
	}
	if (!TestNotNull(TEXT("premessa: la squadra ha un'altra unita'"), Altra)
		|| !TestEqual(TEXT("premessa: l'altra unita' non ha un piano"), Altra->PlannedPath.Num(), 0)
		|| !TestTrue(TEXT("premessa: Mine ha un piano"), B.Mine->PlannedPath.Num() > 0)
		|| !TestTrue(TEXT("controllo: la rotta di partenza e' il piano di Mine"), PlannedRouteShowsPlan(B)))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	B.PC->SelectUnit(Altra, /*bRecordAsPlayerInput=*/ false);
	TestEqual(TEXT("selezionata un'unita' senza piano non resta la rotta dell'altra"), B.HexMap->NumPreviewPathCells(), 0);

	B.PC->SelectUnit(B.Mine, /*bRecordAsPlayerInput=*/ false);
	TestTrue(TEXT("e tornando su Mine la sua rotta e' di nuovo il suo piano"), PlannedRouteShowsPlan(B) && B.HexMap->NumPreviewPathCells() > 0);

	DestroyHexMatchWorld(B.World);
	return true;
}

/**
 * **`#3892` — aggiungere un waypoint allunga la rotta, annullarlo la accorcia: sempre il piano, mai un'altra cosa.**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlannedRouteFollowsWaypointAddAndUndoTest,
	"RefactorTactics.HexMatch.PlannedRouteFollowsWaypointAddAndUndo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlannedRouteFollowsWaypointAddAndUndoTest::RunTest(const FString&)
{
	FRTLockInPreviewBench B;
	if (!MakeLockInPreviewBench(*this, B))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	const int32 Prima = B.Mine->PlannedPath.Num();
	if (!TestTrue(TEXT("premessa: la rotta di partenza e' il piano di Mine"), Prima > 0 && PlannedRouteShowsPlan(B))
		|| !TestTrue(TEXT("premessa: il piano si puo' allungare di un passo"), PlannedRouteGrowByOneStep(B)))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	TestTrue(TEXT("il click ha allungato il piano"), B.Mine->PlannedPath.Num() > Prima);
	TestTrue(TEXT("e la rotta a schermo lo segue"), PlannedRouteShowsPlan(B));

	B.PC->OnUndoWaypointForTest();
	TestEqual(TEXT("l'annullo toglie il waypoint"), B.Mine->PlannedPath.Num(), Prima);
	TestTrue(TEXT("e la rotta a schermo torna al piano di prima"), PlannedRouteShowsPlan(B));

	DestroyHexMatchWorld(B.World);
	return true;
}

/**
 * **`#3892` — armare l'Overwatch tronca il piano al tetto, e disarmarlo lo restituisce: la rotta segue entrambi.**
 *
 * 🔑 Il piano e' portato al budget perche' il troncamento sia osservabile (`premessa`): con un piano che il tetto non tocca, un
 * sito che non scrivesse la rotta passerebbe lo stesso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlannedRouteFollowsArmAndDisarmTest,
	"RefactorTactics.HexMatch.PlannedRouteFollowsArmAndDisarm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlannedRouteFollowsArmAndDisarmTest::RunTest(const FString&)
{
	FRTLockInPreviewBench B;
	if (!MakeLockInPreviewBench(*this, B))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	const int32 Overwatch = KitIndexOf(B.Mine, FName(TEXT("Action.Overwatch")));
	const int32 Lunga = PlannedRouteGrowToBudget(B);
	if (!TestTrue(TEXT("premessa: l'Overwatch e' nel kit"), Overwatch != INDEX_NONE)
		|| !TestTrue(TEXT("premessa: il piano e' lungo"), Lunga >= 3)
		|| !TestTrue(TEXT("premessa: e la rotta e' il piano"), PlannedRouteShowsPlan(B)))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	B.PC->SelectAbilityForCurrentForTest(Overwatch);
	if (!TestTrue(TEXT("premessa: armare l'Overwatch tronca il piano al tetto"), B.Mine->PlannedPath.Num() < Lunga))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}
	TestTrue(TEXT("armata, la rotta a schermo e' il piano troncato"), PlannedRouteShowsPlan(B));

	B.PC->SelectAbilityForCurrentForTest(INDEX_NONE);
	TestEqual(TEXT("disarmata, il tetto restituisce i waypoint che aveva tolto"), B.Mine->PlannedPath.Num(), Lunga);
	TestTrue(TEXT("e la rotta a schermo e' di nuovo il piano intero"), PlannedRouteShowsPlan(B));

	DestroyHexMatchWorld(B.World);
	return true;
}

/**
 * **`#3892` — dichiarare lo Sneak tronca il piano al suo tetto, e la rotta a schermo lo segue.**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlannedRouteFollowsSneakTest,
	"RefactorTactics.HexMatch.PlannedRouteFollowsSneak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlannedRouteFollowsSneakTest::RunTest(const FString&)
{
	FRTLockInPreviewBench B;
	if (!MakeLockInPreviewBench(*this, B))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	const int32 Lunga = PlannedRouteGrowToBudget(B);
	if (!TestTrue(TEXT("premessa: il piano e' lungo"), Lunga >= 3)
		|| !TestTrue(TEXT("premessa: e la rotta e' il piano"), PlannedRouteShowsPlan(B)))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}

	B.PC->ToggleSneakForTest();
	if (!TestEqual(TEXT("premessa: lo Sneak e' dichiarato"), B.Mine->PlannedMovementProfileId, URTMovementProfileLibrary::ProfileSneak)
		|| !TestTrue(TEXT("premessa: lo Sneak tronca il piano al suo tetto"), B.Mine->PlannedPath.Num() < Lunga))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}
	TestTrue(TEXT("dichiarato, la rotta a schermo e' il piano troncato"), PlannedRouteShowsPlan(B));

	B.PC->ToggleSneakForTest();
	TestTrue(TEXT("ritirato, la rotta a schermo e' ancora il piano, qualunque sia"), PlannedRouteShowsPlan(B));

	DestroyHexMatchWorld(B.World);
	return true;
}

// =====================================================================================================
// `#3750`, fetta C di F5 — il FUOCO sulla mappa, sul controller vero.
//
// Il banco e' quello di `#3511` (partita reale in pianificazione, il controller vero, la mappa vera): qui si guarda cio' che la
// mappa MOSTRA dopo `PinInspection` e `UpdatePlanningOverlay`, non le funzioni pure, che hanno i loro test. La conoscenza della
// squadra si dichiara con `SetTeamKnowledgeForTest`: il banco non ha un `ARTGameMode` che la rinfreschi.
//
// 🔑 **Ogni regola ha un gemello e un controllo positivo**: due mondi che differiscono solo in cio' che la squadra non vede
// (o nel piano del nemico) danno la stessa area; un cambiamento in cio' che si vede la cambia.
// =====================================================================================================

namespace
{
	/** Le celle entro `Radius` da `Center`, ordinate in modo stabile. L'oracolo e' la distanza esagonale, non la libreria. */
	TArray<FRTCellId> FwWithin(const URTHexMapAsset* Map, const FRTCellId& Center, int32 Radius)
	{
		TArray<FRTCellId> Cells;
		for (const FRTCellId& Cell : URTHexLibrary::HexArea(Center, Radius))
		{
			if (Map->ContainsCell(Cell)) { Cells.Add(Cell); }
		}
		Cells.Sort([](const FRTCellId& A, const FRTCellId& B) { return URTHexLibrary::StableLess(A, B); });
		return Cells;
	}

	TArray<FRTCellId> FwSorted(TArray<FRTCellId> Cells)
	{
		Cells.Sort([](const FRTCellId& A, const FRTCellId& B) { return URTHexLibrary::StableLess(A, B); });
		return Cells;
	}

	FRTTeamKnowledge FwAllSeen(const URTHexMapAsset* Map)
	{
		FRTTeamKnowledge Knowledge;
		Knowledge.TeamId = 0;
		for (const FRTHexCellData& Cell : Map->Cells)
		{
			Knowledge.VisibleCells.Add(Cell.Id);
			Knowledge.ExploredCells.Add(Cell.Id);
		}
		return Knowledge;
	}

	/** L'altra unita' propria del banco (la prima e' `Mine`). */
	ARTUnit* FwOtherOwn(UWorld* World, const ARTUnit* Mine)
	{
		for (TActorIterator<ARTUnit> It(World); It; ++It)
		{
			if (It->TeamId == Mine->TeamId && *It != Mine) { return *It; }
		}
		return nullptr;
	}

	/** Un caso del banco del fuoco: cosa cambia nel mondo, cosa sa la squadra, chi si ispeziona. */
	struct FFwCase
	{
		TFunction<void(FRTLockInPreviewBench&, ARTUnit* Enemy)> Edit;
		TFunction<void(FRTTeamKnowledge&, const ARTUnit* Enemy)> Knowledge;
		TFunction<ARTUnit*(FRTLockInPreviewBench&, ARTUnit* Enemy)> PinWho;
		bool bPin = true;
	};

	struct FFwResult
	{
		bool bOk = false;
		TArray<FRTCellId> Normal;
		TArray<FRTCellId> RunOnly;
		int32 Lit = 0;
		FRTCellId EnemyCell;
		int32 EnemyBase = 0;
	};

	/**
	 * Allestisce il banco, applica il caso, ispeziona e fa un tick delle aree: restituisce cio' che la mappa mostra. Nessuna
	 * unita' e' selezionata (`IdleSelection`), cosi' a tenere la mappa puo' essere solo il fuoco.
	 */
	FFwResult FwRun(FAutomationTestBase& Test, const FFwCase& Case)
	{
		FFwResult Out;
		FRTLockInPreviewBench B;
		if (!MakeLockInPreviewBench(Test, B)) { DestroyHexMatchWorld(B.World); return Out; }
		ARTUnit* Enemy = RTWorldFixtures::FirstUnitOfTeam(B.World, 1);
		if (!Test.TestNotNull(TEXT("fuoco: il nemico del banco"), Enemy) || !Test.TestTrue(TEXT("fuoco: la mappa del banco"), B.HexMap->MapAsset != nullptr))
		{
			DestroyHexMatchWorld(B.World);
			return Out;
		}
		if (Case.Edit) { Case.Edit(B, Enemy); }
		FRTTeamKnowledge Knowledge = FwAllSeen(B.HexMap->MapAsset);
		if (Case.Knowledge) { Case.Knowledge(Knowledge, Enemy); }
		B.TM->SetTeamKnowledgeForTest(Knowledge);

		B.PC->SelectActorForTest(nullptr);
		if (Case.bPin)
		{
			B.PC->PinInspectionForTest(Case.PinWho ? Case.PinWho(B, Enemy) : Enemy);
		}
		B.PC->UpdatePlanningOverlay();

		Out.Normal = B.HexMap->GetPreviewFocusNormalCells();
		Out.RunOnly = B.HexMap->GetPreviewFocusRunOnlyCells();
		Out.Lit = PreviewCellsLit(B.HexMap);
		Out.EnemyCell = Enemy->Cell;
		Out.EnemyBase = Enemy->GetEffectiveMoveRange();
		Out.bOk = true;
		DestroyHexMatchWorld(B.World);
		return Out;
	}

	/** L'arena del banco (`MakeCountdownMatch`: raggio 4), per l'oracolo. */
	URTHexMapAsset* FwBenchArena()
	{
		return URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), /*Radius=*/ 4);
	}
}

/**
 * **`#3750` — un nemico ispezionato e osservato mostra le sue DUE aree di movimento, e la mappa non mostra altro.**
 *
 * L'oracolo e' la distanza esagonale sull'arena del banco: normale = entro il movimento base, corsa = entro il doppio, al netto
 * della normale. Il conteggio delle celle accese dice che il piano ha ceduto la mappa (nessuna unita' e' selezionata, ma se
 * un'altra sorgente scrivesse, `Lit` sarebbe piu' grande).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFocusWiringTwoAreasTest,
	"RefactorTactics.FocusWiring.AnInspectedObservedEnemyShowsItsTwoMovementAreas",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFocusWiringTwoAreasTest::RunTest(const FString&)
{
	const FFwResult Shown = FwRun(*this, FFwCase());
	if (!TestTrue(TEXT("banco"), Shown.bOk)) { return false; }
	URTHexMapAsset* Arena = FwBenchArena();

	if (!TestTrue(TEXT("premessa: il nemico si muove"), Shown.EnemyBase > 0)) { return false; }
	const TArray<FRTCellId> WantNormal = FwWithin(Arena, Shown.EnemyCell, Shown.EnemyBase);
	const TSet<FRTCellId> InNormal(WantNormal);
	TArray<FRTCellId> WantRunOnly;
	for (const FRTCellId& Cell : FwWithin(Arena, Shown.EnemyCell, Shown.EnemyBase * 2))
	{
		if (!InNormal.Contains(Cell)) { WantRunOnly.Add(Cell); }
	}

	TestTrue(TEXT("premessa: le due bande hanno celle sull'arena del banco"), WantNormal.Num() > 1 && WantRunOnly.Num() > 0);
	TestTrue(TEXT("l'area normale: tutte e sole le celle entro il movimento base"), FwSorted(Shown.Normal) == WantNormal);
	TestTrue(TEXT("l'area di corsa, al netto della normale: tutte e sole le celle fra il base e il doppio"),
		FwSorted(Shown.RunOnly) == WantRunOnly);
	TestEqual(TEXT("e la mappa non mostra nient'altro"), Shown.Lit, Shown.Normal.Num() + Shown.RunOnly.Num());

	// CONTROLLO POSITIVO: senza ispezione la mappa e' spenta, con la stessa conoscenza e lo stesso mondo.
	FFwCase NoPin;
	NoPin.bPin = false;
	const FFwResult Quiet = FwRun(*this, NoPin);
	TestTrue(TEXT("controllo: senza ispezione nessuna area del fuoco"), Quiet.bOk && Quiet.Normal.Num() == 0 && Quiet.RunOnly.Num() == 0);
	TestEqual(TEXT("controllo: e la mappa e' spenta"), Quiet.Lit, 0);
	return true;
}

/**
 * **`#3750` — MAI IL PIANO, sul percorso cablato.** Due mondi che differiscono solo nel piano del nemico danno la stessa area;
 * gli stati PUBBLICI (`Slow`, `Unbalanced`), invece, la cambiano — ciascuno per la sua parte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFocusWiringNeverThePlanTest,
	"RefactorTactics.FocusWiring.TheAreaIsTheEnemyPotentialWhateverItPlans",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFocusWiringNeverThePlanTest::RunTest(const FString&)
{
	const FFwResult Quiet = FwRun(*this, FFwCase());

	FFwCase Planner;
	Planner.Edit = [](FRTLockInPreviewBench&, ARTUnit* Enemy)
	{
		// Il piano piu' pericoloso: waypoint, un'azione e il profilo di corsa dichiarato.
		for (const FRTCellId& Step : URTHexLibrary::Neighbors(Enemy->Cell)) { Enemy->PlannedWaypoints.Add(Step); }
		Enemy->PlannedAbilityIndex = 0;
		Enemy->PlannedDashAbility = 1;
		Enemy->PlannedMovementProfileId = URTMovementProfileLibrary::ProfileSprint;
	};
	const FFwResult Planning = FwRun(*this, Planner);

	FFwCase Slowed;
	Slowed.Edit = [](FRTLockInPreviewBench&, ARTUnit* Enemy) { Enemy->ApplyStatus(TAG_Status_Slow, 2); };
	const FFwResult SlowedResult = FwRun(*this, Slowed);

	FFwCase Unbalanced;
	Unbalanced.Edit = [](FRTLockInPreviewBench&, ARTUnit* Enemy) { Enemy->ApplyStatus(TAG_Status_Unbalanced, 2); };
	const FFwResult UnbalancedResult = FwRun(*this, Unbalanced);

	if (!TestTrue(TEXT("banco"), Quiet.bOk && Planning.bOk && SlowedResult.bOk && UnbalancedResult.bOk)) { return false; }
	TestTrue(TEXT("premessa: l'area quieta ha le due bande"), Quiet.Normal.Num() > 1 && Quiet.RunOnly.Num() > 0);

	TestTrue(TEXT("il piano del nemico non cambia l'area normale"), FwSorted(Planning.Normal) == FwSorted(Quiet.Normal));
	TestTrue(TEXT("ne' quella di corsa"), FwSorted(Planning.RunOnly) == FwSorted(Quiet.RunOnly));

	// Controlli positivi: gli stati PUBBLICI cambiano l'area.
	TestTrue(TEXT("Slow restringe l'area normale"), SlowedResult.Normal.Num() < Quiet.Normal.Num());
	TestEqual(TEXT("Unbalanced toglie la corsa"), UnbalancedResult.RunOnly.Num(), 0);
	TestTrue(TEXT("e lascia l'area normale com'era"), FwSorted(UnbalancedResult.Normal) == FwSorted(Quiet.Normal));
	return true;
}

/**
 * **`#3750` — MONDI GEMELLI sulla geometria che la squadra non vede, sul percorso cablato.** Le celle non in vista (mai viste, o
 * esplorate e non piu' in vista) sono muri nel calcolo: un muro in piu' o in meno LI' SOTTO non cambia l'area, uno in una cella in
 * vista si'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFocusWiringHiddenGeometryTest,
	"RefactorTactics.FocusWiring.TheAreaIsTheSameWhenOnlyWhatIsHiddenDiffers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFocusWiringHiddenGeometryTest::RunTest(const FString&)
{
	// Il buio: il lato sud della mappa (Y >= 1). Il nemico del banco sta a Y = -1, quindi in vista.
	const auto IsDark = [](const FRTCellId& Cell) { return Cell.Y >= 1; };
	const auto DarkNeverSeen = [IsDark](FRTTeamKnowledge& K, const ARTUnit*)
	{
		K.VisibleCells.RemoveAll([IsDark](const FRTCellId& Cell) { return IsDark(Cell); });
		K.ExploredCells.RemoveAll([IsDark](const FRTCellId& Cell) { return IsDark(Cell); });
	};
	const auto DarkExplored = [IsDark](FRTTeamKnowledge& K, const ARTUnit*)
	{
		K.VisibleCells.RemoveAll([IsDark](const FRTCellId& Cell) { return IsDark(Cell); }); // esplorate: restano in ExploredCells
	};
	const auto WallsInTheDark = [IsDark](FRTLockInPreviewBench& B, ARTUnit*)
	{
		for (FRTHexCellData& Cell : B.HexMap->MapAsset->Cells) { if (IsDark(Cell.Id)) { Cell.bBlocksMovement = true; } }
	};

	FFwCase Open;     Open.Knowledge = DarkNeverSeen;
	FFwCase Walled;   Walled.Edit = WallsInTheDark;   Walled.Knowledge = DarkNeverSeen;
	// 🔴 I due mondi ESPLORATI differiscono nel SOLO stato nascosto: in entrambi il buio e' esplorato e non piu' in vista, in uno
	// ha i muri e nell'altro no. (Una stesura precedente confrontava il buio esplorato CON muri col buio mai visto SENZA: era
	// equivalente al caso `Walled`, e «l'esplorato vale come visto» restava verde — rilievo della review.)
	FFwCase ExploredWalled; ExploredWalled.Edit = WallsInTheDark; ExploredWalled.Knowledge = DarkExplored;
	FFwCase ExploredOpen;   ExploredOpen.Knowledge = DarkExplored;
	const FFwResult InOpen = FwRun(*this, Open);
	const FFwResult InWalled = FwRun(*this, Walled);
	const FFwResult InExploredWalled = FwRun(*this, ExploredWalled);
	const FFwResult InExploredOpen = FwRun(*this, ExploredOpen);
	if (!TestTrue(TEXT("banco"), InOpen.bOk && InWalled.bOk && InExploredWalled.bOk && InExploredOpen.bOk)) { return false; }
	TestTrue(TEXT("premessa: c'e' un'area da confrontare"), InOpen.Normal.Num() > 1);

	TestTrue(TEXT("la geometria mai vista non cambia l'area normale"), FwSorted(InWalled.Normal) == FwSorted(InOpen.Normal));
	TestTrue(TEXT("ne' quella di corsa"), FwSorted(InWalled.RunOnly) == FwSorted(InOpen.RunOnly));
	TestTrue(TEXT("la geometria esplorata e non piu' in vista: stessa area normale con i muri e senza"),
		FwSorted(InExploredWalled.Normal) == FwSorted(InExploredOpen.Normal));
	TestTrue(TEXT("...e stessa area di corsa"), FwSorted(InExploredWalled.RunOnly) == FwSorted(InExploredOpen.RunOnly));
	TestTrue(TEXT("e l'esplorato non vale come visto: l'area e' quella del buio mai visto"),
		FwSorted(InExploredOpen.Normal) == FwSorted(InOpen.Normal) && FwSorted(InExploredOpen.RunOnly) == FwSorted(InOpen.RunOnly));

	// CONTROLLO POSITIVO 1: guardando tutto, quel buio MUTEREBBE l'area (e' nel raggio del nemico).
	FFwCase SeenWalled;   SeenWalled.Edit = WallsInTheDark;
	const FFwResult AllSeen = FwRun(*this, FFwCase());
	const FFwResult AllSeenWalled = FwRun(*this, SeenWalled);
	TestTrue(TEXT("premessa: se si vedesse tutto, i muri nel buio cambierebbero l'area"),
		AllSeen.bOk && AllSeenWalled.bOk && (FwSorted(AllSeen.Normal) != FwSorted(AllSeenWalled.Normal)
			|| FwSorted(AllSeen.RunOnly) != FwSorted(AllSeenWalled.RunOnly)));

	// CONTROLLO POSITIVO 2: un muro in una cella IN VISTA accanto al nemico cambia l'area.
	FFwCase VisibleWall;
	VisibleWall.Edit = [](FRTLockInPreviewBench& B, ARTUnit* Enemy)
	{
		// Il primo vicino che esiste nella mappa (il nemico sta vicino al bordo): un muro fuori mappa non cambierebbe niente.
		for (const FRTCellId& Next : URTHexLibrary::Neighbors(Enemy->Cell))
		{
			if (!B.HexMap->MapAsset->ContainsCell(Next)) { continue; }
			for (FRTHexCellData& Cell : B.HexMap->MapAsset->Cells) { if (Cell.Id == Next) { Cell.bBlocksMovement = true; } }
			break;
		}
	};
	const FFwResult WithWall = FwRun(*this, VisibleWall);
	TestTrue(TEXT("un muro in una cella in vista cambia l'area"),
		WithWall.bOk && (FwSorted(WithWall.Normal) != FwSorted(AllSeen.Normal)));
	return true;
}

/**
 * **`#3750` — P2 sul controller: un nemico NON osservato non ha area, anche se l'ispezione lo ha fissato.**
 *
 * Il click fissa un nemico appena nato senza voce `Live` (`IsKnownToObserver` nasce vero), e la fissazione dura fino al tick
 * dell'HUD. In quella finestra l'area mostrerebbe a chi guarda cio' che non vede: il cancello e' del controller, sulla stessa
 * vista del velo. Ogni caso e' un modo di non essere osservato; il controllo e' lo stesso mondo, osservato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFocusWiringUnobservedTest,
	"RefactorTactics.FocusWiring.AnUnobservedEnemyHasNoAreaEvenWhenPinned",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFocusWiringUnobservedTest::RunTest(const FString&)
{
	const FFwResult Observed = FwRun(*this, FFwCase());
	if (!TestTrue(TEXT("banco"), Observed.bOk && Observed.Normal.Num() > 0)) { return false; }

	struct FUnobserved { const TCHAR* Name; FFwCase Case; };
	TArray<FUnobserved> Cases;

	FFwCase OnADarkCell;
	OnADarkCell.Knowledge = [](FRTTeamKnowledge& K, const ARTUnit* Enemy)
	{
		K.VisibleCells.Remove(Enemy->Cell);
		K.ExploredCells.Remove(Enemy->Cell);
	};
	Cases.Add({ TEXT("la sua cella non e' mai stata vista"), OnADarkCell });

	// ESPLORATA e non piu' in vista: dice dove sta l'unita' quanto una cella mai vista (D-225, D-227).
	FFwCase OnAnExploredCell;
	OnAnExploredCell.Knowledge = [](FRTTeamKnowledge& K, const ARTUnit* Enemy) { K.VisibleCells.Remove(Enemy->Cell); };
	Cases.Add({ TEXT("la sua cella e' esplorata ma non piu' in vista"), OnAnExploredCell });

	// Solo un RICORDO: la squadra ha un contatto vivo, quindi la vista ha una voce `Remembered` — che esiste, e non e' `Live`.
	// E' il caso che distingue «c'e' una voce» da «la voce e' `Live`»: il cancello e' il secondo (P2).
	//
	// 🔴 **Il contatto sta su una cella VICINA E ANCORA IN VISTA, non sulla cella del nemico.** Con il contatto sulla stessa
	// cella non in vista il caso era vacuo: `URTFocusAreaLibrary::MoveAreasFor` restituisce gia' area vuota per un soggetto su una
	// cella non in vista, e togliere il cancello (`IsInspectable`) lasciava il test verde (la mutazione K1 sopravviveva). Con
	// l'ultimo avvistamento su una cella che si vede, senza cancello l'area partirebbe da dove il nemico non e' piu'.
	FFwCase OnlyRemembered;
	OnlyRemembered.Knowledge = [](FRTTeamKnowledge& K, const ARTUnit* Enemy)
	{
		K.VisibleCells.Remove(Enemy->Cell);
		FRTCellId LastSeen = Enemy->Cell;
		for (const FRTCellId& Next : URTHexLibrary::Neighbors(Enemy->Cell))
		{
			if (K.VisibleCells.Contains(Next)) { LastSeen = Next; break; }
		}
		K.Contacts.Add(FRTLastKnownContact(Enemy->StableUnitId, LastSeen, /*InTurn=*/ 1));
	};
	Cases.Add({ TEXT("la squadra ne ha solo un ricordo (voce Remembered)"), OnlyRemembered });

	// Cella in vista ma conoscenza illeggibile (versione sbagliata): fail-closed, nessuna voce `Live`.
	FFwCase UnreadableKnowledge;
	UnreadableKnowledge.Knowledge = [](FRTTeamKnowledge& K, const ARTUnit*) { K.Version = 0; };
	Cases.Add({ TEXT("la cella e' in vista ma la conoscenza della squadra e' illeggibile"), UnreadableKnowledge });

	FFwCase Fallen;
	Fallen.Edit = [](FRTLockInPreviewBench&, ARTUnit* Enemy) { Enemy->Health = 0; };
	Cases.Add({ TEXT("e' caduto"), Fallen });

	// Un'unita' PROPRIA a fuoco non e' coperta da D-552 (scelta della sessione): niente area.
	FFwCase OwnUnit;
	OwnUnit.PinWho = [](FRTLockInPreviewBench& B, ARTUnit*) { return FwOtherOwn(B.World, B.Mine); };
	Cases.Add({ TEXT("e' un'unita' propria"), OwnUnit });

	for (const FUnobserved& Case : Cases)
	{
		const FFwResult Result = FwRun(*this, Case.Case);
		TestTrue(*FString::Printf(TEXT("%s: banco"), Case.Name), Result.bOk);
		TestEqual(*FString::Printf(TEXT("%s: nessuna area normale"), Case.Name), Result.Normal.Num(), 0);
		TestEqual(*FString::Printf(TEXT("%s: nessuna area di corsa"), Case.Name), Result.RunOnly.Num(), 0);
		TestEqual(*FString::Printf(TEXT("%s: e la mappa e' spenta"), Case.Name), Result.Lit, 0);
	}
	return true;
}

/**
 * **`#3750` — IL CICLO DEL FUOCO sul controller.** Il piano cede la mappa all'area del soggetto; armare la fa uscire e il piano
 * torna; disarmare non la riporta; una nuova fissazione si'; `RMB` chiude l'ispezione e il piano torna. In tutti i passi
 * la scheda (`InspectedUnit`) resta finche' non e' chiusa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFocusWiringLifecycleTest,
	"RefactorTactics.FocusWiring.TheAreaCedesTheMapToThePlanAtTheFirstGestureAndDoesNotComeBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFocusWiringLifecycleTest::RunTest(const FString&)
{
	FRTLockInPreviewBench B;
	if (!MakeLockInPreviewBench(*this, B)) { DestroyHexMatchWorld(B.World); return false; }
	ARTUnit* Enemy = RTWorldFixtures::FirstUnitOfTeam(B.World, 1);
	if (!TestNotNull(TEXT("il nemico del banco"), Enemy)) { DestroyHexMatchWorld(B.World); return false; }
	B.TM->SetTeamKnowledgeForTest(FwAllSeen(B.HexMap->MapAsset));

	// Mine selezionata e senza piano: `Planning`, e il ventaglio e' acceso (la sorgente e' il piano).
	B.Mine->PlannedWaypoints.Reset();
	B.PC->RefreshPlanningOverlay(B.Mine);
	if (!TestEqual(TEXT("premessa: il contesto e' Planning"), B.PC->GetPointerContext(), ERTPointerContext::Planning)
		|| !TestTrue(TEXT("premessa: il ventaglio del piano e' acceso"), B.HexMap->NumPreviewReachableCells() > 0))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}
	TestEqual(TEXT("premessa: nessuna area del fuoco"), B.HexMap->GetPreviewFocusNormalCells().Num(), 0);

	// 1. LA FISSAZIONE: il fuoco prende la mappa, e il ventaglio del piano cede.
	B.PC->PinInspectionForTest(Enemy);
	B.PC->UpdatePlanningOverlay();
	TestTrue(TEXT("1: l'area normale del nemico e' sulla mappa"), B.HexMap->GetPreviewFocusNormalCells().Num() > 0);
	TestEqual(TEXT("1: il ventaglio dell'unita' comandata ha ceduto la mappa"), B.HexMap->NumPreviewReachableCells(), 0);
	TestTrue(TEXT("1: la scheda e' fissata sul nemico"), B.PC->GetInspectedUnit() == Enemy);

	// 2. IL PRIMO GESTO (armare): l'area esce, il piano torna, la scheda resta.
	int32 Armed = INDEX_NONE;
	for (int32 I = 0; I < B.Mine->NumAbilities() && Armed == INDEX_NONE; ++I)
	{
		B.Mine->SelectAbility(I);
		if (B.Mine->SelectedAbilityIndex == I) { Armed = I; }
	}
	if (!TestTrue(TEXT("premessa: si puo' armare un'azione"), Armed != INDEX_NONE)) { DestroyHexMatchWorld(B.World); return false; }
	B.PC->UpdatePlanningOverlay();
	TestEqual(TEXT("2: armare fa uscire l'area normale"), B.HexMap->GetPreviewFocusNormalCells().Num(), 0);
	TestEqual(TEXT("2: ...e quella di corsa"), B.HexMap->GetPreviewFocusRunOnlyCells().Num(), 0);
	TestTrue(TEXT("2: il piano ha ripreso la mappa"), PreviewCellsLit(B.HexMap) > 0);
	TestTrue(TEXT("2: la scheda resta"), B.PC->GetInspectedUnit() == Enemy);

	// 3. DISARMARE non la riporta.
	B.Mine->SelectAbility(INDEX_NONE);
	B.PC->UpdatePlanningOverlay();
	TestEqual(TEXT("3: disarmare non riporta l'area"), B.HexMap->GetPreviewFocusNormalCells().Num(), 0);
	TestTrue(TEXT("3: il ventaglio del piano e' tornato"), B.HexMap->NumPreviewReachableCells() > 0);

	// 4. UNA NUOVA FISSAZIONE la riporta, sulla stessa unita'.
	B.PC->PinInspectionForTest(Enemy);
	B.PC->UpdatePlanningOverlay();
	TestTrue(TEXT("4: rifissare riporta l'area"), B.HexMap->GetPreviewFocusNormalCells().Num() > 0);
	TestEqual(TEXT("4: e il ventaglio cede di nuovo"), B.HexMap->NumPreviewReachableCells(), 0);

	// 5. UN WAYPOINT e' un gesto di pianificazione: la revisione sale e l'area esce.
	B.Mine->PlannedWaypoints.Add(URTHexLibrary::Neighbors(B.Mine->Cell)[0]);
	B.PC->RefreshPlanningOverlay(B.Mine);
	TestEqual(TEXT("5: un waypoint fa uscire l'area"), B.HexMap->GetPreviewFocusNormalCells().Num(), 0);
	B.Mine->PlannedWaypoints.Reset();
	B.PC->RefreshPlanningOverlay(B.Mine);

	// 6. RMB con un fuoco attivo lo chiude (passo `Inspector` del Back) e tornano le anteprime di pianificazione.
	B.PC->PinInspectionForTest(Enemy);
	B.PC->UpdatePlanningOverlay();
	if (!TestTrue(TEXT("6: premessa, l'area e' tornata"), B.HexMap->GetPreviewFocusNormalCells().Num() > 0))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}
	B.PC->ApplyBack();
	B.PC->UpdatePlanningOverlay();
	TestTrue(TEXT("6: RMB chiude l'ispezione"), B.PC->GetInspectedUnit() == nullptr);
	TestEqual(TEXT("6: e l'area lascia la mappa"), B.HexMap->GetPreviewFocusNormalCells().Num(), 0);
	TestTrue(TEXT("6: tornano le anteprime di pianificazione"), B.HexMap->NumPreviewReachableCells() > 0);

	DestroyHexMatchWorld(B.World);
	return true;
}

/**
 * **`#3750` — l'area segue la conoscenza, non l'attore.** Se il soggetto esce di vista l'area lascia la mappa subito (anche prima
 * che l'HUD rilasci l'ispezione), e non torna quando rientra: serve una nuova fissazione. Cio' da cui l'area dipende
 * (la vista) la rifa'; i tick fermi e l'hover no.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFocusWiringFollowsKnowledgeTest,
	"RefactorTactics.FocusWiring.TheAreaFollowsWhatTheTeamSeesAndIdleTicksDoNotRedoIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFocusWiringFollowsKnowledgeTest::RunTest(const FString&)
{
	FRTLockInPreviewBench B;
	if (!MakeLockInPreviewBench(*this, B)) { DestroyHexMatchWorld(B.World); return false; }
	ARTUnit* Enemy = RTWorldFixtures::FirstUnitOfTeam(B.World, 1);
	if (!TestNotNull(TEXT("il nemico del banco"), Enemy)) { DestroyHexMatchWorld(B.World); return false; }
	const FRTTeamKnowledge All = FwAllSeen(B.HexMap->MapAsset);
	B.TM->SetTeamKnowledgeForTest(All);
	B.PC->SelectActorForTest(nullptr);
	B.PC->PinInspectionForTest(Enemy);
	B.PC->UpdatePlanningOverlay();
	const TArray<FRTCellId> Shown = B.HexMap->GetPreviewFocusNormalCells();
	if (!TestTrue(TEXT("premessa: l'area e' sulla mappa"), Shown.Num() > 1)) { DestroyHexMatchWorld(B.World); return false; }

	// CONTATORI: dieci tick fermi non ricalcolano; l'hover non rifa' l'area.
	int32 P = B.PC->GetPlanningOverlayPlanComputations();
	for (int32 Tick = 0; Tick < 10; ++Tick) { B.PC->UpdatePlanningOverlay(); }
	TestEqual(TEXT("dieci tick a ingressi invariati non rifanno l'area"), B.PC->GetPlanningOverlayPlanComputations(), P);
	B.HexMap->SetHoveredCell(Shown[0], /*bValid=*/ true);
	B.PC->UpdatePlanningOverlay();
	TestEqual(TEXT("la sola cella sotto il cursore non rifa' l'area (criterio di #711)"), B.PC->GetPlanningOverlayPlanComputations(), P);
	TestTrue(TEXT("e l'area e' la stessa"), FwSorted(B.HexMap->GetPreviewFocusNormalCells()) == FwSorted(Shown));

	// LA VISTA CAMBIA: una cella dell'area (non quella del nemico) non si vede piu' -> l'area si rifa' senza di lei.
	FRTCellId Lost = Shown[0];
	for (const FRTCellId& Cell : Shown) { if (!(Cell == Enemy->Cell)) { Lost = Cell; break; } }
	FRTTeamKnowledge Narrower = All;
	Narrower.VisibleCells.Remove(Lost);
	B.TM->SetTeamKnowledgeForTest(Narrower);
	B.PC->UpdatePlanningOverlay();
	TestEqual(TEXT("una vista diversa rifa' l'area"), B.PC->GetPlanningOverlayPlanComputations(), P + 1);
	TestFalse(TEXT("la cella non piu' in vista non e' nell'area normale"), B.HexMap->GetPreviewFocusNormalCells().Contains(Lost));
	TestFalse(TEXT("ne' in quella di corsa"), B.HexMap->GetPreviewFocusRunOnlyCells().Contains(Lost));
	TestTrue(TEXT("controllo: prima c'era"), Shown.Contains(Lost));

	// IL SOGGETTO ESCE DI VISTA: l'area lascia la mappa subito, la scheda resta (la rilascia l'HUD) e non torna da sola.
	FRTTeamKnowledge Blind = All;
	Blind.VisibleCells.Remove(Enemy->Cell);
	B.TM->SetTeamKnowledgeForTest(Blind);
	B.PC->UpdatePlanningOverlay();
	TestEqual(TEXT("fuori di vista l'area lascia la mappa"), B.HexMap->GetPreviewFocusNormalCells().Num(), 0);
	TestEqual(TEXT("...tutta"), PreviewCellsLit(B.HexMap), 0);
	TestTrue(TEXT("la scheda resta finche' non la rilascia l'HUD"), B.PC->GetInspectedUnit() == Enemy);
	B.TM->SetTeamKnowledgeForTest(All);
	B.PC->UpdatePlanningOverlay();
	TestEqual(TEXT("rientrato in vista l'area NON torna da sola"), B.HexMap->GetPreviewFocusNormalCells().Num(), 0);
	B.PC->PinInspectionForTest(Enemy);
	B.PC->UpdatePlanningOverlay();
	TestTrue(TEXT("una nuova fissazione la riporta"), B.HexMap->GetPreviewFocusNormalCells().Num() > 0);

	DestroyHexMatchWorld(B.World);
	return true;
}

/**
 * **`#3750` — il click di PRODUZIONE fissa il fuoco.** `DispatchUnitClick` su un nemico osservato, da `Planning`, ispeziona e passa
 * da `PinInspection`: l'area compare e il ventaglio dell'unita' comandata cede. Senza questo, i test sopra provano un
 * collegamento che il gioco non percorre.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFocusWiringClickTest,
	"RefactorTactics.FocusWiring.AClickOnAnObservedEnemyPinsTheFocusThroughTheProductionPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFocusWiringClickTest::RunTest(const FString&)
{
	FRTLockInPreviewBench B;
	if (!MakeLockInPreviewBench(*this, B)) { DestroyHexMatchWorld(B.World); return false; }
	ARTUnit* Enemy = RTWorldFixtures::FirstUnitOfTeam(B.World, 1);
	if (!TestNotNull(TEXT("il nemico del banco"), Enemy)) { DestroyHexMatchWorld(B.World); return false; }
	B.TM->SetTeamKnowledgeForTest(FwAllSeen(B.HexMap->MapAsset));
	B.Mine->PlannedWaypoints.Reset();
	B.PC->RefreshPlanningOverlay(B.Mine);
	if (!TestEqual(TEXT("premessa: il contesto e' Planning"), B.PC->GetPointerContext(), ERTPointerContext::Planning))
	{
		DestroyHexMatchWorld(B.World);
		return false;
	}
	TestEqual(TEXT("premessa: nessuna area del fuoco prima del click"), B.HexMap->GetPreviewFocusNormalCells().Num(), 0);

	TestTrue(TEXT("il click sul nemico osservato e' consumato"), B.PC->DispatchUnitClickForTest(Enemy, B.Mine));
	TestTrue(TEXT("e fissa la scheda"), B.PC->GetInspectedUnit() == Enemy);
	B.PC->UpdatePlanningOverlay();
	TestTrue(TEXT("l'area di movimento del nemico e' sulla mappa"), B.HexMap->GetPreviewFocusNormalCells().Num() > 0);
	TestEqual(TEXT("e il ventaglio dell'unita' comandata ha ceduto"), B.HexMap->NumPreviewReachableCells(), 0);
	TestTrue(TEXT("la selezione non e' cambiata: il fuoco non tocca selezione ne' piano"), B.PC->GetSelectedUnit() == B.Mine);

	DestroyHexMatchWorld(B.World);
	return true;
}

/**
 * **`#3750` — le altre unita' non cambiano l'area.** L'occupazione non entra nel ventaglio (D-445, D-446), e un nemico non
 * osservato non puo' cambiarla (P2). Un mondo con unita' proprie e un secondo nemico DENTRO l'area, e un terzo nemico non
 * osservato nel buio, da' la stessa area del mondo vuoto. Una regressione verso uno snapshot con tutte le unita' (come fa
 * `PlanningSnapshotFor`) sarebbe verde senza questo gemello (rilievo della review).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFocusWiringCrowdTest,
	"RefactorTactics.FocusWiring.OtherUnitsDoNotChangeTheArea",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFocusWiringCrowdTest::RunTest(const FString&)
{
	const auto DarkNeverSeen = [](FRTTeamKnowledge& K, const ARTUnit*)
	{
		K.VisibleCells.RemoveAll([](const FRTCellId& Cell) { return Cell.Y >= 1; });
		K.ExploredCells.RemoveAll([](const FRTCellId& Cell) { return Cell.Y >= 1; });
	};
	FFwCase Empty;
	Empty.Knowledge = DarkNeverSeen;
	const FFwResult Baseline = FwRun(*this, Empty);
	if (!TestTrue(TEXT("banco"), Baseline.bOk && Baseline.Normal.Num() > 2)) { return false; }

	// Due celle DENTRO l'area, che non sono quella del nemico, per le unita' che la affollano.
	TArray<FRTCellId> Inside;
	for (const FRTCellId& Cell : Baseline.Normal)
	{
		if (!(Cell == Baseline.EnemyCell) && Inside.Num() < 3) { Inside.Add(Cell); }
	}
	if (!TestEqual(TEXT("premessa: tre celle dell'area per le unita'"), Inside.Num(), 3)) { return false; }
	const FRTCellId InTheDark(0, 2, 0);

	FFwCase Crowded;
	Crowded.Knowledge = DarkNeverSeen;
	Crowded.Edit = [Inside, InTheDark](FRTLockInPreviewBench& B, ARTUnit*)
	{
		B.Mine->PlaceOnCell(Inside[0], FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		if (ARTUnit* Ally = FwOtherOwn(B.World, B.Mine)) { Ally->PlaceOnCell(Inside[1], FVector::ZeroVector, 100.f, 250.f); }
		SpawnHexMatchUnit(B.World, /*TeamId=*/ 1, URTHeroCatalogLibrary::MakeAevik(), Inside[2]); // un secondo nemico, in vista
		SpawnHexMatchUnit(B.World, /*TeamId=*/ 1, URTHeroCatalogLibrary::MakeBranth(), InTheDark); // un terzo, nel buio
	};
	const FFwResult WithCrowd = FwRun(*this, Crowded);
	if (!TestTrue(TEXT("banco affollato"), WithCrowd.bOk)) { return false; }

	TestTrue(TEXT("unita' proprie e un secondo nemico dentro l'area, un terzo nel buio: l'area normale e' la stessa"),
		FwSorted(WithCrowd.Normal) == FwSorted(Baseline.Normal));
	TestTrue(TEXT("...e quella di corsa"), FwSorted(WithCrowd.RunOnly) == FwSorted(Baseline.RunOnly));
	TestTrue(TEXT("premessa: le celle delle unita' sono davvero dentro l'area"), WithCrowd.Normal.Contains(Inside[0])
		&& WithCrowd.Normal.Contains(Inside[1]) && WithCrowd.Normal.Contains(Inside[2]));
	return true;
}

/**
 * **`#3750` — il modale NASCONDE l'area, non la chiude** (spec §3 regole 5 e 11: la scheda torna quando si chiude, e l'area con
 * lei). Pausa aperta: la mappa e' spenta e l'ispezione resta; pausa chiusa: l'area torna da sola, perche' aprire un modale non
 * e' un gesto di pianificazione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFocusWiringModalTest,
	"RefactorTactics.FocusWiring.APauseHidesTheAreaAndClosingItBringsItBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFocusWiringModalTest::RunTest(const FString&)
{
	FRTLockInPreviewBench B;
	if (!MakeLockInPreviewBench(*this, B)) { DestroyHexMatchWorld(B.World); return false; }
	ARTUnit* Enemy = RTWorldFixtures::FirstUnitOfTeam(B.World, 1);
	if (!TestNotNull(TEXT("il nemico del banco"), Enemy)) { DestroyHexMatchWorld(B.World); return false; }
	B.TM->SetTeamKnowledgeForTest(FwAllSeen(B.HexMap->MapAsset));
	B.PC->SelectActorForTest(nullptr);
	B.PC->PinInspectionForTest(Enemy);
	B.PC->UpdatePlanningOverlay();
	const TArray<FRTCellId> Before = B.HexMap->GetPreviewFocusNormalCells();
	if (!TestTrue(TEXT("premessa: l'area e' sulla mappa"), Before.Num() > 1)) { DestroyHexMatchWorld(B.World); return false; }

	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	GameInstance->AddToRoot();
	GameInstance->Init();
	URTFrontendNavigator* Navigator = GameInstance->GetSubsystem<URTFrontendNavigator>();
	if (TestNotNull(TEXT("il navigatore esiste"), Navigator))
	{
		B.World->SetGameInstance(GameInstance);
		Navigator->EnterMatch();
		Navigator->ShowPause();
		if (TestEqual(TEXT("premessa: il contesto e' Modal"), B.PC->GetPointerContext(), ERTPointerContext::Modal))
		{
			B.PC->UpdatePlanningOverlay();
			TestEqual(TEXT("con la pausa aperta la mappa e' spenta"), PreviewCellsLit(B.HexMap), 0);
			TestTrue(TEXT("e la scheda resta fissata"), B.PC->GetInspectedUnit() == Enemy);

			Navigator->ResumeMatch();
			TestEqual(TEXT("premessa: chiusa la pausa il contesto non e' piu' Modal"),
				B.PC->GetPointerContext() == ERTPointerContext::Modal, false);
			B.PC->UpdatePlanningOverlay();
			TestTrue(TEXT("chiusa la pausa l'area TORNA da sola: aprire un modale non e' un gesto di pianificazione"),
				FwSorted(B.HexMap->GetPreviewFocusNormalCells()) == FwSorted(Before));
		}
	}

	GameInstance->Shutdown();
	GameInstance->RemoveFromRoot();
	DestroyHexMatchWorld(B.World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
