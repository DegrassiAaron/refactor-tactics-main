// L'anteprima dello scatto: se si mostra, e con quale traiettoria.
//
// Perché esiste (#2184): le due decisioni vivevano dentro `ARTHUD::DrawHUD`, che il motore chiama ogni
// fotogramma e che **non ha copertura headless**.
//
// 🔴 **`bDashing` non è un flag di presenza, e il campo del modello mente sul punto.**
// `ARTUnit::PlannedDashCell` si dichiara «valida solo se `PlannedDashAbility` è impostata», ma **nessuno
// la azzera**: `ARTTurnManager` consuma l'*abilità* a fine turno (`PlannedDashAbility = INDEX_NONE`) e
// lascia la cella dov'è, e `URTHudViewModel` la copia **incondizionatamente**. Con `bDashing` falso il
// payload resta quindi valorizzato — alla destinazione dello scatto appena eseguito — e senza la
// decisione si disegnerebbe un rettangolo verde su quella cella, più la linea che ci porta.
//
// ⌫ **La verifica di chiusura del 2026-09-24 aveva classificato `bDashing` come GUARDIA**, sulla fede
// del commento del campo. Era sbagliato, e l'ha trovato una sfida adversariale andando a cercare *chi
// azzera la cella*: nessuno. Un payload opzionale non è leggibile a flag spento; questo lo è, ed è
// attivamente sbagliato. È la stessa forma di `bMoving`, già giudicata decisione.
//
// 🔑 **E la traiettoria è una regola sostanziale, non trigonometria** (#142): una mobilità **lineare** va
// dritta e non gira gli angoli, una a budget segue il grafo. Disegnare l'A* per uno scatto lineare
// mostrerebbe un percorso curvo attorno a un ostacolo che in realtà lo ferma.
//
// ⚠️ Questo è l'**unico sito di `URTMovementActionLibrary::IsLinear` che decide un disegno**: gli altri
// stanno nel controller, nei bot e nella query tattica — cioè dal lato che la regola la *esegue*. Se le
// due letture divergessero, l'anteprima prometterebbe una traiettoria che la risoluzione non fa.
//
// ── TABELLA DELLE ATTESE DI MUTAZIONE ────────────────────────────────────────────────────────────────
// Scritta prima di lanciare ed **eseguibile**: ogni riga nomina un test che esiste, parte dal codice
// spedito, e descrive una mutazione che compila.
//
//   # | mutazione su `ARTHUD::ComposeDashPreview`                  | rosso atteso        | esito
//  ---|-------------------------------------------------------------|---------------------|----------
//   1 | `Out.bShow = View.bDashing;` → `= true;`                     | …IsHidden…          | 1, esatto
//   2 | `Out.bShow = View.bDashing;` → `= !View.bDashing;`           | …IsHidden… + Linear…| 4 rossi
//   3 | `IsLinear(View.DashStyle)` → `!IsLinear(…)`                  | Linear… + Budget…   | 3 rossi †
//
// 🔴 **† La 3 è SOPRAVVISSUTA alla prima esecuzione, ed è la ragione per cui questo file costruisce una
// mappa con un ostacolo.** La fixture era una `MakeFlatArena` nuda, e su una piana libera
// `HexLine(0,0 → 3,0)` e `FindPath(0,0 → 3,0)` rendono lo **stesso** percorso: invertire il ramo non
// cambiava niente, e i due test principali erano **tautologici**. Bloccando `(2,0,0)` i rami divergono
// — la retta ci passa sopra, il grafo lo aggira — e con quello la mutazione cade.
//
// È lo stesso difetto della fetta 6 (una soglia misurata da un lato solo) in forma nuova: **una
// distinzione misurata su un input che non la esprime**.
//
// ⚠️ La 2 produce quattro rossi invece dei due attesi: invertire `bShow` rompe ogni proprietà del file.
// ⛔ Ogni build di mutazione ha dato `Result: Succeeded` prima della run, e ogni run ha usato il filtro
// `RefactorTactics.HUD` intero.

