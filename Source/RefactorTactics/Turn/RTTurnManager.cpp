#include "Turn/RTTurnManager.h"
#include "Turn/RTMovementResolver.h"
#include "Turn/RTPlaybackLibrary.h"
#include "Combat/RTCombatResolver.h"
#include "Combat/RTCombatLibrary.h"
#include "Ability/RTAbilityData.h"
#include "Bot/RTBotLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Grid/RTGridActor.h"
#include "Grid/RTGridLibrary.h"
#include "Unit/RTUnit.h"
#include "Core/RTTypes.h"
#include "RefactorTactics.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ARTTurnManager::ARTTurnManager()
{
	// Tick abilitato solo durante il playback della risoluzione (presentazione, non logica).
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void ARTTurnManager::BeginPlay()
{
	Super::BeginPlay();
	StartPlanningTimer();
}

void ARTTurnManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bIsResolving)
	{
		TickPlayback(DeltaSeconds);
	}
}

void ARTTurnManager::AddLogEvent(const FString& Message)
{
	UE_LOG(LogRT, Log, TEXT("[RT] %s"), *Message);
	RecentEvents.Add(Message);
	while (RecentEvents.Num() > MaxLogLines)
	{
		RecentEvents.RemoveAt(0);
	}
}

void ARTTurnManager::PlanBots()
{
	static const TArray<FRTGridCoord> NoBlockers;
	const ARTGridActor* Grid = Cast<ARTGridActor>(UGameplayStatics::GetActorOfClass(this, ARTGridActor::StaticClass()));
	const TArray<FRTGridCoord> VisionBlockers = Grid ? Grid->GetVisionBlockers() : NoBlockers;
	// Cost map del bot: pathfinding pesato dal terreno, con gli hazard resi impassabili (li evita).
	TMap<FRTGridCoord, int32> BotCostMap;
	if (Grid) { Grid->BuildBotCostMap(BotCostMap); }
	// Archi (rampe/scale) per il bot: puo' usare le rampe se avvicinano/aiutano la fuga.
	static const TArray<FRTTraversalEdge> NoBotEdges;
	const TArray<FRTTraversalEdge>& BotEdges = Grid ? Grid->GetEdges() : NoBotEdges;

	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), Actors);

	TArray<ARTUnit*> Units;
	Units.Reserve(Actors.Num());
	for (AActor* Actor : Actors)
	{
		if (ARTUnit* Unit = Cast<ARTUnit>(Actor))
		{
			Units.Add(Unit);
		}
	}

	for (ARTUnit* Bot : Units)
	{
		if (!Bot->bIsBotControlled || !Bot->IsAlive())
		{
			continue;
		}

		Bot->PlannedCell = Bot->GridCell;   // default: fermo
		Bot->PlannedAttackTarget = nullptr;
		Bot->PlannedAbilityIndex = INDEX_NONE;

		// Difesa: se ferito (sotto meta' HP) e ha un'abilita' di supporto pronta, la usa e salta il turno.
		bool bUsedSupport = false;
		for (int32 A = 0; A < Bot->NumAbilities(); ++A)
		{
			const URTAbilityData* Ab = Bot->GetAbility(A);
			if (Ab && Ab->bSelfTarget && Bot->CanUseAbility(A) && Bot->Health * 2 < Bot->MaxHealth)
			{
				Bot->PlannedAbilityIndex = A;
				bUsedSupport = true;
				break;
			}
		}
		if (bUsedSupport)
		{
			continue;
		}

		// Miglior (abilità, bersaglio) attaccabile — focus fire: preferisce chi può uccidere/indebolire.
		// In parallelo tiene traccia del nemico più vicino (fallback per l'avvicinamento).
		int32 BestScore = TNumericLimits<int32>::Lowest();
		int32 BestAbility = INDEX_NONE;
		ARTUnit* BestTarget = nullptr;
		ARTUnit* Nearest = nullptr;
		int32 NearestDistance = MAX_int32;

		for (ARTUnit* Other : Units)
		{
			if (!Other->IsAlive() || Other->TeamId == Bot->TeamId)
			{
				continue;
			}
			const int32 Distance = URTGridLibrary::ManhattanDistance(Bot->GridCell, Other->GridCell);
			if (Distance < NearestDistance)
			{
				NearestDistance = Distance;
				Nearest = Other;
			}
			if (!URTGridLibrary::HasLineOfSight(Bot->GridCell, Other->GridCell, VisionBlockers))
			{
				continue;
			}
			const int32 TargetHP = Other->Health + Other->Shield;
			for (int32 A = 0; A < Bot->NumAbilities(); ++A)
			{
				const URTAbilityData* Ability = Bot->GetAbility(A);
				if (!Ability || Ability->bDash || !Bot->CanUseAbility(A)
					|| !URTGridLibrary::IsWithinRange(Bot->GridCell, Other->GridCell, Ability->RangeCells))
				{
					continue; // le abilita' di scatto non sono attacchi
				}
				const int32 Score = URTBotLibrary::AttackScore(Ability->Power, TargetHP);
				if (Score > BestScore)
				{
					BestScore = Score;
					BestAbility = A;
					BestTarget = Other;
				}
			}
		}

		const int32 MoveBudget = Bot->GetEffectiveMoveRange();
		const int32 GridW = Grid ? Grid->Width : 10;
		const int32 GridH = Grid ? Grid->Height : 10;
		const bool bKiter = Bot->KiteStandoff > 0;
		// Priorita' ritirata: se un nemico e' molto vicino (meta' dello standoff), il kiter fugge
		// SUBITO, rinunciando al tiro (comportamento da kiter: non farsi raggiungere dalla mischia).
		const bool bPanic = bKiter && Nearest && NearestDistance <= Bot->KiteStandoff / 2;

		// Scatto DIFENSIVO: se minacciato e con lo scatto pronto, il bot SCHIVA con lo scatto (fase Dash,
		// prima del Blast). La posizione post-scatto ri-valida gittata/LOS del bersaglio dell'attaccante:
		// uscendo dal tiro, l'attacco previsto MANCA. Dash-only. Ritorna true se ha pianificato la fuga.
		auto TryFleeDash = [&](const FRTGridCoord& ThreatCell) -> bool
		{
			const int32 DIdx = Bot->FindDashAbilityIndex();
			const URTAbilityData* DAb = Bot->GetAbility(DIdx);
			if (!DAb || !DAb->bDash || !Bot->CanUseAbility(DIdx)) { return false; }
			const FRTGridCoord Dest = URTBotLibrary::BestKiteCell(Bot->GridCell, ThreatCell, Bot->GetEffectiveDashRange(DAb->RangeCells), BotCostMap, GridW, GridH, BotEdges);
			if (Dest == Bot->GridCell) { return false; }
			Bot->PlannedDashAbility = DIdx;
			Bot->PlannedDashCell = Dest;
			AddLogEvent(FString::Printf(TEXT("%s: scatto difensivo (schiva) -> (%d,%d,L%d)"), *Bot->GetName(), Dest.X, Dest.Y, Dest.Layer));
			return true;
		};

		if (bPanic)
		{
			// Fuga che massimizza la distanza (aggira bordi/ostacoli); tiro e bersaglio restano azzerati.
			if (!TryFleeDash(Nearest->GridCell)) { Bot->PlannedCell = URTBotLibrary::BestKiteCell(Bot->GridCell, Nearest->GridCell, MoveBudget, BotCostMap, GridW, GridH, BotEdges); }
		}
		else if (BestTarget)
		{
			Bot->PlannedAbilityIndex = BestAbility;
			Bot->PlannedAttackTarget = BestTarget;
		}
		else if (Nearest)
		{
			// Nessun tiro disponibile: un kiter arretra se la minaccia e' entro lo standoff, altrimenti
			// si avvicina per rientrare a distanza di tiro; la mischia (Guardian) chiude sempre.
			// In ogni caso si evitano le celle-copertura (routing a un turno attorno agli ostacoli).
			// Se lo SCATTO e' pronto, riposizionati/avvicinati IN FRETTA con lo scatto (fase Dash, prima del
				// Blast; piu' portata del movimento). Dash-only per il bot (niente move normale quando scatta).
				const int32 DashIdx = Bot->FindDashAbilityIndex();
				const URTAbilityData* DashAb = Bot->GetAbility(DashIdx);
				FRTGridCoord DashDest = Bot->GridCell;
				if (DashAb && DashAb->bDash && Bot->CanUseAbility(DashIdx) && !(bKiter && NearestDistance < Bot->KiteStandoff))
				{
					int32 FireRangeD = 0;
					for (int32 A = 0; A < Bot->NumAbilities(); ++A)
					{
						const URTAbilityData* Ab = Bot->GetAbility(A);
						if (Ab && !Ab->bDash && Bot->CanUseAbility(A)) { FireRangeD = FMath::Max(FireRangeD, Ab->RangeCells); }
					}
					// Prima una posizione di tiro raggiungibile con lo scatto; altrimenti chiudi la distanza.
					DashDest = URTBotLibrary::BestFiringCell(Bot->GridCell, Nearest->GridCell, Bot->GetEffectiveDashRange(DashAb->RangeCells), BotCostMap, GridW, GridH, VisionBlockers, FireRangeD, BotEdges);
					if (DashDest == Bot->GridCell)
					{
						DashDest = URTBotLibrary::BestApproachCell(Bot->GridCell, Nearest->GridCell, Bot->GetEffectiveDashRange(DashAb->RangeCells), BotCostMap, GridW, GridH, BotEdges);
					}
				}
				if (DashDest != Bot->GridCell)
				{
					Bot->PlannedDashAbility = DashIdx;
					Bot->PlannedDashCell = DashDest;
					AddLogEvent(FString::Printf(TEXT("%s: scatto -> (%d,%d,L%d)"), *Bot->GetName(), DashDest.X, DashDest.Y, DashDest.Layer));
				}
				else if (bKiter && NearestDistance < Bot->KiteStandoff)
			{
				if (!TryFleeDash(Nearest->GridCell)) { Bot->PlannedCell = URTBotLibrary::BestKiteCell(Bot->GridCell, Nearest->GridCell, MoveBudget, BotCostMap, GridW, GridH, BotEdges); }
			}
			else
			{
				// Prova a raggiungere una POSIZIONE DI TIRO (sfrutta l'alta quota del ponte per sparare oltre
				// le coperture basse). Richiede un'abilita' utilizzabile con la sua gittata; se non c'e' una
				// posizione di tiro raggiungibile, ci si avvicina come prima.
				int32 FireRange = 0;
				for (int32 A = 0; A < Bot->NumAbilities(); ++A)
				{
					const URTAbilityData* Ability = Bot->GetAbility(A);
					if (Ability && !Ability->bDash && Bot->CanUseAbility(A))
					{
						FireRange = FMath::Max(FireRange, Ability->RangeCells);
					}
				}
				FRTGridCoord FireCell = Bot->GridCell;
				if (FireRange > 0)
				{
					FireCell = URTBotLibrary::BestFiringCell(Bot->GridCell, Nearest->GridCell, MoveBudget,
						BotCostMap, GridW, GridH, VisionBlockers, FireRange, BotEdges);
				}
				// BU.2: la cella di posizionamento si sceglie via utility scoring (ScorePlan) fra le candidate
				// {restare, cella di tiro, cella d'avvicinamento}, pesando minaccia e kiting. Unifica il vecchio
				// "riposiziona per il tiro / altrimenti avvicìnati" e puo' preferire di restare se muoversi espone.
				FRTBotContext BotCtx;
				BotCtx.KiteStandoff = Bot->KiteStandoff;
				for (ARTUnit* Enemy : Units)
				{
					if (!Enemy->IsAlive() || Enemy->TeamId == Bot->TeamId) { continue; }
					BotCtx.Enemies.Add(Enemy->GridCell);
					int32 EnemyReach = Enemy->AttackRange;
					for (int32 a = 0; a < Enemy->NumAbilities(); ++a)
					{
						const URTAbilityData* EAb = Enemy->GetAbility(a);
						if (EAb && !EAb->bDash) { EnemyReach = FMath::Max(EnemyReach, EAb->RangeCells); }
					}
					BotCtx.EnemyRanges.Add(EnemyReach);
				}
				const FRTGridCoord ApproachCell = URTBotLibrary::BestApproachCell(Bot->GridCell, Nearest->GridCell, MoveBudget, BotCostMap, GridW, GridH, BotEdges);
				TArray<FRTGridCoord> MoveCands;
				MoveCands.Add(Bot->GridCell);                                  // restare e' sempre una candidata
				if (FireCell != Bot->GridCell)    { MoveCands.AddUnique(FireCell); }
				if (ApproachCell != Bot->GridCell){ MoveCands.AddUnique(ApproachCell); }
				FRTGridCoord ChosenCell = Bot->GridCell;
				int32 ChosenScore = TNumericLimits<int32>::Lowest();
				for (const FRTGridCoord& Cand : MoveCands)                     // ordine stabile -> deterministico
				{
					FRTBotPlan MovePlan;
					MovePlan.DestCell = Cand;
					const int32 CandScore = URTBotLibrary::ScorePlan(MovePlan, BotCtx);
					if (CandScore > ChosenScore) { ChosenScore = CandScore; ChosenCell = Cand; } // '>' : a parita' resta
				}
				Bot->PlannedCell = ChosenCell;
				if (ChosenCell != Bot->GridCell)
				{
					AddLogEvent(FString::Printf(TEXT("%s: posizione scelta via utility (layer %d)"), *Bot->GetName(), ChosenCell.Layer));
				}
			}
		}
	}
}

