#pragma once

#include "CoreMinimal.h"
#include "Map/RTHexOccupancyLibrary.h"

/**
 * LE QUATTRO FIXTURE DI GEOMETRIA di §22 del sorgente Map Sketch Editor — segmento solido, angolo,
 * footprint solido, footprint void.
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
}
