#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Templates/SubclassOf.h"
#include "Core/RTTypes.h"
// CP 46.2: il rapporto d'avvio. E' un tipo di **dato** — enum e struct puri, nessuna dipendenza dal
// frontend: il GameMode dichiara cosa e' successo, chi mostra la cosa sta dall'altra parte.
#include "Frontend/RTStartupReport.h"
// Il ciclo di vita a runtime degli scenari. E' un membro per VALORE, quindi serve la definizione: la
// forward declaration basterebbe solo per un puntatore, e un'indirezione qui non comprerebbe niente.
#include "ScenarioHarness/RTScenarioCoordinator.h"
#include "RTGameMode.generated.h"

class ARTUnit;
class ARTHexMapActor;
class ARTTurnManager;
class URTFrontendNavigator;
class URTHeroData;
class URTHexMapAsset;
class URTMatchFormatData;
struct FRTMatchRules;

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

protected:
	virtual void BeginPlay() override;

	/** Fa avanzare la sessione dello scenario: un passo per frame, cosi' il playback si VEDE. */
	virtual void Tick(float DeltaSeconds) override;

public:
	/**
	 * 🔑 **DI CHI E' LA VISTA che il velo disegna. E' la decisione di `E13.8`, e sta scritta qui perche'
	 * [D-227] la dichiarava non presa** — *«decidere chi sia il viewer del velo: `ApplyKnowledgeVeil` riceve
	 * una conoscenza gia' scelta dal chiamante, e la firma non deve decidere il team»*.
	 *
	 * **La risposta e' `ARTPlayerController::PlayerTeamId`**, e non e' una scelta nuova: e' gia' la fonte del
	 * progetto per la stessa domanda. `ARTCameraPawn::FrameOwnTeam` la legge per inquadrare la propria
	 * squadra (`Camera/RTCameraPawn.cpp`), e `CanPlayerControlUnit` la legge per decidere cosa si puo'
	 * selezionare (`Player/RTPlayerController.cpp`, due siti). Il velo si aggiunge a quei lettori invece di
	 * aprire una terza autorita'.
	 *
	 * ⚠️ **Si RILEGGE a ogni applicazione, non si copia in un campo.** Copiarla in `BeginPlay` farebbe del
	 * GameMode una seconda sede del valore, che e' esattamente cio' che questa funzione esiste per evitare.
	 *
	 * 🔴 **Il precedente da non ripetere, misurato il 2026-08-29**: `ARTHUD::DrawHUD` scrive
	 * `const int32 PlayerTeamId = 0;` e con quello alimenta **sia** `ComputePlannedHitMarks` **sia**
	 * `KnowledgeForTeamPublic` — cioe' la stessa conoscenza di squadra che il velo usa, letta con un
	 * letterale invece che dal controller. Vale `0` come questa funzione, quindi oggi non si vede: e' una
	 * divergenza latente, non un difetto attivo. **Non e' corretto qui** per non allargare lo scope di
	 * `E13.8` all'overlay delle unita' e agli hit marks, e resta un lavoro dichiarato.
	 *
	 * Ripiega su `0` quando un controller non c'e' — headless, harness — con la stessa regola di
	 * `FrameOwnTeam`, che il suo test pinna.
	 */
	int32 ViewerTeamId() const;

	/**
	 * Stende il velo sulla board secondo la conoscenza di `ViewerTeamId()`.
	 *
	 * ⛔ **Non decide niente**: legge il team da `ViewerTeamId` e la conoscenza da
	 * `ARTTurnManager::KnowledgeForTeamPublic`. Se un giorno il velo dovesse mostrare la vista di un altro,
	 * la riga da cambiare e' una e non e' questa.
	 */
	void ApplyKnowledgeVeilForViewer();

	/**
	 * Aggancia il velo al `TurnManager` e lo stende subito. Chiamata da `BeginPlay`.
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
	 * 🔑 **Quante volte il velo e' stato steso. Esiste per rendere osservabile l'ANELLO che non lo era.**
	 *
	 * I tre conteggi di `GetVeilCounts` dicono **come** e' la board, non **quante volte** e' stata
	 * ridipinta — e i due difetti che contano qui producono lo stesso quadro:
	 *
	 *  - il velo steso una volta sola all'aggancio, con l'handler mai invocato;
	 *  - il velo ridipinto a ogni refresh, ma con una conoscenza vuota.
	 *
	 * Entrambi lasciano la board tutta nascosta. Senza un contatore, distinguerli richiede un log — e un log
	 * assente ha due letture, «non eseguito» e «non catturato», che portano a fix opposti. Questo numero e'
	 * un fatto che un test legge, non un messaggio che un test spera di trovare.
	 *
	 * ⚠️ **NON e' un contatore di correttezza**: dice che `ApplyKnowledgeVeilForViewer` e' stata chiamata,
	 * non che abbia disegnato bene. Quella la misura `GetVeilCounts`, che legge le istanze reali. Servono
	 * entrambi, e nessuno dei due sostituisce l'altro.
	 */
	int32 GetKnowledgeVeilApplications() const { return KnowledgeVeilApplications; }

