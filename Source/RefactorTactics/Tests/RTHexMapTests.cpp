#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FRTHexCellData Cell(int32 X, int32 Y, int32 Layer = 0)
	{
		return FRTHexCellData(FRTCellId(X, Y, Layer));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapAddFindTest,
	"RefactorTactics.HexMap.AddFindContainsUpdateRemove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapAddFindTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
	Map->AddOrUpdateCell(Cell(0, 0));
	Map->AddOrUpdateCell(Cell(1, 0));
	Map->AddOrUpdateCell(Cell(0, 1));
	TestEqual(TEXT("3 celle"), Map->NumCells(), 3);
	TestTrue(TEXT("contiene (1,0)"), Map->ContainsCell(FRTCellId(1, 0)));
	TestFalse(TEXT("non contiene (9,9)"), Map->ContainsCell(FRTCellId(9, 9)));
	TestTrue(TEXT("find (0,1) != null"), Map->FindCell(FRTCellId(0, 1)) != nullptr);
	TestTrue(TEXT("find assente = null"), Map->FindCell(FRTCellId(9, 9)) == nullptr);

	// Update per Id: non duplica, aggiorna il dato.
	FRTHexCellData Updated = Cell(1, 0);
	Updated.MoveCost = 5;
	Updated.Surface = ERTHexSurface::Water;
	Map->AddOrUpdateCell(Updated);
	TestEqual(TEXT("update non duplica"), Map->NumCells(), 3);
	const FRTHexCellData* Found = Map->FindCell(FRTCellId(1, 0));
	TestTrue(TEXT("dato aggiornato"), Found && Found->MoveCost == 5 && Found->Surface == ERTHexSurface::Water);

	// Remove.
	TestTrue(TEXT("remove esistente"), Map->RemoveCell(FRTCellId(0, 0)));
	TestFalse(TEXT("remove assente"), Map->RemoveCell(FRTCellId(0, 0)));
	TestEqual(TEXT("2 celle dopo remove"), Map->NumCells(), 2);
	TestFalse(TEXT("(0,0) rimossa"), Map->ContainsCell(FRTCellId(0, 0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapSortHashTest,
	"RefactorTactics.HexMap.SortAndDeterministicHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapSortHashTest::RunTest(const FString&)
{
	// Stesse celle, ordine di inserimento diverso -> stesso hash (indipendente dall'ordine).
	URTHexMapAsset* A = NewObject<URTHexMapAsset>();
	A->AddOrUpdateCell(Cell(2, -1)); A->AddOrUpdateCell(Cell(0, 0)); A->AddOrUpdateCell(Cell(1, 0, 1));
	URTHexMapAsset* B = NewObject<URTHexMapAsset>();
	B->AddOrUpdateCell(Cell(1, 0, 1)); B->AddOrUpdateCell(Cell(2, -1)); B->AddOrUpdateCell(Cell(0, 0));
	TestEqual(TEXT("hash indipendente dall'ordine"), A->ComputeHash(), B->ComputeHash());

	// Contenuto diverso -> hash diverso.
	URTHexMapAsset* C = NewObject<URTHexMapAsset>();
	C->AddOrUpdateCell(Cell(2, -1)); C->AddOrUpdateCell(Cell(0, 0)); C->AddOrUpdateCell(Cell(1, 0, 2)); // layer 2 != 1
	TestTrue(TEXT("hash diverso per contenuto diverso"), A->ComputeHash() != C->ComputeHash());

	// SortCells: ordine stabile Layer->X->Y.
	A->SortCells();
	bool bSorted = true;
	for (int32 I = 1; I < A->Cells.Num(); ++I)
	{
		bSorted = bSorted && URTHexLibrary::StableLess(A->Cells[I - 1].Id, A->Cells[I].Id);
	}
	TestTrue(TEXT("celle ordinate stabilmente"), bSorted);
	// La cache resta corretta dopo il sort.
	TestTrue(TEXT("find ok dopo sort"), A->FindCell(FRTCellId(1, 0, 1)) != nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapValidateTest,
	"RefactorTactics.HexMap.Validate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapValidateTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
	Map->AddOrUpdateCell(Cell(0, 0));
	Map->AddOrUpdateCell(Cell(1, 0));
	TestEqual(TEXT("mappa valida = 0 errori"), Map->ValidateMap().Num(), 0);

	// Duplicato (accesso diretto per bypassare AddOrUpdate) + costo negativo + transizione verso cella inesistente.
	Map->Cells.Add(Cell(0, 0)); // Id duplicato
	FRTHexCellData Bad = Cell(2, 0);
	Bad.MoveCost = -3;
	Map->Cells.Add(Bad);
	Map->Transitions.Add(FRTHexEdge(FRTCellId(0, 0), FRTCellId(9, 9), 1)); // (9,9) inesistente

	const TArray<FString> Errors = Map->ValidateMap();
	TestTrue(TEXT("almeno 3 errori"), Errors.Num() >= 3);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
