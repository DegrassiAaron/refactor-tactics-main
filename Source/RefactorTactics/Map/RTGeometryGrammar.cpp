#include "Map/RTGeometryGrammar.h"
#include "Map/RTHexLibrary.h"

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

// ============================================================================
//  INCIDENZA FRA DUE SEGMENTI — `#1894`, `GEO-6` e `GEO-9` di `D-288`
// ============================================================================

namespace
{
	/**
	 * IL RETICOLO INTERO in cui l'incidenza si decide — la §11 di `spec-hex-geometry-authoring.md`
	 * applicata a una domanda che riguarda DUE segmenti invece di uno.
	 *
	 * 🔑 **Il problema che risolve.** Due segmenti su assi diversi si incontrano in un punto, e la domanda
	 * e' *«quel punto e' uno dei tredici anchor?»*. Chiesta in `FVector2D` sarebbe un confronto con
	 * tolleranza — e una tolleranza qui non e' un dettaglio di precisione, e' una regola di gioco: decide
	 * quali muri un livello puo' contenere, e la stessa mappa potrebbe validarsi in due modi diversi.
	 * La grammatica e' discreta apposta, e la sua validazione deve restarlo.
	 *
	 * **La misura che rende possibile il calcolo**: i tredici punti notevoli cadono su coordinate INTERE
	 * nella base `(HexSize * sqrt(3)/4, HexSize/4)`. Il vertice a `-30` gradi e' `(2, -2)`, il punto medio
	 * del lato `E` e' `(2, 0)`, quello del lato `NE` e' `(1, 3)`: l'apotema irrazionale che
	 * `RT_GeometryQuanta` esiste per aggirare sparisce, perche' `sqrt(3)` finisce nell'UNITA' dell'asse `X`
	 * invece che nelle coordinate.
	 *
	 * ⚠️ **I dodici punti non sono incisi qui.** Vengono derivati da `SectorBoundaryPoints` — l'unica
	 * definizione — e arrotondati, verificando che l'arrotondamento sia esatto. Una tabella scritta a mano
	 * sarebbe la seconda copia della convenzione dei sei lati, che e' il difetto che `#588` ha gia' pagato:
	 * se quella convenzione cambiasse, la tabella mentirebbe in silenzio, mentre questa derivazione la
	 * segue — e se smettesse di essere intera si dichiarerebbe non valida invece di approssimare.
	 *
	 * Le coordinate sono SCALATE di `RT_GeometryQuanta`, cosi' che anche i punti interni al segmento —
	 * `Perp * Offset + Along * t`, che valgono un dodicesimo di unita' ciascuno — restino interi.
	 */
	constexpr double RTIncidenceReferenceHexSize = 100.0;

	/** Un punto del reticolo. `int64` perche' i determinanti moltiplicano due coordinate fra loro. */
	struct FRTIncidencePoint
	{
		int64 X = 0;
		int64 Y = 0;
	};

	struct FRTIncidenceLattice
	{
		/** I dodici confini di settore, in unita' della base. Componenti in `{0, ±1, ±2, ±3, ±4}`. */
		FRTIncidencePoint Boundary[RT_OccupancySectorCount];

		/** I tredici anchor, gia' scalati di `RT_GeometryQuanta`: centro, sei vertici, sei punti medi. */
		FRTIncidencePoint Anchors[RT_AnchorsPerCell];

		/**
		 * ⚠️ Falso se la derivazione NON e' risultata intera. Non e' difensivismo: e' l'unico modo in cui
		 * questa regola puo' dichiarare che la geometria le e' cambiata sotto, invece di arrotondare un
		 * punto vicino a un anchor e chiamarlo anchor.
		 */
		bool bValid = false;
	};

