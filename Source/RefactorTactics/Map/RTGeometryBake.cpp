#include "Map/RTGeometryBake.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexOccupancyLibrary.h"

// ⚠️ Namespace NOMINATO, non anonimo: `RTHexOccupancyLibrary.cpp` ha helper con gli stessi nomi nel suo
// namespace anonimo, e nella unity build i due .cpp possono finire nella stessa unit di traduzione — dove
// due `Cross2D` anonimi collidono. Non e' un'ipotesi: e' successo, e la build precedente passava solo per
// come i file erano raggruppati in quel momento.
namespace RTGeometryBakeInternal
{
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

	/**
	 * IL MURO CHIUDE QUESTO BORDO? — e la risposta NON e' quella di `ComputeMask`.
	 *
	 * L'occupancy usa un test d'intersezione deliberatamente **conservativo**: un contatto in un solo punto
	 * conta, perche' la' la domanda e' *«questa geometria invade il settore?»* e nel dubbio si invade. Qui la
	 * domanda e' un'altra — *«si passa da questo lato?»* — e la stessa regola darebbe la risposta sbagliata:
	 * un muro appoggiato al lato `E` ha gli estremi **sui due vertici**, e ogni vertice appartiene a DUE lati,
	 * quindi murerebbe anche i lati adiacenti che si limita a sfiorare. E' `MSE-4` applicata ai bordi.
	 *
	 * Un bordo e' chiuso solo se il muro lo **attraversa davvero** o vi **giace sopra** con lunghezza non
	 * nulla:
	 *
	 * ```text
	 * attraversamento PROPRIO   segni strettamente opposti da entrambe le parti
	 * sovrapposizione COLLINEARE  stessa retta, e proiezioni che si sovrappongono di piu' di un punto
	 * ```
	 *
	 * Non e' una seconda copia della regola dell'occupancy: e' una regola **diversa** per una domanda diversa,
	 * e le due non vanno unificate — l'occupancy deve restare conservativa, questa non puo' esserlo.
	 */
	bool SegmentClosesEdge(const FVector2D& A, const FVector2D& B, const FVector2D& C, const FVector2D& D)
	{
		const int32 D1 = SignOf(Cross2D(A, B, C));
		const int32 D2 = SignOf(Cross2D(A, B, D));
		const int32 D3 = SignOf(Cross2D(C, D, A));
		const int32 D4 = SignOf(Cross2D(C, D, B));

		// Attraversamento proprio: nessuno dei quattro punti e' sulla retta dell'altro segmento.
		if (D1 * D2 < 0 && D3 * D4 < 0)
		{
			return true;
		}

		// Collineari: contano solo se si sovrappongono di piu' di un punto. Proietto sull'asse dominante del
		// bordo per evitare la divisione, e confronto gli intervalli.
		if (D1 == 0 && D2 == 0)
		{
			const FVector2D Dir = D - C;
			const bool bUseX = FMath::Abs(Dir.X) >= FMath::Abs(Dir.Y);

			auto Project = [bUseX](const FVector2D& P) { return bUseX ? P.X : P.Y; };
			const double EdgeMin = FMath::Min(Project(C), Project(D));
			const double EdgeMax = FMath::Max(Project(C), Project(D));
			const double WallMin = FMath::Min(Project(A), Project(B));
			const double WallMax = FMath::Max(Project(A), Project(B));

			const double Overlap = FMath::Min(EdgeMax, WallMax) - FMath::Max(EdgeMin, WallMin);
			return Overlap > UE_KINDA_SMALL_NUMBER; // un solo punto in comune NON chiude il bordo
		}

		return false;
	}
}

