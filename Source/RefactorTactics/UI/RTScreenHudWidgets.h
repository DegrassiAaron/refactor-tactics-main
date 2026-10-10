#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/RTHudViewModel.h"
#include "UI/RTIconCatalogData.h" // FRTIconResolution e' un valore di ritorno: serve la definizione, non basta la forward
#include "Turn/RTReactionWindowView.h" // idem per FRTReactionWindowView, reso per valore da `URTFastDecisionWidget`
#include "RTScreenHudWidgets.generated.h"

class ARTPlayerController;
class ARTTurnManager;
class ARTUnit;
class URTIconCatalogData;
class URTReactionWindowViewModel;
class URTFastDecisionOptionWidget;
class UImage;

/**
 * Le classi BASE dei widget dello Screen HUD (§4.1 di `progettazione-hud.md`, CP 11.7 / #613).
 *
 * ⚠️ **Qui non c'e' layout.** Il `.uasset` `WBP_RT_*` fa aspetto e disposizione; questo file dichiara **cosa
 * il widget puo' leggere**, e soprattutto cosa non puo'. La ricetta per costruire i Blueprint sta in
 * `docs/technical/runbooks/guida-screen-hud-umg.md`.
 *
 * Tre vincoli del DoD diventano proprieta' della FIRMA invece che disciplina da ricordare:
 *
 *  1. **I widget non ricalcolano.** L'unica cosa che restituiscono sono le viste di `URTHudViewModel`, gia'
 *     sanitizzate. Non c'e' un accessor che dia l'`ARTTurnManager` o un `ARTUnit` a un Blueprint: se non
 *     c'e' il puntatore, non c'e' il modo di ricalcolare.
 *  2. **Nessun widget referenzia una texture.** In questo file non compare `UTexture2D`. Le icone viaggiano
 *     come `FName` (`UI.Icon.Action.Move`) e si risolvono col catalogo (D-031, `#220`).
 *  3. **Il debug e' spento di default** nella vista giocatore, ed e' un `UPROPERTY` con default `false`, non
 *     una casella da ricordare in ogni Blueprint.
 *
 * ⚠️ **Il limite di questi vincoli va detto**: valgono per la superficie C++. Un Blueprint derivato puo'
 * sempre aggiungersi una variabile propria, e la firma dichiarata qui non la vede.
 * 🔴 **La ragione che questa riserva dava e' scaduta**: diceva *«nessun gate lo impedisce, perche' i
 * `.uasset` non sono versionati in questo repository»*, e i `WBP_RT_*` di `Content/RT/UI/Match/` **sono
 * versionati** — `.gitignore:78` li re-include con una negazione su `Content/RT/UI` (doppio asterisco, poi
 * l'estensione `.uasset`), e `git check-ignore` non ne nomina nessuno. Lo stesso errore stava in
 * `RTFrontendWidgets.h`, dov'e' gia' stato corretto.
 *
 * ⛔ **Il glob non si scrive per esteso qui, e non e' pedanteria.** Scritto per intero contiene la
 * sequenza di due caratteri che CHIUDE un commento a blocco, e da li' in poi il testo smette di essere un
 * commento. Era cosi' dal 2026-09-18 (`de995ec9`): questo header non compilava appena UnrealHeaderTool lo
 * rigenerava, e l'errore arrivava come *«Unterminated character constant»* alcune righe piu' sotto — sul
 * primo apostrofo del testo tornato codice, che non c'entrava niente. ⚠️ Il difetto era **latente**:
 * finche' nessuno toccava il file, UHT non lo rigenerava e la build passava.
 *
 * Le due meta' della regola D-031 hanno ciascuna il proprio gate:
 *
 *  - la superficie **C++** — `RefactorTactics.ScreenHud.WidgetApiExposesNoTexture`;
 *  - le variabili dichiarate **dentro** i `.uasset` —
 *    `RefactorTactics.ScreenHud.BlueprintPropertiesExposeNoTexture`
 *    (`Tests/RTMatchWidgetAssetTests.cpp`), che itera per reflection le proprieta' della
 *    `UWidgetBlueprintGeneratedClass` dei widget di Match e rifiuta ogni `UTexture2D`, anche dentro array,
 *    set, map e `TSoftObjectPtr`.
 *
 * ⛔ **Cio' che resta scoperto davvero**, perche' il limite continui a essere dichiarato — ma quello vero:
 *
 *  1. un **riferimento diretto a un Actor** aggiunto dentro un `WBP_RT_*`: nessun test noto lo rifiuta. Il
 *     vincolo 1 qui sopra chiude la porta nella firma C++, non nel `.uasset`;
 *  2. una texture **dentro una struct** — `FSlateBrush::ResourceObject` e' un `UObject*`, non un
 *     `UTexture2D*`, quindi un `Brush` impostato nel designer passa anche il gate Blueprint, che lo
 *     dichiara nel proprio docstring;
 *  3. che l'icona **si veda**: resta `PIE-ICON-01`, e nessun test la sostituisce.
 */

/**
 * Base comune: il CONTESTO di partita, e nient'altro.
 *
 * Il contesto si acquisisce da solo in gioco (`NativeConstruct` -> owning player), oppure si inietta nei
 * test. Non e' esposto ai Blueprint: un widget che potesse leggere `ARTTurnManager` avrebbe il modo di
 * ricalcolare, ed e' esattamente la porta che §4.1 chiude.
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTScreenHudWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Debug spento nella vista giocatore. `EditDefaultsOnly` e non `EditAnywhere`: si accende cambiando il
	 * default della classe, non per istanza — cosi' non resta acceso in una sola schermata dimenticata.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|HUD")
	bool bShowDebug = false;

	/** Vero quando il contesto di partita e' disponibile: un widget costruito prima del manager mostra «—». */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	bool HasMatchContext() const;

	/**
	 * Inietta il contesto senza un `PlayerController`: e' il modo in cui i test guidano un widget.
	 *
	 * ⚠️ **Prende un `TWeakObjectPtr` e non un puntatore nudo**, per la stessa ragione per cui
	 * `SetSelectedUnitForTest` e' definita nel `.cpp`: `TWeakObjectPtr::operator=` vuole il tipo completo, e
	 * `ARTTurnManager` qui e' solo forward-declared. Chiamandola con un `ARTTurnManager*` la conversione
	 * avviene **nel chiamante**, che essendo un test l'header ce l'ha gia'. Vedi `Turn/RTTurnManagerAccess.h`
	 * per il difetto che questa forma chiude.
	 */
	void SetMatchContextForTest(TWeakObjectPtr<ARTTurnManager> InTurnManager, int32 InPlayerTeamId);

	/**
	 * Inietta la SELEZIONE senza un `PlayerController`, ed e' l'altra meta' di `SetMatchContextForTest`.
	 *
	 * 🔴 **Senza questo, tre widget su sette non erano verificabili affatto.** `GetSelectedUnit()` passa da
	 * `GetOwningPlayer()`, e `UUserWidget::SetOwningPlayer` memorizza il **`ULocalPlayer`**, non il
	 * controller: in una run headless non esiste un local player, quindi `GetOwningPlayer()` resta nullo
	 * anche dopo aver spawnato un `ARTPlayerController` e avergli selezionato un'unita'. Pannello unita',
	 * dock e slot leggono tutti da li', e i loro test potevano provare solo il ramo «nessuna selezione».
	 *
	 * ⚠️ **E il prezzo si e' visto**: lo Step 7.4 di `#613` chiedeva che il dock accendesse lo slot armato,
	 * il Blueprint passava `false` fisso, e `ActionDockShowsTheNeutralState` era verde — perche' senza
	 * selezione anche un dock rotto risponde `INDEX_NONE`.
	 *
	 * In gioco resta nulla e la verita' e' il `PlayerController`: l'iniezione **non** e' un secondo canale
	 * di selezione, e non e' esposta ai Blueprint.
	 *
	 * ⚠️ Definita nel `.cpp` e non qui: `ARTUnit` e' solo forward-declared in questo header, e
	 * `TWeakObjectPtr::operator=` vuole il tipo completo.
	 */
	void SetSelectedUnitForTest(ARTUnit* InUnit);

	/** Come sopra, per il soggetto **ispezionato** (`#705`): quello che si guarda, non quello che si comanda. */
	void SetInspectedUnitForTest(ARTUnit* InUnit);

	/**
	 * Inietta il VIEW MODEL della finestra di reazione senza un `PlayerController` (CP 14.6, `#166`).
	 *
	 * 🔴 **Non e' una comodita': senza, il widget della finestra sarebbe verificabile solo nel ramo «nessuna
	 * finestra».** `AcquireMatchContext` ripara il proprietario mancante con
	 * `SetOwningPlayer(World->GetFirstPlayerController())`, ma `UUserWidget::SetOwningPlayer` memorizza il
	 * **`ULocalPlayer`** e in headless non ne esiste uno — la stessa ragione, misurata, per cui
	 * `SetSelectedUnitForTest` esiste, e con lo stesso prezzo gia' pagato una volta: *«il Blueprint passava
	 * `false` fisso, e `ActionDockShowsTheNeutralState` era verde»*.
	 *
	 * In gioco resta nulla e la verita' e' il `PlayerController`. **Non** e' esposta ai Blueprint: un secondo
	 * canale per raggiungere la finestra sarebbe un secondo posto da cui rispondere.
	 *
	 * ⚠️ Definita nel `.cpp` come le due sorelle: `URTReactionWindowViewModel` e' solo forward-declared qui e
	 * `TWeakObjectPtr::operator=` vuole il tipo completo.
	 */
	void SetReactionWindowForTest(URTReactionWindowViewModel* InViewModel);

	/**
	 * Inietta il controller a cui le porte in USCITA di un widget inoltrano — il badge di Sneak della dock, i
	 * pulsanti di `URTPlanCommitWidget` ([D-457], [D-458]) — senza passare da un `ULocalPlayer`.
	 * E' la stessa forma di `URTActionSlotWidget::SetArmingControllerForTest`, e per la stessa ragione: in
	 * headless `GetOwningPlayer()` e' nullo, e un test senza questa porta misurerebbe il ramo «nessun
	 * proprietario» credendo di misurare il click. **Nullo in gioco.**
	 */
	void SetCommandControllerForTest(ARTPlayerController* InController);

	/** Il gruppo di controllo di chi guarda, senza un `PlayerController` (#3618). In gioco viene dal controller. */
	void SetControlGroupForTest(int32 InControlGroup) { PlayerControlGroup = InControlGroup; }

