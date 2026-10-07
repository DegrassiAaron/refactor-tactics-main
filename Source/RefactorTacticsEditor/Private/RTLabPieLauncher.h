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
	 * `FRTLabCVarSnapshot::Apply`, cioe' `SetWithCurrentPriority`: vince su un valore gia' digitato in
	 * console senza alzare la priorita' della variabile — e chiede PIE su `L_DevSandbox` tramite
	 * `GlobalMapOverride`, senza toccare il livello aperto nell'Editor.
	 *
	 * Valore e priorita' precedenti vengono catturati e **ripristinati** al primo fra `EndPIE` e `CancelPIE`.
	 *
	 * ⛔ Rifiuta, senza lasciare niente cambiato, se: PIE e' gia' in corso o gia' richiesto (in coda per il
	 * tick successivo: `IsPlaySessionInProgress`); una delle due CVar non si trova; una delle due scritture
	 * non prende (CVar `ReadOnly`, valore non parsabile); un lancio precedente e' ancora in attesa di
	 * ripristino.
	 *
	 * `OnFinished`, se data, e' chiamata **una volta**, al primo fra `EndPIE` e `CancelPIE`, dopo aver
	 * ripristinato le CVar e sganciato i delegate (#3542). Non e' chiamata se `Launch` rifiuta. Il chiamante
	 * che la lega a un oggetto la lega **debole**: il PIE puo' finire dopo che il pannello e' stato chiuso.
	 * Riceve `bRipristinato`: vero solo se ENTRAMBE le CVar sono tornate com'erano; un ripristino che non
	 * prende si dichiara anche nel log.
	 */
	static bool Launch(const FString& ScenarioId, FString& OutError, TFunction<void(bool bRipristinato)> OnFinished = nullptr);

	/**
	 * Le guardie di `Launch` che non dipendono dall'Id **e non toccano le CVar**: `GEditor`, PIE in corso o gia'
	 * richiesto, ripristino di un lancio precedente ancora pendente. `false` col motivo in `OutError`.
	 * ⚠️ L'esistenza delle CVar e una `Apply` rifiutata NON sono qui: restano in `Launch`, e in quei casi la
	 * fixture e' gia' stata scritta.
	 *
	 * 🔑 **Esiste perche' il chiamante possa chiedere «si puo' lanciare?» PRIMA di scrivere lo scenario su
	 * disco** (#3542): il pannello salvava la fixture e solo dopo scopriva che PIE era gia' in corso,
	 * lasciando un file per un lancio mai avvenuto. Non modifica nulla.
	 */
	static bool CanLaunch(FString& OutError);

	/** `/Game/RT/Maps/Dev/L_DevSandbox/L_DevSandbox`: la mappa ospite di ogni scenario. */
	static FString DevSandboxMapPath();
};
