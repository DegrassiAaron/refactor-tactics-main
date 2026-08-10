#include "Turn/RTTurnManager.h"
#include "Turn/RTPacingLibrary.h"
#include "Turn/RTPlaybackLibrary.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTActionQueueLibrary.h"
#include "Turn/RTActionEffectLibrary.h"
#include "Turn/RTActionFallbackLibrary.h"
#include "Turn/RTMovementActionLibrary.h"
#include "Turn/RTReactionLibrary.h"
#include "Turn/RTPredictiveLibrary.h" // boundary della Predictive Action (E18): la decisione sta nel puro
#include "Ability/RTCatalogLibrary.h"
#include "Combat/RTCombatResolver.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexArcLibrary.h"
#include "Map/RTHexDoorLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Ability/RTActionData.h"
#include "Bot/RTHexBotLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Turn/RTFacingLibrary.h"
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

TArray<FRTCellId> ARTTurnManager::CellsEnteredAlong(const TArray<FRTCellId>& Path)
{
	// La cella di PARTENZA non e' una cella «entrata»: l'unita' ci stava gia'. Confonderle farebbe applicare
	// due volte gli effetti di quella cella — una all'arrivo e una alla partenza dello spostamento successivo —
	// e per un fuoco significherebbe il doppio del danno dichiarato dal catalogo.
	TArray<FRTCellId> Entered;
	if (Path.Num() < 2) { return Entered; }
	Entered.Reserve(Path.Num() - 1);
	for (int32 I = 1; I < Path.Num(); ++I)
	{
		Entered.Add(Path[I]);
	}
	return Entered;
}

