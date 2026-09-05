#pragma once

#include "CoreMinimal.h"
#include "Net/UnrealNetwork.h" // DOREPLIFETIME: la fixture dichiara davvero una proprieta' replicata
#include "RTServerOnlyGuardFixturesForTest.generated.h"

/**
 * IL LEAK PIANTATO APPOSTA — il controllo positivo di `RTServerOnlyGuard`.
 *
 * 🔴 **Questo file esiste perche' la guardia nasce verde per assenza, e nessuno se ne accorgerebbe.**
 * Misurato su `fac75cff`: zero proprieta' replicate in tutto `Source/`. Uno sweep «nessun tipo server-only
 * e' raggiungibile da una proprieta' replicata» girerebbe su un insieme vuoto contro un insieme vuoto —
 * verde per costruzione, e **verde anche a guardia completamente rotta**. Le due condizioni producono lo
 * stesso output, quindi il verde dello sweep da solo non dimostra niente.
 *
 * Qui c'e' una violazione **vera**: una struct dichiarata server-only, raggiunta da due `UPROPERTY(Replicated)`
 * — una diretta e una a due salti, attraverso una struct e un `TArray`. Il test
 * `Privacy.GuardDetectsAPlantedLeak` asserisce che la guardia le trovi entrambe. Quando quel test e' verde,
 * il verde dello sweep comincia a significare qualcosa.
 *
 * ⛔ **NON RIMUOVERE «per pulizia», e non togliere il `Replicated`.** Sono l'unico oracolo della guardia:
 * senza, il gate resta verde per sempre, anche il giorno in cui smette di guardare.
 *
 * ⚠️ **Perche' non e' dentro `#if WITH_DEV_AUTOMATION_TESTS`**: UHT processa gli header e non compila i
 * `UCLASS`/`USTRUCT` dentro rami di preprocessore condizionali in modo affidabile. Segue la convenzione
 * gia' in uso qui — `RTGameModeLevelSpyForTest.h`, `RTAuthoredArenaForTest.h` — che sono `UCLASS` non
 * condizionati. La classe non viene istanziata da nessuna parte in gioco.
 *
 * ⚠️ **La `RTServerOnlyGuardFixture` la esclude dallo sweep di produzione**, e lo sweep **conta** le
 * esclusioni: un'esclusione che cresce in silenzio e' il modo in cui un gate smette di coprire senza mai
 * diventare rosso.
 */

/** La struct che finge di essere server-only. Il bersaglio del controllo positivo. */
USTRUCT(meta = (RTServerOnly, RTServerOnlyGuardFixture))
struct FRTServerOnlyGuardPlantedSecret
{
	GENERATED_BODY()

	/** Il campo che non dovrebbe mai partire. Il suo valore non conta: conta che il tipo sia raggiungibile. */
	UPROPERTY()
	int32 Secret = 0;
};

/**
 * Un livello di annidamento fra la proprieta' replicata e il segreto.
 *
 * ⚠️ **NON e' marcata server-only**, ed e' il punto: la guardia deve arrivare al segreto *attraverso* un
 * tipo innocente e un `TArray`. Una guardia che guardasse solo il tipo diretto della proprieta' replicata
 * mancherebbe esattamente il leak indiretto per cui esiste.
 */
USTRUCT(meta = (RTServerOnlyGuardFixture))
struct FRTServerOnlyGuardInnocentWrapper
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FRTServerOnlyGuardPlantedSecret> Items;
};

/** Il portatore: due proprieta' replicate, una diretta e una annidata. */
UCLASS(meta = (RTServerOnlyGuardFixture))
class URTServerOnlyGuardLeakyCarrierForTest : public UObject
{
	GENERATED_BODY()

public:
	/** Rotta diretta: il tipo della proprieta' replicata **e'** il tipo server-only. */
	UPROPERTY(Replicated)
	FRTServerOnlyGuardPlantedSecret Direct;

	/** Rotta a due salti: proprieta' replicata -> wrapper innocente -> `TArray` -> tipo server-only. */
	UPROPERTY(Replicated)
	FRTServerOnlyGuardInnocentWrapper Nested;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override
	{
		Super::GetLifetimeReplicatedProps(OutLifetimeProps);
		DOREPLIFETIME(URTServerOnlyGuardLeakyCarrierForTest, Direct);
		DOREPLIFETIME(URTServerOnlyGuardLeakyCarrierForTest, Nested);
	}
};
