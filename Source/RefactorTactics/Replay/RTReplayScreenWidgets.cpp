#include "Replay/RTReplayScreenWidgets.h"

#include "Frontend/RTFrontendNavigator.h"
#include "Frontend/RTFrontendScreenIds.h"
#include "Replay/RTReplayViewerSubsystem.h"
#include "Engine/GameInstance.h"

#define LOCTEXT_NAMESPACE "RefactorTactics"

namespace
{
	/**
	 * Il subsystem chiesto alla `GameInstance` del widget.
	 *
	 * ⚠️ `nullptr` non e' un caso da nascondere: un widget costruito fuori da una `GameInstance` — come in
	 * un test che non allestisce l'host — non ha da chi farsi servire, e ogni chiamante di questo file
	 * risponde con l'esito che quella condizione merita invece di crashare.
	 */
	template <typename T>
	T* SubsystemOf(const UUserWidget* Widget)
	{
		const UGameInstance* GI = Widget ? Widget->GetGameInstance() : nullptr;
		return GI ? GI->GetSubsystem<T>() : nullptr;
	}
}

// =====================================================================================================
// MatchHistory
// =====================================================================================================

URTReplayViewerSubsystem* URTMatchHistoryWidgetBase::Replay() const
{
	return SubsystemOf<URTReplayViewerSubsystem>(this);
}

URTFrontendNavigator* URTMatchHistoryWidgetBase::Navigator() const
{
	return SubsystemOf<URTFrontendNavigator>(this);
}

TArray<FRTMatchHistoryEntry> URTMatchHistoryWidgetBase::LoadMatches(bool& bOutReadFailed)
{
	TArray<FRTMatchHistoryEntry> Partite = LoadMatchesFrom(Replay(), bOutReadFailed);

	// `IsEmpty()` risponde su cio' che si e' letto DAVVERO: su un fallimento la lista e' vuota, ma il
	// messaggio «nessun replay» sarebbe una bugia — e i due stati hanno due messaggi diversi.
	bLastLoadWasEmpty = !bOutReadFailed && Partite.Num() == 0;
	return Partite;
}

TArray<FRTMatchHistoryEntry> URTMatchHistoryWidgetBase::LoadMatchesFrom(URTReplayViewerSubsystem* V,
	bool& bOutReadFailed)
{
	TArray<FRTMatchHistoryEntry> Partite;

	// ⚠️ **Senza subsystem e' un FALLIMENTO DI LETTURA, non una lista vuota**, e la differenza e' cio' che
	// la schermata mostra: «non hai ancora giocato» contro «non ho potuto leggere». Confonderli farebbe
	// sembrare vuota un'installazione piena di registrazioni.
	bOutReadFailed = (V == nullptr) || !V->LoadMatchList(/*bNewestFirst=*/ true, Partite);
	return Partite;
}

bool URTMatchHistoryWidgetBase::OpenMatch(const FGuid& MatchId)
{
	return SelectAndNavigate(Replay(), Navigator(), MatchId);
}

bool URTMatchHistoryWidgetBase::SelectAndNavigate(URTReplayViewerSubsystem* Replay,
	URTFrontendNavigator* Navigator, const FGuid& MatchId)
{
	// Un `FGuid` invalido non arriva a `SelectMatch`: la selezione resterebbe «vuota» e il viewer si
	// aprirebbe su niente, cioe' il dead-end che questa schermata esiste per non produrre.
	if (Replay == nullptr || Navigator == nullptr || !MatchId.IsValid())
	{
		return false;
	}

	// 🔴 **PRIMA la selezione, POI la navigazione**, ed e' il contratto: `PushScreen` presenta il widget in
	// modo SINCRONO, quindi il viewer consuma la selezione durante questa riga. Invertire darebbe un viewer
	// che si apre su nulla — e il difetto non sarebbe intermittente: sarebbe sempre.
	Replay->SelectMatch(MatchId);

	const ERTNavResult Esito = Navigator->PushScreen(RTScreenIds::ReplayViewer);
	if (Esito != ERTNavResult::Ok)
	{
		// ⚠️ **La selezione si ritira**, e non e' zelo: lasciata li', la prossima comparsa del viewer —
		// magari per un'altra strada — aprirebbe una partita che nessuno ha scelto adesso. E' lo stesso
		// motivo per cui la selezione si CONSUMA invece di leggersi.
		Replay->ConsumeSelectedMatch();
		return false;
	}
	return true;
}

