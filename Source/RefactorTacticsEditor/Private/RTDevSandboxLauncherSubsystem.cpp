#include "RTDevSandboxLauncherSubsystem.h"

#include "EditorModeManager.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "LevelEditor.h"
#include "Framework/Docking/TabManager.h"
#include "Misc/Paths.h"
#include "RTLauncherWorkspace.h"
#include "ScenarioHarness/RTScenarioAuthoring.h"
#include "ScenarioHarness/RTScenarioDraft.h"
#include "SRTLauncherScenarioPanel.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "RTDevSandboxLauncher"

// ⚠️ Category LOCALE, e non `LogRT` del modulo runtime. `LogRT` e' dichiarato in `RefactorTactics.h` senza
// `REFACTORTACTICS_API`, quindi il simbolo non attraversa il confine di modulo: misurato, il link fallisce
// con `LNK2001: LogRT non risolto`. E' anche la ragione — non detta finora — per cui `RTHexEditorMode.cpp`
// si definisce `LogRTHexEditorMode` invece di riusare quella del progetto.
DEFINE_LOG_CATEGORY(LogRTDevSandboxLauncher);

const FName URTDevSandboxLauncherSubsystem::TabId(TEXT("RTDevSandboxLauncher"));

bool URTDevSandboxLauncherSubsystem::InvokeTabInLayout(const FName InTabId)
{
	// ⛔ Il tab manager del **Level Editor** per primo, e il globale solo come uscita di sicurezza.
	//
	// La posizione del tab (#2168) e' dichiarata nel layout che il `LevelEditorTabManager` ripristina, e
	// quel manager e' un SUB-manager (`LevelEditor.cpp:815`). `AttemptToOpenTab` cerca il tab chiuso nelle
	// proprie `DockAreas`/`CollapsedDockAreas`: dal globale, la docking area del Level Editor non e'
	// raggiungibile. Il motore ha un ramo che sale dal sub-manager al globale (`TabManager.cpp:1778`) e
	// **nessuno** che scenda — quindi invocare dal globale un tab collocato nel layout del Level Editor
	// significa non trovarlo, e aprire una finestra.
	//
	// ⚠️ `GetModulePtr` e non `LoadModulePtr`: qui siamo a runtime, non in startup. Se il Level Editor non
	// e' caricato non c'e' nemmeno un layout in cui atterrare, e forzarne il caricamento per aprire un tab
	// sarebbe un effetto collaterale sproporzionato.
	if (const FLevelEditorModule* LevelEditor = FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor")))
	{
		if (const TSharedPtr<FTabManager> LevelEditorTabManager = LevelEditor->GetLevelEditorTabManager())
		{
			if (LevelEditorTabManager->TryInvokeTab(FTabId(InTabId)).IsValid())
			{
				return true;
			}
		}
	}

	// Il fallback non e' una resa: e' cio' che succede a chi ha il tab in una finestra salvata nell'ini, e
	// a chi apre l'editor senza Level Editor. Un pannello fluttuante e' meglio di nessun pannello.
	return FGlobalTabmanager::Get()->TryInvokeTab(FTabId(InTabId)).IsValid();
}

namespace
{
	/**
	 * Nome base del livello di bootstrap. E' un NOME, non un percorso, e la ragione sta in `ShouldOpenFor`:
	 * il delegate consegna percorsi in forme diverse per la stessa mappa.
	 *
	 * ⚠️ Resta allineato a `EditorStartupMap` in `Config/DefaultEngine.ini`. Se un giorno il livello di
	 * bootstrap cambiasse nome, questa costante e quella riga si muovono insieme — il test
	 * `OpensOnTheBootstrapLevel` fallisce se solo una delle due si muove.
	 */
	const TCHAR* const BootstrapLevelName = TEXT("L_DevSandbox");

	TSharedRef<SDockTab> SpawnLauncherTab(const FSpawnTabArgs&)
	{
		// L1 (#1680) apriva il tab su un segnaposto; #1705 lo riempie con la selezione sull'asse deciso da
		// #1681. Il tab resta di questa slice — quello che ci sta dentro no: `Start Session` e le superfici
		// del workspace sono #1682, e il pannello non le anticipa.
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
				SNew(SRTLauncherScenarioPanel)
			];
	}
}

