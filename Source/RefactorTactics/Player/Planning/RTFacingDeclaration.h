#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h" // FRTCellId, ERTHexDirection

class ARTUnit;
class FRTFacingHost;
enum class ERTMovementStyle : uint8; // Ability/RTActionDef.h: qui basta il nome

/**
 * **LA DICHIARAZIONE DEL VERSO FINALE** ([D-367], [D-462], [D-463], `#291`) — il primo carico della sessione di
 * pianificazione che esce dal controller (#4065, S4).
 *
 * Il verso si sceglie col secondo click sull'esagono finale del movimento: la propria cella da fermo, dopo un primo click
 * che apre il selettore; la destinazione in marcia, direttamente su un lato. La dichiarazione **chiude il movimento**:
 * da quel momento un click su un'altra cella non aggiunge waypoint, finche' un Back non lo riapre.
 *
 * Qui stanno lo stato del selettore (aperto, e il lato sotto il cursore) e le regole del gesto. Il controller tiene le
 * facciate pubbliche con le firme di prima, e implementa `FRTFacingHost`, l'unica cosa che di lui questo tipo conosce.
 *
 * ⚠️ **La legalita' non si decide qui**: e' di `URTFacingLibrary`, e qui la si chiede. Un settore illegale e' rifiutato,
 * mai corretto verso il legale piu' vicino. E il verso che si dichiara e' una PREVISIONE: il resolver rivalida a fine
 * Move su cio' che e' successo davvero, e la dichiarazione allora puo' cadere con `DeclarationRejected`.
 *
 * ⚠️ **Nessuna riflessione**, come `FRTPlaybackDirector`: lo stato e' di sessione e muore col controller.
 */
class REFACTORTACTICS_API FRTFacingDeclaration
{
public:
	/** La dead-zone centrale dell'esagono del verso, in frazione di `HexSize` ([D-367]). */
	static constexpr float DeadZoneFraction = 0.3f;

	/** Vero mentre il selettore e' aperto. */
	bool IsDeclaring() const { return bDeclaring; }

	/** Il lato su cui il cursore ha girato la mesh, se c'e'. */
	TOptional<ERTHexDirection> GetHoverSector() const { return HoverSector; }

	/** Apre il selettore (l'unita' selezionata deve esserci). */
	void Begin(const FRTFacingHost& Host);

	/** Chiude il selettore senza dichiarare nulla. L'anteprima dell'hover non sopravvive: la mesh torna al verso pianificato. */
	void End(const FRTFacingHost& Host);

	/**
	 * `Facing` -> `PlannedFacing` + `bDeclaresPlannedFacing`. Rifiuta un settore **illegale** invece di correggerlo in
	 * silenzio. @return true se il verso e' stato dichiarato.
	 */
	bool HandleSector(FRTFacingHost& Host, ERTHexDirection Sector);

	/**
	 * Il SECONDO CLICK sull'esagono finale: `Cell` deve essere la cella finale del movimento, e `Sector` il lato puntato.
	 * ⚠️ **Da fermo serve il selettore aperto** (`TryOpenSelector`): la propria cella non e' una destinazione.
	 * @return true se il verso e' stato dichiarato.
	 */
	bool HandleClick(FRTFacingHost& Host, const FRTCellId& Cell, ERTHexDirection Sector);

	/**
	 * Apre il selettore se `ClickedCell` e' la cella finale del movimento: la propria da fermo ([D-462] punto 4), la
	 * destinazione in marcia ([D-463]). 🔑 **Aperto, chiude il movimento**. @return true se il selettore e' stato aperto.
	 */
	bool TryOpenSelector(const FRTFacingHost& Host, const FRTCellId& ClickedCell);

	/**
	 * La decisione di `TryHandleClickUnderCursor` PRIMA del raggio: un click sull'esagono finale va al selettore del verso?
	 * Falsa se il click e' di un altro (un bersaglio da scegliere, il primo click da fermo, il nemico puntato dalla carica,
	 * il piano non scrivibile), vera quando il selettore e' il suo destinatario.
	 *
	 * 🔴 Per una carica l'esagono finale e' quello del nemico puntato, e `OnSelect` manda quel click al selettore PRIMA
	 * della matrice delle unita': i rami `Center` e `Side` consumano il click comunque, quindi senza questa risposta un
	 * secondo click sul nemico non arriva mai a `DispatchUnitClick` ([D-528] (3)).
	 */
	bool HandlesClickOnFinalCell(const FRTFacingHost& Host, const ARTUnit* Unit) const;

	/** Il click del giocatore sull'esagono finale, dal cursore: settore, dead-zone, priorita' sulla mesh. */
	bool TryHandleClickUnderCursor(FRTFacingHost& Host, ARTUnit* Unit);

	/**
	 * L'hover del selettore aperto ([D-367]: *«l'hover ne anticipa il settore e la rotazione visuale senza mutare lo stato
	 * autorevole»*): la mesh si gira verso il lato **legale** sotto il cursore, e torna al verso pianificato quando il
	 * cursore esce o il selettore si chiude. ⚠️ Un lato illegale non ruota: non e' interattivo.
	 */
	void UpdateHoverFromRay(const FRTFacingHost& Host, bool bHasRay, const FVector& RayOrigin, const FVector& RayDir);

	/** La cella su cui si sceglie il verso: la destinazione dello scatto pianificato, del percorso, o la propria. */
	static FRTCellId CellFor(const ARTUnit* Unit);

	/** Stile e rotta del movimento pianificato su cui si giudica il verso: come li applichera' il resolver (`#291`). */
	static void PlannedMovement(const FRTFacingHost& Host, const ARTUnit* Unit, ERTMovementStyle& OutStyle,
		TArray<FRTCellId>& OutPath);

	/** Il verso `Sector` e' legale per il movimento pianificato? E' la stessa domanda per il click e per l'hover. */
	static bool IsLegalForPlan(const FRTFacingHost& Host, const ARTUnit* Unit, ERTHexDirection Sector);

	/** Cancella il verso dichiarato e riapre il movimento, dicendo perche'. Niente se non c'era un verso. */
	static void CancelDeclared(const FRTFacingHost& Host, ARTUnit* Unit, const TCHAR* Perche);

	/**
	 * Ruota la MESH verso il facing che l'unita' avra' a fine mossa: la rotazione dichiarata se c'e', altrimenti quella
	 * derivata dal percorso pianificato, altrimenti l'orientamento attuale.
	 *
	 * ⚠️ **Solo presentazione, e in ANTICIPO.** Il facing logico non cambia in pianificazione — lo scrive il resolver a
	 * fine Move — quindi fra qui e la risoluzione la mesh mostra un orientamento che le regole non hanno ancora.
	 * `PIE-FACING-1` chiede che mesh e regola coincidano **a fine playback**, ed e' li' che il TurnManager le riallinea.
	 */
	static void PreviewPlanned(const FRTFacingHost& Host, ARTUnit* Unit);

	/** Ruota la MESH verso `Direction`, dalla geometria della cella. Solo presentazione: non tocca il piano. */
	static void PreviewToward(const FRTFacingHost& Host, ARTUnit* Unit, ERTHexDirection Direction);

private:
	/** Il selettore del verso e' aperto. */
	bool bDeclaring = false;

	/** Il lato legale sotto il cursore su cui la mesh e' girata, mentre il selettore e' aperto. */
	TOptional<ERTHexDirection> HoverSector;
};
