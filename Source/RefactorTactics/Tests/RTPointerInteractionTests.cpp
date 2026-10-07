// Contratto del puntatore (CP 11.8) — owner: docs/technical/systems/spec-pointer-interaction.md
//
// Due famiglie, e la divisione non e' organizzativa: le prime tre non toccano un `UWorld` perche' la
// precedenza semantica e l'ordine del Back sono funzioni PURE, e volerle provare attraverso il controller
// significherebbe non poter distinguere «ha scelto la cella perche' era il candidato giusto» da «ha scelto
// la cella perche' e' l'unica cosa che sa fare».
//
// Cio' che questi test NON possono provare resta lo stesso di sempre: che a schermo si VEDA l'affordance
// prima del click. Quella meta' e' `PIE-V01-POINTER`.

#include "Misc/AutomationTest.h"
#include "Player/RTPlayerController.h"
#include "Player/RTPointerInteraction.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Unit/RTUnit.h"
#include "Ability/RTActionData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Turn/RTFacingLibrary.h" // l'insieme legale si CHIEDE al servizio, non si indovina nel test
#include "Map/RTHexMapActor.h"
#include "Map/RTHexLibrary.h"
#include "RTWorldFixtures.h" // il mondo di prova: stessa porta che usano gli altri test d'integrazione
#include "Turn/RTPlaybackLibrary.h"
#include "Turn/RTMovementActionLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTCellId.h"
#include "Ability/RTCatalogLibrary.h" // IsFastMovement: la premessa «chiede un bersaglio» dei test di #3517
#include "Combat/RTCombatLibrary.h"   // TargetableRangeCells e ClassifyHexTargeting, chiesti come premesse
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	UWorld* MakePointerWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyPointerWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTUnit* SpawnPointerUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = false;
		U->DispatchBeginPlay();
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/**
	 * L'azione si CERCA per proprieta', non si hardcoda l'indice: e' la stessa disciplina di
	 * `FindDashAbilityIndex` nei test dell'interazione, e sopravvive a un riordino del kit.
	 */
	int32 FindAreaAbility(const ARTUnit* U)
	{
		for (int32 i = 0; i < U->NumAbilities(); ++i)
		{
			const URTActionData* A = U->GetAbility(i);
			if (A && A->Shape == ERTAbilityShape::Area && !A->bSelfTarget) { return i; }
		}
		return INDEX_NONE;
	}

	int32 FindStructureAbility(const ARTUnit* U)
	{
		for (int32 i = 0; i < U->NumAbilities(); ++i)
		{
			const URTActionData* A = U->GetAbility(i);
			if (A && A->Def.StructureOp != ERTStructureOp::None) { return i; }
		}
		return INDEX_NONE;
	}

	/**
	 * Un'azione che chiede un bersaglio e ha una portata positiva: quella che la portata viola mostra (`#3507`).
	 * `bConRicarica` ne chiede una che la ricarica possa davvero fermare (`#3517`).
	 *
	 * ⚠️ **Si cerca per proprieta' e non per nome, e per `E14` e' una correzione e non uno stile.** Il referto nomina
	 * `ArcPulse` in ricarica, ma `ArcPulse` e' un attacco base, e `Action.BasicAttack` ha ricarica `0` a catalogo:
	 * `ConsumeAbility` non lo ferma mai, quindi in partita non e' mai in ricarica.
	 */
	int32 FindAimedAbilityForRange(const ARTUnit* U, bool bConRicarica)
	{
		for (int32 i = 0; i < U->NumAbilities(); ++i)
		{
			const URTActionData* A = U->GetAbility(i);
			if (A && !A->bSelfTarget && A->Def.Slot == ERTActionSlot::Main && A->RangeCells > 0
				&& A->Def.ReservesMovementProfileId.IsNone() && !URTCatalogLibrary::IsFastMovement(A->Def)
				&& URTPointerLibrary::TargetKindForAction(A->Def, A->bSelfTarget, A->Shape) != ERTPointerTargetKind::None
				&& (!bConRicarica || A->CooldownTurns > 0))
			{
				return i;
			}
		}
		return INDEX_NONE;
	}
}

// ======================================================================================================
// §4.1 — la mesh non governa la UX
// ======================================================================================================

/**
 * Le tre conseguenze di §4.1 in un test solo, perche' sono la stessa regola vista da tre lati: cio' che il
 * raycast restituisce non e' cio' che il contesto rende significativo.
 *
 * Il caso della porta e' quello che si sbaglia per primo implementando: il collider della porta sta davanti,
 * quindi «la porta e' stata colpita» sembra la risposta — e il giocatore non riesce piu' a indicare la cella
 * oltre la porta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPointerPathingCellWinsTest,
	"RefactorTactics.PlayerInput.PathingCellWinsOverDoorMesh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPointerPathingCellWinsTest::RunTest(const FString&)
{
	FRTPointerCandidates C;
	C.bHasCell = true;
	C.Cell = FRTCellId(2, -1, 0);
	C.bMapElement = true; // il raggio ha colpito la mesh di una porta davanti alla cella

	const FRTPointerTarget T = URTPointerLibrary::ResolveTarget(
		ERTPointerContext::Pathing, ERTPointerTargetKind::None, C);

	TestEqual(TEXT("in Pathing vince la CELLA, non la mesh della porta"),
		T.Kind, ERTPointerTargetKind::Cell);
	TestTrue(TEXT("ed e' la cella sotto il punto colpito"), T.Cell == FRTCellId(2, -1, 0));

	// Controprova: nello stato NEUTRO la stessa porta e' invece l'oggetto giusto — li' si ispeziona.
	// Senza questa meta' il test passerebbe anche con un resolver che ignora sempre gli elementi logici.
	const FRTPointerTarget Neutral = URTPointerLibrary::ResolveTarget(
		ERTPointerContext::Planning, ERTPointerTargetKind::None, C);
	TestEqual(TEXT("in Planning la stessa porta e' un oggetto logico"),
		Neutral.Kind, ERTPointerTargetKind::Object);

	return true;
}

/**
 * Un'unita' sopra una cella non deve impedire di bersagliare quella cella: e' il caso che rende un'area
 * utilizzabile, perche' la si centra su chi la occupa.
 *
 * La controprova conta quanto il caso: con `TargetKind::Unit` la stessa coppia di candidati deve dare
 * l'UNITA', altrimenti il resolver sta semplicemente ignorando sempre le unita'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPointerTargetCellIgnoresUnitTest,
	"RefactorTactics.PlayerInput.TargetCellIgnoresOccupyingUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPointerTargetCellIgnoresUnitTest::RunTest(const FString&)
{
	UWorld* World = MakePointerWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTUnit* Occupant = SpawnPointerUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(1, 0, 0));

	FRTPointerCandidates C;
	C.Unit = Occupant;
	C.bHasCell = true;
	C.Cell = FRTCellId(1, 0, 0);

	const FRTPointerTarget AsCell = URTPointerLibrary::ResolveTarget(
		ERTPointerContext::Targeting, ERTPointerTargetKind::Cell, C);
	TestEqual(TEXT("un'area centrata su una cella OCCUPATA resta un bersaglio-cella"),
		AsCell.Kind, ERTPointerTargetKind::Cell);
	TestTrue(TEXT("ed e' la cella occupata"), AsCell.Cell == FRTCellId(1, 0, 0));

	const FRTPointerTarget AsUnit = URTPointerLibrary::ResolveTarget(
		ERTPointerContext::Targeting, ERTPointerTargetKind::Unit, C);
	TestEqual(TEXT("controprova: con TargetKind::Unit gli stessi candidati danno l'UNITA'"),
		AsUnit.Kind, ERTPointerTargetKind::Unit);

	DestroyPointerWorld(World);
	return true;
}

/**
 * Un ghost e' un fuoco della UI, mai un bersaglio di gioco: dove copre qualcosa, vince cio' che sta sotto.
 * Se non ci fosse nulla sotto, il puntatore non produce alcun target — non «il ghost».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPointerGhostIsNeverTargetTest,
	"RefactorTactics.PlayerInput.GhostIsNeverAGameplayTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPointerGhostIsNeverTargetTest::RunTest(const FString&)
{
	FRTPointerCandidates OverCell;
	OverCell.bGhost = true;
	OverCell.bHasCell = true;
	OverCell.Cell = FRTCellId(0, 1, 0);

	const FRTPointerTarget T = URTPointerLibrary::ResolveTarget(
		ERTPointerContext::Pathing, ERTPointerTargetKind::None, OverCell);
	TestEqual(TEXT("il ghost non ruba la cella sotto"), T.Kind, ERTPointerTargetKind::Cell);

	FRTPointerCandidates GhostOnly;
	GhostOnly.bGhost = true;
	const FRTPointerTarget Nothing = URTPointerLibrary::ResolveTarget(
		ERTPointerContext::Pathing, ERTPointerTargetKind::None, GhostOnly);
	TestEqual(TEXT("un ghost su nulla non produce un bersaglio"), Nothing.Kind, ERTPointerTargetKind::None);

	return true;
}

// ======================================================================================================
// §5.5 — RMB e' un Back, e l'ordine e' totale
// ======================================================================================================

/**
 * L'ordine e' la cosa da provare, non il fatto che «qualcosa venga annullato»: per questo lo stato di
 * partenza ha PIU' livelli montati insieme e si verifica che cada il piu' prioritario.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPointerBackOrderTest,
	"RefactorTactics.PlayerInput.RightClickBackFollowsTotalOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPointerBackOrderTest::RunTest(const FString&)
{
	// Tutti i livelli montati insieme: vince l'inspector, che e' il 3 e batte targeting, waypoint e focus.
	TestEqual(TEXT("inspector batte targeting, waypoint e phase focus"),
		URTPointerLibrary::ResolveBack(ERTPointerContext::Targeting, /*Inspector*/ true, /*Waypoints*/ 3,
			/*PhaseFocus*/ true),
		ERTPointerBackStep::Inspector);

	// Tolto l'inspector, tocca alla dichiarazione.
	TestEqual(TEXT("senza inspector, esce dal targeting"),
		URTPointerLibrary::ResolveBack(ERTPointerContext::Targeting, false, 3, true),
		ERTPointerBackStep::Declaration);

	// `#291`: il selettore aperto si chiude per primo, poi il verso dichiarato, poi il targeting e i waypoint.
	TestEqual(TEXT("un selettore del verso aperto si chiude per primo"),
		URTPointerLibrary::ResolveBack(ERTPointerContext::Facing, false, 0, true, /*bHasDeclaredFacing=*/ true),
		ERTPointerBackStep::Declaration);
	TestEqual(TEXT("il verso dichiarato batte il targeting"),
		URTPointerLibrary::ResolveBack(ERTPointerContext::Targeting, false, 3, true, /*bHasDeclaredFacing=*/ true),
		ERTPointerBackStep::DeclaredFacing);
	TestEqual(TEXT("e batte i waypoint"),
		URTPointerLibrary::ResolveBack(ERTPointerContext::Pathing, false, 2, true, /*bHasDeclaredFacing=*/ true),
		ERTPointerBackStep::DeclaredFacing);
	TestEqual(TEXT("ma durante il playback il Back non tocca il piano"),
		URTPointerLibrary::ResolveBack(ERTPointerContext::ResolutionPlayback, false, 0, false, /*bHasDeclaredFacing=*/ true),
		ERTPointerBackStep::None);

	// In Pathing i waypoint vengono prima dell'uscita dal contesto.
	TestEqual(TEXT("con waypoint, ne rimuove uno"),
		URTPointerLibrary::ResolveBack(ERTPointerContext::Pathing, false, 2, true),
		ERTPointerBackStep::Waypoint);
	TestEqual(TEXT("senza waypoint, esce dal Pathing"),
		URTPointerLibrary::ResolveBack(ERTPointerContext::Pathing, false, 0, true),
		ERTPointerBackStep::Pathing);

	// Il phase focus e' l'ULTIMO livello prima di NoOp: si smonta solo quando non resta altro.
	TestEqual(TEXT("il phase focus cade per ultimo"),
		URTPointerLibrary::ResolveBack(ERTPointerContext::Planning, false, 0, true),
		ERTPointerBackStep::PhaseFocus);
	TestEqual(TEXT("e senza nulla montato, NoOp"),
		URTPointerLibrary::ResolveBack(ERTPointerContext::Planning, false, 0, false),
		ERTPointerBackStep::None);

	// I due contesti che sovrascrivono tutto restano in cima anche con ogni altro livello montato.
	TestEqual(TEXT("la ReactionWindow batte qualunque cosa"),
		URTPointerLibrary::ResolveBack(ERTPointerContext::ReactionWindow, true, 5, true),
		ERTPointerBackStep::ReactionFallback);
	TestEqual(TEXT("il modale batte tutto tranne la ReactionWindow"),
		URTPointerLibrary::ResolveBack(ERTPointerContext::Modal, true, 5, true),
		ERTPointerBackStep::Modal);

	// Durante il playback nessun input cambia il piano: §5.3 dice NoOp anche con waypoint sul tavolo.
	TestEqual(TEXT("durante il playback il Back non tocca il piano"),
		URTPointerLibrary::ResolveBack(ERTPointerContext::ResolutionPlayback, false, 3, false),
		ERTPointerBackStep::None);

	return true;
}

