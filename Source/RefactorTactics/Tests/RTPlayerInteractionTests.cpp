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

	ARTUnit* SpawnInteractionUnit(UWorld* World, int32 TeamId, ERTArchetype Arch, const FRTCellId& Cell)
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureAsArchetype(Arch);
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
	ARTUnit* Unit = SpawnInteractionUnit(World, 0, ERTArchetype::Ranger, FRTCellId(2, -2, 0));
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
		int32 Rejected = 0;
		const FRTCellId Far[] = { FRTCellId(4, -1, 0), FRTCellId(4, 0, 0), FRTCellId(3, 1, 0), FRTCellId(2, 2, 0) };
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

	ARTUnit* Unit = SpawnInteractionUnit(World, 0, ERTArchetype::Ranger, FRTCellId(2, -2, 0));
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

	ARTUnit* Unit = SpawnInteractionUnit(World, 0, ERTArchetype::Ranger, FRTCellId(2, -2, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!PC || !Unit) { DestroyInteractionWorld(World); return false; }

	// Lo Scatto e' la quarta abilita' del Ranger (indice 3), portata 5, lineare.
	const int32 DashIdx = 3;
	const URTActionData* Dash = Unit->GetAbility(DashIdx);
	if (!TestNotNull(TEXT("premessa: il Ranger ha lo scatto"), (void*)Dash))
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

#endif // WITH_DEV_AUTOMATION_TESTS
