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
 * Derivazione dell'identita' di una opportunity. Funzioni pure: nessuno stato, nessun accesso al mondo.
 */
UCLASS()
class REFACTORTACTICS_API URTReactionOpportunityLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Id stabile e ispezionabile derivato dai sei campi di `Key`.
	 *
	 * Stabile: la stessa chiave da' lo stesso id in ogni esecuzione e in ogni processo — nessun hash di
	 * puntatore, nessun contatore globale, nessun GUID.
	 */
	static FString DeriveOpportunityId(const FRTReactionOpportunityKey& Key);
};
