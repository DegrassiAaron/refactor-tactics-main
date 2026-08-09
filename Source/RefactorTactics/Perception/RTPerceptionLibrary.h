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
};
