#include "Misc/AutomationTest.h"
#include "Perception/RTTeamKnowledge.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h" // HashTurnLog: l'invariante che la proiezione non deve toccare
#include "Turn/RTCombatLog.h" // ComposeVisibleLogLines: il canale gemello da confrontare
#include "UI/RTPlayerEventProjector.h"
#include "Turn/RTReactionLibrary.h"        // #3734: ERTReactionOutcome, una reazione non scattata
#include "Ability/RTActionData.h"          // #3734: l'indice nel kit dello scatto e del getto di Muiren
#include "Ability/RTHeroCatalogLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexMapActor.h"
#include "Tests/RTWorldFixtures.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTActionEvent.h"         // #3734: FRTActionEffectSpec, la spinta che spinge e basta
#include "Turn/RTTurnManager.h"
#include "UI/RTHudViewModel.h"             // #3734: il feed dalla porta che il widget usa
#include "UI/RTScreenHudWidgets.h"         // #3734: e dal widget stesso, la porta di produzione
#include "Unit/RTUnit.h"

// La guardia: senza, questi test finiscono compilati DENTRO il binario Shipping. Vedi `#923`.
#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Una voce di TurnLog leggibile dalla squadra indicata. Nomi distinti: la unity build fonde le TU. */
	FRTTurnLogEntry PlayerEventEntry(ERTLogCategory Category, uint8 Outcome, int32 UnitId, int32 AllowedTeam)
	{
		FRTTurnLogEntry E;
		E.Category = Category;
		E.Outcome = Outcome;
		E.UnitId = UnitId;
		E.Verdict.AllowTeam(AllowedTeam);
		return E;
	}

	/** Quante volte un tipo compare nella proiezione. */
	int32 PlayerEventCountOf(const TArray<FRTPlayerEvent>& Events, ERTPlayerEventType Type)
	{
		int32 N = 0;
		for (const FRTPlayerEvent& E : Events) { if (E.Type == Type) { ++N; } }
		return N;
	}
}

/**
 * LO SCIVOLAMENTO ARRIVA AL FEED, E NON COME UN MOVIMENTO QUALUNQUE — `#2253`.
 *
 * 🔴 **Il difetto che questo test esiste per impedire e' una SPARIZIONE, e non fallisce a compilazione.**
 * `ClassifyEntry` traduce gli esiti di `Move` con uno `switch` che termina in `default: return false`:
 * un valore nuovo dell'enum non tradotto qui non rompe nulla — semplicemente non produce piu' l'evento
 * che produceva prima. Quando `Slid` e' stato aggiunto, lo scivolamento e' passato da `Moved`/`Minor` a
 * NESSUNA riga, e la suite e' rimasta verde perche' nessun test guardava questo caso.
 *
 * ⚠️ **`Important` e non `Minor`**: l'argomento di §D per cui un movimento riuscito e' minore — «il
 * giocatore lo vede gia' animato» — vale per un movimento CHIESTO. Qui l'unita' e' finita dove il
 * giocatore non l'aveva mandata, ed e' la stessa natura di `Displaced`: uno spostamento subito.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerEventSlideIsImportantTest,
	"RefactorTactics.UI.PlayerEventLog.SlideIsImportant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerEventSlideIsImportantTest::RunTest(const FString&)
{
	TArray<FRTTurnLogEntry> Log;
	Log.Add(PlayerEventEntry(ERTLogCategory::Move,
		static_cast<uint8>(ERTMoveOutcome::Slid), /*UnitId*/ 3, /*Team*/ 0));

	const TArray<FRTPlayerEvent> Events = URTPlayerEventProjector::Project(Log, /*ObserverTeamId*/ 0);

	// ANTI-VACUITA': senza questa riga il test passerebbe anche se `Slid` non producesse NIENTE — che e'
	// esattamente il difetto da cogliere, non un caso limite.
	if (!TestEqual(TEXT("uno scivolamento produce una riga"), Events.Num(), 1))
	{
		return false;
	}
	TestEqual(TEXT("e' un evento di movimento"),
		static_cast<int32>(Events[0].Type), static_cast<int32>(ERTPlayerEventType::Moved));
	TestEqual(TEXT("ed e' Important: l'unita' e' finita dove il giocatore non l'ha mandata"),
		static_cast<int32>(Events[0].Importance), static_cast<int32>(ERTPlayerEventImportance::Important));
	return true;
}

/**
 * LO SCIVOLAMENTO IMPEDITO E' UN FATTO SUO — `#2314`, decisione d'autore D2.
 *
 * 🔴 **`Slid` e `SlideBlocked` raccontano due cose OPPOSTE**, e nel feed rischiavano di essere la stessa
 * riga: in `#2290` producevano `Type` uguale, `Importance` uguale e `ActionId` uguale, cioe' la distinzione
 * che l'esito esiste per registrare non arrivava al canale piu' visibile. Il terreno che ti ha spostato e il
 * terreno che ci ha provato senza riuscirci sono la premessa opposta per il turno dopo — solo il primo
 * lascia `Status.Unbalanced` (`D-319`).
 *
 * ⛔ **E non e' `MoveBlocked`.** Il Move che il giocatore ha chiesto e' RIUSCITO. Presentarlo come un
 * fallimento del movimento sposta di un canale il difetto che `#2314` chiude nel resolver — e sarebbe il
 * secondo posto in cui il gioco dice al giocatore che il suo piano non ha funzionato quando ha funzionato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerEventSlideBlockedIsDistinctTest,
	"RefactorTactics.UI.PlayerEventLog.SlideBlockedIsDistinctFromSlid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerEventSlideBlockedIsDistinctTest::RunTest(const FString&)
{
	TArray<FRTTurnLogEntry> Bloccato;
	Bloccato.Add(PlayerEventEntry(ERTLogCategory::Move,
		static_cast<uint8>(ERTMoveOutcome::SlideBlocked), /*UnitId*/ 3, /*Team*/ 0));
	const TArray<FRTPlayerEvent> EventiBloccato = URTPlayerEventProjector::Project(Bloccato, 0);

	// ANTI-VACUITA': un esito non tradotto in `ClassifyEntry` cade nel `default` e NON produce niente —
	// senza questa riga il test passerebbe proprio nel caso che deve cogliere.
	if (!TestEqual(TEXT("uno scivolamento impedito produce una riga"), EventiBloccato.Num(), 1))
	{
		return false;
	}

	TArray<FRTTurnLogEntry> Scivolato;
	Scivolato.Add(PlayerEventEntry(ERTLogCategory::Move,
		static_cast<uint8>(ERTMoveOutcome::Slid), /*UnitId*/ 3, /*Team*/ 0));
	const TArray<FRTPlayerEvent> EventiScivolato = URTPlayerEventProjector::Project(Scivolato, 0);
	if (!TestEqual(TEXT("premessa: anche lo scivolamento avvenuto ne produce una"), EventiScivolato.Num(), 1))
	{
		return false;
	}

	TestNotEqual(TEXT("i due sono tipi DIVERSI, non la stessa riga con un'altra importanza"),
		static_cast<int32>(EventiBloccato[0].Type), static_cast<int32>(EventiScivolato[0].Type));
	TestEqual(TEXT("ed e' il tipo dedicato"),
		static_cast<int32>(EventiBloccato[0].Type), static_cast<int32>(ERTPlayerEventType::SlideBlocked));
	TestEqual(TEXT("importanza Important: il non-fatto va letto, non seppellito"),
		static_cast<int32>(EventiBloccato[0].Importance),
		static_cast<int32>(ERTPlayerEventImportance::Important));
	TestNotEqual(TEXT("e NON e' MoveBlocked: il movimento chiesto era riuscito"),
		static_cast<int32>(EventiBloccato[0].Type), static_cast<int32>(ERTPlayerEventType::MoveBlocked));

	// `Important` e non `Minor`: il filtro finale toglie i `Minor`, e un `SlideBlocked` classificato cosi'
	// sparirebbe dal feed senza che nessuna asserzione sul tipo se ne accorga.
	TestEqual(TEXT("e sopravvive al filtro dei Minor"),
		PlayerEventCountOf(EventiBloccato, ERTPlayerEventType::SlideBlocked), 1);
	return true;
}

