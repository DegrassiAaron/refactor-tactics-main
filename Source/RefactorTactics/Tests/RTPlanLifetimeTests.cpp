#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnRules.h"
#include "Unit/RTUnit.h"
#include "Turn/RTReactionOpportunityTypes.h" // la condizione dichiarata fa parte del piano

#if WITH_DEV_AUTOMATION_TESTS

/**
 * **Un piano non sopravvive al turno.**
 *
 * L'invariante e' dichiarata in `ARTTurnManager` in otto punti — «consumato: attivata o no, il piano non
 * sopravvive al turno» (riga 830), «consumato in Prep» (1141), «consumato per questo turno (valido o no)»
 * (1185), «lo slot principale e' speso» (1334) — e per `ARTUnit::PlaceOnCell` vale sul movimento
 * («dopo un movimento, il piano riparte dalla cella attuale»).
 *
 * Nessun test la verificava. Finche' gli scenari esprimevano solo movimento la cosa era innocua, perche' il
 * movimento si riallinea da solo; con gli intent di abilita' (S2-2) diventa la differenza fra un'unita' che
 * agisce una volta e una che ripete l'azione ogni turno — un difetto che nello scenario si leggerebbe come
 * un bug di gioco.
 *
 * Test di **caratterizzazione**, dichiarato tale: non aggiunge comportamento, fissa quello che c'e'. Vive
 * FUORI dallo Scenario Harness, e pianifica scrivendo direttamente i campi di `ARTUnit`, perche' il reset
 * per turno del runner mascherebbe esattamente cio' che qui si vuole misurare.
 */