protected:
	virtual void NativeConstruct() override;

	/**
	 * Riprova ad acquisire il contesto finche' non c'e'.
	 *
	 * 🔴 **Il widget puo' nascere PRIMA del `TurnManager`, e nel percorso normale succede.**
	 * `ARTGameMode::BeginPlay` chiama `EnterMatch()` — che presenta il HUD — alla riga 295, e spawna il
	 * `ARTTurnManager` alla riga 337. `NativeConstruct` cerca un actor che non esiste ancora, trova
	 * `nullptr`, e senza questo tick resterebbe senza contesto **per sempre**: `HasMatchContext()` falso,
	 * tutte le viste neutre, e a schermo «—» al posto di «Round 1/12».
	 *
	 * ⚠️ **Il costo e' limitato per costruzione**: la ricerca gira solo finche' `TurnManager` e' invalido,
	 * cioe' i pochi frame iniziali. Appena il contesto c'e', questo tick non fa piu' nulla.
	 */
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Risolve il contesto dall'owning player. Chiamata da `NativeConstruct`; ripetibile. */
	void AcquireMatchContext();

	const ARTTurnManager* GetTurnManager() const { return TurnManager.Get(); }
	int32 GetPlayerTeamId() const { return PlayerTeamId; }

	/**
	 * Il gruppo di controllo di chi guarda, dallo STESSO controller da cui viene la squadra (#3618). Con la squadra e
	 * `URTCombatLibrary::CanPlayerControlUnitInGroup` dice quali unita' sono comandate: la squadra da sola direbbe
	 * comandata anche l'alleata del bot.
	 */
	int32 GetPlayerControlGroup() const { return PlayerControlGroup; }

	/**
	 * Le unita' in campo, in ordine STABILE per `HeroId`.
	 *
	 * ⚠️ **L'ordine e' parte del contratto, non una comodita'.** `GetAllActorsOfClass` non dichiara un
	 * ordine, e una vista che cambia disposizione fra due frame e' illeggibile. Ordinare qui — invece che
	 * in ogni chiamante — e' anche cio' che rende il roster e la scoperta delle squadre coerenti fra loro.
	 *
	 * ⛔ **Non e' esposta ai Blueprint**: restituisce `ARTUnit*`, cioe' esattamente la porta da cui un
	 * widget potrebbe ricalcolare. La superficie pubblica resta fatta di VISTE.
	 */
	TArray<ARTUnit*> GatherUnitsInWorld() const;

	/**
	 * Chi e' autorizzato a guardare questa partita: la propria squadra, o tutte in sessione non presidiata.
	 *
	 * 🔴 **Delega a `URTHudViewModel::ResolveObserverTeamIds` e non decide qui**, ed e' la ragione
	 * architetturale di `#2744`: il dato che decide e' `ARTTurnManager::IsUnattendedSession()`, che e'
	 * **inline** nell'header dell'orchestratore. Leggerla da questo file ritirerebbe dentro
	 * `RTScreenHudWidgets.cpp` l'header che `#2257` ha tolto — lo dichiara il commento accanto al suo
	 * `#include "Turn/RTTurnManagerAccess.h"`. Qui si passa il puntatore, come gia' fa `GetFeed()`.
	 */
	TArray<int32> ResolveObserverTeamIds() const;

	/** L'unita' selezionata dal giocatore, o `nullptr`. Protetta: i Blueprint vedono solo le VISTE. */
	const ARTUnit* GetSelectedUnit() const;

	/** Il controller a cui le porte in uscita inoltrano: quello iniettato dai test, altrimenti il proprietario. */
	ARTPlayerController* ResolveCommandController() const;

	/**
	 * L'unita' che si sta **guardando**, che puo' non essere quella che si comanda (`#705`).
	 *
	 * ⛔ **Non e' un secondo `GetSelectedUnit()`, e chi la usa deve saperlo**: un soggetto ispezionato puo'
	 * essere avversario, quindi da qui **non** si costruiscono ne' gli slot pianificati ne' il dock delle
	 * azioni. Protetta come la sorella, e per la stessa ragione: i Blueprint vedono le viste, non le unita'.
	 */
	const ARTUnit* GetInspectedUnit() const;

	/**
	 * Il view model della finestra di reazione di questo client, o `nullptr` (CP 14.6, `#166`).
	 *
	 * 🔴 **`protected`, e la differenza NON e' stilistica.** `URTReactionWindowViewModel::SubmitResponse` e'
	 * `BlueprintCallable`: un accessore **pubblico** su questa base metterebbe un nodo che **spara un
	 * Overwatch** nel grafo di tutte e sei le classi che ne derivano — roster, dock, event log compresi. Ed e'
	 * l'esatto contrario della disciplina che questo file dichiara in testa: *«se non c'e' il puntatore, non
	 * c'e' il modo di ricalcolare»*. Qui c'e' un solo sito di risoluzione, e la superficie pubblica vive su
	 * `URTFastDecisionWidget`, la cui firma porta solo cio' che quel widget puo' fare.
	 *
	 * ⚠️ **Si risolve dall'OWNING PLAYER, non dal primo controller del mondo**, per la stessa ragione di
	 * `PlayerTeamId`: la finestra e' una domanda posta a **un** giocatore, e in split-screen il primo
	 * controller non e' necessariamente il proprio. Il prezzo e' che in headless resta nullo — ed e' il
	 * motivo per cui `SetReactionWindowForTest` esiste.
	 */
	URTReactionWindowViewModel* GetReactionWindow() const;

public:
	/**
	 * Il catalogo iconografico, o `nullptr`.
	 *
	 * 🔴 **Esiste perche' il catalogo vive sulla RADICE e non su questa base**, e nessuno dei widget che
	 * devono mostrare un'icona lo raggiungeva: `IconCatalog` e' dichiarato su `URTTacticalHUDWidget`, che
	 * e' una classe **sorella** nella gerarchia — discende da questa base, non la precede. Il dock quindi
	 * non lo ereditava, e nel grafo di un Blueprint non c'era nodo che lo producesse.
	 *
	 * ⚠️ **Dichiararlo qui sulla base sarebbe stata l'altra strada, e costa di piu':** e' un
	 * `EditDefaultsOnly`, quindi ogni `WBP_RT_*` avrebbe la propria copia da assegnare a mano — N
	 * occasioni di dimenticarne una, contro l'unica assegnazione sulla radice che c'e' oggi.
	 *
	 * ⚠️ **Risale con `GetTypedOuter`, ed e' un idioma NUOVO per questo file.** Il resto della base non
	 * cammina l'albero UMG: prende gli attori dal mondo (`GetActorOfClass`) o passa da
	 * `GetOwningPlayer()`. Qui non e' possibile — il catalogo non e' un attore e non appartiene al
	 * controller, e' un dato di configurazione di un widget. La risalita e' quindi l'unica via, e il suo
	 * limite si dichiara: **se un widget viene innestato fuori dal `WBP_RT_TacticalHUD` questa funzione
	 * restituisce `nullptr` in silenzio**.
	 *
	 * ✅ Il silenzio non e' pero' una perdita: `URTIconLibrary::ResolveIcon` con catalogo nullo da' il
	 * missing-icon **con una warning che nomina la chiave e il consumer**. A schermo si vede che manca,
	 * che e' il comportamento voluto dallo Step 6.4 del piano di `#613`.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	const URTIconCatalogData* GetIconCatalog() const;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<ARTTurnManager> TurnManager;

	UPROPERTY(Transient)
	int32 PlayerTeamId = 0;

	/** Vedi `GetPlayerControlGroup`. `0` e' il gruppo di default, lo stesso di `ARTPlayerState` e `ARTUnit`. */
	UPROPERTY(Transient)
	int32 PlayerControlGroup = 0;

	/**
	 * Vedi `SetSelectedUnitForTest`. Nulla in gioco: la selezione vera resta del `PlayerController`.
	 *
	 * ⚠️ Non `TWeakObjectPtr<const ARTUnit>`: il template non accetta un tipo `const` e l'assegnazione non
	 * compila. La const-ness sta dove serve — `GetSelectedUnit()` restituisce comunque un puntatore const.
	 */
	UPROPERTY(Transient)
	TWeakObjectPtr<ARTUnit> SelectedUnitForTest;

	/** Vedi `SetCommandControllerForTest`. Nullo in gioco: il proprietario vero resta `GetOwningPlayer()`. */
	TWeakObjectPtr<ARTPlayerController> CommandControllerForTest;

	/**
	 * Gemella della precedente per il soggetto **ispezionato** (`#705`), e per la stessa ragione: senza un
	 * `ULocalPlayer` — che una run headless non ha — `GetOwningPlayer()` resta nullo e il pannello non
	 * sarebbe verificabile. Nulla in gioco: l'ispezione vera resta del `PlayerController`.
	 */
	UPROPERTY(Transient)
	TWeakObjectPtr<ARTUnit> InspectedUnitForTest;

	/**
	 * La finestra di reazione, risolta dal proprietario in `AcquireMatchContext` oppure iniettata da un test.
	 *
	 * ⚠️ **Weak e non `TObjectPtr`**: il view model vive col `PlayerController` (`#2723`), che questo widget
	 * non possiede. Un puntatore forte lo terrebbe in vita oltre il proprio controller, e
	 * `URTReactionWindowViewModel::Hook` fa dipendere da quella morte proprio cio' che spegne il ramo
	 * interattivo — `IsBound()` e' falso quando il view model non c'e' piu'.
	 */
	UPROPERTY(Transient)
	TWeakObjectPtr<URTReactionWindowViewModel> ReactionWindow;
};

