#include "Player/RTFocusArea.h"

#include "Ability/RTMovementProfileLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Unit/RTUnit.h"

namespace
{
	/** Nomi distinti per file: il progetto usa unity build. */
	TArray<FRTCellId> FocusReachableCells(const URTHexMapAsset* Map, const FRTFocusMoveSubject& Subject, int32 Budget)
	{
		// Uno snapshot con la SOLA unita' del soggetto: nessun'altra unita' puo' incidere sul ventaglio, e `ReachableCells`
		// legge soltanto la mappa, la cella, il budget e il costo in piu' per cella.
		FRTHexSnapshot Snapshot;
		Snapshot.Map = Map;
		FRTHexSimUnit Unit(/*InUnitId=*/ 0, Subject.Cell, Budget, /*bInAlive=*/ true);
		Unit.MoveCostModifier = Subject.MoveCostModifier;
		Snapshot.Units.Add(Unit);

		TArray<FRTCellId> Cells;
		for (const FRTHexReachableCell& Reachable : URTHexSimLibrary::ReachableCells(Snapshot, /*UnitId=*/ 0))
		{
			Cells.Add(Reachable.Cell);
		}
		// Nessun ordinamento qui: `ReachableCells` ordina gia' la propria uscita con `StableLess` (l'ultima istruzione prima
		// del `return`). I test confrontano queste liste con un oracolo ordinato nello stesso modo, quindi se la libreria
		// smettesse di ordinare cadrebbero, e due chiamate con gli stessi ingressi darebbero comunque le stesse liste.
		return Cells;
	}
}

FRTFocusMoveSubject URTFocusAreaLibrary::SubjectFor(const ARTUnit* Unit)
{
	FRTFocusMoveSubject Subject;
	if (Unit == nullptr)
	{
		return Subject;
	}
	// Cella, movimento base, `Slow`, `Unbalanced`: letture pubbliche. Il piano (`Planned*`) non si legge: e' esattamente
	// cio' che D-552 vieta.
	Subject.Cell = Unit->Cell;
	Subject.BaseMoveRange = Unit->GetEffectiveMoveRange();
	Subject.MoveCostModifier = Unit->HasStatus(TAG_Status_Slow) ? 1 : 0;
	Subject.bRunDenied = Unit->HasStatus(TAG_Status_Unbalanced);
	return Subject;
}

URTHexMapAsset* URTFocusAreaLibrary::MakeVisibleOnlyMap(UObject* Outer, const URTHexMapAsset* Map,
	const FRTTeamKnowledge& Knowledge)
{
	if (Map == nullptr)
	{
		return nullptr;
	}
	URTHexMapAsset* Copy = DuplicateObject<URTHexMapAsset>(Map, Outer != nullptr ? Outer : GetTransientPackage());
	if (Copy == nullptr)
	{
		return nullptr;
	}
	// In loco e non con `AddOrUpdateCell`: l'`Id` non cambia, l'indice per cella resta valido (stessa scelta di
	// `URTHexSimLibrary::MakeOptimisticPlanningMap`). ⚠️ Solo `VisibleCells`: unire `ExploredCells` renderebbe
	// attraversabile cio' che la squadra non puo' leggere adesso.
	for (FRTHexCellData& Cell : Copy->Cells)
	{
		if (!Knowledge.VisibleCells.Contains(Cell.Id))
		{
			Cell.bBlocksMovement = true;
		}
	}
	return Copy;
}

FRTFocusMoveAreas URTFocusAreaLibrary::MoveAreas(const URTHexMapAsset* VisibleOnlyMap, const FRTFocusMoveSubject& Subject)
{
	FRTFocusMoveAreas Areas;
	Areas.bRunDenied = Subject.bRunDenied;
	if (VisibleOnlyMap == nullptr || !VisibleOnlyMap->ContainsCell(Subject.Cell))
	{
		return Areas;
	}

	const int32 Base = FMath::Max(0, Subject.BaseMoveRange);

	// Le due bande FISSE, dal movimento base e non dal piano: `Move` e' `1x`, `Sprint` e' `2x` (D-412).
	const FRTMovementProfile Normal = URTMovementProfileLibrary::FindProfile(URTMovementProfileLibrary::ProfileMove);
	Areas.Normal = FocusReachableCells(VisibleOnlyMap, Subject, Normal.ResolveMoveBudget(Base));

	if (!Subject.bRunDenied)
	{
		// Il tetto NUDO, senza profilo dichiarato ne' riserva: sono i campi del piano, e per un nemico sono privati.
		// Con la corsa negata scenderebbe a `Move` ([D-319]), ed e' il ramo qui sopra.
		const FRTMovementProfile Ceiling = URTMovementProfileLibrary::CeilingProfile(NAME_None, NAME_None, /*bRunDenied=*/ false);
		Areas.Run = FocusReachableCells(VisibleOnlyMap, Subject, Ceiling.ResolveMoveBudget(Base));
	}
	return Areas;
}

FRTFocusMoveAreas URTFocusAreaLibrary::MoveAreasFor(UObject* Outer, const URTHexMapAsset* Map,
	const FRTTeamKnowledge& Knowledge, const FRTFocusMoveSubject& Subject)
{
	FRTFocusMoveAreas None;
	None.bRunDenied = Subject.bRunDenied;
	// Un soggetto che la squadra non vede adesso non ha area. La verifica sta qui e non nella mappa ristretta: li' la
	// cella del soggetto risulterebbe un muro, e un'uscita vuota per un motivo implicito sarebbe un'uscita fragile.
	if (Map == nullptr || !Knowledge.VisibleCells.Contains(Subject.Cell))
	{
		return None;
	}
	return MoveAreas(MakeVisibleOnlyMap(Outer, Map, Knowledge), Subject);
}
