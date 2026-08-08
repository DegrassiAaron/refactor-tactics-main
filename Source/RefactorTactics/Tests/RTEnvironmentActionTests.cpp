#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexArcLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Pathfinding/RTHexPath.h"
#include "Pathfinding/RTHexPathLibrary.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * CP 8.5 — le azioni che modificano il campo o curano, chiusura dell'epic E8.
 *
 * `Ignite` ed `Electrify` sono gia' verificate dai loro checkpoint (8.3/8.4): qui si coprono le tre che
 * mancavano — `Heal`, il raggio 1 di `CreateWater` e `ModifyArc` — e si fissa cio' che il catalogo dichiara.
 *
 * Prefissi `Env*` negli helper: unity build, namespace anonimi fusi con gli altri file di test.
 */
namespace
{
	UWorld* MakeEnvWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyEnvWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTHexMapActor* SpawnEnvMap(UWorld* World, int32 Radius = 4)
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

	ARTUnit* SpawnEnvUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false;
		U->ConfigureAsArchetype(ERTArchetype::Ranger);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell;
		return U;
	}

	void PlanEnvAction(ARTUnit* Caster, const TCHAR* ActionId, ARTUnit* Target)
	{
		URTActionData* Action = NewObject<URTActionData>(Caster);
		Action->Def = URTCatalogLibrary::FindCoreAction(FName(ActionId));
		Action->RangeCells = Action->Def.RangeCells;
		Action->CooldownTurns = Action->Def.CooldownTurns;
		Action->Power = URTCatalogLibrary::FirstDamage(Action->Def);
		Caster->Abilities[3] = Action;
		Caster->PlannedAbilityIndex = 3;
		Caster->PlannedAttackTarget = Target;
	}

	void RunEnvTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionHealTest,
	"RefactorTactics.Actions.Heal.RestoresWithoutExceedingMax",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionHealTest::RunTest(const FString&)
{
	// Le tre regole del catalogo §6, tutte insieme: cura 20 · non supera la salute massima · non rimuove stati.
	const FRTActionDef Def = URTCatalogLibrary::FindCoreAction(TEXT("Action.Heal"));
	TestEqual(TEXT("il catalogo dichiara 20"),
		Def.Effects.Num() > 0 ? Def.Effects[0].Amount : 0, 20);
	TestEqual(TEXT("portata 3"), Def.RangeCells, 3);
	TestTrue(TEXT("risolve nel Blast"),
		URTCatalogLibrary::MapResolutionPhase(Def.ResolutionPhase) == ERTMatchPhase::Blast);

	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnEnvMap(World);

	ARTUnit* Medic = SpawnEnvUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Wounded = SpawnEnvUnit(World, 0, FRTCellId(1, 0));
	// Un avversario inerte e lontano: senza nemici in campo la partita finirebbe al primo turno
	// (`EvaluateOutcome`) e il secondo turno non risolverebbe nulla. Non e' un dettaglio del test: e' la regola
	// di fine partita, e il test deve viverci dentro invece di aggirarla.
	SpawnEnvUnit(World, 1, FRTCellId(-4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Medic"), Medic) || !TestNotNull(TEXT("Wounded"), Wounded) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyEnvWorld(World);
		return false;
	}

	// Ferita di 30 e uno stato addosso: la cura ne ripara 20 e **non** tocca lo stato.
	Wounded->ApplyCombatState(Wounded->MaxHealth - 30, Wounded->Shield);
	Wounded->ApplyStatus(TAG_Status_Slow, 2);

	PlanEnvAction(Medic, TEXT("Action.Heal"), Wounded);
	RunEnvTurn(TM);

	TestEqual(TEXT("cura 20"), Wounded->Health, Wounded->MaxHealth - 10);
	TestTrue(TEXT("non rimuove gli stati"), Wounded->HasStatus(TAG_Status_Slow));

	// Seconda cura su chi e' quasi pieno: si ferma al massimo, non lo supera.
	Wounded->ApplyCombatState(Wounded->MaxHealth - 5, Wounded->Shield);
	PlanEnvAction(Medic, TEXT("Action.Heal"), Wounded);
	RunEnvTurn(TM);
	TestEqual(TEXT("non supera la salute massima"), Wounded->Health, Wounded->MaxHealth);

	// L'esito e' nel TurnLog, e dice quanto e' stato curato DAVVERO (5, non 20).
	bool bLogged = false;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category == ERTLogCategory::Combat
			&& E.Outcome == static_cast<uint8>(ERTCombatOutcome::Healed)
			&& E.ActionId == FName(TEXT("Action.Heal"))
			&& E.Amount == 5)
		{
			bLogged = true;
		}
	}
	TestTrue(TEXT("il TurnLog registra la cura effettiva"), bLogged);

	DestroyEnvWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionCreateWaterTest,
	"RefactorTactics.Actions.CreateWater.CoversRadiusAndWetsOccupants",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionCreateWaterTest::RunTest(const FString&)
{
	// Catalogo §6: «acqua raggio 1», e le unita' presenti si bagnano. Il raggio e' dell'AZIONE — CP 8.4
	// applicava la sola cella del bersaglio.
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World);

	ARTUnit* Caster = SpawnEnvUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Target = SpawnEnvUnit(World, 1, FRTCellId(2, 0));
	ARTUnit* Neighbour = SpawnEnvUnit(World, 1, FRTCellId(3, 0)); // adiacente al bersaglio: dentro il raggio 1
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Caster"), Caster) || !TestNotNull(TEXT("Target"), Target)
		|| !TestNotNull(TEXT("Neighbour"), Neighbour) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyEnvWorld(World);
		return false;
	}

	PlanEnvAction(Caster, TEXT("Action.CreateWater"), Target);
	RunEnvTurn(TM);

	auto SurfaceAt = [MapActor](const FRTCellId& Cell)
	{
		const FRTHexCellData* Data = MapActor->MapAsset ? MapActor->MapAsset->FindCell(Cell) : nullptr;
		return Data ? Data->Surface : ERTHexSurface::Floor;
	};

	TestTrue(TEXT("la cella bersaglio e' allagata"), SurfaceAt(FRTCellId(2, 0)) == ERTHexSurface::ShallowWater);
	TestTrue(TEXT("e anche una adiacente (raggio 1)"), SurfaceAt(FRTCellId(3, 0)) == ERTHexSurface::ShallowWater);
	TestTrue(TEXT("ma non una a due celle di distanza"),
		SurfaceAt(FRTCellId(4, 0)) != ERTHexSurface::ShallowWater);

	// Chi c'era gia' si bagna subito: aspettare che esca e rientri per applicare `Wet` sarebbe una regola
	// che nessuno capirebbe guardando il campo.
	TestTrue(TEXT("il bersaglio e' bagnato"), Target->HasStatus(TAG_Status_Wet));
	TestTrue(TEXT("e anche chi era nel raggio"), Neighbour->HasStatus(TAG_Status_Wet));

	DestroyEnvWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionModifyArcTest,
	"RefactorTactics.Actions.ModifyArc.BumpsChunkRevision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionModifyArcTest::RunTest(const FString&)
{
	// **Nome vincolante** della DoD. La revisione e' il numero che invalida le cache di percorso: se cambiare
	// la topologia non la incrementasse, un percorso calcolato prima resterebbe valido dopo — cioe' un'unita'
	// camminerebbe su un ponte che non c'e' piu'.
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World);

	ARTUnit* Caster = SpawnEnvUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Target = SpawnEnvUnit(World, 1, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Caster"), Caster) || !TestNotNull(TEXT("Target"), Target) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyEnvWorld(World);
		return false;
	}

	const int32 RevisionBefore = MapActor->MapAsset->Revision;
	const int32 ArcsBefore = MapActor->MapAsset->Transitions.Num();

	// Nessun collegamento fra le due celle: l'azione lo APRE (ponte).
	PlanEnvAction(Caster, TEXT("Action.ModifyArc"), Target);
	RunEnvTurn(TM);

	TestTrue(TEXT("la revisione e' aumentata"), MapActor->MapAsset->Revision > RevisionBefore);
	TestTrue(TEXT("un collegamento in piu'"), MapActor->MapAsset->Transitions.Num() > ArcsBefore);

	// Rigiocata sulla stessa coppia, l'azione CHIUDE quello che aveva aperto — «apri o chiudi» e' la stessa
	// azione vista dai due lati, come una porta.
	const int32 RevisionAfterOpen = MapActor->MapAsset->Revision;
	PlanEnvAction(Caster, TEXT("Action.ModifyArc"), Target);
	RunEnvTurn(TM);

	TestEqual(TEXT("il collegamento e' tornato a zero"), MapActor->MapAsset->Transitions.Num(), ArcsBefore);
	TestTrue(TEXT("e la revisione e' aumentata di nuovo"), MapActor->MapAsset->Revision > RevisionAfterOpen);

	// La modifica della topologia e' osservabile come le altre modifiche ambientali.
	int32 ArcEntries = 0;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category == ERTLogCategory::Environment && E.ActionId == FName(TEXT("Action.ModifyArc")))
		{
			++ArcEntries;
		}
	}
	TestTrue(TEXT("il TurnLog registra la modifica del collegamento"), ArcEntries > 0);

	DestroyEnvWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEnvironmentActionsMatchCatalogTest,
	"RefactorTactics.Actions.EnvironmentalSetMatchesCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEnvironmentActionsMatchCatalogTest::RunTest(const FString&)
{
	// Le azioni ambientali del catalogo §6 esistono con i numeri dichiarati, e risolvono tutte nel Cleanup:
	// una di esse nel Blast cambierebbe il TERRENO a meta' turno, e il costo di un percorso gia' calcolato
	// cambierebbe sotto i piedi di chi lo sta percorrendo senza che nulla lo fermi.
	//
	// `Action.ModifyArc` **non e' piu' fra queste** (CP 9.4, 2026-08-08): e' passata al Blast con porte e
	// strutture, perche' la TOPOLOGIA e' un caso diverso dal terreno — un passo che non esiste piu' viene
	// troncato da `TruncatePathToTopology` con un reason code, mentre un costo che cambia non lo si nota.
	// La sua fase e' verificata piu' sotto, insieme al perche'.
	struct FExpected { const TCHAR* Id; int32 Range; int32 Cooldown; };
	const FExpected Environmental[] = {
		{ TEXT("Action.Electrify"),   4, 2 },
		{ TEXT("Action.Ignite"),      4, 2 },
		{ TEXT("Action.CreateWater"), 4, 2 },
	};

	for (const FExpected& E : Environmental)
	{
		const FRTActionDef Def = URTCatalogLibrary::FindCoreAction(FName(E.Id));
		if (!TestTrue(FString::Printf(TEXT("%s e' nel catalogo"), E.Id), Def.ActionId == FName(E.Id))) { continue; }
		TestEqual(FString::Printf(TEXT("%s: portata"), E.Id), Def.RangeCells, E.Range);
		TestEqual(FString::Printf(TEXT("%s: cooldown"), E.Id), Def.CooldownTurns, E.Cooldown);
		TestTrue(FString::Printf(TEXT("%s: risolve nel Cleanup"), E.Id),
			URTCatalogLibrary::MapResolutionPhase(Def.ResolutionPhase) == ERTMatchPhase::Cleanup);
	}

	// `Action.ModifyArc` resta nel catalogo con i suoi numeri, ma nel **Blast**: e' la decisione di CP 9.4, e
	// senza questa riga il cambio di fase passerebbe senza che nessun test se ne accorga.
	{
		const FRTActionDef Arc = URTCatalogLibrary::FindCoreAction(TEXT("Action.ModifyArc"));
		TestTrue(TEXT("ModifyArc e' nel catalogo"), Arc.ActionId == FName(TEXT("Action.ModifyArc")));
		TestEqual(TEXT("ModifyArc: portata invariata"), Arc.RangeCells, 3);
		TestEqual(TEXT("ModifyArc: cooldown invariato"), Arc.CooldownTurns, 2);
		TestTrue(TEXT("ModifyArc risolve nel BLAST, con porte e strutture"),
			URTCatalogLibrary::MapResolutionPhase(Arc.ResolutionPhase) == ERTMatchPhase::Blast);
	}

	// `Action.CreateCover` **non** e' qui, ed e' una decisione: le coperture non esistono nel modello dati
	// (`FRTHexCellData` non ha bordi protetti) e costruirle e' l'epic E9. Un'azione che dichiarasse di creare
	// una copertura senza che le coperture esistano sarebbe un'abilita' inerte — il difetto che questo
	// checkpoint chiude altrove, non uno da aggiungere qui.
	TestTrue(TEXT("CreateCover resta fuori finche' le coperture non esistono (E9)"),
		URTCatalogLibrary::FindCoreAction(TEXT("Action.CreateCover")).ActionId.IsNone());

	const TArray<FString> Errors = URTCatalogLibrary::ValidateActions(URTCatalogLibrary::GetCoreActionCatalog());
	for (const FString& Err : Errors) { AddError(Err); }
	TestEqual(TEXT("il catalogo resta valido"), Errors.Num(), 0);
	return true;
}

