#include "Map/RTHexOccupancyLibrary.h"

namespace
{
	/** Prodotto vettoriale 2D di `OA` x `OB`: il segno dice da che parte sta `B` rispetto alla retta `OA`. */
	double Cross2D(const FVector2D& O, const FVector2D& A, const FVector2D& B)
	{
		return (A.X - O.X) * (B.Y - O.Y) - (A.Y - O.Y) * (B.X - O.X);
	}

	int32 SignOf(double Value)
	{
		if (Value > UE_KINDA_SMALL_NUMBER) { return 1; }
		if (Value < -UE_KINDA_SMALL_NUMBER) { return -1; }
		return 0;
	}

	/** `P` collineare con `AB`: ci sta dentro? */
	bool WithinSegmentBounds(const FVector2D& A, const FVector2D& B, const FVector2D& P)
	{
		return P.X <= FMath::Max(A.X, B.X) + UE_KINDA_SMALL_NUMBER
			&& P.X >= FMath::Min(A.X, B.X) - UE_KINDA_SMALL_NUMBER
			&& P.Y <= FMath::Max(A.Y, B.Y) + UE_KINDA_SMALL_NUMBER
			&& P.Y >= FMath::Min(A.Y, B.Y) - UE_KINDA_SMALL_NUMBER;
	}

	bool SegmentsIntersect(const FVector2D& A, const FVector2D& B, const FVector2D& C, const FVector2D& D)
	{
		const int32 D1 = SignOf(Cross2D(A, B, C));
		const int32 D2 = SignOf(Cross2D(A, B, D));
		const int32 D3 = SignOf(Cross2D(C, D, A));
		const int32 D4 = SignOf(Cross2D(C, D, B));

		if (D1 != D2 && D3 != D4)
		{
			return true;
		}
		// Casi collineari: un contatto sul bordo CONTA come occupazione. E' la scelta conservativa, ed e'
		// quella giusta qui: un muro appoggiato esattamente al confine fra due settori li invade entrambi.
		if (D1 == 0 && WithinSegmentBounds(A, B, C)) { return true; }
		if (D2 == 0 && WithinSegmentBounds(A, B, D)) { return true; }
		if (D3 == 0 && WithinSegmentBounds(C, D, A)) { return true; }
		if (D4 == 0 && WithinSegmentBounds(C, D, B)) { return true; }
		return false;
	}

	bool PointInTriangle(const FVector2D& P, const FVector2D& T0, const FVector2D& T1, const FVector2D& T2)
	{
		const int32 S0 = SignOf(Cross2D(T0, T1, P));
		const int32 S1 = SignOf(Cross2D(T1, T2, P));
		const int32 S2 = SignOf(Cross2D(T2, T0, P));
		const bool bAnyNegative = (S0 < 0) || (S1 < 0) || (S2 < 0);
		const bool bAnyPositive = (S0 > 0) || (S1 > 0) || (S2 > 0);
		return !(bAnyNegative && bAnyPositive); // uno zero significa «sul bordo», e il bordo conta
	}

	bool SegmentHitsTriangle(const FVector2D& A, const FVector2D& B,
		const FVector2D& T0, const FVector2D& T1, const FVector2D& T2)
	{
		if (PointInTriangle(A, T0, T1, T2) || PointInTriangle(B, T0, T1, T2))
		{
			return true;
		}
		return SegmentsIntersect(A, B, T0, T1)
			|| SegmentsIntersect(A, B, T1, T2)
			|| SegmentsIntersect(A, B, T2, T0);
	}

	/** Ray casting classico. `Polygon` e' implicitamente chiuso. */
	bool PointInPolygon(const FVector2D& P, const TArray<FVector2D>& Polygon)
	{
		bool bInside = false;
		const int32 Num = Polygon.Num();
		for (int32 I = 0, J = Num - 1; I < Num; J = I++)
		{
			const FVector2D& Pi = Polygon[I];
			const FVector2D& Pj = Polygon[J];
			if (((Pi.Y > P.Y) != (Pj.Y > P.Y))
				&& (P.X < (Pj.X - Pi.X) * (P.Y - Pi.Y) / (Pj.Y - Pi.Y) + Pi.X))
			{
				bInside = !bInside;
			}
		}
		return bInside;
	}
}

