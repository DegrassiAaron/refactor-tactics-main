// Interazioni del giocatore guidate headless: seleziona, clicca celle, annulla. Non serve un viewport perche'
// ARTPlayerController::HandleClickOnCell riceve la CELLA gia' risolta (il raycast, che il viewport richiede,
// resta in OnSelect). Cio' che questi test NON possono provare e' che a schermo si VEDA qualcosa: l'esagono
// giallo, l'anteprima ciano, le unita' centrate. Quella meta' resta al PIE, per costruzione.

#include "Misc/AutomationTest.h"
#include "Player/RTPlayerController.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Unit/RTUnit.h"
#include "Ability/RTActionData.h"
#include "Turn/RTMovementActionLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"

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
	ARTUnit* Unit = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(2, -2, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("unita'"), Unit))
	{
		DestroyInteractionWorld(World); return false;
	}

	// Senza selezione un click su cella non deve fare nulla (nessun piano fantasma).
	PC->HandleClickOnCell(FRTCellId(3, -2, 0));
	TestEqual(TEXT("nessuna selezione -> nessun waypoint"), Unit->PlannedWaypoints.Num(), 0);

	PC->SelectActorForTest(Unit);

	// Primo click valido: un waypoint, percorso costruito.
	PC->HandleClickOnCell(FRTCellId(3, -2, 0));
	TestEqual(TEXT("primo click -> 1 waypoint"), Unit->PlannedWaypoints.Num(), 1);
	TestTrue(TEXT("il percorso parte dalla cella dell'unita'"),
		Unit->PlannedPath.Num() >= 2 && Unit->PlannedPath[0] == FRTCellId(2, -2, 0));
	TestTrue(TEXT("il percorso finisce sulla cella cliccata"),
		Unit->PlannedPath.Num() >= 2 && Unit->PlannedPath.Last() == FRTCellId(3, -2, 0));

	// Secondo click valido: l'anteprima si allunga.
	const int32 LenAfterOne = Unit->PlannedPath.Num();
	PC->HandleClickOnCell(FRTCellId(3, -1, 0));
	TestEqual(TEXT("secondo click -> 2 waypoint"), Unit->PlannedWaypoints.Num(), 2);
	TestTrue(TEXT("il percorso si e' allungato"), Unit->PlannedPath.Num() > LenAfterOne);

	// Click su cella BLOCCATA: rifiutato, e il piano precedente resta intatto.
	{
		const TArray<FRTCellId> Before = Unit->PlannedPath;
		PC->HandleClickOnCell(FRTCellId(2, 1, 0)); // ostacolo della mappa di prova
		TestEqual(TEXT("cella bloccata -> waypoint non aggiunto"), Unit->PlannedWaypoints.Num(), 2);
		TestTrue(TEXT("il piano precedente e' intatto"), Unit->PlannedPath == Before);
	}

	// Click FUORI dalla mappa: rifiutato allo stesso modo.
	{
		const TArray<FRTCellId> Before = Unit->PlannedPath;
		PC->HandleClickOnCell(FRTCellId(99, 99, 0));
		TestEqual(TEXT("cella fuori mappa -> waypoint non aggiunto"), Unit->PlannedWaypoints.Num(), 2);
		TestTrue(TEXT("il piano precedente e' intatto"), Unit->PlannedPath == Before);
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
			const int32 Before = Unit->PlannedWaypoints.Num();
			PC->HandleClickOnCell(C);
			if (Unit->PlannedWaypoints.Num() == Before) { ++Rejected; }
		}
		TestTrue(TEXT("il budget finisce e i click successivi vengono rifiutati"), Rejected > 0);
		TestTrue(TEXT("il piano resta coerente col budget"),
			Unit->PlannedPath.Num() == 0 || Unit->PlannedPath[0] == FRTCellId(2, -2, 0));
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

	ARTUnit* Unit = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(2, -2, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit) { DestroyInteractionWorld(World); return false; }
	PC->SelectActorForTest(Unit);

	PC->HandleClickOnCell(FRTCellId(3, -2, 0));
	PC->HandleClickOnCell(FRTCellId(3, -1, 0));
	if (!TestEqual(TEXT("due waypoint pianificati"), Unit->PlannedWaypoints.Num(), 2))
	{
		DestroyInteractionWorld(World); return false;
	}
	const int32 LenTwo = Unit->PlannedPath.Num();

	// Un annullamento: un waypoint in meno e percorso piu' corto.
	Unit->PlannedWaypoints.Pop();
	PC->RebuildPlannedPathForTest();
	TestEqual(TEXT("un waypoint dopo l'annullamento"), Unit->PlannedWaypoints.Num(), 1);
	TestTrue(TEXT("il percorso si e' accorciato"), Unit->PlannedPath.Num() < LenTwo);

	// Annullando l'ultimo: nessun movimento pianificato.
	Unit->PlannedWaypoints.Pop();
	PC->RebuildPlannedPathForTest();
	TestEqual(TEXT("nessun percorso pianificato"), Unit->PlannedPath.Num(), 0);
	TestTrue(TEXT("la destinazione torna la cella attuale"), Unit->PlannedCell == Unit->Cell);

	DestroyInteractionWorld(World);
	return true;
}


