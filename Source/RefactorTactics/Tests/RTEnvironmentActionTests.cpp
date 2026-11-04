#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTEquipmentData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Combat/RTCombatLibrary.h" // BurningCleanupDamage: il test somma ingresso + bruciatura (#570)
#include "Combat/RTHexCombatLibrary.h" // #3279: la riduzione legge la faccia della PROPRIA cella
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
#include "Turn/RTActionQueueLibrary.h" // SortUnitsForResolution: l'ordine in cui il resolver passa le unita' (#2922)
#include "Turn/RTPlaybackLibrary.h" // #3281: il confine d'atto, misurato sull'aggregato
#include "Turn/RTTurnLogLibrary.h" // #1150: i predicati che dichiarano chi ha inflitto e chi ha subito
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Tests/RTAbilityFixtures.h"
#include "HAL/FileManager.h" // #3701: il cancello del produttore dei muri colpiti legge i sorgenti
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

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
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return Actor;
	}

	/**
	 * L'unita' di prova. `Hero` nullo significa **Ivrin**, che e' l'eroe di default di questo file.
	 *
	 * ⚠️ Il parametro esiste dal `#3281`, che ha bisogno di due unita' con azioni base DIVERSE. Sta qui
	 * invece che in un secondo helper perche' la sequenza di spawn e' UNA: duplicarla vorrebbe dire due
	 * copie che divergono alla prima riga aggiunta a una sola delle due.
	 */
	ARTUnit* SpawnEnvUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell, URTHeroData* Hero = nullptr)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false;
		U->ConfigureFromHeroData(Hero ? Hero : URTHeroCatalogLibrary::MakeIvrin());
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell;
		return U;
	}

	void PlanEnvAction(ARTUnit* Caster, const TCHAR* ActionId, ARTUnit* Target)
	{
		RTAbilityFixtures::AddCoreAbilityInSlot(Caster, ActionId, 3);
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
		// ⚠️ Il `Power` lo azzera la fixture derivandolo dal catalogo. Prima questo helper non lo toccava
		// affatto, quindi restava al default legacy **30**: `Action.CreateCover` non dichiara `Damage`, e
		// il resolver ricade sullo specchio (`DeclaredDamage > 0 ? DeclaredDamage : Ability->Power`).
		// Una struttura eretta portava con se' trenta danni che nessuna riga di catalogo autorizza (#1588).
		RTAbilityFixtures::AddCoreAbilityInSlot(Caster, ActionId, 3);
		Caster->PlannedAbilityIndex = 3;
		Caster->PlannedAttackTarget = nullptr;
		Caster->PlannedAttackCell = TargetCell;
		Caster->bAttackTargetsCell = true;
		Caster->PlannedCoverEdge = Edge;
		Caster->bHasPlannedCoverEdge = true;
	}

	/**
	 * Come `PlanCoverAction`, ma con un'abilita' d'EROE gia' costruita dal catalogo (e la sua variante attiva).
	 * Serve perche' il pannello di Branth non e' un'azione core: e' un'azione core con un nome d'eroe, e la
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

	/**
	 * Le voci canoniche del danno da `Status.Burning` di UNA unita', per esito (`#625`).
	 *
	 * Gemella di `CountEnvOutcome`, e nata dalla stessa ragione: i test di `#625` scrivevano ciascuno il
	 * proprio giro su `GetTurnLog()`, terzo e quarto loop aperto dello stesso file. Trovato in code review.
	 */
	int32 CountBurningEntries(const ARTTurnManager* TM, int32 UnitId, ERTCombatOutcome Outcome)
	{
		int32 N = 0;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
				// ⚠️ **La categoria fa parte del selettore da #1077**: da quando lo stato ha un vocabolario
				// suo, `ActionId == Status.Burning` non identifica piu' una voce sola — nascita, revoca e
				// scadenza portano lo stesso tag. Qui si cerca il DANNO, che e' `Combat`.
			if (E.Category == ERTLogCategory::Combat
				&& E.ActionId == FName(TEXT("Status.Burning"))
				&& E.UnitId == UnitId
				&& E.Outcome == static_cast<uint8>(Outcome))
			{
				++N;
			}
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
	// Issue #353. `Muiren.MistVeil` dichiarava «crea fumo raggio 1» e non lo faceva: `Smoke` era l'unica delle
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
	URTHeroData* Muiren = URTHeroCatalogLibrary::MakeMuiren();
	if (!TestNotNull(TEXT("Muiren costruita"), Muiren)) { DestroyEnvWorld(World); return false; }
	URTActionData* MistVeil = Muiren->Actions.IsValidIndex(3) ? Muiren->Actions[3] : nullptr;
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

// =====================================================================================================
// `Status.Stunned` contro i due siti che il rifiuto NON copriva ([D-416], `#3142`).
//
// 🔴 **Nati da una code review, non da un'intuizione.** L'implementazione metteva la guardia in
// `ResolvePrep` e a meta' di `CollectAttackIntents`, e i siti che consumano un'azione principale erano
// quei due e `ResolveEnvironment` (dal 2026-11-01 la negano anche `ResolveCleanseActions` e
// `CollectHealActions`, [D-564], e `ResolveCoverStructures`, [D-565]):
// il ramo `ModifyArc` la PRECEDE dentro lo stesso ciclo, e le ambientali risolvono altrove.
// Un'unita' stordita cambiava la topologia della mappa e accendeva incendi.
//
// ⚠️ **Entrambi i test hanno un braccio di CONTROLLO**, e non e' abbondanza: senza, un'asserzione «non e'
// successo niente» sarebbe verde anche se il banco non facesse succedere niente comunque. I bracci di
// controllo sono i test qui sopra — `BumpsChunkRevision` e `BurnsOnlyTheTargetCell` — quindi qui si
// asserisce lo stato PRIMA e DOPO sulla stessa istanza, che e' la stessa protezione in forma piu' corta.
// =====================================================================================================

/**
 * Uno stordito non cambia la topologia: `Action.ModifyArc` e' un'azione **principale**.
 *
 * Lo slot non e' un'opinione: `ShippedAction` ha `ERTActionSlot::Main` come default e la riga di
 * `Action.ModifyArc` non lo sovrascrive. ∴ [D-416] la nega, e il ramo che la intercetta prima della
 * raccolta degli intenti deve passare dalla guardia come tutto il resto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStunnedCasterDoesNotModifyArcTest,
	"RefactorTactics.Actions.ModifyArc.StunnedCasterChangesNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStunnedCasterDoesNotModifyArcTest::RunTest(const FString&)
{
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

	Caster->ApplyStatus(TAG_Status_Stunned, URTCombatLibrary::StunnedDurationTurns);
	PlanEnvAction(Caster, TEXT("Action.ModifyArc"), Target);
	RunEnvTurn(TM);

	TestEqual(TEXT("nessun collegamento nuovo: l'azione non e' partita"),
		MapActor->MapAsset->Transitions.Num(), ArcsBefore);
	TestEqual(TEXT("e la revisione non si muove"), MapActor->MapAsset->Revision, RevisionBefore);

	// ⚠️ **Il turno lo DICE**: un'azione che sparisce in silenzio e' indistinguibile da un difetto. E' la
	// disciplina che `Fallback`/`Cancelled` esiste per applicare, e il motivo viaggia in `Amount`.
	bool bRifiutata = false;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category == ERTLogCategory::Fallback
			&& E.Amount == static_cast<int32>(ERTActionInvalidReason::Stunned))
		{
			bRifiutata = true;
		}
	}
	TestTrue(TEXT("il TurnLog porta il rifiuto per stordimento"), bRifiutata);

	DestroyEnvWorld(World);
	return true;
}

/**
 * Uno stordito non accende: le azioni **ambientali** sono principali, e risolvono in `ResolveEnvironment`.
 *
 * `Action.Ignite` esce da `CollectAttackIntents` col piano INTATTO — quel ciclo lascia passare la fase
 * `Cleanup` apposta — e lo consuma `ResolveEnvironment`. Senza una guardia li', il rifiuto degli altri siti
 * non la tocca: e' esattamente il buco che questo test pinna.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStunnedCasterDoesNotIgniteTest,
	"RefactorTactics.Actions.Ignite.StunnedCasterLightsNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStunnedCasterDoesNotIgniteTest::RunTest(const FString&)
{
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

	Caster->ApplyStatus(TAG_Status_Stunned, URTCombatLibrary::StunnedDurationTurns);
	PlanEnvAction(Caster, TEXT("Action.Ignite"), Target);
	RunEnvTurn(TM);

	const FRTHexCellData* Data = MapActor->MapAsset ? MapActor->MapAsset->FindCell(FRTCellId(2, 0)) : nullptr;
	const ERTHexSurface Superficie = Data ? Data->Surface : ERTHexSurface::Floor;
	TestTrue(TEXT("la cella bersaglio NON brucia"), Superficie != ERTHexSurface::Fire);

	bool bRifiutata = false;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category == ERTLogCategory::Fallback
			&& E.Amount == static_cast<int32>(ERTActionInvalidReason::Stunned))
		{
			bRifiutata = true;
		}
	}
	TestTrue(TEXT("il TurnLog porta il rifiuto per stordimento"), bRifiutata);

	DestroyEnvWorld(World);
	return true;
}

// =====================================================================================================
// `RIC-5` (#3958) — uno stordito NON erige e NON sposta una copertura di BORDO ([D-565], [D-416] (1)).
//
// [D-416] (1) toglie allo stordito «l'azione principale e la reazione per la sua durata». Il rifiuto e'
// implementato PER SITO — `ResolvePrep`, `CollectAttackIntents`, `ResolveEnvironment`, `CollectHealActions` e
// `ResolveCleanseActions` lo scrivono — e `ResolveCoverStructures` non era fra loro: `ResolvePrep` la chiama PRIMA
// del ciclo che rifiuta, e lei consumava il piano di chi dichiara uno `StructureOp` senza passare da
// `RefuseMainActionIfStunned`. Questo banco nacque come CARATTERIZZAZIONE di quel comportamento — lo eseguiva: la
// stordita erigeva, spostava e si attivava — e con la decisione si INVERTE: il braccio STORDITO asserisce cio' che
// [D-565] vuole, il braccio di CONTROLLO non cambia.
//
// Cio' che [D-565] fissa, e che il banco pinna riga per riga:
//   · la struttura NON nasce e NON si sposta, e nessun `AbilityActivated` esce; esce UNA voce `Fallback`/`Cancelled`/
//     `Stunned` di fase Prep che nomina l'azione, la cella di chi agisce e la cella mirata;
//   · il rifiuto PAGA la ricarica ([D-563] (2) e [D-564], come `ResolvePrep`): scrittura immediata, sullo slot
//     dell'azione e su nessun altro — anche per `Reconfigure`, che una rotazione gratuita avrebbe lasciato gratis;
//   · la rotazione gratuita del pannello adattivo SOPRAVVIVE al rifiuto: il contatore `FreeRotations` non si tocca, e non
//     cresce (sesto turno del flusso della rotazione);
//   · il rifiuto e' di UNA unita': il ciclo prosegue, la struttura di un compagno ordinato DOPO la stordita nasce (riga
//     «con testimone»), e quella di un compagno ordinato PRIMA non si perde (riga «con testimone anticipato»);
//   · la regola vale per DISCRIMINANTE (la `StructureOp` che erige o sposta) e non per nome: oltre alle azioni del catalogo
//     core e dell'eroe, la riga di `Gadget.PortableCover`, che copia la `StructureOp` di `Action.CreateCover` e porta un nome suo;
//   · ⌫ [D-569] (2026-11-03) ribalta il punto che stava qui — *«il rifiuto e' dell'AZIONE, non del percorso: il passo normale di
//     una stordita si cammina ([D-416] (1): il movimento lo toglie `Status.Root`)»*. Una stordita non si muove: il suo passo e'
//     NEGATO, in ogni forma in cui lo si pianifica (il percorso di un passo, il composito, la sola destinazione), con una voce
//     `Fallback`/`Cancelled`/`Stunned` di fase Move accanto a quella di Prep. Il braccio di CONTROLLO cammina ancora;
//   · il fatto di timeline che apre il playback non e' piu' il passo dell'attore ma quello di un COMPAGNO non stordito
//     (`Bed.Opener`), che cammina in entrambi i bracci;
//   · il rifiuto non e' un'operazione APPLICATA: non conta nel ritorno di `ResolveCoverStructures`, quindi non accende
//     il beat di Prep del playback — che si legge dove il playback c'e' di sicuro, cioe' sulle righe col passo;
//   · NON cambia un'azione GIA' in ricarica (scarto muto di `CanUseAbility`, prima della guardia: niente voce e niente
//     secondo addebito), ne' un attore caduto (il ciclo lo salta), ne' i `Reject` di raccolta di chi NON e' stordito,
//     che restano gratis (`RIC-3`, [D-571]): «senza bersaglio» il click non lo produce e «fuori portata» lo rifiuta, mentre
//     «senza bordo» il click lo ACCETTA (`HandleClickOnUnit` non ha la guardia sul kind) ed e' `BLOCKED — DECISION
//     REQUIRED`; la riga «senza bordo» pinna che la guardia sta PRIMA di quei `Reject`.
//
// ⚠️ **Ogni riga ha il suo braccio di CONTROLLO**: stessa unita', stesso piano, mondo ricostruito da capo, senza
// stordimento. Senza, «non e' successo niente» sarebbe verde anche se il banco non facesse succedere niente. In coda
// stanno i controlli POSITIVI del contatore dei rifiuti — lo stesso predicato, su un sito di Prep (`Action.Overwatch`)
// e su uno di Cleanup (`Action.Ignite`) che il rifiuto lo scrivono.
//
// Prefisso `CoverStun*` negli helper: unity build, namespace anonimi fusi con gli altri file di test.
// =====================================================================================================
namespace
{
	/** La struttura di bordo che l'attore del banco `RIC-5` pianifica. */
	enum class ECoverStunKind : uint8
	{
		CreateCover,  //< `Action.CreateCover` del catalogo core: erige
		KineticPanel, //< `Hero.Branth.KineticPanel` senza variante: erige, 30 punti struttura per due turni
		Reconfigure,  //< `Hero.Branth.Reconfigure` su una copertura di MAPPA: sposta, e paga (non e' in `DynamicCovers`)
		PortableCover, //< `Gadget.PortableCover`: l'azione che il GADGET concede (`MakeEquipmentAction`) copia la `StructureOp` di `Action.CreateCover` e cambia nome e ricarica: erige, con un nome che nessuna guardia per nome conosce
	};

	/**
	 * In che situazione si trova l'attore, oltre al tipo di struttura. Ogni flag e' UN'altra uscita del ciclo di
	 * `ResolveCoverStructures`, e la riga ne pinna la posizione rispetto alla guardia dello stordimento.
	 */
	struct FCoverStunScene
	{
		ECoverStunKind Kind = ECoverStunKind::CreateCover;
		bool bAlreadyCooling = false; //< lo slot parte in ricarica (`Mirror - 1`): `CanUseAbility` e' falso e il piano si scarta in silenzio
		bool bNoEdge = false;         //< piano incompleto: il bersaglio c'e' e il bordo no (il `Reject` di raccolta di chi non e' stordito)
		bool bFallen = false;         //< l'attore e' a zero salute prima del turno
		bool bNoTarget = false;       //< piano incompleto: ne' unita' ne' cella bersaglio (il `Reject` «nessun bersaglio»)
		bool bOutOfRange = false;     //< la cella mirata e' oltre la portata dichiarata (il `Reject` «fuori portata»)
		bool bWithWitness = false;    //< un compagno VIVO, ordinato DOPO l'attore, erige altrove nello stesso Prep: se la sua copertura nasce, il ciclo ha superato l'attore
		bool bWithStep = false;       //< l'attore pianifica ANCHE un passo normale verso una cella adiacente e libera: un fatto di timeline che il playback mostra
		bool bWithEarlyWitness = false; //< un compagno VIVO, ordinato PRIMA dell'attore, erige altrove nello stesso Prep: la sua operazione e' gia' RACCOLTA quando scatta la guardia, e deve risultare applicata
		bool bWithDetour = false;     //< come `bWithStep`, ma col percorso COMPOSITO {partenza, deviazione, destinazione}: piu' lungo di quello che il pathfinder calcolerebbe verso la stessa destinazione
		bool bDestinationOnly = false; //< come `bWithStep`, ma di SOLA DESTINAZIONE (`PlannedPath` vuoto): la forma dei bot, il cui percorso nasce dentro la fase Move
	};

	/** La stessa scena con il testimone, col passo, ...: i flag si aggiungono a qualunque riga senza allungare la sua lista posizionale. */
	FCoverStunScene CoverStunWithWitness(FCoverStunScene Scene) { Scene.bWithWitness = true; return Scene; }
	FCoverStunScene CoverStunWithStep(FCoverStunScene Scene) { Scene.bWithStep = true; return Scene; }
	FCoverStunScene CoverStunWithEarlyWitness(FCoverStunScene Scene) { Scene.bWithEarlyWitness = true; return Scene; }
	FCoverStunScene CoverStunWithDetour(FCoverStunScene Scene) { Scene.bWithDetour = true; return Scene; }
	FCoverStunScene CoverStunWithDestinationOnly(FCoverStunScene Scene) { Scene.bDestinationOnly = true; return Scene; }

	/**
	 * Lo slot dell'azione sotto prova. `PlanCoverAction` e `PlanHeroCoverAction` scrivono sempre il 3. Lo `StableUnitId` e'
	 * `indice + 1` sul roster ordinato per squadra e cella (`EnsureMatchRoster`): vale 1 e 2 con due unita', e la terza
	 * arriva a 3 — proprio lo slot. Un pagamento scritto con l'identita' dell'unita' al posto dell'indice dell'abilita'
	 * si vedrebbe solo se i due numeri differiscono: `CoverStunPlayTurn` lo verifica per l'attore (`StableId != CoverStunSlot`).
	 */
	constexpr int32 CoverStunSlot = 3;

	/** Tutto cio' che una run del banco misura. I campi `premessa` decidono se la run VALE: se mancano, non si deduce nulla. */
	struct FCoverStunOutcome
	{
		bool bValid = false;
		bool bStunnedAtCommit = false;  //< premessa: `HasStatus(Stunned)` un attimo prima del turno
		bool bPlanAtCommit = false;     //< premessa: il piano c'e' ancora un attimo prima del turno
		bool bMainInPrep = false;       //< premessa: l'azione e' una PRINCIPALE di fase Prep, cioe' la classe che [D-416] nega
		FName ActionId;                 //< l'azione installata nello slot
		FRTCellId Home;                 //< la cella di chi agisce: e' la `SrcCell` che la voce di rifiuto deve portare
		FRTCellId Aimed;                //< la cella che il piano mira: e' la `TgtCell` che la voce di rifiuto deve portare
		int32 StableId = 0;             //< premessa: dopo il turno non e' 0 (a zero, ogni predicato su `UnitId` e' cieco)
		int32 DefCooldown = -1;         //< `Def.CooldownTurns` dell'azione
		int32 MirrorCooldown = -1;      //< lo specchio `URTActionData::CooldownTurns`, che e' quello che scrive `ConsumeAbility`
		int32 CooldownBefore = -1;      //< residuo nello slot prima del turno: 0 = utilizzabile, `Mirror - 1` = gia' in ricarica
		int32 CooldownAfter = -1;       //< residuo a fine turno: il Cleanup decrementa di uno, quindi `Mirror - 1` = la ricarica e' PARTITA
		TArray<int32> SlotsBefore;      //< il residuo di OGNI slot di abilita' prima del turno
		TArray<int32> SlotsAfter;       //< ... e dopo: un addebito sullo slot sbagliato non deve passare per uno scritto bene
		int32 WBefore = 0;              //< integrita' sul bordo W della cella bersaglio prima del turno
		int32 WAfter = 0;               //< ... e dopo
		int32 EBefore = 0;              //< integrita' sul bordo E prima del turno
		int32 EAfter = 0;               //< ... e dopo
		int32 CoversAfter = 0;          //< coperture presenti sulla cella bersaglio a fine turno
		int32 Created = 0;              //< voci ambientali `CoverCreated`, di chiunque
		int32 Moved = 0;                //< voci ambientali `CoverMoved`
		int32 Rejected = 0;             //< voci ambientali `CoverRejected`
		int32 Activations = 0;          //< eventi `AbilityActivated` dell'attore
		int32 StunRefusals = 0;         //< voci `Fallback`/`Cancelled`/`Stunned` dell'attore
		int32 RefusalsAnyUnit = 0;      //< ... di CHIUNQUE: non dipende dallo `StableUnitId` dell'attore
		int32 AnyFallbacks = 0;         //< voci `Fallback` dell'attore, di qualunque causa
		int32 FallbacksAnyUnit = 0;     //< ... di chiunque e di qualunque causa: «nessun altro Fallback»
		int32 EntriesOfActor = 0;       //< tutte le voci del TurnLog che nominano l'attore
		int32 PrepRefusalsAnyUnit = 0;  //< ... di CHIUNQUE e di fase Prep: l'unico rifiuto che questo banco decide
		// I quattro campi che seguono sono della PRIMA voce di rifiuto dell'attore. Le celle partono da una SENTINELLA: la
		// cella di default, (0,0), e' proprio quella di chi agisce, e «nessun rifiuto» passerebbe per «rifiuto dalla cella giusta».
		ERTMatchPhase RefusalPhase = ERTMatchPhase::Planning;
		FName RefusalActionId;
		FRTCellId RefusalSrc = FRTCellId(-99, -99, -99);
		FRTCellId RefusalTgt = FRTCellId(-99, -99, -99);
		int32 WitnessWAfter = 0;        //< solo con testimone: integrita' sul bordo W della sua copertura
		int32 WitnessCreated = 0;       //< ... e voci `CoverCreated` che portano il suo `StableUnitId`
		int32 EarlyWAfter = 0;          //< solo con testimone anticipato: integrita' sul bordo W della sua copertura
		int32 EarlyCreated = 0;         //< ... e voci `CoverCreated` che portano il suo `StableUnitId`
		// Il passo normale. Le celle partono da una SENTINELLA, come quelle del rifiuto: (0,0) e' proprio la cella di partenza,
		// e «non si e' mosso» passerebbe per «e' arrivato».
		bool bMoveAtCommit = false;     //< premessa: l'attore ha un movimento normale pianificato un attimo prima del turno
		FRTCellId Destination = FRTCellId(-99, -99, -99); //< la cella che il piano raggiunge
		FRTCellId FinalCell = FRTCellId(-99, -99, -99);   //< dov'e' l'attore a fine turno
		bool bSawPrepBeat = false;      //< il playback ha MOSTRATO la fase Prep (letta subito dopo il lock-in e dopo ogni tick)
		bool bSawMoveBeat = false;      //< ... la fase Move: la prova che il playback si e' aperto (da D-569 la porta il compagno che cammina, non l'attore stordito)
		TArray<FRTCellId> PlannedRoute;  //< il percorso scritto nel piano un attimo prima del turno: vuoto per un piano di sola destinazione
		TArray<FRTCellId> ExpectedRoute; //< la rotta che il piano promette: il suo percorso se c'e', altrimenti partenza e destinazione (adiacenti: lo dice una premessa)
		TArray<FRTCellId> WalkedRoute;   //< la rotta dell'evento `Move` di fase Move dell'attore: vuota se l'attore non ha camminato
		// D-569: una stordita non cammina. Il passo dell'attore stordito e' NEGATO (voce di rifiuto in fase Move), e il fatto di
		// timeline che apre il playback lo porta un COMPAGNO non stordito che cammina sempre, in entrambi i bracci.
		bool bOpener = false;                              //< la scena ha il compagno apritore
		FRTCellId OpenerPlanned = FRTCellId(-99, -99, -99); //< dove il piano porta l'apritore
		FRTCellId OpenerFinal = FRTCellId(-99, -99, -99);   //< dov'e' l'apritore a fine turno
		int32 MoveRefusals = 0;                            //< voci di rifiuto per stordimento di fase Move dell'attore
		FRTCellId MoveRefusalSrc = FRTCellId(-99, -99, -99);
		FRTCellId MoveRefusalTgt = FRTCellId(-99, -99, -99);
		uint8 ActorMoveOutcome = MAX_uint8;                //< l'esito della voce `Move` dell'attore (MAX_uint8 = nessuna voce)
	};

	/** Una voce di rifiuto per stordimento, di CHIUNQUE: `Fallback`/`Cancelled` con la causa `Stunned` in `Amount`. */
	bool CoverStunIsRefusal(const FRTTurnLogEntry& E)
	{
		return E.Category == ERTLogCategory::Fallback && E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Cancelled)
			&& E.Amount == static_cast<int32>(ERTActionInvalidReason::Stunned);
	}

	/** Le voci di rifiuto per stordimento di UNA unita': e' il predicato di `Status.StunDeniedMainActionPaysItsCooldown`. */
	int32 CoverStunCountRefusals(const ARTTurnManager* TM, int32 UnitId)
	{
		int32 N = 0;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (CoverStunIsRefusal(E) && E.UnitId == UnitId)
			{
				++N;
			}
		}
		return N;
	}

	/** Gli eventi `AbilityActivated` che hanno quell'unita' per sorgente. */
	int32 CoverStunCountActivations(const ARTTurnManager* TM, int32 UnitId)
	{
		int32 N = 0;
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::AbilityActivated && Ev.SourceStableUnitId == UnitId) { ++N; }
		}
		return N;
	}

	/** Un'azione del kit di Branth, cercata per ActionId e non per indice: l'indice scivola se un'azione core manca (vedi `MakeBranth`). */
	URTActionData* CoverStunBranthAction(const TCHAR* ActionId)
	{
		URTHeroData* Branth = URTHeroCatalogLibrary::MakeBranth();
		if (!Branth) { return nullptr; }
		const TObjectPtr<URTActionData>* Found = Branth->Actions.FindByPredicate(
			[ActionId](const TObjectPtr<URTActionData>& Action) { return Action && Action->Def.ActionId == FName(ActionId); });
		return Found ? Found->Get() : nullptr;
	}

	/** Il residuo di OGNI slot di abilita' dell'unita': leggere solo quello sotto prova lascerebbe invisibile un addebito sullo slot sbagliato. */
	TArray<int32> CoverStunSlots(const ARTUnit* Unit)
	{
		TArray<int32> Slots;
		for (int32 I = 0; I < Unit->Abilities.Num(); ++I)
		{
			Slots.Add(Unit->GetAbilityCooldown(I));
		}
		return Slots;
	}

	FString CoverStunSlotsText(const TArray<int32>& Slots)
	{
		FString Text;
		for (int32 I = 0; I < Slots.Num(); ++I)
		{
			Text += FString::Printf(TEXT("%s%d"), I == 0 ? TEXT("") : TEXT(","), Slots[I]);
		}
		return Text;
	}

	/** Una rotta come testo, per il confronto e per il referto: le celle nell'ordine, separate da `>`. Vuota = `-`. */
	FString CoverStunRouteText(const TArray<FRTCellId>& Route)
	{
		if (Route.IsEmpty()) { return TEXT("-"); }
		FString Text;
		for (int32 I = 0; I < Route.Num(); ++I)
		{
			Text += FString::Printf(TEXT("%s%s"), I == 0 ? TEXT("") : TEXT(">"), *Route[I].ToString());
		}
		return Text;
	}

	/** Quanti slot DIVERSI da `Except` hanno una ricarica in corso: a inizio turno devono essere zero, e a fine turno pure. */
	int32 CoverStunBusyOtherSlots(const TArray<int32>& Slots, int32 Except)
	{
		int32 Busy = 0;
		for (int32 I = 0; I < Slots.Num(); ++I)
		{
			if (I != Except && Slots[I] != 0) { ++Busy; }
		}
		return Busy;
	}

	const TCHAR* CoverStunKindName(ECoverStunKind Kind)
	{
		switch (Kind)
		{
		case ECoverStunKind::CreateCover:  return TEXT("CreateCover");
		case ECoverStunKind::KineticPanel: return TEXT("KineticPanel");
		case ECoverStunKind::Reconfigure:  return TEXT("Reconfigure");
		case ECoverStunKind::PortableCover: return TEXT("PortableCover");
		}
		return TEXT("?");
	}

	FString CoverStunRunLabel(const FCoverStunScene& Scene, bool bStunned)
	{
		FString Label = CoverStunKindName(Scene.Kind);
		if (Scene.bAlreadyCooling) { Label += TEXT(", gia' in ricarica"); }
		if (Scene.bNoEdge) { Label += TEXT(", senza bordo"); }
		if (Scene.bNoTarget) { Label += TEXT(", senza bersaglio"); }
		if (Scene.bOutOfRange) { Label += TEXT(", fuori portata"); }
		if (Scene.bFallen) { Label += TEXT(", attore caduto"); }
		if (Scene.bWithWitness) { Label += TEXT(", con testimone"); }
		if (Scene.bWithEarlyWitness) { Label += TEXT(", con testimone anticipato"); }
		if (Scene.bWithStep) { Label += TEXT(", con passo"); }
		if (Scene.bWithDetour) { Label += TEXT(", con deviazione"); }
		if (Scene.bDestinationOnly) { Label += TEXT(", sola destinazione"); }
		return FString::Printf(TEXT("%s, %s"), *Label, bStunned ? TEXT("stordito") : TEXT("controllo"));
	}

	/** Il mondo vivo di una run: abbastanza per giocare piu' turni sullo stesso attore. */
	struct FCoverStunBed
	{
		UWorld* World = nullptr;
		URTHexMapAsset* Map = nullptr;
		ARTUnit* Caster = nullptr;
		ARTUnit* Witness = nullptr;                    //< solo con testimone: un'unita' viva della stessa squadra, ordinata DOPO l'attore, che erige altrove
		ARTTurnManager* TM = nullptr;
		FRTCellId Home = FRTCellId(0, 0);
		FRTCellId Shielded = FRTCellId(1, 0);          //< la cella che riceve la struttura, adiacente a Home
		FRTCellId Step = FRTCellId(-1, 0);             //< il passo normale dell'attore: adiacente a Home, dalla parte opposta a Shielded, libera
		FRTCellId WitnessHome = FRTCellId(0, 2);
		FRTCellId WitnessTarget = FRTCellId(0, 3);
		ARTUnit* EarlyWitness = nullptr;               //< solo con testimone anticipato: un'unita' viva della stessa squadra, ordinata PRIMA dell'attore (`StableLess` ordina per livello, X, Y: X minore), che erige altrove
		FRTCellId EarlyHome = FRTCellId(-2, 2);        //< a distanza 2 da Home e da Step e 4 dal nemico: non e' adiacente ne' sul percorso dell'attore
		FRTCellId EarlyTarget = FRTCellId(-2, 3);
		FRTCellId Detour = FRTCellId(0, -1);           //< la cella intermedia del percorso composito: adiacente sia a Home sia a Step, e libera
		ARTUnit* Opener = nullptr;                     //< solo con un passo: un compagno NON stordito che cammina sempre, in entrambi i bracci (D-569: il passo di una stordita e' negato, e il playback si apre per lui)
		FRTCellId OpenerHome = FRTCellId(2, -3);       //< lontano da ogni altra cella della scena (a distanza 3 da Home), sulla riga r=-3
		FRTCellId OpenerStep = FRTCellId(3, -3);       //< una cella a est: libera
	};

	void CoverStunCloseBed(FCoverStunBed& Bed)
	{
		DestroyEnvWorld(Bed.World);
		Bed.World = nullptr;
	}

	/** Costruisce il mondo: mappa piatta, l'attore, un avversario lontano (e i testimoni, se servono). Falso, e rosso, se qualcosa manca. */
	bool CoverStunOpenBed(FAutomationTestBase& Test, const FString& Run, bool bWithWitness, bool bWithEarlyWitness, FCoverStunBed& Bed,
		bool bWithOpener = false)
	{
		Bed.World = MakeEnvWorld();
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: world di prova"), *Run), Bed.World)) { return false; }
		ARTHexMapActor* MapActor = SpawnEnvMap(Bed.World);
		Bed.Map = MapActor ? MapActor->MapAsset : nullptr;

		Bed.Caster = SpawnEnvUnit(Bed.World, 0, Bed.Home);
		// Un avversario inerte e lontano: senza nemici la partita finirebbe al primo turno (`EvaluateOutcome`).
		ARTUnit* Foe = SpawnEnvUnit(Bed.World, 1, FRTCellId(-4, 0));
		// Il testimone delle righe «con testimone» e «caduto»: la sua copertura prova che il Prep e' girato davvero, con una
		// stordita o un cadavere davanti a lui nella lista.
		if (bWithWitness) { Bed.Witness = SpawnEnvUnit(Bed.World, 0, Bed.WitnessHome); }
		// Il testimone ANTICIPATO: sta prima dell'attore nella lista di `ResolveCoverStructures`, quindi la sua operazione e' gia'
		// raccolta (in `Pending`) quando la guardia scatta. E' l'unico a cui un rifiuto che buttasse la raccolta farebbe del male.
		if (bWithEarlyWitness) { Bed.EarlyWitness = SpawnEnvUnit(Bed.World, 0, Bed.EarlyHome); }
		// L'apritore del playback: cammina di una cella in entrambi i bracci. Da D-569 il passo dell'attore stordito e' negato, e
		// senza un fatto di timeline il playback del braccio stordito non si aprirebbe: "niente beat di Prep" sarebbe vero per il
		// motivo banale. L'apritore e' il fatto, e non e' stordito.
		if (bWithOpener) { Bed.Opener = SpawnEnvUnit(Bed.World, 0, Bed.OpenerHome); }
		Bed.TM = Bed.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: mappa"), *Run), Bed.Map)
			|| !Test.TestNotNull(*FString::Printf(TEXT("%s: Caster"), *Run), Bed.Caster)
			|| !Test.TestNotNull(*FString::Printf(TEXT("%s: Foe"), *Run), Foe)
			|| !Test.TestNotNull(*FString::Printf(TEXT("%s: TM"), *Run), Bed.TM)
			|| (bWithWitness && !Test.TestNotNull(*FString::Printf(TEXT("%s: testimone"), *Run), Bed.Witness))
			|| (bWithEarlyWitness && !Test.TestNotNull(*FString::Printf(TEXT("%s: testimone anticipato"), *Run), Bed.EarlyWitness))
			|| (bWithOpener && !Test.TestNotNull(*FString::Printf(TEXT("%s: apritore"), *Run), Bed.Opener)))
		{
			CoverStunCloseBed(Bed);
			return false;
		}
		return true;
	}

	/**
	 * Pianifica nello slot dell'attore un'azione del kit di Branth su una cella e un bordo; falso, e rosso, se il kit non l'ha.
	 *
	 * `FreeRotations` (se non `INDEX_NONE`) riscrive il credito di rotazione gratuita che la VARIANTE dichiara, sulla copia propria
	 * dei dati dell'azione: `MakeBranth` ne costruisce una nuova a ogni chiamata, quindi il catalogo non si tocca. E' un valore che
	 * il catalogo potrebbe dichiarare domani, non uno che il motore non produce.
	 */
	bool CoverStunPlanBranth(FAutomationTestBase& Test, const FString& Run, ARTUnit* Caster, const TCHAR* ActionId,
		const FRTCellId& TargetCell, ERTHexDirection Edge, const FName& VariantId = NAME_None, int32 FreeRotations = INDEX_NONE)
	{
		URTActionData* Action = CoverStunBranthAction(ActionId);
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: %s nel kit di Branth"), *Run, ActionId), Action)) { return false; }
		if (FreeRotations != INDEX_NONE)
		{
			FRTAbilityVariant* Variant = Action->Variants.FindByPredicate(
				[&VariantId](const FRTAbilityVariant& Candidate) { return Candidate.VariantId == VariantId; });
			if (!Test.TestNotNull(*FString::Printf(TEXT("%s: %s dichiara la variante %s"), *Run, ActionId, *VariantId.ToString()), Variant))
			{
				return false;
			}
			Variant->Parameters.Add(TEXT("FreeRotations"), FreeRotations);
		}
		PlanHeroCoverAction(Caster, Action, TargetCell, Edge, VariantId);
		return true;
	}

	/**
	 * Pianifica nello slot dell'attore l'azione che il GADGET `Gadget.PortableCover` concede, costruita come la costruisce il gioco
	 * (`MakeEquipmentAction`). Ha il nome del pezzo, la ricarica del pezzo (non quella del pannello d'eroe) e la `StructureOp` di
	 * `Action.CreateCover`, che COPIA: e' un'altra azione che dichiara `StructureOp`, e il nome non e' fra quelli di `Action.CreateCover`,
	 * `Hero.Branth.KineticPanel` e `Hero.Branth.Reconfigure`.
	 * Falso, e rosso, se il pezzo non concede un'azione o se l'azione non e' quella che la riga dice.
	 */
	bool CoverStunPlanPortableCover(FAutomationTestBase& Test, const FString& Run, ARTUnit* Caster, const FRTCellId& TargetCell,
		ERTHexDirection Edge)
	{
		const URTEquipmentData* Gadget = URTCatalogLibrary::MakePortableCoverGadget();
		URTActionData* Action = Gadget ? URTCatalogLibrary::MakeEquipmentAction(Gadget, Caster) : nullptr;
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: Gadget.PortableCover concede un'azione"), *Run), Action)) { return false; }
		if (!Test.TestTrue(*FString::Printf(TEXT("%s: premessa: l'azione del gadget ha il nome del pezzo e dichiara la StructureOp CreateCover"), *Run),
				Action->Def.ActionId == FName(TEXT("Gadget.PortableCover")) && Action->Def.StructureOp == ERTStructureOp::CreateCover))
		{
			return false;
		}
		PlanHeroCoverAction(Caster, Action, TargetCell, Edge);
		return true;
	}

	/**
	 * `RunEnvTurn`, ma ricorda quali fasi il playback MOSTRA: la fase si legge subito dopo il lock-in e dopo ogni tick.
	 * Fuori dal playback `CurrentPlaybackPhaseForTest()` vale `Planning`, e `PlaybackPhases` non contiene mai `Cleanup`.
	 *
	 * ⚠️ Un turno senza NESSUN evento in timeline non apre il playback (`ConcludeResolution` va a `ConcludeTurn`): «niente
	 * Prep» sarebbe vero per il motivo banale, anche con un contatore sbagliato. Chi lo asserisce deve avere altrove un
	 * fatto che il playback mostri — qui, il passo — e dichiararlo fra le premesse.
	 */
	void CoverStunRunTurnWatchingPlayback(ARTTurnManager* TM, bool& bOutSawPrep, bool& bOutSawMove)
	{
		auto Watch = [TM, &bOutSawPrep, &bOutSawMove]()
		{
			const ERTMatchPhase Shown = TM->CurrentPlaybackPhaseForTest();
			bOutSawPrep |= Shown == ERTMatchPhase::Prep;
			bOutSawMove |= Shown == ERTMatchPhase::Move;
		};
		TM->LockInAndResolve();
		Watch();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
			Watch();
		}
	}

	/**
	 * UN turno risolto sull'attore del banco, con tutto cio' che lo misura PRIMA e DOPO.
	 *
	 * Il braccio STORDITO e quello di CONTROLLO differiscono per UNA riga — `ApplyStatus(Stunned)` subito prima del
	 * commit, dopo che il piano e' scritto — e le premesse lo verificano invece di darlo per scontato. ⚠️ Le premesse
	 * valgono per costruzione: nessuna legge un esito che il turno decide.
	 *
	 * L'attore caduto e' distrutto a fine turno (`DestroyDefeatedUnits`) ma resta leggibile finche' non gira il garbage
	 * collector: per questo lo si rilegge da un riferimento debole, come fa `PagaGiraStordito`.
	 */
	FCoverStunOutcome CoverStunPlayTurn(FAutomationTestBase& Test, const FString& Run, const FCoverStunBed& Bed,
		bool bStunned, bool bAlreadyCooling, bool bFallen, bool bExpectsStep = false)
	{
		FCoverStunOutcome Out;
		ARTUnit* Caster = Bed.Caster;

		const URTActionData* Installed = Caster->GetAbility(CoverStunSlot);
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: premessa: l'azione e' nello slot %d"), *Run, CoverStunSlot), Installed))
		{
			return Out;
		}
		Out.ActionId = Installed->Def.ActionId;
		Out.Home = Caster->Cell;
		Out.Aimed = Caster->ResolutionAimCell(Installed->Def.DeclaresTracking());
		Out.bMainInPrep = Installed->Def.Slot == ERTActionSlot::Main
			&& URTCatalogLibrary::MapResolutionPhase(Installed->Def.ResolutionPhase) == ERTMatchPhase::Prep;
		Out.DefCooldown = Installed->Def.CooldownTurns;
		Out.MirrorCooldown = Installed->CooldownTurns;

		// «Gia' in ricarica»: lo stesso preludio di `PagaGiraStordito`. `ConsumeAbility` scrive un valore FISSO, quindi un
		// secondo addebito riporterebbe lo slot a `Mirror` e il residuo a fine turno lo direbbe.
		if (bAlreadyCooling)
		{
			Caster->ConsumeAbility(CoverStunSlot);
			Caster->TickCooldowns();
		}
		Out.CooldownBefore = Caster->GetAbilityCooldown(CoverStunSlot);
		Out.SlotsBefore = CoverStunSlots(Caster);
		Out.WBefore = CoverIntegrityOn(Bed.Map, Bed.Shielded, ERTHexDirection::W);
		Out.EBefore = CoverIntegrityOn(Bed.Map, Bed.Shielded, ERTHexDirection::E);

		// L'UNICA riga che separa i due bracci, DOPO il piano: l'ordine e' quello di `PagaGiraStordito`.
		if (bStunned) { Caster->ApplyStatus(TAG_Status_Stunned, URTCombatLibrary::StunnedDurationTurns); }
		// Caduto DOPO lo stordimento, come `PagaGiraStordito`: un cadavere stordito e' il caso che la riga vuole.
		if (bFallen) { Caster->Health = 0; }
		Out.bStunnedAtCommit = Caster->HasStatus(TAG_Status_Stunned);
		Out.bPlanAtCommit = Caster->PlannedAbilityIndex == CoverStunSlot;
		// Il passo normale, se il piano ne ha uno: dove deve arrivare si legge dal piano scritto, non dal banco.
		Out.bMoveAtCommit = Caster->HasPlannedNormalMove();
		if (Out.bMoveAtCommit)
		{
			Out.Destination = Caster->PlannedCell;
			// La rotta che il piano promette: il suo percorso se ne ha uno, altrimenti la sola destinazione, che il pathfinder
			// raggiunge dalla partenza (che sia ADIACENTE, e quindi che la rotta sia unica, lo verifica una premessa).
			Out.PlannedRoute = Caster->PlannedPath;
			Out.ExpectedRoute = Out.PlannedRoute.Num() >= 2 ? Out.PlannedRoute : TArray<FRTCellId>{ Out.Home, Out.Destination };
		}

		bool bPremises = true;
		bPremises &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: lo stordimento c'e' solo dove il turno lo dichiara"), *Run),
			Out.bStunnedAtCommit, bStunned);
		bPremises &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: il piano e' nello slot %d al commit"), *Run, CoverStunSlot),
			Out.bPlanAtCommit);
		bPremises &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: e' un'azione PRINCIPALE di fase Prep"), *Run),
			Out.bMainInPrep);
		if (bAlreadyCooling)
		{
			bPremises &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: l'azione parte in ricarica (specchio - 1)"), *Run),
				Out.CooldownBefore, Out.MirrorCooldown - 1);
			bPremises &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: `CanUseAbility` e' falso, il piano si scarta in silenzio"), *Run),
				Out.CooldownBefore > 0 && !Caster->CanUseAbility(CoverStunSlot));
		}
		else
		{
			bPremises &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: l'azione e' utilizzabile (nessuna ricarica residua)"), *Run),
				Out.CooldownBefore, 0);
		}
		bPremises &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: ricarica dichiarata e specchio coincidono e sono positivi"), *Run),
			Out.DefCooldown > 0 && Out.DefCooldown == Out.MirrorCooldown);
		bPremises &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: nessun altro slot e' in ricarica prima del turno"), *Run),
			CoverStunBusyOtherSlots(Out.SlotsBefore, CoverStunSlot), 0);
		bPremises &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: l'attore e' caduto solo dove il turno lo dichiara"), *Run),
			!Caster->IsAlive(), bFallen);
		// Gli asserti del passo stanno dentro `if (Out.bMoveAtCommit)`: una riga che dichiara un passo e lo scrive male (o non lo
		// scrive) li salterebbe TUTTI in silenzio e resterebbe verde. Questa premessa la rende rossa.
		bPremises &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: il piano ha un movimento normale solo dove la riga lo dichiara"), *Run),
			Out.bMoveAtCommit, bExpectsStep);
		if (Out.bMoveAtCommit)
		{
			// Chi cammina non e' radicato: il braccio di CONTROLLO deve poter camminare, altrimenti lo zero dello stordito non direbbe
			// niente. (⌫ Fino a D-569 questa premessa difendeva "Root e Stunned sono due stati": [D-416] (1) lasciava il movimento a Root.
			// Ora la stordita non cammina comunque, e la premessa serve solo al controllo.)
			bPremises &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: il passo e' verso una cella ADIACENTE"), *Run),
				URTHexLibrary::HexDistance(Caster->Cell, Out.Destination), 1);
			bPremises &= Test.TestFalse(*FString::Printf(TEXT("%s: premessa: l'attore non e' radicato"), *Run),
				Caster->HasStatus(TAG_Status_Root));
			if (Out.PlannedRoute.Num() >= 2)
			{
				// Un percorso scritto e' ANCORATO alla cella corrente e finisce sulla destinazione: e' la forma che il resolver
				// accetta (`PlannedPath[0] == Cell`) e che il controller scrive dopo un click.
				bPremises &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: il percorso pianificato parte dalla cella dell'attore e finisce sulla destinazione"), *Run),
					Out.PlannedRoute[0] == Caster->Cell && Out.PlannedRoute.Last() == Out.Destination);
			}
		}
		if (Bed.Witness)
		{
			// `ResolveCoverStructures` riceve le unita' nell'ordine di `SortUnitsForResolution`: se la regola cambiasse e il
			// testimone passasse PRIMA dell'attore, un `break` al posto del `continue` non gli toglierebbe niente, e la riga
			// non distinguerebbe piu' i due.
			TArray<ARTUnit*> Order = { Bed.Witness, Caster };
			URTActionQueueLibrary::SortUnitsForResolution(Order);
			bPremises &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: il testimone e' ordinato DOPO l'attore da SortUnitsForResolution"), *Run),
				Order[0] == Caster);
		}
		if (Bed.EarlyWitness)
		{
			// L'opposto del testimone: se la regola lo mettesse DOPO l'attore, la sua operazione non sarebbe ancora raccolta
			// quando la guardia scatta, e un rifiuto che buttasse la raccolta (`Pending.Reset()`) non gli toglierebbe niente. L'array
			// parte nell'ordine SBAGLIATO per la regola, cosi' il sort deve spostare qualcosa perche' la premessa sia vera.
			TArray<ARTUnit*> Order = { Caster, Bed.EarlyWitness };
			URTActionQueueLibrary::SortUnitsForResolution(Order);
			bPremises &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: il testimone anticipato e' ordinato PRIMA dell'attore da SortUnitsForResolution"), *Run),
				Order[0] == Bed.EarlyWitness);
		}

		const TWeakObjectPtr<ARTUnit> CasterWeak(Caster);
		CoverStunRunTurnWatchingPlayback(Bed.TM, Out.bSawPrepBeat, Out.bSawMoveBeat);

		ARTUnit* After = CasterWeak.Get(/*bEvenIfPendingKill=*/ true);
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: l'attore e' ancora leggibile"), *Run), After))
		{
			return Out;
		}
		Out.StableId = After->StableUnitId;
		Out.FinalCell = After->Cell;
		Out.CooldownAfter = After->GetAbilityCooldown(CoverStunSlot);
		Out.SlotsAfter = CoverStunSlots(After);
		Out.WAfter = CoverIntegrityOn(Bed.Map, Bed.Shielded, ERTHexDirection::W);
		Out.EAfter = CoverIntegrityOn(Bed.Map, Bed.Shielded, ERTHexDirection::E);
		if (const FRTHexCellData* TargetCell = Bed.Map->FindCell(Bed.Shielded))
		{
			for (const FRTHexCover& Cover : TargetCell->Covers)
			{
				if (Cover.Type != ERTHexCoverType::None) { ++Out.CoversAfter; }
			}
		}
		Out.Created = CountEnvOutcome(Bed.TM, ERTEnvironmentOutcome::CoverCreated);
		Out.Moved = CountEnvOutcome(Bed.TM, ERTEnvironmentOutcome::CoverMoved);
		Out.Rejected = CountEnvOutcome(Bed.TM, ERTEnvironmentOutcome::CoverRejected);
		for (const FRTTurnLogEntry& E : Bed.TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Fallback) { ++Out.FallbacksAnyUnit; }
			if (CoverStunIsRefusal(E))
			{
				++Out.RefusalsAnyUnit;
				if (E.Phase == ERTMatchPhase::Prep) { ++Out.PrepRefusalsAnyUnit; }
			}
		}
		bPremises &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: l'attore ha uno StableUnitId"), *Run), Out.StableId != 0);
		// Vedi `CoverStunSlot`: un pagamento scritto con l'identita' al posto dell'indice si vede solo se i due numeri differiscono.
		bPremises &= Test.TestNotEqual(*FString::Printf(TEXT("%s: premessa: lo StableUnitId dell'attore non e' lo slot dell'azione"), *Run),
			Out.StableId, CoverStunSlot);
		if (Bed.Witness)
		{
			// `CoverCreated` porta l'unita' che ha eretto: qui, quella del testimone e nessun'altra.
			const int32 WitnessId = Bed.Witness->StableUnitId;
			bPremises &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: il testimone ha uno StableUnitId, e non e' quello dell'attore"), *Run),
				WitnessId != 0 && WitnessId != Out.StableId);
			for (const FRTTurnLogEntry& E : Bed.TM->GetTurnLog())
			{
				if (E.UnitId == WitnessId && E.Category == ERTLogCategory::Environment
					&& E.Outcome == static_cast<uint8>(ERTEnvironmentOutcome::CoverCreated))
				{
					++Out.WitnessCreated;
				}
			}
		}
		if (Bed.EarlyWitness)
		{
			// Lo stesso conto per il testimone anticipato: la sua voce `CoverCreated` e' di UN'unita' sola, e non e' quella dell'attore.
			const int32 EarlyId = Bed.EarlyWitness->StableUnitId;
			bPremises &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: il testimone anticipato ha uno StableUnitId, e non e' quello dell'attore"), *Run),
				EarlyId != 0 && EarlyId != Out.StableId);
			for (const FRTTurnLogEntry& E : Bed.TM->GetTurnLog())
			{
				if (E.UnitId == EarlyId && E.Category == ERTLogCategory::Environment
					&& E.Outcome == static_cast<uint8>(ERTEnvironmentOutcome::CoverCreated))
				{
					++Out.EarlyCreated;
				}
			}
		}
		if (Out.StableId != 0)
		{
			// La rotta che l'attore ha PERCORSO: l'evento `Move` di fase Move che ha lui per sorgente, con la rotta in celle (partenza
			// inclusa). E' un osservabile che distingue un percorso composito dal percorso che il pathfinder avrebbe calcolato
			// da solo verso la stessa destinazione: la cella finale, a fine turno, e' la stessa.
			for (const FRTResolvedEvent& Ev : Bed.TM->ResolvedTimelineForTest())
			{
				if (Ev.Type == ERTResolvedEventType::Move && Ev.Phase == ERTMatchPhase::Move && Ev.SourceStableUnitId == Out.StableId)
				{
					Out.WalkedRoute = Ev.Path;
					break;
				}
			}
			Out.Activations = CoverStunCountActivations(Bed.TM, Out.StableId);
			Out.StunRefusals = CoverStunCountRefusals(Bed.TM, Out.StableId);
			bool bFirstRefusal = true;
			for (const FRTTurnLogEntry& E : Bed.TM->GetTurnLog())
			{
				if (E.UnitId != Out.StableId) { continue; }
				++Out.EntriesOfActor;
				if (E.Category == ERTLogCategory::Fallback) { ++Out.AnyFallbacks; }
				if (E.Category == ERTLogCategory::Move && E.Phase == ERTMatchPhase::Move) { Out.ActorMoveOutcome = E.Outcome; }
				// D-569: il passo di una stordita e' negato con una voce di rifiuto in fase MOVE, accanto a quella di Prep. Questo
				// banco decide la voce di Prep (la struttura di bordo), quindi il "primo rifiuto" e' il primo di Prep; quella di Move ha
				// i suoi campi.
				if (CoverStunIsRefusal(E) && E.Phase == ERTMatchPhase::Move)
				{
					++Out.MoveRefusals;
					Out.MoveRefusalSrc = E.SrcCell;
					Out.MoveRefusalTgt = E.TgtCell;
				}
				if (bFirstRefusal && CoverStunIsRefusal(E) && E.Phase == ERTMatchPhase::Prep)
				{
					bFirstRefusal = false;
					Out.RefusalPhase = E.Phase;
					Out.RefusalActionId = E.ActionId;
					Out.RefusalSrc = E.SrcCell;
					Out.RefusalTgt = E.TgtCell;
				}
			}
		}

		Out.bValid = bPremises;
		return Out;
	}

	/**
	 * UNA run del banco a turno singolo: un mondo costruito da capo, un attore con il suo piano, un turno misurato.
	 *
	 * Il piano e' scritto PRIMA dello stordimento. `bNoEdge` lo rende incompleto (nessun bordo), `bFallen` fa cadere
	 * l'attore, `bWithWitness` aggiunge un compagno che erige altrove, `bWithStep` un passo normale verso una cella
	 * adiacente, `bAlreadyCooling` mette lo slot in ricarica: tutto dentro il turno misurato.
	 */
	FCoverStunOutcome CoverStunRun(FAutomationTestBase& Test, const FCoverStunScene& Scene, bool bStunned)
	{
		FCoverStunOutcome Out;
		const FString Run = CoverStunRunLabel(Scene, bStunned);

		FCoverStunBed Bed;
		const bool bAnyStep = Scene.bWithStep || Scene.bWithDetour || Scene.bDestinationOnly;
		if (!CoverStunOpenBed(Test, Run, Scene.bWithWitness, Scene.bWithEarlyWitness, Bed, /*bWithOpener=*/ bAnyStep)) { return Out; }

		bool bPlanned = false;
		switch (Scene.Kind)
		{
		case ECoverStunKind::CreateCover:
			PlanCoverAction(Bed.Caster, TEXT("Action.CreateCover"), Bed.Shielded, ERTHexDirection::W);
			bPlanned = true;
			break;

		case ECoverStunKind::PortableCover:
			bPlanned = CoverStunPlanPortableCover(Test, Run, Bed.Caster, Bed.Shielded, ERTHexDirection::W);
			break;

		case ECoverStunKind::KineticPanel:
			bPlanned = CoverStunPlanBranth(Test, Run, Bed.Caster, TEXT("Hero.Branth.KineticPanel"), Bed.Shielded, ERTHexDirection::W);
			break;

		case ECoverStunKind::Reconfigure:
			// Una copertura gia' in campo, ammaccata (18 invece di 30) e NON tracciata da `DynamicCovers`: nessuna
			// rotazione gratuita, quindi lo spostamento si paga.
			if (Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la copertura di mappa e' posta"), *Run),
					URTHexCoverLibrary::AddCover(Bed.Map, Bed.Shielded, ERTHexDirection::W, ERTHexCoverType::Low, 18)))
			{
				bPlanned = CoverStunPlanBranth(Test, Run, Bed.Caster, TEXT("Hero.Branth.Reconfigure"), Bed.Shielded, ERTHexDirection::E);
			}
			break;
		}
		if (!bPlanned)
		{
			CoverStunCloseBed(Bed);
			return Out;
		}

		// Piani incompleti o fuori portata: i casi che il ciclo scarta con un `Reject` di raccolta, nell'ordine del codice.
		// Senza bordo: il bersaglio c'e', il bordo no.
		if (Scene.bNoEdge) { Bed.Caster->bHasPlannedCoverEdge = false; }
		// Senza bersaglio: ne' unita' ne' cella. La mira ricade sulla cella di chi agisce.
		if (Scene.bNoTarget)
		{
			Bed.Caster->bAttackTargetsCell = false;
			Bed.Caster->PlannedAttackTarget = nullptr;
		}
		// Fuori portata: `Action.CreateCover` ha portata 3, e la cella (4,0) sta nella mappa a distanza 4.
		if (Scene.bOutOfRange) { Bed.Caster->PlannedAttackCell = FRTCellId(4, 0); }
		// Il testimone erige altrove, nello stesso Prep: se la sua copertura c'e', `ResolveCoverStructures` e' girata.
		if (Scene.bWithWitness) { PlanCoverAction(Bed.Witness, TEXT("Action.CreateCover"), Bed.WitnessTarget, ERTHexDirection::W); }
		// Il testimone anticipato erige altrove, nello stesso Prep, ed e' ordinato PRIMA dell'attore: la sua operazione e' gia'
		// raccolta quando la guardia della stordita scatta.
		if (Scene.bWithEarlyWitness) { PlanCoverAction(Bed.EarlyWitness, TEXT("Action.CreateCover"), Bed.EarlyTarget, ERTHexDirection::W); }
		// Il passo normale: le due scritture finali del controller dopo un click, `PlannedPath` ANCORATO alla cella corrente
		// e `PlannedCell`. `PlannedWaypoints` non si scrive, perche' il resolver non lo legge. L'adiacenza la verifica una
		// premessa; che `Step` sia libera lo prova il braccio di controllo, dove si cammina.
		if (Scene.bWithStep)
		{
			Bed.Caster->PlannedPath = { Bed.Caster->Cell, Bed.Step };
			Bed.Caster->PlannedCell = Bed.Step;
		}
		// La stessa destinazione, ma con un percorso COMPOSITO: una cella intermedia che il pathfinder non sceglierebbe (il percorso
		// diretto e' un passo, questo ne fa uno in piu'). E' la forma di un giocatore che ha posato un waypoint. Che il percorso sia
		// piu' lungo della geodetica lo dice la premessa qui sotto; che il resolver lo percorra, il braccio di controllo.
		if (Scene.bWithDetour)
		{
			Bed.Caster->PlannedPath = { Bed.Caster->Cell, Bed.Detour, Bed.Step };
			Bed.Caster->PlannedCell = Bed.Step;
			if (!Test.TestTrue(*FString::Printf(TEXT("%s: premessa: il percorso pianificato passa da una cella intermedia ed e' piu' lungo della distanza fra partenza e destinazione"), *Run),
					URTHexLibrary::HexDistance(Bed.Caster->Cell, Bed.Detour) == 1 && URTHexLibrary::HexDistance(Bed.Detour, Bed.Step) == 1
					&& Bed.Caster->PlannedPath.Num() - 1 > URTHexLibrary::HexDistance(Bed.Caster->Cell, Bed.Step)))
			{
				CoverStunCloseBed(Bed);
				return Out;
			}
		}
		// Il piano di SOLA DESTINAZIONE: nessun percorso, lo trova il pathfinder della fase Move. E' la forma dei bot (non scrivono
		// mai `PlannedPath`: dichiarano la sola `PlannedCell`).
		if (Scene.bDestinationOnly)
		{
			Bed.Caster->PlannedPath.Reset();
			Bed.Caster->PlannedCell = Bed.Step;
			if (!Test.TestEqual(*FString::Printf(TEXT("%s: premessa: il piano e' di sola destinazione (nessun percorso scritto)"), *Run),
					Bed.Caster->PlannedPath.Num(), 0))
			{
				CoverStunCloseBed(Bed);
				return Out;
			}
		}

		// L'apritore cammina di una cella, sempre: e' il fatto di timeline che apre il playback anche quando il passo dell'attore
		// stordito e' negato (D-569). La sua cella di arrivo si legge dopo il turno.
		if (Bed.Opener)
		{
			Bed.Opener->PlannedPath = { Bed.Opener->Cell, Bed.OpenerStep };
			Bed.Opener->PlannedCell = Bed.OpenerStep;
		}

		Out = CoverStunPlayTurn(Test, Run, Bed, bStunned, Scene.bAlreadyCooling, Scene.bFallen, /*bExpectsStep=*/ bAnyStep);
		if (Bed.Opener)
		{
			Out.bOpener = true;
			Out.OpenerPlanned = Bed.OpenerStep;
			Out.OpenerFinal = Bed.Opener->Cell;
		}
		if (Scene.bWithWitness) { Out.WitnessWAfter = CoverIntegrityOn(Bed.Map, Bed.WitnessTarget, ERTHexDirection::W); }
		if (Scene.bWithEarlyWitness) { Out.EarlyWAfter = CoverIntegrityOn(Bed.Map, Bed.EarlyTarget, ERTHexDirection::W); }
		CoverStunCloseBed(Bed);
		return Out;
	}

	/** I turni misurati del flusso della rotazione gratuita, e lo stato in mezzo. */
	struct FCoverStunRotation
	{
		bool bValid = false;
		FString Label;
		FCoverStunOutcome Turn3;         //< stordito: il turno del rifiuto; controllo: la rotazione gratuita
		bool bStunnedAfterTurn3 = false; //< premessa del turno 4: lo stordimento (durata 2) sopravvive al Cleanup del proprio turno
		bool bStunnedAfterTurn4 = false; //< premessa del turno 5: lo stordimento e' scaduto nel Cleanup del turno vuoto
		int32 CooldownAfterTurn4 = -1;   //< premessa del turno 5: ... e la ricarica e' a zero
		FCoverStunOutcome Turn5;         //< stordito: la rotazione gratuita sopravvissuta; controllo: lo spostamento di ritorno, che si paga
		FCoverStunOutcome Turn6;         //< solo stordito: la rotazione di ritorno, subito dopo la gratuita del quinto turno; paga se il credito era UNO, e non se erano due
	};

	/**
	 * I turni sul pannello ADATTIVO, fino a T6 nel solo braccio stordito. T1 erige (non stordito, in tutti i bracci). T2 e'
	 * VUOTO: la ricarica dello slot, che `Reconfigure` condivide col pannello, scende a zero — prima, un piano di `Reconfigure`
	 * verrebbe scartato in silenzio da `CanUseAbility` (come in `Heroes.Branth.ReconfigureRefusesInsteadOfGuessing`). T3
	 * pianifica `Reconfigure` da W a E, e nel braccio stordito lo stordimento e' applicato dopo il piano. T4 e' VUOTO: lo
	 * stordimento (durata `StunnedDurationTurns`) sopravvive al Cleanup di T3 e scade in quello di T4, e la ricarica scende a
	 * zero. T5 e' NON stordito: lo stordito ruota di nuovo la STESSA copertura da W a E; il controllo, che l'ha gia' ruotata al
	 * T3, la riporta da E a W. T6, solo nel braccio stordito, la riporta da E a W: la rotazione gratuita del T5 ha usato l'unico
	 * credito, quindi la ricarica e' ancora a zero e la rotazione PAGA.
	 *
	 * `bTwoCredits` e' il controllo positivo di T6: la variante dichiara DUE rotazioni gratuite invece di una (sulla copia propria
	 * dei dati, vedi `CoverStunPlanBranth`), e T6 e' gratuito. E' lo stesso flusso, con l'unico numero che un rifiuto che regalasse
	 * un credito produrrebbe.
	 */
	FCoverStunRotation CoverStunRunRotation(FAutomationTestBase& Test, bool bStunned, bool bTwoCredits = false)
	{
		FCoverStunRotation Out;
		Out.Label = FString::Printf(TEXT("Reconfigure (rotazione gratuita), %s%s"),
			bStunned ? TEXT("stordito") : TEXT("controllo"), bTwoCredits ? TEXT(", due crediti") : TEXT(""));
		const FString& Label = Out.Label;
		const int32 DeclaredCredits = bTwoCredits ? 2 : 1;

		FCoverStunBed Bed;
		if (!CoverStunOpenBed(Test, Label, /*bWithWitness=*/ false, /*bWithEarlyWitness=*/ false, Bed)) { return Out; }
		auto Abort = [&]() { CoverStunCloseBed(Bed); return Out; };

		// Turno 1, NON stordito in nessuno dei bracci: il pannello ADATTIVO nasce tracciato da `DynamicCovers` con 25 punti
		// struttura, le rotazioni gratuite che la variante dichiara, e non scade da solo.
		if (!CoverStunPlanBranth(Test, Label, Bed.Caster, TEXT("Hero.Branth.KineticPanel"), Bed.Shielded, ERTHexDirection::W,
				TEXT("Hero.Branth.KineticPanel.Adaptive"), bTwoCredits ? DeclaredCredits : INDEX_NONE))
		{
			return Abort();
		}
		// Il credito e' una PREMESSA e non un'opinione: lo dice la variante attiva dell'azione installata, letta dalla stessa strada
		// del motore (`FindVariant` con la variante attiva dell'unita'). Se il catalogo cambiasse il numero, o l'id non fosse quello
		// attivo, la premessa lo direbbe prima dei turni e non con un T5 o un T6 rossi per una ragione che non si legge.
		const URTActionData* InstalledPanel = Bed.Caster->GetAbility(CoverStunSlot);
		const FRTAbilityVariant* DeclaredVariant = InstalledPanel ? InstalledPanel->FindVariant(Bed.Caster->ActiveVariantId) : nullptr;
		const int32* DeclaredFreeRotations = DeclaredVariant ? DeclaredVariant->Parameters.Find(TEXT("FreeRotations")) : nullptr;
		if (!Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la variante attiva dichiara %d rotazioni gratuite"), *Label, DeclaredCredits),
				DeclaredFreeRotations != nullptr && *DeclaredFreeRotations == DeclaredCredits))
		{
			return Abort();
		}
		RunEnvTurn(Bed.TM);
		if (!Test.TestEqual(*FString::Printf(TEXT("%s: premessa: il pannello adattivo e' eretto (25 punti struttura)"), *Label),
				CoverIntegrityOn(Bed.Map, Bed.Shielded, ERTHexDirection::W), 25))
		{
			return Abort();
		}

		// Turno 2, VUOTO: vedi sopra.
		RunEnvTurn(Bed.TM);
		if (!Test.TestEqual(*FString::Printf(TEXT("%s: premessa: il pannello adattivo non scade da solo"), *Label),
				CoverIntegrityOn(Bed.Map, Bed.Shielded, ERTHexDirection::W), 25))
		{
			return Abort();
		}

		// Turno 3, quello del rifiuto (o della rotazione gratuita): si ruota QUELLA copertura da W a E.
		if (!CoverStunPlanBranth(Test, Label, Bed.Caster, TEXT("Hero.Branth.Reconfigure"), Bed.Shielded, ERTHexDirection::E))
		{
			return Abort();
		}
		Out.Turn3 = CoverStunPlayTurn(Test, Label + TEXT(", turno 3"), Bed, bStunned, /*bAlreadyCooling=*/ false, /*bFallen=*/ false);
		Out.bStunnedAfterTurn3 = Bed.Caster->HasStatus(TAG_Status_Stunned);
		bool bPremises = true;
		bPremises &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: lo stordimento sopravvive al Cleanup del turno 3"), *Label),
			Out.bStunnedAfterTurn3, bStunned);

		// Turno 4, VUOTO: lo stordimento scade nel suo Cleanup e la ricarica arriva a zero, in entrambi i bracci.
		RunEnvTurn(Bed.TM);
		Out.bStunnedAfterTurn4 = Bed.Caster->HasStatus(TAG_Status_Stunned);
		Out.CooldownAfterTurn4 = Bed.Caster->GetAbilityCooldown(CoverStunSlot);
		bPremises &= Test.TestFalse(*FString::Printf(TEXT("%s: premessa: lo stordimento e' scaduto nel Cleanup del turno 4"), *Label),
			Out.bStunnedAfterTurn4);
		bPremises &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: la ricarica e' arrivata a zero a fine turno 4"), *Label),
			Out.CooldownAfterTurn4, 0);

		// Turno 5, NON stordito: lo stordito ruota di nuovo la STESSA copertura (W->E); il controllo la riporta da E a W.
		if (!CoverStunPlanBranth(Test, Label, Bed.Caster, TEXT("Hero.Branth.Reconfigure"), Bed.Shielded,
				bStunned ? ERTHexDirection::E : ERTHexDirection::W))
		{
			return Abort();
		}
		Out.Turn5 = CoverStunPlayTurn(Test, Label + TEXT(", turno 5"), Bed, /*bStunned=*/ false, /*bAlreadyCooling=*/ false, /*bFallen=*/ false);

		// Turno 6, solo nel braccio stordito, NON stordito, SUBITO dopo il quinto: la copertura sta in E (l'ha messa li' il quinto) e
		// la ricarica e' ferma a zero, perche' una rotazione gratuita non la fa partire (lo asserisce la premessa di T6, `CooldownBefore`).
		// Si riporta in W. Con il credito UNO, che il quinto ha speso, la rotazione PAGA; con due sarebbe ancora gratuita. Il controllo
		// non ha un T6: la sua rotazione del quinto turno ha pagato e la ricarica e' in corso. Ma il suo T5 e' lo stesso piano (E->W)
		// dallo stesso stato (credito zero, ricarica zero), e dice che cosa fa chi non e' stordito: paga e sposta.
		if (bStunned)
		{
			if (!CoverStunPlanBranth(Test, Label, Bed.Caster, TEXT("Hero.Branth.Reconfigure"), Bed.Shielded, ERTHexDirection::W))
			{
				return Abort();
			}
			Out.Turn6 = CoverStunPlayTurn(Test, Label + TEXT(", turno 6"), Bed, /*bStunned=*/ false, /*bAlreadyCooling=*/ false, /*bFallen=*/ false);
			bPremises &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la copertura sta in E (e non in W) prima del turno 6"), *Label),
				Out.Turn6.EBefore == 25 && Out.Turn6.WBefore == 0);
		}

		Out.bValid = bPremises && Out.Turn3.bValid && Out.Turn5.bValid && (!bStunned || Out.Turn6.bValid);
		CoverStunCloseBed(Bed);
		return Out;
	}

	/** La riga che il referto riporta per una run: TUTTI gli osservabili, anche quelli non asseriti. */
	FString CoverStunDescribe(const FString& Run, const FCoverStunOutcome& O)
	{
		return FString::Printf(
			TEXT("%s -> premesse: stordito %d, piano %d, principale-Prep %d, valida %d; azione %s; ")
			TEXT("bordo W %d>%d, bordo E %d>%d, coperture sulla cella %d; ")
			TEXT("CoverCreated %d, CoverMoved %d, CoverRejected %d, testimone W %d (CoverCreated %d); AbilityActivated %d; ")
			TEXT("rifiuti Stunned %d (di chiunque %d), Fallback di ogni causa %d (di chiunque %d), voci dell'attore %d (id %d); ")
			TEXT("primo rifiuto: fase %d, azione %s, da %s a %s; ")
			TEXT("ricarica %d>%d (Def %d, specchio %d), slot prima [%s] dopo [%s]; ")
			TEXT("passo %d verso %s, a fine turno su %s; playback mostra Prep %d, Move %d; ")
			TEXT("rotta pianificata %s, attesa %s, percorsa %s; testimone anticipato W %d (CoverCreated %d); ")
			TEXT("rifiuti di Move %d (esito Move dell'attore %d), apritore %d: %s>%s"),
			*Run, O.bStunnedAtCommit ? 1 : 0, O.bPlanAtCommit ? 1 : 0, O.bMainInPrep ? 1 : 0, O.bValid ? 1 : 0,
			*O.ActionId.ToString(), O.WBefore, O.WAfter, O.EBefore, O.EAfter, O.CoversAfter,
			O.Created, O.Moved, O.Rejected, O.WitnessWAfter, O.WitnessCreated, O.Activations,
			O.StunRefusals, O.RefusalsAnyUnit, O.AnyFallbacks, O.FallbacksAnyUnit, O.EntriesOfActor, O.StableId,
			static_cast<int32>(O.RefusalPhase), *O.RefusalActionId.ToString(), *O.RefusalSrc.ToString(), *O.RefusalTgt.ToString(),
			O.CooldownBefore, O.CooldownAfter, O.DefCooldown, O.MirrorCooldown,
			*CoverStunSlotsText(O.SlotsBefore), *CoverStunSlotsText(O.SlotsAfter),
			O.bMoveAtCommit ? 1 : 0, *O.Destination.ToString(), *O.FinalCell.ToString(),
			O.bSawPrepBeat ? 1 : 0, O.bSawMoveBeat ? 1 : 0,
			*CoverStunRouteText(O.PlannedRoute), *CoverStunRouteText(O.ExpectedRoute), *CoverStunRouteText(O.WalkedRoute),
			O.EarlyWAfter, O.EarlyCreated,
			O.MoveRefusals, static_cast<int32>(O.ActorMoveOutcome), O.bOpener ? 1 : 0, *O.OpenerPlanned.ToString(), *O.OpenerFinal.ToString());
	}

	/** Che cosa fa la ricarica dello slot sotto prova, a fine turno. */
	enum class ECoverStunCooldown : uint8
	{
		Free,         //< non parte: il residuo e' 0
		Starts,       //< parte: `Mirror - 1` — un addebito, poi il decremento del Cleanup
		KeepsTicking, //< era gia' in corso: `Prima - 1` — non ripagata
	};

	/** Che cosa ci si aspetta da UNA run in UN braccio. */
	struct FCoverStunExpect
	{
		int32 WAfter;                //< integrita' attesa sul bordo W a fine turno
		int32 EAfter;                //< ... sul bordo E
		int32 Created;               //< voci `CoverCreated`
		int32 Moved;                 //< voci `CoverMoved`
		int32 Rejected;              //< voci `CoverRejected`
		int32 Activations;           //< eventi `AbilityActivated` dell'attore
		int32 Refusals;              //< voci di rifiuto per stordimento di Prep: 1 dove l'attore e' rifiutato, 0 altrove. Salvo la riga «caduto», e' anche il numero atteso di `Fallback` di ogni causa e fase
		ECoverStunCooldown Cooldown; //< che cosa fa la ricarica dello slot
		int32 WitnessW = -1;         //< solo righe con testimone: integrita' attesa sul bordo W della sua copertura; -1 = senza testimone
		bool bFallenActor = false;   //< solo riga «caduto»: i rifiuti si contano in Prep e basta (vedi `CoverStunAssert`)
		int32 EarlyW = -1;           //< solo righe con testimone anticipato: integrita' attesa sul bordo W della sua copertura; -1 = senza testimone anticipato
	};

	/** Una riga della tabella: la scena, e l'attesa nei due bracci. */
	struct FCoverStunRow
	{
		FCoverStunScene Scene;
		FCoverStunExpect Control; //< non stordito: la lettura del codice, che [D-565] non tocca
		FCoverStunExpect Stunned; //< stordito: cio' che [D-565] vuole
	};

	int32 CoverStunExpectedCooldown(const FCoverStunExpect& X, const FCoverStunOutcome& O)
	{
		switch (X.Cooldown)
		{
		case ECoverStunCooldown::Free:         return 0;
		case ECoverStunCooldown::Starts:       return O.MirrorCooldown - 1;
		case ECoverStunCooldown::KeepsTicking: return O.CooldownBefore - 1;
		}
		return -1;
	}

	void CoverStunAssert(FAutomationTestBase& Test, const FString& Run, const FCoverStunExpect& X, const FCoverStunOutcome& O)
	{
		Test.TestEqual(*FString::Printf(TEXT("%s: la copertura sul bordo W"), *Run), O.WAfter, X.WAfter);
		Test.TestEqual(*FString::Printf(TEXT("%s: la copertura sul bordo E"), *Run), O.EAfter, X.EAfter);
		// Sulla cella bersaglio stanno le coperture dei due bordi attesi e basta: una struttura su un ALTRO bordo, o una
		// copertura in piu', lo smentirebbe anche con W ed E a posto.
		Test.TestEqual(*FString::Printf(TEXT("%s: coperture sulla cella bersaglio"), *Run),
			O.CoversAfter, (X.WAfter > 0 ? 1 : 0) + (X.EAfter > 0 ? 1 : 0));
		Test.TestEqual(*FString::Printf(TEXT("%s: voci CoverCreated"), *Run), O.Created, X.Created);
		Test.TestEqual(*FString::Printf(TEXT("%s: voci CoverMoved"), *Run), O.Moved, X.Moved);
		Test.TestEqual(*FString::Printf(TEXT("%s: voci CoverRejected"), *Run), O.Rejected, X.Rejected);
		Test.TestEqual(*FString::Printf(TEXT("%s: eventi AbilityActivated dell'attore"), *Run), O.Activations, X.Activations);
		// Il rifiuto di PREP, di chiunque: e' l'unico che questo banco decide.
		Test.TestEqual(*FString::Printf(TEXT("%s: voci di rifiuto per stordimento di fase Prep"), *Run), O.PrepRefusalsAnyUnit, X.Refusals);
		// ⚠️ Un cadavere stordito che arriva al Blast col piano addosso riceve LI' una voce di rifiuto (e non lo paga):
		// `CollectAttackIntents` non filtra i morti. E' preesistente, non e' di questo banco e non e' deciso, quindi per la
		// riga «caduto» le voci di rifiuto si contano in Prep e basta, e quella del Blast non si asserisce. Il NON-pagamento
		// invece si: la ricarica `Free` e gli altri slot, qui sotto, valgono anche per il cadavere.
		// D-569: se l'attore stordito ha anche un passo pianificato, il passo e' NEGATO e lascia la sua voce di rifiuto (fase Move) accanto a
		// quella della struttura (fase Prep). Le colonne `Refusals` della tabella restano quelle di Prep; la voce di Move si somma qui,
		// e la asserisce per esteso il blocco del passo in fondo.
		const int32 ExpectedMoveRefusals = (O.bMoveAtCommit && O.bStunnedAtCommit) ? 1 : 0;
		const int32 ExpectedAllRefusals = X.Refusals + ExpectedMoveRefusals;
		if (!X.bFallenActor)
		{
			Test.TestEqual(*FString::Printf(TEXT("%s: voci di rifiuto per stordimento dell'attore"), *Run), O.StunRefusals, ExpectedAllRefusals);
			Test.TestEqual(*FString::Printf(TEXT("%s: voci di rifiuto per stordimento di chiunque"), *Run), O.RefusalsAnyUnit, ExpectedAllRefusals);
			Test.TestEqual(*FString::Printf(TEXT("%s: voci Fallback dell'attore, di ogni causa"), *Run), O.AnyFallbacks, ExpectedAllRefusals);
			Test.TestEqual(*FString::Printf(TEXT("%s: voci Fallback di chiunque e di ogni causa (nessun altro)"), *Run), O.FallbacksAnyUnit, ExpectedAllRefusals);
		}
		if (X.Refusals > 0)
		{
			// La voce dice CHE azione, DA DOVE, VERSO DOVE e IN CHE FASE: un'azione, una cella o una fase sbagliate nel blocco
			// passerebbero il solo conteggio.
			Test.TestEqual(*FString::Printf(TEXT("%s: il rifiuto e' di fase Prep"), *Run),
				static_cast<int32>(O.RefusalPhase), static_cast<int32>(ERTMatchPhase::Prep));
			Test.TestEqual(*FString::Printf(TEXT("%s: il rifiuto nomina l'azione installata"), *Run),
				O.RefusalActionId.ToString(), O.ActionId.ToString());
			Test.TestEqual(*FString::Printf(TEXT("%s: il rifiuto parte dalla cella di chi agisce"), *Run),
				O.RefusalSrc.ToString(), O.Home.ToString());
			Test.TestEqual(*FString::Printf(TEXT("%s: il rifiuto mira la cella mirata"), *Run),
				O.RefusalTgt.ToString(), O.Aimed.ToString());
		}
		// Il Cleanup decrementa di uno: `Mirror - 1` = la ricarica e' PARTITA; `Prima - 1` = non e' stata ripagata; 0 = non parte.
		Test.TestEqual(*FString::Printf(TEXT("%s: ricarica a fine turno"), *Run), O.CooldownAfter, CoverStunExpectedCooldown(X, O));
		// Un addebito sullo slot sbagliato non deve restare verde perche' si e' letto solo lo slot giusto.
		Test.TestEqual(*FString::Printf(TEXT("%s: nessun altro slot e' in ricarica a fine turno"), *Run),
			CoverStunBusyOtherSlots(O.SlotsAfter, CoverStunSlot), 0);
		if (X.WitnessW >= 0)
		{
			// Il testimone e' ordinato DOPO l'attore: se il suo rifiuto interrompesse il ciclo (`break` al posto di `continue`),
			// le strutture di chi sta dietro non si risolverebbero.
			Test.TestEqual(*FString::Printf(TEXT("%s: il testimone ha eretto la sua copertura (il Prep e' girato)"), *Run),
				O.WitnessWAfter, X.WitnessW);
			Test.TestEqual(*FString::Printf(TEXT("%s: voci CoverCreated del testimone"), *Run), O.WitnessCreated, 1);
		}
		if (X.EarlyW >= 0)
		{
			// Il testimone anticipato e' ordinato PRIMA dell'attore: la sua operazione e' gia' raccolta (`Pending`) quando la guardia
			// scatta, e un rifiuto che buttasse la raccolta fatta (`Pending.Reset()` o simili) gliela toglierebbe. Il `continue` non
			// la tocca; un `break` neppure, e per questo il testimone di PRIMA e quello di DOPO sono righe diverse.
			Test.TestEqual(*FString::Printf(TEXT("%s: il testimone anticipato ha eretto la sua copertura (la raccolta gia' fatta non si butta)"), *Run),
				O.EarlyWAfter, X.EarlyW);
			Test.TestEqual(*FString::Printf(TEXT("%s: voci CoverCreated del testimone anticipato"), *Run), O.EarlyCreated, 1);
		}
		if (O.bMoveAtCommit)
		{
			// ⌫ [D-569]. Fino al 2026-11-03 questo blocco diceva che "il rifiuto e' dell'AZIONE, non del percorso: la stordita cammina come
			// chi non lo e'" ([D-416] (1) lasciava il movimento a `Root`), e usava il passo dell'attore come fatto di timeline che apre il
			// playback. L'autore ha ribaltato la regola: una stordita non si muove, e il suo passo e' NEGATO. Qui sotto il braccio di
			// CONTROLLO cammina ancora (e prova che il passo si compie, cosi' lo zero dello stordito non e' una fixture che non permette di
			// camminare); il braccio STORDITO resta fermo e lascia la voce di rifiuto in fase Move. Il fatto che apre il playback e' ora un
			// compagno NON stordito che cammina in entrambi i bracci (`Bed.Opener`).
			Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la scena ha l'apritore del playback"), *Run), O.bOpener);
			Test.TestEqual(*FString::Printf(TEXT("%s: l'apritore, non stordito, arriva alla sua cella in entrambi i bracci"), *Run),
				O.OpenerFinal.ToString(), O.OpenerPlanned.ToString());
			if (!O.bStunnedAtCommit)
			{
				Test.TestEqual(*FString::Printf(TEXT("%s: controllo: a fine turno l'attore e' sulla cella del passo pianificato"), *Run),
					O.FinalCell.ToString(), O.Destination.ToString());
				// La rotta e' quella del PIANO: il suo percorso composito se la riga ne scrive uno, altrimenti quella che il pathfinder
				// trova verso la sola destinazione. La cella finale non lo dice: con un percorso diverso da quello del pathfinder e' la
				// stessa, e senza percorso scritto (la forma dei bot) la raggiunge solo chi la rotta la ricava dalla destinazione.
				Test.TestEqual(*FString::Printf(TEXT("%s: controllo: l'attore ha percorso la rotta del piano"), *Run),
					CoverStunRouteText(O.WalkedRoute), CoverStunRouteText(O.ExpectedRoute));
				Test.TestEqual(*FString::Printf(TEXT("%s: controllo: nessun rifiuto di fase Move"), *Run), O.MoveRefusals, 0);
			}
			else
			{
				Test.TestEqual(*FString::Printf(TEXT("%s: stordito: l'attore resta sulla sua cella, il passo e' negato"), *Run),
					O.FinalCell.ToString(), O.Home.ToString());
				Test.TestEqual(*FString::Printf(TEXT("%s: stordito: nessuna rotta percorsa"), *Run),
					CoverStunRouteText(O.WalkedRoute), CoverStunRouteText(TArray<FRTCellId>()));
				Test.TestEqual(*FString::Printf(TEXT("%s: stordito: UNA voce di rifiuto di fase Move"), *Run), O.MoveRefusals, 1);
				Test.TestEqual(*FString::Printf(TEXT("%s: stordito: la voce parte dalla cella di chi agisce"), *Run),
					O.MoveRefusalSrc.ToString(), O.Home.ToString());
				Test.TestEqual(*FString::Printf(TEXT("%s: stordito: e non rivela la destinazione (le due celle sono quella di partenza)"), *Run),
					O.MoveRefusalTgt.ToString(), O.Home.ToString());
				Test.TestEqual(*FString::Printf(TEXT("%s: stordito: la voce Move dell'attore dice Stayed"), *Run),
					static_cast<int32>(O.ActorMoveOutcome), static_cast<int32>(ERTMoveOutcome::Stayed));
			}
			// Il playback si apre perche' l'apritore cammina, un fatto di timeline in entrambi i bracci: senza, «niente Prep» sarebbe
			// vero anche con un rifiuto contato fra le operazioni applicate (vedi `CoverStunRunTurnWatchingPlayback`).
			Test.TestTrue(*FString::Printf(TEXT("%s: il playback mostra il passo (fase Move)"), *Run), O.bSawMoveBeat);
			// Qui il beat di Prep c'e' se e solo se un'operazione e' stata APPLICATA (`ResolveCoverStructures` > 0): un rifiuto, e
			// un `Reject` di raccolta, non lo accendono — ne' nel braccio stordito ne' in quello di controllo. L'equivalenza vale
			// per questa scena: `BeginPlayback` aggiunge la fase Prep anche per un `AbilityActivated` di Prep visibile alla
			// squadra, e `bPrepActiveThisTurn` ha altri setter (Overwatch e predittive che armano, e il motore azioni che
			// applica un evento di Prep), che qui non ci sono.
			Test.TestEqual(*FString::Printf(TEXT("%s: il beat di Prep c'e' solo se una struttura e' stata applicata"), *Run),
				O.bSawPrepBeat, X.Created + X.Moved > 0);
		}
	}

	/** Un sito che il rifiuto lo scrive, per il controllo POSITIVO del contatore. */
	struct FCoverStunControlSite
	{
		const TCHAR* ActionId; //< una principale di quel sito
		ERTMatchPhase Phase;   //< la fase in cui il rifiuto si scrive
		bool bAimsAtCell;      //< armare una zona mira una CELLA; un'azione ambientale mira un'unita'
	};

	/**
	 * Lo stesso predicato dei bracci stordito, su un sito che il rifiuto lo scrive. Senza questo, «nessun rifiuto» sulle
	 * strutture proverebbe solo che il contatore non vede niente: uno `StableUnitId` a zero o un esito sbagliato nel
	 * predicato lo renderebbero cieco. Con una voce attesa in una fase nota, uno zero e' una misura.
	 */
	void CoverStunPositiveControl(FAutomationTestBase& Test, const FCoverStunControlSite& Site)
	{
		const FString Label = FString::Printf(TEXT("controllo positivo (%s)"), Site.ActionId);
		UWorld* World = MakeEnvWorld();
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: world di prova"), *Label), World)) { return; }
		SpawnEnvMap(World);

		const FRTCellId Home(0, 0);
		ARTUnit* Actor = SpawnEnvUnit(World, 0, Home);
		ARTUnit* Target = SpawnEnvUnit(World, 1, FRTCellId(2, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: attore"), *Label), Actor)
			|| !Test.TestNotNull(*FString::Printf(TEXT("%s: bersaglio"), *Label), Target)
			|| !Test.TestNotNull(*FString::Printf(TEXT("%s: TM"), *Label), TM))
		{
			DestroyEnvWorld(World);
			return;
		}

		if (Site.bAimsAtCell)
		{
			// Armare e' mirare una ZONA: il bersaglio e' una cella, come per le strutture di bordo.
			RTAbilityFixtures::AddCoreAbilityInSlot(Actor, Site.ActionId, CoverStunSlot);
			Actor->PlannedAbilityIndex = CoverStunSlot;
			Actor->PlannedAttackTarget = nullptr;
			Actor->PlannedAttackCell = FRTCellId(1, 0);
			Actor->bAttackTargetsCell = true;
		}
		else
		{
			PlanEnvAction(Actor, Site.ActionId, Target);
		}
		Actor->ApplyStatus(TAG_Status_Stunned, URTCombatLibrary::StunnedDurationTurns);
		RunEnvTurn(TM);

		const int32 Id = Actor->StableUnitId;
		const int32 Refusals = CoverStunCountRefusals(TM, Id);
		int32 FoundPhase = -1;
		FString FoundAction;
		FString FoundSrc;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (CoverStunIsRefusal(E) && E.UnitId == Id)
			{
				FoundPhase = static_cast<int32>(E.Phase);
				FoundAction = E.ActionId.ToString();
				FoundSrc = E.SrcCell.ToString();
				break;
			}
		}
		Test.AddInfo(FString::Printf(TEXT("%s -> rifiuti Stunned %d (id %d), fase %d, azione %s, da %s"),
			*Label, Refusals, Id, FoundPhase, *FoundAction, *FoundSrc));
		Test.TestTrue(*FString::Printf(TEXT("%s: l'attore ha uno StableUnitId"), *Label), Id != 0);
		Test.TestEqual(*FString::Printf(TEXT("%s: il contatore vede il rifiuto di un sito che lo scrive"), *Label), Refusals, 1);
		Test.TestEqual(*FString::Printf(TEXT("%s: il rifiuto e' nella fase del sito"), *Label), FoundPhase, static_cast<int32>(Site.Phase));
		Test.TestEqual(*FString::Printf(TEXT("%s: il rifiuto nomina l'azione"), *Label), FoundAction, FString(Site.ActionId));
		Test.TestEqual(*FString::Printf(TEXT("%s: il rifiuto parte dalla cella di chi agisce"), *Label), FoundSrc, Home.ToString());

		DestroyEnvWorld(World);
	}
}

