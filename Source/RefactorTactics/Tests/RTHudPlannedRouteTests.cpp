// La rotta pianificata: se si disegna, e da dove arrivano le sue celle.
//
// Perché esiste (#2184): le due decisioni vivevano dentro `ARTHUD::DrawHUD`, che il motore chiama ogni
// fotogramma e che **non ha copertura headless**. Sono pure funzioni della sola `FRTIntentView` — nessuna
// tocca `Project()`, `Canvas`, `Map` o una coordinata.
//
// 🔴 **`bShow` non è un predicato di presenza, ed è tutto il punto.** La lettura ingenua — «c'è una
// destinazione, quindi disegnala» — è sbagliata, e il codice lo dimostra in due punti: `PlannedCell` e
// `PlannedPath` sono copiati **incondizionatamente** dal modello (`RTHudViewModel.cpp:532` e `:537`) e dal
// filtro di privacy (`RTIntentPrivacyLibrary.cpp:56` e `:60`). Restano quindi valorizzati su un'unità che
// non si muove: un piano di scatto, un piano sostituito da un attacco, il residuo del turno prima.
// Senza la decisione si disegnerebbe una rotta verso una destinazione che l'unità non percorrerà.
//
// ⛔ **E la soglia della rotta è DUE celle, non una.** Chi disegna unisce le celle a coppie partendo da
// `i = 1`: una rotta di una cella sola — la sola origine — non produce nessun segmento, e l'unità
// resterebbe senza rotta visibile invece di ricadere sull'A* dell'autorità. È una reticenza: il caso non
// si vede mai a schermo, perché il sintomo è l'assenza di un disegno.
//
// ⚠️ **Il ricalcolo NON è sotto test qui**, e non è una dimenticanza: `URTHexPathLibrary::FindPath` vuole
// `Map`, quindi vive dove ha i suoi ingressi. Qui si verifica la **scelta**, che è l'altra metà.
//
// ── TABELLA DELLE ATTESE DI MUTAZIONE ────────────────────────────────────────────────────────────────
// Scritta prima di lanciare — e, dopo la lezione della fetta 5, **eseguibile**: ogni riga nomina un test
// che esiste, parte dal codice spedito, e descrive una mutazione che compila. Una tabella che non si può
// rieseguire non è evidenza, è una promessa.
//
//   # | mutazione su `ARTHUD::ComposePlannedRoutePresentation`   | rosso atteso
//  ---|----------------------------------------------------------|----------------------------------------
//   1 | `Out.bShow = View.bMoving;` → `= true;`                   | RouteIsHiddenForAStandingUnit
//   2 | `Out.bShow = View.bMoving;` → `= !View.bMoving;`          | RouteIsHiddenForAStandingUnit
//     |                                                          |  + RouteIsShownForAMovingUnit
//   3 | `View.PlannedPath.Num() >= 2` → `>= 1`                    | OneCellIsNotARouteAndFallsBack
//   4 | `View.PlannedPath.Num() >= 2` → `>= 3`                    | TwoCellsAreEnoughToBeARoute
//
// Le quattro colpiscono una reticenza o un confine, cioè le due categorie che le fette precedenti di
// questa issue hanno mostrato essere le più fragili. Nessuna è un conteggio: sono tutte proprietà.

#include "Misc/AutomationTest.h"
#include "UI/RTHUD.h"
#include "Turn/RTIntentPrivacyLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * 🔴 **Un'unità ferma non mostra la rotta, benché la porti.**
 *
 * È il caso che nessun test teneva fermo, ed è quello che un'implementazione ingenua sbaglia: la vista di
 * un'unità che non si muove ha comunque `PlannedCell` e `PlannedPath` valorizzati.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudRouteHiddenWhenStandingTest,
	"RefactorTactics.HUD.RouteIsHiddenForAStandingUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudRouteHiddenWhenStandingTest::RunTest(const FString&)
{
	FRTIntentView Ferma;
	Ferma.bMoving = false;

	// ⛔ I dati della rotta CI SONO: è esattamente la condizione in cui la lettura ingenua sbaglia.
	Ferma.PlannedCell = FRTCellId(5, 0, 0);
	Ferma.PlannedPath.Add(FRTCellId(0, 0, 0));
	Ferma.PlannedPath.Add(FRTCellId(3, 0, 0));
	Ferma.PlannedPath.Add(FRTCellId(5, 0, 0));

	const FRTPlannedRoutePresentation Rotta = ARTHUD::ComposePlannedRoutePresentation(Ferma);

	TestFalse(TEXT("⛔ nessuna rotta per un'unita' che non si muove, anche se la porta"), Rotta.bShow);

	// ⚠️ L'altra metà resta calcolata comunque, ed è corretto: `bRouteComesFromTheView` descrive la vista,
	// non autorizza il disegno. Chi consuma legge `bShow` per primo — è la stessa forma di
	// `FRTIntentPresentation`, dove «`bShow` falso significa nessuna riga, e i campi accanto non vanno letti».
	TestTrue(TEXT("e la descrizione della rotta resta coerente col dato"), Rotta.bRouteComesFromTheView);

	return true;
}

