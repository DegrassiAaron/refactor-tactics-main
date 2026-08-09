#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Core/RTGameplayTags.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexArcLibrary.h"
#include "Map/RTHexCoverLibrary.h"
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

	/**
	 * Pianifica un'azione che agisce su una STRUTTURA di bordo: bersaglio-CELLA (non unita') piu' il bordo, che
	 * a portata 3 non e' derivabile dalla coppia di celle. E' la stessa forma che l'HUD dovra' produrre (E11).
	 */
	void PlanCoverAction(ARTUnit* Caster, const TCHAR* ActionId, const FRTCellId& TargetCell,
		ERTHexDirection Edge)
	{
		URTActionData* Action = NewObject<URTActionData>(Caster);
		Action->Def = URTCatalogLibrary::FindCoreAction(FName(ActionId));
		Action->RangeCells = Action->Def.RangeCells;
		Action->CooldownTurns = Action->Def.CooldownTurns;
		Caster->Abilities[3] = Action;
		Caster->PlannedAbilityIndex = 3;
		Caster->PlannedAttackTarget = nullptr;
		Caster->PlannedAttackCell = TargetCell;
		Caster->bAttackTargetsCell = true;
		Caster->PlannedCoverEdge = Edge;
		Caster->bHasPlannedCoverEdge = true;
	}

	/**
	 * Come `PlanCoverAction`, ma con un'abilita' d'EROE gia' costruita dal catalogo (e la sua variante attiva).
	 * Serve perche' il pannello di Bastion non e' un'azione core: e' un'azione core con un nome d'eroe, e la
	 * differenza va verificata su cio' che il giocatore usa davvero.
	 */
	void PlanHeroCoverAction(ARTUnit* Caster, URTActionData* HeroAction, const FRTCellId& TargetCell,
		ERTHexDirection Edge, const FName& VariantId = NAME_None)
	{
		Caster->Abilities[3] = HeroAction;
		Caster->PlannedAbilityIndex = 3;
		Caster->PlannedAttackTarget = nullptr;
		Caster->PlannedAttackCell = TargetCell;
		Caster->bAttackTargetsCell = true;
		Caster->PlannedCoverEdge = Edge;
		Caster->bHasPlannedCoverEdge = true;
		Caster->ActiveVariantId = VariantId;
	}

	/** Integrita' della copertura su quel bordo, o 0 se il bordo e' scoperto. */
	int32 CoverIntegrityOn(const URTHexMapAsset* Map, const FRTCellId& Cell, ERTHexDirection Edge)
	{
		const FRTHexCellData* Data = Map ? Map->FindCell(Cell) : nullptr;
		const FRTHexCover* Entry = Data ? Data->CoverEntryOn(Edge) : nullptr;
		return Entry ? Entry->Integrity : 0;
	}

	/** Quante voci di quell'esito ambientale ci sono nel TurnLog. */
	int32 CountEnvOutcome(const ARTTurnManager* TM, ERTEnvironmentOutcome Outcome)
	{
		int32 N = 0;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Environment && E.Outcome == static_cast<uint8>(Outcome)) { ++N; }
		}
		return N;
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

	// `Action.CreateCover` **e' entrata** con CP 9.5 (2026-08-09). Fino a CP 9.4 restava fuori di proposito —
	// un'azione che dichiara di creare una copertura mentre le coperture non esistono e' un'abilita' inerte —
	// e ora il modello c'e' (formato v3, `FRTHexCover` per bordo) e qualcuno la consuma.
	//
	// **Fase `Prep`, non Blast**, contro la riga del catalogo azioni v0.1 che diceva Blast (D-a): eretta nel
	// Blast arriverebbe dopo aver incassato i colpi di quel Blast. Non e' il caso di `ModifyArc` qui sopra,
	// perche' quella cambia la TOPOLOGIA e il Move che segue deve vederla; una copertura bassa non tocca ne'
	// grafo ne' vista.
	{
		const FRTActionDef Cover = URTCatalogLibrary::FindCoreAction(TEXT("Action.CreateCover"));
		TestTrue(TEXT("CreateCover e' nel catalogo"), Cover.ActionId == FName(TEXT("Action.CreateCover")));
		TestEqual(TEXT("CreateCover: portata 3"), Cover.RangeCells, 3);
		TestEqual(TEXT("CreateCover: cooldown 2"), Cover.CooldownTurns, 2);
		TestTrue(TEXT("CreateCover risolve in PREP, prima dei colpi che deve riparare"),
			URTCatalogLibrary::MapResolutionPhase(Cover.ResolutionPhase) == ERTMatchPhase::Prep);
		TestTrue(TEXT("e dichiara la sua operazione come DATO, non per ActionId"),
			Cover.StructureOp == ERTStructureOp::CreateCover);
		TestEqual(TEXT("nessun effetto su unita': il suo esito e' una modifica della mappa"),
			Cover.Effects.Num(), 0);
	}

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

