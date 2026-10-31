#pragma once

#include "CoreMinimal.h"
#include "Combat/RTRangeVerdict.h"       // FRTRangeSubject: i valori della portata di una skill in hover (#3750, fetta D)
#include "Map/RTCellId.h"
#include "Player/RTFocusArea.h"          // FRTFocusMoveSubject: i valori del soggetto a fuoco (#3750)
#include "Player/RTPointerInteraction.h" // ERTPointerContext, ERTPointerTargetKind
#include "UObject/WeakObjectPtrTemplates.h"

/**
 * **Chi scrive le aree della mappa, adesso** — l'arbitro dell'anteprima (`#3750`, fetta C di F5; spec dell'ispettore §6.3).
 *
 * Una sola variabile, derivata dagli ingressi e mai tenuta a parte: sorgenti che scrivessero i setter per conto loro
 * si contenderebbero la mappa col piano. `SkillHover` e' entrata con la fetta D, insieme al suo produttore (lo slot della
 * barra) e alla sua resa: una voce che nessuno emette non e' un vocabolario, e' una promessa.
 *
 * Precedenza: l'hover su una skill (temporaneo: «quando finisce torna la sorgente di prima», §6.3) prima del fuoco, il fuoco
 * prima del piano.
 */
enum class ERTPreviewSource : uint8
{
	/** Le aree non si mostrano (commit, playback, `Modal`, KO, fine partita, nessuna unita' e nessun fuoco). */
	None,

	/** Il piano dell'unita' comandata: ventaglio, portata, rotta, timeline. */
	Plan,

	/** Il fuoco su un personaggio che la squadra osserva: la sua area di movimento, in due bande. */
	Focus,

	/** Il cursore e' su una skill: la sua portata a tre stati (libero, bloccato, senza verdetto), da dove sta chi la possiede. */
	SkillHover,
};

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

	// --- IL FUOCO (`#3750`, fetta C; [D-552]) ---------------------------------------------------------------------
	//
	// 🔴 **Questi campi li scrive il solo risolutore del controller, e SOLO quando l'area del fuoco e' sulla mappa**: in ogni
	// altro caso valgono i default. E' la normalizzazione che tiene la struct onesta: un soggetto a fuoco la cui area e' gia'
	// uscita non fa ricalcolare il piano quando si muove, e nessun campo e' un ingresso che non rifa' mai niente.

	/**
	 * Il personaggio a fuoco, **gia' passato dal cancello di conoscenza**: nullo se l'unita' ispezionata non e' una voce `Live`
	 * nella vista di chi guarda, o non e' un nemico vivo (P2, D-552). Debole, come `Unit`.
	 */
	TWeakObjectPtr<const UObject> FocusSubject;

	/**
	 * Il soggetto come VALORI (cella, movimento base, `Slow`, corsa negata): cio' da cui dipende l'area, e niente di piu'. Mai
	 * il piano (P3): chi lo riempie e' `URTFocusAreaLibrary::SubjectFor`, l'unico punto che legge l'attore.
	 */
	FRTFocusMoveSubject FocusMove;

	/**
	 * Un'impronta di `VisibleCells` di adesso. L'area dipende da cio' che la squadra vede: senza questo ingresso un nemico che
	 * si muove o un velo che cambia lascerebbero a schermo un'area calcolata su una vista di prima.
	 */
	uint32 FocusKnowledgeStamp = 0;

	/**
	 * L'area del fuoco e' ancora sulla mappa. Lo decide `FRTFocusLatch`, e **non torna da sola**: lascia la mappa al primo gesto
	 * di pianificazione (spec §3 regola 4) e ci rientra solo con una nuova fissazione.
	 */
	bool bFocusOnMap = false;

	// --- L'HOVER SU UNA SKILL (`#3750`, fetta D) ---------------------------------------------------------------------
	//
	// Stessa disciplina del fuoco: li scrive il solo risolutore del controller, e valgono i default quando la portata non e'
	// quella che tiene la mappa. L'identita' dell'hover e' nei VALORI della portata, non nell'indice dello slot: due skill con
	// la stessa portata sono la stessa cosa per la mappa, e passare dall'una all'altra non ricalcola niente.

	/**
	 * L'unita' che possiede la skill sotto il cursore, **gia' passata dai cancelli**: e' l'unita' comandata (la barra e' sua),
	 * viva, e la skill chiede un bersaglio e ha una portata. Debole, come `Unit`.
	 */
	TWeakObjectPtr<const UObject> HoverSkillUnit;

	/** La portata di quella skill come valori. Mai il piano del possessore: l'origine e' la cella da cui mira. */
	FRTRangeSubject SkillRange;

	/** Un'impronta di `VisibleCells`: la portata a tre stati dipende da cio' che la squadra vede adesso. */
	uint32 SkillKnowledgeStamp = 0;

	/**
	 * La skill sotto il cursore e' quella di un'unita' ISPEZIONATA e non comandata (`#3748`): il proprietario non e' l'unita'
	 * comandata, e puo' non esserci nessuna selezione. Il produttore e' lo slot della barra in sola lettura
	 * (`URTActionSlotWidget::HoverBegin`, che passa da `ARTPlayerController::SetSkillHover`), non la scheda: le righe della scheda sono testo e
	 * non producono nessun hover (la scheda mostra i dettagli della skill che lo slot ha sotto il cursore, e non l'elenco del kit).
	 *
	 * ⛔ **Lo scrive il solo risolutore del controller, e solo quando il proprietario e' l'unita' ispezionata e fissata** e ha passato i
	 * cancelli (viva, e per un nemico una voce `Live` nella vista di chi guarda). Senza questo flag la skill di un'altra unita' non tiene
	 * la mappa da qui: l'arbitro non si apre con un `HoverSkillUnit` qualunque, come non si apriva prima dell'ispezione.
	 */
	bool bSkillHoverOfInspected = false;

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

	/**
	 * **L'area del fuoco si mostra?** Pura. Vera con un soggetto osservato, l'area ancora sulla mappa, la partita in corso e un
	 * contesto che lo ammette (`ContextAllowsFocus`). Non chiede un'unita' comandata: si ispeziona anche senza selezione.
	 */
	bool ShowsFocus() const;

	/**
	 * **La portata di una skill in hover si mostra?** Pura. Vera con un'unita' comandata viva, una skill sotto il cursore che e'
	 * SUA (`HoverSkillUnit == Unit`: la barra e' di chi si comanda), una portata positiva, e un contesto in cui le aree di
	 * pianificazione si mostrano (`ShowsAreas()`): dove la barra e' attiva e il mondo e' scrivibile. In `Targeting` vale
	 * anch'essa: sostituisce la portata dell'azione armata finche' il cursore e' sullo slot, e poi torna.
	 *
	 * ➕ `#3748`: vera anche per la skill di chi e' a fuoco, quando il risolutore l'ha ammessa (`bSkillHoverOfInspected`), in un contesto
	 * che lo consente (`ContextAllowsInspectedHover`) e a partita in corso. Non chiede un'unita' comandata: si ispeziona anche senza selezione.
	 */
	bool ShowsSkillHover() const;

	/**
	 * **L'hover su una skill di un'unita' ispezionata puo' stare sulla mappa in questo contesto?** (`#3748`) Dove la scheda si vede e il
	 * mondo si guarda per pianificare: `IdleSelection` (si ispeziona anche senza selezione) e i quattro contesti di pianificazione. Non in
	 * `ResolutionPlayback`, dove le aree di pianificazione sono spente e il fuoco lo e' per scelta della sessione (da giudicare in PIE),
	 * ne' in `ReactionWindow` e `Modal`, dove la scheda e' nascosta. Nessun `default:`, come le sorelle.
	 */
	static bool ContextAllowsInspectedHover(ERTPointerContext Context);

	/**
	 * **Il fuoco puo' stare sulla mappa in questo contesto?** Solo `IdleSelection` e `Planning`: il resto e' o un gesto di
	 * pianificazione gia' fatto (`Pathing`, `Targeting`, `Facing`: l'area e' gia' uscita), o un mondo in sola lettura dove
	 * l'ispettore e' inerte (`ReactionWindow`, `Modal`) o dove il fuoco resta spento per scelta della sessione
	 * (`ResolutionPlayback`, da giudicare in PIE). Nessun `default:`, come `ShowsAreas`: un contesto nuovo lo dice qui.
	 */
	static bool ContextAllowsFocus(ERTPointerContext Context);

	/**
	 * **Il fuoco resta in vita ma nascosto in questo contesto?** Finestra di reazione e modale: l'ispettore e' inerte e la
	 * scheda e' nascosta SENZA essere chiusa, e torna quando si chiudono (spec §3 regole 5 e 11). Non sono gesti di
	 * pianificazione, quindi il latch non si spegne e l'area torna con la scheda. ⚠️ `ResolutionPlayback` NON e' qui: il fuoco
	 * resta spento per scelta della sessione, e a playback finito il turno e' cambiato, quindi l'area e' comunque uscita.
	 * Nessun `default:`, come le due sorelle.
	 */
	static bool ContextSuspendsFocus(ERTPointerContext Context);

	/** **Chi scrive adesso**: il fuoco prima del piano, perche' una sola sorgente alla volta tiene la mappa. */
	ERTPreviewSource ActiveSource() const;
};