void ARTTurnManager::StartPlanningTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Aggiorna i visuali del terreno (riflette ignite/reversione avvenuti nella risoluzione).
	if (ARTGridActor* GridVis = Cast<ARTGridActor>(UGameplayStatics::GetActorOfClass(this, ARTGridActor::StaticClass())))
	{
		GridVis->RefreshTerrainVisuals();
	}

	PlanBots(); // il bot pianifica a inizio turno

	World->GetTimerManager().ClearTimer(PlanningTimerHandle);
	if (PlanningSeconds > 0.f)
	{
		World->GetTimerManager().SetTimer(PlanningTimerHandle, this, &ARTTurnManager::OnPlanningTimeout, PlanningSeconds, false);
	}
	AddLogEvent(FString::Printf(TEXT("Turno %d - pianificazione"), TurnNumber));
}

void ARTTurnManager::OnPlanningTimeout()
{
	UE_LOG(LogRT, Log, TEXT("[RT] Timer scaduto -> lock-in automatico"));
	LockInAndResolve();
}

float ARTTurnManager::GetPlanningTimeRemaining() const
{
	if (const UWorld* World = GetWorld())
	{
		const float Remaining = World->GetTimerManager().GetTimerRemaining(PlanningTimerHandle);
		return Remaining > 0.f ? Remaining : 0.f;
	}
	return 0.f;
}

