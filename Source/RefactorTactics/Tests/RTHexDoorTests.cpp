#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexDoorLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"
#include "Pathfinding/RTHexPathLibrary.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Porte (CP 9.3): una topologia che cambia a partita in corso senza produrre percorsi fantasma.
 *
 * La decisione che questi test fissano e' che una porta e' un BORDO e non un arco: la si legge da
 * `URTHexCoverLibrary::BlocksTraversal`, l'unica funzione che vista, grafo e combat interrogano, e la si muta
 * da `URTHexDoorLibrary::SetDoorState`, l'unico ingresso che incrementa la revisione. Vedi
 * docs/gameplay/spec-porte-cp93.md.
 */
namespace
{
	/** Esagono pieno di raggio N sul layer 0. Nome distinto per file (unity build). */
	URTHexMapAsset* MakeDoorMap(int32 Radius)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();
		return M;
	}

	/** Porta sul bordo indicato della cella (la cella deve esistere). Scrive UNA faccia sola: e' il caso da
	 *  difendere — la regola non deve dipendere da quale delle due celle ha ricevuto il dato. */
	void PutDoor(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge, ERTHexDoorState State,
		int32 DoorId = INDEX_NONE)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		Data.Doors.Add(FRTHexDoor(Edge, State, DoorId));
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	/** Muro alto sul bordo indicato (per il caso muro + porta sullo stesso bordo). */
	void PutHighWall(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		Data.Covers.Add(FRTHexCover(Edge, ERTHexCoverType::High,
			FRTHexCover::DefaultIntegrity(ERTHexCoverType::High)));
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	/** Vero se il grafo di traversata offre `To` fra i vicini di `From`. */
	bool DoorGraphHasStep(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To)
	{
		for (const TPair<FRTCellId, int32>& Step : URTHexPathLibrary::GraphNeighbors(Map, From))
		{
			if (Step.Key == To) { return true; }
		}
		return false;
	}

	FRTHexSimUnit DoorUnit(int32 UnitId, const FRTCellId& Cell, int32 MoveBudget)
	{
		return FRTHexSimUnit(UnitId, Cell, MoveBudget, /*bAlive=*/ true);
	}
}

/**
 * La revisione e' il segnale con cui snapshot e percorsi si accorgono che il grafo non e' piu' quello di prima
 * (CP 9.1 l'ha messa li' per questo). Ogni cambio di stato la incrementa — e un PORTONE, che tocca piu' celle,
 * la incrementa UNA volta sola: e' un evento, non tre.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDoorRevisionTest,
	"RefactorTactics.Structures.Door.StateChangeBumpsRevision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDoorRevisionTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeDoorMap(2);
	const FRTCellId A(0, 0);
	const FRTCellId B(1, 0);
	PutDoor(Map, A, ERTHexDirection::E, ERTHexDoorState::Open);

	const int32 Before = Map->Revision;
	const TArray<FRTDoorChange> Closed = URTHexDoorLibrary::SetDoorState(Map, A, B, ERTHexDoorState::Closed);
	TestEqual(TEXT("un bordo cambiato"), Closed.Num(), 1);
	TestEqual(TEXT("la revisione e' salita di uno"), Map->Revision, Before + 1);

	// Riapplicare lo STESSO stato non e' un cambio: se la revisione salisse comunque, ogni snapshot risulterebbe
	// stale a ogni turno e l'invalidazione non direbbe piu' niente.
	const int32 AfterClose = Map->Revision;
	const TArray<FRTDoorChange> Again = URTHexDoorLibrary::SetDoorState(Map, A, B, ERTHexDoorState::Closed);
	TestEqual(TEXT("nessun cambio riportato"), Again.Num(), 0);
	TestEqual(TEXT("la revisione non si muove"), Map->Revision, AfterClose);

	// Portone largo tre bordi: un comando, una revisione.
	URTHexMapAsset* Wide = MakeDoorMap(3);
	PutDoor(Wide, FRTCellId(0, 0), ERTHexDirection::E, ERTHexDoorState::Open, /*DoorId*/ 7);
	PutDoor(Wide, FRTCellId(0, 1), ERTHexDirection::E, ERTHexDoorState::Open, /*DoorId*/ 7);
	PutDoor(Wide, FRTCellId(0, -1), ERTHexDirection::E, ERTHexDoorState::Open, /*DoorId*/ 7);

	const int32 WideBefore = Wide->Revision;
	const TArray<FRTDoorChange> Portone =
		URTHexDoorLibrary::SetDoorState(Wide, FRTCellId(0, 0), FRTCellId(1, 0), ERTHexDoorState::Closed);
	TestEqual(TEXT("tre bordi cambiati insieme"), Portone.Num(), 3);
	TestEqual(TEXT("ma una sola revisione"), Wide->Revision, WideBefore + 1);
	return true;
}

