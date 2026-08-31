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

#endif // WITH_DEV_AUTOMATION_TESTS
