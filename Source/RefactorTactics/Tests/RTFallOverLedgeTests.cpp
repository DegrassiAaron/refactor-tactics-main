// La CADUTA GRAVITAZIONALE: chi viene spostato a forza oltre un bordo aperto scende (#2402).
//
// Il modello sta in `docs/gameplay/spec-caduta-e-bordi.md` §3-§5. Qui si misura, e la domanda che ogni test
// deve poter far cadere e' scritta sopra di lui.
//
// 🔑 **Perche' un file proprio.** `RTHexEdgeGuardTests.cpp` misura il VOCABOLARIO del bordo (#2401) —
// `IsEdgeOpen` e `FindLandingCell`, due funzioni pure su una mappa. Questi misurano la MECCANICA: un turno
// vero, un colpo vero, e dove finisce l'unita'. Sono due soggetti, e tenerli separati e' anche cio' che
// permette a `-Filter RefactorTactics.Fall` di girare da solo.
//
// ⚠️ **Cio' che NON si misura qui, e la ragione e' misurata.** Gli effetti numerici della caduta
// (`FallEffects`, `ImpactEffects`) hanno **zero occorrenze in `Source/`**: non esiste il dato da applicare,
// e un test che li asserisse misurerebbe la propria invenzione. Sono **#2430**.

#include "Misc/AutomationTest.h"

#include "Ability/RTActionData.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTActionEvent.h"
#include "Replay/RTReplaySeekLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// ⚠️ Ogni nome qui dentro porta il prefisso `Ledge`/`Fall`, e non e' vezzo: sotto unity build le omonime
	// in anonymous namespace collidono (`#2397`). `MakeFallWorld` esiste gia' in `RTUnbalancedProneTests.cpp`
	// — per un'ALTRA caduta, quella di `D-319` — e un secondo `MakeFallWorld` qui romperebbe la build al
	// primo commit, non nel working set dove l'adaptive build tiene il file fuori dall'unity.

	UWorld* MakeLedgeWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyLedgeWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/**
	 * Una mappa fatta ESATTAMENTE delle celle indicate, e di nessun'altra.
	 *
	 * 🔑 E' il punto: un'arena piatta non ha bordi aperti al suo interno, e la caduta vive proprio
	 * nell'assenza di una cella. Qui l'assenza e' il dato, quindi la mappa si scrive per elenco.
	 */
	ARTHexMapActor* SpawnLedgeMap(UWorld* World, const TArray<FRTCellId>& Celle)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>(GetTransientPackage());
		for (const FRTCellId& C : Celle)
		{
			M->AddOrUpdateCell(FRTHexCellData(C));
		}
		M->SortCells();

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return Actor;
	}

	/** Aggiunge un parapetto passando dall'API di mappa: `Cells.Add` diretto non aggiorna la cache. */
	void AddLedgeGuard(ARTHexMapActor* MapActor, const FRTCellId& Id, ERTHexDirection Edge)
	{
		if (!MapActor || !MapActor->MapAsset) { return; }
		const FRTHexCellData* Trovata = MapActor->MapAsset->FindCell(Id);
		if (!Trovata) { return; }
		FRTHexCellData Data = *Trovata;
		Data.Guards.Add(FRTHexEdgeGuard(Edge));
		MapActor->MapAsset->AddOrUpdateCell(Data);
	}

	ARTUnit* SpawnLedgeUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // i piani li scriviamo noi
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeWraith());
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		// Senza questa riga ogni unita' senza piano pianifica un movimento verso `(0,0,0)`, che e' una cella
		// vera: traversate spurie e voci che nessun test ha chiesto.
		U->PlannedCell = Cell;
		U->PlannedAbilityIndex = INDEX_NONE;
		U->PlannedPath.Reset();
		U->Shield = 0;
		U->PushResistance = 0;
		return U;
	}

	/**
	 * Un'azione sintetica che spinge (o tira) di `Celle`, dichiarata nel test.
	 *
	 * Dichiararla qui e' piu' onesto che tenere in vita un'azione di gioco per sostenere una verifica: il
	 * catalogo dice cosa il gioco spedisce, questa dice cosa il motore deve saper fare. Stesso precedente di
	 * `MakePush2Def` in `RTControlActionTests.cpp`.
	 */
	FRTActionDef MakeLedgeShoveDef(int32 Celle, ERTActionEffect Verso = ERTActionEffect::Push)
	{
		FRTActionDef Def;
		Def.ActionId = TEXT("Test.LedgeShove");
		Def.ResolutionPhase = ERTResolutionPhase::Attack;
		Def.Priority = 55;
		Def.RangeCells = 4;
		Def.CostMP = 0;
		Def.CooldownTurns = 0;
		Def.Fallback = ERTActionFallback::AttackCell;
		Def.InterruptPolicy = ERTInterruptPolicy::InterruptBeforeEffect;
		Def.Effects.Add(FRTActionEffectSpec(Verso, Celle));
		// ⚠️ Danno ZERO, di proposito: un bersaglio morto non viene spostato, e la caduta non si misurerebbe
		// piu'. `bCountsAsAttack` resta `true` perche' senza di lui (`INT-8`) non si produce nessun colpo e
		// la spinta non arriva mai.
		Def.bCountsAsAttack = true;
		return Def;
	}

	void PlanLedgeShove(ARTUnit* Attaccante, ARTUnit* Bersaglio, int32 Celle,
		ERTActionEffect Verso = ERTActionEffect::Push)
	{
		if (!Attaccante || !Bersaglio) { return; }
		URTActionData* Ability = NewObject<URTActionData>(Attaccante);
		Ability->Def = MakeLedgeShoveDef(Celle, Verso);
		Ability->RangeCells = Ability->Def.RangeCells;
		Ability->Power = 0;
		Attaccante->Abilities.Add(Ability);
		Attaccante->PlannedAbilityIndex = Attaccante->Abilities.Num() - 1;
		Attaccante->PlannedAttackTarget = Bersaglio;
		Attaccante->PlannedCell = Attaccante->Cell;
	}

	void RunLedgeTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/** L'esito della voce `Move` di questa unita' in questo turno, o `MAX_uint8` se non ce n'e' nessuna. */
	uint8 LedgeMoveOutcome(const ARTTurnManager* TM, const ARTUnit* Unit)
	{
		if (!TM || !Unit) { return MAX_uint8; }
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Move && E.UnitId == Unit->StableUnitId)
			{
				return E.Outcome;
			}
		}
		return MAX_uint8;
	}

	/**
	 * La CAUSA della voce `DisplacementResisted` di questa unita', o `MAX_uint8` se non ce n'e' nessuna.
	 *
	 * 🔑 Il reason code viaggia in `Amount`, non in un campo proprio: lo dichiara `ERTDisplacementBlockReason`
	 * — *«i valori finiscono nel TurnLog serializzato»* — e leggerlo da li' e' cio' che fa il lettore vero.
	 */
	uint8 LedgeBlockReason(const ARTTurnManager* TM, const ARTUnit* Unit)
	{
		if (!TM || !Unit) { return MAX_uint8; }
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.UnitId == Unit->StableUnitId && E.Outcome == static_cast<uint8>(ERTMoveOutcome::DisplacementResisted))
			{
				return static_cast<uint8>(E.Amount);
			}
		}
		return MAX_uint8;
	}

	/** La voce `Move` di questa unita', o `nullptr`. */
	const FRTTurnLogEntry* LedgeMoveEntry(const ARTTurnManager* TM, const ARTUnit* Unit)
	{
		if (!TM || !Unit) { return nullptr; }
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Move && E.UnitId == Unit->StableUnitId) { return &E; }
		}
		return nullptr;
	}

	/**
	 * La geometria di riferimento: una passerella su `Layer 1` e un piano sotto.
	 *
	 *     Layer 1:  (-1,0) [attaccante]  (0,0) [bersaglio]  (1,0) [ultima stabile]   || vuoto
	 *     Layer 0:                                          (1,0) [atterraggio]  (2,0)  (3,0)
	 *
	 * Le due celle in piu' sul piano di sotto non sono decorazione: se la spinta proseguisse DOPO
	 * l'atterraggio — il difetto che `spec` §3.1 vieta — l'unita' finirebbe li', e il test se ne accorge.
	 */
	TArray<FRTCellId> PasserellaConAtterraggio()
	{
		return {
			FRTCellId(-1, 0, 1), FRTCellId(0, 0, 1), FRTCellId(1, 0, 1),
			FRTCellId(1, 0, 0), FRTCellId(2, 0, 0), FRTCellId(3, 0, 0)
		};
	}

	/**
	 * La geometria in cui **due** cadute puntano alla stessa cella.
	 *
	 *     Layer 1:  (-1,0) A1   (0,0) B1   (1,0) ultima stabile   || vuoto
	 *               (-1,1) A2   (0,1) B2   (1,1) ultima stabile   || vuoto
	 *     Layer 0:                         (1,0) primario di B1
	 *                                      (1,1) primario di B2 — OCCUPATO
	 *
	 * 🔑 **La collisione non e' un caso fortunato, e' costruita.** Due unita' non possono cadere dalla stessa
	 * cella — quindi non condividono mai il primario — ma possono incontrarsi sull'**alternativa**: il primario
	 * di `B2` e' occupato, e i vicini di `(1,1,0)` che esistono sulla mappa sono **uno solo**, `(1,0,0)`, che e'
	 * il primario di `B1`. Togliere `(2,1,0)` e `(2,0,0)` dalla mappa e' cio' che rende l'alternativa forzata
	 * invece che probabile.
	 */
	TArray<FRTCellId> DuePasserelleUnAtterraggio()
	{
		return {
			FRTCellId(-1, 0, 1), FRTCellId(0, 0, 1), FRTCellId(1, 0, 1),
			FRTCellId(-1, 1, 1), FRTCellId(0, 1, 1), FRTCellId(1, 1, 1),
			FRTCellId(1, 0, 0), FRTCellId(1, 1, 0)
		};
	}
}

