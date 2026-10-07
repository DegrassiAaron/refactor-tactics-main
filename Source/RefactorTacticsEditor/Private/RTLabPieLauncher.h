// Il lanciatore PIE del Lab: la sola parte del banco che tocca `GEditor` (spec §3).
//
// 🔑 **E' separato dal modello per la stessa ragione per cui il modello e' separato dal widget**: cio' che
// qui succede non lo vede nessun automation test, quindi deve essere il meno possibile. Costruire la
// fixture, salvarla e verificarla e' di `FRTLabViewModel::PrepareForPie`; qui si chiede PIE e si
// ripristinano due CVar.
//
// ⛔ **Nessuna regola di gioco**: lo scenario lo risolve il GameMode dalla console, come per ogni Id.

#pragma once

#include "CoreMinimal.h"

class FRTLabPieLauncher
{
public:
	/**
	 * Imposta `rt.Test.Scenario` su `ScenarioId` e `rt.Debug.PlaybackControls` su `1` — entrambe con
	 * `ECVF_SetByConsole`, perche' un valore gia' digitato in console vince su un `Set` a priorita'
	 * inferiore — e chiede PIE su `L_DevSandbox` tramite `GlobalMapOverride`, senza toccare il livello
	 * aperto nell'Editor.
	 *
	 * I valori precedenti vengono catturati e **ripristinati** al primo fra `EndPIE` e `CancelPIE`.
	 *
	 * ⛔ Rifiuta, senza toccare niente, se: PIE e' gia' in corso; una delle due CVar non si trova; un
	 * lancio precedente e' ancora in attesa di ripristino.
	 */
	static bool Launch(const FString& ScenarioId, FString& OutError);

	/** `/Game/RT/Maps/Dev/L_DevSandbox/L_DevSandbox`: la mappa ospite di ogni scenario. */
	static FString DevSandboxMapPath();
};
