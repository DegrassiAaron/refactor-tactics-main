#pragma once

#include "CoreMinimal.h"
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
	ObjectiveChanged
};

/**
 * Un fatto che il giocatore puo' leggere, gia' autorizzato — `#1936` §C.
 *
 * 🔑 **Argomenti semantici, non una frase.** Il testo non e' un campo: nasce a valle da questi argomenti,
 * cosi' che la localizzazione abbia qualcosa su cui lavorare e nessuno sia tentato di ricavare un fatto
 * facendo il parsing di una stringa diagnostica — che e' il divieto centrale della issue.
 *
 * ⛔ **Nessuna cella.** Non e' una dimenticanza: la posizione e' l'informazione che [D-223] protegge, ed e'
 * cio' che distingue l'annuncio pubblico di un'eliminazione (nome e squadra, mai dove) dalla riga letale del
 * canale derivato, che porta due celle e resta filtrata. Un campo cella qui sarebbe una porta aperta per
 * omissione.
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
};
