#include "Turn/RTMoveRoute.h"

TArray<FRTCellId> URTMoveRouteLibrary::VisibleTrailFor(const FRTMoveRoute& Route, int32 ObserverTeamId)
{
	// 🔴 **La regola vive in `ObservedPrefixLength`, non qui** (`#1525`). Il troncamento e il suo
	// fail-closed erano scritti in questa funzione, e finche' ci sono stati il playback — l'altro
	// consumatore della stessa rotta — non poteva applicarli senza copiarli. Due copie sarebbero
	// divergute, e la contraddizione che [D-223] nomina (traccia troncata mentre il modello prosegue)
	// sarebbe tornata dalla porta di servizio.
	const int32 Visible = URTTeamKnowledgeLibrary::ObservedPrefixLength(
		Route.Cells, Route.CellVerdicts, ObserverTeamId);

	TArray<FRTCellId> Trail;
	Trail.Reserve(Visible);
	for (int32 i = 0; i < Visible; ++i)
	{
		Trail.Add(Route.Cells[i]);
	}
	return Trail;
}