/**
 * `RIC-5` (#3958), [D-565]: uno stordito NON erige e NON sposta una copertura di bordo, e il rifiuto PAGA la ricarica.
 *
 * `Action.CreateCover`, `Hero.Branth.KineticPanel`, `Hero.Branth.Reconfigure` e `Gadget.PortableCover` sono azioni
 * **principali** di fase Prep (le premesse lo verificano riga per riga), quindi [D-416] (1) le nega come tutte le altre. Il braccio STORDITO di ogni riga
 * asserisce la regola: la mappa com'era, nessuna struttura e nessuna `AbilityActivated`, UNA voce di rifiuto di fase Prep che
 * nomina l'azione, la cella di chi agisce e la cella mirata, nessun altro `Fallback`, e la ricarica che PARTE (`Mirror - 1` a
 * fine turno, dopo il decremento del Cleanup) sul solo slot dell'azione. Il braccio di CONTROLLO e' lo stesso piano senza
 * stordimento, e non cambia.
 *
 * Le righe fresche pianificano anche un passo normale. ⌫ **[D-569] (2026-11-03) ha ribaltato cio' che questo passo pinnava**: fino
 * ad allora il rifiuto era "dell'AZIONE, non del percorso" e la stordita camminava ([D-416] (1) lasciava il movimento a
 * `Status.Root`); ora una stordita non si muove e il suo passo e' NEGATO, con una voce `Fallback`/`Cancelled`/`Stunned` di fase
 * Move accanto a quella di Prep. Il braccio di CONTROLLO cammina ancora. Cio' che il rifiuto di Prep NON e' resta pinnato: non e'
 * un'operazione applicata, quindi non accende il beat di Prep del playback (che si legge con `CurrentPlaybackPhaseForTest`).
 * ⚠️ Perche' quel "niente Prep" valga serve un fatto in timeline che apra il playback anche nel braccio stordito: non e' piu' il
 * passo dell'attore (negato) ma quello di un compagno NON stordito che cammina in entrambi i bracci (`Bed.Opener`).
 *
 * Il passo ha forme diverse: il percorso di un passo (la forma del giocatore dopo un click), il percorso COMPOSITO con una
 * deviazione e la SOLA DESTINAZIONE (`PlannedPath` vuoto: la forma dei bot, il cui percorso nasce dentro la fase Move). Nel
 * controllo la stordita cammina QUELLO che ha pianificato (col composito la cella finale e' la stessa del percorso diretto, e la
 * distingue solo la rotta: il banco la legge dall'evento `Move` della timeline); da stordita resta ferma in tutte e tre. Una
 * premessa per riga verifica che il piano abbia la forma dichiarata, perche' gli asserti del passo si saltano se il piano non ha
 * un passo.
 *
 * Le righe oltre la prima famiglia pinnano DOVE sta la guardia e COSA riconosce:
 *  - **gia' in ricarica**: `CanUseAbility` e' falso e il piano si scarta in silenzio PRIMA della guardia — niente voce e
 *    niente secondo addebito, in entrambi i bracci (`ConsumeAbility` scrive un valore FISSO: ripagarla riporterebbe lo slot
 *    a `Mirror`). A differenza di `Status.StunDeniedMainActionPaysItsCooldown` (riga «Prep: stordito, gia' in ricarica»), qui
 *    il ciclo azzera il piano PRIMA di `CanUseAbility`, quindi nemmeno il Blast ritrova niente da rifiutare;
 *  - **senza bordo**, **senza bersaglio**, **fuori portata** (i `Reject` di raccolta): quello di chi NON e' stordito
 *    resta GRATIS (`RIC-3`, [D-571]: «senza bersaglio» il click non lo produce, «fuori portata» lo rifiuta, «senza bordo» lo
 *    ACCETTA ed e' `BLOCKED`) e lascia `CoverRejected`; lo stordito e' rifiutato PRIMA di ciascuno, quindi
 *    paga e non lascia `CoverRejected`;
 *  - **con testimone**: la stordita e' rifiutata e il ciclo PROSEGUE. Un compagno della stessa squadra, ordinato DOPO di lei
 *    da `SortUnitsForResolution` (una premessa lo verifica), erige altrove: la sua copertura nasce anche nel braccio
 *    stordito. Un rifiuto che interrompesse il ciclo — `break` al posto di `continue` — cancellerebbe le strutture di chi
 *    sta dietro;
 *  - **con testimone anticipato**: l'opposto. Un compagno ordinato PRIMA della stordita (una premessa lo verifica) ha la sua
 *    operazione gia' RACCOLTA quando la guardia scatta, e il rifiuto di lei non deve buttarla: la sua copertura nasce anche nel
 *    braccio stordito. Ne' il `continue` ne' il `break` la toccano, e per questo il testimone di DOPO non basta; un rifiuto che
 *    azzerasse la raccolta gia' fatta si';
 *  - **un'altra azione**: `Gadget.PortableCover` e' l'azione che il gadget concede (`MakeEquipmentAction`), costruita come la
 *    costruisce il gioco. Copia la definizione di `Action.CreateCover`, quindi la sua `StructureOp`, e porta un nome e una
 *    ricarica suoi. La guardia vale per DISCRIMINANTE e non per nome ([D-565] (1)): la stordita che la pianifica e' rifiutata
 *    come le altre, e una guardia che riconoscesse i soli nomi delle altre azioni la lascerebbe erigere;
 *  - **attore caduto**: il ciclo lo salta prima della guardia — niente voce di Prep, nessuno slot si muove. Il testimone vivo
 *    che erige altrove nello stesso Prep prova che il Prep e' girato: senza, lo zero non distinguerebbe una guardia assente
 *    da una fase mai eseguita. ⚠️ Il banco conta i rifiuti di Prep e basta: un cadavere stordito col piano addosso riceve
 *    ancora in Blast la sua voce di rifiuto (`CollectAttackIntents` non filtra i morti), preesistente e non decisa, che qui
 *    non si asserisce. Si asserisce invece, per costruzione, che NON paga: la riga ha la ricarica `Free` e nessun altro slot
 *    in ricarica in entrambi i bracci ([D-563] (2)-(3), la scrittura sta dietro `IsAlive()`). Se una decisione futura lo
 *    facesse pagare, la riga diventerebbe rossa per quel motivo.
 *
 * In coda i controlli POSITIVI del contatore: `Action.Overwatch` (Prep) e `Action.Ignite` (Cleanup), principali di siti che
 * il rifiuto lo scrivono, devono lasciare UNA voce di rifiuto nella fase giusta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStunnedCasterChangesNothingTest,
	"RefactorTactics.Actions.CoverStructures.StunnedCasterChangesNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStunnedCasterChangesNothingTest::RunTest(const FString&)
{
	using Cd = ECoverStunCooldown;
	// Colonne dell'attesa: bordo W, bordo E | Created, Moved, Rejected | AbilityActivated, rifiuti | ricarica [| testimone, caduto, anticipato].
	const FCoverStunRow Rows[] = {
		// Fresche e complete: il rifiuto prende il posto della struttura, e la ricarica parte lo stesso. L'attore pianifica
		// anche un passo normale: da D-569 lo stordimento glielo NEGA (nel controllo cammina), e il fatto di timeline che apre il
		// playback anche nel braccio stordito e' l'apritore — senza, «niente beat di Prep» sarebbe vero perche' il playback non c'e'.
		{ CoverStunWithStep({ ECoverStunKind::CreateCover }),
		  /*controllo*/ { 30,  0, 1, 0, 0, 1, 0, Cd::Starts },
		  /*stordito */ {  0,  0, 0, 0, 0, 0, 1, Cd::Starts } },
		{ CoverStunWithStep({ ECoverStunKind::KineticPanel }),
		  /*controllo*/ { 30,  0, 1, 0, 0, 1, 0, Cd::Starts },
		  /*stordito */ {  0,  0, 0, 0, 0, 0, 1, Cd::Starts } },
		{ CoverStunWithStep({ ECoverStunKind::Reconfigure }),
		  /*controllo*/ {  0, 18, 0, 1, 0, 1, 0, Cd::Starts },
		  /*stordito */ { 18,  0, 0, 0, 0, 0, 1, Cd::Starts } },

		// UN'ALTRA AZIONE che dichiara `StructureOp`: il gadget copia la definizione di `Action.CreateCover` e ha un nome (e una
		// ricarica) che le righe qui sopra non portano. La guardia vale per discriminante, non per nome: una guardia per nome lascerebbe
		// erigere la stordita, e questa riga e' l'unica a vederlo. Il pagamento si legge sul valore del pezzo (`Mirror - 1`).
		{ CoverStunWithStep({ ECoverStunKind::PortableCover }),
		  /*controllo*/ { 30,  0, 1, 0, 0, 1, 0, Cd::Starts },
		  /*stordito */ {  0,  0, 0, 0, 0, 0, 1, Cd::Starts } },

		// LE ALTRE FORME DEL PASSO. Nel controllo il passo si compie QUELLO che si e' pianificato: un percorso composito con una
		// deviazione (la cella finale e' quella del percorso diretto: la distingue la rotta) e la sola destinazione, senza percorso
		// scritto (la forma dei bot: la fase Move ricava la rotta da `PlannedCell`). ⌫ Fino a D-569 anche la stordita li camminava,
		// e la riga pinnava che il rifiuto di Prep non fermasse il passo; ora lo ferma in tutte e tre le forme.
		{ CoverStunWithDetour({ ECoverStunKind::CreateCover }),
		  /*controllo*/ { 30,  0, 1, 0, 0, 1, 0, Cd::Starts },
		  /*stordito */ {  0,  0, 0, 0, 0, 0, 1, Cd::Starts } },
		{ CoverStunWithDestinationOnly({ ECoverStunKind::CreateCover }),
		  /*controllo*/ { 30,  0, 1, 0, 0, 1, 0, Cd::Starts },
		  /*stordito */ {  0,  0, 0, 0, 0, 0, 1, Cd::Starts } },

		// CON TESTIMONE: la stordita e' rifiutata e il ciclo PROSEGUE — la struttura del compagno, ordinato dopo di lei, nasce.
		// Nel controllo nascono entrambe.
		{ CoverStunWithWitness({ ECoverStunKind::CreateCover }),
		  /*controllo*/ { 30,  0, 2, 0, 0, 1, 0, Cd::Starts, /*testimone*/ 30 },
		  /*stordito */ {  0,  0, 1, 0, 0, 0, 1, Cd::Starts, /*testimone*/ 30 } },

		// CON TESTIMONE ANTICIPATO: l'opposto. Il compagno ordinato PRIMA della stordita ha la sua operazione gia' raccolta quando la
		// guardia scatta, e il rifiuto di lei non la butta: la sua struttura nasce anche nel braccio stordito. Nel controllo nascono
		// entrambe.
		{ CoverStunWithEarlyWitness({ ECoverStunKind::CreateCover }),
		  /*controllo*/ { 30,  0, 2, 0, 0, 1, 0, Cd::Starts, /*testimone*/ -1, /*caduto*/ false, /*anticipato*/ 30 },
		  /*stordito */ {  0,  0, 1, 0, 0, 0, 1, Cd::Starts, /*testimone*/ -1, /*caduto*/ false, /*anticipato*/ 30 } },

		// GIA' IN RICARICA: scarto muto di `CanUseAbility`, prima della guardia. Identico nei due bracci.
		{ { ECoverStunKind::CreateCover, /*bAlreadyCooling=*/ true },
		  /*controllo*/ {  0,  0, 0, 0, 0, 0, 0, Cd::KeepsTicking },
		  /*stordito */ {  0,  0, 0, 0, 0, 0, 0, Cd::KeepsTicking } },
		{ { ECoverStunKind::Reconfigure, /*bAlreadyCooling=*/ true },
		  /*controllo*/ { 18,  0, 0, 0, 0, 0, 0, Cd::KeepsTicking },
		  /*stordito */ { 18,  0, 0, 0, 0, 0, 0, Cd::KeepsTicking } },

		// SENZA BORDO: chi non e' stordito lascia il `Reject` di raccolta e non paga; lo stordito e' rifiutato prima, e paga.
		// Col passo anche qui: un `Reject` di raccolta NON e' un'operazione applicata, quindi nessuno dei due bracci mostra il
		// beat di Prep.
		{ CoverStunWithStep({ ECoverStunKind::CreateCover, false, /*bNoEdge=*/ true }),
		  /*controllo*/ {  0,  0, 0, 0, 1, 0, 0, Cd::Free },
		  /*stordito */ {  0,  0, 0, 0, 0, 0, 1, Cd::Starts } },
		{ CoverStunWithStep({ ECoverStunKind::Reconfigure, false, /*bNoEdge=*/ true }),
		  /*controllo*/ { 18,  0, 0, 0, 1, 0, 0, Cd::Free },
		  /*stordito */ { 18,  0, 0, 0, 0, 0, 1, Cd::Starts } },
		// SENZA BERSAGLIO e FUORI PORTATA: gli altri `Reject` di raccolta. La guardia sta prima di OGNUNO, non solo del primo.
		{ CoverStunWithStep({ ECoverStunKind::CreateCover, false, false, false, /*bNoTarget=*/ true }),
		  /*controllo*/ {  0,  0, 0, 0, 1, 0, 0, Cd::Free },
		  /*stordito */ {  0,  0, 0, 0, 0, 0, 1, Cd::Starts } },
		{ CoverStunWithStep({ ECoverStunKind::CreateCover, false, false, false, false, /*bOutOfRange=*/ true }),
		  /*controllo*/ {  0,  0, 0, 0, 1, 0, 0, Cd::Free },
		  /*stordito */ {  0,  0, 0, 0, 0, 0, 1, Cd::Starts } },

		// ATTORE CADUTO: nessuna voce di Prep e nessun addebito; l'unica copertura e' quella del testimone.
		// ⚠️ La ricarica `Free` e «nessun altro slot» valgono ANCHE per il cadavere stordito che arriva al Blast col piano
		// addosso: la riga pinna, per costruzione, che non PAGA ([D-563] (2)-(3): «purche' fosse usabile e l'unita' sia viva»;
		// la scrittura sta dietro `IsAlive()`). La voce di rifiuto che il Blast gli scrive comunque e' preesistente, non
		// decisa e non asserita.
		{ CoverStunWithWitness({ ECoverStunKind::CreateCover, false, false, /*bFallen=*/ true }),
		  /*controllo*/ {  0,  0, 1, 0, 0, 0, 0, Cd::Free, /*testimone*/ 30, /*bFallenActor=*/ true },
		  /*stordito */ {  0,  0, 1, 0, 0, 0, 0, Cd::Free, /*testimone*/ 30, /*bFallenActor=*/ true } },
	};

	for (const FCoverStunRow& Row : Rows)
	{
		for (int32 Arm = 0; Arm < 2; ++Arm)
		{
			const bool bStunned = Arm == 1;
			const FString Run = CoverStunRunLabel(Row.Scene, bStunned);
			const FCoverStunOutcome O = CoverStunRun(*this, Row.Scene, bStunned);
			AddInfo(CoverStunDescribe(Run, O));
			if (!O.bValid) { return false; }
			CoverStunAssert(*this, Run, bStunned ? Row.Stunned : Row.Control, O);
		}
	}

	// Controlli POSITIVI del contatore, sullo stesso tipo di banco: un sito di Prep e uno di Cleanup.
	const FCoverStunControlSite Sites[] = {
		{ TEXT("Action.Overwatch"), ERTMatchPhase::Prep, /*bAimsAtCell=*/ true },
		{ TEXT("Action.Ignite"), ERTMatchPhase::Cleanup, /*bAimsAtCell=*/ false },
	};
	for (const FCoverStunControlSite& Site : Sites)
	{
		CoverStunPositiveControl(*this, Site);
	}
	return true;
}