/** `WBP_RT_TurnHeader` — round su `RoundLimit`, fase, timer. */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTTurnHeaderWidget : public URTScreenHudWidgetBase
{
	GENERATED_BODY()

public:
	/**
	 * L'intestazione, dal view model. Senza contesto restituisce la vista neutra — round `0`, nessun limite,
	 * timer negativo — che il widget mostra come «—» invece che come «Round 0/0».
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	FRTMatchHeaderView GetHeader() const;

	/**
	 * Il testo del contatore, gia' composto: `Round 3/12`, oppure `Round 3` quando il formato non dichiara un
	 * limite.
	 *
	 * Esiste come funzione e non come regola scritta nella guida perche' e' proprio il punto in cui un widget
	 * sbaglierebbe: `RoundLimit == 0` significa «nessun limite», e un binding ingenuo stamperebbe `Round 3/0`,
	 * che si legge come una partita gia' scaduta. La distinzione e' nel tipo, ma la decisione va fatta una
	 * volta sola — qui.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	FText GetRoundCounterText() const;
};

/**
 * `WBP_RT_EventLog` — il feed di chi gioca: cosa e' successo nel turno, filtrato per squadra.
 *
 * 🔴 **E' il consumatore che `#2697` ha misurato mancante.** Il canale autorizzato esisteva, era coperto da
 * test ed era **verde** — e nessuno lo leggeva: la spiegazione di un'azione dichiarata e non avvenuta
 * arrivava al `TurnLog` e all'Output Log, dove in partita nessuno guarda. Verdetto d'autore del 2026-09-09:
 * *«non si capisce perche' non parte, non ci sono riferimenti video, solo log»*.
 *
 * ⛔ **Non c'e' un accessor per il log completo, ed e' la proprieta' che conta.** `GetFeed()` passa da
 * `URTHudViewModel::BuildPlayerEventFeed`, che filtra per l'osservatore; il `TurnManager` non e' esposto ai
 * Blueprint dalla base, quindi un widget derivato non ha una seconda porta da cui leggere le righe non
 * filtrate. Una regressione a un canale completo sarebbe un **leak di conoscenza**, non un dettaglio di UI.
 *
 * ⚠️ **Qui non c'e' layout, come per gli altri.** Posizione, budget di righe e comportamento fra Planning e
 * Resolution stanno nel `.uasset` e in `progettazione-hud.md` §4.1, che ne e' l'owner (`#1936` §F).
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTPlayerEventLogWidget : public URTScreenHudWidgetBase
{
	GENERATED_BODY()

public:
	/**
	 * Le righe da mostrare, gia' autorizzate e gia' composte. Senza contesto: nessuna riga.
	 *
	 * ⚠️ Non ha un parametro «mostra tutto». La regola di privacy diventa una proprieta' della firma invece
	 * che disciplina da ricordare — la stessa forma di `GetRoster()`.
	 *
	 * ⚠️ **In sessione non presidiata l'insieme degli osservatori si allarga, la firma no** (`#2744`): chi
	 * decide e' `ResolveObserverTeamIds()`, da un dato della sessione. Un widget non ha comunque modo di
	 * chiedere le righe non filtrate.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	TArray<FRTPlayerEventLineView> GetFeed() const;

	/** Perche' il feed e' vuoto, per `rt.Debug.ScreenHud`: inoltra al view model, che ha il tipo completo.
	 *
	 * ⚠️ **Il manager si PASSA e non si dereferenzia qui**, come in `GetFeed`: leggere `GetTurnLog()` da
	 * questo file rivorrebbe l'header che `#2257` ha tolto.
	 */
	TArray<FString> DescribeFeedState() const;
};

/**
 * `WBP_RT_TeamRoster` — le unita' della PROPRIA squadra, morte comprese; e in autobattle anche le altre.
 *
 * 🔴 **Due liste, mai una fusa** (`#2744`). `FRTUnitCardView` non porta `TeamId`: porta `bIsAlly`, calcolato
 * contro la squadra chiesta. In una lista sola meta' delle carte direbbe «alleata» a uno spettatore che non
 * comanda nessuno — un occultamento semantico dentro un tipo, che e' lo stesso difetto di forma che `#2281`
 * descrive per il punteggio. Con due liste ciascuna e' interamente di una squadra, e a dirlo e' la lista.
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTTeamRosterWidget : public URTScreenHudWidgetBase
{
	GENERATED_BODY()

public:
	/**
	 * Il roster della propria squadra. Non c'e' un parametro «mostra anche gli avversari», ed e' voluto: la
	 * regola di §4.1 diventa una proprieta' della firma invece di una disciplina.
	 *
	 * ⚠️ **Questa firma non cambia in autobattle**, e non e' un dettaglio: risponde a *«chi comando io»*
	 * (`URTHudViewModel::BuildTeamRoster`), e ribaltarne il significato per una modalita' renderebbe la
	 * stessa funzione due cose diverse a seconda della sessione.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	TArray<FRTUnitCardView> GetRoster() const;

	/**
	 * Le altre squadre in campo — **vuoto in sessione presidiata**, ed e' li' che sta la regola (`#2744`).
	 *
	 * Non c'e' una guardia scritta a parte: l'insieme viene da `ResolveObserverTeamIds()`, che in sessione
	 * presidiata restituisce la sola squadra del giocatore. Il ciclo qui sotto salta quella, e non resta
	 * niente. ∴ un `if (presidiata)` sarebbe una seconda copia della stessa decisione.
	 *
	 * 🔑 **Due chiamate a `BuildTeamRoster` compongono senza doppioni PER COSTRUZIONE**, perche' quel filtro
	 * e' `TeamId == PlayerTeamId` e gli insiemi di due squadre sono disgiunti. ⛔ Non e' la forma giusta per
	 * il feed, dove il filtro autorizza per voce e gli insiemi si sovrappongono — vedi
	 * `URTPlayerEventProjector::Project`.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	TArray<FRTUnitCardView> GetOpposingRoster() const;

	/**
	 * Il chip `REAZ.` della card di `HeroId` ([D-478], #3618): vero se l'alleata e' COMANDATA da chi guarda e il suo
	 * piano arma una reazione (`FRTUnitSlotsView::bReactionArmed`).
	 *
	 * 🔴 **Gli slot si costruiscono solo per un'unita' comandata**, con la stessa barriera di
	 * `URTSelectedUnitPanelWidget::GetSlots()`: per un'avversaria, un'alleata del bot o un altro gruppo di controllo
	 * `BuildUnitSlots` non viene chiamata, e la risposta e' falso. Il piano non lascia il core (D-478 punto 4).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	bool IsReactionArmed(FName HeroId) const;
};

/** `WBP_RT_SelectedUnitPanel` — dettaglio di chi si sta comandando: carta, slot occupati. */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTSelectedUnitPanelWidget : public URTScreenHudWidgetBase
{
	GENERATED_BODY()

public:
	/**
	 * Falso quando non c'e' selezione: il pannello si nasconde invece di mostrare una carta vuota.
	 *
	 * ⚠️ **Significa «comando un'unita'», non «il pannello ha qualcosa da mostrare»**: da `#705` il soggetto
	 * puo' essere anche un'unita' **ispezionata**, che non si comanda. Per «c'e' qualcosa da mostrare» esiste
	 * `HasSubject()`. Il nome resta questo perche' i grafi esistenti lo leggono col significato di sempre, e
	 * cambiarlo sotto di loro avrebbe spostato il difetto invece di aggiungere il caso.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	bool HasSelection() const;

	/**
	 * Vero quando il pannello ha un soggetto: comandato **oppure** ispezionato (`#705`).
	 *
	 * ⚠️ Chi disegna distingue i due casi con `HasSelection()` e con `GetSlots().bAuthorized`, non con
	 * questo: qui si risponde solo *«c'e' qualcosa da mostrare»*.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	bool HasSubject() const;

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	FRTUnitCardView GetCard() const;

	/**
	 * I tre slot del turno. ⚠️ Non sono tre booleani indipendenti: un'azione che dichiara `MovementAndMain`
	 * ne occupa due - nessuna oggi, `Action.Sprint` fino a [D-028]. Chi lo decide e' il catalogo, non il widget.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	FRTUnitSlotsView GetSlots() const;

	/**
	 * Gli avvisi del piano dell'unita' COMANDATA ([D-480], #3622), sopra il pannello in `MiddleLeft`. Vuoto in
	 * Risoluzione, e vuoto per un'unita' solo ispezionata: segue il comando, come `GetSlots()`.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	TArray<FRTPlanWarningView> GetPlanWarnings() const;

	/**
	 * L'ANDATURA dello slot movimento, pronta da legare a un `TextBlock` (`#1410` `AC-1`, [D-425]).
	 *
	 * 🔑 **Delega a `ARTHUD::DescribeMovementProfile`, che e' la sede unica.** La stessa stringa la
	 * compone `ComposeSlotLines` per il Canvas: due rese dello stesso fatto divergerebbero al primo profilo
	 * nuovo, e il giocatore leggerebbe un'andatura diversa a seconda di quale HUD sta guardando.
	 *
	 * ⚠️ **Vuoto significa «cammino, o niente da dire»**, non «dato mancante«: `Move` e `Still` non si
	 * nominano. Chi lo disegna nasconda il blocco quando e' vuoto invece di lasciare una riga spenta.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	FText GetMovementProfileText() const;

	/**
	 * Che cosa riempie lo slot MOVIMENTO, pronto da legare al `Text` di `MovementText` (`#1410` `AC-1`).
	 *
	 * 🔑 **Delega a `ARTHUD::DescribeMovementSlot`, che e' la sede unica**: e' la stessa funzione da cui il
	 * Canvas compone la propria riga. ⏱️ *Fino al 2026-09-17 il binding di `MovementText` leggeva
	 * `GetSlots()` e mostrava il `DisplayName` grezzo* — due rese dello stesso fatto che divergevano gia'
	 * prima che [D-425] aggiungesse l'andatura.
	 *
	 * ⛔ **Rende il CONTENUTO senza etichetta**: i tre `TextBlock` del pannello mostrano il solo valore, e
	 * anteporre «Movimento: » qui lo farebbe comparire due volte a chi guarda.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	FText GetMovementSlotText() const;

protected:
	/**
	 * Il soggetto del pannello: l'unita' comandata se c'e', altrimenti quella ispezionata (`#705`).
	 *
	 * ⛔ **Non e' `UFUNCTION`, e resta protetta**: i Blueprint vedono le **viste**, mai le unita' — la stessa
	 * regola di `GetSelectedUnit()`. E chi la usa in C++ deve sapere che il soggetto puo' essere avversario:
	 * `GetSlots()` non passa da qui, di proposito.
	 */
	const ARTUnit* GetSubject() const;
};