ESlateVisibility URTMatchHistoryWidgetBase::GetEmptyNoticeVisibility() const
{
	return IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
}

// =====================================================================================================
// ReplayViewer
// =====================================================================================================

URTReplayViewerSubsystem* URTReplayViewerWidgetBase::Replay() const
{
	return SubsystemOf<URTReplayViewerSubsystem>(this);
}

URTFrontendNavigator* URTReplayViewerWidgetBase::Navigator() const
{
	return SubsystemOf<URTFrontendNavigator>(this);
}

ERTReplayOpenResult URTReplayViewerWidgetBase::OpenSelected()
{
	return OpenSelectedOn(Replay());
}

ERTReplayOpenResult URTReplayViewerWidgetBase::OpenSelectedOn(URTReplayViewerSubsystem* V)
{
	if (V == nullptr)
	{
		return ERTReplayOpenResult::ManifestUnreadable;
	}

	// Consuma: una selezione che sopravvivesse verrebbe riusata da una comparsa successiva, e il viewer
	// riaprirebbe la partita di prima senza che nulla lo dica.
	const FGuid Scelta = V->ConsumeSelectedMatch();
	if (!Scelta.IsValid())
	{
		// ⚠️ **`ManifestUnreadable` e non un quinto esito**: non c'e' un archivio da leggere, ed e'
		// esattamente cio' che quel valore significa. Inventare un `NoSelection` darebbe alla schermata un
		// ramo in piu' da disegnare per una condizione che, se il flow e' corretto, non accade mai.
		return ERTReplayOpenResult::ManifestUnreadable;
	}

	// Con gli occhi di chi l'ha giocata ([D-317]): l'osservatore lo dichiara l'archivio, e questa schermata
	// non ha modo di conoscerlo.
	return V->OpenMatchAsRecordedObserver(Scelta);
}

bool URTReplayViewerWidgetBase::Back()
{
	return BackToListOn(Replay(), Navigator());
}

bool URTReplayViewerWidgetBase::BackToListOn(URTReplayViewerSubsystem* V, URTFrontendNavigator* Nav)
{
	if (Nav == nullptr)
	{
		return false;
	}

	const ERTNavResult Esito = Nav->PopScreen();
	if (Esito != ERTNavResult::Ok)
	{
		// L'archivio NON si chiude su una navigazione rifiutata: si resta nel viewer, e chiuderlo lascerebbe
		// una schermata viva davanti a una sessione svuotata.
		return false;
	}

	// Si e' usciti: le tracce si liberano. `FRTReplaySession::Traces` porta il TurnLog di **ogni** turno
	// registrato, e un subsystem di `GameInstance` sopravvive a ogni caricamento di livello.
	if (V)
	{
		V->Close();
	}
	return true;
}

FText URTReplayViewerWidgetBase::GetOpenFailureText(ERTReplayOpenResult Result)
{
	switch (Result)
	{
	case ERTReplayOpenResult::Opened:
		return FText::GetEmpty();

	case ERTReplayOpenResult::ManifestUnreadable:
		// Copre due condizioni che per chi guarda sono la stessa: l'archivio non c'e' piu' (l'indice per
		// design non apre le cartelle, quindi una riga puo' puntare a una cancellata) oppure il suo header
		// non e' leggibile da questo binario.
		return LOCTEXT("ReplayOpenManifestUnreadable",
			"Questa registrazione non è più leggibile: l'archivio è stato rimosso o è di una versione sconosciuta.");

	case ERTReplayOpenResult::TopologyMismatch:
		return LOCTEXT("ReplayOpenTopologyMismatch",
			"Questa registrazione viene da una versione del gioco con una mappa diversa, e non può essere riprodotta.");

	case ERTReplayOpenResult::TraceUnreadable:
		return LOCTEXT("ReplayOpenTraceUnreadable",
			"L'archivio è danneggiato: uno dei turni registrati non si rilegge.");

	default:
		// ⚠️ Nessun `default` silenzioso che restituisca vuoto: un esito aggiunto domani senza il suo testo
		// darebbe una schermata muta su un fallimento, che e' il difetto di `#472` («i quattro esiti restano
		// distinti») nella sua forma piu' subdola.
		return LOCTEXT("ReplayOpenUnknown", "La registrazione non si è aperta, per un motivo non dichiarato.");
	}
}

#undef LOCTEXT_NAMESPACE