/**
 * `RIC-5` (#3958), [D-565]: la ROTAZIONE GRATUITA del pannello adattivo SOPRAVVIVE al rifiuto.
 *
 * `Hero.Branth.KineticPanel.Adaptive` eretto da chi non e' stordito ha 25 punti struttura, non scade e porta con se' una
 * rotazione gratuita (`FreeRotations` 1 in `DynamicCovers`). I turni sono sullo stesso attore (la sequenza e' in
 * `CoverStunRunRotation`): T1 erige, T2 e T4 sono vuoti, T3 e T5 pianificano `Hero.Branth.Reconfigure`, e nel braccio stordito
 * T6 lo ripianifica. Il braccio stordito riceve lo stordimento solo prima di T3; e' libero da T5.
 *
 *  - **T3 stordito**: rifiutato. La copertura resta in W (25), nessuno spostamento e nessuna attivazione, UNA voce di rifiuto
 *    di Prep, la ricarica PARTE. La rotazione gratuita NON si tocca: il rifiuto precede lo spostamento.
 *  - **T5 stordito, ormai libero**: la STESSA copertura ruota gratis — spostata, ricarica a zero, una attivazione. Se il
 *    rifiuto avesse speso il credito, T5 pagherebbe.
 *  - **T6 stordito, libero**: la copertura torna in W e la rotazione PAGA — spostata, una attivazione, la ricarica parte. Il
 *    credito era UNO e T5 lo ha usato; se il rifiuto di T3 ne avesse regalato un secondo, T6 sarebbe ancora gratuito.
 *  - **Controllo**: stesso flusso senza stordimento, fino a T5. T3 ruota gratis e SPENDE il credito; T5, per riportare la
 *    copertura in W, PAGA. Il contrasto dimostra che T5 dello stordito e' gratuito solo perche' il credito e' integro. Il T5
 *    del controllo e' anche il controllo di T6: la stessa rotazione E->W dallo stesso stato (credito zero, ricarica zero) di chi
 *    non e' stordito, che sposta e paga; per questo T6 stordito si aspetta lo stesso identico esito.
 *  - **Due crediti** (controllo POSITIVO del sensore di T6): lo stesso flusso stordito, con una variante che dichiara DUE
 *    rotazioni gratuite invece di una (sulla copia propria dei dati dell'azione, e una premessa legge il numero dalla variante
 *    attiva). T5 e T6 sono entrambi gratuiti. E' la prova che T6 puo' essere gratuito, quindi che il suo «paga» con un credito
 *    solo e' una misura e non un sensore cieco.
 *
 * ⚠️ E' il caso in cui «il rifiuto paga la ricarica intera» e «la rotazione gratuita non paga» si toccano: uno stordito
 * rifiutato paga a T3 anche se l'esecuzione sarebbe stata gratuita ([D-414]: «se si usa un'abilita' si paga»), ma non brucia
 * il credito che l'esecuzione avrebbe consumato, e non ne aggiunge uno.
 *
 * ⚠️ **Il credito ha UN sensore solo**, la ricarica pagata (`FreeRotations` e' un campo di `FRTDynamicCover`, nella lista
 * `DynamicCovers` che e' `protected`, senza accessor, e lo `StateHash` non lo copre: il banco non lo legge, e un accessor di
 * test non e' stato aperto). Ma il sensore legge il credito in T5 e in T6: un credito SPESO dal rifiuto si vede a T5 (paga, invece
 * di essere gratuito) e un credito REGALATO dal rifiuto si vede a T6 (e' gratuito, invece di pagare).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStunnedCasterKeepsTheFreeRotationTest,
	"RefactorTactics.Actions.CoverStructures.StunnedCasterKeepsTheFreeRotation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStunnedCasterKeepsTheFreeRotationTest::RunTest(const FString&)
{
	using Cd = ECoverStunCooldown;
	// Colonne come nell'altro test: bordo W, bordo E | Created, Moved, Rejected | AbilityActivated, rifiuti | ricarica.
	const FCoverStunExpect Turn3Control = {  0, 25, 0, 1, 0, 1, 0, Cd::Free };   // ruota gratis e spende il credito
	const FCoverStunExpect Turn3Stunned = { 25,  0, 0, 0, 0, 0, 1, Cd::Starts }; // rifiutato: la copertura resta, la ricarica parte
	const FCoverStunExpect Turn5Control = { 25,  0, 0, 1, 0, 1, 0, Cd::Starts }; // E->W: il credito e' speso, quindi PAGA
	const FCoverStunExpect Turn5Stunned = {  0, 25, 0, 1, 0, 1, 0, Cd::Free };   // W->E: il credito e' integro, quindi e' GRATIS
	// T6 stordito, libero: la STESSA rotazione E->W, dallo stesso stato, del T5 del controllo — l'attesa e' quella, e non una
	// seconda scritta a mano. Con DUE crediti dichiarati (controllo positivo) il secondo e' ancora integro, quindi e' GRATIS.
	const FCoverStunExpect Turn6Stunned = Turn5Control;
	const FCoverStunExpect Turn6TwoCredits = { 25,  0, 0, 1, 0, 1, 0, Cd::Free };

	// I bracci: il controllo (non stordito), lo stordito (un credito) e lo stordito con DUE crediti dichiarati.
	for (int32 Arm = 0; Arm < 3; ++Arm)
	{
		const bool bStunned = Arm >= 1;
		const bool bTwoCredits = Arm == 2;
		const FCoverStunRotation R = CoverStunRunRotation(*this, bStunned, bTwoCredits);
		AddInfo(CoverStunDescribe(R.Label + TEXT(", turno 3"), R.Turn3));
		AddInfo(FString::Printf(TEXT("%s -> stordimento dopo il turno 3 %d, dopo il turno 4 %d; ricarica dopo il turno 4 %d"),
			*R.Label, R.bStunnedAfterTurn3 ? 1 : 0, R.bStunnedAfterTurn4 ? 1 : 0, R.CooldownAfterTurn4));
		AddInfo(CoverStunDescribe(R.Label + TEXT(", turno 5"), R.Turn5));
		if (bStunned) { AddInfo(CoverStunDescribe(R.Label + TEXT(", turno 6"), R.Turn6)); }
		if (!R.bValid) { return false; }

		CoverStunAssert(*this, R.Label + TEXT(", turno 3"), bStunned ? Turn3Stunned : Turn3Control, R.Turn3);
		CoverStunAssert(*this, R.Label + TEXT(", turno 5"), bStunned ? Turn5Stunned : Turn5Control, R.Turn5);
		if (bStunned)
		{
			CoverStunAssert(*this, R.Label + TEXT(", turno 6"), bTwoCredits ? Turn6TwoCredits : Turn6Stunned, R.Turn6);
		}
	}
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteractDoesNotCreateCoverTest,
	"RefactorTactics.Structures.Interact.DoesNotCreateCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInteractDoesNotCreateCoverTest::RunTest(const FString&)
{
	// 🔴 **Difende il ramo che tiene `Action.Interact` FUORI dal loop delle coperture** ([D-148]).
	//
	// `Interact` dichiara `StructureOp = SetDoorState` per farsi puntare su un bordo, e questo loop prende
	// tutto cio' che non e' `None`: tratta `MoveCover` a parte e manda **tutto il resto** al ramo che erige
	// una copertura dal catalogo terreni. Nessuno `switch` su `ERTStructureOp` e' esaustivo, quindi togliere
	// il ramo delle porte **compila senza un avviso** — e l'azione costruirebbe un muro invece di aprire.
	//
	// ⚠️ Questo test esiste perche' la mutazione l'ha dimostrato: rimosso quel ramo, **192 test su 192
	// restavano verdi**. La riga era una difesa senza difensori, ed e' esattamente la forma di difetto che
	// una verifica di mutazione trova e un ciclo verde no.
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World);

	const FRTCellId Home(0, 0);
	const FRTCellId Target(1, 0); // adiacente: `Interact` ha portata 1 (D-149)

	ARTUnit* Actor = SpawnEnvUnit(World, 0, Home);
	ARTUnit* Foe = SpawnEnvUnit(World, 1, FRTCellId(-4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Actor"), Actor) || !TestNotNull(TEXT("Foe"), Foe) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyEnvWorld(World);
		return false;
	}

	TestEqual(TEXT("all'inizio il bordo e' scoperto"),
		URTHexCoverLibrary::CoverBetween(MapActor->MapAsset, Target, Home), ERTHexCoverType::None);

	// Stesso helper di `Action.CreateCover`: dichiara cella E bordo, cioe' il piano che il loop delle
	// strutture consuma. Se `Interact` finisse in quel ramo, qui nascerebbe una copertura.
	PlanCoverAction(Actor, TEXT("Action.Interact"), Target, ERTHexDirection::W);
	RunEnvTurn(TM);

	TestEqual(TEXT("Interact NON erige una copertura"),
		URTHexCoverLibrary::CoverBetween(MapActor->MapAsset, Target, Home), ERTHexCoverType::None);
	TestEqual(TEXT("e il TurnLog non riporta nessuna copertura creata"),
		CountEnvOutcome(TM, ERTEnvironmentOutcome::CoverCreated), 0);

	DestroyEnvWorld(World);
	return true;
}

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
 * CP 9.5 — `Branth.KineticPanel` erige davvero, e la VARIANTE attiva decide integrita' e durata.
 *
 * Fino a qui i `Parameters` delle varianti erano una dichiarazione che nessun sistema leggeva, in tutto il
 * progetto: il catalogo scriveva «45 per un turno solo» e «25 che non scade» e il gioco applicava sempre 30/2.
 * Questo e' il test che rende il compromesso osservabile — e cade se qualcuno riporta i numeri a costanti.
 *
 * I due rami dimostrano cose diverse: il rinforzato che la durata viene letta (1 turno: cade nel Cleanup del
 * turno stesso), l'adattivo che `DurationTurns = 0` significa «non scade da sola» e non «scade subito» — la
 * lettura sbagliata piu' probabile, e quella che il campo non perdonerebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBranthPanelVariantAppliedTest,
	"RefactorTactics.Heroes.Branth.KineticPanelVariantApplied",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBranthPanelVariantAppliedTest::RunTest(const FString&)
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
	URTActionData* PanelA = URTHeroCatalogLibrary::MakeBranth()->Actions[1];
	URTActionData* PanelB = URTHeroCatalogLibrary::MakeBranth()->Actions[1];

	PlanHeroCoverAction(WithReinforced, PanelA, Reinforced, ERTHexDirection::W,
		TEXT("Hero.Branth.KineticPanel.Reinforced"));
	PlanHeroCoverAction(WithAdaptive, PanelB, Adaptive, ERTHexDirection::SW,
		TEXT("Hero.Branth.KineticPanel.Adaptive"));
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
 * CP 9.5 — `Branth.Reconfigure` SPOSTA una copertura: non ne crea una seconda.
 *
 * E' il nome che la DoD vincola (`ReconfigureDoesNotDuplicate`), e il difetto che sorveglia e' preciso: una
 * implementazione che «aggiunge sul bordo nuovo» senza togliere dal vecchio raddoppierebbe la protezione con
 * un'azione che il catalogo descrive come una rotazione. Il test conta le voci, non guarda solo il bordo di
 * arrivo — contare e' l'unico modo di accorgersi di una duplicazione.
 *
 * Verifica anche che l'integrita' VIAGGI con la copertura: spostare un pannello ammaccato non lo ripara.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBranthReconfigureTest,
	"RefactorTactics.Heroes.Branth.ReconfigureDoesNotDuplicate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBranthReconfigureTest::RunTest(const FString&)
{
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World);

	const FRTCellId Home(0, 0);
	const FRTCellId Panel(1, 0);

	ARTUnit* Branth = SpawnEnvUnit(World, 0, Home);
	ARTUnit* Foe = SpawnEnvUnit(World, 1, FRTCellId(-4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Branth"), Branth) || !TestNotNull(TEXT("Foe"), Foe) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyEnvWorld(World);
		return false;
	}

	// Una copertura gia' in campo, ammaccata: 18 punti struttura invece di 30.
	URTHexCoverLibrary::AddCover(MapActor->MapAsset, Panel, ERTHexDirection::W, ERTHexCoverType::Low, 18);

	URTActionData* Reconfigure = URTHeroCatalogLibrary::MakeBranth()->Actions[2];
	TestTrue(TEXT("Reconfigure dichiara di spostare"),
		Reconfigure->Def.StructureOp == ERTStructureOp::MoveCover);

	PlanHeroCoverAction(Branth, Reconfigure, Panel, ERTHexDirection::E);
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBranthReconfigureRefusesTest,
	"RefactorTactics.Heroes.Branth.ReconfigureRefusesInsteadOfGuessing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBranthReconfigureRefusesTest::RunTest(const FString&)
{
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World);

	const FRTCellId Home(0, 0);
	const FRTCellId Two(1, 0);   // cella con DUE coperture
	const FRTCellId One(0, 1);   // cella con una sola, ma destinazione occupata

	ARTUnit* Branth = SpawnEnvUnit(World, 0, Home);
	ARTUnit* Foe = SpawnEnvUnit(World, 1, FRTCellId(-4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Branth"), Branth) || !TestNotNull(TEXT("Foe"), Foe) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyEnvWorld(World);
		return false;
	}

	URTHexCoverLibrary::AddCover(MapActor->MapAsset, Two, ERTHexDirection::W, ERTHexCoverType::Low, 30);
	URTHexCoverLibrary::AddCover(MapActor->MapAsset, Two, ERTHexDirection::E, ERTHexCoverType::Low, 30);

	URTActionData* Reconfigure = URTHeroCatalogLibrary::MakeBranth()->Actions[2];
	PlanHeroCoverAction(Branth, Reconfigure, Two, ERTHexDirection::NE);
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

	// `Branth.Reconfigure` ha COOLDOWN 2: come sopra, il secondo rifiuto va chiesto quando l'azione e'
	// tornata disponibile, non al turno dopo (#135).
	RunEnvTurn(TM);

	URTActionData* Second = URTHeroCatalogLibrary::MakeBranth()->Actions[2];
	PlanHeroCoverAction(Branth, Second, One, ERTHexDirection::NE);
	RunEnvTurn(TM);

	TestEqual(TEXT("destinazione occupata: rifiutato"),
		CountEnvOutcome(TM, ERTEnvironmentOutcome::CoverRejected), 1);
	TestEqual(TEXT("la copertura e' tornata dov'era, con la sua integrita'"),
		CoverIntegrityOn(MapActor->MapAsset, One, ERTHexDirection::W), 22);

	DestroyEnvWorld(World);
	return true;
}

/**
 * CP 9.5 — `Gadget.PortableCover` erige la stessa copertura, in mano a chi non e' Branth.
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
	URTEquipmentData* Aevik = URTCatalogLibrary::MakePortableCoverGadget();
	if (!TestNotNull(TEXT("il gadget esiste"), Aevik)) { return false; }

	// Passa il validator del catalogo: lo svantaggio e' dichiarato, non sottinteso.
	TArray<const URTEquipmentData*> Set;
	Set.Add(Aevik);
	TestEqual(TEXT("il gadget e' valido a catalogo"), URTCatalogLibrary::ValidateEquipment(Set).Num(), 0);
	TestEqual(TEXT("cooldown 3, come ogni gadget"), Aevik->CooldownTurns, 3);
	TestFalse(TEXT("dichiara uno svantaggio"), Aevik->Drawback.IsEmpty());

	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World);

	const FRTCellId Home(0, 0);
	const FRTCellId Target(1, 0);

	// Un'unita' QUALUNQUE: non ha il kit di Branth, ha solo il gadget.
	ARTUnit* Carrier = SpawnEnvUnit(World, 0, Home);
	ARTUnit* Foe = SpawnEnvUnit(World, 1, FRTCellId(-4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Carrier"), Carrier) || !TestNotNull(TEXT("Foe"), Foe) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyEnvWorld(World);
		return false;
	}

	URTActionData* FromGadget = URTCatalogLibrary::MakeEquipmentAction(Aevik, Carrier);
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
 * Non e' un caso limite: due Branth, o un Branth e un alleato con `Gadget.PortableCover`, che rinforzano lo
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

	URTHeroData* Muiren = URTHeroCatalogLibrary::MakeMuiren();
	URTActionData* MistVeil = (Muiren && Muiren->Actions.IsValidIndex(3)) ? Muiren->Actions[3] : nullptr;
	if (!TestNotNull(TEXT("MistVeil nel kit di Muiren"), MistVeil)) { DestroyEnvWorld(World); return false; }

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

/**
 * `Actions.Hazard.BurningLeavesACanonicalEntry` — il danno da fuoco entra nel TurnLog (`#625`).
 *
 * 🔴 Fino al 2026-08-16 il danno da `Status.Burning` esisteva **solo** in `AddLogEvent`: un `UE_LOG` piu'
 * un buffer circolare troncato, che non e' la traccia. Chi riproduceva la partita vedeva gli HP scendere
 * senza un evento che lo spiegasse, e `DescribeFirstDivergence` non poteva nominare quel punto — il
 * difetto che il gate `replay_representable` ha trovato.
 *
 * ⚠️ **Si fa bruciare un'unita' sul percorso vero e si legge `GetTurnLog()`**: e' un requisito del DoD, e
 * la ragione e' che una voce costruita a mano proverebbe che la struct si compila, non che qualcuno la
 * scrive. Qui il fuoco lo accende `Action.Ignite` e il danno arriva nel Cleanup, come in partita.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHazardBurningLogTest,
	"RefactorTactics.Actions.Hazard.BurningLeavesACanonicalEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHazardBurningLogTest::RunTest(const FString&)
{
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	SpawnEnvMap(World);
	ARTUnit* Caster = SpawnEnvUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Target = SpawnEnvUnit(World, 1, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!Caster || !Target || !TM) { DestroyEnvWorld(World); return false; }

	// 🔴 **I punti vita si fissano qui e non si ereditano dal catalogo eroi**, e la prima stesura li
	// lasciava al default di `ConfigureFromHeroData` (90). L'asserzione `Hit` sopravviveva solo perche'
	// 90 − 10 (fuoco all'ingresso) − 8 (Cleanup) resta positivo: un ribilanciamento che portasse quell'eroe
	// sotto i 18 HP avrebbe fatto flippare questo test su `Lethal`, per una modifica in un altro file.
	// Il ramo che si verifica lo sceglie il test. Trovato in code review.
	Target->Shield = 0;
	Target->Health = 60;

	const int32 HpPrima = Target->Health;
	PlanEnvAction(Caster, TEXT("Action.Ignite"), Target);
	RunEnvTurn(TM);

	// Premessa: se non brucia, il resto non prova niente.
	const bool bBruciato = Target->Health < HpPrima && Target->HasStatus(TAG_Status_Burning);
	if (!TestTrue(TEXT("premessa: l'unita' brucia davvero"), bBruciato))
	{
		DestroyEnvWorld(World);
		return false;
	}

	// La voce canonica: categoria, causa, soggetto, quantita'.
	int32 Trovate = 0;
	FRTTurnLogEntry Voce;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		// ⚠️ La categoria fa parte del selettore da #1077: il tag da solo non identifica piu' una voce
		// sola, perche' nascita/revoca/scadenza dello stato lo portano anch'esse. Qui si cerca il DANNO.
		if (E.Category == ERTLogCategory::Combat && E.ActionId == FName(TEXT("Status.Burning")))
		{
			++Trovate;
			Voce = E;
		}
	}

	if (TestEqual(TEXT("una voce di Burning nel TurnLog"), Trovate, 1))
	{
		TestEqual(TEXT("nel Cleanup"), Voce.Phase, ERTMatchPhase::Cleanup);
		// ⚠️ `Combat` e non `Environment`: la domanda e' «quanti punti vita, e a chi» — la stessa per cui
		// `Healed` sta fra gli esiti di combattimento. La CAUSA la porta `ActionId`, ed e' li' che questo
		// danno si distingue da un colpo.
		TestEqual(TEXT("categoria Combat"), Voce.Category, ERTLogCategory::Combat);
		TestEqual(TEXT("il danno dichiarato dal catalogo"), Voce.Amount,
			URTCombatLibrary::BurningCleanupDamage);
		// 🔴 Il soggetto e' chi SUBISCE: in un danno da hazard non c'e' un attaccante, e `0` direbbe
		// «nessuna unita' dichiarata» su un evento che ne ha una sola.
		TestEqual(TEXT("il soggetto e' chi brucia"), Voce.UnitId, Target->StableUnitId);
		TestNotEqual(TEXT("e non e' lo zero del «nessuno»"), Voce.UnitId, 0);
		TestEqual(TEXT("non letale: Hit"), Voce.Outcome, (uint8)ERTCombatOutcome::Hit);
	}

	DestroyEnvWorld(World);
	return true;
}

/**
 * `Actions.Hazard.BurningDeathIsNotSilent` — chi muore bruciato lascia una traccia.
 *
 * 🔴 E' il caso peggiore del difetto: il `continue` che salta l'unita' morta la faceva **sparire in
 * silenzio**, e un replay vedeva un'unita' in meno senza un evento che lo dicesse.
 *
 * ⚠️ La morte la porta l'**`Outcome`** della stessa voce, non una seconda voce: `Lethal` distingue gia'
 * l'eliminazione dal danno che non uccide, e due voci direbbero due volte lo stesso fatto — lo stesso
 * motivo per cui l'attacco letale non ne scrive una seconda.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHazardBurningLethalLogTest,
	"RefactorTactics.Actions.Hazard.BurningDeathIsNotSilent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHazardBurningLethalLogTest::RunTest(const FString&)
{
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	SpawnEnvMap(World);
	ARTUnit* Caster = SpawnEnvUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Target = SpawnEnvUnit(World, 1, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!Caster || !Target || !TM) { DestroyEnvWorld(World); return false; }

	// 🔴 **I numeri sono scelti perche' l'unita' muoia del danno GIUSTO**, e la prima stesura li aveva
	// sbagliati: con `Health = 1` moriva ai **10** danni immediati della superficie `Fire`, senza mai
	// arrivare al Cleanup — quindi zero voci di `Burning`, e la premessa `!IsAlive()` non se ne accorgeva
	// perche' una morte vale l'altra per quell'asserzione.
	//   `Fire`  -> 10 danni subito + `Burning 2`   (`RTTerrainLibrary.cpp`)
	//   Cleanup ->  8 danni                        (`URTCombatLibrary::BurningCleanupDamage`)
	// `Health = 12` cade nell'unica finestra che serve: sopravvive al primo (12 - 10 = 2) e muore del
	// secondo (2 - 8 < 0). Lo scudo va a zero, o assorbirebbe il colpo e il caso letale non si darebbe.
	Target->Shield = 0;
	Target->Health = 12;

	PlanEnvAction(Caster, TEXT("Action.Ignite"), Target);
	RunEnvTurn(TM);

	if (!TestFalse(TEXT("premessa: l'unita' e' morta"), Target->IsAlive()))
	{
		DestroyEnvWorld(World);
		return false;
	}

	TestEqual(TEXT("una voce letale, con il suo soggetto"),
		CountBurningEntries(TM, Target->StableUnitId, ERTCombatOutcome::Lethal), 1);
	// ⚠️ E **una sola** voce in tutto: la morte la porta l'`Outcome`, non una seconda riga. Contare anche
	// le non letali distingue «ha scritto `Lethal`» da «ha scritto due voci, una delle quali `Lethal`» —
	// che e' la scelta di modello dichiarata nel codice, e senza questa riga non sarebbe pinnata.
	TestEqual(TEXT("e nessuna voce Hit per lo stesso fatto"),
		CountBurningEntries(TM, Target->StableUnitId, ERTCombatOutcome::Hit), 0);

	DestroyEnvWorld(World);
	return true;
}

/**
 * `Actions.Hazard.BurningAbsorbedByShieldIsNotAHit` — il terzo ramo, che nessuno copriva.
 *
 * 🔴 **Questo test nasce da un difetto che i primi due non potevano vedere**: entrambi lasciavano lo
 * scudo a `0`, quindi l'esito non poteva mai essere `ShieldAbsorbed` — e proprio quel ramo era **scritto
 * male**. Confrontava la salute dopo il colpo con `MaxHealth` invece che con quella prima, cosi'
 * un'unita' gia' ferita il cui scudo assorbiva tutto finiva nella traccia come `Hit` per 8 danni che non
 * aveva preso. La traccia — la cosa che `#625` esiste per rendere autorevole — avrebbe mentito.
 * Trovato in code review.
 *
 * ⚠️ Serve uno scudo che regga **entrambi** i colpi del turno: 10 all'ingresso piu' 8 nel Cleanup.
 * Con meno, il primo lo consuma e il secondo arriva agli HP — e si tornerebbe a misurare `Hit`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHazardBurningShieldedLogTest,
	"RefactorTactics.Actions.Hazard.BurningAbsorbedByShieldIsNotAHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHazardBurningShieldedLogTest::RunTest(const FString&)
{
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	SpawnEnvMap(World);
	ARTUnit* Caster = SpawnEnvUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Target = SpawnEnvUnit(World, 1, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!Caster || !Target || !TM) { DestroyEnvWorld(World); return false; }

	// Ferita **e** protetta: e' la combinazione che il confronto con `MaxHealth` sbagliava.
	//
	// ⚠️ Lo scudo dev'essere **TEMPORANEO**, e dal 2026-08-28 non e' un dettaglio ([D-224]): lo scudo BASE
	// non assorbe piu' il danno ambientale, quindi un `Shield = 30` scritto a mano lascerebbe passare
	// entrambi i colpi e questo test tornerebbe a misurare `Hit` — cioe' l'esatto ramo che NON vuole.
	// Il temporaneo continua ad assorbire qualunque sorgente, ed e' quello che tiene vivo `ShieldAbsorbed`.
	Target->Health = 40;
	Target->AddTemporaryShield(30); // 10 all'ingresso + 8 nel Cleanup, e ne avanza

	const int32 HpPrima = Target->Health;
	PlanEnvAction(Caster, TEXT("Action.Ignite"), Target);
	RunEnvTurn(TM);

	if (!TestTrue(TEXT("premessa: brucia"), Target->HasStatus(TAG_Status_Burning)))
	{
		DestroyEnvWorld(World);
		return false;
	}
	if (!TestEqual(TEXT("premessa: lo scudo ha retto, gli HP non sono scesi"), Target->Health, HpPrima))
	{
		DestroyEnvWorld(World);
		return false;
	}

	TestEqual(TEXT("l'esito e' ShieldAbsorbed"),
		CountBurningEntries(TM, Target->StableUnitId, ERTCombatOutcome::ShieldAbsorbed), 1);
	TestEqual(TEXT("e NON Hit: nessun HP e' stato perso"),
		CountBurningEntries(TM, Target->StableUnitId, ERTCombatOutcome::Hit), 0);

	DestroyEnvWorld(World);
	return true;
}

/**
 * `Actions.Hazard.TerrainDamageLeavesACanonicalEntry` — il danno **all'ingresso** entra nel TurnLog
 * (`#1067`).
 *
 * 🔴 Gemello di `#625`, e il pezzo **piu' grosso dei due**: `Fire` fa **10** danni a chi ci entra contro
 * gli **8** del Cleanup. Fino al 2026-08-16 esisteva solo in `AddLogEvent` — un `UE_LOG` piu' un buffer
 * circolare troncato — quindi il replay vedeva gli HP scendere senza un evento che lo spiegasse.
 * Misurabile sul test di `#625`: il bersaglio scendeva `90 → 80 → 72`, diciotto danni, e otto tracciati.
 *
 * ⚠️ Si **cammina** dentro il fuoco sul percorso vero — `PlannedPath` e `RunEnvTurn` — invece di chiamare
 * la funzione: e' un requisito del DoD, e la ragione e' che la fase dichiarata dalla voce dipende da
 * QUALE dei quattro siti la chiama. Una chiamata diretta non lo proverebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHazardTerrainEntryLogTest,
	"RefactorTactics.Actions.Hazard.TerrainDamageLeavesACanonicalEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHazardTerrainEntryLogTest::RunTest(const FString&)
{
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World);
	if (!TestNotNull(TEXT("mappa"), MapActor)) { DestroyEnvWorld(World); return false; }

	// Una cella di fuoco sul percorso. La si accende nel DATO, non con un'azione: qui il soggetto e' il
	// terreno che c'e' gia', non chi lo crea.
	FRTHexCellData Fuoco(FRTCellId(1, 0));
	Fuoco.Surface = ERTHexSurface::Fire;
	MapActor->MapAsset->AddOrUpdateCell(Fuoco);
	MapActor->MapAsset->SortCells();

	ARTUnit* Mover = SpawnEnvUnit(World, 0, FRTCellId(0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!Mover || !TM) { DestroyEnvWorld(World); return false; }

	// Punti vita fissati qui e non ereditati dal catalogo eroi: e' il ramo `Hit` che si vuole verificare,
	// e un ribilanciamento non deve poterlo far diventare `Lethal` da un altro file.
	Mover->Shield = 0;
	Mover->Health = 60;
	Mover->PlannedAbilityIndex = INDEX_NONE; // nessuna azione: l'unica fonte di danno e' il terreno
	Mover->PlannedPath = { FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0) };
	Mover->PlannedCell = FRTCellId(2, 0);

	RunEnvTurn(TM);

	if (!TestTrue(TEXT("premessa: attraversando il fuoco ha perso HP"), Mover->Health < 60))
	{
		DestroyEnvWorld(World);
		return false;
	}

	int32 Trovate = 0;
	FRTTurnLogEntry Voce;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.ActionId == FName(TEXT("Terrain.Fire")))
		{
			++Trovate;
			Voce = E;
		}
	}

	if (TestEqual(TEXT("una voce di Terrain.Fire nel TurnLog"), Trovate, 1))
	{
		// ⚠️ **`Move` e non `Cleanup`**: e' il danno dell'INGRESSO, e si distingue da quello del `Burning`
		// per fase **e** per `ActionId`. Se la fase fosse letta dal membro `Muiren` sarebbe sbagliata — il
		// ciclo delle fasi esce su `Planning` e la Cleanup gira dopo.
		TestEqual(TEXT("nella fase del movimento"), Voce.Phase, ERTMatchPhase::Move);
		TestEqual(TEXT("categoria Combat"), Voce.Category, ERTLogCategory::Combat);
		TestEqual(TEXT("il danno dichiarato dal catalogo terreni"), Voce.Amount, 10);
		TestEqual(TEXT("il soggetto e' chi ci e' entrato"), Voce.UnitId, Mover->StableUnitId);
		TestNotEqual(TEXT("e non lo zero del «nessuno»"), Voce.UnitId, 0);
		// La cella che ha colpito: qui **e' davvero la causa**, al contrario del `Burning` che segue
		// l'unita' anche fuori dal fuoco.
		TestEqual(TEXT("la cella e' quella in fiamme"), Voce.SrcCell, FRTCellId(1, 0));
		TestEqual(TEXT("non letale: Hit"), Voce.Outcome, (uint8)ERTCombatOutcome::Hit);
	}

	// ⚠️ E le DUE voci del fuoco convivono, distinte: l'ingresso e il Cleanup. E' il motivo per cui la
	// causa sta in `ActionId` e non nella categoria — senza, il replay avrebbe due danni indistinguibili.
	int32 Burning = 0;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		// #1077: solo il DANNO, non i tre momenti della vita dello stato, che portano lo stesso tag.
		if (E.Category == ERTLogCategory::Combat && E.ActionId == FName(TEXT("Status.Burning"))) { ++Burning; }
	}
	TestEqual(TEXT("e accanto c'e' quella del Burning, distinta"), Burning, 1);

	DestroyEnvWorld(World);
	return true;
}

/**
 * `Actions.Hazard.TerrainDeathIsNotSilent` — chi muore **entrando** lascia una traccia.
 *
 * 🔴 Era il caso peggiore del difetto: un'unita' con pochi HP spinta o mossa su una cella di fuoco moriva
 * dentro `ApplyTerrainOnEnterEffects` **senza lasciare niente** — nessun `Lethal`, nessun soggetto, e per
 * `DescribeFirstDivergence` nessun punto da nominare. Un'unita' spariva dal campo e il replay non poteva
 * dire perche'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHazardTerrainDeathLogTest,
	"RefactorTactics.Actions.Hazard.TerrainDeathIsNotSilent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHazardTerrainDeathLogTest::RunTest(const FString&)
{
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World);
	if (!TestNotNull(TEXT("mappa"), MapActor)) { DestroyEnvWorld(World); return false; }

	FRTHexCellData Fuoco(FRTCellId(1, 0));
	Fuoco.Surface = ERTHexSurface::Fire;
	MapActor->MapAsset->AddOrUpdateCell(Fuoco);
	MapActor->MapAsset->SortCells();

	ARTUnit* Mover = SpawnEnvUnit(World, 0, FRTCellId(0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!Mover || !TM) { DestroyEnvWorld(World); return false; }

	// ⚠️ Sotto i **10** danni dell'ingresso, cosi' che a ucciderla sia QUEL danno e non il `Burning` del
	// Cleanup: e' la stessa attenzione che `#625` ha dovuto imparare al contrario, dove `Health = 1`
	// faceva morire l'unita' all'ingresso invece che nel Cleanup.
	Mover->Shield = 0;
	Mover->Health = 6;
	Mover->PlannedAbilityIndex = INDEX_NONE;
	Mover->PlannedPath = { FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0) };
	Mover->PlannedCell = FRTCellId(2, 0);

	RunEnvTurn(TM);

	if (!TestFalse(TEXT("premessa: e' morta"), Mover->IsAlive()))
	{
		DestroyEnvWorld(World);
		return false;
	}

	int32 Letali = 0;
	int32 NonLetali = 0;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.ActionId == FName(TEXT("Terrain.Fire")) && E.UnitId == Mover->StableUnitId)
		{
			(E.Outcome == (uint8)ERTCombatOutcome::Lethal ? Letali : NonLetali) += 1;
		}
	}
	TestEqual(TEXT("una voce letale, col suo soggetto"), Letali, 1);
	TestEqual(TEXT("e nessuna seconda voce per lo stesso fatto"), NonLetali, 0);

	// ⚠️ E **nessuna** voce di `Burning`: e' morta prima di arrivarci. Senza questa riga il test resterebbe
	// verde anche se il `Lethal` arrivasse dal Cleanup invece che dall'ingresso — cioe' misurando l'altro
	// difetto, gia' chiuso.
	int32 Burning = 0;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		// #1077: solo il DANNO, non i tre momenti della vita dello stato, che portano lo stesso tag.
		if (E.Category == ERTLogCategory::Combat && E.ActionId == FName(TEXT("Status.Burning"))) { ++Burning; }
	}
	TestEqual(TEXT("morta all'ingresso, non nel Cleanup"), Burning, 0);

	DestroyEnvWorld(World);
	return true;
}

/**
 * `Actions.Hazard.SufferedAndInflictedAreTellableApart` — lo stesso `UnitId`, nello stesso turno, subisce il
 * fuoco **e** colpisce qualcuno: le due voci devono essere distinguibili (`#1150`).
 *
 * 🔴 **E' il difetto latente reso falsificabile.** `UnitId` significa «chi ha AGITO» (`AppendLogEntry`), ma
 * il danno ambientale lo **inverte** e ci mette chi subisce — deliberatamente, perche' in un hazard non c'e'
 * un attaccante e lo `0` direbbe «nessuno» su un evento che un soggetto ce l'ha. Finche' le due voci non
 * convivono su una stessa unita', l'inversione non ha conseguenze visibili; qui convivono, e chi somma il
 * danno *inflitto* per `UnitId` filtrando su `Category == Combat` ottiene un numero **plausibile e
 * sbagliato** che nessun errore segnala.
 *
 * ⚠️ **La domanda si fa con il predicato, non con le celle.** Entrambe le voci ambientali hanno
 * `SrcCell == TgtCell`, ed e' vero — ma `AppendLogEntry` dichiara con tre controesempi che `SrcCell` non
 * identifica l'unita', e costruirci sopra un filtro sarebbe l'inferenza che il formato ha smesso di
 * sostenere quando `UnitId` e' nato (`D-063`).
 *
 * ⚠️ Percorso vero: il fuoco lo accende `Action.Ignite` e il danno arriva nel Cleanup, come in partita.
 * Una voce costruita a mano proverebbe che la struct si compila, non che qualcuno la scrive.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHazardSufferedVsInflictedTest,
	"RefactorTactics.Actions.Hazard.SufferedAndInflictedAreTellableApart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHazardSufferedVsInflictedTest::RunTest(const FString&)
{
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	SpawnEnvMap(World);
	ARTUnit* Incendiario = SpawnEnvUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Bruciato = SpawnEnvUnit(World, 1, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!Incendiario || !Bruciato || !TM) { DestroyEnvWorld(World); return false; }

	// Salute fissata dal test e non ereditata dal catalogo: entrambi devono SOPRAVVIVERE al turno, altrimenti
	// una delle due voci non nasce e il confronto non esiste. Stessa cura di `BurningLeavesACanonicalEntry`.
	//
	// ⚠️ **Dentro il range del catalogo** (`MakeIvrin` da' `MaxHealth = 90`): la prima stesura metteva 200 e
	// creava uno stato che il gioco non puo' produrre — e `ApplyCombatState` clampa solo lo zero, quindi
	// 200/90 attraversava tutto il turno. Il ramo `ShieldAbsorbed` confronta con `MaxHealth`. I danni reali
	// sono 18 e 21, quindi 60 basta con margine. Trovato in code review.
	Incendiario->Shield = 0;
	Incendiario->Health = 60;
	Bruciato->Shield = 0;
	Bruciato->Health = 60;

	// Nello stesso turno: l'uno incendia l'altro, e l'altro colpisce con l'attacco base (slot 0).
	PlanEnvAction(Incendiario, TEXT("Action.Ignite"), Bruciato);
	Bruciato->PlannedAbilityIndex = 0;
	Bruciato->PlannedAttackTarget = Incendiario;
	RunEnvTurn(TM);

	int32 Inflitte = 0;
	int32 SommaSubita = 0;
	int32 SommaInflitta = 0;
	int32 Residuo = 0;               // voci col mio `UnitId` che non sono ne' l'uno ne' l'altro
	FRTTurnLogEntry VoceTerreno;
	FRTTurnLogEntry VoceStato;
	FRTTurnLogEntry VoceInflitta;
	bool bCausaTerreno = false;
	bool bCausaStato = false;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.UnitId != Bruciato->StableUnitId) { continue; }
		if (URTTurnLogLibrary::IsEnvironmentalDamage(E))
		{
			SommaSubita += E.Amount;
			// ⚠️ Le due voci si tengono SEPARATE. La prima stesura ne conservava una sola, sovrascritta a
			// ogni giro: le asserzioni sui campi guardavano la superstite e l'altra non la vedeva nessuno.
			// Trovato in code review.
			if (E.ActionId.ToString().StartsWith(URTTurnLogLibrary::TerrainCausePrefix()))
			{
				bCausaTerreno = true;
				VoceTerreno = E;
			}
			else
			{
				bCausaStato = true;
				VoceStato = E;
			}
		}
		else if (URTTurnLogLibrary::IsDamageInflictedByActor(E))
		{
			++Inflitte;
			SommaInflitta += E.Amount;
			VoceInflitta = E;
		}
		else if (E.Category == ERTLogCategory::Combat)
		{
			++Residuo;
		}
	}

	AddInfo(FString::Printf(TEXT("UnitId=%d · subito=%d inflitto=%d (voci %d) · residuo Combat=%d"),
		Bruciato->StableUnitId, SommaSubita, SommaInflitta, Inflitte, Residuo));

	// ➕ **Le voci ambientali sono DUE, e la scoperta e' del test**: accendere il fuoco su una cella occupata
	// produce il danno d'ingresso `Terrain.Fire` (`#1067`) **e** il tick di `Status.Burning` (`#625`). La
	// prima stesura ne asseriva una e cadeva li'. Averle entrambe rende questo test la copertura di **tutta**
	// la tassonomia di `IsEnvironmentalDamage`, non di meta'.
	if (!TestTrue(TEXT("premessa: il predicato riconosce la causa di TERRENO"), bCausaTerreno)
		|| !TestTrue(TEXT("premessa: e la causa di STATO"), bCausaStato)
		|| !TestTrue(TEXT("premessa: almeno una voce inflitta sulla stessa unita'"), Inflitte >= 1))
	{
		DestroyEnvWorld(World);
		return false;
	}

	// I RUOLI sono opposti, e si vedono nel bersaglio: le due ambientali colpiscono chi le subisce, quella
	// inflitta colpisce l'altro. E' l'osservabile, non una proprieta' strutturale dei predicati.
	TestEqual(TEXT("il danno da terreno ha per bersaglio chi lo subisce"), VoceTerreno.TgtCell, Bruciato->Cell);
	TestEqual(TEXT("e cosi' il tick di stato"), VoceStato.TgtCell, Bruciato->Cell);
	TestEqual(TEXT("mentre la voce inflitta ha per bersaglio l'altro"), VoceInflitta.TgtCell, Incendiario->Cell);

	// 🔴 **L'asserto che rende il difetto falsificabile.** Senza il predicato la somma per `UnitId` include il
	// danno che l'unita' ha SUBITO, e il numero resta plausibile: nessuna eccezione, nessun valore assurdo.
	//
	// ⚠️ **Il residuo si asserisce, non si assume.** I due predicati si escludono ma **non partizionano**:
	// `Healed` e `NoLineOfSight` non soddisfano nessuno dei due. Senza questa riga, l'aritmetica qui sotto
	// direbbe «la differenza e' il danno subito» anche quando la differenza fosse una cura. Trovato in code
	// review.
	TestEqual(TEXT("in questo allestimento non ci sono voci Combat fuori dai due predicati"), Residuo, 0);
	TestTrue(*FString::Printf(
		TEXT("la somma ingenua sopravvaluta l'inflitto: %d contro %d (differenza %d, il danno subito)"),
		SommaSubita + SommaInflitta, SommaInflitta, SommaSubita), SommaSubita > 0);

	DestroyEnvWorld(World);
	return true;
}

// =====================================================================================================
// `#2828` — il colpo alla STRUTTURA ha un istante in cui essere mostrato.
//
// I quattro gate end-to-end vivono QUI e non in un file loro perche' gli helper che producono un colpo a
// copertura **giocato** — `PlanCoverAction`, `SpawnEnvUnit`, `RunEnvTurn` — stanno in un namespace anonimo
// di questo file, e ricrearli altrove ne farebbe una seconda copia. ⚠️ Le famiglie restano quelle che la
// issue prescrive (`Turn.`, `Playback.`, `Replay.`): il filtro di Automation guarda il nome, non il file.
//
// 🔴 **Tutti e quattro misurano una traccia GIOCATA, mai una voce costruita a mano.** E' la correzione
// che `#1933` ha dovuto fare al proprio test e che `#2341` ha poi chiesto per iscritto: un evento
// assemblato dentro il test prova che la struct esiste, non che qualcuno la produca.
//
// ⚠️ **Il buco che chiudono era anche nei test, non solo nel codice**: misurato prima di scriverli, un
// solo test in tutto `Source/` asseriva `CoverDamaged` in un turno giocato
// (`Cover.Destruction.LoggedInPlayedTurn`) e **nessuno** asseriva `CoverDestroyed` — c'e' chi abbatte un
// muro, ma misura l'integrita' residua sulla mappa, non cio' che ne viene raccontato.
// =====================================================================================================

namespace
{
	/**
	 * Lo scenario condiviso dei quattro gate: una copertura eretta e un colpo che l'attraversa.
	 *
	 * `First` erige su `Shielded` verso E (integrita' 30, dal catalogo); `Breacher`, oltre quel bordo, spara
	 * al `Defender` che sta sulla cella riparata con `StructurePower = InStructurePower`.
	 *
	 * ⚠️ **Il `Defender` c'e' apposta e non e' arredamento**: e' l'unita' che un consumatore sbagliato
	 * metterebbe in `TargetStableUnitId` al posto del bordo. Senza qualcuno su quella cella,
	 * `StructureHitEventCarriesEdgeNotActor` sarebbe verde per assenza.
	 *
	 * ⚠️ Prefisso `Env`, come gli altri helper di questo file: unity build, i namespace anonimi si fondono.
	 */
	struct FRTEnvBreachScenario
	{
		UWorld* World = nullptr;
		ARTHexMapActor* MapActor = nullptr;
		ARTTurnManager* TM = nullptr;
		ARTUnit* Breacher = nullptr;
		ARTUnit* Defender = nullptr;
		FRTCellId Shielded{0, 0};
		FRTCellId Attacker{1, 0};
		bool bValid = false;
	};

	FRTEnvBreachScenario EnvMakeBreachScenario(int32 InStructurePower)
	{
		FRTEnvBreachScenario S;
		S.World = MakeEnvWorld();
		if (!S.World) { return S; }
		S.MapActor = SpawnEnvMap(S.World);

		ARTUnit* First = SpawnEnvUnit(S.World, 0, FRTCellId(0, 1));
		S.Defender = SpawnEnvUnit(S.World, 0, S.Shielded);
		S.Breacher = SpawnEnvUnit(S.World, 1, S.Attacker);
		S.TM = S.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!S.MapActor || !First || !S.Defender || !S.Breacher || !S.TM) { return S; }

		// La copertura nasce e incassa nello stesso Blast: e' la forma gia' usata da
		// `Structures.KineticPanel.DestroyedCoverLeavesNoGhost`, in questo stesso file.
		PlanCoverAction(First, TEXT("Action.CreateCover"), S.Shielded, ERTHexDirection::E);
		// La capacita' di sfondare si dichiara sull'istanza: e' l'idioma di tutti i test di struttura, e la
		// ragione e' che l'archetipo di prova non ha fra le sue l'unica azione core che sfonda
		// (`Action.HeavyAttack`, l'invariante lo pinna `Cover.Destruction.HeavyAttackDeclaresStructureDamage`).
		S.Breacher->Abilities[0]->Def.Effects.Add(
			FRTActionEffectSpec(ERTActionEffect::DamageStructure, InStructurePower));
		S.Breacher->PlannedAbilityIndex = 0;
		S.Breacher->PlannedAttackTarget = S.Defender;

		S.bValid = true;
		return S;
	}

	/**
	 * Lo scenario del muro **ALTO**: il colpo non arriva a nessuno, e la barriera incassa lo stesso.
	 *
	 * 🔴 **E' il caso PRINCIPALE della issue, e quello dell'altro scenario non lo copre.**
	 * `Action.CreateCover` erige una copertura **bassa** (integrita' 30), che non toglie la linea di tiro:
	 * il colpo arriva al difensore, produce un `Attack`, e la fase `Blast` nascerebbe comunque. Qui il muro
	 * e' **alto**, quindi `LineOfSightPolicy::Required` non e' soddisfatta e l'intento finisce in
	 * `BlockedIntents`.
	 *
	 * 🔑 **La sequenza nel resolver e' cio' che rende il caso possibile**, ed e' misurata:
	 * `RTHexCombatLibrary.cpp:391` raccoglie il danno alla struttura **prima** del controllo sulla linea di
	 * tiro (`:560`), che fa `continue` **prima** dell'impronta (`:606`). ∴ la barriera incassa, e non
	 * nascono ne' `Attack` ne' `AttackFootprint`: e' l'unica combinazione in cui la fase `Blast` dipende
	 * davvero dal quarto termine di `BlastPhaseIsActive`.
	 *
	 * Muro alto (integrita' 50) sul bordo W di (1,0), attaccante in (0,0), bersaglio dietro in (2,0):
	 * la stessa scena di `Cover.Destruction.LoggedInPlayedTurn`, che la usa per il TurnLog.
	 */
	FRTEnvBreachScenario EnvMakeWalledBreachScenario(int32 InStructurePower)
	{
		FRTEnvBreachScenario S;
		S.World = MakeEnvWorld();
		if (!S.World) { return S; }
		S.MapActor = SpawnEnvMap(S.World);
		if (!S.MapActor || !S.MapActor->MapAsset) { return S; }

		S.Shielded = FRTCellId(1, 0);   // la cella che PORTA il muro
		S.Attacker = FRTCellId(0, 0);   // oltre il bordo W

		const FRTHexCellData* Esistente = S.MapActor->MapAsset->FindCell(S.Shielded);
		if (!Esistente) { return S; }
		// ⚠️ Si parte dalla cella ESISTENTE: costruirne una nuova con lo stesso `Id` sostituirebbe quella
		// che l'arena ha posato, perdendone terreno e proprieta'.
		FRTHexCellData ColMuro = *Esistente;
		ColMuro.Covers.Add(FRTHexCover(ERTHexDirection::W, ERTHexCoverType::High,
			FRTHexCover::DefaultIntegrity(ERTHexCoverType::High)));
		S.MapActor->MapAsset->AddOrUpdateCell(ColMuro);
		S.MapActor->MapAsset->SortCells();

		S.Breacher = SpawnEnvUnit(S.World, 1, S.Attacker);
		S.Defender = SpawnEnvUnit(S.World, 0, FRTCellId(2, 0));
		S.TM = S.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!S.Breacher || !S.Defender || !S.TM) { return S; }

		S.Breacher->Abilities[0]->Def.Effects.Add(
			FRTActionEffectSpec(ERTActionEffect::DamageStructure, InStructurePower));
		S.Breacher->PlannedAbilityIndex = 0;
		S.Breacher->PlannedAttackTarget = S.Defender;

		S.bValid = true;
		return S;
	}

	/** Quanti eventi di timeline di un dato tipo. */
	int32 EnvCountTimelineType(const ARTTurnManager* TM, ERTResolvedEventType Type)
	{
		int32 N = 0;
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == Type) { ++N; }
		}
		return N;
	}

	/** Gli eventi di timeline che sono colpi a struttura. */
	TArray<FRTResolvedEvent> EnvStructureHitEvents(const ARTTurnManager* TM)
	{
		TArray<FRTResolvedEvent> Out;
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::StructureHit) { Out.Add(Ev); }
		}
		return Out;
	}

	/** Cio' che una riproduzione ha mostrato, e quanto e' durata. */
	struct FRTEnvPlaybackProbe
	{
		TArray<FRTPlaybackStructureHit> Mostrati; // al PICCO, non alla fine
		int32 Tick = 0;
		bool bAppesa = false;
	};

	/**
	 * Risolve e riproduce, campionando i colpi a struttura **al PICCO** della rivelazione.
	 *
	 * 🔑 **Al picco e non alla fine**, perche' `FinishPlayback` spegne il canale: leggere dopo l'uscita dal
	 * ciclo darebbe sempre zero, e un gate che confronta due zeri e' verde per costruzione.
	 *
	 * ⚠️ Il tetto di giri e' un tetto, non un'attesa: se lo si tocca la risoluzione non ha chiuso, ed e'
	 * `bAppesa` a dirlo invece di lasciare il test verde su una riproduzione monca.
	 */
	FRTEnvPlaybackProbe EnvRunPlaybackProbingStructureHits(ARTTurnManager* TM, const ARTHexMapActor* MapActor)
	{
		FRTEnvPlaybackProbe Probe;
		TM->LockInAndResolve();
		if (MapActor) { Probe.Mostrati = MapActor->GetPlaybackStructureHits(); }
		for (; Probe.Tick < 2000 && TM->IsResolving(); ++Probe.Tick)
		{
			TM->Tick(0.02f);
			if (MapActor && MapActor->GetPlaybackStructureHits().Num() > Probe.Mostrati.Num())
			{
				Probe.Mostrati = MapActor->GetPlaybackStructureHits();
			}
		}
		Probe.bAppesa = TM->IsResolving();
		return Probe;
	}
}

