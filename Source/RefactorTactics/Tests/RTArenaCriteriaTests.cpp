#include "Misc/AutomationTest.h"
#include "Algo/Reverse.h"
#include "Map/RTArenaCriteriaLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Pathfinding/RTHexPathLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Esagono pieno di raggio R sul layer 0, tutto pavimento a costo 1. Punto di partenza di ogni fixture. */
	URTHexMapAsset* MakeFlatHex(int32 Radius)
	{
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			Map->AddOrUpdateCell(FRTHexCellData(Id));
		}
		Map->SortCells();
		return Map;
	}

	/** Rende la cella un muro che blocca la vista (ma non il passo: la copertura non e' un ostacolo). */
	void SetBlocksLoS(URTHexMapAsset* Map, const FRTCellId& Id)
	{
		FRTHexCellData Cell = Map->FindCell(Id) ? *Map->FindCell(Id) : FRTHexCellData(Id);
		Cell.bBlocksLineOfSight = true;
		Map->AddOrUpdateCell(Cell);
		Map->SortCells();
	}

	/** Alza il costo di movimento della cella. */
	void SetCost(URTHexMapAsset* Map, const FRTCellId& Id, int32 Cost)
	{
		FRTHexCellData Cell = Map->FindCell(Id) ? *Map->FindCell(Id) : FRTHexCellData(Id);
		Cell.MoveCost = Cost;
		Map->AddOrUpdateCell(Cell);
		Map->SortCells();
	}

	/** Rende la cella un ostacolo pieno. */
	void SetBlocksMove(URTHexMapAsset* Map, const FRTCellId& Id)
	{
		FRTHexCellData Cell = Map->FindCell(Id) ? *Map->FindCell(Id) : FRTHexCellData(Id);
		Cell.bBlocksMovement = true;
		Map->AddOrUpdateCell(Cell);
		Map->SortCells();
	}
}

