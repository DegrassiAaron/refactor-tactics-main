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