/**
 * `RMB` non deseleziona MAI implicitamente l'unita'. E' l'errore che costringe a ricliccare la propria
 * unita' dopo ogni ripensamento, e si nota solo usando il gioco: nessun altro test lo guarda.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPointerBackNeverDeselectsTest,
	"RefactorTactics.PlayerInput.RightClickNeverDeselects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPointerBackNeverDeselectsTest::RunTest(const FString&)
{
	UWorld* World = MakePointerWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	ARTUnit* Unit = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(2, -2, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit) { DestroyPointerWorld(World); return false; }

	PC->SelectActorForTest(Unit);
	PC->HandleClickOnCell(FRTCellId(3, -2, 0));
	TestEqual(TEXT("premessa: c'e' un waypoint"), Unit->PlannedWaypoints.Num(), 1);

	TestEqual(TEXT("il Back toglie il waypoint"), PC->ApplyBack(), ERTPointerBackStep::Waypoint);
	TestNotNull(TEXT("e l'unita' resta selezionata"), PC->GetSelectedUnit());

	// Secondo Back: non c'e' piu' nulla da annullare, e la selezione resta comunque.
	PC->ApplyBack();
	TestNotNull(TEXT("neanche il secondo Back deseleziona"), PC->GetSelectedUnit());

	DestroyPointerWorld(World);
	return true;
}

// ======================================================================================================
// D-128 — lo stato neutro
// ======================================================================================================

/**
 * In stato neutro il click su un nemico ISPEZIONA: non pianifica. E la controprova nello stesso test —
 * armare l'azione e ricliccare pianifica — perche' senza di essa il test passerebbe anche con un
 * controller che non sa piu' bersagliare nulla.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPointerNeutralEnemyClickTest,
	"RefactorTactics.PlayerInput.NeutralEnemyClickDoesNotPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPointerNeutralEnemyClickTest::RunTest(const FString&)
{
	UWorld* World = MakePointerWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	ARTUnit* Mine  = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(),   FRTCellId(0, 0, 0));
	ARTUnit* Enemy = SpawnPointerUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(1, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Mine || !Enemy) { DestroyPointerWorld(World); return false; }

	PC->SelectActorForTest(Mine);

	TestEqual(TEXT("selezionata e niente armato -> contesto NEUTRO"),
		PC->GetPointerContext(), ERTPointerContext::Planning);
	TestEqual(TEXT("nessuna abilita' e' armata all'avvio"), Mine->SelectedAbilityIndex, (int32)INDEX_NONE);

	PC->HandleClickOnUnitForTest(Enemy);
	TestEqual(TEXT("il click su un nemico in stato neutro NON pianifica"),
		Mine->PlannedAbilityIndex, (int32)INDEX_NONE);
	TestNull(TEXT("e non registra un bersaglio"), (void*)Mine->PlannedAttackTarget.Get());

	// Controprova: armata l'azione, lo stesso click pianifica. Il percorso rapido resta due gesti.
	const int32 Attack = 0;
	Mine->SelectAbility(Attack);
	TestEqual(TEXT("armata -> contesto Targeting"), PC->GetPointerContext(), ERTPointerContext::Targeting);
	PC->HandleClickOnUnitForTest(Enemy);
	TestEqual(TEXT("armata, lo stesso click pianifica"), Mine->PlannedAbilityIndex, Attack);
	TestTrue(TEXT("sul nemico cliccato"), Mine->PlannedAttackTarget == Enemy);

	DestroyPointerWorld(World);
	return true;
}

// ======================================================================================================
// #737 — i tre produttori mancanti
// ======================================================================================================

/**
 * `Targeting`/`Cell` scrive `PlannedAttackCell` + `bAttackTargetsCell`. Prima di questo test quei due campi
 * avevano un solo produttore fuori dai test — lo Scenario Harness — e in partita erano irraggiungibili.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPointerTargetCellProducerTest,
	"RefactorTactics.PlayerInput.TargetCellProducesPlannedAttackCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPointerTargetCellProducerTest::RunTest(const FString&)
{
	UWorld* World = MakePointerWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	ARTUnit* Mine = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(0, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Mine) { DestroyPointerWorld(World); return false; }

	const int32 AreaIdx = FindAreaAbility(Mine);
	if (!TestTrue(TEXT("premessa: il kit contiene un'azione ad AREA"), AreaIdx != INDEX_NONE))
	{
		DestroyPointerWorld(World); return false;
	}

	PC->SelectActorForTest(Mine);

	// Senza armare, il produttore rifiuta: non e' un canale sempre aperto.
	TestFalse(TEXT("senza azione armata il bersaglio a cella e' rifiutato"),
		PC->HandleTargetCell(FRTCellId(1, 0, 0)));
	TestFalse(TEXT("e non ha sporcato il piano"), Mine->bAttackTargetsCell);

	Mine->SelectAbility(AreaIdx);
	TestEqual(TEXT("un'azione ad area chiede una CELLA"),
		PC->GetPointerTargetKind(), ERTPointerTargetKind::Cell);

	const FRTCellId Target(1, 0, 0);
	TestTrue(TEXT("il bersaglio a cella viene accettato"), PC->HandleTargetCell(Target));
	TestTrue(TEXT("bAttackTargetsCell e' vero"), Mine->bAttackTargetsCell);
	TestTrue(TEXT("PlannedAttackCell e' la cella indicata"), Mine->PlannedAttackCell == Target);
	TestEqual(TEXT("e l'azione pianificata e' quella armata"), Mine->PlannedAbilityIndex, AreaIdx);
	TestNull(TEXT("nessun bersaglio-unita' residuo"), (void*)Mine->PlannedAttackTarget.Get());

	// Fuori portata: rifiutato, e il piano precedente NON viene distrutto.
	const FRTCellId TooFar(9, 0, 0);
	TestFalse(TEXT("una cella fuori portata e' rifiutata"), PC->HandleTargetCell(TooFar));
	TestTrue(TEXT("e il bersaglio precedente resta"), Mine->PlannedAttackCell == Target);

	DestroyPointerWorld(World);
	return true;
}

/**
 * `Targeting`/`Edge` scrive `PlannedCoverEdge` + `bHasPlannedCoverEdge`. Il lato serve perche' a portata 3
 * il bordo non si deduce piu' dalla coppia di celle, e senza il resolver rifiuta con `CoverRejected`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPointerTargetEdgeProducerTest,
	"RefactorTactics.PlayerInput.TargetEdgeProducesPlannedCoverEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPointerTargetEdgeProducerTest::RunTest(const FString&)
{
	UWorld* World = MakePointerWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	ARTUnit* Builder = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(0, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Builder) { DestroyPointerWorld(World); return false; }

	const int32 StructIdx = FindStructureAbility(Builder);
	if (!TestTrue(TEXT("premessa: il kit contiene un'azione su STRUTTURA di bordo"), StructIdx != INDEX_NONE))
	{
		DestroyPointerWorld(World); return false;
	}

	PC->SelectActorForTest(Builder);
	Builder->SelectAbility(StructIdx);

	TestEqual(TEXT("un'azione su struttura chiede un BORDO"),
		PC->GetPointerTargetKind(), ERTPointerTargetKind::Edge);

	const FRTCellId Cell(1, 0, 0);
	TestTrue(TEXT("il bordo viene accettato"), PC->HandleTargetEdge(Cell, ERTHexDirection::NE));
	TestTrue(TEXT("bHasPlannedCoverEdge e' vero"), Builder->bHasPlannedCoverEdge);
	TestEqual(TEXT("PlannedCoverEdge e' il lato indicato"), Builder->PlannedCoverEdge, ERTHexDirection::NE);
	TestTrue(TEXT("e la cella su cui agire e' registrata"), Builder->PlannedAttackCell == Cell);

	DestroyPointerWorld(World);
	return true;
}

/**
 * `Facing` scrive `PlannedFacing` + `bDeclaresPlannedFacing`, che era la riga rimasta aperta di #291: le
 * regole della rotazione dichiarata esistevano, testate, e nessun input le raggiungeva.
 *
 * La seconda meta' e' la piu' importante: una dichiarazione ILLEGALE viene rifiutata, non corretta in
 * silenzio verso la legale piu' vicina.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPointerFacingProducerTest,
	"RefactorTactics.PlayerInput.FacingSectorProducesPlannedFacing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPointerFacingProducerTest::RunTest(const FString&)
{
	UWorld* World = MakePointerWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	ARTUnit* Unit = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit) { DestroyPointerWorld(World); return false; }

	PC->SelectActorForTest(Unit);

	// Fuori dal contesto `Facing` la dichiarazione e' rifiutata: non e' un canale sempre aperto.
	TestFalse(TEXT("senza entrare in Facing la rotazione e' rifiutata"),
		PC->HandleFacingSector(ERTHexDirection::W));
	TestFalse(TEXT("e nulla e' stato dichiarato"), Unit->bDeclaresPlannedFacing);

	PC->BeginFacingDeclaration();
	TestEqual(TEXT("il contesto e' Facing"), PC->GetPointerContext(), ERTPointerContext::Facing);

	// Ferma: ruota libera, tutte e sei legali.
	TestTrue(TEXT("da ferma, una rotazione qualunque e' legale"),
		PC->HandleFacingSector(ERTHexDirection::W));
	TestTrue(TEXT("bDeclaresPlannedFacing e' vero"), Unit->bDeclaresPlannedFacing);
	TestEqual(TEXT("PlannedFacing e' il settore indicato"), Unit->PlannedFacing, ERTHexDirection::W);
	TestEqual(TEXT("e il contesto e' tornato al neutro"),
		PC->GetPointerContext(), ERTPointerContext::Planning);

	DestroyPointerWorld(World);
	return true;
}

/**
 * Una rotazione illegale per lo stile di movimento pianificato viene RIFIUTATA, e il piano precedente resta
 * intatto: nessuna correzione silenziosa verso la direzione legale piu' vicina.
 *
 * Separato dal test sopra perche' e' la meta' che si rompe per prima se qualcuno «aggiusta» la dichiarazione
 * invece di negarla — e in una fase simultanea il giocatore non potrebbe accorgersene.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPointerIllegalFacingRejectedTest,
	"RefactorTactics.PlayerInput.IllegalFacingIsRejectedNotCorrected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPointerIllegalFacingRejectedTest::RunTest(const FString&)
{
	UWorld* World = MakePointerWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	// 🔴 **Branth e non Ivrin, e la sostituzione e' il punto del test** — ADR-0008 §1 (#1605). Questo test
	// ha bisogno che esista almeno una direzione ILLEGALE da rifiutare, e con Ivrin non esiste piu':
	// `MoveEndPivotMaxSteps = 3` gli concede tutte e sei le direzioni a fine Move. Branth, con budget 1, ne
	// concede tre e ne lascia tre da rifiutare. Il soggetto del test — «una rotazione illegale e'
	// rifiutata» — resta lo stesso; cambia l'eroe che ne ha ancora una.
	ARTUnit* Unit = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, -2, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit) { DestroyPointerWorld(World); return false; }

	PC->SelectActorForTest(Unit);

	// Con un percorso pianificato lo stile diventa `Budget`: restano le direzioni del budget dell'eroe.
	PC->HandleClickOnCell(FRTCellId(3, -2, 0));
	if (!TestTrue(TEXT("premessa: c'e' un percorso pianificato"), Unit->PlannedPath.Num() > 1))
	{
		DestroyPointerWorld(World); return false;
	}

	PC->BeginFacingDeclaration();

	// L'insieme legale si CHIEDE al servizio autorevole, non si indovina: si prende una direzione che NON
	// c'e' dentro, qualunque essa sia. Hardcodarne una renderebbe il test dipendente dalla geometria della
	// mappa di prova invece che dalla regola.
	const TArray<ERTHexDirection> Legal = URTFacingLibrary::LegalFacings(
		ERTMovementStyle::Budget, Unit->PlannedPath, Unit->Facing, Unit->PivotBudget());
	if (!TestTrue(TEXT("premessa: il budget di questo eroe lascia meno di sei direzioni"), Legal.Num() < 6))
	{
		DestroyPointerWorld(World); return false;
	}

	ERTHexDirection Illegal = ERTHexDirection::E;
	bool bFound = false;
	for (uint8 D = 0; D < 6; ++D)
	{
		const ERTHexDirection Candidate = static_cast<ERTHexDirection>(D);
		if (!Legal.Contains(Candidate)) { Illegal = Candidate; bFound = true; break; }
	}
	if (!TestTrue(TEXT("premessa: esiste una direzione illegale"), bFound))
	{
		DestroyPointerWorld(World); return false;
	}

	TestFalse(TEXT("una rotazione illegale e' rifiutata"), PC->HandleFacingSector(Illegal));
	TestFalse(TEXT("e NON viene dichiarata una direzione al suo posto"), Unit->bDeclaresPlannedFacing);

	// Controprova: una legale passa. Senza, il test passerebbe anche con un produttore che rifiuta tutto.
	TestTrue(TEXT("controprova: una rotazione legale viene accettata"), PC->HandleFacingSector(Legal[0]));
	TestTrue(TEXT("ed e' dichiarata"), Unit->bDeclaresPlannedFacing);
	TestEqual(TEXT("con la direzione chiesta"), Unit->PlannedFacing, Legal[0]);

	DestroyPointerWorld(World);
	return true;
}


/**
 * IL SETTORE DEL CURSORE E' IL LATO PIU' VICINO, CON UNA DEAD-ZONE E UNA REGOLA SUI CONFINI - `#291`, [D-367].
 *
 * 🔑 La funzione pura che il secondo click usa: se sbaglia, il giocatore clicca verso un lato e ne ottiene un
 * altro, e nessun test d'integrazione headless lo vedrebbe, perche' senza viewport non c'e' un cursore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFacingSectorFromOffsetTest,
	"RefactorTactics.Pointer.FacingSectorFromOffsetPicksTheNearestSide",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFacingSectorFromOffsetTest::RunTest(const FString&)
{
	// Sei lati a 60° l'uno dall'altro, ruotati di un angolo qualunque: la funzione non deve dipendere
	// dall'orientamento della griglia.
	const double Rotazione = 17.0;
	TArray<FVector2D> Direzioni;
	for (int32 I = 0; I < 6; ++I)
	{
		const double A = FMath::DegreesToRadians(Rotazione + 60.0 * I);
		Direzioni.Add(FVector2D(FMath::Cos(A), FMath::Sin(A)) * 150.0);
	}
	auto Verso = [&](double Gradi, double Raggio)
	{
		const double A = FMath::DegreesToRadians(Rotazione + Gradi);
		return FVector2D(FMath::Cos(A), FMath::Sin(A)) * Raggio;
	};

	ERTHexDirection Settore = ERTHexDirection::E;
	for (int32 I = 0; I < 6; ++I)
	{
		// 25° oltre il lato I: piu' vicino a I che a I+1 (che sta a 35°).
		TestTrue(*FString::Printf(TEXT("lato %d: il cursore sceglie"), I),
			URTPointerLibrary::FacingSectorFromOffset(Verso(60.0 * I + 25.0, 100.0), Direzioni, 30.f, Settore));
		TestEqual(*FString::Printf(TEXT("lato %d: e' il lato piu' vicino"), I), Settore, static_cast<ERTHexDirection>(I));
	}

	// Il confine esatto fra il lato 0 e il lato 1: vince il valore minore dell'enum, dichiarato e non lasciato
	// all'arrotondamento.
	TestTrue(TEXT("sul confine il cursore sceglie"),
		URTPointerLibrary::FacingSectorFromOffset(Verso(30.0, 100.0), Direzioni, 30.f, Settore));
	TestEqual(TEXT("e sceglie il lato di valore minore"), Settore, static_cast<ERTHexDirection>(0));

	// La dead-zone: un click al centro non sceglie niente.
	TestFalse(TEXT("nella dead-zone non si sceglie"),
		URTPointerLibrary::FacingSectorFromOffset(Verso(45.0, 20.0), Direzioni, 30.f, Settore));
	return true;
}

/**
 * ARMARE UN'AZIONE CHIUDE IL SELETTORE DEL VERSO - `#291`, dalla revisione (M1).
 *
 * 🔴 `Facing` precede `Targeting` in `GetPointerContext`: un selettore rimasto aperto mascherava il bersaglio, e il
 * click di mira diventava un waypoint.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTArmingClosesFacingSelectorTest,
	"RefactorTactics.PlayerInput.ArmingAnActionClosesTheFacingSelector",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTArmingClosesFacingSelectorTest::RunTest(const FString&)
{
	UWorld* World = MakePointerWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTUnit* Unit = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit) { DestroyPointerWorld(World); return false; }
	PC->SelectActorForTest(Unit);

	PC->HandleClickOnCell(Unit->Cell);
	if (!TestEqual(TEXT("premessa: il selettore e' aperto"), PC->GetPointerContext(), ERTPointerContext::Facing))
	{
		DestroyPointerWorld(World); return false;
	}
	int32 ConBersaglio = INDEX_NONE;
	for (int32 I = 0; I < Unit->NumAbilities() && ConBersaglio == INDEX_NONE; ++I)
	{
		const URTActionData* A = Unit->GetAbility(I);
		if (A && !A->bSelfTarget && A->Def.Slot == ERTActionSlot::Main && A->Def.ReservesMovementProfileId.IsNone())
		{
			ConBersaglio = I;
		}
	}
	if (!TestNotEqual(TEXT("premessa: un'azione con bersaglio"), ConBersaglio, (int32)INDEX_NONE))
	{
		DestroyPointerWorld(World); return false;
	}
	PC->SelectAbilityForCurrentForTest(ConBersaglio);
	TestEqual(TEXT("armata l'azione, il contesto e' il bersaglio, non il verso"),
		PC->GetPointerContext(), ERTPointerContext::Targeting);

	DestroyPointerWorld(World);
	return true;
}

/**
 * IL SECONDO CLICK SULLA DESTINAZIONE SCEGLIE IL VERSO E CHIUDE IL MOVIMENTO - `#291`, [D-367], [D-462].
 *
 * ⚠️ Quattro fatti in fila, perche' sono una sola esperienza: il click ripetuto non duplica il waypoint, il lato
 * dichiara, un'altra cella non estende il movimento chiuso, e il Back lo riapre togliendo il verso per primo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSecondClickClosesTheMoveTest,
	"RefactorTactics.PlayerInput.SecondClickOnTheDestinationDeclaresFacingAndClosesTheMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSecondClickClosesTheMoveTest::RunTest(const FString&)
{
	UWorld* World = MakePointerWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTUnit* Unit = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit) { DestroyPointerWorld(World); return false; }
	PC->SelectActorForTest(Unit);

	const FRTCellId Meta(1, 0, 0);
	PC->HandleClickOnCell(Meta);
	if (!TestEqual(TEXT("premessa: un waypoint, e la destinazione e' quella"), Unit->PlannedWaypoints.Num(), 1)
		|| !TestEqual(TEXT("premessa: la cella del verso e' la destinazione"), PC->FacingCellFor(Unit), Meta))
	{
		DestroyPointerWorld(World); return false;
	}

	// Il secondo click senza un lato (il centro, o un chiamante senza cursore): non sceglie e NON duplica, ma apre la
	// scelta del verso ([D-463]).
	PC->HandleClickOnCell(Meta);
	TestEqual(TEXT("il click ripetuto non duplica il waypoint"), Unit->PlannedWaypoints.Num(), 1);
	TestFalse(TEXT("e non dichiara niente"), Unit->bDeclaresPlannedFacing);
	TestEqual(TEXT("ma apre la scelta del verso"), PC->GetPointerContext(), ERTPointerContext::Facing);

	TestTrue(TEXT("il secondo click verso W dichiara il verso"), PC->HandleFacingClick(Meta, ERTHexDirection::W));
	TestTrue(TEXT("il verso e' dichiarato"), Unit->bDeclaresPlannedFacing);
	TestEqual(TEXT("ed e' W"), Unit->PlannedFacing, ERTHexDirection::W);

	PC->HandleClickOnCell(FRTCellId(2, 0, 0));
	TestEqual(TEXT("a movimento chiuso un'altra cella non aggiunge waypoint"), Unit->PlannedWaypoints.Num(), 1);
	TestTrue(TEXT("e il verso resta"), Unit->bDeclaresPlannedFacing);

	TestEqual(TEXT("il primo Back toglie il verso"), PC->ApplyBack(), ERTPointerBackStep::DeclaredFacing);
	TestFalse(TEXT("il verso non c'e' piu'"), Unit->bDeclaresPlannedFacing);
	TestEqual(TEXT("e il waypoint resta"), Unit->PlannedWaypoints.Num(), 1);

	PC->HandleClickOnCell(FRTCellId(2, 0, 0));
	TestEqual(TEXT("riaperto: un'altra cella aggiunge il waypoint"), Unit->PlannedWaypoints.Num(), 2);
	TestEqual(TEXT("e il Back dopo toglie un waypoint"), PC->ApplyBack(), ERTPointerBackStep::Waypoint);

	DestroyPointerWorld(World);
	return true;
}

namespace
{
	/** Un settore diverso da `Dir`: chi lo riceve indietro uguale a `Dir` l'ha davvero scritto. */
	ERTHexDirection AltroVerso(ERTHexDirection Dir)
	{
		return Dir == ERTHexDirection::E ? ERTHexDirection::W : ERTHexDirection::E;
	}
}