// =========================================================================================================
// 1. Il caso nominale: si cade, e si atterra sul primario
// =========================================================================================================

/**
 * Chi e' spinto oltre un bordo aperto **scende**, invece di fermarsi sul ciglio.
 *
 * E' il comportamento che `#2402` apre: prima di lui `StepUntilBlocked` usciva sulla cella libera precedente
 * e li' l'unita' restava. Il test cade se il ramo non esiste — il bersaglio resterebbe su `(1,0,1)`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOpenLedgeStartsFallTest,
	"RefactorTactics.ForcedMovement.OpenLedgeStartsFall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOpenLedgeStartsFallTest::RunTest(const FString&)
{
	UWorld* World = MakeLedgeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnLedgeMap(World, PasserellaConAtterraggio());

	ARTUnit* Attaccante = SpawnLedgeUnit(World, 0, FRTCellId(-1, 0, 1));
	ARTUnit* Bersaglio = SpawnLedgeUnit(World, 1, FRTCellId(0, 0, 1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attaccante || !Bersaglio) { DestroyLedgeWorld(World); return false; }

	PlanLedgeShove(Attaccante, Bersaglio, /*Celle=*/ 2);
	RunLedgeTurn(TM);

	TestEqual(TEXT("e' sceso sul piano di sotto, non si e' fermato sul ciglio"),
		Bersaglio->Cell, FRTCellId(1, 0, 0));

	DestroyLedgeWorld(World);
	return true;
}

/**
 * L'atterraggio PRIMARIO e' la cella sottostante il punto d'uscita (`spec` §4.1), e non una qualunque.
 *
 * Il gemello del test sopra sulla stessa geometria: quello chiede *«e' sceso?»*, questo *«dove»*. Separarli
 * e' cio' che fa cadere un solo test quando la scelta dell'atterraggio cambia senza che la caduta si rompa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFallPrimaryLandingFreeTest,
	"RefactorTactics.Fall.PrimaryLandingFree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFallPrimaryLandingFreeTest::RunTest(const FString&)
{
	UWorld* World = MakeLedgeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnLedgeMap(World, PasserellaConAtterraggio());

	ARTUnit* Attaccante = SpawnLedgeUnit(World, 0, FRTCellId(-1, 0, 1));
	ARTUnit* Bersaglio = SpawnLedgeUnit(World, 1, FRTCellId(0, 0, 1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attaccante || !Bersaglio) { DestroyLedgeWorld(World); return false; }

	PlanLedgeShove(Attaccante, Bersaglio, /*Celle=*/ 2);
	RunLedgeTurn(TM);

	// La colonna e' quella dell'ultima cella stabile — `(1,0)` — non quella oltre il bordo.
	TestEqual(TEXT("stessa colonna assiale dell'ultima cella stabile"), Bersaglio->Cell.X, 1);
	TestEqual(TEXT("stessa colonna assiale dell'ultima cella stabile"), Bersaglio->Cell.Y, 0);
	TestTrue(TEXT("ed e' SCESO di layer"), Bersaglio->Cell.Layer < 1);

	DestroyLedgeWorld(World);
	return true;
}

/**
 * Il TurnLog dice `Fell`, non `Displaced`.
 *
 * ⛔ `Displaced` porta con se' *«raggiunta la destinazione della spinta»*, e la destinazione della spinta
 * non e' dove l'unita' e' finita. E' lo stesso difetto che `D-319` ha dovuto correggere per lo scivolamento:
 * un esito che dice «arrivata dove doveva» su un'unita' finita altrove manda a cercare un difetto del
 * resolver che non c'e'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFallOutcomeIsFellTest,
	"RefactorTactics.Fall.OutcomeIsFellNotDisplaced",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFallOutcomeIsFellTest::RunTest(const FString&)
{
	UWorld* World = MakeLedgeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnLedgeMap(World, PasserellaConAtterraggio());

	ARTUnit* Attaccante = SpawnLedgeUnit(World, 0, FRTCellId(-1, 0, 1));
	ARTUnit* Bersaglio = SpawnLedgeUnit(World, 1, FRTCellId(0, 0, 1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attaccante || !Bersaglio) { DestroyLedgeWorld(World); return false; }

	PlanLedgeShove(Attaccante, Bersaglio, /*Celle=*/ 2);
	RunLedgeTurn(TM);

	TestEqual(TEXT("premessa: e' caduto davvero"), Bersaglio->Cell, FRTCellId(1, 0, 0));
	TestEqual(TEXT("e la voce di movimento dice Fell"), LedgeMoveOutcome(TM, Bersaglio),
		static_cast<uint8>(ERTMoveOutcome::Fell));

	DestroyLedgeWorld(World);
	return true;
}

// =========================================================================================================
// 2. I passi residui: la caduta CONSUMA il resto dello spostamento
// =========================================================================================================

/**
 * Una spinta da 3 che cade al primo passo **non prosegue** dopo l'atterraggio (`spec` §3.1).
 *
 * 🔑 La geometria e' scelta perche' il difetto sia VISIBILE: sotto ci sono `(2,0,0)` e `(3,0,0)`, quindi
 * un'implementazione che conservasse i passi residui porterebbe l'unita' fin li'. Senza quelle due celle il
 * test sarebbe verde per costruzione — non c'e' dove proseguire — e non proverebbe niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFallConsumesRemainingDisplacementTest,
	"RefactorTactics.ForcedMovement.FallConsumesRemainingDisplacement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFallConsumesRemainingDisplacementTest::RunTest(const FString&)
{
	UWorld* World = MakeLedgeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnLedgeMap(World, PasserellaConAtterraggio());

	ARTUnit* Attaccante = SpawnLedgeUnit(World, 0, FRTCellId(-1, 0, 1));
	ARTUnit* Bersaglio = SpawnLedgeUnit(World, 1, FRTCellId(0, 0, 1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attaccante || !Bersaglio) { DestroyLedgeWorld(World); return false; }

	PlanLedgeShove(Attaccante, Bersaglio, /*Celle=*/ 3);
	RunLedgeTurn(TM);

	TestEqual(TEXT("atterra sul primario e li' si ferma"), Bersaglio->Cell, FRTCellId(1, 0, 0));
	TestNotEqual(TEXT("NON prosegue verso est sul piano di sotto"), Bersaglio->Cell, FRTCellId(2, 0, 0));
	TestNotEqual(TEXT("ne' oltre"), Bersaglio->Cell, FRTCellId(3, 0, 0));

	DestroyLedgeWorld(World);
	return true;
}

