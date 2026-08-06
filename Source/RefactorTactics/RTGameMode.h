#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Templates/SubclassOf.h"
#include "Core/RTTypes.h"
#include "RTGameMode.generated.h"

class ARTUnit;
class ARTHexMapActor;

/**
 * Sorgente della mappa su cui allestire la partita. Le voci generate non richiedono asset: `Content/**` non e'
 * versionato, quindi una mappa generata da codice e' l'unica che sopravvive a un clone.
 */
UENUM(BlueprintType)
enum class ERTMapSource : uint8
{
	/** La mappa d'autore assegnata all'`ARTHexMapActor` del livello. Se manca o e' vuota si ripiega sull'arena demo. */
	LevelAsset,

	/** Arena di ripiego generata: esagono pieno di `DemoArenaRadius`, pavimento liscio. Un fondo di scena giocabile. */
	GeneratedDemoArena,

	/** Mappa di PROVA generata: ostacoli, muri che bloccano la vista, terreno costoso e piattaforma su un secondo layer. */
	GeneratedTestArena
};

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
	 * Da dove arriva l'arena su cui si gioca. E' una **scelta**, non una catena di flag: i modi di lanciare una
	 * partita cresceranno (mappe d'autore, arene generate, in futuro scenari e tutorial) e ognuno va aggiunto
	 * come voce qui, non come booleano a parte.
	 *
	 * Le voci **generate** valgono anche se il livello porta una mappa d'autore: sceglierle esplicitamente
	 * significa volerle, e la sostituzione e' dichiarata nel log.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Map")
	ERTMapSource MapSource = ERTMapSource::LevelAsset;

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
	/** Applica `MapSource` all'actor mappa: sostituisce l'asset quando la scelta lo richiede, e lo dichiara nel log. */
	void ApplyMapSource(ARTHexMapActor* HexMap);

	ARTUnit* SpawnUnit(int32 TeamId, const FRTCellId& InCell, bool bGuardian, const FVector& Origin,
		float HexSize, float LayerHeight);
};
