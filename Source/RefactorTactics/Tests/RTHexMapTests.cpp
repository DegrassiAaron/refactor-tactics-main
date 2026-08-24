#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexMapCustomVersion.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexVisionLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Serialization/CustomVersion.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"

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
/**
 * `#902`: **svuotare la mappa muove la revisione**, come ogni altra modifica strutturale.
 *
 * `ARTHexMapActor::ClearAsset` scriveva sui due array direttamente (`Cells.Reset()`), ed era l'unico
 * punto del progetto a modificare l'asset senza incrementare `Revision` — proprio con la modifica piu'
 * grande possibile. `AddOrUpdateCell`, `UpdateCells`, `RemoveCell`, `AddTransition`,
 * `RemoveTransition` e `UpdateTransitions` la incrementano tutti: misurato, sei siti su sei.
 *
 * Conta perche' `Revision` invalida le cache di percorso *e* perche'
 * `ARTTurnManager::CurrentGraphRevision()` la legge per scrivere `GraphRevision` su ogni voce del
 * TurnLog (`D-067`), campo che entra nell'hash. Con la revisione ferma, due voci ai due lati di uno
 * svuotamento sarebbero indistinguibili — cioe' cio' che quel campo esiste per impedire.
 *
 * ⚠️ **Il gemello negativo conta quanto la prova**: svuotare una mappa GIA' vuota non deve muovere
 * niente. Senza, un `++Revision` incondizionato passerebbe la prima meta' del test e romperebbe la
 * regola che `UpdateCells` dichiara — *«nessuna modifica: la revisione non deve muoversi»*.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapClearBumpsRevisionTest,
	"RefactorTactics.HexMap.ClearAllBumpsTheRevisionOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapClearBumpsRevisionTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 1))
	{
		Map->AddOrUpdateCell(FRTHexCellData(Id));
	}
	Map->AddTransition(FRTCellId(0, 0, 0), FRTCellId(0, 0, 1), /*Cost=*/ 1);
	Map->SortCells();

	// La premessa: c'e' davvero qualcosa da togliere, in ENTRAMBI gli array. Senza, «dopo e' vuoto»
	// sarebbe vero anche di una funzione che non fa niente.
	TestEqual(TEXT("premessa: sette celle"), Map->NumCells(), 7);
	TestTrue(TEXT("premessa: almeno una transizione"), Map->Transitions.Num() > 0);
	// La cache viene popolata interrogando l'asset: e' lo stato in cui un reset puo' lasciarla stantia.
	TestTrue(TEXT("premessa: la cella si trova"), Map->FindCell(FRTCellId(1, 0)) != nullptr);

	const int32 RevisionBefore = Map->Revision;

	TestTrue(TEXT("lo svuotamento dichiara di aver tolto qualcosa"), Map->ClearAll());

	TestEqual(TEXT("celle azzerate"), Map->NumCells(), 0);
	TestEqual(TEXT("transizioni azzerate"), Map->Transitions.Num(), 0);
	// `+1` esatto, non «maggiore di»: due incrementi per un solo svuotamento sarebbero un difetto
	// simmetrico, e `UpdateCells` dichiara la regola — una volta per gruppo.
	TestEqual(TEXT("la revisione sale di UNO"), Map->Revision, RevisionBefore + 1);

	// La cache non sopravvive al reset: con gli indici vecchi `FindCell` leggerebbe fuori dall'array.
	TestTrue(TEXT("nessuna cella fantasma dopo il reset"), Map->FindCell(FRTCellId(1, 0)) == nullptr);

	// --- Il gemello negativo: una mappa gia' vuota non muove niente ---------------------------------
	const int32 RevisionAfterClear = Map->Revision;
	TestFalse(TEXT("un secondo svuotamento non toglie nulla"), Map->ClearAll());
	TestEqual(TEXT("e la revisione resta ferma"), Map->Revision, RevisionAfterClear);

	return true;
}

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