/**
 * L'ALTRA META', e senza di lei la caduta sarebbe indistinguibile da «si cade sempre vicino a un bordo».
 *
 * Una spinta ESAURITA che deposita esattamente sul ciglio **non** fa cadere: un bordo aperto *raggiunto* non
 * e' un bordo *attraversato*, e `spec` §3 dice attraversato. Stessa mappa, stesso bersaglio, stessa cella
 * finale della spinta: l'unica differenza e' la distanza.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPushExhaustedAtEdgeDoesNotFallTest,
	"RefactorTactics.ForcedMovement.ExhaustedPushAtEdgeDoesNotFall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPushExhaustedAtEdgeDoesNotFallTest::RunTest(const FString&)
{
	UWorld* World = MakeLedgeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnLedgeMap(World, PasserellaConAtterraggio());

	ARTUnit* Attaccante = SpawnLedgeUnit(World, 0, FRTCellId(-1, 0, 1));
	ARTUnit* Bersaglio = SpawnLedgeUnit(World, 1, FRTCellId(0, 0, 1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attaccante || !Bersaglio) { DestroyLedgeWorld(World); return false; }

	PlanLedgeShove(Attaccante, Bersaglio, /*Celle=*/ 1);
	RunLedgeTurn(TM);

	TestEqual(TEXT("resta sul ciglio, sul proprio layer"), Bersaglio->Cell, FRTCellId(1, 0, 1));

	DestroyLedgeWorld(World);
	return true;
}

/**
 * Chi cade **perde il piano di movimento residuo** ([D-045] `Model A`, `spec` §3.2).
 *
 * 🔑 E' il caso in cui il divieto di auto-reroute si vede a occhio nudo: la destinazione pianificata sta su
 * un altro **layer**, e un percorso ricalcolato dalla nuova origine manderebbe l'unita' a **risalire** —
 * cioe' ad annullare da sola lo spostamento che l'ha appena buttata giu'.
 *
 * ⚠️ Il piano e' una **destinazione**, non un percorso a waypoint, ed e' deliberato: la path composita
 * decadeva gia' prima di `#2501` perche' non parte piu' dalla cella attuale. Il ramo che sopravviveva — e
 * che questo test inchioda — e' proprio quello della destinazione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFallCancelsVoluntaryPathTest,
	"RefactorTactics.ForcedMovement.CancelsRemainingVoluntaryPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFallCancelsVoluntaryPathTest::RunTest(const FString&)
{
	UWorld* World = MakeLedgeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnLedgeMap(World, PasserellaConAtterraggio());

	ARTUnit* Attaccante = SpawnLedgeUnit(World, 0, FRTCellId(-1, 0, 1));
	ARTUnit* Bersaglio = SpawnLedgeUnit(World, 1, FRTCellId(0, 0, 1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attaccante || !Bersaglio) { DestroyLedgeWorld(World); return false; }

	// Il bersaglio aveva deciso di restare sulla passerella, all'estremita' est.
	const FRTCellId Pianificata(1, 0, 1);
	Bersaglio->PlannedCell = Pianificata;

	// 🔴 **E il piano va POPOLATO, non solo dichiarato.** Un `PlannedCell` da solo non esercita
	// l'azzeramento: `PlannedPath` e `PlannedWaypoints` nascono vuoti, e asserire che siano vuoti dopo il
	// turno e' verde per costruzione — con o senza il codice che li azzera.
	//
	// ⚠️ Il percorso e' ANCORATO alla cella corrente (`PlannedPath[0] == Cell`), che e' la forma che
	// `ResolveMovement` pretende per percorrerlo: un percorso non ancorato verrebbe scartato da solo, e il
	// test tornerebbe a misurare la propria fixture invece della regola.
	Bersaglio->PlannedPath = { Bersaglio->Cell, Pianificata };
	Bersaglio->PlannedWaypoints = { Pianificata };

	PlanLedgeShove(Attaccante, Bersaglio, /*Celle=*/ 2);
	RunLedgeTurn(TM);

	TestEqual(TEXT("e' caduto sul piano di sotto"), Bersaglio->Cell, FRTCellId(1, 0, 0));
	TestNotEqual(TEXT("e NON e' risalito verso la cella che aveva pianificato"),
		Bersaglio->Cell, Pianificata);

	// 🔴 **E il PIANO e' stato tolto, non solo disatteso.** Le due righe sopra guardano dove l'unita' e'
	// finita, e restano verdi anche se `PlannedPath` sopravvive: `ResolveMovement` esige che il percorso sia
	// **ancorato** — `PlannedPath[0] == Cell` — e dopo una caduta non lo e' piu', quindi ripiega su
	// `PlannedCell`, che il ramo `bPrimaDelMove` riscrive comunque. La regola ha DUE guardie e il
	// comportamento non distingue quale abbia retto.
	//
	// ⚠️ **Trovato dal gate di #2406**: la mutazione `5-percorso-volontario` — togliere i due `Reset()` da
	// `ApplyForcedDisplacement` — usciva **SOPRAVVISSUTA**, cioe' nessun test se ne accorgeva. Non era una
	// svista del test: era la difesa in profondita' che `RTUnit.h:595` gia' dichiara — *«oggi la differenza
	// non e' raggiungibile, entrambi gli scrittori di `Cell` azzerano `PlannedPath`»* — e che nessuno
	// misurava. Un codice difeso due volte e coperto zero e' esattamente cio' che una mutazione
	// sopravvissuta segnala.
	//
	// 🔑 `spec` §3.2 dice *«perde il resto del piano»*: e' uno stato, e va asserito come tale — su un piano
	// che ESISTE, altrimenti l'asserzione non ha niente da falsificare.
	TestEqual(TEXT("il percorso residuo e' stato annullato"), Bersaglio->PlannedPath.Num(), 0);
	TestEqual(TEXT("e i waypoint con lui"), Bersaglio->PlannedWaypoints.Num(), 0);

	DestroyLedgeWorld(World);
	return true;
}

// =========================================================================================================
// 3. Il bordo adiacente: il ramo che la formulazione dell'issue mancava
// =========================================================================================================

/**
 * Un bordo aperto **adiacente alla cella di partenza** fa cadere lo stesso (`#2402` D006).
 *
 * 🔑 E' il ramo che si dimenticava. Con il vuoto subito accanto `StepUntilBlocked` non avanza di un passo,
 * `Dest == T->Cell`, e il flusso finisce nel ramo `NoDestination` — non in quello dello spostamento. Una
 * caduta gestita nel solo ramo dello spostamento lascerebbe questo caso fermo, e il test lo dice.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFallAdjacentEdgeStillFallsTest,
	"RefactorTactics.Fall.AdjacentEdgeStillFalls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFallAdjacentEdgeStillFallsTest::RunTest(const FString&)
{
	UWorld* World = MakeLedgeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	// Niente `(1,0,1)`: il vuoto e' SUBITO a est del bersaglio.
	SpawnLedgeMap(World, {
		FRTCellId(-1, 0, 1), FRTCellId(0, 0, 1),
		FRTCellId(0, 0, 0)
	});

	ARTUnit* Attaccante = SpawnLedgeUnit(World, 0, FRTCellId(-1, 0, 1));
	ARTUnit* Bersaglio = SpawnLedgeUnit(World, 1, FRTCellId(0, 0, 1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attaccante || !Bersaglio) { DestroyLedgeWorld(World); return false; }

	PlanLedgeShove(Attaccante, Bersaglio, /*Celle=*/ 2);
	RunLedgeTurn(TM);

	TestEqual(TEXT("cade nella propria colonna, senza aver percorso nemmeno un passo"),
		Bersaglio->Cell, FRTCellId(0, 0, 0));

	DestroyLedgeWorld(World);
	return true;
}

// =========================================================================================================
// 4. Il parapetto: l'unico dato AUTORATO del vocabolario, e nega la caduta
// =========================================================================================================

/**
 * Un parapetto sul lato aperto ferma lo spostamento **come prima** (`spec` §2, riga *parapetto*).
 *
 * ⚠️ Il test e' sulla stessa geometria del caso nominale: l'unica differenza e' il parapetto. Uno che
 * cambiasse anche la mappa misurerebbe due cose insieme.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGuardedLedgeDoesNotFallTest,
	"RefactorTactics.ForcedMovement.GuardedLedgeDoesNotFall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGuardedLedgeDoesNotFallTest::RunTest(const FString&)
{
	UWorld* World = MakeLedgeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnLedgeMap(World, PasserellaConAtterraggio());
	AddLedgeGuard(MapActor, FRTCellId(1, 0, 1), ERTHexDirection::E);

	ARTUnit* Attaccante = SpawnLedgeUnit(World, 0, FRTCellId(-1, 0, 1));
	ARTUnit* Bersaglio = SpawnLedgeUnit(World, 1, FRTCellId(0, 0, 1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attaccante || !Bersaglio) { DestroyLedgeWorld(World); return false; }

	PlanLedgeShove(Attaccante, Bersaglio, /*Celle=*/ 2);
	RunLedgeTurn(TM);

	TestEqual(TEXT("il parapetto lo tiene sul ciglio"), Bersaglio->Cell, FRTCellId(1, 0, 1));
	TestNotEqual(TEXT("e non e' sceso"), Bersaglio->Cell, FRTCellId(1, 0, 0));

	DestroyLedgeWorld(World);
	return true;
}

// =========================================================================================================
// 5. Quando sotto non c'e' niente: si resta, non si sparisce
// =========================================================================================================

/**
 * Una colonna senza fondo lascia l'unita' su `LastStableCell` (`#2402` D006).
 *
 * ⚠️ **Caso che `spec-caduta-e-bordi.md` §4 non copre**: quel paragrafo presuppone che un atterraggio
 * primario esista. Il corpo dell'issue invece lo nomina, ed e' la garanzia che conta: nessuna unita' esce
 * dal mondo perche' la mappa non ha un piano di sotto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFallNoCellBelowTest,
	"RefactorTactics.Fall.NoCellBelowStaysOnLastStable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFallNoCellBelowTest::RunTest(const FString&)
{
	UWorld* World = MakeLedgeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	// La passerella e basta: sotto `(1,0,1)` non c'e' nulla.
	SpawnLedgeMap(World, { FRTCellId(-1, 0, 1), FRTCellId(0, 0, 1), FRTCellId(1, 0, 1) });

	ARTUnit* Attaccante = SpawnLedgeUnit(World, 0, FRTCellId(-1, 0, 1));
	ARTUnit* Bersaglio = SpawnLedgeUnit(World, 1, FRTCellId(0, 0, 1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attaccante || !Bersaglio) { DestroyLedgeWorld(World); return false; }

	PlanLedgeShove(Attaccante, Bersaglio, /*Celle=*/ 2);
	RunLedgeTurn(TM);

	TestEqual(TEXT("resta sull'ultima cella stabile"), Bersaglio->Cell, FRTCellId(1, 0, 1));
	TestTrue(TEXT("ed e' ancora vivo e sulla mappa"), Bersaglio->IsAlive());
	// #2403: la POSIZIONE non distingue questo caso dal saturo — entrambi finiscono su `LastStableCell`.
	// Solo l'esito lo fa, ed e' la ragione per cui `FellWithoutLanding` esiste ([D-352]).
	TestEqual(TEXT("e la traccia dice che non c'era atterraggio"), LedgeMoveOutcome(TM, Bersaglio),
		static_cast<uint8>(ERTMoveOutcome::FellWithoutLanding));

	DestroyLedgeWorld(World);
	return true;
}

// =========================================================================================================
// 6. L'atterraggio occupato: l'esito di mezzo, e il caso saturo
// =========================================================================================================

/**
 * Primario occupato, alternativa adiacente disponibile: chi cade **non condivide la cella** (`spec` §4.2).
 *
 * 🔑 La garanzia che il test misura non e' *quale* alternativa viene scelta — quello dipende dal Facing
 * dell'occupante, ed e' un tie-break locale — ma che chi cade finisca **da qualche altra parte**, e che
 * l'occupante **resti dov'e'**.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFallOccupiedLandingUsesAlternativeTest,
	"RefactorTactics.Fall.OccupiedLandingUsesAdjacentAlternative",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFallOccupiedLandingUsesAlternativeTest::RunTest(const FString&)
{
	UWorld* World = MakeLedgeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	// Sotto: il primario `(1,0,0)` piu' i suoi vicini, cosi' un'alternativa ESISTE.
	SpawnLedgeMap(World, {
		FRTCellId(-1, 0, 1), FRTCellId(0, 0, 1), FRTCellId(1, 0, 1),
		FRTCellId(1, 0, 0), FRTCellId(2, 0, 0), FRTCellId(0, 0, 0),
		FRTCellId(1, 1, 0), FRTCellId(1, -1, 0)
	});

	ARTUnit* Attaccante = SpawnLedgeUnit(World, 0, FRTCellId(-1, 0, 1));
	ARTUnit* Bersaglio = SpawnLedgeUnit(World, 1, FRTCellId(0, 0, 1));
	ARTUnit* Occupante = SpawnLedgeUnit(World, 1, FRTCellId(1, 0, 0)); // gia' sul primario
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attaccante || !Bersaglio || !Occupante) { DestroyLedgeWorld(World); return false; }

	PlanLedgeShove(Attaccante, Bersaglio, /*Celle=*/ 2);
	RunLedgeTurn(TM);

	TestNotEqual(TEXT("chi cade NON finisce sul primario occupato"), Bersaglio->Cell, FRTCellId(1, 0, 0));
	TestEqual(TEXT("e l'occupante resta dov'e'"), Occupante->Cell, FRTCellId(1, 0, 0));
	TestTrue(TEXT("chi cade e' comunque SCESO dalla passerella"), Bersaglio->Cell.Layer < 1);
	TestEqual(TEXT("e la traccia dice ALTERNATIVA, non primario"), LedgeMoveOutcome(TM, Bersaglio),
		static_cast<uint8>(ERTMoveOutcome::FellToAlternative));

	DestroyLedgeWorld(World);
	return true;
}

