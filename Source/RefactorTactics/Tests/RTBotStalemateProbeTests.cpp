// Probe di #1088: dalle celle dello stallo, esiste un bersaglio legale?
//
// In gioco i bot si avvicinano, si fermano a distanza esagonale 3 e non attaccano piu': 12 round di sola
// `Fase Move`, zero combattimento, pareggio allo scadere. Succede su due arene con copertura e NON succede
// sull'esagono liscio, dove la partita finisce per eliminazione al round 9.
//
// `rt.Arena.Check` ha gia' escluso la spiegazione piu' comoda: la mappa d'autore soddisfa **tutti e tre** i
// criteri di U1, e la riga della copertura — «3 celle che bloccano la vista sul segmento, linea di tiro
// interrotta» — dice che la vista bloccata fra gli spawn e' **voluta**. Restano due candidati:
//
//   2. il PATHING non trova una cella da cui il tiro sia legale, e il bot resta fermo invece di aggirare;
//   3. l'UTILITY del bot non sceglie l'attacco, pur essendo disponibile.
//
// Questo file li separa, e lo fa **senza simulare una partita**: `MakeTestArena` genera l'arena da codice e
// `HasLineOfSight` e' pura, quindi la domanda «esiste una cella da cui si spara?» e' un'enumerazione.
//
// ⚠️ **E' un probe, non una regressione.** Le sue asserzioni pinnano cio' che la misura ha trovato, perche'
// un test che stampa e non asserisce smette di verificare senza dirlo — ma il suo scopo e' rispondere alla
// domanda del criterio 1 di #1088, e quando la causa sara' corretta questo file andra' riletto, non tenuto
// per inerzia.

#include "Misc/AutomationTest.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexVisionLibrary.h"
#include "Map/RTArenaCriteriaLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nome distinto da ogni altro file di test: la unity build condivide la translation unit.
	struct FRTProbeFiringReport
	{
		int32 CellsOnLayer0 = 0;
		int32 InRangeOfTarget = 0;       // entro la gittata dell'attacco base
		int32 WithLineOfSight = 0;       // ... e con la linea di tiro libera
		int32 NearestFiringDistance = TNumericLimits<int32>::Max(); // dallo spawn del tiratore
	};

	/**
	 * Da quante celle della mappa il bersaglio in `Target` sarebbe colpibile, e quanto dista la piu' vicina
	 * a `ShooterSpawn`.
	 *
	 * ⚠️ Interroga `HasLineOfSight`, la stessa funzione del resolver: se rifacessi il conto qui dentro
	 * misurerei la mia idea di linea di tiro, non quella del gioco.
	 */
	FRTProbeFiringReport ProbeFiringPositions(const URTHexMapAsset* Map, const FRTCellId& ShooterSpawn,
		const FRTCellId& Target, int32 RangeCells)
	{
		FRTProbeFiringReport Report;
		if (!Map)
		{
			return Report;
		}

		for (const FRTCellId& Cell : Map->CellsInLayer(0))
		{
			++Report.CellsOnLayer0;

			if (URTHexLibrary::HexDistance(Cell, Target) > RangeCells)
			{
				continue;
			}
			++Report.InRangeOfTarget;

			if (!URTHexVisionLibrary::HasLineOfSight(Map, Cell, Target))
			{
				continue;
			}
			++Report.WithLineOfSight;

			Report.NearestFiringDistance = FMath::Min(Report.NearestFiringDistance,
				URTHexLibrary::HexDistance(ShooterSpawn, Cell));
		}

		return Report;
	}
}

