#include "RTLauncherWorkspace.h"

#include "RTDevSandboxLauncherSubsystem.h"
#include "RTHexEditorMode.h"
#include "ScenarioHarness/RTScenarioDraft.h"

FRTLauncherStartDecision FRTLauncherWorkspace::DecideStart(const FString& SelectedId, ERTScenarioAuthoringResult OpenResult, const FString& OpenError)
{
	FRTLauncherStartDecision Decision;

	// ⚠️ La selezione si controlla PRIMA dell'esito, e non e' un ordine arbitrario: senza id la facade non
	// e' stata nemmeno interrogata, quindi `OpenResult` sarebbe un valore che nessuno ha prodotto. Leggerlo
	// per primo significherebbe classificare come «scenario invalido» il fatto che non ci sia uno scenario.
	if (SelectedId.IsEmpty())
	{
		Decision.Refusal = ERTLauncherStartRefusal::NoSelection;
		Decision.Reason = TEXT("Nessuno scenario selezionato: scegli una riga nell'elenco.");
		return Decision;
	}

	switch (OpenResult)
	{
	case ERTScenarioAuthoringResult::Success:
		Decision.bAllowed = true;
		Decision.Refusal = ERTLauncherStartRefusal::None;
		return Decision;

	case ERTScenarioAuthoringResult::NotFound:
		Decision.Refusal = ERTLauncherStartRefusal::NotFound;
		break;

	case ERTScenarioAuthoringResult::Invalid:
		Decision.Refusal = ERTLauncherStartRefusal::Invalid;
		break;

	// ⛔ Nessun `default:` che conceda l'avvio. `WriteFailed`, `NoScenarioOpen` e `RunFailed` non sono
	// difetti dello scenario e non hanno un gesto che li sblocchi dal pannello — e un valore aggiunto un
	// giorno all'enum della facade deve cadere qui, non passare. E' il `silent fallback` che il guardrail
	// di questa issue vieta, scritto come ramo invece che come raccomandazione.
	default:
		Decision.Refusal = ERTLauncherStartRefusal::ToolFailure;
		break;
	}

	// La frase della facade e' migliore della nostra — nomina il campo, il percorso, la riga — quindi
	// prevale quando c'e'. Il ripiego esiste perche' un rifiuto muto soddisfa meta' dell'AC: non avvia, e
	// non dice perche'.
	if (!OpenError.IsEmpty())
	{
		Decision.Reason = OpenError;
		return Decision;
	}

	switch (Decision.Refusal)
	{
	case ERTLauncherStartRefusal::NotFound:
		Decision.Reason = FString::Printf(TEXT("Lo scenario '%s' non e' nell'indice, o il suo file non si legge."), *SelectedId);
		break;
	case ERTLauncherStartRefusal::Invalid:
		Decision.Reason = FString::Printf(TEXT("Lo scenario '%s' non passa la validazione canonica."), *SelectedId);
		break;
	default:
		Decision.Reason = FString::Printf(TEXT("Lo strumento non ha potuto aprire '%s': il difetto non e' nello scenario."), *SelectedId);
		break;
	}

	return Decision;
}

const TArray<FRTLauncherSurface>& FRTLauncherWorkspace::Surfaces()
{
	// `static` locale e non una variabile globale: `EM_RTHexEditorModeId` e `TabId` sono a loro volta
	// `static` di altre unita' di traduzione, e l'ordine di inizializzazione fra unita' non e' definito.
	// Costruire qui, alla prima chiamata, e' l'unico modo di leggerle gia' inizializzate.
	static const TArray<FRTLauncherSurface> Registry = []
	{
		TArray<FRTLauncherSurface> Out;

		// --- Dichiarate: esistono, e si raggiungono adesso ------------------------------------------

		// La mappa: l'Editor Mode esagonale e i suoi cinque tool.
		Out.Add({ TEXT("Map"), true, ERTLauncherActivationKind::EditorMode, URTHexEditorMode::EM_RTHexEditorModeId, 0 });

		// Lo scenario: elenco, selezione, readout e anteprima vivono nel tab del launcher (#1705, #1753).
		Out.Add({ TEXT("Scenario"), true, ERTLauncherActivationKind::Tab, URTDevSandboxLauncherSubsystem::TabId, 0 });

		// La validazione: stesso tab, ed e' voluto. Vedi il commento su `ActivationTarget`: il registro
		// dichiara dove si atterra, non che ogni superficie abbia una finestra propria.
		Out.Add({ TEXT("Validation"), true, ERTLauncherActivationKind::Tab, URTDevSandboxLauncherSubsystem::TabId, 0 });

		// --- Pendenti: NON dichiarate, e ciascuna nomina la issue che la porta -----------------------

		// Il playback nel viewport e' `T1` dell'epic #1105.
		Out.Add({ TEXT("Playback"), false, ERTLauncherActivationKind::Tab, NAME_None, 1625 });

		// Il TurnLog esiste come API — `URTScenarioAuthoring::GetLastRunLog()` — e non ha ancora un
		// consumatore d'editor. Il suo State Diff e' `T7`.
		Out.Add({ TEXT("TurnLog"), false, ERTLauncherActivationKind::Tab, NAME_None, 1630 });

		return Out;
	}();

	return Registry;
}

const FRTLauncherSurface* FRTLauncherWorkspace::Find(FName Key)
{
	return Surfaces().FindByPredicate([Key](const FRTLauncherSurface& Surface) { return Surface.Key == Key; });
}

FString FRTLauncherWorkspace::PendingLabel(const FRTLauncherSurface& Surface)
{
	if (Surface.bDeclared)
	{
		return FString();
	}

	return FString::Printf(TEXT("%s: non ancora costruita — la porta #%d."), *Surface.Key.ToString(), Surface.PendingIssue);
}
