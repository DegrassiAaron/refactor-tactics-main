#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"       // FGameplayTag: gli stati che i colpi applicano
#include "Map/RTCellId.h"
#include "Ability/RTActionDef.h"        // FRTActionDef: la definizione che ha prodotto un intento
#include "Combat/RTCombatResolver.h"    // FRTUnitCombatState
#include "Combat/RTHexCombatLibrary.h"  // FRTHexCombatUnit, FRTHexAttackIntent, FRTHexBlastPlan
#include "Turn/RTReactionPassResult.h"  // FRTReactionPassResult: l'esito del pass reazioni sopravvive al pass

class ARTUnit;
class URTActionData;
class URTHexMapAsset;

/**
 * Modifica di un ARCO richiesta durante il Blast (CP 9.4), in attesa di essere applicata a fase conclusa.
 *
 * `Actor` viaggia con l'operazione, e non e' ridondante: `PendingArcOps` viene RIORDINATO prima
 * dell'applicazione — due unita' che agiscono sullo stesso ponte devono dare lo stesso esito in qualunque
 * ordine (invariante #3) — quindi dopo il sort l'indice non e' piu' quello dell'unita', e il TurnLog deve
 * comunque dire chi ha agito (#405).
 */
struct FRTPendingArcOp
{
	FRTCellId From;
	FRTCellId To;
	ARTUnit* Actor = nullptr;

	/**
	 * QUALE azione ha modificato l'arco.
	 *
	 * 🔴 Non si ricostruisce a valle scrivendo `Action.ModifyArc` a mano: `MakeEquipmentAction` riscrive
	 * `ActionId` con l'id del PEZZO ([D-195]), e da `#1443` il resolver riconosce anche le azioni derivate.
	 * Senza questo campo un gadget si leggeva col proprio nome quando FALLIVA — `ArcRejected` copia la def —
	 * e con quello generico quando riusciva: lo stesso pezzo con due nomi a seconda dell'esito, e `ActionId`
	 * entra nell'hash ([D-067]).
	 */
	FRTActionDef Def;
};

/**
 * Stato vivo di UNA risoluzione della fase Blast, dalla raccolta delle unita' fino agli effetti finali.
 *
 * Perche' esiste: `ARTTurnManager::ResolveCombat` risolve il Blast in una sequenza di pass che condividono
 * una cinquantina di valori — chi c'e' in campo, con quale indice, quali intenti, quale piano. Finche' erano
 * variabili locali di una funzione sola, quella condivisione era invisibile: leggendo un pass non si poteva
 * sapere che cosa il pass prima gli avesse lasciato, e l'unico modo per spostarne uno era leggere le altre
 * millesettecento righe. Il contesto rende quella dipendenza DICHIARATA, e quindi verificabile.
 *
 * Che cosa NON e': non e' stato del turno e non sopravvive alla fase. Nasce all'inizio di `ResolveCombat` e
 * muore alla fine — cio' che deve durare oltre il Blast sta nei membri del manager (`TurnLog`,
 * `TeamKnowledgeState`, `ReactionBlockedThisTurn`), non qui. Un campo che qualcuno volesse leggere nel turno
 * successivo e' un campo che sta nel posto sbagliato.
 *
 * Invariante di identita': `UnitId` e' SEMPRE l'indice in `Units`, e `Units` e' ordinato per cella
 * (`URTHexLibrary::StableLess`) una volta sola, all'inizio. Da quell'ordine dipendono gli indici del piano,
 * il TurnLog e la sequenza del playback: chi riordina `Units` a fase iniziata rompe il replay, non solo
 * questa struttura. Le identita' che devono sopravvivere all'ordinamento usano `ARTUnit::StableUnitId`
 * ([D-063]), mai l'indice.
 */
struct FRTBlastContext
{
	// --- Geometria autorevole della fase: letta una volta, mai riscritta ---------------------------

	/** Mappa esagonale su cui si valutano portata e linea di tiro. `nullptr` = nessuna mappa autorevole. */
	const URTHexMapAsset* Map = nullptr;

	FVector HexOrigin = FVector::ZeroVector;
	float HexSize = 200.f;
	float HexLayerH = 0.f;

	// --- Chi e' in campo, e con quale identita' ----------------------------------------------------

	/** Unita' del match ordinate per cella. L'indice in questo array E' l'`UnitId` di tutta la fase. */
	TArray<ARTUnit*> Units;

	/** Inverso di `Units`: dall'attore al suo indice. Serve a tradurre i bersagli pianificati in `UnitId`. */
	TMap<ARTUnit*, int32> IndexOf;

