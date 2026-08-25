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
#include "Turn/RTPlaybackLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTCellId.h"
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
	ARTUnit* Occupant = SpawnPointerUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(1, 0, 0));

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

	TestEqual(TEXT("Facing e' allo stesso livello del Targeting"),
		URTPointerLibrary::ResolveBack(ERTPointerContext::Facing, false, 0, true),
		ERTPointerBackStep::Declaration);

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

	ARTUnit* Unit = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(2, -2, 0));
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

	ARTUnit* Mine  = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeGadget(),   FRTCellId(0, 0, 0));
	ARTUnit* Enemy = SpawnPointerUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(1, 0, 0));
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

	ARTUnit* Mine = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeGadget(), FRTCellId(0, 0, 0));
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

	ARTUnit* Builder = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0, 0));
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

	ARTUnit* Unit = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0, 0));
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

	ARTUnit* Unit = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(2, -2, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit) { DestroyPointerWorld(World); return false; }

	PC->SelectActorForTest(Unit);

	// Con un percorso pianificato lo stile diventa `Budget`: restano solo le tre direzioni dell'ultimo passo.
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
		ERTMovementStyle::Budget, Unit->PlannedPath, Unit->Facing);
	if (!TestTrue(TEXT("premessa: a budget le direzioni legali sono meno di sei"), Legal.Num() < 6))
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


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCycleDeclaredFacingTest,
	"RefactorTactics.Pointer.CycleDeclaredFacingStaysWithinTheLegalSet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCycleDeclaredFacingTest::RunTest(const FString&)
{
	UWorld* World = MakePointerWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	ARTUnit* Unit = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit) { DestroyPointerWorld(World); return false; }

	PC->SelectActorForTest(Unit);

	// 🔴 **Il difetto che questo test esiste per fermare non era un bug: era un'ASSENZA.** Le regole della
	// rotazione dichiarata erano complete e testate dal 2026-08-09, e `BeginFacingDeclaration` /
	// `HandleFacingSector` avevano come unici chiamanti dei test: dal gioco non ci arrivava nessuno.

	// Da ferma le direzioni legali sono SEI: il ciclo deve poterle raggiungere tutte e tornare al punto di
	// partenza. Un'implementazione che dichiarasse sempre la stessa direzione passerebbe un controllo
	// scritto solo su «dopo la pressione c'e' una dichiarazione».
	const TArray<ERTHexDirection> Legali =
		URTFacingLibrary::LegalFacings(ERTMovementStyle::None, Unit->PlannedPath, Unit->Facing);
	TestEqual(TEXT("da ferma le legali sono sei"), Legali.Num(), 6);

	TSet<ERTHexDirection> Viste;
	for (int32 I = 0; I < Legali.Num(); ++I)
	{
		PC->CycleDeclaredFacing();
		if (!TestTrue(*FString::Printf(TEXT("pressione %d: qualcosa e' stato dichiarato"), I + 1),
			Unit->bDeclaresPlannedFacing))
		{
			break;
		}

		// Ogni direzione che il ciclo produce e' NELL'INSIEME LEGALE. E' la garanzia che sostituisce
		// l'indicatore a schermo (`#613`): una direzione illegale non e' raggiungibile.
		TestTrue(*FString::Printf(TEXT("pressione %d: %d e' legale"), I + 1, (int32)Unit->PlannedFacing),
			Legali.Contains(Unit->PlannedFacing));
		Viste.Add(Unit->PlannedFacing);
	}

	// E le raggiunge TUTTE: se il ciclo si fermasse su due direzioni alternate, i controlli sopra
	// resterebbero verdi mentre meta' delle rotazioni sarebbe irraggiungibile.
	TestEqual(TEXT("sei pressioni raggiungono tutte e sei le direzioni"), Viste.Num(), Legali.Num());

	// Il contesto non resta aperto: `HandleFacingSector` lo chiude, e un contesto appeso mangerebbe il
	// click successivo.
	TestEqual(TEXT("il contesto e' tornato al neutro"), PC->GetPointerContext(), ERTPointerContext::Planning);

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

	ARTUnit* Unit = SpawnPointerUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(0, 0, 0));
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
	Unit->PlannedPath = { FRTCellId(0, 0, 0), FRTCellId(0, 1, 0) };
	TestNotEqual(TEXT("e il percorso porta ALTROVE"),
		URTFacingLibrary::FacingFromPath(Unit->PlannedPath, Unit->Facing), Unit->Facing);
	PC->PreviewPlannedFacing(Unit);
	const ERTHexDirection Derivato = URTFacingLibrary::FacingFromPath(Unit->PlannedPath, Unit->Facing);
	TestEqual(TEXT("la mesh guarda dove il percorso la portera'"),
		static_cast<float>(Unit->GetActorRotation().Yaw), YawPer(Derivato), 0.5f);

	// (2) E il facing LOGICO non e' cambiato: l'anteprima e' presentazione, e le regole restano quelle di
	// prima fino a fine Move. Senza questo controllo, l'anteprima potrebbe scrivere sullo stato e nessuno
	// se ne accorgerebbe finche' una difesa direzionale non desse l'esito sbagliato.
	TestEqual(TEXT("il facing logico NON e' stato toccato"), Unit->Facing, ERTHexDirection::E);

	// (3) Una rotazione dichiarata VINCE sul derivato, come a fine Move nel TurnManager: se le due regole
	// divergessero, l'anteprima mostrerebbe una direzione e il turno ne produrrebbe un'altra.
	PC->BeginFacingDeclaration();
	const TArray<ERTHexDirection> Legali =
		URTFacingLibrary::LegalFacings(ERTMovementStyle::Budget, Unit->PlannedPath, Unit->Facing);
	const ERTHexDirection Scelta = Legali.Last();   // una legale diversa dal derivato, quando ce n'e' piu' d'una
	if (TestTrue(TEXT("la dichiarazione e' accettata"), PC->HandleFacingSector(Scelta)))
	{
		TestEqual(TEXT("la mesh segue la rotazione dichiarata"),
			static_cast<float>(Unit->GetActorRotation().Yaw), YawPer(Scelta), 0.5f);
	}

	DestroyPointerWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
