#include "Perception/RTTeamKnowledge.h"

#include "Map/RTHexLibrary.h"
#include "Perception/RTKnowledgeView.h"   // FRTKnowledgeSubject: i tre ingressi di `ClassifyTarget`

FRTTeamKnowledge URTTeamKnowledgeLibrary::Observe(const URTHexMapAsset* Map, int32 TeamId, int32 TurnNumber,
	const TArray<FRTPerceiver>& Observers, const TArray<FRTLastKnownContact>& EnemiesNow,
	const FRTTeamKnowledge& Previous)
{
	FRTTeamKnowledge Out;
	Out.Version = FRTTeamKnowledge::CurrentVersion;
	Out.TeamId = TeamId;
	Out.TurnNumber = TurnNumber;
	Out.VisibleCells = URTPerceptionLibrary::TeamVisibleCells(Map, Observers);

	// Ricerca per cella su un TSet: l'appartenenza e' una domanda puntuale e ripetuta una volta per nemico.
	// L'insieme NON viene mai iterato — il suo ordine dipenderebbe dall'hash, e l'ordine dell'output lo
	// decide il Sort finale su `StableUnitId`.
	const TSet<FRTCellId> Visible(Out.VisibleCells);

	// 1. Chi si vede ORA: contatto fresco. Sovrascrive qualunque ricordo, perche' il ricordo e' cio' che
	//    resta quando la vista non c'e' — averli entrambi significherebbe due verita' sulla stessa unita'.
	TSet<int32> SeenNow;
	for (const FRTLastKnownContact& Enemy : EnemiesNow)
	{
		if (Enemy.StableUnitId == INDEX_NONE || !Visible.Contains(Enemy.Cell))
		{
			continue;
		}
		SeenNow.Add(Enemy.StableUnitId);
		Out.Contacts.Add(FRTLastKnownContact(Enemy.StableUnitId, Enemy.Cell, TurnNumber));
	}

	// 2. Chi non si vede piu': si conserva il RICORDO, con la sua cella e il suo turno originali. Non si
	//    aggiorna la cella — un ricordo che seguisse l'unita' sarebbe la vista sotto mentite spoglie, ed e'
	//    esattamente l'informazione che l'invariante di privacy non concede.
	//
	// ⚠️ Una `Previous` di versione ignota si scarta. Reinterpretarla darebbe una memoria plausibile e
	// sbagliata, che e' il difetto piu' difficile da vedere: nessuno la segnala, e le decisioni del bot ne
	// dipendono.
	if (Previous.Version == FRTTeamKnowledge::CurrentVersion)
	{
		for (const FRTLastKnownContact& Old : Previous.Contacts)
		{
			if (SeenNow.Contains(Old.StableUnitId))
			{
				continue; // gia' rinfrescato al punto 1
			}
			// Persistenza: il ricordo vale per `ContactLifetimeTurns` turni DOPO quello dell'avvistamento.
			if (TurnNumber - Old.TurnNumber <= ContactLifetimeTurns)
			{
				Out.Contacts.Add(Old);
			}
		}
	}

	// Ordine canonico per `StableUnitId`: l'output non deve dipendere da come erano ordinati osservatori o nemici
	// in ingresso (invariante #3). A parita' di `StableUnitId` non c'e' ambiguita' — ce n'e' al piu' uno.
	Out.Contacts.Sort([](const FRTLastKnownContact& A, const FRTLastKnownContact& B)
		{ return A.StableUnitId < B.StableUnitId; });
	return Out;
}

