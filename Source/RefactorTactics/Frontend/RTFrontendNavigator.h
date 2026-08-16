#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Frontend/RTScreenStack.h"
#include "RTFrontendNavigator.generated.h"

class UUserWidget;

// ⚠️ **`FRTScreenBinding` e' stata rimossa in code review**: era una `USTRUCT` con `ScreenId` +
// `WidgetClass` e **zero consumatori** — il navigatore usa `TMap<FName, TSoftClassPtr<UUserWidget>>`.
// Portava anche `EditAnywhere` e un `meta = (AllowAbstract)` che non configuravano niente. Un tipo esposto
// all'editor che nessuno legge e' peggio di un tipo assente: sembra il modo previsto di dichiarare le
// schermate, e non lo e'. Quando servira' un array configurabile da `.ini` o da data asset, si riscrive
// insieme al suo lettore.

/**
 * **L'unico owner del flow del frontend** (CP 46.1, #936).
 *
 * Possiede lo stack logico (`FRTScreenStack`) e ne traduce le transizioni in widget. E' l'unico punto del
 * codebase autorizzato a chiamare `CreateWidget`, `AddToViewport` e `RemoveFromParent` — ed e' il criterio
 * **verificabile** del DoD, non un'intenzione:
 *
 *     grep -rn "AddToViewport\|RemoveFromParent\|CreateWidget" Source/
 *
 * non deve produrre occorrenze fuori da questo file. La baseline era **zero** in tutto `Source/`
 * (misurata su `5530bd03`): questa classe e' la prima a introdurle, e deve restare la sola.
 *
 * ## Perche' un `UGameInstanceSubsystem`
 *
 * Perche' il frontend deve **sopravvivere al cambio di livello**: `Main Menu -> partita -> Main Menu` e' il
 * ciclo di E46, e un owner del flow che muore col livello perderebbe lo stack proprio nel momento in cui
 * serve a tornare indietro. Un `AHUD` o un `APlayerController` non vanno bene per la stessa ragione — sono
 * per-livello e per-giocatore, mentre il back stack e' del **gioco**.
 *
 * ## Cosa NON fa
 *
 * - **Non decide la precedenza dell'input.** `ERTPointerContext::Modal` e la precedenza
 *   `Modal/Reaction UI > HUD > world tactical hit` sono di **CP 11.8**, e questo navigatore non li tocca:
 *   e' il vincolo esplicito di #936. Qui si decide *quale schermata e' visibile*, li' *chi consuma un
 *   click in partita*.
 * - **Non contiene regole di gioco.** Nessuna schermata legge o scrive lo stato del resolver.
 */
