#include "Combat/RTRangeVerdict.h"

#include "Combat/RTCombatLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTHexSimLibrary.h"

URTHexMapAsset* URTRangeVerdictLibrary::MakeVisibleOnlyNeutralMap(UObject* Outer, const URTHexMapAsset* Map,
	const FRTTeamKnowledge& Knowledge)
{
	// Una conoscenza con la SOLA `VisibleCells`: `MakeOptimisticPlanningMap` neutralizza le celle fuori dall'unione di
	// visibili ed esplorate, quindi con `ExploredCells` vuoto neutralizza tutto cio' che non si vede adesso.
	FRTTeamKnowledge VisibleOnly;
	VisibleOnly.TeamId = Knowledge.TeamId;
	VisibleOnly.VisibleCells = Knowledge.VisibleCells;
	return URTHexSimLibrary::MakeOptimisticPlanningMap(Outer, Map, VisibleOnly);
}

TArray<FRTRangeCellVerdict> URTRangeVerdictLibrary::Classify(const URTHexMapAsset* NeutralMap,
	const TArray<FRTCellId>& VisibleCells, const FRTCellId& From, int32 RangeCells, ERTLineOfSightPolicy Policy,
	int32 MinRangeCells)
{
	TArray<FRTRangeCellVerdict> Out;
	// Un insieme per la ricerca: ogni cella in portata percorre la sua linea, e `VisibleCells` e' un array.
	const TSet<FRTCellId> Seen(VisibleCells);
	if (NeutralMap == nullptr || !Seen.Contains(From))
	{
		return Out;
	}

	// L'INSIEME e' quello dell'azione armata (`TargetableRangeCells`), ma calcolato sulla mappa neutra: nessun effetto di
	// una cella non in vista (Fumo, blocchi) puo' cambiarne la forma.
	for (const FRTCellId& Cell : URTCombatLibrary::TargetableRangeCells(NeutralMap, From, RangeCells, Policy, MinRangeCells))
	{
		FRTRangeCellVerdict Entry;
		Entry.Cell = Cell;
		Entry.Verdict = ERTRangeVerdict::Unverdicted;

		// Il verdetto si consulta solo se OGNI cella della linea e' in vista (compresi partenza e bersaglio).
		bool bWholeLineSeen = true;
		for (const FRTCellId& OnLine : URTHexLibrary::HexLine(From, Cell))
		{
			if (!Seen.Contains(OnLine))
			{
				bWholeLineSeen = false;
				break;
			}
		}

		if (bWholeLineSeen)
		{
			// Su una linea tutta in vista la mappa neutra coincide con quella vera: il verdetto non legge niente di nascosto.
			switch (URTCombatLibrary::ClassifyHexTargeting(NeutralMap, From, Cell, RangeCells, Policy, MinRangeCells))
			{
			case ERTHexTargetReason::Ok:
				Entry.Verdict = ERTRangeVerdict::Free;
				break;
			case ERTHexTargetReason::NoLineOfSight:
				Entry.Verdict = ERTRangeVerdict::Blocked;
				break;
			default:
				// Una cella dell'insieme non dovrebbe avere un'altra ragione. Se capita, tace invece di inventare un verdetto.
				break;
			}
		}
		Out.Add(Entry);
	}
	return Out;
}

TArray<FRTRangeCellVerdict> URTRangeVerdictLibrary::ClassifyFor(UObject* Outer, const URTHexMapAsset* Map,
	const FRTTeamKnowledge& Knowledge, const FRTCellId& From, int32 RangeCells, ERTLineOfSightPolicy Policy,
	int32 MinRangeCells)
{
	const URTHexMapAsset* Neutral = MakeVisibleOnlyNeutralMap(Outer, Map, Knowledge);
	return Classify(Neutral, Knowledge.VisibleCells, From, RangeCells, Policy, MinRangeCells);
}
