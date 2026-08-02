#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RTCombatResolver.generated.h"

/** Stato di combattimento di un'unita' (HP e scudo). */
USTRUCT(BlueprintType)
struct FRTUnitCombatState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Combat")
	int32 Health = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Combat")
	int32 Shield = 0;

	FRTUnitCombatState() = default;
	FRTUnitCombatState(int32 InHealth, int32 InShield) : Health(InHealth), Shield(InShield) {}
};

/** Un attacco pianificato: colpisce l'unita' TargetIndex con Power danni. */
USTRUCT(BlueprintType)
struct FRTAttack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Combat")
	int32 TargetIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Combat")
	int32 Power = 0;

	FRTAttack() = default;
	FRTAttack(int32 InTarget, int32 InPower) : TargetIndex(InTarget), Power(InPower) {}
};

/**
 * Risoluzione simultanea degli attacchi della fase Blast.
 * "Raccogli poi applica": i danni sono calcolati sullo stato iniziale (snapshot),
 * sommati per bersaglio e applicati insieme. Un'unita' colpita a morte infligge comunque
 * il proprio danno (il risultato non dipende dall'ordine degli attacchi).
 */
UCLASS()
class REFACTORTACTICS_API URTCombatResolver : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Ritorna il nuovo stato di ogni unita' (stesso ordine dell'input). */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Combat")
	static TArray<FRTUnitCombatState> ResolveAttacks(const TArray<FRTUnitCombatState>& Units, const TArray<FRTAttack>& Attacks);
};
