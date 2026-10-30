#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/RTInspect.h"
#include "UI/RTScreenHudWidgets.h"
#include "RTInspectCardWidget.generated.h"

class ARTUnit;
class UPanelWidget;
class UTextBlock;

/**
 * `WBP_RT_InspectLine` - UNA riga di testo della scheda dell'ispezione (`#3748`). Come `URTPlayerEventLineWidget` RICEVE cio' che deve
 * scrivere e non va a prenderlo: la costruisce e la riscrive `URTInspectCardWidget`.
 *
 * ⛔ **Non ha hover.** La scheda non ha piu' righe di skill (il kit di chi e' a fuoco lo mostra la barra, in sola lettura), e il
 * produttore dell'hover su una skill di un'unita' ispezionata e' lo slot di quella barra (`URTActionSlotWidget::HoverBegin`, che passa
 * da `ARTPlayerController::SetSkillHover`). Una riga non chiede niente al controller.
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
	 * Scrive la riga.
	 *
	 * ⛔ **Non e' una `UFUNCTION`**, come `SetCard` e `SetLine` dell'evento: la riga la costruisce e la riscrive la scheda, e una
	 * porta Blueprint ne farebbe rientrare un secondo produttore.
	 */
	void SetLine(const FString& InText);

	const FString& GetLineString() const { return Text; }

private:
	FString Text;

	/** `LineText` ha ricevuto almeno una volta il testo: la prima scrittura non si salta anche se la stringa e' vuota. */
	bool bTextWritten = false;
};

/**
 * `WBP_RT_InspectCard` - la SCHEDA dell'ispezione (`#3748`, fetta F3 di `#705`): il pannello della zona `MiddleRight` che mostra
 * carta, stati, verso ed equipaggiamento di un'unita' ispezionata. ⛔ Non il kit: lo mostra la barra delle skill, in sola lettura.
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

	/**
	 * Il delta di rotella non ancora speso in tacche intere. Una rotella ad alta risoluzione o un touchpad mandano molti eventi
	 * frazionari: contare il solo segno di ciascuno scorrerebbe una tacca intera per evento. Come lo zoom del mondo, e' proporzionale.
	 */
	float WheelRemainder = 0.f;

	float SecondsSinceCompose = 1.f; // il primo tick compone subito
	bool bShown = false;

	/** Vedi `SetViewForTest`. Vuoto in gioco. */
	TOptional<FRTInspectView> ViewForTest;

	/** Vedi `SetContextForTest`. Vuoto in gioco. */
	TOptional<ERTPointerContext> ContextForTest;

	UPROPERTY(Transient)
	TArray<TObjectPtr<URTInspectLineWidget>> LineWidgets;
};
