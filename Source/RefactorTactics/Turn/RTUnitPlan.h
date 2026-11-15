#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Map/RTCellId.h"
#include "Turn/Actions/RTDeclaredCondition.h"
#include "RTUnitPlan.generated.h"

class ARTUnit;

/**
 * IL PIANO DI UN'UNITA' PER IL TURNO: SOLO L'INTENTO ([D-608] (4); E50 fetta 2, tappa 3, #4123).
 *
 * E' la sorgente unica del piano ([D-608] (1)): controller, harness e bot lo scrivono attraverso i mutatori di
 * `ARTUnit`, che lo tiene in un membro privato e lo espone in sola lettura (`ARTUnit::GetPlan()`). Le altre forme di
 * piano — `FRTIntentView` per la privacy, `FRTPlannedAction` per il validatore, le anteprime, la decisione del bot —
 * restano tipi distinti e sono proiezioni.
 *
 * 🔑 **La forma e' quella di oggi** ([D-608] (5)): nomi, tipi e default sono i campi che stavano in `ARTUnit`, spostati
 * senza cambiarli, con la loro documentazione. Rinominarli o cambiarne la forma e' lavoro delle tappe che seguono (la
 * lista di E38, CP 38.12), non di questo spostamento.
 *
 * ⛔ **Cosa NON c'e', ed e' deciso**: la mira congelata (`PlannedAimCell`) va nella copia immutabile al lock-in con
 * `StableUnitId` (tappa 4); la cache della carica resta derivata; la memoria del Back va nella sessione di
 * pianificazione (S6). Il bersaglio resta un puntatore: l'id compare solo nella copia al lock-in ([D-608] (2)).
 *
 * 🔒 **Nessun campo e' visibile ai Blueprint**: l'intento e' privato della squadra che lo dichiara (CLAUDE.md §7), e
 * nessun asset del progetto li nominava (misura in #4123).
 */
USTRUCT()
struct REFACTORTACTICS_API FRTUnitPlan
{
	GENERATED_BODY()

	/** Cella di destinazione pianificata per il turno corrente (default = cella attuale). */
	UPROPERTY()
	FRTCellId PlannedCell;

	/** Abilita' pianificata per il turno (INDEX_NONE = nessun attacco). */
	UPROPERTY()
	int32 PlannedAbilityIndex = INDEX_NONE;

	/** Bersaglio dell'attacco pianificato per il turno (nullo = nessun attacco). */
	UPROPERTY()
	TObjectPtr<ARTUnit> PlannedAttackTarget = nullptr;

	/**
	 * Cella bersagliata dall'azione principale, in alternativa a `PlannedAttackTarget`: le aree si centrano
	 * su una CELLA, che puo' essere vuota (`Gadget.Overload` su un varco, una cella conduttiva senza nessuno
	 * sopra). Valida solo con `bAttackTargetsCell`.
	 *
	 * Chiude a meta' il limite dichiarato in `RTTurnManager` (CP 8.3): la pianificazione non aveva un
	 * bersaglio-cella. Resta scoperto il lato HUD — puntare una cella col mouse e' E11.
	 */
	UPROPERTY()
	FRTCellId PlannedAttackCell;

	/**
	 * Bordo bersagliato dalle azioni che agiscono su una STRUTTURA di bordo (CP 9.5: `Action.CreateCover`,
	 * `Riktor.Reconfigure`). Valido solo con `bHasPlannedCoverEdge`.
	 *
	 * Serve un dato in piu' perche' una copertura sta su un BORDO, e una cella ne ha sei: con portata 3 la
	 * coppia (chi la erige, cella bersaglio) non basta a determinarlo — a portata 1 sarebbe bastata, ma il
	 * catalogo azioni dichiara 3. E' l'equivalente per i bordi di `PlannedAttackCell`, e come quello resta
	 * scoperto sul lato HUD: puntare un bordo col mouse e' E11.
	 */
	UPROPERTY()
	ERTHexDirection PlannedCoverEdge = ERTHexDirection::E;

	/**
	 * Vero se il piano dichiara `PlannedCoverEdge`. Serve un flag e non basta un valore di riposo: le sei
	 * direzioni sono tutte legittime, quindi nessuna puo' fare da «nessuna» senza diventare un caso speciale
	 * che il giocatore non puo' dedurre.
	 */
	UPROPERTY()
	bool bHasPlannedCoverEdge = false;

	/**
	 * Vero se l'azione principale mira a `PlannedAttackCell` invece che a un'unita'.
	 *
	 * Serve un flag e non basta «target nullo»: nel resolver `TargetUnitId == INDEX_NONE` significa gia'
	 * **bersaglio perso** (eliminato o mai valido), che degrada al fallback. Senza distinguerle, mirare a una
	 * cella verrebbe letto come un errore di pianificazione.
	 *
	 * ⛔ **NON scriverlo a mano dal codice di gioco: usa `DeclareAttackOnCell` / `DeclareAttackOnUnit` /
	 * `ClearPlannedAttack`** (`#2884`). Questo campo e `PlannedAttackTarget` sono **mutuamente esclusivi**, e
	 * finche' l'esclusivita' e' stata una convenzione invece che una funzione nessuno l'ha rispettata:
	 * `HandleTargetCell` accendeva il flag e `HandleClickOnUnit` scriveva il bersaglio **senza spegnerlo**.
	 */
	UPROPERTY()
	bool bAttackTargetsCell = false;

	/**
	 * Percorso composito pianificato (waypoint risolti in celle, From = Cell incluso).
	 * Vuoto o < 2 celle = nessun movimento composito (si usa PlannedCell come destinazione singola).
	 */
	UPROPERTY()
	TArray<FRTCellId> PlannedPath;