/** `WBP_RT_ActionDock` — le azioni della selezionata, con stato disponibile / armata / in ricarica. */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTActionDockWidget : public URTScreenHudWidgetBase
{
	GENERATED_BODY()

public:
	/** Una riga per azione del kit, nell'ordine del kit: l'indice e' quello che l'hotkey arma. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	TArray<FRTAbilityCooldownView> GetActions() const;

	/**
	 * Le stesse azioni in ORDINE DI LETTURA — Comuni, Base, Kit — per disporre la barra (`#3478`, [D-455]).
	 * ⛔ **E' l'ordine a schermo, non l'identita'**: ogni voce porta il proprio `AbilityIndex`, ed e' quello
	 * che si arma. Il separatore va dove `Group` cambia IN QUESTA lista, che per costruzione e' contigua.
	 *
	 * ➕ **Senza un'unita' comandata consegna la STRUTTURA della barra** ([D-460], #3494): le Comuni spente e i
	 * vuoti di Base e Kit, da `URTHudViewModel::BuildIdleBar` sulle unita' della propria squadra. ⚠️ Solo
	 * questa porta: `GetActions()` resta l'identita', vuota senza unita'.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	TArray<FRTAbilityCooldownView> GetActionsInReadingOrder() const;

	/**
	 * La lettura del movimento per l'estremita' destra della barra (`#3470`, [D-456] punto 3).
	 * ⛔ **Dall'unita' comandata soltanto**, come le azioni: un soggetto ispezionato da' il default non
	 * autorizzato, e chi disegna nasconde l'indicatore.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	FRTMovementReadoutView GetMovementReadout() const;

	/**
	 * 🔴 **Il click sul badge `M`: dichiara o ritira `Sneak`** ([D-457], #3470). Inoltra a
	 * `ARTPlayerController::ToggleSneakDeclaration`, che e' il corpo del tasto: nessuna regola qui, e il grafo
	 * non compone la chiamata da se'. Senza controller non fa nulla.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HUD")
	void ToggleSneak();

	/**
	 * L'indice dell'azione ARMATA, o `INDEX_NONE`.
	 *
	 * `INDEX_NONE` non e' un caso limite: e' lo stato NEUTRO di [D-128], quello in cui il giocatore non ha
	 * armato nulla e un click su un nemico ispeziona. Il dock deve poterlo mostrare — nessuno slot acceso —
	 * altrimenti a schermo sembra sempre esserci un'azione pronta a partire.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	int32 GetArmedActionIndex() const;

	// ------------------------------------------------------------------------------------------------
	// La lettura del movimento, collegata PER NOME (`#3489`): lo stesso disegno di `RTHeroProfileWidget`.
	//
	// 🔑 Un binding authorato nel `.uasset` non si diffa e non si testa, e il bridge MCP non sa scriverlo.
	// Il Designer dichiara i widget con questi nomi; il C++ li riempie. Tutti OPZIONALI: un WBP che non ne
	// dichiara uno resta valido.
	// ------------------------------------------------------------------------------------------------

	/** Il contenitore della lettura: `Collapsed` quando la vista non e' autorizzata, mai spento a meta'. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|Dock")
	TObjectPtr<class UWidget> MovementReadout;

	/** `Move ×1`, `Sprint ×2`, `Withdraw ×0,25`… da `GetMovementReadout().Label`. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|Dock")
	TObjectPtr<class UTextBlock> MovementReadoutText;

	/** Il badge `M`, cliccabile ([D-457]): il click chiama `ToggleSneak()`. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|Dock")
	TObjectPtr<class UButton> SneakBadge;

	/** Il tasto del badge, da `GetMovementReadout().SneakKeyLabel`: mai scritto nel Designer. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|Dock")
	TObjectPtr<class UTextBlock> SneakBadgeText;

	/** L'opacita' del badge quando `Sneak` NON e' dichiarato: acceso = 1, spento = questo. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|HUD|Dock")
	float SneakBadgeIdleOpacity = 0.45f;

	/** Riempie i widget della lettura dalla vista. La chiama `NativeTick`; i test la chiamano a mano. */
	void RefreshMovementReadout();

	/** Collega il click del badge a `ToggleSneak()`. Idempotente; la chiama `NativeConstruct`. */
	void BindNamedButtons();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};

/**
 * `WBP_RT_PlanCommit` — **Conferma** e **Annulla** in `TopRight` ([D-456] punto 4, [D-458], #3471).
 *
 * 🔑 **Due pulsanti, due porte che esistono gia'**: `Conferma` e' `Invio` — dichiara o ritratta il piano
 * dell'unita' selezionata, e non risolve niente — e `Annulla` e' l'**intero** Back del tasto destro, che durante
 * il countdown ritira il Ready. Il widget **inoltra**, non decide: nessuna guardia, nessun controllo di stato
 * qui dentro, esattamente come lo slot con `ArmKitAbility`.
 *
 * ⛔ **Non e' il `LockIn` di `Spazio`**, che chiude il turno per tutti: [D-458] lo ha escluso dal pulsante.
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTPlanCommitWidget : public URTScreenHudWidgetBase
{
	GENERATED_BODY()

public:
	/** Il click su `Conferma`: inoltra a `ARTPlayerController::TogglePlanDeclaration`. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HUD")
	void Confirm();

	/** Il click su `Annulla`: inoltra a `ARTPlayerController::UndoStep`. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HUD")
	void Undo();

	/** C'e' un'unita' COMANDATA a cui i pulsanti parlano: senza, chi disegna li spegne. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	bool HasCommandedUnit() const;

	/** Il piano dell'unita' selezionata e' dichiarato: `Conferma` diventa «ritratta». Falso senza unita'. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	bool IsPlanDeclared() const;

	/** Il tasto di `Conferma`, da `ARTPlayerController::DeclarePlanHotkey()`: mai scritto nel grafo. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	FText GetConfirmKeyLabel() const;

	/** Il tasto da tastiera di `Annulla`, da `ARTPlayerController::UndoKeyboardHotkey()`. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	FText GetUndoKeyLabel() const;

	/**
	 * Il contatore accanto a `Conferma` ([D-494], #3622): un numero per livello, sommato su TUTTE le unita'
	 * comandate, perche' `Conferma` le chiude tutte. Zero in Risoluzione. Un'unita' non comandata non entra.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	FRTPlanWarningCounts GetPlanWarningCounts() const;

	// ------------------------------------------------------------------------------------------------
	// I pulsanti, collegati PER NOME (`#3489`). Il Designer li dichiara; il C++ ne collega il click alle porte
	// e ne scrive il testo. Opzionali.
	// ------------------------------------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|PlanCommit")
	TObjectPtr<class UButton> ConfirmButton;

	/** «Conferma» o «Ritira», col tasto di `GetConfirmKeyLabel()`. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|PlanCommit")
	TObjectPtr<class UTextBlock> ConfirmText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|PlanCommit")
	TObjectPtr<class UButton> UndoButton;

	/** «Annulla», col tasto di `GetUndoKeyLabel()`. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|PlanCommit")
	TObjectPtr<class UTextBlock> UndoText;

	/**
	 * Testi e abilitazione. La chiama `NativeTick`; i test la chiamano a mano.
	 *
	 * ⚠️ **Si spegne `Conferma` senza un'unita' comandata, NON `Annulla`.** Il Back non chiede un'unita': il
	 * suo primo ramo ritira il Ready durante il countdown, e gli altri smontano ispettore e focus di fase. Un
	 * `Annulla` spento senza selezione toglierebbe al giocatore il ritiro del Ready proprio quando ha
	 * deselezionato per guardare la mappa.
	 */
	void RefreshButtons();

	/** Collega i click a `Confirm()` e `Undo()`. Idempotente; la chiama `NativeConstruct`. */
	void BindNamedButtons();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};

