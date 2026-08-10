#include "Turn/RTReactionOpportunityTypes.h"

FString URTReactionOpportunityLibrary::DeriveOpportunityId(const FRTReactionOpportunityKey& Key)
{
	// Il separatore e' `|` e non il punto della notazione `Turn.MacroPhase.MicroStep.OwnerId.ReactionDefId.Seq`
	// del checkpoint: quella nomina i CAMPI, non il carattere. Il punto compare gia' dentro `ReactionDefId`
	// (`Action.Counter`), quindi userebbe lo stesso separatore dei campi e renderebbe possibile una coppia di
	// chiavi diverse con lo stesso id. Costa un carattere evitarlo.
	//
	// `MacroPhase` entra come intero: e' stabile finche' i valori di `ERTResolutionPhase` si aggiungono in
	// CODA, che e' gia' la regola del progetto per gli enum serializzati. Riordinarli cambierebbe l'id di ogni
	// opportunity gia' registrata — cioe' e' una migrazione di formato, e va trattata come tale.
	return FString::Printf(TEXT("T%d|P%d|M%d|U%d|%s|S%d"),
		Key.TurnNumber,
		static_cast<int32>(Key.MacroPhase),
		Key.MicroStepIndex,
		Key.OwnerId,
		*Key.ReactionDefId,
		Key.Seq);
}

bool URTReactionOpportunityLibrary::RequiresDecisionBoundary(const FRTReactionOpportunity& Opportunity)
{
	// ADR-0004 §2, e nient'altro. La soglia e' `>= 2` e non `> 0`: una sola risposta legale non e' una scelta,
	// quindi non c'e' niente da chiedere e il commit e' immediato — e' il caso degenere in cui vivono le
	// reazioni E5 (`Counter`, `Deflect`, `Shield`, `Cleanse`, e `Brace` col suo profilo base).
	return Opportunity.AllowedResponses.Num() >= 2;
}
