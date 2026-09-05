#pragma once

#include "CoreMinimal.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnRules.h"
#include "RTReplayManifest.generated.h"

/**
 * Versione del formato del manifest. Versionato dal primo byte anche se oggi ha pochi campi, perche' il
 * TurnLog ha gia' dimostrato cosa succede altrimenti: e' alla `v7` e le versioni dalla 2 in poi restano
 * tutte leggibili **solo** perche' ogni estensione ha accodato campi invece di inserirli in mezzo.
 *
 * ⚠️ Non e' UENUM (uint16 esce dai vincoli UHT del BlueprintType uint8): e' una costante di formato interna,
 * come `ERTTurnLogFormatVersion`.
 */
enum class ERTReplayManifestVersion : uint16
{
	/** Header di partita, hash ordinati per turno, esito, chiusura. Nessun campo di compatibilita' (#413). */
	Initial = 1,

	/**
	 * `ObserverTeamIds`: le squadre per cui esiste una traccia pubblica **filtrata per osservatore**
	 * ([D-316], `#2098`).
	 *
	 * ⚠️ **Un manifest `v1` resta leggibile e significa «nessuna traccia per osservatore»**, che e' cio' che
	 * quegli archivi hanno davvero su disco: l'array nasce vuoto e il campo assente lo lascia vuoto. Non
	 * serve un ramo di migrazione, e non e' fortuna — e' la ragione per cui il campo e' un elenco di cio'
	 * che ESISTE invece di un flag `bFiltered` che un archivio vecchio non saprebbe smentire.
	 *
	 * ⛔ **Rivendicata il 2026-09-03 dopo aver misurato TUTTI i branch remoti** e non solo `main`: nessuno
	 * rivendicava una `v2` di questo manifest. Il conto e' del giorno, e chi legge dopo lo rifaccia.
	 */
	WithObserverTraces = 2,

	/**
	 * `LocalObserverTeamId`: di CHI era questa registrazione ([D-317], `#2156`).
	 *
	 * 🔴 **La `v2` aveva consegnato il meccanismo senza il suo produttore.** `ObserverTeamIds` dice *quali
	 * viste esistono* — `{0, 1}` e' vero per entrambi i giocatori di quella partita — e **non** quale sia la
	 * tua. Senza questo campo nemmeno una UI ben scritta saprebbe cosa passare a `OpenMatchAsTeam`, e ogni
	 * replay si aprirebbe neutrale **per omissione** invece che per scelta.
	 *
	 * ⚠️ **`INDEX_NONE` su un manifest `v1`/`v2` e' il valore GIUSTO, non un ripiego**: quelle registrazioni
	 * davvero non dichiarano un osservatore locale, e la lettura corretta e' «nessuno» — cioe' spettatore
	 * neutrale, che e' il comportamento che avevano gia'. Nessun ramo di migrazione.
	 *
	 * ⛔ **Rivendicata il 2026-09-03 dopo aver misurato TUTTI i branch remoti**: nessuno rivendicava una
	 * `v3`. E' la seconda versione in due giorni — il conto va rifatto, non ereditato.
	 */
	WithLocalObserver = 3,

	/**
	 * Versione che questo binario SCRIVE. Chi aggiunge un campo alza questo alias e lascia in piedi il valore
	 * storico sopra, cosi' l'elenco resta la storia del formato invece di una riga riscritta ogni volta.
	 *
	 * ⚠️ **Un numero di versione e' una risorsa contesa** (`#471`): prima di prenderlo, controllare TUTTI i
	 * branch remoti e non solo `main`. La `v6` del TurnLog fu rivendicata da due branch insieme, e il
	 * duplicato non si rinumera da solo — corrompe tracce gia' scritte.
	 */
	Current = WithLocalObserver
};

/**
 * Header di una partita registrata: l'indice dei suoi turni e cio' che serve a nominarla senza aprirla.
 *
 * E' l'artefatto deciso da [D-077](../../../docs/decisions/RT_PDR_00_Decision_Log.md) — «un manifest per
 * partita piu' una traccia per turno» — e non e' un file in piu': e' **lo stesso** che l'indice di `#416`
 * chiede, e la casa che `D-062` aveva gia' assegnato a `HashTurnLogOrdered` senza che esistesse.
 *
 * JSON e non binario, per una ragione operativa: il payload sono le tracce, questo sono **metadati**, e un
 * archivio rotto lo si diagnostica leggendo l'header a occhio. `Json`/`JsonUtilities` sono gia' dipendenze
 * del modulo — l'harness le usa per i report — quindi non ne entra una nuova.
 */
USTRUCT(BlueprintType)
struct FRTReplayManifest
{
	GENERATED_BODY()

