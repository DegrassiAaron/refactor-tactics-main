#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/RTInspect.h"
#include "UI/RTScreenHudWidgets.h"
#include "RTInspectCardWidget.generated.h"

class ARTPlayerController;
class ARTUnit;
class UPanelWidget;
class UTextBlock;

/**
 * `WBP_RT_InspectLine` - UNA riga della scheda dell'ispezione (`#3748`). Come `URTPlayerEventLineWidget` RICEVE cio' che deve
 * scrivere e non va a prenderlo: la costruisce e la riscrive `URTInspectCardWidget`.
 *
 * Una riga di skill porta la posizione di kit di quella skill e, passandoci sopra col cursore, ne chiede la portata al controller
 * (`SetSkillHover` / `ClearSkillHover`), come lo slot della barra (`URTActionSlotWidget`): la spec vuole lo stesso gesto «nella
 * barra o nella scheda». Le altre righe non chiedono niente.
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTInspectLineWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Il testo della riga. Senza, il widget tiene la stringa e non la disegna. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD")
	TObjectPtr<UTextBlock> LineText;

	/**
	 * Scrive la riga. `InHoverUnit` e `InAbilityIndex` dicono di chi e' la skill: nulla e `INDEX_NONE` per una riga che non lo e'.
	 *
	 * ⛔ **Non e' una `UFUNCTION`**, come `SetCard` e `SetLine` dell'evento: la riga la costruisce e la riscrive la scheda, e una
	 * porta Blueprint ne farebbe rientrare un secondo produttore.
	 *
	 * ⚠️ **Se la riga e' sotto il cursore mentre cambia skill**, l'hover vecchio si spegne e quello nuovo si accende: Slate non manda un
	 * nuovo ingresso a un widget che il cursore non ha lasciato, e la mappa resterebbe sulla portata di una skill che la riga non
	 * mostra piu'.
	 */
	void SetLine(const FString& InText, const ARTUnit* InHoverUnit, int32 InAbilityIndex);

	const FString& GetLineString() const { return Text; }
	int32 GetAbilityIndex() const { return AbilityIndex; }

	/** Le due meta' dell'hover, chiamate da `NativeOnMouseEnter` / `NativeOnMouseLeave` e dai test. */
	void HoverBegin();
	void HoverEnd();

	/**
	 * Inietta il controller a cui l'hover inoltra, senza un `ULocalPlayer`: la stessa forma e la stessa ragione di
	 * `URTActionSlotWidget::SetArmingControllerForTest`. Nullo in gioco: il proprietario vero resta `GetOwningPlayer()`.
	 */
	void SetHoverControllerForTest(ARTPlayerController* InController);

	/** Pubblici e non protetti, come in `URTActionSlotWidget`: i test provano che il passaggio del cursore arriva a `HoverBegin` / `HoverEnd`. */
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

private:
	ARTPlayerController* ResolveHoverController() const;

	FString Text;

	/** Non e' una `UPROPERTY`, come `BuiltForUnit` dello slot: e' un confronto, non un secondo proprietario. */
	TWeakObjectPtr<const ARTUnit> HoverUnit;
	int32 AbilityIndex = INDEX_NONE;

	/**
	 * Cio' che QUESTA riga ha registrato all'ingresso. L'uscita spegne solo quello: la riga puo' essere stata riscritta fra
	 * l'ingresso e l'uscita, e leggere i campi correnti spegnerebbe un hover che non e' suo (lo stesso difetto che
	 * `URTActionSlotWidget` ha gia' pagato).
	 */
	TWeakObjectPtr<const ARTUnit> HoverRegisteredUnit;
	int32 HoverRegisteredIndex = INDEX_NONE;

	UPROPERTY(Transient)
	TWeakObjectPtr<ARTPlayerController> HoverControllerForTest;
};

