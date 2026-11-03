// #3973 - D-569: UNA STORDITA NON SI MUOVE. Ne' scatti ne' camminare, letto alla fase in cui l'azione risolve.
//
// ## Che cosa dice la decisione, e che cosa misura questo file
//
// D-416 (1) toglieva alla stordita l'azione principale e la reazione e scriveva che "non tocca il movimento", perche'
// quello e' `Root`. D-567 aveva esteso il divieto alla mobilita' che prende anche la principale. D-569 (2026-11-02 e
// 2026-11-03) ribalta il resto: una stordita non compie nessun movimento VOLONTARIO, e l'eccezione alla regola
// anti-stun-lock di D-072 e' dichiarata. I siti che muovono sono due, e ciascuno ha qui il suo banco:
//
//   - il DASH (`ARTTurnManager::ResolveDash`): ogni mobilita' rapida dei cataloghi, ricavata dai cataloghi e mai da un
//     elenco di nomi (`EveryCatalogMobilityIsRefusedWhenStunned`);
//   - il MOVE (`ARTTurnManager::BeginMovementResolution`): il camminare, anche col profilo Sprint
//     (`TheWalkIsRefusedWhenStunned`).
//
// Il MOMENTO e' parte della regola: lo stato si legge ALLA FASE in cui l'azione risolve. Uno stun nato nel Blast del
// turno T nega il Move di T (che risolve dopo) e NON il Dash di T (gia' risolto): `TheStatusIsReadAtTheResolvingPhase`.
// Il movimento FORZATO non e' volontario e non si tocca: `ForcedMovementIsNotDenied`.
//
// ## Ogni asserzione negativa ha il suo controllo positivo
//
// "Non si e' mosso" e "non ha danneggiato nessuno" sono veri anche per una fixture che non permette il movimento. Ogni
// riga gira percio' due bracci su mondi ricostruiti da capo, che differiscono per UNA riga (`ApplyStatus(Stunned)`): il
// braccio di controllo deve muoversi, pagare e (dove la mobilita' consegna danno) colpire; solo allora lo zero del braccio
// stordito dice qualcosa.
//
// ## Che cosa NON e' deciso qui, e resta ribaltabile (D-569 (5))
//
// Il rifiuto di una mobilita' paga la ricarica (D-563 (2), D-567); il click accetta ancora il piano di una stordita e lo
// nega la risoluzione (D-565 (2), #3134). La voce di rifiuto del CAMMINARE negato usa la stessa forma di quella del Dash
// (`Fallback`/`Cancelled`, motivo `Stunned`, SrcCell e TgtCell uguali alla cella dell'unita': la destinazione pianificata
// e' piano privato) ma in fase `Move`: e' una lettura dell'implementazione, non del testo che l'autore ha visto.
//
// Helper in un namespace NOMINATO: la unity build condivide la translation unit con gli altri file di test.

