#include "Combat/RTHexCombatLibrary.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"

namespace
{
	/** Ordine canonico e senza duplicati: l'output non deve dipendere da come e' stato costruito. */
	void CanonicalizeCells(TArray<FRTCellId>& Cells)
	{
		Cells.Sort(URTHexLibrary::StableLess);
		for (int32 i = Cells.Num() - 1; i > 0; --i)
		{
			if (Cells[i] == Cells[i - 1]) { Cells.RemoveAt(i, EAllowShrinking::No); }
		}
	}
}

TArray<FRTCellId> URTHexCombatLibrary::HexHitCells(ERTAbilityShape Shape, const FRTCellId& From, const FRTCellId& Target,
	int32 RangeCells, int32 AreaRadius)
{
	TArray<FRTCellId> Cells;
	switch (Shape)
	{
	case ERTAbilityShape::Area:
		// Esagono pieno attorno al bersaglio (raggio 0 = solo la sua cella).
		Cells = URTHexLibrary::HexArea(Target, FMath::Max(0, AreaRadius));
		break;

	case ERTAbilityShape::Line:
		// Traiettoria attraversata, estremi inclusi da HexLine: la cella dell'attaccante non e' colpita
		// (parita' col quadrato, dove CellsInLine escludeva From).
		Cells = URTHexLibrary::HexLine(From, Target);
		Cells.Remove(From);
		break;

	case ERTAbilityShape::Cone:
		// Ventaglio 120 gradi verso il bersaglio (From gia' escluso da HexCone).
		Cells = URTHexLibrary::HexCone(From, Target, RangeCells);
		break;

	default:
		Cells.Add(Target);
		break;
	}

	CanonicalizeCells(Cells);
	return Cells;
}

FRTHexBlastPlan URTHexCombatLibrary::CollectHexAttacks(const TArray<FRTHexCombatUnit>& Units,
	const TArray<FRTHexAttackIntent>& Intents, const URTHexMapAsset* Map)
{
	FRTHexBlastPlan Plan;

	for (int32 IntentIdx = 0; IntentIdx < Intents.Num(); ++IntentIdx)
	{
		const FRTHexAttackIntent& Intent = Intents[IntentIdx];
		if (!Units.IsValidIndex(Intent.AttackerId) || !Units.IsValidIndex(Intent.TargetId))
		{
			continue;
		}

		const FRTHexCombatUnit& Attacker = Units[Intent.AttackerId];
		const FRTHexCombatUnit& Target = Units[Intent.TargetId];
		if (!Attacker.bAlive || !Target.bAlive || Attacker.TeamId == Target.TeamId)
		{
			continue; // bersaglio non ingaggiabile: nessun esito da registrare
		}

		if (URTHexLibrary::HexDistance(Attacker.Cell, Target.Cell) > Intent.RangeCells)
		{
			continue; // fuori portata: scartato in silenzio (come il quadrato)
		}

		// FAIL-CLOSED: senza mappa autorevole la linea di tiro non e' valutabile -> nessun colpo.
		if (Map == nullptr || !URTHexVisionLibrary::HasLineOfSight(Map, Attacker.Cell, Target.Cell))
		{
			Plan.BlockedIntents.Add(IntentIdx);
			continue;
		}

		const TArray<FRTCellId> HitCells =
			HexHitCells(Intent.Shape, Attacker.Cell, Target.Cell, Intent.RangeCells, Intent.AreaRadius);

		// Colpisce ogni nemico VIVO su una cella dell'area (niente fuoco amico, niente colpi sui morti).
		for (int32 u = 0; u < Units.Num(); ++u)
		{
			const FRTHexCombatUnit& Other = Units[u];
			if (!Other.bAlive || Other.TeamId == Attacker.TeamId)
			{
				continue;
			}
			if (HitCells.Contains(Other.Cell))
			{
				Plan.Hits.Add(FRTHexAttackHit(Intent.AttackerId, u, Intent.Power, IntentIdx));
			}
		}
	}

	// Ordine canonico: permutare gli intenti in ingresso non deve cambiare il piano in uscita.
	Plan.Hits.Sort([](const FRTHexAttackHit& A, const FRTHexAttackHit& B)
	{
		if (A.AttackerId != B.AttackerId) { return A.AttackerId < B.AttackerId; }
		if (A.TargetId != B.TargetId) { return A.TargetId < B.TargetId; }
		if (A.Power != B.Power) { return A.Power < B.Power; }
		return A.IntentIndex < B.IntentIndex; // ordine TOTALE: Sort non e' stabile
	});
	Plan.BlockedIntents.Sort();

	return Plan;
}

FRTCellId URTHexCombatLibrary::HexKnockbackDestination(const FRTCellId& Attacker, const FRTCellId& Target, int32 Distance,
	const URTHexMapAsset* Map, const TArray<FRTCellId>& Occupied)
{
	// FAIL-CLOSED: senza mappa autorevole non si sposta nessuno (non si sa cosa c'e' oltre il bersaglio).
	if (Distance <= 0 || Map == nullptr)
	{
		return Target;
	}

	// Direzione della spinta: l'ULTIMO passo della linea attaccante -> bersaglio, cioe' una delle sei
	// direzioni esagonali. La linea si calcola sul piano del bersaglio (il Layer non entra nella geometria).
	const FRTCellId From(Attacker.X, Attacker.Y, Target.Layer);
	if (From.X == Target.X && From.Y == Target.Y)
	{
		return Target; // stessa cella assiale: nessuna direzione da cui allontanarsi
	}

	const TArray<FRTCellId> Line = URTHexLibrary::HexLine(From, Target);
	if (Line.Num() < 2)
	{
		return Target;
	}
	const FRTCellId& Previous = Line[Line.Num() - 2];
	const int32 StepX = Target.X - Previous.X;
	const int32 StepY = Target.Y - Previous.Y;

	FRTCellId Current = Target;
	for (int32 Step = 0; Step < Distance; ++Step)
	{
		const FRTCellId Next(Current.X + StepX, Current.Y + StepY, Target.Layer);
		const FRTHexCellData* Data = Map->FindCell(Next);
		if (Data == nullptr || Data->bBlocksMovement || Occupied.Contains(Next))
		{
			break; // bordo della mappa, ostacolo o unita': ci si ferma sulla cella libera precedente
		}
		Current = Next;
	}
	return Current;
}

TArray<FRTAttack> URTHexCombatLibrary::ToAttacks(const FRTHexBlastPlan& Plan)
{
	TArray<FRTAttack> Attacks;
	Attacks.Reserve(Plan.Hits.Num());
	for (const FRTHexAttackHit& Hit : Plan.Hits)
	{
		Attacks.Add(FRTAttack(Hit.TargetId, Hit.Power));
	}
	return Attacks;
}
