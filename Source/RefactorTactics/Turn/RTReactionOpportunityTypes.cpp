#include "Turn/RTReactionOpportunityTypes.h"

FString URTReactionOpportunityLibrary::DeriveOpportunityId(const FRTReactionOpportunityKey& Key)
{
	// Il separatore e' `|` e non il punto della notazione `Turn.MacroPhase.MicroStep.OwnerId.ReactionDefId.Seq`
	// del checkpoint: quella nomina i CAMPI, non il carattere. Il punto compare gia' dentro `ReactionDefId`
	// (`Action.Counter`), quindi userebbe lo stesso separatore dei campi e renderebbe possibile una coppia di
	// chiavi diverse con lo stesso id. Costa un carattere evitarlo.
	//
	// `MacroPhase` entra come intero. `ERTMatchPhase` e' l'enum delle sei macro-fasi del turno, che il
	// progetto non riordina — l'ordine E' la sequenza del turno, quindi cambiarlo sarebbe una modifica alle
	// regole prima che al formato. Un valore nuovo va comunque aggiunto in CODA: gli id gia' registrati non
	// devono cambiare significato.
	//
	// `ReactionDefId` viene NORMALIZZATO a minuscole, e non e' cosmesi. `FName` confronta senza distinguere la
	// casing, ma `ToString()` restituisce quella dell'istanza: senza `ToLower()`, due punti del codice che
	// l'engine considera la stessa azione produrrebbero due id diversi, cioe' proprio l'identificatore
	// non-funzione-del-proprio-stato che questo file esiste per impedire. Pinnato da
	// `OpportunityIdIgnoresActionIdCasing`.
	return FString::Printf(TEXT("T%d|P%d|M%d|U%d|%s|S%d"),
		Key.TurnNumber,
		static_cast<int32>(Key.MacroPhase),
		Key.MicroStepIndex,
		Key.OwnerId,
		*Key.ReactionDefId.ToString().ToLower(),
		Key.Seq);
}

bool URTReactionOpportunityLibrary::RequiresDecisionBoundary(const FRTReactionOpportunity& Opportunity)
{
	// ADR-0004 §2, e nient'altro. La soglia e' `>= 2` e non `> 0`: una sola risposta legale non e' una scelta,
	// quindi non c'e' niente da chiedere e il commit e' immediato — e' il caso degenere in cui vivono le
	// reazioni E5 (`Counter`, `Deflect`, `Shield`, `Cleanse`, e `Brace` col suo profilo base).
	return Opportunity.AllowedResponses.Num() >= 2;
}