#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTEquipmentData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Combat/RTCombatLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Tests/RTAbilityFixtures.h"
#include "Tests/RTWorldFixtures.h"
#include "Turn/RTActionFallbackLibrary.h"
#include "Turn/RTActionQueueLibrary.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTResolvedEvent.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTStunnedMovementTestsInternal
{
	/** Ricarica forzata della mobilita' di prova: un addebito e il decremento del Cleanup lasciano `Cd - 1`, distinguibile da zero. */
	constexpr int32 TestCooldown = 3;

	/** Lo slot dove si monta una mobilita' core (come `Status.StunDeniesAMobilityThatAlsoCostsTheMainAction`). */
	constexpr int32 CoreSlot = 3;

	/** La scena sta SPOSTATA di un offset non nullo: una cella mai scritta (0,0) non deve poter passare per quella di partenza. */
	const FRTCellId Home(1, 0);

	ARTHexMapActor* SpawnMap(UWorld* World, int32 Radius)
	{
		URTHexMapAsset* Asset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = Asset;
		return Actor;
	}

	ARTUnit* SpawnUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell, URTHeroData* Hero = nullptr)
	{
		if (!World) { return nullptr; }
		ARTUnit* Unit = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!Unit) { return nullptr; }
		Unit->TeamId = TeamId;
		Unit->bIsBotControlled = false; // i piani li scriviamo noi
		Unit->ConfigureFromHeroData(Hero ? Hero : URTHeroCatalogLibrary::MakeIvrin());
		UGameplayStatics::FinishSpawningActor(Unit, FTransform::Identity);
		Unit->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		// (0,0) e' una cella VERA: senza questa riga ogni unita' senza piano pianifica un movimento verso l'origine.
		Unit->PlannedCell = Cell;
		Unit->Shield = 0;
		return Unit;
	}

	/** Azzera OGNI intento dell'unita': azione, scatto, percorso e destinazione. */
	void Stand(ARTUnit* Unit)
	{
		if (!Unit) { return; }
		Unit->PlannedAbilityIndex = INDEX_NONE;
		Unit->PlannedDashAbility = INDEX_NONE;
		Unit->PlannedPath.Reset();
		Unit->PlannedCell = Unit->Cell;
	}

	void RunTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	bool IsStunRefusal(const FRTTurnLogEntry& E, int32 UnitId, ERTMatchPhase Phase)
	{
		return E.Category == ERTLogCategory::Fallback && E.UnitId == UnitId && E.Phase == Phase
			&& E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Cancelled)
			&& E.Amount == static_cast<int32>(ERTActionInvalidReason::Stunned);
	}

	int32 CountActivations(const ARTTurnManager* TM, int32 UnitId)
	{
		int32 N = 0;
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::AbilityActivated && Ev.SourceStableUnitId == UnitId) { ++N; }
		}
		return N;
	}

	/** Quanti eventi `Move` di quella fase ha quell'unita' per sorgente: il fatto di timeline che il playback mostra. */
	int32 CountMoveEvents(const ARTTurnManager* TM, int32 UnitId, ERTMatchPhase Phase)
	{
		int32 N = 0;
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::Move && Ev.Phase == Phase && Ev.SourceStableUnitId == UnitId) { ++N; }
		}
		return N;
	}

	/** L'esito della voce `Move` di fase Move di questa unita', o `MAX_uint8` se non ce n'e'. */
	uint8 MoveOutcomeOf(const ARTTurnManager* TM, int32 UnitId)
	{
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Move && E.Phase == ERTMatchPhase::Move && E.UnitId == UnitId) { return E.Outcome; }
		}
		return MAX_uint8;
	}

	/**
	 * Lo stordente di prova: l'azione e' SINTETICA perche' nessuna azione di catalogo infligge lo stordimento (D-416, #3965).
	 *
	 * `AtCell` (se valida) mira una CELLA invece dell'unita': un colpo a unita' e' mirato dove l'unita' STA in pianificazione, e
	 * contro chi scatta nello stesso turno non trova nessuno (la causa "bersaglio scattato" di D-563: misurato qui, il TurnLog
	 * dello scattante non portava ne' il colpo ne' lo stordimento). Per stordire nel Blast chi ha gia' scattato nel Dash si mira
	 * la cella in cui arrivera'.
	 */
	void GiveStunAction(ARTUnit* Stunner, ARTUnit* Victim, const FRTCellId* AtCell = nullptr)
	{
		URTActionData* Azione = NewObject<URTActionData>(Stunner);
		FRTActionDef Def;
		Def.ActionId = TEXT("Test.Stun");
		Def.ResolutionPhase = ERTResolutionPhase::Control;
		Def.Priority = 25;
		Def.RangeCells = 3;
		Def.CooldownTurns = 0;
		Def.Fallback = ERTActionFallback::Cancel;
		Def.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Stunned, URTCombatLibrary::StunnedDurationTurns));
		Def.bCountsAsAttack = true; // controllo OSTILE: raggiunge il bersaglio come colpo
		Azione->Def = Def;
		Azione->RangeCells = Def.RangeCells;
		Azione->Power = 0;
		Stunner->Abilities.Add(Azione);
		Stunner->PlannedAbilityIndex = Stunner->Abilities.Num() - 1;
		if (AtCell)
		{
			Stunner->PlannedAttackTarget = nullptr;
			Stunner->PlannedAttackCell = *AtCell;
			Stunner->bAttackTargetsCell = true;
		}
		else
		{
			Stunner->PlannedAttackTarget = Victim;
		}
		Stunner->PlannedCell = Stunner->Cell;
	}

	// ---------------------------------------------------------------------------------------------
	// IL DASH: ogni mobilita' dei cataloghi
	// ---------------------------------------------------------------------------------------------

	enum class EProvenance : uint8
	{
		Kit,        //< nel kit di un eroe del roster
		Core,       //< nel catalogo core, o generica: si monta con `AddCoreAbilityInSlot`
		Equipment,  //< concessa da un pezzo di equipaggiamento: nessuna oggi, e il test lo dice se compare
	};

	struct FMobility
	{
		EProvenance Provenance = EProvenance::Core;
		int32 HeroIndex = INDEX_NONE;  //< solo Kit: l'eroe del roster che la porta
		FString Origin;
		FRTActionDef Def;
	};

	/** Le mobilita' rapide dei cataloghi: kit d'eroe, core, generiche ed equipaggiamento. Nessun `ActionId` e' scritto qui. */
	TArray<FMobility> CollectMobilities()
	{
		TArray<FMobility> Out;
		TSet<FName> Seen;

		const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
		for (int32 H = 0; H < Roster.Num(); ++H)
		{
			if (!Roster[H]) { continue; }
			for (const TObjectPtr<URTActionData>& Action : Roster[H]->Actions)
			{
				if (Action && URTCatalogLibrary::IsFastMovement(Action->Def) && !Seen.Contains(Action->Def.ActionId))
				{
					FMobility M;
					M.Provenance = EProvenance::Kit;
					M.HeroIndex = H;
					M.Origin = FString::Printf(TEXT("kit %s"), *Roster[H]->HeroId.ToString());
					M.Def = Action->Def;
					Seen.Add(Action->Def.ActionId);
					Out.Add(M);
				}
			}
		}

		for (const FRTActionDef& Core : URTCatalogLibrary::GetCoreActionCatalog())
		{
			if (URTCatalogLibrary::IsFastMovement(Core) && !Seen.Contains(Core.ActionId))
			{
				FMobility M;
				M.Provenance = EProvenance::Core;
				M.Origin = TEXT("core");
				M.Def = Core;
				Seen.Add(Core.ActionId);
				Out.Add(M);
			}
		}

		for (const URTActionData* Generic : URTCatalogLibrary::MakeGenericActions(GetTransientPackage()))
		{
			if (Generic && URTCatalogLibrary::IsFastMovement(Generic->Def) && !Seen.Contains(Generic->Def.ActionId))
			{
				FMobility M;
				M.Provenance = EProvenance::Core;
				M.Origin = TEXT("generica");
				M.Def = Generic->Def;
				Seen.Add(Generic->Def.ActionId);
				Out.Add(M);
			}
		}

		auto AddEquipment = [&Out, &Seen](const TArray<URTEquipmentData*>& Items)
		{
			for (const URTEquipmentData* Item : Items)
			{
				const URTActionData* Granted = Item ? URTCatalogLibrary::MakeEquipmentAction(Item, GetTransientPackage()) : nullptr;
				if (Granted && URTCatalogLibrary::IsFastMovement(Granted->Def) && !Seen.Contains(Granted->Def.ActionId))
				{
					FMobility M;
					M.Provenance = EProvenance::Equipment;
					M.Origin = TEXT("equipaggiamento");
					M.Def = Granted->Def;
					Seen.Add(Granted->Def.ActionId);
					Out.Add(M);
				}
			}
		};
		AddEquipment(URTCatalogLibrary::MakeGadgets());
		AddEquipment(URTCatalogLibrary::MakeReactionModules());
		return Out;
	}

	/** Dove si mettono il nemico-testimone e la meta, secondo come la mobilita' tocca gli altri. */
	enum class EScene : uint8
	{
		Plain,   //< nessuno sulla traiettoria: meta a due celle, il nemico sta lontano
		Charge,  //< il nemico sta sulla meta (si ferma ADDOSSO e lo colpisce)
		Pass,    //< il nemico sta a una cella, la meta oltre di lui (lo attraversa e lo colpisce)
		Unknown, //< uno stile che questo test non sa montare: cade e dice di dichiararlo
	};

	EScene SceneFor(const FRTActionDef& Def)
	{
		switch (Def.MovementStyle)
		{
		case ERTMovementStyle::Budget:
		case ERTMovementStyle::LinearDash:
		case ERTMovementStyle::LinearLeap:   return EScene::Plain;
		case ERTMovementStyle::LinearCharge: return EScene::Charge;
		case ERTMovementStyle::LinearPass:   return EScene::Pass;
		default:                             return EScene::Unknown;
		}
	}

	struct FDashOutcome
	{
		bool bValid = false;
		int32 StableId = 0;
		bool bAppliesBefore = false;  //< `PlannedDashApplies()` prima del turno: per la regola del catalogo lo scatto parte
		bool bMovesBefore = false;    //< `PlannedDashMoves()` prima del turno: la domanda di anteprima, mira e verso
		FRTCellId Arrival;
		int32 Refusals = 0;           //< voci Fallback/Cancelled/Stunned di fase Dash che nominano l'azione
		int32 RefusalsStill = 0;      //< fra quelle, SrcCell e TgtCell sono la cella di partenza
		int32 Activations = 0;
		int32 CooldownAfter = -1;
		int32 FoeHealthLost = 0;
		bool bFoeMoved = false;
	};

	FDashOutcome RunDashTurn(FAutomationTestBase& Test, const FMobility& M, bool bStunned, const FString& Label)
	{
		FDashOutcome Out;
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: world di prova"), *Label), World)) { return Out; }
		SpawnMap(World, /*Radius=*/ 8);

		const EScene Scene = SceneFor(M.Def);
		const FRTCellId Step1(Home.X + 1, Home.Y);
		const FRTCellId Step2(Home.X + 2, Home.Y);
		const FRTCellId Far(-6, 0);

		URTHeroData* Hero = M.Provenance == EProvenance::Kit ? URTHeroCatalogLibrary::GetHeroRoster()[M.HeroIndex] : nullptr;
		ARTUnit* Runner = SpawnUnit(World, 0, Home, Hero);
		const FRTCellId FoeCell = Scene == EScene::Charge ? Step2 : (Scene == EScene::Pass ? Step1 : Far);
		ARTUnit* Foe = SpawnUnit(World, 1, FoeCell);
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Runner || !Foe || !TM) { RTWorldFixtures::DestroyWorld(World); return Out; }
		Stand(Foe);
		Stand(Runner);

		int32 Slot = INDEX_NONE;
		if (M.Provenance == EProvenance::Kit)
		{
			for (int32 I = 0; I < Runner->Abilities.Num(); ++I)
			{
				if (Runner->Abilities[I] && Runner->Abilities[I]->Def.ActionId == M.Def.ActionId) { Slot = I; break; }
			}
		}
		else
		{
			Slot = RTAbilityFixtures::AddCoreAbilityInSlot(Runner, *M.Def.ActionId.ToString(), CoreSlot);
		}
		if (!Test.TestTrue(*FString::Printf(TEXT("%s: premessa: l'azione e' montata"), *Label), Slot != INDEX_NONE))
		{
			RTWorldFixtures::DestroyWorld(World);
			return Out;
		}
		URTActionData* Azione = Runner->GetAbility(Slot);
		Azione->Def.CooldownTurns = TestCooldown;
		Azione->CooldownTurns = TestCooldown;

		if (bStunned) { Runner->ApplyStatus(TAG_Status_Stunned, URTCombatLibrary::StunnedDurationTurns); }
		Runner->PlannedDashAbility = Slot;
		// La meta e' sempre a due celle: per la carica e' la cella del nemico (bersaglio), per il passaggio e' oltre di lui.
		Runner->PlannedDashCell = Step2;
		Out.bAppliesBefore = Runner->PlannedDashApplies();
		Out.bMovesBefore = Runner->PlannedDashMoves();
		Test.TestEqual(*FString::Printf(TEXT("%s: premessa: la ricarica prima del turno e' zero"), *Label), Runner->GetAbilityCooldown(Slot), 0);
		Test.TestEqual(*FString::Printf(TEXT("%s: premessa: lo stordimento c'e' solo dove il braccio lo dichiara"), *Label),
			Runner->HasStatus(TAG_Status_Stunned), bStunned);
		const int32 FoeHealthBefore = Foe->Health;
		const FRTCellId FoeBefore = Foe->Cell;

		RunTurn(TM);

		Out.StableId = Runner->StableUnitId;
		Out.Arrival = Runner->Cell;
		Out.CooldownAfter = Runner->GetAbilityCooldown(Slot);
		Out.Activations = CountActivations(TM, Runner->StableUnitId);
		Out.FoeHealthLost = FoeHealthBefore - Foe->Health;
		Out.bFoeMoved = !(Foe->Cell == FoeBefore);
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (IsStunRefusal(E, Runner->StableUnitId, ERTMatchPhase::Dash) && E.ActionId == M.Def.ActionId)
			{
				++Out.Refusals;
				if (E.SrcCell == Home && E.TgtCell == Home) { ++Out.RefusalsStill; }
			}
		}
		Out.bValid = true;
		RTWorldFixtures::DestroyWorld(World);
		return Out;
	}

	// ---------------------------------------------------------------------------------------------
	// IL MOVE: il camminare, e lo Sprint che e' un profilo del camminare
	// ---------------------------------------------------------------------------------------------

	enum class EWalkForm : uint8
	{
		OneStep,          //< il percorso scritto dal click: {partenza, cella adiacente}
		DestinationOnly,  //< la forma dei bot: nessun percorso, la sola destinazione a due celle
		Sprint,           //< oltre il passo del Move: la banda legge `Sprint`
		OccupiedTarget,   //< la destinazione dichiarata e' occupata da un'altra unita'
	};

	const TCHAR* WalkFormName(EWalkForm Form)
	{
		switch (Form)
		{
		case EWalkForm::OneStep:         return TEXT("un passo");
		case EWalkForm::DestinationOnly: return TEXT("sola destinazione");
		case EWalkForm::Sprint:          return TEXT("Sprint");
		case EWalkForm::OccupiedTarget:  return TEXT("destinazione occupata");
		}
		return TEXT("?");
	}

	struct FWalkOutcome
	{
		bool bValid = false;
		int32 StableId = 0;
		FRTCellId Planned;           //< dove il piano porta l'unita'
		FRTCellId Arrival;
		FRTCellId MateArrival;       //< dove arriva il compagno non stordito, ordinato DOPO
		FRTCellId MatePlanned;
		bool bMateAfter = false;     //< premessa: nell'ordine reale di raccolta il compagno viene dopo
		int32 Refusals = 0;          //< voci Fallback/Cancelled/Stunned di fase Move di chi cammina
		int32 RefusalsStill = 0;     //< fra quelle, SrcCell e TgtCell sono la cella di partenza
		int32 RefusalsAnyPhase = 0;  //< voci Fallback/Cancelled/Stunned dell'unita', di qualunque fase
		int32 MoveEvents = 0;        //< eventi `Move` di fase Move di chi cammina
		uint8 MoveOutcome = MAX_uint8;
		FRTCellId MoveEntryTarget;   //< TgtCell della voce `Move` di chi cammina
		bool bExposed = false;       //< `Status.Exposed` a fine turno (il prezzo dello Sprint)
		bool bPlanHadMove = false;   //< premessa: `HasPlannedNormalMove()` al commit
	};

	FWalkOutcome RunWalkTurn(FAutomationTestBase& Test, EWalkForm Form, bool bStunned, const FString& Label)
	{
		FWalkOutcome Out;
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: world di prova"), *Label), World)) { return Out; }
		SpawnMap(World, /*Radius=*/ 10);

		ARTUnit* Walker = SpawnUnit(World, 0, Home);
		// Il compagno sta DOPO chi cammina nell'ordine di raccolta (stessa X, Y maggiore): se la guardia uscisse dal ciclo o
		// sfasasse gli array paralleli, e' lui a non arrivare.
		const FRTCellId MateHome(Home.X, Home.Y + 2);
		ARTUnit* Mate = SpawnUnit(World, 0, MateHome);
		ARTUnit* Foe = SpawnUnit(World, 1, FRTCellId(-8, 0));
		ARTUnit* Blocker = Form == EWalkForm::OccupiedTarget ? SpawnUnit(World, 1, FRTCellId(Home.X + 1, Home.Y)) : nullptr;
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Walker || !Mate || !Foe || !TM || (Form == EWalkForm::OccupiedTarget && !Blocker))
		{
			RTWorldFixtures::DestroyWorld(World);
			return Out;
		}
		Stand(Foe);
		if (Blocker) { Stand(Blocker); }
		Stand(Walker);
		Stand(Mate);

		switch (Form)
		{
		case EWalkForm::OneStep:
			Walker->PlannedPath = { Home, FRTCellId(Home.X + 1, Home.Y) };
			Walker->PlannedCell = FRTCellId(Home.X + 1, Home.Y);
			break;
		case EWalkForm::DestinationOnly:
			Walker->PlannedCell = FRTCellId(Home.X + 2, Home.Y);
			break;
		case EWalkForm::OccupiedTarget:
			Walker->PlannedPath = { Home, FRTCellId(Home.X + 1, Home.Y) };
			Walker->PlannedCell = FRTCellId(Home.X + 1, Home.Y);
			break;
		case EWalkForm::Sprint:
		{
			// Un passo OLTRE il Move neutro: la banda si legge dalla distanza pianificata (D-425).
			const int32 Oltre = Walker->GetEffectiveMoveRange() + 1;
			Walker->PlannedPath.Reset();
			for (int32 Q = 0; Q <= Oltre; ++Q) { Walker->PlannedPath.Add(FRTCellId(Home.X + Q, Home.Y)); }
			Walker->PlannedCell = Walker->PlannedPath.Last();
			break;
		}
		}
		Out.Planned = Walker->PlannedCell;
		Out.bPlanHadMove = Walker->HasPlannedNormalMove();

		// Il compagno cammina di due celle, sempre.
		Out.MatePlanned = FRTCellId(MateHome.X + 2, MateHome.Y);
		Mate->PlannedCell = Out.MatePlanned;

		TArray<ARTUnit*> Ordine = { Mate, Walker, Foe };
		URTActionQueueLibrary::SortUnitsForResolution(Ordine);
		Out.bMateAfter = Ordine.IndexOfByKey(Mate) > Ordine.IndexOfByKey(Walker);

		if (bStunned) { Walker->ApplyStatus(TAG_Status_Stunned, URTCombatLibrary::StunnedDurationTurns); }
		Test.TestEqual(*FString::Printf(TEXT("%s: premessa: lo stordimento c'e' solo dove il braccio lo dichiara"), *Label),
			Walker->HasStatus(TAG_Status_Stunned), bStunned);
		Test.TestFalse(*FString::Printf(TEXT("%s: premessa: chi cammina non e' radicato (Root e Stunned sono due stati)"), *Label),
			Walker->HasStatus(TAG_Status_Root));

		RunTurn(TM);

		Out.StableId = Walker->StableUnitId;
		Out.Arrival = Walker->Cell;
		Out.MateArrival = Mate->Cell;
		Out.MoveEvents = CountMoveEvents(TM, Walker->StableUnitId, ERTMatchPhase::Move);
		Out.MoveOutcome = MoveOutcomeOf(TM, Walker->StableUnitId);
		Out.bExposed = Walker->HasStatus(TAG_Status_Exposed);
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Move && E.Phase == ERTMatchPhase::Move && E.UnitId == Walker->StableUnitId)
			{
				Out.MoveEntryTarget = E.TgtCell;
			}
			if (E.Category == ERTLogCategory::Fallback && E.UnitId == Walker->StableUnitId
				&& E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Cancelled)
				&& E.Amount == static_cast<int32>(ERTActionInvalidReason::Stunned))
			{
				++Out.RefusalsAnyPhase;
			}
			if (IsStunRefusal(E, Walker->StableUnitId, ERTMatchPhase::Move))
			{
				++Out.Refusals;
				if (E.SrcCell == Home && E.TgtCell == Home) { ++Out.RefusalsStill; }
			}
		}
		Out.bValid = true;
		RTWorldFixtures::DestroyWorld(World);
		return Out;
	}
}

