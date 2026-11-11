#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RTLockInCommittedProbeForTest.generated.h"

/**
 * Sonda per `ARTTurnManager::OnLockInCommitted` (#2359).
 *
 * ⚠️ **Un `UObject` e non una lambda**: `FRTLockInCommittedSignature` e' un
 * `DECLARE_DYNAMIC_MULTICAST_DELEGATE`, e `AddDynamic` pretende una `UFUNCTION` — che una lambda non
 * puo' essere. E' l'unica ragione per cui questo tipo esiste; stessa forma e stessa motivazione di
 * `URTAttackPlaybackProbeForTest` e `URTVeilProbeForTest`.
 *
 * Conta invece di limitarsi a un booleano, perche' le due domande sono diverse: «e' scattato» la
 * risponde anche un flag, «e' scattato UNA volta» no. Un commit che facesse doppio broadcast
 * lascerebbe il flag verde.
 */
UCLASS()
class URTLockInCommittedProbeForTest : public UObject
{
	GENERATED_BODY()

public:
	/** Quante volte il commit e' stato annunciato da quando la sonda si e' agganciata. */
	int32 Broadcasts = 0;

	UFUNCTION()
	void OnLockInCommitted()
	{
		++Broadcasts;
	}
};
