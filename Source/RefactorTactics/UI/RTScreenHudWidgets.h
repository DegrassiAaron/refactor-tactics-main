#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/RTHudViewModel.h"
#include "RTScreenHudWidgets.generated.h"

class ARTTurnManager;
class ARTUnit;
class URTIconCatalogData;

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

	/** Inietta il contesto senza un `PlayerController`: e' il modo in cui i test guidano un widget. */
	void SetMatchContextForTest(ARTTurnManager* InTurnManager, int32 InPlayerTeamId);

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

	/** L'unita' selezionata dal giocatore, o `nullptr`. Protetta: i Blueprint vedono solo le VISTE. */
	const ARTUnit* GetSelectedUnit() const;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<ARTTurnManager> TurnManager;

	UPROPERTY(Transient)
	int32 PlayerTeamId = 0;
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

/** `WBP_RT_TeamRoster` — le unita' della PROPRIA squadra, morte comprese. */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTTeamRosterWidget : public URTScreenHudWidgetBase
{
	GENERATED_BODY()

public:
	/**
	 * Il roster. Non c'e' un parametro «mostra anche gli avversari», ed e' voluto: la regola di §4.1 diventa
	 * una proprieta' della firma invece di una disciplina.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	TArray<FRTUnitCardView> GetRoster() const;
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
	 * I tre slot del turno. ⚠️ Non sono tre booleani indipendenti: `Action.Sprint` dichiara `MovementAndMain`
	 * e ne occupa due. Chi lo decide e' il catalogo, non il widget.
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
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HUD")
	void SetAction(const FRTAbilityCooldownView& InAction, bool bInArmed);

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
