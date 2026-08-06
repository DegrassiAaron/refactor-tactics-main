#include "Combat/RTHexCombatLibrary.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"
#include "Terrain/RTTerrainLibrary.h"

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
		if (!Units.IsValidIndex(Intent.AttackerId))
		{
			continue;
		}

		const FRTHexCombatUnit& Attacker = Units[Intent.AttackerId];
		if (!Attacker.bAlive)
		{
			continue;
		}

		// Il bersaglio puo' essere un'unita' oppure una CELLA (`Fallback.AttackCell`: il bersaglio e' sparito,
		// l'area parte comunque dove era stata puntata). Da qui in poi conta solo dove si mira.
		const bool bTargetsUnit = Units.IsValidIndex(Intent.TargetId);
		const FRTCellId AimCell = bTargetsUnit ? Units[Intent.TargetId].Cell : Intent.TargetCell;

		if (bTargetsUnit)
		{
			const FRTHexCombatUnit& Target = Units[Intent.TargetId];
			if (!Target.bAlive || Attacker.TeamId == Target.TeamId)
			{
				continue; // bersaglio non ingaggiabile: nessun esito da registrare
			}
		}

		// Il Fumo lungo la linea di tiro CAPPA la portata effettiva (indipendente da RangeCells): riusa
		// HexLine, la stessa primitiva di HexHitCells per Shape::Line. Guardato da Map != nullptr perche'
		// serve la cella; se Map e' nullo il path fail-closed a valle scarta comunque l'intento.
		int32 EffectiveRange = Intent.RangeCells;
		if (Map != nullptr)
		{
			for (const FRTCellId& LineCell : URTHexLibrary::HexLine(Attacker.Cell, AimCell))
			{
				const FRTHexCellData* LineCellData = Map->FindCell(LineCell);
				if (LineCellData)
				{
					const FRTTerrainDef Terrain = URTTerrainLibrary::FindTerrainDef(LineCellData->Surface);
					if (Terrain.MaxTargetingRangeThrough > 0)
					{
						EffectiveRange = FMath::Min(EffectiveRange, Terrain.MaxTargetingRangeThrough);
					}
				}
			}
		}

		if (URTHexLibrary::HexDistance(Attacker.Cell, AimCell) > EffectiveRange)
		{
			continue; // fuori portata (anche per il cap del Fumo): scartato in silenzio (come il quadrato)
		}

		// FAIL-CLOSED: senza mappa autorevole non si colpisce. Il motivo resta pero' DISTINTO da una
		// copertura: «non valutabile» e' un difetto di configurazione del livello, non un esito di gioco.
		if (Map == nullptr)
		{
			Plan.UnverifiableIntents.Add(IntentIdx);
			continue;
		}
		if (!URTHexVisionLibrary::HasLineOfSight(Map, Attacker.Cell, AimCell))
		{
			Plan.BlockedIntents.Add(IntentIdx);
			continue;
		}

		const TArray<FRTCellId> HitCells =
			HexHitCells(Intent.Shape, Attacker.Cell, AimCell, Intent.RangeCells, Intent.AreaRadius);

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
	Plan.UnverifiableIntents.Sort();

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