/**
 * Il criterio della copertura non si accontenta di CONTARE le celle che bloccano la vista: chiede che la
 * linea di tiro sia davvero interrotta.
 *
 * La differenza non e' teorica. Gli estremi non bloccano mai (`URTHexVisionLibrary::HasLineOfSight`), quindi
 * due muri messi SUGLI spawn soddisfano il conteggio e non coprono niente: il criterio conterebbe
 * l'intenzione invece del risultato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTArenaCoverageTest,
	"RefactorTactics.Arena.CoverageNeedsBlockedSightNotJustBlockingCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTArenaCoverageTest::RunTest(const FString&)
{
	const FRTCellId A(-3, 0, 0);
	const FRTCellId B(3, 0, 0);

	// 1. Campo aperto: nessuna copertura -> non passa.
	{
		URTHexMapAsset* Map = MakeFlatHex(4);
		const FRTArenaCriterionResult R = URTArenaCriteriaLibrary::CheckCoverage(Map, A, B);
		TestFalse(TEXT("campo aperto -> niente copertura"), R.bPassed);
		TestFalse(TEXT("ed e' un verdetto, non un'impossibilita'"), R.bNotEvaluable);
	}

	// 2. Un solo muro sulla linea: la vista e' interrotta, ma il criterio ne chiede DUE.
	{
		URTHexMapAsset* Map = MakeFlatHex(4);
		SetBlocksLoS(Map, FRTCellId(0, 0, 0));
		const FRTArenaCriterionResult R = URTArenaCriteriaLibrary::CheckCoverage(Map, A, B);
		TestFalse(TEXT("un muro solo -> non basta"), R.bPassed);
	}

	// 3. Due muri sulla linea -> passa.
	{
		URTHexMapAsset* Map = MakeFlatHex(4);
		SetBlocksLoS(Map, FRTCellId(-1, 0, 0));
		SetBlocksLoS(Map, FRTCellId(1, 0, 0));
		const FRTArenaCriterionResult R = URTArenaCriteriaLibrary::CheckCoverage(Map, A, B);
		TestTrue(TEXT("due muri sulla linea -> passa"), R.bPassed);
	}

	// 4. Il caso che il conteggio da solo sbaglierebbe: due muri SUGLI spawn.
	//    Sono due celle bBlocksLineOfSight sul segmento (estremi inclusi), e non coprono nulla.
	{
		URTHexMapAsset* Map = MakeFlatHex(4);
		SetBlocksLoS(Map, A);
		SetBlocksLoS(Map, B);
		const FRTArenaCriterionResult R = URTArenaCriteriaLibrary::CheckCoverage(Map, A, B);
		TestFalse(TEXT("muri sugli estremi -> la vista e' libera, non passa"), R.bPassed);
	}

	// 5. Due muri, ma FUORI dalla linea: esistono nella mappa e non coprono quella traiettoria.
	{
		URTHexMapAsset* Map = MakeFlatHex(4);
		SetBlocksLoS(Map, FRTCellId(0, 3, 0));
		SetBlocksLoS(Map, FRTCellId(0, -3, 0));
		const FRTArenaCriterionResult R = URTArenaCriteriaLibrary::CheckCoverage(Map, A, B);
		TestFalse(TEXT("muri fuori dal segmento -> non passa"), R.bPassed);
	}
	return true;
}

/**
 * Il criterio del costo chiede DUE cose insieme, e ciascuna da sola e' insufficiente.
 *
 * Un percorso caro perche' semplicemente lungo non mette davanti a nessuna scelta; una zona cara che si
 * aggira senza rinunciare a niente nemmeno. Il criterio le pretende entrambe: una cella > 1 sul percorso
 * ottimale **e** un costo che supera il budget del turno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTArenaCostBiteTest,
	"RefactorTactics.Arena.CostBiteNeedsExpensiveCellAndOverBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTArenaCostBiteTest::RunTest(const FString&)
{
	const FRTCellId A(-2, 0, 0);
	const FRTCellId B(2, 0, 0);

	// 1. Corridoio corto e tutto a costo 1: 4 passi, budget 5 -> il budget non morde.
	{
		URTHexMapAsset* Map = MakeFlatHex(3);
		const FRTArenaCriterionResult R = URTArenaCriteriaLibrary::CheckCostBite(Map, A, B, /*MoveBudget=*/ 5);
		TestFalse(TEXT("tutto a costo 1 entro budget -> non morde"), R.bPassed);
	}

	// 2. Lungo ma tutto a costo 1: supera il budget, e non c'e' nessuna scelta da fare.
	//    Il costo alto viene dalla distanza, non dal terreno: il criterio deve rifiutarlo.
	{
		URTHexMapAsset* Map = MakeFlatHex(6);
		const FRTCellId Far1(-6, 0, 0);
		const FRTCellId Far2(6, 0, 0);
		const FRTArenaCriterionResult R = URTArenaCriteriaLibrary::CheckCostBite(Map, Far1, Far2, /*MoveBudget=*/ 5);
		TestFalse(TEXT("caro solo perche' lungo -> non e' una scelta"), R.bPassed);
	}

	// 3. Zona costosa che si AGGIRA: il percorso ottimale la evita, quindi non morde niente.
	{
		URTHexMapAsset* Map = MakeFlatHex(3);
		SetCost(Map, FRTCellId(0, 0, 0), 9);
		const FRTArenaCriterionResult R = URTArenaCriteriaLibrary::CheckCostBite(Map, A, B, /*MoveBudget=*/ 5);
		TestFalse(TEXT("zona cara aggirabile -> il percorso ottimale non la tocca"), R.bPassed);
	}

	// 4. Fango su TUTTA la strettoia: non aggirabile, e il costo supera il budget -> morde.
	{
		URTHexMapAsset* Map = MakeFlatHex(3);
		// Muro di fango sulla colonna q=0: per passare da ovest a est bisogna attraversarlo.
		for (int32 r = -3; r <= 3; ++r)
		{
			const FRTCellId Id(0, r, 0);
			if (Map->ContainsCell(Id)) { SetCost(Map, Id, 4); }
		}
		const FRTArenaCriterionResult R = URTArenaCriteriaLibrary::CheckCostBite(Map, A, B, /*MoveBudget=*/ 5);
		TestTrue(TEXT("fango non aggirabile e oltre budget -> morde"), R.bPassed);
	}

	// 5. Nessun percorso: non e' «il criterio fallisce», e' «non si puo' valutare».
	{
		URTHexMapAsset* Map = MakeFlatHex(3);
		for (int32 r = -3; r <= 3; ++r)
		{
			const FRTCellId Id(0, r, 0);
			if (Map->ContainsCell(Id)) { SetBlocksMove(Map, Id); }
		}
		const FRTArenaCriterionResult R = URTArenaCriteriaLibrary::CheckCostBite(Map, A, B, /*MoveBudget=*/ 5);
		TestTrue(TEXT("spawn scollegati -> non valutabile"), R.bNotEvaluable);
		TestFalse(TEXT("e non passa"), R.bPassed);
	}
	return true;
}