/**
 * `WBP_RT_ActionTooltip` — il tooltip di uno slot (`#3499`). Come lo slot, **riceve** la vista e non va a prenderla:
 * la compone `URTHudViewModel::BuildActionTooltip`, e questa classe la scrive nelle porte.
 *
 * 🔑 **Le porte hanno un nome, e il C++ le scrive**, come per lo slot (`#3489`, `#3498`): il Blueprint disegna e
 * basta. Una porta mancante non e' un errore — il testo semplice resta il ripiego — ma un nome sbagliato lascia la
 * porta spenta senza dire niente, ed e' il gate della seduta a verificarli.
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTActionTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Lo slot chiama questa, una volta per cambio azione. Scrive le porte, poi chiama `OnViewChanged`. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HUD")
	void SetView(const FRTActionTooltipView& InView);

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	FRTActionTooltipView GetView() const { return View; }

	/** Le righe dei numeri in un testo solo, «Etichetta  valore» per riga: e' cio' che riceve `LinesText`. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	FText GetLinesText() const;

	/** Il nome dell'azione. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|Tooltip")
	TObjectPtr<UTextBlock> TitleText;

	/** La frase d'autore. Collassata quando l'azione non ne ha una. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|Tooltip")
	TObjectPtr<UTextBlock> DescriptionText;

	/** Le righe dei numeri: fase, slot, portata, ricarica, danno. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|Tooltip")
	TObjectPtr<UTextBlock> LinesText;

	/** Il perche' di uno stato spento. Collassata quando non c'e' niente da spiegare. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|Tooltip")
	TObjectPtr<UTextBlock> ReasonText;

	/** Il Blueprint che vuole aggiungere qualcosa lo fa qui, sopra cio' che il C++ ha gia' scritto. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RefactorTactics|HUD")
	void OnViewChanged();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	FRTActionTooltipView View;
};

/**
 * `WBP_RT_ActionSlot` — UNA azione. Non estende la base di contesto: **riceve** i dati, non va a prenderli.
 *
 * E' la differenza fra un elemento di lista e un pannello: un dock con sei slot che leggono ciascuno il
 * proprio stato dal gioco fa sei letture per frame e puo' mostrarne una disallineata dalle altre.
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTActionSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	FRTAbilityCooldownView Action;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	bool bArmed = false;

	/**
	 * La classe del tooltip (`#3499`), da assegnare nei default di `WBP_RT_ActionSlot`.
	 *
	 * ⚠️ **Vuota, il tooltip esiste lo stesso, in testo semplice** (`URTHudViewModel::ComposeTooltipText`): il dato
	 * arriva prima del disegno, e il giocatore lo legge anche prima della seduta che lo veste.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HUD")
	TSubclassOf<URTActionTooltipWidget> TooltipClass;

	/** Il tooltip composto all'ultimo `SetAction`. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	FRTActionTooltipView GetTooltipView() const { return TooltipView; }

	/** Quante volte lo slot ha consegnato un tooltip a Slate (`SetToolTip`, `SetToolTipText`). Serve ai test. */
	int32 GetTooltipDeliveriesForTest() const { return TooltipDeliveries; }

	/** Il dock chiama questa; lo slot non si aggiorna da solo. */
	/**
	 * Il catalogo da cui risolvere l'icona. Lo **riceve**, non va a prenderlo.
	 *
	 * ⚠️ E' la stessa disciplina per cui questa classe non estende `URTScreenHudWidgetBase`: uno slot che
	 * andasse a cercarsi il catalogo sarebbe un secondo posto che decide da dove viene, e sei slot che lo
	 * cercano ciascuno per conto proprio possono trovarne sei diversi. Chi crea lo slot lo passa.
	 *
	 * ✅ E' un `URTIconCatalogData*`, **non** una `UTexture2D*`: `ScreenHud.WidgetApiExposesNoTexture`
	 * continua a valere, ed e' il punto — l'icona resta una CHIAVE risolta da un catalogo ([D-031]), non un
	 * asset che il widget si tiene.
	 *
	 * ⚠️ **Si chiama `ReceivedCatalog` e non `IconCatalog`, e il nome e' una CORREZIONE.** Con `IconCatalog`
	 * il getter automatico del Blueprint diventava `Get IconCatalog`, indistinguibile a occhio da
	 * `Get Icon Catalog` — la funzione che la base espone — nel menu di ricerca dei nodi. Un autore che
	 * trascinava dal pin `In Catalog` del dock prendeva la variabile dello SLOT invece della funzione della
	 * base, e il compilatore rispondeva *«This blueprint (self) is not a RTActionSlotWidget»*: un errore che
	 * nomina il sintomo e non la causa. E' successo davvero, due volte di fila.
	 *
	 * ∴ i due nomi ora divergono, e il verbo dice anche la disciplina: lo slot **riceve** il catalogo.
	 */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "RefactorTactics|HUD")
	TObjectPtr<const URTIconCatalogData> ReceivedCatalog;

	/**
	 * ⚠️ `InCatalog` ha un default `nullptr` per una ragione dichiarata: senza catalogo lo slot mostra il
	 * missing-icon, che e' il comportamento voluto, e un chiamante che non lo passa non e' un errore da
	 * bloccare. Chi lo passa e' il dock, che il catalogo lo raggiunge con `GetIconCatalog()`.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HUD")
	void SetAction(const FRTAbilityCooldownView& InAction, bool bInArmed,
		const URTIconCatalogData* InCatalog = nullptr);

	/**
	 * 🔴 **Il click: inoltra ad `ARTPlayerController::ArmKitAbility` il PROPRIO indice di kit, e nient'altro**
	 * (`#2826`).
	 *
	 * 🔑 **Esiste perche' lo slot non aveva nessuna porta in USCITA.** `SetAction` e' entrante,
	 * `GetResolvedIcon`, `GetIconId` e `GetActionLine` sono pure: il dock mostrava l'azione e il click non
	 * aveva dove andare. Il contrasto si misura — `ArmKitAbility` e' `BlueprintCallable`, ed e' testata da
	 * `PlayerInput.TheDockPortArmsAndDisarms`, ma `git grep -l ArmKitAbility -- Content/` non trovava
	 * **nessun** chiamante: la porta c'era e nessuno ci bussava.
	 *
	 * ⛔ **Nessuna formula, nessun controllo di disponibilita', nessun reason code qui dentro.** Cooldown,
	 * slot reazione, self-target e input bloccato li decide `SelectAbilityForCurrent`, che e' anche il corpo
	 * che i dieci tasti numerici attraversano: **la stessa porta del tasto, non una seconda**. Un controllo
	 * scritto qui sarebbe un secondo giudice con la propria copia delle regole, e divergerebbe al primo
	 * cambio — il difetto che `#2826` nomina per il proprio percorso di armamento.
	 *
	 * ⚠️ **E il grafo non deve comporre la chiamata da se'.** Un `Get Player Controller` + `Cast` dentro
	 * `WBP_RT_ActionSlot` metterebbe la risoluzione del proprietario in chi disegna, e sei slot potrebbero
	 * risolverne sei diversi. Il `.uasset` chiama **questa**, e basta.
	 *
	 * ⛔ **Fail-closed su uno slot MAI assegnato**, ed e' la stessa guardia che `URTFastDecisionOptionWidget::Choose()`
	 * ha sul proprio proprietario. `Action.AbilityIndex` vale `INDEX_NONE` finche' `SetAction` non passa una
	 * posizione, e `ArmKitAbility(INDEX_NONE)` **DISARMA**: senza la guardia un riquadro rimasto vuoto — o
	 * sopravvissuto alla ricostruzione della lista — spegnerebbe l'azione armata da un altro.
	 *
	 * ⚠️ **Non e' un controllo di disponibilita', e la differenza e' misurabile**: una posizione di kit
	 * **vuota** porta comunque il proprio indice — `Cooldowns[i].AbilityIndex == i` vale per costruzione
	 * (`#2987`) — quindi passa di qui e a rifiutarla e' il core.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HUD")
	void Activate();

	/**
	 * Inietta il controller a cui `Activate()` inoltra, senza passare da un `ULocalPlayer`. E' il modo in cui
	 * i test guidano questo widget, ed e' la stessa forma delle tre iniezioni di `URTScreenHudWidgetBase`.
	 *
	 * 🔴 **Senza, `Activate()` non sarebbe verificabile affatto in headless — e il difetto si presenterebbe
	 * come un VERDE.** `UUserWidget::SetOwningPlayer` memorizza il **`ULocalPlayer`**, che una run headless
	 * non ha: `GetOwningPlayer()` resta nullo anche dopo aver spawnato un `ARTPlayerController` e avergli
	 * selezionato un'unita'. Un test scritto senza questa porta misurerebbe il ramo «nessun proprietario»
	 * credendo di misurare il click. Il prezzo e' gia' stato pagato una volta su questa stessa famiglia di
	 * widget, e lo racconta `SetSelectedUnitForTest`: *«il Blueprint passava `false` fisso, e
	 * `ActionDockShowsTheNeutralState` era verde»*.
	 *
	 * In gioco resta nulla e la verita' e' `GetOwningPlayer()`. **Non** e' esposta ai Blueprint: sarebbe un
	 * secondo canale per decidere a chi arriva il click.
	 *
	 * ⚠️ Definita nel `.cpp` come le sorelle: `ARTPlayerController` e' solo forward-declared qui, e
	 * `TWeakObjectPtr::operator=` vuole il tipo completo.
	 */
	void SetArmingControllerForTest(ARTPlayerController* InController);

	/**
	 * L'icona di questa azione, risolta dal catalogo — o il missing-icon.
	 *
	 * 🔴 **Esiste perche' il Blueprint non deve comporre la chiamata da solo.** `ResolveIcon` vuole tre
	 * ingressi (catalogo, chiave, consumer) e il terzo serve al log: *«senza, "icona mancante" in una
	 * partita con dieci widget non dice dove guardare»*. Lasciarli comporre nel grafo significherebbe che
	 * ogni slot puo' scriverci una stringa diversa, o vuota — e la warning perderebbe l'unica cosa per cui
	 * esiste. Qui il consumer e' fisso e corretto per costruzione.
	 *
	 * ⚠️ **`BlueprintPure` nonostante `ResolveIcon` logghi**, e la differenza rispetto a li' e' il punto di
	 * chiamata: questa si consuma dentro `OnActionChanged` — un evento, una volta per cambio azione — non
	 * in un property binding valutato a ogni frame.
	 */
	// ⛔ **Niente `UFUNCTION` qui, ed e' il fix di `#3178`.** Esposta al Blueprint, questa firma faceva
	// uscire una texture dall'API del widget dentro una struct — cio' che [D-031] vieta e che il gate
	// `WidgetApiExposesNoTexture` non vedeva, perche' il suo predicato non scendeva nelle `FStructProperty`.
	// Resta un metodo C++ pubblico: i test la chiamano, il grafo no.
	FRTIconResolution GetResolvedIcon() const;

	/**
	 * Applica l'icona di questa azione all'`UImage` indicata: risolve dal catalogo, **carica** e imposta il
	 * brush. Sostituisce la catena che il grafo componeva da solo (`#3178`).
	 *
	 * 🔴 **Il difetto che chiude, e perche' non era visibile.** `WBP_RT_ActionSlot` faceva
	 * `SetBrushFromTexture(IconImage, ResolveSoftReference(Break(GetResolvedIcon)))`. Ma **`Resolve Soft
	 * Reference` non carica**: la doc del nodo dice *«If the object isn't already loaded in memory this will
	 * return none»* (`K2Node_ConvertAsset.cpp`). Con la texture non in memoria il brush restava quello di
	 * default — un rettangolo bianco — **senza nessun warning**, perche' la chiave si era risolta benissimo.
	 *
	 * ⚠️ **`LoadSynchronous` e non `Get`**, ed e' l'unica riga che conta: `Get()` e' il `Resolve Soft
	 * Reference` del grafo e ha lo stesso difetto. Il costo sta dove il docstring di `GetResolvedIcon` lo
	 * voleva — **una volta per cambio azione**, dentro `OnActionChanged` — non a ogni frame.
	 *
	 * ⚠️ **Anche il ripiego passava di qui**: `MissingIcon` e' a sua volta una `TSoftObjectPtr`,
	 * quindi un'icona di fallback non sarebbe comparsa comunque. Il difetto non aveva un ramo sano.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HUD")
	void ApplyResolvedIconTo(UImage* Target);

	// ------------------------------------------------------------------------------------------------
	// L'aspetto dello slot, collegato PER NOME (`#3489`): lo stesso disegno di `RTHeroProfileWidget`.
	//
	// 🔑 **Il C++ sceglie QUALE widget accendere; forma, texture e posizione restano del Designer.** Lo stato
	// e' di `URTHudViewModel::ResolveSlotState` e il segno di fase di `PhaseMarkFor`: qui non nasce nessuna
	// regola, si traduce un valore in visibilita' e colore. E' cio' che un grafo Blueprint ricomporrebbe a
	// mano — ed e' cio' che il bridge MCP non sa legare come property binding.
	//
	// ⚠️ Tutti OPZIONALI: un WBP che non dichiara uno di questi nomi resta valido e compila.
	// ------------------------------------------------------------------------------------------------

	/** La striscia di fase in cima allo slot. Colore da `PhaseColors[PhaseMark]`; `Collapsed` senza voce. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|Slot")
	TObjectPtr<class UBorder> PhaseStrip;

	/** L'etichetta di fase — il canale che non dipende dal colore ([D-232] punto 3). */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|Slot")
	TObjectPtr<class UTextBlock> PhaseLabelText;

	/** Il bordo dello slot. Colore da `FrameColors[stato]`. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|Slot")
	TObjectPtr<class UBorder> StateFrame;

	/** Secondo canale di `Selected`: la barra sotto lo slot. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|Slot")
	TObjectPtr<class UWidget> SelectedBar;

	/** Secondo canale di `Planned`: l'angolo pieno in alto a destra. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|Slot")
	TObjectPtr<class UWidget> PlannedCorner;

	/** Secondo canale di `Unavailable`: il tratteggio diagonale. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|Slot")
	TObjectPtr<class UWidget> UnavailableHatch;

	/** Secondo canale di `Invalid`: la ✕ ([D-459]). */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|Slot")
	TObjectPtr<class UWidget> InvalidMark;

	/** Secondo canale di `Warning`: il triangolo ([D-459]). */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|Slot")
	TObjectPtr<class UWidget> WarningMark;

	/**
	 * Il colore della striscia per segno di fase. Default: [D-233] e §32, pinnati da
	 * `ScreenHud.SlotPhaseStripReadsThePhaseMark`. Un segno SENZA voce chiude la striscia: e' il caso di
	 * `None`, e di `Cleanup`, che ha un'etichetta e non un colore ([D-232] §1, [D-233]) — la fase la dice
	 * `PhaseLabelText`.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|HUD|Slot")
	TMap<ERTActionPhaseMark, FLinearColor> PhaseColors;

	/**
	 * Il colore del bordo per stato. Default: la ricetta del mockup (`sorgente-mockup/Main.dc.html`, #3498).
	 *
	 * 🔑 **Su un `StateFrame` disegnato come `RoundedBox` e' il colore del CONTORNO**, e il fondo viene da
	 * `FillColors`; su un `Border` a texture e' il colore dell'intero bordo. ⚠️ I valori di #3498 valgono anche
	 * li': ricarica e indisponibile hanno il contorno `#2E3746`, e una reazione armata e' viola.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|HUD|Slot")
	TMap<ERTActionSlotState, FLinearColor> FrameColors;

	/** Il fondo dello slot per stato, su un `StateFrame` `RoundedBox` (#3498). Available `#212733`, Selected `#2B2918`… */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|HUD|Slot")
	TMap<ERTActionSlotState, FLinearColor> FillColors;

	/** Lo spessore del contorno per stato, in px: 2 dove il mockup marca lo stato col bordo, 1 altrove (#3498). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|HUD|Slot")
	TMap<ERTActionSlotState, float> FrameWidths;

	/** Il colore dell'icona per stato (#3498): ambra se armata, spenta in ricarica, bianca altrove. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|HUD|Slot")
	TMap<ERTActionSlotState, FLinearColor> IconTints;

	/** Il colore del nome per stato (#3498): spento in ricarica e indisponibile, grigio sullo slot vuoto. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|HUD|Slot")
	TMap<ERTActionSlotState, FLinearColor> NameTints;

	/** La striscia di uno slot indisponibile: grigia, al posto del colore della fase (`Stati.dc.html`). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|HUD|Slot")
	FLinearColor UnavailableStripColor;

	/** L'opacita' della striscia in ricarica: il colore della fase resta, attenuato (`Stati.dc.html`). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|HUD|Slot")
	float CooldownStripOpacity = 0.3f;

	/** Il colore dell'etichetta di fase. Su uno slot di reazione e' `ReactionLabelColor`, il viola chiaro di `REAZ.`. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|HUD|Slot")
	FLinearColor PhaseLabelColor;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|HUD|Slot")
	FLinearColor ReactionLabelColor;

	/**
	 * Il colore di `SelectedBar`: ambra; su una reazione armata prende `ReactionArmedFrame`. Lo scrive il C++ se
	 * la barra e' un `Border` (fondo) o un'`Image` (tinta). Il tratteggio della barra della reazione resta escluso.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|HUD|Slot")
	FLinearColor SelectedBarColor;

	/**
	 * 🔑 **Una REAZIONE armata non e' l'ambra di un'azione principale**: il mockup la dice viola — fondo `#221E3A`,
	 * contorno `#7C5CFF`, icona `#B9A8FF`. Lo stato resta `Selected`; cambia la resa, perche' lo slot e'
	 * `Reaction` (`Action.Slot`). Il tratteggio del contorno, che `RoundedBox` non sa fare, resta escluso.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|HUD|Slot")
	FLinearColor ReactionArmedFill;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|HUD|Slot")
	FLinearColor ReactionArmedFrame;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|HUD|Slot")
	FLinearColor ReactionArmedIcon;

	/** Il riquadro del tasto in alto a sinistra (#3498): `Collapsed` quando lo slot non ha un tasto. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|Slot")
	TObjectPtr<class UWidget> HotkeyBadge;

	/** Il tasto, da `Action.HotkeyLabel` — separato dal nome, come nel mockup (#3498). */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|Slot")
	TObjectPtr<class UTextBlock> HotkeyText;

	/** Il nome dell'azione, da `Action.DisplayName`, senza il tasto (#3498). */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|Slot")
	TObjectPtr<class UTextBlock> ActionNameText;

	/**
	 * L'intestazione del gruppo sopra lo slot — `COMUNI` · `BASE` · `KIT` (#3498). Visibile solo sulla prima voce
	 * di ogni gruppo (`bFirstOfGroup`); sulle altre e' `Hidden`, NON `Collapsed`, perche' tutti gli slot della
	 * fila devono restare alla stessa altezza.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|Slot")
	TObjectPtr<class UTextBlock> GroupHeaderText;

	/**
	 * L'alone di `Selected` (#3498): nel mockup 3 px d'ambra al 16% attorno allo slot, che un `RoundedBox` non
	 * disegna. Acceso solo su un'azione armata che NON e' una reazione: la reazione armata e' viola, senza alone.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|Slot")
	TObjectPtr<class UWidget> SelectedGlow;

	/**
	 * Il divisore che apre un gruppo (#3498): nel mockup una riga di 1 px `#203542` fra due gruppi, a 22 px da
	 * ciascuno. Acceso solo dove `bGroupBreakBefore`; il Designer lo disegna FUORI dallo slot, a sinistra, dentro
	 * il `GroupGap`. ⛔ Non e' un widget in `SlotBox`: sposterebbe ogni `GetChildAt(i)` (#3489).
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD|Slot")
	TObjectPtr<class UWidget> GroupDivider;

	/** Il testo dell'intestazione di un gruppo. `None` — una posizione vuota — si legge col Kit. */
	static FText GroupHeaderFor(ERTActionGroup Group);

	/** Lo spazio a sinistra di uno slot dentro un gruppo (`dati/tokens.json`: `gap`). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|HUD|Slot")
	float ItemGap = 8.f;

	/**
	 * Lo spazio a sinistra di uno slot che apre un gruppo: `separatore_gruppi` (22, `dati/tokens.json`) su ENTRAMBI
	 * i lati del divisore da 1 px, come nel mockup — 22 + 1 + 22 (#3498). ⌫ *Era 22 fino a #3498: il divisore non
	 * c'era, e lo spazio di un lato solo bastava.*
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|HUD|Slot")
	float GroupGap = 45.f;

	/**
	 * Il nome del widget che porta il secondo canale di uno stato, o `NAME_None` per gli stati che non ne
	 * hanno uno (`Empty`, `Available`). `Cooldown` lo ha gia': e' `CooldownText`, il numero di turni.
	 *
	 * 🔑 E' la tabella che `RefreshLook` applica e che il gate sull'asset legge: un nome qui e un widget nel
	 * Designer sono lo stesso fatto.
	 */
	static FName IndicatorNameFor(ERTActionSlotState State);

	/** Applica stato, fase e padding ai widget collegati. La chiama `SetAction`, prima di `OnActionChanged`. */
	void RefreshLook();

	explicit URTActionSlotWidget(const FObjectInitializer& ObjectInitializer);

	private:
	/** Il tooltip composto in `SetAction`, una volta per cambio azione come l'icona (`#3499`). */
	FRTActionTooltipView TooltipView;

	/** L'istanza di `TooltipClass`, creata la prima volta che serve e riusata. */
	UPROPERTY(Transient)
	TObjectPtr<URTActionTooltipWidget> ActionTooltip;

	/** Consegna `TooltipView`: al widget di `TooltipClass` se c'e', altrimenti come testo semplice. */
	void RefreshTooltip();

	/** Le consegne a Slate, contate: ogni consegna crea un `SToolTip` nuovo, e chiude quello aperto. */
	int32 TooltipDeliveries = 0;

	/** L'icona risolta UNA VOLTA, in `SetAction`. `GetResolvedIcon` la rende senza ricalcolare.
	 *
	 * 🔴 **Non e' un'ottimizzazione: e' il contratto dichiarato reso vero.** `ResolveIcon` LOGGA quando una
	 * chiave non si risolve, e il docstring di `GetResolvedIcon` prescrive «un evento, una volta per cambio
	 * azione» proprio per questo. Ma nulla lo impediva, e un property binding la chiama a ogni frame: nella
	 * seduta del 2026-09-11 sono state **16 388** righe di warning per **quattro** chiavi distinte, cioe' la
	 * stessa diagnostica ripetuta finche' non e' illeggibile.
	 *
	 * ⚠️ **E il costo non era solo il log**: `GetIconId` interroga il catalogo per scegliere fra chiave
	 * preferita e ripiego, e da un binding quell'attraversamento andava a ogni frame per ogni slot.
	 */
	FRTIconResolution CachedResolvedIcon;

	/**
	 * Vedi `SetArmingControllerForTest`. **Nullo in gioco**: il proprietario vero resta `GetOwningPlayer()`,
	 * e questo campo esiste solo perche' in headless quello non c'e'.
	 */
	UPROPERTY(Transient)
	TWeakObjectPtr<ARTPlayerController> ArmingControllerForTest;

	/**
	 * Il controller a cui inoltrare: l'iniezione dei test PRIMA, il proprietario poi.
	 *
	 * ⚠️ **L'ordine e' lo stesso di `URTScreenHudWidgetBase::GetSelectedUnit()`, e per la stessa ragione
	 * misurata**: in gioco l'iniezione e' sempre nulla, quindi anteporla non puo' scavalcare niente.
	 */
	ARTPlayerController* ResolveArmingController() const;
	public:

	/** Ridisegna. Il Blueprint la implementa: qui non c'e' layout. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RefactorTactics|HUD")
	void OnActionChanged();

	/**
	 * La CHIAVE dell'icona (`UI.Icon.Action.Move`), mai un asset.
	 *
	 * ⚠️ Il tipo di ritorno e' la regola: un `FName` non si puo' collegare a un `Image` senza passare dal
	 * catalogo, e il catalogo e' l'unico posto in cui una chiave diventa una texture (D-031). Un widget che
	 * ricevesse `UTexture2D` renderebbe la regola una raccomandazione.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	FName GetIconId() const;

	/**
	 * La riga testuale dello slot: **tasto**, nome, stato armato e motivo d'indisponibilita', gia' composti.
	 *
	 * 🔑 **Inoltra a `ARTHUD::ComposeAbilityLine` e non compone niente**, ed e' l'unica ragione per cui questa
	 * funzione puo' stare su un widget. Quella riga esisteva, era testata e **non aveva un chiamante fuori dai
	 * test** — il suo commento lo dichiarava: *«finche' #613 non la consuma, il suo unico chiamante sono i
	 * test»*. Non era raggiungibile dal Blueprint perche' non e' una `UFUNCTION`: il dato c'era e la porta no.
	 *
	 * ⛔ **Non aggiunge un secondo produttore della stessa stringa.** Se il grafo di `WBP_RT_ActionSlot`
	 * concatenasse da se' numero, nome e ricarica, due composizioni divergerebbero al primo cambio di formato
	 * — ed e' il difetto che `#2826` nomina per il proprio percorso di armamento: *«stesso percorso, non un
	 * secondo»*. `ScreenHud.ActionSlotLineIsTheSameComposerAsTheHud` lo pinna per **uguaglianza**, quindi una
	 * composizione locale non lo fa passare.
	 *
	 * ⚠️ **Torna il solo `Text`, non il colore.** La grammatica visiva dello stato armato resta nel Blueprint:
	 * portarla qui sposterebbe il dominio dentro la presentazione, ed e' cio' che lo spec panel del
	 * 2026-09-10 ha escluso per iscritto. Cio' che questa riga garantisce e' che lo stato sia leggibile anche
	 * **senza** colore — il prefisso `> ` dell'armata e il `(ricarica N)` sono testo.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	FText GetActionLine() const;
};

/**
 * `WBP_RT_FastDecision` — la finestra di reazione: countdown, bersaglio, e i bottoni della scelta (CP 14.6,
 * `#166`, voci **2 · 3 · 4** della DoD).
 *
 * ## Che cosa questa firma rende IMPOSSIBILE
 *
 * La DoD chiede *«nessuna logica di gioco nel widget»*, e fino a qui era una promessa che si verificava
 * leggendo il diff. Qui diventa una proprieta' della firma, su tre fronti:
 *
 *  1. **Non si nomina una risposta.** `ChooseOption` prende un **`int32`**, non una `FString`. Un widget che
 *     scrivesse `TEXT("FIRE")` nel proprio grafo sarebbe il nono produttore di quel letterale — otto siti
 *     dello scenario harness lo confrontano `CaseSensitive` — e il primo **fuori** dai test del core. Qui il
 *     widget puo' soltanto **indicare** un'opzione che il core ha prodotto.
 *     ⚠️ E un `FRTReactionWindowOptionView` come parametro non basterebbe: un Blueprint puo' costruirne uno
 *     ai default, con `Response` **vuota**, e una risposta vuota il core la legge come **scadenza**
 *     (`RTTurnManager.cpp`, `Response.IsEmpty()` → `DecisionOnTimeout`) — cioe' un bottone che sembra una
 *     scelta e vale un timeout. L'indice non ha un valore «di default plausibile»: fuori range non fa nulla.
 *  2. **Non si conta il tempo.** `GetRemainingSeconds()` inoltra a `GetOpenReactionWindowRemainingSeconds()`.
 *     Un countdown contato dal client e' un client che decide quando scade.
 *  3. **Non c'e' una seconda porta.** La base non espone il `TurnManager` ai Blueprint e
 *     `GetReactionWindow()` e' `protected`: cio' che si puo' leggere e' la vista **gia' sanitizzata** da
 *     `URTReactionWindowLibrary::FilterWindowForTeam`, che per un avversario e' ai default.
 *
 * ## ⚠️ Il numero mostrato e l'apertura sono due condizioni diverse, e solo una e' autorevole
 *
 * 🔴 **Il widget smette di accettare input quando `IsWindowOpen()` e' falso — mai quando il numero arriva a
 * zero.** I due orologi non hanno lo stesso tick: `GetOpenReactionWindowRemainingSeconds()` scorre col
 * `Tick` dell'Actor, il widget disegna col proprio `NativeTick`. Un countdown arrotondato per difetto mostra
 * `0` per almeno un frame **prima** che la finestra si chiuda, e un giocatore che preme in quel frame
 * vedrebbe il proprio input sparire in un prompt che dice zero ed e' ancora aperto. Il numero e' cosmesi;
 * l'apertura e' il contratto.
 *
 * ## ⚠️ Due finestre possono arrivare in fila, senza un frame di stacco
 *
 * Chiudere una finestra **riprende** la resolution, e la ripresa puo' aprirne un'altra nello stesso stack —
 * misurato da `#2723`, dove `Reactions.ViewModel.SubmitClosesAndResumesOverwatch` e' andato rosso proprio
 * su questo. Non viola *«un boundary → una decisione»* (sono due boundary), ma il `.uasset` deve
 * **ricostruirsi** sull'identita' della finestra invece di animare una transizione: 300 ms di apertura
 * costano il 10% di una finestra da 3,0 s, e il countdown autorevole non li aspetta.
 *
 * ⚠️ **Qui non c'e' layout**, come per gli altri di questo file. La ricetta del `.uasset` sta in
 * `docs/technical/runbooks/guida-screen-hud-umg.md`.
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTFastDecisionWidget : public URTScreenHudWidgetBase
{
	GENERATED_BODY()

public:
	/**
	 * C'e' una finestra da disegnare, ADESSO? Falso anche quando il view model non c'e' — un HUD senza
	 * proprietario mostra la finestra chiusa, non una finestra vuota.
	 *
	 * 🔑 **E' questa, e non il countdown, la condizione che governa la visibilita' e l'input** (vedi la nota
	 * sui due orologi nella dichiarazione della classe).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Reaction")
	bool IsWindowOpen() const;

	/**
	 * La finestra, gia' sanitizzata per chi la riceve: durata, opzioni, scelta sicura. Ai default quando
	 * nessuna attende — che e' la stessa forma che un avversario riceve, e non per caso.
	 *
	 * ⚠️ **`SafeResponse` dice QUALE delle `Options` e' la scelta sicura, e non e' la costante `HOLD`**: nel
	 * `Brace` e' `Hold Ground`. Un widget che scrivesse «tieni» a mano sarebbe corretto oggi e sbagliato con
	 * la prima finestra che non e' un Overwatch.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Reaction")
	FRTReactionWindowView GetWindow() const;

	/**
	 * Secondi che restano, dall'orologio **autorevole**. Negativo quando nessuna finestra attende — la
	 * convenzione di `FRTMatchHeaderView::PlanningSecondsRemaining`, dove `0.f` direbbe «scaduta adesso».
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Reaction")
	float GetRemainingSeconds() const;

	/**
	 * Inoltra la risposta che sta all'indice `OptionIndex` di `GetWindow().Options`.
	 *
	 * ⛔ **Fail-closed**: indice fuori range, o nessuna finestra aperta, non fanno **nulla** — con una warning
	 * che nomina indice e conteggio. Il caso non e' teorico: le opzioni cambiano quando la finestra cambia, e
	 * un bottone disegnato per la finestra precedente porta con se' il proprio indice.
	 *
	 * La legalita' non si giudica qui e nemmeno nel view model: passa da `AskReactionDecision` come quella di
	 * un bot. E l'identita' della finestra la mette `URTReactionWindowViewModel::SubmitResponse`, che nomina
	 * quella per cui la vista e' stata costruita — non «qualunque finestra ci sia adesso».
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Reaction")
	void ChooseOption(int32 OptionIndex);

	/** Quante risposte offre la finestra aperta. Zero quando non ce n'e' una. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Reaction")
	int32 GetOptionCount() const;

	/**
	 * Costruisce il bottone per l'opzione `OptionIndex`, gia' collegato a questa finestra.
	 *
	 * 🔑 **Esiste per tenere FUORI dal grafo tre cose che il grafo sbaglierebbe**, e ognuna ha un
	 * precedente in questo file:
	 *
	 *  1. **il proprietario** — se il grafo dovesse passarlo, `URTFastDecisionWidget` diventerebbe un tipo che
	 *     un Blueprint puo' maneggiare, e con esso una seconda porta su `ChooseOption` con un indice che non e'
	 *     il proprio;
	 *  2. **quale opzione e' la SICURA** — si decide confrontando con `SafeResponse`, non cercando la parola
	 *     `HOLD`: nel `Brace` si chiama `Hold Ground`;
	 *  3. **la risposta stessa** — che resta privata nel figlio e non attraversa mai il grafo.
	 *
	 * ⚠️ **La CLASSE arriva dal grafo e non da qui**, ed e' voluto: un percorso di `Content/` scritto in
	 * C++ sarebbe un riferimento duro a un `.uasset` dentro il modulo, che questo progetto non ha in nessun
	 * altro widget.
	 *
	 * ⛔ Indice fuori range o classe nulla -> `nullptr`, con una warning. Il chiamante e' un grafo: non deve
	 * poter costruire un bottone che risponde per un'opzione che non esiste.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Reaction")
	URTFastDecisionOptionWidget* MakeOptionWidget(
		TSubclassOf<URTFastDecisionOptionWidget> OptionClass, int32 OptionIndex);

	// -------------------------------------------------------------------------------------------------
	// I VESTITI DEI BINDING — §4.3 della guida: *«tutte le funzioni delle basi sono `BlueprintPure`: si
	// collegano DIRETTAMENTE a un binding di proprieta' (`Text`, `Visibility`)»*.
	//
	// 🔑 **Esistono perche' senza di loro il Designer non ha nulla da legare.** Un binding di `Visibility`
	// vuole una funzione che renda `ESlateVisibility`, uno di `Text` una che renda `FText`: `IsWindowOpen()`
	// rende `bool` e `GetRemainingSeconds()` rende `float`, e **nessuna delle due e' collegabile**. Le
	// alternative erano due, e questa e' la meno cara: comporre il vestito in un grafo Blueprint avrebbe
	// messo la formattazione — e con essa la regola dei due orologi — dentro un `.uasset` che nessun test
	// legge. E' lo stesso argomento con cui `URTTurnHeaderWidget::GetRoundCounterText` esiste come funzione
	// invece che come regola scritta nella guida.
	// -------------------------------------------------------------------------------------------------

	/**
	 * La visibilita' della finestra: `Visible` quando c'e' una domanda, `Collapsed` quando non c'e'.
	 *
	 * 🔴 **Segue `IsWindowOpen()`, MAI il countdown a zero**, ed e' il punto per cui questa funzione sta in
	 * C++ e non nel grafo: i due orologi non hanno lo stesso tick, e il numero disegnato tocca `0` per
	 * almeno un frame **prima** che la finestra si chiuda. Un binding costruito sul numero farebbe sparire
	 * il prompt mentre il gioco sta ancora aspettando una risposta — e la risposta mancata diventerebbe un
	 * `HoldTimeout` che nel TurnLog e' indistinguibile da una scelta.
	 *
	 * ⚠️ **`Collapsed` e non `Hidden`**: `Hidden` continua a occupare spazio nel layout, e una finestra
	 * chiusa lascerebbe un buco nel pannello che la ospita.
	 *
	 * ⚠️ **`Visible` e non `SelfHitTestInvisible`**: i bottoni delle opzioni devono ricevere il click, ed e'
	 * l'unica ragione per cui la scelta fra i due valori conta qui.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Reaction")
	ESlateVisibility GetWindowVisibility() const;

	/**
	 * Il countdown come lo legge un giocatore: `2.4` — un decimale, mai negativo. Vuoto senza finestra.
	 *
	 * ⚠️ **Un decimale e non un intero, e la scelta e' misurabile.** Arrotondando all'intero per difetto il
	 * numero mostrerebbe `0` per l'**ultimo secondo intero** di una finestra ancora aperta; con un decimale
	 * la stessa finestra mostra `0.0` solo negli ultimi ~50 ms. Il disallineamento fra numero e apertura non
	 * si elimina — sono due orologi — ma si riduce di venti volte, e quel che resta e' sotto la soglia in
	 * cui qualcuno prova a premere.
	 *
	 * ⛔ **Non e' il gate dell'input**: quello e' `GetWindowVisibility()`, che interroga l'apertura. Questo
	 * numero e' cosmesi.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Reaction")
	FText GetCountdownText() const;

	/**
	 * L'etichetta della finestra. Vuota quando non ce n'e' una.
	 *
	 * 🔴 **NON nomina il bersaglio, e non e' una semplificazione: oggi il bersaglio NON E' ESPRIMIBILE.** La
	 * DoD di CP 14.6 chiede *«UI `FIRE`/`HOLD` con countdown **e bersaglio**»*, ma l'unico riferimento al
	 * bersaglio che il DTO porta e' `FRTReactionWindowOptionView::TargetSnapshotIndex` — e la sua stessa
	 * dichiarazione vieta di risolverlo qui: *«e' un indice nello spazio di `MakeCurrentSnapshot`, che
	 * scarta i morti — NON un id stabile … chi lo risolvesse su un roster, su `StableUnitId` o su una lista
	 * di Actor nominerebbe l'unita' SBAGLIATA in ogni partita in cui qualcuno e' gia' caduto»*.
	 *
	 * ∴ mostrare un nome richiede che il **produttore** lo metta nel DTO — ha lo snapshot in mano, questo
	 * widget no. Finche' non c'e', un'etichetta neutra e' l'unica cosa onesta: inventare un nome qui
	 * sarebbe il difetto che quella dichiarazione esiste per impedire. Owner: `#166`, meta' «e bersaglio».
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Reaction")
	FText GetPromptText() const;

	// -------------------------------------------------------------------------------------------------
	// LA RICOSTRUZIONE DEI BOTTONI — chi dice QUANDO, e chi disegna COSA
	// -------------------------------------------------------------------------------------------------

	/**
	 * La finestra e' cambiata: ricostruisci `OptionsBox`. Il Blueprint lo implementa; qui non c'e' layout.
	 *
	 * 🔴 **Un evento e non un binding, e non e' una preferenza: e' l'unica forma disponibile.** L'identita'
	 * della finestra e' irraggiungibile dal grafo — `FRTReactionWindowView::Key` non e' `BlueprintReadOnly`,
	 * deliberatamente — quindi un Blueprint non puo' chiedersi «e' ancora la stessa?». Dedurlo da un
	 * surrogato (numero di opzioni, testo di un bottone) creerebbe un **secondo identificatore** della
	 * finestra.
	 *
	 * ⛔ **E ricostruire a ogni tick non e' l'alternativa economica: rompe il click.** Un click Slate e' due
	 * eventi su due frame — `MouseButtonDown` e `MouseButtonUp` — e devono atterrare sulla **stessa
	 * istanza** di widget. Svuotare e ripopolare il box a ogni frame non lo garantisce, e in una finestra da
	 * 3,0 s un click perso matura in `HoldTimeout` — che nel TurnLog e' indistinguibile da una scelta
	 * deliberata.
	 *
	 * ⚠️ **Scatta anche quando la finestra si CHIUDE** (identita' → vuota): e' il momento in cui i bottoni
	 * vanno tolti. Un evento che segnalasse solo le aperture lascerebbe a schermo i bottoni dell'ultima
	 * domanda.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "RefactorTactics|Reaction")
	void OnWindowChanged();

	/**
	 * Fa avanzare il rilevamento e dice se l'identita' e' cambiata (per i test).
	 *
	 * 🔴 **Esiste perche' `NativeTick` non gira in una run headless**, e senza questo il rilevamento sarebbe
	 * verificabile solo aprendo l'Editor — cioe' non verificabile dalla suite. E' la stessa ragione, e la
	 * stessa forma, di `SetSelectedUnitForTest`.
	 *
	 * ⚠️ **Consuma**: due chiamate di fila sulla stessa finestra danno `true` e poi `false`. E' cio' che
	 * rende il test capace di distinguere «e' cambiata» da «c'e' una finestra».
	 */
	bool PollWindowChangedForTest() { return PollWindowChanged(); }

