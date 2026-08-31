#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Map/RTGeometryGrammar.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTMapDependencyLibrary.h"
#include "Map/RTStructureIdentityLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Le regole di dipendenza dell'authoring (#1864): CHE COSA muore con che cosa.
 *
 * Vivono nel modulo runtime e sono pure, per la stessa ragione per cui `URTGeometryGrammarLibrary` non sta
 * nell'editor: un tool che cancella deve poter chiedere l'elenco dei dipendenti **prima** di aprire una
 * transazione, e la risposta non puo' dipendere da uno stato d'editor.
 *
 * ⚠️ La funzione NON rimuove niente. Raccoglie e basta: chi cancella e' il chiamante, dentro il proprio
 * `FScopedTransaction`. Separare le due cose e' cio' che rende la regola testabile headless.
 */
namespace
{
	constexpr float MapDepHexSize = 100.f;

	/** Esagono pieno di raggio N. Nome prefissato per dominio: unity build. */
	URTHexMapAsset* MapDepMakeMap(int32 Radius)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		M->HexSize = MapDepHexSize;
		return M;
	}

	/**
	 * La corda per due vertici opposti: attraversa la cella per il centro senza giacere su nessun bordo,
	 * quindi e' un muro INTERNO e non una copertura. E' lo stesso segmento di
	 * `Geometry.InteriorWallHygiene`, agganciato alla grammatica invece che scritto in quanti a mano.
	 */
	bool MapDepMakeChord(FRTGeometrySegment& Out)
	{
		const double Deg30 = PI / 6.0;
		const FVector2D A(MapDepHexSize * FMath::Cos(Deg30), MapDepHexSize * FMath::Sin(Deg30));
		const FVector2D B(-A.X, -A.Y);

		if (!URTGeometryGrammarLibrary::SnapToGrammar(A, B, MapDepHexSize, Out))
		{
			return false;
		}
		Out.WallType = ERTHexCoverType::High;
		return true;
	}

	/** Porta con nome pubblico sul bordo indicato, come in `Structures.Identity.*`. */
	void MapDepPutDoor(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge,
		int32 DoorId, FName StableId)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		FRTHexDoor Door(Edge, ERTHexDoorState::Closed, DoorId);
		Door.StableId = StableId;
		Data.Doors.Add(Door);
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}
}