ERTAwareness URTTeamKnowledgeLibrary::AwarenessOfUnit(const FRTTeamKnowledge& Knowledge, int32 StableUnitId,
	const FRTCellId& CurrentCell)
{
	if (Knowledge.Version != FRTTeamKnowledge::CurrentVersion)
	{
		return ERTAwareness::Hidden; // fail-closed: una conoscenza illeggibile non concede nulla
	}
	if (Knowledge.VisibleCells.Contains(CurrentCell))
	{
		return ERTAwareness::Detected;
	}
	for (const FRTLastKnownContact& Contact : Knowledge.Contacts)
	{
		if (Contact.StableUnitId == StableUnitId)
		{
			return ERTAwareness::Uncertain;
		}
	}
	return ERTAwareness::Hidden;
}

bool URTTeamKnowledgeLibrary::LastKnownCell(const FRTTeamKnowledge& Knowledge, int32 StableUnitId, FRTCellId& OutCell)
{
	if (const FRTLastKnownContact* Contact = FindContact(Knowledge, StableUnitId))
	{
		OutCell = Contact->Cell;
		return true;
	}
	return false;
}

const FRTLastKnownContact* URTTeamKnowledgeLibrary::FindContact(const FRTTeamKnowledge& Knowledge, int32 StableUnitId)
{
	if (Knowledge.Version != FRTTeamKnowledge::CurrentVersion)
	{
		return nullptr;
	}
	for (const FRTLastKnownContact& Contact : Knowledge.Contacts)
	{
		if (Contact.StableUnitId == StableUnitId)
		{
			return &Contact;
		}
	}
	return nullptr;
}

ERTTargetKnowledge URTTeamKnowledgeLibrary::ClassifyTarget(const FRTTeamKnowledge& Knowledge, int32 TargetStableUnitId,
	int32 TargetTeamId, const FRTCellId& TargetCurrentCell)
{
	// Un alleato non passa dalla conoscenza: la squadra conosce i propri per appartenenza, e `Knowledge` non
	// contiene se stessa. Senza questo ramo una cura a un compagno fuori vista verrebbe rifiutata, il che
	// non e' conoscenza parziale ma amnesia.
	if (TargetTeamId == Knowledge.TeamId)
	{
		return ERTTargetKnowledge::Allowed;
	}

	switch (AwarenessOfUnit(Knowledge, TargetStableUnitId, TargetCurrentCell))
	{
	case ERTAwareness::Detected:  return ERTTargetKnowledge::Allowed;
	case ERTAwareness::Uncertain: return ERTTargetKnowledge::CellOnly;
	default:                      return ERTTargetKnowledge::Rejected;
	}
}

FRTKnowledgeVerdict URTTeamKnowledgeLibrary::FreezeVerdict(const TArray<FRTTeamKnowledge>& AllKnowledge,
	const FRTKnowledgeSubject& Subject)
{
	// Parte da NoOne: una squadra che non compare in `AllKnowledge` non riceve il bit, e una lista vuota
	// produce un verdetto che non mostra nulla. Il silenzio e' l'esito sicuro di un filtro di privacy.
	FRTKnowledgeVerdict Verdict;

	for (const FRTTeamKnowledge& Knowledge : AllKnowledge)
	{
		// 🔴 Serve `Allowed`, non «diverso da Rejected». `Allowed` e' cio' che `ViewForTeam` traduce in
		// `Live` (`RTKnowledgeView.cpp`, ramo `Allowed`): il criterio e' lo STESSO di prima, spostato nel
		// tempo, non allargato.
		//
		// ⚠️ **`CellOnly` non basta, ed e' la tentazione da non seguire.** Una squadra con un contatto
		// incerto sa che quel soggetto esiste e dove lo ha visto l'ULTIMA volta; una riga di log ne stampa
		// la cella di ADESSO — che al momento della scrittura e' la cella del fatto. Concedere su `CellOnly`
		// regalerebbe proprio la posizione che il ricordo non contiene.
		if (ClassifyTarget(Knowledge, Subject.StableUnitId, Subject.TeamId, Subject.Cell) == ERTTargetKnowledge::Allowed)
		{
			Verdict.AllowTeam(Knowledge.TeamId);
		}
	}

	return Verdict;
}