/**
 * LO SCIVOLAMENTO IMPEDITO NON DICE CHI L'HA IMPEDITO — `#2314`, guardrail di privacy.
 *
 * 🔴 **Un esito nuovo e' una porta nuova.** Il `TurnLog` autorevole sa perche' lo scivolamento non e'
 * avvenuto — puo' essere stata un'unita' che l'osservatore non e' autorizzato a vedere — e il feed e' il
 * canale piu' visibile che esista. La difesa non e' una policy nuova: e' `IsAuthorized`, che chiama
 * `FRTKnowledgeVerdict::AllowsTeam` come **primo** passo, piu' il fatto che una voce `Move` porti il solo
 * soggetto (`#1150`, `[D-063]`) e nessun campo in cui un blocker possa entrare.
 *
 * ⚠️ **Il test misura entrambe le meta'.** Che la voce non autorizzata non produca **niente** — nemmeno un
 * conteggio o un tipo, che e' la differenza fra filtrare prima e sanitizzare dopo — e che quella
 * autorizzata non porti con se' l'altro capo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerEventSlideBlockedHidesTheBlockerTest,
	"RefactorTactics.UI.PlayerEventLog.SlideBlockedDoesNotRevealTheBlocker",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerEventSlideBlockedHidesTheBlockerTest::RunTest(const FString&)
{
	// Lo scivolamento di un'unita' AVVERSARIA, impedito: la voce esiste nel TurnLog autorevole ed e'
	// leggibile dalla squadra 1, non dalla 0.
	TArray<FRTTurnLogEntry> Log;
	FRTTurnLogEntry Nemica = PlayerEventEntry(ERTLogCategory::Move,
		static_cast<uint8>(ERTMoveOutcome::SlideBlocked), /*UnitId*/ 9, /*AllowedTeam*/ 1);
	Nemica.SrcCell = FRTCellId(4, -2, 0);
	Nemica.TgtCell = FRTCellId(5, -2, 0);
	Log.Add(Nemica);

	// PREMESSA: per chi e' autorizzato la riga c'e'. Senza, «zero eventi» sarebbe soddisfatto anche da un
	// proiettore rotto, e il test proverebbe la privacy misurando un difetto.
	const TArray<FRTPlayerEvent> PerLAutorizzato = URTPlayerEventProjector::Project(Log, /*Team*/ 1);
	if (!TestEqual(TEXT("premessa: chi e' autorizzato legge lo scivolamento impedito"),
		PlayerEventCountOf(PerLAutorizzato, ERTPlayerEventType::SlideBlocked), 1))
	{
		return false;
	}

	// E per chi non lo e': NIENTE. Non una riga anonima, non un conteggio.
	const TArray<FRTPlayerEvent> PerLAltro = URTPlayerEventProjector::Project(Log, /*Team*/ 0);
	TestEqual(TEXT("chi non e' autorizzato non riceve nessun evento"), PerLAltro.Num(), 0);

	// L'evento autorizzato parla del SOGGETTO — chi non e' scivolato — e di nessun altro: `SecondaryUnitId`
	// resta vuoto, ed e' il campo in cui un blocker potrebbe entrare per distrazione.
	TestEqual(TEXT("l'evento nomina chi non e' scivolato"), PerLAutorizzato[0].PrimaryUnitId, 9);
	TestEqual(TEXT("e nessun secondo soggetto: il blocker non ha un campo dove stare"),
		PerLAutorizzato[0].SecondaryUnitId, static_cast<int32>(INDEX_NONE));

	// 🔴 **`BlockerCell` e' un campo NUOVO, ed e' esattamente il posto in cui questo blocker potrebbe
	// entrare per distrazione** (`#2697`). Esiste per il muro di `NoLineOfSight` — una cella di TERRENO che
	// `SightBlockerForLog` ha gia' filtrato — e cio' che impedisce lo scivolamento non e' quello: puo'
	// essere un'unita' che l'osservatore non conosce, e per essa nessun filtro e' stato applicato.
	//
	// ⚠️ **Senza questa riga il guardrail sopra invecchia in silenzio**: la doc di `SlideBlocked` dichiarava
	// «non esiste nessun campo in cui un blocker possa entrare», e dal momento in cui il campo esiste quella
	// frase e' una promessa da verificare invece che un fatto strutturale.
	TestFalse(TEXT("e la cella del blocker resta la sentinella: il campo nuovo non e' una porta"),
		PerLAutorizzato[0].HasBlockerCell());

	// ⛔ E l'autorizzazione e' quella ESISTENTE, non una scritta per questo esito: lo stesso predicato che
	// il canale testuale applica. Un `SlideBlocked` che si autorizzasse da solo passerebbe le righe sopra.
	TestFalse(TEXT("e passa dal predicato di autorizzazione condiviso"),
		URTPlayerEventProjector::IsAuthorized(Nemica, /*ObserverTeamId*/ 0));
	TestTrue(TEXT("che ammette la squadra autorizzata"),
		URTPlayerEventProjector::IsAuthorized(Nemica, /*ObserverTeamId*/ 1));
	return true;
}

/**
 * IL TIRO SENZA LINEA DI VISTA ARRIVA A CHI GIOCA — `#2697`, ed e' cio' che completa la via *(b)* di
 * `#2534`.
 *
 * 🔴 **Il difetto che questo test esiste per impedire e' un SILENZIO.** `ClassifyEntry` scartava
 * `NoLineOfSight` con la motivazione *«e' il perche' di un colpo che non c'e' stato: diagnostica, non
 * cronaca»* — vera finche' l'unico canale al giocatore era il testo diagnostico, falsa da quando la riga
 * e' **l'unica cosa** che spiega un'azione dichiarata e non avvenuta. `D-340` ha scelto di far nominare
 * la causa al log; la seduta del 2026-09-09 ha misurato che quella riga non lascia l'Output Log.
 *
 * ⚠️ **Il bersaglio e' VELATO, ed e' la ragione per cui il silenzio non e' recuperabile guardando.** E' il
 * muro stesso a bloccare la vista: chi guarda vede la propria unita' sparare verso il nulla, senza vedere
 * ne' il nemico ne' l'ostacolo. Senza questa riga il giocatore conclude che l'attacco sia rotto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerEventNoLineOfSightReachesTheFeedTest,
	"RefactorTactics.UI.PlayerEventLog.NoLineOfSightReachesTheFeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerEventNoLineOfSightReachesTheFeedTest::RunTest(const FString&)
{
	TArray<FRTTurnLogEntry> Log;
	FRTTurnLogEntry Rifiutato = PlayerEventEntry(ERTLogCategory::Combat,
		static_cast<uint8>(ERTCombatOutcome::NoLineOfSight), /*UnitId*/ 3, /*AllowedTeam*/ 0);
	Rifiutato.SrcCell = FRTCellId(-1, 0, 0);
	Rifiutato.TgtCell = FRTCellId(1, 0, 0);
	Rifiutato.SightBlockerCell = FRTCellId(0, 0, 0);
	Rifiutato.ActionId = TEXT("Action.BasicAttack");
	Log.Add(Rifiutato);

	const TArray<FRTPlayerEvent> Eventi = URTPlayerEventProjector::Project(Log, /*ObserverTeamId*/ 0);

	if (!TestEqual(TEXT("il tiro rifiutato produce una riga per chi gioca"), Eventi.Num(), 1))
	{
		return false;
	}

	TestEqual(TEXT("che nomina chi ha provato a sparare"), Eventi[0].PrimaryUnitId, 3);
	TestEqual(TEXT("e l'azione dichiarata"), Eventi[0].ActionId.ToString(),
		FString(TEXT("Action.BasicAttack")));

	// ⚠️ **`Minor` sarebbe una sparizione travestita da evento**: `Project` scarta le righe minori alla
	// fine, quindi un tipo classificato `Minor` non arriverebbe comunque al feed. L'asserzione guarda cio'
	// che il giocatore riceve, non cio' che il proiettore ha pensato.
	TestTrue(TEXT("e non e' silenziosa"),
		Eventi[0].Importance != ERTPlayerEventImportance::Minor);
	return true;
}

/**
 * IL MURO ARRIVA AL FEED COME CELLA, NON COME FRASE — `#2697`.
 *
 * 🔑 **E' il campo che permette il «riferimento video» che il verdetto d'autore chiedeva**: *«non ci sono
 * riferimenti video, solo log»*. Con la cella, chi disegna puo' marcare l'ostacolo nel mondo; con una
 * stringa gia' composta potrebbe solo ristamparla, e ristampare `(q=0,r=0,L=0)` a un giocatore e' la
 * diagnostica che `#1936` §A toglie dallo schermo.
 *
 * ⛔ **La cella NON viene ricalcolata qui e non passa da nessun filtro nuovo.** Arriva dalla voce, dove
 * `URTTurnLogLibrary::SightBlockerForLog` l'ha gia' ammessa solo se la squadra dell'attaccante la conosceva
 * — `VisibleCells` ∪ `ExploredCells`, fail-closed. E' la stessa ragione per cui `RTReplayPrivacyLibrary` la
 * marca **`Public`**: cio' che arriva a valle e' gia' filtrato, e nasconderlo non aggiungerebbe privacy.
 *
 * ⚠️ **Le due meta' del test non sono ridondanti.** Che la cella nominabile arrivi prova il canale; che
 * quella non nominabile resti la **sentinella** prova che il silenzio si propaga — e il silenzio deve
 * restare indistinguibile dall'assenza ([D-225]), altrimenti il feed direbbe «c'e' un muro ma non te lo
 * dico», che e' gia' informazione sulla geometria velata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerEventBlockerCellReachesTheFeedTest,
	"RefactorTactics.UI.PlayerEventLog.BlockerCellReachesTheFeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerEventBlockerCellReachesTheFeedTest::RunTest(const FString&)
{
	// ── Muro nominabile: la squadra dell'attaccante conosceva la cella, e `SightBlockerForLog` l'ha scritta.
	{
		TArray<FRTTurnLogEntry> Log;
		FRTTurnLogEntry ConMuro = PlayerEventEntry(ERTLogCategory::Combat,
			static_cast<uint8>(ERTCombatOutcome::NoLineOfSight), /*UnitId*/ 3, /*AllowedTeam*/ 0);
		ConMuro.SightBlockerCell = FRTCellId(0, 0, 0);
		Log.Add(ConMuro);

		const TArray<FRTPlayerEvent> Eventi = URTPlayerEventProjector::Project(Log, /*ObserverTeamId*/ 0);
		if (!TestEqual(TEXT("premessa: il tiro rifiutato produce una riga"), Eventi.Num(), 1))
		{
			return false;
		}
		TestTrue(TEXT("che porta un muro nominabile"), Eventi[0].HasBlockerCell());
		TestTrue(TEXT("ed e' la cella che la voce portava"),
			Eventi[0].BlockerCell == FRTCellId(0, 0, 0));
	}

	// ── Muro NON nominabile: la squadra non conosceva quella cella, e la voce porta la sentinella. La riga
	// per il giocatore esiste lo stesso — «il tiro non e' partito» resta vero e utile — ma non dice dove.
	{
		TArray<FRTTurnLogEntry> Log;
		FRTTurnLogEntry SenzaMuro = PlayerEventEntry(ERTLogCategory::Combat,
			static_cast<uint8>(ERTCombatOutcome::NoLineOfSight), /*UnitId*/ 3, /*AllowedTeam*/ 0);
		SenzaMuro.SightBlockerCell = FRTTurnLogEntry::NoSightBlocker();
		Log.Add(SenzaMuro);

		const TArray<FRTPlayerEvent> Eventi = URTPlayerEventProjector::Project(Log, /*ObserverTeamId*/ 0);
		if (!TestEqual(TEXT("la riga c'e' anche senza muro nominabile"), Eventi.Num(), 1))
		{
			return false;
		}
		TestFalse(TEXT("ma non nomina nessuna cella: il silenzio si propaga"),
			Eventi[0].HasBlockerCell());
	}
	return true;
}

