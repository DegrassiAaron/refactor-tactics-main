#pragma once

#include "CoreMinimal.h"
#include "Map/RTHexOccupancyLibrary.h"

/**
 * LE FIXTURE DI GEOMETRIA. Le prime quattro vengono da §22 del sorgente Map Sketch Editor — segmento
 * solido, angolo, footprint solido, footprint void.
 *
 * La quinta e la sesta (piu' i costruttori di muri perimetrali) sono state aggiunte dalla revisione
 * spec panel del 2026-08-12, che ha rilevato come gli assi tattici di **#620** coincidano con i confini
 * dei dodici settori: il caso collineare, che le prime quattro evitano di proposito, sta per diventare
 * l'unico caso in uso. Vedi il blocco in fondo.
 *
 * ⚠️ **Non sono scenari, e non stanno in `Scenarios/`.** `FRTScenarioCell` porta `Cell`,
 * `bBlocksMovement`, `bBlocksLineOfSight`, `MoveCost` e `OccupancySurcharge`: nessun campo per segmenti o
 * footprint. Uno scenario e' una PARTITA; queste sono l'INGRESSO di una funzione pura, e il posto di un
 * ingresso lo decide cio' che il formato sa esprimere (`scenario-map.md` §2).
 *
 * Vivono qui, e non dentro un singolo file di test, perche' **#620** (grammatica delle direttrici) e
 * **#621** (cottura verso i bordi) hanno bisogno delle stesse quattro geometrie: se ognuno se le riscrivesse,
 * «la stessa fixture» smetterebbe di essere la stessa senza che nessuno se ne accorga.
 *
 * Le coordinate sono LOCALI di cella — origine nel centro — e usano `RT_OccupancyFixtureHexSize` come raggio.
 * Gli angoli citati nei commenti sono quelli del settore che la geometria deve occupare, con il settore 0
 * ancorato al primo vertice del pointy-top (`-30` gradi).
 */
namespace RTOccupancyFixtures
{
	/** Raggio dell'esagono su cui sono costruite tutte e quattro. */
	constexpr float HexSize = 100.f;

	/** Punto in coordinate locali, dato un angolo in gradi e una frazione del raggio. */
	inline FVector2D PointAt(double AngleDegrees, double RadiusFraction)
	{
		const double Radians = FMath::DegreesToRadians(AngleDegrees);
		const double R = HexSize * RadiusFraction;
		return FVector2D(R * FMath::Cos(Radians), R * FMath::Sin(Radians));
	}

	inline FRTOccupancyPolyline OpenLine(const TArray<FVector2D>& Points)
	{
		FRTOccupancyPolyline Line;
		Line.Points = Points;
		Line.bClosed = false;
		return Line;
	}

	/** Quadrato CHIUSO centrato su `Centre`, mezzo-lato `Half`. */
	inline FRTOccupancyPolyline ClosedSquare(const FVector2D& Centre, double Half)
	{
		FRTOccupancyPolyline Line;
		Line.Points = {
			FVector2D(Centre.X - Half, Centre.Y - Half),
			FVector2D(Centre.X + Half, Centre.Y - Half),
			FVector2D(Centre.X + Half, Centre.Y + Half),
			FVector2D(Centre.X - Half, Centre.Y + Half)
		};
		Line.bClosed = true;
		return Line;
	}

	/**
	 * **Fixture 1 — segmento solido.** Un muro corto interamente dentro il settore 0 (angoli `[-30, 0)`).
	 * Entrambi gli estremi stanno nel settore con margine: non tocca i confini, quindi l'esito non dipende
	 * da come si trattano i casi collineari.
	 */
	inline TArray<FRTOccupancyPolyline> SingleSolidSegment()
	{
		return { OpenLine({ PointAt(-20.0, 0.3), PointAt(-10.0, 0.6) }) };
	}

	/**
	 * **Fixture 2 — angolo.** Tre punti a raggio costante che spazzano i settori 0, 1 e 2. Serve a dire che
	 * la maschera segue il PERCORSO e non solo gli estremi: un'implementazione che guardasse i soli vertici
	 * accenderebbe gli stessi bit e passerebbe, ma sbaglierebbe su un segmento lungo che attraversa.
	 */
	inline TArray<FRTOccupancyPolyline> Corner()
	{
		return { OpenLine({ PointAt(-20.0, 0.5), PointAt(10.0, 0.5), PointAt(40.0, 0.5) }) };
	}

	/**
	 * **Fixture 3 — footprint solido.** Un contorno chiuso attorno al CENTRO: `bCoreBlocked` e tutti e dodici
	 * i settori occupati, perche' un anello attorno all'origine attraversa ogni settore.
	 */
	inline TArray<FRTOccupancyPolyline> SolidFootprint()
	{
		return { ClosedSquare(FVector2D::ZeroVector, 20.0) };
	}

	/**
	 * **Fixture 4 — footprint void.** Un contorno chiuso che NON contiene il centro: occupa i suoi settori e
	 * lascia il core libero. E' il gemello di controllo del solido — senza, «il centro e' bloccato»
	 * passerebbe anche con un'implementazione che lo mette sempre a vero.
	 */
	inline TArray<FRTOccupancyPolyline> VoidFootprint()
	{
		return { ClosedSquare(PointAt(-15.0, 0.6), 5.0) };
	}

