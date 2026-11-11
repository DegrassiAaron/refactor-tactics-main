#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"                // ERTHexDirection
#include "Player/RTPointerInteraction.h" // ERTPointerContext, ERTPointerTargetKind

class ARTUnit;
class UWorld;

/**
 * Cio' che l'arbitro delle aree di pianificazione chiede al controller, e nient'altro (#4065, S5).
 *
 * 🔑 **`FRTPlanningOverlayArbiter` non conosce `ARTPlayerController`**: parla con questa interfaccia, che il controller
 * implementa — lo schema di `FRTPlaybackHost` (#4057) e di `FRTFacingHost`. Sono tutte LETTURE: l'arbitro decide cosa la
 * mappa mostra, e non scrive niente del controller.
 *
 * ⚠️ **Prefisso `F` e non `I`**, e **nomi propri** (`GetOverlayPointerContext`, non `GetPointerContext`): con lo stesso nome
 * di un metodo del controller, l'override renderebbe virtuale una `UFUNCTION` che oggi non lo e'.
 */
class FRTPlanningOverlayHost
{
public:
	virtual ~FRTPlanningOverlayHost() = default;

	/** Il mondo: la mappa esagonale, il `TurnManager` (turno, fine partita, conoscenza della squadra), le unita'. */
	virtual UWorld* GetOverlayWorld() const = 0;

	/** L'unita' selezionata: di chi e' il piano che le aree mostrano. */
	virtual const ARTUnit* GetOverlayUnit() const = 0;

	/** Il contesto e il tipo di bersaglio si LEGGONO dal puntatore, che e' l'autorita'. */
	virtual ERTPointerContext GetOverlayPointerContext() const = 0;
	virtual ERTPointerTargetKind GetOverlayTargetKind() const = 0;

	/** Il lato su cui il selettore del verso ha girato la mesh, se c'e'. */
	virtual TOptional<ERTHexDirection> GetOverlayFacingSector() const = 0;

	/** Il piano attivo (#1775). */
	virtual int32 GetOverlayActiveLayer() const = 0;

	/** La squadra di chi guarda: la conoscenza che le aree del fuoco e della skill rispettano. */
	virtual int32 GetOverlayObserverTeamId() const = 0;

	/** L'unita' ispezionata, se la scheda e' fissata, e se la barra la segue (`#3748`). */
	virtual const ARTUnit* GetOverlayInspectedUnit() const = 0;
	virtual bool IsOverlayInspectorPinned() const = 0;
	virtual bool DoesOverlayDockFollowInspection() const = 0;
};
