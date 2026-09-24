#include "Misc/AutomationTest.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Map/RTHexMapCustomVersion.h"
#include "Map/RTCellId.h"
#include "Map/RTGeometryBake.h"
#include "Map/RTGeometryGrammar.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexDoorLibrary.h" // DoorBetween: la simmetria delle due facce si CHIEDE al gioco
#include "Map/RTHexMapAsset.h"
#include "Map/RTMapDependencyLibrary.h"
#include "Map/RTMapEditLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Il MOVE di un elemento autorato (#1864), e l'identita' che lo rende possibile.
 *
 * 🔑 **Questo file esiste per una ragione sola, ed e' la ragione per cui `FRTHexInteriorWall` guadagna uno
 * `StableId`**: la chiave naturale di un muro interno e' `(Cell, Segment)`, e spostarlo cambia il
 * `Segment` — cioe' la chiave. Un handle derivato si romperebbe **esattamente durante l'operazione che
 * deve sopravvivergli**, ed e' il motivo per cui l'opzione «nessun campo nuovo» e' stata scartata.
 *
 * La copertura non prende un campo: la sua chiave e' `(Cell, Edge)`, e l'unicita' per bordo non e'
 * un'assunzione — `ValidateMap` la applica gia'.
 */
namespace
{
	constexpr float MapEditHexSize = 100.f;

	/** Nome prefissato per dominio: unity build. */
	URTHexMapAsset* MapEditMakeMap(int32 Radius)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		M->HexSize = MapEditHexSize;
		return M;
	}

	/**
	 * Una corda per due vertici opposti, ruotata di `Sector` sesti di giro.
	 *
	 * Serve **due** posizioni legali distinte: una di partenza e una d'arrivo. Entrambe attraversano la
	 * cella per il centro senza chiudere bordi, quindi entrambe sono muri interni legittimi — e il move
	 * fra le due e' un gesto che l'autore puo' davvero fare.
	 */
	bool MapEditMakeChord(int32 Sector, FRTGeometrySegment& Out)
	{
		const double Angle = (PI / 6.0) + (Sector * PI / 3.0);
		const FVector2D A(MapEditHexSize * FMath::Cos(Angle), MapEditHexSize * FMath::Sin(Angle));
		const FVector2D B(-A.X, -A.Y);

		if (!URTGeometryGrammarLibrary::SnapToGrammar(A, B, MapEditHexSize, Out))
		{
			return false;
		}
		Out.WallType = ERTHexCoverType::High;
		return true;
	}

	/** Porta con nome pubblico sul bordo indicato. */
	void MapEditPutDoor(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge,
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
 * 🔑 L'handle di un muro interno risolve allo STESSO muro dopo che il muro si e' mosso.
 *
 * E' l'`Automation Test` che #1864 chiede con le parole *«salva, ricarica, l'handle risolve allo stesso
 * elemento»*, nella sua forma piu' stretta: non basta che sopravviva a un round-trip in cui nulla cambia,
 * deve sopravvivere alla **modifica dell'elemento stesso**.
 *
 * ⚠️ La controprova sta dentro il test e non e' decorativa: si verifica che il segmento sia **davvero
 * cambiato**. Senza, un move che non muove nulla farebbe passare l'asserzione sull'identita' senza aver
 * dimostrato niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapEditHandleSurvivesMoveTest,
	"RefactorTactics.Map.Edit.InteriorWallHandleSurvivesTheMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapEditHandleSurvivesMoveTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MapEditMakeMap(2);

	FRTGeometrySegment From;
	FRTGeometrySegment To;
	if (!TestTrue(TEXT("le due corde si agganciano alla grammatica"),
		MapEditMakeChord(0, From) && MapEditMakeChord(1, To)))
	{
		return false;
	}
	if (!TestTrue(TEXT("le due corde sono posizioni DIVERSE"), !(From == To)))
	{
		return false;
	}

	const FRTCellId Home(0, 0, 0);

	FRTHexInteriorWall Wall(Home, From);
	Wall.StableId = TEXT("W1");
	Map->InteriorWalls.Add(Wall);

	// Una seconda parete, anonima, sulla stessa cella: il muro nominato non deve essere l'unico candidato
	// che la risoluzione possa trovare per esclusione.
	FRTGeometrySegment Third;
	if (!TestTrue(TEXT("la terza corda si aggancia"), MapEditMakeChord(2, Third)))
	{
		return false;
	}
	Map->InteriorWalls.Add(FRTHexInteriorWall(Home, Third));

	if (!TestEqual(TEXT("l'allestimento non produce errori di validazione"),
		Map->ValidateMap().Num(), 0))
	{
		return false;
	}

	const FRTMapElementHandle Handle = FRTMapElementHandle::ForInteriorWall(TEXT("W1"));

	const int32 Before = URTMapEditLibrary::ResolveInteriorWall(Map, Handle);
	if (!TestTrue(TEXT("l'handle risolve prima del move"), Before != INDEX_NONE))
	{
		return false;
	}

	const ERTMapEditOutcome Outcome = URTMapEditLibrary::MoveInteriorWall(Map, Handle, Home, To);
	if (!TestEqual(TEXT("il move e' applicato"), Outcome, ERTMapEditOutcome::Applied))
	{
		return false;
	}

	const int32 After = URTMapEditLibrary::ResolveInteriorWall(Map, Handle);
	if (!TestTrue(TEXT("l'handle risolve ANCORA dopo il move"), After != INDEX_NONE))
	{
		return false;
	}

	// La controprova: il muro si e' mosso davvero. Un move a vuoto renderebbe l'asserzione sopra gratuita.
	TestTrue(TEXT("il segmento e' cambiato"), Map->InteriorWalls[After].Segment == To);
	TestEqual(TEXT("e il nome e' rimasto suo"),
		Map->InteriorWalls[After].StableId, FName(TEXT("W1")));

	// L'anonimo non e' stato toccato: il move ha un bersaglio, non un raggio d'azione.
	TestEqual(TEXT("il muro anonimo e' rimasto dov'era"), Map->InteriorWalls.Num(), 2);

	return true;
}

