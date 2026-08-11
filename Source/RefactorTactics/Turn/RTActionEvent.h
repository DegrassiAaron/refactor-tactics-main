#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Map/RTCellId.h"
#include "RTActionEvent.generated.h"

/**
 * Effetto ELEMENTARE che un'azione puo' produrre. E' l'unita' minima del motore azioni: un'azione del
 * catalogo dichiara quale effetto genera, e il registry sa tradurla in eventi.
 *
 * Enum e non `FName`: gli effetti sono un insieme chiuso e piccolo, il compilatore verifica che ogni caso sia
 * gestito, e la funzione di traduzione resta TOTALE. Il modding (effetti definiti da dati) e' north-star, non
 * v0.1 — vedi docs/gameplay/spec-motore-azioni-e4.md §7, domanda 1.
 */
UENUM(BlueprintType)
enum class ERTActionEffect : uint8
{
	/** Nessun effetto: l'azione occupa lo slot e basta (es. attesa). */
	None,
	/** Danno diretto al bersaglio (scudo prima, poi salute). */
	Damage,
	/** Cura al bersaglio, senza superare la salute massima. */
	Heal,
	/** Scudo TEMPORANEO al bersaglio: protegge il turno e scade nel Cleanup. */
	Shield,
	/** Spinta del bersaglio lungo la direzione del colpo (allontana). */
	Push,
	/** Trazione del bersaglio lungo la direzione del colpo (avvicina). Simmetrica a Push, mai la stessa cosa
	 * con un segno: le due hanno una destinazione diversa da calcolare (`URTHexCombatLibrary::HexPullDestination`
	 * si ferma prima della cella di chi tira, `HexKnockbackDestination` prima di un ostacolo qualunque). */
	Pull,
	/** Applica uno stato (Root, Slow, Reveal, ...) per una durata in turni. */
	Status,
	/**
	 * RIDUCE il danno diretto che sta per colpire il bersaglio dell'effetto (`Action.Deflect`: -20).
	 *
	 * Non e' uno scudo: lo scudo ASSORBE danno e si consuma, questa toglie punti al colpo prima che venga
	 * calcolato — la differenza si vede quando il colpo e' piu' debole della riduzione (lo scudo resterebbe,
	 * la riduzione no) e nel TurnLog, dove un colpo assorbito e un colpo attenuato non sono lo stesso esito.
	 *
	 * Fino a CP 5.5 il -20 di Deflect era un `if (ActionId == "Action.Deflect")` nel `TurnManager`: renderlo un
	 * effetto DICHIARATO e' cio' che permette a una reazione d'eroe di riusarne la semantica con numeri propri
	 * senza aggiungere un ramo. Chi lo consuma e' il pass delle reazioni di `ARTTurnManager::ResolveCombat`,
	 * che lo raccoglie prima di risolvere i colpi; nelle altre fasi non ha significato (una riduzione senza un
	 * colpo in arrivo non riduce nulla) ed e' ignorata, come gli altri effetti fuori posto.
	 *
	 * Aggiunto in CODA all'enum: i valori precedenti non si rinumerano, quindi i data asset gia' salvati
	 * continuano a significare la stessa cosa.
	 */
	DamageReduction,
	/**
	 * Danno alle STRUTTURE (coperture, e da CP 9.3/9.4 porte e ponti): una scala DIVERSA da `Damage` —
	 * `Action.HeavyAttack` vale 35 contro un'unita' e 20 contro un muro, `Gadget.BreachCharge` 35 contro un
	 * muro (catalogo v0.1, #61).
	 *
	 * Effetto separato e non un secondo campo su `Damage` perche' il catalogo deve dire ESPLICITAMENTE chi
	 * puo' sfondare: con un campo, le ~28 azioni che non sfondano porterebbero uno zero che non significa
	 * nulla. Chi lo consuma e' la raccolta del Blast (`URTHexCombatLibrary::CollectHexAttacks`), che lo porta
	 * sul primo bordo coperto attraversato dal colpo; l'applicazione e' a fase conclusa
	 * (`URTHexCoverLibrary::ApplyStructureDamage`), cosi' l'ordine dei colpi non cambia l'esito.
	 *
	 * Aggiunto in CODA all'enum, come `DamageReduction`: i valori precedenti non si rinumerano e i data asset
	 * gia' salvati continuano a significare la stessa cosa.
	 */
	DamageStructure,
	/**
	 * Porta una PORTA allo stato dichiarato (CP 9.3): `Amount` e' il valore di `ERTHexDoorState` — la stessa
	 * convenzione con cui la durata di uno stato viaggia in `Amount`, interi soltanto. Un valore fuori
	 * intervallo non produce nessuna operazione.
	 *
	 * Come `DamageStructure` non colpisce un'UNITA': lo raccoglie il Blast sulla prima PORTA attraversata dal
	 * colpo (`URTHexCombatLibrary::CollectHexAttacks`) e lo applica a fase conclusa
	 * (`URTHexDoorLibrary::ApplyDoorOps`), cosi' l'ordine degli ordini non cambia l'esito.
	 *
	 * Non e' un'azione di catalogo: l'azione che apre e chiude porte e' CP 10.1 (`Action.Interact`).
	 * Diceva `Action.Activate` fino a `#199`, che l'ha ritirata: [D-014] la dichiarava «assorbita
	 * semanticamente da `Interact`» e [D-025] lo ha confermato. Lo Stable ID resta interpretabile in lettura
	 * (`URTCatalogLibrary::ResolveLegacyActionId`), ma non e' piu' il nome di un'azione che qualcuno userera'.
	 */
	SetDoorState,

