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
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"

// La guardia: senza, i test di questo file finiscono compilati DENTRO il binario Shipping che si
// distribuisce. Non e' una formalita' di build — e' cio' che tiene il codice di test fuori dal gioco.
// Aggiunta con #923, dopo che lo stesso difetto ha rotto la build Shipping due volte (2026-08-09 e
// 2026-08-15) senza che la suite, che gira sul target Editor, se ne accorgesse.
#if WITH_DEV_AUTOMATION_TESTS

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

	/**
	 * Dove la cella logica impone che l'unita' si veda, **alla scala che il mondo usa davvero**.
	 *
	 * 🔴 **Questi confronti avevano `100.f` e `250.f` scritti a mano, e sono caduti quando `#1155` ha
	 * portato `HexSize` a `150`.** Non era un difetto del gioco: il TurnManager legge la scala dalla mappa
	 * (`GetHexContext`) e la posizione era giusta — era l'ATTESA a essere calcolata in un mondo diverso.
	 * Riscriverla come `150.f` avrebbe solo spostato la stessa trappola al prossimo cambio di scala: qui
	 * l'attesa si chiede alla stessa sorgente del codice sotto misura.
	 */
	FVector DoveLaCellaImpone(const ARTUnit* Unit, const FRTCellId& Cell, const ARTHexMapActor* HexMap)
	{
		FVector Origin = FVector::ZeroVector;
		float HexSize = 0.f;
		float LayerHeight = 0.f;
		HexMap->GetHexContext(Origin, HexSize, LayerHeight);
		return Unit->WorldForCell(Cell, Origin, HexSize, LayerHeight);
	}

	ARTUnit* SpawnHexUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // i piani li scriviamo noi: niente decisioni del bot in mezzo
		U->ConfigureFromHeroData(Hero);
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
	ARTHexMapActor* HexMap = SpawnHexMap(World, /*Radius=*/ 4);

	ARTUnit* Mover = SpawnHexUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
	ARTUnit* Foe = SpawnHexUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover || !Foe) { DestroyHexMoveWorld(World); return false; }

	// Due celle a est lungo l'asse assiale: entro il budget, nessun ostacolo.
	const FRTCellId Goal(2, -1);
	Mover->PlannedCell = Goal;
	Foe->PlannedCell = Foe->Cell; // fermo

	RunTurn(TM);

	TestTrue(TEXT("l'unita' e' sulla cella pianificata"), Mover->Cell == Goal);

	// Nessuna deriva: la posizione visiva coincide con quella che la cella logica impone.
	const FVector Expected = DoveLaCellaImpone(Mover, Mover->Cell, HexMap);
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

	ARTUnit* Mover = SpawnHexUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
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

	ARTUnit* Mover = SpawnHexUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
	// Un avversario lontano e fermo: senza, la squadra 1 e' gia' eliminata e la partita finisce al primo
	// turno (MatchEnded), quindi il secondo lock-in non risolverebbe nulla.
	ARTUnit* Foe = SpawnHexUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(-5, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover || !Foe) { DestroyHexMoveWorld(World); return false; }
	Foe->PlannedCell = Foe->Cell;

	// Sul corridoio costoso ogni cella vale 2: col budget B si arriva a B/2 celle e non a una in piu'.
	// Era scritto «budget 5, quindi 2 celle e non 3»: i numeri del Ranger legacy. Derivarli tiene in piedi
	// la proprieta' — e' il COSTO a fermare, non la distanza — con qualunque eroe cammini.
	const int32 Budget = Mover->GetEffectiveMoveRange();
	const int32 Reachable = Budget / 2;
	const FRTCellId TooFar(Reachable + 1, 0);
	TestTrue(TEXT("premessa: la cella scelta costa piu' del budget"), 2 * (Reachable + 1) > Budget);

	Mover->PlannedCell = TooFar;
	RunTurn(TM);
	TestTrue(TEXT("una cella di troppo di terreno difficile costa piu' del budget: non ci arriva"),
		!(Mover->Cell == TooFar));
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
	ARTHexMapActor* HexMap = SpawnHexMap(World, /*Radius=*/ 6);

	// Destinazione OBLIQUA (3,-3): distanza ESAGONALE 3, dentro la portata 5 dello scatto — ma distanza di
	// Manhattan 6 e coordinate negative, quindi irraggiungibile per il pathfinding quadrato. E' il caso che
	// distingue le due geometrie: se lo scatto girasse ancora sul quadrato, l'unita' resterebbe ferma.
	ARTUnit* Runner = SpawnHexUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
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
		Runner->GetActorLocation().Equals(DoveLaCellaImpone(Runner, Goal, HexMap), 1.0f));

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

	ARTUnit* Runner = SpawnHexUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
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
	ARTUnit* A = SpawnHexUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(1, 0));
	ARTUnit* B = SpawnHexUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(-1, 0));
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
	ARTUnit* Runner = SpawnHexUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(8, 0));
	ARTUnit* Foe = SpawnHexUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
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
	// Il colpo pieno lo dichiara l'attacco base di chi spara: la proprieta' e' «Exposed aggiunge 5 al primo
	// colpo diretto», non «il colpo fa 30». Prima erano 25 + 5, cioe' il danno del Ranger legacy.
	const int32 FullHit = Foe->AttackPower;
	TestEqual(TEXT("chi ha sprintato incassa FullHit + 5 dal primo colpo diretto"),
		StartHealth - Runner->Health, FullHit + 5);

	// Controprova: stessa unita', stesso colpo, ma senza Sprint -> il danno torna nominale.
	Runner->PlaceOnCell(FRTCellId(2, 0), FVector::ZeroVector, 100.f, 250.f);
	Runner->ApplyCombatState(StartHealth, 0);
	Runner->PlannedCell = Runner->Cell;
	Runner->PlannedDashAbility = INDEX_NONE;
	Foe->PlannedCell = Foe->Cell;
	Foe->PlannedAbilityIndex = 0;
	Foe->PlannedAttackTarget = Runner;

	RunTurn(TM);

	TestEqual(TEXT("senza Sprint lo stesso colpo arriva nominale: il +5 viene dallo stato, non dall'attacco"),
		StartHealth - Runner->Health, FullHit);

	DestroyHexMoveWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSprintConsumesSlotsTest,
	"RefactorTactics.Actions.Sprint.ConsumesOnlyMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSprintConsumesSlotsTest::RunTest(const FString&)
{
	// D-028: lo `Sprint` occupa il SOLO slot movimento. Chi sprinta non prosegue col Move — quello slot e'
	// speso — ma l'azione principale gli resta, quindi spara. Qui si pianifica di fare TUTTO (sprint, attacco
	// e movimento) e si verifica che passino sprint e attacco, non il Move.
	//
	// Prima di D-028 lo slot era `Movimento + Principale` e questo test verificava l'opposto sull'attacco. Non
	// era sbagliato: era la regola di allora.
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMap(World, /*Radius=*/ 8);

	ARTUnit* Runner = SpawnHexUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
	ARTUnit* Foe = SpawnHexUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(6, 0));
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
	// L'azione principale NON e' spesa dallo sprint: il colpo parte da (3,0), dove il Guardian e' a tiro.
	TestTrue(TEXT("l'azione principale resta: il Guardian viene colpito"),
		(Foe->Health + Foe->Shield) < (FoeHealth + FoeShield));

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

	ARTUnit* Mover = SpawnHexUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
	// Un avversario fermo e lontano: senza, la squadra 1 e' gia' eliminata e il secondo lock-in non risolve.
	ARTUnit* Foe = SpawnHexUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(-5, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover || !Foe) { DestroyHexMoveWorld(World); return false; }

	// Il budget si legge: la soglia di scivolata e' «residuo >= 2», e serve un budget che permetta di
	// osservare ENTRAMBI i lati — con residuo sufficiente scivola, con residuo 1 no. Era fissato a 5, il
	// valore del Ranger legacy, e i due arrivi sotto erano calcolati su quello.
	const int32 IceBudget = Mover->GetEffectiveMoveRange();
	if (!TestTrue(TEXT("premessa: il budget distingue i due casi"), IceBudget >= 4))
	{
		DestroyHexMoveWorld(World);
		return false;
	}

	// Arrivo su (2,0): costo 2, residuo IceBudget-2 >= 2 -> scivola di una cella nella direzione d'ingresso.
	const int32 StartHealth = Mover->Health;
	Mover->PlannedCell = FRTCellId(2, 0);
	Foe->PlannedCell = Foe->Cell;
	RunTurn(TM);

	TestTrue(TEXT("finire il Move sul ghiaccio porta una cella OLTRE la destinazione"),
		Mover->Cell == FRTCellId(3, 0));
	TestTrue(TEXT("posizione visiva = cella logica anche dopo la scivolata"),
		Mover->GetActorLocation().Equals(DoveLaCellaImpone(Mover, Mover->Cell, MapActor), 1.0f));
	// Ghiaccio -> Fuoco: la cella in cui si SCIVOLA brucia come quella in cui si entra camminando. Nessuno ha
	// scritto la regola "ghiaccio piu' fuoco": e' la conseguenza di due terreni indipendenti sullo stesso path.
	// Il turno intero, non il solo ingresso: 10 dal Fuoco + gli 8 di `Status.Burning` nel Cleanup (CP 8.2).
	TestEqual(TEXT("10 danni dal Fuoco nella cella di arrivo della scivolata, piu' Burning"),
		Mover->Health, StartHealth - 10 - URTCombatLibrary::BurningCleanupDamage);
	TestTrue(TEXT("Burning applicato dalla cella scivolata"), Mover->HasStatus(TAG_Status_Burning));

	// Controprova: stesso ghiaccio, budget residuo insufficiente. Arrivando a IceBudget-1 celle resta 1 MP,
	// sotto la soglia di 2 -> nessuna scivolata. E' il BUDGET a muoverla, non la superficie da sola.
	//
	// La cella d'arrivo dev'essere GHIACCIO, e va marcata qui perche' la sua posizione dipende dal budget:
	// la mappa dichiara ghiaccio su (2,0) e (4,0), scelte quando il budget era 5. Con un budget diverso
	// l'arrivo cade su una cella normale, `ApplyIceSliding` esce alla prima riga (nessun `SlideCells`) e
	// l'unita' si ferma li' perche' non c'e' ghiaccio — non perche' il residuo sia insufficiente. Il test
	// resterebbe verde anche cancellando la soglia che esiste per verificare.
	const FRTCellId NoSlideTarget(IceBudget - 1, 0);
	{
		FRTHexCellData IceData(NoSlideTarget);
		IceData.Surface = ERTHexSurface::Ice;
		Map->AddOrUpdateCell(IceData);
		Map->SortCells();
	}
	const FRTHexCellData* ArrivalData = Map->FindCell(NoSlideTarget);
	TestTrue(TEXT("premessa: la controprova arriva SU GHIACCIO"),
		ArrivalData != nullptr && ArrivalData->Surface == ERTHexSurface::Ice);
	Mover->PlaceOnCell(FRTCellId(0, 0), FVector::ZeroVector, 100.f, 250.f);
	Mover->PlannedCell = NoSlideTarget;
	Foe->PlannedCell = Foe->Cell;
	RunTurn(TM);

	TestTrue(TEXT("budget residuo < 2: si ferma sul ghiaccio senza scivolare"), Mover->Cell == NoSlideTarget);

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

	ARTUnit* Mover = SpawnHexUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
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

	ARTUnit* Runner = SpawnHexUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
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

	// Scudo base + temporaneo: i 10 danni del fuoco stanno tutti nello scudo.
	//
	// Lo scudo base lo dichiara il TEST, non l'eroe: `ConfigureFromHeroData` non imposta `Shield` e nessun
	// eroe del roster ne parte fornito — il Guardian legacy, con i suoi 20, era l'unico. La proprieta' in
	// esame e' la CONTABILITA' (il temporaneo si consuma per primo, e anche il danno ambientale del Cleanup
	// passa di li'), non da dove arriva lo scudo: darglielo qui la tiene verificabile.
	ARTUnit* Mover = SpawnHexUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover) { DestroyHexMoveWorld(World); return false; }

	constexpr int32 BaseShield = 20;
	Mover->ApplyCombatState(Mover->Health, BaseShield);
	const int32 StartHealth = Mover->Health;
	Mover->AddTemporaryShield(5);
	Mover->PlannedAbilityIndex = INDEX_NONE; // nessuna azione: l'unica fonte di danno e' il terreno
	Mover->PlannedPath = { FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0) };
	Mover->PlannedCell = FRTCellId(2, 0);

	RunTurn(TM);

	TestEqual(TEXT("gli HP non calano: il danno del terreno lo assorbe lo scudo"), Mover->Health, StartHealth);
	// (base + 5) - 10 (ingresso) - 8 (Burning nel Cleanup, CP 8.2), tutti di scudo BASE: i 5 temporanei sono
	// stati consumati per primi, quindi ExpireTemporaryShield non ne toglie altri. Anche il danno ambientale
	// del Cleanup passa dalla stessa contabilita': se ne saltasse una, questo numero sarebbe piu' alto o piu'
	// basso di una delle due voci.
	TestEqual(TEXT("resta lo scudo BASE non consumato, non uno in meno"),
		Mover->Shield, (BaseShield + 5) - 10 - URTCombatLibrary::BurningCleanupDamage);

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

	ARTUnit* Mover = SpawnHexUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
	// Il test dura due turni: senza un avversario vivo il primo Cleanup dichiara vinta la partita
	// (`EvaluateOutcome(1, 0)`) e il secondo turno non verrebbe mai risolto. Sta fuori portata e fermo.
	ARTUnit* Foe = SpawnHexUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(4, 0));
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