// =====================================================================================================
// 1. IL DASH: ogni mobilita' dei cataloghi
// =====================================================================================================

/**
 * Ogni mobilita' rapida dei cataloghi e' NEGATA a una stordita, e il rifiuto paga (D-569 (1), D-563 (2), D-567).
 *
 * L'insieme viene dai cataloghi (kit d'eroe, core, generiche, equipaggiamento) filtrati con `IsFastMovement`: nessun
 * `ActionId` e' scritto qui, e una mobilita' nuova cade in questo test senza che nessuno lo ricordi. Per ognuna due
 * bracci su mondi identici fuori dallo stordimento:
 *
 *  - CONTROLLO (non stordita): la mobilita' si applica e sposta, arriva, attiva, paga e - dove consegna danno (la carica
 *    e il passaggio) - colpisce il nemico-testimone;
 *  - CUORE (stordita): la regola del catalogo dice ancora che lo scatto si applica, ma lo stordimento lo nega, e allora
 *    non sposta (anteprima e mira), resta nella sua cella, lascia UNA voce Fallback/Cancelled/Stunned in fase Dash che
 *    nomina l'azione con le due celle uguali a quella di partenza (la meta e' piano privato), non attiva, PAGA, e il
 *    testimone non perde un punto ne' si sposta (gli impatti della carica non nascono).
 *
 * Mutazioni suggerite: rimettere `TakesMainSlot` nella condizione di `ARTUnit::PlannedDashDeniedByStun` deve far cadere
 * ogni riga (lo slot di queste mobilita' e' `Movement`); togliere `ConsumeAbility` dal ramo del rifiuto in `ResolveDash`
 * deve far cadere il pagamento.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStunnedEveryCatalogMobilityIsRefusedTest,
	"RefactorTactics.Status.StunCensus.EveryMobilityIsRefusedWhenStunned",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStunnedEveryCatalogMobilityIsRefusedTest::RunTest(const FString&)
{
	namespace T = RTStunnedMovementTestsInternal;

	const TArray<T::FMobility> Mobilities = T::CollectMobilities();
	// Anti-vacuita': un filtro che riconosce zero elementi assolve.
	if (!TestTrue(TEXT("i cataloghi offrono mobilita' rapide da censire"), Mobilities.Num() > 0)) { return false; }
	// Ogni strada che porta una mobilita' a un'unita' di serie deve contribuirne almeno una, o e' cieca.
	TestTrue(TEXT("almeno una mobilita' arriva dal catalogo core"),
		Mobilities.ContainsByPredicate([](const T::FMobility& M) { return M.Provenance == T::EProvenance::Core; }));
	TestTrue(TEXT("almeno una mobilita' arriva da un kit d'eroe"),
		Mobilities.ContainsByPredicate([](const T::FMobility& M) { return M.Provenance == T::EProvenance::Kit; }));
	// Il fatto che la carica e il passaggio consegnino danno e' cio' che separa questo test da uno che vede solo lo
	// spostamento: se il catalogo li perdesse, la clausola "nessun danno" sarebbe vuota.
	TestTrue(TEXT("almeno una mobilita' e' una carica (impatto a contatto)"),
		Mobilities.ContainsByPredicate([](const T::FMobility& M) { return M.Def.MovementStyle == ERTMovementStyle::LinearCharge; }));
	TestTrue(TEXT("almeno una mobilita' e' un passaggio (colpisce chi attraversa)"),
		Mobilities.ContainsByPredicate([](const T::FMobility& M) { return M.Def.MovementStyle == ERTMovementStyle::LinearPass; }));

	for (const T::FMobility& M : Mobilities)
	{
		const FString Label = FString::Printf(TEXT("%s (%s)"), *M.Def.ActionId.ToString(), *M.Origin);
		const T::EScene Scene = T::SceneFor(M.Def);
		if (Scene == T::EScene::Unknown || M.Provenance == T::EProvenance::Equipment)
		{
			AddError(FString::Printf(
				TEXT("%s: mobilita' rapida che questo test non sa montare (stile %d, strada %d). D-569 la nega a una stordita come tutte "
					 "le altre: dichiara qui la sua scena (dove stanno il nemico e la meta) e la sua riga di montaggio."),
				*Label, static_cast<int32>(M.Def.MovementStyle), static_cast<int32>(M.Provenance)));
			continue;
		}
		const bool bDeliversDamage = URTCatalogLibrary::FirstDamage(M.Def) > 0;
		const FRTCellId Step1(T::Home.X + 1, T::Home.Y);
		const FRTCellId Step2(T::Home.X + 2, T::Home.Y);
		const FRTCellId Expected = Scene == T::EScene::Charge ? Step1 : Step2; // la carica si ferma ADDOSSO al nemico
		const int32 Paid = T::TestCooldown - 1;                                  // un addebito e il decremento del Cleanup

		// CONTROLLO - non stordita.
		const T::FDashOutcome Free = T::RunDashTurn(*this, M, /*bStunned=*/ false, Label + TEXT(", controllo"));
		if (!TestTrue(*FString::Printf(TEXT("%s: controllo: il turno e' girato"), *Label), Free.bValid)) { continue; }
		TestTrue(*FString::Printf(TEXT("%s: controllo: lo scatto si applica"), *Label), Free.bAppliesBefore);
		TestTrue(*FString::Printf(TEXT("%s: controllo: e sposta (anteprima e mira)"), *Label), Free.bMovesBefore);
		TestEqual(*FString::Printf(TEXT("%s: controllo: arriva"), *Label), Free.Arrival.ToString(), Expected.ToString());
		TestEqual(*FString::Printf(TEXT("%s: controllo: nessun rifiuto"), *Label), Free.Refusals, 0);
		TestTrue(*FString::Printf(TEXT("%s: controllo: attiva l'azione"), *Label), Free.Activations >= 1);
		TestEqual(*FString::Printf(TEXT("%s: controllo: la ricarica e' pagata"), *Label), Free.CooldownAfter, Paid);
		if (bDeliversDamage)
		{
			TestTrue(*FString::Printf(TEXT("%s: controllo: il testimone viene colpito"), *Label), Free.FoeHealthLost > 0);
		}

		// CUORE - stordita.
		const T::FDashOutcome Stunned = T::RunDashTurn(*this, M, /*bStunned=*/ true, Label + TEXT(", stordita"));
		if (!TestTrue(*FString::Printf(TEXT("%s: stordita: il turno e' girato"), *Label), Stunned.bValid)) { continue; }
		TestTrue(*FString::Printf(TEXT("%s: premessa: per la regola del catalogo lo scatto si applica ancora"), *Label), Stunned.bAppliesBefore);
		TestFalse(*FString::Printf(TEXT("%s: ma lo stordimento lo nega: non sposta (anteprima e mira)"), *Label), Stunned.bMovesBefore);
		TestEqual(*FString::Printf(TEXT("%s: la stordita resta dov'e'"), *Label), Stunned.Arrival.ToString(), T::Home.ToString());
		TestEqual(*FString::Printf(TEXT("%s: il TurnLog dice perche': UNA voce Fallback/Cancelled/Stunned in fase Dash"), *Label),
			Stunned.Refusals, 1);
		TestEqual(*FString::Printf(TEXT("%s: la voce non rivela la meta: le sue due celle sono quella di partenza"), *Label),
			Stunned.RefusalsStill, 1);
		TestEqual(*FString::Printf(TEXT("%s: l'azione non si attiva"), *Label), Stunned.Activations, 0);
		TestEqual(*FString::Printf(TEXT("%s: il rifiuto PAGA la ricarica"), *Label), Stunned.CooldownAfter, Paid);
		TestEqual(*FString::Printf(TEXT("%s: nessun danno al testimone"), *Label), Stunned.FoeHealthLost, 0);
		TestFalse(*FString::Printf(TEXT("%s: e il testimone non e' spinto"), *Label), Stunned.bFoeMoved);
	}
	return true;
}