	/**
	 * Sposta **chi agisce**, non il bersaglio: la sorgente arretra di `Amount` celle allontanandosi da chi
	 * l'ha innescata (D-093, CP 7.5).
	 *
	 * Esiste perche' `Push` e `Pull` muovono il `TargetUnitId` e **nessun effetto muoveva la sorgente** — una
	 * invariante di fatto del registry, mai decisa, che teneva fuori dal gioco `Reaction.EmergencyDash` e
	 * `Reaction.HazardEscape`, i cui esiti sono entrambi «chi reagisce si sposta».
	 *
	 * ⚠️ **In coda all'enum, mai in mezzo**: i valori entrano nel TurnLog serializzato, e inserirlo prima di
	 * `SetDoorState` rinumererebbe una traccia gia' scritta.
	 *
	 * La direzione non e' un parametro: e' la linea da chi ha innescato verso chi reagisce, la stessa che
	 * `URTHexCombatLibrary::HexKnockbackDestination` gia' calcola per le spinte — quindi si ferma davanti
	 * agli stessi ostacoli, e non serve una seconda geometria dello spostamento.
	 */
	SelfReposition,

	/**
	 * ANNULLA lo spostamento forzato che sta per colpire chi reagisce (`Reaction.Anchor`, CP 7.5 `#505`).
	 *
	 * «Annulla» e non «riduce»: il catalogo equipaggiamento §3 dice «impedisce **una** spinta», che e' un
	 * conteggio e non una soglia (D-094). E' la differenza con i due meccanismi vicini, che restano distinti:
	 * `Status.Guarded` regge una spinta fino a `GuardResistedPushDistance` e cede sopra, `Status.Braced` regge
	 * qualunque distanza ma non e' una reazione — nessuno dei due si consuma come attivazione.
	 *
	 * «Una» non ha bisogno di un contatore proprio: la garantisce il pass, che attiva al piu' una reazione per
	 * unita' e per turno. Un secondo conteggio direbbe la stessa cosa in un secondo posto, e i due potrebbero
	 * divergere.
	 *
	 * `Amount` e' ignorato: non c'e' un «quanto» in un annullamento. Chi lo consuma sono i rami di spinta e di
	 * trazione di `ARTTurnManager::ResolveCombat`, che lo leggono come un quinto `ERTDisplacementBlockReason`
	 * accanto a `Guarded`/`Braced`/`NoDestination`/`OpposingForces` — cosi' il replay dice QUALE difesa ha
	 * retto, invece del solo «non si e' mosso».
	 *
	 * ⚠️ In coda all'enum, mai in mezzo: vale la stessa ragione di `SelfReposition` qui sopra.
	 */
	CancelDisplacement
};

/**
 * Un effetto DICHIARATO da un'azione del catalogo: cosa fa e quanto.
 *
 * Un'azione ne dichiara una LISTA, perche' molte ne combinano piu' d'uno: la Spazzata del Guardian infligge
 * danno **e** respinge; `Riva.PressureJet` del catalogo v0.1 fa danno, bagna e spinge. Con un solo effetto per
 * azione, la seconda meta' finirebbe di nuovo in un flag hard-coded — cioe' il problema che il motore azioni
 * esiste per togliere.
 */
USTRUCT(BlueprintType)
struct FRTActionEffectSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Actions")
	ERTActionEffect Effect = ERTActionEffect::None;

	/** Entita': danni, cura, punti scudo, celle di spinta. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Actions")
	int32 Amount = 0;

	/** Stato applicato (solo con `Effect == Status`). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Actions")
	FGameplayTag StatusTag;

	/** Durata in turni dello stato (solo con `Effect == Status`). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Actions")
	int32 StatusDuration = 0;

	FRTActionEffectSpec() = default;
	FRTActionEffectSpec(ERTActionEffect InEffect, int32 InAmount)
		: Effect(InEffect), Amount(InAmount) {}
	FRTActionEffectSpec(ERTActionEffect InEffect, FGameplayTag InTag, int32 InTurns)
		: Effect(InEffect), StatusTag(InTag), StatusDuration(InTurns) {}
};

/**
 * Un effetto GIA' RISOLTO, pronto da applicare. Le azioni non mutano lo stato: **producono eventi**, che
 * vengono applicati insieme sullo snapshot iniziale della fase (invariante #3, "raccogli poi applica").
 *
 * Solo interi (invariante #4). Riferimento: docs/gameplay/spec-motore-azioni-e4.md §3.
 */
USTRUCT(BlueprintType)
struct FRTActionEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Actions")
	ERTActionEffect Kind = ERTActionEffect::None;

	/** Chi subisce l'effetto (indice stabile nello snapshot; INDEX_NONE = nessuno). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Actions")
	int32 TargetUnitId = INDEX_NONE;

	/** Chi lo produce: serve alla direzione della spinta e all'osservabilita' del TurnLog. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Actions")
	int32 SourceUnitId = INDEX_NONE;

	/** Cella coinvolta (spinta, effetti sul terreno). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Actions")
	FRTCellId Cell;

	/** Entita' dell'effetto: danni, cura, punti scudo, celle di spinta, turni di durata. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Actions")
	int32 Amount = 0;

	/** Stato applicato (solo con `Kind == Status`). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Actions")
	FGameplayTag StatusTag;

	FRTActionEvent() = default;
};
