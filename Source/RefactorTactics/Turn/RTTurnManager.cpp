#include "Turn/RTTurnManager.h"
#include "Turn/RTPacingLibrary.h"
#include "Turn/RTPlaybackLibrary.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTActionQueueLibrary.h"
#include "Turn/RTActionEffectLibrary.h"
#include "Turn/RTActionFallbackLibrary.h"
#include "Turn/RTMovementActionLibrary.h"
#include "Turn/RTReactionLibrary.h"
#include "Ability/RTCatalogLibrary.h"
#include "Combat/RTCombatResolver.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Ability/RTActionData.h"
#include "Bot/RTHexBotLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Turn/RTHexSimLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Unit/RTUnit.h"
#include "Core/RTTypes.h"
#include "RefactorTactics.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "HAL/FileManager.h"

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

void ARTTurnManager::ApplyTerrainOnEnterEffects(const FRTHexSnapshot& Snapshot, ARTUnit* Unit, const TArray<FRTCellId>& Entered)
{
	if (!Snapshot.Map || !Unit) { return; }

	for (const FRTCellId& Cell : Entered)
	{
		const FRTHexCellData* CellData = Snapshot.Map->FindCell(Cell);
		if (!CellData) { continue; }

		// Gli effetti li DICHIARA il catalogo terreni, non uno switch qui: aggiungere un terreno pericoloso
		// (o cambiarne i numeri) non tocca il resolver. Vocabolario condiviso con le azioni (FRTActionEffectSpec).
		const FRTTerrainDef Terrain = URTTerrainLibrary::FindTerrainDef(CellData->Surface);
		for (const FRTActionEffectSpec& Effect : Terrain.OnEnterEffects)
		{
			if (Effect.Effect == ERTActionEffect::Damage)
			{
				const FRTDamageResult Result = URTCombatLibrary::ApplyDamage(Effect.Amount, Unit->Shield, Unit->Health);
				// ApplyCombatState, non l'assegnazione diretta: e' la stessa contabilita' del danno da azione ed
				// e' l'unica che erode anche TemporaryShield. Scrivendo Health/Shield a mano lo scudo temporaneo
				// resterebbe al valore vecchio e il Cleanup lo sottrarrebbe una seconda volta.
				Unit->ApplyCombatState(Result.Health, Result.Shield);
				AddLogEvent(FString::Printf(TEXT("%s: %d danni da terreno (q=%d,r=%d,L%d)"),
					*Unit->GetName(), Effect.Amount, Cell.X, Cell.Y, Cell.Layer));
			}
			else if (Effect.Effect == ERTActionEffect::Status)
			{
				Unit->ApplyStatus(Effect.StatusTag, Effect.StatusDuration);
				// Il log solo se qualcosa e' successo davvero: ApplyStatus scarta le durate <= 0 (no-op
				// silenzioso), e un combat log che annuncia uno stato mai applicato e' peggio del silenzio.
				// Oggi riguarda Wet e Obscured, dichiarati con durata 0 nel catalogo (gap tracciato per CP 8.2).
				if (Effect.StatusDuration > 0)
				{
					AddLogEvent(FString::Printf(TEXT("%s: %s da terreno"), *Unit->GetName(), *Effect.StatusTag.ToString()));
				}
			}
		}
	}
}

