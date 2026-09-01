#include "Misc/AutomationTest.h"

#include "RTHexLosReadout.h"

#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * L'ispettore della LOS nel viewport (#1755).
 *
 * ⛔ **Cosa questi test NON coprono.** Che la linea si veda, che il rosso si distingua dal verde, che il
 * marcatore del blocco cada sulla cella giusta a schermo: sono giudizi su cio' che appare, e restano voce di
 * seduta. Qui c'e' cio' che, sbagliato, fa **mentire** l'ispettore — una ragione che non corrisponde
 * all'esito, e una query per fotogramma travestita da event-driven.
 *
 * ⚠️ **La parita' fra ragione ed esito NON si riprova qui**: e' del produttore
 * (`RefactorTactics.HexVision.ReasonAgreesWithHasLineOfSight`, 1369 coppie). Duplicarla registrerebbe due
 * volte la stessa garanzia, e la seconda invecchierebbe da sola.
 */

namespace
{
	/** Arena piatta di raggio N, nessun ostacolo. Nome distinto per file (unity build). */
	URTHexMapAsset* MakeLosMap(int32 Radius)
	{
		return URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
	}

	void SetLosBlocker(URTHexMapAsset* Map, const FRTCellId& Id)
	{
		FRTHexCellData Data = Map->FindCell(Id) ? *Map->FindCell(Id) : FRTHexCellData(Id);
		Data.Id = Id;
		Data.bBlocksLineOfSight = true;
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}
}