	/** Salute e scudo all'inizio del Blast, paralleli a `Units`: la base su cui il resolver applica i danni. */
	TArray<FRTUnitCombatState> States;

	/**
	 * Copia posizionale usata dalla geometria e dalla difesa direzionale (CP 16.2). E' una FOTOGRAFIA presa
	 * prima degli intenti: quando un'unita' si riorienta verso il bersaglio, il suo `Facing` va aggiornato
	 * anche qui, altrimenti due unita' che si attaccano a vicenda risultano entrambe colpite alle spalle.
	 */
	TArray<FRTHexCombatUnit> HexUnits;

	// --- Cure: raccolte prima degli intenti, applicate dopo i danni --------------------------------
	//
	// Quattro array paralleli invece di una struct: e' la forma che il TurnLog consuma, e cambiarla qui
	// vorrebbe dire cambiare anche chi la legge. Restano paralleli, e `AddHeal` e' l'unico punto che li
	// riempie — cosi' non possono divergere di lunghezza.

	TArray<ARTUnit*> HealTargets;
	TArray<int32> HealAmounts;
	TArray<FRTCellId> HealSources;

	/** Chi cura: una cella non identifica un'unita' ([D-063]), e il TurnLog deve dire chi ha agito (#405). */
	TArray<ARTUnit*> HealActors;

	/**
	 * QUALE azione ha curato.
	 *
	 * 🔴 Non si ricostruisce a valle, e la voce non puo' scrivere `Action.Heal` a mano: `MakeEquipmentAction`
	 * riscrive `ActionId` con l'id del PEZZO ([D-195]), quindi una cura da `Gadget.Medkit` deve leggersi
	 * come tale. Fino a `#1443` la voce di successo diceva `Action.Heal` mentre quelle di FALLIMENTO —
	 * scritte nello stesso file, dallo stesso piano — dicevano `Gadget.Medkit`: lo stesso gadget con due
	 * nomi a seconda che avesse funzionato. E `ActionId` entra nell'hash ([D-067]), quindi la traccia
	 * archiviata era autoritativa e sbagliata.
	 */
	TArray<FRTActionDef> HealDefs;

	/** Registra una cura pianificata mantenendo allineati i cinque array. */
	void AddHeal(ARTUnit* Actor, ARTUnit* Target, int32 Amount, const FRTCellId& SourceCell,
		const FRTActionDef& Def)
	{
		HealActors.Add(Actor);
		HealTargets.Add(Target);
		HealAmounts.Add(Amount);
		HealSources.Add(SourceCell);
		HealDefs.Add(Def);
	}

	// --- Intenti d'attacco raccolti dai piani, prima che diventino colpi --------------------------
	//
	// Qui si valida l'ABILITA' (esiste, non e' uno scatto, e' utilizzabile); la GEOMETRIA (portata, linea di
	// tiro, celle colpite) la valida `URTHexCombatLibrary`. I quattro array sono paralleli e indicizzati
	// dall'`IntentIndex` che il piano riporta su ogni colpo.

	TArray<FRTHexAttackIntent> Intents;
	TArray<int32> IntentAbilityIndex;
	TArray<const URTActionData*> IntentAbility;

	/**
	 * La definizione che ha prodotto l'intento. Esiste separata da `IntentAbility` perche' un intento puo'
	 * non avere un `URTActionData` dietro: l'impatto di una carica e' dati puri, non un'abilita' selezionata.
	 */
	TArray<FRTActionDef> IntentDefs;

	/** Operazioni sugli archi raccolte nella fase, applicate a fase CONCLUSA come i colpi e il danno alle strutture. */
	TArray<FRTPendingArcOp> PendingArcOps;

	/** Colpi che la geometria ha validato: l'esito di `CollectHexAttacks` sugli intenti raccolti. */
	FRTHexBlastPlan Plan;

	/**
	 * Cio' che il pass delle reazioni ha raccolto sui colpi del Blast (CP 5.1).
	 *
	 * Vive nel contesto e non nel pass che lo produce perche' viene consumato molto piu' tardi, in due punti
	 * distinti: la riduzione del danno entra nel delta del PRIMO colpo, i contrattacchi si accodano ai colpi
	 * veri quando il danno si applica. Fra la raccolta e i due usi passa l'intera catena di marchi, bagnato e
	 * risoluzione: e' esattamente il genere di distanza che questa struct esiste per rendere visibile.
	 */
	FRTReactionPassResult Reactions;

