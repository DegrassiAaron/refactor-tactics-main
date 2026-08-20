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

bool URTHexLibrary::DirectionBetween(const FRTCellId& From, const FRTCellId& To, ERTHexDirection& OutDirection)
{
	if (From.Layer != To.Layer)
	{
		return false;
	}

	const int32 Dq = To.X - From.X;
	const int32 Dr = To.Y - From.Y;
	for (int32 I = 0; I < 6; ++I)
	{
		if (RT_HEX_DX[I] == Dq && RT_HEX_DY[I] == Dr)
		{
			OutDirection = static_cast<ERTHexDirection>(I);
			return true;
		}
	}
	return false;
}

bool URTHexLibrary::DirectionTowards(const FRTCellId& From, const FRTCellId& To, ERTHexDirection& OutDirection)
{
	if (HexDistance(From, To) <= 0)
	{
		return false;
	}

	// Il primo passo della linea e' per costruzione adiacente a From (HexLine produce celle consecutive
	// adiacenti), quindi DirectionBetween lo riconosce sempre: una sola definizione di "verso dove".
	const TArray<FRTCellId> Line = HexLine(From, To);
	if (Line.Num() < 2)
	{
		return false;
	}
	return DirectionBetween(From, Line[1], OutDirection);
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

FRTCellId URTHexLibrary::WorldToCellId(const FVector& World, const FVector& Origin, float HexSize, float LayerHeight)
{
	const int32 Layer = WorldToLayer(World.Z, Origin.Z, LayerHeight);
	return WorldToAxial(World, Origin, HexSize, Layer);
}

FRTCellId URTHexLibrary::ResolveRayToCellOnLayer(const FVector& RayOrigin, const FVector& RayDirection,
	const FVector& Origin, float HexSize, float LayerHeight, int32 ActiveLayer,
	bool bHasValidHit, const FVector& HitPoint)
{
	if (bHasValidHit)
	{
		return WorldToAxial(HitPoint, Origin, HexSize, ActiveLayer);
	}

	// Nessun colpo utile: si punta il PIANO del layer attivo. La quota e' quella del layer, non quella del
	// raggio, perche' la domanda e' «quale cella di questo piano sto indicando» e non «cosa ho colpito».
	const double PlaneZ = Origin.Z + static_cast<double>(ActiveLayer) * static_cast<double>(LayerHeight);
	const FPlane LayerPlane(FVector(0, 0, PlaneZ), FVector(0, 0, 1));
	const FVector Point = FMath::RayPlaneIntersection(RayOrigin, RayDirection, LayerPlane);
	return WorldToAxial(Point, Origin, HexSize, ActiveLayer);
}

FVector URTHexLibrary::CellsCentroidWorld(const TArray<FRTCellId>& Cells, const FVector& Origin, float HexSize,
	float LayerHeight)
{
	if (Cells.Num() == 0)
	{
		return Origin; // nessuna cella: si ripiega sull'origine, non su un punto arbitrario
	}

	FVector Sum = FVector::ZeroVector;
	for (const FRTCellId& Cell : Cells)
	{
		Sum += AxialToWorld(Cell, Origin, HexSize, LayerHeight);
	}
	// Somma di centri: commutativa, quindi l'ordine dell'input non cambia il risultato.
	return Sum / static_cast<double>(Cells.Num());
}

FBox URTHexLibrary::CellsBoundsWorld(const TArray<FRTCellId>& Cells, const FVector& Origin, float HexSize,
	float LayerHeight)
{
	FBox Box(ForceInit); // non valido finche' non entra una cella: una mappa vuota non ha un'inquadratura

	// Semi-estensione dell'esagono, non del suo centro. I sei vertici di un pointy-top di circumraggio
	// `HexSize` stanno a -30 + 60k gradi (`HexCorners`), quindi |cos| massimo e' sqrt(3)/2 e |sin| e' 1:
	// la cella e' piu' STRETTA in X che in Y, e le due semi-estensioni non sono intercambiabili.
	const double HalfX = static_cast<double>(HexSize) * RT_SQRT3 * 0.5;
	const double HalfY = static_cast<double>(HexSize);

	for (const FRTCellId& Cell : Cells)
	{
		const FVector Centre = AxialToWorld(Cell, Origin, HexSize, LayerHeight);
		// `+=` su un FBox e' min/max componente per componente: piega commutativa, quindi l'ordine
		// dell'input non cambia il risultato.
		Box += FVector(Centre.X - HalfX, Centre.Y - HalfY, Centre.Z);
		Box += FVector(Centre.X + HalfX, Centre.Y + HalfY, Centre.Z);
	}
	return Box;
}

FBox URTHexLibrary::CellsBoundsWorld(const TArray<FRTHexCellData>& Cells, const FVector& Origin, float HexSize,
	float LayerHeight)
{
	FBox Box(ForceInit);

	const double HalfX = static_cast<double>(HexSize) * RT_SQRT3 * 0.5;
	const double HalfY = static_cast<double>(HexSize);

	for (const FRTHexCellData& Cell : Cells)
	{
		const FVector Centre = AxialToWorld(Cell.Id, Origin, HexSize, LayerHeight);
		// La quota d'autore alza la cella nel RENDER (`ARTHexMapActor::RebuildInstances`): il box deve
		// contenere dove la cella si vede, non dove il suo layer sarebbe.
		const double Z = Centre.Z + static_cast<double>(Cell.Height);
		Box += FVector(Centre.X - HalfX, Centre.Y - HalfY, Z);
		Box += FVector(Centre.X + HalfX, Centre.Y + HalfY, Z);
	}
	return Box;
}

FColor URTHexLibrary::SurfaceColor(ERTHexSurface Surface)
{
	// Tinte scelte per essere distinguibili fra loro e dal rosso del blocco (test:
	// Hex.SurfaceColorsAreDistinguishable). Cambiandone una, quel test dice subito se si e' persa la leggibilita'.
	switch (Surface)
	{
	case ERTHexSurface::ShallowWater: return FColor(60, 120, 255);
	case ERTHexSurface::Rough:        return FColor(140, 100, 60);
	case ERTHexSurface::Fire:         return FColor(255, 130, 40);
	case ERTHexSurface::Conductive:   return FColor(80, 230, 230);
	case ERTHexSurface::Ice:          return FColor(160, 220, 255);
	case ERTHexSurface::Void:         return FColor(150, 40, 150);
	// Grigio-bluastro slavato, distinto dal grigio neutro del Floor: il Fumo ha una regola VERA (cap di
	// targeting a 2 celle) e chi dipinge la mappa deve vedere dove l'ha messo.
	case ERTHexSurface::Smoke:        return FColor(180, 190, 215);
	case ERTHexSurface::HighGround:   return FColor(230, 200, 80);
	case ERTHexSurface::Floor:
	default:                          return FColor(160, 160, 160);
	}
}

FVector URTHexLibrary::EdgeMidpointWorld(const FRTCellId& Cell, ERTHexDirection Dir, const FVector& Origin,
	float HexSize, float LayerHeight)
{
	// Punto medio fra i due centri: derivato dalla convenzione delle direzioni, non da un angolo inciso.
	const FVector Here = AxialToWorld(Cell, Origin, HexSize, LayerHeight);
	const FVector There = AxialToWorld(Neighbor(Cell, Dir), Origin, HexSize, LayerHeight);
	return (Here + There) * 0.5;
}

FRotator URTHexLibrary::EdgeRotation(const FRTCellId& Cell, ERTHexDirection Dir)
{
	// Lo yaw si ricava dallo spostamento fra i due centri in coordinate-mondo. La scala non conta (e' una
	// direzione), quindi si usano origine e dimensioni neutre: cio' che importa e' l'ANGOLO.
	const FVector Here = AxialToWorld(Cell, FVector::ZeroVector, 100.f, 0.f);
	const FVector There = AxialToWorld(Neighbor(Cell, Dir), FVector::ZeroVector, 100.f, 0.f);
	return (There - Here).Rotation();
}

ERTHexDirection URTHexLibrary::OppositeDirection(ERTHexDirection Dir)
{
	// Le sei direzioni sono in ordine stabile 0..5 e diametralmente opposte a tre di distanza: E(0)<->W(3),
	// NE(1)<->SW(4), NW(2)<->SE(5). Derivato dall'ordine, non da una tabella da tenere allineata.
	return static_cast<ERTHexDirection>((static_cast<int32>(Dir) + 3) % 6);
}

float URTHexLibrary::ReliefHeightForCost(int32 MoveCost)
{
	// Il pavimento (costo 1) sta a zero: il rilievo misura il sovrapprezzo, non il costo assoluto. Costi
	// minori di 1 non esistono a catalogo, ma un asset editato a mano puo' contenerli: si trattano come piani
	// invece di produrre una buca, che direbbe «qui si va piu' veloci» — cosa che il gioco non prevede.
	return FMath::Max(0, MoveCost - 1) * ReliefUnitHeight;
}

FColor URTHexLibrary::BlockedCellColor()
{
	return FColor(230, 40, 40); // rosso: non ci si passa
}

FColor URTHexLibrary::SightBlockerColor()
{
	return FColor(250, 240, 90); // giallo: non ci si vede attraverso, ma ci si passa
}

FColor URTHexLibrary::SpawnTeam0Color()
{
	return FColor(60, 170, 255); // azzurro: di qui parte la squadra 0
}

FColor URTHexLibrary::SpawnTeam1Color()
{
	return FColor(255, 130, 40); // arancio: di qui parte la squadra 1
}

FColor URTHexLibrary::UnreachableCellColor()
{
	return FColor(255, 0, 255); // magenta: il colore che nella mappa non usa nessuno, perche' e' un allarme
}

TArray<FVector> URTHexLibrary::HexCorners(const FVector& Center, float Radius)
{
	TArray<FVector> Corners;
	Corners.Reserve(6);
	for (int32 I = 0; I < 6; ++I)
	{
		const double Angle = PI / 180.0 * (60.0 * I - 30.0); // pointy-top: primo vertice a -30 gradi
		Corners.Add(Center + FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), 0.0));
	}
	return Corners;
}

