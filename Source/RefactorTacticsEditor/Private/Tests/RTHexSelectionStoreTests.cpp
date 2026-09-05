#include "Misc/AutomationTest.h"

#include "Map/RTCellId.h"
#include "Map/RTGeometryGrammar.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTMapDependencyLibrary.h"
#include "Map/RTMapEditLibrary.h"
#include "RTHexSelectionStore.h"
#include "Turn/RTMatchSetupLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * La selezione condivisa del mode Hex Map (#1864), e il ciclo sui candidati.
 *
 * 🔴 **Perche' non e' una proprieta' del tool**: #921 ha misurato il difetto opposto — `bShowOverlay` vive
 * in due `UInteractiveToolPropertySet` distinti, quindi accenderlo in Select non lo accende in Paint e
 * cambiando strumento si perde. Uno stato che deve sopravvivere al cambio di tool non puo' stare nel tool.
 *
 * ⚠️ Lo store si istanzia con `NewObject` invece di prenderlo da `GEditor`: e' un `UObject` e non usa nulla
 * dell'inizializzazione del subsystem, quindi il test non ha bisogno di un editor vivo. Se un giorno
 * `Initialize()` gli servisse, questo test lo direbbe.
 */
namespace
{
	constexpr float SelStoreHexSize = 100.f;

	/** Nome prefissato per dominio: unity build. */
	URTHexMapAsset* SelStoreMakeMap()
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 2);
		M->HexSize = SelStoreHexSize;
		return M;
	}

	/**
	 * Una cella con porta E copertura sullo stesso bordo, piu' un muro interno.
	 *
	 * Quattro candidati sotto un punto solo — ed e' uno stato che `ValidateMap` **permette**, non un caso
	 * costruito per il test: la coppia vietata e' porta + copertura `High`, non `Low`.
	 */
	URTHexMapAsset* SelStoreMakeCrowdedMap(const FRTCellId& Cell, ERTHexDirection Edge)
	{
		URTHexMapAsset* M = SelStoreMakeMap();

		FRTHexCellData Data = *M->FindCell(Cell);
		FRTHexDoor Door(Edge, ERTHexDoorState::Closed, /*DoorId*/ 1);
		Door.StableId = TEXT("D1");
		Data.Doors.Add(Door);
		FRTHexCover Cover;
		Cover.Edge = Edge;
		Cover.Type = ERTHexCoverType::Low;
		Data.Covers.Add(Cover);
		M->AddOrUpdateCell(Data);

		const double Angle = PI / 6.0;
		const FVector2D A(SelStoreHexSize * FMath::Cos(Angle), SelStoreHexSize * FMath::Sin(Angle));
		FRTGeometrySegment Chord;
		if (URTGeometryGrammarLibrary::SnapToGrammar(A, FVector2D(-A.X, -A.Y), SelStoreHexSize, Chord))
		{
			Chord.WallType = ERTHexCoverType::High;
			FRTHexInteriorWall Wall(Cell, Chord);
			Wall.StableId = TEXT("W1");
			M->InteriorWalls.Add(Wall);
		}

		return M;
	}

	/**
	 * Il `Kind` del primo elemento selezionato, o `None` se non c'e' selezione.
	 *
	 * 🔴 **Esiste perche' un test deve FALLIRE, non far cadere il processo.** La prima stesura leggeva il
	 * primo elemento senza guardia: con lo store ancora stub l'array era vuoto e l'accesso ha fatto scattare
	 * l'assert di `TArray` — *«Array index out of bounds: 0 into an array of size 0»*. Il crash ha portato
	 * giu' la run, e il test successivo **non e' mai stato eseguito**: il RED che avrebbe dovuto provarlo
	 * non e' mai esistito.
	 *
	 * Un fallimento si legge e lascia girare il resto; un crash cancella anche l'evidenza degli altri.
	 */
	ERTMapElementKind SelStoreFirstKind(const URTHexSelectionStore* Store)
	{
		const TArray<FRTMapElementHandle>& Sel = Store->GetSelection();
		return Sel.Num() > 0 ? Sel[0].Kind : ERTMapElementKind::None;
	}
}

