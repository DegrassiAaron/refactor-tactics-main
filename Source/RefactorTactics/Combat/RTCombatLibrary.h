#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RTCombatLibrary.generated.h"

/** Esito dell'applicazione del danno: HP e scudo risultanti. */
USTRUCT(BlueprintType)
struct FRTDamageResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 Health = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 Shield = 0;

	FRTDamageResult() = default;
	FRTDamageResult(int32 InHealth, int32 InShield) : Health(InHealth), Shield(InShield) {}
};

/** Calcoli puri di combattimento (indipendenti dagli Actor, testabili). */
UCLASS()
class REFACTORTACTICS_API URTCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Applica il danno: lo scudo assorbe per primo, poi gli HP. Nessun valore scende sotto 0. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static FRTDamageResult ApplyDamage(int32 Damage, int32 Shield, int32 Health);

	/** Accumula energia con clamp in [0, Max]. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static int32 GainEnergy(int32 Current, int32 Gain, int32 Max);

	/** Vero se l'ultimate e' disponibile (energia al massimo). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static bool IsUltimateReady(int32 Energy, int32 Max);

	/** Range di movimento effettivo con gli status: Root azzera, Slow dimezza (arrotondato per difetto). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static int32 EffectiveMoveRange(int32 BaseRange, bool bRooted, bool bSlowed);
};
