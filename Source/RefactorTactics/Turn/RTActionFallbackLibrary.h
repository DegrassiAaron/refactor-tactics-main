#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Ability/RTActionDef.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Turn/RTActionQueue.h"
#include "Turn/RTTurnLog.h"
#include "RTActionFallbackLibrary.generated.h"

class URTHexMapAsset;

/**
 * Perche' un'azione pianificata non e' piu' eseguibile al momento della risoluzione. E' il "reason code" del
 * fallback: senza, il log direbbe che l'azione e' stata annullata senza dire da cosa — e chi gioca non
 * imparerebbe nulla da un turno andato storto.
 */
UENUM(BlueprintType)
enum class ERTActionInvalidReason : uint8
{
	/** L'azione e' eseguibile come pianificata. */
	None,
	/** Il bersaglio non esiste piu' nel turno (rimosso, o mai indicato). */
	TargetGone,
	/** Il bersaglio e' stato eliminato prima che l'azione risolvesse. */
	TargetDead,
	/** Il bersaglio e' un alleato: le azioni offensive del vertical slice non colpiscono la propria squadra. */
	TargetFriendly,
	/** Il bersaglio si e' allontanato oltre la portata dichiarata. */
	OutOfRange,
	/** In portata, ma la traiettoria e' bloccata. */
	NoLineOfSight,
	/** Nessuna mappa autorevole: non si valida (fail-closed, difetto di configurazione del livello). */
	NoMap,
	/**
	 * Il bersaglio e' IGNOTO alla squadra dell'attaccante (CP 13.2): nessuno lo vede e non ne resta un
	 * ricordo vivo. Non e' un difetto di geometria — portata e traiettoria possono essere perfette — ma di
	 * CONOSCENZA, ed e' per questo che ha un motivo proprio invece di riusare `TargetGone`: quello dice «non
	 * c'e' piu'», questo dice «per te non c'e' mai stato».
	 *
	 * Valore aggiunto in coda: le tracce gia' scritte conservano il proprio significato.
	 */
	TargetUnknown,

	/**
	 * Lo slot che l'azione occupa e' gia' preso da un'altra voce dello stesso piano (CP 38.2).
	 *
	 * E' la forma CORRETTA del vincolo che il kit d'autore del 2026-08-12 proponeva come regola a se'
	 * (`OverwatchDisallowsVoluntaryDash`): D-070 dichiara che quel divieto non serve, perche' chi riserva
	 * lo slot movimento non VIETA lo scatto — semplicemente non ha piu' lo slot. Un motivo solo copre
	 * `Sprint`+principale, due principali e scatto+movimento, invece di una regola per combinazione.
	 */
	SlotOccupied,

	/** Il costo in Movement Point del piano supera il budget dell'unita' (`FRTHexSimUnit::MoveBudget`). */
	InsufficientMovementPoints,

	/** L'abilita' non e' ancora ripetibile: il cooldown residuo e' maggiore di zero. */
	OnCooldown
};

/** Esito dell'applicazione di un fallback: cosa si esegue davvero, e cosa e' stato applicato. */
USTRUCT(BlueprintType)
struct FRTFallbackResult
{
	GENERATED_BODY()

	/** L'istanza da eseguire al posto di quella pianificata. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Actions")
	FRTActionInstance Instance;

	/** Il fallback effettivamente applicato (quello dichiarato, o `Cancel` se non praticabile in v0.1). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Actions")
	ERTActionFallback Applied = ERTActionFallback::Cancel;

	/** Falso se l'azione non produce piu' alcun effetto (`Cancel`, e `Wait` che occupa lo slot e basta). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Actions")
	bool bProducesEffects = false;

	FRTFallbackResult() = default;
};

/**
 * Validazione delle azioni pianificate e applicazione del fallback DICHIARATO dal catalogo.
 *
 * Sta in un posto solo (spec E4 §D5): concentrare la domanda «e se il bersaglio non c'e' piu'?» evita che
 * ogni effetto se la rifaccia per conto suo, con risposte diverse. Funzioni pure: nessun Actor, nessun UWorld.
 */
UCLASS()
class REFACTORTACTICS_API URTActionFallbackLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Perche' l'istanza non e' piu' eseguibile, valutata sullo stato CONGELATO della fase (invariante #3).
	 * Ordine dei controlli: esistenza -> vita -> squadra -> portata -> linea di tiro. E' l'ordine in cui i
	 * motivi si escludono a vicenda, e da esso dipende quale reason code finisce nel log.
	 *
	 * Le azioni senza bersaglio (movimento, supporto su se stessi) sono sempre valide qui: il loro esito lo
	 * decide il resolver del movimento, non la geometria del tiro.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Actions")
	static ERTActionInvalidReason ValidateInstance(const FRTActionInstance& Instance,
		const TArray<FRTHexCombatUnit>& Units, const URTHexMapAsset* Map);

	/**
	 * L'azione da eseguire quando la validazione fallisce, secondo il `Fallback` dichiarato dall'azione.
	 *
	 * - `Cancel` — non esegue nulla (attacchi diretti e cure).
	 * - `AttackCell` — perde il bersaglio ma colpisce la CELLA pianificata (AoE).
	 * - `AttackTarget` — segue il bersaglio se e' ancora valido; se il motivo dice che non lo e', annulla.
	 * - `Stop` — l'azione di movimento procede fino all'ultima cella valida: e' il resolver a fermarla, qui
	 *   l'istanza resta intatta.
	 * - `Wait` — sostituisce l'azione con `Action.Wait`, che occupa lo slot e non fa nulla.
	 * - `BasicAttack` — **non praticabile in v0.1**: sceglierebbe «il bersaglio valido piu' vicino», cioe' il
	 *   targeting automatico che il catalogo §17 elenca fra gli errori da evitare. Degrada a `Cancel`, e il
	 *   risultato lo DICHIARA (`Applied`), invece di fingere di aver fatto qualcosa.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Actions")
	static FRTFallbackResult ApplyFallback(const FRTActionInstance& Instance, ERTActionInvalidReason Reason);

	/**
	 * Come si registra nel TurnLog il fallback APPLICATO. Funzione totale: un fallback senza esito
	 * corrispondente sarebbe un'azione che sparisce dal log, cioe' il difetto che CP 4.3 va a chiudere.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Actions")
	static ERTFallbackOutcome ToLogOutcome(ERTActionFallback Fallback);
};