ERTHexDirection URTHexLibrary::DirectionForEdgeIndex(int32 EdgeIndex)
{
	// Rispecchiamento attorno all'asse X: il bordo geometrico `k` ha il punto medio a `60k` gradi, mentre
	// `AxialDirection(j)` punta a `-60j`. `E` (0) e `W` (3) restano fermi perche' sono i due punti fissi.
	// Il modulo positivo regge anche un indice fuori intervallo, che altrimenti diventerebbe un cast a un
	// enum inesistente.
	const int32 Wrapped = ((EdgeIndex % 6) + 6) % 6;
	return static_cast<ERTHexDirection>((6 - Wrapped) % 6);
}

int32 URTHexLibrary::EdgeIndexForDirection(ERTHexDirection Dir)
{
	// Stessa operazione: un rispecchiamento e' l'inverso di se' stesso. Scritta come chiamata invece che
	// ricopiata, cosi' se la convenzione cambia c'e' un posto solo da cambiare.
	return static_cast<int32>(DirectionForEdgeIndex(static_cast<int32>(Dir)));
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

namespace
{
	// Scala del "nudge" intero che rompe i pareggi dell'arrotondamento: il punto interpolato viene spostato di
	// 1/1024 di cella in una direzione FISSA (bias 1,1,-2 -> somma nulla, l'invariante cubica regge). Sostituisce
	// l'epsilon float della formulazione classica: una linea che passa esattamente sul confine fra due celle
	// sceglie sempre lo stesso lato, senza oscillare. Calcoli in int64: nessun overflow con mappe grandi.
	constexpr int64 RT_HEX_LINE_SCALE = 1024;

	/** Divisione intera con arrotondamento al piu' vicino (pareggi lontano dallo zero). Den > 0. */
	int64 RoundDivInt(int64 Num, int64 Den)
	{
		return (Num >= 0) ? (2 * Num + Den) / (2 * Den) : -((-2 * Num + Den) / (2 * Den));
	}
}

TArray<FRTCellId> URTHexLibrary::HexLine(const FRTCellId& A, const FRTCellId& B)
{
	TArray<FRTCellId> Out;

	const int32 N = HexDistance(A, B);
	Out.Reserve(N + 1);
	if (N == 0)
	{
		Out.Add(FRTCellId(A.X, A.Y, A.Layer));
		return Out;
	}

	// Estremi in coordinate cubiche (q, r, s = -q-r).
	const int64 Aq = A.X, Ar = A.Y, As = A.CubeZ();
	const int64 Bq = B.X, Br = B.Y, Bs = B.CubeZ();
	const int64 Den = RT_HEX_LINE_SCALE * N;

	for (int32 i = 0; i <= N; ++i)
	{
		// Punto i/N sulla retta come frazione INTERA (numeratori scalati + bias di rottura dei pareggi).
		const int64 Nq = RT_HEX_LINE_SCALE * (Aq * (N - i) + Bq * i) + 1;
		const int64 Nr = RT_HEX_LINE_SCALE * (Ar * (N - i) + Br * i) + 1;
		const int64 Ns = RT_HEX_LINE_SCALE * (As * (N - i) + Bs * i) - 2;

		int64 Rq = RoundDivInt(Nq, Den);
		int64 Rr = RoundDivInt(Nr, Den);
		int64 Rs = RoundDivInt(Ns, Den);

		// Resti interi (scala Den): la componente col resto maggiore si ricava dalle altre due, cosi' q+r+s = 0
		// resta vero. Ordine di preferenza fisso q -> r -> s, come CubeRound.
		const int64 Dq = FMath::Abs(Rq * Den - Nq);
		const int64 Dr = FMath::Abs(Rr * Den - Nr);
		const int64 Ds = FMath::Abs(Rs * Den - Ns);

		if (Dq > Dr && Dq > Ds)
		{
			Rq = -Rr - Rs;
		}
		else if (Dr > Ds)
		{
			Rr = -Rq - Rs;
		}
		// altrimenti si aggiusta s, che non serve all'output (q e r bastano)

		Out.Add(FRTCellId(static_cast<int32>(Rq), static_cast<int32>(Rr), A.Layer));
	}
	return Out;
}

TArray<FRTCellId> URTHexLibrary::HexCone(const FRTCellId& From, const FRTCellId& Target, int32 Range)
{
	TArray<FRTCellId> Out;
	if (Range <= 0 || (From.X == Target.X && From.Y == Target.Y))
	{
		return Out;
	}

	// Direzione principale = primo passo della linea verso il bersaglio (sempre uno dei sei vicini).
	const TArray<FRTCellId> Line = HexLine(From, Target);
	if (Line.Num() < 2)
	{
		return Out;
	}
	const int32 StepX = Line[1].X - From.X;
	const int32 StepY = Line[1].Y - From.Y;

	int32 DirIdx = INDEX_NONE;
	for (int32 I = 0; I < 6; ++I)
	{
		if (RT_HEX_DX[I] == StepX && RT_HEX_DY[I] == StepY)
		{
			DirIdx = I;
			break;
		}
	}
	if (DirIdx == INDEX_NONE)
	{
		return Out;
	}

	// Ventaglio di 120 gradi = due settori a 60 gradi adiacenti. Nel settore fra due direzioni contigue ogni
	// cella e' a*D1 + b*D2 con a,b >= 0 e distanza esattamente a+b: il taglio a+b <= Range da' la profondita'.
	TSet<FRTCellId> Unique;
	auto AddSector = [&Unique, &From, Range](int32 D1, int32 D2)
	{
		for (int32 A = 0; A <= Range; ++A)
		{
			for (int32 B = 0; A + B <= Range; ++B)
			{
				if (A + B < 1)
				{
					continue; // l'origine non fa parte del cono
				}
				Unique.Add(FRTCellId(
					From.X + A * RT_HEX_DX[D1] + B * RT_HEX_DX[D2],
					From.Y + A * RT_HEX_DY[D1] + B * RT_HEX_DY[D2],
					From.Layer));
			}
		}
	};
	AddSector((DirIdx + 5) % 6, DirIdx); // settore sinistro
	AddSector(DirIdx, (DirIdx + 1) % 6); // settore destro

	// TSet non ha ordine garantito: si ordina in modo stabile (determinismo).
	Out = Unique.Array();
	Out.Sort([](const FRTCellId& L, const FRTCellId& R) { return StableLess(L, R); });
	return Out;
}