void ARTTurnManager::ApplyTerrainOnEnterEffects(const URTHexMapAsset* Map, ARTUnit* Unit, const TArray<FRTCellId>& Entered)
{
	if (!Map || !Unit) { return; }

	for (const FRTCellId& Cell : Entered)
	{
		const FRTHexCellData* CellData = Map->FindCell(Cell);
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
				// Durata 0 nel catalogo = "finche' sulla cella" (CP 8.2): qui diventa la sentinella che
				// ARTUnit riconosce. La revoca la fa il Cleanup leggendo lo STESSO catalogo, non una
				// seconda tabella (URTTerrainLibrary::CellBoundStatusesFor).
				const int32 Duration = (Effect.StatusDuration == 0)
					? ARTUnit::PersistentWhileOnCell
					: Effect.StatusDuration;
				Unit->ApplyStatus(Effect.StatusTag, Duration);
				AddLogEvent(FString::Printf(TEXT("%s: %s da terreno"), *Unit->GetName(), *Effect.StatusTag.ToString()));
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
				// Alleati: servono a pesare il collaterale di un'area (#213). Il bot NON si conta fra loro
				// perche' `CollectHexAttacks` salta sempre l'attaccante.
				if (Other != Bot)
				{
					Ctx.Allies.Add(Other->Cell);
					Ctx.AllyHealth.Add(Other->Health + Other->Shield);
				}
				continue;
			}
			int32 EnemyReach = Other->AttackRange;
			for (int32 a = 0; a < Other->NumAbilities(); ++a)
			{
				const URTActionData* EAb = Other->GetAbility(a);
				// La minaccia e' cio' che il nemico puo' COLPIRE: una mobilita' rapida sposta, non fa danno a
				// distanza, e contarla gonfierebbe la portata percepita di ogni eroe che ne ha una.
				if (EAb && !URTCatalogLibrary::IsFastMovement(EAb->Def))
				{
					EnemyReach = FMath::Max(EnemyReach, EAb->RangeCells);
				}
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
		const bool bDashReady = DashAb && URTCatalogLibrary::IsFastMovement(DashAb->Def) && Bot->CanUseAbility(DashIdx);

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
		// Anche il GATE "questa e' un'azione di scatto" e' lo stesso (#142): `URTCatalogLibrary::IsFastMovement`
		// legge la fase del catalogo, qui come in ResolveDash. Prima le due risposte divergevano e le azioni
		// degli eroi — che dichiarano la fase e nient'altro — non venivano mai pianificate come scatto.
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
		// Vero se la candidata e' una CARICA: allora si punta la cella del NEMICO (`Ctx.Enemies[TargetIndex]`),
		// non `DestCell` — che per una carica e' dove ci si ferma, cioe' davanti al bersaglio. Serve un flag e
		// non una cella-sentinella: `FRTCellId()` vale (0,0,0), che e' una cella vera della mappa.
		TArray<bool> PlanIsCharge;

		auto AddCandidates = [&](const FRTHexSnapshot& Snap, int32 AbilityIndex, int32 Range, int32 Damage,
			bool bViaDash, bool bAttacksOnly)
		{
			FRTHexBotContext LocalCtx = Ctx;
			LocalCtx.AttackRange = Range;
			LocalCtx.AttackDamage = Damage;

			// Forma dell'azione valutata: senza, ogni attacco verrebbe pesato come un colpo singolo e un'area
			// non mostrerebbe ne' i nemici presi in piu' ne' il compagno investito (#213).
			if (const URTActionData* ShapedAbility = (AbilityIndex != INDEX_NONE) ? Bot->GetAbility(AbilityIndex) : nullptr)
			{
				LocalCtx.AttackShape = ShapedAbility->Shape;
				LocalCtx.AttackAreaRadius = ShapedAbility->AreaRadius;
				LocalCtx.bAttackFriendlyFire = ShapedAbility->Def.bFriendlyFire;
			}
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
				PlanIsCharge.Add(false);
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
			if (!Ability || URTCatalogLibrary::IsFastMovement(Ability->Def) || Ability->bSelfTarget
				|| !Bot->CanUseAbility(A)) { continue; }
			AddCandidates(StaySnapshot, A, Ability->RangeCells, Ability->Power, /*bViaDash*/ false, /*bAttacksOnly*/ true);
		}

		// 3) CARICA: l'unico modo di scattare E colpire nello stesso turno, perche' il danno e' dell'azione di
		// movimento stessa e non di una seconda azione principale (#145). Le candidate non possono nascere da
		// `ReachableCells`: quella cerca celle LIBERE, mentre una carica punta la cella OCCUPATA dal nemico e
		// si ferma davanti. Si generano quindi dai bersagli, chiedendo al resolver se la traiettoria li
		// raggiunge — lo stesso codice che poi la eseguira'.
		if (bDashReady && DashStyle == ERTMovementStyle::LinearCharge)
		{
			const int32 ImpactDamage = URTCatalogLibrary::FirstDamage(DashAb->Def);
			for (int32 e = 0; e < Ctx.Enemies.Num(); ++e)
			{
				const FRTLinearMoveResult Linear = URTMovementActionLibrary::ResolveLinearMove(
					Snapshot.Map, Bot->Cell, Ctx.Enemies[e], DashBudget, DashStyle, Snapshot.Occupancy, DashHostiles);

				// Vale solo se l'impatto colpisce PROPRIO quel nemico: una traiettoria che ne incontra un altro
				// prima e' una candidata diversa, e la genera il suo giro di ciclo.
				if (Linear.Stop != ERTLinearStop::Impact
					|| !EnemyUnitIndex.IsValidIndex(e) || Units[EnemyUnitIndex[e]] != Units[Linear.ImpactUnitId])
				{
					continue;
				}

				FRTHexBotPlan Charge;
				Charge.DestCell = Linear.Final;   // dove il bot si ferma: adiacente al bersaglio
				Charge.bHasAttack = true;
				Charge.TargetIndex = e;
				Charge.AttackDamage = ImpactDamage;
				Charge.TargetHealth = Ctx.EnemyHealth.IsValidIndex(e) ? Ctx.EnemyHealth[e] : 0;
				Plans.Add(Charge);
				PlanAbility.Add(INDEX_NONE);      // il colpo NON e' una seconda azione: e' l'impatto della carica
				PlanViaDash.Add(true);
				PlanIsCharge.Add(true);
			}
		}

		// 4) Scatto + attacco, e scatto per riposizionarsi.
		//
		// NOTA (#145, aggiornata da D-028): scatto e attacco sono ora slot DIVERSI — movimento e principale —
		// quindi pianificarli insieme e' legale, ed e' la scelta *schivo e sparo*. Il prezzo c'e' e non e' piu'
		// implicito: chi scatta non prosegue col Move (lo applica il resolver piu' sotto), chi carica si.
		//
		// Resta il problema di bilanciamento che la nota segnalava, e resta misurato sugli ARCHETIPI: per il
		// Guardian «scatto + Sweep» fa 30 danni e spinta 2 con cooldown 0, la Charge 20 e spinta 1 con
		// cooldown 3. Sul roster eroi i numeri sono altri. Il meccanismo qui sopra e' corretto; a renderlo
		// utile e' il bilanciamento — voce `BAL-1` del backlog, che parte da una misura e non da una correzione.
		if (bDashReady)
		{
			for (int32 A = 0; A < Bot->NumAbilities(); ++A)
			{
				const URTActionData* Ability = Bot->GetAbility(A);
				if (!Ability || URTCatalogLibrary::IsFastMovement(Ability->Def) || Ability->bSelfTarget
					|| !Bot->CanUseAbility(A)) { continue; }
				AddCandidates(DashSnapshot, A, Ability->RangeCells, Ability->Power, /*bViaDash*/ true, /*bAttacksOnly*/ true);
			}
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
		const bool bIsCharge = Plans.IsValidIndex(BestIdx) && PlanIsCharge[BestIdx];
		const int32 BestAbility = Plans.IsValidIndex(BestIdx) ? PlanAbility[BestIdx] : INDEX_NONE;
		ARTUnit* Target = (Best.bHasAttack && EnemyUnitIndex.IsValidIndex(Best.TargetIndex))
			? Units[EnemyUnitIndex[Best.TargetIndex]] : nullptr;
		const int32 Score = URTHexBotLibrary::ScorePlan(Snapshot.Map, Best, Ctx);

		if (bIsCharge && Target && Ctx.Enemies.IsValidIndex(Best.TargetIndex))
		{
			// CARICA: si punta la cella del bersaglio e la fase Dash si ferma addosso a lui registrando
			// l'impatto. Nessun `PlannedAbilityIndex`: il colpo e' dell'azione di movimento, e pianificare
			// anche un'azione principale significherebbe spendere due volte lo stesso slot.
			Bot->PlannedDashAbility = DashIdx;
			Bot->PlannedDashCell = Ctx.Enemies[Best.TargetIndex];
			AddLogEvent(FString::Printf(TEXT("%s: utility -> CARICA su %s (impatto da (q=%d,r=%d,L%d)) score=%d"),
				*Bot->GetName(), *Target->GetName(), Best.DestCell.X, Best.DestCell.Y, Best.DestCell.Layer, Score));
		}
		else if (bViaDash && Target && BestAbility != INDEX_NONE)
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

void ARTTurnManager::SetPlanningSeconds(float NewSeconds)
{
	PlanningSeconds = FMath::Max(0.f, NewSeconds);

	// Se la pianificazione e' GIA' in corso, il timer va rifatto: cambiare solo il campo lascerebbe scorrere
	// quello vecchio, e il valore nuovo varrebbe dal turno dopo — cioe' proprio nel turno in cui serviva non
	// farebbe niente.
	UWorld* World = GetWorld();
	if (World && Phase == ERTMatchPhase::Planning && !bIsResolving)
	{
		World->GetTimerManager().ClearTimer(PlanningTimerHandle);
		if (PlanningSeconds > 0.f)
		{
			World->GetTimerManager().SetTimer(PlanningTimerHandle, this,
				&ARTTurnManager::OnPlanningTimeout, PlanningSeconds, false);
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

	// Fase Cleanup, nell'ordine fissato da `spec-stati-temporanei-cp82.md` §4: revoca degli stati legati alla
	// cella -> scadenza delle durate -> energia/scudo/cooldown -> conteggio delle unita' vive.
	// La revoca precede il tick perche' le due nature non si sovrappongono: chi ha lasciato l'acqua si asciuga
	// in QUESTO Cleanup, senza aspettare un turno.
	ARTHexMapActor* MapActor = ARTHexMapActor::FindInWorld(GetWorld());
	URTHexMapAsset* CleanupMap = MapActor ? MapActor->MapAsset : nullptr;

	// 0. Scadenza delle modifiche ambientali dei turni PRECEDENTI (CP 8.4). Precede le azioni di questo turno
	// per una ragione di durata: una cella incendiata adesso deve bruciare per i suoi due turni pieni: se il
	// tick venisse dopo, le mangerebbe subito uno.
	TickDynamicSurfaces(CleanupMap);

	// Stessa ragione di ordine delle superfici: un ponte creato ADESSO deve durare i suoi turni pieni, quindi
	// il tick precede le azioni di questo turno invece di mangiarne subito uno.
	TickDynamicArcs(CleanupMap);

	// Le coperture temporanee (CP 9.5) scalano invece GIA' in questo Cleanup, e non e' un'incoerenza con le due
	// righe qui sopra: un pannello nasce in **Prep**, cioe' prima del Blast che lo usa, quindi il turno in cui
	// e' stato eretto e' un turno in cui ha gia' riparato qualcuno. Saltarlo gliene regalerebbe uno.
	TickDynamicCovers(CleanupMap);

	// 1. Azioni ambientali (CP 8.3/8.4): la scarica elettrica e le modifiche del terreno precedono il danno di
	// `Burning`. Chi cade qui e' morto in QUESTO turno, come chi cade bruciato: il conteggio dei vivi arriva
	// dopo entrambi.
	ResolveEnvironment(CleanupMap);

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

			// 1. Effetti ambientali PRIMA dei KO (ADR-0003 §3): chi muore bruciato muore in questo turno.
			// Passa dalla contabilita' del danno di gioco (ApplyDamage + ApplyCombatState), quindi erode
			// prima lo scudo TEMPORANEO — che infatti scade solo piu' sotto, non prima.
			if (Unit->HasStatus(TAG_Status_Burning))
			{
				const FRTDamageResult Burn = URTCombatLibrary::ApplyDamage(
					URTCombatLibrary::BurningCleanupDamage, Unit->Shield, Unit->Health);
				Unit->ApplyCombatState(Burn.Health, Burn.Shield);
				AddLogEvent(FString::Printf(TEXT("%s: %d danni da Status.Burning (q=%d,r=%d,L%d)"),
					*Unit->GetName(), URTCombatLibrary::BurningCleanupDamage,
					Unit->Cell.X, Unit->Cell.Y, Unit->Cell.Layer));

				if (!Unit->IsAlive())
				{
					// L'eliminazione da hazard non ha un beat di playback (la timeline e' gia' chiusa):
					// la nasconde il catch-all di ConcludeTurn, che esiste proprio per questo caso.
					AddLogEvent(FString::Printf(TEXT("%s eliminato dalle fiamme"), *Unit->GetName()));
					continue; // morto adesso: non guadagna energia, non conta fra i vivi
				}
			}

			// 2. Senza mappa autorevole non si revoca nulla: cancellare gli stati sarebbe inventare che la
			// cella non li sostiene, quando in realta' non la si e' potuta leggere.
			if (CleanupMap)
			{
				const FRTHexCellData* CellData = CleanupMap->FindCell(Unit->Cell);
				Unit->RevokeCellBoundStatusesNotIn(CellData
					? URTTerrainLibrary::CellBoundStatusesFor(CellData->Surface)
					: TSet<FGameplayTag>());
			}

			Unit->Energy = URTCombatLibrary::GainEnergy(Unit->Energy, Unit->EnergyPerTurn, Unit->MaxEnergy);
			Unit->ExpireTemporaryShield(); // la protezione delle abilita' di supporto vale un turno solo
			Unit->TickStatuses();
			Unit->TickCooldowns();
			(Unit->TeamId == 0 ? Team0Alive : Team1Alive)++;
		}
	}

	// Fine partita a tre vie (CP 10.3), valutata QUI: nel Cleanup, dopo gli effetti ambientali e i KO, e
	// fuori dai resolver puri — nessuno di loro consulta il formato, e passarglielo sarebbe un parametro che
	// nessuna funzione legge (spec §16.3).
	FRTMatchState MatchState;
	MatchState.Team0Alive = Team0Alive;
	MatchState.Team1Alive = Team1Alive;
	MatchState.Team0Score = Team0Score;
	MatchState.Team1Score = Team1Score;
	MatchState.RoundNumber = TurnNumber;
	PendingResult = URTTurnRules::EvaluateMatchEnd(MatchState, MatchRules);

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

void ARTTurnManager::AppendDisplacementEntry(const ARTUnit* Target, const FRTCellId& From, const FRTCellId& To,
	int32 Steps, const TMap<ARTUnit*, FName>& CauseById, const TMap<ARTUnit*, FName>& CauseByBaseId)
{
	FRTTurnLogEntry Entry;
	// Fase `Blast`: lo spostamento forzato avviene dove avviene il colpo che lo produce, non nella fase Move.
	// E' quello che rende leggibile «sono stato spostato PRIMA di potermi muovere».
	Entry.Phase = ERTMatchPhase::Blast;
	Entry.Category = ERTLogCategory::Move;
	Entry.Outcome = static_cast<uint8>(ERTMoveOutcome::Displaced);
	Entry.SrcCell = From;
	Entry.TgtCell = To;
	Entry.Amount = FMath::Max(0, Steps);

	// La chiave delle due mappe e' il puntatore non-const del bersaglio: qui si arriva con un const, e
	// `const_cast` sul solo scopo di CERCARE non muta nulla — la mappa e' const anch'essa.
	ARTUnit* Key = const_cast<ARTUnit*>(Target);
	if (const FName* Cause = CauseById.Find(Key))         { Entry.ActionId = *Cause; }
	if (const FName* Base  = CauseByBaseId.Find(Key))     { Entry.BaseActionId = *Base; }

	TurnLog.Add(Entry);
}

void ARTTurnManager::ApplyPlannedHeals(const TArray<ARTUnit*>& Targets, const TArray<int32>& Amounts,
	const TArray<FRTCellId>& Sources)
{
	// Tre regole del catalogo, tutte verificabili: non supera la salute massima · non rimuove stati (si tocca
	// solo `Health`) · **non resuscita** chi e' caduto in questo turno — una cura che riportasse in piedi
	// un'unita' a zero renderebbe il KO reversibile, che e' una regola diversa e non dichiarata da nessuna parte.
	for (int32 h = 0; h < Targets.Num(); ++h)
	{
		ARTUnit* HealTarget = Targets[h];
		if (!HealTarget || !HealTarget->IsAlive()) { continue; }

		const int32 Before = HealTarget->Health;
		HealTarget->ApplyCombatState(FMath::Min(HealTarget->MaxHealth, Before + Amounts[h]), HealTarget->Shield);
		const int32 Restored = HealTarget->Health - Before;

		FRTTurnLogEntry Entry;
		Entry.Phase = ERTMatchPhase::Blast;
		Entry.Category = ERTLogCategory::Combat;
		Entry.Outcome = static_cast<uint8>(ERTCombatOutcome::Healed);
		Entry.ActionId = FName(TEXT("Action.Heal"));
		Entry.SrcCell = Sources.IsValidIndex(h) ? Sources[h] : HealTarget->Cell;
		Entry.TgtCell = HealTarget->Cell;
		Entry.Amount = Restored; // quanto e' stato curato DAVVERO: a salute piena la voce dice zero
		TurnLog.Add(Entry);
		AddLogEvent(FString::Printf(TEXT("%s: +%d salute"), *HealTarget->GetName(), Restored));
	}
}

bool ARTTurnManager::ApplyDynamicSurface(URTHexMapAsset* Map, const FRTCellId& Cell, ERTHexSurface NewSurface,
	int32 Turns, const FName& CauseActionId)
{
	const FRTHexCellData* Existing = Map ? Map->FindCell(Cell) : nullptr;
	if (!Existing || Turns <= 0)
	{
		return false; // fuori mappa o durata non positiva: non si modifica il campo per sbaglio
	}
	if (Existing->Surface == NewSurface)
	{
		return false; // gia' cosi': nessun cambiamento da registrare
	}

	FRTTurnLogEntry Entry;
	Entry.Phase = ERTMatchPhase::Cleanup;
	Entry.Category = ERTLogCategory::Environment;
	Entry.ActionId = CauseActionId;
	Entry.SrcCell = Cell;
	Entry.TgtCell = Cell;

	// Il fuoco non attecchisce su cio' che non brucia (catalogo terreni §2: «non incendia automaticamente
	// acqua o metallo»). E' una proprieta' della superficie di DESTINAZIONE, letta dal catalogo — non un
	// elenco di eccezioni scritto qui.
	if (NewSurface == ERTHexSurface::Fire && !URTTerrainLibrary::FindTerrainDef(Existing->Surface).bIsFlammable)
	{
		Entry.Outcome = static_cast<uint8>(ERTEnvironmentOutcome::SurfaceRejected);
		Entry.Amount = 0;
		TurnLog.Add(Entry);
		AddLogEvent(FString::Printf(TEXT("(q=%d,r=%d,L%d): non prende fuoco"), Cell.X, Cell.Y, Cell.Layer));
		return false;
	}

	// L'acqua SPEGNE il fuoco: e' la stessa trasformazione, ma il TurnLog la distingue perche' per chi legge
	// il replay «la cella si allaga» e «la cella si spegne» non sono lo stesso evento.
	const bool bExtinguishes = (Existing->Surface == ERTHexSurface::Fire
		&& NewSurface == ERTHexSurface::ShallowWater);

	// L'ORIGINALE si registra una volta sola: una cella allagata e poi incendiata deve tornare al pavimento,
	// non all'acqua che c'era un turno prima.
	FRTDynamicSurface& State = DynamicSurfaces.FindOrAdd(Cell);
	if (State.TurnsRemaining <= 0)
	{
		State.Original = Existing->Surface;
	}
	State.TurnsRemaining = Turns;

	// La superficie corrente va nella MAPPA, che e' cio' che tutti leggono: il costo di movimento segue il
	// catalogo della nuova superficie, altrimenti una pozza costerebbe ancora quanto il pavimento.
	FRTHexCellData Updated = *Existing;
	Updated.Surface = NewSurface;
	Updated.MoveCost = URTTerrainLibrary::FindTerrainDef(NewSurface).MoveCost;
	Map->AddOrUpdateCell(Updated);

	Entry.Outcome = static_cast<uint8>(
		bExtinguishes ? ERTEnvironmentOutcome::SurfaceExtinguished : ERTEnvironmentOutcome::SurfaceChanged);
	Entry.Amount = Turns;
	TurnLog.Add(Entry);
	AddLogEvent(FString::Printf(TEXT("(q=%d,r=%d,L%d): %s (%d turni)"), Cell.X, Cell.Y, Cell.Layer,
		bExtinguishes ? TEXT("il fuoco si spegne") : TEXT("la superficie cambia"), Turns));
	return true;
}

void ARTTurnManager::TickDynamicArcs(URTHexMapAsset* Map)
{
	if (!Map || DynamicArcs.Num() == 0) { return; }

	// Ordine STABILE: da qui escono voci di TurnLog, e due esecuzioni della stessa partita devono scriverle
	// nella stessa sequenza (la stessa disciplina di `TickDynamicSurfaces`).
	DynamicArcs.Sort([](const FRTDynamicArc& A, const FRTDynamicArc& B)
	{
		if (!(A.From == B.From)) { return URTHexLibrary::StableLess(A.From, B.From); }
		return URTHexLibrary::StableLess(A.To, B.To);
	});

	for (int32 I = DynamicArcs.Num() - 1; I >= 0; --I)
	{
		FRTDynamicArc& Arc = DynamicArcs[I];
		if (Arc.CreatedOnTurn == TurnNumber)
		{
			continue; // nato in questo turno: i suoi turni cominciano dal prossimo, non da adesso
		}
		if (--Arc.TurnsRemaining > 0)
		{
			continue; // il ponte regge ancora
		}

		// Scaduto: l'arco sparisce dalla mappa. I due layer tornano irraggiungibili e il percorso FALLISCE —
		// il Move del turno successivo lo scopre da `GraphNeighbors`, non da un secondo controllo qui.
		const FRTCellId From = Arc.From;
		const FRTCellId To = Arc.To;
		DynamicArcs.RemoveAt(I);
		if (!Map->RemoveTransition(From, To, /*bBothDirections*/ true))
		{
			continue; // gia' tolto da un `ModifyArc`: niente da registrare
		}
		// NOTA (#302): questo ramo NON e' la rete di sicurezza per i ponti distrutti in combattimento, e
		// crederlo e' costato un difetto. `DamageArc` non rimuove l'arco — lo marca `Destroyed` e lo lascia
		// sulla mappa — quindi su un ponte crollato `RemoveTransition` RIUSCIREBBE, e da qui uscirebbe un
		// `BridgeRemoved` per un evento mai avvenuto. Un ponte abbattuto non arriva piu' fin qui: la sua entry
		// e' tolta da `DynamicArcs` nel momento del crollo.

		FRTTurnLogEntry Entry;
		Entry.Phase = ERTMatchPhase::Cleanup;
		Entry.Category = ERTLogCategory::Environment;
		Entry.Outcome = static_cast<uint8>(ERTEnvironmentOutcome::BridgeRemoved);
		Entry.ActionId = FName(TEXT("Action.ModifyArc"));
		Entry.SrcCell = From;
		Entry.TgtCell = To;
		Entry.Amount = 0;
		TurnLog.Add(Entry);
		AddLogEvent(FString::Printf(TEXT("Il ponte (q=%d,r=%d,L%d) -> (q=%d,r=%d,L%d) e' scaduto"),
			From.X, From.Y, From.Layer, To.X, To.Y, To.Layer));
	}
}

void ARTTurnManager::TickDynamicCovers(URTHexMapAsset* Map)
{
	if (!Map || DynamicCovers.Num() == 0) { return; }

	// Ordine STABILE: da qui escono voci di TurnLog, e due esecuzioni della stessa partita devono scriverle
	// nella stessa sequenza (la stessa disciplina di `TickDynamicArcs`).
	DynamicCovers.Sort([](const FRTDynamicCover& A, const FRTDynamicCover& B)
	{
		if (!(A.Cell == B.Cell)) { return URTHexLibrary::StableLess(A.Cell, B.Cell); }
		return static_cast<uint8>(A.Edge) < static_cast<uint8>(B.Edge);
	});

	for (int32 I = DynamicCovers.Num() - 1; I >= 0; --I)
	{
		FRTDynamicCover& Cover = DynamicCovers[I];
		if (Cover.TurnsRemaining <= 0)
		{
			continue; // non scade da sola (pannello adattivo): resta finche' qualcuno non l'abbatte
		}
		if (--Cover.TurnsRemaining > 0)
		{
			continue; // il pannello regge ancora
		}

		const FRTCellId Cell = Cover.Cell;
		const ERTHexDirection Edge = Cover.Edge;
		DynamicCovers.RemoveAt(I);

		// La cella OLTRE il bordo: il TurnLog identifica una struttura con la coppia di celle, come per
		// coperture danneggiate, porte e ponti — nessun campo direzione nel log.
		const TArray<FRTCellId> Ring = URTHexLibrary::Neighbors(Cell);
		const int32 EdgeIndex = static_cast<int32>(Edge);
		const FRTCellId Toward = Ring.IsValidIndex(EdgeIndex) ? Ring[EdgeIndex] : Cell;

		if (!URTHexCoverLibrary::RemoveCover(Map, Cell, Edge))
		{
			continue; // gia' sparita per altra via (abbattuta in combattimento): niente da registrare
		}

		FRTTurnLogEntry Entry;
		Entry.Phase = ERTMatchPhase::Cleanup;
		Entry.Category = ERTLogCategory::Environment;
		Entry.Outcome = static_cast<uint8>(ERTEnvironmentOutcome::CoverExpired);
		Entry.ActionId = FName(TEXT("Action.CreateCover"));
		Entry.SrcCell = Cell;
		Entry.TgtCell = Toward;
		Entry.Amount = 0;
		TurnLog.Add(Entry);
		AddLogEvent(FString::Printf(TEXT("La copertura (q=%d,r=%d,L%d) e' scaduta"),
			Cell.X, Cell.Y, Cell.Layer));
	}
}

void ARTTurnManager::TickDynamicSurfaces(URTHexMapAsset* Map)
{
	if (!Map || DynamicSurfaces.Num() == 0) { return; }

	// Ordine STABILE: `TMap` non ha un ordine garantito, e da qui escono voci di TurnLog. Senza questo, due
	// esecuzioni della stessa partita produrrebbero lo stesso insieme di voci in ordine diverso — l'hash e'
	// permutazione-invariante e non se ne accorgerebbe, ma il combat log letto da un umano si', e il giorno in
	// cui qualcosa dipendesse dall'ordine il difetto sarebbe gia' dentro.
	TArray<FRTCellId> Cells;
	DynamicSurfaces.GetKeys(Cells);
	Cells.Sort([](const FRTCellId& A, const FRTCellId& B) { return URTHexLibrary::StableLess(A, B); });

	for (const FRTCellId& Cell : Cells)
	{
		FRTDynamicSurface& State = DynamicSurfaces[Cell];
		if (--State.TurnsRemaining > 0)
		{
			continue;
		}

		if (const FRTHexCellData* Current = Map->FindCell(Cell))
		{
			FRTHexCellData Restored = *Current;
			Restored.Surface = State.Original;
			Restored.MoveCost = URTTerrainLibrary::FindTerrainDef(State.Original).MoveCost;
			Map->AddOrUpdateCell(Restored);

			FRTTurnLogEntry Entry;
			Entry.Phase = ERTMatchPhase::Cleanup;
			Entry.Category = ERTLogCategory::Environment;
			Entry.Outcome = static_cast<uint8>(ERTEnvironmentOutcome::SurfaceRestored);
			Entry.SrcCell = Cell;
			Entry.TgtCell = Cell;
			Entry.Amount = 0;
			TurnLog.Add(Entry);
			AddLogEvent(FString::Printf(TEXT("(q=%d,r=%d,L%d): la superficie torna com'era"),
				Cell.X, Cell.Y, Cell.Layer));
		}
		DynamicSurfaces.Remove(Cell);
	}
}

void ARTTurnManager::ResolveEnvironment(URTHexMapAsset* Map)
{
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
	if (Units.Num() == 0) { return; }
	// Stesso ordine stabile per cella del resto del turno: da qui dipendono gli indici passati alla libreria
	// e l'ordine in cui due scariche dello stesso turno si applicano.
	Units.Sort([](const ARTUnit& A, const ARTUnit& B) { return URTHexLibrary::StableLess(A.Cell, B.Cell); });

	// Snapshot delle unita' PRIMA di applicare qualunque danno: "raccogli poi applica" (invariante #3). Due
	// scariche nello stesso Cleanup vedono lo stesso campo, quindi il loro esito non dipende dall'ordine.
	TArray<FRTHexCombatUnit> HexUnits;
	HexUnits.Reserve(Units.Num());
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		FRTHexCombatUnit HexUnit;
		HexUnit.UnitId = i;
		HexUnit.TeamId = Units[i]->TeamId;
		HexUnit.Cell = Units[i]->Cell;
		HexUnit.bAlive = Units[i]->IsAlive();
		HexUnits.Add(HexUnit);
	}

	for (int32 i = 0; i < Units.Num(); ++i)
	{
		ARTUnit* Caster = Units[i];
		const int32 AbilityIndex = Caster->PlannedAbilityIndex;
		const URTActionData* Ability = Caster->GetAbility(AbilityIndex);
		if (!Ability
			|| URTCatalogLibrary::MapResolutionPhase(Ability->Def.ResolutionPhase) != ERTMatchPhase::Cleanup)
		{
			continue; // nessuna azione ambientale pianificata da questa unita'
		}

		ARTUnit* Target = Caster->PlannedAttackTarget;
		Caster->PlannedAbilityIndex = INDEX_NONE; // consumato: attivata o no, il piano non sopravvive al turno
		Caster->PlannedAttackTarget = nullptr;
		if (!Caster->CanUseAbility(AbilityIndex)) { continue; }

		// Il fallback dichiarato di `Action.Electrify` e' `Cancel`: senza bersaglio valido non succede nulla,
		// e non si sceglie un bersaglio di ripiego (il catalogo vieta le scelte implicite).
		if (!Target || !Target->IsAlive())
		{
			AddLogEvent(FString::Printf(TEXT("%s: %s annullata (nessun bersaglio)"),
				*Caster->GetName(), *Ability->Def.ActionId.ToString()));
			continue;
		}

		// Azioni che modificano la MAPPA (CP 8.4). Quale superficie creano lo dice l'ActionId, ed e' l'unico
		// punto in cui questo orchestratore lo guarda: la coppia azione->superficie non e' esprimibile come
		// `FRTActionEffectSpec` (gli effetti agiscono su unita', non su celle) e inventare un campo
		// «SurfaceCreated» nel catalogo per due sole azioni sarebbe un dato che nessun'altra azione useria.
		// Quando le azioni ambientali saranno molte (CP 8.5), il posto giusto e' quel campo.
		{
			// Dal DATO, non dal nome (D-046, #282). Il confronto per ActionId funzionava finche' le uniche
			// azioni ambientali erano quelle del catalogo core; non appena un eroe ne possiede una — e
			// `Riva.FluidTrail` non puo' chiamarsi `Action.CreateWater` — un `if` sul nome smette di poter
			// esprimere «e' quell'azione». Stessa strada di `PropagationLimit`, che infatti gia' funzionava
			// per l'eroe che aveva ereditato `Action.Electrify`.
			const bool bCreatesSurface = Ability->Def.bCreatesSurface;
			const ERTHexSurface Created = Ability->Def.SurfaceCreated;
			// La CAUSA registrata nel TurnLog resta l'ActionId dell'abilita' usata: con un eroe owner e'
			// `Riva.FluidTrail`, non `Action.CreateWater`. Chi legge il replay deve vedere CHI ha allagato,
			// non la primitiva che c'e' sotto — l'identita' dell'eroe e' meta' del valore del log.
			const FName EnvActionId = Ability->Def.ActionId;

			if (bCreatesSurface)
			{
				Caster->ConsumeAbility(AbilityIndex);
				// Durata 2 turni per entrambe (catalogo terreni §2 per il fuoco, catalogo azioni §6 per l'acqua).
				// La cella e' quella del bersaglio, come per la scarica: stesso limite dichiarato sul
				// targeting per cella.
				//
				// Il raggio e' dell'AZIONE, non della superficie. Era `(Created == ShallowWater) ? 1 : 0`, cioe'
				// il ramo che il commento qui sopra dichiara di voler evitare: con tre produttori — acqua 1,
				// fuoco 0, fumo 1 (`Riva.MistVeil`, #353) — quel ramo dovrebbe indovinare quale superficie
				// vuole quale area, e sbaglierebbe alla prima azione che allaga una cella sola.
				const int32 Radius = FMath::Max(0, Ability->Def.SurfaceRadius);
				// Ordine STABILE delle celle: `HexArea` restituisce gia' un'area ordinata, quindi le voci di
				// TurnLog escono sempre nella stessa sequenza (#4).
				for (const FRTCellId& Cell : URTHexLibrary::HexArea(Target->Cell, Radius))
				{
					if (!ApplyDynamicSurface(Map, Cell, Created, /*Turns*/ 2, EnvActionId))
					{
						continue; // cella fuori mappa, gia' cosi', o che non ammette la trasformazione
					}
					// Le unita' GIA' presenti si bagnano subito: gli `OnEnterEffects` valgono per chi ENTRA, e
					// aspettare che escano e rientrino per applicare `Wet` sarebbe una regola che nessuno
					// capirebbe guardando il campo.
					if (Created == ERTHexSurface::ShallowWater)
					{
						for (ARTUnit* Occupant : Units)
						{
							if (Occupant && Occupant->IsAlive() && Occupant->Cell == Cell)
							{
								Occupant->ApplyStatus(TAG_Status_Wet, ARTUnit::PersistentWhileOnCell);
							}
						}
					}
				}
				continue;
			}

			// `Action.ModifyArc` NON passa piu' di qui: dal CP 9.4 risolve nel **Blast** insieme a porte e
			// strutture, perche' la topologia deve cambiare tutta nello stesso momento e il Move che segue
			// deve vederla. Il suo ramo vive in `ResolveCombat`.
		}

		// La SORGENTE e' la cella del bersaglio, non l'unita': l'elettricita' entra nel terreno e da li' si
		// propaga. **Limite dichiarato (CP 8.3)**: il catalogo prevede anche «colpisce una cella conduttiva»
		// senza unita' sopra, ma la pianificazione non ha ancora un bersaglio-cella per le azioni
		// (`PlannedAttackTarget` e' un'unita'); arrivera' col targeting per cella dell'HUD (E11).
		const int32 InitialDamage = URTCatalogLibrary::FirstDamage(Ability->Def);
		const TArray<FRTPropagationHit> Hits = URTTerrainLibrary::CollectElectricPropagation(
			Map, Target->Cell, Ability->Def.PropagationLimit, InitialDamage,
			URTCombatLibrary::PropagatedElectricDamage, HexUnits);

		Caster->ConsumeAbility(AbilityIndex);
		if (Hits.Num() == 0)
		{
			// Nessun colpo: senza mappa autorevole (fail-closed) o con il bersaglio ormai fuori dallo snapshot.
			AddLogEvent(FString::Printf(TEXT("%s: %s senza effetto"),
				*Caster->GetName(), *Ability->Def.ActionId.ToString()));
			continue;
		}

		// APPLICA nell'ordine dichiarato dalla libreria (distanza -> cella -> unita'), che e' anche l'ordine
		// in cui le voci finiscono nel TurnLog: il replay racconta la scarica come si e' propagata.
		for (const FRTPropagationHit& Hit : Hits)
		{
			if (!Units.IsValidIndex(Hit.UnitId) || !Units[Hit.UnitId] || !Units[Hit.UnitId]->IsAlive())
			{
				continue;
			}
			ARTUnit* Victim = Units[Hit.UnitId];
			const FRTDamageResult Result = URTCombatLibrary::ApplyDamage(Hit.Damage, Victim->Shield, Victim->Health);
			Victim->ApplyCombatState(Result.Health, Result.Shield);

			FRTTurnLogEntry Entry;
			Entry.Phase = ERTMatchPhase::Cleanup;
			Entry.Category = ERTLogCategory::Combat;
			Entry.ActionId = Ability->Def.ActionId; // identita' dell'azione: un danno senza causa e' inspiegabile
			Entry.BaseActionId = Ability->Def.BaseActionId; // di quale generica e' un profilo (D-033, #354)
			Entry.SrcCell = Caster->Cell;
			Entry.TgtCell = Hit.Cell;
			Entry.Amount = Hit.Damage;
			Entry.Outcome = static_cast<uint8>(
				!Victim->IsAlive() ? ERTCombatOutcome::Lethal
				: (Result.Health == Victim->MaxHealth || Hit.Damage <= 0) ? ERTCombatOutcome::ShieldAbsorbed
				: ERTCombatOutcome::Hit);
			TurnLog.Add(Entry);

			AddLogEvent(FString::Printf(TEXT("%s: %d danni da %s (%d %s)"),
				*Victim->GetName(), Hit.Damage, *Ability->Def.ActionId.ToString(),
				Hit.Steps, Hit.Steps == 0 ? TEXT("colpo diretto") : TEXT("celle di propagazione")));
			if (!Victim->IsAlive())
			{
				AddLogEvent(FString::Printf(TEXT("%s eliminato dalla scarica"), *Victim->GetName()));
			}
		}
	}
}

void ARTTurnManager::ConcludeTurn()
{
	// PRIMA di tutto il resto: a partita finita questa funzione esce anticipatamente, e il turno che
	// decide la partita e' proprio quello che non verrebbe mai misurato.
	ClosePacingSample();

	// Morte visiva differita: ora che il playback ha mostrato le eliminazioni, rimuovi gli Actor morti
	// (prima del prossimo turno, cosi' non figurano piu' come bersagli/ostacoli).
	DestroyDefeatedUnits();

	if (PendingResult.Outcome != ERTMatchOutcome::InProgress)
	{
		Phase = ERTMatchPhase::MatchEnded;

		// Esito E via: "vince il team 0" e' la stessa frase per un'eliminazione e per un punto di vantaggio
		// allo scadere dei round, e senza la via il log non permetterebbe di distinguerle (DoD di CP 10.3).
		AddLogEvent(FString::Printf(TEXT("Partita finita: %s - %s (round %d/%d, obiettivo %d-%d, formato %s)"),
			*URTTurnRules::DescribeOutcome(PendingResult.Outcome),
			*URTTurnRules::DescribeEndReason(PendingResult.Reason),
			TurnNumber,
			MatchRules.RoundLimit,
			Team0Score,
			Team1Score,
			*MatchRules.FormatId.ToString()));
		return; // niente nuovo turno
	}

	++TurnNumber;

	// Riavvia la pianificazione del nuovo turno.
	StartPlanningTimer();
}

int32 ARTTurnManager::GetTeamScore(int32 TeamId) const
{
	if (TeamId == 0) { return Team0Score; }
	if (TeamId == 1) { return Team1Score; }
	return 0;
}

void ARTTurnManager::AddTeamScore(int32 TeamId, int32 Points)
{
	if (TeamId == 0)
	{
		Team0Score += Points;
	}
	else if (TeamId == 1)
	{
		Team1Score += Points;
	}
	else
	{
		// Il 2v2 ha due squadre: un punto assegnato a una terza sparirebbe senza che nessuno lo noti, e
		// l'esito della partita dipende da questi numeri.
		UE_LOG(LogRT, Warning, TEXT("[RT] Punteggio %d assegnato alla squadra %d, che non esiste: ignorato"),
			Points, TeamId);
		return;
	}

	AddLogEvent(FString::Printf(TEXT("Obiettivo: team %d +%d (ora %d-%d)"),
		TeamId, Points, Team0Score, Team1Score));
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

int32 ARTTurnManager::ResolveCoverStructures(const TArray<ARTUnit*>& Units)
{
	ARTHexMapActor* MapActor = ARTHexMapActor::FindInWorld(GetWorld());
	URTHexMapAsset* Map = MapActor ? MapActor->MapAsset : nullptr;

	// Che cosa si e' chiesto alla mappa, gia' validato contro il PIANO (bordo dichiarato, bersaglio, portata).
	// Si raccoglie tutto prima di scrivere, come per `PendingArcOps` nel Blast: due pannelli sullo stesso bordo
	// nello stesso turno non possono coesistere, e chi vince deve dipendere da un ordine stabile e non
	// dall'ordine in cui `GetAllActorsOfClass` ha restituito gli attori.
	struct FRTPendingCoverOp
	{
		FRTCellId Cell;
		ERTHexDirection Edge = ERTHexDirection::E;
		int32 Integrity = 0;
		int32 Turns = 0;
		int32 FreeRotations = 0;
		FName ActionId;
	};
	TArray<FRTPendingCoverOp> Pending;

	// Gli SPOSTAMENTI (`Bastion.Reconfigure`). Portano con se' chi li ha chiesti, perche' il cooldown si decide
	// solo in applicazione: una rotazione gratuita non lo spende, e quale copertura si sposta — quindi se una
	// rotazione gratuita c'e' — si sa solo guardando il campo.
	struct FRTPendingMoveOp
	{
		FRTCellId Cell;
		ERTHexDirection ToEdge = ERTHexDirection::E;
		FName ActionId;
		ARTUnit* Mover = nullptr;
		int32 AbilityIndex = INDEX_NONE;
	};
	TArray<FRTPendingMoveOp> Moves;

	// Le voci di rifiuto decise gia' in raccolta: escono in coda agli esiti, cosi' il replay racconta prima
	// cosa e' successo al campo e poi cosa non e' successo a chi ci ha provato.
	TArray<FRTTurnLogEntry> Rejections;

	auto Reject = [this, &Rejections](const FRTCellId& From, const FRTCellId& Toward, const FName& ActionId,
		const TCHAR* Why, const ARTUnit* Who)
	{
		FRTTurnLogEntry Entry;
		Entry.Phase = ERTMatchPhase::Prep;
		Entry.Category = ERTLogCategory::Environment;
		Entry.Outcome = static_cast<uint8>(ERTEnvironmentOutcome::CoverRejected);
		Entry.ActionId = ActionId;
		Entry.SrcCell = From;
		Entry.TgtCell = Toward;
		Entry.Amount = 0;
		Rejections.Add(Entry);
		AddLogEvent(FString::Printf(TEXT("%s: %s annullata (%s)"),
			Who ? *Who->GetName() : TEXT("?"), *ActionId.ToString(), Why));
	};

	for (ARTUnit* Unit : Units) // gia' ordinati per cella dal chiamante
	{
		if (!Unit || !Unit->IsAlive()) { continue; }

		const int32 Index = Unit->PlannedAbilityIndex;
		const URTActionData* Ability = Unit->GetAbility(Index);
		if (!Ability || Ability->Def.StructureOp == ERTStructureOp::None)
		{
			continue; // non tocca le strutture di bordo: la vede il motore azioni, o nessuno
		}

		// Il piano si consuma nel turno, attivata o no: e' la stessa disciplina di `ModifyArc` nel Blast.
		const FRTActionDef Def = Ability->Def;
		const bool bHasEdge = Unit->bHasPlannedCoverEdge;
		const ERTHexDirection Edge = Unit->PlannedCoverEdge;
		const bool bTargetsCell = Unit->bAttackTargetsCell;
		const FRTCellId TargetCell = bTargetsCell ? Unit->PlannedAttackCell
			: (Unit->PlannedAttackTarget ? Unit->PlannedAttackTarget->Cell : Unit->Cell);
		const bool bHasTarget = bTargetsCell || Unit->PlannedAttackTarget != nullptr;

		Unit->PlannedAbilityIndex = INDEX_NONE;
		Unit->PlannedAttackTarget = nullptr;
		Unit->bHasPlannedCoverEdge = false;
		if (!Unit->CanUseAbility(Index)) { continue; }

		if (!bHasTarget)
		{
			// Nessuna scelta implicita: il catalogo la vieta, e «la propria cella» sarebbe una scelta implicita
			// per quanto comoda. Chi vuole ripararsi davanti dichiara la propria cella.
			Reject(Unit->Cell, Unit->Cell, Def.ActionId, TEXT("nessun bersaglio"), Unit);
			continue;
		}
		if (!bHasEdge)
		{
			Reject(Unit->Cell, TargetCell, Def.ActionId, TEXT("nessun bordo dichiarato"), Unit);
			continue;
		}
		// La portata si valida QUI, prima di toccare la mappa. `ModifyArc` non lo fa e la issue #206 lo
		// registra come difetto: un'azione che dichiara `Range 3` e opera a dieci celle non e' un'azione a
		// portata, e' un'azione senza portata.
		if (URTHexLibrary::HexDistance(Unit->Cell, TargetCell) > Def.RangeCells)
		{
			Reject(Unit->Cell, TargetCell, Def.ActionId, TEXT("fuori portata"), Unit);
			continue;
		}

		// Lo SPOSTAMENTO non consuma qui: la rotazione gratuita del pannello adattivo si scopre solo guardando
		// quale copertura verra' mossa, e quello succede in applicazione.
		if (Def.StructureOp == ERTStructureOp::MoveCover)
		{
			Moves.Add({ TargetCell, Edge, Def.ActionId, Unit, Index });
			continue;
		}

		Unit->ConsumeAbility(Index);

		// Integrita' e durata del catalogo terreni (`Structure.KineticPanel`: 30 punti struttura) e del
		// catalogo azioni (2 turni, come ogni altra modifica temporanea del campo).
		int32 Integrity = FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low);
		int32 Turns = 2;
		int32 FreeRotations = 0;

		// La VARIANTE attiva sostituisce i due numeri, se li dichiara. E' il primo punto in cui i `Parameters`
		// di una variante decidono qualcosa: il pannello rinforzato compra integrita' con la durata (45 per un
		// turno solo), l'adattivo compra flessibilita' con l'integrita' (25, e `DurationTurns` 0 — non scade
		// da sola). Non c'e' un ramo per eroe: si legge il dato di quell'abilita', qualunque sia.
		if (const FRTAbilityVariant* Variant = Ability->FindVariant(Unit->ActiveVariantId))
		{
			if (const int32* Declared = Variant->Parameters.Find(TEXT("Integrity")))
			{
				Integrity = *Declared;
			}
			if (const int32* Declared = Variant->Parameters.Find(TEXT("DurationTurns")))
			{
				Turns = *Declared;
			}
			if (const int32* Declared = Variant->Parameters.Find(TEXT("FreeRotations")))
			{
				FreeRotations = *Declared;
			}
		}

		Pending.Add({ TargetCell, Edge, Integrity, Turns, FreeRotations, Def.ActionId });
	}

	if (Pending.Num() == 0 && Moves.Num() == 0 && Rejections.Num() == 0)
	{
		return 0;
	}

	Pending.Sort([](const FRTPendingCoverOp& A, const FRTPendingCoverOp& B)
	{
		if (!(A.Cell == B.Cell)) { return URTHexLibrary::StableLess(A.Cell, B.Cell); }
		return static_cast<uint8>(A.Edge) < static_cast<uint8>(B.Edge);
	});

	int32 Applied = 0;
	for (const FRTPendingCoverOp& Op : Pending)
	{
		const TArray<FRTCellId> Ring = URTHexLibrary::Neighbors(Op.Cell);
		const int32 EdgeIndex = static_cast<int32>(Op.Edge);
		const FRTCellId Toward = Ring.IsValidIndex(EdgeIndex) ? Ring[EdgeIndex] : Op.Cell;

		if (!URTHexCoverLibrary::AddCover(Map, Op.Cell, Op.Edge, ERTHexCoverType::Low, Op.Integrity))
		{
			// Bordo gia' riparato o cella fuori mappa. Il cooldown resta speso: l'azione e' stata usata, ed e'
			// il `Cancel` del catalogo — non un'azione che non e' mai partita.
			FRTTurnLogEntry Entry;
			Entry.Phase = ERTMatchPhase::Prep;
			Entry.Category = ERTLogCategory::Environment;
			Entry.Outcome = static_cast<uint8>(ERTEnvironmentOutcome::CoverRejected);
			Entry.ActionId = Op.ActionId;
			Entry.SrcCell = Op.Cell;
			Entry.TgtCell = Toward;
			Entry.Amount = 0;
			Rejections.Add(Entry);
			AddLogEvent(FString::Printf(TEXT("(q=%d,r=%d,L%d): il bordo e' gia' riparato"),
				Op.Cell.X, Op.Cell.Y, Op.Cell.Layer));
			continue;
		}

		// Entra nella lista anche se non scade (`Turns` 0, pannello adattivo): la lista dice quali coperture
		// sono «di partita», e serve a `Reconfigure` per portarsi dietro le rotazioni gratuite. E' il tick a
		// saltare le permanenti, non l'inserimento a escluderle.
		DynamicCovers.Add({ Op.Cell, Op.Edge, Op.Turns, Op.FreeRotations });

		FRTTurnLogEntry Entry;
		Entry.Phase = ERTMatchPhase::Prep;
		Entry.Category = ERTLogCategory::Environment;
		Entry.Outcome = static_cast<uint8>(ERTEnvironmentOutcome::CoverCreated);
		Entry.ActionId = Op.ActionId;
		Entry.SrcCell = Op.Cell;
		Entry.TgtCell = Toward;
		Entry.Amount = Op.Turns;
		TurnLog.Add(Entry);
		AddLogEvent(FString::Printf(TEXT("(q=%d,r=%d,L%d): copertura eretta (%d turni)"),
			Op.Cell.X, Op.Cell.Y, Op.Cell.Layer, Op.Turns));
		++Applied;
	}

	// Gli SPOSTAMENTI dopo le creazioni: agiscono su un campo gia' aggiornato, e l'ordine fra i due gruppi e'
	// dichiarato invece che emergente dall'ordine degli attori.
	Moves.Sort([](const FRTPendingMoveOp& A, const FRTPendingMoveOp& B)
	{
		if (!(A.Cell == B.Cell)) { return URTHexLibrary::StableLess(A.Cell, B.Cell); }
		return static_cast<uint8>(A.ToEdge) < static_cast<uint8>(B.ToEdge);
	});

	for (const FRTPendingMoveOp& Move : Moves)
	{
		ARTUnit* Mover = Move.Mover;
		auto RejectMove = [&](const TCHAR* Why)
		{
			// Un tentativo fallito spende comunque l'azione: e' il `Cancel` del catalogo, non un turno gratis.
			if (Mover) { Mover->ConsumeAbility(Move.AbilityIndex); }
			Reject(Move.Cell, Move.Cell, Move.ActionId, Why, Mover);
		};

		const FRTHexCellData* Source = Map ? Map->FindCell(Move.Cell) : nullptr;
		if (Source == nullptr)
		{
			RejectMove(TEXT("cella fuori mappa"));
			continue;
		}

		// QUALE copertura si sposta: quella della cella indicata. Se ce n'e' piu' d'una la scelta sarebbe
		// implicita — «la prima dell'array» e' un ordine che il giocatore non vede — quindi si rifiuta. Un
		// rifiuto leggibile e' meglio di una regola indovinabile solo leggendo il codice.
		int32 Found = 0;
		ERTHexDirection FromEdge = ERTHexDirection::E;
		for (const FRTHexCover& Cover : Source->Covers)
		{
			if (Cover.Type == ERTHexCoverType::None) { continue; }
			++Found;
			FromEdge = Cover.Edge;
		}
		if (Found == 0)
		{
			RejectMove(TEXT("nessuna copertura da spostare"));
			continue;
		}
		if (Found > 1)
		{
			RejectMove(TEXT("piu' coperture sulla cella: quale spostare non e' dichiarabile"));
			continue;
		}
		if (FromEdge == Move.ToEdge)
		{
			RejectMove(TEXT("e' gia' su quel bordo"));
			continue;
		}

		const FRTHexCover* Entry = Source->CoverEntryOn(FromEdge);
		const ERTHexCoverType MovedType = Entry ? Entry->Type : ERTHexCoverType::Low;
		const int32 MovedIntegrity = Entry ? Entry->Integrity : FRTHexCover::DefaultIntegrity(MovedType);

		// Si toglie e si rimette: se la destinazione rifiuta (bordo gia' riparato), si RIMETTE dov'era. Una
		// copertura che sparisce perche' lo spostamento non era possibile sarebbe la peggiore delle due.
		URTHexCoverLibrary::RemoveCover(Map, Move.Cell, FromEdge);
		if (!URTHexCoverLibrary::AddCover(Map, Move.Cell, Move.ToEdge, MovedType, MovedIntegrity))
		{
			URTHexCoverLibrary::AddCover(Map, Move.Cell, FromEdge, MovedType, MovedIntegrity);
			RejectMove(TEXT("il bordo di destinazione e' gia' riparato"));
			continue;
		}

		// La voce di partita segue la copertura: durata residua conservata (spostare non ringiovanisce un
		// pannello) e una rotazione gratuita in meno, se ne aveva.
		bool bWasFree = false;
		for (FRTDynamicCover& Tracked : DynamicCovers)
		{
			if (Tracked.Cell == Move.Cell && Tracked.Edge == FromEdge)
			{
				Tracked.Edge = Move.ToEdge;
				if (Tracked.FreeRotations > 0)
				{
					--Tracked.FreeRotations;
					bWasFree = true;
				}
				break;
			}
		}

		// La rotazione gratuita non spende il cooldown: e' cio' che il pannello adattivo compra con i 5 punti
		// struttura in meno. Le successive si pagano come tutte le altre.
		if (!bWasFree && Mover)
		{
			Mover->ConsumeAbility(Move.AbilityIndex);
		}

		const TArray<FRTCellId> Ring = URTHexLibrary::Neighbors(Move.Cell);
		const int32 ToIndex = static_cast<int32>(Move.ToEdge);
		const FRTCellId Toward = Ring.IsValidIndex(ToIndex) ? Ring[ToIndex] : Move.Cell;

		FRTTurnLogEntry Entry2;
		Entry2.Phase = ERTMatchPhase::Prep;
		Entry2.Category = ERTLogCategory::Environment;
		Entry2.Outcome = static_cast<uint8>(ERTEnvironmentOutcome::CoverMoved);
		Entry2.ActionId = Move.ActionId;
		Entry2.SrcCell = Move.Cell;
		Entry2.TgtCell = Toward;
		Entry2.Amount = MovedIntegrity; // l'integrita' viaggia con la copertura: spostarla non la ripara
		TurnLog.Add(Entry2);
		AddLogEvent(FString::Printf(TEXT("(q=%d,r=%d,L%d): copertura spostata%s"),
			Move.Cell.X, Move.Cell.Y, Move.Cell.Layer, bWasFree ? TEXT(" (rotazione gratuita)") : TEXT("")));
		// Anche uno SPOSTAMENTO e' successo qualcosa. Contando solo le erezioni, un turno in cui l'unico
		// evento e' una `Reconfigure` non accendeva il beat di Prep nel playback: il TurnLog lo registrava e
		// lo spettatore non vedeva niente. E' la presentazione a tacere, non la regola — ma tacere resta il
		// difetto che questo checkpoint dice di non voler commettere.
		++Applied;
	}

	TurnLog.Append(Rejections);
	return Applied;
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

	// 0. Le strutture di BORDO (CP 9.5) prima del motore azioni, e fuori da esso: il loro esito e' una modifica
	// della mappa, non un evento verso un'unita'. Passando dalla raccolta, un'azione senza `Effects`
	// ripiegherebbe sul campo legacy `Power` e un pannello si metterebbe a fare danno — e' lo stesso motivo per
	// cui `ModifyArc` si intercetta prima degli intenti nel Blast.
	if (ResolveCoverStructures(Units) > 0)
	{
		bPrepActiveThisTurn = true; // c'e' un beat di Prep da mostrare nel playback
	}

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

		// PREDITTIVA (E18 CP 18.2): si ARMA qui e risolve al boundary del Move, quindi NON entra fra le
		// istanze che producono eventi adesso. Tenercela dentro le farebbe tradurre il proprio `Damage` in un
		// evento verso se stessa — oggi innocuo perche' la Prep ignora il danno, ma sarebbe un difetto latente
		// in attesa che qualcuno aggiunga quel `case`. Meglio che l'esclusione sia dichiarata.
		if (Ability->Def.PredictiveTargeting != ERTPredictiveTargeting::None)
		{
			// Senza una cella dichiarata non c'e' previsione: il piano e' incompleto e l'azione non si arma.
			// Non e' un errore da segnalare qui — la validazione del piano sta a monte — ma non si finge
			// nemmeno che una previsione senza bersaglio sia stata fatta.
			if (Unit->bAttackTargetsCell)
			{
				FRTArmedPrediction Armed;
				Armed.Shooter = Unit;
				Armed.LockedCell = Unit->PlannedAttackCell;
				Armed.ActionId = Ability->Def.ActionId;
				Armed.BaseActionId = Ability->Def.BaseActionId;
				// Il danno viene dagli EFFECTS del catalogo, non da un numero scritto qui: cambiare quanto
				// fa `InterceptShot` deve restare una modifica ai dati.
				for (const FRTActionEffectSpec& Effect : Ability->Def.Effects)
				{
					if (Effect.Effect == ERTActionEffect::Damage) { Armed.Damage += Effect.Amount; }
				}
				ArmedPredictions.Add(Armed);
				bPrepActiveThisTurn = true; // armare e' un beat di Prep osservabile
			}

			// L'abilita' e' comunque SPESA: chi ha scommesso ha pagato il cooldown, che la previsione sia
			// giusta o no. E' la meta' del costo che rende il whiff una scelta e non un tentativo gratuito.
			Unit->ConsumeAbility(Index);
			Unit->PlannedAbilityIndex = INDEX_NONE;
			Unit->PlannedAttackTarget = nullptr;
			continue;
		}

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
	TArray<bool> bPassThrough;    // parallelo a Units: vero per chi ATTRAVERSA le unita' ferme (LinearPass)
	Paths.Reserve(Units.Num());
	DashAbilityIdx.Init(INDEX_NONE, Units.Num());
	Priorities.Init(0, Units.Num());
	bLinearMovers.Init(false, Units.Num());
	bPassThrough.Init(false, Units.Num());
	int32 DasherCount = 0;
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		ARTUnit* Unit = Units[i];
		Paths.Add({ Unit->Cell }); // default: fermo

		const int32 DashIdx = Unit->PlannedDashAbility;
		Unit->PlannedDashAbility = INDEX_NONE; // consumato per questo turno (valido o no)
		const URTActionData* Dash = Unit->GetAbility(DashIdx);

		// Mobilita' rapida: lo dichiara il CATALOGO (fase FastMovement -> macro-fase Dash) e nient'altro. Il
		// flag legacy `bDash` non esiste piu' (#142): era la seconda risposta alla stessa domanda, e chi
		// leggeva l'una non vedeva le azioni dichiarate solo con l'altra.
		const bool bFastMovement = Dash != nullptr && URTCatalogLibrary::IsFastMovement(Dash->Def);
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

			// Chi e' stato ATTRAVERSATO paga come chi viene urtato: stessa coda, stessa fase, stessi effetti
			// dichiarati dall'azione. Non un secondo percorso di danno — un secondo percorso divergerebbe dal
			// primo alla prima modifica, ed e' esattamente il difetto che la issue #140 ha chiuso altrove.
			for (const int32 CrossedId : Linear.PassedThroughUnitIds)
			{
				if (!Units.IsValidIndex(CrossedId) || !Units[CrossedId] || !Units[CrossedId]->IsAlive())
				{
					continue;
				}
				FRTChargeImpact Crossed;
				Crossed.Attacker = Unit;
				Crossed.Target = Units[CrossedId];
				Crossed.Def = Dash->Def;
				PendingChargeImpacts.Add(Crossed);
				PendingImpactAttackerIdx.Add(i);
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
		// `ResolveLinearMove` ha gia' deciso CHI viene attraversato, ma il percorso passa ancora dal microstep
		// simultaneo, che ferma chiunque davanti a un'unita' immobile. Senza questo flag la lama si fermerebbe
		// davanti al primo bersaglio nonostante la traiettoria dica il contrario — ed e' esattamente cosi' che
		// il difetto si presentava: libreria corretta, partita no.
		bPassThrough[i] = (Dash->Def.MovementStyle == ERTMovementStyle::LinearPass);
		++DasherCount;
	}

	if (DasherCount == 0) { return; }

	// Scatti simultanei, ordine-indipendenti (stesso resolver a microstep del movimento, con priorita' e
	// scontro frontale fra mobilita' lineari — CP 4.8).
	const TArray<FRTHexMoveResult> Resolved = URTHexSimLibrary::ResolveHexPaths(Paths, Priorities, bLinearMovers, bPassThrough);

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

	// Applica le posizioni e consuma l'abilita'. Lo scatto SPENDE il movimento del turno (D-028): chi ha
	// scattato non prosegue col Move, e questo e' il punto in cui la regola diventa vera — nel resolver, che
	// e' cio' che decide l'esito (invariante #1), non in un controllo di pianificazione che il bot potrebbe
	// aggirare. Prima di D-028 il move normale sopravviveva allo scatto: era il «movimento doppio» di
	// `docs/gameplay/spec-dash.md`, vigente e implementato, ora superato.
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		if (DashAbilityIdx[i] == INDEX_NONE) { continue; }

		ARTUnit* Unit = Units[i];
		const FRTCellId Final = Resolved[i].Final;
		AddLogEvent(FString::Printf(TEXT("Scatto: %s -> (q=%d,r=%d,L%d)"), *Unit->GetName(), Final.X, Final.Y, Final.Layer));

		// Lo SCATTO nel TurnLog (#307). Fino a qui la fase Dash non lasciava nessuna voce di movimento: il
		// replay vedeva un'unita' comparire altrove fra un turno e l'altro, e chi leggeva la traccia non
		// poteva distinguere uno scatto da un Move — perche' il Move c'era e lo scatto no.
		//
		// La voce e' la stessa del movimento volontario e si distingue per due campi che ci sono gia':
		// `Phase = Dash` (quando) e `ActionId` (con quale mobilita'). Nessun esito nuovo: `ERTMoveOutcome`
		// descrive gia' cosa e' successo alla rotta, e uno scatto bloccato da una collisione e' bloccato
		// esattamente come un passo.
		if (const URTActionData* DashDef = Unit->GetAbility(DashAbilityIdx[i]))
		{
			FRTTurnLogEntry DashEntry;
			DashEntry.Phase = ERTMatchPhase::Dash;
			DashEntry.Category = ERTLogCategory::Move;
			DashEntry.Outcome = static_cast<uint8>(Resolved[i].Outcome);
			DashEntry.SrcCell = Unit->Cell; // cella di PARTENZA: `Unit->Cell` non e' ancora stata riscritta
			DashEntry.TgtCell = Final;
			DashEntry.Amount = Resolved[i].Entered.Num();
			DashEntry.ActionId = DashDef->Def.ActionId;
			DashEntry.BaseActionId = DashDef->Def.BaseActionId;
			TurnLog.Add(DashEntry);
		}

		Unit->ConsumeAbility(DashAbilityIdx[i]);

		// `FacingAfterDash` (CP 16.1, D-020): lo scatto orienta, e il Blast che segue leggera' QUESTO valore.
		// Derivato dalla rotta effettiva, prima di spostare l'unita', cosi' la partenza e' ancora quella vera.
		if (Resolved[i].Entered.Num() > 0)
		{
			TArray<FRTCellId> Walked;
			Walked.Reserve(Resolved[i].Entered.Num() + 1);
			Walked.Add(Unit->Cell);
			Walked.Append(Resolved[i].Entered);

			FRTHexSimUnit Dashed(i, Unit->Cell, /*InMoveBudget=*/ 0);
			Dashed.Facing = Unit->Facing;
			URTFacingLibrary::RecordFacingChange(Dashed, URTFacingLibrary::FacingFromPath(Walked, Dashed.Facing),
				ERTFacingOutcome::DerivedFromDash, ERTMatchPhase::Dash, TurnLog);
			Unit->Facing = Dashed.Facing;

			// Traccia per la rotazione dichiarata (#291): dopo uno scatto LINEARE la sola direzione legale e'
			// quella del movimento, e a fine turno non sarebbe piu' deducibile — chi ha scattato arriva al Move
			// con `PlannedCell` uguale alla cella attuale, indistinguibile da chi non si e' mosso.
			if (const URTActionData* DashUsed = Unit->GetAbility(DashAbilityIdx[i]))
			{
				Unit->MovementStyleThisTurn = DashUsed->Def.MovementStyle;
				Unit->WalkedThisTurn = Walked;
			}
		}

		Unit->Cell = Final;
		Unit->SetVisualLocation(Unit->WorldForCell(Final, Origin, CellSize, LayerH));
		ApplyTerrainOnEnterEffects(Snapshot.Map, Unit, Resolved[i].Entered);
		// Il movimento del turno e' finito qui: si scarta il percorso pianificato e la destinazione DIVENTA
		// la cella d'arrivo dello scatto. Senza l'assegnazione il resolver del Move vedrebbe una `PlannedCell`
		// diversa dalla posizione attuale e proverebbe comunque ad avvicinarcisi.
		Unit->PlannedPath.Reset();
		Unit->PlannedWaypoints.Reset();
		Unit->PlannedCell = Final;

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

		// SLOT consumati: lo dice il catalogo, non l'ActionId. Il movimento e' gia' speso qui sopra per OGNI
		// mobilita' rapida (D-028); resta da spendere la principale per chi dichiara `MovementAndMain`.
		//
		// Dopo D-028 nessuna azione di serie usa quello slot — `Action.Sprint`, che era l'unica, e' passata a
		// `Movement`. Il ramo resta perche' e' il meccanismo con cui un kit dichiara l'eccezione: «questa
		// mobilita' costa tutto il turno» si esprime in DATI, senza un `if` sull'ActionId qui dentro. E resta
		// verificato — `Actions.KitCanDeclareAMobilityThatCostsBothSlots` costruisce proprio quel caso, perche'
		// un ramo che nessun dato attraversa e' un ramo che nessun test difende.
		if (Used->Def.Slot == ERTActionSlot::MovementAndMain)
		{
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
		HexUnit.Facing = Unit->Facing; // CP 16.2: da che lato e' scoperta
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

	// `Action.Heal` (CP 8.5): azione di SUPPORTO, non un colpo. Si raccoglie qui, prima del ciclo degli
	// intenti — che consuma `PlannedAbilityIndex` e costruirebbe un intento d'attacco su un alleato — e si
	// applica DOPO i danni, piu' sotto: la priorita' 70 del catalogo la mette dopo gli attacchi (50-65),
	// quindi cura le ferite di questo turno, non quelle del turno prima.
	TArray<ARTUnit*> HealTargets;
	TArray<int32> HealAmounts;
	TArray<FRTCellId> HealSources;
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		ARTUnit* Unit = Units[i];
		const int32 HealIdx = Unit->PlannedAbilityIndex;
		const URTActionData* Heal = Unit->GetAbility(HealIdx);
		if (!Heal || Heal->Def.ActionId != FName(TEXT("Action.Heal")) || !Unit->CanUseAbility(HealIdx))
		{
			continue;
		}

		// Bersaglio: chi e' stato scelto in pianificazione, oppure SE STESSI se non c'e' nessuno — il catalogo
		// dichiara che la cura «puo' bersagliare se stessi», e curare a vuoto non e' un'alternativa sensata.
		ARTUnit* HealTarget = Unit->PlannedAttackTarget ? Unit->PlannedAttackTarget.Get() : Unit;
		Unit->PlannedAbilityIndex = INDEX_NONE;
		Unit->PlannedAttackTarget = nullptr;
		Unit->ConsumeAbility(HealIdx);

		// Portata dal catalogo, misurata come per ogni altra azione: una cura a distanza infinita sarebbe una
		// regola diversa da quella scritta.
		if (URTHexLibrary::HexDistance(Unit->Cell, HealTarget->Cell) > Heal->Def.RangeCells)
		{
			AddLogEvent(FString::Printf(TEXT("%s: cura fuori portata"), *Unit->GetName()));
			continue;
		}

		int32 Amount = 0;
		for (const FRTActionEffectSpec& Spec : Heal->Def.Effects)
		{
			if (Spec.Effect == ERTActionEffect::Heal) { Amount = Spec.Amount; break; }
		}
		if (Amount <= 0) { continue; }

		HealTargets.Add(HealTarget);
		HealAmounts.Add(Amount);
		HealSources.Add(Unit->Cell);
	}

	// Intenti d'attacco: qui si valida l'ABILITA' (esiste, non e' uno scatto, e' utilizzabile);
	// la GEOMETRIA (portata, linea di tiro, celle colpite) la valida URTHexCombatLibrary.
	TArray<FRTHexAttackIntent> Intents;
	TArray<int32> IntentAbilityIndex;
	TArray<const URTActionData*> IntentAbility;
	TArray<FRTActionDef> IntentDefs; // la definizione che ha prodotto l'intento: anche senza un URTActionData
					 // dietro (l'impatto di una carica e' dati puri, non un'abilita' selezionata)

	// Operazioni sugli ARCHI raccolte in questa fase (CP 9.4), applicate a fase CONCLUSA come i colpi e il
	// danno alle strutture: due unita' che agiscono sullo stesso ponte devono dare lo stesso esito in
	// qualunque ordine (invariante #3).
	struct FRTPendingArcOp { FRTCellId From; FRTCellId To; };
	TArray<FRTPendingArcOp> PendingArcOps;

	for (int32 i = 0; i < Units.Num(); ++i)
	{
		ARTUnit* Unit = Units[i];

		// Le azioni AMBIENTALI (fase `Environment`) risolvono nel Cleanup, non qui: il loro piano deve
		// sopravvivere a questo ciclo. E' lo stesso tranello gia' incontrato con `Action.Interrupt` (CP 4.7)
		// e `Action.Cleanse` (CP 5.2) — questo ciclo AZZERA `PlannedAbilityIndex` per ogni unita', quindi un
		// pass successivo non troverebbe piu' nulla da leggere.
		const URTActionData* PlannedNow = Unit->GetAbility(Unit->PlannedAbilityIndex);
		if (PlannedNow
			&& URTCatalogLibrary::MapResolutionPhase(PlannedNow->Def.ResolutionPhase) == ERTMatchPhase::Cleanup)
		{
			continue; // il piano resta: lo consuma `ResolveEnvironment`
		}

		// `Action.ModifyArc` (CP 9.4) risolve QUI, ma non e' un intento d'attacco: si intercetta PRIMA della
		// raccolta perche' il percorso normale leggerebbe il danno dagli effetti e, non trovandone, ripiegherebbe
		// sul campo legacy `Ability->Power` — un'azione che cambia la topologia si metterebbe a fare danno.
		//
		// L'arco e' identificato dalla COPPIA (chi la usa, il bersaglio): la pianificazione non ha un
		// bersaglio-arco, e questo resta un limite dichiarato finche' l'HUD di E11 non ne porta uno.
		if (PlannedNow && PlannedNow->Def.ActionId == FName(TEXT("Action.ModifyArc")))
		{
			ARTUnit* ArcTarget = Unit->PlannedAttackTarget;
			const int32 ArcAbilityIndex = Unit->PlannedAbilityIndex;
			Unit->PlannedAttackTarget = nullptr;
			Unit->PlannedAbilityIndex = INDEX_NONE; // consumato nel turno, attivata o no
			if (Unit->CanUseAbility(ArcAbilityIndex) && ArcTarget && ArcTarget->IsAlive())
			{
				// La PORTATA dichiarata dal catalogo si valida qui, prima di toccare la topologia (#206).
				// `ModifyArc` non passa da `ValidateInstance` — si intercetta prima della raccolta degli
				// intenti, due righe sopra — quindi il controllo che ogni altra azione del Blast riceve
				// gratis va scritto: senza, un'azione che dichiara `Range 3` opera dall'altra parte della
				// mappa, e non e' un'azione a portata, e' un'azione senza portata.
				if (URTHexLibrary::HexDistance(Unit->Cell, ArcTarget->Cell) > PlannedNow->Def.RangeCells)
				{
					// Il fallback dichiarato a catalogo e' `Cancel`: nessun effetto, ma VISIBILE. Un'azione
					// che sparisce in silenzio e' indistinguibile da un difetto — la stessa ragione per cui
					// esiste `CoverRejected`. Il motivo viaggia in `Amount` come per ogni altro fallback.
					FRTTurnLogEntry ArcRejected;
					ArcRejected.Phase = ERTMatchPhase::Blast;
					ArcRejected.Category = ERTLogCategory::Fallback;
					ArcRejected.Outcome = static_cast<uint8>(ERTFallbackOutcome::Cancelled);
					ArcRejected.ActionId = PlannedNow->Def.ActionId;
					ArcRejected.SrcCell = Unit->Cell;
					ArcRejected.TgtCell = ArcTarget->Cell;
					ArcRejected.Amount = static_cast<int32>(ERTActionInvalidReason::OutOfRange);
					TurnLog.Add(ArcRejected);
					AddLogEvent(FString::Printf(TEXT("%s: %s"),
						*Unit->GetName(), *URTTurnLogLibrary::DescribeEntry(ArcRejected)));

					// L'abilita' NON si consuma: il piano e' gia' stato azzerato sopra (si spende nel turno,
					// attivata o no), ma il cooldown paga solo cio' che ha davvero toccato la mappa.
					continue;
				}

				Unit->ConsumeAbility(ArcAbilityIndex);
				PendingArcOps.Add({ Unit->Cell, ArcTarget->Cell });
			}
			continue;
		}

		ARTUnit* Target = Unit->PlannedAttackTarget;
		const int32 AbilityIndex = Unit->PlannedAbilityIndex;
		Unit->PlannedAttackTarget = nullptr; // consumati nel turno
		Unit->PlannedAbilityIndex = INDEX_NONE;

		const URTActionData* Ability = Unit->GetAbility(AbilityIndex);
		if (!Ability || URTCatalogLibrary::IsFastMovement(Ability->Def) || !Unit->CanUseAbility(AbilityIndex))
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

		// D-020: un'azione con BERSAGLIO orienta chi la usa PRIMA che risolva. Non e' una proprieta' dichiarata
		// per azione — e' la regola: si guarda cio' che si colpisce. Qui, e non nel Prep, perche' le azioni di
		// Prep del vertical slice agiscono su chi le usa (`Instance.TargetUnitId = i`) e non hanno un bersaglio
		// esterno verso cui girarsi.
		//
		// Conta la cella DICHIARATA nel piano, non il primo bersaglio che l'area colpira': l'orientamento e' una
		// scelta del giocatore, e un'area che prende tre unita' non deve farlo dipendere dall'ordine di calcolo.
		// `Unit->IsAlive()` e non solo il bersaglio: questo ciclo non filtra gli attaccanti morti (chi cade nel
		// Dash ci arriva col piano ancora addosso) e il loro colpo viene scartato piu' avanti, da
		// `CollectHexAttacks`, che salta le unita' non vive. Senza il guard un cadavere si girerebbe verso il
		// bersaglio e lascerebbe la sua voce nel TurnLog: deterministica, ma rumore che entra nell'hash del replay.
		if (Unit->IsAlive() && Target && Target->IsAlive() && Target != Unit)
		{
			ERTHexDirection TowardsTarget = Unit->Facing;
			if (URTHexLibrary::DirectionTowards(Unit->Cell, Target->Cell, TowardsTarget))
			{
				FRTHexSimUnit Attacker(i, Unit->Cell, /*InMoveBudget=*/ 0);
				Attacker.Facing = Unit->Facing;
				URTFacingLibrary::RecordFacingChange(Attacker, TowardsTarget, ERTFacingOutcome::TargetingReoriented,
					ERTMatchPhase::Blast, TurnLog);
				Unit->Facing = Attacker.Facing;

				// Anche nella copia del Blast: `HexUnits` e' stato costruito PRIMA di questo ciclo, e la difesa
				// direzionale (CP 16.2) legge di li'. Senza questa riga, due unita' che si attaccano a vicenda
				// nella stessa fase si girerebbero l'una verso l'altra e verrebbero comunque colpite alle spalle.
				if (HexUnits.IsValidIndex(i))
				{
					HexUnits[i].Facing = Unit->Facing;
				}
			}
		}

		// Da qui si ragiona su ISTANZE, non su puntatori: e' l'istanza che si valida e su cui si applica il
		// fallback DICHIARATO dall'azione, in un punto solo (spec E4 §D5).
		FRTActionInstance Instance;
		Instance.Def = Ability->Def;
		// Portata dell'istanza: dal catalogo se la dichiara, altrimenti dal portatore. `RangeCells <= 0` NON
		// vuol dire "nessuna portata" — e' la convenzione del catalogo per «portata dell'arma»
		// (`Action.PrecisionAttack`, «range dell'arma +1»; `Action.MarkTarget`).
		//
		// Prima il ponte valeva solo per le azioni SENZA `ActionId`, quindi quelle due si validavano con
		// portata 0 e degradavano sempre al proprio fallback, mentre `Intent.RangeCells` leggeva il campo
		// dell'asset: due verita' sulla stessa portata, e l'azione non arrivava mai a bersaglio.
		// Difetto trovato al CP 8.2 (issue #65) mentre si cablava `Status.Marked`.
		if (Instance.Def.ActionId.IsNone() || Instance.Def.RangeCells <= 0)
		{
			Instance.Def.RangeCells = Ability->RangeCells;
		}
		Instance.SourceUnitId = i;
		// Bersaglio a CELLA: nessuna unita' mirata per costruzione, non una che si e' persa. La distinzione
		// conta subito qui sotto, dove `TargetUnitId == INDEX_NONE` significa `TargetGone` e degraderebbe al
		// fallback un'azione che invece sta facendo esattamente cio' che le e' stato chiesto.
		const bool bTargetsCell = Unit->bAttackTargetsCell;
		Instance.TargetUnitId = (!bTargetsCell && Target && IndexOf.Contains(Target)) ? IndexOf[Target] : INDEX_NONE;
		Instance.TargetCell = bTargetsCell ? Unit->PlannedAttackCell : (Target ? Target->Cell : Unit->Cell);
		Instance.EventSequence = Intents.Num();

		// Un'azione di Blast senza bersaglio non e' un'azione «che non ne ha uno» (quelle sono il movimento e il
		// supporto su se stessi, e risolvono altrove): e' un'azione che il bersaglio l'ha PERSO — eliminato e
		// rimosso dal livello, o mai valido. Senza questa distinzione l'istanza risulterebbe valida e l'unita'
		// finirebbe per puntare la propria cella.
		const ERTActionInvalidReason Reason = (Instance.TargetUnitId == INDEX_NONE && !bTargetsCell)
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
		// La friendly fire policy la DICHIARA l'azione (`Action.CircularAoE` la ha true): senza questa riga
		// l'intento nasceva sempre a false e nessuna area colpiva un alleato in partita, benche' il dato
		// esistesse nel catalogo e il resolver puro lo rispettasse. Difetto trovato al CP 8.2 e corretto qui.
		Intent.bFriendlyFire = Instance.Def.bFriendlyFire;
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

		// Danno alle STRUTTURE (CP 9.2): scala distinta, dichiarata dall'azione. Nessun ripiego sul `Power`
		// legacy — un'abilita' che non lo dichiara non sfonda, e va bene cosi': sfondare e' una capacita' che
		// il catalogo concede, non un effetto collaterale di essere forti.
		for (const FRTActionEffectSpec& Spec : Ability->Def.Effects)
		{
			if (Spec.Effect == ERTActionEffect::DamageStructure)
			{
				Intent.StructurePower = FMath::Max(0, Spec.Amount);
				break;
			}
		}

		// PORTE (CP 9.3): lo stato viaggia in `Amount` (interi soltanto, come la durata di uno stato). Un
		// valore fuori intervallo non produce nessun ordine — meglio nessuna operazione che una porta portata
		// a uno stato che non esiste.
		for (const FRTActionEffectSpec& Spec : Ability->Def.Effects)
		{
			if (Spec.Effect != ERTActionEffect::SetDoorState)
			{
				continue;
			}
			if (Spec.Amount >= 0 && Spec.Amount <= static_cast<int32>(ERTHexDoorState::Destroyed))
			{
				Intent.bChangesDoor = true;
				Intent.DoorState = static_cast<ERTHexDoorState>(Spec.Amount);
			}
			break;
		}
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

		const int32 ImpactDamage = URTCatalogLibrary::FirstDamage(Impact.Def);
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
		Entry.ActionId = Reaction->Def.ActionId; // `Bastion.Interposition` non e' `Action.Intercept` (CP 5.5)
		Entry.BaseActionId = Reaction->Def.BaseActionId; // vuoto finche' le reazioni non dichiarano un profilo

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
	//
	// Riscrivere il solo `TargetId` non basta (D-017): la copertura e' gia' dentro il `Power`, calcolata sul
	// bordo davanti a chi era il bersaglio quando i colpi sono stati raccolti. Il colpo arriverebbe a chi si
	// interpone protetto dal muretto di qualcun altro — e il combat log mostrerebbe una copertura che davanti
	// a lui non esiste. `RedirectHitTo` rivalida la geometria target-dependent su chi il colpo lo incassa
	// davvero, ed e' l'unico posto dove farlo: qui il redirect e' deciso e nessuna reazione e' ancora stata
	// valutata sui colpi riscritti, quindi la rivalidazione non puo' aprire una seconda opportunity.
	//
	// E' la stessa disciplina dei bonus di coppia piu' sotto (`Flux.LinearDischarge` contro `Status.Wet`):
	// cio' che dipende da CHI subisce si decide dopo l'Intercept, non prima.
	for (int32 r = 0; r < RedirectHit.Num(); ++r)
	{
		Plan.Hits[RedirectHit[r]] = URTHexCombatLibrary::RedirectHitTo(RedirectTo[r],
			Plan.Hits[RedirectHit[r]], HexUnits, Intents, Map);
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
	TArray<int32> DeflectDelta;       // riduzione del danno per bersaglio, DICHIARATA dalle reazioni attivate
	TArray<FRTAttack> CounterAttacks; // colpi di ritorno delle reazioni attivate, accodati ai colpi veri
	TArray<FRTCellId> CounterAttackSrc; // parallelo a CounterAttacks: cella di chi contrattacca, per il TurnLog
	// Identita' della REAZIONE che ha prodotto il colpo di ritorno (CP 11.3, #79). Senza, un contrattacco
	// finiva nel log indistinguibile da un attacco pianificato: due voci di combattimento identiche per un
	// colpo scelto in planning e uno arrivato di rimbalzo. Qui l'ActionId c'e' — e' `Reaction->Def.ActionId`,
	// lo stesso che la voce di categoria `Reaction` porta gia' da CP 5.5.
	TArray<FName> CounterActionId;
	TArray<FName> CounterBaseActionId;
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
		Entry.ActionId = Reaction->Def.ActionId; // identita': `Vektor.Deflection` non e' `Action.Deflect` (CP 5.5)
		Entry.BaseActionId = Reaction->Def.BaseActionId; // vuoto finche' le reazioni non dichiarano un profilo

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

			// TUTTI gli effetti che la reazione DICHIARA, non il primo che questo orchestratore riconosce
			// (CP 5.5). `URTReactionLibrary::BuildReactionEvents` decide anche CHI li subisce, per tipo di
			// effetto: offensivi a chi ha innescato, difensivi a chi reagisce. Qui non si guarda mai
			// l'`ActionId`: e' cio' che permette a una reazione d'eroe di riusare la semantica di
			// `Action.Deflect`/`Action.Counter` con numeri propri senza un ramo per eroe.
			for (const FRTActionEvent& Event : URTReactionLibrary::BuildReactionEvents(Reaction->Def, i, TriggeredBy))
			{
				if (!Units.IsValidIndex(Event.TargetUnitId) || !Units[Event.TargetUnitId]) { continue; }
				ARTUnit* EffectTarget = Units[Event.TargetUnitId];
				switch (Event.Kind)
				{
				case ERTActionEffect::Damage:
					// Il colpo di ritorno entra fra gli attacchi normali, quindi risolve sullo stato iniziale
					// come tutti gli altri: chi cade in questo stesso Blast contrattacca comunque, che e' la
					// regola gia' dichiarata da URTCombatResolver ("un'unita' colpita a morte infligge
					// comunque il proprio danno").
					CounterAttacks.Add(FRTAttack(Event.TargetUnitId, Event.Amount));
					CounterAttackSrc.Add(Unit->Cell);
					CounterActionId.Add(Reaction->Def.ActionId);
					CounterBaseActionId.Add(Reaction->Def.BaseActionId);
					AddLogEvent(FString::Printf(TEXT("%s: contrattacco su %s (%d)"),
						*Unit->GetName(), *EffectTarget->GetName(), Event.Amount));
					break;

				case ERTActionEffect::DamageReduction:
					// Vale sul colpo che ha innescato la reazione: si attiva una volta sola, quindi entra fra
					// i delta del PRIMO danno diretto, come il -15 di Guard.
					DeflectDelta[Event.TargetUnitId] -= Event.Amount;
					break;

				case ERTActionEffect::Shield:
					// Prima che i colpi vengano risolti, e con lo stesso aggiornamento sullo snapshot `States`
					// da cui il resolver legge: uno scudo che arrivasse dopo scadrebbe nel Cleanup dello
					// stesso turno senza aver protetto da niente.
					EffectTarget->AddTemporaryShield(Event.Amount);
					States[Event.TargetUnitId].Shield = EffectTarget->Shield;
					AddLogEvent(FString::Printf(TEXT("%s: +%d scudo dalla reazione"),
						*EffectTarget->GetName(), Event.Amount));
					break;

				default:
					// `Heal`, `Push`, `Pull`, `Status`: nessuna reazione del catalogo v0.1 li dichiara, e
					// applicarli qui richiederebbe cio' che il pass non ha (una direzione per la spinta, il
					// consumo degli stati insieme agli altri colpi). Il posto dove aggiungerli e' questo.
					break;
				}
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
		// Il bersaglio puo' essere una CELLA e non un'unita' (`Fallback.AttackCell`, e da CP 9.2 anche il
		// modo in cui si punta una struttura): con `TargetId == INDEX_NONE` indicizzare `HexUnits` uscirebbe
		// dall'array. La cella mirata e' comunque cio' che il log deve dire.
		const bool bTargetsUnit = HexUnits.IsValidIndex(Blocked.TargetId);
		FRTTurnLogEntry NoLos;
		NoLos.Phase = ERTMatchPhase::Blast;
		NoLos.Category = ERTLogCategory::Combat;
		NoLos.Outcome = static_cast<uint8>(ERTCombatOutcome::NoLineOfSight);
		NoLos.SrcCell = HexUnits[Blocked.AttackerId].Cell;
		NoLos.TgtCell = bTargetsUnit ? HexUnits[Blocked.TargetId].Cell : Blocked.TargetCell;
		NoLos.Amount = 0;
		// QUALE azione e' stata fermata (CP 11.3, #79). `Plan.BlockedIntents` indicizza `Intents`, e
		// `IntentDefs` gli e' parallelo per costruzione: entrambi crescono negli stessi due punti, quelli
		// degli intenti pianificati e degli impatti di carica.
		//
		// Serve piu' qui che altrove: un attacco che non avviene lascia SOLO questa voce, e senza il nome
		// «nessuna linea di tiro» non dice se a mancare il colpo e' stata l'ultimate o l'attacco base.
		if (IntentDefs.IsValidIndex(BlockedIdx))
		{
			NoLos.ActionId = IntentDefs[BlockedIdx].ActionId;
			NoLos.BaseActionId = IntentDefs[BlockedIdx].BaseActionId;
		}
		TurnLog.Add(NoLos);
		AddLogEvent(FString::Printf(TEXT("%s (%s -> %s)"), *URTTurnLogLibrary::DescribeEntry(NoLos),
			*Units[Blocked.AttackerId]->GetName(),
			bTargetsUnit ? *Units[Blocked.TargetId]->GetName() : TEXT("cella")));
	}

	// STRUTTURE (CP 9.2): il danno raccolto contro le barriere si applica ORA, a colpi risolti — non durante
	// la raccolta. Chi ha sparato in questo Blast non guadagna la linea perche' il muro e' caduto: la vista e
	// il grafo si riaprono dalla fase successiva, e l'ordine dei colpi non cambia l'esito (invariante #3).
	//
	// La mappa si prende dall'actor, non dal `Map` const di questa funzione: la partita gira su una COPIA di
	// lavoro (`ARTGameMode` la duplica all'avvio), quindi abbattere un muro non tocca l'asset su disco. E' lo
	// stesso puntatore che il Cleanup usa per le superfici dinamiche.
	ARTHexMapActor* StructureMapActor = ARTHexMapActor::FindInWorld(GetWorld());
	URTHexMapAsset* MutableMap = StructureMapActor ? StructureMapActor->MapAsset : nullptr;
	for (const FRTCoverDamageResult& Change :
		URTHexCoverLibrary::ApplyStructureDamage(MutableMap, Plan.StructureHits))
	{
		FRTTurnLogEntry Entry;
		Entry.Phase = ERTMatchPhase::Blast;
		Entry.Category = ERTLogCategory::Environment;
		Entry.Outcome = static_cast<uint8>(
			Change.bDestroyed ? ERTEnvironmentOutcome::CoverDestroyed : ERTEnvironmentOutcome::CoverDamaged);
		// La coppia di celle E' il bordo: nessun campo nuovo nel TurnLog per dire "quale lato".
		Entry.SrcCell = Change.Cell;
		Entry.TgtCell = Change.Toward;
		Entry.Amount = Change.RemainingIntegrity;
		TurnLog.Add(Entry);
		AddLogEvent(FString::Printf(TEXT("Copertura (q=%d,r=%d,L%d) verso (q=%d,r=%d): %s (integrita' %d)"),
			Change.Cell.X, Change.Cell.Y, Change.Cell.Layer, Change.Toward.X, Change.Toward.Y,
			Change.bDestroyed ? TEXT("abbattuta") : TEXT("danneggiata"), Change.RemainingIntegrity));

		// Una copertura abbattuta smette di essere anche una copertura TEMPORANEA (CP 9.5). Senza questa
		// riga l'entry sopravvive al proprio riparo, e siccome identifica la barriera con la sola coppia
		// (cella, bordo), alla scadenza del suo timer porterebbe via **quello che trova su quel bordo**: se
		// nel frattempo qualcuno ha riparato lo stesso varco, gli distrugge il pannello in anticipo e scrive
		// nel TurnLog una scadenza mai avvenuta. Due Bastion sullo stesso choke point bastano — i cooldown
		// sono per unita', quindi il secondo non aspetta il primo.
		if (Change.bDestroyed)
		{
			ERTHexDirection DestroyedEdge = ERTHexDirection::E;
			if (URTHexCoverLibrary::EdgeDirection(Change.Cell, Change.Toward, DestroyedEdge))
			{
				const FRTCellId DestroyedCell = Change.Cell;
				DynamicCovers.RemoveAll([&DestroyedCell, DestroyedEdge](const FRTDynamicCover& Tracked)
				{
					return Tracked.Cell == DestroyedCell && Tracked.Edge == DestroyedEdge;
				});
			}
		}
	}

	// ARCHI (CP 9.4). Due pezzi, entrambi a fase conclusa:
	//
	// 1. il DANNO. Un'azione che dichiara `DamageStructure` e mira a un'unita' agli estremi di un ponte lo
	//    scalfisce. La coppia (attaccante, bersaglio) identifica l'arco per la stessa ragione di `ModifyArc`;
	//    se fra le due celle non c'e' un arco, `DamageArc` non fa nulla e non tocca la revisione.
	for (int32 IntentIdx = 0; IntentIdx < Intents.Num(); ++IntentIdx)
	{
		const FRTHexAttackIntent& Intent = Intents[IntentIdx];
		if (Intent.StructurePower <= 0 || !HexUnits.IsValidIndex(Intent.AttackerId))
		{
			continue;
		}
		// Il bersaglio puo' essere una CELLA (`TargetId == INDEX_NONE`): l'arco si cerca comunque, verso la
		// cella mirata — indicizzare `HexUnits[TargetId]` uscirebbe dall'array.
		const FRTCellId ArcTo = HexUnits.IsValidIndex(Intent.TargetId)
			? HexUnits[Intent.TargetId].Cell : Intent.TargetCell;

		for (const FRTArcChange& Change :
			URTHexArcLibrary::DamageArc(MutableMap, HexUnits[Intent.AttackerId].Cell, ArcTo, Intent.StructurePower))
		{
			FRTTurnLogEntry Entry;
			Entry.Phase = ERTMatchPhase::Blast;
			Entry.Category = ERTLogCategory::Environment;
			Entry.Outcome = static_cast<uint8>(Change.bBroken
				? ERTEnvironmentOutcome::BridgeDestroyed : ERTEnvironmentOutcome::BridgeDamaged);
			Entry.SrcCell = Change.From;
			Entry.TgtCell = Change.To;
			Entry.Amount = Change.RemainingIntegrity;
			TurnLog.Add(Entry);
			AddLogEvent(FString::Printf(TEXT("Ponte (q=%d,r=%d,L%d) -> (q=%d,r=%d,L%d): %s (integrita' %d)"),
				Change.From.X, Change.From.Y, Change.From.Layer,
				Change.To.X, Change.To.Y, Change.To.Layer,
				Change.bBroken ? TEXT("crollato") : TEXT("danneggiato"), Change.RemainingIntegrity));

			// Un ponte ABBATTUTO smette di essere anche un ponte TEMPORANEO. Senza questa riga l'entry
			// sopravvive al proprio ponte, e non in modo innocuo: `DamageArc` non RIMUOVE l'arco, lo marca
			// `Destroyed` e lo lascia sulla mappa, quindi alla scadenza del timer `TickDynamicArcs` chiama
			// `RemoveTransition` e ci RIESCE — scrivendo un `BridgeRemoved` per un crollo avvenuto turni prima
			// e portandosi via le macerie. Le macerie devono restare: e' su di esse che `SetArcState` esercita
			// il proprio «terminale» (un ponte abbattuto non si riattiva).
			//
			// Stessa disciplina delle coperture (#301, `ApplyStructureDamage`): l'entry muore quando muore la
			// struttura che rappresenta. `DamageArc` restituisce un `Change` per verso, quindi qui si passa due
			// volte sulla stessa coppia: `RemoveAll` e' idempotente e la seconda non trova piu' nulla.
			if (Change.State == ERTHexArcState::Destroyed)
			{
				const FRTCellId BrokenFrom = Change.From;
				const FRTCellId BrokenTo = Change.To;
				DynamicArcs.RemoveAll([&BrokenFrom, &BrokenTo](const FRTDynamicArc& Tracked)
				{
					return (Tracked.From == BrokenFrom && Tracked.To == BrokenTo)
						|| (Tracked.From == BrokenTo && Tracked.To == BrokenFrom);
				});
			}
		}
	}

	// 2. `ModifyArc`: se il collegamento c'e' lo TOGLIE, altrimenti lo CREA — «apri o chiudi» e' la stessa
	//    azione vista dai due lati. Il ponte creato e' TEMPORANEO (2 turni, come le altre modifiche ambientali
	//    del catalogo) e CONDUTTIVO: la scarica lo risale, quindi e' un rischio oltre che una scorciatoia.
	//    Ordine canonico prima di applicare: l'ordine delle unita' non deve decidere l'esito.
	PendingArcOps.Sort([](const FRTPendingArcOp& A, const FRTPendingArcOp& B)
	{
		if (!(A.From == B.From)) { return URTHexLibrary::StableLess(A.From, B.From); }
		return URTHexLibrary::StableLess(A.To, B.To);
	});
	for (const FRTPendingArcOp& Op : PendingArcOps)
	{
		if (!MutableMap) { break; }

		const bool bRemoved = MutableMap->RemoveTransition(Op.From, Op.To, /*bBothDirections*/ true);
		if (bRemoved)
		{
			DynamicArcs.RemoveAll([&Op](const FRTDynamicArc& D)
			{
				return (D.From == Op.From && D.To == Op.To) || (D.From == Op.To && D.To == Op.From);
			});
		}
		else
		{
			// Creazione: i due versi in UNA sola revisione (un ponte e' un evento, non due archi).
			TArray<FRTHexEdge> Built;
			FRTHexEdge Forward(Op.From, Op.To, /*Cost*/ 1, ERTHexTransitionKind::Bridge);
			Forward.bConductsElectricity = true;
			FRTHexEdge Backward(Op.To, Op.From, /*Cost*/ 1, ERTHexTransitionKind::Bridge);
			Backward.bConductsElectricity = true;
			Built.Add(Forward);
			Built.Add(Backward);
			MutableMap->UpdateTransitions(Built);
			// Durata 2 turni, come le altre modifiche ambientali del catalogo (`Ignite`, `CreateWater`).
			DynamicArcs.Add({ Op.From, Op.To, /*TurnsRemaining*/ 2, /*CreatedOnTurn*/ TurnNumber });
		}

		FRTTurnLogEntry Entry;
		Entry.Phase = ERTMatchPhase::Blast;
		Entry.Category = ERTLogCategory::Environment;
		Entry.Outcome = static_cast<uint8>(
			bRemoved ? ERTEnvironmentOutcome::BridgeRemoved : ERTEnvironmentOutcome::BridgeCreated);
		Entry.ActionId = FName(TEXT("Action.ModifyArc"));
		Entry.SrcCell = Op.From;
		Entry.TgtCell = Op.To;
		Entry.Amount = bRemoved ? 0 : 2; // turni di durata del ponte creato
		TurnLog.Add(Entry);
		AddLogEvent(FString::Printf(TEXT("Collegamento %s: (q=%d,r=%d,L%d) -> (q=%d,r=%d,L%d)"),
			bRemoved ? TEXT("rimosso") : TEXT("creato"),
			Op.From.X, Op.From.Y, Op.From.Layer, Op.To.X, Op.To.Y, Op.To.Layer));
	}

	// PORTE (CP 9.3): stessa disciplina delle strutture — gli ordini si applicano ORA, a colpi risolti, non
	// durante la raccolta. Chi ha sparato in questo Blast non perde la linea perche' una porta si e' chiusa: la
	// topologia cambia dalla fase SUCCESSIVA, che e' il Move — ed e' li' che il percorso gia' pianificato deve
	// accorgersene.
	for (const FRTDoorChange& Change : URTHexDoorLibrary::ApplyDoorOps(MutableMap, Plan.DoorOps))
	{
		FRTTurnLogEntry Entry;
		Entry.Phase = ERTMatchPhase::Blast;
		Entry.Category = ERTLogCategory::Environment;
		Entry.Outcome = static_cast<uint8>(
			Change.bBlocking ? ERTEnvironmentOutcome::DoorClosed : ERTEnvironmentOutcome::DoorOpened);
		// La coppia di celle E' il bordo, come per le coperture: nessun campo nuovo nel TurnLog.
		Entry.SrcCell = Change.Cell;
		Entry.TgtCell = Change.Toward;
		Entry.Amount = static_cast<int32>(Change.State);
		TurnLog.Add(Entry);
		AddLogEvent(FString::Printf(TEXT("Porta (q=%d,r=%d,L%d) verso (q=%d,r=%d): %s"),
			Change.Cell.X, Change.Cell.Y, Change.Cell.Layer, Change.Toward.X, Change.Toward.Y,
			Change.bBlocking ? TEXT("chiusa") : TEXT("aperta")));
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
	// `Status.Marked` (CP 8.2, chiude la issue #137): pass a PRIORITA' dentro il Blast. Il catalogo da' a
	// `Action.MarkTarget` la priorita' 40 «la piu' bassa delle offensive, perche' un marchio che arrivasse
	// dopo i colpi non servirebbe a nulla»: il marchio deve quindi valere per i colpi dello stesso Blast a
	// priorita' piu' alta. Applicarlo insieme agli altri status (piu' sotto, dopo il danno) lo renderebbe
	// inutilizzabile in ogni turno — con durata 1 scade nel Cleanup dello stesso turno in cui e' applicato.
	//
	// Resta «raccogli poi applica»: si aggiustano i Power di colpi GIA' congelati, non si ricalcolano
	// bersagli o traiettorie. Precedente nel codice: `Action.Interrupt` (priorita' 20) filtra i colpi del
	// piano prima che diventino danno.
	TArray<int32> IncomingMarkPriority; // per bersaglio: priorita' del marchio applicato in QUESTO Blast
	IncomingMarkPriority.Init(MAX_int32, Units.Num());
	TArray<bool> bMarkedBeforeBlast;    // marchio ereditato da un turno precedente: vale per qualunque colpo
	bMarkedBeforeBlast.Init(false, Units.Num());
	// Stessa disciplina per `Wet`, e per la stessa ragione: `Riva.PressureJet` bagna DENTRO il Blast, quindi
	// un bonus che leggesse solo `HasStatus` non lo vedrebbe mai — e con durata 1 il bagnato scade nel Cleanup
	// dello stesso turno, quindi non lo vedrebbe nemmeno il turno dopo. La combo acqua+elettricita' era
	// documentata, aveva un test verde sull'aritmetica, e non era eseguibile in partita (#242).
	// Differenza dal marchio: il bagnato NON si consuma. Vale su ogni colpo finche' il bersaglio e' bagnato,
	// quindi qui si raccoglie soltanto la priorita' — non c'e' un `WetSpentOn`.
	TArray<int32> IncomingWetPriority;  // per bersaglio: priorita' dell'azione che lo bagna in QUESTO Blast
	IncomingWetPriority.Init(MAX_int32, Units.Num());
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		bMarkedBeforeBlast[i] = Units[i] && Units[i]->HasStatus(TAG_Status_Marked);
	}
	for (const FRTHexAttackHit& Hit : Plan.Hits)
	{
		if (!IntentDefs.IsValidIndex(Hit.IntentIndex) || !Units.IsValidIndex(Hit.TargetId)
			|| !Units.IsValidIndex(Hit.AttackerId) || !Units[Hit.TargetId] || !Units[Hit.AttackerId])
		{
			continue;
		}
		const FRTActionDef& Def = IntentDefs[Hit.IntentIndex];
		for (const FRTActionEffectSpec& Spec : Def.Effects)
		{
			if (Spec.Effect == ERTActionEffect::Status && Spec.StatusTag == TAG_Status_Marked)
			{
				Units[Hit.TargetId]->ApplyMarkedBy(Units[Hit.AttackerId]->TeamId, Spec.StatusDuration);
				IncomingMarkPriority[Hit.TargetId] = FMath::Min(IncomingMarkPriority[Hit.TargetId], Def.Priority);
			}
			// Il bagnato si REGISTRA e basta: applicarlo qui lo farebbe scadere nel Cleanup come prima, e
			// soprattutto lo renderebbe visibile a colpi che risolvono PRIMA di chi bagna. Lo applica il pass
			// degli effetti, dopo il danno, come per ogni altro status.
			else if (Spec.Effect == ERTActionEffect::Status && Spec.StatusTag == TAG_Status_Wet)
			{
				IncomingWetPriority[Hit.TargetId] = FMath::Min(IncomingWetPriority[Hit.TargetId], Def.Priority);
			}
		}
	}

	// Ordine di spesa del marchio: priorita' crescente, poi ActionId -> AttackerId -> IntentIndex. E' l'ordine
	// canonico dell'ADR-0003 §3, e serve perche' «il PROSSIMO attacco alleato» sia una domanda con una sola
	// risposta anche quando due alleati colpiscono lo stesso bersaglio nello stesso turno.
	TArray<int32> HitOrder;
	HitOrder.Reserve(Plan.Hits.Num());
	for (int32 h = 0; h < Plan.Hits.Num(); ++h) { HitOrder.Add(h); }
	HitOrder.Sort([&Plan, &IntentDefs](int32 A, int32 B)
	{
		const FRTHexAttackHit& HA = Plan.Hits[A];
		const FRTHexAttackHit& HB = Plan.Hits[B];
		const int32 PA = IntentDefs.IsValidIndex(HA.IntentIndex) ? IntentDefs[HA.IntentIndex].Priority : MAX_int32;
		const int32 PB = IntentDefs.IsValidIndex(HB.IntentIndex) ? IntentDefs[HB.IntentIndex].Priority : MAX_int32;
		if (PA != PB) { return PA < PB; }
		const FName IdA = IntentDefs.IsValidIndex(HA.IntentIndex) ? IntentDefs[HA.IntentIndex].ActionId : NAME_None;
		const FName IdB = IntentDefs.IsValidIndex(HB.IntentIndex) ? IntentDefs[HB.IntentIndex].ActionId : NAME_None;
		if (IdA != IdB) { return IdA.LexicalLess(IdB); }
		if (HA.AttackerId != HB.AttackerId) { return HA.AttackerId < HB.AttackerId; }
		return HA.IntentIndex < HB.IntentIndex; // ordine TOTALE
	});

	TSet<int32> MarkSpentOn;
	for (int32 h : HitOrder)
	{
		FRTHexAttackHit& Hit = Plan.Hits[h];
		if (Hit.Power <= 0 || MarkSpentOn.Contains(Hit.TargetId)) { continue; } // "attacco": un colpo che fa danno
		if (!Units.IsValidIndex(Hit.TargetId) || !Units.IsValidIndex(Hit.AttackerId)
			|| !Units[Hit.TargetId] || !Units[Hit.AttackerId])
		{
			continue;
		}
		ARTUnit* Target = Units[Hit.TargetId];
		if (!Target->HasStatus(TAG_Status_Marked)) { continue; }
		if (Units[Hit.AttackerId]->TeamId != Target->GetMarkedByTeam()) { continue; } // solo la squadra del marcatore
		if (!bMarkedBeforeBlast[Hit.TargetId])
		{
			// Marchio nato in questo Blast: vale solo per i colpi a priorita' PIU' ALTA, cioe' risolti dopo.
			const int32 HitPriority = IntentDefs.IsValidIndex(Hit.IntentIndex)
				? IntentDefs[Hit.IntentIndex].Priority : MAX_int32;
			if (HitPriority <= IncomingMarkPriority[Hit.TargetId]) { continue; }
		}
		Hit.Power += URTCombatLibrary::MarkedFirstHitBonus;
		MarkSpentOn.Add(Hit.TargetId);
	}
	// Consumo dopo il pass, non dentro: durante il pass `HasStatus` deve rispondere sullo stato congelato.
	for (int32 TargetId : MarkSpentOn)
	{
		Units[TargetId]->RemoveStatus(TAG_Status_Marked);
	}

	// Bonus condizionati alla COPPIA (chi colpisce, chi subisce): si applicano sui colpi del piano, dove
	// attaccante e bersaglio sono ancora entrambi noti — `FRTAttack` conserva solo il bersaglio, e dopo
	// `ToAttacks` l'informazione non esiste piu'. Dopo l'Intercept, perche' il bersaglio puo' essere cambiato:
	// il bonus lo decide chi il colpo lo incassa davvero.
	//
	// `Flux.LinearDischarge` +8 contro bersaglio `Status.Wet` (catalogo eroi §1). Non e' nella lista `Effects`
	// perche' non e' un danno fisso, e vale su OGNI colpo dell'azione finche' il bersaglio e' bagnato — non
	// solo sul primo, quindi non passa dai delta qui sotto.
	//
	// Due sorgenti di bagnato, e contano entrambe:
	//   - GIA' bagnato quando il Blast comincia (acqua bassa attraversata nel Dash, o turno precedente):
	//     `HasStatus` risponde di si', e vale per qualunque colpo;
	//   - bagnato IN QUESTO Blast (`Riva.PressureJet`, priorita' 50): vale solo per i colpi a priorita' piu'
	//     ALTA, cioe' risolti dopo. `LinearDischarge` ha priorita' 55, quindi la coordinazione funziona.
	// La seconda meta' mancava, ed e' il motivo per cui la combo firma della v0.1 non era eseguibile (#242).
	// L'ordine e' quello canonico di ADR-0003 §3, lo stesso del marchio: non ne nasce un secondo.
	//
	// LIMITE DICHIARATO (CP 8.2): il confronto e' su un `ActionId` scritto qui. E' l'unico bonus condizionale
	// del catalogo v0.1; quando ce ne sara' un secondo, la forma giusta e' un campo del catalogo azioni
	// («bonus X contro stato Y»), non un secondo `if`.
	for (FRTHexAttackHit& Hit : Plan.Hits)
	{
		if (!IntentDefs.IsValidIndex(Hit.IntentIndex)
			|| IntentDefs[Hit.IntentIndex].ActionId != FName(TEXT("Flux.LinearDischarge")))
		{
			continue;
		}
		if (!Units.IsValidIndex(Hit.TargetId) || !Units[Hit.TargetId])
		{
			continue;
		}
		const bool bWetBeforeBlast = Units[Hit.TargetId]->HasStatus(TAG_Status_Wet);
		const bool bWetFromThisBlast =
			IntentDefs[Hit.IntentIndex].Priority > IncomingWetPriority[Hit.TargetId];
		if (bWetBeforeBlast || bWetFromThisBlast)
		{
			Hit.Power = URTCombatLibrary::EffectiveAttackPower(Hit.Power, URTCombatLibrary::FluxWetDischargeBonus);
		}
	}

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
			// CP 16.2: la guardia copre il davanti. Se il colpo che la consuma arriva fuori dall'arco frontale
			// non vale — si TOGLIE una protezione, non si aggiunge danno.
			//
			// Quale colpo la consuma lo decide `ApplyFirstHitDelta`: il PRIMO dell'array, che e' l'ordine
			// canonico gia' fissato da `Plan.Hits`. Si guarda quello, non «un colpo qualsiasi da dietro»:
			// altrimenti un attacco frontale perderebbe la guardia per colpa di un secondo colpo alle spalle
			// che il delta non tocca nemmeno.
			const FRTHexAttackHit* FirstHit = Plan.Hits.FindByPredicate(
				[i](const FRTHexAttackHit& Hit) { return Hit.TargetId == i; });

			bool bGuardHolds = true;
			if (FirstHit && HexUnits.IsValidIndex(FirstHit->AttackerId))
			{
				bGuardHolds = URTHexCombatLibrary::IsInFrontalArc(
					HexUnits[i].Cell, HexUnits[i].Facing, HexUnits[FirstHit->AttackerId].Cell);
			}

			if (bGuardHolds)
			{
				FirstHitDelta[i] -= URTCombatLibrary::GuardFirstHitReduction;
			}
			else
			{
				FRTTurnLogEntry Bypassed;
				Bypassed.Phase = ERTMatchPhase::Blast;
				Bypassed.Category = ERTLogCategory::Facing;
				Bypassed.Outcome = static_cast<uint8>(ERTFacingOutcome::RearHitBypassedCover);
				Bypassed.SrcCell = HexUnits[FirstHit->AttackerId].Cell;
				Bypassed.TgtCell = HexUnits[i].Cell;
				Bypassed.Amount = static_cast<int32>(HexUnits[i].Facing);
				TurnLog.Add(Bypassed);
				AddLogEvent(FString::Printf(TEXT("%s: %s"), *Units[i]->GetName(),
					*URTTurnLogLibrary::DescribeEntry(Bypassed)));
			}
		}
		// Riduzione dichiarata dalle reazioni attivate (`Action.Deflect` e le reazioni d'eroe che ne riusano
		// la semantica): una reazione si attiva UNA volta, quindi vale sul colpo che l'ha innescata — stessa
		// meccanica di Guard, non una riduzione permanente del turno.
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
	// CON QUALE azione ogni bersaglio e' stato spostato (#307). **Due mappe, non una condivisa**, e per la
	// stessa ragione per cui `KnockFrom`/`KnockDist` e `PullToward`/`PullDist` sono gia' separate qui sopra.
	//
	// Una mappa sola sembrava bastare perche' «un bersaglio spinto da 2+ attaccanti non si sposta affatto»,
	// ma quel filtro conta le spinte e le trazioni SEPARATAMENTE: un bersaglio con una spinta da A e una
	// trazione da B ha `KnockCount == 1` e `PullCount == 1`, passa entrambi i filtri e **si sposta due volte**,
	// scrivendo due voci. Con una chiave sola la seconda `Add` sovrascrive la prima, e la voce della spinta
	// finirebbe per dichiarare l'azione di chi ha TIRATO. Su una PR che esiste per rendere il TurnLog
	// attribuibile sarebbe stato il difetto peggiore possibile: una causa scritta, precisa e falsa.
	TMap<ARTUnit*, FName> PushCauseBy;
	TMap<ARTUnit*, FName> PushCauseByBase;
	TMap<ARTUnit*, FName> PullCauseBy;
	TMap<ARTUnit*, FName> PullCauseByBase;
	AttackSrc.Reserve(Plan.Hits.Num());
	// IDENTITA' dell'azione per ogni colpo, paralleli ad `Attacks`/`AttackSrc` (CP 11.3, #79). Fino a qui le
	// voci di combattimento uscivano ANONIME: `ERTCombatOutcome` diceva «22 danni, eliminata» senza dire da
	// cosa. Con due attaccanti nello stesso Blast il replay non poteva attribuire il colpo, e la coppia
	// «azione base + profilo» di D-033 restava scritta nel catalogo e assente dalla traccia.
	//
	// Non e' un campo nuovo: `ActionId` e `BaseActionId` esistono gia' nella voce, sono gia' serializzati
	// (formato v3 e v5) e il primo entra gia' nell'hash. Riempirli non muove la versione del formato.
	TArray<FName> AttackActionId;
	TArray<FName> AttackBaseActionId;
	AttackActionId.Reserve(Plan.Hits.Num());
	AttackBaseActionId.Reserve(Plan.Hits.Num());
	for (const FRTHexAttackHit& Hit : Plan.Hits)
	{
		ARTUnit* Attacker = Units[Hit.AttackerId];
		ARTUnit* Victim = Units[Hit.TargetId];
		const URTActionData* Ability = IntentAbility.IsValidIndex(Hit.IntentIndex) ? IntentAbility[Hit.IntentIndex] : nullptr;
		const bool bHasDef = IntentDefs.IsValidIndex(Hit.IntentIndex);
		AttackSrc.Add(HexUnits[Hit.AttackerId].Cell);
		// `NAME_None` quando l'intento non ha una definizione: e' la stessa convenzione del campo, «non
		// dichiarata», e vale meno di un nome inventato qui.
		AttackActionId.Add(bHasDef ? IntentDefs[Hit.IntentIndex].ActionId : NAME_None);
		AttackBaseActionId.Add(bHasDef ? IntentDefs[Hit.IntentIndex].BaseActionId : NAME_None);

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
					// `Status.Marked` NO: l'ha gia' applicato il pass a priorita' qui sopra, che ne conosce
					// anche la squadra. Riapplicarlo ora rimetterebbe in piedi un marchio appena speso —
					// e senza provenienza, quindi inutilizzabile ma visibile nell'HUD.
					if (Event.StatusTag == TAG_Status_Marked) { break; }
					StatusTargets.Add(Victim);
					StatusTags.Add(Event.StatusTag);
					StatusDurations.Add(Event.Amount);
					break;

				case ERTActionEffect::Push:
					// `PushResistance` e' una SOGLIA, non una sottrazione (D-038): chi la possiede regge le
					// spinte fino a quel valore e cede a quelle piu' forti, per intero. La forma da `Action.Guard`,
					// che dal CP 5.2 fa gia' esattamente questo con `GuardResistedPushDistance` — due
					// resistenze alla spinta nello stesso combat con due semantiche diverse sarebbero state la
					// prima cosa da spiegare a chi bilancia (#241).
					//
					// La regola sta QUI, nel punto in cui la distanza viene registrata, e non nei sette
					// produttori di spinta del catalogo: il settimo nascerebbe gia' rotto.
					//
					// Il `Pull` tre casi piu' sotto **non** la applica, ed e' deliberato: il catalogo v0.1 §1
					// riserva la resistenza di Guard alla spinta, e una trazione non viene resistita da chi si
					// e' piantato. Estenderla qui avrebbe reso `PushResistance` piu' forte di `Guard` senza che
					// nessuno lo avesse deciso.
					if (Victim && Event.Amount <= Victim->PushResistance)
					{
						break; // spinta assorbita: non si registra, quindi non c'e' niente da risolvere
					}
					KnockFrom.Add(Victim, HexUnits[Hit.AttackerId].Cell);
					KnockDist.Add(Victim, Event.Amount);
					KnockCount.FindOrAdd(Victim)++;
					PushCauseBy.Add(Victim, IntentDefs.IsValidIndex(Hit.IntentIndex)
						? IntentDefs[Hit.IntentIndex].ActionId : NAME_None);
					PushCauseByBase.Add(Victim, IntentDefs.IsValidIndex(Hit.IntentIndex)
						? IntentDefs[Hit.IntentIndex].BaseActionId : NAME_None);
					break;

				case ERTActionEffect::Pull:
					PullToward.Add(Victim, HexUnits[Hit.AttackerId].Cell);
					PullDist.Add(Victim, Event.Amount);
					PullCount.FindOrAdd(Victim)++;
					PullCauseBy.Add(Victim, IntentDefs.IsValidIndex(Hit.IntentIndex)
						? IntentDefs[Hit.IntentIndex].ActionId : NAME_None);
					PullCauseByBase.Add(Victim, IntentDefs.IsValidIndex(Hit.IntentIndex)
						? IntentDefs[Hit.IntentIndex].BaseActionId : NAME_None);
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
	AttackActionId.Append(CounterActionId);
	AttackBaseActionId.Append(CounterBaseActionId);

	if (Attacks.Num() == 0)
	{
		// Nessun colpo, ma le cure vanno applicate lo stesso: un supporto che cura fuori da uno scontro e' il
		// caso NORMALE, non un'eccezione. (Difetto trovato da `Actions.Heal.RestoresWithoutExceedingMax`: la
		// prima stesura usciva di qui e la cura spariva in silenzio.)
		ApplyPlannedHeals(HealTargets, HealAmounts, HealSources);
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
		// CHI ha colpito, e di quale azione generica e' un profilo (CP 11.3 · D-033). Gli array sono paralleli
		// ad `Attacks` per costruzione: `AttackSrc` lo era gia', questi due sono stati riempiti e accodati
		// negli stessi due punti. `IsValidIndex` non e' difesa contro un bug ipotetico — e' che `Attacks`
		// passa da `ApplyDamageDelta`, e un giorno un delta potrebbe non conservare la cardinalita'.
		if (AttackActionId.IsValidIndex(a))     { E.ActionId = AttackActionId[a]; }
		if (AttackBaseActionId.IsValidIndex(a)) { E.BaseActionId = AttackBaseActionId[a]; }
		TurnLog.Add(E);
	}

	for (int32 i = 0; i < Units.Num(); ++i)
	{
		Units[i]->ApplyCombatState(Resolved[i].Health, Resolved[i].Shield); // solo logico: rimozione visiva differita
	}

	ApplyPlannedHeals(HealTargets, HealAmounts, HealSources);

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
			// In v0.1 una spinta piu' forte NON ESISTE: il catalogo si ferma a 1, quindi questo ramo assorbe
			// ogni spinta del gioco e il ramo `Braced` sotto non aggiunge copertura. Vedi il commento la'.
			if (T->HasStatus(TAG_Status_Guarded) && KnockDist[T] <= URTCombatLibrary::GuardResistedPushDistance)
			{
				AddLogEvent(FString::Printf(TEXT("%s: in guardia, resiste alla spinta"), *T->GetName()));
				continue;
			}

			// `Action.Brace` (CP 5.2) "impedisce la PRIMA spinta". "Prima" e non "tutte" e' rispettato per
			// costruzione, non da un contatore: tutte le spinte del Blast si risolvono in questo unico
			// passaggio, e un bersaglio spinto da 2+ attaccanti e' gia' escluso sopra (`*Pushes != 1`,
			// forze contraddittorie).
			//
			// Il ramo non guarda `KnockDist`, quindi regge una spinta di qualunque distanza. ATTENZIONE:
			// questo NON e' cio' che distingue `Brace` da `Guard` in v0.1, e il commento diceva il contrario
			// fino al 2026-08-10. Il catalogo ha un solo valore di spinta, `1`, quindi il ramo `Guarded`
			// sopra intercetta gia' OGNI spinta del gioco e questo ramo non vede mai un caso che quello non
			// avrebbe retto. La differenza fra le due difese in v0.1 e' solo il danno (-15 sul primo colpo
			// contro -10 su ogni colpo): D-073, uscita (B) di #400. Pinnato da
			// `Spec.Brace.GuardAndBraceOnMixedHit` e `Spec.Brace.BraceWinsOnSecondHit`.
			// Il ramo resta senza limite di distanza perche' e' gratis tenerlo corretto per una v0.2 che
			// introduca una spinta >= 2; non perche' oggi si osservi.
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

			// Celle attraversate dalla spinta: la linea esagonale fra le due celle, i cui passi sono adiacenti
			// per costruzione. Serve al playback E agli hazard — «lo spostamento forzato ignora il costo
			// VOLONTARIO del terreno, non la geometria e non gli hazard» (spec-tassonomia-movimento §3).
			const TArray<FRTCellId> KPath = URTHexLibrary::HexLine(OldCell, NewCell);

			// La SPINTA nel TurnLog (#307). Prima esisteva solo come riga di combat log — una stringa per
			// l'HUD, non una voce di traccia: il replay registrava il danno e taceva lo spostamento, e chi
			// rileggeva il file vedeva l'unita' altrove senza nulla che lo spiegasse.
			AppendDisplacementEntry(T, OldCell, NewCell, KPath.Num() - 1, PushCauseBy, PushCauseByBase);

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

			// Chi subisce una spinta si gira verso CHI l'ha spinto (CP 16.1, ADR-0005 §3). Con la cella nuova,
			// quindi la voce di log porta la posizione in cui l'unita' e' finita.
			//
			// Un Move volontario successivo VINCE: risolve dopo, nella sua fase, e riscrive. Non serve un flag
			// che ricordi «e' stata spinta» — e' l'ORDINE delle fasi a deciderlo, ed e' il motivo per cui
			// `URTFacingLibrary` non tiene stato.
			{
				FRTHexSimUnit Pushed(0, NewCell, /*InMoveBudget=*/ 0);
				Pushed.Facing = T->Facing;
				const ERTHexDirection Turned = URTFacingLibrary::FacingAfterDisplacement(
					NewCell, KnockFrom[T], ERTDisplacementCause::Forced, Pushed.Facing);
				URTFacingLibrary::RecordFacingChange(Pushed, Turned,
					ERTFacingOutcome::TurnedToDisplacementSource, ERTMatchPhase::Blast, TurnLog);
				T->Facing = Pushed.Facing;
			}

			T->SetVisualLocation(T->WorldForCell(NewCell, HexOrigin, HexSize, HexLayerH));

			// Gli hazard di OGNI cella attraversata, non solo di quella d'arrivo (#308). Chi viene spinto
			// attraverso `asciutto -> fuoco -> fuoco -> asciutto` ha attraversato quelle due celle di fuoco e ne
			// subisce le conseguenze, pur non avendo speso un solo punto movimento: il costo e' cio' che si paga
			// per SCEGLIERE di passare, la geometria e' cio' che c'e'.
			//
			// Fino a qui `KPath` serviva solo all'animazione, e il commento diceva che il danno da
			// attraversamento «torna con l'ambiente attivo (epic E8)». E8 e' atterrata e questo punto non e'
			// stato ripassato: era un rinvio scaduto, non una decisione.
			ApplyTerrainOnEnterEffects(Map, T, CellsEnteredAlong(KPath));

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

			// La TRAZIONE nel TurnLog (#307), stessa voce della spinta. Le due si distinguono senza un esito
			// dedicato: `SrcCell -> TgtCell` si avvicina alla sorgente invece di allontanarsene, e la sorgente
			// e' quella del colpo che porta lo stesso `ActionId`.
			AppendDisplacementEntry(T, OldCell, NewCell, PPath.Num() - 1, PullCauseBy, PullCauseByBase);
			{
				FRTResolvedEvent Ev;
				Ev.Phase = ERTMatchPhase::Blast;
				Ev.Type = ERTResolvedEventType::Move;
				Ev.Source = T;
				Ev.Path = PPath;
				ResolvedTimeline.Add(Ev);
			}

			T->Cell = NewCell;

			// Come la spinta: chi viene trascinato guarda chi lo tira. La sorgente e' `PullToward`, che e' gia'
			// la cella verso cui si viene attirati — con la trazione le due cose coincidono.
			{
				FRTHexSimUnit Pulled(0, NewCell, /*InMoveBudget=*/ 0);
				Pulled.Facing = T->Facing;
				const ERTHexDirection Turned = URTFacingLibrary::FacingAfterDisplacement(
					NewCell, PullToward[T], ERTDisplacementCause::Forced, Pulled.Facing);
				URTFacingLibrary::RecordFacingChange(Pulled, Turned,
					ERTFacingOutcome::TurnedToDisplacementSource, ERTMatchPhase::Blast, TurnLog);
				T->Facing = Pulled.Facing;
			}

			T->SetVisualLocation(T->WorldForCell(NewCell, HexOrigin, HexSize, HexLayerH));

			// Gli hazard delle celle attraversate, come per la spinta (#308). La regola di
			// `spec-tassonomia-movimento` §3 parla di spostamento FORZATO, non di spinta: trattare le due vie
			// in modo diverso rifarebbe l'asimmetria `ModifyArc`/`DamageArc` corretta in #302, dove la stessa
			// disciplina era mantenuta per una via di uscita e non per l'altra.
			ApplyTerrainOnEnterEffects(Map, T, CellsEnteredAlong(PPath));

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
		// Orientamento (CP 16.1): lo snapshot lo porta perche' e' stato di gioco, e perche' il facing di fine
		// round e' quello di inizio del round dopo senza nessun travaso esplicito.
		SimUnit.Facing = OutUnits[i]->Facing;
		SimUnits.Add(SimUnit);
	}
	return URTHexSimLibrary::MakeSnapshot(Map, SimUnits);
}

void ARTTurnManager::ResolvePredictiveBoundary(const TArray<ARTUnit*>& Units, TArray<FRTHexMoveResult>& Resolved)
{
	if (ArmedPredictions.Num() == 0)
	{
		return;
	}

	// I colpi armati diventano dati puri: il pointer del tiratore ridiventa un INDICE, e l'ostilita' — che
	// e' l'unica cosa che il mondo sa e lo strato puro no — si risolve qui, una volta sola.
	TArray<FRTPredictiveShot> Shots;
	TArray<int32> ArmedIndexForShot; // per ritrovare l'armamento a cui ogni esito appartiene
	for (int32 a = 0; a < ArmedPredictions.Num(); ++a)
	{
		const FRTArmedPrediction& Armed = ArmedPredictions[a];
		ARTUnit* Shooter = Armed.Shooter.Get();
		if (!IsValid(Shooter) || !Shooter->IsAlive())
		{
			continue; // chi e' caduto nel Blast non spara: il colpo muore con lui, in silenzio
		}

		const int32 ShooterIdx = Units.IndexOfByKey(Shooter);
		if (ShooterIdx == INDEX_NONE)
		{
			continue;
		}

		FRTPredictiveShot Shot;
		Shot.SourceUnitId = ShooterIdx;
		Shot.LockedCell = Armed.LockedCell;
		Shot.ActionId = Armed.ActionId;
		for (int32 u = 0; u < Units.Num(); ++u)
		{
			if (IsValid(Units[u]) && Units[u]->TeamId != Shooter->TeamId)
			{
				Shot.Hostiles.Add(u);
			}
		}
		ArmedIndexForShot.Add(a);
		Shots.Add(Shot);
	}

	// Ripulisce SEMPRE, anche quando nessun colpo e' sopravvissuto: una previsione non vale due turni.
	const TArray<FRTArmedPrediction> Snapshot = ArmedPredictions;
	ArmedPredictions.Reset();

	if (Shots.Num() == 0)
	{
		return;
	}

	const TArray<FRTPredictiveResolution> Outcomes = URTPredictiveLibrary::ResolvePredictions(Shots, Resolved);
	URTPredictiveLibrary::ApplyPredictionsToMoves(Outcomes, Resolved);

	for (const FRTPredictiveResolution& Outcome : Outcomes)
	{
		if (!ArmedIndexForShot.IsValidIndex(Outcome.ShotIndex)) { continue; }
		const FRTArmedPrediction& Armed = Snapshot[ArmedIndexForShot[Outcome.ShotIndex]];
		ARTUnit* Shooter = Armed.Shooter.Get();
		if (!IsValid(Shooter)) { continue; }

		FRTTurnLogEntry Entry;
		Entry.Phase = ERTMatchPhase::Move;
		Entry.Category = ERTLogCategory::Predictive;
		Entry.ActionId = Armed.ActionId;
		Entry.BaseActionId = Armed.BaseActionId;
		Entry.SrcCell = Shooter->Cell;
		// La cella BLOCCATA anche sul whiff: e' li' che si e' scommesso, ed e' l'informazione che serve per
		// capire il turno. Una voce che dicesse solo «a vuoto» non insegnerebbe niente a chi legge il replay.
		Entry.TgtCell = Armed.LockedCell;

		if (Outcome.bMatched && Units.IsValidIndex(Outcome.VictimUnitId))
		{
			ARTUnit* Victim = Units[Outcome.VictimUnitId];
			const FRTDamageResult Result = URTCombatLibrary::ApplyDamage(Armed.Damage, Victim->Shield, Victim->Health);
			Victim->ApplyCombatState(Result.Health, Result.Shield);

			Entry.Outcome = static_cast<uint8>(ERTPredictiveOutcome::TriggerMatched);
			Entry.Amount = Armed.Damage;
			TurnLog.Add(Entry);

			AddLogEvent(FString::Printf(TEXT("%s: previsione azzeccata, %d danni a %s"),
				*Shooter->GetName(), Armed.Damage, *Victim->GetName()));
		}
		else
		{
			Entry.Outcome = static_cast<uint8>(ERTPredictiveOutcome::PredictionWhiffed);
			Entry.Amount = 0;
			TurnLog.Add(Entry);

			// Il whiff si SENTE: e' il `Misplay / Failure State` di D-032, e tacerlo lo renderebbe
			// indistinguibile da un turno in cui nessuno ha dichiarato niente.
			AddLogEvent(FString::Printf(TEXT("%s: previsione a vuoto, nessuno e' entrato"), *Shooter->GetName()));
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

	FVector Origin; float HexSize; float LayerH;
	GetHexContext(Origin, HexSize, LayerH);

	TArray<ARTUnit*> Units;
	const FRTHexSnapshot Snapshot = MakeCurrentSnapshot(Units);

	TArray<TArray<FRTCellId>> Paths;
	Paths.Reserve(Units.Num());
	// Chi e' stato accorciato dalla TOPOLOGIA: il resolver non puo' saperlo (il taglio avviene prima che lui
	// veda il percorso) e classificherebbe `Moved`, vero sul percorso troncato ma falso su cio' che l'unita'
	// aveva pianificato. Lo sa questo ciclo, e lo scrive lui nel log.
	TArray<bool> bStoppedByTopology;
	bStoppedByTopology.Init(false, Units.Num());
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

		// TOPOLOGIA (CP 9.3): il percorso e' stato validato quando la mappa era un'altra — una porta chiusa
		// nel Blast di QUESTO turno, un muro caduto — e `TruncatePathToBudget` non se ne accorge, perche'
		// guarda il budget. Senza questo taglio un percorso gia' pianificato attraverserebbe un varco che nel
		// frattempo si e' chiuso: il «path fantasma». Il movimento si FERMA all'ultima cella valida
		// (`Fallback.Stop`), non si annulla.
		const int32 PlannedLength = Path.Num();
		Path = URTHexSimLibrary::TruncatePathToTopology(Snapshot, Path);
		bStoppedByTopology[i] = Path.Num() < PlannedLength;

		Paths.Add(Path);
	}

	TArray<FRTHexMoveResult> Resolved = URTHexSimLibrary::ResolveHexPaths(Paths);

	// BOUNDARY DELLA PREDICTIVE ACTION (E18 CP 18.2). Sta QUI — dopo che le rotte sono calcolate, prima che
	// il log sia costruito e le posizioni applicate — perche' e' l'unico punto in cui esistono entrambe le
	// informazioni che servono: chi ha ATTRAVERSATO una cella e chi ci si e' fermato. Un controllo sulla sola
	// posizione finale mancherebbe chi ci passa sopra e prosegue, che e' il caso normale.
	//
	// Il troncamento avviene prima di `BuildMoveLog` proprio perche' il log dica la verita': una voce Move
	// costruita sulla rotta piena racconterebbe un movimento che non e' avvenuto.
	ResolvePredictiveBoundary(Units, Resolved);

	// TurnLog dagli esiti: la chiave e' la cella di PARTENZA (Paths[i][0]), stabile perche' Cell cambia
	// dopo PlaceOnCell. BuildMoveLog produce una voce per unita' nell'ordine dell'input.
	// Causa dichiarata (#307): questo e' il movimento VOLONTARIO della fase Move. Scatto e spostamento
	// forzato hanno altri produttori e dichiareranno la propria.
	TArray<FRTTurnLogEntry> MoveLog = URTHexSimLibrary::BuildMoveLog(Paths, Resolved, TEXT("Action.Move"));
	for (int32 i = 0; i < MoveLog.Num(); ++i)
	{
		// Il reason code della topologia sostituisce quello del resolver solo se l'unita' ha davvero percorso
		// tutto cio' che le restava: se si e' fermata anche per un'unita' o per una cella contesa, quel motivo
		// e' avvenuto DOPO il taglio ed e' la spiegazione piu' vicina a cio' che il giocatore ha visto.
		if (bStoppedByTopology.IsValidIndex(i) && bStoppedByTopology[i]
			&& Resolved[i].Outcome == ERTMoveOutcome::Moved)
		{
			MoveLog[i].Outcome = static_cast<uint8>(ERTMoveOutcome::BlockedByTopology);
		}
	}
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
		ApplyTerrainOnEnterEffects(Snapshot.Map, Units[i], Resolved[i].Entered);
	}

	// Orientamento di fine Move (CP 16.1, `FacingFinalAfterMove` di D-020). Si deriva dalla rotta EFFETTIVA —
	// partenza piu' celle davvero attraversate — non dal percorso pianificato: un'unita' fermata a meta' strada
	// da una cella contesa guarda dove e' arrivata, non dove voleva andare.
	//
	// Dopo `PlaceOnCell`, quindi la voce di log porta la cella finale come chiave. E' l'ultima scrittura del
	// round: il Move risolve per ultimo, e questo valore persiste nel round successivo.
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		if (Resolved[i].Entered.Num() == 0)
		{
			continue; // chi non si e' mosso non deriva nessun orientamento
		}

		TArray<FRTCellId> Walked;
		Walked.Reserve(Resolved[i].Entered.Num() + 1);
		Walked.Add(Paths[i].Num() > 0 ? Paths[i][0] : Units[i]->Cell);
		Walked.Append(Resolved[i].Entered);

		FRTHexSimUnit Moved(i, Units[i]->Cell, /*InMoveBudget=*/ 0);
		Moved.Facing = Units[i]->Facing;
		const ERTHexDirection Derived = URTFacingLibrary::FacingFromPath(Walked, Moved.Facing);
		URTFacingLibrary::RecordFacingChange(Moved, Derived, ERTFacingOutcome::DerivedFromMove,
			ERTMatchPhase::Move, TurnLog);
		Units[i]->Facing = Moved.Facing;

		// Il Move e' a BUDGET: le rotazioni legali saranno tre (l'ultimo passo e le due adiacenti). Sovrascrive
		// l'eventuale traccia dello scatto perche' il Move risolve dopo ed e' l'ultimo movimento del round.
		Units[i]->MovementStyleThisTurn = ERTMovementStyle::Budget;
		Units[i]->WalkedThisTurn = Walked;
	}

	// Rotazione DICHIARATA in pianificazione (D-020, #291). Ultimo passo del round, DOPO l'orientamento
	// derivato: e' quello il `Current` su cui si misura la legalita' — «una accanto» vuol dire accanto a dove
	// si e' arrivati, non a dove si era partiti. Chi non si e' mosso ha stile `None` e ruota libero: e' il caso
	// «resto fermo e mi giro», che prima di questo passaggio non era esprimibile.
	//
	// Una dichiarazione ILLEGALE viene rifiutata, non corretta verso la legale piu' vicina: in una fase
	// simultanea il giocatore non potrebbe accorgersi della correzione, e si ritroverebbe a subire un
	// orientamento che non ha scelto. Il rifiuto lascia una voce nel TurnLog proprio perche' NON cambia nulla:
	// senza, sarebbe indistinguibile da una dichiarazione mai fatta.
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		ARTUnit* Unit = Units[i];
		if (!IsValid(Unit)) { continue; }
		if (!Unit->bDeclaresPlannedFacing || !Unit->IsAlive())
		{
			Unit->ClearDeclaredFacing();
			continue;
		}

		ERTHexDirection Applied = Unit->Facing;
		const bool bLegal = URTFacingLibrary::TryApplyDeclaredFacing(Unit->MovementStyleThisTurn,
			Unit->WalkedThisTurn, Unit->Facing, Unit->PlannedFacing, Applied);

		FRTHexSimUnit Declaring(i, Unit->Cell, /*InMoveBudget=*/ 0);
		Declaring.Facing = Unit->Facing;
		URTFacingLibrary::RecordFacingChange(Declaring, Applied,
			bLegal ? ERTFacingOutcome::DeclaredInPlanning : ERTFacingOutcome::DeclarationRejected,
			ERTMatchPhase::Move, TurnLog);
		Unit->Facing = Declaring.Facing;

		AddLogEvent(FString::Printf(TEXT("%s: rotazione dichiarata %s"), *Unit->GetName(),
			bLegal ? TEXT("applicata") : TEXT("RIFIUTATA (illegale per lo stile di movimento)")));
		Unit->ClearDeclaredFacing();
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
	// La mesh ATTERRA sul facing logico (CP 16.1). Durante il playback lo yaw interpola verso la direzione di
	// marcia, che e' presentazione; a fine risoluzione deve coincidere con il valore che le REGOLE useranno nel
	// turno dopo — altrimenti il giocatore legge un orientamento e ne subisce un altro quando arriva la difesa
	// direzionale (CP 16.2).
	//
	// Qui e non alla fine dell'animazione: `SkipPlayback` passa da questa funzione, quindi saltare la
	// risoluzione non deve lasciare la figura girata dalla parte sbagliata.
	{
		TArray<AActor*> AllUnitsForFacing;
		UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), AllUnitsForFacing);
		for (AActor* UnitActor : AllUnitsForFacing)
		{
			ARTUnit* U = Cast<ARTUnit>(UnitActor);
			if (!U || !U->IsAlive()) { continue; }

			const FVector Here = U->WorldForCell(U->Cell, PBOrigin, PBCellSize, PBLayerHeight);
			const FRTCellId Ahead = URTHexLibrary::Neighbor(U->Cell, U->Facing);
			const FVector There = U->WorldForCell(Ahead, PBOrigin, PBCellSize, PBLayerHeight);
			U->SetActorRotation(FRotator(0.f, URTPlaybackLibrary::DirectionYaw(Here, There), 0.f));
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
// Spec: docs/gameplay/spec-pacing-turno.md

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