/**
 * 🔴 Il move RIFIUTA un segmento che chiuderebbe un bordo, e non lo accomoda.
 *
 * `InteriorWalls` porta per invariante solo cio' che **nessuna copertura puo' rappresentare**
 * (`RTHexMapAsset.h`): un segmento che chiude un bordo e' gia' descritto dalla sua copertura, e scriverlo
 * anche qui creerebbe due verita' sullo stesso muro. `ValidateMap` lo segnala come quarta regola dei muri
 * interni.
 *
 * ⛔ **Il punto e' il rifiuto, non la segnalazione a cose fatte.** I Non-goal di #1864 vietano di correggere
 * in silenzio uno stato autorato invalido; scriverlo e poi lasciare che il validator protesti sarebbe
 * proprio quello — con l'aggravante che l'autore ha gia' perso la posizione precedente.
 *
 * ⚠️ Le due asserzioni sono complementari e nessuna basta da sola: l'esito dice **quale** regola ha fermato
 * il gesto, e il muro immutato dice che il rifiuto e' avvenuto **prima** della scrittura.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapEditRefusesEdgeClosingTest,
	"RefactorTactics.Map.Edit.MoveRefusesASegmentThatWouldCloseAnEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapEditRefusesEdgeClosingTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MapEditMakeMap(2);
	const FRTCellId Home(0, 0, 0);

	FRTGeometrySegment Chord;
	if (!TestTrue(TEXT("la corda si aggancia"), MapEditMakeChord(0, Chord)))
	{
		return false;
	}

	FRTHexInteriorWall Wall(Home, Chord);
	Wall.StableId = TEXT("W1");
	Map->InteriorWalls.Add(Wall);

	// Un segmento che GIACE su un bordo: lo stesso costrutto di `Geometry.InteriorWallHygiene` caso (d).
	const double Deg30 = PI / 6.0;
	const FVector Here = URTHexLibrary::AxialToWorld(Home, FVector::ZeroVector, MapEditHexSize, 0.f);
	const FVector There = URTHexLibrary::AxialToWorld(
		URTHexLibrary::Neighbor(Home, ERTHexDirection::NE), FVector::ZeroVector, MapEditHexSize, 0.f);
	const double Mid = FMath::Atan2(There.Y - Here.Y, There.X - Here.X);

	FRTGeometrySegment OnEdge;
	if (!TestTrue(TEXT("il segmento di bordo si aggancia"),
		URTGeometryGrammarLibrary::SnapToGrammar(
			FVector2D(MapEditHexSize * FMath::Cos(Mid - Deg30), MapEditHexSize * FMath::Sin(Mid - Deg30)),
			FVector2D(MapEditHexSize * FMath::Cos(Mid + Deg30), MapEditHexSize * FMath::Sin(Mid + Deg30)),
			MapEditHexSize, OnEdge)))
	{
		return false;
	}

	// Controprova: quel segmento chiude DAVVERO un bordo. Senza, il test proverebbe il rifiuto di un
	// segmento innocuo e passerebbe per la ragione sbagliata.
	TArray<ERTHexDirection> Touched;
	URTGeometryBakeLibrary::EdgesTouchedBy(OnEdge, MapEditHexSize, Touched);
	if (!TestTrue(TEXT("il segmento di prova chiude almeno un bordo"), Touched.Num() > 0))
	{
		return false;
	}

	const FRTMapElementHandle Handle = FRTMapElementHandle::ForInteriorWall(TEXT("W1"));
	const ERTMapEditOutcome Outcome = URTMapEditLibrary::MoveInteriorWall(Map, Handle, Home, OnEdge);

	TestEqual(TEXT("il move e' rifiutato, e dice quale regola"),
		Outcome, ERTMapEditOutcome::RefusedWouldCloseEdge);

	// E il rifiuto e' avvenuto PRIMA della scrittura: il muro non si e' mosso di un quanto.
	const int32 Index = URTMapEditLibrary::ResolveInteriorWall(Map, Handle);
	if (TestTrue(TEXT("il muro esiste ancora"), Index != INDEX_NONE))
	{
		TestTrue(TEXT("ed e' rimasto dov'era"), Map->InteriorWalls[Index].Segment == Chord);
	}

	// La mappa resta valida: un rifiuto non lascia mai uno stato peggiore di quello che ha trovato.
	TestEqual(TEXT("la mappa e' ancora valida"), Map->ValidateMap().Num(), 0);

	return true;
}

/**
 * Il move rifiuta ogni stato che `ValidateMap` segnalerebbe, e dice quale regola per ciascuno.
 *
 * Un comportamento solo — *«non produrre un invalido»* — nelle tre forme che restano dopo il bordo chiuso.
 * Ognuna corrisponde a una regola gia' scritta per i muri interni (`RTHexMapAsset.cpp`), e il punto e' che
 * il gesto si ferma **prima**: un validator che protesta dopo ha gia' lasciato l'autore senza la posizione
 * di partenza.
 *
 * ⚠️ Ogni caso riparte da una mappa pulita. Concatenarli su un'unica mappa farebbe dipendere il terzo
 * esito dall'esattezza dei primi due, e un rifiuto sbagliato si leggerebbe come tre.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapEditRefusesInvalidStatesTest,
	"RefactorTactics.Map.Edit.MoveRefusesEveryStateValidateMapWouldFlag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapEditRefusesInvalidStatesTest::RunTest(const FString&)
{
	const FRTCellId Home(0, 0, 0);
	const FRTMapElementHandle Handle = FRTMapElementHandle::ForInteriorWall(TEXT("W1"));

	FRTGeometrySegment Chord;
	FRTGeometrySegment Other;
	if (!TestTrue(TEXT("le corde si agganciano"),
		MapEditMakeChord(0, Chord) && MapEditMakeChord(1, Other)))
	{
		return false;
	}

	auto MakeMapWithNamedWall = [&Chord, &Home]()
	{
		URTHexMapAsset* M = MapEditMakeMap(2);
		FRTHexInteriorWall Wall(Home, Chord);
		Wall.StableId = TEXT("W1");
		M->InteriorWalls.Add(Wall);
		return M;
	};

	// --- 1. Fuori grammatica: `ValidateSegment` decide, il move la chiama invece di indovinare ------
	{
		URTHexMapAsset* Map = MakeMapWithNamedWall();

		FRTGeometrySegment Broken = Other;
		Broken.AlongEnd = Broken.AlongStart; // lunghezza zero

		// Controprova: il segmento di prova e' davvero illegale.
		if (!TestTrue(TEXT("il segmento di prova e' fuori grammatica"),
			URTGeometryGrammarLibrary::ValidateSegment(Broken) != ERTGeometryViolation::None))
		{
			return false;
		}

		TestEqual(TEXT("fuori grammatica: rifiutato"),
			URTMapEditLibrary::MoveInteriorWall(Map, Handle, Home, Broken),
			ERTMapEditOutcome::RefusedOutOfGrammar);
		TestEqual(TEXT("e la mappa resta valida"), Map->ValidateMap().Num(), 0);
	}

	// --- 2. Cella inesistente: ci finirebbe un orfano ------------------------------------------------
	{
		URTHexMapAsset* Map = MakeMapWithNamedWall();
		const FRTCellId Nowhere(9, 9, 0);

		// Controprova: quella cella non esiste davvero.
		if (!TestTrue(TEXT("la cella di destinazione non esiste"), Map->FindCell(Nowhere) == nullptr))
		{
			return false;
		}

		TestEqual(TEXT("cella inesistente: rifiutato"),
			URTMapEditLibrary::MoveInteriorWall(Map, Handle, Nowhere, Other),
			ERTMapEditOutcome::RefusedNoSuchCell);
		TestEqual(TEXT("e la mappa resta valida"), Map->ValidateMap().Num(), 0);
	}

	// --- 3. Duplicato: due volte lo stesso muro sulla stessa cella -----------------------------------
	{
		URTHexMapAsset* Map = MakeMapWithNamedWall();
		Map->InteriorWalls.Add(FRTHexInteriorWall(Home, Other));

		if (!TestEqual(TEXT("l'allestimento a due muri e' valido"), Map->ValidateMap().Num(), 0))
		{
			return false;
		}

		// W1 prova a sovrapporsi all'altro.
		TestEqual(TEXT("duplicato: rifiutato"),
			URTMapEditLibrary::MoveInteriorWall(Map, Handle, Home, Other),
			ERTMapEditOutcome::RefusedDuplicate);
		TestEqual(TEXT("e la mappa resta valida"), Map->ValidateMap().Num(), 0);
	}

	return true;
}

/**
 * Il ROUND-TRIP del campo nuovo (formato v12), che il Gate dell'Epic pretende per ogni campo serializzato.
 *
 * ⚠️ **E' anche l'`Automation Test` che #1864 chiede alla lettera** — *«salva, ricarica, l'handle risolve
 * allo stesso elemento»* — mentre `InteriorWallHandleSurvivesTheMove` ne prova la forma piu' stretta.
 * Servono entrambe: un nome che sopravvive al move ma non al salvataggio non e' un'identita' stabile, e
 * viceversa.
 *
 * ⚠️ **Il muro anonimo e' la meta' che verifica la MIGRAZIONE.** `NAME_None` e' cio' che ogni muro scritto
 * prima di v12 diventa rileggendosi, ed e' esattamente cio' che quei muri gia' erano: se la rilettura gli
 * inventasse un nome, un asset vecchio cambierebbe significato — che e' cio' che un passo dichiarativo
 * promette di non fare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapEditWallIdentityRoundTripTest,
	"RefactorTactics.Map.Edit.InteriorWallIdentitySurvivesSerialization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapEditWallIdentityRoundTripTest::RunTest(const FString&)
{
	const FRTCellId Home(0, 0, 0);

	FRTGeometrySegment Named;
	FRTGeometrySegment Anonymous;
	if (!TestTrue(TEXT("le corde si agganciano"),
		MapEditMakeChord(0, Named) && MapEditMakeChord(1, Anonymous)))
	{
		return false;
	}

	URTHexMapAsset* Source = MapEditMakeMap(2);
	{
		FRTHexInteriorWall Wall(Home, Named);
		Wall.StableId = TEXT("W1");
		Source->InteriorWalls.Add(Wall);
	}
	Source->InteriorWalls.Add(FRTHexInteriorWall(Home, Anonymous));

	// --- Scrittura ---------------------------------------------------------------------------------
	TArray<uint8> Bytes;
	FCustomVersionContainer WrittenVersions;
	{
		FMemoryWriter Writer(Bytes);
		Source->Serialize(Writer);
		WrittenVersions = Writer.GetCustomVersions();
	}

	// --- Rilettura ---------------------------------------------------------------------------------
	URTHexMapAsset* Reloaded = NewObject<URTHexMapAsset>();
	{
		FMemoryReader Reader(Bytes);
		Reader.SetCustomVersions(WrittenVersions);
		Reloaded->Serialize(Reader);
	}

	if (!TestEqual(TEXT("i due muri sono tornati"), Reloaded->InteriorWalls.Num(), 2))
	{
		return false;
	}

	// L'handle risolve nella mappa RICARICATA, che e' il punto di tutto il campo.
	const FRTMapElementHandle Handle = FRTMapElementHandle::ForInteriorWall(TEXT("W1"));
	const int32 Index = URTMapEditLibrary::ResolveInteriorWall(Reloaded, Handle);

	if (!TestTrue(TEXT("l'handle risolve dopo la ricarica"), Index != INDEX_NONE))
	{
		return false;
	}
	TestTrue(TEXT("e nomina il muro giusto, non solo un muro"),
		Reloaded->InteriorWalls[Index].Segment == Named);

	// L'anonimo resta anonimo: e' cio' che un muro scritto prima di v12 gia' era.
	const int32 Other = (Index == 0) ? 1 : 0;
	TestTrue(TEXT("il muro senza nome non ne guadagna uno"),
		Reloaded->InteriorWalls[Other].StableId.IsNone());

	// E un handle su `NAME_None` non risolve, per la stessa ragione: molti muri lo soddisfano.
	TestEqual(TEXT("un handle anonimo non risolve niente"),
		URTMapEditLibrary::ResolveInteriorWall(Reloaded, FRTMapElementHandle::ForInteriorWall(NAME_None)),
		INDEX_NONE);

	return true;
}

/**
 * 🔑 `DeleteElement` APPLICA la cascata, e la mappa resta valida.
 *
 * `URTMapDependencyLibrary::CollectDependents` dice *che cosa* muore; questa dice *fallo*. Finora quel
 * passaggio esisteva solo dentro un test, che rimuoveva gli indici a mano — e un tool non puo' rifare a
 * mano cio' che un test ha gia' dovuto scrivere: e' il modo in cui due implementazioni della stessa regola
 * finiscono per divergere.
 *
 * ⚠️ **La controprova e' che rimuovere la sola cella NON basta.** Senza, questo test passerebbe anche con
 * una `DeleteElement` che ignora i dipendenti su una mappa troppo povera per averne.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapEditDeleteCellCascadeTest,
	"RefactorTactics.Map.Edit.DeleteElementAppliesTheWholeCascade",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapEditDeleteCellCascadeTest::RunTest(const FString&)
{
	const FRTCellId Doomed(0, 0, 0);
	const FRTCellId Far(2, 0, 0);

	FRTGeometrySegment Chord;
	if (!TestTrue(TEXT("la corda si aggancia"), MapEditMakeChord(0, Chord)))
	{
		return false;
	}

	auto MakeLoadedMap = [&Chord, &Doomed, &Far]()
	{
		URTHexMapAsset* M = MapEditMakeMap(2);
		M->InteriorWalls.Add(FRTHexInteriorWall(Doomed, Chord));
		M->Transitions.Add(FRTHexEdge(Doomed, Far, /*Cost*/ 1));
		MapEditPutDoor(M, Doomed, ERTHexDirection::E, /*DoorId*/ 1, TEXT("S1"));
		MapEditPutDoor(M, Far, ERTHexDirection::W, /*DoorId*/ 2, TEXT("D1"));
		M->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D1") }));
		return M;
	};

	// --- Controprova: la sola cella non basta, e la mappa si sporca ---------------------------------
	{
		URTHexMapAsset* Naive = MakeLoadedMap();
		if (!TestEqual(TEXT("l'allestimento parte valido"), Naive->ValidateMap().Num(), 0))
		{
			return false;
		}
		Naive->RemoveCell(Doomed);
		if (!TestTrue(TEXT("rimuovere la SOLA cella lascia errori"), Naive->ValidateMap().Num() > 0))
		{
			return false;
		}
	}

	// --- La misura -----------------------------------------------------------------------------------
	URTHexMapAsset* Map = MakeLoadedMap();

	const ERTMapEditOutcome Outcome =
		URTMapEditLibrary::DeleteElement(Map, FRTMapElementHandle::ForCell(Doomed));

	if (!TestEqual(TEXT("la cancellazione e' applicata"), Outcome, ERTMapEditOutcome::Applied))
	{
		return false;
	}

	TestTrue(TEXT("la cella non c'e' piu'"), Map->FindCell(Doomed) == nullptr);
	TestEqual(TEXT("il muro interno e' andato con lei"), Map->InteriorWalls.Num(), 0);
	TestEqual(TEXT("e cosi' la transizione"), Map->Transitions.Num(), 0);
	TestEqual(TEXT("e il binding che la nominava"), Map->InteractionBindings.Num(), 0);

	const TArray<FString> Errors = Map->ValidateMap();
	TestEqual(*FString::Printf(TEXT("la mappa resta valida (residui: %s)"),
		Errors.Num() > 0 ? *FString::Join(Errors, TEXT(" | ")) : TEXT("nessuno")),
		Errors.Num(), 0);

	// Un handle che non risolve non e' un'operazione riuscita a vuoto: e' un rifiuto dichiarato.
	TestEqual(TEXT("cancellare una cella inesistente e' un rifiuto"),
		URTMapEditLibrary::DeleteElement(Map, FRTMapElementHandle::ForCell(FRTCellId(9, 9, 0))),
		ERTMapEditOutcome::RefusedUnresolved);

	return true;
}

/**
 * 🔑 I candidati sotto un punto, in ordine **deterministico**, dal piu' specifico al piu' generale.
 *
 * E' il dominio su cui poggia il ciclo di selezione: `ValidateMap` **permette** una porta e una copertura
 * `Low` sullo stesso bordo (vieta solo la coppia con `High`), quindi due elementi selezionabili nello stesso
 * punto sono uno stato che l'autoraggio produce e il validatore approva — non un caso limite da scoprire in
 * seduta.
 *
 * ⚠️ **L'ordine e' parte del contratto, non un dettaglio d'implementazione.** Chi clicca due volte si aspetta
 * la stessa sequenza, e una funzione che restituisse i candidati nell'ordine di un array interno cambierebbe
 * risposta dopo un'edit che non c'entra nulla.
 *
 * ⛔ **Le transizioni non sono qui, ed e' dichiarato invece che dimenticato**: un arco collega due celle su
 * layer diversi e non giace su un bordo, quindi «che cosa c'e' sotto questo bordo» non lo raggiunge. Il suo
 * hit-test e' un problema di viewport e appartiene al tool.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapEditEnumerateAtTest,
	"RefactorTactics.Map.Edit.ElementsAtAPointComeInAStableOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapEditEnumerateAtTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MapEditMakeMap(2);
	const FRTCellId Home(0, 0, 0);
	const ERTHexDirection Edge = ERTHexDirection::E;

	// Una porta e una copertura LOW sullo stesso bordo: `ValidateMap` lo permette.
	MapEditPutDoor(Map, Home, Edge, /*DoorId*/ 1, TEXT("D1"));
	{
		FRTHexCellData Data = *Map->FindCell(Home);
		FRTHexCover Cover;
		Cover.Edge = Edge;
		Cover.Type = ERTHexCoverType::Low;
		Data.Covers.Add(Cover);
		Map->AddOrUpdateCell(Data);
	}

	FRTGeometrySegment Chord;
	if (!TestTrue(TEXT("la corda si aggancia"), MapEditMakeChord(0, Chord)))
	{
		return false;
	}
	FRTHexInteriorWall Wall(Home, Chord);
	Wall.StableId = TEXT("W1");
	Map->InteriorWalls.Add(Wall);

	// 🔴 Controprova: l'allestimento e' uno stato LEGALE. Se `ValidateMap` lo rifiutasse, questo test
	// starebbe difendendo un caso che l'autoraggio non puo' produrre.
	if (!TestEqual(TEXT("porta e copertura Low sullo stesso bordo sono uno stato valido"),
		Map->ValidateMap().Num(), 0))
	{
		return false;
	}

	const TArray<FRTMapElementHandle> At =
		URTMapEditLibrary::ElementsAt(Map, Home, Edge);

	if (!TestEqual(TEXT("quattro candidati sotto quel punto"), At.Num(), 4))
	{
		return false;
	}

	TestEqual(TEXT("1. la porta, che e' la piu' specifica"), At[0].Kind, ERTMapElementKind::Door);
	TestEqual(TEXT("   e la nomina"), At[0].StableId, FName(TEXT("D1")));
	TestEqual(TEXT("2. la copertura sullo stesso bordo"), At[1].Kind, ERTMapElementKind::Cover);
	TestEqual(TEXT("3. il muro interno della cella"), At[2].Kind, ERTMapElementKind::InteriorWall);
	TestEqual(TEXT("   e lo nomina"), At[2].StableId, FName(TEXT("W1")));
	TestEqual(TEXT("4. la cella, che e' la piu' generale"), At[3].Kind, ERTMapElementKind::Cell);

	// La stessa domanda due volte da' la stessa risposta: e' cio' che rende ripetibile il ciclo di click.
	const TArray<FRTMapElementHandle> Again = URTMapEditLibrary::ElementsAt(Map, Home, Edge);
	if (TestEqual(TEXT("la seconda chiamata da' lo stesso numero"), Again.Num(), At.Num()))
	{
		for (int32 I = 0; I < At.Num(); ++I)
		{
			TestEqual(TEXT("e lo stesso ordine"), Again[I].Kind, At[I].Kind);
		}
	}

	// Un bordo spoglio della stessa cella: resta la sola cella. Senza questo caso passerebbe anche una
	// funzione che ignora il bordo e restituisce sempre tutto.
	const TArray<FRTMapElementHandle> Bare = URTMapEditLibrary::ElementsAt(Map, Home, ERTHexDirection::W);
	if (TestEqual(TEXT("su un bordo spoglio restano muro e cella"), Bare.Num(), 2))
	{
		TestEqual(TEXT("il muro interno c'e' comunque: vive nella cella, non sul bordo"),
			Bare[0].Kind, ERTMapElementKind::InteriorWall);
		TestEqual(TEXT("e poi la cella"), Bare[1].Kind, ERTMapElementKind::Cell);
	}

	// Una cella che non esiste non ha candidati.
	TestEqual(TEXT("una cella inesistente non produce candidati"),
		URTMapEditLibrary::ElementsAt(Map, FRTCellId(9, 9, 0), Edge).Num(), 0);

	return true;
}

