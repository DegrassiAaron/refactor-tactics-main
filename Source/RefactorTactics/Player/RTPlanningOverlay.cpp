#include "Player/RTPlanningOverlay.h"

bool FRTPlanningOverlayInputs::SamePlanLevel(const FRTPlanningOverlayInputs& Other) const
{
	// 🔑 **Il contesto entra come `ShowsAreas()`, non come valore**: `Planning`, `Pathing`, `Targeting` e `Facing` mostrano le
	// stesse aree e `RefreshPlanningPreview` non legge `bDeclaringFacing`, quindi aprire e chiudere il selettore del verso
	// rifarebbe due volte, per niente, snapshot e flood fill (review di `#3511`). Cio' che il contesto cambia DAVVERO per il
	// piano e' l'azione armata (che ha i suoi campi) e il passaggio fra mostrare e non mostrare (che e' qui).
	return Unit == Other.Unit
		&& bUnitAlive == Other.bUnitAlive
		&& ShowsAreas() == Other.ShowsAreas()
		&& ArmedAbilityIndex == Other.ArmedAbilityIndex
		&& ArmedTargetKind == Other.ArmedTargetKind
		&& PlanRevision == Other.PlanRevision
		&& TurnIndex == Other.TurnIndex
		&& bMatchEnded == Other.bMatchEnded
		// Il fuoco cambia COSA si calcola, non dove sta il cursore: e' livello del piano. 🔑 La sorgente entra insieme ai
		// campi che la producono, cosi' la struct resta onesta anche senza il risolutore che la normalizza.
		&& ActiveSource() == Other.ActiveSource()
		&& FocusSubject == Other.FocusSubject
		&& FocusMove == Other.FocusMove
		&& FocusKnowledgeStamp == Other.FocusKnowledgeStamp
		&& bFocusOnMap == Other.bFocusOnMap;
}

bool FRTPlanningOverlayInputs::operator==(const FRTPlanningOverlayInputs& Other) const
{
	// Il piano e poi l'hover. ⚠️ Ogni campo nuovo della struct entra in UNO dei due: un campo che non compare ne'
	// qui ne' in `SamePlanLevel` e' un ingresso che non fa mai ricalcolare niente.
	return SamePlanLevel(Other)
		&& Context == Other.Context
		&& ActiveLayer == Other.ActiveLayer
		&& HoveredCell == Other.HoveredCell
		&& bHoveredValid == Other.bHoveredValid
		&& FacingSector == Other.FacingSector;
}

bool FRTPlanningOverlayInputs::ShowsAreas() const
{
	if (!Unit.IsValid() || !bUnitAlive || bMatchEnded)
	{
		return false;
	}
	switch (Context)
	{
	case ERTPointerContext::Planning:
	case ERTPointerContext::Pathing:
	case ERTPointerContext::Targeting:
	case ERTPointerContext::Facing:
		return true;

	// Nessun `default:` e nessuna assunzione sui valori futuri: un contesto nuovo deve dire QUI se il mondo e'
	// scrivibile, e il compilatore lo chiede. Un `default: return false` renderebbe ogni contesto nuovo muto per
	// omissione — la forma dei tre difetti che questa struct esiste per chiudere.
	case ERTPointerContext::IdleSelection:
	case ERTPointerContext::ResolutionPlayback:
	case ERTPointerContext::ReactionWindow:
	case ERTPointerContext::Modal:
		return false;
	}
	return false;
}

bool FRTPlanningOverlayInputs::ContextAllowsFocus(ERTPointerContext InContext)
{
	switch (InContext)
	{
	case ERTPointerContext::IdleSelection:
	case ERTPointerContext::Planning:
		return true;

	// Gia' dopo un gesto di pianificazione: l'area del fuoco e' uscita (spec §3 regola 4).
	case ERTPointerContext::Pathing:
	case ERTPointerContext::Targeting:
	case ERTPointerContext::Facing:
	// Il fuoco resta spento in playback per scelta della sessione, e la scheda resta; in finestra di reazione e in modale
	// l'ispettore e' inerte (regola 5) e l'area e' NASCOSTA, non chiusa (`ContextSuspendsFocus`).
	case ERTPointerContext::ResolutionPlayback:
	case ERTPointerContext::ReactionWindow:
	case ERTPointerContext::Modal:
		return false;
	}
	return false;
}

bool FRTPlanningOverlayInputs::ContextSuspendsFocus(ERTPointerContext InContext)
{
	switch (InContext)
	{
	// Nascondono l'ispettore senza chiuderlo (spec §3 regola 11): l'area torna quando si chiudono.
	case ERTPointerContext::ReactionWindow:
	case ERTPointerContext::Modal:
		return true;

	case ERTPointerContext::IdleSelection:
	case ERTPointerContext::Planning:
	case ERTPointerContext::Pathing:
	case ERTPointerContext::Targeting:
	case ERTPointerContext::Facing:
	case ERTPointerContext::ResolutionPlayback:
		return false;
	}
	return false;
}

bool FRTPlanningOverlayInputs::ShowsFocus() const
{
	return FocusSubject.IsValid() && bFocusOnMap && !bMatchEnded && ContextAllowsFocus(Context);
}

ERTPreviewSource FRTPlanningOverlayInputs::ActiveSource() const
{
	if (ShowsFocus())
	{
		return ERTPreviewSource::Focus;
	}
	return ShowsAreas() ? ERTPreviewSource::Plan : ERTPreviewSource::None;
}

bool FRTFocusLatch::Observe(int32 InPinSerial, const FRTPlanningOverlayInputs& Raw)
{
	if (InPinSerial != PinSerial)
	{
		// Una fissazione nuova, anche sulla stessa unita': l'area rientra, e il piano di adesso e' la base da cui si misura il
		// primo gesto. Senza soggetto osservato non c'e' niente da accendere.
		PinSerial = InPinSerial;
		BaseUnit = Raw.Unit;
		BaseRevision = Raw.PlanRevision;
		BaseArmed = Raw.ArmedAbilityIndex;
		BaseTurn = Raw.TurnIndex;
		bOnMap = Raw.FocusSubject.IsValid();
		return bOnMap;
	}

	if (bOnMap)
	{
		const bool bUnchanged = Raw.FocusSubject.IsValid()
			&& !Raw.bMatchEnded
			&& (FRTPlanningOverlayInputs::ContextAllowsFocus(Raw.Context) || FRTPlanningOverlayInputs::ContextSuspendsFocus(Raw.Context))
			&& Raw.Unit == BaseUnit
			&& Raw.PlanRevision == BaseRevision
			&& Raw.ArmedAbilityIndex == BaseArmed
			&& Raw.TurnIndex == BaseTurn;
		// Una sola direzione: spenta resta spenta, anche se lo stato torna uguale.
		bOnMap = bUnchanged;
	}
	return bOnMap;
}
