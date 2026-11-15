#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Map/RTCellId.h"
#include "Turn/Actions/RTDeclaredCondition.h"
#include "Turn/RTPlanEntry.h"
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
USTRUCT(meta = (RTServerOnly))
struct REFACTORTACTICS_API FRTUnitPlan
{
	GENERATED_BODY()

	// ---- La lista (E38 CP 38.12, #4113) ------------------------------------------------------------------------------
	//
	// 🔑 **Il piano e' una lista di voci, e la POSIZIONE di una voce e' il suo ordine di dichiarazione** ([D-608] (1),
	// [D-615] (5)). L'ordine si mostra e non decide: nessun resolver lo legge per ordinare o per scegliere un esito, e
	// `MakePlanFor` deriva la banda del movimento da tutte le altre voci, qualunque sia la loro posizione.
	//
	// 🔑 **Una voce per classe** (scatto, principale, reazione, movimento), finche' il resolver ne legge una: sono i campi
	// a casella unica di prima, e la lista ha solo le forme che quei campi sapevano dire. E' una regola sulla FORMA del
	// piano, non sull'esito. `AddEntry` rifiuta una seconda voce della stessa classe; le porte qui sotto, che sono i
	// gesti di prima, SOSTITUISCONO il valore nella voce della loro classe.
	//
	// 🔑 **Gli accessori dicono cio' che dicevano i campi**: il valore della voce della classe, o il default se la voce
	// non c'e'. Una voce nasce alla prima scrittura che le da' un valore e si toglie quando torna tutta al default, cosi'
	// la lista non porta voci vuote e i valori letti sono quelli di prima per costruzione.

	/** Le voci, nell'ordine di dichiarazione. */
	const TArray<FRTPlanEntry>& GetEntries() const { return Entries; }

	/** La posizione della voce di una classe, cioe' il suo ordine di dichiarazione; `INDEX_NONE` se non c'e'. */
	int32 DeclarationIndexOf(ERTPlanEntryClass Class) const
	{
		return Entries.IndexOfByPredicate([Class](const FRTPlanEntry& E) { return E.Class == Class; });
	}

	/** Aggiunge una voce in coda. Rifiuta, e lascia il piano com'era, una voce vuota o una seconda voce della stessa classe. */
	bool AddEntry(const FRTPlanEntry& Entry)
	{
		if (Entry.IsEmpty() || DeclarationIndexOf(Entry.Class) != INDEX_NONE)
		{
			return false;
		}
		Entries.Add(Entry);
		return true;
	}

	/** Toglie la voce in quella posizione; le altre restano nel loro ordine. */
	bool RemoveEntryAt(int32 Position)
	{
		if (!Entries.IsValidIndex(Position))
		{
			return false;
		}
		Entries.RemoveAt(Position);
		return true;
	}

	/** Svuota la lista: nessuna voce, quindi ogni accessore della lista torna al default. */
	void ResetEntries() { Entries.Reset(); }

