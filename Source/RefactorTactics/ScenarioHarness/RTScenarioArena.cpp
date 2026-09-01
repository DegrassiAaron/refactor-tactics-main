// L'arena di uno scenario, costruita dal dato. Vedi `RTScenarioArena.h` per il perche' vive qui e non nella
// sessione.
//
// Estratta da `BuildScenarioArena` (`RTScenarioSession.cpp`) con `#1116`: la sessione continua a chiamarla e
// ci aggiunge la sua meta' — l'`ARTHexMapActor` che rende la mappa visibile in PIE — che e' presentazione e
// non appartiene a chi calcola.

#include "ScenarioHarness/RTScenarioArena.h"

#include "Map/RTHexMapAsset.h"
#include "Map/RTGeometryBake.h"
#include "Map/RTGeometryGrammar.h"
#include "ScenarioHarness/RTTestScenario.h"
#include "Turn/RTMatchSetupLibrary.h"

URTHexMapAsset* URTScenarioArenaLibrary::BuildArena(const FRTTestScenario& Scenario, UObject* Outer)
{
	URTHexMapAsset* Map = nullptr;

	if (!Scenario.Fixture.IsEmpty())
	{
		// Nome sconosciuto -> nullptr, mai un'arena vuota: quella farebbe girare la partita e produrrebbe un
		// fallimento che parla di unita' fuori mappa invece che della fixture inesistente.
		Map = URTMatchSetupLibrary::MakeFixtureArena(Outer, Scenario.Fixture);
		if (!Map)
		{
			return nullptr;
		}
	}
	else
	{
		Map = URTMatchSetupLibrary::MakeFlatArena(Outer, Scenario.MapRadius);
	}

	// `MakeFlatArena` torna nullptr per un raggio negativo, dove `NewObject` dava sempre un asset: senza
	// questa guardia le righe qui sotto lo dereferenziano.
	if (!Map)
	{
		return nullptr;
	}

	// Le modifiche DOPO l'arena piena: una cella elencata due volte vince l'ultima, e l'esito non dipende
	// dall'ordine in cui la fixture le aveva generate.
	for (const FRTScenarioCell& Spec : Scenario.Cells)
	{
		FRTHexCellData Cell(Spec.Cell);
		Cell.bBlocksMovement = Spec.bBlocksMovement;
		Cell.bBlocksLineOfSight = Spec.bBlocksLineOfSight;
		if (Spec.MoveCost > 0)
		{
			Cell.MoveCost = Spec.MoveCost;
		}
		Cell.OccupancySurcharge = Spec.OccupancySurcharge; // 0 = cella larga, come una non elencata
		Map->AddOrUpdateCell(Cell);
	}

	// I MURI INTERNI, cotti DOPO che ogni cella e' al suo stato finale — `#2031`.
	//
	// 🔑 **Si chiama la cottura VERA, e non e' un dettaglio di riuso.** `BakeCell` produce i muri, le
	// coperture di bordo che il segmento chiude, e deriva `bBlocksMovement` dall'assenza di posa legale
	// (`E23.6`). Uno scenario che scrivesse l'esito a mano proverebbe se stesso: qui prova la catena —
	// segmento -> maschera -> posa -> passo — che e' cio' per cui questi scenari esistono.
	//
	// ⚠️ In un ciclo suo perche' `BakeCell` chiede che la cella esista gia': il primo ciclo la crea.
	for (const FRTScenarioCell& Spec : Scenario.Cells)
	{
		if (Spec.InteriorWalls.Num() == 0)
		{
			continue;
		}

		TArray<FRTGeometrySegment> Segments;
		Segments.Reserve(Spec.InteriorWalls.Num());
		for (const FRTScenarioInteriorWall& Wall : Spec.InteriorWalls)
		{
			FRTGeometrySegment Segment;
			Segment.Axis = static_cast<ERTTacticalAxis>(Wall.Axis);
			Segment.Offset = Wall.Offset;
			Segment.AlongStart = Wall.AlongStart;
			Segment.AlongEnd = Wall.AlongEnd;
			Segment.Layer = Spec.Cell.Layer;
			// Vuoto = `High`, che e' il default del campo: un muretto si chiede per nome.
			Segment.WallType = Wall.WallType.Equals(TEXT("Low"), ESearchCase::IgnoreCase)
				? ERTHexCoverType::Low
				: ERTHexCoverType::High;
			Segments.Add(Segment);
		}

		URTGeometryBakeLibrary::BakeCell(Map, Spec.Cell, Segments, Map->HexSize);

		// ⚠️ **`blocksMovement` d'autore vince sul derivato**, e va riapplicato DOPO la cottura: il bake
		// toglie il proprio blocco quando una posa esiste, e senza questa riga cancellerebbe la scelta
		// dello scenario. E' la stessa disciplina di `D-131`, dal lato di chi allestisce.
		if (Spec.bBlocksMovement)
		{
			if (const FRTHexCellData* Baked = Map->FindCell(Spec.Cell))
			{
				FRTHexCellData Authored = *Baked;
				Authored.bBlocksMovement = true;
				Authored.bMovementBlockGenerated = false;
				Map->AddOrUpdateCell(Authored);
			}
		}
	}

	Map->SortCells();

	return Map;
}
