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

namespace
{
	/**
	 * IL PUNTO NOTEVOLE PIU' VICINO: centro, sei vertici, sei punti medi di lato.
	 *
	 * ⚠️ Sono i tredici punti su cui l'autore disegna, e la misura dice che sono anche i soli che il
	 * reticolo sappia descrivere esattamente: ciascuno cade su coordinate INTERE nel reticolo di **ogni**
	 * asse, sempre dentro `{0, ±6, ±9, ±12}`. Non e' una comodita' d'interfaccia sovrapposta alla
	 * grammatica — e' la grammatica letta ad alta voce.
	 *
	 * Restituisce `false` se il punto e' troppo lontano per essere un punto di QUESTA cella: la' il gesto
	 * e' un muro lungo che attraversa piu' celle, e agganciarlo qui lo accorcerebbe.
	 */
	bool SnapToNotablePoint(const FVector2D& P, float HexSize, FVector2D& Out)
	{
		// Oltre questa distanza il gesto esce dalla cella e non e' piu' affar suo. `1.3` lascia respiro
		// oltre il circumraggio senza arrivare al centro del vicino, che dista `sqrt(3)` raggi.
		const double Reach = static_cast<double>(HexSize) * 1.3;
		if (P.Size() > Reach)
		{
			return false;
		}

		TArray<FVector2D> Boundary;
		URTHexOccupancyLibrary::SectorBoundaryPoints(HexSize, Boundary);

		FVector2D Best(0.0, 0.0);          // il centro e' il tredicesimo punto, e il primo candidato
		double BestDistSq = P.SizeSquared();
		for (const FVector2D& Point : Boundary)
		{
			const double DistSq = FVector2D::DistSquared(P, Point);
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Best = Point;
			}
		}

		Out = Best;
		return true;
	}
}

