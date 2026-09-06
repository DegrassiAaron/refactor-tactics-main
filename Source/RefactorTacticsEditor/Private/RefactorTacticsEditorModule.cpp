#include "RefactorTacticsEditorModule.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/LayoutExtender.h"
#include "LevelEditor.h"
#include "RTDevSandboxLauncherSubsystem.h"
#include "RTHexEditorModeCommands.h"
#include "SRTAnimBrowserPanel.h"
#include "SRTLabPanel.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "FRefactorTacticsEditorModule"

// ⚠️ Stessa categoria del subsystem, e non una nuova: il `done_when` di `U31` dice all'operatore di
// cercare `LogRTDevSandboxLauncher`. Una diagnostica del launcher stampata sotto un'altra categoria
// finirebbe fuori dal filtro di chi sta indagando proprio su un pannello che non compare.
// La categoria e' DICHIARATA in `RTDevSandboxLauncherSubsystem.h` e definita nel suo `.cpp`.

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

	// Il browser delle animazioni (#2443). Stessa guardia di sopra e per la stessa ragione: un tab non lo
	// guarda nessun commandlet, e `RTBuildAnimBindings` gira proprio come commandlet.
	//
	// ⛔ `RegisterNomadTabSpawner` e non un'estensione di layout: questo pannello **non** deve avere una
	// posizione che lo apra da solo. Si apre da Window > Tools quando l'autore decide di rivedere delle
	// clip — e' la stessa disciplina di #1680, che ha deciso di non mettersi addosso a chi ha aperto
	// l'editor per un'altra ragione.
	FGlobalTabmanager::Get()
		->RegisterNomadTabSpawner(SRTAnimBrowserPanel::TabId, FOnSpawnTab::CreateLambda(
			[](const FSpawnTabArgs&)
			{
				return SNew(SDockTab)
					.TabRole(ETabRole::NomadTab)
					[ SNew(SRTAnimBrowserPanel) ];
			}))
		.SetDisplayName(LOCTEXT("AnimBrowserTitle", "Anim Browser"))
		.SetTooltipText(LOCTEXT("AnimBrowserTooltip",
			"Guarda le clip di un pack, promuovile o scartale, e legale a un ruolo."))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());

	// Il Lab (#2599 Fetta B, #2600). Stessa guardia e stessa forma del browser qui sopra: un tab non lo
	// guarda nessun commandlet.
	//
	// ⛔ Un pannello SOLO per due issue, e non e' un'economia: `ListHeroKit` **e'** un filtro su
	// `ListCanonicalAbilities`, e `BuildHeroFixture` verifica l'appartenenza e delega a `BuildFixture`.
	// Due tab distinti ripeterebbero selettore, readout, Run, before/after e vista del TurnLog — e i due
	// divergerebbero al primo campo aggiunto a uno solo.
	FGlobalTabmanager::Get()
		->RegisterNomadTabSpawner(SRTLabPanel::TabId, FOnSpawnTab::CreateLambda(
			[](const FSpawnTabArgs&)
			{
				return SNew(SDockTab)
					.TabRole(ETabRole::NomadTab)
					[ SNew(SRTLabPanel) ];
			}))
		.SetDisplayName(LOCTEXT("LabTitle", "Ability / Hero Lab"))
		.SetTooltipText(LOCTEXT("LabTooltip",
			"Esegui una ability canonica in una fixture deterministica. Filtra per eroe per vederne il kit."))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());

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

	// Uno spawner che sopravvive allo scarico del modulo fa costruire un widget di una classe che
	// non c'e' piu': stessa ragione dell'handle qui sotto.
	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SRTAnimBrowserPanel::TabId);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SRTLabPanel::TabId);
	}

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
