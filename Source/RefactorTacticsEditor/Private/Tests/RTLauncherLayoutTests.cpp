#include "Misc/AutomationTest.h"

#include "Framework/Docking/LayoutExtender.h"
#include "Framework/Docking/TabManager.h"
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"
#include "RefactorTacticsEditorModule.h"
#include "RTDevSandboxLauncherSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Dove sta il tab del Tactical Designer (#2168) — e perche' questo e' misurabile a macchina mentre
 * «il pannello compare» non lo e'.
 *
 * 🔑 L'intestazione di `RTDevSandboxLauncherTests.cpp` dice che *«che il pannello COMPAIA e' Slate su un
 * editor vivo: nessun automation test lo vede»*. E' vero per il **pixel**, e falso per il **layout**:
 * `FTabManager::FLayout` e `FLayoutExtender` sono oggetti puri: si costruiscono, si estendono e si
 * interrogano senza una finestra, senza un `FSlateApplication` e senza aprire un livello.
 *
 * ⛔ **Cosa questi test NON coprono.** Che il tab si veda **a destra**, in scheda con i Details, resta
 * occhio umano — la seduta `U31` in `editor-sessions.yaml`. Qui si misura che l'estensione dichiari la
 * posizione giusta e che si comporti bene su un layout gia' popolato; non che Slate la disegni.
 */

// ⚠️ Nomi con prefisso `LauncherLayout`: namespace anonimo + unity build. `RTPlaygroundLayoutTests.cpp:29`
// porta gia' la stessa nota, e `KitTolerance` in `RTGrayboxMeshTests` e' il caso in cui questo modulo ci e'
// finito dentro. Un `FindTab` generico qui e uno in un futuro file di test sui tab atterrano nello stesso
// blob e non compilano, con l'errore lontano da entrambi gli autori.
namespace
{
	/** Raccoglie ricorsivamente tutti gli stack di un nodo. `FArea` deriva da `FSplitter`, quindi entra dal secondo ramo. */
	void LauncherLayoutGatherStacks(const TSharedRef<FTabManager::FLayoutNode>& Node, TArray<TSharedRef<FTabManager::FStack>>& Out)
	{
		if (const TSharedPtr<FTabManager::FStack> Stack = Node->AsStack())
		{
			Out.Add(Stack.ToSharedRef());
			return;
		}

		if (const TSharedPtr<FTabManager::FSplitter> Splitter = Node->AsSplitter())
		{
			for (const TSharedRef<FTabManager::FLayoutNode>& Child : Splitter->GetChildNodes())
			{
				LauncherLayoutGatherStacks(Child, Out);
			}
		}
	}

	TArray<TSharedRef<FTabManager::FStack>> LauncherLayoutAllStacks(const TSharedRef<FTabManager::FLayout>& Layout)
	{
		TArray<TSharedRef<FTabManager::FStack>> Stacks;
		for (const TSharedRef<FTabManager::FArea>& Area : Layout->GetAreas())
		{
			LauncherLayoutGatherStacks(Area, Stacks);
		}
		return Stacks;
	}

	/** Quante volte il tab compare in TUTTO il layout: e' il numero che distingue «spostato» da «duplicato». */
	int32 LauncherLayoutCountTab(const TSharedRef<FTabManager::FLayout>& Layout, const FName TabType)
	{
		int32 Count = 0;
		for (const TSharedRef<FTabManager::FStack>& Stack : LauncherLayoutAllStacks(Layout))
		{
			for (const FTabManager::FTab& Tab : Stack->GetTabs())
			{
				if (Tab.TabId.TabType == TabType)
				{
					++Count;
				}
			}
		}
		return Count;
	}

	/** Lo stack che contiene un dato tab, o `nullptr`. Serve a chiedere *accanto a chi* e' finito. */
	TSharedPtr<FTabManager::FStack> LauncherLayoutStackContaining(const TSharedRef<FTabManager::FLayout>& Layout, const FName TabType)
	{
		for (const TSharedRef<FTabManager::FStack>& Stack : LauncherLayoutAllStacks(Layout))
		{
			for (const FTabManager::FTab& Tab : Stack->GetTabs())
			{
				if (Tab.TabId.TabType == TabType)
				{
					return Stack;
				}
			}
		}
		return nullptr;
	}

