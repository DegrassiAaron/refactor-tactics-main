#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/RTHudViewModel.h" // FRTUnitOverlayView: la vista arriva gia' composta, il widget non la calcola
#include "RTUnitOverlayWidget.generated.h"

/**
 * La sovrapposizione sopra un'unita': nome, vita, scudo, energia e stati (`#2288`, `D-320`).
 *
 * 🔴 **Il widget DISEGNA e non decide.** Riceve una `FRTUnitOverlayView` gia' composta da
 * `URTHudViewModel::BuildUnitOverlay` — che a sua volta unisce due produttori testati headless — e la posa.
 * Nessun `HasStatus`, nessun ordinamento, nessuna soglia: ogni `if` scritto in un Blueprint e' un `if` che
 * nessun test vede, ed e' la ragione per cui questa classe esiste in C++ invece di essere un `UserWidget`
 * nudo.
 *
 * ## Come si usa
 *
 * `WBP_RT_UnitOverlay` eredita da qui e implementa `OnOverlayUpdated`, dove lega i campi di `View` ai propri
 * elementi. Il C++ non conosce quegli elementi e non deve: il **contratto e' il dato**, non un elenco di
 * `BindWidget` che legherebbe la classe alla forma dell'asset.
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
	 * Aggiorna la vista e notifica il Blueprint. Chiamata dal driver una volta per fotogramma.
	 *
	 * ⚠️ **Notifica SEMPRE, anche se la vista non e' cambiata**, e non e' una svista: confrontare due
	 * `FRTUnitOverlayView` richiederebbe un `operator==` su una struct che cresce, e il primo campo
	 * dimenticato sarebbe un'icona che non si aggiorna piu' — un difetto silenzioso, contro un costo che a
	 * quattro unita' non si misura. Se un giorno il costo contasse, la porta da cambiare e' questa e non
	 * il Blueprint.
	 */
	void SetOverlayView(const FRTUnitOverlayView& InView);

protected:
	/**
	 * Il Blueprint disegna qui, leggendo `View`.
	 *
	 * 🔑 `BlueprintImplementableEvent` e non `Native`: se `WBP_RT_UnitOverlay` non lo implementa **non
	 * succede nulla**, e il gioco resta identico — la stessa forma delle cue di `D-278`, con lo stesso
	 * limite dichiarato: il C++ non puo' accorgersi che l'asset non disegna. Lo vede solo il PIE.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "RefactorTactics|HUD")
	void OnOverlayUpdated();
};
