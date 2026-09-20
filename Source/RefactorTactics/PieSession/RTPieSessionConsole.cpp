// I tre comandi del conduttore di seduta (#3208).
//
// Stessa forma dei comandi dell'harness (`rt.Test.Run`, `rt.Test.List`): chi conduce una seduta ha gia'
// la console aperta, e non deve imparare un secondo modo di parlare col gioco.

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "PieSession/RTPieSessionPlaylist.h"
#include "PieSession/RTPieSessionSubsystem.h"

namespace
{
	URTPieSessionSubsystem* RTPieConduttore(UWorld* World)
	{
		const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		return GI ? GI->GetSubsystem<URTPieSessionSubsystem>() : nullptr;
	}

	/**
	 * Stampa la coda PRIMA di avviarla, ed e' il punto del comando senza argomenti.
	 *
	 * Chi apre una seduta deve sapere **prima** cosa non e' coperto: scoprire a meta' che tre voci erano
	 * fuori portata costa la stessa apertura che il conduttore esiste per risparmiare.
	 */
	void RTPieStampaPiano(const FRTPieSessionPlan& Plan, FOutputDevice& Ar)
	{
		Ar.Logf(TEXT("[RT-Pie] %d passi in coda:"), Plan.Steps.Num());
		for (int32 I = 0; I < Plan.Steps.Num(); ++I)
		{
			Ar.Logf(TEXT("  %2d. %-24s  da  %s"), I + 1, *Plan.Steps[I].PieItem,
				*Plan.Steps[I].ScenarioId);
		}

		for (const FString& Fuori : Plan.Excluded)
		{
			Ar.Logf(TEXT("[RT-Pie] ESCLUSA: %s"), *Fuori);
		}

		for (const FString& Ambigua : Plan.Ambiguous)
		{
			Ar.Logf(TEXT("[RT-Pie] AMBIGUA: %s"), *Ambigua);
		}

		if (Plan.Ambiguous.Num() > 0)
		{
			Ar.Logf(TEXT("[RT-Pie] la coda NON parte: scegli l'allestimento nominando lo scenario "
				"invece della voce."));
		}
	}

	void RTPieSessionCommand(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
	{
		URTPieSessionSubsystem* Conduttore = RTPieConduttore(World);
		if (!Conduttore)
		{
			Ar.Logf(ELogVerbosity::Error, TEXT("[RT-Pie] nessun conduttore: serve una GameInstance viva."));
			return;
		}

		// ⛔ **Senza argomenti si COMPONE davvero e si stampa**, non si stampa l'uso: la spec promette un
		// dry-run, e un messaggio d'aiuto che dice «stampo cosa comporrei» senza comporre niente era la
		// promessa scritta due volte e mantenuta zero. Un prefisso vuoto seleziona tutto il corpus.
		// Un secondo argomento `dry` fa lo stesso su un selettore, senza avviare.
		const bool bSoloStampa = Args.Num() == 0
			|| (Args.Num() > 1 && Args[1].Equals(TEXT("dry"), ESearchCase::IgnoreCase));
		const FString Selettore = Args.Num() > 0 ? Args[0] : FString();

		const FRTPieSessionPlan Plan = URTPieSessionPlaylist::Compose(Selettore);
		RTPieStampaPiano(Plan, Ar);

		if (bSoloStampa)
		{
			Ar.Logf(TEXT("[RT-Pie] dry-run: non ho avviato niente. Ripeti col solo selettore per condurre."));
			return;
		}

		if (!Plan.IsRunnable())
		{
			Ar.Logf(ELogVerbosity::Warning, TEXT("[RT-Pie] niente da condurre."));
			return;
		}

		if (!Conduttore->HasPorts())
		{
			// Le installa il GameMode al `BeginPlay`: se mancano, la partita non e' partita.
			Ar.Logf(ELogVerbosity::Error,
				TEXT("[RT-Pie] porte non installate: premi Play prima di aprire la seduta."));
			return;
		}

		Conduttore->Begin(Plan.Steps);

		// ⚠️ Le esclusioni si ripetono QUI, dopo l'avvio: stampate solo prima, scorrerebbero via nello
		// stesso istante in cui il primo scenario parte e riempie il log.
		if (Plan.Excluded.Num() > 0)
		{
			Ar.Logf(ELogVerbosity::Warning,
				TEXT("[RT-Pie] ⚠️ questa seduta NON copre %d voci chieste — vedi le righe ESCLUSA sopra."),
				Plan.Excluded.Num());
		}
		Ar.Logf(TEXT("[RT-Pie] seduta %s avviata. Verdetti: 1 si', 2 no, 3 non giudicabile "
			"(o rt.Pie.Verdict pass|fail|na)."), *Conduttore->SessionId());
	}

	void RTPieVerdictCommand(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
	{
		URTPieSessionSubsystem* Conduttore = RTPieConduttore(World);
		if (!Conduttore)
		{
			Ar.Logf(ELogVerbosity::Error, TEXT("[RT-Pie] nessun conduttore."));
			return;
		}

		if (Conduttore->State() != ERTPieSessionState::AwaitingVerdict)
		{
			Ar.Logf(ELogVerbosity::Warning,
				TEXT("[RT-Pie] nessun passo sta aspettando un verdetto."));
			return;
		}

		if (Args.Num() == 0)
		{
			Ar.Logf(TEXT("[RT-Pie] rt.Pie.Verdict pass|fail|na [motivo]"));
			return;
		}

		const FString Scelta = Args[0].ToLower();
		ERTPieVerdict Verdetto = ERTPieVerdict::Pending;
		if (Scelta == TEXT("pass") || Scelta == TEXT("si") || Scelta == TEXT("1"))
		{
			Verdetto = ERTPieVerdict::Pass;
		}
		else if (Scelta == TEXT("fail") || Scelta == TEXT("no") || Scelta == TEXT("2"))
		{
			Verdetto = ERTPieVerdict::Fail;
		}
		else if (Scelta == TEXT("na") || Scelta == TEXT("3"))
		{
			Verdetto = ERTPieVerdict::NotJudgeable;
		}
		else
		{
			Ar.Logf(ELogVerbosity::Error, TEXT("[RT-Pie] '%s' non e' un verdetto: pass, fail o na."),
				*Args[0]);
			return;
		}

		// Il motivo e' il resto della riga: un `na` senza motivo lascia un buco che si legge dopo.
		FString Motivo;
		for (int32 I = 1; I < Args.Num(); ++I)
		{
			Motivo += (I > 1 ? TEXT(" ") : TEXT("")) + Args[I];
		}

		if (Verdetto == ERTPieVerdict::NotJudgeable && Motivo.IsEmpty())
		{
			Ar.Logf(ELogVerbosity::Warning,
				TEXT("[RT-Pie] 'na' senza motivo: scrivilo, o chi legge il file non sapra' perche'."));
		}

		Conduttore->SubmitVerdict(Verdetto, Motivo);
	}

	void RTPieAbortCommand(const TArray<FString>&, UWorld* World, FOutputDevice& Ar)
	{
		URTPieSessionSubsystem* Conduttore = RTPieConduttore(World);
		if (!Conduttore || !Conduttore->IsConducting())
		{
			Ar.Logf(ELogVerbosity::Warning, TEXT("[RT-Pie] nessuna seduta in corso."));
			return;
		}

		Conduttore->Abort();
		Ar.Logf(TEXT("[RT-Pie] seduta interrotta · %s"), *Conduttore->WrittenSessionFile());
	}
}

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTPieSession(
	TEXT("rt.Pie.Session"),
	TEXT("rt.Pie.Session [<prefisso scenario>|<id voce>,<id voce>] [dry] — senza argomenti, o con 'dry', compone e stampa senza avviare."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTPieSessionCommand));

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTPieVerdict(
	TEXT("rt.Pie.Verdict"),
	TEXT("rt.Pie.Verdict pass|fail|na [motivo] — chiude il passo corrente e passa al successivo."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTPieVerdictCommand));

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTPieAbort(
	TEXT("rt.Pie.Session.Abort"),
	TEXT("rt.Pie.Session.Abort — scrive i verdetti gia' dati, il resto resta NOT_RUN."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTPieAbortCommand));