/**
 * Il caso SATURO (`spec` §4.3): primario occupato e **nessuna** alternativa. Chi cade termina su
 * `LastStableCell`, l'occupante resta, e nessuno condivide una cella.
 *
 * 🔑 E' il caso che rende leggibile l'ordine delle domande di §3: *prima dove sarebbe l'atterraggio, poi se
 * e' libero*. Al contrario, questo sarebbe un difetto invece che un esito.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFallSaturatedLandingTest,
	"RefactorTactics.Fall.SaturatedLandingStaysOnLastStable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFallSaturatedLandingTest::RunTest(const FString&)
{
	UWorld* World = MakeLedgeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	// Sotto c'e' SOLO il primario: nessuna alternativa adiacente esiste.
	SpawnLedgeMap(World, {
		FRTCellId(-1, 0, 1), FRTCellId(0, 0, 1), FRTCellId(1, 0, 1),
		FRTCellId(1, 0, 0)
	});

	ARTUnit* Attaccante = SpawnLedgeUnit(World, 0, FRTCellId(-1, 0, 1));
	ARTUnit* Bersaglio = SpawnLedgeUnit(World, 1, FRTCellId(0, 0, 1));
	ARTUnit* Occupante = SpawnLedgeUnit(World, 1, FRTCellId(1, 0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attaccante || !Bersaglio || !Occupante) { DestroyLedgeWorld(World); return false; }

	PlanLedgeShove(Attaccante, Bersaglio, /*Celle=*/ 2);
	RunLedgeTurn(TM);

	TestEqual(TEXT("chi cade termina su LastStableCell"), Bersaglio->Cell, FRTCellId(1, 0, 1));
	TestEqual(TEXT("l'occupante resta dov'e'"), Occupante->Cell, FRTCellId(1, 0, 0));
	// #2403: stessa CELLA di `FellWithoutLanding`, esito diverso — qui sotto un primario c'era, ed era
	// occupato. La posizione non li distingue: solo l'esito.
	TestEqual(TEXT("e la traccia dice SATURO, non «senza atterraggio»"), LedgeMoveOutcome(TM, Bersaglio),
		static_cast<uint8>(ERTMoveOutcome::FellToLastStable));

	DestroyLedgeWorld(World);
	return true;
}

/**
 * L'alternativa si sceglie **dal Facing dell'occupante**, e questo test lo inchioda a una cella precisa.
 *
 * 🔑 Gli altri test dell'esito §4.2 asseriscono garanzie ORDINE-INDIPENDENTI — «non sul primario»,
 * «l'occupante resta» — e restano verdi comunque si scandiscano i vicini. Quello e' un pregio li' e un buco
 * qui: senza questo test, invertire l'anello canonico non farebbe cadere niente, e la regola piu' densa
 * della spec sarebbe la meno protetta.
 *
 * L'occupante guarda a **NE**, quindi la prima candidata e' `Neighbor(primario, NE)` = `(2,-1,0)`. La cella
 * `E` — `(2,0,0)`, che l'anello canonico offrirebbe per prima se il Facing NON contasse — esiste ed e'
 * libera: e' li' apposta, ed e' la cella su cui il test cade se il tie-break viene ignorato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFallAlternativeFromFacingTest,
	"RefactorTactics.Fall.AlternativeFollowsCanonicalRingFromFacing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFallAlternativeFromFacingTest::RunTest(const FString&)
{
	UWorld* World = MakeLedgeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnLedgeMap(World, {
		FRTCellId(-1, 0, 1), FRTCellId(0, 0, 1), FRTCellId(1, 0, 1),
		FRTCellId(1, 0, 0),                      // primario, occupato
		FRTCellId(2, 0, 0),                      // E: la prima dell'anello se il Facing non contasse
		FRTCellId(2, -1, 0)                      // NE: dove l'occupante guarda
	});

	ARTUnit* Attaccante = SpawnLedgeUnit(World, 0, FRTCellId(-1, 0, 1));
	ARTUnit* Bersaglio = SpawnLedgeUnit(World, 1, FRTCellId(0, 0, 1));
	ARTUnit* Occupante = SpawnLedgeUnit(World, 1, FRTCellId(1, 0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attaccante || !Bersaglio || !Occupante) { DestroyLedgeWorld(World); return false; }

	Occupante->Facing = ERTHexDirection::NE;

	PlanLedgeShove(Attaccante, Bersaglio, /*Celle=*/ 2);
	RunLedgeTurn(TM);

	TestEqual(TEXT("atterra dove l'occupante guarda, non sulla prima dell'anello"),
		Bersaglio->Cell, FRTCellId(2, -1, 0));
	TestEqual(TEXT("l'occupante non si e' mosso"), Occupante->Cell, FRTCellId(1, 0, 0));

	DestroyLedgeWorld(World);
	return true;
}

// =========================================================================================================
// 7. L'invariante che non deve mai rompersi: una unita' per cella
// =========================================================================================================

/**
 * **Nessuna sovrapposizione, in nessuno dei tre esiti** (`spec` §4.2.3 e §4.3.7).
 *
 * 🔑 Il test non guarda una cella scelta: guarda TUTTE le unita' e cerca due che condividano la stessa.
 * E' la forma che regge anche quando la scelta dell'atterraggio cambia — un test che asserisse una cella
 * precisa diventerebbe rosso a ogni ritocco del tie-break, e verde su una sovrapposizione altrove.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFallNeverOverlapsTest,
	"RefactorTactics.Fall.NeverOverlaps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFallNeverOverlapsTest::RunTest(const FString&)
{
	UWorld* World = MakeLedgeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnLedgeMap(World, {
		FRTCellId(-1, 0, 1), FRTCellId(0, 0, 1), FRTCellId(1, 0, 1),
		FRTCellId(1, 0, 0)
	});

	ARTUnit* Attaccante = SpawnLedgeUnit(World, 0, FRTCellId(-1, 0, 1));
	ARTUnit* Bersaglio = SpawnLedgeUnit(World, 1, FRTCellId(0, 0, 1));
	ARTUnit* Occupante = SpawnLedgeUnit(World, 1, FRTCellId(1, 0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attaccante || !Bersaglio || !Occupante) { DestroyLedgeWorld(World); return false; }

	PlanLedgeShove(Attaccante, Bersaglio, /*Celle=*/ 2);
	RunLedgeTurn(TM);

	TArray<ARTUnit*> Vive = { Attaccante, Bersaglio, Occupante };
	for (int32 a = 0; a < Vive.Num(); ++a)
	{
		for (int32 b = a + 1; b < Vive.Num(); ++b)
		{
			TestNotEqual(FString::Printf(TEXT("nessuna sovrapposizione fra unita' %d e %d"), a, b),
				Vive[a]->Cell, Vive[b]->Cell);
		}
	}

	DestroyLedgeWorld(World);
	return true;
}

// =========================================================================================================
// 8. La trazione: `spec` §3 dice SPOSTAMENTO FORZATO, non «spinta»
// =========================================================================================================

/**
 * Chi e' **tirato** oltre un bordo aperto cade come chi e' spinto.
 *
 * 🔑 Senza questo test l'asimmetria *«spinto oltre un bordo cade, tirato oltre un bordo no»* passerebbe
 * inosservata, e nessuna regola la dichiara. Spinta e trazione condividono `StepUntilBlocked`, quindi
 * condividono anche il bordo su cui si fermano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPullOverOpenLedgeStartsFallTest,
	"RefactorTactics.ForcedMovement.PullOverOpenLedgeStartsFall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPullOverOpenLedgeStartsFallTest::RunTest(const FString&)
{
	UWorld* World = MakeLedgeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	// Chi tira sta a OVEST, il bersaglio a EST, e in mezzo manca `(1,0,1)`: la trazione attraversa il vuoto.
	SpawnLedgeMap(World, {
		FRTCellId(0, 0, 1), FRTCellId(2, 0, 1),
		FRTCellId(2, 0, 0)
	});

	ARTUnit* Attaccante = SpawnLedgeUnit(World, 0, FRTCellId(0, 0, 1));
	ARTUnit* Bersaglio = SpawnLedgeUnit(World, 1, FRTCellId(2, 0, 1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attaccante || !Bersaglio) { DestroyLedgeWorld(World); return false; }

	PlanLedgeShove(Attaccante, Bersaglio, /*Celle=*/ 2, ERTActionEffect::Pull);
	RunLedgeTurn(TM);

	TestEqual(TEXT("chi e' tirato oltre il bordo scende"), Bersaglio->Cell, FRTCellId(2, 0, 0));

	DestroyLedgeWorld(World);
	return true;
}

// =========================================================================================================
// 9. Il parapetto ha un esito proprio (#2403, [D-354])
// =========================================================================================================

