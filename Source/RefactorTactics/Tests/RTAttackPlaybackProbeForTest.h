#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RTAttackPlaybackProbeForTest.generated.h"

class ARTUnit;

/**
 * Sonda per `ARTTurnManager::OnAttackResolved` (#911).
 *
 * ⚠️ **Un `UObject` e non una lambda**: `AddDynamic` richiede una `UFUNCTION`, che una lambda non puo'
 * essere. E' l'unica ragione per cui questo tipo esiste — stessa forma, e stessa motivazione, di
 * `URTFrontendMatchListenerForTest`.
 *
 * Registra **quando** ogni colpo viene rivelato, non quanti ne arrivano: il conteggio finale e' identico
 * con e senza scaglionamento — e' il difetto di #911 che il totale non lo vedeva. Cio' che distingue le due
 * strade e' la DISTRIBUZIONE nel tempo, quindi la sonda annota il tick di risoluzione corrente, che il
 * test le comunica prima di ogni `Tick`.
 */
UCLASS()
class URTAttackPlaybackProbeForTest : public UObject
{
	GENERATED_BODY()

public:
	/** Tick di risoluzione in corso, scritto dal test prima di ogni `ARTTurnManager::Tick`. */
	int32 CurrentTick = -1;

	/** Un elemento per colpo rivelato: il tick in cui e' arrivato. */
	TArray<int32> AttackTicks;

	UFUNCTION()
	void OnAttackResolved(ARTUnit* Source, ARTUnit* Target, int32 Amount)
	{
		AttackTicks.Add(CurrentTick);
	}

	/**
	 * Il massimo numero di colpi rivelati **dentro lo stesso tick**.
	 *
	 * E' la misura discriminante: con i colpi scaglionati vale 1, con i colpi svuotati in blocco vale
	 * quanti sono i colpi di quella fase.
	 */
	int32 MaxAttacksInOneTick() const
	{
		TMap<int32, int32> PerTick;
		for (const int32 T : AttackTicks) { ++PerTick.FindOrAdd(T); }

		int32 Max = 0;
		for (const TPair<int32, int32>& Pair : PerTick) { Max = FMath::Max(Max, Pair.Value); }
		return Max;
	}
};
