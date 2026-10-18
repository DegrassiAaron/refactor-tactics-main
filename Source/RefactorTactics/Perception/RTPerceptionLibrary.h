#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h" // FRTCellId, ERTHexDirection
#include "RTPerceptionLibrary.generated.h"

class URTHexMapAsset;

/**
 * Quanto una squadra sa di una cella o di chi la occupa (CP 13.1).
 *
 * Tre livelli e non due perche' il **rumore** (CP 13.4) produce contatti che non sono ne' ignoranza ne'
 * certezza: sapere che qualcuno si muove *da quella parte* e' un'informazione diversa dal vederlo. Senza il
 * livello di mezzo il canale acustico si ridurrebbe a un secondo raggio di rilevamento.
 */
UENUM(BlueprintType)
enum class ERTAwareness : uint8
{
	/** Nessun contatto: per la squadra, li' non c'e' niente. */
	Hidden,
	/**
	 * Contatto senza identita' o senza cella esatta — l'area da cui e' arrivato un rumore. Bersagliabile per
	 * CELLA, mai per unita' (CP 13.2). Nessun produttore in CP 13.1: lo riempiono il rumore e la memoria.
	 */
	Uncertain,
	/** Contatto pieno: la squadra vede la cella e chi la occupa. */
	Detected
};

/**
 * Un osservatore, ridotto a cio' che serve per vedere. NON e' un'unita': la percezione non ha bisogno di HP,
 * squadra o abilita', e prendere `ARTUnit` legherebbe una regola pura al mondo di gioco.
 */
USTRUCT(BlueprintType)
struct FRTPerceiver
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Perception")
	FRTCellId Cell;

	/** Orientamento autorevole (CP 16.1): decide dove punta l'arco frontale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Perception")
	ERTHexDirection Facing = ERTHexDirection::E;

	/** `URTHeroData::VisionRange`. 0 o negativo = non vede oltre la propria cella. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Perception")
	int32 VisionRange = 5;

	FRTPerceiver() = default;
};

/**
 * PERCHE' la linea di vista **fra due piani** e' ferma (`D-511`, #3743).
 *
 * Quattro valori oltre a `None`, uno per ramo di `DescribeSightAcrossLayers`: un valore che nessun ramo
 * produce sarebbe un'etichetta che nessuno emette. I tre ostacoli sono gli stessi tre predicati della linea
 * dentro un piano (`ERTLineOfSightBlock`), chiesti agli stessi owner; qui pero' il piano e' quello in cui la
 * linea sta passando, non quello di chi guarda.
 */
UENUM(BlueprintType)
enum class ERTCrossLayerSightBlock : uint8
{
	/** Via libera. E' anche la risposta fra due celle dello STESSO piano: non e' la domanda di questa regola. */
	None = 0,

	/** La linea attraversa la quota di un piano in un punto dove quel piano ha una cella: ha bucato un pavimento. */
	Floor = 1,

	/** Una cella intermedia porta `FRTHexCellData::bBlocksLineOfSight`, su un piano che la linea occupa mentre l'attraversa. */
	CellBlocker = 2,

	/** Il bordo attraversato nega l'attraversamento (`URTHexCoverLibrary::BlocksTraversal`), al piano in cui la linea lo passa. */
	EdgeBlocker = 3,

	/** Un muro interno alto incrocia la corda dentro una cella (`URTHexOcclusionLibrary::BlocksSight`). */
	InteriorGeometry = 4,
};

