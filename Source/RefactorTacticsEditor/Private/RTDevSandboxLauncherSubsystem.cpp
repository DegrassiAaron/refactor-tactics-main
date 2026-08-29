#include "RTDevSandboxLauncherSubsystem.h"

#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Misc/Paths.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Text/STextBlock.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "RTDevSandboxLauncher"

// ⚠️ Category LOCALE, e non `LogRT` del modulo runtime. `LogRT` e' dichiarato in `RefactorTactics.h` senza
// `REFACTORTACTICS_API`, quindi il simbolo non attraversa il confine di modulo: misurato, il link fallisce
// con `LNK2001: LogRT non risolto`. E' anche la ragione — non detta finora — per cui `RTHexEditorMode.cpp`
// si definisce `LogRTHexEditorMode` invece di riusare quella del progetto.
DEFINE_LOG_CATEGORY_STATIC(LogRTDevSandboxLauncher, Log, All);

const FName URTDevSandboxLauncherSubsystem::TabId(TEXT("RTDevSandboxLauncher"));

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
		// Slice L1 (#1680): questa slice consegna un pannello che si apre al momento giusto e NON mostra
		// ancora niente. Gli assi di selezione sono #1681, le superfici del workspace #1682.
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
				SNew(STextBlock)
					.Text(LOCTEXT("Placeholder", "Tactical Designer — la selezione arriva con #1681."))
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

void URTDevSandboxLauncherSubsystem::Deinitialize()
{
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

	FGlobalTabmanager::Get()->TryInvokeTab(TabId);
}

#undef LOCTEXT_NAMESPACE