// =====================================================================================================
// 2. IL MOVE: il camminare
// =====================================================================================================

/**
 * Una stordita non CAMMINA, e nemmeno con lo Sprint (D-569 (1)); il camminare ha ricarica zero, lo Sprint paga `Exposed`
 * (D-116 (3): "si paga l'impegno dichiarato").
 *
 * Quattro forme del piano, ognuna con il suo braccio di controllo (la stessa unita', lo stesso piano, senza stordimento):
 * il percorso a un passo che scrive il click, la sola destinazione che dichiarano i bot, lo Sprint (un passo oltre il Move
 * neutro) e la destinazione occupata da un'altra unita'. Per la stordita:
 *
 *  - resta nella sua cella e nessun evento `Move` di fase Move la nomina;
 *  - UNA voce Fallback/Cancelled/Stunned in fase Move con SrcCell e TgtCell uguali alla cella dell'unita': la destinazione
 *    pianificata e' piano privato (D-567). E' una lettura, non il testo dell'autore: ribaltabile;
 *  - la voce `Move` dice `Stayed` e non `BlockedByUnit` anche quando la destinazione e' occupata: la stordita non ha
 *    camminato, quindi non ha urtato nessuno ne' ne ha rivelato la posizione;
 *  - lo Sprint paga `Exposed` anche da ferma; il camminare no;
 *  - un compagno non stordito, ordinato DOPO di lei, arriva comunque (gli array paralleli di `BeginMovementResolution`
 *    restano allineati: nessun `continue` che salti `Ctx.Paths`).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStunnedTheWalkIsRefusedTest,
	"RefactorTactics.Status.StunnedMovement.TheWalkIsRefusedWhenStunned",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStunnedTheWalkIsRefusedTest::RunTest(const FString&)
{
	namespace T = RTStunnedMovementTestsInternal;

	const T::EWalkForm Forms[] = { T::EWalkForm::OneStep, T::EWalkForm::DestinationOnly, T::EWalkForm::Sprint, T::EWalkForm::OccupiedTarget };
	for (const T::EWalkForm Form : Forms)
	{
		const FString Name = T::WalkFormName(Form);
		const bool bSprint = Form == T::EWalkForm::Sprint;
		const bool bOccupied = Form == T::EWalkForm::OccupiedTarget;

		// CONTROLLO - non stordita: cammina (o, con la destinazione occupata, e' negata dal piano come sempre).
		const T::FWalkOutcome Free = T::RunWalkTurn(*this, Form, /*bStunned=*/ false, Name + TEXT(", controllo"));
		if (!TestTrue(*FString::Printf(TEXT("%s: controllo: il turno e' girato"), *Name), Free.bValid)) { return false; }
		TestTrue(*FString::Printf(TEXT("%s: premessa: il piano ha un movimento normale"), *Name), Free.bPlanHadMove);
		TestEqual(*FString::Printf(TEXT("%s: controllo: nessun rifiuto per stordimento"), *Name), Free.RefusalsAnyPhase, 0);
		if (bOccupied)
		{
			TestEqual(*FString::Printf(TEXT("%s: controllo: la destinazione occupata la nega il piano (BlockedByUnit)"), *Name),
				static_cast<int32>(Free.MoveOutcome), static_cast<int32>(ERTMoveOutcome::BlockedByUnit));
			TestEqual(*FString::Printf(TEXT("%s: controllo: e non si e' mossa"), *Name), Free.Arrival.ToString(), T::Home.ToString());
		}
		else
		{
			TestEqual(*FString::Printf(TEXT("%s: controllo: cammina fino alla destinazione"), *Name), Free.Arrival.ToString(), Free.Planned.ToString());
			TestTrue(*FString::Printf(TEXT("%s: controllo: e il playback ha un evento Move"), *Name), Free.MoveEvents >= 1);
		}
		TestEqual(*FString::Printf(TEXT("%s: controllo: il compagno arriva"), *Name), Free.MateArrival.ToString(), Free.MatePlanned.ToString());
		TestEqual(*FString::Printf(TEXT("%s: controllo: Exposed c'e' se e solo se e' uno Sprint"), *Name), Free.bExposed, bSprint && !bOccupied);

		// CUORE - stordita.
		const T::FWalkOutcome Stunned = T::RunWalkTurn(*this, Form, /*bStunned=*/ true, Name + TEXT(", stordita"));
		if (!TestTrue(*FString::Printf(TEXT("%s: stordita: il turno e' girato"), *Name), Stunned.bValid)) { return false; }
		TestTrue(*FString::Printf(TEXT("%s: premessa: il piano ha un movimento normale anche da stordita (il click lo accetta)"), *Name),
			Stunned.bPlanHadMove);
		TestEqual(*FString::Printf(TEXT("%s: la stordita resta dov'e'"), *Name), Stunned.Arrival.ToString(), T::Home.ToString());
		TestEqual(*FString::Printf(TEXT("%s: nessun evento Move di fase Move la nomina"), *Name), Stunned.MoveEvents, 0);
		TestEqual(*FString::Printf(TEXT("%s: UNA voce Fallback/Cancelled/Stunned in fase Move"), *Name), Stunned.Refusals, 1);
		TestEqual(*FString::Printf(TEXT("%s: la voce non rivela la destinazione: le sue due celle sono quella di partenza"), *Name),
			Stunned.RefusalsStill, 1);
		TestEqual(*FString::Printf(TEXT("%s: e non ce n'e' una in altra fase"), *Name), Stunned.RefusalsAnyPhase, 1);
		TestEqual(*FString::Printf(TEXT("%s: la voce Move dice Stayed, non BlockedByUnit"), *Name),
			static_cast<int32>(Stunned.MoveOutcome), static_cast<int32>(ERTMoveOutcome::Stayed));
		TestEqual(*FString::Printf(TEXT("%s: e porta la cella di partenza come destinazione"), *Name),
			Stunned.MoveEntryTarget.ToString(), T::Home.ToString());
		TestEqual(*FString::Printf(TEXT("%s: lo Sprint paga Exposed anche da ferma, il camminare no"), *Name), Stunned.bExposed, bSprint);
		TestTrue(*FString::Printf(TEXT("%s: premessa: nell'ordine reale di raccolta il compagno viene DOPO chi cammina"), *Name), Stunned.bMateAfter);
		TestEqual(*FString::Printf(TEXT("%s: il compagno non stordito arriva comunque"), *Name),
			Stunned.MateArrival.ToString(), Stunned.MatePlanned.ToString());
	}
	return true;
}