void ARTTurnManager::PlanBots()
{
	// Osservabilita' del tuning: i pesi correnti, una riga per turno (verifica delle modifiche in PIE).
	// UE_LOG diretto (non AddLogEvent) per non riempire il combat log della HUD.
	UE_LOG(LogRT, Log, TEXT("[RT] Pesi bot: WKill=%d WDamage=%d WThreat=%d WKiteViolation=%d WApproach=%d WElevation=%d"),
		WKill, WDamage, WThreat, WKiteViolation, WApproach, WElevation);

	// Stesso snapshot autorevole del movimento e dell'input: mappa, occupazione e budget congelati.
	// Le mosse candidate nascono da ReachableCells, quindi il bot NON puo' proporre una mossa illegale
	// (niente celle inesistenti, bloccate, occupate o fuori budget) e non rifa' pathfinding per conto suo.
	TArray<ARTUnit*> Units;
	const FRTHexSnapshot Snapshot = MakeCurrentSnapshot(Units); // solo unita' vive; Units[i].UnitId == i

	for (int32 BotIdx = 0; BotIdx < Units.Num(); ++BotIdx)
	{
		ARTUnit* Bot = Units[BotIdx];
		if (!Bot->bIsBotControlled)
		{
			continue;
		}

		Bot->PlannedCell = Bot->Cell;   // default: fermo
		Bot->PlannedAttackTarget = nullptr;
		Bot->PlannedAbilityIndex = INDEX_NONE;
		Bot->PlannedPath.Reset();       // il bot pianifica destinazioni, non percorsi a waypoint
		Bot->PlannedWaypoints.Reset();

		// Difesa: se ferito (sotto meta' HP) e ha un'abilita' di supporto pronta, la usa e salta il turno.
		bool bUsedSupport = false;
		for (int32 A = 0; A < Bot->NumAbilities(); ++A)
		{
			const URTActionData* Ab = Bot->GetAbility(A);
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

		// Contesto di valutazione: nemici vivi (celle, gittata effettiva, HP+scudo) e pesi dal tuning.
		// L'ordine dei nemici viene da Units (ordine stabile dello snapshot): il punteggio non dipende
		// dall'ordine di enumerazione degli Actor.
		FRTHexBotContext Ctx;
		Ctx.Origin = Bot->Cell;
		Ctx.KiteStandoff = Bot->KiteStandoff;
		Ctx.WKill = WKill;
		Ctx.WDamage = WDamage;
		Ctx.WThreat = WThreat;
		Ctx.WKiteViolation = WKiteViolation;
		Ctx.WApproach = WApproach;
		Ctx.WElevation = WElevation;

		TArray<int32> EnemyUnitIndex; // parallelo a Ctx.Enemies: indice dell'unita' in Units
		ARTUnit* Nearest = nullptr;
		int32 NearestDistance = MAX_int32;
		for (int32 j = 0; j < Units.Num(); ++j)
		{
			ARTUnit* Other = Units[j];
			if (Other->TeamId == Bot->TeamId)
			{
				continue;
			}
			int32 EnemyReach = Other->AttackRange;
			for (int32 a = 0; a < Other->NumAbilities(); ++a)
			{
				const URTActionData* EAb = Other->GetAbility(a);
				if (EAb && !EAb->bDash) { EnemyReach = FMath::Max(EnemyReach, EAb->RangeCells); }
			}
			Ctx.Enemies.Add(Other->Cell);
			Ctx.EnemyRanges.Add(EnemyReach);
			Ctx.EnemyHealth.Add(Other->Health + Other->Shield);
			EnemyUnitIndex.Add(j);

			const int32 Distance = URTHexLibrary::HexDistance(Bot->Cell, Other->Cell);
			if (Distance < NearestDistance)
			{
				NearestDistance = Distance;
				Nearest = Other;
			}
		}

		// Scatto disponibile per questo turno (serve sia alla fuga sia alle candidate di riposizionamento).
		const int32 DashIdx = Bot->FindDashAbilityIndex();
		const URTActionData* DashAb = Bot->GetAbility(DashIdx);
		const bool bDashReady = DashAb && DashAb->bDash && Bot->CanUseAbility(DashIdx);

		// Portata dello scatto letta come la legge ResolveDash: dal CATALOGO se l'azione ne fa parte,
		// altrimenti dal campo legacy dell'asset. Se il bot leggesse un numero diverso da quello che il
		// resolver usera', proporrebbe scatti fuori portata (o si negherebbe quelli buoni).
		const int32 DashDeclaredRange = bDashReady
			? (DashAb->Def.ActionId.IsNone() ? DashAb->RangeCells : DashAb->Def.RangeCells)
			: 0;
		const int32 DashBudget = bDashReady ? Bot->GetEffectiveDashRange(DashDeclaredRange) : 0;
		const ERTMovementStyle DashStyle = bDashReady ? DashAb->Def.MovementStyle : ERTMovementStyle::None;

		// Nemici del bot, per UnitId dello snapshot: la carica li tratta come bersagli, gli altri stili come
		// ostacoli. Sono gli stessi indici che ResolveDash passa a ResolveLinearMove.
		TSet<int32> DashHostiles;
		for (int32 j = 0; j < Units.Num(); ++j)
		{
			if (Units[j] && Units[j]->IsAlive() && Units[j]->TeamId != Bot->TeamId) { DashHostiles.Add(j); }
		}

		FRTHexSnapshot DashSnapshot = Snapshot;
		if (bDashReady)
		{
			// Le candidate nascono da `ReachableCells`, che spende PUNTI MOVIMENTO (Dijkstra sui costi). Ma la
			// portata di una mobilita' LINEARE si misura in CELLE — il catalogo dice che il terreno non la
			// riduce. Passare la portata direttamente come budget tronca le candidate sul terreno caro: su
			// acqua (costo 2) uno scatto da 5 celle ne vedrebbe 2, e le celle 3-5 non verrebbero mai
			// proposte benche' il resolver le raggiunga. E' la stessa divergenza celle-vs-MP di #140, un
			// gradino piu' a monte: il filtro puo' solo SCARTARE candidate, non farle nascere.
			//
			// Si allarga quindi il budget al caso peggiore (portata x costo della cella piu' cara della
			// mappa) e si lascia che `IsDashReachable` poti cio' che non e' in linea.
			int32 CandidateBudget = DashBudget;
			if (URTMovementActionLibrary::IsLinear(DashStyle) && Snapshot.Map)
			{
				int32 MaxCellCost = 1;
				for (const FRTHexCellData& Cell : Snapshot.Map->Cells)
				{
					MaxCellCost = FMath::Max(MaxCellCost, Cell.MoveCost);
				}
				CandidateBudget = DashBudget * MaxCellCost;
			}
			DashSnapshot.Units[BotIdx].MoveBudget = CandidateBudget;
		}

		// Il bot valuta la raggiungibilita' con lo STESSO codice che la fase Dash usa per eseguirla
		// (issue #140): il grafo genera le candidate, ma e' la linearita' a dire quali sopravvivono.
		//
		// L'instradamento per STILE e' quello di ResolveDash: solo le mobilita' LINEARI passano da
		// `ResolveLinearMove`. Una mobilita' a budget (`Action.Sprint`) risolve col pathfinding, lo stesso
		// grafo da cui le candidate sono nate — quindi li' non c'e' nulla da filtrare, e applicare il filtro
		// lineare scarterebbe mosse perfettamente legali.
		//
		// Resta divergente il GATE "questa e' un'azione di scatto": qui `FindDashAbilityIndex` cerca il flag
		// legacy `bDash`, mentre ResolveDash accetta anche la sola fase `FastMovement` dichiarata dal
		// catalogo. Conseguenza: le abilita' degli eroi (che dichiarano la fase ma non il flag) non vengono
		// mai pianificate come scatto dal bot. E' preesistente e tracciato in #142, non introdotto qui.
		auto IsDashReachable = [&](const FRTHexSnapshot& Snap, const FRTCellId& Goal) -> bool
		{
			if (!URTMovementActionLibrary::IsLinear(DashStyle))
			{
				return true;
			}
			return URTMovementActionLibrary::IsLinearReachable(
				Snap.Map, Bot->Cell, Goal, DashBudget, DashStyle, Snap.Occupancy, DashHostiles);
		};

		// Priorita' ritirata: se un nemico e' molto vicino (meta' dello standoff), il kiter fugge SUBITO,
		// rinunciando al tiro. Guardia del bot quadrato, conservata: e' una scelta di archetipo, non utility.
		const bool bKiter = Bot->KiteStandoff > 0;
		if (bKiter && Nearest && NearestDistance <= Bot->KiteStandoff / 2)
		{
			if (bDashReady)
			{
				// Anche la fuga del kiter passa da ReachableCells (grafo): se la cella scelta non e'
				// raggiungibile in LINEA, lo scatto verrebbe rifiutato e il panico si tradurrebbe in un turno
				// perso. Meglio non scattare e lasciare decidere al movimento normale.
				const FRTCellId Dest = URTHexBotLibrary::BestKiteCell(DashSnapshot, BotIdx, Nearest->Cell);
				if (Dest != Bot->Cell && IsDashReachable(DashSnapshot, Dest))
				{
					Bot->PlannedDashAbility = DashIdx;
					Bot->PlannedDashCell = Dest;
					AddLogEvent(FString::Printf(TEXT("%s: scatto difensivo (schiva) -> (q=%d,r=%d,L%d)"),
						*Bot->GetName(), Dest.X, Dest.Y, Dest.Layer));
					continue;
				}
			}
			Bot->PlannedCell = URTHexBotLibrary::BestKiteCell(Snapshot, BotIdx, Nearest->Cell);
			AddLogEvent(FString::Printf(TEXT("%s: arretra -> (q=%d,r=%d,L%d)"),
				*Bot->GetName(), Bot->PlannedCell.X, Bot->PlannedCell.Y, Bot->PlannedCell.Layer));
			continue;
		}

		// --- Pool di candidate ---------------------------------------------------------------------
		// Un'unica utility sceglie fra: restare e sparare, riposizionarsi, scattare e sparare, scattare
		// per riposizionarsi. L'ATTACCO vale solo dalla cella in cui il bot si trovera' nel Blast: quella
		// attuale (il Move viene DOPO il Blast) o quella post-scatto (il Dash viene PRIMA).
		TArray<FRTHexBotPlan> Plans;
		TArray<int32> PlanAbility;  // abilita' d'attacco della candidata (INDEX_NONE = solo movimento)
		TArray<bool> PlanViaDash;   // la candidata si raggiunge con lo scatto

		auto AddCandidates = [&](const FRTHexSnapshot& Snap, int32 AbilityIndex, int32 Range, int32 Damage,
			bool bViaDash, bool bAttacksOnly)
		{
			FRTHexBotContext LocalCtx = Ctx;
			LocalCtx.AttackRange = Range;
			LocalCtx.AttackDamage = Damage;
			for (const FRTHexBotPlan& Candidate : URTHexBotLibrary::BuildCandidates(Snap, BotIdx, LocalCtx))
			{
				if (bAttacksOnly && !Candidate.bHasAttack) { continue; }
				// Le candidate nascono da ReachableCells, che segue il GRAFO. Lo scatto invece e' lineare
				// (CP 4.5): senza questo filtro il bot proporrebbe scatti che ResolveDash rifiuta, sprecando
				// l'abilita' in silenzio. L'invariante "il bot non propone mosse illegali" vale anche qui.
				if (bViaDash && !IsDashReachable(Snap, Candidate.DestCell))
				{
					continue;
				}
				Plans.Add(Candidate);
				PlanAbility.Add(Candidate.bHasAttack ? AbilityIndex : INDEX_NONE);
				PlanViaDash.Add(bViaDash);
			}
		};

		// 1) Riposizionamento col movimento normale (gittata 0 -> nessun attacco: nel Blast il bot e' ancora qui).
		AddCandidates(Snapshot, INDEX_NONE, /*Range*/ 0, /*Damage*/ 0, /*bViaDash*/ false, /*bAttacksOnly*/ false);

		// 2) Attacco da FERMO, un'abilita' per volta: budget 0 -> l'unica cella candidata e' quella attuale.
		FRTHexSnapshot StaySnapshot = Snapshot;
		StaySnapshot.Units[BotIdx].MoveBudget = 0;
		for (int32 A = 0; A < Bot->NumAbilities(); ++A)
		{
			const URTActionData* Ability = Bot->GetAbility(A);
			if (!Ability || Ability->bDash || Ability->bSelfTarget || !Bot->CanUseAbility(A)) { continue; }
			AddCandidates(StaySnapshot, A, Ability->RangeCells, Ability->Power, /*bViaDash*/ false, /*bAttacksOnly*/ true);
		}

		// 3) Scatto + attacco: dalla cella post-scatto si spara nello stesso turno (Dash prima del Blast).
		if (bDashReady)
		{
			for (int32 A = 0; A < Bot->NumAbilities(); ++A)
			{
				const URTActionData* Ability = Bot->GetAbility(A);
				if (!Ability || Ability->bDash || Ability->bSelfTarget || !Bot->CanUseAbility(A)) { continue; }
				AddCandidates(DashSnapshot, A, Ability->RangeCells, Ability->Power, /*bViaDash*/ true, /*bAttacksOnly*/ true);
			}
			// 4) Scatto per riposizionarsi (senza tiro): utile per chiudere in fretta la distanza.
			AddCandidates(DashSnapshot, INDEX_NONE, /*Range*/ 0, /*Damage*/ 0, /*bViaDash*/ true, /*bAttacksOnly*/ false);
		}

		const FRTHexBotPlan Best = URTHexBotLibrary::ChooseBestPlan(Snapshot.Map, Plans, Ctx);

		// Da quale candidata viene il piano scelto (per sapere abilita' e se passa dallo scatto). Le candidate
		// di movimento normale sono in testa: a parita' di campi si preferisce NON consumare lo scatto.
		int32 BestIdx = INDEX_NONE;
		for (int32 p = 0; p < Plans.Num(); ++p)
		{
			if (Plans[p].DestCell == Best.DestCell && Plans[p].bHasAttack == Best.bHasAttack
				&& Plans[p].TargetIndex == Best.TargetIndex && Plans[p].AttackDamage == Best.AttackDamage)
			{
				BestIdx = p;
				break;
			}
		}

		const bool bViaDash = Plans.IsValidIndex(BestIdx) && PlanViaDash[BestIdx];
		const int32 BestAbility = Plans.IsValidIndex(BestIdx) ? PlanAbility[BestIdx] : INDEX_NONE;
		ARTUnit* Target = (Best.bHasAttack && EnemyUnitIndex.IsValidIndex(Best.TargetIndex))
			? Units[EnemyUnitIndex[Best.TargetIndex]] : nullptr;
		const int32 Score = URTHexBotLibrary::ScorePlan(Snapshot.Map, Best, Ctx);

		if (bViaDash && Target && BestAbility != INDEX_NONE)
		{
			// Scatta (fase Dash) e attacca dalla cella post-scatto: nel Blast, che segue il Dash, il bot e' li'.
			Bot->PlannedDashAbility = DashIdx;
			Bot->PlannedDashCell = Best.DestCell;
			Bot->PlannedAbilityIndex = BestAbility;
			Bot->PlannedAttackTarget = Target;
			AddLogEvent(FString::Printf(TEXT("%s: utility -> scatto (q=%d,r=%d,L%d) + attacca %s score=%d"),
				*Bot->GetName(), Best.DestCell.X, Best.DestCell.Y, Best.DestCell.Layer, *Target->GetName(), Score));
		}
		else if (Target && BestAbility != INDEX_NONE)
		{
			// Resta e attacca dalla cella attuale (Best.DestCell == cella d'origine).
			Bot->PlannedCell = Best.DestCell;
			Bot->PlannedAbilityIndex = BestAbility;
			Bot->PlannedAttackTarget = Target;
			AddLogEvent(FString::Printf(TEXT("%s: utility -> (q=%d,r=%d,L%d) attacca %s score=%d"),
				*Bot->GetName(), Best.DestCell.X, Best.DestCell.Y, Best.DestCell.Layer, *Target->GetName(), Score));
		}
		else if (bViaDash)
		{
			// Riposizionamento rapido con lo scatto (nessun tiro disponibile da nessuna cella).
			Bot->PlannedDashAbility = DashIdx;
			Bot->PlannedDashCell = Best.DestCell;
			AddLogEvent(FString::Printf(TEXT("%s: scatto -> (q=%d,r=%d,L%d) score=%d"),
				*Bot->GetName(), Best.DestCell.X, Best.DestCell.Y, Best.DestCell.Layer, Score));
		}
		else
		{
			// Posizionamento con il movimento normale (o "resta", se l'utility preferisce la cella attuale).
			Bot->PlannedCell = Best.DestCell;
			AddLogEvent(FString::Printf(TEXT("%s: utility -> (q=%d,r=%d,L%d) score=%d%s"),
				*Bot->GetName(), Best.DestCell.X, Best.DestCell.Y, Best.DestCell.Layer, Score,
				Best.DestCell == Bot->Cell ? TEXT(" (resta)") : TEXT("")));
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

	BeginPacingSample(); // apre il campione: il cronometro parte quando parte la pianificazione

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
	PacingCurrent.LockInSource = ERTLockInSource::Timeout; // non l'ha chiusa il giocatore
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

	// Sonda di pacing: chiude i tempi della pianificazione. Telemetria, nessun effetto sul turno.
	{
		const double Now = FPlatformTime::Seconds();
		PacingCurrent.MsToLockIn = FMath::RoundToInt((Now - PacingPlanningStart) * 1000.0);
		// Senza nessun input, "tempo dall'ultimo input" e' l'intera pianificazione: cosi' un turno passato
		// inerte finisce fra le attese a vuoto e non fra i tagli, che e' la classificazione corretta.
		PacingCurrent.MsSinceLastInput = bPacingHadInput
			? FMath::RoundToInt((Now - PacingLastInput) * 1000.0)
			: PacingCurrent.MsToLockIn;
		if (!bPacingHadInput)
		{
			PacingCurrent.MsToFirstInput = PacingCurrent.MsToLockIn;
		}
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
	URTTurnLogLibrary::SortTurnLog(TurnLog); // ordine totale deterministico (libreria pura testabile)

	// Fase Cleanup: energia, tick di status e cooldown, conteggio delle unita' vive per squadra.
	// Gli HAZARD di fine turno non ci sono piu': dipendevano dal terreno quadrato (`URTTerrainData`), rimosso
	// col substrato al CP 7.2. L'ambiente attivo su esagoni e' l'epic E8 e riporta qui il danno di fine turno.
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

			Unit->Energy = URTCombatLibrary::GainEnergy(Unit->Energy, Unit->EnergyPerTurn, Unit->MaxEnergy);
			Unit->ExpireTemporaryShield(); // la protezione delle abilita' di supporto vale un turno solo
			Unit->TickStatuses();
			Unit->TickCooldowns();
			(Unit->TeamId == 0 ? Team0Alive : Team1Alive)++;
		}
	}

	PendingOutcome = URTTurnRules::EvaluateOutcome(Team0Alive, Team1Alive);

	// Il playback di QUESTO turno parte da zero anche se non verra' riprodotto: senza, il ramo senza
	// playback lascerebbe il valore del turno precedente e la misura leggerebbe una durata mai avvenuta.
	PlaybackElapsedTotal = 0.f;

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
	// PRIMA di tutto il resto: a partita finita questa funzione esce anticipatamente, e il turno che
	// decide la partita e' proprio quello che non verrebbe mai misurato.
	ClosePacingSample();

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
	// Prima fase che passa dal MOTORE AZIONI (epic E4): raccogli -> ordina -> traduci in EVENTI -> applica.
	// Questo orchestratore non sa piu' che cosa faccia un'abilita' di supporto: applica un evento `Shield`.
	// Aggiungere un'azione di Prep con un altro effetto (una cura, uno stato) non richiede di toccarlo.
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), Actors);

	TArray<ARTUnit*> Units;
	for (AActor* Actor : Actors)
	{
		if (ARTUnit* Unit = Cast<ARTUnit>(Actor))
		{
			Units.Add(Unit);
		}
	}
	Units.Sort([](const ARTUnit& A, const ARTUnit& B) { return URTHexLibrary::StableLess(A.Cell, B.Cell); });

	// 1. RACCOGLI: un'istanza per ogni azione di Prep pianificata e utilizzabile.
	TArray<FRTActionInstance> Instances;
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		ARTUnit* Unit = Units[i];
		if (!Unit->IsAlive()) { continue; }

		const int32 Index = Unit->PlannedAbilityIndex;
		const URTActionData* Ability = Unit->GetAbility(Index);
		if (!Ability || !Unit->CanUseAbility(Index)) { continue; }
		if (URTCatalogLibrary::MapResolutionPhase(Ability->Def.ResolutionPhase) != ERTMatchPhase::Prep) { continue; }

		FRTActionInstance Instance;
		Instance.Def = Ability->Def;
		Instance.SourceUnitId = i;
		Instance.TargetUnitId = i;   // le azioni di Prep del vertical slice agiscono su chi le usa
		Instance.TargetCell = Unit->Cell;
		Instance.EventSequence = Instances.Num();
		Instances.Add(Instance);
	}
	if (Instances.Num() == 0) { return; }

	// 2. ORDINA con la regola unica (priorita' intera intra-fase, tie-break assoluto).
	URTActionQueueLibrary::SortActionInstances(Instances);

	// 3. TRADUCI in eventi e 4. APPLICA: si lavora sugli EVENTI, non sulle abilita'.
	for (const FRTActionEvent& Event : URTActionEffectLibrary::ProduceEventsForAll(Instances))
	{
		if (!Units.IsValidIndex(Event.TargetUnitId)) { continue; }
		ARTUnit* Target = Units[Event.TargetUnitId];
		switch (Event.Kind)
		{
		case ERTActionEffect::Shield:
			Target->AddTemporaryShield(Event.Amount); // temporaneo: scade nel Cleanup (issue #96)
			AddLogEvent(FString::Printf(TEXT("%s: +%d scudo"), *Target->GetName(), Event.Amount));
			break;
		case ERTActionEffect::Heal:
			Target->ApplyCombatState(FMath::Min(Target->MaxHealth, Target->Health + Event.Amount), Target->Shield);
			AddLogEvent(FString::Printf(TEXT("%s: +%d salute"), *Target->GetName(), Event.Amount));
			break;
		case ERTActionEffect::Status:
			Target->ApplyStatus(Event.StatusTag, Event.Amount);
			AddLogEvent(FString::Printf(TEXT("%s: stato applicato"), *Target->GetName()));
			break;
		default:
			// Danno e spinta non appartengono alla Prep: risolvono nel Blast, dove l'ordine conta insieme
			// agli altri attacchi. Dichiararli qui sarebbe un errore di catalogo, non un caso da gestire.
			break;
		}
		bPrepActiveThisTurn = true; // c'e' un beat di Prep da mostrare nel playback
	}

	// 5. Consuma le abilita' usate e libera i piani.
	for (const FRTActionInstance& Instance : Instances)
	{
		ARTUnit* Unit = Units[Instance.SourceUnitId];
		Unit->ConsumeAbility(Unit->PlannedAbilityIndex);
		Unit->PlannedAbilityIndex = INDEX_NONE; // consumato in Prep
		Unit->PlannedAttackTarget = nullptr;
	}
}

