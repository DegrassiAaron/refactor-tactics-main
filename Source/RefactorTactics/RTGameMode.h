#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Templates/SubclassOf.h"
#include "Core/RTTypes.h"
// `ERTMapSource`. Vive in `Map/` e non qui da `E-SOLID` fetta 3: l'allestimento e' uscito dal GameMode, e
// un `Match/` che dovesse includere questo header per leggere un enum di mappa avrebbe la dipendenza al
// contrario. Resta incluso di qui perche' `MapSource` e' ancora una proprieta' di questo Actor.
#include "Map/RTMapSource.h"
// CP 46.2: il rapporto d'avvio. E' un tipo di **dato** — enum e struct puri, nessuna dipendenza dal
// frontend: il GameMode dichiara cosa e' successo, chi mostra la cosa sta dall'altra parte.
#include "Frontend/RTStartupReport.h"
// Il ciclo di vita a runtime degli scenari. E' un membro per VALORE, quindi serve la definizione: la
// forward declaration basterebbe solo per un puntatore, e un'indirezione qui non comprerebbe niente.
#include "ScenarioHarness/RTScenarioCoordinator.h"
// `FRTMatchRules`, per il membro `AssignedRules`: e' un valore, non un puntatore, quindi la sola forward
// declaration di `URTMatchFormatData` piu' sotto non basterebbe.
#include "Turn/RTMatchFormatData.h"
#include "RTGameMode.generated.h"

