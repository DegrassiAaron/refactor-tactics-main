// Interazioni del giocatore guidate headless: seleziona, clicca celle, annulla. Non serve un viewport perche'
// ARTPlayerController::HandleClickOnCell riceve la CELLA gia' risolta (il raycast, che il viewport richiede,
// resta in OnSelect). Cio' che questi test NON possono provare e' che a schermo si VEDA qualcosa: l'esagono
// giallo, l'anteprima ciano, le unita' centrate. Quella meta' resta al PIE, per costruzione.

#include "Misc/AutomationTest.h"
#include "Player/RTPlayerController.h"
#include "Player/Planning/RTFacingDeclaration.h" // CellFor: la cella del verso con scatto + Move (CP 38.16)
#include "Player/Input/RTInputKeys.h"
#include "Turn/RTTurnManager.h"
#include "Turn/Log/RTTurnLog.h"
#include "Turn/Log/RTTurnLogLibrary.h"
#include "Turn/Format/RTMatchSetupLibrary.h"
#include "Tests/Shared/RTWorldFixtures.h" // #3400: PlayOneTurn, per leggere il campione a turno chiuso
#include "Tests/Shared/RTAbilityFixtures.h" // #3400: AddCoreAbility, per la mobilita' sintetica a budget
#include "Unit/RTUnit.h"
#include "Ability/RTActionData.h"
#include "Turn/Resolution/RTMovementActionLibrary.h"
#include "Ability/RTMovementProfileLibrary.h" // il profilo Withdraw, di cui il test ricava il budget
#include "Turn/Actions/RTPlanValidationLibrary.h" // MakePlanFor: la riserva e la banda si leggono dal PIANO
#include "Pathfinding/RTHexPath.h" // il FRTHexPathResult che il test del troncamento riceve
#include "Turn/Resolution/RTHexSimLibrary.h" // #1939: lo snapshot e la sua Occupancy, che il test del rifiuto costruisce
#include "Map/Actor/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/Cover/RTHexCoverLibrary.h" // [D-524]: la copertura che chiude la retta dello scatto
#include "Map/RTHexLibrary.h" // #1776: WorldToLayer e ResolveRayToCellOnLayer, che i due test della review interrogano
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Ability/Catalog/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/Catalog/RTCatalogLibrary.h"
#include "InputMappingContext.h" // #1771: il test delle collisioni interroga i MapKey reali
#include "UI/HUD/RTHudViewModel.h"          // [D-516]: gli avvisi di piano e gli intenti della carica oltre la portata
#include "UI/HUD/RTHUD.h"                   // [D-516]: ComposeDashPreview, il tratto che la carica non fara'
#include "Turn/Actions/RTIntentPrivacyLibrary.h" // [D-516]: la vista filtrata che l'HUD disegna
#include "Perception/RTTeamKnowledge.h" // [D-516]: ClassifyTarget, chi la squadra conosce
#include "Player/RTPointerInteraction.h" // ERTPointerContext
#include "Core/RTGameplayTags.h"        // TAG_Status_Root: una carica senza portata non ruba il click
#include "Turn/Playback/RTPlanPreview.h"         // [D-516]: la timeline del piano posata sulla mappa
#include "Perception/RTEnemyArcLibrary.h"   // [D-528]: l'arco di un nemico sotto il cursore, che la carica armata non mostra
#include "Perception/Veil/RTVisibilityBorder.h"  // [D-528]: i lati esposti con cui si costruisce un arco di prova
#include "Tests/Shared/RTUnitPlanTestAccess.h"
#include "Replay/RTReplayPrivacyLibrary.h"        // E38 CP 38.10: la traccia dell'osservatore avversario
#include "Turn/Actions/RTActionFallbackLibrary.h"  // E38 CP 38.10: i motivi del piano svuotato
#include "ScenarioHarness/Authoring/RTScenarioLoader.h"   // E38 CP 38.10: i due scenari del lock-in
#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/Report/RTTestResult.h"
#include "Turn/Log/RTMatchStateHash.h"          // E38 CP 38.12: lo stato finale dei due ordini di dichiarazione
#include "Turn/Resolution/RTResolvedEvent.h"      // E38 CP 38.12: le attivazioni dei due ordini

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	UWorld* MakeInteractionWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyInteractionWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/**
	 * Esagono pieno senza ostacoli, per i test che misurano una REGOLA del piano.
	 *
	 * `MakeTestArena` ha muri centrali, e una cella bloccata fa rifiutare il waypoint dal pathfinding prima
	 * che la regola in esame abbia voce: un test cosi' resta verde anche togliendo cio' che dice di
	 * difendere. Misurato il 2026-08-26 — `«Waypoint rifiutato: cella bloccata»` con la validazione
	 * disattivata, e il test verde lo stesso.
	 */
	ARTHexMapActor* SpawnCleanInteractionMap(UWorld* World, int32 Radius)
	{
		if (!World)
		{
			return nullptr;
		}
		// `World` come Outer, non il transient package: e' la stessa disciplina di
		// `URTMatchSetupLibrary::MakeTestArena`, che rifiuta un Outer nullo invece di inventarsene uno.
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(World, Radius);
		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		if (!Actor)
		{
			return nullptr;
		}
		Actor->MapAsset = M;
		return Actor;
	}

	ARTUnit* SpawnInteractionUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
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
}

/**
 * La sequenza che in PIE si fa col mouse: seleziona un'unita', clicca celle valide (l'anteprima si allunga),
 * clicca una cella oltre il budget (rifiutata, il piano precedente resta), clicca una cella bloccata
 * (rifiutata), poi annulla. Copre la META' LOGICA di PIE-HEXPLAY-3: quel che resta al PIE e' vedere l'anteprima.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerWaypointInteractionTest,
	"RefactorTactics.PlayerInput.WaypointClicksBuildAndRejectPlans",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerWaypointInteractionTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	// Ranger: 5 punti movimento. Parte in una zona libera del quadrante destro.
	ARTUnit* Unit = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(2, -2, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("unita'"), Unit))
	{
		DestroyInteractionWorld(World); return false;
	}

	// Senza selezione un click su cella non deve fare nulla (nessun piano fantasma).
	PC->HandleClickOnCell(FRTCellId(3, -2, 0));
	TestEqual(TEXT("nessuna selezione -> nessun waypoint"), Unit->GetPlan().PlannedWaypoints().Num(), 0);

	PC->SelectActorForTest(Unit);

	// Primo click valido: un waypoint, percorso costruito.
	PC->HandleClickOnCell(FRTCellId(3, -2, 0));
	TestEqual(TEXT("primo click -> 1 waypoint"), Unit->GetPlan().PlannedWaypoints().Num(), 1);
	TestTrue(TEXT("il percorso parte dalla cella dell'unita'"),
		Unit->GetPlan().PlannedPath().Num() >= 2 && Unit->GetPlan().PlannedPath()[0] == FRTCellId(2, -2, 0));
	TestTrue(TEXT("il percorso finisce sulla cella cliccata"),
		Unit->GetPlan().PlannedPath().Num() >= 2 && Unit->GetPlan().PlannedPath().Last() == FRTCellId(3, -2, 0));

	// Secondo click valido: l'anteprima si allunga.
	const int32 LenAfterOne = Unit->GetPlan().PlannedPath().Num();
	PC->HandleClickOnCell(FRTCellId(3, -1, 0));
	TestEqual(TEXT("secondo click -> 2 waypoint"), Unit->GetPlan().PlannedWaypoints().Num(), 2);
	TestTrue(TEXT("il percorso si e' allungato"), Unit->GetPlan().PlannedPath().Num() > LenAfterOne);

	// Click su cella BLOCCATA: rifiutato, e il piano precedente resta intatto.
	{
		const TArray<FRTCellId> Before = Unit->GetPlan().PlannedPath();
		PC->HandleClickOnCell(FRTCellId(2, 1, 0)); // ostacolo della mappa di prova
		TestEqual(TEXT("cella bloccata -> waypoint non aggiunto"), Unit->GetPlan().PlannedWaypoints().Num(), 2);
		TestTrue(TEXT("il piano precedente e' intatto"), Unit->GetPlan().PlannedPath() == Before);
	}

	// Click FUORI dalla mappa: rifiutato allo stesso modo.
	{
		const TArray<FRTCellId> Before = Unit->GetPlan().PlannedPath();
		PC->HandleClickOnCell(FRTCellId(99, 99, 0));
		TestEqual(TEXT("cella fuori mappa -> waypoint non aggiunto"), Unit->GetPlan().PlannedWaypoints().Num(), 2);
		TestTrue(TEXT("il piano precedente e' intatto"), Unit->GetPlan().PlannedPath() == Before);
	}

    // Click ripetuti finche' il budget si esaurisce: a un certo punto il rifiuto e' per BUDGET, non per la cella.
	{
		// La lista dei click deve superare il budget di CHIUNQUE si stia muovendo: con quattro celle bastava
		// il budget 5 del Ranger legacy, con 6 nessun click veniva piu' rifiutato e il test verificava il
		// nulla. Si allunga finche' non eccede il budget effettivo.
		int32 Rejected = 0;
		TArray<FRTCellId> Far = { FRTCellId(4, -1, 0), FRTCellId(4, 0, 0), FRTCellId(3, 1, 0), FRTCellId(2, 2, 0) };
		for (int32 Q = 1; Far.Num() <= Unit->GetEffectiveMoveRange(); ++Q)
		{
			Far.Add(FRTCellId(2 - Q, 2 + Q, 0));
		}
		for (const FRTCellId& C : Far)
		{
			const int32 Before = Unit->GetPlan().PlannedWaypoints().Num();
			PC->HandleClickOnCell(C);
			if (Unit->GetPlan().PlannedWaypoints().Num() == Before) { ++Rejected; }
		}
		TestTrue(TEXT("il budget finisce e i click successivi vengono rifiutati"), Rejected > 0);
		TestTrue(TEXT("il piano resta coerente col budget"),
			Unit->GetPlan().PlannedPath().Num() == 0 || Unit->GetPlan().PlannedPath()[0] == FRTCellId(2, -2, 0));
	}

	DestroyInteractionWorld(World);
	return true;
}

/** Annullare l'ultimo waypoint accorcia il piano; annullando tutto si torna a "resto fermo". */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerUndoInteractionTest,
	"RefactorTactics.PlayerInput.UndoShortensThePlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerUndoInteractionTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	ARTUnit* Unit = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(2, -2, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit) { DestroyInteractionWorld(World); return false; }
	PC->SelectActorForTest(Unit);

	PC->HandleClickOnCell(FRTCellId(3, -2, 0));
	PC->HandleClickOnCell(FRTCellId(3, -1, 0));
	if (!TestEqual(TEXT("due waypoint pianificati"), Unit->GetPlan().PlannedWaypoints().Num(), 2))
	{
		DestroyInteractionWorld(World); return false;
	}
	const int32 LenTwo = Unit->GetPlan().PlannedPath().Num();

	// Un annullamento: un waypoint in meno e percorso piu' corto.
	Unit->PopPlannedWaypoint();
	PC->RebuildPlannedPathForTest();
	TestEqual(TEXT("un waypoint dopo l'annullamento"), Unit->GetPlan().PlannedWaypoints().Num(), 1);
	TestTrue(TEXT("il percorso si e' accorciato"), Unit->GetPlan().PlannedPath().Num() < LenTwo);

	// Annullando l'ultimo: nessun movimento pianificato.
	Unit->PopPlannedWaypoint();
	PC->RebuildPlannedPathForTest();
	TestEqual(TEXT("nessun percorso pianificato"), Unit->GetPlan().PlannedPath().Num(), 0);
	TestTrue(TEXT("la destinazione torna la cella attuale"), Unit->GetPlan().PlannedCell == Unit->Cell);

	DestroyInteractionWorld(World);
	return true;
}


namespace
{
	/** L'indice della carica (`LinearCharge`) nel kit: si CERCA per stile, l'indice e' una convenzione del kit. */
	int32 FindChargeAbilityIndex(const ARTUnit* Unit)
	{
		for (int32 i = 0; Unit && i < Unit->NumAbilities(); ++i)
		{
			const URTActionData* A = Unit->GetAbility(i);
			if (A && A->Def.MovementStyle == ERTMovementStyle::LinearCharge)
			{
				return i;
			}
		}
		return INDEX_NONE;
	}

	/**
	 * Terreno accidentato su una cella della mappa di prova: nega la corsa (`bBlocksDashCharge`) e lascia passare
	 * la vista. Nome distinto per file (unity build).
	 */
	void MakeInteractionRough(URTHexMapAsset* Map, const FRTCellId& Id)
	{
		FRTHexCellData Data = Map->FindCell(Id) ? *Map->FindCell(Id) : FRTHexCellData(Id);
		Data.Surface = ERTHexSurface::Rough;
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}
}

/**
 * Con una CARICA armata, il click VERO su un nemico osservato la pianifica contro di lui ([D-516], `#3736`).
 *
 * Prima di #145 il click veniva rifiutato con «lo scatto si pianifica su una CELLA, non su un nemico». Da #145
 * il ramo che inoltra la carica alla cella del nemico c'era, ma fino a `#3736` lo raggiungeva solo
 * `HandleClickOnUnitForTest`: questo test entrava da li', ed era verde mentre dal click vero `ResolveOutcome`
 * rispondeva `Inspect` — la carica e' mobilita' rapida e resta fuori da `Targeting`. Ora entra da
 * `DispatchUnitClick`, la porta che `OnSelect` attraversa dopo il raycast.
 *
 * Il rifiuto resta giusto per gli scatti che NON sono cariche: quelli si fermano davanti alle unita', e con uno
 * scatto armato il click sul nemico continua a ispezionarlo. ⌫ *Fino a `#3736` quella meta' armava l'indice 3
 * di Ivrin, che dalla migrazione al roster e' `Deflection`, una reazione: asseriva su un'azione che non c'entra.*
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerChargeOnEnemyTest,
	"RefactorTactics.PlayerInput.ChargeIsPlannedByClickingTheEnemy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerChargeOnEnemyTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	// Riktor e il bersaglio allineati sull'asse q, a distanza 3: dentro la portata della carica.
	ARTUnit* Charger = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0, 0));
	ARTUnit* Enemy   = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),   FRTCellId(3, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Charger || !Enemy || !TM) { DestroyInteractionWorld(World); return false; }

	// 🔑 La conoscenza di squadra: senza, la fotografia di chi pianifica non contiene il nemico ([D-371]).
	TM->RefreshTeamKnowledgeNow();

	const int32 ChargeIdx = FindChargeAbilityIndex(Charger);
	if (!TestNotEqual(TEXT("premessa: Riktor ha una carica nel kit"), ChargeIdx, (int32)INDEX_NONE))
	{
		DestroyInteractionWorld(World); return false;
	}
	TestTrue(TEXT("premessa: il nemico e' noto a chi guarda"), Enemy->IsKnownToObserver());
	TestTrue(TEXT("premessa: in portata"),
		URTHexLibrary::HexDistance(Charger->Cell, Enemy->Cell) <= Charger->EffectiveDashRangeOf(ChargeIdx));

	PC->SelectActorForTest(Charger);
	Charger->SelectAbility(ChargeIdx);
	TestTrue(TEXT("premessa: con la carica armata il contesto resta Planning, fuori da Targeting"),
		PC->GetPointerContext() == ERTPointerContext::Planning);

	TestTrue(TEXT("il click sul nemico e' consumato"), PC->DispatchUnitClickForTest(Enemy, Charger));
	TestEqual(TEXT("il click sul nemico pianifica la carica"), Charger->GetPlan().PlannedDashAbility(), ChargeIdx);
	TestTrue(TEXT("verso la cella del nemico"), Charger->GetPlan().PlannedDashCell() == Enemy->Cell);
	TestTrue(TEXT("e non lo ispeziona"), PC->GetInspectedUnit() != Enemy);
	// La carica NON e' un attacco del Blast: l'impatto lo produce la fase Dash. Pianificare anche un attacco
	// significherebbe colpire due volte con la stessa azione.
	TestEqual(TEXT("e non pianifica anche un attacco separato"),
		Charger->GetPlan().PlannedAbilityIndex(), (int32)INDEX_NONE);
	// In portata: nessun tratto da percorrere a vuoto, e nessun avviso ([D-516] tace).
	TestEqual(TEXT("in portata la carica non si ferma prima"), Charger->PlannedChargeReachBeyondRange(), 0);

	// La carica punta i NEMICI: un'alleata che non comandi — qui guidata dal bot — si ispeziona anche con la
	// carica armata, come prima.
	ARTUnit* Alleata = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-1, 0, 0));
	if (!TestNotNull(TEXT("premessa: l'alleata"), Alleata)) { DestroyInteractionWorld(World); return false; }
	Alleata->bIsBotControlled = true;
	PC->SelectActorForTest(Charger);
	Charger->SelectAbility(ChargeIdx);
	TestTrue(TEXT("il click sull'alleata e' consumato"), PC->DispatchUnitClickForTest(Alleata, Charger));
	TestTrue(TEXT("e la ispeziona: con la carica armata un'alleata non e' un bersaglio"),
		PC->GetInspectedUnit() == Alleata);
	TestTrue(TEXT("e la carica resta quella sul nemico"), Charger->GetPlan().PlannedDashCell() == Enemy->Cell);

	// Una carica che adesso non puo' pianificare non ruba il click (seconda revisione della PR #3767): sotto `Root` la
	// portata e' zero, e col verso dichiarato il movimento e' chiuso. In tutti e due i casi il nemico si ispeziona.
	Charger->ApplyStatus(TAG_Status_Root, 1);
	TestEqual(TEXT("premessa: sotto Root la carica non ha portata"), Charger->EffectiveDashRangeOf(ChargeIdx), 0);
	PC->DispatchUnitClickForTest(Enemy, Charger);
	TestTrue(TEXT("sotto Root il nemico si ispeziona"), PC->GetInspectedUnit() == Enemy);
	Charger->RemoveStatus(TAG_Status_Root);
	PC->DispatchUnitClickForTest(Alleata, Charger);
	FRTUnitPlanTestAccess::SetDeclaresFacing(*Charger, true);
	PC->DispatchUnitClickForTest(Enemy, Charger);
	TestTrue(TEXT("col verso dichiarato il nemico si ispeziona"), PC->GetInspectedUnit() == Enemy);
	Charger->RetractPlannedFacing();
	PC->DispatchUnitClickForTest(Alleata, Charger);

	// Uno scatto che NON e' una carica si ferma davanti alle unita': puntarne una resta senza senso.
	ARTUnit* Dasher = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-3, 0, 0));
	const int32 DashIdx = Dasher ? Dasher->FindDashAbilityIndex() : INDEX_NONE;
	const URTActionData* Dash = Dasher ? Dasher->GetAbility(DashIdx) : nullptr;
	if (!TestNotNull(TEXT("premessa: Ivrin ha una mobilita' rapida"), (void*)Dash))
	{
		DestroyInteractionWorld(World); return false;
	}
	TestTrue(TEXT("premessa: e non e' una carica"), Dash->Def.MovementStyle != ERTMovementStyle::LinearCharge);
	PC->SelectActorForTest(Dasher);
	Dasher->SelectAbility(DashIdx);
	PC->DispatchUnitClickForTest(Enemy, Dasher);
	TestEqual(TEXT("uno scatto non-carica sul nemico non si pianifica"),
		Dasher->GetPlan().PlannedDashAbility(), (int32)INDEX_NONE);
	TestTrue(TEXT("e il nemico si ispeziona, come prima"), PC->GetInspectedUnit() == Enemy);

	DestroyInteractionWorld(World);
	return true;
}

/**
 * [D-523] / [D-524] (`#3793`) — **con la carica armata, un nemico fuori dalle sei direzioni ma sulla retta libera si
 * carica**: e' la scena di `PIE-V01-CHARGECLICK` (4), Riktor in `(-4,3)` e Muiren in `(-2,4)`, scarto `(+2,+1)`.
 * Se la retta e' bloccata il click rifiuta la carica, e non ispeziona: il click vuol dire «carica li'».
 *
 * ⌫ *Fino a [D-524] questo test si chiamava `ChargeOnAnEnemyOutOfLineIsRefused` e asseriva il rifiuto di una cella a
 * `(2,1)`.* Il nome diceva il contrario della regola, quindi e' cambiato con lei.
 *
 * ⚠️ **La retta bloccata e' un terreno che nega la corsa, non un muro o un bordo alto**: quelli tolgono anche la
 * vista, il nemico cadrebbe nel rifiuto «ignoto alla squadra» ([D-371]) prima di arrivare alla regola della retta.
 * Il bordo che ferma lo scatto si prova alla libreria (`Dash.AnEdgeStopsDashChargeAndPassButNotTheLeap`) e alla
 * partita (`HexMatch.ChargeIsStoppedByAnEdgeOnTheLine`).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerChargeOffAxesTest,
	"RefactorTactics.PlayerInput.ChargeOnAnEnemyOffTheAxesFollowsTheFreeLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerChargeOffAxesTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTHexMapActor* MapActor = SpawnCleanInteractionMap(World, /*Radius=*/ 6);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTUnit* Charger = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(-4, 3, 0));
	ARTUnit* Enemy = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(-2, 4, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!MapActor || !PC || !Charger || !Enemy || !TM) { DestroyInteractionWorld(World); return false; }
	TM->RefreshTeamKnowledgeNow();

	int32 Distanza = 0;
	TestFalse(TEXT("premessa: il nemico non e' su una delle sei direzioni"),
		URTMovementActionLibrary::IsStraightLine(Charger->Cell, Enemy->Cell, Distanza));

	const int32 ChargeIdx = FindChargeAbilityIndex(Charger);
	if (!TestNotEqual(TEXT("premessa: Riktor ha una carica nel kit"), ChargeIdx, (int32)INDEX_NONE))
	{
		DestroyInteractionWorld(World); return false;
	}
	TestTrue(TEXT("premessa: Muiren e' a distanza 3, entro la portata della carica"),
		URTHexLibrary::HexDistance(Charger->Cell, Enemy->Cell) == 3
		&& URTHexLibrary::HexDistance(Charger->Cell, Enemy->Cell) <= Charger->EffectiveDashRangeOf(ChargeIdx));
	TestTrue(TEXT("premessa: Muiren e' noto a chi guarda"), Enemy->IsKnownToObserver());

	PC->SelectActorForTest(Charger);
	Charger->SelectAbility(ChargeIdx);

	// (1) La retta e' libera: il click pianifica la carica su di lui, e non lo ispeziona.
	TestTrue(TEXT("il click e' consumato dalla carica"), PC->DispatchUnitClickForTest(Enemy, Charger));
	TestEqual(TEXT("la carica su Muiren si pianifica"), Charger->GetPlan().PlannedDashAbility(), ChargeIdx);
	TestTrue(TEXT("verso la sua cella"), Charger->GetPlan().PlannedDashCell() == Enemy->Cell);
	TestTrue(TEXT("e il nemico non si ispeziona"), PC->GetInspectedUnit() != Enemy);
	TestEqual(TEXT("in portata: nessun tratto da percorrere a vuoto"), Charger->PlannedChargeReachBeyondRange(), 0);

	// (2) Il verso dopo la carica e' l'ULTIMO PASSO PERCORSO: la carica si ferma in (-3,4), dopo (-3,3), cioe' SE.
	// Verso Muiren (E) non e' l'ultimo passo percorso: e' la direzione dell'impatto, e il facing non la usa. ⚠️ Il
	// facing di partenza e' un altro (`NW`) per distinguere questo caso dal «resta com'e'» di una rotta vuota, che e'
	// quanto `FRTFacingDeclaration::PlannedMovement` rispondeva su un bersaglio fuori asse.
	Charger->Facing = ERTHexDirection::NW;
	TestTrue(TEXT("il verso SE, ultimo passo della rotta percorsa, e' legale"),
		PC->IsFacingLegalForPlan(Charger, ERTHexDirection::SE));
	TestFalse(TEXT("il verso NE non lo e' (non e' ne' l'ultimo passo percorso ne' un suo vicino)"),
		PC->IsFacingLegalForPlan(Charger, ERTHexDirection::NE));

	// (3) La retta e' BLOCCATA da un terreno che nega la corsa: la carica e' rifiutata, e il nemico non si ispeziona.
	Charger->ClearPlannedDash();
	MakeInteractionRough(MapActor->MapAsset, FRTCellId(-3, 4, 0));
	PC->SelectActorForTest(Charger);
	Charger->SelectAbility(ChargeIdx);
	TestTrue(TEXT("il click e' consumato dalla carica anche quando e' rifiutata"),
		PC->DispatchUnitClickForTest(Enemy, Charger));
	TestEqual(TEXT("con (-3,4) accidentata la retta non si percorre di corsa: carica rifiutata"),
		Charger->GetPlan().PlannedDashAbility(), (int32)INDEX_NONE);
	TestTrue(TEXT("e il nemico non si ispeziona"), PC->GetInspectedUnit() != Enemy);

	DestroyInteractionWorld(World);
	return true;
}

/**
 * [D-516] (`#3736`) — **una carica su un nemico in linea ma OLTRE la portata si pianifica, e lo dice**: l'avviso
 * di piano (`Warning`, [D-480]) e l'anteprima, piena fino alla portata e col tratto che la carica non fara'.
 *
 * Tre rifiuti nella stessa scena, perche' la regola ha tre condizioni e ognuna va vista cadere:
 * - **il nemico dev'essere noto**: prima di allestire la conoscenza la sua cella e' vuota per chi pianifica, e la
 *   carica oltre la portata si rifiuta come su una cella vuota. E' anche cio' che impedisce al gesto di dire
 *   «li' c'e' qualcuno» ([D-371]);
 * - **una cella vuota oltre la portata resta rifiutata**: la regola e' «caricare un nemico»;
 * - **la retta fino al nemico dev'essere libera dal terreno**, anche oltre la portata. ⚠️ Con un terreno che
 *   nega la corsa e non un muro: un muro toglierebbe anche la vista, e il nemico cadrebbe nel primo rifiuto
 *   invece che in questo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerChargeBeyondRangeTest,
	"RefactorTactics.PlayerInput.ChargeBeyondRangeIsPlannedWithAWarning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerChargeBeyondRangeTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTHexMapActor* MapActor = SpawnCleanInteractionMap(World, /*Radius=*/ 8);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTUnit* Charger = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!MapActor || !TM || !Charger || !PC) { DestroyInteractionWorld(World); return false; }

	const int32 ChargeIdx = FindChargeAbilityIndex(Charger);
	const int32 Portata = Charger->EffectiveDashRangeOf(ChargeIdx);
	if (!TestTrue(TEXT("premessa: la carica ha una portata, e la mappa ne contiene due celle in piu'"),
		ChargeIdx != INDEX_NONE && Portata >= 2 && Portata + 2 <= 7))
	{
		DestroyInteractionWorld(World); return false;
	}
	const FName CaricaId = Charger->GetAbility(ChargeIdx)->Def.ActionId;

	// Il bersaglio vero, due celle oltre la portata sull'asse q.
	ARTUnit* Enemy = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(Portata + 2, 0, 0));
	// Un secondo nemico sull'altra diagonale davanti — alle spalle la squadra non lo vedrebbe —, con un terreno
	// che nega la corsa fra la portata e lui.
	ARTUnit* Ostacolato = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(Portata + 2, -(Portata + 2), 0));
	MakeInteractionRough(MapActor->MapAsset, FRTCellId(Portata + 1, -(Portata + 1), 0));
	if (!Enemy || !Ostacolato) { DestroyInteractionWorld(World); return false; }

	PC->SelectActorForTest(Charger);
	Charger->SelectAbility(ChargeIdx);

	// (1) Nemico non ancora noto alla squadra: per chi pianifica la sua cella e' vuota.
	TestTrue(TEXT("controllo: prima di allestirla, la squadra non conosce il nemico"),
		URTTeamKnowledgeLibrary::ClassifyTarget(TM->KnowledgeForTeamPublic(0), Enemy->StableUnitId, Enemy->TeamId,
			Enemy->Cell) == ERTTargetKnowledge::Rejected);
	PC->DispatchUnitClickForTest(Enemy, Charger);
	TestEqual(TEXT("un nemico che la squadra non conosce non si carica oltre la portata"),
		Charger->GetPlan().PlannedDashAbility(), (int32)INDEX_NONE);

	TM->RefreshTeamKnowledgeNow();
	TestTrue(TEXT("premessa: allestita la conoscenza, la squadra vede il nemico"),
		URTTeamKnowledgeLibrary::ClassifyTarget(TM->KnowledgeForTeamPublic(0), Enemy->StableUnitId, Enemy->TeamId,
			Enemy->Cell) == ERTTargetKnowledge::Allowed);

	// (2) Una cella vuota oltre la portata, in linea.
	PC->HandleClickOnCellForTest(FRTCellId(0, Portata + 2, 0));
	TestEqual(TEXT("una cella vuota oltre la portata resta rifiutata"),
		Charger->GetPlan().PlannedDashAbility(), (int32)INDEX_NONE);

	// (3) Il nemico dietro un terreno che nega la corsa, OLTRE la portata.
	TestTrue(TEXT("premessa: la squadra vede anche lui"),
		URTTeamKnowledgeLibrary::ClassifyTarget(TM->KnowledgeForTeamPublic(0), Ostacolato->StableUnitId,
			Ostacolato->TeamId, Ostacolato->Cell) == ERTTargetKnowledge::Allowed);
	PC->DispatchUnitClickForTest(Ostacolato, Charger);
	TestEqual(TEXT("una retta che non si percorre di corsa, anche oltre la portata, rifiuta la carica"),
		Charger->GetPlan().PlannedDashAbility(), (int32)INDEX_NONE);

	// (4) Un nemico oltre la portata con un altro nemico noto sulla retta ENTRO la portata (review di #3736). La corsa
	// verso il lontano incontra il vicino e si ferma con un impatto. ⏱️ *Fino a [D-528] era rifiutato*: era l'ultima clausola
	// di [D-516] (3), che il caso non lo copriva. Ora si pianifica sul lontano, con l'avviso di chi colpira' (il perche' e il
	// come sono di `ChargeWithAKnownEnemyInTheWayIsPlannedAndSaysWhoItHits`). Sulla diagonale SE, dove la cella vuota e' gia'
	// stata provata.
	ARTUnit* Vicino = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 2, 0));
	ARTUnit* Lontano = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, Portata + 2, 0));
	if (!Vicino || !Lontano) { DestroyInteractionWorld(World); return false; }
	TM->RefreshTeamKnowledgeNow();
	TestTrue(TEXT("premessa: la squadra vede il vicino e il lontano"),
		URTTeamKnowledgeLibrary::ClassifyTarget(TM->KnowledgeForTeamPublic(0), Vicino->StableUnitId, Vicino->TeamId,
			Vicino->Cell) == ERTTargetKnowledge::Allowed
		&& URTTeamKnowledgeLibrary::ClassifyTarget(TM->KnowledgeForTeamPublic(0), Lontano->StableUnitId, Lontano->TeamId,
			Lontano->Cell) == ERTTargetKnowledge::Allowed);
	PC->DispatchUnitClickForTest(Lontano, Charger);
	TestTrue(TEXT("un nemico oltre la portata dietro un nemico noto entro la portata si pianifica ([D-528])"),
		Charger->GetPlan().PlannedDashAbility() == ChargeIdx && Charger->GetPlan().PlannedDashCell() == Lontano->Cell);
	Charger->ClearPlannedDash();
	Charger->SelectAbility(ChargeIdx);
	PC->DispatchUnitClickForTest(Vicino, Charger);
	TestTrue(TEXT("controllo: il vicino, in portata, si carica"),
		Charger->GetPlan().PlannedDashAbility() == ChargeIdx && Charger->GetPlan().PlannedDashCell() == Vicino->Cell);
	// Si toglie il piano. Vicino e lontano stanno sulla diagonale SE, fuori dalla retta E del caso qui sotto.
	Charger->ClearPlannedDash();
	Charger->SelectAbility(ChargeIdx);

	// Il caso della regola.
	TestTrue(TEXT("il click sul nemico oltre la portata e' consumato"), PC->DispatchUnitClickForTest(Enemy, Charger));
	TestEqual(TEXT("e la carica si pianifica"), Charger->GetPlan().PlannedDashAbility(), ChargeIdx);
	TestTrue(TEXT("verso la sua cella"), Charger->GetPlan().PlannedDashCell() == Enemy->Cell);
	TestEqual(TEXT("correra' per tutta la portata"), Charger->PlannedChargeReachBeyondRange(), Portata);

	// L'avviso: uno, `Warning`, attribuito alla carica, coi due numeri.
	const TArray<FRTPlanWarningView> Avvisi = URTHudViewModel::BuildPlanWarnings(Charger, { Charger, Enemy, Ostacolato });
	const FRTPlanWarningView* Corta = Avvisi.FindByPredicate([&CaricaId](const FRTPlanWarningView& A)
	{
		return A.SourceActionId == CaricaId;
	});
	if (TestNotNull(TEXT("un avviso attribuito alla carica"), Corta))
	{
		TestTrue(TEXT("di livello Warning: il piano e' legale e degradato"), Corta->Level == ERTPlanWarningLevel::Warning);
		TestTrue(TEXT("il perche' dice la distanza"), Corta->Why.ToString().Contains(FString::FromInt(Portata + 2)));
		TestTrue(TEXT("il costo dice dove si ferma"), Corta->Cost.ToString().Contains(FString::FromInt(Portata)));
	}
	FRTPlanWarningCounts Contatore;
	URTHudViewModel::AddPlanWarningCounts(Avvisi, Contatore);
	TestEqual(TEXT("il contatore accanto a Conferma lo conta fra i Warning"), Contatore.Warning, 1);

	// L'anteprima, dalla vista filtrata che l'HUD disegna.
	const TArray<FRTIntentView> Viste = URTIntentPrivacyLibrary::FilterForTeam(0,
		URTHudViewModel::BuildAuthoritativeIntents({ Charger, Enemy, Ostacolato }));
	const FRTIntentView* Mia = Viste.FindByPredicate([Charger](const FRTIntentView& V)
	{
		return V.OwnerCell == Charger->Cell;
	});
	if (TestNotNull(TEXT("la vista della propria carica"), Mia))
	{
		TestEqual(TEXT("la vista porta quante celle la carica percorre"), Mia->DashReachCells, Portata);
		const FRTDashPreview Anteprima = ARTHUD::ComposeDashPreview(*Mia, MapActor->MapAsset);
		TestTrue(TEXT("l'anteprima arriva fino al nemico"),
			Anteprima.PathCells.Num() > 0 && Anteprima.PathCells.Last() == Enemy->Cell);
		TestEqual(TEXT("ed e' corsa per i soli tratti della portata"), Anteprima.ReachSegments, Portata);
		TestTrue(TEXT("il resto e' il tratto che la carica non fara'"), Anteprima.HasShortfall());
		TestTrue(TEXT("e il segno d'arrivo sta dove si ferma"), Anteprima.StopCell() == FRTCellId(Portata, 0, 0));
	}

	// La timeline del piano, quella che il controller compone e posa sulla mappa: il fantasma dello scatto si ferma
	// alla portata. ⚠️ **Letta subito dopo il click, senza altri gesti** (seconda revisione della PR #3767): una prima
	// stesura la ricomponeva riarmando l'azione, perche' il click che pianifica lo scatto non la ricomponeva — e
	// nel gioco il giocatore quel gesto non lo fa.
	const FRTPhasePreviewEntry* FantasmaScatto = nullptr;
	for (const FRTPhasePreviewEntry& Voce : MapActor->GetPlanPreview().Phases)
	{
		if (Voce.Phase == ERTResolutionPhase::FastMovement) { FantasmaScatto = &Voce; }
	}
	if (TestNotNull(TEXT("la timeline posata porta la fase Dash"), FantasmaScatto))
	{
		TestTrue(TEXT("e il suo fantasma si ferma alla portata, non sul nemico"),
			FantasmaScatto->PreviewDestination == FRTCellId(Portata, 0, 0));
	}

	DestroyInteractionWorld(World);
	return true;
}

/**
 * Lo scatto del giocatore e' LINEARE come quello che il resolver esegue: una cella raggiungibile sul grafo ma la
 * cui RETTA e' bloccata va rifiutata in pianificazione, e una cella FUORI ASSE con la retta libera si accetta.
 *
 * Senza questo gate il piano verrebbe accettato e la fase Dash non muoverebbe nulla: il turno si perde in
 * silenzio, senza che nessuno dica perche'. E' lo stesso invariante che il bot ha da #140 ("non proporre mosse
 * che il resolver rifiuta"), qui applicato alla mano umana — diventato osservabile con #142, da quando lo
 * scatto degli archetipi dichiara `LinearDash` e non passa piu' dall'A*.
 *
 * ⌫ *Fino a [D-524] (#3793) questo test si chiamava `DashRejectsNonLinearDestination` e rifiutava `(3,-4)` perche'
 * non stava su una delle sei direzioni.* La retta di `HexLine` da `(2,-2)` a `(3,-4)` passa da `(3,-3)` ed e'
 * libera: ora si accetta. A fermare uno scatto fuori asse e' la retta, non l'asse — `(0,-3)` dista 3 e la sua
 * retta passa dal muro di `(1,-2)`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerDashIsLinearTest,
	"RefactorTactics.PlayerInput.DashFollowsTheFreeLineNotTheSixDirections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerDashIsLinearTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	ARTUnit* Unit = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(2, -2, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit) { DestroyInteractionWorld(World); return false; }

	// La mobilita' rapida si CERCA: l'indice 3 era lo Scatto del Ranger legacy, e dopo la migrazione al
	// roster e' `Ivrin.Deflection`, una reazione. `FindDashAbilityIndex` legge la fase dal catalogo, che e'
	// come il gioco stesso riconosce uno scatto (#142).
	const int32 DashIdx = Unit->FindDashAbilityIndex();
	const URTActionData* Dash = Unit->GetAbility(DashIdx);
	if (!TestNotNull(TEXT("premessa: chi scatta ha una mobilita' rapida nel kit"), (void*)Dash))
	{
		DestroyInteractionWorld(World); return false;
	}
	TestTrue(TEXT("premessa: lo scatto e' lineare"),
		URTMovementActionLibrary::IsLinear(Dash->Def.MovementStyle));
	PC->SelectActorForTest(Unit);
	Unit->SelectAbility(DashIdx);

	// (3,-4) e' libera, dentro la mappa e a due passi — e NON e' su una delle sei direzioni. Da [D-524] conta la
	// retta, e quella di HexLine passa da (3,-3): libera, quindi lo scatto si pianifica.
	const FRTCellId Oblique(3, -4, 0);
	int32 Distanza = 0;
	TestTrue(TEXT("premessa: la cella esiste ed e' libera"),
		Arena->ContainsCell(Oblique) && !Arena->FindCell(Oblique)->bBlocksMovement);
	TestFalse(TEXT("premessa: non e' su una delle sei direzioni"),
		URTMovementActionLibrary::IsStraightLine(Unit->Cell, Oblique, Distanza));
	PC->HandleClickOnCell(Oblique);
	TestEqual(TEXT("una destinazione fuori asse con la retta libera diventa un piano di scatto"),
		Unit->GetPlan().PlannedDashAbility(), DashIdx);
	TestTrue(TEXT("verso la cella cliccata"), Unit->GetPlan().PlannedDashCell() == Oblique);

	// Una cella fuori asse la cui retta e' BLOCCATA si rifiuta: da (2,-2) a (0,-3) la retta passa dal muro di (1,-2),
	// benche' il grafo ci arrivi girandoci attorno. Il piano di prima non si tocca.
	const FRTCellId Bloccata(0, -3, 0);
	TestTrue(TEXT("premessa: la cella e' libera, a distanza 3 (la portata della lama) e fuori asse"),
		Arena->ContainsCell(Bloccata) && !Arena->FindCell(Bloccata)->bBlocksMovement
		&& URTHexLibrary::HexDistance(Unit->Cell, Bloccata) == 3
		&& !URTMovementActionLibrary::IsStraightLine(Unit->Cell, Bloccata, Distanza));
	Unit->ClearPlannedDash();
	Unit->SelectAbility(DashIdx); // riarmato: il click precedente puo' aver lasciato lo stato neutro
	PC->HandleClickOnCell(Bloccata);
	TestEqual(TEXT("una retta che passa da un muro non diventa un piano di scatto"),
		Unit->GetPlan().PlannedDashAbility(), (int32)INDEX_NONE);
	Unit->SelectAbility(DashIdx);

	// Controprova: sulla stessa mappa, una cella ALLINEATA e libera viene accettata. Senza, il test passerebbe
	// anche con un gate che rifiuta tutto.
	const FRTCellId Aligned(0, 0, 0); // (2,-2) + 2 * direzione (-1,+1)
	PC->HandleClickOnCell(Aligned);
	TestEqual(TEXT("una destinazione in linea diventa un piano di scatto"), Unit->GetPlan().PlannedDashAbility(), DashIdx);
	TestTrue(TEXT("verso la cella cliccata"), Unit->GetPlan().PlannedDashCell() == Aligned);

	// [D-524] (5): anche un LATO chiude la retta. Sulla stessa cella allineata e libera, una copertura ALTA sul bordo
	// (1,-1)-(0,0) rifiuta lo scatto in pianificazione, e una porta chiusa sullo stesso bordo pure — mentre una copertura
	// BASSA no. E' la meta' di pianificazione di `PIE-V01-DASHCOVER`: a fermare lo scatto a risoluzione, se un altro
	// produttore lo scrive, e' il resolver (`HexMatch.ChargeIsStoppedByAnEdgeOnTheLine`).
	const FRTCellId PrimaDelBordo(1, -1, 0);
	ERTHexDirection Lato = ERTHexDirection::E;
	verify(URTHexLibrary::DirectionBetween(PrimaDelBordo, Aligned, Lato));
	auto ClickConBordo = [&](auto&& PosaIlBordo) -> bool
	{
		Unit->ClearPlannedDash();
		Unit->SelectAbility(DashIdx);
		PosaIlBordo();
		PC->HandleClickOnCell(Aligned);
		return Unit->GetPlan().PlannedDashAbility() == DashIdx;
	};

	URTHexCoverLibrary::AddCover(Arena, PrimaDelBordo, Lato, ERTHexCoverType::Low, FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low));
	TestTrue(TEXT("una copertura bassa sul bordo non chiude la retta"), ClickConBordo([]{}));
	URTHexCoverLibrary::RemoveCover(Arena, PrimaDelBordo, Lato);

	TestFalse(TEXT("una copertura alta sul bordo chiude la retta: lo scatto non si pianifica"), ClickConBordo([&]
	{
		URTHexCoverLibrary::AddCover(Arena, PrimaDelBordo, Lato, ERTHexCoverType::High,
			FRTHexCover::DefaultIntegrity(ERTHexCoverType::High));
	}));
	URTHexCoverLibrary::RemoveCover(Arena, PrimaDelBordo, Lato);

	TestFalse(TEXT("una porta chiusa sul bordo chiude la retta: lo scatto non si pianifica"), ClickConBordo([&]
	{
		FRTHexCellData Dato = *Arena->FindCell(PrimaDelBordo);
		Dato.Doors.Add(FRTHexDoor(Lato, ERTHexDoorState::Closed));
		Arena->AddOrUpdateCell(Dato);
		Arena->SortCells();
	}));

	DestroyInteractionWorld(World);
	return true;
}




// =====================================================================================================
// E38 CP 38.16 (#4117) — scatto e Move nello stesso piano: il Move parte dalla cella d'arrivo dello scatto.
//
// ⏱️ *Fino a CP 38.16 (CP 38.2) il giocatore poteva comporre scatto + movimento normale, due voci per lo slot movimento, e
// `ResolveDash` faceva vincere lo scatto: azzerava il percorso e lo diceva con una voce `Move/SupersededByDash`.* Questo test
// pinnava quella voce col nome `DashSupersedesTheNormalMove`. Con [D-584] (1) gli slot non decidono piu': il piano e' legale,
// il percorso si compone dalla cella d'arrivo dello scatto ([D-628] (4)) e il Move avviene. `SupersededByDash` resta nell'enum
// senza produttore.
//
// I due test che seguono restano veri e restano utili: nessuna voce di scarto su un piano senza scatto, ne' su uno scatto senza
// Move.
// =====================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDashThenMoveStartsFromTheArrivalTest,
	"RefactorTactics.PlayerInteraction.DashThenMoveStartsFromTheArrival",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDashThenMoveStartsFromTheArrivalTest::RunTest(const FString&)
{
	// Due ordini dei click: lo scatto e poi il waypoint, e il waypoint e poi lo scatto ([D-628] (4): i waypoint si ricompongono). E due
	// scatti: Ram, una carica, che si ferma dove dice la sua corsa, e `Action.Dodge`, uno scatto che arriva sulla cella cliccata. Sono
	// i due rami di `ARTUnit::PlannedMoveStartCell`, e una mutazione ha mostrato che col solo Ram il secondo non lo copriva nessuno.
	for (const int32 Caso : { 0, 1, 2, 3 })
	{
		const bool bScattoPrima = (Caso % 2) == 0;
		const bool bCarica = Caso < 2;
		const FString Ordine = FString::Printf(TEXT("%s, %s"), bCarica ? TEXT("Ram") : TEXT("Dodge"),
			bScattoPrima ? TEXT("scatto, poi waypoint") : TEXT("waypoint, poi scatto"));
		UWorld* World = MakeInteractionWorld();
		if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
		if (!TestNotNull(TEXT("mappa senza ostacoli"), SpawnCleanInteractionMap(World, /*Radius=*/ 6)))
		{
			DestroyInteractionWorld(World);
			return false;
		}

		ARTUnit* U = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(1, 1));
		ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TestNotNull(TEXT("unita'"), U) || !TestNotNull(TEXT("controller"), PC)
			|| !TestNotNull(TEXT("turn manager"), TM))
		{
			DestroyInteractionWorld(World);
			return false;
		}
		PC->SelectActorForTest(U);

		const int32 DashIdx = bCarica ? U->FindDashAbilityIndex() : RTAbilityFixtures::AddCoreAbility(U, TEXT("Action.Dodge"));
		if (!TestNotEqual(TEXT("premessa: l'eroe ha una mobilita' rapida"), DashIdx, static_cast<int32>(INDEX_NONE)))
		{
			DestroyInteractionWorld(World);
			return false;
		}
		const URTActionData* Scelto = U->GetAbility(DashIdx);
		if (!TestTrue(TEXT("premessa: carica o scatto, come chiede il caso"), Scelto
				&& (Scelto->Def.MovementStyle == ERTMovementStyle::LinearCharge) == bCarica))
		{
			DestroyInteractionWorld(World);
			return false;
		}

		const FRTCellId DashTo(1, 2);
		const FRTCellId MoveTo(3, 1);
		auto Scatta = [&]() { U->SelectAbility(DashIdx); PC->HandleClickOnCell(DashTo); U->SelectAbility(INDEX_NONE); };
		auto Cammina = [&]() { PC->HandleClickOnCell(MoveTo); };
		if (bScattoPrima) { Scatta(); Cammina(); } else { Cammina(); Scatta(); }

		const TArray<FRTCellId>& Percorso = U->GetPlan().PlannedPath();
		if (!TestEqual(*FString::Printf(TEXT("%s, premessa: lo scatto e' pianificato"), *Ordine), U->GetPlan().PlannedDashCell(), DashTo)
			|| !TestTrue(*FString::Printf(TEXT("%s, premessa: il percorso c'e'"), *Ordine), Percorso.Num() >= 2))
		{
			DestroyInteractionWorld(World);
			return false;
		}
		TestEqual(*FString::Printf(TEXT("%s: il percorso parte dalla cella d'arrivo dello scatto"), *Ordine), Percorso[0], DashTo);
		TestEqual(*FString::Printf(TEXT("%s: e arriva alla cella cliccata"), *Ordine), U->GetPlan().PlannedCell, MoveTo);
		TestEqual(*FString::Printf(TEXT("%s: il waypoint resta"), *Ordine), U->GetPlan().PlannedWaypoints().Num(), 1);
		// Il verso si dichiara da dove l'unita' finira': la fine del Move, che risolve dopo lo scatto (revisione di CP 38.16a). Fino a
		// CP 38.16 lo scatto sostituiva il movimento, e il selettore si apriva sulla sua cella.
		TestEqual(*FString::Printf(TEXT("%s: il verso si dichiara dalla fine del Move"), *Ordine), FRTFacingDeclaration::CellFor(U), MoveTo);
		TestTrue(*FString::Printf(TEXT("%s: il piano e' legale"), *Ordine),
			URTPlanValidationLibrary::ValidatePlan(FRTHexSimUnit(0, U->Cell, 10, true),
				URTPlanValidationLibrary::MakePlanFor(U)).bLegal);

		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

		auto Conta = [TM, U](ERTMoveOutcome Esito)
		{
			int32 N = 0;
			for (const FRTTurnLogEntry& E : TM->GetTurnLog())
			{
				N += (E.UnitId == U->StableUnitId && E.Category == ERTLogCategory::Move && E.Outcome == static_cast<uint8>(Esito)) ? 1 : 0;
			}
			return N;
		};
		TestEqual(*FString::Printf(TEXT("%s: nessuna voce di scarto"), *Ordine), Conta(ERTMoveOutcome::SupersededByDash), 0);
		// La voce `Moved` della fase Move: anche lo scatto ne scrive una, nella fase Dash.
		int32 MossoNelMove = 0;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			MossoNelMove += (E.UnitId == U->StableUnitId && E.Phase == ERTMatchPhase::Move && E.Category == ERTLogCategory::Move
				&& E.Outcome == static_cast<uint8>(ERTMoveOutcome::Moved)) ? 1 : 0;
		}
		TestEqual(*FString::Printf(TEXT("%s: il Move e' avvenuto"), *Ordine), MossoNelMove, 1);
		TestEqual(*FString::Printf(TEXT("%s: l'unita' e' dove l'ha portata il Move, dopo lo scatto"), *Ordine), U->Cell, MoveTo);
		DestroyInteractionWorld(World);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNoSupersededEntryOnALegalPlanTest,
	"RefactorTactics.PlayerInteraction.NoSupersededEntryOnALegalPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNoSupersededEntryOnALegalPlanTest::RunTest(const FString&)
{
	// L'altra meta': un movimento normale senza scatto non lascia la voce. Senza questo, un
	// `AppendLogEntry` incondizionato dentro `ResolveDash` supererebbe il test gemello, e ogni scatto del
	// gioco — anche quello di chi non aveva pianificato nulla — dichiarerebbe uno scarto inesistente.
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	if (!TestNotNull(TEXT("mappa senza ostacoli"), SpawnCleanInteractionMap(World, /*Radius=*/ 6)))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	ARTUnit* U = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(1, 1));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("unita'"), U) || !TestNotNull(TEXT("controller"), PC)
		|| !TestNotNull(TEXT("turn manager"), TM))
	{
		DestroyInteractionWorld(World);
		return false;
	}
	PC->SelectActorForTest(U);

	// Il piano canonico di D-028: un movimento e basta. Due waypoint sono UN movimento, non due.
	U->SelectAbility(INDEX_NONE);
	PC->HandleClickOnCell(FRTCellId(1, 2));
	PC->HandleClickOnCell(FRTCellId(2, 2));
	if (!TestEqual(TEXT("premessa: due waypoint compongono un solo movimento"), U->GetPlan().PlannedWaypoints().Num(), 2))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	TM->LockInAndResolve();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

	int32 Superseded = 0;
	for (const FRTTurnLogEntry& Entry : TM->GetTurnLog())
	{
		if (Entry.Category == ERTLogCategory::Move
			&& static_cast<ERTMoveOutcome>(Entry.Outcome) == ERTMoveOutcome::SupersededByDash)
		{
			++Superseded;
		}
	}

	TestEqual(TEXT("un piano legale non dichiara nessuno scarto"), Superseded, 0);
	TestTrue(TEXT("e l'unita' si e' mossa davvero"), U->Cell != FRTCellId(1, 1));

	DestroyInteractionWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNoSupersededEntryOnADashWithoutAPlannedMoveTest,
	"RefactorTactics.PlayerInteraction.NoSupersededEntryOnADashWithoutAPlannedMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNoSupersededEntryOnADashWithoutAPlannedMoveTest::RunTest(const FString&)
{
	// Il caso che i due test gemelli NON toccano, ed e' quello che il 2026-08-26 era rotto: uno scatto e
	// basta, senza nessun movimento pianificato. Il gemello "su un piano legale" non pianifica **nessuno
	// scatto**, quindi `ResolveDash` esce a `DasherCount == 0` e il blocco che scrive la voce non viene mai
	// eseguito: prometteva di difendere questo caso e non lo attraversava.
	//
	// 🔴 **Perche' e' il test che serve**: il predicato «aveva un movimento normale» confronta `PlannedCell`
	// con `Unit->Cell`, e `Unit->Cell` viene riscritto con la cella d'arrivo PRIMA del confronto. Letto la',
	// dice vero per **ogni** scatto che ha spostato l'unita' — e ogni scatto del gioco dichiarerebbe uno
	// scarto inesistente dentro un formato serializzato, ordinato e riprodotto.
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	if (!TestNotNull(TEXT("mappa senza ostacoli"), SpawnCleanInteractionMap(World, /*Radius=*/ 6)))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	const FRTCellId Start(1, 1);
	ARTUnit* U = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), Start);
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("unita'"), U) || !TestNotNull(TEXT("controller"), PC)
		|| !TestNotNull(TEXT("turn manager"), TM))
	{
		DestroyInteractionWorld(World);
		return false;
	}
	PC->SelectActorForTest(U);

	const int32 DashIdx = U->FindDashAbilityIndex();
	if (!TestNotEqual(TEXT("premessa: l'eroe ha una mobilita' rapida"), DashIdx, static_cast<int32>(INDEX_NONE)))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	// Lo scatto e nient'altro: nessun waypoint, nessuna destinazione dichiarata.
	const FRTCellId DashTo(1, 2);
	U->SelectAbility(DashIdx);
	PC->HandleClickOnCell(DashTo);

	// Le premesse: senza, il turno non conterrebbe il caso in esame.
	if (!TestNotEqual(TEXT("premessa: lo scatto e' pianificato"), U->GetPlan().PlannedDashAbility(), static_cast<int32>(INDEX_NONE))
		|| !TestEqual(TEXT("premessa: nessun waypoint posato"), U->GetPlan().PlannedWaypoints().Num(), 0)
		|| !TestEqual(TEXT("premessa: nessun movimento dichiarato — la destinazione e' la cella attuale"),
			U->GetPlan().PlannedCell, Start))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	TM->LockInAndResolve();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

	int32 Superseded = 0;
	for (const FRTTurnLogEntry& Entry : TM->GetTurnLog())
	{
		if (Entry.Category == ERTLogCategory::Move
			&& static_cast<ERTMoveOutcome>(Entry.Outcome) == ERTMoveOutcome::SupersededByDash)
		{
			++Superseded;
		}
	}

	// E lo scatto ha spostato l'unita' davvero: e' la condizione che innesca il falso positivo, non un
	// dettaglio di contorno. Un dash che non muove nessuno non proverebbe niente.
	if (!TestEqual(TEXT("premessa: lo scatto ha spostato l'unita'"), U->Cell, DashTo))
	{
		DestroyInteractionWorld(World);
		return false;
	}
	TestEqual(TEXT("uno scatto senza movimento pianificato non dichiara nessuno scarto"), Superseded, 0);

	DestroyInteractionWorld(World);
	return true;
}

// =====================================================================================================
// Il validatore al lock-in acquista un test.
//
// `ARTTurnManager::ValidatePlansAtLockIn` e' la META' del DoD di #605 che vive al COMMIT: «un punto solo
// che risponde LEGALE / ILLEGALE prima del commit». Fino al 2026-08-26 non aveva un test: cancellare la
// chiamata a `ValidatePlansAtLockIn` in `LockInAndResolve` lasciava la suite verde.
//
// I due test vanno tenuti INSIEME: senza il secondo, un `AddLogEvent` incondizionato passerebbe il primo.
// =====================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLockInDeclaresTheContradictionTest,
	"RefactorTactics.PlayerInteraction.LockInDeclaresTheContradiction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLockInDeclaresTheContradictionTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	if (!TestNotNull(TEXT("mappa senza ostacoli"), SpawnCleanInteractionMap(World, /*Radius=*/ 6)))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	ARTUnit* U = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(1, 1));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("unita'"), U) || !TestNotNull(TEXT("controller"), PC)
		|| !TestNotNull(TEXT("turn manager"), TM))
	{
		DestroyInteractionWorld(World);
		return false;
	}
	PC->SelectActorForTest(U);

	const int32 DashIdx = U->FindDashAbilityIndex();
	if (!TestNotEqual(TEXT("premessa: l'eroe ha una mobilita' rapida"), DashIdx, static_cast<int32>(INDEX_NONE)))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	// Il piano incoerente: due scatti diversi.
	U->SelectAbility(DashIdx);
	PC->HandleClickOnCell(FRTCellId(1, 2));
	U->SelectAbility(INDEX_NONE);
	// E38 CP 38.16: scatto + Move e' legale ([D-584] (1)); l'unico piano con un detentore che resta e' un secondo scatto, diverso
	// ([D-625] (4)). Nessun gesto lo compone: lo si scrive con la strada dell'harness. ⏱️ *Fino a CP 38.16 qui c'era un waypoint.*
	const int32 SecondoScatto = RTAbilityFixtures::AddCoreAbility(U, TEXT("Action.Dodge"));
	if (!TestTrue(TEXT("premessa: il secondo scatto entra nel piano"), SecondoScatto != INDEX_NONE && U->AddPlannedDash(SecondoScatto, FRTCellId(2, 1))))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	TM->LockInAndResolve();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

	int32 Righe = 0;
	for (const FString& Evento : TM->GetRecentEvents())
	{
		if (Evento.Contains(TEXT("piano non valido al lock-in"))) { ++Righe; }
	}
	TestEqual(TEXT("il lock-in dichiara la contraddizione nel combat log"), Righe, 1);

	// E ogni scatto lascia la propria voce `Fallback`/`Cancelled`, con la PROPRIA cella (revisione di CP 38.16a): leggere
	// `PlannedDashCell()` dava a entrambe la cella del primo.
	const FName IdDelPrimo = U->GetAbility(DashIdx) ? U->GetAbility(DashIdx)->Def.ActionId : NAME_None;
	FRTCellId CellaDelPrimo(0, 0, INDEX_NONE);
	FRTCellId CellaDelSecondo(0, 0, INDEX_NONE);
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category != ERTLogCategory::Fallback || E.Amount != static_cast<int32>(ERTActionInvalidReason::SecondDash)) { continue; }
		if (E.ActionId == IdDelPrimo) { CellaDelPrimo = E.TgtCell; }
		if (E.ActionId == FName(TEXT("Action.Dodge"))) { CellaDelSecondo = E.TgtCell; }
	}
	TestEqual(TEXT("la voce del primo scatto porta la sua cella"), CellaDelPrimo, FRTCellId(1, 2));
	TestEqual(TEXT("e quella del secondo la sua"), CellaDelSecondo, FRTCellId(2, 1));

	DestroyInteractionWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLockInStaysSilentOnALegalPlanTest,
	"RefactorTactics.PlayerInteraction.LockInStaysSilentOnALegalPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLockInStaysSilentOnALegalPlanTest::RunTest(const FString&)
{
	// Il gemello: senza, un `AddLogEvent` incondizionato passerebbe il test qui sopra.
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	if (!TestNotNull(TEXT("mappa senza ostacoli"), SpawnCleanInteractionMap(World, /*Radius=*/ 6)))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	ARTUnit* U = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(1, 1));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("unita'"), U) || !TestNotNull(TEXT("controller"), PC)
		|| !TestNotNull(TEXT("turn manager"), TM))
	{
		DestroyInteractionWorld(World);
		return false;
	}
	PC->SelectActorForTest(U);

	// Il piano canonico di D-028: un movimento e basta.
	U->SelectAbility(INDEX_NONE);
	PC->HandleClickOnCell(FRTCellId(1, 2));
	PC->HandleClickOnCell(FRTCellId(2, 2));

	TM->LockInAndResolve();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

	int32 Righe = 0;
	for (const FString& Evento : TM->GetRecentEvents())
	{
		if (Evento.Contains(TEXT("piano non valido al lock-in"))) { ++Righe; }
	}
	TestEqual(TEXT("un piano legale non produce nessuna riga di rifiuto"), Righe, 0);

	DestroyInteractionWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLockInNamesBothActionsTest,
	"RefactorTactics.PlayerInteraction.LockInNamesBothActions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLockInNamesBothActionsTest::RunTest(const FString&)
{
	// 🔴 Il messaggio NON deve nominare un solo colpevole: la legalita' e' la coppia, e nominarne una sola manderebbe il giocatore a
	// correggere quella sbagliata. ⏱️ *Fino a CP 38.16 la coppia era scatto + Move, sullo slot movimento*; ora e' l'unica regola di
	// classe che resta, due scatti diversi ([D-625] (4)).
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	if (!TestNotNull(TEXT("mappa senza ostacoli"), SpawnCleanInteractionMap(World, /*Radius=*/ 6)))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	ARTUnit* U = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(1, 1));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("unita'"), U) || !TestNotNull(TEXT("controller"), PC)
		|| !TestNotNull(TEXT("turn manager"), TM))
	{
		DestroyInteractionWorld(World);
		return false;
	}
	PC->SelectActorForTest(U);

	const int32 DashIdx = U->FindDashAbilityIndex();
	if (!TestNotEqual(TEXT("premessa: l'eroe ha una mobilita' rapida"), DashIdx, static_cast<int32>(INDEX_NONE)))
	{
		DestroyInteractionWorld(World);
		return false;
	}
	U->SelectAbility(DashIdx);
	PC->HandleClickOnCell(FRTCellId(1, 2));
	U->SelectAbility(INDEX_NONE);
	// E38 CP 38.16: scatto + Move e' legale ([D-584] (1)); l'unico piano con un detentore che resta e' un secondo scatto, diverso
	// ([D-625] (4)). Nessun gesto lo compone: lo si scrive con la strada dell'harness. ⏱️ *Fino a CP 38.16 qui c'era un waypoint.*
	const int32 SecondoScatto = RTAbilityFixtures::AddCoreAbility(U, TEXT("Action.Dodge"));
	if (!TestTrue(TEXT("premessa: il secondo scatto entra nel piano"), SecondoScatto != INDEX_NONE && U->AddPlannedDash(SecondoScatto, FRTCellId(2, 1))))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	const FName DashId = U->GetAbility(DashIdx) ? U->GetAbility(DashIdx)->Def.ActionId : NAME_None;
	if (!TestFalse(TEXT("premessa: la mobilita' ha un ActionId"), DashId.IsNone()))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	TM->LockInAndResolve();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

	FString Riga;
	for (const FString& Evento : TM->GetRecentEvents())
	{
		if (Evento.Contains(TEXT("piano non valido al lock-in"))) { Riga = Evento; break; }
	}
	if (!TestFalse(TEXT("premessa: la riga di rifiuto esiste"), Riga.IsEmpty()))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	TestTrue(TEXT("nomina il secondo scatto"), Riga.Contains(TEXT("Action.Dodge")));
	TestTrue(TEXT("e nomina anche il primo"), Riga.Contains(*DashId.ToString()));
	TestTrue(TEXT("dice il motivo in italiano"), Riga.Contains(TEXT("scatto nel piano")));
	TestFalse(TEXT("e non stampa l'identificatore grezzo dell'enum"), Riga.Contains(TEXT("SecondDash")));

	DestroyInteractionWorld(World);
	return true;
}

/**
 * Il ramo di `DescribeInvalidReason` senza `HolderActionId` e' raggiungibile SOLO da `ERTActionInvalidReason::
 * OnCooldown` — `SlotOccupied` porta sempre un detentore, e `InsufficientMovementPoints` non ha produttore
 * (D-190). Fino ad ora quel ramo cadeva nel `default`, stampando «non eseguibile» invece del motivo: l'unico
 * rifiuto del lock-in raggiungibile da questo ramo restava senza reason code leggibile.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLockInDescribesTheCooldownReasonTest,
	"RefactorTactics.PlayerInteraction.LockInDescribesTheCooldownReason",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLockInDescribesTheCooldownReasonTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	if (!TestNotNull(TEXT("mappa senza ostacoli"), SpawnCleanInteractionMap(World, /*Radius=*/ 6)))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	ARTUnit* U = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(1, 1));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("unita'"), U) || !TestNotNull(TEXT("controller"), PC)
		|| !TestNotNull(TEXT("turn manager"), TM))
	{
		DestroyInteractionWorld(World);
		return false;
	}
	PC->SelectActorForTest(U);

	// KineticPanel (indice 1 nel roster di Riktor, RTHeroCatalogLibrary.cpp): Main, cooldown 2.
	constexpr int32 PanelIdx = 1;
	const URTActionData* Panel = U->GetAbility(PanelIdx);
	if (!TestTrue(TEXT("premessa: e' KineticPanel, e ha un cooldown"),
		Panel && Panel->Def.ActionId == FName(TEXT("Hero.Riktor.KineticPanel")) && Panel->Def.CooldownTurns > 0))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	// La messa in ricarica passa dall'API reale (`ConsumeAbility`), non da un campo forzato a mano.
	U->ConsumeAbility(PanelIdx);
	if (!TestTrue(TEXT("premessa: l'abilita' e' in ricarica"), U->GetAbilityCooldown(PanelIdx) > 0))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	// Pianificata comunque, scrivendo il campo: il rifiuto per ricarica del giocatore sta in
	// `ARTPlayerController::SelectAbilityForCurrent` (da `#3833` non piu' nei gestori di click come
	// `HandleTargetEdge`), non nel dato — ed e' esattamente cio' che il validatore al lock-in deve
	// intercettare quando quel rifiuto non si e' applicato.
	U->SetPlannedMainAction(PanelIdx);

	TM->LockInAndResolve();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

	FString Riga;
	for (const FString& Evento : TM->GetRecentEvents())
	{
		if (Evento.Contains(TEXT("piano non valido al lock-in"))) { Riga = Evento; break; }
	}
	if (!TestFalse(TEXT("premessa: la riga di rifiuto esiste"), Riga.IsEmpty()))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	TestTrue(TEXT("dice il motivo in italiano"), Riga.Contains(TEXT("ricarica")));
	TestFalse(TEXT("e non stampa l'identificatore grezzo dell'enum"), Riga.Contains(TEXT("OnCooldown")));

	DestroyInteractionWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSupersededEntryRendersTheDiscardedRouteTest,
	"RefactorTactics.PlayerInteraction.SupersededEntryRendersTheDiscardedRoute",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSupersededEntryRendersTheDiscardedRouteTest::RunTest(const FString&)
{
	// La voce esiste per portare la destinazione mai raggiunta. Se il rendering non la stampa, la traccia
	// dice al lettore la cella dove l'unita' E', che e' l'informazione che gia' aveva.
	FRTTurnLogEntry Entry;
	Entry.Phase = ERTMatchPhase::Dash;
	Entry.Category = ERTLogCategory::Move;
	Entry.Outcome = static_cast<uint8>(ERTMoveOutcome::SupersededByDash);
	Entry.ActionId = TEXT("Action.Move");
	Entry.SrcCell = FRTCellId(1, 1);
	Entry.TgtCell = FRTCellId(2, 1);
	Entry.Amount = 2;

	const FString Testo = URTTurnLogLibrary::DescribeEntry(Entry);

	TestTrue(TEXT("nomina la cella di partenza"), Testo.Contains(TEXT("q=1,r=1")));
	TestTrue(TEXT("e la destinazione mai raggiunta"), Testo.Contains(TEXT("q=2,r=1")));
	TestTrue(TEXT("e quante celle sono state scartate"), Testo.Contains(TEXT("2 celle")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLockInRejectionsFollowTheStableOrderTest,
	"RefactorTactics.PlayerInteraction.LockInRejectionsFollowTheStableOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLockInRejectionsFollowTheStableOrderTest::RunTest(const FString&)
{
	// 🔴 **L'ordine delle righe di rifiuto non deve dipendere dall'ordine di SPAWN.**
	// `ValidatePlansAtLockIn` itera `CollectLivingUnits`, che ordina con `SortUnitsForResolution` — cella per
	// prima (#2922). Sostituire
	// quella chiamata con un `GetAllActorsOfClass` grezzo lascerebbe verde tutto il resto della suite e
	// renderebbe il combat log dipendente dall'ordine in cui il livello tiene gli Actor — la stessa classe di
	// difetto che `Match.Autobattle.DeterminismSurvivesUnitPermutation` difende per la risoluzione, e che
	// qui non era difesa da nessuno.
	//
	// Le due unita' si spawnano di proposito nell'ordine OPPOSTO a quello canonico: `StableLess` confronta
	// prima `Layer`, poi `X`, poi `Y`, quindi `(1,0)` precede `(3,0)` mentre lo spawn fa il contrario.
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	if (!TestNotNull(TEXT("mappa senza ostacoli"), SpawnCleanInteractionMap(World, /*Radius=*/ 6)))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	const FRTCellId CellaTardi(3, 0); // spawnata per PRIMA, canonicamente SECONDA
	const FRTCellId CellaPrima(1, 0); // spawnata per SECONDA, canonicamente PRIMA
	ARTUnit* Tardi = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), CellaTardi);
	ARTUnit* Prima = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), CellaPrima);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("unita' spawnata per prima"), Tardi) || !TestNotNull(TEXT("unita' spawnata per seconda"), Prima)
		|| !TestNotNull(TEXT("turn manager"), TM))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	// Un piano illegale su ENTRAMBE: due scatti diversi ([D-625] (4)). ⏱️ *Fino a CP 38.16 era scatto piu' movimento, sullo slot.*
	for (ARTUnit* U : { Tardi, Prima })
	{
		const int32 DashIdx = U->FindDashAbilityIndex();
		if (!TestNotEqual(TEXT("premessa: l'eroe ha una mobilita' rapida"), DashIdx, static_cast<int32>(INDEX_NONE)))
		{
			DestroyInteractionWorld(World);
			return false;
		}
		U->SetPlannedDash(DashIdx, FRTCellId(U->Cell.X, U->Cell.Y + 1));
		const int32 SecondoScatto = RTAbilityFixtures::AddCoreAbility(U, TEXT("Action.Dodge"));
		if (!TestTrue(TEXT("premessa: il secondo scatto entra nel piano"),
				SecondoScatto != INDEX_NONE && U->AddPlannedDash(SecondoScatto, FRTCellId(U->Cell.X + 1, U->Cell.Y))))
		{
			DestroyInteractionWorld(World);
			return false;
		}
	}

	TM->LockInAndResolve();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

	// La posizione delle due righe di rifiuto nel combat log, nell'ordine in cui sono state emesse.
	int32 PosPrima = INDEX_NONE;
	int32 PosTardi = INDEX_NONE;
	const TArray<FString>& Eventi = TM->GetRecentEvents();
	for (int32 I = 0; I < Eventi.Num(); ++I)
	{
		if (!Eventi[I].Contains(TEXT("piano non valido al lock-in"))) { continue; }
		if (Eventi[I].Contains(Prima->GetName()) && PosPrima == INDEX_NONE) { PosPrima = I; }
		if (Eventi[I].Contains(Tardi->GetName()) && PosTardi == INDEX_NONE) { PosTardi = I; }
	}

	if (!TestNotEqual(TEXT("premessa: l'unita' canonicamente prima ha una riga"), PosPrima, static_cast<int32>(INDEX_NONE))
		|| !TestNotEqual(TEXT("premessa: l'altra unita' ha una riga"), PosTardi, static_cast<int32>(INDEX_NONE)))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	TestTrue(TEXT("le righe seguono l'ordine canonico per cella, non quello di spawn"), PosPrima < PosTardi);

	DestroyInteractionWorld(World);
	return true;
}

/**
 * **Ogni voce del kit e' raggiungibile dall'input.** L'invariante che #1409 e #1034 chiedevano, e la ragione
 * per cui i tasti abilita' sono diventati una collezione invece di quattro campi distinti.
 *
 * Il difetto che copre e' stato osservato in PIE il 2026-08-16 (seduta U18): il kit di un'unita' e' cinque
 * voci d'eroe piu' le generiche accodate (D-025), i tasti mappati erano `1`..`4`, e tutto cio' che stava
 * oltre l'indice `3` — `Overwatch`, `Guard`, `Brace`, `Wait`, `Interact` e la reazione di tre eroi su
 * quattro — il giocatore non poteva armarlo. **Il bot invece le arma da se'**, ed e' la ragione per cui il
 * difetto era difficile da vedere guardando una partita: le reazioni si vedevano comunque accadere.
 *
 * ⚠️ **Si confronta con `AbilityHotkeys().Num()`, mai con un letterale.** Un `TestEqual(..., 4)` sarebbe
 * stato verde il giorno prima e falso il giorno dopo senza che nessuno lo toccasse — e soprattutto avrebbe
 * misurato il numero che qualcuno ha scritto, non quello che l'input raggiunge davvero.
 *
 * Cade in tre modi, e sono i tre modi in cui il difetto puo' tornare: un eroe con piu' abilita', una
 * generica accodata in piu' (`GetGenericActionIds`), o un tasto tolto dalla lista.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerInputEveryKitEntryIsReachableTest,
	"RefactorTactics.PlayerInput.EveryKitEntryIsReachable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerInputEveryKitEntryIsReachableTest::RunTest(const FString&)
{
	const TArray<FKey>& Hotkeys = RTInputKeys::AbilityHotkeys();
	const int32 Generiche = URTCatalogLibrary::GetGenericActionIds().Num();
	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();

	// Anti-vacuita': con un roster vuoto o senza tasti il ciclo sotto non asserirebbe nulla e il test
	// resterebbe verde raccontando che va tutto bene.
	TestTrue(TEXT("il roster non e' vuoto"), Roster.Num() > 0);
	TestTrue(TEXT("almeno un tasto arma una posizione del kit"), Hotkeys.Num() > 0);
	TestTrue(TEXT("le generiche esistono: sono parte del kit di ogni unita' (D-025)"), Generiche > 0);

	// ⚠️ **Non si confrontano piu' due totali, e la differenza conta.** Fino a quando l'input aveva un solo
	// canale, `Hotkeys.Num() >= VociDelKit` bastava. Ora i canali sono due — i numeri per POSIZIONE, i tasti
	// generici per NOME — e quella somma sarebbe verde per il motivo sbagliato: quindici tasti contro undici
	// voci tornerebbe vero anche se un'abilita' d'eroe finisse oltre la decima posizione, dove nessun numero
	// la raggiunge. Si guarda voce per voce, ognuna col canale che la serve davvero.
	TSet<FName> GenericheConTasto;
	for (const TPair<FName, FKey>& Riga : RTInputKeys::GenericHotkeys())
	{
		GenericheConTasto.Add(Riga.Key);
	}
	TestEqual(TEXT("ogni generica del catalogo ha il suo tasto"), GenericheConTasto.Num(), Generiche);

	for (const URTHeroData* Hero : Roster)
	{
		if (!Hero) { continue; }

		// Le abilita' d'EROE stanno in testa al kit e si raggiungono per posizione: servono tanti numeri
		// quante sono. E' il vincolo che ha bloccato `Action.Shield` finche' le generiche occupavano la fila.
		TestTrue(*FString::Printf(
				TEXT("%s: %d abilita' d'eroe, %d posizioni numeriche"),
				*Hero->HeroId.ToString(), Hero->Actions.Num(), Hotkeys.Num()),
			Hotkeys.Num() >= Hero->Actions.Num());

		// Le GENERICHE sono accodate dopo, e il loro indice dipende da quante azioni porta l'eroe: si
		// raggiungono per nome, quindi cio' che va verificato e' che ognuna abbia un tasto — non che ci
		// stia dentro una posizione.
		for (const FName& Id : URTCatalogLibrary::GetGenericActionIds())
		{
			TestTrue(*FString::Printf(TEXT("%s: la generica %s ha un tasto"),
					*Hero->HeroId.ToString(), *Id.ToString()),
				GenericheConTasto.Contains(Id));
		}
	}

	// Due tasti uguali renderebbero una posizione irraggiungibile in silenzio: la seconda mappatura
	// vincerebbe e l'indice della prima non risponderebbe piu' a niente.
	TSet<FKey> Distinti;
	for (const FKey& K : Hotkeys)
	{
		bool bGiaPresente = false;
		Distinti.Add(K, &bGiaPresente);
		TestFalse(*FString::Printf(TEXT("il tasto %s compare una volta sola"), *K.ToString()), bGiaPresente);
	}

	// I tasti generici entrano nello STESSO insieme, e non in uno separato: una `G` che servisse sia una
	// posizione sia una generica sarebbe la stessa collisione silenziosa, e due controlli distinti non la
	// vedrebbero. E' il verso che copre l'aggiunta di un tasto qualsiasi da una delle due parti.
	for (const TPair<FName, FKey>& Riga : RTInputKeys::GenericHotkeys())
	{
		bool bGiaPresente = false;
		Distinti.Add(Riga.Value, &bGiaPresente);
		TestFalse(*FString::Printf(TEXT("il tasto %s di %s non collide"),
			*Riga.Value.ToString(), *Riga.Key.ToString()), bGiaPresente);
	}

	return true;
}

/**
 * La proprieta' per cui il canale generico risolve per NOME invece che per posizione.
 *
 * Le generiche sono accodate al kit, quindi il loro indice dipende da quante azioni porta l'eroe: Phase ne
 * ha sei e Gadget cinque, e la stessa `Action.Guard` sta a indici DIVERSI sui due. Un tasto legato a una
 * posizione fissa punterebbe a un'abilita' d'eroe sull'uno e alla generica giusta sull'altro.
 *
 * ⚠️ **Serve un roster con kit di lunghezza diversa, ed e' la ragione per cui questo test non esisteva
 * prima**: finche' tutti e quattro gli eroi avevano cinque azioni, la risoluzione per indice e quella per
 * nome davano lo stesso risultato e nessun test poteva distinguerle.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerInputGenericResolvesByNameTest,
	"RefactorTactics.PlayerInput.GenericHotkeyResolvesByNameNotPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerInputGenericResolvesByNameTest::RunTest(const FString&)
{
	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
	if (!TestTrue(TEXT("il roster non e' vuoto"), Roster.Num() > 0)) { return false; }

	// Anti-vacuita' che vale per questo test soltanto: se ogni eroe avesse lo stesso numero di azioni, gli
	// indici coinciderebbero e il test passerebbe senza dimostrare niente.
	TSet<int32> Lunghezze;
	for (const URTHeroData* Hero : Roster) { if (Hero) { Lunghezze.Add(Hero->Actions.Num()); } }
	if (!TestTrue(TEXT("il roster ha kit di lunghezza DIVERSA: senza, il test non discrimina"),
			Lunghezze.Num() > 1))
	{
		return false;
	}

	for (const TPair<FName, FKey>& Riga : RTInputKeys::GenericHotkeys())
	{
		TSet<int32> IndiciTrovati;
		for (const URTHeroData* Hero : Roster)
		{
			if (!Hero) { continue; }
			ARTUnit* Unit = NewObject<ARTUnit>();
			if (!TestNotNull(TEXT("unita' di prova"), Unit)) { return false; }
			Unit->ConfigureFromHeroData(Hero);

			int32 Trovato = INDEX_NONE;
			for (int32 i = 0; i < Unit->NumAbilities(); ++i)
			{
				const URTActionData* A = Unit->GetAbility(i);
				if (A && A->Def.ActionId == Riga.Key) { Trovato = i; break; }
			}
			TestTrue(*FString::Printf(TEXT("%s ha %s nel kit"),
				*Hero->HeroId.ToString(), *Riga.Key.ToString()), Trovato != INDEX_NONE);
			if (Trovato != INDEX_NONE) { IndiciTrovati.Add(Trovato); }
		}

		// Il cuore: la stessa generica NON sta allo stesso indice su tutti. Se un giorno il roster tornasse
		// uniforme questo asserto cadrebbe, ed e' corretto che cada — direbbe che il test ha smesso di
		// discriminare, non che il codice e' rotto.
		TestTrue(*FString::Printf(
				TEXT("%s sta a indici diversi a seconda dell'eroe: %d indici distinti"),
				*Riga.Key.ToString(), IndiciTrovati.Num()),
			IndiciTrovati.Num() > 1);
	}

	return true;
}

// --- #1771 · nessun tasto serve due azioni diverse ---------------------------------------------------

/**
 * 🔴 **Questo test era CITATO e non esisteva.** Il commento di `GenericHotkeys()` diceva *«e' un controllo
 * che `PlayerInput.HotkeysDoNotCollide` rifa'»*, e `grep -rn "DoNotCollide" Source/` rispondeva **una sola
 * riga**: quel commento. L'unica difesa contro due azioni sullo stesso tasto era un elenco scritto a mano
 * in un altro commento — *«mappati altrove sono A D E F Q R S T V W, Home, Escape…»* — cioe' una promessa
 * che invecchia al primo `MapKey` aggiunto, ed e' esattamente cio' che e' successo: `#1771` ne aggiunge
 * quattro.
 *
 * Interroga il `UInputMappingContext` **reale**, non una lista: aggiungere un tasto e' una riga in
 * `BuildInputMappings`, e da qui in poi il controllo la vede da se'.
 *
 * ⚠️ Uno stesso tasto su **piu' chiavi della stessa azione** e' legittimo e non e' una collisione:
 * `PanAction` sta su `W A S D` con modificatori diversi, `UndoAction` su `RMB` e `BackSpace`,
 * `CameraModifierAction` su entrambi i `Ctrl`. Cio' che si vieta e' un tasto che serva **due azioni
 * distinte**, perche' li' il giocatore non puo' sapere quale otterra'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerInputHotkeysDoNotCollideTest,
	"RefactorTactics.PlayerInput.HotkeysDoNotCollide",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerInputHotkeysDoNotCollideTest::RunTest(const FString&)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
	Ctx.SetCurrentWorld(World);

	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	bool bOk = TestNotNull(TEXT("controller"), PC);
	if (bOk)
	{
		const UInputMappingContext* Imc = PC->BuildAndGetMappingContextForTest();
		bOk = TestNotNull(TEXT("mapping context costruito"), Imc);
		if (bOk)
		{
			TMap<FKey, const UInputAction*> Owner;
			int32 Collisioni = 0;
			for (const FEnhancedActionKeyMapping& M : Imc->GetMappings())
			{
				const UInputAction* Action = M.Action;
				if (!Action) { continue; }
				if (const UInputAction** Existing = Owner.Find(M.Key))
				{
					if (*Existing != Action)
					{
						++Collisioni;
						AddError(FString::Printf(TEXT("il tasto %s serve due azioni: %s e %s"),
							*M.Key.ToString(), *(*Existing)->GetName(), *Action->GetName()));
					}
				}
				else
				{
					Owner.Add(M.Key, Action);
				}
			}

			// Anti-vacuita': con zero mapping il ciclo non asserirebbe nulla e il verde direbbe che va
			// tutto bene proprio quando l'input non esiste.
			TestTrue(TEXT("il contesto contiene mapping"), Imc->GetMappings().Num() > 10);
			TestEqual(TEXT("nessun tasto serve due azioni diverse"), Collisioni, 0);

			// I quattro tasti di #1771 sono davvero mappati: senza questa riga il test resterebbe verde
			// anche se qualcuno li togliesse, perche' «zero collisioni» e' vero anche di zero tasti nuovi.
			bool bCtrl = false, bPageUp = false, bPageDown = false;
			for (const FEnhancedActionKeyMapping& M : Imc->GetMappings())
			{
				bCtrl     |= (M.Key == EKeys::LeftControl);
				bPageUp   |= (M.Key == EKeys::PageUp);
				bPageDown |= (M.Key == EKeys::PageDown);
			}
			TestTrue(TEXT("Ctrl e' mappato (modificatore camera)"), bCtrl);
			TestTrue(TEXT("PageUp e' mappato (piano attivo)"), bPageUp);
			TestTrue(TEXT("PageDown e' mappato (piano attivo)"), bPageDown);
		}
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
	return bOk;
}

// --- #3774 · i due tasti del banco -----------------------------------------------------------------

/**
 * `,` e `.` cambiano azione nelle scene del banco: precedente e successiva dello stesso eroe — `#3774`.
 *
 * Che non collidano con niente lo dice `HotkeysDoNotCollide`, che interroga lo stesso contesto; questo test dice
 * che ci SONO, e su quale azione. «Zero collisioni» e' vero anche di zero tasti nuovi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerInputLabStepKeysTest,
	"RefactorTactics.PlayerInput.LabStepKeysAreCommaAndPeriod",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerInputLabStepKeysTest::RunTest(const FString&)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
	Ctx.SetCurrentWorld(World);

	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	bool bOk = TestNotNull(TEXT("controller"), PC);
	const UInputMappingContext* Imc = bOk ? PC->BuildAndGetMappingContextForTest() : nullptr;
	bOk = bOk && TestNotNull(TEXT("mapping context costruito"), Imc);
	if (bOk)
	{
		FString SuVirgola;
		FString SuPunto;
		for (const FEnhancedActionKeyMapping& M : Imc->GetMappings())
		{
			if (!M.Action) { continue; }
			if (M.Key == EKeys::Comma)  { SuVirgola = M.Action->GetName(); }
			if (M.Key == EKeys::Period) { SuPunto = M.Action->GetName(); }
		}
		TestEqual(TEXT("`,` passa all'azione precedente del banco"), SuVirgola, FString(TEXT("IA_LabStepPrevious")));
		TestEqual(TEXT("`.` passa all'azione successiva del banco"), SuPunto, FString(TEXT("IA_LabStepNext")));
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
	return bOk;
}

// --- #3749 · il modificatore della camera e' `Ctrl`, su entrambi i lati ------------------------------

/**
 * Il tasto del modificatore vive in un punto solo: i due `MapKey` di `BuildInputMappings`. Tutta la
 * famiglia di gesti legge poi `FRTCameraGestures::IsModifierDown`, quindi questo test e' l'unico posto in cui «quale tasto»
 * si puo' asserire.
 *
 * Tre cose, e la terza e' quella che D-514 chiede con le parole «i due `Alt` non sono piu' mappati».
 * ⚠️ Mappati, non «senza effetto»: su una tastiera italiana `AltGr` arriva come `Ctrl` sinistro piu' `Alt`
 * destro, e arma il modificatore passando dal `Ctrl`. Questo test non lo puo' vedere, e non lo afferma.
 *  1. l'azione esiste nel contesto (anti-vacuita': senza, gli altri due asserti sarebbero veri di niente);
 *  2. i suoi tasti sono ESATTAMENTE i due `Ctrl` — non «almeno il sinistro», che e' cio' che il test
 *     vicino controllava per `Alt`;
 *  3. nessuna azione del contesto, non solo questa, sta su un `Alt`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerInputCameraModifierIsCtrlTest,
	"RefactorTactics.PlayerInput.CameraModifierIsCtrlOnBothSidesAndAltIsUnmapped",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerInputCameraModifierIsCtrlTest::RunTest(const FString&)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
	Ctx.SetCurrentWorld(World);

	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	const UInputMappingContext* Imc = PC ? PC->BuildAndGetMappingContextForTest() : nullptr;
	if (TestNotNull(TEXT("mapping context costruito"), Imc))
	{
		TSet<FKey> ModifierKeys;
		for (const FEnhancedActionKeyMapping& M : Imc->GetMappings())
		{
			if (!M.Action) { continue; }
			if (M.Action->GetName() == TEXT("IA_CameraModifier"))
			{
				ModifierKeys.Add(M.Key);
			}
			TestFalse(*FString::Printf(TEXT("%s non sta su un Alt (%s)"), *M.Action->GetName(), *M.Key.ToString()),
				M.Key == EKeys::LeftAlt || M.Key == EKeys::RightAlt);
		}

		TestTrue(TEXT("il modificatore camera e' nel contesto"), ModifierKeys.Num() > 0);
		TestTrue(TEXT("il modificatore sta sul Ctrl sinistro"), ModifierKeys.Contains(EKeys::LeftControl));
		TestTrue(TEXT("il modificatore sta sul Ctrl destro"), ModifierKeys.Contains(EKeys::RightControl));
		TestEqual(TEXT("il modificatore non ha altri tasti oltre ai due Ctrl"), ModifierKeys.Num(), 2);
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
	return true;
}

// --- code review 2026-08-30 · il picking del layer, e i due modi in cui sbagliava -------------------

/**
 * 🔴 **`WorldToLayer` ARROTONDA, e un'unita' e' alta piu' di mezzo layer.**
 *
 * Con `LayerHeight = 250` ogni quota sopra `125` diventa layer 1; il cilindro di un'unita' arriva a
 * `UnitHalfHeight * 2 = 180` sopra la propria cella. Colpirne la parte alta rispondeva quindi «layer 1»
 * per un'unita' ferma sul layer 0 — e l'hover finiva sulla cella *dietro* di lei, marcata **valida**,
 * cioe' indistinguibile da un hover corretto.
 *
 * Questo test non passa dal cursore, che headless non esiste: verifica la **regola** che la riparazione
 * introduce — il layer di un'unita' si chiede all'unita', non ai suoi pixel.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerInputUnitLayerIsNotDeducedFromHitHeightTest,
	"RefactorTactics.PlayerInput.AUnitsLayerComesFromItsCellNotFromTheHitHeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerInputUnitLayerIsNotDeducedFromHitHeightTest::RunTest(const FString&)
{
	// La quota del colpo su un'unita' del layer 0, come la produce il mondo.
	const float LayerHeight = 250.f;
	const float TopOfUnit = ARTUnit::UnitHalfHeight * 2.f; // 180: la sommita' del cilindro

	// Anti-vacuita': se il cilindro fosse piu' basso di mezzo layer il difetto non esisterebbe, e il test
	// sarebbe verde senza dire niente.
	TestTrue(TEXT("il cilindro supera mezzo layer: il difetto e' possibile"),
		TopOfUnit > LayerHeight * 0.5f);

	// La deduzione geometrica sbaglia — ed e' cio' che il codice faceva.
	TestEqual(TEXT("dedurre il layer dalla quota del colpo da' 1 per un'unita' del layer 0"),
		URTHexLibrary::WorldToLayer(TopOfUnit, /*OriginZ=*/ 0.0, LayerHeight), 1);

	// La cella dell'unita' invece lo sa: e' un dato di gameplay.
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
	Ctx.SetCurrentWorld(World);

	ARTUnit* Unit = World->SpawnActor<ARTUnit>();
	bool bOk = TestNotNull(TEXT("unita'"), Unit);
	if (bOk)
	{
		Unit->Cell = FRTCellId(2, -1, 0);
		TestEqual(TEXT("la cella dell'unita' dichiara il layer 0"), Unit->Cell.Layer, 0);
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
	return bOk;
}

/**
 * 🔴 **Il ripiego sul piano non verificava che il raggio andasse VERSO il piano.**
 *
 * `FMath::RayPlaneIntersection` e' `Origin + Dir * t` senza guardia: col cursore **sopra l'orizzonte** —
 * raggiungibile a ogni pitch piu' radente della meta' del FOV verticale, e `MaxPitch` e' `0` — `t` diventa
 * negativo e la cella risolta sta *dietro* la camera. Non era raggiungibile prima di `D-255`, perche' il
 * vecchio percorso richiedeva un colpo del raycast.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerInputRayAwayFromPlaneDoesNotResolveBehindTest,
	"RefactorTactics.PlayerInput.ARayPointingAwayFromThePlaneDoesNotResolveBehindTheCamera",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerInputRayAwayFromPlaneDoesNotResolveBehindTest::RunTest(const FString&)
{
	const FVector Origin = FVector::ZeroVector;
	const float HexSize = 150.f;
	const float LayerHeight = 250.f;

	// Camera in alto che guarda **verso l'alto**: il piano e' dietro di lei.
	const FVector RayOrigin(0.f, 0.f, 1000.f);
	const FVector Up(1.f, 0.f, 1.f);

	const FRTCellId Cell = URTHexLibrary::ResolveRayToCellOnLayer(RayOrigin, Up.GetSafeNormal(),
		Origin, HexSize, LayerHeight, /*ActiveLayer=*/ 0, /*bHasValidHit=*/ false, FVector::ZeroVector);

	// Il ripiego del ripiego e' la cella **sotto la camera**, che a `RayOrigin` X=Y=0 e' l'origine.
	// Senza la guardia, `t` negativo avrebbe prodotto una cella a centinaia di metri nella direzione
	// opposta — e la formula la avrebbe restituita senza segnalare nulla.
	TestEqual(TEXT("col raggio che punta lontano dal piano si ripiega sotto la camera (q)"), Cell.X, 0);
	TestEqual(TEXT("...e (r)"), Cell.Y, 0);
	TestEqual(TEXT("...sul layer attivo"), Cell.Layer, 0);

	// Controprova: col raggio che punta **verso** il piano la funzione lavora come sempre, e la cella non
	// e' quella sotto la camera — o il test sopra passerebbe anche con la guardia sempre attiva.
	const FVector Down(1.f, 0.f, -1.f);
	const FRTCellId Ahead = URTHexLibrary::ResolveRayToCellOnLayer(RayOrigin, Down.GetSafeNormal(),
		Origin, HexSize, LayerHeight, /*ActiveLayer=*/ 0, /*bHasValidHit=*/ false, FVector::ZeroVector);
	TestNotEqual(TEXT("verso il piano la cella e' avanti, non sotto la camera"), Ahead.X, 0);

	return true;
}

/**
 * 🔴 **Un rifiuto per cella occupata nomina CHI occupa, non chi pianifica** (`#1939`).
 *
 * Il messaggio riceveva `SelectedUnit`, quindi prometteva un occupante e consegnava il pianificante:
 *
 * ```text
 * [RT] Waypoint (3,0,L0) rifiutato: cella occupata da un'altra unita' (RTUnit_0)   <- RTUnit_0 e' CHI CLICCA
 * ```
 *
 * ⚠️ **Tre righe su quattro erano giuste**, ed e' per questo che il difetto e' rimasto: `NotOnMap`,
 * `BlocksMovement` e il ramo del budget nominano *di chi e' il piano* e non promettono nessun secondo
 * attore. Il test li pinna tutti e quattro, cosi' correggere il quarto non puo' spostare gli altri tre.
 *
 * ⚠️ L'id in `Occupancy` e' l'**indice** nell'array delle unita' vive, non uno `StableUnitId`: qui gli
 * indici dello snapshot e quelli di `Units` combaciano per costruzione, che e' esattamente il contratto che
 * `RTPlanningSnapshot::For` garantisce in partita.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWaypointRejectionNamesOccupantTest,
	"RefactorTactics.PlayerInput.WaypointRejectionNamesTheOccupant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWaypointRejectionNamesOccupantTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* MapActor = SpawnCleanInteractionMap(World, /*Radius=*/ 3);
	if (!TestNotNull(TEXT("mappa pulita"), MapActor))
	{
		DestroyInteractionWorld(World); return false;
	}

	const FRTCellId Mia(0, 0, 0);
	const FRTCellId Loro(2, 0, 0);
	ARTUnit* Chi  = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), Mia);
	ARTUnit* Alt  = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), Loro);
	if (!TestNotNull(TEXT("chi pianifica"), Chi) || !TestNotNull(TEXT("chi occupa"), Alt))
	{
		DestroyInteractionWorld(World); return false;
	}

	// Gli id dello snapshot sono gli INDICI di `Units`: 0 = chi pianifica, 1 = chi occupa.
	const TArray<ARTUnit*> Units = { Chi, Alt };
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshotOmniscient(MapActor->MapAsset, {
		FRTHexSimUnit(0, Mia,  /*MoveBudget=*/ 4),
		FRTHexSimUnit(1, Loro, /*MoveBudget=*/ 4)
	});

	// --- 1. 🔴 Il ramo che era sbagliato -------------------------------------------------------------
	{
		const FString Riga = ARTPlayerController::DescribeWaypointRejection(
			Snap, Units, /*PlannerIndex=*/ 0, Loro, /*SpentCost=*/ 0);

		TestTrue(*FString::Printf(TEXT("nomina CHI OCCUPA: %s"), *Riga), Riga.Contains(Alt->GetName()));
		// La riga resta utile anche su chi ha pianificato: sono due identita' diverse, e servono entrambe.
		TestTrue(*FString::Printf(TEXT("e dice di chi era il piano: %s"), *Riga), Riga.Contains(Chi->GetName()));
		// L'ordine e' cio' che rende la frase vera: l'occupante viene DOPO «occupata da».
		TestTrue(*FString::Printf(TEXT("l'occupante segue «occupata da»: %s"), *Riga),
			Riga.Contains(FString(TEXT("occupata da ")) + Alt->GetName()));
	}

	// --- 2. La propria cella non e' «occupata»: e' un waypoint legittimo, quindi il motivo e' il budget --
	{
		const FString Riga = ARTPlayerController::DescribeWaypointRejection(
			Snap, Units, /*PlannerIndex=*/ 0, Mia, /*SpentCost=*/ 3);
		TestTrue(*FString::Printf(TEXT("la propria cella non e' un'occupazione: %s"), *Riga),
			Riga.Contains(TEXT("oltre il budget")));
		TestTrue(*FString::Printf(TEXT("e il budget si legge: %s"), *Riga), Riga.Contains(TEXT("3 di 4")));
	}

	// --- 2bis. 🔴 Il tetto nominato viene dallo SNAPSHOT, non dalla portata nuda dell'unita' ---------
	// Il difetto che questo caso chiude (`#1410`): il chiamante di produzione passava
	// `GetEffectiveMoveRange()` mentre a rifiutare era il budget del PROFILO, che per [D-412] ne e' una
	// percentuale. Sotto `Sprint` la riga prometteva un tetto di `5` dopo averne dichiarati spesi `10`.
	//
	// ⚠️ **Il budget qui e' `9`, diverso da quello degli altri casi**: e' cio' che distingue «lo legge dallo
	// snapshot» da «porta un numero che per caso coincide». Con un solo valore in tutto il test, una
	// costante cablata sarebbe indistinguibile dalla lettura giusta.
	{
		const FRTHexSnapshot Largo = URTHexSimLibrary::MakeSnapshotOmniscient(MapActor->MapAsset, {
			FRTHexSimUnit(0, Mia,  /*MoveBudget=*/ 9),
			FRTHexSimUnit(1, Loro, /*MoveBudget=*/ 9)
		});
		const FString Riga = ARTPlayerController::DescribeWaypointRejection(
			Largo, Units, /*PlannerIndex=*/ 0, Mia, /*SpentCost=*/ 3);
		TestTrue(*FString::Printf(TEXT("il tetto segue lo snapshot: %s"), *Riga), Riga.Contains(TEXT("3 di 9")));
		TestFalse(*FString::Printf(TEXT("e non e' quello dell'altro snapshot: %s"), *Riga),
			Riga.Contains(TEXT("3 di 4")));
	}

	// --- 3. Fuori mappa: soggetto = chi pianifica, ed e' corretto -------------------------------------
	{
		const FString Riga = ARTPlayerController::DescribeWaypointRejection(
			Snap, Units, /*PlannerIndex=*/ 0, FRTCellId(99, 99, 0), 0);
		TestTrue(*FString::Printf(TEXT("dice fuori dalla mappa: %s"), *Riga),
			Riga.Contains(TEXT("fuori dalla mappa")));
		TestTrue(*FString::Printf(TEXT("e nomina chi pianifica: %s"), *Riga), Riga.Contains(Chi->GetName()));
	}

	// --- 4. Un indice fuori range non inventa un nome -------------------------------------------------
	{
		const TArray<ARTUnit*> Monca = { Chi }; // l'occupante non ha un Actor a cui corrispondere
		const FString Riga = ARTPlayerController::DescribeWaypointRejection(
			Snap, Monca, /*PlannerIndex=*/ 0, Loro, 0);
		TestTrue(*FString::Printf(TEXT("resta un rifiuto per occupazione: %s"), *Riga),
			Riga.Contains(TEXT("occupata da")));
		TestFalse(*FString::Printf(TEXT("e NON attribuisce l'occupazione a chi pianifica: %s"), *Riga),
			Riga.Contains(FString(TEXT("occupata da ")) + Chi->GetName()));
	}

	DestroyInteractionWorld(World);
	return true;
}


/**
 * IL TURNO 1 ACCETTA LA SELEZIONE ESATTAMENTE COME IL TURNO 2 — e il turno 2 e' il controllo, non un extra.
 *
 * L'indagine di `#1957` per la strada che non richiede l'editor. Il referto di playtest dice *«dopo `Play`
 * il primo turno non accetta input: il giocatore preme Spazio, e solo dal turno 2 riesce a selezionare
 * un'unita'»*, e il turno che si chiudeva da solo si e' gia' rivelato un lock-in muto
 * (`HexMatch.FirstTurnDoesNotCloseItself`). Resta questo, che e' il disagio vero e la cui causa e' IGNOTA.
 *
 * 🔑 **Il turno 2 nello stesso test, sullo stesso montaggio.** Senza, un turno 1 che seleziona non
 * distinguerebbe *«il difetto non c'e'»* da *«la fixture non permetteva di selezionare comunque»* — che e'
 * il modo piu' comune di scrivere un verde che non misura niente. Cio' che si asserisce e' la **differenza**
 * fra i due turni, e la differenza attesa e' nessuna.
 *
 * ⚠️ **Cosa questo test NON puo' vedere, ed e' meta' del suo valore.** `OnSelect` comincia con un
 * `GetHitResultUnderCursor`, cioe' un raycast dal viewport che headless non esiste: da qui si esercita la
 * strada che comincia DOPO, con l'attore gia' risolto. Se il test passa, il difetto non e' nella selezione
 * ne' nelle guardie che la precedono, e cio' che resta da guardare in PIE si restringe a raycast, viewport
 * e possessione del pawn — che e' un'informazione, non un fallimento.
 *
 * ⛔ E non usa `SelectActorForTest`, che scrive `SelectedActor` direttamente: quella scorciatoia salterebbe
 * proprio il codice in esame. Qui si chiama `SelectUnit`, che e' cio' che `OnSelect` invoca dopo il raycast.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFirstTurnAcceptsSelectionTest,
	"RefactorTactics.PlayerInput.FirstTurnAcceptsSelectionLikeTheSecond",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFirstTurnAcceptsSelectionTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	if (MapActor) { MapActor->MapAsset = Arena; }

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTUnit* Mia = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(2, -2, 0));
	// ⚠️ **Un avversario serve, e la prima stesura non ce l'aveva.** Con una sola squadra in campo
	// `EvaluateOutcome` dichiara vinta la partita alla fine del turno 1: `Phase` diventa `MatchEnded`,
	// `TurnNumber` resta `1` e il turno 2 — che qui e' il CONTROLLO — non arriva mai. L'ha intercettato
	// la guardia sull'avanzamento, con «era 1, ora 1».
	//
	// Lontano e senza piano: deve esistere, non partecipare.
	ARTUnit* Avversario = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-4, 2, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!TestNotNull(TEXT("turn manager"), TM) || !TestNotNull(TEXT("unita'"), Mia)
		|| !TestNotNull(TEXT("avversario"), Avversario)
		|| !TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("mappa"), MapActor))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	// E' `BeginPlay` ad aprire il turno 1. Senza, `TurnNumber` varrebbe 1 per default e il test misurerebbe
	// un turno che nessuno ha mai iniziato — cioe' non il turno 1 del referto.
	TM->DispatchBeginPlay();

	// Cio' che si osserva a ogni turno: le due guardie che `OnSelect` attraversa PRIMA del raycast, e l'esito
	// della selezione. Si raccoglie in una lambda perche' i due turni devono essere misurati allo stesso
	// modo: due blocchi copiati divergerebbero al primo ritocco, ed e' la differenza l'oggetto del test.
	struct FEsito
	{
		bool bInputBloccato = false;
		bool bInputInerte = false;
		bool bSelezionata = false;
		int32 Waypoint = 0;
	};
	auto ProvaSelezione = [PC, Mia](const FRTCellId& Destinazione)
	{
		FEsito E;
		E.bInputBloccato = PC->IsGameplayInputBlocked();
		E.bInputInerte = PC->IsPlanningInputInert();

		PC->SelectUnit(Mia); // la strada vera, quella che `OnSelect` prende dopo il raycast
		E.bSelezionata = (PC->GetSelectedUnit() == Mia);

		FRTUnitPlanTestAccess::ResetWaypoints(*Mia);
		PC->HandleClickOnCell(Destinazione);
		E.Waypoint = Mia->GetPlan().PlannedWaypoints().Num();
		return E;
	};

	// ── Turno 1
	if (!TestEqual(TEXT("si parte dal turno 1"), TM->GetTurnNumber(), 1))
	{
		DestroyInteractionWorld(World);
		return false;
	}
	const FEsito Primo = ProvaSelezione(FRTCellId(3, -2, 0));

	// ── Si chiude il turno e si arriva al secondo, che e' il CONTROLLO.
	//
	// ⚠️ **Il piano si azzera per intero, `PlannedCell` COMPRESA.** Resettare i soli waypoint non
	// basta: il resolver muove verso `PlannedCell`, quindi l'unita' arrivava sulla cella che il turno 2
	// avrebbe poi cliccato — e un click sulla cella in cui gia' ti trovi non produce nessun waypoint.
	// La premessa del controllo cadeva, e il test sembrava dire qualcosa sulla selezione mentre
	// misurava una geometria che si era spostata sotto.
	//
	// Ferma fra i due turni, la stessa destinazione resta valida per entrambi: e' cio' che rende i due
	// esiti confrontabili.
	FRTUnitPlanTestAccess::ResetWaypoints(*Mia);
	FRTUnitPlanTestAccess::ResetPath(*Mia);
	Mia->SetPlannedCell(Mia->Cell);
	TM->LockInAndResolve();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
	{
		TM->Tick(0.05f);
	}
	const int32 TurnoDopo = TM->GetTurnNumber();

	const FEsito Secondo = ProvaSelezione(FRTCellId(3, -2, 0));
	const FRTCellId CellaFinale = Mia->Cell;
	DestroyInteractionWorld(World);

	// ── Il controllo vale solo se il turno e' davvero avanzato: altrimenti si starebbero confrontando due
	// misure dello stesso turno, e l'uguaglianza sarebbe vera per costruzione.
	if (!TestTrue(*FString::Printf(TEXT("il turno e' avanzato (era 1, ora %d)"), TurnoDopo), TurnoDopo >= 2))
	{
		return false;
	}

	// ── ANTI-VACUITA': al turno 2 la selezione deve funzionare. Se non funzionasse nemmeno li', il confronto
	// sotto sarebbe soddisfatto da due fallimenti uguali.
	if (!TestTrue(TEXT("premessa: al turno 2 la selezione funziona"), Secondo.bSelezionata))
	{
		return false;
	}
	if (!TestTrue(*FString::Printf(TEXT("premessa: al turno 2 il click produce un waypoint (cella %d,%d)"),
		CellaFinale.X, CellaFinale.Y), Secondo.Waypoint > 0))
	{
		return false;
	}

	// ── Il cuore: i due turni si comportano allo stesso modo.
	TestEqual(TEXT("l'input non e' bloccato al turno 1 piu' che al 2"),
		Primo.bInputBloccato, Secondo.bInputBloccato);
	TestEqual(TEXT("l'input non e' inerte al turno 1 piu' che al 2"),
		Primo.bInputInerte, Secondo.bInputInerte);
	TestEqual(TEXT("il turno 1 accetta la selezione come il turno 2"),
		Primo.bSelezionata, Secondo.bSelezionata);
	TestEqual(TEXT("e il click su cella produce lo stesso numero di waypoint"),
		Primo.Waypoint, Secondo.Waypoint);

	return true;
}

// =====================================================================================================
// #79 / CP 11.3 — UN MOVIMENTO NEGATO IN PIANIFICAZIONE LASCIA LA SUA TRACCIA.
//
// Il DoD della issue chiede che «i fallback applicati siano espliciti ("percorso bloccato -> fermo")».
// Oggi non lo sono: chi dichiara una destinazione occupata e chi non dichiara niente producono la
// STESSA riga — «resta (q,r,L) (Action.Move, p50)» — e la sola cosa che li separa e' un `UE_LOG` che
// emette `RTScenarioSession`, cioe' il runner degli scenari. In partita normale quella riga non esiste.
//
// 🔑 **Perche' questi tre test passano dal CONTROLLER e non dai campi `Planned*`.**
// `Actions.Move.PathBlocked` produce gia' `BlockedByUnit`, ma per un'altra strada: li' il percorso e'
// scritto a mano e ATTRAVERSA il blocker, quindi il resolver lo vede e lo tronca. Qui il percorso non
// arriva mai al resolver — `BuildCompositeHexPath` lo rifiuta in pianificazione e
// `PlannedWaypoints.Pop()` non lascia nulla dietro di se' (`RTPlayerController.cpp:1624-1629`).
// Scrivere `PlannedPath` a mano misurerebbe il meccanismo che gia' funziona e lascerebbe il difetto
// della #79 intatto sotto un verde.
//
// Oracoli dal contratto della issue #79, non dal codice (il documento di wave che li
// portava e' stato rimosso col control plane; la regola no). In particolare:
// la voce di blocco **sostituisce** `Stayed`, non si aggiunge — due voci `Move` per la stessa unita'
// nella stessa fase sono una violazione, non un dettaglio.
// =====================================================================================================

namespace
{
	/** Porta a termine risoluzione e playback. Stesso ciclo dei test di scatto qui sopra. */
	void RisolviIlTurno(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/**
	 * Le voci `Move` che il TurnLog porta PER QUELL'UNITA', nell'ordine in cui ci stanno.
	 *
	 * Si restituisce l'elenco e non la prima voce perche' il contratto asserisce anche la CARDINALITA':
	 * «una sola voce finale». Una funzione che tornasse `TOptional` renderebbe indistinguibile
	 * «una voce» da «due voci di cui la prima e' giusta», che e' esattamente il modo in cui
	 * l'alternativita' fra `Stayed` e `BlockedByUnit` si romperebbe senza che niente diventi rosso.
	 *
	 * Il filtro e' `UnitId`: `BuildMoveLog` non lo popola, ma `ARTTurnManager::AppendLogEntry` lo scrive
	 * subito dopo con lo `StableUnitId` dell'attore. Filtrare per `SrcCell` collide appena due unita'
	 * partono dalla stessa cella.
	 */
	TArray<FRTTurnLogEntry> VociMoveDi(const ARTTurnManager* TM, const ARTUnit* Unit)
	{
		TArray<FRTTurnLogEntry> Out;
		if (!TM || !Unit) { return Out; }
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Move && E.UnitId == Unit->StableUnitId)
			{
				Out.Add(E);
			}
		}
		return Out;
	}

	/** Quante voci `Move` con quell'esito, su TUTTO il log: serve ai due gemelli negativi. */
	int32 ContaEsitoMove(const ARTTurnManager* TM, ERTMoveOutcome Esito)
	{
		int32 N = 0;
		if (!TM) { return N; }
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Move && static_cast<ERTMoveOutcome>(E.Outcome) == Esito)
			{
				++N;
			}
		}
		return N;
	}
}

/**
 * CASO B — la destinazione dichiarata e' occupata da un'unita' ferma, e il TurnLog lo dice.
 *
 * L'asserzione guarda i CAMPI, non il conteggio: `TgtCell` e' la meta' del referto che rende la voce
 * utile a chi legge il combat log, e un test che contasse solo le voci resterebbe verde su una
 * `TgtCell` uguale alla cella di partenza — cioe' su una voce che dice «sono fermo dove sono»
 * invece di «volevo andare LI' e me l'hanno negato».
 *
 * ⚠️ **Anti-vacuita', dichiarata.** `Stayed` e `BlockedByUnit` descrivono entrambi un'unita' che non
 * si e' mossa: la posizione finale non distingue il fix dal difetto. Cio' che discrimina e' l'esito
 * sulla voce, e le mutazioni che devono renderlo rosso sono elencate nel contributo DEV-TEST.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDeniedMoveDeclaresTheDenialTest,
	"RefactorTactics.PlayerInteraction.DeniedMoveDeclaresTheDenial",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDeniedMoveDeclaresTheDenialTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	if (!TestNotNull(TEXT("mappa senza ostacoli"), SpawnCleanInteractionMap(World, /*Radius=*/ 6)))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	// La mappa e' PULITA di proposito: su `MakeTestArena` un muro farebbe rifiutare il waypoint per
	// «cella bloccata», e il test resterebbe verde togliendo proprio la regola che dice di difendere
	// (stessa trappola gia' misurata il 2026-08-26 e annotata su `SpawnCleanInteractionMap`).
	const FRTCellId Partenza(0, 0);
	const FRTCellId Occupata(1, 0); // adiacente: il rifiuto e' l'occupazione, non il budget
	ARTUnit* Chi = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), Partenza);
	ARTUnit* Occupante = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), Occupata);
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	// ⚠️ **La conoscenza va CALCOLATA, o il pianificatore e' cieco** ([D-371]). In partita lo fa
	// `ARTGameMode::SetupHexMatch`; un mondo di prova che spawna il `TurnManager` a mano no, e da quando
	// l'anteprima filtra l'occupazione per osservatore un mondo senza conoscenza non vede **nessun**
	// avversario — quindi nessun diniego, che e' proprio cio' che questo banco misura.
	TM->RefreshTeamKnowledgeNow();
	if (!TestNotNull(TEXT("chi pianifica"), Chi) || !TestNotNull(TEXT("chi occupa"), Occupante)
		|| !TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("turn manager"), TM))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	PC->SelectActorForTest(Chi);
	Chi->SelectAbility(INDEX_NONE); // movimento normale, non scatto: il ramo in esame e' quello dei waypoint
	Occupante->SetPlannedCell(Occupante->Cell); // FERMA: e' l'occupazione, non una contesa

	PC->HandleClickOnCell(Occupata);

	// 🔴 **IL DINIEGO SI E' SPOSTATO DALLA PIANIFICAZIONE ALLA RISOLUZIONE** ([D-446]), e questa
	// premessa e' dove si vede. Diceva *«il waypoint e' stato RIFIUTATO in pianificazione»* e misurava
	// `PlannedWaypoints.Num() == 0`: oggi il clic si accetta, perche' rifiutarlo avrebbe richiesto di
	// sapere se l'occupante se ne andra' — cioe' di leggere il suo piano.
	//
	// 🔑 **Tutte le asserzioni a valle sono rimaste identiche, ed e' il punto.** Il diniego continua a
	// esistere, a nominare la destinazione richiesta e a sostituire `Stayed`: cambia **quando** si produce,
	// non **se**. La distinzione che `#79` esiste per tenere — *«ho provato e me l'hanno negato»* contro
	// *«non ho provato»* — e' esattamente quella che il banco continua a misurare.
	//
	// ⚠️ **E il ramo che la produce ora era gia' scritto, per il bot.** `RTTurnManager_Movement.cpp`
	// teneva due vie asimmetriche — giocatore e harness portavano uno stato dal momento del rifiuto, il bot
	// non aveva un rifiuto e la sua destinazione si leggeva da `PlannedCell`. Da oggi i tre produttori
	// passano tutti dalla seconda.
	if (!TestEqual(TEXT("premessa: il waypoint e' stato ACCETTATO in pianificazione"),
			Chi->GetPlan().PlannedWaypoints().Num(), 1)
		|| !TestEqual(TEXT("premessa: e la destinazione dichiarata e' proprio quella occupata"),
			Chi->GetPlan().PlannedCell, Occupata))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	RisolviIlTurno(TM);

	const TArray<FRTTurnLogEntry> Voci = VociMoveDi(TM, Chi);
	if (!TestEqual(TEXT("UNA sola voce Move per l'unita' negata: la voce di blocco SOSTITUISCE Stayed"),
			Voci.Num(), 1))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	const FRTTurnLogEntry& Voce = Voci[0];
	TestEqual(TEXT("l'esito dichiara il diniego, non una immobilita' volontaria"),
		Voce.Outcome, static_cast<uint8>(ERTMoveOutcome::BlockedByUnit));
	TestEqual(TEXT("la voce sta nella fase in cui il movimento si risolve"), Voce.Phase, ERTMatchPhase::Move);
	TestEqual(TEXT("SrcCell e' la cella di partenza — chiave stabile dell'unita' nel turno"),
		Voce.SrcCell, Partenza);
	// 🔴 Il campo per cui questa wave esiste. Con `TgtCell == SrcCell` la voce direbbe «fermo dove
	// sono», che e' il difetto scritto in una forma nuova invece che corretto.
	TestEqual(TEXT("TgtCell e' la destinazione RICHIESTA e negata, non la cella di partenza"),
		Voce.TgtCell, Occupata);
	TestEqual(TEXT("nomina il movimento"), Voce.ActionId, FName(TEXT("Action.Move")));
	// `Priority` per voce e' in `main` dalla #419 (formato v7) ed e' funzione del CATALOGO: un letterale
	// in un sito nuovo sarebbe una regressione su una decisione gia' presa, e passerebbe inosservato
	// perche' entra nell'ordine canonico e nell'hash senza che nessuno lo guardi.
	TestEqual(TEXT("Priority viene dal catalogo, non da uno zero implicito"),
		Voce.Priority, URTCatalogLibrary::FindCoreAction(TEXT("Action.Move")).Priority);
	// `Amount` e' un VALORE, non un'assenza: zero celle percorse. `SupersededByDash` ha gia' insegnato
	// che questo campo e' stato causa di un'asserzione sbagliata quando nessuno lo guardava.
	TestEqual(TEXT("Amount conta le celle percorse, e sono zero"), Voce.Amount, 0);

	// La voce descrive il turno invece di contraddirlo: l'unita' non si e' spostata davvero.
	TestEqual(TEXT("l'unita' e' rimasta sulla cella di partenza"), Chi->Cell, Partenza);
	TestEqual(TEXT("e l'occupante non si e' mosso: era lui l'ostacolo"), Occupante->Cell, Occupata);

	DestroyInteractionWorld(World);
	return true;
}

/**
 * CASO A — chi non ha dichiarato niente continua a dire `Stayed`, ed e' il GEMELLO del caso B.
 *
 * Senza questo test, un ramo che scrivesse `BlockedByUnit` incondizionatamente supererebbe il caso B
 * e ogni unita' immobile del gioco dichiarerebbe un diniego che nessuno le ha opposto. E' la stessa
 * disciplina di `NoSupersededEntryOnALegalPlan` e di `LockInStaysSilentOnALegalPlan`.
 *
 * La fixture e' IDENTICA a quella del caso B — stessa mappa, stesse due unita', stesse posizioni — e
 * cambia una cosa sola: il click che non arriva. E' cio' che rende i due test una differenza misurata
 * invece di due montaggi che nessuno confronta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUndeclaredMoveDoesNotDeclareADenialTest,
	"RefactorTactics.PlayerInteraction.UndeclaredMoveDoesNotDeclareADenial",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUndeclaredMoveDoesNotDeclareADenialTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	if (!TestNotNull(TEXT("mappa senza ostacoli"), SpawnCleanInteractionMap(World, /*Radius=*/ 6)))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	const FRTCellId Partenza(0, 0);
	const FRTCellId Occupata(1, 0);
	ARTUnit* Chi = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), Partenza);
	ARTUnit* Occupante = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), Occupata);
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("chi non dichiara"), Chi) || !TestNotNull(TEXT("il vicino"), Occupante)
		|| !TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("turn manager"), TM))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	// Selezionata, come nel caso B: cio' che manca e' la DICHIARAZIONE, non la selezione. Se il test
	// non selezionasse l'unita', proverebbe che un controller inerte non scrive nulla — vero e inutile.
	PC->SelectActorForTest(Chi);
	Chi->SelectAbility(INDEX_NONE);
	Occupante->SetPlannedCell(Occupante->Cell);

	// Nessun `HandleClickOnCell`: e' l'unica differenza rispetto al caso B.
	if (!TestEqual(TEXT("premessa: nessun waypoint dichiarato"), Chi->GetPlan().PlannedWaypoints().Num(), 0))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	RisolviIlTurno(TM);

	const TArray<FRTTurnLogEntry> Voci = VociMoveDi(TM, Chi);
	if (!TestEqual(TEXT("una sola voce Move per chi non ha dichiarato"), Voci.Num(), 1))
	{
		DestroyInteractionWorld(World);
		return false;
	}
	TestEqual(TEXT("chi non ha dichiarato niente dice Stayed, come prima della #79"),
		Voci[0].Outcome, static_cast<uint8>(ERTMoveOutcome::Stayed));

	// 🔴 Il gemello vero e proprio: su TUTTO il log, nessun diniego. Un `BlockedByUnit` incondizionato
	// morirebbe qui, ed e' il solo posto in cui puo' morire.
	TestEqual(TEXT("nessun diniego inventato per chi non ha tentato niente"),
		ContaEsitoMove(TM, ERTMoveOutcome::BlockedByUnit), 0);

	TestEqual(TEXT("l'unita' e' rimasta dov'era"), Chi->Cell, Partenza);

	DestroyInteractionWorld(World);
	return true;
}

/**
 * CASO C — il giocatore sbaglia, corregge, e il TurnLog racconta il PIANO FINALE.
 *
 * ⛔ E' il test che impedisce la falsa soluzione. Il modo piu' semplice di far passare il caso B e'
 * registrare un evento a ogni click rifiutato: funzionerebbe, e trasformerebbe ogni tentativo
 * esplorativo in un fatto del replay — il TurnLog e' un formato serializzato, ordinato e riprodotto,
 * non un diario dell'interazione. Il soggetto della voce e' il piano che il giocatore ha COMMITTATO.
 *
 * `WORK-ORDER.md` § *Falsi punti di partenza* punto 4, e § *EXPECTED BEHAVIOR* riga `Timing boundary`:
 * lo stato di rifiuto nasce in `Planning` e **muore quando un piano valido lo sostituisce**.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWaypointOnAnOccupantDoesNotDenyTest,
	"RefactorTactics.PlayerInteraction.AWaypointOnAnOccupantDoesNotDenyTheFinalPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWaypointOnAnOccupantDoesNotDenyTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	if (!TestNotNull(TEXT("mappa senza ostacoli"), SpawnCleanInteractionMap(World, /*Radius=*/ 6)))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	const FRTCellId Partenza(0, 0);
	const FRTCellId Occupata(1, 0);  // il tentativo che viene negato
	const FRTCellId Ripiego(0, 1);   // adiacente e libera: la correzione del giocatore
	ARTUnit* Chi = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), Partenza);
	ARTUnit* Occupante = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), Occupata);
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	// ⚠️ **La conoscenza va CALCOLATA, o il pianificatore e' cieco** ([D-371]). In partita lo fa
	// `ARTGameMode::SetupHexMatch`; un mondo di prova che spawna il `TurnManager` a mano no, e da quando
	// l'anteprima filtra l'occupazione per osservatore un mondo senza conoscenza non vede **nessun**
	// avversario — quindi nessun diniego, che e' proprio cio' che questo banco misura.
	TM->RefreshTeamKnowledgeNow();
	if (!TestNotNull(TEXT("chi pianifica"), Chi) || !TestNotNull(TEXT("chi occupa"), Occupante)
		|| !TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("turn manager"), TM))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	PC->SelectActorForTest(Chi);
	Chi->SelectAbility(INDEX_NONE);
	Occupante->SetPlannedCell(Occupante->Cell);

	// 🔴 **LA SEQUENZA REALE DEL GIOCATORE E' CAMBIATA** ([D-446]). Era: *prova il varco occupato, se
	// lo vede negare, e ripiega* — due clic di cui il primo veniva **scartato**, e il banco misurava che il
	// rifiuto non lasciasse residui. Oggi il primo clic si accetta, quindi i due si **sommano**: un waypoint
	// sopra l'occupante, e la destinazione finale sul ripiego.
	//
	// 🔑 **Cio' che il banco misura resta lo stesso fatto**: dichiarare qualcosa sopra un'unita' non
	// nega il piano. Prima perche' il rifiuto non sopravviveva al piano che lo sostituiva; ora perche' il
	// rifiuto non c'e' — e l'asserzione che lo dice, `ContaEsitoMove(BlockedByUnit) == 0`, non e' stata
	// toccata. ⚠️ Il nome e' cambiato con la cosa misurata: non c'e' piu' un *replan*.
	PC->HandleClickOnCell(Occupata);
	if (!TestEqual(TEXT("premessa: il primo clic E' stato accettato"), Chi->GetPlan().PlannedWaypoints().Num(), 1))
	{
		DestroyInteractionWorld(World);
		return false;
	}
	PC->HandleClickOnCell(Ripiego);
	if (!TestEqual(TEXT("premessa: i due clic si sommano in un piano di due tappe"), Chi->GetPlan().PlannedWaypoints().Num(), 2)
		|| !TestEqual(TEXT("premessa: e il piano finale punta al ripiego"), Chi->GetPlan().PlannedCell, Ripiego))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	RisolviIlTurno(TM);

	// 🔴 Nessun diniego: la cella dell'occupante si **attraversa** ([D-445]) e il piano arriva dove
	// puntava. Su TUTTO il log, non solo sulla voce dell'unita': un evento scritto al momento del click
	// starebbe nel log anche se la voce finale fosse corretta.
	TestEqual(TEXT("nessun diniego nel log: il piano e' arrivato"),
		ContaEsitoMove(TM, ERTMoveOutcome::BlockedByUnit), 0);

	const TArray<FRTTurnLogEntry> Voci = VociMoveDi(TM, Chi);
	if (!TestEqual(TEXT("una sola voce Move: il turno ha un esito, non due"), Voci.Num(), 1))
	{
		DestroyInteractionWorld(World);
		return false;
	}
	TestEqual(TEXT("e l'esito e' il movimento eseguito"),
		Voci[0].Outcome, static_cast<uint8>(ERTMoveOutcome::Moved));
	TestEqual(TEXT("con la destinazione finale, non col waypoint intermedio"),
		Voci[0].TgtCell, Ripiego);

	TestEqual(TEXT("l'unita' e' davvero sul ripiego"), Chi->Cell, Ripiego);
	// ➕ **E l'occupante e' ancora dov'era**: ci si e' passati sopra, non lo si e' spostato ([D-289]).
	TestEqual(TEXT("e l'occupante non si e' mosso"), Occupante->Cell, Occupata);

	DestroyInteractionWorld(World);
	return true;
}

/**
 * BOUNDARY — NON TUTTI I RIFIUTI SONO LO STESSO RIFIUTO.
 *
 * Il contratto della wave e' esplicito sul confine (§ EXPECTED BEHAVIOR, riga `Failure`): un rifiuto
 * di pianificazione con motivo diverso da «destinazione occupata» — budget esaurito, cella non
 * percorribile, fuori mappa — resta `Stayed`, comportamento invariato. E' una scelta di scope della
 * #79, dichiarata per non farla scambiare per una svista dal prossimo lettore.
 *
 * 🔴 **Senza questo test la scelta non e' difesa da niente.** Un classificatore che scrivesse il
 * diniego a ogni ramo di rifiuto — invece di delegare a `ClassifyWaypointCell`, che i motivi li
 * distingue gia' — supererebbe i casi A, B e C: A non ha rifiuti, B ha un rifiuto per occupazione, C
 * finisce con un piano valido. Nessuno dei tre passa da un rifiuto di ALTRO motivo, e questo si'.
 *
 * ⚠️ **Una sola unita' nel mondo, ed e' deliberato.** Su una cella libera di una mappa senza ostacoli
 * l'unico motivo di rifiuto possibile e' il budget: non c'e' nessuno da cui essere bloccati. Un
 * `BlockedByUnit` che comparisse qui non avrebbe nemmeno un occupante a cui riferirsi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBudgetDenialIsNotAUnitDenialTest,
	"RefactorTactics.PlayerInteraction.BudgetDenialIsNotAUnitDenial",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBudgetDenialIsNotAUnitDenialTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	// Raggio largo: la cella oltre il budget deve restare DENTRO la mappa, altrimenti il rifiuto
	// sarebbe «fuori dalla mappa» e il test misurerebbe un motivo diverso da quello che dice.
	//
	// ⏱️ *Il raggio era il letterale `12` fino al 2026-09-15, e [D-425] lo ha reso insufficiente da
	// sotto*: col tetto a `2x` quel numero e' diventato esattamente il tetto di Ivrin, e la cella
	// «fuori portata» sarebbe finita fuori MAPPA — cioe' il rifiuto giusto per il motivo sbagliato, che
	// e' il difetto contro cui questo stesso commento metteva in guardia. Ora si ricava dall'eroe.
	const URTHeroData* Eroe = URTHeroCatalogLibrary::MakeIvrin();
	if (!TestNotNull(TEXT("l'eroe del catalogo"), Eroe))
	{
		DestroyInteractionWorld(World);
		return false;
	}
	const int32 Raggio = 2 * Eroe->MovePoints + 4;
	if (!TestNotNull(TEXT("mappa ampia e senza ostacoli"), SpawnCleanInteractionMap(World, Raggio)))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	const FRTCellId Partenza(0, 0);
	ARTUnit* Chi = SpawnInteractionUnit(World, 0, Eroe, Partenza);
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("unita'"), Chi) || !TestNotNull(TEXT("controller"), PC)
		|| !TestNotNull(TEXT("turn manager"), TM))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	PC->SelectActorForTest(Chi);
	Chi->SelectAbility(INDEX_NONE);

	// La distanza si chiede al budget invece di scriverla: un eroe ribilanciato sposterebbe il confine,
	// e un letterale renderebbe il test verde su un click che il budget copre ancora. E' la stessa
	// trappola gia' misurata su `WaypointClicksBuildAndRejectPlans`, dove quattro celle bastavano al
	// Ranger legacy e non piu' a chi lo ha sostituito.
	// Dal 2026-09-15 il numero da chiedere e' il TETTO, non il movimento base: [D-425] punto (9) porta
	// a `2x` quanto si puo' PIANIFICARE senza dichiarare niente. Con `GetEffectiveMoveRange() + 2` la
	// cella «fuori portata» e' finita DENTRO il tetto, e la premessa del test e' diventata falsa — la
	// stessa trappola che il commento qui sopra descrive, scattata una seconda volta per un confine
	// che si e' spostato invece che per un eroe ribilanciato.
	const int32 Budget = URTMovementProfileLibrary::CeilingProfile(
		Chi->GetPlan().PlannedMovementProfileId(), NAME_None).ResolveStepBudget(Chi->GetEffectiveMoveRange());
	if (!TestTrue(TEXT("premessa: l'eroe ha un tetto di movimento"), Budget > 0))
	{
		DestroyInteractionWorld(World);
		return false;
	}
	const FRTCellId Lontana(Budget + 2, 0, 0); // libera, sulla mappa, e fuori dal tetto
	// ⛔ La premessa «sulla mappa» si ASSERISCE: se cadesse, il rifiuto arriverebbe lo stesso e il test
	// resterebbe verde misurando «fuori dalla mappa» invece di «oltre il tetto».
	if (!TestTrue(TEXT("premessa: la cella oltre il tetto e' ancora DENTRO la mappa"),
		Lontana.X <= Raggio))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	PC->HandleClickOnCell(Lontana);
	if (!TestEqual(TEXT("premessa: il waypoint e' stato rifiutato"), Chi->GetPlan().PlannedWaypoints().Num(), 0))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	RisolviIlTurno(TM);

	// 🔴 Il confine. Il rifiuto c'e' stato, ma non e' un'occupazione: la #79 non lo copre.
	TestEqual(TEXT("un rifiuto per BUDGET non diventa un diniego da unita'"),
		ContaEsitoMove(TM, ERTMoveOutcome::BlockedByUnit), 0);

	const TArray<FRTTurnLogEntry> Voci = VociMoveDi(TM, Chi);
	if (!TestEqual(TEXT("una sola voce Move per l'unita'"), Voci.Num(), 1))
	{
		DestroyInteractionWorld(World);
		return false;
	}
	TestEqual(TEXT("l'esito resta quello di prima della #79"),
		Voci[0].Outcome, static_cast<uint8>(ERTMoveOutcome::Stayed));

	TestEqual(TEXT("e l'unita' non si e' mossa"), Chi->Cell, Partenza);

	DestroyInteractionWorld(World);
	return true;
}

// --- #3145 · `TAB`: il percorso tastiera della selezione ----------------------------------------------

/**
 * Il ciclo della selezione (`N` da D-488, prima `TAB`) visita le proprie unita' in ordine STABILE, e non
 * dichiara niente. Il nome del test era `TabCyclesOwnUnitsInStableOrder` fino al 2026-10-10: il tasto e'
 * cambiato, la regola no, e il test interroga la regola (`CycleSelectionForTest`), non il tasto.
 *
 * 🔴 **L'asserzione che porta il peso e' la seconda**: due cicli completi sullo stesso stato danno la
 * stessa sequenza. Un ciclo che seguisse l'ordine di `GetAllActorsOfClass` potrebbe darne due diverse, e
 * sarebbe un input di planning non riproducibile — il difetto contro cui l'ordinamento per `StableUnitId`
 * esiste.
 *
 * ⚠️ **Il filtro «solo le unita' non ancora pronte» non e' qui perche' non e' esprimibile**: `ARTUnit` non
 * porta uno stato di dichiarazione conclusa, e il lock-in e' del turno. Sta scritto su `CycleSelection`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCycleSelectionTest,
	"RefactorTactics.PlayerInput.SelectionCycleVisitsOwnUnitsInStableOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCycleSelectionTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	// Tre proprie e una avversaria: l'avversaria non deve mai entrare nel ciclo.
	ARTUnit* A = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(2, -2, 0));
	ARTUnit* B = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(3, -2, 0));
	ARTUnit* C = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(3, -1, 0));
	ARTUnit* Nemica = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-2, 2, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("A"), A)
		|| !TestNotNull(TEXT("B"), B) || !TestNotNull(TEXT("C"), C) || !TestNotNull(TEXT("nemica"), Nemica))
	{
		DestroyInteractionWorld(World); return false;
	}

	// Id distinti e dichiarati: l'ordine atteso e' il loro, non quello di spawn.
	A->StableUnitId = 30;
	B->StableUnitId = 10;
	C->StableUnitId = 20;
	Nemica->StableUnitId = 5;

	// --- 1. Il ciclo tocca le proprie, in ordine di StableUnitId ------------------------------------
	TArray<FString> Prima;
	for (int32 i = 0; i < 3; ++i)
	{
		PC->CycleSelectionForTest();
		const ARTUnit* Sel = PC->GetSelectedUnit();
		if (!TestNotNull(TEXT("il ciclo seleziona qualcuno"), Sel))
		{
			DestroyInteractionWorld(World); return false;
		}
		Prima.Add(Sel->GetName());
		TestTrue(TEXT("mai l'unita' avversaria"), Sel != Nemica);
	}
	TestEqual(TEXT("primo giro: B (10)"), Prima[0], B->GetName());
	TestEqual(TEXT("poi C (20)"), Prima[1], C->GetName());
	TestEqual(TEXT("poi A (30)"), Prima[2], A->GetName());

	// --- 2. 🔴 Lo stesso stato da' la stessa sequenza ------------------------------------------------
	TArray<FString> Dopo;
	for (int32 i = 0; i < 3; ++i)
	{
		PC->CycleSelectionForTest();
		Dopo.Add(PC->GetSelectedUnit()->GetName());
	}
	TestEqual(TEXT("il secondo giro ripete il primo"), Dopo, Prima);

	// --- 3. TAB non dichiara niente ------------------------------------------------------------------
	{
		PC->SelectActorForTest(A);
		PC->HandleClickOnCell(FRTCellId(3, -3, 0));
		const int32 WaypointDiA = A->GetPlan().PlannedWaypoints().Num();
		const int32 ArmataDiA = A->SelectedAbilityIndex;

		PC->CycleSelectionForTest();

		TestEqual(TEXT("i waypoint di chi si lascia restano"), A->GetPlan().PlannedWaypoints().Num(), WaypointDiA);
		TestEqual(TEXT("e lo stato armato con loro"), A->SelectedAbilityIndex, ArmataDiA);
		TestTrue(TEXT("chi si prende non ha waypoint inventati"),
			PC->GetSelectedUnit()->GetPlan().PlannedWaypoints().Num() == 0);
	}

	DestroyInteractionWorld(World);
	return true;
}

/**
 * La dichiarazione «ho deciso le mie mosse»: `TAB` salta chi l'ha data, e il flag non sopravvive al turno.
 *
 * 🔴 **La prima asserzione e' quella che distingue il DICHIARATO dal DERIVATO**: l'unita' che dichiara in
 * questo test non ha pianificato **niente**. Ogni formula sui campi `Planned*` la direbbe «non ancora
 * guardata»; solo un flag dichiarato la distingue da chi non e' stato aperto. E' il caso su cui la
 * decisione d'autore del 2026-09-15 ha scelto fra le tre uscite.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDeclaredTurnPlanTest,
	"RefactorTactics.PlayerInput.DeclaredPlanLeavesTheTabCycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDeclaredTurnPlanTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	ARTUnit* A = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(2, -2, 0));
	ARTUnit* B = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(3, -2, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("A"), A) || !TestNotNull(TEXT("B"), B)
		|| !TestNotNull(TEXT("turn manager"), TM))
	{
		DestroyInteractionWorld(World); return false;
	}
	A->StableUnitId = 10;
	B->StableUnitId = 20;

	// --- 1. 🔴 Si dichiara SENZA aver pianificato niente --------------------------------------------
	{
		PC->SelectActorForTest(A);
		TestEqual(TEXT("premessa: A non ha pianificato nulla"), A->GetPlan().PlannedWaypoints().Num(), 0);
		TestFalse(TEXT("premessa: A non ha dichiarato"), A->bTurnPlanDeclared);

		TestTrue(TEXT("Enter dichiara"), PC->ToggleTurnPlanDeclaredForTest());
		TestTrue(TEXT("A risulta dichiarata pur senza piano"), A->bTurnPlanDeclared);
	}

	// --- 2. TAB salta chi ha dichiarato --------------------------------------------------------------
	{
		PC->SelectActorForTest(nullptr);
		TestTrue(TEXT("il ciclo trova ancora qualcuno"), PC->CycleSelectionForTest());
		TestEqual(TEXT("e non e' A, che ha dichiarato"), PC->GetSelectedUnit(), B);

		// Un secondo giro non riporta su A: resta fuori dal ciclo finche' non ritratta.
		PC->CycleSelectionForTest();
		TestEqual(TEXT("il ciclo resta su B"), PC->GetSelectedUnit(), B);
	}

	// --- 3. Si ritratta, e il ciclo la riprende -----------------------------------------------------
	{
		PC->SelectActorForTest(A);
		TestTrue(TEXT("Enter ritratta"), PC->ToggleTurnPlanDeclaredForTest());
		TestFalse(TEXT("A non e' piu' dichiarata"), A->bTurnPlanDeclared);

		PC->SelectActorForTest(B);
		PC->CycleSelectionForTest();
		TestEqual(TEXT("il ciclo torna su A"), PC->GetSelectedUnit(), A);
	}

	// --- 4. 🔴 Il flag non sopravvive al turno -------------------------------------------------------
	{
		PC->SelectActorForTest(A);
		PC->ToggleTurnPlanDeclaredForTest();
		B->SetTurnPlanDeclared(true);
		TestTrue(TEXT("premessa: entrambe dichiarate"), A->bTurnPlanDeclared && B->bTurnPlanDeclared);

		TM->LockInAndResolve();

		TestFalse(TEXT("A riparte non dichiarata"), A->bTurnPlanDeclared);
		TestFalse(TEXT("B riparte non dichiarata"), B->bTurnPlanDeclared);
	}

	DestroyInteractionWorld(World);
	return true;
}

/**
 * Il troncamento del piano al budget di un profilo: si scartano waypoint INTERI, dalla fine.
 *
 * 🔴 **L'asserzione che porta il peso e' la terza**: la cella finale e' sempre una che il giocatore ha
 * CLICCATO. Un troncamento al costo esatto — fermarsi a meta' fra due waypoint — passerebbe le prime due e
 * cadrebbe su questa, ed e' esattamente la mutazione contro cui il test esiste: il repository rifiuta in
 * due punti un percorso che finisca dove nessuno ha chiesto di andare.
 *
 * ⚠️ Il budget dello snapshot e' volutamente ALTO (`9`): il vincolo che si misura dev'essere quello passato
 * alla funzione, non quello che `BuildCompositeHexPath` applica per conto suo. Con un budget stretto
 * nello snapshot il test resterebbe verde anche se la funzione non troncasse nulla.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTruncateWaypointsToBudgetTest,
	"RefactorTactics.PlayerInput.TruncationDropsWholeWaypoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTruncateWaypointsToBudgetTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* MapActor = SpawnCleanInteractionMap(World, /*Radius=*/ 3);
	if (!TestNotNull(TEXT("mappa pulita"), MapActor))
	{
		DestroyInteractionWorld(World); return false;
	}

	const FRTCellId Partenza(0, 0, 0);
	const FRTCellId W1(1, 0, 0);
	const FRTCellId W2(2, 0, 0);
	const FRTCellId W3(3, 0, 0);

	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshotOmniscient(MapActor->MapAsset, {
		FRTHexSimUnit(0, Partenza, /*MoveBudget=*/ 9)
	});

	// --- 1. Un percorso che sta nel budget non perde niente ------------------------------------------
	{
		TArray<FRTCellId> Waypoints = { W1, W2, W3 };
		FRTHexPathResult Kept;
		const int32 Dropped = ARTPlayerController::TruncateWaypointsToBudget(
			Snap, /*UnitId=*/ 0, Waypoints, /*StepBudget=*/ 5, /*CostBudget=*/ 5, Kept);

		TestEqual(TEXT("nessun waypoint scartato"), Dropped, 0);
		TestEqual(TEXT("i tre waypoint restano"), Waypoints.Num(), 3);
		TestEqual(TEXT("e il percorso arriva all'ultimo"), Kept.Path.Last(), W3);
	}

	// --- 2. Un percorso oltre il budget perde i waypoint finali, non i primi -------------------------
	{
		TArray<FRTCellId> Waypoints = { W1, W2, W3 };
		FRTHexPathResult Kept;
		const int32 Dropped = ARTPlayerController::TruncateWaypointsToBudget(
			Snap, /*UnitId=*/ 0, Waypoints, /*StepBudget=*/ 2, /*CostBudget=*/ 2, Kept);

		TestEqual(TEXT("uno scartato"), Dropped, 1);
		TestEqual(TEXT("ne restano due"), Waypoints.Num(), 2);
		// ⛔ Il primo waypoint e' quello che sopravvive: si taglia dalla FINE.
		TestEqual(TEXT("e il primo e' intatto"), Waypoints[0], W1);
		TestTrue(TEXT("il percorso sta nel budget"), Kept.Path.Num() - 1 <= 2);
	}

	// --- 3. 🔴 La cella finale e' sempre una che il giocatore ha cliccato ----------------------------
	{
		// Budget 2 su un percorso da 3: un taglio al costo esatto si fermerebbe su `W2` **passando per**
		// celle intermedie, e con budget dispari finirebbe su una cella mai cliccata. Qui si verifica la
		// proprieta' direttamente, per ogni budget fra 0 e 4.
		for (int32 Budget = 0; Budget <= 4; ++Budget)
		{
			TArray<FRTCellId> Waypoints = { W1, W2, W3 };
			FRTHexPathResult Kept;
			ARTPlayerController::TruncateWaypointsToBudget(
				Snap, /*UnitId=*/ 0, Waypoints, Budget, Budget, Kept);

			if (Kept.Path.Num() > 0)
			{
				const FRTCellId Finale = Kept.Path.Last();
				const bool bCliccata = (Finale == W1) || (Finale == W2) || (Finale == W3);
				TestTrue(*FString::Printf(
					TEXT("budget %d: la destinazione (%d,%d) e' un waypoint cliccato"),
					Budget, Finale.X, Finale.Y), bCliccata);
			}
			else
			{
				TestEqual(*FString::Printf(TEXT("budget %d: senza percorso non resta alcun waypoint"), Budget),
					Waypoints.Num(), 0);
			}
		}
	}

	// --- 4. Budget nullo: non resta niente, e non e' un caso a parte ---------------------------------
	{
		TArray<FRTCellId> Waypoints = { W1, W2, W3 };
		FRTHexPathResult Kept;
		const int32 Dropped = ARTPlayerController::TruncateWaypointsToBudget(
			Snap, /*UnitId=*/ 0, Waypoints, /*StepBudget=*/ 0, /*CostBudget=*/ 0, Kept);

		TestEqual(TEXT("scartati tutti e tre"), Dropped, 3);
		TestEqual(TEXT("nessun waypoint resta"), Waypoints.Num(), 0);
		TestEqual(TEXT("e nessun percorso"), Kept.Path.Num(), 0);
	}

	DestroyInteractionWorld(World);
	return true;
}

/**
 * L'ANCORA DEL CHIAMANTE: armare un'azione che riserva il profilo **tronca** il piano, non lo azzera.
 *
 * 🔴 **Esiste perche' il test della funzione pura non basta.** `TruncationDropsWholeWaypoints` verifica
 * `TruncateWaypointsToBudget`, e resterebbe **verde** se `SelectAbilityForCurrent` tornasse a chiamare
 * `PlannedWaypoints.Reset()`: e' il modulo puro verde mentre il chiamante sbaglia. Questo test guarda il
 * campo DOPO il gesto, che e' l'unico posto in cui la differenza fra le due regole si vede.
 *
 * ⚠️ **Il budget non e' cablato**: `Withdraw` e' una percentuale del movimento dell'eroe ([D-412]), quindi
 * il numero atteso si RICAVA dal profilo. Scrivere `1` qui renderebbe il test una funzione del roster.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReserveTruncatesInsteadOfClearingTest,
	"RefactorTactics.PlayerInput.ReservingAProfileTruncatesThePlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReserveTruncatesInsteadOfClearingTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	ARTUnit* Unit = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(2, -2, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("unita'"), Unit))
	{
		DestroyInteractionWorld(World); return false;
	}

	PC->SelectActorForTest(Unit);
	PC->HandleClickOnCell(FRTCellId(3, -2, 0));
	PC->HandleClickOnCell(FRTCellId(3, -1, 0));
	if (!TestEqual(TEXT("premessa: due waypoint posati"), Unit->GetPlan().PlannedWaypoints().Num(), 2))
	{
		DestroyInteractionWorld(World); return false;
	}
	const TArray<FRTCellId> Dichiarati = Unit->GetPlan().PlannedWaypoints();

	// L'indice si CERCA: le generiche sono accodate al kit, quindi la posizione dipende dall'eroe.
	int32 IdxOverwatch = INDEX_NONE;
	for (int32 i = 0; i < Unit->NumAbilities(); ++i)
	{
		const URTActionData* A = Unit->GetAbility(i);
		if (A && A->Def.ActionId == TEXT("Action.Overwatch")) { IdxOverwatch = i; break; }
	}
	if (!TestTrue(TEXT("premessa: l'eroe ha Action.Overwatch nel kit"), IdxOverwatch != INDEX_NONE))
	{
		DestroyInteractionWorld(World); return false;
	}

	const FRTMovementProfile Withdraw =
		URTMovementProfileLibrary::FindProfile(URTMovementProfileLibrary::ProfileWithdraw);
	const int32 Passi = Withdraw.ResolveStepBudget(Unit->GetEffectiveMoveRange());

	PC->SelectAbilityForCurrentForTest(IdxOverwatch);

	// Fino al 2026-09-15 qui si leggeva `Unit->PlannedMovementProfileId`. Con [D-425] quel campo porta
	// la sola DICHIARAZIONE del giocatore: cio' che l'`Overwatch` impone si legge dal PIANO, ed e' la sede
	// che non puo' restare stantia quando l'azione viene disarmata.
	TestEqual(TEXT("il piano riserva lo slot movimento al Withdraw"),
		URTMovementProfileLibrary::ReservedProfileForPlan(URTPlanValidationLibrary::MakePlanFor(Unit)),
		URTMovementProfileLibrary::ProfileWithdraw);
	TestTrue(TEXT("e la riserva NON si copia sul campo dichiarato: nessuno l'ha dichiarata"),
		Unit->GetPlan().PlannedMovementProfileId().IsNone());

	if (Passi >= 1)
	{
		// 🔴 L'asserzione che distingue le due regole: con l'azzeramento sarebbe `0`.
		TestTrue(TEXT("il piano NON e' azzerato: qualche waypoint sopravvive"),
			Unit->GetPlan().PlannedWaypoints().Num() > 0);
		// ⚠️ **La guardia e' `Num() > 0`, non `Num() <= Dichiarati.Num()`**, e la differenza l'ha trovata una
		// verifica di mutazione: col vecchio azzeramento il primo confronto e' `0 <= 2`, cioe' VERO, e
		// l'indice `[0]` andava a leggere un array vuoto — il test **crashava** invece di fallire, portandosi
		// dietro il worker e gli altri test della run. Un test deve sopravvivere alla mutazione che rileva.
		TestTrue(TEXT("e sopravvivono i PRIMI, in ordine"),
			Unit->GetPlan().PlannedWaypoints().Num() > 0
			&& Unit->GetPlan().PlannedWaypoints().Num() <= Dichiarati.Num()
			&& Unit->GetPlan().PlannedWaypoints()[0] == Dichiarati[0]);
		TestTrue(*FString::Printf(TEXT("il percorso sta nel budget del Withdraw (%d passi)"), Passi),
			FMath::Max(0, Unit->GetPlan().PlannedPath().Num() - 1) <= Passi);
		// La destinazione e' una cella CLICCATA, mai una intermedia.
		TestTrue(TEXT("e la destinazione e' un waypoint dichiarato"),
			Unit->GetPlan().PlannedPath().Num() == 0 || Dichiarati.Contains(Unit->GetPlan().PlannedPath().Last()));
	}
	else
	{
		// Un `Withdraw` che non concede passi azzera per costruzione, e non e' un difetto: e' il troncamento
		// che arriva fino in fondo. Si dichiara invece di lasciare il ramo muto.
		TestEqual(TEXT("budget nullo: nessun waypoint sopravvive"), Unit->GetPlan().PlannedWaypoints().Num(), 0);
	}

	DestroyInteractionWorld(World);
	return true;
}

/**
 * `AC-3` di `#1410`: **dichiarare `Sneak` abbassa il tetto a meta', e il piano che non ci sta si tronca.**
 *
 * E' l'ancora del CHIAMANTE, non del predicato. `MovementProfile.NakedCeilingIsTwiceTheBase` prova
 * che `CeilingProfile` risponde a una dichiarazione; questo prova che **premere il tasto** produce
 * quella dichiarazione e che il percorso gia' disegnato la subisce. Il primo resterebbe verde se
 * `OnToggleSneak` scrivesse nel campo sbagliato, o non troncasse affatto.
 *
 * Il budget non e' cablato: `Sneak` e' una percentuale del movimento dell'eroe ([D-412]), quindi il
 * numero atteso si RICAVA dal profilo. Scrivere `2` qui renderebbe il test una funzione del roster.
 *
 * E il gesto e' un INTERRUTTORE, non un ciclo: la seconda pressione torna al tetto nudo. Senza
 * l'ultima asserzione, un ciclo fra quattro profili che passasse per `Sneak` supererebbe questo test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDeclaringSneakTruncatesToHalfTest,
	"RefactorTactics.PlayerInput.DeclaringSneakTruncatesToHalf",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDeclaringSneakTruncatesToHalfTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	ARTUnit* Unit = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-4, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("unita'"), Unit))
	{
		DestroyInteractionWorld(World); return false;
	}

	const int32 Base = Unit->GetEffectiveMoveRange();
	const FRTMovementProfile Sneak =
		URTMovementProfileLibrary::FindProfile(URTMovementProfileLibrary::ProfileSneak);
	const int32 PassiSneak = Sneak.ResolveStepBudget(Base);

	PC->SelectActorForTest(Unit);

	// Un piano che sta nel tetto NUDO e non in quello dello `Sneak`: e' l'unico che distingue le due
	// regole. La lunghezza si RICAVA dal movimento base — `Base` passi contro un tetto di `Base/2` —
	// invece di essere cablata: due waypoint bastavano su un eroe da 4 e non su uno da 6, e la prima
	// stesura di questo test e' fallita esattamente cosi' (piano da 2 passi, tetto da 3).
	for (int32 Passo = 1; Passo <= Base; ++Passo)
	{
		PC->HandleClickOnCellForTest(FRTCellId(-4 + Passo, 0, 0));
	}
	const int32 PassiPrima = FMath::Max(0, Unit->GetPlan().PlannedPath().Num() - 1);
	if (!TestTrue(*FString::Printf(
			TEXT("premessa: il piano (%d passi) eccede il tetto dello Sneak (%d)"), PassiPrima, PassiSneak),
		PassiPrima > PassiSneak))
	{
		DestroyInteractionWorld(World); return false;
	}
	const TArray<FRTCellId> Dichiarati = Unit->GetPlan().PlannedWaypoints();

	PC->ToggleSneakForTest();

	TestEqual(TEXT("lo Sneak e' dichiarato sul campo del piano"),
		Unit->GetPlan().PlannedMovementProfileId(), URTMovementProfileLibrary::ProfileSneak);
	TestTrue(*FString::Printf(TEXT("e il percorso sta nel tetto dimezzato (%d passi)"), PassiSneak),
		FMath::Max(0, Unit->GetPlan().PlannedPath().Num() - 1) <= PassiSneak);

	if (PassiSneak >= 1)
	{
		// L'asserzione che distingue il troncamento dall'azzeramento: con `Reset()` sarebbe `0`.
		// La guardia e' `Num() > 0` PRIMA di indicizzare, per la stessa ragione misurata su
		// `ReservingAProfileTruncatesThePlan`: `Num() <= Dichiarati.Num()` e' vero anche per un array vuoto,
		// e l'indice `[0]` farebbe crashare il worker invece di far fallire il test.
		TestTrue(TEXT("il piano NON e' azzerato: qualche waypoint sopravvive"),
			Unit->GetPlan().PlannedWaypoints().Num() > 0);
		TestTrue(TEXT("e sopravvivono i PRIMI, in ordine"),
			Unit->GetPlan().PlannedWaypoints().Num() > 0
			&& Unit->GetPlan().PlannedWaypoints().Num() <= Dichiarati.Num()
			&& Unit->GetPlan().PlannedWaypoints()[0] == Dichiarati[0]);
	}

	// Seconda pressione: si torna al tetto nudo. E' cio' che rende il gesto un interruttore invece di
	// un ciclo - la forma che [D-425] punto (7) chiede esplicitamente.
	PC->ToggleSneakForTest();
	TestTrue(TEXT("la seconda pressione annulla la dichiarazione"),
		Unit->GetPlan().PlannedMovementProfileId().IsNone());

	DestroyInteractionWorld(World);
	return true;
}

/**
 * [D-425] punto (9): **col movimento nudo si pianifica fino a `2x`**, e la banda `Sprint` e' raggiungibile
 * senza dichiarare niente ne' equipaggiare una skill.
 *
 * E' l'ancora che chiude il cerchio fra tetto e banda, e nessuna delle due meta' la copre da sola.
 * Il test puro prova che `CeilingProfile` risponde `2x`; questo prova che un giocatore che clicca **ci
 * arriva davvero** - cioe' che il tetto attraversa `MakeSimUnit`, lo snapshot e `HandleClickOnCell` senza
 * essere abbassato da nessuno lungo la strada. Con il tetto vecchio a `1x` il waypoint oltre la portata
 * sarebbe stato rifiutato, e la banda alta irraggiungibile: una banda che non e' una banda.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNakedPlanningReachesTwiceTheBaseTest,
	"RefactorTactics.PlayerInput.NakedPlanningReachesTwiceTheBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNakedPlanningReachesTwiceTheBaseTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	ARTUnit* Unit = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-4, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("unita'"), Unit))
	{
		DestroyInteractionWorld(World); return false;
	}

	const int32 Base = Unit->GetEffectiveMoveRange();
	if (!TestTrue(TEXT("premessa: l'eroe ha un movimento base"), Base >= 2))
	{
		DestroyInteractionWorld(World); return false;
	}

	PC->SelectActorForTest(Unit);

	// Si clicca UNA cella alla volta lungo una riga, oltre `1x`: e' il gesto reale, e ogni passo che il
	// tetto vecchio avrebbe rifiutato si vedrebbe qui come un piano che smette di crescere.
	for (int32 Passo = 1; Passo <= Base + 1; ++Passo)
	{
		PC->HandleClickOnCellForTest(FRTCellId(-4 + Passo, 0, 0));
	}

	const int32 Passi = FMath::Max(0, Unit->GetPlan().PlannedPath().Num() - 1);
	TestTrue(*FString::Printf(TEXT("si pianifica oltre 1x: %d passi su base %d"), Passi, Base),
		Passi > Base);

	// E cio' che si e' pianificato si LEGGE `Sprint`, senza che nessuno lo abbia dichiarato: e' la forma
	// di [D-425] punto (2) - il prezzo segue la distanza, non una scelta.
	TestEqual(TEXT("e la banda che ne risulta e' Sprint, senza dichiararlo"),
		URTMovementProfileLibrary::ProfileForPlan(URTPlanValidationLibrary::MakePlanFor(Unit)).Id,
		URTMovementProfileLibrary::ProfileSprint);
	TestTrue(TEXT("e nessuno ha dichiarato niente"), Unit->GetPlan().PlannedMovementProfileId().IsNone());

	DestroyInteractionWorld(World);
	return true;
}

/**
 * `AC-1` di `#1410`: dichiarare `Sneak` **ridisegna le celle raggiungibili**, subito.
 *
 * Trovato in PIE dall'utente il 2026-09-17: *«se premo M gli esagoni verdi non cambiano immediatamente»*.
 * La causa era che `RTPlanningPreview::Refresh` viveva nel solo ramo del TRONCAMENTO, e i due rami che escono
 * prima — piano vuoto, e percorso che ci sta gia' — lo saltavano. Il primo e' il caso normale: chi preme
 * `M` prima di cliccare vedeva le celle del tetto VECCHIO.
 *
 * 🔴 **Nessun test lo prendeva, e la ragione e' istruttiva**: `DeclaringSneakTruncatesToHalf` guarda
 * `PlannedMovementProfileId` e `PlannedWaypoints`, che erano **corretti** — la dichiarazione arrivava, il
 * piano si troncava. A mentire era solo cio' che il giocatore VEDE, e nessuna asserzione lo guardava. E'
 * il modulo verde mentre il chiamante sbaglia, con la presentazione al posto del modulo.
 *
 * ⚠️ **Il caso in esame e' SENZA waypoint**, perche' e' quello che il difetto colpiva e quello in cui il
 * giocatore preme il tasto: prima di disegnare, per sapere fin dove puo' arrivare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSneakRedrawsReachablePreviewTest,
	"RefactorTactics.PlayerInput.SneakRedrawsTheReachablePreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSneakRedrawsReachablePreviewTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	ARTUnit* Unit = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("unita'"), Unit)
		|| !TestNotNull(TEXT("actor mappa"), MapActor))
	{
		DestroyInteractionWorld(World); return false;
	}

	// ⚠️ **`SelectUnit` e non `SelectActorForTest` ne' `HandleClickOnUnitForTest`.** Il primo scrive il
	// campo e basta — l'anteprima resterebbe vuota. Il secondo e' il clic su un'unita' *mentre un'altra e'
	// gia' selezionata* (targeting) ed esce subito senza selezione preesistente: misurato, da' `0` celle.
	// `SelectUnit` e' il punto in cui la selezione DISEGNA, ed e' la ragione per cui la premessa qui sotto
	// e' asserita invece che assunta — con zero contro zero il test sarebbe passato dicendo niente.
	PC->SelectUnit(Unit);

	const int32 Prima = MapActor->NumPreviewReachableCells();
	if (!TestTrue(*FString::Printf(
			TEXT("premessa: selezionare disegna un'anteprima non vuota (%d celle)"), Prima), Prima > 0))
	{
		DestroyInteractionWorld(World); return false;
	}
	if (!TestEqual(TEXT("premessa: nessun waypoint posato, come quando si preme M per primo"),
		Unit->GetPlan().PlannedWaypoints().Num(), 0))
	{
		DestroyInteractionWorld(World); return false;
	}

	// --- dichiarare Sneak dimezza il tetto: le celle raggiungibili devono CALARE, e subito ---------------
	PC->ToggleSneakForTest();
	const int32 Dopo = MapActor->NumPreviewReachableCells();

	TestTrue(*FString::Printf(TEXT("l'anteprima si restringe col tetto dimezzato (%d -> %d)"), Prima, Dopo),
		Dopo < Prima);
	// ⛔ **E non si svuota**: sgusciare e' andare meno lontano, non restare fermi. Senza questa riga un
	// ridisegno che azzerasse l'anteprima passerebbe il confronto qui sopra.
	TestTrue(TEXT("ma non si svuota: lo Sneak riduce la distanza, non la annulla"), Dopo > 0);

	// --- annullare la dichiarazione la fa TORNARE -------------------------------------------------------
	// 🔑 E' l'asserzione piu' forte delle tre: prova che il ridisegno avviene nei DUE versi. Un ridisegno
	// che restringesse soltanto — o che avvenisse una volta sola — cadrebbe qui.
	PC->ToggleSneakForTest();
	TestEqual(TEXT("annullando lo Sneak l'anteprima torna quella di prima"),
		MapActor->NumPreviewReachableCells(), Prima);

	DestroyInteractionWorld(World);
	return true;
}

/**
 * UNA DESTINAZIONE DI MOVIMENTO E' UN ORDINE; UN WAYPOINT RIFIUTATO NO — `#3400`.
 *
 * 🔑 **Trovato eseguendo `PIE-PACING-1` in seduta**, non da una rilettura: un turno col solo waypoint
 * dava `OrderCount` **0** mentre `UndoCount` saliva a 1. Il contratto di `Turn/Pacing/RTPacing.h:21` dichiara
 * `Order` come «abilita' **o destinazione**», e la meta' «destinazione» non era cablata.
 *
 * ⚠️ **Il secondo caso e' la meta' che conta.** Con la sola asserzione sull'accettato, un `Order`
 * registrato in cima a `HandleClickOnCell` passerebbe — e conterebbe come ordine ogni clic su una cella
 * bloccata o fuori budget, gonfiando la metrica che `#971` usa per distinguere «sto pensando» da «sto
 * litigando con l'interfaccia». Le due asserzioni insieme fissano la POSIZIONE della chiamata, non la
 * sua presenza.
 *
 * ⚠️ **Non e' il contatore che si prova qui**: quello e' gia' coperto da
 * `RefactorTactics.Pacing.RecordsDecisionComposition`, che chiama `RecordPlanningInput` a mano. Si prova
 * la **catena** dal gesto al campione, che e' il punto dove il difetto viveva.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementDestinationIsAnOrderTest,
	"RefactorTactics.PlayerInput.MovementDestinationCountsAsAnOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementDestinationIsAnOrderTest::RunTest(const FString&)
{
	// Il campione si legge solo a turno chiuso, quindi ogni caso vuole il proprio mondo.
	auto OrdiniDopoUnClick = [this](const FRTCellId& Bersaglio, int32& OutOrdini, int32& OutWaypoint) -> bool
	{
		UWorld* World = MakeInteractionWorld();
		if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

		URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = Arena;
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		// ⚠️ **Senza questo il campione non si apre mai**, e i contatori finiscono in un `FRTPacingSample`
		// che nessuno chiude: stesso motivo per cui `SpawnHexPacingTurnManager` lo fa in
		// `RTPacingIntegrationTests`. Il primo giro di questo test e' caduto proprio qui, con `OrderCount`
		// a 0 su un waypoint accettato — cioe' con l'aspetto del difetto che stava verificando.
		if (TM) { TM->DispatchBeginPlay(); }

		// Ivrin: 5 punti movimento, stessa posizione del test dei waypoint qui sopra.
		ARTUnit* Unit = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(2, -2, 0));
		// Un avversario: senza, la fine della partita potrebbe chiudere il turno per eliminazione.
		SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(-2, 2, 0));
		ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
		if (!TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("unita'"), Unit) || !TM)
		{
			DestroyInteractionWorld(World); return false;
		}

		PC->SelectActorForTest(Unit);
		PC->HandleClickOnCell(Bersaglio);
		OutWaypoint = Unit->GetPlan().PlannedWaypoints().Num();

		RTWorldFixtures::PlayOneTurn(TM);
		if (!TestTrue(TEXT("un campione chiuso"), TM->GetPacingSamples().Num() >= 1))
		{
			DestroyInteractionWorld(World); return false;
		}
		OutOrdini = TM->GetPacingSamples()[0].OrderCount;
		DestroyInteractionWorld(World);
		return true;
	};

	// (1) Destinazione ACCETTATA -> un ordine.
	{
		int32 Ordini = -1, Waypoint = -1;
		if (!OrdiniDopoUnClick(FRTCellId(3, -2, 0), Ordini, Waypoint)) { return false; }
		// Controllo positivo dell'allestimento: se il click non avesse prodotto un waypoint, uno zero
		// sugli ordini direbbe «il fix non c'e'» quando invece il bersaglio era irraggiungibile.
		TestEqual(TEXT("il click e' stato ACCETTATO (allestimento valido)"), Waypoint, 1);
		TestEqual(TEXT("una destinazione accettata vale UN ordine"), Ordini, 1);
	}

	// (2) Destinazione RIFIUTATA -> nessun ordine. Cella bloccata della mappa di prova.
	{
		int32 Ordini = -1, Waypoint = -1;
		if (!OrdiniDopoUnClick(FRTCellId(2, 1, 0), Ordini, Waypoint)) { return false; }
		TestEqual(TEXT("il click e' stato RIFIUTATO (allestimento valido)"), Waypoint, 0);
		TestEqual(TEXT("un tentativo respinto NON e' un ordine"), Ordini, 0);
	}

	return true;
}

/**
 * UNA DESTINAZIONE DI SCATTO E' UN ORDINE; UNO SCATTO RIFIUTATO NO — la seconda meta' di `#3400`.
 *
 * 🔴 **Il difetto.** `HandleClickOnCell` aveva una sola chiamata a `RecordPlanningInput(Order)`, in coda al
 * ramo del waypoint. I due rami della mobilita' rapida — lineare e a budget — scrivevano `PlannedDashCell` e
 * uscivano con `return` prima di arrivarci: una cella d'arrivo dichiarata con uno scatto non contava.
 *
 * 🔑 **Il contatore misura GESTI, non lunghezza del piano** (`Turn/Pacing/RTPacing.h`: «abilita' **o**
 * destinazione»), e le due meta' dello scatto non contano con la stessa regola. La pressione del tasto la
 * registra `SelectAbilityForCurrent` appena raggiunge un'unita' selezionata, prima di validare. La destinazione
 * conta solo se accettata. Per questo qui l'arm passa dal controller e non da `ARTUnit::SelectAbility`: il
 * numero atteso e' quello che il giocatore produce davvero. ⚠️ Ne segue che un turno di solo scatto contava
 * gia' UNO prima della correzione, non zero — e i casi di rifiuto qui sotto lo fissano.
 *
 * ⚠️ **Sono DUE chiamate in due rami, e ciascuna ha il proprio caso accettato.** Con un caso solo, una delle due
 * resterebbe senza un test che cada quando viene tolta. E ciascun ramo ha il proprio caso RIFIUTATO: con la sola
 * asserzione sull'accettato, una chiamata spostata in cima al blocco dello scatto passerebbe — e conterebbe
 * come ordine ogni cella non allineata o fuori budget.
 *
 * ⚠️ **Una destinazione ripetuta conta ogni volta.** Due celle valide una dopo l'altra sono due gesti: il piano
 * tiene l'ultima, il contatore ne registra due. E' un caso a se', perche' e' la lettura che distingue «gesti» da
 * «destinazioni nel piano».
 *
 * ⚠️ **Il ramo a budget si raggiunge con un'azione SINTETICA**, e la premessa e' asserita in cima al test invece
 * che scritta in un commento: ogni mobilita' rapida del catalogo e del roster dichiara uno stile lineare.
 * `Action.Sprint`, per cui il ramo era nato, risolve nel movimento normale. La prova lo esercita lo stesso,
 * perche' la chiamata c'e' e deve stare al posto giusto il giorno in cui un'azione ci arrivera' — e quel giorno
 * la premessa cade e dice di sostituire la sintetica con l'azione vera.
 *
 * ⛔ **L'ultimo caso NON e' un gesto del giocatore, ed e' dichiarato per quello che e'.** Chiama
 * `HandleClickOnUnit` dalla porta di test, con una carica armata e un nemico in linea: prova che quella
 * funzione INOLTRA alla cella del nemico e che l'ordine si conta una volta sola, non due. Il click vero non ci
 * arriva: con una mobilita' rapida armata il puntatore non e' in `Targeting`, e `DispatchUnitClick` risponde
 * `Inspect` a un nemico osservato. Che la carica sul nemico non si pianifichi dal click vero e' un difetto a
 * se', con la sua issue (`#3736`); qui non lo si copre e non lo si nasconde.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDashDestinationIsAnOrderTest,
	"RefactorTactics.PlayerInput.DashDestinationCountsAsAnOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDashDestinationIsAnOrderTest::RunTest(const FString&)
{
	// ── Premessa del ramo a budget: nessuna mobilita' rapida SPEDITA ha uno stile non lineare ──
	{
		TArray<TPair<FString, FRTActionDef>> Spedite;
		for (const FRTActionDef& Def : URTCatalogLibrary::GetCoreActionCatalog())
		{
			Spedite.Emplace(FString::Printf(TEXT("catalogo, %s"), *Def.ActionId.ToString()), Def);
		}
		for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
		{
			if (!Hero) { continue; }
			for (int32 i = 0; i < Hero->Actions.Num(); ++i)
			{
				if (const URTActionData* Azione = Hero->Actions[i])
				{
					Spedite.Emplace(FString::Printf(TEXT("%s, azione %d"), *Hero->HeroId.ToString(), i), Azione->Def);
				}
			}
		}
		int32 Rapide = 0;
		for (const TPair<FString, FRTActionDef>& Voce : Spedite)
		{
			if (!URTCatalogLibrary::IsFastMovement(Voce.Value)) { continue; }
			++Rapide;
			TestTrue(*FString::Printf(TEXT("premessa: la mobilita' rapida spedita (%s) ha uno stile lineare — se "
				"cade, il caso «a budget» deve usare quell'azione e non una sintetica"), *Voce.Key),
				URTMovementActionLibrary::IsLinear(Voce.Value.MovementStyle));
		}
		if (!TestTrue(TEXT("premessa: il censimento ha trovato mobilita' rapide (non e' vacuo)"), Rapide > 0))
		{
			return false;
		}
	}

	enum class ECaso : uint8 { Lineare, Budget, CaricaInoltrata };

	// Il campione si legge solo a turno chiuso, quindi ogni caso vuole il proprio mondo.
	auto OrdiniDopoArmEClick = [this](ECaso Caso, const TArray<FRTCellId>& Click, int32& OutOrdini,
		bool& bOutPianificato, FRTCellId& OutCellaPianificata) -> bool
	{
		UWorld* World = MakeInteractionWorld();
		if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

		URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = Arena;
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		// Senza, il campione non si apre mai: vedi `MovementDestinationCountsAsAnOrder` qui sopra.
		if (TM) { TM->DispatchBeginPlay(); }

		// La carica vuole Riktor e un nemico in linea; gli altri due casi, Ivrin e un nemico lontano.
		const bool bCarica = (Caso == ECaso::CaricaInoltrata);
		ARTUnit* Unit = bCarica
			? SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0, 0))
			: SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(2, -2, 0));
		ARTUnit* Nemico = bCarica
			? SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(3, 0, 0))
			: SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(-2, 2, 0));
		ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
		if (!TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("unita'"), Unit)
			|| !TestNotNull(TEXT("nemico"), Nemico) || !TM)
		{
			DestroyInteractionWorld(World); return false;
		}

		int32 Idx = INDEX_NONE;
		if (Caso == ECaso::Budget)
		{
			// Una mobilita' rapida il cui stile NON e' lineare: `Action.Dodge` equipaggiata come la equipaggia
			// la produzione (`RTAbilityFixtures::AddCoreAbility`), con il solo stile cambiato. E' cio' che
			// porta il click nel ramo del pathfinding.
			Idx = RTAbilityFixtures::AddCoreAbility(Unit, TEXT("Action.Dodge"));
			if (Unit->Abilities.IsValidIndex(Idx) && Unit->Abilities[Idx])
			{
				Unit->Abilities[Idx]->Def.MovementStyle = ERTMovementStyle::Budget;
			}
		}
		else
		{
			Idx = Unit->FindDashAbilityIndex();
		}

		const URTActionData* Scatto = Unit->GetAbility(Idx);
		if (!TestNotNull(TEXT("premessa: l'unita' ha la mobilita' rapida del caso"), (void*)Scatto)
			|| !TestTrue(TEXT("premessa: e' una mobilita' rapida"), URTCatalogLibrary::IsFastMovement(Scatto->Def))
			|| !TestTrue(TEXT("premessa: lo stile e' quello del ramo che si vuole esercitare"),
				URTMovementActionLibrary::IsLinear(Scatto->Def.MovementStyle) == (Caso != ECaso::Budget))
			|| (bCarica && !TestTrue(TEXT("premessa: la mobilita' di Riktor e' una carica"),
				Scatto->Def.MovementStyle == ERTMovementStyle::LinearCharge)))
		{
			DestroyInteractionWorld(World); return false;
		}

		PC->SelectActorForTest(Unit);
		PC->SelectAbilityForCurrentForTest(Idx); // l'arm VERO: registra il proprio ordine
		if (bCarica)
		{
			// ⛔ La porta di test, NON il click del giocatore: vedi l'intestazione.
			PC->HandleClickOnUnitForTest(Nemico);
		}
		else
		{
			for (const FRTCellId& Cella : Click)
			{
				PC->HandleClickOnCell(Cella);
			}
		}
		bOutPianificato = (Unit->GetPlan().PlannedDashAbility() == Idx);
		OutCellaPianificata = Unit->GetPlan().PlannedDashCell();

		RTWorldFixtures::PlayOneTurn(TM);
		if (!TestTrue(TEXT("un campione chiuso"), TM->GetPacingSamples().Num() >= 1))
		{
			DestroyInteractionWorld(World); return false;
		}
		OutOrdini = TM->GetPacingSamples()[0].OrderCount;
		DestroyInteractionWorld(World);
		return true;
	};

	struct FCaso
	{
		const TCHAR* Nome;
		ECaso Caso;
		TArray<FRTCellId> Click;   // nell'ordine in cui arrivano
		int32 Accettati;           // quanti di quei click devono diventare un ordine
		FRTCellId CellaAttesa;     // la destinazione che resta nel piano, se almeno uno e' accettato
	};
	const FCaso Casi[] = {
		// (2,-2) + 2 * direzione (-1,+1): in linea, libera, in portata.
		{ TEXT("lineare, cella allineata"), ECaso::Lineare, { FRTCellId(0, 0, 0) }, 1, FRTCellId(0, 0, 0) },
		// Fuori dalle sei direzioni ma con la retta LIBERA: da [D-524] si accetta. ⌦ *Fino a [D-524] era il caso
		// «respinto»* (`(3,-4)`, «cella non allineata»): ora il rifiuto lo da' la retta, non l'asse.
		{ TEXT("lineare, cella fuori asse con la retta libera"), ECaso::Lineare, { FRTCellId(3, -4, 0) }, 1,
			FRTCellId(3, -4, 0) },
		// Fuori asse, a distanza 3 (la portata della lama) e libera sul grafo, ma la sua retta passa dal muro di (1,-2).
		{ TEXT("lineare, cella fuori asse con la retta bloccata"), ECaso::Lineare, { FRTCellId(0, -3, 0) }, 0,
			FRTCellId() },
		// Due celle valide sulla stessa linea: il piano tiene l'ultima, i gesti sono due.
		{ TEXT("lineare, due destinazioni valide di fila"), ECaso::Lineare,
			{ FRTCellId(0, 0, 0), FRTCellId(1, -1, 0) }, 2, FRTCellId(1, -1, 0) },
		// Una valida e poi una respinta: il piano tiene la prima, e la respinta non conta.
		{ TEXT("lineare, una valida e poi una respinta"), ECaso::Lineare,
			{ FRTCellId(0, 0, 0), FRTCellId(0, -3, 0) }, 1, FRTCellId(0, 0, 0) },
		{ TEXT("a budget, cella vicina"), ECaso::Budget, { FRTCellId(3, -2, 0) }, 1, FRTCellId(3, -2, 0) },
		// La cella bloccata della mappa di prova: nessun percorso la raggiunge.
		{ TEXT("a budget, cella bloccata"), ECaso::Budget, { FRTCellId(2, 1, 0) }, 0, FRTCellId() },
		// La cella e' quella del nemico: `HandleClickOnUnit` la inoltra, e l'ordine si conta una volta.
		{ TEXT("carica, HandleClickOnUnit inoltra la cella del nemico"), ECaso::CaricaInoltrata,
			{ FRTCellId(3, 0, 0) }, 1, FRTCellId(3, 0, 0) },
	};

	for (const FCaso& C : Casi)
	{
		int32 Ordini = -1;
		bool bPianificato = false;
		FRTCellId Cella;
		if (!OrdiniDopoArmEClick(C.Caso, C.Click, Ordini, bPianificato, Cella)) { return false; }

		// Il controllo positivo dell'allestimento: senza, un conteggio fermo non distingue «la chiamata manca»
		// da «lo scatto non e' stato accettato».
		const bool bAtteso = (C.Accettati > 0);
		TestTrue(*FString::Printf(TEXT("%s: lo scatto e' %s (allestimento valido)"),
			C.Nome, bAtteso ? TEXT("nel piano") : TEXT("fuori dal piano")), bPianificato == bAtteso);
		if (bAtteso)
		{
			TestTrue(*FString::Printf(TEXT("%s: nel piano resta l'ultima destinazione ACCETTATA"), C.Nome),
				Cella == C.CellaAttesa);
		}
		TestEqual(*FString::Printf(TEXT("%s: un ordine per l'arm, piu' uno per ogni destinazione accettata"), C.Nome),
			Ordini, 1 + C.Accettati);
	}

	return true;
}


/**
 * ARMARE UN'AZIONE CHE RISERVA LO SLOT RESTRINGE IL VENTAGLIO **SUBITO** — `#3418`.
 *
 * 🔑 **Il difetto era di ORDINE, non di calcolo.** `ArmKitAbility` chiamava `RTPlanningPreview::Refresh`
 * dodici righe **sopra** `Unit->PlannedAbilityIndex = Index`, e il ventaglio verde si ricava dal tetto che
 * `RTPlanningSnapshot::For` legge da `ReservedProfileForPlan(MakePlanFor(Unit))` — cioe' **dal piano**. Disegnato
 * prima della scrittura, il piano non conteneva ancora l'azione: nessun tetto, verde intero. Col percorso
 * ciano gia' troncato dalla riga sopra, le due meta' della stessa anteprima dicevano cose diverse nello
 * stesso fotogramma.
 *
 * ⚠️ **La terza asserzione e' quella che nomina il difetto, e le prime due da sole non bastano.** Il
 * difetto si **autocorregge** al gesto successivo, quindi un test che guardasse solo <<il verde si e'
 * ristretto>> potrebbe passare su un'implementazione che lo restringe un attimo tardi. Qui si torna
 * sull'unita' passando da un'altra e si pretende che il conteggio **non cambi**: se il primo disegno era
 * quello giusto, il secondo non ha niente da correggere.
 *
 * 🔑 **L'occupazione e' costante fra le tre letture**: la seconda unita' viene generata **prima** della
 * misura di partenza, cosi' il cambio di selezione non sposta nulla di cio' che si conta. Senza questa
 * accortezza il terzo confronto misurerebbe anche un'occupazione diversa.
 *
 * ⛔ **Non si asserisce il DISARMO qui**: `ArmKitAbility(INDEX_NONE)` non azzera `PlannedAbilityIndex`, e
 * il tetto resterebbe. E' un difetto diverso e ha la sua issue (`#3417`); metterlo qui darebbe un rosso che
 * non parla di questa correzione.
 *
 * Gemello di `SneakRedrawsTheReachablePreview`, che prova la stessa proprieta' per il tetto **dichiarato**
 * dal giocatore, e vicino di `ReservingAProfileTruncatesThePlan`, che prova il troncamento del percorso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTArmingAReserveRedrawsAtOnceTest,
	"RefactorTactics.PlayerInput.ArmingAReserveShrinksThePreviewAtOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTArmingAReserveRedrawsAtOnceTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	// Le DUE unita' si generano subito: la seconda serve solo a poter cambiare selezione e tornare, e
	// generarla dopo la misura di partenza ne cambierebbe l'occupazione.
	ARTUnit* Unit = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0, 0));
	ARTUnit* Altra = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(2, -2, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("unita'"), Unit)
		|| !TestNotNull(TEXT("seconda unita'"), Altra) || !TestNotNull(TEXT("actor mappa"), MapActor))
	{
		DestroyInteractionWorld(World); return false;
	}

	// L'indice si CERCA: le generiche sono accodate al kit, quindi la posizione dipende dall'eroe.
	int32 IdxOverwatch = INDEX_NONE;
	for (int32 i = 0; i < Unit->NumAbilities(); ++i)
	{
		const URTActionData* A = Unit->GetAbility(i);
		if (A && A->Def.ActionId == TEXT("Action.Overwatch")) { IdxOverwatch = i; break; }
	}
	if (!TestTrue(TEXT("premessa: l'eroe ha Action.Overwatch nel kit"), IdxOverwatch != INDEX_NONE))
	{
		DestroyInteractionWorld(World); return false;
	}

	// ⚠️ `SelectUnit` e non `SelectActorForTest`: il primo e' il punto in cui la selezione DISEGNA. Con
	// `SelectActorForTest` l'anteprima resterebbe vuota e i tre confronti sarebbero zero contro zero.
	PC->SelectUnit(Unit);
	const int32 Prima = MapActor->NumPreviewReachableCells();
	if (!TestTrue(*FString::Printf(
			TEXT("premessa: selezionare disegna un'anteprima non vuota (%d celle)"), Prima), Prima > 0))
	{
		DestroyInteractionWorld(World); return false;
	}

	// --- armare l'Overwatch impone il tetto `Withdraw` (×0,25): il ventaglio deve calare SUBITO ---------
	PC->SelectAbilityForCurrentForTest(IdxOverwatch);
	if (!TestEqual(TEXT("premessa: il piano riserva davvero lo slot al Withdraw"),
			URTMovementProfileLibrary::ReservedProfileForPlan(URTPlanValidationLibrary::MakePlanFor(Unit)),
			URTMovementProfileLibrary::ProfileWithdraw))
	{
		DestroyInteractionWorld(World); return false;
	}

	const int32 Dopo = MapActor->NumPreviewReachableCells();
	// 🔴 Col refresh chiamato prima della scrittura del piano, qui `Dopo` era uguale a `Prima`.
	TestTrue(*FString::Printf(TEXT("il ventaglio si restringe col tetto imposto (%d -> %d)"), Prima, Dopo),
		Dopo < Prima);
	// ⛔ E non si svuota: il `Withdraw` e' un ripiegamento corto, non un'immobilizzazione. Senza questa
	// riga un ridisegno che azzerasse l'anteprima passerebbe il confronto qui sopra.
	TestTrue(TEXT("ma non si svuota: il ripiegamento riduce la distanza, non la annulla"), Dopo > 0);

	// --- e il gesto successivo NON ha niente da correggere --------------------------------------------
	PC->SelectUnit(Altra);
	PC->SelectUnit(Unit);
	const int32 Terzo = MapActor->NumPreviewReachableCells();
	TestEqual(TEXT("il primo disegno era gia' quello giusto: il gesto dopo non lo cambia"), Terzo, Dopo);

	DestroyInteractionWorld(World);
	return true;
}

// ===================================================================================================
// #3417 / [D-444] - un tetto che si ALZA restituisce cio' che aveva tolto
// ===================================================================================================

namespace
{
	/** L'allestimento comune dei test di [D-444]: mondo, arena, mappa, turn manager, unita', controller. */
	struct FRTBancoTetto
	{
		UWorld* World = nullptr;
		ARTHexMapActor* MapActor = nullptr;
		ARTUnit* Unit = nullptr;
		ARTPlayerController* PC = nullptr;
		int32 IdxOverwatch = INDEX_NONE;

		bool Valido() const { return World && MapActor && Unit && PC && IdxOverwatch != INDEX_NONE; }
	};

	FRTBancoTetto AllestisciBancoTetto(const FRTCellId& Partenza)
	{
		FRTBancoTetto B;
		B.World = MakeInteractionWorld();
		if (!B.World) { return B; }

		URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(B.World);
		B.MapActor = B.World->SpawnActor<ARTHexMapActor>();
		if (B.MapActor) { B.MapActor->MapAsset = Arena; }
		B.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

		B.Unit = SpawnInteractionUnit(B.World, 0, URTHeroCatalogLibrary::MakeIvrin(), Partenza);
		B.PC = B.World->SpawnActor<ARTPlayerController>();
		if (!B.Unit) { return B; }

		// L'indice si CERCA: le generiche sono accodate al kit, quindi la posizione dipende dall'eroe.
		for (int32 i = 0; i < B.Unit->NumAbilities(); ++i)
		{
			const URTActionData* A = B.Unit->GetAbility(i);
			if (A && A->Def.ActionId == TEXT("Action.Overwatch")) { B.IdxOverwatch = i; break; }
		}
		return B;
	}
}

/**
 * IL DISARMO RILASCIA LO SLOT MOVIMENTO - `#3417`.
 *
 * 🔑 **E' il cuore del difetto, e sta in una riga che non c'era.** `SelectAbility(INDEX_NONE)` scrive
 * `SelectedAbilityIndex` e **nient'altro**: `PlannedAbilityIndex` restava, `MakePlanFor` continuava ad
 * aggiungere l'azione al piano, e `ReservedProfileForPlan` a rispondere `Withdraw`. Lo slot si spegneva a
 * schermo e il tetto restava per tutto il turno.
 *
 * ⚠️ **Le due asserzioni non sono ridondanti.** Azzerare `PlannedAbilityIndex` e' il meccanismo; il
 * tetto rilasciato e' la **conseguenza** che il giocatore sente. Un'implementazione che azzerasse il campo
 * senza che la catena del tetto lo leggesse passerebbe la prima e cadrebbe sulla seconda.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDisarmReleasesReservedSlotTest,
	"RefactorTactics.PlayerInput.DisarmingReleasesTheReservedSlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDisarmReleasesReservedSlotTest::RunTest(const FString&)
{
	FRTBancoTetto B = AllestisciBancoTetto(FRTCellId(2, -2, 0));
	if (!TestTrue(TEXT("banco allestito, con Action.Overwatch nel kit"), B.Valido()))
	{
		DestroyInteractionWorld(B.World); return false;
	}
	B.PC->SelectActorForTest(B.Unit);

	B.PC->SelectAbilityForCurrentForTest(B.IdxOverwatch);
	if (!TestEqual(TEXT("premessa: armare riserva lo slot al Withdraw"),
			URTMovementProfileLibrary::ReservedProfileForPlan(URTPlanValidationLibrary::MakePlanFor(B.Unit)),
			URTMovementProfileLibrary::ProfileWithdraw))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	B.PC->SelectAbilityForCurrentForTest(INDEX_NONE);

	// 🔴 Il meccanismo.
	TestEqual(TEXT("il disarmo azzera l'azione PIANIFICATA, non solo quella selezionata"),
		B.Unit->GetPlan().PlannedAbilityIndex(), (int32)INDEX_NONE);
	// 🔴 E la conseguenza che il giocatore sente.
	TestTrue(TEXT("e lo slot movimento torna libero: nessun tetto imposto dal piano"),
		URTMovementProfileLibrary::ReservedProfileForPlan(
			URTPlanValidationLibrary::MakePlanFor(B.Unit)).IsNone());

	DestroyInteractionWorld(B.World);
	return true;
}

/**
 * DISARMARE RESTITUISCE I WAYPOINT CHE LA RISERVA AVEVA TOLTO - `#3417`, [D-444].
 *
 * 🔑 **Togliere un tetto e' reversibile, troncare no.** [D-401] ha scelto il troncamento
 * sull'azzeramento - *<<si tiene cio' che il profilo nuovo consente>>* - e [D-444] chiude il verso
 * opposto, che quella decisione non copriva.
 *
 * ⚠️ **Si asserisce anche `PlannedPath`, non solo i waypoint.** Il percorso e' DERIVATO: rimettere i
 * waypoint senza ricalcolarlo lascerebbe due verita' sullo stesso piano, ed e' il difetto che l'anteprima
 * mostrerebbe per prima.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDisarmGivesBackWaypointsTest,
	"RefactorTactics.PlayerInput.DisarmingGivesBackTheTruncatedWaypoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDisarmGivesBackWaypointsTest::RunTest(const FString&)
{
	FRTBancoTetto B = AllestisciBancoTetto(FRTCellId(2, -2, 0));
	if (!TestTrue(TEXT("banco allestito"), B.Valido()))
	{
		DestroyInteractionWorld(B.World); return false;
	}
	B.PC->SelectActorForTest(B.Unit);
	B.PC->HandleClickOnCell(FRTCellId(3, -2, 0));
	B.PC->HandleClickOnCell(FRTCellId(3, -1, 0));
	const TArray<FRTCellId> Dichiarati = B.Unit->GetPlan().PlannedWaypoints();
	if (!TestEqual(TEXT("premessa: due waypoint posati"), Dichiarati.Num(), 2))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	B.PC->SelectAbilityForCurrentForTest(B.IdxOverwatch);
	if (!TestTrue(*FString::Printf(TEXT("premessa: la riserva ha troncato (%d -> %d)"),
			Dichiarati.Num(), B.Unit->GetPlan().PlannedWaypoints().Num()),
			B.Unit->GetPlan().PlannedWaypoints().Num() < Dichiarati.Num()))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	B.PC->SelectAbilityForCurrentForTest(INDEX_NONE);

	TestEqual(TEXT("i waypoint tornano tutti"), B.Unit->GetPlan().PlannedWaypoints().Num(), Dichiarati.Num());
	bool bStessi = B.Unit->GetPlan().PlannedWaypoints().Num() == Dichiarati.Num();
	for (int32 i = 0; bStessi && i < Dichiarati.Num(); ++i)
	{
		bStessi = B.Unit->GetPlan().PlannedWaypoints()[i] == Dichiarati[i];
	}
	TestTrue(TEXT("e sono gli STESSI, nello stesso ordine"), bStessi);
	// ⚠️ Il derivato: senza `RebuildPlannedPath` questa riga cade mentre quelle sopra passano.
	TestTrue(TEXT("e il percorso derivato e' stato ricalcolato: arriva all'ultimo waypoint"),
		B.Unit->GetPlan().PlannedPath().Num() > 0 && B.Unit->GetPlan().PlannedPath().Last() == Dichiarati.Last());

	DestroyInteractionWorld(B.World);
	return true;
}

/**
 * ANNULLARE LO SNEAK RESTITUISCE I SUOI - `#3417`, [D-444], e la simmetria che [D-401] pretende.
 *
 * 🔴 **Questo test esiste per una ragione di coerenza, non per un difetto osservato a parte.** [D-401]
 * dice che il troncamento rende i due inneschi *<<una sola regola>>*: un ripristino sul solo innesco
 * imposto ricreerebbe l'asimmetria che quella decisione ha rimosso - disarmando l'`Overwatch` il percorso
 * torna, annullando lo `Sneak` no.
 *
 * ⚠️ **Il ripristino sta PRIMA della lambda che rivalida**, e questo test lo coprirebbe anche se il
 * piano fosse stato troncato a zero: il primo ramo di quella lambda esce sui waypoint vuoti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUndeclareSneakGivesBackWaypointsTest,
	"RefactorTactics.PlayerInput.UndeclaringSneakGivesBackTheTruncatedWaypoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUndeclareSneakGivesBackWaypointsTest::RunTest(const FString&)
{
	FRTBancoTetto B = AllestisciBancoTetto(FRTCellId(-4, 0, 0));
	if (!TestTrue(TEXT("banco allestito"), B.Valido()))
	{
		DestroyInteractionWorld(B.World); return false;
	}
	B.PC->SelectActorForTest(B.Unit);

	// ⚠️ **La lunghezza del piano si RICAVA dal movimento base, non si cabla.** Il tetto dello `Sneak`
	// e' `Base/2`, quindi su un eroe da 6 un piano di tre passi ci **sta** e non viene troncato: la prima
	// stesura di questi due test e' fallita esattamente cosi' (`3 passi/3`, premessa non soddisfatta).
	// E' la stessa trappola che il commento di `DeclaringSneakTruncatesToHalf` dichiara, con lo stesso
	// rimedio: la riga retta lunga `Base`.
	const int32 Base = B.Unit->GetEffectiveMoveRange();
	for (int32 Passo = 1; Passo <= Base; ++Passo)
	{
		B.PC->HandleClickOnCell(FRTCellId(-4 + Passo, 0, 0));
	}
	const TArray<FRTCellId> Dichiarati = B.Unit->GetPlan().PlannedWaypoints();
	const FRTMovementProfile Sneak =
		URTMovementProfileLibrary::FindProfile(URTMovementProfileLibrary::ProfileSneak);
	const int32 PassiSneak = Sneak.ResolveStepBudget(Base);
	if (!TestTrue(*FString::Printf(
			TEXT("premessa: il piano (%d waypoint) eccede il tetto dello Sneak (%d passi)"),
			Dichiarati.Num(), PassiSneak), Dichiarati.Num() > PassiSneak))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	B.PC->ToggleSneakForTest();
	if (!TestTrue(*FString::Printf(TEXT("premessa: lo Sneak ha troncato (%d -> %d)"),
			Dichiarati.Num(), B.Unit->GetPlan().PlannedWaypoints().Num()),
			B.Unit->GetPlan().PlannedWaypoints().Num() < Dichiarati.Num()))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	B.PC->ToggleSneakForTest();

	TestTrue(TEXT("annullando lo Sneak la dichiarazione si spegne"),
		B.Unit->GetPlan().PlannedMovementProfileId().IsNone());
	TestEqual(TEXT("e i waypoint che aveva tolto tornano"),
		B.Unit->GetPlan().PlannedWaypoints().Num(), Dichiarati.Num());

	DestroyInteractionWorld(B.World);
	return true;
}

/**
 * UN PIANO MODIFICATO A MANO NON VIENE RESUSCITATO - `#3417`, [D-444], guardia.
 *
 * 🔴 **E' la guardia che rende il ripristino accettabile.** Se dopo il troncamento il giocatore annulla
 * un waypoint da se', restituirgli il percorso INTERO annullerebbe il suo annullamento - cioe' il
 * ripristino diventerebbe un modo di perdere lavoro invece di recuperarlo.
 *
 * ⚠️ **Il confronto e' sull'uguaglianza esatta e non su <<e' un prefisso>>**: un prefisso accetterebbe
 * sia il waypoint annullato sia uno aggiunto, cioe' proprio i due casi da escludere.
 *
 * ⚠️ **Il waypoint si toglie con `Pop()` e non col gesto `Back`**, di proposito: `ResolveBack` risolve
 * la sua cascata sul contesto del puntatore, e con un'azione armata potrebbe restituire `Declaration`
 * invece di `Waypoint`. Cio' che va misurato qui e' la guardia, non quale passo la cascata scelga.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEditedPlanIsNotResurrectedTest,
	"RefactorTactics.PlayerInput.AnEditedPlanIsNotResurrected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEditedPlanIsNotResurrectedTest::RunTest(const FString&)
{
	FRTBancoTetto B = AllestisciBancoTetto(FRTCellId(2, -2, 0));
	if (!TestTrue(TEXT("banco allestito"), B.Valido()))
	{
		DestroyInteractionWorld(B.World); return false;
	}
	B.PC->SelectActorForTest(B.Unit);
	B.PC->HandleClickOnCell(FRTCellId(3, -2, 0));
	B.PC->HandleClickOnCell(FRTCellId(3, -1, 0));
	if (!TestEqual(TEXT("premessa: due waypoint posati"), B.Unit->GetPlan().PlannedWaypoints().Num(), 2))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	B.PC->SelectAbilityForCurrentForTest(B.IdxOverwatch);
	const int32 DopoIlTaglio = B.Unit->GetPlan().PlannedWaypoints().Num();
	if (!TestTrue(TEXT("premessa: la riserva ha troncato e qualcosa e' rimasto"),
			DopoIlTaglio > 0 && DopoIlTaglio < 2))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	// Il giocatore tocca il piano dopo il troncamento.
	B.Unit->PopPlannedWaypoint();
	const int32 Modificato = B.Unit->GetPlan().PlannedWaypoints().Num();

	B.PC->SelectAbilityForCurrentForTest(INDEX_NONE);

	TestEqual(TEXT("il piano modificato a mano resta come il giocatore l'ha lasciato"),
		B.Unit->GetPlan().PlannedWaypoints().Num(), Modificato);
	// ⚠️ E il tetto si rilascia comunque: la guardia vale sul RIPRISTINO, non sul disarmo.
	TestTrue(TEXT("ma lo slot movimento si libera comunque"),
		URTMovementProfileLibrary::ReservedProfileForPlan(
			URTPlanValidationLibrary::MakePlanFor(B.Unit)).IsNone());

	DestroyInteractionWorld(B.World);
	return true;
}

/**
 * UN INNESCO NON RESTITUISCE CIO' CHE L'ALTRO AVEVA TOLTO - `#3417`, [D-444], guardia.
 *
 * 🔴 **Senza la chiave del tetto questo scenario produce un piano ILLEGALE.** `Sneak` dichiarato
 * (x0,5) e `Overwatch` armato (x0,25) sono due troncamenti. Se il disarmo dell'`Overwatch` restituisse la
 * memoria dell'ALTRO, il piano tornerebbe alla lunghezza pre-`Sneak` **mentre lo `Sneak` e' ancora
 * dichiarato** - oltre il tetto vigente. La chiave e' cio' che lo impedisce.
 *
 * ⚠️ **Si misura la MEMORIA su `ARTUnit`, non i due gesti sul controller, e la ragione e' misurata.**
 * Nell'arena di prova la catena retta non e' percorribile oltre il primo passo: lo `Sneak` cade nel ramo
 * duro di `TruncateWaypointsToBudget` e tronca **a un waypoint** qualunque sia la lunghezza di partenza -
 * *<<3 waypoint scartati (percorso non piu' percorribile: 0 passi/3), ne restano 1>>*. A quel punto il
 * `Withdraw`, che ha lo stesso budget di un passo, riporta *<<0 waypoint scartati>>*: il **secondo**
 * taglio non avviene, la seconda memoria non nasce, e un test passato da la' sarebbe verde per assenza
 * dello scenario invece che per la chiave.
 *
 * 🔑 **Le due guardie che la chiave deve dare sono asserite separatamente.** Non restituire col tetto
 * sbagliato e **non consumare** quella memoria sono cose diverse: un'implementazione che rispondesse
 * `false` e poi scordasse passerebbe la prima e perderebbe il ripristino legittimo del gesto dopo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOneCeilingDoesNotGiveBackTheOthersTest,
	"RefactorTactics.PlayerInput.OneCeilingDoesNotGiveBackWhatTheOtherTook",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOneCeilingDoesNotGiveBackTheOthersTest::RunTest(const FString&)
{
	FRTBancoTetto B = AllestisciBancoTetto(FRTCellId(-4, 0, 0));
	if (!TestTrue(TEXT("banco allestito"), B.Valido()))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	const TArray<FRTCellId> PrimaDelloSneak = {
		FRTCellId(-3, 0, 0), FRTCellId(-2, 0, 0), FRTCellId(-1, 0, 0), FRTCellId(0, 0, 0) };
	const TArray<FRTCellId> DopoLoSneak = { FRTCellId(-3, 0, 0), FRTCellId(-2, 0, 0) };

	// Lo `Sneak` tronca: il piano passa da quattro waypoint a due, e la memoria porta i quattro.
	FRTUnitPlanTestAccess::SetWaypoints(*B.Unit, DopoLoSneak);
	B.Unit->RicordaTroncamentoDelTetto(URTMovementProfileLibrary::ProfileSneak, PrimaDelloSneak);

	// Poi la riserva tronca ancora: da due a uno, e la memoria porta i DUE - cioe' il piano gia' sgusciato.
	const TArray<FRTCellId> DopoLaRiserva = { FRTCellId(-3, 0, 0) };
	FRTUnitPlanTestAccess::SetWaypoints(*B.Unit, DopoLaRiserva);
	B.Unit->RicordaTroncamentoDelTetto(URTMovementProfileLibrary::ProfileWithdraw, DopoLoSneak);

	// 🔴 Disarmare restituisce il piano dello `Sneak`, non quello di partenza.
	TestTrue(TEXT("disarmando la riserva il ripristino avviene"),
		B.Unit->RipristinaWaypointsDelTetto(URTMovementProfileLibrary::ProfileWithdraw));
	TestEqual(TEXT("e torna il piano che lo Sneak aveva lasciato, non quello di partenza"),
		B.Unit->GetPlan().PlannedWaypoints().Num(), DopoLoSneak.Num());

	DestroyInteractionWorld(B.World);
	return true;
}

/**
 * LA MEMORIA DI UN TETTO NON SI CONSUMA CHIEDENDONE UN ALTRO - `#3417`, [D-444], guardia.
 *
 * 🔴 **E' la meta' che un `return false` da solo non copre.** Se `RipristinaWaypointsDelTetto` scordasse
 * la memoria anche quando la chiave non corrisponde, disarmare l'`Overwatch` cancellerebbe il ripristino
 * che spetta a chi annulla lo `Sneak`: il primo gesto non restituirebbe niente - corretto - e il secondo
 * nemmeno, che non lo e'.
 *
 * ⚠️ **L'ordine e' quello che fa danno**: si chiede col tetto SBAGLIATO per primo, e solo dopo con
 * quello giusto. Chiedendo prima col giusto il test passerebbe anche sull'implementazione difettosa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWrongCeilingDoesNotEatTheMemoryTest,
	"RefactorTactics.PlayerInput.AskingWithTheWrongCeilingDoesNotEatTheMemory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWrongCeilingDoesNotEatTheMemoryTest::RunTest(const FString&)
{
	FRTBancoTetto B = AllestisciBancoTetto(FRTCellId(-4, 0, 0));
	if (!TestTrue(TEXT("banco allestito"), B.Valido()))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	const TArray<FRTCellId> Intero = {
		FRTCellId(-3, 0, 0), FRTCellId(-2, 0, 0), FRTCellId(-1, 0, 0) };
	FRTUnitPlanTestAccess::SetWaypoints(*B.Unit, { FRTCellId(-3, 0, 0) });
	B.Unit->RicordaTroncamentoDelTetto(URTMovementProfileLibrary::ProfileSneak, Intero);

	// Il tetto SBAGLIATO per primo: non restituisce...
	TestFalse(TEXT("il tetto sbagliato non restituisce niente"),
		B.Unit->RipristinaWaypointsDelTetto(URTMovementProfileLibrary::ProfileWithdraw));
	TestEqual(TEXT("e non tocca il piano"), B.Unit->GetPlan().PlannedWaypoints().Num(), 1);

	// 🔴 ...e non ha nemmeno consumato la memoria: quello giusto funziona ancora.
	TestTrue(TEXT("il tetto giusto, chiesto DOPO, restituisce ancora"),
		B.Unit->RipristinaWaypointsDelTetto(URTMovementProfileLibrary::ProfileSneak));
	TestEqual(TEXT("e il piano torna intero"), B.Unit->GetPlan().PlannedWaypoints().Num(), Intero.Num());

	// ⚠️ La memoria si consuma su un ripristino avvenuto: una seconda richiesta non deve ridarla.
	TestFalse(TEXT("e la memoria si e' consumata: la seconda richiesta non restituisce"),
		B.Unit->RipristinaWaypointsDelTetto(URTMovementProfileLibrary::ProfileSneak));

	DestroyInteractionWorld(B.World);
	return true;
}

/**
 * IL BACK SU UN'AZIONE GIA' NEL PIANO E' UN DISARMO - `#3501`, [D-444], [D-458].
 *
 * 🔴 **E' il gemello di `DisarmingReleasesTheReservedSlot`, e il difetto e' lo stesso in un'altra porta.** Un
 * supporto su se stessi entra nel piano all'armamento; il Back a livello `Declaration` chiamava il solo
 * `SelectAbility(INDEX_NONE)`, cioe' la riga che [D-444] punto (1) nomina come difetto. Lo slot si spegneva, il
 * tetto `Withdraw` restava, e i waypoint troncati non tornavano. Il Back e' anche `Annulla` ([D-458]).
 *
 * ⚠️ **Tre conseguenze, tre asserzioni**: il piano senza l'azione (il meccanismo), il tetto rilasciato (cio' che
 * il giocatore sente al prossimo waypoint), i waypoint restituiti (cio' che [D-444] promette al rilascio).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBackOnAPlannedActionDisarmsTest,
	"RefactorTactics.PlayerInput.BackOnAPlannedActionReleasesTheReservedSlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBackOnAPlannedActionDisarmsTest::RunTest(const FString&)
{
	FRTBancoTetto B = AllestisciBancoTetto(FRTCellId(2, -2, 0));
	if (!TestTrue(TEXT("banco allestito, con Action.Overwatch nel kit"), B.Valido()))
	{
		DestroyInteractionWorld(B.World); return false;
	}
	B.PC->SelectActorForTest(B.Unit);
	B.PC->HandleClickOnCell(FRTCellId(3, -2, 0));
	B.PC->HandleClickOnCell(FRTCellId(3, -1, 0));
	const TArray<FRTCellId> Dichiarati = B.Unit->GetPlan().PlannedWaypoints();

	B.PC->SelectAbilityForCurrentForTest(B.IdxOverwatch);
	if (!TestEqual(TEXT("premessa: l'Overwatch e' nel piano, non solo selezionato"),
			B.Unit->GetPlan().PlannedAbilityIndex(), B.IdxOverwatch)
		|| !TestTrue(*FString::Printf(TEXT("premessa: la riserva ha troncato (%d -> %d)"),
			Dichiarati.Num(), B.Unit->GetPlan().PlannedWaypoints().Num()),
			B.Unit->GetPlan().PlannedWaypoints().Num() < Dichiarati.Num()))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	TestEqual(TEXT("il Back esce dalla dichiarazione"), B.PC->ApplyBack(), ERTPointerBackStep::Declaration);

	// 🔴 Il meccanismo.
	TestEqual(TEXT("il Back toglie l'azione dal PIANO, non solo dalla selezione"),
		B.Unit->GetPlan().PlannedAbilityIndex(), (int32)INDEX_NONE);
	TestEqual(TEXT("e dalla selezione"), B.Unit->SelectedAbilityIndex, (int32)INDEX_NONE);
	// 🔴 La conseguenza che il giocatore sente.
	TestTrue(TEXT("e lo slot movimento torna libero: nessun tetto imposto dal piano"),
		URTMovementProfileLibrary::ReservedProfileForPlan(
			URTPlanValidationLibrary::MakePlanFor(B.Unit)).IsNone());
	// 🔴 E cio' che [D-444] promette al rilascio.
	TestEqual(TEXT("i waypoint troncati tornano tutti"), B.Unit->GetPlan().PlannedWaypoints().Num(), Dichiarati.Num());
	TestTrue(TEXT("e il percorso derivato arriva all'ultimo"),
		B.Unit->GetPlan().PlannedPath().Num() > 0 && Dichiarati.Num() > 0 && B.Unit->GetPlan().PlannedPath().Last() == Dichiarati.Last());

	DestroyInteractionWorld(B.World);
	return true;
}

/**
 * IL BACK DOPO UN BERSAGLIO DICHIARATO TOGLIE L'ATTACCO DAL PIANO - `#3501`, decisione d'autore.
 *
 * 🔑 **Il Back disfa l'ultimo gesto**, e dopo il click sul bersaglio l'ultimo gesto e' la dichiarazione. Fino a
 * `#3501` il Back usciva dal targeting e lasciava l'attacco nel piano: lo slot passava da armato a pianificato,
 * e il solo modo di togliere l'attacco era il secondo click sullo slot.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBackAfterADeclaredTargetDisarmsTest,
	"RefactorTactics.PlayerInput.BackAfterADeclaredTargetRemovesTheAttack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBackAfterADeclaredTargetDisarmsTest::RunTest(const FString&)
{
	FRTBancoTetto B = AllestisciBancoTetto(FRTCellId(2, -2, 0));
	if (!TestTrue(TEXT("banco allestito"), B.Valido()))
	{
		DestroyInteractionWorld(B.World); return false;
	}
	// Un nemico accanto, e la prima azione del kit che punta un'UNITA': cercata, non scritta.
	ARTUnit* Nemico = SpawnInteractionUnit(B.World, 1, URTHeroCatalogLibrary::MakeGadget(), FRTCellId(3, -2, 0));
	B.PC->SelectActorForTest(B.Unit);
	int32 Armata = INDEX_NONE;
	for (int32 i = 0; Nemico && i < B.Unit->NumAbilities() && Armata == INDEX_NONE; ++i)
	{
		const URTActionData* A = B.Unit->GetAbility(i);
		if (!A || A->bSelfTarget || !A->Def.ReservesMovementProfileId.IsNone() || A->Def.Slot != ERTActionSlot::Main)
		{
			continue;
		}
		B.PC->SelectAbilityForCurrentForTest(i);
		if (B.PC->GetPointerTargetKind() != ERTPointerTargetKind::Unit) { continue; }
		B.PC->HandleClickOnUnitForTest(Nemico);
		if (B.Unit->GetPlan().PlannedAttackTarget() == Nemico) { Armata = i; }
	}
	if (!TestNotEqual(TEXT("premessa: un'azione del kit ha preso il nemico come bersaglio"), Armata, (int32)INDEX_NONE)
		|| !TestEqual(TEXT("premessa: l'attacco e' nel piano"), B.Unit->GetPlan().PlannedAbilityIndex(), Armata)
		|| !TestEqual(TEXT("premessa: e resta selezionato dopo il bersaglio"), B.Unit->SelectedAbilityIndex, Armata))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	// Un lato dichiarato, come lo lascerebbe un'azione su bordo: il disarmo deve spegnere anche quello.
	FRTUnitPlanTestAccess::SetHasCoverEdge(*B.Unit, true);

	TestEqual(TEXT("il Back esce dalla dichiarazione"), B.PC->ApplyBack(), ERTPointerBackStep::Declaration);

	TestEqual(TEXT("l'attacco esce dal piano"), B.Unit->GetPlan().PlannedAbilityIndex(), (int32)INDEX_NONE);
	TestFalse(TEXT("e il lato dichiarato e' spento"), B.Unit->GetPlan().bHasPlannedCoverEdge());
	TestEqual(TEXT("e dalla selezione"), B.Unit->SelectedAbilityIndex, (int32)INDEX_NONE);
	TestTrue(TEXT("e il bersaglio dichiarato e' spento"), B.Unit->GetPlan().PlannedAttackTarget() == nullptr);

	DestroyInteractionWorld(B.World);
	return true;
}

/**
 * UN WAYPOINT POSATO DOPO L'AZIONE SI TOGLIE PRIMA DI DISARMARLA - `#3501`, [D-461] punto 3.
 *
 * 🔑 **Il Back disfa l'ultimo gesto.** Un supporto su se stessi resta armato dopo essere entrato nel piano, quindi
 * il giocatore puo' posare waypoint DOPO. Con l'ordine del Back (`Declaration` prima di `Waypoint`) il primo Back
 * avrebbe tolto l'`Overwatch` invece del waypoint appena posato: trovato in revisione, deciso dall'autore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBackRemovesTheLaterWaypointFirstTest,
	"RefactorTactics.PlayerInput.BackRemovesAWaypointPlacedAfterTheActionFirst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBackRemovesTheLaterWaypointFirstTest::RunTest(const FString&)
{
	FRTBancoTetto B = AllestisciBancoTetto(FRTCellId(2, -2, 0));
	if (!TestTrue(TEXT("banco allestito"), B.Valido()))
	{
		DestroyInteractionWorld(B.World); return false;
	}
	B.PC->SelectActorForTest(B.Unit);
	B.PC->SelectAbilityForCurrentForTest(B.IdxOverwatch);
	B.PC->HandleClickOnCell(FRTCellId(3, -2, 0));
	if (!TestEqual(TEXT("premessa: l'Overwatch e' nel piano"), B.Unit->GetPlan().PlannedAbilityIndex(), B.IdxOverwatch)
		|| !TestEqual(TEXT("premessa: un waypoint posato DOPO, dentro il tetto Withdraw"), B.Unit->GetPlan().PlannedWaypoints().Num(), 1))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	TestEqual(TEXT("il primo Back toglie il waypoint"), B.PC->ApplyBack(), ERTPointerBackStep::Waypoint);
	TestEqual(TEXT("e il waypoint non c'e' piu'"), B.Unit->GetPlan().PlannedWaypoints().Num(), 0);
	TestEqual(TEXT("ma l'Overwatch resta nel piano"), B.Unit->GetPlan().PlannedAbilityIndex(), B.IdxOverwatch);
	TestEqual(TEXT("e resta armato"), B.Unit->SelectedAbilityIndex, B.IdxOverwatch);

	TestEqual(TEXT("il secondo Back esce dalla dichiarazione"), B.PC->ApplyBack(), ERTPointerBackStep::Declaration);
	TestEqual(TEXT("e disarma l'Overwatch"), B.Unit->GetPlan().PlannedAbilityIndex(), (int32)INDEX_NONE);
	TestTrue(TEXT("e il tetto cade"),
		URTMovementProfileLibrary::ReservedProfileForPlan(
			URTPlanValidationLibrary::MakePlanFor(B.Unit)).IsNone());

	DestroyInteractionWorld(B.World);
	return true;
}

// =====================================================================================================
// `#3927` — il Back su una REAZIONE armata la toglie dal piano ([D-557], seguito di [D-555]).
// =====================================================================================================

namespace
{
	/**
	 * Una reazione nel kit di `Unit`, in una posizione che NON sia quella indicata (l'`Overwatch`): SOSTITUISCE lo slot
	 * invece di accodare, perche' un'abilita' appesa sta a un indice che `AbilityCooldowns` non copre. Torna l'indice.
	 */
	int32 AggiungiReazione(ARTUnit* Unit, int32 Escluso)
	{
		for (int32 Posizione = 0; Posizione < Unit->NumAbilities(); ++Posizione)
		{
			if (Posizione != Escluso)
			{
				return RTAbilityFixtures::AddCoreAbilityInSlot(Unit, TEXT("Action.Counter"), Posizione);
			}
		}
		return INDEX_NONE;
	}

	/**
	 * Una `Action.Guard` nel kit di `Unit`, in una posizione che non sia nessuna delle due escluse (l'`Overwatch` e la
	 * reazione): SOSTITUISCE lo slot. Una principale su se stessi che NON riserva il movimento, a differenza dell'`Overwatch`:
	 * niente troncamento a sporcare i conteggi, e un segno che vale esattamente quanti waypoint c'erano all'armo.
	 */
	int32 AggiungiGuardia(ARTUnit* Unit, int32 Escluso1, int32 Escluso2)
	{
		for (int32 Posizione = 0; Posizione < Unit->NumAbilities(); ++Posizione)
		{
			if (Posizione == Escluso1 || Posizione == Escluso2)
			{
				continue;
			}
			const int32 Indice = RTAbilityFixtures::AddCoreAbilityInSlot(Unit, TEXT("Action.Guard"), Posizione);
			if (Indice != INDEX_NONE)
			{
				return Indice;
			}
		}
		return INDEX_NONE;
	}

	/** Le premesse comuni: la reazione e' entrata nel kit, e' davvero una reazione, ed e' pronta. */
	bool ReazioneValida(FAutomationTestBase& Test, const FRTBancoTetto& B, int32 Reazione)
	{
		const URTActionData* Counter = B.Unit->GetAbility(Reazione);
		return Test.TestTrue(TEXT("premessa: la reazione e' entrata nel kit"), Reazione != INDEX_NONE && Counter != nullptr)
			&& Test.TestEqual(TEXT("premessa: e' davvero uno slot di reazione"),
				static_cast<int32>(Counter->Def.Slot), static_cast<int32>(ERTActionSlot::Reaction))
			&& Test.TestTrue(TEXT("premessa: la reazione e' pronta"), B.Unit->CanUseAbility(Reazione));
	}
}

/**
 * 🔴 **IL BACK SU UNA REAZIONE ARMATA LA TOGLIE DAL PIANO** (`#3927`, decisione d'autore).
 *
 * Una reazione scrive il piano nel momento in cui la si arma, a differenza di un attacco in targeting, che non scrive
 * niente finche' non si clicca il bersaglio: quindi l'ultimo gesto ha scritto il piano, e il Back che lo disfa lo toglie.
 * ⏱️ *Fino a `#3927` `ApplyBack` guardava `PlannedAbilityIndex`, che una reazione non scrive*: il Back spegneva lo slot e
 * lasciava la reazione nel piano, il dock la mostrava come pianificata e a turno risolto scattava.
 *
 * ⚠️ **L'oracolo e' il piano, non lo slot**: lo slot si spegneva gia'. La condizione dichiarata esce con la reazione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBackOnAnArmedReactionRemovesItTest,
	"RefactorTactics.PlayerInput.BackOnAnArmedReactionRemovesItFromThePlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBackOnAnArmedReactionRemovesItTest::RunTest(const FString&)
{
	FRTBancoTetto B = AllestisciBancoTetto(FRTCellId(2, -2, 0));
	if (!TestTrue(TEXT("banco allestito, con Action.Overwatch nel kit"), B.Valido()))
	{
		DestroyInteractionWorld(B.World); return false;
	}
	const int32 Reazione = AggiungiReazione(B.Unit, B.IdxOverwatch);
	if (!ReazioneValida(*this, B, Reazione))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	B.PC->SelectActorForTest(B.Unit);
	B.PC->SelectAbilityForCurrentForTest(Reazione);
	// L'id e' quello che il validator ammette: la condizione si dichiara DOPO l'armo, che le da' una reazione a cui applicarsi.
	const bool bCondizione = B.Unit->SetPlannedReactionCondition(
		FRTDeclaredCondition(TEXT("TargetHealthAtOrBelowPercent"), 50));
	if (!TestEqual(TEXT("premessa: la reazione e' armata"), B.Unit->SelectedAbilityIndex, Reazione)
		|| !TestEqual(TEXT("premessa: ed e' nel piano"), B.Unit->GetPlan().PlannedReactionAbility(), Reazione)
		|| !TestEqual(TEXT("premessa: la principale NON e' nel piano (e' la ragione del difetto)"),
			B.Unit->GetPlan().PlannedAbilityIndex(), (int32)INDEX_NONE)
		|| !TestTrue(TEXT("premessa: la condizione dichiarata entra nel piano"),
			bCondizione && B.Unit->GetPlan().PlannedReactionCondition().IsDeclared()))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	TestEqual(TEXT("il Back esce dalla dichiarazione"), B.PC->ApplyBack(), ERTPointerBackStep::Declaration);
	TestEqual(TEXT("lo slot si spegne"), B.Unit->SelectedAbilityIndex, (int32)INDEX_NONE);
	TestEqual(TEXT("e il Back toglie la reazione dal PIANO, non solo dalla selezione"),
		B.Unit->GetPlan().PlannedReactionAbility(), (int32)INDEX_NONE);
	TestFalse(TEXT("con lei esce la condizione dichiarata"), B.Unit->GetPlan().PlannedReactionCondition().IsDeclared());

	DestroyInteractionWorld(B.World);
	return true;
}

/**
 * 🔑 **IL BACK TOGLIE PRIMA I WAYPOINT POSATI DOPO LA REAZIONE** ([D-461] punto 3, esteso alle reazioni).
 *
 * Una reazione non riserva il movimento, quindi dopo averla armata il giocatore puo' posare waypoint: con una reazione
 * armata il contesto del puntatore e' `Targeting`, ma per un'azione senza bersaglio un click sulla mappa ricade sul
 * waypoint. Il Back disfa l'ultimo gesto: toglie quelli, uno per volta, e disarma la reazione al Back in cui non ne restano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBackTakesTheWaypointsAfterAReactionFirstTest,
	"RefactorTactics.PlayerInput.BackTakesTheWaypointsPlacedAfterAReactionFirst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBackTakesTheWaypointsAfterAReactionFirstTest::RunTest(const FString&)
{
	FRTBancoTetto B = AllestisciBancoTetto(FRTCellId(2, -2, 0));
	if (!TestTrue(TEXT("banco allestito, con Action.Overwatch nel kit"), B.Valido()))
	{
		DestroyInteractionWorld(B.World); return false;
	}
	const int32 Reazione = AggiungiReazione(B.Unit, B.IdxOverwatch);
	if (!ReazioneValida(*this, B, Reazione))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	B.PC->SelectActorForTest(B.Unit);
	B.PC->HandleClickOnCell(FRTCellId(3, -2, 0));      // un waypoint PRIMA dell'armo
	B.PC->SelectAbilityForCurrentForTest(Reazione);
	B.PC->HandleClickOnCell(FRTCellId(3, -1, 0));      // e uno DOPO
	if (!TestEqual(TEXT("premessa: la reazione e' nel piano"), B.Unit->GetPlan().PlannedReactionAbility(), Reazione)
		|| !TestEqual(TEXT("premessa: i due waypoint ci sono (il click a reazione armata ricade sul waypoint)"),
			B.Unit->GetPlan().PlannedWaypoints().Num(), 2))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	TestEqual(TEXT("il primo Back toglie il waypoint posato dopo"), B.PC->ApplyBack(), ERTPointerBackStep::Waypoint);
	TestEqual(TEXT("e resta quello di prima"), B.Unit->GetPlan().PlannedWaypoints().Num(), 1);
	TestEqual(TEXT("la reazione resta nel piano"), B.Unit->GetPlan().PlannedReactionAbility(), Reazione);
	TestEqual(TEXT("e resta armata"), B.Unit->SelectedAbilityIndex, Reazione);

	TestEqual(TEXT("il secondo Back esce dalla dichiarazione"), B.PC->ApplyBack(), ERTPointerBackStep::Declaration);
	TestEqual(TEXT("e toglie la reazione dal piano"), B.Unit->GetPlan().PlannedReactionAbility(), (int32)INDEX_NONE);
	TestEqual(TEXT("senza toccare il waypoint di prima"), B.Unit->GetPlan().PlannedWaypoints().Num(), 1);

	DestroyInteractionWorld(B.World);
	return true;
}

/**
 * 🔑 **IL BACK SU UNA REAZIONE NON TOCCA UN'AZIONE PRINCIPALE PIANIFICATA** ([D-555] (2), [D-557]).
 *
 * `Reaction` ha il proprio slot e il proprio campo: l'`Overwatch` pianificato impone il tetto `Withdraw`, ha troncato
 * i waypoint e resta nel piano quando il giocatore arma e poi annulla una reazione. Il caso piu' forte e' proprio questo,
 * non una guardia qualunque: se il disarmo della reazione scivolasse nel ramo della principale, rilascerebbe il tetto e
 * restituirebbe i waypoint ([D-444]) di un'azione che nessuno ha annullato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBackOnAReactionKeepsAPlannedOverwatchTest,
	"RefactorTactics.PlayerInput.BackOnAReactionKeepsAPlannedOverwatchAndItsCap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBackOnAReactionKeepsAPlannedOverwatchTest::RunTest(const FString&)
{
	FRTBancoTetto B = AllestisciBancoTetto(FRTCellId(2, -2, 0));
	if (!TestTrue(TEXT("banco allestito, con Action.Overwatch nel kit"), B.Valido()))
	{
		DestroyInteractionWorld(B.World); return false;
	}
	const int32 Reazione = AggiungiReazione(B.Unit, B.IdxOverwatch);
	if (!ReazioneValida(*this, B, Reazione))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	B.PC->SelectActorForTest(B.Unit);
	B.PC->HandleClickOnCell(FRTCellId(3, -2, 0));
	B.PC->HandleClickOnCell(FRTCellId(3, -1, 0));
	const int32 Dichiarati = B.Unit->GetPlan().PlannedWaypoints().Num();
	B.PC->SelectAbilityForCurrentForTest(B.IdxOverwatch);
	const int32 Troncati = B.Unit->GetPlan().PlannedWaypoints().Num();
	B.PC->SelectAbilityForCurrentForTest(Reazione);
	if (!TestEqual(TEXT("premessa: l'Overwatch e' nel piano"), B.Unit->GetPlan().PlannedAbilityIndex(), B.IdxOverwatch)
		|| !TestTrue(*FString::Printf(TEXT("premessa: la riserva ha troncato (%d -> %d)"), Dichiarati, Troncati),
			Troncati < Dichiarati)
		|| !TestEqual(TEXT("premessa: ci sono due cose nel piano, la reazione e' quella armata"),
			B.Unit->SelectedAbilityIndex, Reazione)
		|| !TestEqual(TEXT("premessa: la reazione e' nel piano"), B.Unit->GetPlan().PlannedReactionAbility(), Reazione))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	TestEqual(TEXT("il Back esce dalla dichiarazione"), B.PC->ApplyBack(), ERTPointerBackStep::Declaration);
	TestEqual(TEXT("toglie la reazione dal piano"), B.Unit->GetPlan().PlannedReactionAbility(), (int32)INDEX_NONE);
	TestEqual(TEXT("e l'Overwatch RESTA nel piano"), B.Unit->GetPlan().PlannedAbilityIndex(), B.IdxOverwatch);
	TestFalse(TEXT("col suo tetto di movimento ancora imposto"),
		URTMovementProfileLibrary::ReservedProfileForPlan(URTPlanValidationLibrary::MakePlanFor(B.Unit)).IsNone());
	TestEqual(TEXT("e i waypoint troncati NON tornano"), B.Unit->GetPlan().PlannedWaypoints().Num(), Troncati);

	DestroyInteractionWorld(B.World);
	return true;
}

/**
 * 🔑 **L'ALTRO VERSO: il Back su un'azione principale non toglie una reazione indipendente** ([D-555] (2)).
 *
 * Controllo, verde prima e dopo: armata la reazione e poi l'`Overwatch`, lo slot acceso e' l'`Overwatch` e il Back lo
 * disarma, con il suo tetto, lasciando la reazione.
 *
 * ⚠️ **E' un controllo del COMPORTAMENTO, non il pinning dei predicati**, e la distinzione conta (dalla review di `#3933`):
 * qui la reazione si salva perche' il ramo di `DisarmPlannedAction` per le reazioni NON scatta
 * (`SelectedAbilityIndex != PlannedReactionAbility`), e il Back parte a zero waypoint con i due segni a zero. Un
 * `bReazioneNelPiano` che contasse ogni reazione nel piano, o che unisse i due predicati, lascerebbe questo test verde:
 * a ucciderlo sono `BackUsesTheWaypointMarkOfTheActionBeingDisarmed` e `BackUsesTheReactionMarkWithAMainActionAlreadyPlanned`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBackOnAMainActionKeepsAnIndependentReactionTest,
	"RefactorTactics.PlayerInput.BackOnAMainActionKeepsAnIndependentReaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBackOnAMainActionKeepsAnIndependentReactionTest::RunTest(const FString&)
{
	FRTBancoTetto B = AllestisciBancoTetto(FRTCellId(2, -2, 0));
	if (!TestTrue(TEXT("banco allestito, con Action.Overwatch nel kit"), B.Valido()))
	{
		DestroyInteractionWorld(B.World); return false;
	}
	const int32 Reazione = AggiungiReazione(B.Unit, B.IdxOverwatch);
	if (!ReazioneValida(*this, B, Reazione))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	B.PC->SelectActorForTest(B.Unit);
	B.PC->SelectAbilityForCurrentForTest(Reazione);
	B.PC->SelectAbilityForCurrentForTest(B.IdxOverwatch);
	if (!TestEqual(TEXT("premessa: lo slot acceso e' l'Overwatch"), B.Unit->SelectedAbilityIndex, B.IdxOverwatch)
		|| !TestEqual(TEXT("premessa: l'Overwatch e' nel piano"), B.Unit->GetPlan().PlannedAbilityIndex(), B.IdxOverwatch)
		|| !TestEqual(TEXT("premessa: la reazione e' ancora nel piano"), B.Unit->GetPlan().PlannedReactionAbility(), Reazione))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	TestEqual(TEXT("il Back esce dalla dichiarazione"), B.PC->ApplyBack(), ERTPointerBackStep::Declaration);
	TestEqual(TEXT("disarma l'Overwatch"), B.Unit->GetPlan().PlannedAbilityIndex(), (int32)INDEX_NONE);
	TestTrue(TEXT("e il suo tetto cade"),
		URTMovementProfileLibrary::ReservedProfileForPlan(URTPlanValidationLibrary::MakePlanFor(B.Unit)).IsNone());
	TestEqual(TEXT("ma la reazione indipendente RESTA nel piano"), B.Unit->GetPlan().PlannedReactionAbility(), Reazione);

	DestroyInteractionWorld(B.World);
	return true;
}

/**
 * 🔑 **OGNI AZIONE HA IL SUO SEGNO DEI WAYPOINT, E IL BACK USA QUELLO DI CHI DISARMA** ([D-557] (4)).
 *
 * Reazione e principale possono stare nel piano insieme, ciascuna entrata con un conteggio di waypoint diverso. Il caso:
 * la reazione entra a zero waypoint, se ne posa uno, entra la guardia (segno 1), se ne posa un altro (due in tutto).
 * Il primo Back toglie il waypoint posato dopo la guardia (restano uno); il secondo trova zero waypoint DOPO la guardia
 * e la disarma. Con il segno della reazione (0) al posto di quello della guardia (1) il secondo Back toglierebbe anche
 * l'ultimo waypoint, e la guardia resterebbe nel piano: i due segni valgono 0 e 1 proprio perche' un test che li
 * confondesse non passerebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBackUsesTheWaypointMarkOfTheActionBeingDisarmedTest,
	"RefactorTactics.PlayerInput.BackUsesTheWaypointMarkOfTheActionBeingDisarmed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBackUsesTheWaypointMarkOfTheActionBeingDisarmedTest::RunTest(const FString&)
{
	FRTBancoTetto B = AllestisciBancoTetto(FRTCellId(2, -2, 0));
	if (!TestTrue(TEXT("banco allestito, con Action.Overwatch nel kit"), B.Valido()))
	{
		DestroyInteractionWorld(B.World); return false;
	}
	const int32 Reazione = AggiungiReazione(B.Unit, B.IdxOverwatch);
	if (!ReazioneValida(*this, B, Reazione))
	{
		DestroyInteractionWorld(B.World); return false;
	}
	const int32 Guardia = AggiungiGuardia(B.Unit, B.IdxOverwatch, Reazione);
	if (!TestTrue(TEXT("premessa: la guardia e' entrata nel kit, in una posizione libera"),
			Guardia != INDEX_NONE && Guardia != Reazione && Guardia != B.IdxOverwatch && B.Unit->CanUseAbility(Guardia)))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	B.PC->SelectActorForTest(B.Unit);
	B.PC->SelectAbilityForCurrentForTest(Reazione);      // segno della reazione: 0 waypoint
	B.PC->HandleClickOnCell(FRTCellId(3, -2, 0));          // un waypoint fra le due
	B.PC->SelectAbilityForCurrentForTest(Guardia);       // segno della guardia: 1 waypoint
	B.PC->HandleClickOnCell(FRTCellId(3, -1, 0));          // e uno dopo la guardia
	if (!TestEqual(TEXT("premessa: lo slot acceso e' la guardia"), B.Unit->SelectedAbilityIndex, Guardia)
		|| !TestEqual(TEXT("premessa: la guardia e' nel piano"), B.Unit->GetPlan().PlannedAbilityIndex(), Guardia)
		|| !TestEqual(TEXT("premessa: la reazione e' ancora nel piano"), B.Unit->GetPlan().PlannedReactionAbility(), Reazione)
		|| !TestEqual(TEXT("premessa: il segno della reazione e' 0"), B.Unit->WaypointsAllaReazione, 0)
		|| !TestEqual(TEXT("premessa: il segno della guardia e' 1"), B.Unit->WaypointsAllaDichiarazione, 1)
		|| !TestEqual(TEXT("premessa: i waypoint sono due"), B.Unit->GetPlan().PlannedWaypoints().Num(), 2))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	TestEqual(TEXT("il primo Back toglie il waypoint posato dopo la guardia"),
		B.PC->ApplyBack(), ERTPointerBackStep::Waypoint);
	TestEqual(TEXT("ne resta uno"), B.Unit->GetPlan().PlannedWaypoints().Num(), 1);
	TestEqual(TEXT("la guardia e' ancora nel piano"), B.Unit->GetPlan().PlannedAbilityIndex(), Guardia);

	// Il segno DELLA GUARDIA e' 1 e i waypoint sono 1: nessuno posato dopo di lei. Col segno della reazione (0) ne
	// risulterebbe uno "posato dopo", e il Back lo toglierebbe invece di disarmare.
	TestEqual(TEXT("il secondo Back disarma la guardia, non toglie un altro waypoint"),
		B.PC->ApplyBack(), ERTPointerBackStep::Declaration);
	TestEqual(TEXT("la guardia esce dal piano"), B.Unit->GetPlan().PlannedAbilityIndex(), (int32)INDEX_NONE);
	TestEqual(TEXT("il waypoint di prima resta"), B.Unit->GetPlan().PlannedWaypoints().Num(), 1);
	TestEqual(TEXT("e la reazione indipendente resta nel piano"), B.Unit->GetPlan().PlannedReactionAbility(), Reazione);

	DestroyInteractionWorld(B.World);
	return true;
}

/**
 * 🔑 **IL BACK USA IL SEGNO DELLA REAZIONE ANCHE CON UNA PRINCIPALE GIA' NEL PIANO** ([D-557] (4), dalla review di `#3933`).
 *
 * `BackUsesTheWaypointMarkOfTheActionBeingDisarmed` copre l'ordine «reazione prima, principale dopo», dove lo slot acceso e'
 * la principale. Qui l'inverso, che e' quello che il difetto di `#3927` (b) descrive: la guardia entra a ZERO waypoint
 * (segno 0), se ne posa uno, entra la reazione (segno 1), se ne posa un altro. Lo slot acceso e' la REAZIONE, e il Back deve
 * disfare l'ultimo gesto con il SUO segno.
 *
 * ⚠️ **Perche' serve, e quale mutante lo rende necessario:** scegliere il segno della principale quando una principale e' nel
 * piano — `(bReazioneNelPiano && PlannedAbilityIndex == INDEX_NONE) ? Reazione : Dichiarazione` — sopravviveva a tutti gli
 * altri test: dove la reazione ha una principale sotto (`BackOnAReactionKeepsAPlannedOverwatchAndItsCap`) i due segni valgono
 * lo stesso numero, perche' la reazione e' armata subito dopo il troncamento. Qui valgono 0 e 1 apposta. Col segno della
 * guardia (0) il secondo Back troverebbe un waypoint "posato dopo" e lo toglierebbe invece di disarmare la reazione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBackUsesTheReactionMarkWithAMainActionAlreadyPlannedTest,
	"RefactorTactics.PlayerInput.BackUsesTheReactionMarkWithAMainActionAlreadyPlanned",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBackUsesTheReactionMarkWithAMainActionAlreadyPlannedTest::RunTest(const FString&)
{
	FRTBancoTetto B = AllestisciBancoTetto(FRTCellId(2, -2, 0));
	if (!TestTrue(TEXT("banco allestito, con Action.Overwatch nel kit"), B.Valido()))
	{
		DestroyInteractionWorld(B.World); return false;
	}
	const int32 Reazione = AggiungiReazione(B.Unit, B.IdxOverwatch);
	if (!ReazioneValida(*this, B, Reazione))
	{
		DestroyInteractionWorld(B.World); return false;
	}
	const int32 Guardia = AggiungiGuardia(B.Unit, B.IdxOverwatch, Reazione);
	if (!TestTrue(TEXT("premessa: la guardia e' entrata nel kit, in una posizione libera"),
			Guardia != INDEX_NONE && Guardia != Reazione && Guardia != B.IdxOverwatch && B.Unit->CanUseAbility(Guardia)))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	B.PC->SelectActorForTest(B.Unit);
	B.PC->SelectAbilityForCurrentForTest(Guardia);        // segno della guardia: 0 waypoint
	B.PC->HandleClickOnCell(FRTCellId(3, -2, 0));          // un waypoint fra le due
	B.PC->SelectAbilityForCurrentForTest(Reazione);       // segno della reazione: 1 waypoint
	if (!TestEqual(TEXT("premessa: lo slot acceso e' la REAZIONE"), B.Unit->SelectedAbilityIndex, Reazione)
		|| !TestEqual(TEXT("premessa: la guardia e' nel piano"), B.Unit->GetPlan().PlannedAbilityIndex(), Guardia)
		|| !TestEqual(TEXT("premessa: e la reazione pure"), B.Unit->GetPlan().PlannedReactionAbility(), Reazione)
		|| !TestEqual(TEXT("premessa: il segno della guardia e' 0"), B.Unit->WaypointsAllaDichiarazione, 0)
		|| !TestEqual(TEXT("premessa: il segno della reazione e' 1"), B.Unit->WaypointsAllaReazione, 1)
		|| !TestEqual(TEXT("premessa: il waypoint c'e'"), B.Unit->GetPlan().PlannedWaypoints().Num(), 1))
	{
		DestroyInteractionWorld(B.World); return false;
	}
	B.PC->HandleClickOnCell(FRTCellId(3, -1, 0));          // e uno DOPO la reazione
	if (!TestEqual(TEXT("premessa: i waypoint sono due"), B.Unit->GetPlan().PlannedWaypoints().Num(), 2))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	TestEqual(TEXT("il primo Back toglie il waypoint posato dopo la reazione"),
		B.PC->ApplyBack(), ERTPointerBackStep::Waypoint);
	TestEqual(TEXT("ne resta uno"), B.Unit->GetPlan().PlannedWaypoints().Num(), 1);
	TestEqual(TEXT("la reazione e' ancora nel piano"), B.Unit->GetPlan().PlannedReactionAbility(), Reazione);

	// Il segno DELLA REAZIONE e' 1 e il waypoint e' 1: nessuno posato dopo di lei. Col segno della guardia (0) ne
	// risulterebbe uno "posato dopo", e il Back lo toglierebbe invece di disarmare.
	TestEqual(TEXT("il secondo Back disarma la REAZIONE, non toglie un altro waypoint"),
		B.PC->ApplyBack(), ERTPointerBackStep::Declaration);
	TestEqual(TEXT("la reazione esce dal piano"), B.Unit->GetPlan().PlannedReactionAbility(), (int32)INDEX_NONE);
	TestEqual(TEXT("la guardia RESTA nel piano"), B.Unit->GetPlan().PlannedAbilityIndex(), Guardia);
	TestEqual(TEXT("col suo segno intatto"), B.Unit->WaypointsAllaDichiarazione, 0);
	TestEqual(TEXT("e il waypoint di prima resta"), B.Unit->GetPlan().PlannedWaypoints().Num(), 1);

	DestroyInteractionWorld(B.World);
	return true;
}

/**
 * IL BACK SU UN'AZIONE ARMATA MA NON NEL PIANO NON TOCCA QUELLA PIANIFICATA - `#3501`, guardia.
 *
 * 🔑 **Il Back disfa l'ultimo gesto, non il piano intero.** Con l'`Overwatch` gia' pianificato, armare un'azione
 * a bersaglio apre un targeting; il Back chiude quel targeting e basta. Se il disarmo di `#3501` scattasse su
 * qualunque Back a livello `Declaration`, questo toglierebbe l'`Overwatch` che il giocatore non ha toccato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBackOnATargetingKeepsThePlanTest,
	"RefactorTactics.PlayerInput.BackOnATargetingKeepsThePlannedAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBackOnATargetingKeepsThePlanTest::RunTest(const FString&)
{
	FRTBancoTetto B = AllestisciBancoTetto(FRTCellId(2, -2, 0));
	if (!TestTrue(TEXT("banco allestito"), B.Valido()))
	{
		DestroyInteractionWorld(B.World); return false;
	}
	// Un'azione a bersaglio del kit, cercata e non scritta: la posizione dipende dall'eroe.
	int32 IdxBersaglio = INDEX_NONE;
	for (int32 i = 0; i < B.Unit->NumAbilities(); ++i)
	{
		const URTActionData* A = B.Unit->GetAbility(i);
		if (A && !A->bSelfTarget && A->Def.ReservesMovementProfileId.IsNone() && A->Def.Slot == ERTActionSlot::Main)
		{
			IdxBersaglio = i; break;
		}
	}
	if (!TestNotEqual(TEXT("premessa: il kit ha un'azione a bersaglio"), IdxBersaglio, (int32)INDEX_NONE))
	{
		DestroyInteractionWorld(B.World); return false;
	}
	B.PC->SelectActorForTest(B.Unit);
	B.PC->SelectAbilityForCurrentForTest(B.IdxOverwatch);
	B.PC->SelectAbilityForCurrentForTest(IdxBersaglio);
	if (!TestEqual(TEXT("premessa: armata l'azione a bersaglio"), B.Unit->SelectedAbilityIndex, IdxBersaglio)
		|| !TestEqual(TEXT("premessa: l'Overwatch resta nel piano"), B.Unit->GetPlan().PlannedAbilityIndex(), B.IdxOverwatch))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	TestEqual(TEXT("il Back esce dal targeting"), B.PC->ApplyBack(), ERTPointerBackStep::Declaration);

	TestEqual(TEXT("la selezione torna neutra"), B.Unit->SelectedAbilityIndex, (int32)INDEX_NONE);
	TestEqual(TEXT("ma l'Overwatch, che il Back non ha toccato, resta nel piano"),
		B.Unit->GetPlan().PlannedAbilityIndex(), B.IdxOverwatch);
	TestEqual(TEXT("e il suo tetto resta"),
		URTMovementProfileLibrary::ReservedProfileForPlan(URTPlanValidationLibrary::MakePlanFor(B.Unit)),
		URTMovementProfileLibrary::ProfileWithdraw);

	DestroyInteractionWorld(B.World);
	return true;
}

/**
 * `#3470` — IL TASTO CHE LA BARRA MOSTRA PER `Sneak` E' QUELLO CHE IL CONTESTO DI INPUT LEGA DAVVERO.
 *
 * 🔑 **Interroga il `UInputMappingContext` reale**, come `HotkeysDoNotCollide`: la barra legge
 * `RTInputKeys::SneakHotkey()`, e questo test prova che la stessa sede alimenta `IA_DeclareSneak`. Se
 * qualcuno rimappasse il gesto scrivendo un tasto a mano in `BuildInputMappings`, la barra direbbe `M` per un
 * tasto morto — e questo diventerebbe rosso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerInputSneakKeyTest,
	"RefactorTactics.PlayerInput.SneakIsMappedOnTheKeyTheBarShows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerInputSneakKeyTest::RunTest(const FString&)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
	Ctx.SetCurrentWorld(World);

	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	bool bOk = TestNotNull(TEXT("controller"), PC);
	if (bOk)
	{
		const UInputMappingContext* Imc = PC->BuildAndGetMappingContextForTest();
		bOk = TestNotNull(TEXT("mapping context costruito"), Imc);
		if (bOk)
		{
			TArray<FKey> TastiDiSneak;
			for (const FEnhancedActionKeyMapping& M : Imc->GetMappings())
			{
				if (M.Action && M.Action->GetName() == TEXT("IA_DeclareSneak"))
				{
					TastiDiSneak.Add(M.Key);
				}
			}

			TestEqual(TEXT("IA_DeclareSneak ha esattamente un tasto"), TastiDiSneak.Num(), 1);
			if (TastiDiSneak.Num() == 1)
			{
				TestEqual(TEXT("ed e' SneakHotkey(), cioe' quello che la barra mostra"),
					TastiDiSneak[0], RTInputKeys::SneakHotkey());
			}
			// L'etichetta che la barra stampa, scritta a mano: il controllo positivo dell'uguaglianza sopra.
			TestEqual(TEXT("l'etichetta del tasto e' M"),
				RTInputKeys::SneakHotkey().GetDisplayName(false).ToString(), FString(TEXT("M")));
		}
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
	return bOk;
}

/**
 * D-488 (3) — `TAB` apre la vista strategica e il ciclo della selezione e' su `N`, **sul contesto reale**.
 *
 * 🔴 **Le due meta' si asseriscono insieme perche' D-488 le vuole nello stesso commit.** Un `Tab` che restasse
 * anche sul ciclo farebbe due cose (lo prenderebbe `HotkeysDoNotCollide`); un ciclo spostato senza la vista
 * lascerebbe `Tab` muto, e quello solo questo test lo vede.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerInputStrategicViewKeyTest,
	"RefactorTactics.PlayerInput.StrategicViewIsOnTabAndSelectionCycleOnN",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerInputStrategicViewKeyTest::RunTest(const FString&)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
	Ctx.SetCurrentWorld(World);

	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	bool bOk = TestNotNull(TEXT("controller"), PC);
	if (bOk)
	{
		const UInputMappingContext* Imc = PC->BuildAndGetMappingContextForTest();
		bOk = TestNotNull(TEXT("mapping context costruito"), Imc);
		if (bOk)
		{
			TArray<FKey> TastiDellaVista;
			TArray<FKey> TastiDelCiclo;
			for (const FEnhancedActionKeyMapping& M : Imc->GetMappings())
			{
				if (!M.Action) { continue; }
				if (M.Action->GetName() == TEXT("IA_ToggleStrategicView")) { TastiDellaVista.Add(M.Key); }
				if (M.Action->GetName() == TEXT("IA_CycleSelection"))      { TastiDelCiclo.Add(M.Key); }
			}

			TestEqual(TEXT("IA_ToggleStrategicView ha esattamente un tasto"), TastiDellaVista.Num(), 1);
			TestEqual(TEXT("IA_CycleSelection ha esattamente un tasto"), TastiDelCiclo.Num(), 1);
			if (TastiDellaVista.Num() == 1)
			{
				TestEqual(TEXT("la vista strategica e' su StrategicViewHotkey()"),
					TastiDellaVista[0], RTInputKeys::StrategicViewHotkey());
			}
			if (TastiDelCiclo.Num() == 1)
			{
				TestEqual(TEXT("il ciclo e' su CycleSelectionHotkey()"),
					TastiDelCiclo[0], RTInputKeys::CycleSelectionHotkey());
			}
			// I tasti scritti a mano: il controllo positivo delle due uguaglianze sopra, che da sole
			// resterebbero vere anche se le due sedi si scambiassero i tasti.
			TestEqual(TEXT("la vista strategica e' su Tab"), RTInputKeys::StrategicViewHotkey(), EKeys::Tab);
			TestEqual(TEXT("il ciclo della selezione e' su N"), RTInputKeys::CycleSelectionHotkey(), EKeys::N);
		}
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
	return bOk;
}

/**
 * `#3471` — I PULSANTI `CONFERMA` E `ANNULLA` MOSTRANO I TASTI CHE IL CONTESTO DI INPUT LEGA DAVVERO ([D-458]).
 *
 * Stessa forma di `SneakIsMappedOnTheKeyTheBarShows`, sul contesto reale: le etichette dei pulsanti leggono
 * `DeclarePlanHotkey()` e `UndoKeyboardHotkey()`, e qui si prova che le stesse sedi alimentano la mappatura.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerInputCommitKeysTest,
	"RefactorTactics.PlayerInput.CommitButtonsShowTheKeysTheContextMaps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerInputCommitKeysTest::RunTest(const FString&)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
	Ctx.SetCurrentWorld(World);

	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	bool bOk = TestNotNull(TEXT("controller"), PC);
	if (bOk)
	{
		const UInputMappingContext* Imc = PC->BuildAndGetMappingContextForTest();
		bOk = TestNotNull(TEXT("mapping context costruito"), Imc);
		if (bOk)
		{
			TArray<FKey> Dichiara;
			TArray<FKey> Indietro;
			for (const FEnhancedActionKeyMapping& M : Imc->GetMappings())
			{
				if (!M.Action) { continue; }
				if (M.Action->GetName() == TEXT("IA_DeclarePlan")) { Dichiara.Add(M.Key); }
				if (M.Action->GetName() == TEXT("IA_UndoWaypoint")) { Indietro.Add(M.Key); }
			}
			TestEqual(TEXT("IA_DeclarePlan ha esattamente un tasto"), Dichiara.Num(), 1);
			if (Dichiara.Num() == 1)
			{
				TestEqual(TEXT("ed e' DeclarePlanHotkey(), cioe' quello che Conferma mostra"),
					Dichiara[0], RTInputKeys::DeclarePlanHotkey());
			}
			TestTrue(TEXT("IA_UndoWaypoint e' legata a UndoKeyboardHotkey(), cioe' quello che Annulla mostra"),
				Indietro.Contains(RTInputKeys::UndoKeyboardHotkey()));
			TestTrue(TEXT("e resta legata anche al tasto destro: il pulsante non toglie il gesto"),
				Indietro.Contains(EKeys::RightMouseButton));
		}
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
	return bOk;
}

/**
 * [D-528] (`#3769`) — **con un nemico noto in mezzo, la carica si pianifica su quello cliccato, si ferma davanti al primo
 * e dice chi colpira'**.
 *
 * Il caso: Riktor in `(0,0)`, A (Ivrin) in `(2,0)` e B (Muiren) in `(3,0)`, entrambi visti. Il giocatore clicca B. Il resolver
 * ferma la carica in `(1,0)` e colpisce A ([D-296]); fino a [D-528] l'anteprima disegnava la retta fino a B, col segno d'arrivo
 * su di lui, e l'avviso non diceva niente.
 *
 * Cinque cose, perche' l'anteprima le legge da posti diversi e una sola lettura sbagliata basta a mentire:
 * - il piano resta su B (l'intento registrato e' quello cliccato);
 * - la lettura di pianificazione dice dove si ferma e chi urta;
 * - l'avviso e' UNO, attribuito alla carica, e nomina A e B;
 * - l'anteprima, dalla vista FILTRATA che l'HUD disegna, e' piena fino a `(1,0)` e rossa da li' a B;
 * - il controllo positivo: cliccando A (il primo, a contatto) il segno sta nello stesso punto, l'avviso non c'e' e il tratto
 *   fino a lui NON e' rosso. Senza questa meta' il test passerebbe anche con ogni carica segnata come mancata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerChargeInTheWayTest,
	"RefactorTactics.PlayerInput.ChargeWithAKnownEnemyInTheWayIsPlannedAndSaysWhoItHits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerChargeInTheWayTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTHexMapActor* MapActor = SpawnCleanInteractionMap(World, /*Radius=*/ 6);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTUnit* Charger = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0, 0));
	ARTUnit* A = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(2, 0, 0));
	ARTUnit* B = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(3, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!MapActor || !TM || !Charger || !A || !B || !PC) { DestroyInteractionWorld(World); return false; }
	TM->RefreshTeamKnowledgeNow();

	const int32 ChargeIdx = FindChargeAbilityIndex(Charger);
	if (!TestTrue(TEXT("premessa: la carica ha una portata che arriva a B"),
		ChargeIdx != INDEX_NONE && Charger->EffectiveDashRangeOf(ChargeIdx) >= 3)) { DestroyInteractionWorld(World); return false; }
	const FName CaricaId = Charger->GetAbility(ChargeIdx)->Def.ActionId;
	TestTrue(TEXT("premessa: la squadra vede A e B"),
		URTTeamKnowledgeLibrary::ClassifyTarget(TM->KnowledgeForTeamPublic(0), A->StableUnitId, A->TeamId, A->Cell) == ERTTargetKnowledge::Allowed
		&& URTTeamKnowledgeLibrary::ClassifyTarget(TM->KnowledgeForTeamPublic(0), B->StableUnitId, B->TeamId, B->Cell) == ERTTargetKnowledge::Allowed);

	TArray<FRTIntentView> Viste; // la vista filtrata che l'HUD disegna; si rilegge a ogni chiamata
	const auto VistaDellaCarica = [&]() -> const FRTIntentView*
	{
		Viste = URTIntentPrivacyLibrary::FilterForTeam(0, URTHudViewModel::BuildAuthoritativeIntents({ Charger, A, B }));
		return Viste.FindByPredicate([Charger](const FRTIntentView& V) { return V.OwnerCell == Charger->Cell; });
	};
	const auto AvvisiDellaCarica = [&]()
	{
		TArray<FRTPlanWarningView> Dalla;
		for (const FRTPlanWarningView& V : URTHudViewModel::BuildPlanWarnings(Charger, { Charger, A, B }))
		{
			if (V.SourceActionId == CaricaId) { Dalla.Add(V); }
		}
		return Dalla;
	};

	// (1) Il click su B: il piano resta su B.
	PC->SelectActorForTest(Charger);
	Charger->SelectAbility(ChargeIdx);
	TestTrue(TEXT("il click su B e' consumato"), PC->DispatchUnitClickForTest(B, Charger));
	TestTrue(TEXT("e la carica e' pianificata verso B, non verso A"),
		Charger->GetPlan().PlannedDashAbility() == ChargeIdx && Charger->GetPlan().PlannedDashCell() == B->Cell);

	// (2) La lettura di pianificazione.
	FRTChargeRun Corsa;
	if (TestTrue(TEXT("chi ha pianificato ha scritto dove si ferma la carica"), Charger->GetPlannedChargeRun(Corsa)))
	{
		TestEqual(TEXT("percorre una cella"), Corsa.RunCells, 1);
		TestTrue(TEXT("urta A, sulla sua cella"), Corsa.bHasImpact && Corsa.ImpactCell == A->Cell);
		TestTrue(TEXT("e manca B"), Corsa.bMissesTarget);
	}

	// (3) L'avviso: uno, Warning, che nomina A e B.
	const TArray<FRTPlanWarningView> Avvisi = AvvisiDellaCarica();
	if (TestEqual(TEXT("un solo avviso attribuito alla carica"), Avvisi.Num(), 1))
	{
		const FString NomeA = ARTUnit::DisplayLabel(A->HeroDisplayName, A->HeroId, A->GetName());
		const FString NomeB = ARTUnit::DisplayLabel(B->HeroDisplayName, B->HeroId, B->GetName());
		TestTrue(TEXT("e' un Warning: il piano e' legale e degradato"), Avvisi[0].Level == ERTPlanWarningLevel::Warning);
		TestTrue(TEXT("dice chi colpira'"), Avvisi[0].Cost.ToString().Contains(NomeA));
		TestTrue(TEXT("e chi non colpira'"), Avvisi[0].Cost.ToString().Contains(NomeB));
	}

	// (4) L'anteprima dalla vista filtrata: piena fino a (1,0), rossa da li' a B.
	if (const FRTIntentView* Mia = VistaDellaCarica(); TestNotNull(TEXT("la vista della propria carica"), Mia))
	{
		TestEqual(TEXT("la vista porta quante celle la carica percorre"), Mia->DashRunCells, 1);
		TestTrue(TEXT("e che manca il bersaglio"), Mia->bDashMissesTarget);
		const FRTDashPreview Anteprima = ARTHUD::ComposeDashPreview(*Mia, MapActor->MapAsset);
		TestTrue(TEXT("la retta arriva fino a B"), Anteprima.PathCells.Num() > 0 && Anteprima.PathCells.Last() == B->Cell);
		TestTrue(TEXT("il segno d'arrivo sta davanti ad A"), Anteprima.StopCell() == FRTCellId(1, 0, 0));
		TestTrue(TEXT("e il resto fino a B e' il tratto che non fara'"), Anteprima.HasShortfall() && Anteprima.bShowsMiss);
	}

	// (5) Il controllo positivo: A cliccato, a contatto. Stesso punto d'arresto, nessun avviso, nessun rosso.
	Charger->ClearPlannedDash();
	PC->SelectActorForTest(Charger);
	Charger->SelectAbility(ChargeIdx);
	PC->DispatchUnitClickForTest(A, Charger);
	TestTrue(TEXT("controllo: la carica su A si pianifica"), Charger->GetPlan().PlannedDashAbility() == ChargeIdx && Charger->GetPlan().PlannedDashCell() == A->Cell);
	FRTChargeRun SuA;
	if (TestTrue(TEXT("controllo: anche questa ha la sua lettura"), Charger->GetPlannedChargeRun(SuA)))
	{
		TestEqual(TEXT("controllo: percorre la stessa cella"), SuA.RunCells, 1);
		TestFalse(TEXT("controllo: e non manca il bersaglio"), SuA.bMissesTarget);
	}
	TestEqual(TEXT("controllo: nessun avviso"), AvvisiDellaCarica().Num(), 0);
	if (const FRTIntentView* Mia = VistaDellaCarica(); TestNotNull(TEXT("controllo: la vista"), Mia))
	{
		const FRTDashPreview Anteprima = ARTHUD::ComposeDashPreview(*Mia, MapActor->MapAsset);
		TestTrue(TEXT("controllo: il segno d'arresto sta accanto ad A, non su di lui"), Anteprima.StopCell() == FRTCellId(1, 0, 0));
		TestFalse(TEXT("controllo: e il tratto fino ad A non e' rosso"), Anteprima.bShowsMiss);
	}

	DestroyInteractionWorld(World);
	return true;
}

/**
 * [D-528] (`#3769`) / [D-225] — **l'anteprima e l'avviso non dicono un nemico che la squadra non conosce**.
 *
 * Tre mondi, che differiscono per UNA cosa ciascuno. In tutti, Riktor in `(0,0)` clicca la cella vuota `(3,0)`:
 * - **N**: nessun nemico sulla retta;
 * - **U**: un nemico in `(2,0)` che la squadra NON conosce (la conoscenza non e' stata aggiornata);
 * - **K**: lo stesso nemico, conosciuto.
 *
 * Per chi pianifica N e U devono essere indistinguibili — la lettura, l'anteprima, gli avvisi — perche' vederli diversi
 * direbbe «li' c'e' qualcuno». Alla risoluzione la carica lo colpira' lo stesso, ed e' la sorpresa di [D-516] (4).
 * K e' il controllo positivo: senza che differisca, il confronto N/U passerebbe anche se il codice non guardasse mai i nemici.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerChargeUnknownEnemyTest,
	"RefactorTactics.PlayerInput.ChargePreviewDoesNotRevealAnEnemyTheSquadDoesNotKnow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerChargeUnknownEnemyTest::RunTest(const FString&)
{
	struct FLettura
	{
		bool bPianificata = false;
		bool bHaLaLettura = false;
		FRTChargeRun Corsa;
		int32 Tratti = INDEX_NONE;
		FRTCellId Arresto;
		bool bRosso = false;
		int32 Avvisi = INDEX_NONE;
	};

	const auto Leggi = [this](bool bConNemico, bool bConosciuto) -> FLettura
	{
		FLettura L;
		UWorld* World = MakeInteractionWorld();
		if (!TestNotNull(TEXT("world di prova"), World)) { return L; }
		ARTHexMapActor* MapActor = SpawnCleanInteractionMap(World, /*Radius=*/ 6);
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		ARTUnit* Charger = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0, 0));
		ARTUnit* Nemico = bConNemico ? SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(2, 0, 0)) : nullptr;
		ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
		if (!MapActor || !TM || !Charger || !PC || (bConNemico && !Nemico)) { DestroyInteractionWorld(World); return L; }
		if (bConosciuto) { TM->RefreshTeamKnowledgeNow(); }

		const int32 ChargeIdx = FindChargeAbilityIndex(Charger);
		if (!TestNotEqual(TEXT("premessa: la carica"), ChargeIdx, (int32)INDEX_NONE)) { DestroyInteractionWorld(World); return L; }
		PC->SelectActorForTest(Charger);
		Charger->SelectAbility(ChargeIdx);
		PC->HandleClickOnCellForTest(FRTCellId(3, 0, 0));

		L.bPianificata = Charger->GetPlan().PlannedDashAbility() == ChargeIdx && Charger->GetPlan().PlannedDashCell() == FRTCellId(3, 0, 0);
		L.bHaLaLettura = Charger->GetPlannedChargeRun(L.Corsa);
		TArray<ARTUnit*> Unita = { Charger };
		if (Nemico) { Unita.Add(Nemico); }
		TArray<AActor*> Attori;
		for (ARTUnit* U : Unita) { Attori.Add(U); }
		const TArray<FRTIntentView> Viste = URTIntentPrivacyLibrary::FilterForTeam(0, URTHudViewModel::BuildAuthoritativeIntents(Attori));
		if (const FRTIntentView* Mia = Viste.FindByPredicate([Charger](const FRTIntentView& V) { return V.OwnerCell == Charger->Cell; }))
		{
			const FRTDashPreview P = ARTHUD::ComposeDashPreview(*Mia, MapActor->MapAsset);
			L.Tratti = P.ReachSegments;
			L.Arresto = P.StopCell();
			L.bRosso = P.HasShortfall() && P.bShowsMiss;
		}
		L.Avvisi = URTHudViewModel::BuildPlanWarnings(Charger, Unita).Num();
		DestroyInteractionWorld(World);
		return L;
	};

	const FLettura N = Leggi(/*bConNemico=*/ false, /*bConosciuto=*/ false);
	const FLettura U = Leggi(/*bConNemico=*/ true, /*bConosciuto=*/ false);
	const FLettura K = Leggi(/*bConNemico=*/ true, /*bConosciuto=*/ true);

	TestTrue(TEXT("premessa N: la carica sulla cella vuota si pianifica"), N.bPianificata && N.bHaLaLettura);
	TestTrue(TEXT("premessa U: si pianifica anche con un nemico ignoto in mezzo"), U.bPianificata && U.bHaLaLettura);

	// N e U: indistinguibili.
	TestEqual(TEXT("U come N: le stesse celle di corsa"), U.Corsa.RunCells, N.Corsa.RunCells);
	TestEqual(TEXT("U come N: nessun urto"), U.Corsa.bHasImpact, N.Corsa.bHasImpact);
	TestEqual(TEXT("U come N: non manca il bersaglio"), U.Corsa.bMissesTarget, N.Corsa.bMissesTarget);
	TestEqual(TEXT("U come N: lo stesso numero di tratti disegnati"), U.Tratti, N.Tratti);
	TestTrue(TEXT("U come N: lo stesso segno d'arrivo"), U.Arresto == N.Arresto);
	TestEqual(TEXT("U come N: lo stesso rosso"), U.bRosso, N.bRosso);
	TestEqual(TEXT("U come N: gli stessi avvisi"), U.Avvisi, N.Avvisi);

	// K: il controllo positivo. Se non differisce, N/U non misura niente.
	TestEqual(TEXT("controllo K: con il nemico noto la corsa si accorcia"), K.Corsa.RunCells, 1);
	TestTrue(TEXT("controllo K: urta lui e manca il bersaglio"), K.Corsa.bHasImpact && K.Corsa.bMissesTarget);
	TestTrue(TEXT("controllo K: il segno d'arrivo sta davanti a lui"), K.Arresto == FRTCellId(1, 0, 0));
	TestTrue(TEXT("controllo K: e il resto e' rosso"), K.bRosso);
	TestEqual(TEXT("controllo K: l'avviso c'e'"), K.Avvisi, 1);
	return true;
}

/**
 * [D-528] (`#3769`) — **un secondo click sullo stesso nemico, con la carica gia' pianificata su di lui, lo ispeziona**.
 *
 * Fino a [D-528] il secondo click ripeteva il primo: la carica armata resta armata, il click sul nemico risponde `Confirm`
 * e arriva alla cella del nemico, che per una carica e' la cella finale — e li' si apre il selettore del verso. Ma una carica
 * non ha un verso da scegliere: l'ultimo passo lo detta ([D-524] (4)), e il giocatore voleva guardare il nemico che aveva
 * appena puntato.
 *
 * Tre cose, perche' la regola e' stretta e si rompe da due parti:
 * - il secondo click sullo stesso nemico lo ispeziona, non apre il selettore e non tocca il piano;
 * - un click su UN ALTRO nemico, con la carica gia' pianificata sul primo, continua a RIPIANIFICARE: non e' un secondo click;
 * - il primo click, prima che il piano esista, pianifica (lo prova gia' `ChargeIsPlannedByClickingTheEnemy`, qui e' il controllo).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerChargeSecondClickTest,
	"RefactorTactics.PlayerInput.SecondClickOnThePlannedChargeTargetInspectsIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerChargeSecondClickTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTHexMapActor* MapActor = SpawnCleanInteractionMap(World, /*Radius=*/ 6);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTUnit* Charger = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0, 0));
	ARTUnit* Primo = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(3, 0, 0));
	ARTUnit* Secondo = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(0, 3, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!MapActor || !TM || !Charger || !Primo || !Secondo || !PC) { DestroyInteractionWorld(World); return false; }
	TM->RefreshTeamKnowledgeNow();

	const int32 ChargeIdx = FindChargeAbilityIndex(Charger);
	if (!TestTrue(TEXT("premessa: la carica arriva a tutti e due"),
		ChargeIdx != INDEX_NONE && Charger->EffectiveDashRangeOf(ChargeIdx) >= 3)) { DestroyInteractionWorld(World); return false; }

	PC->SelectActorForTest(Charger);
	Charger->SelectAbility(ChargeIdx);

	// Il primo click pianifica.
	PC->DispatchUnitClickForTest(Primo, Charger);
	TestTrue(TEXT("controllo: il primo click pianifica la carica sul primo"),
		Charger->GetPlan().PlannedDashAbility() == ChargeIdx && Charger->GetPlan().PlannedDashCell() == Primo->Cell);
	TestTrue(TEXT("controllo: e non lo ispeziona"), PC->GetInspectedUnit() != Primo);
	TestTrue(TEXT("controllo: la carica resta armata"), Charger->SelectedAbilityIndex == ChargeIdx);

	// Il secondo click sullo stesso nemico lo ispeziona.
	TestTrue(TEXT("il secondo click e' consumato"), PC->DispatchUnitClickForTest(Primo, Charger));
	TestTrue(TEXT("e ispeziona il nemico"), PC->GetInspectedUnit() == Primo);
	TestFalse(TEXT("senza aprire il selettore del verso"), PC->IsDeclaringFacing());
	TestTrue(TEXT("e il contesto non e' il verso"), PC->GetPointerContext() != ERTPointerContext::Facing);
	TestTrue(TEXT("il piano non cambia"), Charger->GetPlan().PlannedDashAbility() == ChargeIdx && Charger->GetPlan().PlannedDashCell() == Primo->Cell);
	TestTrue(TEXT("e la carica resta armata"), Charger->SelectedAbilityIndex == ChargeIdx);

	// Lo stesso cliccando il PAVIMENTO dell'esagono del nemico, non la sua mesh: il raggio colpisce la mappa e non l'unita', e il
	// click arriva a `HandleClickOnCell` invece che a `DispatchUnitClick`. E' il percorso che apriva il selettore: la tavola di
	// `PIE-V01-CHARGEHOVER` lo ha trovato, e il click sulla mesh da solo non lo vedeva.
	PC->SetInspectedUnitForTest(nullptr);
	PC->HandleClickOnCellForTest(Primo->Cell);
	TestTrue(TEXT("il pavimento del suo esagono lo ispeziona"), PC->GetInspectedUnit() == Primo);
	TestFalse(TEXT("senza aprire il selettore del verso"), PC->IsDeclaringFacing());
	TestTrue(TEXT("e il contesto non e' il verso"), PC->GetPointerContext() != ERTPointerContext::Facing);
	TestTrue(TEXT("il piano non cambia"), Charger->GetPlan().PlannedDashAbility() == ChargeIdx && Charger->GetPlan().PlannedDashCell() == Primo->Cell);

	// ➕ `#3748`: l'ispezione ha portato la barra sul nemico, e in sola lettura il piano non si scrive e il selettore del verso non reclama nessun click
	// (`FacingHandlesClickOnFinalCell` risponde no per quella causa). Le righe che seguono provano due altre cause — la carica gia' pianificata sul
	// nemico, e il click su un altro nemico che ripianifica — e con la barra in sola lettura passerebbero, o fallirebbero, per la ragione sbagliata.
	// Si torna alla propria unita' con il gesto di sempre (`SelectUnit`): la scheda resta sul nemico, che e' cio' che l'ultimo asserto di sotto misura.
	PC->SelectUnit(Charger, /*bRecordAsPlayerInput=*/ false);
	TestTrue(TEXT("la scheda resta sul nemico mentre la barra torna alla propria unita'"), PC->GetInspectedUnit() == Primo && !PC->IsPlanningWriteBlocked());

	// Il click VERO passa da `OnSelect`, che manda un click sull'esagono finale al selettore PRIMA della matrice delle unita'
	// (`FRTFacingDeclaration::TryHandleClickUnderCursor`): per una carica l'esagono finale e' quello del nemico. Quel ramo ha bisogno del cursore e
	// non gira headless, ma chiede alla stessa funzione di `TryOpenFacingSelector`, che qui si prova direttamente. ⚠️ Questa riga e'
	// stata trovata dalla tavola di `PIE-V01-CHARGEHOVER`: i click di prima, che entravano da `DispatchUnitClick` e `HandleClickOnCell`,
	// non passavano di li'.
	TestTrue(TEXT("la cella del bersaglio e' di un nemico noto: la funzione lo dice"), PC->PlannedChargeTargetAt(Charger, Primo->Cell) == Primo);
	TestTrue(TEXT("un'altra cella no"), PC->PlannedChargeTargetAt(Charger, Secondo->Cell) == nullptr);
	TestFalse(TEXT("il selettore non si apre sull'esagono del nemico puntato"), PC->TryOpenFacingSelector(Primo->Cell));
	TestFalse(TEXT("e non resta aperto"), PC->IsDeclaringFacing());
	// La decisione di `FRTFacingDeclaration::TryHandleClickUnderCursor` PRIMA del raggio. ⚠️ Era la meta' non coperta: i rami `Center` e `Side` consumano il
	// click comunque, quindi senza questa risposta il secondo click non arriva mai alla matrice delle unita' e resta morto (revisione di #3821).
	TestFalse(TEXT("il click sull'esagono del nemico puntato non va al selettore del verso"), PC->FacingHandlesClickOnFinalCell(Charger));
	TestFalse(TEXT("un click diretto su un lato di quell'esagono non dichiara un verso"), PC->HandleFacingClick(Primo->Cell, ERTHexDirection::E));
	TestFalse(TEXT("e non lo scrive nel piano"), Charger->GetPlan().bDeclaresPlannedFacing);
	// Controllo positivo: senza una carica pianificata, un movimento in marcia manda il click sulla sua cella finale al selettore.
	const int32 DashSalvato = Charger->GetPlan().PlannedDashAbility();
	Charger->ClearPlannedDash();
	FRTUnitPlanTestAccess::SetPath(*Charger, { Charger->Cell, FRTCellId(0, -1, 0) });
	TestTrue(TEXT("controllo: in marcia il click sulla cella finale va al selettore"), PC->FacingHandlesClickOnFinalCell(Charger));
	FRTUnitPlanTestAccess::ResetPath(*Charger);
	FRTUnitPlanTestAccess::SetDashAbility(*Charger, DashSalvato);

	// Un click su un ALTRO nemico ripianifica: non e' un secondo click.
	PC->DispatchUnitClickForTest(Secondo, Charger);
	TestTrue(TEXT("un altro nemico: la carica si ripianifica su di lui"),
		Charger->GetPlan().PlannedDashAbility() == ChargeIdx && Charger->GetPlan().PlannedDashCell() == Secondo->Cell);
	TestTrue(TEXT("e non lo ispeziona"), PC->GetInspectedUnit() == Primo);

	// ➕ `#3748` (revisione del Task 3): **ispezionare non e' scrivere il piano, e il click sul pavimento del bersaglio della carica funziona anche a
	// barra in sola lettura.** La carica e' ora pianificata su `Secondo`; l'ispezione e' su `Primo`, un altro nemico, e la barra e' in sola lettura su
	// di lui. Il click sul pavimento dell'esagono di `Secondo` — il raggio colpisce la mappa, quindi passa da `HandleClickOnCell` — deve spostare
	// l'ispezione su `Secondo`, come fa lo stesso click sulla sua mesh (`DispatchUnitClick`): due gesti dello stesso significato non possono dire due cose.
	PC->PinInspectionForTest(Primo);
	if (!TestTrue(TEXT("premessa: la barra e' in sola lettura su `Primo`, e la carica e' pianificata su `Secondo`"),
		PC->IsDockReadOnly() && PC->IsPlanningWriteBlocked() && PC->GetDockUnit() == Primo
			&& PC->PlannedChargeTargetAt(Charger, Secondo->Cell) == Secondo && PC->PlannedChargeTargetAt(Charger, Primo->Cell) == nullptr))
	{
		DestroyInteractionWorld(World);
		return false;
	}
	PC->HandleClickOnCellForTest(Secondo->Cell);
	TestTrue(TEXT("a barra in sola lettura il pavimento del bersaglio della carica sposta l'ispezione su di lui"), PC->GetInspectedUnit() == Secondo);
	TestTrue(TEXT("e la barra segue la nuova fissazione, ancora in sola lettura"), PC->IsDockReadOnly() && PC->GetDockUnit() == Secondo);
	TestTrue(TEXT("senza toccare il piano: la carica e' ancora su `Secondo` e resta armata"),
		Charger->GetPlan().PlannedDashAbility() == ChargeIdx && Charger->GetPlan().PlannedDashCell() == Secondo->Cell && Charger->SelectedAbilityIndex == ChargeIdx);

	// Ogni altra strada di `HandleClickOnCell` resta rifiutata: il pavimento di `Primo`, che ora NON e' il bersaglio, ripianificherebbe la carica
	// su di lui, e a barra in sola lettura non deve — e lo dice, una volta.
	AddExpectedMessagePlain(TEXT("chiudi la scheda con il tasto destro"), ELogVerbosity::Display, EAutomationExpectedMessageFlags::Contains, /*Occurrences=*/ 1);
	PC->HandleClickOnCellForTest(Primo->Cell);
	TestTrue(TEXT("un click su una cella che non e' il bersaglio della carica e' rifiutato: la carica resta su `Secondo`"),
		Charger->GetPlan().PlannedDashAbility() == ChargeIdx && Charger->GetPlan().PlannedDashCell() == Secondo->Cell);
	TestTrue(TEXT("e l'ispezione non si e' mossa"), PC->GetInspectedUnit() == Secondo);

	// Il controllo positivo: chiusa la fissazione, lo STESSO click ripianifica la carica su `Primo`. Senza, il rifiuto di sopra potrebbe valere perche'
	// quel click non pianifica mai.
	PC->SetInspectorPinned(false);
	PC->HandleClickOnCellForTest(Primo->Cell);
	TestTrue(TEXT("controllo: a fissazione chiusa lo stesso click ripianifica la carica su `Primo`"),
		Charger->GetPlan().PlannedDashAbility() == ChargeIdx && Charger->GetPlan().PlannedDashCell() == Primo->Cell);

	DestroyInteractionWorld(World);
	return true;
}

/**
 * [D-528] (`#3769`) / [D-128] — **l'hover di un nemico con la carica armata annuncia cio' che il click pianifichera'**, e per
 * questo l'hover e il click leggono la STESSA funzione.
 *
 * Fino a [D-528] l'hover mostrava l'annotazione pubblica di un'ispezione (l'arco del nemico) mentre il click pianificava una carica.
 * Ora, sul nemico che la carica armata punta, l'hover disegna cio' che il click scrivera' — con la stessa `ComposeDashPreview`
 * del piano — e non mostra l'arco.
 *
 * L'oracolo e' il CONFRONTO, caso per caso: la vista dell'hover e il piano che il click scrive devono dire le stesse cose. Un
 * test che guardasse solo l'hover passerebbe anche se l'hover e il click leggessero due regole. I casi:
 * - **pianificabili**: in portata, con un altro nemico noto in mezzo, oltre la portata;
 * - **rifiutato dal terreno**: il click non pianifica e non ispeziona, l'hover non ha niente da annunciare — ma non mostra l'arco;
 * - **fuori dalla regola**: carica non armata (una in ricarica non si arma da `#3830`: non e' piu' un caso), un'alleata, un nemico che la squadra non conosce (il velo non ha
 *   segni: [D-225]), e il nemico su cui la carica e' GIA' pianificata (il secondo click lo ispeziona).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerChargeHoverTest,
	"RefactorTactics.PlayerInput.HoverOnAnEnemyWithTheChargeArmedAnnouncesWhatTheClickWillPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerChargeHoverTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTHexMapActor* MapActor = SpawnCleanInteractionMap(World, /*Radius=*/ 8);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTUnit* Charger = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!MapActor || !TM || !Charger || !PC) { DestroyInteractionWorld(World); return false; }

	const int32 ChargeIdx = FindChargeAbilityIndex(Charger);
	const int32 Portata = Charger->EffectiveDashRangeOf(ChargeIdx);
	if (!TestTrue(TEXT("premessa: la carica ha una portata di almeno tre celle, e la mappa ne contiene due in piu'"),
		ChargeIdx != INDEX_NONE && Portata >= 3 && Portata + 2 <= 8)) { DestroyInteractionWorld(World); return false; }

	// Quattro nemici su tre rette DAVANTI a chi carica (E, SE, NE): alle spalle la squadra non vede, e un nemico non visto non
	// entra nella fotografia di chi pianifica. `Vicino`/`Dietro` stanno sulla stessa retta: il primo e' la carica «in portata»,
	// il secondo quella con un altro nemico noto in mezzo.
	ARTUnit* Vicino = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 2, 0));
	ARTUnit* Dietro = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(0, 3, 0));
	ARTUnit* Lontano = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(Portata + 2, 0, 0));
	ARTUnit* Ostacolato = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(3, -3, 0));
	ARTUnit* Alleata = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-3, 3, 0));
	if (!Vicino || !Dietro || !Lontano || !Ostacolato || !Alleata) { DestroyInteractionWorld(World); return false; }
	Alleata->bIsBotControlled = true;
	MakeInteractionRough(MapActor->MapAsset, FRTCellId(2, -2, 0)); // nega la corsa verso `Ostacolato`, e lascia passare la vista
	TM->RefreshTeamKnowledgeNow();
	// Un nemico che il velo nasconde: il flag che il click legge (`IsKnownToObserver`) lo scrive `ARTHUD::UpdateObserverVeil`,
	// e in un test si dichiara a mano. Il default e' vero, ed e' per questo che i nemici sopra sono noti senza altro.
	ARTUnit* Ignoto = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, -3, 0));
	if (!Ignoto) { DestroyInteractionWorld(World); return false; }
	Ignoto->SetKnownToObserver(false);
	// ⚠️ La conoscenza che conta e' DOPPIA e va provata su entrambe: la fotografia di chi pianifica (`ClassifyTarget`, che dipende
	// da cosa la squadra vede) e il flag del velo (`IsKnownToObserver`, che il click legge e che qui si dichiara a mano).
	for (const ARTUnit* Nemico : { Vicino, Dietro, Lontano, Ostacolato })
	{
		TestTrue(TEXT("premessa: la squadra vede il nemico, e il velo non lo nasconde"),
			URTTeamKnowledgeLibrary::ClassifyTarget(TM->KnowledgeForTeamPublic(0), Nemico->StableUnitId, Nemico->TeamId,
				Nemico->Cell) == ERTTargetKnowledge::Allowed && Nemico->IsKnownToObserver());
	}
	TestFalse(TEXT("premessa: il velo nasconde l'ignoto"), Ignoto->IsKnownToObserver());

	PC->SelectActorForTest(Charger);
	Charger->SelectAbility(ChargeIdx);

	// Un caso: l'hover sul bersaglio, poi il click, poi il confronto. `bPianificabile` e' cio' che la regola vuole.
	const auto Confronta = [&](ARTUnit* Bersaglio, bool bPianificabile, const TCHAR* Caso)
	{
		MapActor->SetHoveredCell(Bersaglio->Cell, /*bValid=*/ true);
		TestTrue(*FString::Printf(TEXT("%s: la carica armata punta il nemico sotto il cursore"), Caso),
			PC->HoveredArmedChargeTarget() == Bersaglio);
		FRTIntentView Annuncio;
		const bool bHover = PC->BuildChargeHoverView(Annuncio);
		TestEqual(*FString::Printf(TEXT("%s: l'hover annuncia una carica solo se il click la pianifica"), Caso), bHover, bPianificabile);

		PC->DispatchUnitClickForTest(Bersaglio, Charger);
		const bool bPianificata = Charger->GetPlan().PlannedDashAbility() == ChargeIdx && Charger->GetPlan().PlannedDashCell() == Bersaglio->Cell;
		TestEqual(*FString::Printf(TEXT("%s: l'hover e il click dicono la stessa cosa"), Caso), bHover, bPianificata);

		FRTChargeRun Scritta;
		if (bHover && bPianificata && TestTrue(*FString::Printf(TEXT("%s: il click ha scritto la lettura"), Caso),
			Charger->GetPlannedChargeRun(Scritta)))
		{
			TestEqual(*FString::Printf(TEXT("%s: stesse celle di corsa"), Caso), Annuncio.DashRunCells, Scritta.RunCells);
			TestEqual(*FString::Printf(TEXT("%s: stesso mancato arrivo"), Caso), Annuncio.bDashMissesTarget, Scritta.bMissesTarget);
			TestTrue(*FString::Printf(TEXT("%s: la vista parte da chi carica e punta al bersaglio"), Caso),
				Annuncio.bDashing && Annuncio.OwnerCell == Charger->Cell && Annuncio.DashCell == Bersaglio->Cell
				&& Annuncio.DashStyle == ERTMovementStyle::LinearCharge);
		}
		// Si toglie il piano: il caso dopo parte da una carica armata e non ancora pianificata.
		Charger->ClearPlannedDash();
		Charger->SelectAbility(ChargeIdx);
		return Annuncio;
	};

	const FRTIntentView InPortataView = Confronta(Vicino, true, TEXT("in portata, a contatto"));
	TestEqual(TEXT("in portata: percorre la cella prima di lui"), InPortataView.DashRunCells, 1);
	TestFalse(TEXT("in portata: e non lo manca"), InPortataView.bDashMissesTarget);

	const FRTIntentView InMezzoView = Confronta(Dietro, true, TEXT("con un altro nemico noto in mezzo"));
	TestEqual(TEXT("in mezzo: si ferma davanti ad A, dopo una cella"), InMezzoView.DashRunCells, 1);
	TestTrue(TEXT("in mezzo: e manca il bersaglio"), InMezzoView.bDashMissesTarget);

	const FRTIntentView OltreView = Confronta(Lontano, true, TEXT("oltre la portata"));
	TestEqual(TEXT("oltre la portata: corre per tutta la portata"), OltreView.DashRunCells, Portata);
	TestTrue(TEXT("oltre la portata: e manca il bersaglio"), OltreView.bDashMissesTarget);

	Confronta(Ostacolato, false, TEXT("dietro un terreno che nega la corsa"));

	// Fuori dalla regola: l'hover non punta nessuno.
	MapActor->SetHoveredCell(Alleata->Cell, true);
	TestTrue(TEXT("un'alleata non e' un bersaglio: l'hover resta un'ispezione"), PC->HoveredArmedChargeTarget() == nullptr);
	MapActor->SetHoveredCell(Ignoto->Cell, true);
	TestTrue(TEXT("un nemico che la squadra non conosce non ha segni"), PC->HoveredArmedChargeTarget() == nullptr);
	FRTIntentView Niente;
	TestFalse(TEXT("e non annuncia niente"), PC->BuildChargeHoverView(Niente));

	// Il nemico su cui la carica e' GIA' pianificata: il secondo click lo ispeziona, quindi l'hover non lo annuncia piu'.
	PC->DispatchUnitClickForTest(Vicino, Charger);
	MapActor->SetHoveredCell(Vicino->Cell, true);
	TestTrue(TEXT("controllo: la carica e' pianificata su di lui"), Charger->GetPlan().PlannedDashCell() == Vicino->Cell);
	TestTrue(TEXT("gia' pianificata: l'hover non lo punta"), PC->HoveredArmedChargeTarget() == nullptr);
	MapActor->SetHoveredCell(Dietro->Cell, true);
	TestTrue(TEXT("ma un altro nemico si'"), PC->HoveredArmedChargeTarget() == Dietro);
	Charger->ClearPlannedDash();

	// Carica non armata.
	Charger->SelectAbility(INDEX_NONE);
	MapActor->SetHoveredCell(Vicino->Cell, true);
	TestTrue(TEXT("senza la carica armata l'hover ispeziona, come prima"), PC->HoveredArmedChargeTarget() == nullptr);
	TestFalse(TEXT("e non annuncia una carica"), PC->BuildChargeHoverView(Niente));

	DestroyInteractionWorld(World);
	return true;
}

/**
 * [D-528] (`#3769`) — **sul nemico che la carica armata punta, l'hover non mostra il suo arco**: l'arco e' l'annotazione
 * pubblica di un'ispezione, e il click non ispezionera' — pianifichera' la carica, o la rifiutera' (D-516 (2): *«rifiutata, e
 * nessuna ispezione»*). Il controllo positivo e' lo stesso nemico senza la carica armata, dove l'arco c'e'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerChargeHoverArcTest,
	"RefactorTactics.PlayerInput.HoverOnAnEnemyWithTheChargeArmedDoesNotShowItsArc",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerChargeHoverArcTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTHexMapActor* MapActor = SpawnCleanInteractionMap(World, /*Radius=*/ 6);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTUnit* Charger = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0, 0));
	ARTUnit* Nemico = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(3, 0, 0));
	ARTUnit* Ostacolato = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(3, -3, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!MapActor || !TM || !Charger || !Nemico || !Ostacolato || !PC) { DestroyInteractionWorld(World); return false; }
	MakeInteractionRough(MapActor->MapAsset, FRTCellId(2, -2, 0));
	TM->RefreshTeamKnowledgeNow();

	const int32 ChargeIdx = FindChargeAbilityIndex(Charger);
	if (!TestNotEqual(TEXT("premessa: la carica"), ChargeIdx, (int32)INDEX_NONE)) { DestroyInteractionWorld(World); return false; }

	// Un arco per ciascuno dei due nemici.
	FRTEnemyArcView Arco;
	Arco.Edges = URTVisibilityBorderLibrary::ExposedEdgesFacingMapCells(MapActor->MapAsset, { Nemico->Cell });
	Arco.Color = FColor(255, 51, 38);
	TMap<FRTCellId, FRTEnemyArcView> Archi;
	Archi.Add(Nemico->Cell, Arco);
	Archi.Add(Ostacolato->Cell, Arco);
	MapActor->SetEnemyArcs(Archi);

	PC->SelectActorForTest(Charger);

	// Controllo positivo: senza la carica armata, l'arco del nemico sotto il cursore si vede.
	Charger->SelectAbility(INDEX_NONE);
	MapActor->SetHoveredCell(Nemico->Cell, true);
	PC->UpdateEnemyArcFocusForTest(MapActor);
	TestEqual(TEXT("controllo: senza la carica armata l'hover mostra l'arco"), MapActor->GetShownEnemyArcs().Num(), 1);

	// Con la carica armata: il click pianifichera', l'hover non ispeziona.
	Charger->SelectAbility(ChargeIdx);
	PC->UpdateEnemyArcFocusForTest(MapActor);
	TestEqual(TEXT("con la carica armata l'hover non mostra l'arco"), MapActor->GetShownEnemyArcs().Num(), 0);

	// Anche dove il click rifiuterebbe: non ispeziona neppure li'.
	MapActor->SetHoveredCell(Ostacolato->Cell, true);
	PC->UpdateEnemyArcFocusForTest(MapActor);
	TestEqual(TEXT("neppure dove la carica sarebbe rifiutata"), MapActor->GetShownEnemyArcs().Num(), 0);

	// Una cella vuota: niente da puntare, e nessun arco da mostrare.
	MapActor->SetHoveredCell(FRTCellId(1, 1, 0), true);
	PC->UpdateEnemyArcFocusForTest(MapActor);
	TestEqual(TEXT("una cella vuota non ha arco"), MapActor->GetShownEnemyArcs().Num(), 0);

	// Pianificata sul nemico: il secondo click lo ispeziona, quindi l'hover riprende a mostrare l'arco.
	PC->DispatchUnitClickForTest(Nemico, Charger);
	MapActor->SetHoveredCell(Nemico->Cell, true);
	PC->UpdateEnemyArcFocusForTest(MapActor);
	TestEqual(TEXT("sul nemico gia' puntato l'hover e' di nuovo un'ispezione, e l'arco c'e'"), MapActor->GetShownEnemyArcs().Num(), 1);

	DestroyInteractionWorld(World);
	return true;
}

/**
 * [D-528] (`#3769`) — **l'avviso di piano non nomina un nemico che la squadra non conosce.** Lo ha trovato la revisione indipendente di #3821.
 *
 * Il click su una cella DIETRO un nemico noto A si accetta (la carica urta A), e l'avviso dice «Colpira' A, non B». B si cercava per cella fra
 * TUTTE le unita': se la cella e' di un nemico H che la squadra non conosce, l'avviso ne scriveva il nome, e chi prova le celle dietro un nemico
 * noto legge chi c'e'. Ora il bersaglio si nomina solo se la lettura dice che e' un nemico noto, e i nomi si cercano fra i soli noti al velo.
 *
 * Due mondi che differiscono per UNA cosa: H e' ignoto, o H e' noto. Nel secondo il nome c'e': senza il controllo positivo il test passerebbe
 * anche se l'avviso non nominasse mai nessuno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerChargeWarningPrivacyTest,
	"RefactorTactics.PlayerInput.ChargeWarningDoesNotNameAnEnemyTheSquadDoesNotKnow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerChargeWarningPrivacyTest::RunTest(const FString&)
{
	struct FAvviso
	{
		int32 Quanti = INDEX_NONE;
		FString Perche;
		FString Costo;
		FString NomeA;
		FString NomeH;
		bool bPianificata = false;
	};

	const auto Leggi = [this](bool bHNoto, bool bVeloLoNasconde) -> FAvviso
	{
		FAvviso L;
		UWorld* World = MakeInteractionWorld();
		if (!TestNotNull(TEXT("world di prova"), World)) { return L; }
		ARTHexMapActor* MapActor = SpawnCleanInteractionMap(World, /*Radius=*/ 6);
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		ARTUnit* Charger = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0, 0));
		ARTUnit* A = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(2, 0, 0));
		ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
		if (!MapActor || !TM || !Charger || !A || !PC) { DestroyInteractionWorld(World); return L; }
		// H dopo l'aggiornamento della conoscenza: non e' nella fotografia di chi pianifica, e il velo lo nasconde. Se e' noto, si aggiorna dopo.
		TM->RefreshTeamKnowledgeNow();
		ARTUnit* H = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(3, 0, 0));
		if (!H) { DestroyInteractionWorld(World); return L; }
		if (bHNoto)
		{
			TM->RefreshTeamKnowledgeNow();
		}
		else if (bVeloLoNasconde)
		{
			H->SetKnownToObserver(false); // il velo lo nasconde; senza questa riga resta ignoto alla sola conoscenza di squadra
		}

		L.NomeA = ARTUnit::DisplayLabel(A->HeroDisplayName, A->HeroId, A->GetName());
		L.NomeH = ARTUnit::DisplayLabel(H->HeroDisplayName, H->HeroId, H->GetName());
		const int32 ChargeIdx = FindChargeAbilityIndex(Charger);
		if (!TestNotEqual(TEXT("premessa: la carica"), ChargeIdx, (int32)INDEX_NONE)) { DestroyInteractionWorld(World); return L; }
		PC->SelectActorForTest(Charger);
		Charger->SelectAbility(ChargeIdx);
		PC->HandleClickOnCellForTest(FRTCellId(3, 0, 0));
		L.bPianificata = Charger->GetPlan().PlannedDashAbility() == ChargeIdx && Charger->GetPlan().PlannedDashCell() == FRTCellId(3, 0, 0);

		const FName CaricaId = Charger->GetAbility(ChargeIdx)->Def.ActionId;
		const TArray<FRTPlanWarningView> Avvisi = URTHudViewModel::BuildPlanWarnings(Charger, { Charger, A, H });
		L.Quanti = 0;
		for (const FRTPlanWarningView& V : Avvisi)
		{
			if (V.SourceActionId == CaricaId)
			{
				++L.Quanti;
				L.Perche = V.Why.ToString();
				L.Costo = V.Cost.ToString();
			}
		}
		DestroyInteractionWorld(World);
		return L;
	};

	// ⚠️ Un H spawnato dopo l'aggiornamento ma col velo ACCESO non e' un caso di «ignoto»: la fotografia di chi pianifica lo vede (misurato:
	// l'avviso lo nominava, ed era giusto), quindi la sua ignoranza si dichiara col flag del velo. Il bersaglio e il filtro dei nomi
	// partono dalla stessa conoscenza, e questo test misura la loro somma: tolti tutti e due, il nome di H esce.
	const FAvviso Ignoto = Leggi(/*bHNoto=*/ false, /*bVeloLoNasconde=*/ true);
	const FAvviso Noto = Leggi(/*bHNoto=*/ true, /*bVeloLoNasconde=*/ false);

	TestTrue(TEXT("premessa: la carica dietro il nemico noto si pianifica anche con H ignoto"), Ignoto.bPianificata);
	TestEqual(TEXT("H ignoto: un solo avviso, quello di A"), Ignoto.Quanti, 1);
	TestTrue(TEXT("premessa: i due nomi sono diversi, o il controllo non distingue"), Ignoto.NomeA != Ignoto.NomeH);
	TestTrue(TEXT("H ignoto: nomina A"), Ignoto.Costo.Contains(Ignoto.NomeA));
	TestFalse(TEXT("H ignoto: NON nomina H nel costo"), Ignoto.Costo.Contains(Ignoto.NomeH));
	TestFalse(TEXT("H ignoto: ne' nel perche'"), Ignoto.Perche.Contains(Ignoto.NomeH));

	// Controllo positivo: con H noto il nome c'e'.
	TestEqual(TEXT("controllo, H noto: un solo avviso"), Noto.Quanti, 1);
	TestTrue(TEXT("controllo, H noto: nomina A e H"), Noto.Costo.Contains(Noto.NomeA) && Noto.Costo.Contains(Noto.NomeH));
	return true;
}

/**
 * IL CLICK FISSA LA SCHEDA E IL TASTO DESTRO LA CHIUDE (`#3748`, Q5/Q6). Prima nessuno accendeva `bInspectorPinned`, e
 * `ApplyBack` lo spegneva senza azzerare il soggetto: la scheda non si sarebbe chiusa mai.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInspectionPinsAndBackClosesTest,
	"RefactorTactics.PlayerInput.InspectingPinsAndRightClickCloses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInspectionPinsAndBackClosesTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	ARTUnit* Enemy = World->SpawnActor<ARTUnit>();
	if (!PC || !Enemy)
	{
		RTWorldFixtures::DestroyWorld(World);
		return TestTrue(TEXT("controller e unita' esistono"), false);
	}
	Enemy->TeamId = 1;

	TestFalse(TEXT("all'inizio non e' fissata"), PC->IsInspectorPinned());
	PC->DispatchUnitClickForTest(Enemy, nullptr);
	TestTrue(TEXT("il click su un nemico osservato lo ispeziona"), PC->GetInspectedUnit() == Enemy);
	TestTrue(TEXT("e fissa la scheda"), PC->IsInspectorPinned());

	TestEqual(TEXT("il Back chiude la scheda"), PC->ApplyBack(), ERTPointerBackStep::Inspector);
	TestFalse(TEXT("il pin e' spento"), PC->IsInspectorPinned());
	TestTrue(TEXT("e il soggetto e' azzerato"), PC->GetInspectedUnit() == nullptr);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

// ── E38 CP 38.10 (#4111): il lock-in svuota un piano oltre il budget o in ricarica ([D-615] (1), [D-617]) ───────────
//
// Un banco solo: Riktor in (0,0), squadra 0; Gadget accanto in (-1,0), squadra 1, a distanza 1, cioe' dentro il canale
// ravvicinato della vista: per la squadra 1 Riktor e' visibile, ed e' la premessa che rende non vacui i due test di
// privacy. Gadget non ha piano, quindi nessun'altra unita' agisce su Riktor (criterio 1).
namespace
{
	int32 LockInAbilityIndex(const ARTUnit* U, const TCHAR* Id)
	{
		for (int32 i = 0; U && U->GetAbility(i); ++i)
		{
			if (U->GetAbility(i)->Def.ActionId == FName(Id)) { return i; }
		}
		return INDEX_NONE;
	}

	struct FLockInBench
	{
		UWorld* World = nullptr;
		ARTUnit* Riktor = nullptr;
		ARTUnit* Gadget = nullptr;
		ARTPlayerController* PC = nullptr;
		ARTTurnManager* TM = nullptr;
		int32 Ram = INDEX_NONE;
		int32 Impact = INDEX_NONE;
		int32 Interposition = INDEX_NONE;
		int32 Mortar = INDEX_NONE;
	};

	bool MakeLockInBench(FAutomationTestBase& Test, FLockInBench& B)
	{
		B.World = MakeInteractionWorld();
		if (!Test.TestNotNull(TEXT("world di prova"), B.World)) { return false; }
		if (!Test.TestNotNull(TEXT("mappa senza ostacoli"), SpawnCleanInteractionMap(B.World, /*Radius=*/ 6))) { return false; }
		B.Riktor = SpawnInteractionUnit(B.World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0));
		B.Gadget = SpawnInteractionUnit(B.World, 1, URTHeroCatalogLibrary::MakeGadget(), FRTCellId(-1, 0));
		B.PC = B.World->SpawnActor<ARTPlayerController>();
		B.TM = B.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Test.TestNotNull(TEXT("Riktor"), B.Riktor) || !Test.TestNotNull(TEXT("Gadget"), B.Gadget)
			|| !Test.TestNotNull(TEXT("controller"), B.PC) || !Test.TestNotNull(TEXT("turn manager"), B.TM))
		{
			return false;
		}
		B.Ram = LockInAbilityIndex(B.Riktor, TEXT("Hero.Riktor.Ram"));
		B.Impact = LockInAbilityIndex(B.Riktor, TEXT("Hero.Riktor.ImpactShot"));
		B.Interposition = LockInAbilityIndex(B.Riktor, TEXT("Hero.Riktor.Interposition"));
		B.Mortar = LockInAbilityIndex(B.Riktor, TEXT("Hero.Riktor.MortarShot"));
		return Test.TestTrue(TEXT("premessa: il kit di Riktor ha Ram, ImpactShot, Interposition e MortarShot"),
			B.Ram != INDEX_NONE && B.Impact != INDEX_NONE && B.Interposition != INDEX_NONE && B.Mortar != INDEX_NONE);
	}

	/** Ram verso (2,0), ImpactShot su Gadget, Interposition, Move verso (0,2), verso NE: 2 + 2 + 2 + 2 = 8 > 6. */
	void PlanOverBudget(FLockInBench& B)
	{
		B.Riktor->SetPlannedDash(B.Ram, FRTCellId(2, 0));
		B.Riktor->SetPlannedMainAction(B.Impact);
		B.Riktor->DeclareAttackOnUnit(B.Gadget);
		B.Riktor->SetPlannedReaction(B.Interposition);
		B.Riktor->SetPlannedMove({ FRTCellId(0, 2) }, { FRTCellId(0, 0), FRTCellId(0, 1), FRTCellId(0, 2) });
		B.Riktor->DeclarePlannedFacing(ERTHexDirection::NE);
	}

	FRTPlanValidation LockInVerdictOf(const ARTUnit* U)
	{
		return URTPlanValidationLibrary::ValidatePlan(FRTHexSimUnit(), URTPlanValidationLibrary::MakePlanFor(U));
	}

	void ResolveOneTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }
	}

	int32 CountUnitEntries(const TArray<FRTTurnLogEntry>& Log, int32 UnitId, ERTLogCategory Category, uint8 Outcome)
	{
		int32 N = 0;
		for (const FRTTurnLogEntry& E : Log)
		{
			if (E.UnitId == UnitId && E.Category == Category && E.Outcome == Outcome) { ++N; }
		}
		return N;
	}

	int32 CountLinesWith(const TArray<FString>& Lines, const TCHAR* Text)
	{
		int32 N = 0;
		for (const FString& L : Lines) { if (L.Contains(Text)) { ++N; } }
		return N;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLockInEmptiesAPlanOverBudgetTest,
	"RefactorTactics.Plan.LockInEmptiesAPlanOverBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLockInEmptiesAPlanOverBudgetTest::RunTest(const FString&)
{
	FLockInBench B;
	if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
	PlanOverBudget(B);
	const FRTPlanValidation Verdetto = LockInVerdictOf(B.Riktor);
	if (!TestTrue(TEXT("premessa: il piano e' OverBudget (8 > 6)"),
			!Verdetto.bLegal && Verdetto.Reason == ERTActionInvalidReason::OverBudget))
	{
		DestroyInteractionWorld(B.World); return false;
	}
	const ERTHexDirection VersoPrima = B.Riktor->Facing;

	ResolveOneTurn(B.TM);

	const TArray<FRTTurnLogEntry>& Log = B.TM->GetTurnLog();
	const int32 Id = B.Riktor->StableUnitId;
	TestNotEqual(TEXT("premessa: Riktor ha uno StableUnitId"), Id, 0);

	// Criterio 2: una `Fallback`/`Cancelled` per voce del validatore, col motivo in `Amount`.
	const uint8 Annullata = static_cast<uint8>(ERTFallbackOutcome::Cancelled);
	TestEqual(TEXT("una voce Fallback/Cancelled per ognuna delle quattro voci del piano"),
		CountUnitEntries(Log, Id, ERTLogCategory::Fallback, Annullata), 4);
	for (const FRTTurnLogEntry& E : Log)
	{
		if (E.UnitId != Id || E.Category != ERTLogCategory::Fallback) { continue; }
		TestEqual(*FString::Printf(TEXT("%s: Amount = OverBudget"), *E.ActionId.ToString()),
			E.Amount, static_cast<int32>(ERTActionInvalidReason::OverBudget));
		TestTrue(*FString::Printf(TEXT("%s: privata alla squadra (OwnerTeam)"), *E.ActionId.ToString()),
			E.VerdictSubject.PlanVerdict == ERTPlanVerdict::OwnerTeam);
	}

	// Criterio 1: nessun'altra voce di Riktor, salvo la `Move`/`Stayed` che riceve chiunque resti fermo.
	for (const FRTTurnLogEntry& E : Log)
	{
		if (E.UnitId != Id) { continue; }
		const bool bAmmessa = E.Category == ERTLogCategory::Fallback
			|| (E.Category == ERTLogCategory::Move && E.Outcome == static_cast<uint8>(ERTMoveOutcome::Stayed));
		TestTrue(*FString::Printf(TEXT("voce di Riktor ammessa (categoria %d, esito %d, %s)"),
			static_cast<int32>(E.Category), static_cast<int32>(E.Outcome), *E.ActionId.ToString()), bAmmessa);
	}
	TestTrue(TEXT("Riktor resta in (0,0): lo scatto e il Move non si risolvono"), B.Riktor->Cell == FRTCellId(0, 0));
	TestTrue(TEXT("il verso dichiarato si azzera col piano: Riktor non gira"), B.Riktor->Facing == VersoPrima);

	DestroyInteractionWorld(B.World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLockInEmptiesAPlanOnCooldownTest,
	"RefactorTactics.Plan.LockInEmptiesAPlanOnCooldown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLockInEmptiesAPlanOnCooldownTest::RunTest(const FString&)
{
	FLockInBench B;
	if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }

	// MortarShot in ricarica (cooldown 3), nel piano con la reazione e un verso: 2 + 2 = 4, quindi il motivo e' solo la ricarica.
	B.Riktor->ConsumeAbility(B.Mortar);
	B.Riktor->SetPlannedMainAction(B.Mortar);
	B.Riktor->DeclareAttackOnCell(FRTCellId(-3, 0));
	B.Riktor->SetPlannedReaction(B.Interposition);
	B.Riktor->DeclarePlannedFacing(ERTHexDirection::NE);
	const FRTPlanValidation Verdetto = LockInVerdictOf(B.Riktor);
	if (!TestTrue(TEXT("premessa: MortarShot e' in ricarica"), B.Riktor->GetAbilityCooldown(B.Mortar) > 0)
		|| !TestTrue(TEXT("premessa: il piano e' OnCooldown"),
			!Verdetto.bLegal && Verdetto.Reason == ERTActionInvalidReason::OnCooldown))
	{
		DestroyInteractionWorld(B.World); return false;
	}
	const ERTHexDirection VersoPrima = B.Riktor->Facing;

	ResolveOneTurn(B.TM);

	const TArray<FRTTurnLogEntry>& Log = B.TM->GetTurnLog();
	const int32 Id = B.Riktor->StableUnitId;
	TestEqual(TEXT("una Fallback/Cancelled per la principale e una per la reazione"),
		CountUnitEntries(Log, Id, ERTLogCategory::Fallback, static_cast<uint8>(ERTFallbackOutcome::Cancelled)), 2);
	for (const FRTTurnLogEntry& E : Log)
	{
		if (E.UnitId != Id) { continue; }
		if (E.Category == ERTLogCategory::Fallback)
		{
			TestEqual(*FString::Printf(TEXT("%s: Amount = OnCooldown"), *E.ActionId.ToString()),
				E.Amount, static_cast<int32>(ERTActionInvalidReason::OnCooldown));
			continue;
		}
		TestTrue(*FString::Printf(TEXT("nessuna voce di Riktor oltre la Move/Stayed (categoria %d, %s)"),
			static_cast<int32>(E.Category), *E.ActionId.ToString()),
			E.Category == ERTLogCategory::Move && E.Outcome == static_cast<uint8>(ERTMoveOutcome::Stayed));
	}
	TestTrue(TEXT("il verso dichiarato si azzera col piano"), B.Riktor->Facing == VersoPrima);

	DestroyInteractionWorld(B.World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLockInLeavesASlotConflictToTheResolverTest,
	"RefactorTactics.Plan.LockInLeavesASlotConflictToTheResolver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLockInLeavesASlotConflictToTheResolverTest::RunTest(const FString&)
{
	// ⏱️ *Fino a CP 38.16 (criterio 3 di CP 38.10): scatto + Move era `SlotOccupied`, registrato e non bloccato, e il resolver scartava il
	// Move con `SupersededByDash`.* Da CP 38.16 il piano e' legale ([D-584] (1)), il lock-in non lo tocca, e il Move parte dalla cella
	// d'arrivo dello scatto. Il percorso qui parte dalla cella di prima, come lo scriveva un piano del vecchio modello: il resolver lo
	// ricostruisce verso `PlannedCell` da dove lo scatto ha portato l'unita'. Il nome del test e' storico.
	FLockInBench B;
	if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
	B.Riktor->SetPlannedDash(B.Ram, FRTCellId(2, 0));
	B.Riktor->SetPlannedMove({ FRTCellId(0, 2) }, { FRTCellId(0, 0), FRTCellId(0, 1), FRTCellId(0, 2) });
	const FRTPlanValidation Verdetto = LockInVerdictOf(B.Riktor);
	if (!TestTrue(TEXT("premessa: scatto + Move e' legale"), Verdetto.bLegal))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	ResolveOneTurn(B.TM);

	const int32 Id = B.Riktor->StableUnitId;
	int32 FallbackDelLockIn = 0;
	for (const FRTTurnLogEntry& E : B.TM->GetTurnLog())
	{
		if (E.UnitId == Id && E.Category == ERTLogCategory::Fallback
			&& (E.Amount == static_cast<int32>(ERTActionInvalidReason::SlotOccupied)
				|| E.Amount == static_cast<int32>(ERTActionInvalidReason::OverBudget)
				|| E.Amount == static_cast<int32>(ERTActionInvalidReason::OnCooldown)))
		{
			++FallbackDelLockIn;
		}
	}
	TestEqual(TEXT("il lock-in non svuota un piano legale"), FallbackDelLockIn, 0);
	TestEqual(TEXT("il resolver non scarta il Move"),
		CountUnitEntries(B.TM->GetTurnLog(), Id, ERTLogCategory::Move, static_cast<uint8>(ERTMoveOutcome::SupersededByDash)), 0);
	TestEqual(TEXT("nessuna riga di piano non valido"),
		CountLinesWith(B.TM->GetRecentEventsForTeam(0), TEXT("piano non valido al lock-in")), 0);
	TestEqual(TEXT("e Riktor finisce dove il Move lo porta, dopo lo scatto"), B.Riktor->Cell, FRTCellId(0, 2));

	DestroyInteractionWorld(B.World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReadyIsRefusedWhileThePlanBreaksTheBudgetTest,
	"RefactorTactics.PlayerInteraction.ReadyIsRefusedWhileThePlanBreaksTheBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReadyIsRefusedWhileThePlanBreaksTheBudgetTest::RunTest(const FString&)
{
	// Criterio 4: il Ready di un piano OverBudget si rifiuta; tolto il Move, lo stesso Ready passa.
	FLockInBench B;
	if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
	PlanOverBudget(B);
	const int32 Gruppo = B.Riktor->ControlGroup;
	const int32 TurnoPrima = B.TM->GetTurnNumber();

	B.TM->DeclareParticipantReady(0, Gruppo);
	TestFalse(TEXT("il Ready di un piano oltre il budget e' rifiutato"), B.TM->IsParticipantReady(0, Gruppo));
	TestFalse(TEXT("e non arma il countdown"), B.TM->IsReadyCountdownActive());
	TestEqual(TEXT("il turno non si chiude"), B.TM->GetTurnNumber(), TurnoPrima);
	TestEqual(TEXT("il rifiuto lo dice nel combat log della squadra"),
		CountLinesWith(B.TM->GetRecentEventsForTeam(0), TEXT("Ready rifiutato")), 1);

	// Controllo positivo: lo stesso piano senza il Move costa 6, e il Ready passa.
	B.Riktor->ClearPlannedMove();
	if (!TestTrue(TEXT("premessa: senza il Move il piano e' legale"), LockInVerdictOf(B.Riktor).bLegal))
	{
		DestroyInteractionWorld(B.World); return false;
	}
	B.TM->DeclareParticipantReady(0, Gruppo);
	TestTrue(TEXT("senza il Move il Ready e' accettato"),
		B.TM->IsParticipantReady(0, Gruppo) || B.TM->IsReadyCountdownActive() || B.TM->GetTurnNumber() != TurnoPrima);
	TestEqual(TEXT("e non lascia un secondo rifiuto"),
		CountLinesWith(B.TM->GetRecentEvents(), TEXT("Ready rifiutato")), 1);

	DestroyInteractionWorld(B.World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRefusedPlanStaysWithItsTeamTest,
	"RefactorTactics.Replay.Privacy.RefusedPlanStaysWithItsTeam",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRefusedPlanStaysWithItsTeamTest::RunTest(const FString&)
{
	// Criterio 7: le voci del piano svuotato non arrivano alla traccia dell'osservatore avversario.
	FLockInBench B;
	if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
	PlanOverBudget(B);
	ResolveOneTurn(B.TM);

	const int32 Id = B.Riktor->StableUnitId;
	const ERTTargetKnowledge Vista = URTTeamKnowledgeLibrary::ClassifyTarget(
		B.TM->KnowledgeForTeamPublic(1), Id, B.Riktor->TeamId, B.Riktor->Cell);
	if (!TestTrue(TEXT("premessa: per la squadra 1 Riktor e' visibile"), Vista == ERTTargetKnowledge::Allowed))
	{
		DestroyInteractionWorld(B.World); return false;
	}
	const uint8 Annullata = static_cast<uint8>(ERTFallbackOutcome::Cancelled);
	const TArray<FRTTurnLogEntry> PerLaSquadra = URTReplayPrivacyLibrary::FilterEntriesForObserver(B.TM->GetTurnLog(), 0);
	const TArray<FRTTurnLogEntry> PerLAvversario = URTReplayPrivacyLibrary::FilterEntriesForObserver(B.TM->GetTurnLog(), 1);
	TestEqual(TEXT("controllo positivo: la traccia della squadra 0 ha le quattro voci"),
		CountUnitEntries(PerLaSquadra, Id, ERTLogCategory::Fallback, Annullata), 4);
	TestEqual(TEXT("la traccia della squadra 1 non ne ha nessuna"),
		CountUnitEntries(PerLAvversario, Id, ERTLogCategory::Fallback, Annullata), 0);

	DestroyInteractionWorld(B.World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBackReopensTheReadyAfterAnOverBudgetPlanTest,
	"RefactorTactics.PlayerInteraction.BackReopensTheReadyAfterAnOverBudgetPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBackReopensTheReadyAfterAnOverBudgetPlanTest::RunTest(const FString&)
{
	// Criterio 8: con uno scatto pianificato, che il Back non toglie, il piano oltre il budget si accorcia lo stesso.
	FLockInBench B;
	if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
	B.PC->SelectActorForTest(B.Riktor);
	PlanOverBudget(B);
	const int32 Gruppo = B.Riktor->ControlGroup;
	B.TM->DeclareParticipantReady(0, Gruppo);
	if (!TestFalse(TEXT("premessa: il Ready del piano oltre il budget e' rifiutato"), B.TM->IsParticipantReady(0, Gruppo)))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	for (int32 Passi = 0; Passi < 4 && !LockInVerdictOf(B.Riktor).bLegal; ++Passi)
	{
		B.PC->ApplyBack();
	}
	TestTrue(TEXT("il Back porta il piano entro il budget"), LockInVerdictOf(B.Riktor).bLegal);
	TestEqual(TEXT("lo scatto resta pianificato: il Back ha tolto un'altra voce"), B.Riktor->GetPlan().PlannedDashAbility(), B.Ram);

	const int32 TurnoPrima = B.TM->GetTurnNumber();
	B.TM->DeclareParticipantReady(0, Gruppo);
	TestTrue(TEXT("dopo il Back il Ready e' accettato"),
		B.TM->IsParticipantReady(0, Gruppo) || B.TM->IsReadyCountdownActive() || B.TM->GetTurnNumber() != TurnoPrima);

	DestroyInteractionWorld(B.World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLockInLineStaysWithItsTeamTest,
	"RefactorTactics.PlayerInteraction.LockInLineStaysWithItsTeam",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLockInLineStaysWithItsTeamTest::RunTest(const FString&)
{
	// Criterio 9: la riga del lock-in racconta un piano, quindi la legge solo la squadra che lo ha composto ([D-349]).
	//
	// 🔴 **Prima un turno a piano vuoto**, e non e' cerimonia: al primo lock-in la conoscenza di squadra non e' ancora
	// calcolata, e col verdetto di conoscenza la riga sarebbe nascosta a TUTTE e due le squadre. L'asserzione sulla
	// squadra 1 sarebbe vera anche col codice di prima. Misurato con la mutazione che rimette `FRTLogSubject::Unit`.
	FLockInBench B;
	if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
	ResolveOneTurn(B.TM);
	const ERTTargetKnowledge Vista = URTTeamKnowledgeLibrary::ClassifyTarget(
		B.TM->KnowledgeForTeamPublic(1), B.Riktor->StableUnitId, B.Riktor->TeamId, B.Riktor->Cell);
	if (!TestTrue(TEXT("premessa: al lock-in del turno 2 la squadra 1 vede Riktor"), Vista == ERTTargetKnowledge::Allowed))
	{
		DestroyInteractionWorld(B.World); return false;
	}

	PlanOverBudget(B);
	ResolveOneTurn(B.TM);
	TestEqual(TEXT("la squadra 0 legge la riga del lock-in"),
		CountLinesWith(B.TM->GetRecentEventsForTeam(0), TEXT("piano non valido al lock-in")), 1);
	TestEqual(TEXT("la squadra 1, che vede Riktor, non la legge"),
		CountLinesWith(B.TM->GetRecentEventsForTeam(1), TEXT("piano non valido al lock-in")), 0);

	DestroyInteractionWorld(B.World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLockInScenariosPassTest,
	"RefactorTactics.Plan.LockInScenariosPass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLockInScenariosPassTest::RunTest(const FString&)
{
	// Gli scenari di CP 38.10, e `DashDiscardsPlannedMove` per il criterio 3: un piano il cui solo motivo e' lo slot si
	// risolve come prima, col Move scartato dallo scatto.
	const TCHAR* const Scenari[] = {
		TEXT("Scenarios/Spec/ActionEconomy/LockInRefusesAPlanOverBudget.json"),
		TEXT("Scenarios/Spec/ActionEconomy/LockInRefusesAnActionOnCooldown.json"),
		TEXT("Scenarios/Spec/Movement/DashDiscardsPlannedMove.json"),
	};
	for (const TCHAR* Rel : Scenari)
	{
		FRTTestScenario Scenario;
		FString Error;
		if (!TestTrue(*FString::Printf(TEXT("%s si carica: %s"), Rel, *Error),
				URTScenarioLoader::LoadFromFile(FPaths::Combine(FPaths::ProjectDir(), Rel), Scenario, Error)))
		{
			continue;
		}
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
		const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
		RTWorldFixtures::DestroyWorld(World);
		if (Result.Outcome == ERTTestOutcome::Error)
		{
			AddError(FString::Printf(TEXT("%s: ERROR invece di PASS — %s"), Rel, *Result.ErrorMessage));
			continue;
		}
		for (const FRTAssertionResult& A : Result.Assertions)
		{
			if (!A.bPassed)
			{
				AddError(FString::Printf(TEXT("%s — assertion caduta al turno %d: %s (atteso '%s', letto '%s')"),
					Rel, A.Turn, *A.Description, *A.Expected, *A.Actual));
			}
		}
		TestEqual(*FString::Printf(TEXT("%s passa"), Rel), Result.OutcomeString(), FString(TEXT("PASS")));
	}
	return true;
}

// ── E38 CP 38.11 (#4112): il budget del turno e' un numero dell'unita' ([D-613] (1)) ────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudVerdictMatchesTheLockInWithBudgetFiveTest,
	"RefactorTactics.HudViewModel.VerdictMatchesTheLockInWithBudgetFive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudVerdictMatchesTheLockInWithBudgetFiveTest::RunTest(const FString&)
{
	// Criterio 2: view-model e lock-in danno lo stesso verdetto per la stessa unita' con budget 5.
	FLockInBench B;
	if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
	B.Riktor->SetActionPointBudget(5);
	B.Riktor->SetPlannedMainAction(B.Impact);
	B.Riktor->DeclareAttackOnUnit(B.Gadget);
	B.Riktor->SetPlannedReaction(B.Interposition);
	B.Riktor->SetPlannedMove({ FRTCellId(0, 1) }, { FRTCellId(0, 0), FRTCellId(0, 1) });

	// L'HUD: l'avviso critico porta il motivo del budget.
	const FText Motivo = FText::FromString(URTTurnLogLibrary::DescribeInvalidReason(ERTActionInvalidReason::OverBudget));
	int32 Critici = 0;
	for (const FRTPlanWarningView& W : URTHudViewModel::BuildPlanWarnings(B.Riktor, { B.Riktor, B.Gadget }))
	{
		if (W.Level == ERTPlanWarningLevel::Critical && W.Why.EqualTo(Motivo)) { ++Critici; }
	}
	TestEqual(TEXT("l'HUD giudica il piano da 6 OverBudget con budget 5"), Critici, 1);

	// Il lock-in: lo stesso piano si svuota per lo stesso motivo.
	ResolveOneTurn(B.TM);
	int32 VociDelBudget = 0;
	for (const FRTTurnLogEntry& E : B.TM->GetTurnLog())
	{
		if (E.UnitId == B.Riktor->StableUnitId && E.Category == ERTLogCategory::Fallback
			&& E.Amount == static_cast<int32>(ERTActionInvalidReason::OverBudget))
		{
			++VociDelBudget;
		}
	}
	TestEqual(TEXT("il lock-in lo svuota per OverBudget: una voce per azione"), VociDelBudget, 3);

	// Controllo positivo: col budget di default lo stesso piano non accende l'avviso critico del budget.
	FLockInBench C;
	if (!MakeLockInBench(*this, C)) { DestroyInteractionWorld(B.World); DestroyInteractionWorld(C.World); return false; }
	C.Riktor->SetPlannedMainAction(C.Impact);
	C.Riktor->DeclareAttackOnUnit(C.Gadget);
	C.Riktor->SetPlannedReaction(C.Interposition);
	C.Riktor->SetPlannedMove({ FRTCellId(0, 1) }, { FRTCellId(0, 0), FRTCellId(0, 1) });
	int32 CriticiDefault = 0;
	for (const FRTPlanWarningView& W : URTHudViewModel::BuildPlanWarnings(C.Riktor, { C.Riktor, C.Gadget }))
	{
		if (W.Level == ERTPlanWarningLevel::Critical && W.Why.EqualTo(Motivo)) { ++CriticiDefault; }
	}
	TestEqual(TEXT("controllo: con budget 6 nessun avviso del budget"), CriticiDefault, 0);

	DestroyInteractionWorld(B.World);
	DestroyInteractionWorld(C.World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionPointBudgetScenariosPassTest,
	"RefactorTactics.Plan.ActionPointBudgetScenariosPass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionPointBudgetScenariosPassTest::RunTest(const FString&)
{
	const TCHAR* const Scenari[] = {
		TEXT("Scenarios/Spec/ActionEconomy/BudgetFiveRefusesASixPointPlan.json"),
		TEXT("Scenarios/Spec/ActionEconomy/DefaultBudgetAcceptsTheSamePlan.json"),
	};
	for (const TCHAR* Rel : Scenari)
	{
		FRTTestScenario Scenario;
		FString Error;
		if (!TestTrue(*FString::Printf(TEXT("%s si carica: %s"), Rel, *Error),
				URTScenarioLoader::LoadFromFile(FPaths::Combine(FPaths::ProjectDir(), Rel), Scenario, Error)))
		{
			continue;
		}
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
		const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
		RTWorldFixtures::DestroyWorld(World);
		if (Result.Outcome == ERTTestOutcome::Error)
		{
			AddError(FString::Printf(TEXT("%s: ERROR invece di PASS — %s"), Rel, *Result.ErrorMessage));
			continue;
		}
		for (const FRTAssertionResult& A : Result.Assertions)
		{
			if (!A.bPassed)
			{
				AddError(FString::Printf(TEXT("%s — assertion caduta al turno %d: %s (atteso '%s', letto '%s')"),
					Rel, A.Turn, *A.Description, *A.Expected, *A.Actual));
			}
		}
		TestEqual(*FString::Printf(TEXT("%s passa"), Rel), Result.OutcomeString(), FString(TEXT("PASS")));
	}
	return true;
}

// ── E38 CP 38.12 (#4113): il piano e' una lista; l'ordine di dichiarazione si mostra e non decide ──────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanEntriesKeepTheirOrderThroughTheMutatorsTest,
	"RefactorTactics.Plan.EntriesKeepTheirOrderThroughTheMutators",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanEntriesKeepTheirOrderThroughTheMutatorsTest::RunTest(const FString&)
{
	FLockInBench B;
	if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
	const FRTUnitPlan& P = B.Riktor->GetPlan();
	TestEqual(TEXT("premessa: il piano parte vuoto"), P.GetEntries().Num(), 0);

	// Aggiungere: la posizione e' l'ordine in cui le voci sono state dichiarate.
	B.Riktor->SetPlannedReaction(B.Interposition);
	B.Riktor->SetPlannedMove({ FRTCellId(0, 1) }, { FRTCellId(0, 0), FRTCellId(0, 1) });
	B.Riktor->SetPlannedMainAction(B.Impact);
	TestEqual(TEXT("reazione per prima"), P.DeclarationIndexOf(ERTPlanEntryClass::Reaction), 0);
	TestEqual(TEXT("movimento per secondo"), P.DeclarationIndexOf(ERTPlanEntryClass::Movement), 1);
	TestEqual(TEXT("principale per terza"), P.DeclarationIndexOf(ERTPlanEntryClass::Main), 2);

	// Il compositore del validatore porta lo stesso ordine su ogni voce.
	for (const FRTPlannedAction& A : URTPlanValidationLibrary::MakePlanFor(B.Riktor))
	{
		const int32 Atteso = A.Def.ActionId == FName(TEXT("Hero.Riktor.Interposition")) ? 0
			: A.Def.ActionId == FName(TEXT("Hero.Riktor.ImpactShot")) ? 2 : 1;
		TestEqual(*FString::Printf(TEXT("%s porta il suo ordine"), *A.Def.ActionId.ToString()), A.DeclarationIndex, Atteso);
	}

	// Sostituire non sposta: armare un'altra principale riscrive la voce dov'e'.
	B.Riktor->SetPlannedMainAction(B.Mortar);
	TestEqual(TEXT("la principale sostituita resta terza"), P.DeclarationIndexOf(ERTPlanEntryClass::Main), 2);
	TestEqual(TEXT("e porta l'abilita' nuova"), P.PlannedAbilityIndex(), B.Mortar);

	// Togliere: le altre restano nel loro ordine.
	TestTrue(TEXT("la reazione si toglie"), B.Riktor->RemovePlanEntry(0));
	TestEqual(TEXT("il movimento sale a primo"), P.DeclarationIndexOf(ERTPlanEntryClass::Movement), 0);
	TestEqual(TEXT("la principale a seconda"), P.DeclarationIndexOf(ERTPlanEntryClass::Main), 1);
	TestEqual(TEXT("e la reazione non c'e' piu'"), P.PlannedReactionAbility(), (int32)INDEX_NONE);
	TestFalse(TEXT("una posizione che non c'e' non toglie niente"), B.Riktor->RemovePlanEntry(5));

	// Una voce che torna tutta al default esce dalla lista, e riarmata torna in coda.
	B.Riktor->ClearPlannedMainAction();
	TestEqual(TEXT("la principale consumata esce"), P.DeclarationIndexOf(ERTPlanEntryClass::Main), (int32)INDEX_NONE);
	B.Riktor->SetPlannedReaction(B.Interposition);
	TestEqual(TEXT("la reazione riarmata e' in coda"), P.DeclarationIndexOf(ERTPlanEntryClass::Reaction), 1);
	DestroyInteractionWorld(B.World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanMutatorsKeepOneEntryPerClassTest,
	"RefactorTactics.Plan.MutatorsKeepOneEntryPerClass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanMutatorsKeepOneEntryPerClassTest::RunTest(const FString&)
{
	// Criterio 10: un mutatore che aggiungerebbe una seconda voce di una classe la rifiuta e lascia il piano com'era.
	FLockInBench B;
	if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
	const FRTUnitPlan& P = B.Riktor->GetPlan();
	B.Riktor->SetPlannedDash(B.Ram, FRTCellId(2, 0));
	B.Riktor->SetPlannedMainAction(B.Impact);
	B.Riktor->SetPlannedReaction(B.Interposition);
	B.Riktor->SetPlannedMove({ FRTCellId(0, 2) }, { FRTCellId(0, 0), FRTCellId(0, 1), FRTCellId(0, 2) });
	if (!TestEqual(TEXT("premessa: una voce per classe"), P.GetEntries().Num(), 4)) { DestroyInteractionWorld(B.World); return false; }
	const TArray<FRTPlanEntry> Prima = P.GetEntries();

	// E38 CP 38.14, CP 38.15 e CP 38.16: principale, reazione e scatto sono aperte; resta chiuso il movimento, che e' uno (il percorso a
	// tratti di [D-625] (1) resta una voce). Un secondo scatto entra nel piano e lo rifiuta il validatore (`SecondDash`, [D-625] (4)).
	{
		FRTPlanEntry Seconda;
		Seconda.Class = ERTPlanEntryClass::Movement;
		Seconda.AbilityIndex = B.Mortar;
		Seconda.Waypoints = { FRTCellId(1, 0) };
		TestFalse(TEXT("una seconda voce di movimento si rifiuta"), B.Riktor->AddPlanEntry(Seconda));
	}
	TestEqual(TEXT("e il piano resta com'era: quante voci"), P.GetEntries().Num(), Prima.Num());
	for (int32 i = 0; i < Prima.Num(); ++i)
	{
		TestEqual(*FString::Printf(TEXT("voce %d: stessa classe"), i), P.GetEntries()[i].Class, Prima[i].Class);
		TestEqual(*FString::Printf(TEXT("voce %d: stessa abilita'"), i), P.GetEntries()[i].AbilityIndex, Prima[i].AbilityIndex);
	}

	// E38 CP 38.16: un secondo scatto entra, in coda, e si toglie per tornare al piano di prima.
	{
		FRTPlanEntry SecondoScatto;
		SecondoScatto.Class = ERTPlanEntryClass::Dash;
		SecondoScatto.AbilityIndex = B.Ram;
		SecondoScatto.DashCell = FRTCellId(0, 2);
		TestTrue(TEXT("un secondo scatto entra"), B.Riktor->AddPlanEntry(SecondoScatto));
		TestTrue(TEXT("e si toglie"), B.Riktor->RemovePlanEntry(P.GetEntries().Num() - 1));
	}

	// Controllo: una voce di una classe che manca entra, in coda.
	TestTrue(TEXT("tolta la reazione"), B.Riktor->RemovePlanEntry(P.DeclarationIndexOf(ERTPlanEntryClass::Reaction)));
	FRTPlanEntry Reazione;
	Reazione.Class = ERTPlanEntryClass::Reaction;
	Reazione.AbilityIndex = B.Interposition;
	TestTrue(TEXT("controllo: la reazione che manca entra"), B.Riktor->AddPlanEntry(Reazione));
	TestEqual(TEXT("in coda"), P.DeclarationIndexOf(ERTPlanEntryClass::Reaction), 3);
	TestFalse(TEXT("e una voce vuota non entra"), B.Riktor->AddPlanEntry(FRTPlanEntry()));

	// E38 CP 38.15: una seconda reazione entra, in coda; `ClearReactionPlan` le toglie tutte e due, e la rimetto.
	TestTrue(TEXT("una seconda reazione entra"), B.Riktor->AddPlanEntry(Reazione));
	TestEqual(TEXT("e il piano ne porta due"), P.GetEntries().FilterByPredicate([](const FRTPlanEntry& E) { return E.Class == ERTPlanEntryClass::Reaction; }).Num(), 2);
	B.Riktor->ClearReactionPlan();
	TestEqual(TEXT("ClearReactionPlan toglie tutte le reazioni"), P.DeclarationIndexOf(ERTPlanEntryClass::Reaction), (int32)INDEX_NONE);
	TestTrue(TEXT("e la reazione torna"), B.Riktor->AddPlanEntry(Reazione));

	// E38 CP 38.14: una seconda principale entra, in coda, con un'identita' sua; la tolgo per tornare al piano di prima.
	FRTPlanEntry SecondaPrincipale;
	SecondaPrincipale.Class = ERTPlanEntryClass::Main;
	SecondaPrincipale.AbilityIndex = B.Mortar;
	TestTrue(TEXT("una seconda principale entra"), B.Riktor->AddPlanEntry(SecondaPrincipale));
	TestNotEqual(TEXT("con un'identita' sua"), P.GetEntries().Last().Id, P.GetEntries()[P.DeclarationIndexOf(ERTPlanEntryClass::Main)].Id);
	TestTrue(TEXT("e si toglie"), B.Riktor->RemovePlanEntry(P.GetEntries().Num() - 1));

	// E38 CP 38.13: la classe Prep e' aperta, e una sua voce entra anche accanto a un'altra.
	FRTPlanEntry Prep;
	Prep.Class = ERTPlanEntryClass::Prep;
	Prep.AbilityIndex = B.Impact;
	TestTrue(TEXT("una voce Prep entra"), B.Riktor->AddPlanEntry(Prep));
	TestTrue(TEXT("e una seconda pure"), B.Riktor->AddPlanEntry(Prep));
	B.Riktor->ClearPlannedPrepEntries();
	TestEqual(TEXT("tolte le voci Prep, le altre restano"), P.GetEntries().Num(), 4);

	// E38 CP 38.16: scatto e Move stanno nello stesso piano ([D-584] (1)). ⏱️ *Fino a CP 38.16 il validatore li registrava
	// `SlotOccupied`.* Il piano qui puo' sforare il budget per le altre voci, ma il motivo non e' piu' mai lo slot.
	const FRTPlanValidation V = URTPlanValidationLibrary::ValidatePlan(ARTTurnManager::MakeSimUnit(INDEX_NONE, B.Riktor),
		URTPlanValidationLibrary::MakePlanFor(B.Riktor));
	TestTrue(TEXT("scatto e Move nello stesso piano: legale, o oltre il budget, mai slot"),
		V.bLegal || V.Reason == ERTActionInvalidReason::OverBudget);
	DestroyInteractionWorld(B.World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanMovementBandIgnoresTheDeclaredSequenceTest,
	"RefactorTactics.Plan.MovementBandIgnoresTheDeclaredSequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanMovementBandIgnoresTheDeclaredSequenceTest::RunTest(const FString&)
{
	// Criterio 7: Move -> Overwatch e Overwatch -> Move danno la stessa banda e lo stesso `MoveBudget`.
	struct FEsito { FName Banda; int32 MoveBudget = INDEX_NONE; int32 OrdineMove = INDEX_NONE; };
	auto Gioca = [this](bool bMovePrima, FEsito& Out)
	{
		FLockInBench B;
		if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
		const int32 Overwatch = RTAbilityFixtures::AddCoreAbility(B.Riktor, TEXT("Action.Overwatch"));
		if (!TestTrue(TEXT("premessa: Riktor ha l'Overwatch"), Overwatch != INDEX_NONE)) { DestroyInteractionWorld(B.World); return false; }
		auto Muovi = [&B]() { B.Riktor->SetPlannedMove({ FRTCellId(0, 1) }, { FRTCellId(0, 0), FRTCellId(0, 1) }); };
		if (bMovePrima) { Muovi(); B.Riktor->SetPlannedMainAction(Overwatch); }
		else { B.Riktor->SetPlannedMainAction(Overwatch); Muovi(); }
		for (const FRTPlannedAction& A : URTPlanValidationLibrary::MakePlanFor(B.Riktor))
		{
			if (A.Def.Slot == ERTActionSlot::Movement) { Out.Banda = A.Def.MovementProfileId; }
		}
		Out.MoveBudget = ARTTurnManager::MakeSimUnit(INDEX_NONE, B.Riktor).MoveBudget;
		Out.OrdineMove = B.Riktor->GetPlan().DeclarationIndexOf(ERTPlanEntryClass::Movement);
		DestroyInteractionWorld(B.World);
		return true;
	};
	FEsito MovePrima;
	FEsito OverwatchPrima;
	if (!Gioca(true, MovePrima) || !Gioca(false, OverwatchPrima)) { return false; }
	TestNotEqual(TEXT("controllo: l'ordine di dichiarazione e' davvero diverso"), MovePrima.OrdineMove, OverwatchPrima.OrdineMove);
	TestEqual(TEXT("con l'Overwatch dichiarato dopo, il Move ripiega lo stesso (D-070)"), MovePrima.Banda, URTMovementProfileLibrary::ProfileWithdraw);
	TestEqual(TEXT("stessa banda nei due ordini"), MovePrima.Banda, OverwatchPrima.Banda);
	TestEqual(TEXT("e stesso MoveBudget"), MovePrima.MoveBudget, OverwatchPrima.MoveBudget);
	return true;
}

namespace
{
	/** Una voce del TurnLog in forma confrontabile: tutti i campi di esito, senza verdetti di conoscenza. */
	FString DescriviVoce(const FRTTurnLogEntry& E)
	{
		return FString::Printf(TEXT("T%d P%d C%d O%d U%d %s/%s A%d %s>%s"), E.TurnNumber, static_cast<int32>(E.Phase),
			static_cast<int32>(E.Category), E.Outcome, E.UnitId, *E.ActionId.ToString(), *E.BaseActionId.ToString(), E.Amount,
			*E.SrcCell.ToString(), *E.TgtCell.ToString());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTResolutionPermutingTheDeclarationKeepsTheOutcomeTest,
	"RefactorTactics.Resolution.PermutingTheDeclarationKeepsTheOutcome",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTResolutionPermutingTheDeclarationKeepsTheOutcomeTest::RunTest(const FString&)
{
	// Criterio 8, il test di permutazione della DoD di #608: lo stesso piano legale di Riktor dichiarato in due ordini
	// da' lo stesso stato finale, lo stesso hash di stato, le stesse attivazioni e le stesse voci di TurnLog.
	// Banco: Riktor in (0,0,0), Gadget in (-1,0,0); ImpactShot su Gadget, Interposition, Move verso (0,1,0). Da (0,1,0)
	// Gadget dista 2, dentro la portata di ImpactShot: il colpo parte in entrambi gli ordini.
	struct FEsito { FRTCellId Cella; int32 VitaGadget = 0; uint32 Hash = 0; TArray<FName> Attivazioni; TArray<FString> Voci; int32 OrdinePrincipale = INDEX_NONE; };
	auto Gioca = [this](bool bPrincipalePrima, FEsito& Out)
	{
		FLockInBench B;
		if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
		auto Principale = [&B]() { B.Riktor->SetPlannedMainAction(B.Impact); B.Riktor->DeclareAttackOnUnit(B.Gadget); };
		auto Reazione = [&B]() { B.Riktor->SetPlannedReaction(B.Interposition); };
		auto Movimento = [&B]() { B.Riktor->SetPlannedMove({ FRTCellId(0, 1) }, { FRTCellId(0, 0), FRTCellId(0, 1) }); };
		if (bPrincipalePrima) { Principale(); Reazione(); Movimento(); }
		else { Movimento(); Reazione(); Principale(); }
		Out.OrdinePrincipale = B.Riktor->GetPlan().DeclarationIndexOf(ERTPlanEntryClass::Main);
		const FRTPlanValidation V = URTPlanValidationLibrary::ValidatePlan(ARTTurnManager::MakeSimUnit(INDEX_NONE, B.Riktor),
			URTPlanValidationLibrary::MakePlanFor(B.Riktor));
		TestTrue(TEXT("premessa: il piano e' legale"), V.bLegal);

		ResolveOneTurn(B.TM);
		Out.Cella = B.Riktor->Cell;
		Out.VitaGadget = B.Gadget->Health;
		const URTHexMapAsset* Mappa = nullptr;
		for (TActorIterator<ARTHexMapActor> It(B.World); It; ++It) { Mappa = It->MapAsset; }
		Out.Hash = URTMatchStateHashLibrary::HashMatchState(Mappa, URTMatchStateHashLibrary::BuildUnitDigests({ B.Riktor, B.Gadget }), {});
		for (const FRTResolvedEvent& E : B.TM->ResolvedTimelineForTest())
		{
			if (E.Type == ERTResolvedEventType::AbilityActivated) { Out.Attivazioni.Add(E.ActionId); }
		}
		for (const FRTTurnLogEntry& E : B.TM->GetTurnLog()) { Out.Voci.Add(DescriviVoce(E)); }
		DestroyInteractionWorld(B.World);
		return true;
	};
	FEsito A;
	FEsito Bx;
	if (!Gioca(true, A) || !Gioca(false, Bx)) { return false; }

	TestNotEqual(TEXT("controllo: le due dichiarazioni sono davvero in ordine diverso"), A.OrdinePrincipale, Bx.OrdinePrincipale);
	TestTrue(TEXT("controllo: il colpo e' partito (Gadget ha perso vita)"), A.VitaGadget < URTHeroCatalogLibrary::MakeGadget()->MaxHealth);
	TestEqual(TEXT("stessa cella d'arrivo"), A.Cella, Bx.Cella);
	TestEqual(TEXT("stessa vita di Gadget"), A.VitaGadget, Bx.VitaGadget);
	TestEqual(TEXT("stesso hash di stato"), A.Hash, Bx.Hash);
	TestEqual(TEXT("stesse attivazioni, nello stesso ordine"), A.Attivazioni, Bx.Attivazioni);
	TestEqual(TEXT("stesse voci di TurnLog"), A.Voci.Num(), Bx.Voci.Num());
	for (int32 i = 0; i < FMath::Min(A.Voci.Num(), Bx.Voci.Num()); ++i)
	{
		TestEqual(*FString::Printf(TEXT("voce %d"), i), A.Voci[i], Bx.Voci[i]);
	}
	return true;
}

// ── [D-608] tappa 4 (#4127): la copia del piano al lock-in ──────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanLockedPlanIsTakenAtLockInTest,
	"RefactorTactics.Plan.LockedPlanIsTakenAtLockIn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanLockedPlanIsTakenAtLockInTest::RunTest(const FString&)
{
	// Criteri 1 e 2: dopo il lock-in la copia porta il piano, il bersaglio per StableUnitId e la mira sulla cella del
	// bersaglio al lock-in; e non cambia quando, dopo il lock-in, cambiano il piano vivo e la posizione del bersaglio.
	FLockInBench B;
	if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
	B.Riktor->SetPlannedMainAction(B.Impact);
	B.Riktor->DeclareAttackOnUnit(B.Gadget);
	B.Riktor->SetPlannedReaction(B.Interposition);
	B.Riktor->SetPlannedMove({ FRTCellId(0, 1) }, { FRTCellId(0, 0), FRTCellId(0, 1) });
	// Gadget si muove nel turno: il Move risolve dopo il Blast, e la mira resta dov'era al lock-in.
	B.Gadget->SetPlannedMove({ FRTCellId(-2, 0) }, { FRTCellId(-1, 0), FRTCellId(-2, 0) });
	const FRTCellId DoveEraGadget = B.Gadget->Cell;

	ResolveOneTurn(B.TM);
	const FRTLockedPlan& Copia = B.Riktor->GetLockedPlan();
	TestNotEqual(TEXT("premessa: il roster c'e', Gadget ha un'identita'"), B.Gadget->StableUnitId, 0);
	TestEqual(TEXT("il bersaglio per StableUnitId"), Copia.AttackTargetStableId, B.Gadget->StableUnitId);
	TestTrue(TEXT("la mira e' congelata"), Copia.bHasAim);
	TestEqual(TEXT("sulla cella del bersaglio al lock-in"), Copia.AimCell, DoveEraGadget);
	TestEqual(TEXT("la principale del piano"), Copia.Plan.PlannedAbilityIndex(), B.Impact);
	TestEqual(TEXT("la reazione"), Copia.Plan.PlannedReactionAbility(), B.Interposition);
	TestEqual(TEXT("e le voci nel loro ordine"), Copia.Plan.DeclarationIndexOf(ERTPlanEntryClass::Movement), 2);

	// Il turno ha consumato il piano vivo e spostato il bersaglio: la copia no.
	TestEqual(TEXT("controllo: la principale viva e' consumata"), B.Riktor->GetPlan().PlannedAbilityIndex(), (int32)INDEX_NONE);
	TestNotEqual(TEXT("controllo: Gadget si e' mosso"), B.Gadget->Cell, DoveEraGadget);
	TestEqual(TEXT("e la copia ha ancora la principale"), Copia.Plan.PlannedAbilityIndex(), B.Impact);
	TestEqual(TEXT("e la mira di prima"), Copia.AimCell, DoveEraGadget);
	DestroyInteractionWorld(B.World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanLockedPlanOfARefusedPlanIsEmptyTest,
	"RefactorTactics.Plan.LockedPlanOfARefusedPlanIsEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanLockedPlanOfARefusedPlanIsEmptyTest::RunTest(const FString&)
{
	// Criterio 3: la copia di un piano svuotato al lock-in (CP 38.10) e' il piano che si risolve, cioe' vuoto.
	FLockInBench B;
	if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
	PlanOverBudget(B);
	ResolveOneTurn(B.TM);
	const FRTLockedPlan& Copia = B.Riktor->GetLockedPlan();
	TestEqual(TEXT("nessuna principale"), Copia.Plan.PlannedAbilityIndex(), (int32)INDEX_NONE);
	TestEqual(TEXT("nessuno scatto"), Copia.Plan.PlannedDashAbility(), (int32)INDEX_NONE);
	TestEqual(TEXT("nessuna reazione"), Copia.Plan.PlannedReactionAbility(), (int32)INDEX_NONE);
	TestEqual(TEXT("nessun percorso"), Copia.Plan.PlannedPath().Num(), 0);
	TestEqual(TEXT("nessun bersaglio"), Copia.AttackTargetStableId, (int32)INDEX_NONE);
	TestFalse(TEXT("e nessuna mira"), Copia.bHasAim);
	// Controllo: un piano legale, nello stesso banco, lascia una copia piena.
	FLockInBench C;
	if (!MakeLockInBench(*this, C)) { DestroyInteractionWorld(B.World); DestroyInteractionWorld(C.World); return false; }
	C.Riktor->SetPlannedMainAction(C.Impact);
	C.Riktor->DeclareAttackOnUnit(C.Gadget);
	ResolveOneTurn(C.TM);
	TestEqual(TEXT("controllo: il piano legale lascia la principale nella copia"), C.Riktor->GetLockedPlan().Plan.PlannedAbilityIndex(), C.Impact);
	DestroyInteractionWorld(B.World);
	DestroyInteractionWorld(C.World);
	return true;
}

// ── E38 CP 38.13 (#4114): la regola della copia legge la riga, e l'Overwatch a ricarica 1 ─────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanCopyRuleReadsTheCatalogCooldownTest,
	"RefactorTactics.Plan.CopyRuleReadsTheCatalogCooldown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanCopyRuleReadsTheCatalogCooldownTest::RunTest(const FString&)
{
	// Criterio 3, [D-615] (6). Prima parte: con `Weapon.Overcharge` la voce dell'attacco base porta il cooldown della
	// riga, mentre il suo `Def` porta la riga piu' i turni della variante.
	FLockInBench B;
	if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
	const int32 DellaRiga = B.Riktor->GetAbility(0)->Def.CooldownTurns;
	B.Riktor->EquipLoadout({ FName(TEXT("Weapon.Overcharge")) });
	B.Riktor->SetPlannedMainAction(0);
	B.Riktor->DeclareAttackOnUnit(B.Gadget);
	const TArray<FRTPlannedAction> Piano = URTPlanValidationLibrary::MakePlanFor(B.Riktor);
	if (TestEqual(TEXT("una voce, l'attacco base"), Piano.Num(), 1))
	{
		TestTrue(TEXT("premessa: la variante allunga la ricarica del Def"), Piano[0].Def.CooldownTurns > DellaRiga);
		TestEqual(TEXT("la voce porta il cooldown della riga"), Piano[0].CatalogCooldownTurns, DellaRiga);
	}
	// E38 CP 38.14, criterio 2: con la variante, due attacchi base sono legali, perche' la riga non ha ricarica.
	if (TestTrue(TEXT("il gesto aggiunge il secondo attacco base"), B.Riktor->DeclarePlannedAction(0)))
	{
		TestTrue(TEXT("due attacchi base con Overcharge: legali"), LockInVerdictOf(B.Riktor).bLegal);
		B.Riktor->RemovePlanEntry(B.Riktor->GetPlan().GetEntries().Num() - 1);
	}
	// Una seconda variante non registra un cooldown gia' cambiato.
	B.Riktor->EquipLoadout({ FName(TEXT("Weapon.Overcharge")) });
	const TArray<FRTPlannedAction> Dopo = URTPlanValidationLibrary::MakePlanFor(B.Riktor);
	if (TestEqual(TEXT("una voce"), Dopo.Num(), 1))
	{
		TestEqual(TEXT("la riga resta quella di prima della prima variante"), Dopo[0].CatalogCooldownTurns, DellaRiga);
	}
	DestroyInteractionWorld(B.World);

	// Seconda parte, su voci sintetiche, in ogni fase: decide il cooldown della riga, non quello del `Def`.
	auto Coppia = [](ERTResolutionPhase Fase, ERTActionSlot Slot, int32 DellaRigaSint, int32 DelDef)
	{
		TArray<FRTPlannedAction> Plan;
		for (int32 I = 0; I < 2; ++I)
		{
			FRTPlannedAction E;
			E.Def.ActionId = TEXT("Test.Copia");
			E.Def.ResolutionPhase = Fase;
			E.Def.Slot = Slot;
			E.Def.ActionPointCost = 1;
			E.Def.CooldownTurns = DelDef;
			E.CatalogCooldownTurns = DellaRigaSint;
			E.DeclarationIndex = I;
			Plan.Add(E);
		}
		return URTPlanValidationLibrary::ValidatePlan(FRTHexSimUnit(), Plan);
	};
	TestTrue(TEXT("Prep, riga 0 e Def 2: legale"), Coppia(ERTResolutionPhase::Preparation, ERTActionSlot::Main, 0, 2).bLegal);
	TestTrue(TEXT("Blast, riga 0 e Def 2: legale da CP 38.14"), Coppia(ERTResolutionPhase::Attack, ERTActionSlot::Main, 0, 2).bLegal);
	const struct { ERTResolutionPhase Fase; ERTActionSlot Slot; const TCHAR* Nome; } Fasi[] = {
		{ ERTResolutionPhase::Preparation, ERTActionSlot::Main, TEXT("Prep") },
		{ ERTResolutionPhase::Attack, ERTActionSlot::Main, TEXT("Blast") },
		{ ERTResolutionPhase::FastMovement, ERTActionSlot::Movement, TEXT("Dash") },
		{ ERTResolutionPhase::Attack, ERTActionSlot::Reaction, TEXT("reazione") },
	};
	for (const auto& F : Fasi)
	{
		TestEqual(*FString::Printf(TEXT("%s, riga 2 e Def 0: RepeatedWithCooldown"), F.Nome),
			Coppia(F.Fase, F.Slot, 2, 0).Reason, ERTActionInvalidReason::RepeatedWithCooldown);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverwatchCooldownOneReturnsEveryTurnTest,
	"RefactorTactics.Overwatch.CooldownOneReturnsEveryTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverwatchCooldownOneReturnsEveryTurnTest::RunTest(const FString&)
{
	// Criterio 14, [D-615] (4): l'Overwatch ha ricarica 1 e torna ogni turno, perche' il cooldown impostato all'uso
	// si sconta nel Cleanup dello stesso turno.
	FLockInBench B;
	if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
	const int32 Overwatch = LockInAbilityIndex(B.Riktor, TEXT("Action.Overwatch"));
	if (!TestNotEqual(TEXT("premessa: Riktor ha l'Overwatch"), Overwatch, (int32)INDEX_NONE)) { DestroyInteractionWorld(B.World); return false; }
	TestEqual(TEXT("premessa: la copia dell'unita' porta la ricarica 1"), B.Riktor->GetAbility(Overwatch)->CooldownTurns, 1);

	B.Riktor->SetPlannedMainAction(Overwatch);
	ResolveOneTurn(B.TM);
	TestEqual(TEXT("dopo il Cleanup del turno 1 la ricarica vale 0"), B.Riktor->GetAbilityCooldown(Overwatch), 0);
	B.Riktor->SetPlannedMainAction(Overwatch);
	TestTrue(TEXT("al turno 2 si riarma senza rifiuto"), LockInVerdictOf(B.Riktor).bLegal);

	// Controllo positivo: una riga a ricarica 2, usata al turno 1, al turno 2 e' ancora in ricarica.
	FLockInBench C;
	if (!MakeLockInBench(*this, C)) { DestroyInteractionWorld(B.World); DestroyInteractionWorld(C.World); return false; }
	TestEqual(TEXT("premessa del controllo: Ram ha ricarica 2"), C.Riktor->GetAbility(C.Ram)->CooldownTurns, 2);
	C.Riktor->SetPlannedDash(C.Ram, FRTCellId(2, 0));
	ResolveOneTurn(C.TM);
	TestTrue(TEXT("controllo: Ram e' ancora in ricarica"), C.Riktor->GetAbilityCooldown(C.Ram) > 0);
	C.Riktor->SetPlannedDash(C.Ram, FRTCellId(2, 0));
	TestEqual(TEXT("controllo: al turno 2 Ram e' OnCooldown"), LockInVerdictOf(C.Riktor).Reason, ERTActionInvalidReason::OnCooldown);
	DestroyInteractionWorld(B.World);
	DestroyInteractionWorld(C.World);
	return true;
}

// ── E38 CP 38.13 (#4114): il Prep risolve piu' azioni della stessa unita' ─────────────────────────────────────────────
namespace
{
	/** Una riga di Prep sintetica, a priorita' 40, che da' uno scudo a chi la usa: due righe cosi' stanno nello stesso sotto-passo. */
	int32 AddSyntheticPrepRow(ARTUnit* Unit, const TCHAR* ActionId)
	{
		URTActionData* Action = NewObject<URTActionData>(Unit);
		Action->Def.ActionId = FName(ActionId);
		Action->Def.ResolutionPhase = ERTResolutionPhase::Preparation;
		Action->Def.Priority = 40;
		Action->Def.Slot = ERTActionSlot::Main;
		Action->Def.ActionPointCost = 1;
		Action->Def.bSelfTarget = true;
		Action->Def.Effects = { FRTActionEffectSpec(ERTActionEffect::Shield, 5) };
		Action->bSelfTarget = true;
		Action->Power = 0;
		Unit->Abilities.Add(Action);
		return Unit->Abilities.Num() - 1;
	}

	/** Gli `ActionId` delle attivazioni di un'unita' in una fase, nell'ordine della timeline. */
	TArray<FName> ActivationsOf(const ARTTurnManager* TM, const ARTUnit* Unit, ERTMatchPhase Phase)
	{
		TArray<FName> Out;
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::AbilityActivated && Ev.Phase == Phase && Ev.SourceStableUnitId == Unit->StableUnitId)
			{
				Out.Add(Ev.ActionId);
			}
		}
		return Out;
	}

	int32 CountCombatOf(const TArray<FRTTurnLogEntry>& Log, FName ActionId)
	{
		int32 N = 0;
		for (const FRTTurnLogEntry& E : Log)
		{
			if (E.Category == ERTLogCategory::Combat && E.ActionId == ActionId) { ++N; }
		}
		return N;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTResolutionPrepIgnoresTheDeclaredSequenceTest,
	"RefactorTactics.Resolution.PrepIgnoresTheDeclaredSequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTResolutionPrepIgnoresTheDeclaredSequenceTest::RunTest(const FString&)
{
	// Criterio 4: due azioni di Prep della stessa unita' alla stessa priorita' si attivano in ordine di `ActionId` in
	// entrambi gli ordini di dichiarazione; un'armata e un'istanza, nell'ordine dei sotto-passi, l'armata prima.
	auto Gioca = [this](bool bRovescio, bool bArmata, TArray<FName>& Out)
	{
		FLockInBench B;
		if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
		int32 Prima = INDEX_NONE;
		int32 Seconda = INDEX_NONE;
		if (bArmata)
		{
			Prima = RTAbilityFixtures::AddCoreAbility(B.Riktor, TEXT("Action.Overwatch")); // 45, si arma
			Seconda = RTAbilityFixtures::AddCoreAbility(B.Riktor, TEXT("Action.Guard"));   // 40, un'istanza
		}
		else
		{
			Prima = AddSyntheticPrepRow(B.Riktor, TEXT("Test.PrepA"));
			Seconda = AddSyntheticPrepRow(B.Riktor, TEXT("Test.PrepB"));
		}
		const bool bA = B.Riktor->DeclarePlannedAction(bRovescio ? Seconda : Prima);
		const bool bB = B.Riktor->DeclarePlannedAction(bRovescio ? Prima : Seconda);
		if (!TestTrue(TEXT("premessa: le due voci si scrivono"), bA && bB)
			|| !TestTrue(TEXT("premessa: il piano e' legale"), LockInVerdictOf(B.Riktor).bLegal))
		{
			DestroyInteractionWorld(B.World);
			return false;
		}
		ResolveOneTurn(B.TM);
		Out = ActivationsOf(B.TM, B.Riktor, ERTMatchPhase::Prep);
		DestroyInteractionWorld(B.World);
		return true;
	};
	TArray<FName> Dritto, Rovescio, ArmataDritto, ArmataRovescio;
	if (!Gioca(false, false, Dritto) || !Gioca(true, false, Rovescio) || !Gioca(false, true, ArmataDritto) || !Gioca(true, true, ArmataRovescio))
	{
		return false;
	}
	const TArray<FName> PerActionId = { FName(TEXT("Test.PrepA")), FName(TEXT("Test.PrepB")) };
	TestEqual(TEXT("a pari priorita': per ActionId"), Dritto, PerActionId);
	TestEqual(TEXT("e nell'altro ordine di dichiarazione lo stesso"), Rovescio, PerActionId);
	const TArray<FName> PerSottoPasso = { FName(TEXT("Action.Overwatch")), FName(TEXT("Action.Guard")) };
	TestEqual(TEXT("l'armata prima dell'istanza"), ArmataDritto, PerSottoPasso);
	TestEqual(TEXT("in entrambi gli ordini di dichiarazione"), ArmataRovescio, PerSottoPasso);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTResolutionPrepConsumesOnlyTheActivatedEntryTest,
	"RefactorTactics.Resolution.PrepConsumesOnlyTheActivatedEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTResolutionPrepConsumesOnlyTheActivatedEntryTest::RunTest(const FString&)
{
	// Criterio 12: con Guard + ImpactShot il Prep consuma la Guard e lascia il bersaglio di ImpactShot, che colpisce nel
	// Blast, in entrambi gli ordini di dichiarazione.
	for (const bool bGuardPrima : { true, false })
	{
		FLockInBench B;
		if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
		const int32 Guard = RTAbilityFixtures::AddCoreAbility(B.Riktor, TEXT("Action.Guard"));
		if (bGuardPrima)
		{
			B.Riktor->DeclarePlannedAction(Guard);
			B.Riktor->DeclarePlannedAction(B.Impact);
		}
		else
		{
			B.Riktor->DeclarePlannedAction(B.Impact);
			B.Riktor->DeclarePlannedAction(Guard);
		}
		B.Riktor->DeclareAttackOnUnit(B.Gadget);
		const FRTUnitPlan& P = B.Riktor->GetPlan();
		const TCHAR* Ordine = bGuardPrima ? TEXT("Guard, ImpactShot") : TEXT("ImpactShot, Guard");
		TestEqual(*FString::Printf(TEXT("%s: la principale e' ImpactShot"), Ordine), P.PlannedAbilityIndex(), B.Impact);
		TestEqual(*FString::Printf(TEXT("%s: la Guard e' una voce Prep al suo ordine"), Ordine),
			P.DeclarationIndexOf(ERTPlanEntryClass::Prep), bGuardPrima ? 0 : 1);
		TestTrue(*FString::Printf(TEXT("%s: premessa, il piano e' legale"), Ordine), LockInVerdictOf(B.Riktor).bLegal);

		ResolveOneTurn(B.TM);
		TestEqual(*FString::Printf(TEXT("%s: la Guard si attiva nel Prep"), Ordine),
			ActivationsOf(B.TM, B.Riktor, ERTMatchPhase::Prep), TArray<FName>{ FName(TEXT("Action.Guard")) });
		TestEqual(*FString::Printf(TEXT("%s: ImpactShot colpisce nel Blast"), Ordine),
			CountCombatOf(B.TM->GetTurnLog(), FName(TEXT("Hero.Riktor.ImpactShot"))), 1);
		TestEqual(*FString::Printf(TEXT("%s: e nessuna voce Prep resta nel piano"), Ordine),
			B.Riktor->GetPlan().DeclarationIndexOf(ERTPlanEntryClass::Prep), (int32)INDEX_NONE);
		DestroyInteractionWorld(B.World);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanLockInEmptiesAForbiddenCopyInAClosedPhaseTest,
	"RefactorTactics.Plan.LockInEmptiesAForbiddenCopyInAClosedPhase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanLockInEmptiesAForbiddenCopyInAClosedPhaseTest::RunTest(const FString&)
{
	// Criterio 17: due MortarShot su un bersaglio in portata, scritte dall'accesso dei test perche' nessun produttore le
	// scrive, al lock-in svuotano il piano col motivo nuovo, e nessun colpo parte.
	// Il bersaglio a 3 celle: il mortaio non spara sotto 2 (`MinRangeCells`), e il Gadget del banco e' a 1. Si mira la
	// CELLA, come fa un'arma d'area: a 3 celle il bersaglio non e' noto alla squadra, e un colpo su un'unita' ignota esce
	// `TargetUnknown`.
	FLockInBench B;
	if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
	ARTUnit* Lontano = SpawnInteractionUnit(B.World, 1, URTHeroCatalogLibrary::MakeGadget(), FRTCellId(-3, 0));
	if (!TestNotNull(TEXT("il bersaglio a 3 celle"), Lontano)) { DestroyInteractionWorld(B.World); return false; }
	FRTPlanEntry Mortaio;
	Mortaio.Class = ERTPlanEntryClass::Main;
	Mortaio.AbilityIndex = B.Mortar;
	Mortaio.AttackCell = Lontano->Cell;
	Mortaio.bAttackTargetsCell = true;
	FRTUnitPlanTestAccess::AppendRawEntry(*B.Riktor, Mortaio);
	FRTUnitPlanTestAccess::AppendRawEntry(*B.Riktor, Mortaio);
	TestEqual(TEXT("premessa: il validatore vede due copie"), LockInVerdictOf(B.Riktor).Reason, ERTActionInvalidReason::RepeatedWithCooldown);
	ResolveOneTurn(B.TM);
	const TArray<FRTTurnLogEntry>& Log = B.TM->GetTurnLog();
	int32 Copie = 0;
	for (const FRTTurnLogEntry& E : Log)
	{
		if (E.UnitId == B.Riktor->StableUnitId && E.Category == ERTLogCategory::Fallback
			&& E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Cancelled)
			&& E.Amount == static_cast<int32>(ERTActionInvalidReason::RepeatedWithCooldown))
		{
			++Copie;
		}
	}
	TestEqual(TEXT("due voci Fallback/Cancelled col motivo RepeatedWithCooldown"), Copie, 2);
	TestEqual(TEXT("nessun colpo di mortaio"), CountCombatOf(Log, FName(TEXT("Hero.Riktor.MortarShot"))), 0);
	TestTrue(TEXT("e il piano e' vuoto"), B.Riktor->GetPlan().GetEntries().IsEmpty());
	DestroyInteractionWorld(B.World);

	// Controllo positivo: lo stesso mortaio, una volta sola, colpisce.
	FLockInBench C;
	if (!MakeLockInBench(*this, C)) { DestroyInteractionWorld(C.World); return false; }
	ARTUnit* LontanoC = SpawnInteractionUnit(C.World, 1, URTHeroCatalogLibrary::MakeGadget(), FRTCellId(-3, 0));
	if (!TestNotNull(TEXT("il bersaglio a 3 celle, nel controllo"), LontanoC)) { DestroyInteractionWorld(C.World); return false; }
	C.Riktor->SetPlannedMainAction(C.Mortar);
	C.Riktor->DeclareAttackOnCell(LontanoC->Cell);
	ResolveOneTurn(C.TM);
	TestTrue(TEXT("controllo: un mortaio solo parte"),
		CountCombatOf(C.TM->GetTurnLog(), FName(TEXT("Hero.Riktor.MortarShot"))) > 0);
	DestroyInteractionWorld(C.World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanTheListGestureAddsPrepActionsTest,
	"RefactorTactics.Plan.TheListGestureAddsPrepActions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanTheListGestureAddsPrepActionsTest::RunTest(const FString&)
{
	// Criterio 18: il gesto della lista aggiunge le azioni di Prep, e una seconda voce di una classe chiusa non si scrive.
	FLockInBench B;
	if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
	const int32 Guard = RTAbilityFixtures::AddCoreAbility(B.Riktor, TEXT("Action.Guard"));
	const int32 Brace = RTAbilityFixtures::AddCoreAbility(B.Riktor, TEXT("Action.Brace"));
	const FRTUnitPlan& P = B.Riktor->GetPlan();
	TestTrue(TEXT("la prima azione di Prep va nella principale"), B.Riktor->DeclarePlannedAction(Guard));
	TestEqual(TEXT("e' la principale"), P.PlannedAbilityIndex(), Guard);
	TestTrue(TEXT("la seconda si aggiunge"), B.Riktor->DeclarePlannedAction(Brace));
	TestTrue(TEXT("una principale del Blast fa posto"), B.Riktor->DeclarePlannedAction(B.Impact));
	if (!TestEqual(TEXT("tre voci"), P.GetEntries().Num(), 3)) { DestroyInteractionWorld(B.World); return false; }
	TestEqual(TEXT("la Guard resta al suo ordine, fra le voci Prep"), P.GetEntries()[0].Class, ERTPlanEntryClass::Prep);
	TestEqual(TEXT("la Guard resta la Guard"), P.GetEntries()[0].AbilityIndex, Guard);
	TestEqual(TEXT("il Brace e' una voce Prep"), P.GetEntries()[1].Class, ERTPlanEntryClass::Prep);
	TestEqual(TEXT("ImpactShot e' la principale, in coda"), P.DeclarationIndexOf(ERTPlanEntryClass::Main), 2);
	TestEqual(TEXT("e la principale e' ImpactShot"), P.PlannedAbilityIndex(), B.Impact);

	// E38 CP 38.14: il Blast e' aperto. Una seconda principale del Blast si aggiunge, e cosi' una copia senza ricarica;
	// la prima principale resta quella che i lettori di oggi vedono.
	const int32 Voci = P.GetEntries().Num();
	TestTrue(TEXT("una seconda principale del Blast si aggiunge"), B.Riktor->DeclarePlannedAction(B.Mortar));
	TestTrue(TEXT("e cosi' una seconda ImpactShot"), B.Riktor->DeclarePlannedAction(B.Impact));
	TestEqual(TEXT("due voci in piu'"), P.GetEntries().Num(), Voci + 2);
	TestEqual(TEXT("con la stessa prima principale"), P.PlannedAbilityIndex(), B.Impact);
	// Una mobilita' non passa da qui: e' lo scatto, una classe chiusa.
	const int32 Prima = P.GetEntries().Num();
	TestFalse(TEXT("lo scatto non entra fra le principali"), B.Riktor->DeclarePlannedAction(B.Ram));
	TestEqual(TEXT("e il piano resta com'era"), P.GetEntries().Num(), Prima);
	TestTrue(TEXT("una copia di Prep senza ricarica si aggiunge"), B.Riktor->DeclarePlannedAction(Guard));
	TestFalse(TEXT("un indice fuori dal kit si rifiuta"), B.Riktor->DeclarePlannedAction(9999));
	DestroyInteractionWorld(B.World);
	return true;
}

// ── E38 CP 38.13 (#4114): il campo d'ordine nel TurnLog ────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogEveryPlannedEntryCarriesItsIndexTest,
	"RefactorTactics.TurnLog.EveryPlannedEntryCarriesItsIndex",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogEveryPlannedEntryCarriesItsIndexTest::RunTest(const FString&)
{
	// Criterio 19: Guard, Guard, ImpactShot su Gadget. Le voci di stato delle due Guard portano 0 e 1; il colpo e il
	// rallentamento che lascia su Gadget portano 2, anche se il rallentamento e' intestato a Gadget; ogni voce con
	// l'indice porta la squadra di chi l'ha dichiarata.
	FLockInBench B;
	if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
	const int32 Guard = RTAbilityFixtures::AddCoreAbility(B.Riktor, TEXT("Action.Guard"));
	B.Riktor->DeclarePlannedAction(Guard);
	B.Riktor->DeclarePlannedAction(Guard);
	B.Riktor->DeclarePlannedAction(B.Impact);
	B.Riktor->DeclareAttackOnUnit(B.Gadget);
	TestEqual(TEXT("premessa: la principale e' l'ultima dichiarata"), B.Riktor->GetPlan().DeclarationIndexOf(ERTPlanEntryClass::Main), 2);
	ResolveOneTurn(B.TM);

	TArray<int32> Guardie;
	int32 Colpo = -99;
	int32 Rallentamento = -99;
	int32 Rallentamenti = 0;
	for (const FRTTurnLogEntry& E : B.TM->GetTurnLog())
	{
		if (E.DeclarationIndex != INDEX_NONE)
		{
			TestEqual(*FString::Printf(TEXT("la voce %s con l'indice porta la squadra di Riktor"), *E.ActionId.ToString()), E.DeclaringTeamId, B.Riktor->TeamId);
		}
		if (E.Phase == ERTMatchPhase::Prep && E.Category == ERTLogCategory::Status && E.UnitId == B.Riktor->StableUnitId) { Guardie.Add(E.DeclarationIndex); }
		if (E.Category == ERTLogCategory::Combat && E.ActionId == FName(TEXT("Hero.Riktor.ImpactShot"))) { Colpo = E.DeclarationIndex; }
		if (E.Phase == ERTMatchPhase::Blast && E.Category == ERTLogCategory::Status && E.UnitId == B.Gadget->StableUnitId)
		{
			Rallentamento = E.DeclarationIndex;
			++Rallentamenti;
		}
	}
	Guardie.Sort();
	TestEqual(TEXT("le due Guard portano 0 e 1"), Guardie, TArray<int32>{ 0, 1 });
	TestEqual(TEXT("il colpo porta 2"), Colpo, 2);
	TestEqual(TEXT("premessa: il rallentamento c'e'"), Rallentamenti, 1);
	TestEqual(TEXT("e porta 2, benche' intestato a Gadget"), Rallentamento, 2);
	DestroyInteractionWorld(B.World);

	// Controllo: una voce che nessuna voce del piano produce, la scadenza di uno stato, non porta l'indice.
	FLockInBench C;
	if (!MakeLockInBench(*this, C)) { DestroyInteractionWorld(C.World); return false; }
	const int32 GuardC = RTAbilityFixtures::AddCoreAbility(C.Riktor, TEXT("Action.Guard"));
	C.Riktor->DeclarePlannedAction(GuardC);
	int32 Scadenze = 0;
	auto ContaScadenze = [this, &C, &Scadenze]()
	{
		for (const FRTTurnLogEntry& E : C.TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Status && E.UnitId == C.Riktor->StableUnitId && E.Phase == ERTMatchPhase::Cleanup)
			{
				++Scadenze;
				TestEqual(TEXT("la scadenza non ha un ordine di dichiarazione"), E.DeclarationIndex, (int32)INDEX_NONE);
			}
		}
	};
	ResolveOneTurn(C.TM);
	ContaScadenze();
	ResolveOneTurn(C.TM); // il TurnLog e' del turno: la scadenza puo' cadere nel primo o nel secondo, e il piano e' vuoto
	ContaScadenze();
	TestTrue(TEXT("premessa del controllo: la scadenza c'e'"), Scadenze > 0);
	DestroyInteractionWorld(C.World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTResolutionDeclarationPermutationChangesOnlyTheOrderFieldTest,
	"RefactorTactics.Resolution.DeclarationPermutationChangesOnlyTheOrderField",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTResolutionDeclarationPermutationChangesOnlyTheOrderFieldTest::RunTest(const FString&)
{
	// Criterio 13, [D-615] (5) e (2): Guard, Brace, ImpactShot e ImpactShot, Brace, Guard danno lo stesso stato, gli stessi
	// eventi e le stesse voci; cambiano solo gli indici di Guard e ImpactShot, e con loro l'hash di traccia.
	struct FEsito { uint32 Stato = 0; TArray<FName> Attivazioni; TArray<FRTTurnLogEntry> Voci; int32 VitaGadget = 0; };
	auto Gioca = [this](bool bGuardPrima, FEsito& Out)
	{
		FLockInBench B;
		if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
		const int32 Guard = RTAbilityFixtures::AddCoreAbility(B.Riktor, TEXT("Action.Guard"));
		const int32 Brace = RTAbilityFixtures::AddCoreAbility(B.Riktor, TEXT("Action.Brace"));
		const TArray<int32> Ordine = bGuardPrima ? TArray<int32>{ Guard, Brace, B.Impact } : TArray<int32>{ B.Impact, Brace, Guard };
		for (const int32 A : Ordine) { B.Riktor->DeclarePlannedAction(A); }
		B.Riktor->DeclareAttackOnUnit(B.Gadget);
		if (!TestTrue(TEXT("premessa: 1 + 1 + 2 = 4 e' legale"), LockInVerdictOf(B.Riktor).bLegal)) { DestroyInteractionWorld(B.World); return false; }
		ResolveOneTurn(B.TM);
		const URTHexMapAsset* Mappa = nullptr;
		for (TActorIterator<ARTHexMapActor> It(B.World); It; ++It) { Mappa = It->MapAsset; }
		Out.Stato = URTMatchStateHashLibrary::HashMatchState(Mappa, URTMatchStateHashLibrary::BuildUnitDigests({ B.Riktor, B.Gadget }), {});
		Out.Attivazioni = ActivationsOf(B.TM, B.Riktor, ERTMatchPhase::Prep);
		Out.Attivazioni.Append(ActivationsOf(B.TM, B.Riktor, ERTMatchPhase::Blast));
		Out.Voci = B.TM->GetTurnLog();
		Out.VitaGadget = B.Gadget->Health;
		DestroyInteractionWorld(B.World);
		return true;
	};
	FEsito A, Bx;
	if (!Gioca(true, A) || !Gioca(false, Bx)) { return false; }
	auto SenzaOrdine = [](TArray<FRTTurnLogEntry> Voci)
	{
		for (FRTTurnLogEntry& E : Voci) { E.DeclarationIndex = INDEX_NONE; }
		URTTurnLogLibrary::SortTurnLog(Voci);
		return Voci;
	};
	auto IndiceDi = [](const TArray<FRTTurnLogEntry>& Voci, ERTMatchPhase Fase, ERTLogCategory Cat, int32 UnitId)
	{
		for (const FRTTurnLogEntry& E : Voci) { if (E.Phase == Fase && E.Category == Cat && (UnitId == 0 || E.UnitId == UnitId)) { return E.DeclarationIndex; } }
		return -99;
	};
	TestTrue(TEXT("controllo: il colpo e' partito"), A.VitaGadget < URTHeroCatalogLibrary::MakeGadget()->MaxHealth);
	TestEqual(TEXT("stesso stato finale"), A.Stato, Bx.Stato);
	TestEqual(TEXT("stesse attivazioni, nello stesso ordine"), A.Attivazioni, Bx.Attivazioni);
	TestEqual(TEXT("il colpo porta 2 in un ordine"), IndiceDi(A.Voci, ERTMatchPhase::Blast, ERTLogCategory::Combat, 0), 2);
	TestEqual(TEXT("e 0 nell'altro"), IndiceDi(Bx.Voci, ERTMatchPhase::Blast, ERTLogCategory::Combat, 0), 0);
	const uint32 HashA = URTTurnLogLibrary::HashTurnLog(A.Voci);
	const uint32 HashB = URTTurnLogLibrary::HashTurnLog(Bx.Voci);
	TestNotEqual(TEXT("controllo positivo: gli hash di traccia sono diversi"), HashA, HashB);
	TestEqual(TEXT("e coincidono portando il campo a INDEX_NONE"), URTTurnLogLibrary::HashTurnLog(SenzaOrdine(A.Voci)), URTTurnLogLibrary::HashTurnLog(SenzaOrdine(Bx.Voci)));
	const TArray<FRTTurnLogEntry> VA = SenzaOrdine(A.Voci);
	const TArray<FRTTurnLogEntry> VB = SenzaOrdine(Bx.Voci);
	if (TestEqual(TEXT("stesse voci di TurnLog"), VA.Num(), VB.Num()))
	{
		for (int32 i = 0; i < VA.Num(); ++i) { TestEqual(*FString::Printf(TEXT("voce %d, senza l'ordine"), i), DescriviVoce(VA[i]), DescriviVoce(VB[i])); }
	}
	// E38 CP 38.16 (#4117, criterio 1): lo stesso con scatto + Move. Ram verso (2,0) e un Move da (2,0) a (2,1), dichiarati nei due
	// ordini: stesso stato e stesse voci di TurnLog senza il campo d'ordine; scatto e Move portano ciascuno il proprio indice, e
	// l'hash di traccia cambia ([D-615] (2) e (5)). Lo scatto risolve prima perche' e' della macro-fase Dash, non perche' e' dichiarato
	// prima.
	struct FEsitoScatto { uint32 Stato = 0; FRTCellId Cella; TArray<FRTTurnLogEntry> Voci; int32 IndiceDelMove = -99; };
	auto GiocaScatto = [this](bool bScattoPrima, FEsitoScatto& Out)
	{
		FLockInBench B;
		if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
		auto Scatta = [&B]() { B.Riktor->SetPlannedDash(B.Ram, FRTCellId(2, 0)); };
		auto Cammina = [&B]() { B.Riktor->SetPlannedMove({ FRTCellId(2, 1) }, { FRTCellId(2, 0), FRTCellId(2, 1) }); };
		if (bScattoPrima) { Scatta(); Cammina(); } else { Cammina(); Scatta(); }
		if (!TestTrue(TEXT("premessa: scatto + Move, 2 + 2 = 4, e' legale"), LockInVerdictOf(B.Riktor).bLegal)) { DestroyInteractionWorld(B.World); return false; }
		ResolveOneTurn(B.TM);
		const URTHexMapAsset* Mappa = nullptr;
		for (TActorIterator<ARTHexMapActor> It(B.World); It; ++It) { Mappa = It->MapAsset; }
		Out.Stato = URTMatchStateHashLibrary::HashMatchState(Mappa, URTMatchStateHashLibrary::BuildUnitDigests({ B.Riktor, B.Gadget }), {});
		Out.Cella = B.Riktor->Cell;
		Out.Voci = B.TM->GetTurnLog();
		for (const FRTTurnLogEntry& E : Out.Voci)
		{
			if (E.UnitId == B.Riktor->StableUnitId && E.Phase == ERTMatchPhase::Move && E.Category == ERTLogCategory::Move) { Out.IndiceDelMove = E.DeclarationIndex; }
		}
		DestroyInteractionWorld(B.World);
		return true;
	};
	FEsitoScatto SA, SB;
	if (!GiocaScatto(true, SA) || !GiocaScatto(false, SB)) { return false; }
	TestEqual(TEXT("scatto + Move: Riktor finisce dopo lo scatto e il Move"), SA.Cella, FRTCellId(2, 1));
	TestEqual(TEXT("scatto + Move: stessa cella nei due ordini"), SA.Cella, SB.Cella);
	TestEqual(TEXT("scatto + Move: stesso stato finale"), SA.Stato, SB.Stato);
	TestEqual(TEXT("scatto + Move: il Move porta 1 dichiarato dopo lo scatto"), SA.IndiceDelMove, 1);
	TestEqual(TEXT("e 0 dichiarato prima"), SB.IndiceDelMove, 0);
	TestNotEqual(TEXT("scatto + Move, controllo positivo: gli hash di traccia sono diversi"),
		URTTurnLogLibrary::HashTurnLog(SA.Voci), URTTurnLogLibrary::HashTurnLog(SB.Voci));
	TestEqual(TEXT("scatto + Move: e coincidono portando il campo a INDEX_NONE"),
		URTTurnLogLibrary::HashTurnLog(SenzaOrdine(SA.Voci)), URTTurnLogLibrary::HashTurnLog(SenzaOrdine(SB.Voci)));
	{
		const TArray<FRTTurnLogEntry> XA = SenzaOrdine(SA.Voci);
		const TArray<FRTTurnLogEntry> XB = SenzaOrdine(SB.Voci);
		if (TestEqual(TEXT("scatto + Move: stesse voci di TurnLog"), XA.Num(), XB.Num()))
		{
			for (int32 i = 0; i < XA.Num(); ++i) { TestEqual(*FString::Printf(TEXT("scatto + Move, voce %d, senza l'ordine"), i), DescriviVoce(XA[i]), DescriviVoce(XB[i])); }
		}
	}

	// E38 CP 38.15 (#4116, criterio 6): lo stesso con un piano di DUE reazioni, che scattano entrambe sullo stesso colpo di
	// Gadget ([D-619]). Dichiarate nei due ordini, danno lo stesso stato, gli stessi eventi di gioco nello stesso ordine e le
	// stesse voci; cambiano gli indici delle voci delle reazioni ([D-615] (2) e (5)), e con loro l'hash di traccia.
	struct FEsitoReazioni { uint32 Stato = 0; TArray<FString> Eventi; TArray<FRTTurnLogEntry> Voci; int32 Attivate = 0; };
	auto GiocaReazioni = [this](bool bPrimaLaB, FEsitoReazioni& Out)
	{
		FLockInBench B;
		if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
		const int32 RA = RTAbilityFixtures::AddCoreAbility(B.Riktor, TEXT("Action.Counter"));
		const int32 RB = RTAbilityFixtures::AddCoreAbility(B.Riktor, TEXT("Action.Counter"));
		URTActionData* Seconda = B.Riktor->GetAbility(RB);
		Seconda->Def.ActionId = FName(TEXT("Test.Counter.Prima"));
		Seconda->Def.Priority = B.Riktor->GetAbility(RA)->Def.Priority - 1;
		B.Riktor->ClearPlannedMainAction();
		TestTrue(TEXT("la prima reazione si arma"), B.Riktor->AddPlannedReaction(bPrimaLaB ? RB : RA));
		TestTrue(TEXT("e la seconda si accoda"), B.Riktor->AddPlannedReaction(bPrimaLaB ? RA : RB));
		if (!TestTrue(TEXT("premessa: il piano di due reazioni e' legale"), LockInVerdictOf(B.Riktor).bLegal)) { DestroyInteractionWorld(B.World); return false; }
		B.Gadget->SetPlannedMainAction(0);
		B.Gadget->DeclareAttackOnUnit(B.Riktor);
		ResolveOneTurn(B.TM);
		const URTHexMapAsset* Mappa = nullptr;
		for (TActorIterator<ARTHexMapActor> It(B.World); It; ++It) { Mappa = It->MapAsset; }
		Out.Stato = URTMatchStateHashLibrary::HashMatchState(Mappa, URTMatchStateHashLibrary::BuildUnitDigests({ B.Riktor, B.Gadget }), {});
		for (const FRTResolvedEvent& Ev : B.TM->ResolvedTimelineForTest())
		{
			Out.Eventi.Add(FString::Printf(TEXT("%d/%d %d->%d %s %d"), (int32)Ev.Phase, (int32)Ev.Type, Ev.SourceStableUnitId,
				Ev.TargetStableUnitId, *Ev.ActionId.ToString(), Ev.Amount));
		}
		Out.Voci = B.TM->GetTurnLog();
		for (const FRTTurnLogEntry& E : Out.Voci)
		{
			if (E.Category == ERTLogCategory::Reaction && E.UnitId == B.Riktor->StableUnitId
				&& E.Outcome == static_cast<uint8>(ERTReactionOutcome::Activated)) { ++Out.Attivate; }
		}
		DestroyInteractionWorld(B.World);
		return true;
	};
	FEsitoReazioni RA_, RB_;
	if (!GiocaReazioni(false, RA_) || !GiocaReazioni(true, RB_)) { return false; }
	TestEqual(TEXT("premessa: le due reazioni scattano"), RA_.Attivate, 2);
	TestEqual(TEXT("anche nell'altro ordine"), RB_.Attivate, 2);
	TestEqual(TEXT("due reazioni: stesso stato finale"), RA_.Stato, RB_.Stato);
	TestEqual(TEXT("due reazioni: stessi eventi di gioco, nello stesso ordine"), RA_.Eventi, RB_.Eventi);
	auto IndiceDellaReazione = [](const TArray<FRTTurnLogEntry>& Voci, FName ActionId)
	{
		for (const FRTTurnLogEntry& E : Voci) { if (E.Category == ERTLogCategory::Reaction && E.ActionId == ActionId) { return E.DeclarationIndex; } }
		return -99;
	};
	const FName Prima(TEXT("Test.Counter.Prima"));
	TestEqual(TEXT("la reazione dichiarata seconda porta 1"), IndiceDellaReazione(RA_.Voci, Prima), 1);
	TestEqual(TEXT("e dichiarata prima porta 0"), IndiceDellaReazione(RB_.Voci, Prima), 0);
	TestNotEqual(TEXT("due reazioni, controllo positivo: gli hash di traccia sono diversi"),
		URTTurnLogLibrary::HashTurnLog(RA_.Voci), URTTurnLogLibrary::HashTurnLog(RB_.Voci));
	TestEqual(TEXT("e coincidono portando il campo a INDEX_NONE"),
		URTTurnLogLibrary::HashTurnLog(SenzaOrdine(RA_.Voci)), URTTurnLogLibrary::HashTurnLog(SenzaOrdine(RB_.Voci)));
	const TArray<FRTTurnLogEntry> RVA = SenzaOrdine(RA_.Voci);
	const TArray<FRTTurnLogEntry> RVB = SenzaOrdine(RB_.Voci);
	if (TestEqual(TEXT("due reazioni: stesse voci di TurnLog"), RVA.Num(), RVB.Num()))
	{
		for (int32 i = 0; i < RVA.Num(); ++i) { TestEqual(*FString::Printf(TEXT("due reazioni, voce %d, senza l'ordine"), i), DescriviVoce(RVA[i]), DescriviVoce(RVB[i])); }
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerInputArmingASecondPrepActionAddsItTest,
	"RefactorTactics.PlayerInput.ArmingASecondPrepActionAddsIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerInputArmingASecondPrepActionAddsItTest::RunTest(const FString&)
{
	// Il gesto della lista: con la principale presa, armare un'azione di Prep la AGGIUNGE, e il bersaglio della principale
	// resta. Il controllo: con la principale libera la stessa azione va nella principale, come prima.
	FLockInBench B;
	if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
	const int32 Guard = RTAbilityFixtures::AddCoreAbility(B.Riktor, TEXT("Action.Guard"));
	B.PC->SelectActorForTest(B.Riktor);
	B.Riktor->DeclarePlannedMainAction(B.Impact);
	B.Riktor->DeclareAttackOnUnit(B.Gadget);
	B.PC->SelectAbilityForCurrentForTest(Guard);
	const FRTUnitPlan& P = B.Riktor->GetPlan();
	TestEqual(TEXT("la principale resta ImpactShot"), P.PlannedAbilityIndex(), B.Impact);
	TestTrue(TEXT("col suo bersaglio"), P.PlannedAttackTarget() == B.Gadget);
	if (TestEqual(TEXT("la Guard e' una voce Prep, in coda"), P.DeclarationIndexOf(ERTPlanEntryClass::Prep), 1))
	{
		TestEqual(TEXT("ed e' la Guard"), P.GetEntries()[1].AbilityIndex, Guard);
	}
	DestroyInteractionWorld(B.World);

	FLockInBench C;
	if (!MakeLockInBench(*this, C)) { DestroyInteractionWorld(C.World); return false; }
	const int32 GuardC = RTAbilityFixtures::AddCoreAbility(C.Riktor, TEXT("Action.Guard"));
	C.PC->SelectActorForTest(C.Riktor);
	C.PC->SelectAbilityForCurrentForTest(GuardC);
	TestEqual(TEXT("controllo: con la principale libera la Guard va nella principale"), C.Riktor->GetPlan().PlannedAbilityIndex(), GuardC);
	TestEqual(TEXT("e nessuna voce Prep"), C.Riktor->GetPlan().DeclarationIndexOf(ERTPlanEntryClass::Prep), (int32)INDEX_NONE);
	DestroyInteractionWorld(C.World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerInputBackRemovesTheLastDeclaredEntryTest,
	"RefactorTactics.PlayerInput.BackRemovesTheLastDeclaredEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerInputBackRemovesTheLastDeclaredEntryTest::RunTest(const FString&)
{
	// Il Back toglie l'ultima voce dichiarata, l'azione di Prep che il gesto ha appena aggiunto; la principale resta.
	FLockInBench B;
	if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
	const int32 Guard = RTAbilityFixtures::AddCoreAbility(B.Riktor, TEXT("Action.Guard"));
	B.PC->SelectActorForTest(B.Riktor);
	B.Riktor->DeclarePlannedMainAction(B.Impact);
	B.Riktor->DeclareAttackOnUnit(B.Gadget);
	B.PC->SelectAbilityForCurrentForTest(Guard);
	if (!TestEqual(TEXT("premessa: la Guard e' stata aggiunta"), B.Riktor->GetPlan().DeclarationIndexOf(ERTPlanEntryClass::Prep), 1))
	{
		DestroyInteractionWorld(B.World);
		return false;
	}
	B.PC->ApplyBack();
	TestEqual(TEXT("il Back toglie la voce Prep"), B.Riktor->GetPlan().DeclarationIndexOf(ERTPlanEntryClass::Prep), (int32)INDEX_NONE);
	TestEqual(TEXT("la principale resta"), B.Riktor->GetPlan().PlannedAbilityIndex(), B.Impact);
	TestTrue(TEXT("col suo bersaglio"), B.Riktor->GetPlan().PlannedAttackTarget() == B.Gadget);
	DestroyInteractionWorld(B.World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUIRevealedEnemySeesOneEntryOfAListPlanTest,
	"RefactorTactics.UI.RevealedEnemySeesOneEntryOfAListPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUIRevealedEnemySeesOneEntryOfAListPlanTest::RunTest(const FString&)
{
	// Criterio 11: due piani che differiscono solo per le copie o per l'ordine di dichiarazione danno a un avversario
	// rivelato la stessa vista, una voce sola; un alleato riceve le voci nell'ordine di dichiarazione.
	struct FViste { FRTIntentView Nemico; FRTIntentView Alleato; int32 Viste = 0; };
	auto Componi = [this](const TArray<int32>& Ordine, FViste& Out)
	{
		FLockInBench B;
		if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
		const int32 Guard = RTAbilityFixtures::AddCoreAbility(B.Riktor, TEXT("Action.Guard"));
		B.Riktor->Abilities[Guard]->DisplayName = FText::FromString(TEXT("Guardia")); // la fixture non da' un nome mostrato
		for (const int32 Codice : Ordine) { B.Riktor->DeclarePlannedAction(Codice == 0 ? Guard : B.Impact); }
		if (Ordine.Contains(1)) { B.Riktor->DeclareAttackOnUnit(B.Gadget); }
		TArray<FRTPlannedIntent> Intenti = URTHudViewModel::BuildAuthoritativeIntents({ B.Riktor });
		for (FRTPlannedIntent& I : Intenti) { I.bRevealed = true; }
		const TArray<FRTIntentView> PerNemico = URTIntentPrivacyLibrary::FilterForTeam(B.Gadget->TeamId, Intenti);
		const TArray<FRTIntentView> PerAlleato = URTIntentPrivacyLibrary::FilterForTeam(B.Riktor->TeamId, Intenti);
		Out.Viste = PerNemico.Num() + PerAlleato.Num();
		if (PerNemico.Num() == 1) { Out.Nemico = PerNemico[0]; }
		if (PerAlleato.Num() == 1) { Out.Alleato = PerAlleato[0]; }
		DestroyInteractionWorld(B.World);
		return true;
	};
	FViste GuardPrima, ColpoPrima, DueGuardie, SoloGuardia;
	if (!Componi({ 0, 1 }, GuardPrima) || !Componi({ 1, 0 }, ColpoPrima) || !Componi({ 0, 0, 1 }, DueGuardie)
		|| !Componi({ 0 }, SoloGuardia))
	{
		return false;
	}
	TestEqual(TEXT("premessa: una vista per osservatore"), GuardPrima.Viste, 2);
	// Il livello di certezza e' della voce che il nemico vede: non dice che dietro la Guard c'e' un colpo.
	for (const FViste* Piano : { &GuardPrima, &ColpoPrima, &DueGuardie })
	{
		TestEqual(TEXT("lo stesso livello di certezza di una Guard sola"), Piano->Nemico.Certainty, SoloGuardia.Nemico.Certainty);
		TestTrue(TEXT("e la stessa azione"), Piano->Nemico.ActionName.EqualTo(SoloGuardia.Nemico.ActionName));
	}

	for (const FViste* Altro : { &ColpoPrima, &DueGuardie })
	{
		TestTrue(TEXT("il nemico rivelato vede la stessa azione"), Altro->Nemico.ActionName.EqualTo(GuardPrima.Nemico.ActionName));
		TestEqual(TEXT("con lo stesso bersaglio"), Altro->Nemico.bHasTarget, GuardPrima.Nemico.bHasTarget);
		TestEqual(TEXT("sulla stessa cella"), Altro->Nemico.TargetCell, GuardPrima.Nemico.TargetCell);
		TestEqual(TEXT("e nessun elenco"), Altro->Nemico.DeclaredActionNames.Num(), 0);
	}
	TestFalse(TEXT("il nemico vede un'azione"), GuardPrima.Nemico.ActionName.IsEmpty());
	TestEqual(TEXT("nessun elenco al nemico"), GuardPrima.Nemico.DeclaredActionNames.Num(), 0);

	// L'alleato: le voci nell'ordine di dichiarazione, che i due piani rovesciano.
	if (TestEqual(TEXT("l'alleato riceve due voci"), GuardPrima.Alleato.DeclaredActionNames.Num(), 2)
		&& TestEqual(TEXT("anche nell'altro ordine"), ColpoPrima.Alleato.DeclaredActionNames.Num(), 2))
	{
		TestTrue(TEXT("nell'ordine di dichiarazione"), GuardPrima.Alleato.DeclaredActionNames[0].EqualTo(ColpoPrima.Alleato.DeclaredActionNames[1]));
		TestFalse(TEXT("controllo: i due ordini sono davvero diversi"),
			GuardPrima.Alleato.DeclaredActionNames[0].EqualTo(ColpoPrima.Alleato.DeclaredActionNames[0]));
	}
	TestEqual(TEXT("con due copie l'alleato ne riceve tre"), DueGuardie.Alleato.DeclaredActionNames.Num(), 3);
	return true;
}

// ── E38 CP 38.14 (#4115): il Blast e il Cleanup a piu' azioni della stessa unita' ─────────────────────────────────────
namespace
{
	/** Una riga del Blast sintetica: un attacco a portata 3, alla priorita' data, con un danno e una politica di interruzione. */
	int32 AddSyntheticBlastRow(ARTUnit* Unit, const TCHAR* ActionId, int32 Priority, int32 Damage,
		ERTInterruptPolicy Policy = ERTInterruptPolicy::None)
	{
		URTActionData* Action = NewObject<URTActionData>(Unit);
		Action->Def.ActionId = FName(ActionId);
		Action->Def.ResolutionPhase = ERTResolutionPhase::Attack;
		Action->Def.Priority = Priority;
		Action->Def.Slot = ERTActionSlot::Main;
		Action->Def.ActionPointCost = 2;
		Action->Def.RangeCells = 3;
		Action->Def.bCountsAsAttack = true;
		Action->Def.InterruptPolicy = Policy;
		Action->Def.Fallback = ERTActionFallback::Cancel;
		Action->Def.Effects = { FRTActionEffectSpec(ERTActionEffect::Damage, Damage) };
		Action->RangeCells = 3;
		Action->Power = Damage;
		Unit->Abilities.Add(Action);
		return Unit->Abilities.Num() - 1;
	}

	/**
	 * Dichiara una principale e il suo bersaglio-unita', col fuoco sulla voce appena scritta. Se il gesto rifiuta la voce
	 * non dichiara niente: il test cade sui suoi asserti, non su una voce che non c'e'.
	 */
	void DeclareMainOn(ARTUnit* Unit, int32 Ability, ARTUnit* Target)
	{
		if (!Unit->DeclarePlannedAction(Ability) || Unit->GetPlan().GetEntries().IsEmpty()) { return; }
		const ARTUnit::FMainFocus Fuoco(*Unit, Unit->GetPlan().GetEntries().Last().Id);
		Unit->DeclareAttackOnUnit(Target);
	}

	void DeclareMainOnCell(ARTUnit* Unit, int32 Ability, const FRTCellId& Cell)
	{
		if (!Unit->DeclarePlannedAction(Ability) || Unit->GetPlan().GetEntries().IsEmpty()) { return; }
		const ARTUnit::FMainFocus Fuoco(*Unit, Unit->GetPlan().GetEntries().Last().Id);
		Unit->DeclareAttackOnCell(Cell);
	}

	struct FAttivazione { FName ActionId; int32 Bersaglio = 0; FRTCellId Mira; };
	TArray<FAttivazione> BlastActivationsOf(const ARTTurnManager* TM, const ARTUnit* Unit)
	{
		TArray<FAttivazione> Out;
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::AbilityActivated && Ev.Phase == ERTMatchPhase::Blast && Ev.SourceStableUnitId == Unit->StableUnitId)
			{
				Out.Add({ Ev.ActionId, Ev.TargetStableUnitId, Ev.AimCell });
			}
		}
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTResolutionBlastOrderIgnoresDeclarationTest,
	"RefactorTactics.Resolution.BlastOrderIgnoresDeclaration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTResolutionBlastOrderIgnoresDeclarationTest::RunTest(const FString&)
{
	// Criteri 3 e 4: due azioni del Blast della stessa unita' alla stessa priorita' si attivano per `ActionId`, e due copie
	// su due bersagli per contenuto (il bersaglio), in entrambi gli ordini di dichiarazione.
	auto Gioca = [this](bool bRovescio, bool bCopie, TArray<FAttivazione>& Out)
	{
		FLockInBench B;
		if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
		ARTUnit* Secondo = SpawnInteractionUnit(B.World, 1, URTHeroCatalogLibrary::MakeGadget(), FRTCellId(1, -1));
		if (!TestNotNull(TEXT("il secondo bersaglio"), Secondo)) { DestroyInteractionWorld(B.World); return false; }
		struct FVoce { int32 Abilita; ARTUnit* Bersaglio; };
		TArray<FVoce> Voci;
		if (bCopie)
		{
			Voci = { { B.Impact, B.Gadget }, { B.Impact, Secondo } };
		}
		else
		{
			const int32 A = AddSyntheticBlastRow(B.Riktor, TEXT("Test.BlastA"), 50, 10);
			const int32 Bx = AddSyntheticBlastRow(B.Riktor, TEXT("Test.BlastB"), 50, 10);
			Voci = { { A, B.Gadget }, { Bx, B.Gadget } };
		}
		if (bRovescio) { Swap(Voci[0], Voci[1]); }
		for (const FVoce& V : Voci) { DeclareMainOn(B.Riktor, V.Abilita, V.Bersaglio); }
		if (!TestTrue(TEXT("premessa: il piano e' legale"), LockInVerdictOf(B.Riktor).bLegal)) { DestroyInteractionWorld(B.World); return false; }
		ResolveOneTurn(B.TM);
		Out = BlastActivationsOf(B.TM, B.Riktor);
		DestroyInteractionWorld(B.World);
		return true;
	};
	TArray<FAttivazione> A1, A2, C1, C2;
	if (!Gioca(false, false, A1) || !Gioca(true, false, A2) || !Gioca(false, true, C1) || !Gioca(true, true, C2)) { return false; }
	if (TestEqual(TEXT("due attivazioni"), A1.Num(), 2) && TestEqual(TEXT("anche nell'altro ordine"), A2.Num(), 2))
	{
		TestEqual(TEXT("per ActionId: prima Test.BlastA"), A1[0].ActionId, FName(TEXT("Test.BlastA")));
		TestEqual(TEXT("nell'altro ordine lo stesso"), A2[0].ActionId, FName(TEXT("Test.BlastA")));
	}
	if (TestEqual(TEXT("due copie, due attivazioni"), C1.Num(), 2) && TestEqual(TEXT("anche nell'altro ordine"), C2.Num(), 2))
	{
		TestTrue(TEXT("le copie per bersaglio: prima quello di id minore"), C1[0].Bersaglio < C1[1].Bersaglio);
		TestEqual(TEXT("e nell'altro ordine lo stesso primo"), C2[0].Bersaglio, C1[0].Bersaglio);
		TestEqual(TEXT("e lo stesso secondo"), C2[1].Bersaglio, C1[1].Bersaglio);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTResolutionFrozenAimIsPerEntryTest,
	"RefactorTactics.Resolution.FrozenAimIsPerEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTResolutionFrozenAimIsPerEntryTest::RunTest(const FString&)
{
	// Criterio 8: due attacchi della stessa unita' su due bersagli diversi, e il secondo bersaglio scatta via prima del Blast.
	// La copia al lock-in porta una mira congelata per voce, la cella che ciascun bersaglio aveva al lock-in, e ogni colpo
	// mira alla sua, non a una cella sola e non a quella in cui il bersaglio e' arrivato.
	FLockInBench B;
	if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
	ARTUnit* Secondo = SpawnInteractionUnit(B.World, 1, URTHeroCatalogLibrary::MakeGadget(), FRTCellId(1, -1));
	if (!TestNotNull(TEXT("il secondo bersaglio"), Secondo)) { DestroyInteractionWorld(B.World); return false; }
	const int32 Scatto = RTAbilityFixtures::AddCoreAbility(Secondo, TEXT("Action.Dodge"));
	Secondo->SetPlannedDash(Scatto, FRTCellId(4, -1));
	const FRTCellId CellaPrimo = B.Gadget->Cell;
	const FRTCellId CellaSecondo = Secondo->Cell;
	DeclareMainOn(B.Riktor, B.Impact, B.Gadget);
	DeclareMainOn(B.Riktor, B.Impact, Secondo);
	ResolveOneTurn(B.TM);
	TestNotEqual(TEXT("premessa: il secondo bersaglio si e' spostato prima del Blast"), Secondo->Cell, CellaSecondo);

	int32 Congelate = 0;
	TSet<FRTCellId> Mire;
	for (const FRTPlanEntry& Voce : B.Riktor->GetLockedPlan().Plan.GetEntries())
	{
		if (Voce.Class == ERTPlanEntryClass::Main && Voce.bHasFrozenAim) { ++Congelate; Mire.Add(Voce.FrozenAimCell); }
	}
	TestEqual(TEXT("due mire congelate nella copia, una per voce"), Congelate, 2);
	TestTrue(TEXT("sulla cella di ciascun bersaglio al lock-in"), Mire.Contains(CellaPrimo) && Mire.Contains(CellaSecondo));
	TSet<FRTCellId> Mirate;
	for (const FAttivazione& A : BlastActivationsOf(B.TM, B.Riktor)) { Mirate.Add(A.Mira); }
	TestTrue(TEXT("e i due colpi mirano ciascuno alla sua"), Mirate.Contains(CellaPrimo) && Mirate.Contains(CellaSecondo));
	DestroyInteractionWorld(B.World);

	// Il campo d'ordine distingue le due copie: ogni voce `Combat` di ImpactShot porta l'indice della sua voce del piano.
	FLockInBench C;
	if (!MakeLockInBench(*this, C)) { DestroyInteractionWorld(C.World); return false; }
	ARTUnit* SecondoC = SpawnInteractionUnit(C.World, 1, URTHeroCatalogLibrary::MakeGadget(), FRTCellId(1, -1));
	if (!TestNotNull(TEXT("il secondo bersaglio, fermo"), SecondoC)) { DestroyInteractionWorld(C.World); return false; }
	DeclareMainOn(C.Riktor, C.Impact, C.Gadget);
	DeclareMainOn(C.Riktor, C.Impact, SecondoC);
	ResolveOneTurn(C.TM);
	TArray<int32> Indici;
	for (const FRTTurnLogEntry& E : C.TM->GetTurnLog())
	{
		if (E.Category == ERTLogCategory::Combat && E.ActionId == FName(TEXT("Hero.Riktor.ImpactShot"))) { Indici.Add(E.DeclarationIndex); }
	}
	Indici.Sort();
	TestEqual(TEXT("le voci dei due colpi portano 0 e 1"), Indici, TArray<int32>{ 0, 1 });
	DestroyInteractionWorld(C.World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTResolutionBlastPermutationChangesOnlyTheOrderFieldTest,
	"RefactorTactics.Resolution.BlastPermutationChangesOnlyTheOrderField",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTResolutionBlastPermutationChangesOnlyTheOrderFieldTest::RunTest(const FString&)
{
	// Criterio 9: permutare la dichiarazione di un piano con piu' voci del Blast non cambia stato, eventi e voci; cambiano
	// gli indici, e con loro l'hash di traccia. Controllo positivo come in CP 38.13.
	struct FEsito { uint32 Stato = 0; TArray<FName> Attivazioni; TArray<FRTTurnLogEntry> Voci; };
	auto Gioca = [this](bool bRovescio, FEsito& Out)
	{
		FLockInBench B;
		if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
		if (bRovescio) { DeclareMainOnCell(B.Riktor, B.Mortar, FRTCellId(2, 0)); DeclareMainOn(B.Riktor, B.Impact, B.Gadget); }
		else { DeclareMainOn(B.Riktor, B.Impact, B.Gadget); DeclareMainOnCell(B.Riktor, B.Mortar, FRTCellId(2, 0)); }
		if (!TestTrue(TEXT("premessa: legale"), LockInVerdictOf(B.Riktor).bLegal)) { DestroyInteractionWorld(B.World); return false; }
		ResolveOneTurn(B.TM);
		const URTHexMapAsset* Mappa = nullptr;
		for (TActorIterator<ARTHexMapActor> It(B.World); It; ++It) { Mappa = It->MapAsset; }
		Out.Stato = URTMatchStateHashLibrary::HashMatchState(Mappa, URTMatchStateHashLibrary::BuildUnitDigests({ B.Riktor, B.Gadget }), {});
		for (const FAttivazione& A : BlastActivationsOf(B.TM, B.Riktor)) { Out.Attivazioni.Add(A.ActionId); }
		Out.Voci = B.TM->GetTurnLog();
		DestroyInteractionWorld(B.World);
		return true;
	};
	FEsito A, Bx;
	if (!Gioca(false, A) || !Gioca(true, Bx)) { return false; }
	auto SenzaOrdine = [](TArray<FRTTurnLogEntry> Voci)
	{
		for (FRTTurnLogEntry& E : Voci) { E.DeclarationIndex = INDEX_NONE; }
		URTTurnLogLibrary::SortTurnLog(Voci);
		return Voci;
	};
	TestEqual(TEXT("premessa: due attivazioni nel Blast"), A.Attivazioni.Num(), 2);
	TestEqual(TEXT("stesso stato finale"), A.Stato, Bx.Stato);
	TestEqual(TEXT("stesse attivazioni, nello stesso ordine"), A.Attivazioni, Bx.Attivazioni);
	TestNotEqual(TEXT("controllo positivo: gli hash di traccia sono diversi"), URTTurnLogLibrary::HashTurnLog(A.Voci), URTTurnLogLibrary::HashTurnLog(Bx.Voci));
	TestEqual(TEXT("e coincidono portando il campo a INDEX_NONE"), URTTurnLogLibrary::HashTurnLog(SenzaOrdine(A.Voci)), URTTurnLogLibrary::HashTurnLog(SenzaOrdine(Bx.Voci)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTResolutionCleanupVisitsAUnitByPriorityThenActionIdTest,
	"RefactorTactics.Resolution.CleanupVisitsAUnitByPriorityThenActionId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTResolutionCleanupVisitsAUnitByPriorityThenActionIdTest::RunTest(const FString&)
{
	// Criterio 10, a pari priorita': `Action.CreateWater` e `Action.Ignite`, entrambe a 60, su due celle a distanza 2. In
	// entrambi gli ordini di dichiarazione tutte le voci `Environment` dell'acqua vengono prima di quelle del fuoco.
	auto Gioca = [this](bool bFuocoPrima, TArray<FName>& Out)
	{
		FLockInBench B;
		if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
		const int32 Acqua = RTAbilityFixtures::AddCoreAbility(B.Riktor, TEXT("Action.CreateWater"));
		const int32 Fuoco = RTAbilityFixtures::AddCoreAbility(B.Riktor, TEXT("Action.Ignite"));
		if (bFuocoPrima) { DeclareMainOnCell(B.Riktor, Fuoco, FRTCellId(2, 0)); DeclareMainOnCell(B.Riktor, Acqua, FRTCellId(0, 2)); }
		else { DeclareMainOnCell(B.Riktor, Acqua, FRTCellId(0, 2)); DeclareMainOnCell(B.Riktor, Fuoco, FRTCellId(2, 0)); }
		if (!TestTrue(TEXT("premessa: due ambientali, legali"), LockInVerdictOf(B.Riktor).bLegal)) { DestroyInteractionWorld(B.World); return false; }
		ResolveOneTurn(B.TM);
		for (const FRTTurnLogEntry& E : B.TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Environment && E.Phase == ERTMatchPhase::Cleanup
				&& (E.ActionId == FName(TEXT("Action.CreateWater")) || E.ActionId == FName(TEXT("Action.Ignite"))))
			{
				Out.Add(E.ActionId);
			}
		}
		DestroyInteractionWorld(B.World);
		return true;
	};
	TArray<FName> AcquaPrima, FuocoPrima;
	if (!Gioca(false, AcquaPrima) || !Gioca(true, FuocoPrima)) { return false; }
	auto AcquaDavanti = [](const TArray<FName>& Voci)
	{
		const int32 UltimaAcqua = Voci.FindLast(FName(TEXT("Action.CreateWater")));
		const int32 PrimoFuoco = Voci.Find(FName(TEXT("Action.Ignite")));
		return UltimaAcqua != INDEX_NONE && PrimoFuoco != INDEX_NONE && UltimaAcqua < PrimoFuoco;
	};
	TestTrue(TEXT("dichiarata prima l'acqua: le sue voci vengono prima"), AcquaDavanti(AcquaPrima));
	TestTrue(TEXT("dichiarato prima il fuoco: lo stesso"), AcquaDavanti(FuocoPrima));
	TestEqual(TEXT("e le due sequenze coincidono"), AcquaPrima, FuocoPrima);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTResolutionShooterFacesTheLastResolvedShotTest,
	"RefactorTactics.Resolution.ShooterFacesTheLastResolvedShot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTResolutionShooterFacesTheLastResolvedShotTest::RunTest(const FString&)
{
	// Criterio 13, [D-616] (2): ImpactShot (50) su Gadget a ovest e MortarShot (65) su una cella a est. Riktor finisce il
	// Blast girato verso la mira del mortaio, l'ultimo colpo nell'ordine del resolver, in entrambi gli ordini.
	auto Gioca = [this](bool bMortaioPrima, ERTHexDirection& Out)
	{
		FLockInBench B;
		if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
		if (bMortaioPrima) { DeclareMainOnCell(B.Riktor, B.Mortar, FRTCellId(2, 0)); DeclareMainOn(B.Riktor, B.Impact, B.Gadget); }
		else { DeclareMainOn(B.Riktor, B.Impact, B.Gadget); DeclareMainOnCell(B.Riktor, B.Mortar, FRTCellId(2, 0)); }
		ResolveOneTurn(B.TM);
		Out = B.Riktor->Facing;
		DestroyInteractionWorld(B.World);
		return true;
	};
	ERTHexDirection Uno = ERTHexDirection::E, Due = ERTHexDirection::E;
	if (!Gioca(false, Uno) || !Gioca(true, Due)) { return false; }
	ERTHexDirection VersoIlMortaio = ERTHexDirection::E;
	URTHexLibrary::DirectionTowards(FRTCellId(0, 0), FRTCellId(2, 0), VersoIlMortaio);
	ERTHexDirection VersoGadget = ERTHexDirection::E;
	URTHexLibrary::DirectionTowards(FRTCellId(0, 0), FRTCellId(-1, 0), VersoGadget);
	TestNotEqual(TEXT("premessa: i due colpi sono su due lati"), VersoIlMortaio, VersoGadget);
	TestEqual(TEXT("girato verso il mortaio"), Uno, VersoIlMortaio);
	TestEqual(TEXT("anche dichiarando prima il mortaio"), Due, VersoIlMortaio);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTResolutionTwoImpactShotsPushTwiceTest,
	"RefactorTactics.Resolution.TwoImpactShotsPushTwice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTResolutionTwoImpactShotsPushTwiceTest::RunTest(const FString&)
{
	// Criterio 14, [D-616] (3): due ImpactShot con `Weapon.Impact` della stessa unita' sullo stesso bersaglio lo spingono di
	// 2: le spinte dello stesso attaccante si sommano anche fra due azioni. Controllo: un solo colpo spinge di 1.
	auto Gioca = [this](int32 Colpi, int32& Spinta)
	{
		FLockInBench B;
		if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
		B.Riktor->EquipLoadout({ FName(TEXT("Weapon.Impact")) });
		const FRTCellId Prima = B.Gadget->Cell;
		for (int32 k = 0; k < Colpi; ++k) { DeclareMainOn(B.Riktor, B.Impact, B.Gadget); }
		ResolveOneTurn(B.TM);
		Spinta = URTHexLibrary::HexDistance(Prima, B.Gadget->Cell);
		DestroyInteractionWorld(B.World);
		return true;
	};
	int32 UnColpo = -1, DueColpi = -1;
	if (!Gioca(1, UnColpo) || !Gioca(2, DueColpi)) { return false; }
	TestEqual(TEXT("controllo: un colpo spinge di 1"), UnColpo, 1);
	TestEqual(TEXT("due colpi spingono di 2"), DueColpi, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTResolutionInterruptReachesEveryBlastActionOfItsTargetTest,
	"RefactorTactics.Resolution.InterruptReachesEveryBlastActionOfItsTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTResolutionInterruptReachesEveryBlastActionOfItsTargetTest::RunTest(const FString&)
{
	// Criterio 12, [D-616] (1): un Interrupt su un bersaglio con due azioni `InterruptBeforeEffect` del Blast le annulla
	// entrambe. Su righe sintetiche, perche' nessun eroe ne porta due. Controllo: senza Interrupt le due azioni colpiscono.
	auto Gioca = [this](bool bInterrompe, int32& Interrotte, int32& Colpi)
	{
		FLockInBench B;
		if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
		const int32 A = AddSyntheticBlastRow(B.Gadget, TEXT("Test.PesanteA"), 60, 10, ERTInterruptPolicy::InterruptBeforeEffect);
		const int32 Bx = AddSyntheticBlastRow(B.Gadget, TEXT("Test.PesanteB"), 60, 10, ERTInterruptPolicy::InterruptBeforeEffect);
		DeclareMainOn(B.Gadget, A, B.Riktor);
		DeclareMainOn(B.Gadget, Bx, B.Riktor);
		if (bInterrompe)
		{
			const int32 Interrupt = RTAbilityFixtures::AddCoreAbility(B.Riktor, TEXT("Action.Interrupt"));
			DeclareMainOn(B.Riktor, Interrupt, B.Gadget);
		}
		ResolveOneTurn(B.TM);
		Interrotte = 0;
		Colpi = 0;
		for (const FRTTurnLogEntry& E : B.TM->GetTurnLog())
		{
			const bool bSua = E.ActionId == FName(TEXT("Test.PesanteA")) || E.ActionId == FName(TEXT("Test.PesanteB"));
			if (bSua && E.Category == ERTLogCategory::Fallback && E.Amount == static_cast<int32>(ERTActionInvalidReason::Interrupted)) { ++Interrotte; }
			if (bSua && E.Category == ERTLogCategory::Combat) { ++Colpi; }
		}
		DestroyInteractionWorld(B.World);
		return true;
	};
	int32 Interrotte = 0, Colpi = 0, InterrotteSenza = 0, ColpiSenza = 0;
	if (!Gioca(true, Interrotte, Colpi) || !Gioca(false, InterrotteSenza, ColpiSenza)) { return false; }
	TestEqual(TEXT("controllo: senza Interrupt le due azioni colpiscono"), ColpiSenza, 2);
	TestEqual(TEXT("l'Interrupt le annulla entrambe"), Interrotte, 2);
	TestEqual(TEXT("e nessuna colpisce"), Colpi, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTResolutionCleanupKeepsTheUnitOrderAcrossUnitsTest,
	"RefactorTactics.Resolution.CleanupKeepsTheUnitOrderAcrossUnits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTResolutionCleanupKeepsTheUnitOrderAcrossUnitsTest::RunTest(const FString&)
{
	// Criterio 15, [D-616] (4): fra unita' diverse il Cleanup tiene l'ordine delle unita' di oggi, non la priorita'. Riktor
	// con `Gadget.Sprinkler` (`Action.CreateWater`, 60) sulla cella che viene prima, un Gadget alleato con
	// `Hero.Gadget.ConductiveNode` (`Action.Electrify`, 30) su quella dopo: l'acqua precede la scarica, benche' la scarica
	// abbia la priorita' minore.
	FLockInBench B;
	if (!MakeLockInBench(*this, B)) { DestroyInteractionWorld(B.World); return false; }
	ARTUnit* Alleato = SpawnInteractionUnit(B.World, 0, URTHeroCatalogLibrary::MakeGadget(), FRTCellId(1, 0));
	if (!TestNotNull(TEXT("il Gadget alleato"), Alleato)) { DestroyInteractionWorld(B.World); return false; }
	B.Riktor->EquipLoadout({ FName(TEXT("Gadget.Sprinkler")) });
	const int32 Acqua = LockInAbilityIndex(B.Riktor, TEXT("Gadget.Sprinkler"));
	const int32 Scarica = LockInAbilityIndex(Alleato, TEXT("Hero.Gadget.ConductiveNode"));
	if (!TestTrue(TEXT("premessa: le due ambientali ci sono"), Acqua != INDEX_NONE && Scarica != INDEX_NONE)) { DestroyInteractionWorld(B.World); return false; }
	TestTrue(TEXT("premessa: Riktor viene prima nell'ordine delle unita'"), URTHexLibrary::StableLess(B.Riktor->Cell, Alleato->Cell));
	TestTrue(TEXT("premessa: la scarica ha la priorita' minore"),
		B.Riktor->GetAbility(Acqua)->Def.Priority > Alleato->GetAbility(Scarica)->Def.Priority);
	DeclareMainOnCell(B.Riktor, Acqua, B.Gadget->Cell);
	DeclareMainOn(Alleato, Scarica, B.Gadget);
	ResolveOneTurn(B.TM);
	int32 PosAcqua = INDEX_NONE;
	int32 PosScarica = INDEX_NONE;
	const TArray<FRTTurnLogEntry>& Log = B.TM->GetTurnLog();
	for (int32 k = 0; k < Log.Num(); ++k)
	{
		if (PosAcqua == INDEX_NONE && Log[k].Category == ERTLogCategory::Environment && Log[k].ActionId == FName(TEXT("Gadget.Sprinkler"))) { PosAcqua = k; }
		if (PosScarica == INDEX_NONE && Log[k].Category == ERTLogCategory::Combat && Log[k].ActionId == FName(TEXT("Hero.Gadget.ConductiveNode"))) { PosScarica = k; }
	}
	if (TestTrue(TEXT("premessa: acqua e scarica lasciano le loro voci"), PosAcqua != INDEX_NONE && PosScarica != INDEX_NONE))
	{
		TestTrue(TEXT("l'acqua precede la scarica: vale l'ordine delle unita'"), PosAcqua < PosScarica);
	}
	DestroyInteractionWorld(B.World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