/**
 * IL TIRO RIFIUTATO DI UN AVVERSARIO NON RIVELA IL SUO MURO — `#2697`, guardrail di privacy.
 *
 * 🔴 **Un campo nuovo e' una porta nuova, e questa ne porta una cella.** `BlockerCell` e' filtrata **alla
 * scrittura** per la squadra dell'attaccante; il feed pero' lo legge per un OSSERVATORE, che puo' essere
 * un'altra squadra. La difesa non e' un filtro nuovo — sarebbe un secondo contratto di conoscenza — ma
 * `IsAuthorized` come **primo** passo: una voce che l'osservatore non puo' leggere non diventa un evento,
 * quindi non c'e' nessun evento in cui la cella possa viaggiare.
 *
 * ⚠️ **Il test misura entrambe le meta'**, come il suo gemello per `SlideBlocked`: che l'autorizzato la
 * riceva — senza, «zero eventi» sarebbe soddisfatto anche da un proiettore rotto — e che l'altro non
 * riceva niente, nemmeno un conteggio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerEventBlockedShotHidesTheWallTest,
	"RefactorTactics.UI.PlayerEventLog.BlockedShotDoesNotRevealTheWallToTheUnauthorized",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerEventBlockedShotHidesTheWallTest::RunTest(const FString&)
{
	// Il tiro rifiutato di un'unita' della squadra 1, con un muro che LA SUA squadra conosce.
	TArray<FRTTurnLogEntry> Log;
	FRTTurnLogEntry Avversario = PlayerEventEntry(ERTLogCategory::Combat,
		static_cast<uint8>(ERTCombatOutcome::NoLineOfSight), /*UnitId*/ 9, /*AllowedTeam*/ 1);
	Avversario.SrcCell = FRTCellId(4, -2, 0);
	Avversario.TgtCell = FRTCellId(6, -2, 0);
	Avversario.SightBlockerCell = FRTCellId(5, -2, 0);
	Log.Add(Avversario);

	// PREMESSA: per chi e' autorizzato la riga c'e', con la sua cella.
	const TArray<FRTPlayerEvent> PerLAutorizzato = URTPlayerEventProjector::Project(Log, /*Team*/ 1);
	if (!TestEqual(TEXT("premessa: chi e' autorizzato legge il tiro rifiutato"),
		PlayerEventCountOf(PerLAutorizzato, ERTPlayerEventType::AttackBlocked), 1))
	{
		return false;
	}
	TestTrue(TEXT("premessa: e ne vede il muro"), PerLAutorizzato[0].HasBlockerCell());

	// E per chi non lo e': NIENTE. Non una riga anonima, non una cella, non un conteggio.
	const TArray<FRTPlayerEvent> PerLAltro = URTPlayerEventProjector::Project(Log, /*Team*/ 0);
	TestEqual(TEXT("chi non e' autorizzato non riceve nessun evento"), PerLAltro.Num(), 0);

	// ⛔ E il predicato e' quello ESISTENTE, non uno scritto per questo esito.
	TestFalse(TEXT("e passa dal predicato di autorizzazione condiviso"),
		URTPlayerEventProjector::IsAuthorized(Avversario, /*ObserverTeamId*/ 0));
	TestTrue(TEXT("che ammette la squadra autorizzata"),
		URTPlayerEventProjector::IsAuthorized(Avversario, /*ObserverTeamId*/ 1));
	return true;
}

/**
 * UN MOVIMENTO NELLO STESSO TURNO NON COPRE IL TIRO RIFIUTATO — `#2697`, dominanza.
 *
 * 🔴 **E' il difetto documentato per `SlideBlocked`, alla lettera.** Un tipo senza riga propria in
 * `DominanceRank` cade nel `default` a `10`, cioe' **sotto** `Moved` a `20`: la stessa unita' che si muove
 * e poi prova a sparare tiene la riga del movimento, che il filtro finale scarta perche' `Minor`. Ne esce
 * **zero eventi** — la sparizione che questo test esiste per impedire.
 *
 * ⚠️ **Lo scenario e' quello ordinario, non un caso limite**: muoversi e attaccare nello stesso turno e'
 * il turno normale di questo gioco, ed e' precisamente il turno in cui il giocatore si chiede perche' il
 * colpo non sia partito.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerEventAttackBlockedSurvivesAMoveTest,
	"RefactorTactics.UI.PlayerEventLog.AttackBlockedSurvivesAMoveInTheSameTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerEventAttackBlockedSurvivesAMoveTest::RunTest(const FString&)
{
	// La stessa unita', nello stesso turno: prima si sposta, poi il tiro non parte.
	TArray<FRTTurnLogEntry> Log;
	Log.Add(PlayerEventEntry(ERTLogCategory::Move,
		static_cast<uint8>(ERTMoveOutcome::Moved), /*UnitId*/ 3, /*AllowedTeam*/ 0));

	FRTTurnLogEntry Rifiutato = PlayerEventEntry(ERTLogCategory::Combat,
		static_cast<uint8>(ERTCombatOutcome::NoLineOfSight), /*UnitId*/ 3, /*AllowedTeam*/ 0);
	Rifiutato.SightBlockerCell = FRTCellId(0, 0, 0);
	Log.Add(Rifiutato);

	const TArray<FRTPlayerEvent> Eventi = URTPlayerEventProjector::Project(Log, /*ObserverTeamId*/ 0);

	TestEqual(TEXT("il tiro rifiutato sopravvive al movimento della stessa unita'"),
		PlayerEventCountOf(Eventi, ERTPlayerEventType::AttackBlocked), 1);

	// ⚠️ **E resta l'UNICA riga**: il movimento riuscito non ne aggiunge una seconda. Senza questa
	// asserzione, un proiettore che accodasse invece di dominare passerebbe la riga sopra.
	TestEqual(TEXT("e resta l'unica riga di quell'unita'"), Eventi.Num(), 1);
	return true;
}

/**
 * IL MOVIMENTO NON PRODUCE UNA RIGA PER CELLA — `#1936` §E.
 *
 * ⚠️ **Il difetto non sarebbe nel TurnLog ma nel feed**, ed e' la ragione per cui il test sta qui: il
 * `TurnLog` ha gia' cinque produttori distinti di voci `Move` — `BuildMoveLog`, il Dash, il superseded e
 * due altri — quindi la stessa unita' che si muove, scatta e viene spostata puo' portarne piu' d'una nello
 * stesso turno. Senza dominanza, chi guarda leggerebbe tre righe per un'unita' che si e' mossa una volta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerEventCollapsesMoveCellsTest,
	"RefactorTactics.UI.PlayerEventLog.CollapsesMoveCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerEventCollapsesMoveCellsTest::RunTest(const FString&)
{
	// Tre voci `Move` della STESSA unita', tutte bloccate: tre fatti, un solo protagonista.
	TArray<FRTTurnLogEntry> Log;
	Log.Add(PlayerEventEntry(ERTLogCategory::Move,
		static_cast<uint8>(ERTMoveOutcome::BlockedContested), /*UnitId*/ 7, /*Team*/ 0));
	Log.Add(PlayerEventEntry(ERTLogCategory::Move,
		static_cast<uint8>(ERTMoveOutcome::BlockedByUnit), 7, 0));
	Log.Add(PlayerEventEntry(ERTLogCategory::Move,
		static_cast<uint8>(ERTMoveOutcome::BlockedByTopology), 7, 0));

	const TArray<FRTPlayerEvent> Events = URTPlayerEventProjector::Project(Log, /*ObserverTeamId*/ 0);

	// ANTI-VACUITA': se l'autorizzazione fallisse, zero eventi soddisferebbe «non piu' di uno».
	if (!TestTrue(TEXT("premessa: qualcosa e' stato proiettato"), Events.Num() > 0))
	{
		return false;
	}

	TestEqual(TEXT("tre voci Move della stessa unita' -> una riga sola"),
		PlayerEventCountOf(Events, ERTPlayerEventType::MoveBlocked), 1);
	return true;
}

/**
 * IL MOVIMENTO RIUSCITO E' SILENZIOSO — `#1936` §D.
 *
 * Il giocatore lo vede gia' animato. Una riga per ogni spostamento seppellirebbe le tre che contano, ed e'
 * il difetto che questa issue esiste per togliere: *«il log racconta le celle, non la partita»*.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerEventOmitsMinorMovementTest,
	"RefactorTactics.UI.PlayerEventLog.OmitsMinorMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerEventOmitsMinorMovementTest::RunTest(const FString&)
{
	TArray<FRTTurnLogEntry> Log;
	Log.Add(PlayerEventEntry(ERTLogCategory::Move, static_cast<uint8>(ERTMoveOutcome::Moved), 3, 0));
	Log.Add(PlayerEventEntry(ERTLogCategory::Move, static_cast<uint8>(ERTMoveOutcome::Moved), 4, 0));

	TestEqual(TEXT("due movimenti riusciti -> nessuna riga"),
		URTPlayerEventProjector::Project(Log, 0).Num(), 0);

	// ANTI-VACUITA': lo stesso montaggio con un esito che CONTA produce una riga. Senza questo, il test
	// resterebbe verde anche se il proiettore non producesse mai niente.
	TArray<FRTTurnLogEntry> Bloccato;
	Bloccato.Add(PlayerEventEntry(ERTLogCategory::Move,
		static_cast<uint8>(ERTMoveOutcome::BlockedByUnit), 3, 0));
	TestEqual(TEXT("ma un movimento bloccato si vede"),
		URTPlayerEventProjector::Project(Bloccato, 0).Num(), 1);
	return true;
}

/**
 * IL KO DOMINA IL DANNO, E IL DANNO IL COLPO — `#1936` §E.
 *
 * 🔴 La narrazione tripla *«Ivrin colpisce Phase · Phase subisce 24 · Phase e' eliminata»* e' il modo in
 * cui un feed diventa illeggibile pur essendo corretto: tre righe per un fatto solo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerEventKODominatesTest,
	"RefactorTactics.UI.PlayerEventLog.KODominatesDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerEventKODominatesTest::RunTest(const FString&)
{
	TArray<FRTTurnLogEntry> Log;
	Log.Add(PlayerEventEntry(ERTLogCategory::Combat,
		static_cast<uint8>(ERTCombatOutcome::ShieldAbsorbed), /*UnitId*/ 9, 0));
	Log.Add(PlayerEventEntry(ERTLogCategory::Combat, static_cast<uint8>(ERTCombatOutcome::Hit), 9, 0));
	Log.Add(PlayerEventEntry(ERTLogCategory::Combat, static_cast<uint8>(ERTCombatOutcome::Lethal), 9, 0));

	const TArray<FRTPlayerEvent> Events = URTPlayerEventProjector::Project(Log, 0);

	TestEqual(TEXT("tre voci sulla stessa unita' -> una riga"), Events.Num(), 1);
	if (Events.Num() == 1)
	{
		TestTrue(TEXT("e la riga che resta e' il KO"), Events[0].Type == ERTPlayerEventType::Defeated);
		TestTrue(TEXT("con importanza Critical"),
			Events[0].Importance == ERTPlayerEventImportance::Critical);
	}

	// ⚠️ L'ordine di arrivo non deve cambiare l'esito: il KO domina anche se arriva per PRIMO.
	TArray<FRTTurnLogEntry> Invertito;
	Invertito.Add(PlayerEventEntry(ERTLogCategory::Combat,
		static_cast<uint8>(ERTCombatOutcome::Lethal), 9, 0));
	Invertito.Add(PlayerEventEntry(ERTLogCategory::Combat, static_cast<uint8>(ERTCombatOutcome::Hit), 9, 0));
	const TArray<FRTPlayerEvent> Rovesciato = URTPlayerEventProjector::Project(Invertito, 0);
	TestEqual(TEXT("ordine invertito -> sempre una riga"), Rovesciato.Num(), 1);
	if (Rovesciato.Num() == 1)
	{
		TestTrue(TEXT("ed e' sempre il KO"), Rovesciato[0].Type == ERTPlayerEventType::Defeated);
	}
	return true;
}

