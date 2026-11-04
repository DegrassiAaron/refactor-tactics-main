// I cancelli di fase e il segno di selezione di `ARTHUD::DrawHUD` (#2184, fetta 13).
//
// Perché esiste: due regole vivevano dentro `DrawHUD`, che il motore chiama ogni fotogramma e che **non ha
// copertura headless**. Erano le due decisioni che la verifica di chiusura del 2026-09-25 aveva lasciato
// nel metodo come "da una riga": non toglievano righe, e per questo restavano.
//
// 1. **Quali strati dipendono dalla fase.** Tre confronti scritti a mano su `TurnManager->GetPhase()`, in tre
//    siti diversi: la traccia dell'ultima risoluzione, gli intenti pianificati, il pannello di fine partita.
//    🔑 **Non sono lo stesso confronto, ed è la regola che il censimento voleva pinnata**: la traccia resta
//    visibile durante il playback, gli intenti no.
//    ⚠️ **È lo stato di fatto, non una regola dichiarata desiderabile**: `D-356` elenca la traccia sopra un turno
//    in corso fra i difetti e la chiude per la sola sospensione (con `Phase` a `Blast`/`Move` il `== Planning` è
//    falso). Se il playback ordinario debba nasconderla non lo decide questo file, che la registra com'è: chi
//    la cambia deve cambiare anche il test, e saperlo. La regola, prima, non era interrogabile da nessun test.
// 2. **Quando un intento è "quello che sto pianificando".** `bHasSelection && View.OwnerCell == SelectedCell`
//    è l'unico produttore del flag che sceglie fra `[PIANO *]` e `[PIANO]`, cioè il testo. `ComposeIntentPresentation`
//    lo riceve come parametro e per progetto non sa da dove venga, quindi la risposta non aveva una sede pura:
//    nessun test cadeva se il flag diventava `= bHasSelection`, e ogni unità propria si sarebbe marcata come
//    quella in pianificazione.
//
// ⚠️ **Cosa NON copre questo file: il cablaggio.** Che ciascuno dei tre siti di `DrawHUD` legga il SUO cancello
// (e non quello di un altro) si vede solo in PIE, e quella voce è `NOT RUN` in questa fetta. Le funzioni sono
// pinnate; la riga che le chiama no, perché `DrawHUD` non è raggiungibile headless.
//
// ⚠️ **`bResolving` non è una fase.** `ARTTurnManager::IsResolving()` è "il playback della risoluzione è in
// corso", e durante il playback `GetPhase()` può già dire `Planning`: per questo il cancello degli intenti
// vuole entrambi. La tabella sotto è scritta a mano, per fase e per valore di `bResolving`, e non è ricavata
// dalla funzione.
//
// ── TABELLA DELLE ATTESE DI MUTAZIONE ────────────────────────────────────────
// Scritta prima di lanciare ed **eseguibile**: ogni riga nomina un test che esiste, parte dal codice spedito,
// e descrive una mutazione che compila.
//
//   # | mutazione                                                         | rosso atteso            | esito
//  ---|-------------------------------------------------------------------|-------------------------|--------
//   1 | traccia: `Phase == Planning` → `Phase != MatchEnded`               | PostLockTrail…          | UCCISA: 1 rosso, il previsto
//   2 | traccia: aggiunto `&& !bResolving`                                 | PostLockTrail…          | UCCISA: 1 rosso, il previsto
//   3 | intenti: tolto `&& !bResolving`                                    | PlannedIntents…         | UCCISA: 1 rosso, il previsto
//   4 | intenti: `!bResolving` → `bResolving`                              | PlannedIntents…         | UCCISA: 1 rosso, il previsto
//   5 | pannello: `Phase == MatchEnded` → `Phase != Planning`              | MatchEndPanel…          | UCCISA: 1 rosso, il previsto
//   6 | pannello: aggiunto `&& !bResolving`                                | MatchEndPanel…          | UCCISA: 1 rosso, il previsto
//   7 | selezione: `bHasSelection && Owner == Sel` → `bHasSelection`       | IntentIsMarked…         | UCCISA: 1 rosso, il previsto
//   8 | selezione: → `Owner == Sel` (senza `bHasSelection`)                | IntentIsMarked…         | UCCISA: 1 rosso, il previsto
//   9 | selezione: `&&` → `||`                                             | IntentIsMarked…         | UCCISA: 1 rosso, il previsto
//  10 | selezione: confronta solo `X` e `Y` (ignora il layer)              | IntentIsMarked…         | UCCISA: 1 rosso, il previsto
//  11 | default: `bShowPlannedIntents = false` → `= true` (in `RTHUD.h`)   | DefaultGatesAre…        | UCCISA: 1 rosso, il previsto
//
// ⛔ Ogni build di mutazione ha dato `Result: Succeeded` prima della run. Il filtro di ogni run era
// `RefactorTactics.HUD.PhaseGates` per le righe 1–6 e `RefactorTactics.HUD.IntentIsMarked` per le righe 7–10, e i test
// trovati erano quanti in baseline. Un filtro stretto dice che il test atteso cade, non che non cada altro in `HUD`.
// La riga 11 è stata aggiunta dopo la revisione indipendente, col test `DefaultGatesAreAllClosed` e lo stesso filtro
// `PhaseGates`; la baseline di quel giro conta un test in più. La riga 11 è misurata il 2026-11-04 su `da0de90f0`
// (dopo il merge di `main`): 4 test trovati col filtro `PhaseGates`, 1 rosso, `DefaultGatesAreAllClosed`; il controllo senza
// mutante sul filtro `HUD` intero è verde prima e dopo.