// ⚠️ `ClientContext` oltre a `EditorContext`, e non per simmetria con i vicini: e' uno dei **dieci test
// vincolanti** del catalogo (`roadmap-v0.1.md` §6), e CP 12.3 (`#83`) chiede che la suite giri anche in un
// pacchetto. Con il solo `EditorContext` il controller lo filtra fuori dal target `Game` — misurato:
// `Automation List` in un pacchetto Development registrava **zero** test su 875, tutti dichiarati solo Editor.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovePathBlockedTest,
	"RefactorTactics.Actions.Move.PathBlocked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FRTMovePathBlockedTest::RunTest(const FString&)
{
	// Nome vincolante del catalogo v0.1 §15. `Fallback.Stop` in partita: il percorso si chiude a meta' strada
	// e l'unita' si ferma nell'ultima cella valida — non annulla il movimento, non aggira, non teletrasporta.
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMap(World, /*Radius=*/ 6);

	ARTUnit* Mover = SpawnHexUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
	ARTUnit* Blocker = SpawnHexUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(2, 0));
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

// ⚠️ `ClientContext` oltre a `EditorContext`, e non per simmetria con i vicini: e' uno dei **dieci test
// vincolanti** del catalogo (`roadmap-v0.1.md` §6), e CP 12.3 (`#83`) chiede che la suite giri anche in un
// pacchetto. Con il solo `EditorContext` il controller lo filtra fuori dal target `Game` — misurato:
// `Automation List` in un pacchetto Development registrava **zero** test su 875, tutti dichiarati solo Editor.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMoveCellConflictTest,
	"RefactorTactics.Actions.Move.CellConflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FRTMoveCellConflictTest::RunTest(const FString&)
{
	// Nome vincolante del catalogo v0.1 (CP 4.8). Stessa cella, stessa priorita' (nessuna delle due dichiara
	// un'azione con precedenza sull'altra): entrambe si fermano nella cella precedente, nessuna la ottiene.
	// Equivalente per contenuto a `HexMove.ContestedCellStopsBoth` (H6), qui sotto il nome del catalogo.
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMap(World, /*Radius=*/ 4);

	const FRTCellId Contested(0, 0);
	ARTUnit* A = SpawnHexUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(-2, 0));
	ARTUnit* B = SpawnHexUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(2, 0));
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



IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDashConsumesMovementTest,
	"RefactorTactics.Actions.Dash.ConsumesTheMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDashConsumesMovementTest::RunTest(const FString&)
{
	// L'altra meta': schivato, il movimento e' speso. Un Move pianificato oltre lo scatto NON avviene.
	// Prima di D-028 avveniva, ed era la decisione «dash + move» di spec-dash.md — ora superata.
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMap(World, /*Radius=*/ 8);

	ARTUnit* Runner = SpawnHexUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
	ARTUnit* Foe = SpawnHexUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(8, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Runner || !Foe) { DestroyHexMoveWorld(World); return false; }

	URTActionData* Dash = NewObject<URTActionData>(Runner);
	Dash->DisplayName = FText::FromString(TEXT("Scatto"));
	Dash->Def = URTCatalogLibrary::FindCoreAction(TEXT("Action.Dash"));
	Runner->Abilities.Add(Dash);

	Runner->PlannedDashAbility = Runner->Abilities.Num() - 1;
	Runner->PlannedDashCell = FRTCellId(3, 0);
	Runner->PlannedCell = FRTCellId(5, 0);   // e dopo lo scatto vorrebbe avanzare di due celle
	Foe->PlannedCell = Foe->Cell;

	RunTurn(TM);

	TestTrue(TEXT("si finisce dove ha portato lo scatto, non oltre"), Runner->Cell == FRTCellId(3, 0));

	DestroyHexMoveWorld(World);
	return true;
}


