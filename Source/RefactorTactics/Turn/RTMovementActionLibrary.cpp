#include "Turn/RTMovementActionLibrary.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Terrain/RTTerrainLibrary.h"

namespace
{
	/** La cella esiste sulla mappa ed e' percorribile (il muro e' un dato della cella, non un'eccezione). */
	bool IsWalkable(const URTHexMapAsset* Map, const FRTCellId& Cell)
	{
		const FRTHexCellData* Data = Map ? Map->FindCell(Cell) : nullptr;
		return Data != nullptr && !Data->bBlocksMovement;
	}

	/**
	 * La cella e' attraversabile da una mobilita' che AVANZA passo passo (Dash, Charge, Reposition).
	 *
	 * Oltre al muro c'e' il terreno: il catalogo dice che l'accidentato «Dash e Charge non lo attraversano»
	 * (E8/CP 8.1), e il dato che lo dichiara e' `bBlocksDashCharge`. A piedi ci si passa lo stesso, pagando
	 * il costo maggiore — il movimento normale non passa di qui.
	 *
	 * Il SALTO non usa questo predicato: scavalca le celle intermedie, e il terreno accidentato non ostacola
	 * chi ci passa sopra. Atterrarci resta permesso (`IsWalkable`).
	 */
	bool IsTraversableByStep(const URTHexMapAsset* Map, const FRTCellId& Cell)
	{
		if (!IsWalkable(Map, Cell)) { return false; }
		const FRTHexCellData* Data = Map->FindCell(Cell);
		return !URTTerrainLibrary::FindTerrainDef(Data->Surface).bBlocksDashCharge;
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

		if (!IsTraversableByStep(Map, Next))
		{
			Result.Stop = ERTLinearStop::BlockedByTerrain; // muro, bordo mappa, o terreno che nega lo scatto
			break;
		}

		if (const int32* Occupant = Occupancy.Find(Next))
		{
			// La lama ATTRAVERSA e tira dritto — tranne che sulla cella d'arrivo: si passa in mezzo a
			// qualcuno, non ci si ferma dentro. Due unita' nella stessa cella non sono rappresentabili, e
			// un'eccezione qui la renderebbe possibile per una sola azione.
			if (Style == ERTMovementStyle::LinearPass && K < Distance)
			{
				Result.PassedThroughUnitIds.Add(*Occupant);
				Current = Next;
				Result.Entered.Add(Next);
				continue;
			}

			// La carica si ferma ADIACENTE al nemico e lo colpisce (impatto); per tutti gli altri e' solo un
			// ostacolo. Il `break` qui sotto lascia `Current` dov'e', quindi `Final` e' la cella PRECEDENTE
			// all'occupante: e' la regola, non un dettaglio del ciclo — [D-296].
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

bool URTMovementActionLibrary::IsLinearReachable(const URTHexMapAsset* Map, const FRTCellId& From,
	const FRTCellId& Target, int32 MaxCells, ERTMovementStyle Style,
	const TMap<FRTCellId, int32>& Occupancy, const TSet<int32>& Hostiles)
{
	if (From == Target)
	{
		return true; // "resto dove sono" e' sempre praticabile
	}
	return ResolveLinearMove(Map, From, Target, MaxCells, Style, Occupancy, Hostiles).Final == Target;
}