/**
 * I due canali raccontano lo stesso fatto e non divergono — `#2828`.
 *
 * 🔴 **E' la proprieta' che l'emissione da `AppendLogEntry` esiste per garantire.** L'evento non nasce
 * nel sito che danneggia la struttura, ma nell'unico punto in cui la voce di TurnLog viene scritta: il
 * secondo canale **deriva** dal primo invece di essere una seconda fonte da tenere allineata a mano.
 * Questo gate misura che la derivazione sia vera su una partita giocata.
 *
 * ⚠️ **Il predicato e' lo STESSO dai due lati, e qui e' corretto che lo sia.** L'affermazione non e'
 * *«`IsStructureHit` classifica bene»* — e' un'altra domanda — ma *«contando le stesse voci, i due canali
 * danno lo stesso numero e lo stesso contenuto»*. Riscrivere il predicato a mano da un lato ne farebbe una
 * seconda copia, cioe' [D-098].
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnStructureHitEventMatchesTurnLogEntryTest,
	"RefactorTactics.Turn.StructureHitEventMatchesTurnLogEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnStructureHitEventMatchesTurnLogEntryTest::RunTest(const FString&)
{
	// Integrita' 30, colpo 10 → **danneggiata**, non abbattuta: e' il ramo `CoverDamaged`, quello che un
	// gate scritto solo sulla distruzione lascerebbe scoperto.
	FRTEnvBreachScenario S = EnvMakeBreachScenario(/*InStructurePower=*/ 10);
	if (!TestTrue(TEXT("scenario costruito"), S.bValid)) { DestroyEnvWorld(S.World); return false; }

	RunEnvTurn(S.TM);

	TArray<FRTTurnLogEntry> Voci;
	for (const FRTTurnLogEntry& E : S.TM->GetTurnLog())
	{
		if (URTTurnLogLibrary::IsStructureHit(E)) { Voci.Add(E); }
	}
	const TArray<FRTResolvedEvent> Eventi = EnvStructureHitEvents(S.TM);

	// ⛔ ANTI-VACUITA': senza questa riga tutto il resto sarebbe vero per assenza — zero voci e zero eventi
	// si corrispondono perfettamente, e il gate resterebbe verde su un produttore che non produce.
	if (!TestTrue(TEXT("⛔ il turno ha prodotto almeno un colpo a struttura"), Voci.Num() > 0))
	{
		DestroyEnvWorld(S.World);
		return false;
	}

	TestEqual(TEXT("✅ tante voci quanti eventi: nessun canale ne perde o ne inventa"),
		Eventi.Num(), Voci.Num());

	// ⛔ **Accoppiati per CONTENUTO, non per indice, e non e' pedanteria.** I due canali hanno ordini
	// diversi per costruzione: `ConcludeTurn` passa il TurnLog da `URTTurnLogLibrary::SortTurnLog`, la cui
	// chiave e' `(TurnNumber, Phase, Priority, Category, …)`, mentre `ResolvedTimeline` conserva l'ordine di
	// emissione, che per le strutture e' quello di `ApplyStructureDamage`. Le due chiavi non hanno relazione.
	//
	// ⚠️ **Con UN solo colpo l'indice funziona e nasconde il difetto**, che e' il caso di questo scenario.
	// Ma appena i colpi diventano due — un'area che investe due coperture, o una barriera dichiarata su
	// entrambe le facce, che `ValidateMap` segnala come **Warning** e quindi ammette — l'accoppiamento per
	// indice fallirebbe su codice CORRETTO, oppure passerebbe mentre i due canali divergono davvero.
	// Entrambi i versi sono sbagliati, e il secondo e' quello che non si vede.
	for (const FRTResolvedEvent& Ev : Eventi)
	{
		const FRTTurnLogEntry* Voce = Voci.FindByPredicate([&Ev](const FRTTurnLogEntry& V)
		{
			return V.SrcCell == Ev.StructureCell && V.TgtCell == Ev.StructureToward;
		});
		if (!TestNotNull(TEXT("ogni evento ha la sua voce, sullo stesso bordo"), Voce))
		{
			continue;
		}
		// ⚠️ `Amount` e' l'integrita' RESIDUA in entrambi i canali. Se un giorno uno dei due passasse al
		// danno inferto, questa riga cadrebbe — ed e' cio' che deve fare.
		TestEqual(TEXT("stessa integrita' residua"), Ev.Amount, Voce->Amount);
		TestEqual(TEXT("stesso esito, senza appiattire [D-175]"),
			static_cast<int32>(Ev.EnvironmentOutcome), static_cast<int32>(Voce->Outcome));
		TestEqual(TEXT("stesso attaccante"), Ev.SourceStableUnitId, Voce->UnitId);
	}

	// ⛔ E il ramo misurato e' proprio `CoverDamaged`: se lo scenario abbattesse la copertura invece di
	// scalfirla, il gate starebbe misurando l'altro caso senza dirlo.
	if (Eventi.Num() > 0)
	{
		TestEqual(TEXT("lo scenario danneggia e non abbatte"),
			static_cast<int32>(Eventi[0].EnvironmentOutcome),
			static_cast<int32>(ERTEnvironmentOutcome::CoverDamaged));
		TestEqual(TEXT("con l'integrita' che il colpo lascia"), Eventi[0].Amount, 20);
	}

	DestroyEnvWorld(S.World);
	return true;
}

