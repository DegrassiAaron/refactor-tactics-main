#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/RTHeroProfileView.h"
#include "RTHeroRadarWidget.generated.h"

/**
 * Il radar tattico: disegna gli assi che riceve, **quanti che siano**.
 *
 * ⚠️ **Non e' un esagono.** Il dataset corrente ha sei assi, ma il numero e' un fatto del dato, non del
 * widget: `RefactorTactics.HeroProfile.RadarSupportsVariableAxisCount` prova 3, 5, 6 e 8. Cablare sei
 * significherebbe che il primo profilo con un asse in piu' o in meno verrebbe disegnato sbagliato senza
 * che nessuno lo veda.
 *
 * 🔴 **Il centro e' `0`, non `1`.** I rating pubblicati oggi partono da `1`, ma trattare `1` come centro
 * renderebbe indistinguibili «minimo» e «assente» e cambierebbe la sagoma di ogni eroe. Il renderer mappa
 * `Value/MaxValue` sull'intervallo pieno, e lo `0` ha il posto che gli spetta.
 *
 * 🔵 **La geometria e' presentazione, non simulazione.** Questi `float` non entrano in `TurnLog`, non
 * toccano lo `StateHash` e non sono visti da nessun resolver: sono pixel. La parte matematica sta in
 * funzioni statiche pure proprio per poter essere verificata senza costruire un widget, senza un mondo e
 * senza PIE.
 *
 * ⛔ **Nessuna formula di rating vive qui.** Il widget riceve `Value` gia' calcolato da chi lo possiede
 * (`tools/radar/profile.ts`, D-107): non normalizza rubriche, non pesa abilita', non conosce eroi.
 */
UCLASS(BlueprintType, Blueprintable)
class REFACTORTACTICS_API URTHeroRadarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ------------------------------------------------------------------------------------------------
	// Geometria pura — statica, senza stato, testabile a freddo.
	// ------------------------------------------------------------------------------------------------

	/**
	 * L'angolo dell'asse `AxisIndex` su `AxisCount` assi, in gradi.
	 *
	 * `-90°` per il primo — cioe' **verso l'alto** in coordinate schermo, dove la Y cresce verso il basso —
	 * poi in senso orario a passi uguali. ⚠️ L'ordine e' quello dell'array: non si ordina per etichetta ne'
	 * per `FName`, o lo stesso eroe avrebbe due sagome diverse.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HeroProfile|Radar")
	static float ComputeAxisAngleDegrees(int32 AxisIndex, int32 AxisCount);

	/**
	 * La frazione di raggio occupata da un asse: `clamp(Value / MaxValue, 0, 1)`.
	 *
	 * @return `-1` quando l'asse non e' rappresentabile (`MaxValue <= 0`). ⚠️ **Non `0`**: zero e' un
	 *         valore legittimo che significa «al centro», e confondere «minimo misurato» con «dato rotto»
	 *         e' esattamente l'errore che disegnerebbe una debolezza mai dichiarata.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HeroProfile|Radar")
	static float ComputeAxisRatio(const FRTProfileRadarAxisView& Axis);

	/**
	 * I vertici del poligono dei valori, uno per asse, nell'ordine ricevuto.
	 *
	 * ⚠️ **Fail-closed**: se un solo asse non e' rappresentabile la funzione svuota `OutPoints` e
	 * restituisce `false`. Disegnare gli assi buoni e omettere gli altri produrrebbe una sagoma che sembra
	 * misurata e non lo e'.
	 *
	 * 🔵 Un array di assi vuoto restituisce `true` con zero punti: «niente da disegnare» non e' un errore.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HeroProfile|Radar")
	static bool ComputeRadarPoints(const TArray<FRTProfileRadarAxisView>& Axes, FVector2D Center, float Radius, TArray<FVector2D>& OutPoints);

	/**
	 * I vertici del bordo esterno (fondoscala) per `AxisCount` assi: la cornice e le tacche della griglia.
	 *
	 * Separata da `ComputeRadarPoints` perche' non dipende dai valori: serve anche per disegnare gli
	 * anelli intermedi, dove il raggio e' una frazione del massimo.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HeroProfile|Radar")
	static void ComputeRingPoints(int32 AxisCount, FVector2D Center, float Radius, TArray<FVector2D>& OutPoints);

	/** Il raggio utile dentro un riquadro: meta' del lato corto, ridotta da `RadiusScale`. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HeroProfile|Radar")
	float ComputeRadiusForSize(FVector2D LocalSize) const;

	// ------------------------------------------------------------------------------------------------
	// Dato
	// ------------------------------------------------------------------------------------------------

	/** Sostituisce gli assi e ridisegna. Chi chiama possiede il dato: il widget non lo corregge. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HeroProfile|Radar")
	void SetRadarAxes(const TArray<FRTProfileRadarAxisView>& InAxes);

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HeroProfile|Radar")
	const TArray<FRTProfileRadarAxisView>& GetRadarAxes() const { return RadarAxes; }

	/**
	 * Vero quando gli assi correnti sono disegnabili. ⚠️ E' la stessa domanda di `ComputeRadarPoints`, ed
	 * esiste perche' un Blueprint possa **nascondere** la sezione invece di mostrare un radar a zero.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HeroProfile|Radar")
	bool HasDrawableAxes() const;

	/** Notifica il Blueprint che gli assi sono cambiati, per riallineare etichette e testi. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RefactorTactics|HeroProfile|Radar")
	void OnRadarAxesChanged();

	// ------------------------------------------------------------------------------------------------
	// Presentazione — colori e spessori, niente che decida COSA si vede.
	// ------------------------------------------------------------------------------------------------

	/** Anelli della griglia, bordo escluso. `0` disegna solo la cornice. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile|Radar", meta = (ClampMin = "0", ClampMax = "10"))
	int32 GridRingCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile|Radar")
	FLinearColor GridColor = FLinearColor(1.f, 1.f, 1.f, 0.25f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile|Radar")
	FLinearColor AxisColor = FLinearColor(1.f, 1.f, 1.f, 0.45f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile|Radar")
	FLinearColor ValueColor = FLinearColor(0.35f, 0.75f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile|Radar", meta = (ClampMin = "0.1"))
	float GridThickness = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile|Radar", meta = (ClampMin = "0.1"))
	float ValueThickness = 2.f;

	/**
	 * Quanto del riquadro occupa il radar. Il default lascia un margine: le etichette degli assi vivono
	 * fuori dal poligono, e un radar a filo del bordo le taglierebbe.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile|Radar", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float RadiusScale = 0.8f;

	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

protected:
	/**
	 * Gli assi da disegnare. `EditAnywhere` perche' un WBP possa avere una **preview di design-time**
	 * senza codice: sono valori di fixture, non un catalogo, e in gioco vengono sempre sostituiti da
	 * `SetRadarAxes`.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile|Radar")
	TArray<FRTProfileRadarAxisView> RadarAxes;
};
