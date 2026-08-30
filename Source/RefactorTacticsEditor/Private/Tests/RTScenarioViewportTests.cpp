#include "Misc/AutomationTest.h"

#include "RTScenarioPreviewSubsystem.h"
#include "RTScenarioViewportModel.h"

#include "Editor.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTMapVisuals.h"
#include "ScenarioHarness/RTScenarioAuthoring.h"
#include "ScenarioHarness/RTScenarioDraft.h"
#include "ScenarioHarness/RTScenarioIndex.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Lo stato iniziale dello scenario nel viewport (#1753).
 *
 * ⛔ **Cosa questi test NON coprono, e non e' una lacuna da riempire con un'asserzione finta.** Che il
 * graybox si legga, che una copertura alta si distingua da una bassa, che la porta chiusa si distingua
 * dall'aperta e che il cuneo dica davvero l'orientamento sono giudizi su cio' che appare a schermo: nessun
 * automation test li vede, e restano voce di seduta. Qui c'e' cio' che, sbagliato, produce un viewport
 * **plausibile e falso** — la posizione, la rotazione, il layer.
 */

namespace
{
	/** I parametri geometrici di una mappa qualunque: origine non banale, per non nascondere un offset perso. */
	const FVector TestOrigin(1000.f, -250.f, 40.f);
	constexpr float TestHexSize = 150.f;   // il valore corrente del progetto (D-163)
	constexpr float TestLayerHeight = 250.f;
}