void ARTTurnManager::ResolveDash()
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	FVector Origin; float CellSize; float LayerH;
	GetHexContext(Origin, CellSize, LayerH);

	// Stesso strato puro esagonale del movimento normale (CP 6.2): lo scatto e' un movimento con un altro
	// budget, non un secondo sistema di pathfinding. Lo snapshot congela mappa e occupazione a inizio fase.
	TArray<ARTUnit*> Units;
	FRTHexSnapshot Snapshot = MakeCurrentSnapshot(Units);

	// Fresco per il turno: chi corre a perdifiato (Action.Sprint) non para (CP 5.1), e questo e' l'unico
	// posto dove si sa CON CERTEZZA cosa ha davvero usato lo slot di scatto.
	ReactionBlockedThisTurn.Reset();

	// Indice (in Units) dell'attaccante per ogni impatto accodato in QUESTO scatto: serve a scartare l'impatto,
	// dopo la risoluzione simultanea, se la collisione ha bloccato il caricatore prima del contatto (CP 4.8).
	TArray<int32> PendingImpactAttackerIdx;

	// Percorsi: uno per ogni unita' (le non-scattanti restano ferme, ma occupano e bloccano come le altre).
	// Priorita' e stile lineare sono per la collisione simultanea (CP 4.8): due mobilita' del catalogo diverse
	// (`Action.Charge` priorita' 35, `Action.Dash` 30, ecc.) possono coesistere nella STESSA fase Dash.
	TArray<TArray<FRTCellId>> Paths;
	TArray<int32> DashAbilityIdx; // parallelo a Units: INDEX_NONE = non scatta
	TArray<int32> Priorities;     // parallelo a Units: FRTActionDef::Priority dell'azione, 0 per chi non scatta
	TArray<bool> bLinearMovers;   // parallelo a Units: vero se la mobilita' e' lineare (URTMovementActionLibrary::IsLinear)
	Paths.Reserve(Units.Num());
	DashAbilityIdx.Init(INDEX_NONE, Units.Num());
	Priorities.Init(0, Units.Num());
	bLinearMovers.Init(false, Units.Num());
	int32 DasherCount = 0;
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		ARTUnit* Unit = Units[i];
		Paths.Add({ Unit->Cell }); // default: fermo

		const int32 DashIdx = Unit->PlannedDashAbility;
		Unit->PlannedDashAbility = INDEX_NONE; // consumato per questo turno (valido o no)
		const URTActionData* Dash = Unit->GetAbility(DashIdx);

		// Mobilita' rapida: lo dichiara il CATALOGO (fase FastMovement -> macro-fase Dash). Il flag legacy
		// `bDash` resta accettato per le abilita' non ancora catalogate; sparisce con la migrazione di CP 4.5.
		const bool bFastMovement = Dash != nullptr && (Dash->bDash
			|| URTCatalogLibrary::MapResolutionPhase(Dash->Def.ResolutionPhase) == ERTMatchPhase::Dash);
		if (!bFastMovement || !Unit->CanUseAbility(DashIdx) || Unit->PlannedDashCell == Unit->Cell)
		{
			continue;
		}

		// Budget della fase Dash: la portata dichiarata dall'azione (con gli status), non il movimento del
		// turno. Per un'azione catalogata la verita' e' il `Def`: `Action.Sprint` vale 8 MP e quel numero sta
		// nel catalogo, non sul campo legacy dell'asset.
		const int32 DeclaredRange = Dash->Def.ActionId.IsNone() ? Dash->RangeCells : Dash->Def.RangeCells;
		const int32 EffectiveRange = Unit->GetEffectiveDashRange(DeclaredRange);

		TArray<FRTCellId> Path;
		bool bChargedIntoTarget = false;
		if (URTMovementActionLibrary::IsLinear(Dash->Def.MovementStyle))
		{
			// Mobilita' LINEARE (catalogo §2): una direzione fra le sei, e cio' che sta sulla traiettoria la
			// ferma. Non e' il pathfinding del movimento normale — con quello un muro davanti non fermerebbe
			// nulla, lo si girerebbe intorno, e `Dash.BlockedArc` non avrebbe nulla da verificare.
			TSet<int32> Hostiles;
			for (int32 u = 0; u < Units.Num(); ++u)
			{
				if (Units[u] && Units[u]->IsAlive() && Units[u]->TeamId != Unit->TeamId) { Hostiles.Add(u); }
			}

			const FRTLinearMoveResult Linear = URTMovementActionLibrary::ResolveLinearMove(
				Snapshot.Map, Unit->Cell, Unit->PlannedDashCell, EffectiveRange,
				Dash->Def.MovementStyle, Snapshot.Occupancy, Hostiles);

			// L'impatto della carica NON si applica qui: il catalogo le da' codice 20/30, cioe' movimento in
			// fase Dash e impatto fra i controlli, che risolvono per priorita' dentro il Blast.
			if (Linear.Stop == ERTLinearStop::Impact && Units.IsValidIndex(Linear.ImpactUnitId))
			{
				FRTChargeImpact Impact;
				Impact.Attacker = Unit;
				Impact.Target = Units[Linear.ImpactUnitId];
				Impact.Def = Dash->Def;
				PendingChargeImpacts.Add(Impact);
				PendingImpactAttackerIdx.Add(i);
				bChargedIntoTarget = true;
			}

			Path.Add(Unit->Cell);
			Path.Append(Linear.Entered);
		}
		else
		{
			Snapshot.Units[i].MoveBudget = EffectiveRange;
			Path = URTHexSimLibrary::FindPathForUnit(Snapshot, /*UnitId=*/ i, Unit->PlannedDashCell).Path;
		}

		// Una carica che si ferma subito ha comunque colpito: l'impatto e' gia' registrato qui sopra.
		if (Path.Num() < 2 && !bChargedIntoTarget)
		{
			continue; // destinazione non allineata, fuori portata, bloccata o occupata
		}
		if (Path.Num() < 2)
		{
			Path = { Unit->Cell };
		}
		Paths[i] = Path;
		DashAbilityIdx[i] = DashIdx;
		Priorities[i] = Dash->Def.Priority;
		bLinearMovers[i] = URTMovementActionLibrary::IsLinear(Dash->Def.MovementStyle);
		++DasherCount;
	}

	if (DasherCount == 0) { return; }

	// Scatti simultanei, ordine-indipendenti (stesso resolver a microstep del movimento, con priorita' e
	// scontro frontale fra mobilita' lineari — CP 4.8).
	const TArray<FRTHexMoveResult> Resolved = URTHexSimLibrary::ResolveHexPaths(Paths, Priorities, bLinearMovers);

	// Un impatto era stato previsto sul percorso GIA' troncato dal solo `ResolveLinearMove` (occupazione
	// congelata a inizio fase): non sa se la collisione simultanea fermera' il caricatore PRIMA del contatto
	// (due cariche opposte, CP 4.8 — senza questo filtro entrambe infliggerebbero comunque danno, pur non
	// essendosi mai davvero scontrate). Vale solo se il caricatore ha completato il percorso GIA' troncato
	// esattamente com'era: qualunque scarto (priorita' persa o scontro frontale) invalida l'impatto previsto.
	for (int32 k = PendingChargeImpacts.Num() - 1; k >= 0; --k)
	{
		const int32 AttackerIdx = PendingImpactAttackerIdx[k];
		if (!Resolved.IsValidIndex(AttackerIdx) || Resolved[AttackerIdx].Outcome != ERTMoveOutcome::Moved)
		{
			PendingChargeImpacts.RemoveAt(k);
		}
	}

	// Eventi per il playback (Move-type, fase Dash) + traccia post-lock. Catturati PRIMA del placement.
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		if (DashAbilityIdx[i] != INDEX_NONE && Resolved[i].Entered.Num() > 0)
		{
			TArray<FRTCellId> Route;
			Route.Add(Units[i]->Cell);
			Route.Append(Resolved[i].Entered);
			LastMoveRoutes.Add(Route);

			FRTResolvedEvent Ev;
			Ev.Phase = ERTMatchPhase::Dash;
			Ev.Type = ERTResolvedEventType::Move;
			Ev.Source = Units[i];
			Ev.Path = Route;
			ResolvedTimeline.Add(Ev);
		}
	}

	// Applica le posizioni SENZA cancellare il move normale (scatto + move) e consuma l'abilita'.
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		if (DashAbilityIdx[i] == INDEX_NONE) { continue; }

		ARTUnit* Unit = Units[i];
		const FRTCellId PreDash = Unit->Cell;
		const FRTCellId Final = Resolved[i].Final;
		AddLogEvent(FString::Printf(TEXT("Scatto: %s -> (q=%d,r=%d,L%d)"), *Unit->GetName(), Final.X, Final.Y, Final.Layer));
		Unit->ConsumeAbility(DashAbilityIdx[i]);
		Unit->Cell = Final;
		Unit->SetVisualLocation(Unit->WorldForCell(Final, Origin, CellSize, LayerH));
		ApplyTerrainOnEnterEffects(Snapshot, Unit, Resolved[i].Entered);
		Unit->PlannedPath.Reset();       // la path composita partiva da PreDash: non piu' valida
		Unit->PlannedWaypoints.Reset();
		if (Unit->PlannedCell == PreDash)
		{
			Unit->PlannedCell = Final;   // nessun move pianificato: resta dopo lo scatto (niente ritorno indietro)
		}

		const URTActionData* Used = Unit->GetAbility(DashAbilityIdx[i]);
		if (!Used) { continue; }

		// Chi ha usato un'azione che nega la reazione (CP 5.1: `Action.Sprint`) non ne tiene pronta una in
		// questo turno, comunque sia pianificata — vale QUI, non dove lo scatto e' stato solo pianificato,
		// perche' qui e' l'unico punto in cui l'azione risulta EFFETTIVAMENTE usata (non su cooldown, non
		// scartata dal fallback).
		if (!Used->Def.bAllowsReaction)
		{
			ReactionBlockedThisTurn.Add(Unit);
		}

		// SLOT consumati: lo dice il catalogo, non l'ActionId. `Action.Sprint` prende movimento **e** azione
		// principale — chi corre allo scoperto non prosegue col Move e non spara nello stesso turno.
		if (Used->Def.Slot == ERTActionSlot::MovementAndMain)
		{
			Unit->PlannedCell = Final;              // il movimento del turno finisce qui
			Unit->PlannedAbilityIndex = INDEX_NONE; // lo slot principale e' speso
			Unit->PlannedAttackTarget = nullptr;
		}

		// Effetti DICHIARATI dall'azione (Sprint applica `Status.Exposed`): stesso registry di Prep e Blast.
		// L'orchestratore non sa quale stato sia ne' perche': aggiungerne un altro non lo tocca.
		FRTActionInstance Instance;
		Instance.Def = Used->Def;
		Instance.SourceUnitId = i;
		Instance.TargetUnitId = i;   // le mobilita' del vertical slice applicano i propri effetti a chi le usa
		Instance.TargetCell = Final;
		Instance.EventSequence = i;
		for (const FRTActionEvent& Event : URTActionEffectLibrary::ProduceEvents(Instance))
		{
			if (Event.Kind == ERTActionEffect::Status)
			{
				Unit->ApplyStatus(Event.StatusTag, Event.Amount);
				AddLogEvent(FString::Printf(TEXT("%s: %s per %d turno/i"),
					*Unit->GetName(), *Event.StatusTag.ToString(), Event.Amount));
			}
			// Danno e spinta della Carica (CP 4.5) non si applicano qui: hanno per bersaglio il primo nemico
			// sulla linea, non chi scatta, e vanno risolti con gli altri colpi.
		}
	}

	UE_LOG(LogRT, Log, TEXT("[RT] Fase Dash: %d scatti"), DasherCount);
}

