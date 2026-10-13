#pragma once

#include "CoreMinimal.h"

/**
 * **Come si vede un piano rispetto a quello attivo** ([D-501], #3679), **e come lo dice il suo bordo** ([D-508], #3704).
 *
 * I piani sopra l'attivo sono traslucidi e schiariti (#3674), quelli sotto desaturati e scuriti di piu' quanto piu'
 * sono lontani, l'attivo ha i colori pieni. Il bordo dice il piano: nero sull'attivo, bianco sopra, viola sotto
 * (`BorderColorFor`). Una mappa a un piano non ha trattamento, e i suoi bordi sono neri come prima.
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
 * ⛔ **I bordi non ricevono il trattamento** ([D-508]): il loro colore DICE la relazione, e desaturarlo o renderlo
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

/**
 * **La tinta con cui un VOLUME dice il suo piano** ([D-508], #3706): muri, coperture, porte, blocchi e corpi.
 *
 * Il colore e' quello del bordo della relazione (`BorderColorFor`), in lineare; la forza dice quanto il volume lo
 * prende, e a zero resta del colore suo. Su una mappa a un piano la forza e' zero: nessuna tinta, come prima.
 *
 * 🔑 **La tinta sta FRA la desaturazione e la luce**, nel materiale. Dopo la desaturazione, perche' un volume sotto
 * e' desaturato e una tinta messa prima perderebbe il suo viola: e' la ragione per cui i bordi delle celle non
 * ricevono il trattamento, il colore DICE la relazione. Prima della luce, perche' lo scurimento di un piano sotto
 * deve valere per tutto il volume, tinta compresa: messo dopo, il viola dominava un corpo scurito e lo rendeva
 * PIU' chiaro di un volume non tinto — il contrario di «sotto» (misurato a schermo il 2026-10-13).
 */
struct REFACTORTACTICS_API FRTLayerVolumeTint
{
	FLinearColor Color = FLinearColor::Black;
	float Strength = 0.f;

