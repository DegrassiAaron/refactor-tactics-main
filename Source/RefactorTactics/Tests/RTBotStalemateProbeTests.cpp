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
#include "Pathfinding/RTHexPathLibrary.h" // GraphNeighbors: gli archi del gioco, non una loro copia
#include "Perception/RTTeamKnowledge.h"
#include "Perception/RTPerceptionLibrary.h"

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

/**
 * L'anello che chiude la catena: **senza nemici percepiti il bot resta fermo.**
 *
 * I due probe precedenti hanno escluso geometria, pathing e utility — ma tutti e tre costruivano
 * `Ctx.Enemies` **a mano**, cioe' assumendo conoscenza perfetta. Il gioco no.
 *
 * `ARTTurnManager::PlanBots` passa ogni avversario per `URTTeamKnowledgeLibrary::ClassifyTarget`:
 *
 *   · `Allowed`  -> la squadra lo vede: cella e condizione attuali;
 *   · `CellOnly` -> contatto incerto: vale la cella dell'ULTIMO contatto, e **senza ricordo si fa
 *                   `continue`** — *«incerto senza ricordo: non e' ne' bersaglio ne' minaccia
 *                   contabilizzabile»*;
 *   · `Rejected` -> *«Ignoto alla squadra: non e' un bersaglio»*.
 *
 * Un nemico non percepito **non entra in `Ctx.Enemies`**. E un `Ctx` senza nemici toglie in un colpo solo
 * il bersaglio (nessuna candidata d'attacco) e l'incentivo di avvicinamento (`WApproach` misura
 * l'avvicinamento *a un nemico*). Restano candidate tutte equivalenti, e il tie-break di `ChooseBestPlan`
 * — *«a parita' vince la mossa minima da Origin, restare vince»* — le risolve **restando**.
 *
 * ⚠️ Questo test misura l'ULTIMO anello, non l'intera catena: che in partita la percezione si perda
 * davvero sull'arena con copertura resta un'inferenza dal codice e dalle tracce, non una misura. E' il
 * limite dichiarato, e il passo successivo e' una partita headless che stampi `Ctx.Enemies.Num()`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotStalemateNoPerceivedEnemyTest,
	"RefactorTactics.Bot.StalemateProbeNoPerceivedEnemyMeansStandStill",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotStalemateNoPerceivedEnemyTest::RunTest(const FString&)
{
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(GetTransientPackage());
	if (!TestNotNull(TEXT("arena di prova generata"), Arena)) { return false; }

	const FRTCellId SelfCell(-4, 0, 0);

	TArray<FRTHexSimUnit> ProbeUnits;
	ProbeUnits.Add(FRTHexSimUnit(1, SelfCell, /*budget*/ 5));
	const FRTHexSnapshot Snapshot = URTHexSimLibrary::MakeSnapshot(Arena, ProbeUnits);

	// Il contesto che il filtro di percezione produce quando la squadra non vede nessuno e non ricorda
	// nessuno: nemici **vuoti**. Tutto il resto identico al probe precedente.
	FRTHexBotContext Ctx;
	Ctx.Origin = SelfCell;
	Ctx.AttackRange = 4;
	Ctx.AttackDamage = 21;
	Ctx.KiteStandoff = URTHexBotLibrary::DeriveKiteStandoff(Ctx.AttackRange);

	const TArray<FRTHexBotPlan> Candidates = URTHexBotLibrary::BuildCandidates(Snapshot, 1, Ctx);

	int32 WithAttack = 0;
	for (const FRTHexBotPlan& Plan : Candidates)
	{
		if (Plan.bHasAttack) { ++WithAttack; }
	}

	const FRTHexBotPlan Chosen = URTHexBotLibrary::ChooseBestPlan(Arena, Candidates, Ctx);

	AddInfo(FString::Printf(TEXT("senza nemici percepiti: %d candidate, %d con attacco"),
		Candidates.Num(), WithAttack));
	AddInfo(FString::Printf(TEXT("scelto: dest (q=%d,r=%d) — origine (q=%d,r=%d)"),
		Chosen.DestCell.X, Chosen.DestCell.Y, SelfCell.X, SelfCell.Y));

	// --- Nessun bersaglio: nessuna candidata d'attacco. Ovvio, e va pinnato perche' e' la premessa
	// dell'asserzione successiva.
	TestEqual(TEXT("senza nemici non nasce nessuna candidata d'attacco"), WithAttack, 0);

	// --- **Il fatto che spiega lo stallo**: il bot non si muove. Non e' un difetto della scelta — e' il
	// tie-break che fa il suo mestiere su candidate tutte equivalenti. Cio' che manca al bot e' un
	// comportamento per «non vedo nessuno»: cercare, pattugliare, tenere una formazione.
	TestEqual(TEXT("senza nemici percepiti il piano scelto RESTA all'origine"), Chosen.DestCell, SelfCell);
	TestFalse(TEXT("e non porta attacco"), Chosen.bHasAttack);

	return true;
}