/**
 * Il criterio delle rotte pretende TRE condizioni insieme, e ogni test qui sotto ne rompe una sola.
 *
 * Due rotte gemelle non sono una scelta; una lunga il doppio e' sempre la stessa scelta; due rotte esposte
 * allo stesso modo sono la stessa rotta con un'altra forma.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTArenaTwoRoutesTest,
	"RefactorTactics.Arena.TwoRoutesNeedDisjointSimilarCostDifferentExposure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTArenaTwoRoutesTest::RunTest(const FString&)
{
	// 1. Corridoio largo una cella: non esiste una seconda rotta disgiunta.
	//    E' un verdetto sulla mappa, non un'impossibilita' di misurare.
	{
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
		for (int32 q = 0; q <= 4; ++q) { Map->AddOrUpdateCell(FRTHexCellData(FRTCellId(q, 0, 0))); }
		Map->SortCells();
		const FRTArenaCriterionResult R = URTArenaCriteriaLibrary::CheckTwoRoutes(
			Map, FRTCellId(0, 0, 0), FRTCellId(4, 0, 0));
		TestFalse(TEXT("corridoio singolo -> nessuna alternativa"), R.bPassed);
		TestFalse(TEXT("ed e' un verdetto, non un errore"), R.bNotEvaluable);
	}

	// 2. Campo completamente aperto: le due rotte esistono, sono disgiunte e nessuna e' coperta.
	//
	//    E' il caso che ha fatto cadere la PRIMA versione del criterio, che contava le celle esposte in numero
	//    ASSOLUTO: la seconda rotta deve deviare, quindi ha piu' celle, quindi ne ha piu' esposte — «esposizione
	//    diversa» era soddisfatta da qualunque mappa aperta. Contare le celle misura la lunghezza; la frazione
	//    misura la copertura. Qui entrambe le rotte sono esposte al 100% e il criterio deve rifiutarle.
	{
		URTHexMapAsset* Map = MakeFlatHex(4);
		const FRTArenaCriterionResult R = URTArenaCriteriaLibrary::CheckTwoRoutes(
			Map, FRTCellId(-3, 0, 0), FRTCellId(3, 0, 0));
		AddInfo(FString::Printf(TEXT("campo aperto: %s"), *R.Detail));
		TestFalse(TEXT("campo aperto -> nessuna delle due e' coperta, nessun trade-off"), R.bPassed);
	}

	// 3. Il caso buono: due vie disgiunte, costo confrontabile, e la piu' CARA e' piu' coperta.
	//    La direzione conta: se paghi di piu' e resti esposto uguale, non e' una scelta ma una rotta peggiore.
	{
		URTHexMapAsset* Map = MakeFlatHex(4);
		const FRTCellId A(-3, 0, 0);
		const FRTCellId B(3, 0, 0);

		// Sbarra la linea centrale: costringe a scegliere fra la via nord e quella sud.
		for (int32 q = -1; q <= 1; ++q) { SetBlocksMove(Map, FRTCellId(q, 0, 0)); }

		// Schermo davanti alla via SUD: nasconde le sue celle alla vista di B, senza impedirne il passo.
		// (bBlocksLineOfSight non blocca il movimento: la copertura non e' un ostacolo.)
		for (int32 q = -1; q <= 2; ++q) { SetBlocksLoS(Map, FRTCellId(q, 1, 0)); }

		const FRTArenaCriterionResult R = URTArenaCriteriaLibrary::CheckTwoRoutes(Map, A, B);
		AddInfo(FString::Printf(TEXT("due rotte: %s"), *R.Detail));
		TestTrue(TEXT("disgiunte, costo simile, la piu' cara e' piu' coperta -> passa"), R.bPassed);

		// La stessa mappa con uno scarto minimo irraggiungibile deve fallire: e' cio' che rende la soglia un
		// parametro e non una decorazione. Se il verdetto non cambiasse, il numero non sarebbe letto da nessuno.
		const FRTArenaCriterionResult Strict = URTArenaCriteriaLibrary::CheckTwoRoutes(
			Map, A, B, URTArenaCriteriaLibrary::DefaultMaxCostRatio, /*MinExposureGap=*/ 0.99f);
		TestFalse(TEXT("scarto minimo 99% -> la stessa mappa non basta piu'"), Strict.bPassed);
	}
	return true;
}

