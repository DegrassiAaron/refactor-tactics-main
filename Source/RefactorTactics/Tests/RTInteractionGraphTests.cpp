#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTStructureIdentityLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Grafo di interazione `Source -> Target` (CP 23.4, #833).
 *
 * Il dato dice quale struttura comanda quali altre. Sorgente e bersagli si nominano con gli **StableId di
 * CP 23.3** (#832): questa feature non inventa una seconda identita', e infatti la risoluzione passa da
 * `FindDoorEdges` e la validazione da `ValidateReferences`.
 *
 * ⚠️ **Perche' l'ordine si prova perturbando l'inserimento, e non ripetendo la stessa risoluzione.** Il DoD
 * di #833 chiedeva «verificato ripetendo la stessa risoluzione»: dentro lo stesso processo anche
 * l'iterazione di una `TMap` e' ripetibile, quindi quel test sarebbe passato **sul difetto** che
 * l'invariante n. 3 esiste per escludere. Qui si costruiscono due mappe con le porte inserite in ordine
 * OPPOSTO e si pretende lo stesso esito: e' l'unico verso in cui la proprieta' morde.
 */
namespace
{
	/** Esagono pieno di raggio N sul layer 0. Nome prefissato per dominio: unity build. */
	URTHexMapAsset* MakeGraphMap(int32 Radius)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();
		return M;
	}

	/** Porta con nome pubblico sul bordo indicato. */
	void PutGraphDoor(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge, int32 DoorId, FName StableId)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		FRTHexDoor Door(Edge, ERTHexDoorState::Closed, DoorId);
		Door.StableId = StableId;
		Data.Doors.Add(Door);
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	/** Vero se una delle stringhe contiene il frammento: i reason code si cercano, non si confrontano interi. */
	bool AnyContains(const TArray<FString>& Errors, const TCHAR* Fragment)
	{
		for (const FString& E : Errors)
		{
			if (E.Contains(Fragment))
			{
				return true;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteractionGraphResolvesInDeclaredOrder,
	"RefactorTactics.InteractionGraph.ResolvesInDeclaredOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTInteractionGraphResolvesInDeclaredOrder::RunTest(const FString&)
{
	// `S1` comanda tre porte. L'ordine di applicazione e' quello DICHIARATO in `TargetIds`, e non quello in
	// cui le porte stanno nell'asset: le inserisco al contrario proprio per distinguere le due cose.
	URTHexMapAsset* Map = MakeGraphMap(3);
	PutGraphDoor(Map, FRTCellId(0, 0, 0), ERTHexDirection::E, 1, TEXT("S1"));
	PutGraphDoor(Map, FRTCellId(2, 0, 0), ERTHexDirection::E, 4, TEXT("D3"));
	PutGraphDoor(Map, FRTCellId(1, 0, 0), ERTHexDirection::E, 3, TEXT("D2"));
	PutGraphDoor(Map, FRTCellId(0, 1, 0), ERTHexDirection::E, 2, TEXT("D1"));

	Map->InteractionBindings.Add(FRTInteractionBinding(
		TEXT("S1"), { TEXT("D1"), TEXT("D2"), TEXT("D3") }));

	const TArray<FRTStructureEdgeRef> Targets =
		URTStructureIdentityLibrary::ResolveInteractionTargets(Map, TEXT("S1"));

	TestEqual(TEXT("tre bersagli risolti"), Targets.Num(), 3);
	if (Targets.Num() == 3)
	{
		// `FRTStructureEdgeRef` identifica il bordo con cella + direzione: ogni porta sta su una cella
		// diversa proprio per poterle distinguere qui.
		TestTrue(TEXT("primo bersaglio = D1"), Targets[0].Cell == FRTCellId(0, 1, 0));
		TestTrue(TEXT("secondo bersaglio = D2"), Targets[1].Cell == FRTCellId(1, 0, 0));
		TestTrue(TEXT("terzo bersaglio = D3"), Targets[2].Cell == FRTCellId(2, 0, 0));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteractionGraphOrderIgnoresAssetInsertionOrder,
	"RefactorTactics.InteractionGraph.OrderIgnoresAssetInsertionOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTInteractionGraphOrderIgnoresAssetInsertionOrder::RunTest(const FString&)
{
	// Due mappe IDENTICHE per contenuto e OPPOSTE per ordine d'inserimento delle porte. Con lo stesso
	// binding devono risolvere la stessa sequenza. E' il test che una `TMap` non supererebbe.
	const TArray<FName> Declared = { TEXT("D2"), TEXT("D1") };

	URTHexMapAsset* Forward = MakeGraphMap(3);
	PutGraphDoor(Forward, FRTCellId(0, 0, 0), ERTHexDirection::E, 1, TEXT("S1"));
	PutGraphDoor(Forward, FRTCellId(1, 0, 0), ERTHexDirection::E, 7, TEXT("D1"));
	PutGraphDoor(Forward, FRTCellId(0, 1, 0), ERTHexDirection::E, 9, TEXT("D2"));
	Forward->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), Declared));

	URTHexMapAsset* Reverse = MakeGraphMap(3);
	PutGraphDoor(Reverse, FRTCellId(0, 1, 0), ERTHexDirection::E, 9, TEXT("D2"));
	PutGraphDoor(Reverse, FRTCellId(1, 0, 0), ERTHexDirection::E, 7, TEXT("D1"));
	PutGraphDoor(Reverse, FRTCellId(0, 0, 0), ERTHexDirection::E, 1, TEXT("S1"));
	Reverse->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), Declared));

	const TArray<FRTStructureEdgeRef> A =
		URTStructureIdentityLibrary::ResolveInteractionTargets(Forward, TEXT("S1"));
	const TArray<FRTStructureEdgeRef> B =
		URTStructureIdentityLibrary::ResolveInteractionTargets(Reverse, TEXT("S1"));

	TestEqual(TEXT("stesso numero di bersagli"), A.Num(), B.Num());
	TestEqual(TEXT("due bersagli"), A.Num(), 2);
	if (A.Num() == 2 && B.Num() == 2)
	{
		TestTrue(TEXT("primo uguale fra le due mappe"), A[0].Cell == B[0].Cell);
		TestTrue(TEXT("secondo uguale fra le due mappe"), A[1].Cell == B[1].Cell);
		// E l'ordine e' quello DICHIARATO (`D2` prima), non quello dell'asset.
		TestTrue(TEXT("l'ordine e' quello dichiarato: D2 per primo"), A[0].Cell == FRTCellId(0, 1, 0));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteractionGraphMissingTargetFailsValidation,
	"RefactorTactics.InteractionGraph.MissingTargetFailsValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTInteractionGraphMissingTargetFailsValidation::RunTest(const FString&)
{
	// Un bersaglio fantasma non deve arrivare a runtime: e' un errore d'ASSET, con reason code.
	URTHexMapAsset* Map = MakeGraphMap(2);
	PutGraphDoor(Map, FRTCellId(0, 0, 0), ERTHexDirection::E, 1, TEXT("S1"));
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D_FANTASMA") }));

	const TArray<FString> Errors = URTStructureIdentityLibrary::ValidateInteractionGraph(Map);

	TestTrue(TEXT("il bersaglio inesistente e' segnalato"), Errors.Num() > 0);
	TestTrue(TEXT("il reason code nomina la struttura mancante"),
		AnyContains(Errors, TEXT("D_FANTASMA")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteractionGraphDuplicateSourceFailsValidation,
	"RefactorTactics.InteractionGraph.DuplicateSourceFailsValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTInteractionGraphDuplicateSourceFailsValidation::RunTest(const FString&)
{
	// Due binding per la STESSA sorgente: non «vince l'ultimo», fallisce. Silenziosamente, il secondo
	// sovrascriverebbe il primo e nessuno saprebbe quale delle due liste comanda.
	URTHexMapAsset* Map = MakeGraphMap(2);
	PutGraphDoor(Map, FRTCellId(0, 0, 0), ERTHexDirection::E, 1, TEXT("S1"));
	PutGraphDoor(Map, FRTCellId(1, 0, 0), ERTHexDirection::E, 2, TEXT("D1"));
	PutGraphDoor(Map, FRTCellId(0, 1, 0), ERTHexDirection::E, 3, TEXT("D2"));
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D1") }));
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D2") }));

	const TArray<FString> Errors = URTStructureIdentityLibrary::ValidateInteractionGraph(Map);

	TestTrue(TEXT("il binding duplicato e' segnalato"), Errors.Num() > 0);
	TestTrue(TEXT("il reason code nomina la sorgente duplicata"), AnyContains(Errors, TEXT("S1")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteractionGraphSharedTargetIsNotAnAssetError,
	"RefactorTactics.InteractionGraph.SharedTargetIsNotAnAssetError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTInteractionGraphSharedTargetIsNotAnAssetError::RunTest(const FString&)
{
	// `INT-5` e' APERTA: due sorgenti che nominano lo stesso bersaglio devono poter essere RAPPRESENTATE.
	// Se la validazione le rifiutasse, una decisione aperta riceverebbe risposta da un validator.
	URTHexMapAsset* Map = MakeGraphMap(2);
	PutGraphDoor(Map, FRTCellId(0, 0, 0), ERTHexDirection::E, 1, TEXT("S1"));
	PutGraphDoor(Map, FRTCellId(1, 0, 0), ERTHexDirection::E, 2, TEXT("S2"));
	PutGraphDoor(Map, FRTCellId(0, 1, 0), ERTHexDirection::E, 3, TEXT("D1"));
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D1") }));
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S2"), { TEXT("D1") }));

	const TArray<FString> Errors = URTStructureIdentityLibrary::ValidateInteractionGraph(Map);

	TestEqual(TEXT("il bersaglio condiviso NON e' un errore d'asset (INT-5 aperta)"), Errors.Num(), 0);

	// ⚠️ **L'asserzione negativa qui sopra da sola NON verifica niente**: passa anche contro una funzione che
	// restituisce sempre vuoto, ed e' esattamente cio' che faceva finche' `ValidateInteractionGraph` era uno
	// stub. Le due righe seguenti sono positive e cadono su uno stub, cosi' il test dimostra di poter
	// fallire — e insieme provano che «rappresentabile» significa *risolvibile*, non solo *non rifiutato*.
	TestEqual(TEXT("S1 risolve il bersaglio condiviso"),
		URTStructureIdentityLibrary::ResolveInteractionTargets(Map, TEXT("S1")).Num(), 1);
	TestEqual(TEXT("anche S2 lo risolve"),
		URTStructureIdentityLibrary::ResolveInteractionTargets(Map, TEXT("S2")).Num(), 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
