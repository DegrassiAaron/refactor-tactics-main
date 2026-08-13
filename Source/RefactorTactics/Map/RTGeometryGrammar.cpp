#include "Map/RTGeometryGrammar.h"

int32 URTGeometryGrammarLibrary::AxisBoundaryIndex(ERTTacticalAxis Axis)
{
	// L'asse `k` e' il punto di confine `k + 1`: il confine `0` sta a -30 gradi (il primo vertice), quindi
	// il confine `1` sta a 0 gradi, che e' il primo asse. La corrispondenza vive SOLO qui.
	return static_cast<int32>(Axis) + 1;
}

FVector2D URTGeometryGrammarLibrary::AxisPoint(ERTTacticalAxis Axis, float HexSize)
{
	if (!IsKnownAxis(Axis))
	{
		return FVector2D::ZeroVector;
	}

	TArray<FVector2D> Boundary;
	URTHexOccupancyLibrary::SectorBoundaryPoints(HexSize, Boundary);
	return Boundary[AxisBoundaryIndex(Axis) % RT_OccupancySectorCount];
}

FVector2D URTGeometryGrammarLibrary::AxisPerpendicularPoint(ERTTacticalAxis Axis, float HexSize)
{
	if (!IsKnownAxis(Axis))
	{
		return FVector2D::ZeroVector;
	}

	TArray<FVector2D> Boundary;
	URTHexOccupancyLibrary::SectorBoundaryPoints(HexSize, Boundary);

	// Nove passi da 30 gradi sono -90: la perpendicolare all'asse, nel verso che rende positivo l'offset
	// del muro sul lato `E`.
	const int32 Index = (AxisBoundaryIndex(Axis) + 9) % RT_OccupancySectorCount;
	return Boundary[Index];
}

bool URTGeometryGrammarLibrary::IsKnownAxis(ERTTacticalAxis Axis)
{
	return static_cast<int32>(Axis) >= 0 && static_cast<int32>(Axis) < RT_TacticalAxisCount;
}

ERTGeometryViolation URTGeometryGrammarLibrary::ValidateSegment(const FRTGeometrySegment& Segment)
{
	// L'asse per primo: senza una giacitura valida nessuna delle altre regole ha un significato geometrico.
	if (!IsKnownAxis(Segment.Axis))
	{
		return ERTGeometryViolation::OffAxis;
	}

	if (Segment.AlongStart == Segment.AlongEnd)
	{
		return ERTGeometryViolation::ZeroLength;
	}

	if (Segment.Layer < 0)
	{
		return ERTGeometryViolation::InvalidLayer;
	}

	// ⚠️ Il confronto e' su ENTRAMBI i versi invece che sul valore assoluto, ed e' deliberato:
	// `FMath::Abs(MIN_int32)` non e' rappresentabile in `int32` e resta NEGATIVO, quindi un `Extent`
	// calcolato con `Abs` farebbe passare proprio il valore piu' estremo — cioe' il caso che questa regola
	// esiste per fermare, dato che l'unico modo in cui un intero simile entra e' una deserializzazione.
	auto OutOfRange = [](int32 Value)
	{
		return Value > RT_GeometryMaxQuanta || Value < -RT_GeometryMaxQuanta;
	};

	if (OutOfRange(Segment.Offset) || OutOfRange(Segment.AlongStart) || OutOfRange(Segment.AlongEnd))
	{
		return ERTGeometryViolation::OutsideEditableBounds;
	}

	return ERTGeometryViolation::None;
}

void URTGeometryGrammarLibrary::Validate(const TArray<FRTGeometrySegment>& Segments, TArray<FRTGeometryIssue>& OutIssues)
{
	OutIssues.Reset();

	for (int32 Index = 0; Index < Segments.Num(); ++Index)
	{
		const FRTGeometrySegment& Segment = Segments[Index];

		const ERTGeometryViolation Violation = ValidateSegment(Segment);
		if (Violation != ERTGeometryViolation::None)
		{
			OutIssues.Add({ Index, Violation });
			continue;
		}

		// Il duplicato e' una proprieta' della COLLEZIONE, non del segmento: si segnala sulla seconda
		// occorrenza, cosi' che rimuovendola la collezione diventi valida. Confronto lineare all'indietro:
		// nessun ordinamento e nessun hash, quindi l'esito non dipende dall'ordine di iterazione di una
		// `TMap`/`TSet` — l'invariante di determinismo del repository.
		for (int32 Previous = 0; Previous < Index; ++Previous)
		{
			if (Segments[Previous] == Segment)
			{
				OutIssues.Add({ Index, ERTGeometryViolation::DuplicateSegment });
				break;
			}
		}
	}
}