/**
 * Gli spawn si DERIVANO dalla mappa con la stessa funzione che usa l'allestimento della partita.
 *
 * Se il verificatore scegliesse spawn propri, misurerebbe una partita che non si gioca: l'arena potrebbe
 * soddisfare i criteri fra due celle qualsiasi e non fra quelle da cui le squadre partono davvero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTArenaDerivedSpawnsTest,
	"RefactorTactics.Arena.DerivedSpawnsMatchMatchSetup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTArenaDerivedSpawnsTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeFlatHex(4);

	const TArray<FRTCellId> Start = URTMatchSetupLibrary::PickStartCells(Map, /*NumPerTeam=*/ 2, /*Layer=*/ 0);
	TestTrue(TEXT("l'allestimento trova le celle di partenza"), Start.Num() == 4);

	const FRTArenaCriteriaReport Report = URTArenaCriteriaLibrary::EvaluateWithDerivedSpawns(Map);
	TestEqual(TEXT("SpawnA e' la prima cella del team 0"), Report.SpawnA, Start[0]);
	TestEqual(TEXT("SpawnB e' la prima cella del team 1"), Report.SpawnB, Start[2]);

	// Mappa vuota: niente celle di partenza -> tutti e tre non valutabili, e nessuno «passa».
	URTHexMapAsset* Empty = NewObject<URTHexMapAsset>();
	const FRTArenaCriteriaReport EmptyReport = URTArenaCriteriaLibrary::EvaluateWithDerivedSpawns(Empty);
	TestTrue(TEXT("mappa vuota -> copertura non valutabile"), EmptyReport.Coverage.bNotEvaluable);
	TestTrue(TEXT("mappa vuota -> costo non valutabile"), EmptyReport.CostBite.bNotEvaluable);
	TestTrue(TEXT("mappa vuota -> rotte non valutabili"), EmptyReport.TwoRoutes.bNotEvaluable);
	TestFalse(TEXT("e AllPassed e' falso"), EmptyReport.AllPassed());

	// Mappa nulla: stesso trattamento, nessun crash.
	const FRTArenaCriteriaReport NullReport = URTArenaCriteriaLibrary::EvaluateWithDerivedSpawns(nullptr);
	TestFalse(TEXT("mappa nulla -> non passa"), NullReport.AllPassed());
	return true;
}

