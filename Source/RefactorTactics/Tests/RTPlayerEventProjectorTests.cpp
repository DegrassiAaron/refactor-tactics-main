#include "Misc/AutomationTest.h"
#include "Perception/RTTeamKnowledge.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h" // HashTurnLog: l'invariante che la proiezione non deve toccare
#include "Turn/RTCombatLog.h" // ComposeVisibleLogLines: il canale gemello da confrontare
#include "UI/RTPlayerEventProjector.h"

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
 * 🔴 La narrazione tripla *«Wraith colpisce Phase · Phase subisce 24 · Phase e' eliminata»* e' il modo in
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

#endif // WITH_DEV_AUTOMATION_TESTS
