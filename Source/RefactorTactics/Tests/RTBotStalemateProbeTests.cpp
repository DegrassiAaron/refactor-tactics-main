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
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Bot/RTHexBotLibrary.h"

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

/**
 * Il secondo probe: **il bot genera la candidata «attacca», e con quale punteggio la perde?**
 *
 * Il primo probe ha stabilito che la geometria non impedisce il tiro. Restava il candidato 3 — l'utility
 * non sceglie l'attacco — che pero' e' una localizzazione, non una causa. Qui si guarda dentro.
 *
 * ⚠️ **Due fatti letti nel codice, che restringono prima ancora di misurare:**
 *
 *  · `DeriveKiteStandoff` da' standoff `0` sotto gittata 5. Sul roster v0.1 **solo Phase** (`PressureJet`,
 *    gittata 5) tiene le distanze; Gadget e Wraith (4) e Riktor (3) **chiudono**. Lo stallo a distanza 3
 *    non e' quindi il kiting che fa il suo mestiere — non per tre unita' su quattro.
 *  · `ChooseBestPlan` dichiara un **tie-break assoluto**: *«a parita' di punteggio vince la MOSSA MINIMA
 *    da Origin (restare vince)»*. Se attaccare non migliorasse **strettamente** il punteggio, restare
 *    fermi sarebbe il comportamento corretto del confronto — e il difetto starebbe nei punteggi.
 *
 * ⚠️ **Misura una situazione ISOLATA**, e questo e' il suo limite dichiarato: una coppia di unita', nessun
 * alleato, nessuna occupazione di terze celle. Se qui il bot attacca e in partita no, la causa sta in cio'
 * che questo scenario NON riproduce — ed e' un risultato utile quanto il contrario.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotStalemateCandidateScoresTest,
	"RefactorTactics.Bot.StalemateProbeAttackCandidateIsGeneratedAndScored",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotStalemateCandidateScoresTest::RunTest(const FString&)
{
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(GetTransientPackage());
	if (!TestNotNull(TEXT("arena di prova generata"), Arena)) { return false; }

	// La posizione di stallo misurata in gioco: **distanza 3 con linea di tiro libera**.
	//
	// ⚠️ La coppia si DERIVA dalla mappa, non si sceglie a mano. Il primo tentativo fissava due celle a
	// occhio e cadeva sulla propria precondizione: fra `(-1,0)` e `(2,0)` passa la barriera centrale, quindi
	// il tiro non era legale e il probe non misurava nulla. Cercarla toglie l'indovinello e rende il test
	// robusto alla geometria — se un giorno nessuna coppia simile esistesse, sarebbe **quello** il risultato.
	FRTCellId SelfCell, EnemyCell;
	bool bFound = false;
	const TArray<FRTCellId> Layer0 = Arena->CellsInLayer(0);
	for (const FRTCellId& A : Layer0)
	{
		for (const FRTCellId& B : Layer0)
		{
			if (URTHexLibrary::HexDistance(A, B) == 3 && URTHexVisionLibrary::HasLineOfSight(Arena, A, B))
			{
				SelfCell = A; EnemyCell = B; bFound = true;
				break;
			}
		}
		if (bFound) { break; }
	}

	if (!TestTrue(TEXT("esiste una coppia a distanza 3 con tiro libero"), bFound)) { return false; }
	AddInfo(FString::Printf(TEXT("coppia derivata: (q=%d,r=%d) -> (q=%d,r=%d), distanza 3, tiro libero"),
		SelfCell.X, SelfCell.Y, EnemyCell.X, EnemyCell.Y));

	TArray<FRTHexSimUnit> ProbeUnits;
	ProbeUnits.Add(FRTHexSimUnit(1, SelfCell, /*budget*/ 5));
	ProbeUnits.Add(FRTHexSimUnit(2, EnemyCell, /*budget*/ 5));
	const FRTHexSnapshot Snapshot = URTHexSimLibrary::MakeSnapshot(Arena, ProbeUnits);

	// I pesi restano quelli di default, che sono anche quelli che il gioco logga a ogni partita.
	FRTHexBotContext Ctx;
	Ctx.Origin = SelfCell;
	Ctx.Enemies.Add(EnemyCell);
	Ctx.EnemyRanges.Add(4);
	Ctx.EnemyHealth.Add(100);
	Ctx.AttackRange = 4;   // `Hero.Gadget.ArcPulse`
	Ctx.AttackDamage = 21; // `Hero.Wraith.PulseShot`
	Ctx.KiteStandoff = URTHexBotLibrary::DeriveKiteStandoff(Ctx.AttackRange);

	AddInfo(FString::Printf(TEXT("standoff derivato da gittata %d: %d (0 = chiude la distanza)"),
		Ctx.AttackRange, Ctx.KiteStandoff));

	const TArray<FRTHexBotPlan> Candidates = URTHexBotLibrary::BuildCandidates(Snapshot, 1, Ctx);

	int32 WithAttack = 0;
	int32 BestAttackScore = TNumericLimits<int32>::Min();
	int32 BestNoAttackScore = TNumericLimits<int32>::Min();
	int32 StayNoAttack = TNumericLimits<int32>::Min();
	int32 StayWithAttack = TNumericLimits<int32>::Min();

	for (const FRTHexBotPlan& Plan : Candidates)
	{
		const int32 Score = URTHexBotLibrary::ScorePlan(Arena, Plan, Ctx);
		const bool bStays = (Plan.DestCell == SelfCell);

		if (Plan.bHasAttack)
		{
			++WithAttack;
			BestAttackScore = FMath::Max(BestAttackScore, Score);
			if (bStays) { StayWithAttack = FMath::Max(StayWithAttack, Score); }
		}
		else
		{
			BestNoAttackScore = FMath::Max(BestNoAttackScore, Score);
			if (bStays) { StayNoAttack = FMath::Max(StayNoAttack, Score); }
		}
	}

	AddInfo(FString::Printf(TEXT("candidate: %d totali, %d con attacco"), Candidates.Num(), WithAttack));
	AddInfo(FString::Printf(TEXT("punteggi: miglior attacco %d, miglior non-attacco %d"),
		BestAttackScore, BestNoAttackScore));
	AddInfo(FString::Printf(TEXT("restando fermo: con attacco %d, senza attacco %d"),
		StayWithAttack, StayNoAttack));

	const FRTHexBotPlan Chosen = URTHexBotLibrary::ChooseBestPlan(Arena, Candidates, Ctx);
	AddInfo(FString::Printf(TEXT("scelto: dest (q=%d,r=%d) %s, punteggio %d"),
		Chosen.DestCell.X, Chosen.DestCell.Y,
		Chosen.bHasAttack ? TEXT("CON attacco") : TEXT("senza attacco"),
		URTHexBotLibrary::ScorePlan(Arena, Chosen, Ctx)));

	// --- Primo anello: la candidata esiste? La documentazione di `BuildCandidates` promette «una per
	// ciascun nemico entro gittata e in linea di vista DA QUELLA CELLA», e le due condizioni sono
	// verificate sopra.
	TestTrue(TEXT("con bersaglio in gittata e visibile, esiste almeno una candidata con attacco"),
		WithAttack > 0);

	// --- Secondo anello: attaccare da fermo batte STRETTAMENTE il non attaccare da fermo? Il tie-break di
	// `ChooseBestPlan` fa vincere «restare» a parita', quindi il pareggio non basterebbe.
	if (WithAttack > 0)
	{
		TestTrue(TEXT("attaccare da fermo batte strettamente il non attaccare da fermo"),
			StayWithAttack > StayNoAttack);
	}

	// --- Terzo anello, ed e' quello che chiude il candidato 3: la scelta finale porta l'attacco.
	TestTrue(TEXT("il piano scelto ha un attacco"), Chosen.bHasAttack);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
