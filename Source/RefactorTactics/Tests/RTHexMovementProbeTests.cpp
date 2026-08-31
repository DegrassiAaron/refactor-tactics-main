#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapAsset.h"
#include "Pathfinding/RTHexPath.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * La SONDA DI MOVIMENTO dell'editor (#711): «dove arriva questa unita' con questo budget, e perche' quella
 * cella no».
 *
 * 🔑 **Perche' questi test stanno nel modulo runtime e non accanto al tool.** La sonda e' un consumer: il
 * reachable set e' di `ReachableCells`, il vocabolario del rifiuto e' di `ClassifyWaypointCell`, il percorso
 * e' quello del pathfinder canonico. Cio' che qui si aggiunge sono DUE composizioni — risalire `FromCell`
 * invece di ricercare, e separare «fuori budget» da «nessuna strada» — e sono regole, non presentazione.
 * Nel modulo editor sarebbero una seconda risposta alla stessa domanda.
 */
namespace
{
	/**
	 * Arena piatta di raggio N. Nomi distinti per file: questi helper vivono in un namespace ANONIMO, e nella
	 * unity build finiscono nella stessa unita' di traduzione dei loro vicini — dove un omonimo con la stessa
	 * firma e' una ridefinizione, non un override.
	 */
	URTHexMapAsset* MakeProbeMap(int32 Radius)
	{
		return URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
	}

	const FRTHexReachableCell* FindInProbeSet(const TArray<FRTHexReachableCell>& Set, const FRTCellId& Id)
	{
		return Set.FindByPredicate([&Id](const FRTHexReachableCell& R) { return R.Cell == Id; });
	}
}

/**
 * Il percorso mostrato in hover si ricostruisce RISALENDO `FromCell`, e coincide con quello che il pathfinder
 * canonico produrrebbe.
 *
 * 🔴 E' il criterio portante di #711: l'alternativa — una `FindPathForUnit` per ogni cella sorvolata — e' una
 * seconda ricerca per fotogramma, e il giorno in cui diverge dalla prima la sonda mostra un percorso che
 * l'unita' non fara'. L'oracolo qui NON e' un percorso scritto a mano: e' `FindPathForUnit`, cioe' l'autorita'
 * stessa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexProbeHoverPathTest,
	"RefactorTactics.MovementProbe.HoverPathComesFromTheReachableSet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexProbeHoverPathTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeProbeMap(3);
	const FRTCellId Start(0, 0, 0);
	const FRTCellId Goal(2, 0, 0);
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(Map, { FRTHexSimUnit(1, Start, /*MoveBudget=*/ 4) });

	const TArray<FRTHexReachableCell> Set = URTHexSimLibrary::ReachableCells(Snap, 1);
	const TArray<FRTCellId> Hover = URTHexSimLibrary::ProbePathTo(Set, Goal);
	const FRTHexPathResult Authority = URTHexSimLibrary::FindPathForUnit(Snap, 1, Goal);

	TestTrue(TEXT("il pathfinder canonico ci arriva"), Authority.Status == ERTHexPathStatus::Success);
	TestEqual(TEXT("stessa lunghezza del percorso canonico"), Hover.Num(), Authority.Path.Num());
	TestTrue(TEXT("stesse celle, nello stesso ordine"), Hover == Authority.Path);
	TestTrue(TEXT("parte dalla cella dell'unita'"), Hover.Num() > 0 && Hover[0] == Start);
	TestTrue(TEXT("finisce sulla cella sorvolata"), Hover.Num() > 0 && Hover.Last() == Goal);

	const FRTHexReachableCell* Reached = FindInProbeSet(Set, Goal);
	TestTrue(TEXT("il costo cumulato e' quello del percorso canonico"),
		Reached != nullptr && Reached->Cost == Authority.TotalCost);

	// Una cella che nel set non c'e' non ha un percorso da mostrare: meglio niente che un percorso inventato.
	TestEqual(TEXT("cella fuori dal set -> nessun percorso"),
		URTHexSimLibrary::ProbePathTo(Set, FRTCellId(9, 9, 0)).Num(), 0);
	return true;
}

/**
 * «Non ci arrivo perche' non ho abbastanza movimento» e «non ci arrivo perche' non esiste una strada» sono due
 * risposte diverse, e la sonda non le accorpa.
 *
 * ⚠️ `ClassifyWaypointCell` risponde `Ok` a entrambe — e' documentato: *«un eventuale rifiuto del percorso e'
 * questione di budget»*. Per il designer che chiede *perche' quella cella no*, `Ok` non e' una risposta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexProbeOutOfBudgetTest,
	"RefactorTactics.MovementProbe.OutOfBudgetIsNotTheSameAsNoRoute",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexProbeOutOfBudgetTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeProbeMap(4);
	const FRTCellId Start(0, 0, 0);
	const FRTCellId Far(4, 0, 0); // distanza 4 su celle a costo 1
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(Map, { FRTHexSimUnit(1, Start, /*MoveBudget=*/ 2) });

	const TArray<FRTHexReachableCell> Set = URTHexSimLibrary::ReachableCells(Snap, 1);
	TestTrue(TEXT("la cella lontana non e' nel set"), FindInProbeSet(Set, Far) == nullptr);
	TestTrue(TEXT("il vecchio vocabolario la dichiara 'Ok', ed e' il motivo per cui questa sonda esiste"),
		URTHexSimLibrary::ClassifyWaypointCell(Snap, 1, Far) == ERTHexWaypointReason::Ok);

	TestTrue(TEXT("la sonda dice FUORI BUDGET"),
		URTHexSimLibrary::ClassifyProbeCell(Snap, 1, Set, Far) == ERTHexProbeExclusion::OutOfBudget);
	return true;
}

