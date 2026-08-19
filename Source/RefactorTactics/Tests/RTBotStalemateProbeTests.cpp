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

		// ⛔ **Una chiamata di `PlanTeam` PER SQUADRA**, come la sua firma prescrive: le prenotazioni sono
		// informazione sui piani, e i piani di una squadra sono privati. Passando qui tutte e quattro le
		// unita' insieme, un bot schiverebbe la cella di un avversario — un intento che nessun giocatore
		// puo' vedere (CP 13.5).
		OutPlanned.SetNum(Units.Num());
		for (int32 Team = 0; Team < 2; ++Team)
		{
			TArray<int32> TeamIds;
			TArray<FRTHexBotContext> TeamContexts;
			TArray<int32> SlotOf;              // dove rimettere il piano, per non perdere il parallelismo
			for (int32 I = 0; I < Units.Num(); ++I)
			{
				if (Units[I].Team != Team) { continue; }
				TeamIds.Add(Units[I].Id);
				TeamContexts.Add(Contexts[I]);
				SlotOf.Add(I);
			}
			if (TeamIds.Num() == 0) { continue; }

			const TArray<FRTHexBotPlan> Plans = URTHexBotLibrary::PlanTeam(OutSnapshot, TeamIds, TeamContexts);
			for (int32 K = 0; K < Plans.Num() && K < SlotOf.Num(); ++K)
			{
				if (Plans[K].bHasAttack) { ++OutAttackPlans; }
				OutPlanned[SlotOf[K]] = Plans[K].DestCell;
			}
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
			if (!Paths[i].IsValidIndex(Step + 1)) { continue; }
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

			FString Who;
			int32 PerTeam[2] = { 0, 0 };
			// La destinazione del primo contendente, per dire se gli altri puntano LI' o solo di passaggio.
			FRTCellId FirstDestination = Planned[i];
			bool bAllSameDestination = true;
			for (int32 j = 0; j < Units.Num(); ++j)
			{
				if (!bContested[j] || !(ContestedCell[j] == ContestedCell[i])) { continue; }
				Who += FString::Printf(TEXT("%su%d(T%d, dest %s)"), Who.IsEmpty() ? TEXT("") : TEXT(", "),
					Units[j].Id, Units[j].Team, *Planned[j].ToString());
				if (Units[j].Team == 0 || Units[j].Team == 1) { ++PerTeam[Units[j].Team]; }
				if (!(Planned[j] == FirstDestination)) { bAllSameDestination = false; }
			}

			const bool bSameTeam = (PerTeam[0] == 0) || (PerTeam[1] == 0);
			++Out.Contests;
			if (bSameTeam) { ++Out.SameTeamContests; } else { ++Out.CrossTeamContests; }
			if (bAllSameDestination) { ++Out.ContestsWithSameDestination; }
			else { ++Out.ContestsWithDifferentDestination; }

			T.AddInfo(FString::Printf(TEXT("[%s] turno %2d: cella %s contesa da %s -> %s, destinazioni %s"),
				Label, Turn, *ContestedCell[i].ToString(), *Who,
				bSameTeam ? TEXT("STESSA squadra") : TEXT("squadre DIVERSE"),
				bAllSameDestination ? TEXT("IDENTICHE") : TEXT("DIVERSE (collisione di percorso)")));
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
		TEXT("[%s] TOTALI: contese %d (stessa squadra %d, squadre diverse %d | destinazione identica %d, diversa %d) | turni con almeno una mossa %d/12 | primo turno fermo %d | bloccate da unita' ferma %d"),
		Label, Out.Contests, Out.SameTeamContests, Out.CrossTeamContests,
		Out.ContestsWithSameDestination, Out.ContestsWithDifferentDestination, Out.TurnsWithAnyMove,
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
	TestEqual(TEXT("e sono TUTTE fra compagni di squadra"), Covered.CrossTeamContests, 0);
	TestEqual(TEXT("nessuna contesa fra avversari, contate due volte"),
		Covered.SameTeamContests, Covered.Contests);

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
 * Cambia una cosa sola rispetto al test qui sopra: le unita' sono pianificate con `PlanTeam` — una chiamata
 * per squadra — invece che con `PlanUnit` una alla volta. Tutto il resto e' identico, arena compresa, ed e'
 * cio' che rende il confronto una misura invece di due esecuzioni diverse.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotStalemateTeamPlanningBreaksItTest,
	"RefactorTactics.Bot.StalemateBreaksWithTeamPlanning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTBotStalemateTeamPlanningBreaksItTest::RunTest(const FString&)
{
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeTestArena(GetTransientPackage());
	if (!TestNotNull(TEXT("arena di prova generata"), Arena)) { return false; }

	// ⚠️ **Il controllo di non-vacuita', e qui e' l'intero test.** Se lo stallo non si formasse piu' da solo
	// — per un cambio all'utility, all'arena o al catalogo — allora «con `PlanTeam` non ci sono contese»
	// sarebbe vero senza che `PlanTeam` c'entri, e questo test direbbe il falso restando verde. La prima
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

	// Le due righe che dicono che lo stallo e' sciolto: nessuna contesa fra compagni, e il campo si muove.
	TestEqual(TEXT("con la pianificazione di squadra le contese spariscono"), After.Contests, 0);
	TestTrue(TEXT("e le unita' si muovono davvero"), After.TurnsWithAnyMove > 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