/** L'esito della linea fra piani, con la ragione e la cella: cio' che `HasSightAcrossLayers` riduce a un bool. */
USTRUCT(BlueprintType)
struct FRTCrossLayerSightResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Perception")
	ERTCrossLayerSightBlock Block = ERTCrossLayerSightBlock::None;

	/**
	 * La cella che ferma la linea, **col suo piano**: il pavimento bucato, la cella che blocca, quella in cui
	 * si entrava dal bordo, quella col muro interno. Ha senso solo se `Block != None`.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Perception")
	FRTCellId BlockedAt;

	bool IsClear() const { return Block == ERTCrossLayerSightBlock::None; }
};

/**
 * Cosa una squadra VEDE (CP 13.1). Pura e headless: nessun Actor, nessun `UWorld`, nessuno stato.
 *
 * Due canali che convivono ([ADR-0005](../../../docs/decisions/adr-0005-orientamento.md) §4b):
 *
 * 1. **arco frontale** — vista piena fino a `VisionRange` dentro il cono di 120 gradi, la stessa
 *    `HexCone` della difesa direzionale (CP 16.2) e non una seconda geometria;
 * 2. **consapevolezza ravvicinata** — entro `CloseAwarenessRange` celle si percepisce in ogni direzione.
 *    Non e' una concessione: senza, un'unita' resta cieca alle spalle per un turno intero, e cio' che il
 *    giocatore non puo' nemmeno percepire smette di essere una scelta e diventa una punizione.
 *
 * La **LOS serve a entrambi**: si vede attraverso l'aria, non attraverso i muri. E il **fumo** cappa il
 * contatto a 2 celle riusando `URTTerrainLibrary::EffectiveTargetingRange`, che di quella regola e' l'owner.
 *
 * La conoscenza e' di **SQUADRA** ([D-043](../../../docs/decisions/RT_PDR_00_Decision_Log.md)): due squadre
 * hanno due insiemi separati, e l'asimmetria fra loro non va costruita — c'e' gia'. Non esiste una relazione
 * «chi vede chi» unita' per unita'.
 */