/**
 * IL CLICK DEL VERSO SI LEGGE SUL PAVIMENTO DELLA CELLA FINALE - [D-367], [D-463], `#291`.
 *
 * 🔑 La geometria che il controller teneva senza un test (follow-up di `#3504`). Una mappa spostata dall'origine, una
 * cella su un piano alto e un raggio OBLIQUO come quello della camera: se la proiezione sbagliasse il piano, il punto
 * scivolerebbe in un altro settore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTResolveFacingClickTest,
	"RefactorTactics.Pointer.ResolveFacingClickReadsTheFloorOfTheFinalCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTResolveFacingClickTest::RunTest(const FString&)
{
	const FVector Origine(37.f, -112.f, 20.f);
	constexpr float Lato = 100.f;
	constexpr float Piano = 250.f;
	const float DeadZone = 0.3f * Lato;
	const FRTCellId Finale(2, -1, 1);
	const FVector Centro = URTHexLibrary::AxialToWorld(Finale, Origine, Lato, Piano);

	auto Su = [](const FVector& Punto, FVector& OutOrigine, FVector& OutDir)
	{
		OutOrigine = Punto + FVector(-600.f, 350.f, 1200.f);
		OutDir = (Punto - OutOrigine).GetSafeNormal();
	};
	auto Leggi = [&](const FVector& O, const FVector& D, bool bAperto, ERTHexDirection& Settore)
	{
		return URTPointerLibrary::ResolveFacingClick(O, D, Finale, Origine, Lato, Piano, DeadZone, bAperto, Settore);
	};
	FVector O, D;
	ERTHexDirection Settore = ERTHexDirection::E;

	Su(Centro, O, D);
	TestTrue(TEXT("il centro e' la dead-zone, a selettore chiuso"), Leggi(O, D, false, Settore) == ERTFacingClick::Center);
	TestTrue(TEXT("e a selettore aperto"), Leggi(O, D, true, Settore) == ERTFacingClick::Center);

	for (uint8 I = 0; I < 6; ++I)
	{
		const ERTHexDirection Dir = static_cast<ERTHexDirection>(I);
		const FVector Vicino = URTHexLibrary::AxialToWorld(URTHexLibrary::Neighbor(Finale, Dir), Origine, Lato, Piano);

		// Dentro l'esagono e fuori dalla dead-zone: il 35% della distanza fra i centri e' circa 0,61 lati, fra la
		// dead-zone (0,3) e il bordo (0,87).
		Su(FMath::Lerp(Centro, Vicino, 0.35f), O, D);
		Settore = AltroVerso(Dir);
		const ERTFacingClick Dentro = Leggi(O, D, false, Settore);
		TestTrue(*FString::Printf(TEXT("lato %d: dentro l'esagono e' quel lato"), I),
			Dentro == ERTFacingClick::Side && Settore == Dir);

		// Sul centro del vicino: a selettore chiuso e' un'altra cella, cioe' movimento.
		Su(Vicino, O, D);
		TestTrue(*FString::Printf(TEXT("lato %d: il vicino, a selettore chiuso, e' un'altra cella"), I),
			Leggi(O, D, false, Settore) == ERTFacingClick::OtherCell);
		// 🔑 [D-463]: col selettore aperto e' la direzione verso di lui.
		Settore = AltroVerso(Dir);
		const ERTFacingClick Aperto = Leggi(O, D, true, Settore);
		TestTrue(*FString::Printf(TEXT("lato %d: col selettore aperto il vicino e' la sua direzione"), I),
			Aperto == ERTFacingClick::Side && Settore == Dir);
	}

	TestTrue(TEXT("un raggio orizzontale non incontra il pavimento"),
		Leggi(Centro + FVector(0.f, 0.f, 100.f), FVector(1.f, 0.f, 0.f), true, Settore) == ERTFacingClick::Miss);
	TestTrue(TEXT("ne' uno che sale"),
		Leggi(Centro + FVector(0.f, 0.f, 100.f), FVector(0.f, 0.f, 1.f), true, Settore) == ERTFacingClick::Miss);
	return true;
}

/**
 * DA UNA CELLA, IL SETTORE VERSO UN'ALTRA - [D-463], `#291`: il click su una cella col selettore aperto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFacingSectorTowardCellTest,
	"RefactorTactics.Pointer.FacingSectorTowardCellPointsAtTheNeighbour",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFacingSectorTowardCellTest::RunTest(const FString&)
{
	const FRTCellId Da(1, 2, 0);
	for (uint8 I = 0; I < 6; ++I)
	{
		const ERTHexDirection Dir = static_cast<ERTHexDirection>(I);
		const FRTCellId Vicino = URTHexLibrary::Neighbor(Da, Dir);
		ERTHexDirection Out = AltroVerso(Dir);
		TestTrue(*FString::Printf(TEXT("lato %d: il vicino"), I), URTPointerLibrary::FacingSectorTowardCell(Da, Vicino, Out) && Out == Dir);

		const FRTCellId Lontano = URTHexLibrary::Neighbor(URTHexLibrary::Neighbor(Vicino, Dir), Dir);
		Out = AltroVerso(Dir);
		TestTrue(*FString::Printf(TEXT("lato %d: lontano, nella stessa direzione"), I),
			URTPointerLibrary::FacingSectorTowardCell(Da, Lontano, Out) && Out == Dir);

		// Il verso e' planare ([D-367]): il piano della cella cliccata non conta.
		Out = AltroVerso(Dir);
		TestTrue(*FString::Printf(TEXT("lato %d: su un altro piano"), I),
			URTPointerLibrary::FacingSectorTowardCell(Da, FRTCellId(Vicino.X, Vicino.Y, 2), Out) && Out == Dir);
	}
	ERTHexDirection Out = ERTHexDirection::E;
	TestFalse(TEXT("la stessa cella non ha una direzione"), URTPointerLibrary::FacingSectorTowardCell(Da, Da, Out));
	TestFalse(TEXT("ne' la stessa cella su un altro piano"),
		URTPointerLibrary::FacingSectorTowardCell(Da, FRTCellId(Da.X, Da.Y, 3), Out));
	return true;
}

/**
 * IN MARCIA, IL CLICK SULLA DESTINAZIONE APRE LA SCELTA DEL VERSO - [D-463], `#291`.
 *
 * 🔴 **Il difetto visto in PIE il 2026-10-06**: il secondo click cadeva sul segno del waypoint, cioe' al centro, e la
 * dead-zone lo consumava senza fare nulla. Il verso non si sceglieva mai.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDestinationOpensFacingSelectorTest,
	"RefactorTactics.PlayerInput.DestinationCenterOpensTheFacingSelector",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDestinationOpensFacingSelectorTest::RunTest(const FString&)
{
	UWorld* World = MakePointerWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTUnit* Unit = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit) { DestroyPointerWorld(World); return false; }
	PC->SelectActorForTest(Unit);

	const FRTCellId Meta(1, 0, 0);
	PC->HandleClickOnCell(Meta);
	if (!TestEqual(TEXT("premessa: un waypoint"), Unit->PlannedWaypoints.Num(), 1))
	{
		DestroyPointerWorld(World); return false;
	}

	PC->HandleClickOnCell(Meta);
	TestEqual(TEXT("il click sulla destinazione apre la scelta del verso"), PC->GetPointerContext(), ERTPointerContext::Facing);
	TestEqual(TEXT("senza duplicare il waypoint"), Unit->PlannedWaypoints.Num(), 1);

	// Il click sull'esagono vicino in direzione NE: e' una direzione, non un passo.
	PC->HandleClickOnCell(URTHexLibrary::Neighbor(Meta, ERTHexDirection::NE));
	TestEqual(TEXT("il vicino non diventa un waypoint"), Unit->PlannedWaypoints.Num(), 1);
	TestTrue(TEXT("dichiara il verso"), Unit->bDeclaresPlannedFacing);
	TestEqual(TEXT("ed e' quello verso il vicino"), Unit->PlannedFacing, ERTHexDirection::NE);
	TestNotEqual(TEXT("e il selettore si chiude"), PC->GetPointerContext(), ERTPointerContext::Facing);

	DestroyPointerWorld(World);
	return true;
}

/**
 * DA FERMO, COL SELETTORE APERTO IL CLICK SUL VICINO E' UNA DIREZIONE - [D-463], `#291`.
 *
 * 🔴 **Il difetto visto in PIE il 2026-10-06**: il corpo copre quasi tutta la propria cella, quindi il click «sul
 * lato» cadeva sulla cella accanto, e chiudeva il selettore aggiungendo un passo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOpenSelectorNeighbourIsDirectionTest,
	"RefactorTactics.PlayerInput.OpenSelectorReadsANeighbourAsADirection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOpenSelectorNeighbourIsDirectionTest::RunTest(const FString&)
{
	UWorld* World = MakePointerWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTUnit* Unit = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit) { DestroyPointerWorld(World); return false; }
	PC->SelectActorForTest(Unit);

	PC->HandleClickOnCell(Unit->Cell);
	if (!TestEqual(TEXT("premessa: il selettore e' aperto"), PC->GetPointerContext(), ERTPointerContext::Facing))
	{
		DestroyPointerWorld(World); return false;
	}
	PC->HandleClickOnCell(URTHexLibrary::Neighbor(Unit->Cell, ERTHexDirection::SW));
	TestEqual(TEXT("nessun waypoint"), Unit->PlannedWaypoints.Num(), 0);
	TestTrue(TEXT("il verso e' dichiarato"), Unit->bDeclaresPlannedFacing);
	TestEqual(TEXT("ed e' quello verso il vicino"), Unit->PlannedFacing, ERTHexDirection::SW);

	DestroyPointerWorld(World);
	return true;
}

/**
 * UN LATO ILLEGALE LASCIA APERTO IL SELETTORE APERTO, E L'HOVER NON CI GIRA SOPRA - [D-463], [D-367], `#291`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIllegalSideKeepsSelectorOpenTest,
	"RefactorTactics.PlayerInput.IllegalSideKeepsAnOpenSelectorOpen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIllegalSideKeepsSelectorOpenTest::RunTest(const FString&)
{
	UWorld* World = MakePointerWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	// Branth: budget Move 1, quindi esistono lati illegali.
	ARTUnit* Unit = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, -2, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit) { DestroyPointerWorld(World); return false; }
	PC->SelectActorForTest(Unit);

	const FRTCellId Meta(3, -2, 0);
	PC->HandleClickOnCell(Meta);
	const TArray<ERTHexDirection> Legali = URTFacingLibrary::LegalFacings(
		ERTMovementStyle::Budget, Unit->PlannedPath, Unit->Facing, Unit->PivotBudget());
	const ERTHexDirection UltimoPasso = URTFacingLibrary::FacingFromPath(Unit->PlannedPath, Unit->Facing);
	ERTHexDirection Illegale = ERTHexDirection::E;
	ERTHexDirection Legale = UltimoPasso;
	bool bIllegale = false;
	bool bLegale = false;
	for (uint8 D = 0; D < 6; ++D)
	{
		const ERTHexDirection Dir = static_cast<ERTHexDirection>(D);
		if (!Legali.Contains(Dir) && !bIllegale) { Illegale = Dir; bIllegale = true; }
		if (Legali.Contains(Dir) && Dir != UltimoPasso && !bLegale) { Legale = Dir; bLegale = true; }
	}
	if (!TestTrue(TEXT("premessa: un percorso, un lato illegale e uno legale diverso dall'ultimo passo"),
		Unit->PlannedPath.Num() > 1 && bIllegale && bLegale))
	{
		DestroyPointerWorld(World); return false;
	}

	PC->HandleClickOnCell(Meta);
	if (!TestEqual(TEXT("premessa: il selettore e' aperto"), PC->GetPointerContext(), ERTPointerContext::Facing))
	{
		DestroyPointerWorld(World); return false;
	}

	// L'hover su un lato illegale non gira la mesh: non e' interattivo ([D-367]).
	FVector Origin; float HexSize; float LayerH;
	MapActor->GetHexContext(Origin, HexSize, LayerH);
	const FVector Sopra(0.f, 0.f, 500.f);
	const FVector Giu(0.f, 0.f, -1.f);
	PC->UpdateFacingHoverFromRay(true,
		URTHexLibrary::AxialToWorld(URTHexLibrary::Neighbor(Meta, Illegale), Origin, HexSize, LayerH) + Sopra, Giu);
	TestFalse(TEXT("l'hover su un lato illegale non gira la mesh"), PC->GetFacingHoverSector().IsSet());

	PC->HandleClickOnCell(URTHexLibrary::Neighbor(Meta, Illegale));
	TestFalse(TEXT("il lato illegale non dichiara"), Unit->bDeclaresPlannedFacing);
	TestEqual(TEXT("e il selettore resta aperto per un altro lato"), PC->GetPointerContext(), ERTPointerContext::Facing);
	TestEqual(TEXT("senza aggiungere waypoint"), Unit->PlannedWaypoints.Num(), 1);

	PC->HandleClickOnCell(URTHexLibrary::Neighbor(Meta, Legale));
	TestTrue(TEXT("un lato legale, dopo, dichiara"), Unit->bDeclaresPlannedFacing);
	TestEqual(TEXT("ed e' quello"), Unit->PlannedFacing, Legale);

	DestroyPointerWorld(World);
	return true;
}

/**
 * L'HOVER GIRA LA MESH VERSO IL LATO, SENZA TOCCARE IL PIANO - [D-367], [D-463], `#291`.
 *
 * ⚠️ Senza i triangoli disegnati (#172) e' l'unico riscontro del lato prima del click.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFacingHoverTurnsTheMeshTest,
	"RefactorTactics.PlayerInput.FacingHoverTurnsTheMeshWithoutDeclaring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFacingHoverTurnsTheMeshTest::RunTest(const FString&)
{
	UWorld* World = MakePointerWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTUnit* Unit = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit) { DestroyPointerWorld(World); return false; }
	PC->SelectActorForTest(Unit);

	FVector Origin; float HexSize; float LayerH;
	MapActor->GetHexContext(Origin, HexSize, LayerH);
	auto YawPer = [&](ERTHexDirection Dir)
	{
		const FVector Here = Unit->WorldForCell(Unit->Cell, Origin, HexSize, LayerH);
		const FVector There = Unit->WorldForCell(URTHexLibrary::Neighbor(Unit->Cell, Dir), Origin, HexSize, LayerH);
		return URTPlaybackLibrary::DirectionYaw(Here, There);
	};
	const FVector Sopra(0.f, 0.f, 500.f);
	const FVector Giu(0.f, 0.f, -1.f);
	const FVector SulVicinoW = URTHexLibrary::AxialToWorld(
		URTHexLibrary::Neighbor(Unit->Cell, ERTHexDirection::W), Origin, HexSize, LayerH) + Sopra;
	const ERTHexDirection Partenza = Unit->Facing;
	if (!TestNotEqual(TEXT("premessa: W non e' il verso di partenza"), Partenza, ERTHexDirection::W))
	{
		DestroyPointerWorld(World); return false;
	}
	PC->PreviewPlannedFacing(Unit);

	PC->UpdateFacingHoverFromRay(true, SulVicinoW, Giu);
	TestFalse(TEXT("a selettore chiuso l'hover non fa nulla"), PC->GetFacingHoverSector().IsSet());

	PC->HandleClickOnCell(Unit->Cell);
	PC->UpdateFacingHoverFromRay(true, SulVicinoW, Giu);
	TestTrue(TEXT("col selettore aperto l'hover prende il lato W"),
		PC->GetFacingHoverSector().IsSet() && PC->GetFacingHoverSector().GetValue() == ERTHexDirection::W);
	TestEqual(TEXT("e la mesh guarda W"), static_cast<float>(Unit->GetActorRotation().Yaw), YawPer(ERTHexDirection::W), 0.5f);
	TestFalse(TEXT("senza dichiarare"), Unit->bDeclaresPlannedFacing);
	TestEqual(TEXT("ne' toccare il verso logico"), Unit->Facing, Partenza);

	PC->UpdateFacingHoverFromRay(true, URTHexLibrary::AxialToWorld(Unit->Cell, Origin, HexSize, LayerH) + Sopra, Giu);
	TestFalse(TEXT("al centro l'hover si spegne"), PC->GetFacingHoverSector().IsSet());
	TestEqual(TEXT("e la mesh torna al verso pianificato"), static_cast<float>(Unit->GetActorRotation().Yaw), YawPer(Partenza), 0.5f);

	PC->UpdateFacingHoverFromRay(true, SulVicinoW, Giu);
	TestEqual(TEXT("premessa del Back: la mesh guarda di nuovo W"), static_cast<float>(Unit->GetActorRotation().Yaw), YawPer(ERTHexDirection::W), 0.5f);
	TestEqual(TEXT("il Back chiude il selettore"), PC->ApplyBack(), ERTPointerBackStep::Declaration);
	TestFalse(TEXT("l'hover non sopravvive al selettore"), PC->GetFacingHoverSector().IsSet());
	TestEqual(TEXT("e la mesh torna al verso pianificato"), static_cast<float>(Unit->GetActorRotation().Yaw), YawPer(Partenza), 0.5f);

	DestroyPointerWorld(World);
	return true;
}

/**
 * UN LATO ILLEGALE NON DICHIARA E NON CHIUDE IL MOVIMENTO - `#291`, [D-367].
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIllegalFacingClickTest,
	"RefactorTactics.PlayerInput.IllegalFacingClickLeavesTheMoveOpen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIllegalFacingClickTest::RunTest(const FString&)
{
	UWorld* World = MakePointerWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	// Branth: budget Move 1, quindi esistono lati illegali.
	ARTUnit* Unit = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, -2, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit) { DestroyPointerWorld(World); return false; }
	PC->SelectActorForTest(Unit);

	const FRTCellId Meta(3, -2, 0);
	PC->HandleClickOnCell(Meta);
	const TArray<ERTHexDirection> Legali = URTFacingLibrary::LegalFacings(
		ERTMovementStyle::Budget, Unit->PlannedPath, Unit->Facing, Unit->PivotBudget());
	ERTHexDirection Illegale = ERTHexDirection::E;
	bool bTrovata = false;
	for (uint8 D = 0; D < 6 && !bTrovata; ++D)
	{
		if (!Legali.Contains(static_cast<ERTHexDirection>(D))) { Illegale = static_cast<ERTHexDirection>(D); bTrovata = true; }
	}
	if (!TestTrue(TEXT("premessa: c'e' un percorso e un lato illegale"), Unit->PlannedPath.Num() > 1 && bTrovata))
	{
		DestroyPointerWorld(World); return false;
	}

	TestFalse(TEXT("il lato illegale e' rifiutato"), PC->HandleFacingClick(Meta, Illegale));
	TestFalse(TEXT("nessun verso dichiarato"), Unit->bDeclaresPlannedFacing);
	TestNotEqual(TEXT("e nessun selettore resta aperto a mangiare il click dopo"),
		PC->GetPointerContext(), ERTPointerContext::Facing);
	PC->HandleClickOnCell(FRTCellId(4, -2, 0));
	TestEqual(TEXT("il movimento resta aperto: il waypoint si aggiunge"), Unit->PlannedWaypoints.Num(), 2);

	DestroyPointerWorld(World);
	return true;
}

/**
 * DA FERMO, LA PROPRIA CELLA APRE I SEI TRIANGOLI E OGNI LATO E' RAGGIUNGIBILE - `#291`, [D-462] punto 4.
 *
 * ⌫ *Sostituisce `Pointer.CycleDeclaredFacingStaysWithinTheLegalSet`*: il tasto `T` e il ciclo sono usciti dal
 * gioco con [D-367]. La garanzia resta la stessa — da fermo le sei direzioni sono tutte raggiungibili — e la porta
 * e' quella nuova.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStationaryFacingSelectorTest,
	"RefactorTactics.PlayerInput.StationaryOwnCellOpensTheFacingSelector",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStationaryFacingSelectorTest::RunTest(const FString&)
{
	UWorld* World = MakePointerWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTUnit* Unit = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit) { DestroyPointerWorld(World); return false; }
	PC->SelectActorForTest(Unit);
	const FRTCellId Qui = Unit->Cell;

	TestFalse(TEXT("senza il primo click la propria cella non sceglie un verso"),
		PC->HandleFacingClick(Qui, ERTHexDirection::W));

	for (uint8 D = 0; D < 6; ++D)
	{
		const ERTHexDirection Lato = static_cast<ERTHexDirection>(D);
		if (D > 0)
		{
			TestEqual(*FString::Printf(TEXT("lato %d: il Back toglie il verso di prima"), D),
				PC->ApplyBack(), ERTPointerBackStep::DeclaredFacing);
		}
		PC->HandleClickOnCell(Qui);
		TestEqual(*FString::Printf(TEXT("lato %d: il click sulla propria cella apre il selettore"), D),
			PC->GetPointerContext(), ERTPointerContext::Facing);
		TestTrue(*FString::Printf(TEXT("lato %d: e il secondo click lo sceglie"), D), PC->HandleFacingClick(Qui, Lato));
		TestEqual(*FString::Printf(TEXT("lato %d: dichiarato"), D), Unit->PlannedFacing, Lato);
	}
	TestEqual(TEXT("il selettore si e' chiuso"), PC->GetPointerContext(), ERTPointerContext::Planning);

	PC->HandleClickOnCell(FRTCellId(1, 0, 0));
	TestEqual(TEXT("da fermo il verso chiude il movimento: nessun waypoint"), Unit->PlannedWaypoints.Num(), 0);

	DestroyPointerWorld(World);
	return true;
}

/**
 * UNO SCATTO PIANIFICATO SI GIUDICA SUL BUDGET DASH, NON SUL MOVE - `#291`, il difetto misurato da [D-367].
 *
 * 🔴 **Il difetto**: il controller stimava lo stile come `PlannedPath.Num() > 1 ? Budget : None`, quindi uno scatto
 * finiva giudicato sul budget Move. Branth ha Move 1 e Dash 0: col vecchio codice NE dopo uno scatto verso E era
 * legale, e il resolver poi lo rifiutava.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDashFacingBudgetTest,
	"RefactorTactics.PlayerInput.PlannedDashJudgesFacingOnTheDashBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDashFacingBudgetTest::RunTest(const FString&)
{
	UWorld* World = MakePointerWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTUnit* Unit = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, -2, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit) { DestroyPointerWorld(World); return false; }
	PC->SelectActorForTest(Unit);

	int32 Scatto = INDEX_NONE;
	for (int32 I = 0; I < Unit->NumAbilities() && Scatto == INDEX_NONE; ++I)
	{
		const URTActionData* A = Unit->GetAbility(I);
		if (A && URTMovementActionLibrary::IsLinear(A->Def.MovementStyle)) { Scatto = I; }
	}
	if (!TestNotEqual(TEXT("premessa: Branth ha uno scatto lineare"), Scatto, (int32)INDEX_NONE)
		|| !TestTrue(TEXT("premessa: budget Dash 0 e Move almeno 1"),
			Unit->DashEndPivotMaxSteps == 0 && Unit->MoveEndPivotMaxSteps >= 1))
	{
		DestroyPointerWorld(World); return false;
	}

	// Lo scatto si scrive sul piano direttamente: il soggetto e' il giudizio del verso, non la pianificazione dello
	// scatto, che ha i suoi test.
	const FRTCellId Arrivo = URTHexLibrary::Neighbor(URTHexLibrary::Neighbor(Unit->Cell, ERTHexDirection::E), ERTHexDirection::E);
	Unit->PlannedDashAbility = Scatto;
	Unit->PlannedDashCell = Arrivo;
	TestEqual(TEXT("la cella del verso e' l'arrivo dello scatto"), PC->FacingCellFor(Unit), Arrivo);

	const ERTHexDirection Accanto = static_cast<ERTHexDirection>((static_cast<uint8>(ERTHexDirection::E) + 1) % 6);
	TestFalse(TEXT("un lato accanto alla direzione dello scatto e' oltre il budget Dash 0"),
		PC->HandleFacingClick(Arrivo, Accanto));
	TestTrue(TEXT("la direzione dello scatto resta legale"), PC->HandleFacingClick(Arrivo, ERTHexDirection::E));

	DestroyPointerWorld(World);
	return true;
}

/**
 * UN PERCORSO TRONCATO CANCELLA IL VERSO E RIAPRE IL MOVIMENTO - `#291`, [D-367].
 *
 * 🔑 La destinazione cambia, quindi il verso scelto su quella vecchia non vale piu'. Il troncamento della riserva
 * scrive il percorso direttamente, senza passare dalla ricostruzione: e' il caso che un'implementazione col solo
 * `RebuildPlannedPath` lascerebbe scoperto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTruncationCancelsFacingTest,
	"RefactorTactics.PlayerInput.TruncatingTheMoveCancelsTheDeclaredFacing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTruncationCancelsFacingTest::RunTest(const FString&)
{
	UWorld* World = MakePointerWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTUnit* Unit = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(2, -2, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit) { DestroyPointerWorld(World); return false; }
	PC->SelectActorForTest(Unit);

	int32 Overwatch = INDEX_NONE;
	for (int32 I = 0; I < Unit->NumAbilities() && Overwatch == INDEX_NONE; ++I)
	{
		const URTActionData* A = Unit->GetAbility(I);
		if (A && A->Def.ActionId == TEXT("Action.Overwatch")) { Overwatch = I; }
	}
	PC->HandleClickOnCell(FRTCellId(3, -2, 0));
	PC->HandleClickOnCell(FRTCellId(3, -1, 0));
	const bool bDichiarato = PC->HandleFacingClick(FRTCellId(3, -1, 0), ERTHexDirection::W);
	if (!TestNotEqual(TEXT("premessa: Overwatch nel kit"), Overwatch, (int32)INDEX_NONE)
		|| !TestEqual(TEXT("premessa: due waypoint"), Unit->PlannedWaypoints.Num(), 2)
		|| !TestTrue(TEXT("premessa: verso dichiarato"), bDichiarato))
	{
		DestroyPointerWorld(World); return false;
	}

	PC->SelectAbilityForCurrentForTest(Overwatch);
	if (!TestTrue(TEXT("premessa: la riserva ha troncato"), Unit->PlannedWaypoints.Num() < 2))
	{
		DestroyPointerWorld(World); return false;
	}
	TestFalse(TEXT("il verso scelto sulla destinazione vecchia e' cancellato"), Unit->bDeclaresPlannedFacing);

	DestroyPointerWorld(World);
	return true;
}


/**
 * IN TARGETING LA PORTATA PRENDE IL POSTO DEL VENTAGLIO - `#3507`.
 *
 * 🔴 Il difetto, con le parole dell'autore dalla PIE del 2026-10-06: *«se seleziono un blast, vedo gli esagoni verdi.
 * ma non ho ancora selezionato un target»*. Il ventaglio verde e' il movimento; la portata e' dove posso mirare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTargetingShowsTheRangeTest,
	"RefactorTactics.PlayerInput.ArmingATargetedActionShowsTheRangeNotTheFan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTargetingShowsTheRangeTest::RunTest(const FString&)
{
	UWorld* World = MakePointerWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTUnit* Unit = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit) { DestroyPointerWorld(World); return false; }
	PC->SelectActorForTest(Unit);

	int32 ConBersaglio = INDEX_NONE;
	int32 Reazione = INDEX_NONE;
	for (int32 I = 0; I < Unit->NumAbilities(); ++I)
	{
		const URTActionData* A = Unit->GetAbility(I);
		if (!A) { continue; }
		if (ConBersaglio == INDEX_NONE && !A->bSelfTarget && A->Def.Slot == ERTActionSlot::Main
			&& A->Def.ReservesMovementProfileId.IsNone() && A->RangeCells > 0
			&& URTPointerLibrary::TargetKindForAction(A->Def, A->bSelfTarget, A->Shape) != ERTPointerTargetKind::None)
		{
			ConBersaglio = I;
		}
		if (Reazione == INDEX_NONE && A->Def.Slot == ERTActionSlot::Reaction)
		{
			Reazione = I;
		}
	}
	// Un waypoint accende l'anteprima come in partita: `SelectActorForTest` assegna la selezione e basta.
	PC->HandleClickOnCell(FRTCellId(1, 0, 0));
	if (!TestNotEqual(TEXT("premessa: un'azione a bersaglio"), ConBersaglio, (int32)INDEX_NONE)
		|| !TestEqual(TEXT("premessa: un waypoint"), Unit->PlannedWaypoints.Num(), 1)
		|| !TestTrue(TEXT("premessa: si vede il ventaglio"), MapActor->GetPreviewReachableCells().Num() > 0))
	{
		DestroyPointerWorld(World); return false;
	}
	TestEqual(TEXT("e nessuna portata"), MapActor->GetPreviewRangeCells().Num(), 0);

	PC->SelectAbilityForCurrentForTest(ConBersaglio);
	const URTActionData* Armata = Unit->GetAbility(ConBersaglio);
	const TArray<FRTCellId> Attesa = URTCombatLibrary::TargetableRangeCells(
		Arena, Unit->Cell, Armata->RangeCells, Armata->Def.LineOfSightPolicy);
	if (!TestEqual(TEXT("premessa: il contesto e' il bersaglio"), PC->GetPointerContext(), ERTPointerContext::Targeting)
		|| !TestTrue(TEXT("premessa: la portata attesa non e' vuota"), Attesa.Num() > 0))
	{
		DestroyPointerWorld(World); return false;
	}
	TestEqual(TEXT("armata, il ventaglio sparisce"), MapActor->GetPreviewReachableCells().Num(), 0);
	TestTrue(TEXT("e si vede la portata dell'azione"), MapActor->GetPreviewRangeCells() == Attesa);

	// Il Back su un targeting senza bersaglio esce e basta: la portata si spegne, torna il ventaglio.
	TestEqual(TEXT("il Back esce dal targeting"), PC->ApplyBack(), ERTPointerBackStep::Declaration);
	TestEqual(TEXT("la portata si spegne"), MapActor->GetPreviewRangeCells().Num(), 0);
	TestTrue(TEXT("e torna il ventaglio"), MapActor->GetPreviewReachableCells().Num() > 0);

	// Disarmata dal tasto, lo stesso.
	PC->SelectAbilityForCurrentForTest(ConBersaglio);
	TestTrue(TEXT("riarmata, la portata torna"), MapActor->GetPreviewRangeCells().Num() > 0);
	PC->SelectAbilityForCurrentForTest(INDEX_NONE);
	TestEqual(TEXT("disarmata dal tasto, la portata si spegne"), MapActor->GetPreviewRangeCells().Num(), 0);
	TestTrue(TEXT("e torna il ventaglio"), MapActor->GetPreviewReachableCells().Num() > 0);

	// Una reazione armata dopo: la portata dell'azione di prima non resta a schermo.
	if (Reazione != INDEX_NONE)
	{
		PC->SelectAbilityForCurrentForTest(ConBersaglio);
		PC->SelectAbilityForCurrentForTest(Reazione);
		TestEqual(TEXT("armata una reazione, la portata dell'azione di prima si spegne"),
			MapActor->GetPreviewRangeCells().Num(), 0);
	}
	else
	{
		AddWarning(TEXT("Ivrin non ha una reazione: il ramo della reazione non e' misurato"));
	}

	DestroyPointerWorld(World);
	return true;
}

/**
 * UN'AZIONE IN RICARICA ARMATA NON MOSTRA NE' LA PORTATA NE' IL VENTAGLIO - `#3517`, `DR-8` (`E14` del referto del
 * 2026-10-06).
 *
 * 🔴 Il difetto: un'azione attiva in ricarica si arma ancora, e la board mostrava la portata piena su cui il click
 * rifiuta ogni bersaglio. ⚠️ **Il controllo positivo e' la stessa azione PRONTA, dalla stessa cella**: senza, «la
 * portata e' vuota» potrebbe voler dire soltanto che quell'azione una portata non l'ha mai.
 *
 * 🔑 Passa dalla catena del gioco, `SelectUnit` e poi l'armo, non da `SelectActorForTest`: e' `SelectUnit` ad
 * accendere l'anteprima, ed e' il ventaglio che accende quello che questo test vede sparire.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCooldownArmShowsNoRangeTest,
	"RefactorTactics.PlayerInput.AnArmedActionOnCooldownShowsNeitherRangeNorFan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCooldownArmShowsNoRangeTest::RunTest(const FString&)
{
	UWorld* World = MakePointerWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTUnit* Unit = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(-1, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit) { DestroyPointerWorld(World); return false; }
	PC->SelectUnit(Unit);

	const int32 Azione = FindAimedAbilityForRange(Unit, /*bConRicarica=*/ true);
	if (!TestNotEqual(TEXT("premessa: un'azione a bersaglio con una ricarica"), Azione, (int32)INDEX_NONE)
		|| !TestTrue(TEXT("premessa: selezionata, si vede il ventaglio"), MapActor->GetPreviewReachableCells().Num() > 0))
	{
		DestroyPointerWorld(World); return false;
	}

	PC->SelectAbilityForCurrentForTest(Azione);
	if (!TestTrue(TEXT("premessa: pronta, l'azione armata mostra la portata"), MapActor->GetPreviewRangeCells().Num() > 0))
	{
		DestroyPointerWorld(World); return false;
	}
	PC->SelectAbilityForCurrentForTest(INDEX_NONE);

	Unit->ConsumeAbility(Azione);
	if (!TestFalse(TEXT("premessa: l'azione e' in ricarica"), Unit->CanUseAbility(Azione)))
	{
		DestroyPointerWorld(World); return false;
	}

	// ⚠️ Che si armi ancora e' la regola che `#3517` lascia com'e': rifiutare l'armo e' una decisione a parte.
	PC->SelectAbilityForCurrentForTest(Azione);
	if (!TestEqual(TEXT("premessa: in ricarica l'azione si arma ancora"), Unit->SelectedAbilityIndex, Azione)
		|| !TestEqual(TEXT("premessa: e il contesto e' il bersaglio"), PC->GetPointerContext(), ERTPointerContext::Targeting))
	{
		DestroyPointerWorld(World); return false;
	}
	TestEqual(TEXT("in ricarica, nessuna cella di portata"), MapActor->GetPreviewRangeCells().Num(), 0);
	TestEqual(TEXT("e nessun ventaglio"), MapActor->GetPreviewReachableCells().Num(), 0);

	DestroyPointerWorld(World);
	return true;
}