FRTOccupancyPolyline URTGeometryGrammarLibrary::ToPolyline(const FRTGeometrySegment& Segment, float HexSize)
{
	FRTOccupancyPolyline Result;

	// Chi non ha validato non ottiene geometria: una polilinea vuota non occupa nulla (`ComputeMask` esce
	// sui meno di due punti), mentre coordinate calcolate su un asse inesistente sarebbero arbitrarie.
	if (ValidateSegment(Segment) != ERTGeometryViolation::None)
	{
		return Result;
	}

	// Il quanto e' RELATIVO al punto notevole della direzione, ed e' quello che rende esatto il perimetro:
	// `Offset = RT_GeometryQuanta` lungo la perpendicolare vale esattamente un punto medio di lato, e
	// `AlongStart/End = -+RT_GeometryQuanta/2` lungo l'asse cadono esattamente sui due vertici.
	const FVector2D Along = AxisPoint(Segment.Axis, HexSize) / RT_GeometryQuanta;
	const FVector2D Perp = AxisPerpendicularPoint(Segment.Axis, HexSize) / RT_GeometryQuanta;
	const FVector2D Base = Perp * static_cast<double>(Segment.Offset);

	Result.bClosed = false;
	Result.Points = {
		Base + Along * static_cast<double>(Segment.AlongStart),
		Base + Along * static_cast<double>(Segment.AlongEnd)
	};

	return Result;
}

bool URTGeometryGrammarLibrary::SnapToGrammar(const FVector2D& LocalA, const FVector2D& LocalB, float HexSize,
	FRTGeometrySegment& OutSegment)
{
	double BestError = TNumericLimits<double>::Max();
	bool bFound = false;

	for (int32 AxisIndex = 0; AxisIndex < RT_TacticalAxisCount; ++AxisIndex)
	{
		const ERTTacticalAxis Axis = static_cast<ERTTacticalAxis>(AxisIndex);

		// Le due direzioni sono ortogonali ma NON della stessa lunghezza — una punta a un vertice, l'altra a
		// un punto medio di lato. La proiezione divide quindi per il quadrato di ciascuna, non per uno.
		const FVector2D Along = AxisPoint(Axis, HexSize) / RT_GeometryQuanta;
		const FVector2D Perp = AxisPerpendicularPoint(Axis, HexSize) / RT_GeometryQuanta;

		const double AlongLenSq = Along.SizeSquared();
		const double PerpLenSq = Perp.SizeSquared();
		if (AlongLenSq <= UE_KINDA_SMALL_NUMBER || PerpLenSq <= UE_KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const double AlongA = FVector2D::DotProduct(LocalA, Along) / AlongLenSq;
		const double AlongB = FVector2D::DotProduct(LocalB, Along) / AlongLenSq;
		const double OffsetA = FVector2D::DotProduct(LocalA, Perp) / PerpLenSq;
		const double OffsetB = FVector2D::DotProduct(LocalB, Perp) / PerpLenSq;

		FRTGeometrySegment Candidate;
		Candidate.Axis = Axis;
		// Un solo offset per segmento: e' una retta, non una spezzata. La media e' cio' che minimizza
		// l'errore quando il gesto non e' perfettamente parallelo all'asse.
		Candidate.Offset = FMath::RoundToInt((OffsetA + OffsetB) * 0.5);
		Candidate.AlongStart = FMath::RoundToInt(AlongA);
		Candidate.AlongEnd = FMath::RoundToInt(AlongB);
		Candidate.Layer = 0;

		if (ValidateSegment(Candidate) != ERTGeometryViolation::None)
		{
			continue; // un candidato illegale non e' un candidato: niente lunghezza zero, niente fuori bordi
		}

		const FRTOccupancyPolyline Line = ToPolyline(Candidate, HexSize);
		if (Line.Points.Num() < 2)
		{
			continue;
		}

		// L'errore e' quanto il segmento quantizzato si scosta dal gesto: la somma delle due distanze.
		const double Error = FVector2D::Distance(Line.Points[0], LocalA) + FVector2D::Distance(Line.Points[1], LocalB);
		if (Error < BestError)
		{
			BestError = Error;
			OutSegment = Candidate;
			bFound = true;
		}
	}

	return bFound;
}
