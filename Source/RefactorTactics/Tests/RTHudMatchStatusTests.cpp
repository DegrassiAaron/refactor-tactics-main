// La barra di stato in alto: contatore del round, fase o riproduzione, timer, progresso sull'obiettivo.
//
// Perché esiste (#2184): la riga viveva dentro `ARTHUD::DrawHUD`, che il motore chiama ogni fotogramma e
// che **non ha copertura headless** — quindi ogni sua regola era verificabile solo a occhio. Quattro
// decisioni ci abitavano, e tre di esse sono reticenze: cosa NON scrivere quando un dato non si applica.
// Sono precisamente quelle che un controllo a schermo non vede, perché l'assenza di un pezzo di testo non
// somiglia a un difetto.
//
//   1. il contatore mostra il limite solo se il formato ne dichiara uno — mai «/0»;
//   2. durante la riproduzione la riga dice fase e percentuale, e come saltarla;
//   3. il conto alla rovescia compare solo in Pianificazione e solo se un timer esiste;
//   4. l'obiettivo compare solo se la mappa ne dichiara uno, e la soglia solo se il formato la fissa.
//
// ⚠️ **La funzione prende `FRTMatchHeaderView` e non l'attore**, che è l'altra metà del punto: la vista è
// già prodotta da `URTHudViewModel::BuildMatchHeader` per lo Screen HUD, e `DrawHUD` la ricostruiva a mano
// campo per campo. Due sedi per lo stesso dato si scollegano al primo campo che se ne aggiunge a una sola.
//
// ⛔ Non si verifica qui che i numeri siano giusti: quelli vengono dalla vista, e `RTHudViewModelTests` è
// già il loro proprietario. Qui si verifica **cosa la riga scrive**, dato ciò che la vista porta.

#include "Misc/AutomationTest.h"
#include "UI/RTHUD.h"
#include "UI/RTHudViewModel.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Il contatore nomina il limite solo quando il formato ne ha uno.
 *
 * `RoundLimit == 0` significa «nessun limite dichiarato», e un «Round 3/0» direbbe che la partita è già
 * scaduta — la stessa distinzione che `FRTMatchHeaderView::RoundLimit` dichiara per lo Screen HUD.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudMatchStatusRoundCounterTest,
	"RefactorTactics.HUD.MatchStatusRoundCounterNeverShowsZeroLimit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudMatchStatusRoundCounterTest::RunTest(const FString&)
{
	FRTMatchHeaderView View;
	View.Round = 3;
	View.Phase = ERTMatchPhase::MatchEnded;

	View.RoundLimit = 12;
	TestEqual(TEXT("con un formato in vigore il limite si vede"),
		ARTHUD::ComposeMatchStatusLine(View, FString(), 0.f, /*bHasObjectiveCell=*/ false),
		FString(TEXT("Round 3/12  -  Fine")));

	// Senza formato il limite sparisce del tutto: non diventa zero.
	View.RoundLimit = 0;
	const FString NoFormat = ARTHUD::ComposeMatchStatusLine(View, FString(), 0.f, /*bHasObjectiveCell=*/ false);
	TestEqual(TEXT("senza formato resta il solo numero"), NoFormat, FString(TEXT("Round 3  -  Fine")));
	TestFalse(TEXT("nessun «/0» da nessuna parte"), NoFormat.Contains(TEXT("/0")));

	return true;
}

/**
 * In riproduzione la riga dice quale fase sta scorrendo, quanto ne manca e come saltarla.
 *
 * La percentuale si arrotonda **al più vicino**, non per troncamento: `0.336` è 34%, non 33%. Il numero è
 * l'unico segnale di avanzamento che il giocatore ha mentre non può agire.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudMatchStatusPlaybackTest,
	"RefactorTactics.HUD.MatchStatusShowsPlaybackProgressWhileResolving",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudMatchStatusPlaybackTest::RunTest(const FString&)
{
	FRTMatchHeaderView View;
	View.Round = 2;
	View.bResolving = true;

	TestEqual(TEXT("fase in riproduzione, avanzamento e via d'uscita"),
		ARTHUD::ComposeMatchStatusLine(View, FString(TEXT("Blast")), 0.5f, /*bHasObjectiveCell=*/ false),
		FString(TEXT("Round 2  -  Risoluzione: Blast  [50%]  (Spazio: salta)")));

	TestTrue(TEXT("la percentuale si arrotonda al piu' vicino"),
		ARTHUD::ComposeMatchStatusLine(View, FString(TEXT("Move")), 0.336f, false).Contains(TEXT("[34%]")));

	// In riproduzione la fase del modello non si nomina: quella che scorre è un'altra, e nominarle entrambe
	// direbbe che il gioco è in due fasi insieme.
	View.Phase = ERTMatchPhase::Planning;
	TestFalse(TEXT("la fase del modello non compare durante il playback"),
		ARTHUD::ComposeMatchStatusLine(View, FString(TEXT("Blast")), 0.5f, false).Contains(TEXT("Pianificazione")));

	return true;
}

