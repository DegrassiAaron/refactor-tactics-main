#pragma once

#include "CoreMinimal.h"

/**
 * **Come si vede un piano rispetto a quello attivo** ([D-501], #3679).
 *
 * I piani sopra l'attivo sono traslucidi, quelli sotto desaturati e scuriti di piu' quanto piu' sono lontani,
 * l'attivo ha i colori pieni e il bordo delle celle marcato. Una mappa a un piano non ha trattamento.
 *
 * 🔑 **Si attenua l'USCITA del materiale, non la si sostituisce** (#3679, scope 3): il trattamento non tocca i
 * canali 0..2 del colore — che sono del velo ([D-227]) e del terreno — ma ne aggiunge tre che `M_HexCell` applica
 * dopo di loro. Quando arriveranno i terreni con texture, il materiale li desaturera' e scurira' allo stesso modo.
 *
 * 🔴 **La codifica e' NEUTRA A ZERO, e non e' un dettaglio.** Un'istanza nasce con i custom data a zero: con
 * un'«opacita'» nel canale 3, ogni istanza non ancora scritta — l'editor, una board appena ricostruita, una famiglia
 * dimenticata — sarebbe invisibile. Con una «trasparenza» resta com'era.
 *
 *   | canale | nome            | 0 vuol dire     | in `M_HexCell`                          |
 *   |--------|-----------------|-----------------|-----------------------------------------|
 *   | 3      | `Transparency`  | opaco           | `OpacityMask = Dither(1 - x)`, `Masked` |
 *   | 4      | `Desaturation`  | colori pieni    | `Desaturation(Emissive, x)`             |
 *   | 5      | `ExposureStops` | luce invariata  | `Emissive * 2^x`                        |
 */
struct REFACTORTACTICS_API FRTLayerTreatment
{
	float Transparency = 0.f;
	float Desaturation = 0.f;
	float ExposureStops = 0.f;

	/** L'esposizione dei BORDI delle celle (`CellBorders`), che sul piano attivo si marcano. */
	float BorderExposureStops = 0.f;

	bool IsNeutral() const
	{
		return Transparency == 0.f && Desaturation == 0.f && ExposureStops == 0.f && BorderExposureStops == 0.f;
	}
};

/**
 * Le intensita' del trattamento. ⚠️ **I default sono PROVVISORI**: li ha scelti la PR di #3679 senza l'autore, e si
 * scelgono in PIE con le leve `rt.Map.Layers.*` (#3679, scope 7), come la separazione dei piani.
 */
struct REFACTORTACTICS_API FRTLayerPresentationTuning
{
	/** Quanto si vede un piano sopra l'attivo: 1 = pieno, 0 = invisibile. */
	float UpperOpacity = 0.35f;

	/** La saturazione persa per ogni piano di distanza sotto l'attivo, cumulata: 0,5 vuol dire 50%, poi 75%. */
	float LowerDesaturationPerLayer = 0.5f;

	/** La luce persa per ogni piano di distanza sotto l'attivo, cumulata: 0,35 vuol dire ×0,65, poi ×0,42. */
	float LowerDarkeningPerLayer = 0.35f;

	/**
	 * Il bordo del piano attivo, in stop: +6 moltiplica per 64. Il bordo e' scuro (`CellBorderColor`, sRGB 25) sul
	 * pavimento grigio (sRGB 160): a +6 diventa chiaro (sRGB ~206), cioe' il piano attivo ha la griglia chiara e gli
	 * altri quella scura.
	 */
	float ActiveBorderStops = 6.f;

	bool operator==(const FRTLayerPresentationTuning& Other) const
	{
		return UpperOpacity == Other.UpperOpacity && LowerDesaturationPerLayer == Other.LowerDesaturationPerLayer
			&& LowerDarkeningPerLayer == Other.LowerDarkeningPerLayer && ActiveBorderStops == Other.ActiveBorderStops;
	}

	/** I default, con le leve in console che li sostituiscono quando sono impostate (>= 0). */
	static FRTLayerPresentationTuning FromConsole();
};

namespace RTLayerPresentation
{
	/** Il primo canale dei custom data per istanza che il trattamento scrive: 0..2 sono il colore. */
	inline constexpr int32 FirstChannel = 3;

	/** I float per istanza delle famiglie che lo ricevono: colore (3) piu' trattamento (3). */
	inline constexpr int32 NumCustomDataFloats = 6;

	/**
	 * Il trattamento di un piano. Pura: la board la chiede per ogni istanza, e i test la misurano senza mondo.
	 *
	 * @param bMultiLayer falso su una mappa a un piano: nessun trattamento, nemmeno il bordo (#3679, scope 6).
	 */
	REFACTORTACTICS_API FRTLayerTreatment Treatment(int32 Layer, int32 ActiveLayer, bool bMultiLayer,
		const FRTLayerPresentationTuning& Tuning);
}