void ARTTurnManager::LockInAndResolve()
{
	if (Phase != ERTMatchPhase::Planning || bIsResolving)
	{
		return; // gia' in risoluzione o non in pianificazione: ignora un secondo lock-in
	}

	// Chiude la pianificazione: ferma il timer (utile anche per il lock-in manuale).
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PlanningTimerHandle);
	}

	// Nuova risoluzione: azzera la timeline del turno (verra' popolata dalle fasi).
	ResolvedTimeline.Reset();
	TurnLog.Reset();
	bPrepActiveThisTurn = false;

	// Avanza le fasi fino a tornare a Planning; il movimento si applica nella fase Move.
	do
	{
		Phase = URTTurnRules::NextPhase(Phase);
		if (Phase == ERTMatchPhase::Prep)
		{
			ResolvePrep(); // abilita' di supporto (buff su se stessi)
		}
		else if (Phase == ERTMatchPhase::Dash)
		{
			ResolveDash(); // scatti: riposizionamento rapido PRIMA del Blast
		}
		else if (Phase == ERTMatchPhase::Blast)
		{
			ResolveCombat(); // gli attacchi usano la posizione PRIMA del movimento
		}
		else if (Phase == ERTMatchPhase::Move)
		{
			ResolveMovement();
		}
	} while (Phase != ERTMatchPhase::Planning);

	// TurnLog: ordinamento deterministico (fase -> categoria -> cella di partenza); GetAllActorsOfClass
	// non e' ordinato, quindi l'ordine di inserimento non e' affidabile (enum: confronto per valore intero).
	TurnLog.Sort([](const FRTTurnLogEntry& A, const FRTTurnLogEntry& B)
	{
		if (A.Phase != B.Phase) { return static_cast<uint8>(A.Phase) < static_cast<uint8>(B.Phase); }
		if (A.Category != B.Category) { return static_cast<uint8>(A.Category) < static_cast<uint8>(B.Category); }
		if (A.SrcCell.X != B.SrcCell.X) { return A.SrcCell.X < B.SrcCell.X; }
		if (A.SrcCell.Y != B.SrcCell.Y) { return A.SrcCell.Y < B.SrcCell.Y; }
		if (A.SrcCell.Layer != B.SrcCell.Layer) { return A.SrcCell.Layer < B.SrcCell.Layer; }
		return A.TgtCell.X < B.TgtCell.X;
	});

	// Fase Cleanup: danno hazard di fine turno (Lava/Fuoco) su chi occupa, tick durate,
	// reversione del terreno temporaneo; poi conteggio unita' vive per squadra.
	ARTGridActor* Grid = Cast<ARTGridActor>(UGameplayStatics::GetActorOfClass(this, ARTGridActor::StaticClass()));
	int32 Team0Alive = 0, Team1Alive = 0;
	{
		TArray<AActor*> Actors;
		UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), Actors);
		for (AActor* Actor : Actors)
		{
			ARTUnit* Unit = Cast<ARTUnit>(Actor);
			if (!Unit || !Unit->IsAlive())
			{
				continue;
			}

			// Hazard di fine turno: dipende solo dalla cella dell'unita' -> ordine-indipendente.
			const URTTerrainData* Terrain = Grid ? Grid->GetTerrainAt(Unit->GridCell) : nullptr;
			const int32 Hazard = Terrain ? Terrain->GetProps().EndTurnDamage : 0;
			if (Hazard > 0)
			{
				const FRTDamageResult R = URTCombatLibrary::ApplyDamage(Hazard, Unit->Shield, Unit->Health);
				AddLogEvent(FString::Printf(TEXT("%s: %d danno da %s"), *Unit->GetName(), Hazard, *Terrain->DisplayName.ToString()));
				if (R.Health <= 0)
				{
					AddLogEvent(FString::Printf(TEXT("Eliminata: %s (team %d)"), *Unit->GetName(), Unit->TeamId));
					FRTResolvedEvent Ev;
					Ev.Phase = ERTMatchPhase::Cleanup;
					Ev.Type = ERTResolvedEventType::Defeated;
					Ev.Source = Unit;
					ResolvedTimeline.Add(Ev);
				}
				Unit->ApplyCombatState(R.Health, R.Shield); // solo logico: la rimozione visiva e' differita
				if (!Unit->IsAlive())
				{
					continue; // morta: niente energia/tick per questa unita'
				}
			}

			Unit->Energy = URTCombatLibrary::GainEnergy(Unit->Energy, Unit->EnergyPerTurn, Unit->MaxEnergy);
			Unit->TickStatuses();
			Unit->TickCooldowns();
			(Unit->TeamId == 0 ? Team0Alive : Team1Alive)++;
		}
	}
	if (Grid) { Grid->TickTerrain(); } // Fuoco -> normale allo scadere della durata

	PendingOutcome = URTTurnRules::EvaluateOutcome(Team0Alive, Team1Alive);

	// Se c'e' qualcosa da mostrare (movimenti/attacchi) e il playback e' attivo, riproduci la risoluzione
	// nel tempo; altrimenti concludi subito il turno (comportamento istantaneo: es. headless/senza eventi).
	if (bEnablePlayback && ResolvedTimeline.Num() > 0)
	{
		BeginPlayback();
		return;
	}
	ConcludeTurn();
}

void ARTTurnManager::ConcludeTurn()
{
	// Morte visiva differita: ora che il playback ha mostrato le eliminazioni, rimuovi gli Actor morti
	// (prima del prossimo turno, cosi' non figurano piu' come bersagli/ostacoli).
	DestroyDefeatedUnits();

	if (PendingOutcome != ERTMatchOutcome::InProgress)
	{
		Phase = ERTMatchPhase::MatchEnded;
		const TCHAR* Msg =
			PendingOutcome == ERTMatchOutcome::Team0Wins ? TEXT("Vince il team 0 (blu)") :
			PendingOutcome == ERTMatchOutcome::Team1Wins ? TEXT("Vince il team 1 (rosso)") :
			TEXT("Pareggio");
		AddLogEvent(FString::Printf(TEXT("Partita finita: %s"), Msg));
		return; // niente nuovo turno
	}

	++TurnNumber;

	// Riavvia la pianificazione del nuovo turno.
	StartPlanningTimer();
}

void ARTTurnManager::DestroyDefeatedUnits()
{
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), Actors);
	for (AActor* Actor : Actors)
	{
		ARTUnit* Unit = Cast<ARTUnit>(Actor);
		if (Unit && !Unit->IsAlive())
		{
			Unit->Destroy();
		}
	}
}

void ARTTurnManager::ResolvePrep()
{
	// Abilita' di supporto su se stessi: aggiungono scudo pari a Power.
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), Actors);
	for (AActor* Actor : Actors)
	{
		ARTUnit* Unit = Cast<ARTUnit>(Actor);
		if (!Unit || !Unit->IsAlive())
		{
			continue;
		}
		const int32 Index = Unit->PlannedAbilityIndex;
		const URTAbilityData* Ability = Unit->GetAbility(Index);
		if (Ability && Ability->bSelfTarget && Unit->CanUseAbility(Index))
		{
			Unit->Shield += Ability->Power;
			Unit->ConsumeAbility(Index);
			AddLogEvent(FString::Printf(TEXT("%s: %s (+%d scudo)"), *Unit->GetName(), *Ability->DisplayName.ToString(), Ability->Power));
			Unit->PlannedAbilityIndex = INDEX_NONE; // consumato in Prep
			Unit->PlannedAttackTarget = nullptr;
			bPrepActiveThisTurn = true; // c'e' un beat di Prep da mostrare nel playback
		}
	}
}

