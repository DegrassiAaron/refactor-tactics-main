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
#include "Player/RTPlayerController.h" // l'invariante dell'armo si prova dalla porta del giocatore (#3833)
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

	ARTUnit* Attacker = SpawnPlanHero(World, TEXT("Hero.Branth"), /*Team=*/ 0, FRTCellId(-1, 0, 0));
	ARTUnit* Target   = SpawnPlanHero(World, TEXT("Hero.Ivrin"),  /*Team=*/ 1, FRTCellId( 1, 0, 0));
	if (!Attacker || !Target)
	{
		AddError(TEXT("impossibile allestire le due unita' dal catalogo eroi"));
		DestroyPlanWorld(World);
		return false;
	}

	const int32 MainIdx     = FindAbilityIndexBySlot(Attacker, ERTActionSlot::Main);
	const int32 ReactionIdx = FindAbilityIndexBySlot(Attacker, ERTActionSlot::Reaction);

	// Si dichiara TUTTO cio' che un giocatore puo' dichiarare in un turno.
	// ⛔ **L'armamento va acceso, o l'asserzione su [D-397] §5 in fondo sarebbe VACUA**: `SelectedAbilityIndex`
	// nasce gia' `INDEX_NONE`, quindi senza questa riga «torna al neutro» sarebbe vero senza che il Cleanup
	// faccia niente.
	Attacker->SelectAbility(MainIdx);
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

	// 🔴 **E l'ARMAMENTO torna al neutro, per [D-397] §5.** Non e' parte del piano — `SelectedAbilityIndex`
	// dice *«cosa sto per fare»*, non *«cosa ho dichiarato»* — ma muore nello stesso punto e per la stessa
	// ragione: *«uno slot armato dopo che il piano e' stato consumato afferma una scelta che non esiste
	// piu'»*. Con `bPlanned` (`#2988`) la contraddizione diventa visibile a schermo: **armato senza
	// pianificato**.
	//
	// ⚠️ **Il cambio di UNITA' invece non disarma, e la decisione lo conferma** invece di correggerlo:
	// l'armamento e' stato dell'unita', e ritrovarcelo e' la conseguenza del modello. E' pinnato da
	// `HudViewModel.ArmingIsPerUnitAndReturnsToNeutralAtCleanup`, che prova le due meta' insieme.
	TestEqual(TEXT("l'armamento torna al neutro di D-128 nel Cleanup"),
		Attacker->SelectedAbilityIndex, static_cast<int32>(INDEX_NONE));

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

	ARTUnit* Attacker = SpawnPlanHero(World, TEXT("Hero.Branth"), /*Team=*/ 0, FRTCellId(-9, 0, 0));
	ARTUnit* Target   = SpawnPlanHero(World, TEXT("Hero.Ivrin"),  /*Team=*/ 1, FRTCellId( 9, 0, 0));
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

