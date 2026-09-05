#include "Map/RTHexOcclusionLibrary.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexOccupancyLibrary.h"

namespace
{
	// Prefisso `Occlusion`: la unity build fonde i namespace anonimi, e un nome generico collide con
	// l'omonimo di un altro file (`#1530`).

	/**
	 * I DODICI PUNTI DI CONFINE, esatti, in unita' `HexSize / 4` con la radice di tre fattorizzata.
	 *
	 * Confine `s` = angolo `-30 + 30 * s`, raggio pieno per `s` pari (vertice) e apotema per `s` dispari
	 * (punto medio di lato) — la stessa costruzione di `URTHexOccupancyLibrary::SectorBoundaryPoints`,
	 * scritta in interi invece che in coseni.
	 *
	 * ⚠️ E' l'unica trascrizione di questa libreria, quindi e' anche l'unico posto che potrebbe mentire:
	 * `Occlusion.BoundaryTableMatchesTheFloatOracle` la confronta con l'originale punto per punto.
	 */
	constexpr int64 OcclusionBoundaryR3[RT_OccupancySectorCount] = { 2, 2, 2, 1, 0, -1, -2, -2, -2, -1, 0, 1 };
	constexpr int64 OcclusionBoundaryOne[RT_OccupancySectorCount] = { -2, 0, 2, 3, 4, 3, 2, 0, -2, -3, -4, -3 };

	int32 OcclusionWrapBoundary(int32 Index)
	{
		return ((Index % RT_OccupancySectorCount) + RT_OccupancySectorCount) % RT_OccupancySectorCount;
	}

	/**
	 * Il prodotto vettoriale `(A - O) x (B - O)`, **senza** il fattore radice di tre.
	 *
	 * Quel fattore e' positivo e comune a tutti i termini, quindi non cambia nessun segno: ometterlo e' cio'
	 * che tiene l'intero calcolo in aritmetica intera esatta. Vedi `FRTLocalPointQ`.
	 */
	int64 OcclusionCross(const FRTLocalPointQ& O, const FRTLocalPointQ& A, const FRTLocalPointQ& B)
	{
		const int64 A3 = A.R3 - O.R3;
		const int64 A1 = A.One - O.One;
		const int64 B3 = B.R3 - O.R3;
		const int64 B1 = B.One - O.One;
		return A3 * B1 - A1 * B3;
	}

	/** I due segni sono stretti e opposti: il punto sta da una parte e l'altro dall'altra, nessuno sopra. */
	bool OcclusionStraddles(int64 D1, int64 D2)
	{
		return (D1 > 0 && D2 < 0) || (D1 < 0 && D2 > 0);
	}

	/** Il capo della corda dal lato di `Neighbor`: il centro se non c'e' vicino, il punto medio del lato se c'e'. */
	bool OcclusionChordEnd(const FRTCellId& Neighbor, const FRTCellId& Cell, FRTLocalPointQ& Out)
	{
		if (Neighbor == Cell)
		{
			Out = FRTLocalPointQ(0, 0); // la linea nasce o muore qui: il capo e' il CENTRO
			return true;
		}

		ERTHexDirection Dir = ERTHexDirection::E;
		if (!URTHexLibrary::DirectionBetween(Cell, Neighbor, Dir))
		{
			return false; // non adiacenti (o layer diversi): non esiste un lato d'attraversamento
		}

		// 🔴 **La corrispondenza direzione -> indice di lato NON si trascrive**: `DirectionForEdgeIndex` vale
		// `(6 - k) % 6`, quindi l'ordinale di `ERTHexDirection` e l'indice geometrico girano in versi opposti
		// e coincidono solo su `E` e `W`. Chi la riscrive a mano sbaglia su quattro direzioni su sei, e il
		// difetto e' SIMMETRICO: su una mappa simmetrica i test passano lo stesso. E' l'errore che `#1920` ha
		// pagato due volte in due giorni.
		Out = URTHexOcclusionLibrary::AnchorPointQ(ERTAnchorKind::EdgeMid, URTHexLibrary::EdgeIndexForDirection(Dir));
		return true;
	}
}

FRTLocalPointQ URTHexOcclusionLibrary::BoundaryPointQ(int32 BoundaryIndex)
{
	const int32 I = OcclusionWrapBoundary(BoundaryIndex);
	// Da `HexSize / 4` a `HexSize / RT_OcclusionQuanta`: il fattore e' `RT_GeometryQuanta`.
	return FRTLocalPointQ(OcclusionBoundaryR3[I] * RT_GeometryQuanta, OcclusionBoundaryOne[I] * RT_GeometryQuanta);
}

FRTLocalPointQ URTHexOcclusionLibrary::AnchorPointQ(ERTAnchorKind Kind, int32 Index)
{
	if (Kind == ERTAnchorKind::Center)
	{
		return FRTLocalPointQ(0, 0);
	}

	// Stessa corrispondenza di `URTGeometryGrammarLibrary::AnchorLocal`: vertice `k` = confine `2k`, punto
	// medio `k` = confine `2k + 1`.
	const int32 Wrapped = ((Index % 6) + 6) % 6;
	return BoundaryPointQ(Kind == ERTAnchorKind::Vertex ? 2 * Wrapped : 2 * Wrapped + 1);
}