void ARTTurnManager::ResolveDash()
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	ARTGridActor* Grid = Cast<ARTGridActor>(UGameplayStatics::GetActorOfClass(this, ARTGridActor::StaticClass()));
	const FVector Origin = Grid ? Grid->GetActorLocation() : FVector::ZeroVector;
	const float CellSize = Grid ? Grid->CellSize : 200.f;
	const int32 GridW = Grid ? Grid->Width : 10;
	const int32 GridH = Grid ? Grid->Height : 10;
	TMap<FRTGridCoord, int32> CostMap;
	if (Grid) { Grid->BuildCostMap(CostMap); }
	static const TArray<FRTTraversalEdge> NoEdges;
	const TArray<FRTTraversalEdge>& Edges = Grid ? Grid->GetEdges() : NoEdges;
	const float LayerH = Grid ? Grid->LayerHeight : 0.f;

	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), Actors);

	// Raccogli le unita' con uno scatto pianificato VALIDO (abilita' dash utilizzabile, destinazione
	// raggiungibile entro la portata dello scatto). Lo scatto e' consumato per il turno in ogni caso.
	TArray<ARTUnit*> Dashers;
	TArray<int32> DashAbilityIdx;
	TArray<TArray<FRTGridCoord>> Paths;
	for (AActor* Actor : Actors)
	{
		ARTUnit* Unit = Cast<ARTUnit>(Actor);
		if (!Unit || !Unit->IsAlive()) { continue; }
		const int32 DashIdx = Unit->PlannedDashAbility;
		Unit->PlannedDashAbility = INDEX_NONE; // consumato per questo turno (valido o no)
		const URTAbilityData* Dash = Unit->GetAbility(DashIdx);
		if (!Dash || !Dash->bDash || !Unit->CanUseAbility(DashIdx) || Unit->PlannedDashCell == Unit->GridCell)
		{
			continue;
		}
		// Percorso di scatto: pathfinding a grafo, entro il budget di COSTO = RangeCells dello scatto.
		TArray<FRTGridCoord> Path = URTGridLibrary::FindPathByGraph(Unit->GridCell, Unit->PlannedDashCell, CostMap, Edges, GridW, GridH);
		const int32 Cost = URTGridLibrary::PathCost(Path, CostMap, Edges);
		if (Path.Num() < 2 || Cost < 0 || Cost > Unit->GetEffectiveDashRange(Dash->RangeCells))
		{
			continue; // destinazione fuori dalla portata dello scatto
		}
		Dashers.Add(Unit);
		DashAbilityIdx.Add(DashIdx);
		Paths.Add(Path);
	}

	if (Dashers.Num() == 0) { return; }

	// Scatti simultanei, ordine-indipendenti (stesso resolver del movimento).
	const TArray<FRTPathResult> Resolved = URTMovementResolver::ResolvePaths(Paths);

	// Eventi per il playback (Move-type, fase Dash) + traccia post-lock. Catturati PRIMA del placement.
	for (int32 i = 0; i < Dashers.Num(); ++i)
	{
		if (Resolved[i].Entered.Num() > 0)
		{
			TArray<FRTGridCoord> Route;
			Route.Add(Dashers[i]->GridCell);
			Route.Append(Resolved[i].Entered);
			LastMoveRoutes.Add(Route);

			FRTResolvedEvent Ev;
			Ev.Phase = ERTMatchPhase::Dash;
			Ev.Type = ERTResolvedEventType::Move;
			Ev.Source = Dashers[i];
			Ev.Path = Route;
			ResolvedTimeline.Add(Ev);
		}
	}

	// Applica le posizioni SENZA cancellare il move normale (scatto + move) e consuma l'abilita'.
	for (int32 i = 0; i < Dashers.Num(); ++i)
	{
		ARTUnit* Unit = Dashers[i];
		const FRTGridCoord PreDash = Unit->GridCell;
		const FRTGridCoord Final = Resolved[i].Final;
		AddLogEvent(FString::Printf(TEXT("Scatto: %s -> (%d,%d,L%d)"), *Unit->GetName(), Final.X, Final.Y, Final.Layer));
		Unit->ConsumeAbility(DashAbilityIdx[i]);
		Unit->GridCell = Final;
		Unit->SetVisualLocation(Unit->WorldForCell(Final, Origin, CellSize, LayerH));
		Unit->PlannedPath.Reset();       // la path composita partiva da PreDash: non piu' valida
		Unit->PlannedWaypoints.Reset();
		if (Unit->PlannedCell == PreDash)
		{
			Unit->PlannedCell = Final;   // nessun move pianificato: resta dopo lo scatto (niente ritorno indietro)
		}
	}

	// Cross-damage per le celle pericolose ATTRAVERSATE dallo scatto (dipende solo dalle celle -> ord.-indip.).
	for (int32 i = 0; i < Dashers.Num(); ++i)
	{
		ARTUnit* Unit = Dashers[i];
		if (!Unit->IsAlive()) { continue; }
		int32 CrossDmg = 0;
		for (const FRTGridCoord& Cell : Resolved[i].Entered)
		{
			const URTTerrainData* Terrain = Grid ? Grid->GetTerrainAt(Cell) : nullptr;
			if (Terrain) { CrossDmg += Terrain->GetProps().CrossDamage; }
		}
		if (CrossDmg > 0)
		{
			const FRTDamageResult R = URTCombatLibrary::ApplyDamage(CrossDmg, Unit->Shield, Unit->Health);
			AddLogEvent(FString::Printf(TEXT("%s: %d danno scattando"), *Unit->GetName(), CrossDmg));
			if (R.Health <= 0)
			{
				AddLogEvent(FString::Printf(TEXT("Eliminata: %s (team %d)"), *Unit->GetName(), Unit->TeamId));
				FRTResolvedEvent Ev; Ev.Phase = ERTMatchPhase::Dash; Ev.Type = ERTResolvedEventType::Defeated; Ev.Source = Unit;
				ResolvedTimeline.Add(Ev);
			}
			Unit->ApplyCombatState(R.Health, R.Shield);
		}
	}

	UE_LOG(LogRT, Log, TEXT("[RT] Fase Dash: %d scatti"), Dashers.Num());
}