/**
 * 🔴 Un muro interno SENZA nome deve restare selezionabile — ed e' il caso normale, non il caso limite.
 *
 * `StableId` nasce `NAME_None` (v12): **ogni muro disegnato prima di oggi e' anonimo**, e lo sara' ogni muro
 * che un tool crea senza battezzarlo. Se l'handle sapesse identificare solo i muri nominati, il ciclo di
 * selezione emetterebbe candidati che non risolvono — cioe' il tool mostrerebbe «muro interno» e poi non
 * riuscirebbe a farci nulla.
 *
 * 🔑 La chiave di riserva non e' inventata qui: `(Cell, Segment)` e' **unica per una regola che `ValidateMap`
 * gia' applica** — due muri identici sulla stessa cella sono un errore dichiarato. Il nome, quando c'e',
 * vince comunque, perche' e' l'unico che sopravvive al move.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapEditAnonymousWallTest,
	"RefactorTactics.Map.Edit.AnAnonymousInteriorWallIsStillSelectable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapEditAnonymousWallTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MapEditMakeMap(2);
	const FRTCellId Home(0, 0, 0);

	FRTGeometrySegment First;
	FRTGeometrySegment Second;
	if (!TestTrue(TEXT("le corde si agganciano"),
		MapEditMakeChord(0, First) && MapEditMakeChord(1, Second)))
	{
		return false;
	}

	// DUE muri anonimi sulla stessa cella: il nome non li distingue, e nemmeno la cella. Con un solo muro
	// il test passerebbe anche con una risoluzione che tira a indovinare.
	Map->InteriorWalls.Add(FRTHexInteriorWall(Home, First));
	Map->InteriorWalls.Add(FRTHexInteriorWall(Home, Second));

	if (!TestEqual(TEXT("due muri anonimi sono uno stato valido"), Map->ValidateMap().Num(), 0))
	{
		return false;
	}

	const TArray<FRTMapElementHandle> At = URTMapEditLibrary::ElementsAt(Map, Home, ERTHexDirection::E);

	// I due muri piu' la cella.
	if (!TestEqual(TEXT("i due muri anonimi sono candidati"), At.Num(), 3))
	{
		return false;
	}

	// 🔴 Il cuore: ogni candidato emesso deve RISOLVERE. Un handle che non risolve e' un candidato finto.
	int32 ResolvedCount = 0;
	for (const FRTMapElementHandle& Handle : At)
	{
		if (Handle.Kind != ERTMapElementKind::InteriorWall)
		{
			continue;
		}
		const int32 Index = URTMapEditLibrary::ResolveInteriorWall(Map, Handle);
		if (TestTrue(TEXT("un candidato muro risolve"), Index != INDEX_NONE))
		{
			++ResolvedCount;
		}
	}
	if (!TestEqual(TEXT("entrambi i muri anonimi risolvono"), ResolvedCount, 2))
	{
		return false;
	}

	// E risolvono a DUE muri diversi: se collassassero sullo stesso, il ciclo di click girerebbe a vuoto.
	const int32 A = URTMapEditLibrary::ResolveInteriorWall(Map, At[0]);
	const int32 B = URTMapEditLibrary::ResolveInteriorWall(Map, At[1]);
	TestTrue(TEXT("i due candidati sono due muri distinti"), A != B);

	return true;
}

/**
 * 🔑 Dal punto cliccato al BORDO, senza inventare una seconda convenzione angolare.
 *
 * Il ciclo di selezione ha bisogno di `(Cella, Bordo)`, ma il viewport da' un punto. La conversione e'
 * geometria pura, quindi sta nel runtime e si prova headless — non nel tool, dove sarebbe verificabile solo
 * a schermo.
 *
 * ⚠️ **Il bordo si trova confrontando le distanze dai sei `EdgeMidpointWorld`, non ricavando un angolo.**
 * La convenzione dei sei lati esiste gia' li' dentro: riscriverla come `Atan2` diviso in spicchi
 * significherebbe averne due, e il giorno in cui una delle due cambiasse mentirebbero in silenzio. E' lo
 * stesso criterio per cui l'orientamento di uno `EdgeBound` viene da `EdgeRotation` e non da un angolo
 * inciso a mano.
 *
 * Il test gira su **tutti e sei** i bordi: con uno solo passerebbe anche una funzione che ne restituisce
 * sempre uno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapEditNearestEdgeTest,
	"RefactorTactics.Map.Edit.AClickedPointResolvesToItsNearestEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapEditNearestEdgeTest::RunTest(const FString&)
{
	const FRTCellId Cell(0, 0, 0);
	const FVector Origin = FVector::ZeroVector;
	constexpr float LayerHeight = 300.f;

	const ERTHexDirection AllSix[] = {
		ERTHexDirection::E, ERTHexDirection::NE, ERTHexDirection::NW,
		ERTHexDirection::W, ERTHexDirection::SW, ERTHexDirection::SE
	};

	const FVector Center = URTHexLibrary::AxialToWorld(Cell, Origin, MapEditHexSize, LayerHeight);

	for (const ERTHexDirection Dir : AllSix)
	{
		const FVector Mid = URTHexLibrary::EdgeMidpointWorld(Cell, Dir, Origin, MapEditHexSize, LayerHeight);

		// Un punto appena DENTRO la cella rispetto al centro del bordo: e' dove cade un click che mira a
		// quel lato senza essere esattamente sul confine.
		const FVector Inside = Mid + (Center - Mid) * 0.25;

		TestEqual(*FString::Printf(TEXT("il punto vicino al bordo %d risolve a quel bordo"),
			static_cast<int32>(Dir)),
			URTHexLibrary::NearestEdgeDirection(Cell, Inside, Origin, MapEditHexSize, LayerHeight), Dir);
	}

	// ⚠️ Il centro esatto della cella non appartiene a nessun bordo piu' che a un altro. La funzione deve
	// comunque rispondere in modo DETERMINISTICO: due chiamate identiche danno lo stesso lato, altrimenti un
	// click al centro farebbe saltare la selezione fra bordi diversi a ogni tentativo.
	const ERTHexDirection FromCentre =
		URTHexLibrary::NearestEdgeDirection(Cell, Center, Origin, MapEditHexSize, LayerHeight);
	TestEqual(TEXT("il centro cella da' sempre la stessa risposta"),
		URTHexLibrary::NearestEdgeDirection(Cell, Center, Origin, MapEditHexSize, LayerHeight), FromCentre);

	// E il bordo condiviso resta lo stesso fisico visto dalle due celle: il punto sul lato E di (0,0) e' il
	// lato W del vicino. E' l'invariante che `EdgeMidpointWorld` gia' dichiara, e questa funzione non deve
	// romperlo.
	const FRTCellId EastNeighbour = URTHexLibrary::Neighbor(Cell, ERTHexDirection::E);
	const FVector SharedMid =
		URTHexLibrary::EdgeMidpointWorld(Cell, ERTHexDirection::E, Origin, MapEditHexSize, LayerHeight);
	const FVector NeighbourCentre =
		URTHexLibrary::AxialToWorld(EastNeighbour, Origin, MapEditHexSize, LayerHeight);

	TestEqual(TEXT("lo stesso bordo, visto dal vicino, e' il suo lato opposto"),
		URTHexLibrary::NearestEdgeDirection(EastNeighbour,
			SharedMid + (NeighbourCentre - SharedMid) * 0.25, Origin, MapEditHexSize, LayerHeight),
		ERTHexDirection::W);

	return true;
}

/**
 * `DeleteElement` cancella **ogni tipo**, non solo la cella — ed e' cio' che l'`Erase` del tool chiedera'.
 *
 * Un comportamento solo, quattro forme. Ogni caso riparte da una mappa pulita: concatenarli farebbe
 * dipendere il quarto esito dall'esattezza dei primi tre.
 *
 * 🔴 **Cancellare una PORTA porta via i binding che la nominavano.** E' lo stesso `C2` che la cascata della
 * cella aveva gia' incontrato: `ValidateReferences` segnala il riferimento a una struttura inesistente, e
 * lasciare il binding renderebbe la mappa invalida. La regola non cambia perche' cambia il gesto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapEditDeleteEveryKindTest,
	"RefactorTactics.Map.Edit.DeleteElementHandlesEveryKind",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapEditDeleteEveryKindTest::RunTest(const FString&)
{
	const FRTCellId Home(0, 0, 0);
	const ERTHexDirection Edge = ERTHexDirection::E;

	FRTGeometrySegment Chord;
	if (!TestTrue(TEXT("la corda si aggancia"), MapEditMakeChord(0, Chord)))
	{
		return false;
	}

	// --- 1. Muro interno, per NOME --------------------------------------------------------------------
	{
		URTHexMapAsset* Map = MapEditMakeMap(2);
		FRTHexInteriorWall Wall(Home, Chord);
		Wall.StableId = TEXT("W1");
		Map->InteriorWalls.Add(Wall);

		TestEqual(TEXT("il muro nominato si cancella"),
			URTMapEditLibrary::DeleteElement(Map, FRTMapElementHandle::ForInteriorWall(TEXT("W1"))),
			ERTMapEditOutcome::Applied);
		TestEqual(TEXT("e non c'e' piu'"), Map->InteriorWalls.Num(), 0);
		TestEqual(TEXT("la mappa resta valida"), Map->ValidateMap().Num(), 0);
	}

	// --- 2. Muro interno ANONIMO, per chiave ----------------------------------------------------------
	{
		URTHexMapAsset* Map = MapEditMakeMap(2);
		Map->InteriorWalls.Add(FRTHexInteriorWall(Home, Chord));

		TestEqual(TEXT("il muro anonimo si cancella per chiave"),
			URTMapEditLibrary::DeleteElement(Map, FRTMapElementHandle::ForInteriorWallAt(Home, Chord)),
			ERTMapEditOutcome::Applied);
		TestEqual(TEXT("e non c'e' piu'"), Map->InteriorWalls.Num(), 0);
	}

	// --- 3. Copertura, per chiave naturale ------------------------------------------------------------
	{
		URTHexMapAsset* Map = MapEditMakeMap(2);
		FRTHexCellData Data = *Map->FindCell(Home);
		FRTHexCover Cover;
		Cover.Edge = Edge;
		Cover.Type = ERTHexCoverType::Low;
		Data.Covers.Add(Cover);
		Map->AddOrUpdateCell(Data);

		TestEqual(TEXT("la copertura si cancella"),
			URTMapEditLibrary::DeleteElement(Map, FRTMapElementHandle::ForCover(Home, Edge)),
			ERTMapEditOutcome::Applied);
		TestEqual(TEXT("e il bordo e' sgombro"),
			Map->FindCell(Home)->Covers.Num(), 0);
		TestEqual(TEXT("la mappa resta valida"), Map->ValidateMap().Num(), 0);
	}

	// --- 4. 🔴 Porta, e il binding che la nominava ----------------------------------------------------
	{
		URTHexMapAsset* Map = MapEditMakeMap(2);
		const FRTCellId Other(2, 0, 0);
		MapEditPutDoor(Map, Home, Edge, /*DoorId*/ 1, TEXT("S1"));
		MapEditPutDoor(Map, Other, ERTHexDirection::W, /*DoorId*/ 2, TEXT("D1"));
		Map->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D1") }));

		if (!TestEqual(TEXT("l'allestimento parte valido"), Map->ValidateMap().Num(), 0))
		{
			return false;
		}

		TestEqual(TEXT("la porta si cancella"),
			URTMapEditLibrary::DeleteElement(Map, FRTMapElementHandle::ForDoor(Home, Edge, TEXT("S1"))),
			ERTMapEditOutcome::Applied);
		TestEqual(TEXT("il bordo e' sgombro"), Map->FindCell(Home)->Doors.Num(), 0);

		const TArray<FString> Errors = Map->ValidateMap();
		TestEqual(*FString::Printf(TEXT("e il binding se n'e' andato con lei (residui: %s)"),
			Errors.Num() > 0 ? *FString::Join(Errors, TEXT(" | ")) : TEXT("nessuno")),
			Errors.Num(), 0);
	}

	// --- 5. Un handle che non nomina niente resta un rifiuto ------------------------------------------
	{
		URTHexMapAsset* Map = MapEditMakeMap(2);
		TestEqual(TEXT("una copertura che non c'e' e' un rifiuto"),
			URTMapEditLibrary::DeleteElement(Map, FRTMapElementHandle::ForCover(Home, Edge)),
			ERTMapEditOutcome::RefusedUnresolved);
	}

	return true;
}

