// Blind Fire — il requisito di linea di tiro e' un DATO dell'azione (`#2870`, [D-378]).
//
// Le sette prove che la issue elenca, piu' il canary strutturale sull'anteprima. La divisione fra
// funzioni pure e mondo di prova non e' organizzativa: cio' che deve valere ANCHE per il giocatore si
// misura attraverso `ARTPlayerController`, perche' «l'harness ci arriva» non e' una prova che ci arrivi
// l'input. Cio' che invece e' geometria si misura sulla libreria, dove un fallimento nomina la regola
// invece di nominare il mondo.

#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Perception/RTTeamKnowledge.h" // la premessa di scenario: l'ostacolo e' noto o no?
#include "Player/RTPlayerController.h"
#include "Player/RTPlayerState.h"       // la squadra da cui l'HUD deriva il filtro, letta dalla stessa porta
#include "Tests/RTAbilityFixtures.h"
#include "Player/RTPointerInteraction.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "UI/RTHUD.h"                 // il canale a schermo del rifiuto: e' cio' che `#3064` misura
#include "Terrain/RTTerrainLibrary.h" // la portata APPLICATA, per non scrivere un numero a mano
#include "Unit/RTUnit.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Arena piatta con un muro alla vista in mezzo: nomi distinti per file, il progetto usa unity build. */
	URTHexMapAsset* MakeBlindFireMap(int32 Radius, const FRTCellId& Blocker)
	{
		URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		FRTHexCellData Wall = Map->FindCell(Blocker) ? *Map->FindCell(Blocker) : FRTHexCellData(Blocker);
		Wall.Id = Blocker;
		Wall.bBlocksLineOfSight = true;
		Map->AddOrUpdateCell(Wall);
		Map->SortCells();
		return Map;
	}

	FRTHexCombatUnit BlindFireUnit(int32 UnitId, int32 TeamId, const FRTCellId& Cell)
	{
		FRTHexCombatUnit U;
		U.UnitId = UnitId;
		U.TeamId = TeamId;
		U.Cell = Cell;
		U.bAlive = true;
		return U;
	}

	UWorld* MakeBlindFireWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyBlindFireWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTUnit* SpawnBlindFireUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
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

	/**
	 * L'azione si CERCA per la sua POLICY, non per il nome: se domani `Hero.Muiren.MistVeil` cambiasse
	 * identita' o un'altra azione ereditasse la licenza, questi test seguono il dato invece di rompersi su
	 * una stringa. E' la stessa disciplina di `FindAreaAbility` nei test del puntatore.
	 */
	int32 FindAbilityWithPolicy(const ARTUnit* U, ERTLineOfSightPolicy Policy, ERTAbilityShape Shape)
	{
		for (int32 i = 0; i < U->NumAbilities(); ++i)
		{
			const URTActionData* A = U->GetAbility(i);
			if (A && !A->bSelfTarget && A->Shape == Shape && A->Def.LineOfSightPolicy == Policy)
			{
				return i;
			}
		}
		return INDEX_NONE;
	}

	/** Il mondo minimo in cui un click di targeting significa qualcosa: mappa, turn manager, controller. */
	struct FBlindFireBench
	{
		UWorld* World = nullptr;
		URTHexMapAsset* Map = nullptr;
		ARTPlayerController* PC = nullptr;
		ARTUnit* Mine = nullptr;
	};

	/**
	 * L'arena e' quella con il muro alla vista lungo `q = 0` (`MakeTestArena`, `r = -2..2`), quindi
	 * l'attaccante in `(-1,0)` non vede `(1,0)`: e' il banco che tutta questa suite condivide.
	 */
	bool SetUpBlindFireBench(FBlindFireBench& B)
	{
		B.World = MakeBlindFireWorld();
		if (!B.World) { return false; }
		B.Map = URTMatchSetupLibrary::MakeTestArena(B.World);
		ARTHexMapActor* MapActor = B.World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = B.Map;
		B.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		B.Mine = SpawnBlindFireUnit(B.World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(-1, 0, 0));
		B.PC = B.World->SpawnActor<ARTPlayerController>();
		return B.PC != nullptr && B.Mine != nullptr;
	}

	/**
	 * Attacca un `ARTHUD` al controller del banco — `#3064`.
	 *
	 * ⛔ **Senza questo, i test di `#3064` sarebbero verdi per VACUITA', e in modo invisibile.** Il canale
	 * del giocatore e' guardato da `if (ARTHUD* Hud = Cast<ARTHUD>(GetHUD()))`, e
	 * `APlayerController::MyHUD` lo popolano soltanto `SpawnDefaultHUD` e `ClientSetHUD`, che in Automation
	 * non girano: il ramo non verrebbe eseguito, i due mondi del canary produrrebbero «niente» identico, e
	 * la mutazione che il DoD 5 chiede di uccidere resterebbe inosservabile.
	 *
	 * 🔑 **Si assegna `MyHUD` direttamente invece di chiamare `ClientSetHUD`.** Quella e' una RPC client, e
	 * su un controller senza `NetConnection` il suo comportamento dipende dalla callspace: il banco
	 * dipenderebbe da un fatto dell'engine invece che da uno del gioco.
	 *
	 * ⚠️ **Si assegnano ENTRAMBI i lati**: `MyHUD` e' cio' che `GetHUD()` legge, `PlayerOwner` e' come
	 * `ARTHUD` risale al controller per la squadra (`ARTPlayerState::TeamIdOf`), che alimenta il filtro di
	 * conoscenza del tratto rifiutato. Metterne uno solo darebbe un HUD che scrive e non filtra.
	 *
	 * ⚠️ **OPT-IN, e non dentro `SetUpBlindFireBench`**: `ARTHUD::Tick` chiama `UpdateObserverVeil`, che
	 * riscriverebbe `bKnownToObserver` su ogni unita' del mondo. Questi mondi non tickano, quindi oggi e'
	 * inerte — ma un banco condiviso che porta un HUD e' una superficie in piu' per tutta la famiglia, e i
	 * test che non ne hanno bisogno restano com'erano.
	 */
	ARTHUD* AttachBlindFireHud(FBlindFireBench& B)
	{
		ARTHUD* Hud = B.World->SpawnActor<ARTHUD>();
		if (!Hud) { return nullptr; }
		Hud->PlayerOwner = B.PC;
		B.PC->MyHUD = Hud;
		return Hud;
	}

	/**
	 * Semina la conoscenza canonica della squadra che sta pianificando — `#3064`.
	 *
	 * 🔴 **Senza, il tratto nel mondo e' spento per COSTRUZIONE e nessuna asserzione su di esso vale.**
	 * `ARTTurnManager::KnowledgeForTeam` non calcola: scorre `TeamKnowledgeState` e, se la squadra non ha
	 * voce, restituisce una conoscenza **vuota**. Il banco spawna un turn manager nudo e non percorre mai il
	 * flusso di turno, quindi con l'insieme vuoto `ComputeRefusedShotLine` esce al primo cancello e
	 * `bShow` e' `false` per qualunque ingresso: un test che confrontasse `false == false` chiamerebbe
	 * corretto un canale che non si accende mai.
	 *
	 * 🔑 **`RefreshTeamKnowledgeNow()` e' PUBBLICA e gia' usata cosi'** da `RTPlayerInteractionTests.cpp` e
	 * `RTHexMovementIntegrationTests.cpp`: non serve un hook nuovo, e non serve passare dai bot.
	 *
	 * ⚠️ **Va chiamata DOPO lo spawn delle unita'**: costruisce l'elenco delle squadre dai vivi, quindi zero
	 * unita' significa zero squadre — cioe' esattamente lo stato che deve superare (`#1762`).
	 */
	bool SeedBlindFireKnowledge(FBlindFireBench& B)
	{
		ARTTurnManager* TM = Cast<ARTTurnManager>(
			UGameplayStatics::GetActorOfClass(B.World, ARTTurnManager::StaticClass()));
		if (!TM) { return false; }
		TM->RefreshTeamKnowledgeNow();
		return true;
	}

	/**
	 * Mette del Fumo su una cella — `#3064`.
	 *
	 * 🔑 **Esiste perche' senza, l'asserzione sulla portata APPLICATA e' una tautologia.** `Smoke` e' l'unica
	 * superficie del catalogo con `MaxTargetingRangeThrough > 0` (= 2) e `MakeTestArena` non ne piazza
	 * nessuna: senza Fumo `EffectiveTargetingRange` restituisce `RangeCells` invariato, e la mutazione
	 * «stampa la portata dichiarata» produce la stringa IDENTICA. Sarebbe la guardia che nessuna mutazione
	 * puo' far fallire, cioe' il difetto che `#2800` ha gia' pagato.
	 *
	 * ⚠️ **Il Fumo non tocca la linea di tiro**: nel catalogo ha `bBlocksLineOfSight = false`, e comunque il
	 * campo che la vista legge e' quello della CELLA, non quello del terreno. Cappa la distanza, non la
	 * traiettoria — che e' la distinzione su cui `OutOfRangeDiagnostic` e' costruita.
	 */
	void SeedSmoke(URTHexMapAsset* Map, const FRTCellId& Id)
	{
		FRTHexCellData Fumo(Id);
		Fumo.Surface = ERTHexSurface::Smoke;
		Map->AddOrUpdateCell(Fumo);
		Map->SortCells();
	}
}