// =====================================================================================================
// 3. IL MOMENTO: lo stato si legge alla fase in cui l'azione risolve
// =====================================================================================================

/**
 * Uno stun nato nel Blast del turno T nega il Move di T e NON il Dash di T; vale per tutto T+1; scade dopo (D-569 (4)).
 *
 * "Se l'azione avviene prima dello stun allora si'" (risposta d'autore del 2026-11-02): il Dash risolve prima del Blast,
 * il Move dopo. Due vittime della stessa squadra, ciascuna con il suo stordente (un'azione sintetica di fase Control: nessun
 * catalogo infligge lo stordimento): una pianifica uno scatto (`Action.Dodge`), l'altra un passo.
 *
 *  - T:   lo scatto arriva (e la vittima esce stordita dal Blast), il passo e' negato;
 *  - T+1: lo stordimento dura due turni, quindi sono negati ENTRAMBI, anche lo scatto che a T era riuscito;
 *  - T+2: scaduto nel Cleanup di T+1, entrambi si muovono di nuovo.
 *
 * Il controllo e' lo stesso turno T senza stordenti: entrambe arrivano. Senza di lui "il passo e' negato" sarebbe vero per
 * una fixture che non permette di camminare. Il precedente e' `Root` (`ARTUnit::GetEffectiveMoveRange` lo legge dal
 * vivo): `TheMoveIsDeniedByARootBornInTheBlastToo` lo misura, perche' la tabella di #1479 non era stata rimisurata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStunnedTheStatusIsReadAtThePhaseTest,
	"RefactorTactics.Status.StunnedMovement.TheStatusIsReadAtTheResolvingPhase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStunnedTheStatusIsReadAtThePhaseTest::RunTest(const FString&)
{
	namespace T = RTStunnedMovementTestsInternal;

	struct FRig
	{
		UWorld* World = nullptr;
		ARTTurnManager* TM = nullptr;
		ARTUnit* Dasher = nullptr;
		ARTUnit* Walker = nullptr;
		ARTUnit* StunnerOfDasher = nullptr;
		ARTUnit* StunnerOfWalker = nullptr;
		int32 DodgeSlot = INDEX_NONE;
	};
	const FRTCellId DasherHome(0, 0), DasherT1(1, 0), DasherT2(2, 0), DasherT3(3, 0);
	const FRTCellId WalkerHome(-3, 3), WalkerT1(-1, 3), WalkerT2(0, 3), WalkerT3(1, 3);

	auto Open = [&](FRig& R, bool bWithStunners) -> bool
	{
		R.World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("world di prova"), R.World)) { return false; }
		T::SpawnMap(R.World, /*Radius=*/ 8);
		R.Dasher = T::SpawnUnit(R.World, 0, DasherHome);
		R.Walker = T::SpawnUnit(R.World, 0, WalkerHome);
		// Gli stordenti stanno DIETRO la vittima, sulla sua riga e a due celle da dove si trova al Blast: lo scatto e' gia' avvenuto
		// (il Dash risolve prima), il passo no (il Move risolve dopo), come nel banco di `Status.StunSilencesAnArmedWatcher`.
		R.StunnerOfDasher = T::SpawnUnit(R.World, 1, FRTCellId(-1, 0));
		R.StunnerOfWalker = T::SpawnUnit(R.World, 1, FRTCellId(-5, 3));
		R.TM = R.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!R.Dasher || !R.Walker || !R.StunnerOfDasher || !R.StunnerOfWalker || !R.TM)
		{
			RTWorldFixtures::DestroyWorld(R.World);
			return false;
		}
		T::Stand(R.Dasher);
		T::Stand(R.Walker);
		T::Stand(R.StunnerOfDasher);
		T::Stand(R.StunnerOfWalker);
		R.DodgeSlot = RTAbilityFixtures::AddCoreAbilityInSlot(R.Dasher, TEXT("Action.Dodge"), T::CoreSlot);
		if (!TestTrue(TEXT("premessa: Action.Dodge e' montata"), R.DodgeSlot != INDEX_NONE))
		{
			RTWorldFixtures::DestroyWorld(R.World);
			return false;
		}
		R.Dasher->PlannedDashAbility = R.DodgeSlot;
		R.Dasher->PlannedDashCell = DasherT1;
		R.Walker->PlannedCell = WalkerT1;
		if (bWithStunners)
		{
			// Contro chi scatta si mira la cella d'arrivo; contro chi cammina (fermo fino al Move) basta l'unita'.
			T::GiveStunAction(R.StunnerOfDasher, R.Dasher, &DasherT1);
			T::GiveStunAction(R.StunnerOfWalker, R.Walker);
		}
		return true;
	};
	auto Close = [](FRig& R) { RTWorldFixtures::DestroyWorld(R.World); R.World = nullptr; };

	// CONTROLLO - lo stesso turno T senza stordenti: entrambe arrivano.
	{
		FRig R;
		if (!Open(R, /*bWithStunners=*/ false)) { return false; }
		T::RunTurn(R.TM);
		TestEqual(TEXT("controllo: senza stordenti lo scatto arriva"), R.Dasher->Cell.ToString(), DasherT1.ToString());
		TestEqual(TEXT("controllo: senza stordenti il passo arriva"), R.Walker->Cell.ToString(), WalkerT1.ToString());
		TestFalse(TEXT("controllo: nessuno e' stordito"), R.Dasher->HasStatus(TAG_Status_Stunned) || R.Walker->HasStatus(TAG_Status_Stunned));
		Close(R);
	}

	FRig R;
	if (!Open(R, /*bWithStunners=*/ true)) { return false; }

	// TURNO T - lo stun nasce nel Blast.
	T::RunTurn(R.TM);
	const bool bDasherStunned = R.Dasher->HasStatus(TAG_Status_Stunned);
	const bool bWalkerStunned = R.Walker->HasStatus(TAG_Status_Stunned);
	TestTrue(TEXT("premessa: lo scatto e' stato compiuto PRIMA dello stun: e' lo stordente a colpire chi e' gia' arrivato"), bDasherStunned);
	TestTrue(TEXT("premessa: anche chi doveva camminare e' stato stordito nel Blast"), bWalkerStunned);
	TestEqual(TEXT("T: lo scatto risolve prima del Blast e si compie"), R.Dasher->Cell.ToString(), DasherT1.ToString());
	TestEqual(TEXT("T: il passo risolve dopo il Blast ed e' negato"), R.Walker->Cell.ToString(), WalkerHome.ToString());
	TestEqual(TEXT("T: la voce del passo negato e' in fase Move"),
		[&]() { int32 N = 0; for (const FRTTurnLogEntry& E : R.TM->GetTurnLog()) { if (T::IsStunRefusal(E, R.Walker->StableUnitId, ERTMatchPhase::Move)) { ++N; } } return N; }(), 1);
	TestEqual(TEXT("T: e nessuna voce di rifiuto per lo scatto, che si e' compiuto"),
		[&]() { int32 N = 0; for (const FRTTurnLogEntry& E : R.TM->GetTurnLog()) { if (T::IsStunRefusal(E, R.Dasher->StableUnitId, ERTMatchPhase::Dash)) { ++N; } } return N; }(), 0);

	// TURNO T+1 - lo stordimento dura due turni: negati entrambi. Nessuno stordente: non rinnovano lo stato.
	T::Stand(R.StunnerOfDasher);
	T::Stand(R.StunnerOfWalker);
	R.Dasher->PlannedDashAbility = R.DodgeSlot;
	R.Dasher->PlannedDashCell = DasherT2;
	R.Walker->PlannedCell = WalkerT2;
	TestTrue(TEXT("premessa: la ricarica di Dodge (1) e' scaduta nel Cleanup di T, lo scatto si applica"), R.Dasher->PlannedDashApplies());
	TestTrue(TEXT("premessa: lo stordimento attraversa il Cleanup di T (dura due turni)"), R.Dasher->HasStatus(TAG_Status_Stunned) && R.Walker->HasStatus(TAG_Status_Stunned));
	TestFalse(TEXT("e a T+1 lo scatto non sposta (anteprima e mira)"), R.Dasher->PlannedDashMoves());
	T::RunTurn(R.TM);
	TestEqual(TEXT("T+1: lo scatto e' negato"), R.Dasher->Cell.ToString(), DasherT1.ToString());
	TestEqual(TEXT("T+1: il passo e' negato"), R.Walker->Cell.ToString(), WalkerHome.ToString());
	TestFalse(TEXT("premessa: lo stordimento scade nel Cleanup di T+1"), R.Dasher->HasStatus(TAG_Status_Stunned) || R.Walker->HasStatus(TAG_Status_Stunned));

	// TURNO T+2 - scaduto: entrambi si muovono.
	R.Dasher->PlannedDashAbility = R.DodgeSlot;
	R.Dasher->PlannedDashCell = DasherT3;
	R.Walker->PlannedCell = WalkerT3;
	TestTrue(TEXT("premessa: Dodge si applica ancora (la ricarica del rifiuto di T+1 e' scaduta)"), R.Dasher->PlannedDashApplies());
	T::RunTurn(R.TM);
	TestEqual(TEXT("T+2: lo scatto riparte"), R.Dasher->Cell.ToString(), DasherT3.ToString());
	TestEqual(TEXT("T+2: il passo riparte"), R.Walker->Cell.ToString(), WalkerT3.ToString());
	Close(R);
	return true;
}