	/**
	 * Waypoint cliccati dal giocatore (esclusa la cella di partenza), da cui deriva PlannedPath.
	 * Vive sull'unita' cosi' la riselezione ne preserva l'editing (aggiungi/annulla step).
	 */
	UPROPERTY()
	TArray<FRTCellId> PlannedWaypoints;

	/**
	 * Il profilo di movimento **DICHIARATO** per questo turno — oggi `MovementProfile.Sneak`, o vuoto
	 * (`#1410`, [D-425]). Vive accanto ai waypoint perche' come loro si edita fino al lock-in.
	 *
	 * ⏱️ *Fino al 2026-09-15 questo campo portava l'ANDATURA, scelta ciclando fra quattro etichette.*
	 * [D-425] ha rovesciato la derivazione: `Move` e `Sprint` si **leggono** da quanto si e' pianificato e
	 * non si dichiarano affatto, `Still` e' l'assenza di piano, e il `Withdraw` lo **impone** l'`Overwatch`
	 * e si legge dal piano (`ReservedProfileForPlan`), non da qui.
	 *
	 * 🔑 **Cio' che resta qui e' un TETTO, non un'etichetta** ([D-425] punto (2)): dichiarare `Sneak`
	 * dimezza la distanza pianificabile. E' l'unica cosa che il giocatore dichiara sullo slot movimento, ed
	 * e' il motivo per cui il campo sopravvive al selettore che lo riempiva.
	 *
	 * 🔑 **`NAME_None` significa «nessuna dichiarazione», e lo traduce chi legge.** Il campo nasce vuoto —
	 * e' il default di un `FName`, e un'unita' deserializzata da un salvataggio precedente lo troverebbe
	 * vuoto comunque — quindi la traduzione sta in `CeilingProfile`, in un punto solo: vuoto ⇒ il tetto
	 * nudo, `2×`.
	 *
	 * ⛔ **`Still`, `Move`, `Sprint` e `Withdraw` NON si scrivono qui.** I primi tre sono derivati e il
	 * quarto e' imposto: scriverne uno darebbe due modi di dire la stessa cosa — questo campo e la sua
	 * derivazione — che possono contraddirsi.
	 *
	 * ⛔ **Non e' replicato, come `PlannedWaypoints` e `PlannedCell` qui sopra, e non e' un dettaglio.**
	 * La dichiarazione e' intento di pianificazione: dire a un avversario *«questo sguscia»* prima del
	 * lock-in gli anticipa la distanza che quell'unita' puo' coprire, cioe' esattamente l'informazione che
	 * la simultaneita' esiste per nascondere.
	 */
	UPROPERTY()
	FName PlannedMovementProfileId;

	/**
	 * Rotazione DICHIARATA in pianificazione (D-020, #291): «finito il movimento, guarda di la'».
	 *
	 * Il flag separato dal valore ha la stessa ragione di `bAttackTargetsCell`: `E` e' una direzione legittima
	 * e non puo' fare da «non dichiarato». Il resolver la consuma a fine Move — dopo l'orientamento derivato,
	 * che e' il `Current` su cui si misura la legalita' — e la azzera: una dichiarazione vale per il turno in
	 * cui e' stata fatta, come ogni altro pezzo del piano.
	 */
	UPROPERTY()
	bool bDeclaresPlannedFacing = false;

	UPROPERTY()
	ERTHexDirection PlannedFacing = ERTHexDirection::E;

	/** Abilita' di scatto pianificata per il turno (INDEX_NONE = nessuno scatto). Si risolve in fase Dash. */
	UPROPERTY()
	int32 PlannedDashAbility = INDEX_NONE;

	/** Cella di destinazione dello scatto pianificato (valida solo se PlannedDashAbility e' impostata). */
	UPROPERTY()
	FRTCellId PlannedDashCell;

	/**
	 * Reazione pianificata per il turno (INDEX_NONE = nessuna reazione pronta), slot indipendente da
	 * Movimento e Principale (CP 5.1, epic E5). Non ha una cella o un bersaglio: il trigger dichiarato
	 * dall'abilita' (`FRTActionDef::ReactionTrigger`) e' valutato sullo snapshot del Blast, non scelto qui.
	 */
	UPROPERTY()
	int32 PlannedReactionAbility = INDEX_NONE;

	/**
	 * La condizione dichiarata sulla reazione armata ([D-109]). Vuota = nessuna: si risponde a ogni trigger.
	 *
	 * Si scrive SOLO da `SetPlannedReactionCondition`, che la valida: un campo pubblico scrivibile a mano
	 * rimetterebbe in gioco proprio cio' che il validator esiste per impedire — una condizione che nessuna
	 * funzione sa valutare, scoperta al trigger invece che in pianificazione.
	 */
	UPROPERTY()
	FRTDeclaredCondition PlannedReactionCondition;

	/**
	 * Ordine di rimozione dichiarato per `Action.Cleanse` (CP 5.2): si purifica il PRIMO stato di questa lista
	 * che l'unita' possiede davvero, e uno solo.
	 *
	 * La scelta e' del giocatore ed e' fatta in PIANIFICAZIONE, mai a runtime: il catalogo lo pretende
	 * esplicitamente ("nessuna scelta implicita"), perche' un'euristica automatica ("togli il piu' dannoso")
	 * renderebbe l'esito impredicibile — e la predizione e' un pilastro di prodotto. Lista vuota = nessuna
	 * scelta dichiarata = nessuna rimozione (fail-closed): il Cleanse risolve senza effetto e lo registra,
	 * invece di indovinare per conto del giocatore.
	 */
	UPROPERTY()
	TArray<FGameplayTag> PlannedCleansePriority;
};