void ARTTurnManager::ResolveCombat()
{
	// Celle che bloccano la linea di tiro (copertura + terreno, es. cespuglio).
	static const TArray<FRTGridCoord> NoBlockers;
	ARTGridActor* Grid = Cast<ARTGridActor>(UGameplayStatics::GetActorOfClass(this, ARTGridActor::StaticClass()));
	const TArray<FRTGridCoord> Blockers = Grid ? Grid->GetVisionBlockers() : NoBlockers;

	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), Actors);

	TArray<ARTUnit*> Units;
	TMap<ARTUnit*, int32> IndexOf;
	TArray<FRTUnitCombatState> States;
	Units.Reserve(Actors.Num());
	States.Reserve(Actors.Num());
	for (AActor* Actor : Actors)
	{
		if (ARTUnit* Unit = Cast<ARTUnit>(Actor))
		{
			IndexOf.Add(Unit, Units.Num());
			Units.Add(Unit);
			States.Add(FRTUnitCombatState(Unit->Health, Unit->Shield));
		}
	}

	// Raccogli gli attacchi validi in base all'abilita' pianificata: bersaglio nemico, vivo,
	// entro la portata dell'abilita', con linea di tiro; l'abilita' deve essere utilizzabile.
	TArray<FRTAttack> Attacks;
	TArray<FRTGridCoord> AttackSrc;  // cella dell'attaccante per ogni FRTAttack (TurnLog)
	TArray<int32> AttackBonus;       // bonus altura dell'attaccante per ogni FRTAttack (TurnLog)
	TArray<ARTUnit*> Attackers;
	TArray<int32> UsedAbilityIndex;
	// Status inflitti dalle abilita' (bersaglio + tag + durata, in array paralleli).
	TArray<ARTUnit*> StatusTargets;
	TArray<FGameplayTag> StatusTags;
	TArray<int32> StatusDurations;
	// Knockback: per ogni bersaglio colpito da un'abilita' con spinta, la cella dell'attaccante, la distanza
	// e quanti attaccanti lo spingono (2+ = forze contraddittorie -> nessuna spinta, deterministico).
	TMap<ARTUnit*, FRTGridCoord> KnockFrom;
	TMap<ARTUnit*, int32> KnockDist;
	TMap<ARTUnit*, int32> KnockCount;
	for (ARTUnit* Unit : Units)
	{
		ARTUnit* Target = Unit->PlannedAttackTarget;
		const int32 AbilityIndex = Unit->PlannedAbilityIndex;
		Unit->PlannedAttackTarget = nullptr; // consumati nel turno
		Unit->PlannedAbilityIndex = INDEX_NONE;

		const URTAbilityData* Ability = Unit->GetAbility(AbilityIndex);
		// Attacco valido a meno della LOS: se solo la LOS manca, registra NoLineOfSight (attacco "a vuoto").
		const bool bBaseValid = Ability && !Ability->bDash && Target && IndexOf.Contains(Target)
			&& Target->TeamId != Unit->TeamId && Unit->CanUseAbility(AbilityIndex)
			&& URTGridLibrary::IsWithinRange(Unit->GridCell, Target->GridCell, Ability->RangeCells);
		if (bBaseValid && !URTGridLibrary::HasLineOfSight(Unit->GridCell, Target->GridCell, Blockers))
		{
			FRTTurnLogEntry NoLos;
			NoLos.Phase = ERTMatchPhase::Blast;
			NoLos.Category = ERTLogCategory::Combat;
			NoLos.Outcome = static_cast<uint8>(ERTCombatOutcome::NoLineOfSight);
			NoLos.SrcCell = Unit->GridCell;
			NoLos.TgtCell = Target->GridCell;
			NoLos.Amount = 0;
			TurnLog.Add(NoLos);
			AddLogEvent(FString::Printf(TEXT("%s -> %s: nessuna linea di tiro"), *Unit->GetName(), *Target->GetName()));
		}
		if (!bBaseValid || !URTGridLibrary::HasLineOfSight(Unit->GridCell, Target->GridCell, Blockers))
		{
			continue;
		}

		auto AddStatus = [&](ARTUnit* Victim)
		{
			if (Ability->StatusToApply.IsValid() && Ability->StatusDuration > 0)
			{
				StatusTargets.Add(Victim);
				StatusTags.Add(Ability->StatusToApply);
				StatusDurations.Add(Ability->StatusDuration);
			}
		};

		// Celle colpite in base alla forma dell'abilita'.
		TArray<FRTGridCoord> HitCells;
		switch (Ability->Shape)
		{
		case ERTAbilityShape::Line:
			HitCells = URTGridLibrary::CellsInLine(Unit->GridCell, Target->GridCell);
			break;
		case ERTAbilityShape::Cone:
			HitCells = URTGridLibrary::CellsInCone(Unit->GridCell, Target->GridCell, Ability->RangeCells);
			break;
		case ERTAbilityShape::Area:
			HitCells = URTGridLibrary::CellsInRadius(Target->GridCell, Ability->AreaRadius);
			break;
		default:
			HitCells.Add(Target->GridCell);
			break;
		}

		// Ignite (terreno dinamico): un'abilita' che incendia converte le celle infiammabili nell'area
			// in Fuoco (stesso turno: la mutazione avviene in Blast, prima del Move).
			if (Ability->bIgnites && Grid)
			{
				for (const FRTGridCoord& HC : HitCells)
				{
					const URTTerrainData* Terr = Grid->GetTerrainAt(HC);
					if (Terr && Terr->GetProps().bFlammable && Terr->IgnitesTo)
					{
						Grid->SetTerrainAt(HC, Terr->IgnitesTo, Terr->IgnitesTo->GetProps().TransientDuration);
						AddLogEvent(FString::Printf(TEXT("Incendio a (%d,%d)"), HC.X, HC.Y));
					}
				}
			}

			// Buff della cella dell'attaccante (es. Altura +danno), valutato in Blast = posizione pre-movimento.
			const URTTerrainData* AttackerTerrain = Grid ? Grid->GetTerrainAt(Unit->GridCell) : nullptr;
			const int32 AttackerDmgBonus = AttackerTerrain ? AttackerTerrain->GetProps().OccupantDamageBonus : 0;
			const int32 EffPower = URTCombatLibrary::EffectiveAttackPower(Ability->Power, AttackerDmgBonus);

			// Colpisce ogni nemico su una cella bersaglio.
		for (ARTUnit* Other : Units)
		{
			if (Other->TeamId != Unit->TeamId && HitCells.Contains(Other->GridCell))
			{
				Attacks.Add(FRTAttack(IndexOf[Other], EffPower));
				AttackSrc.Add(Unit->GridCell);
				AttackBonus.Add(AttackerDmgBonus);
				AddStatus(Other);

				// Knockback: registra l'intento di spinta (dalla cella dell'attaccante).
				if (Ability->bKnockback && Ability->KnockbackDistance > 0)
				{
					KnockFrom.Add(Other, Unit->GridCell);
					KnockDist.Add(Other, Ability->KnockbackDistance);
					KnockCount.FindOrAdd(Other)++;
				}

				// Evento per il playback: colpo Unit -> Other (mostrato nel Blast).
				FRTResolvedEvent Ev;
				Ev.Phase = ERTMatchPhase::Blast;
				Ev.Type = ERTResolvedEventType::Attack;
				Ev.Source = Unit;
				Ev.Target = Other;
				Ev.Amount = EffPower;
				ResolvedTimeline.Add(Ev);
			}
		}
		Attackers.Add(Unit);
		UsedAbilityIndex.Add(AbilityIndex);
	}

	if (Attacks.Num() == 0)
	{
		return;
	}

	const TArray<FRTUnitCombatState> Resolved = URTCombatResolver::ResolveAttacks(States, Attacks);
	AddLogEvent(FString::Printf(TEXT("Blast: %d attacchi"), Attacks.Num()));

	// Chi muore in questo Blast (viva PRIMA, morta DOPO): evento Defeated per la morte visiva differita
	// (il playback mostra prima il colpo, poi l'eliminazione).
	TArray<int32> BeforeHP, AfterHP;
	BeforeHP.Reserve(Units.Num());
	AfterHP.Reserve(Units.Num());
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		BeforeHP.Add(States[i].Health);
		AfterHP.Add(Resolved[i].Health);
	}
	for (const int32 Idx : URTCombatLibrary::NewlyDefeated(BeforeHP, AfterHP))
	{
		AddLogEvent(FString::Printf(TEXT("Eliminata: %s (team %d)"), *Units[Idx]->GetName(), Units[Idx]->TeamId));
		FRTResolvedEvent Ev;
		Ev.Phase = ERTMatchPhase::Blast;
		Ev.Type = ERTResolvedEventType::Defeated;
		Ev.Source = Units[Idx];
		ResolvedTimeline.Add(Ev);
	}

	// TurnLog: esito di ogni attacco applicato (classificato da stato pre/post + bonus altura).
	for (int32 a = 0; a < Attacks.Num(); ++a)
	{
		const int32 Idx = Attacks[a].TargetIndex;
		FRTTurnLogEntry E;
		E.Phase = ERTMatchPhase::Blast;
		E.Category = ERTLogCategory::Combat;
		E.Outcome = static_cast<uint8>(URTCombatLibrary::ClassifyCombatOutcome(BeforeHP[Idx], AfterHP[Idx], AttackBonus[a]));
		E.SrcCell = AttackSrc[a];
		E.TgtCell = Units[Idx]->GridCell;
		E.Amount = BeforeHP[Idx] - AfterHP[Idx];
		TurnLog.Add(E);
	}

	for (int32 i = 0; i < Units.Num(); ++i)
	{
		Units[i]->ApplyCombatState(Resolved[i].Health, Resolved[i].Shield); // solo logico: rimozione visiva differita
	}

	// --- Knockback (spinta): dopo il danno, sulle posizioni snapshot del Blast -----------------------
	if (KnockCount.Num() > 0)
	{
		const int32 KW = Grid ? Grid->Width : 10;
		const int32 KH = Grid ? Grid->Height : 10;
		const FVector KOrigin = Grid ? Grid->GetActorLocation() : FVector::ZeroVector;
		const float KCell = Grid ? Grid->CellSize : 200.f;
		const float KLayerH = Grid ? Grid->LayerHeight : 0.f;
		// Bloccanti: ostacoli + celle di tutte le unita' (non si spinge dentro un'altra unita').
		TArray<FRTGridCoord> KBlocked = Grid ? Grid->GetMoveBlockers() : TArray<FRTGridCoord>();
		for (ARTUnit* U : Units) { KBlocked.Add(U->GridCell); }

		// Destinazioni dallo snapshot: solo bersagli vivi spinti da ESATTAMENTE un attaccante.
		TArray<ARTUnit*> KTargets;
		TArray<FRTGridCoord> KFinal;
		for (const TPair<ARTUnit*, int32>& P : KnockCount)
		{
			ARTUnit* T = P.Key;
			if (P.Value != 1 || !IsValid(T) || !T->IsAlive()) { continue; }
			const FRTGridCoord Dest = URTCombatLibrary::KnockbackDestination(KnockFrom[T], T->GridCell, KnockDist[T], KBlocked, KW, KH);
			if (Dest != T->GridCell) { KTargets.Add(T); KFinal.Add(Dest); }
		}
		for (int32 a = 0; a < KTargets.Num(); ++a)
		{
			// Destinazione contesa (2+ verso la stessa cella): quei bersagli restano (ordine-indipendente).
			bool bContested = false;
			for (int32 b = 0; b < KTargets.Num(); ++b)
			{
				if (a != b && KFinal[a] == KFinal[b]) { bContested = true; break; }
			}
			if (bContested) { continue; }

			ARTUnit* T = KTargets[a];
			const FRTGridCoord OldCell = T->GridCell;
			const FRTGridCoord NewCell = KFinal[a];
			AddLogEvent(FString::Printf(TEXT("Spinta: %s -> (%d,%d)"), *T->GetName(), NewCell.X, NewCell.Y));

			// Percorso cardinale della spinta (per animazione + cross-damage): OldCell + celle attraversate.
			const int32 SX = FMath::Clamp(NewCell.X - OldCell.X, -1, 1);
			const int32 SY = FMath::Clamp(NewCell.Y - OldCell.Y, -1, 1);
			TArray<FRTGridCoord> KPath;
			KPath.Add(OldCell);
			int32 KCross = 0;
			for (FRTGridCoord W(OldCell.X + SX, OldCell.Y + SY, NewCell.Layer); ; W = FRTGridCoord(W.X + SX, W.Y + SY, NewCell.Layer))
			{
				KPath.Add(W);
				const URTTerrainData* Terr = Grid ? Grid->GetTerrainAt(W) : nullptr;
				if (Terr) { KCross += Terr->GetProps().CrossDamage; }
				if (W == NewCell) { break; }
			}

			// Evento di movimento per il playback: la spinta scivola OldCell -> NewCell nella fase Blast.
			{
				FRTResolvedEvent Ev;
				Ev.Phase = ERTMatchPhase::Blast;
				Ev.Type = ERTResolvedEventType::Move;
				Ev.Source = T;
				Ev.Path = KPath;
				ResolvedTimeline.Add(Ev);
			}

			T->GridCell = NewCell;
			T->SetVisualLocation(T->WorldForCell(NewCell, KOrigin, KCell, KLayerH));
			T->PlannedPath.Reset();      // path composita dalla vecchia cella non valida
			T->PlannedWaypoints.Reset();
			if (T->PlannedCell == OldCell) { T->PlannedCell = NewCell; } // niente move pianificato: resta spinto
			if (KCross > 0)
			{
				const FRTDamageResult R = URTCombatLibrary::ApplyDamage(KCross, T->Shield, T->Health);
				AddLogEvent(FString::Printf(TEXT("%s: %d danno spinto"), *T->GetName(), KCross));
				if (R.Health <= 0)
				{
					AddLogEvent(FString::Printf(TEXT("Eliminata: %s (team %d)"), *T->GetName(), T->TeamId));
					FRTResolvedEvent Ev; Ev.Phase = ERTMatchPhase::Blast; Ev.Type = ERTResolvedEventType::Defeated; Ev.Source = T;
					ResolvedTimeline.Add(Ev);
				}
				T->ApplyCombatState(R.Health, R.Shield);
			}
		}
	}

	// Attaccanti sopravvissuti: consuma l'abilita' (energia+cooldown); se gratuita, accumula energia.
	for (int32 i = 0; i < Attackers.Num(); ++i)
	{
		ARTUnit* Attacker = Attackers[i];
		if (!IsValid(Attacker) || !Attacker->IsAlive())
		{
			continue;
		}
		const URTAbilityData* Ability = Attacker->GetAbility(UsedAbilityIndex[i]);
		Attacker->ConsumeAbility(UsedAbilityIndex[i]);
		if (Ability && Ability->EnergyCost > 0)
		{
			AddLogEvent(FString::Printf(TEXT("Ultimate! %s"), *Attacker->GetName()));
		}
		else
		{
			Attacker->Energy = URTCombatLibrary::GainEnergy(Attacker->Energy, Attacker->EnergyOnHit, Attacker->MaxEnergy);
		}
	}

	// L'ultimate applica il proprio status ai bersagli sopravvissuti.
	for (int32 i = 0; i < StatusTargets.Num(); ++i)
	{
		ARTUnit* Slowed = StatusTargets[i];
		if (IsValid(Slowed) && Slowed->IsAlive())
		{
			Slowed->ApplyStatus(StatusTags[i], StatusDurations[i]);
			AddLogEvent(FString::Printf(TEXT("Status: %s"), *Slowed->GetName()));
		}
	}
}