/**
 * 🔴 Un binding perde il BERSAGLIO morto, non se stesso — e muore solo se resta senza.
 *
 * La regola era gia' scritta nel contratto di `FRTMapDependencySet`: *«un binding entra qui quando perde la
 * **sorgente** oppure l'**ultimo** bersaglio»*. Nessuna delle due strade la applicava, e sbagliavano in
 * direzioni OPPOSTE — trovato da una code review, non da un test:
 *
 * ```text
 * DeleteElement(porta)   rimuoveva il binding se il nome era sorgente O bersaglio
 *                        -> `S1 -> [D1, D2]`, cancelli D1, e S1 smette di comandare anche D2
 * CollectDependents      guardava solo SourceId
 *                        -> un bersaglio morto restava citato, e `ValidateReferences` lo segnala
 * ```
 *
 * ⚠️ **Il primo e' perdita di dato SILENZIOSA**: nessun errore, nessun log, e un comando che l'autore
 * aveva scritto sparisce. Il secondo lascia la mappa invalida, che almeno il validator dice.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapEditBindingTargetTest,
	"RefactorTactics.Map.Edit.ABindingLosesOnlyTheDeadTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapEditBindingTargetTest::RunTest(const FString&)
{
	const FRTCellId Source(0, 0, 0);
	const FRTCellId First(2, 0, 0);
	const FRTCellId Second(0, 2, 0);

	// `S1` comanda DUE porte. Con una sola il test non distinguerebbe «toglie il bersaglio» da «toglie il
	// binding»: entrambi lascerebbero zero binding.
	auto MakeMap = [&]()
	{
		URTHexMapAsset* M = MapEditMakeMap(2);
		MapEditPutDoor(M, Source, ERTHexDirection::E, 1, TEXT("S1"));
		MapEditPutDoor(M, First,  ERTHexDirection::W, 2, TEXT("D1"));
		MapEditPutDoor(M, Second, ERTHexDirection::W, 3, TEXT("D2"));
		M->InteractionBindings.Add(FRTInteractionBinding(TEXT("S1"), { TEXT("D1"), TEXT("D2") }));
		return M;
	};

	// --- 1. Cancellata UNA delle due porte comandate: il binding SOPRAVVIVE, con l'altro bersaglio ---
	{
		URTHexMapAsset* Map = MakeMap();
		if (!TestEqual(TEXT("l'allestimento parte valido"), Map->ValidateMap().Num(), 0))
		{
			return false;
		}

		TestEqual(TEXT("la porta si cancella"),
			URTMapEditLibrary::DeleteElement(Map,
				FRTMapElementHandle::ForDoor(First, ERTHexDirection::W, TEXT("D1"))),
			ERTMapEditOutcome::Applied);

		if (!TestEqual(TEXT("il binding e' ancora li'"), Map->InteractionBindings.Num(), 1))
		{
			return false;
		}
		TestEqual(TEXT("ma comanda un bersaglio solo"),
			Map->InteractionBindings[0].TargetIds.Num(), 1);
		TestEqual(TEXT("e resta quello vivo"),
			Map->InteractionBindings[0].TargetIds[0], FName(TEXT("D2")));

		const TArray<FString> Errors = Map->ValidateMap();
		TestEqual(*FString::Printf(TEXT("la mappa resta valida (residui: %s)"),
			Errors.Num() > 0 ? *FString::Join(Errors, TEXT(" | ")) : TEXT("nessuno")),
			Errors.Num(), 0);
	}

	// --- 2. Cancellato l'ULTIMO bersaglio: il binding muore, perche' senza bersagli e' gia' invalido ---
	{
		URTHexMapAsset* Map = MakeMap();
		URTMapEditLibrary::DeleteElement(Map, FRTMapElementHandle::ForDoor(First, ERTHexDirection::W, TEXT("D1")));
		URTMapEditLibrary::DeleteElement(Map, FRTMapElementHandle::ForDoor(Second, ERTHexDirection::W, TEXT("D2")));

		TestEqual(TEXT("senza bersagli il binding se ne va"), Map->InteractionBindings.Num(), 0);
		TestEqual(TEXT("e la mappa resta valida"), Map->ValidateMap().Num(), 0);
	}

	// --- 3. Cancellata la SORGENTE: il binding muore ---------------------------------------------------
	{
		URTHexMapAsset* Map = MakeMap();
		URTMapEditLibrary::DeleteElement(Map, FRTMapElementHandle::ForDoor(Source, ERTHexDirection::E, TEXT("S1")));

		TestEqual(TEXT("senza sorgente il binding se ne va"), Map->InteractionBindings.Num(), 0);
		TestEqual(TEXT("e la mappa resta valida"), Map->ValidateMap().Num(), 0);
	}

	// --- 4. 🔴 La stessa regola dalla CASCATA DELLA CELLA, che sbagliava all'opposto ------------------
	{
		URTHexMapAsset* Map = MakeMap();

		// La cella di `D1` se ne va: `D1` era un BERSAGLIO, e la cascata guardava solo la sorgente.
		TestEqual(TEXT("la cella si cancella"),
			URTMapEditLibrary::DeleteElement(Map, FRTMapElementHandle::ForCell(First)),
			ERTMapEditOutcome::Applied);

		const TArray<FString> Errors = Map->ValidateMap();
		TestEqual(*FString::Printf(TEXT("nessun bersaglio fantasma resta citato (residui: %s)"),
			Errors.Num() > 0 ? *FString::Join(Errors, TEXT(" | ")) : TEXT("nessuno")),
			Errors.Num(), 0);

		if (TestEqual(TEXT("e il binding sopravvive con l'altro bersaglio"),
			Map->InteractionBindings.Num(), 1))
		{
			TestEqual(TEXT("che e' D2"), Map->InteractionBindings[0].TargetIds.Num(), 1);
		}
	}

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// AddDoor (#2330) — l'anello che mancava all'authoring delle porte.
//
// Misurato prima di scriverlo: mesh del kit committate, `ARTHexMapActor` che le disegna, `Interact` che le
// apre e formato mappa dalla v4 — ma NIENTE sapeva crearne una su un asset. La conseguenza, misurata da
// `#2312`: nell'intero contenuto versionato non esiste una porta.
// ---------------------------------------------------------------------------------------------------------

namespace
{
	/** Il primo bordo di `Cell` che porta FUORI dalla mappa, se esiste. Nome prefissato: unity build. */
	bool MapEditFindFrontierEdge(const URTHexMapAsset* Map, const FRTCellId& Cell, ERTHexDirection& Out)
	{
		for (int32 D = 0; D < 6; ++D)
		{
			const ERTHexDirection Dir = static_cast<ERTHexDirection>(D);
			if (Map->FindCell(URTHexLibrary::Neighbor(Cell, Dir)) == nullptr)
			{
				Out = Dir;
				return true;
			}
		}
		return false;
	}

	/** Il primo bordo di `Cell` che porta a una cella ESISTENTE. */
	bool MapEditFindInteriorEdge(const URTHexMapAsset* Map, const FRTCellId& Cell, ERTHexDirection& Out)
	{
		for (int32 D = 0; D < 6; ++D)
		{
			const ERTHexDirection Dir = static_cast<ERTHexDirection>(D);
			if (Map->FindCell(URTHexLibrary::Neighbor(Cell, Dir)) != nullptr)
			{
				Out = Dir;
				return true;
			}
		}
		return false;
	}
}