/**
 * **La partita headless**: il ciclo percezione -> decisione -> movimento, con le funzioni del gioco.
 *
 * I tre probe precedenti hanno costruito `Ctx.Enemies` a mano. Questo no: passa da
 * `URTTeamKnowledgeLibrary::Observe` e `ClassifyTarget`, cioe' dalla **stessa percezione** che
 * `ARTTurnManager::PlanBots` consulta, e riproduce il filtro con la stessa struttura a tre casi.
 *
 * ⚠️ **Misura l'anello che restava LETTO**: quante unita' avversarie entrano in `Ctx.Enemies`, turno per
 * turno. Se scende a zero e le posizioni si congelano, la catena di #1088 e' misurata da capo a fondo.
 *
 * ⚠️ **Cosa NON riproduce, dichiarato**: il resolver, le reazioni, gli scatti, il consumo di energia e il
 * facing che ruota. E' il ciclo di **decisione**, che e' dove lo stallo vive — dodici round di sola
 * `Fase Move` dicono che il resto non stava succedendo comunque.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotStalemateHeadlessMatchTest,
	"RefactorTactics.Bot.StalemateProbeHeadlessMatchLosesContact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
namespace
{
	struct FRTHeadlessRunReport
	{
		int32 FirstTurnWithoutContact = INDEX_NONE;
		int32 FrozenSince = INDEX_NONE;
		int32 PeakPerceived = 0;
		int32 TotalAttackPlans = 0;
	};

	// Nel namespace anonimo, e non piu' dentro la funzione, perche' da qui la usano DUE cicli: quello che
	// applica `Plan.DestCell` direttamente e quello che lo fa passare dalla risoluzione simultanea.
	struct FRTProbeUnit { int32 Id; int32 Team; FRTCellId Cell; };

	/**
	 * Il 2v2 del formato spedito, allo stesso allestimento per tutti i probe di questo file.
	 *
	 * ⚠️ **NON agli spawn derivati**, e la prima stesura sbagliava proprio qui: quelli distano **8** mentre
	 * `FRTPerceiver::VisionRange` vale **5** di default, quindi nessuno vedeva nessuno **su nessuna arena** —
	 * e il congelamento che ne usciva era un artefatto dell'allestimento, non il difetto. L'ha scoperto il
	 * controllo sull'esagono liscio, che dava contatto zero dove il contatto e' impossibile da bloccare.
	 *
	 * Le due squadre partono quindi a distanza **4**, dentro il raggio visivo e a cavallo del centro: e' la
	 * configurazione in cui le unita' si sono davvero fermate in partita (distanza 3), e l'unica differenza
	 * fra le due arene diventa la **copertura**.
	 */
	TArray<FRTProbeUnit> MakeProbeRoster()
	{
		TArray<FRTProbeUnit> Units;
		Units.Add({ 1, 0, FRTCellId(-2, 0, 0) });
		Units.Add({ 2, 0, FRTCellId(-2, 1, 0) });
		Units.Add({ 3, 1, FRTCellId(2, 0, 0) });
		Units.Add({ 4, 1, FRTCellId(2, -1, 0) });
		return Units;
	}

	/**
	 * Un turno di percezione e decisione, con le funzioni del gioco. Restituisce la destinazione pianificata
	 * per ogni unita' (parallela a `Units`) e lo snapshot su cui e' stata decisa.
	 *
	 * ⚠️ Estratta perche' due cicli la condividono. Duplicarla significherebbe due percorsi che divergono
	 * alla prima modifica — lo stesso difetto che `RTTurnManager` documenta per il danno da attraversamento.
	 */
	void PlanProbeTurn(URTHexMapAsset* Arena, const TArray<FRTProbeUnit>& Units,
		TArray<FRTTeamKnowledge>& Knowledge, int32 Turn,
		FRTHexSnapshot& OutSnapshot, TArray<FRTCellId>& OutPlanned, int32& OutPerceived, int32& OutAttackPlans,
		bool bPlanAsTeam = false)
	{
		OutPlanned.Reset();
		OutPerceived = 0;

		// --- 1. Percezione, con la funzione del gioco: ogni squadra osserva con i propri vivi.
		for (int32 Team = 0; Team < 2; ++Team)
		{
			TArray<FRTPerceiver> Observers;
			TArray<FRTLastKnownContact> EnemiesNow;
			for (const FRTProbeUnit& U : Units)
			{
				if (U.Team == Team)
				{
					FRTPerceiver P; P.Cell = U.Cell;
					Observers.Add(P);
				}
				else
				{
					FRTLastKnownContact C; C.StableUnitId = U.Id; C.Cell = U.Cell;
					EnemiesNow.Add(C);
				}
			}
			Knowledge[Team] = URTTeamKnowledgeLibrary::Observe(Arena, Team, Turn, Observers, EnemiesNow,
				Knowledge[Team]);
		}

		// --- 2. Decisione, unita' per unita', col filtro di percezione di `PlanBots`.
		TArray<FRTHexSimUnit> SimUnits;
		for (const FRTProbeUnit& U : Units) { SimUnits.Add(FRTHexSimUnit(U.Id, U.Cell, /*budget*/ 5)); }
		OutSnapshot = URTHexSimLibrary::MakeSnapshot(Arena, SimUnits);

		TArray<FRTHexBotContext> Contexts;
		for (const FRTProbeUnit& Self : Units)
		{
			FRTHexBotContext Ctx;
			Ctx.Origin = Self.Cell;
			Ctx.AttackRange = 4;
			Ctx.AttackDamage = 21;
			Ctx.KiteStandoff = URTHexBotLibrary::DeriveKiteStandoff(Ctx.AttackRange);

			for (const FRTProbeUnit& Other : Units)
			{
				if (Other.Team == Self.Team) { continue; }

				// La stessa struttura a tre casi di `PlanBots`: chi non e' ne' visto ne' ricordato NON entra.
				FRTCellId KnownCell = Other.Cell;
				const ERTTargetKnowledge Class = URTTeamKnowledgeLibrary::ClassifyTarget(
					Knowledge[Self.Team], Other.Id, Other.Team, Other.Cell);
				if (Class == ERTTargetKnowledge::Rejected) { continue; }
				if (Class == ERTTargetKnowledge::CellOnly
					&& !URTTeamKnowledgeLibrary::LastKnownCell(Knowledge[Self.Team], Other.Id, KnownCell))
				{
					continue;
				}
				Ctx.Enemies.Add(KnownCell);
				Ctx.EnemyRanges.Add(4);
				Ctx.EnemyHealth.Add(100);
			}
			OutPerceived += Ctx.Enemies.Num();
			Contexts.Add(Ctx);
		}

		if (!bPlanAsTeam)
		{
			for (int32 I = 0; I < Units.Num(); ++I)
			{
				const FRTHexBotPlan Plan = URTHexBotLibrary::PlanUnit(OutSnapshot, Units[I].Id, Contexts[I]);
				if (Plan.bHasAttack) { ++OutAttackPlans; }
				OutPlanned.Add(Plan.DestCell);
			}
			return;
		}

		// ⚠️ **Questo ramo rispecchia `ARTTurnManager::PlanBots`, e deve continuare a farlo**: uno snapshot
		// di pianificazione PER SQUADRA, `PlanUnit` su quello, e la rotta scelta prenotata subito dopo. Se
		// qui si usasse una funzione che il gioco non chiama, il probe misurerebbe un gemello della
		// correzione invece della correzione — ed e' esattamente l'errore in cui i primi cinque probe di
		// #1088 sono caduti, modellando la decisione invece di attraversare la risoluzione.
		//
		// ⛔ Per squadra, ed e' fairness (CP 13.5): con uno snapshot condiviso un bot schiverebbe la cella
		// di un AVVERSARIO, cioe' un intento che nessun giocatore puo' vedere.
		OutPlanned.SetNum(Units.Num());
		TMap<int32, FRTHexSnapshot> TeamSnapshots;
		for (int32 I = 0; I < Units.Num(); ++I)
		{
			FRTHexSnapshot* TeamSnapshot = TeamSnapshots.Find(Units[I].Team);
			if (!TeamSnapshot) { TeamSnapshot = &TeamSnapshots.Add(Units[I].Team, OutSnapshot); }

			const FRTHexBotPlan Plan = URTHexBotLibrary::PlanUnit(*TeamSnapshot, Units[I].Id, Contexts[I]);
			if (Plan.bHasAttack) { ++OutAttackPlans; }
			OutPlanned[I] = Plan.DestCell;
			URTHexBotLibrary::ReservePlannedRoute(*TeamSnapshot, Units[I].Id, Plan.DestCell);
		}
	}
}

/** Il ciclo percezione -> decisione -> movimento su una mappa qualsiasi. Vedi il test qui sotto. */
static FRTHeadlessRunReport RTRunHeadlessDecisionLoop(URTHexMapAsset* Arena, FAutomationTestBase& T,
	const TCHAR* Label)
{
	FRTHeadlessRunReport Out;

	TArray<FRTProbeUnit> Units = MakeProbeRoster();

	TArray<FRTTeamKnowledge> Knowledge;
	Knowledge.SetNum(2);

	TArray<FRTCellId> PreviousCells;

	for (int32 Turn = 1; Turn <= 12; ++Turn)
	{
		int32 TotalPerceived = 0;
		FRTHexSnapshot Snapshot;
		TArray<FRTCellId> NextCells;
		PlanProbeTurn(Arena, Units, Knowledge, Turn, Snapshot, NextCells, TotalPerceived, Out.TotalAttackPlans);

		// --- 3. Le posizioni si muovono ancora?
		const bool bFrozen = (PreviousCells.Num() == NextCells.Num()) && (PreviousCells == NextCells);
		Out.PeakPerceived = FMath::Max(Out.PeakPerceived, TotalPerceived);
		T.AddInfo(FString::Printf(TEXT("[%s] turno %2d: nemici percepiti = %d | piani con attacco finora = %d%s"),
			Label, Turn, TotalPerceived, Out.TotalAttackPlans,
			bFrozen ? TEXT("  [posizioni congelate]") : TEXT("")));

		if (TotalPerceived == 0 && Out.FirstTurnWithoutContact == INDEX_NONE) { Out.FirstTurnWithoutContact = Turn; }
		if (bFrozen && Out.FrozenSince == INDEX_NONE) { Out.FrozenSince = Turn; }
		if (!bFrozen) { Out.FrozenSince = INDEX_NONE; }

		PreviousCells = NextCells;
		for (int32 i = 0; i < Units.Num(); ++i) { Units[i].Cell = NextCells[i]; }
	}

	T.AddInfo(FString::Printf(TEXT("[%s] primo turno senza contatto: %d | congelate da: %d | picco contatti: %d"),
		Label, Out.FirstTurnWithoutContact, Out.FrozenSince, Out.PeakPerceived));
	return Out;
}

