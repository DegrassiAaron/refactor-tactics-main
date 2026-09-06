#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Turn/RTPacingLibrary.h"
#include "Turn/RTTurnManager.h"

/**
 * `rt.Debug.Pacing` — sommario della sessione corrente. Sola lettura: non tocca lo stato di gioco.
 * Il prefisso `rt.Debug.*` anticipa il namespace di CP 11.4 (#80): quando quella issue verra' lavorata,
 * questo comando va aggiunto al suo elenco, non duplicato.
 */
static void RTDebugPacingCommand(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	if (!World)
	{
		Ar.Log(TEXT("[RT] Nessun mondo attivo."));
		return;
	}
	ARTTurnManager* TM = Cast<ARTTurnManager>(
		UGameplayStatics::GetActorOfClass(World, ARTTurnManager::StaticClass()));
	if (!TM)
	{
		Ar.Log(TEXT("[RT] Nessun TurnManager nel livello."));
		return;
	}

	const FRTPacingSummary S = URTPacingLibrary::SummarizeSamples(TM->GetPacingSamples(), /*CutoffWindowMs=*/ 3000);
	Ar.Logf(TEXT("[RT] Pacing su %d turni%s:"), S.SampleCount,
		S.UnmeasuredSamples > 0
			? *FString::Printf(TEXT(" (%d senza cronometro: campione mai aperto)"), S.UnmeasuredSamples)
			: TEXT(""));
	Ar.Logf(TEXT("[RT]   lock-in: mediana %d ms, p90 %d ms"), S.MedianMsToLockIn, S.P90MsToLockIn);
	Ar.Logf(TEXT("[RT]   tagli veri: %d | attese a vuoto: %d"), S.TrueCutoffs, S.IdleTimeouts);
	Ar.Logf(TEXT("[RT]   playback: mediana %d ms, saltati %d"), S.MedianMsPlayback, S.SkippedPlaybacks);

	// Il carico di decisione (CP 14.6). Il tetto e' `finestre × FastReactionDuration` e vale se ognuna arriva
	// a scadenza: e' cio' che `InitialBankMs` deve poter coprire, non il tempo che il giocatore ha speso
	// davvero — quello lo dira' il playtest, e questa riga e' dove si vedra' se i due divergono.
	if (const ARTTurnManager* Manager = TM)
	{
		const float WindowSeconds = Manager->GetFastReactionDuration();
		Ar.Logf(TEXT("[RT]   finestre di reazione: %d in sessione, tetto %.1f s (= %d x %.1f s a scadenza)"),
			S.TotalReactionWindows,
			URTPacingLibrary::ReactionDecisionSecondsUpperBound(S.TotalReactionWindows, WindowSeconds),
			S.TotalReactionWindows, WindowSeconds);
	}
	// Le opportunity accanto alle finestre, e non al posto loro: il RAPPORTO fra i due e' il dato.
	// Con il solo conteggio delle finestre, «poche interruzioni» e «poche reazioni» sarebbero
	// indistinguibili — e sono due letture opposte della stessa sessione.
	{
		const float OpportunitiesPerTurn = URTPacingLibrary::PerTurnRate(S.TotalReactionOpportunities, S.SampleCount);
		const float BoundariesPerTurn = URTPacingLibrary::PerTurnRate(S.TotalReactionWindows, S.SampleCount);
		if (OpportunitiesPerTurn < 0.f)
		{
			// ⚠️ Non «0,0 per turno»: senza turni non c'e' denominatore, e stampare zero direbbe che il
			// gioco non apre opportunity — un'affermazione che questa sessione non ha misurato.
			Ar.Logf(TEXT("[RT]   opportunity: %d in sessione, per turno NON MISURATO (nessun campione)"),
				S.TotalReactionOpportunities);
		}
		else
		{
			Ar.Logf(TEXT("[RT]   opportunity: %d in sessione, %.2f per turno | boundary %.2f per turno"),
				S.TotalReactionOpportunities, OpportunitiesPerTurn, BoundariesPerTurn);
		}
		if (S.TotalReactionWindows > S.TotalReactionOpportunities)
		{
			// Incoerente per costruzione: le finestre sono un sottoinsieme. Si segnala invece di
			// normalizzare — un rapporto aggiustato nasconderebbe il difetto che lo produce.
			Ar.Logf(TEXT("[RT]   ⚠ INCOERENTE: finestre (%d) > opportunity (%d)"),
				S.TotalReactionWindows, S.TotalReactionOpportunities);
		}
	}
	Ar.Logf(TEXT("[RT]   lettura: tagli > 0 -> alza PlanningSeconds; tagli 0 e attese alte -> e' l'interfaccia, non il timer."));
}

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTDebugPacing(
	TEXT("rt.Debug.Pacing"),
	TEXT("Sommario del pacing della sessione corrente (telemetria: nessun effetto sul gioco)."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTDebugPacingCommand));