	// ---- Le scritture di un campo della voce: le chiamano solo i mutatori di `ARTUnit` e l'accesso dei test ----------
	void SetPlannedAbilityIndex(int32 V) { Edit(ERTPlanEntryClass::Main).AbilityIndex = V; Prune(); }
	void SetPlannedAttackTarget(ARTUnit* V) { Edit(ERTPlanEntryClass::Main).AttackTarget = V; Prune(); }
	void SetPlannedAttackCell(const FRTCellId& V) { Edit(ERTPlanEntryClass::Main).AttackCell = V; Prune(); }
	void SetAttackTargetsCell(bool V) { Edit(ERTPlanEntryClass::Main).bAttackTargetsCell = V; Prune(); }
	void SetPlannedCoverEdge(ERTHexDirection V) { Edit(ERTPlanEntryClass::Main).CoverEdge = V; Prune(); }
	void SetHasPlannedCoverEdge(bool V) { Edit(ERTPlanEntryClass::Main).bHasCoverEdge = V; Prune(); }
	void SetPlannedDashAbility(int32 V) { Edit(ERTPlanEntryClass::Dash).AbilityIndex = V; Prune(); }
	void SetPlannedDashCell(const FRTCellId& V) { Edit(ERTPlanEntryClass::Dash).DashCell = V; Prune(); }
	void SetPlannedReactionAbility(int32 V) { Edit(ERTPlanEntryClass::Reaction).AbilityIndex = V; Prune(); }
	void SetPlannedReactionCondition(const FRTDeclaredCondition& V) { Edit(ERTPlanEntryClass::Reaction).ReactionCondition = V; Prune(); }
	void SetPlannedPath(const TArray<FRTCellId>& V) { Edit(ERTPlanEntryClass::Movement).Path = V; Prune(); }
	void AddPlannedPathCell(const FRTCellId& V) { Edit(ERTPlanEntryClass::Movement).Path.Add(V); Prune(); }
	void SetPlannedWaypoints(const TArray<FRTCellId>& V) { Edit(ERTPlanEntryClass::Movement).Waypoints = V; Prune(); }
	void AddPlannedWaypoint(const FRTCellId& V) { Edit(ERTPlanEntryClass::Movement).Waypoints.Add(V); Prune(); }
	void PopPlannedWaypoint() { Edit(ERTPlanEntryClass::Movement).Waypoints.Pop(); Prune(); }
	void SetPlannedMovementProfileId(FName V) { Edit(ERTPlanEntryClass::Movement).MovementProfileId = V; Prune(); }

	/** Cella di destinazione pianificata per il turno corrente (default = cella attuale). */
	UPROPERTY()
	FRTCellId PlannedCell;

	/** Abilita' pianificata per il turno (INDEX_NONE = nessun attacco). */
	int32 PlannedAbilityIndex() const { return Of(ERTPlanEntryClass::Main).AbilityIndex; }

	/** Bersaglio dell'attacco pianificato per il turno (nullo = nessun attacco). */
	const TObjectPtr<ARTUnit>& PlannedAttackTarget() const { return Of(ERTPlanEntryClass::Main).AttackTarget; }

	/**
	 * Cella bersagliata dall'azione principale, in alternativa a `PlannedAttackTarget`: le aree si centrano
	 * su una CELLA, che puo' essere vuota (`Gadget.Overload` su un varco, una cella conduttiva senza nessuno
	 * sopra). Valida solo con `bAttackTargetsCell`.
	 *
	 * Chiude a meta' il limite dichiarato in `RTTurnManager` (CP 8.3): la pianificazione non aveva un
	 * bersaglio-cella. Resta scoperto il lato HUD — puntare una cella col mouse e' E11.
	 */
	const FRTCellId& PlannedAttackCell() const { return Of(ERTPlanEntryClass::Main).AttackCell; }

	/**
	 * Bordo bersagliato dalle azioni che agiscono su una STRUTTURA di bordo (CP 9.5: `Action.CreateCover`,
	 * `Riktor.Reconfigure`). Valido solo con `bHasPlannedCoverEdge`.
	 *
	 * Serve un dato in piu' perche' una copertura sta su un BORDO, e una cella ne ha sei: con portata 3 la
	 * coppia (chi la erige, cella bersaglio) non basta a determinarlo — a portata 1 sarebbe bastata, ma il
	 * catalogo azioni dichiara 3. E' l'equivalente per i bordi di `PlannedAttackCell`, e come quello resta
	 * scoperto sul lato HUD: puntare un bordo col mouse e' E11.
	 */
	ERTHexDirection PlannedCoverEdge() const { return Of(ERTPlanEntryClass::Main).CoverEdge; }

