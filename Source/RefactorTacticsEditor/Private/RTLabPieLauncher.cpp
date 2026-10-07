#include "RTLabPieLauncher.h"

#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "HAL/IConsoleManager.h"
#include "PlayInEditorDataTypes.h"

namespace
{
	/** Cio' che serve per rimettere le cose com'erano: i due valori e i due handle. */
	struct FRTLabPieRestore
	{
		FString ScenarioPrima;
		FString PlaybackControlsPrima;
		FDelegateHandle SuEndPIE;
		FDelegateHandle SuCancelPIE;
	};

	TUniquePtr<FRTLabPieRestore> GRipristino;

	IConsoleVariable* TrovaCVar(const TCHAR* Nome, FString& OutError)
	{
		IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Nome);
		if (!Var)
		{
			OutError = FString::Printf(TEXT("la console variable '%s' non esiste in questo binario"), Nome);
		}
		return Var;
	}

	/** Riapplica i valori catturati, con la stessa priorita' con cui erano stati scavalcati, e si sgancia. */
	void Ripristina()
	{
		if (!GRipristino)
		{
			return;
		}
		TUniquePtr<FRTLabPieRestore> R = MoveTemp(GRipristino);
		FEditorDelegates::EndPIE.Remove(R->SuEndPIE);
		FEditorDelegates::CancelPIE.Remove(R->SuCancelPIE);

		FString Ignorato;
		if (IConsoleVariable* Scenario = TrovaCVar(TEXT("rt.Test.Scenario"), Ignorato))
		{
			Scenario->Set(*R->ScenarioPrima, ECVF_SetByConsole);
		}
		if (IConsoleVariable* Controls = TrovaCVar(TEXT("rt.Debug.PlaybackControls"), Ignorato))
		{
			Controls->Set(*R->PlaybackControlsPrima, ECVF_SetByConsole);
		}
	}
}

FString FRTLabPieLauncher::DevSandboxMapPath()
{
	return TEXT("/Game/RT/Maps/Dev/L_DevSandbox/L_DevSandbox");
}

bool FRTLabPieLauncher::Launch(const FString& ScenarioId, FString& OutError)
{
	OutError.Reset();

	if (ScenarioId.IsEmpty())
	{
		OutError = TEXT("nessuno ScenarioId da lanciare");
		return false;
	}
	if (!GEditor)
	{
		OutError = TEXT("GEditor assente: il lanciatore vive solo nell'Editor");
		return false;
	}
	if (GEditor->PlayWorld != nullptr)
	{
		OutError = TEXT("PIE in corso: fermalo prima di lanciare il banco");
		return false;
	}
	if (GRipristino)
	{
		OutError = TEXT("un lancio precedente aspetta ancora il ripristino delle CVar");
		return false;
	}

	// Entrambe le CVar PRIMA di toccarne una: se la seconda manca, la prima non va cambiata.
	IConsoleVariable* Scenario = TrovaCVar(TEXT("rt.Test.Scenario"), OutError);
	if (!Scenario) { return false; }
	IConsoleVariable* Controls = TrovaCVar(TEXT("rt.Debug.PlaybackControls"), OutError);
	if (!Controls) { return false; }

	GRipristino = MakeUnique<FRTLabPieRestore>();
	GRipristino->ScenarioPrima = Scenario->GetString();
	GRipristino->PlaybackControlsPrima = Controls->GetString();

	// 🔑 `ECVF_SetByConsole`: un valore digitato in console ha quella priorita', e un `Set` a priorita'
	// inferiore verrebbe ignorato con un warning — il banco giocherebbe lo scenario sbagliato credendo di
	// aver scelto.
	Scenario->Set(*ScenarioId, ECVF_SetByConsole);
	Controls->Set(TEXT("1"), ECVF_SetByConsole);

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