/**
 * **La porta si posa, e la REVISIONE si muove.**
 *
 * 🔴 La seconda meta' non e' decorativa: `Revision` e' cio' che invalida i percorsi gia' calcolati, e una
 * scrittura in place su `Cells[i].Doors` non la muoverebbe. Un cammino calcolato prima resterebbe valido
 * dopo, attraverso una porta appena chiusa — il difetto che `ClearAsset` ha gia' pagato e che l'header
 * dell'asset dichiara: *«la revisione e' responsabilita' del DATO, non di chi lo modifica»*.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapEditAddDoorAppliesTest,
	"RefactorTactics.MapEdit.AddDoorAppliesAndBumpsRevision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapEditAddDoorAppliesTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MapEditMakeMap(2);
	const FRTCellId Centro(0, 0, 0);

	ERTHexDirection Bordo;
	if (!TestTrue(TEXT("il centro ha almeno un vicino"), MapEditFindInteriorEdge(M, Centro, Bordo)))
	{
		return false;
	}

	const int32 RevisionePrima = M->Revision;
	TestEqual(TEXT("la posa e' applicata"),
		URTMapEditLibrary::AddDoor(M, Centro, Bordo, ERTHexDoorState::Closed),
		ERTMapEditOutcome::Applied);

	const FRTHexCellData* Data = M->FindCell(Centro);
	if (!TestNotNull(TEXT("la cella esiste ancora"), Data))
	{
		return false;
	}
	if (!TestEqual(TEXT("e porta esattamente una porta"), Data->Doors.Num(), 1))
	{
		return false;
	}
	TestEqual(TEXT("sul bordo richiesto"), static_cast<int32>(Data->Doors[0].Edge), static_cast<int32>(Bordo));
	TestEqual(TEXT("nello stato richiesto"),
		static_cast<int32>(Data->Doors[0].State), static_cast<int32>(ERTHexDoorState::Closed));

	// ⛔ **Non inventa un nome pubblico.** `NAME_None` e' legale ed e' cio' che ogni porta pre-v9 e' diventata
	// rileggendosi; sceglierlo qui sarebbe decidere del contenuto.
	TestTrue(TEXT("e senza StableId, che non si inventa"), Data->Doors[0].StableId.IsNone());

	TestTrue(FString::Printf(TEXT("la revisione si e' mossa: %d -> %d"), RevisionePrima, M->Revision),
		M->Revision > RevisionePrima);
	return true;
}

/**
 * **Sul bordo esterno la posa e' RIFIUTATA**, ed e' il rifiuto che rende `AddDoor` non banale.
 *
 * Una porta e' **sottrattiva** (`spec-porte-cp93.md`): nega un'adiacenza che esiste. Fuori dalla mappa non
 * c'e' adiacenza da negare, quindi la porta si salverebbe, cambierebbe l'hash, si vedrebbe pure — e nessun
 * oracolo suonerebbe. E' la classe di difetto che `#170` ha pagato tre settimane.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapEditAddDoorFrontierTest,
	"RefactorTactics.MapEdit.AddDoorRefusesFrontierEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapEditAddDoorFrontierTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MapEditMakeMap(2);

	// Una cella di bordo: su un'arena piatta di raggio 2, `(2,0)` sta sul perimetro.
	const FRTCellId Bordo(2, 0, 0);
	if (!TestNotNull(TEXT("la cella di perimetro esiste"), M->FindCell(Bordo)))
	{
		return false;
	}

	ERTHexDirection Fuori;
	if (!TestTrue(TEXT("e ha almeno un bordo che porta fuori dalla mappa"),
		MapEditFindFrontierEdge(M, Bordo, Fuori)))
	{
		return false;
	}

	const int32 RevisionePrima = M->Revision;
	TestEqual(TEXT("la posa e' rifiutata, e con la ragione giusta"),
		URTMapEditLibrary::AddDoor(M, Bordo, Fuori, ERTHexDoorState::Closed),
		ERTMapEditOutcome::RefusedNoNeighbour);

	// «O si applica intera o non lascia traccia»: il rifiuto non deve aver scritto niente.
	TestEqual(TEXT("niente e' stato scritto"), M->FindCell(Bordo)->Doors.Num(), 0);
	TestEqual(TEXT("e la revisione non si e' mossa"), M->Revision, RevisionePrima);

	// E la cella inesistente e' un rifiuto DIVERSO: due cause con lo stesso esito manderebbero a correggere
	// la cosa sbagliata.
	TestEqual(TEXT("una cella che non esiste e' RefusedNoSuchCell, non RefusedNoNeighbour"),
		URTMapEditLibrary::AddDoor(M, FRTCellId(99, 99, 0), ERTHexDirection::E, ERTHexDoorState::Closed),
		ERTMapEditOutcome::RefusedNoSuchCell);
	return true;
}

/** **Due porte sullo stesso bordo non si accumulano**: `ValidateMap` gia' pretende l'unicita' per bordo. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapEditAddDoorDuplicateTest,
	"RefactorTactics.MapEdit.AddDoorRefusesDuplicateEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapEditAddDoorDuplicateTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MapEditMakeMap(2);
	const FRTCellId Centro(0, 0, 0);

	ERTHexDirection Bordo;
	if (!TestTrue(TEXT("il centro ha almeno un vicino"), MapEditFindInteriorEdge(M, Centro, Bordo)))
	{
		return false;
	}

	TestEqual(TEXT("la prima passa"),
		URTMapEditLibrary::AddDoor(M, Centro, Bordo, ERTHexDoorState::Closed), ERTMapEditOutcome::Applied);

	const int32 RevisioneDopoLaPrima = M->Revision;
	TestEqual(TEXT("la seconda sullo stesso bordo e' rifiutata"),
		URTMapEditLibrary::AddDoor(M, Centro, Bordo, ERTHexDoorState::Locked),
		ERTMapEditOutcome::RefusedDuplicate);
	TestEqual(TEXT("e la porta resta UNA"), M->FindCell(Centro)->Doors.Num(), 1);
	TestEqual(TEXT("lo stato non e' stato sovrascritto"),
		static_cast<int32>(M->FindCell(Centro)->Doors[0].State), static_cast<int32>(ERTHexDoorState::Closed));
	TestEqual(TEXT("e la revisione non si e' mossa una seconda volta"), M->Revision, RevisioneDopoLaPrima);
	return true;
}

/**
 * **Una faccia sola basta, e questo test lo chiede al GIOCO.**
 *
 * `spec-porte-cp93.md` §3: *«il bordo puo' essere dichiarato dalla cella A verso B, da B verso A, o da
 * entrambe … una porta disegnata da un lato solo vale comunque»*, e `DoorBetween` vale *«il piu' RESTRITTIVO
 * delle due facce»*. `AddDoor` scrive **una** faccia proprio per questo: scriverne due sarebbe la divergenza
 * che quella regola evita.
 *
 * 🔑 **Se questo test cade, il difetto NON e' in `AddDoor`**: e' nella lettura, e la spec sta mentendo. E'
 * l'unico dei quattro che misura il gioco invece del tool, ed e' la ragione per cui vale la pena scriverlo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapEditAddDoorBothSidesTest,
	"RefactorTactics.MapEdit.AddDoorIsVisibleFromBothSides",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapEditAddDoorBothSidesTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MapEditMakeMap(2);
	const FRTCellId A(0, 0, 0);

	ERTHexDirection Bordo;
	if (!TestTrue(TEXT("il centro ha almeno un vicino"), MapEditFindInteriorEdge(M, A, Bordo)))
	{
		return false;
	}
	const FRTCellId B = URTHexLibrary::Neighbor(A, Bordo);

	// La premessa, asserita: senza porta i due versi rispondono `Open`, altrimenti il verde sotto non
	// distinguerebbe «la porta si vede da entrambi i lati» da «rispondono sempre la stessa cosa».
	TestEqual(TEXT("premessa: senza porta il bordo e' Open da A"),
		static_cast<int32>(URTHexDoorLibrary::DoorBetween(M, A, B)),
		static_cast<int32>(ERTHexDoorState::Open));

	TestEqual(TEXT("si scrive UNA sola faccia"),
		URTMapEditLibrary::AddDoor(M, A, Bordo, ERTHexDoorState::Closed), ERTMapEditOutcome::Applied);
	TestEqual(TEXT("e infatti l'altra cella non ne porta nessuna"), M->FindCell(B)->Doors.Num(), 0);

	TestEqual(TEXT("ma la si vede da A verso B"),
		static_cast<int32>(URTHexDoorLibrary::DoorBetween(M, A, B)),
		static_cast<int32>(ERTHexDoorState::Closed));
	TestEqual(TEXT("e anche da B verso A: la barriera e' fisica, non direzionale"),
		static_cast<int32>(URTHexDoorLibrary::DoorBetween(M, B, A)),
		static_cast<int32>(ERTHexDoorState::Closed));
	return true;
}


/**
 * **L'arco di transizione è un elemento autorato come gli altri** (#1864).
 *
 * 🔴 Fino al 2026-09-23 `ERTMapElementKind::Transition` era un valore che **nasceva morto**: dichiarato,
 * senza un costruttore che lo producesse, e con `DeleteElement` che lo rifiutava per una ragione scritta —
 * *«`Transition` non ha ancora un gesto che la selezioni»*. Il gesto c'era già: `URTHexArchTool` possiede
 * da sempre il hit-test di viewport che la spec §13.3 gli assegna. Mancava solo il collegamento.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapEditDeleteTransitionTest,
	"RefactorTactics.Map.Edit.DeleteTransitionRemovesBothDirections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapEditDeleteTransitionTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MapEditMakeMap(2);
	if (!TestNotNull(TEXT("la mappa di prova esiste"), Map))
	{
		return false;
	}

	const FRTCellId Basso(0, 0, 0);
	const FRTCellId Alto(0, 0, 1);

	// Andata e ritorno come due `FRTHexEdge` distinti: è così che l'asset li tiene.
	Map->Transitions.Add(FRTHexEdge(Basso, Alto, /*Cost*/ 1));
	Map->Transitions.Add(FRTHexEdge(Alto, Basso, /*Cost*/ 1));
	const int32 Prima = Map->Transitions.Num();
	if (!TestEqual(TEXT("due direzioni in partenza"), Prima, 2))
	{
		return false;
	}

	// ⚠️ Si cancella nominando la coppia AL CONTRARIO di come la prima è stata inserita: per chi seleziona,
	// andata e ritorno sono lo stesso arco, e l'handle non deve sapere quale delle due è stata cliccata.
	const ERTMapEditOutcome Esito =
		URTMapEditLibrary::DeleteElement(Map, FRTMapElementHandle::ForTransition(Alto, Basso));

	TestEqual(TEXT("l'arco si cancella"), static_cast<int32>(Esito), static_cast<int32>(ERTMapEditOutcome::Applied));
	TestEqual(TEXT("e se ne vanno ENTRAMBE le direzioni, non una"), Map->Transitions.Num(), 0);

	// Controllo NEGATIVO: sulla mappa ormai senza archi, la stessa chiamata si rifiuta invece di rispondere
	// `Applied` a vuoto. Senza questo, un `DeleteElement` che non trova niente e dice «fatto» passerebbe.
	const ERTMapEditOutcome Vuoto =
		URTMapEditLibrary::DeleteElement(Map, FRTMapElementHandle::ForTransition(Alto, Basso));
	TestEqual(TEXT("un arco che non c'e' si RIFIUTA, non si dichiara cancellato"),
		static_cast<int32>(Vuoto), static_cast<int32>(ERTMapEditOutcome::RefusedUnresolved));

	return true;
}

/**
 * **La passata a vuoto risponde come quella vera, e non tocca la mappa** (#1864).
 *
 * 🔑 Serve alla cancellazione multipla, che deve essere tutto-o-niente: senza una prova preventiva, un
 * rifiuto a metà lascia applicati i precedenti. ⛔ E la via che sembrerebbe ovvia non funziona —
 * `FScopedTransaction::Cancel()` **non ripristina** (`UTransBuffer::Cancel`, `EditorTransaction.cpp`): toglie
 * la transazione dal buffer di undo e lascia lo stato com'è.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapEditDryRunTest,
	"RefactorTactics.Map.Edit.DryRunAnswersTheSameAndChangesNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapEditDryRunTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MapEditMakeMap(2);
	if (!TestNotNull(TEXT("la mappa di prova esiste"), Map))
	{
		return false;
	}

	const FRTCellId Basso(0, 0, 0);
	const FRTCellId Alto(0, 0, 1);
	Map->Transitions.Add(FRTHexEdge(Basso, Alto, /*Cost*/ 1));

	const int32 CelleP = Map->Cells.Num();
	const int32 ArchiP = Map->Transitions.Num();
	const int32 MuriP = Map->InteriorWalls.Num();

	int32 Esaminati = 0;

	// Ogni Kind che può entrare in selezione, provato a vuoto: la risposta deve essere quella vera, e la
	// mappa deve restare identica.
	const TArray<FRTMapElementHandle> Prove = {
		FRTMapElementHandle::ForTransition(Basso, Alto),
		FRTMapElementHandle::ForCell(Basso),
	};

	for (const FRTMapElementHandle& Handle : Prove)
	{
		const ERTMapEditOutcome Prova = URTMapEditLibrary::DeleteElement(Map, Handle, /*bDryRun=*/ true);
		++Esaminati;
		TestEqual(TEXT("la prova a vuoto risponde Applied su un elemento che esiste"),
			static_cast<int32>(Prova), static_cast<int32>(ERTMapEditOutcome::Applied));
	}

	// Guardia anti-vacuità: se l'elenco smettesse di enumerare, il ciclo girerebbe a vuoto e il test
	// passerebbe senza aver provato niente.
	TestTrue(TEXT("almeno due Kind esaminati a vuoto"), Esaminati >= 2);

	TestEqual(TEXT("la prova a vuoto non toglie celle"), Map->Cells.Num(), CelleP);
	TestEqual(TEXT("ne' archi"), Map->Transitions.Num(), ArchiP);
	TestEqual(TEXT("ne' muri interni"), Map->InteriorWalls.Num(), MuriP);

	// Controllo POSITIVO: la stessa chiamata SENZA `bDryRun` cambia davvero la mappa. Senza questo, una
	// `DeleteElement` che non facesse mai niente passerebbe entrambe le asserzioni qui sopra.
	const ERTMapEditOutcome Vera =
		URTMapEditLibrary::DeleteElement(Map, FRTMapElementHandle::ForTransition(Basso, Alto));
	TestEqual(TEXT("senza bDryRun l'esito e' lo stesso"),
		static_cast<int32>(Vera), static_cast<int32>(ERTMapEditOutcome::Applied));
	TestEqual(TEXT("ma ORA l'arco e' sparito davvero"), Map->Transitions.Num(), ArchiP - 1);

	return true;
}

