#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
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
#include "Turn/RTPlaybackLibrary.h" // #3281: il confine d'atto, misurato sull'aggregato
#include "Turn/RTTurnLogLibrary.h" // #1150: i predicati che dichiarano chi ha inflitto e chi ha subito
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Tests/RTAbilityFixtures.h"

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
// `ResolvePrep` e a meta' di `CollectAttackIntents`, e i siti che consumano un'azione principale sono
// **tre**: il ramo `ModifyArc` la PRECEDE dentro lo stesso ciclo, e le ambientali risolvono altrove.
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
 * Uno stordito non accende: le azioni **ambientali** sono principali, e risolvono nel terzo sito.
 *
 * `Action.Ignite` esce da `CollectAttackIntents` col piano INTATTO — quel ciclo lascia passare la fase
 * `Cleanup` apposta — e lo consuma `ResolveEnvironment`. Senza una guardia li', il rifiuto degli altri due
 * siti non la tocca: e' esattamente il buco che questo test pinna.
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

		// ⛔ `NAME_None` e' il valore dichiarato, non una dimenticanza: su un bordo colpito da piu' intenti
		// non esiste UNA azione da nominare, e `Next Action` non deve fermarsi su un muro.
		TestTrue(TEXT("⛔ nessuna identita' d'azione: e' un limite dichiarato"), Ev.ActionId.IsNone());
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
	 */
	FRTEnvBreachScenario EnvMakeRedundantFaceScenario(int32 InStructurePower, int32 InHeightDelta)
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

		// La faccia W di (1,0), su una cella rialzata di `InHeightDelta`.
		FRTHexCellData ConFaccia = *A;
		ConFaccia.Height = InHeightDelta;
		ConFaccia.Covers.Add(FRTHexCover(ERTHexDirection::W, ERTHexCoverType::Low,
			FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low)));
		Asset->AddOrUpdateCell(ConFaccia);

		// ⛔ E la faccia E di (0,0): la SECONDA dichiarazione della stessa barriera, a quota zero.
		FRTHexCellData ConSpecchio = *B;
		ConSpecchio.Height = 0;
		ConSpecchio.Covers.Add(FRTHexCover(ERTHexDirection::E, ERTHexCoverType::Low,
			FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low)));
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
	// 🔴 **`Next Action` non si ferma sul muro che cade**, ed e' il fatto da cui la issue parte. Qui e'
	// misurato e non dedotto: l'evento aggregato non e' un confine d'atto.
	//
	// ⛔ **Il gate non dice che sia giusto ne' che sia sbagliato.** `RTPlaybackLibrary.h` dichiara oggi che
	// «gli eventi senza azione non sono confini», nominando fra essi il danno ambientale: la conseguenza e'
	// coerente con il contratto scritto, ed e' la decisione della issue a stabilire se quel contratto debba
	// cambiare.
	const TArray<FRTResolvedEvent>& Timeline = S.TM->ResolvedTimelineForTest();
	int32 IndiceColpo = INDEX_NONE;
	for (int32 i = 0; i < Timeline.Num(); ++i)
	{
		if (Timeline[i].Type == ERTResolvedEventType::StructureHit) { IndiceColpo = i; break; }
	}
	if (TestTrue(TEXT("il colpo sta nella timeline"), IndiceColpo != INDEX_NONE))
	{
		TestNotEqual(TEXT("∴ e non e' un confine d'atto: `Next Action` non ci si ferma"),
			URTPlaybackLibrary::NextActionBoundary(Timeline, IndiceColpo - 1), IndiceColpo);
	}

	DestroyEnvWorld(S.World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
