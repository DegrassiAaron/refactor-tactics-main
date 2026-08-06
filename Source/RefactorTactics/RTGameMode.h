#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Templates/SubclassOf.h"
#include "Core/RTTypes.h"
#include "RTGameMode.generated.h"

class ARTUnit;
class ARTHexMapActor;
class URTHeroData;

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

	/**
	 * Eroi della squadra 0 (giocatore) e della squadra 1 (bot), per `HeroId` del catalogo eroi v0.1.
	 *
	 * Default: **Flux + Riva** contro **Bastion + Vektor**. Le due coppie non sono casuali — Riva bagna e
	 * Flux fulmina (`+8` su `Status.Wet`), Bastion costruisce e Vektor sfrutta lo spazio: ogni squadra ha una
	 * combo interna giocabile, che e' l'unico modo di vedere in partita cio' che CP 6.2/6.3 hanno costruito.
	 *
	 * E' un DATO e non una scelta scritta nel codice: cambiare formazione non richiede ricompilare, e quando
	 * la selezione pre-partita esistera' (north-star) questa restera' solo il valore di partenza.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Units")
	TArray<FName> Team0Heroes = { TEXT("Hero.Flux"), TEXT("Hero.Riva") };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Units")
	TArray<FName> Team1Heroes = { TEXT("Hero.Bastion"), TEXT("Hero.Vektor") };

	/**
	 * Classe visiva per `HeroId` (es. `BP_Unit_Flux` con skeletal mesh). Un eroe assente da questa mappa
	 * ricade su `ARTUnit` — il cilindro segnaposto — che resta il comportamento di ripiego di sempre: un
	 * personaggio senza asset si vede lo stesso e la partita si gioca.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Units")
	TMap<FName, TSubclassOf<ARTUnit>> HeroUnitClasses;

protected:
	virtual void BeginPlay() override;

private:
	/** Applica `MapSource` all'actor mappa: sostituisce l'asset quando la scelta lo richiede, e lo dichiara nel log. */
	void ApplyMapSource(ARTHexMapActor* HexMap);

	/**
	 * Spawna l'eroe con l'`HeroId` dato. `Hero == nullptr` non spawna nulla (fail-closed): un'unita' con
	 * statistiche di default al posto di un eroe sarebbe piu' difficile da diagnosticare di un'unita' assente.
	 */
	ARTUnit* SpawnHero(int32 TeamId, const URTHeroData* Hero, const FRTCellId& InCell, const FVector& Origin,
		float HexSize, float LayerHeight);
};