/**
 * UN ARMO DEGENERE NON MOSTRA NESSUNA PORTATA - `#3517`, `AC-10` del referto del 2026-10-06.
 *
 * Tre casi, ognuno col suo controllo:
 * - **`Action.Wait`**, portata `0`. La premessa chiede al produttore cosa darebbe: la sola cella del tiratore, che la
 *   board contornava di viola. Cosi' il verde non puo' venire da un produttore cambiato;
 * - **un'unita' caduta**, ancora selezionata quando il playback finisce e l'anteprima si ridisegna. Il controllo e'
 *   la stessa azione con l'unita' viva;
 * - **la mappa che manca**. ⚠️ Questo caso lo tiene gia' il produttore, che con una mappa nulla restituisce un
 *   insieme vuoto: qui si misura che la catena non lo aggiri, non una riga di `#3517`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDegenerateArmShowsNoRangeTest,
	"RefactorTactics.PlayerInput.ADegenerateArmShowsNoRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDegenerateArmShowsNoRangeTest::RunTest(const FString&)
{
	UWorld* World = MakePointerWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	// 🔴 **Senza, la fine del playback non raggiunge il controller, e non lo dice.** Il delegate e' DINAMICO e
	// passa da `AActor::ProcessEvent`, che scarta ogni evento finche' il mondo non ha `AreActorsInitialized()`.
	// Misurato qui il 2026-10-07: il controller era iscritto e la portata restava quella di prima. La spiegazione
	// completa sta in `MakeLockInPreviewBench` (`RTHexMatchIntegrationTests.cpp`).
	World->InitializeActorsForPlay(FURL());
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTUnit* Unit = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(-1, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit || !TM) { DestroyPointerWorld(World); return false; }
	PC->SelectUnit(Unit);

	// --- 1. `Action.Wait`: portata 0 ----------------------------------------------------------------------------
	int32 Attesa = INDEX_NONE;
	for (int32 I = 0; I < Unit->NumAbilities() && Attesa == INDEX_NONE; ++I)
	{
		const URTActionData* A = Unit->GetAbility(I);
		if (A && A->Def.ActionId == TEXT("Action.Wait")) { Attesa = I; }
	}
	const URTActionData* Wait = Unit->GetAbility(Attesa);
	if (!TestNotNull(TEXT("premessa: Action.Wait nel kit"), Wait)
		|| !TestEqual(TEXT("premessa: ha portata 0"), Wait->RangeCells, 0)
		|| !TestNotEqual(TEXT("premessa: e chiede un bersaglio, quindi si arma in targeting"),
			URTPointerLibrary::TargetKindForAction(Wait->Def, Wait->bSelfTarget, Wait->Shape), ERTPointerTargetKind::None)
		|| !TestTrue(TEXT("premessa: il produttore da solo le darebbe la cella del tiratore"),
			URTCombatLibrary::TargetableRangeCells(Arena, Unit->Cell, Wait->RangeCells, Wait->Def.LineOfSightPolicy)
				== TArray<FRTCellId>{ Unit->Cell }))
	{
		DestroyPointerWorld(World); return false;
	}
	PC->SelectAbilityForCurrentForTest(Attesa);
	if (!TestEqual(TEXT("premessa: Action.Wait e' armata"), Unit->SelectedAbilityIndex, Attesa))
	{
		DestroyPointerWorld(World); return false;
	}
	TestEqual(TEXT("1: Action.Wait armata non mostra nessuna portata"), MapActor->GetPreviewRangeCells().Num(), 0);
	PC->SelectAbilityForCurrentForTest(INDEX_NONE);

	// --- 2. l'unita' caduta: la fine del playback ridisegna dalla selezione ---------------------------------------
	const int32 Azione = FindAimedAbilityForRange(Unit, /*bConRicarica=*/ false);
	PC->SelectAbilityForCurrentForTest(Azione);
	if (!TestNotEqual(TEXT("premessa: un'azione a bersaglio"), Azione, (int32)INDEX_NONE)
		|| !TestTrue(TEXT("premessa: viva, l'azione armata mostra la portata"), MapActor->GetPreviewRangeCells().Num() > 0))
	{
		DestroyPointerWorld(World); return false;
	}
	Unit->Health = 0;
	TM->OnResolvePlaybackFinished.Broadcast();
	if (!TestTrue(TEXT("premessa: la caduta resta selezionata"), PC->GetSelectedUnit() == Unit)
		|| !TestFalse(TEXT("premessa: e non e' viva"), Unit->IsAlive()))
	{
		DestroyPointerWorld(World); return false;
	}
	TestEqual(TEXT("2: un'unita' caduta non mostra nessuna portata"), MapActor->GetPreviewRangeCells().Num(), 0);

	// --- 3. la mappa che manca ---------------------------------------------------------------------------------
	Unit->Health = 100;
	TM->OnResolvePlaybackFinished.Broadcast();
	if (!TestTrue(TEXT("premessa: di nuovo viva, la portata torna"), MapActor->GetPreviewRangeCells().Num() > 0))
	{
		DestroyPointerWorld(World); return false;
	}
	MapActor->MapAsset = nullptr;
	TM->OnResolvePlaybackFinished.Broadcast();
	TestEqual(TEXT("3: senza mappa, nessuna portata"), MapActor->GetPreviewRangeCells().Num(), 0);

	DestroyPointerWorld(World);
	return true;
}

