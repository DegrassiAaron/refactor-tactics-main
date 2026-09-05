#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RTStartupReport.generated.h"

/**
 * La **fase** dell'allestimento, mentre il caricamento e' in corso.
 *
 * ⚠️ **Esiste perche' il DoD di CP 46.2 la chiedeva e nessuno la produceva.** Il DoD nominava tre
 * messaggi — *«Loading map…», «Initializing scenario…», «Preparing bots…»* — e
 * `grep -rn "Loading map\|Initializing scenario\|Preparing bots\|LoadingPhase" Source/` dava **zero**:
 * tre stringhe scelte a mano nel widget sarebbero state tre **percentuali con un nome**, cioe' lo stesso
 * dato inventato che il DoD vieta due righe sopra quando parla della barra di avanzamento.
 *
 * Qui la fase e' un **dato**: il widget la legge, non la compone.
 */
UENUM(BlueprintType)
enum class ERTLoadPhase : uint8
{
	/** Nessun caricamento in corso. */
	Idle,
	/** Si sta costruendo la mappa esagonale (`ApplyMapSource`). */
	Map,
	/** Si stanno applicando formato e regole (`ApplyMatchFormat`). */
	Scenario,
	/** Si stanno schierando le unita' e i profili bot. */
	Bots,
	/** Allestimento concluso: la partita e' giocabile. */
	Ready
};

/**
 * L'esito dell'**avvio della partita**. Elenco **chiuso**, come `ERTNavResult` (CP 46.1) e come gli otto
 * esiti del contratto del puntatore: se un avvio non produce uno di questi, il caso non e' descritto e si
 * dichiara qui prima di scriverlo.
 *
 * ## Perche' un ottavo vocabolario, e perche' istruito prima di scrivere
 *
 * Il repository ne ha gia' **sette** — `ERTHexTargetReason`, `ERTActionInvalidReason`,
 * `ERTHexWaypointReason`, `ERTDisplacementBlockReason`, `ERTMatchEndReason`, `ERTMoveOutcome`,
 * `ERTNavResult` — e **nessuno copre l'avvio**. L'alternativa non era «nessun vocabolario»: era che il
 * motivo nascesse come `FString` composta dentro il widget, e allora la UI sarebbe diventata la sorgente
 * della spiegazione — una seconda autorita' su *perche'* qualcosa non e' partito, libera di divergere dal
 * log. I valori qui sotto sono **misurati** su `RTGameMode.cpp`, uno per punto di uscita reale, non
 * immaginati.
 *
 * ## La divisione che conta: fatale contro degradato
 *
 * Il DoD di CP 46.2 immaginava un mondo binario — o parte, o `SCENARIO COULD NOT START`. Il codice non si
 * comporta cosi': misurati **21 `UE_LOG(Warning)` contro 8 `UE_LOG(Error)`**, e i due casi piu' frequenti
 * dell'avvio sono **ripieghi silenziosi** che fanno partire una partita normale — l'arena di PROVA al
 * posto della mappa d'autore, e il formato di RIPIEGO. Sono **le due riserve che tengono `G13` 🟡**.
 *
 * Da qui le due forme, e `IsFatal()` e' cio' che le separa:
 *
 * - **fatale** → la partita non e' allestita → **modale**, e il giocatore deve uscire;
 * - **degradato** → la partita parte, ma non e' quella che si crede → **banner** persistente.
 */
UENUM(BlueprintType)
enum class ERTStartupOutcome : uint8
{
	/** Allestita come dichiarato: mappa d'autore, formato dichiarato. Nessun banner. */
	Ok,

	// ── Fatali: `ApplyMatchFormat` restituisce `false` e la partita NON viene allestita ──────────────

	/** Il `MatchFormat` assegnato non passa il proprio validator. Contenuto sbagliato: si rifiuta. */
	FormatAssetInvalid,
	/** Il formato **spedito** non e' valido: difetto di codice, non di dato. */
	ShippedFormatInvalid,
	/** Formato e mappa non combaciano (CP 19.1): un 3v3 su una mappa 2v2 non e' una partita piu' stretta. */
	FormatMapMismatch,
	/**
	 * Un eroe della formazione non e' nel catalogo (`#1069`): l'allestimento si ferma.
	 *
	 * ⚠️ **Fatale, e prima non lo era**: la guardia faceva `continue` con un Warning, quindi la partita
	 * partiva con le unita' risolte e senza le altre. E' lo stesso dato che `FormatMapMismatch` protegge
	 * dall'altro lato — quanti ne schiera il formato — e riceve lo stesso trattamento.
	 */
	RosterHeroMissing,

	// ── Degradati: la partita parte, ma non e' quella che si crede ──────────────────────────────────

