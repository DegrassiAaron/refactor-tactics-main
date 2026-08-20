#include "Map/RTGeometryBake.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexOccupancyLibrary.h"
#include "Map/RTHexLibrary.h" // DirectionForEdgeIndex: le due numerazioni dei bordi non coincidono

// ⚠️ Namespace NOMINATO, non anonimo: `RTHexOccupancyLibrary.cpp` ha helper con gli stessi nomi nel suo
// namespace anonimo, e nella unity build i due .cpp possono finire nella stessa unit di traduzione — dove
// due `Cross2D` anonimi collidono. Non e' un'ipotesi: e' successo, e la build precedente passava solo per
// come i file erano raggruppati in quel momento.
namespace RTGeometryBakeInternal
{
	/**
	 * Il corpo condiviso da `BakeCell` e `AddSegmentsToCell`. Le due vie differiscono in UN passo — se le
	 * coperture generate esistenti vengano rimosse prima di scrivere — e tenerle in una funzione sola e'
	 * cio' che impedisce alle altre regole (mano che vince, `High` su `Low`, ordine di bordo crescente) di
	 * divergere fra loro. Duplicare il corpo per cambiare una riga e' il modo in cui nascono i difetti che
	 * questa seduta ha passato la giornata a rincorrere.
	 */
	int32 Bake(URTHexMapAsset* Map, const FRTCellId& CellId, const TArray<FRTGeometrySegment>& Segments,
		float HexSize, bool bReplaceGenerated);

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

	// Il lato geometrico `k` va dal vertice `2k` al vertice `2k+2`, e il suo punto medio e' il confine
	// `2k+1`, a `60k` gradi.
	//
	// 🔴 **`k` NON e' `ERTHexDirection(k)`, e per quattro bordi su sei e' un'altra cosa.** Questa riga
	// faceva un `static_cast` diretto, e la seduta `U22` ha visto il muro comparire sul lato opposto.
	// Le due numerazioni girano in verso contrario:
	//
	// ```text
	// bordo geometrico k        punto medio a 60k gradi   ->   0    60   120   180   240   300
	// ERTHexDirection j         AxialDirection(j) punta a  ->   0   300   240   180   120    60
	// ```
	//
	// `E` (0) e `W` (3) coincidono perche' sono i due punti fissi del rispecchiamento; i quattro diagonali
	// si scambiano a coppie — `NE↔SE`, `NW↔SW`. La corrispondenza giusta e' quindi `j = (6 - k) % 6`.
	//
	// ⚠️ **Non e' un dettaglio di presentazione.** `ERTHexDirection` significa *«verso quel vicino»* — lo
	// dicono `NeighborAcross` in `RTHexDoorLibrary` e la risoluzione della copertura in
	// `RTHexCombatLibrary`, che cercano quale cella dell'anello sta in quella direzione. Una copertura
	// scritta sul diagonale sbagliato blocca vista e passo **dal lato opposto** a quello disegnato.
	//
	// ⚠️ Il difetto e' sopravvissuto perche' i test della cottura usano tutti `E` o `W`, cioe' proprio i
	// due casi in cui il rispecchiamento non si vede. `BakeCoverLandsTowardTheNeighbour` copre i sei.
	for (int32 Edge = 0; Edge < 6; ++Edge)
	{
		const FVector2D& V0 = Boundary[(2 * Edge) % RT_OccupancySectorCount];
		const FVector2D& V1 = Boundary[(2 * Edge + 2) % RT_OccupancySectorCount];

		if (RTGeometryBakeInternal::SegmentClosesEdge(Line.Points[0], Line.Points[1], V0, V1))
		{
			OutEdges.Add(URTHexLibrary::DirectionForEdgeIndex(Edge));
		}
	}

	// L'ordine crescente e' parte del contratto dichiarato nell'header, e il rimappaggio lo romperebbe
	// (produce 0,5,4,3,2,1). Si riordina qui invece di cambiare la promessa: due mappe uguali non devono
	// differire per come le coperture ci sono finite dentro.
	OutEdges.Sort([](const ERTHexDirection& A, const ERTHexDirection& B)
	{
		return static_cast<uint8>(A) < static_cast<uint8>(B);
	});
}

int32 URTGeometryBakeLibrary::BakeCell(URTHexMapAsset* Map, const FRTCellId& CellId,
	const TArray<FRTGeometrySegment>& Segments, float HexSize)
{
	return RTGeometryBakeInternal::Bake(Map, CellId, Segments, HexSize, /*bReplaceGenerated=*/ true);
}

int32 URTGeometryBakeLibrary::AddSegmentsToCell(URTHexMapAsset* Map, const FRTCellId& CellId,
	const TArray<FRTGeometrySegment>& Segments, float HexSize)
{
	return RTGeometryBakeInternal::Bake(Map, CellId, Segments, HexSize, /*bReplaceGenerated=*/ false);
}

int32 RTGeometryBakeInternal::Bake(URTHexMapAsset* Map, const FRTCellId& CellId,
	const TArray<FRTGeometrySegment>& Segments, float HexSize, bool bReplaceGenerated)
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

	// 1. Via le PROPRIE — **solo in rebake**. Quelle a mano restano dove sono: e' cio' che rende il rebake
	//    idempotente senza diventare distruttivo, ed e' la meta' di `D-131` che senza il campo non sarebbe
	//    esprimibile.
	//    ⚠️ In modo ADDITIVO questo passo si salta, ed e' l'intera differenza fra le due vie. Il disegno
	//    vede un gesto per volta e non possiede l'elenco dei segmenti della cella: farglielo eseguire
	//    significava cancellare il muro precedente a ogni tratto — il difetto trovato in `U22` disegnando
	//    due muri che condividono un vertice.
	if (bReplaceGenerated)
	{
		Updated.Covers.RemoveAll([](const FRTHexCover& Cover) { return Cover.bGenerated; });
	}

	// 2. Che cosa murerebbero i segmenti correnti. `High` prevale su `Low` se due segmenti insistono sullo
	//    stesso bordo: regola deterministica, invece di «vince l'ultimo arrivato».
	TMap<ERTHexDirection, ERTHexCoverType> Wanted;
	for (const FRTGeometrySegment& Segment : Segments)
	{
		TArray<ERTHexDirection> Edges;
		URTGeometryBakeLibrary::EdgesTouchedBy(Segment, HexSize, Edges);

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

		// Chi c'e' gia' su questo bordo? Un bordo ha al massimo una copertura — invariante di `ValidateMap`.
		FRTHexCover* Present = Updated.Covers.FindByPredicate(
			[Edge](const FRTHexCover& Cover) { return Cover.Edge == Edge; });

		if (Present != nullptr)
		{
			// Una copertura dipinta a mano vince sempre: il bordo e' gia' suo, e nessuna delle due vie la
			// sostituisce.
			if (!Present->bGenerated)
			{
				continue;
			}

			// ⚠️ Qui arriva SOLO la via additiva: in rebake le generate sono gia' state rimosse al passo 1,
			// quindi `Present` non puo' essere generata. Una generata che incontra un nuovo segmento segue
			// la stessa regola dei due segmenti sullo stesso bordo — `High` prevale su `Low` — invece di
			// «vince l'ultimo arrivato», che renderebbe il risultato dipendente dall'ordine del disegno.
			if (*Type == ERTHexCoverType::High && Present->Type != ERTHexCoverType::High)
			{
				Present->Type = ERTHexCoverType::High;
				Present->Integrity = FRTHexCover::DefaultIntegrity(ERTHexCoverType::High);
			}
			continue; // niente da CONTARE: la copertura c'era gia', questo gesto non ne ha aggiunta una
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