// ======================================================================================================
// 1-2 — cio' che NON cambia: il default e' il requisito
// ======================================================================================================

/**
 * **Test 1** — un attacco diretto continua a volere la linea di tiro.
 *
 * E' il caso che `#2870` non deve toccare, e la prova sta prima delle altre apposta: una policy che
 * aprisse il tiro indiretto a tutti passerebbe ogni altro test di questo file e romperebbe il gioco.
 *
 * ⛔ **Verifica di mutazione**: sostituire il default di `FRTActionDef::LineOfSightPolicy` con
 * `NotRequired`, o togliere il ramo `Required` da `ClassifyHexTargeting`, deve rendere ROSSO questo test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlindFireDirectAttackRequiresLosTest,
	"RefactorTactics.BlindFire.DirectAttackStillRequiresLineOfSight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlindFireDirectAttackRequiresLosTest::RunTest(const FString&)
{
	const FRTCellId From(0, 0, 0);
	const FRTCellId To(3, 0, 0);
	URTHexMapAsset* Map = MakeBlindFireMap(4, FRTCellId(2, 0, 0));

	// Il DEFAULT del catalogo, non un valore scritto qui: e' cio' che un'azione ottiene senza dichiarare
	// nulla, ed e' quello che deve restare chiuso.
	const FRTActionDef Silente;
	TestTrue(TEXT("un'azione che non dichiara nulla CHIEDE la linea di tiro"), Silente.RequiresLineOfSight());

	TestTrue(TEXT("con il muro sulla linea il bersaglio e' rifiutato"),
		URTCombatLibrary::ClassifyHexTargeting(Map, From, To, /*RangeCells=*/ 5, Silente.LineOfSightPolicy)
			== ERTHexTargetReason::NoLineOfSight);

	// E il motivo resta DISTINTO: la licenza non deve confondere «coperto» con «lontano».
	TestTrue(TEXT("fuori portata resta fuori portata"),
		URTCombatLibrary::ClassifyHexTargeting(Map, From, To, /*RangeCells=*/ 1, Silente.LineOfSightPolicy)
			== ERTHexTargetReason::OutOfRange);
	return true;
}

/**
 * **Test 2** — un'ability a CELLA che dichiara `Required` resta rifiutata dal muro.
 *
 * 🔑 **E' la meta' che dimostra che la regola non e' dedotta dalla forma del bersaglio.** Se il tiro
 * indiretto fosse «le aree possono, le altre no», questo test non potrebbe esistere: qui il bersaglio E'
 * una cella, l'azione E' ad area, e il rifiuto arriva lo stesso perche' l'azione la linea la chiede.
 *
 * Il percorso e' quello del giocatore — `HandleTargetCell` — non la libreria: cio' che si misura e' che
 * l'ingresso reale legga il dato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlindFireCellAttackRequiresLosTest,
	"RefactorTactics.BlindFire.CellAttackRequiringSightIsRefusedByABlocker",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlindFireCellAttackRequiresLosTest::RunTest(const FString&)
{
	FBlindFireBench B;
	if (!TestTrue(TEXT("banco di prova"), SetUpBlindFireBench(B))) { DestroyBlindFireWorld(B.World); return false; }

	// Un'area che CHIEDE la linea: nel kit di Muiren e' `CircularTide`, e la si trova per policy.
	const int32 Idx = FindAbilityWithPolicy(B.Mine, ERTLineOfSightPolicy::Required, ERTAbilityShape::Area);
	if (!TestTrue(TEXT("premessa: il kit ha un'area che chiede la linea"), Idx != INDEX_NONE))
	{
		DestroyBlindFireWorld(B.World); return false;
	}

	B.PC->SelectActorForTest(B.Mine);
	B.Mine->SelectAbility(Idx);
	TestEqual(TEXT("un'area chiede una CELLA"), B.PC->GetPointerTargetKind(), ERTPointerTargetKind::Cell);

	// Oltre il muro: rifiutata. La cella esiste, e' in portata, e non e' visibile.
	const FRTCellId Oltre(1, 0, 0);
	TestFalse(TEXT("oltre il muro l'area che chiede la linea e' rifiutata"), B.PC->HandleTargetCell(Oltre));
	TestFalse(TEXT("e non ha scritto niente nel piano"), B.Mine->bAttackTargetsCell);

	// Controllo POSITIVO: la stessa azione, su una cella visibile, passa. Senza, un rifiuto per qualunque
	// altra ragione — portata, contesto, ricarica — supererebbe il test dicendo il falso.
	const FRTCellId InVista(-3, 0, 0);
	TestTrue(TEXT("in vista la stessa azione e' accettata"), B.PC->HandleTargetCell(InVista));
	TestTrue(TEXT("e ha scritto la cella"), B.Mine->PlannedAttackCell == InVista);

	DestroyBlindFireWorld(B.World);
	return true;
}

// ======================================================================================================
// 3-5 — cio' che la licenza apre, e cio' che NON apre
// ======================================================================================================

/**
 * **Test 3** — un'ability che dichiara `NotRequired` bersaglia una cella oltre il muro, **dall'input del
 * giocatore**.
 *
 * 🔴 **Il test vive qui e non nella libreria, ed e' il punto della issue.** `#2870` chiede che la capacita'
 * sia raggiungibile dal percorso reale: un test sulla funzione pura direbbe che la regola esiste, non che
 * qualcuno la usi. `HandleTargetCell` e' l'unico ingresso del giocatore verso `PlannedAttackCell`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlindFireCellIsTargetableTest,
	"RefactorTactics.BlindFire.BlindFireCellIsTargetableThroughABlocker",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlindFireCellIsTargetableTest::RunTest(const FString&)
{
	FBlindFireBench B;
	if (!TestTrue(TEXT("banco di prova"), SetUpBlindFireBench(B))) { DestroyBlindFireWorld(B.World); return false; }

	const int32 Idx = FindAbilityWithPolicy(B.Mine, ERTLineOfSightPolicy::NotRequired, ERTAbilityShape::Area);
	if (!TestTrue(TEXT("premessa: il roster contiene un'azione a tiro indiretto"), Idx != INDEX_NONE))
	{
		DestroyBlindFireWorld(B.World); return false;
	}

	B.PC->SelectActorForTest(B.Mine);
	B.Mine->SelectAbility(Idx);
	TestEqual(TEXT("anche il tiro indiretto chiede una CELLA"),
		B.PC->GetPointerTargetKind(), ERTPointerTargetKind::Cell);

	// La premessa del banco, misurata e non assunta: senza il muro il test proverebbe soltanto che si puo'
	// bersagliare una cella libera, che era gia' vero prima di `#2870`.
	TestTrue(TEXT("premessa: quella cella NON e' in linea di tiro"),
		URTCombatLibrary::ClassifyHexTargeting(B.Map, B.Mine->Cell, FRTCellId(1, 0, 0), /*RangeCells=*/ 4,
			ERTLineOfSightPolicy::Required) == ERTHexTargetReason::NoLineOfSight);

	const FRTCellId Oltre(1, 0, 0);
	TestTrue(TEXT("il tiro indiretto accetta la cella non visibile"), B.PC->HandleTargetCell(Oltre));
	TestTrue(TEXT("bAttackTargetsCell e' vero"), B.Mine->bAttackTargetsCell);
	TestTrue(TEXT("PlannedAttackCell e' la cella scelta"), B.Mine->PlannedAttackCell == Oltre);
	TestEqual(TEXT("e l'azione pianificata e' quella armata"), B.Mine->PlannedAbilityIndex, Idx);
	TestNull(TEXT("nessun bersaglio-unita' residuo"), (void*)B.Mine->PlannedAttackTarget.Get());

	DestroyBlindFireWorld(B.World);
	return true;
}