	/**
	 * `MapSource=GeneratedTestArena`: si gioca sulla mappa di PROVA.
	 *
	 * ✅ E' **la prima riserva di `G13`** — *«la partita gira su `MapSource=GeneratedTestArena`, l'arena di
	 * test, non un livello di gioco»* — e finora esisteva solo in una riga di log.
	 */
	UsingTestArena,
	/** `MapSource=GeneratedDemoArena`: arena di ripiego per scelta esplicita. */
	UsingDemoArena,
	/**
	 * Il livello non porta **nessun** `MapAsset`: si ripiega sull'arena demo.
	 *
	 * ⚠️ **Non copre l'asset presente e vuoto**, che e' `LevelMapEmpty`. Fino a #1921 questo valore diceva
	 * «assente **o** senza celle», e i due casi mandavano a correggere cose diverse con la stessa frase.
	 */
	LevelMapMissing,
	/**
	 * Il livello porta un `MapAsset` che ha **zero celle**: si ripiega sull'arena demo (#1921).
	 *
	 * 🔑 **E' l'unico dei due che si verifica davvero oggi**, ed e' documentato come osservato in
	 * `RTMatchBootstrapper`: *«un asset assegnato ma VUOTO non allestisce nulla e premere Play mostra una
	 * schermata nera senza spiegazione (osservato in PIE su `L_DevSandbox`, il cui asset si e' ritrovato a
	 * 0 celle)»*. Ha un esito proprio per la stessa ragione di `MatchRequestNotConsumed`: la causa e la
	 * correzione sono diverse da quelle del fratello, e dire «il livello non porta una mappa» a chi
	 * l'actor **l'ha posato** lo manda a cercare un difetto che non ha.
	 *
	 * Degradato come `LevelMapMissing`: il ripiego e' lo stesso, cambia solo cio' che viene detto.
	 */
	LevelMapEmpty,
	/**
	 * `MatchLevel` non e' configurato: `PLAY` non sa quale livello aprire (CP 46.4, #939).
	 *
	 * 🔴 **Fatale, a differenza di `LevelMapMissing`**, e la differenza e' il ripiego: li' la mappa manca ma
	 * l'arena demo la sostituisce, qui non c'e' niente da aprire e la partita non parte. Un esito senza
	 * ripiego che non fosse fatale lascerebbe il modale disarmato, cioe' un fallimento invisibile.
	 */
	MatchLevelUnset,
	/**
	 * `MatchLevel` e' configurato ma la richiesta precedente non e' stata consumata (CP 46.4, #939).
	 *
	 * ⚠️ Non e' un errore di configurazione: e' un aggancio mancante nel codice — nessuno chiama
	 * `ConsumePendingMatchLevel`. Ha un esito proprio perche' la causa e la correzione sono diverse da
	 * quelle di `MatchLevelUnset`, e un modale che le confondesse manderebbe a controllare il file sbagliato.
	 */
	MatchRequestNotConsumed,
	/**
	 * Nessun formato assegnato **ne' spedito**: regole di ripiego.
	 *
	 * ⚠️ **Ramo raro**: `Format.Skirmish2v2` e' spedito da C++ (`9f44570d`), quindi in una build normale
	 * non ci si arriva. 🔴 Una stesura precedente lo chiamava «seconda riserva di `G13`»: **falso**. La
	 * seconda riserva e' *«la via a punti non e' mai stata esercitata, perche' la soglia obiettivo e' 0»*,
	 * cioe' un **valore** del formato in vigore — non il ripiego del formato. Trovato da un test rosso.
	 */
	UsingFallbackFormat,
	/** Nessun `ARTTurnManager` nel livello: il formato non ha destinatario e non e' stato applicato. */
	NoTurnManager,

	/**
	 * `FrontendLevel` non e' configurato: `RETURN TO MAIN MENU` non sa quale livello aprire (CP 46.6, #941).
	 *
	 * 🔴 **Fatale, e senza ripiego.** Non si puo' «tornare al menu approssimativamente»: se il livello non
	 * e' dichiarato, l'unica alternativa a un errore rumoroso e' un pulsante che non fa niente — cioe' il
	 * soft-lock che il DoD di CP 46.1 chiama dead-end. Stessa forma e stessa ragione di `MatchLevelUnset`.
	 */
	FrontendLevelUnset,

	/**
	 * `RETURN TO MAIN MENU` e' gia' stato chiesto e nessuno ha raccolto la richiesta (CP 46.6, #941).
	 *
	 * ⚠️ Gemello di `MatchRequestNotConsumed`, e per lo stesso motivo ha un esito proprio: la causa non e'
	 * nel `.ini` ma nel codice — nessuno chiama `ConsumePendingFrontendLevel`. Nel ciclo di partita il
	 * consumatore e' `ARTGameMode`, non `ARTFrontendGameMode`: sono due mondi diversi, e per un intero
	 * checkpoint questo lato del confine non aveva nessuno.
	 */
	FrontendReturnNotConsumed
};

