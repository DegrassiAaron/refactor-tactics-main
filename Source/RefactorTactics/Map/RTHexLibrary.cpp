#include "Map/RTHexLibrary.h"

namespace
{
	// sqrt(3) in doppia precisione (matematica pointy-top). Usata solo nelle conversioni verso/da il mondo.
	constexpr double RT_SQRT3 = 1.7320508075688772;

	// Vettori assiali (dq,dr) delle sei direzioni pointy-top, ordine stabile E,NE,NW,W,SW,SE.
	constexpr int32 RT_HEX_DX[6] = { +1, +1,  0, -1, -1,  0 };
	constexpr int32 RT_HEX_DY[6] = {  0, -1, -1,  0, +1, +1 };

	// Arrotondamento cubico: dal cubo frazionario (q,r,s) alla cella intera piu' vicina, mantenendo q+r+s=0.
	FRTCellId CubeRound(double Fq, double Fr, int32 Layer)
	{
		const double Fs = -Fq - Fr;
		int32 Rq = FMath::RoundToInt(Fq);
		int32 Rr = FMath::RoundToInt(Fr);
		int32 Rs = FMath::RoundToInt(Fs);

		const double Dq = FMath::Abs(Rq - Fq);
		const double Dr = FMath::Abs(Rr - Fr);
		const double Ds = FMath::Abs(Rs - Fs);

		if (Dq > Dr && Dq > Ds)
		{
			Rq = -Rr - Rs;
		}
		else if (Dr > Ds)
		{
			Rr = -Rq - Rs;
		}
		// altrimenti Rs si aggiusta implicitamente (non serve, X e Y bastano)
		return FRTCellId(Rq, Rr, Layer);
	}
}

FIntPoint URTHexLibrary::AxialDirection(ERTHexDirection Dir)
{
	const int32 I = static_cast<int32>(Dir);
	return FIntPoint(RT_HEX_DX[I], RT_HEX_DY[I]);
}

FRTCellId URTHexLibrary::Neighbor(const FRTCellId& Cell, ERTHexDirection Dir)
{
	const FIntPoint D = AxialDirection(Dir);
	return FRTCellId(Cell.X + D.X, Cell.Y + D.Y, Cell.Layer);
}

TArray<FRTCellId> URTHexLibrary::Neighbors(const FRTCellId& Cell)
{
	TArray<FRTCellId> Out;
	Out.Reserve(6);
	for (int32 I = 0; I < 6; ++I)
	{
		Out.Add(FRTCellId(Cell.X + RT_HEX_DX[I], Cell.Y + RT_HEX_DY[I], Cell.Layer));
	}
	return Out;
}

int32 URTHexLibrary::HexDistance(const FRTCellId& A, const FRTCellId& B)
{
	// Distanza cubica: (|dq| + |dr| + |ds|) / 2.
	const int32 Dq = FMath::Abs(A.X - B.X);
	const int32 Dr = FMath::Abs(A.Y - B.Y);
	const int32 Ds = FMath::Abs(A.CubeZ() - B.CubeZ());
	return (Dq + Dr + Ds) / 2;
}

FVector URTHexLibrary::AxialToWorld(const FRTCellId& Cell, const FVector& Origin, float HexSize, float LayerHeight)
{
	const double Size = static_cast<double>(HexSize);
	const double Q = static_cast<double>(Cell.X);
	const double R = static_cast<double>(Cell.Y);
	const double Wx = Size * RT_SQRT3 * (Q + R / 2.0);
	const double Wy = Size * 1.5 * R;
	return FVector(
		Origin.X + Wx,
		Origin.Y + Wy,
		Origin.Z + static_cast<double>(Cell.Layer) * static_cast<double>(LayerHeight));
}

FRTCellId URTHexLibrary::WorldToAxial(const FVector& World, const FVector& Origin, float HexSize, int32 Layer)
{
	const double Size = static_cast<double>(HexSize);
	const double Lx = World.X - Origin.X;
	const double Ly = World.Y - Origin.Y;
	const double Fq = (RT_SQRT3 / 3.0 * Lx - 1.0 / 3.0 * Ly) / Size;
	const double Fr = (2.0 / 3.0 * Ly) / Size;
	return CubeRound(Fq, Fr, Layer);
}

int32 URTHexLibrary::WorldToLayer(double WorldZ, double OriginZ, float LayerHeight)
{
	if (LayerHeight <= 0.f)
	{
		return 0;
	}
	return FMath::RoundToInt((WorldZ - OriginZ) / static_cast<double>(LayerHeight));
}

float URTHexLibrary::DistanceRayToSegment(const FVector& RayOrigin, const FVector& RayDir, const FVector& A, const FVector& B)
{
	// Closest points tra semi-retta (s>=0, dir unitaria) e segmento (t in [0,1]). Adattato da Ericson, con doppio clamp.
	const FVector D1 = RayDir.GetSafeNormal(); // direzione ray (unitaria) -> s = distanza lungo il ray
	const FVector D2 = B - A;                   // direzione segmento -> t in [0,1]
	const double E = FVector::DotProduct(D2, D2);
	const FVector R = RayOrigin - A;
	const double B1 = FVector::DotProduct(D1, D2);
	const double C = FVector::DotProduct(D1, R);
	const double F = FVector::DotProduct(D2, R);

	double S = 0.0; // lungo il ray (>=0)
	double T = 0.0; // lungo il segmento ([0,1])
	if (E <= (double)SMALL_NUMBER)
	{
		// Segmento degenere (A==B): T=0, S = proiezione di A sul ray (clamp>=0).
		T = 0.0;
		S = FMath::Max(0.0, -C);
	}
	else
	{
		const double Denom = E - B1 * B1; // = |D2|^2 - dot(D1,D2)^2 (>=0)
		T = (Denom > (double)SMALL_NUMBER) ? ((F - C * B1) / Denom) : (F / E);
		T = FMath::Clamp(T, 0.0, 1.0);
		S = FMath::Max(0.0, T * B1 - C);
		T = FMath::Clamp((F + S * B1) / E, 0.0, 1.0);
	}

	const FVector PRay = RayOrigin + static_cast<float>(S) * D1;
	const FVector QSeg = A + static_cast<float>(T) * D2;
	return static_cast<float>(FVector::Dist(PRay, QSeg)); // FVector::Dist ritorna double in UE5: cast esplicito (no C4244)
}

bool URTHexLibrary::StableLess(const FRTCellId& A, const FRTCellId& B)
{
	if (A.Layer != B.Layer) { return A.Layer < B.Layer; }
	if (A.X != B.X) { return A.X < B.X; }
	return A.Y < B.Y;
}

TArray<FRTCellId> URTHexLibrary::HexArea(const FRTCellId& Center, int32 Radius)
{
	TArray<FRTCellId> Out;
	if (Radius < 0)
	{
		return Out;
	}
	Out.Reserve(3 * Radius * (Radius + 1) + 1);
	for (int32 Dq = -Radius; Dq <= Radius; ++Dq)
	{
		const int32 RLo = FMath::Max(-Radius, -Dq - Radius);
		const int32 RHi = FMath::Min(Radius, -Dq + Radius);
		for (int32 Dr = RLo; Dr <= RHi; ++Dr)
		{
			Out.Add(FRTCellId(Center.X + Dq, Center.Y + Dr, Center.Layer));
		}
	}
	return Out;
}
