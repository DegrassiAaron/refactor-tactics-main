#include "Misc/AutomationTest.h"

#include "RTHexProbeReadout.h"

#include "Map/RTCellId.h"
#include "Turn/RTHexSimLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Il pannello della sonda di movimento (#711): cosa SCRIVE dato un verdetto, e quando decide di richiedere.
 *
 * 🔑 **Perche' queste due decisioni stanno qui e non nel tool.** Di un `UInteractiveTool` con hover un
 * automation test vede quasi niente — non muove un mouse, non guarda un pannello. Cio' che invece puo'
 * esaminare e' *cosa* si scrive dato un esito, e *se* una nuova domanda e' dovuta. E' la stessa scelta di
 * `RTHexLos` (#1755) e `FRTLauncherScenarioBrowser` (#1705).
 *
 * ⛔ **Nessuna regola di movimento qui dentro.** `Describe` RICEVE un `ERTHexProbeExclusion` gia' deciso da
 * `URTHexSimLibrary::ClassifyProbeCell` e lo traduce. Non guarda la mappa, non conta costi, non deduce. Il
 * giorno in cui questo file includesse il pathfinder sarebbe la seconda risposta che #711 vieta.
 */

namespace
{
	/** Nomi distinti per file: namespace anonimo + unity build (vedi `KitTolerance` in RTGrayboxMeshTests). */
	RTHexProbe::FReadout DescribeProbeReachable(int32 Cost, int32 Budget, int32 Cells)
	{
		return RTHexProbe::Describe(/*bHasUnit=*/ true, ERTHexProbeExclusion::Reachable, Cost, Budget, Cells);
	}
}

/**
 * Una cella raggiungibile dice quanto costa arrivarci E quanto ne resta: il designer sta calibrando un
 * budget, e «raggiungibile» da solo non gli dice se per un soffio o con il doppio del movimento.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexProbeReadoutReachableTest,
	"RefactorTactics.MovementProbe.ReadoutSaysCostAgainstBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexProbeReadoutReachableTest::RunTest(const FString&)
{
	const RTHexProbe::FReadout R = DescribeProbeReachable(/*Cost=*/ 2, /*Budget=*/ 4, /*Cells=*/ 3);

	TestTrue(TEXT("il costo compare"), R.Cost.Contains(TEXT("2")));
	TestTrue(TEXT("e il budget con lui"), R.Cost.Contains(TEXT("4")));
	TestEqual(TEXT("i passi sono le celle meno la partenza"), R.Steps, 2);
	TestTrue(TEXT("niente da spiegare: la cella si raggiunge"), R.Reason.IsEmpty() || R.Reason == TEXT("—"));
	return true;
}

/**
 * 🔴 **Il test che difende la ragione d'essere di #711.** I cinque motivi di esclusione producono cinque
 * righe DIVERSE, e nessuna vuota.
 *
 * Il difetto che previene ha un precedente scritto nel repository: *«"oltre il budget, bloccata o occupata"
 * mette tre difetti diversi nella stessa frase: chi gioca clicca una cella libera, legge "bloccata" e crede
 * a un difetto del gioco»*. Un readout che accorpasse due esclusioni sotto la stessa frase manderebbe il
 * designer a correggere la cosa sbagliata — ad alzare un budget dove il difetto e' nella mappa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexProbeReadoutDistinctTest,
	"RefactorTactics.MovementProbe.EveryExclusionGetsItsOwnLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexProbeReadoutDistinctTest::RunTest(const FString&)
{
	const TArray<ERTHexProbeExclusion> All = {
		ERTHexProbeExclusion::NotOnMap,
		ERTHexProbeExclusion::BlocksMovement,
		ERTHexProbeExclusion::Occupied,
		ERTHexProbeExclusion::OutOfBudget,
		ERTHexProbeExclusion::NoRoute
	};

	TSet<FString> Seen;
	for (const ERTHexProbeExclusion Ex : All)
	{
		const RTHexProbe::FReadout R = RTHexProbe::Describe(/*bHasUnit=*/ true, Ex, /*Cost=*/ 0, /*Budget=*/ 4, /*Cells=*/ 0);
		TestFalse(FString::Printf(TEXT("la ragione di %d non e' vuota"), static_cast<int32>(Ex)), R.Reason.IsEmpty());
		TestEqual(TEXT("una cella esclusa non ha passi"), R.Steps, 0);
		Seen.Add(R.Reason);
	}

	TestEqual(TEXT("cinque esclusioni, cinque frasi diverse"), Seen.Num(), All.Num());
	return true;
}

/**
 * Senza un'unita' selezionata la sonda non promette niente: non «raggiungibile», non «fuori budget».
 * All'apertura del tool e' lo stato normale, ed e' anche l'unico in cui un readout puo' mentire senza che
 * nessun calcolo sia sbagliato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexProbeReadoutNoUnitTest,
	"RefactorTactics.MovementProbe.ReadoutWithoutAUnitPromisesNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexProbeReadoutNoUnitTest::RunTest(const FString&)
{
	const RTHexProbe::FReadout R = RTHexProbe::Describe(/*bHasUnit=*/ false, ERTHexProbeExclusion::NoRoute,
		/*Cost=*/ 0, /*Budget=*/ 0, /*Cells=*/ 0);

	const RTHexProbe::FReadout Excluded = RTHexProbe::Describe(/*bHasUnit=*/ true, ERTHexProbeExclusion::NoRoute,
		/*Cost=*/ 0, /*Budget=*/ 4, /*Cells=*/ 0);

	TestNotEqual(TEXT("«nessuna unita'» non e' «nessuna strada»"), R.Reason, Excluded.Reason);
	TestFalse(TEXT("e lo dice invece di tacere"), R.Reason.IsEmpty());
	TestEqual(TEXT("nessun costo da mostrare"), R.Steps, 0);
	return true;
}