// =====================================================================================================
// L'ECCEZIONE dichiarata da un kit, non dalla regola generale (D-028).
//
// Dopo la migrazione degli slot nessuna azione di serie usa `MovementAndMain`, e il ramo del resolver che
// lo gestisce sarebbe diventato codice che nessun dato attraversa. Questo test costruisce il dato: una
// mobilita' che dichiara di costare ENTRAMBI gli slot, come farebbe il kit di un eroe.
//
// Non e' un RED — il meccanismo esisteva gia' ed era cio' che rendeva `Action.Sprint` speciale prima di
// D-028. E' la copertura di un ramo rimasto scoperto dalla migrazione: senza, la prossima persona che lo
// trova morto lo cancella, e con lui il modo di esprimere l'eccezione senza un `if` sull'ActionId.
// =====================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKitDeclaredBothSlotsTest,
	"RefactorTactics.Actions.KitCanDeclareAMobilityThatCostsBothSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKitDeclaredBothSlotsTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMap(World, /*Radius=*/ 8);

	ARTUnit* Runner = SpawnHexUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Runner) { DestroyHexMoveWorld(World); return false; }

	// Il bersaglio va messo DENTRO la portata dell'attacco base, misurata dalla cella d'arrivo dello scatto:
	// e' l'unica posizione in cui «nessun colpo» dimostra qualcosa. Stava a (8,0), a distanza 5 dall'arrivo:
	// con la portata 6 del Ranger legacy era un bersaglio legittimo, con la portata 4 del roster e' fuori
	// tiro comunque — e il test sarebbe rimasto verde anche cancellando la gestione di `MovementAndMain`,
	// che e' esattamente cio' che esiste per difendere.
	constexpr int32 DashTo = 3;
	const int32 ShotRange = Runner->AttackRange;
	ARTUnit* Foe = SpawnHexUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(DashTo + ShotRange, 0));
	if (!TestNotNull(TEXT("Foe"), Foe)) { DestroyHexMoveWorld(World); return false; }
	TestTrue(TEXT("premessa: dalla cella d'arrivo il bersaglio E' a portata"),
		URTHexLibrary::HexDistance(FRTCellId(DashTo, 0), Foe->Cell) <= ShotRange);

	// Uno scatto identico ad `Action.Dash` TRANNE lo slot: e' il kit a dichiarare che costa tutto il turno.
	URTActionData* Costly = NewObject<URTActionData>(Runner);
	Costly->DisplayName = FText::FromString(TEXT("Scatto totale"));
	Costly->Def = URTCatalogLibrary::FindCoreAction(TEXT("Action.Dash"));
	Costly->Def.ActionId = FName(TEXT("Hero.CostlyDash"));
	Costly->Def.Slot = ERTActionSlot::MovementAndMain;
	Runner->Abilities.Add(Costly);

	const int32 FoeBefore = Foe->Health + Foe->Shield;

	Runner->PlannedDashAbility = Runner->Abilities.Num() - 1;
	Runner->PlannedDashCell = FRTCellId(DashTo, 0);
	Runner->PlannedAbilityIndex = 0;          // l'attacco base, che dalla cella d'arrivo avrebbe il bersaglio a portata
	Runner->PlannedAttackTarget = Foe;
	Foe->PlannedCell = Foe->Cell;

	RunTurn(TM);

	TestTrue(TEXT("lo scatto e' avvenuto"), Runner->Cell == FRTCellId(DashTo, 0));
	// La differenza con `Action.Dash`, e l'unica cosa che questo test dimostra: qui il colpo NON parte.
	TestEqual(TEXT("il kit ha dichiarato che costa anche la principale: nessun colpo"),
		Foe->Health + Foe->Shield, FoeBefore);

	DestroyHexMoveWorld(World);
	return true;
}