	/** L'indice del tab nello stack, o `INDEX_NONE`. E' il solo modo di distinguere `After` da `Before`. */
	int32 LauncherLayoutIndexOf(const TSharedPtr<FTabManager::FStack>& Stack, const FName TabType)
	{
		if (!Stack.IsValid())
		{
			return INDEX_NONE;
		}

		const TConstArrayView<FTabManager::FTab> Tabs = Stack->GetTabs();
		for (int32 Index = 0; Index < Tabs.Num(); ++Index)
		{
			if (Tabs[Index].TabId.TabType == TabType)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	const FTabManager::FTab* LauncherLayoutFindTab(const TSharedPtr<FTabManager::FStack>& Stack, const FName TabType)
	{
		if (!Stack.IsValid())
		{
			return nullptr;
		}

		for (const FTabManager::FTab& Tab : Stack->GetTabs())
		{
			if (Tab.TabId.TabType == TabType)
			{
				return &Tab;
			}
		}
		return nullptr;
	}

	/**
	 * La colonna destra del Level Editor in miniatura: due stack, come nel layout vero
	 * (`SLevelEditor.cpp:1766-1782`). Solo i tab che contano per questa misura.
	 */
	TSharedRef<FTabManager::FLayout> LauncherLayoutMakeRightColumn()
	{
		return FTabManager::NewLayout(TEXT("RT_LauncherLayoutTest"))
			->AddArea
			(
				FTabManager::NewPrimaryArea()
				->SetOrientation(Orient_Vertical)
				->Split
				(
					FTabManager::NewStack()
					->AddTab(LevelEditorTabIds::LevelEditorSceneOutliner, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->AddTab(LevelEditorTabIds::LevelEditorSelectionDetails, ETabState::OpenedTab)
					->AddTab(LevelEditorTabIds::WorldSettings, ETabState::ClosedTab)
				)
			);
	}
}

/**
 * Il caso per cui la slice esiste: dopo l'estensione, il launcher ha un posto — ed e' quello dei Details.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLauncherLayoutDocksTest,
	"RefactorTactics.DevSandboxLauncher.LayoutExtensionDocksTheLauncher",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLauncherLayoutDocksTest::RunTest(const FString&)
{
	const TSharedRef<FTabManager::FLayout> Layout = LauncherLayoutMakeRightColumn();

	// La premessa, misurata invece che assunta: prima dell'estensione il tab NON e' nel layout. Senza
	// questa riga il test resterebbe verde anche se `LauncherLayoutMakeRightColumn` lo avesse gia' dentro per
	// sbaglio, e non misurerebbe piu' l'estensione ma se stesso.
	TestEqual(TEXT("prima dell'estensione il launcher non e' nel layout"),
		LauncherLayoutCountTab(Layout, URTDevSandboxLauncherSubsystem::TabId), 0);

	FLayoutExtender Extender;
	FRefactorTacticsEditorModule::ExtendLevelEditorLayout(Extender);
	Layout->ProcessExtensions(Extender);

	TestEqual(TEXT("dopo l'estensione il launcher compare una volta sola"),
		LauncherLayoutCountTab(Layout, URTDevSandboxLauncherSubsystem::TabId), 1);

	// ⚠️ Non basta che ci sia: deve stare ACCANTO AI DETAILS. Un tab finito nello stack dell'Outliner
	// passerebbe un test di sola presenza, e sarebbe la posizione sbagliata.
	const TSharedPtr<FTabManager::FStack> Host =
		LauncherLayoutStackContaining(Layout, URTDevSandboxLauncherSubsystem::TabId);

	TestTrue(TEXT("il launcher e' nello stack che contiene i Details"),
		LauncherLayoutFindTab(Host, LevelEditorTabIds::LevelEditorSelectionDetails) != nullptr);

	TestFalse(TEXT("e non in quello dell'Outliner"),
		LauncherLayoutFindTab(Host, LevelEditorTabIds::LevelEditorSceneOutliner) != nullptr);

	// ⚠️ E nemmeno basta lo stack giusto: `After` e `Before` mettono il tab in due posti diversi dello
	// STESSO stack, e un test che guarda solo l'appartenenza resta verde su entrambi. Qui si chiede
	// l'ordine — il launcher DOPO i Details — che e' la meta' di R-1 che la prosa dichiarava e nessuna
	// asserzione teneva ferma.
	const int32 DetailsIndex = LauncherLayoutIndexOf(Host, LevelEditorTabIds::LevelEditorSelectionDetails);
	const int32 LauncherIndex = LauncherLayoutIndexOf(Host, URTDevSandboxLauncherSubsystem::TabId);
	TestTrue(TEXT("il launcher segue i Details nello stack, non li precede"),
		DetailsIndex != INDEX_NONE && LauncherIndex == DetailsIndex + 1);

	// ⛔ Il contratto di #1680 in una riga. `OpenedTab` aprirebbe il pannello su ogni livello e per
	// chiunque: e' il tab ad avere un posto, non il pannello ad aprirsi.
	const FTabManager::FTab* const Tab = LauncherLayoutFindTab(Host, URTDevSandboxLauncherSubsystem::TabId);
	if (TestNotNull(TEXT("il tab esteso e' leggibile"), Tab))
	{
		TestEqual(TEXT("il launcher e' dichiarato CHIUSO: ad aprirlo resta HandleMapOpened"),
			static_cast<int32>(Tab->TabState), static_cast<int32>(ETabState::ClosedTab));
	}

	return true;
}

/**
 * Il limite, non la funzione — ed e' il test che conta di piu'.
 *
 * `FTabManager::FLayout::ProcessExtensions` inserisce un tab esteso solo `if (!AllTabs.Contains(...))`
 * (`TabManager.cpp:674`), e `AllTabs` viene raccolto dal layout **gia' caricato da `EditorLayout.ini`**
 * (`SLevelEditor.cpp:1789`, prima del `ProcessExtensions` a `1816`).
 *
 * ⛔ **La conseguenza va asserita, non solo scritta**: chi ha aperto il launcher anche una volta ha la
 * voce nel proprio ini, e l'estensione **non lo sposta**. Il difetto sopravvive esattamente per chi ce
 * l'ha, il sintomo e' *«a me funziona»*, e la via d'uscita e' `Window -> Load Layout -> Default Editor
 * Layout`. Questo test e' il posto in cui quel limite e' misurato invece che ricordato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLauncherLayoutRespectsSavedTabTest,
	"RefactorTactics.DevSandboxLauncher.LayoutExtensionRespectsASavedTab",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLauncherLayoutRespectsSavedTabTest::RunTest(const FString&)
{
	// Un layout come quello di chi ha gia' usato il launcher: STESSA baseline del test sopra, e il tab
	// aggiunto dove l'ini se l'era salvato — lo stack dell'Outliner, al posto della finestra fluttuante.
	//
	// ⚠️ Riusare `LauncherLayoutMakeRightColumn()` invece di riscrivere il layout a mano e' cio' che tiene
	// i due test confrontabili: la differenza dichiarata fra loro e' **una sola**, e deve restare una sola
	// anche dopo che qualcuno avra' arricchito la baseline.
	const TSharedRef<FTabManager::FLayout> Layout = LauncherLayoutMakeRightColumn();
	LauncherLayoutAllStacks(Layout)[0]->AddTab(URTDevSandboxLauncherSubsystem::TabId, ETabState::OpenedTab);

	FLayoutExtender Extender;
	FRefactorTacticsEditorModule::ExtendLevelEditorLayout(Extender);
	Layout->ProcessExtensions(Extender);

	// Il difetto peggiore sarebbe la DUPLICAZIONE: due schede con lo stesso nome, e un pannello che si
	// apre in una delle due a caso. Il motore lo evita, e questa riga lo tiene fermo.
	TestEqual(TEXT("nessun duplicato: il tab resta uno"),
		LauncherLayoutCountTab(Layout, URTDevSandboxLauncherSubsystem::TabId), 1);

	// E la meta' amara: NON viene spostato accanto ai Details. E' il limite dichiarato in #2168 R-3.
	const TSharedPtr<FTabManager::FStack> Host =
		LauncherLayoutStackContaining(Layout, URTDevSandboxLauncherSubsystem::TabId);

	TestTrue(TEXT("il tab salvato resta dov'era"),
		LauncherLayoutFindTab(Host, LevelEditorTabIds::LevelEditorSceneOutliner) != nullptr);

	TestFalse(TEXT("l'estensione NON lo sposta accanto ai Details: serve ripristinare il layout"),
		LauncherLayoutFindTab(Host, LevelEditorTabIds::LevelEditorSelectionDetails) != nullptr);

	return true;
}

/**
 * Che qualcuno RICEVA l'estensione, e non solo che l'estensione sia giusta.
 *
 * ⛔ I due test sopra chiamano `ExtendLevelEditorLayout` **direttamente**: misurano il contenuto
 * dell'extender, non il fatto che il Level Editor lo interroghi. Cancellata la riga `AddStatic` in
 * `StartupModule`, resterebbero entrambi verdi e la feature morirebbe in silenzio — esattamente il modo
 * che il commento su quella riga dichiara di temere.
 *
 * E' lo stesso buco che `SubscribesOnInitialize` chiude per l'altro gancio del launcher, quello su
 * `OnMapOpened`: senza questo test le due meta' della feature avrebbero due discipline diverse.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLauncherLayoutIsRegisteredTest,
	"RefactorTactics.DevSandboxLauncher.LayoutExtensionIsRegisteredOnStartup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLauncherLayoutIsRegisteredTest::RunTest(const FString&)
{
	const FRefactorTacticsEditorModule* const Module =
		FModuleManager::GetModulePtr<FRefactorTacticsEditorModule>(TEXT("RefactorTacticsEditor"));

	if (!TestNotNull(TEXT("il modulo editor e' caricato"), Module))
	{
		return false;
	}

	TestTrue(TEXT("il modulo e' iscritto a OnRegisterLayoutExtensions"),
		Module->IsLayoutExtensionRegistered());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