/**
 * LA PROPAGAZIONE AMBIENTALE E' UN EVENTO SOLO — `#1936` §E.
 *
 * *«Niente cella d'acqua A / cella d'acqua B / cella elettrificata C»*: l'ambiente si raggruppa
 * semanticamente, e il conteggio va in `Amount` invece che in tre righe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerEventGroupsEnvironmentTest,
	"RefactorTactics.UI.PlayerEventLog.GroupsEnvironment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerEventGroupsEnvironmentTest::RunTest(const FString&)
{
	TArray<FRTTurnLogEntry> Log;
	for (int32 i = 0; i < 5; ++i)
	{
		Log.Add(PlayerEventEntry(ERTLogCategory::Environment, /*Outcome*/ 0, /*UnitId*/ 0, 0));
	}

	const TArray<FRTPlayerEvent> Events = URTPlayerEventProjector::Project(Log, 0);

	TestEqual(TEXT("cinque celle ambientali -> un evento"), Events.Num(), 1);
	if (Events.Num() == 1)
	{
		TestTrue(TEXT("di tipo Environment"), Events[0].Type == ERTPlayerEventType::Environment);
		TestEqual(TEXT("e il conteggio vive in Amount, non in cinque righe"), Events[0].Amount, 5);
	}
	return true;
}

/**
 * L'ORDINE E' QUELLO DI RISOLUZIONE, ED E' DETERMINISTICO — `#1936` §E.
 *
 * ⚠️ **La dominanza sostituisce sul posto**, quindi la riga di un'unita' resta dove quell'unita' e'
 * comparsa la prima volta. E' cio' che rende il feed una cronaca invece di un elenco riordinato: se il KO
 * saltasse in fondo, il giocatore leggerebbe la fine prima della causa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerEventPreservesOrderTest,
	"RefactorTactics.UI.PlayerEventLog.PreservesSemanticOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerEventPreservesOrderTest::RunTest(const FString&)
{
	TArray<FRTTurnLogEntry> Log;
	Log.Add(PlayerEventEntry(ERTLogCategory::Combat, static_cast<uint8>(ERTCombatOutcome::Hit), 1, 0));
	Log.Add(PlayerEventEntry(ERTLogCategory::Combat, static_cast<uint8>(ERTCombatOutcome::Hit), 2, 0));
	Log.Add(PlayerEventEntry(ERTLogCategory::Combat, static_cast<uint8>(ERTCombatOutcome::Lethal), 1, 0));

	const TArray<FRTPlayerEvent> Events = URTPlayerEventProjector::Project(Log, 0);

	TestEqual(TEXT("due unita' -> due righe"), Events.Num(), 2);
	if (Events.Num() == 2)
	{
		TestEqual(TEXT("l'unita' 1 resta PRIMA, dove e' comparsa"), Events[0].PrimaryUnitId, 1);
		TestTrue(TEXT("e la sua riga e' diventata il KO"), Events[0].Type == ERTPlayerEventType::Defeated);
		TestEqual(TEXT("l'unita' 2 resta seconda"), Events[1].PrimaryUnitId, 2);
	}

	// Determinismo: la stessa sequenza proiettata due volte da' lo stesso risultato.
	const TArray<FRTPlayerEvent> Bis = URTPlayerEventProjector::Project(Log, 0);
	TestEqual(TEXT("due proiezioni identiche"), Bis.Num(), Events.Num());
	return true;
}

/**
 * PROIETTORE E CANALE TESTUALE AMMETTONO LO STESSO INSIEME — `#1936`, criterio di non-divergenza.
 *
 * 🔴 **E' l'assertion che tiene insieme le due porte.** `ComposeVisibleLogLines` e il proiettore rispondono
 * alla stessa domanda — *«questo osservatore puo' leggerlo?»* — e se un giorno divergessero, un fatto
 * privato uscirebbe da una delle due senza che nulla lo dica. Il test non confronta il TESTO (sono formati
 * diversi): confronta **quanti fatti** ciascuna porta ammette, sugli stessi verdetti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerEventAuthorizationMatchesTest,
	"RefactorTactics.UI.PlayerEventLog.AuthorizationMatchesLogLines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerEventAuthorizationMatchesTest::RunTest(const FString&)
{
	// Cinque verdetti misti: due per la squadra 0, due per la 1, uno per tutti.
	TArray<FRTKnowledgeVerdict> Verdetti;
	{
		FRTKnowledgeVerdict V0; V0.AllowTeam(0); Verdetti.Add(V0);
		FRTKnowledgeVerdict V1; V1.AllowTeam(1); Verdetti.Add(V1);
		FRTKnowledgeVerdict V2; V2.AllowTeam(0); Verdetti.Add(V2);
		FRTKnowledgeVerdict V3; V3.AllowTeam(1); Verdetti.Add(V3);
		Verdetti.Add(FRTKnowledgeVerdict::Everyone());
	}

	for (int32 Observer = 0; Observer <= 1; ++Observer)
	{
		// Lato proiettore: quante voci passano `IsAuthorized`.
		int32 AmmesseDalProiettore = 0;
		for (const FRTKnowledgeVerdict& V : Verdetti)
		{
			FRTTurnLogEntry E;
			E.Verdict = V;
			if (URTPlayerEventProjector::IsAuthorized(E, Observer)) { ++AmmesseDalProiettore; }
		}

		// Lato canale testuale: quante righe escono da `ComposeVisibleLogLines` con gli stessi verdetti.
		TArray<FRTCombatLogLine> Righe;
		for (const FRTKnowledgeVerdict& V : Verdetti)
		{
			FRTCombatLogLine L;
			L.Text = TEXT("riga");
			L.Verdict = V;
			Righe.Add(L);
		}
		const int32 AmmesseDalCanale = URTCombatLogLibrary::ComposeVisibleLogLines(Righe, Observer).Num();

		// ANTI-VACUITA': se entrambe ammettessero zero, l'uguaglianza sarebbe soddisfatta da due rifiuti.
		if (!TestTrue(*FString::Printf(TEXT("premessa: la squadra %d legge qualcosa"), Observer),
			AmmesseDalCanale > 0))
		{
			return false;
		}

		TestEqual(*FString::Printf(TEXT("squadra %d: le due porte ammettono lo stesso numero di fatti"),
			Observer), AmmesseDalProiettore, AmmesseDalCanale);
	}
	return true;
}

/**
 * L'ELIMINAZIONE E' PUBBLICA, E L'EVENTO NON PORTA UNA CELLA — [D-223], `#1936` §Privacy.
 *
 * 🔑 **L'asimmetria e' decisa, non un difetto**: un fatto ordinario su un nemico che non conosci non ti
 * raggiunge; la sua eliminazione si'. Il criterio non e' *di cosa parla* la riga, e' **cosa rivela** — nome
 * e squadra, mai una posizione.
 *
 * ⛔ La seconda meta' non e' un'assertion su un valore ma sul TIPO: `FRTPlayerEvent` non ha un campo cella,
 * quindi l'evento non puo' portarne una nemmeno per errore. Il test lo dichiara perche' chi aggiungesse un
 * `FRTCellId` a quella struct trovi qui scritto che cosa romperebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerEventDeathIsPublicTest,
	"RefactorTactics.UI.PlayerEventLog.DeathIsPublicWithoutCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerEventDeathIsPublicTest::RunTest(const FString&)
{
	// Un fatto ORDINARIO su un'unita' della squadra 1, leggibile solo da lei.
	FRTTurnLogEntry Ordinario = PlayerEventEntry(ERTLogCategory::Combat,
		static_cast<uint8>(ERTCombatOutcome::Hit), /*UnitId*/ 5, /*AllowedTeam*/ 1);

	// La sua ELIMINAZIONE, che [D-223] rende pubblica.
	FRTTurnLogEntry Morte = PlayerEventEntry(ERTLogCategory::Combat,
		static_cast<uint8>(ERTCombatOutcome::Lethal), 5, 1);
	Morte.Verdict = FRTKnowledgeVerdict::Everyone();

	const TArray<FRTPlayerEvent> Nemico = URTPlayerEventProjector::Project({ Ordinario, Morte },
		/*ObserverTeamId*/ 0);

	TestEqual(TEXT("chi non lo conosce vede SOLO l'eliminazione"), Nemico.Num(), 1);
	if (Nemico.Num() == 1)
	{
		TestTrue(TEXT("ed e' il KO"), Nemico[0].Type == ERTPlayerEventType::Defeated);
	}

	// Controprova: chi lo conosce vede il fatto ordinario, dominato dal KO nella stessa riga.
	const TArray<FRTPlayerEvent> Suoi = URTPlayerEventProjector::Project({ Ordinario, Morte }, 1);
	TestEqual(TEXT("chi lo conosce ha comunque una riga sola, per dominanza"), Suoi.Num(), 1);
	return true;
}