namespace
{
	/**
	 * Un diametro per il CENTRO lungo l'asse dato. `Offset == 0` significa, per definizione della
	 * grammatica, «passa per il centro della cella» — la forma che `#2085` ha stabilito essere un MURO
	 * INTERNO e non una copertura.
	 *
	 * ⚠️ Gli assi DISPARI (`Deg30`, `Deg90`, `Deg150`) puntano ai VERTICI, e tre di essi occupano tutti e
	 * dodici i settori: e' l'aritmetica gia' misurata da `RefactorTactics.Standability.*`, riusata qui
	 * perche' e' l'unico modo noto di rendere una cella non calpestabile con la sola geometria.
	 */
	FRTGeometrySegment MapEditDiameter(ERTTacticalAxis Axis)
	{
		FRTGeometrySegment S;
		S.Axis = Axis;
		S.Offset = 0;
		S.AlongStart = -RT_GeometryQuanta;
		S.AlongEnd = RT_GeometryQuanta;
		S.Layer = 0;
		S.WallType = ERTHexCoverType::High;
		return S;
	}

	/** Da' un nome al muro interno che vive su `(Cell, Segment)`, perche' l'handle del move e' un NOME. */
	bool MapEditNameWall(URTHexMapAsset* Map, const FRTCellId& Cell, const FRTGeometrySegment& Segment,
		FName StableId)
	{
		const int32 Index = Map->InteriorWalls.IndexOfByPredicate(
			[&Cell, &Segment](const FRTHexInteriorWall& Wall)
			{
				return Wall.Cell == Cell && Wall.Segment == Segment;
			});
		if (Index == INDEX_NONE)
		{
			return false;
		}
		Map->InteriorWalls[Index].StableId = StableId;
		return true;
	}

	/** Quante segnalazioni di validazione riguardano questa cella. */
	int32 MapEditIssuesOn(const URTHexMapAsset* Map, const FRTCellId& Cell)
	{
		TArray<FRTMapValidationIssue> Issues;
		Map->ValidateMapDetailed(Issues);

		int32 Count = 0;
		for (const FRTMapValidationIssue& Issue : Issues)
		{
			if (Issue.Cell == Cell)
			{
				++Count;
			}
		}
		return Count;
	}
}

/**
 * LA COTTURA SEGUE IL MOVE, e segue le SOLE celle che il move ha toccato (#1864, casella 4).
 *
 * 🔴 **Il difetto che questo test esiste per fermare**: `MoveInteriorWall` scriveva `Cell` e `Segment` e
 * non ricuoceva niente — dentro `RTMapEditLibrary.cpp` l'unica cosa che veniva chiamata della cottura era
 * `EdgesTouchedBy`, che e' una domanda sui bordi e non deriva nulla. Ne seguivano DUE segnalazioni da un
 * gesto solo, e una era un **errore**:
 *
 * ```text
 * cella d'ORIGINE   il muro se n'e' andato, ma bMovementBlockGenerated resta acceso
 *                   -> REGOLA 4  StaleGeneratedBlock  (warning)
 * cella d'ARRIVO    il muro chiude l'ultima posa, ma bBlocksMovement resta spento
 *                   -> REGOLA 1  NoLegalPlacement     (ERRORE)
 * ```
 *
 * ⚠️ **«La sola cella» e' meta' del criterio, ed e' la meta' che un test ingenuo non misura.** Il
 * testimone `Lontana` porta un blocco stantio DELIBERATO su una cella che il move non tocca: se la
 * correzione ricuocesse l'intera mappa quel blocco sparirebbe, nessuna asserzione sulle due celle mosse lo
 * vedrebbe, e la mappa risulterebbe perfino «piu' valida». E' il controllo che distingue una ricottura
 * mirata da una passata globale, e senza di esso il criterio sarebbe verificato a meta'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapEditMoveRebakesTouchedCellsTest,
	"RefactorTactics.Map.Edit.MoveRebakesOnlyTheCellsItTouched",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapEditMoveRebakesTouchedCellsTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MapEditMakeMap(2);

	const FRTCellId Origine(0, 0, 0);
	const FRTCellId Arrivo(1, 0, 0);
	const FRTCellId Lontana(-2, 0, 0);

	const FRTGeometrySegment D30 = MapEditDiameter(ERTTacticalAxis::Deg30);
	const FRTGeometrySegment D90 = MapEditDiameter(ERTTacticalAxis::Deg90);
	const FRTGeometrySegment D150 = MapEditDiameter(ERTTacticalAxis::Deg150);

	// L'origine: tre diametri, quindi dodici settori occupati e nessuna posa legale.
	URTGeometryBakeLibrary::BakeCell(Map, Origine, { D30, D90, D150 }, MapEditHexSize);
	// L'arrivo: due soli, quindi ancora calpestabile — il terzo la chiudera'.
	URTGeometryBakeLibrary::BakeCell(Map, Arrivo, { D30, D90 }, MapEditHexSize);

	const FRTHexCellData* Prima = Map->FindCell(Origine);
	if (!TestNotNull(TEXT("l'origine esiste"), Prima)) { return false; }
	if (!TestTrue(TEXT("l'allestimento chiude l'origine, e per geometria"),
		Prima->bBlocksMovement && Prima->bMovementBlockGenerated))
	{
		return false; // senza questo l'intero test non discriminerebbe: non ci sarebbe niente da ricuocere
	}
	const FRTHexCellData* ArrivoPrima = Map->FindCell(Arrivo);
	if (!TestNotNull(TEXT("l'arrivo esiste"), ArrivoPrima)) { return false; }
	if (!TestFalse(TEXT("e l'arrivo e' ancora calpestabile"), ArrivoPrima->bBlocksMovement))
	{
		return false;
	}

	// IL TESTIMONE: un blocco derivato stantio su una cella che il move non tocca. Sopravvivere e' il suo
	// mestiere — se sparisce, la cottura ha camminato su tutta la mappa.
	FRTHexCellData Testimone = *Map->FindCell(Lontana);
	Testimone.bBlocksMovement = true;
	Testimone.bMovementBlockGenerated = true;
	Map->AddOrUpdateCell(Testimone);
	if (!TestEqual(TEXT("il testimone nasce con la sua segnalazione"),
		MapEditIssuesOn(Map, Lontana), 1))
	{
		return false;
	}

	if (!TestTrue(TEXT("il muro da spostare prende un nome"),
		MapEditNameWall(Map, Origine, D150, TEXT("W150"))))
	{
		return false;
	}

	const ERTMapEditOutcome Outcome = URTMapEditLibrary::MoveInteriorWall(
		Map, FRTMapElementHandle::ForInteriorWall(TEXT("W150")), Arrivo, D150);
	if (!TestEqual(TEXT("il move si applica"),
		static_cast<int32>(Outcome), static_cast<int32>(ERTMapEditOutcome::Applied)))
	{
		return false;
	}

	// --- l'ORIGINE ha perso un muro: la posa torna possibile, e il blocco DERIVATO se ne va con essa ---
	const FRTHexCellData* Dopo = Map->FindCell(Origine);
	if (!TestNotNull(TEXT("l'origine esiste ancora"), Dopo)) { return false; }
	TestFalse(TEXT("l'origine torna calpestabile"), Dopo->bBlocksMovement);
	TestFalse(TEXT("e non resta la provenienza accesa"), Dopo->bMovementBlockGenerated);
	TestEqual(TEXT("l'origine non produce segnalazioni"), MapEditIssuesOn(Map, Origine), 0);

	// --- l'ARRIVO ha guadagnato il terzo: si chiude, e si dichiara chiusa -----------------------------
	const FRTHexCellData* ArrivoDopo = Map->FindCell(Arrivo);
	if (!TestNotNull(TEXT("l'arrivo esiste ancora"), ArrivoDopo)) { return false; }
	TestTrue(TEXT("l'arrivo si chiude"), ArrivoDopo->bBlocksMovement);
	TestTrue(TEXT("e il blocco e' DERIVATO, non d'autore"), ArrivoDopo->bMovementBlockGenerated);
	TestEqual(TEXT("l'arrivo non produce segnalazioni"), MapEditIssuesOn(Map, Arrivo), 0);

	// --- e la mappa NON e' stata ricotta tutta: il testimone e' intatto -------------------------------
	const FRTHexCellData* TestimoneDopo = Map->FindCell(Lontana);
	if (!TestNotNull(TEXT("il testimone esiste"), TestimoneDopo)) { return false; }
	TestTrue(TEXT("il testimone conserva il suo blocco: la cottura non ha camminato su tutta la mappa"),
		TestimoneDopo->bBlocksMovement && TestimoneDopo->bMovementBlockGenerated);
	TestEqual(TEXT("e la sua segnalazione e' ancora li'"), MapEditIssuesOn(Map, Lontana), 1);

	return true;
}

/**
 * LA COTTURA SEGUE ANCHE LA CANCELLAZIONE (#1864, casella 7).
 *
 * 🔑 **Il difetto e' SIMMETRICO a quello del move, e la issue non lo nominava.** Il corpo di #1864
 * dichiarava la cottura mancante come un problema del solo `MoveInteriorWall`; misurando si e' visto che
 * `DeleteElement` ha la stessa lacuna e produce lo stesso `StaleGeneratedBlock`. Cancellare l'ultimo muro
 * che chiudeva una cella la lasciava chiusa per una geometria che non c'era piu'.
 *
 * E' la casella 7 alla lettera — *«nessuna operazione lascia un elemento orfano che `ValidateMap`
 * segnalerebbe»*: un `bBlocksMovement` derivato senza la sua geometria e' esattamente quell'orfano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapEditDeleteRebakesItsCellTest,
	"RefactorTactics.Map.Edit.DeletingAnInteriorWallRebakesItsCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapEditDeleteRebakesItsCellTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MapEditMakeMap(2);
	const FRTCellId Home(0, 0, 0);

	const FRTGeometrySegment D30 = MapEditDiameter(ERTTacticalAxis::Deg30);
	const FRTGeometrySegment D90 = MapEditDiameter(ERTTacticalAxis::Deg90);
	const FRTGeometrySegment D150 = MapEditDiameter(ERTTacticalAxis::Deg150);

	URTGeometryBakeLibrary::BakeCell(Map, Home, { D30, D90, D150 }, MapEditHexSize);

	const FRTHexCellData* Prima = Map->FindCell(Home);
	if (!TestNotNull(TEXT("la cella esiste"), Prima)) { return false; }
	if (!TestTrue(TEXT("l'allestimento la chiude per geometria"),
		Prima->bBlocksMovement && Prima->bMovementBlockGenerated))
	{
		return false;
	}

	if (!TestTrue(TEXT("il muro da cancellare prende un nome"),
		MapEditNameWall(Map, Home, D150, TEXT("W150"))))
	{
		return false;
	}

	const ERTMapEditOutcome Outcome = URTMapEditLibrary::DeleteElement(
		Map, FRTMapElementHandle::ForInteriorWall(TEXT("W150")));
	if (!TestEqual(TEXT("la cancellazione si applica"),
		static_cast<int32>(Outcome), static_cast<int32>(ERTMapEditOutcome::Applied)))
	{
		return false;
	}

	// CONTROPROVA: il muro se n'e' andato davvero. Senza, una `DeleteElement` che non facesse niente
	// lascerebbe la cella coerente con se' stessa e le asserzioni qui sotto non proverebbero nulla.
	TestEqual(TEXT("il muro e' sparito"), Map->InteriorWalls.Num(), 2);

	const FRTHexCellData* Dopo = Map->FindCell(Home);
	if (!TestNotNull(TEXT("la cella esiste ancora"), Dopo)) { return false; }
	TestFalse(TEXT("la cella torna calpestabile"), Dopo->bBlocksMovement);
	TestFalse(TEXT("e la provenienza non resta accesa"), Dopo->bMovementBlockGenerated);
	TestEqual(TEXT("e non resta nessuna segnalazione"), MapEditIssuesOn(Map, Home), 0);

	return true;
}

/**
 * LA COTTURA NON CONTRADDICE L'AUTORE, nemmeno quando il gesto la invita a farlo (#1864).
 *
 * ⛔ **E' il confine della correzione, e va misurato insieme a essa.** `DeriveStandability` dichiara
 * *«l'autore vince»* — un `bBlocksMovement` dipinto a mano (`bMovementBlockGenerated == false`) non si
 * tocca — e `ValidateMap` REGOLA 4 dichiara la stessa cosa non segnalandolo. Una ricottura agganciata al
 * move che spegnesse quel blocco sarebbe uno strumento che cancella una scelta di design mentre l'autore
 * ne sposta un'altra: il Non-goal *«nessuna rimozione automatica di un elemento la cui dipendenza e'
 * ambigua»*, applicato al volume.
 *
 * ⚠️ Senza questo test la correzione passerebbe comunque i due qui sopra, perche' li' il blocco e' sempre
 * derivato. E' il caso che distingue «ricuoci» da «ricuoci cio' che e' tuo».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapEditRebakeLeavesTheAuthorAloneTest,
	"RefactorTactics.Map.Edit.TheRebakeAfterAMoveLeavesAnAuthoredBlockAlone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapEditRebakeLeavesTheAuthorAloneTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MapEditMakeMap(2);

	const FRTCellId Origine(0, 0, 0);
	const FRTCellId Arrivo(1, 0, 0);

	const FRTGeometrySegment D30 = MapEditDiameter(ERTTacticalAxis::Deg30);

	URTGeometryBakeLibrary::BakeCell(Map, Origine, { D30 }, MapEditHexSize);

	// Il blocco d'AUTORE sull'origine: un diametro solo non la chiuderebbe, quindi il `true` qui sotto puo'
	// venire solo dalla mano di chi ha disegnato la mappa.
	FRTHexCellData Autorata = *Map->FindCell(Origine);
	if (!TestFalse(TEXT("la geometria da sola non chiuderebbe l'origine"), Autorata.bBlocksMovement))
	{
		return false;
	}
	Autorata.bBlocksMovement = true;
	Autorata.bMovementBlockGenerated = false; // d'autore, ed e' il default
	Map->AddOrUpdateCell(Autorata);

	if (!TestTrue(TEXT("il muro prende un nome"), MapEditNameWall(Map, Origine, D30, TEXT("W30"))))
	{
		return false;
	}

	const ERTMapEditOutcome Outcome = URTMapEditLibrary::MoveInteriorWall(
		Map, FRTMapElementHandle::ForInteriorWall(TEXT("W30")), Arrivo, D30);
	if (!TestEqual(TEXT("il move si applica"),
		static_cast<int32>(Outcome), static_cast<int32>(ERTMapEditOutcome::Applied)))
	{
		return false;
	}

	const FRTHexCellData* Dopo = Map->FindCell(Origine);
	if (!TestNotNull(TEXT("l'origine esiste"), Dopo)) { return false; }
	TestTrue(TEXT("il blocco d'autore sopravvive alla ricottura"), Dopo->bBlocksMovement);
	TestFalse(TEXT("e resta d'autore"), Dopo->bMovementBlockGenerated);
	TestEqual(TEXT("e non produce segnalazioni: REGOLA 4 non tocca la mano dell'autore"),
		MapEditIssuesOn(Map, Origine), 0);

	return true;
}

/**
 * NEMMENO IL DRY-RUN RICUOCE — la purezza della prova a vuoto, ora che la cancellazione cuoce (#1864).
 *
 * 🔴 **Il buco che questo test chiude, misurato sul test che avrebbe dovuto vederlo.**
 * `DryRunAnswersTheSameAndChangesNothing` prova due `Kind` — `Transition` e `Cell` — e **non**
 * `InteriorWall`: il ramo che ha imparato a ricuocere non era esercitato a vuoto da nessuno.
 *
 * ⚠️ **E il modo ovvio di scrivere questo test sarebbe stato VACUO.** Allestire una cella chiusa da tre
 * muri e asserire che dopo la prova a vuoto resta chiusa non dimostra niente: in dry-run il muro **non
 * viene rimosso**, quindi una ricottura eseguita per sbaglio ricalcolerebbe *lo stesso* risultato e
 * l'asserzione passerebbe comunque. La prima stesura faceva esattamente questo, e la verifica di
 * mutazione l'ha scartata prima che fosse pubblicata.
 *
 * 🔑 **Perche' morda, lo stato di partenza dev'essere STANTIO**: un blocco derivato che la geometria
 * attuale **non** giustifica piu' — cioe' il `StaleGeneratedBlock` della REGOLA 4. Una ricottura
 * eseguita durante la prova a vuoto lo spegnerebbe, e allora la differenza si vede. E' l'unica forma
 * in cui «non ha ricotto» e' un'affermazione falsificabile.
 *
 * ⛔ **Perche' importa davvero**: `URTHexEditorMode::EraseSelection` fa una passata a vuoto su TUTTA la
 * selezione prima di toccare qualunque cosa, per essere tutto-o-niente. Una prova che ricuocesse
 * lascerebbe la mappa modificata anche quando l'operazione viene poi rifiutata — cioe' esattamente lo
 * stato parziale che `bDryRun` esiste per impedire.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapEditDryRunDoesNotRebakeTest,
	"RefactorTactics.Map.Edit.TheDryRunDoesNotRebakeEither",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapEditDryRunDoesNotRebakeTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MapEditMakeMap(2);
	const FRTCellId Home(0, 0, 0);

	const FRTGeometrySegment D30 = MapEditDiameter(ERTTacticalAxis::Deg30);
	const FRTGeometrySegment D90 = MapEditDiameter(ERTTacticalAxis::Deg90);
	const FRTGeometrySegment D150 = MapEditDiameter(ERTTacticalAxis::Deg150);

	// Tre diametri: dodici settori occupati, nessuna posa legale, blocco DERIVATO.
	URTGeometryBakeLibrary::BakeCell(Map, Home, { D30, D90, D150 }, MapEditHexSize);

	// 🔑 LO STATO STANTIO, costruito a mano e non da un gesto: si toglie un muro dall'array **senza**
	// passare da `DeleteElement`, quindi nessuno ricuoce e il blocco derivato resta acceso su una
	// geometria che non lo giustifica piu'. E' lo stato che REGOLA 4 segnala, ed e' l'unico in cui
	// «ha ricotto» e «non ha ricotto» danno risultati diversi.
	const int32 Vittima = Map->InteriorWalls.IndexOfByPredicate(
		[&Home, &D150](const FRTHexInteriorWall& Wall)
		{
			return Wall.Cell == Home && Wall.Segment == D150;
		});
	if (!TestTrue(TEXT("il muro da togliere a mano esiste"), Vittima != INDEX_NONE))
	{
		return false;
	}
	Map->InteriorWalls.RemoveAt(Vittima);

	const FRTHexCellData* Stantia = Map->FindCell(Home);
	if (!TestNotNull(TEXT("la cella esiste"), Stantia)) { return false; }
	if (!TestTrue(TEXT("lo stato di partenza e' STANTIO: blocco derivato senza la sua geometria"),
		Stantia->bBlocksMovement && Stantia->bMovementBlockGenerated))
	{
		return false; // senza questo il test non discriminerebbe: non ci sarebbe niente da spegnere
	}
	if (!TestEqual(TEXT("e ValidateMap lo dice"), MapEditIssuesOn(Map, Home), 1))
	{
		return false;
	}

	// Un ALTRO muro, che e' quello su cui si prova a vuoto.
	if (!TestTrue(TEXT("il muro del gesto prende un nome"), MapEditNameWall(Map, Home, D90, TEXT("W90"))))
	{
		return false;
	}
	const FRTMapElementHandle Handle = FRTMapElementHandle::ForInteriorWall(TEXT("W90"));

	const ERTMapEditOutcome Prova = URTMapEditLibrary::DeleteElement(Map, Handle, /*bDryRun=*/ true);
	TestEqual(TEXT("la prova a vuoto risponde come risponderebbe quella vera"),
		static_cast<int32>(Prova), static_cast<int32>(ERTMapEditOutcome::Applied));

	const FRTHexCellData* Dopo = Map->FindCell(Home);
	if (!TestNotNull(TEXT("la cella esiste ancora"), Dopo)) { return false; }
	TestEqual(TEXT("la prova a vuoto non toglie il muro"), Map->InteriorWalls.Num(), 2);
	TestTrue(TEXT("e NON ricuoce: il blocco stantio e' ancora li'"), Dopo->bBlocksMovement);
	TestTrue(TEXT("con la sua provenienza intatta"), Dopo->bMovementBlockGenerated);
	TestEqual(TEXT("e la segnalazione non e' sparita"), MapEditIssuesOn(Map, Home), 1);

	// --- CONTROLLO POSITIVO: la stessa chiamata SENZA `bDryRun` ricuoce davvero ----------------------
	// Senza questo, una `DeleteElement` che non facesse mai niente supererebbe ogni asserzione qui sopra.
	const ERTMapEditOutcome Vera = URTMapEditLibrary::DeleteElement(Map, Handle);
	TestEqual(TEXT("senza bDryRun l'esito e' lo stesso"),
		static_cast<int32>(Vera), static_cast<int32>(ERTMapEditOutcome::Applied));

	const FRTHexCellData* Finale = Map->FindCell(Home);
	if (!TestNotNull(TEXT("la cella esiste"), Finale)) { return false; }
	TestEqual(TEXT("ma ORA il muro e' sparito"), Map->InteriorWalls.Num(), 1);
	TestFalse(TEXT("e la ricottura ha spento anche il blocco che era rimasto stantio"),
		Finale->bBlocksMovement);
	TestEqual(TEXT("cosi' la cella non produce piu' segnalazioni"), MapEditIssuesOn(Map, Home), 0);

	return true;
}