#include "Misc/AutomationTest.h"
#include "UI/RTHUD.h"
#include "Turn/RTTurnRules.h"
#include "Map/RTCellId.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Una riga della tabella: fase, "playback in corso", e cosa ciascun cancello deve rispondere. */
	struct FPhaseGateRow
	{
		ERTMatchPhase Phase;
		bool bResolving;
		bool bTrail;
		bool bIntents;
		bool bEndPanel;
	};

	// Scritta a mano, una riga per ogni coppia (fase, bResolving) di `ERTMatchPhase`: Planning, Prep, Dash,
	// Blast, Move, Cleanup, MatchEnded. Se l'enum cresce la tabella va estesa a mano: nessun asserto lo segnala.
	const FPhaseGateRow PhaseGateRows[] =
	{
		{ ERTMatchPhase::Planning,   false, true,  true,  false },
		{ ERTMatchPhase::Planning,   true,  true,  false, false }, // il playback: la traccia resta, gli intenti spariscono
		{ ERTMatchPhase::Prep,       false, false, false, false },
		{ ERTMatchPhase::Prep,       true,  false, false, false },
		{ ERTMatchPhase::Dash,       false, false, false, false },
		{ ERTMatchPhase::Dash,       true,  false, false, false },
		{ ERTMatchPhase::Blast,      false, false, false, false },
		{ ERTMatchPhase::Blast,      true,  false, false, false },
		{ ERTMatchPhase::Move,       false, false, false, false },
		{ ERTMatchPhase::Move,       true,  false, false, false },
		{ ERTMatchPhase::Cleanup,    false, false, false, false },
		{ ERTMatchPhase::Cleanup,    true,  false, false, false },
		{ ERTMatchPhase::MatchEnded, false, false, false, true  },
		{ ERTMatchPhase::MatchEnded, true,  false, false, true  }, // a partita finita il pannello non dipende dal playback
	};

	/** I tre cancelli non si confondono: si controlla UNA colonna alla volta, cosi' una mutazione cade su un test solo. */
	void PhaseGateCheck(FAutomationTestBase& Test, const TCHAR* Gate, bool FRTHudPhaseGates::* Member,
		bool FPhaseGateRow::* Expected)
	{
		for (const FPhaseGateRow& Row : PhaseGateRows)
		{
			const FRTHudPhaseGates Gates = ARTHUD::ComposeHudPhaseGates(Row.Phase, Row.bResolving);
			Test.TestEqual(
				*FString::Printf(TEXT("%s con fase %d e playback %d"), Gate, static_cast<int32>(Row.Phase),
					Row.bResolving ? 1 : 0),
				(Gates.*Member) ? 1 : 0, (Row.*Expected) ? 1 : 0);
		}
	}
}