/**
 * LA PROIEZIONE NON ENTRA NELL'HASH DEL TURNLOG — `#1936` §C, invariante di determinismo.
 *
 * 🔴 **E' cio' che rende il feed una vista e non un'autorita'.** Se derivare eventi giocatore spostasse il
 * checksum, un replay dipenderebbe da come la UI ha deciso di raccontarlo — e la classe di difetto che
 * `#295` ha chiuso («un TurnLog sanitizzato per squadra toglie il determinismo al replay») tornerebbe dalla
 * porta di servizio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerEventHashIndependenceTest,
	"RefactorTactics.UI.PlayerEventLog.HashIsIndependentOfPlayerEvents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerEventHashIndependenceTest::RunTest(const FString&)
{
	TArray<FRTTurnLogEntry> Log;
	Log.Add(PlayerEventEntry(ERTLogCategory::Combat, static_cast<uint8>(ERTCombatOutcome::Hit), 1, 0));
	Log.Add(PlayerEventEntry(ERTLogCategory::Move,
		static_cast<uint8>(ERTMoveOutcome::BlockedByUnit), 2, 0));
	Log.Add(PlayerEventEntry(ERTLogCategory::Environment, 0, 0, 0));

	const uint32 Prima = URTTurnLogLibrary::HashTurnLog(Log);

	// Si proietta per DUE osservatori diversi: se la proiezione toccasse il log, due viste diverse
	// darebbero due hash diversi — che e' esattamente il difetto da escludere.
	const TArray<FRTPlayerEvent> Vista0 = URTPlayerEventProjector::Project(Log, 0);
	const TArray<FRTPlayerEvent> Vista1 = URTPlayerEventProjector::Project(Log, 1);

	const uint32 Dopo = URTTurnLogLibrary::HashTurnLog(Log);

	// ANTI-VACUITA': la proiezione deve aver fatto qualcosa, o l'invarianza sarebbe banale.
	if (!TestTrue(TEXT("premessa: la squadra 0 vede degli eventi"), Vista0.Num() > 0))
	{
		return false;
	}
	TestEqual(TEXT("e la squadra 1 non ne vede nessuno: le due viste differiscono"), Vista1.Num(), 0);

	TestEqual(TEXT("l'hash del TurnLog non cambia dopo la proiezione"), Dopo, Prima);
	return true;
}

/**
 * OGNI ESITO DI MOVIMENTO ARRIVA AL FEED, OPPURE E' DICHIARATO QUI — `#3081`.
 *
 * 🔴 **Il difetto che questo test rende impossibile e' una recidiva gia' avvenuta due volte.** `ClassifyEntry`
 * traduce gli esiti di `Move` con uno `switch` che termina in `default: return false`. Un valore aggiunto
 * all'enum e non tradotto li' non rompe niente: smette di produrre una riga. E' successo con `Slid` (`#2253`),
 * poi con le quattro cadute, il parapetto e il ciclo: l'unita' cambiava cella, spesso piano, e il feed taceva.
 * E' la forma di `RefactorTactics.TurnLog.EveryMoveOutcomeIsDescribed` (`#2628`), sull'altro canale.
 *
 * ⚠️ **Si passa da `Project`, non dalla funzione di classificazione**, che e' privata al file: cio' che si fissa
 * e' che l'esito produca una RIGA, che e' la proprieta' che il giocatore vede.
 *
 * 🔑 **Gli esclusi sono un elenco chiuso, col motivo, e anche loro sono asseriti.** Un escluso che un giorno
 * producesse una riga fa cadere il test: l'elenco non puo' restare indietro in nessuno dei due versi.
 * `Moved` e' fra gli esclusi per una ragione diversa dagli altri due — e' classificato `Minor`, e `Project` lo
 * toglie alla fine (`#1936` §D, `OmitsMinorMovement`). Da qui non si distingue da un esito non tradotto, ed e'
 * il limite di passare dalla porta pubblica: un esito nuovo che si volesse `Minor` va aggiunto qui a mano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerEventEveryMoveOutcomeTest,
	"RefactorTactics.UI.PlayerEventLog.EveryMoveOutcomeReachesTheFeedOrIsDeclared",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerEventEveryMoveOutcomeTest::RunTest(const FString&)
{
	const UEnum* Enum = StaticEnum<ERTMoveOutcome>();
	if (!TestNotNull(TEXT("l'enum degli esiti di movimento e' riflesso"), Enum))
	{
		return false;
	}
	// `NumEnums()` include il `_MAX` sintetico che UHT aggiunge: si sottrae.
	const int32 Valori = Enum->NumEnums() - 1;
	if (!TestTrue(TEXT("l'enum ha dei valori da coprire"), Valori > 0))
	{
		return false;
	}

	const TMap<ERTMoveOutcome, const TCHAR*> Esclusi = {
		{ ERTMoveOutcome::Stayed,           TEXT("non e' accaduto niente") },
		{ ERTMoveOutcome::SupersededByDash, TEXT("il movimento non e' avvenuto: lo ha sostituito lo scatto") },
		{ ERTMoveOutcome::Moved,            TEXT("Minor: Project lo toglie alla fine (#1936 par. D)") },
	};

	auto RigheDi = [](uint8 Esito)
	{
		TArray<FRTTurnLogEntry> Log;
		Log.Add(PlayerEventEntry(ERTLogCategory::Move, Esito, /*UnitId*/ 3, /*Team*/ 0));
		return URTPlayerEventProjector::Project(Log, /*ObserverTeamId*/ 0).Num();
	};

	// 🔑 **IL CONTROLLO POSITIVO DELLA RETE, e viene prima.** `200` non e' un enumeratore: dimostra che il
	// `default` c'e' ancora e non produce righe. Senza, un `default` che classificasse tutto renderebbe verde
	// il ciclo qui sotto per la ragione sbagliata.
	TestEqual(TEXT("un valore fuori enum cade nel default e non produce righe"), RigheDi(200), 0);

	int32 Raccontati = 0;
	for (int32 i = 0; i < Valori; ++i)
	{
		const ERTMoveOutcome Esito = static_cast<ERTMoveOutcome>(Enum->GetValueByIndex(i));
		const FString Nome = Enum->GetNameStringByIndex(i);
		const int32 Righe = RigheDi(static_cast<uint8>(Esito));

		if (const TCHAR* const* Motivo = Esclusi.Find(Esito))
		{
			TestEqual(FString::Printf(TEXT("%s e' escluso (%s) e non produce righe"), *Nome, *Motivo), Righe, 0);
		}
		else
		{
			TestEqual(FString::Printf(
				TEXT("%s produce una riga nel feed: un esito non tradotto in ClassifyEntry sparisce in silenzio"),
				*Nome), Righe, 1);
			Raccontati += Righe;
		}
	}

	// ANTI-VACUITA': se l'elenco degli esclusi coprisse tutto l'enum il ciclo non asserirebbe niente di utile.
	TestTrue(TEXT("gli esiti raccontati sono piu' degli esclusi"), Raccontati > Esclusi.Num());
	return true;
}

/**
 * LE QUATTRO CADUTE E IL PARAPETTO SONO SPOSTAMENTI SUBITI; IL CICLO E' UN BLOCCO — `#3081`.
 *
 * Il test qui sopra dice che una riga c'e'. Questo dice QUALE: tipo e importanza sono decisi, non ereditati.
 *
 * ⛔ **`StoppedByEdgeGuard` non e' `MoveBlocked`.** Lo spostamento forzato ha percorso celle e il parapetto
 * l'ha accorciato: dire «Movimento bloccato» direbbe al giocatore che il suo piano e' fallito, che e' falso —
 * la stessa obiezione che `SlideBlockedIsDistinctFromSlid` porta per lo scivolamento. Sta con `Displaced`, che
 * e' cio' che quel fatto produceva prima che il valore esistesse ([D-354]).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerEventFallsAreSufferedMovesTest,
	"RefactorTactics.UI.PlayerEventLog.FallsAndEdgeGuardAreSufferedMoves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerEventFallsAreSufferedMovesTest::RunTest(const FString&)
{
	auto Proietta = [](ERTMoveOutcome Esito)
	{
		TArray<FRTTurnLogEntry> Log;
		Log.Add(PlayerEventEntry(ERTLogCategory::Move, static_cast<uint8>(Esito), /*UnitId*/ 3, /*Team*/ 0));
		return URTPlayerEventProjector::Project(Log, /*ObserverTeamId*/ 0);
	};

	const ERTMoveOutcome Subiti[] = {
		ERTMoveOutcome::Fell, ERTMoveOutcome::FellToAlternative, ERTMoveOutcome::FellToLastStable,
		ERTMoveOutcome::FellWithoutLanding, ERTMoveOutcome::StoppedByEdgeGuard,
	};
	for (const ERTMoveOutcome Esito : Subiti)
	{
		const FString Nome = StaticEnum<ERTMoveOutcome>()->GetNameStringByValue(static_cast<int64>(Esito));
		const TArray<FRTPlayerEvent> Events = Proietta(Esito);
		if (!TestEqual(*(Nome + TEXT(": una riga")), Events.Num(), 1))
		{
			continue;
		}
		TestTrue(*(Nome + TEXT(": e' uno spostamento, non un blocco")),
			Events[0].Type == ERTPlayerEventType::Moved);
		TestTrue(*(Nome + TEXT(": Important, perche' l'unita' non e' dove il giocatore l'aveva mandata")),
			Events[0].Importance == ERTPlayerEventImportance::Important);
	}

	const TArray<FRTPlayerEvent> Ciclo = Proietta(ERTMoveOutcome::BlockedByCycle);
	if (TestEqual(TEXT("BlockedByCycle: una riga"), Ciclo.Num(), 1))
	{
		TestTrue(TEXT("BlockedByCycle: e' un movimento bloccato, come gli altri Blocked*"),
			Ciclo[0].Type == ERTPlayerEventType::MoveBlocked);
		TestTrue(TEXT("BlockedByCycle: Important"),
			Ciclo[0].Importance == ERTPlayerEventImportance::Important);
	}
	return true;
}