/**
 * 🔴 **UN'AZIONE CON RICARICA ESCE DAL TURNO CONSUMATA, DISARMATA, E L'INPUT NON LA RIARMA** — `#3833`.
 *
 * E' l'invariante che permette di togliere le guardie sul modello — `MostraLaPortata`, `ArmedChargeTargets` e i
 * gestori di click — che controllavano `CanUseAbility` sull'azione **armata**: *«un'unita' viva, in Pianificazione,
 * non ha un'azione armata in ricarica»*. Si forma da tre pezzi, e prima di questo test ognuno era pinnato per conto
 * suo, mai insieme:
 *
 *  - la guardia di `SelectAbilityForCurrent` (#3830), provata con la ricarica scritta sul modello;
 *  - il disarmo del Cleanup (D-397 §5), provato da `PlansDoNotSurviveTheTurn` con un'azione **senza** ricarica;
 *  - la ricarica che nasce solo in risoluzione.
 *
 * Qui passano tutti dal flusso vero: si arma dalla porta del giocatore, si pianifica, si gioca un turno con un
 * `ARTTurnManager` vero, e si guarda cosa trova il turno dopo.
 *
 * ⚠️ **Per ENTRAMBE le squadre**, ognuna nel proprio mondo. La revisione indipendente ha trovato che un Cleanup che
 * disarmasse solo la squadra 0 lasciava verde sia questo test sia `PlansDoNotSurviveTheTurn`: il codice vero non
 * distingue per squadra, ma niente lo pinnava.
 *
 * ⚠️ **Le premesse portano il peso.** Senza `GetAbilityCooldown > 0` l'azione potrebbe non essere stata eseguita
 * (e allora «disarmata» e «non riarmabile» sarebbero veri per un motivo qualunque); senza il controllo positivo
 * sul riarmo di un'azione PRONTA, «rifiutata» varrebbe anche con l'input spento.
 *
 * ⛔ **Verifica di mutazione**: togliere `SelectAbility(INDEX_NONE)` dal Cleanup fa cadere B; disarmare solo la squadra 0
 * fa cadere B per la squadra 1; togliere la guardia di `SelectAbilityForCurrent` fa cadere C.
 *
 * ⚠️ **Cosa NON copre, e la revisione l'ha misurato**: un'unita' UCCISA nel Cleanup (per esempio dal Burning) resta
 * armata e in ricarica, perche' il ciclo la salta, finche' `DestroyDefeatedUnits` non la toglie in `ConcludeTurn`; non e'
 * uno stato che il giocatore possa toccare, e lo pinna `Turn.AUnitKilledInTheCleanupStaysArmedUntilDestroyedOutOfReachOfInput`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCooldownActionLeavesTheTurnDisarmedTest,
	"RefactorTactics.Turn.ACooldownActionLeavesTheTurnConsumedDisarmedAndNotRearmable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCooldownActionLeavesTheTurnDisarmedTest::RunTest(const FString&)
{
	const auto Prova = [this](int32 Squadra) -> bool
	{
		// Il nome della squadra in ogni messaggio: un rosso deve dire QUALE delle due lo ha prodotto.
		const auto E = [Squadra](const TCHAR* Testo) { return FString::Printf(TEXT("squadra %d - %s"), Squadra, Testo); };

		UWorld* World = MakePlanWorld();
		if (!TestNotNull(*E(TEXT("mondo di prova")), World)) { return false; }
		SpawnPlanMap(World, /*Radius=*/ 6);

		ARTUnit* Attacker = SpawnPlanHero(World, TEXT("Hero.Aevik"), /*Team=*/ Squadra, FRTCellId(-1, 0, 0));
		ARTUnit* Target   = SpawnPlanHero(World, TEXT("Hero.Ivrin"), /*Team=*/ 1 - Squadra, FRTCellId( 1, 0, 0));
		if (!Attacker || !Target)
		{
			AddError(*E(TEXT("impossibile allestire le due unita' dal catalogo eroi")));
			DestroyPlanWorld(World);
			return false;
		}

		// Un'azione ATTIVA con ricarica di almeno due turni: dopo il tick del Cleanup ne resta ancora uno, quindi
		// «in ricarica» e' leggibile all'inizio del turno dopo. Con ricarica uno tornerebbe pronta nello stesso Cleanup.
		int32 ConRicarica = INDEX_NONE;
		for (int32 I = 0; I < Attacker->NumAbilities(); ++I)
		{
			const URTActionData* A = Attacker->GetAbility(I);
			if (A && A->Def.Slot == ERTActionSlot::Main && !A->bSelfTarget
				&& A->Def.MovementStyle == ERTMovementStyle::None && A->CooldownTurns >= 2)
			{
				ConRicarica = I;
				break;
			}
		}
		if (!TestNotEqual(*E(TEXT("premessa: Aevik ha un'azione attiva con ricarica di almeno due turni")),
				ConRicarica, static_cast<int32>(INDEX_NONE)))
		{
			DestroyPlanWorld(World);
			return false;
		}

		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
		if (!TestNotNull(*E(TEXT("turn manager")), TM) || !TestNotNull(*E(TEXT("controller")), PC))
		{
			DestroyPlanWorld(World);
			return false;
		}
		PC->SelectActorForTest(Attacker);

		// --- A. si ARMA dalla porta del giocatore, pronta, e si pianifica -------------------------
		PC->SelectAbilityForCurrentForTest(ConRicarica);
		if (!TestEqual(*E(TEXT("A: pronta, l'azione si arma dalla porta del giocatore")),
				Attacker->SelectedAbilityIndex, ConRicarica))
		{
			DestroyPlanWorld(World);
			return false;
		}
		Attacker->PlannedAbilityIndex = ConRicarica;
		Attacker->PlannedAttackTarget = Target;

		PlayPlanTurn(TM);

		// --- B. esce dal turno consumata e DISARMATA -----------------------------------------------
		if (!TestTrue(*E(TEXT("premessa ANTI-VACUITA': l'azione e' stata eseguita, quindi e' in ricarica")),
				Attacker->GetAbilityCooldown(ConRicarica) > 0)
			|| !TestFalse(*E(TEXT("premessa: e CanUseAbility lo conferma")), Attacker->CanUseAbility(ConRicarica)))
		{
			DestroyPlanWorld(World);
			return false;
		}
		TestEqual(*E(TEXT("B: il Cleanup l'ha disarmata: niente armato E in ricarica all'inizio del turno dopo")),
			Attacker->SelectedAbilityIndex, static_cast<int32>(INDEX_NONE));

		// --- C. e l'input non la riarma ------------------------------------------------------------
		// Il controllo positivo prima: un'azione PRONTA si arma ancora, cioe' l'input e' vivo e il mondo non e' in sola
		// lettura. Senza, il rifiuto qui sotto potrebbe venire da un input spento.
		int32 Pronta = INDEX_NONE;
		for (int32 I = 0; I < Attacker->NumAbilities(); ++I)
		{
			if (I != ConRicarica && Attacker->CanUseAbility(I)) { Pronta = I; break; }
		}
		if (!TestNotEqual(*E(TEXT("premessa: esiste un'altra azione pronta per il controllo positivo")),
				Pronta, static_cast<int32>(INDEX_NONE)))
		{
			DestroyPlanWorld(World);
			return false;
		}
		PC->SelectAbilityForCurrentForTest(Pronta);
		if (!TestEqual(*E(TEXT("C: controllo positivo, un'azione pronta si arma nel turno dopo")),
				Attacker->SelectedAbilityIndex, Pronta))
		{
			DestroyPlanWorld(World);
			return false;
		}
		PC->SelectAbilityForCurrentForTest(INDEX_NONE);

		PC->SelectAbilityForCurrentForTest(ConRicarica);
		TestEqual(*E(TEXT("C: l'azione in ricarica NON si riarma dal tasto")),
			Attacker->SelectedAbilityIndex, static_cast<int32>(INDEX_NONE));
		// ⚠️ Il dock si misura da neutro: se il tasto l'avesse armata, un click sullo slot gia' armato la DISARMEREBBE e
		// la riga qui sotto sarebbe verde per il motivo sbagliato (trovato con la mutazione sulla guardia, #3833).
		PC->SelectAbilityForCurrentForTest(INDEX_NONE);
		PC->ArmKitAbility(ConRicarica);
		TestEqual(*E(TEXT("C: e NON si riarma dallo slot del dock")),
			Attacker->SelectedAbilityIndex, static_cast<int32>(INDEX_NONE));

		DestroyPlanWorld(World);
		return true;
	};

	// Le due squadre si provano SEMPRE entrambe: una premessa che salta per la 0 non deve nascondere la 1.
	const bool bSquadra0 = Prova(0);
	const bool bSquadra1 = Prova(1);
	return bSquadra0 && bSquadra1;
}

#endif // WITH_DEV_AUTOMATION_TESTS