/**
 * La traccia dell'ultima risoluzione si mostra in `Planning`, ANCHE mentre il playback gira. Non vuole
 * `!bResolving`: e' cio' che la distingue dal cancello degli intenti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudPhaseGatePostLockTrailTest,
	"RefactorTactics.HUD.PhaseGates.PostLockTrailShowsOnlyInPlanningEvenDuringPlayback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudPhaseGatePostLockTrailTest::RunTest(const FString&)
{
	PhaseGateCheck(*this, TEXT("traccia"), &FRTHudPhaseGates::bShowPostLockTrail, &FPhaseGateRow::bTrail);
	return true;
}

/**
 * Gli intenti pianificati si mostrano in `Planning` e NON durante il playback: si vede cio' che e' successo,
 * non cio' che era stato pianificato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudPhaseGatePlannedIntentsTest,
	"RefactorTactics.HUD.PhaseGates.PlannedIntentsShowOnlyInPlanningAndHideDuringPlayback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudPhaseGatePlannedIntentsTest::RunTest(const FString&)
{
	PhaseGateCheck(*this, TEXT("intenti"), &FRTHudPhaseGates::bShowPlannedIntents, &FPhaseGateRow::bIntents);
	return true;
}

/** Il pannello di fine partita si mostra solo a partita finita, qualunque cosa dica il playback. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudPhaseGateMatchEndPanelTest,
	"RefactorTactics.HUD.PhaseGates.MatchEndPanelShowsOnlyWhenTheMatchHasEnded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudPhaseGateMatchEndPanelTest::RunTest(const FString&)
{
	PhaseGateCheck(*this, TEXT("pannello"), &FRTHudPhaseGates::bShowMatchEndPanel, &FPhaseGateRow::bEndPanel);
	return true;
}

/**
 * Senza un `ARTTurnManager` `DrawHUD` usa il default di `FRTHudPhaseGates`: nessuno dei tre strati ha qualcosa da
 * mostrare. Oggi e' inerte — i siti tengono `TurnManager &&` perche' lo leggono nel corpo — ed e' pinnato lo
 * stesso: se un giorno un sito perde quella guardia, il default e' tutto cio' che lo separa da un puntatore nullo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudPhaseGateDefaultTest,
	"RefactorTactics.HUD.PhaseGates.DefaultGatesAreAllClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudPhaseGateDefaultTest::RunTest(const FString&)
{
	const FRTHudPhaseGates Chiusi = FRTHudPhaseGates();
	TestFalse(TEXT("traccia chiusa di default"), Chiusi.bShowPostLockTrail);
	TestFalse(TEXT("intenti chiusi di default"), Chiusi.bShowPlannedIntents);
	TestFalse(TEXT("pannello chiuso di default"), Chiusi.bShowMatchEndPanel);
	return true;
}

/**
 * Un intento e' "quello che sto pianificando" solo se esiste una selezione E il suo proprietario sta nella
 * cella selezionata. Si identifica per CELLA e non per pointer: la vista non sa nulla di `ARTUnit`.
 *
 * 🔴 **Il caso che fa cadere `= OwnerCell == SelectedCell`** e' l'assenza di selezione: la cella "selezionata"
 * allora e' quella di default, `(0,0,0)`, e un'unita' proprietaria di quella cella si marcherebbe come quella in
 * pianificazione in autobattle, dove nessuno seleziona niente. Per questo il test usa proprio `(0,0,0)`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudIntentSelectionTest,
	"RefactorTactics.HUD.IntentIsMarkedSelectedOnlyForTheSelectedCellAndOnlyWithASelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudIntentSelectionTest::RunTest(const FString&)
{
	// Premessa: il default di FRTCellId e' davvero (0,0,0), altrimenti il caso sotto non prova quel che dice.
	TestTrue(TEXT("premessa: la cella di default e' (0,0,0)"), FRTCellId() == FRTCellId(0, 0, 0));

	const FRTCellId Selected(2, -1, 0);

	TestTrue(TEXT("con una selezione, l'intento del proprietario della cella selezionata e' marcato"),
		ARTHUD::ComposeIntentIsSelected(true, Selected, FRTCellId(2, -1, 0)));

	TestFalse(TEXT("con una selezione, l'intento di un'altra cella non e' marcato"),
		ARTHUD::ComposeIntentIsSelected(true, Selected, FRTCellId(3, -1, 0)));

	TestFalse(TEXT("senza selezione, l'intento nella cella di default NON e' marcato"),
		ARTHUD::ComposeIntentIsSelected(false, FRTCellId(), FRTCellId(0, 0, 0)));

	TestFalse(TEXT("senza selezione nessun intento e' marcato, nemmeno se la cella coincide con quella 'ricordata'"),
		ARTHUD::ComposeIntentIsSelected(false, Selected, FRTCellId(2, -1, 0)));

	TestFalse(TEXT("lo stesso (X, Y) su un altro layer e' un'altra cella"),
		ARTHUD::ComposeIntentIsSelected(true, Selected, FRTCellId(2, -1, 1)));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