	bool IsNone() const { return Strength == 0.f; }
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
 * per piano. **L'opacita' «sopra», 0,35, l'ha scelta lo stesso giorno su piani davvero traslucidi**: prima la maschera
 * di `M_HexCell` era inerte (#3704), i piani sopra erano opachi a ogni valore e 0,2 non era mai stato visto.
 * **Lo schiarimento «sopra», 3 stop, l'ha scelto la sera stessa** su `Visual.Map.MultiLevel` (#3674), dopo aver
 * provato 1,5 e 4. A zero il suo giudizio era stato: «No, si sovrappongono e sembrano sparire».
 */
struct REFACTORTACTICS_API FRTLayerPresentationTuning
{
	/** Quanto si vede un piano sopra l'attivo: 1 = pieno, 0 = invisibile. */
	float UpperOpacity = 0.35f;

	/**
	 * Quanto si SCHIARISCE un piano sopra l'attivo, in stop: +1 raddoppia la luce. Zero = il colore del terreno com'e'.
	 *
	 * ➕ #3674: un velo dello stesso grigio del pavimento che copre non si vede, a nessuna opacita' — 35% di grigio sopra
	 * 65% di grigio e' grigio. Schiarito, il velo ha un colore suo e la piattaforma torna a leggersi come un piano.
	 * A 3 stop la luce e' ×8: il grigio del terreno va verso il bianco, ed e' quello che lo stacca dal pavimento.
	 */
	float UpperExposureStops = 3.f;

	/** La saturazione persa per ogni piano di distanza sotto l'attivo, cumulata: 0,7 vuol dire 70%, poi 91%. */
	float LowerDesaturationPerLayer = 0.7f;

	/** La luce persa per ogni piano di distanza sotto l'attivo, cumulata: 0,55 vuol dire ×0,45, poi ×0,2. */
	float LowerDarkeningPerLayer = 0.55f;

	/**
	 * Quanto un volume prende il colore del suo piano ([D-508], #3706): 0 = del colore suo, 1 = tutto tinta.
	 *
	 * Scelta dall'autore il 2026-10-13, sulle tavole della seduta `U86`, fra 0, 0,25 e 0,6: la PR di #3706 l'aveva
	 * messa a 0,25 senza di lui, e 0,25 resta. La leva e' `rt.Map.Layers.VolumeTint`.
	 */
	float VolumeTintStrength = 0.25f;

	bool operator==(const FRTLayerPresentationTuning& Other) const
	{
		return UpperOpacity == Other.UpperOpacity && UpperExposureStops == Other.UpperExposureStops
			&& LowerDesaturationPerLayer == Other.LowerDesaturationPerLayer
			&& LowerDarkeningPerLayer == Other.LowerDarkeningPerLayer
			&& VolumeTintStrength == Other.VolumeTintStrength;
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
	 * I float per istanza delle famiglie dei VOLUMI (#3706), e come `M_HexVolume` li legge:
	 *
	 *   | canali | che cosa                | 0 vuol dire      |
	 *   |--------|-------------------------|------------------|
	 *   | 0..2   | colore della tinta      | (con forza zero) |
	 *   | 3..5   | il trattamento di D-501 | come le celle    |
	 *   | 6      | forza della tinta       | nessuna tinta    |
	 *
	 * 🔴 Neutra a zero come quella delle celle: un'istanza appena nata e' opaca, del colore suo, senza tinta.
	 * I canali 3..5 sono GLI STESSI delle celle, cosi' il trattamento si scrive con una sola procedura.
	 */
	inline constexpr int32 VolumeNumCustomDataFloats = 7;
	inline constexpr int32 VolumeTintStrengthChannel = 6;

	/** `M_HexVolume`, costruito dal commandlet `RTBuildHexVolumeMaterial` (`D-229`), e il suo colore proprio. */
	REFACTORTACTICS_API extern const TCHAR* const VolumeMaterialPackage;
	REFACTORTACTICS_API extern const TCHAR* const VolumeMaterialName;
	REFACTORTACTICS_API extern const FName VolumeParamOwnColor;
	REFACTORTACTICS_API extern const FName VolumeParamRoughness;

	/**
	 * Il colore PROPRIO di un volume, cioe' cio' che si vede dove la tinta non arriva: in lineare, e neutro.
	 *
	 * 🔑 **Due grigi, e non sono una scelta estetica: sono cio' che c'era.** Fino a #3706 queste famiglie avevano il
	 * materiale della loro mesh — quello di default del motore sui prismi (`Blockers`, `StructuralBodies`,
	 * `SurfaceVolumes`), quello delle forme base sul cubo dei pannelli (`EdgeFeatures`). I valori sono tarati perche'
	 * una mappa a un piano si veda come prima (#3706, scope 3): si MISURANO sulle catture, non si scelgono.
	 *
	 * ⚠️ **Misurati in due passate, il 2026-10-13**, su `Visual.Map.BlockVolumes` e `Visual.Map.GrayKitYard`: media
	 * sRGB di una zona, stessa camera. Con un valore solo (0,13) i blocchi tornavano com'erano (66/74/69 contro
	 * 64/77/73) e i pannelli no (150 contro 128): i due materiali di partenza non avevano lo stesso colore.
	 * ⌫ *Restano due differenze che un grigio piatto non puo' togliere*: la trama a griglia del materiale di default,
	 * e la sua leggera dominante calda.
	 */
	REFACTORTACTICS_API FLinearColor VolumeOwnColor(bool bEdgePanel);

	/** La ruvidita' del materiale dei volumi: opaco, senza riflessi che una tinta leggera non reggerebbe. */
	inline constexpr float VolumeRoughness = 0.85f;

	/**
	 * La tinta di un volume sul piano `Layer`. Pura: e' la funzione della DoD di #3706.
	 *
	 * @param bMultiLayer falso su una mappa a un piano: nessuna tinta (#3706, scope 3).
	 */
	REFACTORTACTICS_API FRTLayerVolumeTint VolumeTint(int32 Layer, int32 ActiveLayer, bool bMultiLayer,
		const FRTLayerPresentationTuning& Tuning);

	/** Il tetto dello schiarimento di un piano sopra (#3674): +4 stop e' luce ×16, oltre il bianco per ogni terreno. */
	inline constexpr float MaxUpperExposureStops = 4.f;

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
	 * **Il colore del bordo che dice il piano** ([D-508]), in sRGB come la palette da cui viene.
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