/**
 * Un parapetto che ferma una spinta si legge nella traccia, e non somiglia a un muro.
 *
 * 🔴 **Prima di [D-354] questo caso scriveva `Displaced`**, identico a una spinta fermata da una parete: un
 * replay non poteva dire se il bordo era protetto o se non c'era. Il parapetto e' l'unica delle quattro
 * qualifiche del `spec` §2 a essere **autorata** invece che derivata, e renderla illeggibile toglieva senso
 * all'averla autorata.
 *
 * ⚠️ Stessa geometria di `GuardedLedgeDoesNotFall`, che misura la POSIZIONE. Questo misura la TRACCIA: due
 * domande diverse sullo stesso fatto, e la prima resterebbe verde anche con l'esito sbagliato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGuardedLedgeIsNamedInTheLogTest,
	"RefactorTactics.ForcedMovement.GuardedLedgeIsNamedInTheLog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGuardedLedgeIsNamedInTheLogTest::RunTest(const FString&)
{
	UWorld* World = MakeLedgeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	ARTHexMapActor* MapActor = SpawnLedgeMap(World, PasserellaConAtterraggio());
	AddLedgeGuard(MapActor, FRTCellId(1, 0, 1), ERTHexDirection::E);

	ARTUnit* Attaccante = SpawnLedgeUnit(World, 0, FRTCellId(-1, 0, 1));
	ARTUnit* Bersaglio = SpawnLedgeUnit(World, 1, FRTCellId(0, 0, 1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attaccante || !Bersaglio) { DestroyLedgeWorld(World); return false; }

	PlanLedgeShove(Attaccante, Bersaglio, /*Celle=*/ 2);
	RunLedgeTurn(TM);

	TestEqual(TEXT("premessa: si e' mosso e il parapetto l'ha tenuto"), Bersaglio->Cell, FRTCellId(1, 0, 1));
	TestEqual(TEXT("e la traccia nomina il parapetto"), LedgeMoveOutcome(TM, Bersaglio),
		static_cast<uint8>(ERTMoveOutcome::StoppedByEdgeGuard));

	DestroyLedgeWorld(World);
	return true;
}

/**
 * Un parapetto **adiacente** non lascia percorrere nessuna cella: senza spostamento non c'e' una voce di
 * movimento, e la causa viaggia in `ERTDisplacementBlockReason::EdgeGuard`.
 *
 * 🔑 **E' il secondo ramo**, lo stesso che `#2402` D006 ha dovuto gestire per la caduta: con il bordo
 * adiacente `StepUntilBlocked` non avanza, `Dest == cella di partenza`, e il flusso finisce dove finiscono
 * le spinte senza destinazione. Un solo valore ne coprirebbe uno e lascerebbe l'altro indistinguibile.
 *
 * ⛔ **`NoDestination` non basta**: dice *«non c'e' dove andare»*, che e' vero anche per un muro. Qui il
 * vuoto c'era, ed era protetto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAdjacentGuardedLedgeSaysEdgeGuardTest,
	"RefactorTactics.ForcedMovement.AdjacentGuardedLedgeSaysEdgeGuard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAdjacentGuardedLedgeSaysEdgeGuardTest::RunTest(const FString&)
{
	UWorld* World = MakeLedgeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	// Due sole celle: il bersaglio e' GIA' sul ciglio, e il suo lato E da' sul vuoto ed e' protetto.
	ARTHexMapActor* MapActor = SpawnLedgeMap(World, { FRTCellId(-1, 0, 1), FRTCellId(0, 0, 1) });
	AddLedgeGuard(MapActor, FRTCellId(0, 0, 1), ERTHexDirection::E);

	ARTUnit* Attaccante = SpawnLedgeUnit(World, 0, FRTCellId(-1, 0, 1));
	ARTUnit* Bersaglio = SpawnLedgeUnit(World, 1, FRTCellId(0, 0, 1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attaccante || !Bersaglio) { DestroyLedgeWorld(World); return false; }

	PlanLedgeShove(Attaccante, Bersaglio, /*Celle=*/ 2);
	RunLedgeTurn(TM);

	TestEqual(TEXT("premessa: non si e' mosso"), Bersaglio->Cell, FRTCellId(0, 0, 1));
	TestEqual(TEXT("e la causa e' il parapetto, non la mancanza di destinazione"),
		LedgeBlockReason(TM, Bersaglio), static_cast<uint8>(ERTDisplacementBlockReason::EdgeGuard));

	DestroyLedgeWorld(World);
	return true;
}

// =========================================================================================================
// 10. La catena causale: che cosa la traccia deve poter dire da sola (#2403)
// =========================================================================================================

/**
 * Dalla sola voce si ricostruisce la catena: **da dove**, **dove**, **con quale esito**.
 *
 * 🔑 **Il test guarda i tre campi INSIEME**, che e' il punto: `SrcCell` da sola dice dov'era, `TgtCell` da
 * sola dice dov'e' finita, e nessuna delle due dice che di mezzo c'e' stata una caduta. E' l'esito a
 * chiudere la catena, ed e' la ragione per cui `spec` §9 chiede tutti e tre.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFallTracePreservesCauseChainTest,
	"RefactorTactics.Fall.TracePreservesCauseChain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFallTracePreservesCauseChainTest::RunTest(const FString&)
{
	UWorld* World = MakeLedgeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnLedgeMap(World, PasserellaConAtterraggio());

	ARTUnit* Attaccante = SpawnLedgeUnit(World, 0, FRTCellId(-1, 0, 1));
	ARTUnit* Bersaglio = SpawnLedgeUnit(World, 1, FRTCellId(0, 0, 1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attaccante || !Bersaglio) { DestroyLedgeWorld(World); return false; }

	PlanLedgeShove(Attaccante, Bersaglio, /*Celle=*/ 2);
	RunLedgeTurn(TM);

	const FRTTurnLogEntry* Voce = LedgeMoveEntry(TM, Bersaglio);
	if (!TestNotNull(TEXT("la caduta ha lasciato una voce di movimento"), Voce))
	{
		DestroyLedgeWorld(World);
		return false;
	}
	TestEqual(TEXT("da dove: la cella prima del bordo"), Voce->SrcCell, FRTCellId(0, 0, 1));
	TestEqual(TEXT("dove: l'atterraggio primario"), Voce->TgtCell, FRTCellId(1, 0, 0));
	TestEqual(TEXT("con quale esito: una caduta, non uno spostamento"), Voce->Outcome,
		static_cast<uint8>(ERTMoveOutcome::Fell));
	TestEqual(TEXT("nella fase in cui gli spostamenti si applicano"), Voce->Phase, ERTMatchPhase::Blast);

	DestroyLedgeWorld(World);
	return true;
}