/**
 * Il campo `Covers` entra nel formato: la versione sale a 3 e una mappa scritta con la v2 deve sopravvivere
 * senza perdere NIENTE (invariante #4: ogni formato serializzato e' versionato). La migrazione non converte
 * dati — il campo nuovo nasce vuoto — quindi cio' che va dimostrato e' che non tocca quelli vecchi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapFormatMigrationTest,
	"RefactorTactics.HexMap.FormatMigrationPreservesCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapFormatMigrationTest::RunTest(const FString&)
{
	// Mappa "come l'avrebbe scritta la v2": celle con tutti i campi valorizzati e una transizione.
	URTHexMapAsset* Legacy = NewObject<URTHexMapAsset>();
	FRTHexCellData Floor = Cell(0, 0);
	Floor.Height = 3;
	Floor.MoveCost = 2;
	Floor.Surface = ERTHexSurface::Rough;
	FRTHexCellData Wall = Cell(1, 0);
	Wall.bBlocksMovement = true;
	Wall.bBlocksLineOfSight = true;
	FRTHexCellData Upper = Cell(0, 0, 1);
	Legacy->AddOrUpdateCell(Floor);
	Legacy->AddOrUpdateCell(Wall);
	Legacy->AddOrUpdateCell(Upper);
	Legacy->AddTransition(FRTCellId(0, 0, 0), FRTCellId(0, 0, 1), 2, ERTHexTransitionKind::Stair);
	Legacy->FormatVersion = 2;

	Legacy->MigrateToCurrentFormat();

	TestEqual(TEXT("versione portata alla corrente"), Legacy->FormatVersion, URTHexMapAsset::CurrentFormatVersion);
	// Il numero e' pinnato di proposito: un bump di formato deve far cadere un test, non passare inosservato.
	// v9 (#832) aggiunge l'identita' stabile delle strutture; nessun dato precedente cambia significato.
	TestEqual(TEXT("la versione corrente e' la 10"), URTHexMapAsset::CurrentFormatVersion, 10);
	TestEqual(TEXT("nessuna cella persa"), Legacy->NumCells(), 3);
	TestEqual(TEXT("nessuna transizione persa"), Legacy->Transitions.Num(), 2); // bidirezionale

	const FRTHexCellData* MigratedFloor = Legacy->FindCell(FRTCellId(0, 0, 0));
	TestTrue(TEXT("la cella esiste ancora"), MigratedFloor != nullptr);
	if (MigratedFloor)
	{
		TestEqual(TEXT("Height preservata"), MigratedFloor->Height, 3);
		TestEqual(TEXT("MoveCost preservato"), MigratedFloor->MoveCost, 2);
		TestTrue(TEXT("Surface preservata"), MigratedFloor->Surface == ERTHexSurface::Rough);
		TestEqual(TEXT("nessuna copertura inventata"), MigratedFloor->Covers.Num(), 0);
	}
	const FRTHexCellData* MigratedWall = Legacy->FindCell(FRTCellId(1, 0, 0));
	TestTrue(TEXT("i blocchi restano blocchi"),
		MigratedWall && MigratedWall->bBlocksMovement && MigratedWall->bBlocksLineOfSight);
	TestEqual(TEXT("mappa migrata valida"), Legacy->ValidateMap().Num(), 0);

	// Idempotente: rieseguirla su una mappa gia' migrata non deve toccare nulla (PostLoad puo' ripetersi).
	FRTHexCellData Sheltered = Cell(2, 0);
	Sheltered.Covers.Add(FRTHexCover(ERTHexDirection::W));
	Legacy->AddOrUpdateCell(Sheltered);
	const uint32 Before = Legacy->ComputeHash();
	Legacy->MigrateToCurrentFormat();
	TestEqual(TEXT("seconda migrazione: nessun effetto"), Legacy->ComputeHash(), Before);
	TestEqual(TEXT("le coperture gia' presenti restano"),
		Legacy->FindCell(FRTCellId(2, 0, 0))->Covers.Num(), 1);
	return true;
}

/**
 * L'hash resta deterministico col campo nuovo: dipende dal CONTENUTO delle coperture, non dall'ordine in cui
 * i bordi finiscono nell'array (che l'editor non garantisce).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapCoverHashTest,
	"RefactorTactics.HexMap.CoverHashDeterminism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapCoverHashTest::RunTest(const FString&)
{
	auto MapWithCovers = [](const TArray<ERTHexDirection>& Edges)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		FRTHexCellData Data = Cell(0, 0);
		for (const ERTHexDirection Edge : Edges) { Data.Covers.Add(FRTHexCover(Edge)); }
		M->AddOrUpdateCell(Data);
		M->AddOrUpdateCell(Cell(1, 0));
		M->SortCells();
		return M;
	};

	URTHexMapAsset* Plain = MapWithCovers({});
	URTHexMapAsset* Covered = MapWithCovers({ ERTHexDirection::W });
	TestTrue(TEXT("una copertura cambia l'hash"), Plain->ComputeHash() != Covered->ComputeHash());

	URTHexMapAsset* OtherEdge = MapWithCovers({ ERTHexDirection::E });
	TestTrue(TEXT("il bordo protetto entra nell'hash"), Covered->ComputeHash() != OtherEdge->ComputeHash());

	URTHexMapAsset* Forward = MapWithCovers({ ERTHexDirection::W, ERTHexDirection::NE });
	URTHexMapAsset* Reversed = MapWithCovers({ ERTHexDirection::NE, ERTHexDirection::W });
	TestEqual(TEXT("hash indipendente dall'ordine dei bordi"), Forward->ComputeHash(), Reversed->ComputeHash());

	URTHexMapAsset* Damaged = MapWithCovers({ ERTHexDirection::W });
	Damaged->Cells[0].Covers[0].Integrity = 15;
	TestTrue(TEXT("l'integrita' entra nell'hash"), Covered->ComputeHash() != Damaged->ComputeHash());

	TestEqual(TEXT("stessa mappa, stesso hash (nessuna sorgente instabile)"),
		Covered->ComputeHash(), MapWithCovers({ ERTHexDirection::W })->ComputeHash());
	return true;
}

/**
 * La validazione e' il primo consumatore del dato: le coperture si scrivono a mano nell'editor delle
 * proprieta', e due coperture sullo stesso bordo (il "non sovrapponibile" del catalogo) o un'integrita' non
 * positiva sono stati che nessuna regola sa risolvere. Meglio un errore leggibile che un comportamento a caso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapCoverValidationTest,
	"RefactorTactics.HexMap.CoverValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapCoverValidationTest::RunTest(const FString&)
{
	URTHexMapAsset* Valid = NewObject<URTHexMapAsset>();
	FRTHexCellData Good = Cell(0, 0);
	Good.Covers.Add(FRTHexCover(ERTHexDirection::W));
	Good.Covers.Add(FRTHexCover(ERTHexDirection::E));
	Valid->AddOrUpdateCell(Good);
	TestEqual(TEXT("due bordi diversi: valida"), Valid->ValidateMap().Num(), 0);

	URTHexMapAsset* Overlapping = NewObject<URTHexMapAsset>();
	FRTHexCellData Twice = Cell(0, 0);
	Twice.Covers.Add(FRTHexCover(ERTHexDirection::W));
	Twice.Covers.Add(FRTHexCover(ERTHexDirection::W));
	Overlapping->AddOrUpdateCell(Twice);
	TestEqual(TEXT("copertura sovrapposta: un errore"), Overlapping->ValidateMap().Num(), 1);

	URTHexMapAsset* Broken = NewObject<URTHexMapAsset>();
	FRTHexCellData Zero = Cell(0, 0);
	Zero.Covers.Add(FRTHexCover(ERTHexDirection::W, ERTHexCoverType::Low, 0));
	Broken->AddOrUpdateCell(Zero);
	TestEqual(TEXT("integrita' non positiva: un errore"), Broken->ValidateMap().Num(), 1);

	URTHexMapAsset* Empty = NewObject<URTHexMapAsset>();
	FRTHexCellData NoneType = Cell(0, 0);
	NoneType.Covers.Add(FRTHexCover(ERTHexDirection::W, ERTHexCoverType::None));
	Empty->AddOrUpdateCell(NoneType);
	TestEqual(TEXT("voce senza copertura: un errore"), Empty->ValidateMap().Num(), 1);

	// Il valore di catalogo e' 30: sta nel dato (default della struct), non in un numero scritto altrove.
	TestEqual(TEXT("integrita' di catalogo della copertura bassa"),
		FRTHexCover(ERTHexDirection::W).Integrity, 30);
	return true;
}

/**
 * I tre colori dell'overlay restano distinguibili fra loro.
 *
 * Sembra pedante finche' non si guarda a cosa servono: superficie, «non ci si passa» e «non ci si vede»
 * sono tre regole diverse disegnate come tre anelli concentrici sulla stessa cella, in editor e in partita.
 * Se due di quei colori collassassero, chi dipinge la mappa vedrebbe una regola al posto di un'altra — ed e'
 * il modo in cui l'overlay smette di comunicare senza che nessun test cada.
 *
 * La tavolozza e' condivisa apposta fra editor e runtime: e' l'invariante che tiene le due viste d'accordo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexOverlayPaletteTest,
	"RefactorTactics.HexMap.OverlayPaletteIsDistinguishable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexOverlayPaletteTest::RunTest(const FString&)
{
	const FColor Blocked = URTHexLibrary::BlockedCellColor();
	const FColor Sight = URTHexLibrary::SightBlockerColor();
	const FColor Floor = URTHexLibrary::SurfaceColor(ERTHexSurface::Floor);

	TestTrue(TEXT("«non si passa» != «non si vede»"), Blocked != Sight);
	TestTrue(TEXT("«non si passa» != pavimento"), Blocked != Floor);
	TestTrue(TEXT("«non si vede» != pavimento"), Sight != Floor);

	// Le due squadre devono distinguersi fra loro e da tutto il resto: un anello di partenza che si confonde
	// con un marcatore di regola direbbe «qui parte una squadra» dove c'e' scritto «qui non si passa».
	const FColor Spawn0 = URTHexLibrary::SpawnTeam0Color();
	const FColor Spawn1 = URTHexLibrary::SpawnTeam1Color();
	TestTrue(TEXT("squadra 0 != squadra 1"), Spawn0 != Spawn1);
	for (const FColor& Other : { Blocked, Sight, Floor })
	{
		TestTrue(TEXT("partenza squadra 0 distinta dal resto"), Spawn0 != Other);
		TestTrue(TEXT("partenza squadra 1 distinta dal resto"), Spawn1 != Other);
	}
	return true;
}

/**
 * Il pennello sa dipingere un muro, e sa toglierlo.
 *
 * Prima esisteva un solo verso: `ApplyBrush` preservava sempre `bBlocksLineOfSight` e nessuno strumento
 * dell'editor lo scriveva, quindi una cella che blocca la vista si poteva ottenere solo editando l'array
 * `Cells` a mano nel Data Asset. Il passo 3 di U1 ne chiede due, e U13 ne aggiunge altre.
 *
 * Il flag si verifica sull'**effetto**, non sul campo: leggere `bBlocksLineOfSight` direbbe solo che il dato
 * e' stato scritto, non che la linea di tiro sia davvero interrotta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBrushLineOfSightTest,
	"RefactorTactics.HexMap.BrushWritesAndClearsLineOfSight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBrushLineOfSightTest::RunTest(const FString&)
{
	const FRTCellId Id(1, 0, 0);

	// 1. Non passato: preserva. E' il contratto storico, quello che `ApplyBrushMerge` pinna.
	{
		FRTHexCellData Existing(Id);
		Existing.bBlocksLineOfSight = true;
		const FRTHexCellData Painted = URTHexMapAsset::ApplyBrush(&Existing, Id, ERTHexSurface::Fire, 5, true);
		TestTrue(TEXT("senza il parametro il muro resta"), Painted.bBlocksLineOfSight);
	}

	// 2. Passato true su una cella che non lo aveva: scrive.
	{
		FRTHexCellData Existing(Id);
		Existing.bBlocksLineOfSight = false;
		const FRTHexCellData Painted = URTHexMapAsset::ApplyBrush(&Existing, Id, ERTHexSurface::Floor, 1, false,
			TOptional<bool>(true));
		TestTrue(TEXT("il pennello alza il flag"), Painted.bBlocksLineOfSight);
	}

	// 3. Passato false su una cella che lo aveva: TOGLIE. Senza questo verso il muro sarebbe irreversibile
	//    dall'editor, che e' il difetto opposto a quello risolto.
	{
		FRTHexCellData Existing(Id);
		Existing.bBlocksLineOfSight = true;
		const FRTHexCellData Painted = URTHexMapAsset::ApplyBrush(&Existing, Id, ERTHexSurface::Floor, 1, false,
			TOptional<bool>(false));
		TestFalse(TEXT("il pennello abbassa il flag"), Painted.bBlocksLineOfSight);
	}

	// 4. L'effetto vero: dopo una pennellata la linea di tiro e' interrotta, e ridipingendo torna libera.
	//    Il muro NON blocca il passo: `bBlocksMovement` resta false, ed e' cio' che serve a una rotta coperta
	//    ma percorribile.
	{
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
		for (const FRTCellId& C : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 3))
		{
			Map->AddOrUpdateCell(FRTHexCellData(C));
		}
		Map->SortCells();

		const FRTCellId From(-2, 0, 0);
		const FRTCellId To(2, 0, 0);
		TestTrue(TEXT("prima: vista libera"), URTHexVisionLibrary::HasLineOfSight(Map, From, To));

		Map->BeginStroke();
		Map->PaintCellInStroke(Id, ERTHexSurface::Floor, 1, /*bBlocksMovement=*/ false, TOptional<bool>(true));
		Map->EndStroke();
		TestFalse(TEXT("dopo la pennellata: vista interrotta"), URTHexVisionLibrary::HasLineOfSight(Map, From, To));
		TestFalse(TEXT("e il passo resta libero"), Map->FindCell(Id)->bBlocksMovement);

		Map->BeginStroke();
		Map->PaintCellInStroke(Id, ERTHexSurface::Floor, 1, /*bBlocksMovement=*/ false, TOptional<bool>(false));
		Map->EndStroke();
		TestTrue(TEXT("ridipinto senza flag: vista di nuovo libera"),
			URTHexVisionLibrary::HasLineOfSight(Map, From, To));
	}
	return true;
}