/**
 * CHI CADE HA UNA RIGA SOLA, ED E' QUELLA DEL DANNO — `#3081`.
 *
 * 🔴 **E' il fatto che rende insufficiente il solo `case`.** Una caduta scrive per la stessa unita' la voce di
 * movimento (`Fell`, rango 20) e una voce di danno con causa `Fall.Fall` (rango 80). La dominanza tiene una
 * riga per unita', quindi classificare la caduta come `Moved` NON la porta nel feed: la riga che resta e'
 * quella del danno. Chi leggesse solo il test degli esiti crederebbe che il feed racconti la caduta.
 *
 * 🔑 Cio' che questa prova fissa e' che la riga superstite porti la CAUSA — `ActionId` e `Amount` — perche' e'
 * da li' che `URTHudViewModel::ComposePlayerEventText` scrive «Caduta» invece di «Colpita». Il testo lo
 * asserisce `RefactorTactics.ScreenHud.FeedNamesAFallAndAnImpact`; il percorso vero del resolver,
 * `RefactorTactics.Fall.TheFeedSaysFallNotHit`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerEventFallDamageDominatesTest,
	"RefactorTactics.UI.PlayerEventLog.FallDamageDominatesTheFallMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerEventFallDamageDominatesTest::RunTest(const FString&)
{
	FRTTurnLogEntry Caduta = PlayerEventEntry(ERTLogCategory::Move,
		static_cast<uint8>(ERTMoveOutcome::Fell), /*UnitId*/ 7, /*Team*/ 0);
	FRTTurnLogEntry Danno = PlayerEventEntry(ERTLogCategory::Combat,
		static_cast<uint8>(ERTCombatOutcome::Hit), /*UnitId*/ 7, /*Team*/ 0);
	Danno.ActionId = URTTurnLogLibrary::FallCauseOfFaller();
	Danno.Amount = 5;

	// Nei due ordini: la voce di movimento precede il danno nel resolver, ma la dominanza non deve dipenderne.
	for (const bool bDannoPrima : { false, true })
	{
		TArray<FRTTurnLogEntry> Log;
		Log.Add(bDannoPrima ? Danno : Caduta);
		Log.Add(bDannoPrima ? Caduta : Danno);
		const TArray<FRTPlayerEvent> Events = URTPlayerEventProjector::Project(Log, /*ObserverTeamId*/ 0);

		const TCHAR* Ordine = bDannoPrima ? TEXT("danno prima") : TEXT("caduta prima");
		if (!TestEqual(*FString::Printf(TEXT("%s: due voci della stessa unita' -> una riga"), Ordine),
			Events.Num(), 1))
		{
			continue;
		}
		TestTrue(*FString::Printf(TEXT("%s: la riga che resta e' il danno"), Ordine),
			Events[0].Type == ERTPlayerEventType::Attacked);
		TestTrue(*FString::Printf(TEXT("%s: e porta la causa di caduta"), Ordine),
			Events[0].ActionId == URTTurnLogLibrary::FallCauseOfFaller());
		TestEqual(*FString::Printf(TEXT("%s: e l'importo"), Ordine), Events[0].Amount, 5);
	}

	// Il caso per cui il `case` serve comunque: l'osservatore e' autorizzato alla voce di movimento e non a
	// quella di danno. Senza il ramo di `#3081` qui non resterebbe nessuna riga.
	FRTTurnLogEntry DannoNascosto = PlayerEventEntry(ERTLogCategory::Combat,
		static_cast<uint8>(ERTCombatOutcome::Hit), /*UnitId*/ 7, /*Team*/ 1);
	DannoNascosto.ActionId = URTTurnLogLibrary::FallCauseOfFaller();
	TArray<FRTTurnLogEntry> Parziale;
	Parziale.Add(Caduta);
	Parziale.Add(DannoNascosto);
	const TArray<FRTPlayerEvent> Vista = URTPlayerEventProjector::Project(Parziale, /*ObserverTeamId*/ 0);
	if (TestEqual(TEXT("senza la voce di danno resta la riga dello spostamento"), Vista.Num(), 1))
	{
		TestTrue(TEXT("ed e' lo spostamento subito"), Vista[0].Type == ERTPlayerEventType::Moved);
	}
	return true;
}