namespace
{
	UWorld* MakePlanWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyPlanWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTHexMapActor* SpawnPlanMap(UWorld* World, int32 Radius)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		if (Actor) { Actor->MapAsset = M; }
		return Actor;
	}

	/** Eroe dal CATALOGO: le invarianti sulle azioni si verificano sul roster, non sugli archetipi di test. */
	ARTUnit* SpawnPlanHero(UWorld* World, FName HeroId, int32 TeamId, const FRTCellId& Cell)
	{
		const URTHeroData* Hero = nullptr;
		for (const URTHeroData* Candidate : URTHeroCatalogLibrary::GetHeroRoster())
		{
			if (Candidate && Candidate->HeroId == HeroId) { Hero = Candidate; break; }
		}
		if (!World || !Hero) { return nullptr; }

		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = false; // i piani li scrive il test: nessun bot deve sovrascriverli
		U->DispatchBeginPlay();      // senza, i cooldown non esistono e ogni abilita' risulta sempre pronta
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/** Primo indice di abilita' con quello slot, o INDEX_NONE. Per identita', mai per posizione nel kit. */
	int32 FindAbilityIndexBySlot(const ARTUnit* Unit, ERTActionSlot Slot)
	{
		if (!Unit) { return INDEX_NONE; }
		for (int32 I = 0; I < Unit->NumAbilities(); ++I)
		{
			const URTActionData* Ability = Unit->GetAbility(I);
			if (Ability && Ability->Def.Slot == Slot) { return I; }
		}
		return INDEX_NONE;
	}

	void PlayPlanTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/** Cio' che deve essere vuoto all'inizio del turno successivo. */
	struct FRTPlanSlots
	{
		int32 Main = INDEX_NONE;
		bool bHasTarget = false;
		int32 Dash = INDEX_NONE;
		int32 Reaction = INDEX_NONE;
	};

	FRTPlanSlots ReadPlanSlots(const ARTUnit* U)
	{
		FRTPlanSlots S;
		if (U)
		{
			S.Main = U->PlannedAbilityIndex;
			S.bHasTarget = (U->PlannedAttackTarget != nullptr);
			S.Dash = U->PlannedDashAbility;
			S.Reaction = U->PlannedReactionAbility;
		}
		return S;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlansDoNotSurviveTheTurnTest,
	"RefactorTactics.Turn.PlansDoNotSurviveTheTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlansDoNotSurviveTheTurnTest::RunTest(const FString&)
{
	UWorld* World = MakePlanWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	SpawnPlanMap(World, /*Radius=*/ 6);

	ARTUnit* Attacker = SpawnPlanHero(World, TEXT("Hero.Riktor"), /*Team=*/ 0, FRTCellId(-1, 0, 0));
	ARTUnit* Target   = SpawnPlanHero(World, TEXT("Hero.Wraith"),  /*Team=*/ 1, FRTCellId( 1, 0, 0));
	if (!Attacker || !Target)
	{
		AddError(TEXT("impossibile allestire le due unita' dal catalogo eroi"));
		DestroyPlanWorld(World);
		return false;
	}

	const int32 MainIdx     = FindAbilityIndexBySlot(Attacker, ERTActionSlot::Main);
	const int32 ReactionIdx = FindAbilityIndexBySlot(Attacker, ERTActionSlot::Reaction);

	// Si dichiara TUTTO cio' che un giocatore puo' dichiarare in un turno.
	Attacker->PlannedAbilityIndex   = MainIdx;
	Attacker->PlannedAttackTarget   = Target;
	Attacker->PlannedReactionAbility = ReactionIdx;
	// Compresa la CONDIZIONE sulla reazione ([D-109]): fa parte del piano quanto lo slot che la ospita.
	Attacker->SetPlannedReactionCondition(FRTDeclaredCondition(
		URTReactionOpportunityLibrary::TargetHealthAtOrBelowPercent(), 50));

	// Il dash e' uno slot a parte: si prende un'abilita' che sposta davvero, non una qualunque.
	int32 DashIdx = INDEX_NONE;
	for (int32 I = 0; I < Attacker->NumAbilities(); ++I)
	{
		const URTActionData* A = Attacker->GetAbility(I);
		if (A && A->Def.MovementStyle != ERTMovementStyle::None && A->Def.Slot == ERTActionSlot::Main)
		{
			DashIdx = I;
			break;
		}
	}
	if (DashIdx != INDEX_NONE)
	{
		Attacker->PlannedDashAbility = DashIdx;
		Attacker->PlannedDashCell = FRTCellId(-2, 0, 0);
	}

	// Precondizione: se non c'e' niente da consumare, il test non prova niente.
	if (!TestNotEqual(TEXT("l'eroe ha un'azione principale a catalogo"), MainIdx, static_cast<int32>(INDEX_NONE)))
	{
		DestroyPlanWorld(World);
		return false;
	}

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("turn manager"), TM)) { DestroyPlanWorld(World); return false; }

	PlayPlanTurn(TM);

	const FRTPlanSlots After = ReadPlanSlots(Attacker);
	TestEqual(TEXT("l'azione principale e' stata consumata"), After.Main, INDEX_NONE);
	TestFalse(TEXT("il bersaglio pianificato non sopravvive"), After.bHasTarget);
	TestEqual(TEXT("lo scatto pianificato non sopravvive"), After.Dash, INDEX_NONE);
	TestEqual(TEXT("la reazione pianificata non sopravvive"), After.Reaction, INDEX_NONE);

	// E nemmeno la sua condizione. Azzerare lo slot e lasciare la condizione la renderebbe ORFANA: il turno
	// dopo, armando una reazione qualunque, il giocatore se la ritroverebbe addosso senza averla chiesta — e
	// ristretta com'e', gli farebbe saltare un colpo che credeva di avere. `SetPlannedReactionCondition`
	// rifiuta di crearne una orfana all'ingresso; se il reset non la togliesse, la stessa cosa entrerebbe
	// dalla porta di servizio.
	TestFalse(TEXT("la condizione della reazione non sopravvive"),
		Attacker->PlannedReactionCondition.IsDeclared());

	// Il movimento si riallinea da solo (`PlaceOnCell`): qui si verifica che sia davvero cosi', perche' e'
	// la meta' dell'invariante che nessuno aveva mai controllato.
	TestEqual(TEXT("la destinazione pianificata coincide con la cella raggiunta"),
		Attacker->PlannedCell, Attacker->Cell);
	TestEqual(TEXT("il percorso composito e' consumato"), Attacker->PlannedPath.Num(), 0);
	TestEqual(TEXT("i waypoint sono consumati"), Attacker->PlannedWaypoints.Num(), 0);

	DestroyPlanWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDiscardedPlanDoesNotSurviveTest,
	"RefactorTactics.Turn.DiscardedPlanDoesNotSurviveTheTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDiscardedPlanDoesNotSurviveTest::RunTest(const FString&)
{
	// Il caso che il percorso felice non copre: l'azione NON viene eseguita perche' il bersaglio e' fuori
	// portata. Se il consumo del piano vivesse solo nel ramo «azione risolta», qui il piano sopravviverebbe
	// e l'unita' ritenterebbe da sola il turno dopo — con un attacco che il giocatore non ha ridichiarato.
	UWorld* World = MakePlanWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	SpawnPlanMap(World, /*Radius=*/ 10);

	ARTUnit* Attacker = SpawnPlanHero(World, TEXT("Hero.Riktor"), /*Team=*/ 0, FRTCellId(-9, 0, 0));
	ARTUnit* Target   = SpawnPlanHero(World, TEXT("Hero.Wraith"),  /*Team=*/ 1, FRTCellId( 9, 0, 0));
	if (!Attacker || !Target)
	{
		AddError(TEXT("impossibile allestire le due unita' dal catalogo eroi"));
		DestroyPlanWorld(World);
		return false;
	}

	const int32 MainIdx = FindAbilityIndexBySlot(Attacker, ERTActionSlot::Main);
	Attacker->PlannedAbilityIndex = MainIdx;
	Attacker->PlannedAttackTarget = Target; // 18 celle: fuori da qualunque portata del catalogo v0.1

	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("turn manager"), TM)) { DestroyPlanWorld(World); return false; }

	PlayPlanTurn(TM);

	TestEqual(TEXT("il bersaglio e' rimasto intatto (l'attacco non e' avvenuto)"),
		Target->Health, Target->MaxHealth);

	const FRTPlanSlots After = ReadPlanSlots(Attacker);
	TestEqual(TEXT("un'azione scartata viene consumata lo stesso"), After.Main, INDEX_NONE);
	TestFalse(TEXT("il bersaglio pianificato non sopravvive a un'azione scartata"), After.bHasTarget);

	DestroyPlanWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