/**
 * **Solo `Cells` ha collisione.** E' l'invariante da cui dipende la selezione, e ogni componente aggiunto
 * all'actor puo' romperla senza che nulla se ne accorga.
 *
 * `ResolveClickedCell` valida il COMPONENTE colpito (`IsPickOnSelectableCell`), non l'actor: dal
 * 2026-08-12 geometria collidibile su `Relief` o `Blockers` non produce piu' una cella sbagliata, viene
 * scartata e la risoluzione ripiega sul piano del layer. Resta pero' una precisione persa in silenzio, e
 * questo test tiene ferma la sola cosa che nessun altro guarda: **quali componenti collidono**.
 *
 * ⚠️ Questo test copre la collisione dei componenti, NON la risoluzione. La risoluzione e' pura e vive in
 * `URTHexLibrary::ResolveRayToCellOnLayer`, coperta da `Hex.ResolveRayToCellOnLayer*` in `RTHexTests.cpp`.
 *
 * Verifica anche il verso opposto: `Cells` la collisione DEVE averla, o la selezione smetterebbe di
 * funzionare del tutto — e un test che controllasse solo «gli altri non collidono» resterebbe verde.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexOnlyCellsAreClickableTest,
	"RefactorTactics.HexMap.OnlyTheCellsComponentIsClickable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexOnlyCellsAreClickableTest::RunTest(const FString&)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
	if (!World) { return false; }
	FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
	Ctx.SetCurrentWorld(World);

	ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
	TestNotNull(TEXT("l'actor esiste"), Actor);
	if (Actor)
	{
		TArray<UInstancedStaticMeshComponent*> Components;
		Actor->GetComponents<UInstancedStaticMeshComponent>(Components);
		TestTrue(TEXT("ci sono piu' componenti istanziati"), Components.Num() >= 2);

		int32 Collidable = 0;
		for (const UInstancedStaticMeshComponent* C : Components)
		{
			if (C->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
			{
				++Collidable;
				// Se un componente nuovo diventa collidibile, il messaggio dice QUALE: senza il nome, chi legge
				// il fallimento saprebbe solo che «qualcosa» ruba i click.
				TestEqual(TEXT("l'unico componente collidibile e' quello delle celle"),
					C->GetName(), FString(TEXT("Cells")));
			}
		}
		TestEqual(TEXT("esattamente UN componente collidibile"), Collidable, 1);
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
	return true;
}


/**
 * Un nome di fixture sbagliato **non svuota l'asset**.
 *
 * Il pulsante SOSTITUISCE il contenuto, quindi il momento pericoloso e' il refuso: `CoverYrad` invece di
 * `CoverYard` cancellerebbe una mappa d'autore e la sostituirebbe con niente. `MakeFixtureArena` risponde
 * `nullptr` a un nome sconosciuto, e il chiamante deve fermarsi PRIMA di toccare l'asset — non dopo.
 *
 * Verifica anche il caso buono, o resterebbe verde un pulsante che non fa mai nulla.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexFixtureLoaderTest,
	"RefactorTactics.HexMap.UnknownFixtureLeavesTheAssetUntouched",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexFixtureLoaderTest::RunTest(const FString&)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
	if (!World) { return false; }
	FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
	Ctx.SetCurrentWorld(World);

	ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
	if (Actor)
	{
		Actor->MapAsset = NewObject<URTHexMapAsset>();
		Actor->MapAsset->AddOrUpdateCell(FRTHexCellData(FRTCellId(7, 7, 0))); // una cella riconoscibile
		const uint32 BeforeHash = Actor->MapAsset->ComputeHash();

		// 1. Nome sbagliato: l'asset non si muove. E' la proprieta' che conta, perche' il pulsante sostituisce.
		Actor->FixtureId = TEXT("CoverYrad"); // refuso plausibile
		Actor->GenerateFixtureIntoAsset();
		TestEqual(TEXT("nome sconosciuto -> asset invariato"), Actor->MapAsset->ComputeHash(), BeforeHash);
		TestTrue(TEXT("la cella d'origine c'e' ancora"),
			Actor->MapAsset->ContainsCell(FRTCellId(7, 7, 0)));

		// 2. Nome giusto: la fixture entra davvero, e porta con se' cio' che la rende utile — CoverYard esiste
		//    perche' e' l'unica mappa con una copertura ALTA, e senza quelle sarebbe una fixture qualunque.
		Actor->FixtureId = TEXT("CoverYard");
		Actor->GenerateFixtureIntoAsset();
		TestTrue(TEXT("nome valido -> l'asset cambia"), Actor->MapAsset->ComputeHash() != BeforeHash);
		TestFalse(TEXT("la cella d'origine e' stata sostituita, non fusa"),
			Actor->MapAsset->ContainsCell(FRTCellId(7, 7, 0)));

		int32 HighCovers = 0;
		for (const FRTHexCellData& Cell : Actor->MapAsset->Cells)
		{
			for (const FRTHexCover& Cover : Cell.Covers)
			{
				if (Cover.Type == ERTHexCoverType::High) { ++HighCovers; }
			}
		}
		TestTrue(TEXT("CoverYard porta la copertura alta, che e' la ragione per cui esiste"), HighCovers > 0);
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
	return true;
}

/**
 * `#905`: **generare una fixture e' UN evento, e muove la revisione una volta**.
 *
 * Il metodo scriveva il rimpiazzo a mano — `Cells.Reset()` piu' un `AddOrUpdateCell` per cella — quindi
 * l'arena della v0.1 faceva salire `Revision` di **98**, e le transizioni (assegnate direttamente) non la
 * muovevano affatto.
 *
 * E' la regola che `UpdateCells` enuncia: *«un portone largo tre bordi si apre una volta, non tre — cosi'
 * che chi osserva la revisione non veda tre cambi dove ce n'e' stato uno»*. Chi legge `Revision` per
 * invalidare una cache di percorso, o come `GraphRevision` nel TurnLog, vedeva 98 eventi per un comando.
 *
 * ⚠️ **Il test conta attraverso il PULSANTE, non sull'asset.** Verificare `ReplaceContent` da solo direbbe
 * che il metodo nuovo e' corretto, non che il pulsante lo usa — ed e' il cablaggio la cosa che era rotta.
 * Il numero di celle e' pinnato per la stessa ragione: senza, «una revisione» sarebbe vero anche di una
 * fixture vuota.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexFixtureOneRevisionTest,
	"RefactorTactics.HexMap.FixtureGenerationIsOneRevision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexFixtureOneRevisionTest::RunTest(const FString&)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
	if (!World) { return false; }
	FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
	Ctx.SetCurrentWorld(World);

	ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
	if (Actor)
	{
		Actor->MapAsset = NewObject<URTHexMapAsset>();
		const int32 RevisionBefore = Actor->MapAsset->Revision;

		Actor->FixtureId = TEXT("ArenaV01");
		Actor->GenerateFixtureIntoAsset();

		// La premessa: la fixture ha DAVVERO molte celle. Senza, «una revisione» non proverebbe niente —
		// e' proprio la differenza fra 1 e N a essere in discussione.
		const int32 Written = Actor->MapAsset->NumCells();
		TestTrue(FString::Printf(TEXT("premessa: la fixture porta molte celle (%d)"), Written), Written > 10);
		TestTrue(TEXT("premessa: porta anche transizioni"), Actor->MapAsset->Transitions.Num() > 0);

		// Il criterio: UNA revisione per un evento, non una per cella.
		TestEqual(TEXT("generare una fixture muove la revisione di UNO"),
			Actor->MapAsset->Revision, RevisionBefore + 1);

		// Il gemello: rigenerare la stessa fixture e' un secondo evento, e ne vale un'altra — non zero
		// (non e' un no-op) e non N.
		const int32 RevisionAfterFirst = Actor->MapAsset->Revision;
		Actor->GenerateFixtureIntoAsset();
		TestEqual(TEXT("rigenerarla vale un'altra revisione, non N"),
			Actor->MapAsset->Revision, RevisionAfterFirst + 1);
		TestEqual(TEXT("e il contenuto non si e' fuso con se stesso"), Actor->MapAsset->NumCells(), Written);
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
	return true;
}


/**
 * La migrazione di formato verificata su un asset **serializzato**, non su uno costruito in memoria.
 *
 * Tutti gli altri test di migrazione fanno `NewObject` -> impostano `FormatVersion` -> migrano: verificano la
 * FUNZIONE, non la serializzazione. E' una distinzione che il progetto ha gia' pagato — «scrivi l'asset col
 * binario vecchio, rileggilo col nuovo: il test in memoria non tocca la serializzazione».
 *
 * `DA_HexMap_Arena` e' quel binario vecchio: committato l'11 agosto quando `CurrentFormatVersion` era **6**,
 * mentre la **7** e' arrivata il giorno dopo (#619). E' un artefatto genuinamente pre-migrazione, e finche'
 * nessuno lo caricava la v6->v7 non aveva soggetto.
 *
 * L'asserzione che conta e' che **un campo nuovo nasca vuoto**: una mappa scritta prima della v7 non ha
 * geometria cotta, quindi nessuna cella deve aver acquisito un sovrapprezzo di occupazione dal nulla. Se lo
 * acquisisse, la stessa mappa costerebbe di piu' solo per essere stata ricaricata.
 *
 * ✅ **Scrivendolo era emerso che la migrazione era inerte** — `FormatVersion` non finiva nei byte (delta
 * contro il CDO), quindi ogni asset si ricaricava gia' «aggiornato». Chiuso da #687 ([D-137]): la versione
 * viaggia ora in `FRTHexMapCustomVersion`, e la prima asserzione qui sotto ha smesso di essere vacua.
 *
 * Questo test e l'altro guardano due cose diverse e servono entrambi:
 * `FormatVersionTravelsInSerializedBytes` prova il **meccanismo** su byte scritti nel test, questo lo prova
 * sull'**artefatto reale** committato nel repository — l'unico binario che nessuno puo' aver costruito per
 * far passare il test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSerializedAssetMigrationTest,
	"RefactorTactics.HexMap.SerializedAssetMigratesWithoutGainingData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSerializedAssetMigrationTest::RunTest(const FString&)
{
	const TCHAR* AssetPath = TEXT("/Game/RT/Maps/Dev/L_HexArena/Data/DA_HexMap_Arena.DA_HexMap_Arena");
	URTHexMapAsset* Loaded = LoadObject<URTHexMapAsset>(nullptr, AssetPath);

	// Se l'asset sparisse, la migrazione tornerebbe senza soggetto e nessuno se ne accorgerebbe: il test deve
	// FALLIRE, non essere saltato. E' l'unico asset mappa con contenuto reale del repository.
	TestNotNull(TEXT("l'arena committata si carica"), Loaded);
	if (!Loaded) { return false; }

	// 1. La versione viene dai BYTE, non dal default del CDO — che e' il difetto chiuso da #687.
	//    `LoadedFormatVersion` e' `INDEX_NONE` solo per un asset costruito in memoria: se lo fosse qui,
	//    significherebbe che `Serialize` non ha letto la versione dall'archivio, cioe' che il meccanismo
	//    non ha agito sull'unico artefatto reale del repository.
	TestNotEqual(TEXT("la versione arriva dall'archivio, non dal CDO"),
		Loaded->LoadedFormatVersion, static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("dopo PostLoad la versione e' corrente"),
		Loaded->FormatVersion, URTHexMapAsset::CurrentFormatVersion);

	//    Oggi `DA_HexMap_Arena` e' un binario pre-meccanismo (committato l'11 agosto), quindi la migrazione
	//    ha avuto un soggetto vero. Il giorno in cui qualcuno lo risalvera' dall'editor acquisira' la voce
	//    nel registro e questa riga cambiera' valore: e' un'informazione, non un guasto, e per questo si
	//    registra nel log invece di far cadere il test. Il meccanismo resta coperto in entrambi i versi da
	//    `RefactorTactics.HexMap.FormatVersionTravelsInSerializedBytes`.
	AddInfo(FString::Printf(TEXT("versione dichiarata dai byte dell'arena committata: %d (%s)"),
		Loaded->LoadedFormatVersion,
		Loaded->LoadedFormatVersion == FRTHexMapCustomVersion::BeforeCustomVersionWasAdded
			? TEXT("pre-meccanismo: la migrazione ha girato per intero")
			: TEXT("asset risalvato dopo #687")));

	// 2. Ha contenuto VERO. Senza questo il test resterebbe verde su un asset vuoto — e nel repository ce n'e'
	//    uno (`DA_HexMap_Sandbox`, 1396 byte): «migra senza perdere nulla» e' banalmente vero del nulla.
	TestTrue(TEXT("ha celle"), Loaded->NumCells() > 0);
	TestTrue(TEXT("ha almeno una transizione"), Loaded->Transitions.Num() > 0);
	TestTrue(TEXT("ha celle su piu' di un layer"), Loaded->GetLayers().Num() >= 2);

	// 3. Niente si e' corrotto nel viaggio.
	TestEqual(TEXT("il validator non trova errori"), Loaded->ValidateMap().Num(), 0);

	// 4. **Il campo nuovo nasce vuoto.** Una mappa scritta prima della v7 non ha geometria cotta: nessuna cella
	//    deve aver acquisito un sovrapprezzo dal nulla, o la stessa mappa costerebbe di piu' solo per essere
	//    stata ricaricata. Questa SI' e' una proprieta' della serializzazione, e nessun test in memoria la
	//    tocca: e' l'asserzione per cui questo test esiste.
	int32 WithSurcharge = 0;
	for (const FRTHexCellData& Cell : Loaded->Cells)
	{
		if (Cell.TotalMoveCost() != Cell.MoveCost) { ++WithSurcharge; }
	}
	TestEqual(TEXT("nessuna cella ha guadagnato un sovrapprezzo migrando"), WithSurcharge, 0);

	// 5. **Stessa domanda per il campo di v9** (#832, CP 23.3). L'arena e' stata scritta quando le
	//    strutture non avevano un nome pubblico: nessuna porta e nessun arco deve averne guadagnato uno
	//    ricaricandosi. Un nome inventato dalla migrazione sarebbe un bersaglio che nessun designer ha
	//    dichiarato — e da #833 in poi qualcuno potrebbe citarlo.
	int32 NamedStructures = 0;
	for (const FRTHexCellData& Cell : Loaded->Cells)
	{
		for (const FRTHexDoor& Door : Cell.Doors)
		{
			if (!Door.StableId.IsNone()) { ++NamedStructures; }
		}
	}
	for (const FRTHexEdge& Arc : Loaded->Transitions)
	{
		if (!Arc.StableId.IsNone()) { ++NamedStructures; }
	}
	TestEqual(TEXT("nessuna struttura ha guadagnato un nome migrando"), NamedStructures, 0);
	return true;
}


/**
 * **Il test a due binari** — criterio di chiusura di #687 ([D-137]), e non e' negoziabile:
 *
 * ```
 * scrivi un asset con il binario di IERI  ->  rileggilo con quello di OGGI  ->  la migrazione DEVE partire
 * ```
 *
 * ## Perche' serviva, e perche' nessun test esistente lo copriva
 *
 * Tutti gli altri test di migrazione fanno `NewObject` -> impostano `FormatVersion` a mano -> migrano:
 * verificano la FUNZIONE, mai il viaggio. E' la distinzione che il progetto ha gia' pagato — *«scrivi
 * l'asset col binario vecchio, rileggilo col nuovo: il test in memoria non tocca la serializzazione»* — e
 * il difetto e' vissuto **otto versioni di formato** dentro quel punto cieco.
 *
 * ## Cosa distingue i due versi, in una riga
 *
 * L'unica differenza fra i due binari e' se l'archivio porta la chiave di `FRTHexMapCustomVersion`:
 * un package scritto prima di quel meccanismo non ce l'ha, e `FArchive::CustomVer` restituisce `-1`
 * (verificato sull'implementazione della 5.8: nessun fallback sul registro globale, quindi «assente»
 * resta assente). Sono gli stessi byte in entrambi i casi — cambia solo cosa l'archivio **dichiara**.
 *
 * ⚠️ **Il verso «binario di oggi» non e' decorativo.** Senza, un `Serialize` che dichiarasse tutti gli
 * asset legacy passerebbe: la migrazione partirebbe sempre, e a ogni caricamento — che e' il difetto del
 * «default fisso» che D-137 ha escluso per iscritto. Un gate provato in un verso solo non e' un gate.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapFormatVersionTravelsTest,
	"RefactorTactics.HexMap.FormatVersionTravelsInSerializedBytes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapFormatVersionTravelsTest::RunTest(const FString&)
{
	// Un asset con contenuto VERO: «migra senza perdere nulla» e' banalmente vero del nulla.
	URTHexMapAsset* Source = NewObject<URTHexMapAsset>();
	Source->AddOrUpdateCell(Cell(0, 0));
	Source->AddOrUpdateCell(Cell(1, 0));
	Source->AddOrUpdateCell(Cell(0, 1, /*Layer=*/ 1));
	Source->Transitions.Add(FRTHexEdge(FRTCellId(0, 0), FRTCellId(0, 1, 1), /*Cost=*/ 2));
	Source->SortCells();
	const int32 SourceCells = Source->NumCells();

	// --- Scrittura ---------------------------------------------------------------------------------
	TArray<uint8> Bytes;
	FCustomVersionContainer WrittenVersions;
	{
		FMemoryWriter Writer(Bytes);
		Source->Serialize(Writer);
		WrittenVersions = Writer.GetCustomVersions();
	}

	// Il lato-scrittura esiste. Se qualcuno togliesse `UsingCustomVersion` da `Serialize`, il numero
	// smetterebbe di viaggiare e TUTTO il resto continuerebbe a passare: e' l'asserzione che tiene in
	// piedi le altre.
	const FCustomVersion* Written = WrittenVersions.GetVersion(FRTHexMapCustomVersion::GUID);
	TestNotNull(TEXT("l'archivio in scrittura dichiara la versione di formato"), Written);
	if (!Written) { return false; }
	TestEqual(TEXT("e la dichiara alla versione corrente"),
		Written->Version, static_cast<int32>(FRTHexMapCustomVersion::LatestVersion));

	// --- Verso A: il binario di OGGI ---------------------------------------------------------------
	// L'archivio porta la chiave, quindi l'asset dichiara la propria versione e non c'e' niente da migrare.
	{
		URTHexMapAsset* Today = NewObject<URTHexMapAsset>();
		FMemoryReader Reader(Bytes);
		Reader.SetCustomVersions(WrittenVersions);
		Today->Serialize(Reader);

		TestEqual(TEXT("A: la versione letta dai byte e' quella corrente"),
			Today->LoadedFormatVersion, static_cast<int32>(FRTHexMapCustomVersion::LatestVersion));

		// La migrazione non deve avere nulla da fare: se ne avesse, girerebbe a ogni caricamento.
		Today->MigrateToCurrentFormat();
		TestEqual(TEXT("A: la migrazione non ha spostato niente"),
			Today->FormatVersion, URTHexMapAsset::CurrentFormatVersion);
	}

	// --- Verso B: il binario di IERI ---------------------------------------------------------------
	// Stessi byte, archivio SENZA la chiave: e' esattamente com'e' un package committato prima di #687.
	{
		URTHexMapAsset* Yesterday = NewObject<URTHexMapAsset>();
		FMemoryReader Reader(Bytes);

		// ⚠️ Un container VUOTO esplicito, non `ResetCustomVersions()`: su un archivio in lettura quel
		// metodo non svuota niente: alla prima interrogazione ricarica **tutte** le versioni registrate
		// globalmente (`Archive.cpp`, `GetCustomVersions`). Il nome inganna, e usandolo qui questo test
		// avrebbe misurato il proprio setup invece del meccanismo — verde per il motivo sbagliato.
		Reader.SetCustomVersions(FCustomVersionContainer());
		Yesterday->Serialize(Reader);

		// ⛔ **L'asserzione per cui questo test esiste.** Prima della fix qui si leggeva
		// `CurrentFormatVersion`: la property prendeva il default del CDO e la migrazione usciva alla
		// prima riga credendosi gia' aggiornata. Se questa riga torna verde su un valore corrente, il
		// meccanismo e' di nuovo inerte.
		TestEqual(TEXT("B: un asset senza voce nel registro si dichiara pre-meccanismo"),
			Yesterday->LoadedFormatVersion,
			static_cast<int32>(FRTHexMapCustomVersion::BeforeCustomVersionWasAdded));
		TestTrue(TEXT("B: e la migrazione ha quindi un soggetto reale"),
			Yesterday->FormatVersion < URTHexMapAsset::CurrentFormatVersion);

		Yesterday->MigrateToCurrentFormat();
		TestEqual(TEXT("B: dopo la migrazione la versione e' corrente"),
			Yesterday->FormatVersion, URTHexMapAsset::CurrentFormatVersion);

		// E il viaggio non ha inventato ne' perso dati: la migrazione e' conservativa per costruzione,
		// ma e' proprio cio' che un passo TRASFORMATIVO futuro cambiera' — e allora questa riga sara' il
		// posto in cui accorgersene.
		TestEqual(TEXT("B: le celle sono sopravvissute al giro"), Yesterday->NumCells(), SourceCells);
		TestEqual(TEXT("B: la transizione e' sopravvissuta al giro"), Yesterday->Transitions.Num(), 1);
		int32 WithSurcharge = 0;
		for (const FRTHexCellData& C : Yesterday->Cells)
		{
			if (C.TotalMoveCost() != C.MoveCost) { ++WithSurcharge; }
		}
		TestEqual(TEXT("B: nessuna cella ha guadagnato un sovrapprezzo migrando"), WithSurcharge, 0);
	}

	// --- Verso C: un archivio in memoria QUALUNQUE (undo/redo, duplicazione) ------------------------
	//
	// Il caso che i due versi sopra non coprono, e che nell'editor capita ogni giorno: `Serialize` viene
	// chiamato da archivi che nessuno ha configurato — il transaction buffer di un undo, una duplicazione.
	// Se li' `CustomVer` rispondesse `-1`, ogni undo su una mappa lascerebbe `FormatVersion` a zero in
	// memoria, e `MigrateToCurrentFormat` non lo correggerebbe (vive in `PostLoad`, che un undo non chiama).
	//
	// Non succede, e la ragione è la stessa trappola di sopra letta al contrario: su un archivio **in
	// lettura** non configurato, `GetCustomVersions()` carica **tutte** le versioni registrate globalmente.
	// È il comportamento giusto qui — ma è dedotto da un'implementazione dell'engine, quindi va pinnato:
	// se cambiasse, il difetto sarebbe silenzioso e visibile solo dopo un undo.
	{
		URTHexMapAsset* Undone = NewObject<URTHexMapAsset>();
		FMemoryReader Reader(Bytes); // nessun `SetCustomVersions`: com'è l'archivio di un undo
		Undone->Serialize(Reader);

		TestEqual(TEXT("C: un archivio non configurato non degrada l'asset a legacy"),
			Undone->LoadedFormatVersion, static_cast<int32>(FRTHexMapCustomVersion::LatestVersion));
		TestEqual(TEXT("C: e la versione resta quella corrente senza bisogno di migrare"),
			Undone->FormatVersion, URTHexMapAsset::CurrentFormatVersion);
	}

	return true;
}