protected:
	/**
	 * Rileva il cambio e chiama `OnWindowChanged`.
	 *
	 * ⚠️ **L'evento va per ULTIMO**, dopo che `LastWindowId` e' aggiornato — la stessa regola che
	 * `URTActionSlotWidget::SetAction` porta scritta: *«e' il Blueprint che disegna, e disegna leggendo i
	 * campi qui sopra; se partisse prima, un'implementazione leggerebbe il catalogo del turno PRECEDENTE»*.
	 * Qui il difetto sarebbe peggiore che un frame di ritardo: il grafo costruirebbe i bottoni della
	 * finestra precedente, e il primo click risponderebbe a una domanda che il gioco non sta piu' facendo.
	 */
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** Vero se l'identita' e' cambiata da questa chiamata; aggiorna `LastWindowId`. */
	bool PollWindowChanged();

	/**
	 * L'identita' della finestra per cui i bottoni sono stati costruiti.
	 *
	 * 🔑 **Non e' una copia della vista: e' cio' che distingue «un'altra finestra» da «la stessa».**
	 * `IsWindowOpen()` non basta — chiudere una finestra RIPRENDE la resolution, e la ripresa puo' aprirne
	 * subito un'altra (`#2723`): il widget vedrebbe «aperta» prima e dopo, e terrebbe i bottoni della
	 * domanda precedente.
	 */
	FString LastWindowId;
};