/**
 * Il precedente: un `Root` nato nel Blast di T nega il Move di T (D-569 (4): "e' la lettura di Root"). La tabella di #1479 non
 * era stata rimisurata; qui lo e'. La voce di TurnLog che `Root` scrive per il Move negato si stampa con `AddInfo`: non e'
 * asserita, perche' nessuna decisione la fissa (vedi il report di #3973).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStunnedRootBornInTheBlastDeniesTheMoveTest,
	"RefactorTactics.Status.StunnedMovement.TheMoveIsDeniedByARootBornInTheBlastToo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStunnedRootBornInTheBlastDeniesTheMoveTest::RunTest(const FString&)
{
	namespace T = RTStunnedMovementTestsInternal;

	for (const bool bRooter : { false, true })
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
		T::SpawnMap(World, /*Radius=*/ 8);
		ARTUnit* Walker = T::SpawnUnit(World, 0, FRTCellId(0, 0));
		ARTUnit* Rooter = T::SpawnUnit(World, 1, FRTCellId(-1, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Walker || !Rooter || !TM) { RTWorldFixtures::DestroyWorld(World); return false; }
		T::Stand(Walker);
		T::Stand(Rooter);
		Walker->PlannedPath = { FRTCellId(0, 0), FRTCellId(1, 0) };
		Walker->PlannedCell = FRTCellId(1, 0);
		if (bRooter)
		{
			const int32 Idx = RTAbilityFixtures::AddCoreAbility(Rooter, TEXT("Action.Root"));
			Rooter->PlannedAbilityIndex = Idx;
			Rooter->PlannedAttackTarget = Walker;
		}
		T::RunTurn(TM);

		if (bRooter)
		{
			TestEqual(TEXT("un Root nato nel Blast di T nega il Move di T"), Walker->Cell.ToString(), FRTCellId(0, 0).ToString());
			for (const FRTTurnLogEntry& E : TM->GetTurnLog())
			{
				if (E.UnitId == Walker->StableUnitId)
				{
					AddInfo(FString::Printf(TEXT("voce di %s: categoria %d, fase %d, esito %d, azione %s, %s -> %s"),
						TEXT("chi e' radicato"), static_cast<int32>(E.Category), static_cast<int32>(E.Phase), static_cast<int32>(E.Outcome),
						*E.ActionId.ToString(), *E.SrcCell.ToString(), *E.TgtCell.ToString()));
				}
			}
		}
		else
		{
			TestEqual(TEXT("controllo: senza Root lo stesso passo si compie"), Walker->Cell.ToString(), FRTCellId(1, 0).ToString());
		}
		RTWorldFixtures::DestroyWorld(World);
	}
	return true;
}