/**
 * CP 16.1 / D-020 — la ROTAZIONE DICHIARATA in un turno vero (#291).
 *
 * `URTFacingLibrary::TryApplyDeclaredFacing` esisteva ed era testata, ma **nessuno la chiamava**: in tutto il
 * progetto la invocavano solo i test della libreria pura. Il caso «resto fermo e mi giro» non era esprimibile,
 * e un'unita' che non si muoveva teneva l'orientamento del turno prima.
 *
 * Questi tre test coprono il CABLAGGIO, non le regole: che la dichiarazione arrivi dal piano al resolver, che
 * un rifiuto sia osservabile nel TurnLog, e che una dichiarazione non sopravviva al proprio turno. E' il
 * difetto ricorrente di questo repository — libreria coperta, cablaggio scoperto.
 */
namespace
{
	/** Quante voci di orientamento con quell'esito ci sono nel TurnLog. */
	int32 CountFacingOutcome(const ARTTurnManager* TM, ERTFacingOutcome Outcome)
	{
		int32 N = 0;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Facing && E.Outcome == static_cast<uint8>(Outcome)) { ++N; }
		}
		return N;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMoveStationaryDeclaredRotationTest,
	"RefactorTactics.HexMove.StationaryUnitAppliesDeclaredRotation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMoveStationaryDeclaredRotationTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMap(World, /*Radius=*/ 4);

	ARTUnit* Sentry = SpawnHexUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0));
	ARTUnit* Foe = SpawnHexUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Sentry || !Foe) { DestroyHexMoveWorld(World); return false; }

	Sentry->Facing = ERTHexDirection::E;
	Sentry->PlannedCell = Sentry->Cell; // ferma: sei direzioni legali
	Foe->PlannedCell = Foe->Cell;

	// La dichiarazione: resto dove sono e mi giro a SW. Senza il consumo nel resolver questo campo non
	// arriverebbe da nessuna parte e l'unita' finirebbe il turno guardando ancora a E.
	Sentry->bDeclaresPlannedFacing = true;
	Sentry->PlannedFacing = ERTHexDirection::SW;

	RunTurn(TM);

	TestTrue(TEXT("chi non si muove puo' girarsi: il facing e' quello dichiarato"),
		Sentry->Facing == ERTHexDirection::SW);
	TestTrue(TEXT("l'unita' non si e' spostata"), Sentry->Cell == FRTCellId(0, 0));
	TestEqual(TEXT("e la dichiarazione e' registrata nel TurnLog"),
		CountFacingOutcome(TM, ERTFacingOutcome::DeclaredInPlanning), 1);
	TestEqual(TEXT("nessun rifiuto: era legale"),
		CountFacingOutcome(TM, ERTFacingOutcome::DeclarationRejected), 0);

	DestroyHexMoveWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMoveIllegalDeclaredRotationTest,
	"RefactorTactics.HexMove.IllegalDeclaredRotationIsRejectedInPlayedTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMoveIllegalDeclaredRotationTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMap(World, /*Radius=*/ 4);

	ARTUnit* Mover = SpawnHexUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
	ARTUnit* Foe = SpawnHexUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 3));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover || !Foe) { DestroyHexMoveWorld(World); return false; }

	// Un passo a EST: dopo un Move a budget le direzioni legali sono l'ultimo passo e le due adiacenti nel
	// ciclo — `E`, `NE`, `SE`. `W` e' l'opposta esatta, quindi illegale per costruzione e non per caso.
	const FRTCellId Goal = URTHexLibrary::Neighbor(FRTCellId(0, 0), ERTHexDirection::E);
	Mover->Facing = ERTHexDirection::NW; // di partenza: cosi' «rifiutata» non coincide con «invariata»
	Mover->PlannedCell = Goal;
	Foe->PlannedCell = Foe->Cell;

	Mover->bDeclaresPlannedFacing = true;
	Mover->PlannedFacing = ERTHexDirection::W;

	RunTurn(TM);

	TestTrue(TEXT("il movimento e' avvenuto"), Mover->Cell == Goal);
	TestTrue(TEXT("la rotazione illegale NON e' stata applicata"), Mover->Facing != ERTHexDirection::W);
	TestTrue(TEXT("e vale l'orientamento DERIVATO dal movimento, non quello di partenza"),
		Mover->Facing == ERTHexDirection::E);
	TestEqual(TEXT("il rifiuto e' osservabile: senza voce sarebbe indistinguibile da una dichiarazione mai fatta"),
		CountFacingOutcome(TM, ERTFacingOutcome::DeclarationRejected), 1);
	TestEqual(TEXT("e non e' stata registrata nessuna dichiarazione accolta"),
		CountFacingOutcome(TM, ERTFacingOutcome::DeclaredInPlanning), 0);

	DestroyHexMoveWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMoveDeclarationDoesNotSurviveTurnTest,
	"RefactorTactics.HexMove.DeclaredRotationDoesNotSurviveItsTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMoveDeclarationDoesNotSurviveTurnTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMap(World, /*Radius=*/ 4);

	ARTUnit* Sentry = SpawnHexUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0));
	ARTUnit* Foe = SpawnHexUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Sentry || !Foe) { DestroyHexMoveWorld(World); return false; }

	Sentry->Facing = ERTHexDirection::E;
	Sentry->PlannedCell = Sentry->Cell;
	Foe->PlannedCell = Foe->Cell;
	Sentry->bDeclaresPlannedFacing = true;
	Sentry->PlannedFacing = ERTHexDirection::SW;

	RunTurn(TM);
	TestTrue(TEXT("primo turno: la rotazione si applica"), Sentry->Facing == ERTHexDirection::SW);
	TestFalse(TEXT("e la dichiarazione e' consumata"), Sentry->bDeclaresPlannedFacing);

	TestEqual(TEXT("registrata una volta, nel turno in cui e' stata fatta"),
		CountFacingOutcome(TM, ERTFacingOutcome::DeclaredInPlanning), 1);

	// Secondo turno senza dichiarare nulla. Una rotazione dichiarata e' un pezzo del PIANO, e i piani non
	// sopravvivono al proprio turno: se questo campo restasse acceso, l'unita' continuerebbe a «ridichiarare»
	// per sempre l'ultima direzione scelta, e in ogni TurnLog comparirebbe una decisione che nessuno ha preso.
	//
	// Il confronto e' con ZERO, non col conteggio del turno prima: `TurnLog` viene azzerato a ogni turno
	// (`ARTTurnManager::LockInAndResolve`), quindi qui si legge il log del SOLO secondo turno.
	Sentry->PlannedCell = Sentry->Cell;
	Foe->PlannedCell = Foe->Cell;
	RunTurn(TM);

	TestTrue(TEXT("secondo turno: l'orientamento resta quello raggiunto"), Sentry->Facing == ERTHexDirection::SW);
	TestEqual(TEXT("e nel log del secondo turno non c'e' nessuna dichiarazione"),
		CountFacingOutcome(TM, ERTFacingOutcome::DeclaredInPlanning), 0);
	TestEqual(TEXT("ne' un rifiuto: non e' stato dichiarato niente, non e' stato rifiutato niente"),
		CountFacingOutcome(TM, ERTFacingOutcome::DeclarationRejected), 0);

	DestroyHexMoveWorld(World);
	return true;
}

