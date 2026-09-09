#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTOverlayArea.h"
#include "RTOverlayPalette.generated.h"

/**
 * **La sede unica** della grammatica visiva degli overlay semantici (OVL-01, #1941).
 *
 * 🔑 **Il difetto che questa classe chiude non e' «manca un overlay»: e' che i significati non avevano una
 * sede.** Colore, scala concentrica e ordine di disegno vivevano dentro `ARTHexMapActor::DrawPlanningPreview`
 * — i primi due come letterali, il terzo come **commento**:
 *
 * > *«Ordine di disegno: dal meno al piu' urgente … 1) dove POSSO andare 2) dove VADO 3) chi COLPISCO
 * > 4) cosa sto indicando.»*
 *
 * Un ordine scritto in un commento non e' un dato: nessuno lo puo' leggere, nessun test lo puo' verificare, e
 * il prossimo significato lo cambia spostando una riga. Qui e' `PriorityFor`.
 *
 * ⛔ **I VALORI sono quelli gia' in produzione, e questo e' deliberato.** La spec v0.2 propone un'altra
 * palette (`#35C759` · `#32ADE6` · `#AF52DE` · `#FF453A` · `#FF9F0A` · `#FFD60A` · `#8E8E93`), ma #1941
 * dichiara che le collisioni *«non [vanno] risolte in silenzio nel codice»* e il maintainer ha ribadito il
 * 2026-09-02 che la collisione ciano *«resta interamente vostra da decidere»*. ∴ questa classe **consolida
 * cio' che esiste** e non muove un pixel: la migrazione ai valori v0.2 appartiene alla decisione di palette,
 * non a chi costruisce la sede.
 *
 * 🔴 **Due collisioni MISURATE sulla palette spedita, che nessun documento della famiglia registrava.** Con la
 * formula e la soglia gia' in uso in `RefactorTactics.Hex.SurfaceColorsAreDistinguishable` (Manhattan RGB,
 * `>= 60`):
 *
 * | Coppia | Distanza | Perche' conta |
 * |---|---|---|
 * | `Attack (230,60,50)` vs `URTHexLibrary::BlockedCellColor() (230,40,40)` | **30** | due rossi, entrambi «pericolo», a un terzo della soglia |
 * | `FriendlyFire (255,150,30)` vs superficie `Fire (255,130,40)` | **30** | l'avviso di fuoco amico su una cella di fuoco |
 *
 * ⚠️ La seconda e' peggio di quanto sembri: l'arancione del fuoco amico ha una voce PIE **verde firmata**
 * (`PIE-PREVIEW-AREA`, 2026-08-09, *«si capisce»*) — ma quella verifica non puo' essere passata su una cella
 * di fuoco, perche' li' i due arancioni distano `30`. Le due coppie sono **pinnate** da
 * `RefactorTactics.AreaOverlay.PaletteRatchet`, che le conosce e vieta che se ne aggiungano altre.
 */
UCLASS()
class REFACTORTACTICS_API URTOverlayPalette : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Il colore di un significato. **Non dipende da `Source`** (invariante di #1941): due unita' diverse con
	 * lo stesso `Meaning` producono lo stesso colore, e non c'e' un parametro con cui violarlo.
	 */
	UFUNCTION(BlueprintPure, Category = "Overlay|Palette")
	static FColor ColorFor(ERTOverlayMeaning Meaning);

	/**
	 * L'opacita' della certezza.
	 *
	 * ⚠️ **Dichiarata qui, non ancora disegnata da nessuno — e la ragione e' misurata.** Il renderer di oggi
	 * usa `DrawDebugLine`, che **ignora l'alpha**; la certezza a schermo passa oggi dal **tratteggio**
	 * (`RTHexMapActor.cpp`, origine confermata vs prevista). Il consumatore dell'alpha e' l'*area fill*
	 * aggregata, che e' scope di #1944. Sta qui perche' il modello di #1941 la richiede e perche'
	 * `CertaintyChangesOnlyOpacity` la possa verificare — non perche' qualcuno la disegni adesso.
	 */
	UFUNCTION(BlueprintPure, Category = "Overlay|Palette")
	static uint8 AlphaFor(ERTOverlayCertainty Certainty);

	/** Colore e opacita' insieme: l'RGB viene dal significato, l'alpha dalla certezza. Mai il contrario. */
	UFUNCTION(BlueprintPure, Category = "Overlay|Palette")
	static FColor ColorForArea(ERTOverlayMeaning Meaning, ERTOverlayCertainty Certainty);

	/**
	 * L'ordine di disegno, dal meno al piu' urgente. Era un commento; ora e' un dato, e a passi di dieci
	 * perche' un significato nuovo si possa infilare senza rinumerare gli altri.
	 */
	UFUNCTION(BlueprintPure, Category = "Overlay|Palette")
	static int32 PriorityFor(ERTOverlayMeaning Meaning);

	/** La scala concentrica del contorno: la grammatica con cui oggi si risolve la sovrapposizione. */
	UFUNCTION(BlueprintPure, Category = "Overlay|Palette")
	static float ScaleFor(ERTOverlayMeaning Meaning);

	/**
	 * Se il contorno va disegnato in `SDPG_Foreground`.
	 *
	 * Serve ai significati che stanno su celle **occupabili**: da camera dall'alto il cilindro di chi sta
	 * sulla cella copre il contorno per intero. Non si applica a Movement e PathTrace, che per definizione
	 * stanno su celle vuote — metterli in foreground li farebbe vedere attraverso il terreno.
	 */
	UFUNCTION(BlueprintPure, Category = "Overlay|Palette")
	static bool DrawsThroughUnits(ERTOverlayMeaning Meaning);

	/** Tutti i significati, per i test che devono coprirli senza che l'enum dichiari `TEnumRange`. */
	static TArray<ERTOverlayMeaning> AllMeanings();

	/** Tutte le certezze, stessa ragione. */
	static TArray<ERTOverlayCertainty> AllCertainties();
};
