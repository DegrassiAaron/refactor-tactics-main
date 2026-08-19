// `ReservePlannedRoute`: una rotta prenotata è occupata per le ALTRE unità e libera per la propria (#1088).
//
// È il mattone su cui poggia `PlanTeam`, ed è testato qui **da solo** perché la sua proprietà non dipende
// dall'utility del bot: qualunque cella scelga, dopo la prenotazione quella rotta non deve essere
// disponibile a nessun altro. L'effetto sul difetto — le 24 contese fra compagni misurate da #1088 — si
// verifica invece nel ciclo che le ha prodotte, in `RTBotStalemateProbeTests.cpp`, perché lì la
// pianificazione passa dal filtro di percezione ed è quella la condizione in cui lo stallo si forma.
//
// 🔴 **La prima stesura di questo file sbagliava proprio qui**: costruiva due compagne con conoscenza
// perfetta dei nemici e dava per scontato che scegliessero la stessa cella. Non è così — misurato:
// `u1 -> (0,-3)` e `u2 -> (-1,4)`, destinazioni opposte — perché senza il filtro di percezione il bot
// valuta la minaccia in modo diverso. Il test l'ha detto invece di restare verde a vuoto, e solo perché
// asseriva la propria premessa. Vedi il controllo di non-vacuità nel probe.

#include "Misc/AutomationTest.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Bot/RTHexBotLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotReserveRouteTest,
	"RefactorTactics.Bot.ReservedRouteBlocksTeammatesOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTBotReserveRouteTest::RunTest(const FString&)
{
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(GetTransientPackage());
	if (!TestNotNull(TEXT("arena di prova generata"), Arena)) { return false; }

	const FRTCellId CellA(-2, 0, 0);
	const FRTCellId CellB(-2, 1, 0);

	TArray<FRTHexSimUnit> SimUnits;
	SimUnits.Add(FRTHexSimUnit(1, CellA, /*budget*/ 5));
	SimUnits.Add(FRTHexSimUnit(2, CellB, /*budget*/ 5));
	const FRTHexSnapshot Snapshot = URTHexSimLibrary::MakeSnapshot(Arena, SimUnits);

	// Una destinazione qualsiasi ma RAGGIUNGIBILE: la rotta dev'essere reale, altrimenti il test verificherebbe
	// la prenotazione di un percorso vuoto — che è vera per costruzione e non dice niente.
	const FRTCellId Dest(0, 0, 0);
	const TArray<FRTCellId> Route = URTHexSimLibrary::FindPathForUnit(Snapshot, 1, Dest).Path;
	if (!TestTrue(TEXT("premessa: la rotta esiste ed è di almeno due celle"), Route.Num() >= 2)) { return false; }

	FString Printed;
	for (const FRTCellId& C : Route) { Printed += (Printed.IsEmpty() ? TEXT("") : TEXT(" -> ")) + C.ToString(); }
	AddInfo(FString::Printf(TEXT("rotta di u1: %s"), *Printed));

	FRTHexSnapshot Reserved = Snapshot;
	URTHexBotLibrary::ReservePlannedRoute(Reserved, /*UnitId=*/ 1, Dest);

	// --- 1. Ogni cella della rotta risulta occupata, e dall'unità che l'ha prenotata.
	int32 Missing = 0;
	int32 WrongOwner = 0;
	for (const FRTCellId& Cell : Route)
	{
		const int32* Owner = Reserved.Occupancy.Find(Cell);
		if (!Owner) { ++Missing; }
		else if (*Owner != 1) { ++WrongOwner; }
	}
	TestEqual(TEXT("nessuna cella della rotta è rimasta libera"), Missing, 0);
	TestEqual(TEXT("e nessuna risulta di un'altra unità"), WrongOwner, 0);

	// --- 2. Per la COMPAGNA quelle celle sono occupate: è il punto della prenotazione.
	//
	// Si misura sulle celle raggiungibili, che è la funzione da cui il bot genera le candidate: se una cella
	// prenotata comparisse ancora fra le raggiungibili di `u2`, `BuildCandidates` potrebbe riproporla e la
	// prenotazione non servirebbe a niente.
	const TArray<FRTHexReachableCell> ReachableBefore = URTHexSimLibrary::ReachableCells(Snapshot, /*UnitId=*/ 2);
	const TArray<FRTHexReachableCell> ReachableAfter = URTHexSimLibrary::ReachableCells(Reserved, /*UnitId=*/ 2);

	auto Contains = [](const TArray<FRTHexReachableCell>& Cells, const FRTCellId& Target)
	{
		for (const FRTHexReachableCell& C : Cells) { if (C.Cell == Target) { return true; } }
		return false;
	};

	// ⚠️ Il controllo di non-vacuità: se `u2` non potesse già raggiungere nessuna cella della rotta, «dopo non
	// le raggiunge» sarebbe vero senza che la prenotazione abbia fatto niente.
	int32 ReachableOnRouteBefore = 0;
	int32 ReachableOnRouteAfter = 0;
	for (int32 I = 1; I < Route.Num(); ++I)     // dalla 1: la cella di partenza di u1 era già occupata
	{
		if (Contains(ReachableBefore, Route[I])) { ++ReachableOnRouteBefore; }
		if (Contains(ReachableAfter, Route[I])) { ++ReachableOnRouteAfter; }
	}
	AddInfo(FString::Printf(TEXT("celle della rotta raggiungibili da u2: prima %d, dopo %d"),
		ReachableOnRouteBefore, ReachableOnRouteAfter));

	TestTrue(TEXT("premessa: prima della prenotazione u2 poteva entrare nella rotta di u1"),
		ReachableOnRouteBefore > 0);
	TestEqual(TEXT("dopo la prenotazione, nessuna cella della rotta è raggiungibile da u2"),
		ReachableOnRouteAfter, 0);

	// --- 3. Ma u1 la sua rotta la percorre ancora: `ReachableCells` non blocca un'unità con se stessa
	// (`*Occupant != UnitId`), ed è la ragione per cui si prenota con l'id del prenotante e non con un
	// marcatore generico. Con un id qualsiasi, l'unità si sbarrerebbe la strada da sola.
	const TArray<FRTCellId> RouteAfter = URTHexSimLibrary::FindPathForUnit(Reserved, /*UnitId=*/ 1, Dest).Path;
	TestEqual(TEXT("u1 percorre ancora la propria rotta, invariata"), RouteAfter.Num(), Route.Num());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