/**
 * **Il fuoco lascia la mappa al primo gesto di pianificazione e non torna da sola** (`#3750`, spec §3 regola 4).
 *
 * Pura, e derivata dallo stato come il resto di `#3511`: nessun evento da ricordarsi di spegnere. Si accende con una nuova
 * fissazione (`PinSerial` diverso dall'ultimo visto) e fotografa il piano di quel momento; da li' in poi si spegne appena
 * cambia l'unita' comandata, la revisione del piano, l'azione armata o il turno, esce da un contesto che ammette il fuoco
 * (salvo i due che lo SOSPENDONO, `ContextSuspendsFocus`: la finestra di reazione e il modale nascondono l'area senza
 * spegnerla), o il soggetto smette di essere osservato. Una volta spenta resta spenta, anche se lo stato torna uguale
 * (disarmare non riporta l'area).
 *
 * ⚠️ **Seleziona un'altra unita' comandata = un gesto di pianificazione.** La spec dice che il fuoco non tocca selezione ne'
 * piano, non cosa succede a lui quando la selezione cambia: la scelta e' della sessione, da giudicare in PIE.
 */
struct REFACTORTACTICS_API FRTFocusLatch
{
	bool bOnMap = false;

	/** L'ultima fissazione vista. */
	int32 PinSerial = 0;

	/** Il piano al momento della fissazione. */
	TWeakObjectPtr<const UObject> BaseUnit;
	int32 BaseRevision = 0;
	int32 BaseArmed = INDEX_NONE;
	int32 BaseTurn = 0;

	/**
	 * Osserva gli ingressi di adesso (con `FocusSubject` gia' passato dal cancello; `bFocusOnMap` e' ignorato) e dice se l'area
	 * e' ancora sulla mappa.
	 * @param InPinSerial  quante fissazioni ha fatto il controller dall'inizio: cambia a ogni `PinInspection`, anche sulla
	 *                     stessa unita'.
	 */
	bool Observe(int32 InPinSerial, const FRTPlanningOverlayInputs& Raw);
};
