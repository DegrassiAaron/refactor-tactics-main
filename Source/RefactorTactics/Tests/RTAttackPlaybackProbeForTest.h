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

	/**
	 * Quanti colpi sono arrivati con il soggetto RISOLTO a un Actor, e quanti no.
	 *
	 * 🔴 Serve perche' il conteggio dei tick non discrimina: da #1800 l'evento porta uno `StableUnitId` e la
	 * presentazione lo ritrasforma in `ARTUnit*` con `UnitByStableId`. Se quella porta rispondesse sempre
	 * `nullptr` il delegate scatterebbe **lo stesso**, con gli stessi tick, e il test dello scaglionamento
	 * resterebbe verde su una risoluzione completamente rotta.
	 *
	 * L'atteso e' **zero non risolti**: `DestroyDefeatedUnits` gira in `ConcludeTurn`, cioe' DOPO il
	 * playback, quindi mentre i colpi si mostrano ogni Actor coinvolto esiste ancora.
	 */
	int32 SourcesResolved = 0;
	int32 SourcesUnresolved = 0;
	int32 TargetsResolved = 0;
	int32 TargetsUnresolved = 0;

	UFUNCTION()
	void OnAttackResolved(ARTUnit* Source, ARTUnit* Target, int32 Amount)
	{
		AttackTicks.Add(CurrentTick);
		(Source ? SourcesResolved : SourcesUnresolved)++;
		(Target ? TargetsResolved : TargetsUnresolved)++;
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
