// La barra di stato in alto: contatore del round, fase o riproduzione, timer, progresso sull'obiettivo.
//
// Perché esiste (#2184): la riga viveva dentro `ARTHUD::DrawHUD`, che il motore chiama ogni fotogramma e
// che **non ha copertura headless** — quindi ogni sua regola era verificabile solo a occhio. Quattro
// decisioni ci abitavano, e tutte e quattro sono reticenze: cosa NON scrivere quando un dato non si
// applica. Sono precisamente quelle che un controllo a schermo non vede, perché l'assenza di un pezzo di
// testo non somiglia a un difetto.
//
//   1. il contatore mostra il limite solo se il formato ne dichiara uno — mai «/0»;
//   2. durante la riproduzione la riga dice fase e percentuale, e come saltarla;
//   3. il conto alla rovescia compare solo in Pianificazione e solo se un timer esiste;
//   4. l'obiettivo compare solo se la mappa ne dichiara uno, e la soglia solo se il formato la fissa.
//
// ⚠️ **`TestEqualSensitive` e non `TestEqual`.** `FAutomationTestBase::TestEqual` su stringhe passa da
// `FCString::Stricmp` (`AutomationTest.cpp:2163`): è case-INSENSITIVE, quindi con `TestEqual` un
// `Fine` → `FINE` o un `Obiettivo` → `obiettivo` regredirebbe in PIE lasciando questi test verdi — cioè
// esattamente il buco che questo file esiste per chiudere.
//
// ⚠️ **La funzione prende `FRTMatchHeaderView` e non l'attore**, che è l'altra metà del punto: la vista è
// già prodotta da `URTHudViewModel::BuildMatchHeader` per lo Screen HUD, e `DrawHUD` la ricostruiva a mano
// campo per campo. Due sedi per lo stesso dato si scollegano al primo campo che se ne aggiunge a una sola.
//
// ⛔ Non si verifica qui che i numeri siano giusti: quelli vengono dalla vista, e `RTHudViewModelTests` è
// già il loro proprietario. Qui si verifica **cosa la riga scrive**, dato ciò che la vista porta.
//
// ⚠️ **Le uguaglianze esatte qui sotto pinnano un PREFISSO di ciò che si vede a schermo, non la riga
// intera.** `DrawHUD` appende dopo il segmento della velocità di riproduzione (`  -  Velocita': x1 (V)`),
// che resta fuori da questa funzione perché `screen-hud-umg-2026-08-26.md` lo classifica come **debug con
// regole proprie**, separato da round, fase e timer — e perché ha già la sua statica pura,
// `ARTHUD::ComposePlaybackSpeedLabel`, con i test di `RTHudPlaybackSpeedTests`. Una regressione nella
// concatenazione fra i due pezzi non la vede nessuno dei due file: e' il buco dichiarato di questa fetta.

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
	TestEqualSensitive(TEXT("con un formato in vigore il limite si vede"),
		ARTHUD::ComposeMatchStatusLine(View, FString(), 0.f, /*bHasObjectiveCell=*/ false),
		FString(TEXT("Round 3/12  -  Fine")));

	// Senza formato il limite sparisce del tutto: non diventa zero.
	View.RoundLimit = 0;
	const FString NoFormat = ARTHUD::ComposeMatchStatusLine(View, FString(), 0.f, /*bHasObjectiveCell=*/ false);
	TestEqualSensitive(TEXT("senza formato resta il solo numero"), NoFormat, FString(TEXT("Round 3  -  Fine")));
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

	TestEqualSensitive(TEXT("fase in riproduzione, avanzamento e via d'uscita"),
		ARTHUD::ComposeMatchStatusLine(View, FString(TEXT("Blast")), 0.5f, /*bHasObjectiveCell=*/ false),
		FString(TEXT("Round 2  -  Risoluzione: Blast  [50%]  (Spazio: salta)")));

	TestTrue(TEXT("la percentuale si arrotonda al piu' vicino"),
		ARTHUD::ComposeMatchStatusLine(View, FString(TEXT("Move")), 0.336f, false).Contains(TEXT("[34%]")));

	// ⚠️ **Fuori da `[0,1]` la percentuale si CLAMPA, e non è difensività generica.** La funzione è una
	// statica pubblica: la garanzia `[0,1]` era strutturale finché l'unico chiamante passava da
	// `ARTTurnManager::GetPlaybackProgress01()`, che clampa; ora un secondo chiamante — il viewer di
	// replay, che nomina già `FRTMatchHeaderView` come proprio modello — può passare un `Elapsed/Total`
	// grezzo. Senza clamp `FMath::RoundToInt` su x64 rende `0x80000000` per un NaN, e la barra stampa
	// `[-2147483648%]`.
	TestTrue(TEXT("oltre 1 la percentuale si ferma a 100"),
		ARTHUD::ComposeMatchStatusLine(View, FString(TEXT("Move")), 1.43f, false).Contains(TEXT("[100%]")));
	TestTrue(TEXT("sotto 0 la percentuale si ferma a 0"),
		ARTHUD::ComposeMatchStatusLine(View, FString(TEXT("Move")), -0.12f, false).Contains(TEXT("[0%]")));

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
	TestEqualSensitive(TEXT("in Pianificazione con timer: i secondi arrotondati per eccesso"),
		ARTHUD::ComposeMatchStatusLine(View, FString(), 0.f, false),
		FString(TEXT("Round 1  -  Pianificazione  -  4s")));

	// 🔴 **Il timer positivo FUORI dalla Pianificazione non si mostra, e senza questo caso metà della
	// regola che dà il nome al test resterebbe non misurata**: togliendo il congiunto
	// `Phase == Planning` dalla guardia, ogni altra asserzione qui resterebbe verde.
	View.Phase = ERTMatchPhase::MatchEnded;
	TestEqualSensitive(TEXT("timer positivo a partita finita: nessun conto alla rovescia"),
		ARTHUD::ComposeMatchStatusLine(View, FString(), 0.f, false),
		FString(TEXT("Round 1  -  Fine")));

	// ⚠️ **Zero non è «resta un secondo»: è «non si applica».** `PlanningSecondsRemaining` vale esattamente
	// `0.f` in una finestra REALE — `ARTTurnManager::BeginPlay` non arma il timer quando il primo turno lo
	// rivendica l'allestimento, e `ARTGameMode` lo apre ~350 ms dopo. In quel varco la fase è già Planning
	// e `PlanningSeconds` è 30, quindi `BuildMatchHeader` pubblica `0.f`. Con la guardia rilassata a
	// `>= 0.f` la barra mostrerebbe `0s` lampeggiante a ogni inizio partita.
	View.Phase = ERTMatchPhase::Planning;
	View.PlanningSecondsRemaining = 0.f;
	TestEqualSensitive(TEXT("timer a zero: «non si applica», non «scaduto adesso»"),
		ARTHUD::ComposeMatchStatusLine(View, FString(), 0.f, false),
		FString(TEXT("Round 1  -  Pianificazione")));

	// Negativo = la stessa cosa detta dalla vista quando la domanda non si pone affatto.
	View.PlanningSecondsRemaining = -1.f;
	TestEqualSensitive(TEXT("in Pianificazione senza timer: nessun conto alla rovescia"),
		ARTHUD::ComposeMatchStatusLine(View, FString(), 0.f, false),
		FString(TEXT("Round 1  -  Pianificazione")));

	View.Phase = ERTMatchPhase::MatchEnded;
	TestEqualSensitive(TEXT("a partita finita: «Fine»"),
		ARTHUD::ComposeMatchStatusLine(View, FString(), 0.f, false),
		FString(TEXT("Round 1  -  Fine")));

	// ⚠️ **Questo caso pinna il contratto della funzione, non uno stato che si vede.** `ARTTurnManager`
	// assegna `Phase` in due soli punti — il ciclo sincrono di `LockInAndResolve`, che non cede un
	// fotogramma, e `MatchEnded` — quindi `GetPhase()` è osservabile solo come `Planning` o `MatchEnded` e
	// il ramo `default:` non è raggiungibile dal percorso spedito. Resta perché la funzione è pubblica e
	// un secondo chiamante (il viewer di replay) può costruirsi la vista a mano.
	View.Phase = ERTMatchPhase::Blast;
	TestEqualSensitive(TEXT("una fase interna, se mai arrivasse, si legge come Risoluzione"),
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

	TestEqualSensitive(TEXT("mappa senza obiettivo: nessun punteggio, nemmeno 0-0"),
		ARTHUD::ComposeMatchStatusLine(View, FString(), 0.f, /*bHasObjectiveCell=*/ false),
		FString(TEXT("Round 1  -  Fine")));

	// Con obiettivo ma senza soglia (la via per obiettivo è disattivata in v0.1): il progresso si vede, il
	// «a quanto» tace. Un «(a 0)» leggerebbe come «già vinta».
	TestEqualSensitive(TEXT("con obiettivo e senza soglia: solo il progresso"),
		ARTHUD::ComposeMatchStatusLine(View, FString(), 0.f, /*bHasObjectiveCell=*/ true),
		FString(TEXT("Round 1  -  Fine  -  Obiettivo 2-1")));

	// 🔴 **`0-0` CON obiettivo dichiarato si mostra**, ed è il complemento della prima asserzione: lì è
	// inventato perché nessuno sta correndo la gara, qui è il punteggio vero di una gara in parità al
	// calcio d'inizio. Senza questo caso una guardia `(Team0Score > 0 || Team1Score > 0)` passerebbe la
	// suite e farebbe sparire la riga proprio all'inizio.
	View.Team0Score = 0;
	View.Team1Score = 0;
	TestEqualSensitive(TEXT("con obiettivo, 0-0 e' un punteggio vero e si mostra"),
		ARTHUD::ComposeMatchStatusLine(View, FString(), 0.f, /*bHasObjectiveCell=*/ true),
		FString(TEXT("Round 1  -  Fine  -  Obiettivo 0-0")));

	View.Team0Score = 2;
	View.Team1Score = 1;
	View.ScoreToWin = 3;
	TestEqualSensitive(TEXT("con soglia dichiarata: anche il traguardo"),
		ARTHUD::ComposeMatchStatusLine(View, FString(), 0.f, /*bHasObjectiveCell=*/ true),
		FString(TEXT("Round 1  -  Fine  -  Obiettivo 2-1 (a 3)")));

	// 🔴 **La soglia SENZA obiettivo dichiarato tace anche lei.** È la quarta reticenza, e senza questo
	// caso staccare `if (ScoreToWin > 0)` dalla guardia dell'obiettivo passerebbe la suite: su una mappa
	// senza obiettivo la barra scriverebbe `Round 1  -  Fine (a 3)` — un traguardo per una gara che
	// nessuno sta correndo. Un formato che dichiara la soglia esiste già: `RTHudViewModelTests` pinna
	// `ScoreToWin == 3`.
	TestEqualSensitive(TEXT("soglia dichiarata ma nessun obiettivo: tace anche il traguardo"),
		ARTHUD::ComposeMatchStatusLine(View, FString(), 0.f, /*bHasObjectiveCell=*/ false),
		FString(TEXT("Round 1  -  Fine")));

	// 🔴 **L'obiettivo si mostra ANCHE durante la riproduzione**, ed è il momento in cui conta di più: un
	// obiettivo conteso cambia mano proprio lì. Senza questo caso, spostare il blocco dentro il ramo
	// `else` passerebbe la suite e farebbe sparire il punteggio per tutta la risoluzione.
	View.bResolving = true;
	TestEqualSensitive(TEXT("in riproduzione l'obiettivo resta visibile"),
		ARTHUD::ComposeMatchStatusLine(View, FString(TEXT("Blast")), 0.5f, /*bHasObjectiveCell=*/ true),
		FString(TEXT("Round 1  -  Risoluzione: Blast  [50%]  (Spazio: salta)  -  Obiettivo 2-1 (a 3)")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