/**
 * ARMARE PORTA IL PIANO ATTIVO A QUELLO DA CUI SI MIRA - `#3517`, `DR-5` (`AC-8` ed `E7` del referto del 2026-10-06).
 *
 * 🔴 Il difetto: il click si risolve sul piano attivo, la portata sta sul piano del tiratore. Da una piattaforma, con
 * il piano attivo a terra, ogni cella della portata cliccata diventava la cella di sotto, e `HandleTargetCell` la
 * rifiutava «su un altro piano».
 *
 * 🔑 **Il click si riproduce com'e' in partita, senza raycast.** `ResolveCellUnderCursor` restituisce sempre una
 * cella del piano ATTIVO, quindi la cella cliccata sopra una cella `c` della portata e' `(c.X, c.Y, piano attivo)`.
 * Il raycast headless non c'e'; la scelta del piano si', ed e' cio' che `DR-5` cambia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTArmingAlignsActivePlaneTest,
	"RefactorTactics.PlayerInput.ArmingMovesTheActivePlaneToTheShooter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTArmingAlignsActivePlaneTest::RunTest(const FString&)
{
	UWorld* World = MakePointerWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	const FRTCellId Piattaforma(2, 0, 1);
	ARTUnit* Unit = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeMuiren(), Piattaforma);
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit) { DestroyPointerWorld(World); return false; }
	PC->SelectUnit(Unit);

	const int32 Area = FindAreaAbility(Unit);
	const URTActionData* A = Unit->GetAbility(Area);
	if (!TestTrue(TEXT("premessa: la piattaforma e' nella mappa"), Arena->ContainsCell(Piattaforma))
		|| !TestNotNull(TEXT("premessa: un'azione ad area"), A)
		|| !TestTrue(TEXT("premessa: pronta, e con una portata"), Unit->CanUseAbility(Area) && A->RangeCells > 0)
		|| !TestEqual(TEXT("premessa: il piano attivo e' a terra"), PC->GetActiveLayer(), 0))
	{
		DestroyPointerWorld(World); return false;
	}

	PC->SelectAbilityForCurrentForTest(Area);
	TestEqual(TEXT("armata, il piano attivo e' quello del tiratore"), PC->GetActiveLayer(), Piattaforma.Layer);

	// Una cella della portata che il click accetta: la portata contiene anche le celle coperte, che il click
	// rifiuta col motivo (`#3507`), e la cella del tiratore resta fuori per non dipendere da un caso speciale.
	FRTCellId Bersaglio;
	bool bTrovato = false;
	for (const FRTCellId& C : MapActor->GetPreviewRangeCells())
	{
		if (!bTrovato && !(C == Unit->Cell)
			&& URTCombatLibrary::ClassifyHexTargeting(Arena, Unit->Cell, C, A->RangeCells, A->Def.LineOfSightPolicy)
				== ERTHexTargetReason::Ok)
		{
			Bersaglio = C;
			bTrovato = true;
		}
	}
	if (!TestTrue(TEXT("premessa: una cella della portata che il click accetta"), bTrovato)
		|| !TestEqual(TEXT("premessa: e sta sul piano del tiratore"), Bersaglio.Layer, Piattaforma.Layer))
	{
		DestroyPointerWorld(World); return false;
	}

	PC->HandleClickOnCellForTest(FRTCellId(Bersaglio.X, Bersaglio.Y, PC->GetActiveLayer()));
	TestEqual(TEXT("il click sulla portata produce un piano"), Unit->PlannedAbilityIndex, Area);
	TestTrue(TEXT("con il bersaglio su una cella"), Unit->bAttackTargetsCell);
	TestEqual(TEXT("quella cliccata"), Unit->PlannedAttackCell, Bersaglio);

	DestroyPointerWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlannedFacingPreviewTest,
	"RefactorTactics.Pointer.PlannedFacingPreviewFollowsThePlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlannedFacingPreviewTest::RunTest(const FString&)
{
	UWorld* World = MakePointerWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	ARTUnit* Unit = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit) { DestroyPointerWorld(World); return false; }
	PC->SelectActorForTest(Unit);

	// Lo yaw che una direzione produce, calcolato come lo calcolano il playback e l'anteprima: dalla
	// geometria, non da una tabella scritta a mano che sarebbe una seconda verita'.
	FVector Origin; float HexSize; float LayerH;
	MapActor->GetHexContext(Origin, HexSize, LayerH);
	auto YawPer = [&](ERTHexDirection Dir)
	{
		const FVector Here = Unit->WorldForCell(Unit->Cell, Origin, HexSize, LayerH);
		const FVector There = Unit->WorldForCell(URTHexLibrary::Neighbor(Unit->Cell, Dir), Origin, HexSize, LayerH);
		return URTPlaybackLibrary::DirectionYaw(Here, There);
	};

	// (1) Un percorso pianificato ruota la mesh verso l'ULTIMO PASSO, che e' il facing che il resolver
	// derivera' a fine Move. Prima di questa anteprima l'unita' restava girata come stava, e a fine
	// risoluzione scattava a un orientamento mai visto arrivare.
	// 🔴 **La cella d'arrivo NON deve stare nella direzione del facing di partenza.** La prima stesura
	// usava `(1,0,0)`, che da `(0,0,0)` e' proprio `E` — il default di `ARTUnit::Facing` — quindi il
	// derivato coincideva con l'orientamento che l'unita' aveva gia': l'assert era **vacuo**, e una
	// mutazione che ignorava del tutto il percorso lo lasciava verde. Trovato dalla verifica di mutazione.
	TestEqual(TEXT("si parte dal facing di default"), Unit->Facing, ERTHexDirection::E);

	// 🔴 **Un percorso che CURVA**, cosi' il primo passo e l'ultimo NON coincidono: e' l'unico caso in cui
	// l'assert distingue «guarda dove parte» da «guarda dove arrivera'». Su un percorso dritto le due
	// risposte sono la stessa, e il test non proverebbe niente.
	Unit->PlannedPath = { FRTCellId(0, 0, 0), FRTCellId(0, 1, 0), FRTCellId(1, 1, 0) };

	ERTHexDirection PrimoPasso = Unit->Facing;
	TestTrue(TEXT("il primo passo ha una direzione"),
		URTHexLibrary::DirectionBetween(Unit->PlannedPath[0], Unit->PlannedPath[1], PrimoPasso));
	const ERTHexDirection UltimoPasso = URTFacingLibrary::FacingFromPath(Unit->PlannedPath, Unit->Facing);
	TestNotEqual(TEXT("e il percorso curva: primo e ultimo passo differiscono"), PrimoPasso, UltimoPasso);
	TestNotEqual(TEXT("e il primo passo non e' il facing di partenza"), PrimoPasso, Unit->Facing);

	PC->PreviewPlannedFacing(Unit);

	// In pianificazione l'unita' e' ferma sulla cella di partenza: deve guardare dove **sta per andare**.
	// Orientarla secondo l'ultimo passo la farebbe guardare verso una direzione che assumera' dall'altra
	// parte del percorso, e che da qui puo' essere l'opposta di dove si incammina.
	TestEqual(TEXT("la mesh guarda verso il PRIMO passo"),
		static_cast<float>(Unit->GetActorRotation().Yaw), YawPer(PrimoPasso), 0.5f);
	TestNotEqual(TEXT("e NON verso l'ultimo"),
		static_cast<float>(Unit->GetActorRotation().Yaw), YawPer(UltimoPasso), 0.5f);

	// (2) E il facing LOGICO non e' cambiato: l'anteprima e' presentazione, e le regole restano quelle di
	// prima fino a fine Move. Senza questo controllo, l'anteprima potrebbe scrivere sullo stato e nessuno
	// se ne accorgerebbe finche' una difesa direzionale non desse l'esito sbagliato.
	TestEqual(TEXT("il facing logico NON e' stato toccato"), Unit->Facing, ERTHexDirection::E);

	// (3) Una rotazione dichiarata VINCE sul derivato, come a fine Move nel TurnManager: se le due regole
	// divergessero, l'anteprima mostrerebbe una direzione e il turno ne produrrebbe un'altra.
	PC->BeginFacingDeclaration();
	const TArray<ERTHexDirection> Legali =
		URTFacingLibrary::LegalFacings(ERTMovementStyle::Budget, Unit->PlannedPath, Unit->Facing,
			Unit->PivotBudget());
	const ERTHexDirection Scelta = Legali.Last();   // una legale diversa dal derivato, quando ce n'e' piu' d'una
	if (TestTrue(TEXT("la dichiarazione e' accettata"), PC->HandleFacingSector(Scelta)))
	{
		TestEqual(TEXT("la mesh segue la rotazione dichiarata"),
			static_cast<float>(Unit->GetActorRotation().Yaw), YawPer(Scelta), 0.5f);
	}

	DestroyPointerWorld(World);
	return true;
}

/**
 * CAMBIARE HOVER NON TOCCA NIENTE DI CIO' CHE E' STATO DECISO — `#1766`, e sblocca `#1614`.
 *
 * 🔑 **È un test di regressione su un'invariante che già regge**, ed è il motivo per cui si scrive adesso:
 * `#1614` allarga l'overlay dell'hover, e un overlay più grande rende **più visibile** un comportamento che
 * nessuno stava misurando. Se domani `SetHoveredCell` cominciasse a toccare la selezione o il piano, oggi
 * nessun test lo direbbe.
 *
 * ⚠️ **Il puntamento non è un impegno.** L'hover vive su `ARTHexMapActor` e scrive tre cose — `HoveredCell`,
 * `bHoveredValid`, `SetActorTickEnabled` — e nient'altro. Questo test lo verifica dal **verso opposto**:
 * costruisce uno stato deciso (selezione, waypoint, bersaglio, facing dichiarato), muove l'hover su celle
 * che sarebbero ottimi candidati per ciascuno di essi, e pretende che nulla si muova.
 *
 * ⛔ Non tocca `Player/` né `Map/`: questa issue aggiunge un test, non cambia comportamento.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPointerHoverNeverCommitsTest,
	"RefactorTactics.PlayerInput.HoverNeverCommits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPointerHoverNeverCommitsTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }

	ARTHexMapActor* Map = World->SpawnActor<ARTHexMapActor>();
	ARTUnit* Unit = World->SpawnActor<ARTUnit>();
	if (!Map || !Unit)
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	Map->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), /*Radius=*/ 4);

	// LO STATO DECISO: le quattro cose che `#1614` elenca nella propria DoD.
	Unit->Cell = FRTCellId(0, 0, 0);
	Unit->PlannedWaypoints = { FRTCellId(1, 0, 0), FRTCellId(2, 0, 0) };
	Unit->PlannedAttackCell = FRTCellId(2, -1, 0);
	Unit->bDeclaresPlannedFacing = true;
	Unit->PlannedFacing = ERTHexDirection::NE;

	const TArray<FRTCellId> WaypointsBefore = Unit->PlannedWaypoints;
	const FRTCellId AttackBefore = Unit->PlannedAttackCell;
	const ERTHexDirection FacingBefore = Unit->PlannedFacing;
	const bool bDeclaresBefore = Unit->bDeclaresPlannedFacing;
	const FRTCellId CellBefore = Unit->Cell;

	// L'hover passa su celle che sarebbero candidati PLAUSIBILI per ciascuna decisione: la cella del
	// bersaglio, il prossimo waypoint, una cella libera. Se il puntamento impegnasse, impegnerebbe qui.
	const FRTCellId Sweep[] = {
		FRTCellId(2, -1, 0),   // proprio il bersaglio dichiarato
		FRTCellId(3, 0, 0),    // il waypoint successivo naturale
		FRTCellId(-1, 1, 0),   // una cella qualunque, dall'altra parte
		FRTCellId(0, 0, 0)     // la cella dell'unita' stessa
	};
	for (const FRTCellId& Hovered : Sweep)
	{
		Map->SetHoveredCell(Hovered, /*bValid=*/ true);
	}
	Map->SetHoveredCell(FRTCellId(9, 9, 0), /*bValid=*/ false);

	// L'hover E' cambiato: senza questa riga, tutto il resto sarebbe vero anche se la funzione non facesse
	// niente, e il test passerebbe misurando l'assenza di una chiamata invece della sua innocuita'.
	TestEqual(TEXT("controllo: l'hover ha registrato l'ultima cella"), Map->GetHoveredCell(), FRTCellId(9, 9, 0));
	TestFalse(TEXT("controllo: e la sua validita'"), Map->IsHoveredCellValid());

	TestEqual(TEXT("i waypoint non cambiano"), Unit->PlannedWaypoints.Num(), WaypointsBefore.Num());
	for (int32 i = 0; i < WaypointsBefore.Num() && i < Unit->PlannedWaypoints.Num(); ++i)
	{
		TestEqual(FString::Printf(TEXT("waypoint %d"), i), Unit->PlannedWaypoints[i], WaypointsBefore[i]);
	}
	TestEqual(TEXT("l'azione armata mira ancora dove mirava"), Unit->PlannedAttackCell, AttackBefore);
	TestEqual(TEXT("il facing dichiarato non ruota"), static_cast<int32>(Unit->PlannedFacing),
		static_cast<int32>(FacingBefore));
	TestEqual(TEXT("e resta dichiarato"), Unit->bDeclaresPlannedFacing, bDeclaresBefore);
	TestEqual(TEXT("l'unita' non si e' mossa"), Unit->Cell, CellBefore);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * IL TASTO DESTRO SMONTA UN PASSO PER VOLTA, E QUANDO NON C'E' NIENTE DA SMONTARE NON FA NIENTE — `#1766`.
 *
 * 🔑 **«Cancella solo l'anteprima» vuol dire due cose insieme**: che il passo indietro tolga *il livello più
 * esterno* e non l'intera dichiarazione, e che a mani vuote **non** deselezioni. La seconda metà è già
 * pinnata da `PlayerInput.RightClickNeverDeselects`; questa copre la prima e il loro punto di incontro —
 * `ERTPointerBackStep::None`, l'unico esito che non smonta niente.
 *
 * ⚠️ Il verso che conta è **l'assenza di scorciatoie**: nessun contesto salta un livello. Un `Targeting` con
 * tre waypoint montati non deve tornare al `Planning` in un colpo, o l'anteprima non sarebbe più
 * annullabile «solo» — sarebbe annullabile *tutta*.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPointerRightClickCancelsPreviewOnlyTest,
	"RefactorTactics.PlayerInput.RightClickCancelsPreviewOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPointerRightClickCancelsPreviewOnlyTest::RunTest(const FString&)
{
	// Da `Targeting` con tutto montato si esce di UN livello: l'inspector, non la dichiarazione.
	TestEqual(TEXT("con l'inspector aperto, il primo passo indietro chiude quello"),
		URTPointerLibrary::ResolveBack(ERTPointerContext::Targeting, /*Inspector=*/ true, /*Waypoints=*/ 3,
			/*PhaseFocus=*/ true),
		ERTPointerBackStep::Inspector);

	// ⛔ E NON scende oltre: chiuso l'inspector, il passo successivo esce dal targeting — non dal phase focus.
	TestEqual(TEXT("il passo dopo esce dal targeting, non dalla fase"),
		URTPointerLibrary::ResolveBack(ERTPointerContext::Targeting, false, 3, true),
		ERTPointerBackStep::Declaration);

	// I waypoint si smontano UNO per volta, e finche' ce n'e' uno il contesto non si abbandona.
	TestEqual(TEXT("con tre waypoint ne toglie uno"),
		URTPointerLibrary::ResolveBack(ERTPointerContext::Pathing, false, 3, true),
		ERTPointerBackStep::Waypoint);
	TestEqual(TEXT("con uno solo, ancora un waypoint e non il contesto"),
		URTPointerLibrary::ResolveBack(ERTPointerContext::Pathing, false, 1, true),
		ERTPointerBackStep::Waypoint);
	TestEqual(TEXT("a zero waypoint esce dal Pathing"),
		URTPointerLibrary::ResolveBack(ERTPointerContext::Pathing, false, 0, true),
		ERTPointerBackStep::Pathing);

	// 🔑 IL PUNTO DELLA VOCE: a mani vuote il tasto destro non ha niente da annullare, e NON deseleziona.
	TestEqual(TEXT("senza niente montato non succede nulla"),
		URTPointerLibrary::ResolveBack(ERTPointerContext::Planning, false, 0, false),
		ERTPointerBackStep::None);

	// ⚠️ La controprova che `None` non sia l'esito di comodo di ogni caso non gestito: con il solo phase
	// focus montato, lo stesso contesto risponde diversamente.
	TestEqual(TEXT("ma col phase focus montato, quello cade"),
		URTPointerLibrary::ResolveBack(ERTPointerContext::Planning, false, 0, true),
		ERTPointerBackStep::PhaseFocus);

	return true;
}