/**
 * Una cella libera che nessun percorso raggiunge — a qualunque budget — e' `NoRoute`, non `OutOfBudget`. Dire
 * «ti manca movimento» a chi guarda un'isola manda il designer ad alzare un budget che non servira'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexProbeNoRouteTest,
	"RefactorTactics.MovementProbe.NoRouteWhenTheGraphIsCut",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexProbeNoRouteTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeProbeMap(2);

	// Un'isola: una cella nella mappa, non adiacente a nessuna delle altre e senza transizioni.
	const FRTCellId Island(7, 7, 0);
	Map->AddOrUpdateCell(FRTHexCellData(Island));
	Map->SortCells();

	const FRTCellId Start(0, 0, 0);
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(Map, { FRTHexSimUnit(1, Start, /*MoveBudget=*/ 99) });
	const TArray<FRTHexReachableCell> Set = URTHexSimLibrary::ReachableCells(Snap, 1);

	TestTrue(TEXT("l'isola e' nella mappa"), Map->ContainsCell(Island));
	TestTrue(TEXT("l'isola non e' nel set"), FindInProbeSet(Set, Island) == nullptr);
	TestTrue(TEXT("la sonda dice NESSUNA STRADA, non 'fuori budget'"),
		URTHexSimLibrary::ClassifyProbeCell(Snap, 1, Set, Island) == ERTHexProbeExclusion::NoRoute);
	return true;
}

/**
 * I tre motivi che il repository ha gia' non vengono riscritti: la sonda li DELEGA, uno per ragione. Un secondo
 * elenco di ragioni per la stessa esclusione diverge, ed e' il divieto esplicito di #711.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexProbeVocabularyTest,
	"RefactorTactics.MovementProbe.ExclusionKeepsTheExistingVocabulary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexProbeVocabularyTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeProbeMap(3);

	FRTHexCellData Wall(FRTCellId(1, 0, 0));
	Wall.bBlocksMovement = true;
	Map->AddOrUpdateCell(Wall);
	Map->SortCells();

	const FRTCellId Mine(0, 0, 0);
	const FRTCellId Other(0, 1, 0);
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(Map, {
		FRTHexSimUnit(1, Mine,  /*MoveBudget=*/ 4),
		FRTHexSimUnit(2, Other, /*MoveBudget=*/ 0)
	});
	const TArray<FRTHexReachableCell> Set = URTHexSimLibrary::ReachableCells(Snap, 1);

	TestTrue(TEXT("fuori mappa -> NotOnMap"),
		URTHexSimLibrary::ClassifyProbeCell(Snap, 1, Set, FRTCellId(9, 9, 0)) == ERTHexProbeExclusion::NotOnMap);
	TestTrue(TEXT("ostacolo -> BlocksMovement"),
		URTHexSimLibrary::ClassifyProbeCell(Snap, 1, Set, FRTCellId(1, 0, 0)) == ERTHexProbeExclusion::BlocksMovement);
	TestTrue(TEXT("occupata da un'altra unita' -> Occupied"),
		URTHexSimLibrary::ClassifyProbeCell(Snap, 1, Set, Other) == ERTHexProbeExclusion::Occupied);
	TestTrue(TEXT("una cella DEL set non ha niente da spiegare"),
		URTHexSimLibrary::ClassifyProbeCell(Snap, 1, Set, Mine) == ERTHexProbeExclusion::Reachable);
	return true;
}

/**
 * Cambiando il budget cambia il set, e la stessa cella cambia risposta: e' la domanda che il designer fa
 * davvero — «e se gli dessi un punto in piu'?».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexProbeBudgetTest,
	"RefactorTactics.MovementProbe.BudgetChangesTheSetAndTheAnswer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexProbeBudgetTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeProbeMap(4);
	const FRTCellId Start(0, 0, 0);
	const FRTCellId Edge(3, 0, 0);

	const FRTHexSnapshot Tight = URTHexSimLibrary::MakeSnapshot(Map, { FRTHexSimUnit(1, Start, /*MoveBudget=*/ 2) });
	const FRTHexSnapshot Loose = URTHexSimLibrary::MakeSnapshot(Map, { FRTHexSimUnit(1, Start, /*MoveBudget=*/ 3) });

	const TArray<FRTHexReachableCell> SetTight = URTHexSimLibrary::ReachableCells(Tight, 1);
	const TArray<FRTHexReachableCell> SetLoose = URTHexSimLibrary::ReachableCells(Loose, 1);

	TestTrue(TEXT("col budget piu' alto il set e' piu' grande"), SetLoose.Num() > SetTight.Num());
	TestTrue(TEXT("con budget 2 la cella e' fuori budget"),
		URTHexSimLibrary::ClassifyProbeCell(Tight, 1, SetTight, Edge) == ERTHexProbeExclusion::OutOfBudget);
	TestTrue(TEXT("con budget 3 la stessa cella e' raggiungibile"),
		URTHexSimLibrary::ClassifyProbeCell(Loose, 1, SetLoose, Edge) == ERTHexProbeExclusion::Reachable);
	return true;
}

