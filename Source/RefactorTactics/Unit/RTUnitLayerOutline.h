#pragma once

#include "CoreMinimal.h"
#include "Map/RTLayerPresentation.h"

/**
 * **Il piano di un'unita' si legge dal contorno del modello** ([D-508], #3705).
 *
 * E' la regola dei bordi applicata alle unita': nero sul piano attivo, bianco sopra, viola sotto, lo stesso colore a
 * ogni distanza. La relazione e i colori NON si ridefiniscono qui: vengono da `RTLayerPresentation::Relation` e da
 * `RTLayerPresentation::BorderColorFor`, le stesse che colorano i bordi delle celle.
 *
 * 🔑 **La tecnica: custom depth con un valore di stencil per relazione, e UN post-process che disegna il contorno.**
 * Il corpo mostrato dell'unita' (cilindro, bracci, skeletal) si disegna anche nel custom depth con lo stencil della
 * sua relazione; `M_UnitLayerOutline`, sulla camera di chi guarda, colora i pixel appena FUORI dalla sagoma. Il costo
 * e' un passaggio a schermo intero, qualunque sia il numero di unita' — il vincolo di budget di #3705.
 *
 * 🔴 **Lo stencil `Below` passa attraverso la geometria, gli altri no** (#3705, scope 5). Un'unita' sotto il piano
 * attivo puo' stare sotto una cella di quel piano, che e' piena: il suo contorno si disegna lo stesso, e dice dove
 * sta. Per `Active` e `Above` il materiale confronta la profondita' dell'unita' con quella della scena, e un'unita'
 * dietro un muro non mostra il contorno attraverso di lui. Scartato il guscio invertito per mesh: resta dietro la
 * geometria che copre l'unita', e costa una mesh in piu' per corpo.
 *
 * ⛔ **Privacy: chi non si vede non ha contorno, e nemmeno una quota.** Lo stencil lo decide l'HUD dalla VISTA di chi
 * guarda (`ARTHUD::UnitOutlineStencilForUnit`), e per un nemico il piano e' quello della sua voce di conoscenza, mai
 * la cella dell'attore. `ARTUnit` lo applica solo ai corpi che il velo lascia accesi.
 */
namespace RTUnitLayerOutline
{
	/** Nessun contorno: il corpo non entra nel custom depth. E' anche il default di ogni componente. */
	inline constexpr uint8 StencilNone = 0;
	inline constexpr uint8 StencilActive = 1;
	inline constexpr uint8 StencilAbove = 2;
	inline constexpr uint8 StencilBelow = 3;

	/** Lo stencil di una relazione. Mai `StencilNone`: un'unita' che si vede ha sempre un contorno. */
	REFACTORTACTICS_API uint8 StencilFor(ERTLayerRelation InRelation);

	/**
	 * Lo stencil del contorno di un'unita' sul piano `UnitLayer`. Pura: e' la funzione della DoD di #3705.
	 *
	 * @param bMultiLayer falso su una mappa a un piano: il contorno e' quello dell'attivo, nero, per tutti (scope 4).
	 */
	REFACTORTACTICS_API uint8 StencilForLayer(int32 UnitLayer, int32 ActiveLayer, bool bMultiLayer);

	/** Se il contorno di questo stencil si disegna anche dove la geometria copre l'unita'. Solo `StencilBelow`. */
	REFACTORTACTICS_API bool ShowsThroughGeometry(uint8 Stencil);

	/** Il colore del contorno di uno stencil, da `BorderColorFor`. Falso per `StencilNone` e per i valori non nostri. */
	REFACTORTACTICS_API bool ColorFor(uint8 Stencil, FColor& OutColor);

	/** Il materiale di post-process, costruito dal commandlet `RTBuildUnitOutlineMaterial` (`D-229`). */
	REFACTORTACTICS_API extern const TCHAR* const MaterialPackage;
	REFACTORTACTICS_API extern const TCHAR* const MaterialName;

	/** I parametri che il runtime scrive e che la ricetta dell'asset dichiara: un nome solo per entrambi. */
	REFACTORTACTICS_API extern const FName ParamColorActive;
	REFACTORTACTICS_API extern const FName ParamColorAbove;
	REFACTORTACTICS_API extern const FName ParamColorBelow;
	REFACTORTACTICS_API extern const FName ParamThickness;

	/** Il parametro di colore di uno stencil, `NAME_None` per chi non ne ha. */
	REFACTORTACTICS_API FName ColorParamFor(uint8 Stencil);

	/** Lo spessore di default del contorno, in pixel a 1080 righe; il materiale lo scala con l'altezza del quadro. */
	inline constexpr float DefaultThicknessPx = 2.f;
	inline constexpr float MaxThicknessPx = 8.f;
}

/**
 * Le leve del contorno, `rt.Unit.LayerOutline*`, da provare in PIE.
 *
 * ⚠️ **Provvisorie, scelte senza l'autore** (PR di #3705): lo spessore, e il fatto stesso che il contorno ci sia.
 * La voce PIE le giudica.
 */
struct REFACTORTACTICS_API FRTUnitLayerOutlineTuning
{
	/** Spento: nessun corpo entra nel custom depth e il post-process non pesa. */
	bool bEnabled = true;

	/** Lo spessore del contorno, in pixel a 1080 righe. */
	float ThicknessPx = RTUnitLayerOutline::DefaultThicknessPx;

	bool operator==(const FRTUnitLayerOutlineTuning& Other) const
	{
		return bEnabled == Other.bEnabled && ThicknessPx == Other.ThicknessPx;
	}

	/** I default, con le leve in console che li sostituiscono quando sono impostate (>= 0). */
	static FRTUnitLayerOutlineTuning FromConsole();
};
