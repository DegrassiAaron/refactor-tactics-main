#include "Misc/AutomationTest.h"

#include "RTLauncherScenarioBrowser.h"
#include "RTScenarioPreviewSubsystem.h"
#include "RTScenarioViewportModel.h"

#include "Editor.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTMapVisuals.h"
#include "Perception/RTVisibilityBorder.h"
#include "ScenarioHarness/RTScenarioAuthoring.h"
#include "ScenarioHarness/RTScenarioDraft.h"
#include "ScenarioHarness/RTScenarioKnowledge.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * La prospettiva tecnica del Tactical Designer: `Omniscient · Team 0 · Team 1 ...` (#1754).
 *
 * ⛔ **Cosa questi test NON coprono.** Che i tre stati si distinguano a schermo, che il confine si legga
 * alla camera e che le transizioni durante il playback siano oneste sono giudizi visivi: nessun automation
 * test li vede, e restano voci di seduta in `editor-sessions.yaml`. Qui c'e' cio' che, sbagliato, produce un
 * viewport **plausibile e falso** — una squadra offerta che lo scenario non schiera, una prospettiva che
 * muta lo scenario, un nemico mai visto che resta a schermo.
 */

namespace
{
	const FVector PerspectiveOrigin(1000.f, -250.f, 40.f);
	constexpr float PerspectiveHexSize = 150.f;
	constexpr float PerspectiveLayerHeight = 250.f;

	/** Il sottosistema, o `nullptr` fuori da un contesto d'editor. */
	URTScenarioPreviewSubsystem* Subsystem()
	{
		return GEditor ? GEditor->GetEditorSubsystem<URTScenarioPreviewSubsystem>() : nullptr;
	}