/**
 * Il percorso e' la cosa che una porta chiusa deve invalidare. Due prove nello stesso test perche' sono la
 * stessa domanda: lo snapshot preso PRIMA deve risultare stale, e il grafo interrogato DOPO non deve piu'
 * offrire quel passo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDoorInvalidatesPathTest,
	"RefactorTactics.Structures.Door.InvalidatesPathCache",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDoorInvalidatesPathTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeDoorMap(3);
	const FRTCellId A(0, 0);
	const FRTCellId B(1, 0);
	PutDoor(Map, A, ERTHexDirection::E, ERTHexDoorState::Open);

	TArray<FRTHexSimUnit> Units;
	Units.Add(DoorUnit(0, A, /*MoveBudget*/ 6));
	const FRTHexSnapshot Fresh = URTHexSimLibrary::MakeSnapshot(Map, Units);
	TestFalse(TEXT("lo snapshot nasce valido"), URTHexSimLibrary::IsSnapshotStale(Fresh));

	// Con la porta aperta il passo esiste e il percorso lo usa.
	TestTrue(TEXT("porta aperta: il grafo offre il passo"), DoorGraphHasStep(Map, A, B));
	const FRTHexPathResult ThroughOpen = URTHexPathLibrary::FindPath(Map, A, B, /*MaxCost*/ 0);
	TestTrue(TEXT("porta aperta: si arriva"), ThroughOpen.Status == ERTHexPathStatus::Success);

	URTHexDoorLibrary::SetDoorState(Map, A, B, ERTHexDoorState::Closed);

	TestTrue(TEXT("lo snapshot di prima e' stale"), URTHexSimLibrary::IsSnapshotStale(Fresh));
	TestFalse(TEXT("porta chiusa: il grafo non offre piu' il passo"), DoorGraphHasStep(Map, A, B));

	// Ricalcolato sulla mappa aggiornata il percorso c'e' ancora, ma GIRA INTORNO: la cella resta raggiungibile,
	// e' il bordo a non essere piu' attraversabile. Una porta non toglie celle dalla mappa.
	const FRTHexPathResult Around = URTHexPathLibrary::FindPath(Map, A, B, /*MaxCost*/ 0);
	TestTrue(TEXT("la cella oltre la porta resta raggiungibile"), Around.Status == ERTHexPathStatus::Success);
	TestTrue(TEXT("ma non piu' con un passo solo"), Around.Path.Num() > 2);
	return true;
}

