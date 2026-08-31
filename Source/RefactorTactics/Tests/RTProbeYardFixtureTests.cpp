#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * `ProbeYard` — la fixture su cui la SONDA DI MOVIMENTO (#711) si guarda in seduta.
 *
 * 🔑 **Perche' una fixture e non «allestisci a mano».** La voce `PIE-HEX-MOVEMENT-PROBE` chiede una mappa
 * con una superficie costosa, un blocco e una zona irraggiungibile: senza quelle tre condizioni la sonda non
 * ha modo di mostrare motivi diversi, e la seduta direbbe «funziona» avendo visto un motivo solo. Chiedere
 * all'esecutore di dipingerle ogni volta rende la verifica **non ripetibile** — due sedute su due
 * allestimenti diversi non si confrontano.
 *
 * 🔴 **Misurato il 2026-08-31 sull'Editor vivo, ed e' la ragione per cui questa fixture esiste.** La mappa
 * su cui `L_DevSandbox` era aperto (`DA_HexMap_Scratch_Basin`, 65 celle) e' stata interrogata cella per
 * cella attraverso il ponte MCP, con lo stesso A* canonico che la sonda usa:
 *
 * ```text
 * Reachable 57 · BlocksMovement 3 · OutOfBudget 1 · NoRoute 0
 * ```
 *
 * Tre motivi su quattro ottenibili, e **zero** celle irraggiungibili. Su quella mappa la seduta non avrebbe
 * potuto distinguere `NoRoute` da `OutOfBudget` — cioe' proprio la distinzione per cui #711 e' stata fatta.
 */
namespace
{
	/** L'unita' sondata: una sola, come nel tool. Il nome e' distinto per file (unity build). */
	FRTHexSnapshot ProbeYardSnapshot(const URTHexMapAsset* Map, const FRTCellId& Start, int32 Budget)
	{
		return URTHexSimLibrary::MakeSnapshot(Map, { FRTHexSimUnit(0, Start, Budget) });
	}
}

/**
 * Le tre condizioni ci sono, e si leggono dall'asset invece che dalla fiducia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTProbeYardConditionsTest,
	"RefactorTactics.MovementProbe.ProbeYardCarriesTheThreeConditions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTProbeYardConditionsTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFixtureArena(GetTransientPackage(), TEXT("ProbeYard"));
	if (!TestNotNull(TEXT("la fixture si costruisce per nome"), Map))
	{
		return false;
	}

	int32 Costly = 0;
	int32 Blocking = 0;
	for (const FRTHexCellData& Cell : Map->Cells)
	{
		if (Cell.bBlocksMovement)      { ++Blocking; }
		else if (Cell.TotalMoveCost() >= 2) { ++Costly; }
	}

	TestTrue(TEXT("almeno una superficie costosa"), Costly >= 1);
	TestTrue(TEXT("almeno una cella che blocca il movimento"), Blocking >= 1);

	// La zona irraggiungibile non si legge da un flag: si misura chiedendo al grafo.
	const FRTCellId Start(0, 0, 0);
	const FRTHexSnapshot Snap = ProbeYardSnapshot(Map, Start, /*Budget=*/ 99);
	const TArray<FRTHexReachableCell> Everything = URTHexSimLibrary::ReachableCells(Snap, 0);

	int32 Unreachable = 0;
	for (const FRTHexCellData& Cell : Map->Cells)
	{
		if (Cell.bBlocksMovement) { continue; } // un ostacolo non e' una zona isolata: e' un altro motivo
		if (!Everything.ContainsByPredicate([&Cell](const FRTHexReachableCell& R) { return R.Cell == Cell.Id; }))
		{
			++Unreachable;
		}
	}

	TestTrue(TEXT("almeno una cella libera che NESSUN budget raggiunge"), Unreachable >= 1);
	TestTrue(TEXT("e la partenza suggerita esiste ed e' percorribile"),
		URTHexSimLibrary::IsCellFree(Snap, Start, 0));
	return true;
}

/**
 * 🔴 **Il test che rende la seduta possibile.** Da `(0,0)` con un budget di catalogo, la fixture produce
 * **tutti e quattro** i motivi che la sonda puo' mostrare a schermo — e li produce distinti.
 *
 * ⚠️ **Quattro, non cinque, e non e' una svista.** `ERTHexProbeExclusion::Occupied` esiste ed e' testato,
 * ma il tool d'editor non puo' mostrarlo: il suo snapshot contiene **una sola unita'**, quindi nessuna cella
 * risultera' mai occupata da un'altra. Una voce di seduta che promettesse «cinque motivi» chiederebbe di
 * osservare qualcosa che il codice non produce.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTProbeYardExclusionsTest,
	"RefactorTactics.MovementProbe.ProbeYardShowsEveryReachableExclusion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTProbeYardExclusionsTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFixtureArena(GetTransientPackage(), TEXT("ProbeYard"));
	if (!TestNotNull(TEXT("la fixture si costruisce"), Map))
	{
		return false;
	}

	const FRTCellId Start(0, 0, 0);
	constexpr int32 Budget = 5; // ordine di grandezza di `URTHeroData::MovePoints`
	const FRTHexSnapshot Snap = ProbeYardSnapshot(Map, Start, Budget);
	const TArray<FRTHexReachableCell> Set = URTHexSimLibrary::ReachableCells(Snap, 0);

	TSet<ERTHexProbeExclusion> Seen;
	for (const FRTHexCellData& Cell : Map->Cells)
	{
		Seen.Add(URTHexSimLibrary::ClassifyProbeCell(Snap, 0, Set, Cell.Id));
	}
	// Fuori dalla mappa: il quarto motivo, che nessuna cella dell'asset puo' produrre.
	Seen.Add(URTHexSimLibrary::ClassifyProbeCell(Snap, 0, Set, FRTCellId(99, 99, 0)));

	TestTrue(TEXT("c'e' del terreno raggiungibile"),   Seen.Contains(ERTHexProbeExclusion::Reachable));
	TestTrue(TEXT("c'e' un ostacolo"),                 Seen.Contains(ERTHexProbeExclusion::BlocksMovement));
	TestTrue(TEXT("c'e' terreno FUORI BUDGET"),        Seen.Contains(ERTHexProbeExclusion::OutOfBudget));
	TestTrue(TEXT("c'e' una zona SENZA STRADA"),       Seen.Contains(ERTHexProbeExclusion::NoRoute));
	TestTrue(TEXT("e il fuori mappa si distingue"),    Seen.Contains(ERTHexProbeExclusion::NotOnMap));

	// ⚠️ Il quinto motivo NON deve comparire: con una sola unita' nessuna cella e' occupata da un'altra.
	// Se un giorno comparisse, vorrebbe dire che la sonda ha smesso di guardare l'unita' che dichiara.
	TestFalse(TEXT("`Occupied` non e' ottenibile con una sola unita'"),
		Seen.Contains(ERTHexProbeExclusion::Occupied));
	return true;
}

/**
 * La fixture e' nominabile come le altre: `KnownFixtureIds()` la elenca, ed e' cio' che la rende caricabile
 * da `rt.Map.Fixture` in seduta senza dipingere niente a mano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTProbeYardIsNamedTest,
	"RefactorTactics.MovementProbe.ProbeYardIsAFixtureByName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTProbeYardIsNamedTest::RunTest(const FString&)
{
	TestTrue(TEXT("`ProbeYard` e' fra i nomi noti"),
		URTMatchSetupLibrary::KnownFixtureIds().Contains(TEXT("ProbeYard")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
