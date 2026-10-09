// Quali celle di un intento prendono un rettangolo, in che ordine, con che forma e con che tinta.
//
// Perché esiste (#2184): `ARTHUD::DrawHUD` teneva queste tre regole in mezzo al tracciamento, e le
// spiegava in **undici righe di commento** che nessun test poteva verificare:
//
//   - la destinazione ha un rettangolo solo se la rotta si mostra;
//   - i waypoint ce l'hanno sempre;
//   - ⛔ nessuno dei due è graduato dalla certezza.
//
// 🔴 **La terza regola è nata da un difetto vero, e il commento lo dichiarava.** La prima stesura
// graduava la destinazione, sbagliando due volte: quel blocco vive dentro `bMoving`, e `ClassifyPlan`
// rende `Uncertain` ogni volta che `bMoving` — il livello sarebbe **sempre** lo stesso, quindi attenuare
// non distingue niente e toglie leggibilità. Il rettangolo passava da alpha `0.35` a `0.105`, in
// permanenza, per ogni unità in movimento. Una regola nata da un difetto e custodita solo da un
// commento è la definizione di ciò che questa issue porta fuori.
//
// ⚠️ **L'ORDINE è parte del contratto, non una conseguenza.** Destinazione prima, waypoint dopo: è
// l'ordine in cui venivano disegnati, e un rettangolo che passa sopra un altro si vede. Invertirlo non
// romperebbe niente di compilabile.
//
// ── TABELLA DELLE ATTESE DI MUTAZIONE ────────────────────────────────────────────────────────────────
// Scritta prima di lanciare ed eseguibile; esiti misurati sull'insieme di test FINALE.
//
//   # | mutazione                                                     | rosso atteso                | esito
//  ---|-----------------------------------------------------------------|-----------------------------|------
//   1 | `if (bRouteShown)` → `if (View.PlannedCell.IsValid())`           | …OnlyWhenTheRouteIsShown…   | 2 rossi
//   2 | `DestinationMarkerHalfPx = 12.f` → `13.f`                        | …PinsTheTwoSizes…           | 1, esatto
//   3 | `WaypointMarkerHalfPx = 5.f` → `6.f`                             | …PinsTheTwoSizes…           | 1, esatto
//   4 | `DestinationMarkerAlpha = 0.35f` → `1.f`                         | …DimsTheDestinationOnly…    | 1, esatto
//   5 | i waypoint aggiunti PRIMA della destinazione                     | …PutsTheDestinationFirst…   | 3 rossi
//   6 | il colore della destinazione → `IntentColor` (niente alpha)      | …DimsTheDestinationOnly…    | 1, esatto
//
// 🔑 **La 1 riproduce il difetto documentato, e uccide DUE test.** `PlannedCell` resta valorizzata anche
// a piano concluso: letta come presenza, disegnerebbe un rettangolo sotto un'unità ferma — la stessa
// trappola che `ComposePlannedRoute` dichiara per sé.
//
// ⛔ **E la trappola è più profonda di come quel commento la descrive.** `FRTCellId::IsValid()` non è un
// controllo debole: è una **tautologia**. `RTCellId.h:45,51` dà `CubeZ() = -X - Y` e
// `IsValid() = (X + Y + CubeZ() == 0)`, cioè `0 == 0`. Non esiste una cella che la renda falsa, quindi
// `if (PlannedCell.IsValid())` non è «la domanda sbagliata»: è **nessuna domanda**. Ed è il motivo per
// cui questa mutazione aggiunge sempre la destinazione, anche per un'unità ferma. Oltre al gate cade infatti
// `…AreEmptyWithoutRouteOrWaypoints`, cioè proprio il caso dell'unità **ferma**: la mutazione riproduce
// il difetto storico e il test lo vede. Una riga di tabella che uccide il caso giusto per la ragione
// giusta vale più di tre che uccidono qualcosa.
//
// ⚠️ **La 5 ne uccide tre e non uno**, perché `Insert(..., 0)` non si limita a invertire l'ordine: cambia
// anche quale marcatore sta a `M[0]` e `M[1]`, e i test delle dimensioni e dell'opacità leggono per
// indice. L'attesa era un minimo, non un'uguaglianza — «tre invece di uno» qui è accoppiamento fra
// asserzioni, non copertura in più.

