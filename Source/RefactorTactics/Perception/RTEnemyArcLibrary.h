#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Perception/RTPerceptionLibrary.h" // FRTPerceiver: l'arco e' quello della percezione, non una seconda geometria
#include "RTEnemyArcLibrary.generated.h"

class URTHexMapAsset;

/**
 * L'ARCO DI UN NEMICO, come lo puo' vedere la squadra che lo guarda (#3762).
 *
 * 🔑 **La regola di privacy e' dell'autore, 2026-10-14.** Calcolare la vista del nemico sulla mappa intera puo'
 * rivelare terreno mai visto: se l'arco si ferma contro un muro che la squadra non ha mai visto, quel muro si
 * scopre dalla forma dell'arco. Quindi **l'arco si calcola solo sul terreno che la squadra conosce.** La domanda,
 * come l'autore l'ha vista: *«La privacy dell'arco del nemico (#3762): va calcolato solo sul terreno che la tua
 * squadra conosce, per non rivelare muri mai visti?»* La risposta: *«si»*.
 *
 * 🔑 **Come si traduce in calcolo.** Si costruisce una copia della mappa in cui ogni cella mai vista **esiste
 * ancora** — che una cella esista e' pubblico ([D-373]) — ma e' **vuota**: non blocca la vista, non porta fumo,
 * coperture, porte o muri interni. E' la stessa scelta della mappa ottimistica di [D-372] («ignoto = passabile»),
 * applicata alla vista: ignoto = trasparente. Poi l'arco si chiede a `URTPerceptionLibrary::VisibleCells` su
 * quella copia, e se ne tengono solo le celle note.
 *
 * ⚠️ **Le due meta' servono entrambe.** Senza la copia, un muro mai visto toglierebbe dall'arco le celle note
 * dietro di lui, e il buco lo rivelerebbe. Senza il filtro, l'arco si disegnerebbe su celle mai viste, che il
 * velo non disegna.
 *
 * ⛔ **Non e' una regola di gioco**: nessun esito dipende da qui. E' informazione a schermo, calcolata sul
 * client con dati che il client ha gia'.
 */
UCLASS()
class REFACTORTACTICS_API URTEnemyArcLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Una copia di `Map` in cui le celle fuori da `KnownCells` esistono ma sono vuote.
	 *
	 * Di una cella mai vista si toglie tutto cio' che ferma la vista: `bBlocksLineOfSight`, la superficie (il
	 * fumo), e i muri interni. Coperture, porte e guardie si tolgono dai lati che danno su un'altra cella mai
	 * vista; restano su un lato che confina con una cella nota, perche' il velo lo disegna gia'
	 * ([D-502] (d): un pannello di bordo si vede da un lato qualunque).
	 *
	 * `Map == nullptr` -> `nullptr`. La copia vive in `Outer`, ed e' di chi la chiede.
	 */
	static URTHexMapAsset* KnownTerrainMap(UObject* Outer, const URTHexMapAsset* Map, const TArray<FRTCellId>& KnownCells);

	/**
	 * Le celle che `Enemy` vede, calcolate su `KnownMap` e ristrette a `KnownCells`. In ordine stabile.
	 *
	 * `KnownMap` e' quella di `KnownTerrainMap`, costruita con lo stesso `KnownCells`: si passa gia' fatta perche'
	 * si costruisce una volta per refresh di conoscenza, non una per nemico.
	 */
	static TArray<FRTCellId> SightOnKnownTerrain(const URTHexMapAsset* KnownMap, const FRTPerceiver& Enemy,
		const TArray<FRTCellId>& KnownCells);

	/**
	 * La PORTATA PIENA dei due canali della percezione — il cono frontale fino a `VisionRange` e l'anello ravvicinato —
	 * senza linea di vista, ristretta a `KnownCells`. E' la seconda risposta possibile alla domanda aperta di #3762
	 * («la portata piena del cono, o solo le celle che il nemico vede davvero»), per le tavole dell'autore.
	 *
	 * ⚠️ Non rivela niente per costruzione: non legge il contenuto di nessuna cella, solo quali esistono ([D-373]).
	 */
	static TArray<FRTCellId> ConeOnKnownTerrain(const URTHexMapAsset* Map, const FRTPerceiver& Enemy,
		const TArray<FRTCellId>& KnownCells);
};
