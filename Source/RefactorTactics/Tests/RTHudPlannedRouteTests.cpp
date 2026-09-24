// La rotta pianificata: se si disegna, e con quali celle.
//
// Perché esiste (#2184): le due decisioni vivevano dentro `ARTHUD::DrawHUD`, che il motore chiama ogni
// fotogramma e che **non ha copertura headless**.
//
// 🔴 **Ciò che la decisione protegge è il RETTANGOLO, non la linea — e la prima stesura di questo file
// diceva il contrario.** Il modello deriva `bMoving` da `ARTUnit::HasPlannedNormalMove()`, cioè
// `PlannedCell != Cell || PlannedPath.Num() > 1`. Quindi `bMoving` falso **implica** destinazione uguale
// alla cella e rotta più corta di due celle: non esiste lo stato «unità ferma con una rotta lunga», e un
// test che lo costruisce pinna una regola su un input che il modello non può produrre.
//
// Senza la decisione, allora, la linea non comparirebbe comunque — un percorso da una cella a se stessa è
// lungo una cella e non produce segmenti — ma il `DrawRect` della destinazione finirebbe **sulla cella
// dell'unità stessa**: un marcatore di arrivo sopra ogni unità ferma in campo.
//
// ⌫ La prima stesura costruiva `bMoving = false` con tre celle di rotta e motivava la decisione con «una
// rotta verso una destinazione che l'unità non percorrerà». Entrambe le cose erano sbagliate, e le ha
// trovate la code review leggendo `HasPlannedNormalMove()`. Il difetto non era il codice di produzione:
// era il **fixture**, e un fixture irrealizzabile fa passare qualunque regola.
//
// ⚠️ **`Map` è un parametro, e i test lo costruiscono senza mondo**: `MakeFlatArena(GetTransientPackage(),
// N)` è un `NewObject` su un asset. Zero `SpawnActor` in questo file.
//
// ── TABELLA DELLE ATTESE DI MUTAZIONE ────────────────────────────────────────────────────────────────
// Scritta prima di lanciare, e — dopo la lezione della fetta 5 — **eseguibile**: ogni riga nomina un test
// che esiste, parte dal codice spedito, e descrive una mutazione che compila.
//
//   # | mutazione su `ARTHUD::ComposePlannedRoute`                | rosso atteso              | esito
//  ---|-----------------------------------------------------------|---------------------------|---------
//   1 | `Out.bShow = View.bMoving;` → `= true;`                    | RouteIsHidden…            | 1, esatto
//   2 | `Out.bShow = View.bMoving;` → `= !View.bMoving;`           | RouteIsHidden… + Uses…    | 4 rossi
//   3 | `View.PlannedPath.Num() >= 2` → `>= 1`                     | OneCellIsNot…             | 1, esatto
//   4 | `View.PlannedPath.Num() >= 2` → `>= 3`                     | RouteUsesTheViewRoute…    | 1, esatto †
//
// 🔴 **† La 4 è SOPRAVVISSUTA alla prima esecuzione, ed è il difetto che questa tabella serve a trovare.**
// Il caso della rotta composita ha **tre** celle, quindi `3 >= 3` restava vero e il test verde: il confine
// a **due** celle non era coperto da nessuna asserzione. È lo stesso difetto che la fetta 1 di questa issue
// aveva già pagato — una soglia misurata da un lato solo passa anche quando è spostata di uno. Il caso
// `DueEsatte` è stato aggiunto **dopo** averlo visto sopravvivere, e con quello la 4 cade.
//
// ⚠️ La 2 produce quattro rossi invece dei due attesi: scambiare i rami rompe ogni proprietà del file.
// L'attesa era un minimo, e una sorpresa in eccesso si registra quanto una in difetto.
//
// ⛔ Ogni build di mutazione ha dato `Result: Succeeded` prima della run, e ogni run ha usato il filtro
// `RefactorTactics.HUD` **intero**: `RefactorTactics.HUD.Route` ne cattura solo due su cinque — due test
// non cominciano per `Route` — e un filtro che non copre tutti i test darebbe un verde che non significa
// niente.
//
// Le quattro colpiscono una reticenza o un confine. Nessuna è un conteggio: sono tutte proprietà.