	const FRTIncidenceLattice& RTIncidenceLatticeOf()
	{
		// Costruito una volta sola: dipende da `SectorBoundaryPoints` con un `HexSize` di RIFERIMENTO, e il
		// reticolo e' adimensionale — le unita' scalano con la cella, gli interi no. Un `HexSize` diverso
		// darebbe gli stessi tredici interi.
		static const FRTIncidenceLattice Lattice = []()
		{
			FRTIncidenceLattice L;

			TArray<FVector2D> Boundary;
			URTHexOccupancyLibrary::SectorBoundaryPoints(RTIncidenceReferenceHexSize, Boundary);
			if (Boundary.Num() != RT_OccupancySectorCount)
			{
				return L;
			}

			const double UnitX = RTIncidenceReferenceHexSize * FMath::Sqrt(3.0) / 4.0;
			const double UnitY = RTIncidenceReferenceHexSize / 4.0;

			// Lo scarto misurato sui dodici punti e' dell'ordine di `1e-16` relativo: la soglia e' larga
			// dieci ordini di grandezza e resta lontanissima dal mezzo passo che confonderebbe due interi.
			const double Tolerance = 1e-6;

			L.Anchors[0] = FRTIncidencePoint{ 0, 0 }; // il centro
			for (int32 I = 0; I < RT_OccupancySectorCount; ++I)
			{
				const double Fx = Boundary[I].X / UnitX;
				const double Fy = Boundary[I].Y / UnitY;
				const int64 Rx = static_cast<int64>(FMath::RoundToInt(Fx));
				const int64 Ry = static_cast<int64>(FMath::RoundToInt(Fy));
				if (FMath::Abs(Fx - static_cast<double>(Rx)) > Tolerance
					|| FMath::Abs(Fy - static_cast<double>(Ry)) > Tolerance)
				{
					return L; // `bValid` resta falso: la base non e' piu' quella, e nessuna regola si applica
				}
				L.Boundary[I] = FRTIncidencePoint{ Rx, Ry };
				L.Anchors[I + 1] = FRTIncidencePoint{ Rx * RT_GeometryQuanta, Ry * RT_GeometryQuanta };
			}

			L.bValid = true;
			return L;
		}();
		return Lattice;
	}

	/** Dove cade il segmento al parametro `Along`, nel reticolo scalato. */
	FRTIncidencePoint RTIncidencePointAt(const FRTIncidenceLattice& L, const FRTGeometrySegment& Segment,
		int32 Along)
	{
		// Gli indici sono quelli di `AxisPoint` e `AxisPerpendicularPoint`: la corrispondenza fra i sei assi
		// e i dodici confini resta scritta in `AxisBoundaryIndex` e in nessun altro posto.
		const int32 Base = URTGeometryGrammarLibrary::AxisBoundaryIndex(Segment.Axis);
		const FRTIncidencePoint& Dir = L.Boundary[Base % RT_OccupancySectorCount];
		const FRTIncidencePoint& Perp = L.Boundary[(Base + 9) % RT_OccupancySectorCount];

		return FRTIncidencePoint{
			Perp.X * Segment.Offset + Dir.X * Along,
			Perp.Y * Segment.Offset + Dir.Y * Along
		};
	}

	int64 RTIncidenceCross(int64 UX, int64 UY, int64 VX, int64 VY)
	{
		return UX * VY - UY * VX;
	}

	/**
	 * I DUE SEGMENTI SI INCONTRANO FUORI DA UN ANCHOR?
	 *
	 * Presuppone assi DIVERSI — due segmenti dello stesso asse sono paralleli o collineari, e li tratta la
	 * regola di sovrapposizione. Estremi INCLUSI: toccarsi in un punto e' incontrarsi, ed e' esattamente il
	 * caso della T che questa regola deve giudicare invece di ignorare.
	 */
	bool RTIncidenceMeetsOffAnchor(const FRTIncidenceLattice& L, const FRTGeometrySegment& A,
		const FRTGeometrySegment& B)
	{
		if (!L.bValid)
		{
			return false; // senza reticolo non si giudica: meglio nessuna regola di una regola inventata
		}

		const FRTIncidencePoint A0 = RTIncidencePointAt(L, A, A.AlongStart);
		const FRTIncidencePoint A1 = RTIncidencePointAt(L, A, A.AlongEnd);
		const FRTIncidencePoint B0 = RTIncidencePointAt(L, B, B.AlongStart);
		const FRTIncidencePoint B1 = RTIncidencePointAt(L, B, B.AlongEnd);

		const int64 D1X = A1.X - A0.X, D1Y = A1.Y - A0.Y;
		const int64 D2X = B1.X - B0.X, D2Y = B1.Y - B0.Y;
		const int64 WX = B0.X - A0.X, WY = B0.Y - A0.Y;

		int64 Den = RTIncidenceCross(D1X, D1Y, D2X, D2Y);
		if (Den == 0)
		{
			return false; // paralleli: nessun punto d'incontro isolato da giudicare
		}

		int64 SNum = RTIncidenceCross(WX, WY, D2X, D2Y);
		int64 UNum = RTIncidenceCross(WX, WY, D1X, D1Y);
		if (Den < 0)
		{
			Den = -Den;
			SNum = -SNum;
			UNum = -UNum;
		}

		// Il punto d'incontro delle due RETTE cade dentro entrambi i TRATTI, estremi compresi.
		if (SNum < 0 || SNum > Den || UNum < 0 || UNum > Den)
		{
			return false;
		}

		// Il punto e' `A0 + D1 * SNum / Den`: si confronta moltiplicato per `Den`, cosi' che la divisione —
		// l'unico passo che uscirebbe dagli interi — non venga mai eseguita.
		const int64 PX = A0.X * Den + D1X * SNum;
		const int64 PY = A0.Y * Den + D1Y * SNum;

		for (const FRTIncidencePoint& Anchor : L.Anchors)
		{
			if (PX == Anchor.X * Den && PY == Anchor.Y * Den)
			{
				return false; // si incontrano su un anchor: e' il caso normale, ed e' legale
			}
		}
		return true;
	}

