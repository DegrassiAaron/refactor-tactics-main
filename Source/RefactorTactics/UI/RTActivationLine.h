#pragma once

#include "CoreMinimal.h"
#include "Turn/RTTurnRules.h" // ERTMatchPhase

/**
 * La riga del Registro per un'attivazione che il playback ha mostrato (#4033, [D-600] (1)).
 *
 * 🔴 **Solo presentazione: nessun hash.** Non entra in `FRTTurnLogEntry`, nello snapshot, nel replay, in `StateHash`
 * ne' nella replicazione. Nasce da un fatto gia' risolto (`ARTTurnManager::ShowActivation`) e vive quanto il turno.
 *
 * 🔑 **Il testo e' composto quando la riga nasce**, perche' li' c'e' l'unita': dopo, la riga non legge piu' il mondo.
 * Il feed la fonde con le righe d'esito (`URTHudViewModel::MergeActivationLines`) senza rifare nessun giudizio di
 * conoscenza: `ViewerTeamId` e' la chiave del pubblico, non un verdetto.
 *
 * Struct C++ semplice e non `USTRUCT`, in un header proprio: `RTTurnManager.h` la include senza tirarsi dietro il
 * view model.
 */
struct FRTActivationLine
{
	/** Chi attiva, come `ARTUnit::StableUnitId`. Decide dove la riga si inserisce nel feed. */
	int32 SourceStableUnitId = INDEX_NONE;

	/** L'azione attivata. */
	FName ActionId;

	/** La fase del playback in cui l'attivazione e' stata mostrata. */
	ERTMatchPhase Phase = ERTMatchPhase::Planning;

	/** La squadra per cui il playback l'ha mostrata: la chiave del pubblico (#4033, R7b). */
	int32 ViewerTeamId = INDEX_NONE;

	/**
	 * Il turno che il playback stava mostrando, lo stesso `TurnNumber` delle voci di TurnLog di quel turno. Il widget legge
	 * il turno del feed dalla PRIMA riga (`Feed[0].TurnNumber`): un turno con sole attivazioni non ha una riga d'esito da
	 * cui copiarlo, quindi la riga porta il suo.
	 */
	int32 TurnNumber = 0;

	/** La frase gia' composta, «<unita'> — <azione>». */
	FText Text;
};