#include "Misc/AutomationTest.h"
#include "UI/RTHUD.h"
#include "Turn/RTIntentPrivacyLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTMovementActionLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"
#include "Pathfinding/RTHexPathLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/**
	 * Arena di raggio 4 **con un ostacolo sulla retta**, costruita senza mondo.
	 *
	 * 🔴 **L'ostacolo non e' decorativo: senza, questi test sono TAUTOLOGICI.** Su una piana libera
	 * `HexLine(0,0 → 3,0)` e `FindPath(0,0 → 3,0)` rendono lo **stesso** percorso, quindi invertire il
	 * ramo lineare non fa cadere niente — misurato con la mutazione 3, che sopravviveva. Bloccando
	 * `(2,0,0)` i due rami divergono: la retta ci passa sopra, il grafo lo aggira.
	 */
	URTHexMapAsset* MakeDashTestMap()
	{
		URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 4);
		FRTHexCellData Muro = Map->FindCell(FRTCellId(2, 0, 0))
			? *Map->FindCell(FRTCellId(2, 0, 0))
			: FRTHexCellData(FRTCellId(2, 0, 0));
		Muro.bBlocksMovement = true;
		Map->AddOrUpdateCell(Muro);
		Map->SortCells();
		return Map;
	}
}

/**
 * 🔴 **Senza uno scatto pianificato non si disegna niente — benché la cella di scatto ci sia.**
 *
 * È il caso reale del turno dopo: l'abilità è stata consumata, la destinazione no.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudDashPreviewHiddenTest,
	"RefactorTactics.HUD.DashPreviewIsHiddenWithoutAPlannedDash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudDashPreviewHiddenTest::RunTest(const FString&)
{
	FRTIntentView Consumato;
	Consumato.bDashing = false;
	Consumato.OwnerCell = FRTCellId(0, 0, 0);

	// ⛔ La cella RESTA valorizzata: è esattamente ciò che il turn manager lascia indietro, e la
	// condizione in cui la lettura ingenua disegnerebbe un rettangolo su una meta già raggiunta.
	Consumato.DashCell = FRTCellId(3, 0, 0);
	Consumato.DashStyle = ERTMovementStyle::LinearDash;

	const FRTDashPreview Scatto = ARTHUD::ComposeDashPreview(Consumato, MakeDashTestMap());

	TestFalse(TEXT("⛔ niente anteprima: l'abilita' e' stata consumata, non la cella"), Scatto.bShow);
	TestEqual(TEXT("e nessuna traiettoria da disegnare"), Scatto.PathCells.Num(), 0);

	return true;
}

/**
 * 🔑 **Uno scatto LINEARE va dritto, e non gira gli angoli** (#142).
 *
 * La traiettoria è quella che la fase Dash eseguirà: se qui uscisse l'A*, il giocatore vedrebbe un
 * percorso curvo attorno a un ostacolo che in realtà lo ferma contro il muro.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudDashPreviewLinearTest,
	"RefactorTactics.HUD.LinearDashGoesStraight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudDashPreviewLinearTest::RunTest(const FString&)
{
	FRTIntentView Lineare;
	Lineare.bDashing = true;
	Lineare.OwnerCell = FRTCellId(0, 0, 0);
	Lineare.DashCell = FRTCellId(3, 0, 0);
	Lineare.DashStyle = ERTMovementStyle::LinearDash;

	// ⛔ Precondizione dichiarata: se `IsLinear` smettesse di riconoscere questo stile, il test
	// misurerebbe il ramo sbagliato senza accorgersene.
	if (!TestTrue(TEXT("precondizione: LinearDash e' lineare"),
		URTMovementActionLibrary::IsLinear(ERTMovementStyle::LinearDash)))
	{
		return false;
	}

	const FRTDashPreview Scatto = ARTHUD::ComposeDashPreview(Lineare, MakeDashTestMap());

	TestTrue(TEXT("lo scatto pianificato si mostra"), Scatto.bShow);

	// La retta esagonale: passa PER la cella bloccata, perche' uno scatto lineare non gira gli angoli.
	const TArray<FRTCellId> Retta = URTHexLibrary::HexLine(FRTCellId(0, 0, 0), FRTCellId(3, 0, 0));
	if (!TestEqual(TEXT("la traiettoria e' la RETTA"), Scatto.PathCells.Num(), Retta.Num()))
	{
		return false;
	}
	for (const FRTCellId& C : Retta)
	{
		TestTrue(TEXT("e ne contiene ogni cella"), Scatto.PathCells.Contains(C));
	}

	// ⛔ **L'asserzione che distingue i due rami**: la retta ATTRAVERSA l'ostacolo. Un ricalcolo sul
	// grafo lo aggirerebbe, e senza questa riga il test passerebbe con entrambi i rami — che e'
	// esattamente come la mutazione 3 e' sopravvissuta alla prima esecuzione.
	TestTrue(TEXT("⛔ e passa PER la cella bloccata: uno scatto lineare non gira gli angoli (#142)"),
		Scatto.PathCells.Contains(FRTCellId(2, 0, 0)));

	return true;
}

/**
 * Uno scatto **a budget** segue il grafo, cioè la stessa risposta che il movimento userà.
 *
 * ⚠️ Il confronto è con `FindPath`, non con un elenco scritto a mano: un'attesa cablata si romperebbe
 * al primo cambio del costo del terreno, e misurerebbe l'A* invece della scelta fra i due rami.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudDashPreviewBudgetTest,
	"RefactorTactics.HUD.BudgetDashFollowsTheGraph",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudDashPreviewBudgetTest::RunTest(const FString&)
{
	URTHexMapAsset* Mappa = MakeDashTestMap();

	FRTIntentView ABudget;
	ABudget.bDashing = true;
	ABudget.OwnerCell = FRTCellId(0, 0, 0);
	ABudget.DashCell = FRTCellId(3, 0, 0);
	ABudget.DashStyle = ERTMovementStyle::Budget;

	if (!TestFalse(TEXT("precondizione: Budget NON e' lineare"),
		URTMovementActionLibrary::IsLinear(ERTMovementStyle::Budget)))
	{
		return false;
	}

	const FRTDashPreview Scatto = ARTHUD::ComposeDashPreview(ABudget, Mappa);

	TestTrue(TEXT("si mostra"), Scatto.bShow);

	const TArray<FRTCellId> DalGrafo =
		URTHexPathLibrary::FindPath(Mappa, FRTCellId(0, 0, 0), FRTCellId(3, 0, 0)).Path;
	TestEqual(TEXT("la traiettoria e' quella del GRAFO"), Scatto.PathCells.Num(), DalGrafo.Num());

	// ⛔ **L'asserzione simmetrica**: il grafo AGGIRA l'ostacolo, la retta no. Le due insieme fissano la
	// scelta fra i rami da entrambi i lati — una sola la lascerebbe soddisfatta da tutti e due.
	TestFalse(TEXT("⛔ e NON passa per la cella bloccata: il grafo la aggira"),
		Scatto.PathCells.Contains(FRTCellId(2, 0, 0)));
	TestTrue(TEXT("percio' e' piu' lunga della retta"),
		Scatto.PathCells.Num() > URTHexLibrary::HexLine(FRTCellId(0, 0, 0), FRTCellId(3, 0, 0)).Num());

	return true;
}

/**
 * Senza mappa il ramo a budget resta a mani vuote — e non esplode.
 *
 * ⚠️ È il caso che `DrawHUD` può incontrare (`ARTHexMapActor::FindInWorld` può non trovare nulla), e chi
 * disegna tiene comunque la propria guardia `&& Map`: qui si verifica che la funzione **risponda**
 * invece di presupporre.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudDashPreviewNullMapTest,
	"RefactorTactics.HUD.BudgetDashWithoutAMapIsEmptyNotACrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudDashPreviewNullMapTest::RunTest(const FString&)
{
	FRTIntentView V;
	V.bDashing = true;
	V.OwnerCell = FRTCellId(0, 0, 0);
	V.DashCell = FRTCellId(3, 0, 0);
	V.DashStyle = ERTMovementStyle::Budget;

	const FRTDashPreview Scatto = ARTHUD::ComposeDashPreview(V, nullptr);

	TestTrue(TEXT("la decisione di mostrare non dipende dalla mappa"), Scatto.bShow);
	TestEqual(TEXT("ma senza grafo non c'e' traiettoria"), Scatto.PathCells.Num(), 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
