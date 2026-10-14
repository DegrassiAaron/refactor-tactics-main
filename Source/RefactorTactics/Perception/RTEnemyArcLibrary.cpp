#include "Perception/RTEnemyArcLibrary.h"

#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"

URTHexMapAsset* URTEnemyArcLibrary::KnownTerrainMap(UObject* Outer, const URTHexMapAsset* Map,
	const TArray<FRTCellId>& KnownCells)
{
	if (!Map)
	{
		return nullptr;
	}

	URTHexMapAsset* Copia = DuplicateObject<URTHexMapAsset>(Map, Outer ? Outer : GetTransientPackage());
	if (!Copia)
	{
		return nullptr;
	}

	const TSet<FRTCellId> Note(KnownCells);
	const auto LatoNoto = [&Note](const FRTCellId& Cella, ERTHexDirection Lato)
	{
		return Note.Contains(URTHexLibrary::Neighbor(Cella, Lato));
	};

	// ⚠️ Si scrive sulle celle IN LOCO e non con `AddOrUpdateCell`: l'`Id` non cambia, quindi l'indice per cella
	// resta valido, e la copia non ha un registro delle modifiche da tenere — nessuno la ridisegna.
	for (FRTHexCellData& Cella : Copia->Cells)
	{
		if (Note.Contains(Cella.Id))
		{
			continue;
		}
		Cella.bBlocksLineOfSight = false;
		// La superficie di una cella mai vista non si sa: il fumo, e ogni superficie che il catalogo dei terreni dice
		// accorciare la vista (`URTTerrainLibrary::EffectiveTargetingRange`), tornano pavimento.
		Cella.Surface = ERTHexSurface::Floor;
		Cella.Covers.RemoveAll([&](const FRTHexCover& C) { return !LatoNoto(Cella.Id, C.Edge); });
		Cella.Doors.RemoveAll([&](const FRTHexDoor& D) { return !LatoNoto(Cella.Id, D.Edge); });
		Cella.Guards.RemoveAll([&](const FRTHexEdgeGuard& G) { return !LatoNoto(Cella.Id, G.Edge); });
	}

	Copia->InteriorWalls.RemoveAll([&Note](const FRTHexInteriorWall& W) { return !Note.Contains(W.Cell); });
	return Copia;
}

TArray<FRTCellId> URTEnemyArcLibrary::SightOnKnownTerrain(const URTHexMapAsset* KnownMap, const FRTPerceiver& Enemy,
	const TArray<FRTCellId>& KnownCells)
{
	// La cella del nemico resta anche se fosse fuori da `KnownCells`: lo si vede, quindi la si conosce. `VisibleCells`
	// la mette sempre per prima regola, e qui non si toglie.
	const TSet<FRTCellId> Note(KnownCells);
	TArray<FRTCellId> Out = URTPerceptionLibrary::VisibleCells(KnownMap, Enemy);
	Out.RemoveAll([&](const FRTCellId& C) { return !(C == Enemy.Cell) && !Note.Contains(C); });
	return Out;
}