	// ---------------------------------------------------------------------------------------------
	// QUINTA E SESTA FIXTURE — il caso che le prime quattro evitano di proposito.
	//
	// Le quattro sopra usano -20, -10, 10, 40, -15 gradi: nessuna tocca un multiplo di 30, e il
	// commento della prima lo dichiara apertamente. Era la scelta giusta allora, perche' isolava la
	// misura dalla regola conservativa sui casi collineari.
	//
	// Ma i confini dei dodici settori stanno a `-30 + 30k` gradi, e gli assi tattici che #620 vuole
	// imporre sono 0/30/60/90/120/150: LO STESSO INSIEME. Ogni segmento canonico di #620 nascera'
	// quindi sopra un confine — cioe' nel caso che nessuna fixture copre e nessun test protegge.
	// ---------------------------------------------------------------------------------------------

	/**
	 * **Fixture 5 — segmento SUL confine di settore.** Radiale sull'asse a 0 gradi, che e' il confine
	 * condiviso fra il settore 0 (`[-30, 0)`) e il settore 1 (`[0, 30)`).
	 *
	 * Raggio `0.2`–`0.6`: NON parte dal centro, di proposito. Un segmento che passa per il centro fa
	 * scattare un ramo diverso (`Cross2D(A, B, Centro) == 0` con il centro dentro i bounds), e qui
	 * interessa isolare il solo contatto radiale.
	 *
	 * Atteso: due settori — `0` e `1`. E' la regola conservativa di `SegmentsIntersect`,
	 * *«un muro appoggiato esattamente al confine fra due settori li invade entrambi»*.
	 */
	inline TArray<FRTOccupancyPolyline> SegmentOnSectorBoundary()
	{
		return { OpenLine({ PointAt(0.0, 0.2), PointAt(0.0, 0.6) }) };
	}

	/**
	 * **Fixture 6 — il gemello di controllo.** Lo STESSO segmento ruotato di 15 gradi: stessa lunghezza,
	 * stesso raggio, ma interamente dentro il settore 1 invece che sul suo bordo.
	 *
	 * Senza questo gemello, «due settori» passerebbe anche con un'implementazione che ne accende sempre
	 * due. E' la coppia a dire qualcosa, non la fixture 5 da sola.
	 */
	inline TArray<FRTOccupancyPolyline> SegmentJustOffSectorBoundary()
	{
		return { OpenLine({ PointAt(15.0, 0.2), PointAt(15.0, 0.6) }) };
	}

	/**
	 * Muro su un LATO dell'esagono: dal vertice `EdgeIndex` al successivo, entrambi a raggio pieno.
	 * E' la terza forma ammessa dalla grammatica di #620 — «segmenti sui lati/perimetro» — ed e' la
	 * geometria che un designer disegna per murare una stanza.
	 *
	 * Pointy-top: il vertice `k` sta a `-30 + 60k` gradi, la stessa convenzione di
	 * `URTHexLibrary::HexCorners`.
	 */
	inline FRTOccupancyPolyline WallOnHexEdge(int32 EdgeIndex)
	{
		return OpenLine({
			PointAt(-30.0 + 60.0 * EdgeIndex, 1.0),
			PointAt(-30.0 + 60.0 * (EdgeIndex + 1), 1.0)
		});
	}

	/**
	 * Lo STESSO muro perimetrale, ma **rientrato** di `Inset` a entrambi gli estremi: stessa giacitura,
	 * stesso lato, e non tocca piu' i due vertici dell'esagono.
	 *
	 * E' il gemello di controllo di `WallOnHexEdge`, e serve a isolare *da cosa* nasce l'occupazione. Un
	 * vertice dell'esagono e' il punto in comune fra QUATTRO triangoli di settore: un estremo che ci cade
	 * sopra accende anche i settori che la geometria non invade per area, ma solo tocca in un punto.
	 * La differenza fra questa fixture e quella intera misura esattamente quel contributo puntuale.
	 */
	inline FRTOccupancyPolyline WallOnHexEdgeInset(int32 EdgeIndex, double Inset = 0.05)
	{
		const FVector2D V0 = PointAt(-30.0 + 60.0 * EdgeIndex, 1.0);
		const FVector2D V1 = PointAt(-30.0 + 60.0 * (EdgeIndex + 1), 1.0);
		return OpenLine({
			FMath::Lerp(V0, V1, Inset),
			FMath::Lerp(V0, V1, 1.0 - Inset)
		});
	}

	/** `Count` muri su lati CONSECUTIVI, a partire dal lato 0. L'angolo di una stanza. */
	inline TArray<FRTOccupancyPolyline> WallsOnConsecutiveEdges(int32 Count)
	{
		TArray<FRTOccupancyPolyline> Walls;
		for (int32 Edge = 0; Edge < Count; ++Edge)
		{
			Walls.Add(WallOnHexEdge(Edge));
		}
		return Walls;
	}
}
