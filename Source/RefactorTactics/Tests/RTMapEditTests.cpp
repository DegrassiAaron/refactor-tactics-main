#include "Misc/AutomationTest.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Map/RTHexMapCustomVersion.h"
#include "Map/RTCellId.h"
#include "Map/RTGeometryBake.h"
#include "Map/RTGeometryGrammar.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
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

#endif // WITH_DEV_AUTOMATION_TESTS
