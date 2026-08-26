#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapAsset.h"
#include "Perception/RTTeamKnowledge.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * CP 13.2 — il targeting CONSUMA la conoscenza (#157).
 *
 * CP 13.1 aveva costruito la vista; qui la vista comincia a **decidere**. Le quattro proprieta' della DoD
 * sono quattro test perche' sono quattro regole indipendenti, e verificarne tre lascerebbe la quarta libera
 * di essere sbagliata senza che nulla lo dica.
 *
 * Tutto puro e headless: nessun Actor, nessun `UWorld`. La regola vive in `URTTeamKnowledgeLibrary`, e il
 * resolver la chiama — non la riscrive. E' la lezione di #507, applicata prima invece che dopo.
 */

namespace
{
	/** Arena piena di raggio N sul layer 0. Nome distinto per file: la unity build condivide la TU. */
	URTHexMapAsset* MakeKnowledgeMap(int32 Radius)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		return M;
	}

	FRTPerceiver KnowledgeWatcher(const FRTCellId& Cell, ERTHexDirection Facing, int32 VisionRange = 5)
	{
		FRTPerceiver P;
		P.Cell = Cell;
		P.Facing = Facing;
		P.VisionRange = VisionRange;
		return P;
	}

	/** Un avversario dove si trova ORA: il `TurnNumber` in ingresso lo ignora `Observe`. */
	FRTLastKnownContact EnemyAt(int32 StableUnitId, const FRTCellId& Cell)
	{
		return FRTLastKnownContact(StableUnitId, Cell, /*ignorato*/ 0);
	}

	/** Squadra 0 con un solo osservatore, che guarda a est dall'origine. */
	FRTTeamKnowledge ObserveEast(const URTHexMapAsset* Map, int32 TurnNumber,
		const TArray<FRTLastKnownContact>& Enemies, const FRTTeamKnowledge& Previous)
	{
		const TArray<FRTPerceiver> Team = { KnowledgeWatcher(FRTCellId(0, 0), ERTHexDirection::E) };
		return URTTeamKnowledgeLibrary::Observe(Map, /*TeamId*/ 0, TurnNumber, Team, Enemies, Previous);
	}
}

/**
 * Prima regola: **non si spara a cio' che la squadra non conosce.**
 *
 * Il caso e' costruito perche' la GEOMETRIA sia perfetta — bersaglio a portata, nessun muro, nessun fumo — e
 * a mancare sia solo la conoscenza. Senza questa cura il test passerebbe anche con una regola che rifiuta per
 * distanza, e non direbbe niente su cio' che deve verificare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeCannotTargetUnknownTest,
	"RefactorTactics.Vision.CannotTargetUnknown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeCannotTargetUnknownTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeKnowledgeMap(8);

	// Il nemico #7 sta DIETRO l'osservatore, fuori dall'arco frontale e oltre la consapevolezza ravvicinata.
	const FRTCellId Behind(-5, 0);
	const FRTTeamKnowledge Knowledge = ObserveEast(Map, /*Turno*/ 1, { EnemyAt(7, Behind) }, FRTTeamKnowledge());

	// Premessa: la squadra davvero non lo vede. Se lo vedesse, il rifiuto qui sotto sarebbe una coincidenza.
	if (!TestFalse(TEXT("premessa: la cella del nemico non e' vista"), Knowledge.VisibleCells.Contains(Behind)))
	{
		return false;
	}
	TestEqual(TEXT("nessun ricordo di lui: e' Hidden"),
		static_cast<int32>(URTTeamKnowledgeLibrary::AwarenessOfUnit(Knowledge, 7, Behind)),
		static_cast<int32>(ERTAwareness::Hidden));

	TestEqual(TEXT("un bersaglio ignoto e' RIFIUTATO"),
		static_cast<int32>(URTTeamKnowledgeLibrary::ClassifyTarget(Knowledge, 7, /*TeamNemico*/ 1, Behind)),
		static_cast<int32>(ERTTargetKnowledge::Rejected));

	// Il gemello di controllo, nella stessa conoscenza: un nemico VISTO resta bersagliabile. Senza, il test
	// passerebbe anche con una regola che rifiuta sempre — cioe' con il gioco rotto.
	const FRTCellId Ahead(3, 0);
	const FRTTeamKnowledge Seen = ObserveEast(Map, 1, { EnemyAt(9, Ahead) }, FRTTeamKnowledge());
	if (!TestTrue(TEXT("premessa del controllo: questa cella e' vista"), Seen.VisibleCells.Contains(Ahead)))
	{
		return false;
	}
	TestEqual(TEXT("un bersaglio visto e' AMMESSO"),
		static_cast<int32>(URTTeamKnowledgeLibrary::ClassifyTarget(Seen, 9, 1, Ahead)),
		static_cast<int32>(ERTTargetKnowledge::Allowed));

	// Un ALLEATO non passa dalla conoscenza: si cura chi si conosce per appartenenza. Verificato sulla stessa
	// cella che per un nemico era rifiutata, cosi' l'unica variabile e' la squadra.
	TestEqual(TEXT("un alleato fuori vista resta bersagliabile (cure, scudi)"),
		static_cast<int32>(URTTeamKnowledgeLibrary::ClassifyTarget(Knowledge, 7, /*TeamProprio*/ 0, Behind)),
		static_cast<int32>(ERTTargetKnowledge::Allowed));
	return true;
}