void URTGeometryBakeLibrary::EdgesTouchedBy(const FRTGeometrySegment& Segment, float HexSize,
	TArray<ERTHexDirection>& OutEdges)
{
	OutEdges.Reset();

	// Un segmento fuori grammatica non produce geometria, quindi non muraglia nulla: `ToPolyline` restituisce
	// gia' una polilinea vuota, e qui il controllo e' esplicito perche' l'esito sia leggibile.
	const FRTOccupancyPolyline Line = URTGeometryGrammarLibrary::ToPolyline(Segment, HexSize);
	if (Line.Points.Num() < 2)
	{
		return;
	}

	TArray<FVector2D> Boundary;
	URTHexOccupancyLibrary::SectorBoundaryPoints(HexSize, Boundary);

	// Il lato `k` va dal vertice `2k` al vertice `2k+2`; la sua direzione e' `ERTHexDirection(k)` perche' il
	// suo punto medio e' il confine `2k+1`, a `60k` gradi. Iterazione in ordine crescente: l'esito non
	// dipende da come il segmento e' arrivato.
	for (int32 Edge = 0; Edge < 6; ++Edge)
	{
		const FVector2D& V0 = Boundary[(2 * Edge) % RT_OccupancySectorCount];
		const FVector2D& V1 = Boundary[(2 * Edge + 2) % RT_OccupancySectorCount];

		if (RTGeometryBakeInternal::SegmentClosesEdge(Line.Points[0], Line.Points[1], V0, V1))
		{
			OutEdges.Add(static_cast<ERTHexDirection>(Edge));
		}
	}
}

int32 URTGeometryBakeLibrary::BakeCell(URTHexMapAsset* Map, const FRTCellId& CellId,
	const TArray<FRTGeometrySegment>& Segments, float HexSize)
{
	if (Map == nullptr)
	{
		return 0;
	}

	const FRTHexCellData* Existing = Map->FindCell(CellId);
	if (Existing == nullptr)
	{
		return 0; // niente cella, niente bordi da murare
	}

	FRTHexCellData Updated = *Existing;

	// 1. Via le PROPRIE. Quelle a mano restano dove sono: e' cio' che rende il rebake idempotente senza
	//    diventare distruttivo, ed e' la meta' di `D-131` che senza il campo non sarebbe esprimibile.
	Updated.Covers.RemoveAll([](const FRTHexCover& Cover) { return Cover.bGenerated; });

	// 2. Che cosa murerebbero i segmenti correnti. `High` prevale su `Low` se due segmenti insistono sullo
	//    stesso bordo: regola deterministica, invece di «vince l'ultimo arrivato».
	TMap<ERTHexDirection, ERTHexCoverType> Wanted;
	for (const FRTGeometrySegment& Segment : Segments)
	{
		TArray<ERTHexDirection> Edges;
		EdgesTouchedBy(Segment, HexSize, Edges);

		for (const ERTHexDirection Edge : Edges)
		{
			ERTHexCoverType& Slot = Wanted.FindOrAdd(Edge, Segment.WallType);
			if (Segment.WallType == ERTHexCoverType::High)
			{
				Slot = ERTHexCoverType::High;
			}
		}
	}

	// 3. Scrittura in ordine di BORDO crescente, non nell'ordine di `TMap`: l'iterazione di una `TMap` non e'
	//    stabile, e l'array delle coperture entra nell'hash della mappa.
	int32 Generated = 0;
	for (int32 EdgeIndex = 0; EdgeIndex < 6; ++EdgeIndex)
	{
		const ERTHexDirection Edge = static_cast<ERTHexDirection>(EdgeIndex);
		const ERTHexCoverType* Type = Wanted.Find(Edge);
		if (Type == nullptr)
		{
			continue;
		}

		// Una copertura dipinta a mano vince: il bordo e' gia' suo, e il rebake non la sostituisce.
		const bool bHandPainted = Updated.Covers.ContainsByPredicate(
			[Edge](const FRTHexCover& Cover) { return Cover.Edge == Edge; });
		if (bHandPainted)
		{
			continue;
		}

		FRTHexCover Cover(Edge, *Type, FRTHexCover::DefaultIntegrity(*Type));
		Cover.bGenerated = true;
		Updated.Covers.Add(Cover);
		++Generated;
	}

	Map->AddOrUpdateCell(Updated);
	return Generated;
}

int32 URTGeometryBakeLibrary::CountGeneratedCovers(const URTHexMapAsset* Map, const FRTCellId& CellId)
{
	if (Map == nullptr)
	{
		return 0;
	}

	const FRTHexCellData* Cell = Map->FindCell(CellId);
	if (Cell == nullptr)
	{
		return 0;
	}

	int32 Count = 0;
	for (const FRTHexCover& Cover : Cell->Covers)
	{
		if (Cover.bGenerated)
		{
			++Count;
		}
	}
	return Count;
}
