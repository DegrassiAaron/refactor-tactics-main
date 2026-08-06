#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Ability/RTActionDef.h"
#include "Combat/RTHexCombatLibrary.h"
#include "RTReactionLibrary.generated.h"

/**
 * Esito registrato nel TurnLog per una reazione pianificata (CP 5.1): l'attivazione — o la non-attivazione —
 * compare sempre, mai in silenzio.
 */
UENUM(BlueprintType)
enum class ERTReactionOutcome : uint8
{
	/** Il trigger e' scattato: la reazione parte (CP 5.2 e succ. ne applicano l'effetto). */
	Activated,
	/** Pianificata e disponibile, ma nessun trigger valido in questo Blast. */
	NotTriggered,
	/** Pianificata ma non poteva scattare: cooldown attivo, o un'altra azione del turno lo vieta (Sprint). */
	Unavailable
};

/**
 * Valutazione PURA del trigger di una reazione (CP 5.1, epic E5): nessun Actor, nessun UWorld, nessun
 * `Delay`/timeline — non puo' introdurre un'attesa nel resolver (invariante #3) perche' non ha modo di farlo.
 * Opera sui colpi GIA' raccolti del Blast (`FRTHexBlastPlan::Hits`, dopo il filtro di `Action.Interrupt`):
 * lo stesso "raccogli poi applica" del resto del motore azioni.
 */
UCLASS()
class REFACTORTACTICS_API URTReactionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Vero se almeno un colpo di Hits soddisfa Trigger per l'unita' SelfId.
	 *
	 * `HitByDirectAttack` — SelfId e' il bersaglio di almeno un colpo con intento a forma `Single` (un
	 * attacco AoE/lineare non conta come "diretto": lo dichiara il catalogo per Counter/Deflect).
	 *
	 * Con piu' colpi validi nello stesso Blast, la funzione si ferma al primo: e' il chiamante
	 * (`ARTTurnManager::ResolveCombat`) a garantire una sola attivazione per turno, non un conteggio qui.
	 */
	static bool EvaluateReactionTrigger(ERTReactionTrigger Trigger, int32 SelfId,
		const TArray<FRTHexAttackHit>& Hits, const TArray<FRTHexAttackIntent>& Intents);
};
