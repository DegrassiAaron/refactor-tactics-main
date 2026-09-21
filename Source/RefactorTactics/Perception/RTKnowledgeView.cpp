#include "Perception/RTKnowledgeView.h"

FRTKnowledgeView URTKnowledgeViewLibrary::ViewForTeam(const FRTTeamKnowledge& Knowledge,
	const TArray<FRTKnowledgeSubject>& Subjects, int32 ObserverTeamId)
{
	FRTKnowledgeView View;
	View.ObserverTeamId = ObserverTeamId;

	for (const FRTKnowledgeSubject& S : Subjects)
	{
		// «Un morto non e' un soggetto di conoscenza»: la regola vive in [D-431] (`#1498`), non qui.
		//
		// ⌫ **Fino al 2026-09-21 questa riga ERA la sede della regola**, e la portava in una riga di
		// commento. E' la stessa forma che [D-371] ha ritirato altrove nel repository, e qui valeva lo
		// stesso: il corpo di `#1498` cita questo commento come *«la causa (a)»* e non aveva una `D-` da
		// citare al suo posto.
		//
		// 🔑 **Cosa [D-431] decide.** La regola **resta**, e governa i canali calcolati **in lettura** —
		// velo, modello, sagoma del contatto — dove chiedere *«conosco quel morto?»* e' la domanda giusta.
		// Il **combat log** non passa piu' di qui: [D-223] congela il verdetto alla **scrittura**, quando il
		// soggetto e' ancora vivo, e `ClassifyTarget` non guarda lo stato vitale. Le due cause che `#1498`
		// teneva in serie sono oggi **disgiunte**.
		//
		// 🔴 **E oggi questa guardia non ha nessun consumatore che la osservi — misurato, non dedotto.**
		// L'unico chiamante di produzione e' `UvViewForObserver`, l'helper che `ARTHUD::UpdateObserverVeil`
		// usa; e il ciclo che consuma la vista **salta gia' i caduti da se'**
		// (`if (!Unit || !Unit->IsAlive()) { continue; }`). Togliere questa riga non farebbe comparire nessun
		// morto sull'overlay: non cambierebbe **niente di osservabile**.
		// ∴ [D-431] la tiene come **difesa in profondita'**, non perche' stia portando un carico: toglierla
		// farebbe dipendere il contratto di `ViewForTeam` dal fatto che ogni consumatore **futuro** si
		// ricordi di filtrare.
		//
		// ⌫ **E la ragione qui sopra e' piu' debole del vero: corretta con `#3253`.** Non e' il salto del
		// ciclo a rendere inosservabile questa guardia. Un cadavere lascia lo schermo per un atto
		// **imperativo** — `ARTUnit::HideForDefeat()`, chiamato da `ARTTurnManager::FinishPlayback` — e fino
		// a li' resta a schermo APPOSTA, perche' il colpo mortale sia osservabile
		// (`RefactorTactics.Playback.DefeatAnnouncedWhileStillVisible` lo pinna). Le guardie **dichiarative**
		// sono tre — questa, quella del ciclo, e il termine `bAlive` di `ARTUnit::ShouldBeRendered` — e
		// **nessuna delle tre scatta alla morte**. Il conto sta in [D-431].
		//
		// ✅ **La duplicazione dichiarata qui era un `FOLLOW-UP CANDIDATE`, ed e' CHIUSO** (`#3253`): il
		// docstring di `UvViewForObserver` non vieta piu' una duplicazione che il file commette comunque, e
		// dice il conto giusto — filtrare nei soggetti sarebbe la **terza** copia, non la seconda.
		if (!S.bAlive)
		{
			continue;
		}

		FRTKnowledgeEntry E;
		E.StableUnitId = S.StableUnitId;
		E.HeroId = S.HeroId;
		E.HeroDisplayName = S.HeroDisplayName;
		// ➕ **La squadra, scritta QUI e non in ciascun ramo** (`#3039`). Tre rami la aggiungono uguale, e
		// la quarta uscita e' *nessuna voce*: metterla sopra la biforcazione toglie il difetto in cui un
		// ramo aggiunto domani la dimentica. ⚠️ Non e' un dato nuovo — `ClassifyTarget` lo consuma gia'
		// due righe piu' sotto per decidere se la voce esiste.
		E.TeamId = S.TeamId;

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