/**
 * Fuori dalla riproduzione la riga nomina la fase, e aggiunge il conto alla rovescia **solo** in
 * Pianificazione e **solo** se un timer esiste davvero.
 *
 * ⚠️ Il timer si arrotonda per ECCESSO: a 3,2 secondi residui mostra `4s`. Mostrare `3s` farebbe sparire
 * l'ultimo secondo, e il giocatore vedrebbe scadere un tempo che credeva di avere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudMatchStatusPhaseTest,
	"RefactorTactics.HUD.MatchStatusShowsCountdownOnlyWhenPlanningHasATimer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudMatchStatusPhaseTest::RunTest(const FString&)
{
	FRTMatchHeaderView View;
	View.Round = 1;
	View.Phase = ERTMatchPhase::Planning;

	View.PlanningSecondsRemaining = 3.2f;
	TestEqual(TEXT("in Pianificazione con timer: i secondi arrotondati per eccesso"),
		ARTHUD::ComposeMatchStatusLine(View, FString(), 0.f, false),
		FString(TEXT("Round 1  -  Pianificazione  -  4s")));

	// Negativo = «la domanda non si applica»: nessun timer impostato. Una partita headless gira con
	// `SetPlanningSeconds(0)`, e un «0s» lampeggiante direbbe che il tempo è appena scaduto.
	View.PlanningSecondsRemaining = -1.f;
	TestEqual(TEXT("in Pianificazione senza timer: nessun conto alla rovescia"),
		ARTHUD::ComposeMatchStatusLine(View, FString(), 0.f, false),
		FString(TEXT("Round 1  -  Pianificazione")));

	View.Phase = ERTMatchPhase::MatchEnded;
	TestEqual(TEXT("a partita finita: «Fine»"),
		ARTHUD::ComposeMatchStatusLine(View, FString(), 0.f, false),
		FString(TEXT("Round 1  -  Fine")));

	// Ogni altra fase del turno è, per chi guarda, «Risoluzione»: le fasi interne hanno nomi che il
	// giocatore non ha mai visto altrove.
	View.Phase = ERTMatchPhase::Blast;
	TestEqual(TEXT("una fase interna si legge come Risoluzione"),
		ARTHUD::ComposeMatchStatusLine(View, FString(), 0.f, false),
		FString(TEXT("Round 1  -  Risoluzione")));

	return true;
}

/**
 * Il progresso sull'obiettivo compare **solo se la mappa ne dichiara uno**, e la soglia solo se il formato
 * la fissa.
 *
 * Su una mappa senza obiettivo `0-0` non è una parità: è un punteggio inventato per una gara che nessuno
 * sta correndo. È la stessa reticenza che il resolver ha già — nessuna voce di TurnLog senza
 * `HasObjectiveCell()`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudMatchStatusObjectiveTest,
	"RefactorTactics.HUD.MatchStatusShowsObjectiveOnlyWhenMapDeclaresOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudMatchStatusObjectiveTest::RunTest(const FString&)
{
	FRTMatchHeaderView View;
	View.Round = 1;
	View.Phase = ERTMatchPhase::MatchEnded;
	View.Team0Score = 2;
	View.Team1Score = 1;

	TestEqual(TEXT("mappa senza obiettivo: nessun punteggio, nemmeno 0-0"),
		ARTHUD::ComposeMatchStatusLine(View, FString(), 0.f, /*bHasObjectiveCell=*/ false),
		FString(TEXT("Round 1  -  Fine")));

	// Con obiettivo ma senza soglia (la via per obiettivo è disattivata in v0.1): il progresso si vede, il
	// «a quanto» tace. Un «(a 0)» leggerebbe come «già vinta».
	TestEqual(TEXT("con obiettivo e senza soglia: solo il progresso"),
		ARTHUD::ComposeMatchStatusLine(View, FString(), 0.f, /*bHasObjectiveCell=*/ true),
		FString(TEXT("Round 1  -  Fine  -  Obiettivo 2-1")));

	View.ScoreToWin = 3;
	TestEqual(TEXT("con soglia dichiarata: anche il traguardo"),
		ARTHUD::ComposeMatchStatusLine(View, FString(), 0.f, /*bHasObjectiveCell=*/ true),
		FString(TEXT("Round 1  -  Fine  -  Obiettivo 2-1 (a 3)")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