void ARTTurnManager::ResolveCombat()
{
	// Mappa ESAGONALE autorevole: portata (distanza esagonale) e linea di tiro si valutano qui.
	// Il terreno quadrato (Altura, incendio) non entra piu' nel Blast: l'ambiente attivo su hex e' l'epic E8.
	FVector HexOrigin; float HexSize; float HexLayerH;
	const URTHexMapAsset* Map = GetHexContext(HexOrigin, HexSize, HexLayerH);

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
	// Ordine STABILE per cella: GetAllActorsOfClass non e' ordinato, e da questo ordine dipendono gli indici
	// del piano, il TurnLog e la sequenza del playback. Una cella ospita al piu' un'unita' -> ordine totale.
	Units.Sort([](const ARTUnit& A, const ARTUnit& B) { return URTHexLibrary::StableLess(A.Cell, B.Cell); });

	TMap<ARTUnit*, int32> IndexOf;
	TArray<FRTUnitCombatState> States;
	TArray<FRTHexCombatUnit> HexUnits;
	States.Reserve(Units.Num());
	HexUnits.Reserve(Units.Num());
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		ARTUnit* Unit = Units[i];
		IndexOf.Add(Unit, i);
		States.Add(FRTUnitCombatState(Unit->Health, Unit->Shield));

		FRTHexCombatUnit HexUnit;
		HexUnit.UnitId = i; // identita' = indice (come FRTHexSnapshot::Units)
		HexUnit.TeamId = Unit->TeamId;
		HexUnit.Cell = Unit->Cell;
		HexUnit.bAlive = Unit->IsAlive();
		HexUnits.Add(HexUnit);
	}

	// `Action.Cleanse` (CP 5.2): azione PRINCIPALE, non una reazione, e l'unica del Blast che agisce su CHI LA
	// USA invece che su un bersaglio. Risolve PRIMA del ciclo degli intenti, per due motivi indipendenti:
	//
	// 1. quel ciclo CONSUMA `PlannedAbilityIndex` (lo azzera appena letto, per ogni unita'): un pass successivo
	//    non troverebbe piu' nulla da leggere — e' lo stesso tranello gia' incontrato con `Action.Interrupt`
	//    al CP 4.7;
	// 2. purificarsi da `Exposed` DOPO aver incassato il +5 che quello stato comporta non servirebbe a niente.
	//    Il catalogo le da' infatti codice 30 (controllo), non 40 (attacco): il controllo viene prima del danno.
	//
	// QUALE stato togliere lo dice il PIANO (`PlannedCleansePriority`), mai il resolver: si scorre la lista
	// dichiarata e si rimuove il primo stato effettivamente presente, uno solo. Lista vuota -> nessuna
	// rimozione (fail-closed): indovinare per conto del giocatore e' esattamente cio' che il catalogo vieta.
	//
	// Limite noto: non puo' togliere uno stato applicato da un'azione di controllo dello STESSO Blast (quelli
	// si applicano a fine fase, insieme agli altri effetti dei colpi). Purifica cio' che c'era a inizio turno.
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		ARTUnit* Unit = Units[i];
		const int32 CleanseIdx = Unit->PlannedAbilityIndex;
		const URTActionData* Cleanse = Unit->GetAbility(CleanseIdx);
		if (!Cleanse || Cleanse->Def.ActionId != FName(TEXT("Action.Cleanse")) || !Unit->CanUseAbility(CleanseIdx))
		{
			continue;
		}

		FGameplayTag Removed;
		for (const FGameplayTag& Candidate : Unit->PlannedCleansePriority)
		{
			if (Unit->RemoveStatus(Candidate))
			{
				Removed = Candidate;
				break; // UNO solo: e' il vincolo del catalogo, non un'ottimizzazione
			}
		}

		Unit->ConsumeAbility(CleanseIdx);
		Unit->PlannedAbilityIndex = INDEX_NONE; // consumata qui: non deve diventare anche un intento d'attacco
		Unit->PlannedAttackTarget = nullptr;

		AddLogEvent(Removed.IsValid()
			? FString::Printf(TEXT("%s: purificato %s"), *Unit->GetName(), *Removed.ToString())
			: FString::Printf(TEXT("%s: nessuno stato da purificare"), *Unit->GetName()));
	}

	// Intenti d'attacco: qui si valida l'ABILITA' (esiste, non e' uno scatto, e' utilizzabile);
	// la GEOMETRIA (portata, linea di tiro, celle colpite) la valida URTHexCombatLibrary.
	TArray<FRTHexAttackIntent> Intents;
	TArray<int32> IntentAbilityIndex;
	TArray<const URTActionData*> IntentAbility;
	TArray<FRTActionDef> IntentDefs; // la definizione che ha prodotto l'intento: anche senza un URTActionData
					 // dietro (l'impatto di una carica e' dati puri, non un'abilita' selezionata)
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		ARTUnit* Unit = Units[i];
		ARTUnit* Target = Unit->PlannedAttackTarget;
		const int32 AbilityIndex = Unit->PlannedAbilityIndex;
		Unit->PlannedAttackTarget = nullptr; // consumati nel turno
		Unit->PlannedAbilityIndex = INDEX_NONE;

		const URTActionData* Ability = Unit->GetAbility(AbilityIndex);
		if (!Ability || Ability->bDash || !Unit->CanUseAbility(AbilityIndex))
		{
			continue; // nessuna azione di Blast pianificata: non c'e' un'azione da far fallire
		}

		// Chi usa un'azione principale che nega la reazione (CP 5.1: nessuna oggi, ma il dato e' generico)
		// non ne tiene pronta una in questo turno. `Action.Sprint` (l'unico caso reale) passa dallo scatto,
		// non da qui: e' `ResolveDash` a registrarlo, perche' risolve prima e consuma lo slot principale.
		if (!Ability->Def.bAllowsReaction)
		{
			ReactionBlockedThisTurn.Add(Unit);
		}

		// Da qui si ragiona su ISTANZE, non su puntatori: e' l'istanza che si valida e su cui si applica il
		// fallback DICHIARATO dall'azione, in un punto solo (spec E4 §D5).
		FRTActionInstance Instance;
		Instance.Def = Ability->Def;
		if (Instance.Def.ActionId.IsNone())
		{
			Instance.Def.RangeCells = Ability->RangeCells; // ponte per le abilita' non ancora catalogate (CP 4.5)
		}
		Instance.SourceUnitId = i;
		Instance.TargetUnitId = (Target && IndexOf.Contains(Target)) ? IndexOf[Target] : INDEX_NONE;
		Instance.TargetCell = Target ? Target->Cell : Unit->Cell;
		Instance.EventSequence = Intents.Num();

		// Un'azione di Blast senza bersaglio non e' un'azione «che non ne ha uno» (quelle sono il movimento e il
		// supporto su se stessi, e risolvono altrove): e' un'azione che il bersaglio l'ha PERSO — eliminato e
		// rimosso dal livello, o mai valido. Senza questa distinzione l'istanza risulterebbe valida e l'unita'
		// finirebbe per puntare la propria cella.
		const ERTActionInvalidReason Reason = (Instance.TargetUnitId == INDEX_NONE)
			? ERTActionInvalidReason::TargetGone
			: URTActionFallbackLibrary::ValidateInstance(Instance, HexUnits, Map);

		// La copertura NON passa di qui: la registra il piano del Blast col suo reason code (NoLineOfSight), e
		// con la traiettoria bloccata nemmeno un'area potrebbe partire — applicarle `AttackCell` significherebbe
		// colpire attraverso il muro. Stesso discorso per la mappa assente, che e' un difetto del livello.
		const bool bHandledByPlan = Reason == ERTActionInvalidReason::NoLineOfSight
			|| Reason == ERTActionInvalidReason::NoMap;

		if (Reason != ERTActionInvalidReason::None && !bHandledByPlan)
		{
			const FRTFallbackResult Fallback = URTActionFallbackLibrary::ApplyFallback(Instance, Reason);

			// L'azione fallita non sparisce piu' in silenzio: cosa e' stato applicato e PERCHE' finiscono nel
			// TurnLog (categoria Fallback, motivo in Amount) e nel combat log.
			FRTTurnLogEntry FallbackEntry;
			FallbackEntry.Phase = ERTMatchPhase::Blast;
			FallbackEntry.Category = ERTLogCategory::Fallback;
			FallbackEntry.Outcome = static_cast<uint8>(URTActionFallbackLibrary::ToLogOutcome(Fallback.Applied));
			FallbackEntry.SrcCell = Unit->Cell;
			FallbackEntry.TgtCell = Instance.TargetCell;
			FallbackEntry.Amount = static_cast<int32>(Reason);
			TurnLog.Add(FallbackEntry);
			AddLogEvent(FString::Printf(TEXT("%s: %s"),
				*Unit->GetName(), *URTTurnLogLibrary::DescribeEntry(FallbackEntry)));

			if (!Fallback.bProducesEffects)
			{
				continue; // Cancel (e cio' che vi degrada): l'azione non avviene
			}
			Instance = Fallback.Instance; // AttackCell: si perde il bersaglio, resta la cella
		}

		FRTHexAttackIntent Intent;
		Intent.AttackerId = i;
		Intent.TargetId = Instance.TargetUnitId;
		Intent.TargetCell = Instance.TargetCell;
		Intent.Shape = Ability->Shape;
		Intent.RangeCells = Ability->RangeCells;
		Intent.AreaRadius = Ability->AreaRadius;
		// Danno DICHIARATO dagli effetti dell'azione: e' il catalogo a dirlo. Il campo legacy `Power` resta
		// come ripiego per le abilita' non ancora catalogate (quelle generiche di EnsureDefaultAbilities):
		// finche' esistono, toglierlo del tutto trasformerebbe i loro colpi in danno zero.
		int32 DeclaredDamage = 0;
		for (const FRTActionEffectSpec& Spec : Ability->Def.Effects)
		{
			if (Spec.Effect == ERTActionEffect::Damage)
			{
				DeclaredDamage = Spec.Amount;
				break;
			}
		}
		Intent.Power = URTCombatLibrary::EffectiveAttackPower(
			DeclaredDamage > 0 ? DeclaredDamage : Ability->Power, /*OccupantDamageBonus=*/ 0);
		Intents.Add(Intent);
		IntentAbilityIndex.Add(AbilityIndex);
		IntentAbility.Add(Ability);
		IntentDefs.Add(Instance.Def);
	}

	// Impatti delle cariche risolte nella fase Dash: entrano nel Blast come intenti a portata 1, cioe' addosso
	// al bersaglio. E' il codice 20/30 del catalogo — il movimento e' avvenuto prima, il colpo risolve qui, con
	// gli altri, per priorita'. Applicarlo dentro la fase Dash lo avrebbe messo fuori dall'ordine.
	for (const FRTChargeImpact& Impact : PendingChargeImpacts)
	{
		ARTUnit* Attacker = Impact.Attacker.Get();
		ARTUnit* Victim = Impact.Target.Get();
		if (!Attacker || !Victim || !IndexOf.Contains(Attacker) || !IndexOf.Contains(Victim)) { continue; }

		FRTHexAttackIntent Intent;
		Intent.AttackerId = IndexOf[Attacker];
		Intent.TargetId = IndexOf[Victim];
		Intent.TargetCell = Victim->Cell;
		Intent.Shape = ERTAbilityShape::Single;
		Intent.RangeCells = 1; // dopo l'impatto si e' adiacenti: e' questa la portata del colpo
		Intent.AreaRadius = 0;

		int32 ImpactDamage = 0;
		for (const FRTActionEffectSpec& Spec : Impact.Def.Effects)
		{
			if (Spec.Effect == ERTActionEffect::Damage) { ImpactDamage = Spec.Amount; break; }
		}
		Intent.Power = URTCombatLibrary::EffectiveAttackPower(ImpactDamage, /*OccupantDamageBonus=*/ 0);

		Intents.Add(Intent);
		IntentAbilityIndex.Add(INDEX_NONE); // nessuna abilita' da consumare: lo scatto l'ha gia' fatto
		IntentAbility.Add(nullptr);
		IntentDefs.Add(Impact.Def);
	}
	PendingChargeImpacts.Reset();

	FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(HexUnits, Intents, Map);

	// `Action.Interrupt` (CP 4.7): cancella l'INTERA azione di un'altra unita', non un effetto su un
	// bersaglio — per questo si filtra QUI, sui colpi gia' raccolti, invece di passare dal registry
	// (`URTActionEffectLibrary::ProduceEvents`), che sa tradurre effetti su un bersaglio ma non "annulla
	// l'azione X". Filtrare `Plan.Hits` prima che diventino danno o eventi cancella ENTRAMBI in un colpo solo
	// — anche per un'abilita' ad area che avrebbe prodotto piu' Hit dallo stesso attaccante.
	//
	// L'Interrupt stesso passa dalla normale validazione di bersaglio/portata/linea di tiro di
	// CollectHexAttacks (e' un intento come un altro, portata 1): un Interrupt senza linea di tiro sul
	// bersaglio non produce un Hit, quindi non cancella nulla, esattamente come un attacco bloccato dalla
	// copertura.
	TSet<int32> InterruptedAttackerIds;
	for (const FRTHexAttackHit& Hit : Plan.Hits)
	{
		if (!IntentDefs.IsValidIndex(Hit.IntentIndex)
			|| IntentDefs[Hit.IntentIndex].ActionId != FName(TEXT("Action.Interrupt")))
		{
			continue;
		}
		if (!Units.IsValidIndex(Hit.TargetId)) { continue; }

		// L'azione pianificata dal BERSAGLIO non si legge da `Unit->PlannedAbilityIndex`: il ciclo che ha
		// costruito `Intents`, qualche riga sopra, l'ha gia' CONSUMATA (azzerata) per ogni unita', bersaglio
		// compreso — e' cosi' che il turno evita di rieseguire due volte la stessa azione. Va cercata fra gli
		// `Intents` gia' catturati, nell'entrata che il bersaglio ha prodotto per SE STESSO (AttackerId ==
		// l'indice del bersaglio dell'Interrupt): e' li' che la definizione originale sopravvive al reset.
		int32 VictimIntentIdx = INDEX_NONE;
		for (int32 k = 0; k < Intents.Num(); ++k)
		{
			if (Intents[k].AttackerId == Hit.TargetId) { VictimIntentIdx = k; break; }
		}

		// Solo se il bersaglio ha DAVVERO pianificato un'azione interrompibile: un Interrupt su chi non ha
		// pianificato nulla (o ha pianificato Guard, non interrompibile) non ha niente da cancellare.
		if (VictimIntentIdx != INDEX_NONE && IntentDefs.IsValidIndex(VictimIntentIdx)
			&& IntentDefs[VictimIntentIdx].bCanBeInterrupted)
		{
			InterruptedAttackerIds.Add(Hit.TargetId);
			AddLogEvent(FString::Printf(TEXT("%s: interrotto da %s"),
				*Units[Hit.TargetId]->GetName(), *Units[Hit.AttackerId]->GetName()));
		}
	}
	// Il colpo dell'Interrupt STESSO non deve mai diventare un `FRTAttack`: non fa danno (`Effects` vuoto),
	// ma un colpo a Power 0 nell'array conterebbe comunque come "primo colpo" per `ApplyFirstHitDelta` —
	// consumando il bonus/malus di Guard/Exposed/Marked su un colpo fantasma invece che sull'attacco vero
	// che dovrebbe riceverlo. Si toglie insieme ai colpi degli interrotti, nello stesso filtro.
	Plan.Hits.RemoveAll([&InterruptedAttackerIds, &IntentDefs](const FRTHexAttackHit& Hit)
	{
		if (InterruptedAttackerIds.Contains(Hit.AttackerId)) { return true; }
		return IntentDefs.IsValidIndex(Hit.IntentIndex)
			&& IntentDefs[Hit.IntentIndex].ActionId == FName(TEXT("Action.Interrupt"));
	});

	// `Action.Intercept` (CP 5.3): la reazione piu' delicata, perche' cambia il bersaglio di un attacco ALTRUI.
	// Ha un pass tutto suo, PRIMA delle altre reazioni, e non e' una comodita': il catalogo le da' priorita' 10,
	// la piu' bassa fra le reazioni (Deflect 15, Counter 20). Se risolvesse insieme alle altre, il bersaglio
	// originale valuterebbe il proprio Counter su un colpo che non riceve piu' — contrattaccando per una ferita
	// che ha preso qualcun altro.
	//
	// "Raccogli poi applica" anche qui: si decide TUTTO sui colpi congelati, poi si riscrivono i bersagli.
	// `ExcludedHits` impedisce a due intercettori di reclamare lo stesso colpo; con due unita' per squadra la
	// contesa non e' raggiungibile (chi e' colpito non puo' intercettare per se stesso), ma il meccanismo non
	// dipende da quel numero.
	TSet<int32> ClaimedHits;
	TArray<int32> RedirectHit;      // parallelo alle claim: indice del colpo in Plan.Hits
	TArray<int32> RedirectTo;       // chi lo intercetta (UnitId = indice in Units)
	TArray<int32> RedirectFrom;     // bersaglio ORIGINALE, per il TurnLog
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		ARTUnit* Unit = Units[i];
		const int32 ReactionIdx = Unit->PlannedReactionAbility;
		const URTActionData* Reaction = Unit->GetAbility(ReactionIdx);
		if (!Reaction || Reaction->Def.Slot != ERTActionSlot::Reaction
			|| Reaction->Def.ReactionTrigger != ERTReactionTrigger::AllyHitByDirectAttack)
		{
			continue; // non e' un'interposizione: la valuta il ciclo generale piu' sotto
		}

		// Da qui l'azione e' NOSTRA: il ciclo generale non deve rivalutarla ne' registrarla una seconda volta.
		Unit->PlannedReactionAbility = INDEX_NONE;

		FRTTurnLogEntry Entry;
		Entry.Phase = ERTMatchPhase::Blast;
		Entry.Category = ERTLogCategory::Reaction;
		Entry.SrcCell = Unit->Cell;
		Entry.TgtCell = Unit->Cell;

		const int32 HitIdx = (Unit->CanUseAbility(ReactionIdx) && !ReactionBlockedThisTurn.Contains(Unit))
			? URTReactionLibrary::FindInterceptableHit(i, Reaction->Def.RangeCells,
				Plan.Hits, Intents, HexUnits, Map, ClaimedHits)
			: INDEX_NONE;

		if (!Unit->CanUseAbility(ReactionIdx) || ReactionBlockedThisTurn.Contains(Unit))
		{
			Entry.Outcome = static_cast<uint8>(ERTReactionOutcome::Unavailable);
		}
		else if (HitIdx != INDEX_NONE)
		{
			const int32 OriginalTarget = Plan.Hits[HitIdx].TargetId;
			ClaimedHits.Add(HitIdx);
			RedirectHit.Add(HitIdx);
			RedirectTo.Add(i);
			RedirectFrom.Add(OriginalTarget);

			Unit->ConsumeAbility(ReactionIdx);
			Entry.Outcome = static_cast<uint8>(ERTReactionOutcome::Activated);
			// Bersaglio ORIGINALE -> bersaglio FINALE: il TurnLog deve dire da chi a chi e' passato il colpo,
			// altrimenti un danno comparso su un'unita' mai bersagliata risulterebbe inspiegabile nel replay.
			Entry.SrcCell = Units[OriginalTarget]->Cell;
			Entry.TgtCell = Unit->Cell;
			AddLogEvent(FString::Printf(TEXT("%s: si interpone per %s"),
				*Unit->GetName(), *Units[OriginalTarget]->GetName()));
		}
		else
		{
			Entry.Outcome = static_cast<uint8>(ERTReactionOutcome::NotTriggered);
		}

		TurnLog.Add(Entry);
		AddLogEvent(FString::Printf(TEXT("%s: %s"), *Unit->GetName(), *URTTurnLogLibrary::DescribeEntry(Entry)));
	}
	// APPLICA: i bersagli si riscrivono solo ora, quando ogni decisione e' stata presa sullo stesso snapshot.
	for (int32 r = 0; r < RedirectHit.Num(); ++r)
	{
		Plan.Hits[RedirectHit[r]].TargetId = RedirectTo[r];
	}

	// Reazioni (CP 5.1): valutate sui colpi GIA' raccolti di `Plan.Hits`, dopo il filtro di Interrupt — lo
	// snapshot congelato del Blast, non un evento a cui reagire mentre il turno gira (invariante #3).
	// Un'unita' con piu' trigger validi nello stesso Blast si ferma comunque a UNA attivazione:
	// `FindTriggeringAttacker` restituisce il primo colpo che soddisfa il trigger, non li conta.
	// L'attivazione — o la non-attivazione, col motivo — finisce SEMPRE nel TurnLog, mai in silenzio.
	//
	// Gli EFFETTI delle reazioni attivate (CP 5.2) non si applicano qui: si raccolgono e si applicano insieme
	// agli altri colpi, piu' sotto. E' "raccogli poi applica" (invariante #3) — una reazione che modificasse
	// subito il danno lo farebbe su un totale ancora incompleto, e l'esito dipenderebbe dall'ordine delle unita'.
	TArray<int32> DeflectDelta;       // riduzione del danno per bersaglio, dalle `Action.Deflect` attivate
	TArray<FRTAttack> CounterAttacks; // contrattacchi delle `Action.Counter` attivate, accodati ai colpi veri
	TArray<FRTCellId> CounterAttackSrc; // parallelo a CounterAttacks: cella di chi contrattacca, per il TurnLog
	DeflectDelta.Init(0, Units.Num());
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		ARTUnit* Unit = Units[i];
		const int32 ReactionIdx = Unit->PlannedReactionAbility;
		Unit->PlannedReactionAbility = INDEX_NONE; // consumato per questo turno (attivata o no)

		const URTActionData* Reaction = Unit->GetAbility(ReactionIdx);
		if (!Reaction || Reaction->Def.Slot != ERTActionSlot::Reaction)
		{
			continue; // nessuna reazione pianificata: niente da registrare
		}

		FRTTurnLogEntry Entry;
		Entry.Phase = ERTMatchPhase::Blast;
		Entry.Category = ERTLogCategory::Reaction;
		Entry.SrcCell = Unit->Cell;
		Entry.TgtCell = Unit->Cell;

		const int32 TriggeredBy = URTReactionLibrary::FindTriggeringAttacker(
			Reaction->Def.ReactionTrigger, i, Plan.Hits, Intents);

		if (!Unit->CanUseAbility(ReactionIdx) || ReactionBlockedThisTurn.Contains(Unit))
		{
			Entry.Outcome = static_cast<uint8>(ERTReactionOutcome::Unavailable);
		}
		else if (TriggeredBy != INDEX_NONE)
		{
			Unit->ConsumeAbility(ReactionIdx);
			Entry.Outcome = static_cast<uint8>(ERTReactionOutcome::Activated);

			// Effetto della reazione, DICHIARATO dall'azione. `Counter` porta il suo danno negli `Effects`
			// (16, dal catalogo); `Deflect` non ne ha nessuno perche' "ridurre il danno subito" non e' un
			// effetto applicato a un bersaglio ma un modificatore del calcolo — sta in URTCombatLibrary,
			// come il -15 di Guard.
			int32 CounterDamage = 0;
			for (const FRTActionEffectSpec& Spec : Reaction->Def.Effects)
			{
				if (Spec.Effect == ERTActionEffect::Damage) { CounterDamage = Spec.Amount; break; }
			}
			if (CounterDamage > 0 && Units.IsValidIndex(TriggeredBy))
			{
				// Il contrattacco colpisce CHI ha colpito. Entra fra gli attacchi normali, quindi risolve
				// sullo stato iniziale come tutti gli altri: chi cade in questo stesso Blast contrattacca
				// comunque, che e' la regola gia' dichiarata da URTCombatResolver ("un'unita' colpita a
				// morte infligge comunque il proprio danno").
				CounterAttacks.Add(FRTAttack(TriggeredBy, CounterDamage));
				CounterAttackSrc.Add(Unit->Cell);
				AddLogEvent(FString::Printf(TEXT("%s: contrattacco su %s (%d)"),
					*Unit->GetName(), *Units[TriggeredBy]->GetName(), CounterDamage));
			}
			if (Reaction->Def.ActionId == FName(TEXT("Action.Deflect")))
			{
				DeflectDelta[i] -= URTCombatLibrary::DeflectDamageReduction;
			}
		}
		else
		{
			Entry.Outcome = static_cast<uint8>(ERTReactionOutcome::NotTriggered);
		}

		TurnLog.Add(Entry);
		AddLogEvent(FString::Printf(TEXT("%s: %s"), *Unit->GetName(), *URTTurnLogLibrary::DescribeEntry(Entry)));
	}

	// Intenti fermati dalla copertura: l'attacco non avviene e il TurnLog ne registra il motivo.
	for (const int32 BlockedIdx : Plan.BlockedIntents)
	{
		if (!Intents.IsValidIndex(BlockedIdx)) { continue; }
		const FRTHexAttackIntent& Blocked = Intents[BlockedIdx];
		FRTTurnLogEntry NoLos;
		NoLos.Phase = ERTMatchPhase::Blast;
		NoLos.Category = ERTLogCategory::Combat;
		NoLos.Outcome = static_cast<uint8>(ERTCombatOutcome::NoLineOfSight);
		NoLos.SrcCell = HexUnits[Blocked.AttackerId].Cell;
		NoLos.TgtCell = HexUnits[Blocked.TargetId].Cell;
		NoLos.Amount = 0;
		TurnLog.Add(NoLos);
		AddLogEvent(FString::Printf(TEXT("%s (%s -> %s)"), *URTTurnLogLibrary::DescribeEntry(NoLos),
			*Units[Blocked.AttackerId]->GetName(), *Units[Blocked.TargetId]->GetName()));
	}

	// Intenti NON VALUTABILI (nessuna mappa autorevole): non finiscono nel TurnLog come «nessuna linea di
	// tiro» — sarebbe un esito di gioco al posto di un difetto di configurazione del livello. Restano un
	// warning, perche' e' il livello a dover essere corretto, non la posizione dell'unita'.
	if (Plan.UnverifiableIntents.Num() > 0)
	{
		AddLogEvent(FString::Printf(
			TEXT("Nessuna mappa esagonale: %d attacchi non validabili, nessun colpo applicato"),
			Plan.UnverifiableIntents.Num()));
		UE_LOG(LogRT, Warning,
			TEXT("[RT] Blast senza mappa autorevole: %d intenti non valutabili (livello senza ARTHexMapActor con celle)"),
			Plan.UnverifiableIntents.Num());
	}

	// Colpi a segno -> attacchi da applicare, con gli effetti collaterali dell'abilita' che li ha prodotti.
	// Gli stati che valgono sul PRIMO danno diretto entrano qui come un delta per bersaglio: `Status.Exposed`
	// (chi ha scattato allo scoperto) somma +5, `Status.Guarded` (chi si e' messo in guardia) sottrae 15.
	// Valgono una volta sola, quindi il totale non dipende da quale colpo se li prenda; chi e' esposto E in
	// guardia li cumula, che e' l'esito prevedibile di aver fatto entrambe le cose.
	TArray<int32> FirstHitDelta;
	FirstHitDelta.Init(0, Units.Num());
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		if (!Units[i]) { continue; }
		if (Units[i]->HasStatus(TAG_Status_Exposed))
		{
			FirstHitDelta[i] += URTCombatLibrary::ExposedFirstHitBonus;
		}
		if (Units[i]->HasStatus(TAG_Status_Guarded))
		{
			FirstHitDelta[i] -= URTCombatLibrary::GuardFirstHitReduction;
		}
		// `Action.Deflect` (CP 5.2): la reazione si attiva UNA volta, quindi il suo -20 vale sul colpo che
		// l'ha innescata — stessa meccanica di Guard, non una riduzione permanente del turno.
		FirstHitDelta[i] += DeflectDelta[i];
	}

	// `Action.Brace` (CP 5.2): -10 su OGNI danno diretto, non solo sul primo. E' un secondo passaggio con una
	// funzione diversa (`ApplyDamageDelta`, senza il gate "una volta sola"): applicarlo con `ApplyFirstHitDelta`
	// insieme agli altri delta ridurrebbe un colpo e lascerebbe passare interi tutti i successivi.
	TArray<int32> EveryHitDelta;
	EveryHitDelta.Init(0, Units.Num());
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		if (Units[i] && Units[i]->HasStatus(TAG_Status_Braced))
		{
			EveryHitDelta[i] -= URTCombatLibrary::BraceDamageReduction;
		}
	}

	TArray<FRTAttack> Attacks = URTCombatResolver::ApplyDamageDelta(
		URTCombatResolver::ApplyFirstHitDelta(URTHexCombatLibrary::ToAttacks(Plan), FirstHitDelta),
		EveryHitDelta);

	TArray<FRTCellId> AttackSrc;  // cella dell'attaccante per ogni FRTAttack (TurnLog)
	TArray<ARTUnit*> Attackers;
	TArray<int32> UsedAbilityIndex;
	// Status inflitti dalle abilita' (bersaglio + tag + durata, in array paralleli).
	TArray<ARTUnit*> StatusTargets;
	TArray<FGameplayTag> StatusTags;
	TArray<int32> StatusDurations;
	// Knockback: per ogni bersaglio colpito da un'abilita' con spinta, la cella dell'attaccante, la distanza
	// e quanti attaccanti lo spingono (2+ = forze contraddittorie -> nessuna spinta, deterministico).
	TMap<ARTUnit*, FRTCellId> KnockFrom;
	TMap<ARTUnit*, int32> KnockDist;
	TMap<ARTUnit*, int32> KnockCount;
	// Trazione (`Action.Pull`, CP 4.7): stessa disciplina della spinta, array paralleli propri — una
	// direzione INVERTITA (verso chi tira, non lontano da lui) non e' la stessa spinta con un segno cambiato
	// nel dato che la applica.
	TMap<ARTUnit*, FRTCellId> PullToward;
	TMap<ARTUnit*, int32> PullDist;
	TMap<ARTUnit*, int32> PullCount;
	AttackSrc.Reserve(Plan.Hits.Num());
	for (const FRTHexAttackHit& Hit : Plan.Hits)
	{
		ARTUnit* Attacker = Units[Hit.AttackerId];
		ARTUnit* Victim = Units[Hit.TargetId];
		const URTActionData* Ability = IntentAbility.IsValidIndex(Hit.IntentIndex) ? IntentAbility[Hit.IntentIndex] : nullptr;
		const bool bHasDef = IntentDefs.IsValidIndex(Hit.IntentIndex);
		AttackSrc.Add(HexUnits[Hit.AttackerId].Cell);

		// Effetti COLLATERALI del colpo (stato, spinta) dagli EVENTI dichiarati dall'azione, non da flag
		// letti qui: e' il motore azioni (epic E4). Il danno resta separato perche' segue una regola sua —
		// si somma per bersaglio e si applica in blocco, cosi' l'ordine dei colpi non cambia l'esito.
		if (bHasDef)
		{
			FRTActionInstance Instance;
			Instance.Def = IntentDefs[Hit.IntentIndex];
			Instance.SourceUnitId = Hit.AttackerId;
			Instance.TargetUnitId = Hit.TargetId;
			Instance.TargetCell = HexUnits[Hit.TargetId].Cell;
			Instance.EventSequence = Plan.Hits.Num();

			for (const FRTActionEvent& Event : URTActionEffectLibrary::ProduceEvents(Instance))
			{
				switch (Event.Kind)
				{
				case ERTActionEffect::Status:
					StatusTargets.Add(Victim);
					StatusTags.Add(Event.StatusTag);
					StatusDurations.Add(Event.Amount);
					break;

				case ERTActionEffect::Push:
					KnockFrom.Add(Victim, HexUnits[Hit.AttackerId].Cell);
					KnockDist.Add(Victim, Event.Amount);
					KnockCount.FindOrAdd(Victim)++;
					break;

				case ERTActionEffect::Pull:
					PullToward.Add(Victim, HexUnits[Hit.AttackerId].Cell);
					PullDist.Add(Victim, Event.Amount);
					PullCount.FindOrAdd(Victim)++;
					break;

				default:
					// Il danno e' gia' nel piano dei colpi (Hit.Power); cure e scudi non appartengono a un
					// colpo a segno. Nessun altro effetto ha senso qui: dichiararlo sarebbe un errore di
					// catalogo, non un caso da gestire.
					break;
				}
			}
		}

		// Evento per il playback: colpo Attacker -> Victim (mostrato nel Blast).
		FRTResolvedEvent Ev;
		Ev.Phase = ERTMatchPhase::Blast;
		Ev.Type = ERTResolvedEventType::Attack;
		Ev.Source = Attacker;
		Ev.Target = Victim;
		Ev.Amount = Hit.Power;
		ResolvedTimeline.Add(Ev);

		// L'abilita' si consuma una volta per attaccante, anche se il colpo prende piu' bersagli.
		if (!Attackers.Contains(Attacker))
		{
			Attackers.Add(Attacker);
			UsedAbilityIndex.Add(IntentAbilityIndex.IsValidIndex(Hit.IntentIndex) ? IntentAbilityIndex[Hit.IntentIndex] : INDEX_NONE);
		}
	}

	// Contrattacchi (`Action.Counter`, CP 5.2): accodati QUI, dopo che `AttackSrc` e' completo — i due array
	// sono paralleli e il TurnLog li legge per indice, quindi vanno estesi insieme o le voci si disallineano.
	//
	// In coda, non in mezzo: il catalogo descrive il contrattacco come un attacco eseguito "dopo l'attacco
	// ricevuto". `ResolveAttacks` somma comunque per bersaglio sullo stato iniziale, quindi la posizione non
	// cambia il totale; cambia quale colpo conta come "primo" per Guard/Exposed/Deflect, ed e' giusto che sia
	// l'attacco pianificato a consumare quei delta, non un contrattacco arrivato di rimbalzo.
	Attacks.Append(CounterAttacks);
	AttackSrc.Append(CounterAttackSrc);

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

	// TurnLog: esito di ogni attacco applicato (classificato da stato pre/post).
	// Il bonus di elevazione e' 0 finche' l'ambiente esagonale non lo reintroduce (epic E8/E9): senza dato
	// reale, dichiararlo 0 e' preferibile a leggerlo da un terreno quadrato che non e' piu' nella partita.
	for (int32 a = 0; a < Attacks.Num(); ++a)
	{
		const int32 Idx = Attacks[a].TargetIndex;
		FRTTurnLogEntry E;
		E.Phase = ERTMatchPhase::Blast;
		E.Category = ERTLogCategory::Combat;
		E.Outcome = static_cast<uint8>(URTCombatLibrary::ClassifyCombatOutcome(BeforeHP[Idx], AfterHP[Idx], /*AttackerDmgBonus=*/ 0));
		E.SrcCell = AttackSrc[a];
		E.TgtCell = Units[Idx]->Cell;
		E.Amount = BeforeHP[Idx] - AfterHP[Idx];
		TurnLog.Add(E);
	}

	for (int32 i = 0; i < Units.Num(); ++i)
	{
		Units[i]->ApplyCombatState(Resolved[i].Health, Resolved[i].Shield); // solo logico: rimozione visiva differita
	}

	// --- Spinta (knockback): dopo il danno, sulle posizioni snapshot del Blast -----------------------
	// Direzione ESAGONALE (una delle sei), non piu' cardinale: la spinta segue la linea attaccante->bersaglio
	// e si ferma su bordo mappa, ostacolo o unita'. Spinte multiple sullo stesso bersaglio si annullano.
	if (KnockCount.Num() > 0)
	{
		// Bloccanti: le celle di tutte le unita' (non si spinge dentro un'altra unita').
		TArray<FRTCellId> KOccupied;
		for (ARTUnit* U : Units) { KOccupied.Add(U->Cell); }

		// Destinazioni dallo snapshot: solo bersagli vivi spinti da ESATTAMENTE un attaccante.
		// Si itera su Units (ordine stabile per cella): l'ordine di iterazione di una TMap non e' garantito
		// e da qui dipendono la sequenza del playback e quella del combat log.
		TArray<ARTUnit*> KTargets;
		TArray<FRTCellId> KFinal;
		for (ARTUnit* T : Units)
		{
			const int32* Pushes = KnockCount.Find(T);
			if (!Pushes || *Pushes != 1 || !IsValid(T) || !T->IsAlive()) { continue; }

			// `Action.Guard` regge una spinta di UNA cella: chi si e' piantato non arretra di un passo, ma una
			// spinta piu' forte lo sposta comunque (la guardia non e' un'ancora, catalogo v0.1 §1).
			if (T->HasStatus(TAG_Status_Guarded) && KnockDist[T] <= URTCombatLibrary::GuardResistedPushDistance)
			{
				AddLogEvent(FString::Printf(TEXT("%s: in guardia, resiste alla spinta"), *T->GetName()));
				continue;
			}

			// `Action.Brace` (CP 5.2) "impedisce la PRIMA spinta", senza limite di distanza: e' cio' che lo
			// distingue da Guard, che regge solo un passo. "Prima" e non "tutte" e' rispettato per costruzione,
			// non da un contatore: tutte le spinte del Blast si risolvono in questo unico passaggio, e un
			// bersaglio spinto da 2+ attaccanti e' gia' escluso sopra (`*Pushes != 1`, forze contraddittorie).
			if (T->HasStatus(TAG_Status_Braced))
			{
				AddLogEvent(FString::Printf(TEXT("%s: irrigidito, la spinta non lo sposta"), *T->GetName()));
				continue;
			}

			const FRTCellId Dest = URTHexCombatLibrary::HexKnockbackDestination(
				KnockFrom[T], T->Cell, KnockDist[T], Map, KOccupied);
			if (Dest != T->Cell) { KTargets.Add(T); KFinal.Add(Dest); }
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
			const FRTCellId OldCell = T->Cell;
			const FRTCellId NewCell = KFinal[a];
			AddLogEvent(FString::Printf(TEXT("Spinta: %s -> (q=%d,r=%d,L%d)"),
				*T->GetName(), NewCell.X, NewCell.Y, NewCell.Layer));

			// Celle attraversate dalla spinta (per l'animazione): la linea esagonale fra le due celle, i cui
			// passi sono adiacenti per costruzione. Il danno da attraversamento del terreno quadrato non esiste
			// piu' nella partita esagonale: torna con l'ambiente attivo (epic E8).
			const TArray<FRTCellId> KPath = URTHexLibrary::HexLine(OldCell, NewCell);

			// Evento di movimento per il playback: la spinta scivola OldCell -> NewCell nella fase Blast.
			{
				FRTResolvedEvent Ev;
				Ev.Phase = ERTMatchPhase::Blast;
				Ev.Type = ERTResolvedEventType::Move;
				Ev.Source = T;
				Ev.Path = KPath;
				ResolvedTimeline.Add(Ev);
			}

			T->Cell = NewCell;
			T->SetVisualLocation(T->WorldForCell(NewCell, HexOrigin, HexSize, HexLayerH));
			T->PlannedPath.Reset();      // path composita dalla vecchia cella non valida
			T->PlannedWaypoints.Reset();
			if (T->PlannedCell == OldCell) { T->PlannedCell = NewCell; } // niente move pianificato: resta spinto
		}
	}

	// --- Trazione (`Action.Pull`, CP 4.7): stessa disciplina della spinta, direzione opposta -------------
	// Il catalogo v0.1 §1 riserva la resistenza di `Action.Guard` alla "spinta": una trazione non viene
	// resistita da chi si e' messo in guardia — non e' un'estensione implicita, e' cio' che il testo dice.
	if (PullCount.Num() > 0)
	{
		TArray<FRTCellId> POccupied;
		for (ARTUnit* U : Units) { POccupied.Add(U->Cell); }

		TArray<ARTUnit*> PTargets;
		TArray<FRTCellId> PFinal;
		for (ARTUnit* T : Units)
		{
			const int32* Pulls = PullCount.Find(T);
			if (!Pulls || *Pulls != 1 || !IsValid(T) || !T->IsAlive()) { continue; }

			const FRTCellId Dest = URTHexCombatLibrary::HexPullDestination(
				PullToward[T], T->Cell, PullDist[T], Map, POccupied);
			if (Dest != T->Cell) { PTargets.Add(T); PFinal.Add(Dest); }
		}
		for (int32 a = 0; a < PTargets.Num(); ++a)
		{
			bool bContested = false;
			for (int32 b = 0; b < PTargets.Num(); ++b)
			{
				if (a != b && PFinal[a] == PFinal[b]) { bContested = true; break; }
			}
			if (bContested) { continue; }

			ARTUnit* T = PTargets[a];
			const FRTCellId OldCell = T->Cell;
			const FRTCellId NewCell = PFinal[a];
			AddLogEvent(FString::Printf(TEXT("Trazione: %s -> (q=%d,r=%d,L%d)"),
				*T->GetName(), NewCell.X, NewCell.Y, NewCell.Layer));

			const TArray<FRTCellId> PPath = URTHexLibrary::HexLine(OldCell, NewCell);
			{
				FRTResolvedEvent Ev;
				Ev.Phase = ERTMatchPhase::Blast;
				Ev.Type = ERTResolvedEventType::Move;
				Ev.Source = T;
				Ev.Path = PPath;
				ResolvedTimeline.Add(Ev);
			}

			T->Cell = NewCell;
			T->SetVisualLocation(T->WorldForCell(NewCell, HexOrigin, HexSize, HexLayerH));
			T->PlannedPath.Reset();
			T->PlannedWaypoints.Reset();
			if (T->PlannedCell == OldCell) { T->PlannedCell = NewCell; }
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
		const URTActionData* Ability = Attacker->GetAbility(UsedAbilityIndex[i]);
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

const URTHexMapAsset* ARTTurnManager::GetHexContext(FVector& OutOrigin, float& OutHexSize, float& OutLayerHeight) const
{
	if (const ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(GetWorld()))
	{
		return HexMap->GetHexContext(OutOrigin, OutHexSize, OutLayerHeight);
	}

	// Nessuna mappa nel livello: valori neutri. Le unita' restano dove sono, la scala non ha effetto.
	OutOrigin = FVector::ZeroVector;
	OutHexSize = 100.f;
	OutLayerHeight = 250.f;
	return nullptr;
}

FRTHexSnapshot ARTTurnManager::MakeCurrentSnapshot(TArray<ARTUnit*>& OutUnits) const
{
	OutUnits.Reset();

	FVector Origin; float HexSize; float LayerH;
	const URTHexMapAsset* Map = GetHexContext(Origin, HexSize, LayerH);

	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(const_cast<ARTTurnManager*>(this), ARTUnit::StaticClass(), Actors);
	OutUnits.Reserve(Actors.Num());
	for (AActor* Actor : Actors)
	{
		ARTUnit* Unit = Cast<ARTUnit>(Actor);
		if (Unit && Unit->IsAlive())
		{
			OutUnits.Add(Unit); // i morti (es. nel Blast) non si muovono e non bloccano
		}
	}

	// L'identita' e' l'INDICE dell'unita' in OutUnits, un intero stabile — mai un pointer (stessa
	// disciplina del TurnLog). Il chiamante ritrova la propria unita' con OutUnits.IndexOfByKey.
	TArray<FRTHexSimUnit> SimUnits;
	SimUnits.Reserve(OutUnits.Num());
	for (int32 i = 0; i < OutUnits.Num(); ++i)
	{
		FRTHexSimUnit SimUnit(i, OutUnits[i]->Cell, OutUnits[i]->GetEffectiveMoveRange(), /*bAlive=*/ true);
		// `Action.Slow` (CP 4.7): +1 al costo di ogni cella, letto FRESCO a ogni snapshot — cosi' uno Slow
		// applicato nel Blast (stesso turno) si riflette gia' sulla fase Move che segue, senza bisogno di
		// ricordare "quando" e' stato applicato.
		SimUnit.MoveCostModifier = OutUnits[i]->HasStatus(TAG_Status_Slow) ? 1 : 0;
		SimUnits.Add(SimUnit);
	}
	return URTHexSimLibrary::MakeSnapshot(Map, SimUnits);
}

void ARTTurnManager::ResolveMovement()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector Origin; float HexSize; float LayerH;
	GetHexContext(Origin, HexSize, LayerH);

	TArray<ARTUnit*> Units;
	const FRTHexSnapshot Snapshot = MakeCurrentSnapshot(Units);

	TArray<TArray<FRTCellId>> Paths;
	Paths.Reserve(Units.Num());
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		ARTUnit* Unit = Units[i];

		// Path del turno: percorso composito (waypoint) se presente e coerente, altrimenti rotta calcolata
		// verso la destinazione singola. La validazione e' AUTOREVOLE e passa dallo strato puro esagonale:
		// FindPathForUnit rispetta costi, blocchi, occupazione e budget, a prescindere da cosa arriva dal client.
		TArray<FRTCellId> Path;
		if (Unit->PlannedPath.Num() >= 2 && Unit->PlannedPath[0] == Unit->Cell)
		{
			Path = Unit->PlannedPath;
		}
		else if (Unit->PlannedCell != Unit->Cell)
		{
			Path = URTHexSimLibrary::FindPathForUnit(Snapshot, /*UnitId=*/ i, Unit->PlannedCell).Path;
		}

		// Il percorso e' stato calcolato al momento del click (o impostato direttamente), PRIMA che il Blast
		// di QUESTO turno potesse radicare o rallentare l'unita' (`Action.Root`/`Action.Slow`, CP 4.7). Si
		// TRONCA qui contro il budget FRESCO — non si ricalcola da zero: un ostacolo POSIZIONALE (un'altra
		// unita' che occupa una cella a meta' strada) resta compito di `ResolveHexPaths` sotto, che cammina il
		// percorso passo per passo; qui si intercetta solo "il budget e' cambiato da quando il piano e' stato
		// scritto". Se non e' cambiato, il troncamento non taglia nulla — il percorso `FindPathForUnit` gia'
		// rispettava lo snapshot fresco, quindi qui e' un no-op per costruzione.
		Path = URTHexSimLibrary::TruncatePathToBudget(Snapshot, /*UnitId=*/ i, Path);

		if (Path.Num() < 2)
		{
			Path = { Unit->Cell }; // fermo
		}
		// Ghiaccio: chi finisce il Move su Ice con budget residuo scivola di una cella oltre. La cella extra
		// e' aggiunta al PATH, non alla posizione finale: cosi' occupazione e collisioni simultanee restano
		// affare del microstep di ResolveHexPaths, che le risolve gia' in modo indipendente dall'ordine.
		// Solo il Move normale: le mobilita' lineari (ResolveLinearMove) non passano da qui — §5.2 di
		// spec-terreni-e8.md: senza il microstep condiviso lo scivolamento non avrebbe la stessa garanzia
		// sotto collisione simultanea.
		Path = URTHexSimLibrary::ApplyIceSliding(Snapshot, /*UnitId=*/ i, Path);
		Paths.Add(Path);
	}

	const TArray<FRTHexMoveResult> Resolved = URTHexSimLibrary::ResolveHexPaths(Paths);

	// TurnLog dagli esiti: la chiave e' la cella di PARTENZA (Paths[i][0]), stabile perche' Cell cambia
	// dopo PlaceOnCell. BuildMoveLog produce una voce per unita' nell'ordine dell'input.
	const TArray<FRTTurnLogEntry> MoveLog = URTHexSimLibrary::BuildMoveLog(Paths, Resolved);
	TurnLog.Append(MoveLog);
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		// Il combat log mostra il REASON CODE del TurnLog, con le coordinate assiali: cosi' quel che il
		// giocatore legge e quel che il replay registra sono la stessa cosa, non due descrizioni parallele.
		if (MoveLog.IsValidIndex(i)
			&& (Resolved[i].Outcome == ERTMoveOutcome::BlockedContested || Resolved[i].Outcome == ERTMoveOutcome::BlockedByUnit))
		{
			AddLogEvent(FString::Printf(TEXT("%s: %s"),
				*Units[i]->GetName(), *URTTurnLogLibrary::DescribeEntry(MoveLog[i])));
		}
	}

	// Traccia post-lock: rotte effettivamente percorse (viz del percorso risolto). Catturate PRIMA
	// del placement, cosi' includono la cella di partenza reale.
	LastMoveRoutes.Reset();
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		if (Resolved[i].Entered.Num() > 0)
		{
			TArray<FRTCellId> Route;
			Route.Add(Units[i]->Cell);
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

	// Applica le posizioni finali e gli effetti delle celle ATTRAVERSATE (non solo di quella finale).
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		Units[i]->PlaceOnCell(Resolved[i].Final, Origin, HexSize, LayerH);
		ApplyTerrainOnEnterEffects(Snapshot, Units[i], Resolved[i].Entered);
	}

	// NOTA (CP 8.1): il cross-damage delle celle attraversate esiste di nuovo, ma solo per i terreni che
	// DICHIARANO effetti nel catalogo v0.1 — oggi Fire (10 danni + Burning), ShallowWater (Wet) e Smoke
	// (Obscured). Gli altri cinque non hanno OnEnterEffects: attraversarli non fa nulla, per scelta del
	// catalogo e non per un buco del resolver. Gli hazard di FINE turno (danno periodico a chi sosta) restano
	// fuori: li porta il CP 8.x dedicato, non questo.

	UE_LOG(LogRT, Log, TEXT("[RT] Fase Move: risolte %d unita'"), Units.Num());
}