#include "Misc/AutomationTest.h"
#include "UI/RTHUD.h"
#include "Turn/RTIntentPrivacyLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// ⚠️ Nomi lunghi apposta: a livello di file, sotto unity build, diventano globali che nascondono le
	// locali omonime degli altri file del blocco, e `C4459` qui è un errore.
	const FLinearColor TintaDiProva(0.2f, 0.4f, 0.6f, 1.f);

	FRTIntentView VistaConWaypoint()
	{
		FRTIntentView V;
		V.OwnerCell = FRTCellId(0, 0, 0);
		V.PlannedCell = FRTCellId(3, 0, 0);
		V.PlannedWaypoints = { FRTCellId(1, 0, 0), FRTCellId(2, 0, 0) };
		return V;
	}
}

/**
 * 🔴 **La destinazione compare solo se la rotta si mostra, non se la cella esiste.**
 *
 * `PlannedCell` è valorizzata in entrambi i casi: è il verdetto della rotta a decidere, ed è la trappola
 * che `ComposePlannedRoute` documenta per sé.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudIntentMarkersRouteGateTest,
	"RefactorTactics.HUD.IntentMarkersShowTheDestinationOnlyWhenTheRouteIsShown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudIntentMarkersRouteGateTest::RunTest(const FString&)
{
	const FRTIntentView V = VistaConWaypoint();

	const TArray<FRTIntentMarker> SenzaRotta = ARTHUD::ComposeIntentMarkers(V, false, TintaDiProva);
	if (!TestEqual(TEXT("senza rotta restano i soli due waypoint"), SenzaRotta.Num(), 2)) { return false; }

	// ⛔ **La premessa che conta: la destinazione E' popolata, e non basta.** Si asserisce che sia una
	// cella DIVERSA dall'origine, non che sia «valida».
	//
	// 🔴 **`IsValid()` qui sarebbe stato vacuo, ed e' stato verificato invece che supposto.**
	// `RTCellId.h:45,51` definisce `CubeZ() = -X - Y` e `IsValid() = (X + Y + CubeZ() == 0)`, cioe'
	// `0 == 0`: e' vera per QUALUNQUE cella, e nessun valore puo' renderla falsa. Un'asserzione che non
	// puo' cadere sembra una premessa e non ne e' una. ⚠️ La prima stesura di questa riga la usava —
	// trovata dalla revisione della PR #3363, non da chi l'ha scritta.
	TestNotEqual(TEXT("⛔ premessa: la destinazione e' una cella diversa dall'origine"),
		V.PlannedCell, V.OwnerCell);
	for (const FRTIntentMarker& M : SenzaRotta)
	{
		TestNotEqual(TEXT("⛔ e nessun marcatore cade sulla destinazione"), M.Cell, V.PlannedCell);
	}

	const TArray<FRTIntentMarker> ConRotta = ARTHUD::ComposeIntentMarkers(V, true, TintaDiProva);
	TestEqual(TEXT("con la rotta si aggiunge la destinazione"), ConRotta.Num(), 3);

	return true;
}

/**
 * ⚠️ **L'ordine è il contratto: destinazione prima, waypoint dopo.**
 *
 * È l'ordine in cui venivano disegnati, e un rettangolo che passa sopra un altro si vede. Invertirlo
 * compila e non rompe niente che non sia questo test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudIntentMarkersOrderTest,
	"RefactorTactics.HUD.IntentMarkersPutsTheDestinationFirst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudIntentMarkersOrderTest::RunTest(const FString&)
{
	const FRTIntentView V = VistaConWaypoint();
	const TArray<FRTIntentMarker> M = ARTHUD::ComposeIntentMarkers(V, true, TintaDiProva);

	if (!TestEqual(TEXT("tre marcatori"), M.Num(), 3)) { return false; }

	TestEqual(TEXT("⛔ il primo e' la DESTINAZIONE"), M[0].Cell, V.PlannedCell);
	TestEqual(TEXT("poi i waypoint, nell'ordine in cui li porta la vista"), M[1].Cell, V.PlannedWaypoints[0]);
	TestEqual(TEXT("e il secondo"), M[2].Cell, V.PlannedWaypoints[1]);

	return true;
}

/**
 * Le due mezze-dimensioni, al pixel: la destinazione è più grande del waypoint.
 *
 * ⚠️ I valori sono scritti a mano e non riletti dalle costanti: un test che le rileggesse sarebbe
 * soddisfatto da qualunque numero, e lo Scope della issue vieta di spostare i pixel.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudIntentMarkersSizesTest,
	"RefactorTactics.HUD.IntentMarkersPinsTheTwoSizes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudIntentMarkersSizesTest::RunTest(const FString&)
{
	const TArray<FRTIntentMarker> M = ARTHUD::ComposeIntentMarkers(VistaConWaypoint(), true, TintaDiProva);
	if (!TestEqual(TEXT("tre marcatori"), M.Num(), 3)) { return false; }

	TestEqual(TEXT("⛔ destinazione: mezzo lato 12, quindi lato 24"), M[0].HalfSize, 12.f, 0.f);
	TestEqual(TEXT("⛔ waypoint: mezzo lato 5, quindi lato 10"), M[1].HalfSize, 5.f, 0.f);

	// 🔑 La proprieta' che i due numeri insieme esprimono, e che sopravvive a un cambio di scala.
	TestTrue(TEXT("🔑 il waypoint e' piu' piccolo della destinazione"), M[1].HalfSize < M[0].HalfSize);

	return true;
}

/**
 * ⛔ **Solo la destinazione è attenuata, e NON per certezza.**
 *
 * L'alpha `0.35` la distingue dalla linea che la raggiunge; i waypoint restano a tinta piena. Nessuno
 * dei due è graduato dal livello di certezza, che qui è `Uncertain` per costruzione — graduarli non
 * distinguerebbe niente e toglierebbe leggibilità.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudIntentMarkersAlphaTest,
	"RefactorTactics.HUD.IntentMarkersDimsTheDestinationOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudIntentMarkersAlphaTest::RunTest(const FString&)
{
	const TArray<FRTIntentMarker> M = ARTHUD::ComposeIntentMarkers(VistaConWaypoint(), true, TintaDiProva);
	if (!TestEqual(TEXT("tre marcatori"), M.Num(), 3)) { return false; }

	TestEqual(TEXT("⛔ la destinazione e' attenuata a 0,35"), M[0].Color.A, 0.35f, 0.f);
	TestEqual(TEXT("⛔ e il waypoint resta a tinta PIENA"), M[1].Color.A, 1.f, 0.f);

	// La tinta e' quella dell'intento in entrambi: cambia l'opacita', non il colore.
	TestEqual(TEXT("stessa componente rossa"), M[0].Color.R, TintaDiProva.R, 0.f);
	TestEqual(TEXT("e stessa verde"), M[1].Color.G, TintaDiProva.G, 0.f);

	return true;
}

/**
 * Senza waypoint resta la sola destinazione, e senza nessuno dei due l'elenco è vuoto.
 *
 * ⚠️ Il caso vuoto non è una formalità: è ciò che il sito di disegno riceve per ogni unità ferma, cioè
 * la maggioranza dei fotogrammi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudIntentMarkersEmptyTest,
	"RefactorTactics.HUD.IntentMarkersAreEmptyWithoutRouteOrWaypoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudIntentMarkersEmptyTest::RunTest(const FString&)
{
	FRTIntentView Ferma;
	Ferma.OwnerCell = FRTCellId(0, 0, 0);
	Ferma.PlannedCell = FRTCellId(0, 0, 0);

	TestEqual(TEXT("senza rotta ne' waypoint: nessun marcatore"),
		ARTHUD::ComposeIntentMarkers(Ferma, false, TintaDiProva).Num(), 0);
	TestEqual(TEXT("con la sola rotta: la destinazione e basta"),
		ARTHUD::ComposeIntentMarkers(Ferma, true, TintaDiProva).Num(), 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