bool FRTBotStalemateHeadlessMatchTest::RunTest(const FString&)
{
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(GetTransientPackage());
	if (!TestNotNull(TEXT("arena di prova generata"), Arena)) { return false; }
	URTHexMapAsset* Demo = URTMatchSetupLibrary::MakeDemoArena(GetTransientPackage(), 4);
	if (!TestNotNull(TEXT("arena demo generata"), Demo)) { return false; }

	const FRTHeadlessRunReport Covered = RTRunHeadlessDecisionLoop(Arena, *this, TEXT("copertura"));
	const FRTHeadlessRunReport Open = RTRunHeadlessDecisionLoop(Demo, *this, TEXT("esagono liscio"));

	const int32 FirstTurnWithoutContact = Covered.FirstTurnWithoutContact;
	const int32 FrozenSince = Covered.FrozenSince;

	// ⚠️ Nessuna delle due righe qui sotto e' un'assunzione: sono il RISULTATO, e messe come asserzioni
	// perche' un probe che stampa e non asserisce smette di misurare senza dirlo. Se un giorno cadono, il
	// comportamento del bot e' cambiato — ed e' esattamente quando #1088 va riletta.
	TestTrue(TEXT("le posizioni si congelano entro i 12 turni"), FrozenSince != INDEX_NONE);

	// ⚠️ **Il controllo che toglie la vacuita'.** Sull'arena con copertura il contatto non c'e' MAI, quindi
	// «si congelano dopo averlo perso» e' soddisfatto anche da «non l'hanno mai avuto»: la riga sopra, da
	// sola, non distingue le due cose. L'esagono liscio e' lo stesso ciclo su una mappa dove la vista non
	// e' mai interrotta — se anche li' il contatto fosse zero, a essere rotto sarebbe il mio modello, non
	// il bot.
	TestTrue(TEXT("con copertura il contatto C'E'"), Covered.PeakPerceived > 0);
	TestTrue(TEXT("sull'esagono liscio anche"), Open.PeakPerceived > 0);

	// 🔴 **Il risultato che falsifica l'ipotesi della percezione**, e va pinnato perche' e' cio' che si e'
	// misurato: le unita' si congelano **pur vedendo** gli avversari, su entrambe le arene. Se un giorno
	// una delle due righe cade, il comportamento e' cambiato e #1088 va riletta.
	TestTrue(TEXT("con copertura si congelano NONOSTANTE il contatto"), Covered.FrozenSince != INDEX_NONE);
	TestTrue(TEXT("e sull'esagono liscio pure"), Open.FrozenSince != INDEX_NONE);

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Chi contende quale cella.
//
// I cinque probe qui sopra applicano `Plan.DestCell` DIRETTAMENTE — `Units[i].Cell = NextCells[i]` — cioe'
// saltano la risoluzione simultanea. E' li' che vive il difetto: nel log della configurazione spedita
// `fermo: cella contesa` e' l'evento dominante, 43 volte in 12 round.
//
// ⚠️ **Il TurnLog non puo' rispondere alla domanda**, e per questo serve un probe: la voce porta la cella di
// PARTENZA — `RTTurnManager.cpp`, *«la chiave e' la cella di PARTENZA (Paths[i][0])»* — non la cella contesa.
// Quali unita' si contendano quale cella non e' mai stato scritto da nessuna parte, e decide DOVE va la
// correzione: fra compagni di squadra e' pianificazione del bot, fra avversari e' la regola di contesa.
//
// ⚠️ Il `p50` che si legge nel log e' un'altra cosa ancora: `RTTurnManager` lo scrive nella voce leggendolo
// dal catalogo (`FindCoreAction("Action.Move").Priority`), ma al resolver **non lo passa** — la fase Move
// chiama `BeginHexMovement(Paths)` senza l'array delle priorita'. Il resolver confronta zeri.
// ---------------------------------------------------------------------------------------------------------

namespace
{
	struct FRTProbeContestReport
	{
		int32 Contests = 0;
		int32 SameTeamContests = 0;
		int32 CrossTeamContests = 0;
		int32 TurnsWithAnyMove = 0;
		int32 FirstFrozenTurn = INDEX_NONE;
		int32 BlockedByUnitEvents = 0;
		int32 AttackPlans = 0;
		// ⚠️ La cella contesa e' il PRIMO PASSO che non e' stato fatto, non la destinazione: due unita' con
		// destinazioni diverse possono collidere sul primo passo di un percorso condiviso. Le due cose
		// vogliono correzioni diverse — una prenotazione delle destinazioni non scioglierebbe la seconda —
		// quindi si contano separate invece di dedurre l'una dall'altra.
		int32 ContestsWithSameDestination = 0;
		int32 ContestsWithDifferentDestination = 0;
		/**
		 * Il sottoinsieme che la prenotazione PUO' chiudere: due COMPAGNE sulla stessa destinazione.
		 *
		 * ⚠️ Separato da `ContestsWithSameDestination` perche' quello conta anche le coincidenze fra
		 * avversari, che nessuna pianificazione di squadra previene — asserirlo `== 0` farebbe fallire il
		 * test su una condizione di cui la feature non risponde.
		 */
		int32 SameTeamContestsWithSameDestination = 0;
	};
}

/**
 * Lo stesso ciclo percezione -> decisione dei probe precedenti, ma i piani passano da `ResolveHexPaths`
 * invece di essere applicati: e' la differenza fra il ciclo di DECISIONE, gia' isolato e corretto, e la
 * RISOLUZIONE SIMULTANEA, dove il difetto vive.
 */
static FRTProbeContestReport RTRunSimultaneousResolutionProbe(URTHexMapAsset* Arena, FAutomationTestBase& T,
	const TCHAR* Label, bool bPlanAsTeam = false)
{
	FRTProbeContestReport Out;

	TArray<FRTProbeUnit> Units = MakeProbeRoster();
	TArray<FRTTeamKnowledge> Knowledge;
	Knowledge.SetNum(2);

	for (int32 Turn = 1; Turn <= 12; ++Turn)
	{
		int32 Perceived = 0;
		FRTHexSnapshot Snapshot;
		TArray<FRTCellId> Planned;
		PlanProbeTurn(Arena, Units, Knowledge, Turn, Snapshot, Planned, Perceived, Out.AttackPlans, bPlanAsTeam);

		// --- 3. I percorsi come li costruisce la fase Move: la rotta autorevole verso la destinazione
		// pianificata, e chi non ha un percorso valido resta fermo occupando la propria cella.
		TArray<TArray<FRTCellId>> Paths;
		for (int32 i = 0; i < Units.Num(); ++i)
		{
			TArray<FRTCellId> Path;
			if (!(Planned[i] == Units[i].Cell))
			{
				Path = URTHexSimLibrary::FindPathForUnit(Snapshot, Units[i].Id, Planned[i]).Path;
			}
			if (Path.Num() < 2) { Path = { Units[i].Cell }; }
			Paths.Add(Path);
		}

		// ⚠️ Un solo argomento, ed e' il punto: `RTTurnManager` in fase Move chiama `BeginHexMovement(Paths)`
		// senza `Priorities`. Usare qui l'overload con le priorita' misurerebbe una fase che non esiste.
		const TArray<FRTHexMoveResult> Resolved = URTHexSimLibrary::ResolveHexPaths(Paths);

		// --- 4. La cella contesa e' il passo che l'unita' NON ha fatto: quello dopo l'ultimo entrato.
		TArray<FRTCellId> ContestedCell;
		ContestedCell.Init(FRTCellId(), Units.Num());
		TArray<bool> bContested;
		bContested.Init(false, Units.Num());

		int32 MovedThisTurn = 0;
		for (int32 i = 0; i < Units.Num(); ++i)
		{
			if (Resolved[i].Entered.Num() > 0) { ++MovedThisTurn; }
			if (Resolved[i].Outcome == ERTMoveOutcome::BlockedByUnit) { ++Out.BlockedByUnitEvents; }
			if (Resolved[i].Outcome != ERTMoveOutcome::BlockedContested) { continue; }

			const int32 Step = Resolved[i].Entered.Num();
			// ⚠️ Non un `continue` silenzioso: un'unita' dichiarata `BlockedContested` deve avere un passo
			// successivo nel proprio percorso. Se non ce l'ha il modello e' rotto, e tacerlo toglierebbe un
			// contendente dal censimento — falsando proprio la misura che questo file esiste per produrre.
			if (!Paths[i].IsValidIndex(Step + 1))
			{
				T.AddError(FString::Printf(
					TEXT("[%s] turno %d: u%d e' BlockedContested ma il suo percorso non ha un passo %d"),
					Label, Turn, Units[i].Id, Step + 1));
				continue;
			}
			ContestedCell[i] = Paths[i][Step + 1];
			bContested[i] = true;
		}

		// Una riga per CELLA contesa, non per unita' bloccata: la domanda e' «chi la contende», e contare le
		// unita' risponderebbe a un'altra — la stessa distinzione che il log della partita non fa.
		TArray<FRTCellId> AlreadyReported;
		for (int32 i = 0; i < Units.Num(); ++i)
		{
			if (!bContested[i] || AlreadyReported.Contains(ContestedCell[i])) { continue; }
			AlreadyReported.Add(ContestedCell[i]);

			// 🔴 **I contendenti si derivano dai PERCORSI, non dal sottoinsieme marcato `BlockedContested`.**
			// La marcatura e' un esito, e ne esistono altri per la stessa collisione: chi perde per priorita'
			// prende `BlockedByPriority`, chi trova la cella gia' occupata prende `BlockedByUnit`. Contando
			// solo i marcati, una contesa fra AVVERSARI in cui uno dei due esce con un altro esito lascia un
			// solo contendente nel censimento, `PerTeam` diventa {1,0}, e la contesa viene archiviata come
			// «stessa squadra». L'asserzione `CrossTeamContests == 0` — su cui poggia l'intera misura —
			// diventerebbe vera per costruzione invece che per misura.
			//
			// Chi punta quella cella nel proprio prossimo passo la sta contendendo, qualunque esito abbia poi.
			// ⚠️ **Si contano DUE coincidenze, e servono a due cose diverse.**
			//
			//   `bSharedDestination`  — fra contendenti QUALSIASI: descrive la contesa e alimenta il log.
			//                           Ristretta ai soli compagni classificava «collisione di percorso»
			//                           due avversari che puntavano la stessa cella, il che e' falso.
			//   `bSharedWithinTeam`   — fra COMPAGNI: e' il sottoinsieme che la prenotazione puo' chiudere,
			//                           ed e' l'unico su cui un oracolo puo' pretendere zero. Sulla prima
			//                           il test avrebbe fallito per una coincidenza fra avversari, che
			//                           nessuna pianificazione di squadra previene.
			// ⚠️ **Si cerca una COPPIA che condivide la destinazione, non «tutte uguali».** Con `FirstDestination`
			// per squadra si confrontava ogni contendente col PRIMO soltanto: tre unita' con destinazioni
			// A, B, B non producevano nessuna coincidenza, e la coppia B/B finiva in «collisione di percorso».
			// Oggi il probe e' 2v2 e non si vede; il ciclo pero' e' generico su `Units.Num()`.
			//
			// ⚠️ E la coincidenza si cerca **anche fra squadre diverse**: due avversari che puntano la stessa
			// cella finale sono una contesa di destinazione, e classificarla «di percorso» era falso.
			FString Who;
			int32 PerTeam[2] = { 0, 0 };
			TArray<FRTCellId> SeenDestinations;
			TArray<FRTCellId> SeenPerTeam[2];
			bool bSharedDestination = false;
			bool bSharedWithinTeam = false;
			for (int32 j = 0; j < Units.Num(); ++j)
			{
				const int32 StepJ = Resolved[j].Entered.Num();
				if (!Paths[j].IsValidIndex(StepJ + 1)) { continue; }        // fermo o percorso esaurito
				if (!(Paths[j][StepJ + 1] == ContestedCell[i])) { continue; }

				Who += FString::Printf(TEXT("%su%d(T%d, dest %s, esito %d)"),
					Who.IsEmpty() ? TEXT("") : TEXT(", "),
					Units[j].Id, Units[j].Team, *Planned[j].ToString(),
					static_cast<int32>(Resolved[j].Outcome));

				const int32 Team = Units[j].Team;
				if (Team != 0 && Team != 1) { continue; }
				++PerTeam[Team];
				if (SeenDestinations.Contains(Planned[j])) { bSharedDestination = true; }
				else { SeenDestinations.Add(Planned[j]); }

				if (SeenPerTeam[Team].Contains(Planned[j])) { bSharedWithinTeam = true; }
				else { SeenPerTeam[Team].Add(Planned[j]); }
			}

			// Cross-team richiede contendenti da ENTRAMBE le squadre. Un gruppo con un solo contendente non e'
			// una contesa: e' un difetto del censimento, e va detto invece di essere classificato.
			if (PerTeam[0] + PerTeam[1] < 2)
			{
				T.AddError(FString::Printf(
					TEXT("[%s] turno %d: cella %s marcata contesa ma con %d contendente/i — censimento incoerente"),
					Label, Turn, *ContestedCell[i].ToString(), PerTeam[0] + PerTeam[1]));
				continue;
			}
			const bool bSameTeam = (PerTeam[0] == 0) || (PerTeam[1] == 0);
			++Out.Contests;
			if (bSameTeam) { ++Out.SameTeamContests; } else { ++Out.CrossTeamContests; }
			if (bSharedDestination) { ++Out.ContestsWithSameDestination; }
			else { ++Out.ContestsWithDifferentDestination; }
			if (bSharedWithinTeam) { ++Out.SameTeamContestsWithSameDestination; }

			T.AddInfo(FString::Printf(TEXT("[%s] turno %2d: cella %s contesa da %s -> %s, destinazioni %s"),
				Label, Turn, *ContestedCell[i].ToString(), *Who,
				bSameTeam ? TEXT("STESSA squadra") : TEXT("squadre DIVERSE"),
				bSharedDestination
				? (bSharedWithinTeam ? TEXT("CONDIVISA fra COMPAGNI") : TEXT("CONDIVISA fra avversari"))
				: TEXT("DIVERSE (collisione di percorso)")));
		}

		if (MovedThisTurn > 0)
		{
			++Out.TurnsWithAnyMove;
		}
		else if (Out.FirstFrozenTurn == INDEX_NONE)
		{
			Out.FirstFrozenTurn = Turn;
		}

		T.AddInfo(FString::Printf(TEXT("[%s] turno %2d: si sono mosse %d/%d | percepiti %d | piani con attacco %d"),
			Label, Turn, MovedThisTurn, Units.Num(), Perceived, Out.AttackPlans));

		// --- 5. Si applica il RISULTATO, non il piano: e' l'anello che i probe precedenti saltavano.
		for (int32 i = 0; i < Units.Num(); ++i)
		{
			Units[i].Cell = Resolved[i].Final;
		}
	}

	T.AddInfo(FString::Printf(
		TEXT("[%s] TOTALI: contese %d (stessa squadra %d, squadre diverse %d | destinazione condivisa %d di cui FRA COMPAGNI %d, diversa %d) | turni con almeno una mossa %d/12 | primo turno fermo %d | bloccate da unita' ferma %d"),
		Label, Out.Contests, Out.SameTeamContests, Out.CrossTeamContests,
		Out.ContestsWithSameDestination, Out.SameTeamContestsWithSameDestination,
		Out.ContestsWithDifferentDestination, Out.TurnsWithAnyMove,
		Out.FirstFrozenTurn, Out.BlockedByUnitEvents));

	return Out;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotStalemateContendersTest,
	"RefactorTactics.Bot.StalemateProbeContendersAreNamed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTBotStalemateContendersTest::RunTest(const FString&)
{
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(GetTransientPackage());
	if (!TestNotNull(TEXT("arena di prova generata"), Arena)) { return false; }
	URTHexMapAsset* Demo = URTMatchSetupLibrary::MakeDemoArena(GetTransientPackage(), 4);
	if (!TestNotNull(TEXT("arena demo generata"), Demo)) { return false; }

	const FRTProbeContestReport Covered = RTRunSimultaneousResolutionProbe(Arena, *this, TEXT("copertura"));
	const FRTProbeContestReport Open = RTRunSimultaneousResolutionProbe(Demo, *this, TEXT("esagono liscio"));

	// 🔴 **IL RISULTATO, e ribalta la domanda di #1088**: le contese ci sono, sono ventiquattro, e sono
	// **tutte fra COMPAGNI DI SQUADRA**. `u1` e `u2` si contendono `(-1,0)` per dodici turni di fila, `u3` e
	// `u4` si contendono `(1,0)`, e nessuna coppia avversaria contende mai niente.
	//
	// ∴ la regola di contesa NON sta sbagliando: due unita' non possono stare nella stessa cella, e a parita'
	// nessuna entra. A sbagliare e' la pianificazione, che decide **un'unita' alla volta** (`PlanUnit` non
	// riceve i piani delle compagne) e produce due volte la stessa destinazione. Un tie-break nel resolver
	// farebbe entrare una delle due e lascerebbe l'altra a ripetere la stessa scelta perdente al turno dopo.
	TestTrue(TEXT("con copertura le contese ci sono"), Covered.Contests > 0);
	// ⚠️ Una riga sola, ed e' voluto: `Contests == SameTeam + CrossTeam` per costruzione, quindi
	// «SameTeamContests == Contests» sarebbe aritmetica, non una seconda misura. Sembrava una prova in piu'
	// e gonfiava l'evidenza della tesi centrale di questo file.
	TestEqual(TEXT("e sono TUTTE fra compagni di squadra"), Covered.CrossTeamContests, 0);

	// Il campo non si sblocca MAI: zero turni con almeno una mossa su dodici. E' lo stallo della partita
	// reale, riprodotto senza mondo ne' attori.
	TestEqual(TEXT("con copertura non si muove nessuno, per dodici turni"), Covered.TurnsWithAnyMove, 0);

	// 🔴 **Le contese sono di DUE forme, ed e' questo a vincolare la correzione.** Misurato: `u1` e `u2`
	// vogliono la STESSA cella finale — `(2,0,L=1)`, l'high ground a est — mentre `u3` e `u4` ne vogliono due
	// diverse, `(2,0,L=1)` e `(2,-1,L=1)`, e collidono sul PRIMO PASSO di percorsi che condividono `(1,0,L=0)`.
	//
	// ∴ prenotare le sole destinazioni durante la pianificazione scioglierebbe **meta'** dello stallo. L'altra
	// meta' non e' una questione di destinazioni: sono due rotte diverse che passano dalla stessa cella.
	//
	// ⚠️ Le due righe non hanno numeri letterali di proposito: il fatto che decide e' che **entrambe le forme
	// esistano**, non che siano dodici e dodici — quel rapporto dipende dall'arena e invecchierebbe da solo.
	TestTrue(TEXT("esistono contese sulla stessa destinazione"), Covered.ContestsWithSameDestination > 0);
	TestTrue(TEXT("ed esistono collisioni di percorso, fra destinazioni diverse"),
		Covered.ContestsWithDifferentDestination > 0);

	// ⚠️ **Il controllo che toglie la vacuita'.** Sull'esagono liscio — la mappa dove in partita si arriva
	// all'eliminazione — lo stesso ciclo non produce **nessuna** contesa e qualcuno si muove. Se il
	// congelamento comparisse anche li' con gli stessi numeri, a essere rotto sarebbe questo modello e non il
	// gioco: e' la stessa difesa del probe headless qui sopra.
	TestEqual(TEXT("sull'esagono liscio nessuna contesa"), Open.Contests, 0);
	TestTrue(TEXT("e li' qualcuno si muove"), Open.TurnsWithAnyMove > 0);

	return true;
}

/**
 * La correzione, misurata sullo STESSO ciclo che ha prodotto il difetto.
 *
 * Cambia una cosa sola rispetto al test qui sopra: le unita' pianificano su uno snapshot PER SQUADRA e
 * prenotano la rotta scelta — come fa `ARTTurnManager::PlanBots` — invece di decidere tutte sullo stesso
 * snapshot congelato. Tutto il resto e' identico, arena compresa, ed e' cio' che rende il confronto una
 * misura invece di due esecuzioni diverse.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotStalemateTeamPlanningBreaksItTest,
	"RefactorTactics.Bot.StalemateBreaksWithTeamPlanning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTBotStalemateTeamPlanningBreaksItTest::RunTest(const FString&)
{
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(GetTransientPackage());
	if (!TestNotNull(TEXT("arena di prova generata"), Arena)) { return false; }

	// ⚠️ **Il controllo di non-vacuita', e qui e' l'intero test.** Se lo stallo non si formasse piu' da solo
	// — per un cambio all'utility, all'arena o al catalogo — allora «con la prenotazione non ci sono contese»
	// sarebbe vero senza che la prenotazione c'entri, e questo test direbbe il falso restando verde. La prima
	// stesura di `RTBotTeamPlanningTests.cpp` e' caduta esattamente cosi', su un allestimento che credevo
	// producesse la contesa e non la produceva.
	const FRTProbeContestReport Before = RTRunSimultaneousResolutionProbe(Arena, *this,
		TEXT("una alla volta"), /*bPlanAsTeam=*/ false);
	TestTrue(TEXT("premessa: pianificate una alla volta, le compagne si bloccano ancora"),
		Before.Contests > 0 && Before.TurnsWithAnyMove == 0);

	const FRTProbeContestReport After = RTRunSimultaneousResolutionProbe(Arena, *this,
		TEXT("come squadra"), /*bPlanAsTeam=*/ true);

	AddInfo(FString::Printf(TEXT("contese: %d -> %d | turni con almeno una mossa: %d -> %d"),
		Before.Contests, After.Contests, Before.TurnsWithAnyMove, After.TurnsWithAnyMove));

	// Le righe che dicono che lo stallo e' sciolto: sparisce cio' che la prenotazione PUO' far sparire, e il
	// campo si muove.
	//
	// 🔴 **L'asserzione era piu' larga della propria tesi, ed e' caduta il 2026-08-22** (#1088) quando il
	// bot ha smesso di parcheggiarsi e ha ricominciato a muoversi: chiedeva `After.Contests == 0`, cioe'
	// ZERO contese di ogni specie, mentre il commento sopra di lei diceva «nessuna contesa **fra compagni**».
	// Misurato dopo: `contese 3 (stessa squadra 2, squadre diverse 1 | destinazione identica 0, diversa 3)`.
	//
	// Nessuna delle tre e' un difetto della prenotazione:
	//   - **1 e' fra squadre diverse**, e nessuna pianificazione di squadra la previene per costruzione —
	//     due avversari non si coordinano;
	//   - **2 sono fra compagni ma su destinazioni DIVERSE**, cioe' collisioni di percorso. Questo file lo
	//     aveva gia' scritto cinquanta righe sopra: «l'altra meta' non e' una questione di destinazioni:
	//     sono due rotte diverse che passano dalla stessa cella».
	//
	// ∴ il criterio che misura la prenotazione e' `SameTeamContestsWithSameDestination`, e li' il risultato
	// e' netto. **Non** `ContestsWithSameDestination`, che conta anche le coincidenze fra avversari.
	// ⚠️ **L'oracolo guarda le contese FRA COMPAGNI, non tutte.** `ContestsWithSameDestination` conta anche
	// le coincidenze fra avversari, che la prenotazione non previene per costruzione — il commento sopra lo
	// dichiara, e asserirlo `== 0` avrebbe fatto fallire il test su una condizione di cui la feature non
	// risponde. E' la stessa specie di difetto che questo file ha gia' corretto una volta: un'asserzione
	// piu' larga della propria tesi.
	TestEqual(TEXT("con la pianificazione di squadra nessuna contesa fra COMPAGNI sulla stessa destinazione"),
		After.SameTeamContestsWithSameDestination, 0);
	TestTrue(FString::Printf(
		TEXT("e la premessa non e' vacua: una alla volta ce n'erano %d"),
		Before.SameTeamContestsWithSameDestination),
		Before.SameTeamContestsWithSameDestination > 0);

	// ⚠️ **L'altra meta' non resta scoperta, e il predicato guarda `After`.** Le collisioni di percorso sono
	// fuori dalla portata della prenotazione delle destinazioni — questo file lo dichiara cinquanta righe
	// sopra — ma se sparissero anche loro, `ContestsWithSameDestination == 0` diventerebbe vero per assenza
	// di contese e non per merito della prenotazione: il test direbbe il falso restando verde.
	//
	// 🔴 Scritto la prima volta su `Before`, che e' l'altra run: non poteva rilevare il difetto che nomina.
	TestTrue(FString::Printf(
		TEXT("e nella run con prenotazione restano contese di percorso: %d (prima ce n'erano %d)"),
		After.ContestsWithDifferentDestination, Before.ContestsWithDifferentDestination),
		After.ContestsWithDifferentDestination > 0);

	// ⚠️ **Un tetto sul TOTALE, che si era perso.** `After.Contests == 0` e' stato tolto perche' pretendeva
	// zero contese di ogni specie — anche quelle fra avversari, che la prenotazione non previene. Ma
	// toglierlo ha lasciato il totale senza limite: una regressione che triplicasse le collisioni di
	// percorso soddisfarebbe tutte le asserzioni rimaste, e `ContestsWithDifferentDestination > 0` e'
	// addirittura PIU' soddisfatta quanto peggio va. Il tetto giusto non e' zero: e' «meno di prima».
	TestTrue(FString::Printf(
		TEXT("la pianificazione di squadra riduce le contese totali: %d -> %d"),
		Before.Contests, After.Contests),
		After.Contests < Before.Contests);

	TestTrue(TEXT("e le unita' si muovono davvero"), After.TurnsWithAnyMove > 0);

	return true;
}