#include "Misc/AutomationTest.h"
#include "UI/RTHUD.h"
#include "Turn/RTIntentPrivacyLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Map/RTHexMapAsset.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Arena piatta di raggio 3, costruita senza mondo: serve solo al ricalcolo. */
	URTHexMapAsset* MakeRouteTestMap()
	{
		return URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 3);
	}
}

/**
 * 🔴 **Un'unità ferma non mostra la rotta — e lo stato è quello che il modello produce davvero.**
 *
 * `bMoving` falso implica `PlannedCell == Cell`: è esattamente la condizione in cui il rettangolo di
 * destinazione finirebbe sotto l'unità.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudRouteHiddenWhenStandingTest,
	"RefactorTactics.HUD.RouteIsHiddenForAStandingUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudRouteHiddenWhenStandingTest::RunTest(const FString&)
{
	// Lo stato di un'unità ferma, come `HasPlannedNormalMove()` lo consente: destinazione = cella, e
	// nessuna rotta composita. Costruirlo altrimenti sarebbe inventare un input.
	FRTIntentView Ferma;
	Ferma.bMoving = false;
	Ferma.OwnerCell = FRTCellId(1, 0, 0);
	Ferma.PlannedCell = FRTCellId(1, 0, 0);

	const FRTPlannedRoutePresentation Rotta = ARTHUD::ComposePlannedRoute(Ferma, MakeRouteTestMap());

	TestFalse(TEXT("⛔ nessuna rotta, e nessun rettangolo, per un'unita' ferma"), Rotta.bShow);

	// ⛔ **Il contratto della struct**: `bShow` falso significa che i campi accanto non vanno letti, ed è
	// la stessa forma di `FRTIntentPresentation`. Qui si verifica che sia vero, non che sia dichiarato:
	// senza, una stesura potrebbe riempire `PathCells` e lasciare a chi disegna il compito di ignorarle.
	TestEqual(TEXT("e le celle restano vuote, non «da ignorare»"), Rotta.PathCells.Num(), 0);

	return true;
}

/**
 * La rotta della vista si usa quando ha almeno un segmento. È il confine SOPRA della soglia, e insieme
 * al test successivo la fissa da entrambi i lati.
 *
 * ⚠️ Le celle si confrontano una per una, non solo di numero: un'implementazione che ricalcolasse
 * comunque produrrebbe un percorso della stessa lunghezza fra le stesse due estremità, e un test che
 * contasse soltanto passerebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudRouteUsesViewRouteTest,
	"RefactorTactics.HUD.RouteUsesTheViewRouteWhenItHasASegment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudRouteUsesViewRouteTest::RunTest(const FString&)
{
	// Una rotta COMPOSITA: passa da (0,1,0), che l'A* non sceglierebbe per andare da (0,0,0) a (2,0,0).
	// È il caso dei waypoint, cioè la ragione per cui la vista porta una rotta propria.
	FRTIntentView Composita;
	Composita.bMoving = true;
	Composita.OwnerCell = FRTCellId(0, 0, 0);
	Composita.PlannedCell = FRTCellId(2, 0, 0);
	Composita.PlannedPath.Add(FRTCellId(0, 0, 0));
	Composita.PlannedPath.Add(FRTCellId(0, 1, 0));
	Composita.PlannedPath.Add(FRTCellId(2, 0, 0));

	const FRTPlannedRoutePresentation Rotta = ARTHUD::ComposePlannedRoute(Composita, MakeRouteTestMap());

	TestTrue(TEXT("l'unita' che si muove mostra la rotta"), Rotta.bShow);
	if (!TestEqual(TEXT("e sono le tre celle della vista, non un ricalcolo"), Rotta.PathCells.Num(), 3))
	{
		return false;
	}
	TestTrue(TEXT("⛔ compreso il waypoint che l'A* non avrebbe scelto"),
		Rotta.PathCells.Contains(FRTCellId(0, 1, 0)));

	// 🔴 **DUE celle esatte: il confine, e la prima stesura non lo copriva.**
	//
	// ⌫ Il caso qui sopra ne ha tre, quindi la mutazione «soglia `>= 3`» **sopravviveva**: `3 >= 3` è vero
	// e il test restava verde. È il difetto che la fetta 1 di questa issue aveva già pagato — una soglia
	// misurata da un lato solo passa anche quando è spostata di uno.
	//
	// Le due celle sono NON adiacenti, ed è ciò che rende l'asserzione capace di distinguere: il ricalcolo
	// fra le stesse estremità darebbe tre celle, quindi il conteggio separa «rotta della vista» da
	// «ricalcolo» invece di essere soddisfatto da entrambi.
	FRTIntentView DueEsatte;
	DueEsatte.bMoving = true;
	DueEsatte.OwnerCell = FRTCellId(0, 0, 0);
	DueEsatte.PlannedCell = FRTCellId(2, 0, 0);
	DueEsatte.PlannedPath.Add(FRTCellId(0, 0, 0));
	DueEsatte.PlannedPath.Add(FRTCellId(2, 0, 0)); // salta (1,0,0): il ricalcolo non lo farebbe

	const FRTPlannedRoutePresentation Confine = ARTHUD::ComposePlannedRoute(DueEsatte, MakeRouteTestMap());

	TestEqual(TEXT("⛔ due celle bastano: restano DUE, non diventano il percorso ricalcolato"),
		Confine.PathCells.Num(), 2);

	return true;
}

/**
 * 🔴 **Una cella sola non è una rotta: si ricalcola.**
 *
 * Il confine SOTTO. Con una cella chi disegna non produce nessun segmento, quindi usarla lascerebbe
 * l'unità **senza** rotta visibile — e il sintomo di quel difetto è un'assenza, che non si nota
 * guardando lo schermo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudRouteOneCellIsRecomputedTest,
	"RefactorTactics.HUD.OneCellIsNotARouteAndIsRecomputed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudRouteOneCellIsRecomputedTest::RunTest(const FString&)
{
	URTHexMapAsset* Mappa = MakeRouteTestMap();

	FRTIntentView UnaSola;
	UnaSola.bMoving = true;
	UnaSola.OwnerCell = FRTCellId(0, 0, 0);
	UnaSola.PlannedCell = FRTCellId(2, 0, 0);
	UnaSola.PlannedPath.Add(FRTCellId(0, 0, 0)); // solo l'origine

	const FRTPlannedRoutePresentation Rotta = ARTHUD::ComposePlannedRoute(UnaSola, Mappa);

	TestTrue(TEXT("si mostra: l'unita' si muove"), Rotta.bShow);

	// ⛔ Il ricalcolo ha prodotto un percorso VERO, non la singola cella che la vista portava.
	TestTrue(TEXT("⛔ una cella sola non basta: si ricalcola, e il percorso ha piu' di una cella"),
		Rotta.PathCells.Num() > 1);
	TestTrue(TEXT("e arriva alla destinazione pianificata"),
		Rotta.PathCells.Contains(FRTCellId(2, 0, 0)));

	// Nessuna rotta affatto: stesso ramo, stessa risposta.
	FRTIntentView Vuota;
	Vuota.bMoving = true;
	Vuota.OwnerCell = FRTCellId(0, 0, 0);
	Vuota.PlannedCell = FRTCellId(2, 0, 0);
	TestTrue(TEXT("e con nessuna cella si ricalcola ugualmente"),
		ARTHUD::ComposePlannedRoute(Vuota, Mappa).PathCells.Num() > 1);

	return true;
}

/**
 * Una mappa nulla non fa cadere niente: il ricalcolo torna a mani vuote, e chi disegna non disegna.
 *
 * ⚠️ È il caso che `DrawHUD` può davvero incontrare — `Map` arriva da `ARTHexMapActor::FindInWorld`, che
 * può non trovare nulla — e una funzione pura deve rispondere invece di presupporre.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudRouteNullMapTest,
	"RefactorTactics.HUD.RouteWithoutAMapIsEmptyNotACrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudRouteNullMapTest::RunTest(const FString&)
{
	FRTIntentView V;
	V.bMoving = true;
	V.OwnerCell = FRTCellId(0, 0, 0);
	V.PlannedCell = FRTCellId(2, 0, 0);

	const FRTPlannedRoutePresentation Rotta = ARTHUD::ComposePlannedRoute(V, nullptr);

	TestTrue(TEXT("la decisione di mostrare non dipende dalla mappa"), Rotta.bShow);
	TestEqual(TEXT("ma senza mappa non c'e' percorso da disegnare"), Rotta.PathCells.Num(), 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
