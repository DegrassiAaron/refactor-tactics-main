#pragma once

#include "CoreMinimal.h"
#include "Player/RTPointerInteraction.h" // ERTPointerContext, ERTPointerTargetKind

class ARTUnit;
class UWorld;
struct FRTCellId;
struct FRTHexSnapshot;

/**
 * Cio' che la dichiarazione del verso chiede al controller, e nient'altro (#4065, S4).
 *
 * 🔑 **`FRTFacingDeclaration` non conosce `ARTPlayerController`**: parla con questa interfaccia, che il controller
 * implementa. E' lo schema di `FRTPlaybackHost` (#4057): il file nuovo non nomina i membri del controller, quindi cio' di
 * cui ha bisogno sta scritto qui, in un posto solo.
 *
 * ⚠️ **Prefisso `F` e non `I`**: e' una classe astratta C++, non una `UINTERFACE`. Con `I`, UHT la cercherebbe fra le
 * interfacce riflesse. ⚠️ **E i metodi hanno nomi propri** (`IsFacingInputInert`, non `IsPlanningInputInert`): con lo
 * stesso nome di un metodo del controller, l'override renderebbe virtuale una `UFUNCTION` che oggi non lo e'.
 */
class FRTFacingHost
{
public:
	virtual ~FRTFacingHost() = default;

	/** Il mondo: la mappa esagonale, e il `TurnManager` a cui chiedere la mappa di mira. */
	virtual UWorld* GetFacingWorld() const = 0;

	/** L'unita' selezionata: il verso si dichiara per lei. */
	virtual ARTUnit* GetFacingUnit() const = 0;

	/** La barra mostra un'unita' che non si comanda, o il mondo e' in sola lettura (`IsPlanningWriteBlocked`, #3748). */
	virtual bool IsFacingWriteBlocked() const = 0;

	/** L'autobattle e' in corso (`IsPlanningInputInert`, #971). */
	virtual bool IsFacingInputInert() const = 0;

	/** Il contesto del puntatore, che a sua volta legge se il selettore e' aperto. */
	virtual ERTPointerContext GetFacingPointerContext() const = 0;

	/** La forma di bersaglio dell'azione armata: un bersaglio da scegliere ha la precedenza sul selettore. */
	virtual ERTPointerTargetKind GetFacingTargetKind() const = 0;

	/** La cella e' quella del nemico che la carica pianificata punta ([D-528] (3))? Li' il click ispeziona. */
	virtual bool IsFacingChargeTargetAt(const ARTUnit* Unit, const FRTCellId& Cell) const = 0;

	/** Il pacing (#971): dichiarare un verso e' un ordine. */
	virtual void RecordFacingOrder() = 0;

	/** Il raggio del cursore, solo se il cursore sta nella scena (#3661). */
	virtual bool DeprojectFacingCursor(FVector& OutOrigin, FVector& OutDirection) const = 0;

	/** Lo snapshot di pianificazione dell'unita' e gli Actor agli stessi indici, dall'autorita' (invariante #5). */
	virtual bool MakeFacingSnapshot(const ARTUnit* Unit, FRTHexSnapshot& OutSnapshot, int32& OutUnitId,
		TArray<ARTUnit*>& OutUnits) const = 0;
};