// ============================================================================================
// #1287 — IL 2-CICLO CHIESTO ALLA BOARD, INVECE CHE A UNA PARTITA
// ============================================================================================

namespace
{
	/**
	 * Il ciclo di periodo due di #1287, misurato come CICLO e non come indizio.
	 *
	 * 🔴 **Perche' il filtro sul dominio era necessario perche' il ciclo esistesse, e non un aggravante.**
	 * `URTHexBotLibrary::ScorePlan` non legge `Context.Origin`: il punteggio e' funzione della DESTINAZIONE
	 * (l'unica dipendenza dalla provenienza e' il facing d'arrivo, che vale `WDamage x delta di copertura`).
	 * E `ChooseBestPlan` rompe le parita' con la mossa minima, cioe' **restare vince**. Ne segue che
	 * `A -> B` chiede `punteggio(B) > punteggio(A)` e `B -> A` chiede `punteggio(A) > punteggio(B)`:
	 * incompatibili. **Con il dominio intero un 2-ciclo non e' formabile su nessuna board.**
	 *
	 * Il filtro di #1287 lo rendeva formabile perche' **toglieva l'origine dalle candidate**: su una cella
	 * cieca il bot non poteva restare, quindi finiva sulla migliore che VEDE anche valendo meno. Il ciclo e'
	 * quindi la congiunzione di due condizioni, e serve misurarle **insieme**:
	 *
	 *     (1) dalla cella CIECA `A`, con il filtro acceso, la migliore fra quelle che vedono e' `B`;
	 *     (2) da `B`, dove il filtro e' spento per costruzione, la migliore in assoluto e' di nuovo `A`.
	 *
	 * La (1) da sola non basta, ed e' l'errore che questa misura ha fatto alla prima stesura: la contava da
	 * sola, e su ENTRAMBE le board dava un numero positivo — sull'arena generata la piattaforma `L1` vede e
	 * la cella `(1,0,L0)` sotto di lei e' cieca e piu' vicina — eppure li' il ciclo non si chiude, perche'
	 * dalla cieca il filtro manda **avanti** (sul muro di `q=0`, che vede) e non indietro.
	 *
	 * ⚠️ **La metrica e' la LINEA D'ARIA, che e' la mutazione pre-#1296 e non il codice di oggi.** E' voluto:
	 * la domanda non e' se il bot attuale orbiti — non lo fa, e `ApproachSteps` e' la ragione — ma se questa
	 * board **possa ospitare** il ciclo che l'oracolo dichiara di sorvegliare.
	 *
	 * ⚠️ **Un nemico solo, fermo.** Un'orbita e' un comportamento stazionario, e con il bersaglio che si
	 * muove non esiste una funzione di cui cercare i cicli. E' un'idealizzazione, ed e' quella in cui il
	 * difetto storico e' stato osservato: Riktor fra due celle mentre il resto del campo non decideva.
	 */
	FString RTOrbitCellText(const FRTCellId& Cell)
	{
		return FString::Printf(TEXT("(%d,%d,L%d)"), Cell.X, Cell.Y, Cell.Layer);
	}

