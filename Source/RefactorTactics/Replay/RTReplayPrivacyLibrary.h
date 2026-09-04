#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnRules.h"
#include "RTReplayPrivacyLibrary.generated.h"

/**
 * Chi puo' leggere un campo della traccia, secondo
 * [D-276](../../../docs/decisions/RT_PDR_00_Decision_Log.md).
 */
UENUM()
enum class ERTReplayFieldVisibility : uint8
{
	/** Va nel Replay Pubblico/Sanitizzato: e' un fatto che lo spettatore e' autorizzato a vedere. */
	Public,

	/** Resta nella Traccia di Audit Privata: e' evidenza di come si e' deciso, non di cosa e' successo. */
	AuditOnly
};

/**
 * Una voce del **Replay Pubblico/Sanitizzato** di [D-276].
 *
 * 🔴 **E' un tipo proprio, e non un `FRTTurnLogEntry` con meno valori riempiti.** La differenza e' l'intera
 * ragione per cui esiste: un campo di audit non ci finisce «per errore» perche' non c'e' un campo dove
 * finire. Una voce sanitizzata a runtime avrebbe la stessa forma della voce completa, e la separazione
 * sarebbe una convenzione — che e' precisamente cio' che l'AC di `#1805` vieta.
 *
 * ⚠️ **Non e' lo stesso formato con meno campi.** `ERTTurnLogFormatVersion` e' alla `v7` e la sua disciplina
 * e' *«accodare in coda, mai inserire in mezzo»*: sottrarre un campo da quel formato romperebbe il suo
 * lettore. Questo prodotto avra' la propria versione quando avra' un serializzatore.
 *
 * 🔴 **E chi scrivera' quel serializzatore deve sapere questo: il prodotto pubblico NON ha un ordine
 * canonico proprio.** `EntryLess` scioglie i pareggi con `OpportunityId`, `ReactionInstanceId` e
 * `ReactionResponse`, che qui non ci sono: due decisioni di reazione della stessa unita' nello stesso
 * micro-step arrivano qui **identiche**, e la loro differenza sopravvive solo come posizione nell'array.
 * ∴ l'ordine di questo prodotto e' **ereditato** da quello della traccia di audit e non si ricalcola. Un
 * serializzatore che riordinasse con un `TArray::Sort` — che non e' stabile — produrrebbe due byte diversi
 * per la stessa partita, e l'invariante 3 di `AGENTS.md` §3 non avrebbe piu' risposta.
 */
USTRUCT(BlueprintType)
struct FRTPublicReplayEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	ERTMatchPhase Phase = ERTMatchPhase::Move;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	ERTLogCategory Category = ERTLogCategory::Move;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	uint8 Outcome = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	FRTCellId SrcCell;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	FRTCellId TgtCell;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	int32 Amount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	FName ActionId;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	FName BaseActionId;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	int32 UnitId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	int32 TurnNumber = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	int32 GraphRevision = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	int32 Priority = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	int32 SelectedTargetUnitId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	int32 OriginalTargetUnitId = INDEX_NONE;

	/**
	 * Il micro-step in cui la voce e' nata (`#1880`): la terza coordinata del boundary, e come le altre due
	 * — `Phase` e `TurnNumber` — dice *quando*, non *cosa e' stato deciso*. Pubblico per la stessa ragione
	 * per cui lo sono quelle: e' il momento di un fatto gia' pubblico, non un'intenzione.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	int32 MicroStepIndex = 0;
};

/**
 * Il confine fra i due prodotti di [D-276].
 *
 * ⚠️ **Sono DUE confini e vivono in due momenti**, ed e' la cosa da sapere prima di leggere il resto: i
 * **CAMPI** si tolgono in lettura (`ToPublicTrace`, qui sotto), le **VOCI** si filtrano alla registrazione
 * (`FilterEntriesForObserver`, [D-316]).
 *
 * 🔴 **Perche' non nello stesso posto.** Il filtro per osservatore — *«questa unita' la mia squadra l'aveva
 * vista?»* — ha bisogno del verdetto del turno, e la traccia archiviata **non lo porta**:
 * `FRTTurnLogEntry::Verdict` e' `UPROPERTY(Transient)` e `SerializeTurnLog` non lo scrive. In lettura quel
 * dato e' azzerato, e `AllowsTeam` e' fail-closed: un filtro qui non nasconderebbe troppo poco,
 * nasconderebbe **tutto**.
 *
 * ⚠️ **L'attribuzione va tenuta dritta, perche' la prima stesura di questo file la sbagliava.** [D-223]
 * decide che un canale che racconta il PASSATO porta il verdetto **congelato quando il fatto e' accaduto**
 * invece di ricalcolarlo in lettura: non parla di serializzazione, e semmai ABILITA un'evidenza durevole,
 * perche' un verdetto congelato alla scrittura e' proprio cio' che si potrebbe persistere. A tenerlo fuori
 * dal formato e' la nota sul campo stesso — *«un verdetto e' una risposta alla presentazione, non un fatto
 * della simulazione»* — con le conseguenze che elenca: versione del formato, `EntryLess`, `MixEntryFields`
 * e i golden. Chi cerchera' in [D-223] il permesso di cambiare idea non lo trovera' ne' in un senso ne'
 * nell'altro.
 * ✅ **Il confine per le VOCI esiste dal 2026-09-03** — [D-316], `#2098`. Non e' stato ottenuto serializzando
 * il verdetto: e' stato ottenuto **spostando il momento**. Il verdetto e' vivo in memoria quando la voce
 * nasce (`ARTTurnManager::AppendLogEntry` lo congela li'), quindi il prodotto pubblico si filtra **alla
 * registrazione**, e la traccia archiviata non ha mai bisogno di portarlo.
 *
 * ⚠️ **Per un anno la lettura corrente e' stata che «filtrare il replay richiede il verdetto nella
 * traccia»**, e quella frase e' vera solo se si decide **alla lettura**. Il dato non mancava: si cercava di
 * usarlo nel momento sbagliato.
 *
 * 🔴 **Ed e' la sola forma che il canone ammetteva**, non una fra tre pari.
 * [`conoscenza-parziale-visibile-spec.md`](../../../docs/technical/systems/conoscenza-parziale-visibile-spec.md)
 * §3.5 mette il **combat log** nella colonna *«alla scrittura»* da [D-223]: filtrare in lettura avrebbe
 * contraddetto una decisione gia' presa, non aggiunto un'opzione.
 */