/**
 * L'AUTORE VINCE ANCHE NEL RAMO CHE BLOCCA, e due gesti reversibili non gli cancellano la scelta (#1864).
 *
 * 🔴 **Il difetto, trovato da una code review del diff che lo rendeva raggiungibile.** `DeriveStandability`
 * dichiara *«l'autore vince»* nel ramo calpestabile, e lo contraddiceva nell'altro: il ramo `!bStandable`
 * scriveva `bMovementBlockGenerated = true` **incondizionatamente**, quindi una cella marcata impraticabile
 * a mano, se la geometria la chiudeva, diventava «derivata» — e il gesto successivo la spegneva.
 *
 * ```text
 * stato d'autore        bBlocksMovement=true   bMovementBlockGenerated=false
 * arriva la geometria   bBlocksMovement=true   bMovementBlockGenerated=TRUE    <- la scelta e' stata presa
 * se ne va la geometria bBlocksMovement=FALSE  bMovementBlockGenerated=false   <- e qui e' sparita
 * ```
 *
 * ⚠️ **Nessun oracolo suonava**: `ValidateMap` REGOLA 4 segnala un blocco derivato che la geometria non
 * giustifica, e qui alla fine non c'e' nessun blocco da chiamare stantio. La cella diventa semplicemente
 * percorribile, e chi l'aveva dichiarata impraticabile non ha modo di accorgersene se non giocandoci.
 *
 * ⏱️ **Era invisibile finche' la cottura la chiamava solo il DISEGNO**: serviva disegnare e poi cancellare
 * sulla stessa cella. Da questa fetta la chiamano anche `MoveInteriorWall` e il ramo `InteriorWall` di
 * `DeleteElement` — gesti che l'autore si aspetta reversibili — ed e' il diff a renderlo raggiungibile in
 * due mosse. Per questo la correzione e il test stanno qui e non in una issue.
 *
 * 🔑 **Due gesti, e il secondo e' il vero giudizio.** Il primo verifica che il blocco non venga
 * *etichettato* come derivato; il secondo, che e' quello che prima falliva, verifica che togliendo la
 * geometria il blocco **resti**. Un test che si fermasse al primo passerebbe anche con la vecchia riga, se
 * guardasse solo `bBlocksMovement`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapEditAuthoredBlockSurvivesBothBranchesTest,
	"RefactorTactics.Map.Edit.AnAuthoredBlockSurvivesEvenWhenGeometryClosesTheCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapEditAuthoredBlockSurvivesBothBranchesTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MapEditMakeMap(2);

	const FRTCellId Home(0, 0, 0);
	const FRTCellId Altrove(1, 0, 0);

	const FRTGeometrySegment D30 = MapEditDiameter(ERTTacticalAxis::Deg30);
	const FRTGeometrySegment D90 = MapEditDiameter(ERTTacticalAxis::Deg90);
	const FRTGeometrySegment D150 = MapEditDiameter(ERTTacticalAxis::Deg150);

	// Due diametri su `Home`: lasciano ancora una posa legale, quindi il blocco che segue e' d'autore e
	// non della geometria.
	URTGeometryBakeLibrary::BakeCell(Map, Home, { D30, D90 }, MapEditHexSize);

	FRTHexCellData Autorata = *Map->FindCell(Home);
	if (!TestFalse(TEXT("due diametri da soli non chiudono la cella"), Autorata.bBlocksMovement))
	{
		return false; // senza questo il `true` qui sotto non sarebbe distinguibile da un blocco derivato
	}
	Autorata.bBlocksMovement = true;
	Autorata.bMovementBlockGenerated = false; // d'autore, ed e' il default
	Map->AddOrUpdateCell(Autorata);

	// Il terzo diametro vive altrove, e lo si sposta DENTRO `Home`: la cella si chiude per geometria.
	URTGeometryBakeLibrary::BakeCell(Map, Altrove, { D150 }, MapEditHexSize);
	if (!TestTrue(TEXT("il muro che arrivera' prende un nome"),
		MapEditNameWall(Map, Altrove, D150, TEXT("W150"))))
	{
		return false;
	}
	const FRTMapElementHandle Handle = FRTMapElementHandle::ForInteriorWall(TEXT("W150"));

	// --- GESTO 1: la geometria chiude la cella che l'autore aveva gia' chiuso -------------------------
	if (!TestEqual(TEXT("il move si applica"),
		static_cast<int32>(URTMapEditLibrary::MoveInteriorWall(Map, Handle, Home, D150)),
		static_cast<int32>(ERTMapEditOutcome::Applied)))
	{
		return false;
	}

	const FRTHexCellData* Chiusa = Map->FindCell(Home);
	if (!TestNotNull(TEXT("la cella esiste"), Chiusa)) { return false; }
	TestTrue(TEXT("la cella resta impraticabile"), Chiusa->bBlocksMovement);
	TestFalse(TEXT("e il blocco resta D'AUTORE: la geometria non se ne appropria"),
		Chiusa->bMovementBlockGenerated);

	// --- GESTO 2: la geometria se ne va, e la scelta d'autore deve sopravviverle ----------------------
	if (!TestEqual(TEXT("la cancellazione si applica"),
		static_cast<int32>(URTMapEditLibrary::DeleteElement(Map, Handle)),
		static_cast<int32>(ERTMapEditOutcome::Applied)))
	{
		return false;
	}

	const FRTHexCellData* Finale = Map->FindCell(Home);
	if (!TestNotNull(TEXT("la cella esiste ancora"), Finale)) { return false; }
	TestTrue(TEXT("IL BLOCCO D'AUTORE E' ANCORA LI': due gesti reversibili non l'hanno cancellato"),
		Finale->bBlocksMovement);
	TestFalse(TEXT("e non e' mai diventato derivato"), Finale->bMovementBlockGenerated);
	TestEqual(TEXT("e non produce segnalazioni: REGOLA 4 non tocca la mano dell'autore"),
		MapEditIssuesOn(Map, Home), 0);

	return true;
}

/**
 * ANCHE UN MOVE DENTRO LA STESSA CELLA RICUOCE — il ramo che la guardia `OldCell != NewCell` salta (#1864).
 *
 * 🔴 **Il buco, trovato da una code review per enumerazione dei chiamanti.** `MoveInteriorWall` ricuoce la
 * cella d'arrivo **sempre**, e quella di partenza solo quando e' diversa. Nessun test asseriva la prima
 * meta': spostare la riga della ricottura d'arrivo **dentro** il blocco `if (!(OldCell == NewCell))`
 * sarebbe passato verde, e con essa la regressione — un muro spostato dentro la propria cella cambia i
 * settori che occupa, quindi puo' aprire o chiudere la posa, e la casella 4 esiste per questo.
 *
 * ⚠️ **`InteriorWallHandleSurvivesTheMove` esercita quel ramo e non lo vede**: sposta un muro da `Home` a
 * `Home` — cioe' e' proprio il caso `OldCell == NewCell` — ma asserisce l'IDENTITA' dell'handle, non la
 * calpestabilita'. Un test che attraversa un ramo non lo presidia: lo attraversa.
 *
 * 🔑 **Lo stato di partenza e' STANTIO, per la stessa ragione del dry-run**: se la cella fosse gia'
 * coerente, ricuocere e non ricuocere darebbero lo stesso risultato e l'asserzione sarebbe vacua. Si toglie
 * quindi un muro **a mano**, senza passare da `DeleteElement`, cosi' nessuno ricuoce e il blocco derivato
 * resta acceso su una geometria che non lo giustifica piu'. Il move che segue deve spegnerlo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapEditMoveWithinTheSameCellRebakesTest,
	"RefactorTactics.Map.Edit.AMoveWithinTheSameCellRebakesItToo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapEditMoveWithinTheSameCellRebakesTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MapEditMakeMap(2);
	const FRTCellId Home(0, 0, 0);

	const FRTGeometrySegment D30 = MapEditDiameter(ERTTacticalAxis::Deg30);
	const FRTGeometrySegment D90 = MapEditDiameter(ERTTacticalAxis::Deg90);
	const FRTGeometrySegment D150 = MapEditDiameter(ERTTacticalAxis::Deg150);

	// Tre diametri: dodici settori occupati, blocco DERIVATO.
	URTGeometryBakeLibrary::BakeCell(Map, Home, { D30, D90, D150 }, MapEditHexSize);

	// Lo stato STANTIO: si toglie `D150` dall'array senza passare da `DeleteElement`, quindi nessuno
	// ricuoce e il blocco derivato sopravvive a una geometria che ora lascia quattro settori liberi.
	const int32 Vittima = Map->InteriorWalls.IndexOfByPredicate(
		[&Home, &D150](const FRTHexInteriorWall& Wall)
		{
			return Wall.Cell == Home && Wall.Segment == D150;
		});
	if (!TestTrue(TEXT("il muro da togliere a mano esiste"), Vittima != INDEX_NONE))
	{
		return false;
	}
	Map->InteriorWalls.RemoveAt(Vittima);

	const FRTHexCellData* Stantia = Map->FindCell(Home);
	if (!TestNotNull(TEXT("la cella esiste"), Stantia)) { return false; }
	if (!TestTrue(TEXT("lo stato di partenza e' STANTIO: blocco derivato senza la sua geometria"),
		Stantia->bBlocksMovement && Stantia->bMovementBlockGenerated))
	{
		return false; // senza questo l'asserzione finale sarebbe vacua: non ci sarebbe niente da spegnere
	}
	if (!TestEqual(TEXT("e ValidateMap lo dice"), MapEditIssuesOn(Map, Home), 1))
	{
		return false;
	}

	// IL GESTO: `D30` si sposta su `D150`, **restando nella stessa cella**. La cella finisce con
	// `D90 + D150`, cioe' otto settori occupati e quattro liberi: torna calpestabile.
	if (!TestTrue(TEXT("il muro prende un nome"), MapEditNameWall(Map, Home, D30, TEXT("W30"))))
	{
		return false;
	}
	const FRTMapElementHandle Handle = FRTMapElementHandle::ForInteriorWall(TEXT("W30"));

	const ERTMapEditOutcome Outcome = URTMapEditLibrary::MoveInteriorWall(Map, Handle, Home, D150);
	if (!TestEqual(TEXT("il move si applica"),
		static_cast<int32>(Outcome), static_cast<int32>(ERTMapEditOutcome::Applied)))
	{
		return false;
	}

	// CONTROPROVA che il gesto sia davvero un move NELLA STESSA CELLA, e non altro: il muro nominato e'
	// ancora su `Home`, e il suo segmento e' cambiato.
	const int32 Spostato = URTMapEditLibrary::ResolveInteriorWall(Map, Handle);
	if (!TestTrue(TEXT("il muro nominato risolve ancora"), Spostato != INDEX_NONE))
	{
		return false;
	}
	TestEqual(TEXT("ed e' rimasto sulla stessa cella"), Map->InteriorWalls[Spostato].Cell, Home);
	TestTrue(TEXT("ma il suo segmento e' cambiato"), Map->InteriorWalls[Spostato].Segment == D150);

	const FRTHexCellData* Dopo = Map->FindCell(Home);
	if (!TestNotNull(TEXT("la cella esiste ancora"), Dopo)) { return false; }
	TestFalse(TEXT("il move ha RICOTTO la cella pur non avendola cambiata"), Dopo->bBlocksMovement);
	TestFalse(TEXT("e la provenienza non resta accesa"), Dopo->bMovementBlockGenerated);
	TestEqual(TEXT("cosi' la segnalazione e' sparita"), MapEditIssuesOn(Map, Home), 0);

	return true;
}

/**
 * OGNI RIFIUTO SI NOMINA, e nessuno cade in un «rifiutata» generico (#1864, casella 8).
 *
 * 🔴 **Il difetto, misurato il 2026-09-24.** La disciplina di `ERTMapEditOutcome` e' dichiarata
 * nell'header: *«un rifiuto e' un valore di ritorno, non un'eccezione ne' un silenzio … o si rifiuta il
 * gesto **dicendo quale regola l'ha fermato**»*. Ma l'unico posto che traduceva quei valori in parole era
 * un `DescribeOutcome` privato, nel namespace anonimo di un **commandlet**
 * (`RTSetCellDoorCommandlet.cpp`), e copriva **4 valori su 7**: gli altri tre cadevano su
 * `default: return TEXT("rifiutata")`, cioe' proprio il silenzio che la regola vieta.
 *
 * ⚠️ E il consumatore piu' importante non lo usava affatto: `URTHexEditorMode::EraseSelection` stampava
 * `static_cast<int32>(Outcome)` — **un numero**. Chi legge il log deve aprire l'enum e contare.
 *
 * 🔑 **Il test non elenca le sette stringhe**, che sarebbe riscrivere la funzione nell'asserzione. Verifica
 * due proprieta' che un `default` generico non puo' soddisfare:
 *
 * ```text
 * 1. ogni valore ha un testo NON VUOTO
 * 2. i testi sono tutti DIVERSI fra loro
 * ```
 *
 * La (2) e' quella che morde: aggiungere un valore all'enum senza il suo `case` lo fa collassare sul
 * testo del `default`, che un altro valore gia' usa — e il test cade. E' la stessa guardia che
 * `RTStartupReportTests` applica a `URTStartupReportLibrary::DescribeOutcome`.
 *
 * ⚠️ **Il conteggio dei valori e' scritto qui e va aggiornato a mano**, ed e' deliberato: `ERTMapEditOutcome`
 * non ha un `Count` sentinella, e dedurlo da un cast farebbe passare in silenzio proprio il caso che questo
 * test esiste per fermare — un valore nuovo senza la sua parola.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapEditDescribeOutcomeNamesEveryValueTest,
	"RefactorTactics.Map.Edit.DescribeOutcomeNamesEveryRefusal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapEditDescribeOutcomeNamesEveryValueTest::RunTest(const FString&)
{
	const TArray<ERTMapEditOutcome> Tutti = {
		ERTMapEditOutcome::Applied,
		ERTMapEditOutcome::RefusedUnresolved,
		ERTMapEditOutcome::RefusedNoSuchCell,
		ERTMapEditOutcome::RefusedOutOfGrammar,
		ERTMapEditOutcome::RefusedWouldCloseEdge,
		ERTMapEditOutcome::RefusedDuplicate,
		ERTMapEditOutcome::RefusedNoNeighbour,
	};

	// GUARDIA ANTI-VACUITA': se l'enum cresce e questo elenco no, il test girerebbe sui soli valori vecchi
	// e non vedrebbe mai il valore nuovo senza parola — cioe' passerebbe proprio quando dovrebbe cadere.
	if (!TestEqual(TEXT("l'elenco copre tutti i valori dichiarati di ERTMapEditOutcome"),
		Tutti.Num(), 7))
	{
		return false;
	}

	TSet<FString> Visti;
	for (const ERTMapEditOutcome Outcome : Tutti)
	{
		const FString Testo = URTMapEditLibrary::DescribeOutcome(Outcome);

		if (!TestFalse(FString::Printf(TEXT("il valore %d ha un testo non vuoto"),
			static_cast<int32>(Outcome)), Testo.IsEmpty()))
		{
			continue;
		}

		// Il cuore: due valori che condividono il testo significano che almeno uno e' caduto in un ramo
		// generico, ed e' il silenzio che la disciplina dell'enum vieta.
		TestFalse(FString::Printf(TEXT("il testo del valore %d non e' gia' di un altro valore ('%s')"),
			static_cast<int32>(Outcome), *Testo), Visti.Contains(Testo));
		Visti.Add(Testo);
	}

	TestEqual(TEXT("e sono tutti distinti"), Visti.Num(), Tutti.Num());

	// CONTROPROVA che il testo sia davvero una RAGIONE e non il nome del simbolo: `Applied` e i rifiuti
	// devono distinguersi, altrimenti un log direbbe «RefusedDuplicate» e chi legge e' tornato all'enum.
	TestFalse(TEXT("il testo non e' il nome del simbolo"),
		URTMapEditLibrary::DescribeOutcome(ERTMapEditOutcome::RefusedDuplicate)
			.Contains(TEXT("RefusedDuplicate")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
