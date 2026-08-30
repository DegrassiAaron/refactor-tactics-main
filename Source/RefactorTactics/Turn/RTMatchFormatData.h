#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Map/RTHexMapAsset.h"
#include "RTMatchFormatData.generated.h"

/**
 * Le regole di formato RISOLTE, cioe' quelle realmente in vigore in questa partita.
 *
 * L'asset (`URTMatchFormatData`) e' la SORGENTE, questa struct e' cio' che il `TurnManager` legge: una sola
 * verita' per valore, mai un asset consultato in un punto e una `UPROPERTY` di override in un altro
 * (ADR-0005 §4c: "due sorgenti sarebbero due verita'").
 *
 * Contiene solo parametri di REGOLA (spec-durata-partita-e-scala-mappe.md §16.2): input deterministici da cui
 * l'esito dipende. I tempi di parete (`PlanningSeconds`, `MaxPlaybackSeconds`) NON stanno qui e restano
 * `UPROPERTY` sul `TurnManager` — spostarli senza un consumatore creerebbe la seconda verita' di cui sopra.
 */
USTRUCT(BlueprintType)
struct FRTMatchRules
{
	GENERATED_BODY()

	/**
	 * IDENTITA' stabile del formato in vigore. E' l'unico campo che raggiunge l'header del TurnLog: due
	 * esecuzioni dello stesso scenario con `RoundLimit` diverso producono tracce identiche fino al round in
	 * cui il limite morde, e senza questo marcatore la divergenza verrebbe attribuita al CODICE invece che
	 * alla CONFIGURAZIONE (stessa ragione per cui esiste `ERTLogTopology`).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Match")
	FName FormatId;

	/** Numero massimo di round: raggiunto il limite la partita finisce e si confrontano i punteggi. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Match")
	int32 RoundLimit = 0;

	/** Punteggio che chiude la partita per obiettivo. **0 = via disattivata** (nessuna soglia in vigore). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Match")
	int32 ScoreToWin = 0;

	/**
	 * Unita' per squadra (CP 19.2). E' il formato a dichiarare la composizione, non il `GameMode`: finche' il
	 * numero viveva in una `UPROPERTY` dell'orchestratore, «2v2» era una proprieta' del codice che allestisce
	 * la partita, e lo stress 4v4 di E17 doveva essere un caso speciale invece di un formato.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Match")
	int32 UnitsPerTeam = 0;

	/**
	 * Unita' che una singola PERSONA comanda (CP 19.3, **D-155**). Uniforme: tutti i giocatori di una squadra
	 * ne comandano lo stesso numero, e il validator rifiuta un formato in cui la squadra non si divide.
	 *
	 * ⚠️ NON e' `UnitsPerTeam`, e la differenza e' invisibile proprio dove conta. `UnitsPerTeam` risponde
	 * *quante unita' schiera una squadra*; questo risponde *quante ne comanda una persona*. In
	 * `Format.Skirmish2v2` valgono **entrambi 2** — la v0.1 e' offline contro bot, un umano comanda la
	 * squadra intera — quindi un percorso che legga l'uno al posto dell'altro passa ogni test esistente e
	 * sbaglia al primo formato che divide una squadra fra due persone. E' la ragione per cui il campo si
	 * NOMINA invece di dedurlo.
	 *
	 * Il rapporto `UnitsPerTeam / UnitsPerPlayer` e' il numero di persone per squadra: `2/2 = 1` in v0.1,
	 * `3/1 = 3` in un competitivo dove ognuno comanda un eroe.
	 *
	 * 🔵 **Il resolver non lo legge, e non deve.** Governa autorizzazione, input, planning, `Ready`, privacy
	 * e ownership della decisione di reazione — non l'esito: questo resta vero anche dopo il punto seguente.
	 * ✅ **Ha il suo primo consumatore vivo**: `ARTGameMode::AssignSeats` deriva `SeatsPerTeam` da
	 * `UnitsPerTeam / UnitsPerPlayer` e assegna i posti di conseguenza. Il controllo vivo che lo dimostra e'
	 * `Player.SeatCountFollowsUnitsPerPlayer`, non `MatchFormat.ResolverIsInvariantToControlCount`: quel
	 * test confronta l'hash del TurnLog di due partite che differiscono per il solo conteggio e — per il suo
	 * stesso docstring — non attraversa `ARTGameMode`, quindi resterebbe verde anche cancellando
	 * `AssignSeats`. Resta un **tripwire** sulla risoluzione, non un controllo su questo consumo.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Match")
	int32 UnitsPerPlayer = 0;

	/**
	 * Classe di mappa che questo formato richiede. Il validator la confronta con quella dichiarata dalla
	 * mappa: un 3v3 Standard su una mappa disegnata per il 2v2 non e' una partita piu' stretta, e' una
	 * partita sbagliata — e va rifiutata all'allestimento, non scoperta al terzo turno.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Match")
	ERTMapClass MapClass = ERTMapClass::Skirmish;

	FRTMatchRules() = default;
};

/**
 * Formato di partita: il pacchetto di parametri che governa durata ed esito (2v2 Skirmish, 3v3 Standard...).
 * Asset versionato e confrontabile, come il resto dei dati di gioco (`URTActionData`, `URTHeroData`,
 * `URTHexMapAsset`) — non costanti sparse nell'orchestratore.
 *
 * Decisione di forma: issue #185, riportata in `docs/gameplay/spec-durata-partita-e-scala-mappe.md` §16.1.
 * Entra subito **solo cio' che un checkpoint consuma** (D2): `RoundLimit` e `ScoreToWin`, entrambi letti da
 * CP 10.3. Gli altri parametri della spec (§16) migrano quando avranno un lettore, non prima.
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTMatchFormatData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Identita' stabile del formato (es. `Format.Skirmish2v2`). Chiave del confronto fra tracce. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Match")
	FName FormatId;

	// ⚠️ Qui c'era `FormatVersion`, rimossa il 2026-08-14 (**D-141**, #844). Non era una versione: era il
	// **vocabolario** di una versione senza la macchina che lo regge — nessun `CurrentFormatVersion`, nessuna
	// `MigrateToCurrentFormat`, nessun `PostLoad`, e nessuno che la scrivesse (`FindShippedFormat` la lasciava
	// al default). Prometteva una rete che non esisteva, che e' esattamente cio' che #687 ha pagato per otto
	// versioni su `URTHexMapAsset`.
	//
	// Quando servira' davvero — cioe' quando un formato **autorato come asset** dovra' sopravvivere a un
	// cambio di significato dei suoi campi — si aggiunge insieme al primo passo di migrazione, e con il
	// meccanismo giusto: `FCustomVersionRegistry`, come in `Map/RTHexMapCustomVersion.h`, che ha gia' il suo
	// test a due binari da imitare. Un numero di versione da solo non protegge niente.

	/** Numero massimo di round. Valore iniziale del 2v2 v0.1; banda 10-14, cap 14-16 (spec §6). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Match")
	int32 RoundLimit = 12;

	/**
	 * Round attesi per una partita di questo formato. **Nessun codice di gioco lo legge**: e' un target di
	 * design (spec §16.2, terza classe) e il suo unico lettore e' il validator, che rifiuta un formato in cui
	 * i round attesi superano il limite — una contraddizione che altrimenti resterebbe silenziosa.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Match")
	int32 ExpectedRounds = 12;

	/** Punteggio obiettivo che chiude la partita. 0 = nessuna vittoria per obiettivo in questo formato. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Match")
	int32 ScoreToWin = 0;

	/**
	 * Unita' per squadra (CP 19.2). `Format.Skirmish2v2` dichiara **2**.
	 *
	 * Con questo campo E17 (stress 4v4) smette di essere un ramo del `GameMode` e diventa un formato che
	 * dichiara 4: la differenza fra le due partite sta nel DATO, non in un `if` dell'orchestratore.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Match")
	int32 UnitsPerTeam = 2;

	/**
	 * Unita' comandate da una singola PERSONA (CP 19.3, **D-155**). `Format.Skirmish2v2` dichiara **2**:
	 * la v0.1 e' offline contro bot e l'unico umano comanda la squadra intera.
	 *
	 * ## Perche' il default e' `1`
	 *
	 * > **`1 player = 1 character` e' il MODELLO del gioco.** *(decisione dell'autore, 2026-08-17)* Il default
	 * > dichiara il modello; `Format.Skirmish2v2` dichiara **2** perche' e' l'**eccezione** — offline contro
	 * > bot c'e' un solo umano, e comanda la squadra intera.
	 *
	 * 🔴 **Questo campo e' nato con default `2`, e un test lo ha falsificato prima del merge.** La
	 * motivazione era *«un default deve descrivere il formato che il gioco spedisce»*: sbagliata due volte,
	 * perche' il default non descrive il modello **e** non regge all'esecuzione. Le due ragioni tecniche, che
	 * si vedono solo eseguendo:
	 *
	 * 1. **Il formato spedito non usa mai questo default**: `FindShippedFormat` scrive `2` a mano, e il
	 *    ripiego pure. Il default vale solo per i formati che NON sono quello spedito, cioe' esattamente
	 *    quelli che non descrive.
	 * 2. **Con `2` ogni squadra di dimensione dispari nasce invalida.** `MatchFormat.GameModeHonoursComposition`
	 *    allestisce un `Format.Test3v3` e non tocca questo campo: `3 % 2 != 0`, il validator rifiuta, e il
	 *    test cade su un vincolo che non stava verificando. Il 3v3 e' la baseline del prossimo formato reale.
	 *
	 * `1` e' l'unico valore che **divide sempre** (`n % 1 == 0` per ogni `n`), quindi nessun formato nasce
	 * invalido per un campo che il suo autore non ha toccato. Ed e' anche il piu' **restrittivo** in termini
	 * di autorita': il default concede il minimo controllo possibile, una unita' a testa, che e' la direzione
	 * in cui questo repository sbaglia volentieri.
	 *
	 * ⚠️ **Il costo del default, dichiarato invece che scoperto**: un designer che autora un 2v2 nell'editor
	 * e non tocca questo campo dichiara `1`, cioe' due persone per squadra — e il validator lo **accetta**,
	 * perche' `2 % 1 == 0`. Sarebbe la terza descrizione dello stesso 2v2 accanto a `FindShippedFormat` e a
	 * `MakeFallbackRules`, che scrivono entrambe `2`. Nessun controllo puo' prenderla: il validator non sa se
	 * quel formato intende essere giocato da una persona o da due. Oggi il caso non si produce — nessun
	 * `DA_MatchFormat` esiste su disco e il formato della v0.1 e' spedito da C++ — e il giorno in cui esistera'
	 * un asset d'autore, il campo va compilato come si compila `UnitsPerTeam`.
	 *
	 * Vedi `FRTMatchRules::UnitsPerPlayer` per perche' questo campo non e' `UnitsPerTeam`.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Match")
	int32 UnitsPerPlayer = 1;

	/** Classe di mappa richiesta (CP 19.1): il validator rifiuta l'accoppiata sbagliata. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Match")
	ERTMapClass MapClass = ERTMapClass::Skirmish;
};
