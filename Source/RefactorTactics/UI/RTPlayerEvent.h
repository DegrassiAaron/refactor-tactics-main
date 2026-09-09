#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h" // FRTCellId e' un campo per valore: serve la definizione, non basta la forward
#include "RTPlayerEvent.generated.h"

/**
 * QUANTO un evento merita l'attenzione di chi gioca — `#1936` §D.
 *
 * ⚠️ **E' una tassonomia di PRESENTAZIONE, non di gameplay.** Le categorie canoniche del `TurnLog`
 * (`ERTLogCategory`) restano l'unica verita' del resolver: questa dice soltanto quanto in alto una cosa
 * debba arrivare nel feed, e cambiarla non cambia nessun esito.
 *
 * ⛔ **Non entra in nessun hash.** La proiezione e' a valle del `TurnLog` e non lo modifica; se un giorno
 * un'importanza cambiasse valore, nessuna traccia archiviata cambierebbe identita'.
 */
UENUM(BlueprintType)
enum class ERTPlayerEventImportance : uint8
{
	/** Normalmente silenzioso: il giocatore lo vede gia' animato, o non ha conseguenza tattica. */
	Minor,

	/** Merita una riga: qualcosa e' cambiato nel piano o nello stato di qualcuno. */
	Important,

	/** Cambia la partita: KO, obiettivo, fine. */
	Critical
};

/**
 * CHE COSA e' successo, nel vocabolario di chi gioca.
 *
 * 🔴 **Non e' una copia di `ERTLogCategory`, ed e' deliberato.** Il `TurnLog` classifica per *chi ha
 * prodotto la voce* — `Move`, `Combat`, `Facing`, `Fallback` — perche' e' cio' che serve a un replay. Il
 * feed classifica per *che cosa il giocatore ha visto succedere*, e le due partizioni non coincidono: una
 * voce `Combat` con esito `Lethal` e' un `Defeated`, non un `Attacked`, e una `Facing` non e' niente.
 *
 * ⚠️ Aggiungere valori in **coda**: il tipo viaggia come `uint8` nei campioni di presentazione.
 */
UENUM(BlueprintType)
enum class ERTPlayerEventType : uint8
{
	/** Si e' spostata, e il dettaglio delle celle non entra: quello lo mostra l'animazione. */
	Moved,

	/** Non e' arrivata dove voleva — contesa, unita' ferma, topologia, Overwatch. */
	MoveBlocked,

	/** Un colpo ha tolto salute, o l'ha assorbita lo scudo. */
	Attacked,

	/** Una cura e' andata a segno. */
	Healed,

	/** Un'unita' e' caduta. Sempre `Critical`, e per [D-223] **pubblico**. */
	Defeated,

	/** Una reazione e' scattata: Overwatch, Deflect, interposizione. */
	ReactionFired,

	/** Uno stato rilevante e' stato applicato o e' scaduto addosso a qualcuno. */
	StatusChanged,

	/** L'ambiente e' cambiato — acqua, fuoco, ponti. Raggruppato, mai una riga per cella. */
	Environment,

	/** Un obiettivo e' stato conquistato, perso o e' avanzato. */
	ObjectiveChanged,

