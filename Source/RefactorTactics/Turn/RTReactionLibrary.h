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

	/**
	 * CHI ha innescato la reazione: l'`AttackerId` del primo colpo che soddisfa Trigger per SelfId, o
	 * `INDEX_NONE` se nessuno. `EvaluateReactionTrigger` e' esattamente `!= INDEX_NONE` su questa.
	 *
	 * Serve a `Action.Counter` (CP 5.2), che deve colpire chi l'ha colpito: senza l'identita' dell'attaccante,
	 * il contrattacco non avrebbe un bersaglio e andrebbe scelto a runtime — cioe' proprio la scelta implicita
	 * che il catalogo vieta. "Primo" nell'ordine CANONICO di `Plan.Hits` (attaccante, bersaglio), non nell'ordine
	 * di arrivo: due attaccanti che colpiscono nello stesso Blast producono sempre lo stesso contrattaccato.
	 */
	static int32 FindTriggeringAttacker(ERTReactionTrigger Trigger, int32 SelfId,
		const TArray<FRTHexAttackHit>& Hits, const TArray<FRTHexAttackIntent>& Intents);

	/**
	 * Il primo colpo che `SelfId` puo' INTERCETTARE (`Action.Intercept`, CP 5.3), o `INDEX_NONE`.
	 *
	 * Un colpo e' intercettabile quando TUTTE queste cose sono vere:
	 * - colpisce un ALLEATO (stessa squadra) diverso da `SelfId` — non ci si interpone davanti a se stessi;
	 * - quell'alleato e' entro `InterceptRange` celle da `SelfId` (2, dichiarato dal catalogo);
	 * - e' un attacco DIRETTO (intento a forma `Single`): un'area non ha una traiettoria da intercettare, e il
	 *   catalogo lo esclude esplicitamente. Il danno ambientale non passa nemmeno da qui (arriva con E8);
	 * - la traiettoria dall'attaccante a `SelfId` e' LIBERA: ci si mette in mezzo a un colpo, non lo si
	 *   teletrasporta addosso. Senza mappa non si intercetta (FAIL-CLOSED: non si puo' verificare la linea).
	 *
	 * `ExcludedHits` sono i colpi gia' reclamati da un altro intercettore nello stesso Blast: si passa avanti
	 * invece di contendere. Con due sole unita' per squadra (v0.1) la contesa non e' raggiungibile — un alleato
	 * colpito ne lascia al massimo uno che possa intercettare — ma il parametro tiene la funzione onesta se le
	 * squadre crescono.
	 *
	 * Funzione PURA: nessun Actor, nessun UWorld. Non applica la redirezione, la trova soltanto.
	 */
	static int32 FindInterceptableHit(int32 SelfId, int32 InterceptRange,
		const TArray<FRTHexAttackHit>& Hits, const TArray<FRTHexAttackIntent>& Intents,
		const TArray<FRTHexCombatUnit>& Units, const URTHexMapAsset* Map,
		const TSet<int32>& ExcludedHits);
};