/**
 * **CIO' CHE LA TUA UNITA' HA SUBITO VINCE SU CIO' CHE HA FATTO** (`#3734`, decisione d'autore A1 del 2026-10-13).
 *
 * Una riga per unita' (§E) costringe a scegliere. Per le unita' di chi guarda, una *riuscita propria* — il
 * movimento arrivato, il colpo dato, la cura data — cede a qualunque altra riga della stessa unita'; fra le
 * altre resta la scala di sempre, e a parita' la prima arrivata. I primi quattro casi sono le righe della
 * tabella approvata; gli altri sono i confini.
 *
 * 🔑 **Il quinto e' la guardia, e senza di lui la regola sarebbe sbagliata.** La riga di un colpo d'arma e'
 * agganciata a chi colpisce: quando un nemico colpisce una tua unita', quella riga E' la notizia per te, e
 * non deve cedere a cio' che succede al nemico nello stesso turno.
 *
 * ⚠️ Le stringhe si confrontano con `Equals(..., CaseSensitive)`: `TestEqual` fra due `FString` ignora le
 * maiuscole, e qui si legge quale voce e' rimasta dal suo `ActionId`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerEventSufferedBeatsDoneTest,
	"RefactorTactics.UI.PlayerEventLog.WhatYourUnitSufferedBeatsWhatItDid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerEventSufferedBeatsDoneTest::RunTest(const FString&)
{
	constexpr int32 Mia = 3;
	constexpr int32 Nemica = 7;
	const int32 Proprie = 0; // la squadra di chi guarda, per la regola
	const TArray<int32> Chi = { 0 };

	auto Voce = [](ERTLogCategory Categoria, uint8 Esito, int32 Unita, const FName& Azione)
	{
		FRTTurnLogEntry E = PlayerEventEntry(Categoria, Esito, Unita, /*Team*/ 0);
		E.ActionId = Azione; // serve solo a riconoscere QUALE voce e' rimasta
		// Di chi e' l'unita' lo dice la voce, come la scrive `AppendLogEntry`: la mia e' della squadra 0.
		E.VerdictSubject.StableUnitId = Unita;
		E.VerdictSubject.TeamId = Unita == Nemica ? 1 : 0;
		return E;
	};
	auto Mossa = [&Voce](ERTMoveOutcome Esito, int32 Unita, const TCHAR* Azione)
	{
		return Voce(ERTLogCategory::Move, static_cast<uint8>(Esito), Unita, FName(Azione));
	};
	// ⚠️ **Due celle diverse, e non e' un dettaglio.** Con `SrcCell == TgtCell` la rete di
	// `IsEnvironmentalDamage` classifica la voce come danno SUBITO — e' la forma di una voce ambientale — e la
	// regola la tratterebbe come una notizia. Un colpo dato ha la cella di chi colpisce e quella di chi e' colpito.
	auto Colpo = [&Voce](ERTCombatOutcome Esito, int32 Unita, const FName& Azione)
	{
		FRTTurnLogEntry E = Voce(ERTLogCategory::Combat, static_cast<uint8>(Esito), Unita, Azione);
		E.SrcCell = FRTCellId(0, 0);
		E.TgtCell = FRTCellId(1, 0);
		return E;
	};
	const FName ColpoDato(TEXT("Prova.Colpo"));
	const FName CuraData(TEXT("Prova.Cura"));
	const FName KO(TEXT("Prova.KO"));
	const FName Stato(TEXT("Status.Prova"));

	// L'unica riga rimasta, letta dal suo `ActionId`; zero o due righe sono entrambe un difetto.
	auto Resta = [&Chi](const TArray<FRTTurnLogEntry>& Log, int32 Squadra) -> FString
	{
		const TArray<FRTPlayerEvent> Ev = URTPlayerEventProjector::Project(Log, Chi, Squadra);
		return Ev.Num() == 1 ? Ev[0].ActionId.ToString() : FString::Printf(TEXT("<%d righe>"), Ev.Num());
	};
	auto Atteso = [this](const FString& Caso, const FString& Letta, const FName& Voluta)
	{
		TestTrue(*FString::Printf(TEXT("%s: resta «%s», letta «%s»"), *Caso, *Voluta.ToString(), *Letta),
			Letta.Equals(Voluta.ToString(), ESearchCase::CaseSensitive));
	};

	// La premessa del caso (2): la tassonomia del registro dice chi subisce, e la regola le crede.
	// Il danno da caduta e' riconosciuto dalla sua CAUSA, non dalla forma delle celle.
	const FRTTurnLogEntry DannoDaCaduta = Colpo(ERTCombatOutcome::Hit, Mia, URTTurnLogLibrary::FallCauseOfFaller());
	if (!TestTrue(TEXT("premessa: il danno da caduta e' di chi lo SUBISCE"), URTTurnLogLibrary::IsSubjectTheSufferer(DannoDaCaduta))
		|| !TestFalse(TEXT("premessa: il colpo d'arma e' di chi lo DA"),
			URTTurnLogLibrary::IsSubjectTheSufferer(Colpo(ERTCombatOutcome::Hit, Mia, ColpoDato))))
	{
		return false;
	}

	// (1) Lo scatto riuscito e poi lo spostamento subito: con la scala di sempre la riga silenziosa teneva il
	//     posto e poi spariva. Nei due ordini resta lo spostamento.
	const ERTMoveOutcome Subiti[] = {
		ERTMoveOutcome::Displaced, ERTMoveOutcome::Slid, ERTMoveOutcome::Fell, ERTMoveOutcome::StoppedByEdgeGuard,
	};
	for (const ERTMoveOutcome Subito : Subiti)
	{
		const FString Nome = StaticEnum<ERTMoveOutcome>()->GetNameStringByValue(static_cast<int64>(Subito));
		Atteso(TEXT("(1) scatto riuscito, poi ") + Nome,
			Resta({ Mossa(ERTMoveOutcome::Moved, Mia, TEXT("Prova.Scatto")), Mossa(Subito, Mia, TEXT("Prova.Spinta")) }, Proprie),
			FName(TEXT("Prova.Spinta")));
		Atteso(TEXT("(1) ") + Nome + TEXT(", poi scatto riuscito"),
			Resta({ Mossa(Subito, Mia, TEXT("Prova.Spinta")), Mossa(ERTMoveOutcome::Moved, Mia, TEXT("Prova.Scatto")) }, Proprie),
			FName(TEXT("Prova.Spinta")));
	}

	// (2) Colpisce, poi cade: «Caduta — M», non «Colpita — N». La voce di danno della caduta vince anche sulla
	//     voce `Fell` del movimento, per rango: e' lei che porta il numero.
	Atteso(TEXT("(2) colpisce e poi cade"),
		Resta({ Colpo(ERTCombatOutcome::Hit, Mia, ColpoDato), Mossa(ERTMoveOutcome::Fell, Mia, TEXT("Prova.Caduta")),
			DannoDaCaduta }, Proprie),
		URTTurnLogLibrary::FallCauseOfFaller());

	// (3) Colpisce, poi riceve uno stato.
	Atteso(TEXT("(3) colpisce e riceve uno stato"),
		Resta({ Colpo(ERTCombatOutcome::Hit, Mia, ColpoDato),
			Voce(ERTLogCategory::Status, static_cast<uint8>(ERTStatusOutcome::AppliedByAction), Mia, Stato) }, Proprie),
		Stato);

	// (4) Cura, poi viene spinta.
	Atteso(TEXT("(4) cura e viene spinta"),
		Resta({ Colpo(ERTCombatOutcome::Healed, Mia, CuraData), Mossa(ERTMoveOutcome::Displaced, Mia, TEXT("Prova.Spinta")) },
			Proprie),
		FName(TEXT("Prova.Spinta")));

	// (5) 🔴 LA GUARDIA: un'unita' NON di chi guarda colpisce e poi viene spinta. Quella riga e' il colpo che una
	//     tua unita' ha preso: resta.
	Atteso(TEXT("(5) un nemico colpisce e poi viene spinto"),
		Resta({ Colpo(ERTCombatOutcome::Hit, Nemica, ColpoDato), Mossa(ERTMoveOutcome::Displaced, Nemica, TEXT("Prova.Spinta")) },
			Proprie),
		ColpoDato);

	// (6) Una notizia non cede a una riuscita propria arrivata DOPO, anche se la scala la metterebbe sopra.
	Atteso(TEXT("(6) spinta, poi colpo dato"),
		Resta({ Mossa(ERTMoveOutcome::Displaced, Mia, TEXT("Prova.Spinta")), Colpo(ERTCombatOutcome::Hit, Mia, ColpoDato) },
			Proprie),
		FName(TEXT("Prova.Spinta")));

	// (7) Fra due notizie dello stesso rango resta la prima arrivata, come prima.
	Atteso(TEXT("(7) due spostamenti subiti"),
		Resta({ Mossa(ERTMoveOutcome::Displaced, Mia, TEXT("Prova.Spinta")), Mossa(ERTMoveOutcome::Slid, Mia, TEXT("Prova.Scivolata")) },
			Proprie),
		FName(TEXT("Prova.Spinta")));

	// (8) Fra due riuscite proprie decide la scala: il colpo dato copre il movimento arrivato.
	Atteso(TEXT("(8) scatto riuscito, poi colpo dato"),
		Resta({ Mossa(ERTMoveOutcome::Moved, Mia, TEXT("Prova.Scatto")), Colpo(ERTCombatOutcome::Hit, Mia, ColpoDato) }, Proprie),
		ColpoDato);

	// (9) Il KO della TUA unita' non cede mai. ⚠️ Ma la riga di un colpo letale d'arma e' di chi colpisce: sotto la
	//     mia unita' e' l'eliminazione che lei ha DATO, e cede come un colpo dato (decisione d'autore del
	//     2026-10-14, trovato in code review).
	Atteso(TEXT("(9) elimina un nemico, poi viene spinta"),
		Resta({ Colpo(ERTCombatOutcome::Lethal, Mia, KO), Mossa(ERTMoveOutcome::Displaced, Mia, TEXT("Prova.Spinta")) }, Proprie),
		FName(TEXT("Prova.Spinta")));
	// Il KO che la mia unita' SUBISCE da una causa ambientale e' agganciato a lei: quello resta, anche contro una spinta.
	Atteso(TEXT("(9) muore cadendo, e una spinta arriva dopo"),
		Resta({ Colpo(ERTCombatOutcome::Lethal, Mia, URTTurnLogLibrary::FallCauseOfFaller()),
			Mossa(ERTMoveOutcome::Displaced, Mia, TEXT("Prova.Spinta")) }, Proprie),
		URTTurnLogLibrary::FallCauseOfFaller());

	// (10) Senza la squadra di chi guarda, la scala di sempre: il colpo dato tiene il posto. E' cio' che fa il
	//      proiettore per default, e la sessione non presidiata.
	const TArray<FRTTurnLogEntry> ColpoESpinta = {
		Colpo(ERTCombatOutcome::Hit, Mia, ColpoDato), Mossa(ERTMoveOutcome::Displaced, Mia, TEXT("Prova.Spinta")) };
	Atteso(TEXT("(10) senza la squadra di chi guarda"), Resta(ColpoESpinta, INDEX_NONE), ColpoDato);
	// ... e una voce senza soggetto registrato — una traccia ricaricata: il campo e' `Transient` — anche.
	TArray<FRTTurnLogEntry> Ricaricata = ColpoESpinta;
	for (FRTTurnLogEntry& E : Ricaricata) { E.VerdictSubject = FRTVerdictSubjectRef(); }
	Atteso(TEXT("(10) una traccia ricaricata"), Resta(Ricaricata, Proprie), ColpoDato);

	// (11) Una reazione che NON scatta non e' «Reazione scattata», e non copre il colpo dato. Il registro la
	//      scrive per ogni reazione armata in ogni Blast (code review).
	Atteso(TEXT("(11) colpisce, e la sua reazione armata non scatta"),
		Resta({ Colpo(ERTCombatOutcome::Hit, Mia, ColpoDato),
			Voce(ERTLogCategory::Reaction, static_cast<uint8>(ERTReactionOutcome::NotTriggered), Mia, FName(TEXT("Prova.Reazione"))) },
			Proprie),
		ColpoDato);
	TestEqual(TEXT("(11) da sola, una reazione non scattata non da' righe"),
		URTPlayerEventProjector::Project({ Voce(ERTLogCategory::Reaction, static_cast<uint8>(ERTReactionOutcome::Unavailable), Mia,
			FName(TEXT("Prova.Reazione"))) }, Chi, Proprie).Num(), 0);
	// ... e una scattata e' una notizia: copre il colpo dato.
	Atteso(TEXT("(11) colpisce, e la sua reazione scatta"),
		Resta({ Colpo(ERTCombatOutcome::Hit, Mia, ColpoDato),
			Voce(ERTLogCategory::Reaction, static_cast<uint8>(ERTReactionOutcome::Activated), Mia, FName(TEXT("Prova.Reazione"))) },
			Proprie),
		FName(TEXT("Prova.Reazione")));

	// (12) Uno stato che SCADE non e' successo all'unita' in questo turno: non copre il colpo dato.
	Atteso(TEXT("(12) colpisce, e un suo stato scade"),
		Resta({ Colpo(ERTCombatOutcome::Hit, Mia, ColpoDato),
			Voce(ERTLogCategory::Status, static_cast<uint8>(ERTStatusOutcome::Expired), Mia, Stato) }, Proprie),
		ColpoDato);

	// (13) Una cura su se stessi e' una cura DATA, anche con le due celle uguali che la rete dei danni legge come
	//      ambientali: cede alla spinta.
	FRTTurnLogEntry CuraSuDiSe = Colpo(ERTCombatOutcome::Healed, Mia, CuraData);
	CuraSuDiSe.TgtCell = CuraSuDiSe.SrcCell;
	Atteso(TEXT("(13) si cura, poi viene spinta"),
		Resta({ CuraSuDiSe, Mossa(ERTMoveOutcome::Displaced, Mia, TEXT("Prova.Spinta")) }, Proprie),
		FName(TEXT("Prova.Spinta")));
	const TArray<FRTPlayerEvent> SenzaInsieme = URTPlayerEventProjector::Project(ColpoESpinta, /*ObserverTeamId*/ 0);
	if (TestEqual(TEXT("(10) l'overload a un osservatore: una riga"), SenzaInsieme.Num(), 1))
	{
		Atteso(TEXT("(10) l'overload a un osservatore"), SenzaInsieme[0].ActionId.ToString(), ColpoDato);
	}
	return true;
}

