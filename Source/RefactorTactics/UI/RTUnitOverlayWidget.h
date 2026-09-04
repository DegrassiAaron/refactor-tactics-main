#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/RTHudViewModel.h" // FRTUnitOverlayView: la vista arriva gia' composta, il widget non la calcola
#include "RTUnitOverlayWidget.generated.h"

class UHorizontalBox;
class UProgressBar;
class UTextBlock;

/**
 * La sovrapposizione sopra un'unita': nome, vita, scudo, energia e stati (`#2288`, `D-320`).
 *
 * 🔴 **Il widget DISEGNA e non decide, e il disegno sta in C++.** Riceve una `FRTUnitOverlayView` gia'
 * composta da `URTHudViewModel::BuildUnitOverlay` — che unisce due produttori testati headless — e la posa
 * sugli elementi legati con `BindWidget`. Nessun `HasStatus`, nessun ordinamento, nessuna soglia.
 *
 * 🔑 **Perche' `BindWidget` e non un `BlueprintImplementableEvent` che lascia disegnare al grafo.** La
 * prima stesura di questa classe faceva cosi', ed era la stessa scelta che tutto `#2274` esiste per
 * evitare: ogni `if` scritto in un grafo Blueprint e' un `if` che **nessun test vede**. Con `BindWidget`
 * l'asset porta il **layout** — dove stanno gli elementi, che aspetto hanno — e il C++ porta **cosa ci
 * finisce dentro. Il confine e' quello di `AGENTS.md` §3: *«Data Asset/Blueprint configurano varianti e
 * presentazione»*, non le decidono.
 *
 * ⚠️ **Tutti i binding sono `Optional`, deliberatamente**: un `WBP` a cui manca un elemento **degrada** —
 * mostra il resto — invece di rifiutarsi di compilare. Una sovrapposizione senza barra dell'energia e' un
 * difetto visibile e riparabile; un widget che non compila fa sparire nome, vita e stati insieme.
 *
 * ⚠️ **Chi decide SE mostrarla non e' questo widget**: lo dice `ARTUnit::IsKnownToObserver()`, scritto dal
 * velo di conoscenza (`#2246`). Il componente che ospita questo widget si accende e si spegne con quello.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class REFACTORTACTICS_API URTUnitOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * L'ultima vista ricevuta. `BlueprintReadOnly` perche' il disegno la **legge**: un widget che potesse
	 * scriverla diventerebbe una seconda fonte di cio' che l'unita' e'.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	FRTUnitOverlayView View;

	/**
	 * Aggiorna la vista e ridisegna. Chiamata dal driver una volta per fotogramma.
	 *
	 * ⚠️ **Ridisegna SEMPRE, anche se la vista non e' cambiata**, e non e' una svista: confrontare due
	 * `FRTUnitOverlayView` richiederebbe un `operator==` su una struct che cresce, e il primo campo
	 * dimenticato sarebbe un'icona che non si aggiorna piu' — un difetto silenzioso, contro un costo che a
	 * quattro unita' non si misura.
	 */
	void SetOverlayView(const FRTUnitOverlayView& InView);

	/**
	 * Il testo che la barra della vita mostra, come funzione PURA dei due numeri.
	 *
	 * 🔑 Statica e testabile: e' l'unica regola di formato che questa classe possiede, e sta qui invece che
	 * dentro `NativeUpdate` perche' un test non debba costruire un widget per verificarla.
	 *
	 * ⚠️ `MaxHealth <= 0` non e' «morto», e' «nessuna unita'»: risponde vuoto invece di `0/0`.
	 */
	static FString ComposeHealthLabel(int32 Health, int32 MaxHealth);

	/**
	 * La frazione per una barra, sempre in `[0,1]`.
	 *
	 * ⚠️ Un massimo non positivo da' `0` e non una divisione per zero: e' il caso dell'unita' non
	 * configurata, non un errore da segnalare.
	 */
	static float SafeFraction(int32 Value, int32 Max);

protected:
	/** Il nome dell'eroe. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	/** La vita, come frazione. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthBar;

	/** La vita, come numeri (`60/100`). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HealthText;

	/** Lo scudo, come frazione di `MaxHealth` — la stessa scala che usava il canvas. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ShieldBar;

	/** L'energia, come frazione di `MaxEnergy`. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> EnergyBar;

	/**
	 * Il contenitore delle icone di stato: il C++ lo **svuota e lo riempie** a ogni aggiornamento.
	 *
	 * ⚠️ Ricostruito e non riciclato: gli stati cambiano di numero e di ordine, e tenere un pool
	 * significherebbe una seconda struttura da mantenere allineata a `View.Statuses` — cioe' un secondo
	 * posto dove sbagliare, per un risparmio che a dieci icone non si misura.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> StatusBox;

	/**
	 * Chiamato dopo che il C++ ha posato la vista, per cio' che il **layout** vuole aggiungere.
	 *
	 * 🔑 Esiste ancora, ma non e' piu' il canale del disegno: e' l'estensione. Un `WBP` che non lo
	 * implementa e' completo — a differenza della prima stesura, dove non implementarlo significava non
	 * disegnare niente.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "RefactorTactics|HUD")
	void OnOverlayUpdated();
};