/**
 * 🔑 Click ripetuti sullo stesso punto CICLANO sui candidati, e poi ricominciano.
 *
 * E' la risposta al rilievo `A1` del panel su #1864: con una priorita' fissa, la copertura sotto la porta
 * non sarebbe raggiungibile col click — e quello stato e' legale, quindi sarebbe un elemento autorabile e
 * non selezionabile.
 *
 * ⚠️ Il test verifica anche il **ritorno all'inizio**: un ciclo che si fermasse sull'ultimo candidato
 * lascerebbe l'utente senza modo di tornare al primo se non cliccando altrove e indietro.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSelectionStoreCyclesTest,
	"RefactorTactics.Editor.Selection.RepeatedClicksCycleThroughCandidates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSelectionStoreCyclesTest::RunTest(const FString&)
{
	const FRTCellId Cell(0, 0, 0);
	const ERTHexDirection Edge = ERTHexDirection::E;

	URTHexMapAsset* Map = SelStoreMakeCrowdedMap(Cell, Edge);

	// Controprova sull'allestimento: lo stato e' legale e i candidati sono davvero quattro.
	if (!TestEqual(TEXT("l'allestimento e' valido"), Map->ValidateMap().Num(), 0))
	{
		return false;
	}
	if (!TestEqual(TEXT("quattro candidati sotto quel punto"),
		URTMapEditLibrary::ElementsAt(Map, Cell, Edge).Num(), 4))
	{
		return false;
	}

	URTHexSelectionStore* Store = NewObject<URTHexSelectionStore>();

	const ERTMapElementKind Expected[] = {
		ERTMapElementKind::Door,
		ERTMapElementKind::Cover,
		ERTMapElementKind::InteriorWall,
		ERTMapElementKind::Cell,
		ERTMapElementKind::Door // e si ricomincia
	};

	for (int32 Click = 0; Click < UE_ARRAY_COUNT(Expected); ++Click)
	{
		if (!TestTrue(*FString::Printf(TEXT("click %d riesce"), Click + 1),
			Store->SelectAt(Map, Cell, Edge)))
		{
			return false;
		}
		if (!TestEqual(*FString::Printf(TEXT("click %d seleziona UN elemento"), Click + 1),
			Store->GetSelection().Num(), 1))
		{
			return false;
		}
		TestEqual(*FString::Printf(TEXT("click %d prende il candidato atteso"), Click + 1),
			SelStoreFirstKind(Store), Expected[Click]);
	}

	return true;
}

/**
 * Cliccare ALTROVE azzera il ciclo: il punto nuovo riparte dal candidato piu' specifico.
 *
 * ⚠️ Senza questo, il ciclo diventerebbe un contatore globale e il primo click su una cella nuova
 * prenderebbe un elemento a caso, a seconda di quanti click erano stati fatti prima da un'altra parte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSelectionStoreResetsTest,
	"RefactorTactics.Editor.Selection.ClickingElsewhereRestartsTheCycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSelectionStoreResetsTest::RunTest(const FString&)
{
	const FRTCellId Cell(0, 0, 0);
	const ERTHexDirection Edge = ERTHexDirection::E;

	URTHexMapAsset* Map = SelStoreMakeCrowdedMap(Cell, Edge);
	URTHexSelectionStore* Store = NewObject<URTHexSelectionStore>();

	// Due click qui: siamo alla copertura.
	Store->SelectAt(Map, Cell, Edge);
	Store->SelectAt(Map, Cell, Edge);
	if (!TestEqual(TEXT("il secondo click e' sulla copertura"),
		SelStoreFirstKind(Store), ERTMapElementKind::Cover))
	{
		return false;
	}

	// Stesso bordo, cella diversa: e' un altro punto.
	const FRTCellId Elsewhere(1, 0, 0);
	if (!TestTrue(TEXT("il click altrove riesce"), Store->SelectAt(Map, Elsewhere, Edge)))
	{
		return false;
	}
	TestEqual(TEXT("e riparte dal piu' specifico di LA'"),
		SelStoreFirstKind(Store), ERTMapElementKind::Cell);

	// Tornando al punto affollato si riparte da capo, non dalla copertura dove eravamo rimasti.
	Store->SelectAt(Map, Cell, Edge);
	TestEqual(TEXT("tornando indietro il ciclo e' ripartito"),
		SelStoreFirstKind(Store), ERTMapElementKind::Door);

	return true;
}

/**
 * `AddAt` accumula, non sostituisce — e non aggiunge due volte lo stesso elemento.
 *
 * 🔴 **Il duplicato non e' pignoleria**: la cancellazione itera la selezione, e due copie dello stesso
 * handle proverebbero a rimuovere due volte lo stesso elemento — la seconda su indici gia' spostati.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSelectionStoreAccumulatesTest,
	"RefactorTactics.Editor.Selection.AddAccumulatesWithoutDuplicates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSelectionStoreAccumulatesTest::RunTest(const FString&)
{
	const FRTCellId First(0, 0, 0);
	const FRTCellId Second(1, 0, 0);
	const ERTHexDirection Edge = ERTHexDirection::E;

	URTHexMapAsset* Map = SelStoreMakeCrowdedMap(First, Edge);
	URTHexSelectionStore* Store = NewObject<URTHexSelectionStore>();

	if (!TestTrue(TEXT("il primo click riesce"), Store->SelectAt(Map, First, Edge)))
	{
		return false;
	}
	if (!TestTrue(TEXT("l'aggiunta riesce"), Store->AddAt(Map, Second, Edge)))
	{
		return false;
	}
	if (!TestEqual(TEXT("due elementi selezionati"), Store->GetSelection().Num(), 2))
	{
		return false;
	}

	// Riaggiungere lo stesso punto non cresce la selezione.
	Store->AddAt(Map, Second, Edge);
	TestEqual(TEXT("lo stesso elemento non entra due volte"), Store->GetSelection().Num(), 2);

	Store->Clear();
	if (!TestEqual(TEXT("Clear svuota"), Store->GetSelection().Num(), 0))
	{
		return false;
	}

	// E azzera anche il ciclo: dopo un Clear, il primo click sul punto affollato riparte dal piu' specifico.
	Store->SelectAt(Map, First, Edge);
	TestEqual(TEXT("dopo Clear il ciclo riparte da capo"),
		SelStoreFirstKind(Store), ERTMapElementKind::Door);

	return true;
}

/**
 * Il readout dichiara TIPO e IDENTITA', non «un elemento».
 *
 * ⚠️ E' anche l'unico modo in cui chi clicca capisce a che punto del ciclo si trova: un pannello che dicesse
 * «1 elemento selezionato» lascerebbe indovinare se il prossimo Erase toglie la porta o la cella sotto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSelectionStoreDescribeTest,
	"RefactorTactics.Editor.Selection.ReadoutNamesTypeAndIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSelectionStoreDescribeTest::RunTest(const FString&)
{
	const FRTCellId Cell(1, 2, 0);

	TestEqual(TEXT("selezione vuota"), URTHexSelectionStore::Describe({}), FString(TEXT("niente")));

	// La cella nomina le proprie coordinate.
	const FString CellText = URTHexSelectionStore::Describe({ FRTMapElementHandle::ForCell(Cell) });
	TestTrue(*FString::Printf(TEXT("la cella dichiara il tipo (era: %s)"), *CellText),
		CellText.Contains(TEXT("cella")));
	TestTrue(*FString::Printf(TEXT("e le coordinate (era: %s)"), *CellText),
		CellText.Contains(TEXT("1")) && CellText.Contains(TEXT("2")));

	// La porta nomina il proprio StableId: e' la sua identita' pubblica.
	const FString DoorText = URTHexSelectionStore::Describe(
		{ FRTMapElementHandle::ForDoor(Cell, ERTHexDirection::E, TEXT("D1")) });
	TestTrue(*FString::Printf(TEXT("la porta dichiara il tipo (era: %s)"), *DoorText),
		DoorText.Contains(TEXT("porta")));
	TestTrue(*FString::Printf(TEXT("e il suo nome (era: %s)"), *DoorText),
		DoorText.Contains(TEXT("D1")));

	// Un muro ANONIMO non ha un nome da mostrare, e il readout non deve inventarne uno.
	FRTGeometrySegment Chord;
	const double Angle = PI / 6.0;
	const FVector2D A(SelStoreHexSize * FMath::Cos(Angle), SelStoreHexSize * FMath::Sin(Angle));
	if (URTGeometryGrammarLibrary::SnapToGrammar(A, FVector2D(-A.X, -A.Y), SelStoreHexSize, Chord))
	{
		const FString WallText =
			URTHexSelectionStore::Describe({ FRTMapElementHandle::ForInteriorWallAt(Cell, Chord) });
		TestTrue(*FString::Printf(TEXT("il muro dichiara il tipo (era: %s)"), *WallText),
			WallText.Contains(TEXT("muro")));
		TestFalse(*FString::Printf(TEXT("e non si inventa un nome (era: %s)"), *WallText),
			WallText.Contains(TEXT("None")));
	}

	// Piu' elementi: il readout dice quanti sono invece di elencarli tutti.
	const FString ManyText = URTHexSelectionStore::Describe({
		FRTMapElementHandle::ForCell(Cell),
		FRTMapElementHandle::ForCell(FRTCellId(0, 0, 0)) });
	TestTrue(*FString::Printf(TEXT("la multi-selezione dichiara il conteggio (era: %s)"), *ManyText),
		ManyText.Contains(TEXT("2")));

	return true;
}

/**
 * 🔴 Ctrl+click ripetuto sullo STESSO punto cicla l'ULTIMO aggiunto, e non resta inchiodato al bordo.
 *
 * **Trovato in seduta**: con l'aggiunta che prendeva sempre il candidato piu' specifico, una cella con una
 * copertura sul bordo mirato era **impossibile da aggiungere** — Ctrl+click dava la copertura, sempre, e non
 * c'era nessun gesto per scendere alla cella. Un elemento raggiungibile col click semplice e irraggiungibile
 * con quello multiplo e' la stessa forma del rilievo `A1`, spostata di un tasto.
 *
 * ⚠️ **Ciclare l'ultimo e non tutta la selezione** e' la parte da non sbagliare: gli elementi presi prima
 * restano dove sono, altrimenti costruire una selezione di tre elementi sarebbe impossibile — l'ultimo
 * gesto disferebbe i precedenti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSelectionStoreAddCyclesLastTest,
	"RefactorTactics.Editor.Selection.RepeatedAddCyclesTheLastElement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSelectionStoreAddCyclesLastTest::RunTest(const FString&)
{
	const FRTCellId First(0, 0, 0);
	const FRTCellId Crowded(1, 0, 0);
	const ERTHexDirection Edge = ERTHexDirection::E;

	// Il punto affollato e' il SECONDO: cosi' il test prova che il ciclo dell'aggiunta non tocca cio' che
	// era gia' selezionato.
	URTHexMapAsset* Map = SelStoreMakeCrowdedMap(Crowded, Edge);
	URTHexSelectionStore* Store = NewObject<URTHexSelectionStore>();

	if (!TestTrue(TEXT("il primo click riesce"), Store->SelectAt(Map, First, Edge)))
	{
		return false;
	}
	const ERTMapElementKind FirstKind = SelStoreFirstKind(Store);

	// Prima aggiunta: il piu' specifico del punto affollato.
	if (!TestTrue(TEXT("l'aggiunta riesce"), Store->AddAt(Map, Crowded, Edge)))
	{
		return false;
	}
	if (!TestEqual(TEXT("due elementi"), Store->GetSelection().Num(), 2))
	{
		return false;
	}
	TestEqual(TEXT("e il secondo e' il piu' specifico"),
		Store->GetSelection()[1].Kind, ERTMapElementKind::Door);

	// 🔑 Ri-aggiungere sullo STESSO punto scende al candidato successivo, senza crescere.
	Store->AddAt(Map, Crowded, Edge);
	if (!TestEqual(TEXT("la selezione non cresce: cicla"), Store->GetSelection().Num(), 2))
	{
		return false;
	}
	TestEqual(TEXT("l'ultimo e' sceso alla copertura"),
		Store->GetSelection()[1].Kind, ERTMapElementKind::Cover);

	// E il primo non si e' mosso: e' cio' che rende costruibile una selezione di piu' elementi.
	TestEqual(TEXT("il primo elemento e' rimasto quello"), SelStoreFirstKind(Store), FirstKind);

	// Si arriva fino alla cella, che e' il candidato che prima era irraggiungibile con Ctrl.
	Store->AddAt(Map, Crowded, Edge);
	Store->AddAt(Map, Crowded, Edge);
	TestEqual(TEXT("e si raggiunge la cella"),
		Store->GetSelection()[1].Kind, ERTMapElementKind::Cell);
	TestEqual(TEXT("sempre due elementi"), Store->GetSelection().Num(), 2);

	return true;
}

/**
 * 🔴 Il ciclo dell'aggiunta non deve poter mettere DUE VOLTE lo stesso elemento.
 *
 * Trovato da una code review. Il ramo che cicla scriveva `Selection.Last()` senza passare dal controllo dei
 * duplicati che l'aggiunta normale fa: bastava selezionare una cella e poi ciclare su un punto della STESSA
 * cella fino a tornare su di essa.
 *
 * ⚠️ **Non e' pignoleria e l'header lo dichiarava gia'**: `EraseSelection` itera la selezione e chiama
 * `DeleteElement` per ogni voce — la seconda copia opera su una mappa da cui l'elemento e' gia' sparito,
 * ottiene un rifiuto e logga un avviso che descrive un difetto inesistente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSelectionStoreCycleNoDupTest,
	"RefactorTactics.Editor.Selection.CyclingNeverSelectsTheSameElementTwice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSelectionStoreCycleNoDupTest::RunTest(const FString&)
{
	const FRTCellId Cell(0, 0, 0);
	const ERTHexDirection Crowded = ERTHexDirection::E;
	const ERTHexDirection Bare = ERTHexDirection::NE;

	URTHexMapAsset* Map = SelStoreMakeCrowdedMap(Cell, Crowded);
	URTHexSelectionStore* Store = NewObject<URTHexSelectionStore>();

	// Si seleziona la CELLA passando da un bordo spoglio: li' i candidati sono muro e cella.
	Store->SelectAt(Map, Cell, Bare);
	Store->SelectAt(Map, Cell, Bare);
	if (!TestEqual(TEXT("la cella e' selezionata"), SelStoreFirstKind(Store), ERTMapElementKind::Cell))
	{
		return false;
	}

	// Poi si cicla sul bordo affollato della STESSA cella, fino a passare per la cella.
	for (int32 Click = 0; Click < 6; ++Click)
	{
		Store->AddAt(Map, Cell, Crowded);

		// 🔴 Nessun elemento due volte, mai — a nessun giro del ciclo.
		for (int32 I = 0; I < Store->GetSelection().Num(); ++I)
		{
			for (int32 J = I + 1; J < Store->GetSelection().Num(); ++J)
			{
				const FRTMapElementHandle& A = Store->GetSelection()[I];
				const FRTMapElementHandle& B = Store->GetSelection()[J];
				const bool bSame = (A.Kind == B.Kind) && (A.Cell == B.Cell)
					&& (A.Kind != ERTMapElementKind::Cover || A.Edge == B.Edge);
				TestFalse(*FString::Printf(TEXT("click %d: nessun duplicato in selezione"), Click + 1), bSame);
			}
		}
	}

	return true;
}

/**
 * 🔴 Ctrl+click su un punto GIA' selezionato non lascia il ciclo indietro.
 *
 * Trovato da una code review, ed e' la stessa forma del difetto che questa PR e' venuta a correggere. Il
 * ramo «gia' dentro» usciva senza spostare la memoria del ciclo: tornando su un punto gia' preso, ogni
 * Ctrl+click successivo ricalcolava il candidato piu' specifico, lo trovava duplicato e usciva di nuovo.
 *
 * ∴ gli altri candidati di quel punto restavano **irraggiungibili per sempre** con l'aggiunta — cioe' `A1`
 * un'altra volta, per una porta diversa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSelectionStoreCycleMovesTest,
	"RefactorTactics.Editor.Selection.AddingAnAlreadySelectedPointStillMovesTheCycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSelectionStoreCycleMovesTest::RunTest(const FString&)
{
	const FRTCellId P(0, 0, 0);
	const FRTCellId Q(1, 0, 0);
	const ERTHexDirection Edge = ERTHexDirection::E;

	URTHexMapAsset* Map = SelStoreMakeCrowdedMap(P, Edge);
	URTHexSelectionStore* Store = NewObject<URTHexSelectionStore>();

	// Selezionato il piu' specifico di P; poi si aggiunge Q, e il ciclo si sposta la'.
	Store->SelectAt(Map, P, Edge);
	const ERTMapElementKind AtP = SelStoreFirstKind(Store);
	Store->AddAt(Map, Q, Edge);

	// Si torna su P: il candidato piu' specifico e' gia' dentro, quindi il gesto non aggiunge — ma DEVE
	// spostare il ciclo su P, altrimenti il click seguente ripete lo stesso nulla.
	// ⚠️ **Questa asserzione era mal costruita e passava per la ragione sbagliata**: guardava `Last()`, che
	// dopo l'aggiunta di Q appartiene a Q — quindi era diverso da quello di P qualunque cosa facesse lo
	// store. Ora si conta quanti elementi vengono DA P, che e' la domanda vera.
	auto CountFromP = [&Store, &P]()
	{
		int32 N = 0;
		for (const FRTMapElementHandle& H : Store->GetSelection())
		{
			if (H.Cell == P) { ++N; }
		}
		return N;
	};

	const int32 BeforeReturn = CountFromP();

	// Si torna su P, il cui candidato piu' specifico e' gia' dentro: il gesto deve comunque produrre
	// qualcosa — un altro candidato di P — invece di uscire a vuoto e lasciare il ciclo indietro.
	Store->AddAt(Map, P, Edge);

	if (!TestTrue(TEXT("tornare su P contribuisce un altro suo candidato"),
		CountFromP() > BeforeReturn))
	{
		return false;
	}

	// E il gesto successivo su P scorre ancora, invece di ripetere lo stesso nulla.
	const ERTMapElementKind AfterFirst = Store->GetSelection().Last().Kind;
	Store->AddAt(Map, P, Edge);
	TestNotEqual(TEXT("e il click seguente scorre ancora"),
		Store->GetSelection().Last().Kind, AfterFirst);

	// L'elemento iniziale di P non e' stato buttato via.
	TestEqual(TEXT("il primo elemento di P e' rimasto"), SelStoreFirstKind(Store), AtP);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
