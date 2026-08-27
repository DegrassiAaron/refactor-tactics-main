#include "Perception/RTKnowledgeView.h"

FRTKnowledgeView URTKnowledgeViewLibrary::ViewForTeam(const FRTTeamKnowledge& Knowledge,
	const TArray<FRTKnowledgeSubject>& Subjects, int32 ObserverTeamId)
{
	FRTKnowledgeView View;
	View.ObserverTeamId = ObserverTeamId;

	for (const FRTKnowledgeSubject& S : Subjects)
	{
		if (!S.bAlive)
		{
			continue; // un morto non e' un soggetto di conoscenza: lo tratta la presentazione della sconfitta
		}

		FRTKnowledgeEntry E;
		E.StableUnitId = S.StableUnitId;
		E.HeroId = S.HeroId;
		E.HeroDisplayName = S.HeroDisplayName;

		if (S.TeamId == ObserverTeamId)
		{
			// La propria squadra si conosce sempre: non passa da `ClassifyTarget`, che risponde alla domanda
			// «posso bersagliarlo?» e per un alleato non e' la domanda giusta.
			E.Visibility = ERTKnowledgeVisibility::Live;
			E.Cell = S.Cell;
			View.Entries.Add(E);
			continue;
		}

		switch (URTTeamKnowledgeLibrary::ClassifyTarget(Knowledge, S.StableUnitId, S.TeamId, S.Cell))
		{
		case ERTTargetKnowledge::Allowed:
			E.Visibility = ERTKnowledgeVisibility::Live;
			E.Cell = S.Cell;
			View.Entries.Add(E);
			break;

		case ERTTargetKnowledge::CellOnly:
		{
			// 🔴 La cella del RICORDO, mai `S.Cell`. Se il ricordo non si legge, non si inventa: nessuna voce.
			// `FindContact` porta anche il `TurnNumber`, che la sola cella non conterrebbe: e' lo stesso
			// contatto che `LastKnownCell` cercherebbe, non una seconda ricerca.
			if (const FRTLastKnownContact* Contact = URTTeamKnowledgeLibrary::FindContact(Knowledge, S.StableUnitId))
			{
				E.Visibility = ERTKnowledgeVisibility::Remembered;
				E.Cell = Contact->Cell;
				E.ContactTurn = Contact->TurnNumber;
				View.Entries.Add(E);
			}
			break;
		}

		default:
			break; // Rejected: NESSUNA voce. E' il cuore della porta.
		}
	}

	// Ordine canonico per `StableUnitId`: mai quello di scoperta, che dipenderebbe dall'ordine dei soggetti.
	View.Entries.Sort([](const FRTKnowledgeEntry& A, const FRTKnowledgeEntry& B)
		{ return A.StableUnitId < B.StableUnitId; });
	return View;
}

const FRTKnowledgeEntry* URTKnowledgeViewLibrary::FindEntry(const FRTKnowledgeView& View, int32 StableUnitId)
{
	for (const FRTKnowledgeEntry& E : View.Entries)
	{
		if (E.StableUnitId == StableUnitId)
		{
			return &E;
		}
	}
	return nullptr;
}
