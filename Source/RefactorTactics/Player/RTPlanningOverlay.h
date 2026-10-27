#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "Player/RTPointerInteraction.h" // ERTPointerContext, ERTPointerTargetKind
#include "UObject/WeakObjectPtrTemplates.h"

/**
 * Gli ingressi da cui dipendono le aree di pianificazione, **in una sola struct con uguaglianza** — `#3511`
 * (CX-2 di `aree-di-contesto-spec-panel-2026-10-06.md`).
 *
 * 🔴 **Perche' esiste.** Le aree si accendevano per EVENTI, uno a uno: ogni setter nuovo era un innesco in piu' e un
 * punto in piu' da ricordarsi di spegnere. Il commento di `RefreshPlanningPreview` registra tre difetti della stessa
 * classe — la rotta rimasta a schermo durante la risoluzione (`#2390`), il ventaglio riacceso dietro i risultati
 * (`#2555`), il tratto di LOS rimasto indefinitamente (`#3064`) — e ognuno era un'uscita che nessuno aveva elencato.
 *
 * 🔑 **Con gli ingressi in una struct la domanda cambia**: non piu' «chi deve spegnere l'anteprima quando succede X»,
 * ma «dato lo stato di adesso, le aree si mostrano?» — `ShowsAreas()`, una funzione pura. L'uscita dimenticata si
 * corregge da sola al tick successivo, perche' lo stato che la produce ha cambiato un campo di questa struct.
 *
 * ⛔ **Non ha un `Meaning` e non introduce una sede**: l'hover e' un INGRESSO della funzione che produce le aree, non
 * un significato (CX-1). ⚠️ **Il piano non e' una copia**: `PlanRevision` e' solo un contatore che sale a ogni
 * richiesta esplicita di ricalcolo, perche' un waypoint aggiunto non cambia nessun altro campo qui — e una struct che
 * ignorasse quel cambio salterebbe un ricalcolo dovuto.
 */
struct REFACTORTACTICS_API FRTPlanningOverlayInputs
{
	/** L'unita' selezionata, o nessuna. Debole: una pointer nuda potrebbe puntare a un'unita' distrutta e riusata. */
	TWeakObjectPtr<const UObject> Unit;

	/** L'unita' e' viva. Un'unita' caduta, ancora selezionata a fine playback, non pianifica (`AC-10`, `#3517`). */
	bool bUnitAlive = false;

	/**
	 * Il contesto del puntatore: lo decide `GetPointerContext`, e qui si legge, non si ricalcola. ⚠️ Rompe l'uguaglianza ma
	 * NON `SamePlanLevel` quando cambia fra contesti che mostrano le aree (es. `Planning` → `Facing`): il piano si rifa'
	 * solo se cambia se le aree si mostrano (`ShowsAreas()`).
	 */
	ERTPointerContext Context = ERTPointerContext::IdleSelection;

	/** L'azione armata, `INDEX_NONE` se nessuna. */
	int32 ArmedAbilityIndex = INDEX_NONE;

	/** Che forma di bersaglio chiede l'azione armata. */
	ERTPointerTargetKind ArmedTargetKind = ERTPointerTargetKind::None;

	/** La cella sotto il cursore e se e' valida (nella mappa). */
	FRTCellId HoveredCell;
	bool bHoveredValid = false;

	/** Il settore puntato sull'esagono finale in `Facing`, `INDEX_NONE` se nessuno. */
	int32 FacingSector = INDEX_NONE;

	/**
	 * Il piano attivo (`D-255`). ⚠️ **Vive a livello dell'hover, non del piano**: decide quale cella sotto il cursore un click
	 * seleziona, e `RefreshPlanningPreview` non lo legge — PageUp/PageDown non rifanno il ventaglio.
	 */
	int32 ActiveLayer = 0;

	/** Sale a ogni richiesta esplicita di ricalcolo: il piano cambia senza che nessun altro campo lo dica. */
	int32 PlanRevision = 0;

	/** Il turno: un turno nuovo riparte da capo anche se tutto il resto e' uguale. */
	int32 TurnIndex = 0;

	/** La partita e' finita. `GetPointerContext` non lo dichiara di proposito, quindi va letto a parte. */
	bool bMatchEnded = false;

	bool operator==(const FRTPlanningOverlayInputs& Other) const;
	bool operator!=(const FRTPlanningOverlayInputs& Other) const { return !(*this == Other); }

	/**
	 * Uguali **a meno dell'hover**: cio' da cui dipende il PIANO. Una differenza qui rifa il ventaglio e la portata; una
	 * differenza nel solo hover no (`CX-2`: «nessun produttore all'hover ricalcola il reachable set», criterio di `#711`).
	 * Stanno FUORI da questo livello la cella sotto il cursore con la sua validita', il settore puntato, il piano attivo e il
	 * valore esatto del contesto.
	 */
	bool SamePlanLevel(const FRTPlanningOverlayInputs& Other) const;

	/**
	 * **Le aree di pianificazione si mostrano?** Pura: nessun mondo, nessun attore.
	 *
	 * Vera quando c'e' un'unita' viva e il giocatore sta pianificando (`Planning`, `Pathing`, `Targeting`, `Facing`) e
	 * la partita non e' finita. Falsa a mondo in sola lettura — playback, finestra di reazione, modale — e quindi alle
	 * uscite che l'issue elenca: commit, risoluzione, Cleanup, `Modal`, KO e fine partita.
	 */
	bool ShowsAreas() const;
};
