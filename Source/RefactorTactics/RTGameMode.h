#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RTGameMode.generated.h"

/**
 * GameMode dell'MVP: imposta camera e controller di default e, se assente,
 * genera la griglia visuale all'avvio (cosi' basta un livello vuoto per giocare).
 */
UCLASS()
class REFACTORTACTICS_API ARTGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARTGameMode();

protected:
	virtual void BeginPlay() override;
};