// ======================================================================================================
// #2826 scope 7 — il tasto che applica il Back, non una sua imitazione
// ======================================================================================================

/**
 * L'`RMB` **esce dal targeting prima di toccare i waypoint**, e non deseleziona.
 *
 * 🔴 **Il modulo puro era verde mentre il tasto sbagliava.** `ResolveBack` dichiara l'ordine di §5.5 e
 * `RightClickCancelsPreviewOnly` lo verifica; `ApplyBack` lo applica e ha i suoi test. Ma il percorso che
 * il giocatore usa davvero — `OnUndoWaypoint`, bindata su `RightMouseButton` — ordinava i livelli **per
 * conto proprio** e andava dritta a `PlannedWaypoints.Pop()`: con un'azione armata e un waypoint montato
 * toglieva il waypoint e lasciava il targeting acceso. Nessun test lo guardava, perche' tutti si fermavano
 * un livello piu' in basso.
 *
 * ⚠️ **La premessa e' proprio il caso che distingue le due autorita'**: `GetPointerContext()` mette
 * `Targeting` PRIMA di `Pathing`, quindi con entrambi montati il contesto e' `Targeting`. Un test con la
 * sola azione armata e zero waypoint passerebbe anche con il vecchio corpo — `Pop()` su una lista vuota non
 * fa nulla — e non misurerebbe niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPointerRightClickLeavesTargetingFirstTest,
	"RefactorTactics.PlayerInput.RightClickLeavesTargetingBeforeWaypoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPointerRightClickLeavesTargetingFirstTest::RunTest(const FString&)
{
	UWorld* World = MakePointerWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	ARTUnit* Unit = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(0, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit) { DestroyPointerWorld(World); return false; }

	PC->SelectActorForTest(Unit);

	// Premessa in due tempi: prima il waypoint, poi l'azione armata sopra.
	PC->HandleClickOnCellForTest(FRTCellId(1, 0, 0));
	if (!TestEqual(TEXT("premessa: c'e' un waypoint"), Unit->PlannedWaypoints.Num(), 1))
	{
		DestroyPointerWorld(World);
		return false;
	}

	Unit->SelectAbility(0);
	if (!TestEqual(TEXT("premessa: con entrambi montati il contesto e' Targeting"),
		PC->GetPointerContext(), ERTPointerContext::Targeting))
	{
		DestroyPointerWorld(World);
		return false;
	}

	// --- Il primo destro: esce dalla DICHIARAZIONE ---------------------------------------------------
	PC->OnUndoWaypointForTest();

	TestEqual(TEXT("il destro disarma l'azione"), Unit->SelectedAbilityIndex, (int32)INDEX_NONE);
	TestEqual(TEXT("e NON tocca il waypoint, che sta un livello piu' in basso"),
		Unit->PlannedWaypoints.Num(), 1);
	TestNotNull(TEXT("e non deseleziona"), PC->GetSelectedUnit());

	// --- Il secondo destro: adesso, e solo adesso, il waypoint ---------------------------------------
	// 🔑 Controprova indispensabile: senza di essa il test passerebbe anche con un `OnUndoWaypoint` che non
	// sa piu' togliere waypoint affatto — cioe' con il tasto rotto invece che corretto.
	TestEqual(TEXT("disarmata, il contesto scende a Pathing"),
		PC->GetPointerContext(), ERTPointerContext::Pathing);

	PC->OnUndoWaypointForTest();
	TestEqual(TEXT("il secondo destro toglie il waypoint"), Unit->PlannedWaypoints.Num(), 0);
	TestNotNull(TEXT("e neanche adesso deseleziona"), PC->GetSelectedUnit());

	DestroyPointerWorld(World);
	return true;
}

/**
 * 🔴 **`Inspect` e `Select` sono esiti diversi, e l'oracolo e' la DIFFERENZA.**
 *
 * Un test che asserisse solo *«un'avversaria produce `Inspect`»* passerebbe anche con una funzione che
 * risponde `Inspect` a tutto. Cio' che va pinnato e' che il **soggetto comandato** e il **soggetto
 * ispezionato** non finiscano nello stesso esito: da li' dipende che il pannello degli slot e il dock delle
 * azioni non leggano mai di un'avversaria il piano e il kit.
 *
 * ⚠️ E pinna anche cio' che NON cambia: in `Targeting` una non comandabile resta `Confirm`. E' la meta'
 * della decisione del 2026-09-11 che ha scartato la lettura «larga», e senza questa riga il test
 * descriverebbe solo la meta' nuova.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPointerOutcomeSeparatesInspectFromSelectTest,
	"RefactorTactics.Pointer.OutcomeSeparatesInspectFromSelect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPointerOutcomeSeparatesInspectFromSelectTest::RunTest(const FString&)
{
	using ERTOut = ERTPointerOutcome;

	// Fuori da `Targeting`: comandabile -> `Select`, osservata-e-non-comandabile -> `Inspect`.
	for (const ERTPointerContext Ctx : { ERTPointerContext::IdleSelection, ERTPointerContext::Planning,
		ERTPointerContext::Pathing, ERTPointerContext::Facing })
	{
		const ERTOut Mia = URTPointerLibrary::ResolveOutcome(Ctx, true, /*bCommandable=*/ true, true);
		const ERTOut Altrui = URTPointerLibrary::ResolveOutcome(Ctx, true, /*bCommandable=*/ false, true);

		TestEqual(TEXT("la propria si seleziona"), Mia, ERTOut::Select);
		TestEqual(TEXT("l'altrui osservata si ispeziona"), Altrui, ERTOut::Inspect);
		TestNotEqual(TEXT("e i due esiti NON coincidono"), Mia, Altrui);
	}

	// In `Targeting` cambia solo il lato non comandabile: la propria continua a ri-selezionarsi.
	TestEqual(TEXT("in Targeting l'avversaria si conferma come bersaglio"),
		URTPointerLibrary::ResolveOutcome(ERTPointerContext::Targeting, true, false, true), ERTOut::Confirm);
	TestEqual(TEXT("in Targeting la propria si ri-seleziona, non si bersaglia"),
		URTPointerLibrary::ResolveOutcome(ERTPointerContext::Targeting, true, true, true), ERTOut::Select);

	// I contesti in cui l'input di gioco non arriva rifiutano, e lo dicono.
	for (const ERTPointerContext Ctx : { ERTPointerContext::Modal, ERTPointerContext::ResolutionPlayback,
		ERTPointerContext::ReactionWindow })
	{
		TestEqual(TEXT("input di gioco bloccato: rifiuto dichiarato"),
			URTPointerLibrary::ResolveOutcome(Ctx, true, true, true), ERTOut::Blocked);
	}

	return true;
}