/**
 * Seconda regola: un contatto **incerto** e' bersagliabile per CELLA, mai per unita'.
 *
 * La differenza non e' formale. Mirare all'unita' significa che il colpo la SEGUE dove si e' spostata, cioe'
 * consuma una posizione che la squadra non conosce; mirare alla cella significa colpire dove il ricordo dice,
 * e trovare terra battuta se il bersaglio si e' mosso. La seconda e' conoscenza parziale, la prima e' una
 * fuga di posizione con un altro nome.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeUncertainTargetsCellTest,
	"RefactorTactics.Vision.UncertainTargetsCellNotUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeUncertainTargetsCellTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeKnowledgeMap(8);

	// Turno 1: il nemico #4 e' in vista, davanti.
	const FRTCellId SeenAt(3, 0);
	const FRTTeamKnowledge T1 = ObserveEast(Map, 1, { EnemyAt(4, SeenAt) }, FRTTeamKnowledge());
	if (!TestTrue(TEXT("premessa: al turno 1 e' visto"), T1.VisibleCells.Contains(SeenAt))) { return false; }

	// Turno 2: si e' spostato DIETRO l'osservatore. Non e' piu' visto, ma il ricordo del turno 1 e' ancora vivo.
	const FRTCellId MovedTo(-5, 0);
	const FRTTeamKnowledge T2 = ObserveEast(Map, 2, { EnemyAt(4, MovedTo) }, T1);
	if (!TestFalse(TEXT("premessa: al turno 2 non e' piu' visto"), T2.VisibleCells.Contains(MovedTo)))
	{
		return false;
	}

	TestEqual(TEXT("resta un contatto INCERTO, non ignoto"),
		static_cast<int32>(URTTeamKnowledgeLibrary::AwarenessOfUnit(T2, 4, MovedTo)),
		static_cast<int32>(ERTAwareness::Uncertain));
	TestEqual(TEXT("bersagliabile per CELLA, non per unita'"),
		static_cast<int32>(URTTeamKnowledgeLibrary::ClassifyTarget(T2, 4, 1, MovedTo)),
		static_cast<int32>(ERTTargetKnowledge::CellOnly));

	// E la cella e' quella del RICORDO, non quella attuale. E' l'assertion che vale l'intero test: se qui
	// comparisse `MovedTo`, la memoria starebbe seguendo l'unita' e non ricordandola.
	FRTCellId Aimed;
	if (TestTrue(TEXT("il ricordo ha una cella"), URTTeamKnowledgeLibrary::LastKnownCell(T2, 4, Aimed)))
	{
		TestTrue(TEXT("si mira DOVE ERA, non dove e'"), Aimed == SeenAt);
		TestFalse(TEXT("la posizione attuale non trapela"), Aimed == MovedTo);
	}
	return true;
}

/**
 * Terza regola: **l'avvistamento di un alleato estende il targeting della squadra.**
 *
 * Non e' una regola in piu': e' una conseguenza di dove vive il dato (D-043, la conoscenza e' di squadra). Il
 * test la fissa lo stesso, perche' una futura ottimizzazione «ognuno vede per se'» la romperebbe in silenzio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeAllySpottingExtendsTest,
	"RefactorTactics.Vision.AllySpottingExtendsTargeting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeAllySpottingExtendsTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeKnowledgeMap(8);
	const FRTCellId Enemy(-5, 0); // dietro il primo osservatore, davanti al secondo

	// Da solo, chi guarda a est non lo vede: e' il caso gia' fissato da `CannotTargetUnknown`.
	const FRTTeamKnowledge Alone = ObserveEast(Map, 1, { EnemyAt(7, Enemy) }, FRTTeamKnowledge());
	if (!TestEqual(TEXT("premessa: da solo non lo puo' bersagliare"),
			static_cast<int32>(URTTeamKnowledgeLibrary::ClassifyTarget(Alone, 7, 1, Enemy)),
			static_cast<int32>(ERTTargetKnowledge::Rejected)))
	{
		return false;
	}

	// Stesso osservatore, piu' un COMPAGNO che guarda a ovest. L'attaccante non ha cambiato posizione ne'
	// orientamento: cambia solo che la squadra, nel suo insieme, adesso vede.
	const TArray<FRTPerceiver> Pair = {
		KnowledgeWatcher(FRTCellId(0, 0), ERTHexDirection::E),
		KnowledgeWatcher(FRTCellId(-2, 0), ERTHexDirection::W)
	};
	const FRTTeamKnowledge Together = URTTeamKnowledgeLibrary::Observe(Map, 0, 1, Pair,
		{ EnemyAt(7, Enemy) }, FRTTeamKnowledge());

	TestTrue(TEXT("la squadra ora vede quella cella"), Together.VisibleCells.Contains(Enemy));
	TestEqual(TEXT("e chiunque della squadra puo' bersagliarlo"),
		static_cast<int32>(URTTeamKnowledgeLibrary::ClassifyTarget(Together, 7, 1, Enemy)),
		static_cast<int32>(ERTTargetKnowledge::Allowed));

	// Invariante #3: permutare gli osservatori non cambia nulla. `TeamVisibleCells` passa da un TSet, il cui
	// ordine dipende dall'hash — senza l'ordinamento canonico questa uguaglianza salterebbe.
	const TArray<FRTPerceiver> Swapped = { Pair[1], Pair[0] };
	const FRTTeamKnowledge Other = URTTeamKnowledgeLibrary::Observe(Map, 0, 1, Swapped,
		{ EnemyAt(7, Enemy) }, FRTTeamKnowledge());
	TestEqual(TEXT("permutare gli osservatori non cambia la conoscenza"),
		Other.VisibleCells, Together.VisibleCells);
	return true;
}

/**
 * Quarta regola: il ricordo **scade dopo un turno**.
 *
 * E' la meta' che rende la memoria un'informazione e non un secondo canale di vista permanente. Il test la
 * misura sul confine — vivo al turno successivo, morto a quello dopo — perche' e' li' che un `<` scritto `<=`
 * cambia la regola senza rompere nulla di visibile.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeLastContactExpiresTest,
	"RefactorTactics.Vision.LastContactExpiresAfterOneTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeLastContactExpiresTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeKnowledgeMap(8);
	const FRTCellId SeenAt(3, 0);
	const FRTCellId Hiding(-5, 0);

	// Turno 1 — visto.
	const FRTTeamKnowledge T1 = ObserveEast(Map, 1, { EnemyAt(4, SeenAt) }, FRTTeamKnowledge());
	TestEqual(TEXT("turno 1: un contatto"), T1.Contacts.Num(), 1);
	TestEqual(TEXT("turno 1: Detected"),
		static_cast<int32>(URTTeamKnowledgeLibrary::AwarenessOfUnit(T1, 4, SeenAt)),
		static_cast<int32>(ERTAwareness::Detected));

	// Turno 2 — sparito dalla vista: il ricordo REGGE. E' la persistenza «1 turno».
	const FRTTeamKnowledge T2 = ObserveEast(Map, 2, { EnemyAt(4, Hiding) }, T1);
	TestEqual(TEXT("turno 2: il ricordo e' ancora vivo"), T2.Contacts.Num(), 1);
	TestEqual(TEXT("turno 2: Uncertain"),
		static_cast<int32>(URTTeamKnowledgeLibrary::AwarenessOfUnit(T2, 4, Hiding)),
		static_cast<int32>(ERTAwareness::Uncertain));

	// Turno 3 — il ricordo SCADE: due turni dopo l'avvistamento non resta niente.
	const FRTTeamKnowledge T3 = ObserveEast(Map, 3, { EnemyAt(4, Hiding) }, T2);
	TestEqual(TEXT("turno 3: nessun ricordo"), T3.Contacts.Num(), 0);
	TestEqual(TEXT("turno 3: torna Hidden"),
		static_cast<int32>(URTTeamKnowledgeLibrary::AwarenessOfUnit(T3, 4, Hiding)),
		static_cast<int32>(ERTAwareness::Hidden));
	TestEqual(TEXT("turno 3: non e' piu' un bersaglio"),
		static_cast<int32>(URTTeamKnowledgeLibrary::ClassifyTarget(T3, 4, 1, Hiding)),
		static_cast<int32>(ERTTargetKnowledge::Rejected));

	// Rivedendolo, il ricordo RINASCE fresco e con la cella NUOVA: un contatto attuale sovrascrive la
	// memoria, invece di affiancarsi e lasciare due verita' sulla stessa unita'.
	const FRTTeamKnowledge T4 = ObserveEast(Map, 4, { EnemyAt(4, SeenAt) }, T3);
	TestEqual(TEXT("turno 4: rivisto, un solo contatto"), T4.Contacts.Num(), 1);
	TestEqual(TEXT("turno 4: e il contatto e' di questo turno"), T4.Contacts[0].TurnNumber, 4);

	// ⚠️ Una conoscenza di VERSIONE ignota si scarta invece di essere reinterpretata. Senza questo ramo un
	// campo aggiunto domani produrrebbe ricordi plausibili e sbagliati, che nessuno segnalerebbe.
	FRTTeamKnowledge Futura = T4;
	Futura.Version = FRTTeamKnowledge::CurrentVersion + 1;
	const FRTTeamKnowledge FromFuture = ObserveEast(Map, 5, { EnemyAt(4, Hiding) }, Futura);
	TestEqual(TEXT("una memoria di versione ignota non viene ereditata"), FromFuture.Contacts.Num(), 0);
	TestEqual(TEXT("e leggerla direttamente e' fail-closed"),
		static_cast<int32>(URTTeamKnowledgeLibrary::AwarenessOfUnit(Futura, 4, SeenAt)),
		static_cast<int32>(ERTAwareness::Hidden));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