/**
 * La DoD chiede che una porta chiusa blocchi anche la vista. E' la ragione per cui la porta e' un bordo: la
 * LOS interroga `BlocksTraversal` e non sa nulla di archi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDoorBlocksSightTest,
	"RefactorTactics.Structures.Door.BlocksLineOfSight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDoorBlocksSightTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeDoorMap(4);
	const FRTCellId Shooter(0, 0);
	const FRTCellId Far(3, 0);
	// Porta sul bordo (1,0)<->(2,0), cioe' a meta' della linea di tiro.
	PutDoor(Map, FRTCellId(1, 0), ERTHexDirection::E, ERTHexDoorState::Open);

	TestTrue(TEXT("porta aperta: si vede"), URTHexVisionLibrary::HasLineOfSight(Map, Shooter, Far));

	URTHexDoorLibrary::SetDoorState(Map, FRTCellId(1, 0), FRTCellId(2, 0), ERTHexDoorState::Closed);
	TestFalse(TEXT("porta chiusa: non si vede"), URTHexVisionLibrary::HasLineOfSight(Map, Shooter, Far));

	// `Locked` e' chiusa a tutti gli effetti: la differenza e' chi puo' riaprirla, non cosa lascia passare.
	URTHexDoorLibrary::SetDoorState(Map, FRTCellId(1, 0), FRTCellId(2, 0), ERTHexDoorState::Locked);
	TestFalse(TEXT("porta bloccata: non si vede"), URTHexVisionLibrary::HasLineOfSight(Map, Shooter, Far));

	URTHexDoorLibrary::SetDoorState(Map, FRTCellId(1, 0), FRTCellId(2, 0), ERTHexDoorState::Closed);
	URTHexDoorLibrary::SetDoorState(Map, FRTCellId(1, 0), FRTCellId(2, 0), ERTHexDoorState::Open);
	TestTrue(TEXT("riaperta: si vede di nuovo"), URTHexVisionLibrary::HasLineOfSight(Map, Shooter, Far));
	return true;
}

/**
 * Il bordo ha DUE facce e la porta puo' essere dichiarata da una sola: la regola non deve dipendere da quale
 * delle due celle ha ricevuto il dato, altrimenti la stessa barriera fermerebbe chi arriva da una parte e non
 * chi arriva dall'altra.
 *
 * Questo test non c'era: l'ha reso necessario la VERIFICA DI MUTAZIONE. Togliendo dalla lettura la faccia
 * `To` — cioe' facendo guardare una faccia sola — non cadeva nessuno dei dodici test, perche' tutti
 * dichiaravano la porta dal lato da cui poi la interrogavano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDoorBothFacesTest,
	"RefactorTactics.Structures.Door.ReadsBothFaces",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDoorBothFacesTest::RunTest(const FString&)
{
	// Dichiarata SOLO dalla faccia (1,0)/E.
	URTHexMapAsset* Map = MakeDoorMap(2);
	PutDoor(Map, FRTCellId(1, 0), ERTHexDirection::E, ERTHexDoorState::Closed);

	TestTrue(TEXT("letta dalla faccia che la dichiara"),
		URTHexCoverLibrary::BlocksTraversal(Map, FRTCellId(1, 0), FRTCellId(2, 0)));
	TestTrue(TEXT("letta dalla faccia OPPOSTA, che non la dichiara"),
		URTHexCoverLibrary::BlocksTraversal(Map, FRTCellId(2, 0), FRTCellId(1, 0)));
	TestFalse(TEXT("il grafo non offre il passo nemmeno tornando indietro"),
		DoorGraphHasStep(Map, FRTCellId(2, 0), FRTCellId(1, 0)));

	// E il comando vale nei due versi: chiudere dal lato che NON la dichiara scrive sulla faccia che la
	// dichiara — altrimenti un'unita' potrebbe chiudersi la porta alle spalle solo stando dal lato giusto.
	URTHexMapAsset* FromBehind = MakeDoorMap(2);
	PutDoor(FromBehind, FRTCellId(1, 0), ERTHexDirection::E, ERTHexDoorState::Open);
	TestEqual(TEXT("si commuta anche comandandola dal lato opposto"),
		URTHexDoorLibrary::SetDoorState(FromBehind, FRTCellId(2, 0), FRTCellId(1, 0),
			ERTHexDoorState::Closed).Num(), 1);
	TestTrue(TEXT("e il bordo risulta chiuso da entrambi i lati"),
		URTHexCoverLibrary::BlocksTraversal(FromBehind, FRTCellId(1, 0), FRTCellId(2, 0))
		&& URTHexCoverLibrary::BlocksTraversal(FromBehind, FRTCellId(2, 0), FRTCellId(1, 0)));
	return true;
}

/**
 * Un portone largo si commuta come una porta sola. Il gruppo non deve essere rettilineo: qui i tre bordi
 * stanno su direzioni diverse, ed e' comunque una porta sola.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDoorGroupTest,
	"RefactorTactics.Structures.Door.GroupClosesTogether",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDoorGroupTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeDoorMap(3);
	const FRTCellId Center(0, 0);
	PutDoor(Map, Center, ERTHexDirection::E, ERTHexDoorState::Open, /*DoorId*/ 3);
	PutDoor(Map, Center, ERTHexDirection::NE, ERTHexDoorState::Open, /*DoorId*/ 3);
	PutDoor(Map, Center, ERTHexDirection::NW, ERTHexDoorState::Open, /*DoorId*/ 3);
	// Una quarta porta SENZA gruppo: non deve muoversi insieme alle altre.
	PutDoor(Map, Center, ERTHexDirection::W, ERTHexDoorState::Open);

	const TArray<FRTDoorChange> Changed =
		URTHexDoorLibrary::SetDoorState(Map, Center, FRTCellId(1, 0), ERTHexDoorState::Closed);
	TestEqual(TEXT("i tre bordi del gruppo"), Changed.Num(), 3);

	TestTrue(TEXT("E chiuso"), URTHexCoverLibrary::BlocksTraversal(Map, Center, FRTCellId(1, 0)));
	TestTrue(TEXT("NE chiuso"), URTHexCoverLibrary::BlocksTraversal(Map, Center, FRTCellId(1, -1)));
	TestTrue(TEXT("NW chiuso"), URTHexCoverLibrary::BlocksTraversal(Map, Center, FRTCellId(0, -1)));
	TestFalse(TEXT("la porta senza gruppo resta aperta"),
		URTHexCoverLibrary::BlocksTraversal(Map, Center, FRTCellId(-1, 0)));

	// L'ordine canonico delle voci non dipende dall'ordine in cui i bordi stanno nell'array. L'indice si
	// convalida prima di usarlo: un test che sbaglia deve FALLIRE, non far cadere il runner e con lui i test
	// che vengono dopo.
	for (int32 I = 1; I < Changed.Num(); ++I)
	{
		TestFalse(TEXT("le voci escono in ordine canonico"),
			URTHexLibrary::StableLess(Changed[I].Toward, Changed[I - 1].Toward));
	}
	return true;
}