	/**
	 * DUE COLLINEARI SI SOVRAPPONGONO SU PIU' DI UN PUNTO?
	 *
	 * Presuppone stessa giacitura — asse, offset e layer — verificata dal chiamante. Il confronto e' STRETTO
	 * apposta: con `<=` due segmenti consecutivi che condividono un estremo risulterebbero sovrapposti, e un
	 * muro lungo disegnato in due gesti — il modo normale di disegnarlo — diventerebbe invalido.
	 *
	 * ⚠️ `Min`/`Max` invece degli estremi come sono scritti: sono una coppia NON ordinata, per la stessa
	 * ragione per cui `operator==` li tratta cosi'. Senza, la regola si aggirerebbe disegnando al contrario.
	 */
	bool RTIncidenceOverlaps(const FRTGeometrySegment& A, const FRTGeometrySegment& B)
	{
		const int32 AMin = FMath::Min(A.AlongStart, A.AlongEnd);
		const int32 AMax = FMath::Max(A.AlongStart, A.AlongEnd);
		const int32 BMin = FMath::Min(B.AlongStart, B.AlongEnd);
		const int32 BMax = FMath::Max(B.AlongStart, B.AlongEnd);
		return FMath::Max(AMin, BMin) < FMath::Min(AMax, BMax);
	}
}