protected:
	/**
	 * Il subscriber di `OnTeamKnowledgeRefreshed`, cioe' il consumatore che a `#1467` mancava.
	 *
	 * ⚠️ **`UFUNCTION` perche' il delegate e' dinamico**, non per esposizione: nessuna categoria, nessun
	 * `BlueprintCallable`. Un Blueprint che potesse invocarlo stenderebbe il velo fuori dai punti di
	 * refresh, che e' precisamente cio' che `Veil.FollowsRefreshPoints` impedisce.
	 */
	UFUNCTION()
	void HandleTeamKnowledgeRefreshed(int32 TurnNumber);

private:
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

	/** Quante volte `ApplyKnowledgeVeilForViewer` ha steso il velo. Vedi `GetKnowledgeVeilApplications()`. */
	int32 KnowledgeVeilApplications = 0;

	/** Vedi `IsAutobattleInEffect()`: deciso in `SetupHexMatch`, prima che le unita' entrino in campo. */
	bool bAutobattleInEffect = false;

	/** Come sopra: la sorgente che ha deciso, latchata insieme alla decisione perche' la banda la nomina. */
	FString AutobattleSourceLabel;

	/** Applica `MapSource` all'actor mappa: sostituisce l'asset quando la scelta lo richiede, e lo dichiara nel log. */
	void ApplyMapSource(ARTHexMapActor* HexMap);

	/**
	 * Risolve il formato di partita e lo consegna al `TurnManager`. Ritorna **false** se il formato e'
	 * presente ma invalido, oppure se non e' compatibile con la classe della mappa (CP 19.1) — in quel caso
	 * la partita non va allestita.
	 *
	 * E' qui che vive la politica di ripiego, come per l'arena generata: la libreria pura rifiuta e basta,
	 * l'Actor decide che farne e lo dichiara in Warning. Un ripiego silenzioso non produce una partita rotta,
	 * produce numeri di playtest attribuiti a un formato che non era in vigore.
	 *
	 * `OutRules` esce valorizzato solo quando il ritorno e' `true`: l'allestimento legge da li' la
	 * composizione (`UnitsPerTeam`, CP 19.2) invece di dichiararla per conto proprio.
	 */
	bool ApplyMatchFormat(ARTTurnManager* TurnManager, const URTHexMapAsset* Map, FRTMatchRules& OutRules);

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

private:

	/**
	 * Spawna l'eroe con l'`HeroId` dato. `Hero == nullptr` non spawna nulla (fail-closed): un'unita' con
	 * statistiche di default al posto di un eroe sarebbe piu' difficile da diagnosticare di un'unita' assente.
	 */
	ARTUnit* SpawnHero(int32 TeamId, const URTHeroData* Hero, const FRTCellId& InCell, const FVector& Origin,
		float HexSize, float LayerHeight);
};