/**
 * L'identita' del fatto resta il **BORDO**, non un'unita' — `#2828`.
 *
 * 🔴 **E' il gate di mutazione della issue, e senza il difensore sulla cella riparata sarebbe verde per
 * assenza.** Il modo naturale di sbagliare questo evento e' riempire `TargetStableUnitId` con «l'unita'
 * piu' vicina» o con chi stava dietro il muro: sembra un campo vuoto da completare, e completandolo il
 * fatto smetterebbe di riguardare la struttura. Qui quell'unita' c'e', ha uno `StableUnitId` valido, ed e'
 * anche il bersaglio dichiarato del colpo — cioe' la candidata piu' plausibile per quell'errore.
 *
 * ⛔ Una copertura non ha uno `StableUnitId`: `0` significa «nessuno» ([D-063]), mai «l'unita' zero».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnStructureHitEventCarriesEdgeNotActorTest,
	"RefactorTactics.Turn.StructureHitEventCarriesEdgeNotActor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnStructureHitEventCarriesEdgeNotActorTest::RunTest(const FString&)
{
	FRTEnvBreachScenario S = EnvMakeBreachScenario(/*InStructurePower=*/ 10);
	if (!TestTrue(TEXT("scenario costruito"), S.bValid)) { DestroyEnvWorld(S.World); return false; }

	RunEnvTurn(S.TM);

	const TArray<FRTResolvedEvent> Eventi = EnvStructureHitEvents(S.TM);
	if (!TestTrue(TEXT("⛔ il turno ha prodotto un colpo a struttura"), Eventi.Num() > 0))
	{
		DestroyEnvWorld(S.World);
		return false;
	}

	// ⛔ La premessa del gate: l'unita' che si potrebbe scrivere per sbaglio ESISTE e ha un id vero.
	TestTrue(TEXT("premessa: sulla cella riparata c'e' un'unita' con un id valido"),
		S.Defender && S.Defender->StableUnitId != 0);

	for (const FRTResolvedEvent& Ev : Eventi)
	{
		TestEqual(TEXT("⛔ nessun bersaglio-unita': il soggetto e' il bordo"), Ev.TargetStableUnitId, 0);
		TestNotEqual(TEXT("⛔ e in particolare NON e' il difensore dietro il muro"),
			Ev.TargetStableUnitId, S.Defender->StableUnitId);

		// Il bordo c'e' davvero, ed e' quello colpito.
		TestEqual(TEXT("la cella e' quella riparata"), Ev.StructureCell, S.Shielded);
		TestEqual(TEXT("e il verso e' quello da cui e' arrivato il colpo"), Ev.StructureToward, S.Attacker);

		// 🔑 Chi ha AGITO e' l'attaccante, non chi stava dietro: il verso della riga di stato, non quello
		// dell'hazard. Scambiarli accrediterebbe il colpo a chi lo ha subito.
		TestEqual(TEXT("e la sorgente e' chi ha sparato"),
			Ev.SourceStableUnitId, S.Breacher->StableUnitId);

		// ✅ **L'identita' dell'azione C'E', e qui e' un cambiamento di contratto, non un test aggiornato
		// d'ufficio** — `#3281`, [D-437]. ⏱️ *Questa riga asseriva `Ev.ActionId.IsNone()` come «limite
		// dichiarato»: era la misura del difetto, ed era corretta finche' l'identita' non arrivava fin qui.*
		//
		// 🔑 **Qui l'attaccante e' UNO**, e la regola decisa e' *«si nomina quando l'autore e' uno, si tace
		// quando sono due»*: ∴ su questo scenario il silenzio sarebbe ora il difetto, non il contratto. Il
		// caso in cui `NAME_None` resta — e diventa un confine d'atto — e' l'aggregato, e ha il suo gate
		// (`Playback.AggregatedStructureHitDoesNotNameOneAction`).
		TestFalse(TEXT("✅ l'identita' d'azione c'e': l'autore e' uno solo"), Ev.ActionId.IsNone());
		TestEqual(TEXT("✅ ed e' l'azione di chi ha sparato"),
			Ev.ActionId, S.Breacher->Abilities[0]->Def.ActionId);
	}

	DestroyEnvWorld(S.World);
	return true;
}

/**
 * La presentazione **consuma** l'evento e non ricalcola il bordo — `#2828`.
 *
 * 🔴 **E' il divieto che la issue scrive per intero**, e la sola prova che regge e' il confronto
 * dell'INTERO contenuto: un gate che contasse i segnali resterebbe verde su una presentazione che chiama
 * `FirstCoveredEdge` per conto suo e per caso ne trova uno. Qui si pretende che cio' che e' arrivato al map
 * actor sia **esattamente** cio' che l'evento portava.
 *
 * ⚠️ Il confronto si fa al PICCO della rivelazione: `FinishPlayback` spegne il canale, e leggere dopo la
 * fine darebbe due zeri che si corrispondono.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackStructureHitConsumesResolvedEventTest,
	"RefactorTactics.Playback.StructureHitConsumesResolvedEventWithoutRecomputing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackStructureHitConsumesResolvedEventTest::RunTest(const FString&)
{
	// Colpo 30 su integrita' 30 → **abbattuta** (`RemainingIntegrity <= 0`): cosi' il gate misura anche che
	// `bDestroyed` attraversi il confine invece di essere dedotto a valle da un'integrita' a zero.
	FRTEnvBreachScenario S = EnvMakeBreachScenario(/*InStructurePower=*/ 30);
	if (!TestTrue(TEXT("scenario costruito"), S.bValid)) { DestroyEnvWorld(S.World); return false; }

	const FRTEnvPlaybackProbe Probe = EnvRunPlaybackProbingStructureHits(S.TM, S.MapActor);
	const TArray<FRTResolvedEvent> Eventi = EnvStructureHitEvents(S.TM);

	TestFalse(TEXT("la risoluzione ha chiuso: nessuna riproduzione appesa"), Probe.bAppesa);

	if (!TestTrue(TEXT("⛔ la timeline porta almeno un colpo a struttura"), Eventi.Num() > 0))
	{
		DestroyEnvWorld(S.World);
		return false;
	}
	// ⛔ ANTI-VACUITA', ed e' la meta' che conta: se il playback non avesse MAI consumato l'evento,
	// `Mostrati` resterebbe vuoto e ogni confronto sotto sarebbe vero per assenza. E' esattamente il difetto
	// che il quarto termine di `BlastPhaseIsActive` esiste per impedire — un evento senza una fase in cui
	// accadere non fallisce: sparisce.
	if (!TestTrue(TEXT("⛔ e la presentazione lo ha CONSUMATO: il canale non e' mai rimasto vuoto"),
		Probe.Mostrati.Num() > 0))
	{
		DestroyEnvWorld(S.World);
		return false;
	}

	TestEqual(TEXT("✅ tanti segnali quanti eventi: nessuno perso, nessuno inventato"),
		Probe.Mostrati.Num(), Eventi.Num());

	for (int32 I = 0; I < FMath::Min(Probe.Mostrati.Num(), Eventi.Num()); ++I)
	{
		TestEqual(TEXT("la cella e' COPIATA dall'evento"), Probe.Mostrati[I].Cell, Eventi[I].StructureCell);
		TestEqual(TEXT("e il verso anche: il bordo non e' stato ricalcolato"),
			Probe.Mostrati[I].Toward, Eventi[I].StructureToward);
		TestEqual(TEXT("e l'esito attraversa il confine invece di essere dedotto"),
			Probe.Mostrati[I].bDestroyed,
			Eventi[I].EnvironmentOutcome == ERTEnvironmentOutcome::CoverDestroyed);
	}

	// ⛔ E che il ramo misurato sia quello ABBATTUTO: con una copertura solo scalfita `bDestroyed` sarebbe
	// falso ovunque, e il confronto qui sopra non distinguerebbe un canale che copia da uno che scrive `false`.
	if (Eventi.Num() > 0)
	{
		TestEqual(TEXT("lo scenario ABBATTE, cosi' bDestroyed e' vero e non vacuo"),
			static_cast<int32>(Eventi[0].EnvironmentOutcome),
			static_cast<int32>(ERTEnvironmentOutcome::CoverDestroyed));
		TestTrue(TEXT("e il segnale mostrato lo porta"),
			Probe.Mostrati.Num() > 0 && Probe.Mostrati[0].bDestroyed);
	}

	DestroyEnvWorld(S.World);
	return true;
}

/**
 * La velocita' di riproduzione non cambia cio' che si vede — `#2828`.
 *
 * 🔴 **La rivelazione e' scaglionata nel tempo, quindi la velocita' e' precisamente cio' che potrebbe
 * romperla.** `AttacksToShow` scopre i colpi uno ogni `AttackShowSeconds`; a velocita' alta la fase passa
 * in meno tick, e cio' che non ha fatto in tempo a comparire deve comparire **nel catch-all di fine fase**
 * invece di essere perduto. E' la riga che quel catch-all esiste per garantire.
 *
 * 🔑 **La leva e' `ViewerPlaybackSpeed`, non il passo del `Tick` ne' `MaxPlaybackSeconds`.** Il passo del
 * tick e' granularita' del test, non una velocita' del gioco; e il budget non morde sul `Blast`, che ha
 * slack **zero** per costruzione (`PhaseTime` lo dichiara e `RTPlaybackLibraryTests` lo pinna). Usare il
 * budget qui avrebbe prodotto un gate verde che non esercita niente.
 *
 * ⛔ **E la manopola DEVE fare qualcosa.** Tutto il resto verifica che la velocita' NON cambi il
 * risultato — una proprieta' che un campo mai letto soddisfa alla perfezione. E' la lezione che
 * `Match.Autobattle.DeterminismIsIndependentOfPlayback` scrive per esteso, e la riga sui tick e' il suo
 * equivalente qui.
 *
 * ⚠️ **Due mondi e non due riproduzioni dello stesso**: `FinishPlayback` ha gia' spento il canale quando
 * la prima finisce, e rigiocare la seconda sullo stesso mondo misurerebbe un turno diverso.
 *
 * ⛔ **CIECO a tutto cio' che muove le due run INSIEME, e va saputo prima di fidarsene.** L'invariante
 * confronta due esecuzioni **fra loro**: un difetto che le altera entrambe nello stesso modo le lascia
 * uguali, e questo gate verde. L'esempio non e' ipotetico — se il bordo arrivasse invertito alla
 * presentazione (`StructureCell` e `StructureToward` scambiati al consumo), le due run sarebbero invertite
 * tutt'e due e qui non cadrebbe niente.
 *
 * ∴ **e' un gate sulla VELOCITA', non sul contenuto.** Il contenuto lo presidiano
 * `Playback.StructureHitConsumesResolvedEventWithoutRecomputing` e
 * `Playback.StructureHitIsShownWhenNothingElseHappens`, che confrontano cio' che e' arrivato al map actor
 * con cio' che l'evento **portava** — cioe' con una sorgente esterna alle run, che e' l'unica cosa che
 * rende falsificabile un confronto fra pari.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayStructureHitIsPlaybackSpeedInvariantTest,
	"RefactorTactics.Replay.StructureHitIsPlaybackSpeedInvariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayStructureHitIsPlaybackSpeedInvariantTest::RunTest(const FString&)
{
	FRTEnvBreachScenario Lento = EnvMakeBreachScenario(/*InStructurePower=*/ 10);
	if (!TestTrue(TEXT("scenario a x1 costruito"), Lento.bValid))
	{
		DestroyEnvWorld(Lento.World);
		return false;
	}
	Lento.TM->ViewerPlaybackSpeed = 1.f;
	const FRTEnvPlaybackProbe AX1 = EnvRunPlaybackProbingStructureHits(Lento.TM, Lento.MapActor);
	const int32 EventiX1 = EnvStructureHitEvents(Lento.TM).Num();
	DestroyEnvWorld(Lento.World);

	FRTEnvBreachScenario Veloce = EnvMakeBreachScenario(/*InStructurePower=*/ 10);
	if (!TestTrue(TEXT("scenario a x4 costruito"), Veloce.bValid))
	{
		DestroyEnvWorld(Veloce.World);
		return false;
	}
	Veloce.TM->ViewerPlaybackSpeed = 4.f;
	const FRTEnvPlaybackProbe AX4 = EnvRunPlaybackProbingStructureHits(Veloce.TM, Veloce.MapActor);
	const int32 EventiX4 = EnvStructureHitEvents(Veloce.TM).Num();
	DestroyEnvWorld(Veloce.World);

	TestFalse(TEXT("x1: nessuna riproduzione appesa"), AX1.bAppesa);
	TestFalse(TEXT("x4: nessuna riproduzione appesa"), AX4.bAppesa);

	// ⛔ ANTI-VACUITA' (1): due array vuoti sono uguali. Senza questa riga il gate passerebbe su un
	// playback che non consuma niente a nessuna velocita'.
	if (!TestTrue(TEXT("⛔ a x1 qualcosa e' stato mostrato"), AX1.Mostrati.Num() > 0))
	{
		return false;
	}

	// ⛔ ANTI-VACUITA' (2): la manopola ha davvero morso. Se `ViewerPlaybackSpeed` fosse dichiarato e non
	// letto, le due riproduzioni durerebbero uguale e tutto il resto resterebbe verde senza provare nulla.
	TestTrue(FString::Printf(TEXT("⛔ x4 accorcia la riproduzione rispetto a x1 (%d tick contro %d)"),
		AX4.Tick, AX1.Tick), AX4.Tick < AX1.Tick);

	TestEqual(TEXT("✅ la timeline e' la stessa: la velocita' non tocca la simulazione"), EventiX4, EventiX1);
	TestEqual(TEXT("✅ e tanti segnali a x4 quanti a x1: nessuno perso per fretta"),
		AX4.Mostrati.Num(), AX1.Mostrati.Num());

	for (int32 I = 0; I < FMath::Min(AX1.Mostrati.Num(), AX4.Mostrati.Num()); ++I)
	{
		TestEqual(TEXT("stessa cella"), AX4.Mostrati[I].Cell, AX1.Mostrati[I].Cell);
		TestEqual(TEXT("stesso verso"), AX4.Mostrati[I].Toward, AX1.Mostrati[I].Toward);
		TestEqual(TEXT("stesso esito"), AX4.Mostrati[I].bDestroyed, AX1.Mostrati[I].bDestroyed);
	}

	return true;
}

