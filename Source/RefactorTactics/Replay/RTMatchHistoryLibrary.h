#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Replay/RTReplayManifest.h"
#include "Turn/RTTurnRules.h"
#include "RTMatchHistoryLibrary.generated.h"

/**
 * Una riga della lista delle partite: cio' che serve a NOMINARE una partita senza aprirla (`#416`).
 *
 * ⚠️ **Non e' il manifest, e la differenza e' il punto della issue.** Il manifest vive dentro l'archivio,
 * uno per partita, e porta gli hash di ogni turno: leggerne cento per disegnare una lista significherebbe
 * aprire cento cartelle e cento file. Questa struttura vive nell'INDICE — un file solo, fuori dagli archivi —
 * e non porta nulla che dipenda dal payload.
 */
USTRUCT(BlueprintType)
struct FRTMatchHistoryEntry
{
	GENERATED_BODY()

	/** La partita a cui la riga si riferisce: e' anche il nome della sua cartella sotto la radice. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	FGuid MatchId;

	/** Quando e' cominciata, in UTC. Tempo reale: vive qui e nel manifest, e in nessun campo che entri in un hash. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	FDateTime StartedUtc = FDateTime(0);

	/** Formato di partita (`Format.Skirmish2v2`): e' il «modo» che la lista mostra. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	FName FormatId;

	/** `true` = hex. La stessa dichiarazione del manifest, perche' una lista che mescola topologie mente. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	bool bHexTopology = true;

	/** Esito, quando la partita e' finita. `InProgress` = non e' arrivata alla fine. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	ERTMatchOutcome Outcome = ERTMatchOutcome::InProgress;

	/** Turni registrati. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	int32 TurnCount = 0;

	/** Durata in secondi reali. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	float WallClockSeconds = 0.f;

	/**
	 * Disponibilita' del replay: `true` = l'archivio e' stato chiuso, quindi la partita si riapre per intero.
	 *
	 * `false` non significa «assente»: significa **parziale**. Un archivio senza chiusura si apre lo stesso e
	 * si riproduce fino a dove arriva (`#470`) — la lista lo dice invece di lasciarlo scoprire a chi clicca.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	bool bReplayComplete = false;
};

/**
 * L'indice delle partite giocate: un file di metadati accanto agli archivi, mai dentro (`#416`).
 *
 * ```
 * Replays/
 *   history.rtindex        <- QUESTO: una riga per partita, nessun payload
 *   <MatchId>/             <- l'archivio, che l'indice non apre mai
 *     match.rtmanifest
 *     turn-001.rtlog
 * ```
 *
 * **Perche' non basta elencare le cartelle e leggere i manifest**: perche' e' esattamente la lettura che
 * questa issue esclude. Un indice separato rende la lista istantanea e — cosa che conta di piu' — la rende
 * **indipendente dal formato dell'archivio**: il giorno in cui il manifest cambiasse versione, la lista
 * continuerebbe a funzionare, e chi apre una singola partita si prenderebbe il rifiuto solo per quella.
 *
 * ⚠️ Ridondanza deliberata. Alcuni campi stanno anche nel manifest, e questa e' la definizione di un indice:
 * una copia dei metadati che si paga in scrittura per non pagarla in lettura. L'unica sorgente resta il
 * manifest — `EntryFromManifest` e' l'unico modo di costruire una riga, cosi' i due non possono divergere per
 * strade diverse.
 */
UCLASS()
class REFACTORTACTICS_API URTMatchHistoryLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Percorso del file di indice sotto la radice degli archivi. */
	static FString IndexPath(const FString& ReplaysRoot);

	/**
	 * La riga di indice di una partita, derivata dal SUO manifest — in memoria, non da disco.
	 *
	 * `StartedUtc` arriva da fuori perche' il manifest non porta l'istante d'inizio: porta una durata. Chi
	 * registra sa quando ha cominciato, e passarglielo evita di inventare un secondo orologio.
	 */
	static FRTMatchHistoryEntry EntryFromManifest(const FRTReplayManifest& Manifest, const FDateTime& StartedUtc);

	/**
	 * Inserisce o aggiorna la riga di una partita e riscrive l'indice.
	 *
	 * Aggiorna invece di accodare, e per una ragione operativa: una partita entra nell'indice quando COMINCIA
	 * — cosi' compare anche se il gioco muore — e la stessa riga si completa alla fine. Accodare darebbe due
	 * righe per la stessa partita, e la seconda non renderebbe falsa la prima.
	 */
	static bool AppendOrUpdate(const FString& ReplaysRoot, const FRTMatchHistoryEntry& Entry);

	/**
	 * Legge l'indice. `false` = non esiste o non e' leggibile; `OutEntries` resta invariato.
	 *
	 * **Non apre nessun archivio**, ed e' il criterio della issue: nessuna cartella di partita, nessun
	 * manifest, nessuna traccia. Verificabile e non dichiarativo — il test cancella gli archivi e la lista si
	 * legge lo stesso.
	 */
	static bool LoadIndex(const FString& ReplaysRoot, TArray<FRTMatchHistoryEntry>& OutEntries);
};
