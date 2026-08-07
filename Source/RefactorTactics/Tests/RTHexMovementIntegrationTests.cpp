#include "Misc/AutomationTest.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLog.h"
#include "Unit/RTUnit.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Core/RTGameplayTags.h"
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

	/**
	 * Aggiunge `Action.Sprint` all'unita' e ne restituisce l'indice.
	 *
	 * L'azione arriva DAL CATALOGO e basta il `Def`: niente `bDash`, niente `RangeCells` sull'asset (che vale 5
	 * per default). Se il resolver leggesse ancora il campo legacy invece del catalogo, gli 8 MP non ci
	 * sarebbero e i test qui sotto fallirebbero — che e' esattamente cio' che devono sorvegliare.
	 */
	int32 AddSprintAbility(ARTUnit* Unit)
	{
		if (!Unit) { return INDEX_NONE; }
		URTActionData* Sprint = NewObject<URTActionData>(Unit);
		Sprint->DisplayName = FText::FromString(TEXT("Scatto lungo"));
		Sprint->Def = URTCatalogLibrary::FindCoreAction(TEXT("Action.Sprint"));
		Unit->Abilities.Add(Sprint);
		return Unit->Abilities.Num() - 1;
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMoveBudgetCostsTest,
	"RefactorTactics.HexMove.BudgetCostsInMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMoveBudgetCostsTest::RunTest(const FString&)
{
	// Il budget si spende in PUNTI MOVIMENTO, non in celle: su terreno difficile (costo 2) la stessa unita'
	// arriva meno lontano. La libreria pura lo verifica gia' (HexSim.ReachableRespectsTerrainCost); qui si
	// verifica che la PARTITA lo rispetti — cioe' che il TurnManager passi davvero i costi dell'asset.
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	// META' MAPPA difficile: TUTTE le celle a est (X > 0) costano 2. Non basta un corridoio: l'A* lo
	// aggirerebbe dalle celle vicine a costo 1, e il test misurerebbe la deviazione invece del budget.
	URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 6))
	{
		FRTHexCellData Data(Id);
		if (Id.X > 0)
		{
			Data.Surface = ERTHexSurface::Rough;
			Data.MoveCost = 2; // terreno difficile del catalogo v0.1
		}
		Map->AddOrUpdateCell(Data);
	}
	Map->SortCells();
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Map;

	ARTUnit* Mover = SpawnHexUnit(World, 0, ERTArchetype::Ranger, FRTCellId(0, 0));
	// Un avversario lontano e fermo: senza, la squadra 1 e' gia' eliminata e la partita finisce al primo
	// turno (MatchEnded), quindi il secondo lock-in non risolverebbe nulla.
	ARTUnit* Foe = SpawnHexUnit(World, 1, ERTArchetype::Guardian, FRTCellId(-5, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover || !Foe) { DestroyHexMoveWorld(World); return false; }
	Foe->PlannedCell = Foe->Cell;

	// Budget 5: sul corridoio costoso bastano per 2 celle (2+2=4), non per 3 (6 > 5).
	const int32 Budget = Mover->GetEffectiveMoveRange();
	TestEqual(TEXT("il Ranger ha il budget standard"), Budget, 5);

	Mover->PlannedCell = FRTCellId(3, 0); // costo 6: oltre il budget
	RunTurn(TM);
	TestTrue(TEXT("tre celle di terreno difficile costano piu' del budget: non ci arriva"),
		!(Mover->Cell == FRTCellId(3, 0)));
	TestTrue(TEXT("resta comunque su una cella valida"), Map->ContainsCell(Mover->Cell));

	// Alla stessa distanza sul lato NORMALE (ovest, costo 1) ci arriva invece senza problemi: e' il COSTO a
	// fermarla, non la distanza.
	Mover->PlaceOnCell(FRTCellId(0, 0), FVector::ZeroVector, 100.f, 250.f);
	Mover->PlannedCell = FRTCellId(-3, 0); // 3 celle a costo 1
	Foe->PlannedCell = Foe->Cell;
	RunTurn(TM);
	TestTrue(TEXT("tre celle normali rientrano nello stesso budget"), Mover->Cell == FRTCellId(-3, 0));

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSprintAppliesExposedTest,
	"RefactorTactics.Actions.Sprint.AppliesExposed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSprintAppliesExposedTest::RunTest(const FString&)
{
	// Sprint (catalogo v0.1 §2): 8 MP e `Status.Exposed` fino al Cleanup, cioe' +5 al PRIMO danno diretto.
	// Lo stato scade nel Cleanup dello stesso turno: si verifica quindi il suo EFFETTO — quanto incassa chi ha
	// corso allo scoperto — invece del tag residuo a turno finito, che per definizione non c'e' piu'.
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMap(World, /*Radius=*/ 8);

	// Due Ranger: il tiro (25 danni, portata 6, bersaglio singolo) non spinge e non fa area, quindi l'unica
	// differenza misurabile fra i due turni e' lo stato. Il Ranger non ha scudo: il danno si legge sugli HP.
	ARTUnit* Runner = SpawnHexUnit(World, 0, ERTArchetype::Ranger, FRTCellId(8, 0));
	ARTUnit* Foe = SpawnHexUnit(World, 1, ERTArchetype::Ranger, FRTCellId(0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Runner || !Foe) { DestroyHexMoveWorld(World); return false; }

	const int32 SprintIdx = AddSprintAbility(Runner);
	const int32 StartHealth = Runner->Health;

	// Sei celle di scatto: oltre la portata 5 dello scatto del Ranger, dentro gli 8 MP dello Sprint.
	Runner->PlannedCell = Runner->Cell; // nessun movimento normale pianificato
	Runner->PlannedDashAbility = SprintIdx;
	Runner->PlannedDashCell = FRTCellId(2, 0);

	// L'avversario resta fermo e tira su chi gli arriva davanti (Tiro, 25 danni, portata 6).
	Foe->PlannedCell = Foe->Cell;
	Foe->PlannedAbilityIndex = 0;
	Foe->PlannedAttackTarget = Runner;

	RunTurn(TM);

	if (!TestTrue(TEXT("lo Sprint copre 6 celle: il budget e' quello del catalogo (8 MP)"),
		Runner->Cell == FRTCellId(2, 0)))
	{
		DestroyHexMoveWorld(World);
		return false;
	}
	TestEqual(TEXT("chi ha sprintato incassa 25 + 5 dal primo colpo diretto"),
		StartHealth - Runner->Health, 30);

	// Controprova: stessa unita', stessa Spazzata, ma senza Sprint -> il danno torna nominale.
	Runner->PlaceOnCell(FRTCellId(2, 0), FVector::ZeroVector, 100.f, 250.f);
	Runner->ApplyCombatState(StartHealth, 0);
	Runner->PlannedCell = Runner->Cell;
	Runner->PlannedDashAbility = INDEX_NONE;
	Foe->PlannedCell = Foe->Cell;
	Foe->PlannedAbilityIndex = 0;
	Foe->PlannedAttackTarget = Runner;

	RunTurn(TM);

	TestEqual(TEXT("senza Sprint lo stesso colpo fa 25: il +5 viene dallo stato, non dall'attacco"),
		StartHealth - Runner->Health, 25);

	DestroyHexMoveWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSprintConsumesSlotsTest,
	"RefactorTactics.Actions.Sprint.ConsumesMovementAndMain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSprintConsumesSlotsTest::RunTest(const FString&)
{
	// Lo slot dichiarato dal catalogo (`Movimento + Principale`) e' una regola, non una nota: chi sprinta non
	// spara e non prosegue col Move nello stesso turno. Qui si pianifica di fare TUTTO — sprint, attacco e
	// movimento — e si verifica che restino solo gli 8 MP dello sprint.
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMap(World, /*Radius=*/ 8);

	ARTUnit* Runner = SpawnHexUnit(World, 0, ERTArchetype::Ranger, FRTCellId(0, 0));
	ARTUnit* Foe = SpawnHexUnit(World, 1, ERTArchetype::Guardian, FRTCellId(6, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Runner || !Foe) { DestroyHexMoveWorld(World); return false; }

	const int32 SprintIdx = AddSprintAbility(Runner);
	const int32 FoeHealth = Foe->Health;
	const int32 FoeShield = Foe->Shield;

	Runner->PlannedDashAbility = SprintIdx;
	Runner->PlannedDashCell = FRTCellId(3, 0);
	Runner->PlannedAbilityIndex = 0;          // Tiro (portata 6): da (3,0) il Guardian sarebbe a tiro
	Runner->PlannedAttackTarget = Foe;
	Runner->PlannedCell = FRTCellId(5, 0);    // e dopo lo scatto vorrebbe pure avanzare di due celle
	Foe->PlannedCell = Foe->Cell;

	RunTurn(TM);

	TestTrue(TEXT("il movimento e' finito con lo scatto: nessun Move oltre"), Runner->Cell == FRTCellId(3, 0));
	TestEqual(TEXT("l'azione principale e' spesa: il Guardian non viene colpito"), Foe->Health, FoeHealth);
	TestEqual(TEXT("nemmeno lo scudo del Guardian viene intaccato"), Foe->Shield, FoeShield);

	DestroyHexMoveWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIceSlidesInMatchTest,
	"RefactorTactics.Terrain.Ice.SlidesInMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIceSlidesInMatchTest::RunTest(const FString&)
{
	// Lo scivolamento su Ice e' verificato in isolamento da Terrain.Ice.*; qui si verifica che la PARTITA lo
	// applichi davvero — cioe' che ResolveMovement chiami ApplyIceSliding prima del microstep, e non dopo aver
	// gia' fissato la destinazione. Vale SOLO per il Move normale (lo Scatto non passa da qui, §5.2).
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	// Due lastre di ghiaccio sullo stesso asse est: (2,0) a portata di scivolata, (4,0) al limite del budget.
	// La cella di ARRIVO della scivolata, (3,0), e' Fuoco: la composizione fra due terreni si verifica qui,
	// non "per costruzione". La cella scivolata finisce in `FRTHexMoveResult::Entered` come qualunque altra,
	// quindi ApplyTerrainOnEnterEffects deve applicarle il danno del Fuoco senza sapere come ci si e' arrivati.
	// MoveCost resta 1 (override per-cella, non il 2 del catalogo): il fuoco cambia il DANNO, non i conti del
	// budget su cui poggiano le due controprove di soglia qui sotto.
	URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 6))
	{
		FRTHexCellData Data(Id);
		if (Id == FRTCellId(2, 0) || Id == FRTCellId(4, 0))
		{
			Data.Surface = ERTHexSurface::Ice; // costo 1 da catalogo: e' il budget RESIDUO a decidere
		}
		if (Id == FRTCellId(3, 0))
		{
			Data.Surface = ERTHexSurface::Fire;
		}
		Map->AddOrUpdateCell(Data);
	}
	Map->SortCells();
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Map;

	ARTUnit* Mover = SpawnHexUnit(World, 0, ERTArchetype::Ranger, FRTCellId(0, 0));
	// Un avversario fermo e lontano: senza, la squadra 1 e' gia' eliminata e il secondo lock-in non risolve.
	ARTUnit* Foe = SpawnHexUnit(World, 1, ERTArchetype::Guardian, FRTCellId(-5, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover || !Foe) { DestroyHexMoveWorld(World); return false; }

	if (!TestEqual(TEXT("il Ranger ha il budget standard"), Mover->GetEffectiveMoveRange(), 5))
	{
		DestroyHexMoveWorld(World);
		return false;
	}

	// Arrivo su (2,0): costo 2, residuo 3 >= 2 -> scivola di una cella nella direzione d'ingresso (est).
	const int32 StartHealth = Mover->Health;
	Mover->PlannedCell = FRTCellId(2, 0);
	Foe->PlannedCell = Foe->Cell;
	RunTurn(TM);

	TestTrue(TEXT("finire il Move sul ghiaccio porta una cella OLTRE la destinazione"),
		Mover->Cell == FRTCellId(3, 0));
	TestTrue(TEXT("posizione visiva = cella logica anche dopo la scivolata"),
		Mover->GetActorLocation().Equals(Mover->WorldForCell(Mover->Cell, FVector::ZeroVector, 100.f, 250.f), 1.0f));
	// Ghiaccio -> Fuoco: la cella in cui si SCIVOLA brucia come quella in cui si entra camminando. Nessuno ha
	// scritto la regola "ghiaccio piu' fuoco": e' la conseguenza di due terreni indipendenti sullo stesso path.
	// Il turno intero, non il solo ingresso: 10 dal Fuoco + gli 8 di `Status.Burning` nel Cleanup (CP 8.2).
	TestEqual(TEXT("10 danni dal Fuoco nella cella di arrivo della scivolata, piu' Burning"),
		Mover->Health, StartHealth - 10 - URTCombatLibrary::BurningCleanupDamage);
	TestTrue(TEXT("Burning applicato dalla cella scivolata"), Mover->HasStatus(TAG_Status_Burning));

	// Controprova: stesso ghiaccio, budget residuo insufficiente. Arrivo su (4,0) costa 4 dei 5 MP: resta 1,
	// sotto la soglia di 2 -> nessuna scivolata. E' il BUDGET a muoverla, non la superficie da sola.
	Mover->PlaceOnCell(FRTCellId(0, 0), FVector::ZeroVector, 100.f, 250.f);
	Mover->PlannedCell = FRTCellId(4, 0);
	Foe->PlannedCell = Foe->Cell;
	RunTurn(TM);

	TestTrue(TEXT("budget residuo < 2: si ferma sul ghiaccio senza scivolare"), Mover->Cell == FRTCellId(4, 0));

	DestroyHexMoveWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainFireDamagesAndBurnsOnEnterTest,
	"RefactorTactics.Terrain.Fire.DamagesAndBurnsOnEnter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainFireDamagesAndBurnsOnEnterTest::RunTest(const FString&)
{
	// Il Fuoco colpisce chi ATTRAVERSA la cella, non solo chi ci si ferma: gli OnEnterEffects del catalogo si
	// applicano a ogni cella di `FRTHexMoveResult::Entered`. Il percorso e' scritto a mano perche' l'A* ha piu'
	// rotte di pari costo fino a (2,0) e potrebbe aggirare il fuoco: con `PlannedCell` il test misurerebbe il
	// tie-break del pathfinding invece del danno da terreno.
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnHexMap(World, /*Radius=*/ 4);
	FRTHexCellData FireCell(FRTCellId(1, 0, 0));
	FireCell.Surface = ERTHexSurface::Fire;
	FireCell.MoveCost = 2; // costo del catalogo v0.1, come lo precompila il paint tool
	MapActor->MapAsset->AddOrUpdateCell(FireCell);
	MapActor->MapAsset->SortCells();

	ARTUnit* Mover = SpawnHexUnit(World, 0, ERTArchetype::Ranger, FRTCellId(0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover) { DestroyHexMoveWorld(World); return false; }

	const int32 StartHealth = Mover->Health;
	Mover->PlannedPath = { FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0) };
	Mover->PlannedCell = FRTCellId(2, 0);

	RunTurn(TM);

	TestTrue(TEXT("il fuoco non ferma il movimento: arriva a destinazione"), Mover->Cell == FRTCellId(2, 0));
	// 10 all'ingresso (CP 8.1) + 8 di `Status.Burning` nel Cleanup (CP 8.2): il turno costa 18 in tutto.
	TestEqual(TEXT("10 danni dal Fuoco, piu' gli 8 di Burning nel Cleanup"),
		Mover->Health, StartHealth - 10 - URTCombatLibrary::BurningCleanupDamage);
	TestTrue(TEXT("Burning applicato"), Mover->HasStatus(TAG_Status_Burning));

	DestroyHexMoveWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainFireDamagesOnDashTest,
	"RefactorTactics.Terrain.Fire.DamagesOnDash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainFireDamagesOnDashTest::RunTest(const FString&)
{
	// Stesso effetto per lo Scatto: passa da ResolveDash, non da ResolveMovement, ed e' l'altro punto in cui
	// un'unita' entra in una cella. Lo scatto e' LINEARE, quindi la rotta e' univoca e il fuoco e' per forza
	// attraversato (Fire non ha bBlocksDashCharge, a differenza di Rough).
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnHexMap(World, /*Radius=*/ 6);
	FRTHexCellData FireCell(FRTCellId(1, 0, 0));
	FireCell.Surface = ERTHexSurface::Fire;
	FireCell.MoveCost = 2;
	MapActor->MapAsset->AddOrUpdateCell(FireCell);
	MapActor->MapAsset->SortCells();

	ARTUnit* Runner = SpawnHexUnit(World, 0, ERTArchetype::Ranger, FRTCellId(0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Runner) { DestroyHexMoveWorld(World); return false; }

	const int32 DashIdx = Runner->FindDashAbilityIndex();
	if (!TestTrue(TEXT("il Ranger ha un'abilita' di scatto"), DashIdx != INDEX_NONE))
	{
		DestroyHexMoveWorld(World);
		return false;
	}

	const int32 StartHealth = Runner->Health;
	Runner->PlannedCell = Runner->Cell; // nessun Move normale: il danno puo' venire solo dallo scatto
	Runner->PlannedDashAbility = DashIdx;
	Runner->PlannedDashCell = FRTCellId(3, 0);

	RunTurn(TM);

	TestTrue(TEXT("lo scatto arriva a destinazione attraversando il fuoco"), Runner->Cell == FRTCellId(3, 0));
	TestEqual(TEXT("10 danni dal Fuoco anche in scatto, piu' Burning nel Cleanup"),
		Runner->Health, StartHealth - 10 - URTCombatLibrary::BurningCleanupDamage);
	TestTrue(TEXT("Burning applicato anche in scatto"), Runner->HasStatus(TAG_Status_Burning));

	DestroyHexMoveWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainFireErodesTemporaryShieldTest,
	"RefactorTactics.Terrain.Fire.ErodesTemporaryShieldFirst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainFireErodesTemporaryShieldTest::RunTest(const FString&)
{
	// Il danno da terreno passa dalla STESSA contabilita' del danno da azione (ARTUnit::ApplyCombatState):
	// lo scudo assorbito erode prima la parte TEMPORANEA. Scrivere Health/Shield a mano funzionerebbe sui
	// soli HP, ma lascerebbe TemporaryShield fermo al valore vecchio: nel Cleanup ExpireTemporaryShield lo
	// sottrarrebbe una seconda volta e l'unita' perderebbe scudo BASE mai consumato.
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnHexMap(World, /*Radius=*/ 4);
	FRTHexCellData FireCell(FRTCellId(1, 0, 0));
	FireCell.Surface = ERTHexSurface::Fire;
	FireCell.MoveCost = 2;
	MapActor->MapAsset->AddOrUpdateCell(FireCell);
	MapActor->MapAsset->SortCells();

	// Guardian: 20 di scudo base + 5 temporaneo = 25. I 10 danni del fuoco stanno tutti nello scudo.
	ARTUnit* Mover = SpawnHexUnit(World, 0, ERTArchetype::Guardian, FRTCellId(0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover) { DestroyHexMoveWorld(World); return false; }

	const int32 StartHealth = Mover->Health;
	Mover->AddTemporaryShield(5);
	Mover->PlannedAbilityIndex = INDEX_NONE; // nessuna azione: l'unica fonte di danno e' il terreno
	Mover->PlannedPath = { FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0) };
	Mover->PlannedCell = FRTCellId(2, 0);

	RunTurn(TM);

	TestEqual(TEXT("gli HP non calano: il danno del terreno lo assorbe lo scudo"), Mover->Health, StartHealth);
	// 25 - 10 (ingresso) - 8 (Burning nel Cleanup, CP 8.2) = 7, tutti di scudo BASE: i 5 temporanei sono stati
	// consumati per primi, quindi ExpireTemporaryShield non ne toglie altri. Anche il danno ambientale del
	// Cleanup passa dalla stessa contabilita': se ne saltasse una, questo numero sarebbe 12 o 2, non 7.
	TestEqual(TEXT("resta lo scudo BASE non consumato, non uno in meno"),
		Mover->Shield, 25 - 10 - URTCombatLibrary::BurningCleanupDamage);

	DestroyHexMoveWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainStatusLogMatchesStateTest,
	"RefactorTactics.Terrain.Status.LogMatchesState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainStatusLogMatchesStateTest::RunTest(const FString&)
{
	// Il combat log non deve raccontare cose non successe: se dice "Status.Wet da terreno", l'unita' deve
	// essere stata bagnata davvero.
	//
	// RISCRITTO AL CP 8.2. La versione precedente confrontava `bLoggedWet == HasStatus(Wet)` a fine turno, e
	// passava perche' erano entrambi FALSI: `Wet` aveva durata 0 e ApplyStatus scartava le durate <= 0.
	// Ora l'acqua bagna davvero e il Cleanup revoca lo stato a chi ne e' uscito, quindi log e stato finale si
	// riferiscono a due ISTANTI diversi e l'uguaglianza non e' piu' la proprieta' giusta: chi attraversa
	// l'acqua viene bagnato (il log dice il vero) e poi si asciuga (lo stato finale e' assente).
	// Il test copre entrambi gli esiti, cosi' non torna a essere una coerenza fra due assenze.
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnHexMap(World, /*Radius=*/ 4);
	FRTHexCellData WaterCell(FRTCellId(1, 0, 0));
	WaterCell.Surface = ERTHexSurface::ShallowWater;
	WaterCell.MoveCost = 2;
	MapActor->MapAsset->AddOrUpdateCell(WaterCell);
	MapActor->MapAsset->SortCells();

	ARTUnit* Mover = SpawnHexUnit(World, 0, ERTArchetype::Ranger, FRTCellId(0, 0));
	// Il test dura due turni: senza un avversario vivo il primo Cleanup dichiara vinta la partita
	// (`EvaluateOutcome(1, 0)`) e il secondo turno non verrebbe mai risolto. Sta fuori portata e fermo.
	ARTUnit* Foe = SpawnHexUnit(World, 1, ERTArchetype::Guardian, FRTCellId(4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover || !Foe) { DestroyHexMoveWorld(World); return false; }

	Foe->PlannedAbilityIndex = INDEX_NONE;
	Foe->PlannedCell = Foe->Cell;
	Mover->PlannedPath = { FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0) };
	Mover->PlannedCell = FRTCellId(2, 0);

	RunTurn(TM);

	// ANCORAGGIO: senza questo, il test passerebbe anche se l'unita' non fosse mai arrivata all'acqua e
	// ApplyTerrainOnEnterEffects non fosse mai stata chiamata.
	if (!TestTrue(TEXT("il Move e' davvero passato dall'acqua bassa e ha raggiunto la destinazione"),
		Mover->Cell == FRTCellId(2, 0)))
	{
		DestroyHexMoveWorld(World);
		return false;
	}

	auto LoggedWet = [](const ARTTurnManager* Manager)
	{
		for (const FString& Line : Manager->GetRecentEvents())
		{
			if (Line.Contains(TEXT("da terreno")) && Line.Contains(TEXT("Wet"))) { return true; }
		}
		return false;
	};

	// Caso "attraversa ed esce": l'evento e' avvenuto (il log lo dice) e lo stato e' stato revocato dal
	// Cleanup, perche' la cella d'arrivo non e' acqua.
	TestTrue(TEXT("attraversare l'acqua produce la riga di log"), LoggedWet(TM));
	TestFalse(TEXT("chi ne e' uscito non e' piu' bagnato a fine turno"), Mover->HasStatus(TAG_Status_Wet));

	// Caso "entra e resta": stessa riga di log, ma qui lo stato regge il Cleanup. Le due meta' insieme
	// escludono sia il log bugiardo sia lo stato che non si applica mai.
	Mover->PlannedAbilityIndex = INDEX_NONE;
	Mover->PlannedPath = { FRTCellId(2, 0), FRTCellId(1, 0) };
	Mover->PlannedCell = FRTCellId(1, 0);
	Foe->PlannedAbilityIndex = INDEX_NONE;
	Foe->PlannedPath.Reset();
	Foe->PlannedCell = Foe->Cell;
	RunTurn(TM);

	if (!TestEqual(TEXT("il secondo turno e' stato risolto: l'unita' e' nell'acqua"), Mover->Cell, FRTCellId(1, 0)))
	{
		DestroyHexMoveWorld(World); // altrimenti l'asserzione sullo stato direbbe solo che il turno non e' partito
		return false;
	}

	TestTrue(TEXT("fermarsi nell'acqua bagna, e il log lo dice"), LoggedWet(TM));
	TestTrue(TEXT("chi resta nell'acqua e' bagnato a fine turno"), Mover->HasStatus(TAG_Status_Wet));

	DestroyHexMoveWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovePathBlockedTest,
	"RefactorTactics.Actions.Move.PathBlocked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovePathBlockedTest::RunTest(const FString&)
{
	// Nome vincolante del catalogo v0.1 §15. `Fallback.Stop` in partita: il percorso si chiude a meta' strada
	// e l'unita' si ferma nell'ultima cella valida — non annulla il movimento, non aggira, non teletrasporta.
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMap(World, /*Radius=*/ 6);

	ARTUnit* Mover = SpawnHexUnit(World, 0, ERTArchetype::Ranger, FRTCellId(0, 0));
	ARTUnit* Blocker = SpawnHexUnit(World, 1, ERTArchetype::Guardian, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover || !Blocker) { DestroyHexMoveWorld(World); return false; }

	// Il percorso si scrive a mano, dritto attraverso la cella del blocker. Non e' un caso di laboratorio: e'
	// cio' che succede quando la strada era libera al momento di pianificarla e si chiude durante il turno.
	// Passando invece per `PlannedCell`, l'A* di pianificazione aggirerebbe l'ostacolo e non ci sarebbe alcun
	// percorso bloccato da verificare.
	Mover->PlannedPath = { FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0), FRTCellId(3, 0) };
	Mover->PlannedCell = FRTCellId(3, 0);
	Blocker->PlannedCell = Blocker->Cell;  // fermo: e' l'ostacolo

	RunTurn(TM);

	TestTrue(TEXT("si ferma prima dell'ostacolo, all'ultima cella valida"), Mover->Cell == FRTCellId(1, 0));
	TestTrue(TEXT("il movimento non viene annullato: qualche cella la percorre"), !(Mover->Cell == FRTCellId(0, 0)));
	TestTrue(TEXT("e non arriva a destinazione aggirando"), !(Mover->Cell == FRTCellId(3, 0)));

	// L'esito e' registrato col suo motivo: e' la forma che `Fallback.Stop` prende nel TurnLog del movimento.
	int32 Stopped = 0;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category == ERTLogCategory::Move && E.Outcome == static_cast<uint8>(ERTMoveOutcome::BlockedByUnit))
		{
			++Stopped;
		}
	}
	TestEqual(TEXT("il TurnLog dice che si e' fermata per una cella occupata"), Stopped, 1);

	DestroyHexMoveWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMoveCellConflictTest,
	"RefactorTactics.Actions.Move.CellConflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMoveCellConflictTest::RunTest(const FString&)
{
	// Nome vincolante del catalogo v0.1 (CP 4.8). Stessa cella, stessa priorita' (nessuna delle due dichiara
	// un'azione con precedenza sull'altra): entrambe si fermano nella cella precedente, nessuna la ottiene.
	// Equivalente per contenuto a `HexMove.ContestedCellStopsBoth` (H6), qui sotto il nome del catalogo.
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMap(World, /*Radius=*/ 4);

	const FRTCellId Contested(0, 0);
	ARTUnit* A = SpawnHexUnit(World, 0, ERTArchetype::Ranger, FRTCellId(-2, 0));
	ARTUnit* B = SpawnHexUnit(World, 1, ERTArchetype::Ranger, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !A || !B) { DestroyHexMoveWorld(World); return false; }

	A->PlannedCell = Contested;
	B->PlannedCell = Contested;

	RunTurn(TM);

	// Equidistanti (2 celle ciascuna): la contesa scatta all'ultimo passo, non al primo.
	TestTrue(TEXT("A si ferma nella cella precedente"), A->Cell == FRTCellId(-1, 0));
	TestTrue(TEXT("B si ferma nella cella precedente"), B->Cell == FRTCellId(1, 0));
	TestTrue(TEXT("nessuna delle due occupa la cella contesa"), A->Cell != Contested && B->Cell != Contested);

	int32 Contests = 0;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category == ERTLogCategory::Move && E.Outcome == static_cast<uint8>(ERTMoveOutcome::BlockedContested))
		{
			++Contests;
		}
	}
	TestEqual(TEXT("il TurnLog registra due esiti di contesa"), Contests, 2);

	DestroyHexMoveWorld(World);
	return true;
}