// ===================== Playback della risoluzione (presentazione) =============================

void ARTTurnManager::BeginPlayback()
{
	// Cache della trasformazione della mappa per convertire celle -> mondo durante il playback.
	// Stessa fonte usata da ResolveMovement: risoluzione e playback non possono divergere di scala.
	GetHexContext(PBOrigin, PBCellSize, PBLayerHeight);

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
			for (const FRTCellId& C : Ev.Path)
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
	// In corsa deve risultare SOLO chi si muove in QUESTA fase. Senza l'azzeramento, un'unita' che ha scattato
	// nel Dash resta con il flag alzato per tutto il Blast (e il Move) e l'AnimBP la tiene in corsa sul posto:
	// il flag lo spegneva solo FinishPlayback, a risoluzione conclusa.
	for (const FRTMoveAnim& A : MoveAnims)
	{
		if (A.Unit.IsValid()) { A.Unit->bIsMovingVisually = false; }
	}

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
			A.Unit->SetVisualLocation(A.Unit->WorldForCell(A.Unit->Cell, PBOrigin, PBCellSize, PBLayerHeight));
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
	PacingCurrent.bPlaybackSkipped = true;
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

// --- Sonda di pacing --------------------------------------------------------------------------
// TELEMETRIA. Nessun valore prodotto qui rientra in una decisione di gioco, nel TurnLog o nel suo hash:
// e' l'unica ragione per cui questo canale puo' permettersi di non essere deterministico.
// Spec: docs/design/spec-pacing-turno.md

void ARTTurnManager::BeginPacingSample()
{
	PacingCurrent = FRTPacingSample();
	PacingCurrent.TurnNumber = TurnNumber;

	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), Actors);
	for (AActor* Actor : Actors)
	{
		const ARTUnit* Unit = Cast<ARTUnit>(Actor);
		if (!Unit || !Unit->IsAlive())
		{
			continue;
		}
		(Unit->TeamId == 0 ? PacingCurrent.UnitsAliveTeam0 : PacingCurrent.UnitsAliveTeam1)++;

		if (Unit->TeamId != PacingTeamId)
		{
			continue; // ActionsAvailable misura lo spazio di decisione di CHI decide, non di tutti
		}
		for (int32 I = 0; I < Unit->NumAbilities(); ++I)
		{
			if (Unit->CanUseAbility(I))
			{
				++PacingCurrent.ActionsAvailable;
			}
		}
	}

	PacingPlanningStart = FPlatformTime::Seconds();
	PacingLastInput = PacingPlanningStart;
	bPacingHadInput = false;
}

