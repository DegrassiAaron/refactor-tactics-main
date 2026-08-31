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

	/**
	 * IL SEGMENTO ATTRAVERSA IL TRIANGOLO CON LUNGHEZZA NON NULLA? — `MSE-4`, `#1826`.
	 *
	 * 🔴 **Sostituisce un test di CONTATTO con uno di SOVRAPPOSIZIONE, ed e' l'intera issue.** La versione
	 * precedente accendeva il settore anche per un contatto in un **solo punto**: era una scelta dichiarata
	 * e conservativa, giusta per il contatto lungo un **segmento** — un muro appoggiato al confine fra due
	 * settori li invade entrambi — ma il caso puntuale non era mai stato considerato separatamente.
	 *
	 * ⚠️ **E il caso puntuale non era raro: era ogni muro centrale.** Il centro della cella e' il vertice
	 * comune di TUTTI E DODICI i triangoli, e `ToPolyline` calcola `Base = Perp * Offset`, quindi ogni
	 * segmento con `Offset == 0` passa per il centro **per definizione**. Un diametro attraversa quattro
	 * settori e ne accendeva dodici, lasciando `ComputeFreeRegions` con zero regioni: la cella diventava
	 * inagibile. E' la regola *«geometria che tocca il centro ⇒ cella bloccata»* che il Decision Record
	 * dichiara **superata**, sopravvissuta non come regola scritta ma come effetto collaterale di questa
	 * funzione.
	 *
	 * **Come**: si taglia il segmento contro i tre semipiani del triangolo — e' convesso, quindi bastano —
	 * e si guarda se ne resta un TRATTO invece di un punto.
	 *
	 * 🔑 **Il parametro e' ADIMENSIONALE, ed e' cio' che rispetta il vincolo di determinismo di `#1826`.**
	 * Il confronto finale e' fra due numeri in `[0, 1]`, non fra lunghezze: non dipende da `HexSize`, e la
	 * stessa geometria da' lo stesso bit a qualunque scala. Una soglia sulla lunghezza reale avrebbe reso
	 * la maschera funzione della scala, e la maschera entra in `ComputeHash` per la via del costo di cella.
	 *
	 * **La soglia e' misurata, non scelta a occhio.** Sul diametro, i quattro settori attraversati danno un
	 * tratto di `5.0e-01`; i contatti puntuali danno `0.0`, con un solo residuo a `5.6e-17` di rumore di
	 * macchina. Sedici ordini di grandezza separano i due casi, e `1e-9` sta in mezzo con otto ordini di
	 * margine da ciascun lato.
	 *
	 * ⚠️ **Il contatto lungo un segmento resta occupato**, ed e' il Non-goal esplicito di `#1826`: quando il
	 * segmento GIACE sul lato di un triangolo, per quel lato il taglio non toglie nulla — la funzione e'
	 * costante e nulla — quindi il tratto sopravvive intero.
	 * `SegmentOnSectorBoundaryOccupiesBothAdjacentSectors` lo pinna e resta verde.
	 */
	bool SegmentOverlapsTriangle(const FVector2D& A, const FVector2D& B,
		const FVector2D& T0, const FVector2D& T1, const FVector2D& T2)
	{
		// La soglia e' RELATIVA, e questa e' la riga che lo rende vero: `Num` e `Den` sono entrambi prodotti
		// vettoriali fra il lato del triangolo e il segmento, quindi hanno la dimensione di un'AREA. Un
		// epsilon assoluto su di loro dipenderebbe da `HexSize` — che e' il vincolo di determinismo che
		// `#1826` vieta, perche' la maschera entra in `ComputeHash` per la via del costo di cella.
		//
		// 🔴 **Costato un giro di test rossi**: la prima stesura confrontava `Num < 0.0` senza tolleranza, e
		// su un segmento che GIACE sul lato di un triangolo `Num` non e' zero esatto ma rumore — con il segno
		// che capita. Il diametro perdeva il settore `6` e ne accendeva tre invece di quattro: la maschera
		// era `0x83` invece di `0xC3`, e l'asimmetria fra il settore `6` e il `7` veniva solo dal verso in cui
		// i due triangoli enumerano i propri lati.
		constexpr double OverlapEpsilon = 1e-9;

		const FVector2D Tri[3] = { T0, T1, T2 };
		const double DirX = static_cast<double>(B.X) - static_cast<double>(A.X);
		const double DirY = static_cast<double>(B.Y) - static_cast<double>(A.Y);
		const double DirLen = FMath::Sqrt(DirX * DirX + DirY * DirY);
		if (DirLen <= 0.0)
		{
			return false; // un segmento senza lunghezza non attraversa niente
		}

		double Enter = 0.0;
		double Exit = 1.0;

		// I tre lati orientati in senso ANTIORARIO: `SectorBoundaryPoints` enumera il perimetro per angolo
		// crescente, quindi `(Centro, T1, T2)` e' gia' antiorario e l'interno sta a sinistra di ogni lato.
		// Il verso non si ricava qui con un prodotto vettoriale ad hoc: discende da chi costruisce i punti.
		for (int32 Side = 0; Side < 3; ++Side)
		{
			const FVector2D& Pa = Tri[Side];
			const FVector2D& Pb = Tri[(Side + 1) % 3];
			const double EdgeX = static_cast<double>(Pb.X) - static_cast<double>(Pa.X);
			const double EdgeY = static_cast<double>(Pb.Y) - static_cast<double>(Pa.Y);

			// La scala comune di `Num` e `Den`: il prodotto delle due lunghezze. Dividere per essa rende
			// ogni confronto un SENO — adimensionale, e identico a qualunque `HexSize`.
			const double Scale = FMath::Sqrt(EdgeX * EdgeX + EdgeY * EdgeY) * DirLen;
			if (Scale <= 0.0)
			{
				continue; // lato degenere: non taglia niente
			}

			// `Num` e' da che parte sta `A`: positivo dentro. `Den` dice se il segmento entra o esce.
			const double Num = EdgeX * (static_cast<double>(A.Y) - Pa.Y) - EdgeY * (static_cast<double>(A.X) - Pa.X);
			const double Den = EdgeY * DirX - EdgeX * DirY;

			if (FMath::Abs(Den) <= OverlapEpsilon * Scale)
			{
				// Parallelo a questo lato: o e' interamente dentro il semipiano, o interamente fuori.
				// ⚠️ La tolleranza serve proprio qui: un segmento che giace ESATTAMENTE sul lato deve
				// sopravvivere, perche' e' il contatto lungo un segmento — occupato per Non-goal dichiarato
				// di `#1826` — e il suo `Num` e' zero a meno del rumore.
				if (Num < -OverlapEpsilon * Scale)
				{
					return false;
				}
				continue;
			}

			const double T = Num / Den;
			if (Den > 0.0)
			{
				Exit = FMath::Min(Exit, T);
			}
			else
			{
				Enter = FMath::Max(Enter, T);
			}

		}

		// 🔑 **QUESTA E' LA RIGA CHE CHIUDE `MSE-4`, ed e' l'UNICA.** Con un confronto non stretto, un
		// contatto puntuale — `Exit == Enter` — tornerebbe a contare, e il muro centrale riaccenderebbe
		// dodici settori.
		//
		// 🔴 **Un'uscita anticipata dentro il ciclo diceva la stessa cosa, ed e' stata tolta.** Non era una
		// difesa in piu': era la stessa regola scritta due volte, e la verifica di mutazione l'ha
		// dimostrato — allentandone una, l'altra teneva, e NESSUN test cadeva. Una regola che nessuna
		// mutazione puo' far cadere non e' protetta: e' solo difficile da rompere per caso. Ora il punto di
		// decisione e' uno, e allentarlo fa cadere esattamente i due test che lo proteggono.
		return (Exit - Enter) > OverlapEpsilon;
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
				if (SegmentOverlapsTriangle(A, B, Centre, T1, T2))
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