	/**
	 * Il movimento chiesto e' arrivato, e il terreno **non** e' riuscito a portarla oltre — `#2314`.
	 *
	 * ⛔ **Non e' `MoveBlocked`, ed e' la ragione per cui il valore esiste.** Quello dice «non e' arrivata
	 * dove voleva», che qui e' **falso**: il piano del giocatore ha funzionato per intero. Proiettarlo come
	 * un fallimento del movimento sposta di un canale il difetto che `#2314` chiude nel resolver.
	 *
	 * ⛔ **E non e' `Moved`.** Quello copre gia' due fatti — il movimento ordinario (`Minor`) e lo
	 * scivolamento avvenuto (`Important`) — e appiattirvi anche questo renderebbe il feed incapace di
	 * distinguere «il ghiaccio mi ha spostato» da «il ghiaccio ha provato e non ci e' riuscito», che sono la
	 * premessa opposta per il turno dopo: solo il primo lascia `Status.Unbalanced` (`D-319`).
	 *
	 * 🔴 **Non nomina la causa, e non e' una lacuna.** Cio' che ha impedito lo scivolamento puo' essere
	 * un'unita' che l'osservatore non e' autorizzato a conoscere. L'evento porta il solo soggetto — chi non
	 * e' scivolato — e passa dallo stesso predicato di autorizzazione di ogni altro
	 * (`URTPlayerEventProjector::IsAuthorized`); non esiste nessun campo in cui un'unita' blocker possa
	 * entrare — `BlockerCell` porta una cella di TERRENO e per questo esito resta la sentinella, che
	 * `SlideBlockedDoesNotRevealTheBlocker` verifica.
	 */
	SlideBlocked,

	/**
	 * Il colpo dichiarato non e' partito: fra il tiratore e il bersaglio non c'e' linea di vista — `#2697`.
	 *
	 * 🔴 **E' l'unico esito di `Combat` che il feed racconta pur non essendo accaduto niente**, e la ragione
	 * e' misurata, non stilistica: il bersaglio e' **velato** proprio perche' e' il muro a bloccare la vista.
	 * Chi guarda vede la propria unita' sparare verso il nulla, senza vedere ne' il nemico ne' l'ostacolo, e
	 * senza una riga conclude che l'attacco sia rotto — verdetto d'autore del 2026-09-09 sul banco
	 * `Visual.Map.SightWallIsWalkable`. Ogni altro `NoLineOfSight`-simile — un colpo mancato, una copertura
	 * — lascia qualcosa da guardare; questo no.
	 *
	 * ⚠️ **Non e' `MoveBlocked`**: quello dice che l'unita' non e' arrivata dove voleva, e qui il movimento
	 * puo' essere andato benissimo. Ne' `Attacked`: non c'e' stato nessun colpo, e `Amount` resta `0`.
	 */
	AttackBlocked
};

/**
 * Un fatto che il giocatore puo' leggere, gia' autorizzato — `#1936` §C.
 *
 * 🔑 **Argomenti semantici, non una frase.** Il testo non e' un campo: nasce a valle da questi argomenti,
 * cosi' che la localizzazione abbia qualcosa su cui lavorare e nessuno sia tentato di ricavare un fatto
 * facendo il parsing di una stringa diagnostica — che e' il divieto centrale della issue.
 *
 * ⛔ **Nessuna cella DEI SOGGETTI, e resta la regola.** La posizione di un'unita' e' l'informazione che
 * [D-223] protegge, ed e' cio' che distingue l'annuncio pubblico di un'eliminazione (nome e squadra, mai
 * dove) dalla riga letale del canale derivato, che porta due celle e resta filtrata. Un campo «cella del
 * primario» o «cella del bersaglio» qui sarebbe una porta aperta per omissione, e non esiste.
 *
 * ⚠️ **`BlockerCell` e' l'unica eccezione, ed e' di natura diversa** — `#2697`. Non e' la posizione di
 * nessuno: e' una cella di **terreno**, e arriva gia' autorizzata. Il permesso non viene concesso qui:
 * `URTTurnLogLibrary::SightBlockerForLog` la scrive nella voce **solo** se la squadra dell'attaccante
 * conosceva quella cella (`VisibleCells` ∪ `ExploredCells`, fail-closed), e altrimenti ci lascia la
 * sentinella. E' la stessa ragione per cui `RTReplayPrivacyLibrary` marca `SightBlockerCell` **`Public`**:
 * *«cio' che arriva qui e' gia' filtrato — marcarlo `AuditOnly` non aggiungerebbe privacy, toglierebbe al
 * prodotto pubblico l'unica riga che spiega perche' il colpo non e' partito»*.
 *
 * 🔴 **Il campo esiste per UN tipo, e il proiettore lo riempie solo per quello.** Non e' disciplina da
 * ricordare: `Project` copia la cella soltanto quando l'evento e' un `AttackBlocked`, cosi' un esito il cui
 * blocker sia un'**unita'** — `SlideBlocked` — non ha modo di riempirlo neanche per distrazione. Lo pinnano
 * `SlideBlockedDoesNotRevealTheBlocker` e `BlockedShotDoesNotRevealTheWallToTheUnauthorized`.
 */
