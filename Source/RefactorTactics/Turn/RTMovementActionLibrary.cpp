#include "Turn/RTMovementActionLibrary.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"

namespace
{
	/** La cella esiste sulla mappa ed e' percorribile (il muro e' un dato della cella, non un'eccezione). */
	bool IsWalkable(const URTHexMapAsset* Map, const FRTCellId& Cell)
	{
		const FRTHexCellData* Data = Map ? Map->FindCell(Cell) : nullptr;
		return Data != nullptr && !Data->bBlocksMovement;
	}
}

bool URTMovementActionLibrary::IsStraightLine(const FRTCellId& From, const FRTCellId& To, int32& OutDistance)
{
	OutDistance = 0;
	if (From.Layer != To.Layer || From == To)
	{
		return false; // le mobilita' lineari restano sul proprio piano; il salto di layer e' E9 (rampe/ponti)
	}

	const int32 DeltaX = To.X - From.X;
	const int32 DeltaY = To.Y - From.Y;

	// Allineata se lo scostamento e' un multiplo POSITIVO di una delle sei direzioni assiali.
	for (int32 D = 0; D < 6; ++D)
	{
		const FIntPoint Step = URTHexLibrary::AxialDirection(static_cast<ERTHexDirection>(D));
		for (int32 K = 1; K <= FMath::Max(FMath::Abs(DeltaX), FMath::Abs(DeltaY)); ++K)
		{
			if (Step.X * K == DeltaX && Step.Y * K == DeltaY)
			{
				OutDistance = K;
				return true;
			}
		}
	}
	return false;
}

FRTLinearMoveResult URTMovementActionLibrary::ResolveLinearMove(const URTHexMapAsset* Map,
	const FRTCellId& From, const FRTCellId& Target, int32 MaxCells, ERTMovementStyle Style,
	const TMap<FRTCellId, int32>& Occupancy, const TSet<int32>& Hostiles)
{
	FRTLinearMoveResult Result;
	Result.Final = From;
	Result.Stop = ERTLinearStop::NotAligned;

	// FAIL-CLOSED: senza mappa autorevole non si sa cosa ci sia davanti, quindi non ci si muove.
	int32 Distance = 0;
	if (Map == nullptr || MaxCells <= 0 || !IsStraightLine(From, Target, Distance) || Distance > MaxCells)
	{
		return Result;
	}

	const FIntPoint Step(Target.X - From.X, Target.Y - From.Y);
	const FIntPoint UnitStep(Step.X / Distance, Step.Y / Distance);

	// Il salto guarda solo dove atterra: le celle in mezzo non lo riguardano (unita', coperture basse), ma la
	// cella d'arrivo si' — la si "subisce", quindi dev'essere percorribile e libera.
	if (Style == ERTMovementStyle::LinearLeap)
	{
		if (!IsWalkable(Map, Target))
		{
			Result.Stop = ERTLinearStop::BlockedByTerrain;
			return Result;
		}
		if (Occupancy.Contains(Target))
		{
			Result.Stop = ERTLinearStop::BlockedByUnit;
			return Result;
		}
		Result.Final = Target;
		Result.Entered.Add(Target);
		Result.Stop = ERTLinearStop::Completed;
		return Result;
	}

	// Dash, Charge e Reposition avanzano una cella alla volta: cio' che incontrano le ferma.
	FRTCellId Current = From;
	for (int32 K = 1; K <= Distance; ++K)
	{
		const FRTCellId Next(From.X + UnitStep.X * K, From.Y + UnitStep.Y * K, From.Layer);

		if (!IsWalkable(Map, Next))
		{
			Result.Stop = ERTLinearStop::BlockedByTerrain; // muro, bordo mappa o cella che blocca il passo
			break;
		}

		if (const int32* Occupant = Occupancy.Find(Next))
		{
			// La carica si ferma ADDOSSO al nemico (impatto); per tutti gli altri e' solo un ostacolo.
			Result.Stop = (Style == ERTMovementStyle::LinearCharge && Hostiles.Contains(*Occupant))
				? ERTLinearStop::Impact
				: ERTLinearStop::BlockedByUnit;
			if (Result.Stop == ERTLinearStop::Impact)
			{
				Result.ImpactUnitId = *Occupant;
			}
			break;
		}

		Current = Next;
		Result.Entered.Add(Next);
		if (K == Distance)
		{
			Result.Stop = ERTLinearStop::Completed;
		}
	}

	Result.Final = Current;
	return Result;
}