/**
 * L'OR fra muro e porta e' RESTRITTIVO. Un livello che disegna una porta dentro un muro pieno e' un dato
 * incoerente — `ValidateMap` lo dice — ma a runtime non deve mai produrre un varco a sorpresa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDoorWallWinsTest,
	"RefactorTactics.Structures.Door.WallOverridesOpenDoor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDoorWallWinsTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeDoorMap(2);
	const FRTCellId A(0, 0);
	const FRTCellId B(1, 0);
	PutHighWall(Map, A, ERTHexDirection::E);
	PutDoor(Map, A, ERTHexDirection::E, ERTHexDoorState::Open);

	TestTrue(TEXT("il muro decide: il bordo resta chiuso"), URTHexCoverLibrary::BlocksTraversal(Map, A, B));
	TestFalse(TEXT("e il grafo non offre il passo"), DoorGraphHasStep(Map, A, B));
	TestTrue(TEXT("il dato incoerente e' segnalato"), Map->ValidateMap().Num() > 0);
	return true;
}

/**
 * Invariante #3 applicata alle porte: due unita' che nello stesso turno una apre e una chiude devono dare lo
 * stesso esito in qualunque ordine. Vince lo stato piu' RESTRITTIVO.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDoorOpsOrderTest,
	"RefactorTactics.Structures.Door.OpsOrderIndependent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDoorOpsOrderTest::RunTest(const FString&)
{
	const FRTCellId A(0, 0);
	const FRTCellId B(1, 0);

	URTHexMapAsset* First = MakeDoorMap(2);
	PutDoor(First, A, ERTHexDirection::E, ERTHexDoorState::Open);
	TArray<FRTDoorOp> OpenThenClose;
	OpenThenClose.Add(FRTDoorOp(A, B, ERTHexDoorState::Open));
	OpenThenClose.Add(FRTDoorOp(A, B, ERTHexDoorState::Closed));
	URTHexDoorLibrary::ApplyDoorOps(First, OpenThenClose);

	URTHexMapAsset* Second = MakeDoorMap(2);
	PutDoor(Second, A, ERTHexDirection::E, ERTHexDoorState::Open);
	TArray<FRTDoorOp> CloseThenOpen;
	CloseThenOpen.Add(FRTDoorOp(A, B, ERTHexDoorState::Closed));
	CloseThenOpen.Add(FRTDoorOp(A, B, ERTHexDoorState::Open));
	URTHexDoorLibrary::ApplyDoorOps(Second, CloseThenOpen);

	TestTrue(TEXT("prima aperta poi chiusa: chiusa"), URTHexCoverLibrary::BlocksTraversal(First, A, B));
	TestTrue(TEXT("prima chiusa poi aperta: chiusa lo stesso"),
		URTHexCoverLibrary::BlocksTraversal(Second, A, B));
	TestEqual(TEXT("stesso hash a ordini invertiti"), First->ComputeHash(), Second->ComputeHash());
	return true;
}

/**
 * Le due transizioni che l'API deve RIFIUTARE, perche' sono regole di gioco e non stati del dato: una porta
 * sfondata non si richiude, e una bloccata non si apre da sola (serve l'apertura autorizzata di CP 10.1).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDoorTerminalStateTest,
	"RefactorTactics.Structures.Door.DestroyedStaysOpen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDoorTerminalStateTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeDoorMap(2);
	const FRTCellId A(0, 0);
	const FRTCellId B(1, 0);
	PutDoor(Map, A, ERTHexDirection::E, ERTHexDoorState::Destroyed);

	const int32 Before = Map->Revision;
	const TArray<FRTDoorChange> Rejected = URTHexDoorLibrary::SetDoorState(Map, A, B, ERTHexDoorState::Closed);
	TestEqual(TEXT("una porta sfondata non si richiude"), Rejected.Num(), 0);
	TestEqual(TEXT("e la revisione non si muove"), Map->Revision, Before);
	TestFalse(TEXT("il passo resta aperto"), URTHexCoverLibrary::BlocksTraversal(Map, A, B));

	// `Locked` -> `Open` rifiutata; `Locked` -> `Closed` ammessa (togliere il blocco non e' aprire).
	URTHexMapAsset* Locked = MakeDoorMap(2);
	PutDoor(Locked, A, ERTHexDirection::E, ERTHexDoorState::Locked);
	TestEqual(TEXT("una porta bloccata non si apre da sola"),
		URTHexDoorLibrary::SetDoorState(Locked, A, B, ERTHexDoorState::Open).Num(), 0);
	TestTrue(TEXT("resta chiusa"), URTHexCoverLibrary::BlocksTraversal(Locked, A, B));
	TestEqual(TEXT("ma si puo' sbloccare"),
		URTHexDoorLibrary::SetDoorState(Locked, A, B, ERTHexDoorState::Closed).Num(), 1);
	return true;
}

/**
 * Il percorso gia' pianificato incontra una porta che nel frattempo si e' chiusa: si FERMA all'ultima cella
 * valida (`Fallback.Stop`), non lo attraversa e non viene annullato. E' il difetto che CP 9.3 esiste per
 * impedire, isolato dal turno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDoorTruncatesPathTest,
	"RefactorTactics.Structures.Door.TruncatesPlannedPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDoorTruncatesPathTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeDoorMap(4);
	PutDoor(Map, FRTCellId(1, 0), ERTHexDirection::E, ERTHexDoorState::Open);

	TArray<FRTHexSimUnit> Units;
	Units.Add(DoorUnit(0, FRTCellId(0, 0), /*MoveBudget*/ 6));

	const TArray<FRTCellId> Planned = { FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0), FRTCellId(3, 0) };

	// Porta aperta: il percorso e' valido per intero (nessuna troncatura a vuoto).
	const FRTHexSnapshot Open = URTHexSimLibrary::MakeSnapshot(Map, Units);
	TestEqual(TEXT("porta aperta: percorso intatto"),
		URTHexSimLibrary::TruncatePathToTopology(Open, Planned).Num(), 4);

	URTHexDoorLibrary::SetDoorState(Map, FRTCellId(1, 0), FRTCellId(2, 0), ERTHexDoorState::Closed);
	const FRTHexSnapshot Shut = URTHexSimLibrary::MakeSnapshot(Map, Units);
	const TArray<FRTCellId> Stopped = URTHexSimLibrary::TruncatePathToTopology(Shut, Planned);

	TestEqual(TEXT("si ferma prima della porta"), Stopped.Num(), 2);
	TestTrue(TEXT("l'ultima cella e' quella prima del varco"), Stopped.Last() == FRTCellId(1, 0));

	// Fail-open sul dato non verificabile: senza mappa il piano resta quello che era.
	FRTHexSnapshot NoMap;
	TestEqual(TEXT("senza mappa il percorso non si tocca"),
		URTHexSimLibrary::TruncatePathToTopology(NoMap, Planned).Num(), 4);
	return true;
}

