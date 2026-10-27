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
		&& bMatchEnded == Other.bMatchEnded;
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