/**
 * Dipinta una superficie piu' cara, il set cambia — e cio' che dice alla sonda che deve rifarlo e' la
 * REVISIONE dell'asset, non un timer.
 *
 * 🔴 E' il criterio di #711 sull'aggiornamento, e la forma conta: *«un test lo dimostra sulla revisione
 * dell'asset, non su un refresh a tempo»*. Una sonda che si ridisegnasse a intervalli sarebbe verde qui e
 * sbagliata in mano al designer, che dipinge una cella e guarda subito il ventaglio.
 *
 * ⚠️ Il meccanismo NON e' nuovo: `IsSnapshotStale` esiste ed e' quello che la risoluzione di un turno usa
 * per rifiutare uno snapshot invecchiato. La sonda ne e' un secondo consumer.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexProbeSurfaceEditTest,
	"RefactorTactics.MovementProbe.SurfaceEditInvalidatesTheSetByRevision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexProbeSurfaceEditTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeProbeMap(3);
	const FRTCellId Start(0, 0, 0);
	const FRTCellId Beyond(2, 0, 0); // due passi a costo 1: dentro un budget di 2

	const FRTHexSnapshot Before = URTHexSimLibrary::MakeSnapshot(Map, { FRTHexSimUnit(1, Start, /*MoveBudget=*/ 2) });
	const TArray<FRTHexReachableCell> SetBefore = URTHexSimLibrary::ReachableCells(Before, 1);
	TestTrue(TEXT("prima della modifica la cella e' raggiungibile"),
		URTHexSimLibrary::ClassifyProbeCell(Before, 1, SetBefore, Beyond) == ERTHexProbeExclusion::Reachable);
	TestTrue(TEXT("e il suo snapshot e' fresco"), !URTHexSimLibrary::IsSnapshotStale(Before));

	// Il designer dipinge una superficie piu' cara sulla cella di passaggio.
	FRTHexCellData Mud = *Map->FindCell(FRTCellId(1, 0, 0));
	Mud.MoveCost = 3;
	Map->AddOrUpdateCell(Mud);
	Map->SortCells();

	TestTrue(TEXT("la modifica rende STANTIO lo snapshot: e' la revisione a dirlo"),
		URTHexSimLibrary::IsSnapshotStale(Before));

	const FRTHexSnapshot After = URTHexSimLibrary::MakeSnapshot(Map, { FRTHexSimUnit(1, Start, /*MoveBudget=*/ 2) });
	const TArray<FRTHexReachableCell> SetAfter = URTHexSimLibrary::ReachableCells(After, 1);
	TestTrue(TEXT("rifatto il set, la stessa cella e' fuori budget"),
		URTHexSimLibrary::ClassifyProbeCell(After, 1, SetAfter, Beyond) == ERTHexProbeExclusion::OutOfBudget);
	TestTrue(TEXT("il set si e' ristretto"), SetAfter.Num() < SetBefore.Num());
	return true;
}

/**
 * Nessuna unita' selezionata: la sonda non ha un soggetto, e non inventa un motivo.
 *
 * ⚠️ Non e' un caso di laboratorio — nell'editor e' lo stato NORMALE all'apertura del tool, e con un
 * `UnitId` che non esiste `ClassifyWaypointCell` risponde `Ok` a ogni cella libera: senza questo ramo la
 * sonda direbbe «fuori budget» del budget di nessuno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexProbeNoUnitTest,
	"RefactorTactics.MovementProbe.NoSelectedUnitMeansNoRoute",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexProbeNoUnitTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeProbeMap(2);
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(Map, { FRTHexSimUnit(1, FRTCellId(0, 0, 0), 3) });

	// 42 non esiste nello snapshot: il set che le corrisponde e' vuoto, ed e' l'unica risposta onesta.
	const TArray<FRTHexReachableCell> Empty = URTHexSimLibrary::ReachableCells(Snap, 42);
	TestEqual(TEXT("un'unita' sconosciuta non raggiunge niente"), Empty.Num(), 0);
	TestTrue(TEXT("e nessuna cella e' 'fuori budget' di un budget che non esiste"),
		URTHexSimLibrary::ClassifyProbeCell(Snap, 42, Empty, FRTCellId(1, 1, 0)) == ERTHexProbeExclusion::NoRoute);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