/**
 * La causa di uno spostamento e' leggibile nel TurnLog dopo un TURNO VERO (#307).
 *
 * Il DoD della issue chiede esplicitamente «un test che lo dimostri su un turno vero, non sulla libreria»:
 * e' il difetto ricorrente di questo repository — libreria coperta, cablaggio scoperto.
 *
 * `ActionId` esiste gia' nella voce, e' gia' serializzato e gia' entra in `HashTurnLog`: la causa non costa
 * una migrazione di formato. Restano da dichiarare le cause dello scatto e dello spostamento forzato, che
 * hanno produttori diversi da `BuildMoveLog`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMoveLogCauseInMatchTest,
	"RefactorTactics.HexMove.LogDeclaresMoveCause",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMoveLogCauseInMatchTest::RunTest(const FString&)
{
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMap(World, /*Radius=*/ 4);

	ARTUnit* Mover = SpawnHexUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
	ARTUnit* Foe = SpawnHexUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Mover || !Foe) { DestroyHexMoveWorld(World); return false; }

	const FRTCellId Goal(2, -1);
	Mover->PlannedCell = Goal;
	Foe->PlannedCell = Foe->Cell;

	RunTurn(TM);

	int32 MoveEntries = 0;
	for (const FRTTurnLogEntry& Entry : TM->GetTurnLog())
	{
		if (Entry.Category != ERTLogCategory::Move)
		{
			continue;
		}
		++MoveEntries;
		TestEqual(TEXT("la voce di movimento dichiara il movimento volontario"),
			Entry.ActionId, FName(TEXT("Action.Move")));
	}

	// Senza questa, il ciclo sopra passerebbe su una lista vuota: un TurnLog senza voci Move renderebbe
	// il test verde senza aver verificato nulla.
	TestTrue(TEXT("il turno ha prodotto almeno una voce di movimento"), MoveEntries > 0);

	DestroyHexMoveWorld(World);
	return true;
}