/**
 * ⛔ **Cio' che il velo nasconde risponde come il terreno vuoto, e la prova e' l'UGUAGLIANZA fra i due.**
 *
 * 🔴 Questo test esiste perche' la prima stesura di `ResolveOutcome` sbagliava proprio qui: rispondeva
 * `Blocked` a un'unita' non osservata, citando la DoD di `#705` — *«ogni rifiuto porta un reason code»* — e
 * si contraddiceva col proprio commento accanto, che prometteva un comportamento *«indistinguibile da una
 * cella vuota»*. Il terreno vuoto da' `NoOp`: un `Blocked` sarebbe stato **distinguibile**, e un reason code
 * su un'unita' di cui non dovresti sapere l'esistenza e' il canale che quella riga esiste per chiudere.
 *
 * ⚠️ **L'asserzione e' un confronto, non un valore atteso.** Scrivere `TestEqual(..., NoOp)` passerebbe
 * anche il giorno in cui il terreno vuoto cominciasse a rispondere altro, e i due tornerebbero a
 * distinguersi senza che nessun test cada.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPointerVeiledUnitTest,
	"RefactorTactics.Pointer.VeiledUnitIsIndistinguishableFromEmptyGround",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPointerVeiledUnitTest::RunTest(const FString&)
{
	for (const ERTPointerContext Ctx : { ERTPointerContext::IdleSelection, ERTPointerContext::Planning,
		ERTPointerContext::Pathing, ERTPointerContext::Targeting, ERTPointerContext::Facing })
	{
		const ERTPointerOutcome Velata =
			URTPointerLibrary::ResolveOutcome(Ctx, /*bHitUnit=*/ true, /*bCommandable=*/ false, /*bObserved=*/ false);
		const ERTPointerOutcome TerrenoVuoto =
			URTPointerLibrary::ResolveOutcome(Ctx, /*bHitUnit=*/ false, /*bCommandable=*/ false, /*bObserved=*/ false);

		TestEqual(TEXT("un'unita' velata risponde come il terreno vuoto"), Velata, TerrenoVuoto);
	}

	// Controllo positivo: la stessa unita', **osservata**, produce un esito diverso. Senza questa riga il
	// test passerebbe anche se `ResolveOutcome` rispondesse `NoOp` a qualunque cosa.
	TestNotEqual(TEXT("osservata, la stessa unita' NON risponde come il terreno vuoto"),
		URTPointerLibrary::ResolveOutcome(ERTPointerContext::Planning, true, false, /*bObserved=*/ true),
		URTPointerLibrary::ResolveOutcome(ERTPointerContext::Planning, false, false, false));

	return true;
}