/**
 * Una nota d'avvio: **la categoria** piu' il dettaglio gia' prodotto dal codice.
 *
 * ⚠️ Il `Detail` non e' il motivo — e' il suo *dettaglio*. Il motivo e' `Outcome`, ed e' quello che decide
 * la forma (modale o banner) e che un test puo' asserire. `ResolveRules` e `ValidateAgainstMap` producono
 * gia' quelle stringhe oggi: qui vengono **trasportate**, non ricomposte. E' lo stesso rapporto che il
 * TurnLog ha fra un reason code e i suoi parametri.
 */
USTRUCT(BlueprintType)
struct REFACTORTACTICS_API FRTStartupNote
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Frontend")
	ERTStartupOutcome Outcome = ERTStartupOutcome::Ok;

	/** Dettaglio leggibile, prodotto da chi ha rilevato la condizione. Puo' essere vuoto. */
	UPROPERTY(BlueprintReadOnly, Category = "Frontend")
	FString Detail;

	FRTStartupNote() = default;
	FRTStartupNote(ERTStartupOutcome InOutcome, FString InDetail = FString())
		: Outcome(InOutcome), Detail(MoveTemp(InDetail))
	{
	}
};

/**
 * Cosa e' successo all'avvio, in una forma che un widget puo' leggere **senza interpretare testo**.
 *
 * ⚠️ **Le note sono una LISTA, non una sola.** Un avvio accumula piu' condizioni insieme — mappa di
 * ripiego **e** nessun `TurnManager`, per esempio, che e' il caso provato end-to-end da
 * `BothFallbacksAreReportedTogether`. Mostrarne una sola nasconderebbe l'altra, ed e' il modo esatto in
 * cui queste cose sono rimaste invisibili finora: due righe di log **separate**, nessuna delle quali
 * qualcuno aveva motivo di andare a cercare.
 */
USTRUCT(BlueprintType)
struct REFACTORTACTICS_API FRTStartupReport
{
	GENERATED_BODY()

	/** Ogni condizione rilevata durante l'allestimento, nell'ordine in cui si e' presentata. */
	UPROPERTY(BlueprintReadOnly, Category = "Frontend")
	TArray<FRTStartupNote> Notes;

	/** La fase raggiunta. `Ready` solo se l'allestimento e' arrivato in fondo. */
	UPROPERTY(BlueprintReadOnly, Category = "Frontend")
	ERTLoadPhase Phase = ERTLoadPhase::Idle;

	void Add(ERTStartupOutcome Outcome, FString Detail = FString())
	{
		if (Outcome != ERTStartupOutcome::Ok)
		{
			Notes.Emplace(Outcome, MoveTemp(Detail));
		}
	}

	void Reset()
	{
		Notes.Reset();
		Phase = ERTLoadPhase::Idle;
	}
};

/** Funzioni pure sugli esiti d'avvio. Nessuno stato: si possono provare senza mondo. */
UCLASS()
class REFACTORTACTICS_API URTStartupReportLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * `true` se l'esito **impedisce** la partita. E' la funzione che sceglie fra modale e banner, ed e' il
	 * solo posto in cui quella divisione e' scritta: un widget che la ridecidesse sarebbe la seconda
	 * autorita' che questo tipo esiste per evitare.
	 */
	UFUNCTION(BlueprintPure, Category = "Frontend")
	static bool IsFatal(ERTStartupOutcome Outcome);

	/** `true` se la partita e' partita ma non e' quella dichiarata: e' il dominio del banner. */
	UFUNCTION(BlueprintPure, Category = "Frontend")
	static bool IsDegraded(ERTStartupOutcome Outcome);

	/** L'esito fatale del report, `Ok` se non ce n'e' nessuno. Un avvio ne ha al massimo uno: si ferma li'. */
	UFUNCTION(BlueprintPure, Category = "Frontend")
	static ERTStartupOutcome FindFatal(const FRTStartupReport& Report);

	/** `true` se il report porta almeno una condizione degradata da mostrare nel banner. */
	UFUNCTION(BlueprintPure, Category = "Frontend")
	static bool HasDegradation(const FRTStartupReport& Report);

	/** Riga breve e leggibile per una nota: e' il **testo del banner**, e nasce qui, non nel widget. */
	UFUNCTION(BlueprintPure, Category = "Frontend")
	static FText DescribeOutcome(ERTStartupOutcome Outcome);
};