/**
 * **La versione finisce nei byte di un `.uasset` scritto su disco** — l'ultimo pezzo di #687.
 *
 * ## Perché serviva ancora un test
 *
 * `FormatVersionTravelsInSerializedBytes` prova il meccanismo su un archivio **in memoria**, dove le custom
 * version le mette e le toglie il test stesso. In un package vero non è così: le scrive il *summary*, e chi
 * lo popola è UE. Restava quindi una domanda che nessun test headless copriva — *«UE scrive davvero GUID e
 * versione nel file?»* — ed era stata registrata come verifica manuale (`PIE-FMTVER`).
 *
 * ⚠️ **La verifica manuale aveva un difetto che questo test non ha: consumava il proprio soggetto.** Chiedeva
 * di risalvare `DA_HexMap_Arena` dall'editor, cioè di distruggere l'unico binario **pre-meccanismo** del
 * repository — quello che `FMT-2` ha scelto come rivelatore e su cui poggia
 * `SerializedAssetMigratesWithoutGainingData`. Qui il package se lo crea e se lo cancella.
 *
 * ## Il metodo è quello che ha diagnosticato il difetto
 *
 * #687 non è stata trovata leggendo il codice ma **guardando i byte** dell'asset committato
 * (*«FormatVersion presente nei byte: False»*). Questo test fa la stessa domanda al contrario, e su un file
 * che scrive lui: i 16 byte del GUID devono comparire nel `.uasset`. Se `Serialize` smette di dichiarare la
 * versione, il GUID non c'è e la riga diventa rossa — senza dipendere da come UE impagina il summary.
 *
 * ✅ **Misurato per mutazione, e il costo si vede**: rimuovendo `UsingCustomVersion` da `Serialize` il
 * package passa da **2786 a 2766 byte** — venti in meno, cioè i 16 del GUID più i 4 della versione — e il
 * GUID risulta `ASSENTE`. La versione occupa spazio reale nel file: è il modo più concreto di dire che
 * *viaggia*, e prima di #687 quei venti byte non c'erano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapPackageOnDiskTest,
	"RefactorTactics.HexMap.CustomVersionIsWrittenIntoThePackageFile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapPackageOnDiskTest::RunTest(const FString&)
{
	// `/Temp/` è un mount point sempre presente e fuori da `Content/`: il file non entra in git, non finisce
	// nell'asset registry e non può essere scambiato per contenuto di gioco.
	const FString PackageName = TEXT("/Temp/RT_FmtOnDiskProbe");
	const FString FileName = FPackageName::LongPackageNameToFilename(
		PackageName, FPackageName::GetAssetPackageExtension());

	UPackage* Package = CreatePackage(*PackageName);
	if (!TestNotNull(TEXT("package di prova creato"), Package)) { return false; }

	URTHexMapAsset* Asset = NewObject<URTHexMapAsset>(
		Package, TEXT("DA_FmtOnDiskProbe"), RF_Public | RF_Standalone);
	Asset->AddOrUpdateCell(Cell(0, 0));
	Asset->AddOrUpdateCell(Cell(1, 0));
	Asset->SortCells();
	Package->MarkPackageDirty();

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	SaveArgs.bSlowTask = false;
	const bool bSaved = UPackage::SavePackage(Package, Asset, *FileName, SaveArgs);
	if (!TestTrue(TEXT("il package è stato scritto su disco"), bSaved)) { return false; }

	TArray<uint8> Bytes;
	const bool bRead = FFileHelper::LoadFileToArray(Bytes, *FileName);
	TestTrue(TEXT("il file si rilegge"), bRead);

	// Il GUID si costruisce dalla costante, non si trascrive: una copia a mano qui diventerebbe una seconda
	// fonte, ed è esattamente la classe di difetto che #687 ha pagato.
	const FGuid& Key = FRTHexMapCustomVersion::GUID;
	const uint32 Parts[4] = { Key.A, Key.B, Key.C, Key.D };
	TArray<uint8> Needle;
	Needle.Append(reinterpret_cast<const uint8*>(Parts), sizeof(Parts));

	bool bFound = false;
	for (int32 I = 0; bRead && I + Needle.Num() <= Bytes.Num() && !bFound; ++I)
	{
		bFound = (FMemory::Memcmp(Bytes.GetData() + I, Needle.GetData(), Needle.Num()) == 0);
	}

	// ⛔ L'asserzione per cui questo test esiste. Rossa se `Serialize` smette di chiamare
	// `UsingCustomVersion`: il package tornerebbe a non dichiarare nulla, e ogni asset scritto da lì in poi
	// nascerebbe indistinguibile da uno legacy.
	TestTrue(TEXT("il .uasset su disco porta il GUID della versione di formato nel proprio summary"), bFound);
	AddInfo(FString::Printf(TEXT("package di %d byte, GUID %s %s"),
		Bytes.Num(), *Key.ToString(), bFound ? TEXT("presente") : TEXT("ASSENTE")));

	// Il file di prova non deve sopravvivere al test: `Saved/` è già pieno di artefatti che nessuno rilegge.
	Package->ClearFlags(RF_Standalone);
	IFileManager::Get().Delete(*FileName, /*RequireExists=*/ false, /*EvenReadOnly=*/ true);
	return true;
}


