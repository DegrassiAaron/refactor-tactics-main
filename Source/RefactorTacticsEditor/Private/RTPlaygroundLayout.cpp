#include "RTPlaygroundLayout.h"

namespace RTPlayground
{
	namespace
	{
		/** Un pad e' sempre `8 x 8 m`: la costante esiste perche' il test possa chiederne il valore. */
		constexpr double PadSideMetres = 8.0;

		/** Le quattro colonne, per il loro bordo sinistro. Fra una e la successiva restano 2 m. */
		constexpr double ColumnLeft[4] = { -19.0, -9.0, 1.0, 11.0 };

		/** Le due righe, per il loro bordo inferiore. Fra loro passano corridoio e service strip. */
		constexpr double RowBottomNorth = 3.0;
		constexpr double RowBottomSouth = -11.0;

		FBox2D Pad(double Left, double Bottom)
		{
			return FBox2D(FVector2D(Left, Bottom),
			              FVector2D(Left + PadSideMetres, Bottom + PadSideMetres));
		}
	}

	FBox2D FloorBounds()
	{
		return FBox2D(FVector2D(-20.0, -12.0), FVector2D(20.0, 12.0));
	}

	FBox2D CorridorBounds()
	{
		// Per tutta la lunghezza del floor: il corridoio e' il modo in cui si passa da una colonna
		// all'altra senza attraversare una stazione, ed e' anche cio' che la camera Overview segue.
		return FBox2D(FVector2D(-20.0, -2.0), FVector2D(20.0, 2.0));
	}

	TArray<FBox2D> ServiceStrips()
	{
		return {
			FBox2D(FVector2D(-20.0,  2.0), FVector2D(20.0,  3.0)),
			FBox2D(FVector2D(-20.0, -3.0), FVector2D(20.0, -2.0)),
		};
	}

	TArray<FStation> Stations()
	{
		// L'ordine e' quello del numero, e la disposizione alterna nord/sud per colonna: la 01 sta a
		// nord-ovest e la 08 a sud-est. E' la planimetria dell'Epic #1990, non una derivazione.
		return {
			{ 1, TEXT("Unit + Facing"),                       Pad(ColumnLeft[0], RowBottomNorth), /*bLive=*/ true  },
			{ 2, TEXT("GrayKit primitives + scale"),           Pad(ColumnLeft[0], RowBottomSouth), /*bLive=*/ false },
			{ 3, TEXT("Movement / Path / Destination / Dash"), Pad(ColumnLeft[1], RowBottomNorth), /*bLive=*/ false },
			{ 4, TEXT("Target / Range / Line / AoE / Cone"),   Pad(ColumnLeft[1], RowBottomSouth), /*bLive=*/ false },
			{ 5, TEXT("Defense / Shield / Cover"),             Pad(ColumnLeft[2], RowBottomNorth), /*bLive=*/ false },
			{ 6, TEXT("LOS / Visibility / Knowledge"),         Pad(ColumnLeft[2], RowBottomSouth), /*bLive=*/ false },
			{ 7, TEXT("Water / Ice / Fire / Electricity"),     Pad(ColumnLeft[3], RowBottomNorth), /*bLive=*/ false },
			{ 8, TEXT("Scenario / Resolution / Playback"),     Pad(ColumnLeft[3], RowBottomSouth), /*bLive=*/ false },
		};
	}

	const FStation* FindStation(int32 Number)
	{
		// Statico perche' il chiamante riceve un puntatore: una TArray locale morirebbe al return, e il
		// puntatore dentro di essa con lei. Le otto voci sono costanti e non cambiano a runtime.
		static const TArray<FStation> All = Stations();
		return All.FindByPredicate([Number](const FStation& S) { return S.Number == Number; });
	}

	double WorldFromMetres(double Metres)
	{
		// 1 unita' Unreal = 1 cm. Non e' una scelta di questo file: e' la convenzione del motore, ed e'
		// la stessa con cui `HexSize = 150` significa una cella di 1,5 m.
		return Metres * 100.0;
	}
}