/**
 * Che in PARTITA il danno alle strutture raggiunga davvero un ARCO, non solo una copertura.
 *
 * Questo test non c'era: l'ha reso necessario la VERIFICA DI MUTAZIONE. Disattivando la raccolta del danno
 * verso gli archi nel `TurnManager` non cadeva nessuno dei dieci test di CP 9.4, perche' `DamageBreaksAtZero`
 * chiama `URTHexArcLibrary::DamageArc` DIRETTAMENTE: la libreria era coperta, il cablaggio no. E' il difetto
 * ricorrente di questo repository — codice corretto che nessuno chiama.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBridgeDamagedInTurnTest,
	"RefactorTactics.Structures.Bridge.DamagedInPlayedTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBridgeDamagedInTurnTest::RunTest(const FString&)
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

	ARTUnit* Breacher = SpawnEnvUnit(World, 0, Ground);
	ARTUnit* Foe = SpawnEnvUnit(World, 1, Upper);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Breacher"), Breacher) || !TestNotNull(TEXT("Foe"), Foe)
		|| !TestNotNull(TEXT("TM"), TM))
	{
		DestroyEnvWorld(World);
		return false;
	}

	// L'abilita' dichiara di poter sfondare: e' il catalogo a concederlo (qui lo si simula sull'istanza).
	Breacher->Abilities[0]->Def.Effects.Add(FRTActionEffectSpec(ERTActionEffect::DamageStructure, 20));
	Breacher->PlannedAbilityIndex = 0;
	Breacher->PlannedAttackTarget = Foe;

	RunEnvTurn(TM);

	// Il ponte ha incassato sulla COPIA di lavoro della mappa, quella su cui gira la partita.
	const FRTHexEdge* Damaged = URTHexArcLibrary::FindArc(MapActor->MapAsset, Ground, Upper);
	if (TestTrue(TEXT("il ponte c'e' ancora"), Damaged != nullptr))
	{
		TestEqual(TEXT("integrita' scalata dal colpo"), Damaged->Integrity, 20);
		TestTrue(TEXT("ed e' ancora percorribile"), Damaged->State == ERTHexArcState::Active);
	}
	// Entrambi i versi: un ponte colpito una volta non deve reggere il doppio da una parte.
	const FRTHexEdge* Back = URTHexArcLibrary::FindArc(MapActor->MapAsset, Upper, Ground);
	TestTrue(TEXT("anche il verso opposto ha incassato"), Back && Back->Integrity == 20);

	int32 Logged = 0;
	for (const FRTTurnLogEntry& Entry : TM->GetTurnLog())
	{
		if (Entry.Category == ERTLogCategory::Environment
			&& Entry.Outcome == static_cast<uint8>(ERTEnvironmentOutcome::BridgeDamaged))
		{
			++Logged;
			TestEqual(TEXT("il log riporta l'integrita' residua"), Entry.Amount, 20);
		}
	}
	TestEqual(TEXT("due voci, una per verso"), Logged, 2);

	DestroyEnvWorld(World);
	return true;
}

/**
 * CP 9.5 — il pannello nasce in PARTITA, ripara, e scade da solo.
 *
 * Il test gira un turno vero (`Intent -> Prep -> ... -> Cleanup`), non chiama `AddCover`: la libreria era gia'
 * coperta dai test puri, ed e' esattamente la trappola in cui questo repository e' caduto a CP 9.4 — libreria
 * verde, cablaggio scoperto. Quel che si verifica qui e' che il TurnManager la eriga davvero.
 *
 * **La durata parte dal turno in cui nasce**, al contrario del ponte temporaneo: `CreateCover` risolve in Prep,
 * cioe' prima del Blast che la usa, quindi il turno dell'erezione e' gia' un turno in cui ha riparato qualcuno.
 * Due turni di durata = protetta nel turno 1 e nel turno 2, scoperta dal 3.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKineticPanelTemporaryCoverTest,
	"RefactorTactics.Structures.KineticPanel.TemporaryCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKineticPanelTemporaryCoverTest::RunTest(const FString&)
{
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World);

	const FRTCellId Home(0, 0);
	const FRTCellId Shielded(1, 0); // la cella che riceve il pannello, adiacente a Home

	ARTUnit* Builder = SpawnEnvUnit(World, 0, Home);
	ARTUnit* Foe = SpawnEnvUnit(World, 1, FRTCellId(-4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Builder"), Builder) || !TestNotNull(TEXT("Foe"), Foe) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyEnvWorld(World);
		return false;
	}

	TestEqual(TEXT("all'inizio il bordo e' scoperto"),
		URTHexCoverLibrary::CoverBetween(MapActor->MapAsset, Shielded, Home), ERTHexCoverType::None);

	// Bordo W di (1,0): la faccia rivolta a chi lo erige.
	PlanCoverAction(Builder, TEXT("Action.CreateCover"), Shielded, ERTHexDirection::W);
	RunEnvTurn(TM); // turno 1: il pannello nasce, e ripara gia' questo turno

	TestEqual(TEXT("la copertura c'e', ed e' bassa"),
		URTHexCoverLibrary::CoverBetween(MapActor->MapAsset, Shielded, Home), ERTHexCoverType::Low);
	TestEqual(TEXT("il TurnLog dice che e' stata eretta"),
		CountEnvOutcome(TM, ERTEnvironmentOutcome::CoverCreated), 1);
	if (const FRTHexCellData* Cell = MapActor->MapAsset->FindCell(Shielded))
	{
		const FRTHexCover* Entry = Cell->CoverEntryOn(ERTHexDirection::W);
		TestNotNull(TEXT("la voce e' sul bordo dichiarato"), Entry);
		if (Entry) { TestEqual(TEXT("integrita' 30, dal catalogo terreni"), Entry->Integrity, 30); }
	}

	// Turno 2: e' il SECONDO dei suoi due turni. Il pannello c'e' per tutta la fase in cui si combatte e cade
	// nel Cleanup, a fine turno — che la scadenza cada qui e non nel turno 3 e' la prova che il turno di
	// nascita e' stato contato, cioe' che il pannello non ha ricevuto un turno di grazia.
	RunEnvTurn(TM);
	TestEqual(TEXT("nel Cleanup del secondo turno scade"),
		CountEnvOutcome(TM, ERTEnvironmentOutcome::CoverExpired), 1);
	TestEqual(TEXT("e il bordo torna scoperto"),
		URTHexCoverLibrary::CoverBetween(MapActor->MapAsset, Shielded, Home), ERTHexCoverType::None);

	// Il TurnLog e' del TURNO (`TurnLog.Reset()` a ogni risoluzione): al terzo turno non resta traccia, e il
	// campo non deve piu' cambiare da solo — una scadenza che si ripete sarebbe una voce fantasma nel replay.
	RunEnvTurn(TM);
	TestEqual(TEXT("non scade una seconda volta"),
		CountEnvOutcome(TM, ERTEnvironmentOutcome::CoverExpired), 0);
	TestEqual(TEXT("e il bordo resta scoperto"),
		URTHexCoverLibrary::CoverBetween(MapActor->MapAsset, Shielded, Home), ERTHexCoverType::None);

	DestroyEnvWorld(World);
	return true;
}

/**
 * CP 9.5 — la portata dichiarata dal catalogo vale, e il bordo gia' riparato non ne accetta un secondo.
 *
 * La prima meta' e' il difetto che la issue #206 registra su `ModifyArc`: un'azione che dichiara `Range 3` e
 * opera comunque non ha una portata. Qui si valida PRIMA di toccare la mappa, e il rifiuto e' una voce di
 * TurnLog — il `Cancel` del catalogo reso visibile, perche' un'azione che sparisce in silenzio e'
 * indistinguibile da un difetto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCreateCoverRejectsTest,
	"RefactorTactics.Actions.CreateCover.RejectsOutOfRangeAndOccupied",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCreateCoverRejectsTest::RunTest(const FString&)
{
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World);

	const FRTCellId Home(0, 0);
	const FRTCellId TooFar(4, 0); // distanza 4 > portata 3

	ARTUnit* Builder = SpawnEnvUnit(World, 0, Home);
	ARTUnit* Foe = SpawnEnvUnit(World, 1, FRTCellId(-4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Builder"), Builder) || !TestNotNull(TEXT("Foe"), Foe) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyEnvWorld(World);
		return false;
	}

	PlanCoverAction(Builder, TEXT("Action.CreateCover"), TooFar, ERTHexDirection::W);
	RunEnvTurn(TM);

	TestEqual(TEXT("fuori portata: nessuna copertura"),
		URTHexCoverLibrary::CoverBetween(MapActor->MapAsset, TooFar, FRTCellId(3, 0)), ERTHexCoverType::None);
	TestEqual(TEXT("e il rifiuto e' registrato"),
		CountEnvOutcome(TM, ERTEnvironmentOutcome::CoverRejected), 1);
	TestEqual(TEXT("nessuna copertura eretta"),
		CountEnvOutcome(TM, ERTEnvironmentOutcome::CoverCreated), 0);

	// Ora dentro portata, ma su un bordo gia' riparato dal dato di mappa: stesso esito, ragione diversa.
	const FRTCellId Near(1, 0);
	URTHexCoverLibrary::AddCover(MapActor->MapAsset, Near, ERTHexDirection::W, ERTHexCoverType::Low, 30);

	PlanCoverAction(Builder, TEXT("Action.CreateCover"), Near, ERTHexDirection::W);
	RunEnvTurn(TM);

	TestEqual(TEXT("il bordo resta quello di prima, non ne nasce un secondo"),
		CountEnvOutcome(TM, ERTEnvironmentOutcome::CoverCreated), 0);
	// Il TurnLog e' del turno, non della partita (`TurnLog.Reset()`): qui si conta il rifiuto di QUESTO turno.
	TestEqual(TEXT("anche il bordo occupato produce un rifiuto leggibile"),
		CountEnvOutcome(TM, ERTEnvironmentOutcome::CoverRejected), 1);
	if (const FRTHexCellData* Cell = MapActor->MapAsset->FindCell(Near))
	{
		TestEqual(TEXT("una sola voce sul bordo"), Cell->Covers.Num(), 1);
	}

	DestroyEnvWorld(World);
	return true;
}

/**
 * CP 9.5 — `Bastion.KineticPanel` erige davvero, e la VARIANTE attiva decide integrita' e durata.
 *
 * Fino a qui i `Parameters` delle varianti erano una dichiarazione che nessun sistema leggeva, in tutto il
 * progetto: il catalogo scriveva «45 per un turno solo» e «25 che non scade» e il gioco applicava sempre 30/2.
 * Questo e' il test che rende il compromesso osservabile — e cade se qualcuno riporta i numeri a costanti.
 *
 * I due rami dimostrano cose diverse: il rinforzato che la durata viene letta (1 turno: cade nel Cleanup del
 * turno stesso), l'adattivo che `DurationTurns = 0` significa «non scade da sola» e non «scade subito» — la
 * lettura sbagliata piu' probabile, e quella che il campo non perdonerebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBastionPanelVariantAppliedTest,
	"RefactorTactics.Heroes.Bastion.KineticPanelVariantApplied",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBastionPanelVariantAppliedTest::RunTest(const FString&)
{
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World);

	const FRTCellId Home(0, 0);
	const FRTCellId Reinforced(1, 0);
	const FRTCellId Adaptive(0, 1);

	// Due unita' invece di una che agisce due volte: il cooldown del pannello e' 2 turni, e aspettarlo
	// renderebbe il test una storia lunga in cui la durata dell'adattivo si confonde con l'attesa.
	ARTUnit* WithReinforced = SpawnEnvUnit(World, 0, Home);
	ARTUnit* WithAdaptive = SpawnEnvUnit(World, 0, FRTCellId(2, 0));
	ARTUnit* Foe = SpawnEnvUnit(World, 1, FRTCellId(-4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("con rinforzato"), WithReinforced) || !TestNotNull(TEXT("con adattivo"), WithAdaptive)
		|| !TestNotNull(TEXT("Foe"), Foe) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyEnvWorld(World);
		return false;
	}

	// L'abilita' e' quella del catalogo eroi, non l'azione core: e' cio' che il giocatore ha in mano. Due
	// istanze distinte, cosi' nessuno stato dell'una puo' spiegare il comportamento dell'altra.
	URTActionData* PanelA = URTHeroCatalogLibrary::MakeBastion()->Actions[1];
	URTActionData* PanelB = URTHeroCatalogLibrary::MakeBastion()->Actions[1];

	PlanHeroCoverAction(WithReinforced, PanelA, Reinforced, ERTHexDirection::W,
		TEXT("Bastion.KineticPanel.Reinforced"));
	PlanHeroCoverAction(WithAdaptive, PanelB, Adaptive, ERTHexDirection::SW,
		TEXT("Bastion.KineticPanel.Adaptive"));
	RunEnvTurn(TM);

	// I due parametri si verificano dove ciascuno e' osservabile, e non e' un ripiego: e' il compromesso
	// stesso. Il rinforzato dura UN turno, quindi cade nel Cleanup del turno in cui nasce — la sua integrita'
	// non esiste piu' a turno finito, e cercarla qui vorrebbe dire non aver capito che cosa si e' comprato.
	//
	// `DurationTurns` letto dalla variante: due pannelli eretti nello stesso turno, UNA sola scadenza.
	TestEqual(TEXT("il rinforzato scade subito: dura 1, non 2"),
		CountEnvOutcome(TM, ERTEnvironmentOutcome::CoverExpired), 1);
	TestEqual(TEXT("ed e' il suo bordo a essere tornato scoperto"),
		CoverIntegrityOn(MapActor->MapAsset, Reinforced, ERTHexDirection::W), 0);
	TestEqual(TEXT("due pannelli eretti"), CountEnvOutcome(TM, ERTEnvironmentOutcome::CoverCreated), 2);

	// `Integrity` letto dalla variante: 25 non e' ne' il 30 di base ne' il 45 dell'altra.
	TestEqual(TEXT("adattivo: integrita' 25, non i 30 di base"),
		CoverIntegrityOn(MapActor->MapAsset, Adaptive, ERTHexDirection::SW), 25);

	RunEnvTurn(TM);
	RunEnvTurn(TM);
	TestEqual(TEXT("durata 0 = non scade da sola: due turni dopo e' ancora li'"),
		CoverIntegrityOn(MapActor->MapAsset, Adaptive, ERTHexDirection::SW), 25);

	DestroyEnvWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