/**
 * La domanda si rifa' solo quando la cella sorvolata CAMBIA.
 *
 * 🔴 **E' un guardrail, non un'ottimizzazione.** Il mouse produce eventi mentre si muove dentro la stessa
 * cella; una sonda che rispondesse a ognuno rifarebbe, per una cella esclusa e libera, un A* a budget
 * illimitato **per fotogramma**. Il gate e' quello di #1755, condiviso e non ricopiato: due cancelli con la
 * stessa regola divergono, e il secondo lo scopre chi vede il framerate cadere.
 *
 * ⚠️ Il layer fa parte dell'identita' della cella: `(3,4,L0)` -> `(3,4,L1)` **e'** un cambio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexProbeReadoutRequeryTest,
	"RefactorTactics.MovementProbe.RequeryOnlyWhenTheHoveredCellChanges",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexProbeReadoutRequeryTest::RunTest(const FString&)
{
	const FRTCellId A(3, 4, 0);
	const FRTCellId Above(3, 4, 1);
	const FRTCellId B(5, 1, 0);

	TestFalse(TEXT("stessa cella -> nessuna nuova domanda"), RTHexProbe::ShouldRequery(true, A, true, A));
	TestTrue(TEXT("cella diversa -> si'"), RTHexProbe::ShouldRequery(true, A, true, B));
	TestTrue(TEXT("stesse coordinate, layer diverso -> si'"), RTHexProbe::ShouldRequery(true, A, true, Above));
	TestTrue(TEXT("si esce dalla mappa -> si', per cancellare"), RTHexProbe::ShouldRequery(true, A, false, A));
	TestTrue(TEXT("si entra sulla mappa -> si'"), RTHexProbe::ShouldRequery(false, A, true, A));
	TestFalse(TEXT("fuori mappa e ci si resta -> niente"), RTHexProbe::ShouldRequery(false, A, false, B));
	return true;
}

/**
 * 🔴 **Un eroe che il catalogo non conosce non e' «fuori budget».**
 *
 * Il difetto che questo test toglie: `BudgetFromCatalog` restituisce `0` per un `HeroId` sconosciuto, e con
 * budget zero ogni cella diventa `OutOfBudget` — cioe' il pannello scriveva *«fuori budget: 0 MP non
 * bastano»*, mandando a cambiare il **budget** invece dell'**eroe**. E' esattamente il difetto che #711
 * esiste per togliere, ricomparso nel guscio: il motivo giusto detto della cosa sbagliata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexProbeReadoutUnknownHeroTest,
	"RefactorTactics.MovementProbe.UnknownHeroIsNotAZeroBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexProbeReadoutUnknownHeroTest::RunTest(const FString&)
{
	// L'eroe del catalogo: il budget viene da li', e la riga non ha niente da segnalare.
	const RTHexProbe::FBudget Known = RTHexProbe::ResolveBudget(TEXT("Hero.Gadget"));
	TestTrue(TEXT("un eroe del catalogo e' riconosciuto"), Known.bKnown);
	TestTrue(TEXT("e porta il suo movimento"), Known.Points > 0);

	// Un nome che non esiste: NON e' un budget zero, e' un eroe sbagliato.
	const RTHexProbe::FBudget Unknown = RTHexProbe::ResolveBudget(TEXT("Hero.CheNonEsiste"));
	TestFalse(TEXT("un id sconosciuto non e' riconosciuto"), Unknown.bKnown);
	TestEqual(TEXT("e non inventa movimento"), Unknown.Points, 0);

	// E il pannello lo DICE, invece di parlare di budget.
	const RTHexProbe::FReadout R = RTHexProbe::Describe(/*bHasUnit=*/ true, ERTHexProbeExclusion::OutOfBudget,
		/*Cost=*/ 0, /*Budget=*/ 0, /*PathCells=*/ 0, /*bKnownHero=*/ false);
	TestTrue(TEXT("la ragione nomina l'eroe"), R.Reason.Contains(TEXT("eroe")));
	TestFalse(TEXT("e NON manda a cambiare il budget"), R.Reason.Contains(TEXT("budget")));
	return true;
}

/**
 * Con un eroe noto il messaggio resta quello di prima: la correzione non cambia il caso normale.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexProbeReadoutKnownHeroTest,
	"RefactorTactics.MovementProbe.KnownHeroStillSaysOutOfBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexProbeReadoutKnownHeroTest::RunTest(const FString&)
{
	const RTHexProbe::FReadout R = RTHexProbe::Describe(/*bHasUnit=*/ true, ERTHexProbeExclusion::OutOfBudget,
		/*Cost=*/ 0, /*Budget=*/ 5, /*PathCells=*/ 0, /*bKnownHero=*/ true);
	TestTrue(TEXT("resta un problema di budget"), R.Reason.Contains(TEXT("budget")));
	TestTrue(TEXT("e dice quanto ne ha"), R.Reason.Contains(TEXT("5")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