	/**
	 * Vero se il piano dichiara `PlannedCoverEdge`. Serve un flag e non basta un valore di riposo: le sei
	 * direzioni sono tutte legittime, quindi nessuna puo' fare da «nessuna» senza diventare un caso speciale
	 * che il giocatore non puo' dedurre.
	 */
	bool bHasPlannedCoverEdge() const { return Of(ERTPlanEntryClass::Main).bHasCoverEdge; }

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
	bool bAttackTargetsCell() const { return Of(ERTPlanEntryClass::Main).bAttackTargetsCell; }

	/**
	 * Percorso composito pianificato (waypoint risolti in celle, From = Cell incluso).
	 * Vuoto o < 2 celle = nessun movimento composito (si usa PlannedCell come destinazione singola).
	 */
	const TArray<FRTCellId>& PlannedPath() const { return Of(ERTPlanEntryClass::Movement).Path; }

	/**
	 * Waypoint cliccati dal giocatore (esclusa la cella di partenza), da cui deriva PlannedPath.
	 * Vive sull'unita' cosi' la riselezione ne preserva l'editing (aggiungi/annulla step).
	 */
	const TArray<FRTCellId>& PlannedWaypoints() const { return Of(ERTPlanEntryClass::Movement).Waypoints; }

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
	FName PlannedMovementProfileId() const { return Of(ERTPlanEntryClass::Movement).MovementProfileId; }

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
	int32 PlannedDashAbility() const { return Of(ERTPlanEntryClass::Dash).AbilityIndex; }

	/** Cella di destinazione dello scatto pianificato (valida solo se PlannedDashAbility e' impostata). */
	const FRTCellId& PlannedDashCell() const { return Of(ERTPlanEntryClass::Dash).DashCell; }

	/**
	 * Reazione pianificata per il turno (INDEX_NONE = nessuna reazione pronta), slot indipendente da
	 * Movimento e Principale (CP 5.1, epic E5). Non ha una cella o un bersaglio: il trigger dichiarato
	 * dall'abilita' (`FRTActionDef::ReactionTrigger`) e' valutato sullo snapshot del Blast, non scelto qui.
	 */
	int32 PlannedReactionAbility() const { return Of(ERTPlanEntryClass::Reaction).AbilityIndex; }

	/**
	 * La condizione dichiarata sulla reazione armata ([D-109]). Vuota = nessuna: si risponde a ogni trigger.
	 *
	 * Si scrive SOLO da `SetPlannedReactionCondition`, che la valida: un campo pubblico scrivibile a mano
	 * rimetterebbe in gioco proprio cio' che il validator esiste per impedire — una condizione che nessuna
	 * funzione sa valutare, scoperta al trigger invece che in pianificazione.
	 */
	const FRTDeclaredCondition& PlannedReactionCondition() const { return Of(ERTPlanEntryClass::Reaction).ReactionCondition; }

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

private:
	/** Le voci, nell'ordine di dichiarazione: si toccano solo dalle porte qui sopra. */
	UPROPERTY()
	TArray<FRTPlanEntry> Entries;

	/** La voce di una classe, o la voce vuota (i default di prima) se non c'e'. */
	const FRTPlanEntry& Of(ERTPlanEntryClass Class) const
	{
		static const FRTPlanEntry Vuota;
		const int32 At = DeclarationIndexOf(Class);
		return At == INDEX_NONE ? Vuota : Entries[At];
	}

	/** La voce di una classe, creata in coda se non c'e': la prima scrittura e' la sua dichiarazione. */
	FRTPlanEntry& Edit(ERTPlanEntryClass Class)
	{
		const int32 At = DeclarationIndexOf(Class);
		if (At != INDEX_NONE)
		{
			return Entries[At];
		}
		FRTPlanEntry& Nuova = Entries.AddDefaulted_GetRef();
		Nuova.Class = Class;
		return Nuova;
	}

	/** Le voci tornate tutte al default escono; le altre restano nel loro ordine (`RemoveAll` e' stabile). */
	void Prune() { Entries.RemoveAll([](const FRTPlanEntry& E) { return E.IsEmpty(); }); }
};
