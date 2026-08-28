// L'arena di uno scenario, costruita dal dato. Vedi `RTScenarioArena.h` per il perche' vive qui e non nella
// sessione.
//
// Estratta da `BuildScenarioArena` (`RTScenarioSession.cpp`) con `#1116`: la sessione continua a chiamarla e
// ci aggiunge la sua meta' — l'`ARTHexMapActor` che rende la mappa visibile in PIE — che e' presentazione e
// non appartiene a chi calcola.

#include "ScenarioHarness/RTScenarioArena.h"

#include "Map/RTHexMapAsset.h"
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
	Map->SortCells();

	return Map;
}