/**
 * `WBP_RT_InspectCard` - la SCHEDA dell'ispezione (`#3748`, fetta F3 di `#705`): il pannello della zona `MiddleRight` che mostra
 * carta, stati, verso, kit ed equipaggiamento di un'unita' ispezionata.
 *
 * 🔑 **Disegna e non decide.** La scheda arriva composta da `ARTHUD::BuildInspectCard` (cancello di conoscenza, soggetto pubblico,
 * compositore); `URTInspectLibrary::ShouldShowCard` dice se si vede; `URTInspectLibrary::VisibleWindow` dice quali righe entrano. Qui
 * restano la tavola dei widget e lo stato di scorrimento.
 *
 * 🔑 **Nascondere non e' chiudere.** In una finestra di reazione o in un modale il pannello e' `Collapsed` e il soggetto resta
 * fissato; chiude il tasto destro (passo `Inspector` del Back) o la conoscenza (`ReleaseInspectionUnlessLive`), mai il contesto.
 *
 * ⚠️ **Il pannello consuma il tasto sinistro e la rotella, non il destro**: il click sul pannello non raggiunge la mappa e la
 * rotella scorre la scheda (spec §3 regola 6), ma il tasto destro resta del controller, perche' e' quello che chiude la scheda anche
 * col cursore sopra di lei.
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTInspectCardWidget : public URTScreenHudWidgetBase
{
	GENERATED_BODY()

public:
	/** Tutta la scheda: `Collapsed` quando non c'e' niente da mostrare o la scheda e' nascosta da un modale. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD")
	TObjectPtr<class UWidget> CardRoot;

	/** Il pannello delle righe. Senza, o senza `LineClass`, la scheda non costruisce niente. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "RefactorTactics|HUD")
	TObjectPtr<UPanelWidget> LinesBox;

	/** La classe delle righe, nei default del WBP come `LineClass` del registro. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HUD")
	TSubclassOf<URTInspectLineWidget> LineClass;

	/**
	 * Quante righe entrano, marcatori di taglio compresi. Il minimo e' tre: sotto, nemmeno i marcatori entrano e il taglio
	 * tornerebbe muto (`URTInspectLibrary::VisibleWindow`).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HUD", meta = (ClampMin = "3"))
	int32 MaxRighe = 16;

	/** Quante righe scorre una tacca di rotella. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HUD", meta = (ClampMin = "1"))
	int32 LinesPerWheelStep = 3;

	/**
	 * Decide se la scheda si vede, la ricompone se serve e riscrive le righe. La chiama `NativeTick`; i test la chiamano a mano.
	 *
	 * @param bRecompose  ricomporre la scheda dall'attore. `NativeTick` lo fa a 5 Hz, come il registro: la composizione costruisce
	 *                    cataloghi e testi, e a ogni fotogramma sarebbe lavoro per un risultato quasi sempre uguale. Un soggetto
	 *                    nuovo ricompone subito.
	 */
	void RefreshLook(bool bRecompose = true);

	/** Scorre la scheda di `Lines` righe (negative: verso l'alto), dentro i limiti, e riscrive le righe. */
	void ScrollBy(int32 Lines);

	/** La scheda e' a schermo (non nascosta, non vuota). */
	bool IsCardShown() const { return bShown; }

	/** La prima riga mostrata (per i test). */
	int32 GetFirstLine() const { return FirstLine; }

	/** Le righe costruite, nell'ordine della finestra (per i test). */
	const TArray<TObjectPtr<URTInspectLineWidget>>& GetLineWidgets() const { return LineWidgets; }

	/**
	 * Inietta la scheda gia' composta, al posto di quella che verrebbe da `ARTHUD::BuildInspectCard`. In gioco e' vuota. Esiste
	 * perche' la composizione chiede un manager e una vista di conoscenza con il nemico `Live`: la presentazione si prova senza.
	 * Che la composizione sia giusta lo provano i test di `BuildInspectCard`.
	 */
	void SetViewForTest(const FRTInspectView& InView);

	/**
	 * Inietta il contesto del puntatore, al posto di `GetPointerContext()`. Esiste perche' `Modal` in headless richiede
	 * `IsPauseOpen()` del navigatore e `ReactionWindow` non e' ancora prodotto da `GetPointerContext`: la regola «nascondi senza
	 * chiudere» si prova solo cosi'. In gioco e' vuoto.
	 */
	void SetContextForTest(ERTPointerContext InContext) { ContextForTest = InContext; }

	/** Pubblici e non protetti, come in `URTActionSlotWidget`: i test provano che il pannello consuma il tasto sinistro e la rotella. */
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void SetCardShown(bool bInShown);

	/** Riscrive le righe dalla finestra di `CachedView` che parte da `FirstLine`. */
	void ApplyWindow();

	/** Allinea i widget di riga alla finestra: toglie quelle in piu', costruisce quelle che mancano, riscrive tutte. */
	void RebuildLines(const FRTInspectView& Window);

	/** Il tasto sinistro e' quello che sulla mappa seleziona e ispeziona: e' l'unico che il pannello consuma. */
	static bool IsPrimaryButton(const FPointerEvent& Event);

	/** La scheda intera, composta: la finestra ne mostra un pezzo. */
	FRTInspectView CachedView;

	/**
	 * L'unita' di cui `CachedView` e' la scheda: un'altra riparte da capo e ricompone. Debole e non `UPROPERTY`, come `BuiltForUnit` dello
	 * slot: e' un confronto di identita', non un secondo proprietario.
	 */
	TWeakObjectPtr<const ARTUnit> CachedSubject;

	int32 FirstLine = 0;
	float SecondsSinceCompose = 1.f; // il primo tick compone subito
	bool bShown = false;

	/** Vedi `SetViewForTest`. Vuoto in gioco. */
	TOptional<FRTInspectView> ViewForTest;

	/** Vedi `SetContextForTest`. Vuoto in gioco. */
	TOptional<ERTPointerContext> ContextForTest;

	UPROPERTY(Transient)
	TArray<TObjectPtr<URTInspectLineWidget>> LineWidgets;
};
