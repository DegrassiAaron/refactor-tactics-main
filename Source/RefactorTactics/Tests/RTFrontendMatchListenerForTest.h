#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Frontend/RTFrontendNavigator.h"
#include "Frontend/RTFrontendGameMode.h"
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

/**
 * `ARTFrontendGameMode` che REGISTRA l'apertura invece di eseguirla (CP 46.4, #939).
 *
 * ⚠️ **`OpenMatchLevel` e' virtual per questo**: `UGameplayStatics::OpenLevel` in un test aprirebbe davvero
 * un livello, quindi il seam sta dove l'apertura avviene e non altrove. E' lo stesso criterio che
 * `RTFrontendGameMode.h` applica gia' a `ConfigureMenuInput` e `StartFrontendForThisGame` — «dentro l'hook
 * sarebbe verificabile solo in PIE, e questo e' un requisito del DoD, non un dettaglio».
 */
UCLASS()
class ARTFrontendGameModeForTest : public ARTFrontendGameMode
{
	GENERATED_BODY()

public:
	/** I livelli di cui e' stata chiesta l'apertura, in ordine. */
	TArray<FString> OpenedLevels;

	virtual void OpenMatchLevel(const FString& LevelName) override
	{
		OpenedLevels.Add(LevelName);
	}
};