class ARTUnit;
class ARTHexMapActor;
class URTFrontendNavigator;
class URTKnowledgeVeilPresenter;
class URTMatchFormatData;

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
	 * Allestisce la partita sulla mappa esagonale. Non fa nulla se ci sono gia' unita' nel livello o se la
	 * mappa non ha abbastanza celle percorribili (non si allestisce una partita a meta').
	 * Pubblico e separato da BeginPlay per essere verificabile headless, senza il ciclo di vita del GameMode.
	 *
	 * 🔑 **Da `E-SOLID` fetta 3 e' una FACADE, e cio' che fa e' la linea del refactor**: risolve *cosa* e'
	 * stato chiesto — le tre precedenze vivono qui — passa il risultato a `FRTMatchBootstrapper`, latcha
	 * la modalita' della sessione se l'allestimento e' arrivato a deciderla, e assegna le squadre ai
	 * giocatori presenti (`AssignSeats`, **D-285**). Il *come* nasce una partita (mappa, formato, celle,
	 * roster, unita', equipaggiamento) sta dall'altra parte.
	 *
	 * ⚠️ **Resta pubblica perche' e' la porta dei test, non per comodo**: una quarantina di siti in
	 * `Tests/` la chiamano direttamente per allestire una partita vera senza far correre `BeginPlay`.
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
	 * Default: **Gadget + Phase** contro **Riktor + Wraith**. Le due coppie non sono casuali — Phase bagna e
	 * Gadget fulmina (`+8` su `Status.Wet`), Riktor costruisce e Wraith sfrutta lo spazio: ogni squadra ha una
	 * combo interna giocabile, che e' l'unico modo di vedere in partita cio' che CP 6.2/6.3 hanno costruito.
	 *
	 * E' un DATO e non una scelta scritta nel codice: cambiare formazione non richiede ricompilare, e quando
	 * la selezione pre-partita esistera' (north-star) questa restera' solo il valore di partenza.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Units")
	TArray<FName> Team0Heroes = { TEXT("Hero.Gadget"), TEXT("Hero.Phase") };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Units")
	TArray<FName> Team1Heroes = { TEXT("Hero.Riktor"), TEXT("Hero.Wraith") };

	/**
	 * Classe visiva per `HeroId` (es. `BP_Unit_Gadget` con skeletal mesh). Un eroe assente da questa mappa
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
	 * Quale formato SPEDITO usare quando `MatchFormat` non e' assegnato (issue #375).
	 *
	 * L'ordine e': asset assegnato ⇒ formato spedito con questa identita' ⇒ ripiego. Il ripiego non sparisce,
	 * arretra: copre l'id sconosciuto, non piu' l'assenza di un file che qualcuno doveva creare in editor.
	 *
	 * Serve perche' il formato canonico della v0.1 non puo' dipendere da un `.uasset` che il repository non
	 * contiene: CP 12.5 ha misurato una build pacchettizzata che girava sul RIPIEGO proprio per questo.
	 * Svuotare il campo riporta al comportamento precedente, ed e' il modo di verificare che il ripiego
	 * funzioni ancora.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Match")
	FName ShippedFormatId = FName(TEXT("Format.Skirmish2v2"));

	/**
	 * MODALITA' NON PRESIDIATA: **entrambe** le squadre passano al bot, e la partita si gioca da sola fino al
	 * vincitore senza che nessuno tocchi nulla (CP 47.1, issue #954).
	 *
	 * Il turno avanzava gia' da solo — `StartPlanningTimer` chiama `PlanBots`, `OnPlanningTimeout` chiama
	 * `LockInAndResolve`, e a fine risoluzione il timer riparte. L'unico punto che impediva l'autobattle in
	 * partita era `SpawnHero`, che mette sotto il bot la sola squadra 1: **il delta e' una configurazione,
	 * non un motore**, ed e' la ragione per cui questo e' un booleano e non un sistema.
	 *
	 * ⚠️ Il DEFAULT non cambia, ed e' pinnato da `RTHeroSpawnTests` (*«il giocatore comanda i suoi»*): senza
	 * configurazione la squadra 0 resta di chi gioca. Questa proprieta' **estende** quel contratto, non lo
	 * sostituisce.
	 *
	 * ⛔ Non e' una pipeline parallela per i bot (invariante #10): passano da `PlanBots` -> `ChooseBestPlan`
	 * come in ogni partita. Cambia **chi** e' segnato come bot, non **come** decide.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Match")
	bool bAutobattle = false;

	/**
	 * Quanti COMPAGNI del giocatore pianifica il bot, contati dal fondo di `Team0Heroes`.
	 *
	 * `0` (default) = nessuno: la squadra 0 resta tutta di chi gioca, che e' il contratto pinnato da
	 * `RTHeroSpawnTests`. `1` su `[Gadget, Phase]` lascia Gadget al giocatore e da' Phase al bot.
	 *
	 * ⚠️ **E' il fratello minore di `bAutobattle`, non un suo caso particolare**: quella toglie il giocatore
	 * dalla partita, questa gli riduce le unita' da comandare. Si sommano — `FRTMatchBootstrapper` cappa il
	 * conteggio perche' senza autobattle almeno un'unita' deve restare comandabile.
	 *
	 * ⛔ Come per l'autobattle, il delta e' **configurazione e non motore**: il compagno passa da `PlanBots`
	 * -> `ChooseBestPlan` come ogni altra unita' del bot, con lo stesso planner e lo stesso Intent. Cambia
	 * **chi** e' segnato come bot, non **come** decide (invariante #10).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Match", meta = (ClampMin = "0"))
	int32 BotAllyCount = 0;

	/**
	 * Secondi della fase di Planning, **negativo = non intervenire** e vale il valore del `TurnManager`.
	 *
	 * Esiste perche' il default e' **30 s per turno**: giusto per una partita umana, illeggibile per una demo
	 * che si guarda. Senza questa configurazione l'autobattle sarebbe acceso e inutilizzabile, cioe' il
	 * difetto starebbe *dentro* la feature che lo introduce.
	 *
	 * Non e' legata all'autobattle: vale anche in partita normale, per chi vuole un ritmo diverso. Cio' che
	 * l'autobattle aggiunge e' solo un **ripiego** quando nessuna delle tre sorgenti dice nulla — vedi
	 * `ResolveMatchPlanningSeconds`.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Match", meta = (ClampMin = "-1.0"))
	float MatchPlanningSeconds = -1.f;

	/**
	 * La modalita' in vigore: `rt.Match.Autobattle` se impostata, altrimenti `-RTAutobattle`, altrimenti la
	 * proprieta'. Il piu' specifico vince, che e' la stessa regola di `ResolveScenarioToRun`.
	 *
	 * ⚠️ Le due sorgenti esterne sanno anche **spegnere** (`0`), non solo accendere: una precedenza che sa
	 * solo accendere costringerebbe a modificare un `.uasset` per giocare una partita normale.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Match")
	bool ResolveAutobattle() const;

	/**
	 * Quanti compagni al bot: `rt.Match.BotAllies` se impostata, altrimenti `-RTBotAllies=N`, altrimenti la
	 * proprieta'. Stessa scala e stessa regola di `ResolveAutobattle()`.
	 *
	 * ⚠️ La sentinella di «non impostata» e' **negativa** e non zero, per la ragione gia' scritta accanto a
	 * `CVarRTAutobattle`: `0` e' una richiesta legittima — «nessun compagno al bot» — e non puo' voler dire
	 * anche «non ho chiesto nulla», altrimenti la console non saprebbe SPEGNERE quello che la proprieta'
	 * accende.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Match")
	int32 ResolveBotAllies() const;

	/**
	 * La modalita' in vigore per QUESTA sessione, **decisa una volta** in `SetupHexMatch`.
	 *
	 * ⚠️ Non e' un doppione di `ResolveAutobattle()`, ed e' la differenza fra cio' che si puo' *chiedere* e
	 * cio' che la partita *e'*. `bIsBotControlled` si scrive all'allestimento — i siti sono elencati in
	 * `ARTUnit::bIsBotControlled` — e da li' in avanti nessuno lo riscrive:
	 * una console variable digitata a meta' sessione cambierebbe la risposta del resolver ma non lo stato
	 * delle unita' gia' in campo, e la banda finirebbe per dichiarare una partita diversa da quella che si
	 * sta giocando — in **entrambi** i versi. Chi descrive la sessione (banda, log) legge di qui; chi
	 * risponde a «cosa mi stanno chiedendo» legge il resolver.
	 *
	 * ➕ Effetto secondario che vale la pena avere: la banda e' disegnata da `DrawHUD` a ogni fotogramma, e
	 * `ResolveAutobattle()` scandisce la riga di comando due volte. Deciderlo una volta toglie quel lavoro
	 * dal path per-frame.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Match")
	bool IsAutobattleInEffect() const { return bAutobattleInEffect; }

	/**
	 * I secondi di Planning in vigore, oppure **un valore negativo** se nessuno ha chiesto niente e il
	 * `TurnManager` deve tenersi il proprio.
	 *
	 * Stessa precedenza dell'altra configurazione, con un quarto gradino in fondo: console > riga di comando
	 * > proprieta' > **ripiego dell'autobattle**. Il ripiego vale solo a modalita' accesa, per la ragione
	 * scritta in `MatchPlanningSeconds`.
	 */
	float ResolveMatchPlanningSeconds() const;

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
	 * Secondo filtro, in **intersezione** con il primo: `reactions` + `gadget` mostra gli scenari che portano
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

	/**
	 * La sorgente mappa in vigore: `rt.Map.Source` se impostata e valida, altrimenti la proprieta'.
	 * Il piu' specifico vince, come per `ResolveScenarioToRun` — e un valore sconosciuto non ripiega in
	 * silenzio, perche' un playtest sulla mappa sbagliata e' un playtest buttato.
	 */
	ERTMapSource ResolveMapSource() const;

	/** Vero finché lo scenario sta girando: serve alla diagnostica e ai test. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Test")
	bool IsScenarioRunning() const;

	/**
	 * Riga da mostrare **a schermo** quando questa sessione sta eseguendo uno scenario invece della partita
	 * normale. Vuota = partita normale, e l'HUD non disegna nulla.
	 *
	 * Esiste perché il sintomo non punta alla causa. Con `ScenarioToRun` valorizzato `BeginPlay` esegue lo
	 * scenario e ritorna: la partita normale **non viene allestita affatto**, quindi niente unità proprie,
	 * niente selezione, niente barra abilità. Chi guarda vede uno schermo quasi vuoto e non ha modo di
	 * sapere perché — la spiegazione c'è, ma è in una riga di Output Log che non si ha motivo di andare a
	 * cercare. È successo il 2026-08-08, uscendo da `PIE-SCEN-KEEP` che lascia la property impostata.
	 *
	 * Non si basa su `IsScenarioRunning()`: quello torna falso appena lo scenario finisce, cioè **proprio
	 * quando** chi guarda resta davanti a un campo fermo a chiedersi dov'è la partita. La condizione giusta
	 * è «questa sessione è una run di scenario», che dura quanto la sessione.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Test")
	FString GetScenarioBannerText() const;

	/**
	 * Cosa e' successo all'allestimento: le condizioni rilevate e la fase raggiunta (CP 46.2, `#937`).
	 *
	 * ⚠️ **E' un rapporto, non una decisione.** Questo GameMode continua a decidere esattamente come prima
	 * — ripiega dove ripiegava, rifiuta dove rifiutava — e in piu' **dichiara** cosa ha fatto. Le righe
	 * aggiunte per CP 46.2 non cambiano un solo esito: se ne cambiassero uno, il perimetro concordato per
	 * la riallocazione di questo file sarebbe stato superato.
	 *
	 * Esiste perche' quelle condizioni finivano **solo** nel log: `MapSource=GeneratedTestArena` e il
	 * formato di ripiego sono le due riserve che tengono `G13` 🟡, e nessuna delle due si vede a schermo.
	 * E' lo stesso problema che `GetScenarioBannerText` risolve per le run di scenario — *«il sintomo non
	 * punta alla causa»* — generalizzato all'avvio.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Match")
	const FRTStartupReport& GetStartupReport() const { return StartupReport; }

	/**
	 * Assegna la squadra ai giocatori presenti, derivando i posti dal formato.
	 *
	 * ⚠️ **Idempotente A INSIEME DI GIOCATORI INVARIATO, e chiamata da DUE lati** — `OnPostLogin` e
	 * `SetupHexMatch` — perche' il motore non garantisce il loro ordine e le regole esistono solo dopo
	 * l'allestimento. Senza regole non fa nulla.
	 *
	 * ⛔ **La clausola ha una condizione, e non e' decorativa.** `Arrival` e' un indice posizionale
	 * sull'iteratore dei controller, e il ciclo fa `continue` SENZA contare sui controller privi di
	 * `ARTPlayerState`: se l'insieme dei controller "seduti" cambia fra due chiamate, tutti quelli
	 * successivi si reindicizzano e un giocatore gia' assegnato PUO' spostarsi. Non raggiungibile offline —
	 * un controller solo, l'insieme non cambia mai fra `OnPostLogin` e `SetupHexMatch` — ma il giorno in cui
	 * ce ne fossero due la garanzia varrebbe solo finche' nessuno entra o esce fra le due chiamate.
	 */
	void AssignSeats();

protected:
	virtual void OnPostLogin(AController* NewPlayer) override;

private:
	/** Le regole dell'ultimo allestimento riuscito. Vuote finche' non c'e' stato. */
	FRTMatchRules AssignedRules;
	bool bHasRules = false;

protected:
	virtual void BeginPlay() override;

	/** Fa avanzare la sessione dello scenario: un passo per frame, cosi' il playback si VEDE. */
	virtual void Tick(float DeltaSeconds) override;

public:
	/**
	 * Aggancia il velo al `TurnManager` e lo stende subito. Chiamata da `BeginPlay`.
	 *
	 * 🔑 **Da `E-SOLID` fetta 4 questo e' CABLAGGIO e basta.** Chi stende il velo, e per conto di quale
	 * squadra, e' `URTKnowledgeVeilPresenter`, che appartiene al client: il viewer e' del giocatore, non
	 * della partita, e in multiplayer il GameMode e' uno solo e sta sul server.
	 *
	 * ⛔ **Ma l'aggancio resta qui, ed e' una scelta di ORDINE**: e' il GameMode a spawnare il `TurnManager`,
	 * quindi e' l'unico punto in cui esiste per certo. `ARTPlayerController::BeginPlay` non ha nessuna
	 * garanzia di correre dopo, e agganciare di la' avrebbe un difetto che si vede solo a volte.
	 *
	 * 🔴 **E' pubblica per una ragione di misurabilita', non di comodo.** I test allestiscono una partita
	 * chiamando `SetupHexMatch` **direttamente** — `FollowsRefreshPoints` e gli altri — e non fanno correre
	 * `BeginPlay`: un aggancio scritto solo li' dentro non sarebbe attraversato da nessun test, e il velo
	 * risulterebbe coperto mentre il suo **cablaggio** non lo e'. E' la stessa distanza fra «il meccanismo
	 * esiste» e «qualcuno lo chiama» che ha lasciato `#1467` senza consumatore per giorni.
	 *
	 * ⚠️ **Chiamarla due volte e' sicuro** — `AddUniqueDynamic` non duplica l'iscrizione — ed e' cio' che la
	 * rende usabile da un test che non sa se `BeginPlay` sia gia' corso.
	 */
	void HookKnowledgeVeil();

	/**
	 * Il presenter del velo di questa sessione: quello del `ARTPlayerController` se un client c'e', altrimenti
	 * uno senza proprietario creato qui.
	 *
	 * ⚠️ **Il GameMode risolve DOVE vive il presenter, non cosa fa.** E' l'unico che vede insieme il
	 * controller e il turn manager, quindi e' l'unico che puo' rispondere alla domanda; il viewer, la
	 * conoscenza e il conteggio delle applicazioni stanno tutti dall'altra parte.
	 *
	 * ⛔ **Il ramo senza proprietario non e' un ripiego di comodo**: harness e test di simulazione girano
	 * senza client, e la board sta nel mondo anche quando nessuno la guarda. Li' il viewer e' `0`, con la
	 * stessa regola di `ARTCameraPawn::FrameOwnTeam`.
	 */
	URTKnowledgeVeilPresenter* GetKnowledgeVeilPresenter();

private:
	/** Vedi `GetKnowledgeVeilPresenter()`: esiste solo nelle sessioni senza client. */
	UPROPERTY(Transient)
	TObjectPtr<URTKnowledgeVeilPresenter> OwnerlessVeilPresenter;

	/**
	 * Il ciclo di vita dello scenario, quando questa sessione ne esegue uno.
	 *
	 * ⛔ **Il GameMode decide SE si gioca uno scenario; il coordinatore sa COME si esegue.** Qui restano la
	 * precedenza fra le tre sorgenti (`ResolveScenarioToRun`) e la scelta fra scenario e partita; caricamento,
	 * sessione, avanzamento e referto stanno dall'altra parte, con i quattro collaboratori dell'harness che
	 * questo file non deve piu' nominare.
	 */
	FRTScenarioCoordinator ScenarioCoordinator;

	/**
	 * Centra la camera sulla mappa dello scenario, al tick successivo.
	 *
	 * Resta qui e non nel coordinatore per una ragione di **ciclo di vita**, non di dominio: il timer si
	 * aggancia a un Actor vivo, e il coordinatore e' una classe C++ pura che non ne ha uno da offrire.
	 */
	void RecenterCameraOnScenario();

	/** Le condizioni rilevate durante l'allestimento (CP 46.2). Vedi `GetStartupReport()`. */
	FRTStartupReport StartupReport;

	/** Vedi `IsAutobattleInEffect()`: deciso in `SetupHexMatch`, prima che le unita' entrino in campo. */
	bool bAutobattleInEffect = false;

	/** Come sopra: la sorgente che ha deciso, latchata insieme alla decisione perche' la banda la nomina. */
	FString AutobattleSourceLabel;

	/**
	 * Porta il verdetto di fine partita alla schermata di Result (CP 46.5, `#940` · `#939`).
	 *
	 * ⚠️ **Sta qui e non nel `TurnManager`** perché questo è l'unico punto che conosce sia il turno sia il
	 * frontend: la simulazione annuncia il verdetto che ha già dato, e non deve sapere che esista una UI.
	 *
	 * ⚠️ **E non ricalcola niente**: passa a `ShowResult` il `FRTMatchResult` ricevuto, che è la stessa
	 * regola per cui il view model legge invece di ridare il verdetto.
	 */
	UFUNCTION()
	void HandleMatchEnded(const FRTMatchResult& Result, const FRTMatchState& State);

	// ---- CP 46.6 · il lato di partita del confine col frontend (`#941`) -------------------------------
public:

	/**
	 * Si iscrive alle richieste di livello del navigatore. **Pubblica e separata da `BeginPlay`** per essere
	 * verificabile headless, come `SetupHexMatch`.
	 *
	 * ⚠️ **Il `public:` qui sopra non e' decorativo, e la prima stesura lo aveva dimenticato**: questo
	 * blocco cadeva dentro l'unico `private:` del file, quindi la frase «pubblica … per essere verificabile»
	 * descriveva una funzione che un test non poteva chiamare. Trovato in code review sulla PR #1304 — una
	 * giustificazione che descrive il contrario di cio' che il codice fa e' peggio di nessuna.
	 *
	 * 🔴 **Chiude un confine che per un intero checkpoint aveva un solo lato.** `ARTFrontendGameMode`
	 * raccoglie `OnMatchRequested` **sulla mappa del menu**; dentro una partita quel GameMode non esiste,
	 * quindi fino a `#941`:
	 *
	 * - `RETURN TO MAIN MENU` non aveva alcun consumatore — e infatti non esisteva: `ReturnMain()` muoveva
	 *   lo stack e lasciava la partita viva **sotto** il menu, lo stato che CP 46.2 dichiara vietato;
	 * - `PLAY AGAIN` dal Result — che si apre **dentro** il livello di partita — annunciava a zero
	 *   ascoltatori, e il `PLAY` successivo veniva rifiutato da `MatchRequestNotConsumed`.
	 *
	 * ⚠️ **La chiama `BeginPlay`, e non basta un test che la chiami a mano.** E' la lezione di `#939`: otto
	 * test verdi non videro che il consumatore non era collegato a niente, perche' lo collegavano tutti da
	 * se'. Un test deve arrivarci passando dal ciclo di vita.
	 */
	void ListenForLevelRequests(URTFrontendNavigator* Navigator);

	/** `PLAY AGAIN` dal Result: si consuma la richiesta e si riapre il livello di partita. */
	UFUNCTION()
	void HandleMatchRequested(const FString& LevelName);

	/** `RETURN TO MAIN MENU`: si consuma la richiesta e si apre il livello del menu — la partita muore col mondo. */
	UFUNCTION()
	void HandleReturnToFrontendRequested(const FString& LevelName);

	/**
	 * Apre un livello. **Virtual perche' e' il seam dei test**, esattamente come
	 * `ARTFrontendGameMode::OpenMatchLevel`: `UGameplayStatics::OpenLevel` in un mondo di prova non porta da
	 * nessuna parte, e senza questo punto il consumatore si potrebbe provare solo in PIE.
	 */
	virtual void OpenLevelByName(const FString& LevelName);

};