	/** Chiave d'una coppia, indipendente da quale delle due e' la cieca: un ciclo non ha un verso. */
	FString RTOrbitPairText(const FRTCellId& A, const FRTCellId& B)
	{
		const bool bFirst = URTHexLibrary::StableLess(A, B);
		return RTOrbitCellText(bFirst ? A : B) + TEXT(" <-> ") + RTOrbitCellText(bFirst ? B : A);
	}

	struct FRTOrbitCycleReport
	{
		int32 WalkableCells = 0;
		int32 SightBlockers = 0;
		/** Quante di quelle bloccano ANCHE il passo. */
		int32 SightBlockersAlsoBlockingMovement = 0;
		/** Coppie `(cella cieca, bersaglio)` in cui il filtro ha dove mandare il bot: il denominatore. */
		int32 PairsExamined = 0;
		/** ... di cui chiudono il 2-ciclo. */
		int32 Cycles = 0;
		FRTCellId FirstBlind;
		FRTCellId FirstSeeing;
		FRTCellId FirstTarget;
		/** Le coppie `cieca <-> che vede` distinte, per poter chiedere se una coppia NOTA e' fra loro. */
		TSet<FString> Pairs;
	};

	/**
	 * Le celle che un budget di movimento raggiunge sul grafo, occupanti esclusi.
	 *
	 * ⚠️ **Non e' `ReachableCells`, e la differenza e' dichiarata**: qui non c'e' partita, quindi nessuna
	 * unita' occupa celle. L'intorno che ne esce e' un SOVRAINSIEME di quello vero, e un sovrainsieme offre
	 * al ciclo piu' celle su cui posarsi — rende «nessun ciclo» piu' difficile da ottenere, non piu' facile.
	 *
	 * ⚠️ Si percorre `GraphNeighbors`, la stessa primitiva su cui `ApproachSteps` costruisce il proprio campo
	 * di distanze: gli archi verticali e i blocchi al movimento sono quelli del gioco, non una loro copia.
	 */
	void RTOrbitCellsWithinBudget(const URTHexMapAsset* Map, const FRTCellId& Origin, int32 Budget,
		TArray<FRTCellId>& Out)
	{
		Out.Reset();
		if (Map == nullptr)
		{
			return;
		}

		TMap<FRTCellId, int32> Best;
		Best.Add(Origin, 0);
		TArray<FRTCellId> Frontier;
		Frontier.Add(Origin);
		while (Frontier.Num() > 0)
		{
			const FRTCellId Cur = Frontier.Pop();
			const int32 CurCost = Best.FindChecked(Cur);
			for (const TPair<FRTCellId, int32>& Arc : URTHexPathLibrary::GraphNeighbors(Map, Cur))
			{
				// Un arco a costo zero farebbe girare la frontiera all'infinito: un passo costa almeno uno.
				const int32 NextCost = CurCost + FMath::Max(1, Arc.Value);
				if (NextCost > Budget)
				{
					continue;
				}
				const int32* Known = Best.Find(Arc.Key);
				if (Known == nullptr || NextCost < *Known)
				{
					Best.Add(Arc.Key, NextCost);
					Frontier.Add(Arc.Key);
				}
			}
		}
		Best.GenerateKeyArray(Out);
	}