void ARTTurnManager::ResolveMovement()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ARTGridActor* Grid = Cast<ARTGridActor>(UGameplayStatics::GetActorOfClass(this, ARTGridActor::StaticClass()));
	const FVector Origin = Grid ? Grid->GetActorLocation() : FVector::ZeroVector;
	const float CellSize = Grid ? Grid->CellSize : 200.f;
	const int32 GridW = Grid ? Grid->Width : 10;
	const int32 GridH = Grid ? Grid->Height : 10;
	// Costo per cella dal terreno (pesato): il budget di movimento e' un budget di COSTO.
	TMap<FRTGridCoord, int32> CostMap;
	if (Grid) { Grid->BuildCostMap(CostMap); }
	// Archi di traversata (rampe/scale) per il pathfinding a grafo multilivello.
	static const TArray<FRTTraversalEdge> NoEdges;
	const TArray<FRTTraversalEdge>& Edges = Grid ? Grid->GetEdges() : NoEdges;
	const float LayerH = Grid ? Grid->LayerHeight : 0.f;

	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), Actors);

	TArray<ARTUnit*> Units;
	TArray<TArray<FRTGridCoord>> Paths;
	Units.Reserve(Actors.Num());
	Paths.Reserve(Actors.Num());
	for (AActor* Actor : Actors)
	{
		ARTUnit* Unit = Cast<ARTUnit>(Actor);
		if (!Unit || !Unit->IsAlive())
		{
			continue; // i morti (es. nel Blast) non partecipano al movimento: non si muovono e non bloccano
		}
		Units.Add(Unit);

		// Path del turno: percorso composito (waypoint) se presente e coerente, altrimenti auto-route
		// dalla destinazione singola (PlannedCell). Validazione autorevole: contiguo, entro il budget
		// di COSTO; altrimenti l'unita' resta ferma (a prescindere da cosa ha inviato il client).
		TArray<FRTGridCoord> Path;
		if (Unit->PlannedPath.Num() >= 2 && Unit->PlannedPath[0] == Unit->GridCell)
		{
			Path = Unit->PlannedPath;
		}
		else if (Unit->PlannedCell != Unit->GridCell)
		{
			Path = URTGridLibrary::FindPathByGraph(Unit->GridCell, Unit->PlannedCell, CostMap, Edges, GridW, GridH);
		}

		const int32 Cost = URTGridLibrary::PathCost(Path, CostMap, Edges);
		if (Path.Num() < 2 || Cost < 0 || Cost > Unit->GetEffectiveMoveRange())
		{
			Path = { Unit->GridCell }; // fermo
		}
		Paths.Add(Path);
	}

	const TArray<FRTPathResult> Resolved = URTMovementResolver::ResolvePaths(Paths);

	// TurnLog: esito del movimento per ogni unita' (chiave = cella di partenza = Paths[i][0], stabile
	// perche' GridCell cambia dopo PlaceOnCell).
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		FRTTurnLogEntry E;
		E.Phase = ERTMatchPhase::Move;
		E.Category = ERTLogCategory::Move;
		E.Outcome = static_cast<uint8>(Resolved[i].Outcome);
		E.SrcCell = Paths[i].Num() > 0 ? Paths[i][0] : Units[i]->GridCell;
		E.TgtCell = Resolved[i].Final;
		E.Amount = Resolved[i].Entered.Num();
		TurnLog.Add(E);
		if (Resolved[i].Outcome == ERTMoveOutcome::BlockedContested)
		{
			AddLogEvent(FString::Printf(TEXT("%s: fermo (cella contesa)"), *Units[i]->GetName()));
		}
		else if (Resolved[i].Outcome == ERTMoveOutcome::BlockedByUnit)
		{
			AddLogEvent(FString::Printf(TEXT("%s: fermo (cella occupata)"), *Units[i]->GetName()));
		}
	}

	// Traccia post-lock: rotte effettivamente percorse (viz del percorso risolto). Catturate PRIMA
	// del placement, cosi' includono la cella di partenza reale.
	LastMoveRoutes.Reset();
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		if (Resolved[i].Entered.Num() > 0)
		{
			TArray<FRTGridCoord> Route;
			Route.Add(Units[i]->GridCell);
			Route.Append(Resolved[i].Entered);
			LastMoveRoutes.Add(Route);

			// Evento per il playback: rotta percorsa (start + celle attraversate) da animare.
			FRTResolvedEvent Ev;
			Ev.Phase = ERTMatchPhase::Move;
			Ev.Type = ERTResolvedEventType::Move;
			Ev.Source = Units[i];
			Ev.Path = Route;
			ResolvedTimeline.Add(Ev);
		}
	}

	// Applica le posizioni finali.
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		Units[i]->PlaceOnCell(Resolved[i].Final, Origin, CellSize, LayerH);
	}

	// Cross-damage: danno per ogni cella pericolosa ATTRAVERSATA (dipende solo dalle celle della
	// singola unita' -> ordine-indipendente). Il danno di FINE turno resta a carico della fase Cleanup.
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		ARTUnit* Unit = Units[i];
		if (!IsValid(Unit) || !Unit->IsAlive()) { continue; }
		int32 CrossDmg = 0;
		for (const FRTGridCoord& Cell : Resolved[i].Entered)
		{
			const URTTerrainData* Terrain = Grid ? Grid->GetTerrainAt(Cell) : nullptr;
			if (Terrain) { CrossDmg += Terrain->GetProps().CrossDamage; }
		}
		if (CrossDmg > 0)
		{
			const FRTDamageResult R = URTCombatLibrary::ApplyDamage(CrossDmg, Unit->Shield, Unit->Health);
			AddLogEvent(FString::Printf(TEXT("%s: %d danno attraversando"), *Unit->GetName(), CrossDmg));
			if (R.Health <= 0)
			{
				AddLogEvent(FString::Printf(TEXT("Eliminata: %s (team %d)"), *Unit->GetName(), Unit->TeamId));
				FRTResolvedEvent Ev;
				Ev.Phase = ERTMatchPhase::Move;
				Ev.Type = ERTResolvedEventType::Defeated;
				Ev.Source = Unit;
				ResolvedTimeline.Add(Ev);
			}
			Unit->ApplyCombatState(R.Health, R.Shield);
		}
	}

	UE_LOG(LogRT, Log, TEXT("[RT] Fase Move: risolte %d unita'"), Units.Num());
}