/**
 * Con una CARICA selezionata, cliccare un nemico la pianifica contro di lui.
 *
 * Prima di #145 il click veniva rifiutato con «lo scatto si pianifica su una CELLA, non su un nemico», e
 * l'unico modo di caricare era cliccare una cella OLTRE il bersaglio e sperare che la traiettoria lo
 * incontrasse: un'affordance che nessuno indovina. Il rifiuto resta giusto per gli scatti che NON sono
 * cariche — quelli si fermano davanti alle unita', quindi puntarne una non ha senso.
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
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	// Guardian e bersaglio allineati sull'asse q, a distanza 3: dentro la portata della Carica (4).
	ARTUnit* Charger = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0, 0));
	ARTUnit* Enemy   = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeWraith(),   FRTCellId(3, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Charger || !Enemy) { DestroyInteractionWorld(World); return false; }

	const int32 ChargeIdx = 3;
	const URTActionData* Charge = Charger->GetAbility(ChargeIdx);
	if (!TestTrue(TEXT("premessa: la quarta abilita' del Guardian e' una carica"),
		Charge && Charge->Def.MovementStyle == ERTMovementStyle::LinearCharge))
	{
		DestroyInteractionWorld(World); return false;
	}

	PC->SelectActorForTest(Charger);
	Charger->SelectAbility(ChargeIdx);
	PC->HandleClickOnUnitForTest(Enemy);

	TestEqual(TEXT("il click sul nemico pianifica la carica"), Charger->PlannedDashAbility, ChargeIdx);
	TestTrue(TEXT("verso la cella del nemico"), Charger->PlannedDashCell == Enemy->Cell);
	// La carica NON e' un attacco del Blast: l'impatto lo produce la fase Dash. Pianificare anche un attacco
	// significherebbe colpire due volte con la stessa azione.
	TestEqual(TEXT("e non pianifica anche un attacco separato"),
		Charger->PlannedAbilityIndex, (int32)INDEX_NONE);

	// Uno scatto che NON e' una carica si ferma davanti alle unita': puntarne una resta senza senso.
	ARTUnit* Dasher = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(-3, 0, 0));
	PC->SelectActorForTest(Dasher);
	Dasher->SelectAbility(3); // Scatto del Ranger: LinearDash
	PC->HandleClickOnUnitForTest(Enemy);
	TestEqual(TEXT("uno scatto non-carica sul nemico resta rifiutato"),
		Dasher->PlannedDashAbility, (int32)INDEX_NONE);

	DestroyInteractionWorld(World);
	return true;
}

/**
 * Lo scatto del giocatore e' LINEARE come quello che il resolver esegue: una cella raggiungibile sul grafo ma
 * NON in linea va rifiutata in pianificazione.
 *
 * Senza questo gate il piano verrebbe accettato e la fase Dash non muoverebbe nulla: il turno si perde in
 * silenzio, senza che nessuno dica perche'. E' lo stesso invariante che il bot ha da #140 ("non proporre mosse
 * che il resolver rifiuta"), qui applicato alla mano umana — diventato osservabile con #142, da quando lo
 * scatto degli archetipi dichiara `LinearDash` e non passa piu' dall'A*.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerDashIsLinearTest,
	"RefactorTactics.PlayerInput.DashRejectsNonLinearDestination",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerDashIsLinearTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(World);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Arena;
	World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

	ARTUnit* Unit = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(2, -2, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit) { DestroyInteractionWorld(World); return false; }

	// La mobilita' rapida si CERCA: l'indice 3 era lo Scatto del Ranger legacy, e dopo la migrazione al
	// roster e' `Wraith.Deflection`, una reazione. `FindDashAbilityIndex` legge la fase dal catalogo, che e'
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

	// (3,-4) e' libera, dentro la mappa e a due passi sul grafo — ma NON e' su una delle sei direzioni.
	const FRTCellId Oblique(3, -4, 0);
	TestTrue(TEXT("premessa: la cella esiste ed e' libera"),
		Arena->ContainsCell(Oblique) && !Arena->FindCell(Oblique)->bBlocksMovement);
	PC->HandleClickOnCell(Oblique);
	TestEqual(TEXT("una destinazione non allineata non diventa un piano di scatto"),
		Unit->PlannedDashAbility, (int32)INDEX_NONE);

	// Controprova: sulla stessa mappa, una cella ALLINEATA e libera viene accettata. Senza, il test passerebbe
	// anche con un gate che rifiuta tutto.
	const FRTCellId Aligned(0, 0, 0); // (2,-2) + 2 * direzione (-1,+1)
	PC->HandleClickOnCell(Aligned);
	TestEqual(TEXT("una destinazione in linea diventa un piano di scatto"), Unit->PlannedDashAbility, DashIdx);
	TestTrue(TEXT("verso la cella cliccata"), Unit->PlannedDashCell == Aligned);

	DestroyInteractionWorld(World);
	return true;
}


// =====================================================================================================
// CP 38.2 — il lato GIOCATORE passa dal validatore del piano.
//
// Il bot ci passa dal 2026-08-25; il giocatore no, ed era l'ultimo consumatore scoperto. Misurato prima di
// scrivere questi test: armando una mobilita', cliccando una cella e poi DISARMANDO per posare waypoint, il
// piano risultava `[Ram(Movimento), Action.Move(Movimento)]` — due movimenti, `SlotOccupied`. Il resolver lo
// assorbiva in silenzio (`ResolveDash` azzera `PlannedPath`), ma `SetPreviewPath` intanto disegnava sulla
// mappa una rotta che l'unita' non avrebbe mai percorso.
//
// ⚠️ I due casi vanno TENUTI INSIEME: un rifiuto che rifiuta tutto sarebbe verde per il motivo sbagliato, e
// il caso che un `if` ingenuo romperebbe e' il secondo — correggere il proprio piano.
// =====================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerDashThenMoveIsRejectedTest,
	"RefactorTactics.PlayerInteraction.MovementSlotTakenRejectsTheWaypoint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerDashThenMoveIsRejectedTest::RunTest(const FString&)
{
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnCleanInteractionMap(World, /*Radius=*/ 6);

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

	// Lo scatto si pianifica: lo slot movimento e' libero.
	U->SelectAbility(DashIdx);
	PC->HandleClickOnCell(FRTCellId(1, 2));
	if (!TestNotEqual(TEXT("premessa: lo scatto e' stato pianificato"),
		U->PlannedDashAbility, static_cast<int32>(INDEX_NONE)))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	// Ora il giocatore torna allo stato neutro (D-128) e prova a posare un waypoint: lo slot movimento e'
	// gia' speso dallo scatto, quindi il waypoint NON si posa.
	U->SelectAbility(INDEX_NONE);
	PC->HandleClickOnCell(FRTCellId(2, 1));

	TestEqual(TEXT("il waypoint non si posa: lo slot movimento e' occupato dallo scatto"),
		U->PlannedWaypoints.Num(), 0);
	TestEqual(TEXT("e la destinazione resta la cella attuale"), U->PlannedCell, U->Cell);
	TestNotEqual(TEXT("lo scatto pianificato non viene toccato dal rifiuto"),
		U->PlannedDashAbility, static_cast<int32>(INDEX_NONE));

	DestroyInteractionWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerCanStillFixHisOwnPlanTest,
	"RefactorTactics.PlayerInteraction.CorrectingYourOwnPlanIsNotRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerCanStillFixHisOwnPlanTest::RunTest(const FString&)
{
	// Il caso che un `if` ingenuo — «se c'e' uno scatto, niente waypoint» — romperebbe senza accorgersene:
	// cambiare idea sul PROPRIO piano non e' occupare due volte uno slot. Vale nei due versi.
	UWorld* World = MakeInteractionWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnCleanInteractionMap(World, /*Radius=*/ 6);

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

	// 1. Allungare un percorso: due waypoint di fila. Il secondo non e' un secondo movimento.
	U->SelectAbility(INDEX_NONE);
	PC->HandleClickOnCell(FRTCellId(1, 2));
	const int32 DopoIlPrimo = U->PlannedWaypoints.Num();
	PC->HandleClickOnCell(FRTCellId(2, 2));

	TestEqual(TEXT("il primo waypoint si posa"), DopoIlPrimo, 1);
	TestEqual(TEXT("e il secondo lo allunga invece di essere rifiutato"), U->PlannedWaypoints.Num(), 2);

	// 2. Cambiare la destinazione dello scatto: ri-cliccare con la mobilita' armata sostituisce, non somma.
	ARTUnit* V = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(-1, -1));
	if (!TestNotNull(TEXT("seconda unita'"), V)) { DestroyInteractionWorld(World); return false; }
	PC->SelectActorForTest(V);

	const int32 DashIdx = V->FindDashAbilityIndex();
	if (!TestNotEqual(TEXT("premessa: mobilita' rapida"), DashIdx, static_cast<int32>(INDEX_NONE)))
	{
		DestroyInteractionWorld(World);
		return false;
	}
	V->SelectAbility(DashIdx);
	PC->HandleClickOnCell(FRTCellId(-1, 0));
	const FRTCellId Prima = V->PlannedDashCell;
	PC->HandleClickOnCell(FRTCellId(0, -1));

	TestNotEqual(TEXT("lo scatto resta pianificato"), V->PlannedDashAbility, static_cast<int32>(INDEX_NONE));
	TestTrue(TEXT("e la destinazione e' cambiata: si sostituisce, non si somma"),
		V->PlannedDashCell != Prima);

	DestroyInteractionWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
