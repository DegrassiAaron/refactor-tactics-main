#include "Bot/RTHexBotLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"
#include "Turn/RTHexSimLibrary.h"

int32 URTHexBotLibrary::ScorePlan(const URTHexMapAsset* Map, const FRTHexBotPlan& Plan, const FRTHexBotContext& Context)
{
	int32 Score = 0;

	// Focus-fire: danno inflitto, con bonus se il colpo uccide (danno >= HP+scudo del bersaglio).
	if (Plan.bHasAttack)
	{
		Score += Context.WDamage * Plan.AttackDamage;
		if (Plan.AttackDamage >= Plan.TargetHealth)
		{
			Score += Context.WKill;
		}
	}

	// Minaccia sulla cella di destinazione + posizionamento rispetto al nemico piu' vicino.
	const int32 NumEnemies = FMath::Min(Context.Enemies.Num(), Context.EnemyRanges.Num());
	int32 MinDist = MAX_int32;
	for (int32 I = 0; I < NumEnemies; ++I)
	{
		const int32 Dist = URTHexLibrary::HexDistance(Plan.DestCell, Context.Enemies[I]);
		if (Dist <= Context.EnemyRanges[I]
			&& URTHexVisionLibrary::HasLineOfSight(Map, Context.Enemies[I], Plan.DestCell))
		{
			Score -= Context.WThreat; // sotto tiro E in linea di vista di questo nemico (la copertura protegge)
		}
		MinDist = FMath::Min(MinDist, Dist);
	}

	if (MinDist != MAX_int32)
	{
		if (Context.KiteStandoff > 0)
		{
			// Kiter: penalita' proporzionale a quanto si resta SOTTO la distanza di sicurezza.
			if (MinDist < Context.KiteStandoff)
			{
				Score -= Context.WKiteViolation * (Context.KiteStandoff - MinDist);
			}
		}
		else
		{
			// Mischia: penalita' proporzionale alla distanza (chiudere la distanza e' meglio).
			Score -= Context.WApproach * MinDist;
		}
	}

	// Elevazione: premia la quota alta della cella di destinazione (vantaggio di tiro/posizione).
	Score += Context.WElevation * Plan.DestCell.Layer;

	return Score;
}

FRTHexBotPlan URTHexBotLibrary::ChooseBestPlan(const URTHexMapAsset* Map, const TArray<FRTHexBotPlan>& Candidates,
	const FRTHexBotContext& Context)
{
	// Fallback: nessuna candidata -> resta a Origin (c'e' sempre un piano valido).
	FRTHexBotPlan Best;
	Best.DestCell = Context.Origin;
	if (Candidates.Num() == 0)
	{
		return Best;
	}

	int32 BestScore = TNumericLimits<int32>::Lowest();
	int32 BestMove = MAX_int32;
	bool bHave = false;
	for (const FRTHexBotPlan& Cand : Candidates)
	{
		const int32 CandScore = ScorePlan(Map, Cand, Context);
		const int32 CandMove = URTHexLibrary::HexDistance(Cand.DestCell, Context.Origin);

		bool bBetter = !bHave || CandScore > BestScore;
		if (!bBetter && CandScore == BestScore)
		{
			// Tie-break ASSOLUTO: mossa minima da Origin, poi ordine stabile della cella (Layer, X, Y).
			// Non dipende dall'ordine di enumerazione -> permutare le candidate non cambia l'esito.
			if (CandMove < BestMove)
			{
				bBetter = true;
			}
			else if (CandMove == BestMove)
			{
				bBetter = URTHexLibrary::StableLess(Cand.DestCell, Best.DestCell);
			}
		}

		if (bBetter)
		{
			bHave = true;
			BestScore = CandScore;
			BestMove = CandMove;
			Best = Cand;
		}
	}
	return Best;
}

TArray<FRTHexBotPlan> URTHexBotLibrary::BuildCandidates(const FRTHexSnapshot& Snapshot, int32 UnitId,
	const FRTHexBotContext& Context)
{
	TArray<FRTHexBotPlan> Out;

	// Le celle raggiungibili hanno gia' rispettato budget, blocchi, occupanti e archi verticali: il bot non
	// rifa' pathfinding e non puo' proporre una mossa illegale.
	const TArray<FRTHexReachableCell> Reachable = URTHexSimLibrary::ReachableCells(Snapshot, UnitId);
	const int32 NumEnemies = FMath::Min3(Context.Enemies.Num(), Context.EnemyRanges.Num(), Context.EnemyHealth.Num());

	for (const FRTHexReachableCell& Cell : Reachable)
	{
		// Restare/spostarsi senza attaccare e' sempre un'opzione.
		FRTHexBotPlan Move;
		Move.DestCell = Cell.Cell;
		Out.Add(Move);

		if (Context.AttackRange <= 0 || Context.AttackDamage <= 0)
		{
			continue; // niente attacco disponibile: solo posizionamento
		}

		for (int32 I = 0; I < NumEnemies; ++I)
		{
			const FRTCellId& Enemy = Context.Enemies[I];
			if (URTHexLibrary::HexDistance(Cell.Cell, Enemy) > Context.AttackRange)
			{
				continue; // fuori gittata da questa cella
			}
			if (!URTHexVisionLibrary::HasLineOfSight(Snapshot.Map, Cell.Cell, Enemy))
			{
				continue; // linea di tiro interrotta
			}

			FRTHexBotPlan Attack;
			Attack.DestCell = Cell.Cell;
			Attack.bHasAttack = true;
			Attack.TargetIndex = I;
			Attack.AttackDamage = Context.AttackDamage;
			Attack.TargetHealth = Context.EnemyHealth[I];
			Out.Add(Attack);
		}
	}
	return Out;
}

FRTHexBotPlan URTHexBotLibrary::PlanUnit(const FRTHexSnapshot& Snapshot, int32 UnitId, const FRTHexBotContext& Context)
{
	return ChooseBestPlan(Snapshot.Map, BuildCandidates(Snapshot, UnitId, Context), Context);
}