	/**
	 * Identita' della REGISTRAZIONE, non del contenuto (`D-077`): un `FGuid` generato all'avvio della
	 * partita. Il contenuto ha gia' i suoi hash, e derivare l'id dal setup farebbe collidere due partite
	 * giocate davvero due volte.
	 *
	 * ⚠️ **Fuori da ogni hash**, come il wall-clock. Cambiarlo non cambia nessun hash di nessuna traccia.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	FGuid MatchId;

	/** Formato di partita in vigore (`Format.Skirmish2v2`): la stessa identita' che l'header del TurnLog porta. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	FName FormatId;

	/** `true` = hex. Le celle di una traccia quadrata non significano la stessa cosa: senza, un confronto mente. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	bool bHexTopology = true;

	/**
	 * `HashTurnLogOrdered` di ogni turno, in ordine di turno. `D-062` gli aveva assegnato «l'header del
	 * Replay Archive», e `D-077` ha dato a quell'header questa forma: non e' ricalcolabile dai byte della
	 * traccia, perche' quelli sono in forma canonica (`D-SR-1`) e hanno perso l'ordine di append.
	 *
	 * `int64` e non `uint32` perche' `UPROPERTY` non supporta `uint32`: i valori sono hash a 32 bit senza
	 * segno, e l'allargamento e' una zero-extension che li preserva — `0xFFFFFFFF` diventa `4294967295`.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	TArray<int64> OrderedHashPerTurn;

	/**
	 * Checksum dello stato di fine partita. `0` = non calcolato — vedi `bClosed`.
	 *
	 * ⚠️ Il tipo e' `int64` per lo stesso vincolo di `UPROPERTY`, ma il valore che ci arriva e' un **uint32**:
	 * `HashMatchState` ritorna 32 bit. Se un giorno il checksum diventasse davvero a 64 bit, questo campo
	 * andrebbe scritto nel JSON come **stringa**, perche' oltre 2^53 un numero JSON e' un double e comincia
	 * ad arrotondare in silenzio.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	int64 FinalStateHash = 0;

	/** Esito, quando la partita e' finita. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	ERTMatchOutcome Outcome = ERTMatchOutcome::InProgress;

	/** Durata in secondi reali. Vive SOLO qui e in nessun campo che entri in un hash. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	float WallClockSeconds = 0.f;

	/**
	 * Di **CHI** era questa registrazione: la squadra del giocatore locale al momento in cui e' cominciata
	 * ([D-317], `#2156`). `INDEX_NONE` = nessun osservatore locale.
	 *
	 * 🔴 **E' la risposta a una domanda che `ObserverTeamIds` NON risponde**, ed e' il difetto che [D-316]
	 * aveva lasciato scoperto: quell'elenco dice *quali viste esistono* — `{0, 1}` e' vero per entrambi i
	 * giocatori — mentre questo dice *quale sia la tua*. Senza, `OpenMatchAsTeam` non ha un argomento da
	 * ricevere e ogni replay si apre neutrale per omissione.
	 *
	 * ⚠️ **`INDEX_NONE` non e' «non lo so»: e' «non c'era».** Un dedicated server registra una partita che
	 * non e' di nessuno in locale, e la lettura giusta e' lo spettatore neutrale — non la squadra `0`. Per
	 * questo il valore **non** si ricava da `ARTPlayerState::TeamIdOf`, che risponde `0` anche senza
	 * controller: quel ripiego e' corretto in partita, dove un giocatore c'e' sempre, e qui direbbe che la
	 * registrazione era della squadra `0` quando non era di nessuno.
	 *
	 * ⛔ **Non entra in nessun hash**, come `MatchId` e il wall-clock: dice chi guardava, non cosa la partita
	 * ha risolto. Due installazioni che giocano la stessa partita dai due lati producono lo stesso
	 * `OrderedHashPerTurn` e questo campo diverso, ed e' corretto.
	 *
	 * ⚠️ **Non e' un confine di privacy.** Non decide cosa si puo' vedere — quello lo fa il filtro di
	 * [D-316] alla registrazione — ma *quale vista si apre di default*. Un archivio con questo campo a `1`
	 * non impedisce a nessuno di chiamare `OpenMatchAsTeam(id, 0)`: le tracce sono entrambe li'.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	int32 LocalObserverTeamId = INDEX_NONE;

	/**
	 * Le squadre per cui l'archivio porta una traccia **filtrata per osservatore** ([D-316], `#2098`).
	 *
	 * Per ogni `TeamId` qui elencato esiste, accanto a ogni `turn-NNN.rtlog`, un `turn-NNN.tN.rtlog` che
	 * contiene solo le voci che quella squadra era autorizzata a conoscere **quando sono accadute**.
	 *
	 * 🔴 **Elenca cio' che ESISTE, e non e' un dettaglio di stile.** Un flag `bObserverFiltered` avrebbe
	 * costretto ogni lettore a fidarsi di una promessa che un archivio `v1` non puo' smentire; un elenco
	 * vuoto e' invece **vero** su ogni archivio scritto prima di questa versione, senza un ramo di
	 * migrazione. Chi chiede una squadra che non e' qui riceve la traccia canonica, non un file mancante.
	 *
	 * ⚠️ **Fuori da ogni hash**, come `MatchId` e il wall-clock: dice cosa c'e' sul disco, non cosa la
	 * partita ha risolto. Le tracce per osservatore sono un **derivato** della traccia canonica, e far
	 * dipendere un hash di determinismo da quante squadre giocavano lo renderebbe una misura di setup.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	TArray<int32> ObserverTeamIds;

	/**
	 * `false` = la partita non e' arrivata alla fine: crash, uscita, interruzione.
	 *
	 * E' cosi' che un archivio **parziale si dichiara tale** senza bisogno di un secondo meccanismo: il
	 * recorder scrive le tracce **durante** il match e chiude il manifest solo a partita conclusa, quindi un
	 * manifest non chiuso *e'* la dichiarazione di parzialita'. Chi lo legge sa che `Outcome`,
	 * `FinalStateHash` e la durata non sono stati scritti, invece di leggerli come «pareggio, hash zero».
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	bool bClosed = false;

	/** Numero di turni registrati. Ridondante con `OrderedHashPerTurn.Num()`, e serve: un manifest parziale
	 *  puo' avere piu' tracce su disco che hash scritti, e la differenza e' diagnostica. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	int32 TurnCount = 0;
};