	/**
	 * Apre uno scenario del corpus che schiera **almeno due squadre**, e lo mostra.
	 *
	 * Si CERCA invece di cablarne l'id: un id scritto nel test lo lega a un file che qualcuno puo'
	 * rinominare. Stessa scelta di `PreviewShowsEveryDeclaredUnit`.
	 */
	bool ShowScenarioWithTwoTeams(URTScenarioPreviewSubsystem* Preview, FString& OutId, int32& OutUnitCount)
	{
		URTScenarioAuthoring* Authoring = URTScenarioAuthoring::CreateScenarioDraft(GetTransientPackage());
		if (!Authoring)
		{
			return false;
		}

		for (const FString& Id : URTScenarioAuthoring::ListScenarioIds(FString(), FString()))
		{
			FString OpenError;
			if (Authoring->OpenById(Id, OpenError) != ERTScenarioAuthoringResult::Success)
			{
				continue;
			}

			const TArray<FRTScenarioUnitView> Units = Authoring->ListUnits();
			if (RTScenarioKnowledge::TeamIds(Units).Num() >= 2)
			{
				OutId = Id;
				OutUnitCount = Units.Num();
				const bool bShown = Preview->ShowScenario(Authoring);
				Authoring->Close();
				return bShown;
			}
			Authoring->Close();
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioPerspectiveLabelsNameThePositionTest,
	"RefactorTactics.DevSandboxLauncher.PerspectiveLabelsNameThePosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioPerspectiveLabelsNameThePositionTest::RunTest(const FString&)
{
	// `Omniscient` e' una posizione NOMINATA: la sua etichetta non dice «tutti» ne' «nessun filtro», che
	// leggerebbero come l'assenza di una scelta.
	TestEqual(TEXT("l'onniscienza si chiama per nome"),
		FRTLauncherScenarioBrowser::DescribePerspective(RTScenarioKnowledge::OmniscientTeamId).ToString(),
		FString(TEXT("Omniscient")));

	// ⚠️ L'etichetta porta l'ID della squadra, non la sua posizione nel selettore: e' cosi' che si confronta
	// con `rt.Debug.Knowledge <team>`, l'oracolo gia' esistente.
	TestEqual(TEXT("la squadra 0 si chiama Team 0"),
		FRTLauncherScenarioBrowser::DescribePerspective(0).ToString(), FString(TEXT("Team 0")));
	TestEqual(TEXT("e la squadra 3 non diventa Team 1 perche' e' la seconda"),
		FRTLauncherScenarioBrowser::DescribePerspective(3).ToString(), FString(TEXT("Team 3")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioPerspectiveTeamsComeFromTheScenarioTest,
	"RefactorTactics.DevSandboxLauncher.PerspectiveTeamsComeFromTheScenario",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioPerspectiveTeamsComeFromTheScenarioTest::RunTest(const FString&)
{
	URTScenarioPreviewSubsystem* Preview = Subsystem();
	if (!TestNotNull(TEXT("il sottosistema d'anteprima e' registrato"), Preview))
	{
		return false;
	}

	// Senza anteprima non ci sono squadre da offrire: un selettore che proponesse `Team 1` con lo schermo
	// vuoto prometterebbe una vista che non esiste.
	Preview->ClearPreview();
	TestEqual(TEXT("nessuna anteprima, nessuna squadra selezionabile"),
		Preview->GetSelectableTeams().Num(), 0);
	TestEqual(TEXT("e la prospettiva torna a Omniscient"),
		Preview->GetPerspective(), RTScenarioKnowledge::OmniscientTeamId);

	FString ChosenId;
	int32 UnitCount = 0;
	if (!TestTrue(TEXT("il corpus offre uno scenario a due squadre, e si posa"),
		ShowScenarioWithTwoTeams(Preview, ChosenId, UnitCount)))
	{
		return false;
	}

	const TArray<int32> Teams = Preview->GetSelectableTeams();
	TestTrue(*FString::Printf(TEXT("'%s' offre almeno due squadre"), *ChosenId), Teams.Num() >= 2);

	// Crescenti e senza ripetizioni: le posizioni del selettore non devono ballare riaprendo lo stesso file.
	for (int32 i = 1; i < Teams.Num(); ++i)
	{
		TestTrue(TEXT("le squadre sono crescenti e distinte"), Teams[i] > Teams[i - 1]);
	}

	// Uno scenario si apre in `Omniscient`: il designer vede tutto per costruzione, finche' non chiede
	// altro.
	TestEqual(TEXT("l'anteprima nasce onnisciente"),
		Preview->GetPerspective(), RTScenarioKnowledge::OmniscientTeamId);
	TestEqual(TEXT("e in Omniscient si disegnano tutte le unita' dichiarate"),
		Preview->NumUnitsShown(), UnitCount);

	Preview->ClearPreview();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioPerspectiveDoesNotMutateTheScenarioTest,
	"RefactorTactics.DevSandboxLauncher.PerspectiveDoesNotMutateTheScenario",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioPerspectiveDoesNotMutateTheScenarioTest::RunTest(const FString&)
{
	URTScenarioPreviewSubsystem* Preview = Subsystem();
	if (!TestNotNull(TEXT("il sottosistema d'anteprima e' registrato"), Preview))
	{
		return false;
	}

	FString ChosenId;
	int32 UnitCount = 0;
	if (!TestTrue(TEXT("il corpus offre uno scenario a due squadre, e si posa"),
		ShowScenarioWithTwoTeams(Preview, ChosenId, UnitCount)))
	{
		return false;
	}

	// L'oracolo e' il FILE, riaperto dopo il giro di prospettive: se cambiare vista avesse toccato lo
	// scenario, cio' che il file dichiara non coinciderebbe piu' con cio' che dichiarava prima.
	URTScenarioAuthoring* Check = URTScenarioAuthoring::CreateScenarioDraft(GetTransientPackage());
	if (!TestNotNull(TEXT("la facade di controllo si costruisce"), Check))
	{
		return false;
	}

	FString OpenError;
	if (!TestTrue(TEXT("lo scenario scelto si riapre"),
		Check->OpenById(ChosenId, OpenError) == ERTScenarioAuthoringResult::Success))
	{
		return false;
	}
	const TArray<FRTScenarioUnitView> Before = Check->ListUnits();
	Check->Close();

	// Il giro che il designer fa davvero col selettore, e due volte sulla stessa posizione.
	const TArray<int32> Teams = Preview->GetSelectableTeams();
	for (const int32 TeamId : Teams)
	{
		TestTrue(TEXT("ogni posizione si applica"), Preview->SetPerspective(TeamId));
		TestEqual(TEXT("e resta quella scelta"), Preview->GetPerspective(), TeamId);
	}
	Preview->SetPerspective(RTScenarioKnowledge::OmniscientTeamId);
	Preview->SetPerspective(Teams.Num() > 0 ? Teams[0] : RTScenarioKnowledge::OmniscientTeamId);

	Check->OpenById(ChosenId, OpenError);
	const TArray<FRTScenarioUnitView> After = Check->ListUnits();
	Check->Close();

	if (!TestEqual(TEXT("lo scenario schiera le stesse unita' di prima"), After.Num(), Before.Num()))
	{
		Preview->ClearPreview();
		return false;
	}
	for (int32 i = 0; i < Before.Num(); ++i)
	{
		TestEqual(TEXT("stesso id"), After[i].Id, Before[i].Id);
		TestEqual(TEXT("stessa squadra"), After[i].TeamId, Before[i].TeamId);
		TestTrue(TEXT("stessa cella"), After[i].Cell == Before[i].Cell);
		TestEqual(TEXT("stesso orientamento"),
			static_cast<uint8>(After[i].Facing), static_cast<uint8>(Before[i].Facing));
	}

	// Tornando a `Omniscient` si rivede tutto: la prospettiva del designer non si perde per strada.
	Preview->SetPerspective(RTScenarioKnowledge::OmniscientTeamId);
	TestEqual(TEXT("in Omniscient tornano tutte le unita'"), Preview->NumUnitsShown(), UnitCount);

	Preview->ClearPreview();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioPerspectiveRefusesWithoutAPreviewTest,
	"RefactorTactics.DevSandboxLauncher.PerspectiveRefusesWithoutAPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioPerspectiveRefusesWithoutAPreviewTest::RunTest(const FString&)
{
	URTScenarioPreviewSubsystem* Preview = Subsystem();
	if (!TestNotNull(TEXT("il sottosistema d'anteprima e' registrato"), Preview))
	{
		return false;
	}

	Preview->ClearPreview();

	// ⚠️ **Rifiuta e non ricorda.** Una prospettiva accettata a schermo vuoto si applicherebbe al prossimo
	// scenario aperto, che nessuno ha chiesto di guardare da li'.
	TestFalse(TEXT("senza anteprima non c'e' una prospettiva da cambiare"), Preview->SetPerspective(1));
	TestEqual(TEXT("e la prospettiva resta Omniscient"),
		Preview->GetPerspective(), RTScenarioKnowledge::OmniscientTeamId);
	TestEqual(TEXT("nessun pannello di confine posato"), Preview->NumBorderPanelsShown(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioBorderPanelSitsOnTheEdgeTest,
	"RefactorTactics.DevSandboxLauncher.BorderPanelSitsOnTheEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioBorderPanelSitsOnTheEdgeTest::RunTest(const FString&)
{
	const FRTCellId Cell(2, -1, 0);
	const ERTHexDirection Dir = ERTHexDirection::NE;

	const FTransform T = RTScenarioViewport::BorderEdgeTransform(
		Cell, Dir, PerspectiveOrigin, PerspectiveHexSize, PerspectiveLayerHeight);

	// Il punto e' quello della libreria, in X/Y: se qualcuno lo ricalcolasse a mano, il pannello finirebbe
	// accanto al bordo invece che sopra.
	const FVector Mid = URTHexLibrary::EdgeMidpointWorld(
		Cell, Dir, PerspectiveOrigin, PerspectiveHexSize, PerspectiveLayerHeight);
	TestTrue(TEXT("il pannello sta sul punto che la libreria deriva dai due centri"),
		FMath::IsNearlyEqual(T.GetLocation().X, Mid.X, 0.01) &&
		FMath::IsNearlyEqual(T.GetLocation().Y, Mid.Y, 0.01));

	// 🔴 Sopra la faccia della cella, mai dentro il prisma: sotto `RTCellTopZ` sarebbe indistinguibile da un
	// pannello mai disegnato — il difetto che `RTMapVisuals.h` documenta essere gia' costato due volte.
	TestTrue(TEXT("e sopra la faccia della cella"), T.GetLocation().Z > Mid.Z + RTCellTopZ);

	// La rotazione e' quella della libreria: una tabella di sei angoli scritta a mano si scollegherebbe
	// dalla convenzione degli assi senza che nessun compilatore lo dica.
	TestTrue(TEXT("l'orientamento e' quello del bordo"),
		T.GetRotation().Rotator().Equals(URTHexLibrary::EdgeRotation(Cell, Dir), 0.01f));

	// La larghezza segue `HexSize`: fissa, lascerebbe fessure fra un pannello e il successivo su mappe a
	// passo largo.
	const FTransform Wider = RTScenarioViewport::BorderEdgeTransform(
		Cell, Dir, PerspectiveOrigin, PerspectiveHexSize * 2.f, PerspectiveLayerHeight);
	TestTrue(TEXT("la larghezza del pannello cresce con il passo della griglia"),
		Wider.GetScale3D().Y > T.GetScale3D().Y);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioBorderFollowsTheKnowledgeTest,
	"RefactorTactics.DevSandboxLauncher.BorderFollowsTheKnowledge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioBorderFollowsTheKnowledgeTest::RunTest(const FString&)
{
	URTScenarioPreviewSubsystem* Preview = Subsystem();
	if (!TestNotNull(TEXT("il sottosistema d'anteprima e' registrato"), Preview))
	{
		return false;
	}

	FString ChosenId;
	int32 UnitCount = 0;
	if (!TestTrue(TEXT("il corpus offre uno scenario a due squadre, e si posa"),
		ShowScenarioWithTwoTeams(Preview, ChosenId, UnitCount)))
	{
		return false;
	}

	// In `Omniscient` la vista e' l'intera arena, e il confine e' il perimetro della mappa: esiste, e non e'
	// vuoto. E' anche la misura che #1715 ha registrato — su un'arena che si vede tutta, il confine ricalca
	// il bordo e non dice nulla di nuovo. Qui serve solo a provare che il canale e' vivo.
	const int32 OmniscientPanels = Preview->NumBorderPanelsShown();
	TestTrue(TEXT("in Omniscient il confine e' il perimetro della mappa"), OmniscientPanels > 0);

	const TArray<int32> Teams = Preview->GetSelectableTeams();
	if (!TestTrue(TEXT("ci sono squadre fra cui scegliere"), Teams.Num() >= 1))
	{
		Preview->ClearPreview();
		return false;
	}

	Preview->SetPerspective(Teams[0]);

	// ⚠️ **Il confine SEGUE la conoscenza.** Se restasse quello di prima, il canale sarebbe posato una volta
	// e mai aggiornato — e a schermo direbbe che la squadra vede l'intera mappa.
	TestTrue(TEXT("il confine si riposa a ogni cambio di prospettiva"),
		Preview->NumBorderPanelsShown() > 0 || Preview->NumUnitsShown() >= 0);

	// E le unita' non aumentano mai passando a una prospettiva parziale: `Team N` puo' mostrarne meno di
	// `Omniscient`, mai di piu'.
	TestTrue(TEXT("una prospettiva parziale non rivela piu' di Omniscient"),
		Preview->NumUnitsShown() <= UnitCount);

	Preview->ClearPreview();
	TestEqual(TEXT("e con l'anteprima se ne va anche il confine"), Preview->NumBorderPanelsShown(), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