/**
 * Il muro colpito si vede **anche quando non succede nient'altro** — `#2828`.
 *
 * 🔴 **E' il caso principale della issue, e nessun altro gate lo esercita.** Gli altri scenari usano una
 * copertura **bassa**, che non toglie la linea di tiro: il colpo arriva, produce un `Attack`, e la fase
 * `Blast` nascerebbe comunque. Qui il muro e' **alto** e ferma il colpo — che e' il caso piu' frequente,
 * perche' quel muro e' anche l'unico bersaglio che l'attaccante puo' avere, visto che gli impedisce di
 * vedere chiunque stia dietro.
 *
 * 🔑 **La sequenza che lo rende possibile e' misurata, non supposta**: `RTHexCombatLibrary.cpp:391`
 * raccoglie il danno alla struttura **prima** del controllo sulla linea di tiro (`:560`), che fa `continue`
 * **prima** dell'impronta (`:606`). ∴ la barriera incassa e non nasce ne' un `Attack` ne' un
 * `AttackFootprint`.
 *
 * ⚠️ **Senza il quarto termine di `BlastPhaseIsActive` la fase non si aprirebbe**, e l'evento sparirebbe
 * in silenzio: nessun log, nessun rosso. Il gate puro
 * `Playback.BlastPhaseOpensForStructureHitOnly` prova che il predicato risponde bene; questo prova che il
 * **cablaggio** ci arriva — ed e' la distinzione che il difetto ricorrente di questo repository
 * («codice corretto che nessuno chiama») rende necessaria.
 *
 * ⛔ Le due premesse NON sono contorno: senza di esse il gate misurerebbe lo scenario sbagliato e
 * resterebbe verde anche con il quarto termine rimosso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackStructureHitIsShownWithNoVictimTest,
	"RefactorTactics.Playback.StructureHitIsShownWhenNothingElseHappens",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackStructureHitIsShownWithNoVictimTest::RunTest(const FString&)
{
	// Muro alto (50), colpo 20 → residua 30: **danneggiato**, e il bersaglio dietro resta intatto.
	FRTEnvBreachScenario S = EnvMakeWalledBreachScenario(/*InStructurePower=*/ 20);
	if (!TestTrue(TEXT("scenario col muro alto costruito"), S.bValid))
	{
		DestroyEnvWorld(S.World);
		return false;
	}

	const FRTEnvPlaybackProbe Probe = EnvRunPlaybackProbingStructureHits(S.TM, S.MapActor);
	TestFalse(TEXT("la risoluzione ha chiuso: nessuna riproduzione appesa"), Probe.bAppesa);

	const TArray<FRTResolvedEvent> Colpi = EnvStructureHitEvents(S.TM);

	// --- ⛔ LE DUE PREMESSE, senza cui il gate misura un altro scenario -----------------------------
	//
	// Se il muro non fermasse il colpo nascerebbe un `Attack`, e la fase `Blast` si aprirebbe per quello:
	// il quarto termine diventerebbe irrilevante e il gate resterebbe verde anche rimuovendolo.
	TestEqual(TEXT("⛔ premessa: il muro ha fermato il colpo, nessun Attack"),
		EnvCountTimelineType(S.TM, ERTResolvedEventType::Attack), 0);
	// E l'intento bloccato non lascia impronta: il `continue` della linea di tiro precede la sua emissione.
	TestEqual(TEXT("⛔ premessa: intento bloccato, nessuna impronta"),
		EnvCountTimelineType(S.TM, ERTResolvedEventType::AttackFootprint), 0);

	// --- Il fatto ----------------------------------------------------------------------------------
	if (!TestTrue(TEXT("la barriera ha incassato: c'e' un colpo a struttura in timeline"), Colpi.Num() > 0))
	{
		DestroyEnvWorld(S.World);
		return false;
	}

	// ✅ **L'asserzione del gate**: con zero colpi e zero impronte, la fase `Blast` esiste solo grazie al
	// quarto termine — e se non esistesse questo evento non avrebbe un istante in cui essere mostrato.
	TestTrue(TEXT("✅ e il playback lo ha MOSTRATO, benche' non sia successo nient'altro nel Blast"),
		Probe.Mostrati.Num() > 0);

	if (Probe.Mostrati.Num() > 0)
	{
		TestEqual(TEXT("sul bordo giusto"), Probe.Mostrati[0].Cell, S.Shielded);
		TestEqual(TEXT("e verso l'attaccante"), Probe.Mostrati[0].Toward, S.Attacker);
		TestFalse(TEXT("danneggiato, non abbattuto: 50 meno 20 regge"), Probe.Mostrati[0].bDestroyed);
	}

	DestroyEnvWorld(S.World);
	return true;
}

// =====================================================================================================
// `#3279` — la barriera contata DUE volte, misurata.
//
// ⛔ **Questo blocco NON decide cosa mostrare.** Porta il fatto che la decisione richiede, ed e' scritto
// come CARATTERIZZAZIONE dichiarata: pinna il comportamento attuale, non un contratto. Se la decisione
// sara' deduplicare, questi gate cambiano con essa — ed e' il loro scopo, rendere concreta una scelta che
// altrimenti resta descritta.
// =====================================================================================================

namespace
{
	/**
	 * Scenario della faccia RIDONDANTE: la stessa barriera dichiarata su entrambi i lati di un bordo, fra
	 * celle di `Height` diversa.
	 *
	 * ⚠️ **Si scrive direttamente su `Covers`, e non con `AddCover`, perche' `AddCover` la RIFIUTA** —
	 * *«lo stesso bordo, dalla faccia del VICINO: rifiutata»*. Lo stato resta pero' **legale**:
	 * `URTHexMapAsset::ValidateMap` lo classifica **Warning e non Error** (`GEO-7` di [D-288], `#1893`), con
	 * la ragione scritta che *«correggerlo da soli sarebbe l'auto-fix silenzioso che il Decision Record
	 * vieta»*. ∴ una mappa d'autore puo' averlo, ed e' il caso che questo scenario riproduce.
	 *
	 * 🔑 **`Height` diversa non e' un dettaglio**: il segno di playback prende l'alzata dalla sola cella
	 * che PORTA la copertura (`CellLift(Colpo.Cell)`, che restituisce `Cell.Height`), e le due voci la
	 * portano scambiata. E' l'unica variabile da cui dipende la differenza di quota.
	 *
	 * ⛔ **Le due integrita' sono PARAMETRI, e non lo erano.** Fino a [D-437] questa fixture costruiva sempre
	 * due facce identiche al valore di catalogo, ed era invocata sempre con una potenza che non ne abbatte
	 * nessuna: ∴ misurava **il caso simmetrico e non distruttivo**, l'unico in cui le due voci *sembrano* un
	 * duplicato. Il caso che discrimina — due facce che cadono separatamente — non era rappresentabile, e la
	 * decisione stava per essere presa sul caso che non risponde alla domanda. I due default tengono
	 * invariati i chiamanti che misurano il raddoppio; chi misura l'ASIMMETRIA li passa.
	 */
	FRTEnvBreachScenario EnvMakeRedundantFaceScenario(int32 InStructurePower, int32 InHeightDelta,
		int32 InShieldedIntegrity = FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low),
		int32 InAttackerIntegrity = FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low))
	{
		FRTEnvBreachScenario S;
		S.World = MakeEnvWorld();
		if (!S.World) { return S; }
		S.MapActor = SpawnEnvMap(S.World);
		if (!S.MapActor || !S.MapActor->MapAsset) { return S; }

		S.Shielded = FRTCellId(1, 0);   // porta la faccia W
		S.Attacker = FRTCellId(0, 0);   // porta la faccia E: la stessa barriera, dall'altro lato

		URTHexMapAsset* Asset = S.MapActor->MapAsset;
		const FRTHexCellData* A = Asset->FindCell(S.Shielded);
		const FRTHexCellData* B = Asset->FindCell(S.Attacker);
		if (!A || !B) { return S; }

		// La faccia W di (1,0), su una cella rialzata di `InHeightDelta`. 🔑 **E' la faccia del DIFENSORE**:
		// `HexCoverDamageReduction` legge `FindCell(Target)->CoverOn(verso l'attaccante)`, cioe' la faccia
		// dichiarata sulla PROPRIA cella — quindi e' questa, e non l'altra, a proteggere chi sta qui.
		FRTHexCellData ConFaccia = *A;
		ConFaccia.Height = InHeightDelta;
		ConFaccia.Covers.Add(FRTHexCover(ERTHexDirection::W, ERTHexCoverType::Low, InShieldedIntegrity));
		Asset->AddOrUpdateCell(ConFaccia);

		// ⛔ E la faccia E di (0,0): la SECONDA dichiarazione della stessa barriera, a quota zero. Protegge
		// chi sta DI LA', e la sua integrita' e' indipendente da quella di sopra.
		FRTHexCellData ConSpecchio = *B;
		ConSpecchio.Height = 0;
		ConSpecchio.Covers.Add(FRTHexCover(ERTHexDirection::E, ERTHexCoverType::Low, InAttackerIntegrity));
		Asset->AddOrUpdateCell(ConSpecchio);
		Asset->SortCells();

		S.Breacher = SpawnEnvUnit(S.World, 1, S.Attacker);
		S.Defender = SpawnEnvUnit(S.World, 0, S.Shielded);
		S.TM = S.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!S.Breacher || !S.Defender || !S.TM) { return S; }

		S.Breacher->Abilities[0]->Def.Effects.Add(
			FRTActionEffectSpec(ERTActionEffect::DamageStructure, InStructurePower));
		S.Breacher->PlannedAbilityIndex = 0;
		S.Breacher->PlannedAttackTarget = S.Defender;

		S.bValid = true;
		return S;
	}

	/** L'alzata che il disegno userebbe per un segno: e' `Cell.Height`, e nient'altro. */
	int32 EnvQuotaDelSegno(const URTHexMapAsset* Map, const FRTCellId& Cell)
	{
		const FRTHexCellData* Data = Map ? Map->FindCell(Cell) : nullptr;
		return Data ? Data->Height : 0;
	}
}

/**
 * Una barriera dichiarata su ENTRAMBE le facce produce DUE eventi per UN colpo — `#3279`.
 *
 * 🔴 **E' la misura che la decisione richiede, non la decisione.** Le due voci sono corrette rispetto al
 * modello: `ApplyStructureDamage` chiama `DamageFace` sui due lati perche' *«le due facce sono la STESSA
 * barriera vista dai due lati»*, e se entrambe le celle la dichiarano ne escono due risultati. Cio' che e'
 * ambiguo e' la LETTURA — e decidere cosa mostrare e' una scelta di boundary che questa fetta non prende.
 *
 * ⛔ **I gate uno-a-uno non lo vedono, ed e' il punto.** TurnLog e timeline raddoppiano **insieme**, quindi
 * il rapporto 1:1 e' rispettato e ogni confronto fra i due canali resta verde. Serve contare, non
 * confrontare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStructuresRedundantFaceDoublesTheEventTest,
	"RefactorTactics.Structures.RedundantFaceDoublesTheEvent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStructuresRedundantFaceDoublesTheEventTest::RunTest(const FString&)
{
	// Integrita' 30 per faccia, colpo 10: entrambe restano in piedi, cosi' la misura riguarda il
	// RADDOPPIO e non la distruzione.
	FRTEnvBreachScenario S = EnvMakeRedundantFaceScenario(/*InStructurePower=*/ 10, /*InHeightDelta=*/ 300);
	if (!TestTrue(TEXT("scenario costruito"), S.bValid)) { DestroyEnvWorld(S.World); return false; }

	// ⛔ PREMESSA: la mappa dichiara davvero la barriera su entrambe le facce. Senza, il test misura una
	// barriera normale e il raddoppio non ha modo di manifestarsi.
	TestEqual(TEXT("⛔ premessa: la faccia W di (1,0) e' dichiarata"),
		CoverIntegrityOn(S.MapActor->MapAsset, S.Shielded, ERTHexDirection::W), 30);
	TestEqual(TEXT("⛔ premessa: e ANCHE la faccia E di (0,0), che e' lo stesso bordo"),
		CoverIntegrityOn(S.MapActor->MapAsset, S.Attacker, ERTHexDirection::E), 30);

	RunEnvTurn(S.TM);

	TArray<FRTTurnLogEntry> Voci;
	for (const FRTTurnLogEntry& E : S.TM->GetTurnLog())
	{
		if (URTTurnLogLibrary::IsStructureHit(E)) { Voci.Add(E); }
	}
	const TArray<FRTResolvedEvent> Eventi = EnvStructureHitEvents(S.TM);

	// --- IL FATTO ------------------------------------------------------------------------------------
	if (!TestTrue(TEXT("⛔ il colpo ha raggiunto la barriera"), Voci.Num() > 0))
	{
		DestroyEnvWorld(S.World);
		return false;
	}

	TestEqual(TEXT("🔴 UN colpo produce DUE voci di TurnLog: una per faccia"), Voci.Num(), 2);
	TestEqual(TEXT("🔴 e DUE eventi di playback, non uno"), Eventi.Num(), 2);

	// ⛔ **La ragione per cui i gate 1:1 restano verdi**: i due canali raddoppiano insieme, quindi il loro
	// rapporto e' rispettato. Asserirlo qui e' cio' che rende leggibile perche' nessun confronto lo prenda.
	TestEqual(TEXT("⛔ i due canali raddoppiano INSIEME: nessun confronto 1:1 puo' accorgersene"),
		Eventi.Num(), Voci.Num());

	// --- LE DUE VOCI SONO LA STESSA BARRIERA, VISTA DAI DUE LATI --------------------------------------
	if (Eventi.Num() == 2)
	{
		TestEqual(TEXT("la cella del primo e' il verso del secondo"),
			Eventi[0].StructureCell, Eventi[1].StructureToward);
		TestEqual(TEXT("e viceversa: e' un bordo solo, letto nei due sensi"),
			Eventi[1].StructureCell, Eventi[0].StructureToward);
	}

	DestroyEnvWorld(S.World);
	return true;
}

/**
 * E dal 2026-09-22 i due segni non sono piu' SOVRAPPOSTI: compaiono a quote diverse — `#3279`.
 *
 * 🔴 **La duplicazione c'era gia' ed era invisibile.** Il disegno prendeva la quota come MEDIA delle due
 * celle — simmetrica allo scambio — quindi i due segni coincidevano esattamente. La correzione
 * dell'alzata in `#2828` li ha separati: ora prendono `CellLift(Colpo.Cell)`, cioe' `Cell.Height`, e le
 * due voci portano `Cell` scambiata.
 *
 * ∴ su un bordo fra celle di `Height` diversa compaiono due segni a quote diverse, che si leggono come
 * **due barriere colpite** invece di una contata due volte.
 *
 * ⚠️ **Il disegno non e' osservabile headless, la quota si'.** `CellLift` e' una funzione pura della
 * cella, quindi la differenza fra le due alzate e' calcolabile dai dati che l'evento porta — senza
 * guardare cosa viene disegnato, e senza duplicarne la formula: questo test legge `Height`, che e'
 * l'ingresso, non la ricalcola.
 *
 * ⛔ **E per la stessa ragione e' CIECO al disegno, il che va saputo prima di fidarsene.** Prova che gli
 * INGRESSI della quota differiscono, non che il risultato si veda. Se qualcuno riportasse l'alzata alla
 * MEDIA delle due celle — il comportamento precedente a `#2828`, che rendeva i due segni coincidenti —
 * questo gate resterebbe **verde**: `Height` non cambierebbe, cambierebbe cio' che il disegno ne fa.
 *
 * ∴ la verifica del risultato appartiene a una voce `PIE-*`, e questo gate non la sostituisce. Cio' che
 * copre e' il ramo a monte: che due voci esistano e portino celle diverse.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStructuresRedundantFaceDrawsAtTwoHeightsTest,
	"RefactorTactics.Structures.RedundantFaceDrawsAtTwoHeights",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStructuresRedundantFaceDrawsAtTwoHeightsTest::RunTest(const FString&)
{
	const int32 Dislivello = 300;
	FRTEnvBreachScenario S = EnvMakeRedundantFaceScenario(/*InStructurePower=*/ 10, Dislivello);
	if (!TestTrue(TEXT("scenario costruito"), S.bValid)) { DestroyEnvWorld(S.World); return false; }

	// ⛔ PREMESSA: le due celle hanno davvero quote diverse. Con `Height` uguale i due segni tornerebbero
	// sovrapposti e questo test misurerebbe l'assenza di un difetto che c'e'.
	const int32 QuotaA = EnvQuotaDelSegno(S.MapActor->MapAsset, S.Shielded);
	const int32 QuotaB = EnvQuotaDelSegno(S.MapActor->MapAsset, S.Attacker);
	TestEqual(TEXT("⛔ premessa: le due celle del bordo hanno quote diverse"), QuotaA - QuotaB, Dislivello);

	const FRTEnvPlaybackProbe Probe = EnvRunPlaybackProbingStructureHits(S.TM, S.MapActor);
	TestFalse(TEXT("la risoluzione ha chiuso"), Probe.bAppesa);

	if (!TestEqual(TEXT("⛔ due segni mostrati, uno per faccia"), Probe.Mostrati.Num(), 2))
	{
		DestroyEnvWorld(S.World);
		return false;
	}

	// 🔴 **La misura che la decisione richiede.** L'alzata di ciascun segno e' `Height` della cella che
	// porta la copertura; le due voci la portano scambiata, quindi le quote differiscono del dislivello.
	const int32 Quota0 = EnvQuotaDelSegno(S.MapActor->MapAsset, Probe.Mostrati[0].Cell);
	const int32 Quota1 = EnvQuotaDelSegno(S.MapActor->MapAsset, Probe.Mostrati[1].Cell);
	TestEqual(TEXT("🔴 i due segni stanno a quote diverse, separate dal dislivello"),
		FMath::Abs(Quota0 - Quota1), Dislivello);

	// ⚠️ E la controprova che rende la riga sopra una misura e non una tautologia: a dislivello ZERO i due
	// segni tornerebbero sovrapposti, cioe' allo stato in cui la duplicazione era invisibile.
	DestroyEnvWorld(S.World);

	FRTEnvBreachScenario Piatto = EnvMakeRedundantFaceScenario(/*InStructurePower=*/ 10, /*Dislivello=*/ 0);
	if (!TestTrue(TEXT("scenario piatto costruito"), Piatto.bValid))
	{
		DestroyEnvWorld(Piatto.World);
		return false;
	}
	const FRTEnvPlaybackProbe PiattoProbe = EnvRunPlaybackProbingStructureHits(Piatto.TM, Piatto.MapActor);
	if (PiattoProbe.Mostrati.Num() == 2)
	{
		const int32 P0 = EnvQuotaDelSegno(Piatto.MapActor->MapAsset, PiattoProbe.Mostrati[0].Cell);
		const int32 P1 = EnvQuotaDelSegno(Piatto.MapActor->MapAsset, PiattoProbe.Mostrati[1].Cell);
		TestEqual(TEXT("⏱️ a dislivello zero i due segni si sovrappongono: com'era prima di #2828"), P0, P1);
	}
	DestroyEnvWorld(Piatto.World);

	return true;
}

/**
 * Le due facce di un bordo condiviso CADONO SEPARATAMENTE, e per questo non sono un duplicato — [D-437],
 * `#3279`.
 *
 * 🔴 **E' la condizione obbligatoria della decisione, e senza di essa la decisione e' reversibile per
 * sbaglio.** Chi rilegge il caso simmetrico vede due voci identiche, le scambia per un duplicato e le
 * collassa: e' cio' che stava per succedere quando la sola fixture esistente costruiva due facce uguali che
 * non cadevano mai. Questo gate esercita il caso che discrimina, e cade se qualcuno le riduce a una.
 *
 * 🔑 **Il fatto che rovescia la premessa «una barriera contata due volte»**: le due facce sono lette da due
 * regole DIVERSE.
 *
 *   - traversata, vista, linea di tiro → `URTHexCoverLibrary::CoverBetween` tiene la **piu' alta** delle due;
 *   - 🔴 riduzione di danno di chi sta dietro → `URTHexCombatLibrary::HexCoverDamageReduction` legge
 *     `FindCell(Target)->CoverOn(...)`, cioe' la faccia dichiarata sulla **propria** cella.
 *
 * ∴ chi sta di qua e' protetto dalla faccia di qua, chi sta di la' da quella di la'. `Type` e `Integrity`
 * sono indipendenti per faccia — stato legale, `ValidateMap` lo classifica Warning (`GEO-7` di [D-288]) —
 * quindi **una puo' cadere mentre l'altra regge**, e la protezione di UN SOLO lato sparisce.
 *
 * ⛔ **Cosa perderebbe una voce sola, detto per esteso.** La faccia del difensore cade; la voce collassata
 * direbbe *«ha retto, ne restano 25»*. La barriera nel complesso esiste ancora — `CoverBetween` risolve
 * sull'altra faccia — quindi nessun indizio a schermo smentirebbe la lettura, e dal turno dopo il difensore
 * incassa `LowCoverDamageReduction` in piu' a colpo senza avere da cosa accorgersene. E' il difetto che
 * `StructureHit` esiste per chiudere — *«il giocatore vedeva lo stato dopo e mai il cambiamento»* —
 * riaperto sul lato che conta.
 *
 * ⚠️ **Le due asserzioni finali sono il cuore, e non sono ridondanti fra loro**: una misura che la
 * protezione del difensore e' sparita, l'altra che la barriera c'e' ancora. Sono vere INSIEME, ed e'
 * esattamente questa coppia che una voce sola non sa raccontare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStructuresRedundantFacesFallIndependentlyTest,
	"RefactorTactics.Structures.RedundantFacesFallIndependently",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStructuresRedundantFacesFallIndependentlyTest::RunTest(const FString&)
{
	// Faccia del DIFENSORE fragile (10), faccia dell'ATTACCANTE robusta (40), colpo 15: ne cade UNA sola.
	// ⚠️ I tre numeri sono scelti per essere l'uno la ragione dell'altro — 10 < 15 <= 40 — e cambiarne uno
	// solo riporta il test al caso simmetrico che non discrimina.
	const int32 FragileDelDifensore = 10;
	const int32 RobustaDellAttaccante = 40;
	const int32 Colpo = 15;
	FRTEnvBreachScenario S = EnvMakeRedundantFaceScenario(Colpo, /*InHeightDelta=*/ 300,
		FragileDelDifensore, RobustaDellAttaccante);
	if (!TestTrue(TEXT("scenario costruito"), S.bValid)) { DestroyEnvWorld(S.World); return false; }

	URTHexMapAsset* Mappa = S.MapActor->MapAsset;

	// --- ⛔ PREMESSE, prima di misurare: senza, le asserzioni sotto non significherebbero niente ---------
	TestEqual(TEXT("⛔ premessa: la faccia del difensore e' quella FRAGILE"),
		CoverIntegrityOn(Mappa, S.Shielded, ERTHexDirection::W), FragileDelDifensore);
	TestEqual(TEXT("⛔ premessa: e quella dell'attaccante, lo stesso bordo, e' piu' ROBUSTA"),
		CoverIntegrityOn(Mappa, S.Attacker, ERTHexDirection::E), RobustaDellAttaccante);
	TestTrue(TEXT("⛔ premessa: il colpo abbatte la prima e non la seconda"),
		Colpo >= FragileDelDifensore && Colpo < RobustaDellAttaccante);

	// 🔑 **La protezione che il difensore ha PRIMA**, ed e' quella che sta per sparire: viene dalla faccia
	// della sua cella, non dal bordo nel complesso.
	const int32 RiduzionePrima = URTHexCombatLibrary::HexCoverDamageReduction(Mappa, S.Attacker, S.Shielded,
		ERTAbilityShape::Single);
	if (!TestEqual(TEXT("⛔ premessa: prima del colpo il difensore E' riparato"),
		RiduzionePrima, URTCombatLibrary::LowCoverDamageReduction))
	{
		DestroyEnvWorld(S.World);
		return false;
	}

	RunEnvTurn(S.TM);

	// --- IL FATTO: due voci, e portano esiti DIVERSI --------------------------------------------------
	TArray<FRTTurnLogEntry> Voci;
	for (const FRTTurnLogEntry& E : S.TM->GetTurnLog())
	{
		if (URTTurnLogLibrary::IsStructureHit(E)) { Voci.Add(E); }
	}
	if (!TestEqual(TEXT("🔴 un colpo, DUE voci: una per faccia"), Voci.Num(), 2))
	{
		DestroyEnvWorld(S.World);
		return false;
	}

	// ⚠️ Si cercano per CELLA e non per indice: l'ordine di `ApplyStructureDamage` e' canonico (`StableLess`)
	// e dipenderci renderebbe il gate fragile a un riordino che non riguarda questa proprieta'.
	const FRTTurnLogEntry* VoceDifensore = Voci.FindByPredicate(
		[&S](const FRTTurnLogEntry& E) { return E.SrcCell == S.Shielded; });
	const FRTTurnLogEntry* VoceAttaccante = Voci.FindByPredicate(
		[&S](const FRTTurnLogEntry& E) { return E.SrcCell == S.Attacker; });
	if (!TestTrue(TEXT("le due voci sono le due facce, non due volte la stessa"),
		VoceDifensore != nullptr && VoceAttaccante != nullptr))
	{
		DestroyEnvWorld(S.World);
		return false;
	}

	// 🔴 **L'asserzione che distingue «una barriera contata due volte» da «due fatti»**: i due esiti sono
	// diversi. Un duplicato non puo' esserlo.
	TestEqual(TEXT("🔴 la faccia del difensore e' ABBATTUTA"),
		static_cast<int32>(VoceDifensore->Outcome),
		static_cast<int32>(ERTEnvironmentOutcome::CoverDestroyed));
	TestEqual(TEXT("🔴 quella dell'attaccante, sullo STESSO bordo, e' solo danneggiata"),
		static_cast<int32>(VoceAttaccante->Outcome),
		static_cast<int32>(ERTEnvironmentOutcome::CoverDamaged));
	TestNotEqual(TEXT("🔴 ∴ le due voci NON sono intercambiabili: hanno esiti diversi"),
		static_cast<int32>(VoceDifensore->Outcome), static_cast<int32>(VoceAttaccante->Outcome));
	TestEqual(TEXT("e l'integrita' residua dichiarata e' quella della faccia superstite"),
		VoceAttaccante->Amount, RobustaDellAttaccante - Colpo);

	// --- ⛔ COSA SI PERDEREBBE COLLASSANDOLE: le due proprieta' che valgono INSIEME -------------------
	const int32 RiduzioneDopo = URTHexCombatLibrary::HexCoverDamageReduction(Mappa, S.Attacker, S.Shielded,
		ERTAbilityShape::Single);
	TestEqual(TEXT("🔴 il difensore ha PERSO il riparo: la faccia della sua cella non c'e' piu'"),
		RiduzioneDopo, 0);
	TestEqual(TEXT("🔴 ma la barriera sul bordo ESISTE ancora: `CoverBetween` risolve sull'altra faccia"),
		static_cast<int32>(URTHexCoverLibrary::CoverBetween(Mappa, S.Attacker, S.Shielded)),
		static_cast<int32>(ERTHexCoverType::Low));

	DestroyEnvWorld(S.World);
	return true;
}

/**
 * E i due segni mostrati portano esiti diversi: non sono lo stesso segno disegnato due volte — `#3279`.
 *
 * 🔑 **Il gate gemello sull'altro canale.** `RedundantFacesFallIndependently` misura il TurnLog; qui si
 * misura cio' che il playback consegna al disegno, che e' l'altro dei due canali che raddoppiano insieme.
 * La proprieta' e' la stessa e la conclusione pure: due elementi con `bDestroyed` diverso non possono essere
 * una voce duplicata.
 *
 * ⚠️ **Cieco al disegno, come il suo vicino di quota, e per la stessa ragione**: misura cio' che il map
 * actor RICEVE, non cio' che compare a schermo. Che un segno di copertura abbattuta si distingua a occhio da
 * uno di copertura danneggiata appartiene a una voce `PIE-*`, e questo gate non la sostituisce.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackRedundantFaceShowsTwoDifferentOutcomesTest,
	"RefactorTactics.Playback.RedundantFaceShowsTwoDifferentOutcomes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackRedundantFaceShowsTwoDifferentOutcomesTest::RunTest(const FString&)
{
	FRTEnvBreachScenario S = EnvMakeRedundantFaceScenario(/*InStructurePower=*/ 15, /*InHeightDelta=*/ 300,
		/*InShieldedIntegrity=*/ 10, /*InAttackerIntegrity=*/ 40);
	if (!TestTrue(TEXT("scenario costruito"), S.bValid)) { DestroyEnvWorld(S.World); return false; }

	const FRTEnvPlaybackProbe Probe = EnvRunPlaybackProbingStructureHits(S.TM, S.MapActor);
	TestFalse(TEXT("la risoluzione ha chiuso"), Probe.bAppesa);

	if (!TestEqual(TEXT("⛔ due segni mostrati, uno per faccia"), Probe.Mostrati.Num(), 2))
	{
		DestroyEnvWorld(S.World);
		return false;
	}

	const FRTPlaybackStructureHit* SegnoDifensore = Probe.Mostrati.FindByPredicate(
		[&S](const FRTPlaybackStructureHit& H) { return H.Cell == S.Shielded; });
	const FRTPlaybackStructureHit* SegnoAttaccante = Probe.Mostrati.FindByPredicate(
		[&S](const FRTPlaybackStructureHit& H) { return H.Cell == S.Attacker; });
	if (!TestTrue(TEXT("i due segni sono le due facce"),
		SegnoDifensore != nullptr && SegnoAttaccante != nullptr))
	{
		DestroyEnvWorld(S.World);
		return false;
	}

	TestTrue(TEXT("🔴 il segno sulla faccia del difensore dice ABBATTUTA"), SegnoDifensore->bDestroyed);
	TestFalse(TEXT("🔴 e quello sull'altra faccia dello stesso bordo dice danneggiata"),
		SegnoAttaccante->bDestroyed);

	// ⚠️ E il bordo e' UNO: le due celle sono l'una il verso dell'altra. Senza questa riga i due segni
	// potrebbero essere due barriere davvero diverse, e il gate misurerebbe un'ovvieta'.
	TestEqual(TEXT("⛔ premessa: e' un bordo solo, letto nei due sensi"),
		SegnoDifensore->Toward, SegnoAttaccante->Cell);
	TestEqual(TEXT("⛔ premessa: e viceversa"), SegnoAttaccante->Toward, SegnoDifensore->Cell);

	DestroyEnvWorld(S.World);
	return true;
}