/**
 * La DoD di CP 9.4 nel caso che conta: il ponte sparisce a META' TURNO e chi lo stava per attraversare NON si
 * ritrova dall'altra parte. E' il gemello di `Door.ClosingStopsMovement`, ma con una differenza che il
 * checkpoint esiste per fissare — una porta chiusa si aggira, un ponte tolto no: fra due layer non c'e' una
 * via alternativa, quindi il percorso FALLISCE invece di allungarsi.
 *
 * Scena: ponte fra (0,0,L0) e (1,0,L1). Chi lo taglia sta su un estremo — l'arco e' identificato dalla coppia
 * (caster, bersaglio) — e nello stesso turno si sposta, cosi' la cella di arrivo resta LIBERA: senza questo,
 * il Mover si fermerebbe comunque per occupazione e il test non dimostrerebbe niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBridgeNoTeleportTest,
	"RefactorTactics.Structures.Bridge.NoTeleportOnRemoval",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBridgeNoTeleportTest::RunTest(const FString&)
{
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World);

	const FRTCellId Ground(0, 0, 0);
	const FRTCellId Upper(1, 0, 1);
	MapActor->MapAsset->AddOrUpdateCell(FRTHexCellData(Upper));
	MapActor->MapAsset->SortCells();
	MapActor->MapAsset->AddTransition(Ground, Upper, /*Cost*/ 1, ERTHexTransitionKind::Bridge,
		/*bBidirectional*/ true);

	ARTUnit* Cutter = SpawnEnvUnit(World, 0, Ground);
	ARTUnit* Mover = SpawnEnvUnit(World, 1, Upper);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Cutter"), Cutter) || !TestNotNull(TEXT("Mover"), Mover)
		|| !TestNotNull(TEXT("TM"), TM))
	{
		DestroyEnvWorld(World);
		return false;
	}

	// Il percorso e' valido nel momento in cui viene scritto: il ponte c'e'.
	TestTrue(TEXT("col ponte il passo esiste"),
		URTHexArcLibrary::IsArcTraversable(MapActor->MapAsset, Upper, Ground));
	Mover->PlannedPath = { Upper, Ground };

	// Chi taglia il ponte libera anche la cella di arrivo, altrimenti il Mover si fermerebbe per occupazione
	// e non si saprebbe se e' stata la topologia o un'unita' di mezzo.
	PlanEnvAction(Cutter, TEXT("Action.ModifyArc"), Mover);
	Cutter->PlannedPath = { Ground, FRTCellId(1, 0, 0) };

	RunEnvTurn(TM);

	TestFalse(TEXT("il ponte non c'e' piu'"),
		URTHexArcLibrary::IsArcTraversable(MapActor->MapAsset, Upper, Ground));
	TestTrue(TEXT("chi lo attraversava e' rimasto dov'era"), Mover->Cell == Upper);
	TestFalse(TEXT("e NON si e' teletrasportato di sotto"), Mover->Cell == Ground);
	TestTrue(TEXT("la cella di arrivo era davvero libera"), Cutter->Cell != Ground);

	// Dal turno dopo il percorso non esiste proprio: e' il «path fallisce» della DoD, non un giro piu' lungo.
	const FRTHexPathResult Broken = URTHexPathLibrary::FindPath(MapActor->MapAsset, Upper, Ground, /*MaxCost*/ 0);
	TestTrue(TEXT("il percorso fra i due layer FALLISCE"), Broken.Status == ERTHexPathStatus::NoPath);

	int32 Logged = 0;
	for (const FRTTurnLogEntry& Entry : TM->GetTurnLog())
	{
		if (Entry.Category == ERTLogCategory::Environment
			&& Entry.Outcome == static_cast<uint8>(ERTEnvironmentOutcome::BridgeRemoved))
		{
			++Logged;
		}
	}
	TestEqual(TEXT("il TurnLog registra il ponte tolto"), Logged, 1);

	DestroyEnvWorld(World);
	return true;
}

