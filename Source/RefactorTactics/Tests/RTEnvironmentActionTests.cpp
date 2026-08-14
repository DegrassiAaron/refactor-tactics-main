#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTEquipmentData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Combat/RTCombatLibrary.h" // BurningCleanupDamage: il test somma ingresso + bruciatura (#570)
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
#include "Turn/RTActionFallbackLibrary.h"
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
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeVektor());
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionMistVeilTest,
	"RefactorTactics.Actions.MistVeil.CreatesSmokeAndCapsTargeting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionMistVeilTest::RunTest(const FString&)
{
	// Issue #353. `Riva.MistVeil` dichiarava «crea fumo raggio 1» e non lo faceva: `Smoke` era l'unica delle
	// otto superfici che nessuna azione sapeva creare. Il test non si ferma alla superficie — verifica anche
	// il CAP di targeting, perche' e' quello l'effetto tattico, e una superficie dipinta che non cambia nulla
	// sarebbe lo stesso difetto di prima con un colore in piu'.
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World);

	ARTUnit* Caster = SpawnEnvUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Target = SpawnEnvUnit(World, 1, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Caster"), Caster) || !TestNotNull(TEXT("Target"), Target)
		|| !TestNotNull(TEXT("TM"), TM))
	{
		DestroyEnvWorld(World);
		return false;
	}

	// L'abilita' vera del catalogo, non una ricostruita nel test: la issue nasceva proprio da uno scarto fra
	// cio' che il catalogo dichiarava e cio' che l'azione faceva.
	URTHeroData* Riva = URTHeroCatalogLibrary::MakeRiva();
	if (!TestNotNull(TEXT("Riva costruita"), Riva)) { DestroyEnvWorld(World); return false; }
	URTActionData* MistVeil = Riva->Actions.IsValidIndex(3) ? Riva->Actions[3] : nullptr;
	if (!TestNotNull(TEXT("MistVeil nel kit"), MistVeil)) { DestroyEnvWorld(World); return false; }

	Caster->Abilities[3] = MistVeil;
	Caster->PlannedAbilityIndex = 3;
	Caster->PlannedAttackTarget = Target;
	RunEnvTurn(TM);

	auto SurfaceAt = [MapActor](const FRTCellId& Cell)
	{
		const FRTHexCellData* Data = MapActor->MapAsset ? MapActor->MapAsset->FindCell(Cell) : nullptr;
		return Data ? Data->Surface : ERTHexSurface::Floor;
	};

	TestTrue(TEXT("la cella bersaglio si riempie di fumo"), SurfaceAt(FRTCellId(2, 0)) == ERTHexSurface::Smoke);
	TestTrue(TEXT("e anche una adiacente (raggio 1)"), SurfaceAt(FRTCellId(3, 0)) == ERTHexSurface::Smoke);
	TestTrue(TEXT("ma non una a due celle di distanza"), SurfaceAt(FRTCellId(4, 0)) != ERTHexSurface::Smoke);

	// L'effetto TATTICO: sparare attraverso il fumo vale al massimo 2 celle, e la regola sta gia' nel
	// terreno — `EffectiveTargetingRange` la legge dalla superficie della cella, non da uno stato dell'unita'.
	// E' la ragione per cui il fumo funziona nell'istante in cui la cella cambia.
	const int32 Effective = URTTerrainLibrary::EffectiveTargetingRange(MapActor->MapAsset,
		FRTCellId(0, 0), FRTCellId(3, 0), /*RangeCells*/ 6);
	TestEqual(TEXT("attraverso il fumo il targeting e' tagliato a 2"), Effective, 2);

	DestroyEnvWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionIgniteRadiusTest,
	"RefactorTactics.Actions.Ignite.BurnsOnlyTheTargetCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionIgniteRadiusTest::RunTest(const FString&)
{
	// Controprova del raggio come DATO (#353): tre azioni creano superfici con due raggi diversi, e prima il
	// resolver li indovinava da un `if` sul tipo di superficie. Se un giorno `SurfaceRadius` sparisse da
	// `Action.Ignite`, il fuoco erediterebbe in silenzio il raggio di qualcun altro — e un fuoco che si allarga
	// di una cella e' una modifica di bilanciamento fatta senza deciderla.
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World);

	ARTUnit* Caster = SpawnEnvUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Target = SpawnEnvUnit(World, 1, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Caster"), Caster) || !TestNotNull(TEXT("Target"), Target)
		|| !TestNotNull(TEXT("TM"), TM))
	{
		DestroyEnvWorld(World);
		return false;
	}

	PlanEnvAction(Caster, TEXT("Action.Ignite"), Target);
	RunEnvTurn(TM);

	auto SurfaceAt = [MapActor](const FRTCellId& Cell)
	{
		const FRTHexCellData* Data = MapActor->MapAsset ? MapActor->MapAsset->FindCell(Cell) : nullptr;
		return Data ? Data->Surface : ERTHexSurface::Floor;
	};

	TestTrue(TEXT("la cella bersaglio brucia"), SurfaceAt(FRTCellId(2, 0)) == ERTHexSurface::Fire);
	TestTrue(TEXT("l'adiacente NO: il fuoco ha raggio 0"), SurfaceAt(FRTCellId(3, 0)) != ERTHexSurface::Fire);

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

	// `Action.ModifyArc` ha COOLDOWN 2: al turno immediatamente successivo non e' ancora disponibile, e
	// rigiocarla subito era uno scenario che in partita non esiste. Il test lo faceva lo stesso perche'
	// `AbilityCooldowns` restava vuoto senza `BeginPlay` (#135); ora il cooldown c'e' e si aspetta il turno.
	RunEnvTurn(TM);

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

