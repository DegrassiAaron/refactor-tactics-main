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
#include "Player/RTPlayerController.h"
#include "Tests/RTAbilityFixtures.h"
#include "Player/RTPointerInteraction.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
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

#endif // WITH_DEV_AUTOMATION_TESTS