/**
 * Il ponte creato da `ModifyArc` e' TEMPORANEO e CONDUTTIVO. La durata e' quella delle altre modifiche
 * ambientali del catalogo (2 turni) e i suoi turni cominciano dal PROSSIMO: l'azione risolve nel Blast e la
 * scadenza gira nel Cleanup dello stesso turno, quindi senza questa distinzione il ponte ne perderebbe uno
 * prima che qualcuno possa attraversarlo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBridgeTemporaryTest,
	"RefactorTactics.Structures.Bridge.TemporaryBridgeExpires",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBridgeTemporaryTest::RunTest(const FString&)
{
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World);

	const FRTCellId Ground(0, 0, 0);
	const FRTCellId Upper(1, 0, 1);
	MapActor->MapAsset->AddOrUpdateCell(FRTHexCellData(Upper));
	MapActor->MapAsset->SortCells();

	ARTUnit* Builder = SpawnEnvUnit(World, 0, Ground);
	ARTUnit* Target = SpawnEnvUnit(World, 1, Upper);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Builder"), Builder) || !TestNotNull(TEXT("Target"), Target)
		|| !TestNotNull(TEXT("TM"), TM))
	{
		DestroyEnvWorld(World);
		return false;
	}

	TestFalse(TEXT("all'inizio i due layer sono separati"),
		URTHexArcLibrary::IsArcTraversable(MapActor->MapAsset, Ground, Upper));

	PlanEnvAction(Builder, TEXT("Action.ModifyArc"), Target);
	RunEnvTurn(TM); // turno 1: il ponte nasce

	TestTrue(TEXT("il ponte esiste"), URTHexArcLibrary::IsArcTraversable(MapActor->MapAsset, Ground, Upper));
	TestTrue(TEXT("ed e' CONDUTTIVO: la scarica lo risale"),
		URTHexArcLibrary::ArcConductsElectricity(MapActor->MapAsset, Ground, Upper));

	RunEnvTurn(TM); // turno 2: regge (i suoi turni cominciano da qui)
	TestTrue(TEXT("dopo un turno regge ancora"),
		URTHexArcLibrary::IsArcTraversable(MapActor->MapAsset, Ground, Upper));

	RunEnvTurn(TM); // turno 3: scade
	TestFalse(TEXT("scaduto: i due layer sono di nuovo separati"),
		URTHexArcLibrary::IsArcTraversable(MapActor->MapAsset, Ground, Upper));
	TestTrue(TEXT("e il percorso fallisce"),
		URTHexPathLibrary::FindPath(MapActor->MapAsset, Ground, Upper, /*MaxCost*/ 0).Status
			== ERTHexPathStatus::NoPath);

	DestroyEnvWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
