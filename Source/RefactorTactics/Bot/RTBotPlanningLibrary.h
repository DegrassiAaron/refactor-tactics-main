// Copyright RefactorTactics. All Rights Reserved.
//
// La DECISIONE dei bot, fuori dall'orchestratore (#3013, fetta di #1818).
//
// 🔑 **Qui non c'e' un mondo, ed e' tutto il punto.** `ARTTurnManager::PlanBots` teneva 1 054 righe di
// decisione dentro un `Actor`: per provarne una sola serviva spawnare una partita. Questa libreria riceve
// lo snapshot, i fatti delle unita' e i pesi, e restituisce i piani — piu' le righe di log, che prima
// erano un effetto collaterale e ora sono un'uscita.
//
// ⛔ **Non decide NULLA di nuovo.** La fetta dichiara gameplay invariato: stesse scelte, stesso ordine,
// stesso TurnLog. Il corpus golden e' l'oracolo che lo sorveglia.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Bot/RTBotPlanning.h"
#include "Turn/RTHexSim.h"
#include "Perception/RTTeamKnowledge.h"
#include "RTBotPlanningLibrary.generated.h"

UCLASS()
class REFACTORTACTICS_API URTBotPlanningLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Pianifica il turno di tutti i bot presenti nei fatti.
	 *
	 * @param BaseSnapshot   lo snapshot canonico del turno; `Facts[i].Index` indicizza `BaseSnapshot.Units`
	 * @param Facts          i fatti di TUTTE le unita' vive, bot e non: le non-bot sono contesto
	 * @param Weights        i pesi dell'utility scoring, copiati dall'orchestratore
	 * @param KnowledgeByTeam la conoscenza di squadra, per `TeamId`
	 * @param IdleTurns      i turni d'inattivita' per `StableUnitId` — ⚠️ **letta E scritta**
	 * @param IdleRound      l'ultimo round in cui l'inattivita' e' stata aggiornata — ⚠️ **letta E scritta**
	 * @param TurnNumber     il turno corrente, che la guardia del decadimento confronta
	 * @param bRecordAudit   se popolare `AuditDecisions`: costa, e in partita si paga solo registrando
	 * @return               un piano per ogni bot, piu' le righe di log nell'ordine prodotto
	 *
	 * ⚠️ **L'ordine di visita e' quello dei fatti**, e non e' un dettaglio: i bot della stessa squadra
	 * prenotano la rotta scelta su uno snapshot condiviso, quindi chi decide prima vincola chi segue.
	 * Cambiarlo cambierebbe il gioco.
		 *
	 * ⛔ **Non e' una funzione pura, e va detto invece di lasciarlo scoprire**: `IdleTurns` e `IdleRound`
	 * sono memoria che attraversa i turni, e la decisione la aggiorna mentre decide. Restano parametri
	 * mutabili perche' spostarle nell'uscita significherebbe che l'orchestratore le riapplica — stesso
	 * stato, un giro in piu', e una finestra in cui possono divergere. Cio' che questa fetta ottiene non
	 * e' la purezza: e' che **per provare una decisione non serva piu' un mondo**.
	 */
	static FRTBotPlanningOutcome PlanTurn(
		const FRTHexSnapshot& BaseSnapshot,
		const TArray<FRTBotUnitFacts>& Facts,
		const FRTBotWeights& Pesi,
		const TMap<int32, FRTTeamKnowledge>& KnowledgeByTeam,
		TMap<int32, int32>& IdleTurns,
		TMap<int32, int32>& IdleRound,
		int32 TurnNumber,
		bool bRecordAudit);
};
