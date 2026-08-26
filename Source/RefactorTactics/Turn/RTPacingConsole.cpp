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
	Ar.Logf(TEXT("[RT]   lettura: tagli > 0 -> alza PlanningSeconds; tagli 0 e attese alte -> e' l'interfaccia, non il timer."));
}

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTDebugPacing(
	TEXT("rt.Debug.Pacing"),
	TEXT("Sommario del pacing della sessione corrente (telemetria: nessun effetto sul gioco)."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTDebugPacingCommand));