	// --- Spostamenti forzati, raccolti col danno e applicati dopo ---------------------------------
	//
	// Mappe per ATTORE e non array per indice: un bersaglio puo' essere spinto da piu' attaccanti nello
	// stesso Blast, e la spinta risultante si accumula su di lui — non su una riga per colpo. `KnockCount`
	// dice da quanti, e serve a distinguere una spinta sola da due che si sommano.
	//
	// Spinta e trazione tengono mappe SEPARATE, e non e' ridondanza: con entrambe sullo stesso bersaglio una
	// mappa condivisa vedrebbe la seconda `Add` sovrascrivere la prima, e la voce dichiarerebbe l'attaccante
	// sbagliato. E' il difetto vero che ha prodotto `FRTDisplacementCause`, ed e' la ragione per cui i campi
	// stanno insieme dentro quella struct invece che in tre mappe parallele.

	TMap<ARTUnit*, FRTCellId> KnockFrom;
	TMap<ARTUnit*, int32> KnockDist;
	TMap<ARTUnit*, int32> KnockCount;

	TMap<ARTUnit*, FRTCellId> PullToward;
	TMap<ARTUnit*, int32> PullDist;
	TMap<ARTUnit*, int32> PullCount;

	/** Che cosa ha prodotto la spinta e la trazione, per il TurnLog (`#307`). */
	TMap<ARTUnit*, FRTDisplacementCause> PushCause;
	TMap<ARTUnit*, FRTDisplacementCause> PullCause;

	// --- Chi ha colpito, e con quale abilita' ------------------------------------------------------

	/** Attaccanti sopravvissuti al Blast, paralleli a `UsedAbilityIndex`: l'abilita' si spende a fase finita. */
	// ⚠️ **Chi ha COLPITO**, e solo quello: un'unita' che ha speso un'azione senza lasciare un colpo — un
	// `Action.Interrupt`, i cui colpi vengono tolti tutti — NON entra qui, e paga la propria azione dove
	// quell'azione vive (`#1444`). Chi legge questo array puo' contare su un `FRTAttack` corrispondente.
	TArray<ARTUnit*> Attackers;
	TArray<int32> UsedAbilityIndex;

	// --- Le azioni pianificate che sono PARTITE, e che percio' si pagano ---------------------------
	//
	// 🔴 **Un solo posto scrive il cooldown di un'azione pianificata del Blast** (`#1451` punto 3).
	// Prima ne scrivevano cinque — `ResolveCleanseActions`, `CollectHealActions`, `ModifyArc` dentro
	// `CollectAttackIntents`, `ApplyInterrupts`, `MarkAttackerAbilitiesSpent` — e ognuno decideva da se' che
	// cosa significasse «spesa». Ogni azione nuova che potesse validarsi senza colpire ne voleva un sesto.
	//
	// ⚠️ **Qui non si decide, si annota.** Il criterio «l'azione e' PARTITA» resta a chi raccoglie, e deve:
	// e' l'unico che sa cosa puo' sapere in quel momento. [D-200] lo scrive per la portata — l'unico modo di
	// fallire noto in pianificazione — e ogni punto lo applica con cio' che ha in mano. Cio' che era
	// duplicato e non doveva esserlo e' il GESTO di pagare.
	//
	// ⚠️ **E le guardie di vita restano al momento dell'annotazione, non qui**: chi cura e' vivo all'inizio
	// del Blast, chi attacca deve esserlo alla FINE (`Attackers` raccoglie i sopravvissuti). Centralizzare
	// un `IsAlive()` in fondo cambierebbe il gioco per il curatore che cade a meta' fase — e sarebbe un
	// cambio di comportamento travestito da pulizia.
	TArray<ARTUnit*> SpentActors;
	TArray<int32> SpentAbilityIndex;

	/** Annota un'azione pianificata PARTITA. La spende `ARTTurnManager::SpendStartedAbilities`. */
	void MarkAbilitySpent(ARTUnit* Actor, int32 AbilityIndex)
	{
		SpentActors.Add(Actor);
		SpentAbilityIndex.Add(AbilityIndex);
	}

	// --- Stati applicati dai colpi, a bersagli sopravvissuti --------------------------------------
	//
	// Si applicano in fondo alla fase e non al momento del colpo: un'unita' che cade non riceve lo stato, e
	// chi ha annullato il controllo con una reazione va consultato prima — cosa che qui e' possibile perche'
	// la lista completa dei controlli in arrivo esiste gia'.

	TArray<ARTUnit*> StatusTargets;
	TArray<FGameplayTag> StatusTags;
	TArray<int32> StatusDurations;

	/** Numero di unita' in campo. Comodita' di lettura: `Ctx.Num()` invece di `Ctx.Units.Num()`. */
	int32 Num() const { return Units.Num(); }
};
