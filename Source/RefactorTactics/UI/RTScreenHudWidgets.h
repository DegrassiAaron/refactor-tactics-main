#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/RTHudViewModel.h"
#include "UI/RTIconCatalogData.h" // FRTIconResolution e' un valore di ritorno: serve la definizione, non basta la forward
#include "Turn/RTReactionWindowView.h" // idem per FRTReactionWindowView, reso per valore da `URTFastDecisionWidget`
#include "RTScreenHudWidgets.generated.h"

class ARTTurnManager;
class ARTUnit;
class URTIconCatalogData;
class URTReactionWindowViewModel;

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
 * sempre aggiungersi una variabile `Texture2D` o un riferimento diretto a un Actor — nessun gate lo
 * impedisce, perche' i `.uasset` non sono versionati in questo repository. La guida lo scrive come regola,
 * e `RefactorTactics.ScreenHud.WidgetApiExposesNoTexture` lo pinna per la parte che il codice controlla.
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

	/**
	 * Vedi `SetSelectedUnitForTest`. Nulla in gioco: la selezione vera resta del `PlayerController`.
	 *
	 * ⚠️ Non `TWeakObjectPtr<const ARTUnit>`: il template non accetta un tipo `const` e l'assegnazione non
	 * compila. La const-ness sta dove serve — `GetSelectedUnit()` restituisce comunque un puntatore const.
	 */
	UPROPERTY(Transient)
	TWeakObjectPtr<ARTUnit> SelectedUnitForTest;

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
};

/** `WBP_RT_SelectedUnitPanel` — dettaglio di chi si sta comandando: carta, slot occupati. */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTSelectedUnitPanelWidget : public URTScreenHudWidgetBase
{
	GENERATED_BODY()

public:
	/** Falso quando non c'e' selezione: il pannello si nasconde invece di mostrare una carta vuota. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	bool HasSelection() const;

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	FRTUnitCardView GetCard() const;

	/**
	 * I tre slot del turno. ⚠️ Non sono tre booleani indipendenti: un'azione che dichiara `MovementAndMain`
	 * ne occupa due - nessuna oggi, `Action.Sprint` fino a [D-028]. Chi lo decide e' il catalogo, non il widget.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	FRTUnitSlotsView GetSlots() const;
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
	 * L'indice dell'azione ARMATA, o `INDEX_NONE`.
	 *
	 * `INDEX_NONE` non e' un caso limite: e' lo stato NEUTRO di [D-128], quello in cui il giocatore non ha
	 * armato nulla e un click su un nemico ispeziona. Il dock deve poterlo mostrare — nessuno slot acceso —
	 * altrimenti a schermo sembra sempre esserci un'azione pronta a partire.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	int32 GetArmedActionIndex() const;
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
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	FRTIconResolution GetResolvedIcon() const;

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

	/** L'etichetta da stampare sul bottone. `FText`, mai la stringa di risposta. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Reaction")
	FText GetOptionLabel() const;

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
};