// ===================== Playback della risoluzione (presentazione) =============================

void ARTTurnManager::BeginPlayback()
{
	// Cache della trasformazione griglia per convertire celle -> mondo durante il playback.
	ARTGridActor* Grid = Cast<ARTGridActor>(UGameplayStatics::GetActorOfClass(this, ARTGridActor::StaticClass()));
	PBOrigin = Grid ? Grid->GetActorLocation() : FVector::ZeroVector;
	PBCellSize = Grid ? Grid->CellSize : 200.f;
	PBLayerHeight = Grid ? Grid->LayerHeight : 0.f;

	// Deriva le animazioni di movimento e la lista attacchi dagli eventi risolti.
	MoveAnims.Reset();
	PlaybackAttacks.Reset();
	PlaybackDefeated.Reset();
	TSet<ARTUnit*> StartPositioned; // per posizionare il cilindro all'inizio della sua PRIMA fase (Dash prima di Move)
	for (const FRTResolvedEvent& Ev : ResolvedTimeline)
	{
		if (Ev.Type == ERTResolvedEventType::Move && Ev.Source.IsValid() && Ev.Path.Num() >= 2)
		{
			FRTMoveAnim Anim;
			Anim.Unit = Ev.Source;
			Anim.Phase = Ev.Phase; // Dash o Move
			Anim.World.Reserve(Ev.Path.Num());
			for (const FRTGridCoord& C : Ev.Path)
			{
				Anim.World.Add(Ev.Source->WorldForCell(C, PBOrigin, PBCellSize, PBLayerHeight));
			}
			// Metti il cilindro all'inizio della sua PRIMA anim (Dash precede Move nella timeline):
			// niente flash sulla cella finale. Un'anim successiva della stessa unita' non ne sposta lo start.
			if (!StartPositioned.Contains(Ev.Source.Get()))
			{
				Ev.Source->SetVisualLocation(Anim.World[0]);
				StartPositioned.Add(Ev.Source.Get());
			}
			MoveAnims.Add(MoveTemp(Anim));
		}
		else if (Ev.Type == ERTResolvedEventType::Attack)
		{
			PlaybackAttacks.Add(Ev);
		}
		else if (Ev.Type == ERTResolvedEventType::Defeated)
		{
			PlaybackDefeated.Add(Ev);
		}
	}

	// Fasi attive, in ordine canonico (Prep -> Dash -> Blast -> Move). Cleanup: gia' applicato, nessun beat.
	bool bHasDash = false, bHasMove = false, bHasBlastMove = false;
	for (const FRTMoveAnim& A : MoveAnims)
	{
		if (A.Phase == ERTMatchPhase::Dash) { bHasDash = true; }
		else if (A.Phase == ERTMatchPhase::Blast) { bHasBlastMove = true; } // spinta (knockback)
		else { bHasMove = true; }
	}
	PlaybackPhases.Reset();
	if (bPrepActiveThisTurn) { PlaybackPhases.Add(ERTMatchPhase::Prep); }
	if (bHasDash) { PlaybackPhases.Add(ERTMatchPhase::Dash); }
	if (PlaybackAttacks.Num() > 0 || bHasBlastMove) { PlaybackPhases.Add(ERTMatchPhase::Blast); }
	if (bHasMove) { PlaybackPhases.Add(ERTMatchPhase::Move); }

	if (PlaybackPhases.Num() == 0)
	{
		ConcludeTurn(); // niente da mostrare
		return;
	}

	// Durata reale = somma delle durate delle fasi effettivamente riprodotte (progress bar coerente).
	// Poi accelerazione per rientrare nel tetto (SpeedMultiplierForCap, logica pura testata).
	float RawTotal = 0.f;
	for (const ERTMatchPhase Ph : PlaybackPhases)
	{
		RawTotal += DurationForPlaybackPhase(Ph);
	}
	PlaybackSpeed = URTPlaybackLibrary::SpeedMultiplierForCap(RawTotal, MaxPlaybackSeconds);
	PlaybackTotalSeconds = (PlaybackSpeed > 0.f) ? (RawTotal / PlaybackSpeed) : RawTotal;
	PlaybackElapsedTotal = 0.f;

	PlaybackPhaseIdx = 0;
	bIsResolving = true;
	SetActorTickEnabled(true);
	AddLogEvent(FString::Printf(TEXT("Risoluzione: %d fasi, ~%.1fs (x%.2f)"),
		PlaybackPhases.Num(), PlaybackTotalSeconds, PlaybackSpeed));
	EnterPlaybackPhase();
}

void ARTTurnManager::EnterPlaybackPhase()
{
	PlaybackPhaseElapsed = 0.f;
	AttacksShown = 0;
	const ERTMatchPhase Ph = PlaybackPhases[PlaybackPhaseIdx];
	AddLogEvent(FString::Printf(TEXT("Playback fase: %s"), *GetPlaybackPhaseName()));
	OnPhasePlaybackStarted.Broadcast(Ph);
	if (Ph == ERTMatchPhase::Dash || Ph == ERTMatchPhase::Move || Ph == ERTMatchPhase::Blast)
	{
		for (const FRTMoveAnim& A : MoveAnims)
		{
			if (A.Phase == Ph && A.Unit.IsValid()) { A.Unit->bIsMovingVisually = true; OnUnitMoveStarted.Broadcast(A.Unit.Get()); }
		}
	}
}

