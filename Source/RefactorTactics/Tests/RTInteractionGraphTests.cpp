#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexDoorLibrary.h"
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
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
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

	/** Come `PutGraphDoor`, ma con lo stato iniziale dichiarato: serve ai bersagli che NON devono aprirsi. */
	void PutGraphDoorInState(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge, int32 DoorId,
		FName StableId, ERTHexDoorState State)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		FRTHexDoor Door(Edge, State, DoorId);
		Door.StableId = StableId;
		Data.Doors.Add(Door);
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	/**
	 * Arco con nome pubblico fra due celle. `AddTransition` non lo nomina — un arco e' identificato dalla
	 * coppia `(From, To)` — quindi lo `StableId` si scrive dopo, sull'arco appena creato.
	 */
	void PutGraphArc(URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To, FName StableId)
	{
		Map->AddTransition(From, To, 1, ERTHexTransitionKind::Stair, false);
		for (FRTHexEdge& Arc : Map->Transitions)
		{
			if (Arc.From == From && Arc.To == To)
			{
				Arc.StableId = StableId;
			}
		}
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteractionGraphMapValidationSeesTheGraph,
	"RefactorTactics.InteractionGraph.MapValidationSeesTheGraph",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTInteractionGraphMapValidationSeesTheGraph::RunTest(const FString&)
{
	// 🔴 **Le cinque regole di `ValidateInteractionGraph` erano raggiungibili SOLO dai test.** Il validator
	// che il gioco usa e' `URTHexMapAsset::ValidateMap()`, e non le chiamava: una mappa spedita con un
	// bersaglio fantasma passava, e a runtime `FindDoorEdges` restituiva un array vuoto — indistinguibile da
	// una sorgente che non comanda nulla. Trovato da una code review, non da un test.
	//
	// Questo test lega le due cose: chiede il difetto al validator DI PRODUZIONE, non alla libreria.
	URTHexMapAsset* Map = MakeGraphMap(2);
	PutGraphDoor(Map, FRTCellId(0, 0, 0), ERTHexDirection::E, 1, TEXT("S1"));
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D_FANTASMA") }));

	const TArray<FString> Errors = Map->ValidateMap();

	TestTrue(TEXT("ValidateMap vede il bersaglio inesistente"), Errors.Num() > 0);
	TestTrue(TEXT("e il reason code lo nomina"), AnyContains(Errors, TEXT("D_FANTASMA")));

	// ⚠️ L'asserzione positiva da sola non basta: passerebbe anche contro un `ValidateMap` che segnala
	// qualunque cosa. Una mappa con un grafo VALIDO deve restare pulita.
	URTHexMapAsset* Good = MakeGraphMap(2);
	PutGraphDoor(Good, FRTCellId(0, 0, 0), ERTHexDirection::E, 1, TEXT("S1"));
	PutGraphDoor(Good, FRTCellId(1, 0, 0), ERTHexDirection::E, 2, TEXT("D1"));
	Good->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D1") }));
	TestEqual(TEXT("un grafo valido non produce errori"), Good->ValidateMap().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteractionGraphOrderChangesMapHash,
	"RefactorTactics.InteractionGraph.OrderChangesMapHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTInteractionGraphOrderChangesMapHash::RunTest(const FString&)
{
	// 🔴 **`InteractionBindings` non entrava in `ComputeHash`**, quindi due mappe che si giocano DIVERSO
	// hashavano identiche e il confronto di determinismo non poteva vedere la divergenza. Il criterio di
	// esclusione che l'header dichiara — «non tocca la geometria ne' il comportamento» — qui e' falso:
	// l'ordine dei bersagli E' l'ordine di applicazione, ed e' la proprieta' che #833 difende.
	const auto MakeWith = [](const TArray<FName>& Targets)
	{
		URTHexMapAsset* M = MakeGraphMap(2);
		PutGraphDoor(M, FRTCellId(0, 0, 0), ERTHexDirection::E, 1, TEXT("S1"));
		PutGraphDoor(M, FRTCellId(1, 0, 0), ERTHexDirection::E, 2, TEXT("D1"));
		PutGraphDoor(M, FRTCellId(0, 1, 0), ERTHexDirection::E, 3, TEXT("D2"));
		M->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), Targets));
		return M;
	};

	URTHexMapAsset* Forward = MakeWith({ TEXT("D1"), TEXT("D2") });
	URTHexMapAsset* Reverse = MakeWith({ TEXT("D2"), TEXT("D1") });

	TestTrue(TEXT("l'ordine dei bersagli cambia l'hash della mappa"),
		Forward->ComputeHash() != Reverse->ComputeHash());

	// E il caso base: senza binding, o con lo stesso binding, l'hash torna uguale — altrimenti
	// l'asserzione sopra passerebbe anche con un hash casuale.
	TestEqual(TEXT("stesso grafo, stesso hash"),
		MakeWith({ TEXT("D1"), TEXT("D2") })->ComputeHash(), Forward->ComputeHash());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteractionGraphBindingInsertionOrderDoesNotChangeHash,
	"RefactorTactics.InteractionGraph.BindingInsertionOrderDoesNotChangeHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTInteractionGraphBindingInsertionOrderDoesNotChangeHash::RunTest(const FString&)
{
	// ⚠️ **La distinzione che rende l'hash corretto invece che solo diverso.** Due ordini contano in modo
	// opposto, e trattarli allo stesso modo sarebbe un difetto in uno dei due versi:
	//
	//   `TargetIds` dentro un binding  -> E' DATO: l'ordine di applicazione dichiarato (test gemello sopra)
	//   i binding FRA LORO nell'array  -> NON e' dato: la risoluzione cerca per `SourceId`
	//
	// Quindi i binding si ordinano prima di mescolarli, esattamente come `Cells` — *«ordine stabile -> hash
	// indipendente dall'ordine di inserimento»* — e i `TargetIds` no. Senza questo test, ordinare anche i
	// bersagli passerebbe il gemello e romperebbe l'invariante n. 3 senza che nulla protestasse.
	const auto MakeTwoBindings = [](bool bS1First)
	{
		URTHexMapAsset* M = MakeGraphMap(2);
		PutGraphDoor(M, FRTCellId(0, 0, 0), ERTHexDirection::E, 1, TEXT("S1"));
		PutGraphDoor(M, FRTCellId(1, 0, 0), ERTHexDirection::E, 2, TEXT("S2"));
		PutGraphDoor(M, FRTCellId(0, 1, 0), ERTHexDirection::E, 3, TEXT("D1"));
		PutGraphDoor(M, FRTCellId(1, -1, 0), ERTHexDirection::E, 4, TEXT("D2"));
		if (bS1First)
		{
			M->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D1") }));
			M->InteractionBindings.Add(FRTInteractionBinding(TEXT("S2"), { TEXT("D2") }));
		}
		else
		{
			M->InteractionBindings.Add(FRTInteractionBinding(TEXT("S2"), { TEXT("D2") }));
			M->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D1") }));
		}
		return M;
	};

	TestEqual(TEXT("l'ordine di INSERIMENTO dei binding non cambia l'hash"),
		MakeTwoBindings(true)->ComputeHash(), MakeTwoBindings(false)->ComputeHash());
	return true;
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteractionGraphArcTargetFailsValidation,
	"RefactorTactics.InteractionGraph.ArcTargetFailsValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTInteractionGraphArcTargetFailsValidation::RunTest(const FString&)
{
	// Il «binding cross-layer illegale» che lo Scope di #833 dichiara errore d'asset, nella sola forma che
	// questo repository puo' MISURARE: una PORTA cross-layer non esiste — `FindDoorEdges` la costruisce da
	// `(cella, direzione)` e il vicino di un bordo sta sullo stesso layer — mentre l'ARCO e' esattamente la
	// struttura che i layer li attraversa (`spec-mappa-multilivello.md`: «celle su layer diversi non sono
	// adiacenti. Mai»).
	URTHexMapAsset* Map = MakeGraphMap(2);
	PutGraphDoor(Map, FRTCellId(0, 0, 0), ERTHexDirection::E, 1, TEXT("S1"));
	PutGraphArc(Map, FRTCellId(0, 0, 0), FRTCellId(0, 0, 1), TEXT("A1"));
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("A1") }));

	const TArray<FString> Errors = URTStructureIdentityLibrary::ValidateInteractionGraph(Map);

	TestTrue(TEXT("il bersaglio-arco e' segnalato"), Errors.Num() > 0);
	TestTrue(TEXT("il reason code nomina la struttura"), AnyContains(Errors, TEXT("A1")));

	// ⚠️ **Questa e' la meta' che dice PERCHE' la regola serve**, e senza di essa il test proverebbe solo che
	// un errore esce. Il nome `A1` risolve — `ValidateReferences` conosce entrambi i domini e non si lamenta,
	// perche' un riferimento e' valido se trova «una porta OPPURE un arco» (#832) — ma la risoluzione passa
	// solo da `FindDoorEdges` e restituisce ZERO bersagli. Prima di questa regola l'asset era valido, l'
	// `Interact` legale, e non succedeva niente: un difetto che nessuno dei due lati poteva vedere.
	TestEqual(TEXT("il riferimento all'arco di per se' e' valido"),
		URTStructureIdentityLibrary::ValidateReferences(Map, { TEXT("A1") }).Num(), 0);
	TestEqual(TEXT("ma la risoluzione non lo comanda: zero bersagli"),
		URTStructureIdentityLibrary::ResolveInteractionTargets(Map, TEXT("S1")).Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteractionGraphArcInTheAssetIsNotAnError,
	"RefactorTactics.InteractionGraph.ArcInTheAssetIsNotAnError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTInteractionGraphArcInTheAssetIsNotAnError::RunTest(const FString&)
{
	// ⚠️ La regola sopra rifiuta un BERSAGLIO che e' un arco, non un asset che contiene archi: una mappa
	// multilivello ne ha per costruzione, e una regola che li contasse renderebbe invalido ogni asset vero.
	// Senza questo test il falso positivo si scoprirebbe su una mappa di produzione.
	URTHexMapAsset* Map = MakeGraphMap(2);
	PutGraphDoor(Map, FRTCellId(0, 0, 0), ERTHexDirection::E, 1, TEXT("S1"));
	PutGraphDoor(Map, FRTCellId(1, 0, 0), ERTHexDirection::E, 2, TEXT("D1"));
	PutGraphArc(Map, FRTCellId(0, 0, 0), FRTCellId(0, 0, 1), TEXT("A1"));
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D1") }));

	TestEqual(TEXT("un arco che non e' bersaglio non e' un errore"),
		URTStructureIdentityLibrary::ValidateInteractionGraph(Map).Num(), 0);
	TestEqual(TEXT("e il binding verso la porta risolve"),
		URTStructureIdentityLibrary::ResolveInteractionTargets(Map, TEXT("S1")).Num(), 1);
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
	// Una sorgente NON contesa nello stesso asset: serve all'asserzione positiva piu' sotto.
	PutGraphDoor(Map, FRTCellId(1, -1, 0), ERTHexDirection::E, 4, TEXT("S3"));
	PutGraphDoor(Map, FRTCellId(-1, 1, 0), ERTHexDirection::E, 5, TEXT("D2"));
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D1") }));
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S2"), { TEXT("D1") }));
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S3"), { TEXT("D2") }));

	const TArray<FString> Errors = URTStructureIdentityLibrary::ValidateInteractionGraph(Map);

	TestEqual(TEXT("il bersaglio condiviso NON e' un errore d'asset (INT-5 aperta)"), Errors.Num(), 0);

	// ⚠️ **L'asserzione negativa qui sopra da sola NON verifica niente**: passa anche contro una funzione che
	// restituisce sempre vuoto, ed e' esattamente cio' che faceva finche' `ValidateInteractionGraph` era uno
	// stub. La riga seguente e' positiva e cade su uno stub, cosi' il test dimostra di poter fallire — e
	// insieme prova che l'asset **carica ed e' utilizzabile**, non solo che non e' stato rifiutato.
	//
	// ⚠️ Qui NON si asserisce piu' che `S1` e `S2` risolvano il bersaglio conteso: lo asserivo, ed era la
	// risposta a `INT-5` data di nascosto dall'implementazione. «Rappresentabile» non significa
	// «risolvibile»; la meta' mancante sta in `SharedTargetIsRefusedByResolution`.
	TestEqual(TEXT("una sorgente non contesa nello stesso asset risolve"),
		URTStructureIdentityLibrary::ResolveInteractionTargets(Map, TEXT("S3")).Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteractionGraphSharedTargetIsRefusedByResolution,
	"RefactorTactics.InteractionGraph.SharedTargetIsRefusedByResolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTInteractionGraphSharedTargetIsRefusedByResolution::RunTest(const FString&)
{
	// L'altra meta' di `INT-5`: rappresentabile SI', risolvibile NO. Se la risoluzione applicasse comunque,
	// due sorgenti comanderebbero la stessa struttura e la semantica di composizione — che `INT-5` lascia
	// aperta — sarebbe decisa qui, in silenzio e da nessuno.
	URTHexMapAsset* Map = MakeGraphMap(2);
	PutGraphDoor(Map, FRTCellId(0, 0, 0), ERTHexDirection::E, 1, TEXT("S1"));
	PutGraphDoor(Map, FRTCellId(1, 0, 0), ERTHexDirection::E, 2, TEXT("S2"));
	PutGraphDoor(Map, FRTCellId(0, 1, 0), ERTHexDirection::E, 3, TEXT("D1"));
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D1") }));
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S2"), { TEXT("D1") }));

	TArray<FString> Errors;
	const TArray<FRTStructureEdgeRef> Targets =
		URTStructureIdentityLibrary::ResolveInteractionTargets(Map, TEXT("S1"), &Errors);

	TestEqual(TEXT("la risoluzione del bersaglio conteso e' rifiutata"), Targets.Num(), 0);
	TestTrue(TEXT("il rifiuto porta un reason code"), Errors.Num() > 0);
	TestTrue(TEXT("il reason code nomina il bersaglio conteso"), AnyContains(Errors, TEXT("D1")));
	TestTrue(TEXT("e nomina l'altra sorgente"), AnyContains(Errors, TEXT("S2")));

	// Il rifiuto e' simmetrico: non e' «vince chi e' dichiarato per primo», che sarebbe di nuovo una
	// semantica di composizione scelta dall'implementazione.
	TArray<FString> ReverseErrors;
	TestEqual(TEXT("anche la sorgente dichiarata per seconda e' rifiutata"),
		URTStructureIdentityLibrary::ResolveInteractionTargets(Map, TEXT("S2"), &ReverseErrors).Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteractionGraphEmptyTargetListFailsValidation,
	"RefactorTactics.InteractionGraph.EmptyTargetListFailsValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTInteractionGraphEmptyTargetListFailsValidation::RunTest(const FString&)
{
	// Un binding DICHIARATO senza bersagli non e' «una sorgente che non comanda nulla»: quella e' una
	// sorgente **senza binding**, e risolve in un array vuoto senza errori. Questo e' un binding scritto a
	// meta', e passa in silenzio esattamente come passerebbe un `TargetIds` che qualcuno ha svuotato.
	URTHexMapAsset* Map = MakeGraphMap(2);
	PutGraphDoor(Map, FRTCellId(0, 0, 0), ERTHexDirection::E, 1, TEXT("S1"));
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), {}));

	const TArray<FString> Errors = URTStructureIdentityLibrary::ValidateInteractionGraph(Map);

	TestTrue(TEXT("il binding senza bersagli e' segnalato"), Errors.Num() > 0);
	TestTrue(TEXT("il reason code nomina la sorgente"), AnyContains(Errors, TEXT("S1")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteractionGraphSelfBindingFailsValidation,
	"RefactorTactics.InteractionGraph.SelfBindingFailsValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTInteractionGraphSelfBindingFailsValidation::RunTest(const FString&)
{
	// Una struttura che comanda SE STESSA risolve, perche' il nome esiste: `ValidateReferences` la accetta.
	// E' pero' un anello che il runtime dovrebbe percorrere su una porta che sta gia' cambiando stato, ed e'
	// un difetto d'asset con reason code — non un caso da scoprire quando qualcuno lo scrive.
	URTHexMapAsset* Map = MakeGraphMap(2);
	PutGraphDoor(Map, FRTCellId(0, 0, 0), ERTHexDirection::E, 1, TEXT("S1"));
	PutGraphDoor(Map, FRTCellId(1, 0, 0), ERTHexDirection::E, 2, TEXT("D1"));
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D1"), TEXT("S1") }));

	const TArray<FString> Errors = URTStructureIdentityLibrary::ValidateInteractionGraph(Map);

	TestTrue(TEXT("il binding riflessivo e' segnalato"), Errors.Num() > 0);
	TestTrue(TEXT("il reason code nomina la sorgente"), AnyContains(Errors, TEXT("S1")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteractionGraphOrderHoldsAtScale,
	"RefactorTactics.InteractionGraph.OrderHoldsAtScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTInteractionGraphOrderHoldsAtScale::RunTest(const FString&)
{
	// `N` NON ha un tetto, ed e' dichiarato: un limite sarebbe un numero di bilanciamento inventato senza un
	// caso che lo chieda. Cio' che va difeso e' la proprieta' che un tetto avrebbe protetto per caso —
	// l'ordine regge a scala — e questo test la pinna a `N = 40`, cioe' ben oltre qualunque mappa reale.
	const int32 N = 40;
	URTHexMapAsset* Map = MakeGraphMap(6);
	PutGraphDoor(Map, FRTCellId(0, 0, 0), ERTHexDirection::E, 0, TEXT("S1"));

	// Le porte si inseriscono in ordine INVERSO rispetto a quello dichiarato: se la risoluzione seguisse
	// l'asset invece del binding, la sequenza uscirebbe capovolta.
	TArray<FName> Declared;
	TArray<FRTCellId> Expected;
	for (int32 I = 0; I < N; ++I)
	{
		Declared.Add(FName(*FString::Printf(TEXT("D%d"), I)));
		Expected.Add(FRTCellId(1, I - 5, 0));
	}
	for (int32 I = N - 1; I >= 0; --I)
	{
		PutGraphDoor(Map, Expected[I], ERTHexDirection::E, I + 1, Declared[I]);
	}
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), Declared));

	const TArray<FRTStructureEdgeRef> Targets =
		URTStructureIdentityLibrary::ResolveInteractionTargets(Map, TEXT("S1"));

	TestEqual(TEXT("quaranta bersagli risolti"), Targets.Num(), N);
	if (Targets.Num() == N)
	{
		bool bOrdered = true;
		for (int32 I = 0; I < N; ++I)
		{
			bOrdered = bOrdered && (Targets[I].Cell == Expected[I]);
		}
		TestTrue(TEXT("l'ordine dichiarato regge per tutti e quaranta"), bOrdered);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteractionGraphResolutionIgnoresDoorState,
	"RefactorTactics.InteractionGraph.ResolutionIgnoresDoorState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTInteractionGraphResolutionIgnoresDoorState::RunTest(const FString&)
{
	// La risoluzione dice CHI e' comandato, non se il comando andra' a buon fine. Una `Locked` risolve come
	// le altre: `SetDoorState` non la apre, ma quella e' **applicabilita'**.
	//
	// ✅ **[D-150] (2026-08-16) ha deciso l'esito, e conferma questa separazione**: l'operazione su N bersagli
	// NON e' atomica — applica gli applicabili e RIPORTA gli altri con reason code. Segue cio' che il motore
	// gia' fa un livello sotto (`RTHexDoorLibrary.cpp`: un bordo che non puo' transitare viene saltato, gli
	// altri commutano). Quindi la risoluzione dice *chi* e' comandato e l'applicazione *cosa e' cambiato*, ed
	// e' il test dell'APPLICAZIONE — che non esiste ancora — a dover pinnare l'esito per-bersaglio.
	//
	// ⚠️ Il test esiste per impedire la scorciatoia opposta: filtrare qui le `Locked`. La ragione ORIGINALE
	// era che avrebbe reso silenziosamente parziale un'operazione dichiarata atomica — e con [D-150] quella
	// ragione decade, perche' l'operazione parziale lo e' per decisione. Il divieto resta, per una ragione
	// piu' forte: filtrandole, un bersaglio comandato-ma-non-applicabile diventerebbe indistinguibile da uno
	// **non comandato affatto**, e l'esito per-bersaglio che D-150 pretende non potrebbe piu' riportarlo.
	URTHexMapAsset* Map = MakeGraphMap(2);
	PutGraphDoor(Map, FRTCellId(0, 0, 0), ERTHexDirection::E, 1, TEXT("S1"));

	FRTHexCellData Locked(FRTCellId(1, 0, 0));
	FRTHexDoor LockedDoor(ERTHexDirection::E, ERTHexDoorState::Locked, 2);
	LockedDoor.StableId = TEXT("D1");
	Locked.Doors.Add(LockedDoor);
	Map->AddOrUpdateCell(Locked);
	Map->SortCells();

	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D1") }));

	const TArray<FRTStructureEdgeRef> Targets =
		URTStructureIdentityLibrary::ResolveInteractionTargets(Map, TEXT("S1"));

	TestEqual(TEXT("una porta Locked risolve come le altre"), Targets.Num(), 1);
	TestEqual(TEXT("e non e' un errore d'asset"),
		URTStructureIdentityLibrary::ValidateInteractionGraph(Map).Num(), 0);
	return true;
}

/**
 * ── L'APPLICAZIONE ────────────────────────────────────────────────────────────────────────────────
 *
 * I test qui sopra provano la RISOLUZIONE: chi e' comandato. Questi provano cosa e' CAMBIATO, che e' l'altra
 * meta' dichiarata da [D-150] e che il DoD di #833 registrava come «resta da fare».
 *
 * ⚠️ **Cosa questi test NON provano, detto qui perche' non venga letto di piu' di quel che c'e'**: che un
 * giocatore possa aprire una porta remota in partita. Il percorso runtime dell'`Interact` passa da
 * `RTHexCombatLibrary` su `FirstDoorEdge` (CP 9.3) e apre la porta ADIACENTE bersagliata; nessuno chiama
 * ancora `ApplyInteraction`. Quel collegamento vive in `Combat/` e `Turn/RTTurnManager.cpp`, fuori dal
 * write-set di questa track.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteractionGraphSourceOpensTarget,
	"RefactorTactics.InteractionGraph.SourceOpensTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTInteractionGraphSourceOpensTarget::RunTest(const FString&)
{
	// `1 -> 1`: la sorgente comanda un bersaglio, il bersaglio si apre, e la revisione si muove UNA volta.
	URTHexMapAsset* Map = MakeGraphMap(2);
	PutGraphDoor(Map, FRTCellId(0, 0, 0), ERTHexDirection::E, 1, TEXT("S1"));
	PutGraphDoor(Map, FRTCellId(1, 0, 0), ERTHexDirection::E, 2, TEXT("D1"));
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D1") }));

	const int32 RevisionBefore = Map->Revision;
	TArray<FString> Refusals;
	const TArray<FRTDoorChange> Changes =
		URTHexDoorLibrary::ApplyInteraction(Map, TEXT("S1"), ERTHexDoorState::Open, 7, &Refusals);

	TestEqual(TEXT("il bersaglio remoto e' cambiato"), Changes.Num(), 1);
	TestEqual(TEXT("nessun rifiuto"), Refusals.Num(), 0);
	TestEqual(TEXT("la revisione si incrementa UNA volta"), Map->Revision, RevisionBefore + 1);

	// ⚠️ L'asserzione che conta e' sullo STATO DELLA MAPPA, non sul valore di ritorno: una funzione che
	// restituisse le voci giuste senza scrivere passerebbe tutto il resto.
	TestEqual(TEXT("e la porta e' davvero Open sulla mappa"),
		static_cast<int32>(URTHexDoorLibrary::DoorBetween(Map, FRTCellId(1, 0, 0), FRTCellId(2, 0, 0))),
		static_cast<int32>(ERTHexDoorState::Open));

	// La sorgente NON e' un bersaglio di se stessa: resta come era. Senza, «comanda» e «si apre» sarebbero
	// indistinguibili sul caso piu' semplice.
	TestEqual(TEXT("la sorgente non si e' aperta da sola"),
		static_cast<int32>(URTHexDoorLibrary::DoorBetween(Map, FRTCellId(0, 0, 0), FRTCellId(1, 0, 0))),
		static_cast<int32>(ERTHexDoorState::Closed));

	if (Changes.Num() == 1)
	{
		TestEqual(TEXT("chi ha comandato viaggia con l'esito"), Changes[0].ActorId, 7);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteractionGraphMultiTargetIsOneRevision,
	"RefactorTactics.InteractionGraph.MultiTargetIsOneRevision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTInteractionGraphMultiTargetIsOneRevision::RunTest(const FString&)
{
	// `1 -> N`: **una** voce di revisione, non N. E' il requisito che ha imposto di separare la commutazione
	// dal commit: applicando i bersagli uno per uno con `SetDoorState`, la revisione si muoverebbe tre volte e
	// chi la osserva per invalidare una cache vedrebbe tre eventi dove ce n'e' stato uno.
	URTHexMapAsset* Map = MakeGraphMap(3);
	PutGraphDoor(Map, FRTCellId(0, 0, 0), ERTHexDirection::E, 1, TEXT("S1"));
	PutGraphDoor(Map, FRTCellId(1, 0, 0), ERTHexDirection::E, 2, TEXT("D1"));
	PutGraphDoor(Map, FRTCellId(0, 1, 0), ERTHexDirection::E, 3, TEXT("D2"));
	PutGraphDoor(Map, FRTCellId(-1, 1, 0), ERTHexDirection::E, 4, TEXT("D3"));
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D1"), TEXT("D2"), TEXT("D3") }));

	const int32 RevisionBefore = Map->Revision;
	TArray<FString> Refusals;
	const TArray<FRTDoorChange> Changes =
		URTHexDoorLibrary::ApplyInteraction(Map, TEXT("S1"), ERTHexDoorState::Open, INDEX_NONE, &Refusals);

	TestEqual(TEXT("tre bersagli cambiati"), Changes.Num(), 3);
	TestEqual(TEXT("nessun rifiuto"), Refusals.Num(), 0);
	TestEqual(TEXT("UNA revisione per tre bersagli"), Map->Revision, RevisionBefore + 1);

	// I tre sono aperti DAVVERO: il conteggio delle voci da solo non lo dice.
	TestEqual(TEXT("D1 aperta"),
		static_cast<int32>(URTHexDoorLibrary::DoorBetween(Map, FRTCellId(1, 0, 0), FRTCellId(2, 0, 0))),
		static_cast<int32>(ERTHexDoorState::Open));
	TestEqual(TEXT("D2 aperta"),
		static_cast<int32>(URTHexDoorLibrary::DoorBetween(Map, FRTCellId(0, 1, 0), FRTCellId(1, 1, 0))),
		static_cast<int32>(ERTHexDoorState::Open));
	TestEqual(TEXT("D3 aperta"),
		static_cast<int32>(URTHexDoorLibrary::DoorBetween(Map, FRTCellId(-1, 1, 0), FRTCellId(0, 1, 0))),
		static_cast<int32>(ERTHexDoorState::Open));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteractionGraphRefusedTargetDoesNotStopTheOthers,
	"RefactorTactics.InteractionGraph.RefusedTargetDoesNotStopTheOthers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTInteractionGraphRefusedTargetDoesNotStopTheOthers::RunTest(const FString&)
{
	// [D-150]: l'operazione NON e' atomica. Una `Locked` in mezzo agli N si riporta con reason code e gli altri
	// si aprono lo stesso. Il caso ha un esito DECISO, e questo test e' cio' che impedisce di reintrodurre
	// «tutto o niente» — che richiederebbe una pre-validazione su tutti gli N, dato che la commutazione non
	// sa tornare indietro.
	URTHexMapAsset* Map = MakeGraphMap(3);
	PutGraphDoor(Map, FRTCellId(0, 0, 0), ERTHexDirection::E, 1, TEXT("S1"));
	PutGraphDoor(Map, FRTCellId(1, 0, 0), ERTHexDirection::E, 2, TEXT("D1"));
	PutGraphDoorInState(Map, FRTCellId(0, 1, 0), ERTHexDirection::E, 3, TEXT("D2"), ERTHexDoorState::Locked);
	PutGraphDoor(Map, FRTCellId(-1, 1, 0), ERTHexDirection::E, 4, TEXT("D3"));
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D1"), TEXT("D2"), TEXT("D3") }));

	TArray<FString> Refusals;
	const TArray<FRTDoorChange> Changes =
		URTHexDoorLibrary::ApplyInteraction(Map, TEXT("S1"), ERTHexDoorState::Open, INDEX_NONE, &Refusals);

	TestEqual(TEXT("due bersagli su tre sono cambiati"), Changes.Num(), 2);
	TestEqual(TEXT("il terzo e' riportato, non taciuto"), Refusals.Num(), 1);
	TestTrue(TEXT("il reason code nomina la struttura rifiutata"), AnyContains(Refusals, TEXT("D2")));

	// ⚠️ Il reason code deve dire PERCHE', non solo che: senza lo stato, il giocatore preme e non sa se la
	// porta e' bloccata, gia' aperta o inesistente — tre casi con rimedi diversi.
	TestTrue(TEXT("e dice che era Locked"), AnyContains(Refusals, TEXT("Locked")));

	TestEqual(TEXT("la Locked e' rimasta Locked"),
		static_cast<int32>(URTHexDoorLibrary::DoorBetween(Map, FRTCellId(0, 1, 0), FRTCellId(1, 1, 0))),
		static_cast<int32>(ERTHexDoorState::Locked));
	TestEqual(TEXT("ma D3, che viene DOPO di lei, si e' aperta"),
		static_cast<int32>(URTHexDoorLibrary::DoorBetween(Map, FRTCellId(-1, 1, 0), FRTCellId(0, 1, 0))),
		static_cast<int32>(ERTHexDoorState::Open));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteractionGraphWideDoorIsOneTargetNotThreeRefusals,
	"RefactorTactics.InteractionGraph.WideDoorIsOneTargetNotThreeRefusals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTInteractionGraphWideDoorIsOneTargetNotThreeRefusals::RunTest(const FString&)
{
	// 🔴 Il PORTONE LARGO, che e' il caso su cui l'applicazione sbaglia se legge «nessun cambio» come
	// «rifiutato». `FindDoorEdges` restituisce tutti i bordi che portano quel nome — un portone di due bordi
	// arriva come DUE bersagli — e la commutazione propaga sul `DoorId`: il primo bersaglio apre l'intero
	// gruppo, il secondo trova il lavoro fatto. Senza il discrimine sullo stato, meta' portone risulterebbe
	// rifiutata mentre e' aperta, e il giocatore leggerebbe un fallimento che non c'e' stato.
	URTHexMapAsset* Map = MakeGraphMap(2);
	PutGraphDoor(Map, FRTCellId(0, 0, 0), ERTHexDirection::E, 1, TEXT("S1"));
	// Due bordi, UN portone: stesso `DoorId` e stesso nome pubblico.
	PutGraphDoor(Map, FRTCellId(1, 0, 0), ERTHexDirection::E, 2, TEXT("D1"));
	PutGraphDoor(Map, FRTCellId(1, -1, 0), ERTHexDirection::E, 2, TEXT("D1"));
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D1") }));

	// La premessa del test, asserita invece che assunta: il nome risolve DUE bordi.
	TestEqual(TEXT("il portone e' due bordi"),
		URTStructureIdentityLibrary::ResolveInteractionTargets(Map, TEXT("S1")).Num(), 2);

	const int32 RevisionBefore = Map->Revision;
	TArray<FString> Refusals;
	const TArray<FRTDoorChange> Changes =
		URTHexDoorLibrary::ApplyInteraction(Map, TEXT("S1"), ERTHexDoorState::Open, INDEX_NONE, &Refusals);

	TestEqual(TEXT("il portone si apre tutto insieme"), Changes.Num(), 2);
	TestEqual(TEXT("e NESSUN bordo risulta rifiutato"), Refusals.Num(), 0);
	TestEqual(TEXT("un portone largo si apre una volta, non due"), Map->Revision, RevisionBefore + 1);

	TestEqual(TEXT("primo bordo aperto"),
		static_cast<int32>(URTHexDoorLibrary::DoorBetween(Map, FRTCellId(1, 0, 0), FRTCellId(2, 0, 0))),
		static_cast<int32>(ERTHexDoorState::Open));
	TestEqual(TEXT("secondo bordo aperto"),
		static_cast<int32>(URTHexDoorLibrary::DoorBetween(Map, FRTCellId(1, -1, 0), FRTCellId(2, -1, 0))),
		static_cast<int32>(ERTHexDoorState::Open));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteractionGraphWideDoorRefusesOnceNotPerEdge,
	"RefactorTactics.InteractionGraph.WideDoorRefusesOnceNotPerEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTInteractionGraphWideDoorRefusesOnceNotPerEdge::RunTest(const FString&)
{
	// 🔴 L'immagine SPECULARE di `WideDoorIsOneTargetNotThreeRefusals`, e il caso che quel test non copriva:
	// li' il portone si apriva tutto e i bordi successivi non erano rifiuti; qui **non commuta nessuno**, e
	// senza deduplica per struttura il giocatore leggerebbe due fallimenti per UNA porta. Trovato da una code
	// review: il percorso di successo era testato, quello in cui rifiutano tutti no.
	URTHexMapAsset* Map = MakeGraphMap(2);
	PutGraphDoor(Map, FRTCellId(0, 0, 0), ERTHexDirection::E, 1, TEXT("S1"));
	PutGraphDoorInState(Map, FRTCellId(1, 0, 0), ERTHexDirection::E, 2, TEXT("D1"), ERTHexDoorState::Locked);
	PutGraphDoorInState(Map, FRTCellId(1, -1, 0), ERTHexDirection::E, 2, TEXT("D1"), ERTHexDoorState::Locked);
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D1") }));

	TestEqual(TEXT("il portone e' due bordi"),
		URTStructureIdentityLibrary::ResolveInteractionTargets(Map, TEXT("S1")).Num(), 2);

	const int32 RevisionBefore = Map->Revision;
	TArray<FString> Refusals;
	const TArray<FRTDoorChange> Changes =
		URTHexDoorLibrary::ApplyInteraction(Map, TEXT("S1"), ERTHexDoorState::Open, INDEX_NONE, &Refusals);

	TestEqual(TEXT("niente si apre: e' Locked"), Changes.Num(), 0);
	TestEqual(TEXT("UN rifiuto per la struttura, non uno per bordo"), Refusals.Num(), 1);
	TestTrue(TEXT("e nomina la struttura"), AnyContains(Refusals, TEXT("D1")));
	TestEqual(TEXT("una transizione rifiutata non muove la revisione"), Map->Revision, RevisionBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteractionGraphDestroyedDoorAlreadyGrantsOpen,
	"RefactorTactics.InteractionGraph.DestroyedDoorAlreadyGrantsOpen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTInteractionGraphDestroyedDoorAlreadyGrantsOpen::RunTest(const FString&)
{
	// 🔴 Un varco SFONDATO non nega passo ne' vista, quindi chi ordina «apri» ha gia' cio' che voleva — anche
	// se `Destroyed != Open` come valore di enum e la transizione e' vietata. Riportarlo come rifiuto manderebbe
	// il giocatore a cercare un'altra strada che non gli serve. Il criterio e' la proprieta' OSSERVABILE.
	URTHexMapAsset* Map = MakeGraphMap(2);
	PutGraphDoor(Map, FRTCellId(0, 0, 0), ERTHexDirection::E, 1, TEXT("S1"));
	PutGraphDoorInState(Map, FRTCellId(1, 0, 0), ERTHexDirection::E, 2, TEXT("D1"), ERTHexDoorState::Destroyed);
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D1") }));

	TArray<FString> Refusals;
	const TArray<FRTDoorChange> Changes =
		URTHexDoorLibrary::ApplyInteraction(Map, TEXT("S1"), ERTHexDoorState::Open, INDEX_NONE, &Refusals);

	TestEqual(TEXT("niente cambia: `Destroyed` e' terminale"), Changes.Num(), 0);
	TestEqual(TEXT("ma non e' un rifiuto: il passaggio c'e' gia'"), Refusals.Num(), 0);

	// ⚠️ La meta' che impedisce di scambiare la regola per «`Destroyed` non rifiuta mai»: chiedere di CHIUDERE
	// un varco sfondato e' un fallimento vero, e va riportato.
	TArray<FString> CloseRefusals;
	URTHexDoorLibrary::ApplyInteraction(Map, TEXT("S1"), ERTHexDoorState::Closed, INDEX_NONE, &CloseRefusals);
	TestEqual(TEXT("chiudere un varco sfondato invece rifiuta"), CloseRefusals.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteractionGraphBindingThatResolvesToNothingIsReported,
	"RefactorTactics.InteractionGraph.BindingThatResolvesToNothingIsReported",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTInteractionGraphBindingThatResolvesToNothingIsReported::RunTest(const FString&)
{
	// 🔴 Un binding che ESISTE e non risolve nessuna porta non deve essere indistinguibile da «nessun binding».
	// `ValidateInteractionGraph` prende il caso, ma in questo repository i gate si eseguono **a mano**: se
	// l'asset arriva a runtime cosi', il giocatore preme una leva che non fa niente e nessuno sa perche'.
	URTHexMapAsset* Map = MakeGraphMap(2);
	PutGraphDoor(Map, FRTCellId(0, 0, 0), ERTHexDirection::E, 1, TEXT("S1"));
	PutGraphArc(Map, FRTCellId(0, 0, 0), FRTCellId(0, 0, 1), TEXT("A1"));
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("A1") }));

	TArray<FString> Refusals;
	const TArray<FRTDoorChange> Changes =
		URTHexDoorLibrary::ApplyInteraction(Map, TEXT("S1"), ERTHexDoorState::Open, INDEX_NONE, &Refusals);

	TestEqual(TEXT("non cambia niente"), Changes.Num(), 0);
	TestEqual(TEXT("ma il silenzio e' rotto"), Refusals.Num(), 1);
	TestTrue(TEXT("e il reason code nomina la sorgente"), AnyContains(Refusals, TEXT("S1")));

	// ⚠️ La meta' che tiene la regola stretta: una sorgente SENZA binding e' legale e non produce reason code.
	// Senza questa asserzione, «riporta sempre» passerebbe il test qui sopra.
	TArray<FString> NoBinding;
	URTHexDoorLibrary::ApplyInteraction(Map, TEXT("D_INESISTENTE"), ERTHexDoorState::Open, INDEX_NONE, &NoBinding);
	TestEqual(TEXT("una sorgente senza binding resta silenziosa"), NoBinding.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteractionGraphApplicationFollowsDeclaredOrder,
	"RefactorTactics.InteractionGraph.ApplicationFollowsDeclaredOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTInteractionGraphApplicationFollowsDeclaredOrder::RunTest(const FString&)
{
	// L'ordine delle voci prodotte e' quello DICHIARATO in `TargetIds`, non l'ordine canonico di cella: se lo
	// fosse, l'unica cosa osservabile dell'ordine di applicazione sparirebbe dentro un `Sort` finale.
	//
	// ⚠️ **Si perturba l'ordine di dichiarazione, non si ripete la stessa chiamata**: dentro lo stesso processo
	// anche un ordine sbagliato e' ripetibile — e' la stessa ragione per cui `OrderIgnoresAssetInsertionOrder`
	// costruisce due mappe invece di risolvere due volte. Qui le due liste sono l'una l'inverso dell'altra e
	// **contro l'ordine di cella**, cosi' un `Sort` globale farebbe cadere una delle due asserzioni.
	const FRTCellId First(1, 0, 0);
	const FRTCellId Second(-1, 1, 0);

	URTHexMapAsset* Forward = MakeGraphMap(3);
	PutGraphDoor(Forward, FRTCellId(0, 0, 0), ERTHexDirection::E, 1, TEXT("S1"));
	PutGraphDoor(Forward, First, ERTHexDirection::E, 2, TEXT("D1"));
	PutGraphDoor(Forward, Second, ERTHexDirection::E, 3, TEXT("D2"));
	Forward->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D1"), TEXT("D2") }));

	URTHexMapAsset* Reverse = MakeGraphMap(3);
	PutGraphDoor(Reverse, FRTCellId(0, 0, 0), ERTHexDirection::E, 1, TEXT("S1"));
	PutGraphDoor(Reverse, First, ERTHexDirection::E, 2, TEXT("D1"));
	PutGraphDoor(Reverse, Second, ERTHexDirection::E, 3, TEXT("D2"));
	Reverse->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D2"), TEXT("D1") }));

	const TArray<FRTDoorChange> A =
		URTHexDoorLibrary::ApplyInteraction(Forward, TEXT("S1"), ERTHexDoorState::Open);
	const TArray<FRTDoorChange> B =
		URTHexDoorLibrary::ApplyInteraction(Reverse, TEXT("S1"), ERTHexDoorState::Open);

	TestEqual(TEXT("due voci per la lista diretta"), A.Num(), 2);
	TestEqual(TEXT("due voci per la lista inversa"), B.Num(), 2);
	if (A.Num() == 2 && B.Num() == 2)
	{
		TestTrue(TEXT("la lista diretta applica D1 per prima"), A[0].Cell == First);
		TestTrue(TEXT("la lista inversa applica D2 per prima"), B[0].Cell == Second);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