/**
 * Cancellare una cella porta via i muri interni che vivevano SU DI ESSA, e nessun altro.
 *
 * E' il primo dei tre array che sopravvivono a una cella morta — `Covers` e `Doors` vivono DENTRO
 * `FRTHexCellData` e spariscono con lei, senza che nessuno debba raccoglierli.
 *
 * ⚠️ Il secondo muro esiste per essere lasciato in piedi: senza, passerebbe anche un'implementazione che
 * restituisce l'intero array.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapDependencyCellTakesWallsTest,
	"RefactorTactics.Map.Dependency.CellTakesItsInteriorWalls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapDependencyCellTakesWallsTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MapDepMakeMap(2);

	FRTGeometrySegment Chord;
	if (!TestTrue(TEXT("la corda si aggancia alla grammatica"), MapDepMakeChord(Chord)))
	{
		return false;
	}

	const FRTCellId Doomed(0, 0, 0);
	const FRTCellId Spared(1, 0, 0);

	Map->InteriorWalls.Add(FRTHexInteriorWall(Doomed, Chord));
	Map->InteriorWalls.Add(FRTHexInteriorWall(Spared, Chord));

	// Controprova: l'allestimento e' sano. Senza, un errore di setup si leggerebbe come un difetto della
	// regola di dipendenza.
	if (!TestEqual(TEXT("l'allestimento non produce errori di validazione"),
		Map->ValidateMap().Num(), 0))
	{
		return false;
	}

	const FRTMapDependencySet Set =
		URTMapDependencyLibrary::CollectDependents(Map, FRTMapElementHandle::ForCell(Doomed));

	if (!TestEqual(TEXT("un solo muro interno segue la cella cancellata"),
		Set.InteriorWallIndices.Num(), 1))
	{
		return false;
	}

	TestEqual(TEXT("ed e' quello che viveva sulla cella cancellata"),
		Map->InteriorWalls[Set.InteriorWallIndices[0]].Cell, Doomed);

	return true;
}

/**
 * Cancellare una cella porta via le transizioni che la CITAVANO, da entrambi i lati.
 *
 * `ValidateMap` gia' segnala una transizione «verso cella inesistente» (`RTHexMapAsset.cpp:533`), quindi
 * lasciarne una indietro non e' un dettaglio estetico: produce un errore di validazione che l'ottavo AC di
 * #1864 vieta.
 *
 * ⚠️ **Le due direzioni sono entrambe nel test perche' `FRTHexEdge` e' direzionale**: un'implementazione
 * che guardasse solo `From` passerebbe con un test che cita la cella una volta sola, e lascerebbe orfana
 * meta' dei casi reali.
 *
 * ⚠️ Le celle sono a **distanza 2**, non adiacenti: fra celle adiacenti dello stesso layer `ValidateMap`
 * segnala la transizione come ridondante, e la controprova «l'allestimento e' sano» fallirebbe per una
 * ragione che non c'entra con le dipendenze.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapDependencyCellTakesTransitionsTest,
	"RefactorTactics.Map.Dependency.CellTakesTransitionsCitingIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapDependencyCellTakesTransitionsTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MapDepMakeMap(2);

	const FRTCellId Doomed(0, 0, 0);
	const FRTCellId Far(2, 0, 0);
	const FRTCellId Other(0, 2, 0);
	const FRTCellId Another(2, -2, 0);

	// Citata come sorgente, citata come bersaglio, e una che non la nomina affatto.
	Map->Transitions.Add(FRTHexEdge(Doomed, Far, /*Cost*/ 1));
	Map->Transitions.Add(FRTHexEdge(Other, Doomed, /*Cost*/ 1));
	Map->Transitions.Add(FRTHexEdge(Other, Another, /*Cost*/ 1));

	if (!TestEqual(TEXT("l'allestimento non produce errori di validazione"),
		Map->ValidateMap().Num(), 0))
	{
		return false;
	}

	const FRTMapDependencySet Set =
		URTMapDependencyLibrary::CollectDependents(Map, FRTMapElementHandle::ForCell(Doomed));

	if (!TestEqual(TEXT("due transizioni seguono la cella cancellata"),
		Set.TransitionIndices.Num(), 2))
	{
		return false;
	}

	// L'estranea resta in piedi: senza questa asserzione passerebbe anche chi restituisce tutto.
	TestFalse(TEXT("la transizione che non la cita non e' fra i dipendenti"),
		Set.TransitionIndices.Contains(2));

	for (const int32 Index : Set.TransitionIndices)
	{
		const FRTHexEdge& Edge = Map->Transitions[Index];
		TestTrue(TEXT("ogni transizione raccolta cita davvero la cella cancellata"),
			Edge.From == Doomed || Edge.To == Doomed);
	}

	return true;
}

/**
 * 🔴 Cancellare una cella porta via i BINDING DI INTERAZIONE che nominavano le sue porte.
 *
 * E' il difetto che il panel su #1864 ha registrato come `C2`: il quinto AC elencava muri, coperture e
 * transizioni, e l'ottavo vietava di lasciare orfani che `ValidateMap` segnalerebbe — ma un binding la cui
 * sorgente sparisce e' esattamente uno di quegli orfani, e nessuno dei due criteri lo nominava.
 *
 * La catena che lo prova, e che questo test lega insieme:
 * `ValidateMap` -> `ValidateInteractionGraph` -> `ValidateReferences` -> «riferimento a una struttura
 * inesistente».
 *
 * ⚠️ Il binding estraneo esiste per non far passare chi restituisce l'intero array.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapDependencyCellTakesBindingsTest,
	"RefactorTactics.Map.Dependency.CellTakesBindingsNamingItsDoors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapDependencyCellTakesBindingsTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MapDepMakeMap(2);

	const FRTCellId Doomed(0, 0, 0);
	const FRTCellId Spared(2, 0, 0);
	const FRTCellId Elsewhere(0, 2, 0);

	// La sorgente sta sulla cella condannata; il bersaglio no.
	MapDepPutDoor(Map, Doomed, ERTHexDirection::E, /*DoorId*/ 1, TEXT("S1"));
	MapDepPutDoor(Map, Spared, ERTHexDirection::W, /*DoorId*/ 2, TEXT("D1"));
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D1") }));

	// Un binding che non tocca la cella condannata: deve sopravvivere.
	MapDepPutDoor(Map, Elsewhere, ERTHexDirection::E, /*DoorId*/ 3, TEXT("S2"));
	MapDepPutDoor(Map, Spared, ERTHexDirection::NE, /*DoorId*/ 4, TEXT("D2"));
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S2"), { TEXT("D2") }));

	if (!TestEqual(TEXT("l'allestimento non produce errori di validazione"),
		Map->ValidateMap().Num(), 0))
	{
		return false;
	}

	const FRTMapDependencySet Set =
		URTMapDependencyLibrary::CollectDependents(Map, FRTMapElementHandle::ForCell(Doomed));

	if (!TestEqual(TEXT("un solo binding segue la cella cancellata"),
		Set.InteractionBindingIndices.Num(), 1))
	{
		return false;
	}

	TestEqual(TEXT("ed e' quello la cui sorgente stava sulla cella"),
		Map->InteractionBindings[Set.InteractionBindingIndices[0]].SourceId, FName(TEXT("S1")));

	return true;
}