UCLASS()
class REFACTORTACTICS_API URTReplayPrivacyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * La classificazione, campo per campo, di `FRTTurnLogEntry`.
	 *
	 * E' la **sola** sorgente di verita': `ToPublicTrace` copia leggendo di qui, e
	 * `RefactorTactics.Replay.Privacy.EveryLoggedFieldIsClassified` rende rosso chi aggiunge un campo senza
	 * classificarlo.
	 */
	static const TMap<FName, ERTReplayFieldVisibility>& FieldVisibility();

	/** Da traccia di audit a traccia pubblica: l'unico ponte fra i due prodotti. */
	static TArray<FRTPublicReplayEntry> ToPublicTrace(const TArray<FRTTurnLogEntry>& AuditTrace);

	/**
	 * Il confine per le **VOCI**: tiene solo i fatti che `ObserverTeamId` era autorizzato a conoscere quando
	 * sono accaduti ([D-316], `#2098`).
	 *
	 * 🔴 **Si chiama alla REGISTRAZIONE, non alla lettura**, ed e' l'intera decisione. `FRTTurnLogEntry::Verdict`
	 * e' `Transient` — su disco non c'e' — ma e' **vivo** nell'istante in cui la voce nasce, perche'
	 * `ARTTurnManager::AppendLogEntry` lo congela li'. Chiamare di qui mentre la partita gira usa un dato che
	 * esiste; chiamare da un viewer che ha appena deserializzato userebbe una maschera azzerata, e per
	 * fail-closed **non passerebbe nulla**.
	 *
	 * ⚠️ **`ObserverTeamId < 0` significa spettatore NEUTRALE, e passa tutto.** Non e' una scorciatoia: un
	 * replay pubblicato e' gia' una rinuncia alla privacy competitiva, e nasconderne meta' a chi non e' di
	 * nessuna delle due squadre non protegge nessuno — mentre lascerebbe l'archivio senza una lettura
	 * completa che non sia il prodotto d'audit. La scelta e' [D-316] punto (5).
	 *
	 * ⛔ **Non tocca i CAMPI**: chi vuole entrambi i confini compone con `ToPublicTrace`. Sono due domande
	 * diverse — *«questa riga la posso vedere?»* e *«di questa riga quali colonne?»* — e fonderle avrebbe
	 * reso impossibile scrivere una traccia per osservatore che resti nel formato canonico.
	 */
	static TArray<FRTTurnLogEntry> FilterEntriesForObserver(const TArray<FRTTurnLogEntry>& Entries,
		int32 ObserverTeamId);
};
