#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"
#include "Pathfinding/RTHexPathLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

// Test H4 (mappa multilivello): query per layer, celle sovrapposte, transizioni verticali
// (bridge/tunnel/scale/ascensori), hash che include le transizioni, validator esteso.
// NB: helper con nome UNICO per file (LayerCell), non "Cell": nelle build unity questo TU viene
// concatenato con RTHexMapTests.cpp (che ha gia' un helper "Cell") e nomi uguali collidono.
namespace
{
	FRTHexCellData LayerCell(int32 X, int32 Y, int32 Layer = 0)
	{
		return FRTHexCellData(FRTCellId(X, Y, Layer));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLayerQueryTest,
	"RefactorTactics.HexMap.LayerQueries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLayerQueryTest::RunTest(const FString&)
{
	URTHexMapAsset* M = NewObject<URTHexMapAsset>();
	M->AddOrUpdateCell(LayerCell(0, 0, 0));
	M->AddOrUpdateCell(LayerCell(1, 0, 0));
	M->AddOrUpdateCell(LayerCell(0, 0, 2)); // stessa (X,Y) di L0 ma layer 2 -> sovrapposta
	M->AddOrUpdateCell(LayerCell(0, 0, 1));

	// GetLayers: distinti e ordinati.
	const TArray<int32> Layers = M->GetLayers();
	TestEqual(TEXT("3 layer distinti"), Layers.Num(), 3);
	TestTrue(TEXT("layer ordinati 0,1,2"), Layers.Num() == 3 && Layers[0] == 0 && Layers[1] == 1 && Layers[2] == 2);

	// CellsInLayer.
	TestEqual(TEXT("layer 0 ha 2 celle"), M->CellsInLayer(0).Num(), 2);
	TestEqual(TEXT("layer 1 ha 1 cella"), M->CellsInLayer(1).Num(), 1);
	TestEqual(TEXT("layer 3 vuoto"), M->CellsInLayer(3).Num(), 0);

	// Celle sovrapposte: (0,0,0) e (0,0,2) coesistono e sono indirizzabili in modo indipendente.
	TestEqual(TEXT("4 celle totali (sovrapposte coesistono)"), M->NumCells(), 4);
	TestTrue(TEXT("(0,0,0) presente"), M->ContainsCell(FRTCellId(0, 0, 0)));
	TestTrue(TEXT("(0,0,2) presente"), M->ContainsCell(FRTCellId(0, 0, 2)));

	// Update indipendente per layer: modificare (0,0,2) non tocca (0,0,0).
	FRTHexCellData Up = LayerCell(0, 0, 2);
	Up.MoveCost = 7;
	M->AddOrUpdateCell(Up);
	const FRTHexCellData* L0 = M->FindCell(FRTCellId(0, 0, 0));
	const FRTHexCellData* L2 = M->FindCell(FRTCellId(0, 0, 2));
	TestTrue(TEXT("(0,0,2) aggiornata a 7"), L2 && L2->MoveCost == 7);
	TestTrue(TEXT("(0,0,0) invariata"), L0 && L0->MoveCost == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexVerticalTransitionTest,
	"RefactorTactics.HexMap.VerticalTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexVerticalTransitionTest::RunTest(const FString&)
{
	URTHexMapAsset* M = NewObject<URTHexMapAsset>();
	M->AddOrUpdateCell(LayerCell(0, 0, 0));
	M->AddOrUpdateCell(LayerCell(0, 0, 1)); // sovrapposta

	// Senza arco: layer diversi non adiacenti.
	TestTrue(TEXT("no path senza transizione"),
		URTHexPathLibrary::FindPath(M, FRTCellId(0, 0, 0), FRTCellId(0, 0, 1)).Status == ERTHexPathStatus::NoPath);

	// AddTransition bidirezionale (scala): crea due archi, incrementa revision.
	const int32 RevBefore = M->Revision;
	M->AddTransition(FRTCellId(0, 0, 0), FRTCellId(0, 0, 1), 2, ERTHexTransitionKind::Stair, /*bBidirectional=*/true);
	TestEqual(TEXT("2 archi (bidirezionale)"), M->Transitions.Num(), 2);
	TestTrue(TEXT("revision incrementata"), M->Revision > RevBefore);

	// Percorribile in entrambe le direzioni, costo dell'arco.
	const FRTHexPathResult Up = URTHexPathLibrary::FindPath(M, FRTCellId(0, 0, 0), FRTCellId(0, 0, 1));
	const FRTHexPathResult Down = URTHexPathLibrary::FindPath(M, FRTCellId(0, 0, 1), FRTCellId(0, 0, 0));
	TestTrue(TEXT("salita ok"), Up.Status == ERTHexPathStatus::Success && Up.TotalCost == 2);
	TestTrue(TEXT("discesa ok"), Down.Status == ERTHexPathStatus::Success && Down.TotalCost == 2);

	// AddTransition su From->To gia' presente: aggiorna, non duplica.
	M->AddTransition(FRTCellId(0, 0, 0), FRTCellId(0, 0, 1), 5, ERTHexTransitionKind::Elevator, /*bBidirectional=*/false);
	TestEqual(TEXT("ancora 2 archi (update non duplica)"), M->Transitions.Num(), 2);
	TestEqual(TEXT("costo salita aggiornato a 5"),
		URTHexPathLibrary::FindPath(M, FRTCellId(0, 0, 0), FRTCellId(0, 0, 1)).TotalCost, 5);

	// RemoveTransition bidirezionale: rimuove entrambi gli archi.
	TestTrue(TEXT("remove ritorna true"), M->RemoveTransition(FRTCellId(0, 0, 0), FRTCellId(0, 0, 1), /*bBothDirections=*/true));
	TestEqual(TEXT("0 archi dopo remove"), M->Transitions.Num(), 0);
	TestFalse(TEXT("remove su assente ritorna false"), M->RemoveTransition(FRTCellId(0, 0, 0), FRTCellId(0, 0, 1), true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexHashTransitionsTest,
	"RefactorTactics.HexMap.HashIncludesTransitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexHashTransitionsTest::RunTest(const FString&)
{
	// Stesse celle; una con transizione, l'altra no -> hash diverso.
	URTHexMapAsset* A = NewObject<URTHexMapAsset>();
	A->AddOrUpdateCell(LayerCell(0, 0, 0)); A->AddOrUpdateCell(LayerCell(0, 0, 1));
	URTHexMapAsset* B = NewObject<URTHexMapAsset>();
	B->AddOrUpdateCell(LayerCell(0, 0, 0)); B->AddOrUpdateCell(LayerCell(0, 0, 1));
	TestEqual(TEXT("hash uguale senza transizioni"), A->ComputeHash(), B->ComputeHash());

	A->Transitions.Add(FRTHexEdge(FRTCellId(0, 0, 0), FRTCellId(0, 0, 1), 2, ERTHexTransitionKind::Bridge));
	TestTrue(TEXT("una transizione cambia l'hash"), A->ComputeHash() != B->ComputeHash());

	// Stesse transizioni inserite in ordine diverso -> stesso hash (ordine-indipendente).
	URTHexMapAsset* C = NewObject<URTHexMapAsset>();
	C->AddOrUpdateCell(LayerCell(0, 0, 0)); C->AddOrUpdateCell(LayerCell(0, 0, 1)); C->AddOrUpdateCell(LayerCell(1, 0, 0));
	C->Transitions.Add(FRTHexEdge(FRTCellId(0, 0, 0), FRTCellId(0, 0, 1), 2, ERTHexTransitionKind::Bridge));
	C->Transitions.Add(FRTHexEdge(FRTCellId(1, 0, 0), FRTCellId(0, 0, 1), 3, ERTHexTransitionKind::Tunnel));
	URTHexMapAsset* D = NewObject<URTHexMapAsset>();
	D->AddOrUpdateCell(LayerCell(0, 0, 0)); D->AddOrUpdateCell(LayerCell(0, 0, 1)); D->AddOrUpdateCell(LayerCell(1, 0, 0));
	D->Transitions.Add(FRTHexEdge(FRTCellId(1, 0, 0), FRTCellId(0, 0, 1), 3, ERTHexTransitionKind::Tunnel));
	D->Transitions.Add(FRTHexEdge(FRTCellId(0, 0, 0), FRTCellId(0, 0, 1), 2, ERTHexTransitionKind::Bridge));
	TestEqual(TEXT("hash indipendente dall'ordine delle transizioni"), C->ComputeHash(), D->ComputeHash());

	// Kind diverso (stessi From/To/Cost) -> hash diverso.
	URTHexMapAsset* E = NewObject<URTHexMapAsset>();
	E->AddOrUpdateCell(LayerCell(0, 0, 0)); E->AddOrUpdateCell(LayerCell(0, 0, 1));
	E->Transitions.Add(FRTHexEdge(FRTCellId(0, 0, 0), FRTCellId(0, 0, 1), 2, ERTHexTransitionKind::Elevator));
	TestTrue(TEXT("kind diverso cambia l'hash"), A->ComputeHash() != E->ComputeHash());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexValidateTransitionsTest,
	"RefactorTactics.HexMap.ValidateTransitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexValidateTransitionsTest::RunTest(const FString&)
{
	URTHexMapAsset* M = NewObject<URTHexMapAsset>();
	M->AddOrUpdateCell(LayerCell(0, 0, 0));
	M->AddOrUpdateCell(LayerCell(0, 0, 1));
	M->AddOrUpdateCell(LayerCell(1, 0, 0)); // adiacente orizzontale a (0,0,0)

	// Transizione valida verticale: nessun errore.
	M->Transitions.Add(FRTHexEdge(FRTCellId(0, 0, 0), FRTCellId(0, 0, 1), 2, ERTHexTransitionKind::Stair));
	TestEqual(TEXT("transizione verticale valida = 0 errori"), M->ValidateMap().Num(), 0);

	// Self-loop -> Error.
	M->Transitions.Add(FRTHexEdge(FRTCellId(0, 0, 0), FRTCellId(0, 0, 0), 1, ERTHexTransitionKind::Stair));
	{
		const TArray<FString> Errs = M->ValidateMap();
		bool bSelfLoop = false;
		for (const FString& S : Errs) { bSelfLoop |= S.Contains(TEXT("Error")) && S.Contains(TEXT("self")); }
		TestTrue(TEXT("self-loop segnalato come Error"), bSelfLoop);
	}

	// Duplicato (stessa From/To) -> Error.
	M->Transitions.Add(FRTHexEdge(FRTCellId(0, 0, 0), FRTCellId(0, 0, 1), 9, ERTHexTransitionKind::Bridge));
	{
		const TArray<FString> Errs = M->ValidateMap();
		bool bDup = false;
		for (const FString& S : Errs) { bDup |= S.Contains(TEXT("Error")) && S.Contains(TEXT("duplicat")); }
		TestTrue(TEXT("transizione duplicata segnalata come Error"), bDup);
	}

	// Transizione ridondante tra celle gia' adiacenti (stesso layer, distanza 1) -> Warning.
	URTHexMapAsset* W = NewObject<URTHexMapAsset>();
	W->AddOrUpdateCell(LayerCell(0, 0, 0));
	W->AddOrUpdateCell(LayerCell(1, 0, 0));
	W->Transitions.Add(FRTHexEdge(FRTCellId(0, 0, 0), FRTCellId(1, 0, 0), 1, ERTHexTransitionKind::Ramp));
	{
		const TArray<FString> Errs = W->ValidateMap();
		bool bWarn = false;
		for (const FString& S : Errs) { bWarn |= S.Contains(TEXT("Warning")); }
		TestTrue(TEXT("transizione ridondante segnalata come Warning"), bWarn);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
