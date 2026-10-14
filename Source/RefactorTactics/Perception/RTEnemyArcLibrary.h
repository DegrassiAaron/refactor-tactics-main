#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Perception/RTPerceptionLibrary.h" // FRTPerceiver: l'arco e' quello della percezione, non una seconda geometria
#include "Perception/RTTeamKnowledge.h" // FRTTeamKnowledge: chi guarda, e che cosa conosce
#include "Perception/RTVisibilityBorder.h" // FRTExposedEdge: il contorno dell'arco
#include "RTEnemyArcLibrary.generated.h"

class URTHexMapAsset;

/**
 * Un nemico, ridotto a cio' che serve per il suo arco: dove sta, dove guarda, quanto lontano vede, di che squadra e',
 * e di che colore si disegna. Lo riempie chi ha gli attori; la libreria non ne vede nessuno.
 */
struct FRTEnemyForArc
{
	FRTCellId Cell;
	ERTHexDirection Facing = ERTHexDirection::E;
	int32 VisionRange = 5;
	int32 TeamId = 1;
	FColor Color = FColor::Red;
};

/** L'arco di un nemico a schermo: i lati che lo chiudono, e il colore in cui si disegnano (#3762). */
struct FRTEnemyArcView
{
	TArray<FRTExposedEdge> Edges;
	FColor Color = FColor::Red;
};

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
	 * GLI ARCHI DEI NEMICI IN VISTA, per chi guarda con la conoscenza `Viewer`: uno per nemico, chiave la sua cella.
	 *
	 * 🔑 **Solo i nemici in vista adesso**, cioe' di un'altra squadra e su una cella di `Viewer.VisibleCells`: un nemico
	 * ricordato o incerto non mostra niente (il vincolo della issue). L'arco e' `SightOnKnownTerrain` sul terreno
	 * noto — esplorato e visto — e il contorno e' `ExposedEdgesFacingMapCells`, come la linea di confine della vista.
	 *
	 * ⚠️ **La cella e' una chiave valida** perche' ogni cella ha un solo slot di occupazione autorevole: due nemici
	 * non stanno sulla stessa cella.
	 *
	 * La mappa del terreno noto si costruisce UNA volta per chiamata, e solo se c'e' almeno un nemico in vista.
	 */
	static TMap<FRTCellId, FRTEnemyArcView> ArcsOfEnemiesInView(const URTHexMapAsset* Map,
		const FRTTeamKnowledge& Viewer, const TArray<FRTEnemyForArc>& Enemies);

};
