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
	Updated.Surface = ERTHexSurface::ShallowWater;
	Map->AddOrUpdateCell(Updated);
	TestEqual(TEXT("update non duplica"), Map->NumCells(), 3);
	const FRTHexCellData* Found = Map->FindCell(FRTCellId(1, 0));
	TestTrue(TEXT("dato aggiornato"), Found && Found->MoveCost == 5 && Found->Surface == ERTHexSurface::ShallowWater);

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexApplyBrushTest,
	"RefactorTactics.HexMap.ApplyBrushMerge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexApplyBrushTest::RunTest(const FString&)
{
	const FRTCellId Id(2, -1, 1);

	// Cella nuova (Existing = nullptr): default Height=0, LOS=false; applica surface/cost/block.
	const FRTHexCellData New = URTHexMapAsset::ApplyBrush(nullptr, Id, ERTHexSurface::ShallowWater, 3, true);
	TestTrue(TEXT("Id impostato"), New.Id == Id);
	TestTrue(TEXT("Surface applicata"), New.Surface == ERTHexSurface::ShallowWater);
	TestEqual(TEXT("MoveCost applicato"), New.MoveCost, 3);
	TestTrue(TEXT("bBlocksMovement applicato"), New.bBlocksMovement);
	TestEqual(TEXT("Height default 0"), New.Height, 0);
	TestFalse(TEXT("LOS default false"), New.bBlocksLineOfSight);

	// Cella esistente con Height=3 e LOS=true: paint cambia surface/cost/block ma PRESERVA Height e LOS.
	FRTHexCellData Existing(Id);
	Existing.Height = 3;
	Existing.bBlocksLineOfSight = true;
	Existing.Surface = ERTHexSurface::Floor;
	Existing.MoveCost = 1;
	Existing.bBlocksMovement = false;
	const FRTHexCellData Painted = URTHexMapAsset::ApplyBrush(&Existing, Id, ERTHexSurface::Fire, 5, true);
	TestTrue(TEXT("Surface aggiornata"), Painted.Surface == ERTHexSurface::Fire);
	TestEqual(TEXT("MoveCost aggiornato"), Painted.MoveCost, 5);
	TestTrue(TEXT("bBlocksMovement aggiornato"), Painted.bBlocksMovement);
	TestEqual(TEXT("Height preservato"), Painted.Height, 3);
	TestTrue(TEXT("LOS preservato"), Painted.bBlocksLineOfSight);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexStrokeEquivalenceTest,
	"RefactorTactics.HexMap.StrokeEquivalence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexStrokeEquivalenceTest::RunTest(const FString&)
{
	// Una pennellata di 3 celle produce lo stesso contenuto di 3 AddOrUpdateCell, e celle ordinate dopo EndStroke.
	URTHexMapAsset* Stroke = NewObject<URTHexMapAsset>();
	Stroke->BeginStroke();
	TestTrue(TEXT("paint 1"), Stroke->PaintCellInStroke(FRTCellId(2, -1, 0), ERTHexSurface::ShallowWater, 3, true));
	TestTrue(TEXT("paint 2"), Stroke->PaintCellInStroke(FRTCellId(0, 0, 0), ERTHexSurface::Floor, 1, false));
	TestTrue(TEXT("paint 3"), Stroke->PaintCellInStroke(FRTCellId(1, 0, 1), ERTHexSurface::Rough, 2, false));
	Stroke->EndStroke();

	URTHexMapAsset* Direct = NewObject<URTHexMapAsset>();
	FRTHexCellData C1(FRTCellId(2, -1, 0)); C1.Surface = ERTHexSurface::ShallowWater; C1.MoveCost = 3; C1.bBlocksMovement = true;
	FRTHexCellData C2(FRTCellId(0, 0, 0)); // default: Floor, costo 1, no block
	FRTHexCellData C3(FRTCellId(1, 0, 1)); C3.Surface = ERTHexSurface::Rough; C3.MoveCost = 2;
	Direct->AddOrUpdateCell(C1); Direct->AddOrUpdateCell(C2); Direct->AddOrUpdateCell(C3);
	Direct->SortCells();

	TestEqual(TEXT("stesso NumCells"), Stroke->NumCells(), Direct->NumCells());
	TestEqual(TEXT("stesso hash (contenuto)"), Stroke->ComputeHash(), Direct->ComputeHash());

	bool bSorted = true;
	for (int32 I = 1; I < Stroke->Cells.Num(); ++I)
	{
		bSorted = bSorted && URTHexLibrary::StableLess(Stroke->Cells[I - 1].Id, Stroke->Cells[I].Id);
	}
	TestTrue(TEXT("celle ordinate dopo EndStroke"), bSorted);

	// EraseCellInStroke: assente -> false; presente -> true + rimossa.
	TestFalse(TEXT("erase assente = false"), Stroke->EraseCellInStroke(FRTCellId(9, 9, 0)));
	TestTrue(TEXT("erase presente = true"), Stroke->EraseCellInStroke(FRTCellId(0, 0, 0)));
	TestFalse(TEXT("cella rimossa"), Stroke->ContainsCell(FRTCellId(0, 0, 0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapFloodRegionTest,
	"RefactorTactics.HexMap.FloodRegion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapFloodRegionTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = NewObject<URTHexMapAsset>();

	// Helper locale: aggiunge una cella con superficie esplicita (Cell() di default e' Floor).
	auto AddSurf = [Map](int32 X, int32 Y, ERTHexSurface S)
	{
		FRTHexCellData C = Cell(X, Y);
		C.Surface = S;
		Map->AddOrUpdateCell(C);
	};

	// Regione contigua di 3 celle Floor: (0,0)-(1,0)-(0,1) (entrambe adiacenti a (0,0)).
	AddSurf(0, 0, ERTHexSurface::Floor);
	AddSurf(1, 0, ERTHexSurface::Floor);
	AddSurf(0, 1, ERTHexSurface::Floor);
	// Bordo: (2,0) adiacente a (1,0) ma ShallowWater (superficie diversa -> esclusa).
	AddSurf(2, 0, ERTHexSurface::ShallowWater);
	// Floor ma NON contigua alla regione -> esclusa.
	AddSurf(5, 5, ERTHexSurface::Floor);

	const TArray<FRTCellId> Region = Map->FloodRegion(FRTCellId(0, 0));
	const TSet<FRTCellId> RegionSet(Region);
	TestEqual(TEXT("3 celle nella regione"), Region.Num(), 3);
	TestTrue(TEXT("include (0,0)"), RegionSet.Contains(FRTCellId(0, 0)));
	TestTrue(TEXT("include (1,0)"), RegionSet.Contains(FRTCellId(1, 0)));
	TestTrue(TEXT("include (0,1)"), RegionSet.Contains(FRTCellId(0, 1)));
	TestFalse(TEXT("esclude bordo Water (2,0)"), RegionSet.Contains(FRTCellId(2, 0)));
	TestFalse(TEXT("esclude Normal non contigua (5,5)"), RegionSet.Contains(FRTCellId(5, 5)));

	// Start su cella inesistente -> regione vuota.
	TestEqual(TEXT("start inesistente -> vuoto"), Map->FloodRegion(FRTCellId(9, 9)).Num(), 0);
	return true;
}

/**
 * Contratto della cache Id->indice quando Cells viene modificato SENZA passare dalle API (e' cio' che fa
 * l'undo/redo, che riscrive la property direttamente). Senza invalidazione la cache resta allineata allo stato
 * precedente: FindCell leggerebbe l'indice sbagliato e, se le celle sono diminuite, FUORI dall'array.
 * Il fallimento reale e' stato osservato in editor (undo di una pennellata + click su una cella); qui se ne fissa
 * il contratto perche' non possa regredire.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapLookupInvalidationTest,
	"RefactorTactics.HexMap.LookupInvalidatedAfterExternalEdit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapLookupInvalidationTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 1))
	{
		Map->AddOrUpdateCell(FRTHexCellData(Id));
	}
	Map->SortCells();
	TestEqual(TEXT("7 celle"), Map->NumCells(), 7);

	// Popola la cache interrogando l'asset.
	TestTrue(TEXT("cella presente prima della modifica"), Map->FindCell(FRTCellId(1, 0)) != nullptr);

	// Modifica ESTERNA: riscrive l'array come farebbe un undo, lasciando una sola cella.
	const FRTCellId Survivor = Map->Cells[0].Id;
	Map->Cells.SetNum(1);
	Map->InvalidateLookup();

	TestEqual(TEXT("una sola cella dopo la modifica"), Map->NumCells(), 1);
	TestTrue(TEXT("la cella rimasta si trova ancora"), Map->FindCell(Survivor) != nullptr);

	// Nessuna delle celle rimosse deve risultare presente: con la cache stantia avrebbero restituito un
	// puntatore a un indice non piu' valido.
	int32 Ghosts = 0;
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 1))
	{
		if (!(Id == Survivor) && Map->FindCell(Id) != nullptr)
		{
			++Ghosts;
		}
	}
	TestEqual(TEXT("nessuna cella fantasma dalla cache"), Ghosts, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapCenterCellTest,
	"RefactorTactics.HexMap.CenterCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapCenterCellTest::RunTest(const FString&)
{
	// Serve a inquadrare la mappa (camera): il centro non e' per forza l'origine assiale, perche' una
	// mappa costruita nell'editor puo' stare tutta lontano da (0,0).
	URTHexMapAsset* Centered = NewObject<URTHexMapAsset>();
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 3))
	{
		Centered->AddOrUpdateCell(FRTHexCellData(Id));
	}
	Centered->SortCells();
	TestTrue(TEXT("esagono centrato sull'origine: centro (0,0)"), Centered->GetCenterCell() == FRTCellId(0, 0, 0));

	URTHexMapAsset* Offset = NewObject<URTHexMapAsset>();
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(5, -2, 0), 2))
	{
		Offset->AddOrUpdateCell(FRTHexCellData(Id));
	}
	Offset->SortCells();
	TestTrue(TEXT("mappa spostata: il centro la segue"), Offset->GetCenterCell() == FRTCellId(5, -2, 0));

	URTHexMapAsset* Empty = NewObject<URTHexMapAsset>();
	TestTrue(TEXT("mappa vuota: centro all'origine, nessun crash"), Empty->GetCenterCell() == FRTCellId(0, 0, 0));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