/**
 * L'hash e' cio' su cui `IsSnapshotStale` si basa insieme alla revisione: deve cambiare con lo stato della
 * porta e NON dipendere dall'ordine in cui le voci stanno nell'array. Una mappa senza porte hasha come prima.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDoorHashTest,
	"RefactorTactics.HexMap.DoorHashDeterminism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDoorHashTest::RunTest(const FString&)
{
	URTHexMapAsset* Bare = MakeDoorMap(2);
	const uint32 BareHash = Bare->ComputeHash();

	URTHexMapAsset* WithDoor = MakeDoorMap(2);
	PutDoor(WithDoor, FRTCellId(0, 0), ERTHexDirection::E, ERTHexDoorState::Closed);
	TestTrue(TEXT("una porta cambia l'hash"), WithDoor->ComputeHash() != BareHash);

	URTHexMapAsset* SameOpen = MakeDoorMap(2);
	PutDoor(SameOpen, FRTCellId(0, 0), ERTHexDirection::E, ERTHexDoorState::Open);
	TestTrue(TEXT("lo STATO cambia l'hash"), SameOpen->ComputeHash() != WithDoor->ComputeHash());

	// Stesse due porte, inserite in ordine opposto: l'ordine dell'array lo decide chi edita l'asset.
	URTHexMapAsset* OrderA = MakeDoorMap(2);
	PutDoor(OrderA, FRTCellId(0, 0), ERTHexDirection::E, ERTHexDoorState::Closed);
	PutDoor(OrderA, FRTCellId(0, 0), ERTHexDirection::W, ERTHexDoorState::Open);
	URTHexMapAsset* OrderB = MakeDoorMap(2);
	PutDoor(OrderB, FRTCellId(0, 0), ERTHexDirection::W, ERTHexDoorState::Open);
	PutDoor(OrderB, FRTCellId(0, 0), ERTHexDirection::E, ERTHexDoorState::Closed);
	TestEqual(TEXT("l'ordine dell'array non conta"), OrderA->ComputeHash(), OrderB->ComputeHash());

	// Il gruppo e' dato autorevole quanto lo stato: due portoni diversi non devono hashare uguale.
	URTHexMapAsset* Grouped = MakeDoorMap(2);
	PutDoor(Grouped, FRTCellId(0, 0), ERTHexDirection::E, ERTHexDoorState::Closed, /*DoorId*/ 4);
	TestTrue(TEXT("il gruppo entra nell'hash"), Grouped->ComputeHash() != WithDoor->ComputeHash());
	return true;
}