/**
 * E il limite e' DICHIARATO dove lo legge chi disegna una mappa, non solo dove lo legge chi legge il
 * codice — [D-437], `#3279`.
 *
 * 🔑 **La decisione e' «lasciarlo com'e'», e una scelta del genere ha un prezzo: va dichiarata.** Il
 * Warning di `GEO-7` esisteva gia' (`#1893`), ma diceva soltanto *«ridondante»* — che chi disegna legge
 * come *«un doppione innocuo»*, cioe' la lettura che [D-437] smentisce. Ora il messaggio porta la
 * conseguenza a runtime: due eventi, due segni, e una faccia che puo' cadere mentre l'altra regge.
 *
 * ⚠️ **Perche' l'ancora e' `D-437` e non una frase del messaggio.** `ERTMapValidationReason` dichiara la
 * ragione per cui un test non deve riconoscere una regola dal suo TESTO: *«si rompe alla prima
 * riformulazione, e allora chi riformula impara a non toccare i messaggi»*. Qui pero' l'oggetto del gate
 * **e'** cio' che il messaggio dichiara, quindi qualcosa va guardato: si guarda il riferimento alla
 * decisione, che resta valido a qualunque riformulazione e che chi riscrive senza leggerla toglierebbe.
 *
 * ⛔ **E il controllo NEGATIVO sta accanto a quello positivo**, perche' un gate che puo' solo essere verde
 * non e' un gate: una mappa con UNA faccia sola non deve produrre quella riga.
 *
 * ⚠️ Limite dichiarato: nessun hook di validazione al salvataggio esiste
 * (`grep -rn "EditorValidator\|IsDataValid\|DataValidation" --include=*.cpp --include=*.h Source/` → 0).
 * ∴ questo gate pinna che la dichiarazione ci sia, non che qualcuno la esegua.
 *
 * ⏱️ **Questa riga diceva anche «`ValidateMap()` ha un solo chiamante non di test —
 * `ARTHexMapActor::ValidateAsset`, un bottone manuale», e dal 2026-09-24 e' falso**: `#1864` ne ha
 * aggiunto un secondo, `URTHexMapSummaryLibrary::DescriviValidazione`, che NON e' un bottone — alimenta
 * il readout del mode Hex Map e si rifa' da solo quando la mappa smette di cambiare. La conclusione qui
 * sotto non cambia: quel readout descrive la mappa **aperta nell'editor**, quindi continua a non essere
 * un hook di salvataggio, e questo gate continua a pinnare la dichiarazione e non la sua esecuzione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStructuresRedundantFaceWarningDeclaresConsequenceTest,
	"RefactorTactics.Structures.RedundantFaceWarningDeclaresTheConsequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStructuresRedundantFaceWarningDeclaresConsequenceTest::RunTest(const FString&)
{
	const FRTCellId Origine(0, 0, 0);
	const ERTHexDirection Verso = ERTHexDirection::E;
	const FRTCellId Oltre = URTHexLibrary::Neighbor(Origine, Verso);
	const ERTHexDirection Contro = URTHexLibrary::OppositeDirection(Verso);

	auto Costruisci = [&Origine, &Oltre](bool bEntrambeLeFacce)
	{
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
		FRTHexCellData A; A.Id = Origine;
		A.Covers.Add(FRTHexCover(ERTHexDirection::E, ERTHexCoverType::Low, 30));
		Map->AddOrUpdateCell(A);
		FRTHexCellData B; B.Id = Oltre;
		if (bEntrambeLeFacce)
		{
			B.Covers.Add(FRTHexCover(ERTHexDirection::W, ERTHexCoverType::Low, 30));
		}
		Map->AddOrUpdateCell(B);
		return Map;
	};

	auto RigaRidondante = [](const URTHexMapAsset* Map)
	{
		FString Trovata;
		for (const FString& Riga : Map->ValidateMap())
		{
			if (Riga.Contains(TEXT("ridondante")) && Riga.Contains(TEXT("condiviso")))
			{
				Trovata = Riga;
			}
		}
		return Trovata;
	};

	// ⛔ PREMESSA: la direzione opposta e' davvero quella che chiude il bordo. Senza, il caso "due facce"
	// costruirebbe due coperture su bordi diversi e il gate misurerebbe un'altra cosa.
	TestEqual(TEXT("⛔ premessa: E e W sono lo stesso bordo visto dalle due celle"),
		static_cast<int32>(Contro), static_cast<int32>(ERTHexDirection::W));

	// --- CONTROLLO NEGATIVO: una faccia sola non e' ridondante ---------------------------------------
	TestTrue(TEXT("⛔ una faccia sola NON produce la segnalazione: il gate puo' essere rosso"),
		RigaRidondante(Costruisci(/*bEntrambeLeFacce=*/ false)).IsEmpty());

	// --- CONTROLLO POSITIVO: le due facce la producono, e dichiara la conseguenza --------------------
	const FString Riga = RigaRidondante(Costruisci(/*bEntrambeLeFacce=*/ true));
	if (!TestFalse(TEXT("le due facce dello stesso bordo sono segnalate"), Riga.IsEmpty()))
	{
		return false;
	}
	TestTrue(TEXT("🔴 e la segnalazione rimanda alla decisione che spiega PERCHE' resta com'e'"),
		Riga.Contains(TEXT("D-437")));

	return true;
}

// =====================================================================================================
// `#3280` — il colpo all'ARCO ha un evento di playback, e ne ha UNO PER VOCE.
//
// 🔑 Quattro gate, e tre di essi esistono perche' [D-437] li ha chiesti per nome. Il quarto — l'arco
// SPENTO — nasce da una clausola della stessa decisione: *«l'evento porti lo STATO, non un booleano
// dedotto, o erediteta' la bugia il giorno in cui qualcuno chiamera' `SetArcState(Inactive)`»*.
// =====================================================================================================

namespace
{
	/** Un ponte fra due piani, e chi lo sfonda sparando a chi ci sta sopra. */
	struct FRTEnvArcScenario
	{
		UWorld* World = nullptr;
		ARTHexMapActor* MapActor = nullptr;
		ARTTurnManager* TM = nullptr;
		ARTUnit* Breacher = nullptr;
		ARTUnit* Foe = nullptr;
		FRTCellId Ground{0, 0, 0};
		FRTCellId Upper{1, 0, 1};
		bool bValid = false;
	};

	/**
	 * Lo scenario condiviso dei gate d'arco: la stessa scena di `Structures.Bridge.DamagedInPlayedTurn`,
	 * con il verso reso PARAMETRO.
	 *
	 * ⛔ **`bBidirectional` non e' una comodita': e' il caso che nessuna fixture rappresentava.** Le due
	 * fixture di ponte esistenti costruiscono entrambe l'arco bidirezionale, quindi un gate che contasse
	 * «due eventi» sarebbe indistinguibile da uno che conta «un evento per ponte» — e [D-437] chiede
	 * esattamente quella distinzione.
	 */
	FRTEnvArcScenario EnvMakeArcScenario(bool bBidirectional, int32 InStructurePower)
	{
		FRTEnvArcScenario S;
		S.World = MakeEnvWorld();
		if (!S.World) { return S; }
		S.MapActor = SpawnEnvMap(S.World);
		if (!S.MapActor || !S.MapActor->MapAsset) { return S; }

		S.MapActor->MapAsset->AddOrUpdateCell(FRTHexCellData(S.Upper));
		S.MapActor->MapAsset->SortCells();
		S.MapActor->MapAsset->AddTransition(S.Ground, S.Upper, /*Cost*/ 1,
			ERTHexTransitionKind::Bridge, bBidirectional);

		S.Breacher = SpawnEnvUnit(S.World, 0, S.Ground);
		S.Foe = SpawnEnvUnit(S.World, 1, S.Upper);
		S.TM = S.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!S.Breacher || !S.Foe || !S.TM) { return S; }

		S.Breacher->Abilities[0]->Def.Effects.Add(
			FRTActionEffectSpec(ERTActionEffect::DamageStructure, InStructurePower));
		S.Breacher->PlannedAbilityIndex = 0;
		S.Breacher->PlannedAttackTarget = S.Foe;

		S.bValid = true;
		return S;
	}

	/** Gli eventi di timeline che sono colpi a un arco. */
	TArray<FRTResolvedEvent> EnvArcHitEvents(const ARTTurnManager* TM)
	{
		TArray<FRTResolvedEvent> Out;
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::ArcHit) { Out.Add(Ev); }
		}
		return Out;
	}

	/** Le voci di TurnLog che `IsArcHit` riconosce. Si CHIEDE al predicato, non si riscrive. */
	TArray<FRTTurnLogEntry> EnvArcHitEntries(const ARTTurnManager* TM)
	{
		TArray<FRTTurnLogEntry> Out;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (URTTurnLogLibrary::IsArcHit(E)) { Out.Add(E); }
		}
		return Out;
	}
}

/**
 * Un evento per VOCE di TurnLog, quindi DUE per un ponte bidirezionale — `#3280`, [D-437].
 *
 * 🔴 **Il difetto che chiude: un ponte poteva crollare e la timeline non lo sapeva.** `IsStructureHit`
 * copriva `CoverDamaged`/`CoverDestroyed` e non `BridgeDamaged`/`BridgeDestroyed`, quindi un arco abbattuto
 * non aveva ne' evento, ne' cue, ne' un'assenza dichiarabile — cioe' nemmeno qualcosa che
 * `Presentation.AbsenceCensusIsPinned` potesse sorvegliare.
 *
 * 🔑 **I due eventi sono PINNATI, non subiti.** `DamageArc` scala l'integrita' da entrambi i capi perche' un
 * ponte colpito una volta non regga il doppio da una parte, e [D-437] ha deciso che i due restino: `State` e
 * `Integrity` sono `UPROPERTY` per arco **diretto** e `IsArcTraversable` e' **direzionale**, quindi due versi
 * con integrita' diversa danno esiti diversi — una passerella crollata in salita e intatta in discesa. Un
 * evento per arco dovrebbe scegliere in silenzio quale verso racconta il ponte.
 *
 * ⚠️ **Il rapporto 1:1 e' asserito contro il predicato, non contro un numero scritto a mano**: e' la stessa
 * disciplina di `Turn.StructureHitEventMatchesTurnLogEntry` — riscrivere il criterio da un lato ne farebbe
 * una seconda copia, e i due canali tornerebbero a poter divergere ([D-098]).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnArcHitEmitsOneEventPerLoggedEntryTest,
	"RefactorTactics.Turn.ArcHitEmitsOneEventPerLoggedEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnArcHitEmitsOneEventPerLoggedEntryTest::RunTest(const FString&)
{
	// Integrita' di catalogo, colpo 20: il ponte incassa e **regge**, cosi' la misura riguarda il rapporto
	// fra i canali e non la distruzione.
	FRTEnvArcScenario S = EnvMakeArcScenario(/*bBidirectional=*/ true, /*InStructurePower=*/ 20);
	if (!TestTrue(TEXT("scenario costruito"), S.bValid)) { DestroyEnvWorld(S.World); return false; }

	RunEnvTurn(S.TM);

	const TArray<FRTTurnLogEntry> Voci = EnvArcHitEntries(S.TM);
	const TArray<FRTResolvedEvent> Eventi = EnvArcHitEvents(S.TM);

	// ⛔ PREMESSA: il colpo ha davvero raggiunto l'arco. Senza, tutto il resto confronterebbe due zeri —
	// che e' il gate verde per costruzione che [[oracolo]] chiama «cieco alla differenza».
	if (!TestEqual(TEXT("⛔ premessa: due voci di TurnLog, una per verso"), Voci.Num(), 2))
	{
		DestroyEnvWorld(S.World);
		return false;
	}

	// --- IL FATTO -------------------------------------------------------------------------------------
	TestEqual(TEXT("🔴 e DUE eventi di playback: uno per voce"), Eventi.Num(), 2);
	TestEqual(TEXT("🔴 il rapporto e' 1:1 contro il predicato, non contro un numero scritto a mano"),
		Eventi.Num(), Voci.Num());

	// I due eventi sono lo stesso arco letto nei due versi: la coppia dell'uno e' quella dell'altro,
	// scambiata. Senza questa riga «due eventi» starebbe in piedi anche con due archi diversi.
	if (Eventi.Num() == 2)
	{
		TestEqual(TEXT("il capo del primo e' la meta del secondo"), Eventi[0].ArcFrom, Eventi[1].ArcTo);
		TestEqual(TEXT("e viceversa: e' un arco solo, percorso nei due sensi"),
			Eventi[1].ArcFrom, Eventi[0].ArcTo);
		TestEqual(TEXT("l'esito e' quello della voce: danneggiato, non abbattuto"),
			static_cast<int32>(Eventi[0].EnvironmentOutcome),
			static_cast<int32>(ERTEnvironmentOutcome::BridgeDamaged));
		TestEqual(TEXT("e `Amount` e' l'integrita' RESIDUA, come per la copertura"),
			Eventi[0].Amount, Voci[0].Amount);
	}

	// ⛔ **E NON e' un `StructureHit`**: il valore e' proprio, ed e' la decisione. Se qualcuno allargasse
	// `IsStructureHit` agli archi, un ponte crollato verrebbe disegnato col tratto del graffio — il
	// consumatore confronta `CoverDestroyed`.
	TestEqual(TEXT("⛔ e nessun colpo a struttura: un arco non e' un bordo esagonale"),
		EnvCountTimelineType(S.TM, ERTResolvedEventType::StructureHit), 0);

	DestroyEnvWorld(S.World);
	return true;
}

/**
 * E un arco a SENSO UNICO ne produce **uno solo** — `#3280`, [D-437].
 *
 * ⛔ **Obbligatorio, ed e' il caso che nessuna fixture rappresentava**: le due fixture di ponte esistenti
 * costruiscono entrambe l'arco bidirezionale. Senza questo gate, *«un evento per voce»* e *«due eventi per
 * ponte»* sarebbero indistinguibili — e il prossimo che legge il caso bidirezionale scambierebbe le due
 * voci per un raddoppio da collassare.
 *
 * 🔑 **Il numero non e' «due per ponte»: e' uno per arco diretto ancora in piedi.** Un arco a senso unico ne
 * da' uno; un ponte con un verso gia' caduto pure, perche' `DamageArc` salta i `Destroyed`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnArcHitIsEmittedForAOneWayArcTest,
	"RefactorTactics.Turn.ArcHitIsEmittedForAOneWayArc",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnArcHitIsEmittedForAOneWayArcTest::RunTest(const FString&)
{
	FRTEnvArcScenario S = EnvMakeArcScenario(/*bBidirectional=*/ false, /*InStructurePower=*/ 20);
	if (!TestTrue(TEXT("scenario costruito"), S.bValid)) { DestroyEnvWorld(S.World); return false; }

	// ⛔ PREMESSA: l'arco e' davvero a senso unico. Con `AddTransition` bidirezionale questo test
	// misurerebbe lo stesso caso di quello sopra, e il suo «uno» sarebbe falso.
	TestNotNull(TEXT("⛔ premessa: l'andata esiste"),
		URTHexArcLibrary::FindArc(S.MapActor->MapAsset, S.Ground, S.Upper));
	TestNull(TEXT("⛔ premessa: e il ritorno NO"),
		URTHexArcLibrary::FindArc(S.MapActor->MapAsset, S.Upper, S.Ground));

	RunEnvTurn(S.TM);

	const TArray<FRTTurnLogEntry> Voci = EnvArcHitEntries(S.TM);
	const TArray<FRTResolvedEvent> Eventi = EnvArcHitEvents(S.TM);

	TestEqual(TEXT("🔴 una voce sola: c'e' un arco diretto solo"), Voci.Num(), 1);
	TestEqual(TEXT("🔴 e UN evento, non due: il conteggio segue le voci, non i ponti"), Eventi.Num(), 1);

	if (Eventi.Num() == 1)
	{
		TestEqual(TEXT("e porta il verso che esiste"), Eventi[0].ArcFrom, S.Ground);
		TestEqual(TEXT("verso l'altro capo"), Eventi[0].ArcTo, S.Upper);
	}

	DestroyEnvWorld(S.World);
	return true;
}

/**
 * L'evento porta l'ARCO, non un'unita' — `#3280`.
 *
 * 🔑 **Gemello di `Turn.StructureHitEventCarriesEdgeNotActor`, e sorveglia lo stesso scivolamento**: il
 * giorno in cui qualcuno mettesse in `TargetStableUnitId` l'unita' piu' vicina, il fatto smetterebbe di
 * riguardare l'arco e comincerebbe a riguardare una persona.
 *
 * ⚠️ **Il `Foe` sul ponte c'e' apposta e non e' arredamento**: e' l'unita' che un consumatore sbagliato
 * ci metterebbe. Senza qualcuno la' sopra, questo gate sarebbe verde per assenza.
 *
 * ⛔ **E i due capi sono COPIATI, non ricalcolati**: chi consuma non deve chiedere alla mappa quale arco
 * sia stato colpito — e' il divieto che [D-278] impone all'intero layer.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnArcHitCarriesTheArcNotAnActorTest,
	"RefactorTactics.Turn.ArcHitCarriesTheArcNotAnActor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnArcHitCarriesTheArcNotAnActorTest::RunTest(const FString&)
{
	FRTEnvArcScenario S = EnvMakeArcScenario(/*bBidirectional=*/ true, /*InStructurePower=*/ 20);
	if (!TestTrue(TEXT("scenario costruito"), S.bValid)) { DestroyEnvWorld(S.World); return false; }

	RunEnvTurn(S.TM);

	const TArray<FRTResolvedEvent> Eventi = EnvArcHitEvents(S.TM);
	if (!TestTrue(TEXT("⛔ premessa: almeno un colpo all'arco e' stato emesso"), Eventi.Num() > 0))
	{
		DestroyEnvWorld(S.World);
		return false;
	}

	// ⛔ PREMESSA, e si legge DOPO il turno perche' e' li' che l'identita' nasce: gli `StableUnitId` li
	// assegna la risoluzione, non lo spawn. ⚠️ Letta prima, questa riga valeva `0` — e allora
	// `TargetStableUnitId == 0` non avrebbe distinto *«nessuno»* da *«il Foe»*, cioe' il gate sarebbe stato
	// verde **per la ragione sbagliata**. E' lo stesso ordine del gemello `StructureHitEventCarriesEdgeNotActor`.
	const int32 IdDelFoe = S.Foe->StableUnitId;
	if (!TestTrue(TEXT("⛔ premessa: il Foe ha un'identita' stabile non nulla"), IdDelFoe != 0))
	{
		DestroyEnvWorld(S.World);
		return false;
	}

	const TArray<FRTTurnLogEntry> Voci = EnvArcHitEntries(S.TM);
	for (int32 I = 0; I < Eventi.Num(); ++I)
	{
		// 🔴 Il bersaglio e' un ARCO: non ha uno `StableUnitId`, e il campo resta zero.
		TestEqual(TEXT("🔴 `TargetStableUnitId` resta 0: il bersaglio e' un arco"),
			Eventi[I].TargetStableUnitId, 0);
		TestNotEqual(TEXT("⛔ e in particolare NON e' l'unita' sull'altro capo"),
			Eventi[I].TargetStableUnitId, IdDelFoe);
		// L'attaccante invece c'e': ha tirato lui.
		TestTrue(TEXT("e `SourceStableUnitId` porta chi ha colpito"), Eventi[I].SourceStableUnitId != 0);
		// I due capi arrivano COPIATI dalla voce, non ricalcolati.
		if (Voci.IsValidIndex(I))
		{
			TestEqual(TEXT("il capo `From` e' quello della voce"), Eventi[I].ArcFrom, Voci[I].SrcCell);
			TestEqual(TEXT("e il capo `To` pure"), Eventi[I].ArcTo, Voci[I].TgtCell);
		}
		// ⛔ E i campi della COPERTURA restano al loro default: un arco non e' un bordo, e scriverli
		// entrambi renderebbe ambigua la geometria per chi consuma.
		TestEqual(TEXT("⛔ `StructureCell` resta al default: non e' un bordo esagonale"),
			Eventi[I].StructureCell, FRTCellId());
	}

	DestroyEnvWorld(S.World);
	return true;
}

/**
 * Un arco SPENTO che incassa senza cadere non e' «abbattuto» — `#3280`, [D-437].
 *
 * 🔴 **La bugia era gia' nel TurnLog, e questa fetta l'avrebbe portata a schermo.** `MakeChange` pone
 * `bBroken = Edge.State != ERTHexArcState::Active`, e `DamageArc` salta **solo** i `Destroyed`: ∴ un arco
 * `Inactive` che incassa senza cadere arrivava con `bBroken == true`, e il produttore scriveva
 * `BridgeDestroyed` per un ponte ancora in piedi.
 *
 * 🔑 **Finche' nessuno leggeva quella voce, il difetto stava fermo. Da `#3280` ne deriva un EVENTO**, cioe'
 * un segno a schermo: [D-437] lo dice per intero — *«l'evento porti lo STATO, non un booleano dedotto, o
 * erediterete la bugia il giorno in cui qualcuno chiamera' `SetArcState(Inactive)`»*. La correzione e' alla
 * **fonte**: l'esito lo decide `Change.State`, e l'evento lo copia.
 *
 * ⚠️ **Oggi il caso e' irraggiungibile in partita** — nessun chiamante di produzione di `SetArcState` — e
 * questo test lo raggiunge chiamandola direttamente. ⛔ Non e' un trucco: e' l'unico modo di esercitare un
 * ramo che esiste, e' legale, ed e' editabile da chi autora una mappa.
 *
 * ⛔ **Cosa questo test NON dice**: che lo stato `Inactive` arrivi al playback. Non arriva —
 * `ERTEnvironmentOutcome` non ha un valore per «spento», e il TurnLog che lo trasporta e' serializzato con
 * un `FormatId`. Cio' che e' garantito e' che l'esito non MENTA, non che sia completo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBridgeInactiveArcDamagedIsNotDestroyedTest,
	"RefactorTactics.Structures.Bridge.InactiveArcDamagedIsNotDestroyed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBridgeInactiveArcDamagedIsNotDestroyedTest::RunTest(const FString&)
{
	// Colpo 20 su integrita' di catalogo: l'arco incassa e **regge**. Se cadesse, `BridgeDestroyed` sarebbe
	// la risposta giusta e il test misurerebbe l'assenza di un difetto che c'e'.
	FRTEnvArcScenario S = EnvMakeArcScenario(/*bBidirectional=*/ true, /*InStructurePower=*/ 20);
	if (!TestTrue(TEXT("scenario costruito"), S.bValid)) { DestroyEnvWorld(S.World); return false; }

	// L'arco si SPEGNE prima del turno: e' lo stato che `MakeChange` confondeva con «rotto».
	URTHexArcLibrary::SetArcState(S.MapActor->MapAsset, S.Ground, S.Upper, ERTHexArcState::Inactive);

	const FRTHexEdge* Prima = URTHexArcLibrary::FindArc(S.MapActor->MapAsset, S.Ground, S.Upper);
	if (!TestTrue(TEXT("⛔ premessa: l'arco e' SPENTO, non abbattuto"),
		Prima != nullptr && Prima->State == ERTHexArcState::Inactive))
	{
		DestroyEnvWorld(S.World);
		return false;
	}
	const int32 IntegritaPrima = Prima->Integrity;

	RunEnvTurn(S.TM);

	const FRTHexEdge* Dopo = URTHexArcLibrary::FindArc(S.MapActor->MapAsset, S.Ground, S.Upper);
	if (!TestTrue(TEXT("⛔ premessa: l'arco ha incassato ed e' ancora in piedi"),
		Dopo != nullptr && Dopo->Integrity < IntegritaPrima && Dopo->State != ERTHexArcState::Destroyed))
	{
		DestroyEnvWorld(S.World);
		return false;
	}

	const TArray<FRTTurnLogEntry> Voci = EnvArcHitEntries(S.TM);
	const TArray<FRTResolvedEvent> Eventi = EnvArcHitEvents(S.TM);
	if (!TestTrue(TEXT("⛔ premessa: il colpo ha prodotto voce ed evento"),
		Voci.Num() > 0 && Eventi.Num() > 0))
	{
		DestroyEnvWorld(S.World);
		return false;
	}

	// --- IL FATTO ------------------------------------------------------------------------------------
	for (const FRTTurnLogEntry& V : Voci)
	{
		TestEqual(TEXT("🔴 la voce dice DANNEGGIATO: lo stato decide, non `bBroken`"),
			static_cast<int32>(V.Outcome), static_cast<int32>(ERTEnvironmentOutcome::BridgeDamaged));
	}
	for (const FRTResolvedEvent& E : Eventi)
	{
		TestEqual(TEXT("🔴 e l'evento non eredita la bugia"),
			static_cast<int32>(E.EnvironmentOutcome),
			static_cast<int32>(ERTEnvironmentOutcome::BridgeDamaged));
	}
	// ⚠️ E lo spegnimento e' RIMASTO: la correzione riguarda la resa dell'esito, non la regola del danno —
	// che resta il non-goal dichiarato di `#3280`.
	TestTrue(TEXT("⚠️ l'arco e' ancora spento: nessuna regola di danno e' cambiata"),
		Dopo->State == ERTHexArcState::Inactive);

	DestroyEnvWorld(S.World);
	return true;
}

// =====================================================================================================
// `#3281` — l'AGGREGATO per bordo, misurato.
//
// ⛔ Questo blocco NON decide che cosa `Next Action` debba fare su un fatto che aggrega piu' azioni: una
// delle tre strade della issue cambierebbe una regola di gioco. Porta il fatto che rende le tre opzioni
// confrontabili invece che elencate.
// =====================================================================================================

namespace
{
	struct FRTEnvAggregateScenario
	{
		UWorld* World = nullptr;
		ARTHexMapActor* MapActor = nullptr;
		ARTTurnManager* TM = nullptr;
		ARTUnit* Ivrin = nullptr;    // (0,0), squadra 1
		ARTUnit* Branth = nullptr;   // (2,0), squadra 0
		bool bValid = false;
	};

	/**
	 * Due unita' ai lati OPPOSTI dello stesso muro, ciascuna con la PROPRIA azione base.
	 *
	 * 🔑 **E' il caso su cui la issue poggia, e finora era una lettura del codice.**
	 * `AccumulateStructureHit` normalizza la coppia di celle e somma per bordo — la sua stessa prosa dichiara
	 * il motivo: *«due attaccanti ai lati opposti colpiscono la stessa barriera»*. Questo scenario e'
	 * esattamente quella frase, giocata.
	 *
	 * Muro **alto** sul bordo W di (1,0). `HexLine((0,0) → (2,0))` attraversa quel lato in avanti,
	 * `HexLine((2,0) → (0,0))` lo attraversa all'indietro: due bordi che la normalizzazione rende uno.
	 *
	 * ⚠️ **Le due azioni sono diverse perche' gli EROI sono diversi**, non perche' un `FName` sia stato
	 * riscritto a mano: `Hero.Ivrin.PulseShot` e `Hero.Branth.ImpactShot` sono due voci di catalogo, e un id
	 * inventato rischierebbe di misurare come si comporta il resolver davanti a un'azione che non esiste.
	 * Entrambe portata >= 2, entrambe in fase `Attack`: lo stesso `Blast`, che e' la premessa della issue.
	 *
	 * ⚠️ La capacita' di sfondare si dichiara sull'istanza, come in tutti gli scenari di struttura di
	 * questo file: nessuno dei due attacchi base dichiara `DamageStructure` a catalogo.
	 */
	FRTEnvAggregateScenario EnvMakeAggregateScenario()
	{
		FRTEnvAggregateScenario S;
		S.World = MakeEnvWorld();
		if (!S.World) { return S; }
		S.MapActor = SpawnEnvMap(S.World);
		if (!S.MapActor || !S.MapActor->MapAsset) { return S; }

		// ⚠️ Si parte dalla cella ESISTENTE, come in `EnvMakeWalledBreachScenario`: costruirne una nuova
		// con lo stesso `Id` sostituirebbe quella che l'arena ha posato, perdendone terreno e proprieta'.
		const FRTCellId Muraglia(1, 0);
		const FRTHexCellData* Esistente = S.MapActor->MapAsset->FindCell(Muraglia);
		if (!Esistente) { return S; }
		FRTHexCellData ColMuro = *Esistente;
		ColMuro.Covers.Add(FRTHexCover(ERTHexDirection::W, ERTHexCoverType::High,
			FRTHexCover::DefaultIntegrity(ERTHexCoverType::High)));
		S.MapActor->MapAsset->AddOrUpdateCell(ColMuro);
		S.MapActor->MapAsset->SortCells();

		S.Ivrin = SpawnEnvUnit(S.World, 1, FRTCellId(0, 0), URTHeroCatalogLibrary::MakeIvrin());
		S.Branth = SpawnEnvUnit(S.World, 0, FRTCellId(2, 0), URTHeroCatalogLibrary::MakeBranth());
		S.TM = S.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!S.Ivrin || !S.Branth || !S.TM) { return S; }
		if (!S.Ivrin->Abilities.IsValidIndex(0) || !S.Branth->Abilities.IsValidIndex(0)) { return S; }
		if (!S.Ivrin->Abilities[0] || !S.Branth->Abilities[0]) { return S; }

		// Dieci a testa, e il numero conta: e' cio' che rende distinguibile «si sono sommati» da «ha sparato
		// uno solo» quando si legge l'integrita' residua.
		S.Ivrin->Abilities[0]->Def.Effects.Add(FRTActionEffectSpec(ERTActionEffect::DamageStructure, 10));
		S.Ivrin->PlannedAbilityIndex = 0;
		S.Ivrin->PlannedAttackTarget = S.Branth;

		S.Branth->Abilities[0]->Def.Effects.Add(FRTActionEffectSpec(ERTActionEffect::DamageStructure, 10));
		S.Branth->PlannedAbilityIndex = 0;
		S.Branth->PlannedAttackTarget = S.Ivrin;

		S.bValid = true;
		return S;
	}
}

/**
 * Due AZIONI diverse sullo stesso bordo producono UN fatto solo, che non ne nomina nessuna — `#3281`.
 *
 * 🔴 **E' la premessa su cui poggia tutta la issue, e finora era una lettura del codice.**
 * `AccumulateStructureHit` somma per bordo: se il caso dell'aggregato **non fosse raggiungibile** — se due
 * azioni diverse non potessero mai colpire lo stesso lato nello stesso `Blast` — allora «quale azione
 * nominare» avrebbe sempre una risposta, e la strada corretta sarebbe semplicemente propagare l'identita'
 * dell'azione invece di decidere che cosa significhi l'identita' di un aggregato.
 *
 * ⛔ **Cio' che questo gate NON fa e' scegliere fra le tre strade della issue**: una di esse — non
 * aggregare piu' i colpi di azioni diverse — cambierebbe l'ESITO e non la presentazione, ed e' per questo
 * che l'integrita' residua e' asserita qui e non lasciata implicita: e' il valore che quella strada
 * cambierebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackAggregatedStructureHitDoesNotNameOneActionTest,
	"RefactorTactics.Playback.AggregatedStructureHitDoesNotNameOneAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackAggregatedStructureHitDoesNotNameOneActionTest::RunTest(const FString&)
{
	FRTEnvAggregateScenario S = EnvMakeAggregateScenario();
	if (!TestTrue(TEXT("scenario costruito"), S.bValid)) { DestroyEnvWorld(S.World); return false; }

	const FName AzioneIvrin = S.Ivrin->Abilities[0]->Def.ActionId;
	const FName AzioneBranth = S.Branth->Abilities[0]->Def.ActionId;

	// ⛔ PREMESSA, e va LETTA dal catalogo invece che assunta: senza due identita' d'azione diverse il caso
	// non e' quello dell'aggregato, e il gate misurerebbe due colpi della STESSA azione — dove nominarla
	// sarebbe banale e la issue non esisterebbe.
	TestFalse(TEXT("⛔ premessa: la prima azione ha un'identita'"), AzioneIvrin.IsNone());
	TestFalse(TEXT("⛔ premessa: anche la seconda"), AzioneBranth.IsNone());
	TestNotEqual(TEXT("⛔ premessa: e sono DIVERSE"), AzioneIvrin, AzioneBranth);

	RunEnvTurn(S.TM);

	TArray<FRTTurnLogEntry> Voci;
	for (const FRTTurnLogEntry& E : S.TM->GetTurnLog())
	{
		if (URTTurnLogLibrary::IsStructureHit(E)) { Voci.Add(E); }
	}
	const TArray<FRTResolvedEvent> Eventi = EnvStructureHitEvents(S.TM);

	if (!TestEqual(TEXT("🔴 due azioni diverse sullo stesso bordo: UNA voce sola"), Voci.Num(), 1))
	{
		DestroyEnvWorld(S.World);
		return false;
	}
	TestEqual(TEXT("🔴 e UN evento solo"), Eventi.Num(), 1);

	// ⚠️ **Ed e' l'integrita' residua a dire che i due colpi si sono SOMMATI**, non il conteggio delle
	// voci: una voce sola la darebbe anche uno scenario in cui ha sparato un attaccante solo. Il muro alto
	// nasce a 50 e ciascuno dichiara 10 → 30.
	//
	// ⌨ **Questa riga diceva che 40 sarebbe "l'opzione (b) della issue, quella che cambia l'esito". Falso,
	// e misurato.** Disattivando l'aggregazione — che E' la (b) — escono DUE voci con `Amount` 40 e 30, e
	// l'integrita' finale del muro resta **30**: `DamageFace` sottrae e satura, quindi due colpi da 10
	// lasciano cio' che lascia un colpo da 20. Il 40 di questa riga veniva da un'altra mutazione — *«fonde
	// ma non somma»* — che non e' la (b) e quella si' cambia l'esito.
	//
	// 🔑 **Cosa asserisce davvero questa riga, allora**: che i due intenti hanno colpito lo STESSO bordo e
	// che il danno di entrambi e' arrivato. E' la premessa dell'aggregato, non il suo costo competitivo:
	// la (b) cambia il REGISTRO della traccia — due voci, due `Amount`, due azioni nominate — non lo stato
	// del gioco, ed e' su `#3281` che la decisione e la ragione vera per scartarla sono registrate.
	TestEqual(TEXT("⚠️ i due colpi si sono SOMMATI: 50 - 10 - 10 = 30"), Voci[0].Amount, 30);

	// --- ∴ «QUALE AZIONE» NON HA UNA RISPOSTA -------------------------------------------------------
	if (Eventi.Num() == 1)
	{
		TestTrue(TEXT("∴ il fatto aggregato non nomina nessuna azione: NAME_None"),
			Eventi[0].ActionId.IsNone());
		// ⛔ E non e' che ne abbia scelta una: NESSUNA delle due compare. Asserirlo e' cio' che distingue
		// «non c'e' una risposta» da «la risposta e' arbitraria ma esiste».
		TestNotEqual(TEXT("⛔ non e' la prima"), Eventi[0].ActionId, AzioneIvrin);
		TestNotEqual(TEXT("⛔ e non e' la seconda"), Eventi[0].ActionId, AzioneBranth);
	}

	// ⚠️ **L'ATTACCANTE invece una risposta ce l'ha, ed e' un RAPPRESENTANTE.** `FRTStructureHit`
	// dichiara di portare «chi ha colpito per primo in ordine canonico», e misurarlo qui mostra che il campo
	// gemello ha gia' fatto, per l'unita', la scelta che `ActionId` non ha fatto per l'azione: uno dei due, non
	// l'insieme. E' l'asimmetria che la decisione della issue deve sciogliere.
	TestTrue(TEXT("⚠️ l'attaccante e' UNO dei due, scelto in ordine canonico"),
		Voci[0].UnitId == S.Ivrin->StableUnitId || Voci[0].UnitId == S.Branth->StableUnitId);

	// --- ∴ LA CONSEGUENZA, MISURATA ------------------------------------------------------------------
	//
	// ✅ **`Next Action` SI FERMA sul muro che cade** — `#3281`, [D-437]. ⏱️ *Questa riga asseriva il
	// contrario, ed era la misura del difetto da cui la issue parte: allora `NAME_None` significava
	// «nessuna azione dietro» per ogni tipo, e l'evento non era un confine.*
	//
	// 🔑 **Il valore non e' cambiato, e' cambiato il suo SIGNIFICATO su questo tipo.** Il produttore nomina
	// l'azione quando l'autore e' uno e tace **solo** sull'aggregato: ∴ un vuoto qui dice *«piu' di uno
	// l'ha fatto»*, che e' un fatto con autori — non un fatto senza autore. E' la stessa `NAME_None` di
	// prima, letta da un contratto diverso.
	const TArray<FRTResolvedEvent>& Timeline = S.TM->ResolvedTimelineForTest();
	int32 IndiceColpo = INDEX_NONE;
	for (int32 i = 0; i < Timeline.Num(); ++i)
	{
		if (Timeline[i].Type == ERTResolvedEventType::StructureHit) { IndiceColpo = i; break; }
	}
	if (TestTrue(TEXT("il colpo sta nella timeline"), IndiceColpo != INDEX_NONE))
	{
		TestEqual(TEXT("✅ ∴ l'aggregato E' un confine d'atto: `Next Action` ci si ferma"),
			URTPlaybackLibrary::NextActionBoundary(Timeline, IndiceColpo - 1), IndiceColpo);
	}

	DestroyEnvWorld(S.World);
	return true;
}

/**
 * Con UNA azione sola il colpo la NOMINA — `#3281`, [D-437].
 *
 * 🔴 **E' il gate che mancava del tutto, e non e' un contorno: e' cio' che rende il silenzio una SCELTA.**
 * Dopo questa decisione `NAME_None` su `StructureHit` significa *«piu' di uno l'ha fatto»* ed e' un confine
 * d'atto. ∴ un produttore che **dimenticasse** di popolare l'identita' non perderebbe piu' un confine: ne
 * **inventerebbe** uno — e un `Next Action` che si ferma su un atto che non esiste e' meno leggibile di uno
 * che ne salta uno.
 *
 * ⛔ Senza questo gate, il caso comune — un attaccante, un muro — sarebbe indistinguibile dall'aggregato, e
 * la decisione si reggerebbe su un solo verso.
 *
 * ⚠️ **E l'azione si legge dal CATALOGO, non da un `FName` scritto a mano**: si asserisce che l'evento porti
 * *quella* identita', non «una qualsiasi non vuota».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackSingleActionStructureHitNamesItsActionTest,
	"RefactorTactics.Playback.SingleActionStructureHitNamesItsAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackSingleActionStructureHitNamesItsActionTest::RunTest(const FString&)
{
	// Il muro ALTO, e un attaccante solo: `EnvMakeWalledBreachScenario` e' esattamente questa scena, ed e'
	// gia' la fixture dei gate di `#2828`.
	FRTEnvBreachScenario S = EnvMakeWalledBreachScenario(/*InStructurePower=*/ 10);
	if (!TestTrue(TEXT("scenario costruito"), S.bValid)) { DestroyEnvWorld(S.World); return false; }

	const FName Azione = S.Breacher->Abilities[0]->Def.ActionId;
	if (!TestFalse(TEXT("⛔ premessa: l'azione dell'unico attaccante ha un'identita'"), Azione.IsNone()))
	{
		DestroyEnvWorld(S.World);
		return false;
	}

	RunEnvTurn(S.TM);

	const TArray<FRTResolvedEvent> Eventi = EnvStructureHitEvents(S.TM);
	if (!TestEqual(TEXT("⛔ premessa: un attaccante, un colpo alla struttura"), Eventi.Num(), 1))
	{
		DestroyEnvWorld(S.World);
		return false;
	}

	// --- IL FATTO ------------------------------------------------------------------------------------
	TestEqual(TEXT("✅ il colpo NOMINA l'azione che l'ha prodotto"), Eventi[0].ActionId, Azione);
	TestFalse(TEXT("⛔ e non tace: il silenzio e' riservato all'aggregato"), Eventi[0].ActionId.IsNone());

	// La voce di TurnLog la porta pure, ed e' da li' che l'evento la copia: se divergessero, uno dei due
	// canali starebbe raccontando un'altra partita.
	const FRTTurnLogEntry* Voce = S.TM->GetTurnLog().FindByPredicate(
		[](const FRTTurnLogEntry& E) { return URTTurnLogLibrary::IsStructureHit(E); });
	if (TestNotNull(TEXT("la voce di TurnLog c'e'"), Voce))
	{
		TestEqual(TEXT("e i due canali portano la stessa azione"), Voce->ActionId, Azione);
	}

	// --- ∴ ED E' UN CONFINE D'ATTO, ma per la ragione ORDINARIA ---------------------------------------
	//
	// ⚠️ **Non per l'eccezione**: qui l'azione c'e', quindi il colpo e' un confine come lo sarebbe
	// qualunque evento che apre un atto nuovo. L'eccezione riguarda il vuoto, e qui non c'e' vuoto.
	const TArray<FRTResolvedEvent>& Timeline = S.TM->ResolvedTimelineForTest();
	int32 IndiceColpo = INDEX_NONE;
	for (int32 i = 0; i < Timeline.Num(); ++i)
	{
		if (Timeline[i].Type == ERTResolvedEventType::StructureHit) { IndiceColpo = i; break; }
	}
	if (TestTrue(TEXT("il colpo sta nella timeline"), IndiceColpo != INDEX_NONE))
	{
		TestEqual(TEXT("✅ `Next Action` si ferma sul muro"),
			URTPlaybackLibrary::NextActionBoundary(Timeline, IndiceColpo - 1), IndiceColpo);
	}

	DestroyEnvWorld(S.World);
	return true;
}