namespace
{
	/** Minimo e massimo di `GraphRevision` fra le voci di UN turno. `false` se quel turno non ha voci. */
	bool RevisionSpanOfTurn(const ARTTurnManager* TM, int32 Turn, int32& OutMin, int32& OutMax)
	{
		bool bAny = false;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.TurnNumber != Turn) { continue; }
			if (!bAny) { OutMin = OutMax = E.GraphRevision; bAny = true; continue; }
			OutMin = FMath::Min(OutMin, E.GraphRevision);
			OutMax = FMath::Max(OutMax, E.GraphRevision);
		}
		return bAny;
	}
}

/**
 * `GraphRevision` SALE DENTRO UN TURNO — `#882`, la voce di DoD di `#405` che era dichiarata «non fatto».
 *
 * Il campo sta **per voce** e non nell'header, e la ragione e' scritta nel produttore: *«letta ADESSO e non
 * a inizio turno: una porta che si apre o un ponte che crolla la fanno salire in mezzo alla risoluzione»*.
 * Ma nessun test lo osservava: `TurnLog.GraphRevisionEntersTheHash` e `.GraphRevisionRoundTrip` sono test di
 * **formato** — costruiscono le voci a mano (`E.GraphRevision = 4`) e non fanno girare niente.
 *
 * Senza questo test, un produttore che scrivesse sempre lo stesso valore lascerebbe la suite interamente
 * verde, e la scelta «per voce» sarebbe indistinguibile da un campo nell'header.
 *
 * ⚠️ **La controprova conta quanto la prova.** Un turno senza modifiche strutturali deve avere tutte le voci
 * alla STESSA revisione: senza quel controllo, un produttore che incrementasse a ogni voce — cioe' un
 * contatore invece di una revisione — passerebbe la prima meta' del test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGraphRevisionRisesWithinATurnTest,
	"RefactorTactics.TurnLog.GraphRevisionRisesWithinATurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGraphRevisionRisesWithinATurnTest::RunTest(const FString&)
{
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World);

	// Tre attori, e le loro fasi sono la sostanza del test:
	//   Guarder   `Action.Guard`      -> Preparation      voce PRIMA dell'evento
	//   Modifier  `Action.ModifyArc`  -> Attack (Blast)   l'evento strutturale
	//   Mover     `Action.Move`       -> NormalMovement   voce DOPO
	ARTUnit* Guarder = SpawnEnvUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Modifier = SpawnEnvUnit(World, 0, FRTCellId(1, 0));
	ARTUnit* Target = SpawnEnvUnit(World, 1, FRTCellId(3, 0));
	ARTUnit* Mover = SpawnEnvUnit(World, 1, FRTCellId(-3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!Guarder || !Modifier || !Target || !Mover || !TM)
	{
		DestroyEnvWorld(World);
		return false;
	}

	// --- CONTROPROVA, per prima: un turno SENZA modifiche strutturali ------------------------------
	// Va fatta prima, perche' dopo l'apertura del ponte la revisione e' salita e il confronto perderebbe
	// il suo significato.
	{
		const int32 QuietTurn = TM->GetTurnNumber(); // preso dal manager, non contato a mano
		PlanEnvAction(Guarder, TEXT("Action.Guard"), nullptr);
		// ⚠️ Il movimento si dichiara con `PlannedCell` e basta: `Action.Move` non e' un'azione di slot,
		// e' il budget della fase Move. Metterlo in `PlannedAbilityIndex` sarebbe un allestimento che
		// nessun altro test di questo repository usa — misurato, non supposto.
		Mover->PlannedCell = FRTCellId(-2, 0);
		RunEnvTurn(TM);

		int32 Min = 0, Max = 0;
		if (TestTrue(TEXT("il turno tranquillo ha lasciato voci nel log"),
			RevisionSpanOfTurn(TM, QuietTurn, Min, Max)))
		{
			TestEqual(TEXT("senza modifiche strutturali la revisione non si muove dentro il turno"), Max, Min);
		}
	}

	const int32 RevisionBefore = MapActor->MapAsset->Revision;
	const int32 EventTurn = TM->GetTurnNumber();

	// --- LA PROVA: un turno in cui un ponte si apre a meta' risoluzione ----------------------------
	PlanEnvAction(Guarder, TEXT("Action.Guard"), nullptr);
	PlanEnvAction(Modifier, TEXT("Action.ModifyArc"), Target);
	Mover->PlannedCell = FRTCellId(-1, 0); // come sopra: il Move non passa da uno slot
	RunEnvTurn(TM);

	TestEqual(TEXT("l'evento strutturale e' avvenuto una volta sola"),
		MapActor->MapAsset->Revision, RevisionBefore + 1);

	int32 Min = 0, Max = 0;
	if (TestTrue(TEXT("il turno dell'evento ha lasciato voci nel log"),
		RevisionSpanOfTurn(TM, EventTurn, Min, Max)))
	{
		// Il criterio della voce di DoD: due voci dello STESSO turno, ai due lati dell'evento, portano
		// revisioni diverse. Valori concreti, non «diverso da»: `N` e `N+1`.
		TestEqual(TEXT("la voce piu' vecchia del turno porta la revisione di PRIMA"), Min, RevisionBefore);
		TestEqual(TEXT("e la piu' recente quella di DOPO"), Max, RevisionBefore + 1);
	}

	DestroyEnvWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionModifyArcRangeTest,
	"RefactorTactics.Actions.ModifyArc.RejectsOutOfRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionModifyArcRangeTest::RunTest(const FString&)
{
	// **Nome vincolante** della DoD di #206. `Action.ModifyArc` dichiara `Range 3` a catalogo, ma fino a questo
	// checkpoint il suo ramo applicava l'operazione senza mai misurare la distanza: si apriva un ponte con un
	// bersaglio dall'altra parte della mappa. Il difetto non si vedeva perche' l'azione NON passa da
	// `ValidateInstance` — e' intercettata prima della raccolta degli intenti — quindi non ereditava il
	// controllo che ogni altra azione del Blast riceve.
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World); // raggio 4: la cella a distanza 4 esiste ed e' fuori portata

	ARTUnit* Caster = SpawnEnvUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Target = SpawnEnvUnit(World, 1, FRTCellId(4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Caster"), Caster) || !TestNotNull(TEXT("Target"), Target) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyEnvWorld(World);
		return false;
	}

	// La premessa del test e' misurata, non assunta: se il catalogo cambiasse la portata, o se la geometria
	// mettesse le due celle a distanza 3, questo test diventerebbe verde per il motivo sbagliato.
	const int32 Distance = URTHexLibrary::HexDistance(Caster->Cell, Target->Cell);
	const FRTActionDef Def = URTCatalogLibrary::FindCoreAction(FName(TEXT("Action.ModifyArc")));
	TestEqual(TEXT("Action.ModifyArc e' a catalogo"), Def.ActionId, FName(TEXT("Action.ModifyArc")));
	TestTrue(TEXT("il bersaglio e' davvero fuori dalla portata dichiarata"), Distance > Def.RangeCells);

	const int32 RevisionBefore = MapActor->MapAsset->Revision;
	const int32 ArcsBefore = MapActor->MapAsset->Transitions.Num();

	PlanEnvAction(Caster, TEXT("Action.ModifyArc"), Target);
	RunEnvTurn(TM);

	TestEqual(TEXT("nessun collegamento e' nato"), MapActor->MapAsset->Transitions.Num(), ArcsBefore);
	TestEqual(TEXT("e la topologia non e' cambiata: la revisione resta"),
		MapActor->MapAsset->Revision, RevisionBefore);

	// Il `Cancel` dichiarato dal catalogo dev'essere VISIBILE: un'azione che sparisce in silenzio e'
	// indistinguibile da un difetto, ed e' il modo in cui questo difetto e' sopravvissuto a due checkpoint.
	bool bLoggedCancel = false;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category == ERTLogCategory::Fallback
			&& E.ActionId == FName(TEXT("Action.ModifyArc"))
			&& E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Cancelled)
			&& E.Amount == static_cast<int32>(ERTActionInvalidReason::OutOfRange))
		{
			bLoggedCancel = true;
		}
	}
	TestTrue(TEXT("il TurnLog dice che e' stata annullata, e perche'"), bLoggedCancel);

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
 * Un ponte ABBATTUTO non deve lasciare un fantasma in `DynamicArcs`.
 *
 * `ARTTurnManager` tiene in `DynamicArcs` i ponti temporanei di `Action.ModifyArc`, per farli scadere. Quando
 * uno viene distrutto in combattimento, pero', a occuparsene e' `URTHexArcLibrary::DamageArc` — che di quella
 * lista non sa nulla. L'entry sopravvive al proprio ponte.
 *
 * E sopravvivere qui non e' innocuo, perche' `DamageArc` **non toglie** l'arco: lo marca `Destroyed` con
 * integrita' 0 e lo lascia sulla mappa. Quindi alla scadenza del timer del fantasma `RemoveTransition`
 * RIESCE, e fa due danni in uno: scrive nel TurnLog un `BridgeRemoved` per un crollo avvenuto due turni
 * prima — una voce che descrive un evento mai accaduto — e si porta via le macerie, cancellando la prova
 * che li' c'era un ponte abbattuto (`SetArcState` tratta `Destroyed` come terminale: «un ponte abbattuto non
 * si riattiva», e senza arco non ha piu' niente su cui essere terminale).
 *
 * E' lo stesso fantasma corretto per le coperture in #301 — `DestroyedCoverLeavesNoGhost` e' il suo gemello —
 * e la disciplina e' quella: l'entry muore quando muore la struttura che rappresenta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBridgeGhostTrackingTest,
	"RefactorTactics.Structures.Bridge.DestroyedBridgeLeavesNoGhost",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBridgeGhostTrackingTest::RunTest(const FString&)
{
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World);

	const FRTCellId Ground(0, 0, 0);
	const FRTCellId Upper(1, 0, 1);
	MapActor->MapAsset->AddOrUpdateCell(FRTHexCellData(Upper));
	MapActor->MapAsset->SortCells();

	ARTUnit* Builder = SpawnEnvUnit(World, 0, Ground);
	ARTUnit* Foe = SpawnEnvUnit(World, 1, Upper);
	// Riserve lontane dal ponte: servono TRE turni, e una squadra annientata li interromperebbe prima che il
	// timer del fantasma arrivi a scadere — il test finirebbe verde senza aver mai raggiunto il caso.
	SpawnEnvUnit(World, 0, FRTCellId(-4, 0, 0));
	SpawnEnvUnit(World, 1, FRTCellId(-3, 0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Builder"), Builder) || !TestNotNull(TEXT("Foe"), Foe)
		|| !TestNotNull(TEXT("TM"), TM))
	{
		DestroyEnvWorld(World);
		return false;
	}

	// Turno 1: il ponte nasce da `ModifyArc`, quindi e' TEMPORANEO e tracciato in `DynamicArcs` (2 turni,
	// che cominciano dal prossimo).
	PlanEnvAction(Builder, TEXT("Action.ModifyArc"), Foe);
	RunEnvTurn(TM);
	TestTrue(TEXT("il ponte esiste"),
		URTHexArcLibrary::IsArcTraversable(MapActor->MapAsset, Ground, Upper));

	// Turno 2: lo stesso costruttore lo abbatte. 40 e' `FRTHexEdge::DefaultIntegrity`: un colpo solo basta,
	// e il ponte crolla PRIMA della propria scadenza naturale — che e' esattamente la condizione del difetto.
	Builder->Abilities[0]->Def.Effects.Add(FRTActionEffectSpec(ERTActionEffect::DamageStructure, 40));
	Builder->PlannedAbilityIndex = 0;
	Builder->PlannedAttackTarget = Foe;
	RunEnvTurn(TM);

	TestEqual(TEXT("il ponte e' crollato in combattimento, un evento per verso"),
		CountEnvOutcome(TM, ERTEnvironmentOutcome::BridgeDestroyed), 2);
	TestFalse(TEXT("e non e' piu' percorribile"),
		URTHexArcLibrary::IsArcTraversable(MapActor->MapAsset, Ground, Upper));

	// Turno 3: e' adesso che scadeva il timer del ponte ormai crollato. Il fantasma agiva qui.
	RunEnvTurn(TM);

	const FRTHexEdge* Rubble = URTHexArcLibrary::FindArc(MapActor->MapAsset, Ground, Upper);
	if (TestTrue(TEXT("le macerie restano: l'arco abbattuto e' ancora sulla mappa"), Rubble != nullptr))
	{
		TestTrue(TEXT("e sono ancora macerie, non un ponte tornato attivo"),
			Rubble->State == ERTHexArcState::Destroyed);
	}
	TestEqual(TEXT("nessuna scadenza nel TurnLog: quel ponte era gia' crollato, non e' scaduto"),
		CountEnvOutcome(TM, ERTEnvironmentOutcome::BridgeRemoved), 0);

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

/**
 * CP 9.5 — `Bastion.Reconfigure` SPOSTA una copertura: non ne crea una seconda.
 *
 * E' il nome che la DoD vincola (`ReconfigureDoesNotDuplicate`), e il difetto che sorveglia e' preciso: una
 * implementazione che «aggiunge sul bordo nuovo» senza togliere dal vecchio raddoppierebbe la protezione con
 * un'azione che il catalogo descrive come una rotazione. Il test conta le voci, non guarda solo il bordo di
 * arrivo — contare e' l'unico modo di accorgersi di una duplicazione.
 *
 * Verifica anche che l'integrita' VIAGGI con la copertura: spostare un pannello ammaccato non lo ripara.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBastionReconfigureTest,
	"RefactorTactics.Heroes.Bastion.ReconfigureDoesNotDuplicate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBastionReconfigureTest::RunTest(const FString&)
{
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World);

	const FRTCellId Home(0, 0);
	const FRTCellId Panel(1, 0);

	ARTUnit* Bastion = SpawnEnvUnit(World, 0, Home);
	ARTUnit* Foe = SpawnEnvUnit(World, 1, FRTCellId(-4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Bastion"), Bastion) || !TestNotNull(TEXT("Foe"), Foe) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyEnvWorld(World);
		return false;
	}

	// Una copertura gia' in campo, ammaccata: 18 punti struttura invece di 30.
	URTHexCoverLibrary::AddCover(MapActor->MapAsset, Panel, ERTHexDirection::W, ERTHexCoverType::Low, 18);

	URTActionData* Reconfigure = URTHeroCatalogLibrary::MakeBastion()->Actions[2];
	TestTrue(TEXT("Reconfigure dichiara di spostare"),
		Reconfigure->Def.StructureOp == ERTStructureOp::MoveCover);

	PlanHeroCoverAction(Bastion, Reconfigure, Panel, ERTHexDirection::E);
	RunEnvTurn(TM);

	const FRTHexCellData* Cell = MapActor->MapAsset->FindCell(Panel);
	if (!TestNotNull(TEXT("la cella esiste"), Cell))
	{
		DestroyEnvWorld(World);
		return false;
	}

	TestEqual(TEXT("una sola copertura: spostata, non duplicata"), Cell->Covers.Num(), 1);
	TestEqual(TEXT("ora sta sul bordo di destinazione"),
		CoverIntegrityOn(MapActor->MapAsset, Panel, ERTHexDirection::E), 18);
	TestEqual(TEXT("e il bordo di partenza e' scoperto"),
		CoverIntegrityOn(MapActor->MapAsset, Panel, ERTHexDirection::W), 0);
	TestEqual(TEXT("il TurnLog registra uno spostamento, non una creazione"),
		CountEnvOutcome(TM, ERTEnvironmentOutcome::CoverMoved), 1);
	TestEqual(TEXT("nessuna copertura eretta"),
		CountEnvOutcome(TM, ERTEnvironmentOutcome::CoverCreated), 0);

	DestroyEnvWorld(World);
	return true;
}

/**
 * CP 9.5 — `Reconfigure` rifiuta invece di indovinare, e un rifiuto non fa sparire nulla.
 *
 * Due casi che una implementazione frettolosa sbaglia nello stesso modo — prendendo «la prima dell'array»:
 * due coperture sulla stessa cella (quale si sposta?) e una destinazione gia' riparata. Il secondo e' il piu'
 * pericoloso, perche' la via naturale — togli, poi aggiungi — cancella la copertura quando l'aggiunta
 * fallisce. Qui si verifica che torni dov'era.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBastionReconfigureRefusesTest,
	"RefactorTactics.Heroes.Bastion.ReconfigureRefusesInsteadOfGuessing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBastionReconfigureRefusesTest::RunTest(const FString&)
{
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World);

	const FRTCellId Home(0, 0);
	const FRTCellId Two(1, 0);   // cella con DUE coperture
	const FRTCellId One(0, 1);   // cella con una sola, ma destinazione occupata

	ARTUnit* Bastion = SpawnEnvUnit(World, 0, Home);
	ARTUnit* Foe = SpawnEnvUnit(World, 1, FRTCellId(-4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Bastion"), Bastion) || !TestNotNull(TEXT("Foe"), Foe) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyEnvWorld(World);
		return false;
	}

	URTHexCoverLibrary::AddCover(MapActor->MapAsset, Two, ERTHexDirection::W, ERTHexCoverType::Low, 30);
	URTHexCoverLibrary::AddCover(MapActor->MapAsset, Two, ERTHexDirection::E, ERTHexCoverType::Low, 30);

	URTActionData* Reconfigure = URTHeroCatalogLibrary::MakeBastion()->Actions[2];
	PlanHeroCoverAction(Bastion, Reconfigure, Two, ERTHexDirection::NE);
	RunEnvTurn(TM);

	TestEqual(TEXT("ambiguo: rifiutato"), CountEnvOutcome(TM, ERTEnvironmentOutcome::CoverRejected), 1);
	TestEqual(TEXT("e nessuna delle due si e' mossa"),
		CountEnvOutcome(TM, ERTEnvironmentOutcome::CoverMoved), 0);
	if (const FRTHexCellData* Cell = MapActor->MapAsset->FindCell(Two))
	{
		TestEqual(TEXT("le due coperture sono ancora li'"), Cell->Covers.Num(), 2);
	}

	// Destinazione gia' riparata: il rifiuto non deve far sparire la copertura di partenza.
	URTHexCoverLibrary::AddCover(MapActor->MapAsset, One, ERTHexDirection::W, ERTHexCoverType::Low, 22);
	const FRTCellId NorthEast = URTHexLibrary::Neighbors(One)[static_cast<int32>(ERTHexDirection::NE)];
	URTHexCoverLibrary::AddCover(MapActor->MapAsset, NorthEast,
		ERTHexDirection::SW, ERTHexCoverType::Low, 30); // la faccia opposta del bordo NE di `One`

	// `Bastion.Reconfigure` ha COOLDOWN 2: come sopra, il secondo rifiuto va chiesto quando l'azione e'
	// tornata disponibile, non al turno dopo (#135).
	RunEnvTurn(TM);

	URTActionData* Second = URTHeroCatalogLibrary::MakeBastion()->Actions[2];
	PlanHeroCoverAction(Bastion, Second, One, ERTHexDirection::NE);
	RunEnvTurn(TM);

	TestEqual(TEXT("destinazione occupata: rifiutato"),
		CountEnvOutcome(TM, ERTEnvironmentOutcome::CoverRejected), 1);
	TestEqual(TEXT("la copertura e' tornata dov'era, con la sua integrita'"),
		CoverIntegrityOn(MapActor->MapAsset, One, ERTHexDirection::W), 22);

	DestroyEnvWorld(World);
	return true;
}

/**
 * CP 9.5 — `Gadget.PortableCover` erige la stessa copertura, in mano a chi non e' Bastion.
 *
 * E' la prova che `Action.CreateCover` e' semantica CONDIVISA e non l'abilita' di un eroe travestita: se il
 * resolver riconoscesse il pannello per ActionId invece che per `StructureOp`, questo test sarebbe rosso — ed
 * e' esattamente la ragione per cui il campo dati esiste.
 *
 * Il gadget conserva la semantica del core e sostituisce due cose: l'identita' nel TurnLog (chi legge il
 * replay deve vedere il gadget) e il cooldown, che e' dell'oggetto — 3 turni contro i 2 del pannello d'eroe.
 * E' lo svantaggio che il gadget dichiara, e senza uno dichiarato il validator lo rifiuterebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPortableCoverGadgetTest,
	"RefactorTactics.Equipment.PortableCover.CreatesCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPortableCoverGadgetTest::RunTest(const FString&)
{
	URTEquipmentData* Gadget = URTCatalogLibrary::MakePortableCoverGadget();
	if (!TestNotNull(TEXT("il gadget esiste"), Gadget)) { return false; }

	// Passa il validator del catalogo: lo svantaggio e' dichiarato, non sottinteso.
	TArray<const URTEquipmentData*> Set;
	Set.Add(Gadget);
	TestEqual(TEXT("il gadget e' valido a catalogo"), URTCatalogLibrary::ValidateEquipment(Set).Num(), 0);
	TestEqual(TEXT("cooldown 3, come ogni gadget"), Gadget->CooldownTurns, 3);
	TestFalse(TEXT("dichiara uno svantaggio"), Gadget->Drawback.IsEmpty());

	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World);

	const FRTCellId Home(0, 0);
	const FRTCellId Target(1, 0);

	// Un'unita' QUALUNQUE: non ha il kit di Bastion, ha solo il gadget.
	ARTUnit* Carrier = SpawnEnvUnit(World, 0, Home);
	ARTUnit* Foe = SpawnEnvUnit(World, 1, FRTCellId(-4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Carrier"), Carrier) || !TestNotNull(TEXT("Foe"), Foe) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyEnvWorld(World);
		return false;
	}

	URTActionData* FromGadget = URTCatalogLibrary::MakeEquipmentAction(Gadget, Carrier);
	if (!TestNotNull(TEXT("il gadget concede un'azione"), FromGadget))
	{
		DestroyEnvWorld(World);
		return false;
	}
	TestEqual(TEXT("nel TurnLog si leggera' il gadget"),
		FromGadget->Def.ActionId, FName(TEXT("Gadget.PortableCover")));
	TestEqual(TEXT("cooldown dell'oggetto, non dell'azione"), FromGadget->Def.CooldownTurns, 3);
	TestTrue(TEXT("ma la semantica e' quella del core"),
		FromGadget->Def.StructureOp == ERTStructureOp::CreateCover);

	PlanHeroCoverAction(Carrier, FromGadget, Target, ERTHexDirection::W);
	RunEnvTurn(TM);

	TestEqual(TEXT("la copertura c'e', eretta da un gadget"),
		CoverIntegrityOn(MapActor->MapAsset, Target, ERTHexDirection::W), 30);
	TestEqual(TEXT("ed e' registrata come tale"),
		CountEnvOutcome(TM, ERTEnvironmentOutcome::CoverCreated), 1);

	bool bNamedGadget = false;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.ActionId == FName(TEXT("Gadget.PortableCover"))) { bNamedGadget = true; }
	}
	TestTrue(TEXT("il replay dice CHI l'ha eretta"), bNamedGadget);

	DestroyEnvWorld(World);
	return true;
}

/**
 * Una copertura ABBATTUTA non deve portarsi dietro un fantasma che uccide la prossima.
 *
 * `ARTTurnManager` tiene in `DynamicCovers` le coperture erette in partita, per farle scadere. Quando una
 * viene abbattuta in combattimento, pero', a toglierla dalla mappa e' `ApplyStructureDamage` — che di quella
 * lista non sa nulla. L'entry sopravvive alla propria copertura, e siccome identifica il riparo con la sola
 * coppia (cella, bordo), alla scadenza del suo timer rimuove **quello che trova su quel bordo**: se nel
 * frattempo qualcuno ha riparato lo stesso varco, gli distrugge il pannello con un turno di anticipo e scrive
 * nel TurnLog una scadenza che non e' avvenuta.
 *
 * Non e' un caso limite: due Bastion, o un Bastion e un alleato con `Gadget.PortableCover`, che rinforzano lo
 * stesso passaggio sono gioco normale — i cooldown sono per unita', quindi il secondo non aspetta il primo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverGhostTrackingTest,
	"RefactorTactics.Structures.KineticPanel.DestroyedCoverLeavesNoGhost",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverGhostTrackingTest::RunTest(const FString&)
{
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World);

	const FRTCellId Shielded(0, 0);        // la cella riparata
	const FRTCellId Attacker(1, 0);        // oltre il bordo E: il colpo lo attraversa
	const FRTCellId Builder(0, 1);
	const FRTCellId SecondBuilder(-1, 0);

	ARTUnit* First = SpawnEnvUnit(World, 0, Builder);
	ARTUnit* Second = SpawnEnvUnit(World, 0, SecondBuilder);
	ARTUnit* Breacher = SpawnEnvUnit(World, 1, Attacker);
	ARTUnit* Defender = SpawnEnvUnit(World, 0, Shielded);
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("First"), First) || !TestNotNull(TEXT("Second"), Second)
		|| !TestNotNull(TEXT("Breacher"), Breacher) || !TestNotNull(TEXT("Defender"), Defender)
		|| !TestNotNull(TEXT("TM"), TM))
	{
		DestroyEnvWorld(World);
		return false;
	}

	// Turno 1: il primo erige (integrita' 30, durata 2) e il colpo la abbatte nello stesso Blast.
	// La capacita' di sfondare si dichiara sull'istanza, come fa il test gemello dei ponti.
	PlanCoverAction(First, TEXT("Action.CreateCover"), Shielded, ERTHexDirection::E);
	Breacher->Abilities[0]->Def.Effects.Add(FRTActionEffectSpec(ERTActionEffect::DamageStructure, 30));
	Breacher->PlannedAbilityIndex = 0;
	Breacher->PlannedAttackTarget = Defender;
	RunEnvTurn(TM);

	TestEqual(TEXT("il pannello e' stato abbattuto nel turno in cui e' nato"),
		CoverIntegrityOn(MapActor->MapAsset, Shielded, ERTHexDirection::E), 0);

	// Turno 2: un'ALTRA unita' ripara lo stesso varco. Il suo pannello deve durare i propri due turni.
	PlanCoverAction(Second, TEXT("Action.CreateCover"), Shielded, ERTHexDirection::E);
	RunEnvTurn(TM);

	// E' QUESTO il turno che il difetto sbagliava: il timer del primo pannello — abbattuto un turno fa —
	// scadeva proprio adesso e portava via il pannello appena eretto da un'altra unita'.
	TestEqual(TEXT("il nuovo pannello e' in piedi: nessun fantasma lo ha portato via"),
		CoverIntegrityOn(MapActor->MapAsset, Shielded, ERTHexDirection::E), 30);
	TestEqual(TEXT("e nel TurnLog non c'e' una scadenza mai avvenuta"),
		CountEnvOutcome(TM, ERTEnvironmentOutcome::CoverExpired), 0);

	// Poi scade quando deve: eretto al turno 2 con durata 2, cade nel Cleanup del turno 3. La sua scadenza
	// e' un evento legittimo — e verificarla qui distingue «non e' stato ucciso in anticipo» da «non muore
	// mai», che sarebbe il difetto opposto.
	RunEnvTurn(TM);
	TestEqual(TEXT("alla propria scadenza, invece, se ne va"),
		CoverIntegrityOn(MapActor->MapAsset, Shielded, ERTHexDirection::E), 0);
	TestEqual(TEXT("e questa scadenza e' registrata"),
		CountEnvOutcome(TM, ERTEnvironmentOutcome::CoverExpired), 1);

	DestroyEnvWorld(World);
	return true;
}

// =====================================================================================================
// `#570` — una superficie che NASCE fa effetto a chi ci si trova sopra.
//
// Fino a qui la regola valeva per l'acqua sola, con un `if (Created == ShallowWater)` scritto a mano e un
// commento che dichiarava il problema nella sua forma generale. Vero per l'acqua e per tutte le altre:
// un'unita' ferma su cui veniva acceso un incendio **non prendeva fuoco**, perche' gli `OnEnterEffects` li
// applica solo chi ENTRA e il danno del Cleanup dipende dallo STATO, non dalla cella.
//
// Era anche cio' che teneva inerte `Reaction.HazardEscape` (`#505`): nel Cleanup non c'era nessun danno
// imminente da cui fuggire.
// =====================================================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBornSurfaceBurnsOccupantTest,
	"RefactorTactics.Environment.BornSurfaceAffectsOccupant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBornSurfaceBurnsOccupantTest::RunTest(const FString&)
{
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World);

	ARTUnit* Caster = SpawnEnvUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Target = SpawnEnvUnit(World, 1, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Caster"), Caster) || !TestNotNull(TEXT("Target"), Target)
		|| !TestNotNull(TEXT("TM"), TM))
	{
		DestroyEnvWorld(World);
		return false;
	}

	const int32 HpPrima = Target->Health;
	PlanEnvAction(Caster, TEXT("Action.Ignite"), Target);
	RunEnvTurn(TM);

	// Premessa: la cella e' davvero diventata fuoco. Senza, il resto del test parlerebbe di un incendio che
	// non c'e' e passerebbe per la ragione sbagliata.
	const FRTHexCellData* Data = MapActor->MapAsset ? MapActor->MapAsset->FindCell(FRTCellId(2, 0)) : nullptr;
	if (!TestTrue(TEXT("premessa: la cella del bersaglio e' in fiamme"),
		Data != nullptr && Data->Surface == ERTHexSurface::Fire))
	{
		DestroyEnvWorld(World);
		return false;
	}

	// Chi era gia' li' subisce quello che subisce chi entra: i 10 danni del catalogo terreni **e** `Burning`.
	// Prima di `#570` non prendeva niente.
	TestTrue(TEXT("chi era sulla cella brucia"), Target->HasStatus(TAG_Status_Burning));

	// Il numero non e' inventato: 10 d'ingresso dal catalogo terreni piu' il danno che `Burning` fa nello
	// stesso Cleanup, perche' `ResolveEnvironment` gira PRIMA del ciclo che lo fa pagare. E' la stessa somma
	// che paga chi ci entra col Move, quindi la regola resta una sola.
	const int32 Atteso = 10 + URTCombatLibrary::BurningCleanupDamage;
	TestEqual(TEXT("e paga ingresso + bruciatura, come chi ci fosse entrato"), HpPrima - Target->Health, Atteso);

	DestroyEnvWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBornSurfaceIsNotOnlyFireTest,
	"RefactorTactics.Environment.BornSurfaceRuleIsNotPerSurface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBornSurfaceIsNotOnlyFireTest::RunTest(const FString&)
{
	// La regola legge il CATALOGO, non un elenco di superfici nel resolver: il fumo che nasce su un'unita' la
	// oscura, senza che nessuno abbia dovuto aggiungere un ramo per lui. E' la parte che impedisce alla
	// prossima superficie di nascere di nuovo muta.
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnEnvMap(World);

	ARTUnit* Caster = SpawnEnvUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Target = SpawnEnvUnit(World, 1, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!Caster || !Target || !TM) { DestroyEnvWorld(World); return false; }

	URTHeroData* Riva = URTHeroCatalogLibrary::MakeRiva();
	URTActionData* MistVeil = (Riva && Riva->Actions.IsValidIndex(3)) ? Riva->Actions[3] : nullptr;
	if (!TestNotNull(TEXT("MistVeil nel kit di Riva"), MistVeil)) { DestroyEnvWorld(World); return false; }

	Caster->Abilities[3] = MistVeil;
	Caster->PlannedAbilityIndex = 3;
	Caster->PlannedAttackTarget = Target;
	RunEnvTurn(TM);

	TestTrue(TEXT("il fumo nato sull'unita' la oscura"), Target->HasStatus(TAG_Status_Obscured));
	DestroyEnvWorld(World);
	return true;
}

// =====================================================================================================
// CP 7.5 (`#505`) — `Reaction.HazardEscape`: si fugge PRIMA del danno, non dopo.
//
// L'ultimo dei sette moduli, e quello a cui mancava l'evento invece del dato: finche' una superficie che
// nasceva sotto un'unita' ferma non le faceva niente (`#570`), non c'era nulla da cui fuggire.
//
// L'oracolo e' doppio, e serve: l'unita' deve **essere altrove** e **non avere `Burning`**. Solo la prima
// meta' passerebbe anche se la fuga avvenisse dopo l'applicazione degli effetti — che e' esattamente il modo
// in cui questo modulo sarebbe stato inutile pur sembrando funzionante.
// =====================================================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHazardEscapeFleesBeforeDamageTest,
	"RefactorTactics.Equipment.HazardEscape.FleesBeforeDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHazardEscapeFleesBeforeDamageTest::RunTest(const FString&)
{
	const URTEquipmentData* Escape = nullptr;
	for (const URTEquipmentData* M : URTCatalogLibrary::MakeReactionModules())
	{
		if (M && M->EquipmentId == FName(TEXT("Reaction.HazardEscape"))) { Escape = M; break; }
	}
	if (!TestNotNull(TEXT("`Reaction.HazardEscape` e' nel catalogo dei moduli"), Escape)) { return false; }

	// --- PREMESSA: senza il modulo, il fuoco acceso sotto i piedi brucia -----------------------------
	{
		UWorld* World = MakeEnvWorld();
		if (!TestNotNull(TEXT("world della premessa"), World)) { return false; }
		SpawnEnvMap(World);
		ARTUnit* Caster = SpawnEnvUnit(World, 0, FRTCellId(0, 0));
		ARTUnit* Target = SpawnEnvUnit(World, 1, FRTCellId(2, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Caster || !Target || !TM) { DestroyEnvWorld(World); return false; }

		const int32 HpPrima = Target->Health;
		PlanEnvAction(Caster, TEXT("Action.Ignite"), Target);
		RunEnvTurn(TM);

		const bool bBruciato = (Target->Health < HpPrima) && Target->HasStatus(TAG_Status_Burning);
		const bool bFermo = (Target->Cell == FRTCellId(2, 0));
		DestroyEnvWorld(World);
		if (!TestTrue(TEXT("premessa: senza modulo l'unita' resta e brucia"), bBruciato && bFermo))
		{
			return false; // senza un incendio che fa male, il caso sotto non proverebbe niente
		}
	}

	// --- IL CASO: con il modulo, si sposta e non brucia ----------------------------------------------
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World);

	ARTUnit* Caster = SpawnEnvUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Target = SpawnEnvUnit(World, 1, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!Caster || !Target || !TM) { DestroyEnvWorld(World); return false; }

	URTActionData* Reazione = URTCatalogLibrary::MakeEquipmentAction(Escape, Target);
	if (!TestNotNull(TEXT("il modulo concede un'azione"), Reazione)) { DestroyEnvWorld(World); return false; }
	TestTrue(TEXT("ed e' una reazione col trigger dell'ambiente, ereditato da `Action.Evade`"),
		Reazione->Def.Slot == ERTActionSlot::Reaction
		&& Reazione->Def.ReactionTrigger == ERTReactionTrigger::CellBecameHazardous);
	Target->Abilities.Add(Reazione);
	Target->PlannedReactionAbility = Target->Abilities.Num() - 1;

	// ⚠️ Il facing e' `W`, e la scelta NON e' indifferente: l'ordine canonico di ripiego comincia da `E`,
	// quindi con `Facing = E` la cella «davanti» coinciderebbe con la prima di ripiego e l'assert in fondo
	// passerebbe anche se il facing venisse ignorato del tutto. Con `W` le due celle sono diverse — (1,0)
	// contro (3,0) — e il test distingue davvero le due regole.
	//
	// Trovato dalla verifica di mutazione: disattivando il ramo del facing in `FindEscapeCell` la suite
	// restava verde, ed e' l'unico modo in cui questo difetto poteva emergere.
	Target->Facing = ERTHexDirection::W;
	const int32 HpPrima = Target->Health;

	PlanEnvAction(Caster, TEXT("Action.Ignite"), Target);
	RunEnvTurn(TM);

	// Premessa interna: la cella di partenza e' davvero andata a fuoco. Senza, «si e' spostato e non brucia»
	// sarebbe vero anche in una scena dove non succede niente.
	const FRTHexCellData* Partenza = MapActor->MapAsset ? MapActor->MapAsset->FindCell(FRTCellId(2, 0)) : nullptr;
	if (!TestTrue(TEXT("premessa: la cella di partenza e' in fiamme"),
		Partenza != nullptr && Partenza->Surface == ERTHexSurface::Fire))
	{
		DestroyEnvWorld(World);
		return false;
	}

	// I due assert che contano, e servono ENTRAMBI: il primo da solo passerebbe anche se la fuga arrivasse
	// dopo l'applicazione degli effetti — cioe' se il modulo fosse inutile.
	TestTrue(TEXT("si e' spostato dalla cella in fiamme"), Target->Cell != FRTCellId(2, 0));
	TestFalse(TEXT("e la fuga e' arrivata PRIMA del danno: non brucia"), Target->HasStatus(TAG_Status_Burning));
	TestEqual(TEXT("e non ha perso salute"), Target->Health, HpPrima);

	// E' andato DOVE GUARDAVA: la fuga e' prevedibile, non arbitraria.
	TestEqual(TEXT("verso la cella che aveva davanti, non verso la prima dell'ordine canonico"),
		Target->Cell, URTHexLibrary::Neighbor(FRTCellId(2, 0), ERTHexDirection::W));
	TestTrue(TEXT("e infatti NON e' finito nella cella che il ripiego avrebbe scelto"),
		Target->Cell != URTHexLibrary::Neighbor(FRTCellId(2, 0), ERTHexDirection::E));

	DestroyEnvWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
