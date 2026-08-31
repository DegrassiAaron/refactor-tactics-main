// Interazioni del giocatore guidate headless: seleziona, clicca celle, annulla. Non serve un viewport perche'
// ARTPlayerController::HandleClickOnCell riceve la CELLA gia' risolta (il raycast, che il viewport richiede,
// resta in OnSelect). Cio' che questi test NON possono provare e' che a schermo si VEDA qualcosa: l'esagono
// giallo, l'anteprima ciano, le unita' centrate. Quella meta' resta al PIE, per costruzione.

#include "Misc/AutomationTest.h"
#include "Player/RTPlayerController.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Unit/RTUnit.h"
#include "Ability/RTActionData.h"
#include "Turn/RTMovementActionLibrary.h"
#include "Turn/RTHexSimLibrary.h" // #1939: lo snapshot e la sua Occupancy, che il test del rifiuto costruisce
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h" // #1776: WorldToLayer e ResolveRayToCellOnLayer, che i due test della review interrogano
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTCatalogLibrary.h"
#include "InputMappingContext.h" // #1771: il test delle collisioni interroga i MapKey reali

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
// CP 38.2 — lo scarto del movimento si DICE, e lo dice chi lo esegue.
//
// Il giocatore puo' comporre scatto + movimento normale: due voci per lo slot movimento. `ResolveDash` fa
// vincere lo scatto e azzera il percorso — fino al 2026-08-26 in silenzio, quindi una rotta disegnata sulla
// mappa spariva senza che niente la nominasse.
//
// 🔴 **La voce si scrive in RISOLUZIONE, non al lock-in**, ed e' la lezione della code review: al commit il
// piano si contraddice e basta, ma CHI verra' scartato lo decide il resolver. L'ordine canonico di
// `ValidatePlan` — per larghezza di slot, poi per `ActionId` — davanti a `Action.Move` e
// `Hero.Riktor.Ram` nomina **Ram**, che invece esegue. Una voce scritta al lock-in avrebbe accusato
// l'azione sbagliata.
//
// I tre test vanno tenuti INSIEME, e il portante e' il TERZO. Il secondo — piano legale senza scatto —
// non attraversa il blocco: `ResolveDash` esce a `DasherCount == 0` prima di arrivarci, quindi un
// `AppendLogEntry` incondizionato lo passerebbe. E' `NoSupersededEntryOnADashWithoutAPlannedMove` a
// coprire quel caso, ed e' il test che il 2026-08-26 ha misurato il falso positivo su ogni scatto.
// =====================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDashSupersedesNormalMoveTest,
	"RefactorTactics.PlayerInteraction.DashSupersedesTheNormalMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDashSupersedesNormalMoveTest::RunTest(const FString&)
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

	// Il giocatore compone davvero il piano incoerente: scatto, poi stato neutro (D-128), poi waypoint.
	// MoveTo e' a DISTANZA 2 da Start, non adiacente, ed e' deliberato: un solo click
	// (`PlannedWaypoints.Num() == 1`) risolve in un percorso di DUE celle (`PlannedPath.Num() - 1 == 2`).
	// `Superseded.Amount` (RTTurnManager.cpp) viene dal percorso RISOLTO, non dal conteggio dei click — con
	// una destinazione adiacente le due quantita' avrebbero coinciso, e l'asserzione su `Amount` piu' sotto
	// non avrebbe morso una regressione che scambiasse la sorgente (rilievo di code review, 2026-08-26).
	const FRTCellId DashTo(1, 2);
	const FRTCellId MoveTo(3, 1);
	U->SelectAbility(DashIdx);
	PC->HandleClickOnCell(DashTo);
	U->SelectAbility(INDEX_NONE);
	PC->HandleClickOnCell(MoveTo);
	// La sorgente VERA di `Amount`: il percorso risolto, non i click posati (vedi il commento sopra).
	const int32 CelleAttese = FMath::Max(0, U->PlannedPath.Num() - 1);

	// Le premesse: senza, il turno non conterrebbe il caso in esame.
	if (!TestNotEqual(TEXT("premessa: lo scatto e' pianificato"), U->PlannedDashAbility, static_cast<int32>(INDEX_NONE))
		|| !TestTrue(TEXT("premessa: il waypoint si posa — l'input non viene rifiutato"),
			U->PlannedWaypoints.Num() > 0)
		|| !TestEqual(TEXT("premessa: la destinazione del movimento e' quella cliccata"), U->PlannedCell, MoveTo)
		|| !TestNotEqual(TEXT("premessa: un click non e' una cella di percorso — le due sorgenti di Amount divergono qui"),
			U->PlannedWaypoints.Num(), CelleAttese))
	{
		DestroyInteractionWorld(World);
		return false;
	}

	TM->LockInAndResolve();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

	// La voce, e i suoi CAMPI: un conteggio da solo non si accorgerebbe di una destinazione sbagliata o di
	// un esito che nomina l'azione che invece esegue.
	int32 Superseded = 0;
	FRTTurnLogEntry Found;
	for (const FRTTurnLogEntry& Entry : TM->GetTurnLog())
	{
		if (Entry.Category == ERTLogCategory::Move
			&& static_cast<ERTMoveOutcome>(Entry.Outcome) == ERTMoveOutcome::SupersededByDash)
		{
			++Superseded;
			Found = Entry;
		}
	}

	if (!TestEqual(TEXT("una voce dichiara il movimento scartato"), Superseded, 1))
	{
		DestroyInteractionWorld(World);
		return false;
	}
	TestEqual(TEXT("la voce sta nella fase in cui lo scarto avviene"), Found.Phase, ERTMatchPhase::Dash);
	TestEqual(TEXT("nomina il MOVIMENTO, non lo scatto che invece esegue"),
		Found.ActionId, FName(TEXT("Action.Move")));
	TestEqual(TEXT("SrcCell e' da dove il movimento sarebbe partito — chiave stabile dell'unita' nel turno"),
		Found.SrcCell, FRTCellId(1, 1));
	TestEqual(TEXT("TgtCell e' la destinazione dichiarata e mai raggiunta"), Found.TgtCell, MoveTo);
	// I due campi che entrano nell'ORDINE CANONICO e nel formato serializzato. Senza queste asserzioni,
	// mettere `Priority` a zero o `Amount` a un numero qualsiasi lascia la suite verde — ed e' esattamente
	// il difetto che la correzione del 2026-08-26 ha chiuso, non difeso da nessuno.
	//
	// `Amount` si confronta con `CelleAttese`, catturata da `PlannedPath.Num() - 1` (il percorso RISOLTO),
	// non da `PlannedWaypoints.Num()` (i CLICK del giocatore): sono due sorgenti diverse. `MoveTo` e' a
	// distanza 2 da Start non per caso, ma perche' a distanza 1 le due quantita' avrebbero coinciso — un
	// click, un passo — e questa asserzione non avrebbe morso una regressione che scambiasse la sorgente
	// (rilievo di code review, 2026-08-26).
	TestEqual(TEXT("Priority viene dal catalogo, non da uno zero implicito"),
		Found.Priority, URTCatalogLibrary::FindCoreAction(TEXT("Action.Move")).Priority);
	TestEqual(TEXT("Amount conta le celle del PERCORSO risolto, non i click posati"), Found.Amount, CelleAttese);

	// E l'unita' e' davvero dove l'ha portata lo scatto: la voce descrive il turno, non lo contraddice.
	TestEqual(TEXT("l'unita' e' sulla cella dello scatto"), U->Cell, DashTo);

	DestroyInteractionWorld(World);
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
	if (!TestEqual(TEXT("premessa: due waypoint compongono un solo movimento"), U->PlannedWaypoints.Num(), 2))
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
	if (!TestNotEqual(TEXT("premessa: lo scatto e' pianificato"), U->PlannedDashAbility, static_cast<int32>(INDEX_NONE))
		|| !TestEqual(TEXT("premessa: nessun waypoint posato"), U->PlannedWaypoints.Num(), 0)
		|| !TestEqual(TEXT("premessa: nessun movimento dichiarato — la destinazione e' la cella attuale"),
			U->PlannedCell, Start))
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

	// Il piano incoerente: scatto, stato neutro (D-128), waypoint. Due azioni sullo slot Movimento.
	U->SelectAbility(DashIdx);
	PC->HandleClickOnCell(FRTCellId(1, 2));
	U->SelectAbility(INDEX_NONE);
	PC->HandleClickOnCell(FRTCellId(2, 1));

	TM->LockInAndResolve();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

	int32 Righe = 0;
	for (const FString& Evento : TM->GetRecentEvents())
	{
		if (Evento.Contains(TEXT("piano non valido al lock-in"))) { ++Righe; }
	}
	TestEqual(TEXT("il lock-in dichiara la contraddizione nel combat log"), Righe, 1);

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
	// 🔴 Il messaggio NON deve nominare un solo colpevole. `ValidatePlan` ordina per larghezza di slot e poi
	// per `ActionId`: `Action.Move` precede `Hero.Riktor.Ram` alfabeticamente, quindi Move prende lo slot e
	// Ram risulta «l'offender» — ma e' Ram che ESEGUE, ed e' Move che il resolver scarta. Nominare una sola
	// azione qui significa accusare quella sbagliata; nominarle entrambe e' vero comunque vada.
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
	PC->HandleClickOnCell(FRTCellId(2, 1));

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

	TestTrue(TEXT("nomina il movimento"), Riga.Contains(TEXT("Action.Move")));
	TestTrue(TEXT("e nomina anche la mobilita'"), Riga.Contains(*DashId.ToString()));
	TestTrue(TEXT("dice il motivo in italiano"), Riga.Contains(TEXT("slot")));
	TestFalse(TEXT("e non stampa l'identificatore grezzo dell'enum"), Riga.Contains(TEXT("SlotOccupied")));

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

	// Pianificata comunque: il gate di `CanUseAbility` vive nel click del giocatore
	// (`ARTPlayerController::HandleTargetEdge`), non nel dato — ed e' esattamente cio' che il validatore al
	// lock-in deve intercettare quando quel gate non si e' applicato.
	U->PlannedAbilityIndex = PanelIdx;

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
	// `ValidatePlansAtLockIn` itera `CollectLivingUnits`, che ordina per cella con `StableLess`. Sostituire
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

	// Un piano illegale su ENTRAMBE: scatto piu' movimento, due azioni sullo slot Movimento.
	for (ARTUnit* U : { Tardi, Prima })
	{
		const int32 DashIdx = U->FindDashAbilityIndex();
		if (!TestNotEqual(TEXT("premessa: l'eroe ha una mobilita' rapida"), DashIdx, static_cast<int32>(INDEX_NONE)))
		{
			DestroyInteractionWorld(World);
			return false;
		}
		U->PlannedDashAbility = DashIdx;
		U->PlannedDashCell = FRTCellId(U->Cell.X, U->Cell.Y + 1);
		U->PlannedCell = FRTCellId(U->Cell.X, U->Cell.Y + 2); // il movimento normale che rende il piano illegale
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
	const TArray<FKey>& Hotkeys = ARTPlayerController::AbilityHotkeys();
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
	for (const TPair<FName, FKey>& Riga : ARTPlayerController::GenericHotkeys())
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
	for (const TPair<FName, FKey>& Riga : ARTPlayerController::GenericHotkeys())
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

	for (const TPair<FName, FKey>& Riga : ARTPlayerController::GenericHotkeys())
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
 * `CameraModifierAction` su entrambi gli `Alt`. Cio' che si vieta e' un tasto che serva **due azioni
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
			bool bAlt = false, bPageUp = false, bPageDown = false;
			for (const FEnhancedActionKeyMapping& M : Imc->GetMappings())
			{
				bAlt      |= (M.Key == EKeys::LeftAlt);
				bPageUp   |= (M.Key == EKeys::PageUp);
				bPageDown |= (M.Key == EKeys::PageDown);
			}
			TestTrue(TEXT("Alt e' mappato (modificatore camera)"), bAlt);
			TestTrue(TEXT("PageUp e' mappato (piano attivo)"), bPageUp);
			TestTrue(TEXT("PageDown e' mappato (piano attivo)"), bPageDown);
		}
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
	return bOk;
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
 * `PlanningSnapshotFor` garantisce in partita.
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
	ARTUnit* Chi  = SpawnInteractionUnit(World, 0, URTHeroCatalogLibrary::MakeWraith(), Mia);
	ARTUnit* Alt  = SpawnInteractionUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), Loro);
	if (!TestNotNull(TEXT("chi pianifica"), Chi) || !TestNotNull(TEXT("chi occupa"), Alt))
	{
		DestroyInteractionWorld(World); return false;
	}

	// Gli id dello snapshot sono gli INDICI di `Units`: 0 = chi pianifica, 1 = chi occupa.
	const TArray<ARTUnit*> Units = { Chi, Alt };
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(MapActor->MapAsset, {
		FRTHexSimUnit(0, Mia,  /*MoveBudget=*/ 4),
		FRTHexSimUnit(1, Loro, /*MoveBudget=*/ 4)
	});

	// --- 1. 🔴 Il ramo che era sbagliato -------------------------------------------------------------
	{
		const FString Riga = ARTPlayerController::DescribeWaypointRejection(
			Snap, Units, /*PlannerIndex=*/ 0, Loro, /*SpentCost=*/ 0, /*Budget=*/ 4);

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
			Snap, Units, /*PlannerIndex=*/ 0, Mia, /*SpentCost=*/ 3, /*Budget=*/ 4);
		TestTrue(*FString::Printf(TEXT("la propria cella non e' un'occupazione: %s"), *Riga),
			Riga.Contains(TEXT("oltre il budget")));
		TestTrue(*FString::Printf(TEXT("e il budget si legge: %s"), *Riga), Riga.Contains(TEXT("3 di 4")));
	}

	// --- 3. Fuori mappa: soggetto = chi pianifica, ed e' corretto -------------------------------------
	{
		const FString Riga = ARTPlayerController::DescribeWaypointRejection(
			Snap, Units, /*PlannerIndex=*/ 0, FRTCellId(99, 99, 0), 0, 4);
		TestTrue(*FString::Printf(TEXT("dice fuori dalla mappa: %s"), *Riga),
			Riga.Contains(TEXT("fuori dalla mappa")));
		TestTrue(*FString::Printf(TEXT("e nomina chi pianifica: %s"), *Riga), Riga.Contains(Chi->GetName()));
	}

	// --- 4. Un indice fuori range non inventa un nome -------------------------------------------------
	{
		const TArray<ARTUnit*> Monca = { Chi }; // l'occupante non ha un Actor a cui corrispondere
		const FString Riga = ARTPlayerController::DescribeWaypointRejection(
			Snap, Monca, /*PlannerIndex=*/ 0, Loro, 0, 4);
		TestTrue(*FString::Printf(TEXT("resta un rifiuto per occupazione: %s"), *Riga),
			Riga.Contains(TEXT("occupata da")));
		TestFalse(*FString::Printf(TEXT("e NON attribuisce l'occupazione a chi pianifica: %s"), *Riga),
			Riga.Contains(FString(TEXT("occupata da ")) + Chi->GetName()));
	}

	DestroyInteractionWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