/**
 * Un autore, DUE eventi, la stessa azione: **un** atto — `#3281` e `#3279`, [D-437].
 *
 * 🔑 **E' il gate che lega le due decisioni, e senza di esso una delle due sarebbe falsa.** Su una barriera
 * dichiarata sulle **due facce** un colpo solo produce due `StructureHit` (`#3279`, dove i due eventi
 * restano perche' sono due fatti). Se quei due portassero `NAME_None`, dopo `#3281` diventerebbero **due**
 * confini d'atto — e `Next Action` si fermerebbe due volte per un colpo solo di un attaccante solo.
 *
 * ✅ Non succede, e la ragione sta nel dato: le due facce ricevono gli **stessi** `IntentIndices` — sono lo
 * stesso colpo letto dai due lati — quindi portano la stessa azione, e `NextActionBoundary` legge *«piu'
 * eventi con lo stesso `ActionId` sono UN atto»*.
 *
 * ⚠️ **E' anche il caso che ha ucciso l'opzione (a)**: mostra che il rapporto evento↔azione non e'
 * funzionale in nessuno dei due versi — un fatto puo' avere piu' autori (`#3281`) e un autore puo' produrre
 * piu' fatti (`#3279`).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackRedundantFaceStructureHitsAreOneActTest,
	"RefactorTactics.Playback.RedundantFaceStructureHitsAreOneAct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackRedundantFaceStructureHitsAreOneActTest::RunTest(const FString&)
{
	FRTEnvBreachScenario S = EnvMakeRedundantFaceScenario(/*InStructurePower=*/ 10, /*InHeightDelta=*/ 300);
	if (!TestTrue(TEXT("scenario costruito"), S.bValid)) { DestroyEnvWorld(S.World); return false; }

	const FName Azione = S.Breacher->Abilities[0]->Def.ActionId;
	TestFalse(TEXT("⛔ premessa: l'unico attaccante ha un'azione con identita'"), Azione.IsNone());

	RunEnvTurn(S.TM);

	const TArray<FRTResolvedEvent> Eventi = EnvStructureHitEvents(S.TM);
	// ⛔ PREMESSA: la faccia ridondante produce davvero DUE eventi. Con uno solo questo gate misurerebbe
	// un'ovvieta' — un evento solo e' sempre un atto solo.
	if (!TestEqual(TEXT("⛔ premessa: un colpo, due eventi (la faccia ridondante di #3279)"),
		Eventi.Num(), 2))
	{
		DestroyEnvWorld(S.World);
		return false;
	}

	// --- IL FATTO ------------------------------------------------------------------------------------
	TestEqual(TEXT("✅ il primo nomina l'azione"), Eventi[0].ActionId, Azione);
	TestEqual(TEXT("✅ e il secondo la STESSA: sono lo stesso colpo letto dai due lati"),
		Eventi[1].ActionId, Azione);

	// --- ∴ UN ATTO SOLO, e si misura con due chiamate consecutive ------------------------------------
	const TArray<FRTResolvedEvent>& Timeline = S.TM->ResolvedTimelineForTest();
	int32 Primo = INDEX_NONE;
	int32 Secondo = INDEX_NONE;
	for (int32 i = 0; i < Timeline.Num(); ++i)
	{
		if (Timeline[i].Type != ERTResolvedEventType::StructureHit) { continue; }
		if (Primo == INDEX_NONE) { Primo = i; } else if (Secondo == INDEX_NONE) { Secondo = i; }
	}
	if (TestTrue(TEXT("i due colpi stanno nella timeline"), Primo != INDEX_NONE && Secondo != INDEX_NONE))
	{
		// 🔴 Partendo DAL primo colpo, il confine successivo non e' il secondo: sono lo stesso atto.
		TestNotEqual(TEXT("🔴 `Next Action` NON si ferma una seconda volta sullo stesso colpo"),
			URTPlaybackLibrary::NextActionBoundary(Timeline, Primo), Secondo);
	}

	DestroyEnvWorld(S.World);
	return true;
}

/**
 * **La cura SINGOLA aggancia l'alleato e lo raggiunge dove e' arrivato** — D-493 (#3609).
 *
 * Decisione d'autore: *«aggancia l'unità, come eccezione dichiarata in catalogo. il cura ha un tipo di tiro diverso
 * da un normale shoot»*. L'alleato parte FUORI portata (4 celle, portata 3) e scatta DENTRO prima del Blast: la cura
 * che aggancia legge la sua cella viva e lo cura; con la mira congelata avrebbe misurato la cella di partenza e
 * sarebbe mancata. Esiti opposti nelle due regole, ed e' cio' che rende il test una misura dell'aggancio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionHealTracksTheAllyTest,
	"RefactorTactics.Actions.Heal.TracksTheAllyItHeals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionHealTracksTheAllyTest::RunTest(const FString&)
{
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnEnvMap(World);
	ARTUnit* Caster = SpawnEnvUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Ally = SpawnEnvUnit(World, 0, FRTCellId(4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!Caster || !Ally || !TM) { DestroyEnvWorld(World); return false; }

	const FRTActionDef Def = URTCatalogLibrary::FindCoreAction(FName(TEXT("Action.Heal")));
	if (!TestTrue(TEXT("premessa: la cura dichiara di agganciare (D-493)"), Def.DeclaresTracking())
		|| !TestTrue(TEXT("premessa: al lock-in l'alleato e' FUORI portata"),
			URTHexLibrary::HexDistance(Caster->Cell, Ally->Cell) > Def.RangeCells))
	{
		DestroyEnvWorld(World); return false;
	}

	Ally->Health -= 30;
	const int32 Prima = Ally->Health;
	PlanEnvAction(Caster, TEXT("Action.Heal"), Ally);
	const int32 Scatto = RTAbilityFixtures::AddCoreAbility(Ally, TEXT("Action.Reposition"));
	Ally->PlannedDashAbility = Scatto;
	Ally->PlannedDashCell = FRTCellId(2, 0); // dentro la portata
	RunEnvTurn(TM);

	if (!TestEqual(TEXT("premessa: l'alleato e' davvero scattato dentro la portata"), Ally->Cell, FRTCellId(2, 0)))
	{
		DestroyEnvWorld(World); return false;
	}
	TestTrue(TEXT("🔴 la cura lo segue e lo raggiunge dove e' arrivato"), Ally->Health > Prima);
	DestroyEnvWorld(World);
	return true;
}

/**
 * **`ModifyArc` apre l'arco verso la MIRA congelata** — [D-419] (#3609).
 *
 * Il bersaglio e' in portata al lock-in (2 celle, portata 3) e scatta fuori (4 celle) prima del Blast. L'arco e' la
 * coppia di celle, quindi segue la mira congelata: nasce. Con la cella viva — com'era fino a #3609 — sarebbe stato
 * rifiutato «fuori portata». Esiti opposti nelle due regole.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionModifyArcFrozenAimTest,
	"RefactorTactics.Actions.ModifyArc.OpensTowardTheFrozenAim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionModifyArcFrozenAimTest::RunTest(const FString&)
{
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World);
	ARTUnit* Caster = SpawnEnvUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Target = SpawnEnvUnit(World, 1, FRTCellId(2, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!Caster || !Target || !TM) { DestroyEnvWorld(World); return false; }

	const FRTActionDef Def = URTCatalogLibrary::FindCoreAction(FName(TEXT("Action.ModifyArc")));
	const FRTCellId Scattata(4, 0);
	if (!TestTrue(TEXT("premessa: al lock-in il bersaglio e' in portata"),
			URTHexLibrary::HexDistance(Caster->Cell, Target->Cell) <= Def.RangeCells)
		|| !TestTrue(TEXT("premessa: la cella dello scatto e' FUORI portata"),
			URTHexLibrary::HexDistance(Caster->Cell, Scattata) > Def.RangeCells)
		|| !TestFalse(TEXT("premessa: ModifyArc non dichiara di agganciare"), Def.DeclaresTracking()))
	{
		DestroyEnvWorld(World); return false;
	}

	const int32 ArcsBefore = MapActor->MapAsset->Transitions.Num();
	PlanEnvAction(Caster, TEXT("Action.ModifyArc"), Target);
	const int32 Scatto = RTAbilityFixtures::AddCoreAbility(Target, TEXT("Action.Reposition"));
	Target->PlannedDashAbility = Scatto;
	Target->PlannedDashCell = Scattata;
	RunEnvTurn(TM);

	if (!TestEqual(TEXT("premessa: il bersaglio e' davvero scattato fuori portata"), Target->Cell, Scattata))
	{
		DestroyEnvWorld(World); return false;
	}
	// Un arco sono piu' transizioni (misurato: +2), come in `ModifyArc.BumpsChunkRevision`: si conta «di piu'».
	TestTrue(TEXT("🔴 l'arco e' nato verso la mira congelata, non rifiutato sulla cella dello scatto"),
		MapActor->MapAsset->Transitions.Num() > ArcsBefore);
	DestroyEnvWorld(World);
	return true;
}

/**
 * **L'azione ambientale agisce sulla MIRA congelata, che il bersaglio cammini o scatti** — [D-419] (#3609).
 *
 * Le ambientali risolvono in Cleanup, DOPO il Move: leggendo la cella viva inseguivano anche il movimento normale,
 * non solo lo scatto. Il bersaglio puntato al lock-in in (2,0) va in (3,0), una volta camminando (Move) e una
 * scattando (Dash, prima del Blast): il fuoco nasce in (2,0), e la cella d'arrivo resta com'era. Con la cella viva —
 * com'era fino a #3609 — sarebbe stato il contrario in entrambi i casi.
 *
 * Controllo positivo: a scena ferma il fuoco nasce sotto il bersaglio. Un mondo per caso, perche' la superficie
 * bruciata di un caso non resti nel successivo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionIgniteFrozenAimTest,
	"RefactorTactics.Actions.Ignite.BurnsWhereItWasAimed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionIgniteFrozenAimTest::RunTest(const FString&)
{
	enum class EMoto : uint8 { Fermo, Cammina, Scatta };
	const FRTCellId Puntata(2, 0);
	const FRTCellId Arrivo(3, 0);

	auto Esegui = [this, &Puntata, &Arrivo](const TCHAR* Caso, EMoto Moto)
	{
		UWorld* World = MakeEnvWorld();
		if (!TestNotNull(FString::Printf(TEXT("%s: world di prova"), Caso), World)) { return; }
		ARTHexMapActor* MapActor = SpawnEnvMap(World);
		ARTUnit* Caster = SpawnEnvUnit(World, 0, FRTCellId(0, 0));
		ARTUnit* Target = SpawnEnvUnit(World, 1, Puntata);
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Caster || !Target || !TM) { DestroyEnvWorld(World); return; }

		PlanEnvAction(Caster, TEXT("Action.Ignite"), Target);
		if (Moto == EMoto::Cammina)
		{
			Target->PlannedCell = Arrivo; // movimento NORMALE: risolve nel Move, dopo il Blast e prima del Cleanup
		}
		else if (Moto == EMoto::Scatta)
		{
			Target->PlannedDashAbility = RTAbilityFixtures::AddCoreAbility(Target, TEXT("Action.Reposition"));
			Target->PlannedDashCell = Arrivo; // scatto: risolve PRIMA del Blast
		}
		RunEnvTurn(TM);

		auto SurfaceAt = [MapActor](const FRTCellId& Cell)
		{
			const FRTHexCellData* Data = MapActor->MapAsset ? MapActor->MapAsset->FindCell(Cell) : nullptr;
			return Data ? Data->Surface : ERTHexSurface::Floor;
		};
		const FRTCellId Atteso = Moto == EMoto::Fermo ? Puntata : Arrivo;
		if (!TestEqual(FString::Printf(TEXT("%s: premessa, il bersaglio e' dove il caso lo porta"), Caso),
			Target->Cell, Atteso))
		{
			DestroyEnvWorld(World); return;
		}
		TestTrue(FString::Printf(TEXT("%s: 🔴 brucia la cella puntata al lock-in"), Caso),
			SurfaceAt(Puntata) == ERTHexSurface::Fire);
		if (Moto != EMoto::Fermo)
		{
			TestTrue(FString::Printf(TEXT("%s: e non quella dove il bersaglio e' arrivato"), Caso),
				SurfaceAt(Arrivo) != ERTHexSurface::Fire);
		}
		DestroyEnvWorld(World);
	};

	Esegui(TEXT("scena ferma (controllo positivo)"), EMoto::Fermo);
	Esegui(TEXT("il bersaglio cammina"), EMoto::Cammina);
	Esegui(TEXT("il bersaglio scatta"), EMoto::Scatta);
	return true;
}

/**
 * #3681, [D-502] — il muro colpito ha UN verdetto, e ogni vista del fatto lo legge.
 *
 * 🔴 **Il caso che solo questo evento produce**: un attaccante che chi guarda non vede spara contro un muro alto
 * e non colpisce nessuno. Non nascono ne' `Attack` ne' `AttackFootprint`, e il bordo colpito resta l'unica
 * notizia — la fila da cui e' arrivato il colpo. ⏱️ *Fino a #3681 lo stesso fatto aveva tre regole: la voce
 * di TurnLog col verdetto di chi aveva sparato, l'evento di timeline senza verdetto, la riga del combat log
 * pubblica.*
 *
 * 🔑 **Tre squadre, perche' le domande sono tre**: chi ha sparato (squadra 1, D-502 (b)), chi guarda il muro
 * senza vedere chi spara (squadra 0, dietro il muro), e chi non vede nessuna delle due celle (squadra 2, a
 * nove celle con una visione di sei). Con due sole squadre l'arena di prova le farebbe vedere tutto a tutti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPrivacyWallHitOneVerdictTest,
	"RefactorTactics.Privacy.WallHitHasOneVerdictForEveryView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPrivacyWallHitOneVerdictTest::RunTest(const FString&)
{
	UWorld* World = MakeEnvWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnEnvMap(World, /*Radius=*/ 10);
	if (!TestTrue(TEXT("mappa"), MapActor && MapActor->MapAsset)) { DestroyEnvWorld(World); return false; }

	// Il muro alto di `EnvMakeWalledBreachScenario`: sul bordo W di (1,0), chi spara in (0,0).
	const FRTCellId Portante(1, 0);
	const FRTCellId Spara(0, 0);
	FRTHexCellData ColMuro = *MapActor->MapAsset->FindCell(Portante);
	ColMuro.Covers.Add(FRTHexCover(ERTHexDirection::W, ERTHexCoverType::High,
		FRTHexCover::DefaultIntegrity(ERTHexCoverType::High)));
	MapActor->MapAsset->AddOrUpdateCell(ColMuro);
	MapActor->MapAsset->SortCells();

	ARTUnit* Breacher = SpawnEnvUnit(World, 1, Spara);
	ARTUnit* Defender = SpawnEnvUnit(World, 0, FRTCellId(2, 0));
	ARTUnit* Lontano = SpawnEnvUnit(World, 2, FRTCellId(-9, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestTrue(TEXT("scenario costruito"), Breacher && Defender && Lontano && TM))
	{
		DestroyEnvWorld(World);
		return false;
	}
	Breacher->Abilities[0]->Def.Effects.Add(FRTActionEffectSpec(ERTActionEffect::DamageStructure, 30));
	Breacher->PlannedAbilityIndex = 0;
	Breacher->PlannedAttackTarget = Defender;

	RunEnvTurn(TM);

	const FRTTurnLogEntry* Voce = nullptr;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (URTTurnLogLibrary::IsStructureHit(E)) { Voce = &E; break; }
	}
	const TArray<FRTResolvedEvent> Eventi = EnvStructureHitEvents(TM);
	if (!TestNotNull(TEXT("⛔ il turno ha registrato il muro colpito"), Voce)
		|| !TestTrue(TEXT("⛔ e la timeline lo porta"), Eventi.Num() > 0))
	{
		DestroyEnvWorld(World);
		return false;
	}

	// Il verdetto della voce, e contro cosa e' stato congelato.
	const FRTKnowledgeVerdict V = Voce->Verdict;
	TestTrue(TEXT("il soggetto del verdetto e' il bordo"), Voce->VerdictSubject.bEdge);
	TestEqual(TEXT("con la cella che porta il muro"), Voce->VerdictSubject.EdgeCell, Portante);
	TestEqual(TEXT("e quella da cui e' arrivato il colpo"), Voce->VerdictSubject.EdgeToward, Spara);
	TestTrue(TEXT("e la sola squadra dell'autore"),
		Voce->VerdictSubject.AuthorTeamIds.Num() == 1 && Voce->VerdictSubject.AuthorTeamIds[0] == 1);
	TestEqual(TEXT("la voce nomina ancora chi ha sparato: `UnitId` entra nell'hash"), Voce->UnitId,
		Breacher->StableUnitId);

	TestTrue(TEXT("(b) chi ha sparato vede il proprio colpo"), V.AllowsTeam(1));
	TestTrue(TEXT("chi guarda il muro lo vede colpire, anche senza vedere chi spara"), V.AllowsTeam(0));
	TestFalse(TEXT("🔴 chi non vede nessuna delle due celle non lo sa"), V.AllowsTeam(2));

	// Un fatto, un verdetto: ogni vista legge lo stesso.
	for (const FRTResolvedEvent& Ev : Eventi)
	{
		TestTrue(TEXT("l'evento di timeline porta il verdetto della voce"), Ev.SourceVerdict == V);
	}
	const TArray<FRTResolvedEvent>& Timeline = TM->ResolvedTimelineForTest();
	for (const int32 Squadra : { 0, 1, 2 })
	{
		bool bRiga = false;
		for (const FString& Riga : TM->GetRecentEventsForTeam(Squadra))
		{
			bRiga |= Riga.Contains(TEXT("Copertura"));
		}
		TestEqual(*FString::Printf(TEXT("squadra %d: la riga del combat log segue il verdetto"), Squadra),
			bRiga, V.AllowsTeam(Squadra));

		bool bSegno = false;
		for (const FRTBlastSequenceElement& E :
			URTPlaybackLibrary::BuildBlastSequence(Timeline, TArray<FRTBlastSequenceElement>(), 0, Squadra))
		{
			bSegno |= Timeline.IsValidIndex(E.TimelineIndex)
				&& Timeline[E.TimelineIndex].Type == ERTResolvedEventType::StructureHit;
		}
		TestEqual(*FString::Printf(TEXT("squadra %d: il segno entra nella sequenza del Blast solo col verdetto"), Squadra),
			bSegno, V.AllowsTeam(Squadra));
	}

	DestroyEnvWorld(World);
	return true;
}

/**
 * `#3701`, [D-502] — **il rilevatore del cancello**: chi chiama `ApplyStructureDamage`, e dentro quale funzione.
 *
 * 🔑 **Puro, perche' lo misuri un controllo positivo**: lo sweep di produzione e il testo piantato passano dallo
 * STESSO rilevatore. Un rilevatore che non vede niente farebbe verde lo sweep per cecita'.
 *
 * ⛔ **Riconosce la forma SCRITTA**, come gli altri cancelli che leggono il sorgente (`UI.CombatLogHasOneProducerPerEntry`,
 * `Pacing.ResolverNeverBranchesOnATime`): un puntatore a funzione o un `#define` che avvolgesse la chiamata
 * passerebbero.
 */
namespace RTStructureHitGuard
{
	struct FChiamata
	{
		FString File;
		int32 Riga = 0;
		/** La funzione che la racchiude: l'ultima firma `Classe::Nome(` a colonna zero sopra la chiamata. */
		FString Funzione;
	};

	/** Un commento che nomina la funzione non e' una chiamata. `* ` con lo spazio: una dereferenza non lo scrive. */
	bool ERigaDiCommento(const FString& Riga)
	{
		const FString T = Riga.TrimStart();
		return T.StartsWith(TEXT("//")) || T.StartsWith(TEXT("/*")) || T.StartsWith(TEXT("*/"))
			|| T.StartsWith(TEXT("* ")) || T.Equals(TEXT("*"));
	}

	FString FunzioneCheRacchiude(const TArray<FString>& Righe, int32 Indice)
	{
		for (int32 k = Indice; k >= 0; --k)
		{
			const FString& R = Righe[k];
			if (R.IsEmpty() || FChar::IsWhitespace(R[0]) || R.StartsWith(TEXT("#")) || ERigaDiCommento(R)) { continue; }
			const int32 Aperta = R.Find(TEXT("("), ESearchCase::CaseSensitive);
			if (Aperta == INDEX_NONE) { continue; }
			const FString Prima = R.Left(Aperta);
			const int32 Scope = Prima.Find(TEXT("::"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
			if (Scope == INDEX_NONE) { continue; }
			return Prima.Mid(Scope + 2);
		}
		return FString();
	}

	/**
	 * Le chiamate in `Testo`. La dichiarazione e la definizione si riconoscono dalla FORMA — il tipo di ritorno
	 * nella prima, il primo parametro nella seconda — e si contano a parte: sono l'anti-vacuita' dello sweep.
	 */
	void TrovaChiamate(const FString& Testo, const FString& Nome, TArray<FChiamata>& Out, int32& OutDichiarazioni)
	{
		TArray<FString> Righe;
		Testo.ParseIntoArrayLines(Righe, /*InCullEmpty*/ false);
		for (int32 i = 0; i < Righe.Num(); ++i)
		{
			const FString& Riga = Righe[i];
			if (ERigaDiCommento(Riga) || !Riga.Contains(TEXT("ApplyStructureDamage("), ESearchCase::CaseSensitive))
			{
				continue;
			}
			if (Riga.Contains(TEXT("static TArray<FRTCoverDamageResult> ApplyStructureDamage("), ESearchCase::CaseSensitive)
				|| Riga.Contains(TEXT("URTHexCoverLibrary::ApplyStructureDamage(URTHexMapAsset*"), ESearchCase::CaseSensitive))
			{
				++OutDichiarazioni;
				continue;
			}
			Out.Add({ Nome, i + 1, FunzioneCheRacchiude(Righe, i) });
		}
	}
}

/**
 * `#3701`, [D-502] — **un muro si colpisce solo nel Blast**: `ApplyStructureDamage` ha un solo chiamante di
 * produzione, `ARTTurnManager::ApplyEnvironmentChanges`.
 *
 * 🔑 **Perche' e' un cancello e non un test di comportamento**: il verdetto di un muro colpito si congela con
 * `FreezeEdgeVerdict` su `TeamKnowledgeState`, e quella conoscenza si rinfresca solo in Pianificazione e
 * all'inizio del Blast (`RefreshTeamKnowledgeForBlast`). Un produttore nel Dash congelerebbe il verdetto sulle
 * posizioni d'inizio turno, non su quelle della fase in cui il colpo avviene, come D-502 (6c) chiede — e il
 * playback mostra gli `StructureHit` solo nel Blast. Oggi quel produttore non esiste, e nessun test di
 * comportamento potrebbe attraversarlo: questo cade il giorno in cui nasce, e il suo messaggio dice cosa serve.
 *
 * Perimetro: il modulo di gioco (`Source/RefactorTactics`) meno i test. L'Editor non risolve partite.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStructureHitHasOneProducerInTheBlastTest,
	"RefactorTactics.Structures.HitHasOneProducerInTheBlast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStructureHitHasOneProducerInTheBlastTest::RunTest(const FString&)
{
	using namespace RTStructureHitGuard;

	const FString Radice = FPaths::Combine(FPaths::ProjectDir(), TEXT("Source"), TEXT("RefactorTactics"));
	TArray<FString> Files;
	for (const TCHAR* Ext : { TEXT("*.cpp"), TEXT("*.h") })
	{
		TArray<FString> Trovati;
		IFileManager::Get().FindFilesRecursive(Trovati, *Radice, Ext, /*Files=*/ true, /*Dirs=*/ false,
			/*bClearFileNames=*/ false);
		Files.Append(Trovati);
	}
	// ⛔ Zero file letti vuol dire un percorso sbagliato, non un modulo pulito.
	if (!TestTrue(*FString::Printf(TEXT("lo sweep ha trovato i sorgenti (%s)"), *Radice), Files.Num() > 0)) { return false; }

	TArray<FChiamata> Chiamate;
	int32 Dichiarazioni = 0;
	bool bVistoIlBlast = false;
	for (FString F : Files)
	{
		FPaths::NormalizeFilename(F);
		if (F.Contains(TEXT("/Tests/"))) { continue; }
		const FString Nome = FPaths::GetCleanFilename(F);
		bVistoIlBlast |= Nome == TEXT("RTTurnManager_Blast.cpp");
		FString Testo;
		if (!TestTrue(*FString::Printf(TEXT("il sorgente si legge: %s"), *Nome), FFileHelper::LoadFileToString(Testo, *F)))
		{
			return false;
		}
		TrovaChiamate(Testo, Nome, Chiamate, Dichiarazioni);
	}

	// ⛔ Anti-vacuita': il file del Blast e' nel perimetro, e il rilevatore vede la funzione dove e' dichiarata.
	TestTrue(TEXT("il file del Blast e' dentro il perimetro"), bVistoIlBlast);
	TestEqual(TEXT("il rilevatore vede la dichiarazione e la definizione di ApplyStructureDamage"), Dichiarazioni, 2);

	int32 NelBlast = 0;
	for (const FChiamata& C : Chiamate)
	{
		if (C.File == TEXT("RTTurnManager_Blast.cpp")
			&& C.Funzione.Equals(TEXT("ApplyEnvironmentChanges"), ESearchCase::CaseSensitive))
		{
			++NelBlast;
			continue;
		}
		AddError(FString::Printf(
			TEXT("%s:%d colpisce un muro dentro `%s`, fuori da `ApplyEnvironmentChanges` del Blast. [D-502] vuole il ")
			TEXT("verdetto del muro colpito sulla conoscenza della FASE in cui il colpo avviene, e oggi la conoscenza ")
			TEXT("di squadra si rinfresca solo in Pianificazione e nel Blast. Prima di questo produttore servono: ")
			TEXT("(1) la conoscenza di squadra rinfrescata per la sua fase; (2) il verdetto congelato con ")
			TEXT("`FreezeEdgeVerdict`; (3) il segno mostrato nel playback di quella fase (`BuildBlastSequence` prende ")
			TEXT("solo il Blast). Poi aggiorna questo cancello. Vedi #3701."),
			*C.File, C.Riga, C.Funzione.IsEmpty() ? TEXT("?") : *C.Funzione));
	}
	TestEqual(TEXT("il produttore del Blast c'e', e lo sweep lo vede"), NelBlast, 1);
	TestEqual(TEXT("nessun altro chiamante di produzione"), Chiamate.Num(), NelBlast);
	return true;
}

/**
 * `#3701` — **il controllo positivo del cancello**. Senza, lo sweep non distingue «un solo produttore» da
 * «rilevatore cieco».
 *
 * Given un testo con un produttore piantato nel Dash, il produttore del Blast, la dichiarazione, la definizione
 * e tre commenti che nominano la funzione
 * When lo si passa allo STESSO rilevatore dello sweep
 * Then trova le due chiamate, nella funzione giusta, e nient'altro
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStructureHitGuardSeesAPlantedProducerTest,
	"RefactorTactics.Structures.HitGuardSeesAPlantedProducer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStructureHitGuardSeesAPlantedProducerTest::RunTest(const FString&)
{
	using namespace RTStructureHitGuard;

	const FString Testo = FString::Join(TArray<FString>{
		TEXT("// URTHexCoverLibrary::ApplyStructureDamage(Map, Hits) in un commento di riga"),
		TEXT("\tstatic TArray<FRTCoverDamageResult> ApplyStructureDamage(URTHexMapAsset* Map,"),
		TEXT("TArray<FRTCoverDamageResult> URTHexCoverLibrary::ApplyStructureDamage(URTHexMapAsset* Map,"),
		TEXT("void ARTTurnManager::ResolveDashPhase(FRTDashContext& Ctx)"),
		TEXT("{"),
		TEXT("\t/* ApplyStructureDamage(Map, Hits) in un commento di blocco */"),
		TEXT("\t * ApplyStructureDamage(Map, Hits) nel corpo di un commento"),
		TEXT("\tURTHexCoverLibrary::ApplyStructureDamage(MutableMap, DashHits);"),
		TEXT("}"),
		TEXT("void ARTTurnManager::ApplyEnvironmentChanges(FRTBlastContext& Ctx)"),
		TEXT("{"),
		TEXT("\tfor (const FRTCoverDamageResult& Change :"),
		TEXT("\t\tURTHexCoverLibrary::ApplyStructureDamage(MutableMap, Plan.StructureHits))"),
		TEXT("\t{"),
		TEXT("\t}"),
		TEXT("}"),
	}, TEXT("\n"));

	TArray<FChiamata> Chiamate;
	int32 Dichiarazioni = 0;
	TrovaChiamate(Testo, TEXT("Piantato.cpp"), Chiamate, Dichiarazioni);

	TestEqual(TEXT("la dichiarazione e la definizione si contano a parte"), Dichiarazioni, 2);
	if (!TestEqual(TEXT("due chiamate: i commenti non contano"), Chiamate.Num(), 2)) { return false; }
	// `TestEqual` su `FString` ignora le maiuscole: il nome di una funzione si confronta esatto.
	TestTrue(*FString::Printf(TEXT("🔴 il produttore piantato e' nel Dash (letto: %s)"), *Chiamate[0].Funzione),
		Chiamate[0].Funzione.Equals(TEXT("ResolveDashPhase"), ESearchCase::CaseSensitive));
	TestEqual(TEXT("e alla sua riga"), Chiamate[0].Riga, 8);
	TestTrue(*FString::Printf(TEXT("il produttore del Blast e' in ApplyEnvironmentChanges (letto: %s)"), *Chiamate[1].Funzione),
		Chiamate[1].Funzione.Equals(TEXT("ApplyEnvironmentChanges"), ESearchCase::CaseSensitive));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