/**
 * Il marcatore sta SOPRA la faccia della cella, non dentro il prisma.
 *
 * 🔴 E' il difetto che `RTMapVisuals.h` documenta essere gia' costato **due volte**: qualcosa disegnato
 * sotto `RTCellTopZ` finisce dentro un volume opaco, e a schermo non si distingue da qualcosa che non e'
 * stato disegnato affatto — con la suite verde.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioViewportMarkerSitsOnTheCellTest,
	"RefactorTactics.DevSandboxLauncher.MarkerSitsOnTheCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioViewportMarkerSitsOnTheCellTest::RunTest(const FString&)
{
	const FRTCellId Cell(2, -1, 0);
	const FTransform T = RTScenarioViewport::MarkerTransform(
		Cell, ERTHexDirection::E, TestOrigin, TestHexSize, TestLayerHeight);

	const FVector Center = URTHexLibrary::AxialToWorld(Cell, TestOrigin, TestHexSize, TestLayerHeight);

	TestTrue(TEXT("planarmente e' il centro della cella, non un vicino"),
		FVector2D(T.GetLocation() - Center).IsNearlyZero());
	TestEqual(TEXT("e sta esattamente sulla faccia superiore del prisma"),
		static_cast<float>(T.GetLocation().Z - Center.Z), RTCellTopZ, 0.01f);
	TestTrue(TEXT("scala unitaria: la taglia la decide chi disegna"),
		T.GetScale3D().Equals(FVector::OneVector));
	return true;
}

/**
 * Il marcatore GUARDA il vicino che il facing nomina — tutte e sei le direzioni.
 *
 * ⚠️ **L'oracolo e' il vicino, non un angolo scritto nel test.** Asserire «E vale 0 gradi» ricopierebbe qui
 * la convenzione degli assi, e il giorno in cui `AxialDirection` cambiasse il test resterebbe verde
 * confermando la convenzione vecchia. Qui si chiede invece: proiettando in avanti dal centro, si finisce
 * piu' vicino al vicino nominato che a ogni altro? Quella domanda sopravvive a un cambio di assi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioViewportMarkerFacesItsNeighbourTest,
	"RefactorTactics.DevSandboxLauncher.MarkerFacesItsNeighbour",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioViewportMarkerFacesItsNeighbourTest::RunTest(const FString&)
{
	const FRTCellId Cell(0, 0, 0);
	const FVector Center = URTHexLibrary::AxialToWorld(Cell, TestOrigin, TestHexSize, TestLayerHeight);

	for (int32 D = 0; D < 6; ++D)
	{
		const ERTHexDirection Facing = static_cast<ERTHexDirection>(D);
		const FTransform T = RTScenarioViewport::MarkerTransform(
			Cell, Facing, TestOrigin, TestHexSize, TestLayerHeight);

		// Un passo avanti lungo il forward del marcatore, alla distanza di un vicino.
		const FVector Ahead = Center + T.GetRotation().GetForwardVector() * (TestHexSize * 1.7320508f);

		int32 Nearest = INDEX_NONE;
		double Best = TNumericLimits<double>::Max();
		for (int32 N = 0; N < 6; ++N)
		{
			const FRTCellId Candidate = URTHexLibrary::Neighbor(Cell, static_cast<ERTHexDirection>(N));
			const FVector Pos = URTHexLibrary::AxialToWorld(Candidate, TestOrigin, TestHexSize, TestLayerHeight);
			const double Dist = FVector::DistSquared(FVector(Ahead.X, Ahead.Y, Pos.Z), Pos);
			if (Dist < Best)
			{
				Best = Dist;
				Nearest = N;
			}
		}

		TestEqual(*FString::Printf(TEXT("la direzione %d punta al proprio vicino"), D), Nearest, D);
	}
	return true;
}

/**
 * Il LAYER non si perde: due celle con lo stesso `X/Y` e piano diverso sono due marcatori distinti.
 *
 * Un viewport che le fondesse mostrerebbe una sola unita' dove ce ne sono due, e nessun errore lo direbbe:
 * la lettura resterebbe coerente e sbagliata su ponte, tetto e tunnel.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioViewportMarkerKeepsTheLayerTest,
	"RefactorTactics.DevSandboxLauncher.MarkerKeepsTheLayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioViewportMarkerKeepsTheLayerTest::RunTest(const FString&)
{
	const FTransform Ground = RTScenarioViewport::MarkerTransform(
		FRTCellId(1, 1, 0), ERTHexDirection::E, TestOrigin, TestHexSize, TestLayerHeight);
	const FTransform Above = RTScenarioViewport::MarkerTransform(
		FRTCellId(1, 1, 1), ERTHexDirection::E, TestOrigin, TestHexSize, TestLayerHeight);

	TestTrue(TEXT("stesso X/Y: planarmente coincidono"),
		FVector2D(Above.GetLocation() - Ground.GetLocation()).IsNearlyZero());
	TestEqual(TEXT("e sono separati da un layer esatto"),
		static_cast<float>(Above.GetLocation().Z - Ground.GetLocation().Z), TestLayerHeight, 0.01f);
	return true;
}

/** I piani occupati si dichiarano ordinati, senza ripetizioni, e nella forma che si legge. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioViewportDeclaresItsLayersTest,
	"RefactorTactics.DevSandboxLauncher.ViewportDeclaresItsLayers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioViewportDeclaresItsLayersTest::RunTest(const FString&)
{
	TestEqual(TEXT("nessuna unita': lo dice invece di tacere"),
		RTScenarioViewport::DescribeLayers(RTScenarioViewport::LayersInUse({})), FString(TEXT("nessun layer")));

	TArray<FRTScenarioUnitView> Units;
	FRTScenarioUnitView A; A.Cell = FRTCellId(0, 0, 1); Units.Add(A);
	FRTScenarioUnitView B; B.Cell = FRTCellId(1, 0, 0); Units.Add(B);
	FRTScenarioUnitView C; C.Cell = FRTCellId(2, 0, 1); Units.Add(C); // ripete il layer 1

	const TArray<int32> Layers = RTScenarioViewport::LayersInUse(Units);
	TestEqual(TEXT("due piani distinti, non tre"), Layers.Num(), 2);
	TestEqual(TEXT("crescenti e non nell'ordine del file"), Layers[0], 0);
	TestEqual(TEXT("il secondo e' il layer 1"), Layers[1], 1);
	TestEqual(TEXT("e si leggono"), RTScenarioViewport::DescribeLayers(Layers), FString(TEXT("L0, L1")));
	return true;
}

/**
 * Le squadre si distinguono per RAGGIO, e il raggio piu' grande sta dentro la cella.
 *
 * 🔴 Il colore non e' un'opzione oggi: il kit graybox esce con lo **slot materiale vuoto** (#1714), quindi
 * due squadre colorate diversamente sarebbero identiche a schermo e ogni test resterebbe verde. Una
 * differenza di dimensione si vede senza materiali — ma solo finche' non invade le celle vicine, ed e' cio'
 * che la seconda meta' di questo test misura invece di supporre.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioViewportTeamsDifferByShapeTest,
	"RefactorTactics.DevSandboxLauncher.TeamsDifferByShape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioViewportTeamsDifferByShapeTest::RunTest(const FString&)
{
	TestNotEqual(TEXT("squadra 0 e squadra 1 non hanno lo stesso anello"),
		RTScenarioViewport::TeamRingScale(0), RTScenarioViewport::TeamRingScale(1));
	TestEqual(TEXT("la squadra 0 riusa il raggio di ARTUnit::TeamRing"),
		RTScenarioViewport::TeamRingScale(0), 1.6f, 0.001f);
	TestEqual(TEXT("oltre la quarta squadra si FERMA invece di riavvolgersi sulla prima"),
		RTScenarioViewport::TeamRingScale(9), RTScenarioViewport::MaxTeamRingScale(), 0.001f);
	TestEqual(TEXT("una squadra negativa ripiega sulla prima invece di leggere fuori"),
		RTScenarioViewport::TeamRingScale(-3), RTScenarioViewport::TeamRingScale(0), 0.001f);

	// Il vincolo fisico: due anelli su celle adiacenti non si devono toccare. Il passo della griglia si
    // MISURA dai due centri invece di scrivere `sqrt(3) * HexSize` a mano.
	const FVector Here = URTHexLibrary::AxialToWorld(FRTCellId(0, 0), TestOrigin, TestHexSize, TestLayerHeight);
	const FVector Next = URTHexLibrary::AxialToWorld(FRTCellId(1, 0), TestOrigin, TestHexSize, TestLayerHeight);
	const double Step = FVector::Dist2D(Here, Next);

	// L'anello e' un cilindro engine (raggio 50 a scala 1) in scala ASSOLUTA, come in `ARTUnit`.
	const double LargestRadius = RTScenarioViewport::MaxTeamRingScale() * 50.0;
	TestTrue(TEXT("l'anello piu' grande non invade la cella vicina"), 2.0 * LargestRadius < Step);
	return true;
}

/**
 * Il sottosistema NON mostra nulla quando non c'e' niente da mostrare, e i due «niente» restano distinti.
 *
 * ⚠️ Il caso che conta e' il secondo: una facade **chiusa** deve dare `false` e lasciare lo schermo vuoto.
 * Un'anteprima che sopravvive alla chiusura mostra uno scenario che il pannello non sta piu' dicendo, ed e'
 * il modo in cui una lente diventa una bugia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioViewportRefusesWithoutAScenarioTest,
	"RefactorTactics.DevSandboxLauncher.PreviewRefusesWithoutAScenario",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioViewportRefusesWithoutAScenarioTest::RunTest(const FString&)
{
	if (!TestNotNull(TEXT("GEditor esiste nel contesto editor"), GEditor))
	{
		return false;
	}

	URTScenarioPreviewSubsystem* Preview = GEditor->GetEditorSubsystem<URTScenarioPreviewSubsystem>();
	if (!TestNotNull(TEXT("il sottosistema d'anteprima e' registrato"), Preview))
	{
		return false;
	}

	TestFalse(TEXT("senza facade non si mostra niente"), Preview->ShowScenario(nullptr));
	TestFalse(TEXT("e non resta niente a schermo"), Preview->IsShowing());

	URTScenarioAuthoring* Authoring = URTScenarioAuthoring::CreateScenarioDraft(GetTransientPackage());
	if (!TestNotNull(TEXT("la facade si costruisce"), Authoring))
	{
		return false;
	}

	TestFalse(TEXT("facade CHIUSA: nessuna anteprima"), Preview->ShowScenario(Authoring));
	TestFalse(TEXT("e nemmeno un residuo"), Preview->IsShowing());
	TestEqual(TEXT("nessun marcatore"), Preview->NumUnitsShown(), 0);

	Preview->ClearPreview();
	return true;
}

/**
 * Il giro vero: uno scenario del corpus produce **un marcatore per unita' dichiarata**.
 *
 * 🔑 **L'oracolo e' la facade, non un numero scritto qui.** `ListUnits().Num()` e' cio' che il file dichiara;
 * confrontarlo con i marcatori posati prende il difetto che conta — un'unita' che non arriva a schermo, o
 * una che arriva due volte perche' la posa e' incrementale invece che ricostruita.
 *
 * ⚠️ **Non asserisce che si VEDA**: asserisce che sia stata posata. La leggibilita' e' di seduta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioViewportShowsEveryDeclaredUnitTest,
	"RefactorTactics.DevSandboxLauncher.PreviewShowsEveryDeclaredUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioViewportShowsEveryDeclaredUnitTest::RunTest(const FString&)
{
	if (!TestNotNull(TEXT("GEditor esiste nel contesto editor"), GEditor))
	{
		return false;
	}
	URTScenarioPreviewSubsystem* Preview = GEditor->GetEditorSubsystem<URTScenarioPreviewSubsystem>();
	if (!TestNotNull(TEXT("il sottosistema d'anteprima e' registrato"), Preview))
	{
		return false;
	}

	// Uno scenario del corpus con piu' di due unita': lo si CERCA invece di cablarne l'id, perche' un id
	// scritto qui lega il test a un file che qualcuno puo' rinominare.
	URTScenarioAuthoring* Authoring = URTScenarioAuthoring::CreateScenarioDraft(GetTransientPackage());
	if (!TestNotNull(TEXT("la facade si costruisce"), Authoring))
	{
		return false;
	}

	int32 Expected = 0;
	FString ChosenId;
	for (const FString& Id : URTScenarioAuthoring::ListScenarioIds(FString(), FString()))
	{
		FString OpenError;
		if (Authoring->OpenById(Id, OpenError) != ERTScenarioAuthoringResult::Success)
		{
			continue;
		}
		if (Authoring->ListUnits().Num() > 2)
		{
			Expected = Authoring->ListUnits().Num();
			ChosenId = Id;
			break;
		}
		Authoring->Close();
	}

	if (!TestTrue(TEXT("il corpus offre uno scenario con piu' di due unita'"), Expected > 2))
	{
		return false;
	}

	const bool bShown = Preview->ShowScenario(Authoring);
	Authoring->Close();

	if (!TestTrue(*FString::Printf(TEXT("l'anteprima di '%s' si posa"), *ChosenId), bShown))
	{
		return false;
	}

	TestEqual(TEXT("un marcatore per ogni unita' dichiarata, ne' uno di piu' ne' uno di meno"),
		Preview->NumUnitsShown(), Expected);
	TestTrue(TEXT("e dichiara su quale piano sta ragionando"), !Preview->GetLayerReadout().IsEmpty());

	// Riposare lo stesso scenario non deve raddoppiare i marcatori: la posa si RICOSTRUISCE.
	FString Reopen;
	Authoring->OpenById(ChosenId, Reopen);
	Preview->ShowScenario(Authoring);
	Authoring->Close();
	TestEqual(TEXT("riaprendo, i marcatori non si sommano ai precedenti"),
		Preview->NumUnitsShown(), Expected);

	Preview->ClearPreview();
	TestFalse(TEXT("e si tolgono tutti"), Preview->IsShowing());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
