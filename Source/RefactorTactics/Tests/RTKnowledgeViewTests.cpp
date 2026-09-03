#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Perception/RTKnowledgeView.h"
#include "Perception/RTTeamKnowledge.h"
#include "UI/RTHUD.h"
#include "Turn/RTTurnManager.h" // ARTTurnManager::VisibleTrailFor — l'ULTIMA ragione per cui questo test tira l'Actor
#include "Turn/RTCombatLog.h" // URTCombatLogLibrary: il filtro, che non vive piu' nell'Actor
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * CP 13.5 — la porta fra lo stato autorevole e la presentazione.
 *
 * Il test che conta e' `ViewIsIndependentOfHiddenState`: e' il gemello umano di
 * `HexBotPlay.HiddenEnemyFairness`, ed e' il debito che D-143 assegna al primo consumatore che introduca un
 * overlay di conoscenza. Questa fase e' quel consumatore.
 */

namespace
{
	/** Nome distinto per file: la unity build condivide la translation unit. */
	FRTKnowledgeSubject KvSubject(int32 StableId, int32 TeamId, const FRTCellId& Cell)
	{
		FRTKnowledgeSubject S;
		S.StableUnitId = StableId;
		S.TeamId = TeamId;
		S.Cell = Cell;
		S.HeroId = FName(*FString::Printf(TEXT("Hero%d"), StableId));
		S.HeroDisplayName = FText::FromString(FString::Printf(TEXT("Eroe %d"), StableId));
		S.bAlive = true;
		return S;
	}

	/** Conoscenza della squadra 0 al turno 5, con i contatti passati esplicitamente. */
	FRTTeamKnowledge KvKnowledge(const TArray<FRTLastKnownContact>& Contacts)
	{
		FRTTeamKnowledge K;
		K.TeamId = 0;
		K.TurnNumber = 5;
		K.Contacts = Contacts;
		return K;
	}

	/** Una riga col verdetto CONGELATO dalla conoscenza data: la catena vera, non una maschera a mano. */
	FRTCombatLogLine KvLineFor(const TCHAR* Text, const TArray<FRTTeamKnowledge>& Teams,
		const FRTKnowledgeSubject& Subject)
	{
		FRTCombatLogLine L;
		L.Text = Text;
		L.SubjectStableUnitId = Subject.StableUnitId;
		L.Verdict = URTTeamKnowledgeLibrary::FreezeVerdict(Teams, Subject);
		return L;
	}

	/** Una riga di mondo: dichiara di riguardare tutti, e non e' l'assenza di una decisione. */
	FRTCombatLogLine KvWorldLine(const TCHAR* Text)
	{
		FRTCombatLogLine L;
		L.Text = Text;
		L.Verdict = FRTKnowledgeVerdict::Everyone();
		return L;
	}

	// Le celle dei tre casi che il difetto «due copie» mette insieme. L'unita' 3 ne ha DUE, ed e' il punto:
	// dove la squadra la ricorda (`KvContactCell`) e dove sta davvero adesso (`KvActualCell`).
	const FRTCellId KvAllyCell(0, 0, 0);
	const FRTCellId KvSeenCell(3, 0, 0);
	const FRTCellId KvContactCell(4, 0, 0);
	const FRTCellId KvActualCell(9, 0, 0);

