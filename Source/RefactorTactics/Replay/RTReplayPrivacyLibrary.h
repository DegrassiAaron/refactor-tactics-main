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
};

/**
 * Il confine fra i due prodotti di [D-276].
 *
 * ⚠️ **Il confine e' qui per i CAMPI, e non esiste ancora per le VOCI.** Non e' una sfumatura: il filtro
 * per osservatore — *«questa unita' la mia squadra l'aveva vista?»* — richiede la `TeamKnowledge` del
 * turno, e la traccia archiviata **non la porta** (`FRTTurnLogEntry::Verdict` e' `Transient` per `D-223`).
 * Finche' quel dato non esiste in forma durevole, chi legge un replay pubblico vede i fatti di **entrambe**
 * le squadre: e' il difetto che `#1525` osserva sul playback, e questa libreria non lo chiude.
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
};