void URTHexOccupancyLibrary::SectorBoundaryPoints(float HexSize, TArray<FVector2D>& OutPoints)
{
	OutPoints.Reset();
	OutPoints.Reserve(RT_OccupancySectorCount);

	// Raggio del cerchio INSCRITTO: e' la distanza dal centro al punto medio di un lato.
	const double InRadius = static_cast<double>(HexSize) * FMath::Cos(FMath::DegreesToRadians(30.0));

	for (int32 Sector = 0; Sector < RT_OccupancySectorCount; ++Sector)
	{
		// -30 gradi e' il PRIMO VERTICE, lo stesso da cui `URTHexLibrary` costruisce il perimetro pointy-top.
		const double Radians = FMath::DegreesToRadians(-30.0 + 30.0 * Sector);
		const double Radius = (Sector % 2 == 0) ? static_cast<double>(HexSize) : InRadius;
		OutPoints.Add(FVector2D(Radius * FMath::Cos(Radians), Radius * FMath::Sin(Radians)));
	}
}

FRTOccupancyMask URTHexOccupancyLibrary::ComputeMask(const TArray<FRTOccupancyPolyline>& Geometry, float HexSize)
{
	FRTOccupancyMask Mask;

	TArray<FVector2D> Boundary;
	SectorBoundaryPoints(HexSize, Boundary);
	if (Boundary.Num() != RT_OccupancySectorCount)
	{
		return Mask;
	}

	const FVector2D Centre = FVector2D::ZeroVector;

	for (const FRTOccupancyPolyline& Line : Geometry)
	{
		if (Line.Points.Num() < 2)
		{
			continue; // una polilinea di un punto solo non invade niente
		}

		// Il CENTRO dentro un footprint chiuso: da solo rende la cella `Blocked`, e succede anche quando il
		// footprint e' piu' grande dell'intera cella e il suo bordo non tocca un solo triangolo.
		if (Line.bClosed && PointInPolygon(Centre, Line.Points))
		{
			Mask.bCoreBlocked = true;
			Mask.Sectors = (1 << RT_OccupancySectorCount) - 1;
			continue;
		}

		const int32 NumEdges = Line.bClosed ? Line.Points.Num() : Line.Points.Num() - 1;
		for (int32 Edge = 0; Edge < NumEdges; ++Edge)
		{
			const FVector2D& A = Line.Points[Edge];
			const FVector2D& B = Line.Points[(Edge + 1) % Line.Points.Num()];

			for (int32 Sector = 0; Sector < RT_OccupancySectorCount; ++Sector)
			{
				const int32 Bit = 1 << Sector;
				if ((Mask.Sectors & Bit) != 0)
				{
					continue; // gia' acceso: l'OR e' cio' che rende l'esito indipendente dall'ordine
				}
				const FVector2D& T1 = Boundary[Sector];
				const FVector2D& T2 = Boundary[(Sector + 1) % RT_OccupancySectorCount];
				if (SegmentHitsTriangle(A, B, Centre, T1, T2))
				{
					Mask.Sectors |= Bit;
				}
			}
		}

		// Un settore puo' stare INTERAMENTE dentro un footprint senza che il bordo lo attraversi.
		if (Line.bClosed)
		{
			for (int32 Sector = 0; Sector < RT_OccupancySectorCount; ++Sector)
			{
				if (PointInPolygon(Boundary[Sector], Line.Points))
				{
					Mask.Sectors |= (1 << Sector);
				}
			}
		}
	}

	return Mask;
}

int32 URTHexOccupancyLibrary::Surcharge(ERTCellOccupancy Occupancy, const FRTOccupancyThresholds& Thresholds)
{
	// `Blocked` non paga: chi la rende impassabile e' il bordo, non il costo. Un numero alto qui sarebbe un
	// secondo modo di dire «non si passa», e due modi di dire la stessa cosa prima o poi divergono.
	return (Occupancy == ERTCellOccupancy::Constrained) ? FMath::Max(0, Thresholds.ConstrainedSurcharge) : 0;
}

int32 URTHexOccupancyLibrary::NumOccupiedSectors(const FRTOccupancyMask& Mask)
{
	int32 Count = 0;
	for (int32 Sector = 0; Sector < RT_OccupancySectorCount; ++Sector)
	{
		if ((Mask.Sectors & (1 << Sector)) != 0)
		{
			++Count;
		}
	}
	return Count;
}

ERTCellOccupancy URTHexOccupancyLibrary::Classify(const FRTOccupancyMask& Mask,
	const FRTOccupancyThresholds& Thresholds)
{
	// Il centro vince sul conteggio: un footprint solido piu' grande della cella non occupa nessun settore
	// (i suoi bordi cadono tutti fuori) e senza questa riga sarebbe `Free`.
	if (Mask.bCoreBlocked)
	{
		return ERTCellOccupancy::Blocked;
	}

	const int32 Occupied = NumOccupiedSectors(Mask);
	if (Occupied >= Thresholds.BlockedFrom)
	{
		return ERTCellOccupancy::Blocked;
	}
	if (Occupied >= Thresholds.ConstrainedFrom)
	{
		return ERTCellOccupancy::Constrained;
	}
	return ERTCellOccupancy::Free;
}