USTRUCT(BlueprintType)
struct FRTPlayerEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|PlayerEvent")
	ERTPlayerEventType Type = ERTPlayerEventType::Moved;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|PlayerEvent")
	ERTPlayerEventImportance Importance = ERTPlayerEventImportance::Minor;

	/**
	 * Di CHI parla l'evento, come `StableUnitId`.
	 *
	 * ⚠️ Segue la regola gia' fissata per il `TurnLog`: e' il soggetto di cio' che la riga racconta, non
	 * sempre chi ha agito. Per un `Attacked` e' chi **subisce**, come per la categoria `Combat`
	 * (`#1150`) — *«Gadget: colpisce»* direbbe il falso.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|PlayerEvent")
	int32 PrimaryUnitId = INDEX_NONE;

	/** L'altro capo quando esiste — chi ha colpito, chi ha curato. `INDEX_NONE` quando non c'e'. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|PlayerEvent")
	int32 SecondaryUnitId = INDEX_NONE;

	/** L'azione dichiarata dal catalogo, quando la voce la porta. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|PlayerEvent")
	FName ActionId;

	/** Il numero che l'evento porta: danno, cura, celle percorse. `0` quando non ne ha uno. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|PlayerEvent")
	int32 Amount = 0;

	/**
	 * La cella di terreno che ha fermato il colpo, per il solo `AttackBlocked` — `#2697`.
	 *
	 * 🔑 **E' cio' che permette un riferimento nel MONDO invece che una coordinata nel testo.** Il verdetto
	 * che ha aperto la issue diceva *«non ci sono riferimenti video, solo log»*: con la cella chi disegna
	 * puo' marcare l'ostacolo dove il giocatore sta guardando; con una stringa gia' composta potrebbe solo
	 * ristampare `(q=..,r=..,L=..)`, che e' la diagnostica che `#1936` §A toglie dallo schermo.
	 *
	 * ⚠️ **Tre casi collassano nella sentinella, e devono**: linea libera, muro non nominabile per il velo,
	 * o esito che non e' un tiro rifiutato. Distinguerli direbbe *«c'e' un muro ma non te lo dico»*, che e'
	 * gia' informazione sulla geometria che [D-225] nasconde — la stessa scelta di `SightBlockerCell` nel
	 * `TurnLog`, di cui questo campo e' la proiezione.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|PlayerEvent")
	FRTCellId BlockerCell = FRTCellId(0, 0, INDEX_NONE);

	/**
	 * La sentinella di `BlockerCell`.
	 *
	 * ⚠️ **Non si usa `FRTCellId::IsValid()`**: quella verifica l'invariante cubica `q + r + z == 0`, vera
	 * per la cella di default `(0,0,0)` — una cella «vuota» sarebbe valida, e il feed marcherebbe l'origine
	 * dell'arena a ogni evento che non ha muro. Il marcatore e' il `Layer` a `INDEX_NONE`, la stessa
	 * convenzione di `FRTTurnLogEntry::NoSightBlocker()`.
	 */
	static FRTCellId NoBlockerCell() { return FRTCellId(0, 0, INDEX_NONE); }

	/** Vero se l'evento porta un ostacolo nominabile da mostrare. */
	bool HasBlockerCell() const { return BlockerCell.Layer != INDEX_NONE; }
};