/**
 * L'esito e' **nel dato**: una traccia serializzata e riletta **senza la mappa** dice ancora quale caduta e'
 * avvenuta.
 *
 * 🔑 **E' la formulazione falsificabile del criterio «riproduce senza ricalcolare»**. Senza mappa nessuno
 * puo' rieseguire `FindLandingCell`: se l'esito sopravvive al round-trip, e' perche' e' stato scritto, non
 * ricalcolato. La formulazione originale non diceva che cosa si rompe, e sarebbe stata verde per
 * costruzione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFallReplayMatchesResolvedOutcomeTest,
	"RefactorTactics.Fall.ReplayMatchesResolvedOutcome",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFallReplayMatchesResolvedOutcomeTest::RunTest(const FString&)
{
	UWorld* World = MakeLedgeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnLedgeMap(World, PasserellaConAtterraggio());

	ARTUnit* Attaccante = SpawnLedgeUnit(World, 0, FRTCellId(-1, 0, 1));
	ARTUnit* Bersaglio = SpawnLedgeUnit(World, 1, FRTCellId(0, 0, 1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attaccante || !Bersaglio) { DestroyLedgeWorld(World); return false; }

	PlanLedgeShove(Attaccante, Bersaglio, /*Celle=*/ 2);
	RunLedgeTurn(TM);

	const int32 IdBersaglio = Bersaglio->StableUnitId;
	const uint8 EsitoRisolto = LedgeMoveOutcome(TM, Bersaglio);

	const TArray<uint8> Bytes = URTTurnLogLibrary::SerializeTurnLog(TM->GetTurnLog(), ERTLogTopology::Hex);
	TArray<FRTTurnLogEntry> Riletta;
	if (!TestTrue(TEXT("la traccia si rilegge"), URTTurnLogLibrary::DeserializeTurnLog(Bytes, Riletta)))
	{
		DestroyLedgeWorld(World);
		return false;
	}

	// Da qui in poi la mappa non serve piu', ed e' il punto del test: nessuno puo' ricalcolare.
	uint8 EsitoRiletto = MAX_uint8;
	FRTCellId Atterraggio;
	for (const FRTTurnLogEntry& E : Riletta)
	{
		if (E.Category == ERTLogCategory::Move && E.UnitId == IdBersaglio)
		{
			EsitoRiletto = E.Outcome;
			Atterraggio = E.TgtCell;
			break;
		}
	}
	TestEqual(TEXT("l'esito sopravvive al round-trip"), EsitoRiletto, EsitoRisolto);
	TestEqual(TEXT("ed e' quello di una caduta sul primario"), EsitoRiletto,
		static_cast<uint8>(ERTMoveOutcome::Fell));
	TestEqual(TEXT("con l'atterraggio che porta con se'"), Atterraggio, FRTCellId(1, 0, 0));

	DestroyLedgeWorld(World);
	return true;
}

/**
 * Il cursore che trova una caduta e' quello alla **fase**, non quello al **boundary**.
 *
 * 🔴 **Il criterio originale di `#2403` chiedeva il boundary, e non era soddisfacibile.** Le voci della
 * caduta nascono in `ApplyDisplacements`, nella coda della fase di attacco, **fuori** dal ciclo dei
 * micro-step: `CurrentMicroStepIndex` vale `INDEX_NONE`, e `SeekToBoundary` rifiuta i valori negativi
 * *«senza scandire la traccia»* — perche' quelle voci condividono un valore, non una barriera che le abbia
 * decise insieme.
 *
 * 🔑 **La seconda meta' del test e' anti-vacuita'**: senza di essa questo resterebbe verde anche se qualcuno
 * "aggiustasse" il problema popolando `MicroStepIndex` nelle voci di Blast — che e' esattamente l'errore che
 * `SeekToBoundary` esiste per rifiutare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFallSeekToPhaseFindsTheEntryTest,
	"RefactorTactics.Fall.SeekToPhaseFindsTheFallEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFallSeekToPhaseFindsTheEntryTest::RunTest(const FString&)
{
	UWorld* World = MakeLedgeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnLedgeMap(World, PasserellaConAtterraggio());

	ARTUnit* Attaccante = SpawnLedgeUnit(World, 0, FRTCellId(-1, 0, 1));
	ARTUnit* Bersaglio = SpawnLedgeUnit(World, 1, FRTCellId(0, 0, 1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attaccante || !Bersaglio) { DestroyLedgeWorld(World); return false; }

	PlanLedgeShove(Attaccante, Bersaglio, /*Celle=*/ 2);
	RunLedgeTurn(TM);

	const TArray<FRTTurnLogEntry>& Traccia = TM->GetTurnLog();
	int32 Indice = INDEX_NONE;
	TestEqual(TEXT("il seek alla FASE trova le voci del Blast"),
		URTReplaySeekLibrary::SeekToPhase(Traccia, ERTMatchPhase::Blast, Indice),
		ERTReplaySeekResult::Found);
	TestTrue(TEXT("e l'indice e' dentro la traccia"), Traccia.IsValidIndex(Indice));

	// E il boundary NON la trova, ed e' corretto: la caduta non appartiene a nessun ciclo di micro-step.
	int32 IndiceBoundary = INDEX_NONE;
	TestNotEqual(TEXT("il seek al BOUNDARY non la trova, e non deve"),
		URTReplaySeekLibrary::SeekToBoundary(Traccia, ERTMatchPhase::Blast, /*MicroStepIndex=*/ 0, IndiceBoundary),
		ERTReplaySeekResult::Found);

	DestroyLedgeWorld(World);
	return true;
}

// =========================================================================================================
// 11. La trazione smette di essere muta dove la spinta parla (#2403)
// =========================================================================================================

/**
 * Una trazione verso un bordo **protetto** non sposta nessuno, e la traccia dice **perche'**.
 *
 * 🔴 **Il ramo della trazione era muto, e non per una scelta.** `#420` conta i modi in cui uno «spostamento
 * forzato» non sposta nessuno, e `spec` §3 dice che la trazione lo e' — tanto che `Anchored` una voce la
 * scriveva gia'. Mancava il solo caso geometrico, e il buco si vedeva solo dal lato che non lo dichiarava:
 * chi leggeva il ramo della spinta trovava sei cause, chi leggeva quello della trazione ne trovava una.
 *
 * 🔑 **La geometria e' quella di `PullOverOpenLedgeStartsFall`**, con in piu' il parapetto: la stessa
 * trazione che li' fa cadere, qui non sposta nulla. Cambiare anche la mappa misurerebbe due cose insieme.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPullTowardGuardedLedgeSaysEdgeGuardTest,
	"RefactorTactics.ForcedMovement.PullTowardGuardedLedgeSaysEdgeGuard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPullTowardGuardedLedgeSaysEdgeGuardTest::RunTest(const FString&)
{
	UWorld* World = MakeLedgeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	// Chi tira sta a OVEST, il bersaglio a EST, e in mezzo manca `(1,0,1)`: la trazione attraverserebbe il
	// vuoto — ma il lato W del bersaglio e' protetto, quindi non lo attraversa.
	ARTHexMapActor* MapActor = SpawnLedgeMap(World, {
		FRTCellId(0, 0, 1), FRTCellId(2, 0, 1), FRTCellId(2, 0, 0)
	});
	AddLedgeGuard(MapActor, FRTCellId(2, 0, 1), ERTHexDirection::W);

	ARTUnit* Attaccante = SpawnLedgeUnit(World, 0, FRTCellId(0, 0, 1));
	ARTUnit* Bersaglio = SpawnLedgeUnit(World, 1, FRTCellId(2, 0, 1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attaccante || !Bersaglio) { DestroyLedgeWorld(World); return false; }

	PlanLedgeShove(Attaccante, Bersaglio, /*Celle=*/ 2, ERTActionEffect::Pull);
	RunLedgeTurn(TM);

	TestEqual(TEXT("premessa: il parapetto lo tiene, e non scende"), Bersaglio->Cell, FRTCellId(2, 0, 1));
	TestEqual(TEXT("e la trazione non e' piu' muta: dice il parapetto"), LedgeBlockReason(TM, Bersaglio),
		static_cast<uint8>(ERTDisplacementBlockReason::EdgeGuard));

	DestroyLedgeWorld(World);
	return true;
}

// =========================================================================================================
// 12. Due cadute nello stesso Blast: l'invariante di occupazione ([D-353])
// =========================================================================================================