	/**
	 * La migliore del dominio, con lo STESSO tie-break di `ChooseBestPlan`: distanza dal bersaglio, poi
	 * mossa minima dall'origine, poi ordine stabile della cella.
	 *
	 * ⚠️ Senza il tie-break «la migliore» sarebbe ambigua e il conteggio dipenderebbe dall'ordine di
	 * enumerazione — cioe' misurerebbe la `TMap` invece della board. E l'ordine stabile e' `StableLess`,
	 * quello del gioco, non un `operator<` scritto qui.
	 */
	int32 RTOrbitArgMin(const TArray<FRTCellId>& Walk, const TArray<int32>& Domain,
		const FRTCellId& Origin, const FRTCellId& Target)
	{
		int32 Best = INDEX_NONE;
		int32 BestDist = MAX_int32;
		int32 BestMove = MAX_int32;
		for (const int32 C : Domain)
		{
			const int32 Dist = URTHexLibrary::HexDistance(Walk[C], Target);
			const int32 Move = URTHexLibrary::HexDistance(Walk[C], Origin);
			bool bBetter = (Best == INDEX_NONE) || Dist < BestDist;
			if (!bBetter && Dist == BestDist)
			{
				if (Move < BestMove)
				{
					bBetter = true;
				}
				else if (Move == BestMove)
				{
					bBetter = URTHexLibrary::StableLess(Walk[C], Walk[Best]);
				}
			}
			if (bBetter)
			{
				Best = C;
				BestDist = Dist;
				BestMove = Move;
			}
		}
		return Best;
	}

