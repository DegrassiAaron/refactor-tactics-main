#pragma once

#include "CoreMinimal.h"

/**
 * **Come si vede un piano rispetto a quello attivo** ([D-501], #3679), **e come lo dice il suo bordo** ([D-504], #3704).
 *
 * I piani sopra l'attivo sono traslucidi, quelli sotto desaturati e scuriti di piu' quanto piu' sono lontani, l'attivo
 * ha i colori pieni. Il bordo dice il piano: nero sull'attivo, bianco sopra, viola sotto (`BorderColorFor`). Una mappa
 * a un piano non ha trattamento, e i suoi bordi sono neri come prima.
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
 *
 * ⛔ **I bordi non ricevono il trattamento** ([D-504]): il loro colore DICE la relazione, e desaturarlo o renderlo
 * traslucido la cancellerebbe. Un piano sopra resta un riempimento traslucido con un contorno bianco netto.
 */
struct REFACTORTACTICS_API FRTLayerTreatment
{
	float Transparency = 0.f;
	float Desaturation = 0.f;
	float ExposureStops = 0.f;

	bool IsNeutral() const
	{
		return Transparency == 0.f && Desaturation == 0.f && ExposureStops == 0.f;
	}
};

/** Dove sta un piano rispetto a quello attivo. Su una mappa a un piano, tutto e' `Active`. */
enum class ERTLayerRelation : uint8
{
	Active,
	Above,
	Below,
};

/**
 * Le intensita' del trattamento, con le leve `rt.Map.Layers.*` che le sostituiscono in PIE.
 *
 * 🔑 **I valori «sotto» li ha scelti l'autore** nella seduta `U84` (2026-10-12): desaturazione 0,7 e scurimento 0,55
 * per piano. ⚠️ **L'opacita' «sopra» e' ancora provvisoria**: 0,2 era troppo leggero, e si rigiudica coi bordi bianchi
 * di [D-504].
 */
struct REFACTORTACTICS_API FRTLayerPresentationTuning
{
	/** Quanto si vede un piano sopra l'attivo: 1 = pieno, 0 = invisibile. */
	float UpperOpacity = 0.35f;

	/** La saturazione persa per ogni piano di distanza sotto l'attivo, cumulata: 0,7 vuol dire 70%, poi 91%. */
	float LowerDesaturationPerLayer = 0.7f;

	/** La luce persa per ogni piano di distanza sotto l'attivo, cumulata: 0,55 vuol dire ×0,45, poi ×0,2. */
	float LowerDarkeningPerLayer = 0.55f;

	bool operator==(const FRTLayerPresentationTuning& Other) const
	{
		return UpperOpacity == Other.UpperOpacity && LowerDesaturationPerLayer == Other.LowerDesaturationPerLayer
			&& LowerDarkeningPerLayer == Other.LowerDarkeningPerLayer;
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

	/** La relazione di `Layer` con `ActiveLayer`. Su una mappa a un piano risponde sempre `Active`. */
	REFACTORTACTICS_API ERTLayerRelation Relation(int32 Layer, int32 ActiveLayer, bool bMultiLayer);

	/**
	 * Il trattamento di un piano. Pura: la board la chiede per ogni istanza, e i test la misurano senza mondo.
	 *
	 * @param bMultiLayer falso su una mappa a un piano: nessun trattamento (#3679, scope 6).
	 */
	REFACTORTACTICS_API FRTLayerTreatment Treatment(int32 Layer, int32 ActiveLayer, bool bMultiLayer,
		const FRTLayerPresentationTuning& Tuning);

	/**
	 * **Il colore del bordo che dice il piano** ([D-504]), in sRGB come la palette da cui viene.
	 *
	 *   | relazione | colore                          | in scala di grigi (§47-bis.1) |
	 *   |-----------|---------------------------------|-------------------------------|
	 *   | `Active`  | `URTHexLibrary::CellBorderColor` | scuro (25)                    |
	 *   | `Above`   | §32 `RT_UI_White`               | chiaro (255)                  |
	 *   | `Below`   | §32 `RT_UI_Violet`              | medio (circa 120)             |
	 *
	 * ⚠️ **Il nero e' il bordo di sempre**, non un colore nuovo: su una mappa a un piano la griglia resta com'era.
	 */
	REFACTORTACTICS_API FColor BorderColorFor(ERTLayerRelation InRelation);
}