/**
 * Un binding NON muore se il portone che lo comanda sopravvive su un'altra cella.
 *
 * ⚠️ E' la differenza fra «la cella portava un bordo di quella struttura» e «quella struttura non esiste
 * piu'». Un portone e' un GRUPPO di bordi con lo stesso nome — `Structures.Identity.ResolvesDoorGroupByStableId`
 * lo pinna — e cancellare una delle due celle ne toglie meta', non tutto. Il nome continua a risolvere.
 *
 * 🔴 Senza questo caso, la regola rimuoverebbe un binding **ancora valido**: sarebbe la «correzione
 * silenziosa di uno stato autorato» che i Non-goal di #1864 vietano, e per giunta con perdita di dato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapDependencyBindingSurvivesGroupTest,
	"RefactorTactics.Map.Dependency.BindingSurvivesWhenDoorGroupOutlivesTheCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapDependencyBindingSurvivesGroupTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MapDepMakeMap(2);

	const FRTCellId Doomed(0, 0, 0);
	const FRTCellId Survivor(0, 1, 0);
	const FRTCellId Target(2, 0, 0);

	// UN portone, DUE bordi su due celle diverse: stesso `DoorId`, stesso nome pubblico.
	MapDepPutDoor(Map, Doomed, ERTHexDirection::E, /*DoorId*/ 7, TEXT("S1"));
	MapDepPutDoor(Map, Survivor, ERTHexDirection::E, /*DoorId*/ 7, TEXT("S1"));
	MapDepPutDoor(Map, Target, ERTHexDirection::W, /*DoorId*/ 2, TEXT("D1"));
	Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D1") }));

	// Controprova sull'allestimento: il portone e' davvero un gruppo di due bordi, non due strutture.
	if (!TestEqual(TEXT("il nome risolve i due bordi del portone"),
		URTStructureIdentityLibrary::FindDoorEdges(Map, TEXT("S1")).Num(), 2))
	{
		return false;
	}
	if (!TestEqual(TEXT("l'allestimento non produce errori di validazione"),
		Map->ValidateMap().Num(), 0))
	{
		return false;
	}

	const FRTMapDependencySet Set =
		URTMapDependencyLibrary::CollectDependents(Map, FRTMapElementHandle::ForCell(Doomed));

	TestEqual(TEXT("il binding sopravvive: il portone che comanda esiste ancora"),
		Set.InteractionBindingIndices.Num(), 0);

	// E l'altra meta' del contratto: il bordo sulla cella condannata se ne va comunque con lei, perche'
	// vive DENTRO `FRTHexCellData`. Il gruppo si dimezza, il nome resta.
	TestEqual(TEXT("dopo la rimozione il nome risolverebbe un bordo solo"),
		URTStructureIdentityLibrary::FindDoorEdges(Map, TEXT("S1")).Num() - 1, 1);

	return true;
}