/**
 * **Test 4** — il piano arriva al RESOLVER e l'effetto cade sulla cella prevista.
 *
 * 🔴 **E' la prova che la semantica e' UNA.** Prima di `#2870` `CollectHexAttacks` chiamava
 * `HasLineOfSight` senza chiedere niente all'intento: un piano accettato in pianificazione sarebbe finito
 * in `BlockedIntents` — slot speso, nessun effetto, e una riga di TurnLog che nomina la copertura a chi
 * aveva pianificato di farne a meno.
 *
 * ⛔ **Verifica di mutazione**: togliere `Intent.LineOfSightPolicy` dalla condizione di
 * `CollectHexAttacks`, o non copiarlo in `RTTurnManager_Blast`, deve rendere ROSSO questo test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlindFireResolvesTest,
	"RefactorTactics.BlindFire.BlindFireReachesTheResolverAndLandsOnTheAimedCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlindFireResolvesTest::RunTest(const FString&)
{
	const FRTCellId Origin(0, 0, 0);
	const FRTCellId Aim(3, 0, 0);
	URTHexMapAsset* Map = MakeBlindFireMap(4, FRTCellId(2, 0, 0));

	TArray<FRTHexCombatUnit> Units;
	Units.Add(BlindFireUnit(0, 0, Origin));
	Units.Add(BlindFireUnit(1, 1, Aim)); // sta sulla cella mirata: e' cio' che rende osservabile l'effetto

	FRTHexAttackIntent Intent;
	Intent.AttackerId = 0;
	Intent.TargetId = INDEX_NONE; // bersaglio a CELLA, non un'unita' persa
	Intent.TargetCell = Aim;
	Intent.Shape = ERTAbilityShape::Area;
	Intent.AreaRadius = 1;
	Intent.RangeCells = 4;
	Intent.Power = 18;
	Intent.bCountsAsAttack = true;

	// ── Il controllo NEGATIVO per primo: con la policy di default lo stesso piano E' scartato. Senza questo
	//    ramo il test non distinguerebbe «la licenza funziona» da «il muro non bloccava comunque».
	{
		TArray<FRTHexAttackIntent> Chiusi;
		Chiusi.Add(Intent); // `Required` per default
		const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Chiusi, Map);
		TestEqual(TEXT("senza licenza il piano e' bloccato dalla linea"), Plan.BlockedIntents.Num(), 1);
		TestEqual(TEXT("e non produce colpi"), Plan.Hits.Num(), 0);
	}

	Intent.LineOfSightPolicy = ERTLineOfSightPolicy::NotRequired;
	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(Intent);
	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);

	TestEqual(TEXT("il piano a tiro indiretto NON e' bloccato"), Plan.BlockedIntents.Num(), 0);
	TestEqual(TEXT("e produce il proprio colpo"), Plan.Hits.Num(), 1);

	// L'effetto cade DOVE era stato mirato, non dove capita: l'impronta lo dichiara.
	if (TestEqual(TEXT("l'impronta dell'attacco e' registrata"), Plan.Footprints.Num(), 1))
	{
		TestTrue(TEXT("la cella mirata e' quella scelta"), Plan.Footprints[0].AimCell == Aim);
		TestTrue(TEXT("l'area include la cella mirata"), Plan.Footprints[0].HitCells.Contains(Aim));
		TestTrue(TEXT("l'origine e' la cella dell'attaccante"), Plan.Footprints[0].Origin == Origin);
	}
	return true;
}

/**
 * **Test 5** — cieco non significa illimitato.
 *
 * La licenza toglie **la linea** e nient'altro. La prova sta sull'ingresso del giocatore perche' e' li'
 * che un errore sarebbe raggiungibile: una portata ignorata in pianificazione diventa un piano che il
 * resolver poi scarta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlindFireRangeStillMattersTest,
	"RefactorTactics.BlindFire.BlindFireStillObeysRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlindFireRangeStillMattersTest::RunTest(const FString&)
{
	FBlindFireBench B;
	if (!TestTrue(TEXT("banco di prova"), SetUpBlindFireBench(B))) { DestroyBlindFireWorld(B.World); return false; }

	const int32 Idx = FindAbilityWithPolicy(B.Mine, ERTLineOfSightPolicy::NotRequired, ERTAbilityShape::Area);
	if (!TestTrue(TEXT("premessa: il roster contiene un'azione a tiro indiretto"), Idx != INDEX_NONE))
	{
		DestroyBlindFireWorld(B.World); return false;
	}
	const URTActionData* Ability = B.Mine->GetAbility(Idx);
	B.PC->SelectActorForTest(B.Mine);
	B.Mine->SelectAbility(Idx);

	// Una cella dell'arena oltre la portata dichiarata. Si CERCA invece di fidarsi di un letterale: la
	// portata viene dal catalogo, e un numero scritto qui invecchierebbe da solo.
	FRTCellId Lontana;
	bool bTrovata = false;
	for (const FRTHexCellData& Cell : B.Map->Cells)
	{
		if (URTHexLibrary::HexDistance(B.Mine->Cell, Cell.Id) > Ability->RangeCells)
		{
			Lontana = Cell.Id;
			bTrovata = true;
			break;
		}
	}
	if (!TestTrue(TEXT("premessa: l'arena ha una cella fuori portata"), bTrovata))
	{
		DestroyBlindFireWorld(B.World); return false;
	}

	TestFalse(TEXT("fuori portata il tiro indiretto e' rifiutato"), B.PC->HandleTargetCell(Lontana));
	TestFalse(TEXT("e non ha sporcato il piano"), B.Mine->bAttackTargetsCell);

	// Il motivo resta quello giusto: `OutOfRange`, non `Ok` e non `NoLineOfSight`.
	TestTrue(TEXT("il classificatore dice FUORI PORTATA"),
		URTCombatLibrary::ClassifyHexTargeting(B.Map, B.Mine->Cell, Lontana, Ability->RangeCells,
			ERTLineOfSightPolicy::NotRequired) == ERTHexTargetReason::OutOfRange);

	DestroyBlindFireWorld(B.World);
	return true;
}

/**
 * **Test 4b — LA CATENA, non il modulo.** Il piano nasce sui campi `Planned*` di un'unita' vera, passa da
 * `ARTTurnManager` e arriva al colpo: e' l'unico test che tocca la riga
 * `Intent.LineOfSightPolicy = Instance.Def.LineOfSightPolicy`.
 *
 * 🔴 **Senza di lui il test 4 sarebbe un modulo verde con il chiamante rotto.** Quello costruisce
 * l'intento a mano e prova la geometria; qui l'intento lo costruisce il gioco, e se la copia sparisse la
 * `Def` direbbe `NotRequired` mentre l'intento nascerebbe `Required` — il piano morirebbe in
 * `BlockedIntents` senza che una sola funzione pura se ne accorga.
 *
 * ⚠️ **L'azione e' costruita qui e non presa dal roster, ed e' dichiarato**: nessuna azione `NotRequired`
 * del catalogo risolve oggi nel **Blast** — `Hero.Muiren.MistVeil` e' ambientale e risolve nel Cleanup.
 * Il ramo del Blast e' quindi vivo e senza soggetto nel roster: un valore dichiarato che nessun test
 * copre sarebbe peggio di non averlo messo, quindi il soggetto lo fornisce il test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlindFirePolicyReachesTheIntentTest,
	"RefactorTactics.BlindFire.PolicyTravelsFromTheCatalogToTheIntent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlindFirePolicyReachesTheIntentTest::RunTest(const FString&)
{
	// Il turno si gioca due volte sullo stesso allestimento, cambiando SOLO la policy dell'azione: il
	// confronto fra i due esiti e' cio' che rende il test non vacuo.
	auto GiocaIlTurno = [this](ERTLineOfSightPolicy Policy, int32& OutDanno) -> bool
	{
		UWorld* World = MakeBlindFireWorld();
		if (!World) { return false; }

		// Muro alla vista fra chi spara e il bersaglio: senza, la licenza non avrebbe niente da togliere.
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>(World);
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 4))
		{
			FRTHexCellData Data(Id);
			Data.bBlocksLineOfSight = (Id == FRTCellId(1, 0, 0));
			Map->AddOrUpdateCell(Data);
		}
		Map->SortCells();
		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = Map;

		ARTUnit* Shooter = SpawnBlindFireUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0, 0));
		ARTUnit* Foe = SpawnBlindFireUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 0, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Shooter || !Foe) { DestroyBlindFireWorld(World); return false; }

		// Un'area del catalogo core che risolve nel Blast, con la policy sotto misura.
		const int32 Idx = RTAbilityFixtures::AddCoreAbility(Shooter, TEXT("Action.CircularAoE"));
		if (Idx == INDEX_NONE) { DestroyBlindFireWorld(World); return false; }
		URTActionData* Azione = Shooter->Abilities[Idx];
		Azione->Shape = ERTAbilityShape::Area;
		Azione->AreaRadius = 1;
		Azione->Def.LineOfSightPolicy = Policy;

		// Il piano e' quello che `HandleTargetCell` scrive: cella dichiarata, nessun bersaglio-unita'.
		Shooter->PlannedAbilityIndex = Idx;
		Shooter->PlannedAttackTarget = nullptr;
		Shooter->PlannedAttackCell = FRTCellId(2, 0, 0);
		Shooter->bAttackTargetsCell = true;

		const int32 PrimaHP = Foe->Health;
		const int32 PrimaScudo = Foe->Shield;
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }
		OutDanno = (PrimaHP - Foe->Health) + (PrimaScudo - Foe->Shield);

		DestroyBlindFireWorld(World);
		return true;
	};

	int32 DannoChiuso = 0;
	int32 DannoAperto = 0;
	if (!TestTrue(TEXT("turno con policy Required"),
			GiocaIlTurno(ERTLineOfSightPolicy::Required, DannoChiuso))
		|| !TestTrue(TEXT("turno con policy NotRequired"),
			GiocaIlTurno(ERTLineOfSightPolicy::NotRequired, DannoAperto)))
	{
		return false;
	}

	// Il controllo NEGATIVO: col requisito attivo il muro ferma tutto. Se questo non fosse zero, il banco
	// non avrebbe un muro e il ramo positivo non proverebbe niente.
	TestEqual(TEXT("con la linea richiesta il muro ferma il colpo"), DannoChiuso, 0);
	TestTrue(TEXT("con il tiro indiretto il colpo arriva a segno"), DannoAperto > 0);
	return true;
}

// ======================================================================================================
// 6-7 — privacy: il tiro indiretto non e' un rilevatore
// ======================================================================================================

/**
 * **Test 6 — IL CANARY.** Due mondi che differiscono **solo** per un nemico che l'osservatore non conosce,
 * sulla cella mirata. Cio' che il giocatore puo' osservare pianificando dev'essere identico.
 *
 * E' l'invariante di #2791 applicata al targeting:
 *
 * ```text
 * ObserverKnowledge(A) == ObserverKnowledge(B)  =>  PlanningView(A) == PlanningView(B)
 * ```
 *
 * 🔴 **Il confronto e' un ELENCO CHIUSO, e non un «almeno»**: l'esito del click, cio' che il click ha
 * scritto nel piano, e i tre campi dell'anteprima. Un invariante aperto proverebbe i canali che qualcuno
 * si e' ricordato, non l'assenza di un canale — ed e' la correzione che #2792 ha gia' dovuto fare su se'
 * stessa. `FRTBlastPreview` e' tenuto chiuso dal test qui sotto, cosi' il giorno in cui nascesse un quarto
 * campo questo confronto non resterebbe indietro in silenzio.
 *
 * ⛔ **Verifica di mutazione**: far dipendere l'esito del targeting o dell'anteprima dalla presenza del
 * nemico — per esempio rifiutando la cella occupata, o aggiungendo le celle nemiche a `HitCells` — deve
 * rendere ROSSO questo test. E' la ragione per cui il mondo `B` porta un nemico **vivo e ignoto** proprio
 * sotto il punto di mira, invece di tenerlo lontano dal corridoio d'azione: un canary che escluda il caso
 * in cui l'informazione morde e' verde per costruzione, ed e' il difetto che [`D-371`] ha trovato in
 * `HexBotPlay.HiddenEnemyFairness`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlindFireIsNotADetectorTest,
	"RefactorTactics.BlindFire.BlindFireIsNotAnEnemyDetector",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlindFireIsNotADetectorTest::RunTest(const FString&)
{
	const FRTCellId Aim(1, 0, 0); // oltre il muro di `MakeTestArena`

	// Cio' che il giocatore puo' osservare del proprio planning, raccolto in un posto solo: se due mondi
	// producono la stessa struttura, nessun canale li distingue.
	struct FOsservabile
	{
		bool bAccettato = false;
		bool bTargetsCell = false;
		FRTCellId Cell;
		FRTCellId Origin;
		TArray<FRTCellId> HitCells;
		TArray<FRTCellId> AllyCells;
	};

	auto Osserva = [this, Aim](bool bConNemicoIgnoto, FOsservabile& Out) -> bool
	{
		FBlindFireBench B;
		if (!SetUpBlindFireBench(B)) { DestroyBlindFireWorld(B.World); return false; }

		const int32 Idx = FindAbilityWithPolicy(B.Mine, ERTLineOfSightPolicy::NotRequired,
			ERTAbilityShape::Area);
		if (Idx == INDEX_NONE) { DestroyBlindFireWorld(B.World); return false; }

		ARTUnit* Nascosto = nullptr;
		if (bConNemicoIgnoto)
		{
			// Vivo, avversario, **sulla cella mirata**, e mai osservato dalla squadra di chi pianifica.
			Nascosto = SpawnBlindFireUnit(B.World, 1, URTHeroCatalogLibrary::MakeAevik(), Aim);
			if (!Nascosto) { DestroyBlindFireWorld(B.World); return false; }
			Nascosto->SetKnownToObserver(false);
		}

		B.PC->SelectActorForTest(B.Mine);
		B.Mine->SelectAbility(Idx);
		Out.bAccettato = B.PC->HandleTargetCell(Aim);
		Out.bTargetsCell = B.Mine->bAttackTargetsCell;
		Out.Cell = B.Mine->PlannedAttackCell;

		// L'anteprima si costruisce come la costruisce il controller: dallo stesso piano e dalle stesse
		// unita' dello snapshot — il nemico ignoto **compreso**, perche' e' cosi' che il gioco la chiama.
		// Il vincolo non e' che il nemico non arrivi al produttore: e' che il produttore non ne faccia
		// niente di visibile.
		const URTActionData* Ability = B.Mine->GetAbility(Idx);
		FRTBlastPreviewPlan PreviewPlan;
		PreviewPlan.AttackerId = 0;
		PreviewPlan.bHasAction = true;
		PreviewPlan.Shape = Ability->Shape;
		PreviewPlan.RangeCells = Ability->RangeCells;
		PreviewPlan.AreaRadius = Ability->AreaRadius;
		PreviewPlan.bFriendlyFire = Ability->Def.bFriendlyFire;
		PreviewPlan.bTargetsCell = true;
		PreviewPlan.TargetCell = B.Mine->PlannedAttackCell;

		TArray<FRTHexCombatUnit> HexUnits;
		HexUnits.Add(BlindFireUnit(0, B.Mine->TeamId, B.Mine->Cell));
		if (Nascosto)
		{
			HexUnits.Add(BlindFireUnit(1, Nascosto->TeamId, Nascosto->Cell));
		}

		const FRTBlastPreview Preview = URTHexCombatLibrary::MakeBlastPreview(PreviewPlan, HexUnits);
		Out.Origin = Preview.Origin;
		Out.HitCells = Preview.HitCells;
		Out.AllyCells = Preview.AllyCells;

		DestroyBlindFireWorld(B.World);
		return true;
	};

	FOsservabile Vuota;
	FOsservabile ConNemico;
	if (!TestTrue(TEXT("mondo A: cella non visibile e vuota"), Osserva(/*bConNemicoIgnoto=*/ false, Vuota))
		|| !TestTrue(TEXT("mondo B: stessa cella, nemico ignoto sopra"),
			Osserva(/*bConNemicoIgnoto=*/ true, ConNemico)))
	{
		return false;
	}

	// La premessa che rende il test non vacuo: nel mondo A il piano si fa DAVVERO. Due rifiuti identici
	// sarebbero altrettanto «uguali» e non proverebbero niente.
	TestTrue(TEXT("premessa: nel mondo vuoto il piano si fa"), Vuota.bAccettato);
	TestTrue(TEXT("premessa: e ha scritto la cella"), Vuota.bTargetsCell);

	TestEqual(TEXT("stesso esito del click"), ConNemico.bAccettato, Vuota.bAccettato);
	TestEqual(TEXT("stesso flag di bersaglio a cella"), ConNemico.bTargetsCell, Vuota.bTargetsCell);
	TestTrue(TEXT("stessa cella pianificata"), ConNemico.Cell == Vuota.Cell);
	TestTrue(TEXT("stessa origine in anteprima"), ConNemico.Origin == Vuota.Origin);
	TestEqual(TEXT("stesso numero di celle colpite"), ConNemico.HitCells.Num(), Vuota.HitCells.Num());
	TestEqual(TEXT("nessuna cella alleata segnalata in nessuno dei due"),
		ConNemico.AllyCells.Num(), Vuota.AllyCells.Num());
	for (const FRTCellId& Cell : Vuota.HitCells)
	{
		TestTrue(TEXT("le celle colpite sono le stesse"), ConNemico.HitCells.Contains(Cell));
	}
	return true;
}