	FRTOrbitCycleReport RTMeasureOrbitCycles(const URTHexMapAsset* Map, int32 Budget)
	{
		FRTOrbitCycleReport Report;
		if (Map == nullptr)
		{
			return Report;
		}

		TArray<FRTCellId> Walk;
		for (const FRTHexCellData& Cell : Map->Cells)
		{
			if (Cell.bBlocksLineOfSight)
			{
				++Report.SightBlockers;
				if (Cell.bBlocksMovement)
				{
					++Report.SightBlockersAlsoBlockingMovement;
				}
			}
			if (!Cell.bBlocksMovement)
			{
				Walk.Add(Cell.Id);
			}
		}
		Report.WalkableCells = Walk.Num();

		const int32 N = Walk.Num();
		if (N == 0)
		{
			return Report;
		}

		TMap<FRTCellId, int32> Index;
		for (int32 I = 0; I < N; ++I)
		{
			Index.Add(Walk[I], I);
		}

		// La linea si chiede nel verso OFFENSIVO (`cella -> bersaglio`), lo stesso di `BuildCandidates` e del
		// termine di ingaggio: `HexLine` costruisce la linea sul layer del TIRATORE, e fra layer diversi i
		// due versi non coincidono.
		TArray<uint8> Sees;
		Sees.SetNumZeroed(N * N);
		for (int32 A = 0; A < N; ++A)
		{
			for (int32 B = 0; B < N; ++B)
			{
				Sees[A * N + B] = URTHexVisionLibrary::HasLineOfSight(Map, Walk[A], Walk[B]) ? 1 : 0;
			}
		}

		TArray<TArray<int32>> Neigh;
		Neigh.SetNum(N);
		TArray<FRTCellId> Reach;
		for (int32 O = 0; O < N; ++O)
		{
			RTOrbitCellsWithinBudget(Map, Walk[O], Budget, Reach);
			for (const FRTCellId& Cell : Reach)
			{
				if (const int32* Found = Index.Find(Cell))
				{
					Neigh[O].Add(*Found);
				}
			}
		}

		TArray<int32> Filtered;
		TArray<int32> Full;
		for (int32 E = 0; E < N; ++E)
		{
			for (int32 A = 0; A < N; ++A)
			{
				// `A` dev'essere CIECA: e' la condizione con cui il filtro di #1287 si accendeva, ed e' anche
				// la ragione per cui li' il bot non poteva colpire — senza linea di tiro non c'e' attacco.
				if (A == E || Sees[A * N + E] != 0)
				{
					continue;
				}

				// (1) Il filtro acceso: solo le celle DA CUI SI VEDE. L'origine e' cieca, quindi e' fuori —
				// ed e' esattamente cio' che permette al bot di muoversi verso una cella che vale meno.
				Filtered.Reset();
				for (const int32 C : Neigh[A])
				{
					if (C != E && Sees[C * N + E] != 0)
					{
						Filtered.Add(C);
					}
				}
				if (Filtered.Num() == 0)
				{
					continue; // il filtro non ha dove mandarlo: nessun primo passo, nessun ciclo
				}
				++Report.PairsExamined;

				const int32 B = RTOrbitArgMin(Walk, Filtered, Walk[A], Walk[E]);

				// (2) Il ritorno, e qui il filtro e' SPENTO: `B` vede, quindi il dominio e' intero. Se la
				// migliore in assoluto e' di nuovo `A`, l'anello si chiude e non si ferma piu'.
				Full.Reset();
				for (const int32 C : Neigh[B])
				{
					if (C != E)
					{
						Full.Add(C);
					}
				}
				if (RTOrbitArgMin(Walk, Full, Walk[B], Walk[E]) != A)
				{
					continue;
				}

				if (Report.Cycles == 0)
				{
					Report.FirstBlind = Walk[A];
					Report.FirstSeeing = Walk[B];
					Report.FirstTarget = Walk[E];
				}
				++Report.Cycles;
				Report.Pairs.Add(RTOrbitPairText(Walk[A], Walk[B]));
			}
		}

		return Report;
	}


	/** Il conteggio su tutta la spazzata dei budget, piu' la prima occorrenza in forma leggibile. */
	struct FRTOrbitSweep
	{
		int32 Cycles = 0;
		int32 Pairs = 0;
		FString First;
		TSet<FString> DistinctPairs;
	};

	FRTOrbitSweep RTSweepOrbitCycles(const URTHexMapAsset* Map)
	{
		FRTOrbitSweep Sweep;
		// ⚠️ **Il budget si SPAZZA, non si indovina.** Legarlo al valore di un eroe farebbe dipendere una
		// proprieta' della board da una taratura del roster, e il roster si ritara in #149.
		for (int32 Budget = 2; Budget <= 8; ++Budget)
		{
			const FRTOrbitCycleReport R = RTMeasureOrbitCycles(Map, Budget);
			Sweep.Cycles += R.Cycles;
			Sweep.Pairs += R.PairsExamined;
			Sweep.DistinctPairs.Append(R.Pairs);
			if (R.Cycles > 0 && Sweep.First.IsEmpty())
			{
				Sweep.First = FString::Printf(TEXT("budget %d: cieca %s <-> che vede %s, bersaglio %s"),
					Budget, *RTOrbitCellText(R.FirstBlind), *RTOrbitCellText(R.FirstSeeing),
					*RTOrbitCellText(R.FirstTarget));
			}
		}
		return Sweep;
	}
}

