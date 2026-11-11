#pragma once

#include "CoreMinimal.h"
#include "Turn/RTTurnManager.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Le finestre di reazione nei test: presentarle come in partita (`#3666`).
 *
 * 🔑 **Da `#3666` la finestra non si consegna alla sospensione.** Il resolver sospende dentro `LockInAndResolve`;
 * `OnReactionWindowOpened` si esegue quando il playback parziale si trattiene sull'ingresso, e il countdown comincia
 * da li'. Un test che legge la finestra subito dopo `LockInAndResolve` con il playback acceso — il default — la
 * troverebbe sospesa e non ancora presentata.
 */
namespace RTReactionWindowFixtures
{
	/**
	 * Pompa il manager finche' la finestra in attesa non e' presentata. Vero se lo e'.
	 *
	 * ⚠️ **Non fa niente se nessuna finestra attende**: senza un `OpportunityId` aperto il ciclo non gira un solo
	 * tick, quindi chiamarla dopo un `LockInAndResolve` che non sospende non tocca il turno. Il passo e' quello del
	 * runner degli scenari, e l'orologio della finestra non corre prima della presentazione: al ritorno la finestra
	 * ha ancora la sua durata intera.
	 */
	inline bool PompaFinoAllaPresentazione(ARTTurnManager* TurnManager, int32 MaxTick = 600, float Passo = 0.05f)
	{
		if (TurnManager == nullptr)
		{
			return false;
		}
		for (int32 Tick = 0; Tick < MaxTick
			&& !TurnManager->IsReactionWindowPresented()
			&& !TurnManager->GetOpenReactionWindowId().IsEmpty(); ++Tick)
		{
			TurnManager->Tick(Passo);
		}
		return TurnManager->IsReactionWindowPresented();
	}
}

#endif // WITH_DEV_AUTOMATION_TESTS