void ARTTurnManager::RecordPlanningInput(ERTPlanningInput Kind)
{
	if (Phase != ERTMatchPhase::Planning)
	{
		return; // un input fuori dalla pianificazione non e' una decisione di turno
	}

	const double Now = FPlatformTime::Seconds();
	if (!bPacingHadInput)
	{
		bPacingHadInput = true;
		PacingCurrent.MsToFirstInput = FMath::RoundToInt((Now - PacingPlanningStart) * 1000.0);
	}
	PacingLastInput = Now;

	switch (Kind)
	{
	case ERTPlanningInput::Selection: ++PacingCurrent.SelectionCount; break;
	case ERTPlanningInput::Order:     ++PacingCurrent.OrderCount;     break;
	case ERTPlanningInput::Undo:      ++PacingCurrent.UndoCount;      break;
	case ERTPlanningInput::Click:
	default:
		break; // attivita' generica: aggiorna solo i tempi
	}
}

void ARTTurnManager::ClosePacingSample()
{
	PacingCurrent.MsPlayback = FMath::RoundToInt(PlaybackElapsedTotal * 1000.f);
	PacingSamples.Add(PacingCurrent);
	if (bRecordPacing)
	{
		AppendPacingRow(PacingCurrent);
	}
	PacingCurrent = FRTPacingSample();
}

void ARTTurnManager::AppendPacingRow(const FRTPacingSample& Sample)
{
	if (PacingFilePath.IsEmpty())
	{
		// Un file per esecuzione. Si scrive UNA RIGA PER TURNO: un riavvio della partita non perde nulla.
		const FString Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RT"));
		IFileManager::Get().MakeDirectory(*Dir, /*Tree=*/ true);
		PacingFilePath = FPaths::Combine(Dir,
			FString::Printf(TEXT("pacing_%s.csv"), *FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S"))));
		FFileHelper::SaveStringToFile(URTPacingLibrary::CsvHeader() + LINE_TERMINATOR, *PacingFilePath);
	}
	FFileHelper::SaveStringToFile(URTPacingLibrary::CsvRow(Sample) + LINE_TERMINATOR, *PacingFilePath,
		FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), EFileWrite::FILEWRITE_Append);
}