/**
 * **Perche' nessuna mutazione del bot fa oscillare qualcuno sull'arena generata.**
 *
 * `Match.Autobattle.EngagesOnTheGeneratedTestArena` porta un'asserzione anti-oscillazione, e la sua verifica
 * di mutazione dice due cose: il RILEVATORE falsifica (mutandolo cadono tre test), ma nessuna mutazione del
 * BOT lo fa cadere li' — sotto la piu' forte, l'avvicinamento in linea d'aria del pre-#1296, il contatore va
 * a ZERO mentre sulla mappa d'autore va a sette. Restava da sapere se mancasse la mutazione o la board.
 *
 * ✅ **Manca la board, e la ragione e' nel secondo passo del ciclo.** Il primo passo c'e' anche qui: dalla
 * piattaforma `L1`, che vede, la cella `(1,0,L0)` sotto di lei e' cieca e piu' vicina in linea d'aria — la
 * stessa figura del difetto storico. Ma dalla cieca il filtro di #1287 manda **avanti**: il muro di `q=0`
 * blocca la vista e **non il passo**, e `HasLineOfSight` esclude gli estremi dalla regola per-cella
 * (*«non ci si oscura da soli»*), quindi la cella del muro e' insieme piu' vicina al bersaglio **e** una
 * cella che vede. L'anello non torna indietro. Sulla mappa d'autore l'ostacolo centrale blocca vista **e**
 * passo — la riga che #1287 cita, *«(-1,1) e (1,-1) distano 2 ma sono ai lati opposti di (0,0), che blocca
 * vista e passo»* — e li' l'unica cella che vede sta dietro, quindi il ciclo si chiude.
 *
 * ⚠️ **Cosa questo test NON dice.** Non dice «il bot non puo' orbitare»: dice che il ciclo **nella forma di
 * #1287** — dominio ristretto sull'origine cieca, avvicinamento in linea d'aria — non ha su questa board
 * dove chiudersi. Un termine di punteggio che dipenda dall'origine in un altro modo riaprirebbe la domanda,
 * e quel giorno si rimisura qui.
 *
 * ∴ il verde di `EngagesOnTheGeneratedTestArena` va letto cosi': la soglia e' esercitata da dati reali —
 * mutando il rilevatore l'asserzione cade — e il percorso di comportamento che le manca **non esiste su
 * quella board**, non e' stato cercato male.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotGeneratedArenaClosesNoOrbitCycleTest,
	"RefactorTactics.Bot.StalemateProbeGeneratedArenaClosesNoOrbitCycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotGeneratedArenaClosesNoOrbitCycleTest::RunTest(const FString&)
{
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(GetTransientPackage());
	if (!TestNotNull(TEXT("arena di prova generata"), Arena)) { return false; }

	// La CAUSA, prima della conseguenza: su questa board nessuna cella che blocca la vista blocca il passo.
	// E' la riga che manda il filtro avanti invece che indietro; se cade, la conclusione va rifatta.
	const FRTOrbitCycleReport Structure = RTMeasureOrbitCycles(Arena, /*Budget=*/ 4);
	AddInfo(FString::Printf(
		TEXT("arena generata: %d celle percorribili, %d bloccano la vista, di cui %d bloccano anche il passo"),
		Structure.WalkableCells, Structure.SightBlockers, Structure.SightBlockersAlsoBlockingMovement));
	TestTrue(TEXT("premessa: l'arena ha un muro che blocca la vista"), Structure.SightBlockers > 0);
	TestEqual(TEXT("e nessuna di quelle celle blocca il passo: sul muro ci si sale, e da li' si vede"),
		Structure.SightBlockersAlsoBlockingMovement, 0);

	const FRTOrbitSweep Sweep = RTSweepOrbitCycles(Arena);
	AddInfo(FString::Printf(TEXT("budget 2..8: %d cicli chiusi su %d coppie (cella cieca, bersaglio) %s"),
		Sweep.Cycles, Sweep.Pairs, Sweep.First.IsEmpty() ? TEXT("") : *Sweep.First));

	// La misura ha un denominatore: senza, «zero cicli» sarebbe il numero di una board senza celle cieche.
	TestTrue(FString::Printf(TEXT("il filtro ha dove mandare il bot in %d coppie"), Sweep.Pairs),
		Sweep.Pairs > 0);

	TestEqual(FString::Printf(
		TEXT("sull'arena generata il 2-ciclo di #1287 non si chiude in nessuna coppia: %d cicli"),
		Sweep.Cycles), Sweep.Cycles, 0);

	return true;
}

/**
 * **Il controfattuale, sulla stessa domanda: sulla mappa d'autore il ciclo si chiude.**
 *
 * ⚠️ **Esiste per non far dire al test qui sopra piu' di quanto misura** — la stessa ragione per cui questo
 * file affianca l'arena demo all'arena di prova. «Zero cicli» significa qualcosa solo se si sa quanti ne ha
 * una board dove il ciclo si e' formato davvero: senza termine di paragone, uno zero puo' venire dal
 * predicato sbagliato invece che dalla geometria. La prima stesura di questa misura contava una condizione
 * NECESSARIA invece del ciclo, e su entrambe le board dava un numero positivo: era il controfattuale a
 * mancare di potere discriminante, non la board.
 *
 * 🔴 **Ed e' la premessa di `NobodyOscillatesOnTheAuthoredMap`.** Quell'oracolo cade sotto la mutazione in
 * linea d'aria (1 -> 7 ritorni su limite 4); il suo gemello sull'arena generata no. Se un giorno questo
 * numero andasse a zero — un rimaneggiamento della mappa che rende l'ostacolo centrale attraversabile —
 * allora **anche quell'oracolo** perderebbe il proprio percorso di comportamento, e la verifica di mutazione
 * di #1287 andrebbe rifatta invece che ereditata. E' il giorno in cui si vuole essere avvisati.
 *
 * ⚠️ **Asserisce su un `.uasset` di DIR-A, e lo dichiara**: un rosso qui non e' un difetto del bot, e' un
 * cambio di contenuto che sposta il significato di un test del bot.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotAuthoredMapClosesTheOrbitCycleTest,
	"RefactorTactics.Bot.StalemateProbeAuthoredMapClosesTheOrbitCycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotAuthoredMapClosesTheOrbitCycleTest::RunTest(const FString&)
{
	const TCHAR* AssetPath = TEXT("/Game/RT/Maps/Dev/L_HexArena/Data/DA_HexMap_Arena.DA_HexMap_Arena");
	URTHexMapAsset* Authored =
		Cast<URTHexMapAsset>(StaticLoadObject(URTHexMapAsset::StaticClass(), nullptr, AssetPath));
	if (!TestNotNull(TEXT("la mappa d'autore si carica"), Authored)) { return false; }

	const FRTOrbitCycleReport Structure = RTMeasureOrbitCycles(Authored, /*Budget=*/ 4);
	AddInfo(FString::Printf(
		TEXT("mappa d'autore: %d celle percorribili, %d bloccano la vista, di cui %d bloccano anche il passo"),
		Structure.WalkableCells, Structure.SightBlockers, Structure.SightBlockersAlsoBlockingMovement));

	// La differenza fra le due board, in una riga: qui i muri sono anche ostacoli.
	TestTrue(TEXT("sulla mappa d'autore ci sono celle che bloccano vista E passo"),
		Structure.SightBlockersAlsoBlockingMovement > 0);

	const FRTOrbitSweep Sweep = RTSweepOrbitCycles(Authored);
	AddInfo(FString::Printf(TEXT("budget 2..8: %d cicli chiusi su %d coppie (cella cieca, bersaglio) %s"),
		Sweep.Cycles, Sweep.Pairs, Sweep.First.IsEmpty() ? TEXT("nessuno") : *Sweep.First));

	TestTrue(FString::Printf(TEXT("il filtro ha dove mandare il bot in %d coppie"), Sweep.Pairs),
		Sweep.Pairs > 0);

	TestTrue(FString::Printf(
		TEXT("sulla mappa d'autore il 2-ciclo di #1287 si chiude: %d cicli (sull'arena generata sono zero)"),
		Sweep.Cycles), Sweep.Cycles > 0);

	TArray<FString> Elencate = Sweep.DistinctPairs.Array();
	Elencate.Sort();
	AddInfo(FString::Printf(TEXT("coppie distinte (%d): %s"),
		Elencate.Num(), *FString::Join(Elencate, TEXT("  |  "))));

	// 🔴 **E fra quelle coppie c'e' QUELLA MISURATA, che e' cio' che rende questo predicato una misura e non
	// un modello plausibile.** Il consuntivo di #1287 nomina l'orbita osservata in partita — *«Riktor alterna
	// fra `(1,-1,L0)` e la piattaforma `(3,-3,L1)` otto volte in dodici turni»* — e il predicato, che di
	// quella misura non sa nulla, la ritrova da solo enumerando la board. Un predicato che trovasse cicli
	// altrove ma non questo starebbe descrivendo un difetto diverso da quello accaduto.
	const FString Storica = RTOrbitPairText(FRTCellId(1, -1, 0), FRTCellId(3, -3, 1));
	TestTrue(FString::Printf(
		TEXT("fra i cicli c'e' quello MISURATO da #1287 in partita: %s"), *Storica),
		Sweep.DistinctPairs.Contains(Storica));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