/**
 * Un'unità che si muove mostra la rotta. È la metà ovvia, e serve come controllo:
 * senza, una regola che risponde sempre «no» passerebbe il test di sopra.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudRouteShownWhenMovingTest,
	"RefactorTactics.HUD.RouteIsShownForAMovingUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudRouteShownWhenMovingTest::RunTest(const FString&)
{
	FRTIntentView InMovimento;
	InMovimento.bMoving = true;
	InMovimento.PlannedCell = FRTCellId(2, 0, 0);

	TestTrue(TEXT("l'unita' che si muove mostra la rotta"),
		ARTHUD::ComposePlannedRoutePresentation(InMovimento).bShow);

	return true;
}

/**
 * 🔴 **Una cella sola non è una rotta: si ricade sul ricalcolo.**
 *
 * Il confine sotto: con una cella chi disegna non produce nessun segmento, quindi usare la rotta della
 * vista lascerebbe l'unità **senza** rotta visibile. Il sintomo di questo difetto è un'assenza, e le
 * assenze non si notano guardando lo schermo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudRouteOneCellFallsBackTest,
	"RefactorTactics.HUD.OneCellIsNotARouteAndFallsBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudRouteOneCellFallsBackTest::RunTest(const FString&)
{
	FRTIntentView Vuota;
	Vuota.bMoving = true;
	TestFalse(TEXT("nessuna cella: si ricalcola"),
		ARTHUD::ComposePlannedRoutePresentation(Vuota).bRouteComesFromTheView);

	FRTIntentView UnaSola;
	UnaSola.bMoving = true;
	UnaSola.PlannedPath.Add(FRTCellId(0, 0, 0));
	TestFalse(TEXT("⛔ una cella sola e' la sola origine, non una rotta: si ricalcola"),
		ARTHUD::ComposePlannedRoutePresentation(UnaSola).bRouteComesFromTheView);

	// ⛔ Asserzione di controllo: la decisione non dipende da `bShow`, altrimenti questo test misurerebbe
	// la regola sbagliata. Un'unità ferma con una sola cella dà la stessa risposta.
	FRTIntentView FermaUnaSola;
	FermaUnaSola.bMoving = false;
	FermaUnaSola.PlannedPath.Add(FRTCellId(0, 0, 0));
	TestFalse(TEXT("e la risposta non cambia se l'unita' e' ferma"),
		ARTHUD::ComposePlannedRoutePresentation(FermaUnaSola).bRouteComesFromTheView);

	return true;
}

/**
 * Due celle bastano: è il confine sopra, e insieme al precedente fissa la soglia da entrambi i lati.
 *
 * ⚠️ Due test separati e non uno: una soglia misurata da un lato solo passa anche se è spostata di uno
 * nella direzione non provata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudRouteTwoCellsAreEnoughTest,
	"RefactorTactics.HUD.TwoCellsAreEnoughToBeARoute",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudRouteTwoCellsAreEnoughTest::RunTest(const FString&)
{
	FRTIntentView Due;
	Due.bMoving = true;
	Due.PlannedPath.Add(FRTCellId(0, 0, 0));
	Due.PlannedPath.Add(FRTCellId(1, 0, 0));

	TestTrue(TEXT("due celle sono un segmento, cioe' una rotta disegnabile"),
		ARTHUD::ComposePlannedRoutePresentation(Due).bRouteComesFromTheView);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