bool URTDevSandboxLauncherSubsystem::ShouldOpenFor(const FString& MapFilename)
{
	// `FPaths::GetBaseFilename` toglie cartelle ed estensione, quindi le forme in cui l'editor consegna la
	// STESSA mappa collassano tutte sullo stesso nome: percorso assoluto, percorso relativo alla radice del
	// progetto, con o senza `.umap`. Confrontare la stringa intera darebbe esiti diversi per lo stesso livello.
	const FString BaseName = FPaths::GetBaseFilename(MapFilename);

	// Case-insensitive: su Windows lo stesso file si apre con maiuscole diverse, e due esiti per un solo
	// livello sarebbero un difetto invisibile finche' qualcuno non digita il percorso a mano.
	return BaseName.Equals(BootstrapLevelName, ESearchCase::IgnoreCase);
}

void URTDevSandboxLauncherSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FGlobalTabmanager::Get()
		->RegisterNomadTabSpawner(TabId, FOnSpawnTab::CreateStatic(&SpawnLauncherTab))
		.SetDisplayName(LOCTEXT("TabTitle", "Tactical Designer"))
		.SetTooltipText(LOCTEXT("TabTooltip", "Punto d'ingresso del workflow Tactical Designer."))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());

	// ⚠️ Il gancio: nessun polling, nessun Tick. Un `UEditorSubsystem` e' dinamico e si inizializza al
	// caricamento del modulo — misurato, alle 14:44:32 contro le 14:45:12 della mappa d'avvio — quindi
	// l'iscrizione e' in piedi prima del broadcast di startup e non serve un secondo gancio.
	//
	// ⛔ Ma il caricamento della mappa d'avvio ha QUATTRO cancelli a monte (`UnrealEdMisc.cpp:417-429`), e
	// vanno conosciuti senza combatterli — un ingresso che si apre quando l'engine ha deciso di non
	// caricare la mappa e' una seconda autorita' in miniatura:
	//
	//   (1) `bDoAutomatedMapBuild`                     build automatizzata: nessuno guarda, ed e' corretto.
	//   (2) `bMapLoaded`                               una mappa passata da riga di comando ha gia' caricato,
	//                                                  e la mappa di default non si carica affatto.
	//   (3) `OnEditorLoadDefaultStartupMap`            esiste apposta perche' un listener ANNULLI il
	//                                                  caricamento: se qualcuno lo fa, il launcher non
	//                                                  compare e il sintomo punta a questo gancio, che e'
	//                                                  il posto sbagliato dove cercare.
	//   (4) `LoadLevelAtStartup != None`               preferenza per-utente, ed e' anche la via d'uscita
	//                                                  nativa di chi apre Unreal per lavorare su altro.
	MapOpenedHandle = FEditorDelegates::OnMapOpened.AddUObject(this, &URTDevSandboxLauncherSubsystem::HandleMapOpened);
}

FRTLauncherStartDecision URTDevSandboxLauncherSubsystem::StartSession(const FString& ScenarioId)
{
	// ⛔ La sessione precedente si chiude PRIMA di provarne una nuova. Senza, un tentativo fallito
	// lascerebbe aperta quella di prima mentre il pannello annuncia un rifiuto: due verita' sullo schermo.
	EndSession();

	if (ScenarioId.IsEmpty())
	{
		// La decisione conosce gia' questo caso, e passargli un esito inventato sarebbe peggio che
		// lasciarglielo classificare: `Success` qui vorrebbe dire «la facade ha aperto», e non e' successo.
		return FRTLauncherWorkspace::DecideStart(ScenarioId, ERTScenarioAuthoringResult::Success, FString());
	}

	URTScenarioAuthoring* Candidate = URTScenarioAuthoring::CreateScenarioDraft(GetTransientPackage());
	if (!Candidate)
	{
		return FRTLauncherWorkspace::DecideStart(ScenarioId, ERTScenarioAuthoringResult::RunFailed,
			TEXT("la facade d'authoring non e' disponibile: il difetto non e' nello scenario."));
	}

	FString OpenError;
	const ERTScenarioAuthoringResult Result = Candidate->OpenById(ScenarioId, OpenError);

	const FRTLauncherStartDecision Decision = FRTLauncherWorkspace::DecideStart(ScenarioId, Result, OpenError);
	if (!Decision.bAllowed)
	{
		// Niente sessione mezza aperta: la facade candidata viene chiusa e lasciata al GC.
		Candidate->Close();
		return Decision;
	}

	Session = Candidate;

	UE_LOG(LogRTDevSandboxLauncher, Log, TEXT("[TacticalDesigner] sessione aperta su '%s'."), *ScenarioId);
	return Decision;
}

