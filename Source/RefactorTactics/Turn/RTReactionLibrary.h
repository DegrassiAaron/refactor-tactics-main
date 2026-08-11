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
 * QUANDO, dentro il turno, un trigger di reazione puo' essere valutato (D-092, CP 7.5 `#505`).
 *
 * Nasce perche' i trigger hanno smesso di essere omogenei: `HitByDirectAttack` si legge sui colpi raccolti,
 * ma «sto per essere spinto» si sa solo dove la spinta e' decisa, e «la mia cella e' diventata pericolosa»
 * solo nel Cleanup. Un pass unico li valuterebbe tutti nel momento sbagliato per almeno uno.
 *
 * Il valore serve a due cose in un colpo: dice a `ARTTurnManager::RunReactionPass` **quali** unita' guardare,
 * e tiene fuori le altre dal TurnLog — senza, una reazione non pertinente prenderebbe una voce
 * `NotTriggered` in ogni punto di valutazione del turno invece che nel suo.
 */
UENUM()
enum class ERTReactionPassPoint : uint8
{
	/** Nessun punto: il trigger non e' valutato da nessuno (`None`, cioe' dato incompleto). */
	Never,
	/** Sui colpi gia' raccolti del Blast, dopo il filtro di `Action.Interrupt` (`Counter`, `Deflect`). */
	BlastHits,
	/**
	 * L'INTERPOSIZIONE, che ha un ciclo suo in `ResolveCombat` — le serve la mappa, le squadre e una
	 * traiettoria libera, non i soli colpi. `RunReactionPass` la salta proprio per questo.
	 */
	BlastIntercept,
	/**
	 * Gli spostamenti DECISI e non ancora applicati (`Reaction.Anchor`). Dopo l'applicazione sarebbe tardi:
	 * annullare vorrebbe dire rimettere indietro un'unita' gia' mossa, con due voci di TurnLog che si
	 * contraddicono sullo stesso passo.
	 */
	BlastDisplacement
};

/**
 * Esito della valutazione di un trigger: **se** e' scattato, e — quando ha senso — **chi** l'ha innescato.
 *
 * Due campi e non un `int32` con `INDEX_NONE` come «non scattato», che e' la convenzione di
 * `FindTriggeringAttacker`: quella regge finche' ogni trigger nasce da un colpo, e un colpo ha sempre un
 * attaccante. Una spinta no — ne conosciamo la cella di provenienza, non necessariamente un'unita' — e con la
 * convenzione vecchia una reazione legittima come `Anchor` risulterebbe «mai scattata».
 */
struct FRTReactionTriggerHit
{
	/** Vero se la reazione deve attivarsi. */
	bool bTriggered = false;

	/**
	 * CHI l'ha innescata, o `INDEX_NONE` se l'evento non ha un'unita' responsabile identificabile. Decide il
	 * bersaglio degli effetti offensivi; quelli difensivi non ne hanno bisogno.
	 */
	int32 TriggeredBy = INDEX_NONE;
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

	/**
	 * TUTTI gli effetti dichiarati da una reazione appena attivata, tradotti in eventi (CP 5.5).
	 *
	 * Prima di CP 5.5 il resolver leggeva dalla reazione **il primo `Damage` e nient'altro**, piu' un
	 * `if (ActionId == "Action.Deflect")` per la riduzione: una reazione con due effetti — `Shield 15` a se'
	 * **e** 10 danni all'attaccante, che e' `Flux.ReactiveCapacitor` — ne avrebbe applicato meta'. Qui la
	 * lista si traduce per intero, nell'ordine dichiarato.
	 *
	 * CHI subisce ciascun effetto e' una regola per TIPO, non per azione — cosi' una reazione d'eroe non deve
	 * dichiarare bersagli e il motore non deve conoscerne l'identita':
	 * - `Damage`, `Push`, `Pull`, `Status` -> **chi ha innescato** la reazione (il senso di un contrattacco);
	 * - `Heal`, `Shield`, `DamageReduction` -> **chi reagisce** (il senso di una difesa).
	 *
	 * `TriggeredBy == INDEX_NONE` (nessun attaccante identificato) elimina gli effetti del primo gruppo invece
	 * di sceglierne uno a caso: un contrattacco senza bersaglio dichiarato non si inventa un bersaglio.
	 *
	 * Funzione PURA: traduce, non applica. E' il chiamante (`ARTTurnManager::ResolveCombat`) a decidere quando
	 * gli eventi entrano nel turno, con lo stesso "raccogli poi applica" del resto del motore (invariante #3).
	 */
	static TArray<FRTActionEvent> BuildReactionEvents(const FRTActionDef& Def, int32 SelfId, int32 TriggeredBy);

	/**
	 * In quale punto del turno si valuta questo trigger. Funzione PURA e totale: ogni valore dell'enum ne ha
	 * uno, e lo `switch` non ha `default` — un trigger nuovo non compila finche' qualcuno non dichiara **dove**
	 * viene valutato.
	 *
	 * E' la guardia contro il difetto ricorrente di questo motore: aggiungere un valore al catalogo e
	 * ottenere un modulo che non scatta mai, con un test verde che ne verifica solo il DATO. Pinnata da
	 * `Reaction.EveryTriggerHasAPassPoint`, che fallisce se un trigger diverso da `None` finisce su `Never`.
	 */
	static ERTReactionPassPoint PassPointFor(ERTReactionTrigger Trigger);
};