UCLASS()
class REFACTORTACTICS_API URTFrontendNavigator : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Dichiara la radice e apre la prima schermata. **Va chiamata prima di ogni altra cosa**: senza radice
	 * lo stack non ha uno stato legale, e `ReturnMain` non avrebbe dove tornare.
	 *
	 * ⚠️ **Restituisce l'esito, e la prima stesura era `void`.** Con un `RootScreenId` vuoto il navigatore
	 * restava muto: ogni `PushScreen` successivo rispondeva `Ok` — lo stack si muove — ma nessun widget
	 * compariva mai, perche' `SyncPresentation` esce subito. Un fallimento indistinguibile dal successo e'
	 * esattamente cio' che `ERTNavResult` esiste per impedire. Trovato in code review.
	 */
	UFUNCTION(BlueprintCallable, Category = "Frontend")
	ERTNavResult InitializeFrontend(FName RootScreenId);

	UFUNCTION(BlueprintCallable, Category = "Frontend")
	ERTNavResult PushScreen(FName ScreenId);

	UFUNCTION(BlueprintCallable, Category = "Frontend")
	ERTNavResult PopScreen();

	UFUNCTION(BlueprintCallable, Category = "Frontend")
	ERTNavResult ShowModal(FName ModalId);

	UFUNCTION(BlueprintCallable, Category = "Frontend")
	ERTNavResult CloseModal();

	UFUNCTION(BlueprintCallable, Category = "Frontend")
	ERTNavResult ReturnMain();

	/** La schermata in cima. */
	UFUNCTION(BlueprintPure, Category = "Frontend")
	FName GetCurrentScreen() const { return Stack.CurrentScreen(); }

	/** `false` sulla radice: la UI lo legge per **non disegnare** il pulsante Back invece di disegnarlo inerte. */
	UFUNCTION(BlueprintPure, Category = "Frontend")
	bool CanGoBack() const { return Stack.CanGoBack(); }

	UFUNCTION(BlueprintPure, Category = "Frontend")
	bool IsModalOpen() const { return Stack.IsModalOpen(); }

	/** La schermata sotto riceve input? `false` mentre un modale la copre. */
	UFUNCTION(BlueprintPure, Category = "Frontend")
	bool IsScreenInteractive() const { return Stack.IsScreenInteractive(); }

	UFUNCTION(BlueprintPure, Category = "Frontend")
	int32 GetDepth() const { return Stack.Depth(); }

	/** Lo stack, per i test e per la diagnostica. */
	const FRTScreenStack& GetStack() const { return Stack; }

	/**
	 * Le associazioni nome -> widget. Popolate da configurazione o da Blueprint all'avvio.
	 *
	 * ⚠️ Una schermata **senza binding non e' un errore di navigazione**: lo stack si muove lo stesso e
	 * `GetCurrentScreen()` risponde. Manca solo cosa disegnare, e in un test headless e' il caso normale.
	 * Confondere le due cose legherebbe la logica agli asset, che e' cio' che questa divisione evita.
	 */
	UFUNCTION(BlueprintCallable, Category = "Frontend")
	void RegisterScreen(FName ScreenId, TSoftClassPtr<UUserWidget> WidgetClass);

	/**
	 * Il widget **istanziato** per quella schermata, `nullptr` se non ne e' mai stato creato uno.
	 *
	 * ⚠️ **Non dice se e' a schermo.** La prima stesura di questo commento affermava il contrario
	 * (*«nullptr … se non e' sullo schermo»*) mentre l'implementazione legge solo la mappa — trovato in
	 * code review, e corretto qui invece che nel codice per la ragione sotto.
	 *
	 * I widget **restano istanziati** dopo un `Pop`: `DismissWidget` chiama `RemoveFromParent` e lascia la
	 * voce. E' **cache voluta**, non una perdita: ricrearli a ogni navigazione costerebbe un caricamento
	 * per ogni Back. ⚠️ Ha pero' una conseguenza osservabile che va saputa: una schermata ri-mostrata
	 * conserva scroll, selezione e campi di testo, perche' e' **la stessa istanza**. Se una schermata deve
	 * ripartire pulita, e' lei a doversi azzerare in `NativeConstruct` — il navigatore non lo fa e non deve
	 * indovinarlo.
	 *
	 * Chi vuole sapere se un widget e' visibile chiede `IsInViewport()` al widget.
	 */
	UFUNCTION(BlueprintPure, Category = "Frontend")
	UUserWidget* FindLiveWidget(FName ScreenId) const;

	// ~UGameInstanceSubsystem
	virtual void Deinitialize() override;

private:
	/** Crea (o riusa) il widget di `ScreenId` e lo mette a schermo. No-op senza binding. */
	void PresentWidget(FName ScreenId, int32 ZOrder);

	/** Toglie dallo schermo il widget di `ScreenId`, se c'e'. */
	void DismissWidget(FName ScreenId);

	/** Riallinea cio' che e' a schermo con lo stack: e' l'unico punto in cui la presentazione cambia. */
	void SyncPresentation();

	UPROPERTY()
	FRTScreenStack Stack;

	UPROPERTY()
	TMap<FName, TSoftClassPtr<UUserWidget>> Bindings;

	/** I widget istanziati, per nome. Sopravvivono al pop finche' `SyncPresentation` non li smonta. */
	UPROPERTY()
	TMap<FName, TObjectPtr<UUserWidget>> LiveWidgets;

	/** `true` dopo `InitializeFrontend`: prima, le transizioni non hanno una radice a cui riferirsi. */
	bool bInitialized = false;
};