/**
 * **I default di `URTHexMapAsset` non si cambiano senza una migrazione** (#830).
 *
 * ## Perche' un default e' un dato di gioco, qui
 *
 * La serializzazione delta di UE salta le property uguali al CDO. Misurato sui byte di `DA_HexMap_Arena`
 * (#687): `MapClass` e `HexSize` **non ci sono**. Un'arena salvata come `Skirmish` — il default — non porta
 * quel valore nei propri byte: lo *eredita* al caricamento.
 *
 * Quindi il giorno in cui il default cambiasse, **ogni mappa gia' committata cambierebbe significato in
 * silenzio**, senza che nessuno tocchi un asset. E non e' decorativo: la classe serve al validator per
 * rifiutare l'accoppiata mappa/formato sbagliata **prima** dell'allestimento. Una mappa che cambia classe da
 * sola non produce un errore — produce un allestimento che passa la validazione sbagliata.
 *
 * ⚠️ **Nessuna rete a valle lo prenderebbe**: `MapClass` **non** entra in `ComputeHash` (deliberato — non
 * cambia un esito di turno), quindi una mappa che cambia classe non produce nemmeno replay divergence.
 *
 * ## Cosa fa questo test, e cosa NON fa
 *
 * Non impedisce il cambio: lo rende **rosso**. Costringe chi lo fa a scrivere la migrazione nello stesso
 * commit invece di scoprirlo dopo — lo stesso ruolo che `D-108` ha dato al gate degli SVG radar, e
 * `D-141` al pin sull'assenza di `FormatVersion`.
 *
 * ## I due rischi sono diversi, e servono due asserzioni diverse
 *
 * La issue ne nomina due, e un pin sul solo default ne copre uno:
 *
 * 1. **il default cambia** (`= ERTMapClass::Operations`) → le mappe che non portano il valore lo ereditano
 *    nuovo. Lo vede il pin sui default, sotto;
 * 2. **l'enum si riordina** (`Operations` messo prima di `Skirmish`) → il default resta scritto
 *    *simbolicamente* `Skirmish` e il pin passerebbe, ma ogni mappa che ha `MapClass` **nei byte** ha
 *    memorizzato un **numero**, e quel numero ora significa un'altra classe. Lo vede il pin sui valori.
 *    Il commento di `ERTMapClass` lo dichiara gia' — *«Aggiungere valori solo IN CODA»* — e finora nessuna
 *    macchina lo verificava.
 *
 * ## Perche' tre campi e non due
 *
 * La issue nomina `MapClass` e `HexSize`. `LayerHeight` ha lo **stesso** difetto tre righe piu' sotto ed
 * entra in `axial <-> world` esattamente come `HexSize`, che e' la ragione per cui `HexSize` e' nella lista.
 *
 * ⚠️ `Revision` e `MapId` restano **deliberatamente fuori**: la prima cambia al primo `AddOrUpdateCell`,
 * quindi in pratica non e' mai uguale al default e viaggia; il secondo e' un GUID assegnato, e un default
 * vuoto non e' un significato che qualcuno possa ereditare per sbaglio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapPinnedDefaultsTest,
	"RefactorTactics.HexMap.SerializationDefaultsArePinned",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapPinnedDefaultsTest::RunTest(const FString&)
{
	const URTHexMapAsset* Fresh = NewObject<URTHexMapAsset>();

	// --- 1. I default che le mappe gia' committate EREDITANO -----------------------------------------
	// Un'asserzione per campo: se cadono tutte insieme senza dire quale, chi legge il log deve bisecare.
	TestEqual(
		TEXT("il default di MapClass NON si cambia senza migrazione: non e' nei byte delle mappe salvate "
			 "(#687), quindi cambiarlo riclassifica in silenzio ogni mappa gia' committata"),
		Fresh->MapClass, ERTMapClass::Skirmish);

	TestEqual(
		TEXT("il default di HexSize NON si cambia senza migrazione: entra in axial<->world, quindi "
			 "cambiarlo sposta la geometria di ogni mappa che non lo porta nei byte"),
		Fresh->HexSize, 100.f);

	TestEqual(
		TEXT("il default di LayerHeight NON si cambia senza migrazione: stessa ragione di HexSize, "
			 "e' la quota fra un layer e il successivo"),
		Fresh->LayerHeight, 250.f);

	// --- 2. I valori dell'enum, che i byte gia' scritti portano come NUMERI --------------------------
	// Aggiungere in coda e' sicuro; riordinare no. Qui non si pinna il simbolo ma la sua rappresentazione.
	TestEqual(TEXT("ERTMapClass::Skirmish resta 0: i byte gia' scritti lo dicono cosi'"),
		static_cast<int32>(ERTMapClass::Skirmish), 0);
	TestEqual(TEXT("ERTMapClass::Standard resta 1"),
		static_cast<int32>(ERTMapClass::Standard), 1);
	TestEqual(TEXT("ERTMapClass::Operations resta 2"),
		static_cast<int32>(ERTMapClass::Operations), 2);
	return true;
}

/**
 * **Nessuna copertura AUTORATA nasce sotto il catalogo del proprio tipo** (#1317).
 *
 * 🔴 **Il fix di [#1194] copre il costruttore, e il costruttore non e' il percorso d'autoraggio.** Chi
 * aggiunge una entry `Covers` dal pannello dei dettagli di un `URTHexMapAsset` non chiama
 * `FRTHexCover(Edge, Type)`: la struct nasce da `FRTHexCover()` — `Low`/`30` — e cambiare `Type` in `High`
 * **non ricalcola niente**, perche' l'asset non ha un `PostEditChangeProperty`. `ValidateMap` non lo vede:
 * la sua guardia e' `Integrity <= 0`, e `30` la passa. Una `High` cosi' autorata vale il **60%** del proprio
 * catalogo e sotto `D-186` legge «ridotta» appena nata.
 *
 * ⚠️ **Questo test RILEVA, non previene**, ed e' una scelta: la prevenzione costerebbe una regola nuova in
 * `ValidateMap` — che dovrebbe essere un warning, perche' `D-186` dichiara **legittima** una copertura sotto
 * catalogo (`Adaptive` nasce a `25` di proposito) — oppure un hook di editor che rischia di sovrascrivere un
 * valore voluto. Il difetto vive negli **asset versionati**, e sono tre: un oracolo che li guarda costa meno
 * di una regola e non puo' sovrascrivere niente.
 *
 * ⛔ **L'elenco dei path e' MANUALE**, come per `SerializedAssetMigratesWithoutGainingData` qui sopra: chi
 * committa un quarto map asset lo aggiunge qui, e finche' non lo fa questo test non lo guarda. Un
 * `UObjectLibrary` scoprirebbe anche i futuri, ma dipende dall'AssetRegistry popolato e sarebbe verde per
 * vacuita' il giorno in cui non lo trovasse — cioe' proprio il modo in cui un gate smette di guardare senza
 * dirlo.
 *
 * 🔴 **MISURATO il 2026-08-24, e il difetto non ha soggetto: in tutti e tre gli asset esiste UNA copertura.**
 * `DA_HexMap_Arena` ne ha **zero** su 64 celle, `DA_HexMap_Sandbox` e' vuoto, `DA_HexMap_Scratch_Basin` ne ha
 * **una** su 45 celle — e non e' sotto catalogo. Il difetto e' reale nel meccanismo e **assente nei dati**:
 * nessuno ha ancora autorato una copertura a mano in questo repository.
 *
 * ⛔ **Per questo la guardia anti-vacuita' NON e' `Totale > 0`, ed e' una correzione.** La prima stesura di
 * questo test la scriveva cosi', e sarebbe stata appesa a quell'unica copertura — che vive in `_Scratch`,
 * l'asset che `GenerateFixtureIntoAsset` **sovrascrive** a ogni rigenerazione. Una fixture senza coperture, e
 * il test sarebbe diventato **rosso senza che niente si fosse rotto**. Un gate che dipende da un asset
 * volatile misura la volatilita', non l'invariante.
 *
 * ✅ **La guardia e' invece una MUTAZIONE**: si costruisce una `High` a `30` — esattamente cio' che l'editor
 * produce oggi — e si verifica che il predicato la riconosca. Cosi' il test non e' vacuo nemmeno il giorno in
 * cui gli asset non offrono un soggetto, e se `DefaultIntegrity` o il confronto cambiassero lo direbbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAuthoredCoversNotBelowCatalogTest,
	"RefactorTactics.HexMap.AuthoredCoversAreNotBelowCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAuthoredCoversNotBelowCatalogTest::RunTest(const FString&)
{
	// I tre `URTHexMapAsset` versionati al 2026-08-24 (`git ls-files "Content/**/*.uasset"`).
	const TCHAR* PercorsiAsset[] =
	{
		TEXT("/Game/RT/Maps/Dev/L_HexArena/Data/DA_HexMap_Arena.DA_HexMap_Arena"),
		TEXT("/Game/RT/Maps/Dev/L_DevSandbox/Data/DA_HexMap_Sandbox.DA_HexMap_Sandbox"),
		TEXT("/Game/RT/Maps/Dev/_Scratch/DA_HexMap_Scratch_Basin.DA_HexMap_Scratch_Basin"),
	};

	int32 Totale = 0;
	int32 SottoCatalogo = 0;
	for (const TCHAR* Percorso : PercorsiAsset)
	{
		URTHexMapAsset* Mappa = LoadObject<URTHexMapAsset>(nullptr, Percorso);

		// Se un asset sparisse il test deve FALLIRE, non essere saltato: un oracolo che perde il proprio
		// soggetto e resta verde e' peggio di un oracolo assente.
		if (!TestNotNull(*FString::Printf(TEXT("l'asset committato si carica: %s"), Percorso), Mappa))
		{
			continue;
		}

		int32 PerAsset = 0;
		for (const FRTHexCellData& Cella : Mappa->Cells)
		{
			for (const FRTHexCover& Copertura : Cella.Covers)
			{
				++Totale;
				++PerAsset;
				const int32 Catalogo = FRTHexCover::DefaultIntegrity(Copertura.Type);
				if (Copertura.Integrity < Catalogo)
				{
					++SottoCatalogo;
					AddError(FString::Printf(
						TEXT("%s: la cella %s ha sul bordo %d una copertura di tipo %d a %d, sotto il proprio ")
						TEXT("catalogo %d — sotto D-186 legge «ridotta» senza che nulla l'abbia colpita (#1317)"),
						Percorso, *Cella.Id.ToString(), static_cast<int32>(Copertura.Edge),
						static_cast<int32>(Copertura.Type), Copertura.Integrity, Catalogo));
				}
			}
		}
		AddInfo(FString::Printf(TEXT("%s: %d celle, %d coperture"), Percorso, Mappa->NumCells(), PerAsset));
	}

	// LA MISURA che #1317 chiede come primo criterio: quante coperture esistono negli asset versionati, e
	// quante di quelle nascono gia' sotto il catalogo del proprio tipo.
	AddInfo(FString::Printf(TEXT("coperture autorate: %d — di cui sotto catalogo: %d"), Totale, SottoCatalogo));

	TestEqual(TEXT("nessuna copertura autorata nasce sotto il catalogo del proprio tipo"), SottoCatalogo, 0);

	// **Guardia per MUTAZIONE, al posto di un `Totale > 0` che dipenderebbe da un asset volatile.** Questa e'
	// la copertura che il pannello dei dettagli produce oggi: struct costruita di default — `Low`/`30` — e poi
	// `Type` portato a `High` a mano, senza che nulla ricalcoli l'integrita'. Se il predicato qui sopra
	// smettesse di riconoscerla, il test resterebbe verde per la ragione sbagliata.
	FRTHexCover DallEditor;
	DallEditor.Type = ERTHexCoverType::High;
	TestEqual(TEXT("la struct di default nasce al catalogo della Low"),
		FRTHexCover().Integrity, FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low));
	TestTrue(TEXT("e una High autorata cosi' e' sotto il proprio catalogo: il rilevatore la vede"),
		DallEditor.Integrity < FRTHexCover::DefaultIntegrity(DallEditor.Type));
	return true;
}


#endif // WITH_DEV_AUTOMATION_TESTS
