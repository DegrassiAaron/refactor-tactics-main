#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Ability/RTActionDef.h"
#include "RTReactionOpportunityTypes.generated.h"

/**
 * I sei campi che individuano una `ReactionOpportunity` in una partita (CP 14.3).
 *
 * Esiste perche' l'id dell'opportunity dev'essere una FUNZIONE dello stato e non un valore generato: la
 * decisione presa in una finestra entra nel TurnLog, e il replay la ritrova solo se l'id si ricalcola identico
 * su una seconda esecuzione. Un `FGuid::NewGuid()` sarebbe piu' breve da scrivere e renderebbe il replay non
 * riproducibile senza che nessun test se ne accorga.
 */
USTRUCT()
struct FRTReactionOpportunityKey
{
	GENERATED_BODY()

	/** Turno in cui l'opportunity si apre. */
	UPROPERTY()
	int32 TurnNumber = 0;

	/** Macro-fase della risoluzione: due opportunity dello stesso turno in fasi diverse sono distinte. */
	UPROPERTY()
	ERTResolutionPhase MacroPhase = ERTResolutionPhase::Snapshot;

	/**
	 * Micro-step del movimento in cui l'opportunity nasce (CP 14.2).
	 *
	 * ⚠️ `FRTMovementResolutionState::MicroStepIndex` e' oggi documentato come «diagnostico: nessuna regola lo
	 * legge». Entrando qui smette di esserlo: diventa parte di un identificatore che finisce nell'hash del
	 * replay. Il commento alla sorgente va aggiornato quando questa chiave viene popolata dal resolver, o
	 * mentira' esattamente come i due commenti su `Slow` corretti in `ceb1b29`.
	 */
	UPROPERTY()
	int32 MicroStepIndex = 0;

	/** Unita' che possiede l'opportunity — chi puo' rispondere, non chi ha innescato. */
	UPROPERTY()
	int32 OwnerId = INDEX_NONE;

	/** La reaction che l'ha aperta (`Action.Counter`, `Action.Deflect`, ...). */
	UPROPERTY()
	FString ReactionDefId;

	/**
	 * Progressivo fra opportunity identiche in tutto il resto.
	 *
	 * Serve al caso che sembra impossibile finche' non capita: la stessa unita', la stessa reaction, lo stesso
	 * micro-step, due volte. Senza `Seq` le due collidono e il replay attribuisce a una la decisione dell'altra.
	 */
	UPROPERTY()
	int32 Seq = 0;
};

/**
 * Una opportunity di reazione: chi puo' rispondere, e **quali risposte gli sono legali** (ADR-0004 §2).
 *
 * La cardinalita' di `AllowedResponses` e' l'unico discriminante fra i due regimi, e non esiste un secondo
 * criterio: `≤ 1` e' il caso degenere — le reazioni E5 di oggi, che scattano o non scattano — e `≥ 2` apre il
 * decision boundary. Un modello solo, con E5 conservata dentro, invece di due da mantenere.
 */
USTRUCT()
struct FRTReactionOpportunity
{
	GENERATED_BODY()

	/** Identita' dell'opportunity: vedi `FRTReactionOpportunityKey`. */
	UPROPERTY()
	FRTReactionOpportunityKey Key;

	/**
	 * Le risposte legali per il possessore, gia' filtrate.
	 *
	 * ⚠️ Vuoto e singleton **non** sono lo stesso caso per chi legge — nessuna risposta legale contro una sola
	 * — ma per la regola del boundary lo sono: in entrambi non c'e' niente da scegliere. Il conteggio si fa
	 * qui e non sul catalogo: [D-047](../../../docs/decisions/RT_PDR_00_Decision_Log.md) precisa che la
	 * cardinalita' di `Brace` e' 1 **per il profilo base**, non per natura, e un profilo d'eroe che dichiari
	 * una seconda risposta apre il boundary con questa stessa regola invece di aggiungerne una nuova.
	 */
	UPROPERTY()
	TArray<FString> AllowedResponses;
};

/**
 * Derivazione dell'identita' di una opportunity. Funzioni pure: nessuno stato, nessun accesso al mondo.
 */
UCLASS()
class REFACTORTACTICS_API URTReactionOpportunityLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Vero se questa opportunity richiede un decision boundary; falso se si committa immediatamente.
	 *
	 * E' la regola di ADR-0004 §2 e nient'altro: `AllowedResponses.Num() >= 2`. Sta in una funzione pura, e
	 * non dentro il resolver, perche' il resolver possa chiederla senza che nessuno possa risponderle
	 * diversamente altrove — due punti che decidono «serve una finestra?» sono due regole.
	 */
	static bool RequiresDecisionBoundary(const FRTReactionOpportunity& Opportunity);

	/**
	 * Id stabile e ispezionabile derivato dai sei campi di `Key`.
	 *
	 * Stabile: la stessa chiave da' lo stesso id in ogni esecuzione e in ogni processo — nessun hash di
	 * puntatore, nessun contatore globale, nessun GUID.
	 */
	static FString DeriveOpportunityId(const FRTReactionOpportunityKey& Key);
};