bool URTGeometryGrammarLibrary::SnapToGrammar(const FVector2D& LocalA, const FVector2D& LocalB, float HexSize,
	FRTGeometrySegment& OutSegment)
{
	double BestError = TNumericLimits<double>::Max();
	bool bFound = false;

	// ➕ **GLI ESTREMI SI AGGANCIANO AI PUNTI NOTEVOLI PRIMA DI CERCARE L'ASSE** (#712, seduta `U22`).
	//
	// 🔴 Senza, la tolleranza della mano era **mezzo quanto di `Offset`**: 3,61 uu su una cella da 100,
	// cioe' il 3,6% del raggio, pochi pixel in viewport. Misurato: con uno scarto del 4% comparivano i
	// primi muri sul lato sbagliato, all'8% un gesto su tre non produceva niente, al 15% ne falliva il 61%.
	// E il ghost era verde in tutti quei casi, perche' il candidato restava legale — solo, era un altro.
	//
	// Agganciandoli, il bacino di cattura di ogni punto diventa mezza distanza fra due punti notevoli
	// invece di mezzo quanto: l'ordine di grandezza passa da qualche pixel a un terzo di cella.
	FVector2D SnappedA = LocalA;
	FVector2D SnappedB = LocalB;
	const bool bSnappedA = SnapToNotablePoint(LocalA, HexSize, SnappedA);
	const bool bSnappedB = SnapToNotablePoint(LocalB, HexSize, SnappedB);

	// ⚠️ **Vale solo se ENTRAMBI gli estremi appartengono a questa cella.** Un muro lungo che la attraversa
	// ha gli estremi altrove — la grammatica lo prevede, `RT_GeometryMaxQuanta` sono quattro raggi — e
	// agganciarne uno solo lo accorcerebbe fino al bordo.
	const bool bBothOnNotablePoints = bSnappedA && bSnappedB;
	if (bBothOnNotablePoints && SnappedA.Equals(SnappedB, UE_KINDA_SMALL_NUMBER))
	{
		return false; // due estremi sullo stesso punto notevole: non c'e' nessun muro da fare
	}

	// 🔴 **OGNI ESTREMO SI AGGANCIA PER CONTO SUO.** La prima stesura scriveva
	// `bBothOnNotablePoints ? SnappedA : LocalA`, cioe' buttava via l'aggancio dell'estremo VICINO solo
	// perche' l'altro era lontano — due decisioni indipendenti legate insieme per distrazione. L'effetto
	// visto dall'autore: *«continua a disegnare al centro degli esagoni e la linea non parte dal vertice
	// iniziale, quando lo sposto lontano»*. Trascinando fuori dalla cella il punto di partenza perdeva
	// l'ancoraggio, e `Offset` tornava a essere una media che finiva quasi sempre a zero — cioe' il centro.
	const FVector2D SearchA = bSnappedA ? SnappedA : LocalA;
	const FVector2D SearchB = bSnappedB ? SnappedB : LocalB;

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

		const double AlongA = FVector2D::DotProduct(SearchA, Along) / AlongLenSq;
		const double AlongB = FVector2D::DotProduct(SearchB, Along) / AlongLenSq;
		const double OffsetA = FVector2D::DotProduct(SearchA, Perp) / PerpLenSq;
		const double OffsetB = FVector2D::DotProduct(SearchB, Perp) / PerpLenSq;

		FRTGeometrySegment Candidate;
		Candidate.Axis = Axis;

		// Un solo offset per segmento: e' una retta, non una spezzata.
		//
		// 🔴 **Ma la media va usata solo quando NESSUN estremo e' ancorato.** Un estremo agganciato a un
		// punto notevole e' una promessa — «il muro parte DA QUI» — e mediarlo con l'altro la rompe: basta
		// che il gesto esca dalla cella perche' l'offset dell'ancora venga diluito fino a zero, cioe' fino
		// al centro. E' il difetto che l'autore ha visto trascinando lontano.
		//
		// Non serve nessun controllo che la retta ci passi davvero: un punto notevole ha coordinate INTERE
		// nel reticolo di ogni asse — misurato, sempre dentro `{0, ±6, ±9, ±12}` — quindi arrotondare il
		// suo offset lo lascia dov'e'.
		//
		// ⚠️ `A` e' l'estremo della PRESSIONE e `B` quello del trascinamento (`UpdatePreview` passa
		// `LocalStart` e `LocalEnd` in quest'ordine): l'asimmetria e' voluta, e in caso di dubbio comanda
		// il punto da cui l'autore e' partito.
		if (bSnappedA)
		{
			Candidate.Offset = FMath::RoundToInt(OffsetA);
		}
		else if (bSnappedB)
		{
			Candidate.Offset = FMath::RoundToInt(OffsetB);
		}
		else
		{
			// La media minimizza l'errore quando il gesto non e' parallelo all'asse e non c'e' nessuna
			// ancora da rispettare: e' il caso del muro lungo che attraversa la cella da parte a parte.
			Candidate.Offset = FMath::RoundToInt((OffsetA + OffsetB) * 0.5);
		}
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
		const double Error = FVector2D::Distance(Line.Points[0], SearchA) + FVector2D::Distance(Line.Points[1], SearchB);

		// 🔴 **QUANDO GLI ESTREMI SONO PUNTI NOTEVOLI, IL CANDIDATO DEVE PASSARCI SOPRA — o non esiste.**
		//
		// Delle 78 coppie di punti notevoli **54 sono esprimibili** su un asse tattico e **24 no**: sono
		// vertice ↔ punto medio non adiacente, direzioni che nessuno dei sei assi porta. Senza questo
		// controllo la ricerca sceglierebbe comunque «l'asse meno peggio» e disegnerebbe un muro che non
		// passa per i due punti indicati — che e' cio' che l'autore ha visto: *«disegna anche muri fuori
		// dai segmenti validi»*. Un gesto fuori alfabeto deve dare ghost ROSSO, non un muro approssimato.
		//
		// ⚠️ La stretta vale SOLO quando entrambi gli estremi appartengono a questa cella. Un muro lungo
		// che la attraversa ha gli estremi fuori, e li' l'approssimazione e' il comportamento giusto: il
		// segmento continua nella cella accanto, dove sara' quella a descriverne la propria parte.
		if (bBothOnNotablePoints && Error > UE_KINDA_SMALL_NUMBER * 100.0)
		{
			continue;
		}

		if (Error < BestError)
		{
			BestError = Error;
			OutSegment = Candidate;
			bFound = true;
		}
	}

	return bFound;
}