/**
 * **Test 7** — il targeting diretto su un'unita' mai percepita resta impossibile (`#2741`).
 *
 * ⚠️ **Sta in questa suite e non fra i test di `#2741` per una ragione**: `#2870` introduce una policy che
 * autorizza a colpire dove non si vede, e la domanda ovvia che ne segue e' *«allora posso anche puntare
 * cio' che non vedo?»*. La risposta e' no, e i due assi restano separati — la geometria non e' la
 * conoscenza. Questo test e' il confine fra i due, misurato dove qualcuno potrebbe cancellarlo per
 * sbaglio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlindFireUnknownUnitStaysUntargetableTest,
	"RefactorTactics.BlindFire.UnknownUnitStaysUntargetable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlindFireUnknownUnitStaysUntargetableTest::RunTest(const FString&)
{
	FBlindFireBench B;
	if (!TestTrue(TEXT("banco di prova"), SetUpBlindFireBench(B))) { DestroyBlindFireWorld(B.World); return false; }

	// Adiacente, in piena vista: cio' che lo rende non bersagliabile e' SOLO la conoscenza.
	ARTUnit* Nemico = SpawnBlindFireUnit(B.World, 1, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(-2, 0, 0));
	if (!Nemico) { DestroyBlindFireWorld(B.World); return false; }

	const int32 Attacco = 0; // l'attacco base e' il primo del kit, ed e' quello che punta un'unita'
	B.PC->SelectActorForTest(B.Mine);
	B.Mine->SelectAbility(Attacco);

	// ── Controllo POSITIVO: da noto si pianifica. Senza, «non ha pianificato» proverebbe soltanto che il
	//    banco non funziona.
	Nemico->SetKnownToObserver(true);
	B.PC->HandleClickOnUnitForTest(Nemico);
	if (!TestTrue(TEXT("premessa: da NOTO il bersaglio si pianifica"), B.Mine->PlannedAttackTarget == Nemico))
	{
		DestroyBlindFireWorld(B.World); return false;
	}

	// ── E da ignoto, no. Nemmeno adesso che esiste una policy che apre il tiro al buio.
	B.Mine->PlannedAttackTarget = nullptr;
	B.Mine->PlannedAbilityIndex = INDEX_NONE;
	Nemico->SetKnownToObserver(false);
	B.PC->HandleClickOnUnitForTest(Nemico);
	TestNull(TEXT("un'unita' ignota non diventa un bersaglio"), (void*)B.Mine->PlannedAttackTarget.Get());
	TestEqual(TEXT("e nessuna azione risulta pianificata"), B.Mine->PlannedAbilityIndex, (int32)INDEX_NONE);

	DestroyBlindFireWorld(B.World);
	return true;
}

// ======================================================================================================
// Il presidio strutturale
// ======================================================================================================

/**
 * `FRTBlastPreview` e' un ELENCO CHIUSO di campi: chi ne aggiunge uno deve passare di qui e dichiarare
 * perche' non e' un canale di conoscenza.
 *
 * 🔴 **E' il presidio che questa issue lascia al futuro, ed e' l'unica forma che regge.** Oggi l'anteprima
 * e' pulita per costruzione — `HitCells` e' geometria pura, `AllyCells` filtra sulla squadra
 * dell'attaccante — quindi un test sul comportamento sarebbe verde e resterebbe verde anche il giorno in
 * cui qualcuno aggiungesse un `EnemyCells` «solo per il debug». Il canary di sopra confronta i campi che
 * conosce; questo impedisce che ne nasca uno che non conosce.
 *
 * E' la stessa forma di `Overwatch.OpportunityLeaksNoFuture`, che questo repository ha gia' scelto una
 * volta per lo stesso problema, e che #2792 prescrive per `PlanningView`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlastPreviewFieldsStayClosedTest,
	"RefactorTactics.BlindFire.BlastPreviewFieldsStayClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlastPreviewFieldsStayClosedTest::RunTest(const FString&)
{
	UScriptStruct* Struct = FRTBlastPreview::StaticStruct();
	if (!TestNotNull(TEXT("FRTBlastPreview risolta dalla reflection"), Struct)) { return false; }

	// Perche' ciascuno e' ammesso: `Origin` e `bOriginFromPlannedDash` descrivono il piano di CHI GUARDA;
	// `HitCells` e' la forma geometrica dell'area, che non dipende da chi la occupa; `AllyCells` nomina
	// solo unita' della propria squadra. Nessuno dei quattro puo' nominare un avversario.
	const TSet<FString> Ammessi = {
		TEXT("Origin"), TEXT("bOriginFromPlannedDash"), TEXT("HitCells"), TEXT("AllyCells") };

	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		const FString Name = It->GetName();
		if (!Ammessi.Contains(Name))
		{
			AddError(FString::Printf(
				TEXT("FRTBlastPreview espone il campo '%s', che non e' nell'elenco chiuso: se puo' nominare ")
				TEXT("un'unita' che l'osservatore non conosce non puo' stare nell'anteprima; se non puo', va ")
				TEXT("aggiunto qui con la ragione (#2870, #2791)"), *Name));
		}
	}
	return true;
}

/**
 * `#2936` — **il volume del fumo nasce dal GIRO DI GIOCO, non solo da una chiamata al velo.**
 *
 * ## Il buco che questo test chiude
 *
 * I due test del volume (`Veil.SurfaceVolumeFollowsTheData`, `…IsVeiledLikeEverythingElse`) chiamano
 * `ApplyKnowledgeVeil` **direttamente** dopo aver cambiato la superficie a mano. Provano la regola; non
 * provano che in partita qualcuno la percorra.
 *
 * 🔴 **E fra le due strade c'e' di mezzo tutto cio' che puo' rompersi**: `ApplyDynamicSurface` nel Cleanup,
 * la `Revision` dell'asset, il percorso PER CELLA di `#2761` con la sua guardia, e il presenter che chiama il
 * velo su `OnTeamKnowledgeRefreshed`. Un modulo verde con il chiamante rotto e' il difetto che `#2870` ha
 * gia' pagato una volta, su un'altra riga.
 *
 * ⚠️ **Nato da una seduta a schermo che non ha visto niente.** Il log diceva sette celle trasformate e zero
 * ripristini, e la board sembrava vuota: senza questo test non c'era modo di distinguere «il volume non
 * esiste» da «il volume c'e' e la camera non lo mostra». Sono due difetti diversi con lo stesso aspetto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlindFireVolumeAppearsInAGameTurnTest,
	"RefactorTactics.BlindFire.SmokeVolumeAppearsFromAGameTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlindFireVolumeAppearsInAGameTurnTest::RunTest(const FString&)
{
	UWorld* World = MakeBlindFireWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(World, 4);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Map;
	MapActor->RebuildInstances();

	// Premessa: una board di solo pavimento non ha volumi. Senza, un conteggio finale positivo non
	// direbbe che e' stato il fumo a produrlo.
	if (!TestEqual(TEXT("premessa: nessun volume su una board di pavimento"),
		MapActor->NumSurfaceVolumeInstances(), 0))
	{
		DestroyBlindFireWorld(World);
		return false;
	}

	ARTUnit* Caster = SpawnBlindFireUnit(World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(0, 0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Caster) { DestroyBlindFireWorld(World); return false; }

	// `MistVeil` si cerca per POLICY, come nel resto di questa suite: segue il dato invece di un indice.
	const int32 Idx = FindAbilityWithPolicy(Caster, ERTLineOfSightPolicy::NotRequired, ERTAbilityShape::Area);
	if (!TestTrue(TEXT("premessa: il kit porta l'azione a tiro indiretto"), Idx != INDEX_NONE))
	{
		DestroyBlindFireWorld(World);
		return false;
	}

	// Il piano e' quello che `HandleTargetCell` scrive: cella dichiarata, nessun bersaglio-unita'.
	const FRTCellId Bersaglio(2, 0, 0);
	Caster->PlannedAbilityIndex = Idx;
	Caster->DeclareAttackOnCell(Bersaglio);

	// ── Un turno INTERO, non una chiamata al velo: e' il punto del test.
	TM->LockInAndResolve();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

	// La superficie e' cambiata davvero: se questo fallisse, il test successivo misurerebbe l'assenza di una
	// causa invece dell'assenza di un effetto.
	const FRTHexCellData* Dopo = Map->FindCell(Bersaglio);
	if (!TestNotNull(TEXT("la cella bersaglio esiste ancora"), Dopo)) { DestroyBlindFireWorld(World); return false; }
	if (!TestTrue(TEXT("premessa: il Cleanup ha creato il fumo"), Dopo->Surface == ERTHexSurface::Smoke))
	{
		DestroyBlindFireWorld(World);
		return false;
	}

	// ── IL CUORE: il velo gira come in partita — il presenter lo chiama a ogni refresh — e le istanze del
	//    volume devono esistere. Si vela con tutto osservato: qui non si misura la privacy (ci pensa
	//    `Veil.SurfaceVolumeIsVeiledLikeEverythingElse`), si misura che il volume NASCA.
	TArray<FRTCellId> Tutte;
	for (int32 I = 0; I < MapActor->NumInstanceCells(); ++I) { Tutte.Add(MapActor->CellForInstance(I)); }
	FRTTeamKnowledge Conoscenza;
	Conoscenza.Version = FRTTeamKnowledge::CurrentVersion;
	Conoscenza.TeamId = 0;
	Conoscenza.TurnNumber = 1;
	Conoscenza.VisibleCells = Tutte;
	Conoscenza.ExploredCells = Tutte;
	MapActor->ApplyKnowledgeVeil(Conoscenza);

	// Sette celle: `MistVeil` dichiara `SurfaceRadius = 1`, cioe' un esagono pieno. Il numero e' un'asserzione
	// come nello scenario: un 1 direbbe che l'area ha smesso di essere un'area.
	TestEqual(TEXT("il turno di gioco produce i volumi delle sette celle"),
		MapActor->NumSurfaceVolumeInstances(), 7);

	DestroyBlindFireWorld(World);
	return true;
}

/**
 * Un bersaglio a CELLA rifiutato ARRIVA a chi gioca, e dice QUALE — `#3064`, DoD 1 e 2.
 *
 * 🔴 **E' la meta' POSITIVA del canary qui sotto, e servono tutte e due.** Un test di sola
 * indistinguibilita' — «i due mondi producono la stessa cosa» — e' soddisfatto anche da *«nessun canale
 * affatto»*: togliendo il rifiuto dal percorso a cella entrambi i mondi tacerebbero, e due silenzi sono
 * uguali fra loro quanto due frasi. La mutazione del DoD 5 cade **qui**.
 *
 * ⛔ **Si legge `CurrentRefusalText()` e non solo `GetLastTargetRefusal()`.** Il campo direbbe che lo stato
 * e' stato scritto; la stringa e' la STESSA sorgente che `DrawHUD` disegna, ed e' cio' che la issue chiede
 * — «un esito dichiarato a schermo». Un accessor parallelo passerebbe anche se il disegno leggesse altro.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCellRefusalReachesTheScreenTest,
	"RefactorTactics.BlindFire.CellRefusalReachesTheScreen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCellRefusalReachesTheScreenTest::RunTest(const FString&)
{
	FBlindFireBench B;
	if (!TestTrue(TEXT("banco di prova"), SetUpBlindFireBench(B)))
	{
		DestroyBlindFireWorld(B.World); return false;
	}

	// ⛔ **La premessa che tiene in piedi tutto il resto.** Il canale nuovo e' guardato da
	// `Cast<ARTHUD>(GetHUD())`, e headless `MyHUD` nasce nullo: senza queste due righe il ramo non verrebbe
	// eseguito e ogni asserzione che segue leggerebbe «niente» chiamandolo «corretto».
	ARTHUD* Hud = AttachBlindFireHud(B);
	if (!TestNotNull(TEXT("premessa: il controller ha un HUD collegato"), B.PC->GetHUD()))
	{
		DestroyBlindFireWorld(B.World); return false;
	}

	// Il FUMO su `(-3,0,0)`: e' cio' che rende la portata applicata DIVERSA da quella dichiarata. Senza,
	// l'asserzione sul numero sarebbe vera anche stampando `RangeCells`, cioe' inutile.
	SeedSmoke(B.Map, FRTCellId(-3, 0, 0));

	// L'azione si cerca per POLICY, non per nome: e' quella che CHIEDE la linea, cioe' il ramo del rifiuto.
	const int32 Idx = FindAbilityWithPolicy(B.Mine, ERTLineOfSightPolicy::Required, ERTAbilityShape::Area);
	if (!TestTrue(TEXT("premessa: il kit ha un'area che chiede la linea"), Idx != INDEX_NONE))
	{
		DestroyBlindFireWorld(B.World); return false;
	}
	const URTActionData* Ability = B.Mine->GetAbility(Idx);
	B.PC->SelectActorForTest(B.Mine);
	B.Mine->SelectAbility(Idx);

	// ── 1. COPERTURA — `(1,0,0)` sta oltre il muro alla vista di `MakeTestArena`, e in portata.
	TestFalse(TEXT("oltre il muro la cella e' rifiutata"), B.PC->HandleTargetCell(FRTCellId(1, 0, 0)));
	TestEqual(TEXT("e l'esito nominato e' la copertura"),
		Hud->GetLastTargetRefusal(), ERTTargetRefusal::Cover);
	const FString Coperto = Hud->CurrentRefusalText();
	TestFalse(TEXT("il rifiuto per copertura NON e' muto: il silenzio ERA il difetto di #3064"),
		Coperto.IsEmpty());

	// ── 2. DISTANZA — `(-4,0,0)` dista 3 e l'azione porta 4, quindi e' il FUMO su `(-3,0,0)` a rifiutarla.
	const FRTCellId Lontana(-4, 0, 0);
	const int32 Applicata = URTTerrainLibrary::EffectiveTargetingRange(
		B.Map, B.Mine->Cell, Lontana, Ability->RangeCells);
	// ⛔ **La premessa che rende NON tautologica l'asserzione sul numero.** Se il seeding del Fumo fallisse,
	// applicata e dichiarata coinciderebbero e il test direbbe il vero senza misurare niente.
	if (!TestTrue(TEXT("premessa: il Fumo ha DAVVERO abbassato la portata"),
		Applicata < Ability->RangeCells))
	{
		DestroyBlindFireWorld(B.World); return false;
	}

	TestFalse(TEXT("oltre la portata applicata la cella e' rifiutata"), B.PC->HandleTargetCell(Lontana));
	TestEqual(TEXT("e l'esito nominato e' la distanza"),
		Hud->GetLastTargetRefusal(), ERTTargetRefusal::Range);
	const FString Lontano = Hud->CurrentRefusalText();

	// 🔑 **Il numero e' quello APPLICATO, e la coppia di asserzioni serve intera.** La prima da sola
	// passerebbe anche stampando la dichiarata su un'arena senza Fumo; la seconda e' quella che uccide la
	// mutazione. E' lo stesso inganno che `#2766` ha tolto dal log e che `#2800` tiene fuori dallo schermo.
	TestTrue(TEXT("il messaggio porta la portata APPLICATA"),
		Lontano.Contains(FString::FromInt(Applicata)));
	TestFalse(TEXT("e NON quella dichiarata"),
		Lontano.Contains(FString::FromInt(Ability->RangeCells)));

	// ── 3. DoD 2 — le due cause si DISTINGUONO. Si asserisce il REQUISITO e non le due costanti, come fa
	//       `Combat.RefusalDistinguishesCoverFromRange`: le frasi possono cambiare, la distinzione no.
	TestNotEqual(TEXT("copertura e distanza non dicono la stessa cosa"), Lontano, Coperto);

	// ── 4. DoD 1, LA DURATA — «vive finche' il giocatore non fa un altro click», e un click RIUSCITO e'
	//       anch'esso un altro click. Si misura sostituendo, non aspettando: di timer non ce n'e' nessuno.
	//       ⚠️ L'azzeramento reale vive in `OnSelect`, che headless non e' percorribile (serve
	//       `GetHitResultUnderCursor`): cio' che si misura qui e' la meta' verificabile.
	TestTrue(TEXT("in vista e in portata la stessa azione e' accettata"),
		B.PC->HandleTargetCell(FRTCellId(-2, 0, 0)));
	TestTrue(TEXT("e un click che va a segno non lascia niente a schermo"),
		Hud->CurrentRefusalText().IsEmpty());

	DestroyBlindFireWorld(B.World);
	return true;
}

/**
 * **IL CANARY DEL RIFIUTO A CELLA** — `#3064`, DoD 4. Cella vuota e cella con sopra un nemico ignoto devono
 * produrre lo stesso esito, la stessa frase e la stessa geometria.
 *
 * 🔴 **Perche' `BlindFire.BlindFireIsNotAnEnemyDetector` non basta, e non e' un difetto suo.** Quel canary
 * passa da `HandleTargetCell` ma con un'azione `NotRequired`, che `ClassifyHexTargeting` chiude `Ok` prima
 * di guardare la geometria: il ramo del RIFIUTO — l'unico che da `#3064` parla al giocatore — non viene mai
 * eseguito, e il suo elenco chiuso di osservabili non contiene alcun campo dell'HUD. Sarebbe rimasto verde
 * qualunque cosa il canale nuovo facesse, e la DoD 4 sarebbe stata soddisfatta in modo VACUO. Resta verde,
 * e va bene cosi': questo test e' l'altra meta', sul ramo che quello non puo' raggiungere.
 *
 * ⛔ **Elenco CHIUSO come il fratello, e non un «almeno».** Se `#3064` aprisse un settimo osservabile,
 * questa struct va estesa nello stesso commit — o il confronto resta indietro in silenzio, che e' il
 * difetto che il docstring del canary gemello dichiara di voler impedire.
 *
 * ⚠️ **Il TESTO e non solo l'enum.** Due esiti diversi con la stessa frase non sarebbero un canale; lo
 * stesso esito con frasi diverse lo sarebbe. Cio' che il giocatore riceve e' la stringa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCellRefusalIsNotADetectorTest,
	"RefactorTactics.BlindFire.CellRefusalIsNotAnEnemyDetector",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCellRefusalIsNotADetectorTest::RunTest(const FString&)
{
	const FRTCellId Aim(1, 0, 0); // oltre il muro alla vista: il ramo del RIFIUTO, non quello dell'accettazione

	struct FSegnoDelRifiuto
	{
		bool bAccettato = true;
		ERTTargetRefusal Refusal = ERTTargetRefusal::None;
		FString Testo;
		bool bLineaAccesa = false;
		FRTCellId LineaDa;
		FRTCellId LineaFinoA;
	};

	auto Osserva = [Aim](bool bConNemicoIgnoto, FSegnoDelRifiuto& Out) -> bool
	{
		FBlindFireBench B;
		if (!SetUpBlindFireBench(B)) { DestroyBlindFireWorld(B.World); return false; }
		ARTHUD* Hud = AttachBlindFireHud(B);
		if (!Hud || B.PC->GetHUD() == nullptr) { DestroyBlindFireWorld(B.World); return false; }

		const int32 Idx = FindAbilityWithPolicy(B.Mine, ERTLineOfSightPolicy::Required,
			ERTAbilityShape::Area);
		if (Idx == INDEX_NONE) { DestroyBlindFireWorld(B.World); return false; }

		if (bConNemicoIgnoto)
		{
			// Vivo, avversario, **sulla cella mirata**, e mai osservato: e' il posto in cui l'informazione
			// morderebbe. Un canary che tenesse il nemico lontano dal bersaglio sarebbe verde per
			// costruzione.
			ARTUnit* Nascosto = SpawnBlindFireUnit(B.World, 1, URTHeroCatalogLibrary::MakeAevik(), Aim);
			if (!Nascosto) { DestroyBlindFireWorld(B.World); return false; }
			Nascosto->SetKnownToObserver(false);
			// ⚠️ La conoscenza si impone DOPO lo spawn e si verifica PRIMA del click: `bKnownToObserver`
			// nasce `true` ed e' il velo a correggerlo nel `Tick` dell'HUD, che qui non gira.
			if (Nascosto->IsKnownToObserver()) { DestroyBlindFireWorld(B.World); return false; }
		}

		// La conoscenza di squadra si semina in ENTRAMBI i mondi: senza, il tratto sarebbe spento per
		// costruzione e i tre campi geometrici coinciderebbero senza sorvegliare nulla.
		if (!SeedBlindFireKnowledge(B)) { DestroyBlindFireWorld(B.World); return false; }

		B.PC->SelectActorForTest(B.Mine);
		B.Mine->SelectAbility(Idx);
		Out.bAccettato = B.PC->HandleTargetCell(Aim);
		Out.Refusal = Hud->GetLastTargetRefusal();
		Out.Testo = Hud->CurrentRefusalText();
		if (const ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(B.World))
		{
			Out.bLineaAccesa = HexMap->HasPreviewSightBlock();
			Out.LineaDa = HexMap->GetPreviewSightFrom();
			Out.LineaFinoA = HexMap->GetPreviewSightBlockedAt();
		}

		DestroyBlindFireWorld(B.World);
		return true;
	};

	FSegnoDelRifiuto Vuota;
	FSegnoDelRifiuto ConNemico;
	if (!TestTrue(TEXT("mondo A: cella oltre il muro, vuota"), Osserva(/*bConNemicoIgnoto=*/ false, Vuota))
		|| !TestTrue(TEXT("mondo B: stessa cella, nemico ignoto sopra"),
			Osserva(/*bConNemicoIgnoto=*/ true, ConNemico)))
	{
		return false;
	}

	// ⛔ **LE DUE PREMESSE DI NON VACUITA'.** Senza, «uguale» non prova niente: se il rifiuto a cella
	// smettesse di arrivare a schermo, due stringhe vuote sarebbero uguali quanto due frasi e questo canary
	// resterebbe verde proprio sulla mutazione che il DoD 5 chiede di uccidere.
	TestFalse(TEXT("premessa: nel mondo vuoto il click e' RIFIUTATO"), Vuota.bAccettato);
	TestFalse(TEXT("premessa: e il rifiuto dice qualcosa"), Vuota.Testo.IsEmpty());

	TestEqual(TEXT("stesso esito del click"), ConNemico.bAccettato, Vuota.bAccettato);
	TestEqual(TEXT("stesso esito di rifiuto"), ConNemico.Refusal, Vuota.Refusal);
	TestEqual(TEXT("stesso TESTO, carattere per carattere"), ConNemico.Testo, Vuota.Testo);
	TestEqual(TEXT("stessa accensione del tratto nel mondo"), ConNemico.bLineaAccesa, Vuota.bLineaAccesa);
	TestTrue(TEXT("stessa origine del tratto"), ConNemico.LineaDa == Vuota.LineaDa);
	TestTrue(TEXT("stesso punto d'arresto"), ConNemico.LineaFinoA == Vuota.LineaFinoA);
	return true;
}

