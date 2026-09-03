#include "RefactorTacticsEditorModule.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/LayoutExtender.h"
#include "LevelEditor.h"
#include "RTDevSandboxLauncherSubsystem.h"
#include "RTHexEditorModeCommands.h"

#define LOCTEXT_NAMESPACE "FRefactorTacticsEditorModule"

// ⚠️ Stessa categoria del subsystem, e non una nuova: il `done_when` di `U31` dice all'operatore di
// cercare `LogRTDevSandboxLauncher`. Una diagnostica del launcher stampata sotto un'altra categoria
// finirebbe fuori dal filtro di chi sta indagando proprio su un pannello che non compare.
DEFINE_LOG_CATEGORY_STATIC(LogRTDevSandboxLauncher, Log, All);

void FRefactorTacticsEditorModule::ExtendLevelEditorLayout(FLayoutExtender& Extender)
{
	// ⛔ `ClosedTab`, non `OpenedTab`, ed e' la riga su cui poggia il contratto di #1680.
	//
	// `OpenedTab` aprirebbe il pannello a OGNI avvio, su OGNI livello e per CHIUNQUE apra il progetto —
	// cioe' addosso a chi ha aperto l'editor per correggere un materiale. E' esattamente cio' che #1680 ha
	// deciso di non fare, ed e' nel titolo di quella issue. `ClosedTab` da' al tab un **posto** senza
	// aprirlo; ad aprirlo resta `HandleMapOpened`, che lo fa solo su `L_DevSandbox` e che questa slice non
	// tocca. Il `TryInvokeTab` di li' ora trova una posizione e ci apre dentro, invece di creare una finestra.
	//
	// ⚠️ Se un giorno qualcuno cambiasse questa costante in `OpenedTab` per «farlo vedere prima», starebbe
	// revocando #1680 di sponda: e' una decisione, e va scritta come tale.
	Extender.ExtendLayout(
		LevelEditorTabIds::LevelEditorSelectionDetails,
		ELayoutExtensionPosition::After,
		FTabManager::FTab(FTabId(URTDevSandboxLauncherSubsystem::TabId), ETabState::ClosedTab));
}

void FRefactorTacticsEditorModule::StartupModule()
{
	// Il mode si registra da solo via CDO (Info nel costruttore di URTHexEditorMode). Qui solo i comandi dei tool.
	FRTHexEditorModeCommands::Register();

	// ⛔ **Solo in editor interattivo**, e la guardia viene prima di tutto il resto.
	//
	// `LoadModulePtr` **carica** il modulo, non lo cerca soltanto: senza questa guardia ogni commandlet
	// — `RTBuildIconCatalog`, `RTBuildGrayboxMeshes`, `RTBuildPlaygroundPanel` — pagherebbe
	// `FLevelEditorModule::StartupModule` (comandi, tool menus, style) per una posizione di tab che
	// nessuno guardera' mai. ⚠️ **La stesura precedente diceva il contrario di cio' che faceva**: motivava
	// `Ptr` invece di `Checked` proprio con i commandlet, e intanto il modulo lo caricava lo stesso.
	//
	// Resta `LoadModulePtr` e non `GetModulePtr` per l'altra meta' del problema: l'ordine di caricamento
	// non e' garantito, questo modulo e' `LoadingPhase: Default` e puo' partire PRIMA di `LevelEditor`.
	// Un `GetModulePtr` darebbe `nullptr` e la feature morirebbe in silenzio.
	if (IsRunningCommandlet() || !FSlateApplication::IsInitialized())
	{
		return;
	}

	if (FLevelEditorModule* LevelEditor = FModuleManager::LoadModulePtr<FLevelEditorModule>(TEXT("LevelEditor")))
	{
		// ⛔ `OnRegisterLayoutExtensions` e' un broadcast **singolo**, dentro la costruzione del Level
		// Editor (`SLevelEditor.cpp:1815`). Chi si iscrive dopo non riceve niente — ed e' la ragione per
		// cui l'iscrizione sta qui e non in `URTDevSandboxLauncherSubsystem::Initialize`, che il
		// `TabId` invece possiede. Le due meta' del tab vivono in due file, e questo commento e' il ponte.
		LayoutExtensionHandle = LevelEditor->OnRegisterLayoutExtensions()
			.AddStatic(&FRefactorTacticsEditorModule::ExtendLevelEditorLayout);
	}
	else
	{
		UE_LOG(LogRTDevSandboxLauncher, Log,
			TEXT("[TacticalDesigner] modulo LevelEditor non disponibile: nessun layout da estendere."));
	}
}

void FRefactorTacticsEditorModule::ShutdownModule()
{
	FRTHexEditorModeCommands::Unregister();

	// Un handle che sopravvive allo scarico del modulo fa chiamare una funzione che non c'e' piu'.
	if (LayoutExtensionHandle.IsValid())
	{
		if (FLevelEditorModule* LevelEditor = FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor")))
		{
			LevelEditor->OnRegisterLayoutExtensions().Remove(LayoutExtensionHandle);
		}
		LayoutExtensionHandle.Reset();
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRefactorTacticsEditorModule, RefactorTacticsEditor)