/**
 * Il verdetto scritto e' quello che il runtime ha deciso, e la ragione NOMINA la cella colpevole.
 *
 * 🔑 L'oracolo non e' una stringa attesa scritta a mano: e' `HasLineOfSight` sugli stessi due punti. Se un
 * giorno il readout dicesse `CLEAR` dove la LOS decide `false`, l'ispettore mentirebbe **esattamente** dove
 * serve — e questo test e' l'unico posto che se ne accorge.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLosReadoutFollowsTheVerdictTest,
	"RefactorTactics.HexEditor.LosReadoutFollowsTheVerdict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLosReadoutFollowsTheVerdictTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeLosMap(4);
	const FRTCellId From(0, 0);
	const FRTCellId To(3, 0);

	{
		const FRTLineOfSightResult Los = URTHexVisionLibrary::DescribeLineOfSight(Map, From, To);
		const RTHexLos::FReadout R = RTHexLos::Describe(true, From, true, To, Los);

		TestTrue(TEXT("via libera secondo l'autorita'"), URTHexVisionLibrary::HasLineOfSight(Map, From, To));
		TestEqual(TEXT("e il pannello dice CLEAR"), R.Verdict, FString(TEXT("CLEAR")));
		TestEqual(TEXT("senza inventare una ragione"), R.Reason, FString(TEXT("—")));
	}

	SetLosBlocker(Map, FRTCellId(2, 0));
	{
		const FRTLineOfSightResult Los = URTHexVisionLibrary::DescribeLineOfSight(Map, From, To);
		const RTHexLos::FReadout R = RTHexLos::Describe(true, From, true, To, Los);

		TestFalse(TEXT("bloccata secondo l'autorita'"), URTHexVisionLibrary::HasLineOfSight(Map, From, To));
		TestEqual(TEXT("e il pannello dice BLOCKED"), R.Verdict, FString(TEXT("BLOCKED")));
		TestTrue(TEXT("la ragione nomina la causa"), R.Reason.Contains(TEXT("CellBlocker")));
		TestTrue(TEXT("e nomina la cella colpevole, non una qualunque"), R.Reason.Contains(TEXT("(2,0,L0)")));
	}

	// LA GEOMETRIA INTRA-CELLA (`D-269`, `#1830`): senza il suo `case`, lo `switch` cadrebbe nel `default` e
	// il pannello direbbe `unavailable` — cioe' l'ispettore tacerebbe proprio sulla causa nuova, che e' il
	// difetto che #1755 vieta al contrario («meglio nessuna ragione che una inventata»): qui la ragione c'e',
	// e non dirla sarebbe perderla.
	{
		URTHexMapAsset* Walled = MakeLosMap(4);
		FRTGeometrySegment Diameter;
		Diameter.Axis = ERTTacticalAxis::Deg90;
		Diameter.Offset = 0;
		Diameter.AlongStart = -RT_GeometryQuanta;
		Diameter.AlongEnd = RT_GeometryQuanta;
		Diameter.WallType = ERTHexCoverType::High;
		Walled->InteriorWalls.Add(FRTHexInteriorWall(FRTCellId(2, 0), Diameter));

		const FRTLineOfSightResult Los = URTHexVisionLibrary::DescribeLineOfSight(Walled, From, To);
		const RTHexLos::FReadout R = RTHexLos::Describe(true, From, true, To, Los);

		TestFalse(TEXT("bloccata secondo l'autorita'"), URTHexVisionLibrary::HasLineOfSight(Walled, From, To));
		TestEqual(TEXT("e il pannello dice BLOCKED"), R.Verdict, FString(TEXT("BLOCKED")));
		TestTrue(TEXT("la ragione nomina la geometria interna"), R.Reason.Contains(TEXT("InteriorGeometry")));
		TestTrue(TEXT("e la cella che porta il muro"), R.Reason.Contains(TEXT("(2,0,L0)")));
	}
	return true;
}

/**
 * Senza due estremi non si scrive un verdetto.
 *
 * Un ispettore che mostrasse `CLEAR` prima che qualcuno abbia scelto due punti direbbe che la vista passa
 * fra due celle che nessuno ha nominato — e chi legge non ha modo di accorgersene.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLosReadoutSaysNothingWithoutTwoEndsTest,
	"RefactorTactics.HexEditor.LosReadoutSaysNothingWithoutTwoEnds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLosReadoutSaysNothingWithoutTwoEndsTest::RunTest(const FString&)
{
	const FRTLineOfSightResult Clear; // via libera: il valore che tenterebbe di far scrivere CLEAR
	const FRTCellId A(0, 0);
	const FRTCellId B(3, 0);

	const RTHexLos::FReadout NoOrigin = RTHexLos::Describe(false, A, true, B, Clear);
	TestEqual(TEXT("senza origine: nessun verdetto"), NoOrigin.Verdict, FString(TEXT("—")));
	TestEqual(TEXT("e nessuna ragione"), NoOrigin.Reason, FString(TEXT("—")));

	const RTHexLos::FReadout NoTarget = RTHexLos::Describe(true, A, false, B, Clear);
	TestEqual(TEXT("senza bersaglio: nessun verdetto"), NoTarget.Verdict, FString(TEXT("—")));
	return true;
}

/**
 * Il piano dichiarato e' quello del TIRATORE.
 *
 * ⚠️ E' il caveat che #1712 registra e che un ispettore multilivello deve dire, o mentira': la linea resta
 * sul layer di chi guarda, quindi mostrare il layer del bersaglio direbbe dove sta il bersaglio, non su
 * quale piano la vista e' stata decisa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLosReadoutDeclaresTheShooterLayerTest,
	"RefactorTactics.HexEditor.LosReadoutDeclaresTheShooterLayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLosReadoutDeclaresTheShooterLayerTest::RunTest(const FString&)
{
	const FRTLineOfSightResult Clear;
	const RTHexLos::FReadout R =
		RTHexLos::Describe(true, FRTCellId(0, 0, 1), true, FRTCellId(3, 0, 0), Clear);

	TestEqual(TEXT("il piano e' quello dell'origine, non del bersaglio"), R.Layer, 1);
	return true;
}

/**
 * 🔴 **Il guardrail di #1755, in forma di test**: la query si rifa' solo se la cella e' CAMBIATA.
 *
 * Il mouse produce eventi mentre si muove dentro la stessa cella. Senza questo filtro l'ispettore sarebbe
 * una query LOS per fotogramma travestita da event-driven — ⛔ *«non per-frame se nulla cambia»*.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLosRequeriesOnlyWhenTheCellChangesTest,
	"RefactorTactics.HexEditor.LosRequeriesOnlyWhenTheCellChanges",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLosRequeriesOnlyWhenTheCellChangesTest::RunTest(const FString&)
{
	const FRTCellId Cell(2, -1, 0);

	TestFalse(TEXT("stessa cella: non si richiede niente"),
		RTHexLos::ShouldRequery(true, Cell, true, Cell));
	TestTrue(TEXT("cella diversa: si richiede"),
		RTHexLos::ShouldRequery(true, Cell, true, FRTCellId(2, 0, 0)));

	// ⚠️ Il LAYER fa parte dell'identita': stesso X/Y su un piano diverso E' un'altra cella, e trattarlo
	// come «la stessa» mostrerebbe il verdetto del piano sbagliato.
	TestTrue(TEXT("stesso X/Y, layer diverso: e' un cambio"),
		RTHexLos::ShouldRequery(true, Cell, true, FRTCellId(2, -1, 1)));

	// Entrare e uscire dalla mappa sono cambi: uscendo, il pannello deve smettere di mostrare l'ultimo
	// verdetto invece di lasciarlo li' come se valesse ancora.
	TestTrue(TEXT("uscire dalla mappa e' un cambio"),
		RTHexLos::ShouldRequery(true, Cell, false, FRTCellId()));
	TestTrue(TEXT("rientrare e' un cambio"),
		RTHexLos::ShouldRequery(false, FRTCellId(), true, Cell));
	TestFalse(TEXT("fuori mappa prima e adesso: niente da chiedere"),
		RTHexLos::ShouldRequery(false, FRTCellId(), false, FRTCellId()));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
