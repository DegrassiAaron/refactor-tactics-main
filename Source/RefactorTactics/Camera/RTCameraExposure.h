#pragma once

#include "CoreMinimal.h"

struct FPostProcessSettings;

/**
 * **L'esposizione della camera di gioco e' fissa** ([D-501], #3723).
 *
 * 🔑 **Perche'.** D-501 scurisce i piani sotto quello attivo, di piu' quanto piu' sono lontani. Con l'esposizione
 * automatica del motore quello scurimento restava un RAPPORTO dentro il quadro e spariva come LIVELLO: quando gran
 * parte del quadro e' «sotto», l'adattamento alza il guadagno e riporta su tutto. Misurato il 2026-10-13 su
 * `Visual.Map.LayerVolumes`: una cella del piano terra aveva lo stesso grigio (fra 134 e 136 su 255) attiva, un
 * piano sotto e due piani sotto. Con l'adattamento spento scendeva da 170 a 110 a 61, cioe' quanto il trattamento
 * dichiara. E il grigio di una stessa cella dipendeva dall'inquadratura.
 *
 * L'autore ha scelto l'esposizione fissa il 2026-10-13, fra questa, lasciare l'adattamento e limitarlo.
 *
 * 🔑 **Come.** Il metodo e' `AEM_Manual` SENZA la camera fisica: il motore blocca il punto di bianco a EV100 = 0
 * (`CalculateManualAutoExposure`, UE 5.8, `PostProcessEyeAdaptation.cpp`) e applica la sola compensazione,
 * `2^BiasStops`. Con la compensazione a zero la resa e' quella di `ShowFlag.EyeAdaptation 0`. Niente dipende piu'
 * da cio' che il quadro contiene.
 *
 * ⚠️ **Tocca ogni scena, anche a un piano.** Per questo la compensazione di default e' tarata su una scena a un
 * piano: vedi `DefaultBiasStops`.
 */
namespace RTCameraExposure
{
	/** I limiti della compensazione, in stop. Sono anche l'intervallo in cui la leva in console vale. */
	inline constexpr float MinBiasStops = -8.f;
	inline constexpr float MaxBiasStops = 8.f;

	/** Il valore della leva `rt.Camera.FixedExposure.Bias` che vuol dire «usa il default»: sta fuori dai limiti. */
	inline constexpr float UnsetBiasLever = -100.f;

	/**
	 * La compensazione di default, in stop sopra il punto fisso del motore: +1 raddoppia la luce.
	 *
	 * Tarata a schermo il 2026-10-13 perche' le scene si vedano come quando l'esposizione era automatica. Per ogni
	 * vista si e' misurata la compensazione che le rida' il grigio che aveva con l'adattamento, in vista tattica:
	 * +0,10 su `Visual.Map.BlockVolumes` e su `Visual.Map.GrayKitYard`, +0,16 su `Visual.Combat.WaterElectric`,
	 * +0,41 su `Visual.Hud.FirstPlayable`, che usa l'arena della partita normale. In strategica: +0,11 e +0,18.
	 *
	 * Un valore che le rimetta tutte non c'e': l'adattamento dava a ogni scena un'esposizione sua, ed e' il difetto.
	 * 0,25 sta in mezzo: nessuna delle sei viste cambia di piu' di 0,16 stop.
	 *
	 * ⚠️ Scelta della PR di #3723, senza l'autore: la giudica in PIE, con la leva `rt.Camera.FixedExposure.Bias`.
	 */
	inline constexpr float DefaultBiasStops = 0.25f;
}

/**
 * Le leve dell'esposizione, `rt.Camera.FixedExposure*`, da provare in PIE.
 *
 * `rt.Camera.FixedExposure 0` rimette l'adattamento del motore, com'era prima di #3723: e' il confronto prima/dopo.
 */
struct REFACTORTACTICS_API FRTCameraExposureTuning
{
	/** Fissa. Falso: l'adattamento automatico del motore. */
	bool bFixed = true;

	/** La compensazione dell'esposizione fissa, in stop. */
	float BiasStops = RTCameraExposure::DefaultBiasStops;

	bool operator==(const FRTCameraExposureTuning& Other) const
	{
		return bFixed == Other.bFixed && BiasStops == Other.BiasStops;
	}

	/**
	 * I default, con le due leve che li sostituiscono quando sono impostate. Pura: e' cio' che un test misura.
	 *
	 * @param FixedLever  negativo = default; 0 = adattamento del motore; altro = fissa
	 * @param BiasLever   vale solo fra `MinBiasStops` e `MaxBiasStops`; fuori (compreso `UnsetBiasLever`) = default
	 */
	static FRTCameraExposureTuning FromLevers(int32 FixedLever, float BiasLever);

	/** `FromLevers` sulle due leve in console. */
	static FRTCameraExposureTuning FromConsole();
};

namespace RTCameraExposure
{
	/**
	 * Scrive l'esposizione di `Tuning` sulle impostazioni di post-process di una camera.
	 *
	 * Fissa: forza il metodo manuale, la compensazione, e spegne la camera fisica. Non fissa: toglie i tre override e
	 * rimette i valori di default del motore, cosi' la camera torna a cio' che il motore fa da solo.
	 *
	 * ⚠️ Non tocca nessun altro campo: i blendable della camera (il contorno delle unita', #3705) restano dove sono.
	 */
	REFACTORTACTICS_API void Apply(const FRTCameraExposureTuning& Tuning, FPostProcessSettings& Settings);
}