/**
 * Il verdetto su `MakeTestArena` — l'arena GENERATA che le sedute U2…U6 gia' usano.
 *
 * Non e' un test di regressione su un valore atteso: e' la misura che dice se i criteri chiesti all'arena
 * d'autore di U1 siano gia' soddisfatti da quella di codice. Il risultato viene stampato per intero, perche'
 * il suo valore e' informativo — se `MakeTestArena` non li soddisfa, la differenza fra le due arene e'
 * esattamente cio' che U1 deve produrre in piu'.
 *
 * L'unica cosa che questo test PRETENDE e' che i criteri siano **valutabili** su quella mappa: se non lo
 * fossero, il verificatore non saprebbe misurare nemmeno l'arena che il progetto usa ogni giorno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTArenaGeneratedArenaVerdictTest,
	"RefactorTactics.Arena.GeneratedTestArenaIsMeasurable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTArenaGeneratedArenaVerdictTest::RunTest(const FString&)
{
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(GetTransientPackage());
	TestNotNull(TEXT("MakeTestArena produce un'arena"), Arena);
	if (!Arena) { return false; }

	const FRTArenaCriteriaReport Report = URTArenaCriteriaLibrary::EvaluateWithDerivedSpawns(Arena);
	AddInfo(FString::Printf(TEXT("GeneratedTestArena (%d celle):\n%s"),
		Arena->NumCells(), *URTArenaCriteriaLibrary::FormatReport(Report)));

	TestFalse(TEXT("copertura valutabile"), Report.Coverage.bNotEvaluable);
	TestFalse(TEXT("costo valutabile"), Report.CostBite.bNotEvaluable);
	TestFalse(TEXT("rotte valutabili"), Report.TwoRoutes.bNotEvaluable);
	return true;
}

/**
 * L'arena della v0.1 soddisfa **tutti e tre** i criteri del `done_when` di U1.
 *
 * E' il test che rende il layout una cosa verificata invece che disegnata a occhio: se qualcuno sposta un
 * muro o toglie il fango, il criterio che quel pezzo teneva in piedi cade qui, headless, in un secondo —
 * invece che in editor tre sedute dopo.
 *
 * Il verdetto viene stampato per intero: quando fallisce, i numeri dicono **di quanto**, e correggere smette
 * di essere tentativo ed errore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTArenaV01MeetsCriteriaTest,
	"RefactorTactics.Arena.ArenaV01MeetsAllThreeCriteria",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTArenaV01MeetsCriteriaTest::RunTest(const FString&)
{
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeArenaV01(GetTransientPackage());
	TestNotNull(TEXT("l'arena esiste"), Arena);
	if (!Arena) { return false; }

	const FRTArenaCriteriaReport Report = URTArenaCriteriaLibrary::EvaluateWithDerivedSpawns(Arena);
	AddInfo(FString::Printf(TEXT("ArenaV01 (%d celle):\n%s"),
		Arena->NumCells(), *URTArenaCriteriaLibrary::FormatReport(Report)));

	// Diagnostica: quali celle hanno un COSTO, cioe' quali producono un rilievo nella vista dell'editor.
	// Serve a rispondere a «ne vedo N» senza contarle a occhio: celle costose adiacenti si leggono come un
	// blocco solo, e la differenza fra «tre elementi» e «quattro celle» e' esattamente quel tipo di equivoco.
	{
		FString Costly;
		int32 CostlyCount = 0;
		for (const FRTHexCellData& C : Arena->Cells)
		{
			if (C.MoveCost > 1)
			{
				++CostlyCount;
				Costly += FString::Printf(TEXT("(%d,%d)=%d "), C.Id.X, C.Id.Y, C.MoveCost);
			}
		}
		AddInfo(FString::Printf(TEXT("celle con costo > 1: %d --> %s"), CostlyCount, *Costly));
	}

	// Diagnostica: DOVE passano le due rotte. Senza, correggere il layout sarebbe indovinare — e il fango
	// finisce dove nessuno cammina.
	{
		const FRTHexPathResult P1 = URTHexPathLibrary::FindPath(Arena, Report.SpawnA, Report.SpawnB);
		FString S1;
		for (const FRTCellId& C : P1.Path) { S1 += FString::Printf(TEXT("(%d,%d) "), C.X, C.Y); }
		TSet<FRTCellId> Blocked;
		for (int32 i = 1; i < P1.Path.Num() - 1; ++i) { Blocked.Add(P1.Path[i]); }
		const FRTHexPathResult P2 = URTHexPathLibrary::FindPathAvoiding(Arena, Report.SpawnA, Report.SpawnB, &Blocked);
		FString S2;
		for (const FRTCellId& C : P2.Path) { S2 += FString::Printf(TEXT("(%d,%d) "), C.X, C.Y); }
		AddInfo(FString::Printf(TEXT("rotta 1 (costo %d): %s"), P1.TotalCost, *S1));
		AddInfo(FString::Printf(TEXT("rotta 2 (costo %d): %s"), P2.TotalCost, *S2));
	}

	TestTrue(TEXT("copertura"), Report.Coverage.bPassed);
	TestTrue(TEXT("costo"), Report.CostBite.bPassed);
	TestTrue(TEXT("due rotte"), Report.TwoRoutes.bPassed);
	TestTrue(TEXT("tutto raggiungibile"), Report.Reachability.bPassed);

	// La piattaforma esiste, sta su un altro layer, ed e' collegata da UNA SOLA transizione: e' il passo 5 di
	// U1, che nessun criterio verificava. Con due archi diventerebbe una scorciatoia e le rotte cambierebbero.
	TestEqual(TEXT("una sola transizione (bidirezionale = due archi)"), Arena->Transitions.Num(), 2);
	TArray<int32> Layers = Arena->GetLayers();
	TestTrue(TEXT("esistono celle su almeno due layer"), Layers.Num() >= 2);
	return true;
}

/**
 * Togliere l'unica transizione rende la piattaforma IRRAGGIUNGIBILE, e il criterio deve accorgersene.
 *
 * E' la prova che `CheckReachability` guardi il grafo tattico e non i vicini planari: una piattaforma
 * sovrapposta e' vicina nello spazio e lontana nel grafo, e confondere le due cose direbbe «ci si arriva»
 * di una zona dove non ci si arriva.
 *
 * Verifica anche il numero: quante celle restano isolate, non solo che qualcosa non va.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTArenaUnreachablePlatformTest,
	"RefactorTactics.Arena.RemovingTheOnlyTransitionIsolatesThePlatform",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTArenaUnreachablePlatformTest::RunTest(const FString&)
{
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeArenaV01(GetTransientPackage());
	if (!Arena) { return false; }

	const int32 PlatformCells = Arena->CellsInLayer(1).Num();
	TestTrue(TEXT("la piattaforma ha celle"), PlatformCells > 0);

	// Con la transizione: tutto raggiungibile.
	const FRTArenaCriteriaReport Before = URTArenaCriteriaLibrary::EvaluateWithDerivedSpawns(Arena);
	TestTrue(TEXT("collegata -> raggiungibile"), Before.Reachability.bPassed);

	// Senza: le celle della piattaforma restano isolate, ed e' un verdetto — non un errore di misura.
	Arena->Transitions.Reset();
	const FRTArenaCriteriaReport After = URTArenaCriteriaLibrary::EvaluateWithDerivedSpawns(Arena);
	AddInfo(FString::Printf(TEXT("senza transizione: %s"), *After.Reachability.Detail));
	TestFalse(TEXT("scollegata -> NON raggiungibile"), After.Reachability.bPassed);
	TestFalse(TEXT("ed e' un verdetto, non un'impossibilita' di misurare"), After.Reachability.bNotEvaluable);
	TestFalse(TEXT("e AllPassed cade con lui"), After.AllPassed());

	// Il dettaglio riporta il numero giusto: correggere non deve essere una caccia.
	TestTrue(TEXT("il dettaglio cita quante celle sono isolate"),
		After.Reachability.Detail.Contains(FString::Printf(TEXT("%d celle"), PlatformCells)));
	return true;
}

/**
 * Il verdetto non dipende dall'ORDINE delle celle nell'asset.
 *
 * Il pathfinding e la LOS sono dichiarati deterministici e indipendenti dall'ordine di `TMap`/`TSet`; questo
 * lo verifica end-to-end sul verificatore, che di quelle primitive e' un consumatore e potrebbe introdurre
 * una dipendenza propria (per esempio iterando `Cells` per trovare la zona costosa).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTArenaOrderIndependenceTest,
	"RefactorTactics.Arena.VerdictIsIndependentOfCellOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTArenaOrderIndependenceTest::RunTest(const FString&)
{
	const FRTCellId A(-3, 0, 0);
	const FRTCellId B(3, 0, 0);

	auto BuildMap = [&](bool bReversed) -> URTHexMapAsset*
	{
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
		TArray<FRTCellId> Ids = URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 4);
		if (bReversed) { Algo::Reverse(Ids); }
		for (const FRTCellId& Id : Ids) { Map->AddOrUpdateCell(FRTHexCellData(Id)); }
		Map->SortCells();
		for (int32 q = -1; q <= 1; ++q) { SetBlocksMove(Map, FRTCellId(q, 0, 0)); }
		SetBlocksLoS(Map, FRTCellId(-1, 1, 0));
		SetBlocksLoS(Map, FRTCellId(1, 0, 0));
		return Map;
	};

	const FRTArenaCriteriaReport Forward = URTArenaCriteriaLibrary::Evaluate(BuildMap(false), A, B);
	const FRTArenaCriteriaReport Backward = URTArenaCriteriaLibrary::Evaluate(BuildMap(true), A, B);

	TestEqual(TEXT("copertura invariante"), Forward.Coverage.bPassed, Backward.Coverage.bPassed);
	TestEqual(TEXT("costo invariante"), Forward.CostBite.bPassed, Backward.CostBite.bPassed);
	TestEqual(TEXT("rotte invarianti"), Forward.TwoRoutes.bPassed, Backward.TwoRoutes.bPassed);
	TestEqual(TEXT("anche il dettaglio, non solo il verdetto"), Forward.TwoRoutes.Detail, Backward.TwoRoutes.Detail);
	return true;
}


/**
 * Il criterio e l'overlay contano le stesse celle.
 *
 * `FindUnreachableCells` e' stata estratta da `CheckReachability` proprio per questo: il criterio ne dice
 * QUANTE sono, l'overlay dell'editor mostra QUALI. Se fossero due visite scritte a parte, prima o poi
 * risponderebbero diversamente — e un numero nel log che non corrisponde a cio' che si vede in scena e' peggio
 * di nessuno dei due, perche' non si sa a quale credere.
 *
 * Il test lo verifica dove la differenza si vede: una piattaforma collegata, e la stessa senza il suo arco.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTArenaUnreachableSharedTest,
	"RefactorTactics.Arena.CriterionAndOverlayCountTheSameCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTArenaUnreachableSharedTest::RunTest(const FString&)
{
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeArenaV01(GetTransientPackage());
	if (!Arena) { return false; }

	const TArray<FRTCellId> Start = URTMatchSetupLibrary::PickStartCells(Arena, 2, 0);
	TestTrue(TEXT("gli spawn esistono"), Start.Num() > 0);
	if (Start.Num() == 0) { return false; }

	// Collegata: nessuna cella isolata, e il criterio passa. I due devono concordare anche sullo ZERO.
	{
		const TArray<FRTCellId> Isolated = URTArenaCriteriaLibrary::FindUnreachableCells(Arena, Start[0]);
		const FRTArenaCriterionResult R = URTArenaCriteriaLibrary::CheckReachability(Arena, Start[0]);
		TestEqual(TEXT("collegata: nessuna cella isolata"), Isolated.Num(), 0);
		TestTrue(TEXT("collegata: il criterio passa"), R.bPassed);
	}

	// Scollegata: l'elenco e il criterio devono raccontare la STESSA cosa.
	{
		const int32 PlatformCells = Arena->CellsInLayer(1).Num();
		Arena->Transitions.Reset();
		const TArray<FRTCellId> Isolated = URTArenaCriteriaLibrary::FindUnreachableCells(Arena, Start[0]);
		const FRTArenaCriterionResult R = URTArenaCriteriaLibrary::CheckReachability(Arena, Start[0]);

		TestEqual(TEXT("l'elenco contiene tutte le celle della piattaforma"), Isolated.Num(), PlatformCells);
		TestFalse(TEXT("e il criterio cade"), R.bPassed);
		// Il numero nel messaggio del criterio e' quello dell'elenco: e' cio' che rende log e scena
		// confrontabili invece di due opinioni.
		TestTrue(TEXT("il criterio cita lo stesso numero dell'elenco"),
			R.Detail.Contains(FString::Printf(TEXT("%d celle"), Isolated.Num())));

		// Le celle isolate sono TUTTE del layer della piattaforma: una visita sui vicini planari le avrebbe
		// dichiarate raggiungibili perche' stanno sopra celle raggiungibili.
		for (const FRTCellId& Cell : Isolated)
		{
			TestEqual(TEXT("l'isolamento e' del piano staccato"), Cell.Layer, 1);
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
