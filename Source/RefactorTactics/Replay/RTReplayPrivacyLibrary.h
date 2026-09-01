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
 * Una voce del **Replay Pubblico/Sanitizzato**.
 *
 * 🔴 **E' un tipo proprio, e non un `FRTTurnLogEntry` con meno valori riempiti.** La differenza e'
 * l'intera ragione per cui esiste: un campo di audit non ci finisce «per errore» perche' non c'e' un
 * campo dove finire. Una voce sanitizzata a runtime avrebbe la stessa forma della voce completa, e la
 * separazione sarebbe una convenzione — che e' precisamente cio' che l'AC di `#1805` vieta.
 *
 * ⚠️ **Non e' lo stesso formato con meno campi.** `ERTTurnLogFormatVersion` e' alla `v7` e la sua
 * disciplina e' *«accodare in coda, mai inserire in mezzo»*: sottrarre un campo da quel formato
 * romperebbe il suo lettore. Questo prodotto avra' la propria versione quando avra' un serializzatore.
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
};

/**
 * Il confine fra i due prodotti di [D-276]: qui, e in nessun altro posto.
 */
UCLASS()
class REFACTORTACTICS_API URTReplayPrivacyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** La classificazione, campo per campo, di `FRTTurnLogEntry`. */
	static const TMap<FName, ERTReplayFieldVisibility>& FieldVisibility();

	/** Da traccia di audit a traccia pubblica. */
	static TArray<FRTPublicReplayEntry> ToPublicTrace(const TArray<FRTTurnLogEntry>& AuditTrace);
};