bool URTHexOcclusionLibrary::SegmentEndpointsQ(const FRTGeometrySegment& Segment, FRTLocalPointQ& OutA,
	FRTLocalPointQ& OutB)
{
	if (!URTGeometryGrammarLibrary::IsKnownAxis(Segment.Axis))
	{
		return false;
	}

	const int32 AlongIndex = URTGeometryGrammarLibrary::AxisBoundaryIndex(Segment.Axis);
	// La perpendicolare e' l'asse ruotato di `-90` gradi, cioe' nove confini piu' avanti: la stessa riga di
	// `AxisPerpendicularPoint`, chiesta a chi la possiede invece che riscritta.
	const int32 PerpIndex = AlongIndex + 9;

	// Grezzi in `HexSize / 4`: qui NON si usa `BoundaryPointQ`, perche' `Along` e `Offset` sono gia' in
	// dodicesimi e il prodotto riporta da solo il risultato in `HexSize / RT_OcclusionQuanta`.
	const int64 AlongR3 = OcclusionBoundaryR3[OcclusionWrapBoundary(AlongIndex)];
	const int64 AlongOne = OcclusionBoundaryOne[OcclusionWrapBoundary(AlongIndex)];
	const int64 PerpR3 = OcclusionBoundaryR3[OcclusionWrapBoundary(PerpIndex)];
	const int64 PerpOne = OcclusionBoundaryOne[OcclusionWrapBoundary(PerpIndex)];

	const int64 Offset = static_cast<int64>(Segment.Offset);
	const int64 Start = static_cast<int64>(Segment.AlongStart);
	const int64 End = static_cast<int64>(Segment.AlongEnd);

	OutA = FRTLocalPointQ(PerpR3 * Offset + AlongR3 * Start, PerpOne * Offset + AlongOne * Start);
	OutB = FRTLocalPointQ(PerpR3 * Offset + AlongR3 * End, PerpOne * Offset + AlongOne * End);
	return true;
}

bool URTHexOcclusionLibrary::SegmentsCrossProperly(const FRTLocalPointQ& P1, const FRTLocalPointQ& P2,
	const FRTLocalPointQ& Q1, const FRTLocalPointQ& Q2)
{
	if (P1 == P2 || Q1 == Q2)
	{
		return false; // un punto non attraversa niente
	}

	const int64 D1 = OcclusionCross(P1, P2, Q1);
	const int64 D2 = OcclusionCross(P1, P2, Q2);
	const int64 D3 = OcclusionCross(Q1, Q2, P1);
	const int64 D4 = OcclusionCross(Q1, Q2, P2);

	// Incrocio PROPRIO: ciascun segmento ha i due estremi dell'altro da parti opposte, con segno STRETTO.
	// Uno zero significa «un estremo cade sulla retta dell'altro» — tangenza o collinearita', che per scelta
	// dichiarata non bloccano.
	return OcclusionStraddles(D1, D2) && OcclusionStraddles(D3, D4);
}

bool URTHexOcclusionLibrary::ChordThroughCell(const FRTCellId& Prev, const FRTCellId& Cell,
	const FRTCellId& Next, FRTLocalPointQ& OutA, FRTLocalPointQ& OutB)
{
	if (!OcclusionChordEnd(Prev, Cell, OutA) || !OcclusionChordEnd(Next, Cell, OutB))
	{
		return false;
	}
	if (OutA == OutB)
	{
		return false; // corda degenere: la linea entra ed esce dallo stesso punto, o non attraversa nulla
	}
	return true;
}

bool URTHexOcclusionLibrary::BlocksSight(const URTHexMapAsset* Map, const FRTCellId& Prev,
	const FRTCellId& Cell, const FRTCellId& Next)
{
	if (Map == nullptr)
	{
		return false; // nessun dato di mappa: nessun ostacolo noto, come in `DescribeLineOfSight`
	}

	FRTLocalPointQ ChordA;
	FRTLocalPointQ ChordB;
	if (!ChordThroughCell(Prev, Cell, Next, ChordA, ChordB))
	{
		return false;
	}

	// Scansione lineare NELL'ORDINE DELL'ARRAY: nessuna `TMap` attraversa questa funzione, e l'esito e' un
	// booleano, quindi il risultato non dipende dall'ordine ne' potrebbe.
	for (const FRTHexInteriorWall& Wall : Map->InteriorWalls)
	{
		// `Wall.Cell` porta anche il LAYER, ed e' il layer autorevole del muro: e' quello che
		// `ARTHexMapActor` filtra per decidere su quale piano disegnarlo. La linea ragiona sul layer del
		// tiratore (`HexLine` lo propaga a ogni cella), quindi qui il confronto e' gia' completo.
		if (!(Wall.Cell == Cell))
		{
			continue;
		}

		// `D-271`: `Low` e' copertura direzionale parziale, `High` e' occlusione piena. E' la stessa
		// asimmetria del bordo, dove `BlocksTraversal` nega l'attraversamento all'alta e non al muretto.
		if (Wall.Segment.WallType != ERTHexCoverType::High)
		{
			continue;
		}

		// Un segmento fuori grammatica e' dato incoerente, e chi lo rifiuta e' `ValidateMap`: qui viene
		// ignorato invece che interpretato — non e' compito della LoS riparare l'asset.
		if (URTGeometryGrammarLibrary::ValidateSegment(Wall.Segment) != ERTGeometryViolation::None)
		{
			continue;
		}

		FRTLocalPointQ WallA;
		FRTLocalPointQ WallB;
		if (!SegmentEndpointsQ(Wall.Segment, WallA, WallB))
		{
			continue;
		}

		if (SegmentsCrossProperly(ChordA, ChordB, WallA, WallB))
		{
			return true;
		}
	}

	return false;
}

FVector2D URTHexOcclusionLibrary::ToLocal(const FRTLocalPointQ& Point, float HexSize)
{
	const double Scale = static_cast<double>(HexSize) / static_cast<double>(RT_OcclusionQuanta);
	const double Root3 = FMath::Sqrt(3.0);
	return FVector2D(static_cast<double>(Point.R3) * Root3 * Scale, static_cast<double>(Point.One) * Scale);
}