void URTGeometryGrammarLibrary::Validate(const TArray<FRTGeometrySegment>& Segments, TArray<FRTGeometryIssue>& OutIssues)
{
	OutIssues.Reset();

	const FRTIncidenceLattice& Lattice = RTIncidenceLatticeOf();

	auto Report = [&OutIssues](int32 Index, ERTGeometryViolation Violation, int32 Other)
	{
		FRTGeometryIssue Issue;
		Issue.SegmentIndex = Index;
		Issue.Violation = Violation;
		Issue.OtherIndex = Other;
		OutIssues.Add(Issue);
	};

	for (int32 Index = 0; Index < Segments.Num(); ++Index)
	{
		const FRTGeometrySegment& Segment = Segments[Index];

		const ERTGeometryViolation Violation = ValidateSegment(Segment);
		if (Violation != ERTGeometryViolation::None)
		{
			Report(Index, Violation, INDEX_NONE);
			continue;
		}

		// Il duplicato e' una proprieta' della COLLEZIONE, non del segmento: si segnala sulla seconda
		// occorrenza, cosi' che rimuovendola la collezione diventi valida. Confronto lineare all'indietro:
		// nessun ordinamento e nessun hash, quindi l'esito non dipende dall'ordine di iterazione di una
		// `TMap`/`TSet` — l'invariante di determinismo del repository.
		int32 Duplicate = INDEX_NONE;
		for (int32 Previous = 0; Previous < Index; ++Previous)
		{
			if (Segments[Previous] == Segment)
			{
				Duplicate = Previous;
				break;
			}
		}
		if (Duplicate != INDEX_NONE)
		{
			// ⚠️ E si FERMA qui. Un duplicato e' anche, geometricamente, una sovrapposizione totale: senza
			// questa uscita lo stesso segmento porterebbe due reason code, e la verifica di mutazione —
			// «allentata una regola per volta, cade esattamente il test che la protegge» — non saprebbe piu'
			// quale delle due ha ceduto. Un segmento identico si toglie, e le altre relazioni si rileggono
			// sulla collezione risanata.
			Report(Index, ERTGeometryViolation::DuplicateSegment, Duplicate);
			continue;
		}

		// L'INCIDENZA — `#1894`. Due segmenti si incontrano solo su un anchor, e due collineari non si
		// sovrappongono. Sono relazioni fra una COPPIA: come il duplicato si segnalano sul secondo dei due,
		// con `OtherIndex` che nomina il primo, e con lo stesso confronto all'indietro che le rende
		// indipendenti dall'ordine della collezione.
		for (int32 Previous = 0; Previous < Index; ++Previous)
		{
			const FRTGeometrySegment& Other = Segments[Previous];

			// ⚠️ Chi e' gia' fuori grammatica non entra in una relazione: la sua geometria non e' definita —
			// un asse inesistente non ha una direzione — e segnalarlo due volte direbbe a chi disegna di
			// aggiustare un incrocio quando il difetto e' il segmento stesso.
			if (ValidateSegment(Other) != ERTGeometryViolation::None)
			{
				continue;
			}

			// La geometria di un piano non tocca quella di un altro.
			if (Other.Layer != Segment.Layer)
			{
				continue;
			}

			if (Other.Axis == Segment.Axis)
			{
				// Stessa giacitura: o sono la STESSA retta — e allora la domanda e' se i tratti si
				// sovrappongono — o sono due parallele distinte, che non si incontrano mai. Distinguere
				// sull'offset invece che sul solo asse e' cio' che impedisce di segnalare due muri paralleli.
				if (Other.Offset == Segment.Offset && RTIncidenceOverlaps(Other, Segment))
				{
					Report(Index, ERTGeometryViolation::OverlappingSegments, Previous);
				}
				continue;
			}

			if (RTIncidenceMeetsOffAnchor(Lattice, Other, Segment))
			{
				Report(Index, ERTGeometryViolation::CrossingOffAnchor, Previous);
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


// ============================================================================
//  ANCHOR — palette, posizione locale, chiave canonica  (`#1893`, `D-288`)
// ============================================================================

FString FRTAnchorRef::ToString() const
{
	switch (Kind)
	{
	case ERTAnchorKind::Vertex:  return FString::Printf(TEXT("%s/V%d"), *Cell.ToString(), Index);
	case ERTAnchorKind::EdgeMid: return FString::Printf(TEXT("%s/E%d"), *Cell.ToString(), Index);
	default:                     return FString::Printf(TEXT("%s/C"), *Cell.ToString());
	}
}

namespace
{
	/** L'indice ridotto a `0..5`. Regge un dato arrivato da una deserializzazione, come fa la grammatica. */
	int32 RTWrapAnchorIndex(int32 Index)
	{
		return ((Index % 6) + 6) % 6;
	}
}

void URTGeometryGrammarLibrary::AnchorsOfCell(const FRTCellId& Cell, TArray<FRTAnchorRef>& OutAnchors)
{
	OutAnchors.Reset();
	OutAnchors.Reserve(RT_AnchorsPerCell);

	// Ordine dichiarato: centro, poi i sei vertici, poi i sei punti medi. Chi itera non deve dipendere
	// dall'ordine, ma chi lo legge in un log deve trovarlo sempre uguale.
	OutAnchors.Add(FRTAnchorRef(Cell, ERTAnchorKind::Center));
	for (int32 K = 0; K < 6; ++K)
	{
		OutAnchors.Add(FRTAnchorRef(Cell, ERTAnchorKind::Vertex, K));
	}
	for (int32 J = 0; J < 6; ++J)
	{
		OutAnchors.Add(FRTAnchorRef(Cell, ERTAnchorKind::EdgeMid, J));
	}
}

FVector2D URTGeometryGrammarLibrary::AnchorLocal(const FRTAnchorRef& Ref, float HexSize)
{
	if (Ref.Kind == ERTAnchorKind::Center)
	{
		return FVector2D::ZeroVector;
	}

	// ⚠️ I dodici punti hanno UNA definizione, e non e' qui: `SectorBoundaryPoints` li ancora al primo
	// vertice nello stesso verso in cui `URTHexLibrary` enumera il perimetro. Ricalcolare i coseni sarebbe
	// la seconda copia della stessa formula — il difetto che `#588` ha gia' pagato.
	TArray<FVector2D> Boundary;
	URTHexOccupancyLibrary::SectorBoundaryPoints(HexSize, Boundary);
	if (Boundary.Num() != RT_OccupancySectorCount)
	{
		return FVector2D::ZeroVector;
	}

	// I confini alternano vertice e punto medio: il vertice `k` e' il confine `2k`, il punto medio del lato
	// `j` e' il confine `2j + 1`.
	const int32 Index = RTWrapAnchorIndex(Ref.Index);
	const int32 BoundaryIndex = Ref.Kind == ERTAnchorKind::Vertex ? 2 * Index : 2 * Index + 1;
	return Boundary[BoundaryIndex];
}

FRTAnchorRef URTGeometryGrammarLibrary::CanonicalAnchor(const FRTAnchorRef& Ref)
{
	// Il centro appartiene a una cella sola: non ha nessuno con cui accordarsi, ed e' gia' il proprio
	// rappresentante.
	if (Ref.Kind == ERTAnchorKind::Center)
	{
		return FRTAnchorRef(Ref.Cell, ERTAnchorKind::Center);
	}

	const int32 Index = RTWrapAnchorIndex(Ref.Index);

	// I modi in cui questo stesso punto puo' essere nominato. Il primo e' sempre il riferimento dato, cosi'
	// la funzione e' totale anche se la geometria cambiasse.
	TArray<FRTAnchorRef, TInlineAllocator<3>> Named;
	Named.Add(FRTAnchorRef(Ref.Cell, Ref.Kind, Index));

	if (Ref.Kind == ERTAnchorKind::EdgeMid)
	{
		// Un punto medio appartiene a DUE celle: e' il punto medio del lato opposto, visto dal vicino che
		// quel lato separa.
		const ERTHexDirection Dir = URTHexLibrary::DirectionForEdgeIndex(Index);
		const int32 Mirrored = URTHexLibrary::EdgeIndexForDirection(URTHexLibrary::OppositeDirection(Dir));
		Named.Add(FRTAnchorRef(URTHexLibrary::Neighbor(Ref.Cell, Dir), ERTAnchorKind::EdgeMid, Mirrored));
	}
	else
	{
		// Un vertice appartiene a TRE celle: la propria e i due vicini oltre i due lati che lo contengono.
		// Il vertice `k` sta fra il lato `k - 1` e il lato `k`, e nei due vicini prende gli indici `k + 2`
		// e `k + 4` — le due rotazioni di un terzo di giro, che e' cio' che tre esagoni attorno a un punto
		// sono.
		const int32 Previous = RTWrapAnchorIndex(Index - 1);
		Named.Add(FRTAnchorRef(
			URTHexLibrary::Neighbor(Ref.Cell, URTHexLibrary::DirectionForEdgeIndex(Previous)),
			ERTAnchorKind::Vertex, RTWrapAnchorIndex(Index + 2)));
		Named.Add(FRTAnchorRef(
			URTHexLibrary::Neighbor(Ref.Cell, URTHexLibrary::DirectionForEdgeIndex(Index)),
			ERTAnchorKind::Vertex, RTWrapAnchorIndex(Index + 4)));
	}

	// Il rappresentante e' la cella che `StableLess` mette per prima — lo stesso ordinamento con cui
	// `SortCells` e `ComputeHash` rendono deterministico tutto il resto dell'asset. Una seconda convenzione
	// d'ordine sarebbe un secondo posto da tenere allineato.
	FRTAnchorRef Best = Named[0];
	for (const FRTAnchorRef& Candidate : Named)
	{
		if (URTHexLibrary::StableLess(Candidate.Cell, Best.Cell))
		{
			Best = Candidate;
		}
	}
	return Best;
}

bool URTGeometryGrammarLibrary::SegmentBetweenAnchors(const FRTAnchorRef& A, const FRTAnchorRef& B,
	float HexSize, FRTGeometrySegment& OutSegment)
{
	// La grammatica e' definita PER CELLA: due anchor di celle diverse non hanno un sistema di coordinate
	// comune in cui dire un segmento. Un muro lungo e' piu' segmenti, uno per cella, e chi lo vuole passa
	// da `URTHexLibrary::SplitSegmentAcrossCells`.
	if (A.Cell != B.Cell)
	{
		return false;
	}

	const FVector2D PA = AnchorLocal(A, HexSize);
	const FVector2D PB = AnchorLocal(B, HexSize);

	FRTGeometrySegment Snapped;
	if (!SnapToGrammar(PA, PB, HexSize, Snapped))
	{
		return false;
	}
	Snapped.Layer = A.Cell.Layer;

	// 🔴 **LA VERIFICA DI FEDELTA', ed e' l'intera ragione per cui questa funzione non e' `SnapToGrammar`.**
	// Quello *«tiene l'asse che sbaglia meno»* e rifiuta solo un gesto degenere o fuori dai bordi: chiesta
	// una delle ventiquattro coppie che nessun asse porta, non fallisce — produce un muro **diverso** da
	// quello chiesto. Qui la domanda e' un'altra: *«la grammatica esprime QUESTA coppia?»*, e la risposta e'
	// no quando gli estremi prodotti non sono i due anchor chiesti. E' `GEO-8` di `D-288`.
	const FRTOccupancyPolyline Line = ToPolyline(Snapped, HexSize);
	if (Line.Points.Num() != 2)
	{
		return false;
	}

	// Tolleranza relativa alla cella: la grammatica e' esatta sui punti notevoli, quindi lo scarto ammesso
	// e' quello di macchina, non un margine di comodo che farebbe passare un anchor vicino per quello giusto.
	const double Tolerance = static_cast<double>(HexSize) * 1e-4;
	const bool bForward = Line.Points[0].Equals(PA, Tolerance) && Line.Points[1].Equals(PB, Tolerance);
	const bool bBackward = Line.Points[0].Equals(PB, Tolerance) && Line.Points[1].Equals(PA, Tolerance);
	if (!bForward && !bBackward)
	{
		return false;
	}

	OutSegment = Snapped;
	return true;
}

FRTAnchorRef URTGeometryGrammarLibrary::NearestAnchor(const FRTCellId& Cell, const FVector2D& Local,
	float HexSize)
{
	TArray<FRTAnchorRef> Anchors;
	AnchorsOfCell(Cell, Anchors);

	// La palette e' chiusa e non vuota per costruzione; se `AnchorsOfCell` cambiasse, il centro resta la
	// risposta totale invece di un riferimento non inizializzato.
	FRTAnchorRef Best(Cell, ERTAnchorKind::Center);
	double BestDistSq = TNumericLimits<double>::Max();

	for (const FRTAnchorRef& Candidate : Anchors)
	{
		const double DistSq = FVector2D::DistSquared(AnchorLocal(Candidate, HexSize), Local);

		// ⚠️ Strettamente minore: a parita' vince il PRIMO nell'ordine dichiarato da `AnchorsOfCell`. La
		// parita' non e' teorica — un gesto esattamente a meta' fra due vertici la produce — e senza un
		// criterio esplicito l'esito dipenderebbe dall'ordine di iterazione, che e' l'invariante n. 4.
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Candidate;
		}
	}

	return Best;
}

ERTAnchorPairRefusal URTGeometryGrammarLibrary::ExplainPair(const FRTAnchorRef& A, const FRTAnchorRef& B,
	float HexSize)
{
	// L'ordine e' quello di `SegmentBetweenAnchors`, e conta: su due anchor di celle diverse la domanda
	// «sta su un asse?» non ha significato, perche' le loro coordinate sono misurate in due sistemi diversi.
	if (A.Cell.X != B.Cell.X || A.Cell.Y != B.Cell.Y)
	{
		return ERTAnchorPairRefusal::DifferentCell;
	}
	if (A.Cell.Layer != B.Cell.Layer)
	{
		return ERTAnchorPairRefusal::DifferentLayer;
	}

	// Lo STESSO anchor: confronto sulla forma canonica, cosi' che un centro con indice sporco — che un
	// `FRTAnchorRef` riempito campo per campo puo' portarsi dietro — non sembri un secondo punto.
	if (CanonicalAnchor(A) == CanonicalAnchor(B))
	{
		return ERTAnchorPairRefusal::SameAnchor;
	}

	// 🔑 **Qui si CHIEDE, non si ricalcola.** L'esprimibilita' e' esattamente cio' che `SegmentBetweenAnchors`
	// decide con la verifica di fedelta' di `GEO-8`; riderivarla con un'aritmetica sugli indici — «vertice
	// contro punto medio non adiacente» — sarebbe una seconda definizione delle ventiquattro, che il giorno
	// in cui la grammatica guadagnasse un asse mentirebbe in silenzio.
	FRTGeometrySegment Unused;
	if (!SegmentBetweenAnchors(A, B, HexSize, Unused))
	{
		return ERTAnchorPairRefusal::NoTacticalAxis;
	}

	return ERTAnchorPairRefusal::None;
}