/**
 * **UNA SPINTA DOPO UNO SCATTO RIUSCITO ARRIVA AL FEED, DAL TURNO VERO** (`#3734`).
 *
 * Il caso che la issue ha trovato, e che fino a qui era provato solo con voci di registro scritte a mano.
 * Muiren scatta di due celle con la sua scia; un'unita' nemica la spinge di una cella con un colpo che spinge
 * e basta. Le voci le scrive il turn manager, e il feed si costruisce dalla porta che il widget usa.
 *
 * ⚠️ **Non col getto di Muiren, ed e' misurato.** Il getto bagna il bersaglio (`Status.Wet`): la riga di stato
 * vince per rango sulla spinta, come deve, e la scena non isolava piu' il caso. La spinta qui e' la stessa
 * di `RTFallOverLedgeTests`: danno zero, un solo effetto.
 *
 * ⚠️ **Le premesse si leggono dalla traccia**: che lo scatto sia arrivato e che la spinta sia avvenuta, e in
 * quest'ordine. Senza, una riga assente direbbe «il feed tace» quando la scena non ha prodotto il caso.
 *
 * 🔑 Con la scala di sempre — l'overload senza le unita' di chi guarda — la mia unita' resta senza righe: e' il
 * difetto, ed e' la prova che la scena lo produce davvero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlayerEventPushAfterDashTest,
	"RefactorTactics.UI.PlayerEventLog.APushAfterASuccessfulDashReachesTheFeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlayerEventPushAfterDashTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ARTHexMapActor* Mappa = World->SpawnActor<ARTHexMapActor>();
	if (!TestNotNull(TEXT("mappa"), Mappa)) { RTWorldFixtures::DestroyWorld(World); return false; }
	Mappa->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 8);

	auto Muiren = [World](int32 Squadra, const FRTCellId& Cella) -> ARTUnit*
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = Squadra;
		U->bIsBotControlled = false; // i piani li scriviamo noi
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeMuiren());
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cella, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cella;
		return U;
	};
	auto NelKit = [](const ARTUnit* U, const TCHAR* Id) -> int32
	{
		for (int32 i = 0; U && i < U->NumAbilities(); ++i)
		{
			const URTActionData* Azione = U->GetAbility(i);
			if (Azione && Azione->Def.ActionId == FName(Id)) { return i; }
		}
		return INDEX_NONE;
	};

	ARTUnit* Mia = Muiren(/*Squadra=*/ 0, FRTCellId(0, 0));
	ARTUnit* Nemica = Muiren(/*Squadra=*/ 1, FRTCellId(4, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("la mia"), Mia) || !TestNotNull(TEXT("la nemica"), Nemica) || !TestNotNull(TEXT("TurnManager"), TM))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	// 🔴 **La conoscenza di squadra va allestita come la allestisce la partita**, dopo lo spawn. Senza, al primo
	// turno `TeamKnowledgeState` e' vuoto quando lo scatto scrive la sua voce, e il verdetto della voce esce
	// vuoto: non autorizzata per nessuno, nemmeno per la squadra che ha scattato. Il feed non la vedeva, e la
	// spinta compariva gia' con la scala di sempre — misurato: la scena non produceva il caso.
	TM->RefreshTeamKnowledgeNow();

	// La mia scatta di due celle verso est con la scia; la nemica, guardando a ovest, la spinge di una cella.
	const int32 Scatto = NelKit(Mia, TEXT("Hero.Muiren.FluidTrail"));
	Mia->PlannedDashAbility = Scatto;
	Mia->PlannedDashCell = FRTCellId(2, 0);

	URTActionData* AzioneDiSpinta = NewObject<URTActionData>(Nemica);
	AzioneDiSpinta->Def.ActionId = TEXT("Test.Spinta");
	AzioneDiSpinta->Def.ResolutionPhase = ERTResolutionPhase::Attack;
	AzioneDiSpinta->Def.Priority = 55;
	AzioneDiSpinta->Def.RangeCells = 4;
	AzioneDiSpinta->Def.Fallback = ERTActionFallback::AttackCell;
	AzioneDiSpinta->Def.InterruptPolicy = ERTInterruptPolicy::InterruptBeforeEffect;
	AzioneDiSpinta->Def.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Push, 1));
	AzioneDiSpinta->Def.bCountsAsAttack = true; // senza, nessun colpo e nessuna spinta (`INT-8`)
	AzioneDiSpinta->RangeCells = AzioneDiSpinta->Def.RangeCells;
	AzioneDiSpinta->Power = 0; // danno zero: un'unita' a terra non si sposta, e qui conta solo lo spostamento
	Nemica->Abilities.Add(AzioneDiSpinta);
	Nemica->PlannedAbilityIndex = Nemica->Abilities.Num() - 1;
	// ⚠️ **Sulla CELLA d'arrivo dello scatto, non sull'unita'.** Misurato: dichiarato sull'unita', il colpo non
	// arrivava — nessuna voce di combattimento nel turno. Perche', questo test non lo indaga: gli serve una
	// spinta che arrivi, e la premessa qui sotto lo controlla.
	Nemica->DeclareAttackOnCell(FRTCellId(2, 0));
	Nemica->Facing = ERTHexDirection::W;
	if (!TestTrue(TEXT("premessa: la mia ha lo scatto"), Scatto != INDEX_NONE)
		|| !TestTrue(TEXT("premessa: lo scatto si applica, e non e' una carica"), Mia->PlannedDashApplies() && !Mia->PlannedDashIsCharge()))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	TM->LockInAndResolve();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
	{
		TM->Tick(0.05f);
	}

	// Le premesse, dalla traccia: lo scatto arrivato, POI lo spostamento subito.
	int32 Arrivo = INDEX_NONE;
	int32 Spinta = INDEX_NONE;
	const TArray<FRTTurnLogEntry>& Log = TM->GetTurnLog();
	for (int32 i = 0; i < Log.Num(); ++i)
	{
		if (Log[i].UnitId != Mia->StableUnitId || Log[i].Category != ERTLogCategory::Move) { continue; }
		const ERTMoveOutcome Esito = static_cast<ERTMoveOutcome>(Log[i].Outcome);
		if (Esito == ERTMoveOutcome::Moved && Arrivo == INDEX_NONE) { Arrivo = i; }
		if ((Esito == ERTMoveOutcome::Displaced || Esito == ERTMoveOutcome::Slid) && Spinta == INDEX_NONE) { Spinta = i; }
	}
	if (!TestTrue(TEXT("premessa: lo scatto e' arrivato"), Arrivo != INDEX_NONE)
		|| !TestTrue(TEXT("premessa: poi la mia e' stata spinta"), Spinta != INDEX_NONE && Spinta > Arrivo))
	{
		for (const FRTTurnLogEntry& Voce : Log)
		{
			AddInfo(FString::Printf(TEXT("TurnLog: %s"), *URTTurnLogLibrary::DescribeEntry(Voce)));
		}
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	auto RigheDi = [](const TArray<FRTPlayerEventLineView>& Feed, const ARTUnit* U, FString& Testo) -> int32
	{
		int32 Quante = 0;
		for (const FRTPlayerEventLineView& Riga : Feed)
		{
			if (Riga.PrimaryStableUnitId == U->StableUnitId) { ++Quante; Testo = Riga.Text.ToString(); }
		}
		return Quante;
	};

	// Il difetto, con la scala di sempre: nessuna riga per la mia unita'.
	FString Prima;
	if (!TestEqual(TEXT("premessa: con la scala di sempre la mia unita' resta senza righe — e' il difetto"),
		RigheDi(URTHudViewModel::BuildPlayerEventFeed(Log, TArray<int32>{ 0 }), Mia, Prima), 0))
	{
		AddInfo(FString::Printf(TEXT("con la scala di sempre la riga e': «%s»"), *Prima));
		for (const FRTTurnLogEntry& Voce : Log)
		{
			if (Voce.UnitId == Mia->StableUnitId)
			{
				AddInfo(FString::Printf(TEXT("TurnLog della mia (autorizzata %d): %s"),
					URTPlayerEventProjector::IsAuthorized(Voce, 0) ? 1 : 0, *URTTurnLogLibrary::DescribeEntry(Voce)));
			}
		}
	}

	// La regola, dalla porta che il widget usa: il manager e chi guarda. La squadra la sa il manager.
	TestEqual(TEXT("presidiata, una squadra che guarda: e' lei"), URTHudViewModel::OwnTeamForFeed(TM, TArray<int32>{ 0 }), 0);
	TestEqual(TEXT("due squadre che guardano: nessuna"), URTHudViewModel::OwnTeamForFeed(TM, TArray<int32>{ 0, 1 }), INDEX_NONE);
	FString Dopo;
	if (TestEqual(TEXT("🔴 la mia unita' ha UNA riga"),
		RigheDi(URTHudViewModel::BuildPlayerEventFeed(TM, TArray<int32>{ 0 }), Mia, Dopo), 1))
	{
		if (!TestTrue(*FString::Printf(TEXT("ed e' lo spostamento: «%s»"), *Dopo), Dopo.Equals(TEXT("Spostata"), ESearchCase::CaseSensitive)))
		{
			for (const FRTTurnLogEntry& Voce : Log)
			{
				if (Voce.UnitId == Mia->StableUnitId)
				{
					AddInfo(FString::Printf(TEXT("TurnLog della mia: %s"), *URTTurnLogLibrary::DescribeEntry(Voce)));
				}
			}
		}
	}

	// 🔴 **La porta a un osservatore — quella dei segni dei bloccanti nell'HUD — e la diagnostica seguono la stessa
	// regola.** Senza, un segno sulla mappa potrebbe non avere la sua riga nel feed, e la diagnostica contare righe
	// che il widget non mostra (code review).
	FString DallaPortaSingola;
	TestEqual(TEXT("dalla porta a un osservatore, la stessa riga per la mia unita'"),
		RigheDi(URTHudViewModel::BuildPlayerEventFeed(TM, 0), Mia, DallaPortaSingola), 1);
	const FString Conteggio = FString::Printf(TEXT("righe dopo filtro=%d"),
		URTHudViewModel::BuildPlayerEventFeed(TM, TArray<int32>{ 0 }).Num());
	TestTrue(*FString::Printf(TEXT("la diagnostica conta le righe che il feed mostra («%s»)"), *Conteggio),
		FString::Join(URTHudViewModel::DescribeFeedState(TM, TArray<int32>{ 0 }), TEXT("\n")).Contains(Conteggio));

	// 🔴 **In sessione non presidiata nessuna unita' e' «di chi guarda»**, anche se l'insieme si restringe a una
	// squadra — succede a fine autobattle, quando l'altra non ha piu' attori in campo. Lo decide il manager.
	TM->SetUnattendedSession(true);
	FString NonPresidiata;
	TestEqual(TEXT("non presidiata, anche con una squadra sola nell'insieme: nessuna squadra di chi guarda"),
		URTHudViewModel::OwnTeamForFeed(TM, TArray<int32>{ 0 }), INDEX_NONE);
	TestEqual(TEXT("non presidiata: la scala di sempre, quindi nessuna riga per la mia unita'"),
		RigheDi(URTHudViewModel::BuildPlayerEventFeed(TM, TArray<int32>{ 0 }), Mia, NonPresidiata), 0);
	TM->SetUnattendedSession(false);

	// 🔴 **E dal widget, la porta che il giocatore vede.** `GetFeed` raccoglie da solo le unita' del proprio mondo
	// e le passa al view model: senza questa lettura, un widget che smettesse di passarle — e mostrasse di nuovo
	// il feed muto — lascerebbe verde tutto il resto. Misurato con la mutazione `M7`.
	URTPlayerEventLogWidget* Widget = NewObject<URTPlayerEventLogWidget>(World);
	if (TestNotNull(TEXT("il widget del feed"), Widget))
	{
		Widget->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);
		FString DalWidget;
		if (TestEqual(TEXT("🔴 dal widget, la mia unita' ha UNA riga"), RigheDi(Widget->GetFeed(), Mia, DalWidget), 1))
		{
			TestTrue(*FString::Printf(TEXT("ed e' lo spostamento: «%s»"), *DalWidget),
				DalWidget.Equals(TEXT("Spostata"), ESearchCase::CaseSensitive));
		}
	}

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
