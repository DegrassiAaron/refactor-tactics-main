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
	// UN MURO CONNETTE DUE PUNTI NOTEVOLI. Non «si avvicina a due punti notevoli»: li congiunge.
	//
	// 🔴 Questa funzione ha avuto tre stesure in un giorno, e le prime due cercavano l'asse migliore
	// arrotondando le coordinate del gesto. Il difetto sopravvissuto a entrambe, visto dall'autore:
	// *«son capitati muri fuori dalla geometria consentita»*. Con un solo estremo dentro la cella, l'altro
	// finiva su un `AlongEnd` qualunque — un intero legale ma che non corrisponde a nessun punto notevole —
	// e il muro si fermava a mezz'aria.
	//
	// La ricerca non arrotonda piu': **enumera le coppie di punti notevoli** e sceglie quella piu' vicina al
	// gesto. L'alfabeto diventa cosi' una proprieta' della costruzione invece di un vincolo da ricordare, e
	// non esiste piu' un modo di produrre un muro fuori da esso. Costa 13x13x6 prove, cioe' niente.
	//
	// I tredici punti sono centro, sei vertici, sei punti medi di lato. La misura che li rende un alfabeto e
	// non una preferenza: ciascuno cade su coordinate INTERE nel reticolo di **ogni** asse, sempre dentro
	// `{0, ±6, ±9, ±12}`. Delle 78 coppie, **54** stanno su un asse tattico e 24 no.
	TArray<FVector2D> Notable;
	Notable.Reserve(RT_OccupancySectorCount + 1);
	Notable.Add(FVector2D::ZeroVector); // il centro
	{
		TArray<FVector2D> Boundary;
		URTHexOccupancyLibrary::SectorBoundaryPoints(HexSize, Boundary);
		Notable.Append(Boundary);
	}

	// ⚠️ Il gesto deve toccare QUESTA cella. Se entrambi gli estremi sono lontani si tratta di un muro lungo
	// che la attraversa e basta: quel caso non appartiene a questa cella e non si aggancia ai suoi punti —
	// se ne occupera' la cella in cui l'autore ha davvero premuto.
	const double Reach = static_cast<double>(HexSize) * 1.3;
	if (LocalA.Size() > Reach && LocalB.Size() > Reach)
	{
		return false;
	}

	// ⚠️ **Un gesto senza lunghezza non e' un muro**, e va fermato QUI. La ricerca per coppie non se ne
	// accorgerebbe da sola: due estremi coincidenti restano due punti, e la coppia di punti notevoli piu'
	// vicina esiste comunque — quindi un semplice clic senza trascinamento produrrebbe un muro.
	// `SnapRejectsWhatCannotBeLegal` lo pinna dal 2026-08-16, ed e' caduto al primo giro di questo rewrite:
	// la vecchia stesura ci arrivava per un'altra strada (`AlongStart == AlongEnd` bocciato da
	// `ValidateSegment`), che enumerando le coppie non passa piu'.
	if (LocalA.Equals(LocalB, UE_KINDA_SMALL_NUMBER))
	{
		return false;
	}

	double BestError = TNumericLimits<double>::Max();
	bool bFound = false;

	for (int32 First = 0; First < Notable.Num(); ++First)
	{
		for (int32 Second = 0; Second < Notable.Num(); ++Second)
		{
			if (First == Second)
			{
				continue; // due estremi sullo stesso punto: nessun muro da fare
			}

			// L'orientamento conta: `First` e' l'estremo della PRESSIONE, `Second` quello del trascinamento.
			// Entrambe le assegnazioni vengono provate, perche' i due indici scorrono tutte le coppie in
			// entrambi i versi, e vince quella che si scosta meno dal gesto.
			const FVector2D& Start = Notable[First];
			const FVector2D& End = Notable[Second];
			const double Error = FVector2D::Distance(Start, LocalA) + FVector2D::Distance(End, LocalB);
			if (Error >= BestError)
			{
				continue; // gia' peggiore del migliore: inutile cercargli un asse
			}

			for (int32 AxisIndex = 0; AxisIndex < RT_TacticalAxisCount; ++AxisIndex)
			{
				const ERTTacticalAxis Axis = static_cast<ERTTacticalAxis>(AxisIndex);

				// Le due direzioni sono ortogonali ma NON della stessa lunghezza — una punta a un vertice,
				// l'altra a un punto medio di lato. La proiezione divide quindi per il quadrato di ciascuna.
				const FVector2D Along = AxisPoint(Axis, HexSize) / RT_GeometryQuanta;
				const FVector2D Perp = AxisPerpendicularPoint(Axis, HexSize) / RT_GeometryQuanta;

				const double AlongLenSq = Along.SizeSquared();
				const double PerpLenSq = Perp.SizeSquared();
				if (AlongLenSq <= UE_KINDA_SMALL_NUMBER || PerpLenSq <= UE_KINDA_SMALL_NUMBER)
				{
					continue;
				}

				const double AlongStart = FVector2D::DotProduct(Start, Along) / AlongLenSq;
				const double AlongEnd = FVector2D::DotProduct(End, Along) / AlongLenSq;
				const double OffsetStart = FVector2D::DotProduct(Start, Perp) / PerpLenSq;
				const double OffsetEnd = FVector2D::DotProduct(End, Perp) / PerpLenSq;

				// I due punti devono stare sulla STESSA retta di questo asse, e cadere su coordinate intere.
				// Sono le 24 coppie su 78 che nessun asse tattico porta: vertice contro punto medio non
				// adiacente. Qui vengono scartate, e il ghost restera' rosso invece di inventare un muro.
				auto IsWhole = [](double Value)
				{
					return FMath::Abs(Value - FMath::RoundToDouble(Value)) < 0.001;
				};
				if (!IsWhole(AlongStart) || !IsWhole(AlongEnd) || !IsWhole(OffsetStart) || !IsWhole(OffsetEnd))
				{
					continue;
				}
				if (FMath::RoundToInt(OffsetStart) != FMath::RoundToInt(OffsetEnd))
				{
					continue;
				}

				FRTGeometrySegment Candidate;
				Candidate.Axis = Axis;
				Candidate.Offset = FMath::RoundToInt(OffsetStart);
				Candidate.AlongStart = FMath::RoundToInt(AlongStart);
				Candidate.AlongEnd = FMath::RoundToInt(AlongEnd);
				Candidate.Layer = 0;

				if (ValidateSegment(Candidate) != ERTGeometryViolation::None)
				{
					continue;
				}

				BestError = Error;
				OutSegment = Candidate;
				bFound = true;
				break; // trovato l'asse di questa coppia: gli altri darebbero lo stesso muro
			}
		}
	}

	return bFound;
}
