#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Frontend/RTFrontendNavigator.h"
#include "RTFrontendMatchListenerForTest.generated.h"

/**
 * Ascoltatore di prova per `URTFrontendNavigator::OnMatchRequested` (CP 46.4, #939).
 *
 * ⚠️ **Un `UObject` e non una lambda**: `AddDynamic` richiede una `UFUNCTION`, che una lambda non puo'
 * essere. E' l'unica ragione per cui questo tipo esiste.
 *
 * Si comporta come il consumatore vero — sente, CONSUMA, registra — perche' un ascoltatore che non
 * consumasse proverebbe solo che il broadcast parte, non che chi lo riceve trova la richiesta gia'
 * pendente. Quello e' l'ordine che il contratto promette, ed e' cio' che va verificato.
 */
UCLASS()
class URTFrontendMatchListenerForTest : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<URTFrontendNavigator> Nav;

	/** Quante volte e' stato avvisato. */
	int32 Calls = 0;

	/** Il livello annunciato. */
	FString SeenLevel;

	/** Se al momento dell'annuncio la richiesta era davvero pendente e consumabile. */
	bool bConsumedSomething = false;

	UFUNCTION()
	void OnRequested(const FString& LevelName)
	{
		++Calls;
		SeenLevel = LevelName;
		if (Nav)
		{
			bConsumedSomething = !Nav->ConsumePendingMatchLevel().IsEmpty();
		}
	}
};
