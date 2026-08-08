#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Templates/SubclassOf.h"
#include "Core/RTTypes.h"
#include "RTGameMode.generated.h"

class ARTUnit;
class ARTHexMapActor;
class ARTTurnManager;
class URTHeroData;
class URTMatchFormatData;

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

	/**
	 * Formato di partita: da qui arrivano `RoundLimit` e soglia obiettivo (CP 10.3, issue #185).
	 *
	 * **Assente** ⇒ si gioca comunque, con il formato di ripiego, e il log lo dichiara: un gioco che rifiuta
	 * di partire e' peggio di uno che parte dicendo su cosa sta girando (D1). **Presente ma invalido** ⇒ la
	 * partita non si allestisce: il ripiego copre l'assenza, non il contenuto sbagliato.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Match")
	TObjectPtr<URTMatchFormatData> MatchFormat;

	/**
	 * Primo filtro della tendina degli scenari: un tag fra quelli realmente presenti nei file. Vuoto = non
	 * restringe nulla.
	 *
	 * È una **vista, non un vincolo**: restringere l'elenco non tocca mai `ScenarioToRun`. Uno scenario già
	 * scelto resta scelto e viene eseguito anche mentre i filtri mostrano altro — perché il filtro dice
	 * «cosa sto cercando adesso», non «a cosa questo scenario appartiene», e una lente che cancella una
	 * configurazione salvata sarebbe un modo elaborato di perdere lavoro.
	 *
	 * Dichiarato **prima** di `ScenarioToRun` perché il Details Panel segue l'ordine di dichiarazione, e i
	 * filtri devono stare sopra ciò che filtrano.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Test",
		meta = (GetOptions = "GetScenarioTagOptions"))
	FString ScenarioFilterA;

	/**
	 * Secondo filtro, in **intersezione** con il primo: `reactions` + `flux` mostra gli scenari che portano
	 * entrambi i tag.
	 *
	 * Due e non tre: due assi coprono il caso che serve — una tipologia incrociata con un personaggio o una
	 * lente — e il terzo diventerebbe rumore prima di diventare utile.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Test",
		meta = (GetOptions = "GetScenarioTagOptions"))
	FString ScenarioFilterB;

	/**
	 * Voci dei due filtri: il vocabolario dei tag **realmente presenti** negli scenari, più una voce vuota
	 * in testa per non filtrare. Nessun elenco dichiarato a mano, per la stessa ragione di
	 * `GetScenarioOptions`: una categoria vuota nel menu invita a cercare qualcosa che non esiste.
	 */
	UFUNCTION()
	TArray<FString> GetScenarioTagOptions() const;

	/**
	 * Scenario di test da eseguire **al posto** della partita normale (es. `Movement.Basic`).
	 * Vuoto = si gioca normalmente.
	 *
	 * È una proprietà e non solo una console variable perché così **sopravvive alla sessione**: si imposta una
	 * volta nei default di `BP_GameMode` e al primo Play lo scenario parte, senza doverla ridigitare a ogni
	 * riavvio dell'editor. La console variable `rt.Test.Scenario` resta e **prevale**, per il caso opposto:
	 * eseguire uno scenario diverso una volta sola, da riga di comando o in CI, senza toccare l'asset.
	 *
	 * Nel Details Panel è un **menu a tendina**, non una casella di testo: gli ID vengono letti dai file in
	 * `Scenarios/` (vedi `GetScenarioOptions`), quindi non si può digitare un ID inesistente e l'elenco non
	 * va tenuto allineato a mano. La prima voce è **vuota** = partita normale.
	 *
	 * L'elenco è ristretto da `ScenarioFilterA`/`ScenarioFilterB`: la combo di UE non filtra da testo, e
	 * oltre la ventina di voci scorrerla smette di essere un modo di trovare qualcosa.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Test",
		meta = (GetOptions = "GetScenarioOptions"))
	FString ScenarioToRun;

	/**
	 * Voci del menu a tendina di `ScenarioToRun`: gli scenari **realmente presenti** in `Scenarios/`, più una
	 * voce vuota in testa per tornare alla partita normale.
	 *
	 * Legge i file invece di un elenco scritto a mano: aggiungere uno scenario lo fa comparire nel menu senza
	 * toccare il codice, e un elenco che invecchia non puo' esistere.
	 */
	UFUNCTION()
	TArray<FString> GetScenarioOptions() const;

	/**
	 * Pausa in secondi **prima** di risolvere ogni turno dello scenario: il tempo per guardare dove sono le
	 * unità prima che si muovano.
	 *
	 * È ciò che rende uno scenario **osservabile**. Fino a `4e6c2e0` la partita si risolveva tutta dentro
	 * `BeginPlay` e finiva prima del primo fotogramma: non si vedeva niente, e il movimento che sembrava lo
	 * scenario erano turni fantasma. Ora la sessione avanza **un passo per frame**, e questa è la pausa fra un
	 * turno e l'altro.
	 *
	 * 0 = nessuna pausa, i turni si incatenano (il playback resta comunque visibile).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Test", meta = (ClampMin = "0.0"))
	float ScenarioTurnPauseSeconds = 1.5f;

	/** Lo scenario da eseguire: la console variable prevale sulla proprietà, altrimenti vale la proprietà. Vuoto = partita normale. */
	FString ResolveScenarioToRun() const;

	/** Sessione dello scenario in corso, fatta avanzare un passo per frame da `Tick`. Nulla = nessuno scenario. */
	TSharedPtr<class FRTScenarioSession> ScenarioSession;

	/** Vero finché lo scenario sta girando: serve alla diagnostica e ai test. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Test")
	bool IsScenarioRunning() const;

protected:
	virtual void BeginPlay() override;

	/** Fa avanzare la sessione dello scenario: un passo per frame, cosi' il playback si VEDE. */
	virtual void Tick(float DeltaSeconds) override;

private:
	/** Applica `MapSource` all'actor mappa: sostituisce l'asset quando la scelta lo richiede, e lo dichiara nel log. */
	void ApplyMapSource(ARTHexMapActor* HexMap);

	/**
	 * Risolve il formato di partita e lo consegna al `TurnManager`. Ritorna **false** se il formato e'
	 * presente ma invalido — in quel caso la partita non va allestita.
	 *
	 * E' qui che vive la politica di ripiego, come per l'arena generata: la libreria pura rifiuta e basta,
	 * l'Actor decide che farne e lo dichiara in Warning. Un ripiego silenzioso non produce una partita rotta,
	 * produce numeri di playtest attribuiti a un formato che non era in vigore.
	 */
	bool ApplyMatchFormat(ARTTurnManager* TurnManager);

	/**
	 * Spawna l'eroe con l'`HeroId` dato. `Hero == nullptr` non spawna nulla (fail-closed): un'unita' con
	 * statistiche di default al posto di un eroe sarebbe piu' difficile da diagnosticare di un'unita' assente.
	 */
	ARTUnit* SpawnHero(int32 TeamId, const URTHeroData* Hero, const FRTCellId& InCell, const FVector& Origin,
		float HexSize, float LayerHeight);
};