// =====================================================================================================
// D-028 — lo scatto e' movimento: si schiva e si spara, oppure si spara e ci si muove.
//
// Sono le due meta' della stessa regola, e vanno verificate separate: un solo test che le mescola
// passerebbe anche se il Dash consumasse gli slot sbagliati in modo compensato.
// =====================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDashLeavesMainAvailableTest,
	"RefactorTactics.Actions.Dash.LeavesMainAvailable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDashLeavesMainAvailableTest::RunTest(const FString&)
{
	// Schivo e sparo: lo scatto avvicina PRIMA del Blast, e l'attacco parte dalla posizione nuova.
	//
	// Questo test passava GIA' prima della migrazione degli slot, ed e' il fatto piu' importante di D-028: il
	// gioco faceva la cosa giusta con la regola sbagliata scritta accanto (`Dash` dichiarato `Main`, e
	// `ValidateActionSlots` mai chiamata in partita). Non e' un RED, e' caratterizzazione — esiste per
	// impedire che la migrazione porti via il comportamento buono insieme alla regola cattiva.
	UWorld* World = MakeHexMoveWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexMap(World, /*Radius=*/ 8);

	ARTUnit* Runner = SpawnHexUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Runner) { DestroyHexMoveWorld(World); return false; }

	// Il bersaglio si posiziona sul CONFINE: fuori portata dalla partenza, dentro dopo lo scatto. Era a
	// (8,0), a distanza 5 dall'arrivo — dentro la portata 6 del Ranger legacy, fuori dalla 4 del roster, e
	// il colpo non partiva nemmeno dopo lo scatto. Derivandola, la premessa che rende il test discriminante
	// («senza lo scatto il colpo non partirebbe») resta vera con qualunque eroe.
	constexpr int32 DashTo = 3;
	const int32 ShotRange = Runner->AttackRange;
	ARTUnit* Foe = SpawnHexUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(DashTo + ShotRange, 0));
	if (!TestNotNull(TEXT("Foe"), Foe)) { DestroyHexMoveWorld(World); return false; }
	TestTrue(TEXT("premessa: dalla PARTENZA il bersaglio e' fuori portata"),
		URTHexLibrary::HexDistance(FRTCellId(0, 0), Foe->Cell) > ShotRange);
	TestTrue(TEXT("premessa: dopo lo scatto e' a portata"),
		URTHexLibrary::HexDistance(FRTCellId(DashTo, 0), Foe->Cell) <= ShotRange);

	URTActionData* Dash = NewObject<URTActionData>(Runner);
	Dash->DisplayName = FText::FromString(TEXT("Scatto"));
	Dash->Def = URTCatalogLibrary::FindCoreAction(TEXT("Action.Dash"));
	Runner->Abilities.Add(Dash);
	const int32 DashIdx = Runner->Abilities.Num() - 1;

	const int32 FoeBefore = Foe->Health + Foe->Shield;

	// Senza lo scatto il colpo non partirebbe: e' cio' che rende il test discriminante invece che decorativo.
	Runner->PlannedDashAbility = DashIdx;
	Runner->PlannedDashCell = FRTCellId(DashTo, 0);
	Runner->PlannedAbilityIndex = 0;
	Runner->PlannedAttackTarget = Foe;
	Foe->PlannedCell = Foe->Cell;

	RunTurn(TM);

	TestTrue(TEXT("lo scatto ha portato l'unita' a destinazione"), Runner->Cell == FRTCellId(DashTo, 0));
	TestTrue(TEXT("l'azione principale e' rimasta disponibile: il colpo e' arrivato"),
		(Foe->Health + Foe->Shield) < FoeBefore);

	DestroyHexMoveWorld(World);
	return true;
}

// Chi aggiunge un test in fondo a questo file lo aggiunge PRIMA di questa riga: e' il difetto di #923,
// invisibile in Editor dove la guardia vale 1. Il controllo che lo dimostra e'
// `Build.bat RefactorTactics Win64 Shipping`, non la suite.
#endif // WITH_DEV_AUTOMATION_TESTS
