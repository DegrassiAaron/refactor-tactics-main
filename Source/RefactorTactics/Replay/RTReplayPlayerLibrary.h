#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Replay/RTReplayManifest.h"
#include "Replay/RTReplaySeekLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnRules.h"
#include "RTReplayPlayerLibrary.generated.h"

/**
 * Perche' un archivio non si e' aperto. `Opened` e' l'unico esito che permette di riprodurre.
 *
 * Esiste perche' [ADR-0009](../../../docs/decisions/adr-0009-replay-logico-canonico.md) §4 chiede al Player
 * di **rifiutare in apertura** cio' che non sa leggere — e un `false` senza motivo manda chi diagnostica a
 * indovinare fra «manca il file», «e' di un'altra versione» e «e' di un'altra topologia», che sono tre
 * problemi diversi con tre rimedi diversi.
 */
UENUM(BlueprintType)
enum class ERTReplayOpenResult : uint8
{
	/** L'archivio e' leggibile: le tracce sono in memoria e il cursore e' all'inizio. */
	Opened,
	/** Nessun manifest sotto quel `MatchId`, o manifest illeggibile/di versione sconosciuta. */
	ManifestUnreadable,
	/** Il manifest dichiara una topologia che questo Player non riproduce. */
	TopologyMismatch,
	/** Una traccia dichiarata dal manifest manca, e' troncata o non supera il proprio checksum. */
	TraceUnreadable
};

/**
 * Una partita aperta per la riproduzione: il manifest, le sue tracce e la posizione corrente.
 *
 * ⚠️ **Non c'e' stato di gioco qui dentro, e non e' una dimenticanza**: il Player non ricostruisce unita',
 * salute o mappa, perche' ricostruirle significherebbe **calcolarle**. Emette le voci che la traccia porta;
 * chi le disegna e' la presentazione (`#472`).
 */
USTRUCT(BlueprintType)
struct FRTReplaySession
{
	GENERATED_BODY()

	/** Header della partita aperta. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	FRTReplayManifest Manifest;

	/**
	 * Una traccia per turno, nell'ordine dei turni. E' la sequenza che `URTReplaySeekLibrary` indicizza.
	 *
	 * ⚠️ `TArray<TArray<>>` non e' esponibile a Blueprint: resta C++, come tutto cio' che il Player consuma.
	 */
	TArray<TArray<FRTTurnLogEntry>> Traces;

	/** Posizione corrente nella sequenza. Si muove con il seek di `#415`, che non si riscrive. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	FRTReplayCursor Cursor;

	/**
	 * `true` = il manifest era chiuso: la partita si riproduce fino in fondo.
	 *
	 * `false` = archivio **parziale**. Si riproduce fino a dove arriva, e chi lo apre lo sa prima di
	 * cominciare invece di scoprirlo quando la riproduzione si ferma senza spiegazioni.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	bool bComplete = false;
};

/**
 * Il Player: riproduce una partita registrata **senza ricalcolarla** (`#470`).
 *
 * ⚠️ **Vive dove il resolver non e' raggiungibile**, e non per disciplina: guarda gli `#include` di questo
 * file — `CoreMinimal`, la libreria di Blueprint, il manifest, il seek, il TurnLog e le regole di turno.
 * Nessuna di quelle catene arriva a `RTCombatResolver`, `RTHexSimLibrary` o `ARTTurnManager`, quindi qui
 * «il Player non chiama il resolver» non e' una promessa da mantenere: e' **l'assenza della possibilita' di
 * violarla** ([ADR-0009](../../../docs/decisions/adr-0009-replay-logico-canonico.md) §3, rischio `REPLAY-04`).
 * E' costruito accanto a `URTReplaySeekLibrary`, che aveva gia' questa proprieta', e non altrove.
 *
 * Il contratto e' quello dell'ADR §2: **autorevole la traccia**. Non calcola collisioni, danni, reazioni, KO,
 * legalita' dei percorsi ne' targeting. Li legge.
 *
 * ### Cosa NON si puo' chiedergli
 *
 * **L'ordine di emissione originale.** `ARTTurnManager` chiama `SortTurnLog` *dentro* la risoluzione: l'ordine
 * in cui le voci sono nate e' perso **in memoria**, prima ancora della serializzazione. Cio' che resta e' la
 * forma canonica di `EntryLess` (`D-SR-1`), dove `Phase` e' la prima chiave. Il Player emette quindi lo
 * **stesso insieme** di voci, raggruppate per fase e in ordine canonico dentro la fase — che e' cio' che la
 * traccia porta. Chiedergli l'ordine di emissione sarebbe chiedere alla traccia un dato che non ha, e
 * un'implementazione che «lo soddisfacesse» starebbe fingendo in modo invisibile.
 */
UCLASS()
class REFACTORTACTICS_API URTReplayPlayerLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Apre un archivio: legge il manifest, carica le tracce dichiarate, posiziona il cursore all'inizio.
	 *
	 * **Rifiuta in apertura e non verifica piu' nulla dopo** (ADR-0009 §4): versione del manifest, topologia e
	 * integrita' di ogni traccia (`LoadTurnLogFromFile` e' gia' fail-closed su magic, versione, troncamento e
	 * checksum). Da qui in poi la riproduzione non controlla: controllare a meta' riproduzione lascerebbe chi
	 * guarda davanti a uno stato che nessuno ha finito di mostrargli.
	 *
	 * Un archivio **parziale** si apre: `bComplete` resta `false` e si riproduce fino a dove arriva. Un
	 * archivio parziale non e' un archivio rotto.
	 */
	static ERTReplayOpenResult OpenArchive(const FString& ReplaysRoot, const FGuid& MatchId,
		FRTReplaySession& OutSession);

	/**
	 * Le voci della fase in cui si trova il cursore, e avanza il cursore alla fase successiva.
	 *
	 * E' l'unita' di emissione del Player: la traccia raggruppa per fase (`Phase` e' la prima chiave di
	 * `EntryLess`) e `ERTMatchPhase` e' cronologico, quindi «la prossima fase» e' una domanda che la forma
	 * canonica sa gia' rispondere — senza ricostruire niente.
	 *
	 * `false` = la sequenza e' finita e il cursore non si e' mosso.
	 */
	static bool AdvancePhase(FRTReplaySession& Session, ERTMatchPhase& OutPhase,
		TArray<FRTTurnLogEntry>& OutEntries);

	/** Riporta il cursore all'inizio della sequenza. Riaprire un replay non richiede di rileggere il disco. */
	static void Rewind(FRTReplaySession& Session);

	/**
	 * Posiziona la riproduzione all'inizio di un turno, riusando il seek di `#415`.
	 *
	 * Non se ne scrive un secondo, ed e' un requisito della issue: due meccanismi di posizionamento
	 * divergerebbero al primo caso limite, e sarebbe quello a decidere cosa il giocatore vede.
	 */
	static ERTReplaySeekResult SeekToTurn(FRTReplaySession& Session, int32 TurnNumber);
};
