#include "Misc/AutomationTest.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexLibrary.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitWorldForCellTest,
	"RefactorTactics.Unit.WorldForCellIsHexCenter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitWorldForCellTest::RunTest(const FString&)
{
	// WorldForCell e' const e legge solo VisualZOffset: basta il CDO, niente World da creare e distruggere.
	const ARTUnit* Unit = GetDefault<ARTUnit>();
	const FVector Origin(0.f, 0.f, 0.f);
	const float HexSize = 100.f;
	const float LayerHeight = 250.f;
	const FRTCellId Cell(2, -1, 1);

	// La geometria sta in URTHexLibrary (gia' testata): l'unita' aggiunge SOLO l'offset di presentazione.
	const FVector Expected = URTHexLibrary::AxialToWorld(Cell, Origin, HexSize, LayerHeight)
		+ FVector(0.f, 0.f, Unit->VisualZOffset);

	TestTrue(TEXT("la posizione visiva e' il centro esagonale piu' l'offset di presentazione"),
		Unit->WorldForCell(Cell, Origin, HexSize, LayerHeight).Equals(Expected, 0.01f));

	// Celle diverse dello stesso layer stanno alla stessa quota: l'offset non dipende dalle assiali.
	const FVector A = Unit->WorldForCell(FRTCellId(0, 0, 0), Origin, HexSize, LayerHeight);
	const FVector B = Unit->WorldForCell(FRTCellId(3, -2, 0), Origin, HexSize, LayerHeight);
	TestEqual(TEXT("stessa quota sullo stesso layer"), A.Z, B.Z);

	// Un layer piu' in alto e' piu' in alto di esattamente LayerHeight.
	const FVector Up = Unit->WorldForCell(FRTCellId(0, 0, 1), Origin, HexSize, LayerHeight);
	TestTrue(TEXT("il layer superiore e' a +LayerHeight"), FMath::IsNearlyEqual(Up.Z - A.Z, LayerHeight, 0.01f));
	return true;
}