void ARTTurnManager::TickPlayback(float DeltaSeconds)
{
	const float Dt = DeltaSeconds * PlaybackSpeed; // accelerazione per il tetto di durata
	PlaybackPhaseElapsed += Dt;
	PlaybackElapsedTotal += Dt;

	const ERTMatchPhase Ph = PlaybackPhases[PlaybackPhaseIdx];
	const float PhaseDur = DurationForPlaybackPhase(Ph);

	if (Ph == ERTMatchPhase::Dash || Ph == ERTMatchPhase::Move || Ph == ERTMatchPhase::Blast)
	{
		// Movimento in PARALLELO: i cilindri di QUESTA fase (Dash o Move) scorrono con lo stesso Alpha.
		const float Alpha = (PhaseDur > 0.f) ? FMath::Clamp(PlaybackPhaseElapsed / PhaseDur, 0.f, 1.f) : 1.f;
		for (const FRTMoveAnim& A : MoveAnims)
		{
			if (A.Phase == Ph && A.Unit.IsValid())
			{
				A.Unit->SetVisualLocation(URTPlaybackLibrary::InterpolateAlongPath(A.World, Alpha));
			}
		}
	}
	else if (Ph == ERTMatchPhase::Blast)
	{
		// Rivela i colpi in serie (uno ogni AttackShowSeconds) per leggibilita' del danno.
		const int32 ShouldShow = (AttackShowSeconds > 0.f)
			? FMath::Min(PlaybackAttacks.Num(), 1 + FMath::FloorToInt(PlaybackPhaseElapsed / AttackShowSeconds))
			: PlaybackAttacks.Num();
		while (AttacksShown < ShouldShow)
		{
			const FRTResolvedEvent& Atk = PlaybackAttacks[AttacksShown];
			AddLogEvent(FString::Printf(TEXT("Colpo: %s -> %s (%d)"),
				Atk.Source.IsValid() ? *Atk.Source->GetName() : TEXT("?"),
				Atk.Target.IsValid() ? *Atk.Target->GetName() : TEXT("(eliminato)"),
				Atk.Amount));
			if (ARTUnit* AtkSrc = Atk.Source.Get()) { AtkSrc->PlayAttackMontage(); } if (ARTUnit* AtkTgt = Atk.Target.Get()) { AtkTgt->PlayHitMontage(); } OnAttackResolved.Broadcast(Atk.Source.Get(), Atk.Target.Get(), Atk.Amount);
			++AttacksShown;
		}
	}

	if (PlaybackPhaseElapsed >= PhaseDur)
	{
		// Finalizza la fase corrente.
		if (Ph == ERTMatchPhase::Dash || Ph == ERTMatchPhase::Move || Ph == ERTMatchPhase::Blast)
		{
			for (const FRTMoveAnim& A : MoveAnims)
			{
				if (A.Phase == Ph && A.Unit.IsValid() && A.World.Num() > 0)
				{
					A.Unit->SetVisualLocation(A.World.Last());
				}
			}
		}
		if (Ph == ERTMatchPhase::Blast)
		{
			while (AttacksShown < PlaybackAttacks.Num())
			{
				const FRTResolvedEvent& Atk = PlaybackAttacks[AttacksShown];
				if (ARTUnit* AtkSrc = Atk.Source.Get()) { AtkSrc->PlayAttackMontage(); } if (ARTUnit* AtkTgt = Atk.Target.Get()) { AtkTgt->PlayHitMontage(); } OnAttackResolved.Broadcast(Atk.Source.Get(), Atk.Target.Get(), Atk.Amount);
				++AttacksShown;
			}
		}

		// Morte visiva differita: le unita' eliminate IN QUESTA fase spariscono ora, dopo che il colpo
		// (Blast) o l'attraversamento (Move) e' stato mostrato. Idempotente (guardia IsHidden).
		for (const FRTResolvedEvent& D : PlaybackDefeated)
		{
			if (D.Phase == Ph && D.Source.IsValid() && !D.Source->IsHidden())
			{
				AddLogEvent(FString::Printf(TEXT("Morte mostrata: %s"), *D.Source->GetName()));
				D.Source->HideForDefeat();
				if (ARTUnit* DefU = D.Source.Get()) { DefU->PlayDefeatMontage(); } OnUnitDefeated.Broadcast(D.Source.Get());
			}
		}

		++PlaybackPhaseIdx;
		if (PlaybackPhaseIdx >= PlaybackPhases.Num())
		{
			FinishPlayback();
			return;
		}
		EnterPlaybackPhase();
	}
}

void ARTTurnManager::FinishPlayback()
{
	bIsResolving = false;
	SetActorTickEnabled(false);

	// Snap di sicurezza alle posizioni finali (la cella logica e' gia' quella finale).
	for (const FRTMoveAnim& A : MoveAnims)
	{
		if (A.Unit.IsValid())
		{
			A.Unit->SetVisualLocation(A.Unit->WorldForCell(A.Unit->GridCell, PBOrigin, PBCellSize, PBLayerHeight));
		}
	}
	// Catch-all: nasconde eventuali eliminati non ancora mostrati (hazard di Cleanup, oppure skip del playback).
	for (const FRTResolvedEvent& D : PlaybackDefeated)
	{
		if (D.Source.IsValid() && !D.Source->IsHidden())
		{
			AddLogEvent(FString::Printf(TEXT("Morte mostrata: %s"), *D.Source->GetName()));
			D.Source->HideForDefeat();
			if (ARTUnit* DefU = D.Source.Get()) { DefU->PlayDefeatMontage(); } OnUnitDefeated.Broadcast(D.Source.Get());
		}
	}

	MoveAnims.Reset();
	PlaybackAttacks.Reset();
	PlaybackDefeated.Reset();
	PlaybackPhases.Reset();

	AddLogEvent(FString::Printf(TEXT("Risoluzione completata (%.1fs)"), PlaybackElapsedTotal));
	// Fine playback (anche via Skip, che passa di qui): nessuna unita' e' piu' in movimento -> l'AnimBP torna idle.
	{
		TArray<AActor*> AllUnits;
		UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), AllUnits);
		for (AActor* UnitActor : AllUnits)
		{
			if (ARTUnit* U = Cast<ARTUnit>(UnitActor)) { U->bIsMovingVisually = false; }
		}
	}
	OnResolvePlaybackFinished.Broadcast();
	ConcludeTurn();
}

void ARTTurnManager::SkipPlayback()
{
	if (!bIsResolving)
	{
		return;
	}
	AddLogEvent(TEXT("Risoluzione: salto"));
	FinishPlayback();
}

float ARTTurnManager::DurationForPlaybackPhase(ERTMatchPhase InPhase) const
{
	switch (InPhase)
	{
	case ERTMatchPhase::Dash:
	case ERTMatchPhase::Move:
	{
		int32 MaxSeg = 0;
		for (const FRTMoveAnim& A : MoveAnims)
		{
			if (A.Phase == InPhase) { MaxSeg = FMath::Max(MaxSeg, A.World.Num() - 1); }
		}
		return (PlaybackCellsPerSecond > 0.f) ? (MaxSeg / PlaybackCellsPerSecond) : 0.f;
	}
	case ERTMatchPhase::Blast:
	{
		// Il Blast dura almeno quanto i colpi mostrati E quanto lo scivolamento del knockback.
		const float AttackTime = FMath::Max(1, PlaybackAttacks.Num()) * AttackShowSeconds;
		int32 MaxSeg = 0;
		for (const FRTMoveAnim& A : MoveAnims)
		{
			if (A.Phase == ERTMatchPhase::Blast) { MaxSeg = FMath::Max(MaxSeg, A.World.Num() - 1); }
		}
		const float MoveTime = (PlaybackCellsPerSecond > 0.f) ? (MaxSeg / PlaybackCellsPerSecond) : 0.f;
		return FMath::Max(AttackTime, MoveTime);
	}
	default:
		return PhaseBeatSeconds; // Prep/Cleanup: un beat
	}
}

FString ARTTurnManager::GetPlaybackPhaseName() const
{
	if (!bIsResolving || !PlaybackPhases.IsValidIndex(PlaybackPhaseIdx))
	{
		return FString();
	}
	switch (PlaybackPhases[PlaybackPhaseIdx])
	{
	case ERTMatchPhase::Prep:    return TEXT("Prep");
	case ERTMatchPhase::Dash:    return TEXT("Dash");
	case ERTMatchPhase::Blast:   return TEXT("Blast");
	case ERTMatchPhase::Move:    return TEXT("Move");
	case ERTMatchPhase::Cleanup: return TEXT("Cleanup");
	default:                     return TEXT("Risoluzione");
	}
}

float ARTTurnManager::GetPlaybackProgress01() const
{
	if (!bIsResolving || PlaybackTotalSeconds <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(PlaybackElapsedTotal / PlaybackTotalSeconds, 0.f, 1.f);
}