/**
 * Le porte si scrivono a mano nell'editor delle proprieta': qui si intercettano gli stati che nessuna regola
 * sa risolvere, con lo stesso criterio usato per le coperture in CP 9.1.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDoorValidationTest,
	"RefactorTactics.HexMap.DoorValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDoorValidationTest::RunTest(const FString&)
{
	URTHexMapAsset* Clean = MakeDoorMap(2);
	PutDoor(Clean, FRTCellId(0, 0), ERTHexDirection::E, ERTHexDoorState::Closed);
	TestEqual(TEXT("una porta ben posata non genera errori"), Clean->ValidateMap().Num(), 0);

	URTHexMapAsset* Overlapped = MakeDoorMap(2);
	PutDoor(Overlapped, FRTCellId(0, 0), ERTHexDirection::E, ERTHexDoorState::Closed);
	PutDoor(Overlapped, FRTCellId(0, 0), ERTHexDirection::E, ERTHexDoorState::Open);
	TestTrue(TEXT("due porte sullo stesso bordo sono un dato incoerente"),
		Overlapped->ValidateMap().Num() > 0);

	URTHexMapAsset* InsideWall = MakeDoorMap(2);
	PutHighWall(InsideWall, FRTCellId(0, 0), ERTHexDirection::E);
	PutDoor(InsideWall, FRTCellId(0, 0), ERTHexDirection::E, ERTHexDoorState::Open);
	TestTrue(TEXT("una porta dentro un muro pieno non si aprirebbe mai"),
		InsideWall->ValidateMap().Num() > 0);
	return true;
}

/**
 * `Doors` entra nel formato: la versione sale a 4 e una mappa scritta con la v3 deve sopravvivere senza
 * perdere niente. La migrazione non converte dati — il campo nuovo nasce vuoto — quindi cio' che va
 * dimostrato e' che non tocca quelli vecchi, coperture comprese.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDoorMigrationTest,
	"RefactorTactics.HexMap.DoorFormatMigration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDoorMigrationTest::RunTest(const FString&)
{
	URTHexMapAsset* Legacy = NewObject<URTHexMapAsset>();
	FRTHexCellData Rough(FRTCellId(0, 0));
	Rough.Height = 2;
	Rough.MoveCost = 3;
	Rough.Surface = ERTHexSurface::Rough;
	Rough.Covers.Add(FRTHexCover(ERTHexDirection::NE, ERTHexCoverType::Low, 30));
	Legacy->AddOrUpdateCell(Rough);
	Legacy->AddOrUpdateCell(FRTHexCellData(FRTCellId(1, 0)));
	Legacy->AddOrUpdateCell(FRTHexCellData(FRTCellId(0, 0, 1)));
	Legacy->AddTransition(FRTCellId(0, 0, 0), FRTCellId(0, 0, 1), 2, ERTHexTransitionKind::Stair);
	Legacy->FormatVersion = 3;

	Legacy->MigrateToCurrentFormat();

	TestEqual(TEXT("versione portata alla corrente"), Legacy->FormatVersion,
		URTHexMapAsset::CurrentFormatVersion);
	// Il numero e' pinnato di proposito: un bump di formato deve far cadere un test, non passare inosservato.
	// v7 (#619) aggiunge il sovrapprezzo di occupazione; nessun dato precedente cambia significato.
	TestEqual(TEXT("la versione corrente e' la 8"), URTHexMapAsset::CurrentFormatVersion, 8);
	TestEqual(TEXT("nessuna cella persa"), Legacy->NumCells(), 3);
	TestEqual(TEXT("nessuna transizione persa"), Legacy->Transitions.Num(), 2); // bidirezionale

	const FRTHexCellData* Migrated = Legacy->FindCell(FRTCellId(0, 0, 0));
	TestTrue(TEXT("la cella esiste ancora"), Migrated != nullptr);
	if (Migrated)
	{
		TestEqual(TEXT("Height preservata"), Migrated->Height, 2);
		TestEqual(TEXT("MoveCost preservato"), Migrated->MoveCost, 3);
		TestEqual(TEXT("copertura preservata"), Migrated->Covers.Num(), 1);
		TestEqual(TEXT("nessuna porta inventata"), Migrated->Doors.Num(), 0);
	}
	TestEqual(TEXT("mappa migrata valida"), Legacy->ValidateMap().Num(), 0);
	return true;
}


/**
 * Il cambio di stato di una porta dice CHI l'ha voluto (#405).
 *
 * Stessa forma del danno alle strutture: l'intento conosce l'attore, e senza propagarlo la voce di TurnLog
 * «porta aperta» resterebbe l'unica azione deliberata del turno a dichiarare «nessuna unita'».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDoorChangeCarriesActorTest,
	"RefactorTactics.Door.ChangeCarriesActor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDoorChangeCarriesActorTest::RunTest(const FString&)
{
	const FRTCellId A(0, 0, 0);
	const FRTCellId B(1, 0, 0);
	URTHexMapAsset* Map = MakeDoorMap(2);
	PutDoor(Map, A, ERTHexDirection::E, ERTHexDoorState::Closed);

	TArray<FRTDoorOp> Ops;
	Ops.Add(FRTDoorOp(A, B, ERTHexDoorState::Open, /*ActorId*/ 3));

	const TArray<FRTDoorChange> Changes = URTHexDoorLibrary::ApplyDoorOps(Map, Ops);
	if (!TestEqual(TEXT("un cambio di porta"), Changes.Num(), 1)) { return false; }
	TestEqual(TEXT("il cambio porta l'attore che l'ha chiesto"), Changes[0].ActorId, 3);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