/**
 * 🔴 **Il click su un'avversaria la ISPEZIONA, e non costa nulla di cio' che si stava comandando.**
 *
 * Prima della casella ratificata il 2026-09-11 quel click era un **no-op silenzioso**: `HandleClickOnUnit`
 * usciva su `!Ability` e a schermo non cambiava niente.
 *
 * ⚠️ **Le due asserzioni che contano sono quelle NEGATIVE**, e senza di loro il test sarebbe quasi vacuo:
 * che l'unita' comandata resti comandata, e che il soggetto ispezionato **non** sia quello selezionato. La
 * seconda e' la barriera di privacy resa verificabile: se `Inspect` scrivesse `SelectedActor`, il pannello
 * mostrerebbe gli slot dell'avversaria e il dock il suo kit, e questo test cadrebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPointerEnemyClickInspectsTest,
	"RefactorTactics.PlayerInput.EnemyClickInspectsWithoutCostingTheCommandedUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPointerEnemyClickInspectsTest::RunTest(const FString&)
{
	UWorld* World = MakePointerWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	ARTUnit* Mine  = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(0, 0, 0));
	ARTUnit* Enemy = SpawnPointerUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(1, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Mine || !Enemy) { DestroyPointerWorld(World); return false; }

	PC->SelectActorForTest(Mine);
	TestEqual(TEXT("niente armato -> contesto NEUTRO"), PC->GetPointerContext(), ERTPointerContext::Planning);
	TestNull(TEXT("all'inizio non si sta ispezionando nulla"), (void*)PC->GetInspectedUnit());

	TestTrue(TEXT("il click sull'avversaria e' consumato"), PC->DispatchUnitClickForTest(Enemy, Mine));
	TestTrue(TEXT("e l'avversaria e' il soggetto ispezionato"), PC->GetInspectedUnit() == Enemy);

	// ⛔ Le due negative: ispezionare non costa il comando, e il soggetto ispezionato NON e' il selezionato.
	TestTrue(TEXT("l'unita' comandata resta comandata"), PC->GetSelectedUnit() == Mine);
	TestTrue(TEXT("ispezionato e selezionato sono DUE soggetti diversi"),
		PC->GetInspectedUnit() != PC->GetSelectedUnit());
	TestEqual(TEXT("e nessuna abilita' e' stata armata o disarmata"),
		Mine->SelectedAbilityIndex, (int32)INDEX_NONE);
	TestEqual(TEXT("ne' pianificata"), Mine->PlannedAbilityIndex, (int32)INDEX_NONE);

	// In `Targeting` il comportamento NON cambia: lo stesso click bersaglia, come prima della decisione.
	const int32 Attack = 0;
	Mine->SelectAbility(Attack);
	TestEqual(TEXT("armata -> contesto Targeting"), PC->GetPointerContext(), ERTPointerContext::Targeting);

	TestTrue(TEXT("in Targeting il click resta consumato"), PC->DispatchUnitClickForTest(Enemy, Mine));
	TestEqual(TEXT("e pianifica, invece di ispezionare"), Mine->PlannedAbilityIndex, Attack);
	TestTrue(TEXT("sul nemico cliccato"), Mine->PlannedAttackTarget == Enemy);

	DestroyPointerWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