/**
 * L'ottavo AC di #1864, per intero: applicare la cascata NON lascia niente che `ValidateMap` segnalerebbe.
 *
 * I tre test sopra provano ciascuno il proprio array. Questo li lega, ed e' l'unico che risponde alla
 * domanda che il criterio pone davvero — *«dopo, la mappa e' valida?»* — invece che alla domanda piu'
 * comoda *«ho raccolto quello che mi aspettavo?»*.
 *
 * 🔴 **La controprova e' la meta' che conta.** Rimuovere la sola cella DEVE sporcare la validazione: senza
 * quella misura, un test che rimuove tutto e trova zero errori passerebbe anche con una regola che
 * raccoglie a caso, o con una mappa troppo povera per avere dipendenti. Prima si prova che il difetto
 * esiste, poi che la regola lo previene.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapDependencyCascadeLeavesMapValidTest,
	"RefactorTactics.Map.Dependency.CascadeLeavesTheMapValid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapDependencyCascadeLeavesMapValidTest::RunTest(const FString&)
{
	const FRTCellId Doomed(0, 0, 0);
	const FRTCellId Far(2, 0, 0);

	FRTGeometrySegment Chord;
	if (!TestTrue(TEXT("la corda si aggancia alla grammatica"), MapDepMakeChord(Chord)))
	{
		return false;
	}

	// L'allestimento sta in una lambda perche' serve DUE volte: una per la controprova, una per la misura.
	// Ricostruirlo invece di riusarne uno mezzo smontato e' cio' che tiene le due misure indipendenti.
	auto MakeLoadedMap = [&Chord, &Doomed, &Far]()
	{
		URTHexMapAsset* M = MapDepMakeMap(2);
		M->InteriorWalls.Add(FRTHexInteriorWall(Doomed, Chord));
		M->Transitions.Add(FRTHexEdge(Doomed, Far, /*Cost*/ 1));
		MapDepPutDoor(M, Doomed, ERTHexDirection::E, /*DoorId*/ 1, TEXT("S1"));
		MapDepPutDoor(M, Far, ERTHexDirection::W, /*DoorId*/ 2, TEXT("D1"));
		M->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D1") }));
		return M;
	};

	// --- Controprova: la sola cella, e la mappa si sporca -------------------------------------------
	{
		URTHexMapAsset* Naive = MakeLoadedMap();
		if (!TestEqual(TEXT("l'allestimento parte valido"), Naive->ValidateMap().Num(), 0))
		{
			return false;
		}

		Naive->RemoveCell(Doomed);

		if (!TestTrue(TEXT("rimuovere la SOLA cella lascia errori di validazione"),
			Naive->ValidateMap().Num() > 0))
		{
			return false;
		}
	}

	// --- La misura: cella piu' dipendenti, e la mappa resta pulita ----------------------------------
	URTHexMapAsset* Map = MakeLoadedMap();

	const FRTMapDependencySet Set =
		URTMapDependencyLibrary::CollectDependents(Map, FRTMapElementHandle::ForCell(Doomed));

	// Dall'indice piu' alto al piu' basso: rimuovere dal basso invaliderebbe gli indici successivi, ed e'
	// il contratto che `FRTMapDependencySet` dichiara.
	TArray<int32> Walls = Set.InteriorWallIndices;
	TArray<int32> Edges = Set.TransitionIndices;
	TArray<int32> Bindings = Set.InteractionBindingIndices;
	Walls.Sort([](int32 A, int32 B) { return A > B; });
	Edges.Sort([](int32 A, int32 B) { return A > B; });
	Bindings.Sort([](int32 A, int32 B) { return A > B; });

	for (const int32 Index : Walls)    { Map->InteriorWalls.RemoveAt(Index); }
	for (const int32 Index : Edges)    { Map->Transitions.RemoveAt(Index); }
	for (const int32 Index : Bindings) { Map->InteractionBindings.RemoveAt(Index); }
	Map->RemoveCell(Doomed);

	const TArray<FString> Errors = Map->ValidateMap();

	// L'elenco degli errori residui entra nel messaggio: un `0 != 2` non dice QUALE dipendenza manca, e la
	// prossima persona ripeterebbe l'indagine da capo.
	TestEqual(*FString::Printf(TEXT("la cascata non lascia errori (residui: %s)"),
		Errors.Num() > 0 ? *FString::Join(Errors, TEXT(" | ")) : TEXT("nessuno")),
		Errors.Num(), 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
