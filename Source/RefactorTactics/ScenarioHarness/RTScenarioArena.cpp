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

	// La geometria INTERNA alle celle (`D-269`, `#1830`): ferma vista e proiettili, quindi entra nell'arena
	// come dato di gioco e nell'hash della mappa.
	//
	// ⚠️ Si AGGIUNGE, non si assegna: un'arena generata non ne ha nessuno — `MakeFlatArena` non ne produce —
	// ma uno scenario che riparte da un asset esistente ne troverebbe i suoi, e sostituirli in blocco li
	// cancellerebbe in silenzio. Le celle qui sopra si sovrascrivono invece per id, che una lista di segmenti
	// non ha.
	Map->InteriorWalls.Append(Scenario.InteriorWalls);

	// LA COTTURA di quei muri, che e' cio' che li rende visibili al MOVIMENTO e non solo alla vista — `#2031`.
	//
	// 🔑 `#1830` porta il muro dentro l'arena e gli fa fermare vista e proiettile. `BakeCell` gli fa
	// produrre le coperture di bordo che chiude e DERIVA `bBlocksMovement` dall'assenza di una posa legale
	// (`E23.6`). Senza, una cella tagliata in due resterebbe pavimento: opaca allo sguardo e attraversabile
	// a piedi. La derivazione la fa la cottura vera, non una copia qui: uno scenario che scrivesse l'esito a
	// mano proverebbe se stesso.
	//
	// ⚠️ Dopo il ciclo delle celle, perche' `BakeCell` chiede che la cella esista gia'.
	{
		// 🔴 L'ordine e' quello di PRIMA APPARIZIONE, non quello di una `TMap`: l'iterazione di una
		// mappa non e' garantita, e da qui escono `AddOrUpdateCell` che toccano l'asset.
		TArray<FRTCellId> Order;
		TMap<FRTCellId, TArray<FRTGeometrySegment>> PerCell;
		for (const FRTHexInteriorWall& Wall : Scenario.InteriorWalls)
		{
			if (!PerCell.Contains(Wall.Cell))
			{
				Order.Add(Wall.Cell);
			}
			PerCell.FindOrAdd(Wall.Cell).Add(Wall.Segment);
		}

		for (const FRTCellId& Cooked : Order)
		{
			URTGeometryBakeLibrary::BakeCell(Map, Cooked, PerCell[Cooked], Map->HexSize);

			// ⚠️ **`blocksMovement` d'autore vince sul derivato**, e va riapplicato DOPO la cottura:
			// il bake toglie il PROPRIO blocco quando una posa esiste, e senza questa riga cancellerebbe la
			// scelta di chi ha scritto lo scenario. E' la disciplina di provenienza di `D-131`, dal lato di
			// chi allestisce.
			const FRTScenarioCell* Authored = Scenario.Cells.FindByPredicate(
				[&Cooked](const FRTScenarioCell& C) { return C.Cell == Cooked && C.bBlocksMovement; });
			if (Authored)
			{
				if (const FRTHexCellData* Baked = Map->FindCell(Cooked))
				{
					FRTHexCellData Restored = *Baked;
					Restored.bBlocksMovement = true;
					Restored.bMovementBlockGenerated = false;
					Map->AddOrUpdateCell(Restored);
				}
			}
		}
	}

	Map->SortCells();

	return Map;
}