// =====================================================================================================
// 4. IL MOVIMENTO FORZATO non e' volontario e non si tocca
// =====================================================================================================

/**
 * Una stordita spinta o tirata si sposta come una non stordita (D-569 (3)). Il Push (core, forza 1) contro una vittima senza
 * resistenza la fa arretrare di una cella; la Pull (core, portata 2) la avvicina. Per ognuna due bracci, stordita e no:
 * arrivano alla STESSA cella, e quella cella non e' la partenza (altrimenti l'uguaglianza proverebbe un movimento forzato
 * che non c'e' stato).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStunnedForcedMovementIsNotDeniedTest,
	"RefactorTactics.Status.StunnedMovement.ForcedMovementIsNotDenied",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStunnedForcedMovementIsNotDeniedTest::RunTest(const FString&)
{
	namespace T = RTStunnedMovementTestsInternal;

	struct FCase { const TCHAR* ActionId; int32 VictimQ; };
	// La trazione vuole il bersaglio a DUE celle: da una sola la cella verso cui andrebbe e' quella di chi tira.
	const FCase Cases[] = { { TEXT("Action.Push"), 1 }, { TEXT("Action.Pull"), 2 } };
	for (const FCase& C : Cases)
	{
		FRTCellId Arrival[2];
		for (int32 Arm = 0; Arm < 2; ++Arm)
		{
			const bool bStunned = Arm == 1;
			UWorld* World = RTWorldFixtures::MakeWorld();
			if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
			T::SpawnMap(World, /*Radius=*/ 6);
			ARTUnit* Mover = T::SpawnUnit(World, 0, FRTCellId(0, 0));
			ARTUnit* Victim = T::SpawnUnit(World, 1, FRTCellId(C.VictimQ, 0));
			ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
			if (!Mover || !Victim || !TM) { RTWorldFixtures::DestroyWorld(World); return false; }
			T::Stand(Mover);
			T::Stand(Victim);
			Victim->PushResistance = 0;
			Mover->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbility(Mover, C.ActionId);
			Mover->PlannedAttackTarget = Victim;
			if (bStunned) { Victim->ApplyStatus(TAG_Status_Stunned, URTCombatLibrary::StunnedDurationTurns); }
			TestEqual(*FString::Printf(TEXT("%s: premessa: lo stordimento c'e' solo dove il braccio lo dichiara"), C.ActionId),
				Victim->HasStatus(TAG_Status_Stunned), bStunned);
			T::RunTurn(TM);
			Arrival[Arm] = Victim->Cell;
			TestFalse(*FString::Printf(TEXT("%s: la vittima %s e' stata spostata dalla forza"), C.ActionId, bStunned ? TEXT("stordita") : TEXT("di controllo")),
				Victim->Cell == FRTCellId(C.VictimQ, 0));
			RTWorldFixtures::DestroyWorld(World);
		}
		TestEqual(*FString::Printf(TEXT("%s: la stordita finisce dove finisce la non stordita"), C.ActionId),
			Arrival[1].ToString(), Arrival[0].ToString());
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