FRTLauncherStartDecision URTDevSandboxLauncherSubsystem::StartNewSession(const FString& ScenarioId, int32 MapRadius)
{
	EndSession();

	if (ScenarioId.IsEmpty())
	{
		return FRTLauncherWorkspace::DecideStart(ScenarioId, ERTScenarioAuthoringResult::Success, FString());
	}

	URTScenarioAuthoring* Candidate = URTScenarioAuthoring::CreateScenarioDraft(GetTransientPackage());
	if (!Candidate)
	{
		return FRTLauncherWorkspace::DecideStart(ScenarioId, ERTScenarioAuthoringResult::RunFailed,
			TEXT("la facade d'authoring non e' disponibile: il difetto non e' nello scenario."));
	}

	// ⚠️ Dalla facade, non da una costruzione locale: e' l'AC che impedisce al launcher di diventare una
	// seconda porta verso il modello. `NewScenario` non ritorna un esito perche' non puo' fallire — poi si
	// chiede a `Validate` se cio' che e' nato regge, ed e' quella la risposta che il designer legge.
	Candidate->NewScenario(ScenarioId, MapRadius);

	FString ValidateError;
	const ERTScenarioAuthoringResult Result = Candidate->Validate(ValidateError);

	const FRTLauncherStartDecision Decision = FRTLauncherWorkspace::DecideStart(ScenarioId, Result, ValidateError);
	if (!Decision.bAllowed)
	{
		Candidate->Close();
		return Decision;
	}

	Session = Candidate;

	UE_LOG(LogRTDevSandboxLauncher, Log, TEXT("[TacticalDesigner] sessione nuova su '%s' (raggio %d)."), *ScenarioId, MapRadius);
	return Decision;
}

void URTDevSandboxLauncherSubsystem::EndSession()
{
	if (Session)
	{
		Session->Close();
		Session = nullptr;
	}
}

bool URTDevSandboxLauncherSubsystem::HasSession() const
{
	// ⚠️ Due condizioni, non una: la facade puo' esistere e non avere niente di aperto — `Close()` la
	// lascia viva. Chiedere solo il puntatore direbbe «sessione aperta» su un draft vuoto.
	return Session != nullptr && Session->IsOpen();
}

bool URTDevSandboxLauncherSubsystem::ActivateSurface(FName SurfaceKey)
{
	const FRTLauncherSurface* Surface = FRTLauncherWorkspace::Find(SurfaceKey);
	if (!Surface || !Surface->bDeclared)
	{
		// ⛔ Rifiuta e lo dice. Un `return` muto qui sarebbe un pulsante inerte, cioe' il modo piu' rapido
		// di far concludere che lo strumento e' rotto quando la superficie semplicemente non esiste ancora.
		UE_LOG(LogRTDevSandboxLauncher, Warning,
			TEXT("[TacticalDesigner] superficie '%s' non dichiarata: non e' raggiungibile."), *SurfaceKey.ToString());
		return false;
	}

	switch (Surface->ActivationKind)
	{
	case ERTLauncherActivationKind::EditorMode:
		if (GLevelEditorModeTools().IsModeActive(Surface->ActivationTarget))
		{
			return true;
		}
		GLevelEditorModeTools().ActivateMode(Surface->ActivationTarget);
		return GLevelEditorModeTools().IsModeActive(Surface->ActivationTarget);

	case ERTLauncherActivationKind::Tab:
		return InvokeTabInLayout(Surface->ActivationTarget);

	default:
		return false;
	}
}

void URTDevSandboxLauncherSubsystem::Deinitialize()
{
	// La sessione non sopravvive allo scarico del modulo: un draft aperto su un editor che sta chiudendo
	// non ha nessuno che lo legga, e lasciarlo aperto e' l'unico modo di far comparire un `Close()` a GC.
	EndSession();

	// Togliere l'iscrizione PRIMA di sparire: un handle che sopravvive allo scarico del modulo fa
	// chiamare un metodo su un oggetto che non c'e' piu'.
	if (MapOpenedHandle.IsValid())
	{
		FEditorDelegates::OnMapOpened.Remove(MapOpenedHandle);
		MapOpenedHandle.Reset();
	}

	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabId);
	}

	Super::Deinitialize();
}

void URTDevSandboxLauncherSubsystem::HandleMapOpened(const FString& Filename, bool bAsTemplate)
{
	// `bAsTemplate` e' vero quando il livello viene aperto COME MODELLO: l'editor ne fa una copia senza
	// titolo, e cio' che si sta modificando non e' `L_DevSandbox` ma un livello nuovo che le somiglia.
	// L'ingresso non si presenta: il bootstrap environment e' il livello, non la sua forma.
	if (bAsTemplate)
	{
		return;
	}

	if (!ShouldOpenFor(Filename))
	{
		return;
	}

	UE_LOG(LogRTDevSandboxLauncher, Log, TEXT("[TacticalDesigner] livello di bootstrap aperto (%s): presento il launcher."), *Filename);

	InvokeTabInLayout(TabId);
}

#undef LOCTEXT_NAMESPACE
