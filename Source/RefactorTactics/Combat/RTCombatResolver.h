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

	/**
	 * Applica i modificatori che valgono UNA VOLTA SOLA per bersaglio: oggi `Status.Exposed` (+5 al primo
	 * danno diretto ricevuto), domani `Guard` (-15, CP 4.4). `DeltaByTarget[i]` e' il delta dell'unita' `i`
	 * (0 = nessuno); un indice fuori dall'array vale 0.
	 *
	 * Il delta va al PRIMO colpo che quel bersaglio riceve nell'ordine dato. Poiche' `ResolveAttacks` somma i
	 * danni per bersaglio, il totale non dipende da quale colpo se lo prenda: la regola «primo danno» resta
	 * ordine-indipendente (invariante #3) invece di diventare una corsa fra attaccanti.
	 *
	 * Il danno di un colpo non scende sotto 0: un delta negativo puo' annullarlo, non curare.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static TArray<FRTAttack> ApplyFirstHitDelta(const TArray<FRTAttack>& Attacks, const TArray<int32>& DeltaByTarget);
};