/**
 * Il tratto nel MONDO segue il click a cella, e obbedisce al velo sull'OSTACOLO — `#3064`, DoD 3.
 *
 * 🔑 **Due mondi che differiscono SOLO per cio' che la squadra ha osservato**, zero nemici in entrambi:
 * stessa mappa, stessa azione, stesso click, stessa geometria. L'unica variabile e' se la conoscenza
 * canonica sia stata rinfrescata, cioe' se la cella che BLOCCA sia nota.
 *
 * 🔴 **Perche' la coppia serve intera.** Il solo mondo velato («spento») sarebbe soddisfatto da un canale
 * che non si accende MAI; il solo mondo noto («acceso») sarebbe soddisfatto da un canale senza cancello. E'
 * la stessa forma della controprova 3 di `HUD.RefusedShotBreaksAtTheBlocker`, portata dal canale 2D — dove
 * il cancello c'era gia' — a quello 3D, dove fino a `#3064` non c'era.
 *
 * ⚠️ **Il tratto FANTASMA acceso prima del click non e' scenografia**: e' la misura del difetto di durata.
 * `SetPreviewSightBlock` aveva un solo chiamante di produzione, dentro il ramo di rifiuto a unita', quindi
 * un click su una cella non lo raggiungeva e il segmento del click precedente restava a schermo — mentre la
 * frase, azzerata in `OnSelect`, era gia' sparita.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCellRefusalWorldLineFollowsTheVeilTest,
	"RefactorTactics.BlindFire.CellRefusalWorldLineFollowsTheVeil",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCellRefusalWorldLineFollowsTheVeilTest::RunTest(const FString&)
{
	const FRTCellId Oltre(1, 0, 0);
	const FRTCellId Muro(0, 0, 0);       // dove il muro alla vista di `MakeTestArena` ferma la linea
	const FRTCellId Fantasma(3, -1, 0);  // un valore che nessun esito reale di questo click produrrebbe

	struct FTrattoNelMondo
	{
		bool bAccesa = false;
		FRTCellId Da;
		FRTCellId FinoA;
		FString Testo;
		bool bOstacoloNoto = false;
	};

	auto Osserva = [Oltre, Fantasma, Muro](bool bConConoscenza, FTrattoNelMondo& Out) -> bool
	{
		FBlindFireBench B;
		if (!SetUpBlindFireBench(B)) { DestroyBlindFireWorld(B.World); return false; }
		ARTHUD* Hud = AttachBlindFireHud(B);
		ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(B.World);
		if (!Hud || !HexMap || B.PC->GetHUD() == nullptr)
		{
			DestroyBlindFireWorld(B.World); return false;
		}

		const int32 Idx = FindAbilityWithPolicy(B.Mine, ERTLineOfSightPolicy::Required,
			ERTAbilityShape::Area);
		if (Idx == INDEX_NONE) { DestroyBlindFireWorld(B.World); return false; }

		// L'UNICA differenza fra i due mondi.
		if (bConConoscenza && !SeedBlindFireKnowledge(B))
		{
			DestroyBlindFireWorld(B.World); return false;
		}

		// Il tratto del «click precedente», acceso a mano.
		HexMap->SetPreviewSightBlock(/*bBlocked=*/ true, Fantasma, Fantasma);

		B.PC->SelectActorForTest(B.Mine);
		B.Mine->SelectAbility(Idx);
		if (B.PC->HandleTargetCell(Oltre)) { DestroyBlindFireWorld(B.World); return false; }

		Out.bAccesa = HexMap->HasPreviewSightBlock();
		Out.Da = HexMap->GetPreviewSightFrom();
		Out.FinoA = HexMap->GetPreviewSightBlockedAt();
		Out.Testo = Hud->CurrentRefusalText();

		// ⛔ **La premessa che rende questo test DIAGNOSTICO invece che ambiguo.** Senza, un rosso su
		// «la linea si accende» avrebbe due cause indistinguibili: la feature rotta, oppure
		// `RefreshTeamKnowledgeNow()` che non ha messo l'ostacolo fra le celle note — cioe' uno scenario
		// che non esercita il ramo. Misurare la conoscenza le separa, e la separa PRIMA di leggere l'esito.
		//
		// 🔑 Si legge dalla stessa porta che usa l'HUD — `KnowledgeForTeamPublic` sulla squadra del
		// controller — cosi' la premessa misura esattamente cio' che il codice sotto prova consulta, e non
		// un canale parallelo che potrebbe divergere.
		if (const ARTTurnManager* TM = Cast<ARTTurnManager>(
			UGameplayStatics::GetActorOfClass(B.World, ARTTurnManager::StaticClass())))
		{
			const FRTTeamKnowledge K = TM->KnowledgeForTeamPublic(ARTPlayerState::TeamIdOf(B.PC));
			Out.bOstacoloNoto = K.VisibleCells.Contains(Muro) || K.ExploredCells.Contains(Muro);
		}

		DestroyBlindFireWorld(B.World);
		return true;
	};

	FTrattoNelMondo Noto;
	FTrattoNelMondo Velato;
	if (!TestTrue(TEXT("mondo A: la squadra conosce l'ostacolo"), Osserva(/*bConConoscenza=*/ true, Noto))
		|| !TestTrue(TEXT("mondo B: la stessa geometria, mai osservata"),
			Osserva(/*bConConoscenza=*/ false, Velato)))
	{
		return false;
	}

	// ⛔ **LE DUE PREMESSE SULLO SCENARIO, e vengono prima di ogni asserzione sulla feature.** Separano
	// «la feature e' rotta» da «lo scenario non esercita il ramo»: se cadono queste, il test sta misurando
	// il mondo sbagliato e cio' che segue non significa niente — verde o rosso che sia.
	if (!TestTrue(TEXT("premessa: nel mondo A la squadra CONOSCE l'ostacolo"), Noto.bOstacoloNoto)
		|| !TestFalse(TEXT("premessa: nel mondo B non lo conosce"), Velato.bOstacoloNoto))
	{
		return false;
	}

	// ── MONDO A — il canale esiste e indica il muro.
	TestTrue(TEXT("la linea del mondo si accende sull'ostacolo"), Noto.bAccesa);
	TestTrue(TEXT("e parte dal tiratore"), Noto.Da == FRTCellId(-1, 0, 0));
	// ⛔ Si ferma sull'OSTACOLO e non sul bersaglio: disegnarla fino in fondo direbbe che la traiettoria
	// arriva, che e' l'opposto dell'informazione.
	TestTrue(TEXT("e si ferma sull'ostacolo, non sul bersaglio"), Noto.FinoA == Muro);

	// ── MONDO B — [D-225] sull'OSTACOLO. Il rifiuto resta DETTO, il segno che indica una cella no.
	TestFalse(TEXT("un ostacolo mai osservato non accende niente nel mondo"), Velato.bAccesa);
	TestFalse(TEXT("il click a cella ha comunque riscritto il canale: il fantasma non sopravvive"),
		Velato.bAccesa && Velato.FinoA == Fantasma);

	// ⛔ **E la FRASE non cambia fra i due mondi**, che e' la scelta d'autore di questa fetta: «Coperto: la
	// linea di tiro e' interrotta» parla del MIO tiro e non nomina una cella, mentre il tratto ne INDICA
	// una. Tacere la frase su un muro mai esplorato riaprirebbe il silenzio di `#3064` per un sottoinsieme
	// di celle — e un silenzio correlato a cio' che ho esplorato e' informativo per me, non su di me.
	TestEqual(TEXT("la frase e' la stessa: il velo tocca il segno che indica, non quello che descrive"),
		Velato.Testo, Noto.Testo);
	TestFalse(TEXT("premessa: e in entrambi i mondi la frase c'e'"), Noto.Testo.IsEmpty());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