/**
 * `WBP_RT_FastDecisionOption` — UNA risposta della finestra. Non estende la base di contesto: **riceve** i
 * dati, non va a prenderli (CP 14.6, `#166`).
 *
 * 🔑 **Esiste per una ragione meccanica, non estetica.** In un `ForEach` di Blueprint non si puo' catturare
 * l'indice dentro un delegate: `OnClicked` non porta parametri, e tutti i bottoni finirebbero per
 * rispondere con lo stesso indice — l'ultimo. Un widget figlio che **tiene il proprio indice** e' la forma
 * che lo risolve, ed e' gia' il precedente di `URTActionSlotWidget`.
 *
 * ⛔ **E la risposta resta un indice anche qui.** Questo widget non espone `Response`: espone
 * `GetOptionLabel()`, che rende un `FText`. Un `FString` esposto sarebbe la porta da cui il letterale
 * `FIRE`/`HOLD` rientra nel grafo — il difetto che `FastDecisionApiCarriesNoAuthority` presidia un livello
 * piu' su.
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTFastDecisionOptionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** L'indice di questa opzione dentro `GetWindow().Options`. `INDEX_NONE` finche' nessuno lo assegna. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Reaction")
	int32 OptionIndex = INDEX_NONE;

	/**
	 * Vero se questa e' la scelta SICURA — quella che si applica allo scadere.
	 *
	 * ⚠️ **Lo decide il C++ confrontando con `SafeResponse`, non il widget guardando il testo.** Nel `Brace`
	 * la scelta sicura si chiama `Hold Ground`, non `HOLD`: un grafo che cercasse la parola sarebbe corretto
	 * oggi e sbagliato con la prima finestra che non e' un Overwatch.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Reaction")
	bool bIsSafeChoice = false;

	/**
	 * Riceve l'opzione. Chi lo chiama e' il widget della finestra, che ha l'elenco e il proprietario.
	 *
	 * ⚠️ L'evento parte per ULTIMO, dopo i campi: vedi `URTActionSlotWidget::SetAction`.
	 */
	void SetOption(URTFastDecisionWidget* InOwner, const FRTReactionWindowOptionView& InOption,
		int32 InIndex, bool bInIsSafe);

	/** L'etichetta da stampare sul bottone: il NOME della risposta (`RTReactionResponseText`). `FText`, mai la stringa di risposta. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Reaction")
	FText GetOptionLabel() const;

	/** La frase dell'opzione, generica e senza bersaglio ([D-491]). Vuota per una risposta senza voce. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Reaction")
	FText GetOptionDescription() const;

	/**
	 * Il tasto che sceglie questa opzione: il tasto del kit nella stessa posizione ([D-491]). Viene da
	 * `ARTPlayerController::ReactionOptionKeyLabel`, cioe' dalla lista che mappa i tasti: il badge non puo' dire
	 * un tasto diverso da quello che risponde.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Reaction")
	FText GetKeyLabel() const;

	/** Il click: inoltra al widget della finestra il proprio indice, e nient'altro. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Reaction")
	void Choose();

	/** Ridisegna. Il Blueprint la implementa: qui non c'e' layout. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RefactorTactics|Reaction")
	void OnOptionChanged();

private:
	/**
	 * Chi ha la finestra. **Weak, e non esposto ai Blueprint**: un grafo che raggiungesse il widget della
	 * finestra da qui avrebbe una seconda porta su `ChooseOption`, con un indice che non e' il proprio.
	 */
	UPROPERTY(Transient)
	TWeakObjectPtr<URTFastDecisionWidget> Owner;

	/** La risposta esatta, per l'etichetta. Privata: il grafo non la vede e non puo' comporla. */
	FRTReactionWindowOptionView Option;
};