/**
 * La domanda del criterio 1 di #1088, posta alla geometria invece che a una partita.
 *
 * ⚠️ **La gittata non e' un parametro libero: e' 4.** `Hero.Gadget.ArcPulse` e `Hero.Wraith.PulseShot`
 * dichiarano `Range 4` nel catalogo, ed e' il numero che rende lo stallo strano — le unita' si fermano a
 * distanza **3**, cioe' dentro la gittata, e non sparano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotStalemateFiringPositionsTest,
	"RefactorTactics.Bot.StalemateProbeFiringPositionsExist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotStalemateFiringPositionsTest::RunTest(const FString&)
{
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(GetTransientPackage());
	if (!TestNotNull(TEXT("arena di prova generata"), Arena)) { return false; }

	// Gli spawn si DERIVANO dalla mappa, come fa `rt.Arena.Check`: inventarli qui misurerebbe una partita
	// che nessuno gioca.
	const FRTArenaCriteriaReport Criteria = URTArenaCriteriaLibrary::EvaluateWithDerivedSpawns(Arena);
	const FRTCellId SpawnA = Criteria.SpawnA;
	const FRTCellId SpawnB = Criteria.SpawnB;

	AddInfo(FString::Printf(TEXT("spawn derivati: A=(q=%d,r=%d,L=%d) B=(q=%d,r=%d,L=%d), distanza %d"),
		SpawnA.X, SpawnA.Y, SpawnA.Layer, SpawnB.X, SpawnB.Y, SpawnB.Layer,
		URTHexLibrary::HexDistance(SpawnA, SpawnB)));

	// --- 1. Il fatto di partenza: fra gli spawn non ci si vede. E' la copertura, ed e' voluta.
	const bool bSpawnsSeeEachOther = URTHexVisionLibrary::HasLineOfSight(Arena, SpawnA, SpawnB);
	AddInfo(FString::Printf(TEXT("linea di tiro spawn A -> spawn B: %s"),
		bSpawnsSeeEachOther ? TEXT("LIBERA") : TEXT("interrotta")));

	// --- 2. La domanda decisiva: esiste una cella da cui B sarebbe colpibile?
	const int32 BasicAttackRange = 4; // `Hero.Gadget.ArcPulse`, `Hero.Wraith.PulseShot`
	const FRTProbeFiringReport Firing = ProbeFiringPositions(Arena, SpawnA, SpawnB, BasicAttackRange);

	AddInfo(FString::Printf(
		TEXT("celle sul layer 0: %d | entro gittata %d da B: %d | di queste, con tiro libero: %d"),
		Firing.CellsOnLayer0, BasicAttackRange, Firing.InRangeOfTarget, Firing.WithLineOfSight));

	if (Firing.WithLineOfSight > 0)
	{
		AddInfo(FString::Printf(TEXT("la piu' vicina allo spawn A dista %d celle"),
			Firing.NearestFiringDistance));
	}

	// --- Il verdetto che separa i due candidati, e va letto cosi':
	//
	//   celle di tiro > 0  -> il bersaglio E' colpibile da qualche parte: la geometria non lo impedisce,
	//                         e il difetto sta in chi non ci va o non spara (candidati 2 e 3);
	//   celle di tiro == 0 -> nessuna cella dell'arena vede il bersaglio entro gittata: allora lo stallo
	//                         e' geometrico, e la domanda si sposta sul layout.
	TestTrue(TEXT("l'arena ha celle sul layer 0"), Firing.CellsOnLayer0 > 0);

	// ⚠️ Questa riga e' il RISULTATO della misura, non un'assunzione: se un giorno cade, la geometria e'
	// cambiata e il verdetto di #1088 va rifatto — che e' esattamente quando si vuole essere avvisati.
	TestTrue(TEXT("esiste almeno una cella da cui il bersaglio e' colpibile entro gittata 4"),
		Firing.WithLineOfSight > 0);

	return true;
}

/**
 * Il controfattuale, sulla stessa domanda: l'arena demo — esagono liscio, nessuna copertura — e' quella
 * dove i bot combattono davvero.
 *
 * ⚠️ Esiste per non far dire al test precedente piu' di quanto misura. «Ci sono celle di tiro» su un'arena
 * con copertura significa qualcosa solo se si sa quante ce ne sono dove il problema non c'e': senza
 * termine di paragone, un numero e' un aneddoto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotStalemateDemoArenaContrastTest,
	"RefactorTactics.Bot.StalemateProbeDemoArenaHasMoreFiringCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotStalemateDemoArenaContrastTest::RunTest(const FString&)
{
	URTHexMapAsset* Demo = URTMatchSetupLibrary::MakeDemoArena(GetTransientPackage(), /*Radius*/ 4);
	if (!TestNotNull(TEXT("arena demo generata"), Demo)) { return false; }

	const FRTArenaCriteriaReport Criteria = URTArenaCriteriaLibrary::EvaluateWithDerivedSpawns(Demo);
	const FRTProbeFiringReport Firing =
		ProbeFiringPositions(Demo, Criteria.SpawnA, Criteria.SpawnB, /*Range*/ 4);

	AddInfo(FString::Printf(
		TEXT("demo: celle %d | entro gittata: %d | con tiro libero: %d | piu' vicina allo spawn: %d"),
		Firing.CellsOnLayer0, Firing.InRangeOfTarget, Firing.WithLineOfSight,
		Firing.WithLineOfSight > 0 ? Firing.NearestFiringDistance : -1));

	// Senza copertura, ogni cella entro gittata e' anche una cella di tiro: e' cio' che rende l'esagono
	// liscio il caso in cui il bot non puo' sbagliare.
	TestEqual(TEXT("senza copertura, tiro libero da OGNI cella entro gittata"),
		Firing.WithLineOfSight, Firing.InRangeOfTarget);

	// ⚠️ **La riga che rende il confronto non vacuo.** Le due arene danno lo stesso conteggio di celle di
	// tiro (25 su 25), quindi da solo quel numero non dimostra che il probe SAPPIA vedere una copertura:
	// sarebbe soddisfatto anche da una `HasLineOfSight` che risponde sempre vero. Il discriminante e' la
	// linea LUNGA fra gli spawn, dove la copertura dell'arena di prova sta davvero.
	URTHexMapAsset* TestArena = URTMatchSetupLibrary::MakeTestArena(GetTransientPackage());
	if (!TestNotNull(TEXT("arena di prova generata"), TestArena)) { return false; }
	const FRTArenaCriteriaReport TestCriteria =
		URTArenaCriteriaLibrary::EvaluateWithDerivedSpawns(TestArena);

	const bool bDemoSpawnsSee =
		URTHexVisionLibrary::HasLineOfSight(Demo, Criteria.SpawnA, Criteria.SpawnB);
	const bool bTestSpawnsSee =
		URTHexVisionLibrary::HasLineOfSight(TestArena, TestCriteria.SpawnA, TestCriteria.SpawnB);

	AddInfo(FString::Printf(TEXT("linea spawn-spawn: demo %s | arena di prova %s"),
		bDemoSpawnsSee ? TEXT("LIBERA") : TEXT("interrotta"),
		bTestSpawnsSee ? TEXT("LIBERA") : TEXT("interrotta")));

	TestTrue(TEXT("sull'esagono liscio gli spawn si vedono"), bDemoSpawnsSee);
	TestFalse(TEXT("sull'arena di prova la copertura interrompe la linea fra gli spawn"), bTestSpawnsSee);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