	/**
	 * Una vista coi TRE casi insieme: alleato (1), nemico VISTO ora (2), nemico appena perso di vista
	 * (3, `Remembered`). Costruita passando per `ViewForTeam`, non montando `FRTKnowledgeEntry` a mano:
	 * una voce `Remembered` deve nascere dalla porta vera, altrimenti il test proverebbe la propria fixture.
	 */
	FRTKnowledgeView KvViewWithRemembered()
	{
		FRTTeamKnowledge K = KvKnowledge({
			FRTLastKnownContact(2, KvSeenCell, 5),
			FRTLastKnownContact(3, KvContactCell, 5) });
		K.VisibleCells.Add(KvSeenCell); // solo la 2 e' sotto gli occhi: la 3 resta un ricordo

		const TArray<FRTKnowledgeSubject> Subjects = {
			KvSubject(1, 0, KvAllyCell),
			KvSubject(2, 1, KvSeenCell),
			KvSubject(3, 1, KvActualCell)
		};
		return URTKnowledgeViewLibrary::ViewForTeam(K, Subjects, /*ObserverTeamId*/ 0);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeViewOmitsHiddenTest,
	"RefactorTactics.Knowledge.ViewOmitsHidden",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeViewOmitsHiddenTest::RunTest(const FString&)
{
	// Squadra 0: un alleato (id 1) e due avversari — uno visto ora (id 2), uno ignoto (id 3).
	const FRTCellId AllyCell(0, 0, 0);
	const FRTCellId SeenCell(3, 0, 0);
	const FRTCellId HiddenCell(7, 0, 0);

	FRTTeamKnowledge K = KvKnowledge({ FRTLastKnownContact(2, SeenCell, 5) });
	K.VisibleCells.Add(SeenCell);

	const TArray<FRTKnowledgeSubject> Subjects = {
		KvSubject(1, 0, AllyCell),
		KvSubject(2, 1, SeenCell),
		KvSubject(3, 1, HiddenCell)
	};

	const FRTKnowledgeView View = URTKnowledgeViewLibrary::ViewForTeam(K, Subjects, /*ObserverTeamId*/ 0);

	TestEqual(TEXT("l'alleato e il nemico visto: due voci, non tre"), View.Entries.Num(), 2);
	TestNotNull(TEXT("l'alleato c'e'"), URTKnowledgeViewLibrary::FindEntry(View, 1));
	TestNotNull(TEXT("il nemico visto c'e'"), URTKnowledgeViewLibrary::FindEntry(View, 2));

	// 🔴 Il cuore: NESSUNA voce, non una voce con un flag.
	TestNull(TEXT("l'ignoto non ha una voce"), URTKnowledgeViewLibrary::FindEntry(View, 3));

	// E la sua cella non compare da nessuna parte nella vista.
	for (const FRTKnowledgeEntry& E : View.Entries)
	{
		TestFalse(TEXT("la cella dell'ignoto non trapela"), E.Cell == HiddenCell);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeViewIsIndependentOfHiddenStateTest,
	"RefactorTactics.Knowledge.ViewIsIndependentOfHiddenState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeViewIsIndependentOfHiddenStateTest::RunTest(const FString&)
{
	// Lo stesso osservatore, due stati autoritativi DIVERSI: il nemico ignoto sta in A oppure in B.
	// Se la vista differisce, l'informazione e' passata.
	const FRTCellId SeenCell(3, 0, 0);
	FRTTeamKnowledge K = KvKnowledge({ FRTLastKnownContact(2, SeenCell, 5) });
	K.VisibleCells.Add(SeenCell);

	const TArray<FRTKnowledgeSubject> WorldA = {
		KvSubject(1, 0, FRTCellId(0, 0, 0)),
		KvSubject(2, 1, SeenCell),
		KvSubject(3, 1, FRTCellId(7, 0, 0))
	};
	const TArray<FRTKnowledgeSubject> WorldB = {
		KvSubject(1, 0, FRTCellId(0, 0, 0)),
		KvSubject(2, 1, SeenCell),
		KvSubject(3, 1, FRTCellId(-7, 2, 0)) // stesso ignoto, dall'altra parte della mappa
	};

	const FRTKnowledgeView A = URTKnowledgeViewLibrary::ViewForTeam(K, WorldA, 0);
	const FRTKnowledgeView B = URTKnowledgeViewLibrary::ViewForTeam(K, WorldB, 0);

	TestEqual(TEXT("stesso numero di voci"), A.Entries.Num(), B.Entries.Num());
	for (int32 i = 0; i < A.Entries.Num() && i < B.Entries.Num(); ++i)
	{
		TestEqual(TEXT("stessa identita'"), A.Entries[i].StableUnitId, B.Entries[i].StableUnitId);
		TestTrue(TEXT("stessa cella"), A.Entries[i].Cell == B.Entries[i].Cell);
		TestTrue(TEXT("stessa visibilita'"), A.Entries[i].Visibility == B.Entries[i].Visibility);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeLastContactCarriesIdentityNotConditionTest,
	"RefactorTactics.Knowledge.LastContactCarriesIdentityNotCondition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeLastContactCarriesIdentityNotConditionTest::RunTest(const FString&)
{
	// Il nemico 2 e' stato visto in (3,0), poi si e' spostato in (6,0) senza essere visto.
	const FRTCellId Remembered(3, 0, 0);
	const FRTCellId Actual(6, 0, 0);

	const FRTTeamKnowledge K = KvKnowledge({ FRTLastKnownContact(2, Remembered, 5) });
	// `VisibleCells` VUOTA: nessuno lo vede ora. Resta solo il ricordo.

	const TArray<FRTKnowledgeSubject> Subjects = { KvSubject(2, 1, Actual) };
	const FRTKnowledgeView View = URTKnowledgeViewLibrary::ViewForTeam(K, Subjects, 0);

	const FRTKnowledgeEntry* E = URTKnowledgeViewLibrary::FindEntry(View, 2);
	if (!TestNotNull(TEXT("il ricordo produce una voce"), E))
	{
		return false;
	}
	TestTrue(TEXT("e' un ricordo, non un contatto vivo"), E->Visibility == ERTKnowledgeVisibility::Remembered);
	TestTrue(TEXT("porta la cella del CONTATTO"), E->Cell == Remembered);
	TestFalse(TEXT("e NON quella attuale"), E->Cell == Actual);
	TestFalse(TEXT("l'identita' c'e'"), E->HeroDisplayName.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeHudDrawsOnlyKnownUnitsTest,
	"RefactorTactics.Knowledge.HudDrawsOnlyKnownUnits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeHudDrawsOnlyKnownUnitsTest::RunTest(const FString&)
{
	const FRTCellId SeenCell(3, 0, 0);
	FRTTeamKnowledge K = KvKnowledge({ FRTLastKnownContact(2, SeenCell, 5) });
	K.VisibleCells.Add(SeenCell);

	const TArray<FRTKnowledgeSubject> Subjects = {
		KvSubject(1, 0, FRTCellId(0, 0, 0)),  // alleato
		KvSubject(2, 1, SeenCell),            // nemico visto
		KvSubject(3, 1, FRTCellId(7, 0, 0))   // nemico ignoto
	};
	const FRTKnowledgeView View = URTKnowledgeViewLibrary::ViewForTeam(K, Subjects, 0);

	// `ShouldDrawUnitOverlay` prende la voce gia' cercata (RTHUD.cpp la cerca una volta sola per unita' e la
	// riusa anche per `ContactGhostTargetForUnit`): il test replica lo stesso passo.
	TestTrue(TEXT("l'alleato si disegna"),
		ARTHUD::ShouldDrawUnitOverlay(URTKnowledgeViewLibrary::FindEntry(View, 1), /*bIsOwnTeam*/ true));
	TestTrue(TEXT("il nemico visto si disegna"),
		ARTHUD::ShouldDrawUnitOverlay(URTKnowledgeViewLibrary::FindEntry(View, 2), false));
	TestFalse(TEXT("il nemico ignoto NON si disegna"),
		ARTHUD::ShouldDrawUnitOverlay(URTKnowledgeViewLibrary::FindEntry(View, 3), false));

	// Anti-vacuita': un'unita' della propria squadra si disegna anche se, per un difetto della porta, non
	// avesse una voce (qui e' proprio cosi': l'id 99 non compare in nessun soggetto). Il proprio
	// schieramento non si nasconde mai a se stessi.
	TestTrue(TEXT("la propria squadra non si nasconde mai"),
		ARTHUD::ShouldDrawUnitOverlay(URTKnowledgeViewLibrary::FindEntry(View, 99), /*bIsOwnTeam*/ true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeCombatLogOmitsUnknownTest,
	"RefactorTactics.Knowledge.CombatLogOmitsUnknown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeCombatLogOmitsUnknownTest::RunTest(const FString&)
{
	const FRTCellId SeenCell(3, 0, 0);
	FRTTeamKnowledge K = KvKnowledge({ FRTLastKnownContact(2, SeenCell, 5) });
	K.VisibleCells.Add(SeenCell);

	const TArray<FRTKnowledgeSubject> Subjects = {
		KvSubject(1, 0, FRTCellId(0, 0, 0)),  // alleato
		KvSubject(2, 1, SeenCell),            // nemico visto
		KvSubject(3, 1, FRTCellId(7, 0, 0))   // nemico ignoto
	};
	// 🔴 I verdetti si CONGELANO passando da `FreezeVerdict`, non si montano a mano ([D-223]): un test che
	// scrivesse le maschere direttamente proverebbe la propria fixture invece della regola.
	TArray<FRTCombatLogLine> Raw;
	Raw.Add(KvWorldLine(TEXT("Turno 5 - pianificazione")));                      // riga di mondo
	Raw.Add(KvLineFor(TEXT("Alleato: passo -> (0,0,L0)"),      { K }, Subjects[0]));
	Raw.Add(KvLineFor(TEXT("Nemico visto: passo -> (3,0,L0)"), { K }, Subjects[1]));
	Raw.Add(KvLineFor(TEXT("Ignoto: passo -> (7,0,L0)"),       { K }, Subjects[2]));

	const TArray<FString> Visible = URTCombatLogLibrary::ComposeVisibleLogLines(Raw, /*ObserverTeamId*/ 0);

	TestEqual(TEXT("tre righe su quattro"), Visible.Num(), 3);
	TestTrue (TEXT("la riga di mondo resta"),  Visible.Contains(TEXT("Turno 5 - pianificazione")));
	TestTrue (TEXT("l'alleato resta"),         Visible.Contains(TEXT("Alleato: passo -> (0,0,L0)")));
	TestTrue (TEXT("il nemico visto resta"),   Visible.Contains(TEXT("Nemico visto: passo -> (3,0,L0)")));

	// 🔴 Il cuore: la riga sparisce INTERA. Non una riga oscurata, non una riga vuota.
	TestFalse(TEXT("la riga dell'ignoto sparisce"), Visible.Contains(TEXT("Ignoto: passo -> (7,0,L0)")));

	// E la sua cella non trapela in NESSUNA delle righe superstiti.
	for (const FString& L : Visible)
	{
		TestFalse(TEXT("nessuna riga nomina la cella dell'ignoto"), L.Contains(TEXT("(7,0,L0)")));
	}

	// Anti-vacuita': l'ORDINE si conserva. Un filtro che riordinasse renderebbe il combat log illeggibile,
	// e nessuna delle asserzioni sopra lo prenderebbe.
	if (Visible.Num() == 3)
	{
		TestEqual(TEXT("l'ordine e' quello di produzione"), Visible[0], TEXT("Turno 5 - pianificazione"));
		TestEqual(TEXT("secondo"), Visible[1], TEXT("Alleato: passo -> (0,0,L0)"));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeUnitRenderingCombinesAliveAndKnownTest,
	"RefactorTactics.Knowledge.UnitRenderingCombinesAliveAndKnown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeUnitRenderingCombinesAliveAndKnownTest::RunTest(const FString&)
{
	TestTrue (TEXT("vivo e noto: si vede"),        ARTUnit::ShouldBeRendered(true,  true));
	TestFalse(TEXT("vivo ma ignoto: sparisce"),    ARTUnit::ShouldBeRendered(true,  false));

	// 🔴 Le due righe che impediscono il difetto: un MORTO non torna visibile perche' la conoscenza lo
	// «rivela». La morte vince sempre sulla conoscenza, in entrambi i versi.
	TestFalse(TEXT("morto e noto: resta nascosto"), ARTUnit::ShouldBeRendered(false, true));
	TestFalse(TEXT("morto e ignoto: nascosto"),     ARTUnit::ShouldBeRendered(false, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeGhostFadesWithContactAgeTest,
	"RefactorTactics.Knowledge.GhostFadesWithContactAge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeGhostFadesWithContactAgeTest::RunTest(const FString&)
{
	// `ContactLifetimeTurns` vale 1: il ricordo dura il turno successivo a quello dell'avvistamento.
	// Turno del contatto == turno corrente -> sagoma fresca; un turno dopo -> gia' in dissolvenza; oltre -> nulla.
	//
	// 🔴 **Nessuna eta' e' opaca.** La spec dichiara una sagoma **semitrasparente** (S4, riga 101 di
	// `docs/technical/systems/conoscenza-parziale-visibile-spec.md`): l'asserzione precedente chiedeva
	// `1.0` ad `Age == 0`, cioe' proprio il valore che la spec vieta, e nel caso PIU' FREQUENTE — «perso
	// di vista in questo turno». Si asserisce l'intervallo aperto, non il numero: la scala e' presentazione
	// e puo' essere ritoccata, l'opacita' piena no.
	TestTrue(TEXT("appena visto: si vede bene ma NON e' opaca"),
		ARTUnit::GhostOpacityForContact(/*ContactTurn*/ 5, /*CurrentTurn*/ 5) > 0.0f
		&& ARTUnit::GhostOpacityForContact(5, 5) < 1.0f
		&& ARTUnit::GhostOpacityForContact(5, 5) > ARTUnit::GhostOpacityForContact(5, 6));
	TestTrue(TEXT("un turno dopo: dissolve ma c'e' ancora"),
		ARTUnit::GhostOpacityForContact(5, 6) > 0.0f && ARTUnit::GhostOpacityForContact(5, 6) < 1.0f);
	TestEqual(TEXT("oltre la scadenza: sparita"),
		ARTUnit::GhostOpacityForContact(5, 7), 0.0f);

	// Anti-vacuita': un ricordo dal FUTURO (turno maggiore del corrente) non e' un ricordo. Fail-closed.
	TestEqual(TEXT("un contatto dal futuro non disegna nulla"),
		ARTUnit::GhostOpacityForContact(9, 5), 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeRememberedEntryCarriesContactTurnTest,
	"RefactorTactics.Knowledge.RememberedEntryCarriesContactTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeRememberedEntryCarriesContactTurnTest::RunTest(const FString&)
{
	// Il contatto e' avvenuto al turno 5. La CONOSCENZA e' una fotografia del turno 8 -- DIVERSO apposta
	// (sonda anti-vacuita'): un'implementazione che copiasse `Knowledge.TurnNumber` invece del turno DEL
	// CONTATTO, o che lasciasse il campo al default, passerebbe se i due turni coincidessero per caso.
	const FRTCellId Remembered(3, 0, 0);
	const FRTCellId Actual(6, 0, 0);

	FRTTeamKnowledge K;
	K.TeamId = 0;
	K.TurnNumber = 8;
	K.Contacts = { FRTLastKnownContact(2, Remembered, 5) };
	// `VisibleCells` vuota: nessuno lo vede ora, resta solo il ricordo.

	const TArray<FRTKnowledgeSubject> Subjects = { KvSubject(2, 1, Actual) };
	const FRTKnowledgeView View = URTKnowledgeViewLibrary::ViewForTeam(K, Subjects, /*ObserverTeamId*/ 0);

	const FRTKnowledgeEntry* E = URTKnowledgeViewLibrary::FindEntry(View, 2);
	if (!TestNotNull(TEXT("il ricordo produce una voce"), E))
	{
		return false;
	}
	TestTrue(TEXT("e' un ricordo"), E->Visibility == ERTKnowledgeVisibility::Remembered);
	TestEqual(TEXT("porta il turno DEL CONTATTO"), E->ContactTurn, 5);
	TestNotEqual(TEXT("non il turno della conoscenza"), E->ContactTurn, K.TurnNumber);
	TestNotEqual(TEXT("non lo zero del default"), E->ContactTurn, 0);
	return true;
}

/**
 * `ARTHUD::ContactGhostTargetForUnit` e' la porta stessa il cui difetto originale era «nessun chiamante»
 * (ebfee2a9): `DrawHUD` non ha una rete di test, quindi questa e' l'unica automazione che si accorgerebbe
 * se un domani qualcuno invertisse un ramo o leggesse la posizione sbagliata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeContactGhostTargetForFourCasesTest,
	"RefactorTactics.Knowledge.ContactGhostTargetForFourCases",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeContactGhostTargetForFourCasesTest::RunTest(const FString&)
{
	// La voce Remembered si costruisce passando per `ViewForTeam` (non un `FRTKnowledgeEntry` a mano), con
	// la cella ATTUALE del soggetto diversa da quella del RICORDO: e' la sonda anti-vacuita' chiesta dalla
	// review. Un'implementazione che leggesse la posizione attuale invece del ricordo produrrebbe `Actual`,
	// non `Remembered`, e questo test cadrebbe.
	const FRTCellId Remembered(3, 0, 0);
	const FRTCellId Actual(6, 0, 0);

	FRTTeamKnowledge K;
	K.TeamId = 0;
	K.TurnNumber = 8;
	K.Contacts = { FRTLastKnownContact(2, Remembered, 5) };

	const TArray<FRTKnowledgeSubject> Subjects = { KvSubject(2, 1, Actual) };
	const FRTKnowledgeView View = URTKnowledgeViewLibrary::ViewForTeam(K, Subjects, /*ObserverTeamId*/ 0);
	const FRTKnowledgeEntry* RememberedEntry = URTKnowledgeViewLibrary::FindEntry(View, 2);
	if (!TestNotNull(TEXT("il ricordo produce una voce"), RememberedEntry))
	{
		return false;
	}

	// Caso 1 -- nemico Remembered: produce un target con la cella del RICORDO (mai quella attuale) e il
	// turno del contatto.
	const TOptional<FRTContactGhostTarget> RememberedTarget =
		ARTHUD::ContactGhostTargetForUnit(RememberedEntry, /*bIsOwnTeam*/ false);
	if (!TestTrue(TEXT("un ricordo produce un target"), RememberedTarget.IsSet()))
	{
		return false;
	}
	TestTrue(TEXT("la cella e' quella del RICORDO"), RememberedTarget->Cell == Remembered);
	TestFalse(TEXT("mai quella ATTUALE del soggetto"), RememberedTarget->Cell == Actual);
	TestEqual(TEXT("il turno e' quello del contatto"), RememberedTarget->ContactTurn, 5);

	// Caso 2 -- nemico Live: una voce c'e', ma non e' un ricordo -> nessun target.
	FRTKnowledgeEntry LiveEntry;
	LiveEntry.StableUnitId = 3;
	LiveEntry.Visibility = ERTKnowledgeVisibility::Live;
	LiveEntry.Cell = Actual;
	TestFalse(TEXT("un nemico visto ORA non ha sagoma"),
		ARTHUD::ContactGhostTargetForUnit(&LiveEntry, /*bIsOwnTeam*/ false).IsSet());

	// Caso 3 -- unita' della propria squadra: mai una sagoma, ANCHE con una voce Remembered -- prova che
	// `bIsOwnTeam` decide PRIMA di guardare `Visibility`. La squadra propria non si ricorda mai se stessa.
	TestFalse(TEXT("la propria squadra non ha mai una sagoma"),
		ARTHUD::ContactGhostTargetForUnit(RememberedEntry, /*bIsOwnTeam*/ true).IsSet());

	// Caso 4 -- nessuna voce nella vista (Rejected, ricordo scaduto): nessun target.
	TestFalse(TEXT("nessuna voce -> nessuna sagoma"),
		ARTHUD::ContactGhostTargetForUnit(nullptr, /*bIsOwnTeam*/ false).IsSet());

	return true;
}

/**
 * 🔴 Il difetto portante della fase: un nemico `Remembered` restava disegnato, cliccabile, con nome e barra
 * HP alla sua posizione VERA, **mentre** la sagoma lo disegnava una seconda volta alla cella ricordata. Due
 * copie della stessa unita', cioe' l'opposto di cio' per cui la sagoma esiste.
 *
 * La causa era `ShouldDrawUnitOverlay` che chiedeva «esiste una voce?» invece di «la posizione e' attuale?».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeHudHidesRememberedEnemyTest,
	"RefactorTactics.Knowledge.HudHidesRememberedEnemy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeHudHidesRememberedEnemyTest::RunTest(const FString&)
{
	const FRTKnowledgeView View = KvViewWithRemembered();

	const FRTKnowledgeEntry* Remembered = URTKnowledgeViewLibrary::FindEntry(View, 3);
	if (!TestNotNull(TEXT("il ricordo HA una voce: il difetto non era l'assenza della voce"), Remembered))
	{
		return false;
	}
	TestTrue(TEXT("ed e' proprio un ricordo"), Remembered->Visibility == ERTKnowledgeVisibility::Remembered);
	TestTrue(TEXT("la voce porta la cella del CONTATTO"), Remembered->Cell == KvContactCell);

	// 🔴 Il cuore: la voce c'e', e proprio per questo il personaggio vero NON si disegna.
	TestFalse(TEXT("un nemico RICORDATO non si disegna"),
		ARTHUD::ShouldDrawUnitOverlay(Remembered, /*bIsOwnTeam*/ false));

	// Anti-vacuita': il predicato non e' diventato «falso per ogni nemico».
	TestTrue(TEXT("il nemico VISTO ORA continua a disegnarsi"),
		ARTHUD::ShouldDrawUnitOverlay(URTKnowledgeViewLibrary::FindEntry(View, 2), false));
	TestTrue(TEXT("e l'alleato pure"),
		ARTHUD::ShouldDrawUnitOverlay(URTKnowledgeViewLibrary::FindEntry(View, 1), /*bIsOwnTeam*/ true));
	return true;
}

/**
 * La stessa regola sul secondo canale: le righe di un `Remembered` portano le coordinate ATTUALI del
 * soggetto (`DescribeEntry` stampa `SrcCell` e `TgtCell` di ogni movimento), cioe' precisamente cio' che la
 * squadra ha smesso di sapere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeCombatLogOmitsRememberedTest,
	"RefactorTactics.Knowledge.CombatLogOmitsRemembered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeCombatLogOmitsRememberedTest::RunTest(const FString&)
{
	// La stessa conoscenza da cui `KvViewWithRemembered` costruisce la sua vista: la 2 sotto gli occhi,
	// la 3 solo ricordata.
	FRTTeamKnowledge K = KvKnowledge({
		FRTLastKnownContact(2, KvSeenCell, 5),
		FRTLastKnownContact(3, KvContactCell, 5) });
	K.VisibleCells.Add(KvSeenCell);

	const FRTKnowledgeSubject Ally      = KvSubject(1, 0, KvAllyCell);
	const FRTKnowledgeSubject Seen      = KvSubject(2, 1, KvSeenCell);
	const FRTKnowledgeSubject Remembered = KvSubject(3, 1, KvActualCell);

	TArray<FRTCombatLogLine> Raw;
	Raw.Add(KvWorldLine(TEXT("Turno 5 - pianificazione")));
	Raw.Add(KvLineFor(TEXT("Alleato: passo -> (0,0,L0)"),      { K }, Ally));
	Raw.Add(KvLineFor(TEXT("Nemico visto: passo -> (3,0,L0)"), { K }, Seen));
	Raw.Add(KvLineFor(TEXT("Ricordato: passo -> (9,0,L0)"),    { K }, Remembered)); // cella ATTUALE

	const TArray<FString> Visible = URTCombatLogLibrary::ComposeVisibleLogLines(Raw, /*ObserverTeamId*/ 0);

	// 🔴 Il cuore: la riga del ricordato sparisce INTERA.
	TestFalse(TEXT("la riga del RICORDATO sparisce"), Visible.Contains(TEXT("Ricordato: passo -> (9,0,L0)")));

	// E la sua posizione attuale non trapela da nessun'altra riga.
	for (const FString& L : Visible)
	{
		TestFalse(TEXT("nessuna riga nomina la posizione attuale del ricordato"), L.Contains(TEXT("(9,0,L0)")));
	}

	// Anti-vacuita': il filtro non ha svuotato il log. Le altre tre restano, nell'ordine di produzione.
	TestEqual(TEXT("tre righe su quattro"), Visible.Num(), 3);
	TestTrue(TEXT("la riga di mondo resta"), Visible.Contains(TEXT("Turno 5 - pianificazione")));
	TestTrue(TEXT("l'alleato resta"),        Visible.Contains(TEXT("Alleato: passo -> (0,0,L0)")));
	TestTrue(TEXT("il nemico visto resta"),  Visible.Contains(TEXT("Nemico visto: passo -> (3,0,L0)")));
	return true;
}

/**
 * 🔴 L'asserto che impedisce a un futuro refactor di riaprire il caso «due copie».
 *
 * `ShouldDrawUnitOverlay` e `ContactGhostTargetForUnit` leggono la STESSA voce e devono essere
 * COMPLEMENTARI: per un nemico esattamente uno dei due risponde di si'. Se entrambi rispondono di si' si
 * vedono due copie; se entrambi rispondono di no il nemico sparisce senza lasciare traccia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeOverlayAndGhostAreComplementaryTest,
	"RefactorTactics.Knowledge.OverlayAndGhostAreComplementary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeOverlayAndGhostAreComplementaryTest::RunTest(const FString&)
{
	const FRTKnowledgeView View = KvViewWithRemembered();

	const FRTKnowledgeEntry* Remembered = URTKnowledgeViewLibrary::FindEntry(View, 3);
	const FRTKnowledgeEntry* Live = URTKnowledgeViewLibrary::FindEntry(View, 2);
	if (!TestNotNull(TEXT("la voce del ricordo c'e'"), Remembered)
		|| !TestNotNull(TEXT("la voce del nemico visto c'e'"), Live))
	{
		return false;
	}

	// Ricordo: nessun personaggio, una sagoma alla cella del contatto.
	const bool bOverlayForRemembered = ARTHUD::ShouldDrawUnitOverlay(Remembered, /*bIsOwnTeam*/ false);
	const TOptional<FRTContactGhostTarget> GhostForRemembered =
		ARTHUD::ContactGhostTargetForUnit(Remembered, /*bIsOwnTeam*/ false);
	TestFalse(TEXT("ricordo: niente personaggio"), bOverlayForRemembered);
	TestTrue (TEXT("ricordo: una sagoma"),         GhostForRemembered.IsSet());
	TestTrue (TEXT("esattamente uno dei due, sul ricordo"),
		bOverlayForRemembered != GhostForRemembered.IsSet());
	if (GhostForRemembered.IsSet())
	{
		TestTrue (TEXT("la sagoma sta alla cella del CONTATTO"), GhostForRemembered->Cell == KvContactCell);
		TestFalse(TEXT("mai a quella attuale"),                  GhostForRemembered->Cell == KvActualCell);
	}

	// Visto ora: il personaggio, e nessuna sagoma a confonderlo.
	const bool bOverlayForLive = ARTHUD::ShouldDrawUnitOverlay(Live, /*bIsOwnTeam*/ false);
	const bool bGhostForLive = ARTHUD::ContactGhostTargetForUnit(Live, /*bIsOwnTeam*/ false).IsSet();
	TestTrue (TEXT("visto ora: il personaggio"), bOverlayForLive);
	TestFalse(TEXT("visto ora: nessuna sagoma"), bGhostForLive);
	TestTrue (TEXT("esattamente uno dei due, sul visto"), bOverlayForLive != bGhostForLive);
	return true;
}

namespace
{
	/** Una rotta con un verdetto per cella. Le maschere si passano come liste di `TeamId` ammessi. */
	FRTMoveRoute KvRoute(const TArray<FRTCellId>& Cells, const TArray<TArray<int32>>& AllowedPerCell)
	{
		FRTMoveRoute R;
		R.StableUnitId = 7;
		R.Cells = Cells;
		for (const TArray<int32>& Allowed : AllowedPerCell)
		{
			FRTKnowledgeVerdict V;
			for (int32 TeamId : Allowed) { V.AllowTeam(TeamId); }
			R.CellVerdicts.Add(V);
		}
		return R;
	}
}

/**
 * [D-223] — la traccia post-lock porta il tratto OSSERVATO, e si TRONCA dove l'osservatore ha perso il
 * soggetto.
 *
 * 🔴 **L'asserto che distingue `break` da `continue`, ed e' il cuore del test.** Con un buco in mezzo alla
 * rotta un filtro che *salta* le celle non ammesse restituirebbe la partenza E l'arrivo, e l'HUD unirebbe
 * due celle NON adiacenti con un segmento dritto — una linea tesa esattamente sopra il tratto da
 * nascondere. Troncare mostra meno; saltare mostra di piu' di quanto mostrasse il difetto originale.
 *
 * Statica e PURA: nessun HUD, nessun mondo, nessuna partita. E' la condizione che il DoD di `#1497` chiede,
 * perche' il disegno della traccia in `DrawHUD` non ha copertura headless e non puo' averne.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeVisibleTrailTruncatesTest,
	"RefactorTactics.Knowledge.VisibleTrailTruncatesAtLostSubject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeVisibleTrailTruncatesTest::RunTest(const FString&)
{
	const TArray<FRTCellId> Cells = {
		FRTCellId(0, 0, 0), FRTCellId(1, 0, 0), FRTCellId(2, 0, 0), FRTCellId(3, 0, 0) };

	// Il soggetto e' della squadra 1 e la 0 lo perde dopo due celle: e' il caso che PIE-KNOW4 osserva.
	const FRTMoveRoute Persa = KvRoute(Cells, { {0, 1}, {0, 1}, {1}, {1} });

	const TArray<FRTCellId> PerZero = ARTTurnManager::VisibleTrailFor(Persa, /*ObserverTeamId*/ 0);
	TestEqual(TEXT("l'osservatore vede il tratto che ha osservato, e finisce li'"), PerZero.Num(), 2);
	if (PerZero.Num() == 2)
	{
		TestTrue(TEXT("il tratto parte dalla partenza"), PerZero[0] == Cells[0]);
		TestTrue(TEXT("e finisce sull'ultima cella osservata"), PerZero[1] == Cells[1]);
	}
	TestFalse(TEXT("la cella d'arrivo non e' nel tratto: e' li' che la polilinea entrava nella nebbia"),
		PerZero.Contains(Cells[3]));

	// Anti-vacuita': il troncamento non e' «la traccia sparisce sempre». Chi ha percorso la rotta la vede
	// intera, e senza questa riga un `VisibleTrailFor` che rendesse sempre un array vuoto passerebbe.
	TestEqual(TEXT("la squadra del soggetto vede la propria rotta intera"),
		ARTTurnManager::VisibleTrailFor(Persa, /*ObserverTeamId*/ 1).Num(), Cells.Num());

	// 🔴 Il buco in MEZZO: partenza e arrivo osservati, il tratto centrale no. E' il caso comune —
	// `VisibleCells` e' un insieme bucato (LOS + cono + close range), non un raggio.
	const FRTMoveRoute Bucata = KvRoute(Cells, { {0}, {}, {}, {0} });
	const TArray<FRTCellId> Troncata = ARTTurnManager::VisibleTrailFor(Bucata, 0);
	TestEqual(TEXT("il tratto si ferma al buco: UNA cella, non tre"), Troncata.Num(), 1);
	TestFalse(TEXT("l'arrivo osservato NON viene ricucito alla partenza"), Troncata.Contains(Cells[3]));

	// Perso subito: niente da disegnare, nemmeno la partenza.
	TestEqual(TEXT("chi non ha mai visto il soggetto non vede nulla della sua rotta"),
		ARTTurnManager::VisibleTrailFor(KvRoute(Cells, { {1}, {1}, {1}, {1} }), 0).Num(), 0);

	// Fail-closed sul disallineamento: senza il controllo, questa chiamata leggerebbe fuori array.
	FRTMoveRoute Malformata = KvRoute(Cells, { {0}, {0} });
	TestEqual(TEXT("una rotta con meno verdetti che celle non si disegna affatto"),
		ARTTurnManager::VisibleTrailFor(Malformata, 0).Num(), 0);

	// E il default nasconde: una rotta raccolta senza verdetti e' silenziosa, non pubblica.
	FRTMoveRoute SenzaVerdetti;
	SenzaVerdetti.Cells = Cells;
	TestEqual(TEXT("nessun verdetto significa nessuno, non tutti"),
		ARTTurnManager::VisibleTrailFor(SenzaVerdetti, 0).Num(), 0);
	return true;
}

/**
 * [D-223] — il verdetto congelato dice CHI puo' leggere un fatto, e lo dice come lo direbbe `ViewForTeam`.
 *
 * Il caso monta due squadre che vedono cose diverse dello stesso soggetto, cosi' un verdetto che ignorasse
 * l'osservatore — o che accendesse tutti i bit — non passerebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeFreezeVerdictTest,
	"RefactorTactics.Knowledge.FrozenVerdictMatchesLiveVisibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeFreezeVerdictTest::RunTest(const FString&)
{
	const FRTCellId Cell(3, 0, 0);

	// Squadra 0 lo vede; squadra 1 non ne sa nulla; il soggetto e' della squadra 2.
	FRTTeamKnowledge Seeing = KvKnowledge({ FRTLastKnownContact(7, Cell, 5) });
	Seeing.TeamId = 0;
	Seeing.VisibleCells.Add(Cell);

	FRTTeamKnowledge Blind = KvKnowledge({});
	Blind.TeamId = 1;

	const FRTKnowledgeSubject Subject = KvSubject(7, /*TeamId*/ 2, Cell);
	const FRTKnowledgeVerdict Verdict =
		URTTeamKnowledgeLibrary::FreezeVerdict({ Seeing, Blind }, Subject);

	TestTrue (TEXT("chi lo vedeva puo' leggere"), Verdict.AllowsTeam(0));
	TestFalse(TEXT("chi non ne sapeva nulla non legge"), Verdict.AllowsTeam(1));

	// Anti-vacuita': una squadra che non compare affatto nella lista non riceve il bit per omissione.
	TestFalse(TEXT("una squadra assente dalla lista non legge"), Verdict.AllowsTeam(3));

	// 🔴 Il verdetto DEVE coincidere con cio' che `ViewForTeam` chiama `Live`, o sarebbe una seconda
	// definizione della stessa regola — la «terza via» che [D-223] vieta.
	const FRTKnowledgeView ViewSeeing = URTKnowledgeViewLibrary::ViewForTeam(Seeing, { Subject }, 0);
	const FRTKnowledgeEntry* EntrySeeing = URTKnowledgeViewLibrary::FindEntry(ViewSeeing, 7);
	const bool bLiveForSeeing = EntrySeeing != nullptr && EntrySeeing->Visibility == ERTKnowledgeVisibility::Live;
	TestEqual(TEXT("verdetto e vista concordano su chi vede"), Verdict.AllowsTeam(0), bLiveForSeeing);

	return true;
}

/**
 * Un soggetto solo RICORDATO non si legge: `CellOnly` non basta.
 *
 * ⚠️ **E' la tentazione da non seguire, e il test esiste per bloccarla.** Sembra generoso concedere a chi
 * «sa che esiste»: ma una riga di log stampa la cella del FATTO, e un ricordo contiene la cella
 * dell'ULTIMO contatto. Concedere qui regalerebbe proprio la posizione che il ricordo non ha.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeFreezeVerdictRejectsRememberedTest,
	"RefactorTactics.Knowledge.FrozenVerdictRejectsRemembered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeFreezeVerdictRejectsRememberedTest::RunTest(const FString&)
{
	// La squadra 0 ricorda l'unita' 3 dov'era, ma non vede dov'e' adesso: e' il caso `Remembered`.
	FRTTeamKnowledge K = KvKnowledge({ FRTLastKnownContact(3, KvContactCell, 5) });
	K.TeamId = 0;
	K.VisibleCells.Add(KvSeenCell); // guarda altrove

	const FRTKnowledgeSubject Remembered = KvSubject(3, /*TeamId*/ 1, KvActualCell);

	// La fixture non si presuppone: si verifica che sia davvero un `Remembered` passando dalla porta.
	const FRTKnowledgeView View = URTKnowledgeViewLibrary::ViewForTeam(K, { Remembered }, 0);
	const FRTKnowledgeEntry* Entry = URTKnowledgeViewLibrary::FindEntry(View, 3);
	if (!TestNotNull(TEXT("la porta produce una voce per il ricordo"), Entry)) { return false; }
	TestTrue(TEXT("ed e' Remembered, non Live"), Entry->Visibility == ERTKnowledgeVisibility::Remembered);

	const FRTKnowledgeVerdict Verdict = URTTeamKnowledgeLibrary::FreezeVerdict({ K }, Remembered);
	TestFalse(TEXT("un ricordo non concede la lettura"), Verdict.AllowsTeam(0));

	return true;
}

/**
 * Le due costanti nominate, e il default.
 *
 * Un verdetto non dichiarato deve nascondere, non mostrare: e' il contrario del default di `AddLogEvent`
 * che `#1499` rimuove, e la direzione e' deliberata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeVerdictDefaultsClosedTest,
	"RefactorTactics.Knowledge.VerdictDefaultsClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeVerdictDefaultsClosedTest::RunTest(const FString&)
{
	const FRTKnowledgeVerdict Silent;
	TestFalse(TEXT("il default non mostra a nessuno"), Silent.AllowsTeam(0));
	TestFalse(TEXT("nemmeno alla squadra 1"), Silent.AllowsTeam(1));
	TestTrue (TEXT("e NoOne() e' lo stesso valore"), Silent == FRTKnowledgeVerdict::NoOne());

	const FRTKnowledgeVerdict World = FRTKnowledgeVerdict::Everyone();
	TestTrue(TEXT("un fatto di mondo si legge dalla squadra 0"), World.AllowsTeam(0));
	TestTrue(TEXT("e dalla squadra 1"), World.AllowsTeam(1));
	TestTrue(TEXT("e dall'ultima rappresentabile"), World.AllowsTeam(FRTKnowledgeVerdict::MaxTeamId));

	// Fail-closed fuori intervallo: un osservatore che il verdetto non sa rappresentare non legge il bit
	// di qualcun altro. Vale anche per `Everyone`, che altrimenti sembrerebbe un permesso universale.
	TestFalse(TEXT("un TeamId negativo non legge"), World.AllowsTeam(-1));
	TestFalse(TEXT("un TeamId oltre il bound non legge"), World.AllowsTeam(FRTKnowledgeVerdict::MaxTeamId + 1));

	FRTKnowledgeVerdict Built;
	Built.AllowTeam(1);
	TestFalse(TEXT("AllowTeam accende solo il bit chiesto"), Built.AllowsTeam(0));
	TestTrue (TEXT("e quel bit e' acceso"), Built.AllowsTeam(1));

	return true;
}

/**
 * 🔴 Il contenuto POSITIVO di [D-223]: una riga scritta mentre il soggetto era visibile **resta leggibile**
 * dopo che e' diventato un ricordo.
 *
 * ⚠️ **Senza questo test la suite del log sarebbe di sola assenza**, e la semantica vecchia — filtrare in
 * lettura sulla conoscenza di adesso — potrebbe rientrare senza far diventare rosso niente. E' il gemello
 * di `CombatLogOmitsRemembered`: quello prova che cio' che non si sapeva resta nascosto, questo che cio'
 * che si sapeva non si perde.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeFrozenLineSurvivesForgettingTest,
	"RefactorTactics.Knowledge.FrozenLineSurvivesForgetting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeFrozenLineSurvivesForgettingTest::RunTest(const FString&)
{
	const FRTKnowledgeSubject Enemy = KvSubject(2, /*TeamId*/ 1, KvSeenCell);

	// TURNO N: la squadra 0 lo vede. La riga nasce adesso, e porta il verdetto di adesso.
	FRTTeamKnowledge Then = KvKnowledge({ FRTLastKnownContact(2, KvSeenCell, 5) });
	Then.VisibleCells.Add(KvSeenCell);
	const FRTCombatLogLine Line = KvLineFor(TEXT("Nemico: passo -> (3,0,L0)"), { Then }, Enemy);

	// Anti-vacuita': se la riga non fosse leggibile nemmeno adesso, il test sotto passerebbe per la ragione
	// sbagliata — e non misurerebbe nulla sul dimenticare.
	if (!TestTrue(TEXT("nel turno del fatto la riga si legge"),
		URTCombatLogLibrary::ComposeVisibleLogLines({ Line }, 0).Num() == 1))
	{
		return false;
	}

	// TURNO N+2: il contatto e' scaduto, la squadra non sa piu' nulla di lui. La CONOSCENZA e' cambiata;
	// la riga no — ed e' il punto della decisione.
	FRTTeamKnowledge Now = KvKnowledge({});
	Now.TurnNumber = 7;
	const FRTKnowledgeView ViewNow = URTKnowledgeViewLibrary::ViewForTeam(Now, { Enemy }, 0);
	TestNull(TEXT("adesso la porta non ha piu' una voce per lui"),
		URTKnowledgeViewLibrary::FindEntry(ViewNow, 2));

	const TArray<FString> Visible = URTCombatLogLibrary::ComposeVisibleLogLines({ Line }, /*ObserverTeamId*/ 0);
	TestEqual(TEXT("la riga di allora si legge ancora"), Visible.Num(), 1);

	// E resta filtrata per l'altra squadra: il congelamento non e' un lasciapassare universale.
	TestEqual(TEXT("ma non per chi non lo vedeva"),
		URTCombatLogLibrary::ComposeVisibleLogLines({ Line }, /*ObserverTeamId*/ 1).Num(), 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