/**
 * Due unita' che cadono nello stesso Blast **non si sovrappongono**, e l'esito non dipende dall'ordine in
 * cui il resolver le risolve (`spec` §4.3.1, [D-353]).
 *
 * 🔴 **L'invariante era promesso e verificato su N = 1.** `#2402` D002 dichiara *«esito invariante per
 * permutazione dell'ordine di risoluzione»*, ma `Fall.NeverOverlaps` mette in scena **una** caduta e un
 * occupante fermo: dove la permutazione non esiste, l'invarianza e' vera per costruzione. Questo test e' il
 * primo in cui due cadute competono davvero.
 *
 * 🔑 **La seconda meta' e' ANTI-VACUITA'.** Senza di essa il test resterebbe verde anche se le due cadute
 * non si contendessero niente — e allora non misurerebbe l'invariante, ma la fortuna della geometria.
 * Chiedere che la contesa sia **registrata** e' cio' che prova che il caso e' stato esercitato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFallTwoFallersSameLandingTest,
	"RefactorTactics.Fall.TwoFallersSameLandingIsDeterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFallTwoFallersSameLandingTest::RunTest(const FString&)
{
	UWorld* World = MakeLedgeWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnLedgeMap(World, DuePasserelleUnAtterraggio());

	ARTUnit* A1 = SpawnLedgeUnit(World, 0, FRTCellId(-1, 0, 1));
	ARTUnit* B1 = SpawnLedgeUnit(World, 1, FRTCellId(0, 0, 1));
	ARTUnit* A2 = SpawnLedgeUnit(World, 0, FRTCellId(-1, 1, 1));
	ARTUnit* B2 = SpawnLedgeUnit(World, 1, FRTCellId(0, 1, 1));
	ARTUnit* Occupante = SpawnLedgeUnit(World, 1, FRTCellId(1, 1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !A1 || !B1 || !A2 || !B2 || !Occupante) { DestroyLedgeWorld(World); return false; }

	PlanLedgeShove(A1, B1, /*Celle=*/ 2);
	PlanLedgeShove(A2, B2, /*Celle=*/ 2);
	RunLedgeTurn(TM);

	// 1. L'INVARIANTE. Non «B1 e' finito li'» — la scelta dell'atterraggio puo' cambiare — ma che nessuna
	//    cella porti due occupanti, che e' cio' che `spec` §4.3.1 dichiara osservabile.
	TArray<ARTUnit*> Vive = { A1, B1, A2, B2, Occupante };
	for (int32 a = 0; a < Vive.Num(); ++a)
	{
		for (int32 b = a + 1; b < Vive.Num(); ++b)
		{
			TestNotEqual(FString::Printf(TEXT("nessuna sovrapposizione fra %d e %d"), a, b),
				Vive[a]->Cell, Vive[b]->Cell);
		}
	}

	// 2. ANTI-VACUITA': le due cadute si sono DAVVERO contese la stessa cella.
	//
	// 🔑 **`&&`, non `||`.** Con `||` basterebbe UNA contesa qualunque — anche fra due unita' che non
	// c'entrano — e il test tornerebbe a misurare la fortuna della geometria invece dell'invariante. La
	// contesa che questo scenario costruisce ne coinvolge DUE: `B1` sul proprio primario, `B2` sull'unica
	// alternativa che la mappa gli lascia.
	TestEqual(TEXT("B1 ha contesa la cella"), LedgeBlockReason(TM, B1),
		static_cast<uint8>(ERTDisplacementBlockReason::ContestedDestination));
	TestEqual(TEXT("e B2 anche"), LedgeBlockReason(TM, B2),
		static_cast<uint8>(ERTDisplacementBlockReason::ContestedDestination));

	// 3. DOVE FINISCONO, che e' il comportamento vigente e non una regola decisa.
	//
	// 🔴 **La spec non copre due cadute che si contendono lo stesso atterraggio.** §4.3 governa il primario
	// occupato da un'unita' FERMA, non due che scendono insieme; e cio' che accade qui viene da `#420` —
	// «due bersagli spinti verso la stessa cella restano entrambi fermi» — scritto per le spinte, non per la
	// gravita'. La differenza e' visibile: §4.3 dice che nel caso saturo la caduta E' AVVENUTA e chi cade
	// termina su `LastStableCell`, cioe' sul **ciglio**; qui le due unita' non lasciano nemmeno la cella di
	// partenza.
	//
	// ⚠️ **Il test pinna il comportamento vigente, non lo approva**: registrato come `VERT-2` in
	// `OPEN_DECISIONS.md`. Se la decisione va nell'altro verso, questa asserzione cambia **con la regola** —
	// ed e' il punto di scriverla adesso: senza, il cambio passerebbe inosservato.
	TestEqual(TEXT("B1 non ha lasciato la cella di partenza"), B1->Cell, FRTCellId(0, 0, 1));
	TestEqual(TEXT("ne' B2"), B2->Cell, FRTCellId(0, 1, 1));
	TestEqual(TEXT("e l'occupante di sotto non e' stato toccato"), Occupante->Cell, FRTCellId(1, 1, 0));

	DestroyLedgeWorld(World);
	return true;
}

/**
 * Lo stesso scenario con le unita' **spawnate nell'ordine inverso** produce le stesse posizioni finali.
 *
 * 🔑 **E' la permutazione vera.** L'ordine di spawn e' l'ordine dell'array `Units`, cioe' l'ordine in cui il
 * resolver incontra i bersagli: se l'esito ne dipendesse, due partite identiche divergerebbero per un
 * dettaglio che nessuna regola dichiara. E' lo stesso difetto che `HexSim` misura per le mobilita' contese,
 * *«chi vince deve dipendere dalla priorita', non dalla posizione nell'array»*.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFallTwoFallersOrderInvariantTest,
	"RefactorTactics.Fall.TwoFallersOutcomeIsOrderInvariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFallTwoFallersOrderInvariantTest::RunTest(const FString&)
{
	auto Esegui = [this](bool bInverso, TArray<FRTCellId>& OutFinali) -> bool
	{
		UWorld* World = MakeLedgeWorld();
		if (!World) { return false; }
		SpawnLedgeMap(World, DuePasserelleUnAtterraggio());

		ARTUnit *A1 = nullptr, *B1 = nullptr, *A2 = nullptr, *B2 = nullptr, *Occupante = nullptr;
		if (!bInverso)
		{
			A1 = SpawnLedgeUnit(World, 0, FRTCellId(-1, 0, 1));
			B1 = SpawnLedgeUnit(World, 1, FRTCellId(0, 0, 1));
			A2 = SpawnLedgeUnit(World, 0, FRTCellId(-1, 1, 1));
			B2 = SpawnLedgeUnit(World, 1, FRTCellId(0, 1, 1));
		}
		else
		{
			A2 = SpawnLedgeUnit(World, 0, FRTCellId(-1, 1, 1));
			B2 = SpawnLedgeUnit(World, 1, FRTCellId(0, 1, 1));
			A1 = SpawnLedgeUnit(World, 0, FRTCellId(-1, 0, 1));
			B1 = SpawnLedgeUnit(World, 1, FRTCellId(0, 0, 1));
		}
		Occupante = SpawnLedgeUnit(World, 1, FRTCellId(1, 1, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !A1 || !B1 || !A2 || !B2 || !Occupante) { DestroyLedgeWorld(World); return false; }

		PlanLedgeShove(A1, B1, /*Celle=*/ 2);
		PlanLedgeShove(A2, B2, /*Celle=*/ 2);
		RunLedgeTurn(TM);

		// Sempre nello stesso ordine LOGICO — B1 poi B2 — non nell'ordine di spawn: e' il confronto che
		// deve reggere, non l'indice.
		OutFinali = { B1->Cell, B2->Cell, Occupante->Cell };
		DestroyLedgeWorld(World);
		return true;
	};

	TArray<FRTCellId> Diretto, Inverso;
	if (!TestTrue(TEXT("la prima esecuzione riesce"), Esegui(false, Diretto))) { return false; }
	if (!TestTrue(TEXT("la seconda esecuzione riesce"), Esegui(true, Inverso))) { return false; }

	if (!TestEqual(TEXT("stesso numero di posizioni"), Inverso.Num(), Diretto.Num())) { return false; }
	for (int32 i = 0; i < Diretto.Num(); ++i)
	{
		TestEqual(FString::Printf(TEXT("l'unita' %d finisce dove finiva"), i), Inverso[i], Diretto[i]);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
