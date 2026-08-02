#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/RTTypes.h"
#include "RTGameMode.generated.h"

class ARTUnit;

/**
 * GameMode dell'MVP: imposta camera e controller di default e, all'avvio,
 * allestisce un demo barebone (griglia + luce + board 2v2) se il livello e' vuoto.
 */
UCLASS()
class REFACTORTACTICS_API ARTGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARTGameMode();

protected:
	virtual void BeginPlay() override;

private:
	ARTUnit* SpawnUnit(int32 TeamId, const FRTGridCoord& Cell, bool bGuardian, const FVector& GridOrigin, float CellSize);
};
