#include "Turn/RTPacingConsole.h"
#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Turn/RTPacingLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Misc/Paths.h"

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

// ---------------------------------------------------------------------------------------------------
// `rt.Debug.RecordPacing` — armare il CSV senza piazzare il TurnManager (#3398).
//
// 🔑 **Esiste perche' la via alternativa invaliderebbe la misura che serve.** `bRecordPacing` e'
// `EditAnywhere`, quindi la spunta vive solo su un'istanza selezionabile; ma il TurnManager lo spawna
// `ARTGameMode` (`RTGameMode.cpp:509`) da una classe cablata, e piazzarlo nel livello per far comparire
// la spunta fa aprire il turno 1 **prima** dell'allestimento — il difetto che `#2102`/`D-314` hanno
// chiuso, dichiarato dal warning in `RTTurnManager.cpp:97`. Quel turno 1 e' il primo campione di
// `MsToLockIn`, cioe' proprio cio' che `U19` va a misurare.
// ---------------------------------------------------------------------------------------------------

ERTRecordPacingRequest URTPacingConsoleLibrary::ParseArgs(const TArray<FString>& Args)
{
	if (Args.Num() == 0)
	{
		return ERTRecordPacingRequest::Query;
	}
	if (Args.Num() > 1)
	{
		// Due argomenti non hanno una lettura ovvia, e sceglierne una sarebbe indovinare.
		return ERTRecordPacingRequest::Invalid;
	}

	const FString A = Args[0].TrimStartAndEnd().ToLower();
	if (A == TEXT("1") || A == TEXT("true") || A == TEXT("on") || A == TEXT("si"))
	{
		return ERTRecordPacingRequest::Enable;
	}
	if (A == TEXT("0") || A == TEXT("false") || A == TEXT("off") || A == TEXT("no"))
	{
		return ERTRecordPacingRequest::Disable;
	}
	return ERTRecordPacingRequest::Invalid;
}

TArray<FString> URTPacingConsoleLibrary::Describe(ERTRecordPacingRequest Request, bool bWasEnabled,
	const FString& CsvDir, const FString& ExistingCsvPath)
{
	TArray<FString> Out;

	if (Request == ERTRecordPacingRequest::Invalid)
	{
		Out.Add(TEXT("[RT] Argomento non interpretabile: nulla e' stato cambiato."));
		Out.Add(TEXT("[RT]   uso: rt.Debug.RecordPacing [1|0]  (senza argomenti: dichiara lo stato)"));
		return Out;
	}

	const bool bNowEnabled =
		Request == ERTRecordPacingRequest::Enable ? true :
		Request == ERTRecordPacingRequest::Disable ? false : bWasEnabled;

	if (Request == ERTRecordPacingRequest::Query)
	{
		Out.Add(FString::Printf(TEXT("[RT] Registrazione CSV del pacing: %s."),
			bWasEnabled ? TEXT("ATTIVA") : TEXT("SPENTA")));
	}
	else if (bNowEnabled == bWasEnabled)
	{
		Out.Add(FString::Printf(TEXT("[RT] Registrazione CSV del pacing: gia' %s, niente da cambiare."),
			bWasEnabled ? TEXT("ATTIVA") : TEXT("SPENTA")));
	}
	else
	{
		Out.Add(FString::Printf(TEXT("[RT] Registrazione CSV del pacing: %s -> %s."),
			bWasEnabled ? TEXT("ATTIVA") : TEXT("SPENTA"),
			bNowEnabled ? TEXT("ATTIVA") : TEXT("SPENTA")));
	}

	if (bNowEnabled)
	{
		Out.Add(ExistingCsvPath.IsEmpty()
			? FString::Printf(TEXT("[RT]   il file nascera' in %s come pacing_<data>.csv"), *CsvDir)
			: FString::Printf(TEXT("[RT]   file: %s"), *ExistingCsvPath));
		// ⚠️ Il flag e' letto da `FRTPacingRecorder::Close`, quindi arma il PRIMO turno che si chiude da
		// adesso. Chi arma a partita iniziata ottiene un CSV piu' corto della partita, e senza questa
		// riga non avrebbe modo di accorgersene prima di cercare le righe mancanti.
		Out.Add(TEXT("[RT]   vale dal primo turno che si chiude da adesso: i turni gia' conclusi restano ")
			TEXT("solo in memoria (rt.Debug.Pacing)."));
	}
	else
	{
		Out.Add(TEXT("[RT]   i campioni si accumulano comunque in memoria: rt.Debug.Pacing li legge."));
	}

	return Out;
}

static void RTDebugRecordPacingCommand(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	const ERTRecordPacingRequest Request = URTPacingConsoleLibrary::ParseArgs(Args);

	// Un argomento illeggibile si rifiuta PRIMA di cercare il mondo: «non ho capito» e «non c'e' partita»
	// sono due diagnosi diverse, e la seconda nasconderebbe la prima.
	if (Request == ERTRecordPacingRequest::Invalid)
	{
		for (const FString& Line : URTPacingConsoleLibrary::Describe(Request, false, FString(), FString()))
		{
			Ar.Log(*Line);
		}
		return;
	}

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

	const bool bWasEnabled = TM->bRecordPacing;
	if (Request == ERTRecordPacingRequest::Enable)
	{
		TM->bRecordPacing = true;
	}
	else if (Request == ERTRecordPacingRequest::Disable)
	{
		TM->bRecordPacing = false;
	}

	const FString CsvDir = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RT")));
	for (const FString& Line :
		URTPacingConsoleLibrary::Describe(Request, bWasEnabled, CsvDir, TM->GetPacingCsvPath()))
	{
		Ar.Log(*Line);
	}
}

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTDebugRecordPacing(
	TEXT("rt.Debug.RecordPacing"),
	TEXT("Arma o disarma il CSV di pacing sul TurnManager vivo (telemetria: nessun effetto sul gioco). ")
	TEXT("Senza argomenti dichiara lo stato."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTDebugRecordPacingCommand));
