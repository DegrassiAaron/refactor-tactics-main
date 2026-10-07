#include "RTLabPieLauncher.h"

#include "RTLabCVarSnapshot.h"

#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "PlayInEditorDataTypes.h"

namespace
{
	/** Cio' che serve per rimettere le cose com'erano: le due fotografie e i due handle. */
	struct FRTLabPieRestore
	{
		FRTLabCVarSnapshot Scenario;
		FRTLabCVarSnapshot PlaybackControls;
		FDelegateHandle SuEndPIE;
		FDelegateHandle SuCancelPIE;
		/**
		 * Chiamata da `Ripristina()` a CVar rimesse e delegate sganciati (#3542); puo' essere vuota.
		 * Il parametro dice se ENTRAMBE le `Restore` hanno preso.
		 */
		TFunction<void(bool bRipristinato)> OnFinished;
	};

	TUniquePtr<FRTLabPieRestore> GRipristino;

	/**
	 * Riscrive i valori catturati — con `SetWithCurrentPriority`, quindi anche il `SetBy` torna quello di
	 * prima — e si sgancia. Un ripristino che non prende si dichiara nel log: nessuno guarda l'Editor in quel
	 * momento, e una CVar rimasta sul valore del banco farebbe giocare lo scenario sbagliato al PIE dopo.
	 */
	void Ripristina()
	{
		if (!GRipristino)
		{
			return;
		}
		TUniquePtr<FRTLabPieRestore> R = MoveTemp(GRipristino);
		FEditorDelegates::EndPIE.Remove(R->SuEndPIE);
		FEditorDelegates::CancelPIE.Remove(R->SuCancelPIE);

		// Entrambe le `Restore` si eseguono sempre: la seconda non deve restare indietro perche' la prima non ha preso.
		FString Errore;
		bool bRipristinato = true;
		if (!R->Scenario.Restore(Errore))
		{
			bRipristinato = false;
			UE_LOG(LogTemp, Warning, TEXT("Lab PIE: ripristino non riuscito: %s"), *Errore);
		}
		if (!R->PlaybackControls.Restore(Errore))
		{
			bRipristinato = false;
			UE_LOG(LogTemp, Warning, TEXT("Lab PIE: ripristino non riuscito: %s"), *Errore);
		}

		// 🔑 **Per ultima**: chi ascolta «e' finito» deve trovare le CVar gia' com'erano e il lanciatore gia'
		// libero (`GRipristino` vuoto, quindi un nuovo `Launch` e' lecito dal suo interno). Porta l'esito:
		// dire «sono tornate com'erano» dopo un ripristino fallito sarebbe una frase falsa a schermo.
		if (R->OnFinished)
		{
			R->OnFinished(bRipristinato);
		}
	}
}

FString FRTLabPieLauncher::DevSandboxMapPath()
{
	return TEXT("/Game/RT/Maps/Dev/L_DevSandbox/L_DevSandbox");
}

bool FRTLabPieLauncher::CanLaunch(FString& OutError)
{
	OutError.Reset();

	if (!GEditor)
	{
		OutError = TEXT("GEditor assente: il lanciatore vive solo nell'Editor");
		return false;
	}
	// `IsPlaySessionInProgress` copre il PIE in corso E la richiesta gia' in coda per il tick successivo:
	// `PlayWorld` da solo e' nullo fra `RequestPlaySession` e l'avvio, e un secondo clic lo scavalcherebbe.
	if (GEditor->IsPlaySessionInProgress())
	{
		OutError = TEXT("PIE in corso o gia' richiesto: fermalo o aspetta prima di lanciare il banco");
		return false;
	}
	if (GRipristino)
	{
		OutError = TEXT("un lancio precedente aspetta ancora il ripristino delle CVar");
		return false;
	}
	return true;
}

bool FRTLabPieLauncher::Launch(const FString& ScenarioId, FString& OutError, TFunction<void(bool bRipristinato)> OnFinished)
{
	OutError.Reset();

	if (ScenarioId.IsEmpty())
	{
		OutError = TEXT("nessuno ScenarioId da lanciare");
		return false;
	}
	// Le guardie indipendenti dall'Id che stanno in `CanLaunch` sono quelle che NON toccano le CVar
	// (`GEditor`, PIE in corso, ripristino pendente): si possono chiedere **prima** di scrivere lo scenario.
	// ⚠️ L'esistenza delle CVar (`Capture`) e una `Apply` rifiutata restano QUI: in quei casi la fixture e'
	// gia' stata scritta su disco quando il lancio rifiuta.
	if (!CanLaunch(OutError))
	{
		return false;
	}

	// Entrambe le fotografie PRIMA di toccare una CVar: se la seconda manca, la prima non va cambiata.
	TUniquePtr<FRTLabPieRestore> Nuovo = MakeUnique<FRTLabPieRestore>();
	if (!FRTLabCVarSnapshot::Capture(TEXT("rt.Test.Scenario"), Nuovo->Scenario, OutError)) { return false; }
	if (!FRTLabCVarSnapshot::Capture(TEXT("rt.Debug.PlaybackControls"), Nuovo->PlaybackControls, OutError)) { return false; }

	// 🔑 `Apply` scrive con `SetWithCurrentPriority` (#3541): vince su un valore digitato in console — un
	// `Set` a priorita' inferiore verrebbe ignorato, e il banco giocherebbe lo scenario sbagliato credendo di
	// aver scelto — **senza alzare il pavimento** della variabile. E rilegge: se non ha preso, il lancio si
	// rifiuta con il motivo invece di partire su uno scenario che non e' quello scelto.
	if (!Nuovo->Scenario.Apply(ScenarioId, OutError)) { return false; }
	if (!Nuovo->PlaybackControls.Apply(TEXT("1"), OutError))
	{
		// La prima e' gia' applicata: la si rimette com'era prima di rifiutare. Se anche questo non prende,
		// il motivo del rifiuto resta quello primario e il secondo finisce nel log.
		FString ErroreRipristino;
		if (!Nuovo->Scenario.Restore(ErroreRipristino))
		{
			UE_LOG(LogTemp, Warning, TEXT("Lab PIE: ripristino non riuscito: %s"), *ErroreRipristino);
		}
		return false;
	}
	Nuovo->OnFinished = MoveTemp(OnFinished);
	GRipristino = MoveTemp(Nuovo);

	// Al primo dei due che scatta si ripristina e ci si sgancia da entrambi. `CancelPIE` copre il PIE che
	// non comincia: `RequestPlaySession` e' differita, ed `EndPIE` da sola scatterebbe solo per una
	// sessione partita.
	GRipristino->SuEndPIE = FEditorDelegates::EndPIE.AddLambda([](const bool) { Ripristina(); });
	GRipristino->SuCancelPIE = FEditorDelegates::CancelPIE.AddLambda([]() { Ripristina(); });

	FRequestPlaySessionParams Params;
	Params.GlobalMapOverride = DevSandboxMapPath();
	GEditor->RequestPlaySession(Params);
	return true;
}