UCLASS()
class REFACTORTACTICS_API URTPerceptionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Raggio della consapevolezza a 360 gradi. E' **2**, lo stesso cap di contatto del fumo: un solo numero
	 * per «cosa si percepisce comunque a distanza ravvicinata», invece di due che divergerebbero.
	 */
	static constexpr int32 CloseAwarenessRange = 2;

	/**
	 * L'altezza della **testa** sopra il pavimento della propria cella, come frazione dell'altezza di un piano:
	 * tre quinti (`D-511`, #3743). E' da li' che parte e li' che arriva la linea di vista fra due piani.
	 *
	 * ⛔ **Non e' una leva di bilanciamento**: e' una costante della regola, scelta dall'autore fra 0,5, 0,6 e
	 * 0,72 e fissata da un test. Sta in frazione e non in unita' di mondo perche' la regola e' di soli interi:
	 * con `LayerHeight` 250 vale 150 unita', poco sotto la cima di un'unita' alta 180, ma nessuna delle due
	 * misure entra nel calcolo.
	 */
	static constexpr int32 SightHeadHeightNum = 3;
	static constexpr int32 SightHeadHeightDen = 5;

	/**
	 * Le celle che un osservatore vede, in ordine stabile (`URTHexLibrary::StableLess`). Include **sempre** la
	 * propria cella: un'unita' sa dove si trova.
	 *
	 * `Map == nullptr` -> solo la propria cella. Fail-closed: senza mappa non si puo' verificare ne' LOS ne'
	 * terreno, e restituire tutto sarebbe la risposta piu' pericolosa fra le due.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Perception")
	static TArray<FRTCellId> VisibleCells(const URTHexMapAsset* Map, const FRTPerceiver& Observer);

	/**
	 * L'unione di cio' che vedono i membri di una squadra, senza duplicati e in ordine stabile.
	 *
	 * Indipendente dall'ordine degli osservatori (invariante #3): l'ordinamento finale e' canonico, non
	 * l'ordine di scoperta. Squadra vuota -> insieme vuoto, non l'intera mappa.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Perception")
	static TArray<FRTCellId> TeamVisibleCells(const URTHexMapAsset* Map, const TArray<FRTPerceiver>& Team);

	/**
	 * Il livello di consapevolezza della squadra su una cella: `Detected` se qualcuno la vede, altrimenti
	 * `Hidden`.
	 *
	 * `Uncertain` non nasce qui — arriva dal rumore (CP 13.4) e dalla memoria del contatto (CP 13.2), che
	 * hanno informazioni che la vista non ha. Questa funzione non lo produce mai, e dichiararlo e' piu' onesto
	 * che lasciare intendere che il livello di mezzo sia gia' pieno.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Perception")
	static ERTAwareness TeamAwarenessOfCell(const URTHexMapAsset* Map, const TArray<FRTPerceiver>& Team,
		const FRTCellId& Cell);

	/**
	 * La linea di vista fra due celle di **piani diversi** (`D-511`, #3743), con la ragione e la cella che la
	 * fermano. Decisione d'autore del 2026-10-13: «Linea vera in 3D».
	 *
	 * ## La regola
	 *
	 * 1. Ogni cella che esiste e' un **pavimento**, alla quota del suo piano. Dove una cella non c'e', c'e'
	 *    un buco.
	 * 2. La linea va da **testa a testa**: il centro di ciascuna cella, alzato di `SightHeadHeightNum /
	 *    SightHeadHeightDen` di piano sopra il suo pavimento. La stessa altezza ai due capi.
	 * 3. La linea e' **ferma** se attraversa la quota di un piano in un punto dove quel piano ha una cella.
	 * 4. Lungo la linea contano gli ostacoli **del piano in cui la linea sta passando**: un muro a terra ferma
	 *    il tratto basso, un muro sulla piattaforma il tratto alto.
	 *
	 * ## Che cosa garantisce
	 *
	 * - **Lo stesso esito nei due versi**, per costruzione e non per un test di parita': la linea si traccia
	 *   sempre dalla cella piu' bassa alla piu' alta, chiunque sia `From`. Se io ti vedo, tu mi vedi.
	 * - **Soli interi.** Con la testa a una frazione fissa di piano, il punto in cui la linea raggiunge una
	 *   quota e' una frazione della distanza che non dipende dalle misure in unita' di mondo; la cella che lo
	 *   contiene la da' `URTHexLibrary::HexLinePoint`, cioe' l'arrotondamento di `HexLine`.
	 * - **Stessa colonna, piano diverso: ferma** (`D-510`, #3741). E' il caso in cui la linea e' verticale e
	 *   buca il pavimento su cui sta chi e' sopra; qui risponde `Floor` anche se l'asset non ha quella cella.
	 *
	 * ⛔ **Non e' la linea di TIRO, e non la sostituisce.** `URTHexVisionLibrary::HasLineOfSight` resta planare
	 * sul piano di chi guarda: e' la geometria su cui i bot scelgono le celle, e #3741 ha misurato che
	 * toccarla fa cadere due sonde anti-stallo. Colpire un altro piano resta un rifiuto dichiarato (`D-393`,
	 * `D-509`): questa linea decide che cosa una squadra SA, non che cosa puo' colpire.
	 *
	 * `Map == nullptr` -> libera (nessun pavimento noto: il fail-closed e' di `VisibleCells`). Due celle dello
	 * stesso piano -> libera: li' la domanda e' di `HasLineOfSight`, ed e' `ContactHolds` a smistare.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Perception")
	static FRTCrossLayerSightResult DescribeSightAcrossLayers(const URTHexMapAsset* Map, const FRTCellId& From,
		const FRTCellId& To);

	/**
	 * Il tetto di contatto a due celle del **fumo** (`#3757`, scelta d'autore del 2026-10-18: «come i muri»).
	 * Stesso piano -> `URTTerrainLibrary::EffectiveTargetingRange`, invariato. Fra due piani conta il fumo
	 * del piano in cui la linea di `DescribeSightAcrossLayers` attraversa ciascuna cella, da testa a testa:
	 * la linea si traccia dalla cella piu' BASSA alla piu' ALTA, quindi il numero e' lo stesso nei due versi.
	 * `Map == nullptr` -> `Range`.
	 */
	static int32 EffectiveContactRange(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To,
		int32 Range);

	/** `DescribeSightAcrossLayers(...).IsClear()`: un solo attraversamento, il bool ne butta via la ragione. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Perception")
	static bool HasSightAcrossLayers(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To);
};
