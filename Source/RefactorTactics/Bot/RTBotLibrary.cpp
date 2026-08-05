#include "Bot/RTBotLibrary.h"
#include "Grid/RTGridLibrary.h"

int32 URTBotLibrary::ScorePlan(const FRTBotPlan& Plan, const FRTBotContext& Context)
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
	for (int32 i = 0; i < NumEnemies; ++i)
	{
		const int32 Dist = URTGridLibrary::ManhattanDistance(Plan.DestCell, Context.Enemies[i]);
		if (Dist <= Context.EnemyRanges[i]
			&& URTGridLibrary::HasLineOfSight(Context.Enemies[i], Plan.DestCell, Context.VisionBlockers))
		{
			Score -= Context.WThreat; // la cella e' sotto tiro E in linea di vista di questo nemico (copertura)
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

FRTBotPlan URTBotLibrary::ChooseBestPlan(const TArray<FRTBotPlan>& Candidates, const FRTBotContext& Context)
{
	// Fallback: nessuna candidata -> resta a Origin (invariante: c'e' sempre un piano valido).
	FRTBotPlan Best;
	Best.DestCell = Context.Origin;
	if (Candidates.Num() == 0)
	{
		return Best;
	}

	int32 BestScore = TNumericLimits<int32>::Lowest();
	int32 BestMove = MAX_int32;
	bool bHave = false;
	for (const FRTBotPlan& Cand : Candidates)
	{
		const int32 CandScore = ScorePlan(Cand, Context);
		const int32 CandMove = URTGridLibrary::ManhattanDistance(Cand.DestCell, Context.Origin);

		bool bBetter = !bHave || CandScore > BestScore;
		if (!bBetter && CandScore == BestScore)
		{
			// Tie-break ASSOLUTO: mossa minima da Origin, poi coord (X,Y,Layer) crescenti.
			// Non dipende dall'ordine di enumerazione -> permutare le candidate non cambia l'esito.
			if (CandMove < BestMove)
			{
				bBetter = true;
			}
			else if (CandMove == BestMove)
			{
				const FRTCellId& D = Cand.DestCell;
				const FRTCellId& C = Best.DestCell;
				if (D.X != C.X)      { bBetter = D.X < C.X; }
				else if (D.Y != C.Y) { bBetter = D.Y < C.Y; }
				else                 { bBetter = D.Layer < C.Layer; }
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

FRTCellId URTBotLibrary::StepToward(const FRTCellId& From, const FRTCellId& Target, int32 MoveRange)
{
	const int32 Distance = FMath::Abs(Target.X - From.X) + FMath::Abs(Target.Y - From.Y);
	if (Distance <= 1)
	{
		return From; // gia' adiacente o sulla stessa cella: non muoversi
	}

	// Budget di passi, lasciando almeno una cella tra bot e bersaglio (non sovrapporsi).
	int32 Budget = FMath::Min(MoveRange, Distance - 1);

	FRTCellId Result = From;

	// Avvicinamento greedy: prima lungo X, poi lungo Y.
	const int32 DX = Target.X - Result.X;
	const int32 StepX = FMath::Min(Budget, FMath::Abs(DX));
	Result.X += FMath::Sign(DX) * StepX;
	Budget -= StepX;

	const int32 DY = Target.Y - Result.Y;
	const int32 StepY = FMath::Min(Budget, FMath::Abs(DY));
	Result.Y += FMath::Sign(DY) * StepY;

	return Result;
}

FRTCellId URTBotLibrary::BestApproachCell(const FRTCellId& From, const FRTCellId& Target, int32 MoveRange,
	const TMap<FRTCellId, int32>& CellCost, int32 Width, int32 Height, const TArray<FRTTraversalEdge>& Edges)
{
	// Celle raggiungibili col budget di costo, aggirando ostacoli/terreno costoso e usando gli archi (rampe).
	const TArray<FRTCellId> Reachable = URTGridLibrary::ReachableCellsByGraph(From, MoveRange, CellCost, Edges, Width, Height);

	FRTCellId Best = From; // fermarsi e' sempre un'opzione valida
	int32 BestToTarget = FMath::Abs(Target.X - From.X) + FMath::Abs(Target.Y - From.Y);
	int32 BestFromOrigin = 0;

	for (const FRTCellId& Cell : Reachable)
	{
		if (Cell == Target)
		{
			continue; // non sovrapporsi al bersaglio
		}
		const int32 ToTarget = FMath::Abs(Target.X - Cell.X) + FMath::Abs(Target.Y - Cell.Y);
		const int32 FromOrigin = FMath::Abs(Cell.X - From.X) + FMath::Abs(Cell.Y - From.Y);
		// Piu' vicino al bersaglio; a parita', mossa piu' corta.
		if (ToTarget < BestToTarget || (ToTarget == BestToTarget && FromOrigin < BestFromOrigin))
		{
			BestToTarget = ToTarget;
			BestFromOrigin = FromOrigin;
			Best = Cell;
		}
	}
	return Best;
}

FRTCellId URTBotLibrary::BestKiteCell(const FRTCellId& From, const FRTCellId& Threat, int32 MoveRange,
	const TMap<FRTCellId, int32>& CellCost, int32 Width, int32 Height, const TArray<FRTTraversalEdge>& Edges)
{
	// Celle raggiungibili col budget di costo (aggira ostacoli/pericoli, usa gli archi per fuggire in quota).
	const TArray<FRTCellId> Reachable = URTGridLibrary::ReachableCellsByGraph(From, MoveRange, CellCost, Edges, Width, Height);

	FRTCellId Best = From;
	int32 BestToThreat = FMath::Abs(Threat.X - From.X) + FMath::Abs(Threat.Y - From.Y);
	int32 BestFromOrigin = 0;

	for (const FRTCellId& Cell : Reachable)
	{
		const int32 ToThreat = FMath::Abs(Threat.X - Cell.X) + FMath::Abs(Threat.Y - Cell.Y);
		const int32 FromOrigin = FMath::Abs(Cell.X - From.X) + FMath::Abs(Cell.Y - From.Y);
		// Massimizza la distanza dalla minaccia; a parita', mossa piu' corta.
		if (ToThreat > BestToThreat || (ToThreat == BestToThreat && FromOrigin < BestFromOrigin))
		{
			BestToThreat = ToThreat;
			BestFromOrigin = FromOrigin;
			Best = Cell;
		}
	}
	return Best;
}

int32 URTBotLibrary::AttackScore(int32 Damage, int32 TargetHealth)
{
	if (Damage >= TargetHealth)
	{
		// Kill: priorità massima; tra i kill si preferisce il bersaglio più debole.
		return 100000 - TargetHealth;
	}
	// Non-kill: si preferisce il danno maggiore.
	return Damage;
}

bool URTBotLibrary::AttackIsBetter(int32 DamageA, int32 TargetHealthA, const FRTCellId& TargetCellA,
	int32 DamageB, int32 TargetHealthB, const FRTCellId& TargetCellB)
{
	const int32 ScoreA = AttackScore(DamageA, TargetHealthA);
	const int32 ScoreB = AttackScore(DamageB, TargetHealthB);
	if (ScoreA != ScoreB)
	{
		return ScoreA > ScoreB;
	}
	// A parità di AttackScore, tie-break ASSOLUTO sulle coord del bersaglio -> deterministico (invariante #4).
	if (TargetCellA.X != TargetCellB.X) { return TargetCellA.X < TargetCellB.X; }
	if (TargetCellA.Y != TargetCellB.Y) { return TargetCellA.Y < TargetCellB.Y; }
	return TargetCellA.Layer < TargetCellB.Layer;
}

FRTCellId URTBotLibrary::BestFiringCell(const FRTCellId& From, const FRTCellId& Target, int32 MoveRange,
	const TMap<FRTCellId, int32>& CellCost, int32 Width, int32 Height,
	const TArray<FRTCellId>& VisionBlockers, int32 AttackRange, const TArray<FRTTraversalEdge>& Edges)
{
	// Celle raggiungibili col budget di costo (grafo: aggira ostacoli/terreno, usa le rampe per salire).
	const TArray<FRTCellId> Reachable = URTGridLibrary::ReachableCellsByGraph(From, MoveRange, CellCost, Edges, Width, Height);

	FRTCellId Best = From; // sentinella: nessuna posizione di tiro trovata
	bool bFound = false;
	int32 BestLayer = TNumericLimits<int32>::Lowest();
	int32 BestMove = MAX_int32;

	for (const FRTCellId& Cell : Reachable)
	{
		if (Cell == Target)
		{
			continue; // non sovrapporsi al bersaglio
		}
		// Deve offrire un TIRO: gittata + linea di tiro (con LOS di elevazione dalla quota della cella).
		if (!URTGridLibrary::IsWithinRange(Cell, Target, AttackRange)
			|| !URTGridLibrary::HasLineOfSight(Cell, Target, VisionBlockers))
		{
			continue;
		}
		const int32 Move = URTGridLibrary::ManhattanDistance(From, Cell);
		// Preferisci l'alta quota (elevazione); a parità, la mossa più corta.
		if (!bFound || Cell.Layer > BestLayer || (Cell.Layer == BestLayer && Move < BestMove))
		{
			bFound = true;
			BestLayer = Cell.Layer;
			BestMove = Move;
			Best = Cell;
		}
	}
	return Best;
}