/**
 * `WBP_RT_TacticalHUD` — il contenitore a schermo intero: zone Top / Left / Bottom / Right, **centro libero**.
 *
 * ⚠️ Il centro libero e' una regola di LAYOUT e vive nel `.uasset`: qui non c'e' modo di imporlo. La ragione
 * per cui esiste va pero' scritta dove qualcuno la legge — il §4.2 continua a disegnare path, AoE e barre
 * ancorate **sopra la mappa**, e un pannello centrale glieli coprirebbe. La guida lo dichiara come vincolo
 * verificabile a occhio in `PIE-V01-HUD`.
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTTacticalHUDWidget : public URTScreenHudWidgetBase
{
	GENERATED_BODY()

public:
	/**
	 * Il catalogo iconografico da cui i figli risolvono le chiavi. `EditDefaultsOnly`: e' un dato di
	 * configurazione, non uno stato.
	 *
	 * ⚠️ Puo' essere nullo, e non e' un difetto da nascondere: il catalogo e' un `.uasset` di `#220` che oggi
	 * **non esiste**. `ResolveIcon` restituisce il missing-icon con `bResolved = false` e una warning che
	 * nomina la chiave — a schermo si vede che manca, invece di un buco silenzioso.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|HUD")
	TObjectPtr<URTIconCatalogData> IconCatalog;

	/**
	 * Le righe che dicono **chi del §4.1 e' stato costruito e chi no**, a partire da `Radice` e scendendo
	 * DENTRO ogni `UUserWidget` innestato.
	 *
	 * 🔑 **La ricorsione e' la ragione per cui questa funzione esiste**, e non duplica i test dell'albero:
	 * `UWidgetTree::ForEachWidget` cammina l'albero di UN Blueprint e **si ferma sui `UUserWidget`
	 * innestati, che hanno un albero loro** — limite dichiarato in `RTMatchWidgetAssetTests.cpp`. Un
	 * `WBP_RT_ActionSlot` dentro `WBP_RT_ActionDock` non lo vede nessun test di quel file.
	 *
	 * ⚠️ **Guarda cio' che e' stato COSTRUITO, non cio' che il `.uasset` dichiara.** E' la differenza che
	 * `PIE-V01-SCREENHUD` (`#613`, seduta `U49`) misura: i test headless provano l'albero PROGETTATO, e una
	 * partita puo' comunque non montarne un pezzo.
	 *
	 * Pura: non tocca stato e non emette log. Chi la chiama decide dove stamparla.
	 */
	static TArray<FString> ComposeMountReport(const UUserWidget* Radice);

protected:
	/**
	 * Emette `ComposeMountReport` su `LogRT`. **Non e' decorazione**: senza, una partita a cui manca un
	 * pezzo del §4.1 e una completa producono lo stesso log — nessuno — ed e' la condizione in cui `U49`
	 * si e' trovata, a giudicare a occhio quale widget mancasse.
	 */
	virtual void NativeConstruct() override;
};
