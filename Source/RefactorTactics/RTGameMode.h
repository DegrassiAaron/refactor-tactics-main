#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Templates/SubclassOf.h"
#include "Core/RTTypes.h"
#include "RTGameMode.generated.h"

class ARTUnit;
class ARTHexMapActor;

/**
 * GameMode: imposta camera e controller di default e, all'avvio, allestisce la partita sulla mappa
 * ESAGONALE presente nel livello (mappa + luce + board 2v2) se il livello non ha gia' delle unita'.
 */
UCLASS()
class REFACTORTACTICS_API ARTGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARTGameMode();

	/**
	 * Posa la board 2v2 sulle celle di partenza della mappa esagonale. Non fa nulla se ci sono gia' unita'
	 * nel livello o se la mappa non ha abbastanza celle percorribili (non si allestisce una partita a meta').
	 * Pubblico e separato da BeginPlay per essere verificabile headless, senza il ciclo di vita del GameMode.
	 */
	void SetupHexMatch(ARTHexMapActor* HexMap);

	/**
	 * Raggio dell'arena di RIPIEGO, usata quando il livello non porta una mappa esagonale **con celle**
	 * (asset assente oppure presente ma vuoto). 0 = nessun ripiego: la partita non si allestisce e il log lo dice.
	 * Pubblico come `SetupHexMatch`: serve ai test dell'allestimento headless.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Map")
	int32 DemoArenaRadius = 4;

protected:
	virtual void BeginPlay() override;

	/** Classe da spawnare per il Ranger (es. BP_Unit con skeletal mesh). Vuota = ARTUnit cilindro (fallback). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Units")
	TSubclassOf<ARTUnit> RangerUnitClass;

	/** Classe da spawnare per il Guardian (es. BP_Unit con skeletal mesh). Vuota = ARTUnit cilindro (fallback). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Units")
	TSubclassOf<ARTUnit> GuardianUnitClass;

private:
	ARTUnit* SpawnUnit(int32 TeamId, const FRTCellId& InCell, bool bGuardian, const FVector& Origin,
		float HexSize, float LayerHeight);
};
